#pragma once

#include <cstddef>
#include <string>

namespace utf8 {

/// \brief Decodes a single codepoint starting at s[i]; advances i by the sequence length.
/// An invalid sequence yields U+FFFD (ok=false) and advances i by one byte.
/// \param s The input string.
/// \param i The current byte index; advanced past the consumed sequence.
/// \param ok Optional flag set to true on success, false on invalid input.
/// \return The decoded codepoint, or U+FFFD for an invalid sequence.
char32_t decode(const std::string& s, size_t& i, bool* ok = nullptr);

/// \brief Returns the number of UTF-16 code units for a codepoint
/// (2 for astral codepoints, otherwise 1).
/// \param cp The codepoint to measure.
/// \return The UTF-16 code unit count (1 or 2).
int u16len(char32_t cp);

/// \brief Computes the length of a string in UTF-16 code units (like String.length in JS).
/// Each broken byte counts as one replacement character U+FFFD.
/// \param s The input UTF-8 string.
/// \return The number of UTF-16 code units.
size_t u16length(const std::string& s);

/// \brief Sanitizes the string: replaces invalid UTF-8 sequences with U+FFFD.
/// \param s The string to sanitize in place.
/// \return The number of performed replacements.
size_t sanitize(std::string& s);

} // namespace utf8
