module;

#include <string>
#include <iostream>
#include <typeinfo>
#include <utility>
#include <any>
#include <functional>
#include <memory>

export module ast_functions_any;

import impl;

namespace ast {


template<typename Tag, typename NodeT, typename... Args>
concept has_visit = requires(Tag tag, const NodeT& node, Args&&... args) {
    visit(tag, node, std::forward<Args>(args)...);
};

template<typename Tag, typename... Args>
std::any dispatch_any_impl(TypeList<>, Tag, const AnyNode&, Args&&...)
{
    throw std::runtime_error("Unknown node type");
}

template<typename Head, typename... Tail, typename Tag, typename... Args>
std::any dispatch_any_impl(TypeList<Head, Tail...>, Tag tag, const AnyNode& node, Args&&... args)
{
    if (node.type() == typeid(Head)) {
        if constexpr (has_visit<Tag, Head, Args...>) {

            using ReturnType = decltype(
                visit(tag, std::declval<const Head&>(), std::forward<Args>(args)...)
            );

            if constexpr (std::is_void_v<ReturnType>) {
                visit(tag, node.as<Head>(), std::forward<Args>(args)...);
                return std::any{};
            }
            else {
                return std::any(visit(tag, node.as<Head>(), std::forward<Args>(args)...));
            }
        } else {
            throw std::runtime_error("Visit function signature mismatch!");
        }
    }

    return dispatch_any_impl(TypeList<Tail...>{}, tag, node, std::forward<Args>(args)...);
}

export template <typename Tag, typename... Args>
std::any visit(Tag tag, const AnyNode& node, Args&&... args)
{
    return dispatch_any_impl(AvailableAstNodes{}, tag, node, std::forward<Args>(args)...);
}


}; // namespace ast
