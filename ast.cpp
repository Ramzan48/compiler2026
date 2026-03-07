#include "ast.hpp"

using namespace std;

BinaryOp::BinaryOp(string op, Expr* left, Expr* right): op(op), left(left), right(right){}
BinaryOp::~BinaryOp(){delete left; delete right;}
void BinaryOp::print() const {
    cout << "BinOp(" << op << ", ";
    left->print();
    cout << ", ";
    right->print();
    cout << ")";
}

UnaryOp::UnaryOp(std::string op, Expr* expr): op(op), expr(expr){}
UnaryOp::~UnaryOp(){delete expr;}
void UnaryOp::print() const {
    cout << "UnOp(" << op << ", ";
    expr->print();
    cout << ")";
}

New::New(string type_name): type_name(type_name){}
void New::print() const {cout << "New(" << type_name << ")";}

Let::Let(string name, string type, Expr* init_expr, Expr* body): name(name), type(type), init_expr(init_expr), body(body) {}
Let::~Let(){if(init_expr) delete init_expr; delete body;}
void Let::print() const {
    cout << "Let(" << name << ", " << type << ", ";
    if(init_expr){
        init_expr->print();
        cout << ", ";
    }
    body->print();
    cout << ")";
}

While::While(Expr* cond, Expr* body): cond(cond), body(body) {}
While::~While(){delete cond; delete body;}
void While::print() const {
    cout << "While(";
    cond->print();
    cout << ", ";
    body->print();
    cout << ")";
}

If::If(Expr* cond, Expr* then_expr, Expr* else_expr = nullptr): cond(cond), then_expr(then_expr), else_expr(else_expr) {}
If::~If(){delete cond; delete then_expr; delete else_expr;}
void If::print()  const{
    cout << "If(";
    cond->print();
    cout << ", ";
    then_expr->print();
    if(else_expr){
        cout << ", ";
        else_expr->print();
    }
    cout << ")";
}

Assignment::Assignment(string name, Expr* expr): name(name), expr(expr){}
Assignment::~Assignment(){delete expr;}
void Assignment::print() const{
    cout << "Assign(" << name << ", ";
    expr->print();
    cout << ")";
}

Variable::Variable(string name): name(name){}
void Variable::print() const{cout << name;}

Block::Block(vector<Expr*> exprs): exprs(exprs){};
Block::~Block(){for(auto e: exprs) delete e;}
void Block::print() const {
    cout << "[";
    for(size_t i = 0; i < exprs.size(); i++){
        if(i > 0) cout << ", ";
        exprs[i]->print();
    }
    cout << "]";
}

Call::Call(Expr* object, string method_name, vector<Expr*> args): object(object), method_name(method_name), args(args){}
Call::~Call(){if(object) delete object; for(auto a: args) delete a;}
void Call::print() const{
    cout << "Call(";
    if(object) object->print();
    else cout << "self";
    cout << ", " << method_name << ", [";
    for(size_t i=0; i < args.size(); i++){
        if(i>0) cout << ", ";
            args[i]->print();
    }
    cout << "])";
}


StringLiteral::StringLiteral(string value): value(value){}
void StringLiteral::print() const{cout << value;}

IntLiteral::IntLiteral(int value): value(value){}
void IntLiteral::print() const{cout << value;}

BoolLiteral::BoolLiteral(bool value): value(value){}
void BoolLiteral::print() const{cout << (value ? "true" : "false");}



Formal::Formal(string name, string type): name(name), type(type){}
void Formal::print() const{cout << name << " : " << type;}



Field::Field(string name, string type, Expr* init_expr = nullptr): name(name), type(type), init_expr(init_expr){}
Field::~Field(){if(init_expr) delete init_expr;}
void Field::print() const{
    cout << "Field(" << name << ", " << type;
    if(init_expr){
        cout << ", ";
        init_expr->print();
    }
    cout << ")";
}



Method::Method(string name, vector<Formal*> formals, string return_type, Expr* body): name(name), formals(formals), return_type(return_type), body(body){}
Method::~Method(){
    for(auto f: formals) delete f;
    if(body) delete body;
}
void Method::print() const{
    cout << "Method(" << name << ", [";
    for (size_t i = 0; i < formals.size(); i++){
        if( i > 0) cout << ", ";
        formals[i]->print();
    }
    cout << "], " << return_type << ", ";
    body->print();
    cout << ")";
}



Class::Class(string name, string parent, vector<Field*> fields, vector<Method*> methods) : name(name), parent(parent), fields(fields), methods(methods){}
Class::~Class(){
    for(auto f: fields) delete f;
    for(auto m: methods) delete m;
}
void Class::print() const {
    cout << "Class(" << name << ", ";
    if(parent.empty()) cout << "Object";
    else cout << parent;
    cout << ", [";
    for(size_t i = 0; i < fields.size(); i++){
        if(i > 0) cout << ", ";
        fields[i]->print();
    }
    cout << "], [";
    for(size_t i = 0; i < methods.size(); i++){
        if(i > 0) cout << ", ";
        methods[i]->print();
    }
    cout << "])";
}



Program::Program(vector<Class*> classes): classes(classes){}
Program::~Program(){
    for(auto c: classes) delete c;
}
void Program::print() const{
    cout << "[";
    for(size_t i = 0; i < classes.size(); i++){
        if(i > 0) cout << "," << endl;
        classes[i]->print();
    }
    cout << "]" << endl;
}