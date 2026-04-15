#include "type_checker.hpp"
#include <iostream>
#include <set>

using namespace std;

TypeChecker::TypeChecker(const string& filename): filename(filename), has_errors(false), current_class(""), is_in_field_init(false){}

void TypeChecker::error(int line, int col, const string& message){
    cerr << filename << ":" << line << ":" << col << ": semantic error: " << message << endl;
    has_errors = true;
}

bool TypeChecker::check(Program* program){
    if (!program) return false;

    //pass 1
    collect_classes(program);
    //pass 2
    check_inheritance(program);
    //pass 3
    collect_members(program);
    check_main_class();
    // pass 4
    if(!has_errors) type_check(program);

    return !has_errors;
}

void TypeChecker::collect_classes(Program *program){
    for(auto cls: program->classes){
        if(class_table.has_class(cls->name)){
            error(cls->line, cls->col, "redefinition of class " + cls->name);
            continue;
        }
        if(cls->name == "Object") {
            error(cls->line, cls->col, "cannot redefine class Object");
            continue;
        }
        Class_info info;
        info.name = cls->name;
        info.parent = cls->parent.empty() ? "Object" : cls->parent;
        info.ast_node = cls;
        info.line = 1;  // TODO: Get from AST
        info.col = 1;

        class_table.add_class(cls->name, info);
    }
}

void TypeChecker::check_inheritance(Program *program){
    for(auto cls: program->classes){
        if(!cls->parent.empty() && !class_table.has_class(cls->parent))
            error(cls->line, cls->col, "class " + cls->name + " extends unknown class " + cls->parent);
        
        if(class_table.has_cycle(cls->name))
            error(cls->line, cls->col, "class " + cls->name + " involved in inheritance cycle");
    }
}

void TypeChecker::collect_members(Program *program){
    for(auto cls : program->classes){
        Class_info* info = class_table.get_class(cls->name);
        if(!info) return;

        for(auto field: cls->fields){
            if(info->fields.find(field->name) != info->fields.end()) {
                error(field->line, field->col, "duplicate field " + field->name + " in class " + cls->name);
                continue;
            }

            // submit platform 42: need to check if field exist in any parent class
            if(!cls->parent.empty() && cls->parent != "Object"){
                Class_info* parent_info = class_table.get_class(cls->parent);
                if(parent_info && parent_info->has_field(field->name, class_table.get_classes()))
                    error(field->line, field->col, "redefinition of field " + field->name);
            }

            if(field->type != "int32" && field->type != "bool" && field->type != "string" && field->type != "unit" && !class_table.has_class(field->type))
                error(field->line, field->col, "field " + field->name + " has undefined type " + field->type);

            Field_info field_info;
            field_info.name = field->name;
            field_info.type = field->type;
            field_info.ast_node = field;
            info->fields[field->name] = field_info;
        }

        for(auto method: cls->methods){
            if(info->methods.find(method->name) != info->methods.end()) {
                error(method->line, method->col, "duplicate method " + method->name + " in class " + cls->name);
                continue;
            }

            Method_info method_info;
            method_info.name = method->name;
            method_info.return_type = method->return_type;
            method_info.ast_node = method;

            set<string> formal_names;
            for(auto formal: method->formals){
                if(formal_names.find(formal->name) != formal_names.end()){
                    error(formal->line, formal->col, "duplicate formal name " + formal->name);
                    continue;
                }
                formal_names.insert(formal->name);
                if(formal->type != "int32" && formal->type != "bool" && formal->type != "string" && formal->type != "unit" && !class_table.has_class(formal->type))
                    error(formal->line, formal->col, "formal " + formal->name + " has unknwon type " + formal->type);

                Formal_info formal_info;
                formal_info.name = formal->name;
                formal_info.type = formal->type;
                method_info.formals.push_back(formal_info);
            }

            if(method->return_type != "int32" && method->return_type != "bool" && method->return_type != "string" && method->return_type != "unit" && !class_table.has_class(method->return_type))
                error(method->line, method->col, "method " + method->name + " has undefined return type " + method->return_type);
            
            info->methods[method->name] = method_info;
            
            Class_info* parent = class_table.get_class(cls->parent);
            if(!parent) continue;

            Method_info* parent_method = parent->get_method(method_info.name, class_table.get_classes());
            if(!parent_method) continue;

            if(method_info.return_type != parent_method->return_type){
                error(method->line, method->col, "overriding method " + method_info.name + " has different return type");
                continue;
            }

            if(method_info.formals.size() != parent_method->formals.size()){
                error(method->line, method->col, "overriding method " + method_info.name + " has different number of parameters");
                continue;
            }

            for(size_t i = 0; i < method_info.formals.size(); i++) {
                if(method_info.formals[i].type != parent_method->formals[i].type){
                    error(method->line, method->col, "overriding method " + method_info.name + " has different parameter types");
                    continue;
                }
            }
        }
    }
}

