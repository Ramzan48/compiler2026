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

%%
program: /* empty */ { (void) driver; } ; /* Just to silence the unused private field warning */
%%

void VSOP::Parser::error(const location_type& l, const std::string& m){
    const position &pos = l.begin;
    cerr << *(pos.filename) << ":"
         << pos.line << ":" 
         << pos.column << ": "
         << m
         << endl;
}
