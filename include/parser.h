#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"
#include <vector>
#include <memory>
#include <stdexcept>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    
    std::unique_ptr<Program> parse();
    
private:
    std::vector<Token> tokens;
    size_t current;
    
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    bool isAtEnd() const;
    
    // Parsing methods
    std::unique_ptr<ASTNode> parseDeclaration();
    std::unique_ptr<ASTNode> parseImportDeclaration();
    std::unique_ptr<TypeDeclaration> parseTypeDeclaration();
    std::unique_ptr<FunctionDeclaration> parseFunctionDeclaration();
    std::unique_ptr<ProgramDeclaration> parseProgramDeclaration();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Block> parseBlock();
    std::unique_ptr<Statement> parseIfStatement();
    std::unique_ptr<Statement> parseWhileStatement();
    std::unique_ptr<Statement> parseForStatement();
    std::unique_ptr<Statement> parseReturnStatement();
    std::unique_ptr<Statement> parseTryStatement();
    std::unique_ptr<Statement> parseThrowStatement();
    std::unique_ptr<Statement> parseBreakStatement();
    std::unique_ptr<Statement> parseContinueStatement();
    std::unique_ptr<Statement> parseVariableDeclaration();
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseAssignment();
    std::unique_ptr<Expression> parseLogicalOr();
    std::unique_ptr<Expression> parseLogicalAnd();
    std::unique_ptr<Expression> parseEquality();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parseTerm();
    std::unique_ptr<Expression> parseFactor();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parsePrimary();
    std::unique_ptr<Expression> parsePostfix();
    
    // Helper for parsing function types: (type1, type2)->returnType
    std::string parseFunctionType();
    
    // Helper for parsing complex type specifications
    std::string parseComplexTypeSpec();
    std::string parseSimpleTypeSpec();
    std::string parseArrayType();
};

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message)
        : std::runtime_error(message), line(0), column(0), tokenValue("") {}

    ParseError(const std::string& message, const Token& token)
        : std::runtime_error(message), line(token.line), column(token.column), tokenValue(token.value) {}

    int getLine() const { return line; }
    int getColumn() const { return column; }
    const std::string& getTokenValue() const { return tokenValue; }

private:
    int line;
    int column;
    std::string tokenValue;
};

#endif // PARSER_H
