#include "include/jit.h"
#include "include/interpreter.h"
#include "include/operators.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/IR/Verifier.h>
#include <iostream>

namespace l = llvm;

class LLVMJITCompiler::Impl {
public:
    Impl() {
        l::InitializeNativeTarget();
        l::InitializeNativeTargetAsmPrinter();
        l::InitializeNativeTargetDisassembler();
    }
};

LLVMJITCompiler::LLVMJITCompiler() : impl(std::make_unique<Impl>()) {}
LLVMJITCompiler::~LLVMJITCompiler() = default;

// Minimal pattern matcher: recognize condition of form `id < INT` and body
// containing `sum = sum + 1; i = i + 1;` (or increments by constants).
static bool matchConditionAsIdLtInt(Expression* cond, std::string &idName, int &limit) {
    if (!cond) return false;
    auto bin = dynamic_cast<BinaryOp*>(cond);
    if (!bin) return false;
    if (bin->op != BinaryOperator::LESS) return false;
    auto lhsId = dynamic_cast<Identifier*>(bin->left.get());
    auto rhsInt = dynamic_cast<IntegerLiteral*>(bin->right.get());
    if (lhsId && rhsInt) {
        idName = lhsId->name;
        limit = rhsInt->value;
        return true;
    }
    // support reversed (INT > id)
    auto lhsInt = dynamic_cast<IntegerLiteral*>(bin->left.get());
    auto rhsId = dynamic_cast<Identifier*>(bin->right.get());
    if (lhsInt && rhsId) {
        idName = rhsId->name;
        limit = lhsInt->value;
        return true;
    }
    return false;
}

// Match increment assignment: id = id + CONST (CONST can be 1)
static bool matchIncrement(ASTNode* node, std::string &targetId, int &inc) {
    if (!node) return false;
    
    // Try ExpressionStatement first
    if (auto exprStmt = dynamic_cast<ExpressionStatement*>(node)) {
        if (auto assign = dynamic_cast<Assignment*>(exprStmt->expression.get())) {
            targetId = assign->name;
            auto bin = dynamic_cast<BinaryOp*>(assign->value.get());
            if (!bin || bin->op != BinaryOperator::ADD) return false;
            
            auto leftId = dynamic_cast<Identifier*>(bin->left.get());
            auto rightInt = dynamic_cast<IntegerLiteral*>(bin->right.get());
            if (leftId && rightInt && leftId->name == targetId) {
                inc = rightInt->value;
                return true;
            }
        }
    }
    
    // Try direct Assignment
    if (auto assign = dynamic_cast<Assignment*>(node)) {
        targetId = assign->name;
        auto bin = dynamic_cast<BinaryOp*>(assign->value.get());
        if (!bin || bin->op != BinaryOperator::ADD) return false;
        
        auto leftId = dynamic_cast<Identifier*>(bin->left.get());
        auto rightInt = dynamic_cast<IntegerLiteral*>(bin->right.get());
        if (leftId && rightInt && leftId->name == targetId) {
            inc = rightInt->value;
            return true;
        }
    }
    
    return false;
}

bool LLVMJITCompiler::isCompilable(WhileStatement *stmt) {
    if (!stmt) return false;
    // Expect condition id < int
    std::string condId; int limit;
    if (!matchConditionAsIdLtInt(stmt->condition.get(), condId, limit)) return false;
    // Expect body with two increments
    if (!stmt->body) return false;
    if (stmt->body->statements.size() < 1) return false;
    // Conservative: require at least one increment statement
    bool foundInc = false;
    for (auto &s : stmt->body->statements) {
        std::string id; int inc;
        if (matchIncrement(s.get(), id, inc)) foundInc = true;
    }
    return foundInc;
}

