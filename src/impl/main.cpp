// mdsd — Markdown <-> SabyDoc converter.
// Currently: md -> sabydoc (forward). Reverse (sabydoc -> md) is planned;
// the -r/--reverse flag is reserved for it.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>

#include "mapping/md_to_sabydoc/md_to_sabydoc.hpp"
#include "markdown/parser/parser.hpp"
#include "sabydoc/model/resource_collector/resource_collector.hpp"
#include "sabydoc/model/widget_writer/document_writer/document_writer.hpp"

namespace {

constexpr const char* kVersion = "0.0.7";

void printUsage() {
    std::printf(
        "mdsd %s — Markdown <-> SabyDoc converter\n"
        "\n"
        "Usage:\n"
        "  mdsd INPUT.md [-o OUTPUT.sabydoc] [options]\n"
        "  mdsd INPUT.sabydoc -r [-o OUTPUT.md]     (not yet implemented)\n"
        "\n"
        "Options:\n"
        "  -o, --output FILE   output file (default: same as INPUT, with swapped extension)\n"
        "      --title TEXT    document title (documentHeader); default: title from\n"
        "                      front matter, then --title, then filename stem\n"
        "      --page-name NAME page name (default \"Basic\")\n"
        "      --strict        treat warnings as errors (exit code 3)\n"
        "  -r, --reverse       reverse conversion: sabydoc -> md (reserved, not yet implemented)\n"
        "  -h, --help          this help\n"
        "      --version       version\n"
        "\n"
        "Exit codes: 0 — success, 1 — I/O error, 2 — invalid arguments, 3 — "
        "warnings with --strict\n",
        kVersion);
}

/// \brief Parsed command-line arguments.
struct Args {
    std::string input;
    std::string output;
    std::string title;
    bool hasTitle = false;
    std::string pageName = "Basic";
    bool strict = false;
    bool reverse = false; ///< Reserved for sabydoc -> md conversion
};

/// \brief Result of argument parsing; main interprets it without magic numbers.
enum class ArgsResult {
    Ok,         ///< Arguments parsed successfully
    HelpShown,  ///< --help/--version: usage already printed, exit with code 0
    UsageError, ///< Invalid arguments: message already printed, show usage
};

ArgsResult parseArgs(
    std::span<char* const> args,
    Args& out
){
    if (args.size() < 2) {
        return ArgsResult::UsageError;
    }
    bool haveInput = false;
    for (size_t i = 1; i < args.size(); ++i) { // args[0] is the program name
        const std::string a = args[i];
        auto needValue = [&](const char* name) -> const char* {
            if (i + 1 >= args.size()) {
                std::fprintf(
                    stderr,
                    "Error: %s requires a value\n",
                    name
                );
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
        } else if (a == "-r" || a == "--reverse") {
            out.reverse = true;
        } else if (a == "-h" || a == "--help") {
            printUsage();
            return ArgsResult::HelpShown;
        } else if (a == "--version") {
            std::printf(
                "mdsd %s\n",
                kVersion
            );
            return ArgsResult::HelpShown;
        } else if (!a.empty() && a[0] == '-') {
            std::fprintf(
                stderr,
                "Error: unknown option %s\n",
                a.c_str()
            );
            return ArgsResult::UsageError;
        } else if (!haveInput) {
            out.input = a;
            haveInput = true;
        } else {
            std::fprintf(
                stderr,
                "Error: extra positional argument %s\n",
                a.c_str()
            );
            return ArgsResult::UsageError;
        }
    }
    if (!haveInput) {
        std::fprintf(
            stderr,
            "Error: input file not specified\n"
        );
        return ArgsResult::UsageError;
    }
    return ArgsResult::Ok;
}

/// \brief Reads a file into a string. Sets ok=false on failure.
std::string readFile(
    const std::filesystem::path& p,
    bool& ok
){
    std::ifstream f(
        p,
        std::ios::binary
    );
    if (!f) {
        ok = false;
        return {};
    }
    ok = true;
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

/// \brief Writes bytes to a file. Returns false on failure.
bool writeFile(
    const std::filesystem::path& p,
    const std::string& bytes
){
    std::ofstream f(
        p,
        std::ios::binary | std::ios::trunc
    );
    if (!f) {
        return false;
    }
    f.write(
        bytes.data(),
        static_cast<std::streamsize>(bytes.size())
    );
    return static_cast<bool>(f);
}

/// \brief Computes the default output path from the input path and direction.
std::filesystem::path defaultOutput(
    const std::filesystem::path& input,
    bool reverse
){
    const auto ext = reverse ? ".md" : ".sabydoc";
    return input.parent_path() / (input.stem().string() + ext);
}

} // namespace

int main(
    int argc,
    char** argv
){
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

    if (args.reverse) {
        std::fprintf(
            stderr,
            "Error: reverse conversion (sabydoc -> md) is not yet implemented\n"
        );
        return 1;
    }

    namespace fs = std::filesystem;
    const fs::path input(args.input);
    std::error_code ec;
    if (!fs::exists(input, ec) || fs::is_directory(input, ec)) {
        std::fprintf(
            stderr,
            "Error: input file not found: %s\n",
            args.input.c_str()
        );
        return 1;
    }
    fs::path output(args.output);
    if (output.empty()) {
        output = defaultOutput(input, args.reverse);
    }
    if (fs::weakly_canonical(input, ec) == fs::weakly_canonical(output, ec)) {
        std::fprintf(
            stderr,
            "Error: output file is the same as input\n"
        );
        return 1;
    }

    bool ok = false;
    const std::string text = readFile(input, ok);
    if (!ok) {
        std::fprintf(
            stderr,
            "Error: cannot read %s\n",
            args.input.c_str()
        );
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
    ResourceCollector rc(
        input.parent_path(),
        diagnostics
    );
    mapping::Options mopt;
    mopt.title = title;
    mopt.pageName = args.pageName;
    sd::Document sdoc = mapping::map(std::move(doc), rc, mopt);

    const size_t imageCount = sdoc.resources.size();
    std::string bytes;
    try {
        bytes = sd::buildSabydocArchive(sdoc);
    } catch (const std::exception& e) {
        std::fprintf(
            stderr,
            "Error building archive: %s\n",
            e.what()
        );
        return 1;
    }

    if (!writeFile(output, bytes)) {
        std::fprintf(
            stderr,
            "Error: cannot write %s\n",
            output.string().c_str()
        );
        return 1;
    }

    size_t headers = 0;
    for (const auto& b : sdoc.blocks) {
        if (b->kind == sd::Block::Kind::Header) {
            ++headers;
        }
    }

    std::printf(
        "Done: %s (%zu bytes)\n",
        output.string().c_str(),
        bytes.size()
    );
    std::printf("  blocks: %zu, headers: %zu, images: %zu\n",
                sdoc.blocks.size(),
                headers,
                imageCount);
    for (const std::string& w : diagnostics) {
        std::printf(
            "  [warning] %s\n",
            w.c_str()
        );
    }

    if (args.strict && !diagnostics.empty()) {
        std::fprintf(
            stderr,
            "--strict: %zu warnings\n",
            diagnostics.size()
        );
        return 3;
    }
    return 0;
}
