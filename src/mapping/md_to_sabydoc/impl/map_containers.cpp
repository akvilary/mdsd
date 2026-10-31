#include "mapping/md_to_sabydoc/mappers.hpp"

#include "uuid4/uuid4.hpp"
#include "sabydoc/model/text_layout/text_layout.hpp"

namespace mapping {
namespace detail {

// ---------------------------------------------------------------- lists

void mapBlocks(std::vector<md::BlockPtr>& src, std::vector<sd::BlockPtr>& dst,
               ResourceCollector& rc, bool topLevel, int depth);

sd::ListItem mapListItem(md::ListItem& item, ResourceCollector& rc, int depth) {
    sd::ListItem out;
    out.id = makeUuid4();
    out.checked = item.task && item.checked;
    out.content = sd::buildText(item.content);
    std::vector<md::BlockPtr> flat;
    flattenQuotes(item.children, flat);
    mapBlocks(flat, out.children, rc, false, depth + 1);
    return out;
}

sd::ListMarker listMarkerOf(md::ListKind kind, bool anyTask) {
    if (anyTask) {
        return sd::ListMarker::Checkbox;
    }
    switch (kind) {
    case md::ListKind::Ordered:
        return sd::ListMarker::Numbered;
    case md::ListKind::Bullet:
    case md::ListKind::Task:
    default:
        return sd::ListMarker::Bullet;
    }
}

void mapList(md::Block& b, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc, int depth) {
    auto lst = newBlock(sd::Block::Kind::List);
    bool anyTask = false;
    for (md::ListItem& item : b.items) {
        anyTask = item.task || anyTask;
        lst->items.push_back(mapListItem(item, rc, depth));
    }
    lst->marker = listMarkerOf(b.listKind, anyTask);
    dst.push_back(std::move(lst));
}

// ---------------------------------------------------------------- tables

sd::TableCell mapCell(const std::vector<md::InlineElement>& cell) {
    sd::TableCell c;
    const auto groups = md::splitByLineBreaks(cell);
    if (groups.empty()) {
        c.paragraphs.push_back(sd::Text{});
        return c;
    }
    for (const auto& g : groups) {
        c.paragraphs.push_back(sd::buildText(g)); // images inside cells -> alt
    }
    return c;
}

void mapTable(const md::Block& b, std::vector<sd::BlockPtr>& dst) {
    auto t = newBlock(sd::Block::Kind::Table);
    for (md::Align a : b.colAlign) {
        t->colAlign.push_back(alignStr(a));
    }
    for (const auto& row : b.rows) {
        std::vector<sd::TableCell> cells;
        cells.reserve(row.size());
        for (const auto& cell : row) {
            cells.push_back(mapCell(cell));
        }
        t->rows.push_back(std::move(cells));
    }
    dst.push_back(std::move(t));
}

} // namespace detail
} // namespace mapping
