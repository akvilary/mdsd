// Block parser: quotes and lists.
// List policy: an item continues while lines are indented to its content;
// a blank line or a smaller indent ends the item and the whole list.

#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

#include "algo/algo.hpp"
#include "markdown/parser/chars/chars.hpp"
#include "markdown/parser/chars/detectors/detectors.hpp"
#include "markdown/parser/chars/marker/marker.hpp"

namespace md {

namespace {

/// Checks whether a list item starts with a task prefix: "[ ] ", "[x] ", "[X] ".
bool hasTaskPrefix(const std::string& s) {
    return ut::startsWithAny(s, {"[ ] ", "[ ]\t", "[x] ", "[X] ", "[x]\t", "[X]\t"});
}

} // namespace

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
    // One list = one run of items of the same kind (checkbox or plain).
    // A kind change ends the list; the block loop starts a new one.
    const bool taskRun = hasTaskPrefix(m.rest);
    auto b = std::make_unique<Block>();
    b->type = Block::Type::List;
    while (const auto item = peekListItem(m.ordered)) {
        if (hasTaskPrefix(item->rest) != taskRun) {
            break; // a different kind starts its own list
        }
        ++mCursor;
        makeListItem(gatherItem(*item), b->items, depth);
    }
    b->listKind = taskRun ? ListKind::Task : (m.ordered ? ListKind::Ordered : ListKind::Bullet);
    return b;
}

// An item of the current kind at mCursor; blank lines are not skipped —
// per the list policy a blank line terminates the collection.
std::optional<Marker> BlockParser::peekListItem(bool ordered) {
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
    while (mCursor < mLines.size()) {
        const std::string& ln = mLines[mCursor];
        // Continuation: only lines indented at least to the item content.
        // A blank line or a smaller indent ends the item and the list.
        if (isBlank(ln) || indentOf(ln) < m.contentIndent) {
            break;
        }
        item.push_back(ln.substr(m.contentIndent));
        ++mCursor;
    }
    while (!item.empty() && isBlank(item.back())) { // a bare "-" marker
        item.pop_back();
    }
    return item;
}

void BlockParser::makeListItem(
    std::vector<std::string> raw,
    std::vector<ListItem>& out,
    int depth
){
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
    out.push_back(std::move(item));
}

void BlockParser::stripTaskPrefix(
    std::vector<std::string>& raw,
    ListItem& item
){
    if (raw.empty() || !hasTaskPrefix(raw[0])) {
        return;
    }
    item.task = true;
    item.checked = raw[0][1] != ' ';
    raw[0] = raw[0].substr(4);
}

} // namespace md
