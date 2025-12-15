#include "include/builtins.h"

class ToStringBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "toString"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("toString() expects 1 argument");
        Value v = interp->evaluate(node->args[0].get());
        std::string result = interp->valueToString(v);
        interp->lastValue = Value(result);
        return "[string]";
    }
};

class ToIntBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "toInt"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("toInt() expects 1 argument");
        Value v = interp->evaluate(node->args[0].get());
        if (std::holds_alternative<int>(v)) {
            interp->lastValue = std::get<int>(v);
        } else if (std::holds_alternative<float>(v)) {
            interp->lastValue = static_cast<int>(std::get<float>(v));
        } else if (std::holds_alternative<bool>(v)) {
            interp->lastValue = std::get<bool>(v) ? 1 : 0;
        } else if (std::holds_alternative<std::string>(v)) {
            try {
                interp->lastValue = std::stoi(std::get<std::string>(v));
            } catch (...) {
                interp->lastValue = 0;
            }
        } else {
            interp->lastValue = 0;
        }
        return "[int]";
    }
};

class ToFloatBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "toFloat"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("toFloat() expects 1 argument");
        Value v = interp->evaluate(node->args[0].get());
        if (std::holds_alternative<float>(v)) {
            interp->lastValue = std::get<float>(v);
        } else if (std::holds_alternative<int>(v)) {
            interp->lastValue = static_cast<float>(std::get<int>(v));
        } else if (std::holds_alternative<std::string>(v)) {
            try {
                interp->lastValue = std::stof(std::get<std::string>(v));
            } catch (...) {
                interp->lastValue = 0.0f;
            }
        } else {
            interp->lastValue = 0.0f;
        }
        return "[float]";
    }
};

class ToBoolBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "toBool"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 1) throw std::runtime_error("toBool() expects 1 argument");
        Value v = interp->evaluate(node->args[0].get());
        interp->lastValue = interp->isTruthy(v);
        return "[bool]";
    }
};

REGISTER_BUILTIN(ToStringBuiltin)
REGISTER_BUILTIN(ToIntBuiltin)
REGISTER_BUILTIN(ToFloatBuiltin)
REGISTER_BUILTIN(ToBoolBuiltin)
