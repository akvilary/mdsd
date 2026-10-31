#pragma once

// Сериализация модели SabyDoc в JSONML-дерево index.site.
// Все точные формы виджетов (атрибуты, их состав и порядок) живут ТОЛЬКО здесь —
// золотая схема, проверенная по TestDocument.sabydoc.

#include <nlohmann/json.hpp>

#include "sabydoc/model/model.hpp"

namespace sd {

using Json = nlohmann::ordered_json;

Json writeFrame(const Document& doc);           // ["frame", {meta, resources}, [layout...]]
Json writeTableOfContents(const Document& doc); // массив записей toc_*

} // namespace sd
