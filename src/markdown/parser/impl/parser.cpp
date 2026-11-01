#include "markdown/parser/parser.hpp"

#include "utf8/utf8.hpp"
#include "markdown/parser/chars/inline_parser/block_parser/block_parser.hpp"

namespace md {

Document parseMarkdown(const std::string& text) {
    std::vector<std::string> warnings;
    std::string sanitized = text;
    const size_t replaced = utf8::sanitize(sanitized);
    if (replaced) {
        warnings.push_back("invalid UTF-8 sequences replaced: " + std::to_string(replaced));
    }
    BlockParser parser(warnings);
    Document doc = parser.run(sanitized);
    doc.warnings = std::move(warnings);
    return doc;
}

} // namespace md
