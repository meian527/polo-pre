#include "parser.h"

#include <cmath>

#include "common.h"
#include <stdexcept>

#include "typechecker.h"

Parser::Parser(Lexer& lexer) : lexer(lexer) {
    advance();
}

std::shared_ptr<ProgramNode> Parser::parseProgram() {
    auto line = currentToken.line, col = currentToken.column;
    std::vector<ASTNodePtr> program;
    
    while (currentToken.type != TokenType::EOF_TOKEN) {
        ASTNodePtr statement = parseStatement();
        if (statement) {
            program.push_back(statement);
        }
    }
    
    return std::make_shared<ProgramNode>(line, col, program);
}

void Parser::advance() {
    currentToken = lexer.getNextToken();
}

void Parser::expect(const TokenType type) {
    if (currentToken.type != type)
        THROW_ERROR("Unexpected token: " + currentToken.value, currentToken.line, currentToken.column);

    advance();
}

std::shared_ptr<Type> Parser::parseType() {
    bool is_ptr = false, is_arr = false;
    int64_t arr_size = 0;
    if (currentToken.type == TokenType::MULTIPLY) {
        advance();
        is_ptr = true;
    }
    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected type identifier", currentToken.line, currentToken.column);
    }

    std::string typeName = currentToken.value;
    advance();
    if (currentToken.type == TokenType::LBRACKET) {
        advance();
        if (currentToken.type == TokenType::NUM) {
            arr_size = std::stoll(currentToken.value);
            advance();
        }
        expect(TokenType::RBRACKET);
        is_arr = true;
    }
    TypeKind kind = Type::fromString(typeName);
    auto result = std::make_shared<Type>(kind, is_ptr);;
    result->is_arr = is_arr;
    return result;
}

ASTNodePtr Parser::parseStatement() {
    switch (currentToken.type) {
        case TokenType::LET:
            return parseVariableDecl();
        case TokenType::FN:
            return parseFunction();
        case TokenType::RETURN:
            return parseReturnStmt();
        case TokenType::IF:
            return parseIfStmt();
        case TokenType::FOR:
            return parseForStmt();
        case TokenType::BREAK:
            return parseBreakStmt();
        case TokenType::CONTINUE:
            return parseContinueStmt();
        case TokenType::PUB:

        case TokenType::GRID:
            return parseMacroDecl();
        case TokenType::STRUCT:
            return parseStructDecl();
        case TokenType::IMPL:
            return parseImplDecl();
        default:
            ASTNodePtr expr = parseAssignment();
            expect(TokenType::SEMICOLON);
            return expr;
    }
}

std::shared_ptr<VariableDeclNode> Parser::parseVariableDecl() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::LET);

    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected identifier after let", currentToken.line, currentToken.column);
    }

    std::string name = currentToken.value;
    advance();
    
    expect(TokenType::COLON);
    
    auto type = parseType();
    
    expect(TokenType::ASSIGN);
    
    ASTNodePtr initializer = parseExpression();
    expect(TokenType::SEMICOLON);
    
    return std::make_shared<VariableDeclNode>(line, col, name, type, initializer);
}
std::vector<Parameter> Parser::parseFunctionArgs() {
    expect(TokenType::LPAREN);

    std::vector<Parameter> parameters;
    if (currentToken.type != TokenType::RPAREN) {
        do {
            if (currentToken.type != TokenType::IDENTIFIER) {
                THROW_ERROR("Expected parameter name", currentToken.line, currentToken.column);
            }

            std::string paramName = currentToken.value;
            advance();

            expect(TokenType::COLON);

            auto paramType = parseType();

            parameters.push_back({paramName, paramType});
        } while (currentToken.type == TokenType::COMMA && (advance(), true));
    }

    expect(TokenType::RPAREN);
    return parameters;
}
std::shared_ptr<FunctionNode> Parser::parseFunction() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::FN);

    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected function name", currentToken.line, currentToken.column);
    }

    std::string name = currentToken.value;
    advance();

    auto parameters = parseFunctionArgs();
    
    auto returnType = std::make_shared<Type>(TypeKind::I32);
    if (currentToken.type == TokenType::ARROW) {
        advance();
        returnType = parseType();
    }

    std::vector<ASTNodePtr> body;
    if (currentToken.type == TokenType::SEMICOLON) {
        advance();
        auto f = std::make_shared<FunctionNode>(line, col, name, parameters, returnType, body);
        f->has_body = false;
        return f;
    }
    expect(TokenType::LBRACE);
    

    while (currentToken.type != TokenType::RBRACE && currentToken.type != TokenType::EOF_TOKEN) {
        ASTNodePtr statement = parseStatement();
        if (statement) {
            body.push_back(statement);
        }
    }
    
    expect(TokenType::RBRACE);
    
    return std::make_shared<FunctionNode>(line, col, name, parameters, returnType, body);
}

