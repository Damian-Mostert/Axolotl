#include "include/builtins.h"
#include <cmath>
#include <random>
#define MATH_UNARY(name, func)                                                                                        \
    class name##Builtin : public BuiltinFunction                                                                      \
    {                                                                                                                 \
    public:                                                                                                           \
        std::string getName() const override { return #name; }                                                        \
        std::string execute(Interpreter *interp, FunctionCall *node) override                                         \
        {                                                                                                             \
            if (node->args.size() != 1)                                                                               \
                throw std::runtime_error(#name "() expects 1 argument");                                              \
            Value v = interp->evaluate(node->args[0].get());                                                          \
            float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v)); \
            interp->lastValue = func(val);                                                                            \
            return "[float]";                                                                                         \
        }                                                                                                             \
    };                                                                                                                \
    REGISTER_BUILTIN(name##Builtin)
MATH_UNARY(sin, std::sin)
MATH_UNARY(cos, std::cos)
MATH_UNARY(tan, std::tan)
MATH_UNARY(sqrt, std::sqrt)
MATH_UNARY(abs, std::abs)
MATH_UNARY(floor, std::floor)
MATH_UNARY(ceil, std::ceil)
MATH_UNARY(round, std::round)
MATH_UNARY(log, std::log)
MATH_UNARY(log10, std::log10)
MATH_UNARY(exp, std::exp)
MATH_UNARY(asin, std::asin)
MATH_UNARY(acos, std::acos)
MATH_UNARY(atan, std::atan)
class PowBuiltin : public BuiltinFunction
{
public:
    //@desc Raise base to exponent power
    //@parent
    std::string getName() const override { return "pow"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("pow() expects 2 arguments");
        Value base = interp->evaluate(node->args[0].get());
        Value exp = interp->evaluate(node->args[1].get());
        float b = std::holds_alternative<float>(base) ? std::get<float>(base) : static_cast<float>(std::get<int>(base));
        float e = std::holds_alternative<float>(exp) ? std::get<float>(exp) : static_cast<float>(std::get<int>(exp));
        interp->lastValue = std::pow(b, e);
        return "[float]";
    }
};
class MinBuiltin : public BuiltinFunction
{
public:
    //@desc Return minimum of two numbers
    //@parent
    std::string getName() const override { return "min"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("min() expects 2 arguments");
        Value a = interp->evaluate(node->args[0].get());
        Value b = interp->evaluate(node->args[1].get());
        float af = std::holds_alternative<float>(a) ? std::get<float>(a) : static_cast<float>(std::get<int>(a));
        float bf = std::holds_alternative<float>(b) ? std::get<float>(b) : static_cast<float>(std::get<int>(b));
        interp->lastValue = std::min(af, bf);
        return "[float]";
    }
};
class MaxBuiltin : public BuiltinFunction
{
public:
    //@desc Return maximum of two numbers
    //@parent
    std::string getName() const override { return "max"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("max() expects 2 arguments");
        Value a = interp->evaluate(node->args[0].get());
        Value b = interp->evaluate(node->args[1].get());
        float af = std::holds_alternative<float>(a) ? std::get<float>(a) : static_cast<float>(std::get<int>(a));
        float bf = std::holds_alternative<float>(b) ? std::get<float>(b) : static_cast<float>(std::get<int>(b));
        interp->lastValue = std::max(af, bf);
        return "[float]";
    }
};
class ClampBuiltin : public BuiltinFunction
{
public:
    //@desc Clamp value between min and max
    //@parent
    std::string getName() const override { return "clamp"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 3)
            throw std::runtime_error("clamp() expects 3 arguments");
        Value v = interp->evaluate(node->args[0].get());
        Value minV = interp->evaluate(node->args[1].get());
        Value maxV = interp->evaluate(node->args[2].get());
        float val = std::holds_alternative<float>(v) ? std::get<float>(v) : static_cast<float>(std::get<int>(v));
        float minVal = std::holds_alternative<float>(minV) ? std::get<float>(minV) : static_cast<float>(std::get<int>(minV));
        float maxVal = std::holds_alternative<float>(maxV) ? std::get<float>(maxV) : static_cast<float>(std::get<int>(maxV));
        interp->lastValue = std::clamp(val, minVal, maxVal);
        return "[float]";
    }
};
class LerpBuiltin : public BuiltinFunction
{
public:
    //@desc Linear interpolation between a and b by t
    //@parent
    std::string getName() const override { return "lerp"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 3)
            throw std::runtime_error("lerp() expects 3 arguments");
        Value a = interp->evaluate(node->args[0].get());
        Value b = interp->evaluate(node->args[1].get());
        Value t = interp->evaluate(node->args[2].get());
        float af = std::holds_alternative<float>(a) ? std::get<float>(a) : static_cast<float>(std::get<int>(a));
        float bf = std::holds_alternative<float>(b) ? std::get<float>(b) : static_cast<float>(std::get<int>(b));
        float tf = std::holds_alternative<float>(t) ? std::get<float>(t) : static_cast<float>(std::get<int>(t));
        interp->lastValue = af + (bf - af) * tf;
        return "[float]";
    }
};
class Atan2Builtin : public BuiltinFunction
{
public:
    //@desc Calculate arc tangent of y/x in radians
    //@parent
    std::string getName() const override { return "atan2"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("atan2() expects 2 arguments");
        Value y = interp->evaluate(node->args[0].get());
        Value x = interp->evaluate(node->args[1].get());
        float yf = std::holds_alternative<float>(y) ? std::get<float>(y) : static_cast<float>(std::get<int>(y));
        float xf = std::holds_alternative<float>(x) ? std::get<float>(x) : static_cast<float>(std::get<int>(x));
        interp->lastValue = std::atan2(yf, xf);
        return "[float]";
    }
};
class RandomBuiltin : public BuiltinFunction
{
public:
    //@desc Generate random float between 0.0 and 1.0
    //@parent
    std::string getName() const override { return "random"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 0)
            throw std::runtime_error("random() expects no arguments");
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
REGISTER_BUILTIN(MinBuiltin)
REGISTER_BUILTIN(MaxBuiltin)
REGISTER_BUILTIN(ClampBuiltin)
REGISTER_BUILTIN(LerpBuiltin)
REGISTER_BUILTIN(Atan2Builtin)
REGISTER_BUILTIN(RandomBuiltin)
