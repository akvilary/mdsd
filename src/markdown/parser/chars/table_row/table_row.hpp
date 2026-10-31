#pragma once

// Table-row object: splitting by pipes, escaping, alignments.

#include <optional>
#include <string>
#include <vector>

#include "markdown/ast/ast.hpp"

namespace md::detail {

/// \brief Splits a table-row line into raw cell strings (escaped pipes kept as '|').
/// \param line Source line.
/// \return Cell texts without the surrounding pipes.
std::vector<std::string> splitTableRow(const std::string& line);

/// \brief Parses a single delimiter-row cell (":---", ":--:", "---:", "---").
/// \param c Cell text.
/// \return Alignment, or nullopt if the cell is not a valid delimiter.
std::optional<Align> delimiterCellAlign(const std::string& c);

/// \brief Parses a whole delimiter row into per-column alignments.
/// \param line Source line.
/// \return Vector of alignments, or nullopt if the row is not a delimiter row.
std::optional<std::vector<Align>> tableDelimiters(const std::string& line);

} // namespace md::detail
