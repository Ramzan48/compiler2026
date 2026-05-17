#ifndef TYPE_CHECKER_HPP
#define TYPE_CHECKER_HPP

#include "ast.hpp"
#include "symbol_table.hpp"
#include <string>

class TypeChecker{
    private:
        Class_table class_table;
        std::string filename;
        bool has_errors;
        std::string current_class;
        bool is_in_field_init;

        void error(int line, int col, const std::string& message);

        // pass 1
        void collect_classes(Program* program);
        // pass 2
        void check_inheritance(Program* program);
        // pass 3
        void collect_members(Program* program);
        void check_main_class();
        // pass 4
        void type_check(Program* program);

        std::string type_check_expr(Expr* expr, Scope& scope);
        std::string type_check_variable(Variable* var, Scope& scope);
        std::string type_check_binary_op(BinaryOp* binop, Scope& scope);
        std::string type_check_unary_op(UnaryOp* unop, Scope& scope);
        std::string type_check_assignment(Assignment* assign, Scope& scope);
        std::string type_check_block(Block* block, Scope& scope);
        std::string type_check_if(If* ifexpr, Scope& scope);
        std::string type_check_while(While* whileexpr, Scope& scope);
        std::string type_check_let(Let* let, Scope& scope);
        std::string type_check_new(New* newexpr);
        std::string type_check_call(Call* call, Scope& scope);

    public:
        TypeChecker(const std::string& filename);
        bool check(Program* program);
        Class_table& get_class_table(){return class_table;}
};

#endif