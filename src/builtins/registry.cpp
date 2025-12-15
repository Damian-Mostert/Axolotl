#include "include/builtins.h"

BuiltinRegistry& BuiltinRegistry::instance() {
    static BuiltinRegistry registry;
    return registry;
}

void BuiltinRegistry::registerBuiltin(BuiltinFunction* func) {
    builtins[func->getName()] = func;
}

BuiltinFunction* BuiltinRegistry::getBuiltin(const std::string& name) {
    auto it = builtins.find(name);
    return (it != builtins.end()) ? it->second : nullptr;
}
