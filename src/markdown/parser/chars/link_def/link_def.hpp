#pragma once

// Объект «определение ссылки» ([label]: url "title") и утилиты адресов ссылок.

#include <string>
#include <string_view>

namespace md::detail {

struct LinkDef {
    std::string url;
    std::string title;
};

std::string normalizeLabel(std::string s);
bool matchLinkDef(const std::string& line, std::string& label, LinkDef& def);

// Подбор парной ']' с учётом вложенности и экранирования.
size_t matchBracket(const std::string& s, size_t open);

// Адрес в (...) с балансом скобок и кавычками; after — позиция после ')'.
bool parseDestination(const std::string& s, size_t openParen, std::string& url, std::string& title,
                      size_t& after);

} // namespace md::detail
