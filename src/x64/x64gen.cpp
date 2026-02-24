//
// Created by geguj on 2026/2/21.
//

#include "x64gen.hpp"
#include <cstddef>
#include <sstream>
#include <iostream>
#include "register.h"
#include <map>
#include "../common.h"

void WatGen::gen(const ASTNodePtr &node) {
    switch (node->type) {
    using enum NodeType;
    case PROGRAM: {
        gen_program(node);
        break;
    }
    case FUNCTION: {
        gen_function(node);
        break;
    }
    case VARIABLE_DECL: {
        gen_var(node);
        break;
    }
    case ASSIGNMENT: {
        gen_assignment(node);
        break;
    }
    case BINARY_OP: {
        gen_binary(node);
        break;
    }
    case FUNCTION_CALL: {
        gen_function_call(node);
        break;
    }
    case NUMBER: {
        gen_number(node);
        break;
    }
    case FLOAT: {
        gen_float(node);
        break;
    }
    case BOOLEAN: {
        gen_boolean(node);
        break;
    }
    case STRING: {
        gen_string(node);
        break;
    }
    case IDENTIFIER: {
        gen_identifier(node);
        break;
    }
    case TYPE_IDENTIFIER: {
        break;
    }
    case RETURN_STMT: {
        gen_return_stmt(node);
        break;
    }
    case IF_STMT: {
        gen_if_stmt(node);
        break;
    }
    case FOR_STMT: {
        gen_for_stmt(node);
        break;
    }
    case BREAK_STMT: {
        gen_break_stmt(node);
        break;
    }
    case CONTINUE_STMT: {
        gen_continue_stmt(node);
        break;
    }
    case MACRO_CALL: {
        gen_macro_call(node);
        break;
    }
    case MACRO_DECL: {
        gen_macro_decl(node);
        break;
    }
    case UNARY: {
        gen_unary(node);
        break;
    }
    }
}

void WatGen::gen_program(const ASTNodePtr &node) {
    const auto n = std::static_pointer_cast<ProgramNode>(node);
    
    output << ".intel_syntax noprefix" << std::endl;
    output << ".globl main" << std::endl;
    output << std::endl;
    
    stack_offset = 0;
    var_offsets.clear();
    var_str_lens.clear();
    
    for (const auto& c: n->stmts) {
        gen(c);
    }
    
    // 输出数据段（字符串常量）
    if (!data_output.str().empty()) {
        output << std::endl;
        output << ".section .rodata" << std::endl;
        output << data_output.str();
    }
    if (P_TARGET != "Windows")
        output << ".section .note.GNU-stack,\"\",@progbits" << std::endl;
}

void WatGen::gen_function(const ASTNodePtr &node) {
    const auto fn = std::static_pointer_cast<FunctionNode>(node);
    
    has_return = false;
    stack_offset = 0;
    var_offsets.clear();
    var_size = 8;
    if (extern_flag) {
        output << ".extern " << fn->name << std::endl;
        return;
    }
    
    output << fn->name;
    // for (const auto& p : fn->parameters) {
    //     output << "#" << Type::to_string(p.type->kind);
    // }
    output << ":\n";

    
    // 保存栈帧
    output << "    push rbp" << std::endl;
    output << "    mov rbp, rsp" << std::endl;
    output << "    sub rsp, ";
    auto backup_output = output.str();
    output.str("");
    // 为参数分配栈空间并存储
    const auto regs = func_call_regs;
    constexpr size_t reg_count = RegAlloc::used.size();
    
    for (size_t i = 0; i < fn->parameters.size() && i < reg_count; i++) {
        stack_offset += 8;
        var_offsets[fn->parameters[i].name] = stack_offset;
        output << "    mov [rbp - " << stack_offset << "], " << regs[i] << std::endl;
    }
    size_t param_size = stack_offset;
    
    // 生成函数体
    for (const auto& stmt : fn->body) {
        gen(stmt);
    }
    
    // 如果没有显式返回，添加默认返回
    if (!has_return) {
        output << "    leave" << std::endl;
        output << "    ret" << std::endl;
    }
    backup_output += std::to_string(var_size + param_size) + "\n";
    std::ostringstream new_ss;
    new_ss << backup_output;
    new_ss << output.str();
    output = std::move(new_ss);
    output << std::endl;
}

