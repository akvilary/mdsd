// Tests of Cyrillic (UTF-8) handling across all parser subsystems.
// Invariant: ASCII delimiters (0x00-0x7F) never occur inside Cyrillic UTF-8
// sequences (leading byte 0xD0-0xD1 + continuation bytes 0x80-0xBF), so byte
// operations at ASCII boundaries cannot split Cyrillic apart. These tests verify it in practice.

#include <gtest/gtest.h>

#include <string>

#include "harness/md_dump.hpp"
#include "sabydoc/model/text_layout/text_layout.hpp"

namespace {

using test::checkInline;
using test::parseMd;
using test::runsDump;

// --- Inline: Cyrillic around emphasis delimiters ---

TEST(Cyrillic, EmphasisAroundCyrillic) {
    checkInline("**жирный**", "[жирный|B|]");
    checkInline("*курсив*", "[курсив|I|]");
    checkInline("***всё сразу***", "[всё сразу|BI|]");
    checkInline("~~зачёркнутый~~", "[зачёркнутый|S|]");
    checkInline("обычный текст без акцента", "[обычный текст без акцента||]");
}

// Cyrillic before _ is a word character, `_` is not an opener (CommonMark rule)
TEST(Cyrillic, UnderscoreAfterCyrillicIsWord) {
    checkInline("начало_конец", "[начало_конец||]");
    checkInline("внутри_слова_тоже", "[внутри_слова_тоже||]");
    // but with spaces it works
    checkInline("_курсив_", "[курсив|I|]");
}

// Mixed: Cyrillic + ASCII + emoji
TEST(Cyrillic, MixedScripts) {
    checkInline("**Русский English 日本語**", "[Русский English 日本語|B|]");
    checkInline("текст с **жирным** и `кодом` и [ссылкой](https://ru.ру)",
                "[текст с ||][жирным|B|][ и ||][кодом|C|][ и ||][ссылкой||https://ru.ру]");
}

// --- Links with Cyrillic text and URLs ---

TEST(Cyrillic, LinksAndImages) {
    checkInline("[Ссылка на русский сайт](https://ru.wikipedia.org)",
                "[Ссылка на русский сайт||https://ru.wikipedia.org]");
    // the image alt text lives in image.alt, not in a separate Run
    const md::Document d = parseMd("![Кириллическая подпись](картинка.png)");
    ASSERT_EQ(d.blocks.size(), 1u);
    ASSERT_EQ(d.blocks[0]->inlines.size(), 1u);
    ASSERT_EQ(d.blocks[0]->inlines[0].kind, md::InlineElement::Kind::Image);
    EXPECT_EQ(d.blocks[0]->inlines[0].image.url, "картинка.png");
    EXPECT_EQ(d.blocks[0]->inlines[0].image.alt, "Кириллическая подпись");
}

// --- Headers with Cyrillic ---

TEST(Cyrillic, HeadersAndSetext) {
    const md::Document d = parseMd("# Русский заголовок\n\nЗаголовок сетекстом\n===");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[0]->level, 1);
    EXPECT_EQ(test::runsDump(d.blocks[0]->inlines), "[Русский заголовок||]");
    EXPECT_EQ(d.blocks[1]->level, 1); // setext from "=" after a Cyrillic paragraph
}

// --- Tables with Cyrillic and escaped pipes ---

TEST(Cyrillic, TableWithCyrillicAndEscapedPipes) {
    const md::Document d = parseMd(
        "| Русская колонка | English |\n"
        "|-----------------|---------|\n"
        "| Значение с \\| пайпом | Value |");
    ASSERT_EQ(d.blocks.size(), 1u);
    ASSERT_EQ(d.blocks[0]->type, md::Block::Type::Table);
    EXPECT_EQ(md::plainText(d.blocks[0]->rows[1][0]), "Значение с | пайпом");
}

// --- Quotes with nesting and Cyrillic ---

TEST(Cyrillic, QuotesWithCyrillic) {
    // after a blank line, "хвост" is a separate paragraph (correct per CommonMark)
    const md::Document d = parseMd("> Кириллическая цитата\n> > Вложенная\n\nхвост");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::Quote);
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::Paragraph);
}

// --- text_layout: UTF-16 offsets for mixed Cyrillic + emoji ---

