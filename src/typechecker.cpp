#include "typechecker.h"
#include "common.h"
#include <stdexcept>
#include <utility>

TypeChecker::TypeChecker() {
    auto i32Type = std::make_shared<Type>(TypeKind::I32);
    auto voidType = std::make_shared<Type>(TypeKind::VOID);

}

void TypeChecker::pushScope() {
    scopes.push_back(variables);
}

void TypeChecker::popScope() {
    if (!scopes.empty()) {
        variables = scopes.back();
        scopes.pop_back();
    }
}

void TypeChecker::addVariable(const std::string& name, std::shared_ptr<Type> type, size_t line, size_t col) {
    if (variables.contains(name)) {
        THROW_ERROR("Variable already defined: " + name, line, col);
    }
    variables[name] = {std::move(type)};
}

VariableInfo* TypeChecker::findVariable(const std::string& name) {
    const auto it = variables.find(name);
    if (it != variables.end()) {
        return &it->second;
    }
    return nullptr;
}

void TypeChecker::addFunction(const std::string& name, const std::vector<std::shared_ptr<Type>>& paramTypes, std::shared_ptr<Type> returnType, bool has_body, size_t line, size_t col) {

    if (const auto it = functions.find(name); it != functions.end() && it->second.has_body) {
        THROW_ERROR("Function already defined: " + name, line, col);
    }
    functions[name] = {paramTypes, std::move(returnType), has_body};
}

FunctionInfo* TypeChecker::findFunction(const std::string& name) {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return &it->second;
    }
    return nullptr;
}

void TypeChecker::checkProgram(const std::shared_ptr<ProgramNode>& program) {
    auto handle_function = [this](const ASTNodePtr& stmt) {
        auto func = std::static_pointer_cast<FunctionNode>(stmt);

        std::vector<std::shared_ptr<Type>> paramTypes;
        for (const auto& param : func->parameters) {
            paramTypes.push_back(param.type);
        }

        addFunction(func->name, paramTypes, func->returnType, func->has_body, func->line, func->col);
    };
    for (const ASTNodePtr& stmt : program->stmts) {
        if (stmt->type == NodeType::FUNCTION) {
            handle_function(stmt);
        }
        else if (stmt->type == NodeType::MACRO_DECL) {
            bool check = true;
            const auto& tmp = std::static_pointer_cast<MacroDeclNode>(stmt);
            for (const auto& decl : tmp->equations) {
                if (decl.first == "target") {
                    if (std::static_pointer_cast<StringNode>(decl.second)->value != P_TARGET)
                        check = false;
                }
            }
            if (check)
                handle_function(tmp->declaration);
        }

    }

    for (const ASTNodePtr& stmt : program->stmts) {
        if (stmt->type == NodeType::FUNCTION) {
            checkFunction(std::static_pointer_cast<FunctionNode>(stmt));
        } else if (stmt->type == NodeType::MACRO_DECL) {
            const auto& tmp = std::static_pointer_cast<MacroDeclNode>(stmt);
            if (tmp->declaration->type == NodeType::FUNCTION) {
                const auto& fn = std::static_pointer_cast<FunctionNode>(tmp->declaration);
                checkFunction(fn);
            }
        }
    }
}

void TypeChecker::checkFunction(const std::shared_ptr<FunctionNode>& func) {
    /*std::vector<std::shared_ptr<Type>> paramTypes;
    for (const auto& param : func->parameters) {
        paramTypes.push_back(param.type);
    }
    addFunction(func->name, paramTypes, func->returnType, func->has_body, func->line, func->col);
    */pushScope();
    
    for (const auto& param : func->parameters) {
        addVariable(param.name, param.type, func->line, func->col);
    }
    
    for (const ASTNodePtr& stmt : func->body) {
        checkStatement(stmt);
    }
    
    popScope();
}

std::shared_ptr<Type> TypeChecker::checkExpression(const ASTNodePtr& expr) {
    return checkPrimary(expr);
}

