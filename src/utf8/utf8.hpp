#pragma once

#include <cstddef>
#include <string>

namespace utf8 {

/// Decodes a single codepoint starting at s[i]; advances i by the sequence length.
/// An invalid sequence yields U+FFFD (ok=false) and advances i by one byte.
/// Param s: The input string.
/// Param i: The current byte index; advanced past the consumed sequence.
/// Param ok: Optional flag set to true on success, false on invalid input.
/// Returns The decoded codepoint, or U+FFFD for an invalid sequence.
char32_t decode(
    const std::string& s,
    size_t& i,
    bool* ok = nullptr
);

/// Returns the number of UTF-16 code units for a codepoint
/// (2 for astral codepoints, otherwise 1).
/// Param cp: The codepoint to measure.
/// Returns The UTF-16 code unit count (1 or 2).
int u16len(char32_t cp);

/// Computes the length of a string in UTF-16 code units (like String.length in JS).
/// Each broken byte counts as one replacement character U+FFFD.
/// Param s: The input UTF-8 string.
/// Returns The number of UTF-16 code units.
size_t u16length(const std::string& s);

/// Sanitizes the string: replaces invalid UTF-8 sequences with U+FFFD.
/// Param s: The string to sanitize in place.
/// Returns The number of performed replacements.
size_t sanitize(std::string& s);

} // namespace utf8
