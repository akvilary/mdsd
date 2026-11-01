#pragma once

// Inline markdown parser: emphasis, code spans, links, images, entities, HTML tags.
// The implementation is spread across impl/inline_parser_*.cpp.

#include <map>
#include <string>
#include <vector>

#include "algo/algo.hpp"
#include "markdown/ast/ast.hpp"
#include "markdown/parser/chars/entity/entity.hpp"
#include "markdown/parser/chars/html_tag/html_tag.hpp"
#include "markdown/parser/chars/link_def/link_def.hpp"

namespace md::detail {

/// Inline markdown parser: emphasis, code spans, links, images, entities, HTML tags.
class InlineParser {
  public:
    /// Constructs the parser over a set of link definitions and a warning sink.
    /// Param defs: Reference link definitions collected at the block level.
    /// Param warnings: Vector collecting non-fatal warnings.
    explicit InlineParser(const std::map<std::string, LinkDef>& defs,
                          std::vector<std::string>& warnings)
        : mDefs(defs), mWarnings(warnings) {}

    /// Parses a string into inline elements.
    /// Param s: Source text.
    /// Returns Parsed inline elements.
    std::vector<InlineElement> parse(const std::string& s);

    /// Context of a single parseRec traversal: the string being parsed, the style
    /// and link accumulated so far (for nesting), and the recursion depth. Created in
    /// parse()/recursive calls and passed to every try* handler.
    struct Ctx {
        const std::string& s; ///< The string being parsed.
        StyleFlags st;        ///< Style accumulated from enclosing constructs.
        std::string link;     ///< Link URL accumulated from enclosing constructs.
        int depth;            ///< Current recursion depth.
    };

  private:
    const std::map<std::string, LinkDef>& mDefs;
    std::vector<std::string>& mWarnings;

    // --- impl/inline_parser_core.cpp: traversal skeleton, escapes, code spans, entities ---
    /// Records a warning message.
    /// Param msg: Message text.
    void warn(const std::string& msg) const;

    /// Flushes the accumulated text buffer into out as a styled run.
    /// Param c: Current traversal context.
    /// Param out: Output element list.
    /// Param buf: Accumulated plain-text buffer; emptied by the call.
    void emitRun(const Ctx& c, std::vector<InlineElement>& out, std::string& buf) const;

    /// Main traversal: dispatches on the current character; a try* handler
    /// returns how many characters it consumed (0 = not its construct, the
    /// character then goes to the buffer).
    /// Param c: Current traversal context.
    /// Returns Parsed inline elements.
    std::vector<InlineElement> parseRec(const Ctx& c) const;

    /// Merges adjacent runs with identical style and link into single runs.
    /// Param elems: Elements to merge.
    /// Returns Merged element list without trailing empty runs.
    static std::vector<InlineElement> mergeRuns(std::vector<InlineElement> elems);

    /// Tries to parse a backslash escape at position i.
    /// Param c: Current traversal context.
    /// Param i: Position of the backslash.
    /// Param buf: Buffer receiving the escaped character.
    /// Returns Number of characters consumed (2), or 0 if not an escape.
    static size_t tryEscape(
        const Ctx& c,
        size_t i,
        std::string& buf
    );

