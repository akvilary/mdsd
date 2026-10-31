#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "sabydoc/model/model.hpp"

/// \brief Reads images from disk, detects their dimensions, deduplicates them,
/// and sanitizes their archive names. Warnings go to diagnostics.
class ResourceCollector {
  public:
    /// \brief Constructs the collector bound to a base directory and a diagnostics sink.
    /// \param baseDir Root directory used to resolve relative image paths.
    /// \param diagnostics Receiver for warning messages.
    ResourceCollector(std::filesystem::path baseDir, std::vector<std::string>& diagnostics);

    /// \brief Registers an image. Returns the resource key ("r1") or an empty string
    /// when the image is unavailable (a warning has already been added).
    /// \param url The image reference from markdown.
    /// \param width Output: natural image width in pixels.
    /// \param height Output: natural image height in pixels.
    /// \return The resource key, or "" when the image could not be loaded.
    std::string addImage(const std::string& url, unsigned& width, unsigned& height);

    /// \brief Takes ownership of the collected resources.
    /// \return The collected resources in order of appearance.
    std::vector<sd::Resource> take();

  private:
    std::filesystem::path mBaseDir;
    std::vector<std::string>& mDiagnostics;
    std::map<std::string, sd::Resource> mByPath;
    std::set<std::string> mUsedZipNames;
    int mCounter = 0;
};
