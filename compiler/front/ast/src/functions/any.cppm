module;

#include <format>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

export module ast_functions_any;

import ast_impl;

namespace ast
{

template <typename Tag, typename NodeT, typename... Args>
concept HasVisit = requires(Tag& tag, const NodeT& node, Args&&... args) {
    visit(tag, node, std::forward<Args>(args)...);
};

template <typename RetT, typename Tag, typename CheckingNodeT, typename... Args>
std::optional<std::conditional_t<std::is_void_v<RetT>, std::monostate, RetT>>
dispatchNode(Tag& tag, const AnyNode& node, Args&&... args)
{
    if (node.type() != typeid(CheckingNodeT))
    {
        return std::nullopt;
    }

    if constexpr (HasVisit<Tag, CheckingNodeT, Args...>)
    {
        if constexpr (std::is_void_v<RetT>)
        {
            visit(tag, node.as<CheckingNodeT>(), std::forward<Args>(args)...);

            return std::monostate{};
        }
        else
        {
            return visit(
                tag, node.as<CheckingNodeT>(), std::forward<Args>(args)...);
        }
    }
    else
    {
        throw std::logic_error(std::format("{}{}",
            "Visit function signature mismatch for node: ",
            typeid(CheckingNodeT).name()));
    }
}

export template <typename RetT, typename Tag, typename... Args>
RetT
visit(Tag& tag, const AnyNode& node, Args&&... args)
{
    bool found = false;

    if constexpr (std::is_void_v<RetT>)
    {
        [&]<typename... NodeTypes>(TypeList<NodeTypes...>)
        {
            found = (dispatchNode<RetT, Tag, NodeTypes, Args...>(
                         tag, node, std::forward<Args>(args)...)
                         .has_value() ||
                     ...);
        }(availableAstNodes{});

        if (!found)
        {
            throw std::runtime_error("Unknown node type during dispatch!");
        }
    }
    else
    {
        std::optional<RetT> result;

        [&]<typename... NodeTypes>(TypeList<NodeTypes...>)
        {
            (
                [&]
                {
                    auto return_value =
                        dispatchNode<RetT, Tag, NodeTypes, Args...>(
                            tag, node, std::forward<Args>(args)...);
                    if (return_value.has_value())
                    {
                        result = std::move(*return_value);
                        return true;
                    }
                    return false;
                }() ||
                ...);
        }(availableAstNodes{});

        if (!result.has_value())
        {
            throw std::runtime_error(
                std::format("Unknown node type during dispatch! typeid: {}",
                    node.type().name()));
        }

        return std::move(*result);
    }
}

}; // namespace ast
