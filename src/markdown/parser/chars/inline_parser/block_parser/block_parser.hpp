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

/// Block-level markdown parser: turns normalized input lines into a Document AST.
class BlockParser {
  public:
    /// Constructs the parser with a warning sink.
    /// Param warnings: Vector collecting non-fatal warnings.
    explicit BlockParser(std::vector<std::string>& warnings) : mWarnings(warnings) {}

    /// Parses the whole input text into a document.
    /// Param text: Raw input text (normalized, front matter extracted).
    /// Returns Parsed document with blocks, title and warnings.
    Document run(std::string text);

  private:
    std::vector<std::string> mLines;      ///< Input lines being parsed.
    size_t mCursor = 0;                   ///< Current line index.
    std::map<std::string, LinkDef> mDefs; ///< Link definitions found in the prescan.
    std::vector<std::string>& mWarnings;  ///< Warning sink.
    std::vector<BlockPtr> mPending;       ///< Blocks produced ahead of the current parse step.

    /// An alignment HTML wrapper (<div/p/center align=...>).
    struct AlignWrap {
        std::string tag; ///< Tag name.
        Align align;     ///< Alignment requested by the wrapper.
    };

    // --- impl/block_parser_input.cpp: input preparation (normalization, front matter, prescan) ---
    /// Normalizes the input: strips the BOM, converts CR/CRLF to LF, expands leading tabs.
    /// Param s: Raw input text.
    /// Returns Normalized text.
    static std::string normalize(std::string s);

    /// Expands leading tabs to spaces up to the next multiple-of-4 stop; inner
    /// tabs are left untouched (they can be code content).
    /// Param s: Input text.
    /// Returns Text with leading tabs expanded.
    static std::string expandLeadingTabs(const std::string& s);

    /// Extracts the "---" front-matter block from the text, removing it in place.
    /// \param[in,out] text Full text; the front matter is removed from it.
    /// Returns The front-matter title, or "" if there is none.
    static std::string extractFrontMatter(std::string* text);

    /// Reads the "title:" field from front-matter text.
    /// Param fm: Front-matter body.
    /// Returns Unquoted title value, or "" if absent.
    static std::string frontMatterTitle(const std::string& fm);

    /// Trims the value and removes one pair of matching surrounding quotes.
    /// Param v: Raw value.
    /// Returns Unquoted value.
    static std::string unquote(std::string_view v);

    /// Splits text into lines by '\n'.
    /// Param text: Input text.
    /// Returns Lines without the newline characters.
    static std::vector<std::string> splitLines(const std::string& text);

    /// Pre-scans lines for link definitions, records them in mDefs and blanks the lines.
    void prescanLinkDefs();

    // --- impl/block_parser.cpp: traversal skeleton ---
    /// Parses one string into inline elements using the collected link definitions.
    /// Param s: Source text.
    /// Returns Parsed inline elements.
    std::vector<InlineElement> parseInline(const std::string& s);

    /// Parses consecutive lines into blocks until the input is exhausted.
    /// Param depth: Current nesting depth.
    /// Returns Parsed blocks.
    std::vector<BlockPtr> parseBlocks(int depth);

    /// Appends a block and then drains everything accumulated in mPending into out.
    /// Param b: Block to append (may be null).
    /// Param out: Output block list.
    void drainPending(
        BlockPtr b,
        std::vector<BlockPtr>& out
    );

    /// Parses one block at mCursor, trying every block kind in order.
    /// Param depth: Current nesting depth.
    /// Returns Parsed block (a paragraph is the fallback).
    BlockPtr parseOne(int depth);

    /// Recursively parses a subset of lines, saving/restoring the cursor position.
    /// Param inner: Lines to parse.
    /// Param depth: Current nesting depth.
    /// Returns Parsed blocks of the inner range.
    std::vector<BlockPtr> parseInner(
        const std::vector<std::string>& inner,
        int depth
    );

    // --- impl/block_parser_blocks.cpp: comment, fence, ATX header, hr, indented code ---
    /// Tries to consume an HTML comment at mCursor (possibly multi-line).
    /// Returns Always nullptr; content after "-->" is re-inserted as a new line.
    BlockPtr tryComment();

    /// Skips the rest of a line after "-->"; a non-empty remainder becomes a new line.
    /// Param after: Position just past "-->".
    /// Returns Always nullptr.
    BlockPtr skipRestOfComment(size_t after);

    /// Tries to parse a fenced code block at mCursor.
    /// Returns The code block, or nullptr if the line is not a fence.
    BlockPtr tryFence();

    /// Collects the body lines of an open fence until its closing fence.
    /// Param fc: Fence character.
    /// Param fl: Opening fence length.
    /// Returns Raw body lines, without the fences.
    std::vector<std::string> collectFenceBody(
        char fc,
        size_t fl
    );

