module;

#include <algorithm>
#include <ranges>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <string>
#include <string_view>

export module ir_generate_context;

import ir_symbol_table;

namespace ir_generator
{

export class GenContext final
{
public:
    llvm::IRBuilder<>& b_;
    llvm::Module& m_;
    SymbolTable t_;
};

const uint8_t space_ascii_c = 32;
const uint8_t slash_ascii_c = 47;
const uint8_t colon_ascii_c = 58;
const uint8_t above_ascii_c = 62;

export std::string
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

} // namespace ir_generator
