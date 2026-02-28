#ifndef AST_H
#define AST_H

#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <pstl/algorithm_impl.h>
#include <pstl/algorithm_impl.h>

enum class TypeKind {
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    BOOL,
    STR,
    VOID,
    ANY,
};

enum class NodeType {
    PROGRAM,
    FUNCTION,
    VARIABLE_DECL,
    ASSIGNMENT,
    BINARY_OP,
    FUNCTION_CALL,
    NUMBER,
    FLOAT,
    BOOLEAN,
    STRING,
    IDENTIFIER,
    TYPE_IDENTIFIER,
    RETURN_STMT,
    IF_STMT,
    FOR_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    MACRO_CALL, MACRO_DECL, UNARY,

    STRUCT_DECL,
    ENUM_DECL,
    TRAIT_DECL,
    IMPL_DECL,
    FIELD_DECL,
    VARIANT_DECL,
    METHOD_DECL,
    CONSTRUCTOR_DECL,
    MEMBER_ACCESS, MEMBER_ASSIGN, NAME_SPACE_VISIT
};

enum class BinaryOpType {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    EQ,
    NE,
    LT,
    GT,
    LE,
    GE,
    AND,
    OR,
    DOT, AS
};

class Type {
public:
    virtual ~Type() = default;

    bool is_ptr{false};
    bool is_arr{false};
    std::string name;
    TypeKind kind;
    
    explicit Type(const TypeKind k = TypeKind::ANY, const bool is_ptr = false) : is_ptr(is_ptr), name(to_string(k)), kind(k) {} // any is auto infer

    [[nodiscard]] virtual bool equals(const std::shared_ptr<Type>& other) const {
        return kind == other->kind && is_ptr == other->is_ptr && is_arr == other->is_arr;
    }
    [[nodiscard]] virtual std::string to_string() const {
        std::string result;
        if (is_ptr) result += "*";
        result += name;
        if (is_arr) result += "[]";
        return result;
    }
    
    static TypeKind fromString(const std::string& name) {
        if (name == "i8") return TypeKind::I8;
        if (name == "i16") return TypeKind::I16;
        if (name == "i32") return TypeKind::I32;
        if (name == "i64") return TypeKind::I64;
        if (name == "u8") return TypeKind::U8;
        if (name == "u16") return TypeKind::U16;
        if (name == "u32") return TypeKind::U32;
        if (name == "u64") return TypeKind::U64;
        if (name == "f32") return TypeKind::F32;
        if (name == "f64") return TypeKind::F64;
        if (name == "bool") return TypeKind::BOOL;
        if (name == "str") return TypeKind::STR;
        if (name == "void") return TypeKind::VOID;
        if (name == "@any") return TypeKind::ANY;
        return TypeKind::I32;
    }
    static std::string to_string(const TypeKind kind) {
        switch (kind) {
        case TypeKind::I8: return "i8";
        case TypeKind::ANY: return "@any";
        case TypeKind::I16: return "i16";
        case TypeKind::I32: return "i32";
        case TypeKind::I64: return "i64";
        case TypeKind::U8: return "u8";
        case TypeKind::U16: return "u16";
        case TypeKind::U32: return "u32";
        case TypeKind::U64: return "u64";
        case TypeKind::BOOL: return "bool";
        case TypeKind::STR: return "str";
        case TypeKind::VOID: return "void";
        case TypeKind::F32: return "f32";
        case TypeKind::F64: return "f64";
        }
    }

    Type(const Type& other) {
        this->is_arr = other.is_arr;
        this->is_ptr = other.is_ptr;
        this->kind = other.kind;
        this->name = other.name;
    }

    [[nodiscard]] virtual std::shared_ptr<Type> clone() const {
        auto result = std::make_shared<Type>();
        result->is_arr = is_arr;
        result->is_ptr = is_ptr;
        result->kind = kind;
        result->name = name;
        return result;
    }
    [[nodiscard]] virtual size_t size() const {
        if (is_ptr) return 8;
        switch (kind) {
        case TypeKind::BOOL:
        case TypeKind::I8:
        case TypeKind::U8:
            return 1;
        case TypeKind::I16:
        case TypeKind::U16:
            return 2;
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::F32:
            return 4;
        case TypeKind::I64:
        case TypeKind::U64:
        case TypeKind::F64:
        case TypeKind::STR:
            return 8;
        case TypeKind::VOID:
        default:
            return 0;
        }
    }
};
class ExtType final : public Type {
public:
    std::shared_ptr<Type> basic;
    bool is_ptr{false};
    [[nodiscard]] bool equals(const std::shared_ptr<Type>& other) const override {
        if (auto o = std::dynamic_pointer_cast<ExtType>(other)) {
            return o->basic->equals(basic) && o->is_ptr == is_ptr;
        }
        return basic->equals(other);
    }
    ExtType(const ExtType& other) : Type(*other.basic) {
        this->is_ptr = other.is_ptr;
        this->name = other.name;
        this->kind = other.kind;
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = basic->to_string();
        if (is_ptr) result = "*" + result;
        if (is_arr) result = "[]";
        return result;
    }
    ExtType() = default;

