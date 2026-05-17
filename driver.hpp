/* This flex/bison example is provided to you as a starting point for your
 * assignment. You are free to use its code in your project.
 *
 * This example implements a simple calculator. You can use the '-l' flag to
 * list all the tokens found in the source file, and the '-p' flag (or no flag)
 * to parse the file and to compute the result.
 *
 * Also, if you have any suggestions for improvements, please let us know.
 */

#ifndef _DRIVER_HPP
#define _DRIVER_HPP

#include <string>
#include <vector>
#include <map>

#include "ast.hpp"
#include "parser.hpp"
#include "type_checker.hpp"

// Give prototype of yylex() function, then declare it.
#define YY_DECL VSOP::Parser::symbol_type yylex()
YY_DECL;

namespace VSOP
{
    class Driver
    {
    public:
        /**
         * @brief Construct a new Driver.
         *
         * @param _source_file The file containing the source code.
         */
        Driver(const std::string &_source_file) : program(nullptr), source_file(_source_file), tc(""){}

        /**
         * @brief Get the source file.
         *
         * @return const std::string& The source file.
         */
        const std::string &get_source_file() { return source_file; }

        /**
         * @brief Run the lexer on the source file.
         *
         * @return int 0 if no lexical error.
         */
        int lex();

        /**
         * @brief Run the parser on the source file and compute the result.
         *
         * @return int 0 if no syntax or lexical error.
         */
        int parse();

        /**
         * @brief Print all the tokens.
         */
        void print_tokens();

        void print_ast();

        bool semantic_check();

        Class_table& get_class_table(){return tc.get_class_table();}

        Program* program;

        ~Driver(){
            if(program) delete program;
        }

    private:
        /**
         * @brief The source file.
         */
        std::string source_file;

        /**
         * @brief The parser.
         */
        VSOP::Parser *parser;

        /**
         * @brief Store the tokens.
         */
        std::vector<Parser::symbol_type> tokens;

        /**
         * @brief Start the lexer.
         */
        void scan_begin();

        /**
         * @brief Stop the lexer.
         */
        void scan_end();

        TypeChecker tc;
    };
}

#endif
