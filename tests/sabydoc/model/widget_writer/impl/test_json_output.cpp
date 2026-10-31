// Тесты JSON-выхлопа на nlohmann::ordered_json: порядок ключей, экранирование,
// UTF-8 без ASCII-экранирования — контракты, на которые опирается байт-дифф с эталоном.

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <string>

namespace {

using Json = nlohmann::ordered_json;

TEST(JsonOutput, PreservesInsertionOrder) {
    Json o = Json::object();
    o["б"] = 1;
    o["а"] = 2; // не сортируется — порядок вставки
    EXPECT_EQ(o.dump(), "{\"б\":1,\"а\":2}");
}

TEST(JsonOutput, EscapesQuotesNewlinesAndControlChars) {
    const Json j = "значение \"в кавычках\"\nи\tтаб";
    const std::string d = j.dump();
    EXPECT_NE(d.find("\\\"в кавычках\\\""), std::string::npos);
    EXPECT_NE(d.find("\\n"), std::string::npos);
    EXPECT_NE(d.find("\\t"), std::string::npos);
}

TEST(JsonOutput, Utf8PassesThroughUnescaped) {
    const Json j = "Текст 🚀 и 👌";
    const std::string d = j.dump();
    EXPECT_EQ(d.find("\\u"), std::string::npos);           // нет ASCII-экранирования
    EXPECT_NE(d.find("Текст 🚀 и 👌"), std::string::npos); // UTF-8 как есть
}

TEST(JsonOutput, CompactDumpWithoutSpaces) {
    Json o = Json::object();
    o["n"] = nullptr;
    o["b"] = false;
    o["i"] = 0;
    EXPECT_EQ(o.dump(), "{\"n\":null,\"b\":false,\"i\":0}");
}

TEST(JsonOutput, TwoElementArraysNeedExplicitArray) {
    // регрессия ловушки: {"тег", {attrs}} без Json::array станет объектом
    const Json node = Json::array({"tag", Json{{"id", 1}}});
    ASSERT_TRUE(node.is_array());
    ASSERT_EQ(node.size(), 2u);
    EXPECT_EQ(node[0], "tag");
}

} // namespace
