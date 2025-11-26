#include "include/ast.h"

// IntegerLiteral
std::string IntegerLiteral::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// FloatLiteral
std::string FloatLiteral::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// StringLiteral
std::string StringLiteral::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// BooleanLiteral
std::string BooleanLiteral::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// Identifier
std::string Identifier::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// BinaryOp
std::string BinaryOp::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// UnaryOp
std::string UnaryOp::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// FunctionCall
std::string FunctionCall::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ArrayLiteral
std::string ArrayLiteral::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ObjectLiteral
std::string ObjectLiteral::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// FunctionExpression
std::string FunctionExpression::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

std::string ThrowStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

std::string TryStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// IndexAccess
std::string IndexAccess::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// FieldAccess
std::string FieldAccess::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// IndexAssignment
std::string IndexAssignment::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// FieldAssignment
std::string FieldAssignment::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// Block
std::string Block::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// VariableDeclaration
std::string VariableDeclaration::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// Assignment
std::string Assignment::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// IfStatement
std::string IfStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// WhileStatement
std::string WhileStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ForStatement
std::string ForStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ReturnStatement
std::string ReturnStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// FunctionDeclaration
std::string FunctionDeclaration::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ExpressionStatement
std::string ExpressionStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// Program
std::string Program::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ImportDeclaration
std::string ImportDeclaration::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// UseDeclaration
std::string UseDeclaration::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ExportDeclaration
std::string ExportDeclaration::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ProgramDeclaration
std::string ProgramDeclaration::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// AwaitExpression
std::string AwaitExpression::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// TypeDeclaration
std::string TypeDeclaration::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// BreakStatement
std::string BreakStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// ContinueStatement
std::string ContinueStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// CaseClause
std::string CaseClause::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// SwitchStatement
std::string SwitchStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

std::string WhenStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}
