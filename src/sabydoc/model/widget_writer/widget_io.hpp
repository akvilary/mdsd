#pragma once

// Внутренние функции сериализации виджетов (общие для impl/widget_writer_*.cpp).
// Точные формы виджетов живут только в этих файлах — золотая схема из TestDocument.sabydoc.
//
// ordered_json сохраняет порядок вставки ключей — байт-дифф с эталоном стабилен.
// ВАЖНО: массивы строим только через Json::array({...}) — двухэлементный init-лист
// {"тег", {...}} nlohmann трактует как объект.

#include "widget_writer.hpp" // sd::Json — единственное определение алиаса

namespace sd::io {

using sd::Json;

Json writeBlockValue(const Block& b); // диспетчер по Kind
Json writeFormattedText(const Text& t);
Json writeParagraph(const Block& b);
Json writeTable(const Block& b);

} // namespace sd::io
