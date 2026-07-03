module;

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
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
#include <stdexcept>
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










class SymbolTable final
{
    struct VarData
    {
        llvm::Value* value_{};
        ast::anyNode type_info_;

        auto
        operator==(const VarData& other) const -> bool
        {
            return value_ == other.value_;
        }
    };

    using scope          = std::unordered_map<std::string, VarData>;
    using objectIterator = scope::iterator;

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
    findObj(std::string_view name) -> std::optional<objectIterator>
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

            auto symbol_it = scope_it->symbols_.find(std::string(name));

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
        ast::anyNode type_info = ast::anyNode(ast::Var(std::string{})))
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
visit(
    const ast::anyNode& node, const ast::Lit<int>& /*unused*/, GenContext& ctx)
{
    auto& lit = node.as<ast::Lit<int>>();
    return llvm::ConstantInt::get(ctx.b_.getInt32Ty(), lit.data());
}

auto
visit(const ast::anyNode& node,
    const ast::Lit<std::string>& /*unused*/,
    GenContext& ctx)
{
    auto& str_lit    = node.as<ast::Lit<std::string>>();
    auto&& named_val = ctx.m_.getNamedValue(clearName(str_lit.data()));

    auto* global_var = (named_val != nullptr)
                           ? llvm::dyn_cast<llvm::GlobalVariable>(named_val)
                           : nullptr;

    if (global_var == nullptr)
    {
        throw std::runtime_error(
            std::format("String global '{}' not found in module",
                clearName(str_lit.data())));
    }

    llvm::Value* zero = ctx.b_.getInt32(0);

    return ctx.b_.CreateInBoundsGEP(
        global_var->getValueType(), global_var, { zero, zero }, "str_ptr");
}

auto
visit(const ast::anyNode& node, const ast::Var& /*unused*/, GenContext& ctx)
{
    auto& var            = node.as<ast::Var>();
    llvm::Value* var_ptr = ctx.t_.getValue(var.data());

    if (ctx.t_.findObj(var.data()).value()->second.type_info_.type() ==
        typeid(ast::Lit<std::string>))
    {
        return ctx.b_.CreateLoad(ctx.b_.getPtrTy(), var_ptr, var.data());
    }

    return ctx.b_.CreateLoad(ctx.b_.getInt32Ty(), var_ptr, var.data());
}

auto
visit(const ast::anyNode& node, const ast::Assign& /*unused*/, GenContext& ctx)
{
    auto& assign    = node.as<ast::Assign>();
    auto&& var      = assign.getLarg();
    auto&& expr     = assign.getRarg();
    auto&& var_name = var.as<ast::Var>().data();

    auto&& init_val = ast::visit<llvm::Value*>(expr, ctx);

    llvm::Value* alloca = nullptr;

    auto obj_it = ctx.t_.findObj(var_name);

    if (!obj_it.has_value())
    {
        node.printError();
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
visit(const ast::anyNode& node, const ast::BinOp& /*unused*/, GenContext& ctx)
{
    BUILDER_MODULE_TABLE_M;

    auto& binop  = node.as<ast::BinOp>();
    auto&& left  = ast::visit<llvm::Value*>(binop.getLarg(), ctx);
    auto&& right = ast::visit<llvm::Value*>(binop.getRarg(), ctx);

    using opEnum = ast::BinOp::BinOpType;

    llvm::Value* operation = nullptr;

    switch (binop.getOp())
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
visit(const ast::anyNode& node, const ast::IfElse& /*unused*/, GenContext& ctx)
{
    BUILDER_MODULE_TABLE_M;

    auto& ifelse   = node.as<ast::IfElse>();
    auto&& func    = builder.GetInsertBlock()->getParent();
    auto&& context = module.getContext();

    auto&& clause = ast::visit<llvm::Value*>(ifelse.getClause(), ctx);

    auto&& if_label    = llvm::BasicBlock::Create(context, "if", func);
    auto&& else_label  = llvm::BasicBlock::Create(context, "else", func);
    auto&& merge_label = llvm::BasicBlock::Create(context, "endif", func);

    builder.CreateCondBr(clause, if_label, else_label);

    builder.SetInsertPoint(if_label);
    auto&& val_from_if = ast::visit<llvm::Value*>(ifelse.getIf(), ctx);
    builder.CreateBr(merge_label);

    builder.SetInsertPoint(else_label);
    auto&& val_from_else = ast::visit<llvm::Value*>(ifelse.getElse(), ctx);
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
visit(const ast::anyNode& node,
    const ast::Block& /*unused*/,
    GlobalBlock /*unused*/,
    GenContext& ctx)
{
    auto& block           = node.as<ast::Block>();
    llvm::Value* last_val = nullptr;

    for (auto&& stmt : block)
    {
        last_val = ast::visit<llvm::Value*>(stmt, ctx);
    }

    return last_val;
}

llvm::Value*
visit(const ast::anyNode& node, const ast::Block& /*unused*/, GenContext& ctx)
{
    auto& block           = node.as<ast::Block>();
    llvm::Value* last_val = nullptr;

    ctx.t_.deepenScope();

    for (auto&& stmt : block)
    {
        last_val = ast::visit<llvm::Value*>(stmt, ctx);
    }

    ctx.t_.riseScope();

    return last_val;
}

auto
visit(const ast::anyNode& node, const ast::Func& /*unused*/, GenContext& ctx)
{
    BUILDER_MODULE_TABLE_M;

    auto& func                = node.as<ast::Func>();
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

            auto&& init_val = ast::visit<llvm::Value*>(raw_val.value(), ctx);

            builder.CreateStore(init_val, alloca);
        }
    }

    auto&& block_val = ast::visit<llvm::Value*>(func.getBody(), ctx);

    builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));

    table.riseScope();
    builder.SetInsertPoint(old_label);

    return block_val;
}

