# mdsd — Markdown → SabyDoc Converter

A converter from Markdown files (CommonMark subset + GFM) to SabyDoc document
format (`.sabydoc`). **C++20 is the minimum supported standard**. Build via
CMake. The `mdsd` binary has **zero link-time dependencies**: all libraries are
header-only and fetched via `FetchContent` at configure time.

## Dependencies (header-only, fetched at configure time)

| Library | Purpose | Notes |
|---|---|---|
| [nlohmann/json](https://github.com/nlohmann/json) v3.11.3 | JSON generation (`ordered_json` preserves key order for byte-diff with reference); parsing for future reverse conversion | Arrays must be built via `Json::array({...})` — a two-element init-list is treated as an object by nlohmann |
| [utfcpp](https://github.com/nemtrif/utfcpp) v4.2.1 | UTF-8 decoding/validation | Facade at `src/utf8/`; UTF-16 unit semantics (`u16length`, matching JavaScript `String.length`) remains ours — it's a format contract |
| [stduuid](https://github.com/mariusbancila/stduuid) v1.2.3 | UUID v4 generation | Facade at `src/uuid4/`; implements std proposal P0959 |
| [Google Test](https://github.com/google/googletest) v1.14.0 | Unit testing | Test target only |

Hand-written (intentionally): `zip_writer` (store-only ZIP — trivial, verified
by round-trip) and `algo.hpp` (30 lines over `std::ranges`).

## Build

```bash
bash install.sh            # build + install (root: /usr/local; otherwise ~/.local)
bash install.sh build      # build only; binary appears at project root: ./mdsd
bash install.sh test       # build + run unit tests (ctest)
bash install.sh uninstall  # remove installed files
bash install.sh clean      # remove build/
```

`install.sh` works from any directory — artifacts always land in the project
directory. The script auto-detects the best available compiler (prefers newer
g++, then clang++-15+). Override with `CXX` environment variable.

## Usage

```bash
mdsd convert INPUT.md [-o OUTPUT.sabydoc] [--title TEXT] [--page-name NAME] [--strict]
```

- Default output: same as input, with `.sabydoc` extension;
- Document title (`documentHeader`): `title:` from front matter → `--title` →
  filename stem;
- `--strict` — warnings cause exit code 3 (for CI);
- Exit codes: `0` success, `1` I/O error, `2` invalid arguments, `3` warnings in strict mode.

Example:

```bash
mdsd convert examples/test.md
python3 scripts/validate.py examples/test.sabydoc   # structural validation
```

## Feature Mapping

| Markdown | SabyDoc |
|---|---|
| `#`…`#####` | `baseWidgets:Header1..5` + TOC entries |
| `######` | Bold paragraph (level 6 not supported in SabyDoc) |
| paragraph | `paragraph:View` |
| `**x**`, `*x*`, `***x***` | `font.weight/style` in format ranges |
| `~~x~~`, `<u>x</u>` | `decoration.line: line-through / underline` |
| `[x](url)`, `<url>`, bare URL, `[x][ref]` | `href` + `visibleLinkHeading` with zero-length anchors |
| inline code `` `x` `` | plain text (inline code not in format) |
| `-`/`*`/`+`, `1.` | `list:View` (`bulletWhiteBulletSquare` / `numbers`), nesting via list-inside-Item |
| `- [ ]` / `- [x]` | `checkboxes` + `checked: true` on Item |
| `> quote` | `blockquote:View` (nested quotes collapse) |
| GFM table `:--:` | `table:View/Row/Cell`, alignment via `align` on cell paragraphs |
| ``` ``` ``` and indented code | `codeBlock:View` (no highlighting: `valueWithTokens` = plain text) |
| `---` (hr) | `SiteEditorWidgets/separator:View` |
| `![caption](x.png)` | `image:View` + resource in archive + caption `paragraph2:View` |
| `<div align="…">`, `<center>` | paragraph `align` (center/right/justify) |
| hard break (2 spaces / `\`) | separate paragraph |

Styling policy: empty separator paragraphs are inserted after lists/tables/
code/quotes/separators — matching the editor's behavior (cosmetic in the
reference files, not a format requirement).

## Limitations (documented honestly)

- **Remote images** (`http(s)://…`) are not fetched — a visible placeholder
  `[Image: url]` + warning is inserted instead (no network dependencies).
- **Nonexistent image files** — same treatment: placeholder + warning; the
  converter never silently drops content.
- Ordered list starting number (`5.`) is not preserved — SabyDoc always
  numbers from 1.
- Mixing checkboxes and regular items in one list — the entire list becomes
  checkboxes.
- `[![image](x)](url)` — image is preserved, outer link is lost (image widget
  has no href).
- Image paths with spaces must use angle brackets: `(path with spaces)`;
  filenames inside the archive are sanitized to ASCII (`img_<hash>.png`),
  the original name is preserved in the `fileName` attribute.
- Images inside headers and table cells are replaced with alt-text.
- HTML support is a subset: `<u>`, `<b>`, `<i>`, `<s>`, `<code>`, `<a href>`,
  `<img>`, `<br>`, `<div/p/center align>`, comments. Other tags are stripped,
  content is preserved.

## SabyDoc Format (reverse-engineered, programmatically verified)

`.sabydoc` is a ZIP archive without compression (`method=store`):

- `content.json` — page metadata (`siteId` = page id);
- `index.site` — JSONML widget tree + `propertyValues.tableOfContents`;
- `resources/*` — images; descriptors `r1, r2…` in `frame.resources`
  (`file_id`, `naturalWidth/Height`, path).

Key invariants (checked by `scripts/validate.py`):

- `formats` entries = **`[offset, LENGTH, props]`**, offsets in **UTF-16 code
  units** (like `String.length` in JS): astral emoji count as 2;
- links — triple "zero anchor + range with `href` + zero anchor";
- `formattedText`: either one text child OR `children:""` attribute — never both;
- TOC: `toc_<header widget id>`, `parentId` by level stack, `hasChild`
  consistent with children;
- all ids unique; widget attributes exactly match the golden schema
  (`required`/`optional`, extracted from the reference document).

`scripts/validate.py` checks all these invariants and **passes on the reference
`TestDocument.sabydoc`** — proving the validator's own correctness.

## Architecture (ready for reverse conversion)

Directory rule: **only `.hpp` files in directories; each header that needs
implementation keeps it in a local `impl/` subdirectory**. Directory nesting
mirrors the dependency tree: dependent modules live below their dependencies.
All include paths are root-relative (from `src/` or `tests/`), never `../`.

```
src/
  algo/algo.hpp                                (ut::isOneOf/startsWithAny — header only)
  utf8/ json/ zip_writer/ uuid4/ image_size/   (independent leaves: <mod>.hpp + impl/)
  impl/main.cpp                                (CLI: subcommands, std::span)
  markdown/
    ast/{ast.hpp, impl/ast.cpp}                (model for both conversion directions)
    parser/
      parser.hpp + impl/parser.cpp             (public entry point)
      chars/                                   (leaf: string primitives)
        chars.hpp + impl/
        link_def/ marker/ entity/ html_tag/    (children of chars: hpp + impl each)
        detectors/ table_row/
        inline_parser/                         (depends on chars siblings)
          inline_parser.hpp + impl/{core,links,markup}.cpp
          block_parser/                        (depends on inline_parser and siblings)
            block_parser.hpp
            impl/{,_input,blocks,align,table,containers,paragraph}.cpp
  sabydoc/
    model/model.hpp                            (canonical model, pure data)
    model/text_layout/…                        (format ranges in UTF-16)
    model/widget_writer/…                      (widget_writer.hpp + widget_io.hpp +
      impl/{text,blocks,table,frame}.cpp        shared header for impl files)
      document_writer/…                        (index.site + content.json + zip)
    model/resource_collector/…
  mapping/
    md_to_sabydoc/{md_to_sabydoc.hpp, mappers.hpp, impl/map_{blocks,content,containers}.cpp}
tests/                                         (mirrors src structure)
  harness/md_dump.hpp                          (test helpers)
  …/impl/test_*.cpp                            (one test file per module)
```

Future reverse conversion `sabydoc → md` will need symmetric modules
(zip-reader, json-parser, widget_reader, sabydoc_to_md, md-serializer) — no
existing code needs rewriting. Store-ZIP is trivially readable (no inflate).
Fixture pairs for both directions accumulate in tests.

## Quality Assurance

- Tests use **Google Test** (most popular framework; connected via CMake
  `FetchContent` at configure time, affects only the test target — the `mdsd`
  binary remains dependency-free). 70 named tests across 10 suites:
  - `tests/` mirrors `src` (same rules: only hpp in directories, cpp in local
    `impl/`): `utf8/`, `json output/`, `zip_writer/`, `image_size/`, `uuid4/`,
    `sabydoc/model/text_layout/`, `markdown/parser/chars/inline_parser/`
    (+`block_parser/`), `markdown/parser/chars/` (Cyrillic suite), `e2e/`;
  - `harness/md_dump.hpp` — shared helpers (inline dump, `checkInline`);
  - single-module run: `./build/mdsd_tests --gtest_filter=BlockParser.*`;
- `bash install.sh test` — two tests via ctest: **unit** (above) and
  **integration** (convert `examples/test.md` + `scripts/validate.py`
  on result and reference `TestDocument.sabydoc`);
- format reverse-engineering is captured in `scripts/validate.py`.

## What to verify in the SabyDoc editor after first conversion

1. **Astral emoji in styled text** — the UTF-16 hypothesis is strong (JS), but
   the reference samples had no astral characters in `formattedText`; if styles
   shift after an emoji, the fix is one function in `src/utf8/impl/utf8.cpp`;
2. Mid-text link anchors (pattern inferred from a full-string link sample);
3. `codeBlock` rendering with `valueWithTokens` as plain text;
4. List/table inside a quote (not present in reference samples).
