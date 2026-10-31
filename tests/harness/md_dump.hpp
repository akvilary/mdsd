#pragma once

// Markdown test helpers: parsing and dumping inline elements into a comparable string.

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "markdown/parser/parser.hpp"

namespace test {

/// Parse markdown source text into a document.
inline md::Document parseMd(const std::string& s) { return md::parseMarkdown(s); }

/// Dump runs as "[text|BISUC|link]"; images are dumped as "(img:url)".
inline std::string runsDump(const std::vector<md::InlineElement>& elems) {
    std::string out;
    for (const auto& e : elems) {
        if (e.kind == md::InlineElement::Kind::Run) {
            out += "[" + e.run.text + "|";
            if (e.run.style.bold) {
                out += "B";
            }
            if (e.run.style.italic) {
                out += "I";
            }
            if (e.run.style.strike) {
                out += "S";
            }
            if (e.run.style.underline) {
                out += "U";
            }
            if (e.run.style.code) {
                out += "C";
            }
            out += "|";
            out += e.run.link;
            out += "]";
        } else {
            out += "(img:" + e.image.url + ")";
        }
    }
    return out;
}

/// Inline check: a single paragraph parsed from src must produce exactly want.
inline void checkInline(const char* src, const std::string& want) {
    md::Document d = parseMd(src);
    ASSERT_EQ(d.blocks.size(), 1u) << "source: " << src;
    ASSERT_EQ(d.blocks[0]->type, md::Block::Type::Paragraph) << "source: " << src;
    EXPECT_EQ(runsDump(d.blocks[0]->inlines), want) << "source: " << src;
}

} // namespace test