std::shared_ptr<ReturnStmtNode> Parser::parseReturnStmt() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::RETURN);
    
    ASTNodePtr expr = nullptr;
    if (currentToken.type != TokenType::SEMICOLON) {
        expr = parseExpression();
    }
    expect(TokenType::SEMICOLON);

    return std::make_shared<ReturnStmtNode>(line, col, expr);
}


BinaryOpType Parser::tokenType2opType(TokenType type) const {
    BinaryOpType op;
    switch (type) {
    case TokenType::PLUS:
        op = BinaryOpType::ADD;
        break;
    case TokenType::MINUS:
        op = BinaryOpType::SUB;
        break;
    case TokenType::MULTIPLY:
        op = BinaryOpType::MUL;
        break;
    case TokenType::DIVIDE:
        op = BinaryOpType::DIV;
        break;
    case TokenType::MOD:
        op = BinaryOpType::MOD;
        break;
    case TokenType::EQ:
        op = BinaryOpType::EQ;
        break;
    case TokenType::NE:
        op = BinaryOpType::NE;
        break;
    case TokenType::LT:
        op = BinaryOpType::LT;
        break;
    case TokenType::GT:
        op = BinaryOpType::GT;
        break;
    case TokenType::LE:
        op = BinaryOpType::LE;
        break;
    case TokenType::GE:
        op = BinaryOpType::GE;
        break;
    case TokenType::AND:
        op = BinaryOpType::AND;
        break;
    case TokenType::OR:
        op = BinaryOpType::OR;
        break;
    default:
        THROW_ERROR("Unknown binary operator", currentToken.line, currentToken.column);

    }
    return op;
}

#define PARSE_BASE_BINOP(last, logic,  ...) \
    auto line = currentToken.line, col = currentToken.column;\
    ASTNodePtr node = last();   \
    logic (__VA_ARGS__) {    \
        auto op = tokenType2opType(currentToken.type);\
        advance();\
        line = currentToken.line, col = currentToken.column;\
        node = std::make_shared<BinaryOpNode>(line, col, std::move(node), op, last());         \
    }                  \
    if (currentToken.type == TokenType::AS) {\
        advance();\
        auto n = std::static_pointer_cast<ExprNode>(node);\
        n->set_ret_type(parseType());\
        node = n;\
    }\
    return node;
ASTNodePtr Parser::parseExpression() {
    return parseBoolAndOr();
}
ASTNodePtr Parser::parseBoolAndOr() {
    PARSE_BASE_BINOP(parseBoolOper, while, currentToken.type == TokenType::AND || currentToken.type == TokenType::OR)
}
ASTNodePtr Parser::parseBoolOper() {
    PARSE_BASE_BINOP(parseAddSub, while,
        currentToken.type == TokenType::EQ ||
        currentToken.type == TokenType::NE ||
        currentToken.type == TokenType::LT ||
        currentToken.type == TokenType::GT ||
        currentToken.type == TokenType::LE ||
        currentToken.type == TokenType::GE )
}
ASTNodePtr Parser::parseAssignment() {
    auto line = currentToken.line, col = currentToken.column;
    ASTNodePtr left = parseExpression();
    
    if (currentToken.type == TokenType::ASSIGN) {
        advance();
        ASTNodePtr right = parseAssignment();

        if (left->type == NodeType::IDENTIFIER) {
            return std::make_shared<AssignmentNode>(line, col, std::static_pointer_cast<IdentifierNode>(left)->name, right);
        } else {
            THROW_ERROR("Left side of assignment must be an identifier", currentToken.line, currentToken.column);
        }
    }
    
    return left;
}


