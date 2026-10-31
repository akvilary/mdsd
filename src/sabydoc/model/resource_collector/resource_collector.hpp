#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "sabydoc/model/model.hpp"

// Читает картинки с диска, определяет размеры, дедуплицирует,
// санитизирует имена для архива. Предупреждения — в diagnostics.
class ResourceCollector {
  public:
    ResourceCollector(std::filesystem::path baseDir, std::vector<std::string>& diagnostics);

    // Регистрирует картинку. Возвращает ключ ресурса ("r1") или "",
    // если картинка недоступна (уже добавлено предупреждение).
    std::string addImage(const std::string& url, unsigned& width, unsigned& height);

    std::vector<sd::Resource> take();

  private:
    std::filesystem::path mBaseDir;
    std::vector<std::string>& mDiagnostics;
    std::map<std::string, sd::Resource> mByPath;
    std::set<std::string> mUsedZipNames;
    int mCounter = 0;
};
