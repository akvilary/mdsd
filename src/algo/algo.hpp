#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
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

/// Converts a string literal into a fixed-size byte sequence at compile time.
/// Lets binary magic values be spelled as readable strings:
/// toBytes("\x89PNG\r\n\x1A\n") instead of {std::byte{0x89}, std::byte{'P'}, ...}.
/// Param s: The literal to convert; the terminating '\0' is dropped.
/// Returns The literal's bytes as a compile-time array.
template <size_t N>
constexpr std::array<std::byte, N - 1> toBytes(const char (&s)[N]) {
    std::array<std::byte, N - 1> out{};
    for (size_t i = 0; i + 1 < N; ++i) {
        out[i] = static_cast<std::byte>(s[i]);
    }
    return out;
}

/// Checks whether the buffer at p begins with the signature sig.
/// The caller must guarantee that at least sig.size() bytes are readable at p.
/// Param p: The buffer to test.
/// Param sig: The expected byte sequence.
/// Returns true if the first sig.size() bytes at p equal sig.
template <size_t M>
constexpr bool startsWith(
    const std::byte* p,
    const std::array<std::byte, M>& sig
){
    return std::equal(sig.begin(), sig.end(), p);
}

} // namespace ut
