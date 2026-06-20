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
#include "magic_enum.hpp" // from third party
#include <ranges>


export module ast_functions_mmd;

import impl;
import ast_functions_any;

namespace ast {

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

    constexpr NameCleaner(std::string_view name) : data{}
    {
        auto view = name | std::views::filter([](char c){ return c != ' '; })
                         | std::views::transform([](char c) {
                            return (c == '<' || c == '>') ? '_' : c;
                         })
                         | std::views::take(63);
        
        std::ranges::copy(view, data.begin());
    }

    constexpr std::string_view view() const {
        return {data.data()};
    }
};

#define STYLE_M(NodeT, fill_val, stroke_val, color_val, border_val) \
    template<> \
    struct mmdNodeSettings<NodeT> { \
        static constexpr std::string_view fill = fill_val; \
        static constexpr std::string_view stroke = stroke_val; \
        static constexpr std::string_view color = color_val; \
        static constexpr std::string_view borderType = border_val; \
        static constexpr NameCleaner cleaner{#NodeT}; \
        static constexpr std::string_view className = cleaner.view(); \
    };

#include "mmd_styles.hpp"




using nodeId = uintptr_t;

template<typename T>
std::string generateNodeStyle(std::string_view name)
{
    using Settings = mmdNodeSettings<T>;
    
    return std::format("{}{}{}{}{}",
        Settings::borderType[0],
        name,
        Settings::borderType[1],
        ":::",
        Settings::className
    );
}

export struct mmd {};

export template<typename NodeT>
    requires (!std::is_same_v<std::remove_cvref_t<NodeT>, AnyNode>)
nodeId visit(mmd, const NodeT& node, std::string& buffer)
{
    nodeId uniqueNodeName{reinterpret_cast<uintptr_t>(&node)};

    std::stringstream ss;
    ss << node.data();

    buffer += std::to_string(uniqueNodeName) + generateNodeStyle<NodeT>(ss.str()) + '\n';

    return uniqueNodeName;
}

nodeId visit(mmd, const BinOp& node, std::string& buffer)
{
    nodeId uniqueNodeName{reinterpret_cast<uintptr_t>(&node)};

    nodeId largUniqueName = std::any_cast<nodeId>(visit(ast::mmd{}, node.getLarg(), buffer));
    nodeId rargUniqueName = std::any_cast<nodeId>(visit(ast::mmd{}, node.getRarg(), buffer));

    buffer += std::to_string(uniqueNodeName) + 
              generateNodeStyle<BinOp>(magic_enum::enum_name(node.getOp())) + '\n';

    buffer += std::to_string(uniqueNodeName) + " --> " + std::to_string(largUniqueName) + "\n";
    buffer += std::to_string(uniqueNodeName) + " --> " + std::to_string(rargUniqueName) + "\n";


    return uniqueNodeName;
}

nodeId visit(mmd, const Expr& expression, std::string& buffer)
{    
    nodeId uniqueExprName{reinterpret_cast<uintptr_t>(&expression)};

    buffer += std::to_string(uniqueExprName) + generateNodeStyle<Expr>("expression") + '\n';

    for (auto&& node : expression)
    {
        buffer += std::to_string(uniqueExprName) + " --> " + std::to_string(std::any_cast<nodeId>(visit(ast::mmd{}, node, buffer))) + '\n';
    }

    return uniqueExprName;
}

nodeId visit(mmd, const Assign& assignment, std::string& buffer)
{    
    nodeId uniqueAssignName{reinterpret_cast<uintptr_t>(&assignment)};


    nodeId largUniqueName = std::any_cast<nodeId>(visit(ast::mmd{}, assignment.getLarg(), buffer));
    nodeId rargUniqueName = std::any_cast<nodeId>(visit(ast::mmd{}, assignment.getRarg(), buffer));

    buffer += std::to_string(uniqueAssignName) + generateNodeStyle<Assign>("assignment") + '\n';

    buffer += std::to_string(uniqueAssignName) + " --> " + std::to_string(largUniqueName) + "\n";
    buffer += std::to_string(uniqueAssignName) + " --> " + std::to_string(rargUniqueName) + "\n";

    return uniqueAssignName;
}

template<typename AvailableNode>
void generate_style(std::string& buffer)
{
    using mmdSettings = mmdNodeSettings<AvailableNode>;

    buffer += std::format("classDef {} fill:{}, stroke:{}, color:{};\n", 
                      mmdSettings::className, 
                      mmdSettings::fill, 
                      mmdSettings::stroke,
                      mmdSettings::color);
}

template<typename... AvailableNodes>
void generate_styles(TypeList<AvailableNodes...>, std::string& buffer)
{
    (..., generate_style<AvailableNodes>(buffer));
}

/// @brief Generate AST diagram in mermaid format
export decltype(auto) to_mmd(AnyNode& node, std::filesystem::path mmdFilePath)
{
    std::ofstream file(mmdFilePath);

    if (file.is_open()) {

        std::string buffer;
        buffer += "graph TD\n";

        buffer += "\n\%\%Available styles\n";
        generate_styles(AvailableAstNodes{}, buffer);

        buffer += "\n\%\%AST tree\n";
        ast::visit(mmd{}, node, buffer);

        file << buffer;

        file.close();
    }
}






}; // namespace ast
