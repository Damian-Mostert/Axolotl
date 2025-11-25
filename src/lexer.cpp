#include "../include/lexer.h"
#include <cctype>

std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"int", TokenType::KW_INT},
    {"float", TokenType::KW_FLOAT},
    {"string", TokenType::KW_STRING},
    {"bool", TokenType::KW_BOOL},
    {"void", TokenType::KW_VOID},
    {"if", TokenType::KW_IF},
    {"else", TokenType::KW_ELSE},
    {"while", TokenType::KW_WHILE},
    {"for", TokenType::KW_FOR},
    {"return", TokenType::KW_RETURN},
    {"func", TokenType::KW_FUNC},
    {"var", TokenType::KW_VAR},
    {"const", TokenType::KW_CONST},
    {"import", TokenType::KW_IMPORT},
    {"use", TokenType::KW_IMPORT},
    {"object", TokenType::KW_OBJECT},
    {"true", TokenType::KW_TRUE},
    {"false", TokenType::KW_FALSE},
};

Lexer::Lexer(const std::string& source)
    : source(source), position(0), line(1), column(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (position < source.length()) {
        Token token = nextToken();
        if (token.type != TokenType::NEWLINE) {
            tokens.push_back(token);
        }
        if (token.type == TokenType::EOF_TOKEN) break;
    }
    return tokens;
}

Token Lexer::nextToken() {
    skipWhitespace();
    
    if (position >= source.length()) {
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }
    
    char current = currentChar();
    
    // Comments
    if (current == '/' && peekChar() == '/') {
        skipComment();
        return nextToken();
    }
    
    // String literals
    if (current == '"') {
        return readString();
    }
    
    // Numbers
    if (isDigit(current)) {
        return readNumber();
    }
    
    // Identifiers and keywords
    if (isAlpha(current)) {
        return readIdentifier();
    }
    
    // Operators and delimiters
    return readOperator();
}

char Lexer::currentChar() const {
    if (position >= source.length()) return '\0';
    return source[position];
}

char Lexer::peekChar(size_t offset) const {
    if (position + offset >= source.length()) return '\0';
    return source[position + offset];
}

void Lexer::advance() {
    if (position < source.length()) {
        if (source[position] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        position++;
    }
}

void Lexer::skipWhitespace() {
    while (position < source.length() && std::isspace(currentChar())) {
        advance();
    }
}

void Lexer::skipComment() {
    // Skip //
    advance();
    advance();
    // Skip until end of line
    while (position < source.length() && currentChar() != '\n') {
        advance();
    }
}

Token Lexer::readNumber() {
    int startLine = line;
    int startCol = column;
    std::string value;
    bool isFloat = false;
    
    while (isDigit(currentChar())) {
        value += currentChar();
        advance();
    }
    
    if (currentChar() == '.' && isDigit(peekChar())) {
        isFloat = true;
        value += currentChar();
        advance();
        while (isDigit(currentChar())) {
            value += currentChar();
            advance();
        }
    }
    
    return Token(isFloat ? TokenType::FLOAT : TokenType::INTEGER, value, startLine, startCol);
}

Token Lexer::readString() {
    int startLine = line;
    int startCol = column;
    advance(); // Skip opening quote
    std::string value;
    
    while (currentChar() != '"' && position < source.length()) {
        if (currentChar() == '\\') {
            advance();
            switch (currentChar()) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default: value += currentChar();
            }
        } else {
            value += currentChar();
        }
        advance();
    }
    
    if (currentChar() == '"') {
        advance(); // Skip closing quote
    }
    
    return Token(TokenType::STRING, value, startLine, startCol);
}

Token Lexer::readIdentifier() {
    int startLine = line;
    int startCol = column;
    std::string value;
    
    while (isAlphaNumeric(currentChar()) || currentChar() == '_') {
        value += currentChar();
        advance();
    }
    
    // Check if it's a keyword
    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return Token(it->second, value, startLine, startCol);
    }
    
    return Token(TokenType::IDENTIFIER, value, startLine, startCol);
}

Token Lexer::readOperator() {
    int startLine = line;
    int startCol = column;
    char current = currentChar();
    
    switch (current) {
        case '+': advance(); return Token(TokenType::PLUS, "+", startLine, startCol);
        case '-':
            advance();
            if (currentChar() == '>') {
                advance();
                return Token(TokenType::ARROW, "->", startLine, startCol);
            }
            return Token(TokenType::MINUS, "-", startLine, startCol);
        case '*': advance(); return Token(TokenType::STAR, "*", startLine, startCol);
        case '/': advance(); return Token(TokenType::SLASH, "/", startLine, startCol);
        case '%': advance(); return Token(TokenType::PERCENT, "%", startLine, startCol);
        case '=':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::EQUAL, "==", startLine, startCol);
            }
            return Token(TokenType::ASSIGN, "=", startLine, startCol);
        case '!':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::NOT_EQUAL, "!=", startLine, startCol);
            }
            return Token(TokenType::LOGICAL_NOT, "!", startLine, startCol);
        case '<':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::LESS_EQUAL, "<=", startLine, startCol);
            }
            return Token(TokenType::LESS, "<", startLine, startCol);
        case '>':
            advance();
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::GREATER_EQUAL, ">=", startLine, startCol);
            }
            return Token(TokenType::GREATER, ">", startLine, startCol);
        case '&':
            advance();
            if (currentChar() == '&') {
                advance();
                return Token(TokenType::LOGICAL_AND, "&&", startLine, startCol);
            }
            break;
        case '|':
            advance();
            if (currentChar() == '|') {
                advance();
                return Token(TokenType::LOGICAL_OR, "||", startLine, startCol);
            }
            break;
        case '(': advance(); return Token(TokenType::LPAREN, "(", startLine, startCol);
        case ')': advance(); return Token(TokenType::RPAREN, ")", startLine, startCol);
        case '{': advance(); return Token(TokenType::LBRACE, "{", startLine, startCol);
        case '}': advance(); return Token(TokenType::RBRACE, "}", startLine, startCol);
        case '[': advance(); return Token(TokenType::LBRACKET, "[", startLine, startCol);
        case ']': advance(); return Token(TokenType::RBRACKET, "]", startLine, startCol);
        case ';': advance(); return Token(TokenType::SEMICOLON, ";", startLine, startCol);
        case ',': advance(); return Token(TokenType::COMMA, ",", startLine, startCol);
        case '.': advance(); return Token(TokenType::DOT, ".", startLine, startCol);
        case ':': advance(); return Token(TokenType::COLON, ":", startLine, startCol);
        default:
            advance();
            return Token(TokenType::UNKNOWN, std::string(1, current), startLine, startCol);
    }
    
    return Token(TokenType::UNKNOWN, std::string(1, current), startLine, startCol);
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

bool Lexer::isOperatorChar(char c) const {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '=' || c == '!' || c == '<' || c == '>' || c == '&' || c == '|';
}
