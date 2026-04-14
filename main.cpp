#include <iostream>
#include <string>

#include "driver.hpp"

using namespace std;

enum class Mode
{
    LEX,
    PARSE, 
    SEM
};

static const map<string, Mode> flag_to_mode = {
    {"-l", Mode::LEX},
    {"-p", Mode::PARSE},
    {"-c", Mode::SEM},
};

int main(int argc, char const *argv[])
{
    Mode mode;
    string source_file;

    if (argc == 2)
    {
        mode = Mode::PARSE;
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
    }

    return 0;
}
