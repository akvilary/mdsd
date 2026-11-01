// Block parser tests: headers, lists, quotes, tables, code, line breaks.
// Includes regressions from code review rounds (guards, GFM table over a paragraph, etc.).

#include <gtest/gtest.h>

#include <string>

#include "harness/md_dump.hpp"

namespace {

using test::parseMd;
using test::runsDump;

TEST(BlockParser, SetextHeaders) {
    {
        const md::Document d = parseMd("Заголовок\n===");
        ASSERT_EQ(d.blocks.size(), 1u);
        EXPECT_EQ(d.blocks[0]->type, md::Block::Type::Header);
        EXPECT_EQ(d.blocks[0]->level, 1);
    }
    {
        const md::Document d = parseMd("Тема\n---");
        ASSERT_EQ(d.blocks.size(), 1u);
        EXPECT_EQ(d.blocks[0]->type, md::Block::Type::Header);
        EXPECT_EQ(d.blocks[0]->level, 2);
    }
}

TEST(BlockParser, HrAfterBlankIsNotSetext) {
    const md::Document d = parseMd("текст\n\n---");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::Rule);
}

TEST(BlockParser, HeaderLevels1To6) {
    const md::Document d = parseMd("# h1\n## h2\n### h3\n#### h4\n##### h5\n###### h6");
    ASSERT_EQ(d.blocks.size(), 6u);
    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(d.blocks[static_cast<size_t>(i)]->level, i + 1);
    }
}

TEST(BlockParser, NestedList) {
    const md::Document d = parseMd("- item1\n  - sub1\n  - sub2\n- item2");
    ASSERT_EQ(d.blocks.size(), 1u);
    ASSERT_EQ(d.blocks[0]->type, md::Block::Type::List);
    const auto& items = d.blocks[0]->items;
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(runsDump(items[0].content), "[item1||]");
    ASSERT_EQ(items[0].children.size(), 1u);
    EXPECT_EQ(items[0].children[0]->type, md::Block::Type::List);
    EXPECT_EQ(items[0].children[0]->items.size(), 2u);
}

// Lazy continuation guard: text after a sublist closes the list
TEST(BlockParser, LazyContinuationGuard) {
    const md::Document d = parseMd("- item\n  - sub\ntext");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::List);
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::Paragraph);
}

// No lazy continuation: an under-indented line right after an item is a sibling
TEST(BlockParser, UnderIndentedLineIsSibling) {
    const md::Document d = parseMd("- item\nпродолжение");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::List);
    EXPECT_EQ(runsDump(d.blocks[0]->items[0].content), "[item||]");
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::Paragraph);
    EXPECT_EQ(runsDump(d.blocks[1]->inlines), "[продолжение||]");
}

// >4 spaces after the marker: content keeps the excess (code inside an item)
TEST(BlockParser, SpacesAfterMarker) {
    const md::Document d = parseMd("-     code");
    ASSERT_EQ(d.blocks.size(), 1u);
    ASSERT_EQ(d.blocks[0]->items.size(), 1u);
    ASSERT_EQ(d.blocks[0]->items[0].children.size(), 1u);
    EXPECT_EQ(d.blocks[0]->items[0].children[0]->type, md::Block::Type::Code);
    EXPECT_EQ(d.blocks[0]->items[0].children[0]->codeText, "code");
}

TEST(BlockParser, TaskList) {
    const md::Document d = parseMd("- [ ] первый\n- [x] второй");
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(d.blocks[0]->listKind, md::ListKind::Task);
    EXPECT_FALSE(d.blocks[0]->items[0].checked);
    EXPECT_TRUE(d.blocks[0]->items[1].checked);
    EXPECT_EQ(runsDump(d.blocks[0]->items[0].content), "[первый||]"); // task prefix stripped
}

// Mixed items split into lists by kind: runs of checkboxes and plain items
TEST(BlockParser, MixedTasksSplitIntoKindRuns) {
    const md::Document d = parseMd("- [ ] a\n- plain\n- [x] c");
    ASSERT_EQ(d.blocks.size(), 3u);
    EXPECT_EQ(d.blocks[0]->listKind, md::ListKind::Task);
    ASSERT_EQ(d.blocks[0]->items.size(), 1u);
    EXPECT_EQ(runsDump(d.blocks[0]->items[0].content), "[a||]");
    EXPECT_EQ(d.blocks[1]->listKind, md::ListKind::Bullet);
    ASSERT_EQ(d.blocks[1]->items.size(), 1u);
    EXPECT_FALSE(d.blocks[1]->items[0].task);
    EXPECT_EQ(d.blocks[2]->listKind, md::ListKind::Task);
    EXPECT_TRUE(d.blocks[2]->items[0].checked);
}

