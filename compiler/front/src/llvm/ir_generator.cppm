module;
#include <algorithm>
#include <functional>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <ranges>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <string_view>

#include "spdlog/spdlog.h"

export module ir_generator;

import ast;

namespace ir_generator
{










// JUST FOR DEBUG










void
spdlogInit()
{
    spdlog::set_pattern("[%H:%M:%S.%e]\table[%n]: [%^%l%$] %v");

    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("debug.log", true);

    auto make_logger = [&](const std::string& name) -> void
    {
        auto logger = std::make_shared<spdlog::logger>(name, file_sink);
        spdlog::register_logger(logger);
    };

    make_logger("Symbol table");
    make_logger("visit");
    make_logger("general");
    make_logger("Scanner");

    spdlog::flush_on(spdlog::level::trace);

    spdlog::set_level(spdlog::level::debug);
}

auto
llvmValueToTypeStr(const llvm::Value* val) -> std::string
{
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    val->getType()->print(rso);

    return type_str;
}










struct VarData
{
    llvm::Value* value_{};
    ast::AnyNode type_info_;

    auto
    operator==(const VarData& other) const -> bool
    {
        return value_ == other.value_;
    }
};

using scope          = std::unordered_map<std::string, VarData>;
using objectIterator = scope::iterator;


class SymbolTable final
{
    struct ScopeData
    {
        scope symbols_;
        std::weak_ptr<ScopeData> parent_;
        std::vector<std::shared_ptr<ScopeData>> children_;
        size_t next_child_ = 0;

        auto
        operator==(const ScopeData& other) const -> bool
        {
            if (symbols_ != other.symbols_)
            {
                return false;
            }

            if (children_.size() != other.children_.size())
            {
                return false;
            }

            return std::ranges::equal(children_,
                other.children_,
                [](const auto& left_operand, const auto& right_operand) -> bool
                { return *left_operand == *right_operand; });
        }

        auto
        operator!=(const ScopeData& other) const -> bool
        {
            return !(*this == other);
        }
    };

    std::shared_ptr<ScopeData> global_scope_;
    std::shared_ptr<ScopeData> current_;

    size_t depth_{ 0 };

public:
    auto
    findObj(const std::string& name) -> std::optional<objectIterator>
    {
        if (!current_)
        {
            throw std::logic_error("No current scope.");
        }

        spdlog::get("Symbol table")
            ->info(std::format(
                "Try to found node \"{}\" at depth {}.", name, depth_));

        for (auto scope_it = current_; scope_it != nullptr;
            scope_it       = scope_it->parent_.lock())
        {

            auto symbol_it = scope_it->symbols_.find(name);

            if (symbol_it != scope_it->symbols_.end())
            {
                return symbol_it;
            }
        }

        return std::nullopt;
    }

    void
    deepenScope()
    {
        if (current_->next_child_ < current_->children_.size())
        {
            current_ = current_->children_[current_->next_child_];
        }
        else
        {
            auto new_scope     = std::make_shared<ScopeData>();
            new_scope->parent_ = current_;
            current_->children_.push_back(new_scope);
            current_ = new_scope;
        }
        current_->next_child_ = 0;
        ++depth_;
    }

    void
    riseScope()
    {
        if (depth_ > 0 && current_)
        {
            current_ = current_->parent_.lock();
            ++current_->next_child_;
            --depth_;
        }
    }

    void
    setObj(const std::string& name,
        llvm::Value* llvm_val,
        ast::AnyNode type_info = ast::AnyNode(ast::Var(std::string{})))
    {
        if (!current_)
        {
            throw std::logic_error("No active scope.");
        }

        spdlog::get("Symbol table")
            ->info(std::format("Add new symbol {} = {} [depth {}]",
                name,
                (llvm_val != nullptr) ? llvmValueToTypeStr(llvm_val)
                                      : "nullptr",
                depth_));

        current_->symbols_[name] =
            VarData{ .value_ = llvm_val, .type_info_ = std::move(type_info) };
    }

