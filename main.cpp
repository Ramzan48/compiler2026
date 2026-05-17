#include <iostream>
#include <string>

#include <fstream>
#include <cstdlib>

#include "driver.hpp"
#include "ir.hpp"

using namespace std;

enum class Mode
{
    LEX,
    PARSE, 
    SEM,
    CODEGEN,
    COMPILE
};

static const map<string, Mode> flag_to_mode = {
    {"-l", Mode::LEX},
    {"-p", Mode::PARSE},
    {"-c", Mode::SEM},
    {"-i", Mode::CODEGEN}
};

int main(int argc, char const *argv[])
{
    Mode mode;
    string source_file;

    if (argc == 2)
    {
        mode = Mode::COMPILE;
        source_file = argv[1];
    }
    else if (argc == 3)
    {
        if (flag_to_mode.count(argv[1]) == 0)
        {
            cerr << "Invalid mode: " << argv[1] << endl;
            return -1;
        }
        mode = flag_to_mode.at(argv[1]);
        source_file = argv[2];
    }
    else
    {
        cerr << "Usage: " << argv[0] << " [-l|-p] <source_file>" << endl;
        return -1;
    }

    VSOP::Driver driver = VSOP::Driver(source_file);

    int res;
    switch (mode)
    {
    case Mode::LEX:
        res = driver.lex();

        driver.print_tokens();

        return res;

    case Mode::PARSE:
        res = driver.parse();

        if (res == 0)
            driver.print_ast();

        return res;
    case Mode::SEM:
        res = driver.parse();
        if(res == 0 && driver.semantic_check()){
            driver.print_ast();
            return 0;
        }
        return 1;
    case Mode::CODEGEN:
        res = driver.parse();
        if(res == 0 && driver.semantic_check()){
            CodeGenerator codegen(driver.get_class_table());
            string home = getenv("HOME") ? getenv("HOME") : ".";
            string llvm_ir = codegen.generate(driver.program, home + "/.vsop/object.ll");
            cout << llvm_ir;
            return 0;
        }
        return 1;
    case Mode::COMPILE:
        res = driver.parse();
        if(res == 0 && driver.semantic_check()){
            CodeGenerator codegen(driver.get_class_table());
            string home = getenv("HOME") ? getenv("HOME") : ".";
            string ir = codegen.generate(driver.program, home + "/.vsop/object.ll");
            string base = source_file;
            size_t dot = base.find_last_of('.');
            if(dot != string::npos) base = base.substr(0, dot);
            string ll_file = base + ".ll";
            {
                ofstream out(ll_file);
                out << ir;
            }

            string cmd = "clang -Wno-override-module " + ll_file + " -o " + base + " -lm";
            int ret = system(cmd.c_str());
            remove(ll_file.c_str());
    
            return ret == 0 ? 0 : 1;
        }
        return 1;
    }
    return 0;
}
