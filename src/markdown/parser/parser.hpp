#pragma once

#include <string>

#include "markdown/ast/ast.hpp"

namespace md {

/// Full markdown parser (CommonMark subset + GFM: tables, task lists,
/// strikethrough, links/autolinks, HTML subset).
/// Param text: Raw input text (BOM/CRLF/CR are normalized; invalid UTF-8 is
/// sanitized and a warning is recorded).
/// Returns Parsed document AST.
Document parseMarkdown(const std::string& text);

} // namespace md