std::shared_ptr<Type> TypeChecker::checkStatement(const ASTNodePtr& stmt) {
    switch (stmt->type) {
        case NodeType::VARIABLE_DECL:
            checkVariableDecl(std::static_pointer_cast<VariableDeclNode>(stmt));
            return std::make_shared<Type>(TypeKind::VOID);
        case NodeType::ASSIGNMENT: {
            auto assign = std::static_pointer_cast<AssignmentNode>(stmt);
            auto varInfo = findVariable(assign->name);
            if (!varInfo) {
                THROW_ERROR("Undefined variable: " + assign->name, stmt->line, stmt->col);
                return nullptr;
            }
            auto valueType = checkExpression(assign->value);
            if (!varInfo->type->equals(valueType)) {
                THROW_ERROR("Type mismatch in assignment", stmt->line, stmt->col);
                return nullptr;
            }
            return std::make_shared<Type>(TypeKind::VOID);
        }
        case NodeType::RETURN_STMT: {
            auto returnStmt = std::static_pointer_cast<ReturnStmtNode>(stmt);
            if (returnStmt->expression) {
                checkExpression(returnStmt->expression);
            }
            return std::make_shared<Type>(TypeKind::VOID);
        }
        case NodeType::IF_STMT: {
            auto ifStmt = std::static_pointer_cast<IfStmtNode>(stmt);
            auto condType = checkExpression(ifStmt->condition);
            if (condType->kind != TypeKind::BOOL) {
                THROW_ERROR("If condition must be boolean", ifStmt->line, ifStmt->col);
            }
            for (const auto& s : ifStmt->thenBody) {
                checkStatement(s);
            }
            for (const auto& s : ifStmt->elseBody) {
                checkStatement(s);
            }
            return std::make_shared<Type>(TypeKind::VOID);
        }
        case NodeType::FOR_STMT: {
            auto forStmt = std::static_pointer_cast<ForStmtNode>(stmt);
            if (forStmt->init) {
                checkExpression(forStmt->init);
            }
            if (forStmt->condition) {
                checkExpression(forStmt->condition);
            }
            if (forStmt->increment) {
                checkExpression(forStmt->increment);
            }
            for (const auto& s : forStmt->body) {
                checkStatement(s);
            }
            return std::make_shared<Type>(TypeKind::VOID);
        }
        case NodeType::BREAK_STMT:
        case NodeType::CONTINUE_STMT:
            return std::make_shared<Type>(TypeKind::VOID);
        default:
            return checkExpression(stmt);
    }
}

void TypeChecker::checkVariableDecl(const std::shared_ptr<VariableDeclNode>& decl) {
    auto initType = checkExpression(decl->initializer);
    if (!decl->type->equals(initType)) {
        THROW_ERROR("Type mismatch in variable declaration", decl->line, decl->col);
    }
    addVariable(decl->name, decl->type, decl->line, decl->col);
}

std::shared_ptr<Type> TypeChecker::checkBinaryOp(const std::shared_ptr<BinaryOpNode>& op) {
    auto leftType = checkExpression(op->left);
    auto rightType = checkExpression(op->right);

    switch (op->op) {
        case BinaryOpType::ADD:
        case BinaryOpType::SUB:
        case BinaryOpType::MUL:
        case BinaryOpType::DIV:
        case BinaryOpType::MOD:
            // 算术运算需要数值类型
            if (leftType->kind != TypeKind::I8 && leftType->kind != TypeKind::I16 &&
                leftType->kind != TypeKind::I32 && leftType->kind != TypeKind::I64 &&
                leftType->kind != TypeKind::U8 && leftType->kind != TypeKind::U16 &&
                leftType->kind != TypeKind::U32 && leftType->kind != TypeKind::U64 &&
                leftType->kind != TypeKind::F32 && leftType->kind != TypeKind::F64) {
                THROW_ERROR("Arithmetic operations require numeric types, left type: " + std::to_string((int)leftType->kind), op->line, op->col);
            }
            if (rightType->kind != TypeKind::I8 && rightType->kind != TypeKind::I16 &&
                rightType->kind != TypeKind::I32 && rightType->kind != TypeKind::I64 &&
                rightType->kind != TypeKind::U8 && rightType->kind != TypeKind::U16 &&
                rightType->kind != TypeKind::U32 && rightType->kind != TypeKind::U64 &&
                rightType->kind != TypeKind::F32 && rightType->kind != TypeKind::F64) {
                THROW_ERROR("Arithmetic operations require numeric types, right type: " + std::to_string((int)rightType->kind), op->line, op->col);
            }
            return leftType;
        case BinaryOpType::EQ:
        case BinaryOpType::NE:
        case BinaryOpType::LT:
        case BinaryOpType::GT:
        case BinaryOpType::LE:
        case BinaryOpType::GE:
            return std::make_shared<Type>(TypeKind::BOOL);
        case BinaryOpType::AND:
        case BinaryOpType::OR:
            // 逻辑运算需要布尔类型
            if (leftType->kind != TypeKind::BOOL || rightType->kind != TypeKind::BOOL) {
                THROW_ERROR("Logical operations require boolean types", op->line, op->col);
                return nullptr;
            }
            return std::make_shared<Type>(TypeKind::BOOL);
        case BinaryOpType::AS:

        default:
            return leftType;
    }
}

