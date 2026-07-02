module;

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "rang.hpp"

export module ast_error_handler_ext;

import ast_location_ext;

namespace ast
{

export template <typename Derived> class ErrorHandler
{
    std::filesystem::path source_file_;
    std::string msg_;

    void
    printLocation()
    {
        if (!used())
        {
            throw std::logic_error(std::format(
                R"(It is not possible to use the extension "ErrorHandler" for the node "{}".)",
                typeid(Derived).name()));
        }

        auto& self  = static_cast<Derived&>(*this);
        auto&& line = self.getLine();
        auto&& col  = self.getCol();

        std::cerr << rang::fgB::blue
                  << std::format("{}:{}:{}", source_file_.string(), line, col)
                  << rang::fg::reset;
    }

    void
    printDescription(std::string_view msg)
    {
        std::cerr << rang::fgB::blue << std::string(msg) << rang::fg::reset;
    }

    std::string
    readLine(size_t target_line_idx)
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

public:
    ErrorHandler() = default;

    ErrorHandler(std::filesystem::path source_file) :
        source_file_{ std::move(source_file) }
    {}

    void
    setSourceFile(std::filesystem::path source_file)
    {
        source_file_ = std::move(source_file);
    }

    void
    setMsg(std::string msg)
    {
        msg_ = std::move(msg);
    }

    void
    printErrorDescription()
    {
        printLocation();

        std::cerr << rang::fgB::red << " error: " << rang::fg::reset;

        printDescription(msg_) << '\n';
    }

    void
    printNoteDescription()
    {
        printLocation();

        std::cerr << rang::fgB::green << " note: " << rang::fg::reset;

        printDescription(msg_) << '\n';
    }

    [[nodiscard]] bool
    used() const
    {
        return !source_file_.empty() &&
               static_cast<const Derived*>(this)->Location<Derived>::used();
    }

    operator bool() const noexcept { return used(); }
};

} // namespace ast
