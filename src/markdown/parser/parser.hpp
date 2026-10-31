#pragma once

#include <string>

#include "markdown/ast/ast.hpp"

namespace md {

// Полный парсер markdown (CommonMark-подмножество + GFM: таблицы, task-листы,
// зачёркивание, ссылки/автоссылки, HTML-подмножество).
// Вход: сырой текст (BOM/CRLF/CR нормализуются; невалидный UTF-8 санитизируется
// с записью предупреждения). Выход: AST.
Document parseMarkdown(const std::string& text);

} // namespace md