    void
    updateObj(const std::string& name, llvm::Value* llvm_val)
    {
        auto it_opt = findObj(name);

        spdlog::get("Symbol table")
            ->info(std::format(
                "Update {} = {}", name, llvmValueToTypeStr(llvm_val)));

        if (!it_opt.has_value())
        {
            throw std::runtime_error(std::format(
                "Variable \"{}\" used before initialisation", name));
        }

        it_opt.value()->second.value_ = llvm_val;
    }

    auto
    getValue(const std::string& name)
    {
        auto it_opt = findObj(name);
        if (it_opt.has_value())
        {
            return it_opt.value()->second.value_;
        }

        throw std::runtime_error("Symbol not found: " + name);
    }

    [[nodiscard]] auto
    getDepth() const noexcept -> size_t
    {
        return depth_;
    }

    [[nodiscard]] auto
    getCurrentScope() const noexcept -> const ScopeData&
    {
        return *current_;
    }

    [[nodiscard]] auto
    getRootScope() const noexcept -> const ScopeData&
    {
        return *global_scope_;
    }

    void
    resetNavigation()
    {
        std::function<void(std::shared_ptr<ScopeData>&)> reset =
            [&](auto& scope) -> auto
        {
            scope->next_child_ = 0;

            for (auto& child : scope->children_)
            {
                reset(child);
            }
        };
        reset(global_scope_);
        current_ = global_scope_;
        depth_   = 0;
    }

    SymbolTable() :
        global_scope_(std::make_shared<ScopeData>()), current_(global_scope_)
    {}