    /// Returns the first whitespace-delimited word of the string.
    /// Param s: Input string.
    /// Returns The first word (the whole string if there is no whitespace).
    static std::string firstWord(const std::string& s);

    /// Joins body lines, removing up to \p indent leading spaces from each.
    /// Param body: Body lines.
    /// Param indent: Number of leading spaces to strip (at most what is there).
    /// Returns Joined text with '\n' separators.
    static std::string dedentJoin(
        const std::vector<std::string>& body,
        size_t indent
    );

    /// Tries to parse an ATX heading ("# ...") at mCursor.
    /// Returns The header block, or nullptr if the line is not an ATX heading.
    BlockPtr tryAtx();

    /// Removes trailing hashes (and the space before them) from heading text.
    /// Param text: Heading text.
    /// Returns Text without the closing hash sequence.
    static std::string stripTrailingHashes(std::string text);

    /// Tries to parse a thematic break (hr) at mCursor.
    /// Returns The rule block, or nullptr if the line is not an hr.
    BlockPtr tryHr();

    /// Tries to parse an indented (4-space) code block at mCursor.
    /// Returns The code block, or nullptr if the line is not indented enough.
    BlockPtr tryIndentedCode();

    /// Collects the body of an indented code block (4-space prefixes removed).
    /// Returns Body lines without trailing blanks.
    std::vector<std::string> collectIndentedCodeBody();

    /// Checks whether the next non-blank line is indented by 4+ spaces.
    /// Returns true if indented code continues after blank lines.
    bool nextNonBlankIndented() const;

    /// Joins lines with '\n' separators.
    /// Param body: Lines to join.
    /// Returns Joined text.
    static std::string joinLines(const std::vector<std::string>& body);

    // --- impl/block_parser_align.cpp: HTML alignment wrappers ---
    /// Tries to parse an alignment wrapper (<div/p/center align=...>) at mCursor.
    /// Param depth: Current nesting depth.
    /// Returns First aligned block (the rest go to mPending), or nullptr.
    BlockPtr tryAlignWrapper(int depth);

    /// Single-line variant: <div align=...>text</div> on one line.
    /// Param tag: Wrapper tag name.
    /// Param align: Alignment to apply.
    /// Returns The aligned paragraph, or nullptr if not a single-line wrapper.
    BlockPtr trySingleLineAlign(
        const std::string& tag,
        Align align
    );

    /// Multi-line variant: parses the wrapper content and aligns the resulting paragraphs.
    /// Param wrap: Wrapper description.
    /// Param depth: Current nesting depth.
    /// Returns First aligned block (the rest go to mPending), or nullptr.
    BlockPtr tryBlockAlign(
        const AlignWrap& wrap,
        int depth
    );

    /// Detects an alignment-wrapper opening at the start of a line.
    /// Param line: Source line.
    /// Returns Wrapper description, or nullopt.
    static std::optional<AlignWrap> alignWrapper(const std::string& line);

    /// Tries one specific wrapper tag at position p of the line.
    /// Param line: Source line.
    /// Param p: Position of the '<'.
    /// Param tag: Tag name to try ("div", "p", "center").
    /// Returns Wrapper description, or nullopt.
    static std::optional<AlignWrap> tryAlignTag(
        const std::string& line,
        size_t p,
        const char* tag
    );

    /// Derives the alignment from attribute text; <center> defaults to center.
    /// Param lowAttrs: Attribute text (search is case-insensitive).
    /// Param centerTag: True when the tag itself is <center>.
    /// Returns Decoded alignment (Align::None if absent/unknown).
    static Align alignFromAttrs(
        const std::string& lowAttrs,
        bool centerTag
    );

    /// Applies the alignment to every paragraph in the list.
    /// Param blocks: Blocks to update.
    /// Param align: Alignment to apply.
    static void applyAlign(
        std::vector<BlockPtr>& blocks,
        Align align
    );

    // --- impl/block_parser_table.cpp: GFM tables ---
    /// Tries to start a GFM table at mCursor (header line + delimiter line).
    /// Returns The table block, or nullptr if the next lines are not a table.
    BlockPtr tryTableStart();

    /// Builds a table block: parses the header cells and consumes the body rows.
    /// Param headerCells: Raw header cell texts.
    /// Param aligns: Column alignments from the delimiter row.
    /// Returns The assembled table block.
    BlockPtr buildTable(
        const std::vector<std::string>& headerCells,
        const std::vector<Align>& aligns
    );

    /// Parses raw cell texts into inline elements.
    /// Param cells: Raw cell texts.
    /// Returns Parsed cells, one per input cell.
    std::vector<std::vector<InlineElement>> parseCells(const std::vector<std::string>& cells);

