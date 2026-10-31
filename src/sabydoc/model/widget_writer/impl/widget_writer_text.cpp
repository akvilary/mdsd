// formattedText serialization: properties, [offset, length] ranges (UTF-16),
// link anchor triples, and the empty form with children:"".

#include "sabydoc/model/widget_writer/widget_io.hpp"
#include "uuid4/uuid4.hpp"

namespace sd::io {

namespace {

Json propsObject(const FormatRange& r) {
    Json props = Json::object();
    if (r.strike || r.underline) {
        Json lines = Json::array();
        if (r.strike) {
            lines.push_back("line-through");
        }
        if (r.underline) {
            lines.push_back("underline");
        }
        props["decoration"] = Json{{"line", std::move(lines)}};
    }
    if (r.bold || r.italic) {
        Json font = Json::object();
        if (r.italic) {
            font["style"] = "italic";
        }
        if (r.bold) {
            font["weight"] = "bold";
        }
        props["font"] = std::move(font);
    }
    if (!r.href.empty()) {
        props["href"] = r.href;
        props["visibleLinkHeading"] = "visible";
    }
    return props;
}

Json formatsArray(const Text& t) {
    Json formats = Json::array();
    for (const FormatRange& r : t.formats) {
        if (r.length == 0) {
            continue;
        }
        if (!r.href.empty()) {
            // anchor triple as in the editor: [s,0,{}], [s,len,{href}], [s+len,0,{}]
            formats.push_back(Json::array({r.offset, 0, Json::object()}));
            formats.push_back(Json::array({r.offset, r.length, propsObject(r)}));
            formats.push_back(Json::array({r.offset + r.length, 0, Json::object()}));
        } else {
            formats.push_back(Json::array({r.offset, r.length, propsObject(r)}));
        }
    }
    if (formats.empty()) {
        formats.push_back(Json::array({0, 0, Json::object()}));
    }
    return formats;
}

} // namespace

Json writeParagraph(const Block& b) {
    Json attrs = Json::object();
    if (!b.align.empty()) {
        attrs["align"] = b.align;
    }
    attrs["id"] = b.id;
    if (b.indent0) {
        attrs["indent"] = 0;
    }
    attrs["textStyle"] = "default";
    return Json::array(
        {"ModuleEditor/paragraph:View", std::move(attrs), writeFormattedText(b.text)});
}

Json writeFormattedText(const Text& t) {
    Json attrs = Json::object();
    if (t.content.empty()) {
        attrs["children"] = "";
        Json props = Json::object();
        if (t.emptyColor1) {
            props["color"] = "color1";
        }
        attrs["formats"] = Json::array({Json::array({0, 0, std::move(props)})}); // nested triple
    } else {
        attrs["formats"] = formatsArray(t);
    }
    attrs["id"] = makeUuid4();
    if (t.content.empty()) {
        return Json::array({"ModuleEditor/formattedText:View", std::move(attrs)});
    }
    return Json::array({"ModuleEditor/formattedText:View", std::move(attrs), t.content});
}

} // namespace sd::io
