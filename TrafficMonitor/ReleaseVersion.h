#pragma once
#include <array>
#include <charconv>
#include <optional>
#include <string>

namespace ReleaseVersion
{
inline std::optional<std::array<unsigned, 3>> Parse(const std::string& text)
{
    std::array<unsigned, 3> parts{};
    size_t start{};
    for (size_t i = 0; i < parts.size(); ++i)
    {
        auto end = text.find('.', start);
        if (end == std::string::npos) end = text.size();
        if (start == end || (end - start > 1 && text[start] == '0')) return {};
        for (size_t p = start; p < end; ++p) if (text[p] < '0' || text[p] > '9') return {};
        auto parsed = std::from_chars(text.data() + start, text.data() + end, parts[i]);
        if (parsed.ec != std::errc() || parsed.ptr != text.data() + end || parts[i] > 65535) return {};
        if (end == text.size()) return i >= 1 ? std::optional(parts) : std::nullopt;
        start = end + 1;
    }
    return {};
}
inline bool Valid(const std::string& text) { return Parse(text).has_value(); }
inline bool Newer(const std::string& remote, const std::string& current)
{
    auto a = Parse(remote), b = Parse(current); return a && b && *a > *b;
}
}
