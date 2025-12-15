#include "include/builtins.h"
#include <algorithm>

class LenBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "len"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) {
            throw std::runtime_error("len() expects 1 argument");
        }
        Value v = interp->evaluate(node->args[0].get());
        if (std::holds_alternative<std::shared_ptr<ArrayValue>>(v)) {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
            return std::to_string(arr->elements.size());
        }
        if (std::holds_alternative<std::string>(v)) {
            auto s = std::get<std::string>(v);
            return std::to_string(s.size());
        }
        throw std::runtime_error("len() requires array or string");
    }
};

class SliceBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "slice"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 3) throw std::runtime_error("slice() expects 3 arguments");
        Value arrVal = interp->evaluate(node->args[0].get());
        Value startVal = interp->evaluate(node->args[1].get());
        Value endVal = interp->evaluate(node->args[2].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("slice() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        int start = std::get<int>(startVal);
        int end = std::get<int>(endVal);
        auto result = std::make_shared<ArrayValue>();
        for (int i = start; i < end && i < (int)arr->elements.size(); i++) {
            result->elements.push_back(arr->elements[i]);
        }
        interp->lastValue = result;
        return "[array]";
    }
};

class ReverseBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "reverse"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("reverse() expects 1 argument");
        Value arrVal = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("reverse() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        auto result = std::make_shared<ArrayValue>();
        for (auto it = arr->elements.rbegin(); it != arr->elements.rend(); ++it) {
            result->elements.push_back(*it);
        }
        interp->lastValue = result;
        return "[array]";
    }
};

class JoinBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "join"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("join() expects 2 arguments");
        Value arrVal = interp->evaluate(node->args[0].get());
        Value sepVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("join() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        std::string sep = std::get<std::string>(sepVal);
        std::string result;
        for (size_t i = 0; i < arr->elements.size(); i++) {
            if (i > 0) result += sep;
            result += interp->valueToString(arr->elements[i]);
        }
        interp->lastValue = result;
        return "[string]";
    }
};

class FindBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "find"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("find() expects 2 arguments");
        Value arrVal = interp->evaluate(node->args[0].get());
        Value searchVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("find() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        for (size_t i = 0; i < arr->elements.size(); i++) {
            if (interp->valueToString(arr->elements[i]) == interp->valueToString(searchVal)) {
                interp->lastValue = static_cast<int>(i);
                return "[int]";
            }
        }
        interp->lastValue = -1;
        return "[int]";
    }
};

class IncludesBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "includes"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("includes() expects 2 arguments");
        Value arrVal = interp->evaluate(node->args[0].get());
        Value searchVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("includes() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        for (const auto& elem : arr->elements) {
            if (interp->valueToString(elem) == interp->valueToString(searchVal)) {
                interp->lastValue = true;
                return "[bool]";
            }
        }
        interp->lastValue = false;
        return "[bool]";
    }
};

class PushBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "push"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("push() expects 2 arguments");
        Identifier *arrayId = dynamic_cast<Identifier *>(node->args[0].get());
        if (!arrayId) throw std::runtime_error("push() requires array variable as first argument");
        Variable arrayVar = interp->environment.get(arrayId->name);
        Value arrVal = arrayVar.value;
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("push() requires array variable as first argument");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        Value val = interp->evaluate(node->args[1].get());
        arr->elements.push_back(val);
        return "";
    }
};

class PopBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "pop"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("pop() expects 1 argument");
        Identifier *arrayId = dynamic_cast<Identifier *>(node->args[0].get());
        if (!arrayId) throw std::runtime_error("pop() requires array variable");
        Variable arrayVar = interp->environment.get(arrayId->name);
        Value arrVal = arrayVar.value;
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrVal)) throw std::runtime_error("pop() requires array variable");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrVal);
        if (!arr->elements.empty()) {
            Value last = arr->elements.back();
            arr->elements.pop_back();
            return interp->valueToString(last);
        }
        return "";
    }
};

class SortBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "sort"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("sort() expects 1 argument");
        Identifier *arrayId = dynamic_cast<Identifier *>(node->args[0].get());
        if (!arrayId) throw std::runtime_error("sort() requires array variable");
        Variable arrayVar = interp->environment.get(arrayId->name);
        if (!std::holds_alternative<std::shared_ptr<ArrayValue>>(arrayVar.value)) throw std::runtime_error("sort() requires array");
        auto arr = std::get<std::shared_ptr<ArrayValue>>(arrayVar.value);
        std::sort(arr->elements.begin(), arr->elements.end(), [interp](const Value& a, const Value& b) {
            return interp->valueToString(a) < interp->valueToString(b);
        });
        interp->lastValue = arr;
        return "[array]";
    }
};

REGISTER_BUILTIN(LenBuiltin)
REGISTER_BUILTIN(SliceBuiltin)
REGISTER_BUILTIN(ReverseBuiltin)
REGISTER_BUILTIN(JoinBuiltin)
REGISTER_BUILTIN(FindBuiltin)
REGISTER_BUILTIN(IncludesBuiltin)
REGISTER_BUILTIN(PushBuiltin)
REGISTER_BUILTIN(PopBuiltin)
REGISTER_BUILTIN(SortBuiltin)
