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

export module ast_mmd_functions;

import ast_any_node_impl;
import ast_any_visit_impl;
import ast_nodes_impl;

namespace ast
{

template <typename T> struct MmdNodeSettings
{
    static constexpr std::string_view fill_c        = "#FFFFFF";
    static constexpr std::string_view stroke_c      = "#000000";
    static constexpr std::string_view color_c       = "#000000";
    static constexpr std::string_view border_type_c = "()";
    static constexpr std::string_view class_name_c  = "default";
};

const size_t max_name_length_c = 64;

const uint8_t colon_ascii_c = 58;

const uint8_t above_ascii_c = 62;

class NameCleaner
{
    std::array<char, max_name_length_c> data_{};

public:
    constexpr NameCleaner(std::string_view name)
    {

        // clang-format off
        auto view = name 
        | std::views::filter(
            [](char word) 
            {
                return word != ' ';
            }
        )
        | std::views::transform(
            [](char symbol)
            {
                return (colon_ascii_c <= symbol && symbol <= above_ascii_c) ? '_' : symbol;
            }
        )
        | std::views::take(
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

template <typename T>
concept NumberOrStringLiteral =
    std::same_as<std::remove_cvref_t<T>, Lit<int>> ||
    std::same_as<std::remove_cvref_t<T>, Lit<std::string>>;

export template <typename T>
    requires NumberOrStringLiteral<T>
nodeId
visit(const anyNode& node,
    const T& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& lit = node.as<T>();
    nodeId id = getNextId(id_handler);
    std::stringstream ss;

    if constexpr (requires { lit.data(); })
    {
        ss << lit.data();
    }
    else
    {
        ss << "node";
    }

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<T>(std::format("{}\n{}", "Lit", ss.str())));
    return id;
}

export auto
visit(const anyNode& node,
    const Var& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& var = node.as<Var>();
    nodeId id = getNextId(id_handler);

    buffer += std::format("{}{}\n", id, generateNodeStyle<Var>(var.data()));
    return id;
}

export auto
visit(const anyNode& node,
    const BinOp& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& binop    = node.as<BinOp>();
    auto&& id      = getNextId(id_handler);
    auto&& larg_id = ast::visit<nodeId>(binop.getLarg(), id_handler, buffer);
    auto&& rarg_id = ast::visit<nodeId>(binop.getRarg(), id_handler, buffer);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<BinOp>(magic_enum::enum_name(binop.getOp())));
    buffer += std::format("{} --> {}\n{} --> {}\n", id, larg_id, id, rarg_id);
    return id;
}

export auto
visit(const anyNode& node,
    const Block& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& block = node.as<Block>();
    auto&& id   = getNextId(id_handler);
    buffer += std::format("{}{}\n", id, generateNodeStyle<Block>("block"));
    for (auto&& child : block)
    {
        auto&& child_id = ast::visit<nodeId>(child, id_handler, buffer);
        buffer += std::format("{} --> {}\n", id, child_id);
    }
    return id;
}

export auto
visit(const anyNode& node,
    const Assign& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& assignment = node.as<Assign>();
    auto&& id        = getNextId(id_handler);
    auto&& larg_id =
        ast::visit<nodeId>(assignment.getLarg(), id_handler, buffer);
    auto&& rarg_id =
        ast::visit<nodeId>(assignment.getRarg(), id_handler, buffer);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<Assign>(std::format(
            "{}", assignment.isInitialisation() ? "\"= (init)\"" : "\"=\"")));

    buffer += std::format("{} --> {}\n{} --> {}\n", id, larg_id, id, rarg_id);
    return id;
}

export auto
visit(const anyNode& node,
    const IfElse& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& if_else = node.as<IfElse>();
    auto&& id     = getNextId(id_handler);

    auto&& clause_id =
        ast::visit<nodeId>(if_else.getClause(), id_handler, buffer);
    auto&& if_block_id =
        ast::visit<nodeId>(anyNode(if_else.getIf()), id_handler, buffer);

    buffer += std::format("{}{}\n", id, generateNodeStyle<IfElse>("if_else"));
    buffer +=
        std::format("{} --> {}\n{} --> {}\n", id, clause_id, id, if_block_id);

    if (!if_else.getElse().empty())
    {
        auto else_block_id =
            ast::visit<nodeId>(if_else.getElse(), id_handler, buffer);
        buffer += std::format("{} --> {}\n", id, else_block_id);
    }
    return id;
}

export auto
visit(const anyNode& node,
    const While& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& while_node = node.as<While>();
    nodeId id        = getNextId(id_handler);

    auto&& clause_id =
        ast::visit<nodeId>(while_node.getClause(), id_handler, buffer);
    auto&& body_id =
        ast::visit<nodeId>(while_node.getBody(), id_handler, buffer);

    buffer += std::format("{}{}\n", id, generateNodeStyle<While>("while"));
    buffer += std::format("{} --> {}\n{} --> {}\n", id, clause_id, id, body_id);

    return id;
}

export auto
visit(const anyNode& node,
    const StructField& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& field = node.as<StructField>();
    auto&& id   = getNextId(id_handler);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<StructField>(std::format("{}", field.getName())));

    auto&& value = field.getValue();

    if (value)
    {
        auto&& val_id = ast::visit<nodeId>(value.value(), id_handler, buffer);
        buffer += std::format("{} --> {}\n", id, val_id);
    }

    return id;
}

export auto
visit(const anyNode& node,
    const Struct& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& struct_node = node.as<Struct>();
    auto&& id         = getNextId(id_handler);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<Struct>(
            std::format("Struct {}", struct_node.getName())));

    for (auto&& field : struct_node)
    {
        auto&& field_id = ast::visit<nodeId>(field, id_handler, buffer);
        buffer += std::format("{} --> {}\n", id, field_id);
    }

    return id;
}

export auto
visit(const anyNode& node,
    const StructEditor& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& editor = node.as<StructEditor>();
    auto&& id    = getNextId(id_handler);

    auto&& editable_field = editor.getEditableField();

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<StructEditor>(
            std::format("Edit {}", editor.getNameOfInstance())));

    auto&& editable_field_id =
        ast::visit<nodeId>(editable_field, id_handler, buffer);
    buffer += std::format("{} --> {}\n", id, editable_field_id);

    return id;
}

export auto
visit(const anyNode& node,
    const Func& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& func_node = node.as<Func>();
    auto&& id       = getNextId(id_handler);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<Func>(std::format("Func {}", func_node.getName())));

    auto&& args = func_node.getArgs();

    auto args_id = ast::visit<nodeId>(args, id_handler, buffer);
    buffer += std::format("{} --> {}\n", id, args_id);

    auto body_id = ast::visit<nodeId>(func_node.getBody(), id_handler, buffer);
    buffer += std::format("{} --> {}\n", id, body_id);

    return id;
}

export auto
visit(const anyNode& node,
    const FuncCall& /*unused*/,
    mmd& id_handler,
    std::string& buffer)
{
    auto& call = node.as<FuncCall>();
    auto&& id  = getNextId(id_handler);

    buffer += std::format("{}{}\n",
        id,
        generateNodeStyle<Func>(std::format("Call {}", call.getName())));

    auto&& args = call.getArgs();

    auto&& args_id = ast::visit<nodeId>(args, id_handler, buffer);
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
to_mmd(anyNode& node, const std::filesystem::path& mmd_file_path)
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
    ast::visit<nodeId>(node, id_handler, buffer);

    file << buffer;
    file.close();
}

} // namespace ast
