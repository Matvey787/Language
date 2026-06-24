module;

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h" 
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>

#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <unordered_map>
#include <algorithm>
#include <ranges>
#include <iostream>
#include <functional>

export module ir_generator;

import ast;

namespace ir_generator {



// JUST FOR DEBUG

void spdlog_init()
{
    spdlog::set_pattern("[%H:%M:%S.%e]\t[%n]: [%^%l%$] %v");
    
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "debug.log", 
        true
    );
    
    auto make_logger = [&](const std::string& name) {
        auto logger = std::make_shared<spdlog::logger>(name, file_sink);
        spdlog::register_logger(logger);
    };
    
    make_logger("Symbol table");
    make_logger("visit");
    make_logger("general");

    spdlog::flush_on(spdlog::level::trace);
    
    spdlog::set_level(spdlog::level::debug);
}

std::string llvmValueToTypeStr(const llvm::Value* val)
{
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    val->getType()->print(rso);

    return type_str;

}













using scope = std::unordered_map<std::string, llvm::Value*>;
using object_iterator = scope::iterator;

class SymbolTable final
{
    struct ScopeData
    {
        scope symbols;
        std::weak_ptr<ScopeData> parent;
        std::vector<std::shared_ptr<ScopeData>> children;
        size_t next_child = 0;
    };

    std::shared_ptr<ScopeData> root_;
    std::shared_ptr<ScopeData> current_;

    size_t depth_{0};

public:
    std::optional<object_iterator> findObj(const ast::AnyNode& searching_node)
    {
        if (!current_)
            throw std::logic_error("No current scope.");

        const std::string& name = searching_node.as<ast::Lit<std::string>>().data();
        
        spdlog::get("Symbol table")->info(
            std::format("Try to found node \"{}\" at depth {}.", name, depth_)
        );

        for (auto s = current_; s != nullptr; s = s->parent.lock())
        {
            auto it = s->symbols.find(name);
            if (it != s->symbols.end())
                return it;
        }

        return std::nullopt;
    }

    void deepen_scope()
    {
        if (current_->next_child < current_->children.size())
        {
            current_ = current_->children[current_->next_child];
        }
        else
        {
            auto new_scope = std::make_shared<ScopeData>();
            new_scope->parent = current_;
            current_->children.push_back(new_scope);
            current_ = new_scope;
        }
        current_->next_child = 0;
        ++depth_;
    }

    void rise_scope()
    {
        if (depth_ > 0 && current_)
        {
            current_ = current_->parent.lock();
            ++current_->next_child;
            --depth_;
        }
    }

    void setObj(const ast::AnyNode& node, llvm::Value* llvm_val)
    {
        if (!current_)
            throw std::logic_error("No active scope.");

        const std::string& name = node.as<ast::Lit<std::string>>().data();

        spdlog::get("Symbol table")->info(
            std::format("Add new symbol {} = {}", 
                name,
                llvmValueToTypeStr(llvm_val))
        );

        current_->symbols[name] = llvm_val;
    }

    void updateObj(const ast::AnyNode& node, llvm::Value* llvm_val)
    {
        auto it_opt = findObj(node);

        if (!it_opt.has_value())
            throw std::runtime_error(
                std::format("Variable \"{}\" used before initialisation",
                    node.as<ast::Lit<std::string>>().data())
            );

        spdlog::get("Symbol table")->info(
            std::format("Update {} = {}", 
                node.as<ast::Lit<std::string>>().data(),
                llvmValueToTypeStr(llvm_val))
        );

        it_opt.value()->second = llvm_val;
    }

    llvm::Value* getValue(const ast::AnyNode& node)
    {
        auto it_opt = findObj(node);
        if (it_opt.has_value())
            return it_opt.value()->second;

        throw std::runtime_error("Symbol not found: " + 
            node.as<ast::Lit<std::string>>().data());
    }

    size_t getDepth() const noexcept { return depth_; }

    const ScopeData& getCurrentScope() const noexcept { return *current_; }
    const ScopeData& getRootScope() const noexcept { return *root_; }

    void resetNavigation()
    {
        std::function<void(std::shared_ptr<ScopeData>&)> reset = [&](auto& scope) {
            scope->next_child = 0;
            for (auto& child : scope->children)
                reset(child);
        };
        reset(root_);
        current_ = root_;
        depth_ = 0;
    }

    SymbolTable()
    {
        root_ = std::make_shared<ScopeData>();
        current_ = root_;
    }

    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;
};

class GenContext final
{
public:
    llvm::IRBuilder<>& b_;
    llvm::Module& m_;
    SymbolTable table_;
};

llvm::Value* visit(GenContext& ctx, const ast::Lit<int>& node)
{
    return llvm::ConstantInt::get(ctx.b_.getInt32Ty(), node.data());
}

llvm::Value* visit(GenContext& ctx, const ast::Lit<std::string>& node)
{
    llvm::Value* varPtr = ctx.table_.getValue(node);
    return ctx.b_.CreateLoad(ctx.b_.getInt32Ty(), varPtr, node.data());
}

llvm::Value* visit(GenContext& ctx, const ast::Assign& node)
{
    auto&& var     = node.getLarg();
    auto&& expr    = node.getRarg();
    auto&& varName = var.as<ast::Lit<std::string>>().data();

    llvm::Value* initVal = ast::visit<llvm::Value*>(ctx, expr);

    llvm::Value* alloca = nullptr;

    if (node.isInitialisation())
    {
        auto it = ctx.table_.findObj(var);
        if (!it.has_value())
            throw std::runtime_error(
                std::format("Variable \"{}\" not found after scanning", varName)
            );
        alloca = it.value()->second;
    }
    else
    {
        auto it = ctx.table_.findObj(var);
        if (!it.has_value())
            throw std::runtime_error(
                std::format("Variable \"{}\" used before initialisation", varName)
            );
        alloca = it.value()->second;
    }

    ctx.b_.CreateStore(initVal, alloca);
    return alloca;
}


