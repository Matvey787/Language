module;

#include <array>
#include <concepts>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>

#include "magic_enum.hpp"

export module ast_functions_mmd;

import ast_impl;
import ast_functions_any;

namespace ast
{

// --- Настройки отображения ---
template <typename T> struct MmdNodeSettings
{
    static constexpr std::string_view fill_c        = "#FFFFFF";
    static constexpr std::string_view stroke_c      = "#000000";
    static constexpr std::string_view color_c       = "#000000";
    static constexpr std::string_view border_type_c = "()";
    static constexpr std::string_view class_name_c  = "default";
};

// for compile time string
const size_t max_name_length_c = 64;

// ascii symbol of :
const uint8_t colon_ascii_c = 58;

// ascii symbol of >
const uint8_t above_ascii_c = 62;

class NameCleaner
{
    std::array<char, max_name_length_c> data_{};

public:
    constexpr NameCleaner(std::string_view name)
    {

        // clang-format off
        auto view = name 
        | std::views::filter( // remove spaces
            [](char word) 
            {
                return word != ' ';
            }
        )
        | std::views::transform( // change :, ;, <, =, > to _
            [](char symbol)
            {
                return (colon_ascii_c <= symbol && symbol <= above_ascii_c) ? '_' : symbol;
            }
        )
        | std::views::take( // take word without \0
            max_name_length_c - 1
        );
        // clang-format on

        std::ranges::copy(view, data_.begin());
    }
    [[nodiscard]] constexpr std::string_view
    view() const
    {
        return { data_.data() };
    }
};

#define STYLE_M(NodeT, fill_val, stroke_val, color_val, border_val)            \
    template <> struct MmdNodeSettings<NodeT>                                  \
    {                                                                          \
        static constexpr std::string_view fill_c        = fill_val;            \
        static constexpr std::string_view stroke_c      = stroke_val;          \
        static constexpr std::string_view color_c       = color_val;           \
        static constexpr std::string_view border_type_c = border_val;          \
        static constexpr NameCleaner cleaner{ #NodeT };                        \
        static constexpr std::string_view class_name_c = cleaner.view();       \
    };

#include "mmd_styles.hpp"

using nodeId = int;

export struct mmd
{
    int node_counter_ = 0;
};

inline nodeId
getNextId(mmd& id_handler)
{
    return ++id_handler.node_counter_;
}

template <typename T>
std::string
generateNodeStyle(std::string_view name)
{
    using settings = MmdNodeSettings<T>;
    return std::format("{}{}{}{}{}",
        settings::border_type_c[0],
        name,
        settings::border_type_c[1],
        ":::",
        settings::class_name_c);
}

export template <typename T>
concept Number_or_string_literal =
    std::same_as<std::remove_cvref_t<T>, Lit<int>> ||
    std::same_as<std::remove_cvref_t<T>, Lit<std::string>>;

export template <Number_or_string_literal NodeT>
nodeId
visit(mmd& id_handler, const NodeT& node, std::string& buffer)
{
    nodeId id = getNextId(id_handler);
    std::stringstream ss;

    if constexpr (requires { node.data(); })
    {
        ss << node.data();
    }
    else
    {
        ss << "node";
    }

    buffer += std::format("{}{}\n", id, generateNodeStyle<NodeT>(ss.str()));
    return id;
}

export auto
visit(mmd& id_handler, const BinOp& node, std::string& buffer)
{
    auto&& id      = getNextId(id_handler);
    auto&& larg_id = ast::visit<nodeId>(id_handler, node.getLarg(), buffer);
    auto&& rarg_id = ast::visit<nodeId>(id_handler, node.getRarg(), buffer);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<BinOp>(magic_enum::enum_name(node.getOp())));
    buffer += std::format("{} --> {}\n{} --> {}\n", id, larg_id, id, rarg_id);
    return id;
}

export auto
visit(mmd& id_handler, const Block& block, std::string& buffer)
{
    auto&& id = getNextId(id_handler);
    buffer += std::format("{}{}\n", id, generateNodeStyle<Block>("block"));
    for (auto&& node : block)
    {
        auto&& child_id = ast::visit<nodeId>(id_handler, node, buffer);
        buffer += std::format("{} --> {}\n", id, child_id);
    }
    return id;
}

export auto
visit(mmd& id_handler, const Assign& assignment, std::string& buffer)
{
    auto&& id = getNextId(id_handler);
    auto&& larg_id =
        ast::visit<nodeId>(id_handler, assignment.getLarg(), buffer);
    auto&& rarg_id =
        ast::visit<nodeId>(id_handler, assignment.getRarg(), buffer);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<Assign>(std::format(
            "{}", assignment.isInitialisation() ? "\"= (init)\"" : "\"=\"")));

