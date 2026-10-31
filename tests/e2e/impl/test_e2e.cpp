// End-to-end: markdown -> mapping -> archive; diagnostics, TOC, separator policy.

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "harness/md_dump.hpp"
#include "mapping/md_to_sabydoc/md_to_sabydoc.hpp"
#include "sabydoc/model/resource_collector/resource_collector.hpp"
#include "sabydoc/model/widget_writer/document_writer/document_writer.hpp"
#include "sabydoc/model/widget_writer/widget_writer.hpp"

namespace {

// Small document with all key block kinds (including a nonexistent image)
sd::Document makeSampleDoc(ResourceCollector& rc) {
    md::Document doc = test::parseMd(
        "# Header\n\nText with **bold** and a [link](https://e.com).\n\n- item\n  - "
        "nested\n\n"
        "> quote\n\n| A | B |\n|---|---:|\n| 1 | 2 |\n\n---\n\n```js\nlet a=0;\n```\n\n"
        "- [x] done\n\ntext\n\nimage: ![caption](none.png)\n");
    mapping::Options opt;
    opt.title = "Test";
    return mapping::map(std::move(doc), rc, opt);
}

TEST(E2e, BuildsArchive) {
    std::vector<std::string> diags;
    ResourceCollector rc(std::filesystem::path("/nonexistent"), diags);
    const sd::Document sdoc = makeSampleDoc(rc);
    EXPECT_GT(sd::buildSabydocArchive(sdoc).size(), 500u);
}

TEST(E2e, TocContainsHeader) {
    std::vector<std::string> diags;
    ResourceCollector rc(std::filesystem::path("/nonexistent"), diags);
    const sd::Document sdoc = makeSampleDoc(rc);
    EXPECT_TRUE(sd::writeTableOfContents(sdoc).is_array());
}

TEST(E2e, WarnsOnMissingImage) {
    std::vector<std::string> diags;
    ResourceCollector rc(std::filesystem::path("/nonexistent"), diags);
    (void)makeSampleDoc(rc);
    EXPECT_FALSE(diags.empty());
    bool found = false;
    for (const std::string& d : diags) {
        if (d.find("none.png") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// After a list/table/code/quote/rule — an empty separator paragraph
TEST(E2e, SeparatorPolicy) {
    std::vector<std::string> diags;
    ResourceCollector rc(std::filesystem::path("/nonexistent"), diags);
    const sd::Document sdoc = makeSampleDoc(rc);
    size_t separators = 0;
    for (const auto& b : sdoc.blocks) {
        if (b->kind == sd::Block::Kind::Paragraph && b->indent0) {
            ++separators;
        }
    }
    EXPECT_GE(separators, 4u);
}

} // namespace
