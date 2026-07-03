module;

#include <any>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

export module ast_nodes_impl;

import ast_any_node_impl;

namespace ast
{

template <typename... ValidTypes>
auto
validate(const anyNode& node) -> bool
{
    bool match = ((node.type() == typeid(ValidTypes)) || ...);

    return match;
}

template <typename... ValidTypes>
void
restrictToTemplates(const anyNode& node)
{
    if (!validate<ValidTypes...>(node))
    {
        throw std::runtime_error(
            "Type mismatch: node does not match allowed types.");
    }
}

export template <typename LitT> class Lit
{
    LitT data_;

public:
    Lit() = default;
    Lit(LitT&& data) : data_{ std::move(data) } {}

    [[nodiscard]] auto
    data() const -> const LitT&
    {
        return data_;
    }
};

export class Var
{
    std::string data_;

public:
    Var() = default;
    Var(std::string_view data) : data_{ std::string(data) } {}

    [[nodiscard]] auto&
    data() const
    {
        return data_;
    }
};

export class BinOp final
{
public:
    enum class BinOpType : uint8_t
    {
        ADD,
        SUB,
        MUL,
        DIV,
        A,
        L,
        AE,
        LE,
        E,
        NE,
        UNKNOWN_OPERATION
    };

private:
    anyNode larg_;
    anyNode rarg_;
    BinOpType bin_op_;

public:
    BinOp() = default;

    BinOp(anyNode&& larg,
        anyNode&& rarg,
        BinOpType bin_op = BinOpType::UNKNOWN_OPERATION) :
        larg_{ std::move(larg) }, rarg_{ std::move(rarg) },
        bin_op_{ bin_op } {};

    [[nodiscard]] decltype(auto)
    getOp() const
    {
        return bin_op_;
    };
    [[nodiscard]] const auto&
    getLarg() const
    {
        return larg_;
    }
    [[nodiscard]] const auto&
    getRarg() const
    {
        return rarg_;
    }
};

export class Assign final
{
    anyNode larg_;
    anyNode rarg_;
    bool init_;

public:
    Assign() = default;

    Assign(anyNode&& larg, anyNode&& rarg, bool init = false) :
        larg_{ std::move(larg) }, rarg_{ std::move(rarg) }, init_{ init } {};

    [[nodiscard]] const anyNode&
    getLarg() const
    {
        return larg_;
    }

    [[nodiscard]] const anyNode&
    getRarg() const
    {
        return rarg_;
    }

    [[nodiscard]] bool
    isInitialisation() const
    {
        return init_;
    }
};

export class Block final : private std::vector<anyNode>
{
public:
    Block() = default;

    explicit Block(std::vector<anyNode>&& nodes) noexcept :
        std::vector<anyNode>(std::move(nodes)) {};

    explicit Block(std::vector<anyNode>& nodes) noexcept :
        std::vector<anyNode>(nodes) {};

    using std::vector<anyNode>::begin;
    using std::vector<anyNode>::end;
    using std::vector<anyNode>::cbegin;
    using std::vector<anyNode>::cend;

    using std::vector<anyNode>::size;
    using std::vector<anyNode>::empty;
};

export class IfElse final
{

    anyNode clause_;
    Block block_if_;
    Block block_else_;

public:
    IfElse() = default;

    IfElse(anyNode&& clause, anyNode&& block_if) :
        clause_{ std::move(clause) }, block_if_{ block_if.asMove<Block>() }
    {
        restrictToTemplates<BinOp, Lit<int>, Lit<std::string>>(clause_);
    }

    IfElse(anyNode&& clause, anyNode&& block_if, anyNode&& block_else) :
        clause_{ std::move(clause) }, block_if_{ block_if.asMove<Block>() },
        block_else_{ block_else.asMove<Block>() }
    {
        restrictToTemplates<BinOp, Lit<int>, Lit<std::string>>(clause_);
    }

