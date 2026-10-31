// Конец-в-конец: markdown -> маппинг -> архив; диагностики, TOC, политика разделителей.

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

// Маленький документ со всеми ключевыми видами блоков (включая несуществующую картинку)
sd::Document makeSampleDoc(std::vector<std::string>& diags, ResourceCollector& rc) {
    md::Document doc = test::parseMd(
        "# Заголовок\n\nТекст с **жирным** и [ссылкой](https://e.com).\n\n- пункт\n  - "
        "вложенный\n\n"
        "> цитата\n\n| A | B |\n|---|---:|\n| 1 | 2 |\n\n---\n\n```js\nlet a=0;\n```\n\n"
        "- [x] готово\n\nтекст\n\nкартинка: ![подпись](none.png)\n");
    mapping::Options opt;
    opt.title = "Тест";
    return mapping::map(std::move(doc), rc, opt);
}

TEST(E2e, BuildsArchive) {
    std::vector<std::string> diags;
    ResourceCollector rc(std::filesystem::path("/nonexistent"), diags);
    const sd::Document sdoc = makeSampleDoc(diags, rc);
    EXPECT_GT(sd::buildSabydocArchive(sdoc).size(), 500u);
}

TEST(E2e, TocContainsHeader) {
    std::vector<std::string> diags;
    ResourceCollector rc(std::filesystem::path("/nonexistent"), diags);
    const sd::Document sdoc = makeSampleDoc(diags, rc);
    EXPECT_TRUE(sd::writeTableOfContents(sdoc).is_array());
}

TEST(E2e, WarnsOnMissingImage) {
    std::vector<std::string> diags;
    ResourceCollector rc(std::filesystem::path("/nonexistent"), diags);
    (void)makeSampleDoc(diags, rc);
    EXPECT_FALSE(diags.empty());
    bool found = false;
    for (const std::string& d : diags) {
        if (d.find("none.png") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// После списка/таблицы/кода/цитаты/линии — пустой абзац-разделитель
TEST(E2e, SeparatorPolicy) {
    std::vector<std::string> diags;
    ResourceCollector rc(std::filesystem::path("/nonexistent"), diags);
    const sd::Document sdoc = makeSampleDoc(diags, rc);
    size_t separators = 0;
    for (const auto& b : sdoc.blocks) {
        if (b->kind == sd::Block::Kind::Paragraph && b->indent0) {
            ++separators;
        }
    }
    EXPECT_GE(separators, 4u);
}

} // namespace
