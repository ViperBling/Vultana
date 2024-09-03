#pragma once

#include <fmt/format.h>
#include <string>

template<>
struct fmt::formatter<std::string> : formatter<string_view>
{
    template<typename FormatContext>
    auto format(const std::string& value, FormatContext& ctx) const
    {
        return formatter<string_view>::format(value.c_str(), ctx);
    }
};