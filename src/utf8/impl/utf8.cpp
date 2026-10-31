#include "utf8/utf8.hpp"

#include <utf8.h>

// Facade over utf8cpp (nemtrif/utfcpp): decoding and validation belong to the library;
// the u16 code unit semantics (String.length in JS) and the one-byte-step-on-error
// behavior are ours — this is the SabyDoc format contract and the point of truth.
namespace utf8 {

char32_t decode(const std::string& s, size_t& i, bool* ok) {
    if (ok) {
        *ok = true;
    }
    if (i >= s.size()) {
        if (ok) {
            *ok = false;
        }
        return 0xFFFD;
    }
    const size_t start = i;
    auto it = s.begin() + static_cast<std::ptrdiff_t>(i);
    try {
        const char32_t cp = utf8::next(it, s.end());
        i = static_cast<size_t>(it - s.begin());
        return cp;
    } catch (const utf8::exception&) {
        if (ok) {
            *ok = false;
        }
        i = start + 1; // broken sequence: +1 byte, replacement character
        return 0xFFFD;
    }
}

int u16len(char32_t cp) { return cp >= 0x10000 ? 2 : 1; }

size_t u16length(const std::string& s) {
    size_t n = 0;
    size_t i = 0;
    while (i < s.size()) {
        n += static_cast<size_t>(u16len(decode(s, i)));
    }
    return n;
}

size_t sanitize(std::string& s) {
    size_t replaced = 0;
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        const size_t start = i;
        bool ok = false;
        decode(s, i, &ok);
        if (ok) {
            out.append(s, start, i - start);
        } else {
            out += "\xEF\xBF\xBD"; // U+FFFD
            ++replaced;
        }
    }
    if (replaced != 0) {
        s = std::move(out);
    }
    return replaced;
}

} // namespace utf8
