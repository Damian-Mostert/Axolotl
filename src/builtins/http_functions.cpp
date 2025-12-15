#include "include/builtins.h"
#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>

// Note: These HTTP functions are complex and require deep interpreter integration
// They are kept as builtins but need special handling for method calls

class CreateServerBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "createServer"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("createServer() expects 2 arguments");
        Value callbackVal = interp->evaluate(node->args[0].get());
        Value portVal = interp->evaluate(node->args[1].get());
        int port = std::get<int>(portVal);
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) throw std::runtime_error("Failed to create socket");
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(server_fd);
            throw std::runtime_error("Failed to bind to port " + std::to_string(port));
        }
        if (::listen(server_fd, 10) < 0) {
            close(server_fd);
            throw std::runtime_error("Failed to listen on port " + std::to_string(port));
        }
        std::cout << "Server listening on http://localhost:" << port << std::endl;
        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) continue;
            char buffer[4096] = {0};
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                std::string request(buffer, bytes_read);
                std::istringstream iss(request);
                std::string method, path, version;
                iss >> method >> path >> version;
                auto req = std::make_shared<ObjectValue>();
                req->fields["method"] = method;
                req->fields["url"] = path;
                req->fields["path"] = path;
                auto res = std::make_shared<ObjectValue>();
                res->fields["_fd"] = client_fd;
                res->fields["_statusCode"] = 200;
                if (std::holds_alternative<FunctionExpression*>(callbackVal)) {
                    auto func = std::get<FunctionExpression*>(callbackVal);
                    if (func->params.size() == 2) {
                        interp->environment.pushScope();
                        interp->environment.define(func->params[0].first, Variable(req, "object", false));
                        interp->environment.define(func->params[1].first, Variable(res, "object", false));
                        try {
                            interp->executeBlock(func->body.get());
                        } catch (const ReturnException&) {}
                        interp->environment.popScope();
                    }
                }
            }
            close(client_fd);
        }
        close(server_fd);
        return "";
    }
};

class WriteHeadBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "writeHead"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) return "";
        if (node->args.size() < 1 || node->args.size() > 2) throw std::runtime_error("writeHead() expects 1 or 2 arguments");
        if (auto fieldAccess = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value resVal = interp->evaluate(fieldAccess->object.get());
            if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(resVal)) throw std::runtime_error("writeHead() must be called on response object");
            auto res = std::get<std::shared_ptr<ObjectValue>>(resVal);
            Value statusVal = interp->evaluate(node->args[0].get());
            res->fields["_statusCode"] = std::get<int>(statusVal);
            if (node->args.size() == 2) {
                Value headersVal = interp->evaluate(node->args[1].get());
                if (std::holds_alternative<std::shared_ptr<ObjectValue>>(headersVal)) {
                    res->fields["_headers"] = headersVal;
                }
            }
        }
        return "";
    }
};

class WriteBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "write"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) return "";
        if (node->args.size() != 1) throw std::runtime_error("write() expects 1 argument");
        if (auto fieldAccess = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value resVal = interp->evaluate(fieldAccess->object.get());
            if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(resVal)) throw std::runtime_error("write() must be called on response object");
            auto res = std::get<std::shared_ptr<ObjectValue>>(resVal);
            if (res->fields.find("_buffer") == res->fields.end()) {
                res->fields["_buffer"] = std::string("");
            }
            Value dataVal = interp->evaluate(node->args[0].get());
            std::string currentBuffer = std::get<std::string>(res->fields["_buffer"]);
            currentBuffer += interp->valueToString(dataVal);
            res->fields["_buffer"] = currentBuffer;
        }
        return "";
    }
};

class EndBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "end"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) return "";
        if (node->args.size() > 1) throw std::runtime_error("end() expects 0 or 1 argument");
        if (auto fieldAccess = dynamic_cast<FieldAccess*>(node->callee.get())) {
            Value resVal = interp->evaluate(fieldAccess->object.get());
            if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(resVal)) throw std::runtime_error("end() must be called on response object");
            auto res = std::get<std::shared_ptr<ObjectValue>>(resVal);
            int fd = std::get<int>(res->fields["_fd"]);
            int statusCode = std::get<int>(res->fields["_statusCode"]);
            std::string body;
            if (node->args.size() == 1) {
                Value dataVal = interp->evaluate(node->args[0].get());
                body = interp->valueToString(dataVal);
            } else if (res->fields.find("_buffer") != res->fields.end()) {
                body = std::get<std::string>(res->fields["_buffer"]);
            }
            std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " OK\r\n";
            if (std::holds_alternative<std::shared_ptr<ObjectValue>>(res->fields["_headers"])) {
                auto headers = std::get<std::shared_ptr<ObjectValue>>(res->fields["_headers"]);
                for (const auto& [key, val] : headers->fields) {
                    response += key + ": " + interp->valueToString(val) + "\r\n";
                }
            }
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n\r\n" + body;
            ::write(fd, response.c_str(), response.length());
        }
        return "";
    }
};

class FetchBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "fetch"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() < 1 || node->args.size() > 2) throw std::runtime_error("fetch() expects 1 or 2 arguments");
        Value urlVal = interp->evaluate(node->args[0].get());
        std::string url = interp->valueToString(urlVal);
        std::string method = "GET", body = "";
        std::unordered_map<std::string, std::string> headers;
        if (node->args.size() == 2) {
            Value optionsVal = interp->evaluate(node->args[1].get());
            if (std::holds_alternative<std::shared_ptr<ObjectValue>>(optionsVal)) {
                auto options = std::get<std::shared_ptr<ObjectValue>>(optionsVal);
                if (options->fields.find("method") != options->fields.end()) method = interp->valueToString(options->fields["method"]);
                if (options->fields.find("body") != options->fields.end()) {
                    Value bodyVal = options->fields["body"];
                    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(bodyVal)) {
                        auto bodyObj = std::get<std::shared_ptr<ObjectValue>>(bodyVal);
                        body = "{";
                        bool first = true;
                        for (const auto& [key, val] : bodyObj->fields) {
                            if (!first) body += ",";
                            body += "\"" + key + "\":" + (std::holds_alternative<std::string>(val) ? "\"" + std::get<std::string>(val) + "\"" : interp->valueToString(val));
                            first = false;
                        }
                        body += "}";
                    } else body = interp->valueToString(bodyVal);
                }
                if (options->fields.find("headers") != options->fields.end()) {
                    Value headersVal = options->fields["headers"];
                    if (std::holds_alternative<std::shared_ptr<ObjectValue>>(headersVal)) {
                        auto headersObj = std::get<std::shared_ptr<ObjectValue>>(headersVal);
                        for (const auto& [key, val] : headersObj->fields) headers[key] = interp->valueToString(val);
                    }
                }
            }
        }
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("Failed to initialize curl");
        std::string responseBody, responseHeaders;
        long statusCode = 0;
        auto writeCallback = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
            return size * nmemb;
        };
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, +writeCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        } else if (method == "PATCH") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        }
        struct curl_slist* headerList = nullptr;
        for (const auto& [key, val] : headers) headerList = curl_slist_append(headerList, (key + ": " + val).c_str());
        if (headerList) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::string error = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            if (headerList) curl_slist_free_all(headerList);
            throw std::runtime_error("fetch() failed: " + error);
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        curl_easy_cleanup(curl);
        if (headerList) curl_slist_free_all(headerList);
        Value parsedBody = responseBody;
        if (responseBody.size() > 0 && responseBody[0] == '{') {
            try {
                auto jsonObj = std::make_shared<ObjectValue>();
                size_t pos = 1;
                while (pos < responseBody.size() && responseBody[pos] != '}') {
                    while (pos < responseBody.size() && (responseBody[pos] == ' ' || responseBody[pos] == '\n')) pos++;
                    if (pos >= responseBody.size() || responseBody[pos] == '}') break;
                    if (responseBody[pos] != '"') break;
                    pos++;
                    size_t keyStart = pos;
                    while (pos < responseBody.size() && responseBody[pos] != '"') pos++;
                    std::string key = responseBody.substr(keyStart, pos - keyStart);
                    pos++;
                    while (pos < responseBody.size() && (responseBody[pos] == ' ' || responseBody[pos] == ':')) pos++;
                    Value val;
                    if (responseBody[pos] == '"') {
                        pos++;
                        size_t valStart = pos;
                        while (pos < responseBody.size() && responseBody[pos] != '"') { if (responseBody[pos] == '\\') pos++; pos++; }
                        val = responseBody.substr(valStart, pos - valStart);
                        pos++;
                    } else if (responseBody[pos] == 't' || responseBody[pos] == 'f') {
                        val = (responseBody[pos] == 't');
                        pos += (responseBody[pos] == 't') ? 4 : 5;
                    } else if (responseBody[pos] == 'n') {
                        val = std::string("");
                        pos += 4;
                    } else if ((responseBody[pos] >= '0' && responseBody[pos] <= '9') || responseBody[pos] == '-') {
                        size_t numStart = pos;
                        bool isFloat = false;
                        while (pos < responseBody.size() && ((responseBody[pos] >= '0' && responseBody[pos] <= '9') || responseBody[pos] == '.' || responseBody[pos] == '-')) {
                            if (responseBody[pos] == '.') isFloat = true;
                            pos++;
                        }
                        val = isFloat ? Value(std::stof(responseBody.substr(numStart, pos - numStart))) : Value(std::stoi(responseBody.substr(numStart, pos - numStart)));
                    } else val = std::string("");
                    jsonObj->fields[key] = val;
                    while (pos < responseBody.size() && (responseBody[pos] == ' ' || responseBody[pos] == ',')) pos++;
                }
                parsedBody = jsonObj;
            } catch (...) {}
        }
        auto response = std::make_shared<ObjectValue>();
        response->fields["status"] = static_cast<int>(statusCode);
        response->fields["body"] = parsedBody;
        response->fields["headers"] = responseHeaders;
        response->fields["ok"] = (statusCode >= 200 && statusCode < 300);
        response->fields["url"] = url;
        interp->lastValue = response;
        return "{object}";
    }
};

REGISTER_BUILTIN(CreateServerBuiltin)
REGISTER_BUILTIN(WriteHeadBuiltin)
REGISTER_BUILTIN(WriteBuiltin)
REGISTER_BUILTIN(EndBuiltin)
REGISTER_BUILTIN(FetchBuiltin)
