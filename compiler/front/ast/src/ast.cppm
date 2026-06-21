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
    AnyNode() = default;

    template<typename T>
    AnyNode(T&& node) : data_{std::forward<T>(node)} {}

    const std::type_info& type() const {return data_.type(); }

    template<typename T>
    const T& as() const { return std::any_cast<const T&>(data_); }

    template<typename T>
    T&& as_move() { 
        return std::any_cast<T&&>(std::move(data_)); 
    }
};


template <typename... ValidTypes>
bool validate(const AnyNode& node)
{
    return ((node.type() == typeid(ValidTypes)) || ...);
}

template <typename... ValidTypes>
void validate_or_throw(const AnyNode& node) {
    if (!validate<ValidTypes...>(node)) {
        throw std::runtime_error("Type mismatch: node does not match allowed types.");
    }
}


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
        A,
        L,
        AE,
        LE,
        E,
        NE,
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

// export class BoolExpr final : private std::vector<AnyNode> 
// {
// public:
//     explicit BoolExpr(std::vector<AnyNode>&& nodes) noexcept
//         : std::vector<AnyNode>(std::move(nodes)) {};

//     using std::vector<AnyNode>::begin;
//     using std::vector<AnyNode>::end;
//     using std::vector<AnyNode>::cbegin;
//     using std::vector<AnyNode>::cend;

//     using std::vector<AnyNode>::size;
//     using std::vector<AnyNode>::empty;
// };

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

export class Block final : private std::vector<AnyNode> 
{
public:
    Block() = default;

    explicit Block(std::vector<AnyNode>&& nodes) noexcept
        : std::vector<AnyNode>(std::move(nodes)) {};
    
    explicit Block(std::vector<AnyNode>& nodes) noexcept
        : std::vector<AnyNode>(nodes) {};


    using std::vector<AnyNode>::begin;
    using std::vector<AnyNode>::end;
    using std::vector<AnyNode>::cbegin;
    using std::vector<AnyNode>::cend;

    using std::vector<AnyNode>::size;
    using std::vector<AnyNode>::empty;
};

export class IfElse final
{

    AnyNode clause_;
    Block block_if_;
    Block block_else_;

public:
    IfElse(AnyNode&& clause, AnyNode&& block_if)
            : clause_{std::move(clause)},
            block_if_{block_if.as_move<Block>()} {
            validate_or_throw<BinOp, Lit<int>, Lit<std::string>>(clause_);
        }
    IfElse(AnyNode&& clause, AnyNode&& block_if, AnyNode&& block_else)
            : clause_{std::move(clause)},
            block_if_{block_if.as_move<Block>()},
            block_else_{block_else.as_move<Block>()} {
            validate_or_throw<BinOp, Lit<int>, Lit<std::string>>(clause_);
        }

    const AnyNode& getClause() const {return clause_; }
    const Block& getIf() const {return block_if_; }
    const Block& getElse() const {return block_else_; }
};













export template<typename... Types>
class TypeList {};

export using AvailableAstNodes = TypeList<
    Lit<int>,         // number literal
    Lit<std::string>, // string literal
    BinOp,            // binary opeeration +, -, *, /
    Assign,
    Block,
    IfElse
>;

        
}; // namespace ast
