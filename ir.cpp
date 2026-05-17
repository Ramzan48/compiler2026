#include "ir.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;

CodeGenerator::CodeGenerator(Class_table& ct): class_table(ct), label_counter(0), local_counter(0), str_counter(0){}


string CodeGenerator::new_label(const string& s){
    return s + "." + to_string(label_counter++);
}

string CodeGenerator::new_t(){
    return "%t" + to_string(local_counter++);
}

void CodeGenerator::addOutput(const string& s){
    body << s << "\n";
}

void CodeGenerator::addDeclaOutput(const string& s){
    header << s << "\n";
}

// name mangling suggested in slide 66.
string CodeGenerator::mangle(string& cls, string& method){
    return "@" + cls + "__" + method;
}


string CodeGenerator::llvm_type(string& vsop)
{
    if(vsop == "int32") return "i32";
    if(vsop == "bool") return "i1";
    if(vsop == "string") return "i8*";
    if(vsop == "unit") return "i8*";
    return "%" + vsop + "*";
}

bool CodeGenerator::is_primitive(string& vsop){
    return vsop == "int32" || vsop == "bool" || vsop == "string" || vsop == "unit";
}

string CodeGenerator::default_val(string& vsop){
    if(vsop == "int32") return "0";
    if(vsop == "bool") return "false";
    if(vsop == "string") return "null";
    if(vsop == "unit") return "null";
    return "null";
}

string CodeGenerator::process_string(string& s){
    auto it = string_literals.find(s);
    if(it != string_literals.end()) return it->second;
    string name = "@.slit." + to_string(str_counter++);
    string_literals[s] = name;

    
    // remove ""
    string content = s;
    if(content.size() >= 2 && content.front() == '"' && content.back() == '"')
        content = content.substr(1, content.size() - 2);

    string bytes;
    for(size_t i = 0; i < content.size(); i++){
        if(content[i] == '\\' && i + 3 < content.size() && content[i+1] == 'x'){
            // \xHH counted as one byte
            string hex = content.substr(i + 2, 2);
            bytes += (char)stoi(hex, nullptr, 16);
            i += 3;
        } else {
            bytes += content[i];
        }
    }

    ostringstream llvm_str;
    size_t byte_count = 0;
    for(unsigned char c : bytes){
        if(c >= 32 && c <= 126 && c != '"' && c != '\\') {
            llvm_str << c;
        } else {
            llvm_str << "\\";
            llvm_str << hex << uppercase << setw(2) << setfill('0') << (int)c;
            llvm_str << dec;  // reset to decimal
        }
        byte_count++;
    }
    byte_count++;

    addDeclaOutput(name + " = private unnamed_addr constant [" + to_string(byte_count) + " x i8] c\"" + llvm_str.str() + "\\00\"");

    return name;
}

void collect_methods(string& cls_name, vector<string>& vtable_order, Class_table& class_table){
    if(cls_name == "Object") {
        vtable_order.push_back("print");
        vtable_order.push_back("printBool");
        vtable_order.push_back("printInt32");
        vtable_order.push_back("inputLine");
        vtable_order.push_back("inputBool");
        vtable_order.push_back("inputInt32");
        return;
    }

    Class_info* info = class_table.get_class(cls_name);
    if(!info) return;

    string parent = info->parent.empty() ? "Object" : info->parent;
    collect_methods(parent, vtable_order, class_table);

    if(info->ast_node)
        for(auto* m : info->ast_node->methods){
            bool is_override = false;
            for(auto& name : vtable_order)
                if(name == m->name) {
                    is_override = true;
                    break;
                }
            if(!is_override)
                vtable_order.push_back(m->name);
            
        } 

}