TEST(Cyrillic, TextLayoutU16OffsetsMixedScripts) {
    // "Привет" (6 Cyrillic chars = 6 u16) + " " (1) + "🚀" (1 astral = 2 u16)
    // + " " (1) + "мир" (3 Cyrillic chars = 3 u16) = 13 u16
    const std::string text = "Привет \xF0\x9F\x9A\x80 мир";
    const size_t expected = 6 + 1 + 2 + 1 + 3; // = 13
    auto makeElem = [](const std::string& s) {
        return md::InlineElement{md::InlineElement::Kind::Run,
                                 md::InlineRun{s, md::StyleFlags{}, ""}, md::InlineImage{}};
    };
    const auto result = sd::buildText({makeElem(text)});
    EXPECT_EQ(result.content, text);
    ASSERT_EQ(result.formats.size(), 1u);
    EXPECT_EQ(result.formats[0].length, expected);
}

// --- text_layout: bold Cyrillic text — correct offset/length ---

TEST(Cyrillic, TextLayoutBoldCyrillicOffsets) {
    md::StyleFlags bold;
    bold.bold = true;
    // "До" (2 u16) + "Жирный" (6 u16) + "После" (5 u16) = 13 u16
    auto run = [](std::string t, md::StyleFlags st = {}) {
        return md::InlineElement{md::InlineElement::Kind::Run,
                                 md::InlineRun{std::move(t), st, ""}, md::InlineImage{}};
    };
    auto t = sd::buildText({run("До "), run("Жирный", bold), run("После")});
    ASSERT_EQ(t.formats.size(), 3u);
    EXPECT_EQ(t.formats[0].offset, 0u);  // "До " = 3 u16
    EXPECT_EQ(t.formats[0].length, 3u);
    EXPECT_EQ(t.formats[1].offset, 3u);  // "Жирный" = 6 u16
    EXPECT_EQ(t.formats[1].length, 6u);
    EXPECT_FALSE(t.formats[1].bold == false); // bold
    EXPECT_EQ(t.formats[2].offset, 9u);  // "После" = 5 u16
    EXPECT_EQ(t.formats[2].length, 5u);
    EXPECT_TRUE(t.formats[2].length == 5u);
}

// --- Front matter and task lists with Cyrillic ---

TEST(Cyrillic, FrontMatterAndTaskLists) {
    const md::Document d = parseMd("---\ntitle: Русский заголовок\n---\n\n- [x] Выполнено\n- [ ] Не выполнено");
    EXPECT_EQ(d.title, "Русский заголовок");
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(d.blocks[0]->listKind, md::ListKind::Task);
    EXPECT_EQ(md::plainText(d.blocks[0]->items[0].content), "Выполнено");
    EXPECT_EQ(md::plainText(d.blocks[0]->items[1].content), "Не выполнено");
}

// --- Code blocks with Cyrillic inside ---

TEST(Cyrillic, CodeBlockWithCyrillic) {
    const md::Document d = parseMd("```\n// Кириллический комментарий\nlet x = 1;\n```");
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(d.blocks[0]->codeText, "// Кириллический комментарий\nlet x = 1;");
}

// --- Escape gotchas: Cyrillic before a backslash ---

TEST(Cyrillic, EscapesWithCyrillic) {
    checkInline("кириллица\\*не акцент\\*", "[кириллица*не акцент*||]");
    checkInline("текст\\_с экранированием\\_", "[текст_с экранированием_||]");
}

// --- Lists: nesting with Cyrillic ---

TEST(Cyrillic, NestedListCyrillic) {
    const md::Document d = parseMd("- Первый\n  - Второй уровень\n    - Третий\n- Четвёртый");
    ASSERT_EQ(d.blocks.size(), 1u);
    ASSERT_EQ(d.blocks[0]->items.size(), 2u);
    EXPECT_EQ(md::plainText(d.blocks[0]->items[0].content), "Первый");
    ASSERT_EQ(d.blocks[0]->items[0].children.size(), 1u);
    EXPECT_EQ(md::plainText(d.blocks[0]->items[0].children[0]->items[0].content), "Второй уровень");
}

// --- Horizontal rule after a Cyrillic paragraph (not setext) ---

TEST(Cyrillic, HrAfterCyrillicParagraph) {
    const md::Document d = parseMd("Кириллический абзац\n\n---");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::Rule);
}

// --- HTML tags with Cyrillic inside ---

TEST(Cyrillic, HtmlTagsWithCyrillicContent) {
    checkInline("<b>Жирный кириллицей</b>", "[Жирный кириллицей|B|]");
    checkInline("<u>Подчёркнутый текст</u>", "[Подчёркнутый текст|U|]");
    checkInline("<i>Курсив</i> и обычный текст", "[Курсив|I|][ и обычный текст||]");
}

} // namespace test
