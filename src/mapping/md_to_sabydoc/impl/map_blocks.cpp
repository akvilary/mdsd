// Mapping dispatcher, separator policy, and the map() entry point.

#include "mapping/md_to_sabydoc/md_to_sabydoc.hpp"

#include <utility>

#include "uuid4/uuid4.hpp"
#include "sabydoc/model/text_layout/text_layout.hpp"

#include "mapping/md_to_sabydoc/mappers.hpp"

namespace mapping {

// ---------------------------------------------------------------- dispatcher

void mapSingleBlock(
    md::BlockPtr& b,
    std::vector<sd::BlockPtr>& dst,
    ResourceCollector& rc,
    bool topLevel,
    int depth
){
    switch (b->type) {
    case md::Block::Type::Paragraph:
        mapParagraph(
            *b,
            dst,
            rc,
            true
        );
        break;
    case md::Block::Type::Header:
        mapHeader(
            *b,
            dst,
            topLevel
        );
        break;
    case md::Block::Type::Code: {
        auto c = newBlock(sd::Block::Kind::Code);
        c->code = b->codeText;
        dst.push_back(std::move(c));
        break;
    }
    case md::Block::Type::Quote: {
        auto q = newBlock(sd::Block::Kind::Quote);
        std::vector<md::BlockPtr> flat;
        flattenQuotes(b->children, flat);
        mapBlocks(
            flat,
            q->children, rc, false, depth + 1
        );
        dst.push_back(std::move(q));
        break;
    }
    case md::Block::Type::List:
        mapList(
            *b,
            dst,
            rc,
            depth
        );
        break;
    case md::Block::Type::Table:
        mapTable(
            *b,
            dst
        );
        break;
    case md::Block::Type::Rule:
        dst.push_back(newBlock(sd::Block::Kind::Separator));
        break;
    }
}

void mapBlocks(
    std::vector<md::BlockPtr>& src,
    std::vector<sd::BlockPtr>& dst,
    ResourceCollector& rc,
    bool topLevel,
    int depth
){
    if (depth > kMaxDepth) {
        return;
    }
    for (size_t k = 0; k < src.size(); ++k) {
        mapSingleBlock(
            src[k],
            dst,
            rc,
            topLevel,
            depth
        );
        // separator policy: after a special block — an empty paragraph if more content follows
        if (!dst.empty() && needsTrailingSeparator(dst.back()->kind) && k + 1 < src.size()) {
            dst.push_back(makeSeparatorMimic());
        }
    }
}

sd::Document map(
    md::Document doc,
    ResourceCollector& rc,
    const Options& opt
){
    sd::Document out;
    out.title = opt.title;
    out.pageName = opt.pageName;
    out.pageId = makeUuid4();
    out.layoutId = makeUuid4();
    out.rootTextLayoutId = makeUuid4();
    out.headerId = makeUuid4();
    std::vector<md::BlockPtr> src = std::move(doc.blocks);
    mapBlocks(
        src,
        out.blocks,
        rc,
        true,
        0
    );
    out.resources = rc.take();
    return out;
}

} // namespace mapping