    SymbolTable(const SymbolTable&) = delete;
    SymbolTable(SymbolTable&&)      = delete;
    auto
    operator=(const SymbolTable&) -> SymbolTable& = delete;
    auto
    operator=(SymbolTable&&) -> SymbolTable& = delete;
    ~SymbolTable()                           = default;
};










// ascii symbol of spaace
const uint8_t space_ascii_c = 32;

// ascii symbol of /
const uint8_t slash_ascii_c = 47;

// ascii symbol of :
const uint8_t colon_ascii_c = 58;

// ascii symbol of >
const uint8_t above_ascii_c = 62;

std::string
clearName(std::string_view name)
{
    // clang-format off
    return name
    | std::views::filter(
            [](char symbol) 
            {
                return (symbol < space_ascii_c)
                    || (symbol > slash_ascii_c && symbol < colon_ascii_c) 
                    || (symbol > colon_ascii_c && symbol < above_ascii_c)
                    || (symbol > above_ascii_c);
            }
        )
    | std::ranges::to<std::string>();
    // clang-format on
}










// ----------------------------------------------------------------------------
// Second pass: The main pass that generates LLVM IR. It relies on the symbol
// table initialised in the first pass. (See the first pass below.)
// ----------------------------------------------------------------------------










class GenContext final
{
public:
    llvm::IRBuilder<>& b_;
    llvm::Module& m_;
    SymbolTable t_;
};

#define BUILDER_MODULE_TABLE_M                                                 \
    auto&& table   = ctx.t_;                                                   \
    auto&& builder = ctx.b_;                                                   \
    auto&& module  = ctx.m_;

auto
visit(GenContext& ctx, const ast::Lit<int>& node)
{
    return llvm::ConstantInt::get(ctx.b_.getInt32Ty(), node.data());
}

auto
visit(GenContext& ctx, const ast::Lit<std::string>& node)
{
    auto&& named_val = ctx.m_.getNamedValue(clearName(node.data()));

    auto* global_var = (named_val != nullptr)
                           ? llvm::dyn_cast<llvm::GlobalVariable>(named_val)
                           : nullptr;

    if (global_var == nullptr)
    {
        throw std::runtime_error(std::format(
            "String global '{}' not found in module", clearName(node.data())));
    }

    llvm::Value* zero = ctx.b_.getInt32(0);

    return ctx.b_.CreateInBoundsGEP(
        global_var->getValueType(), global_var, { zero, zero }, "str_ptr");
}

auto
visit(GenContext& ctx, const ast::Var& node)
{
    llvm::Value* var_ptr = ctx.t_.getValue(node.data());

    if (ctx.t_.findObj(node.data()).value()->second.type_info_.type() ==
        typeid(ast::Lit<std::string>))
    {
        return ctx.b_.CreateLoad(ctx.b_.getPtrTy(), var_ptr, node.data());
    }

    return ctx.b_.CreateLoad(ctx.b_.getInt32Ty(), var_ptr, node.data());
}

auto
visit(GenContext& ctx, const ast::Assign& node)
{
    auto&& var      = node.getLarg();
    auto&& expr     = node.getRarg();
    auto&& var_name = var.as<ast::Var>().data();

    auto&& init_val = ast::visit<llvm::Value*>(ctx, expr);

    llvm::Value* alloca = nullptr;

    auto obj_it = ctx.t_.findObj(var_name);

    if (!obj_it.has_value())
    {
        throw std::runtime_error(std::format(
            "Variable \"{}\" used before initialisation", var_name));
    }

    alloca = obj_it.value()->second.value_;

    assert(alloca);

    if (init_val != nullptr)
    {
        ctx.b_.CreateStore(init_val, alloca);
    }

    return alloca;
}

llvm::Value*
visit(GenContext& ctx, const ast::BinOp& node)
{
    BUILDER_MODULE_TABLE_M;

    auto&& left  = ast::visit<llvm::Value*>(ctx, node.getLarg());
    auto&& right = ast::visit<llvm::Value*>(ctx, node.getRarg());

    using opEnum = ast::BinOp::BinOpType;

    llvm::Value* operation = nullptr;

    switch (node.getOp())
    {
    case opEnum::ADD:
    {
        operation = builder.CreateAdd(left, right, "add");
        break;
    }
    case opEnum::SUB:
    {
        operation = builder.CreateSub(left, right, "sub");
        break;
    }
    case opEnum::MUL:
    {
        operation = builder.CreateMul(left, right, "mul");
        break;
    }
    case opEnum::DIV:
    {
        operation = builder.CreateFDiv(left, right, "div");
        break;
    }

    case opEnum::A:
    {
        operation = builder.CreateICmpSGT(left, right, "a");
        break;
    }
    case opEnum::AE:
    {
        operation = builder.CreateICmpSGE(left, right, "ae");
        break;
    }
    case opEnum::L:
    {
        operation = builder.CreateICmpSLT(left, right, "l");
        break;
    }
    case opEnum::LE:
    {
        operation = builder.CreateICmpSLE(left, right, "le");
        break;
    }
    case opEnum::E:
    {
        operation = builder.CreateICmpEQ(left, right, "eq");
        break;
    }
    case opEnum::NE:
    {
        operation = builder.CreateICmpNE(left, right, "ne");
        break;
    }

    default:
    {
        throw std::logic_error("Unknown binary operation.");
        break;
    }
    }

    return operation;
}

auto
visit(GenContext& ctx, const ast::IfElse& node)
{
    BUILDER_MODULE_TABLE_M;

    auto&& func    = builder.GetInsertBlock()->getParent();
    auto&& context = module.getContext();

    auto&& clause = ast::visit<llvm::Value*>(ctx, node.getClause());

    auto&& if_label    = llvm::BasicBlock::Create(context, "if", func);
    auto&& else_label  = llvm::BasicBlock::Create(context, "else", func);
    auto&& merge_label = llvm::BasicBlock::Create(context, "endif", func);

    builder.CreateCondBr(clause, if_label, else_label);

    builder.SetInsertPoint(if_label);
    auto&& val_from_if = ast::visit<llvm::Value*>(ctx, node.getIf());
    builder.CreateBr(merge_label);

    builder.SetInsertPoint(else_label);
    auto&& val_from_else = ast::visit<llvm::Value*>(ctx, node.getElse());
    builder.CreateBr(merge_label);

    builder.SetInsertPoint(merge_label);
    auto&& phi = builder.CreatePHI(val_from_if->getType(), 2);
    phi->addIncoming(val_from_if, if_label);
    phi->addIncoming(val_from_else, else_label);

    return phi;
}

struct GlobalBlock
{};

auto
visit(GenContext& ctx, const ast::Block& block, GlobalBlock /*unused*/)
{
    // FIXME -----------------------------!!!FOR A
    // WHILE!!!-----------------------------
    // FIXME This function may be removed in the near future. For the first
    // block,
    // FIXME there is no need to go into the table due to the default table
    // constructor, as
    // FIXME we are already in the global scope. The structure "global_block"
    // FIXME is formally designed to search for a specific signature of the
    // `visit` function.

    llvm::Value* last_val = nullptr;

    for (auto&& stmt : block)
    {
        last_val = ast::visit<llvm::Value*>(ctx, stmt);
    }

    return last_val;
}

llvm::Value*
visit(GenContext& ctx, const ast::Block& block)
{
    llvm::Value* last_val = nullptr;

    ctx.t_.deepenScope();

    for (auto&& stmt : block)
    {
        last_val = ast::visit<llvm::Value*>(ctx, stmt);
    }

    ctx.t_.riseScope();

    return last_val;
}

auto
visit(GenContext& ctx, const ast::Func& func)
{
    BUILDER_MODULE_TABLE_M;

    llvm::Function* llvm_func = module.getFunction(func.getName());

    auto&& old_label = builder.GetInsertBlock();

    llvm::BasicBlock& entry = llvm_func->getEntryBlock();
    builder.SetInsertPoint(&entry);

    table.deepenScope();

    for (auto&& arg : func.getArgs())
    {
        auto&& struct_field = arg.as<ast::StructField>();

        auto&& arg_name = struct_field.getName();
        auto&& raw_val  = struct_field.getValue();

        if (raw_val.has_value())
        {
            auto&& arg_it = table.findObj(arg_name);

            if (!arg_it.has_value())
            {
                throw std::runtime_error(std::format(
                    "Variable \"{}\" used before initialisation", arg_name));
            }

            llvm::Value* alloca = arg_it.value()->second.value_;

            auto&& init_val = ast::visit<llvm::Value*>(ctx, raw_val.value());

            builder.CreateStore(init_val, alloca);
        }
    }

    auto&& block_val = ast::visit<llvm::Value*>(ctx, func.getBody());

    builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));

    table.riseScope();
    builder.SetInsertPoint(old_label);

    return block_val;
}

