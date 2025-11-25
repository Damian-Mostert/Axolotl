#include "../include/parser.h"
#include <string>

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), current(0) {}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    
    while (!isAtEnd()) {
        try {
            auto decl = parseDeclaration();
            if (decl) {
                program->declarations.push_back(std::move(decl));
            }
        } catch (const ParseError& e) {
            throw e;
        }
    }
    
    return program;
}

Token Parser::peek() const {
    if (current >= tokens.size()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens[current];
}

Token Parser::previous() const {
    if (current == 0) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    // Include location information in the message so it's visible even
    // if the exception is handled as a generic std::exception later.
    std::string locMsg = message + " (line " + std::to_string(peek().line) + ", col " + std::to_string(peek().column) + ")";
    throw ParseError(locMsg, peek());
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

std::unique_ptr<ASTNode> Parser::parseDeclaration() {
    if (check(TokenType::KW_IMPORT)) {
        return parseImportDeclaration();
    }
    if (check(TokenType::KW_FUNC)) {
        return parseFunctionDeclaration();
    }
    if (check(TokenType::KW_VAR) || check(TokenType::KW_CONST)) {
        return parseVariableDeclaration();
    }
    return parseStatement();
}

std::unique_ptr<ASTNode> Parser::parseImportDeclaration() {
    consume(TokenType::KW_IMPORT, "Expected 'import'");
    Token pathTok = consume(TokenType::STRING, "Expected string path in import");
    consume(TokenType::SEMICOLON, "Expected ';' after import");
    return std::make_unique<ImportDeclaration>(pathTok.value);
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    consume(TokenType::KW_FUNC, "Expected 'func'");
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    // Parse parameters
    std::vector<std::pair<std::string, std::string>> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name");
            consume(TokenType::COLON, "Expected ':' after parameter name");
            
            // Type can be keyword, identifier, function type, or array type
            std::string paramTypeStr;
            if (check(TokenType::LPAREN)) {
                // Function type
                paramTypeStr = parseFunctionType();
            } else if (check(TokenType::LBRACKET)) {
                // Array type
                advance();
                Token baseType;
                if (check(TokenType::KW_INT)) {
                    baseType = advance();
                } else if (check(TokenType::KW_FLOAT)) {
                    baseType = advance();
                } else if (check(TokenType::KW_STRING)) {
                    baseType = advance();
                } else if (check(TokenType::KW_BOOL)) {
                    baseType = advance();
                } else if (check(TokenType::KW_OBJECT)) {
                    baseType = advance();
                } else {
                    baseType = consume(TokenType::IDENTIFIER, "Expected array type");
                }
                consume(TokenType::RBRACKET, "Expected ']'");
                paramTypeStr = "[" + baseType.value + "]";
            } else {
                // Regular type
                Token paramType;
                if (check(TokenType::KW_INT)) {
                    paramType = advance();
                } else if (check(TokenType::KW_FLOAT)) {
                    paramType = advance();
                } else if (check(TokenType::KW_STRING)) {
                    paramType = advance();
                } else if (check(TokenType::KW_BOOL)) {
                    paramType = advance();
                } else if (check(TokenType::KW_VOID)) {
                    paramType = advance();
                } else if (check(TokenType::KW_OBJECT)) {
                    paramType = advance();
                } else {
                    paramType = consume(TokenType::IDENTIFIER, "Expected parameter type");
                }
                paramTypeStr = paramType.value;
            }
            params.push_back({paramName.value, paramTypeStr});
        } while (match({TokenType::COMMA}));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    consume(TokenType::ARROW, "Expected '->'");
    
    // Return type can be a keyword or identifier
    Token returnType;
    if (check(TokenType::KW_INT)) {
        returnType = advance();
    } else if (check(TokenType::KW_FLOAT)) {
        returnType = advance();
    } else if (check(TokenType::KW_STRING)) {
        returnType = advance();
    } else if (check(TokenType::KW_BOOL)) {
        returnType = advance();
    } else if (check(TokenType::KW_VOID)) {
        returnType = advance();
    } else {
        returnType = consume(TokenType::IDENTIFIER, "Expected return type");
    }
    
    auto body = parseBlock();
    
    auto func = std::make_unique<FunctionDeclaration>(name.value, returnType.value, std::move(body));
    func->params = params;
    return func;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (check(TokenType::KW_IF)) return parseIfStatement();
    if (check(TokenType::KW_WHILE)) return parseWhileStatement();
    if (check(TokenType::KW_FOR)) return parseForStatement();
    if (check(TokenType::KW_RETURN)) return parseReturnStatement();
    if (check(TokenType::LBRACE)) {
        auto block = parseBlock();
        return block;
    }
    
    // Try to parse assignment or expression statement
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExpressionStatement>(std::move(expr));
}

std::unique_ptr<Block> Parser::parseBlock() {
    consume(TokenType::LBRACE, "Expected '{'");
    auto block = std::make_unique<Block>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseDeclaration();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}'");
    return block;
}

