// Inline parser: traversal skeleton, escapes, code spans, entities.

#include "markdown/parser/chars/inline_parser/inline_parser.hpp"

#include "markdown/parser/chars/chars.hpp"

namespace md::detail {

void InlineParser::warn(const std::string& msg) const { mWarnings.push_back(msg); }

void InlineParser::emitRun(const Ctx& c, std::vector<InlineElement>& out, std::string& buf) const {
    if (!buf.empty()) {
        out.push_back(
            InlineElement{InlineElement::Kind::Run, InlineRun{buf, c.st, c.link}, InlineImage{}});
        buf.clear();
    }
}

std::vector<InlineElement> InlineParser::parse(const std::string& s) {
    const Ctx ctx{s, StyleFlags{}, std::string{}, 0};
    return mergeRuns(parseRec(ctx));
}

size_t InlineParser::tryEscape(
    const Ctx& c,
    size_t i,
    std::string& buf
){
    if (i + 1 < c.s.size() && isPunct(c.s[i + 1])) {
        buf += c.s[i + 1];
        return 2; // backslash + escaped character
    }
    return 0;
}

size_t InlineParser::tryCodeSpan(const Ctx& c,
                                 size_t i,
                                 std::vector<InlineElement>& out,
                                 std::string& buf) const {
    const size_t n = countRun(c.s, i, [](char ch) { return ch == '`'; }) - i;
    const size_t close = findCodeSpanClose(c.s, i + n, n);
    if (close == std::string::npos) { // no matching fence of the same length — literal
        buf.append(
            n,
            '`'
        );
        return n;
    }
    std::string content = c.s.substr(i + n, close - (i + n));
    if (needsSpaceTrim(content)) { // trim one space at both edges (CommonMark)
        content = content.substr(1, content.size() - 2);
    }
    emitRun(
        c,
        out,
        buf
    );
    StyleFlags cs = c.st;
    cs.code = true;
    out.push_back(
        InlineElement{InlineElement::Kind::Run, InlineRun{content, cs, ""}, InlineImage{}});
    return close + n - i;
}

size_t InlineParser::findCodeSpanClose(
    const std::string& s,
    size_t from,
    size_t n
){
    size_t j = from;
    while (j < s.size()) {
        if (s[j] == '`') {
            const size_t m = countRun(s, j, [](char ch) { return ch == '`'; }) - j;
            if (m == n) {
                return j;
            }
            j += m; // a fence of a different length does not close the span
        } else {
            ++j;
        }
    }
    return std::string::npos;
}

bool InlineParser::needsSpaceTrim(const std::string& content) {
    return content.size() >= 2 && content.front() == ' ' && content.back() == ' ' &&
           content.find_first_not_of(' ') != std::string::npos;
}

size_t InlineParser::tryEntity(const Ctx& c, size_t i, std::string& buf) const {
    size_t end = 0;
    std::string decoded;
    if (decodeEntity(c.s, i, end, decoded)) {
        buf += decoded;
        return end - i;
    }
    return 0;
}

// Main traversal: dispatch on the current character; a try* handler returns how
// many characters it consumed (0 = not its construct, the character goes to the buffer).
std::vector<InlineElement> InlineParser::parseRec(const Ctx& c) const {
    std::vector<InlineElement> out;
    if (c.depth > kMaxNestingDepth) {
        warn("inline nesting too deep — text left as-is");
        if (!c.s.empty()) {
            out.push_back(InlineElement{InlineElement::Kind::Run,
                                        InlineRun{c.s, c.st, c.link},
                                        InlineImage{}});
        }
        return out;
    }
    std::string buf;
    size_t i = 0;
    while (i < c.s.size()) {
        const char ch = c.s[i];
        size_t used = 0;
        if (ch == '\\') {
            used = tryEscape(c, i, buf);
        } else if (ch == '`') {
            used = tryCodeSpan(c, i, out, buf);
        } else if (ch == '!' && i + 1 < c.s.size() && c.s[i + 1] == '[') {
            used = tryImage(c, i, out, buf);
        } else if (ch == '[') {
            used = tryLink(c, i, out, buf);
        } else if (ch == '<') {
            used = tryAngle(c, i, out, buf);
        } else if (ch == '&') {
            used = tryEntity(c, i, buf);
        } else if (ch == 'h') {
            used = tryBareUrl(c, i, out, buf);
        } else if (ut::isOneOf(ch, '*', '_', '~')) {
            used = tryEmphasis(c, i, out, buf);
        }
        if (used != 0) {
            i += used;
            continue;
        }
        buf += ch;
        ++i;
    }
    emitRun(
        c,
        out,
        buf
    );
    return out;
}

std::vector<InlineElement> InlineParser::mergeRuns(std::vector<InlineElement> elems) {
    std::vector<InlineElement> out;
    out.reserve(elems.size());
    for (auto& e : elems) {
        const bool sameStyle = !out.empty() && out.back().kind == InlineElement::Kind::Run &&
                               e.kind == InlineElement::Kind::Run &&
                               out.back().run.style == e.run.style &&
                               out.back().run.link == e.run.link;
        if (sameStyle) {
            out.back().run.text += e.run.text;
        } else {
            out.push_back(std::move(e));
        }
    }
    while (!out.empty() && out.back().kind == InlineElement::Kind::Run &&
           out.back().run.text.empty()) {
        out.pop_back();
    }
    return out;
}

} // namespace md::detail
