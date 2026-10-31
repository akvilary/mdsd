// mdsd — a Markdown <-> SabyDoc converter.
// Currently: convert (md -> sabydoc). The sabydoc -> md direction is future work
// (the architecture is ready for it, see README).
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>

#include "mapping/md_to_sabydoc/md_to_sabydoc.hpp"
#include "markdown/parser/parser.hpp"
#include "sabydoc/model/widget_writer/document_writer/document_writer.hpp"
#include "sabydoc/model/resource_collector/resource_collector.hpp"

namespace {

constexpr const char* kVersion = "1.0.0";

void printUsage() {
    std::printf(
        "mdsd %s — конвертер Markdown <-> SabyDoc\n"
        "\n"
        "Использование:\n"
        "  mdsd convert ВХОД.md [-o ВЫХОД.sabydoc] [опции]\n"
        "\n"
        "Опции:\n"
        "  -o, --output ФАЙЛ   итоговый .sabydoc (по умолчанию: как ВХОД, с расширением .sabydoc)\n"
        "      --title ТЕКСТ   заголовок документа (documentHeader); по умолчанию: title из\n"
        "                      front matter, иначе --title, иначе имя файла без расширения\n"
        "      --page-name ИМЯ имя страницы (по умолчанию \"Базовый\")\n"
        "      --strict        предупреждения считаются ошибкой (код возврата 3)\n"
        "  -h, --help          эта справка\n"
        "      --version       версия\n"
        "\n"
        "Коды возврата: 0 — успех, 1 — ошибка ввода-вывода, 2 — неверные аргументы, 3 — "
        "предупреждения при --strict\n",
        kVersion);
}

struct Args {
    std::string input;
    std::string output;
    std::string title;
    bool hasTitle = false;
    std::string pageName = "Базовый";
    bool strict = false;
};

// The result of argument parsing; main interprets it without magic numbers.
enum class ArgsResult {
    Ok,         // arguments parsed successfully
    HelpShown,  // --help/--version: usage already printed, exit with code 0
    UsageError, // invalid arguments: message already printed, show usage
};

ArgsResult parseArgs(std::span<char* const> args, Args& out) {
    if (args.size() < 2) {
        return ArgsResult::UsageError;
    }
    const std::string_view sub = args[1]; // args[0] is the program name
    if (sub == "-h" || sub == "--help") {
        printUsage();
        return ArgsResult::HelpShown;
    }
    if (sub == "--version") {
        std::printf("mdsd %s\n", kVersion);
        return ArgsResult::HelpShown;
    }
    if (sub != "convert") {
        return ArgsResult::UsageError;
    }
    bool haveInput = false;
    for (size_t i = 2; i < args.size(); ++i) {
        const std::string a = args[i];
        auto needValue = [&](const char* name) -> const char* {
            if (i + 1 >= args.size()) {
                std::fprintf(stderr, "Ошибка: у %s не хватает значения\n", name);
                std::exit(2);
            }
            ++i;
            return args[i];
        };
        if (a == "-o" || a == "--output") {
            out.output = needValue(a.c_str());
        } else if (a == "--title") {
            out.title = needValue(a.c_str());
            out.hasTitle = true;
        } else if (a == "--page-name") {
            out.pageName = needValue(a.c_str());
        } else if (a == "--strict") {
            out.strict = true;
        } else if (a == "-h" || a == "--help") {
            printUsage();
            return ArgsResult::HelpShown;
        } else if (!a.empty() && a[0] == '-') {
            std::fprintf(stderr, "Ошибка: неизвестная опция %s\n", a.c_str());
            return ArgsResult::UsageError;
        } else if (!haveInput) {
            out.input = a;
            haveInput = true;
        } else {
            std::fprintf(stderr, "Ошибка: лишний позиционный аргумент %s\n", a.c_str());
            return ArgsResult::UsageError;
        }
    }
    if (!haveInput) {
        std::fprintf(stderr, "Ошибка: не указан входной файл\n");
        return ArgsResult::UsageError;
    }
    return ArgsResult::Ok;
}

std::string readFile(const std::filesystem::path& p, bool& ok) {
    std::ifstream f(p, std::ios::binary);
    if (!f) {
        ok = false;
        return {};
    }
    ok = true;
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

bool writeFile(const std::filesystem::path& p, const std::string& bytes) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) {
        return false;
    }
    f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(f);
}

} // namespace

int main(int argc, char** argv) {
    Args args;
    switch (parseArgs(std::span<char* const>(argv, static_cast<size_t>(argc)), args)) {
    case ArgsResult::HelpShown:
        return 0;
    case ArgsResult::UsageError:
        printUsage();
        return 2;
    case ArgsResult::Ok:
        break;
    }

    namespace fs = std::filesystem;
    const fs::path input(args.input);
    std::error_code ec;
    if (!fs::exists(input, ec) || fs::is_directory(input, ec)) {
        std::fprintf(stderr, "Ошибка: входной файл не найден: %s\n", args.input.c_str());
        return 1;
    }
    fs::path output(args.output);
    if (output.empty()) {
        output = input.parent_path() / (input.stem().string() + ".sabydoc");
    }
    if (fs::weakly_canonical(input, ec) == fs::weakly_canonical(output, ec)) {
        std::fprintf(stderr, "Ошибка: выходной файл совпадает со входным\n");
        return 1;
    }

    bool ok = false;
    const std::string text = readFile(input, ok);
    if (!ok) {
        std::fprintf(stderr, "Ошибка: не удалось прочитать %s\n", args.input.c_str());
        return 1;
    }

    md::Document doc = md::parseMarkdown(text);

    std::string title;
    if (args.hasTitle) {
        title = args.title;
    } else if (!doc.title.empty()) {
        title = doc.title;
    } else {
        title = input.stem().string();
    }

    std::vector<std::string> diagnostics = std::move(doc.warnings);
    ResourceCollector rc(input.parent_path(), diagnostics);
    mapping::Options mopt;
    mopt.title = title;
    mopt.pageName = args.pageName;
    sd::Document sdoc = mapping::map(std::move(doc), rc, mopt);

    const size_t imageCount = sdoc.resources.size();
    std::string bytes;
    try {
        bytes = sd::buildSabydocArchive(sdoc);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Ошибка сборки архива: %s\n", e.what());
        return 1;
    }

    if (!writeFile(output, bytes)) {
        std::fprintf(stderr, "Ошибка: не удалось записать %s\n", output.string().c_str());
        return 1;
    }

    size_t headers = 0;
    for (const auto& b : sdoc.blocks) {
        if (b->kind == sd::Block::Kind::Header) {
            ++headers;
        }
    }

    std::printf("Готово: %s (%zu байт)\n", output.string().c_str(), bytes.size());
    std::printf("  блоков: %zu, заголовков: %zu, картинок: %zu\n", sdoc.blocks.size(), headers,
                imageCount);
    for (const std::string& w : diagnostics) {
        std::printf("  [предупреждение] %s\n", w.c_str());
    }

    if (args.strict && !diagnostics.empty()) {
        std::fprintf(stderr, "--strict: есть предупреждения (%zu)\n", diagnostics.size());
        return 3;
    }
    return 0;
}
