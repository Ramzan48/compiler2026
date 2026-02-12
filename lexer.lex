%{
    #include <string>
    #include <stack>

    #include "parser.hpp"
    #include "driver.hpp"

    using namespace std;
    using namespace VSOP;

    Parser::symbol_type make_NUMBER(const string &s, const location &loc);

    static void print_error(const position &pos, const string &m);

    #define YY_USER_ACTION  loc.columns(yyleng);

    location loc;

    static map<string, Parser::token_type> keywords = {
        {"and", Parser::token::AND},
        {"bool", Parser::token::BOOL},
        {"class", Parser::token::CLASS},
        {"do", Parser::token::DO},
        {"else", Parser::token::ELSE},
        {"extends", Parser::token::EXTENDS},
        {"false", Parser::token::FALSE},
        {"if", Parser::token::IF},
        {"in", Parser::token::IN},
        {"int32", Parser::token::INT32},
        {"isnull", Parser::token::ISNULL},
        {"let", Parser::token::LET},
        {"new", Parser::token::NEW},
        {"not", Parser::token::NOT},
        {"self", Parser::token::SELF},
        {"string", Parser::token::STRING},
        {"then", Parser::token::THEN},
        {"true", Parser::token::TRUE},
        {"unit", Parser::token::UNIT},
        {"while", Parser::token::WHILE},
    };

    int comment_depth = 0;
    std::stack<location> comment_loc_stack;
    string string_buffer;
    location loc_string;
    bool first_error_flag = true;

%}

%option noyywrap nounput noinput batch

id              [a-zA-Z][a-zA-Z_0-9]*
int             [0-9]+
hex             0x[0-9a-zA-Z]*
blank           [ \t\r]

%x COMMENT
%x STRING

%%

%{
    loc.step();
%}


{blank}+    loc.step();
\n+         loc.lines(yyleng); loc.step();

"-"         return Parser::make_MINUS(loc);
"+"         return Parser::make_PLUS(loc);
"*"         return Parser::make_TIMES(loc);
"/"         return Parser::make_DIV(loc);
"^"         return Parser::make_POW(loc);
"="         return Parser::make_EQUAL(loc);
"<"         return Parser::make_LOWER(loc);
"<="        return Parser::make_LOWER_EQUAL(loc);
"<-"        return Parser::make_ASSIGN(loc);
"{"         return Parser::make_LBRACE(loc);
"}"         return Parser::make_RBRACE(loc);
"("         return Parser::make_LPAR(loc);
")"         return Parser::make_RPAR(loc);
":"         return Parser::make_COLON(loc); 
";"         return Parser::make_SEMICOLON(loc);
","         return Parser::make_COMMA(loc);
"."         return Parser::make_DOT(loc);

{int}               {  
                        int n = stoi(yytext);
                        return Parser::make_INTEGER_LITERAL(n, loc);
                    }
{hex}               {   
                        string str(yytext);
                        if (str.length() > 2){
                            bool is_hex = true;
                            for(size_t i = 2; i < str.length(); i++){
                                if(!isxdigit(str[i])){
                                    is_hex = false;
                                    break;
                                }
                            }
                            if (!is_hex){
                                print_error(loc.begin, "invalid integer literal: " + str);
                                return Parser::make_YYerror(loc);
                            }
                        }

                        if (str.length() == 2){
                            print_error(loc.begin, "invalid integer literal: " + str);
                            return Parser::make_YYerror(loc);
                        }

                        int n = stoi(yytext, nullptr, 16);
                        return Parser::make_INTEGER_LITERAL(n, loc);
                    }
{id}                {
                        auto it = keywords.find(yytext);
                        if (it != keywords.end()){
                            return Parser::symbol_type(it->second, yytext, loc);
                        }
                        if (isupper(yytext[0])){
                            return Parser::make_TYPE_IDENTIFIER(yytext, loc);
                        }
                        return Parser::make_OBJECT_IDENTIFIER(yytext, loc);
                    }

