#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <iterator>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "llvm/Support/CommandLine.h"
#include "parser.tab.hh"

using std::fclose;

import ast;
import ir_generator;
import parser_context;
import project_info;


extern FILE* yyin;

llvm::cl::OptionCategory ParaCLCategory(
    "ParaCL Options", "Options for controlling the ParaCL compiler");


llvm::cl::list<std::string> InputFiles(llvm::cl::Positional,
    llvm::cl::desc("<input .cl files>"),
    llvm::cl::ZeroOrMore,
    llvm::cl::value_desc("filename"),
    llvm::cl::cat(ParaCLCategory));


llvm::cl::list<std::string> OutputPaths("o",
    llvm::cl::desc("Specify output filenames"),
    llvm::cl::value_desc("paths"),
    llvm::cl::ZeroOrMore,
    llvm::cl::cat(ParaCLCategory));

llvm::cl::alias OutputAlias("output",
    llvm::cl::desc("Alias for -o"),
    llvm::cl::aliasopt(OutputPaths),
    llvm::cl::cat(ParaCLCategory));

llvm::cl::opt<bool> AstDumpFile("mmd",
    llvm::cl::desc("Dump AST to .mmd file for Mermaid presentation "
                   "(https://mermaidviewer.com/)"),
    llvm::cl::init(false),
    llvm::cl::cat(ParaCLCategory));

llvm::cl::alias AstDumpAlias("ast-dump",
    llvm::cl::desc("Alias for -mmd"),
    llvm::cl::aliasopt(AstDumpFile),
    llvm::cl::cat(ParaCLCategory));



std::string
getBriefDescription()
{
    using namespace project_info;

    std::stringstream ss;

    ss << project_name_c
       << " - the best language in the world.\nVERSION: " << project_version_c
       << "\n\n" << project_overview_c << "\nAUTHORS:\n";

    for (const auto& author : authors_c)
    {
        ss << "  - " << author.name_ << " (" << author.git_link_ << ")\n";
    }

    return ss.str();
}

decltype(auto)
handleFile(const std::filesystem::path& in,
    const std::filesystem::path& out,
    bool mmd = false)
{

    FILE* input = std::fopen(in.c_str(), "r");

    if (input == nullptr)
    {
        std::cerr << "Cannot open file: " << in << '\n';
        return 1;
    }

    yyin = input;

    std::filesystem::path file_path(in);

    ParserContext ctx(in);

    yy::parser parser(ctx);
    int return_code = parser.parse();

    fclose(input);

    if (return_code != 0)
    {
        std::cerr << "parse error\n";
        return 1;
    }

    if (mmd)
    {
        const auto mmd_path =
            std::filesystem::path{ in }.replace_extension("mmd");
        ast::to_mmd(ctx.result_, mmd_path);
    }


    const auto output_llvm_path =
        std::filesystem::path{ in }.replace_extension("ll");
    ir_generator::toLLVMIR(ctx.result_, output_llvm_path);

    const auto command = std::format(
        R"(clang {} -o {})", output_llvm_path.string(), out.string());

    std::system(command.c_str());

    return 0;
}

auto
main(int argc, char* argv[]) -> int
try
{
    llvm::cl::HideUnrelatedOptions(ParaCLCategory);

    auto overview = getBriefDescription();
    llvm::cl::ParseCommandLineOptions(argc, argv, overview);



    bool mmd = false;
    if (AstDumpFile)
    {
        mmd = true;
    }

    if (InputFiles.empty())
    {
        std::cerr
            << "No input files provided. Use --help for usage information.\n";
        return 1;
    }

    auto&& input_files = InputFiles;

    std::vector<std::filesystem::path> output_files;

    if (OutputPaths.empty())
    {
        output_files =
            InputFiles |
            std::views::transform(
                [](std::string const& path)
                {
                    return std::filesystem::path(path).replace_extension("exe");
                }) |
            std::ranges::to<std::vector>();
    }
    else
    {
        if (OutputPaths.size() != InputFiles.size())
        {
            throw std::runtime_error(
                "It is impossible to determine which output file corresponds "
                "to the input file; changes are possible in the future.");
        }

        std::ranges::copy(OutputPaths, std::back_inserter(output_files));
    }

    for (auto&& [in, out] : std::ranges::views::zip(input_files, output_files))
    {
        if (handleFile(in, out, mmd) == 1)
        {
            throw std::runtime_error(std::format(
                R"(Errors occurred while processing the file "{}".)", in));
        }
    }
}
catch (const std::exception& e)
{
    std::cerr << e.what() << '\n';
    std::cerr << "Compiler returned: 1\n";

    return 1;
}
