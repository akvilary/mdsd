#pragma once

// Internal widget serialization functions (shared by impl/widget_writer_*.cpp).
// The exact widget shapes live only in these files — the golden schema from TestDocument.sabydoc.
//
// ordered_json preserves key insertion order — byte diffs against the reference stay stable.
// IMPORTANT: build arrays only via Json::array({...}) — nlohmann treats a two-element
// init-list {"tag", {...}} as an object.

#include "widget_writer.hpp" // sd::Json — the single definition of the alias

namespace sd::io {

using sd::Json;

/// \brief Serializes a block by dispatching on its Kind.
/// \param b The block to serialize.
/// \return The JSONML node representing the block value.
Json writeBlockValue(const Block& b);

/// \brief Serializes a rich text with its formatting ranges.
/// \param t The text to serialize.
/// \return The JSONML node for the formatted text.
Json writeFormattedText(const Text& t);

/// \brief Serializes a paragraph block (including header variants).
/// \param b The paragraph-like block.
/// \return The JSONML node for the paragraph.
Json writeParagraph(const Block& b);

/// \brief Serializes a table block with its rows and column alignments.
/// \param b The table block.
/// \return The JSONML node for the table.
Json writeTable(const Block& b);

} // namespace sd::io
