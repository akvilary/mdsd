#include "image_size/image_size.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "algo/algo.hpp"

namespace {

// Binary format signatures, spelled as string literals for readability.
constexpr auto kPngSig = ut::toBytes("\x89PNG\r\n\x1A\n"); // PNG file signature
constexpr auto kJpegSoi = ut::toBytes("\xFF\xD8");         // JPEG start-of-image marker
constexpr auto kVp8Magic = ut::toBytes("\x9D\x01\x2A");    // VP8 keyframe magic

uint32_t rdBE32(const std::byte* p) {
    return (std::to_integer<uint32_t>(p[0]) << 24) | (std::to_integer<uint32_t>(p[1]) << 16) |
           (std::to_integer<uint32_t>(p[2]) << 8) | std::to_integer<uint32_t>(p[3]);
}

uint32_t rdLE32(const std::byte* p) {
    return std::to_integer<uint32_t>(p[0]) | (std::to_integer<uint32_t>(p[1]) << 8) |
           (std::to_integer<uint32_t>(p[2]) << 16) | (std::to_integer<uint32_t>(p[3]) << 24);
}

uint32_t rdLE24(const std::byte* p) {
    return std::to_integer<uint32_t>(p[0]) | (std::to_integer<uint32_t>(p[1]) << 8) |
           (std::to_integer<uint32_t>(p[2]) << 16);
}

ImageSize probePng(
    const std::byte* p,
    size_t n
){
    // 8-byte signature, then chunk length and the "IHDR" header type.
    if (n < 33 || !ut::startsWith(p, kPngSig) ||
        !ut::startsWith(p + 12, ut::toBytes("IHDR"))) {
        return {};
    }
    ImageSize s;
    s.width = rdBE32(p + 16);
    s.height = rdBE32(p + 20);
    s.ok = s.width > 0 && s.height > 0;
    return s;
}

ImageSize probeGif(
    const std::byte* p,
    size_t n
){
    if (n < 10 || !ut::startsWith(p, ut::toBytes("GIF8"))) {
        return {};
    }
    ImageSize s;
    s.width = std::to_integer<unsigned>(p[6]) | (std::to_integer<unsigned>(p[7]) << 8);
    s.height = std::to_integer<unsigned>(p[8]) | (std::to_integer<unsigned>(p[9]) << 8);
    s.ok = s.width > 0 && s.height > 0;
    return s;
}

ImageSize probeBmp(
    const std::byte* p,
    size_t n
){
    if (n < 26 || !ut::startsWith(p, ut::toBytes("BM"))) {
        return {};
    }
    int32_t w, h;
    std::memcpy(
        &w,
        p + 18,
        4
    );
    std::memcpy(
        &h,
        p + 22,
        4
    );
    ImageSize s;
    s.width = static_cast<unsigned>(w < 0 ? -w : w);
    s.height = static_cast<unsigned>(h < 0 ? -h : h);
    s.ok = s.width > 0 && s.height > 0;
    return s;
}

ImageSize probeJpeg(
    const std::byte* p,
    size_t n
){
    if (n < 4 || !ut::startsWith(p, kJpegSoi)) {
        return {};
    }
    size_t i = 2;
    while (i + 4 < n) {
        if (p[i] != std::byte{0xFF}) {
            ++i;
            continue;
        }
        const uint8_t marker = std::to_integer<uint8_t>(p[i + 1]);
        if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
            i += 2;
            continue;
        }
        if (i + 4 > n) {
            break;
        }
        const uint32_t segLen = (std::to_integer<uint32_t>(p[i + 2]) << 8) |
                                std::to_integer<uint32_t>(p[i + 3]);
        const bool isSof = (marker >= 0xC0 && marker <= 0xCF) && marker != 0xC4 && marker != 0xC8 &&
                           marker != 0xCC;
        if (isSof && i + 9 <= n) {
            ImageSize s;
            s.height = (std::to_integer<unsigned>(p[i + 5]) << 8) |
                       std::to_integer<unsigned>(p[i + 6]);
            s.width = (std::to_integer<unsigned>(p[i + 7]) << 8) |
                      std::to_integer<unsigned>(p[i + 8]);
            s.ok = s.width > 0 && s.height > 0;
            return s;
        }
        if (marker == 0xD9 || marker == 0xDA) {
            break; // scan data began — dimensions not found
        }
        if (segLen < 2) {
            break;
        }
        i += 2 + segLen;
    }
    return {};
}

ImageSize probeWebp(
    const std::byte* p,
    size_t n
){
    if (n < 30 || !ut::startsWith(p, ut::toBytes("RIFF")) ||
        !ut::startsWith(p + 8, ut::toBytes("WEBP"))) {
        return {};
    }
    size_t i = 12;
    while (i + 8 <= n) {
        std::byte fourcc[4];
        std::memcpy(
            fourcc,
            p + i,
            4
        );
        const uint32_t size = rdLE32(p + i + 4);
        const std::byte* payload = p + i + 8;
        const size_t avail = n - (i + 8);
        if (ut::startsWith(fourcc, ut::toBytes("VP8X")) && avail >= 10) { // extended format
            ImageSize s;
            s.width = rdLE24(payload + 4) + 1;
            s.height = rdLE24(payload + 7) + 1;
            s.ok = true;
            return s;
        }
        if (ut::startsWith(fourcc, ut::toBytes("VP8 ")) && avail >= 10) { // lossy
            if (ut::startsWith(payload + 3, kVp8Magic)) {
                ImageSize s;
                s.width = std::to_integer<unsigned>(payload[6]) |
                          ((std::to_integer<unsigned>(payload[7]) & 0x3F) << 8);
                s.height = std::to_integer<unsigned>(payload[8]) |
                           ((std::to_integer<unsigned>(payload[9]) & 0x3F) << 8);
                s.ok = s.width > 0 && s.height > 0;
                return s;
            }
        }
        if (ut::startsWith(fourcc, ut::toBytes("VP8L")) && avail >= 5 &&
            payload[0] == std::byte{0x2F}) { // lossless
            const uint32_t bits = rdLE32(payload + 1);
            ImageSize s;
            s.width = (bits & 0x3FFFu) + 1;
            s.height = ((bits >> 14) & 0x3FFFu) + 1;
            s.ok = true;
            return s;
        }
        i += 8 + size + (size & 1);
    }
    return {};
}

} // namespace

ImageSize probeImageSize(const std::string& bytes) {
    const auto* p = reinterpret_cast<const std::byte*>(bytes.data());
    const size_t n = bytes.size();
    if (n < 8) {
        return {};
    }
    if (ImageSize s = probePng(p, n); s.ok) {
        return s;
    }
    if (ImageSize s = probeJpeg(p, n); s.ok) {
        return s;
    }
    if (ImageSize s = probeGif(p, n); s.ok) {
        return s;
    }
    if (ImageSize s = probeBmp(p, n); s.ok) {
        return s;
    }
    if (ImageSize s = probeWebp(p, n); s.ok) {
        return s;
    }
    return {};
}