    [[nodiscard]] const anyNode&
    getClause() const
    {
        return clause_;
    }
    [[nodiscard]] const Block&
    getIf() const
    {
        return block_if_;
    }
    [[nodiscard]] const Block&
    getElse() const
    {
        return block_else_;
    }
};

export class While final
{
    anyNode clause_;
    Block body_;

public:
    While() = default;

    While(anyNode&& clause, anyNode&& body) :
        clause_{ clause }, body_{ body.asMove<Block>() }
    {
        restrictToTemplates<BinOp, Lit<int>, Lit<std::string>>(clause_);
    }

    [[nodiscard]] const anyNode&
    getClause() const
    {
        return clause_;
    }
    [[nodiscard]] const Block&
    getBody() const
    {
        return body_;
    }
};

export class StructField final
{
    std::string name_;
    std::optional<anyNode> value_;

public:
    StructField() = default;

    StructField(std::string_view name) : name_{ name }, value_{ std::nullopt }
    {}

    StructField(std::string_view name, anyNode&& value) :
        name_{ name }, value_{ value }
    {}

    [[nodiscard]] const std::string&
    getName() const
    {
        return name_;
    }
    [[nodiscard]] const std::optional<anyNode>&
    getValue() const
    {
        return value_;
    }
};

export class Struct final : private std::vector<anyNode>
{
    std::string name_;

public:
    Struct() = default;

    Struct(std::string_view name, std::vector<anyNode>&& fields) :
        name_{ name }, std::vector<anyNode>(std::move(fields))
    {
        for (const auto& arg : *this)
        {
            restrictToTemplates<StructField>(arg);
        }
    }

    using std::vector<anyNode>::begin;
    using std::vector<anyNode>::end;
    using std::vector<anyNode>::cbegin;
    using std::vector<anyNode>::cend;

    using std::vector<anyNode>::size;
    using std::vector<anyNode>::empty;

    [[nodiscard]] std::string_view
    getName() const
    {
        return name_;
    }
};

export class StructEditor final
{
    std::string name_of_instance_;
    anyNode editable_field_;

public:
    StructEditor() = default;

    StructEditor(std::string_view name, anyNode&& field) :
        name_of_instance_{ std::string(name) },
        editable_field_{ std::move(field) }
    {
        restrictToTemplates<StructField>(editable_field_);
    }

    [[nodiscard]] std::string_view
    getNameOfInstance() const
    {
        return name_of_instance_;
    }
    [[nodiscard]] const anyNode&
    getEditableField() const
    {
        return editable_field_;
    }
};

export class Func final
{
    std::string name_;
    Struct args_;
    Block body_;

public:
    Func() = default;

    Func(std::string_view name, anyNode&& args, anyNode&& body) :
        name_{ std::string(name) }, args_{ args.asMove<Struct>() },
        body_{ body.asMove<Block>() }
    {}

    [[nodiscard]] std::string_view
    getName() const
    {
        return name_;
    }
    [[nodiscard]] const Struct&
    getArgs() const
    {
        return args_;
    }
    [[nodiscard]] const Block&
    getBody() const
    {
        return body_;
    }
};

export class FuncCall final
{
    std::string name_;
    Struct args_;

public:
    FuncCall() = default;

    FuncCall(std::string_view name, anyNode&& args) :
        name_{ std::string(name) }, args_(args.asMove<Struct>())
    {}

    [[nodiscard]] std::string_view
    getName() const
    {
        return name_;
    }
    [[nodiscard]] const Struct&
    getArgs() const
    {
        return args_;
    }
};



export template <typename... Types> class TypeList
{};

export using availableAstNodes = TypeList<Lit<int>, // number literal
    Lit<std::string>,                               // string literal
    Var,
    BinOp, // binary opeeration +, -, *, /, ...
    Assign,
    Block,
    IfElse,
    While,
    StructField,
    Struct,
    StructEditor,
    Func,
    FuncCall>;

}; // namespace ast
