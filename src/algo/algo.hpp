#pragma once

#include <algorithm>
#include <initializer_list>
#include <ranges>
#include <string_view>

// Утилиты проверок членства поверх C++20 ranges.
namespace ut {

// value == any of (others...) — замена цепочкам a == x || a == y || a == z.
template <typename T, typename... Ts>
constexpr bool isOneOf(const T& value, const Ts&... others) {
    return ((value == others) || ...);
}

// Префикс s — один из перечисленных.
constexpr bool startsWithAny(std::string_view s, std::initializer_list<std::string_view> prefixes) {
    return std::ranges::any_of(prefixes, [&](std::string_view p) { return s.starts_with(p); });
}

} // namespace ut
