// Inline parser tests: emphasis, code spans, links, entities, images.
// Test vectors come from prototype verification of the algorithms (review rounds).

#include <gtest/gtest.h>

#include <string>

#include "harness/md_dump.hpp"

namespace {

using test::checkInline;
using test::parseMd;

TEST(Inline, Emphasis) {
    checkInline("**bold *ital* end**", "[bold |B|][ital|BI|][ end|B|]");
    checkInline("***bi***", "[bi|BI|]");
    checkInline("a*b*c", "[a||][b|I|][c||]");
    checkInline("_ital_", "[ital|I|]");
    checkInline("~~**x**~~", "[x|BS|]");
    // unclosed and non-emphasis markers
    checkInline("**x", "[**x||]");
    checkInline("a * b * c", "[a * b * c||]");
    checkInline("snake_case_x", "[snake_case_x||]");
}

TEST(Inline, Strike) {
    checkInline("~~x~~", "[x|S|]");
    checkInline("~x~", "[~x~||]"); // a single tilde is not strikethrough
}

TEST(Inline, CodeSpans) {
    checkInline("a `code` b", "[a ||][code|C|][ b||]");
    checkInline("``a`b``", "[a`b|C|]");
    checkInline("` spaced `", "[spaced|C|]"); // single space trimmed per CM
}

TEST(Inline, HtmlTags) {
    checkInline("<u>подчёркнутый</u>", "[подчёркнутый|U|]");
    checkInline("<b>ж</b> и <i>к</i>", "[ж|B|][ и ||][к|I|]");
    checkInline("текст <!-- комментарий --> хвост", "[текст  хвост||]");
}

TEST(Inline, Links) {
    checkInline("[ссылка](https://e.com)", "[ссылка||https://e.com]");
    checkInline("[к][r]\n\n[r]: https://e.com \"T\"",
                "[к||https://e.com]"); // definition after use
    checkInline("<https://e.com>", "[https://e.com||https://e.com]");
    checkInline("го https://e.com/x, дальше",
                "[го ||][https://e.com/x||https://e.com/x][, дальше||]"); // trailing punctuation
    // spaces are allowed inside angle brackets (CommonMark)
    checkInline("[br][]\n\n[br]: <https://e.com/a b> \"T\"", "[br||https://e.com/a b]");
}

TEST(Inline, EscapesAndEntities) {
    checkInline("a\\*b\\*c", "[a*b*c||]");
    checkInline("&amp;&copy;&#9731;", "[&\xC2\xA9\xE2\x98\x83||]");
}

TEST(Inline, Images) {
    const md::Document d = parseMd("![подпись](img.png)");
    ASSERT_EQ(d.blocks.size(), 1u);
    const auto& in = d.blocks[0]->inlines;
    ASSERT_EQ(in.size(), 1u);
    ASSERT_EQ(in[0].kind, md::InlineElement::Kind::Image);
    EXPECT_EQ(in[0].image.url, "img.png");
    EXPECT_EQ(in[0].image.alt, "подпись");
}

} // namespace
