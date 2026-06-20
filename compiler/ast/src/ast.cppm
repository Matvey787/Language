module;

#include <string>
#include <iostream>
#include <typeinfo>
#include <utility>
#include <any>
#include <functional>
#include <memory>

export module impl;

namespace ast {


export class AnyNode
{
    std::any data_;
public:
    AnyNode() = delete;

    template<typename T>
    AnyNode(T&& node) : data_{std::move(node)} {}

    const std::type_info& type() const {return data_.type(); }

    template<typename T>
    const T& as() const { return std::any_cast<const T&>(data_); }
};


export template<typename LitT>
class Lit final
{
    LitT data_;

public:
    Lit(LitT&& data) : data_{std::move(data)} {}

    LitT data() const { return data_; }
};

export class BinOp final
{
public:
    enum class binOpType : uint8_t {
        Add,
        Sub,
        Mul,
        Div,
        UnknownOperation
    };

private:
    AnyNode larg_;
    AnyNode rarg_;
    binOpType binOp_;

public:
    BinOp(AnyNode&& larg, AnyNode&& rarg, binOpType binOp = binOpType::UnknownOperation) : 
        larg_{std::move(larg)},
        rarg_{std::move(rarg)},
        binOp_{binOp} {};
    
    decltype(auto) getOp() const { return binOp_; };
    const AnyNode& getLarg() const {return larg_; }
    const AnyNode& getRarg() const {return rarg_; }
};

export class Expr final : private std::vector<AnyNode> 
{
public:
    explicit Expr(std::vector<AnyNode>&& nodes) noexcept
        : std::vector<AnyNode>(std::move(nodes)) {};

    using std::vector<AnyNode>::begin;
    using std::vector<AnyNode>::end;
    using std::vector<AnyNode>::cbegin;
    using std::vector<AnyNode>::cend;

    using std::vector<AnyNode>::size;
    using std::vector<AnyNode>::empty;
};

export class Assign final
{
    AnyNode larg_;
    AnyNode rarg_;

public:
    Assign(AnyNode&& larg, AnyNode&& rarg) : 
        larg_{std::move(larg)},
        rarg_{std::move(rarg)} {};

    const AnyNode& getLarg() const {return larg_; }
    const AnyNode& getRarg() const {return rarg_; }
};

















export template<typename... Types>
class TypeList {};

export using AvailableAstNodes = TypeList<
    Lit<int>,         // number literal
    Lit<std::string>, // string literal
    BinOp,            // binary opeeration +, -, *, /
    Expr,             // expression
    Assign
>;

        
}; // namespace ast
