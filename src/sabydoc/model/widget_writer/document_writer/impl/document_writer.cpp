// Assembly of the final .sabydoc archive (ZIP store) from the model: content.json + index.site + resources.

#include <nlohmann/json.hpp>

#include "sabydoc/model/widget_writer/document_writer/document_writer.hpp"
#include "zip_writer/zip_writer.hpp"
#include "sabydoc/model/widget_writer/widget_writer.hpp"

namespace sd {

namespace {

using sd::Json;

std::string buildContentJson(const std::string& pageId) {
    Json view = Json::object();
    view["siteId"] = pageId;
    view["revisionId"] = nullptr;
    view["originId"] = pageId;
    view["constructorId"] = "SabyDocConstructor";
    view["resourcesFolderId"] = nullptr;

    Json root = Json::object();
    root["view"] = std::move(view);
    root["contextTypeProperties"] = nullptr;
    root["dataContext"] = "{\"f\":0,\"d\":[],\"s\":[],\"_type\":\"record\"}";
    root["version"] = 1;
    return root.dump();
}

std::string buildIndexSite(const Document& doc) {
    Json page = Json::object();
    page["id"] = doc.pageId;
    page["originId"] = nullptr;
    page["name"] = doc.pageName;
    page["content"] = writeFrame(doc);
    page["order"] = 0;
    page["visibility"] = nullptr;
    page["constructorId"] = "SabyDocConstructor";
    page["SEO"] = Json::array();
    page["anonymous"] = true;
    page["url"] = nullptr;

    Json root = Json::object();
    root["propertyValues"] = Json{{"tableOfContents", writeTableOfContents(doc)}};
    root["pages"] = Json::array({std::move(page)});
    root["paletteId"] = nullptr;
    root["themeId"] = nullptr;
    root["themeData"] = nullptr;
    root["fontThemeId"] = nullptr;
    root["fontThemeData"] = nullptr;
    root["widgetThemeId"] = nullptr;
    root["widgetThemeData"] = nullptr;
    root["conditions"] = nullptr;
    return root.dump();
}

} // namespace

std::string buildSabydocArchive(const Document& doc) {
    ZipWriter zw;
    zw.addFile("content.json", buildContentJson(doc.pageId));
    zw.addFile("index.site", buildIndexSite(doc));
    for (const Resource& r : doc.resources) {
        zw.addFile("resources/" + r.zipName, r.bytes);
    }
    return zw.finish();
}

} // namespace sd
