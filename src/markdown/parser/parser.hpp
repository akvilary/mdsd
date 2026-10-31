#pragma once

#include <string>

#include "markdown/ast/ast.hpp"

namespace md {

/// \brief Full markdown parser (CommonMark subset + GFM: tables, task lists,
/// strikethrough, links/autolinks, HTML subset).
/// \param text Raw input text (BOM/CRLF/CR are normalized; invalid UTF-8 is
/// sanitized and a warning is recorded).
/// \return Parsed document AST.
Document parseMarkdown(const std::string& text);

} // namespace md
