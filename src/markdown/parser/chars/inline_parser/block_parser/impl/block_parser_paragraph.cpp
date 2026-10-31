// Блочный парсер: абзацы, setext, GFM-таблица поверх абзаца, жёсткие переносы.

#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

#include "markdown/parser/chars/chars.hpp"
#include "markdown/parser/chars/detectors/detectors.hpp"
#include "markdown/parser/chars/link_def/link_def.hpp"
#include "markdown/parser/chars/marker/marker.hpp"
#include "markdown/parser/chars/table_row/table_row.hpp"

namespace md::detail {

bool BlockParser::interruptsParagraph(const std::string& line, size_t idx) const {
    if (isAtx(line) || isHr(line) || isQuoteStart(line)) {
        return true;
    }
    char fc = 0;
    size_t fl = 0;
    std::string info;
    if (isFenceLine(line, fc, fl, info)) {
        return true;
    }
    const Marker m = matchListMarker(line);
    if (m.ok && !m.rest.empty() && (!m.ordered || isOrderedListStartWithOne(line))) {
        return true;
    }
    if (line.find('|') != std::string::npos && idx + 1 < mLines.size() &&
        tableDelimiters(mLines[idx + 1]).has_value()) {
        return true;
    }
    return alignWrapper(line).has_value() || trim(line).starts_with("<!--");
}

BlockPtr BlockParser::parseParagraph() {
    std::vector<std::pair<std::string, bool>> collected;
    addParagraphLine(collected, mLines[mCursor].substr(indentOf(mLines[mCursor])));
    ++mCursor;
    while (mCursor < mLines.size()) {
        const std::string& line = mLines[mCursor];
        if (isBlank(line)) {
            break;
        }
        if (BlockPtr b = tryParagraphSpecial(collected, line)) {
            return b;
        }
        if (interruptsParagraph(line, mCursor)) {
            break;
        }
        addParagraphLine(collected, line.substr(indentOf(line)));
        ++mCursor;
    }
    auto b = std::make_unique<Block>();
    b->type = Block::Type::Paragraph;
    b->inlines = parseInline(joinCollected(collected, true));
    return b;
}

// setext-подчёркивание и GFM-таблица «поверх» последней строки абзаца
BlockPtr BlockParser::tryParagraphSpecial(std::vector<std::pair<std::string, bool>>& collected,
                                          const std::string& line) {
    if (int level = 0; !collected.empty() && isSetextUnderline(line, level)) {
        ++mCursor;
        auto b = std::make_unique<Block>();
        b->type = Block::Type::Header;
        b->level = level;
        b->inlines = parseInline(joinCollected(collected, false));
        return b;
    }
    if (collected.size() == 1 && line.find('|') != std::string::npos) {
        if (auto b = tableFromDelimiter(collected.back().first, line)) {
            return b;
        }
    }
    return nullptr;
}

// Строка-разделитель превращает предыдущую строку абзаца в шапку таблицы
BlockPtr BlockParser::tableFromDelimiter(const std::string& headerLine,
                                         const std::string& delimLine) {
    if (headerLine.find('|') == std::string::npos) {
        return nullptr;
    }
    const auto aligns = tableDelimiters(delimLine);
    if (!aligns) {
        return nullptr;
    }
    const std::vector<std::string> header = splitTableRow(headerLine);
    if (header.size() != aligns->size()) {
        return nullptr;
    }
    ++mCursor;
    return buildTable(header, *aligns);
}

// Строка абзаца: (текст, жёсткий-перенос-после). Два+ пробела в конце — жёсткий
// перенос; нечётное число обратных слэшей в конце — тоже (чётное — экранированные пары).
void BlockParser::addParagraphLine(std::vector<std::pair<std::string, bool>>& collected,
                                   std::string l) {
    bool hard = false;
    size_t end = l.size();
    while (end > 0 && isSpaceCh(l[end - 1])) {
        --end;
    }
    if (l.size() - end >= 2) {
        hard = true;
    }
    l = l.substr(0, end);
    const size_t bs = trailingBackslashes(l);
    if (bs % 2 == 1) {
        hard = true;
        l.pop_back();
        while (!l.empty() && isSpaceCh(l.back())) {
            l.pop_back();
        }
    }
    collected.emplace_back(l, hard);
}

size_t BlockParser::trailingBackslashes(const std::string& l) {
    size_t bs = 0;
    while (bs < l.size() && l[l.size() - 1 - bs] == '\\') {
        ++bs;
    }
    return bs;
}

std::string BlockParser::joinCollected(const std::vector<std::pair<std::string, bool>>& lines,
                                       bool useBreaks) {
    std::string out;
    for (size_t k = 0; k < lines.size(); ++k) {
        if (k > 0) {
            out += (useBreaks && lines[k - 1].second) ? '\n' : ' ';
        }
        out += lines[k].first;
    }
    return out;
}

} // namespace md::detail