auto
handleUserArgs(GenContext& ctx, const ast::FuncCall& node)
{
    BUILDER_MODULE_TABLE_M;

    std::vector<llvm::Value*> llvm_all_args;

    for (auto&& arg : node.getArgs())
    {
        auto&& struct_field = arg.as<ast::StructField>();
        auto&& raw_val      = struct_field.getValue();

        if (!raw_val.has_value())
        {
            continue;
        }

        if (raw_val.value().type().name() == typeid(ast::Var).name())
        {
            auto&& arg_it =
                table.findObj(raw_val.value().as<ast::Var>().data());

            if (!arg_it.has_value())
            {
                throw std::runtime_error(
                    std::format("Variable \"{}\" used before initialisation",
                        struct_field.getName()));
            }

            llvm_all_args.push_back(arg_it.value()->second.value_);
        }
        else if (raw_val.value().type().name() == typeid(ast::Lit<int>).name())
        {
            llvm_all_args.push_back(
                ast::visit<llvm::Value*>(raw_val.value(), ctx));
        }
        else if (raw_val.value().type().name() ==
                 typeid(ast::Lit<std::string>).name())
        {
            llvm_all_args.push_back(
                ast::visit<llvm::Value*>(raw_val.value(), ctx));
        }
    }

    return llvm_all_args;
}

llvm::FunctionCallee
setupPrintf(GenContext& ctx)
{
    BUILDER_MODULE_TABLE_M;

    llvm::PointerType* byte_ptr_ty = builder.getPtrTy();

    llvm::FunctionType* printf_ty =
        llvm::FunctionType::get(builder.getInt32Ty(), { byte_ptr_ty }, true);

    llvm::FunctionCallee printf_func =
        module.getOrInsertFunction("printf", printf_ty);

    return printf_func;
}