    [[nodiscard]] std::shared_ptr<Type> clone() const override {
        auto result = std::make_shared<ExtType>();
        result->basic = basic->clone();
        return result;
    }

    [[nodiscard]] size_t size() const override {
        if (is_ptr) return 8;
        return basic->size();
    }
};
class StructType final : public Type {
public:
    std::unordered_map<std::string, std::shared_ptr<Type>> fields;
    [[nodiscard]] std::string to_string() const override {
        std::string result = name + " { ";
        for (auto& [name, type] : fields)
            result += name + ": " + type->to_string() + " , ";
        result += " }\n";
        return result;
    }
    StructType() = default;
    [[nodiscard]] std::shared_ptr<Type> clone() const override {
        auto result = std::make_shared<StructType>();
        result->name = name;
        result->fields = fields;
        return result;
    }
    StructType(const StructType& other) {
        *this = *std::static_pointer_cast<StructType>(other.clone());
    }

};

class ASTNode {
public:
    NodeType type;
    size_t line, col;

    virtual ~ASTNode() = default;
    explicit ASTNode(const NodeType type, const size_t line, const size_t col) : type(type), line(line), col(col) {}
};
class ExprNode : public ASTNode {
public:
    std::shared_ptr<Type> ret_type{};
    ~ExprNode() override = default;
    explicit ExprNode(const NodeType type, const size_t line, const size_t col) : ASTNode(type, line, col) {}

    void set_ret_type(const std::shared_ptr<Type> & new_ret_type) {
        this->ret_type = new_ret_type->clone();
    }
    void set_ret_type(const Type& new_ret_type) {
        this->ret_type = std::make_shared<Type>(new_ret_type);
    }
};

class StmtNode : public ASTNode {
public:
    bool is_pub{false};
    ~StmtNode() override = default;
    explicit StmtNode(const NodeType type, const size_t line, const size_t col) : ASTNode(type, line, col) {}
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

class ProgramNode final : public StmtNode {
public:
    std::vector<ASTNodePtr> stmts;

    explicit
    ProgramNode(const size_t line, const size_t col, std::vector<ASTNodePtr> stmts) : StmtNode(NodeType::PROGRAM, line, col),
                                                                          stmts(std::move(stmts)) {}
};

struct Parameter {
    std::string name;
    std::shared_ptr<Type> type;
};

class FunctionNode final: public StmtNode {
public:
    bool has_body{true};
    std::string name;
    std::vector<Parameter> parameters;
    std::shared_ptr<Type> returnType;
    std::vector<ASTNodePtr> body;

    explicit FunctionNode(
        const size_t line,
        const size_t col,
        std::string name,
        std::vector<Parameter> parameters,
        std::shared_ptr<Type> returnType,
        std::vector<ASTNodePtr> body) : StmtNode(NodeType::FUNCTION, line, col), name(std::move(name)),
                                        parameters(std::move(parameters)), returnType(std::move(returnType)),
                                        body(std::move(body)) {}
};

class VariableDeclNode final: public StmtNode {
public:
    std::string name;
    std::shared_ptr<Type> type;
    ASTNodePtr initializer;

    explicit VariableDeclNode(const size_t line, const size_t col, std::string name, std::shared_ptr<Type> type,
                              ASTNodePtr initializer) : StmtNode(NodeType::VARIABLE_DECL, line, col),
                                                        name(std::move(name)), type(std::move(type)),
                                                        initializer(std::move(initializer)) {}
};
class NameSpaceVisitNode final : public StmtNode {
public:
    ASTNodePtr last;
    ASTNodePtr expr;
    explicit NameSpaceVisitNode(const size_t line, const size_t col, ASTNodePtr last, ASTNodePtr name) : StmtNode(NodeType::NAME_SPACE_VISIT, line, col),
        last(std::move(last)), expr(std::move(name)){}
};
class AssignmentNode final: public StmtNode {
public:
    std::string name;
    ASTNodePtr value;

