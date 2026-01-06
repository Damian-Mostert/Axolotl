#include "include/builtins.h"

class JsonParseBuiltin : public BuiltinFunction {
public:
// @desc Parse JSON string into object
    std::string getName() const override { return "json_parse"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("json_parse() expects 1 argument");
        Value jsonVal = interp->evaluate(node->args[0].get());
        std::string jsonStr = interp->valueToString(jsonVal);
        if (jsonStr.size() > 0 && jsonStr[0] == '{') {
            auto jsonObj = std::make_shared<ObjectValue>();
            size_t pos = 1;
            while (pos < jsonStr.size() && jsonStr[pos] != '}') {
                while (pos < jsonStr.size() && (jsonStr[pos] == ' ' || jsonStr[pos] == '\n' || jsonStr[pos] == '\r' || jsonStr[pos] == '\t')) pos++;
                if (pos >= jsonStr.size() || jsonStr[pos] == '}') break;
                if (jsonStr[pos] != '"') break;
                pos++;
                size_t keyStart = pos;
                while (pos < jsonStr.size() && jsonStr[pos] != '"') pos++;
                std::string key = jsonStr.substr(keyStart, pos - keyStart);
                pos++;
                while (pos < jsonStr.size() && (jsonStr[pos] == ' ' || jsonStr[pos] == ':')) pos++;
                Value val;
                if (jsonStr[pos] == '"') {
                    pos++;
                    size_t valStart = pos;
                    while (pos < jsonStr.size() && jsonStr[pos] != '"') {
                        if (jsonStr[pos] == '\\') pos++;
                        pos++;
                    }
                    val = jsonStr.substr(valStart, pos - valStart);
                    pos++;
                } else if (jsonStr[pos] == 't' || jsonStr[pos] == 'f') {
                    val = (jsonStr[pos] == 't');
                    pos += (jsonStr[pos] == 't') ? 4 : 5;
                } else if (jsonStr[pos] == 'n') {
                    val = std::string("");
                    pos += 4;
                } else if ((jsonStr[pos] >= '0' && jsonStr[pos] <= '9') || jsonStr[pos] == '-') {
                    size_t numStart = pos;
                    bool isFloat = false;
                    while (pos < jsonStr.size() && ((jsonStr[pos] >= '0' && jsonStr[pos] <= '9') || jsonStr[pos] == '.' || jsonStr[pos] == '-' || jsonStr[pos] == 'e' || jsonStr[pos] == 'E')) {
                        if (jsonStr[pos] == '.') isFloat = true;
                        pos++;
                    }
                    std::string numStr = jsonStr.substr(numStart, pos - numStart);
                    val = isFloat ? Value(std::stof(numStr)) : Value(std::stoi(numStr));
                } else {
                    val = std::string("");
                }
                jsonObj->fields[key] = val;
                while (pos < jsonStr.size() && (jsonStr[pos] == ' ' || jsonStr[pos] == ',' || jsonStr[pos] == '\n' || jsonStr[pos] == '\r' || jsonStr[pos] == '\t')) pos++;
            }
            interp->lastValue = jsonObj;
            return "{object}";
        }
        throw std::runtime_error("json_parse() requires valid JSON string");
    }
};

class JsonStringifyBuiltin : public BuiltinFunction {
public:
// @desc Convert object to JSON string
    std::string getName() const override { return "json_stringify"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("json_stringify() expects 1 argument");
        Value objVal = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal)) throw std::runtime_error("json_stringify() requires object");
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        std::string json = "{";
        bool first = true;
        for (const auto& [key, val] : obj->fields) {
            if (!first) json += ",";
            json += "\"" + key + "\":";
            if (std::holds_alternative<std::string>(val)) {
                json += "\"" + std::get<std::string>(val) + "\"";
            } else if (std::holds_alternative<bool>(val)) {
                json += std::get<bool>(val) ? "true" : "false";
            } else {
                json += interp->valueToString(val);
            }
            first = false;
        }
        json += "}";
        interp->lastValue = json;
        return "[string]";
    }
};

REGISTER_BUILTIN(JsonParseBuiltin)
REGISTER_BUILTIN(JsonStringifyBuiltin)