auto
generateFmtStrForPrintf(GenContext& ctx, const ast::FuncCall& node)
{
    BUILDER_MODULE_TABLE_M;

    std::string fmt_str;

    std::size_t arg_idx{ 0 }; // JUST FOR DEBUG

    for (auto&& arg : node.getArgs())
    {
        auto&& struct_field = arg.as<ast::StructField>();
        auto&& arg_name     = struct_field.getName();
        auto&& raw_val      = struct_field.getValue();

        if (raw_val.has_value())
        {
            if (raw_val.value().type().name() == typeid(ast::Var).name())
            {
                auto&& arg_it =
                    table.findObj(raw_val.value().as<ast::Var>().data());

                if (!arg_it.has_value())
                {
                    throw std::runtime_error(std::format(
                        "Variable \"{}\" used before initialisation",
                        arg_name));
                }

                if (arg_it.value()->second.type_info_.type() ==
                    typeid(ast::Lit<int>))
                {
                    fmt_str += "%d";
                }
                else if (arg_it.value()->second.type_info_.type() ==
                         typeid(ast::Lit<std::string>))
                {
                    fmt_str += "%s";
                }
            }
            else if (raw_val.value().type().name() ==
                     typeid(ast::Lit<int>).name())
            {
                fmt_str += "%d";
            }
            else if (raw_val.value().type().name() ==
                     typeid(ast::Lit<std::string>).name())
            {
                fmt_str += "%s";
            }
            else
            {
                throw std::runtime_error(std::format(
                    "Type of {} argument is not supported in print.", arg_idx));
            }
        }

        ++arg_idx;
    }

    return fmt_str;
}


auto
visit(
    const ast::anyNode& node, const ast::FuncCall& /*unused*/, GenContext& ctx)
{
    BUILDER_MODULE_TABLE_M;

    auto& func_call = node.as<ast::FuncCall>();
    llvm::Function* llvm_func{};
    std::vector<llvm::Value*> llvm_all_args;

    if (func_call.getName() == "print")
    {
        setupPrintf(ctx);

        llvm_func = module.getFunction("printf");

        auto&& fmt_str = generateFmtStrForPrintf(ctx, func_call);

        llvm::GlobalVariable* fmt_str_var =
            builder.CreateGlobalString(fmt_str, "printf_fmt");

        llvm_all_args.push_back(fmt_str_var);

        for (auto&& arg : func_call.getArgs())
        {
            auto&& struct_field = arg.as<ast::StructField>();
            auto&& raw_val      = struct_field.getValue();

            if (!raw_val.has_value())
            {
                continue;
            }

            if (raw_val.value().type().name() == typeid(ast::Var).name())
            {
                auto&& arg_it =
                    table.findObj(raw_val.value().as<ast::Var>().data());

                if (!arg_it.has_value())
                {
                    throw std::runtime_error(std::format(
                        "Variable \"{}\" used before initialisation",
                        struct_field.getName()));
                }

                llvm_all_args.push_back(
                    ast::visit<llvm::Value*>(raw_val.value(), ctx));
            }
            else
            {
                llvm_all_args.push_back(
                    ast::visit<llvm::Value*>(raw_val.value(), ctx));
            }
        }
    }
    else
    {
        llvm_func = module.getFunction(func_call.getName());

        if (llvm_func == nullptr)
        {
            throw std::runtime_error(
                std::format("Function not found: {}", func_call.getName()));
        }

        auto&& user_args = handleUserArgs(ctx, func_call);
        llvm_all_args.insert(llvm_all_args.end(),
            std::make_move_iterator(user_args.begin()),
            std::make_move_iterator(user_args.end()));
    }

    return builder.CreateCall(llvm_func, llvm_all_args);
}

auto
visit(const ast::anyNode& node, const ast::While& /*unused*/, GenContext& ctx)
{
    BUILDER_MODULE_TABLE_M;

    auto& whilenode = node.as<ast::While>();

    auto&& func    = builder.GetInsertBlock()->getParent();
    auto&& context = module.getContext();

    auto&& condition_label =
        llvm::BasicBlock::Create(context, "whileCond", func);
    auto&& true_label  = llvm::BasicBlock::Create(context, "whileTrue", func);
    auto&& false_label = llvm::BasicBlock::Create(context, "whileFalse", func);

    builder.CreateBr(condition_label);

    builder.SetInsertPoint(condition_label);
    auto&& condition = ast::visit<llvm::Value*>(whilenode.getClause(), ctx);
    builder.CreateCondBr(condition, true_label, false_label);

    builder.SetInsertPoint(true_label);
    auto&& val_from_if = ast::visit<llvm::Value*>(whilenode.getBody(), ctx);
    builder.CreateBr(condition_label);

    builder.SetInsertPoint(false_label);

    return nullptr;
}

