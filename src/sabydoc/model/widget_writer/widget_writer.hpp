#pragma once

// Serialization of the SabyDoc model into the index.site JSONML tree.
// All exact widget shapes (attributes, their set and order) live ONLY here —
// the golden schema verified against TestDocument.sabydoc.

#include <nlohmann/json.hpp>

#include "sabydoc/model/model.hpp"

namespace sd {

/// JSON type preserving key insertion order (the single alias definition).
using Json = nlohmann::ordered_json;

/// Writes the root frame widget: ["frame", {meta, resources}, [layout...]].
/// Param doc: The source document model.
/// Returns The frame JSONML node.
Json writeFrame(const Document& doc);

/// Writes the table of contents: an array of toc_* entries.
/// Param doc: The source document model.
/// Returns A JSON array of toc_* entries.
Json writeTableOfContents(const Document& doc);

} // namespace sd
