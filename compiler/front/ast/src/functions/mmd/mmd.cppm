module;

#include <string>
#include <iostream>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <any>
#include <sstream>
#include <filesystem>
#include <fstream>
#include "magic_enum.hpp"
#include <ranges>
#include <format>
#include <array>

export module ast_functions_mmd;

import impl;
import ast_functions_any;

namespace ast {

// --- Настройки отображения ---
template<typename T>
struct mmdNodeSettings {
    static constexpr std::string_view fill = "#FFFFFF"; 
    static constexpr std::string_view stroke = "#000000"; 
    static constexpr std::string_view color = "#000000";
    static constexpr std::string_view borderType = "()";
    static constexpr std::string_view className = "default";
};

class NameCleaner {
    std::array<char, 64> data{};
public:
    constexpr NameCleaner(std::string_view name) : data{} {
        auto view = name | std::views::filter([](char c){ return c != ' '; })
                         | std::views::transform([](char c) { return (58 <= c && c <= 62) ? '_' : c; })
                         | std::views::take(63);
        std::ranges::copy(view, data.begin());
    }
    constexpr std::string_view view() const { return {data.data()}; }
};

#define STYLE_M(NodeT, fill_val, stroke_val, color_val, border_val) \
    template<> struct mmdNodeSettings<NodeT> { \
        static constexpr std::string_view fill = fill_val; \
        static constexpr std::string_view stroke = stroke_val; \
        static constexpr std::string_view color = color_val; \
        static constexpr std::string_view borderType = border_val; \
        static constexpr NameCleaner cleaner{#NodeT}; \
        static constexpr std::string_view className = cleaner.view(); \
    };

#include "mmd_styles.hpp"

using nodeId = int;

export struct mmd {
    int nodeCounter = 0;
};

inline nodeId getNextId(mmd& ctx) {
    return ++ctx.nodeCounter;
}

template<typename T>
std::string generateNodeStyle(std::string_view name) {
    using Settings = mmdNodeSettings<T>;
    return std::format("{}{}{}{}{}", Settings::borderType[0], name, Settings::borderType[1], ":::", Settings::className);
}


export template<typename T>
concept IsNotAnyNode = !std::is_same_v<std::remove_cvref_t<T>, AnyNode>;

export template<IsNotAnyNode NodeT>
nodeId visit(mmd& ctx, const NodeT& node, std::string& buffer)
{
    nodeId id = getNextId(ctx);
    std::stringstream ss;
    if constexpr (requires { node.data(); }) ss << node.data();
    else ss << "node";

    buffer += std::format("{}{}\n", id, generateNodeStyle<NodeT>(ss.str()));
    return id;
}


export nodeId visit(mmd& ctx, const BinOp& node, std::string& buffer)
{
    nodeId id = getNextId(ctx);
    nodeId largId = ast::visit<nodeId>(ctx, node.getLarg(), buffer);
    nodeId rargId = ast::visit<nodeId>(ctx, node.getRarg(), buffer);

    buffer += std::format("{}{}\n", id, generateNodeStyle<BinOp>(magic_enum::enum_name(node.getOp())));
    buffer += std::format("{} --> {}\n{} --> {}\n", id, largId, id, rargId);
    return id;
}


export nodeId visit(mmd& ctx, const Block& block, std::string& buffer)
{    
    nodeId id = getNextId(ctx);
    buffer += std::format("{}{}\n", id, generateNodeStyle<Block>("block"));
    for (auto&& node : block) {
        nodeId childId = ast::visit<nodeId>(ctx, node, buffer);
        buffer += std::format("{} --> {}\n", id, childId);
    }
    return id;
}


export nodeId visit(mmd& ctx, const Assign& assignment, std::string& buffer)
{    
    nodeId id = getNextId(ctx);
    nodeId largId = ast::visit<nodeId>(ctx, assignment.getLarg(), buffer);
    nodeId rargId = ast::visit<nodeId>(ctx, assignment.getRarg(), buffer);

    buffer += std::format("{}{}\n", id, generateNodeStyle<Assign>("assignment"));
    buffer += std::format("{} --> {}\n{} --> {}\n", id, largId, id, rargId);
    return id;
}


export nodeId visit(mmd& ctx, const IfElse& if_else, std::string& buffer)
{    
    nodeId id = getNextId(ctx);
    nodeId clauseId = ast::visit<nodeId>(ctx, if_else.getClause(), buffer);
    nodeId ifBlockId = ast::visit<nodeId>(ctx, if_else.getIf(), buffer);

    buffer += std::format("{}{}\n", id, generateNodeStyle<IfElse>("if_else"));
    buffer += std::format("{} --> {}\n{} --> {}\n", id, clauseId, id, ifBlockId);

    if (!if_else.getElse().empty()) {
        nodeId elseBlockId = ast::visit<nodeId>(ctx, if_else.getElse(), buffer);
        buffer += std::format("{} --> {}\n", id, elseBlockId);
    }
    return id;
}


template<typename AvailableNode>
void generate_style(std::string& buffer) {
    using mmdSettings = mmdNodeSettings<AvailableNode>;
    buffer += std::format("classDef {} fill:{}, stroke:{}, color:{};\n", 
                      mmdSettings::className, mmdSettings::fill, mmdSettings::stroke, mmdSettings::color);
}

template<typename... AvailableNodes>
void generate_styles(TypeList<AvailableNodes...>, std::string& buffer) {
    (..., generate_style<AvailableNodes>(buffer));
}

export void to_mmd(AnyNode& node, std::filesystem::path mmdFilePath)
{
    std::ofstream file(mmdFilePath);
    if (!file.is_open()) return;

    std::string buffer = "graph TD\n\n%% Available styles\n";
    generate_styles(AvailableAstNodes{}, buffer);
    buffer += "\n%% AST tree\n";

    mmd ctx{};
    ast::visit<nodeId>(ctx, node, buffer);

    file << buffer;
    file.close();
}

} // namespace ast
