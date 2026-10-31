#pragma once

#include <cstddef>
#include <string>

namespace utf8 {

// Декодирует один кодпоинт начиная с s[i]; i сдвигается на длину последовательности.
// Некорректная последовательность возвращает U+FFFD (ok=false) и сдвигает на 1 байт.
char32_t decode(const std::string& s, size_t& i, bool* ok = nullptr);

// Число кодовых единиц UTF-16 для кодпоинта (2 для астральных, иначе 1).
int u16len(char32_t cp);

// Длина строки в кодовых единицах UTF-16 (как String.length в JS).
// Битые байты считаются по одному замещающему символу U+FFFD каждый.
size_t u16length(const std::string& s);

// Санитизация: заменяет некорректные UTF-8 последовательности на U+FFFD.
// Возвращает количество замен.
size_t sanitize(std::string& s);

} // namespace utf8
