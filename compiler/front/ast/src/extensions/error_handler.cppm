module;

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "rang.hpp"

export module ast_error_handler_ext;

import ast_location_ext;

namespace ast
{

export template <typename Derived> class ErrorHandlerExt
{
    std::filesystem::path source_file_;
    std::string error_msg_;
    std::string warning_msg_;
    std::string note_msg_;

    void
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

    void
    printFileSection() const
    {
        auto&& self   = static_cast<const Derived*>(this);
        auto&& line   = self->getLine();
        auto&& col    = self->getCol();
        auto&& width  = self->getWidth();
        auto&& height = self->getHeight();

        if (height > 0)
        {
            for (size_t line_offset = 0; line_offset < height; ++line_offset)
            {
                auto&& curr_line = line + line_offset;
                auto&& line_idx  = curr_line - 1;

                std::cerr << std::format(
                    "  {} | {}\n", curr_line, readLine(line_idx));
            }
        }
        else if (width > 0)
        {
            std::cerr << std::format("  {} | {}\n", line, readLine(line - 1));

            std::cerr << std::format("  {} |{}^",
                std::string(std::to_string(line).length(), ' '),
                std::string(col, ' '));
        }
        else
        {
            std::cerr << std::format("  {} | {}\n", line, readLine(line - 1));

            std::cerr << std::format("  {} |{}^",
                std::string(std::to_string(line).length(), ' '),
                std::string(col, ' '));
        }
    }

public:
    ErrorHandlerExt() = default;

    ErrorHandlerExt(std::filesystem::path source_file,
        std::string_view error_msg   = {},
        std::string_view warning_msg = {},
        std::string_view note_msg    = {}) :
        source_file_{ std::move(source_file) }, error_msg_(error_msg),
        warning_msg_(warning_msg), note_msg_(note_msg)
    {}

    enum class Type : uint8_t
    {
        ERROR,
        WARNING,
        NOTE
    };

    void
    setSourceFile(std::filesystem::path source_file)
    {
        source_file_ = std::move(source_file);
    }

    void
    setErrorMsg(std::string msg)
    {
        error_msg_ = std::move(msg);
    }
    void
    setWarningMsg(std::string msg)
    {
        warning_msg_ = std::move(msg);
    }
    void
    setNoteMsg(std::string msg)
    {
        note_msg_ = std::move(msg);
    }

    void
    print(const Type type) const
    {
        printLocation();

        switch (type)
        {
        case Type::ERROR:
        {
            std::cerr << rang::fgB::red << " error: " << rang::fg::reset;
            printDescription(error_msg_);
            std::cerr << '\n';
            printFileSection();
            std::cerr << '\n';
            throw std::runtime_error(error_msg_);
        }
        case Type::WARNING:
        {
            std::cerr << rang::fgB::magenta << " warning: " << rang::fg::reset;
            printDescription(warning_msg_);
            std::cerr << '\n';
            printFileSection();
            std::cerr << '\n';
            break;
        }
        case Type::NOTE:
        {
            std::cerr << rang::fgB::green << " note: " << rang::fg::reset;
            printDescription(note_msg_);
            std::cerr << '\n';
            printFileSection();
            std::cerr << '\n';
            break;
        }
        default:
        {
            break;
        }
        }
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
