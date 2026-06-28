module;

#include <any>
#include <iostream>
#include <optional>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

export module ast_impl;

namespace ast
{

template <typename T>
concept NotContainer = !requires(T t) {
    typename std::decay_t<T>::value_type;
    t.begin();
    t.end();
};

export class AnyNode
{
    std::any data_;

public:
    AnyNode() = default;

    template <typename T> // The Problem Of The Greedy Constructor
        requires NotContainer<std::decay_t<T>> &&
                 (!std::is_same_v<std::decay_t<T>, AnyNode>)
    AnyNode(T&& node) : data_{ std::forward<T>(node) }
    {}

    [[nodiscard]] auto
    type() const -> const std::type_info&
    {
        return data_.type();
    }

    template <typename T>
    auto
    as() const -> const T&
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

template <typename... ValidTypes>
auto
validate(const AnyNode& node) -> bool
{
    bool match = ((node.type() == typeid(ValidTypes)) || ...);

    if (!match)
    {
        std::cerr << "--- Type Mismatch ---\n";
        std::cerr << "Actual type inside AnyNode: " << node.type().name()
                  << "\n";
    }
    return match;
}

template <typename... ValidTypes>
void
restrictToTemplates(const AnyNode& node)
{
    if (!validate<ValidTypes...>(node))
    {
        throw std::runtime_error(
            "Type mismatch: node does not match allowed types.");
    }
}

export template <typename LitT> class Lit final
{
    LitT data_;

public:
    Lit(LitT&& data) : data_{ std::forward<LitT>(data) } {}

    [[nodiscard]] auto
    data() const -> const LitT&
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
    AnyNode larg_;
    AnyNode rarg_;
    BinOpType bin_op_;

public:
    BinOp(AnyNode&& larg,
        AnyNode&& rarg,
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
    AnyNode larg_;
    AnyNode rarg_;
    bool init_;

public:
    Assign(AnyNode&& larg, AnyNode&& rarg, bool init = false) :
        larg_{ std::move(larg) }, rarg_{ std::move(rarg) }, init_{ init } {};

    [[nodiscard]] const AnyNode&
    getLarg() const
    {
        return larg_;
    }

    [[nodiscard]] const AnyNode&
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

export class Block final : private std::vector<AnyNode>
{
public:
    Block() = default;

    explicit Block(std::vector<AnyNode>&& nodes) noexcept :
        std::vector<AnyNode>(std::move(nodes)) {};

    explicit Block(std::vector<AnyNode>& nodes) noexcept :
        std::vector<AnyNode>(nodes) {};

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
    IfElse(
        AnyNode&& clause, AnyNode&& block_if, AnyNode&& block_else = Block()) :
        clause_{ std::move(clause) }, block_if_{ block_if.asMove<Block>() },
        block_else_{ block_else.asMove<Block>() }
    {
        restrictToTemplates<BinOp, Lit<int>, Lit<std::string>>(clause_);
    }

    [[nodiscard]] const AnyNode&
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
    AnyNode clause_;
    Block body_;

public:
    While(AnyNode&& clause, AnyNode&& body) :
        clause_{ clause }, body_{ body.asMove<Block>() }
    {
        restrictToTemplates<BinOp, Lit<int>, Lit<std::string>>(clause_);
    }

    [[nodiscard]] const AnyNode&
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
    std::optional<AnyNode> value_;

public:
    StructField(std::string_view name) : name_{ name }, value_{ std::nullopt }
    {}

    StructField(std::string_view name, AnyNode&& value) :
        name_{ name }, value_{ value }
    {}

    [[nodiscard]] const std::string&
    getName() const
    {
        return name_;
    }
    [[nodiscard]] const std::optional<AnyNode>&
    getValue() const
    {
        return value_;
    }
};

export class Struct final : private std::vector<AnyNode>
{
    std::string name_;

public:
    Struct(std::string_view name, std::vector<AnyNode>&& fields) :
        name_{ name }, std::vector<AnyNode>(std::move(fields))
    {
        for (const auto& arg : *this)
        {
            restrictToTemplates<StructField>(arg);
        }
    }

    using std::vector<AnyNode>::begin;
    using std::vector<AnyNode>::end;
    using std::vector<AnyNode>::cbegin;
    using std::vector<AnyNode>::cend;

    using std::vector<AnyNode>::size;
    using std::vector<AnyNode>::empty;

    [[nodiscard]] std::string_view
    getName() const
    {
        return name_;
    }
};

export class StructEditor final
{
    std::string name_of_instance_;
    StructField editable_field_;

public:
    StructEditor(std::string_view name, AnyNode&& field) :
        name_of_instance_{ std::string(name) },
        editable_field_{ field.asMove<StructField>() }
    {
        restrictToTemplates<StructField>(field);
    }

    [[nodiscard]] std::string_view
    getNameOfInstance() const
    {
        return name_of_instance_;
    }
    [[nodiscard]] const StructField&
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
    Func(std::string_view name, AnyNode&& args, AnyNode&& body) :
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
    FuncCall(std::string_view name, AnyNode&& args) :
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
