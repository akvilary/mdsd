// Инлайновый парсер: ссылки, картинки по ссылкам, голые URL.

#include "markdown/parser/chars/inline_parser/inline_parser.hpp"

#include "markdown/parser/chars/chars.hpp"

namespace md::detail {

bool InlineParser::lookupDef(const std::string& rawLabel, LinkDef& out) const {
    const auto it = mDefs.find(normalizeLabel(rawLabel));
    if (it == mDefs.end()) {
        return false;
    }
    out = it->second;
    return true;
}

// Цель ссылки: (адрес) / [ref] / [ref][] / collapsed [ref]. false — не ссылка.
bool InlineParser::resolveTarget(const std::string& s, size_t close, const std::string& inner,
                                 std::string& url, std::string& title, size_t& after) const {
    if (close + 1 < s.size() && s[close + 1] == '(' &&
        parseDestination(s, close + 1, url, title, after)) {
        return true;
    }
    std::string label = inner;
    after = close + 1;
    if (close + 1 < s.size() && s[close + 1] == '[') {
        const size_t rc = matchBracket(s, close + 1);
        if (rc == std::string::npos) {
            return false;
        }
        label = s.substr(close + 2, rc - close - 2);
        if (label.empty()) {
            label = inner; // collapsed [label][]
        }
        after = rc + 1;
    }
    LinkDef def;
    if (!lookupDef(label, def)) {
        return false;
    }
    url = def.url;
    title = def.title;
    return true;
}

size_t InlineParser::tryImage(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                              std::string& buf) const {
    const size_t close = matchBracket(c.s, i + 1);
    if (close == std::string::npos) {
        return 0;
    }
    const std::string alt = decodeEntities(c.s.substr(i + 2, close - i - 2));
    std::string url, title;
    size_t after = 0;
    if (!resolveTarget(c.s, close, alt, url, title, after) || url.empty()) {
        return 0;
    }
    InlineImage img;
    img.url = url;
    img.alt = alt;
    img.title = title;
    emitRun(c, out, buf);
    out.push_back(InlineElement{InlineElement::Kind::Image, InlineRun{}, std::move(img)});
    return after - i;
}

size_t InlineParser::tryLink(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                             std::string& buf) const {
    const size_t close = matchBracket(c.s, i);
    if (close == std::string::npos) {
        return 0;
    }
    const std::string inner = c.s.substr(i + 1, close - i - 1);
    std::string url, title;
    size_t after = 0;
    if (!resolveTarget(c.s, close, inner, url, title, after)) {
        return 0;
    }
    emitRun(c, out, buf);
    const Ctx innerCtx{inner, c.st, url, c.depth + 1};
    emitAll(c, out, buf, parseRec(innerCtx));
    return after - i;
}

size_t InlineParser::tryBareUrl(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                                std::string& buf) const {
    const std::string_view rest(c.s.data() + i, c.s.size() - i);
    if (!rest.starts_with("http://") && !rest.starts_with("https://")) {
        return 0;
    }
    const char prev = (i > 0) ? c.s[i - 1] : '\0';
    if (prev != '\0' && isWordByte(static_cast<unsigned char>(prev))) { // не в середине слова
        return 0;
    }
    size_t end = i;
    while (end < c.s.size() && !ut::isOneOf(c.s[end], ' ', '\t', '\n', '<', '>', '"', '\'')) {
        ++end;
    }
    const auto [url, consumedTo] = trimUrlPunctuation(c.s, i, end);
    if (url.size() <= 8 || url.find('.') == std::string::npos) {
        return 0;
    }
    emitRun(c, out, buf);
    out.push_back(
        InlineElement{InlineElement::Kind::Run, InlineRun{url, c.st, url}, InlineImage{}});
    return consumedTo - i;
}

// Убираем хвостовую пунктуацию и лишние непарные ')'
std::pair<std::string, size_t> InlineParser::trimUrlPunctuation(const std::string& s, size_t from,
                                                                size_t to) {
    size_t t = to;
    while (t > from && std::string_view(".,;:!?").find(s[t - 1]) != std::string_view::npos) {
        --t;
    }
    std::string url = s.substr(from, t - from);
    int bal = 0;
    for (char ch : url) {
        bal += (ch == '(') ? 1 : (ch == ')') ? -1 : 0;
    }
    while (bal < 0 && !url.empty() && url.back() == ')') {
        url.pop_back();
        ++bal;
        --t;
    }
    return {std::move(url), t};
}

} // namespace md::detail
