#pragma once

#include <string>

/// \brief Natural image dimensions probed from its header bytes.
struct ImageSize {
    unsigned width = 0;
    unsigned height = 0;
    bool ok = false;
};

/// \brief Detects the natural image dimensions from the header bytes
/// (PNG, JPEG, GIF, BMP, WEBP). If the format is not recognized, ok=false.
/// \param bytes The raw image file contents.
/// \return The probed dimensions; ok is false when the format is unknown.
ImageSize probeImageSize(const std::string& bytes);
