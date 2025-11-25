#include "../include/operators.h"
#include <stdexcept>

BinaryOperator stringToBinaryOp(const std::string& op) {
    if (op == "+") return BinaryOperator::ADD;
    if (op == "-") return BinaryOperator::SUBTRACT;
    if (op == "*") return BinaryOperator::MULTIPLY;
    if (op == "/") return BinaryOperator::DIVIDE;
    if (op == "%") return BinaryOperator::MODULO;
    if (op == "==") return BinaryOperator::EQUAL;
    if (op == "!=") return BinaryOperator::NOT_EQUAL;
    if (op == "<") return BinaryOperator::LESS;
    if (op == ">") return BinaryOperator::GREATER;
    if (op == "<=") return BinaryOperator::LESS_EQUAL;
    if (op == ">=") return BinaryOperator::GREATER_EQUAL;
    if (op == "&&") return BinaryOperator::LOGICAL_AND;
    if (op == "||") return BinaryOperator::LOGICAL_OR;
    if (op == "=") return BinaryOperator::ASSIGN;
    throw std::runtime_error("Unknown binary operator: " + op);
}

UnaryOperator stringToUnaryOp(const std::string& op) {
    if (op == "-") return UnaryOperator::NEGATE;
    if (op == "!") return UnaryOperator::LOGICAL_NOT;
    if (op == "typeof") return UnaryOperator::TYPEOF;
    throw std::runtime_error("Unknown unary operator: " + op);
}

std::string binaryOpToString(BinaryOperator op) {
    switch (op) {
        case BinaryOperator::ADD: return "+";
        case BinaryOperator::SUBTRACT: return "-";
        case BinaryOperator::MULTIPLY: return "*";
        case BinaryOperator::DIVIDE: return "/";
        case BinaryOperator::MODULO: return "%";
        case BinaryOperator::EQUAL: return "==";
        case BinaryOperator::NOT_EQUAL: return "!=";
        case BinaryOperator::LESS: return "<";
        case BinaryOperator::GREATER: return ">";
        case BinaryOperator::LESS_EQUAL: return "<=";
        case BinaryOperator::GREATER_EQUAL: return ">=";
        case BinaryOperator::LOGICAL_AND: return "&&";
        case BinaryOperator::LOGICAL_OR: return "||";
        case BinaryOperator::ASSIGN: return "=";
    }
    return "unknown";
}

std::string unaryOpToString(UnaryOperator op) {
    switch (op) {
        case UnaryOperator::NEGATE: return "-";
        case UnaryOperator::LOGICAL_NOT: return "!";
        case UnaryOperator::TYPEOF: return "typeof";
    }
    return "unknown";
}