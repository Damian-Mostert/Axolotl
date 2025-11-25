#include "include/token.h"

std::string Token::toString() const {
    std::string typeStr;
    switch (type) {
        case TokenType::INTEGER: typeStr = "INTEGER"; break;
        case TokenType::FLOAT: typeStr = "FLOAT"; break;
        case TokenType::STRING: typeStr = "STRING"; break;
        case TokenType::IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case TokenType::KW_INT: typeStr = "KW_INT"; break;
        case TokenType::KW_FLOAT: typeStr = "KW_FLOAT"; break;
        case TokenType::KW_STRING: typeStr = "KW_STRING"; break;
        case TokenType::KW_BOOL: typeStr = "KW_BOOL"; break;
        case TokenType::KW_VOID: typeStr = "KW_VOID"; break;
        case TokenType::KW_IF: typeStr = "KW_IF"; break;
        case TokenType::KW_ELSE: typeStr = "KW_ELSE"; break;
        case TokenType::KW_WHILE: typeStr = "KW_WHILE"; break;
        case TokenType::KW_FOR: typeStr = "KW_FOR"; break;
        case TokenType::KW_RETURN: typeStr = "KW_RETURN"; break;
        case TokenType::KW_FUNC: typeStr = "KW_FUNC"; break;
        case TokenType::KW_VAR: typeStr = "KW_VAR"; break;
        case TokenType::KW_CONST: typeStr = "KW_CONST"; break;
        case TokenType::KW_TRUE: typeStr = "KW_TRUE"; break;
        case TokenType::KW_FALSE: typeStr = "KW_FALSE"; break;
        case TokenType::PLUS: typeStr = "PLUS"; break;
        case TokenType::MINUS: typeStr = "MINUS"; break;
        case TokenType::STAR: typeStr = "STAR"; break;
        case TokenType::SLASH: typeStr = "SLASH"; break;
        case TokenType::PERCENT: typeStr = "PERCENT"; break;
        case TokenType::ASSIGN: typeStr = "ASSIGN"; break;
        case TokenType::EQUAL: typeStr = "EQUAL"; break;
        case TokenType::NOT_EQUAL: typeStr = "NOT_EQUAL"; break;
        case TokenType::LESS: typeStr = "LESS"; break;
        case TokenType::GREATER: typeStr = "GREATER"; break;
        case TokenType::LESS_EQUAL: typeStr = "LESS_EQUAL"; break;
        case TokenType::GREATER_EQUAL: typeStr = "GREATER_EQUAL"; break;
        case TokenType::LOGICAL_AND: typeStr = "LOGICAL_AND"; break;
        case TokenType::LOGICAL_OR: typeStr = "LOGICAL_OR"; break;
        case TokenType::PIPE: typeStr = "PIPE"; break;
        case TokenType::LOGICAL_NOT: typeStr = "LOGICAL_NOT"; break;
        case TokenType::LPAREN: typeStr = "LPAREN"; break;
        case TokenType::RPAREN: typeStr = "RPAREN"; break;
        case TokenType::LBRACE: typeStr = "LBRACE"; break;
        case TokenType::RBRACE: typeStr = "RBRACE"; break;
        case TokenType::LBRACKET: typeStr = "LBRACKET"; break;
        case TokenType::RBRACKET: typeStr = "RBRACKET"; break;
        case TokenType::SEMICOLON: typeStr = "SEMICOLON"; break;
        case TokenType::COMMA: typeStr = "COMMA"; break;
        case TokenType::DOT: typeStr = "DOT"; break;
        case TokenType::COLON: typeStr = "COLON"; break;
        case TokenType::ARROW: typeStr = "ARROW"; break;
        case TokenType::EOF_TOKEN: typeStr = "EOF"; break;
        case TokenType::NEWLINE: typeStr = "NEWLINE"; break;
        case TokenType::UNKNOWN: typeStr = "UNKNOWN"; break;
    }
    return typeStr + "(" + value + ")";
}
