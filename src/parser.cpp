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
    if (check(TokenType::KW_USE)) {
        return parseUseDeclaration();
    }
    if (check(TokenType::KW_EXPORT)) {
        return parseExportDeclaration();
    }
    if (check(TokenType::IDENTIFIER) && peek().value == "type") {
        return parseTypeDeclaration();
    }
    if (check(TokenType::KW_FUNC)) {
        return parseFunctionDeclaration();
    }
    if (check(TokenType::KW_PROGRAM)) {
        return parseProgramDeclaration();
    }
    if (check(TokenType::KW_VAR) || check(TokenType::KW_CONST)) {
        return parseVariableDeclaration();
    }
    return parseStatement();
}

std::unique_ptr<ASTNode> Parser::parseImportDeclaration() {
    consume(TokenType::KW_IMPORT, "Expected 'import'");
    
    auto importDecl = std::make_unique<ImportDeclaration>("");
    
    // Handle different import patterns:
    // import "path";  (simple import)
    // import x from "path";  (default import)
    // import {x, y} from "path";  (named imports)
    // import x, {y, z} from "path";  (mixed import)
    
    if (check(TokenType::STRING)) {
        // Simple import: import "path";
        Token pathTok = advance();
        importDecl->path = pathTok.value;
    } else if (check(TokenType::LBRACE)) {
        // Named imports: import {x, y} from "path";
        advance(); // consume {
        do {
            Token name = consume(TokenType::IDENTIFIER, "Expected import name");
            importDecl->namedImports.push_back(name.value);
        } while (match({TokenType::COMMA}));
        consume(TokenType::RBRACE, "Expected '}' after named imports");
        Token fromToken = consume(TokenType::IDENTIFIER, "Expected 'from'");
        if (fromToken.value != "from") {
            throw ParseError("Expected 'from' keyword, got '" + fromToken.value + "'", fromToken);
        }
        Token pathTok = consume(TokenType::STRING, "Expected string path after 'from'");
        importDecl->path = pathTok.value;
    } else if (check(TokenType::IDENTIFIER)) {
        // Default or mixed import
        Token defaultName = advance();
        importDecl->defaultImport = defaultName.value;
        
        if (match({TokenType::COMMA})) {
            // Mixed import: import x, {y, z} from "path";
            consume(TokenType::LBRACE, "Expected '{' after comma in mixed import");
            do {
                Token name = consume(TokenType::IDENTIFIER, "Expected import name");
                importDecl->namedImports.push_back(name.value);
            } while (match({TokenType::COMMA}));
            consume(TokenType::RBRACE, "Expected '}' after named imports");
        }
        
        Token fromToken2 = consume(TokenType::IDENTIFIER, "Expected 'from'");
        if (fromToken2.value != "from") {
            throw ParseError("Expected 'from' keyword, got '" + fromToken2.value + "'", fromToken2);
        }
        Token pathTok = consume(TokenType::STRING, "Expected string path after 'from'");
        importDecl->path = pathTok.value;
    } else {
        throw ParseError("Expected import pattern", peek());
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after import");
    return importDecl;
}

std::unique_ptr<ASTNode> Parser::parseUseDeclaration() {
    consume(TokenType::KW_USE, "Expected 'use'");
    
    // Use statements only support simple path imports: use "path";
    Token pathTok = consume(TokenType::STRING, "Expected string path after 'use'");
    auto useDecl = std::make_unique<UseDeclaration>(pathTok.value);
    
    consume(TokenType::SEMICOLON, "Expected ';' after use");
    return useDecl;
}

std::unique_ptr<ASTNode> Parser::parseExportDeclaration() {
    consume(TokenType::KW_EXPORT, "Expected 'export'");
    
    // Handle different export patterns:
    // export func name() { ... }  (export function)
    // export var x = 5;  (export variable)
    // export {x, y};  (named exports)
    // export default func() { ... }  (default export)
    
    if (check(TokenType::IDENTIFIER) && peek().value == "default") {
        // Default export
        advance(); // consume "default"
        auto declaration = parseDeclaration();
        return std::make_unique<ExportDeclaration>(std::move(declaration), true);
    } else if (check(TokenType::LBRACE)) {
        // Named exports: export {x, y};
        advance(); // consume {
        std::vector<std::string> exports;
        do {
            Token name = consume(TokenType::IDENTIFIER, "Expected export name");
            exports.push_back(name.value);
        } while (match({TokenType::COMMA}));
        consume(TokenType::RBRACE, "Expected '}' after export names");
        consume(TokenType::SEMICOLON, "Expected ';' after export");
        return std::make_unique<ExportDeclaration>(exports);
    } else {
        // Export declaration: export func name() { ... } or export var x = 5;
        auto declaration = parseDeclaration();
        return std::make_unique<ExportDeclaration>(std::move(declaration));
    }
}

std::unique_ptr<TypeDeclaration> Parser::parseTypeDeclaration() {
    consume(TokenType::IDENTIFIER, "Expected 'type'"); // Should be 'type' identifier
    Token name = consume(TokenType::IDENTIFIER, "Expected type name");
    consume(TokenType::ASSIGN, "Expected '=' after type name");
    
    // Parse the type specification - this can be complex
    std::string typeSpec = parseComplexTypeSpec();
    
    consume(TokenType::SEMICOLON, "Expected ';' after type declaration");
    return std::make_unique<TypeDeclaration>(name.value, typeSpec);
}

std::string Parser::parseComplexTypeSpec() {
    std::string result;
    
    // Handle different type specifications
    if (check(TokenType::LBRACE)) {
        // Object type: { field1: type1, field2: type2 }
        advance(); // consume {
        result += "{";
        
        if (!check(TokenType::RBRACE)) {
            do {
                Token fieldName = consume(TokenType::IDENTIFIER, "Expected field name");
                consume(TokenType::COLON, "Expected ':' after field name");
                
                // Parse field type - could be simple type, array type, object type, or union type
                std::string fieldType;
                if (check(TokenType::LBRACKET)) {
                    // Array type in object field: {field: [string|int]}
                    fieldType = parseComplexTypeSpec();
                } else if (check(TokenType::LBRACE)) {
                    // Nested object type in object field: {field: {nested: type}}
                    fieldType = parseComplexTypeSpec();
                } else {
                    // Simple type or union type
                    fieldType = parseSimpleTypeSpec();
                    
                    // Handle union types in object fields: {field: type1|type2}
                    while (match({TokenType::PIPE})) {
                        fieldType += "|";
                        std::string nextType = parseSimpleTypeSpec();
                        fieldType += nextType;
                    }
                }
                
                result += fieldName.value + ":" + fieldType;
                
                if (check(TokenType::COMMA)) {
                    result += ",";
                }
            } while (match({TokenType::COMMA}));
        }
        
        consume(TokenType::RBRACE, "Expected '}' after object type");
        result += "}";
    } else if (check(TokenType::LBRACKET)) {
        // Array type: [elementType] or specific array: [type1, type2, ...] or [{field:type}, {field:type}]
        advance(); // consume [
        result += "[";
        
        if (check(TokenType::LBRACE)) {
            // Specific array elements with object types
            do {
                std::string elementType = parseComplexTypeSpec();
                result += elementType;
                
                if (check(TokenType::COMMA)) {
                    result += ",";
                }
            } while (match({TokenType::COMMA}));
        } else {
            // Could be regular array type [elementType] or specific elements [type1, type2, ...]
            std::string firstType = parseSimpleTypeSpec();
            result += firstType;
            
            // Handle union types inside arrays: [string|int]
            while (match({TokenType::PIPE})) {
                result += "|";
                std::string nextType = parseSimpleTypeSpec();
                result += nextType;
            }
            
            // Check if there are more comma-separated types (specific array elements)
            while (match({TokenType::COMMA})) {
                result += ",";
                std::string nextType = parseSimpleTypeSpec();
                result += nextType;
                
                // Handle union types for each comma-separated element
                while (match({TokenType::PIPE})) {
                    result += "|";
                    std::string moreType = parseSimpleTypeSpec();
                    result += moreType;
                }
            }
        }
        
        consume(TokenType::RBRACKET, "Expected ']' after array type");
        result += "]";
    } else {
        // Simple type or union type
        result = parseSimpleTypeSpec();
    }
    
    // Handle union types with |
    while (match({TokenType::PIPE})) {
        result += "|";
        if (check(TokenType::LBRACE) || check(TokenType::LBRACKET)) {
            result += parseComplexTypeSpec();
        } else {
            result += parseSimpleTypeSpec();
        }
    }
    
    return result;
}

std::string Parser::parseSimpleTypeSpec() {
    if (check(TokenType::STRING)) {
        // String literal type
        Token str = advance();
        return "\"" + str.value + "\"";
    } else if (check(TokenType::INTEGER)) {
        // Integer literal type
        Token num = advance();
        return num.value;
    } else if (check(TokenType::KW_TRUE) || check(TokenType::KW_FALSE)) {
        // Boolean literal type
        Token bool_val = advance();
        return bool_val.value;
    } else if (check(TokenType::KW_INT)) {
        advance();
        return "int";
    } else if (check(TokenType::KW_FLOAT)) {
        advance();
        return "float";
    } else if (check(TokenType::KW_STRING)) {
        advance();
        return "string";
    } else if (check(TokenType::KW_BOOL)) {
        advance();
        return "bool";
    } else if (check(TokenType::KW_OBJECT)) {
        advance();
        return "object";
    } else if (check(TokenType::KW_ANY)) {
        advance();
        return "any";
    } else if (check(TokenType::KW_VOID)) {
        advance();
        return "void";
    } else if (check(TokenType::IDENTIFIER)) {
        // Custom type reference
        Token type = advance();
        return type.value;
    } else {
        throw ParseError("Expected type specification", peek());
    }
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
                paramTypeStr = parseArrayType();
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
                } else if (check(TokenType::KW_ANY)) {
                    paramType = advance();
                } else {
                    paramType = consume(TokenType::IDENTIFIER, "Expected parameter type");
                }
                paramTypeStr = paramType.value;
                // support union types in parameter types
                while (match({TokenType::PIPE})) {
                    Token moreType;
                    if (check(TokenType::KW_INT)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_FLOAT)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_STRING)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_BOOL)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_VOID)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_OBJECT)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_ANY)) {
                        moreType = advance();
                    } else {
                        moreType = consume(TokenType::IDENTIFIER, "Expected type after '|'");
                    }
                    paramTypeStr += "|" + moreType.value;
                }
            }
            params.push_back({paramName.value, paramTypeStr});
        } while (match({TokenType::COMMA}));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    consume(TokenType::ARROW, "Expected '->'");
    
    // Return type can be a keyword, identifier, or array type
    std::string returnTypeStr;
    if (check(TokenType::LBRACKET)) {
        // Array return type: [int], [string], etc.
        returnTypeStr = parseArrayType();
    } else {
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
        } else if (check(TokenType::KW_ANY)) {
            returnType = advance();
        } else if (check(TokenType::KW_OBJECT)) {
            returnType = advance();
        } else {
            returnType = consume(TokenType::IDENTIFIER, "Expected return type");
        }
        returnTypeStr = returnType.value;
    }
    
    auto body = parseBlock();
    
    auto func = std::make_unique<FunctionDeclaration>(name.value, returnTypeStr, std::move(body));
    func->params = params;
    return func;
}

