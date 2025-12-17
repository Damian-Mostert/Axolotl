#ifndef AXO_UI_AST_H
#define AXO_UI_AST_H

#include "ast.h"
#include <memory>
#include <string>
#include <vector>

// AXO UI Element: <tag attr="value">children</tag>
class UIElement : public Expression {
public:
    std::string tagName;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> attributes;
    std::vector<std::unique_ptr<Expression>> children;
    bool selfClosing;
    
    UIElement(const std::string& tag, bool selfClose = false)
        : tagName(tag), selfClosing(selfClose) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

// AXO UI Fragment: <>children</>
class UIFragment : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> children;
    
    UIFragment() = default;
    std::string accept(class ASTVisitor* visitor) override;
};

// AXO UI Text: plain text between tags
class UIText : public Expression {
public:
    std::string text;
    
    UIText(const std::string& t) : text(t) {}
    std::string accept(class ASTVisitor* visitor) override;
};

// AXO UI Expression: {expression}
class UIExpression : public Expression {
public:
    std::unique_ptr<Expression> expression;
    
    UIExpression(std::unique_ptr<Expression> expr)
        : expression(std::move(expr)) {}
    
    std::string accept(class ASTVisitor* visitor) override;
};

#endif // AXO_UI_AST_H
