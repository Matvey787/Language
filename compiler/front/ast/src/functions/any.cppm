module;

#include <string>
#include <iostream>
#include <typeinfo>
#include <utility>
#include <any>
#include <functional>
#include <memory>

export module ast_functions_any;

import ast_impl;

namespace ast {

template<typename Tag, typename NodeT, typename... Args>
concept has_visit = requires(Tag& tag, const NodeT& node, Args&&... args) {
    visit(tag, node, std::forward<Args>(args)...);
};

template <typename RetT, typename Tag, typename CheckingNodeT, typename... Args>
bool dispatch_node(RetT& result, Tag& tag, const AnyNode& node, Args&&... args) {
    if (node.type() != typeid(CheckingNodeT))
    {
        return false;
    }

    if constexpr (has_visit<Tag, CheckingNodeT, Args...>) {
        if constexpr (std::is_void_v<RetT>) {
            visit(tag, node.as<CheckingNodeT>(), std::forward<Args>(args)...);
        } else {
            result = visit(tag, node.as<CheckingNodeT>(), std::forward<Args>(args)...);
        }
        return true;
    } else {
        throw std::runtime_error(std::format("{}{}", "Visit function signature mismatch for node: ",
        typeid(CheckingNodeT).name()));
    }
}

export template <typename RetT, typename Tag, typename... Args>
RetT visit(Tag& tag, const AnyNode& node, Args&&... args)
{
    RetT result{};

    bool found = false;

    [&]<typename... NodeTypes>(TypeList<NodeTypes...>) {
        found = (dispatch_node<RetT, Tag, NodeTypes, Args...>(result, tag, node, std::forward<Args>(args)...) || ...);
    }(AvailableAstNodes{});

    if (!found)
    {
        throw std::runtime_error("Unknown node type during dispatch!");
    }

    return result;
}

}; // namespace ast
