#pragma once

#include <memory>
#include <string>
#include <vector>

#include "markdown/ast/ast.hpp"

/// \brief Canonical SabyDoc model. The single domain for BOTH directions:
/// md -> sabydoc (mapping builds the model, widget_writer serializes it to JSONML)
/// and in the future sabydoc -> md (widget_reader will build the model from JSONML).
/// No JSONML details should ever live here.
namespace sd {

/// \brief Marker style of a list block.
enum class ListMarker { Bullet, Numbered, Checkbox };

/// \brief Formatting range: [offset, length] in UTF-16 CODE UNITS
/// (like String.length in the JS editor). href != "" means a link.
struct FormatRange {
    size_t offset = 0;
    size_t length = 0;
    bool bold = false;
    bool italic = false;
    bool strike = false;
    bool underline = false;
    std::string href;
};

/// \brief Rich text: UTF-8 content plus a list of formatting ranges.
struct Text {
    std::string content; // UTF-8; empty string → the children:"" attribute
    std::vector<FormatRange> formats;
    bool emptyColor1 = false; // imitates the editor's separator empty paragraph
};

/// \brief A node of the document block tree.
struct Block;
using BlockPtr = std::unique_ptr<Block>;

/// \brief A single list item: checkbox state, content, and nested child blocks.
struct ListItem {
    std::string id;
    bool checked = false;
    Text content;
    std::vector<BlockPtr> children; // nested list, extra paragraphs of the item
};

/// \brief One table cell holding its paragraphs.
struct TableCell {
    std::vector<Text> paragraphs; // hard line breaks inside a cell → multiple paragraphs
};

/// \brief A block-level element of the document (paragraph, header, list, table, ...).
struct Block {
    /// \brief The concrete block kind.
    enum class Kind {
        DocumentHeader,
        Header,
        Paragraph,
        Paragraph2,
        Code,
        Quote,
        List,
        Table,
        Image,
        Separator
    };
    Kind kind = Kind::Paragraph;

    std::string id;
    int level = 0;        // header level 1..5
    std::string align;    // "" | "center" | "right" | "justify"
    bool indent0 = false; // indent:0 attribute of the separator empty paragraph

    Text text;        // DocumentHeader / Header / Paragraph / Paragraph2
    std::string code; // Code

    std::vector<BlockPtr> children; // Quote

    ListMarker marker = ListMarker::Bullet; // List
    std::vector<ListItem> items;            // List

    std::vector<std::string> colAlign; // Table: "" | "center" | "right" | "justify"
    std::vector<std::vector<TableCell>> rows;

    std::string resourceKey; // Image
    unsigned imageW = 0, imageH = 0;
};

/// \brief An embedded binary resource (image) of the document.
struct Resource {
    std::string key;      // r1, r2, ...
    std::string fileName; // original file name (for the fileName attribute)
    std::string fileId;   // uuid
    std::string zipName;  // name inside the archive (sanitized)
    unsigned width = 0, height = 0;
    std::string bytes; // file contents
};

/// \brief The whole SabyDoc document: title, blocks, and resources.
struct Document {
    std::string title; // documentHeader
    std::string pageName = "Basic";
    std::vector<BlockPtr> blocks;    // children of the root textLayout
    std::vector<Resource> resources; // in order of appearance
    std::string pageId, layoutId, rootTextLayoutId, headerId;
};

} // namespace sd
