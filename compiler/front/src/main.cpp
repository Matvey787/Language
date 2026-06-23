#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>
#include <cstdlib>

import ast;
import ir_generator;

#include "parser.tab.hh"

extern FILE* yyin;

int main(int argc, char* argv[]) try
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>\n";
        return 1;
    }

    FILE* input = std::fopen(argv[1], "r");
    if (!input) {
        std::cerr << "Cannot open file: " << argv[1] << '\n';
        return 1;
    }

    yyin = input;

    ast::AnyNode result;

    yy::parser parser(result);
    int rc = parser.parse();

    std::fclose(input);

    if (rc != 0) {
        std::cerr << "parse error\n";
        return 1;
    }

    ast::to_mmd(result, "ast.mmd");

    ir_generator::to_llvmir(result, "../out.ll");

    std::system("clang ../out.ll -o ../out");


}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
