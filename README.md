# mdsd — Markdown → SabyDoc Converter

Converts Markdown files (CommonMark subset + GFM extensions) into SabyDoc
documents (`.sabydoc`) that open in the SabyDoc editor.

- cross-platform: Linux, macOS, Windows;
- single binary, zero link-time dependencies;
- no network needed at run time.

> Internals, directory rules, format invariants and the QA setup live in
> [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Install

Requires Python 3.8+, CMake 3.16+ and a C++20 compiler.
The first build downloads header-only libraries (network needed once).

```python
python3 install.py            # build + install (default)
python3 install.py build      # build only; binary appears at project root: ./mdsd
python3 install.py test       # build + run tests (ctest)
python3 install.py uninstall  # remove installed files
python3 install.py clean      # remove build/
```

Where the binary is installed:

| Platform | User install | System install |
|---|---|---|
| Linux | `~/.local/bin/` | `/usr/local/bin/` (run as root) |
| macOS | `~/.local/bin/` | `/usr/local/bin/` (run as root) |
| Windows | `%LOCALAPPDATA%\Programs\` | `C:\Program Files\` (run as admin) |

`PREFIX=...` overrides the install location, `CXX=...` the compiler.

## Usage

```bash
mdsd INPUT.md [-o OUTPUT.sabydoc] [--title TEXT] [--page-name NAME] [--strict]
```

- default output: the input name with the `.sabydoc` extension;
- document title: `title:` from front matter → `--title` → filename stem;
- `--strict`: warnings make the exit code 3 (useful in CI);
- exit codes: `0` success, `1` I/O error, `2` invalid arguments,
  `3` warnings in strict mode.

Example:

```bash
mdsd examples/test.md
python3 scripts/validate.py examples/test.sabydoc   # structural validation
```

## What is converted

| Markdown | Result in the editor |
|---|---|
| `#`…`#####` | headers of levels 1–5 + table of contents |
| `######` | bold paragraph (level 6 not supported in SabyDoc) |
| paragraph | paragraph |
| `**x**`, `*x*`, `***x***` | bold / italic |
| `~~x~~`, `<u>x</u>` | strike-through / underline |
| `[x](url)`, `<url>`, bare URL, `[x][ref]` | links |
| inline code `` `x` `` | plain text (inline code has no style in the format) |
| `-`/`*`/`+`, `1.` | bullet / numbered lists, nesting supported |
| `- [ ]` / `- [x]` | checkbox list |
| `> quote` | blockquote (nested quotes collapse) |
| GFM table | table with column alignment |
| fenced / indented code | code block (no syntax highlighting) |
| `---` (hr) | separator |
| `![caption](x.png)` | image + caption |
| `<div align="…">`, `<center>` | paragraph alignment (center/right/justify) |
| hard break (2 spaces / `\`) | separate paragraph |

Behavior details:

- an empty separator paragraph is inserted after lists/tables/code/quotes —
  matching the editor's own behavior;
- a list item continues only while its lines are indented at least to the
  item content; a blank line or a smaller indent ends the whole list, and the
  following paragraph/table becomes a sibling block;
- checkbox items (`- [ ]`) and plain items never mix in one list — each run
  of one kind becomes its own list.

## Limitations

- **Remote images** (`http(s)://…`) are not fetched — a visible placeholder
  `[Image: url]` + warning is inserted instead;
- **Nonexistent image files** — same treatment: placeholder + warning; the
  converter never silently drops content;
- the ordered list starting number (`5.`) is not preserved — SabyDoc numbers
  from 1; lists separated by a blank line become distinct collections;
- `[![image](x)](url)` — the image is preserved, the outer link is lost
  (image widget has no href);
- image paths with spaces need angle brackets: `(path with spaces)`;
  filenames inside the archive are sanitized to ASCII, the original name is
  kept in the `fileName` attribute;
- images inside headers and table cells are replaced with alt-text;
- HTML support is a subset: `<u>`, `<b>`, `<i>`, `<s>`, `<code>`,
  `<a href>`, `<img>`, `<br>`, `<div/p/center align>`, comments. Other tags
  are stripped, their content is preserved.

## For contributors

Architecture, dependency rationale, the reverse-engineered format spec and
the test setup: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
