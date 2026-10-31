#pragma once

// Инлайновый парсер markdown: акцент, код-спаны, ссылки, картинки, сущности,
// HTML-теги. Реализация распределена по impl/inline_parser_*.cpp.

#include <map>
#include <string>
#include <vector>

#include "algo/algo.hpp"
#include "markdown/ast/ast.hpp"
#include "markdown/parser/chars/entity/entity.hpp"
#include "markdown/parser/chars/html_tag/html_tag.hpp"
#include "markdown/parser/chars/link_def/link_def.hpp"

namespace md::detail {

class InlineParser {
  public:
    explicit InlineParser(const std::map<std::string, LinkDef>& defs,
                          std::vector<std::string>& warnings)
        : mDefs(defs), mWarnings(warnings) {}

    std::vector<InlineElement> parse(const std::string& s);

    // Контекст одного обхода parseRec: разбираемая строка, накопленные к этому
    // моменту стиль и ссылка (при вложенности), глубина рекурсии. Создаётся в
    // parse/рекурсивных вызовах и передаётся во все try*-обработчики.
    struct Ctx {
        const std::string& s;
        StyleFlags st;
        std::string link;
        int depth;
    };

  private:
    const std::map<std::string, LinkDef>& mDefs;
    std::vector<std::string>& mWarnings;

    // --- impl/inline_parser_core.cpp: каркас обхода, экранирование, код-спаны ---
    void warn(const std::string& msg) const;
    void emitRun(const Ctx& c, std::vector<InlineElement>& out, std::string& buf) const;
    std::vector<InlineElement> parseRec(const Ctx& c) const;
    static std::vector<InlineElement> mergeRuns(std::vector<InlineElement> elems);
    static size_t tryEscape(const Ctx& c, size_t i, std::string& buf);
    size_t tryCodeSpan(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                       std::string& buf) const;
    static size_t findCodeSpanClose(const std::string& s, size_t from, size_t n);
    static bool needsSpaceTrim(const std::string& content);
    size_t tryEntity(const Ctx& c, size_t i, std::string& buf) const;

    // Сбросить накопленный буфер (в стиле ctx) и добавить разобранных детей
    template <typename Elem>
    void emitAll(const Ctx& c, std::vector<InlineElement>& out, std::string& buf,
                 std::vector<Elem> kids) const {
        emitRun(c, out, buf);
        for (auto& k : kids) {
            out.push_back(std::move(k));
        }
    }

    // --- impl/inline_parser_links.cpp: ссылки, картинки по ссылкам, голые URL ---
    bool lookupDef(const std::string& rawLabel, LinkDef& out) const;
    bool resolveTarget(const std::string& s, size_t close, const std::string& inner,
                       std::string& url, std::string& title, size_t& after) const;
    size_t tryImage(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                    std::string& buf) const;
    size_t tryLink(const Ctx& c, size_t i, std::vector<InlineElement>& out, std::string& buf) const;
    size_t tryBareUrl(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                      std::string& buf) const;
    static std::pair<std::string, size_t> trimUrlPunctuation(const std::string& s, size_t from,
                                                             size_t to);

    // --- impl/inline_parser_markup.cpp: акцент, '<' (комментарии/автоссылки/теги) ---
    size_t tryEmphasis(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                       std::string& buf) const;
    static bool canOpenEmphasis(const std::string& s, size_t i, size_t tokLen, bool wordRule);
    static size_t findEmphasisClose(const std::string& s, size_t from, std::string_view tok,
                                    bool wordRule);
    size_t tryAngle(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                    std::string& buf) const;
    static size_t tryComment(const std::string& s, size_t i);
    size_t tryAutolink(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                       std::string& buf) const;
    static std::string autolinkUrl(const std::string& inner);
    size_t tryHtmlTag(const Ctx& c, size_t i, std::vector<InlineElement>& out,
                      std::string& buf) const;
    size_t tryImgTag(const Ctx& c, size_t i, const HtmlTag& tag, std::vector<InlineElement>& out,
                     std::string& buf) const;
    size_t tryStyledTag(const Ctx& c, size_t i, const HtmlTag& tag, int kind,
                        std::vector<InlineElement>& out, std::string& buf) const;
};

} // namespace md::detail