void TypeChecker::check_main_class(){
    Class_info* main_class = class_table.get_class("Main");
    if(!main_class){
        error(1, 1, "no Main class defined");
        return;
    }

    Method_info* main_method = main_class->get_method("main", class_table.get_classes());
    if(!main_method){
        error(main_class->line, main_class->col, "no main() method inside Main class");
        return;
    }

    if(!main_method->formals.empty()){
        error(main_class->line, main_class->col, "main() method must be empty");
        return;
    }

    if(main_method->return_type != "int32") {
        error(main_class->line, main_class->col, "main() method must return int32");
        return;
    }
}

void TypeChecker::type_check(Program *program){
    for(auto cls: program->classes){
        current_class = cls->name;
    
        for(auto field: cls->fields){
            if(!field->init_expr) continue;
            Scope scope;
            is_in_field_init = true;
            string init_type = type_check_expr(field->init_expr, scope);
            is_in_field_init = false;
            if(!class_table.is_subtype(init_type, field->type))
                error(field->line, field->col, "field " + field->name + " initializer type " + init_type + " is not the same as declared type " + field->type);
        }

        for(auto method: cls->methods){
            Scope scope;
            for(auto formal: method->formals)
                scope.add_variable(formal->name, formal->type);

            string body_type = type_check_expr(method->body, scope);
            if(!class_table.is_subtype(body_type, method->return_type))
                error(method->line, method->col, "method " + method->name + " body type " + body_type + " is not the same as return type " + method->return_type);
        }
    }
}

std::string TypeChecker::type_check_expr(Expr *expr, Scope &scope){
    if(!expr) return "unit";

    if(IntLiteral* lit = dynamic_cast<IntLiteral*>(expr)){
        lit->type = "int32";
        return "int32";
    }
    if(StringLiteral* lit = dynamic_cast<StringLiteral*>(expr)){
        lit->type = "string";
        return "string";
    }
    if(BoolLiteral* lit = dynamic_cast<BoolLiteral*>(expr)){
        lit->type = "bool";
        return "bool";
    }
    if(UnitExpr* unit = dynamic_cast<UnitExpr*>(expr)){
        unit->type = "unit";
        return "unit";
    }
    if(Variable* var = dynamic_cast<Variable*>(expr)){
        return type_check_variable(var, scope);
    }
    if(Self* self = dynamic_cast<Self*>(expr)){
        if(is_in_field_init)
            error(self->line, self->col, "can't use self in field initializers");
        self->type = current_class;
        return current_class;
    }
    if(BinaryOp* binop = dynamic_cast<BinaryOp*>(expr)){
        return type_check_binary_op(binop, scope);
    }
    if(UnaryOp* unop = dynamic_cast<UnaryOp*>(expr)){
        return type_check_unary_op(unop, scope);
    }
    if(Assignment* assign = dynamic_cast<Assignment*>(expr)){
        return type_check_assignment(assign, scope);
    }
    if(Block* block = dynamic_cast<Block*>(expr)){
        return type_check_block(block, scope);
    }
    if(If* ifexpr = dynamic_cast<If*>(expr)){
        return type_check_if(ifexpr, scope);
    }
    if(While* whileexpr = dynamic_cast<While*>(expr)){
        return type_check_while(whileexpr, scope);
    }
    if(Let* let = dynamic_cast<Let*>(expr)){
        return type_check_let(let, scope);
    }
    if(New* newexpr = dynamic_cast<New*>(expr)){
        return type_check_new(newexpr);
    }
    if(Call* call = dynamic_cast<Call*>(expr)){
        return type_check_call(call, scope);
    }
    return "Object";
}

