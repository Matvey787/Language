module;

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
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

#include "ir_ctx_unpack.hpp"
#include "spdlog/spdlog.h"

export module ir_second_pass;

import ast;
import ir_generate_context;
import ir_symbol_table;
import ir_print;
import ir_first_pass;
import ir_debug;

namespace ir_generator
{

export auto
visit(
    const ast::anyNode& node, const ast::Lit<int>& /*unused*/, GenContext& ctx)
{
    auto& lit = node.as<ast::Lit<int>>();
    return llvm::ConstantInt::get(ctx.b_.getInt32Ty(), lit.data());
}

export auto
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

export auto
visit(const ast::anyNode& node, const ast::Var& /*unused*/, GenContext& ctx)
{
    auto& var   = node.as<ast::Var>();
    auto obj_it = ctx.t_.findObj(var.data());

    if (!obj_it.has_value())
    {
        node.setErrorMsg(
            std::format("use of undeclared identifier '{}'", var.data()));
        node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }

    llvm::Value* var_ptr = obj_it.value()->second.value_;

    if (ctx.t_.findObj(var.data()).value()->second.type_info_.type() ==
        typeid(ast::Lit<std::string>))
    {
        return ctx.b_.CreateLoad(ctx.b_.getPtrTy(), var_ptr, var.data());
    }

    return ctx.b_.CreateLoad(ctx.b_.getInt32Ty(), var_ptr, var.data());
}

export auto
visit(const ast::anyNode& node, const ast::Assign& /*unused*/, GenContext& ctx)
{
    UNPACK_CTX(ctx)

    auto& assign    = node.as<ast::Assign>();
    auto&& var      = assign.getLarg();
    auto&& expr     = assign.getRarg();
    auto&& var_name = var.as<ast::Var>().data();

    llvm::Value* alloca = nullptr;

    auto obj_it = ctx.t_.findObj(var_name);

    if (!obj_it.has_value())
    {
        node.setErrorMsg(
            std::format("use of undeclared identifier '{}'", var_name));
        node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }

    alloca = obj_it.value()->second.value_;

    assert(alloca);

    if (expr.type() == typeid(ast::Struct))
    {
        auto&& init_struct = expr.as<ast::Struct>();
        auto&& struct_name = init_struct.getName();

        auto&& struct_def_it = ctx.t_.findObj(struct_name);
        if (!struct_def_it.has_value())
        {
            expr.setErrorMsg(
                std::format("use of undeclared struct '{}'", struct_name));
            expr.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
        }

        auto&& struct_def =
            struct_def_it.value()->second.type_info_.as<ast::Struct>();

        auto* struct_type =
            llvm::StructType::getTypeByName(module.getContext(), struct_name);

        if (init_struct.size() < struct_def.size())
        {
            auto&& skipped = struct_def.size() - init_struct.size();
            expr.setNoteMsg(std::format(
                "remaining {} field(s) initialised with default value(s)",
                skipped));
            expr.print(ast::ErrorHandlerExt<ast::anyNode>::Type::NOTE);

            struct_def_it.value()->second.type_info_.setNoteMsg(
                "struct definition is here");
            struct_def_it.value()->second.type_info_.print(
                ast::ErrorHandlerExt<ast::anyNode>::Type::NOTE);
        }

        std::size_t field_idx = 0;
        for (auto&& init_field_node : init_struct)
        {
            auto&& init_field = init_field_node.as<ast::StructField>();

            auto&& field_ptr = builder.CreateGEP(struct_type,
                alloca,
                { builder.getInt32(0), builder.getInt32(field_idx) });

            if (init_field.getValue().has_value())
            {
                auto&& val = ast::visit<llvm::Value*>(
                    init_field.getValue().value(), ctx);
                builder.CreateStore(val, field_ptr);
            }
            else if (field_idx < struct_def.size())
            {
                auto&& def_field = struct_def[field_idx].as<ast::StructField>();
                if (def_field.getValue().has_value())
                {
                    auto&& val = ast::visit<llvm::Value*>(
                        def_field.getValue().value(), ctx);
                    builder.CreateStore(val, field_ptr);
                }
            }

            ++field_idx;
        }

        return alloca;
    }

    auto&& init_val = ast::visit<llvm::Value*>(expr, ctx);

    if (init_val != nullptr)
    {
        ctx.b_.CreateStore(init_val, alloca);
    }

    return alloca;
}

export llvm::Value*
visit(const ast::anyNode& node, const ast::BinOp& /*unused*/, GenContext& ctx)
{
    UNPACK_CTX(ctx)

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
        auto& rarg = binop.getRarg();
        if (rarg.type() == typeid(ast::Lit<int>) &&
            rarg.as<ast::Lit<int>>().data() == 0)
        {
            node.setWarningMsg("division by zero");
            node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::WARNING);
        }
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

export auto
visit(const ast::anyNode& node, const ast::IfElse& /*unused*/, GenContext& ctx)
{
    UNPACK_CTX(ctx)

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

export struct GlobalBlock
{};

export auto
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

export llvm::Value*
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

export auto
visit(const ast::anyNode& node, const ast::Func& /*unused*/, GenContext& ctx)
{
    UNPACK_CTX(ctx)

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

    if (!builder.GetInsertBlock()->getTerminator())
    {
        builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));
    }

