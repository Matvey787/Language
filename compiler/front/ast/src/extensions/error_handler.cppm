module;

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "rang.hpp"

export module ast_error_handler_ext;

import ast_location_ext;

namespace ast
{

export template <typename Derived> class ErrorHandlerExt
{
    std::filesystem::path source_file_;
    std::string msg_;

    decltype(auto)
    printLocation() const
    {
        if (!used())
        {
            throw std::logic_error(std::format(
                R"(It is not possible to use the extension "ErrorHandlerExt" for the node "{}".)",
                typeid(Derived).name()));
        }

        auto&& self = static_cast<const Derived*>(this);
        auto&& line = self->getLine();
        auto&& col  = self->getCol();

        std::cerr << rang::fgB::blue
                  << std::format("{}:{}:{}", source_file_.string(), line, col)
                  << rang::fg::reset;
    }

    void
    printDescription(std::string_view msg) const
    {
        std::cerr << rang::fgB::blue << std::string(msg) << rang::fg::reset;
    }

    [[nodiscard]] std::string
    readLine(size_t target_line_idx) const
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

    decltype(auto)
    printFileSection() const
    {
        auto&& self = static_cast<const Derived*>(this);
        auto&& line = self->getLine();
        auto&& col  = self->getCol();

        std::cerr << std::format("  {} | {}\n", line, readLine(line - 1));

        std::cerr << std::format("  {} |{}^",
            std::string(std::to_string(line).length(), ' '),
            std::string(col, ' '));
    }

public:
    ErrorHandlerExt() = default;

    ErrorHandlerExt(std::filesystem::path&& source_file,
        std::string_view msg = "[no error message]") :
        source_file_{ std::move(source_file) }, msg_(msg)
    {}

    void
    setSourceFile(std::filesystem::path&& source_file)
    {
        source_file_ = std::move(source_file);
    }

    void
    setMsg(std::string msg)
    {
        msg_ = std::move(msg);
    }

    void
    printError() const
    {
        printLocation();

        std::cerr << rang::fgB::red << " error: " << rang::fg::reset;

        printDescription(msg_);

        std::cout << '\n';

        printFileSection();

        std::cout << '\n';

        // FIXME Maybe message??? ---> The error handler generated an error.
        // (see above)

        throw std::runtime_error("");
    }

    void
    printNote() const
    {
        printLocation();

        std::cerr << rang::fgB::green << " note: " << rang::fg::reset;

        printDescription(msg_);

        std::cout << '\n';

        printFileSection();

        std::cout << '\n';
    }

    [[nodiscard]] bool
    used() const
    {
        return !source_file_.empty() &&
               static_cast<const Derived*>(this)->LocationExt<Derived>::used();
    }

    operator bool() const noexcept { return used(); }
};

} // namespace ast