void WatGen::gen_var(const ASTNodePtr &node) {
    const auto var = std::static_pointer_cast<VariableDeclNode>(node);
    
    // 为变量分配栈空间 (8 字节对齐)
    // 如果是字符串类型，需要 16 字节（fat pointer）
    const bool is_string = var->type->kind == TypeKind::STR;
    size_t lvar_size = 8;
    stack_offset += lvar_size;
    var_size += lvar_size;
    var_offsets[var->name] = stack_offset;
    
    output << "    # declare var: " << var->name << std::endl;
    
    // 如果有初始化值，计算并存储
    if (var->initializer) {
        gen(var->initializer);
        if (is_string) {
            var_str_lens[var->name] = str_len;
        }
        output << "    mov [rbp - " << stack_offset << "], rax" << std::endl;
    }
}

void WatGen::gen_unary(const ASTNodePtr& node) {
    const auto n = std::static_pointer_cast<UnaryOpNode>(node);
    switch (n->op) {
        using enum UnaryOpType;
    case Addr: {
        var_operation = "lea";
        gen(n->expr);
        var_operation = "mov";
    }
    case Minus: {
        if (n->expr->type == NodeType::NUMBER) {
            const auto tmp = std::static_pointer_cast<NumberNode>(n->expr);
            output << "    mov rax, -" << tmp->value << std::endl;
        }
        else {
            gen(n->expr);
            output << "neg rax" << std::endl;
        }
    }
    }
}

void WatGen::gen_assignment(const ASTNodePtr &node) {
    const auto assign = std::static_pointer_cast<AssignmentNode>(node);

    // 计算右侧表达式
    gen(assign->value);
    
    // 存储到变量
    output << "    mov [rbp - " << get_var_offset(assign->name) << "], rax" << std::endl;
}

void WatGen::gen_binary(const ASTNodePtr &node) {
    const auto binop = std::static_pointer_cast<BinaryOpNode>(node);
    
    // 先计算左操作数
    gen(binop->left);
    output << "    push rax" << std::endl;
    
    // 再计算右操作数
    gen(binop->right);
    output << "    pop rbx" << std::endl;
    
    // 根据操作符生成指令
    switch (binop->op) {
    case BinaryOpType::ADD:
        output << "    add rbx, rax" << std::endl;
        output << "    mov rax, rbx" << std::endl;
        break;
    case BinaryOpType::SUB:
        output << "    sub rbx, rax" << std::endl;
        output << "    mov rax, rbx" << std::endl;
        break;
    case BinaryOpType::MUL:
        output << "    imul rbx, rax" << std::endl;
        output << "    mov rax, rbx" << std::endl;
        break;
    case BinaryOpType::DIV:
        output << "    mov rax, rbx" << std::endl;
        output << "    xor rdx, rdx" << std::endl;
        output << "    pop rcx" << std::endl;
        output << "    idiv rcx" << std::endl;
        return; // 已经在 rax 中
    case BinaryOpType::MOD:
        output << "    mov rax, rbx" << std::endl;
        output << "    xor rdx, rdx" << std::endl;
        output << "    pop rcx" << std::endl;
        output << "    idiv rcx" << std::endl;
        output << "    mov rax, rdx" << std::endl;
        return; // 余数在 rdx
    case BinaryOpType::EQ:
        output << "    cmp rbx, rax" << std::endl;
        output << "    setz al" << std::endl;
        output << "    movzx rax, al" << std::endl;
        break;
    case BinaryOpType::NE:
        output << "    cmp rbx, rax" << std::endl;
        output << "    setnz al" << std::endl;
        output << "    movzx rax, al" << std::endl;
        break;
    case BinaryOpType::LT:
        output << "    cmp rbx, rax" << std::endl;
        output << "    setl al" << std::endl;
        output << "    movzx rax, al" << std::endl;
        break;
    case BinaryOpType::GT:
        output << "    cmp rbx, rax" << std::endl;
        output << "    setg al" << std::endl;
        output << "    movzx rax, al" << std::endl;
        break;
    case BinaryOpType::LE:
        output << "    cmp rbx, rax" << std::endl;
        output << "    setle al" << std::endl;
        output << "    movzx rax, al" << std::endl;
        break;
    case BinaryOpType::GE:
        output << "    cmp rbx, rax" << std::endl;
        output << "    setge al" << std::endl;
        output << "    movzx rax, al" << std::endl;
        break;
    case BinaryOpType::AND:
        output << "    and rbx, rax" << std::endl;
        output << "    mov rax, rbx" << std::endl;
        break;
    case BinaryOpType::OR:
        output << "    or rbx, rax" << std::endl;
        output << "    mov rax, rbx" << std::endl;
        break;
    default:
        break;
    }
}

