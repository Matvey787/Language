module;

#include <algorithm>
#include <cstddef>
#include <format>
#include <functional>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <ranges>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "spdlog/spdlog.h"

export module ir_symbol_table;

import ast;

namespace ir_generator
{

export auto
llvmValueToTypeStr(const llvm::Value* val) -> std::string
{
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    val->getType()->print(rso);

    return type_str;
}

export class SymbolTable final
{
    struct VarData
    {
        llvm::Value* value_{};
        ast::anyNode type_info_;
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
        operator!=(const ScopeData& other) const -> bool
        {
            return symbols_.size() != other.symbols_.size() ||
                   children_.size() != other.children_.size();
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

} // namespace ir_generator