std::string TypeChecker::type_check_variable(Variable *var, Scope &scope){
    string type = scope.lookup(var->name);
    if(!type.empty()){
        var->type = type;
        return type;
    }

    Class_info* cls = class_table.get_class(current_class);
    if(cls && cls->has_field(var->name, class_table.get_classes())){
        if(is_in_field_init){
            error(var->line, var->col, "cannot use class fields in field initializers.");
            error(var->line, var->col, "use of unbound variable " + var->name);
            var->type = "Object";
            return "Object";
        }
        type = cls->get_field_type(var->name, class_table.get_classes());
        var->type = type;
        return type;
    }


    error(var->line, var->col, "use of unbound variable " + var->name);
    var->type = "Object";
    return "Object";
}

std::string TypeChecker::type_check_binary_op(BinaryOp *binop, Scope &scope){
    string left_type = type_check_expr(binop->left, scope);
    string right_type = type_check_expr(binop->right, scope);

    if(binop->op == "+" || binop->op == "-" || binop->op == "*" || binop->op == "/" || binop->op == "^"){
        if(left_type != "int32" || right_type != "int32")
            error(binop->line, binop->col, "must be int32, found " + left_type + " and " + right_type);
        binop->type = "int32";
        return "int32";
    }

    if(binop->op == "<" || binop->op == "<="){
        if(left_type != "int32" || right_type != "int32")
            error(binop->line, binop->col, "must be int32, found " + left_type + " and " + right_type);
        binop->type = "bool";
        return "bool";
    }

    if(binop->op == "="){
        if(left_type != right_type)
            error(binop->line, binop->col, "equality needs sames types, found " + left_type + " and " + right_type);
        binop->type = "bool";
        return "bool";
    }
    if(binop->op == "and"){
        if(left_type != "bool" || right_type != "bool")
            error(binop->line, binop->col, "must be bool, found " + left_type + " and " + right_type);
        binop->type = "bool";
        return "bool";
    }

    binop->type = "Object";
    return "Object";
}

std::string TypeChecker::type_check_unary_op(UnaryOp *unop, Scope &scope){
    string expr_type = type_check_expr(unop->expr, scope);

    if(unop->op == "-"){
        if(expr_type != "int32")
            error(unop->line, unop->col, "unary minus needs int32, found " + expr_type);
        unop->type = "int32";
        return "int32";
    }

    if(unop->op == "not"){
        if(expr_type != "bool")
            error(unop->line, unop->col, "not needs bool, found " + expr_type);
        unop->type = "bool";
        return "bool";
    }

    if(unop->op == "isnull"){
        // submit platform 50, fix simply in vsopc manual p9
        if (expr_type == "int32" || expr_type == "bool" || expr_type == "string" || expr_type == "unit")
            error(unop->line, unop->col, "this expression has type " + expr_type + ", but expected type was Object");
        unop->type = "bool";
        return "bool";
    }

    unop->type = "Object";
    return "Object";
}

std::string TypeChecker::type_check_assignment(Assignment *assign, Scope &scope){
    string var_type = scope.lookup(assign->name);

    if(var_type.empty()){
        Class_info* cls = class_table.get_class(current_class);
        if(cls && cls->has_field(assign->name, class_table.get_classes()))
            var_type = cls->get_field_type(assign->name, class_table.get_classes());
        else{
            error(assign->line, assign->col, "assignment to unknown variable " + assign->name);
            var_type = "Object";
        }
    }

    string expr_type = type_check_expr(assign->expr, scope);
    if(!class_table.is_subtype(expr_type, var_type))
        error(assign->line, assign->col, "can't assign " + expr_type + " to " + assign->name + " of type " + var_type);
    assign->type = expr_type;
    return expr_type;
}

