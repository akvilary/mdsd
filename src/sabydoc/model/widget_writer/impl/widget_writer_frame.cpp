// Frame assembly: frame (elementMetaTypes + resources), documentLayout, table of contents.

#include "sabydoc/model/widget_writer/widget_writer.hpp"
#include "sabydoc/model/widget_writer/widget_io.hpp"
#include "uuid4/uuid4.hpp"

namespace sd {

using io::writeBlockValue;

namespace {

Json elementMetaTypes() {
    const Json resourceAttrs = Json{{"type", "resource"}};

    Json columns = Json::object();
    columns["attributes"] = Json{
        {"background.image.resource", resourceAttrs},
        {"background.layers.image.original", resourceAttrs},
    };

    Json image = Json::object();
    image["attributes"] = Json{{"value", Json{{"type", "resource"}}}};

    Json table = Json::object();
    table["relatedObjects"] = Json::array({"object"});

    Json textLayout = Json::object();
    textLayout["attachedAttributes"] = Json{{"indent", Json{{"type", "attachedAttribute"}}}};
    textLayout["attributes"] = Json{
        {"background.image.resource", resourceAttrs},
        {"background.layers.image.original", resourceAttrs},
    };
    textLayout["relatedObjects"] = Json::array({"object"});

    Json meta = Json::object();
    meta["FrameControls/columnsLayout:View"] = std::move(columns);
    meta["ModuleEditor/image:View"] = std::move(image);
    meta["ModuleEditor/table:View"] = std::move(table);
    meta["ModuleEditor/textLayout:View"] = std::move(textLayout);
    return meta;
}

Json resourcesObject(const Document& doc) {
    Json resources = Json::object();
    for (const Resource& r : doc.resources) {
        Json res = Json::object();
        res["fileName"] = r.fileName;
        res["file_id"] = r.fileId;
        res["naturalHeight"] = r.height;
        res["naturalWidth"] = r.width;
        res["type"] = "internal";
        res["value"] = "./resources/" + r.zipName;
        resources[r.key] = std::move(res);
    }
    return resources;
}

Json writeRootTextLayout(const Document& doc) {
    Json attrs = Json::object();
    attrs["id"] = doc.rootTextLayoutId;
    attrs["removable"] = false;
    attrs["selectable"] = false;
    Json root = Json::array({"ModuleEditor/textLayout:View", std::move(attrs)});
    for (const BlockPtr& b : doc.blocks) {
        root.push_back(writeBlockValue(*b));
    }
    return root;
}

struct TocHeader {
    int level;
    const std::string* id;
    const std::string* text;
};

std::vector<TocHeader> collectTocHeaders(const Document& doc) {
    std::vector<TocHeader> headers;
    for (const BlockPtr& b : doc.blocks) {
        if (b->kind == Block::Kind::Header) {
            headers.push_back({b->level, &b->id, &b->text.content});
        }
    }
    return headers;
}

struct TocLinks {
    std::vector<int> parent;
    std::vector<char> hasChild;
};

// parentId via a level stack (as in the editor); hasChild is consistent with the children
TocLinks computeTocLinks(const std::vector<TocHeader>& headers) {
    TocLinks links{std::vector<int>(headers.size(), -1), std::vector<char>(headers.size(), 0)};
    std::vector<int> stack;
    for (size_t i = 0; i < headers.size(); ++i) {
        while (!stack.empty() &&
               headers[static_cast<size_t>(stack.back())].level >= headers[i].level) {
            stack.pop_back();
        }
        links.parent[i] = stack.empty() ? -1 : stack.back();
        if (links.parent[i] >= 0) {
            links.hasChild[static_cast<size_t>(links.parent[i])] = 1;
        }
        stack.push_back(static_cast<int>(i));
    }
    return links;
}

} // namespace

Json writeFrame(const Document& doc) {
    Json layoutAttrs = Json::object();
    layoutAttrs["id"] = doc.layoutId;
    Json layout = Json::array({"DocsEditor/documentLayout:View", std::move(layoutAttrs)});

    Json headerAttrs = Json::object();
    headerAttrs["draggable"] = false;
    headerAttrs["id"] = doc.headerId;
    layout.push_back(
        Json::array({"DocsEditor/documentHeader:View", std::move(headerAttrs), doc.title}));

    layout.push_back(writeRootTextLayout(doc));

    Json meta = Json::object();
    meta["elementMetaTypes"] = elementMetaTypes();
    meta["resources"] = resourcesObject(doc);
    return Json::array({"frame", std::move(meta), std::move(layout)});
}

Json writeTableOfContents(const Document& doc) {
    const std::vector<TocHeader> headers = collectTocHeaders(doc);
    const TocLinks links = computeTocLinks(headers);

    Json toc = Json::array();
    for (size_t i = 0; i < headers.size(); ++i) {
        const int p = links.parent[i];
        Json entry = Json::object();
        entry["hasChild"] = links.hasChild[i] != 0;
        entry["id"] = "toc_" + *headers[i].id;
        entry["isHeader"] = true;
        entry["isParent"] = false;
        entry["level"] = headers[i].level;
        entry["parentId"] = p < 0 ? Json() : Json("toc_" + *headers[static_cast<size_t>(p)].id);
        entry["text"] = *headers[i].text;
        toc.push_back(std::move(entry));
    }
    return toc;
}

} // namespace sd
