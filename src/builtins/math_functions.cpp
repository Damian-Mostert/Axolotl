#include "include/builtins.h"
#include <cmath>
#include <random>

#define MATH_UNARY(name, func) \
class name##Builtin : public BuiltinFunction { \
public: \
    std::string getName() const override { return #name; } \
    std::string execute(Interpreter* interp, FunctionCall* node) override { \
        if (node->args.size() != 1) throw std::runtime_error(#name "() expects 1 argument"); \
        Value v = interp->evaluate(node->args[0].get()); \
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v)); \
        interp->lastValue = func(val); \
        return "[float]"; \
    } \
}; \
REGISTER_BUILTIN(name##Builtin)

MATH_UNARY(sin, std::sin)
MATH_UNARY(cos, std::cos)
MATH_UNARY(tan, std::tan)
MATH_UNARY(sqrt, std::sqrt)
MATH_UNARY(log, std::log)
MATH_UNARY(log10, std::log10)
MATH_UNARY(exp, std::exp)
MATH_UNARY(asin, std::asin)
MATH_UNARY(acos, std::acos)
MATH_UNARY(atan, std::atan)

class PowBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "pow"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("pow() expects 2 arguments");
        Value base = interp->evaluate(node->args[0].get());
        Value exp = interp->evaluate(node->args[1].get());
        float b = std::holds_alternative<float>(base) ? std::get<float>(base) : static_cast<float>(std::get<int>(base));
        float e = std::holds_alternative<float>(exp) ? std::get<float>(exp) : static_cast<float>(std::get<int>(exp));
        interp->lastValue = std::pow(b, e);
        return "[float]";
    }
};

class Atan2Builtin : public BuiltinFunction {
public:
    std::string getName() const override { return "atan2"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 2) throw std::runtime_error("atan2() expects 2 arguments");
        Value y = interp->evaluate(node->args[0].get());
        Value x = interp->evaluate(node->args[1].get());
        float yf = std::holds_alternative<float>(y) ? std::get<float>(y) : static_cast<float>(std::get<int>(y));
        float xf = std::holds_alternative<float>(x) ? std::get<float>(x) : static_cast<float>(std::get<int>(x));
        interp->lastValue = std::atan2(yf, xf);
        return "[float]";
    }
};

class RandomBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "random"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 0) throw std::runtime_error("random() expects no arguments");
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
        float randomValue = dis(gen);
        interp->lastValue = randomValue;
        std::ostringstream oss;
        oss << randomValue;
        return oss.str();
    }
};

REGISTER_BUILTIN(PowBuiltin)
REGISTER_BUILTIN(Atan2Builtin)
REGISTER_BUILTIN(RandomBuiltin)