    buffer += std::format("{} --> {}\n{} --> {}\n", id, larg_id, id, rarg_id);
    return id;
}

export auto
visit(mmd& id_handler, const IfElse& if_else, std::string& buffer)
{
    auto&& id = getNextId(id_handler);

    auto&& clause_id =
        ast::visit<nodeId>(id_handler, if_else.getClause(), buffer);
    auto&& if_block_id =
        ast::visit<nodeId>(id_handler, if_else.getIf(), buffer);

    buffer += std::format("{}{}\n", id, generateNodeStyle<IfElse>("if_else"));
    buffer +=
        std::format("{} --> {}\n{} --> {}\n", id, clause_id, id, if_block_id);

    if (!if_else.getElse().empty())
    {
        auto else_block_id =
            ast::visit<nodeId>(id_handler, if_else.getElse(), buffer);
        buffer += std::format("{} --> {}\n", id, else_block_id);
    }
    return id;
}

export auto
visit(mmd& id_handler, const While& while_node, std::string& buffer)
{
    nodeId id = getNextId(id_handler);

    auto&& clause_id =
        ast::visit<nodeId>(id_handler, while_node.getClause(), buffer);
    auto&& body_id =
        ast::visit<nodeId>(id_handler, while_node.getBody(), buffer);

    buffer += std::format("{}{}\n", id, generateNodeStyle<While>("while"));
    buffer += std::format("{} --> {}\n{} --> {}\n", id, clause_id, id, body_id);

    return id;
}

export auto
visit(mmd& id_handler, const StructField& field, std::string& buffer)
{
    auto&& id = getNextId(id_handler);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<StructField>(std::format("{}", field.getName())));

    auto&& value = field.getValue();

    if (value)
    {
        auto&& val_id = ast::visit<nodeId>(id_handler, value.value(), buffer);
        buffer += std::format("{} --> {}\n", id, val_id);
    }

    return id;
}

export auto
visit(mmd& id_handler, const Struct& struct_node, std::string& buffer)
{
    auto&& id = getNextId(id_handler);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<Struct>(
            std::format("Struct {}", struct_node.getName())));

    for (auto&& field : struct_node)
    {
        auto&& field_id = ast::visit<nodeId>(id_handler, field, buffer);
        buffer += std::format("{} --> {}\n", id, field_id);
    }

    return id;
}

export auto
visit(mmd& id_handler, const StructEditor& node, std::string& buffer)
{
    auto&& id = getNextId(id_handler);

    auto&& editable_field = node.getEditableField();

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<StructEditor>(
            std::format("Edit {}", node.getNameOfInstance())));

    auto&& editable_field_id =
        ast::visit<nodeId>(id_handler, editable_field, buffer);
    buffer += std::format("{} --> {}\n", id, editable_field_id);

    return id;
}

export auto
visit(mmd& id_handler, const Func& func_node, std::string& buffer)
{
    auto&& id = getNextId(id_handler);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<Func>(std::format("Func {}", func_node.getName())));

    auto&& args = func_node.getArgs();

    auto args_id = ast::visit<nodeId>(id_handler, args, buffer);
    buffer += std::format("{} --> {}\n", id, args_id);

    auto body_id = ast::visit<nodeId>(id_handler, func_node.getBody(), buffer);
    buffer += std::format("{} --> {}\n", id, body_id);

    return id;
}

export auto
visit(mmd& id_handler, const FuncCall& call, std::string& buffer)
{
    auto&& id = getNextId(id_handler);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<Func>(std::format("Call {}", call.getName())));

    auto&& args = call.getArgs();

    auto&& args_id = ast::visit<nodeId>(id_handler, args, buffer);
    buffer += std::format("{} --> {}\n", id, args_id);

    return id;
}

template <typename AvailableNode>
void
generateStyle(std::string& buffer)
{
    using mmdSettings = MmdNodeSettings<AvailableNode>;
    buffer += std::format("classDef {} fill:{}, stroke:{}, color:{};\n",
        mmdSettings::class_name_c,
        mmdSettings::fill_c,
        mmdSettings::stroke_c,
        mmdSettings::color_c);
}

template <typename... AvailableNodes>
void
generateStyles(TypeList<AvailableNodes...> /*unused*/, std::string& buffer)
{
    (..., generateStyle<AvailableNodes>(buffer));
}

export void
to_mmd(AnyNode& node, const std::filesystem::path& mmd_file_path)
{
    std::ofstream file(mmd_file_path);
    if (!file.is_open())
    {
        return;
    }

    std::string buffer = "graph TD\n\n%% Available styles\n";
    generateStyles(availableAstNodes{}, buffer);
    buffer += "\n%% AST tree\n";

    mmd id_handler{};
    ast::visit<nodeId>(id_handler, node, buffer);

    file << buffer;
    file.close();
}

} // namespace ast
