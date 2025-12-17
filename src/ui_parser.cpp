#include "include/parser.h"
#include "include/ui_ast.h"

bool Parser::isUIStart() const {
    return peek().type == TokenType::LESS && 
           (current + 1 < tokens.size() && 
            (tokens[current + 1].type == TokenType::IDENTIFIER ||
             tokens[current + 1].type == TokenType::GREATER));
}

std::unique_ptr<Expression> Parser::parseUIElement() {
    consume(TokenType::LESS, "Expected '<'");
    
    // Check for fragment <>
    if (check(TokenType::GREATER)) {
        return parseUIFragment();
    }
    
    Token tagToken = consume(TokenType::IDENTIFIER, "Expected tag name");
    auto elem = std::make_unique<UIElement>(tagToken.value);
    
    // Parse attributes
    while (!check(TokenType::GREATER) && !check(TokenType::SLASH)) {
        Token attrName = consume(TokenType::IDENTIFIER, "Expected attribute name");
        consume(TokenType::ASSIGN, "Expected '='");
        
        std::unique_ptr<Expression> attrValue;
        if (check(TokenType::STRING)) {
            attrValue = std::make_unique<StringLiteral>(advance().value);
        } else if (check(TokenType::LBRACE)) {
            advance(); // consume {
            attrValue = parseExpression();
            consume(TokenType::RBRACE, "Expected '}'");
        }
        
        elem->attributes.push_back({attrName.value, std::move(attrValue)});
    }
    
    // Self-closing tag
    if (match({TokenType::SLASH})) {
        consume(TokenType::GREATER, "Expected '>'");
        elem->selfClosing = true;
        return elem;
    }
    
    consume(TokenType::GREATER, "Expected '>'");
    
    // Parse children (text, expressions, or nested elements)
    while (true) {
        if (check(TokenType::LESS)) {
            // Check if it's closing tag
            if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::SLASH) {
                break;
            }
            // Nested element
            elem->children.push_back(parseUIElement());
        } else if (check(TokenType::LBRACE)) {
            advance(); // consume {
            auto expr = parseExpression();
            elem->children.push_back(std::make_unique<UIExpression>(std::move(expr)));
            consume(TokenType::RBRACE, "Expected '}'");
        } else if (!check(TokenType::EOF_TOKEN) && !isAtEnd()) {
            // Text content - collect tokens until < or {
            std::string text;
            while (!check(TokenType::LESS) && !check(TokenType::LBRACE) && !isAtEnd()) {
                text += peek().value;
                if (peek().type != TokenType::IDENTIFIER) text += " ";
                advance();
            }
            if (!text.empty()) {
                elem->children.push_back(std::make_unique<UIText>(text));
            }
        } else {
            break;
        }
    }
    
    // Closing tag
    consume(TokenType::LESS, "Expected '<'");
    consume(TokenType::SLASH, "Expected '/'");
    Token closeTag = consume(TokenType::IDENTIFIER, "Expected closing tag");
    if (closeTag.value != elem->tagName) {
        throw ParseError("Mismatched closing tag: expected " + elem->tagName + " but got " + closeTag.value);
    }
    consume(TokenType::GREATER, "Expected '>'");
    
    return elem;
}

std::unique_ptr<Expression> Parser::parseUIFragment() {
    consume(TokenType::GREATER, "Expected '>'");
    
    auto frag = std::make_unique<UIFragment>();
    
    // Parse children until </>
    while (true) {
        if (check(TokenType::LESS) && current + 1 < tokens.size() && tokens[current + 1].type == TokenType::SLASH) {
            break;
        }
        if (check(TokenType::LBRACE)) {
            advance(); // consume {
            auto expr = parseExpression();
            frag->children.push_back(std::make_unique<UIExpression>(std::move(expr)));
            consume(TokenType::RBRACE, "Expected '}'");
        } else if (isUIStart()) {
            frag->children.push_back(parseUIElement());
        } else if (!check(TokenType::EOF_TOKEN) && !isAtEnd()) {
            // Text content
            std::string text;
            while (!check(TokenType::LESS) && !check(TokenType::LBRACE) && !isAtEnd()) {
                text += peek().value + " ";
                advance();
            }
            if (!text.empty()) {
                frag->children.push_back(std::make_unique<UIText>(text));
            }
        } else {
            break;
        }
    }
    
    // Closing fragment
    consume(TokenType::LESS, "Expected '<'");
    consume(TokenType::SLASH, "Expected '/'");
    consume(TokenType::GREATER, "Expected '>'");
    
    return frag;
}
