#include "../include/builtins.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: extract_parent_types <output_file>" << std::endl;
        return 1;
    }
    
    std::ofstream out(argv[1]);
    auto& registry = BuiltinRegistry::instance();
    auto names = registry.getAllNames();
    
    out << "{" << std::endl;
    bool first = true;
    for (const auto& name : names) {
        auto* builtin = registry.getBuiltin(name);
        std::string parent = builtin->getParent();
        if (!parent.empty()) {
            if (!first) out << "," << std::endl;
            first = false;
            out << "  \"" << name << "\": \"" << parent << "\"";
        }
    }
    out << std::endl << "}" << std::endl;
    
    return 0;
}
