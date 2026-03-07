#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <vector>
#include <string>

class ASTNode{
    public:
        virtual ~ASTNode(){}
        virtual void print() const = 0;
};

class Expr: public ASTNode{};

class BinaryOp: public Expr{
    public:
        std::string op;
        Expr* left;
        Expr* right;
        BinaryOp(std::string op, Expr* left, Expr* right);
        ~BinaryOp();
        void print() const override;
};

class UnaryOp: public Expr{
    public:
        std::string op;
        Expr* expr;
        UnaryOp(std::string op, Expr* expr);
        ~UnaryOp();
        void print() const override;
};

class New: public Expr{
    public:
        std::string type_name;
        New(std::string type_name);
        void print() const override;
};

class Let: public Expr{
    public:
        std::string name;
        std::string type;
        Expr* init_expr;
        Expr* body;
        Let(std::string name, std::string type, Expr* init_expr, Expr* body);
        ~Let();
        void print() const override;
};

class While: public Expr{
    public:
        Expr* cond;
        Expr* body;
        While(Expr* cond, Expr* body);
        ~While();
        void print() const override;
};

class If: public Expr{
    public: 
        Expr* cond;
        Expr* then_expr;
        Expr* else_expr;
        If(Expr* cond, Expr* then_expr, Expr* else_expr);
        ~If();
        void print() const override;
};

class Assignment: public Expr{
    public:
        std::string name;
        Expr* expr;
        Assignment(std::string name, Expr* expr);
        ~Assignment();
        void print() const override;
};

class Variable: public Expr{
    public:
        std::string name;
        Variable(std::string name);
        void print() const override;
};



class Block: public Expr{
    public:
        std::vector<Expr*> exprs;
        Block(std::vector<Expr*> exprs);
        ~Block();
        void print() const override;
};

class Call: public Expr{
    public:
        Expr* object;
        std::string method_name;
        std::vector<Expr*> args;
        Call(Expr* object, std::string method_name, std::vector<Expr*> args);
        ~Call();
        void print() const override;
};

class IntLiteral: public Expr{
    public:
        int value;
        IntLiteral(int value);
        void print() const override;
};

class StringLiteral: public Expr{
    public:
        std::string value;
        StringLiteral(std::string value);
        void print() const override;
};

class BoolLiteral: public Expr{
    public:
        bool value;
        BoolLiteral(bool value);
        void print() const override;
};

class Formal: public ASTNode{
    public:
        std::string name;
        std::string type;
        Formal(std::string name, std::string type);
        void print() const override;
};

class Method: public ASTNode{
    public:
        std::string name;
        std::vector<Formal*> formals;
        std::string return_type;
        Expr* body;
        Method(std::string name, std::vector<Formal*> formals, std::string return_type, Expr* body);
        ~Method();
        void print() const override;
};

class Field: public ASTNode{
    public:
        std::string name;
        std::string type;
        Expr* init_expr;
        Field(std::string name, std::string type, Expr* init_expr);
        ~Field();
        void print() const override;
};

class Class: public ASTNode{
    public:
        std::string name;
        std::string parent;
        std::vector<Field*> fields;
        std::vector<Method*> methods;

        Class(std::string name, std::string parent, std::vector<Field*> fields, std::vector<Method*> methods);
        ~Class();
        void print() const override;
};

class Program: public ASTNode {
    public:
        std::vector<Class*> classes;
        Program(std::vector<Class*> classes);
        ~Program();
        void print() const override;
};

#endif