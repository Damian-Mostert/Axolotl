#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <memory>

enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    IDENTIFIER,
    
    // Keywords
    KW_INT,
    KW_FLOAT,
    KW_STRING,
    KW_BOOL,
    KW_VOID,
    KW_ANY,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_RETURN,
    KW_FUNC,
    KW_VAR,
    KW_CONST,
    KW_IMPORT,
    KW_USE,
    KW_EXPORT,
    KW_OBJECT,
    KW_TRUE,
    KW_FALSE,
    KW_PROGRAM,
    KW_AWAIT,
    KW_TYPE,
    KW_TYPEOF,
    KW_TRY,
    KW_CATCH,
    KW_FINALLY,
    KW_THROW,
    KW_BREAK,
    KW_CONTINUE,
    KW_SWITCH,
    KW_CASE,
    KW_DEFAULT,
    
    // Operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    ASSIGN,
    EQUAL,
    NOT_EQUAL,
    LESS,
    GREATER,
    LESS_EQUAL,
    GREATER_EQUAL,
    LOGICAL_AND,
    LOGICAL_OR,
    PIPE,
    LOGICAL_NOT,
    
    // Delimiters
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    COMMA,
    DOT,
    COLON,
    ARROW,
    
    // Special
    EOF_TOKEN,
    NEWLINE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType type = TokenType::UNKNOWN, const std::string& value = "", 
          int line = 0, int column = 0)
        : type(type), value(value), line(line), column(column) {}
    
    std::string toString() const;
};

#endif // TOKEN_H
