#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <stdexcept>

#include <variant>
#include <unordered_set>
#include <vector>
#include <thread>
#include <future>
#include <mutex>

// Forward declaration for JIT
class LLVMJITCompiler;

// Forward declarations
class ArrayValue;
class ObjectValue;

// Value type supports int, float, string, bool, array, object, and function refs
using Value = std::variant<int, float, std::string, bool, 
                           std::shared_ptr<ArrayValue>, 
                           std::shared_ptr<ObjectValue>,
                           class FunctionDeclaration*,
                           class FunctionExpression*>;

// Array: ordered collection of Values
class ArrayValue {
public:
    std::vector<Value> elements;
    ArrayValue() = default;
    explicit ArrayValue(const std::vector<Value>& elems) : elements(elems) {}
};

// Object: key-value map
class ObjectValue {
public:
    std::unordered_map<std::string, Value> fields;
    ObjectValue() = default;
};

struct Variable {
    Value value;
    std::string type;
    bool isConst;
    
    Variable(const Value& v = 0, const std::string& t = "int", bool c = false)
        : value(v), type(t), isConst(c) {}
};

class Environment {
public:
    void define(const std::string& name, const Variable& var);
    Variable get(const std::string& name) const;
    void set(const std::string& name, const Value& value);
    bool has(const std::string& name) const;
    void pushScope();
    void popScope();
    
private:
    std::vector<std::unordered_map<std::string, Variable>> scopes;
};

class ReturnException : public std::exception {
public:
    Value value;
    ReturnException(const Value& v) : value(v) {}
};

class Interpreter : public ASTVisitor {
public:
    Interpreter();
    ~Interpreter();
    
    void interpret(Program* program);
    
    std::string visit(IntegerLiteral* node) override;
    std::string visit(FloatLiteral* node) override;
    std::string visit(StringLiteral* node) override;
    std::string visit(BooleanLiteral* node) override;
    std::string visit(Identifier* node) override;
    std::string visit(BinaryOp* node) override;
    std::string visit(UnaryOp* node) override;
    std::string visit(FunctionCall* node) override;
    std::string visit(ArrayLiteral* node) override;
    std::string visit(ObjectLiteral* node) override;
    std::string visit(FunctionExpression* node) override;
    std::string visit(IndexAccess* node) override;
    std::string visit(FieldAccess* node) override;
    std::string visit(Block* node) override;
    std::string visit(VariableDeclaration* node) override;
    std::string visit(Assignment* node) override;
    std::string visit(IndexAssignment* node) override;
    std::string visit(FieldAssignment* node) override;
    std::string visit(IfStatement* node) override;
    std::string visit(WhileStatement* node) override;
    std::string visit(ForStatement* node) override;
    std::string visit(ReturnStatement* node) override;
    std::string visit(FunctionDeclaration* node) override;
    std::string visit(ProgramDeclaration* node) override;
    std::string visit(AwaitExpression* node) override;
    std::string visit(ExpressionStatement* node) override;
    std::string visit(Program* node) override;
    std::string visit(ImportDeclaration* node) override;
    
private:
    Environment environment;
    std::unordered_map<std::string, FunctionDeclaration*> functions;
    std::unordered_map<std::string, ProgramDeclaration*> programs;
    std::unordered_map<std::string, std::future<void>> runningPrograms;
    std::mutex programsMutex;
    std::unordered_set<std::string> importedFiles;
    Value lastValue;  // Store last evaluated complex value (array, object)
    std::unique_ptr<LLVMJITCompiler> jitCompiler;  // JIT compiler for loop optimization
    
    Value evaluate(Expression* expr);
    void execute(Statement* stmt);
    void executeBlock(Block* block);
    
    Value performBinaryOp(const Value& left, BinaryOperator op, const Value& right);
    Value performUnaryOp(UnaryOperator op, const Value& operand);
    bool isTruthy(const Value& v);
    std::string valueToString(const Value& v);
};

#endif // INTERPRETER_H
