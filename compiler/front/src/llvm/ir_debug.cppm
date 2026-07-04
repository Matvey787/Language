module;

#include <algorithm>
#include <format>
#include <ranges>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <string_view>

#include "spdlog/spdlog.h"

export module ir_debug;

namespace ir_generator
{

export void
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

export const uint8_t space_ascii_c = 32;
export const uint8_t slash_ascii_c = 47;
export const uint8_t colon_ascii_c = 58;
export const uint8_t above_ascii_c = 62;

} // namespace ir_generator
