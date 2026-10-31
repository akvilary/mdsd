// Блочный парсер: GFM-таблицы (шапка + разделитель + тело).

#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

#include "markdown/parser/chars/table_row/table_row.hpp"

namespace md::detail {

BlockPtr BlockParser::tryTableStart() {
    if (mLines[mCursor].find('|') == std::string::npos || mCursor + 1 >= mLines.size()) {
        return nullptr;
    }
    const auto aligns = tableDelimiters(mLines[mCursor + 1]);
    if (!aligns) {
        return nullptr;
    }
    const std::vector<std::string> header = splitTableRow(mLines[mCursor]);
    if (header.size() != aligns->size()) {
        return nullptr;
    }
    mCursor += 2;
    return buildTable(header, *aligns);
}

BlockPtr BlockParser::buildTable(const std::vector<std::string>& headerCells,
                                 const std::vector<Align>& aligns) {
    auto b = std::make_unique<Block>();
    b->type = Block::Type::Table;
    b->colAlign = aligns;
    b->rows.push_back(parseCells(headerCells));
    while (mCursor < mLines.size()) {
        const std::string& l = mLines[mCursor];
        if (isBlank(l) || l.find('|') == std::string::npos) {
            break;
        }
        std::vector<std::string> cells = splitTableRow(l);
        cells.resize(headerCells.size(), std::string());
        b->rows.push_back(parseCells(cells));
        ++mCursor;
    }
    return b;
}

std::vector<std::vector<InlineElement>>
BlockParser::parseCells(const std::vector<std::string>& cells) {
    std::vector<std::vector<InlineElement>> out;
    out.reserve(cells.size());
    for (const std::string& c : cells) {
        out.push_back(parseInline(c));
    }
    return out;
}

} // namespace md::detail
