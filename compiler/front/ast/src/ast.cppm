module;

#include <string>
#include <iostream>
#include <typeinfo>
#include <utility>
#include <any>
#include <functional>
#include <memory>
#include <optional>

export module ast_impl;

namespace ast {

template<typename T>
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
    
    template<typename T> // The Problem Of The Greedy Constructor
        requires NotContainer<std::decay_t<T>> && (!std::is_same_v<std::decay_t<T>, AnyNode>)
    AnyNode(T&& node) : data_{std::forward<T>(node)} {}

    const std::type_info& type() const {return data_.type(); }

    template<typename T>
    const T& as() const { 
        try {
            return std::any_cast<const T&>(data_);
        } catch (const std::bad_any_cast&)
        {
            throw std::runtime_error(
                "AnyNode: Type mismatch during cast. Requested type: " + 
                std::string(typeid(T).name()) + 
                ", Actual type: " + 
                std::string(data_.type().name())
            );
        }
    }

    template<typename T>
    T&& as_move() { 
        return std::any_cast<T&&>(std::move(data_)); 
    }
};

template <typename... ValidTypes>
bool validate(const AnyNode& node)
{
    bool match = ((node.type() == typeid(ValidTypes)) || ...);
    
    if (!match) {
        std::cerr << "--- Type Mismatch ---\n";
        std::cerr << "Actual type inside AnyNode: " << node.type().name() << "\n";
    }
    return match;
}

template <typename... ValidTypes>
void restrict_to_templates(const AnyNode& node) {
    if (!validate<ValidTypes...>(node)) {
        throw std::runtime_error("Type mismatch: node does not match allowed types.");
    }
}


export template<typename LitT>
class Lit final
{
    LitT data_;

public:
    Lit(LitT&& data) : data_{std::forward<LitT>(data)} {}

    const LitT& data() const { return data_; }
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

export class Assign final
{
    AnyNode larg_;
    AnyNode rarg_;
    bool init_;

public:
    Assign(AnyNode&& larg, AnyNode&& rarg, bool init = false) : 
        larg_{std::move(larg)},
        rarg_{std::move(rarg)},
        init_{init} {};

    const AnyNode& getLarg() const {return larg_; }
    const AnyNode& getRarg() const {return rarg_; }
    const bool isInitialisation() const { return init_; }
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
    IfElse(AnyNode&& clause, AnyNode&& block_if, AnyNode&& block_else = Block())
            : clause_{std::move(clause)},
            block_if_{block_if.as_move<Block>()},
            block_else_{block_else.as_move<Block>()} {
            restrict_to_templates<BinOp, Lit<int>, Lit<std::string>>(clause_);
        }

    const AnyNode& getClause() const {return clause_; }
    const Block& getIf() const {return block_if_; }
    const Block& getElse() const {return block_else_; }
};

export class While final
{
    AnyNode clause_;
    Block body_;

public:
    While(AnyNode&& clause, AnyNode&& body)
        : clause_{clause},
        body_{body.as_move<Block>()} {
        restrict_to_templates<BinOp, Lit<int>, Lit<std::string>>(clause_);
    }

    const AnyNode& getClause() const { return clause_; }
    const Block&   getBody()   const { return body_;  }
};

export class StructField final
{
    std::string name_;
    std::optional<AnyNode> value_;

public:
    StructField(std::string_view name) :
        name_{name},
        value_{std::nullopt} {}

    StructField(std::string_view name, AnyNode&& value) :
        name_{name},
        value_{value} {}

    const std::string& getName() const { return name_; }
    const std::optional<AnyNode>& getValue() const { return value_; }

};

export class Struct final : private std::vector<AnyNode>
{
    std::string name_;
public:
    Struct(std::string_view name, std::vector<AnyNode>&& fields) :
        name_{name},
        std::vector<AnyNode>(std::move(fields))
    {
        for (const auto& arg : *this) restrict_to_templates<StructField>(arg);
    }

    using std::vector<AnyNode>::begin;
    using std::vector<AnyNode>::end;
    using std::vector<AnyNode>::cbegin;
    using std::vector<AnyNode>::cend;

    using std::vector<AnyNode>::size;
    using std::vector<AnyNode>::empty;

    const std::string_view getName() const { return name_; }
};

export class StructEditor final
{
    std::string name_of_instance_;
    StructField editable_field_;
public:
    StructEditor(std::string_view name, AnyNode&& field) :
        name_of_instance_{std::string(name)},
        editable_field_{field.as_move<StructField>()}
    {
        restrict_to_templates<StructField>(field);
    }

    const std::string_view getNameOfInstance() const { return name_of_instance_; }
    const StructField& getEditableField() const { return editable_field_; }
};

export class Func final
{
    std::string name_;
    Struct args_;
    Block body_;

public:
    Func(std::string_view name, AnyNode&& args, AnyNode&& body) :
        name_{std::string(name)},
        args_{args.as_move<Struct>()},
        body_{body.as_move<Block>()} {}

    std::string_view getName() const { return name_; }
    const Struct& getArgs() const { return args_; }
    const Block&   getBody() const { return body_;  }
};

export class FuncCall final
{
    std::string name_;
    Struct args_;

public:
    FuncCall(std::string_view name, AnyNode&& args) :
        name_{std::string(name)},
        args_(args.as_move<Struct>()) {}

    std::string_view getName() const { return name_; }
    const Struct& getArgs() const { return args_; }
};











export template<typename... Types>
class TypeList {};

export using AvailableAstNodes = TypeList<
    Lit<int>,         // number literal
    Lit<std::string>, // string literal
    BinOp,            // binary opeeration +, -, *, /, ...
    Assign,
    Block,
    IfElse,
    While,
    StructField,
    Struct,
    StructEditor,
    Func,
    FuncCall
>;

        
}; // namespace ast
