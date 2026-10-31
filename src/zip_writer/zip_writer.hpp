#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Минимальный writer ZIP-архивов без сжатия (method=store) —
// именно в таком виде хранится .sabydoc. ZIP64 не поддерживается:
// при превышении лимитов (4 ГБ, 65535 записей) бросается исключение.
class ZipWriter {
  public:
    ZipWriter();

    void addFile(const std::string& name, const std::string& data);
    std::string finish();

  private:
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

uint32_t crc32(const void* data, size_t size);
