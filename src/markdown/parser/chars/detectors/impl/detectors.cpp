#include "markdown/parser/chars/detectors/detectors.hpp"

#include <ranges>

#include "algo/algo.hpp"
#include "markdown/parser/chars/chars.hpp"
#include "markdown/parser/chars/marker/marker.hpp"

namespace md {

bool isAtx(std::string_view line) {
    const size_t p = indentOf(line);
    if (p > 3 || p >= line.size() || line[p] != '#') {
        return false;
    }
    const size_t hashes = countRun(line, p, [](char c) { return c == '#'; }) - p;
    return hashes <= 6 && (p + hashes >= line.size() || line[p + hashes] == ' ');
}

bool isFenceLine(
    const std::string& line,
    char& fenceChar,
    size_t& fenceLen,
    std::string& info
){
    const size_t p = indentOf(line);
    if (p > 3 || p >= line.size()) {
        return false;
    }
    const char c = line[p];
    if (!ut::isOneOf(c, '`', '~')) {
        return false;
    }
    const size_t len = countRun(line, p, [c](char ch) { return ch == c; }) - p;
    if (len < 3) {
        return false;
    }
    const std::string infoStr(trim(std::string_view(line).substr(p + len)));
    if (c == '`' && infoStr.find('`') != std::string::npos) {
        return false;
    }
    fenceChar = c;
    fenceLen = len;
    info = infoStr;
    return true;
}

bool isHr(const std::string& line) {
    const size_t p = indentOf(line);
    if (p > 3 || p >= line.size()) {
        return false;
    }
    const char c = line[p];
    if (!ut::isOneOf(c, '*', '-', '_')) {
        return false;
    }
    const std::string_view rest = std::string_view(line).substr(p);
    if (std::ranges::any_of(rest, [&](char ch) { return ch != c && ch != ' '; })) {
        return false;
    }
    return static_cast<size_t>(std::ranges::count(rest, c)) >= 3;
}

bool isSetextUnderline(
    const std::string& line,
    int& level
){
    const size_t p = indentOf(line);
    if (p > 3 || p >= line.size()) {
        return false;
    }
    const char c = line[p];
    if (!ut::isOneOf(c, '=', '-')) {
        return false;
    }
    const std::string_view rest = std::string_view(line).substr(p);
    if (std::ranges::any_of(rest, [&](char ch) { return ch != c && ch != ' '; })) {
        return false;
    }
    level = (c == '=') ? 1 : 2;
    return true;
}

bool isQuoteStart(const std::string& line) {
    const size_t p = indentOf(line);
    return p <= 3 && p < line.size() && line[p] == '>';
}

bool isOrderedListStartWithOne(const std::string& line) {
    const size_t p = indentOf(line);
    const size_t q = countRun(line, p, [](char ch) { return isDigit(ch); });
    if (q == p || line.substr(p, q - p) != "1") {
        return false;
    }
    return q < line.size() && ut::isOneOf(line[q], '.', ')');
}

} // namespace md
