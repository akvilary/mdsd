#pragma once

// Базовые символьно-строковые примитивы парсера markdown.
// Лист дерева зависимостей: ни от чего не зависят.

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace md::detail {

constexpr bool isSpaceCh(char c) { return c == ' ' || c == '\t'; }
constexpr bool isDigit(char c) { return c >= '0' && c <= '9'; }
constexpr bool isWordByte(unsigned char c) { return std::isalnum(c) != 0 || c >= 0x80; }

constexpr bool isPunct(char c) {
    constexpr std::string_view punct = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    return punct.find(c) != std::string_view::npos;
}

constexpr int kMaxNestingDepth = 64;

bool isBlank(const std::string& s);
std::string_view ltrim(std::string_view s);
std::string_view rtrim(std::string_view s);
std::string_view trim(std::string_view s);
std::string rtrimCopy(std::string s);
std::string toLower(std::string s);
size_t indentOf(std::string_view s);

// Подряд идущие символы, удовлетворяющие pred, начиная с from (позиция конца прогона).
template <typename Pred>
size_t countRun(std::string_view s, size_t from, Pred pred) {
    size_t n = from;
    while (n < s.size() && pred(s[n])) {
        ++n;
    }
    return n;
}

// Разбиение по символу (без views::split — переносимость между реализациями библиотек).
std::vector<std::string_view> splitBy(std::string_view s, char sep);

} // namespace md::detail
