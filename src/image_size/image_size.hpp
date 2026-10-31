#pragma once

#include <string>

/// Natural image dimensions probed from its header bytes.
struct ImageSize {
    unsigned width = 0;
    unsigned height = 0;
    bool ok = false;
};

/// Detects the natural image dimensions from the header bytes
/// (PNG, JPEG, GIF, BMP, WEBP). If the format is not recognized, ok=false.
/// Param bytes: The raw image file contents.
/// Returns The probed dimensions; ok is false when the format is unknown.
ImageSize probeImageSize(const std::string& bytes);
