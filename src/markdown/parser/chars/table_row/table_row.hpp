#pragma once

// Table-row object: splitting by pipes, escaping, alignments.

#include <optional>
#include <string>
#include <vector>

#include "markdown/ast/ast.hpp"

namespace md {

/// Splits a table-row line into raw cell strings (escaped pipes kept as '|').
/// Param line: Source line.
/// Returns Cell texts without the surrounding pipes.
std::vector<std::string> splitTableRow(const std::string& line);

/// Parses a single delimiter-row cell (":---", ":--:", "---:", "---").
/// Param c: Cell text.
/// Returns Alignment, or nullopt if the cell is not a valid delimiter.
std::optional<Align> delimiterCellAlign(const std::string& c);

/// Parses a whole delimiter row into per-column alignments.
/// Param line: Source line.
/// Returns Vector of alignments, or nullopt if the row is not a delimiter row.
std::optional<std::vector<Align>> tableDelimiters(const std::string& line);

} // namespace md
