#pragma once

#include <memory>
#include <string>
#include <vector>

// AST markdown. Единственная модель данных для ОБОИХ направлений конвертации:
// md -> sabydoc (парсер текст -> AST) и в будущем sabydoc -> md (AST -> текст).
namespace md {

enum class Align { None, Left, Center, Right, Justify };

struct StyleFlags {
    bool bold = false;
    bool italic = false;
    bool strike = false;
    bool underline = false;
    bool code = false; // код-спан; в sabydoc не отображается — только текст
    bool operator==(const StyleFlags&) const = default;
};

struct InlineRun {
    std::string text; // UTF-8; '\n' внутри = жёсткий перенос
    StyleFlags style;
    std::string link; // "" — не ссылка
};

struct InlineImage {
    std::string url;
    std::string alt;
    std::string title;
};

struct InlineElement {
    enum class Kind { Run, Image } kind = Kind::Run;
    InlineRun run;
    InlineImage image;
};

struct Block;
using BlockPtr = std::unique_ptr<Block>;

struct ListItem {
    std::vector<InlineElement> content; // текст пункта (первый абзац)
    std::vector<BlockPtr> children;     // вложенные списки, абзацы, код и т.п.
    bool task = false;
    bool checked = false;
};

enum class ListKind { Bullet, Ordered, Task };

struct Block {
    enum class Type { Paragraph, Header, Code, Quote, List, Table, Rule };
    Type type = Type::Paragraph;

    // абзац / заголовок
    std::vector<InlineElement> inlines;
    Align align = Align::None;
    int level = 0; // заголовок 1..6

    // код
    std::string codeText;
    std::string lang;

    // цитата
    std::vector<BlockPtr> children;

    // список
    ListKind listKind = ListKind::Bullet;
    std::vector<ListItem> items;

    // таблица: rows[строка][столбец]; строка 0 — шапка
    std::vector<Align> colAlign;
    std::vector<std::vector<std::vector<InlineElement>>> rows;
};

struct Document {
    std::vector<BlockPtr> blocks;
    std::string title; // из front matter ("title: ..."), если был
    std::vector<std::string> warnings;
};

// Склейка в плоский текст (для TOC, заголовка документа, подписей).
std::string plainText(const std::vector<InlineElement>& elems);

// Разбиение по жёстким переносам '\n' (каждая группа = отдельный абзац).
std::vector<std::vector<InlineElement>> splitByLineBreaks(const std::vector<InlineElement>& elems);

// Замена '\n' на пробел (заголовки, ячейки таблиц, пункты списков).
std::vector<InlineElement> flattenLineBreaks(const std::vector<InlineElement>& elems);

} // namespace md
