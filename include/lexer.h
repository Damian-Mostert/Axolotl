#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <string>
#include <vector>
#include <unordered_map>

class Lexer {
public:
    Lexer(const std::string& source);
    
    std::vector<Token> tokenize();
    Token nextToken();
    
private:
    std::string source;
    size_t position;
    int line;
    int column;
    
    static std::unordered_map<std::string, TokenType> keywords;
    
    char currentChar() const;
    char peekChar(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipComment();
    
    Token readNumber();
    Token readString();
    Token readIdentifier();
    Token readOperator();
    
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isOperatorChar(char c) const;
};

#endif // LEXER_H
