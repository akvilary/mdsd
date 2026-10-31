// Tests of the image size sniffer on synthetic headers.

#include <gtest/gtest.h>

#include <string>

#include "image_size/image_size.hpp"

namespace {

TEST(ImageSize, Png) {
    // signature + IHDR 5x3
    const std::string png =
        std::string("\x89PNG\r\n\x1A\n", 8) + std::string("\x00\x00\x00\x0DIHDR", 8) +
        std::string("\x00\x00\x00\x05\x00\x00\x00\x03", 8) + std::string(16, '\0');
    const ImageSize s = probeImageSize(png);
    EXPECT_TRUE(s.ok);
    EXPECT_EQ(s.width, 5u);
    EXPECT_EQ(s.height, 3u);
}

TEST(ImageSize, Gif) {
    const std::string gif = "GIF89a" + std::string("\x20\x00\x10\x00", 4) + std::string(10, '\0');
    const ImageSize s = probeImageSize(gif);
    EXPECT_TRUE(s.ok);
    EXPECT_EQ(s.width, 32u);
    EXPECT_EQ(s.height, 16u);
}

TEST(ImageSize, Jpeg) {
    // SOI + SOF0 (len 8, precision 8, H=2 W=3)
    const std::string jpg = std::string("\xFF\xD8\xFF\xC0\x00\x08\x08\x00\x02\x00\x03", 11);
    const ImageSize s = probeImageSize(jpg);
    EXPECT_TRUE(s.ok);
    EXPECT_EQ(s.width, 3u);
    EXPECT_EQ(s.height, 2u);
}

TEST(ImageSize, UnknownFormat) { EXPECT_FALSE(probeImageSize("not an image").ok); }

} // namespace
