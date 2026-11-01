#include "zip_writer/zip_writer.hpp"

#include <array>
#include <cstddef>
#include <ctime>
#include <stdexcept>

namespace {

constexpr uint32_t makeCrcTableEntry(uint32_t i) {
    uint32_t c = i;
    for (int k = 0; k < 8; ++k) {
        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
    }
    return c;
}

template <size_t... Is>
constexpr std::array<uint32_t, 256> makeCrcTable(std::index_sequence<Is...>) {
    return {{makeCrcTableEntry(Is)...}};
}

constexpr auto kCrcTable = makeCrcTable(std::make_index_sequence<256>{});

void put16(
    std::string& out,
    uint16_t v
){
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
}

void put32(
    std::string& out,
    uint32_t v
){
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
    out.push_back(static_cast<char>((v >> 16) & 0xFF));
    out.push_back(static_cast<char>((v >> 24) & 0xFF));
}

std::pair<uint16_t, uint16_t> currentDosDateTime() {
    std::time_t now = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(
        &tmv,
        &now
    );
#else
    localtime_r(
        &now,
        &tmv
    );
#endif
    uint16_t date =
        static_cast<uint16_t>(((tmv.tm_year - 80) << 9) | ((tmv.tm_mon + 1) << 5) | tmv.tm_mday);
    uint16_t time =
        static_cast<uint16_t>((tmv.tm_hour << 11) | (tmv.tm_min << 5) | (tmv.tm_sec / 2));
    return {time, date};
}

constexpr uint32_t kMaxZip32Size = 0xFFFFFFFFu;

} // namespace

uint32_t crc32(
    const void* data,
    size_t size
){
    const auto* p = static_cast<const std::byte*>(data);
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        crc = kCrcTable[(crc ^ std::to_integer<uint32_t>(p[i])) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

ZipWriter::ZipWriter() = default;

void ZipWriter::addFile(
    const std::string& name,
    const std::string& data
){
    if (mFinished) {
        throw std::runtime_error("ZipWriter: addFile after finish()");
    }
    if (mEntries.size() >= 0xFFFF) {
        throw std::runtime_error("ZipWriter: too many entries (ZIP32 limit: 65535)");
    }
    if (data.size() > kMaxZip32Size || name.size() > 0xFFFF) {
        throw std::runtime_error("ZipWriter: ZIP32 limit exceeded");
    }

    Entry e;
    e.name = name;
    e.data = data;
    e.crc = crc32(data.data(), data.size());
    auto [t, d] = currentDosDateTime();
    e.dosTime = t;
    e.dosDate = d;
    mEntries.push_back(std::move(e));
}

std::string ZipWriter::finish() {
    if (mFinished) {
        throw std::runtime_error("ZipWriter: finish() called twice");
    }
    mFinished = true;

    std::string out;
    std::string central;

    for (Entry& e : mEntries) {
        if (out.size() > kMaxZip32Size - e.data.size() - e.name.size() - 64) {
            throw std::runtime_error("ZipWriter: archive exceeds 4 GB (ZIP32 limit)");
        }
        e.offset = static_cast<uint32_t>(out.size());

        // Local file header
        put32(
            out,
            0x04034b50
        );
        put16(
            out,
            20
        );
        put16(
            out,
            0x0800
        );
        put16(
            out,
            0
        );
        put16(
            out,
            e.dosTime
        );
        put16(
            out,
            e.dosDate
        );
        put32(
            out,
            e.crc
        );
        put32(
            out,
            static_cast<uint32_t>(e.data.size())
        );
        put32(
            out,
            static_cast<uint32_t>(e.data.size())
        );
        put16(
            out,
            static_cast<uint16_t>(e.name.size())
        );
        put16(
            out,
            0
        );
        out += e.name;
        out += e.data;

        // Central directory entry
        put32(
            central,
            0x02014b50
        );
        put16(
            central,
            (3 << 8) | 20
        );
        put16(
            central,
            20
        );
        put16(
            central,
            0x0800
        );
        put16(
            central,
            0
        );
        put16(
            central,
            e.dosTime
        );
        put16(
            central,
            e.dosDate
        );
        put32(
            central,
            e.crc
        );
        put32(
            central,
            static_cast<uint32_t>(e.data.size())
        );
        put32(
            central,
            static_cast<uint32_t>(e.data.size())
        );
        put16(
            central,
            static_cast<uint16_t>(e.name.size())
        );
        put16(
            central,
            0
        );
        put16(
            central,
            0
        );
        put16(
            central,
            0
        );
        put16(
            central,
            0
        );
        put32(
            central,
            0100644u << 16
        );
        put32(
            central,
            e.offset
        );
        central += e.name;
    }

    const uint32_t cdOffset = static_cast<uint32_t>(out.size());
    out += central;

    // End of central directory
    put32(
        out,
        0x06054b50
    );
    put16(
        out,
        0
    );
    put16(
        out,
        0
    );
    put16(
        out,
        static_cast<uint16_t>(mEntries.size())
    );
    put16(
        out,
        static_cast<uint16_t>(mEntries.size())
    );
    put32(
        out,
        static_cast<uint32_t>(central.size())
    );
    put32(
        out,
        cdOffset
    );
    put16(
        out,
        0
    );
    return out;
}
