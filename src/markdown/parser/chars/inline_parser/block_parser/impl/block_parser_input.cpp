// Block parser: input preparation — normalization, front matter, line splitting,
// link-definition prescan.

#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

#include "algo/algo.hpp"
#include "utf8/utf8.hpp"
#include "markdown/parser/chars/chars.hpp"

namespace md::detail {

Document BlockParser::run(std::string text) {
    text = normalize(std::move(text));
    const std::string title = extractFrontMatter(&text);
    mLines = splitLines(text);
    prescanLinkDefs();
    Document doc;
    doc.title = title;
    doc.blocks = parseBlocks(0);
    return doc;
}

std::string BlockParser::normalize(std::string s) {
    if (s.starts_with("\xEF\xBB\xBF")) {
        s.erase(
            0,
            3
        );
    }
    std::string noCr;
    noCr.reserve(s.size());
    for (size_t k = 0; k < s.size(); ++k) {
        if (s[k] == '\r') {
            if (k + 1 < s.size() && s[k + 1] == '\n') {
                ++k;
            }
            noCr += '\n';
        } else {
            noCr += s[k];
        }
    }
    return expandLeadingTabs(noCr);
}

// Leading tabs -> spaces up to the next stop of 4; inner tabs are left untouched (code!)
std::string BlockParser::expandLeadingTabs(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool leading = true;
    size_t col = 0;
    for (const char ch : s) {
        if (leading && ch == '\t') {
            const size_t stop = ((col / 4) + 1) * 4;
            out.append(
                stop - col,
                ' '
            );
            col = stop;
            continue;
        }
        out += ch;
        if (ch == '\n') {
            leading = true;
            col = 0;
        } else {
            leading = leading && ch == ' ';
            ++col;
        }
    }
    return out;
}

std::string BlockParser::extractFrontMatter(std::string* text) {
    if (!text->starts_with("---\n")) {
        return {};
    }
    const size_t end = text->find("\n---", 3);
    if (end == std::string::npos) {
        return {};
    }
    const size_t closeEnd = end + 4;
    if (closeEnd < text->size() && (*text)[closeEnd] != '\n') {
        return {};
    }
    const std::string title = frontMatterTitle(text->substr(4, end - 4));
    *text = (closeEnd < text->size()) ? text->substr(closeEnd + 1) : std::string();
    return title;
}

std::string BlockParser::frontMatterTitle(const std::string& fm) {
    for (const std::string_view part : splitBy(fm, '\n')) {
        const std::string_view ln(trim(part));
        if (toLower(std::string(ln)).starts_with("title:")) {
            return unquote(ln.substr(6));
        }
    }
    return {};
}

std::string BlockParser::unquote(std::string_view v) {
    v = trim(v);
    if (v.size() >= 2 && ut::isOneOf(v.front(), '"', '\'') && v.back() == v.front()) {
        v = v.substr(1, v.size() - 2);
    }
    return std::string(trim(v));
}

std::vector<std::string> BlockParser::splitLines(const std::string& text) {
    std::vector<std::string> lines;
    for (const std::string_view part : splitBy(text, '\n')) {
        lines.emplace_back(part);
    }
    return lines;
}

void BlockParser::prescanLinkDefs() {
    bool inFence = false;
    char fenceChar = 0;
    size_t fenceLen = 0;
    for (std::string& line : mLines) {
        char fc = 0;
        size_t fl = 0;
        std::string info;
        if (isFenceLine(line, fc, fl, info)) {
            if (!inFence) {
                inFence = true;
                fenceChar = fc;
                fenceLen = fl;
            } else if (fc == fenceChar && fl >= fenceLen) {
                inFence = false;
            }
        } else if (!inFence) {
            std::string label;
            LinkDef def;
            if (matchLinkDef(line, label, def)) {
                mDefs[label] = def;
                line.clear();
            }
        }
    }
}

} // namespace md::detail
