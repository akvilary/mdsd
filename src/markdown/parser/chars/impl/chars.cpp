#include "markdown/parser/chars/chars.hpp"

#include <algorithm>
#include <ranges>

namespace md::detail {

bool isBlank(const std::string& s) { return std::ranges::all_of(s, isSpaceCh); }

std::string_view ltrim(std::string_view s) {
    const auto it = std::ranges::find_if(s, [](char c) { return !isSpaceCh(c); });
    return s.substr(static_cast<size_t>(it - s.begin()));
}

std::string_view rtrim(std::string_view s) {
    while (!s.empty() && isSpaceCh(s.back())) {
        s.remove_suffix(1);
    }
    return s;
}

std::string_view trim(std::string_view s) { return rtrim(ltrim(s)); }

std::string rtrimCopy(std::string s) { return std::string(rtrim(s)); }

std::string toLower(std::string s) {
    std::ranges::transform(
        s,
        s.begin(),
        [](unsigned char c
    ){
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

size_t indentOf(std::string_view s) {
    const auto it = std::ranges::find_if(s, [](char c) { return c != ' '; });
    return static_cast<size_t>(it - s.begin());
}

std::vector<std::string_view> splitBy(
    std::string_view s,
    char sep
){
    std::vector<std::string_view> parts;
    size_t start = 0;
    while (true) {
        const size_t pos = s.find(sep, start);
        if (pos == std::string_view::npos) {
            parts.push_back(s.substr(start));
            return parts;
        }
        parts.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
}

} // namespace md::detail