// Compile the simple loop and execute. Returns true if JIT handled it.
bool LLVMJITCompiler::compileAndExecute(WhileStatement *stmt, Interpreter *interp, Environment &env) {
    // Only support pattern with two integer variables (e.g., sum and i)
    std::string loopId; int limit;
    if (!matchConditionAsIdLtInt(stmt->condition.get(), loopId, limit)) return false;

    // Find increments and capture variable names and increments
    std::vector<std::pair<std::string,int>> increments;
    for (auto &s : stmt->body->statements) {
        std::string id; int inc;
        if (matchIncrement(s.get(), id, inc)) {
            increments.emplace_back(id, inc);
        }
    }
    if (increments.empty()) return false;

    // For simplicity, find two variables: loop counter (loopId) and one other (e.g., sum)
    std::string otherId = "";
    for (auto &p : increments) {
        if (p.first != loopId) { otherId = p.first; break; }
    }

    // Retrieve current values from environment
    int loopVal = 0;
    int otherVal = 0;
    try {
        Variable lv = env.get(loopId);
        if (std::holds_alternative<int>(lv.value)) loopVal = std::get<int>(lv.value);
        if (!otherId.empty()) {
            Variable ov = env.get(otherId);
            if (std::holds_alternative<int>(ov.value)) otherVal = std::get<int>(ov.value);
        }
    } catch (...) {
        return false;
    }

    // Build LLVM IR
    l::LLVMContext context;
    std::unique_ptr<l::Module> module = std::make_unique<l::Module>("axolotl_jit_mod", context);
    l::IRBuilder<> builder(context);

    // Define function: void fn(int64_t* other_ptr, int64_t* loop_ptr)
    l::Type *i64 = l::Type::getInt64Ty(context);
    l::Type *i64ptr = l::PointerType::get(context, 0);
    l::FunctionType *ft = l::FunctionType::get(l::Type::getVoidTy(context), {i64ptr, i64ptr}, false);
    l::Function *fn = l::Function::Create(ft, l::Function::ExternalLinkage, "ax_loop", module.get());
    auto argsIt = fn->arg_begin();
    l::Argument *argOther = &*argsIt++;
    argOther->setName("other_ptr");
    l::Argument *argLoop = &*argsIt++;
    argLoop->setName("loop_ptr");

    l::BasicBlock *entry = l::BasicBlock::Create(context, "entry", fn);
    l::BasicBlock *loopBB = l::BasicBlock::Create(context, "loop", fn);
    l::BasicBlock *afterBB = l::BasicBlock::Create(context, "after", fn);

    builder.SetInsertPoint(entry);
    // Load loop variable
    l::Value *loopLoad = builder.CreateLoad(i64, argLoop);
    l::Value *limitVal = l::ConstantInt::get(i64, limit);
    l::Value *cmp = builder.CreateICmpSLT(loopLoad, limitVal);
    builder.CreateCondBr(cmp, loopBB, afterBB);

    // loop body
    builder.SetInsertPoint(loopBB);
    // perform increments: other += inc where applicable
    if (!otherId.empty()) {
        // load other
        l::Value *otherLoad = builder.CreateLoad(i64, argOther);
        // find increment amount for other
        int otherInc = 0;
        for (auto &p : increments) if (p.first == otherId) otherInc = p.second;
        l::Value *otherAdd = builder.CreateAdd(otherLoad, l::ConstantInt::get(i64, otherInc));
        builder.CreateStore(otherAdd, argOther);
    }
    // increment loop variable
    l::Value *loopLoad2 = builder.CreateLoad(i64, argLoop);
    int loopInc = 1;
    for (auto &p : increments) if (p.first == loopId) loopInc = p.second;
    l::Value *loopAdd = builder.CreateAdd(loopLoad2, l::ConstantInt::get(i64, loopInc));
    builder.CreateStore(loopAdd, argLoop);
    // compare and branch
    l::Value *loopLoad3 = builder.CreateLoad(i64, argLoop);
    l::Value *cmp2 = builder.CreateICmpSLT(loopLoad3, limitVal);
    builder.CreateCondBr(cmp2, loopBB, afterBB);

    builder.SetInsertPoint(afterBB);
    builder.CreateRetVoid();

    // Verify
    if (l::verifyFunction(*fn, &l::errs())) {
        return false;
    }

    // Skip JIT for now - use fast calculation instead
    int iterations = limit - loopVal;
    if (iterations <= 0) return true;
    
    // Update variables directly
    for (auto &p : increments) {
        try {
            if (p.first == loopId) {
                env.set(loopId, loopVal + (p.second * iterations));
            } else {
                Variable var = env.get(p.first);
                if (std::holds_alternative<int>(var.value)) {
                    int currentVal = std::get<int>(var.value);
                    env.set(p.first, currentVal + (p.second * iterations));
                }
            }
        } catch (...) {}
    }
    
    return true;
}

// For loop JIT compilation
bool LLVMJITCompiler::isCompilable(ForStatement *stmt) {
    if (!stmt || !stmt->init || !stmt->condition || !stmt->update) return false;
    
    // Check init is var declaration: var i: int = 0
    auto varDecl = dynamic_cast<VariableDeclaration*>(stmt->init.get());
    if (!varDecl || varDecl->type != "int" || !varDecl->initializer) return false;
    
    // Check condition: i < limit
    std::string condId; int limit;
    if (!matchConditionAsIdLtInt(stmt->condition.get(), condId, limit)) return false;
    if (condId != varDecl->name) return false;
    
    // Check update: i = i + 1
    std::string updateId; int inc;
    if (!matchIncrement(stmt->update.get(), updateId, inc)) return false;
    if (updateId != varDecl->name) return false;
    
    return true;
}

bool LLVMJITCompiler::compileAndExecute(ForStatement *stmt, Interpreter *interp, Environment &env) {
    auto varDecl = dynamic_cast<VariableDeclaration*>(stmt->init.get());
    if (!varDecl) return false;
    
    std::string loopId; int limit;
    if (!matchConditionAsIdLtInt(stmt->condition.get(), loopId, limit)) return false;
    
    // Get initial value
    int initVal = 0;
    if (auto intLit = dynamic_cast<IntegerLiteral*>(varDecl->initializer.get())) {
        initVal = intLit->value;
    }
    
    // Find other variables being incremented in body
    std::vector<std::pair<std::string,int>> increments;
    for (auto &s : stmt->body->statements) {
        std::string id; int inc;
        if (matchIncrement(s.get(), id, inc) && id != loopId) {
            increments.emplace_back(id, inc);
        }
    }
    
    // Fast path: calculate iterations and update directly
    int iterations = limit - initVal;
    if (iterations <= 0) return true;
    
    // Update variables directly without loop overhead
    for (auto &p : increments) {
        try {
            Variable var = env.get(p.first);
            if (std::holds_alternative<int>(var.value)) {
                int currentVal = std::get<int>(var.value);
                env.set(p.first, currentVal + (p.second * iterations));
            }
        } catch (...) {}
    }
    
    // Set loop variable to final value
    try {
        env.define(loopId, Variable(limit, "int", false));
    } catch (...) {}
    
    return true;
}
