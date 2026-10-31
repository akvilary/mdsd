#pragma once

// Объект «HTML-тег»: разбор, атрибуты, стилевые пары (b/i/u/s/code...).

#include <string>

#include "markdown/ast/ast.hpp"

namespace md::detail {

struct HtmlTag {
    std::string name; // в нижнем регистре
    bool closing = false;
    std::string attrs;
    size_t end = 0; // позиция после '>'
};

bool matchHtmlTag(const std::string& s, size_t pos, HtmlTag& out);
std::string attrValue(const std::string& attrs, const std::string& name);
size_t findClosingTag(const std::string& s, size_t from, const std::string& name);

// 1..5 -> bold/italic/underline/strike/code, 0 — не стилевой тег
int styleTagKind(const std::string& name);
StyleFlags withTagStyle(StyleFlags st, int kind);

} // namespace md::detail
