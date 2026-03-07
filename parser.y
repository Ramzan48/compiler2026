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
    struct ClassBody{
        std::vector<Field*> fields;
        std::vector<Method*> methods;
    };
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
%type <Field*> field
%type <Method*> method
%type <std::vector<Formal*>*> formal_list formals
%type <Formal*> formal
%type <Expr*> expr
%type <std::string> type
%type <std::vector<Expr*>*> expr_list
%type <std::vector<Expr*>*> args
%type <ClassBody*> class_body
%type <std::vector<Expr*>*> block_exprs


%nonassoc THEN
%nonassoc ELSE
%nonassoc DO
%nonassoc IN
%right ASSIGN
%left AND
%right NOT
%left LOWER LOWER_EQUAL EQUAL
%left PLUS MINUS
%left TIMES DIV
%right UNARYMINUS ISNULL
%right POW
%left DOT


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
    CLASS TYPE_IDENTIFIER LBRACE class_body RBRACE {
        $$ = new Class($2, "", $4->fields, $4->methods);
        delete $4;
    } | CLASS TYPE_IDENTIFIER EXTENDS TYPE_IDENTIFIER LBRACE class_body RBRACE{
        $$ = new Class($2, $4, $6->fields, $6->methods);
        delete $6;
    };

class_body:
    /* empty */ {
        $$ = new ClassBody();
    } | class_body field {
        $1->fields.push_back($2);
        $$ = $1;
    } | class_body method{
        $1->methods.push_back($2);
        $$ = $1;
    };

type:
    TYPE_IDENTIFIER {$$ = $1;}
    | INT32 {$$ = "int32";}
    | BOOL {$$ = "bool";}
    | STRING {$$ = "string";}
    | UNIT {$$ = "unit";};

field:
    OBJECT_IDENTIFIER COLON type SEMICOLON{
        $$ = new Field($1, $3, nullptr);
    } | OBJECT_IDENTIFIER COLON type ASSIGN expr SEMICOLON{
        $$ = new Field($1, $3, $5);
    };

method:
    OBJECT_IDENTIFIER LPAR formals RPAR COLON type LBRACE block_exprs RBRACE{
        Block* body = new Block(*$8);
        $$ = new Method($1, *$3, $6, body);
        delete $3;
        delete $8;
    };

formals:
    /* empty */ {$$ = new std::vector<Formal*>();}
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
    OBJECT_IDENTIFIER COLON type{
        $$ = new Formal($1, $3);
    };

args:
    /* emty */ {$$ = new std::vector<Expr*>();}
    | expr_list{
        $$ = $1;
    };

expr:
    INTEGER_LITERAL{
        $$ = new IntLiteral($1);
    } | STRING_LITERAL{
        $$ = new StringLiteral($1);
    } | TRUE{
        $$ = new BoolLiteral(true);
    } | FALSE{
        $$ = new BoolLiteral(false);
    } | OBJECT_IDENTIFIER{
        $$ = new Variable($1);
    } | OBJECT_IDENTIFIER LPAR args RPAR{
        $$ = new Call(nullptr, $1, *$3);
        delete $3;
    } | expr DOT OBJECT_IDENTIFIER LPAR args RPAR{
        $$ = new Call($1, $3, *$5);
        delete $5;
    } | LBRACE block_exprs RBRACE{
        $$ = new Block(*$2);
        delete $2;
    } | OBJECT_IDENTIFIER ASSIGN expr{
        $$ = new Assignment($1, $3);
    } | LPAR expr RPAR{
        $$ = $2;
    } | IF expr THEN expr{
        $$ = new If($2, $4, nullptr);
    } | IF expr THEN expr ELSE expr{
        $$ = new If($2, $4, $6);
    } | WHILE expr DO expr{
        $$ = new While($2, $4);
    } | LET OBJECT_IDENTIFIER COLON type IN expr{
        $$ = new Let($2, $4, nullptr, $6);
    } | LET OBJECT_IDENTIFIER COLON type ASSIGN expr IN expr{
        $$ = new Let($2, $4, $6, $8);
    } | NEW TYPE_IDENTIFIER{
        $$ = new New($2);
    } | MINUS expr %prec UNARYMINUS{
        $$ = new UnaryOp("-", $2);
    } | NOT expr{
        $$ = new UnaryOp("not", $2);
    } | ISNULL expr{
        $$ = new UnaryOp("isnull", $2);
    } | expr PLUS expr{
        $$ = new BinaryOp("+", $1, $3);
    } | expr MINUS expr{
        $$ = new BinaryOp("-", $1, $3);
    } | expr TIMES expr{
        $$ = new BinaryOp("*", $1, $3);
    } | expr DIV expr{
        $$ = new BinaryOp("/", $1, $3);
    } | expr POW expr{
        $$ = new BinaryOp("^", $1, $3);
    } | expr EQUAL expr{
        $$ = new BinaryOp("=", $1, $3);
    } | expr LOWER expr{
        $$ = new BinaryOp("<", $1, $3);
    } | expr LOWER_EQUAL expr{
        $$ = new BinaryOp("<=", $1, $3);
    } | expr AND expr{
        $$ = new BinaryOp("and", $1, $3);
    } | SELF {
        $$ = new Self();
    } | LPAR RPAR{
        $$ = new UnitExpr();
    };

expr_list:
    expr{
        $$ = new std::vector<Expr*>();
        $$->push_back($1);
    } | expr_list COMMA expr{
        $1->push_back($3);
        $$ = $1;
    };

block_exprs:
    expr{
        $$ = new std::vector<Expr*>();
        $$->push_back($1);
    } | block_exprs SEMICOLON expr{
        $1->push_back($3);
        $$ = $1;
    };


%%

void VSOP::Parser::error(const location_type& l, const std::string& m){
    const position &pos = l.begin;
    cerr << *(pos.filename) << ":"
         << pos.line << ":" 
         << pos.column << ": "
         << m
         << endl;
}