// A blank line always terminates the collection: two lists, one item each
TEST(BlockParser, BlankSplitsLists) {
    const md::Document d = parseMd("- a\n\n- b");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::List);
    ASSERT_EQ(d.blocks[0]->items.size(), 1u);
    EXPECT_EQ(runsDump(d.blocks[0]->items[0].content), "[a||]");
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::List);
    EXPECT_EQ(d.blocks[1]->items.size(), 1u);
    EXPECT_EQ(runsDump(d.blocks[1]->items[0].content), "[b||]");
}

// The same for ordered lists: a blank line starts a new collection
TEST(BlockParser, BlankSplitsOrderedLists) {
    const md::Document d = parseMd("1. первый\n\n2. второй");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[0]->listKind, md::ListKind::Ordered);
    EXPECT_EQ(d.blocks[0]->items.size(), 1u);
    EXPECT_EQ(d.blocks[1]->listKind, md::ListKind::Ordered);
    EXPECT_EQ(d.blocks[1]->items.size(), 1u);
}

// A blank line ends lazy continuation: the paragraph after a list is a sibling,
// not part of the last item (same for tables and other following blocks)
TEST(BlockParser, ParagraphAfterListIsSibling) {
    const md::Document d = parseMd("- a\n- b\n\nТекст абзаца.\nЕщё строка.");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::List);
    EXPECT_EQ(d.blocks[0]->items.size(), 2u);
    EXPECT_EQ(runsDump(d.blocks[0]->items[1].content), "[b||]");
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::Paragraph);
    EXPECT_EQ(runsDump(d.blocks[1]->inlines), "[Текст абзаца. Ещё строка.||]");
}

// ...and the table after a blank line is a sibling block too
TEST(BlockParser, TableAfterListIsSibling) {
    const md::Document d = parseMd("- a\n\n| x | y |\n|---|---|\n| 1 | 2 |\n\nАбзац.");
    ASSERT_EQ(d.blocks.size(), 3u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::List);
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::Table);
    EXPECT_EQ(d.blocks[2]->type, md::Block::Type::Paragraph);
}

// A blank line ends the item even when the following text is indented to it
TEST(BlockParser, IndentedTextAfterBlankIsSibling) {
    const md::Document d = parseMd("- a\n\n  продолжение\n\nАбзац.");
    ASSERT_EQ(d.blocks.size(), 3u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::List);
    ASSERT_EQ(d.blocks[0]->items.size(), 1u);
    EXPECT_EQ(d.blocks[1]->type, md::Block::Type::Paragraph);
    EXPECT_EQ(runsDump(d.blocks[1]->inlines), "[продолжение||]");
    EXPECT_EQ(d.blocks[2]->type, md::Block::Type::Paragraph);
}

// per CM, "хвост" lazily continues into the nested quote — a single top-level block
TEST(BlockParser, QuoteWithNestingAndLazyTail) {
    const md::Document d = parseMd("> первый\n\n> второй\n> > вложенный\nхвост");
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::Quote);
    ASSERT_EQ(d.blocks[0]->children.size(), 3u); // paragraph + paragraph + nested quote
    EXPECT_EQ(d.blocks[0]->children[2]->type, md::Block::Type::Quote);
    EXPECT_EQ(d.blocks[0]->children[0]->type, md::Block::Type::Paragraph);
}

TEST(BlockParser, FenceAndIndentedCode) {
    const md::Document d = parseMd("```js\nlet a = 0;\n```\n\n    indented\n    code");
    ASSERT_EQ(d.blocks.size(), 2u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::Code);
    EXPECT_EQ(d.blocks[0]->lang, "js");
    EXPECT_EQ(d.blocks[0]->codeText, "let a = 0;");
    EXPECT_EQ(d.blocks[1]->codeText, "indented\ncode");
}