auto
visit(const ast::anyNode& node, const ast::Struct& /*unused*/, GenContext& ctx)
    -> llvm::Value*
{
    auto& struc = node.as<ast::Struct>();
    auto&& name = struc.getName();

    auto&& obj_it = ctx.t_.findObj(name);

    if (!obj_it.has_value())
    {
        return nullptr;
    }

    return obj_it.value()->second.value_;
}

auto
visit(const ast::anyNode& node,
    const ast::StructEditor& /*unused*/,
    GenContext& ctx)
{
    BUILDER_MODULE_TABLE_M;

    auto& struct_editor = node.as<ast::StructEditor>();
    auto&& instance     = struct_editor.getNameOfInstance();
    auto&& changeable_field =
        struct_editor.getEditableField().as<ast::StructField>();

    auto&& obj_it = table.findObj(std::string(instance));

    if (!obj_it.has_value())
    {
        node.printError();
        throw std::runtime_error(
            std::format("Struct \"{}\" not found", std::string(instance)));
    }

    auto&& struct_entry       = obj_it.value()->second;
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
        struct_editor.getEditableField().printError();
        // throw std::runtime_error(
        //     std::format(R"(Field "{}" not found in struct "{}")",
        //         changeable_field.getName(),
        //         instance_type_info.getName()));
    }

    auto&& struct_ptr = struct_entry.value_;

    llvm::Type* struct_type = llvm::StructType::getTypeByName(
        module.getContext(), instance_type_info.getName());

    auto&& field_ptr = builder.CreateGEP(struct_type,
        struct_ptr,
        { builder.getInt32(0), builder.getInt32(field_index) });

    auto&& expression =
        ast::visit<llvm::Value*>(changeable_field.getValue().value(), ctx);

    builder.CreateStore(expression, field_ptr);

    return field_ptr;
}








// ----------------------------------------------------------------------------
// First pass: Scanning the programme for variable initialisations.
// This is necessary to ensure that all "alloc" statements appear at the
// start of functions in LLVM IR (in the "entry" block). Furthermore, thanks
// to the first pass, the types of all variables are already known by the
// time the second pass begins.
// ----------------------------------------------------------------------------










ast::anyNode
resolveType(GenContext& ctx, const ast::anyNode& expr)
{
    if (expr.type() == typeid(ast::Lit<int>) ||
        expr.type() == typeid(ast::Lit<std::string>))
    {
        return expr;
    }

    if (expr.type() == typeid(ast::BinOp))
    {
        return ast::anyNode(ast::Lit<int>(0));
    }

    if (expr.type() == typeid(ast::Var))
    {
        auto&& name = expr.as<ast::Var>().data();
        auto&& it   = ctx.t_.findObj(name);
        if (it.has_value())
        {
            return it.value()->second.type_info_;
        }
        return expr;
    }

    return expr;
}

class FirstPass
{};

template <typename NodeT>
auto
visit(const ast::anyNode& /*node*/,
    const NodeT& /*nodeT*/,
    GenContext& /*ctx*/,
    FirstPass /*unused*/)
{}