// This function takes an EXPR from the AST and produces LLVM instructions that compute its value.
// It returns the name of the register (or constant) that holds the result.
string CodeGenerator::compile_expr(Expr* expr){
    if(!expr) return "null";

    if(dynamic_cast<Self*>(expr))
        return "%self";

    if(auto* lit = dynamic_cast<IntLiteral*>(expr)) return to_string(lit->value);

    if(auto* lit = dynamic_cast<BoolLiteral*>(expr)) return lit->value ? "true" : "false";
    
    if(dynamic_cast<UnitExpr*>(expr)) return "null";

    if(auto* block = dynamic_cast<Block*>(expr)){
        if (block->exprs.empty()) return "null";
        string lastExpr;
        for (auto* expr : block->exprs)
            lastExpr = compile_expr(expr);
        return lastExpr;
    }

    if(auto* binop = dynamic_cast<BinaryOp*>(expr)){
        string left_expr = compile_expr(binop->left);
        string right_expr = compile_expr(binop->right);
        string reg = new_t();
        
        if(binop->op == "+") addOutput("  " + reg + " = add i32 " + left_expr + ", " + right_expr);
        else if(binop->op == "-") addOutput("  " + reg + " = sub i32 " + left_expr + ", " + right_expr);
        else if(binop->op == "*") addOutput("  " + reg + " = mul i32 " + left_expr + ", " + right_expr);
        else if(binop->op == "/") addOutput("  " + reg + " = sdiv i32 " + left_expr + ", " + right_expr);
        else if(binop->op == "^") addOutput("  " + reg + " = call i32 @__vsop_power(i32 " + left_expr + ", i32 " + right_expr + ")");
        else if(binop->op == "<") addOutput("  " + reg + " = icmp slt i32 " + left_expr + ", " + right_expr);
        else if(binop->op == "<=") addOutput("  " + reg + " = icmp sle i32 " + left_expr + ", " + right_expr);
        else if(binop->op == "="){
            string left_type = llvm_type(binop->left->type);
            addOutput("  " + reg + " = icmp eq " + left_type + " " + left_expr + ", " + right_expr);
        }
        else if(binop->op == "and") addOutput("  " + reg + " = and i1 " + left_expr + ", " + right_expr);

        return reg;
    }

    if(auto* unop = dynamic_cast<UnaryOp*>(expr)){
        string expr = compile_expr(unop->expr);
        string reg = new_t();

        if(unop->op == "-") addOutput("  " + reg + " = sub i32 0, " + expr);
        else if(unop->op == "not") addOutput("  " + reg + " = xor i1 " + expr + ", true");
        else if(unop->op == "isnull"){
            string expr_type = llvm_type(unop->expr->type);
            addOutput("  " + reg + " = icmp eq " + expr_type + " " + expr + ", null");
        }
        return reg;
        
    }

    if(auto* ifexpr = dynamic_cast<If*>(expr)){
        string then_label = new_label("if.then");
        string else_label = new_label("if.else");
        string end_label  = new_label("if.end");

        string cond_expr = compile_expr(ifexpr->cond);

        if(ifexpr->else_expr)
            addOutput("  br i1 " + cond_expr + ", label %" + then_label + ", label %" + else_label);
        else
            addOutput("  br i1 " + cond_expr + ", label %" + then_label + ", label %" + end_label);
        
        addOutput(then_label + ":");
        string then_expr = compile_expr(ifexpr->then_expr);

        string then_end_label = new_label("if.then.end");
        addOutput("  br label %" + then_end_label);
        addOutput(then_end_label + ":");
        addOutput("  br label %" + end_label);

        string else_expr;
        string else_end_label;
        if(ifexpr->else_expr){
            addOutput(else_label + ":");
            else_expr = compile_expr(ifexpr->else_expr);
            else_end_label = new_label("if.else.end");
            addOutput("  br label %" + else_end_label);
            addOutput(else_end_label + ":");
            addOutput("  br label %" + end_label);
        }

        addOutput(end_label + ":");

        if(ifexpr->else_expr && ifexpr->type != "unit"){
            string reg = new_t();
            addOutput("  " + reg + " = phi " + llvm_type(ifexpr->type) + " [" + then_expr + ", %" + then_end_label + "], [" + else_expr + ", %" + else_end_label + "]");
            return reg;
        }

        return "null";
    }

    if(auto* whileexpr = dynamic_cast<While*>(expr)){
        string cond_label = new_label("while.cond");
        string body_label = new_label("while.body");
        string end_label  = new_label("while.end");

        addOutput("  br label %" + cond_label);

        addOutput(cond_label + ":");
        string cond_expr = compile_expr(whileexpr->cond);
        addOutput("  br i1 " + cond_expr + ", label %" + body_label + ", label %" + end_label);

        addOutput(body_label + ":");
        compile_expr(whileexpr->body);
        addOutput("  br label %" + cond_label);

        addOutput(end_label + ":");

        return "null";
    }

    if (auto* lit = dynamic_cast<StringLiteral*>(expr)){
        string processed_str = process_string(lit->value);

        string s = lit->value;
        if(s.size() >= 2 && s.front() == '"' && s.back() == '"')
            s = s.substr(1, s.size() - 2);

        size_t byte_count = 0;
        for(size_t i = 0; i < s.size(); i++, byte_count++)
            if (s[i] == '\\' && i + 3 < s.size() && s[i+1] == 'x')
                i += 3;
        byte_count++;

        string reg = new_t();
        addOutput("  " + reg + " = getelementptr [" + to_string(byte_count) + " x i8], [" + to_string(byte_count) + " x i8]* " + processed_str + ", i64 0, i64 0");
        return reg;
    }

    if(auto* var = dynamic_cast<Variable*>(expr)){
        auto it = var_addrs.find(var->name);
        if(it != var_addrs.end()){
            string reg = new_t();
            string type = llvm_type(var->type);
            addOutput("  " + reg + " = load " + type + ", " + type + "* " + it->second);
            return reg;
        }

        // TODO: non local variable
        int idx = field_index(current_class, var->name);
        if(idx >= 0){
            string ptr = new_t();
            string reg = new_t();
            string type = llvm_type(var->type);
            addOutput("  " + ptr + " = getelementptr %" + current_class + ", %" + current_class + "* %self, i32 0, i32 " + to_string(idx));
            addOutput("  " + reg + " = load " + type + ", " + type + "* " + ptr);
            return reg;
        }

        return default_val(var->type);
    }

    if(auto* assign = dynamic_cast<Assignment*>(expr)){
        string assign_expr = compile_expr(assign->expr);
        string type = llvm_type(assign->expr->type);

        auto it = var_addrs.find(assign->name);
        if(it != var_addrs.end()){
            addOutput("  store " + type + " " + assign_expr + ", " + type + "* " + it->second);
            return assign_expr;
        }

        // TODO: field assignment
        int idx = field_index(current_class, assign->name);
        if(idx >= 0){
            string ptr = new_t();
            addOutput("  " + ptr + " = getelementptr %" + current_class + ", %" + current_class + "* %self, i32 0, i32 " + to_string(idx));
            addOutput("  store " + type + " " + assign_expr + ", " + type + "* " + ptr);
        }
        return assign_expr;
    }

    if(auto* new_ = dynamic_cast<New*>(expr)) {
        string reg = new_t();
        addOutput("  " + reg + " = call %" + new_->type_name + "* @" + new_->type_name + "___new()");
        return reg;
    }

    if(auto* let = dynamic_cast<Let*>(expr)){
        string type = llvm_type(let->var_type);
        string init_expr;
        if(let->init_expr) init_expr = compile_expr(let->init_expr);
        else init_expr = default_val(let->var_type);

        if(let->init_expr && !is_primitive(let->var_type) && let->init_expr->type != let->var_type){  // bitcast if types don't match
            string cast = new_t();
            addOutput("  " + cast + " = bitcast " + llvm_type(let->init_expr->type) + " " + init_expr + " to " + type);
            init_expr = cast;
        }

        string addr = new_t();
        addOutput("  " + addr + " = alloca " + type);
        addOutput("  store " + type + " " + init_expr + ", " + type + "* " + addr);

        string old_addr;
        bool exists = var_addrs.count(let->name);
        if(exists) old_addr = var_addrs[let->name];

        var_addrs[let->name] = addr;

        string reg = compile_expr(let->body);

        if (exists) var_addrs[let->name] = old_addr;
        else var_addrs.erase(let->name);

        return reg;
    }

    if(auto* call = dynamic_cast<Call*>(expr)){

        string obj;
        string obj_type;
        if(call->object){
            obj = compile_expr(call->object);
            obj_type = call->object->type;
        } else {
            // self
            obj = "%self";
            obj_type = current_class;
        }

        vector<pair<string,string>> args; // (llvm_type, value)
        for(auto* a : call->args)
            args.push_back({llvm_type(a->type), compile_expr(a)});
        
        vector<string> vtable_order;
        collect_methods(obj_type, vtable_order, class_table);

        int method_idx = -1;
        for(size_t i = 0; i < vtable_order.size(); i++)
            if (vtable_order[i] == call->method_name) {
                method_idx = (int)i;
                break;
            }
        
        string ret_t = llvm_type(call->type);
        ostringstream funtype;
        funtype << ret_t << " (%" << obj_type << "*";
        for(auto& a : args) funtype << ", " << a.first;
        funtype << ")*";


        string vtable_ptr_addr = new_t();
        string vtable_ptr = new_t();
        addOutput("  " + vtable_ptr_addr + " = getelementptr %" + obj_type + ", %" + obj_type + "* " + obj + ", i32 0, i32 0");
        addOutput("  " + vtable_ptr + " = load %" + obj_type + "VTable*, %" + obj_type + "VTable** " + vtable_ptr_addr);

        string fun_addr = new_t();
        string fun_ptr = new_t();
        addOutput("  " + fun_addr + " = getelementptr %" + obj_type + "VTable, %" + obj_type + "VTable* " + vtable_ptr + ", i32 0, i32 " + to_string(method_idx));
        addOutput("  " + fun_ptr + " = load " + funtype.str() + ", " + funtype.str() + "* " + fun_addr);

        string reg = new_t();
        ostringstream callstr;
        if(call->type != "unit") callstr << "  " << reg << " = ";
        else callstr << "  ";
        callstr << "call " << ret_t << " " << fun_ptr << "(%" << obj_type << "* " << obj;
        for (auto& a : args)
            callstr << ", " << a.first << " " << a.second;
        callstr << ")";
        addOutput(callstr.str());

        return call->type == "unit" ? "null" : reg;
        }
    // TODO: new
    return "null";

}

