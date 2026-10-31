#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// Minimal ZIP archive writer without compression (method=store) —
/// this is exactly the form .sabydoc files are stored in. ZIP64 is not supported:
/// exceeding the limits (4 GB, 65535 entries) throws an exception.
class ZipWriter {
  public:
    /// Constructs an empty writer.
    ZipWriter();

    /// Adds a file with the given name and contents to the archive.
    /// Param name: The file name inside the archive.
    /// Param data: The file contents.
    void addFile(
        const std::string& name,
        const std::string& data
    );

    /// Finalizes the archive and returns its bytes.
    /// Returns The complete ZIP archive as a byte string.
    std::string finish();

  private:
    /// A single archived file record with its metadata.
    struct Entry {
        std::string name;
        std::string data;
        uint32_t crc = 0;
        uint16_t dosTime = 0;
        uint16_t dosDate = 0;
        uint32_t offset = 0;
    };

    std::vector<Entry> mEntries;
    bool mFinished = false;
};

/// Computes the CRC-32 checksum of a memory block.
/// Param data: Pointer to the input bytes.
/// Param size: The number of bytes to process.
/// Returns The CRC-32 checksum.
uint32_t crc32(
    const void* data,
    size_t size
);
