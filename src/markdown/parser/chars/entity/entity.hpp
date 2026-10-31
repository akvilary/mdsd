#pragma once

// Объект «HTML-сущность»: &amp; &copy; &#9731; — декодирование в UTF-8.

#include <string>

namespace md::detail {

// Декодирует сущность в s[pos..]; end — позиция после ';'. false — не сущность.
bool decodeEntity(const std::string& s, size_t pos, size_t& end, std::string& out);

// Декодирует все сущности в строке.
std::string decodeEntities(const std::string& s);

} // namespace md::detail
