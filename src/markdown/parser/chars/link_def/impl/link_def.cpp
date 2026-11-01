#include "markdown/parser/chars/link_def/link_def.hpp"

#include "algo/algo.hpp"
#include "markdown/parser/chars/chars.hpp"

namespace md::detail {

std::string normalizeLabel(std::string s) {
    std::string out;
    bool prevSpace = false;
    for (char& c : s) {
        if (isSpaceCh(c)) {
            if (!prevSpace && !out.empty()) {
                out += ' ';
            }
            prevSpace = true;
        } else {
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            prevSpace = false;
        }
    }
    while (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

namespace {

// Start of a destination: <...> (spaces allowed inside) or "up to the first space".
// Advances v past the destination; returns false for a malformed angle form.
bool takeUrl(
    std::string_view& v,
    std::string& url
){
    if (!v.empty() && v.front() == '<') {
        const size_t gt = v.find('>', 1);
        if (gt == std::string_view::npos) {
            return false;
        }
        url = std::string(v.substr(1, gt - 1));
        v = trim(v.substr(gt + 1));
        return true;
    }
    const size_t sp = v.find_first_of(" \t\n");
    if (sp == std::string_view::npos) {
        url = std::string(v);
        v = {};
    } else {
        url = std::string(v.substr(0, sp));
        v = trim(v.substr(sp));
    }
    return true;
}

// Optional title: only correctly quoted; garbage is a parse error.
bool takeTitle(
    std::string_view& v,
    std::string& title
){
    if (v.empty()) {
        return true;
    }
    const bool quoted = (v.front() == '"' && v.back() == '"' && v.size() >= 2) ||
                        (v.front() == '\'' && v.back() == '\'' && v.size() >= 2);
    if (!quoted) {
        return false;
    }
    title = std::string(v.substr(1, v.size() - 2));
    return true;
}

} // namespace

bool matchLinkDef(
    const std::string& line,
    std::string& label,
    LinkDef& def
){
    std::string_view s = trim(line);
    if (!s.starts_with("[")) {
        return false;
    }
    const size_t close = s.find(']', 1);
    if (close == std::string_view::npos || close == 1 || s.size() < close + 2 ||
        s[close + 1] != ':') {
        return false;
    }
    label = normalizeLabel(std::string(s.substr(1, close - 1)));
    if (label.empty()) {
        return false;
    }
    s = trim(s.substr(close + 2));
    if (s.empty()) {
        return false;
    }

    std::string url, title;
    if (!takeUrl(s, url) || url.empty() || !takeTitle(s, title)) {
        return false; // broken <>-form, empty destination, or garbage instead of the title
    }
    def = {url, title};
    return true;
}

size_t matchBracket(
    const std::string& s,
    size_t open
){
    int depth = 0;
    for (size_t i = open; i < s.size(); ++i) {
        if (s[i] == '\\') {
            ++i;
            continue;
        }
        if (s[i] == '[') {
            ++depth;
        } else if (s[i] == ']' && --depth == 0) {
            return i;
        }
    }
    return std::string::npos;
}

namespace {

size_t findCloseParen(
    const std::string& s,
    size_t openParen
){
    int depth = 0;
    bool inQ = false;
    char qc = 0;
    for (size_t j = openParen; j < s.size(); ++j) {
        const char c = s[j];
        if (inQ) {
            if (c == qc) {
                inQ = false;
            }
        } else if (ut::isOneOf(c, '"', '\'')) {
            inQ = true;
            qc = c;
        } else if (c == '(') {
            ++depth;
        } else if (c == ')' && --depth == 0) {
            return j;
        }
    }
    return std::string::npos;
}

} // namespace

bool parseDestination(
    const std::string& s,
    size_t openParen,
    std::string& url,
    std::string& title,
    size_t& after
){
    if (openParen >= s.size() || s[openParen] != '(') {
        return false;
    }
    const size_t close = findCloseParen(s, openParen);
    if (close == std::string::npos) {
        return false;
    }
    after = close + 1;
    const std::string inner =
        s.substr(
            openParen + 1,
            close - openParen - 1
        );
    std::string_view v = trim(inner);
    if (!takeUrl(v, url) || !takeTitle(v, title)) {
        return false;
    }
    return !url.empty();
}

} // namespace md::detail