ASTNodePtr Parser::parseAddSub() {
    PARSE_BASE_BINOP(parseMulDivMod, while, currentToken.type == TokenType::PLUS || currentToken.type == TokenType::MINUS)
}
ASTNodePtr Parser::parseMulDivMod() {
    PARSE_BASE_BINOP(parsePrimary, while, currentToken.type == TokenType::MULTIPLY || currentToken.type == TokenType::DIVIDE || currentToken.type == TokenType::MOD)
}

ASTNodePtr Parser::parsePrimary() {
    size_t line = currentToken.line, col = currentToken.column;
    switch (currentToken.type) {
        case TokenType::NUM: {
            return parseNumber();
        }
        case TokenType::FLOAT: {
            return parseFloat();
        }
        case TokenType::TRUE:
        case TokenType::FALSE: {
            return parseBoolean();
        }
        case TokenType::STRING: {
            return parseString();
        }
        case TokenType::IDENTIFIER: {
            if (lexer.peek().type == TokenType::LPAREN) {
                return parseFunctionCall();
            } else if (lexer.peek().type == TokenType::NOT) {
                return parseMacroCall();
            }
            else {
                return parseIdentifier();
            }
        }
        case TokenType::MINUS: {
            advance();
            auto result = std::make_shared<UnaryOpNode>(line, col, UnaryOpType::Minus, parseExpression());
            result->set_ret_type(std::static_pointer_cast<ExprNode>(result->expr)->ret_type);
            return result;
        }
        case TokenType::REF: {
            advance();
            return std::make_shared<UnaryOpNode>(line, col, UnaryOpType::Addr, parseExpression());
        }
        case TokenType::LPAREN: {
            advance();
            ASTNodePtr expr = parseExpression();
            expect(TokenType::RPAREN);
            return expr;
        }
        default:
            THROW_ERROR("Unexpected token: " + currentToken.value, currentToken.line, currentToken.column);
    }
    return nullptr;
}

std::shared_ptr<FunctionCallNode> Parser::parseFunctionCall() {
    auto line = currentToken.line, col = currentToken.column;
    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected function name", currentToken.line, currentToken.column);
    }

    std::string name = currentToken.value;
    advance();
    expect(TokenType::LPAREN);
    
    std::vector<ASTNodePtr> arguments;
    if (currentToken.type != TokenType::RPAREN) {
        do {
            arguments.push_back(parseExpression());
        } while (currentToken.type == TokenType::COMMA && (advance(), true));
    }
    
    expect(TokenType::RPAREN);
    
    return std::make_shared<FunctionCallNode>(line, col, name, arguments);
}

std::shared_ptr<IdentifierNode> Parser::parseIdentifier() {
    auto line = currentToken.line, col = currentToken.column;
    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected identifier", currentToken.line, currentToken.column);
    }

    std::string name = currentToken.value;
    advance();

    return std::make_shared<IdentifierNode>(line, col, name);
}

std::shared_ptr<NumberNode> Parser::parseNumber() {
    auto line = currentToken.line, col = currentToken.column;
    if (currentToken.type != TokenType::NUM) {
        THROW_ERROR("Expected number", currentToken.line, currentToken.column);
    }

    auto value = std::stoll(currentToken.value);
    advance();

    return std::make_shared<NumberNode>(line, col, value);
}