std::unique_ptr<ProgramDeclaration> Parser::parseProgramDeclaration() {
    consume(TokenType::KW_PROGRAM, "Expected 'program'");
    Token name = consume(TokenType::IDENTIFIER, "Expected program name");
    consume(TokenType::LPAREN, "Expected '(' after program name");
    
    // Parse parameters (optional)
    std::vector<std::pair<std::string, std::string>> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name");
            consume(TokenType::COLON, "Expected ':' after parameter name");
            
            Token paramType;
            if (check(TokenType::KW_INT)) {
                paramType = advance();
            } else if (check(TokenType::KW_FLOAT)) {
                paramType = advance();
            } else if (check(TokenType::KW_STRING)) {
                paramType = advance();
            } else if (check(TokenType::KW_BOOL)) {
                paramType = advance();
            } else if (check(TokenType::KW_ANY)) {
                paramType = advance();
            } else {
                paramType = consume(TokenType::IDENTIFIER, "Expected parameter type");
            }
            params.push_back({paramName.value, paramType.value});
        } while (match({TokenType::COMMA}));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    auto body = parseBlock();
    
    auto program = std::make_unique<ProgramDeclaration>(name.value, std::move(body));
    program->params = params;
    return program;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (check(TokenType::KW_IF)) return parseIfStatement();
    if (check(TokenType::KW_WHILE)) return parseWhileStatement();
    if (check(TokenType::KW_FOR)) return parseForStatement();
    if (check(TokenType::KW_RETURN)) return parseReturnStatement();
    if (check(TokenType::KW_TRY)) return parseTryStatement();
    if (check(TokenType::KW_THROW)) return parseThrowStatement();
    if (check(TokenType::KW_BREAK)) return parseBreakStatement();
    if (check(TokenType::KW_CONTINUE)) return parseContinueStatement();
    if (check(TokenType::KW_SWITCH)) return parseSwitchStatement();
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

