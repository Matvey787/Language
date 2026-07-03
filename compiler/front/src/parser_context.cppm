module;

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "location.hh"
#include "rang.hpp"

export module parser_context;

import ast;

export class ParserContext
{
    // TODO Add description

    std::filesystem::path source_file_;
    std::vector<yy::location> brace_stack_;
    std::string current_unit_;
    yy::location unit_loc_;

    decltype(auto)
    printLocation(std::ostream& os, std::size_t line, std::size_t col)
    {
        os << rang::fgB::blue
           << std::format("{}:{}:{}", source_file_.string(), line, col)
           << rang::fg::reset;

        return os;
    }

    decltype(auto)
    printDescription(std::ostream& os, std::string_view msg)
    {
        os << rang::fgB::blue << std::string(msg) << rang::fg::reset;

        return os;
    }

    decltype(auto)
    printErrorDescription(
        std::string_view msg, std::size_t line, std::size_t col)
    {
        printLocation(std::cerr, line, col);

        std::cerr << rang::fgB::red << " error: " << rang::fg::reset;


        printDescription(std::cerr, msg) << '\n';
    }

    decltype(auto)
    printNoteDescription(
        std::string_view msg, std::size_t line, std::size_t col)
    {
        printLocation(std::cerr, line, col);

        std::cerr << rang::fgB::green << " note: " << rang::fg::reset;

        printDescription(std::cerr, msg) << '\n';
    }

    decltype(auto)
    getLine(size_t target_line_idx)
    {
        std::ifstream file(source_file_);

        if (!file.is_open())
        {
            throw std::runtime_error(std::format(
                R"(The file at the specified path "{}" cannot be opened.)",
                source_file_.string()));
        }

        size_t line_idx = 0;
        std::string line;
        while (std::getline(file, line))
        {
            if (line_idx == target_line_idx)
            {
                return line;
            }
            line_idx++;
        }

        throw std::runtime_error("Line not found");
    }

    auto
    checkExpression() -> void
    {
        if (current_unit_ == "expr")
        {
            auto&& unit_line   = unit_loc_.begin.line;
            auto&& unit_column = unit_loc_.begin.column;

            auto&& err_line   = loc_.begin.line;
            auto&& err_column = loc_.begin.column;

            auto&& unit_line_idx = unit_line - 1;

            auto&& line = getLine(unit_line_idx);

            printErrorDescription(
                "expected expression", unit_line, unit_column);

            std::cerr << std::format("  {} | {}\n", unit_line, line);

            std::cerr << std::format("  {} |{}{}^\n",
                std::string(std::to_string(unit_line).length(), ' '),
                std::string(unit_column, ' '),
                std::string((err_line == unit_line)
                                ? (err_column - unit_column)
                                : (line.size() - unit_column),
                    '~'));
        }
        else
        {
            checkBraceStack();
        }
    }

    auto
    checkBraceStack() -> void
    {
        if (brace_stack_.empty())
        {
            return;
        }

        auto&& err_line_pos   = loc_.begin.line;
        auto&& err_column_pos = loc_.begin.column;

        auto&& err_line_idx = err_line_pos - 1;
        auto&& err_line     = getLine(err_line_idx);

        // FIXME expected '}' ---> expected ')' also ?????

        printErrorDescription(R"(expected '}')", err_line_pos, err_column_pos);

        std::cerr << std::format("  {} | {}\n", err_line_pos, err_line);

        std::cerr << std::format("  {} |{}^\n",
            std::string(std::to_string(err_line_pos).length(), ' '),
            std::string(err_column_pos, ' '));


        auto&& brace_location    = brace_stack_.back();
        auto&& open_brace_line   = brace_location.begin.line;
        auto&& open_brace_column = brace_location.begin.column;

        auto&& brace_line_idx = open_brace_line - 1;

        auto&& line = getLine(brace_line_idx);

        printNoteDescription(
            std::format(R"(to match this '{}')", line[open_brace_column - 1]),
            open_brace_line,
            open_brace_column);

        std::cerr << std::format("  {} | {}\n", open_brace_line, line);

        std::cerr << std::format("  {} |{}^\n",
            std::string(std::to_string(open_brace_line).length(), ' '),
            std::string(open_brace_column, ' '));
    }

public:
    ast::anyNode result_;
    yy::location loc_;

    ParserContext() = delete;

    ParserContext(std::filesystem::path source_file) :
        source_file_(std::move(source_file))
    {
        if (!std::filesystem::exists(source_file_))
        {
            throw std::runtime_error(std::format(
                R"(The file at the specified path "{}" is missing.)",
                source_file_.string()));
        }
    }

    decltype(auto)
    pushBrace(const yy::location& location)
    {
        brace_stack_.push_back(location);
    }

    decltype(auto)
    popBrace()
    {
        brace_stack_.pop_back();
    }

    decltype(auto)
    setCurrentUnit(std::string_view unit, const yy::location unit_loc)
    {
        current_unit_ = unit;
        unit_loc_     = unit_loc;
    }

    decltype(auto)
    resetCurrentUnit()
    {
        current_unit_.clear();
    }

    decltype(auto)
    check()
    {
        checkExpression();
    }

    decltype(auto)
    getSourceFile()
    {
        return source_file_;
    }
};
