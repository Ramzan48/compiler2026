#include "ast.hpp"

using namespace std;

void ASTNode::print_indentation(int indent) const {
    for(int i = 0; i < indent; i++) cout << " ";
}


IntLiteral::IntLiteral(int value): value(value){}
void IntLiteral::print(int indent) const{
    print_indentation(indent);
    cout << "IntLiteral: " << value << endl;
}



Formal::Formal(string name, string type): name(name), type(type){}
void Formal::print(int indent) const{
    print_indentation(indent);
    cout << "Formal: " << name << " : " << type << endl;
}



Field::Field(string name, string type, Expr* init_expr = nullptr): name(name), type(type), init_expr(init_expr){}
Field::~Field(){if(init_expr) delete init_expr;}
void Field::print(int indent) const{
    cout << "Field: " << name << " : " << type;
    if(init_expr){
        cout << " <- " << endl;
        init_expr->print(indent+1);
    } else cout << endl;
}



Method::Method(string name, vector<Formal*> formals, string return_type, Expr* body): name(name), formals(formals), return_type(return_type), body(body){}
Method::~Method(){
    for(auto f: formals) delete f;
    if(body) delete body;
}
void Method::print(int indent) const{
    print_indentation(indent);
    cout << "Method: " << name << "(";
    for (size_t i = 0; i < formals.size(); i++)
        if( i > 0) cout << ", ";
    cout << ") : " << return_type << endl;
    body->print(indent+1);
}



Class::Class(string name, string parent, vector<Field*> fields, vector<Method*> methods) : name(name), parent(parent), fields(fields), methods(methods){}
Class::~Class(){
    for(auto f: fields) delete f;
    for(auto m: methods) delete m;
}
void Class::print(int indent) const {
    print_indentation(indent);
    cout << "Class: " << name;
    if(!parent.empty()) cout << "extends " << parent;
    for(auto f: fields) f->print(indent+1);
    for(auto m: methods) m->print(indent+1);
}



Program::Program(vector<Class*> classes): classes(classes){}
Program::~Program(){
    for(auto c: classes) delete c;
}
void Program::print(int indent) const{
    print_indentation(indent);
    cout << "Program" << endl;
    for(auto c: classes) c->print(indent+1);
}