std::unique_ptr<Statement> Parser::parseIfStatement() {
    consume(TokenType::KW_IF, "Expected 'if'");
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    auto thenBlock = parseBlock();
    std::unique_ptr<Block> elseBlock = nullptr;
    
    if (match({TokenType::KW_ELSE})) {
        if (check(TokenType::KW_IF)) {
            // else if
            auto elseIf = parseIfStatement();
            elseBlock = std::make_unique<Block>();
            elseBlock->statements.push_back(std::move(elseIf));
        } else {
            elseBlock = parseBlock();
        }
    }
    
    return std::make_unique<IfStatement>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

std::unique_ptr<Statement> Parser::parseWhileStatement() {
    consume(TokenType::KW_WHILE, "Expected 'while'");
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    auto body = parseBlock();
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::parseForStatement() {
    consume(TokenType::KW_FOR, "Expected 'for'");
    consume(TokenType::LPAREN, "Expected '(' after 'for'");
    
    std::unique_ptr<ASTNode> init;
    if (check(TokenType::KW_VAR)) {
        // Variable declaration but don't consume the semicolon yet
        consume(TokenType::KW_VAR, "Expected 'var'");
        
        Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
        consume(TokenType::COLON, "Expected ':' after variable name");
        
        Token type;
        if (check(TokenType::KW_INT)) {
            type = advance();
        } else if (check(TokenType::KW_FLOAT)) {
            type = advance();
        } else if (check(TokenType::KW_STRING)) {
            type = advance();
        } else if (check(TokenType::KW_BOOL)) {
            type = advance();
        } else if (check(TokenType::KW_VOID)) {
            type = advance();
        } else {
            type = consume(TokenType::IDENTIFIER, "Expected variable type");
        }
        
        std::unique_ptr<Expression> initializer = nullptr;
        if (match({TokenType::ASSIGN})) {
            initializer = parseExpression();
        }
        
        init = std::make_unique<VariableDeclaration>(name.value, type.value, std::move(initializer));
    } else if (!check(TokenType::SEMICOLON)) {
        auto expr = parseExpression();
        init = std::move(expr);
    }
    consume(TokenType::SEMICOLON, "Expected ';' after for init");
    
    auto condition = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after for condition");
    
    auto update = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after for clauses");
    
    auto body = parseBlock();
    
    return std::make_unique<ForStatement>(std::move(init), std::move(condition), 
                                          std::move(update), std::move(body));
}

std::unique_ptr<Statement> Parser::parseReturnStatement() {
    consume(TokenType::KW_RETURN, "Expected 'return'");
    std::unique_ptr<Expression> value = nullptr;
    
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return");
    return std::make_unique<ReturnStatement>(std::move(value));
}

std::unique_ptr<Statement> Parser::parseVariableDeclaration() {
    bool isConst = match({TokenType::KW_CONST});
    if (!isConst) {
        consume(TokenType::KW_VAR, "Expected 'var' or 'const'");
    }
    
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    consume(TokenType::COLON, "Expected ':' after variable name");
    
    // Type can be a keyword, identifier, array type [base_type], or function type (params)->return
    std::string typeStr;
    if (check(TokenType::LPAREN)) {
        // Could be function type like ()->void or (int,string)->bool
        // Look ahead to check if this is a function type
        size_t savedPos = current;
        bool isFunctionType = false;
        
        // Try to parse as function type
        try {
            typeStr = parseFunctionType();
            isFunctionType = true;
        } catch (...) {
            // Not a function type, backtrack
            current = savedPos;
            Token type = consume(TokenType::IDENTIFIER, "Expected variable type");
            typeStr = type.value;
        }
    } else if (check(TokenType::LBRACKET)) {
        // Array type: [int], [string], etc.
        advance(); // consume [
        Token baseType;
        if (check(TokenType::KW_INT)) {
            baseType = advance();
        } else if (check(TokenType::KW_FLOAT)) {
            baseType = advance();
        } else if (check(TokenType::KW_STRING)) {
            baseType = advance();
        } else if (check(TokenType::KW_BOOL)) {
            baseType = advance();
        } else if (check(TokenType::KW_OBJECT)) {
            baseType = advance();
        } else {
            baseType = consume(TokenType::IDENTIFIER, "Expected array element type");
        }
        consume(TokenType::RBRACKET, "Expected ']' after array type");
        typeStr = "[" + baseType.value + "]";
    } else {
        // Regular type
        Token type;
        if (check(TokenType::KW_INT)) {
            type = advance();
        } else if (check(TokenType::KW_FLOAT)) {
            type = advance();
        } else if (check(TokenType::KW_STRING)) {
            type = advance();
        } else if (check(TokenType::KW_BOOL)) {
            type = advance();
        } else if (check(TokenType::KW_VOID)) {
            type = advance();
        } else if (check(TokenType::KW_OBJECT)) {
            type = advance();
        } else {
            type = consume(TokenType::IDENTIFIER, "Expected variable type");
        }
        typeStr = type.value;
    }
    
    std::unique_ptr<Expression> initializer = nullptr;
    if (match({TokenType::ASSIGN})) {
        initializer = parseExpression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return std::make_unique<VariableDeclaration>(name.value, typeStr, std::move(initializer));
}

std::unique_ptr<Expression> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expression> Parser::parseAssignment() {
    auto expr = parseLogicalOr();
    
    if (match({TokenType::ASSIGN})) {
        auto value = parseAssignment();
        // Assignment target can be an identifier, index access, or field access
        if (auto id = dynamic_cast<Identifier*>(expr.get())) {
            return std::make_unique<Assignment>(id->name, std::move(value));
        }
        if (auto idx = dynamic_cast<IndexAccess*>(expr.get())) {
            // need to transfer ownership of object/index into IndexAssignment
            std::unique_ptr<Expression> obj = std::move(idx->object);
            std::unique_ptr<Expression> index = std::move(idx->index);
            return std::make_unique<IndexAssignment>(std::move(obj), std::move(index), std::move(value));
        }
        if (auto fld = dynamic_cast<FieldAccess*>(expr.get())) {
            std::unique_ptr<Expression> obj = std::move(fld->object);
            return std::make_unique<FieldAssignment>(std::move(obj), fld->field, std::move(value));
        }
        throw ParseError("Invalid assignment target", previous());
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    
    while (match({TokenType::LOGICAL_OR})) {
        std::string op = previous().value;
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    
    while (match({TokenType::LOGICAL_AND})) {
        std::string op = previous().value;
        auto right = parseEquality();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto expr = parseComparison();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        std::string op = previous().value;
        auto right = parseComparison();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto expr = parseTerm();
    
    while (match({TokenType::LESS, TokenType::GREATER, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL})) {
        std::string op = previous().value;
        auto right = parseTerm();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseTerm() {
    auto expr = parseFactor();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        std::string op = previous().value;
        auto right = parseFactor();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseFactor() {
    auto expr = parseUnary();
    
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        std::string op = previous().value;
        auto right = parseUnary();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match({TokenType::LOGICAL_NOT, TokenType::MINUS})) {
        std::string op = previous().value;
        auto operand = parseUnary();
        return std::make_unique<UnaryOp>(op, std::move(operand));
    }
    
    return parsePostfix();
}

std::unique_ptr<Expression> Parser::parsePostfix() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match({TokenType::LPAREN})) {
            // Function call. Always use callee-based approach to support both
            // named functions and function variables
            auto callee = std::move(expr);
            auto call = std::make_unique<FunctionCall>(std::move(callee));
            if (!check(TokenType::RPAREN)) {
                do {
                    call->args.push_back(parseExpression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expected ')' after arguments");
            expr = std::move(call);
        } else if (match({TokenType::LBRACKET})) {
            // Array/Object indexing
            auto index = parseExpression();
            consume(TokenType::RBRACKET, "Expected ']' after index");
            expr = std::make_unique<IndexAccess>(std::move(expr), std::move(index));
        } else if (match({TokenType::DOT})) {
            // Field access
            Token field = consume(TokenType::IDENTIFIER, "Expected field name after '.'");
            expr = std::make_unique<FieldAccess>(std::move(expr), field.value);
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    if (match({TokenType::KW_TRUE})) {
        return std::make_unique<BooleanLiteral>(true);
    }
    if (match({TokenType::KW_FALSE})) {
        return std::make_unique<BooleanLiteral>(false);
    }
    if (match({TokenType::INTEGER})) {
        return std::make_unique<IntegerLiteral>(std::stoi(previous().value));
    }
    if (match({TokenType::FLOAT})) {
        return std::make_unique<FloatLiteral>(std::stof(previous().value));
    }
    if (match({TokenType::STRING})) {
        return std::make_unique<StringLiteral>(previous().value);
    }
    if (match({TokenType::LBRACKET})) {
        // Array literal: [1, 2, 3]
        auto arrayLit = std::make_unique<ArrayLiteral>();
        if (!check(TokenType::RBRACKET)) {
            do {
                arrayLit->elements.push_back(parseExpression());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RBRACKET, "Expected ']' after array elements");
        return arrayLit;
    }
    if (match({TokenType::LBRACE})) {
        // Object literal: { key: value, key2: value2 }
        auto objLit = std::make_unique<ObjectLiteral>();
        if (!check(TokenType::RBRACE)) {
            do {
                Token keyToken = consume(TokenType::IDENTIFIER, "Expected property name");
                consume(TokenType::COLON, "Expected ':' after property name");
                auto value = parseExpression();
                objLit->fields.push_back({keyToken.value, std::move(value)});
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RBRACE, "Expected '}' after object fields");
        return objLit;
    }
    if (match({TokenType::KW_FUNC})) {
        // Inline function expression: func() -> returnType { body }
        consume(TokenType::LPAREN, "Expected '(' after 'func'");
        
        // Parse parameters
        std::vector<std::pair<std::string, std::string>> params;
        if (!check(TokenType::RPAREN)) {
            do {
                Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name");
                consume(TokenType::COLON, "Expected ':' after parameter name");
                
                std::string paramTypeStr;
                if (check(TokenType::LPAREN)) {
                    paramTypeStr = parseFunctionType();
                } else if (check(TokenType::LBRACKET)) {
                    advance();
                    Token baseType;
                    if (check(TokenType::KW_INT)) {
                        baseType = advance();
                    } else if (check(TokenType::KW_FLOAT)) {
                        baseType = advance();
                    } else if (check(TokenType::KW_STRING)) {
                        baseType = advance();
                    } else if (check(TokenType::KW_BOOL)) {
                        baseType = advance();
                    } else if (check(TokenType::KW_OBJECT)) {
                        baseType = advance();
                    } else {
                        baseType = consume(TokenType::IDENTIFIER, "Expected array type");
                    }
                    consume(TokenType::RBRACKET, "Expected ']'");
                    paramTypeStr = "[" + baseType.value + "]";
                } else {
                    Token paramType;
                    if (check(TokenType::KW_INT)) {
                        paramType = advance();
                    } else if (check(TokenType::KW_FLOAT)) {
                        paramType = advance();
                    } else if (check(TokenType::KW_STRING)) {
                        paramType = advance();
                    } else if (check(TokenType::KW_BOOL)) {
                        paramType = advance();
                    } else if (check(TokenType::KW_VOID)) {
                        paramType = advance();
                    } else if (check(TokenType::KW_OBJECT)) {
                        paramType = advance();
                    } else {
                        paramType = consume(TokenType::IDENTIFIER, "Expected parameter type");
                    }
                    paramTypeStr = paramType.value;
                }
                params.push_back({paramName.value, paramTypeStr});
            } while (match({TokenType::COMMA}));
        }
        
        consume(TokenType::RPAREN, "Expected ')' after parameters");
        consume(TokenType::ARROW, "Expected '->'");
        
        // Return type
        Token returnType;
        if (check(TokenType::KW_INT)) {
            returnType = advance();
        } else if (check(TokenType::KW_FLOAT)) {
            returnType = advance();
        } else if (check(TokenType::KW_STRING)) {
            returnType = advance();
        } else if (check(TokenType::KW_BOOL)) {
            returnType = advance();
        } else if (check(TokenType::KW_VOID)) {
            returnType = advance();
        } else if (check(TokenType::KW_OBJECT)) {
            returnType = advance();
        } else {
            returnType = consume(TokenType::IDENTIFIER, "Expected return type");
        }
        
        auto body = parseBlock();
        
        // Create a FunctionExpression
        auto funcExpr = std::make_unique<FunctionExpression>(returnType.value, std::move(body));
        funcExpr->params = params;
        return funcExpr;
    }
    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<Identifier>(previous().value);
    }
    if (match({TokenType::LPAREN})) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    throw ParseError("Unexpected token: " + peek().value, peek());
}

std::string Parser::parseFunctionType() {
    // Parse (param1Type, param2Type, ...)->returnType
    consume(TokenType::LPAREN, "Expected '(' for function type");
    
    std::string paramTypes = "";
    if (!check(TokenType::RPAREN)) {
        do {
            Token paramType;
            if (check(TokenType::KW_INT)) {
                paramType = advance();
            } else if (check(TokenType::KW_FLOAT)) {
                paramType = advance();
            } else if (check(TokenType::KW_STRING)) {
                paramType = advance();
            } else if (check(TokenType::KW_BOOL)) {
                paramType = advance();
            } else if (check(TokenType::KW_VOID)) {
                paramType = advance();
            } else if (check(TokenType::KW_OBJECT)) {
                paramType = advance();
            } else if (check(TokenType::LBRACKET)) {
                // Array type in function signature
                advance();
                Token baseType;
                if (check(TokenType::KW_INT)) {
                    baseType = advance();
                } else if (check(TokenType::KW_FLOAT)) {
                    baseType = advance();
                } else if (check(TokenType::KW_STRING)) {
                    baseType = advance();
                } else if (check(TokenType::KW_BOOL)) {
                    baseType = advance();
                } else if (check(TokenType::KW_OBJECT)) {
                    baseType = advance();
                } else {
                    baseType = consume(TokenType::IDENTIFIER, "Expected array type");
                }
                consume(TokenType::RBRACKET, "Expected ']'");
                paramTypes += "[" + baseType.value + "]";
            } else {
                paramType = consume(TokenType::IDENTIFIER, "Expected parameter type");
            }
            if (paramType.type != TokenType::UNKNOWN) {
                if (!paramTypes.empty()) paramTypes += ",";
                paramTypes += paramType.value;
            }
        } while (match({TokenType::COMMA}));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after function parameter types");
    consume(TokenType::ARROW, "Expected '->' in function type");
    
    Token returnType;
    if (check(TokenType::KW_INT)) {
        returnType = advance();
    } else if (check(TokenType::KW_FLOAT)) {
        returnType = advance();
    } else if (check(TokenType::KW_STRING)) {
        returnType = advance();
    } else if (check(TokenType::KW_BOOL)) {
        returnType = advance();
    } else if (check(TokenType::KW_VOID)) {
        returnType = advance();
    } else if (check(TokenType::KW_OBJECT)) {
        returnType = advance();
    } else {
        returnType = consume(TokenType::IDENTIFIER, "Expected return type");
    }
    
    return "(" + paramTypes + ")->" + returnType.value;
}
