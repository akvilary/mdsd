#pragma once

#include <string>

#include "sabydoc/model/model.hpp"

namespace sd {

/// \brief Assembles the final .sabydoc archive (ZIP store) from the model:
/// content.json + index.site + resources.
/// \param doc The source document model.
/// \return The complete .sabydoc archive bytes.
std::string buildSabydocArchive(const Document& doc);

} // namespace sd
