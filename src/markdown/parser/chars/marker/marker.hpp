#pragma once

// List-marker object: -, *, +, 1., 1) and content-indent computation.

#include <string>

namespace md {

/// Result of matching a list marker on a line.
struct Marker {
    bool ok = false;          ///< Whether a marker was found.
    bool ordered = false;     ///< True for ordered markers (1. / 1)), false for bullets.
    size_t indent = 0;        ///< Indentation of the marker itself.
    size_t contentIndent = 0; ///< Indent at which the item content starts.
    std::string rest;         ///< Remaining line content after the marker.
};

/// Matches a list marker at the start of a line.
/// Param line: Source line.
/// Returns Parsed marker; ok == false when the line does not start a list item.
Marker matchListMarker(const std::string& line);

} // namespace md
