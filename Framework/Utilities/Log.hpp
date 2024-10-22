#pragma once

#include <fmt/format.h>
#include <string>
// #define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>

template<>
struct fmt::formatter<std::string> : formatter<string_view>
{
    template<typename FormatContext>
    auto format(const std::string& value, FormatContext& ctx) const
    {
        return formatter<string_view>::format(value.c_str(), ctx);
    }
};

#define VTNA_LOG_TRACE(...) spdlog::default_logger_raw()->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::trace, __VA_ARGS__)
#define VTNA_LOG_DEBUG(...) spdlog::default_logger_raw()->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::debug, __VA_ARGS__)
#define VTNA_LOG_INFO(...) spdlog::default_logger_raw()->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::info, __VA_ARGS__)
#define VTNA_LOG_WARN(...) spdlog::default_logger_raw()->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::warn, __VA_ARGS__)
#define VTNA_LOG_ERROR(...) spdlog::default_logger_raw()->log(spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::err, __VA_ARGS__)