void CodeGenerator::build_field_list(const string& cls_name){
    if(field_list.count(cls_name)) return;
    if(cls_name == "Object"){
        field_list["Object"] = {};
        return;
    }

    Class_info* cls = class_table.get_class(cls_name);
    if(!cls){
        field_list[cls_name] = {};
        return;
    }

    string parent = cls->parent.empty() ? "Object" : cls->parent;
    build_field_list(parent);

    vector<Field_info> fields = field_list[parent];
    if(cls->ast_node) 
        for(auto* f : cls->ast_node->fields){
            Field_info field;
            field.name = f->name;
            field.type = f->type;
            field.ast_node = f;
            fields.push_back(field);
        }
    field_list[cls_name] = fields;

}
int CodeGenerator::field_index(string& cls, string& field){ 
    auto& fields = field_list[cls];
    for(size_t i = 0; i < fields.size(); i++)
        if(fields[i].name == field)
            return (int)i + 1;
    return -1;
}

struct VTable_elem{
    string method_name;
    string class_name;
    Method_info* method_info;
};

static vector<VTable_elem> collect_vtable(string& cls_name, Class_table& class_table){
    if(cls_name == "Object"){
        vector<VTable_elem> vtable;
        Class_info* obj = class_table.get_class("Object");
        vector<string> order = {"print", "printBool", "printInt32", "inputLine", "inputBool", "inputInt32"};
        for(auto method_name : order) {
            VTable_elem e;
            e.method_name = method_name;
            e.class_name = "Object";
            e.method_info = obj->get_method(string(method_name), class_table.get_classes());
            vtable.push_back(e);
        }
        return vtable;
    }

    Class_info* cls = class_table.get_class(cls_name);
    if(!cls) return {};

    string parent = cls->parent.empty() ? "Object" : cls->parent;
    vector<VTable_elem> vtable = collect_vtable(parent, class_table);

    // Check each method
    if (cls->ast_node)
        for(auto* m : cls->ast_node->methods){
            bool found = false;
            for(auto& v : vtable)
                if(v.method_name == m->name){ // Override
                    v.class_name = cls_name;
                    v.method_info = &cls->methods[m->name];
                    found = true;
                    break;
                }
            if(!found){
                VTable_elem e;
                e.method_name = m->name;
                e.class_name = cls_name;
                e.method_info = &cls->methods[m->name];
                vtable.push_back(e);
            }
        }
    return vtable;
}

