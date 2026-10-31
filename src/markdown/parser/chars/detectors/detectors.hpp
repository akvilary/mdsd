#pragma once

// Детекторы строк-блоков: заголовки, fence, hr, цитаты, setext, «1.».

#include <string>

namespace md::detail {

bool isAtx(std::string_view line);
bool isFenceLine(const std::string& line, char& fenceChar, size_t& fenceLen, std::string& info);
bool isHr(const std::string& line);
bool isSetextUnderline(const std::string& line, int& level);
bool isQuoteStart(const std::string& line);

// Число упорядоченного маркера равно 1 (только такие прерывают абзац — CommonMark).
bool isOrderedListStartWithOne(const std::string& line);

} // namespace md::detail
