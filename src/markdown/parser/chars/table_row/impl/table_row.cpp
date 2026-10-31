#include "markdown/parser/chars/table_row/table_row.hpp"

#include <ranges>

#include "markdown/parser/chars/chars.hpp"

namespace md::detail {

std::vector<std::string> splitTableRow(const std::string& line) {
    // экранированный пайп -> один временный маркер (ровно один!)
    std::string s;
    s.reserve(line.size());
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '\\' && i + 1 < line.size() && line[i + 1] == '|') {
            s += '\x01';
            ++i;
        } else {
            s += line[i];
        }
    }
    std::string_view v = trim(s);
    if (!v.empty() && v.front() == '|') {
        v.remove_prefix(1);
    }
    if (!v.empty() && v.back() == '|') {
        v.remove_suffix(1);
    }

    std::vector<std::string> cells;
    for (const std::string_view part : splitBy(v, '|')) {
        std::string cell;
        for (const char ch : trim(part)) {
            cell.push_back(ch == '\x01' ? '|' : ch);
        }
        cells.push_back(std::move(cell));
    }
    return cells;
}

std::optional<Align> delimiterCellAlign(const std::string& c) {
    if (c.empty()) {
        return std::nullopt;
    }
    const bool l = c.front() == ':';
    const bool r = c.back() == ':';
    const std::string core = c.substr(l ? 1 : 0, c.size() - (l ? 1 : 0) - (r ? 1 : 0));
    if (core.empty() || !std::ranges::all_of(core, [](char ch) { return ch == '-'; })) {
        return std::nullopt;
    }
    if (l && r) {
        return Align::Center;
    }
    if (r) {
        return Align::Right;
    }
    if (l) {
        return Align::Left;
    }
    return Align::None;
}

std::optional<std::vector<Align>> tableDelimiters(const std::string& line) {
    if (line.find('|') == std::string::npos) {
        return std::nullopt;
    }
    const std::vector<std::string> cells = splitTableRow(line);
    if (cells.empty()) {
        return std::nullopt;
    }
    std::vector<Align> aligns;
    aligns.reserve(cells.size());
    for (const std::string& c : cells) {
        if (const auto a = delimiterCellAlign(c)) {
            aligns.push_back(*a);
        } else {
            return std::nullopt;
        }
    }
    return aligns;
}

} // namespace md::detail