    table.riseScope();
    builder.SetInsertPoint(old_label);

    return block_val;
}

export auto
handleUserArgs(GenContext& ctx, const ast::FuncCall& node)
{
    UNPACK_CTX(ctx)

    std::vector<llvm::Value*> llvm_all_args;

    std::size_t arg_idx{ 0 };

    for (auto&& arg : node.getArgs())
    {
        auto&& struct_field = arg.as<ast::StructField>();
        auto&& raw_val      = struct_field.getValue();

        if (!raw_val.has_value())
        {
            auto&& func_def = table.findObj(std::string(node.getName()));

            if (func_def.has_value())
            {
                auto&& func_ast =
                    func_def.value()->second.type_info_.as<ast::Func>();

                auto&& def_args = func_ast.getArgs();

                if (arg_idx < def_args.size())
                {
                    auto&& def_field =
                        def_args.at(arg_idx).as<ast::StructField>();

                    if (def_field.getValue().has_value())
                    {
                        llvm_all_args.push_back(ast::visit<llvm::Value*>(
                            def_field.getValue().value(), ctx));
                    }
                    else
                    {
                        arg.setErrorMsg(std::format(
                            "argument {} of function '{}' has no default value "
                            "(probably undefined behavior)",
                            arg_idx,
                            std::string(node.getName())));
                        arg.print(
                            ast::ErrorHandlerExt<ast::anyNode>::Type::WARNING);

                        llvm_all_args.push_back(
                            llvm::UndefValue::get(builder.getInt32Ty()));
                    }
                }
                else
                {
                    arg.setErrorMsg(std::format(
                        "argument {} out of range for function {} "
                        "(last arguments are likely to be ignored) ",
                        arg_idx,
                        std::string(node.getName())));
                    arg.print(
                        ast::ErrorHandlerExt<ast::anyNode>::Type::WARNING);
                }
            }
            else
            {
                arg.setErrorMsg(std::format("use of undeclared function '{}'",
                    std::string(node.getName())));

                arg.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
            }
        }
        else if (raw_val.value().type().name() == typeid(ast::Var).name())
        {
            auto&& arg_it =
                table.findObj(raw_val.value().as<ast::Var>().data());

            if (!arg_it.has_value())
            {
                raw_val.value().setErrorMsg(
                    std::format("variable '{}' used before initialisation",
                        raw_val.value().as<ast::Var>().data()));
                raw_val.value().print(
                    ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);

                // throw std::runtime_error(
                //     std::format("Variable \"{}\" used before initialisation",
                //         struct_field.getName()));
            }

            llvm_all_args.push_back(arg_it.value()->second.value_);
        }
        else if ((raw_val.value().type().name() ==
                     typeid(ast::Lit<int>).name()) ||
                 (raw_val.value().type().name() ==
                     typeid(ast::Lit<std::string>).name()) ||
                 (raw_val.value().type().name() ==
                     typeid(ast::StructEditor).name()))
        {
            llvm_all_args.push_back(
                ast::visit<llvm::Value*>(raw_val.value(), ctx));
        }
        else
        {
            throw std::runtime_error(std::format(
                "Type of {} argument is not supported in print.", arg_idx));
        }

        ++arg_idx;
    }

    return llvm_all_args;
}


