#pragma once

#include <vector>

#include "markdown/ast/ast.hpp"
#include "sabydoc/model/model.hpp"

namespace sd {

// Инлайн-элементы markdown -> Text с диапазонами [offset, length] в UTF-16.
// Картинки заменяются на alt-текст (или url), '\n' — на пробел.
// Стиль code не отображается (в sabydoc нет инлайн-кода) — документированное ограничение.
Text buildText(const std::vector<md::InlineElement>& elems);

// Простой текст без форматирования (подписи, заглушки).
Text plainText(const std::string& s);

// Копия с принудительным жирным (h6 и заголовки внутри цитат -> абзац).
Text forceBold(Text t);

} // namespace sd
