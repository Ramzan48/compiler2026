#include "symbol_table.hpp"
#include "ast.hpp"
#include <set>

Method_info::Method_info(): ast_node(nullptr), line(0), col(0){}
Field_info::Field_info(): ast_node(nullptr), line(0), col(0){}
Class_info::Class_info(): ast_node(nullptr), line(0), col(0){}

bool Class_info::has_field(std::string &field_name, std::map<std::string, Class_info> &class_table){
    if(fields.find(field_name) != fields.end()) return true;

    if(!parent.empty() && parent != "Object"){
        auto it = class_table.find(parent);
        if(it != class_table.end())
            return it->second.has_field(field_name, class_table);
    }
    return false;
}

std::string Class_info::get_field_type(std::string &field_name, std::map<std::string, Class_info> &class_table){
    auto it = fields.find(field_name);
    if(it != fields.end()) return it->second.type;

    if(!parent.empty() && parent != "Object"){
        auto it = class_table.find(parent);
        if(it != class_table.end())
            return it->second.get_field_type(field_name, class_table);
    }
    return "";
}

bool Class_info::has_method(std::string &method_name, std::map<std::string, Class_info> &class_table){
    if(methods.find(method_name) != methods.end()) return true;

    if(!parent.empty() && parent != "Object"){
        auto it = class_table.find(parent);
        if(it != class_table.end())
            return it->second.has_method(method_name, class_table);
    }
    
    if(parent == "Object" || name == "Object"){
        auto it = class_table.find("Object");
        if(it != class_table.end() && it->second.methods.find(method_name) != it->second.methods.end())
            return true;
    }

    return false;
}

Method_info *Class_info::get_method(const std::string &method_name, std::map<std::string, Class_info> &class_table){
    auto it = methods.find(method_name);
    if (it != methods.end()) {
        return &it->second;
    }

    if(!parent.empty()){
        auto it = class_table.find(parent);
        if (it != class_table.end())
            return it->second.get_method(method_name, class_table);
    }    

    return nullptr;
}

bool Class_info::extends(std::string &ancestor, std::map<std::string, Class_info> &class_table){
    if(name == ancestor) return true;
    if(parent.empty()) return false;
    if(parent == ancestor) return true;

    auto it = class_table.find(parent);
    if(it != class_table.end())
        return it->second.extends(ancestor, class_table);

    return false;
}

Scope::Scope(Scope *enclosing_scope): enclosing_scope(enclosing_scope){}

void Scope::add_variable(std::string &name, std::string &type){variables[name] = type;}

std::string Scope::lookup(std::string &name){
    auto it = variables.find(name);
    if(it != variables.end()) return it->second;
    if(enclosing_scope) return enclosing_scope->lookup(name);
    return "";
}

bool Scope::has_local(std::string &name){return variables.find(name) != variables.end();}

Class_table::Class_table(){
    // VSOP manual p10
    Class_info object_class;
    object_class.name = "Object";
    object_class.parent = "";

    Method_info print_method;
    print_method.name = "print";
    print_method.formals.push_back({"s", "string", 0, 0});
    print_method.return_type = "Object";
    print_method.ast_node = nullptr;
    object_class.methods["print"] = print_method;

    Method_info printBool_method;
    printBool_method.name = "printBool";
    printBool_method.formals.push_back({"b", "bool", 0, 0});
    printBool_method.return_type = "Object";
    object_class.methods["printBool"] = printBool_method;

    Method_info printInt32_method;
    printInt32_method.name = "printInt32";
    printInt32_method.formals.push_back({"i", "int32", 0, 0});
    printInt32_method.return_type = "Object";
    object_class.methods["printInt32"] = printInt32_method;

    Method_info inputLine_method;
    inputLine_method.name = "inputLine";
    inputLine_method.return_type = "string";
    object_class.methods["inputLine"] = inputLine_method;

    Method_info inputBool_method;
    inputBool_method.name = "inputBool";
    inputBool_method.return_type = "bool";
    object_class.methods["inputBool"] = inputBool_method;

    Method_info inputInt32_method;
    inputInt32_method.name = "inputInt32";
    inputInt32_method.return_type = "int32";
    object_class.methods["inputInt32"] = inputInt32_method;

    classes["Object"] = object_class;
}

void Class_table::add_class(std::string &name, Class_info &info){classes[name] = info;}

bool Class_table::has_class(std::string &name){return classes.find(name) != classes.end();}

Class_info *Class_table::get_class(const std::string &name){
    auto it = classes.find(name);
    return it != classes.end() ? &it->second : nullptr;
}

std::map<std::string, Class_info> &Class_table::get_classes(){return classes;}

bool Class_table::has_cycle(const std::string &class_name){
    std::set<std::string> visited;
    std::string current = class_name;
    while(!current.empty() && current != "Object"){
        if(visited.count(current)) return true;
        visited.insert(current);
        auto it = classes.find(current);
        if(it == classes.end()) return false;
        current = it->second.parent;
    }
    return false;
}

std::string Class_table::least_common_ancestor(std::string &type1, std::string &type2){

    // page 12 vsop manual
    if(type1 == type2) return type1;
    if(type1 == "unit" || type2 == "unit") return "unit";
    if(type1 == "int32" || type2 == "int32" || type1 == "bool"  || type2 == "bool"  || type1 == "string"|| type2 == "string") return "Object"; 
    if(type1 == "Object" || type2 == "Object") return "Object";

    std::set<std::string> ancestors;
    std::string current = type1;
    while(!current.empty()){
        ancestors.insert(current);
        if(current == "Object") break;
        auto it = classes.find(current);
        if(it == classes.end()) break;
        current = it->second.parent;
    }

    current = type2;
    while(!current.empty()){
        if(ancestors.count(current)) return current;
        if(current == "Object") break;
        auto it = classes.find(current);
        if(it == classes.end()) break;
        current = it->second.parent;
    }
    return "Object";
}

bool Class_table::is_subtype(std::string &type1, std::string &type2){
    if(type1 == "int32" || type1 == "bool" || type1 == "string" || type1 == "unit")
        return type1 == type2;

    if(type1 == type2) return true;

    auto it = classes.find(type1);
    if(it == classes.end()) return false;
    return it->second.extends(type2, classes);
}