auto
visit(GenContext& ctx, const ast::FuncCall& node)
{
    BUILDER_MODULE_TABLE_M;

    llvm::Function* llvm_func = module.getFunction(node.getName());

    if (llvm_func == nullptr)
    {
        throw std::runtime_error(
            std::format("Function not found: {}", node.getName()));
    }

    std::vector<llvm::Value*> llvm_all_args;

    for (auto&& arg : node.getArgs())
    {
        auto&& struct_field = arg.as<ast::StructField>();
        auto&& arg_name     = struct_field.getName();

        auto&& raw_val = struct_field.getValue();

        if (raw_val.has_value())
        {
            auto&& arg_it =
                table.findObj(raw_val.value().as<ast::Var>().data());

            if (!arg_it.has_value())
            {
                throw std::runtime_error(std::format(
                    "Variable \"{}\" used before initialisation", arg_name));
            }

            llvm::Value* alloca = arg_it.value()->second.value_;

            llvm_all_args.push_back(alloca);
        }
    }

    llvm::Value* call = builder.CreateCall(llvm_func, llvm_all_args);

    return call;
}

auto
visit(GenContext& ctx, const ast::While& node)
{
    BUILDER_MODULE_TABLE_M;


    auto&& func    = builder.GetInsertBlock()->getParent();
    auto&& context = module.getContext();

    auto&& condition_label =
        llvm::BasicBlock::Create(context, "whileCond", func);
    auto&& true_label  = llvm::BasicBlock::Create(context, "whileTrue", func);
    auto&& false_label = llvm::BasicBlock::Create(context, "whileFalse", func);

    builder.CreateBr(condition_label);

    builder.SetInsertPoint(condition_label);
    auto&& condition = ast::visit<llvm::Value*>(ctx, node.getClause());
    builder.CreateCondBr(condition, true_label, false_label);

    builder.SetInsertPoint(true_label);
    auto&& val_from_if = ast::visit<llvm::Value*>(ctx, node.getBody());
    builder.CreateBr(condition_label);

    builder.SetInsertPoint(false_label);

    return nullptr;
}

