#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP


#include <string>
#include <vector>
#include <map>

class Class;
class Method;
class Field;

struct Formal_info{
    std::string name;
    std::string type;
    int line;
    int col;
};

struct Method_info{
    std::string name;
    std::vector<Formal_info> formals;
    std::string return_type;
    Method *ast_node;
    int line;
    int col;

    Method_info();
};

struct Field_info{
    std::string name;
    std::string type;
    Field *ast_node;
    int line;
    int col;

    Field_info();
};

struct Class_info{
    std::string name;
    std::string parent;
    std::map<std::string, Field_info> fields;
    std::map<std::string, Method_info> methods;
    Class *ast_node;
    int line;
    int col;

    Class_info();
    bool has_field(std::string& field_name, std::map<std::string, Class_info>& class_table);
    std::string get_field_type(std::string& field_name, std::map<std::string, Class_info>& class_table);
    bool has_method(std::string& method_name, std::map<std::string, Class_info>& class_table);
    Method_info* get_method(const std::string& method_name, std::map<std::string, Class_info>& class_table);
    bool extends(std::string& ancestor, std::map<std::string, Class_info>& class_table);
};

class Scope{
    private:
        std::map<std::string, std::string> variables;
        Scope* enclosing_scope;

    public:
        Scope(Scope* enclosing_scope = nullptr);
        void add_variable(std::string& name, std::string& type);
        std::string lookup(std::string& name);
        bool has_local(std::string& name);
};

class Class_table{
    private:
         std::map<std::string, Class_info> classes;
    public:
        Class_table();
        void add_class(std::string& name, Class_info& info);
        bool has_class(std::string& name);
        Class_info* get_class(const std::string& name);
        std::map<std::string, Class_info>& get_classes();
        bool has_cycle(const std::string& class_name);
        std::string least_common_ancestor(std::string& type1, std::string& type2);
        bool is_subtype(std::string& type1, std::string& type2);
};

#endif