#pragma once

// Internal mapping functions from the md AST to the SabyDoc model (for impl/map_*.cpp).
// All translation policy (h6, hr, separators, placeholders) lives here.

#include <string>
#include <vector>

#include "markdown/ast/ast.hpp"
#include "sabydoc/model/model.hpp"
#include "sabydoc/model/resource_collector/resource_collector.hpp"

namespace mapping::detail {

/// \brief Maximum container nesting depth accepted while mapping.
constexpr int kMaxDepth = 64;

/// \brief Converts a markdown alignment value to its sabydoc string form.
/// \param a The markdown alignment.
/// \return One of "", "center", "right", "justify".
std::string alignStr(md::Align a);

/// \brief Tells whether a block of this kind must be followed by a separator empty paragraph.
/// \param k The block kind.
/// \return true when a trailing separator is required.
bool needsTrailingSeparator(sd::Block::Kind k);

/// \brief Creates a new block of the given kind with a fresh id.
/// \param kind The kind of block to create.
/// \return The newly created block.
std::unique_ptr<sd::Block> newBlock(sd::Block::Kind kind);

/// \brief Creates a paragraph block mimicking the editor's separator empty paragraph.
/// \return The separator-mimicking block.
std::unique_ptr<sd::Block> makeSeparatorMimic();

/// \brief Creates a placeholder paragraph for an unavailable image.
/// \param url The image reference that could not be loaded.
/// \return The placeholder block.
std::unique_ptr<sd::Block> makePlaceholder(const std::string& url);

/// \brief Creates a caption paragraph for an image.
/// \param text The caption text.
/// \return The caption block.
std::unique_ptr<sd::Block> makeCaption(const std::string& text);

// --- impl/map_content.cpp: factories, images, paragraphs, headers, quotes ---
/// \brief Emits image (and optional caption) blocks for a markdown image.
/// \param img The markdown image element.
/// \param dst Destination list for the emitted blocks.
/// \param rc Resource collector used to register the image.
void emitImage(const md::InlineImage& img, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc);

/// \brief Drops trailing empty inline runs (e.g. line-break tails) from a group.
/// \param elems The element list to trim in place.
void dropTrailingEmptyRuns(std::vector<md::InlineElement>& elems);

/// \brief Maps one aligned inline group to a paragraph block.
/// \param group Inline elements of the group.
/// \param align Text alignment of the group.
/// \param dst Destination block list.
/// \param rc Resource collector used to register images.
/// \param allowImages Whether images may be emitted here.
void mapGroup(const std::vector<md::InlineElement>& group, md::Align align,
              std::vector<sd::BlockPtr>& dst, ResourceCollector& rc, bool allowImages);

/// \brief Maps a markdown paragraph block.
/// \param b The markdown paragraph block.
/// \param dst Destination block list.
/// \param rc Resource collector used to register images.
/// \param allowImages Whether images may be emitted here.
void mapParagraph(const md::Block& b, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc,
                  bool allowImages);

/// \brief Flattens nested quote blocks into a single-level block list.
/// \param src Source markdown blocks.
/// \param out Output list of flattened blocks.
void flattenQuotes(std::vector<md::BlockPtr>& src, std::vector<md::BlockPtr>& out);

/// \brief Maps a markdown header block.
/// \param b The markdown header block.
/// \param dst Destination block list.
/// \param topLevel Whether the header sits at the document top level.
void mapHeader(const md::Block& b, std::vector<sd::BlockPtr>& dst, bool topLevel);

// --- impl/map_blocks.cpp: dispatcher, separator policy ---

/// \brief Maps a single markdown block by dispatching on its kind.
/// \param b The markdown block to map.
/// \param dst Destination block list.
/// \param rc Resource collector used to register images.
/// \param topLevel Whether the block sits at the document top level.
/// \param depth Current container nesting depth.
void mapSingleBlock(md::BlockPtr& b, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc,
                    bool topLevel, int depth);

/// \brief Maps a list of markdown blocks.
/// \param src Source markdown blocks.
/// \param dst Destination block list.
/// \param rc Resource collector used to register images.
/// \param topLevel Whether the blocks sit at the document top level.
/// \param depth Current container nesting depth.
void mapBlocks(std::vector<md::BlockPtr>& src, std::vector<sd::BlockPtr>& dst,
               ResourceCollector& rc, bool topLevel, int depth);

// --- impl/map_containers.cpp: lists and tables ---

/// \brief Maps a single markdown list item with its nested children.
/// \param item The markdown list item.
/// \param rc Resource collector used to register images.
/// \param depth Current container nesting depth.
/// \return The mapped sabydoc list item.
sd::ListItem mapListItem(md::ListItem& item, ResourceCollector& rc, int depth);

/// \brief Chooses the sabydoc list marker for a markdown list.
/// \param kind The markdown list kind.
/// \param anyTask Whether any item of the list is a task item.
/// \return The matching sabydoc list marker.
sd::ListMarker listMarkerOf(md::ListKind kind, bool anyTask);

/// \brief Maps a markdown list block.
/// \param b The markdown list block.
/// \param dst Destination block list.
/// \param rc Resource collector used to register images.
/// \param depth Current container nesting depth.
void mapList(md::Block& b, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc, int depth);

/// \brief Maps one table cell to its paragraphs.
/// \param cell Inline elements of the cell.
/// \return The mapped table cell.
sd::TableCell mapCell(const std::vector<md::InlineElement>& cell);

/// \brief Maps a markdown table block.
/// \param b The markdown table block.
/// \param dst Destination block list.
void mapTable(const md::Block& b, std::vector<sd::BlockPtr>& dst);

} // namespace mapping::detail