void WatGen::gen_number(const ASTNodePtr &node) {
    const auto num = std::static_pointer_cast<NumberNode>(node);
    output << "    mov rax, " << num->value << std::endl;
}

void WatGen::gen_float(const ASTNodePtr &node) {
    const auto flt = std::static_pointer_cast<FloatNode>(node);
    // 浮点数需要特殊处理，这里简化为整数加载
    output << "    # float: " << flt->value << std::endl;
    output << "    mov rax, " << static_cast<int64_t>(flt->value) << std::endl;
}

void WatGen::gen_boolean(const ASTNodePtr &node) {
    const auto boolean = std::static_pointer_cast<BooleanNode>(node);
    output << "    mov rax, " << (boolean->value ? 1 : 0) << std::endl;
}

void WatGen::gen_string(const ASTNodePtr &node) {
    const auto str = std::static_pointer_cast<StringNode>(node);
    
    // 生成字符串数据（在数据段）
    int label = gen_string_data(str->value);

    // rax = ptr
    output << "    # string: " << str->value << std::endl;
    output << "    lea rax, [rip + .L_str_" << label << "]" << std::endl;
    str_len = str->value.length();
}

int WatGen::gen_string_data(const std::string& str) {
    int label = string_counter++;
    
    // 检查是否已存在相同字符串
    auto it = string_labels.find(str);
    if (it != string_labels.end()) {
        return it->second;
    }
    
    string_labels[str] = label;
    
    // 转义字符串用于 .string 指令
    std::string escaped;
    for (size_t i = 0; i < str.length(); i++) {
        unsigned char c = str[i];
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else if (c >= 32 && c < 127) escaped += c;
        else {
            // 八进制转义
            char buf[8] = {};
            snprintf(buf, sizeof(buf), "\\%03o", c);
            escaped += buf;
        }
    }
    
    data_output << ".L_str_" << label << ":" << std::endl;
    data_output << "    .string \"" << escaped << "\"" << std::endl;
    data_output << std::endl;
    
    return label;
}

void WatGen::gen_identifier(const ASTNodePtr &node) {
    const auto id = std::static_pointer_cast<IdentifierNode>(node);
    size_t offset = get_var_offset(id->name);
    
    // 检查是否是字符串类型（fat pointer）
    // 简单判断：如果 offset 是 16 的倍数，可能是字符串
    // 这里简化处理，直接读取 ptr 部分
    output << "    " << var_operation << " rax, [rbp - " << offset << "]" << std::endl;
}

void WatGen::gen_function_call(const ASTNodePtr &node) {
    const auto call = std::static_pointer_cast<FunctionCallNode>(node);

    // x86-64 调用约定：使用寄存器传递参数 (rdi, rsi, rdx, rcx, r8, r9)
    const char** regs = func_call_regs;
    size_t reg_count = RegAlloc::used.size();
    
    // 从左到右计算参数并放入寄存器
    std::map<size_t, size_t> no_call_expr;
    for (size_t i = 0; i < call->arguments.size() && i < reg_count; i++) {

        
        if (call->arguments[i]->type != NodeType::FUNCTION_CALL) 
            no_call_expr[i] = i;
        else {
            gen(call->arguments[i]);
            output << "    mov " << regs[i] << ", rax" << std::endl;
        }
    }
    
    // 调用函数
    for (const auto& pair : no_call_expr) {
        gen(call->arguments[pair.first]);
        output << "    mov " << regs[pair.second] << ", rax" << std::endl;
    }
    output << "    call " << call->name << std::endl;
}

void WatGen::gen_return_stmt(const ASTNodePtr &node) {
    const auto ret = std::static_pointer_cast<ReturnStmtNode>(node);
    
    if (ret->expression) {
        gen(ret->expression);
    }
    
    has_return = true;
    output << "    leave" << std::endl;
    output << "    ret" << std::endl;
}

std::string WatGen::get_output() const {
    return output.str();
}

size_t WatGen::get_var_offset(const std::string& name) {
    auto it = var_offsets.find(name);
    if (it != var_offsets.end()) {
        return it->second;
    }
    // 不应该发生，说明变量未声明
    THROW_ERROR("Undefined variable: " + name, 0, 0);
}

int WatGen::new_label() {
    return label_counter++;
}

