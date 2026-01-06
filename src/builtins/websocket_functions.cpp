#include "include/builtins.h"
#include <libwebsockets.h>
#include <string>
#include <queue>
#include <memory>

struct WSConnection {
    struct lws* wsi = nullptr;
    std::queue<std::string> messageQueue;
    std::string url;
    bool connected = false;
};

static std::unordered_map<int, std::shared_ptr<WSConnection>> wsConnections;
static struct lws_context* wsContext = nullptr;
static int nextWSId = 1;

static int wsCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len) {
    WSConnection* conn = (WSConnection*)user;
    
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            if (conn) conn->connected = true;
            break;
            
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (conn && in && len > 0) {
                conn->messageQueue.push(std::string((char*)in, len));
            }
            break;
            
        case LWS_CALLBACK_CLIENT_CLOSED:
            if (conn) conn->connected = false;
            break;
            
        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {"axolotl-ws", wsCallback, sizeof(WSConnection), 4096, 0, nullptr, 0},
    {nullptr, nullptr, 0, 0, 0, nullptr, 0}
};

class WSConnectBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "wsConnect"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("wsConnect(url)");
        
        std::string url = std::get<std::string>(interp->evaluate(node->args[0].get()));
        
        if (!wsContext) {
            struct lws_context_creation_info info;
            memset(&info, 0, sizeof(info));
            info.port = CONTEXT_PORT_NO_LISTEN;
            info.protocols = protocols;
            info.gid = -1;
            info.uid = -1;
            wsContext = lws_create_context(&info);
            if (!wsContext) throw std::runtime_error("Failed to create WebSocket context");
        }
        
        struct lws_client_connect_info ccinfo;
        memset(&ccinfo, 0, sizeof(ccinfo));
        ccinfo.context = wsContext;
        ccinfo.port = 80;
        ccinfo.ssl_connection = 0;
        
        size_t pos = url.find("://");
        if (pos != std::string::npos) {
            std::string protocol = url.substr(0, pos);
            if (protocol == "wss") ccinfo.ssl_connection = LCCSCF_USE_SSL;
            url = url.substr(pos + 3);
        }
        
        pos = url.find(":");
        if (pos != std::string::npos) {
            ccinfo.address = url.substr(0, pos).c_str();
            size_t pathPos = url.find("/", pos);
            if (pathPos != std::string::npos) {
                ccinfo.port = std::stoi(url.substr(pos + 1, pathPos - pos - 1));
                ccinfo.path = url.substr(pathPos).c_str();
            } else {
                ccinfo.port = std::stoi(url.substr(pos + 1));
                ccinfo.path = "/";
            }
        } else {
            pos = url.find("/");
            if (pos != std::string::npos) {
                ccinfo.address = url.substr(0, pos).c_str();
                ccinfo.path = url.substr(pos).c_str();
            } else {
                ccinfo.address = url.c_str();
                ccinfo.path = "/";
            }
        }
        
        ccinfo.host = ccinfo.address;
        ccinfo.origin = ccinfo.address;
        ccinfo.protocol = protocols[0].name;
        
        auto conn = std::make_shared<WSConnection>();
        conn->url = url;
        ccinfo.userdata = conn.get();
        
        struct lws* wsi = lws_client_connect_via_info(&ccinfo);
        if (!wsi) throw std::runtime_error("WebSocket connection failed");
        
        conn->wsi = wsi;
        int id = nextWSId++;
        wsConnections[id] = conn;
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_wsId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};

class WSSendBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "send"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 1) throw std::runtime_error("ws.send(message)");
        
        auto fa = dynamic_cast<FieldAccess*>(node->callee.get());
        Value wsVal = interp->evaluate(fa->object.get());
        auto wsObj = std::get<std::shared_ptr<ObjectValue>>(wsVal);
        int wsId = std::get<int>(wsObj->fields["_wsId"]);
        
        if (!wsConnections.count(wsId)) throw std::runtime_error("Invalid WebSocket connection");
        auto conn = wsConnections[wsId];
        
        if (!conn->connected) throw std::runtime_error("WebSocket not connected");
        
        std::string message = std::get<std::string>(interp->evaluate(node->args[0].get()));
        
        unsigned char buf[LWS_PRE + message.size()];
        memcpy(&buf[LWS_PRE], message.c_str(), message.size());
        
        lws_write(conn->wsi, &buf[LWS_PRE], message.size(), LWS_WRITE_TEXT);
        return "";
    }
};

class WSReceiveBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "receive"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) throw std::runtime_error("ws.receive()");
        
        auto fa = dynamic_cast<FieldAccess*>(node->callee.get());
        Value wsVal = interp->evaluate(fa->object.get());
        auto wsObj = std::get<std::shared_ptr<ObjectValue>>(wsVal);
        int wsId = std::get<int>(wsObj->fields["_wsId"]);
        
        if (!wsConnections.count(wsId)) throw std::runtime_error("Invalid WebSocket connection");
        auto conn = wsConnections[wsId];
        
        lws_service(wsContext, 0);
        
        if (!conn->messageQueue.empty()) {
            std::string msg = conn->messageQueue.front();
            conn->messageQueue.pop();
            interp->lastValue = msg;
            return msg;
        }
        
        interp->lastValue = std::string("");
        return "";
    }
};

class WSCloseBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "close"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) throw std::runtime_error("ws.close()");
        
        auto fa = dynamic_cast<FieldAccess*>(node->callee.get());
        Value wsVal = interp->evaluate(fa->object.get());
        auto wsObj = std::get<std::shared_ptr<ObjectValue>>(wsVal);
        int wsId = std::get<int>(wsObj->fields["_wsId"]);
        
        if (wsConnections.count(wsId)) {
            auto conn = wsConnections[wsId];
            if (conn->wsi) lws_close_reason(conn->wsi, LWS_CLOSE_STATUS_NORMAL, nullptr, 0);
            wsConnections.erase(wsId);
        }
        return "";
    }
};

REGISTER_BUILTIN(WSConnectBuiltin)
REGISTER_BUILTIN(WSSendBuiltin)
REGISTER_BUILTIN(WSReceiveBuiltin)
REGISTER_BUILTIN(WSCloseBuiltin)
