// Block parser: comment, fence, ATX header, hr, indented code.

#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

#include "markdown/parser/chars/chars.hpp"
#include "markdown/parser/chars/detectors/detectors.hpp"

namespace md::detail {

BlockPtr BlockParser::tryComment() {
    if (!trim(mLines[mCursor]).starts_with("<!--")) {
        return nullptr;
    }
    const size_t rel = mLines[mCursor].find("-->", 4);
    if (rel != std::string::npos) {
        return skipRestOfComment(rel + 3);
    }
    ++mCursor;
    while (mCursor < mLines.size() && mLines[mCursor].find("-->") == std::string::npos) {
        ++mCursor;
    }
    if (mCursor < mLines.size()) {
        return skipRestOfComment(mLines[mCursor].find("-->") + 3);
    }
    return nullptr;
}

// The rest of the line after "-->" is content; insert it as a separate line
BlockPtr BlockParser::skipRestOfComment(size_t after) {
    const std::string rest(trim(mLines[mCursor].substr(after)));
    ++mCursor;
    if (!rest.empty()) {
        mLines.insert(mLines.begin() + static_cast<std::ptrdiff_t>(mCursor), rest);
    }
    return nullptr;
}

BlockPtr BlockParser::tryFence() {
    char fc = 0;
    size_t fl = 0;
    std::string info;
    if (!isFenceLine(mLines[mCursor], fc, fl, info)) {
        return nullptr;
    }
    const size_t fenceIndent = indentOf(mLines[mCursor]);
    ++mCursor;
    std::vector<std::string> body = collectFenceBody(fc, fl);
    if (mCursor < mLines.size()) {
        ++mCursor; // closing fence
    }
    auto b = std::make_unique<Block>();
    b->type = Block::Type::Code;
    b->lang = firstWord(info);
    b->codeText = dedentJoin(body, fenceIndent);
    return b;
}

std::vector<std::string> BlockParser::collectFenceBody(char fc, size_t fl) {
    std::vector<std::string> body;
    while (mCursor < mLines.size()) {
        char c2 = 0;
        size_t l2 = 0;
        std::string i2;
        if (isFenceLine(mLines[mCursor], c2, l2, i2) && c2 == fc && l2 >= fl &&
            trim(mLines[mCursor]).size() == l2) {
            break;
        }
        body.push_back(mLines[mCursor]);
        ++mCursor;
    }
    return body;
}

std::string BlockParser::firstWord(const std::string& s) {
    const size_t sp = s.find_first_of(" \t");
    return sp == std::string::npos ? s : s.substr(0, sp);
}

std::string BlockParser::dedentJoin(const std::vector<std::string>& body, size_t indent) {
    std::string out;
    for (size_t k = 0; k < body.size(); ++k) {
        if (k) {
            out += '\n';
        }
        out += body[k].substr(std::min(indentOf(body[k]), indent));
    }
    return out;
}

BlockPtr BlockParser::tryAtx() {
    const std::string& line = mLines[mCursor];
    const size_t p = indentOf(line);
    if (p > 3 || p >= line.size() || line[p] != '#') {
        return nullptr;
    }
    const size_t hashes = countRun(line, p, [](char c) { return c == '#'; }) - p;
    if (hashes > 6 || (p + hashes < line.size() && line[p + hashes] != ' ')) {
        return nullptr;
    }
    std::string text =
        stripTrailingHashes(std::string(trim(std::string_view(line).substr(p + hashes))));
    ++mCursor;
    auto b = std::make_unique<Block>();
    b->type = Block::Type::Header;
    b->level = static_cast<int>(hashes);
    b->inlines = parseInline(text);
    return b;
}

std::string BlockParser::stripTrailingHashes(std::string text) {
    size_t t = text.size();
    while (t > 0 && text[t - 1] == '#') {
        --t;
    }
    if (t < text.size()) {
        if (t == 0) {
            text.clear();
        } else if (text[t - 1] == ' ') {
            text = std::string(trim(std::string_view(text).substr(0, t)));
        }
    }
    return text;
}

BlockPtr BlockParser::tryHr() {
    if (!isHr(mLines[mCursor])) {
        return nullptr;
    }
    ++mCursor;
    auto b = std::make_unique<Block>();
    b->type = Block::Type::Rule;
    return b;
}

BlockPtr BlockParser::tryIndentedCode() {
    if (indentOf(mLines[mCursor]) < 4) {
        return nullptr;
    }
    std::vector<std::string> body = collectIndentedCodeBody();
    auto b = std::make_unique<Block>();
    b->type = Block::Type::Code;
    b->codeText = joinLines(body);
    return b;
}

std::vector<std::string> BlockParser::collectIndentedCodeBody() {
    std::vector<std::string> body;
    while (mCursor < mLines.size()) {
        if (isBlank(mLines[mCursor])) {
            if (nextNonBlankIndented()) {
                body.push_back("");
                ++mCursor;
                continue;
            }
            break;
        }
        if (indentOf(mLines[mCursor]) < 4) {
            break;
        }
        body.push_back(mLines[mCursor].substr(4));
        ++mCursor;
    }
    while (!body.empty() && isBlank(body.back())) {
        body.pop_back();
    }
    return body;
}

bool BlockParser::nextNonBlankIndented() const {
    size_t k = mCursor;
    while (k < mLines.size() && isBlank(mLines[k])) {
        ++k;
    }
    return k < mLines.size() && indentOf(mLines[k]) >= 4;
}

std::string BlockParser::joinLines(const std::vector<std::string>& body) {
    std::string out;
    for (size_t k = 0; k < body.size(); ++k) {
        if (k) {
            out += '\n';
        }
        out += body[k];
    }
    return out;
}

} // namespace md::detail
