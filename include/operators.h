#ifndef OPERATORS_H
#define OPERATORS_H

#include <string>

enum class BinaryOperator {
    ADD,        // +
    SUBTRACT,   // -
    MULTIPLY,   // *
    DIVIDE,     // /
    MODULO,     // %
    EQUAL,      // ==
    NOT_EQUAL,  // !=
    LESS,       // <
    GREATER,    // >
    LESS_EQUAL, // <=
    GREATER_EQUAL, // >=
    LOGICAL_AND,   // &&
    LOGICAL_OR,    // ||
    ASSIGN      // =
};

enum class UnaryOperator {
    NEGATE,     // -
    LOGICAL_NOT, // !
    TYPEOF      // typeof
};

// Convert string operators to enums for fast comparison
BinaryOperator stringToBinaryOp(const std::string& op);
UnaryOperator stringToUnaryOp(const std::string& op);

// Convert enums back to strings for debugging
std::string binaryOpToString(BinaryOperator op);
std::string unaryOpToString(UnaryOperator op);

#endif // OPERATORS_H