    explicit AssignmentNode(const size_t line, const size_t col, std::string name, ASTNodePtr value) : StmtNode(NodeType::ASSIGNMENT,
        line, col), name(std::move(name)), value(std::move(value)) {}
};
class MemberAssignNode final: public StmtNode {
public:
    ASTNodePtr member;
    ASTNodePtr value;
    explicit MemberAssignNode(const size_t line, const size_t col, ASTNodePtr member, ASTNodePtr value) : StmtNode(NodeType::MEMBER_ASSIGN, line, col),
        member(std::move(member)), value(std::move(value)) {}
};

class BinaryOpNode final: public ExprNode {
public:
    BinaryOpType op;
    ASTNodePtr left;
    ASTNodePtr right;

    explicit
    BinaryOpNode(const size_t line, const size_t col, ASTNodePtr left, const BinaryOpType op,
                 ASTNodePtr right) : ExprNode(NodeType::BINARY_OP, line, col), op(op), left(std::move(left)),
                                     right(std::move(right)) {}
};

enum class UnaryOpType{
    Addr, Minus,
};
class UnaryOpNode final: public ExprNode{
public:
    UnaryOpType op;
    ASTNodePtr expr;
    explicit UnaryOpNode(const size_t line, const size_t col, const UnaryOpType op, ASTNodePtr e) :
        ExprNode(NodeType::UNARY, line, col), op(op), expr(std::move(e)) {}
};

class FunctionCallNode final: public ExprNode {
public:
    std::string name;
    std::vector<ASTNodePtr> arguments;

    explicit
    FunctionCallNode(const size_t line, const size_t col, std::string name,
                     std::vector<ASTNodePtr> arguments) : ExprNode(NodeType::FUNCTION_CALL, line, col), name(std::move(name)),
                                                          arguments(std::move(arguments)) {}
};

class NumberNode final: public ExprNode {
public:
    int64_t value;

    explicit NumberNode(const size_t line, const size_t col, const int64_t value) : ExprNode(NodeType::NUMBER, line, col), value(value) {
        if (value > INT32_MAX) set_ret_type(Type{TypeKind::I64});
        else if (value <= INT8_MAX) set_ret_type(Type{TypeKind::I8});
        else if (value <= INT16_MAX) set_ret_type(Type{TypeKind::I16});
        else set_ret_type(Type{TypeKind::I32});
    }
};

class FloatNode final: public ExprNode {
public:
    double value;
    explicit FloatNode(const size_t line, const size_t col, const double value) : ExprNode(NodeType::FLOAT, line, col), value(value) {
        set_ret_type(Type{TypeKind::F64});
    }
};

class BooleanNode final: public ExprNode {
public:
    bool value;
    explicit BooleanNode(const size_t line, const size_t col, const bool value) : ExprNode(NodeType::BOOLEAN, line, col), value(value) {
        set_ret_type(Type{TypeKind::BOOL});
    }
};

class StringNode final: public ExprNode {
public:
    std::string value;

    explicit StringNode(const size_t line, const size_t col, std::string value) : ExprNode(NodeType::STRING, line, col), value(std::move(value)) {
        set_ret_type(Type{TypeKind::STR});
    }
};

class IdentifierNode final: public ExprNode {
public:
    std::string name;
    explicit IdentifierNode(const size_t line, const size_t col, std::string value) : ExprNode(NodeType::IDENTIFIER, line, col), name(std::move(value)) {}
};

class TypeIdentifierNode final: public StmtNode {
public:
    std::string name;
    explicit TypeIdentifierNode(const size_t line, const size_t col, std::string value) : StmtNode(NodeType::STRING, line, col), name(std::move(value)) {}
};

class ReturnStmtNode final : public StmtNode {
public:
    ASTNodePtr expression;
    explicit ReturnStmtNode(const size_t line, const size_t col, ASTNodePtr expression) : StmtNode(NodeType::RETURN_STMT, line, col), expression(std::move(expression)) {}
};

class IfStmtNode final : public StmtNode {
public:
    ASTNodePtr condition;
    std::vector<ASTNodePtr> thenBody;
    std::vector<ASTNodePtr> elseBody;
    
