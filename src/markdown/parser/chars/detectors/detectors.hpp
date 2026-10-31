#pragma once

// Block-line detectors: headers, fences, hr, quotes, setext, "1.".

#include <string>

namespace md::detail {

/// Detects an ATX heading line ("# ...").
/// Param line: Source line.
/// Returns true if the line starts an ATX header.
bool isAtx(std::string_view line);

/// Detects a fenced-code line (``` or ~~~).
/// Param line: Source line.
/// \param[out] fenceChar Fence character ('`' or '~').
/// \param[out] fenceLen Number of fence characters.
/// \param[out] info Info string after the fence (e.g. the language).
/// Returns true if the line is a fence line.
bool isFenceLine(
    const std::string& line,
    char& fenceChar,
    size_t& fenceLen,
    std::string& info
);

/// Detects a thematic break (horizontal rule) line.
/// Param line: Source line.
/// Returns true if the line is an hr.
bool isHr(const std::string& line);

/// Detects a setext underline line ("===" / "---").
/// Param line: Source line.
/// \param[out] level Header level (1 for '=', 2 for '-').
/// Returns true if the line is a setext underline.
bool isSetextUnderline(
    const std::string& line,
    int& level
);

/// Detects the start of a block quote (">").
/// Param line: Source line.
/// Returns true if the line starts a quote.
bool isQuoteStart(const std::string& line);

/// Detects an ordered-list marker whose number equals 1 (only such markers
/// interrupt a paragraph — CommonMark).
/// Param line: Source line.
/// Returns true if the line starts an ordered list with number 1.
bool isOrderedListStartWithOne(const std::string& line);

} // namespace md::detail
