// Inline parser: emphasis, '<' (comments/autolinks/HTML tags).

#include "markdown/parser/chars/inline_parser/inline_parser.hpp"

#include "markdown/parser/chars/chars.hpp"

namespace md::detail {

size_t InlineParser::tryEmphasis(const Ctx& c,
                                 size_t i,
                                 std::vector<InlineElement>& out,
                                 std::string& buf) const {
    const auto attempt = [&](std::string_view tok, StyleFlags ns, bool wordRule) -> size_t {
        if (!canOpenEmphasis(c.s, i, tok.size(), wordRule)) {
            return 0;
        }
        const size_t close = findEmphasisClose(c.s, i + tok.size(), tok, wordRule);
        if (close == std::string::npos) {
            return 0;
        }
        emitRun(
            c,
            out,
            buf
        );
        const std::string inner = c.s.substr(i + tok.size(), close - i - tok.size());
        const Ctx innerCtx{inner, ns, c.link, c.depth + 1};
        emitAll(
            c,
            out,
            buf,
            parseRec(innerCtx)
        );
        return close + tok.size() - i;
    };

    StyleFlags b = c.st;
    b.bold = true;
    StyleFlags bi = b;
    bi.italic = true;
    StyleFlags it = c.st;
    it.italic = true;
    StyleFlags sk = c.st;
    sk.strike = true;

    const std::string_view rest(
        c.s.data() + i,
        c.s.size() - i
    );
    if (size_t u = 0; rest.starts_with("***") && (u = attempt("***", bi, false)) != 0) {
        return u;
    }
    if (size_t u = 0; rest.starts_with("**") && (u = attempt("**", b, false)) != 0) {
        return u;
    }
    if (size_t u = 0; rest.starts_with("__") && (u = attempt("__", b, true)) != 0) {
        return u;
    }
    if (size_t u = 0; rest.starts_with("~~") && (u = attempt("~~", sk, false)) != 0) {
        return u;
    }
    if (size_t u = 0; c.s[i] == '*' && (i + 1 >= c.s.size() || c.s[i + 1] != '*') &&
                      (u = attempt(
                          "*",
                          it,
                          false)) != 0
                      ){
        return u;
    }
    if (size_t u = 0; c.s[i] == '_' && (i + 1 >= c.s.size() || c.s[i + 1] != '_') &&
                      (u = attempt(
                          "_",
                          it,
                          true)) != 0
                      ){
        return u;
    }
    return 0;
}

// Opener: after the token there is no end/space; for _ and __ the char before is not a letter
bool InlineParser::canOpenEmphasis(
    const std::string& s,
    size_t i,
    size_t tokLen,
    bool wordRule
){
    if (i + tokLen >= s.size()) {
        return false;
    }
    if (ut::isOneOf(s[i + tokLen], ' ', '\t')) {
        return false;
    }
    if (wordRule) {
        const char prev = (i > 0) ? s[i - 1] : '\0';
        if (prev != '\0' && isWordByte(static_cast<unsigned char>(prev))) {
            return false;
        }
    }
    return true;
}

// Closer: the char before is not a space; for _ and __ the char after is not a letter
size_t InlineParser::findEmphasisClose(const std::string& s,
                                       size_t from,
                                       std::string_view tok,
                                       bool wordRule) {
    size_t i = from;
    while (i < s.size()) {
        if (s[i] == '\\') { // an escaped delimiter does not count
            i += 2;
            continue;
        }
        if (s.compare(i, tok.size(), tok) == 0) {
            const char prev = (i > 0) ? s[i - 1] : '\0';
            const char next = (i + tok.size() < s.size()) ? s[i + tok.size()] : '\0';
            if (prev != ' ' && prev != '\t' && prev != '\0' &&
                (!wordRule || next == '\0' || !isWordByte(static_cast<unsigned char>(next)))) {
                return i;
            }
        }
        ++i;
    }
    return std::string::npos;
}

size_t InlineParser::tryAngle(const Ctx& c,
                              size_t i,
                              std::vector<InlineElement>& out,
                              std::string& buf) const {
    if (size_t u = tryComment(c.s, i); u != 0) {
        return u;
    }
    if (size_t u = tryAutolink(c, i, out, buf); u != 0) {
        return u;
    }
    return tryHtmlTag(c, i, out, buf);
}

size_t InlineParser::tryComment(
    const std::string& s,
    size_t i
){
    if (!std::string_view(s).substr(i).starts_with("<!--")) {
        return 0;
    }
    const size_t end = s.find("-->", i + 4);
    return (end == std::string::npos) ? s.size() - i : end + 3 - i;
}

size_t InlineParser::tryAutolink(const Ctx& c,
                                 size_t i,
                                 std::vector<InlineElement>& out,
                                 std::string& buf) const {
    const size_t gt = c.s.find('>', i + 1);
    if (gt == std::string::npos) {
        return 0;
    }
    const std::string inner = c.s.substr(i + 1, gt - i - 1);
    if (inner.empty() || inner.find_first_of(" \t\n<>") != std::string::npos) {
        return 0;
    }
    const std::string url = autolinkUrl(inner);
    if (url.empty()) {
        return 0;
    }
    emitRun(
        c,
        out,
        buf
    );
    out.push_back(
        InlineElement{InlineElement::Kind::Run, InlineRun{inner, c.st, url}, InlineImage{}});
    return gt + 1 - i;
}

std::string InlineParser::autolinkUrl(const std::string& inner) {
    const size_t scheme = inner.find("://");
    if (scheme != std::string::npos && scheme >= 2) {
        return inner;
    }
    if (inner.starts_with("mailto:")) {
        return inner;
    }
    if (inner.find(':') == std::string::npos) { // looks like an e-mail
        const size_t at = inner.find('@');
        const size_t dot = (at == std::string::npos) ? std::string::npos : inner.find('.', at);
        if (at > 0 && at != std::string::npos && dot != std::string::npos && dot > at + 1) {
            return "mailto:" + inner;
        }
    }
    return {};
}

size_t InlineParser::tryHtmlTag(const Ctx& c,
                                size_t i,
                                std::vector<InlineElement>& out,
                                std::string& buf) const {
    HtmlTag tag;
    if (!matchHtmlTag(c.s, i, tag)) {
        return 0;
    }
    if (tag.name == "br") {
        buf += '\n'; // hard line break
        return tag.end - i;
    }
    if (tag.name == "img") {
        return tryImgTag(c, i, tag, out, buf);
    }
    if (tag.closing) {
        return tag.end - i; // a lone closing tag — skip it
    }
    const int kind = styleTagKind(tag.name);
    if (kind != 0) {
        if (size_t u = tryStyledTag(c, i, tag, kind, out, buf); u != 0) {
            return u;
        }
    }
    // unclosed paired / transparent / unknown tags: drop the tag itself
    return tag.end - i;
}

size_t InlineParser::tryImgTag(const Ctx& c,
                               size_t i,
                               const HtmlTag& tag,
                               std::vector<InlineElement>& out,
                               std::string& buf) const {
    const std::string src = attrValue(tag.attrs, "src");
    if (!src.empty()) {
        InlineImage img;
        img.url = src;
        img.alt = decodeEntities(attrValue(tag.attrs, "alt"));
        img.title = attrValue(tag.attrs, "title");
        emitRun(
            c,
            out,
            buf
        );
        out.push_back(InlineElement{InlineElement::Kind::Image, InlineRun{}, std::move(img)});
    }
    return tag.end - i;
}

size_t InlineParser::tryStyledTag(const Ctx& c,
                                  size_t i,
                                  const HtmlTag& tag,
                                  int kind,
                                  std::vector<InlineElement>& out,
                                  std::string& buf) const {
    const size_t close = findClosingTag(c.s, tag.end, tag.name);
    if (close == std::string::npos) {
        return 0; // unclosed paired tag — the caller will skip (the tag itself)
    }
    const std::string inner = c.s.substr(tag.end, close - tag.end);
    emitRun(
        c,
        out,
        buf
    );
    const std::string href = (tag.name == "a") ? attrValue(tag.attrs, "href") : c.link;
    const Ctx innerCtx{inner, withTagStyle(c.st, kind), href, c.depth + 1};
    emitAll(
        c,
        out,
        buf,
        parseRec(innerCtx)
    );
    const size_t gt = c.s.find('>', close);
    return ((gt == std::string::npos) ? c.s.size() : gt + 1) - i;
}

} // namespace md::detail
