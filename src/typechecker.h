#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "ast.h"
#include <memory>
#include <map>
#include <vector>
#include <string>

struct VariableInfo {
    std::shared_ptr<Type> type;
};

struct FunctionInfo {
    std::vector<std::shared_ptr<Type>> paramTypes;
    std::shared_ptr<Type> returnType;
    bool has_body;
};

class TypeChecker {
public:
    TypeChecker();
    
    void checkProgram(const std::shared_ptr<ProgramNode> &program);
    
private:
    std::map<std::string, VariableInfo> variables;
    std::map<std::string, FunctionInfo> functions;
    std::vector<std::map<std::string, VariableInfo>> scopes;
    
    void pushScope();
    void popScope();
    void addVariable(const std::string& name, std::shared_ptr<Type> type, size_t line, size_t col);
    std::shared_ptr<VariableInfo> findVariable(const std::string& name);
    void addFunction(const std::string &name, const std::vector<std::shared_ptr<Type>> &paramTypes, std::shared_ptr<Type> returnType, bool
                     has_body, size_t line, size_t col);
    FunctionInfo* findFunction(const std::string& name);
    
    void checkFunction(const std::shared_ptr<FunctionNode> &func);
    std::shared_ptr<Type> checkExpression(const ASTNodePtr& expr);
    std::shared_ptr<Type> checkStatement(const ASTNodePtr &stmt);
    void checkVariableDecl(const std::shared_ptr<VariableDeclNode>& decl);
    std::shared_ptr<Type> checkBinaryOp(const std::shared_ptr<BinaryOpNode>& op);
    std::shared_ptr<Type> checkFunctionCall(const std::shared_ptr<FunctionCallNode> &call);
    std::shared_ptr<Type> checkPrimary(const ASTNodePtr &expr);

    std::shared_ptr<Type> checkUnary(const std::shared_ptr<UnaryOpNode> &op);

    std::shared_ptr<Type> checkIdentifier(const std::shared_ptr<IdentifierNode> &id);
};

#endif // TYPECHECKER_H
