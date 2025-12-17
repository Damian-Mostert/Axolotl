#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include <string>

class Compiler {
public:
    void compile(Program* ast, const std::string& outputFile, const std::string& sourceCode);
};

#endif // COMPILER_H
