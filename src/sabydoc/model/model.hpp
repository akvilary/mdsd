#pragma once

#include <memory>
#include <string>
#include <vector>

#include "markdown/ast/ast.hpp"

// Каноническая модель SabyDoc. Единственный домен для ОБОИХ направлений:
// md -> sabydoc (маппинг строит модель, widget_writer сериализует в JSONML)
// и в будущем sabydoc -> md (widget_reader построит модель из JSONML).
// Никаких JSONML-деталей здесь быть не должно.
namespace sd {

enum class ListMarker { Bullet, Numbered, Checkbox };

// Диапазон форматирования: [offset, length] в КОДОВЫХ ЕДИНИЦАХ UTF-16
// (как String.length в JS-редакторе). href != "" → ссылка.
struct FormatRange {
    size_t offset = 0;
    size_t length = 0;
    bool bold = false;
    bool italic = false;
    bool strike = false;
    bool underline = false;
    std::string href;
};

struct Text {
    std::string content; // UTF-8; пустая строка → атрибут children:""
    std::vector<FormatRange> formats;
    bool emptyColor1 = false; // имитация разделительного пустого абзаца редактора
};

struct Block;
using BlockPtr = std::unique_ptr<Block>;

struct ListItem {
    std::string id;
    bool checked = false;
    Text content;
    std::vector<BlockPtr> children; // вложенный список, доп. абзацы пункта
};

struct TableCell {
    std::vector<Text> paragraphs; // жёсткие переносы внутри ячейки → несколько абзацев
};

struct Block {
    enum class Kind {
        DocumentHeader,
        Header,
        Paragraph,
        Paragraph2,
        Code,
        Quote,
        List,
        Table,
        Image,
        Separator
    };
    Kind kind = Kind::Paragraph;

    std::string id;
    int level = 0;        // заголовок 1..5
    std::string align;    // "" | "center" | "right" | "justify"
    bool indent0 = false; // атрибут indent:0 у разделительного пустого абзаца

    Text text;        // DocumentHeader / Header / Paragraph / Paragraph2
    std::string code; // Code

    std::vector<BlockPtr> children; // Quote

    ListMarker marker = ListMarker::Bullet; // List
    std::vector<ListItem> items;            // List

    std::vector<std::string> colAlign; // Table: "" | "center" | "right" | "justify"
    std::vector<std::vector<TableCell>> rows;

    std::string resourceKey; // Image
    unsigned imageW = 0, imageH = 0;
};

struct Resource {
    std::string key;      // r1, r2, ...
    std::string fileName; // исходное имя файла (для атрибута fileName)
    std::string fileId;   // uuid
    std::string zipName;  // имя внутри архива (санитизированное)
    unsigned width = 0, height = 0;
    std::string bytes; // содержимое файла
};

struct Document {
    std::string title; // documentHeader
    std::string pageName = "Базовый";
    std::vector<BlockPtr> blocks;    // дети корневого textLayout
    std::vector<Resource> resources; // в порядке появления
    std::string pageId, layoutId, rootTextLayoutId, headerId;
};

} // namespace sd
