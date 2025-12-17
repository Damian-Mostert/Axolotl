#include "include/ui_ast.h"
#include "include/interpreter.h"

std::string UIElement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

std::string UIFragment::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

std::string UIText::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

std::string UIExpression::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}
