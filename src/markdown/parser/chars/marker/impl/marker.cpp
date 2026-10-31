#include "markdown/parser/chars/marker/marker.hpp"

#include "algo/algo.hpp"
#include "markdown/parser/chars/chars.hpp"

namespace md::detail {

namespace {

Marker finishMarker(const std::string& line, size_t p, size_t mlen, bool ordered) {
    const size_t after = p + mlen;
    if (after >= line.size()) { // маркер в конце строки — пустой пункт
        return {true, ordered, p, p + mlen + 1, std::string()};
    }
    const size_t spaces = countRun(line, after, [](char ch) { return ch == ' '; }) - after;
    if (spaces == 0) {
        return {}; // за маркером не пробел
    }
    if (spaces <= 4) {
        return {true, ordered, p, after + spaces, line.substr(after + spaces)};
    }
    // >4 пробелов: контент с маркер+1, избыточные пробелы сохраняются (код!)
    const size_t ci = p + mlen + 1;
    return {true, ordered, p, ci, line.substr(ci)};
}

} // namespace

Marker matchListMarker(const std::string& line) {
    const size_t p = indentOf(line);
    if (p >= line.size() || p >= 4) {
        return {};
    }
    const char c = line[p];
    if (ut::isOneOf(c, '-', '+', '*')) {
        return finishMarker(line, p, 1, false);
    }
    if (isDigit(c)) {
        const size_t digits = countRun(line, p, [](char ch) { return isDigit(ch); }) - p;
        if (digits > 9 || p + digits >= line.size() || !ut::isOneOf(line[p + digits], '.', ')')) {
            return {};
        }
        return finishMarker(line, p, digits + 1, true);
    }
    return {};
}

} // namespace md::detail
