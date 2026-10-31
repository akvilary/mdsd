// Сериализация таблиц: columnsConfigs, строки, ячейки с абзацами и выравниванием.

#include <span>

#include "sabydoc/model/widget_writer/widget_io.hpp"
#include "uuid4/uuid4.hpp"

namespace sd::io {

namespace {

Json makeColumnsConfigs(size_t n) {
    Json configs = Json::array();
    for (size_t i = 0; i < n; ++i) {
        configs.push_back(Json{{"width", "auto"}});
    }
    return configs;
}

Json writeCellParagraph(const Text& t, const std::string& align) {
    Block p;
    p.kind = Block::Kind::Paragraph;
    p.id = makeUuid4();
    p.align = align;
    p.text = t;
    return writeParagraph(p);
}

Json writeTableCell(const TableCell& cell, const std::string& align) {
    Json attrs = Json::object();
    attrs["id"] = makeUuid4();
    Json cellNode = Json::array({"ModuleEditor/table:Cell", std::move(attrs)});
    if (cell.paragraphs.empty()) {
        cellNode.push_back(writeCellParagraph(Text{}, align));
    } else {
        for (const Text& t : cell.paragraphs) {
            cellNode.push_back(writeCellParagraph(t, align));
        }
    }
    return cellNode;
}

Json writeTableRow(const std::vector<TableCell>& row, std::span<const std::string> colAlign) {
    Json attrs = Json::object();
    attrs["id"] = makeUuid4();
    Json rowNode = Json::array({"ModuleEditor/table:Row", std::move(attrs)});
    for (size_t col = 0; col < row.size(); ++col) {
        rowNode.push_back(
            writeTableCell(row[col], col < colAlign.size() ? colAlign[col] : std::string()));
    }
    return rowNode;
}

} // namespace

Json writeTable(const Block& b) {
    Json attrs = Json::object();
    attrs["columnsConfigs"] = makeColumnsConfigs(b.colAlign.size());
    attrs["id"] = b.id;
    attrs["stickyColumn"] = false;
    attrs["stickyHeader"] = false;
    Json node = Json::array({"ModuleEditor/table:View", std::move(attrs)});
    for (const auto& row : b.rows) {
        node.push_back(writeTableRow(row, b.colAlign));
    }
    return node;
}

} // namespace sd::io
