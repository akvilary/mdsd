// Тесты генератора UUID v4: форма, версия/вариант, отсутствие коллизий.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "uuid4/uuid4.hpp"

namespace {

TEST(Uuid4, ShapeAndVersion) {
    const std::string u = makeUuid4();
    EXPECT_EQ(u.size(), 36u);
    EXPECT_EQ(u[8], '-');
    EXPECT_EQ(u[13], '-');
    EXPECT_EQ(u[18], '-');
    EXPECT_EQ(u[23], '-');
    EXPECT_EQ(u[14], '4');                          // версия
    EXPECT_NE(std::strchr("89ab", u[19]), nullptr); // вариант
}

TEST(Uuid4, NoCollisions) {
    std::vector<std::string> many;
    for (int i = 0; i < 1000; ++i) {
        many.push_back(makeUuid4());
    }
    std::ranges::sort(many);
    EXPECT_EQ(std::ranges::adjacent_find(many), many.end());
}

} // namespace
