#pragma once

// Внутренние функции маппинга md AST -> модель SabyDoc (для impl/map_*.cpp).
// Вся политика перевода (h6, hr, разделители, заглушки) живёт здесь.

#include <string>
#include <vector>

#include "markdown/ast/ast.hpp"
#include "sabydoc/model/model.hpp"
#include "sabydoc/model/resource_collector/resource_collector.hpp"

namespace mapping::detail {

constexpr int kMaxDepth = 64;

std::string alignStr(md::Align a);
bool needsTrailingSeparator(sd::Block::Kind k);
std::unique_ptr<sd::Block> newBlock(sd::Block::Kind kind);
std::unique_ptr<sd::Block> makeSeparatorMimic();
std::unique_ptr<sd::Block> makePlaceholder(const std::string& url);
std::unique_ptr<sd::Block> makeCaption(const std::string& text);

// --- impl/map_content.cpp: фабрики, картинки, абзацы, заголовки, цитаты ---
void emitImage(const md::InlineImage& img, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc);
void dropTrailingEmptyRuns(std::vector<md::InlineElement>& elems);
void mapGroup(const std::vector<md::InlineElement>& group, md::Align align,
              std::vector<sd::BlockPtr>& dst, ResourceCollector& rc, bool allowImages);
void mapParagraph(const md::Block& b, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc,
                  bool allowImages);
void flattenQuotes(std::vector<md::BlockPtr>& src, std::vector<md::BlockPtr>& out);
void mapHeader(const md::Block& b, std::vector<sd::BlockPtr>& dst, bool topLevel);

// --- impl/map_blocks.cpp: диспетчер, политика разделителей ---
void mapSingleBlock(md::BlockPtr& b, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc,
                    bool topLevel, int depth);
void mapBlocks(std::vector<md::BlockPtr>& src, std::vector<sd::BlockPtr>& dst,
               ResourceCollector& rc, bool topLevel, int depth);

// --- impl/map_containers.cpp: списки и таблицы ---
sd::ListItem mapListItem(md::ListItem& item, ResourceCollector& rc, int depth);
sd::ListMarker listMarkerOf(md::ListKind kind, bool anyTask);
void mapList(md::Block& b, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc, int depth);
sd::TableCell mapCell(const std::vector<md::InlineElement>& cell);
void mapTable(const md::Block& b, std::vector<sd::BlockPtr>& dst);

} // namespace mapping::detail