auto
visit(const ast::anyNode& node,
    const ast::Assign& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{

    BUILDER_MODULE_TABLE_M;

    auto& assign = node.as<ast::Assign>();

    if (assign.isInitialisation())
    {
        auto&& var       = assign.getLarg();
        auto&& rarg      = assign.getRarg();
        auto&& rarg_type = rarg.type();
        auto&& name      = var.as<ast::Var>().data();

        llvm::Value* alloca = nullptr;

        if (rarg_type == typeid(ast::Struct))
        {
            spdlog::get("visit")->info(
                std::format("Init var {} with struct.", name));

            auto&& struct_obj_it =
                table.findObj(std::string(rarg.as<ast::Struct>().getName()));

            if (!struct_obj_it.has_value())
            {
                rarg.printError();
            }

            auto&& struct_entry = struct_obj_it.value()->second;

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

            auto&& clear_name = clearName(str_lit_node.data());

            auto&& existing = ctx.m_.getNamedValue(clear_name);

            if (existing == nullptr)
            {
                ctx.b_.CreateGlobalString(str_lit_node.data(), clear_name);
            }

            alloca = builder.CreateAlloca(builder.getPtrTy(), nullptr, name);

            table.setObj(name, alloca, rarg);
        }
        else
        {
            auto resolved = resolveType(ctx, rarg);
            alloca = builder.CreateAlloca(builder.getInt32Ty(), nullptr, name);

            table.setObj(name, alloca, resolved);
        }
    }
}

void
visit(const ast::anyNode& node,
    const ast::Block& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    BUILDER_MODULE_TABLE_M;

    auto& block = node.as<ast::Block>();

    table.deepenScope();
    for (auto&& stmt : block)
    {
        ast::visit<void>(stmt, ctx, FirstPass{});
    }
    table.riseScope();
}

void
visit(const ast::anyNode& node,
    const ast::Func& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    BUILDER_MODULE_TABLE_M;

    auto& func = node.as<ast::Func>();

    if (table.getCurrentScope() != table.getRootScope())
    {
        throw std::logic_error("It is not possible to define functions outside "
                               "the global scope.(possibly temporarily)");
    }

    auto&& old_label = builder.GetInsertBlock();

    std::vector<llvm::Type*> arg_types(
        func.getArgs().size(), builder.getInt32Ty());

    llvm::FunctionType* func_type =
        llvm::FunctionType::get(builder.getInt32Ty(), arg_types, false);

    llvm::Function* llvm_func = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage, func.getName(), module);

    table.deepenScope();

    auto&& new_label =
        llvm::BasicBlock::Create(module.getContext(), "entry", llvm_func);

    builder.SetInsertPoint(new_label);

    spdlog::get("visit")->info(
        std::format("Starting analyzing args of function {}", func.getName()));

    for (auto&& arg : func.getArgs())
    {
        llvm::Value* alloca = builder.CreateAlloca(builder.getInt32Ty(),
            nullptr,
            arg.as<ast::StructField>().getName());
        table.setObj(arg.as<ast::StructField>().getName(), alloca);
    }

    spdlog::get("visit")->info(
        std::format("Starting analyzing body of function {}", func.getName()));

    for (auto&& stmt : func.getBody())
    {
        ast::visit<void>(stmt, ctx, FirstPass{});
    }

    table.riseScope();
    builder.SetInsertPoint(old_label);
}

void
visit(const ast::anyNode& node,
    const ast::IfElse& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    auto& ifelse = node.as<ast::IfElse>();

    ast::visit<void>(ifelse.getIf(), ctx, FirstPass{});
    ast::visit<void>(ifelse.getElse(), ctx, FirstPass{});
}

void
visit(const ast::anyNode& node,
    const ast::Struct& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    auto& struc = node.as<ast::Struct>();
    ctx.t_.setObj(std::string(struc.getName()), nullptr, node);
}

void
visit(const ast::anyNode& node,
    const ast::While& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    auto& whilenode = node.as<ast::While>();
    ast::visit<void>(whilenode.getBody(), ctx, FirstPass{});
}

void
scanForInitialisations(GenContext& ctx, const ast::anyNode& root)
{

    spdlog::get("Scanner")->info(
        std::format("Start searching initializations"));

    for (auto&& stmt : root.as<ast::Block>())
    {
        ast::visit<void>(stmt, ctx, FirstPass{});
    }
}

#undef BUILDER_MODULE_TABLE_M










export void
toLLVMIR(const ast::anyNode& root, const std::filesystem::path& filename)
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

    setupPrintf(ctx);
    scanForInitialisations(ctx, root);


    spdlog::get("general")->info(
        std::format("Initialisation scanning has been finished"));

    ctx.t_.resetNavigation();

    spdlog::get("general")->info(std::format("Start creating ir."));

    auto&& res = ast::visit<llvm::Value*>(root, GlobalBlock{}, ctx);
    builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));

    std::error_code error_code;
    llvm::raw_fd_ostream dest(
        filename.string(), error_code, llvm::sys::fs::OF_None);
    module.print(dest, nullptr);
}

} // namespace ir_generator