    explicit IfStmtNode(const size_t line, const size_t col, 
                        ASTNodePtr condition, 
                        std::vector<ASTNodePtr> thenBody,
                        std::vector<ASTNodePtr> elseBody)
        : StmtNode(NodeType::IF_STMT, line, col),
          condition(std::move(condition)), 
          thenBody(std::move(thenBody)), 
          elseBody(std::move(elseBody)) {}
};

class ForStmtNode final : public StmtNode {
public:
    ASTNodePtr init;       // 初始化表达式（可选）
    ASTNodePtr condition;  // 条件表达式
    ASTNodePtr increment;  // 增量表达式（可选）
    std::vector<ASTNodePtr> body;
    
    explicit ForStmtNode(const size_t line, const size_t col,
                         ASTNodePtr init,
                         ASTNodePtr condition,
                         ASTNodePtr increment,
                         std::vector<ASTNodePtr> body)
        : StmtNode(NodeType::FOR_STMT, line, col),
          init(std::move(init)),
          condition(std::move(condition)),
          increment(std::move(increment)),
          body(std::move(body)) {}
};

class BreakStmtNode final : public StmtNode {
public:
    explicit BreakStmtNode(const size_t line, const size_t col) 
        : StmtNode(NodeType::BREAK_STMT, line, col) {}
};

class ContinueStmtNode final : public StmtNode {
public:
    explicit ContinueStmtNode(const size_t line, const size_t col) 
        : StmtNode(NodeType::CONTINUE_STMT, line, col) {}
};

class MacroCallNode final : public ExprNode {
public:
    std::string name;
    std::vector<ASTNodePtr> arguments;
    
    explicit MacroCallNode(const size_t line, const size_t col, 
                           std::string name,
                           std::vector<ASTNodePtr> arguments)
        : ExprNode(NodeType::MACRO_CALL, line, col), 
          name(std::move(name)), 
          arguments(std::move(arguments)) {}
};

class MacroDeclNode final : public StmtNode{
public:
    std::unordered_map<std::string, ASTNodePtr> equations;
    ASTNodePtr declaration;
    explicit MacroDeclNode(const size_t line, const size_t col, std::unordered_map<std::string, ASTNodePtr> equations, ASTNodePtr declaration) :
        StmtNode(NodeType::MACRO_DECL, line, col), equations(std::move(equations)), declaration(std::move(declaration)) {}
};

// 结构体声明节点
class StructDeclNode final : public StmtNode {
public:
    bool is_public{false};
    std::string name;
    std::vector<ASTNodePtr> fields;

    explicit StructDeclNode(const size_t line, const size_t col,
                           std::string name,
                           std::vector<ASTNodePtr> fields)
        : StmtNode(NodeType::STRUCT_DECL, line, col),
          name(std::move(name)),
          fields(std::move(fields)) {}
};

// 字段声明节点
class FieldDeclNode final : public StmtNode {
public:
    std::string name;
    std::shared_ptr<Type> type;

    explicit FieldDeclNode(const size_t line, const size_t col,
                          std::string name,
                          std::shared_ptr<Type> type)
        : StmtNode(NodeType::FIELD_DECL, line, col),
          name(std::move(name)),
          type(std::move(type)) {}
};

// Impl声明节点
class ImplDeclNode final : public StmtNode {
public:
    std::string target_type;
    std::vector<ASTNodePtr> methods;

    explicit ImplDeclNode(const size_t line, const size_t col,
                         std::string target_type,
                         std::vector<ASTNodePtr> methods)
        : StmtNode(NodeType::IMPL_DECL, line, col),
          target_type(std::move(target_type)),
          methods(std::move(methods)) {}
};

// 构造函数声明节点
class ConstructorDeclNode final : public StmtNode {
public:
    std::vector<Parameter> parameters;
    std::vector<ASTNodePtr> body;

    explicit ConstructorDeclNode(const size_t line, const size_t col,
                                std::vector<Parameter> parameters,
                                std::vector<ASTNodePtr> body)
        : StmtNode(NodeType::CONSTRUCTOR_DECL, line, col),
          parameters(std::move(parameters)),
          body(std::move(body)) {}
};

// 成员访问节点
class MemberAccessNode final : public ExprNode {
public:
    ASTNodePtr object;
    ASTNodePtr expr;

    explicit MemberAccessNode(const size_t line, const size_t col,
                             ASTNodePtr object,
                             ASTNodePtr member)
        : ExprNode(NodeType::MEMBER_ACCESS, line, col),
          object(std::move(object)),
          expr(std::move(member)) {}
};

#endif // AST_H
