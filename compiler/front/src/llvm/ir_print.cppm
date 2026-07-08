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

#include "ir_ctx_unpack.hpp"
#include "spdlog/spdlog.h"

export module ir_print;

import ast;
import ir_generate_context;

namespace ir_generator
{

export llvm::FunctionCallee
generatePrintfDeclaration(GenContext& ctx)
{
    UNPACK_CTX_M(ctx)

    llvm::PointerType* byte_ptr_ty = builder.getPtrTy();

    llvm::FunctionType* printf_ty =
        llvm::FunctionType::get(builder.getInt32Ty(), { byte_ptr_ty }, true);

    llvm::FunctionCallee printf_func =
        module.getOrInsertFunction("printf", printf_ty);

    return printf_func;
}

export auto
generateFmtStrForPrintf(GenContext& ctx, const ast::FuncCall& node)
{
    UNPACK_CTX_M(ctx)

    std::string fmt_str;

    std::size_t arg_idx{ 0 };

    for (auto&& arg : node.getArgs())
    {
        auto&& field    = arg.as<ast::StructField>();
        auto&& arg_name = field.getName();
        auto&& raw_val  = field.getValue();

        if (raw_val.has_value())
        {
            if (raw_val.value().is<ast::Var>())
            {
                auto&& arg_it =
                    table.findObj(raw_val.value().as<ast::Var>().data());

                if (!arg_it.has_value())
                {
                    raw_val.value().setErrorMsg(std::format(
                        "variable '{}' used before initialisation", arg_name));
                    raw_val.value().print(
                        ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
                }

                if (arg_it.value()->second.type_info_.is<ast::Lit<int>>())
                {
                    fmt_str += "%d";
                }
                else if (arg_it.value()
                             ->second.type_info_.is<ast::Lit<std::string>>())
                {
                    fmt_str += "%s";
                }
            }
            else if (raw_val.value().is<ast::Lit<int>>())
            {
                fmt_str += "%d";
            }
            else if (raw_val.value().is<ast::Lit<std::string>>())
            {
                fmt_str += "%s";
            }
            else if (raw_val.value().is<ast::StructEditor>())
            {
                auto&& struct_editor  = raw_val.value().as<ast::StructEditor>();
                auto&& instance       = struct_editor.getNameOfInstance();
                auto&& any_node_field = struct_editor.getEditableField();
                auto&& instance_obj   = table.findObj(instance);

                if (!instance_obj.has_value())
                {
                    raw_val.value().setErrorMsg(
                        std::format("struct '{}' not found", instance));
                    raw_val.value().print(
                        ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
                }


                auto&& instance_type_info =
                    instance_obj.value()->second.type_info_.as<ast::Struct>();

                auto&& changeable_field = any_node_field.as<ast::StructField>();

                // clang-format off
                auto&& field_it = std::ranges::find_if(
                    instance_type_info,
                    [&changeable_field](auto&& any_node_field)
                    {
                        return any_node_field.template as<ast::StructField>().getName() == changeable_field.getName();
                    }
                );
                // clang-format on

                if (field_it == instance_type_info.end())
                {
                    instance_obj.value()->second.type_info_.setErrorMsg(
                        std::format("no member named '{}' in '{}'",
                            changeable_field.getName(),
                            instance));
                    instance_obj.value()->second.type_info_.print(
                        ast::ErrorHandlerExt<ast::anyNode>::Type::ERROR);
                }

                auto&& struct_field = field_it->as<ast::StructField>();
                auto&& has_value    = struct_field.getValue().has_value();



                if (has_value)
                {
                    if (struct_field.getValue().value().is<ast::Lit<int>>())
                    {
                        fmt_str += "%d";
                    }
                    else if (struct_field.getValue()
                                 .value()
                                 .is<ast::Lit<std::string>>())
                    {
                        fmt_str += "%s";
                    }
                    else if (struct_field.getValue().value().is<ast::BinOp>())
                    {
                        fmt_str += "%d";
                    }
                    else
                    {
                        throw std::runtime_error(
                            std::format("Type of {} print argument (argument "
                                        "from struct) is "
                                        "not supported in print.",
                                arg_idx));
                    }
                }
                else
                {
                    (*field_it).setWarningMsg(
                        std::format("use of uninitialised field '{}' in struct "
                                    "'{}' (probably undefined behavior)",
                            changeable_field.getName(),
                            instance));
                    (*field_it).print(
                        ast::ErrorHandlerExt<ast::anyNode>::Type::WARNING);


                    auto&& struct_obj_it = table.findObj(
                        std::string(instance_type_info.getName()));

                    struct_obj_it.value()->second.type_info_.setNoteMsg(
                        "struct definition is here");
                    struct_obj_it.value()->second.type_info_.print(
                        ast::ErrorHandlerExt<ast::anyNode>::Type::NOTE);

                    fmt_str += "%d";
                }
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

} // namespace ir_generator