std::unique_ptr<Statement> Parser::parseThrowStatement() {
    consume(TokenType::KW_THROW, "Expected 'throw'");
    auto expression = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after throw");
    return std::make_unique<ThrowStatement>(std::move(expression));
}

std::unique_ptr<Statement> Parser::parseBreakStatement() {
    consume(TokenType::KW_BREAK, "Expected 'break'");
    consume(TokenType::SEMICOLON, "Expected ';' after break");
    return std::make_unique<BreakStatement>();
}

std::unique_ptr<Statement> Parser::parseContinueStatement() {
    consume(TokenType::KW_CONTINUE, "Expected 'continue'");
    consume(TokenType::SEMICOLON, "Expected ';' after continue");
    return std::make_unique<ContinueStatement>();
}

std::unique_ptr<Statement> Parser::parseSwitchStatement() {
    consume(TokenType::KW_SWITCH, "Expected 'switch'");
    consume(TokenType::LPAREN, "Expected '(' after 'switch'");
    auto discriminant = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after switch expression");
    consume(TokenType::LBRACE, "Expected '{' after switch condition");
    
    auto switchStmt = std::make_unique<SwitchStatement>(std::move(discriminant));
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (check(TokenType::KW_CASE)) {
            advance(); // consume 'case'
            auto caseValue = parseExpression();
            consume(TokenType::COLON, "Expected ':' after case value");
            
            auto caseClause = std::make_unique<CaseClause>(std::move(caseValue), false);
            
            // Parse statements until next case/default/end of switch
            while (!check(TokenType::KW_CASE) && !check(TokenType::KW_DEFAULT) && 
                   !check(TokenType::RBRACE) && !isAtEnd()) {
                auto stmt = parseDeclaration();
                if (stmt) {
                    caseClause->statements.push_back(std::move(stmt));
                }
            }
            
            switchStmt->cases.push_back(std::move(caseClause));
        } else if (check(TokenType::KW_DEFAULT)) {
            advance(); // consume 'default'
            consume(TokenType::COLON, "Expected ':' after 'default'");
            
            auto defaultClause = std::make_unique<CaseClause>(nullptr, true);
            
            // Parse statements until next case/end of switch
            while (!check(TokenType::KW_CASE) && !check(TokenType::RBRACE) && !isAtEnd()) {
                auto stmt = parseDeclaration();
                if (stmt) {
                    defaultClause->statements.push_back(std::move(stmt));
                }
            }
            
            switchStmt->cases.push_back(std::move(defaultClause));
        } else {
            throw ParseError("Expected 'case' or 'default' in switch statement", peek());
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after switch body");
    return switchStmt;
}

std::unique_ptr<Statement> Parser::parseTryStatement() {
    consume(TokenType::KW_TRY, "Expected 'try'");
    auto tryBlock = parseBlock();
    
    std::string catchVariable;
    std::unique_ptr<Block> catchBlock = nullptr;
    std::unique_ptr<Block> finallyBlock = nullptr;
    
    if (match({TokenType::KW_CATCH})) {
        consume(TokenType::LPAREN, "Expected '(' after 'catch'");
        Token varToken = consume(TokenType::IDENTIFIER, "Expected variable name in catch");
        catchVariable = varToken.value;
        consume(TokenType::RPAREN, "Expected ')' after catch variable");
        catchBlock = parseBlock();
    }
    
    if (match({TokenType::KW_FINALLY})) {
        finallyBlock = parseBlock();
    }
    
    return std::make_unique<TryStatement>(std::move(tryBlock), catchVariable, 
                                          std::move(catchBlock), std::move(finallyBlock));
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
        // Array type: [int], [string], [[int]], etc.
        typeStr = parseArrayType();
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
        } else if (check(TokenType::KW_ANY)) {
            type = advance();
        } else if (check(TokenType::KW_FUNC)) {
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
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryOp>(std::move(expr), BinaryOperator::LOGICAL_OR, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    
    while (match({TokenType::LOGICAL_AND})) {
        auto right = parseEquality();
        expr = std::make_unique<BinaryOp>(std::move(expr), BinaryOperator::LOGICAL_AND, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto expr = parseComparison();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        TokenType opType = previous().type;
        BinaryOperator op = (opType == TokenType::EQUAL) ? BinaryOperator::EQUAL : BinaryOperator::NOT_EQUAL;
        auto right = parseComparison();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto expr = parseTerm();
    
    while (match({TokenType::LESS, TokenType::GREATER, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL})) {
        TokenType opType = previous().type;
        BinaryOperator op;
        switch (opType) {
            case TokenType::LESS: op = BinaryOperator::LESS; break;
            case TokenType::GREATER: op = BinaryOperator::GREATER; break;
            case TokenType::LESS_EQUAL: op = BinaryOperator::LESS_EQUAL; break;
            case TokenType::GREATER_EQUAL: op = BinaryOperator::GREATER_EQUAL; break;
            default: throw ParseError("Invalid comparison operator", previous());
        }
        auto right = parseTerm();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseTerm() {
    auto expr = parseFactor();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        TokenType opType = previous().type;
        BinaryOperator op = (opType == TokenType::PLUS) ? BinaryOperator::ADD : BinaryOperator::SUBTRACT;
        auto right = parseFactor();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseFactor() {
    auto expr = parseUnary();
    
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        TokenType opType = previous().type;
        BinaryOperator op;
        switch (opType) {
            case TokenType::STAR: op = BinaryOperator::MULTIPLY; break;
            case TokenType::SLASH: op = BinaryOperator::DIVIDE; break;
            case TokenType::PERCENT: op = BinaryOperator::MODULO; break;
            default: throw ParseError("Invalid factor operator", previous());
        }
        auto right = parseUnary();
        expr = std::make_unique<BinaryOp>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match({TokenType::LOGICAL_NOT, TokenType::MINUS, TokenType::KW_TYPEOF})) {
        TokenType opType = previous().type;
        UnaryOperator op;
        if (opType == TokenType::LOGICAL_NOT) {
            op = UnaryOperator::LOGICAL_NOT;
        } else if (opType == TokenType::MINUS) {
            op = UnaryOperator::NEGATE;
        } else if (opType == TokenType::KW_TYPEOF) {
            op = UnaryOperator::TYPEOF;
        }
        auto operand = parseUnary();
        return std::make_unique<UnaryOp>(op, std::move(operand));
    }
    
    if (match({TokenType::KW_AWAIT})) {
        auto expr = parseUnary();
        return std::make_unique<AwaitExpression>(std::move(expr));
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
                    // Support union types inside arrays: [string|int]
                    while (match({TokenType::PIPE})) {
                        Token moreType;
                        if (check(TokenType::KW_INT)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_FLOAT)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_STRING)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_BOOL)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_OBJECT)) {
                            moreType = advance();
                        } else {
                            moreType = consume(TokenType::IDENTIFIER, "Expected array element type after '|'");
                        }
                        baseType.value += "|" + moreType.value;
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
                    // support union types in parameter types
                    while (match({TokenType::PIPE})) {
                        Token moreType;
                        if (check(TokenType::KW_INT)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_FLOAT)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_STRING)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_BOOL)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_VOID)) {
                            moreType = advance();
                        } else if (check(TokenType::KW_OBJECT)) {
                            moreType = advance();
                        } else {
                            moreType = consume(TokenType::IDENTIFIER, "Expected type after '|'");
                        }
                        paramTypeStr += "|" + moreType.value;
                    }
                }
                params.push_back({paramName.value, paramTypeStr});
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RPAREN, "Expected ')' after parameters");
        consume(TokenType::ARROW, "Expected '->'");

        // Return type can be array type or simple type
        std::string returnTypeStr;
        if (check(TokenType::LBRACKET)) {
            returnTypeStr = parseArrayType();
        } else {
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
            returnTypeStr = returnType.value;
        }

        auto body = parseBlock();

        // Create a FunctionExpression
        auto funcExpr = std::make_unique<FunctionExpression>(returnTypeStr, std::move(body));
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
                // Support union types inside arrays in function signatures: [string|int]
                while (match({TokenType::PIPE})) {
                    Token moreType;
                    if (check(TokenType::KW_INT)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_FLOAT)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_STRING)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_BOOL)) {
                        moreType = advance();
                    } else if (check(TokenType::KW_OBJECT)) {
                        moreType = advance();
                    } else {
                        moreType = consume(TokenType::IDENTIFIER, "Expected array element type after '|'");
                    }
                    baseType.value += "|" + moreType.value;
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