std::string TypeChecker::type_check_block(Block *block, Scope &scope){
    if(block->exprs.empty()){
        block->type = "unit";
        return "unit";
    }

    string last_type;
    for(auto expr: block->exprs)
        last_type = type_check_expr(expr, scope);
    block->type = last_type;
    return last_type;
}

std::string TypeChecker::type_check_if(If *ifexpr, Scope &scope){
    string cond_type = type_check_expr(ifexpr->cond, scope);
    if(cond_type != "bool")
        error(ifexpr->line, ifexpr->col, "if condition must be bool, found " + cond_type);
    
    string then_type = type_check_expr(ifexpr->then_expr, scope);
    if(ifexpr->else_expr){
        string else_type = type_check_expr(ifexpr->else_expr, scope);
        string result_type = class_table.least_common_ancestor(then_type, else_type); // return common type
        ifexpr->type = result_type;
        return result_type;
    } else {
        ifexpr->type = "unit";
        return "unit";
    }
}

std::string TypeChecker::type_check_while(While *whileexpr, Scope &scope){
    string cond_type = type_check_expr(whileexpr->cond, scope);
    if(cond_type != "bool")
        error(whileexpr->line, whileexpr->col, "while condition must be bool, found " + cond_type);
    type_check_expr(whileexpr->body, scope);
    whileexpr->type = "unit";
    return "unit";
}

std::string TypeChecker::type_check_let(Let *let, Scope &scope){
    if(let->var_type != "int32" && let->var_type != "bool" && let->var_type != "string" && let->var_type != "unit" && !class_table.has_class(let->var_type))
        error(let->line, let->col, "let variable " + let->name + " has unknown type " + let->var_type);
    
    if(let->init_expr){
        string init_type = type_check_expr(let->init_expr, scope);
        if(!class_table.is_subtype(init_type, let->var_type))
            error(let->line, let->col, "let initializer type " + init_type + " is not compatible with declared type " + let->var_type);
    }

    Scope body_scope(&scope);
    body_scope.add_variable(let->name, let->var_type);
    string body_type = type_check_expr(let->body, body_scope);
    let->type = body_type;
    return body_type;
}

std::string TypeChecker::type_check_new(New *newexpr){
    if(!class_table.has_class(newexpr->type_name)){
        error(newexpr->line, newexpr->col, "new expression references unknown class " + newexpr->type_name);
        newexpr->type = "Object";
        return "Object";
    }

    newexpr->type = newexpr->type_name;
    return newexpr->type_name;
}

std::string TypeChecker::type_check_call(Call *call, Scope &scope){
    string object_type;
    if(call->object)
        object_type = type_check_expr(call->object, scope);
    else{
        if(is_in_field_init){ // fix submission platform 70.
            error(call->line, call->col, "cannot find method " + call->method_name + " in type <invalid-type>.");
            error(call->line, call->col, "cannot use self in field initializer.");
            error(call->line, call->col, "use of unbound variable self.");
            
            call->type = "Object";
            return "Object";
        }
        call->object = new Self();
        call->object->type = current_class;
        object_type = current_class;
    }

    Class_info* cls = class_table.get_class(object_type);
    if(!cls){
        error(call->line, call->col, "call on unknown type " + object_type);
        call->type = "Object";
        return "Object";
    }

    Method_info* method = cls->get_method(call->method_name, class_table.get_classes());
    if(!method){
        error(call->line, call->col, "class " + object_type + " has no method " + call->method_name);
        call->type = "Object";
        return "Object";
    }

    if(call->args.size() != method->formals.size()){
        error(call->line, call->col, "method " + call->method_name + " expects " + to_string(method->formals.size()) + " arguments, got " + to_string(call->args.size()));
        call->type = method->return_type;
        return method->return_type;
    }

    for(size_t i = 0; i < call->args.size(); i++){
        string arg_type = type_check_expr(call->args[i], scope);
        if(!class_table.is_subtype(arg_type, method->formals[i].type))
            error(call->line, call->col, "argument " + to_string(i+1) + " from " + call->method_name + " has type " + arg_type + ", expected " + method->formals[i].type);
    }

    call->type = method->return_type;
    return method->return_type;

}