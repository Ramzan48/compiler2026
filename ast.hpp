#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <vector>
#include <string>

class ASTNode{
    public:
        int line;
        int col;

        ASTNode(int line = 1, int col = 1): line(line), col(col){};
        virtual ~ASTNode(){}
        virtual void print() const = 0;
};

class Expr: public ASTNode{
    public:
        std::string type;
        Expr(int line = 1, int col = 1): ASTNode(line, col), type(""){}
        virtual  ~Expr(){}
};

class Self: public Expr{
    public:
        Self(int line = 1, int col = 1);
        void print() const override;
};

class UnitExpr: public Expr{
    public:
        UnitExpr(int line = 1, int col = 1);
        void print() const override;
};

class BinaryOp: public Expr{
    public:
        std::string op;
        Expr* left;
        Expr* right;
        BinaryOp(std::string op, Expr* left, Expr* right, int line = 1, int col = 1);
        ~BinaryOp();
        void print() const override;
};

class UnaryOp: public Expr{
    public:
        std::string op;
        Expr* expr;
        UnaryOp(std::string op, Expr* expr, int line = 1, int col = 1);
        ~UnaryOp();
        void print() const override;
};

class New: public Expr{
    public:
        std::string type_name;
        New(std::string type_name, int line = 1, int col = 1);
        void print() const override;
};

class Let: public Expr{
    public:
        std::string name;
        std::string var_type;
        Expr* init_expr;
        Expr* body;
        Let(std::string name, std::string var_type, Expr* init_expr, Expr* body, int line = 1, int col = 1);
        ~Let();
        void print() const override;
};

class While: public Expr{
    public:
        Expr* cond;
        Expr* body;
        While(Expr* cond, Expr* body, int line = 1, int col = 1);
        ~While();
        void print() const override;
};

class If: public Expr{
    public: 
        Expr* cond;
        Expr* then_expr;
        Expr* else_expr;
        If(Expr* cond, Expr* then_expr, Expr* else_expr, int line = 1, int col = 1);
        ~If();
        void print() const override;
};

class Assignment: public Expr{
    public:
        std::string name;
        Expr* expr;
        Assignment(std::string name, Expr* expr, int line = 1, int col = 1);
        ~Assignment();
        void print() const override;
};

class Variable: public Expr{
    public:
        std::string name;
        Variable(std::string name, int line = 1, int col = 1);
        void print() const override;
};



class Block: public Expr{
    public:
        std::vector<Expr*> exprs;
        Block(std::vector<Expr*> exprs, int line = 1, int col = 1);
        ~Block();
        void print() const override;
};

class Call: public Expr{
    public:
        Expr* object;
        std::string method_name;
        std::vector<Expr*> args;
        Call(Expr* object, std::string method_name, std::vector<Expr*> args, int line = 1, int col = 1);
        ~Call();
        void print() const override;
};

class IntLiteral: public Expr{
    public:
        int value;
        IntLiteral(int value, int line = 1, int col = 1);
        void print() const override;
};

class StringLiteral: public Expr{
    public:
        std::string value;
        StringLiteral(std::string value, int line = 1, int col = 1);
        void print() const override;
};

class BoolLiteral: public Expr{
    public:
        bool value;
        BoolLiteral(bool value, int line = 1, int col = 1);
        void print() const override;
};

class Formal: public ASTNode{
    public:
        std::string name;
        std::string type;
        Formal(std::string name, std::string type, int line = 1, int col = 1);
        void print() const override;
};

class Method: public ASTNode{
    public:
        std::string name;
        std::vector<Formal*> formals;
        std::string return_type;
        Expr* body;
        Method(std::string name, std::vector<Formal*> formals, std::string return_type, Expr* body, int line = 1, int col = 1);
        ~Method();
        void print() const override;
};

class Field: public ASTNode{
    public:
        std::string name;
        std::string type;
        Expr* init_expr;
        Field(std::string name, std::string type, Expr* init_expr, int line = 1, int col = 1);
        ~Field();
        void print() const override;
};

class Class: public ASTNode{
    public:
        std::string name;
        std::string parent;
        std::vector<Field*> fields;
        std::vector<Method*> methods;

        Class(std::string name, std::string parent, std::vector<Field*> fields, std::vector<Method*> methods, int line = 1, int col = 1);
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