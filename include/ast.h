#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "operators.h"

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string accept(class ASTVisitor* visitor) = 0;
};

// Expressions
class Expression : public ASTNode {};

class IntegerLiteral : public Expression {
public:
    int value;
    IntegerLiteral(int val) : value(val) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class FloatLiteral : public Expression {
public:
    float value;
    FloatLiteral(float val) : value(val) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class StringLiteral : public Expression {
public:
    std::string value;
    StringLiteral(const std::string& val) : value(val) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class BooleanLiteral : public Expression {
public:
    bool value;
    BooleanLiteral(bool val) : value(val) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class Identifier : public Expression {
public:
    std::string name;
    Identifier(const std::string& n) : name(n) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class BinaryOp : public Expression {
public:
    std::unique_ptr<Expression> left;
    BinaryOperator op;
    std::unique_ptr<Expression> right;
    
    BinaryOp(std::unique_ptr<Expression> l, BinaryOperator o, 
             std::unique_ptr<Expression> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class UnaryOp : public Expression {
public:
    UnaryOperator op;
    std::unique_ptr<Expression> operand;
    
    UnaryOp(UnaryOperator o, std::unique_ptr<Expression> e)
        : op(o), operand(std::move(e)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class FunctionCall : public Expression {
public:
    // Either `name` is used (simple call) or `callee` is used (call on an expression)
    std::string name;
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> args;
    
    FunctionCall(const std::string& n) : name(n) {}
    FunctionCall(std::unique_ptr<Expression> c) : callee(std::move(c)) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class ArrayLiteral : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;
    ArrayLiteral() = default;
    std::string accept(class ASTVisitor* visitor) override;
};

class ObjectLiteral : public Expression {
public:
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> fields;
    ObjectLiteral() = default;
    std::string accept(class ASTVisitor* visitor) override;
};

class Block;  // Forward declaration added

class FunctionExpression : public Expression {
public:
    std::vector<std::pair<std::string, std::string>> params; // name, type
    std::string returnType;
    std::unique_ptr<Block> body;
    
    FunctionExpression(const std::string& rt, std::unique_ptr<Block> b)
        : returnType(rt), body(std::move(b)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class IndexAccess : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> index;
    
    IndexAccess(std::unique_ptr<Expression> obj, std::unique_ptr<Expression> idx)
        : object(std::move(obj)), index(std::move(idx)) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class FieldAccess : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::string field;
    
    FieldAccess(std::unique_ptr<Expression> obj, const std::string& f)
        : object(std::move(obj)), field(f) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class IndexAssignment : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> index;
    std::unique_ptr<Expression> value;

    IndexAssignment(std::unique_ptr<Expression> obj, std::unique_ptr<Expression> idx,
                    std::unique_ptr<Expression> val)
        : object(std::move(obj)), index(std::move(idx)), value(std::move(val)) {}

    std::string accept(class ASTVisitor* visitor) override;
};

class FieldAssignment : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::string field;
    std::unique_ptr<Expression> value;

    FieldAssignment(std::unique_ptr<Expression> obj, const std::string& f, std::unique_ptr<Expression> val)
        : object(std::move(obj)), field(f), value(std::move(val)) {}

    std::string accept(class ASTVisitor* visitor) override;
};

// Statements
class Statement : public ASTNode {};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    ExpressionStatement(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class Block : public Statement {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    std::string accept(class ASTVisitor* visitor) override;
};

class VariableDeclaration : public Statement {
public:
    std::string name;
    std::string type;
    std::unique_ptr<Expression> initializer;
    
    VariableDeclaration(const std::string& n, const std::string& t, 
                        std::unique_ptr<Expression> init = nullptr)
        : name(n), type(t), initializer(std::move(init)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class Assignment : public Expression {
public:
    std::string name;
    std::unique_ptr<Expression> value;
    
    Assignment(const std::string& n, std::unique_ptr<Expression> v)
        : name(n), value(std::move(v)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> thenBlock;
    std::unique_ptr<Block> elseBlock;
    
    IfStatement(std::unique_ptr<Expression> cond, std::unique_ptr<Block> thenBlk,
                std::unique_ptr<Block> elseBlk = nullptr)
        : condition(std::move(cond)), thenBlock(std::move(thenBlk)), 
          elseBlock(std::move(elseBlk)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> body;
    
    WhileStatement(std::unique_ptr<Expression> cond, std::unique_ptr<Block> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class ForStatement : public Statement {
public:
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> update;
    std::unique_ptr<Block> body;
    
    ForStatement(std::unique_ptr<ASTNode> i, std::unique_ptr<Expression> c,
                 std::unique_ptr<Expression> u, std::unique_ptr<Block> b)
        : init(std::move(i)), condition(std::move(c)), update(std::move(u)), 
          body(std::move(b)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    
    ReturnStatement(std::unique_ptr<Expression> v = nullptr)
        : value(std::move(v)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class FunctionDeclaration : public ASTNode {
public:
    std::string name;
    std::vector<std::pair<std::string, std::string>> params; // name, type
    std::string returnType;
    std::unique_ptr<Block> body;
    
    FunctionDeclaration(const std::string& n, const std::string& rt,
                        std::unique_ptr<Block> b)
        : name(n), returnType(rt), body(std::move(b)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class ProgramDeclaration : public ASTNode {
public:
    std::string name;
    std::vector<std::pair<std::string, std::string>> params; // name, type
    std::unique_ptr<Block> body;
    
    ProgramDeclaration(const std::string& n, std::unique_ptr<Block> b)
        : name(n), body(std::move(b)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class AwaitExpression : public Expression {
public:
    std::unique_ptr<Expression> expression;
    
    AwaitExpression(std::unique_ptr<Expression> expr)
        : expression(std::move(expr)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class ImportDeclaration : public ASTNode {
public:
    std::string path;
    std::vector<std::string> namedImports;
    std::string defaultImport;
    
    ImportDeclaration(const std::string& p) : path(p) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class UseDeclaration : public ASTNode {
public:
    std::string path;
    
    UseDeclaration(const std::string& p) : path(p) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class ExportDeclaration : public ASTNode {
public:
    std::unique_ptr<ASTNode> declaration;
    std::vector<std::string> namedExports;
    bool isDefault;
    
    ExportDeclaration(std::unique_ptr<ASTNode> decl, bool isDefault = false)
        : declaration(std::move(decl)), isDefault(isDefault) {}
    
    ExportDeclaration(const std::vector<std::string>& exports)
        : namedExports(exports), isDefault(false) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class TypeDeclaration : public ASTNode {
public:
    std::string name;
    std::string typeSpec;
    
    TypeDeclaration(const std::string& n, const std::string& spec)
        : name(n), typeSpec(spec) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class ThrowStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    
    ThrowStatement(std::unique_ptr<Expression> expr)
        : expression(std::move(expr)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class TryStatement : public Statement {
public:
    std::unique_ptr<Block> tryBlock;
    std::string catchVariable;
    std::unique_ptr<Block> catchBlock;
    std::unique_ptr<Block> finallyBlock;
    
    TryStatement(std::unique_ptr<Block> tryBlk, const std::string& catchVar = "",
                 std::unique_ptr<Block> catchBlk = nullptr, 
                 std::unique_ptr<Block> finallyBlk = nullptr)
        : tryBlock(std::move(tryBlk)), catchVariable(catchVar),
          catchBlock(std::move(catchBlk)), finallyBlock(std::move(finallyBlk)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class BreakStatement : public Statement {
public:
    BreakStatement() = default;
    std::string accept(class ASTVisitor* visitor) override;
};

class ContinueStatement : public Statement {
public:
    ContinueStatement() = default;
    std::string accept(class ASTVisitor* visitor) override;
};

class CaseClause : public ASTNode {
public:
    std::unique_ptr<Expression> value; // null for default case
    std::vector<std::unique_ptr<ASTNode>> statements;
    bool isDefault;
    
    CaseClause(std::unique_ptr<Expression> val = nullptr, bool isDefault = false)
        : value(std::move(val)), isDefault(isDefault) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

class SwitchStatement : public Statement {
public:
    std::unique_ptr<Expression> discriminant;
    std::vector<std::unique_ptr<CaseClause>> cases;
    
    SwitchStatement(std::unique_ptr<Expression> disc)
        : discriminant(std::move(disc)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};



class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> declarations;
    
    std::string accept(class ASTVisitor* visitor) override;
};

// Visitor Pattern
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual std::string visit(IntegerLiteral* node) = 0;
    virtual std::string visit(FloatLiteral* node) = 0;
    virtual std::string visit(StringLiteral* node) = 0;
    virtual std::string visit(BooleanLiteral* node) = 0;
    virtual std::string visit(Identifier* node) = 0;
    virtual std::string visit(BinaryOp* node) = 0;
    virtual std::string visit(UnaryOp* node) = 0;
    virtual std::string visit(FunctionCall* node) = 0;
    virtual std::string visit(ArrayLiteral* node) = 0;
    virtual std::string visit(ObjectLiteral* node) = 0;
    virtual std::string visit(FunctionExpression* node) = 0;
    virtual std::string visit(IndexAccess* node) = 0;
    virtual std::string visit(FieldAccess* node) = 0;
    virtual std::string visit(Block* node) = 0;
    virtual std::string visit(VariableDeclaration* node) = 0;
    virtual std::string visit(Assignment* node) = 0;
    virtual std::string visit(IndexAssignment* node) = 0;
    virtual std::string visit(FieldAssignment* node) = 0;
    virtual std::string visit(IfStatement* node) = 0;
    virtual std::string visit(WhileStatement* node) = 0;
    virtual std::string visit(ForStatement* node) = 0;
    virtual std::string visit(ReturnStatement* node) = 0;
    virtual std::string visit(FunctionDeclaration* node) = 0;
    virtual std::string visit(ProgramDeclaration* node) = 0;
    virtual std::string visit(AwaitExpression* node) = 0;
    virtual std::string visit(ExpressionStatement* node) = 0;
    virtual std::string visit(ImportDeclaration* node) = 0;
    virtual std::string visit(UseDeclaration* node) = 0;
    virtual std::string visit(ExportDeclaration* node) = 0;
    virtual std::string visit(TypeDeclaration* node) = 0;
    virtual std::string visit(ThrowStatement* node) = 0;
    virtual std::string visit(TryStatement* node) = 0;
    virtual std::string visit(BreakStatement* node) = 0;
    virtual std::string visit(ContinueStatement* node) = 0;
    virtual std::string visit(CaseClause* node) = 0;
    virtual std::string visit(SwitchStatement* node) = 0;
    virtual std::string visit(Program* node) = 0;
};

#endif // AST_H
