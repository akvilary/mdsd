#pragma once

// HTML-entity object: &amp; &copy; &#9731; — decoding into UTF-8.

#include <string>

namespace md::detail {

/// Decodes a single entity starting at s[pos].
/// Param s: Input string.
/// Param pos: Position of the '&'.
/// \param[out] end Position just after the ';'.
/// \param[out] out Decoded UTF-8 text.
/// Returns true if a valid entity was decoded; false if s[pos..] is not an entity.
bool decodeEntity(
    const std::string& s,
    size_t pos,
    size_t& end,
    std::string& out
);

/// Decodes all entities in the string.
/// Param s: Input string.
/// Returns Copy of the string with every entity replaced by its UTF-8 value.
std::string decodeEntities(const std::string& s);

} // namespace md::detail
