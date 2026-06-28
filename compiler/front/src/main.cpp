#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>

using std::fclose;

import ast;
import ir_generator;

#include "parser.tab.hh"

extern FILE* yyin;

auto
main(int argc, char* argv[]) -> int
try
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <file>\n";
        return 1;
    }

    FILE* input = std::fopen(argv[1], "r");

    if (input == nullptr)
    {
        std::cerr << "Cannot open file: " << argv[1] << '\n';
        return 1;
    }

    yyin = input;

    ast::AnyNode result;

    yy::parser parser(result);
    int return_code = parser.parse();

    fclose(input);

    if (return_code != 0)
    {
        std::cerr << "parse error\n";
        return 1;
    }

    ast::to_mmd(result, "ast.mmd");

    ir_generator::toLLVMIR(result, "../out.ll");

    std::system("clang ../out.ll -o ../out");
}
catch (const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
