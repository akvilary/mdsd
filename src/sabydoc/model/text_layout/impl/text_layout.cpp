#include "sabydoc/model/text_layout/text_layout.hpp"

#include "utf8/utf8.hpp"

namespace sd {

namespace {

struct Signature {
    bool bold = false, italic = false, strike = false, underline = false;
    const std::string* href = nullptr;
    bool operator==(const Signature& o) const {
        return bold == o.bold && italic == o.italic && strike == o.strike &&
               underline == o.underline &&
               ((href == nullptr && o.href == nullptr) ||
                (href != nullptr && o.href != nullptr && *href == *o.href));
    }
};

Signature signatureOf(const md::InlineRun& r) {
    Signature s;
    s.bold = r.style.bold;
    s.italic = r.style.italic;
    s.strike = r.style.strike;
    s.underline = r.style.underline;
    if (!r.link.empty()) {
        s.href = &r.link;
    }
    return s;
}

} // namespace

Text buildText(const std::vector<md::InlineElement>& elems) {
    // Input normalization: image -> alt/url, '\n' -> ' '
    std::vector<md::InlineRun> runs;
    runs.reserve(elems.size());
    for (const md::InlineElement& e : elems) {
        if (e.kind == md::InlineElement::Kind::Run) {
            if (e.run.text.empty()) {
                continue;
            }
            std::string t;
            t.reserve(e.run.text.size());
            for (char c : e.run.text) {
                t.push_back(c == '\n' ? ' ' : c);
            }
            runs.push_back({t, e.run.style, e.run.link});
        } else {
            const std::string alt = e.image.alt.empty() ? e.image.url : e.image.alt;
            if (!alt.empty()) {
                runs.push_back({alt, md::StyleFlags{}, std::string()});
            }
        }
    }

    Text out;
    if (runs.empty()) {
        return out; // empty text
    }

    out.content.reserve(64);
    out.formats.reserve(runs.size() + 1);

    const Signature emptySig;
    Signature cur = emptySig;
    bool open = false;
    size_t rangeStart = 0;

    auto closeRange = [&](size_t endU16) {
        if (open && endU16 > rangeStart) {
            FormatRange r;
            r.offset = rangeStart;
            r.length = endU16 - rangeStart;
            r.bold = cur.bold;
            r.italic = cur.italic;
            r.strike = cur.strike;
            r.underline = cur.underline;
            if (cur.href) {
                r.href = *cur.href;
            }
            out.formats.push_back(r);
        }
        open = false;
    };

    size_t pos16 = 0;
    for (const md::InlineRun& run : runs) {
        const Signature sig = signatureOf(run);
        size_t i = 0;
        while (i < run.text.size()) {
            const bool ok = true;
            (void)ok;
            const size_t start = i;
            const char32_t cp = utf8::decode(run.text, i);
            if (sig != cur || !open) {
                closeRange(pos16);
                cur = sig;
                open = true;
                rangeStart = pos16;
            }
            out.content.append(run.text, start, i - start);
            pos16 += static_cast<size_t>(utf8::u16len(cp));
        }
    }
    closeRange(pos16);

    if (out.formats.empty()) {
        out.formats.push_back(FormatRange{0, pos16, false, false, false, false, std::string()});
    }
    return out;
}

Text plainText(const std::string& s) {
    if (s.empty()) {
        return Text{};
    }
    std::vector<md::InlineElement> elems;
    elems.push_back(md::InlineElement{md::InlineElement::Kind::Run,
                                      md::InlineRun{s, md::StyleFlags{}, ""}, md::InlineImage{}});
    return buildText(elems);
}

Text forceBold(Text t) {
    for (FormatRange& r : t.formats) {
        r.bold = true;
    }
    return t;
}

} // namespace sd