void WatGen::gen_if_stmt(const ASTNodePtr &node) {
    const auto ifStmt = std::static_pointer_cast<IfStmtNode>(node);
    
    int elseLabel = new_label();
    int endLabel = new_label();
    
    // 生成条件代码
    gen(ifStmt->condition);
    
    // 如果条件为假，跳转到 else
    output << "    test rax, rax" << std::endl;
    output << "    jz .L_else_" << elseLabel << std::endl;
    
    // then 分支
    for (const auto& stmt : ifStmt->thenBody) {
        gen(stmt);
    }
    output << "    jmp .L_end_" << endLabel << std::endl;
    
    // else 分支
    output << ".L_else_" << elseLabel << ":" << std::endl;
    for (const auto& stmt : ifStmt->elseBody) {
        gen(stmt);
    }
    
    output << ".L_end_" << endLabel << ":" << std::endl;
}

void WatGen::gen_for_stmt(const ASTNodePtr &node) {
    const auto forStmt = std::static_pointer_cast<ForStmtNode>(node);
    
    int startLabel = new_label();
    int endLabel = new_label();
    int continueLabel = new_label();
    
    // 初始化
    if (forStmt->init) {
        gen(forStmt->init);
    }
    
    // 循环开始
    output << ".L_for_start_" << startLabel << ":" << std::endl;
    
    // 条件检查
    if (forStmt->condition) {
        gen(forStmt->condition);
        output << "    test rax, rax" << std::endl;
        output << "    jz .L_for_end_" << endLabel << std::endl;
    }
    
    // 循环体
    for (const auto& stmt : forStmt->body) {
        gen(stmt);
    }
    
    // continue 标签（增量）
    output << ".L_for_continue_" << continueLabel << ":" << std::endl;
    if (forStmt->increment) {
        gen(forStmt->increment);
    }
    
    // 跳回开始
    output << "    jmp .L_for_start_" << startLabel << std::endl;
    
    // 循环结束
    output << ".L_for_end_" << endLabel << ":" << std::endl;
}

void WatGen::gen_break_stmt(const ASTNodePtr &node) {
    (void)node; // 简化处理，需要知道外层循环的结束标签
    output << "    ; break - needs outer loop context" << std::endl;
}

void WatGen::gen_continue_stmt(const ASTNodePtr &node) {
    (void)node; // 简化处理，需要知道外层循环的 continue 标签
    output << "    ; continue - needs outer loop context" << std::endl;
}

void WatGen::gen_macro_call(const ASTNodePtr &node) {
    const auto macro = std::static_pointer_cast<MacroCallNode>(node);

    if (macro->name == "syscall") {
        // vmcall!(syscall_number, args...)
        // 生成系统调用代码
        if (macro->arguments.empty()) {
            THROW_ERROR("vmcall! requires at least syscall number", macro->line, macro->col);
        }

        
        // 计算其他参数并放入寄存器 (rdi, rsi, rdx, r10, r8, r9)
        const char** regs = func_call_regs;
        for (size_t i = 1; i < macro->arguments.size() && i <= 6; i++) {
            gen(macro->arguments[i]);
            output << "    mov " << regs[i - 1] << ", rax" << std::endl;
        }
        gen(macro->arguments[0]);
        
        // 执行 syscall
        output << "    " + macro->name << std::endl;
        
        // vmcall 不会返回，标记函数已有返回
        has_return = true;
    } else if (macro->name == "strlen") {
        if (macro->arguments.size() == 1) {
            switch (macro->arguments[0]->type) {
            case NodeType::STRING: {
                output << "    mov rax, " << std::static_pointer_cast<StringNode>(macro->arguments[0])->value.length() << std::endl;
                break;
            }
            case NodeType::IDENTIFIER: {
                output << "    mov rax, " << var_str_lens[std::static_pointer_cast<IdentifierNode>(macro->arguments[0])->name] << std::endl;
                break;
            }
            default: THROW_ERROR("strlen!() should a strLiteral or strVar", macro->line, macro->col); break;
            }
        } else THROW_ERROR("strlen!() should a strLiteral or strVar", macro->line, macro->col);
    }
}

void WatGen::gen_macro_decl(const ASTNodePtr& node) {
    bool gen_ = true;
    const auto macro = std::static_pointer_cast<MacroDeclNode>(node);
    for (const auto& [name, v] : macro->equations) {
        if (name == "target") {
            if (std::static_pointer_cast<StringNode>(v)->value != P_TARGET) gen_ = false;
        }
        if (name == "extern") extern_flag = true;
    }
    if (gen_) gen(macro->declaration);
    extern_flag = false;
}


