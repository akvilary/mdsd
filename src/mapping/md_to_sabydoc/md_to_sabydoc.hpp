#pragma once

#include <string>

#include "markdown/ast/ast.hpp"
#include "sabydoc/model/model.hpp"
#include "sabydoc/model/resource_collector/resource_collector.hpp"

// Маппинг md AST -> модель SabyDoc. Здесь живёт вся политика перевода:
// h6 -> жирный абзац, hr -> separator, картинки с подписями, разделительные
// пустые абзацы после спец-блоков и т.д. Симметричный sabydoc_to_md — в будущем.
namespace mapping {

struct Options {
    std::string title;
    std::string pageName = "Базовый";
};

sd::Document map(md::Document doc, ResourceCollector& rc, const Options& opt);

} // namespace mapping
