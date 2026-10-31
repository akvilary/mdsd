#pragma once

#include <vector>

#include "markdown/ast/ast.hpp"
#include "sabydoc/model/model.hpp"

namespace sd {

/// Converts inline markdown elements to a Text with [offset, length] ranges in UTF-16.
/// Images are replaced with alt text (or url), '\n' with a space.
/// Code style is not represented (sabydoc has no inline code) — a documented limitation.
/// Param elems: The inline markdown elements.
/// Returns The resulting rich Text.
Text buildText(const std::vector<md::InlineElement>& elems);

/// Builds plain text without any formatting (captions, placeholders).
/// Param s: The raw string.
/// Returns A Text holding s with no format ranges.
Text plainText(const std::string& s);

/// Returns a copy with bold forced on (h6 and headers inside quotes -> a paragraph).
/// Param t: The text to transform.
/// Returns A copy of t with bold formatting applied.
Text forceBold(Text t);

} // namespace sd