void CodeGenerator::generate_struct_types(Program* prog){
    addDeclaOutput("");
    for(auto* cls : prog->classes) {
        build_field_list(cls->name);
        auto& fields = field_list[cls->name];

        ostringstream s;
        s << "%" << cls->name << " = type { %" << cls->name << "VTable*";
        for (auto& f : fields)
            s << ", " << llvm_type(f.type);
        s << " }";
        addDeclaOutput(s.str());
    }
}


void CodeGenerator::generate_vtable_types(Program* prog){
    addDeclaOutput("");
    for(auto* cls : prog->classes) {
        auto vtable = collect_vtable(cls->name, class_table);

        ostringstream s;
        s << "%" << cls->name << "VTable = type { ";
        for(size_t i = 0; i < vtable.size(); i++){
            if(i > 0) s << ", ";
            auto* m = vtable[i].method_info;
            s << llvm_type(m->return_type) << " (%" << cls->name << "*";
            for(auto& f : m->formals)
                s << ", " << llvm_type(f.type);
            s << ")*";
        }
        s << " }";
        addDeclaOutput(s.str());
    }
}

void CodeGenerator::generate_vtable_instances(Program* prog){
    addDeclaOutput("");
    for(auto* cls : prog->classes) {
        auto vtable = collect_vtable(cls->name, class_table);
        
        ostringstream s;
        s << "@" << cls->name << "___vtable = constant %"
          << cls->name << "VTable { ";

        for(size_t i = 0; i < vtable.size(); i++){
            if (i > 0) s << ", ";
            auto& v = vtable[i];
            auto* m = v.method_info;

            ostringstream expected;
            expected << llvm_type(m->return_type) << " (%" << cls->name << "*";
            for(auto& f : m->formals) expected << ", " << llvm_type(f.type);
            expected << ")*";

            ostringstream actual;
            actual << llvm_type(m->return_type) << " (%" << v.class_name << "*";
            for (auto& f : m->formals) actual << ", " << llvm_type(f.type);
            actual << ")*";

            string func_name = mangle(v.class_name, v.method_name);
            if (v.class_name!= cls->name)
                s << expected.str() << " bitcast (" << actual.str() << " " << func_name << " to " << expected.str() << ")"; // slide 63
            else
                s << expected.str() << " " << func_name;
        }
            s << " }";
            addDeclaOutput(s.str());
    }
}