{int}{id}           {
                       print_error(loc.begin, "invalid integer literal: " + string(yytext));
                       return Parser::make_YYerror(loc);
                    }

"\""                                    {
                                            loc_string = loc;
                                            string_buffer = "\"";
                                            BEGIN(STRING);
                                        }

<STRING>[^"\\\n]+|\\x[0-9a-fA-F]{2}     {string_buffer += yytext;}

<STRING>\\[btnr\"\\]                    {
                                            char c = yytext[1];
                                            switch (c){
                                                case 'b': string_buffer += "\\x08"; break;
                                                case 't': string_buffer += "\\x09"; break;
                                                case 'n': string_buffer += "\\x0a"; break;
                                                case 'r': string_buffer += "\\x0d"; break;
                                                case '"': string_buffer += "\\x22"; break;
                                                case '\\': string_buffer += "\\x5c"; break;
                                            }
                                        }

<STRING>\\\n[ \t]*                      {
                                            loc.lines(1);
                                            loc.columns(yyleng - 2);       /* we don't count the "\\\n" */
                                        }

<STRING>\n                              {
                                            loc.columns(yyleng - 2);       /* temporary solution, it was set to -1 because of the \n but the column was always offset by one */ 
                                            if (first_error_flag){  
                                                print_error(loc.end, "raw line feed in string literal");
                                                first_error_flag = false;
                                            }
                                            BEGIN(INITIAL);
                                            return Parser::make_YYerror(loc);
                                        }

<STRING>\\.                             {
                                            if (first_error_flag){ 
                                                print_error(loc.end-yyleng, "invalid escape character in string literal: " + string(yytext));
                                                first_error_flag = false;
                                            }
                                            BEGIN(INITIAL);
                                            return Parser::make_YYerror(loc);
                                        }

<STRING>\"                              {
                                            string_buffer += "\"";
                                            first_error_flag = true;
                                            BEGIN(INITIAL);
                                            return Parser::make_STRING_LITERAL(string_buffer, loc_string);
                                        }

<STRING><<EOF>>                         {
                                            print_error(loc_string.begin, "string not terminated");
                                            BEGIN(INITIAL);
                                            return Parser::make_YYerror(loc_string);
                                        }

"//".*              {}

"(*"                {
                        comment_depth = 1;
                        comment_loc_stack.push(loc);
                        BEGIN(COMMENT);
                    }

<COMMENT>"(*"       {
                        comment_depth++;
                        comment_loc_stack.push(loc);
                    }
<COMMENT>"*)"       {
                        comment_depth--;
                        comment_loc_stack.pop();
                        if (comment_depth == 0){
                            BEGIN(INITIAL);
                        }
                    }
<COMMENT>\n         {loc.lines(1);}
<COMMENT>.          {loc.step();}
<COMMENT><<EOF>>    {
                        location err_loc = comment_loc_stack.top();
                        print_error(err_loc.begin, "Comment not terminated");
                        BEGIN(INITIAL);
                        return Parser::make_YYerror(err_loc);}


.           {
                if (first_error_flag){
                    print_error(loc.begin, "invalid character: " + string(yytext));
                    first_error_flag = false;
                }
                return Parser::make_YYerror(loc);
            }

    /* End of file */
<<EOF>>     return Parser::make_YYEOF(loc);

%%

static void print_error(const position &pos, const string &m){
    cerr << *(pos.filename) << ":"
         << pos.line << ":"
         << pos.column << ":"
         << " lexical error: "
         << m
         << endl;
}

void Driver::scan_begin(){
    loc.initialize(&source_file);

    if (source_file.empty() || source_file == "-")
        yyin = stdin;
    else if (!(yyin = fopen(source_file.c_str(), "r"))){
        cerr << "cannot open " << source_file << ": " << strerror(errno) << '\n';
        exit(EXIT_FAILURE);
    }
}

void Driver::scan_end(){
    fclose(yyin);
}