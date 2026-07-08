module;

#include <cstddef>
#include <filesystem>
#include <format>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>

#include "spdlog/spdlog.h"

export module ir_generator;

import ast;
import ir_generate_context;
import ir_symbol_table;
import ir_debug;
import ir_print;
import ir_first_pass;
import ir_second_pass;

namespace ir_generator
{

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

    GenContext ctx{ .b_ = builder, .m_ = module };

    generatePrintfDeclaration(ctx);
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