export auto
visit(const ast::anyNode& node, const ast::While& /*unused*/, GenContext& ctx)
{
    UNPACK_CTX(ctx)

    auto&& whilenode = node.as<ast::While>();

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

export auto
visit(const ast::anyNode& node, const ast::Struct& /*unused*/, GenContext& ctx)
    -> llvm::Value*
{
    auto&& struc = node.as<ast::Struct>();
    auto&& name  = struc.getName();

    auto&& obj_it = ctx.t_.findObj(name);

    if (!obj_it.has_value())
    {
        return nullptr;
    }

    return obj_it.value()->second.value_;
}

export auto
visit(const ast::anyNode& node,
    const ast::StructEditor& /*unused*/,
    GenContext& ctx) -> llvm::Value*
{
    UNPACK_CTX(ctx)

    auto&& struct_editor = node.as<ast::StructEditor>();
    auto&& instance      = struct_editor.getNameOfInstance();
    auto&& changeable_field =
        struct_editor.getEditableField().as<ast::StructField>();

    auto&& obj_it = table.findObj(std::string(instance));

    if (!obj_it.has_value())
    {
        node.setErrorMsg(
            std::format("use of undeclared (struct) identifier '{}'",
                std::string(instance)));
        node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }

    auto&& struct_entry = obj_it.value()->second;
    auto&& instance_type_info =
        struct_entry.type_info_.asChangeable<ast::Struct>();


    // clang-format off
    auto&& field_it = std::ranges::find_if(
        instance_type_info,
        [&changeable_field](auto&& any_node_field)
        {
            return any_node_field.template as<ast::StructField>().getName() 
                        == changeable_field.getName();
        }
    );
    // clang-format on

    if (field_it == instance_type_info.end())
    {
        struct_editor.getEditableField().setErrorMsg(
            std::format("no member named '{}' in '{}'",
                changeable_field.getName(),
                std::string(instance)));
        struct_editor.getEditableField().print(
            ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }



    auto&& struct_field = field_it->as<ast::StructField>();
    auto&& has_value    = changeable_field.getValue().has_value();

    auto&& field_index = field_it - instance_type_info.begin();
    auto&& struct_ptr  = struct_entry.value_;

    llvm::Type* struct_type = llvm::StructType::getTypeByName(
        module.getContext(), instance_type_info.getName());

    if (!struct_field.getValue().has_value() && !has_value)
    {
        struct_editor.getEditableField().setWarningMsg(
            std::format("use of uninitialised field '{}' in struct '{}'",
                changeable_field.getName(),
                std::string(instance)));
        struct_editor.getEditableField().print(
            ast::ErrorHandlerExt<ast::anyNode>::Type::WARNING);

        auto&& struct_obj_it =
            table.findObj(std::string(instance_type_info.getName()));

        struct_obj_it.value()->second.type_info_.setNoteMsg(
            "struct definition is here");
        struct_obj_it.value()->second.type_info_.print(
            ast::ErrorHandlerExt<ast::anyNode>::Type::NOTE);
    }



    auto&& field_ptr = builder.CreateGEP(struct_type,
        struct_ptr,
        { builder.getInt32(0), builder.getInt32(field_index) });

    if (!has_value)
    {
        auto* field_type = (struct_field.getValue().has_value() &&
                               struct_field.getValue().value().type() ==
                                   typeid(ast::Lit<std::string>))
                               ? static_cast<llvm::Type*>(builder.getPtrTy())
                               : static_cast<llvm::Type*>(builder.getInt32Ty());
        return builder.CreateLoad(field_type, field_ptr);
    }

    auto&& expression =
        ast::visit<llvm::Value*>(changeable_field.getValue().value(), ctx);

    builder.CreateStore(expression, field_ptr);

    instance_type_info.at(field_index)
        .asChangeable<ast::StructField>()
        .setValue(ast::anyNode(changeable_field.getValue().value()));

    return field_ptr;
}

export auto
handlePrintf(const ast::anyNode& node, GenContext& ctx)
{
    UNPACK_CTX(ctx)

    auto&& func_call = node.as<ast::FuncCall>();
    auto&& llvm_func = module.getFunction("printf");

    generateDeclaration(ctx);

    auto&& fmt_str = generateFmtStrForPrintf(ctx, func_call);

    llvm::GlobalVariable* fmt_str_var =
        builder.CreateGlobalString(fmt_str, "printf_fmt");

    std::vector<llvm::Value*> llvm_all_args;
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
                throw std::runtime_error(
                    std::format("Variable \"{}\" used before initialisation",
                        struct_field.getName()));
            }

            llvm_all_args.push_back(
                ast::visit<llvm::Value*>(raw_val.value(), ctx));
        }
        else if (raw_val.value().type().name() ==
                 typeid(ast::StructEditor).name())
        {
            auto&& editor   = raw_val.value().as<ast::StructEditor>();
            auto&& instance = editor.getNameOfInstance();
            auto&& changeable_field =
                editor.getEditableField().as<ast::StructField>();

            auto&& obj_it = table.findObj(std::string(instance));

            if (!obj_it.has_value())
            {
                raw_val.value().setErrorMsg(std::format(
                    "struct '{}' not found", std::string(instance)));
                raw_val.value().print(
                    ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
            }

            auto&& struct_entry = obj_it.value()->second;
            auto&& instance_type_info =
                struct_entry.type_info_.as<ast::Struct>();

            uint32_t field_index = 0;
            uint32_t idx         = 0;
            bool found           = false;

            for (auto&& fld : instance_type_info)
            {
                if (fld.as<ast::StructField>().getName() ==
                    changeable_field.getName())
                {
                    field_index = idx;
                    found       = true;
                    break;
                }
                ++idx;
            }

            if (!found)
            {
                editor.getEditableField().setErrorMsg(
                    std::format("no member named '{}' in '{}'",
                        changeable_field.getName(),
                        std::string(instance)));
                editor.getEditableField().print(
                    ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
            }

            auto&& struct_ptr = struct_entry.value_;

            llvm::Type* struct_type = llvm::StructType::getTypeByName(
                module.getContext(), instance_type_info.getName());

            auto&& field_ptr = builder.CreateGEP(struct_type,
                struct_ptr,
                { builder.getInt32(0), builder.getInt32(field_index) });

            auto&& def_field =
                instance_type_info[field_index].as<ast::StructField>();
            auto* field_type =
                (def_field.getValue().has_value() &&
                    def_field.getValue().value().type() ==
                        typeid(ast::Lit<std::string>))
                    ? static_cast<llvm::Type*>(builder.getPtrTy())
                    : static_cast<llvm::Type*>(builder.getInt32Ty());
            llvm_all_args.push_back(builder.CreateLoad(field_type, field_ptr));
        }
        else
        {
            llvm_all_args.push_back(
                ast::visit<llvm::Value*>(raw_val.value(), ctx));
        }
    }

    return builder.CreateCall(llvm_func, llvm_all_args);
}

export auto
visit(
    const ast::anyNode& node, const ast::FuncCall& /*unused*/, GenContext& ctx)
{
    UNPACK_CTX(ctx)

    auto&& func_call = node.as<ast::FuncCall>();

    if (func_call.getName() == "print")
    {
        return handlePrintf(node, ctx);
    }

    llvm::Function* llvm_func = module.getFunction(func_call.getName());

    if (llvm_func == nullptr)
    {
        node.setErrorMsg(std::format(
            "use of undeclared function '{}'", func_call.getName()));
        node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
    }

    if (func_call.getArgs().size() > llvm_func->arg_size())
    {
        node.setWarningMsg(std::format(
            "too many arguments to function '{}', expected {} but got {}",
            func_call.getName(),
            llvm_func->arg_size(),
            func_call.getArgs().size()));

        node.print(ast::ErrorHandlerExt<ast::anyNode>::Type::WARNING);
    }

    auto&& user_args     = handleUserArgs(ctx, func_call);
    auto&& llvm_all_args = std::vector<llvm::Value*>();

    llvm_all_args.insert(llvm_all_args.end(),
        std::make_move_iterator(user_args.begin()),
        std::make_move_iterator(user_args.end()));

    return builder.CreateCall(llvm_func, llvm_all_args);
}

export auto
visit(const ast::anyNode& node, const ast::Return& ret_node, GenContext& ctx)
{
    UNPACK_CTX(ctx)
    auto&& value = ast::visit<llvm::Value*>(ret_node.getValue(), ctx);

    builder.CreateRet(value);
    return value;
}

} // namespace ir_generator
