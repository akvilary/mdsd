#pragma once

#include <string>

#include "sabydoc/model/model.hpp"

namespace sd {

// Сборка итогового .sabydoc (ZIP store) из модели: content.json + index.site + ресурсы.
std::string buildSabydocArchive(const Document& doc);

} // namespace sd
