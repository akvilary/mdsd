#include "markdown/parser/chars/entity/entity.hpp"

#include <iterator>
#include <map>

#include <utf8.h>

#include "algo/algo.hpp"
#include "markdown/parser/chars/chars.hpp"

namespace md {

namespace {

bool decodeNumericEntity(
    const std::string& body,
    std::string& out,
    size_t& end,
    size_t sc
){
    unsigned long cp = 0;
    try {
        if (body.size() > 1 && ut::isOneOf(body[1], 'x', 'X')) {
            if (body.size() <= 2) {
                return false;
            }
            cp = std::stoul(body.substr(2), nullptr, 16);
        } else {
            if (body.size() <= 1) {
                return false;
            }
            cp = std::stoul(body.substr(1));
        }
    } catch (...) {
        return false;
    }
    if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return false;
    }
    end = sc + 1;
    utf8::append(static_cast<char32_t>(cp),
                 std::back_inserter(out)); // encoding is done by the library
    return true;
}

bool decodeNamedEntity(
    const std::string& body,
    std::string& out,
    size_t& end,
    size_t sc
){
    static const std::map<std::string, std::string> kMap = {
        {"amp", "&"},
        {"lt", "<"},
        {"gt", ">"},
        {"quot", "\""},
        {"apos", "'"},
        {"nbsp", "\xC2\xA0"},
        {"copy", "\xC2\xA9"},
        {"reg", "\xC2\xAE"},
        {"trade", "\xE2\x84\xA2"},
        {"mdash", "\xE2\x80\x94"},
        {"ndash", "\xE2\x80\x93"},
        {"hellip", "\xE2\x80\xA6"},
        {"laquo", "\xC2\xAB"},
        {"raquo", "\xC2\xBB"},
        {"times", "\xC3\x97"},
        {"divide", "\xC3\xB7"},
        {"middot", "\xC2\xB7"},
        {"bull", "\xE2\x80\xA2"},
        {"deg", "\xC2\xB0"},
        {"plusmn", "\xC2\xB1"},
        {"para", "\xC2\xB6"},
        {"sect", "\xC2\xA7"},
        {"prime", "\xE2\x80\xB2"},
        {"euro", "\xE2\x82\xAC"},
        {"pound", "\xC2\xA3"},
        {"yen", "\xC2\xA5"},
        {"cent", "\xC2\xA2"},
        {"szlig", "\xC3\x9F"},
        {"sup2", "\xC2\xB2"},
        {"sup3", "\xC2\xB3"},
        {"frac12", "\xC2\xBD"},
        {"iquest", "\xC2\xBF"},
        {"iexcl", "\xC2\xA1"},
        {"larr", "\xE2\x86\x90"},
        {"rarr", "\xE2\x86\x92"},
        {"harr", "\xE2\x86\x94"},
        {"ne", "\xE2\x89\xA0"},
        {"le", "\xE2\x89\xA4"},
        {"ge", "\xE2\x89\xA5"},
        {"infin", "\xE2\x88\x9E"},
    };
    const auto it = kMap.find(toLower(body));
    if (it == kMap.end()) {
        return false;
    }
    end = sc + 1;
    out = it->second;
    return true;
}

} // namespace

bool decodeEntity(
    const std::string& s,
    size_t pos,
    size_t& end,
    std::string& out
){
    const size_t sc = s.find(';', pos + 1);
    if (sc == std::string::npos || sc - pos > 12 || sc == pos + 1) {
        return false;
    }
    const std::string body = s.substr(pos + 1, sc - pos - 1);
    if (body.front() == '#') {
        return decodeNumericEntity(body, out, end, sc);
    }
    return decodeNamedEntity(body, out, end, sc);
}

std::string decodeEntities(const std::string& s) {
    std::string out;
    size_t i = 0;
    while (i < s.size()) {
        size_t end = 0;
        std::string decoded;
        if (s[i] == '&' && decodeEntity(s, i, end, decoded)) {
            out += decoded;
            i = end;
        } else {
            out += s[i++];
        }
    }
    return out;
}

} // namespace md
