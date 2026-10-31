#pragma once

// Блочный парсер markdown. Реализация распределена по impl/block_parser_*.cpp.

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "algo/algo.hpp"
#include "markdown/ast/ast.hpp"
#include "markdown/parser/chars/chars.hpp"
#include "markdown/parser/chars/detectors/detectors.hpp"
#include "markdown/parser/chars/inline_parser/inline_parser.hpp"
#include "markdown/parser/chars/link_def/link_def.hpp"
#include "markdown/parser/chars/marker/marker.hpp"
#include "markdown/parser/chars/table_row/table_row.hpp"

namespace md::detail {

class BlockParser {
  public:
    explicit BlockParser(std::vector<std::string>& warnings) : mWarnings(warnings) {}

    Document run(std::string text);

  private:
    std::vector<std::string> mLines;
    size_t mCursor = 0;
    std::map<std::string, LinkDef> mDefs;
    std::vector<std::string>& mWarnings;
    std::vector<BlockPtr> mPending;

    struct AlignWrap {
        std::string tag;
        Align align;
    };

    // --- impl/block_parser_input.cpp: подготовка входа (нормализация, front matter, предскан) ---
    static std::string normalize(std::string s);
    static std::string expandLeadingTabs(const std::string& s);
    static std::string extractFrontMatter(std::string* text);
    static std::string frontMatterTitle(const std::string& fm);
    static std::string unquote(std::string_view v);
    static std::vector<std::string> splitLines(const std::string& text);
    void prescanLinkDefs();

    // --- impl/block_parser.cpp: каркас обхода ---
    std::vector<InlineElement> parseInline(const std::string& s);
    std::vector<BlockPtr> parseBlocks(int depth);
    void drainPending(BlockPtr b, std::vector<BlockPtr>& out);
    BlockPtr parseOne(int depth);
    std::vector<BlockPtr> parseInner(const std::vector<std::string>& inner, int depth);

    // --- impl/block_parser_blocks.cpp: комментарий, fence, ATX, hr, код с отступом ---
    BlockPtr tryComment();
    BlockPtr skipRestOfComment(size_t after);
    BlockPtr tryFence();
    std::vector<std::string> collectFenceBody(char fc, size_t fl);
    static std::string firstWord(const std::string& s);
    static std::string dedentJoin(const std::vector<std::string>& body, size_t indent);
    BlockPtr tryAtx();
    static std::string stripTrailingHashes(std::string text);
    BlockPtr tryHr();
    BlockPtr tryIndentedCode();
    std::vector<std::string> collectIndentedCodeBody();
    bool nextNonBlankIndented() const;
    static std::string joinLines(const std::vector<std::string>& body);

    // --- impl/block_parser_align.cpp: html-обёртки выравнивания ---
    BlockPtr tryAlignWrapper(int depth);
    BlockPtr trySingleLineAlign(const std::string& tag, Align align);
    BlockPtr tryBlockAlign(const AlignWrap& wrap, int depth);
    static std::optional<AlignWrap> alignWrapper(const std::string& line);
    static std::optional<AlignWrap> tryAlignTag(const std::string& line, size_t p, const char* tag);
    static Align alignFromAttrs(const std::string& lowAttrs, bool centerTag);
    static void applyAlign(std::vector<BlockPtr>& blocks, Align align);

    // --- impl/block_parser_table.cpp: GFM-таблицы ---
    BlockPtr tryTableStart();
    BlockPtr buildTable(const std::vector<std::string>& headerCells,
                        const std::vector<Align>& aligns);
    std::vector<std::vector<InlineElement>> parseCells(const std::vector<std::string>& cells);

    // --- impl/block_parser_containers.cpp: цитаты и списки ---
    BlockPtr tryQuote(int depth);
    std::vector<std::string> collectQuoteBody();
    bool continueQuoteAfterBlank() const;
    static std::string stripQuoteMarker(const std::string& line);
    bool isBlockStarter(const std::string& line) const;
    BlockPtr tryList(int depth);
    std::optional<Marker> peekListItem(bool ordered);
    std::vector<std::string> gatherItem(const Marker& m);
    bool handleItemBlank(std::vector<std::string>& item, size_t contentIndent);
    bool makeListItem(std::vector<std::string> raw, std::vector<ListItem>& out, int depth);
    static void stripTaskPrefix(std::vector<std::string>& raw, ListItem& item);

    // --- impl/block_parser_paragraph.cpp: абзацы ---
    bool interruptsParagraph(const std::string& line, size_t idx) const;
    BlockPtr parseParagraph();
    BlockPtr tryParagraphSpecial(std::vector<std::pair<std::string, bool>>& collected,
                                 const std::string& line);
    BlockPtr tableFromDelimiter(const std::string& headerLine, const std::string& delimLine);
    static void addParagraphLine(std::vector<std::pair<std::string, bool>>& collected,
                                 std::string l);
    static size_t trailingBackslashes(const std::string& l);
    static std::string joinCollected(const std::vector<std::pair<std::string, bool>>& lines,
                                     bool useBreaks);
};

} // namespace md::detail