std::string Parser::parseArrayType() {
    consume(TokenType::LBRACKET, "Expected '['");
    
    std::string elementType;
    if (check(TokenType::LBRACKET)) {
        // Nested array: [[int]], [[[string]]], etc.
        elementType = parseArrayType();
    } else {
        // Base type
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
        } else if (check(TokenType::KW_ANY)) {
            baseType = advance();
        } else {
            baseType = consume(TokenType::IDENTIFIER, "Expected array element type");
        }
        elementType = baseType.value;
        
        // Support union types inside arrays: [string|int]
        while (match({TokenType::PIPE})) {
            Token moreType;
            if (check(TokenType::KW_INT)) {
                moreType = advance();
            } else if (check(TokenType::KW_FLOAT)) {
                moreType = advance();
            } else if (check(TokenType::KW_STRING)) {
                moreType = advance();
            } else if (check(TokenType::KW_BOOL)) {
                moreType = advance();
            } else if (check(TokenType::KW_OBJECT)) {
                moreType = advance();
            } else if (check(TokenType::KW_ANY)) {
                moreType = advance();
            } else {
                moreType = consume(TokenType::IDENTIFIER, "Expected array element type after '|'");
            }
            elementType += "|" + moreType.value;
        }
    }
    
    consume(TokenType::RBRACKET, "Expected ']' after array type");
    return "[" + elementType + "]"; 
}