void CodeGenerator::generate_constructors(Program* prog){
    for(auto* cls : prog->classes){
        string class_name = cls->name;
        current_class = class_name;
        string parent = cls->parent.empty() ? "Object" : cls->parent;

        addOutput("");

        addOutput("define %" + class_name + "* @" + class_name + "___new() {");
        addOutput("entry:");
        addOutput("  %size_ptr = getelementptr %" + class_name + ", %" + class_name + "* null, i32 1");
        addOutput("  %size = ptrtoint %" + class_name + "* %size_ptr to i64");
        addOutput("  %raw = call i8* @malloc(i64 %size)");
        addOutput("  %self = bitcast i8* %raw to %" + class_name + "*");
        addOutput("  %self2 = call %" + class_name + "* @" + class_name + "___init(%" + class_name + "* %self)");
        addOutput("  ret %" + class_name + "* %self2");
        addOutput("}");

        addOutput("define %" + class_name + "* @" + class_name + "___init(%" + class_name + "* %self) {");
        addOutput("entry:");

        addOutput("  %parent_ptr = bitcast %" + class_name + "* %self to %" + parent + "*");
        addOutput("  call %" + parent + "* @" + parent + "___init(%" + parent + "* %parent_ptr)");

        addOutput("  %vt_ptr = getelementptr %" + class_name + ", %" + class_name + "* %self, i32 0, i32 0");
        addOutput("  store %" + class_name + "VTable* @" + class_name + "___vtable, %" + class_name + "VTable** %vt_ptr");

        local_counter = 0;
        var_addrs.clear();

        int parent_count = 0;
        if (parent != "Object")
            parent_count = (int)field_list[parent].size();

        auto& fields = field_list[class_name];
        for(size_t i = parent_count; i < fields.size(); i++) {
            auto& f = fields[i];
            int vtable_idx = (int)i + 1;
            string type = llvm_type(f.type);

            string init_expr;
            if(f.ast_node && f.ast_node->init_expr) init_expr = compile_expr(f.ast_node->init_expr);
            else init_expr = default_val(f.type);

            string ptr = new_t();
            addOutput("  " + ptr + " = getelementptr %" + class_name + ", %" + class_name + "* %self, i32 0, i32 " + to_string(vtable_idx));
            addOutput("  store " + type + " " + init_expr + ", " + type + "* " + ptr);
        }
        addOutput("  ret %" + class_name + "* %self");
        addOutput("}");
    }
}

