%skeleton "lalr1.cc"
%language "c++"
%require "3.7.5"
%locations
%defines
%define api.namespace {VSOP}
%define api.parser.class {Parser}
%define api.token.raw
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    #include <string>
    namespace VSOP
    {
        class Driver;
    }
    #include "ast.hpp"
}

%parse-param {VSOP::Driver &driver}

%code {
    
    #include "driver.hpp"
    using namespace std;
}

%token
    MINUS       "-"
    PLUS        "+"
    TIMES       "*"
    DIV         "/"
    POW         "^"
    EQUAL       "="
    LOWER       "<"
    LOWER_EQUAL "<="
    ASSIGN      "<-"
    LBRACE      "{"
    RBRACE      "}"
    LPAR        "("
    RPAR        ")"
    COLON       ":"
    SEMICOLON   ";"
    COMMA       ","
    DOT         "."

    
    AND     "and"
    BOOL    "bool"
    CLASS   "class"
    DO      "do"
    ELSE    "else"
    EXTENDS "extends"
    FALSE   "false"
    IF      "if"
    IN      "in"
    INT32   "int32"
    ISNULL  "isnull"
    LET     "let"
    NEW     "new"
    NOT     "not"
    SELF    "self"
    STRING  "string"
    THEN    "then"
    TRUE    "true"
    UNIT    "unit"
    WHILE   "while"
;

%token <std::string> IDENTIFIER "identifier"
%token <int> INTEGER_LITERAL "integer_literal"

%token <std::string> OBJECT_IDENTIFIER "object_identifier"
%token <std::string> TYPE_IDENTIFIER "type_identifier"
%token <std::string> STRING_LITERAL "string_literal"

%type <Program*> program
%type <std::vector<Class*>*> class_list
%type <Class*> class
%type <std::vector<Field*>*> field_list
%type <Field*> field
%type <std::vector<Method*>*> method_list
%type <Method*> method
%type <std::vector<Formal*>*> formal_list formals
%type <Formal*> formal
%type <Expr*> expr
/* %type <std::vector<Expr*>*> expr_list */

%%

program:
    class_list{
        $$ = new Program(*$1);
        driver.program = $$;
        delete $1;
    };

class_list:
    class{
        $$ = new std::vector<Class*>();
        $$->push_back($1);
    } | class_list class{
        $1->push_back($2);
        $$ = $1;
    };

class:
    CLASS TYPE_IDENTIFIER LBRACE field_list method_list RBRACE{
        $$ = new Class($2, "", *$4, *$5);
        delete $4;
        delete $5;
    }; /* DODO: extends */

field_list:
    {$$ = new std::vector<Field*>();}
    | field_list field{
        $1->push_back($2);
        $$ = $1;
    };
field:
    OBJECT_IDENTIFIER COLON TYPE_IDENTIFIER SEMICOLON{
        $$ = new Field($1, $3, nullptr);
    }; /* DODO: assign */

method_list:
    {$$ = new std::vector<Method*>();}
    | method_list method{
        $1->push_back($2);
        $$ = $1;
    };

method:
    OBJECT_IDENTIFIER LPAR formals RPAR COLON TYPE_IDENTIFIER expr{
        $$ = new Method($1, *$3, $6, $7);
        delete $3;
    };

formals:
    {$$ = new std::vector<Formal*>();}
    | formal_list{
        $$ = $1;
    };

formal_list:
    formal{
        $$ = new std::vector<Formal*>();
        $$->push_back($1);
    } | formal_list COMMA formal{
        $1->push_back($3);
        $$ = $1;
    };

formal:
    OBJECT_IDENTIFIER COLON TYPE_IDENTIFIER{
        $$ = new Formal($1, $3);
    };

expr:
    /* Literals */
    INTEGER_LITERAL
    {
        $$ = new IntLiteral($1);
    };

/*
expr_list:
    expr
    {
        $$ = new std::vector<Expr*>();
        $$->push_back($1);
    }
    | expr_list COMMA expr
    {
        $1->push_back($3);
        $$ = $1;
    }
    ;
*/
%%

void VSOP::Parser::error(const location_type& l, const std::string& m){
    const position &pos = l.begin;
    cerr << *(pos.filename) << ":"
         << pos.line << ":" 
         << pos.column << ": "
         << m
         << endl;
}
