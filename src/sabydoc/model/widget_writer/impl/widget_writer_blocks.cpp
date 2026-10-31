// Serialization of simple widgets: headers, paragraphs, code, quotes, lists, images, separators.

#include <span>

#include "sabydoc/model/widget_writer/widget_io.hpp"
#include "uuid4/uuid4.hpp"

namespace sd::io {

namespace {

Json writeParagraph2(const Block& b) {
    Json attrs = Json::object();
    attrs["id"] = b.id;
    attrs["textStyle"] = "default";
    return Json::array(
        {"ModuleEditor/paragraph2:View", std::move(attrs), writeFormattedText(b.text)});
}

Json writeHeader(const Block& b) {
    Json attrs = Json::object();
    attrs["id"] = b.id;
    attrs["size"] = b.level;
    attrs["textStyle"] = "default";
    return Json::array({"ModuleEditor/baseWidgets:Header" + std::to_string(b.level),
                        std::move(attrs),
                        writeFormattedText(b.text)});
}

Json writeCode(const Block& b) {
    Json attrs = Json::object();
    attrs["id"] = b.id;
    attrs["value"] = b.code;
    attrs["valueWithTokens"] = b.code; // no highlighting: tokens = plain text (agreed)
    return Json::array({"ModuleEditor/codeBlock:View", std::move(attrs)});
}

Json writeSeparator(const Block& b) {
    Json attrs = Json::object();
    attrs["id"] = b.id;
    return Json::array({"SiteEditorWidgets/separator:View", std::move(attrs)});
}

const char* markerConfigId(ListMarker m) {
    switch (m) {
    case ListMarker::Bullet:
        return "bulletWhiteBulletSquare";
    case ListMarker::Numbered:
        return "numbers";
    case ListMarker::Checkbox:
        return "checkboxes";
    }
    return "bulletWhiteBulletSquare";
}

Json writeListItemContent(const Text& t) {
    Json attrs = Json::object();
    attrs["id"] = makeUuid4();
    attrs["textStyle"] = "default";
    return Json::array(
        {"ModuleEditor/list:ListItemContent", std::move(attrs), writeFormattedText(t)});
}

Json writeListItem(const ListItem& item) {
    Json itemAttrs = Json::object();
    itemAttrs["id"] = item.id;
    if (item.checked) {
        itemAttrs["checked"] = true;
    }
    Json itemNode = Json::array(
        {"ModuleEditor/list:Item", std::move(itemAttrs), writeListItemContent(item.content)});
    for (const BlockPtr& child : item.children) {
        if (child->kind == Block::Kind::Paragraph) {
            itemNode.push_back(writeListItemContent(child->text)); // extra paragraph of the item
        } else {
            itemNode.push_back(writeBlockValue(*child));
        }
    }
    return itemNode;
}

Json writeList(const Block& b) {
    Json attrs = Json::object();
    attrs["id"] = b.id;
    attrs["markerConfigId"] = markerConfigId(b.marker);
    Json node = Json::array({"ModuleEditor/list:View", std::move(attrs)});
    for (const ListItem& item : b.items) {
        node.push_back(writeListItem(item));
    }
    return node;
}

Json writeQuote(const Block& b) {
    Json attrs = Json::object();
    attrs["id"] = b.id;
    Json node = Json::array({"ModuleEditor/blockquote:View", std::move(attrs)});
    for (const BlockPtr& c : b.children) {
        node.push_back(writeBlockValue(*c));
    }
    return node;
}

Json writeImage(const Block& b) {
    Json style = Json::object();
    style["aspectRatio"] = "auto";
    style["width"] = std::to_string(b.imageW) + "px";
    Json attrs = Json::object();
    attrs[".style"] = std::move(style);
    attrs["action"] = Json{{"id", "openFullScreenRuntime"}};
    attrs["aspectRatioStyle"] = "auto";
    attrs["id"] = b.id;
    attrs["imageFit"] = "cover";
    attrs["isAutoHeight"] = false;
    attrs["saveProportions"] = nullptr;
    attrs["value"] = b.resourceKey;
    return Json::array({"ModuleEditor/image:View", std::move(attrs)});
}

} // namespace

Json writeBlockValue(const Block& b) {
    switch (b.kind) {
    case Block::Kind::DocumentHeader: {
        Json attrs = Json::object();
        attrs["draggable"] = false;
        attrs["id"] = b.id;
        return Json::array({"DocsEditor/documentHeader:View", std::move(attrs), b.text.content});
    }
    case Block::Kind::Header:
        return writeHeader(b);
    case Block::Kind::Paragraph:
        return writeParagraph(b);
    case Block::Kind::Paragraph2:
        return writeParagraph2(b);
    case Block::Kind::Code:
        return writeCode(b);
    case Block::Kind::Quote:
        return writeQuote(b);
    case Block::Kind::List:
        return writeList(b);
    case Block::Kind::Table:
        return writeTable(b);
    case Block::Kind::Image:
        return writeImage(b);
    case Block::Kind::Separator:
        return writeSeparator(b);
    }
    return writeParagraph(b);
}

} // namespace sd::io