TEST(BlockParser, TableWithAlignAndEscapedPipe) {
    const md::Document d = parseMd("| A | B | C |\n|---|:-:|--:|\n| a\\|b |  | x |");
    ASSERT_EQ(d.blocks.size(), 1u);
    ASSERT_EQ(d.blocks[0]->type, md::Block::Type::Table);
    const auto& t = *d.blocks[0];
    ASSERT_EQ(t.colAlign.size(), 3u);
    EXPECT_EQ(t.colAlign[1], md::Align::Center);
    EXPECT_EQ(t.colAlign[2], md::Align::Right);
    ASSERT_EQ(t.rows.size(), 2u);
    ASSERT_EQ(t.rows[1].size(), 3u);
    EXPECT_EQ(md::plainText(t.rows[1][0]), "a|b");
    EXPECT_EQ(md::plainText(t.rows[1][1]), "");
}

TEST(BlockParser, HtmlAlign) {
    {
        const md::Document d = parseMd("<div align=\"center\">\nпо центру\n</div>");
        ASSERT_EQ(d.blocks.size(), 1u);
        EXPECT_EQ(d.blocks[0]->align, md::Align::Center);
    }
    {
        const md::Document d = parseMd("<p align=\"right\">справа</p>");
        ASSERT_EQ(d.blocks.size(), 1u);
        EXPECT_EQ(d.blocks[0]->align, md::Align::Right);
    }
}

TEST(BlockParser, HardBreaks) {
    const md::Document d = parseMd("первая  \nвторая"); // two trailing spaces
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(md::plainText(d.blocks[0]->inlines), "первая\nвторая");
}

TEST(BlockParser, FrontMatter) {
    const md::Document d = parseMd("---\ntitle: Мой документ\n---\n\n# H");
    EXPECT_EQ(d.title, "Мой документ");
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(d.blocks[0]->type, md::Block::Type::Header);
}

// --- code review round regressions ---

// GFM: a delimiter row turns the paragraph's last line into a table header
TEST(BlockParser, GfmTableInterruptsParagraph) {
    const md::Document d = parseMd("Раздел | Глава\n- | -\n1 | 2");
    ASSERT_EQ(d.blocks.size(), 1u);
    ASSERT_EQ(d.blocks[0]->type, md::Block::Type::Table);
    ASSERT_EQ(d.blocks[0]->rows.size(), 2u); // header + 1 body row
    EXPECT_EQ(md::plainText(d.blocks[0]->rows[0][0]), "Раздел");
}

// an ordered list interrupts a paragraph only when starting at 1 (CommonMark)
TEST(BlockParser, OrderedTenDoesNotInterrupt) {
    const md::Document d = parseMd("текст\n10. пункт");
    ASSERT_EQ(d.blocks.size(), 1u);
    ASSERT_EQ(d.blocks[0]->type, md::Block::Type::Paragraph);
    EXPECT_EQ(md::plainText(d.blocks[0]->inlines), "текст 10. пункт");
}

// an even number of backslashes is an escaped pair, NOT a hard break
TEST(BlockParser, EvenBackslashesAreEscaped) {
    const md::Document d = parseMd("x\\\\\ny");
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(md::plainText(d.blocks[0]->inlines), "x\\ y");
}

TEST(BlockParser, OddBackslashIsHardBreak) {
    const md::Document d = parseMd("a\\\nb");
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(md::plainText(d.blocks[0]->inlines), "a\nb");
}

// data-align does not count as align (attribute name boundary check)
TEST(BlockParser, DataAlignIgnored) {
    const md::Document d = parseMd("<div data-align=\"center\">текст</div>");
    ASSERT_EQ(d.blocks.size(), 1u);
    EXPECT_EQ(d.blocks[0]->align, md::Align::None);
    EXPECT_EQ(md::plainText(d.blocks[0]->inlines), "текст");
}

TEST(BlockParser, ImageByReference) {
    const md::Document d = parseMd("![alt][r]\n\n[r]: img.png");
    ASSERT_EQ(d.blocks.size(), 1u);
    const auto& in = d.blocks[0]->inlines;
    ASSERT_EQ(in.size(), 1u);
    ASSERT_EQ(in[0].kind, md::InlineElement::Kind::Image);
    EXPECT_EQ(in[0].image.url, "img.png");
}

} // namespace
