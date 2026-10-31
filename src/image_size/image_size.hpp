#pragma once

#include <string>

struct ImageSize {
    unsigned width = 0;
    unsigned height = 0;
    bool ok = false;
};

// Определение натуральных размеров изображения по заголовкам bytes
// (PNG, JPEG, GIF, BMP, WEBP). Если формат не распознан — ok=false.
ImageSize probeImageSize(const std::string& bytes);
