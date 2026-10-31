#pragma once

#include <string>

#include "sabydoc/model/model.hpp"

namespace sd {

/// Assembles the final .sabydoc archive (ZIP store) from the model:
/// content.json + index.site + resources.
/// Param doc: The source document model.
/// Returns The complete .sabydoc archive bytes.
std::string buildSabydocArchive(const Document& doc);

} // namespace sd
