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
#include <string_view>

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













struct VarData {
    llvm::Value* value;
    ast::AnyNode type_info;

    bool operator==(const VarData& other) const {
        return value == other.value;
    }
};

using scope = std::unordered_map<std::string, VarData>;
using object_iterator = scope::iterator;

class SymbolTable final
{
    struct ScopeData
    {
        scope symbols;
        std::weak_ptr<ScopeData> parent;
        std::vector<std::shared_ptr<ScopeData>> children;
        size_t next_child = 0;

        bool operator==(const ScopeData& other) const {
            if (symbols != other.symbols) return false;

            if (children.size() != other.children.size()) return false;

            return std::ranges::equal(
                children, 
                other.children, 
                [](const auto& a, const auto& b) { return *a == *b; }
            );
        }

        bool operator!=(const ScopeData& other) const {
            return !(*this == other);
        }
    };

    std::shared_ptr<ScopeData> global_scope_;
    std::shared_ptr<ScopeData> current_;

    size_t depth_{0};

public:

    std::optional<object_iterator> findObj(const std::string& name)
    {
        if (!current_)
            throw std::logic_error("No current scope.");
        
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

    void
    setObj(
        const std::string& name, 
        llvm::Value* llvm_val, 
        ast::AnyNode type_info = ast::AnyNode(ast::Lit<std::string>(std::string{}))
    )
    {
        if (!current_)
            throw std::logic_error("No active scope.");

        spdlog::get("Symbol table")->info(
            std::format("Add new symbol {} = {} [depth {}]", 
                name,
                llvm_val ? llvmValueToTypeStr(llvm_val) : "nullptr",
                depth_
            )
        );

        current_->symbols[name] = VarData{llvm_val, std::move(type_info)};
    }

    void updateObj(const std::string& name, llvm::Value* llvm_val)
    {
        auto it_opt = findObj(name);

        spdlog::get("Symbol table")->info(
            std::format("Update {} = {}", 
                name,
                llvmValueToTypeStr(llvm_val))
        );

        if (!it_opt.has_value())
            throw std::runtime_error(
                std::format("Variable \"{}\" used before initialisation",
                    name
                )
            );


        it_opt.value()->second.value = llvm_val;
    }

    llvm::Value* getValue(const std::string& name)
    {
        auto it_opt = findObj(name);
        if (it_opt.has_value())
            return it_opt.value()->second.value;

        throw std::runtime_error("Symbol not found: " + name);
    }

    size_t getDepth() const noexcept { return depth_; }

    const ScopeData& getCurrentScope() const noexcept { return *current_; }
    const ScopeData& getRootScope() const noexcept { return *global_scope_; }

    void resetNavigation()
    {
        std::function<void(std::shared_ptr<ScopeData>&)> reset = [&](auto& scope) {
            scope->next_child = 0;
            for (auto& child : scope->children)
                reset(child);
        };
        reset(global_scope_);
        current_ = global_scope_;
        depth_ = 0;
    }

    SymbolTable()
    {
        global_scope_ = std::make_shared<ScopeData>();
        current_ = global_scope_;
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
    llvm::Value* varPtr = ctx.table_.getValue(node.data());
    return ctx.b_.CreateLoad(ctx.b_.getInt32Ty(), varPtr, node.data());
}

llvm::Value* visit(GenContext& ctx, const ast::Assign& node)
{
    auto&& var     = node.getLarg();
    auto&& expr    = node.getRarg();
    auto&& varName = var.as<ast::Lit<std::string>>().data();

    llvm::Value* initVal = ast::visit<llvm::Value*>(ctx, expr);

    llvm::Value* alloca = nullptr;

    auto it = ctx.table_.findObj(varName);

    if (!it.has_value())
        throw std::runtime_error(
            std::format("Variable \"{}\" used before initialisation", varName)
        );
    alloca = it.value()->second.value;

    assert(alloca);
    
    if (initVal)
    {
        ctx.b_.CreateStore(initVal, alloca);
    }
    
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


// FIXME -----------------------------!!!FOR A WHILE!!!-----------------------------
// FIXME This function may be removed in the near future. For the first block, 
// FIXME there is no need to go into the table due to the default table constructor, as 
// FIXME we are already in the global scope. The structure "global_block"
// FIXME is formally designed to search for a specific signature of the `visit` function.

struct global_block {};

llvm::Value* visit(GenContext& ctx, const ast::Block& block, global_block)
{
    llvm::Value* lastVal = nullptr;

    for (auto&& stmt : block)
    {
        lastVal = ast::visit<llvm::Value*>(ctx, stmt);
    }

    return lastVal;
}






llvm::Value* visit(GenContext& ctx, const ast::Block& block)
{
    llvm::Value* lastVal = nullptr;

    ctx.table_.deepen_scope();

    for (auto&& stmt : block)
    {
        lastVal = ast::visit<llvm::Value*>(ctx, stmt);
    }

    ctx.table_.rise_scope();

    return lastVal;
}

llvm::Value* visit(GenContext& ctx, const ast::Func& func)
{
    llvm::Function* llvm_func = ctx.m_.getFunction(func.getName());

    auto&& oldLabel = ctx.b_.GetInsertBlock();

    llvm::BasicBlock& entry = llvm_func->getEntryBlock();
    ctx.b_.SetInsertPoint(&entry);

    ctx.table_.deepen_scope();

    for (auto&& arg : func.getArgs())
    {
        auto&& structField = arg.as<ast::StructField>();

        auto&& argName = structField.getName();
        auto&& raw_val = structField.getValue();

        if (raw_val.has_value())
        {
            auto&& arg_it = ctx.table_.findObj(argName);

            if (!arg_it.has_value())
                throw std::runtime_error(
                    std::format("Variable \"{}\" used before initialisation", argName)
                );
            llvm::Value* alloca = arg_it.value()->second.value;
            
            auto&& initVal = ast::visit<llvm::Value*>(ctx, raw_val.value());

            ctx.b_.CreateStore(initVal, alloca);
        }
    }

    auto&& blockVal = ast::visit<llvm::Value*>(ctx, func.getBody());

    ctx.b_.CreateRet(llvm::ConstantInt::get(ctx.b_.getInt32Ty(), 0));

    ctx.table_.rise_scope();
    ctx.b_.SetInsertPoint(oldLabel);

    return blockVal;
}

llvm::Value* visit(GenContext& ctx, const ast::FuncCall& node)
{
    llvm::Function* llvm_func = ctx.m_.getFunction(node.getName());

    if (!llvm_func)
    {
        throw std::runtime_error(
            std::format(
                "Function not found: {}",
                node.getName()
            )
        );
    }

    std::vector<llvm::Value*> llvm_all_args;

    for (auto&& arg : node.getArgs())
    {
        auto&& structField = arg.as<ast::StructField>();
        auto&& argName = structField.getName();
        
        auto&& raw_val = structField.getValue();

        if (raw_val.has_value())
        {
            auto&& arg_it = ctx.table_.findObj(raw_val.value().as<ast::Lit<std::string>>().data());

            if (!arg_it.has_value())
                throw std::runtime_error(
                    std::format("Variable \"{}\" used before initialisation", argName)
                );
            llvm::Value* alloca = arg_it.value()->second.value;


            llvm_all_args.push_back(alloca);
        }
    }

    llvm::Value* call = ctx.b_.CreateCall(llvm_func, llvm_all_args);

    return call;
}

llvm::Value* visit(GenContext& ctx, const ast::Struct& node)
{

    // TODO It needs finishing; the structure declaration doesn’t mean anything yet ????????????
    return nullptr;
}

llvm::Value* visit(GenContext& ctx, const ast::StructEditor& node)
{
    auto&& instance = node.getNameOfInstance();
    auto&& changeable_field = node.getEditableField();

    auto&& structEntry = ctx.table_.findObj(std::string(instance)).value()->second;
    auto&& instance_type_info = structEntry.type_info.as<ast::Struct>();

    uint32_t fieldIndex = 0;
    bool found = false;
    uint32_t idx = 0;

    for (auto&& field : instance_type_info)
    {
        auto&& sf = field.as<ast::StructField>();
        if (sf.getName() == changeable_field.getName())
        {
            fieldIndex = idx;
            found = true;
            break;
        }
        ++idx;
    }

    if (!found)
        throw std::runtime_error(
            std::format("Field \"{}\" not found in struct \"{}\"",
                changeable_field.getName(), instance_type_info.getName())
        );

    llvm::Value* structPtr = structEntry.value;
    llvm::Type* structType = llvm::StructType::getTypeByName(
        ctx.m_.getContext(), instance_type_info.getName());

    llvm::Value* fieldPtr = ctx.b_.CreateGEP(
        structType,
        structPtr,
        {ctx.b_.getInt32(0), ctx.b_.getInt32(fieldIndex)}
    );

    llvm::Value* expression = ast::visit<llvm::Value*>(ctx, changeable_field.getValue().value());

    ctx.b_.CreateStore(expression, fieldPtr);

    return fieldPtr;
}



void
initialisations_block_scanning(GenContext& ctx, const ast::AnyNode& node)
{
    auto& t = ctx.table_;
    auto& b = ctx.b_;

    if (node.type() == typeid(ast::Assign))
    {
        auto&& assign = node.as<ast::Assign>();
        if (assign.isInitialisation())
        {
            auto&& var = assign.getLarg();
            auto&& rarg = assign.getRarg();
            auto&& rargType = rarg.type();
            auto&& name = var.as<ast::Lit<std::string>>().data();

            llvm::Value* alloca;

            if (rargType == typeid(ast::Struct))
            {
                spdlog::get("visit")->info(
                    std::format("Init var {} with struct.", 
                        name
                    )
                );

                auto&& structEntry = ctx.table_.findObj(std::string(rarg.as<ast::Struct>().getName())).value()->second;
                auto&& structNode = structEntry.type_info.as<ast::Struct>();

                llvm::StructType* structType = llvm::StructType::create(
                    ctx.m_.getContext(), 
                    std::vector<llvm::Type*>(structNode.size(), ctx.b_.getInt32Ty()), 
                    structNode.getName()
                );

                alloca = b.CreateAlloca(structType);

                t.setObj(name, alloca, structNode);
            }
            else
            {
                alloca = b.CreateAlloca(b.getInt32Ty(), nullptr, name);

                t.setObj(name, alloca);
            }

        }
    }
    else if (node.type() == typeid(ast::Block))
    {
        t.deepen_scope();
        for (auto& stmt : node.as<ast::Block>())
        {
            initialisations_block_scanning(ctx, stmt);
        }
        t.rise_scope();
    }
    else if (node.type() == typeid(ast::Func))
    {
        if (ctx.table_.getCurrentScope() != ctx.table_.getRootScope())
            throw std::logic_error("It is not possible to define functions outside the global scope.(possibly temporarily)");

        auto&& func = node.as<ast::Func>();

        std::vector<llvm::Type*> argTypes(func.getArgs().size(), ctx.b_.getInt32Ty());
        llvm::FunctionType* funcType = llvm::FunctionType::get(ctx.b_.getInt32Ty(), argTypes, false);
        
        llvm::Function* llvm_func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, func.getName(), ctx.m_);

        auto&& argIt = llvm_func->arg_begin();

        t.deepen_scope();

        auto&& oldLabel = ctx.b_.GetInsertBlock();

        auto&& funcLabel = llvm::BasicBlock::Create(ctx.m_.getContext(), "entry", llvm_func);

        ctx.b_.SetInsertPoint(funcLabel);

        spdlog::get("visit")->info(
            std::format("Starting analyzing args of function {}", 
                func.getName()
            )
        );

        for (auto&& arg : func.getArgs())
        {
            llvm::Value* alloca = b.CreateAlloca(b.getInt32Ty(), nullptr, arg.as<ast::StructField>().getName());
            t.setObj(arg.as<ast::StructField>().getName(), alloca);
        }

        spdlog::get("visit")->info(
            std::format("Starting analyzing body of function {}", 
                func.getName()
            )
        );

        for (auto&& stmt : func.getBody())
        {
            initialisations_block_scanning(ctx, stmt);
        }

        t.rise_scope();
        ctx.b_.SetInsertPoint(oldLabel);
    }
    else if (node.type() == typeid(ast::IfElse)) {
        auto& ifelse = node.as<ast::IfElse>();
        
        initialisations_block_scanning(ctx, ifelse.getIf());
        initialisations_block_scanning(ctx, ifelse.getElse());
    }
    else if (node.type() == typeid(ast::Struct))
    {
        auto&& structNode = node.as<ast::Struct>();

        ctx.table_.setObj(std::string(structNode.getName()), nullptr, structNode);

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


    // FIXME -----------------------------!!!FOR A WHILE!!!-----------------------------
    // FIXME This function may be removed in the near future.(problem of global scope)
    for (auto& stmt : root.as<ast::Block>())
    {
        initialisations_block_scanning(ctx, stmt);
    }
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

    llvm::Value* res = ast::visit<llvm::Value*>(ctx, root, global_block{});
    builder.CreateRet(llvm::ConstantInt::get(ctx.b_.getInt32Ty(), 0));

    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    module.print(dest, nullptr);
}

} // namespace ir_generator
