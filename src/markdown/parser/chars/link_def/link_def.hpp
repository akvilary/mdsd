#pragma once

// Link-definition object ([label]: url "title") and link-destination utilities.

#include <string>
#include <string_view>

namespace md::detail {

/// A link definition: destination URL plus optional title.
struct LinkDef {
    std::string url;   ///< Destination URL.
    std::string title; ///< Optional title.
};

/// Normalizes a link label for lookups (lower-cased, whitespace collapsed and trimmed).
/// Param s: Raw label text.
/// Returns Normalized label.
std::string normalizeLabel(std::string s);

/// Tries to parse a link-definition line ([label]: url "title").
/// Param line: Source line.
/// \param[out] label Extracted normalized label.
/// \param[out] def Parsed definition (url and title).
/// Returns true if the line is a link definition.
bool matchLinkDef(
    const std::string& line,
    std::string& label,
    LinkDef& def
);

/// Finds the matching ']' for an opening '[', accounting for nesting and escapes.
/// Param s: Input string.
/// Param open: Position of the opening '['.
/// Returns Position of the matching ']', or std::string::npos if unmatched.
size_t matchBracket(
    const std::string& s,
    size_t open
);

/// Parses a destination in (...) with balanced parentheses and a quoted title.
/// Param s: Input string.
/// Param openParen: Position of the opening '('.
/// \param[out] url Extracted URL.
/// \param[out] title Extracted title (empty if absent).
/// \param[out] after Position just after the closing ')'.
/// Returns true if a destination was parsed.
bool parseDestination(
    const std::string& s,
    size_t openParen,
    std::string& url,
    std::string& title,
    size_t& after
);

} // namespace md::detail
