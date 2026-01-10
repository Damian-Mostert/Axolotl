#include "include/builtins.h"
#include <mysql/mysql.h>
#include <string>
#include <memory>
static std::unordered_map<int, MYSQL*> connections;
static int nextConnId = 1;
class MySQLConnectBuiltin : public BuiltinFunction {
public:
//@desc Connect to MySQL database
    std::string getName() const override { return "mysqlConnect"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() < 4) throw std::runtime_error("mysqlConnect(host, user, password, database)");
        
        std::string host = std::get<std::string>(interp->evaluate(node->args[0].get()));
        std::string user = std::get<std::string>(interp->evaluate(node->args[1].get()));
        std::string password = std::get<std::string>(interp->evaluate(node->args[2].get()));
        std::string database = std::get<std::string>(interp->evaluate(node->args[3].get()));
        
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) throw std::runtime_error("MySQL init failed");
        
        if (!mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), database.c_str(), 0, nullptr, 0)) {
            std::string error = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("MySQL connection failed: " + error);
        }
        
        int id = nextConnId++;
        connections[id] = conn;
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_connId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class MySQLQueryBuiltin : public BuiltinFunction {
public:
//@desc Execute SQL query on database connection
    std::string getName() const override { return "query"; }
//@parent connection
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee || node->args.size() != 1) throw std::runtime_error("conn.query(sql)");
        
        auto fa = dynamic_cast<FieldAccess*>(node->callee.get());
        Value connVal = interp->evaluate(fa->object.get());
        auto connObj = std::get<std::shared_ptr<ObjectValue>>(connVal);
        int connId = std::get<int>(connObj->fields["_connId"]);
        
        if (!connections.count(connId)) throw std::runtime_error("Invalid connection");
        MYSQL* conn = connections[connId];
        
        std::string sql = std::get<std::string>(interp->evaluate(node->args[0].get()));
        
        if (mysql_query(conn, sql.c_str())) {
            throw std::runtime_error("Query failed: " + std::string(mysql_error(conn)));
        }
        
        MYSQL_RES* result = mysql_store_result(conn);
        if (!result) {
            interp->lastValue = (int)mysql_affected_rows(conn);
            return std::to_string(mysql_affected_rows(conn));
        }
        
        auto rows = std::make_shared<ArrayValue>();
        int numFields = mysql_num_fields(result);
        MYSQL_FIELD* fields = mysql_fetch_fields(result);
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            auto rowObj = std::make_shared<ObjectValue>();
            for (int i = 0; i < numFields; i++) {
                std::string fieldName = fields[i].name;
                if (row[i]) rowObj->fields[fieldName] = std::string(row[i]);
                else rowObj->fields[fieldName] = std::string("");
            }
            rows->elements.push_back(rowObj);
        }
        
        mysql_free_result(result);
        interp->lastValue = rows;
        return "[array]";
    }
};
class MySQLCloseBuiltin : public BuiltinFunction {
public:
//@desc Close canvas window
    std::string getName() const override { return "close"; }
//@parent connection
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (!node->callee) throw std::runtime_error("conn.close()");
        
        auto fa = dynamic_cast<FieldAccess*>(node->callee.get());
        Value connVal = interp->evaluate(fa->object.get());
        auto connObj = std::get<std::shared_ptr<ObjectValue>>(connVal);
        int connId = std::get<int>(connObj->fields["_connId"]);
        
        if (connections.count(connId)) {
            mysql_close(connections[connId]);
            connections.erase(connId);
        }
        return "";
    }
};
REGISTER_BUILTIN(MySQLConnectBuiltin)
REGISTER_BUILTIN(MySQLQueryBuiltin)
REGISTER_BUILTIN(MySQLCloseBuiltin)
