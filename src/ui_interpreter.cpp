#include "include/interpreter.h"
#include "include/ui_ast.h"

std::string Interpreter::visit(UIElement* node) {
    (void)node;
    return "";
}

std::string Interpreter::visit(UIFragment* node) {
    (void)node;
    return "";
}

std::string Interpreter::visit(UIText* node) {
    (void)node;
    return "";
}

std::string Interpreter::visit(UIExpression* node) {
    return node->expression->accept(this);
}
