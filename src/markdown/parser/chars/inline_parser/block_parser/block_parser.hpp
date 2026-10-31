#pragma once

// Block markdown parser. The implementation is spread across impl/block_parser_*.cpp.

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

/// \brief Block-level markdown parser: turns normalized input lines into a Document AST.
class BlockParser {
  public:
    /// \brief Constructs the parser with a warning sink.
    /// \param warnings Vector collecting non-fatal warnings.
    explicit BlockParser(std::vector<std::string>& warnings) : mWarnings(warnings) {}

    /// \brief Parses the whole input text into a document.
    /// \param text Raw input text (normalized, front matter extracted).
    /// \return Parsed document with blocks, title and warnings.
    Document run(std::string text);

  private:
    std::vector<std::string> mLines;      ///< Input lines being parsed.
    size_t mCursor = 0;                   ///< Current line index.
    std::map<std::string, LinkDef> mDefs; ///< Link definitions found in the prescan.
    std::vector<std::string>& mWarnings;  ///< Warning sink.
    std::vector<BlockPtr> mPending;       ///< Blocks produced ahead of the current parse step.

    /// \brief An alignment HTML wrapper (<div/p/center align=...>).
    struct AlignWrap {
        std::string tag;  ///< Tag name.
        Align align;      ///< Alignment requested by the wrapper.
    };

    // --- impl/block_parser_input.cpp: input preparation (normalization, front matter, prescan) ---
    /// \brief Normalizes the input: strips the BOM, converts CR/CRLF to LF, expands leading tabs.
    /// \param s Raw input text.
    /// \return Normalized text.
    static std::string normalize(std::string s);

    /// \brief Expands leading tabs to spaces up to the next multiple-of-4 stop; inner
    /// tabs are left untouched (they can be code content).
    /// \param s Input text.
    /// \return Text with leading tabs expanded.
    static std::string expandLeadingTabs(const std::string& s);

    /// \brief Extracts the "---" front-matter block from the text, removing it in place.
    /// \param[in,out] text Full text; the front matter is removed from it.
    /// \return The front-matter title, or "" if there is none.
    static std::string extractFrontMatter(std::string* text);

    /// \brief Reads the "title:" field from front-matter text.
    /// \param fm Front-matter body.
    /// \return Unquoted title value, or "" if absent.
    static std::string frontMatterTitle(const std::string& fm);

    /// \brief Trims the value and removes one pair of matching surrounding quotes.
    /// \param v Raw value.
    /// \return Unquoted value.
    static std::string unquote(std::string_view v);

    /// \brief Splits text into lines by '\n'.
    /// \param text Input text.
    /// \return Lines without the newline characters.
    static std::vector<std::string> splitLines(const std::string& text);

    /// \brief Pre-scans lines for link definitions, records them in mDefs and blanks the lines.
    void prescanLinkDefs();

    // --- impl/block_parser.cpp: traversal skeleton ---
    /// \brief Parses one string into inline elements using the collected link definitions.
    /// \param s Source text.
    /// \return Parsed inline elements.
    std::vector<InlineElement> parseInline(const std::string& s);

    /// \brief Parses consecutive lines into blocks until the input is exhausted.
    /// \param depth Current nesting depth.
    /// \return Parsed blocks.
    std::vector<BlockPtr> parseBlocks(int depth);

    /// \brief Appends a block and then drains everything accumulated in mPending into out.
    /// \param b Block to append (may be null).
    /// \param out Output block list.
    void drainPending(BlockPtr b, std::vector<BlockPtr>& out);

    /// \brief Parses one block at mCursor, trying every block kind in order.
    /// \param depth Current nesting depth.
    /// \return Parsed block (a paragraph is the fallback).
    BlockPtr parseOne(int depth);

    /// \brief Recursively parses a subset of lines, saving/restoring the cursor position.
    /// \param inner Lines to parse.
    /// \param depth Current nesting depth.
    /// \return Parsed blocks of the inner range.
    std::vector<BlockPtr> parseInner(const std::vector<std::string>& inner, int depth);

    // --- impl/block_parser_blocks.cpp: comment, fence, ATX header, hr, indented code ---
    /// \brief Tries to consume an HTML comment at mCursor (possibly multi-line).
    /// \return Always nullptr; content after "-->" is re-inserted as a new line.
    BlockPtr tryComment();

    /// \brief Skips the rest of a line after "-->"; a non-empty remainder becomes a new line.
    /// \param after Position just past "-->".
    /// \return Always nullptr.
    BlockPtr skipRestOfComment(size_t after);

    /// \brief Tries to parse a fenced code block at mCursor.
    /// \return The code block, or nullptr if the line is not a fence.
    BlockPtr tryFence();

    /// \brief Collects the body lines of an open fence until its closing fence.
    /// \param fc Fence character.
    /// \param fl Opening fence length.
    /// \return Raw body lines, without the fences.
    std::vector<std::string> collectFenceBody(char fc, size_t fl);