    // --- impl/block_parser_containers.cpp: quotes and lists ---
    /// Tries to parse a block quote at mCursor.
    /// Param depth: Current nesting depth.
    /// Returns The quote block, or nullptr if the line does not start a quote.
    BlockPtr tryQuote(int depth);

    /// Collects the quote body lines, honoring lazy continuation.
    /// Returns Quote content with the markers stripped.
    std::vector<std::string> collectQuoteBody();

    /// Checks whether the quote continues after blank lines (next text starts with '>').
    /// Returns true if the quote resumes.
    bool continueQuoteAfterBlank() const;

    /// Removes the '>' marker and one optional following space from the line.
    /// Param line: Quote line.
    /// Returns Line content without the marker.
    static std::string stripQuoteMarker(const std::string& line);

    /// Checks whether the line starts any block construct.
    /// Param line: Source line.
    /// Returns true if the line can start a block (header, fence, list, wrapper, ...).
    bool isBlockStarter(const std::string& line) const;

    /// Tries to parse a list at mCursor: one run of consecutive items of the
    /// same kind (checkbox or plain); a kind change starts a new list block.
    /// Param depth: Current nesting depth.
    /// Returns The list block, or nullptr if the line does not start an item.
    BlockPtr tryList(int depth);

    /// Peeks a list item of the given kind at mCursor; does not advance.
    /// Blank lines are not skipped: per the list policy a blank line
    /// terminates the collection, so it fails the peek.
    /// Param ordered: Whether an ordered item is expected.
    /// Returns The item marker, or nullopt.
    std::optional<Marker> peekListItem(bool ordered);

    /// Gathers the lines of one list item: only lines indented at least to
    /// the item content. A blank line or a smaller indent ends the item
    /// (and the whole list); there is no lazy continuation across it.
    /// Param m: The item marker.
    /// Returns Item lines relative to the content indent, without trailing blanks.
    std::vector<std::string> gatherItem(const Marker& m);

    /// Turns raw item lines into a ListItem: strips the task prefix, parses children.
    /// Param raw: Item lines.
    /// \param[out] out Destination list of items.
    /// Param depth: Current nesting depth.
    void makeListItem(
        std::vector<std::string> raw,
        std::vector<ListItem>& out,
        int depth
    );

    /// Strips a "[ ]"/"[x]" task prefix from the first item line, setting the flags.
    /// \param[in,out] raw Item lines (the first line is modified in place).
    /// \param[out] item Item whose task/checked flags are set.
    static void stripTaskPrefix(
        std::vector<std::string>& raw,
        ListItem& item
    );

    // --- impl/block_parser_paragraph.cpp: paragraphs ---
    /// Checks whether the line interrupts a paragraph (header, fence, list, table, ...).
    /// Param line: Source line.
    /// Param idx: Index of the line (used to peek at the next line for tables).
    /// Returns true if the line terminates the current paragraph.
    bool interruptsParagraph(const std::string& line, size_t idx) const;

    /// Parses a paragraph at mCursor (with setext/table special cases and hard breaks).
    /// Returns The paragraph block.
    BlockPtr parseParagraph();

    /// Handles a setext underline or a GFM table "over" the last paragraph line.
    /// \param[in,out] collected Paragraph lines collected so far.
    /// Param line: The candidate special line.
    /// Returns The produced block (header/table), or nullptr if the line is ordinary.
    BlockPtr tryParagraphSpecial(
        std::vector<std::pair<std::string, bool>>& collected,
        const std::string& line
    );

    /// A delimiter line turns the previous paragraph line into a table header.
    /// Param headerLine: Line that becomes the header.
    /// Param delimLine: Delimiter row.
    /// Returns The table block, or nullptr if not a valid table start.
    BlockPtr tableFromDelimiter(
        const std::string& headerLine,
        const std::string& delimLine
    );

    /// Appends a paragraph line as (text, hard-break-after). Two or more trailing
    /// spaces mean a hard break, and so does an odd number of trailing backslashes
    /// (an even count stands for escaped pairs).
    /// \param[in,out] collected Collected (text, hard-break) pairs.
    /// Param l: The raw line.
    static void addParagraphLine(std::vector<std::pair<std::string, bool>>& collected,
                                 std::string l);

    /// Counts trailing backslashes at the end of the line.
    /// Param l: Input line.
    /// Returns Number of consecutive trailing '\' characters.
    static size_t trailingBackslashes(const std::string& l);

    /// Joins collected paragraph lines into one string.
    /// Param lines: (text, hard-break-after) pairs.
    /// Param useBreaks: Whether hard breaks become '\n' instead of ' '.
    /// Returns Joined text.
    static std::string joinCollected(const std::vector<std::pair<std::string, bool>>& lines,
                                     bool useBreaks);
};

} // namespace md::detail
