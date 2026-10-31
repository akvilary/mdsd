#pragma once

#include <string>

#include "markdown/ast/ast.hpp"
#include "sabydoc/model/model.hpp"
#include "sabydoc/model/resource_collector/resource_collector.hpp"

/// \brief Mapping from the md AST to the SabyDoc model. All translation policy lives here:
/// h6 -> bold paragraph, hr -> separator, images with captions, separator
/// empty paragraphs after special blocks, etc. The symmetric sabydoc_to_md is future work.
namespace mapping {

/// \brief Mapping options supplied by the caller.
struct Options {
    std::string title;
    std::string pageName = "Базовый";
};

/// \brief Maps a markdown document to the SabyDoc model.
/// \param doc The markdown AST root.
/// \param rc Resource collector used to register images.
/// \param opt Mapping options.
/// \return The built SabyDoc document model.
sd::Document map(md::Document doc, ResourceCollector& rc, const Options& opt);

} // namespace mapping
