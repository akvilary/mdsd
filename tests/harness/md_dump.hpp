#pragma once

// Помощники markdown-тестов: разбор и дамп инлайн-элементов в сравнимую строку.

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "markdown/parser/parser.hpp"

namespace test {

inline md::Document parseMd(const std::string& s) { return md::parseMarkdown(s); }

// Дамп runs: "[текст|BISUC|ссылка]"; картинки — "(img:url)".
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

// Инлайн-проверка: одиночный абзац из src должен дать ровно want.
inline void checkInline(const char* src, const std::string& want) {
    md::Document d = parseMd(src);
    ASSERT_EQ(d.blocks.size(), 1u) << "источник: " << src;
    ASSERT_EQ(d.blocks[0]->type, md::Block::Type::Paragraph) << "источник: " << src;
    EXPECT_EQ(runsDump(d.blocks[0]->inlines), want) << "источник: " << src;
}

} // namespace test
