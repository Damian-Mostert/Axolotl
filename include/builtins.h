#ifndef BUILTINS_H
#define BUILTINS_H

#include "interpreter.h"
#include "ast.h"
#include <string>

// Base class for built-in functions
class BuiltinFunction {
public:
    virtual ~BuiltinFunction() = default;
    virtual std::string getName() const = 0;
    virtual std::string execute(Interpreter* interp, FunctionCall* node) = 0;
};

// Registry for built-in functions
class BuiltinRegistry {
public:
    static BuiltinRegistry& instance();
    void registerBuiltin(BuiltinFunction* func);
    BuiltinFunction* getBuiltin(const std::string& name);
    
private:
    BuiltinRegistry() = default;
    std::unordered_map<std::string, BuiltinFunction*> builtins;
};

// Macro to register a builtin
#define REGISTER_BUILTIN(ClassName) \
    static ClassName _builtin_##ClassName##_instance; \
    static struct ClassName##Registrar { \
        ClassName##Registrar() { \
            BuiltinRegistry::instance().registerBuiltin(&_builtin_##ClassName##_instance); \
        } \
    } _builtin_##ClassName##_registrar;

#endif // BUILTINS_H
