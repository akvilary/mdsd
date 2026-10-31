// Тесты zip: структура архива, CRC-вектор, защита от повторного finish/addFile.

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "zip_writer/zip_writer.hpp"

namespace {

TEST(Zip, Crc32Vector) { EXPECT_EQ(crc32("123456789", 9), 0xCBF43926u); }

TEST(Zip, ArchiveStructure) {
    ZipWriter zw;
    zw.addFile("content.json", "{\"v\":1}");
    zw.addFile("resources/img.png", std::string(1000, 'x'));
    const std::string arc = zw.finish();
    EXPECT_GT(arc.size(), 1100u);
    EXPECT_EQ(std::memcmp(arc.data(), "PK\x03\x04", 4), 0);
    EXPECT_EQ(arc.rfind("PK\x05\x06"), arc.size() - 22);
}

TEST(Zip, ForbidsDoubleFinish) {
    ZipWriter zw;
    zw.addFile("x", "1");
    (void)zw.finish();
    EXPECT_THROW((void)zw.finish(), std::exception);
}

TEST(Zip, ForbidsAddAfterFinish) {
    ZipWriter zw;
    zw.addFile("x", "1");
    (void)zw.finish();
    EXPECT_THROW(zw.addFile("y", ""), std::exception);
}

} // namespace
