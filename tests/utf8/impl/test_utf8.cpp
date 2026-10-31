// utf8 tests: decoding, u16 counting, sanitization.

#include <gtest/gtest.h>

#include <string>

#include "utf8/utf8.hpp"

namespace {

TEST(Utf8, DecodesCyrillic) {
    std::string s = "Текст 🚀 и 👌";
    size_t i = 0;
    bool ok = true;
    const char32_t cp = utf8::decode(s, i, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(cp, U'Т');
}

TEST(Utf8, CountsAstralAsTwoUnits) {
    const std::string rocket = "\xF0\x9F\x9A\x80"; // 🚀
    EXPECT_EQ(utf8::u16length(rocket), 2u);
    EXPECT_EQ(utf8::u16length("Текст"), 5u);
    EXPECT_EQ(utf8::u16length(std::string("a") + rocket + "b"), 4u);
}

TEST(Utf8, SanitizesBrokenSequences) {
    std::string bad = "a\xFF\xC2z";
    bad += "\xE0\x80"; // truncated 3-byte sequence
    const size_t replaced = utf8::sanitize(bad);
    EXPECT_GE(replaced, 2u);
    EXPECT_GE(utf8::u16length(bad), 3u);
}

} // namespace
