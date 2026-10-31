#pragma once

// Объект «строка таблицы»: разбиение по пайпам, экранирование, выравнивания.

#include <optional>
#include <string>
#include <vector>

#include "markdown/ast/ast.hpp"

namespace md::detail {

std::vector<std::string> splitTableRow(const std::string& line);
std::optional<Align> delimiterCellAlign(const std::string& c);
std::optional<std::vector<Align>> tableDelimiters(const std::string& line);

} // namespace md::detail
