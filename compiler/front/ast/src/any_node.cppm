module;

#include <any>
#include <concepts>
#include <stdexcept>

export module ast_any_node_impl;

import ast_extensions;

namespace ast
{

template <typename T>
concept NotContainer = !requires(T t) {
    typename std::decay_t<T>::value_type;
    t.begin();
    t.end();
};

template <template <typename> typename... Extensions>
class RawAnyNode final : public Extensions<RawAnyNode<Extensions...>>...
{
    using Self = RawAnyNode<Extensions...>;
    std::any data_;

public:
    RawAnyNode() = default;

    template <typename NodeT>
        requires NotContainer<NodeT> &&
                 (!std::is_same_v<std::decay_t<NodeT>, Self>) &&
                 (std::is_default_constructible_v<Extensions<Self>> && ...)
    RawAnyNode(NodeT&& node) : data_{ std::forward<NodeT>(node) }
    {}

    template <typename NodeT, typename... ExtArgs>
        requires NotContainer<std::decay_t<NodeT>> &&
                     (!std::is_same_v<std::decay_t<NodeT>, Self>) &&
                     (sizeof...(ExtArgs) > 0)
    explicit RawAnyNode(NodeT&& node, ExtArgs&&... ext_args) :
        Extensions<Self>(std::forward<ExtArgs>(ext_args))...,
        data_{ std::forward<NodeT>(node) }
    {}

    [[nodiscard]] auto
    type() const -> const std::type_info&
    {
        return data_.type();
    }

    template <typename T>
    decltype(auto)
    asChangeable()
    {
        try
        {
            return std::any_cast<T&>(data_);
        }
        catch (const std::bad_any_cast&)
        {
            throw std::runtime_error(
                "AnyNode: Type mismatch during cast. Requested type: " +
                std::string(typeid(T).name()) +
                ", Actual type: " + std::string(data_.type().name()));
        }
    }

    template <typename T>
    decltype(auto)
    as() const
    {
        try
        {
            return std::any_cast<const T&>(data_);
        }
        catch (const std::bad_any_cast&)
        {
            throw std::runtime_error(
                "AnyNode: Type mismatch during cast. Requested type: " +
                std::string(typeid(T).name()) +
                ", Actual type: " + std::string(data_.type().name()));
        }
    }

    template <typename T>
    auto
    asMove() -> T&&
    {
        return std::any_cast<T&&>(std::move(data_));
    }
};

export using anyNode = RawAnyNode<LocationExt, ErrorHandlerExt>;

} // namespace ast
