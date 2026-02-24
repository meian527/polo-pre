//
// Created by geguj on 2026/2/21.
//

#ifndef POLO_COMPILER_PRE_WATGEN_HPP
#define POLO_COMPILER_PRE_WATGEN_HPP
#include "../ast.h"
#include <sstream>
#include <string>
#include <unordered_map>


class WatGen {
public:
    bool extern_flag{false};
    std::vector<std::string> using_namespace;
    explicit WatGen()  {
        using_namespace.emplace_back("");
    }
    ~WatGen() = default;

    void gen(const ASTNodePtr& node);
    std::string get_output() const;
    
#define decl_gen_tool(name) void gen_##name(const ASTNodePtr& node);
    decl_gen_tool(function);
    decl_gen_tool(var);
    decl_gen_tool(binary);
    decl_gen_tool(number);
    decl_gen_tool(macro_decl);
    decl_gen_tool(float);
    decl_gen_tool(program);
    decl_gen_tool(string);
    decl_gen_tool(assignment);
    decl_gen_tool(boolean);
    decl_gen_tool(identifier);
    decl_gen_tool(function_call);
    decl_gen_tool(return_stmt);
    decl_gen_tool(if_stmt);
    decl_gen_tool(for_stmt);
    decl_gen_tool(break_stmt);
    decl_gen_tool(continue_stmt);
    decl_gen_tool(macro_call);
    decl_gen_tool(unary);
#undef decl_gen_tool

private:
    size_t str_len;
    std::string var_operation{"mov"};
    std::ostringstream output;
    std::ostringstream data_output;
    std::unordered_map<std::string, size_t> var_offsets;
    std::unordered_map<std::string, size_t> var_str_lens;
    std::unordered_map<std::string, int> string_labels;
    size_t stack_offset = 0;
    bool has_return = false;
    int label_counter = 0;
    int string_counter = 0;
    
    size_t get_var_offset(const std::string& name);
    int new_label();
    int gen_string_data(const std::string& str);
    size_t var_size{0};
};


#endif //POLO_COMPILER_PRE_WATGEN_HPP