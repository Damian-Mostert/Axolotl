#include "include/builtins.h"

// toUpper builtin
class ToUpperBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "toUpper"; }
    
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) {
            throw std::runtime_error("toUpper() expects 1 argument");
        }
        Value s = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<std::string>(s)) {
            throw std::runtime_error("toUpper() requires string");
        }
        std::string str = std::get<std::string>(s);
        for (char &c : str) {
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
        }
        return str;
    }
};

// toLower builtin
class ToLowerBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "toLower"; }
    
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) {
            throw std::runtime_error("toLower() expects 1 argument");
        }
        Value s = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<std::string>(s)) {
            throw std::runtime_error("toLower() requires string");
        }
        std::string str = std::get<std::string>(s);
        for (char &c : str) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        return str;
    }
};

class SubstrBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "substr"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 3) throw std::runtime_error("substr() expects 3 arguments");
        Value s = interp->evaluate(node->args[0].get());
        Value start = interp->evaluate(node->args[1].get());
        Value len = interp->evaluate(node->args[2].get());
        if (!std::holds_alternative<std::string>(s) || !std::holds_alternative<int>(start) || !std::holds_alternative<int>(len))
            throw std::runtime_error("substr() requires (string, int, int)");
        std::string str = std::get<std::string>(s);
        int st = std::get<int>(start);
        int l = std::get<int>(len);
        if (st < 0 || st >= (int)str.size()) return "";
        return str.substr(st, l);
    }
};

class IndexOfBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "indexOf"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("indexOf() expects 2 arguments");
        Value s = interp->evaluate(node->args[0].get());
        Value sub = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(s) || !std::holds_alternative<std::string>(sub))
            throw std::runtime_error("indexOf() requires (string, string)");
        std::string str = std::get<std::string>(s);
        std::string substring = std::get<std::string>(sub);
        size_t pos = str.find(substring);
        return pos != std::string::npos ? std::to_string(pos) : "-1";
    }
};

class ContainsBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "contains"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("contains() expects 2 arguments");
        Value s = interp->evaluate(node->args[0].get());
        Value sub = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(s) || !std::holds_alternative<std::string>(sub))
            throw std::runtime_error("contains() requires (string, string)");
        std::string str = std::get<std::string>(s);
        std::string substring = std::get<std::string>(sub);
        interp->lastValue = str.find(substring) != std::string::npos;
        return "[bool]";
    }
};

class TrimBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "trim"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("trim() expects 1 argument");
        Value v = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<std::string>(v)) throw std::runtime_error("trim() requires string");
        std::string str = std::get<std::string>(v);
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            interp->lastValue = std::string("");
            return "[string]";
        }
        size_t end = str.find_last_not_of(" \t\n\r");
        interp->lastValue = str.substr(start, end - start + 1);
        return "[string]";
    }
};

class ReplaceBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "replace"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 3) throw std::runtime_error("replace() expects 3 arguments");
        Value strVal = interp->evaluate(node->args[0].get());
        Value searchVal = interp->evaluate(node->args[1].get());
        Value replaceVal = interp->evaluate(node->args[2].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("replace() requires string");
        std::string str = std::get<std::string>(strVal);
        std::string search = std::get<std::string>(searchVal);
        std::string replacement = std::get<std::string>(replaceVal);
        size_t pos = str.find(search);
        if (pos != std::string::npos) str.replace(pos, search.length(), replacement);
        interp->lastValue = str;
        return "[string]";
    }
};

class SplitBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "split"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("split() expects 2 arguments");
        Value strVal = interp->evaluate(node->args[0].get());
        Value delimVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("split() requires string");
        std::string str = std::get<std::string>(strVal);
        std::string delim = std::get<std::string>(delimVal);
        auto result = std::make_shared<ArrayValue>();
        size_t start = 0, end = str.find(delim);
        while (end != std::string::npos) {
            result->elements.push_back(str.substr(start, end - start));
            start = end + delim.length();
            end = str.find(delim, start);
        }
        result->elements.push_back(str.substr(start));
        interp->lastValue = result;
        return "[array]";
    }
};

class StartsWithBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "startsWith"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("startsWith() expects 2 arguments");
        Value strVal = interp->evaluate(node->args[0].get());
        Value prefixVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("startsWith() requires string");
        std::string str = std::get<std::string>(strVal);
        std::string prefix = std::get<std::string>(prefixVal);
        interp->lastValue = str.rfind(prefix, 0) == 0;
        return "[bool]";
    }
};

class EndsWithBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "endsWith"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("endsWith() expects 2 arguments");
        Value strVal = interp->evaluate(node->args[0].get());
        Value suffixVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("endsWith() requires string");
        std::string str = std::get<std::string>(strVal);
        std::string suffix = std::get<std::string>(suffixVal);
        if (suffix.length() > str.length()) {
            interp->lastValue = false;
        } else {
            interp->lastValue = str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
        }
        return "[bool]";
    }
};

class RepeatBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "repeat"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("repeat() expects 2 arguments");
        Value strVal = interp->evaluate(node->args[0].get());
        Value countVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("repeat() requires string");
        std::string str = std::get<std::string>(strVal);
        int count = std::get<int>(countVal);
        std::string result;
        for (int i = 0; i < count; i++) result += str;
        interp->lastValue = result;
        return "[string]";
    }
};

class CharAtBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "charAt"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("charAt() expects 2 arguments");
        Value strVal = interp->evaluate(node->args[0].get());
        Value idxVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("charAt() requires string");
        std::string str = std::get<std::string>(strVal);
        int idx = std::get<int>(idxVal);
        if (idx < 0 || idx >= (int)str.length()) {
            interp->lastValue = std::string("");
        } else {
            interp->lastValue = std::string(1, str[idx]);
        }
        return "[string]";
    }
};

class CharCodeAtBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "charCodeAt"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("charCodeAt() expects 2 arguments");
        Value strVal = interp->evaluate(node->args[0].get());
        Value idxVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::string>(strVal)) throw std::runtime_error("charCodeAt() requires string");
        std::string str = std::get<std::string>(strVal);
        int idx = std::get<int>(idxVal);
        if (idx < 0 || idx >= (int)str.length()) {
            interp->lastValue = -1;
        } else {
            interp->lastValue = static_cast<int>(str[idx]);
        }
        return "[int]";
    }
};

REGISTER_BUILTIN(ToUpperBuiltin)
REGISTER_BUILTIN(ToLowerBuiltin)
REGISTER_BUILTIN(SubstrBuiltin)
REGISTER_BUILTIN(IndexOfBuiltin)
REGISTER_BUILTIN(ContainsBuiltin)
REGISTER_BUILTIN(TrimBuiltin)
REGISTER_BUILTIN(ReplaceBuiltin)
REGISTER_BUILTIN(SplitBuiltin)
REGISTER_BUILTIN(StartsWithBuiltin)
REGISTER_BUILTIN(EndsWithBuiltin)
REGISTER_BUILTIN(RepeatBuiltin)
REGISTER_BUILTIN(CharAtBuiltin)
REGISTER_BUILTIN(CharCodeAtBuiltin)