auto
visit(GenContext& ctx, const ast::Struct& node)
{
    // TODO It needs finishing; the structure declaration doesn’t mean anything
    // yet ????????????
    return nullptr;
}

auto
visit(GenContext& ctx, const ast::StructEditor& node)
{
    BUILDER_MODULE_TABLE_M;

    auto&& instance         = node.getNameOfInstance();
    auto&& changeable_field = node.getEditableField();

    auto&& struct_entry = table.findObj(std::string(instance)).value()->second;
    auto&& instance_type_info = struct_entry.type_info_.as<ast::Struct>();

    uint32_t field_index = 0;
    bool found           = false;
    uint32_t idx         = 0;

    for (auto&& field : instance_type_info)
    {
        auto&& struct_field = field.as<ast::StructField>();
        if (struct_field.getName() == changeable_field.getName())
        {
            field_index = idx;
            found       = true;
            break;
        }
        ++idx;
    }

    if (!found)
    {
        throw std::runtime_error(
            std::format(R"(Field "{}" not found in struct "{}")",
                changeable_field.getName(),
                instance_type_info.getName()));
    }

    auto&& struct_ptr = struct_entry.value_;

    llvm::Type* struct_type = llvm::StructType::getTypeByName(
        module.getContext(), instance_type_info.getName());

    auto&& field_ptr = builder.CreateGEP(struct_type,
        struct_ptr,
        { builder.getInt32(0), builder.getInt32(field_index) });

    auto&& expression =
        ast::visit<llvm::Value*>(ctx, changeable_field.getValue().value());

    builder.CreateStore(expression, field_ptr);

    return field_ptr;
}










// ----------------------------------------------------------------------------
// First pass: Scanning the programme for variable initialisations.
// This is necessary to ensure that all "alloc" statements appear at the start
// of functions in LLVM IR (in the "entry" block). Furthermore, thanks to the
// first pass, the types of all variables are already known by the time the
// second pass begins.
// ----------------------------------------------------------------------------










class FirstPass
{};

template <typename NodeT>
auto
visit(GenContext& ctx, const NodeT& node, FirstPass /*unused*/)
{}

auto
visit(GenContext& ctx, const ast::Assign& node, FirstPass /*unused*/)
{

    BUILDER_MODULE_TABLE_M;

    if (node.isInitialisation())
    {
        auto&& var       = node.getLarg();
        auto&& rarg      = node.getRarg();
        auto&& rarg_type = rarg.type();
        auto&& name      = var.as<ast::Var>().data();

        llvm::Value* alloca = nullptr;

        if (rarg_type == typeid(ast::Struct))
        {
            spdlog::get("visit")->info(
                std::format("Init var {} with struct.", name));

            auto&& struct_entry =
                table.findObj(std::string(rarg.as<ast::Struct>().getName()))
                    .value()
                    ->second;

            auto&& struct_node = struct_entry.type_info_.as<ast::Struct>();

            llvm::StructType* struct_type =
                llvm::StructType::create(module.getContext(),
                    std::vector<llvm::Type*>(
                        struct_node.size(), builder.getInt32Ty()),
                    struct_node.getName());

            alloca = builder.CreateAlloca(struct_type);

            table.setObj(name, alloca, struct_node);
        }
        else if (rarg_type == typeid(ast::Lit<std::string>))
        {
            auto&& str_lit_node = rarg.as<ast::Lit<std::string>>();

            auto&& _ = ctx.b_.CreateGlobalString(
                str_lit_node.data(), clearName(str_lit_node.data()));

            alloca = builder.CreateAlloca(builder.getPtrTy(), nullptr, name);

            table.setObj(name, alloca, rarg);
        }
        else
        {
            alloca = builder.CreateAlloca(builder.getInt32Ty(), nullptr, name);

            table.setObj(name, alloca);
        }
    }
}

