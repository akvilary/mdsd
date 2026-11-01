#include "markdown/parser/chars/html_tag/html_tag.hpp"

#include <cctype>
#include <cstddef>

#include "algo/algo.hpp"
#include "markdown/parser/chars/chars.hpp"

namespace md {

namespace {

std::string readTagAttrs(
    const std::string& s,
    size_t& p
){
    std::string attrs;
    while (p < s.size() && s[p] != '>') {
        if (ut::isOneOf(s[p], '"', '\'')) {
            const char q = s[p];
            attrs += s[p++];
            while (p < s.size() && s[p] != q) {
                attrs += s[p++];
            }
            if (p < s.size()) {
                attrs += s[p++];
            }
        } else {
            attrs += s[p++];
        }
    }
    return attrs;
}

std::string readAttrValue(
    const std::string& attrs,
    size_t v
){
    while (v < attrs.size() && isSpaceCh(attrs[v])) {
        ++v;
    }
    if (v < attrs.size() && ut::isOneOf(attrs[v], '"', '\'')) {
        const char q = attrs[v++];
        const size_t e = attrs.find(q, v);
        return attrs.substr(v, (e == std::string::npos ? attrs.size() : e) - v);
    }
    size_t e = v;
    while (e < attrs.size() && !isSpaceCh(attrs[e]) &&
           attrs[e] != '>') { // '>' is not allowed in the value
        ++e;
    }
    return attrs.substr(v, e - v);
}

} // namespace

bool matchHtmlTag(
    const std::string& s,
    size_t pos,
    HtmlTag& out
){
    if (pos >= s.size() || s[pos] != '<') {
        return false;
    }
    size_t p = pos + 1;
    bool closing = false;
    if (p < s.size() && s[p] == '/') {
        closing = true;
        ++p;
    }
    if (p >= s.size() || !std::isalpha(static_cast<unsigned char>(s[p]))) {
        return false;
    }
    const size_t nameStart = p;
    while (p < s.size() && (std::isalnum(static_cast<unsigned char>(s[p])) || s[p] == '-')) {
        ++p;
    }
    if (p < s.size() && !ut::isOneOf(s[p], ' ', '\t', '/', '>')) {
        return false;
    }
    const std::string attrs = readTagAttrs(s, p);
    if (p >= s.size()) {
        return false;
    }
    out = {toLower(s.substr(nameStart, p - nameStart)), closing, attrs, p + 1};
    return true;
}

std::string attrValue(
    const std::string& attrs,
    const std::string& name
){
    const std::string pat = toLower(name) + "=";
    const std::string low = toLower(attrs);
    size_t p = 0;
    while ((p = low.find(pat, p)) != std::string::npos) {
        // name boundary: "data-align"/"data-src" must not match as align/src
        const char prev = (p > 0) ? attrs[p - 1] : '\0';
        const bool boundary =
            prev == '\0' || (!isWordByte(static_cast<std::byte>(prev)) && prev != '-');
        if (boundary) {
            return readAttrValue(attrs, p + pat.size());
        }
        p += pat.size();
    }
    return {};
}

size_t findClosingTag(
    const std::string& s,
    size_t from,
    const std::string& name
){
    const std::string low = toLower(s);
    size_t p = low.find("</" + name, from);
    while (p != std::string::npos) {
        const size_t after = p + name.size() + 2;
        if (after < s.size() && ut::isOneOf(s[after], '>', ' ', '\t')) {
            const size_t gt = s.find('>', after);
            if (gt != std::string::npos) {
                return p;
            }
        }
        p = low.find("</" + name, p + 1);
    }
    return std::string::npos;
}

int styleTagKind(const std::string& name) {
    if (ut::isOneOf(name, "b", "strong")) {
        return 1;
    }
    if (ut::isOneOf(name, "i", "em")) {
        return 2;
    }
    if (name == "u") {
        return 3;
    }
    if (ut::isOneOf(name, "s", "strike", "del")) {
        return 4;
    }
    if (ut::isOneOf(name, "code", "kbd", "samp")) {
        return 5;
    }
    return 0;
}

StyleFlags withTagStyle(
    StyleFlags st,
    int kind
){
    switch (kind) {
    case 1:
        st.bold = true;
        break;
    case 2:
        st.italic = true;
        break;
    case 3:
        st.underline = true;
        break;
    case 4:
        st.strike = true;
        break;
    case 5:
        st.code = true;
        break;
    default:
        break;
    }
    return st;
}

} // namespace md
