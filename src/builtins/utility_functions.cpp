#include "include/builtins.h"
#include <chrono>
#include <thread>
class MillisBuiltin : public BuiltinFunction
{
public:
    //@desc Get current time in milliseconds since epoch
    std::string getName() const override { return "millis"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 0)
            throw std::runtime_error("millis() expects no arguments");
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        interp->lastValue = static_cast<int>(millis);
        return "[int]";
    }
};
class SleepBuiltin : public BuiltinFunction
{
public:
    //@desc Sleep for specified milliseconds
    std::string getName() const override { return "sleep"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("sleep() expects 1 argument");
        Value ms = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<int>(ms))
            throw std::runtime_error("sleep() requires int argument");
        int milliseconds = std::get<int>(ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        return "";
    }
};
class AssertBuiltin : public BuiltinFunction
{
public:
    //@desc Throw error if condition is false
    std::string getName() const override { return "assert"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("assert() expects 2 arguments");
        Value condVal = interp->evaluate(node->args[0].get());
        Value msgVal = interp->evaluate(node->args[1].get());
        if (!interp->isTruthy(condVal))
        {
            throw std::runtime_error("Assertion failed: " + std::get<std::string>(msgVal));
        }
        return "";
    }
};
class ErrorBuiltin : public BuiltinFunction
{
    //@desc Throw runtime error with message
public:
    std::string getName() const override { return "error"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("error() expects 1 argument");
        Value msgVal = interp->evaluate(node->args[0].get());
        throw std::runtime_error(std::get<std::string>(msgVal));
    }
};
class KeysBuiltin : public BuiltinFunction
{
public:
    //@desc Get array of object keys
    std::string getName() const override { return "keys"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("keys() expects 1 argument");
        Value objVal = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal))
            throw std::runtime_error("keys() requires object");
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        auto result = std::make_shared<ArrayValue>();
        for (const auto &[key, val] : obj->fields)
        {
            result->elements.push_back(key);
        }
        interp->lastValue = result;
        return "[array]";
    }
};
class ValuesBuiltin : public BuiltinFunction
{
public:
    //@desc Get array of object values
    std::string getName() const override { return "values"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("values() expects 1 argument");
        Value objVal = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal))
            throw std::runtime_error("values() requires object");
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        auto result = std::make_shared<ArrayValue>();
        for (const auto &[key, val] : obj->fields)
        {
            result->elements.push_back(val);
        }
        interp->lastValue = result;
        return "[array]";
    }
};
class HasKeyBuiltin : public BuiltinFunction
{
public:
    //@desc Check if object has key
    std::string getName() const override { return "hasKey"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("hasKey() expects 2 arguments");
        Value objVal = interp->evaluate(node->args[0].get());
        Value keyVal = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal))
            throw std::runtime_error("hasKey() requires object");
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        std::string key = std::get<std::string>(keyVal);
        interp->lastValue = obj->fields.find(key) != obj->fields.end();
        return "[bool]";
    }
};
class CloneBuiltin : public BuiltinFunction
{
public:
    //@desc Create shallow copy of array or object
    std::string getName() const override { return "clone"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("clone() expects 1 argument");
        Value v = interp->evaluate(node->args[0].get());
        if (std::holds_alternative<std::shared_ptr<ArrayValue>>(v))
        {
            auto arr = std::get<std::shared_ptr<ArrayValue>>(v);
            auto newArr = std::make_shared<ArrayValue>();
            newArr->elements = arr->elements;
            interp->lastValue = newArr;
            return "[array]";
        }
        if (std::holds_alternative<std::shared_ptr<ObjectValue>>(v))
        {
            auto obj = std::get<std::shared_ptr<ObjectValue>>(v);
            auto newObj = std::make_shared<ObjectValue>();
            newObj->fields = obj->fields;
            interp->lastValue = newObj;
            return "{object}";
        }
        interp->lastValue = v;
        return interp->valueToString(v);
    }
};
class MergeBuiltin : public BuiltinFunction
{
public:
    //@desc Merge two objects, second overrides first
    std::string getName() const override { return "merge"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("merge() expects 2 arguments");
        Value obj1Val = interp->evaluate(node->args[0].get());
        Value obj2Val = interp->evaluate(node->args[1].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(obj1Val) ||
            !std::holds_alternative<std::shared_ptr<ObjectValue>>(obj2Val))
        {
            throw std::runtime_error("merge() requires two objects");
        }
        auto obj1 = std::get<std::shared_ptr<ObjectValue>>(obj1Val);
        auto obj2 = std::get<std::shared_ptr<ObjectValue>>(obj2Val);
        auto result = std::make_shared<ObjectValue>();
        result->fields = obj1->fields;
        for (const auto &[key, val] : obj2->fields)
        {
            result->fields[key] = val;
        }
        interp->lastValue = result;
        return "{object}";
    }
};
class TypeofBuiltin : public BuiltinFunction
{
public:
    //@desc Get type of value as string
    std::string getName() const override { return "typeof"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("typeof() expects 1 argument");
        Value v = interp->evaluate(node->args[0].get());
        std::string type;
        if (std::holds_alternative<int>(v))
            type = "int";
        else if (std::holds_alternative<float>(v))
            type = "float";
        else if (std::holds_alternative<std::string>(v))
            type = "string";
        else if (std::holds_alternative<bool>(v))
            type = "bool";
        else if (std::holds_alternative<std::shared_ptr<ArrayValue>>(v))
            type = "array";
        else if (std::holds_alternative<std::shared_ptr<ObjectValue>>(v))
            type = "object";
        else
            type = "unknown";
        interp->lastValue = type;
        return "[string]";
    }
};
REGISTER_BUILTIN(MillisBuiltin)
REGISTER_BUILTIN(SleepBuiltin)
REGISTER_BUILTIN(AssertBuiltin)
REGISTER_BUILTIN(ErrorBuiltin)
REGISTER_BUILTIN(KeysBuiltin)
REGISTER_BUILTIN(ValuesBuiltin)
REGISTER_BUILTIN(HasKeyBuiltin)
REGISTER_BUILTIN(CloneBuiltin)
REGISTER_BUILTIN(MergeBuiltin)
REGISTER_BUILTIN(TypeofBuiltin)
