#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <vector>
#include <string>

class ASTNode{
    public:
        virtual ~ASTNode(){}
        virtual void print(int indent = 0) const = 0;
    protected:
        void print_indentation(int indent) const;
};

class Expr: public ASTNode{};

class IntLiteral: public Expr{
    public:
        int value;
        IntLiteral(int value);
        void print(int indent = 0) const override;
};

class Formal: public ASTNode{
    public:
        std::string name;
        std::string type;
        Formal(std::string name, std::string type);
        void print(int indent = 0) const override;
};

class Method: public ASTNode{
    public:
        std::string name;
        std::vector<Formal*> formals;
        std::string return_type;
        Expr* body;
        Method(std::string name, std::vector<Formal*> formals, std::string return_type, Expr* body);
        ~Method();
        void print(int indent = 0) const override;
};

class Field: public ASTNode{
    public:
        std::string name;
        std::string type;
        Expr* init_expr;
        Field(std::string name, std::string type, Expr* init_expr);
        ~Field();
        void print(int indent = 0) const override;
};

class Class: public ASTNode{
    public:
        std::string name;
        std::string parent;
        std::vector<Field*> fields;
        std::vector<Method*> methods;

        Class(std::string name, std::string parent, std::vector<Field*> fields, std::vector<Method*> methods);
        ~Class();
        void print(int indent = 0) const override;
};

class Program: public ASTNode {
    public:
        std::vector<Class*> classes;
        Program(std::vector<Class*> classes);
        ~Program();
        void print(int indent = 0) const override;
};

#endif
