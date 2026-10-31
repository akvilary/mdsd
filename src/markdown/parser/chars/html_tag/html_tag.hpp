#pragma once

// HTML-tag object: parsing, attributes, style tags (b/i/u/s/code...).

#include <string>

#include "markdown/ast/ast.hpp"

namespace md::detail {

/// \brief A parsed inline HTML tag.
struct HtmlTag {
    std::string name;       // tag name, lower-cased
    bool closing = false;   ///< True for a closing tag (</name ...>).
    std::string attrs;      ///< Raw attribute text.
    size_t end = 0;         // position just after '>'
};

/// \brief Matches an HTML tag starting at s[pos].
/// \param s Input string.
/// \param pos Position of the '<'.
/// \param[out] out Parsed tag.
/// \return true if a tag was matched.
bool matchHtmlTag(const std::string& s, size_t pos, HtmlTag& out);

/// \brief Extracts the value of an attribute from raw attribute text.
/// \param attrs Raw attribute string.
/// \param name Attribute name (case-insensitive).
/// \return Attribute value, or an empty string if absent.
std::string attrValue(const std::string& attrs, const std::string& name);

/// \brief Finds the closing tag </name> starting at from.
/// \param s Input string.
/// \param from Position to search from.
/// \param name Tag name (lower-case).
/// \return Position of the closing tag, or std::string::npos if not found.
size_t findClosingTag(const std::string& s, size_t from, const std::string& name);

/// \brief Maps a tag name to a style kind.
/// \param name Tag name (lower-case).
/// \return 1..5 for bold/italic/underline/strike/code, 0 if not a style tag.
int styleTagKind(const std::string& name);

/// \brief Applies a style kind (from styleTagKind) to a flag set.
/// \param st Original flags.
/// \param kind Style kind (1..5).
/// \return Updated flags.
StyleFlags withTagStyle(StyleFlags st, int kind);

} // namespace md::detail
