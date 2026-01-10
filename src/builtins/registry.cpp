#include "include/builtins.h"
#include <algorithm>
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
std::vector<std::string> BuiltinRegistry::getAllNames() const {
    std::vector<std::string> names;
    names.reserve(builtins.size());
    for (const auto& pair : builtins) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}