std::shared_ptr<Type> TypeChecker::checkFunctionCall(const std::shared_ptr<FunctionCallNode>& call) {
    const auto funcInfo = findFunction(call->name);
    if (!funcInfo) {
        THROW_ERROR("Undefined function: " + call->name, call->line, call->col);
        return nullptr;
    }

    if (call->arguments.size() != funcInfo->paramTypes.size()) {
        THROW_ERROR("Wrong number of arguments for function " + call->name, call->line, call->col);
    }

    for (size_t i = 0; i < call->arguments.size(); ++i)
        if (const auto argType = checkExpression(call->arguments[i]); !argType->equals(funcInfo->paramTypes[i]))
            THROW_ERROR("Type mismatch in argument " + std::to_string(i) + " of function " + call->name, call->line, call->col);

    return funcInfo->returnType;
}

std::shared_ptr<Type> TypeChecker::checkPrimary(const ASTNodePtr& expr) {
    switch (expr->type) {
        case NodeType::NUMBER:
            return std::make_shared<Type>(TypeKind::I32);
        case NodeType::FLOAT:
            return std::make_shared<Type>(TypeKind::F32);
        case NodeType::BOOLEAN:
            return std::make_shared<Type>(TypeKind::BOOL);
        case NodeType::STRING:
            return std::make_shared<Type>(TypeKind::STR);
        case NodeType::IDENTIFIER:
            return checkIdentifier(std::static_pointer_cast<IdentifierNode>(expr));
        case NodeType::FUNCTION_CALL:
            return checkFunctionCall(std::static_pointer_cast<FunctionCallNode>(expr));
        case NodeType::BINARY_OP:
            return checkBinaryOp(std::static_pointer_cast<BinaryOpNode>(expr));
        case NodeType::MACRO_CALL:
            // 宏调用由编译器特殊处理，返回 i32
            return std::make_shared<Type>(TypeKind::I32);
        case NodeType::UNARY:
            return checkUnary(std::static_pointer_cast<UnaryOpNode>(expr));
        default:
            THROW_ERROR("Unknown expression type", expr->line, expr->col);
            return nullptr;
    }
}

std::shared_ptr<Type> TypeChecker::checkUnary(const std::shared_ptr<UnaryOpNode>& op) {
    switch (op->op) {
        case UnaryOpType::Addr: {
            auto result = std::make_shared<ExtType>();
            result->basic = checkExpression(op->expr);
            result->is_ptr = true;
            return result;
        }
        case UnaryOpType::Minus:
        default:
            return checkExpression(op->expr);
    }
}
std::shared_ptr<Type> TypeChecker::checkIdentifier(const std::shared_ptr<IdentifierNode>& id) {
    auto varInfo = findVariable(id->name);
    if (!varInfo) {
        THROW_ERROR("Undefined variable: " + id->name, id->line, id->col);
        return nullptr;
    }
    return varInfo->type;
}
