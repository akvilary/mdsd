#pragma once

// HTML-tag object: parsing, attributes, style tags (b/i/u/s/code...).

#include <string>

#include "markdown/ast/ast.hpp"

namespace md::detail {

/// A parsed inline HTML tag.
struct HtmlTag {
    std::string name;     // tag name, lower-cased
    bool closing = false; ///< True for a closing tag (</name ...>).
    std::string attrs;    ///< Raw attribute text.
    size_t end = 0;       // position just after '>'
};

/// Matches an HTML tag starting at s[pos].
/// Param s: Input string.
/// Param pos: Position of the '<'.
/// \param[out] out Parsed tag.
/// Returns true if a tag was matched.
bool matchHtmlTag(
    const std::string& s,
    size_t pos,
    HtmlTag& out
);

/// Extracts the value of an attribute from raw attribute text.
/// Param attrs: Raw attribute string.
/// Param name: Attribute name (case-insensitive).
/// Returns Attribute value, or an empty string if absent.
std::string attrValue(
    const std::string& attrs,
    const std::string& name
);

/// Finds the closing tag </name> starting at from.
/// Param s: Input string.
/// Param from: Position to search from.
/// Param name: Tag name (lower-case).
/// Returns Position of the closing tag, or std::string::npos if not found.
size_t findClosingTag(
    const std::string& s,
    size_t from,
    const std::string& name
);

/// Maps a tag name to a style kind.
/// Param name: Tag name (lower-case).
/// Returns 1..5 for bold/italic/underline/strike/code, 0 if not a style tag.
int styleTagKind(const std::string& name);

/// Applies a style kind (from styleTagKind) to a flag set.
/// Param st: Original flags.
/// Param kind: Style kind (1..5).
/// Returns Updated flags.
StyleFlags withTagStyle(
    StyleFlags st,
    int kind
);

} // namespace md::detail
