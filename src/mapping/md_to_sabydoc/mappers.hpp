#pragma once

// Internal mapping functions from the md AST to the SabyDoc model (for impl/map_*.cpp).
// All translation policy (h6, hr, separators, placeholders) lives here.

#include <string>
#include <vector>

#include "markdown/ast/ast.hpp"
#include "sabydoc/model/model.hpp"
#include "sabydoc/model/resource_collector/resource_collector.hpp"

namespace mapping::detail {

/// Maximum container nesting depth accepted while mapping.
constexpr int kMaxDepth = 64;

/// Converts a markdown alignment value to its sabydoc string form.
/// Param a: The markdown alignment.
/// Returns One of "", "center", "right", "justify".
std::string alignStr(md::Align a);

/// Tells whether a block of this kind must be followed by a separator empty paragraph.
/// Param k: The block kind.
/// Returns true when a trailing separator is required.
bool needsTrailingSeparator(sd::Block::Kind k);

/// Creates a new block of the given kind with a fresh id.
/// Param kind: The kind of block to create.
/// Returns The newly created block.
std::unique_ptr<sd::Block> newBlock(sd::Block::Kind kind);

/// Creates a paragraph block mimicking the editor's separator empty paragraph.
/// Returns The separator-mimicking block.
std::unique_ptr<sd::Block> makeSeparatorMimic();

/// Creates a placeholder paragraph for an unavailable image.
/// Param url: The image reference that could not be loaded.
/// Returns The placeholder block.
std::unique_ptr<sd::Block> makePlaceholder(const std::string& url);

/// Creates a caption paragraph for an image.
/// Param text: The caption text.
/// Returns The caption block.
std::unique_ptr<sd::Block> makeCaption(const std::string& text);

// --- impl/map_content.cpp: factories, images, paragraphs, headers, quotes ---
/// Emits image (and optional caption) blocks for a markdown image.
/// Param img: The markdown image element.
/// Param dst: Destination list for the emitted blocks.
/// Param rc: Resource collector used to register the image.
void emitImage(
    const md::InlineImage& img,
    std::vector<sd::BlockPtr>& dst,
    ResourceCollector& rc
);

/// Drops trailing empty inline runs (e.g. line-break tails) from a group.
/// Param elems: The element list to trim in place.
void dropTrailingEmptyRuns(std::vector<md::InlineElement>& elems);

/// Maps one aligned inline group to a paragraph block.
/// Param group: Inline elements of the group.
/// Param align: Text alignment of the group.
/// Param dst: Destination block list.
/// Param rc: Resource collector used to register images.
/// Param allowImages: Whether images may be emitted here.
void mapGroup(const std::vector<md::InlineElement>& group,
              md::Align align,
              std::vector<sd::BlockPtr>& dst,
              ResourceCollector& rc,
              bool allowImages);

/// Maps a markdown paragraph block.
/// Param b: The markdown paragraph block.
/// Param dst: Destination block list.
/// Param rc: Resource collector used to register images.
/// Param allowImages: Whether images may be emitted here.
void mapParagraph(const md::Block& b,
                  std::vector<sd::BlockPtr>& dst,
                  ResourceCollector& rc,
                  bool allowImages);

/// Flattens nested quote blocks into a single-level block list.
/// Param src: Source markdown blocks.
/// Param out: Output list of flattened blocks.
void flattenQuotes(
    std::vector<md::BlockPtr>& src,
    std::vector<md::BlockPtr>& out
);

/// Maps a markdown header block.
/// Param b: The markdown header block.
/// Param dst: Destination block list.
/// Param topLevel: Whether the header sits at the document top level.
void mapHeader(
    const md::Block& b,
    std::vector<sd::BlockPtr>& dst,
    bool topLevel
);

// --- impl/map_blocks.cpp: dispatcher, separator policy ---

/// Maps a single markdown block by dispatching on its kind.
/// Param b: The markdown block to map.
/// Param dst: Destination block list.
/// Param rc: Resource collector used to register images.
/// Param topLevel: Whether the block sits at the document top level.
/// Param depth: Current container nesting depth.
void mapSingleBlock(md::BlockPtr& b,
                    std::vector<sd::BlockPtr>& dst,
                    ResourceCollector& rc,
                    bool topLevel,
                    int depth);

/// Maps a list of markdown blocks.
/// Param src: Source markdown blocks.
/// Param dst: Destination block list.
/// Param rc: Resource collector used to register images.
/// Param topLevel: Whether the blocks sit at the document top level.
/// Param depth: Current container nesting depth.
void mapBlocks(std::vector<md::BlockPtr>& src,
               std::vector<sd::BlockPtr>& dst,
               ResourceCollector& rc,
               bool topLevel,
               int depth);

// --- impl/map_containers.cpp: lists and tables ---

/// Maps a single markdown list item with its nested children.
/// Param item: The markdown list item.
/// Param rc: Resource collector used to register images.
/// Param depth: Current container nesting depth.
/// Returns The mapped sabydoc list item.
sd::ListItem mapListItem(
    md::ListItem& item,
    ResourceCollector& rc,
    int depth
);

/// Chooses the sabydoc list marker for a markdown list.
/// Param kind: The markdown list kind.
/// Param anyTask: Whether any item of the list is a task item.
/// Returns The matching sabydoc list marker.
sd::ListMarker listMarkerOf(
    md::ListKind kind,
    bool anyTask
);

/// Maps a markdown list block.
/// Param b: The markdown list block.
/// Param dst: Destination block list.
/// Param rc: Resource collector used to register images.
/// Param depth: Current container nesting depth.
void mapList(
    md::Block& b,
    std::vector<sd::BlockPtr>& dst,
    ResourceCollector& rc,
    int depth
);

/// Maps one table cell to its paragraphs.
/// Param cell: Inline elements of the cell.
/// Returns The mapped table cell.
sd::TableCell mapCell(const std::vector<md::InlineElement>& cell);

/// Maps a markdown table block.
/// Param b: The markdown table block.
/// Param dst: Destination block list.
void mapTable(
    const md::Block& b,
    std::vector<sd::BlockPtr>& dst
);

} // namespace mapping::detail