llvm::Value* visit(GenContext& ctx, const ast::BinOp& node)
{
    llvm::Value* left  = ast::visit<llvm::Value*>(ctx, node.getLarg());
    llvm::Value* right = ast::visit<llvm::Value*>(ctx, node.getRarg());

    using opEnum = ast::BinOp::binOpType;

    llvm::Value* operation = nullptr;

    switch (node.getOp())
    {
    case opEnum::Add: { operation = ctx.b_.CreateAdd(left, right, "add"); break; }
    case opEnum::Sub: { operation = ctx.b_.CreateSub(left, right, "sub"); break; }
    case opEnum::Mul: { operation = ctx.b_.CreateMul(left, right, "mul"); break; }
    case opEnum::Div: { operation = ctx.b_.CreateFDiv(left, right, "div"); break; }

    case opEnum::A: { operation = ctx.b_.CreateICmpSGT(left, right, "a"); break; }
    case opEnum::AE: { operation = ctx.b_.CreateICmpSGE(left, right, "ae"); break; }
    case opEnum::L: { operation = ctx.b_.CreateICmpSLT(left, right, "l"); break; }
    case opEnum::LE: { operation = ctx.b_.CreateICmpSLE(left, right, "le"); break; }
    case opEnum::E: { operation = ctx.b_.CreateICmpEQ(left, right, "eq"); break; }
    case opEnum::NE: { operation = ctx.b_.CreateICmpNE(left, right, "ne"); break; }

    default: { throw std::logic_error("Unknown binary operation."); break; }
    }
    
    return operation;
}

llvm::Value* visit(GenContext& ctx, const ast::IfElse& node)
{
    auto* func = ctx.b_.GetInsertBlock()->getParent();
    auto& context = ctx.m_.getContext();

    llvm::Value* clause = ast::visit<llvm::Value*>(ctx, node.getClause());

    auto* if_label = llvm::BasicBlock::Create(context, "if", func);
    auto* else_label = llvm::BasicBlock::Create(context, "else", func);
    auto* merge_label = llvm::BasicBlock::Create(context, "endif", func);

    ctx.b_.CreateCondBr(clause, if_label, else_label);

    ctx.b_.SetInsertPoint(if_label);
    llvm::Value* val_from_if = ast::visit<llvm::Value*>(ctx, node.getIf());
    ctx.b_.CreateBr(merge_label);

    ctx.b_.SetInsertPoint(else_label);
    llvm::Value* val_from_else = ast::visit<llvm::Value*>(ctx, node.getElse());
    ctx.b_.CreateBr(merge_label);

    ctx.b_.SetInsertPoint(merge_label);
    auto* phi = ctx.b_.CreatePHI(val_from_if->getType(), 2);
    phi->addIncoming(val_from_if, if_label);
    phi->addIncoming(val_from_else, else_label);

    return phi;
}

llvm::Value* visit(GenContext& ctx, const ast::Block& block)
{
    llvm::Value* lastVal = nullptr;

    ctx.table_.deepen_scope();

    for (auto&& stmt : block)
    {
        lastVal = visit<llvm::Value*>(ctx, stmt);
    }

    ctx.table_.rise_scope();

    return lastVal;
}

void initialisations_block_scanning(GenContext& ctx, const ast::AnyNode& node)
{
    auto& t = ctx.table_;
    auto& b = ctx.b_;

    if (node.type() == typeid(ast::Block))
    {
        t.deepen_scope();
        for (auto& stmt : node.as<ast::Block>())
        {
            if (stmt.type() == typeid(ast::Assign))
            {
                auto& assign = stmt.as<ast::Assign>();
                if (assign.isInitialisation())
                {
                    auto& var = assign.getLarg();
                    auto name = var.as<ast::Lit<std::string>>().data();

                    llvm::Value* alloca = b.CreateAlloca(b.getInt32Ty(), nullptr, name);
                    t.setObj(var, alloca);
                }
            } 
            else {
                initialisations_block_scanning(ctx, stmt);
            }
        }
        t.rise_scope();
    }

    else if (node.type() == typeid(ast::IfElse)) {
        auto& ifelse = node.as<ast::IfElse>();
        
        initialisations_block_scanning(ctx, ifelse.getIf());
        initialisations_block_scanning(ctx, ifelse.getElse());
    }
}

void
initialisations_scanning(GenContext& ctx, const ast::AnyNode& root)
{
    spdlog::get("general")->info(
        std::format(
            "Start searching initializations"
        )
    );

    initialisations_block_scanning(ctx, root);
}

export void to_llvmir(const ast::AnyNode& root, std::string_view filename) {

    spdlog_init();

    spdlog::get("general")->info(
        std::format(
            "Starting LLVM IR generation"
        )
    );

    llvm::LLVMContext context;
    llvm::Module module("main", context);
    module.setTargetTriple(llvm::Triple(llvm::sys::getDefaultTargetTriple()));
    llvm::IRBuilder<> builder(context);

    auto funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module);
    
    auto entry = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entry);

    // Generate context info during all recursion

    GenContext ctx{builder, module}; 
    initialisations_scanning(ctx, root);

    ctx.table_.resetNavigation();

    spdlog::get("general")->info(
        std::format(
                "Start creating ir."
        )
    );

    llvm::Value* res = ast::visit<llvm::Value*>(ctx, root);
    builder.CreateRet(llvm::ConstantInt::get(ctx.b_.getInt32Ty(), 0));

    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    module.print(dest, nullptr);
}

} // namespace ir_generator
