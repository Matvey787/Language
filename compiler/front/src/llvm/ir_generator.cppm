module;

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
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
    
    spdlog::set_level(spdlog::level::debug);
}

std::string llvmValueToTypeStr(const llvm::Value* val)
{
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    val->getType()->print(rso);

    return type_str;

}
















using scope = std::unordered_map<const ast::AnyNode*, llvm::Value*>;
using object_iterator = scope::iterator;

class SymbolTable final : private std::vector<scope>
{
    /// @return Iterator to founded object
    object_iterator findObjInScope(const ast::AnyNode& searching_node, scope& scope_search_in)
    {
        const std::string& search_name = searching_node.as<ast::Lit<std::string>>().data();

        return std::ranges::find_if(scope_search_in, [&](const auto& pair) {
            return pair.first->template as<ast::Lit<std::string>>().data() == search_name;
        });
    }

public:
    std::optional<object_iterator> findObj(const ast::AnyNode& searching_node)
    {
        if (empty()) throw std::logic_error("Symbol table is empty.");

        spdlog::get("Symbol table")->info(
            std::format(
                "Try to found node \"{}\".",
                searching_node.as<ast::Lit<std::string>>().data()
            )
        );

        for (auto& current_scope : std::ranges::reverse_view(*this)) {
            auto it = findObjInScope(searching_node, current_scope);
            if (it != current_scope.end())
            {
                spdlog::get("Symbol table")->info(
                    std::format(
                        "Node {} has been founded.",
                        searching_node.as<ast::Lit<std::string>>().data()
                    )
                );

                return it;
            }
        }

        return std::nullopt;
    }

    void add_scope() { this->push_back({}); }

    void remove_scope() { this->pop_back(); }

    void setObj(const ast::AnyNode& node, llvm::Value* llvm_val)
    {
        if (empty()) throw std::logic_error("No scope.");
            
        auto it_opt = findObj(node);
        
        if (it_opt.has_value())
        {
            spdlog::get("Symbol table")->info(
                std::format(
                    "Value of {} now is {}",
                    node.as<ast::Lit<std::string>>().data(),
                    llvmValueToTypeStr(llvm_val)
                )
            );

            it_opt.value()->second = llvm_val;
        }
        else
        {
            spdlog::get("Symbol table")->info(
                std::format(
                    "Add new symbol {} = {}",
                    node.as<ast::Lit<std::string>>().data(),
                    llvmValueToTypeStr(llvm_val)
                )
            );

            back()[&node] = llvm_val;
        }
    }

    llvm::Value* getValue(const ast::AnyNode& node)
    {
        auto&& it = findObj(node);

        if (it.has_value()) return it.value()->second;
        
        throw std::runtime_error("Symbol not found.");
    }

    using std::vector<scope>::begin;
    using std::vector<scope>::end;
    using std::vector<scope>::rbegin;
    using std::vector<scope>::rend;

    using std::vector<scope>::empty;
    using std::vector<scope>::size;
    using std::vector<scope>::vector;
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
    auto&& var = node.getLarg();
    auto&& expr = node.getRarg();
    auto&& varName = var.as<ast::Lit<std::string>>().data();

    spdlog::get("visit")->info(
        std::format(
                "Assign {}",
                varName
        )
    );

    llvm::Value* alloca = ctx.b_.CreateAlloca(ctx.b_.getInt32Ty(), nullptr, varName);

    ctx.table_.setObj(var, alloca);

    llvm::Value* initVal = ast::visit<llvm::Value*>(ctx, expr);

    ctx.b_.CreateStore(initVal, alloca);

    return initVal;
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
    default: { throw std::logic_error("Unknown binary operation."); break; }
    }
    
    return operation;
}

llvm::Value* visit(GenContext& ctx, const ast::Block& block)
{
    llvm::Value* lastVal = nullptr;

    for (auto&& stmt : block) lastVal = visit<llvm::Value*>(ctx, stmt);

    return lastVal;
}

export void to_llvmir(ast::AnyNode root, std::string_view filename) {
    llvm::LLVMContext context;
    llvm::Module module("main", context);
    module.setTargetTriple(llvm::Triple(llvm::sys::getDefaultTargetTriple()));
    llvm::IRBuilder<> builder(context);

    auto funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module);
    
    auto entry = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(entry);

    // Generate context info during all recursion 
    // Create table SymbolTable{1} with the first (main) scope
    GenContext ctx{builder, module, SymbolTable{1}};

    spdlog_init();

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