    /// \brief Returns the first whitespace-delimited word of the string.
    /// \param s Input string.
    /// \return The first word (the whole string if there is no whitespace).
    static std::string firstWord(const std::string& s);

    /// \brief Joins body lines, removing up to \p indent leading spaces from each.
    /// \param body Body lines.
    /// \param indent Number of leading spaces to strip (at most what is there).
    /// \return Joined text with '\n' separators.
    static std::string dedentJoin(const std::vector<std::string>& body, size_t indent);

    /// \brief Tries to parse an ATX heading ("# ...") at mCursor.
    /// \return The header block, or nullptr if the line is not an ATX heading.
    BlockPtr tryAtx();

    /// \brief Removes trailing hashes (and the space before them) from heading text.
    /// \param text Heading text.
    /// \return Text without the closing hash sequence.
    static std::string stripTrailingHashes(std::string text);

    /// \brief Tries to parse a thematic break (hr) at mCursor.
    /// \return The rule block, or nullptr if the line is not an hr.
    BlockPtr tryHr();

    /// \brief Tries to parse an indented (4-space) code block at mCursor.
    /// \return The code block, or nullptr if the line is not indented enough.
    BlockPtr tryIndentedCode();

    /// \brief Collects the body of an indented code block (4-space prefixes removed).
    /// \return Body lines without trailing blanks.
    std::vector<std::string> collectIndentedCodeBody();

    /// \brief Checks whether the next non-blank line is indented by 4+ spaces.
    /// \return true if indented code continues after blank lines.
    bool nextNonBlankIndented() const;

    /// \brief Joins lines with '\n' separators.
    /// \param body Lines to join.
    /// \return Joined text.
    static std::string joinLines(const std::vector<std::string>& body);

    // --- impl/block_parser_align.cpp: HTML alignment wrappers ---
    /// \brief Tries to parse an alignment wrapper (<div/p/center align=...>) at mCursor.
    /// \param depth Current nesting depth.
    /// \return First aligned block (the rest go to mPending), or nullptr.
    BlockPtr tryAlignWrapper(int depth);

    /// \brief Single-line variant: <div align=...>text</div> on one line.
    /// \param tag Wrapper tag name.
    /// \param align Alignment to apply.
    /// \return The aligned paragraph, or nullptr if not a single-line wrapper.
    BlockPtr trySingleLineAlign(const std::string& tag, Align align);

    /// \brief Multi-line variant: parses the wrapper content and aligns the resulting paragraphs.
    /// \param wrap Wrapper description.
    /// \param depth Current nesting depth.
    /// \return First aligned block (the rest go to mPending), or nullptr.
    BlockPtr tryBlockAlign(const AlignWrap& wrap, int depth);

    /// \brief Detects an alignment-wrapper opening at the start of a line.
    /// \param line Source line.
    /// \return Wrapper description, or nullopt.
    static std::optional<AlignWrap> alignWrapper(const std::string& line);

    /// \brief Tries one specific wrapper tag at position p of the line.
    /// \param line Source line.
    /// \param p Position of the '<'.
    /// \param tag Tag name to try ("div", "p", "center").
    /// \return Wrapper description, or nullopt.
    static std::optional<AlignWrap> tryAlignTag(const std::string& line, size_t p, const char* tag);

    /// \brief Derives the alignment from attribute text; <center> defaults to center.
    /// \param lowAttrs Attribute text (search is case-insensitive).
    /// \param centerTag True when the tag itself is <center>.
    /// \return Decoded alignment (Align::None if absent/unknown).
    static Align alignFromAttrs(const std::string& lowAttrs, bool centerTag);

    /// \brief Applies the alignment to every paragraph in the list.
    /// \param blocks Blocks to update.
    /// \param align Alignment to apply.
    static void applyAlign(std::vector<BlockPtr>& blocks, Align align);

    // --- impl/block_parser_table.cpp: GFM tables ---
    /// \brief Tries to start a GFM table at mCursor (header line + delimiter line).
    /// \return The table block, or nullptr if the next lines are not a table.
    BlockPtr tryTableStart();

    /// \brief Builds a table block: parses the header cells and consumes the body rows.
    /// \param headerCells Raw header cell texts.
    /// \param aligns Column alignments from the delimiter row.
    /// \return The assembled table block.
    BlockPtr buildTable(const std::vector<std::string>& headerCells,
                        const std::vector<Align>& aligns);

    /// \brief Parses raw cell texts into inline elements.
    /// \param cells Raw cell texts.
    /// \return Parsed cells, one per input cell.
    std::vector<std::vector<InlineElement>> parseCells(const std::vector<std::string>& cells);

