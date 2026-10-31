// Маппинг контентных блоков: фабрики, картинки, абзацы, заголовки, цитаты.

#include "mapping/md_to_sabydoc/md_to_sabydoc.hpp"

#include <utility>

#include "uuid4/uuid4.hpp"
#include "sabydoc/model/text_layout/text_layout.hpp"

#include "mapping/md_to_sabydoc/mappers.hpp"

namespace mapping {
namespace detail {

std::string alignStr(md::Align a) {
    switch (a) {
    case md::Align::Center:
        return "center";
    case md::Align::Right:
        return "right";
    case md::Align::Justify:
        return "justify";
    case md::Align::Left:
    case md::Align::None:
    default:
        return "";
    }
}

// Спец-блоки, после которых редактор ставит пустой абзац-разделитель
bool needsTrailingSeparator(sd::Block::Kind k) {
    switch (k) {
    case sd::Block::Kind::List:
    case sd::Block::Kind::Table:
    case sd::Block::Kind::Code:
    case sd::Block::Kind::Quote:
    case sd::Block::Kind::Separator:
        return true;
    default:
        return false;
    }
}

std::unique_ptr<sd::Block> newBlock(sd::Block::Kind kind) {
    auto b = std::make_unique<sd::Block>();
    b->kind = kind;
    b->id = makeUuid4();
    return b;
}

std::unique_ptr<sd::Block> makeSeparatorMimic() {
    auto b = newBlock(sd::Block::Kind::Paragraph);
    b->indent0 = true;
    b->text.emptyColor1 = true;
    return b;
}

std::unique_ptr<sd::Block> makePlaceholder(const std::string& url) {
    auto b = newBlock(sd::Block::Kind::Paragraph);
    b->text = sd::plainText("[Изображение: " + url + "]");
    return b;
}

std::unique_ptr<sd::Block> makeCaption(const std::string& text) {
    auto b = newBlock(sd::Block::Kind::Paragraph2);
    b->text = sd::plainText(text);
    return b;
}

// ---------------------------------------------------------------- картинки

void emitImage(const md::InlineImage& img, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc) {
    unsigned w = 0, h = 0;
    const std::string key = rc.addImage(img.url, w, h);
    if (key.empty()) { // недоступна (предупреждение уже добавлено) — заметная заглушка
        dst.push_back(makePlaceholder(img.url));
        return;
    }
    auto im = newBlock(sd::Block::Kind::Image);
    im->resourceKey = key;
    im->imageW = w;
    im->imageH = h;
    dst.push_back(std::move(im));
    const std::string caption = !img.alt.empty() ? img.alt : img.title;
    if (!caption.empty()) {
        dst.push_back(makeCaption(caption));
    }
}

// ---------------------------------------------------------------- абзацы

void dropTrailingEmptyRuns(std::vector<md::InlineElement>& elems) {
    while (!elems.empty() && elems.back().kind == md::InlineElement::Kind::Run &&
           elems.back().run.text.empty()) {
        elems.pop_back();
    }
}

void mapGroup(const std::vector<md::InlineElement>& group, md::Align align,
              std::vector<sd::BlockPtr>& dst, ResourceCollector& rc, bool allowImages) {
    std::vector<md::InlineElement> cur;
    auto flushCur = [&] {
        dropTrailingEmptyRuns(cur);
        if (!cur.empty()) {
            auto p = newBlock(sd::Block::Kind::Paragraph);
            p->align = alignStr(align);
            p->text = sd::buildText(cur);
            dst.push_back(std::move(p));
        }
        cur.clear();
    };
    for (const md::InlineElement& e : group) {
        if (e.kind != md::InlineElement::Kind::Image) {
            cur.push_back(e);
            continue;
        }
        if (allowImages) {
            flushCur(); // картинка — блочный виджет, разрывает абзац
            emitImage(e.image, dst, rc);
        } else {
            // внутри заголовков/ячеек картинка заменяется alt-текстом
            const std::string alt = e.image.alt.empty() ? e.image.url : e.image.alt;
            if (!alt.empty()) {
                cur.push_back(md::InlineElement{md::InlineElement::Kind::Run,
                                                md::InlineRun{alt, md::StyleFlags{}, ""},
                                                md::InlineImage{}});
            }
        }
    }
    flushCur();
}

void mapParagraph(const md::Block& b, std::vector<sd::BlockPtr>& dst, ResourceCollector& rc,
                  bool allowImages) {
    for (const auto& group : md::splitByLineBreaks(b.inlines)) { // жёсткие переносы -> новые абзацы
        mapGroup(group, b.align, dst, rc, allowImages);
    }
}

// ---------------------------------------------------------------- цитаты и заголовки

void flattenQuotes(std::vector<md::BlockPtr>& src, std::vector<md::BlockPtr>& out) {
    for (auto& child : src) {
        if (child->type == md::Block::Type::Quote) {
            flattenQuotes(child->children, out); // вложенность цитат схлопываем
        } else {
            out.push_back(std::move(child));
        }
    }
}

void mapHeader(const md::Block& b, std::vector<sd::BlockPtr>& dst, bool topLevel) {
    if (topLevel && b.level >= 1 && b.level <= 5) {
        auto h = newBlock(sd::Block::Kind::Header);
        h->level = b.level;
        h->text = sd::buildText(b.inlines);
        dst.push_back(std::move(h));
        return;
    }
    // h6 или заголовок внутри цитаты/списка: жирный абзац (уровня 6 в sabydoc нет)
    auto p = newBlock(sd::Block::Kind::Paragraph);
    p->text = sd::forceBold(sd::buildText(b.inlines));
    dst.push_back(std::move(p));
}

} // namespace detail
} // namespace mapping
