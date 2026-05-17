#ifndef IR_HPP
#define IR_HPP

#include "ast.hpp"
#include "symbol_table.hpp"
#include <string>
#include <map>
#include <vector>
#include <sstream>

class CodeGenerator{
    private:
        std::ostringstream header;
        std::ostringstream body;

        Class_table& class_table;
        std::string current_class;
        int label_counter;
        int local_counter;
        int str_counter;

        std::map<std::string, std::vector<Field_info>> field_list; // ordered field list per class
        std::map<std::string, std::string> string_literals; // raw strings to global name
        std::map<std::string, std::string> var_addrs; // variable name to LLVM register

        std::string new_label(const std::string& s);
        std::string new_t();
        std::string mangle(std::string& cls, std::string& method);
        std::string llvm_type(std::string& vsop);
        bool is_primitive(std::string& vsop);
        std::string default_val(std::string& vsop);
        void addOutput(const std::string& s);
        void addDeclaOutput(const std::string& s);
        std::string process_string(std::string& s);
        void build_field_list(const std::string& cls_name);
        int field_index(std::string& cls, std::string& field_name);

        void generate_struct_types(Program* prog);
        void generate_vtable_types(Program* prog);
        void generate_vtable_instances(Program* prog);
        void generate_constructors(Program* prog);
        void generate_methods(Program* prog);

        std::string compile_expr(Expr* expr);

    public:
        CodeGenerator(Class_table& ct);
        std::string generate(Program* prog, const std::string& object_ll_path);

};


#endif