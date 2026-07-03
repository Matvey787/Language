#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>

using std::fclose;

import ast;
import ir_generator;
import parser_context;

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

    std::filesystem::path file_path(argv[1]);

    ParserContext ctx(file_path);

    yy::parser parser(ctx);
    int return_code = parser.parse();

    fclose(input);

    if (return_code != 0)
    {
        std::cerr << "parse error\n";
        return 1;
    }

    const auto mmd_path =
        std::filesystem::path{ file_path }.replace_extension("mmd");
    ast::to_mmd(ctx.result_, mmd_path);

    const auto output_llvm_path =
        std::filesystem::path{ file_path }.replace_extension("ll");
    ir_generator::toLLVMIR(ctx.result_, output_llvm_path);

    const auto output_path = std::filesystem::path{ file_path }.stem();
    const auto command     = std::format(
        R"(clang {} -o {})", output_llvm_path.string(), output_path.string());

    std::system(command.c_str());
}
catch (const std::exception& e)
{
    std::cerr << e.what() << '\n';
    std::cerr << "Compiler returned: 1\n";

    return 1;
}
