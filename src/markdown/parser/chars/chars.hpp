#pragma once

// Basic character/string primitives of the markdown parser.
// Leaf of the dependency tree: depends on nothing.

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace md::detail {

/// \brief Checks whether the character is a space or tab.
/// \param c Character to check.
/// \return true if c is ' ' or '\t'.
constexpr bool isSpaceCh(char c) { return c == ' ' || c == '\t'; }

/// \brief Checks whether the character is an ASCII digit.
/// \param c Character to check.
/// \return true if c is in '0'..'9'.
constexpr bool isDigit(char c) { return c >= '0' && c <= '9'; }

/// \brief Checks whether the byte belongs to a word: alnum or non-ASCII (UTF-8).
/// \param c Byte to check.
/// \return true if the byte is alphanumeric or >= 0x80.
constexpr bool isWordByte(unsigned char c) { return std::isalnum(c) != 0 || c >= 0x80; }

/// \brief Checks whether the character is an ASCII punctuation character.
/// \param c Character to check.
/// \return true if c is one of the CommonMark punctuation characters.
constexpr bool isPunct(char c) {
    constexpr std::string_view punct = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    return punct.find(c) != std::string_view::npos;
}

/// \brief Maximum allowed nesting depth of parser structures.
constexpr int kMaxNestingDepth = 64;

/// \brief Checks whether the string is blank (empty or whitespace-only).
/// \param s String to check.
/// \return true if the string is empty or contains only spaces/tabs.
bool isBlank(const std::string& s);

/// \brief Strips leading whitespace.
/// \param s Input view.
/// \return View with leading whitespace removed.
std::string_view ltrim(std::string_view s);

/// \brief Strips trailing whitespace.
/// \param s Input view.
/// \return View with trailing whitespace removed.
std::string_view rtrim(std::string_view s);

/// \brief Strips leading and trailing whitespace.
/// \param s Input view.
/// \return Trimmed view.
std::string_view trim(std::string_view s);

/// \brief Returns a copy of the string without trailing whitespace.
/// \param s Input string.
/// \return Right-trimmed copy.
std::string rtrimCopy(std::string s);

/// \brief Converts the string to lower case.
/// \param s Input string.
/// \return Lower-cased copy.
std::string toLower(std::string s);

/// \brief Measures the indentation of a line (number of leading spaces).
/// \param s Input view.
/// \return Count of leading space characters.
size_t indentOf(std::string_view s);

/// \brief Counts consecutive characters satisfying pred starting at from.
/// \param s Input view.
/// \param from Starting position.
/// \param pred Predicate applied to each character.
/// \return Position just past the end of the run.
template <typename Pred>
size_t countRun(std::string_view s, size_t from, Pred pred) {
    size_t n = from;
    while (n < s.size() && pred(s[n])) {
        ++n;
    }
    return n;
}

/// \brief Splits the view by a separator character (avoids views::split for
/// portability across library implementations).
/// \param s Input view.
/// \param sep Separator character.
/// \return Fragments of the input, without the separators.
std::vector<std::string_view> splitBy(std::string_view s, char sep);

} // namespace md::detail