void CodeGenerator::generate_methods(Program* prog){
    for(auto* cls : prog->classes){
        current_class = cls->name;
        for(auto* method : cls->methods){
            var_addrs.clear();
            local_counter = 0;

            string func = mangle(cls->name, method->name);
            string ret_type = llvm_type(method->return_type);

            ostringstream info;
            info << "define " << ret_type << " " << func << "(%" << cls->name << "* %self";
            for(auto* f : method->formals)
                info << ", " << llvm_type(f->type) << " %p_" << f->name;
            info << ") {";

            addOutput("");
            addOutput(info.str());
            addOutput("entry:");

            for(auto* f : method->formals){
                string type = llvm_type(f->type);
                string ptr = new_t();
                addOutput("  " + ptr + " = alloca " + type);
                addOutput("  store " + type + " %p_" + f->name + ", " + type + "* " + ptr);
                var_addrs[f->name] = ptr;
            }

            string result = compile_expr(method->body);
            if(method->return_type == "unit") addOutput("  ret i8* null");
            else addOutput("  ret " + ret_type + " " + result);
            addOutput("}");
        }
    }
}


// vvlm power operator.
string generate_power_op(){
    ostringstream s;
    s << "define i32 @__vsop_power(i32 %base, i32 %exp) {\n";
    s << "entry:\n";
    s << "  %is_zero = icmp eq i32 %exp, 0\n";
    s << "  br i1 %is_zero, label %ret_one, label %loop_init\n";
    s << "ret_one:\n";
    s << "  ret i32 1\n";
    s << "loop_init:\n";
    s << "  br label %loop\n";
    s << "loop:\n";
    s << "  %result = phi i32 [1, %loop_init], [%new_result, %loop]\n";
    s << "  %i = phi i32 [0, %loop_init], [%next_i, %loop]\n";
    s << "  %new_result = mul i32 %result, %base\n";
    s << "  %next_i = add i32 %i, 1\n";
    s << "  %done = icmp eq i32 %next_i, %exp\n";
    s << "  br i1 %done, label %exit, label %loop\n";
    s << "exit:\n";
    s << "  ret i32 %new_result\n";
    s << "}\n";
    return s.str();
}


string CodeGenerator::generate(Program* prog, const string& object_ll_path){
    string object_ll;

    {
        ifstream f(object_ll_path, ios::in | ios::binary);
        if(f.is_open()){
            f.seekg(0, ios::end); // go to eof
            size_t size = f.tellg(); // get size
            f.seekg(0, ios::beg); // go to begining of file
            if(size > 0){
                object_ll.resize(size);
                f.read(&object_ll[0], size);
            }
        }
        f.close();
    }

    build_field_list("Object");
    for(auto* cls : prog->classes)
        build_field_list(cls->name);
    
    generate_struct_types(prog);
    generate_vtable_types(prog);
    generate_vtable_instances(prog);
    generate_constructors(prog);
    generate_methods(prog);

    // qsfqsfq
    addOutput("");
    addOutput("define i32 @main() {");
    addOutput("entry:");
    addOutput("  %m = call %Main* @Main___new()");
    addOutput("  %r = call i32 @Main__main(%Main* %m)");
    addOutput("  ret i32 %r");
    addOutput("}");
    
    ostringstream final_output;

    final_output << object_ll << "\n";
    final_output << header.str() << "\n";
    final_output << generate_power_op();
    final_output << body.str() << "\n";

    return final_output.str();
    
}
