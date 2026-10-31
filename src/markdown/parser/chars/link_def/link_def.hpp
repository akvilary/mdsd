#pragma once

// Link-definition object ([label]: url "title") and link-destination utilities.

#include <string>
#include <string_view>

namespace md::detail {

/// \brief A link definition: destination URL plus optional title.
struct LinkDef {
    std::string url;    ///< Destination URL.
    std::string title;  ///< Optional title.
};

/// \brief Normalizes a link label for lookups (lower-cased, whitespace collapsed and trimmed).
/// \param s Raw label text.
/// \return Normalized label.
std::string normalizeLabel(std::string s);

/// \brief Tries to parse a link-definition line ([label]: url "title").
/// \param line Source line.
/// \param[out] label Extracted normalized label.
/// \param[out] def Parsed definition (url and title).
/// \return true if the line is a link definition.
bool matchLinkDef(const std::string& line, std::string& label, LinkDef& def);

/// \brief Finds the matching ']' for an opening '[', accounting for nesting and escapes.
/// \param s Input string.
/// \param open Position of the opening '['.
/// \return Position of the matching ']', or std::string::npos if unmatched.
size_t matchBracket(const std::string& s, size_t open);

/// \brief Parses a destination in (...) with balanced parentheses and a quoted title.
/// \param s Input string.
/// \param openParen Position of the opening '('.
/// \param[out] url Extracted URL.
/// \param[out] title Extracted title (empty if absent).
/// \param[out] after Position just after the closing ')'.
/// \return true if a destination was parsed.
bool parseDestination(const std::string& s, size_t openParen, std::string& url, std::string& title,
                      size_t& after);

} // namespace md::detail
