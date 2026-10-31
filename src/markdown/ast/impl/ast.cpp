#include "markdown/ast/ast.hpp"

namespace md {

std::string plainText(const std::vector<InlineElement>& elems) {
    std::string out;
    for (const InlineElement& e : elems) {
        if (e.kind == InlineElement::Kind::Run) {
            out += e.run.text;
        } else {
            out += e.image.alt;
        }
    }
    return out;
}

std::vector<std::vector<InlineElement>> splitByLineBreaks(const std::vector<InlineElement>& elems) {
    std::vector<std::vector<InlineElement>> groups;
    std::vector<InlineElement> cur;
    auto flush = [&] {
        // drop empty runs before closing the group
        while (!cur.empty() && cur.back().kind == InlineElement::Kind::Run &&
               cur.back().run.text.empty()) {
            cur.pop_back();
        }
        if (!cur.empty()) {
            groups.push_back(std::move(cur));
            cur.clear();
        }
    };
    for (const InlineElement& e : elems) {
        if (e.kind == InlineElement::Kind::Run) {
            const std::string& t = e.run.text;
            size_t start = 0;
            while (true) {
                const size_t nl = t.find('\n', start);
                if (nl == std::string::npos) {
                    if (start < t.size() || start == 0) {
                        cur.push_back(
                            InlineElement{InlineElement::Kind::Run,
                                          InlineRun{t.substr(start), e.run.style, e.run.link},
                                          InlineImage{}});
                    }
                    break;
                }
                if (nl > start || !cur.empty()) {
                    cur.push_back(InlineElement{
                        InlineElement::Kind::Run,
                        InlineRun{t.substr(start, nl - start), e.run.style, e.run.link},
                        InlineImage{}});
                }
                flush();
                start = nl + 1;
            }
        } else {
            cur.push_back(e);
        }
    }
    flush();
    return groups;
}

std::vector<InlineElement> flattenLineBreaks(const std::vector<InlineElement>& elems) {
    std::vector<InlineElement> out;
    out.reserve(elems.size());
    for (const InlineElement& e : elems) {
        if (e.kind == InlineElement::Kind::Run) {
            InlineRun r = e.run;
            std::string t;
            t.reserve(r.text.size());
            for (char c : r.text) {
                t.push_back(c == '\n' ? ' ' : c);
            }
            r.text = std::move(t);
            out.push_back(InlineElement{InlineElement::Kind::Run, std::move(r), InlineImage{}});
        } else {
            out.push_back(e);
        }
    }
    return out;
}

} // namespace md