    /// Tries to parse a code span starting with the backtick fence at position i.
    /// Param c: Current traversal context.
    /// Param i: Position of the opening backtick fence.
    /// Param out: Output element list (receives the code run).
    /// Param buf: Plain-text buffer, flushed first.
    /// Returns Number of characters consumed, or 0 if not a code span.
    size_t
    tryCodeSpan(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;

    /// Finds the closing backtick fence of the same length as the opening one.
    /// Param s: Input string.
    /// Param from: Position where the search starts.
    /// Param n: Number of backticks in the opening fence.
    /// Returns Position of the closing fence, or std::string::npos.
    static size_t findCodeSpanClose(
        const std::string& s,
        size_t from,
        size_t n
    );

    /// Checks whether code-span content needs one leading/trailing space trimmed.
    /// Param content: Code-span content.
    /// Returns true if both edges are spaces and some non-space content remains.
    static bool needsSpaceTrim(const std::string& content);

    /// Tries to decode an HTML entity at position i.
    /// Param c: Current traversal context.
    /// Param i: Position of the '&'.
    /// Param buf: Buffer receiving the decoded text.
    /// Returns Number of characters consumed, or 0 if not an entity.
    size_t tryEntity(const Ctx& c, size_t i, std::string& buf) const;

    /// Flushes the accumulated buffer (with the ctx style) and appends parsed children.
    /// Param c: Current traversal context.
    /// Param out: Output element list.
    /// Param buf: Plain-text buffer, flushed first.
    /// Param kids: Parsed child elements to append.
    template <typename Elem>
    void emitAll(
        const Ctx& c,
        std::vector<InlineElement>& out,
        std::string& buf,
        std::vector<Elem> kids
    ) const {
        emitRun(
            c,
            out,
            buf
        );
        for (auto& k : kids) {
            out.push_back(std::move(k));
        }
    }

    // --- impl/inline_parser_links.cpp: links, link images, bare URLs ---
    /// Looks up a link definition by raw label text.
    /// Param rawLabel: Label as written in the source.
    /// \param[out] out Found definition.
    /// Returns true if a definition exists for the normalized label.
    bool lookupDef(const std::string& rawLabel, LinkDef& out) const;

    /// Resolves a link target after ']': (destination), [ref], [ref][] or collapsed [ref].
    /// Param s: Full input string.
    /// Param close: Position of the closing ']'.
    /// Param inner: Label text used for collapsed references.
    /// \param[out] url Resolved URL.
    /// \param[out] title Resolved title.
    /// \param[out] after Position just after the consumed target.
    /// Returns true if the target resolves; false means "not a link".
    bool resolveTarget(
        const std::string& s,
        size_t close,
        const std::string& inner,
        std::string& url,
        std::string& title,
        size_t& after
    ) const;

    /// Tries to parse an image "![alt](url)" at position i.
    /// Param c: Current traversal context.
    /// Param i: Position of the '!'.
    /// Param out: Output element list (receives the image).
    /// Param buf: Plain-text buffer, flushed first.
    /// Returns Number of characters consumed, or 0 if not an image.
    size_t
    tryImage(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;

    /// Tries to parse a link "[text](url)" (or reference forms) at position i.
    /// Param c: Current traversal context.
    /// Param i: Position of the '['.
    /// Param out: Output element list (receives the link runs).
    /// Param buf: Plain-text buffer, flushed first.
    /// Returns Number of characters consumed, or 0 if not a link.
    size_t tryLink(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;

    /// Tries to parse a bare http(s):// URL at position i.
    /// Param c: Current traversal context.
    /// Param i: Position of the 'h'.
    /// Param out: Output element list (receives the URL run).
    /// Param buf: Plain-text buffer, flushed first.
    /// Returns Number of characters consumed, or 0 if not a bare URL.
    size_t
    tryBareUrl(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;

    /// Strips trailing punctuation and unbalanced ')' from a bare URL.
    /// Param s: Full input string.
    /// Param from: Start of the URL.
    /// Param to: End of the URL (exclusive).
    /// Returns The trimmed URL and its new end position.
    static std::pair<std::string, size_t>
    trimUrlPunctuation(
        const std::string& s,
        size_t from,
        size_t to
    );

    // --- impl/inline_parser_markup.cpp: emphasis, '<' (comments/autolinks/tags) ---
    /// Tries to parse emphasis (bold/italic/strikethrough) at position i.
    /// Param c: Current traversal context.
    /// Param i: Position of the delimiter.
    /// Param out: Output element list (receives the styled runs).
    /// Param buf: Plain-text buffer, flushed first.
    /// Returns Number of characters consumed, or 0 if not emphasis.
    size_t
    tryEmphasis(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;

    /// Checks that an emphasis delimiter can open emphasis at position i.
    /// Param s: Input string.
    /// Param i: Position of the delimiter.
    /// Param tokLen: Delimiter length.
    /// Param wordRule: Intra-word rule (applies to '_' and '__').
    /// Returns true if the delimiter may open emphasis.
    static bool canOpenEmphasis(
        const std::string& s,
        size_t i,
        size_t tokLen,
        bool wordRule
    );

    /// Finds a closing emphasis delimiter.
    /// Param s: Input string.
    /// Param from: Position where the search starts.
    /// Param tok: Delimiter token.
    /// Param wordRule: Intra-word rule (applies to '_' and '__').
    /// Returns Position of the closing delimiter, or std::string::npos.
    static size_t
    findEmphasisClose(
        const std::string& s,
        size_t from,
        std::string_view tok,
        bool wordRule
    );

    /// Handles '<': comment, autolink or HTML tag.
    /// Param c: Current traversal context.
    /// Param i: Position of the '<'.
    /// Param out: Output element list.
    /// Param buf: Plain-text buffer.
    /// Returns Number of characters consumed, or 0 if nothing matched.
    size_t
    tryAngle(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;

    /// Tries to skip an HTML comment "<!-- ... -->" at position i.
    /// Param s: Input string.
    /// Param i: Position of the '<'.
    /// Returns Number of characters consumed (to the end of input if unclosed), or 0.
    static size_t tryComment(
        const std::string& s,
        size_t i
    );

    /// Tries to parse an autolink "<scheme://...>" / "<mailto:...>" at position i.
    /// Param c: Current traversal context.
    /// Param i: Position of the '<'.
    /// Param out: Output element list (receives the link run).
    /// Param buf: Plain-text buffer, flushed first.
    /// Returns Number of characters consumed, or 0 if not an autolink.
    size_t
    tryAutolink(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;

    /// Builds the target URL for an autolink body.
    /// Param inner: Text between '<' and '>'.
    /// Returns The URL, "mailto:..." for bare e-mails, or "" if not autolinkable.
    static std::string autolinkUrl(const std::string& inner);

    /// Tries to handle an inline HTML tag at position i (br, img, styled tags).
    /// Param c: Current traversal context.
    /// Param i: Position of the '<'.
    /// Param out: Output element list.
    /// Param buf: Plain-text buffer.
    /// Returns Number of characters consumed, or 0 if not a tag.
    size_t
    tryHtmlTag(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;

    /// Handles an <img> tag: emits an image element from src/alt/title.
    /// Param c: Current traversal context.
    /// Param i: Position of the '<'.
    /// Param tag: Parsed opening tag.
    /// Param out: Output element list (receives the image).
    /// Param buf: Plain-text buffer, flushed first.
    /// Returns Number of characters consumed (always the whole tag).
    size_t tryImgTag(
        const Ctx& c,
        size_t i,
        const HtmlTag& tag,
        std::vector<InlineElement>& out,
        std::string& buf
    ) const;

    /// Handles a paired style tag (b/i/u/s/code/a): parses and emits its content.
    /// Param c: Current traversal context.
    /// Param i: Position of the '<'.
    /// Param tag: Parsed opening tag.
    /// Param kind: Style kind from styleTagKind().
    /// Param out: Output element list (receives the styled runs).
    /// Param buf: Plain-text buffer, flushed first.
    /// Returns Number of characters consumed, or 0 if the tag is unclosed.
    size_t tryStyledTag(
        const Ctx& c,
        size_t i,
        const HtmlTag& tag,
        int kind,
        std::vector<InlineElement>& out,
        std::string& buf
    ) const;
};

} // namespace md::detail
