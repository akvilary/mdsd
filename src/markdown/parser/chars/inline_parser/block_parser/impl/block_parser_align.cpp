// Block parser: alignment HTML wrappers <div/p/center align=...>.

#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

#include "algo/algo.hpp"
#include "markdown/parser/chars/chars.hpp"
#include "markdown/parser/chars/table_row/table_row.hpp"

namespace md::detail {

BlockPtr BlockParser::tryAlignWrapper(int depth) {
    const auto wrap = alignWrapper(mLines[mCursor]);
    if (!wrap) {
        return nullptr;
    }
    if (BlockPtr b = trySingleLineAlign(wrap->tag, wrap->align)) {
        return b;
    }
    return tryBlockAlign(*wrap, depth);
}

// Single-line variant: <div align=...>text</div>
BlockPtr BlockParser::trySingleLineAlign(const std::string& tag, Align align) {
    const std::string& line = mLines[mCursor];
    const size_t openEnd = line.find('>', indentOf(line));
    if (openEnd == std::string::npos) {
        return nullptr;
    }
    const std::string_view after = std::string_view(line).substr(openEnd + 1);
    const size_t cl = toLower(std::string(after)).find("</" + tag);
    if (cl == std::string::npos) {
        return nullptr;
    }
    const std::string one(trim(after.substr(0, cl)));
    ++mCursor;
    if (one.empty()) {
        return nullptr;
    }
    auto b = std::make_unique<Block>();
    b->type = Block::Type::Paragraph;
    b->align = align;
    b->inlines = parseInline(one);
    return b;
}

BlockPtr BlockParser::tryBlockAlign(const AlignWrap& wrap, int depth) {
    ++mCursor;
    std::vector<std::string> inner;
    const std::string closeTag = "</" + wrap.tag;
    while (mCursor < mLines.size()) {
        if (trim(mLines[mCursor]).starts_with(closeTag)) {
            ++mCursor;
            break;
        }
        if (isBlank(mLines[mCursor])) {
            break; // CommonMark: an HTML block ends at a blank line
        }
        inner.push_back(mLines[mCursor]);
        ++mCursor;
    }
    std::vector<BlockPtr> kids = parseInner(inner, depth + 1);
    applyAlign(kids, wrap.align);
    if (kids.empty()) {
        return nullptr;
    }
    for (size_t k = 1; k < kids.size(); ++k) {
        mPending.push_back(std::move(kids[k]));
    }
    return std::move(kids[0]);
}

std::optional<BlockParser::AlignWrap> BlockParser::alignWrapper(const std::string& line) {
    const size_t p = indentOf(line);
    if (p > 3 || p >= line.size() || line[p] != '<') {
        return std::nullopt;
    }
    for (const char* tag : {"div", "p", "center"}) {
        if (auto w = tryAlignTag(line, p, tag)) {
            return w;
        }
    }
    return std::nullopt;
}

std::optional<BlockParser::AlignWrap> BlockParser::tryAlignTag(const std::string& line, size_t p,
                                                               const char* tag) {
    const std::string open = std::string("<") + tag;
    if (!std::string_view(line).substr(p).starts_with(open)) {
        return std::nullopt;
    }
    const size_t after = p + open.size();
    if (after < line.size() && !ut::isOneOf(line[after], ' ', '\t', '>')) {
        return std::nullopt;
    }
    const size_t gt = line.find('>', after);
    if (gt == std::string::npos) {
        return std::nullopt;
    }
    const Align align =
        alignFromAttrs(line.substr(after, gt - after), std::string_view(tag) == "center");
    if (align == Align::None && std::string_view(tag) != "center") {
        return std::nullopt;
    }
    return AlignWrap{tag, align};
}

// The align= value; the attribute scanner is shared (html_tag::attrValue, guards against "data-align").
Align BlockParser::alignFromAttrs(const std::string& attrs, bool centerTag) {
    const std::string v = attrValue(attrs, "align");
    if (v.empty() && centerTag) {
        return Align::Center;
    }
    if (v == "center") {
        return Align::Center;
    }
    if (v == "right") {
        return Align::Right;
    }
    if (v == "justify") {
        return Align::Justify;
    }
    if (v == "left") {
        return Align::Left;
    }
    return Align::None;
}


void BlockParser::applyAlign(std::vector<BlockPtr>& blocks, Align align) {
    for (auto& b : blocks) {
        if (b->type == Block::Type::Paragraph) {
            b->align = align;
        }
    }
}

} // namespace md::detail
