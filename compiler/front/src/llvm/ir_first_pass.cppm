module;

#include <algorithm>
#include <cstddef>
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
#include <unordered_set>

#include "ir_ctx_unpack.hpp"
#include "spdlog/spdlog.h"

export module ir_first_pass;

import ast;
import ir_generate_context;
import ir_symbol_table;
import ir_debug;

namespace ir_generator
{

ast::anyNode
resolveType(GenContext& ctx, const ast::anyNode& expr)
{
    if (expr.is<ast::Lit<int>>() || expr.is<ast::Lit<std::string>>())
    {
        return expr;
    }

    if (expr.is<ast::BinOp>())
    {
        return ast::anyNode(ast::Lit<int>(0));
    }

    if (expr.is<ast::Var>())
    {
        const auto& name = expr.as<ast::Var>().data();
        auto&& it        = ctx.t_.findObj(name);
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

export template <typename NodeT>
auto
visit(const ast::anyNode& /*node*/,
    const NodeT& /*nodeT*/,
    GenContext& /*ctx*/,
    FirstPass /*unused*/)
{}

decltype(auto)
handleRargAsStruct(
    GenContext& ctx, std::string_view var_name, const ast::anyNode& rarg)
{
    UNPACK_CTX_M(ctx)

    spdlog::get("visit")->info(
        std::format("Init var {} with struct.", var_name));

    auto&& struct_obj_it =
        table.findObj(std::string(rarg.as<ast::Struct>().getName()));



    if (!struct_obj_it.has_value())
    {
        rarg.setErrorMsg(std::format("use of undeclared struct '{}'",
            std::string(rarg.as<ast::Struct>().getName())));
        rarg.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }



    auto&& struct_entry = struct_obj_it.value()->second;
    auto&& struct_node  = struct_entry.type_info_.as<ast::Struct>();

    std::vector<llvm::Type*> field_types;
    for (auto&& def_field : struct_node)
    {
        auto&& field = def_field.as<ast::StructField>();
        if (field.getValue().has_value() &&
            field.getValue().value().is<ast::Lit<std::string>>())
        {
            field_types.push_back(builder.getPtrTy());
        }
        else
        {
            field_types.push_back(builder.getInt32Ty());
        }
    }

    auto&& struct_type = llvm::StructType::create(
        module.getContext(), field_types, struct_node.getName());

    auto&& alloca = builder.CreateAlloca(struct_type);

    for (auto&& init_field : rarg.as<ast::Struct>())
    {
        auto&& field = init_field.as<ast::StructField>();

        if (field.getValue().has_value() &&
            field.getValue().value().is<ast::Lit<std::string>>())
        {
            auto&& str_lit =
                field.getValue().value().as<ast::Lit<std::string>>();
            auto&& clear_name = clearName(str_lit.data());

            if (ctx.m_.getNamedValue(clear_name) == nullptr)
            {
                ctx.b_.CreateGlobalString(str_lit.data(), clear_name);
            }
        }
    }

    table.setObj(std::string(var_name), alloca, struct_node);
}

decltype(auto)
handleRargAsString(
    GenContext& ctx, std::string_view var_name, const ast::anyNode& rarg)
{
    UNPACK_CTX_M(ctx)

    auto&& str_lit_node = rarg.as<ast::Lit<std::string>>();

    auto&& clear_name = clearName(str_lit_node.data());

    auto&& existing = ctx.m_.getNamedValue(clear_name);

    if (existing == nullptr)
    {
        ctx.b_.CreateGlobalString(str_lit_node.data(), clear_name);
    }

    llvm::Value* alloca =
        builder.CreateAlloca(builder.getPtrTy(), nullptr, var_name);

    table.setObj(std::string(var_name), alloca, rarg);
}

export decltype(auto)
visit(const ast::anyNode& node,
    const ast::Assign& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    UNPACK_CTX_M(ctx)

    const auto& assign = node.as<ast::Assign>();

    if (assign.isInitialisation())
    {
        const auto& var      = assign.getLarg();
        const auto& var_name = var.as<ast::Var>().data();
        const auto& rarg     = assign.getRarg();



        // Check for variable redefinition in current scope

        auto existing = table.findObjInCurrentScope(var_name);
        if (existing.has_value())
        {
            node.setErrorMsg(std::format("redefinition of '{}'", var_name));
            node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
        }



        if (rarg.is<ast::Struct>())
        {
            handleRargAsStruct(ctx, var_name, rarg);
        }
        else if (rarg.is<ast::Lit<std::string>>())
        {
            handleRargAsString(ctx, var_name, rarg);
        }
        else
        {
            auto resolved = resolveType(ctx, rarg);

            llvm::Value* alloca =
                builder.CreateAlloca(builder.getInt32Ty(), nullptr, var_name);

            table.setObj(var_name, alloca, resolved);
        }
    }
}

export void
visit(const ast::anyNode& node,
    const ast::Block& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    UNPACK_CTX_M(ctx)

    const auto& block = node.as<ast::Block>();

    table.deepenScope();
    for (auto&& stmt : block)
    {
        ast::visit<void>(stmt, ctx, FirstPass{});
    }
    table.riseScope();
}

export void
visit(const ast::anyNode& node,
    const ast::Func& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    UNPACK_CTX_M(ctx)

    const auto& func = node.as<ast::Func>();

    if (table.getCurrentScope() != table.getRootScope())
    {
        node.setErrorMsg(
            "function definition is not allowed in non-global scope");
        node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }

    // Check for function redefinition at global scope

    auto&& existing_func = module.getFunction(func.getName());
    if (existing_func != nullptr)
    {
        node.setErrorMsg(
            std::format("redefinition of function '{}'", func.getName()));
        node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }

    table.setObj(std::string(func.getName()), nullptr, node);

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
        auto&& field        = arg.as<ast::StructField>();
        llvm::Value* alloca = builder.CreateAlloca(
            builder.getInt32Ty(), nullptr, field.getName());
        table.setObj(field.getName(), alloca);
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

export void
visit(const ast::anyNode& node,
    const ast::IfElse& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{

    const auto& ifelse = node.as<ast::IfElse>();

    ast::visit<void>(ifelse.getIf(), ctx, FirstPass{});
    ast::visit<void>(ifelse.getElse(), ctx, FirstPass{});
}

export void
visit(const ast::anyNode& node,
    const ast::Struct& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{

    const auto& struc = node.as<ast::Struct>();

    auto existing = ctx.t_.findObj(std::string(struc.getName()));
    if (existing.has_value())
    {
        node.setErrorMsg(std::format(
            "redefinition of struct '{}'", std::string(struc.getName())));
        node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }

    // Check for field redefinitions within the struct
    std::unordered_set<std::string> seen_fields;
    for (auto&& field : struc)
    {
        const auto& sf         = field.as<ast::StructField>();
        const auto& field_name = sf.getName();

        if (field_name != "arg" && seen_fields.contains(field_name))
        {
            field.setErrorMsg(
                std::format("redefinition of field '{}' in struct '{}'",
                    field_name,
                    std::string(struc.getName())));
            field.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
        }

        seen_fields.insert(field_name);

        if (sf.getValue().has_value() &&
            sf.getValue().value().is<ast::Lit<std::string>>())
        {
            auto&& str_lit = sf.getValue().value().as<ast::Lit<std::string>>();
            auto&& clear_name = clearName(str_lit.data());
            if (ctx.m_.getNamedValue(clear_name) == nullptr)
            {
                ctx.b_.CreateGlobalString(str_lit.data(), clear_name);
            }
        }
    }

    ctx.t_.setObj(std::string(struc.getName()), nullptr, node);
}

export void
visit(const ast::anyNode& node,
    const ast::While& /*nodeT*/,
    GenContext& ctx,
    FirstPass /*unused*/)
{
    const auto& whilenode = node.as<ast::While>();
    ast::visit<void>(whilenode.getBody(), ctx, FirstPass{});
}

export void
scanForInitialisations(GenContext& ctx, const ast::anyNode& root)
{

    spdlog::get("Scanner")->info(
        std::format("Start searching initializations"));

    for (auto&& stmt : root.as<ast::Block>())
    {
        ast::visit<void>(stmt, ctx, FirstPass{});
    }
}

} // namespace ir_generator
