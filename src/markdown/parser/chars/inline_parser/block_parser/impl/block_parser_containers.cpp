// Block parser: quotes and lists (including the lazy-continuation guard).

#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

#include "algo/algo.hpp"
#include "markdown/parser/chars/chars.hpp"
#include "markdown/parser/chars/detectors/detectors.hpp"
#include "markdown/parser/chars/marker/marker.hpp"

namespace md::detail {

BlockPtr BlockParser::tryQuote(int depth) {
    if (!isQuoteStart(mLines[mCursor])) {
        return nullptr;
    }
    std::vector<std::string> qt = collectQuoteBody();
    auto b = std::make_unique<Block>();
    b->type = Block::Type::Quote;
    b->children = parseInner(qt, depth + 1);
    return b;
}

std::vector<std::string> BlockParser::collectQuoteBody() {
    std::vector<std::string> qt;
    bool lastWasText = false;
    while (mCursor < mLines.size()) {
        const std::string& line = mLines[mCursor];
        if (isBlank(line)) {
            if (!continueQuoteAfterBlank()) {
                break;
            }
            qt.push_back("");
            ++mCursor;
            continue;
        }
        if (isQuoteStart(line)) {
            const std::string content = stripQuoteMarker(line);
            qt.push_back(content);
            lastWasText = !trim(content).empty();
            ++mCursor;
            continue;
        }
        if (lastWasText && !isBlockStarter(line)) { // lazy continuation — only after text
            qt.push_back(rtrimCopy(line));
            ++mCursor;
            continue;
        }
        break;
    }
    return qt;
}

bool BlockParser::continueQuoteAfterBlank() const {
    size_t k = mCursor;
    while (k < mLines.size() && isBlank(mLines[k])) {
        ++k;
    }
    return k < mLines.size() && isQuoteStart(mLines[k]);
}

std::string BlockParser::stripQuoteMarker(const std::string& line) {
    const size_t p = indentOf(line);
    size_t after = p + 1;
    if (after < line.size() && line[after] == ' ') {
        ++after;
    }
    return line.substr(after);
}

bool BlockParser::isBlockStarter(const std::string& line) const {
    char fc = 0;
    size_t fl = 0;
    std::string info;
    return isAtx(line) || isHr(line) || isQuoteStart(line) || isFenceLine(line, fc, fl, info) ||
           matchListMarker(line).ok || alignWrapper(line).has_value();
}

BlockPtr BlockParser::tryList(int depth) {
    const Marker m = matchListMarker(mLines[mCursor]);
    if (!m.ok) {
        return nullptr;
    }
    auto b = std::make_unique<Block>();
    b->type = Block::Type::List;
    bool anyTask = false;
    while (const auto item = peekListItem(m.ordered)) {
        ++mCursor;
        anyTask = makeListItem(gatherItem(*item), b->items, depth) || anyTask;
    }
    b->listKind = anyTask ? ListKind::Task : (m.ordered ? ListKind::Ordered : ListKind::Bullet);
    return b;
}

// An item of the current kind at mCursor (blank lines skipped); advances mCursor onto the marker.
std::optional<Marker> BlockParser::peekListItem(bool ordered) {
    while (mCursor < mLines.size() && isBlank(mLines[mCursor])) {
        ++mCursor;
    }
    if (mCursor >= mLines.size()) {
        return std::nullopt;
    }
    const Marker m = matchListMarker(mLines[mCursor]);
    if (!m.ok || m.ordered != ordered) {
        return std::nullopt;
    }
    return m;
}

std::vector<std::string> BlockParser::gatherItem(const Marker& m) {
    std::vector<std::string> item;
    item.push_back(m.rest);
    bool lastWasText = !trim(m.rest).empty();
    while (mCursor < mLines.size()) {
        if (handleItemBlank(item, m.contentIndent)) {
            lastWasText = false;
            continue;
        }
        const std::string& ln = mLines[mCursor];
        if (indentOf(ln) >= m.contentIndent) {
            item.push_back(ln.substr(m.contentIndent));
            lastWasText = !isBlockStarter(ln);
            ++mCursor;
            continue;
        }
        if (isBlockStarter(ln)) {
            break;
        }
        if (lastWasText) { // lazy-continuation guard
            item.push_back(rtrimCopy(ln));
            ++mCursor;
            continue;
        }
        break;
    }
    while (!item.empty() && isBlank(item.back())) {
        item.pop_back();
    }
    return item;
}

bool BlockParser::handleItemBlank(
    std::vector<std::string>& item,
    size_t contentIndent
){
    if (!isBlank(mLines[mCursor])) {
        return false;
    }
    size_t k = mCursor;
    while (k < mLines.size() && isBlank(mLines[k])) {
        ++k;
    }
    if (k < mLines.size() && indentOf(mLines[k]) >= contentIndent) {
        item.push_back("");
        ++mCursor;
        return true;
    }
    return false;
}

bool BlockParser::makeListItem(std::vector<std::string> raw,
                               std::vector<ListItem>& out,
                               int depth) {
    ListItem item;
    stripTaskPrefix(
        raw,
        item
    );
    std::vector<BlockPtr> sub = parseInner(raw, depth + 1);
    if (!sub.empty() && sub[0]->type == Block::Type::Paragraph) {
        item.content = std::move(sub[0]->inlines);
        sub.erase(sub.begin());
    }
    item.children = std::move(sub);
    const bool task = item.task;
    out.push_back(std::move(item));
    return task;
}

void BlockParser::stripTaskPrefix(
    std::vector<std::string>& raw,
    ListItem& item
){
    if (raw.empty()) {
        return;
    }
    std::string& first = raw[0];
    if (ut::startsWithAny(first, {"[ ] ", "[ ]\t"})) {
        item.task = true;
        first = first.substr(4);
    } else if (ut::startsWithAny(first, {"[x] ", "[X] ", "[x]\t", "[X]\t"})) {
        item.task = true;
        item.checked = true;
        first = first.substr(4);
    }
}

} // namespace md::detail