void
visit(GenContext& ctx, const ast::Block& node, FirstPass /*unused*/)
{
    BUILDER_MODULE_TABLE_M;

    table.deepenScope();
    for (auto&& stmt : node)
    {
        ast::visit<void>(ctx, stmt, FirstPass{});
    }
    table.riseScope();
}

void
visit(GenContext& ctx, const ast::Func& node, FirstPass /*unused*/)
{
    BUILDER_MODULE_TABLE_M;

    if (table.getCurrentScope() != table.getRootScope())
    {
        throw std::logic_error("It is not possible to define functions outside "
                               "the global scope.(possibly temporarily)");
    }

    auto&& old_label = builder.GetInsertBlock();

    std::vector<llvm::Type*> arg_types(
        node.getArgs().size(), builder.getInt32Ty());

    llvm::FunctionType* func_type =
        llvm::FunctionType::get(builder.getInt32Ty(), arg_types, false);

    llvm::Function* llvm_func = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage, node.getName(), module);

    table.deepenScope();

    // func label
    auto&& new_label =
        llvm::BasicBlock::Create(module.getContext(), "entry", llvm_func);

    builder.SetInsertPoint(new_label);

    spdlog::get("visit")->info(
        std::format("Starting analyzing args of function {}", node.getName()));

    for (auto&& arg : node.getArgs())
    {
        llvm::Value* alloca = builder.CreateAlloca(builder.getInt32Ty(),
            nullptr,
            arg.as<ast::StructField>().getName());
        table.setObj(arg.as<ast::StructField>().getName(), alloca);
    }

    spdlog::get("visit")->info(
        std::format("Starting analyzing body of function {}", node.getName()));

    for (auto&& stmt : node.getBody())
    {
        ast::visit<void>(ctx, stmt, FirstPass{});
    }

    table.riseScope();
    builder.SetInsertPoint(old_label);
}

void
visit(GenContext& ctx, const ast::IfElse& node, FirstPass /*unused*/)
{

    ast::visit<void>(ctx, node.getIf(), FirstPass{});
    ast::visit<void>(ctx, node.getElse(), FirstPass{});
}

void
visit(GenContext& ctx, const ast::Struct& node, FirstPass /*unused*/)
{
    ctx.t_.setObj(std::string(node.getName()), nullptr, node);
}

void
visit(GenContext& ctx, const ast::While& node, FirstPass /*unused*/)
{
    ast::visit<void>(ctx, node.getBody(), FirstPass{});
}

void
scanForInitialisations(GenContext& ctx, const ast::AnyNode& root)
{

    spdlog::get("Scanner")->info(
        std::format("Start searching initializations"));

    // FIXME -----------------------------!!!FOR A
    // WHILE!!!-----------------------------
    // FIXME This function may be removed in the near future.(problem of
    // global scope)
    for (auto&& stmt : root.as<ast::Block>())
    {
        ast::visit<void>(ctx, stmt, FirstPass{});
    }
}

#undef BUILDER_MODULE_TABLE_M










export void
toLLVMIR(const ast::AnyNode& root, std::string_view filename)
{
    spdlogInit();

    spdlog::get("general")->info(std::format("Starting LLVM IR generation"));

    llvm::LLVMContext context;
    llvm::Module module("main", context);
    module.setTargetTriple(llvm::Triple(llvm::sys::getDefaultTargetTriple()));
    llvm::IRBuilder<> builder(context);

    auto&& func_type = llvm::FunctionType::get(builder.getInt32Ty(), false);
    auto&& func      = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage, "main", module);

    auto&& entry = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entry);

    // Generate context info during all recursion

    GenContext ctx{ .b_ = builder, .m_ = module };
    scanForInitialisations(ctx, root);

    spdlog::get("general")->info(
        std::format("Initialisation scanning has been finished"));

    ctx.t_.resetNavigation();

    spdlog::get("general")->info(std::format("Start creating ir."));

    auto&& res = ast::visit<llvm::Value*>(ctx, root, GlobalBlock{});
    builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));

    std::error_code error_code;
    llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);
    module.print(dest, nullptr);
}

} // namespace ir_generator
