#pragma once

#include <memory>
#include <string>
#include <vector>

/// \brief Markdown AST. The single data model for BOTH conversion directions:
/// md -> sabydoc (text parser -> AST) and, in the future, sabydoc -> md (AST -> text).
namespace md {

/// \brief Horizontal text alignment.
enum class Align { None, Left, Center, Right, Justify };

/// \brief Set of character-style flags applied to an inline text run.
struct StyleFlags {
    bool bold = false;       ///< Bold style.
    bool italic = false;     ///< Italic style.
    bool strike = false;     ///< Strikethrough style.
    bool underline = false;  ///< Underline style.
    bool code = false;  ///< Code span; not rendered in sabydoc — text only.
    /// \brief Defaulted equality comparison.
    /// \return true if all flags are equal.
    bool operator==(const StyleFlags&) const = default;
};

/// \brief A run of styled text.
struct InlineRun {
    std::string text;  ///< UTF-8 text; an embedded '\n' is a hard line break.
    StyleFlags style;  ///< Style flags applied to the whole run.
    std::string link;  ///< Target URL; "" means "not a link".
};

/// \brief An inline image.
struct InlineImage {
    std::string url;    ///< Image URL.
    std::string alt;    ///< Alternative text.
    std::string title;  ///< Optional title.
};

/// \brief A tagged union of inline element kinds (text run or image).
struct InlineElement {
    /// \brief Discriminator selecting the active member.
    enum class Kind { Run, Image } kind = Kind::Run;
    InlineRun run;     ///< Valid when kind == Kind::Run.
    InlineImage image; ///< Valid when kind == Kind::Image.
};

/// \brief A block-level AST node (paragraph, header, code, quote, list, table, rule).
struct Block;
using BlockPtr = std::unique_ptr<Block>;

/// \brief A single item of a list block.
struct ListItem {
    std::vector<InlineElement> content;  ///< Item text (the first paragraph).
    std::vector<BlockPtr> children;      ///< Nested lists, paragraphs, code, etc.
    bool task = false;     ///< True when the item is a task-list item.
    bool checked = false;  ///< Task checkbox state.
};

/// \brief Kind of a list block.
enum class ListKind { Bullet, Ordered, Task };

/// \brief A block-level AST node; only the members relevant to \c type are used.
struct Block {
    /// \brief Discriminator for the block kind.
    enum class Type { Paragraph, Header, Code, Quote, List, Table, Rule };
    Type type = Type::Paragraph;  ///< Kind of this block.

    // paragraph / header
    std::vector<InlineElement> inlines;
    Align align = Align::None;
    int level = 0;  // header level 1..6

    // code
    std::string codeText;
    std::string lang;

    // quote
    std::vector<BlockPtr> children;

    // list
    ListKind listKind = ListKind::Bullet;
    std::vector<ListItem> items;

    // table: rows[row][column]; row 0 is the header
    std::vector<Align> colAlign;
    std::vector<std::vector<std::vector<InlineElement>>> rows;
};

/// \brief Root node of a parsed markdown document.
struct Document {
    std::vector<BlockPtr> blocks;       ///< Top-level blocks.
    std::string title;  // from front matter ("title: ..."), if present
    std::vector<std::string> warnings;  ///< Non-fatal warnings collected while parsing.
};

/// \brief Concatenates inline elements into flat plain text (for TOC, document title, captions).
/// \param elems Inline elements to flatten.
/// \return Concatenated plain text (image alt text is used for images).
std::string plainText(const std::vector<InlineElement>& elems);

/// \brief Splits inline elements at hard line breaks '\n' (each group is a separate paragraph).
/// \param elems Inline elements to split.
/// \return Groups of inline elements, one per line.
std::vector<std::vector<InlineElement>> splitByLineBreaks(const std::vector<InlineElement>& elems);

/// \brief Replaces '\n' with a space (used for headers, table cells, list items).
/// \param elems Inline elements to process.
/// \return Elements with hard line breaks flattened to spaces.
std::vector<InlineElement> flattenLineBreaks(const std::vector<InlineElement>& elems);

} // namespace md
