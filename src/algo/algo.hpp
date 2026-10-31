#pragma once

#include <algorithm>
#include <initializer_list>
#include <ranges>
#include <string_view>

/// Membership-check utilities on top of C++20 ranges.
namespace ut {

/// Checks whether value equals any of the given candidates.
/// Replacement for chains like a == x || a == y || a == z.
/// Param value: The value to test.
/// Param others: Candidate values to compare against.
/// Returns true if value equals at least one of others.
template <typename T, typename... Ts>
constexpr bool isOneOf(
    const T& value,
    const Ts&... others
){
    return ((value == others) || ...);
}

/// Checks whether s starts with any of the listed prefixes.
/// Param s: The string view to test.
/// Param prefixes: Candidate prefixes.
/// Returns true if s starts with at least one of the prefixes.
constexpr bool startsWithAny(
    std::string_view s,
    std::initializer_list<std::string_view> prefixes
){
    return std::ranges::any_of(prefixes, [&](std::string_view p) { return s.starts_with(p); });
}

} // namespace ut
