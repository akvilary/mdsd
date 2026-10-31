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

/// Checks whether the character is a space or tab.
/// Param c: Character to check.
/// Returns true if c is ' ' or '\t'.
constexpr bool isSpaceCh(char c) { return c == ' ' || c == '\t'; }

/// Checks whether the character is an ASCII digit.
/// Param c: Character to check.
/// Returns true if c is in '0'..'9'.
constexpr bool isDigit(char c) { return c >= '0' && c <= '9'; }

/// Checks whether the byte belongs to a word: alnum or non-ASCII (UTF-8).
/// Param c: Byte to check.
/// Returns true if the byte is alphanumeric or >= 0x80.
constexpr bool isWordByte(unsigned char c) { return std::isalnum(c) != 0 || c >= 0x80; }

/// Checks whether the character is an ASCII punctuation character.
/// Param c: Character to check.
/// Returns true if c is one of the CommonMark punctuation characters.
constexpr bool isPunct(char c) {
    constexpr std::string_view punct = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    return punct.find(c) != std::string_view::npos;
}

/// Maximum allowed nesting depth of parser structures.
constexpr int kMaxNestingDepth = 64;

/// Checks whether the string is blank (empty or whitespace-only).
/// Param s: String to check.
/// Returns true if the string is empty or contains only spaces/tabs.
bool isBlank(const std::string& s);

/// Strips leading whitespace.
/// Param s: Input view.
/// Returns View with leading whitespace removed.
std::string_view ltrim(std::string_view s);

/// Strips trailing whitespace.
/// Param s: Input view.
/// Returns View with trailing whitespace removed.
std::string_view rtrim(std::string_view s);

/// Strips leading and trailing whitespace.
/// Param s: Input view.
/// Returns Trimmed view.
std::string_view trim(std::string_view s);

/// Returns a copy of the string without trailing whitespace.
/// Param s: Input string.
/// Returns Right-trimmed copy.
std::string rtrimCopy(std::string s);

/// Converts the string to lower case.
/// Param s: Input string.
/// Returns Lower-cased copy.
std::string toLower(std::string s);

/// Measures the indentation of a line (number of leading spaces).
/// Param s: Input view.
/// Returns Count of leading space characters.
size_t indentOf(std::string_view s);

/// Counts consecutive characters satisfying pred starting at from.
/// Param s: Input view.
/// Param from: Starting position.
/// Param pred: Predicate applied to each character.
/// Returns Position just past the end of the run.
template <typename Pred>
size_t countRun(
    std::string_view s,
    size_t from,
    Pred pred
){
    size_t n = from;
    while (n < s.size() && pred(s[n])) {
        ++n;
    }
    return n;
}

/// Splits the view by a separator character (avoids views::split for
/// portability across library implementations).
/// Param s: Input view.
/// Param sep: Separator character.
/// Returns Fragments of the input, without the separators.
std::vector<std::string_view> splitBy(
    std::string_view s,
    char sep
);

} // namespace md::detail
