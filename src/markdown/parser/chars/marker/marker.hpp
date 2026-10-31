#pragma once

// Объект «маркер списка»: -, *, +, 1., 1) и вычисление отступа контента.

#include <string>

namespace md::detail {

struct Marker {
    bool ok = false;
    bool ordered = false;
    size_t indent = 0;
    size_t contentIndent = 0;
    std::string rest;
};

Marker matchListMarker(const std::string& line);

} // namespace md::detail
