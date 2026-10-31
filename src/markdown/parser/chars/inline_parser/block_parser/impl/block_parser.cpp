// Block parser: traversal skeleton — inline bridge, block loop, parseOne dispatcher,
// recursive parsing of a line subset.

#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

#include "algo/algo.hpp"
#include "utf8/utf8.hpp"
#include "markdown/parser/chars/chars.hpp"

namespace md::detail {

std::vector<InlineElement> BlockParser::parseInline(const std::string& s) {
    InlineParser p(mDefs, mWarnings);
    return p.parse(s);
}

std::vector<BlockPtr> BlockParser::parseBlocks(int depth) {
    std::vector<BlockPtr> out;
    if (depth > kMaxNestingDepth) {
        mWarnings.push_back("nesting too deep — some content skipped");
        return out;
    }
    while (mCursor < mLines.size()) {
        if (isBlank(mLines[mCursor])) {
            ++mCursor;
            continue;
        }
        drainPending(parseOne(depth), out);
    }
    return out;
}

void BlockParser::drainPending(BlockPtr b, std::vector<BlockPtr>& out) {
    while (true) {
        if (b) {
            out.push_back(std::move(b));
        }
        if (mPending.empty()) {
            return;
        }
        b = std::move(mPending.front());
        mPending.erase(mPending.begin());
    }
}

BlockPtr BlockParser::parseOne(int depth) {
    if (BlockPtr b = tryComment()) {
        return b;
    }
    if (BlockPtr b = tryFence()) {
        return b;
    }
    if (BlockPtr b = tryAtx()) {
        return b;
    }
    if (BlockPtr b = tryAlignWrapper(depth)) {
        return b;
    }
    if (BlockPtr b = tryTableStart()) {
        return b;
    }
    if (BlockPtr b = tryQuote(depth)) {
        return b;
    }
    if (BlockPtr b = tryHr()) {
        return b;
    }
    if (BlockPtr b = tryList(depth)) {
        return b;
    }
    if (BlockPtr b = tryIndentedCode()) {
        return b;
    }
    return parseParagraph();
}

std::vector<BlockPtr> BlockParser::parseInner(const std::vector<std::string>& inner, int depth) {
    std::vector<std::string> saved = std::move(mLines);
    const size_t savedI = mCursor;
    mLines = inner;
    mCursor = 0;
    std::vector<BlockPtr> out = parseBlocks(depth);
    mLines = std::move(saved);
    mCursor = savedI;
    return out;
}

} // namespace md::detail
