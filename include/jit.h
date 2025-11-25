#ifndef JIT_H
#define JIT_H

#include "interpreter.h"
#include "ast.h"
#include <memory>
#include <functional>

// JIT Compiler: compiles simple loops to native code using LLVM
class LLVMJITCompiler {
public:
    LLVMJITCompiler();
    ~LLVMJITCompiler();
    
    // Check if a while loop is JIT-compilable
    bool isCompilable(WhileStatement *stmt);
    
    // Compile and execute a while loop, returns false if not compilable
    // If compilable, executes the loop and returns true
    bool compileAndExecute(WhileStatement *stmt, Interpreter *interp, Environment &env);
    
    // Check if a for loop is JIT-compilable
    bool isCompilable(ForStatement *stmt);
    
    // Compile and execute a for loop, returns false if not compilable
    bool compileAndExecute(ForStatement *stmt, Interpreter *interp, Environment &env);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

#endif // JIT_H