    // --- impl/block_parser_containers.cpp: quotes and lists ---
    /// \brief Tries to parse a block quote at mCursor.
    /// \param depth Current nesting depth.
    /// \return The quote block, or nullptr if the line does not start a quote.
    BlockPtr tryQuote(int depth);

    /// \brief Collects the quote body lines, honoring lazy continuation.
    /// \return Quote content with the markers stripped.
    std::vector<std::string> collectQuoteBody();

    /// \brief Checks whether the quote continues after blank lines (next text starts with '>').
    /// \return true if the quote resumes.
    bool continueQuoteAfterBlank() const;

    /// \brief Removes the '>' marker and one optional following space from the line.
    /// \param line Quote line.
    /// \return Line content without the marker.
    static std::string stripQuoteMarker(const std::string& line);

    /// \brief Checks whether the line starts any block construct.
    /// \param line Source line.
    /// \return true if the line can start a block (header, fence, list, wrapper, ...).
    bool isBlockStarter(const std::string& line) const;

    /// \brief Tries to parse a whole list (all consecutive items) at mCursor.
    /// \param depth Current nesting depth.
    /// \return The list block, or nullptr if the line does not start an item.
    BlockPtr tryList(int depth);

    /// \brief Peeks a list item of the given kind at mCursor (blank lines skipped);
    /// advances mCursor onto the marker line.
    /// \param ordered Whether an ordered item is expected.
    /// \return The item marker, or nullopt.
    std::optional<Marker> peekListItem(bool ordered);

    /// \brief Gathers all lines of one list item (continuations, nested content, blanks).
    /// \param m The item marker.
    /// \return Item lines relative to the content indent, without trailing blanks.
    std::vector<std::string> gatherItem(const Marker& m);

    /// \brief Handles a blank line inside a list item: keeps it if the item continues.
    /// \param[in,out] item Item lines collected so far.
    /// \param contentIndent Content indent of the item.
    /// \return true if the blank line was consumed as part of the item.
    bool handleItemBlank(std::vector<std::string>& item, size_t contentIndent);

    /// \brief Turns raw item lines into a ListItem: strips the task prefix, parses children.
    /// \param raw Item lines.
    /// \param[out] out Destination list of items.
    /// \param depth Current nesting depth.
    /// \return true if the item turned out to be a task item.
    bool makeListItem(std::vector<std::string> raw, std::vector<ListItem>& out, int depth);

    /// \brief Strips a "[ ]"/"[x]" task prefix from the first item line, setting the flags.
    /// \param[in,out] raw Item lines (the first line is modified in place).
    /// \param[out] item Item whose task/checked flags are set.
    static void stripTaskPrefix(std::vector<std::string>& raw, ListItem& item);

    // --- impl/block_parser_paragraph.cpp: paragraphs ---
    /// \brief Checks whether the line interrupts a paragraph (header, fence, list, table, ...).
    /// \param line Source line.
    /// \param idx Index of the line (used to peek at the next line for tables).
    /// \return true if the line terminates the current paragraph.
    bool interruptsParagraph(const std::string& line, size_t idx) const;

    /// \brief Parses a paragraph at mCursor (with setext/table special cases and hard breaks).
    /// \return The paragraph block.
    BlockPtr parseParagraph();

    /// \brief Handles a setext underline or a GFM table "over" the last paragraph line.
    /// \param[in,out] collected Paragraph lines collected so far.
    /// \param line The candidate special line.
    /// \return The produced block (header/table), or nullptr if the line is ordinary.
    BlockPtr tryParagraphSpecial(std::vector<std::pair<std::string, bool>>& collected,
                                 const std::string& line);

    /// \brief A delimiter line turns the previous paragraph line into a table header.
    /// \param headerLine Line that becomes the header.
    /// \param delimLine Delimiter row.
    /// \return The table block, or nullptr if not a valid table start.
    BlockPtr tableFromDelimiter(const std::string& headerLine, const std::string& delimLine);

    /// \brief Appends a paragraph line as (text, hard-break-after). Two or more trailing
    /// spaces mean a hard break, and so does an odd number of trailing backslashes
    /// (an even count stands for escaped pairs).
    /// \param[in,out] collected Collected (text, hard-break) pairs.
    /// \param l The raw line.
    static void addParagraphLine(std::vector<std::pair<std::string, bool>>& collected,
                                 std::string l);

    /// \brief Counts trailing backslashes at the end of the line.
    /// \param l Input line.
    /// \return Number of consecutive trailing '\' characters.
    static size_t trailingBackslashes(const std::string& l);

    /// \brief Joins collected paragraph lines into one string.
    /// \param lines (text, hard-break-after) pairs.
    /// \param useBreaks Whether hard breaks become '\n' instead of ' '.
    /// \return Joined text.
    static std::string joinCollected(const std::vector<std::pair<std::string, bool>>& lines,
                                     bool useBreaks);
};

} // namespace md::detail
