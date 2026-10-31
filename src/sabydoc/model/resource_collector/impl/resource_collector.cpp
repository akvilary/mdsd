#include "sabydoc/model/resource_collector/resource_collector.hpp"

#include <fstream>
#include <functional>

#include "image_size/image_size.hpp"
#include "uuid4/uuid4.hpp"

namespace {

bool isRemote(const std::string& url) {
    return url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 ||
           url.rfind("//", 0) == 0 || url.rfind("data:", 0) == 0;
}

std::string fnv1a8(const std::string& s) {
    uint64_t h = 1469598103934665603ull;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ull;
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%08llx", static_cast<unsigned long long>((h >> 32) ^ h));
    return buf;
}

bool asciiSafeName(const std::string& s) {
    if (s.empty() || s.size() > 128) {
        return false;
    }
    for (unsigned char c : s) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '.' || c == '_' || c == '-')) {
            return false;
        }
    }
    return !(s == "." || s == "..") && s.front() != '.';
}

std::string sanitizeName(const std::string& name) {
    if (asciiSafeName(name)) {
        return name;
    }
    // keep the extension when it is safe
    std::string ext;
    const size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) {
        std::string cand = name.substr(dot);
        if (cand.size() <= 9 && asciiSafeName(cand)) {
            ext = cand;
        }
    }
    if (ext.empty()) {
        ext = ".png";
    }
    return "img_" + fnv1a8(name) + ext;
}

std::string readFileBytes(const std::filesystem::path& p, bool& ok) {
    std::ifstream f(p, std::ios::binary);
    if (!f) {
        ok = false;
        return {};
    }
    ok = true;
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

} // namespace

ResourceCollector::ResourceCollector(std::filesystem::path baseDir,
                                     std::vector<std::string>& diagnostics)
    : mBaseDir(std::move(baseDir)), mDiagnostics(diagnostics) {}

std::string ResourceCollector::addImage(const std::string& url, unsigned& width, unsigned& height) {
    width = height = 0;
    if (isRemote(url)) {
        mDiagnostics.push_back("remote image not fetched (no network dependencies): " +
                               url);
        return "";
    }
    std::filesystem::path p(url);
    std::error_code ec;
    if (p.is_relative()) {
        p = mBaseDir / p;
    }
    p = std::filesystem::weakly_canonical(p, ec);
    if (ec) {
        p = std::filesystem::absolute(mBaseDir / std::filesystem::path(url), ec);
    }

    const std::string key = p.string();
    const auto it = mByPath.find(key);
    if (it != mByPath.end()) {
        width = it->second.width;
        height = it->second.height;
        return it->second.key;
    }

    bool ok = false;
    const std::string bytes = readFileBytes(p, ok);
    if (!ok) {
        mDiagnostics.push_back("image file not found: " + url);
        return "";
    }
    ImageSize size = probeImageSize(bytes);
    if (!size.ok) {
        mDiagnostics.push_back("cannot determine image dimensions (format not recognized), "
                               "using default values: " +
                               url);
        size.width = 600;
        size.height = 400;
    }

    sd::Resource r;
    r.key = "r" + std::to_string(++mCounter);
    r.fileName = p.filename().string();
    r.fileId = makeUuid4();
    r.zipName = sanitizeName(r.fileName);
    // uniqueness of the name inside the archive
    if (mUsedZipNames.contains(r.zipName)) {
        const size_t dot = r.zipName.find_last_of('.');
        const std::string stem = (dot == std::string::npos) ? r.zipName : r.zipName.substr(0, dot);
        const std::string ext = (dot == std::string::npos) ? "" : r.zipName.substr(dot);
        int n = 2;
        std::string cand;
        do {
            cand = stem + "_" + std::to_string(n++) + ext;
        } while (mUsedZipNames.contains(cand));
        r.zipName = cand;
    }
    mUsedZipNames.insert(r.zipName);
    r.width = size.width;
    r.height = size.height;
    r.bytes = bytes;
    width = r.width;
    height = r.height;

    const std::string rk = r.key;
    mByPath.emplace(key, std::move(r));
    return rk;
}

std::vector<sd::Resource> ResourceCollector::take() {
    std::vector<sd::Resource> out;
    out.reserve(mByPath.size());
    for (auto& [path, r] : mByPath) {
        (void)path;
        out.push_back(std::move(r));
    }
    mByPath.clear();
    return out;
}
