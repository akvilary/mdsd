// text_layout tests: u16 format ranges, merging, image alt text, flatten.

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "harness/md_dump.hpp"
#include "sabydoc/model/text_layout/text_layout.hpp"

namespace {

md::InlineElement run(std::string t, md::StyleFlags st = {}, std::string link = "") {
    return md::InlineElement{md::InlineElement::Kind::Run,
                             md::InlineRun{std::move(t), st, std::move(link)}, md::InlineImage{}};
}

// Vector from prototype verification: "Текст " + bold link "🚀 ссылка" + italic " хвост"
TEST(TextLayout, U16OffsetsWithAstralAndLink) {
    md::StyleFlags bold, ital;
    bold.bold = true;
    ital.italic = true;
    std::vector<md::InlineElement> elems{
        run("Текст "), run("\xF0\x9F\x9A\x80 ссылка", bold, "https://e.com"), run(" хвост", ital)};
    const sd::Text t = sd::buildText(elems);
    EXPECT_EQ(t.content, "Текст 🚀 ссылка хвост");
    ASSERT_EQ(t.formats.size(), 3u);
    EXPECT_EQ(t.formats[0].offset, 0u);
    EXPECT_EQ(t.formats[0].length, 6u);
    EXPECT_FALSE(t.formats[0].bold);
    EXPECT_EQ(t.formats[1].offset, 6u);
    EXPECT_EQ(t.formats[1].length, 9u); // 🚀 = 2 u16 + ' ' + "ссылка"(6)
    EXPECT_TRUE(t.formats[1].bold);
    EXPECT_EQ(t.formats[1].href, "https://e.com");
    EXPECT_EQ(t.formats[2].offset, 15u);
    EXPECT_EQ(t.formats[2].length, 6u);
    EXPECT_TRUE(t.formats[2].italic);
}

TEST(TextLayout, MergesAdjacentSameStyle) {
    md::StyleFlags bold;
    bold.bold = true;
    const sd::Text t = sd::buildText({run("ab", bold), run("cd", bold)});
    ASSERT_EQ(t.formats.size(), 1u);
    EXPECT_EQ(t.formats[0].length, 4u);
}

TEST(TextLayout, EmptyInput) {
    const sd::Text t = sd::buildText({});
    EXPECT_TRUE(t.content.empty());
}

TEST(TextLayout, ImageBecomesAltText) {
    md::InlineElement img;
    img.kind = md::InlineElement::Kind::Image;
    img.image.url = "x.png";
    img.image.alt = "подпись";
    const sd::Text t = sd::buildText({img});
    EXPECT_EQ(t.content, "подпись");
}

TEST(TextLayout, FlattensLineBreaks) {
    const sd::Text t = sd::buildText({run("a\nb")});
    EXPECT_EQ(t.content, "a b");
}

} // namespace