std::shared_ptr<FloatNode> Parser::parseFloat() {
    auto line = currentToken.line, col = currentToken.column;
    if (currentToken.type != TokenType::FLOAT) {
        THROW_ERROR("Expected float", currentToken.line, currentToken.column);
    }

    double value = std::stod(currentToken.value);
    advance();

    return std::make_shared<FloatNode>(line, col, value);
}

std::shared_ptr<BooleanNode> Parser::parseBoolean() {
    auto line = currentToken.line, col = currentToken.column;
    bool value = (currentToken.type == TokenType::TRUE);
    advance();
    
    return std::make_shared<BooleanNode>(line, col, value);
}

std::shared_ptr<StringNode> Parser::parseString() {
    auto line = currentToken.line, col = currentToken.column;
    if (currentToken.type != TokenType::STRING) {
        THROW_ERROR("Expected string", currentToken.line, currentToken.column);
    }

    std::string value = currentToken.value;
    advance();

    return std::make_shared<StringNode>(line, col, value);
}

std::shared_ptr<IfStmtNode> Parser::parseIfStmt() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::IF);
    
    // 条件表达式（不需要括号）
    ASTNodePtr condition = parseExpression();
    
    expect(TokenType::LBRACE);
    std::vector<ASTNodePtr> thenBody;
    while (currentToken.type != TokenType::RBRACE && currentToken.type != TokenType::EOF_TOKEN) {
        ASTNodePtr stmt = parseStatement();
        if (stmt) {
            thenBody.push_back(stmt);
        }
    }
    expect(TokenType::RBRACE);
    
    std::vector<ASTNodePtr> elseBody;
    if (currentToken.type == TokenType::ELSE) {
        advance();
        if (currentToken.type == TokenType::IF) {
            elseBody.push_back(parseIfStmt());
            return std::make_shared<IfStmtNode>(line, col, condition, thenBody, elseBody);
        }
        expect(TokenType::LBRACE);
        while (currentToken.type != TokenType::RBRACE && currentToken.type != TokenType::EOF_TOKEN) {
            ASTNodePtr stmt = parseStatement();
            if (stmt) {
                elseBody.push_back(stmt);
            }
        }
        expect(TokenType::RBRACE);
    }
    
    return std::make_shared<IfStmtNode>(line, col, condition, thenBody, elseBody);
}

std::shared_ptr<ForStmtNode> Parser::parseForStmt() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::FOR);
    
    ASTNodePtr init = nullptr;
    ASTNodePtr condition = nullptr;
    ASTNodePtr increment = nullptr;
    std::vector<ASTNodePtr> body;
    
    // 检查是否有条件表达式（while 式）
    // for condition { body } 或 for { body }
    if (currentToken.type != TokenType::LBRACE) {
        condition = parseExpression();
    }
    
    expect(TokenType::LBRACE);
    while (currentToken.type != TokenType::RBRACE && currentToken.type != TokenType::EOF_TOKEN) {
        ASTNodePtr stmt = parseStatement();
        if (stmt) {
            body.push_back(stmt);
        }
    }
    expect(TokenType::RBRACE);
    
    return std::make_shared<ForStmtNode>(line, col, init, condition, increment, body);
}

std::shared_ptr<ASTNode> Parser::parseBreakStmt() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::BREAK);
    expect(TokenType::SEMICOLON);
    return std::make_shared<BreakStmtNode>(line, col);
}

std::shared_ptr<ASTNode> Parser::parseContinueStmt() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::CONTINUE);
    expect(TokenType::SEMICOLON);
    return std::make_shared<ContinueStmtNode>(line, col);
}

std::shared_ptr<MacroCallNode> Parser::parseMacroCall() {
    auto name = currentToken.value;
    expect(TokenType::IDENTIFIER);
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::NOT);
    expect(TokenType::LPAREN);
    
    std::vector<ASTNodePtr> arguments;
    if (currentToken.type != TokenType::RPAREN) {
        do {
            arguments.push_back(parseExpression());
        } while (currentToken.type == TokenType::COMMA && (advance(), true));
    }
    
    expect(TokenType::RPAREN);
    
    return std::make_shared<MacroCallNode>(line, col, name, arguments);
}

