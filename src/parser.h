#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>

class Parser {
public:
    explicit Parser(Lexer& lexer);
    
    std::shared_ptr<ProgramNode> parseProgram();
    
private:
    Lexer& lexer;
    Token currentToken;
    
    void advance();
    void expect(TokenType type);
    std::shared_ptr<Type> parseType();
    
    ASTNodePtr parseStatement();
    std::shared_ptr<VariableDeclNode> parseVariableDecl();
    std::shared_ptr<FunctionNode> parseFunction();
    std::shared_ptr<ReturnStmtNode> parseReturnStmt();
    [[nodiscard]] BinaryOpType tokenType2opType(TokenType type) const;

    ASTNodePtr parseExpression();

    ASTNodePtr parseBoolAndOr();

    ASTNodePtr parseBoolOper();

    ASTNodePtr parseAssignment();

    ASTNodePtr parseAddSub();

    ASTNodePtr parseMulDivMod();

    ASTNodePtr parseBinaryOp();
    ASTNodePtr parsePrimary();
    
    std::shared_ptr<FunctionCallNode> parseFunctionCall();
    std::shared_ptr<IdentifierNode> parseIdentifier();
    std::shared_ptr<NumberNode> parseNumber();
    std::shared_ptr<FloatNode> parseFloat();
    std::shared_ptr<BooleanNode> parseBoolean();
    std::shared_ptr<StringNode> parseString();
    
    std::shared_ptr<IfStmtNode> parseIfStmt();
    std::shared_ptr<ForStmtNode> parseForStmt();
    std::shared_ptr<ASTNode> parseBreakStmt();
    std::shared_ptr<ASTNode> parseContinueStmt();
    std::shared_ptr<MacroCallNode> parseMacroCall();
    ASTNodePtr parseMacroDecl();

    std::shared_ptr<StructDeclNode> parseStructDecl();
    std::shared_ptr<FieldDeclNode> parseFieldDecl();
    std::shared_ptr<EnumDeclNode> parseEnumDecl();
    std::shared_ptr<VariantDeclNode> parseVariantDecl();
    std::shared_ptr<TraitDeclNode> parseTraitDecl();
    std::shared_ptr<ImplDeclNode> parseImplDecl();
    std::shared_ptr<MethodDeclNode> parseMethodDecl();
    std::shared_ptr<ConstructorDeclNode> parseConstructorDecl();
    std::shared_ptr<MemberAccessNode> parseMemberAccess(const ASTNodePtr& object);

};

#endif // PARSER_H