ASTNodePtr Parser::parseMacroDecl() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::GRID);
    expect(TokenType::NOT);
    expect(TokenType::LPAREN);
    std::unordered_map<std::string, ASTNodePtr> equations;
    while (currentToken.type != TokenType::RPAREN) {
        auto name = currentToken.value;
        auto nline = currentToken.line, ncol = currentToken.column;
        ASTNodePtr value = std::make_shared<BooleanNode>(nline, ncol, "true");
        expect(TokenType::IDENTIFIER);
        if (currentToken.type == TokenType::ASSIGN) {
            advance();
            value = parseExpression();
        }
        equations[name] = value;
        if (currentToken.type == TokenType::RPAREN) {break;}
        expect(TokenType::COMMA);
    }
    expect(TokenType::RPAREN);
    return std::make_shared<MacroDeclNode>(line, col, equations, parseStatement());
}

std::shared_ptr<StructDeclNode> Parser::parseStructDecl() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::STRUCT);

    bool is_public = false;
    if (currentToken.type == TokenType::PUB) {
        is_public = true;
        advance();
    }

    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected struct name", currentToken.line, currentToken.column);
    }

    std::string name = currentToken.value;
    advance();

    expect(TokenType::LBRACE);

    std::vector<ASTNodePtr> fields;
    while (currentToken.type != TokenType::RBRACE && currentToken.type != TokenType::EOF_TOKEN) {
        fields.push_back(parseFieldDecl());
        expect(TokenType::SEMICOLON);
    }

    expect(TokenType::RBRACE);

    auto struct_decl = std::make_shared<StructDeclNode>(line, col, name, fields);
    struct_decl->is_public = is_public;
    return struct_decl;
}

std::shared_ptr<FieldDeclNode> Parser::parseFieldDecl() {
    auto line = currentToken.line, col = currentToken.column;

    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected field name", currentToken.line, currentToken.column);
    }

    std::string name = currentToken.value;
    advance();

    expect(TokenType::COLON);

    auto type = parseType();

    return std::make_shared<FieldDeclNode>(line, col, name, type);
}



std::shared_ptr<ImplDeclNode> Parser::parseImplDecl() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::IMPL);

    std::string target_type;

    // 解析目标类型
    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected type name", currentToken.line, currentToken.column);
    }
    target_type = currentToken.value;
    advance();

    expect(TokenType::LBRACE);

    std::vector<ASTNodePtr> methods;
    while (currentToken.type != TokenType::RBRACE && currentToken.type != TokenType::EOF_TOKEN) {
        if (currentToken.type == TokenType::CONSTRUCTOR) {
            methods.push_back(parseConstructorDecl());
        } else {
            methods.push_back(parseFunction());
        }
    }

    expect(TokenType::RBRACE);

    return std::make_shared<ImplDeclNode>(line, col, target_type, methods);
}



std::shared_ptr<ConstructorDeclNode> Parser::parseConstructorDecl() {
    auto line = currentToken.line, col = currentToken.column;
    expect(TokenType::CONSTRUCTOR);


    std::vector<Parameter> parameters = parseFunctionArgs();

    expect(TokenType::LBRACE);

    std::vector<ASTNodePtr> body;
    while (currentToken.type != TokenType::RBRACE && currentToken.type != TokenType::EOF_TOKEN)
        if (ASTNodePtr statement = parseStatement()) body.push_back(statement);

    expect(TokenType::RBRACE);

    return std::make_shared<ConstructorDeclNode>(line, col, parameters, body);
}

std::shared_ptr<MemberAccessNode> Parser::parseMemberAccess(const ASTNodePtr& object) {
    auto line = currentToken.line, col = currentToken.column;

    if (currentToken.type != TokenType::IDENTIFIER) {
        THROW_ERROR("Expected member name", currentToken.line, currentToken.column);
    }

    std::string member = currentToken.value;
    advance();

    return std::make_shared<MemberAccessNode>(line, col, object, member);
}
