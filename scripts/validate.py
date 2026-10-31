#!/usr/bin/env python3
"""Structural validator for .sabydoc files.

Checks the same invariants that were extracted from the reference TestDocument.sabydoc:
ZIP integrity, golden attribute schema (required/optional), id uniqueness,
formats = [offset, LENGTH, props] in UTF-16 units, formattedText dichotomy,
TOC consistency with headers, resources in place.

Usage: python3 scripts/validate.py FILE.sabydoc [FILE2.sabydoc ...]
Also works on the reference TestDocument.sabydoc (validator self-test).
"""
import json
import sys
import zipfile
from collections import defaultdict

GOLDEN = {
    "DocsEditor/documentHeader:View": ({"draggable", "id"}, set()),
    "DocsEditor/documentLayout:View": ({"id"}, set()),
    "FrameControls/columnsLayout:View": ({"gap", "id", "itemsWidth"}, set()),
    "ModuleEditor/baseWidgets:Header1": ({"id", "size", "textStyle"}, set()),
    "ModuleEditor/baseWidgets:Header2": ({"id", "size", "textStyle"}, set()),
    "ModuleEditor/baseWidgets:Header3": ({"id", "size", "textStyle"}, set()),
    "ModuleEditor/baseWidgets:Header4": ({"id", "size", "textStyle"}, set()),
    "ModuleEditor/baseWidgets:Header5": ({"id", "size", "textStyle"}, set()),
    "ModuleEditor/blockquote:View": ({"id"}, set()),
    "ModuleEditor/codeBlock:View": ({"id", "value", "valueWithTokens"}, set()),
    "ModuleEditor/emoji:View": ({"id"}, set()),
    "ModuleEditor/formattedText:View": ({"formats", "id"}, {"children"}),
    "ModuleEditor/image:View": (
        {".style", "action", "aspectRatioStyle", "id", "imageFit", "isAutoHeight", "saveProportions", "value"},
        set(),
    ),
    "ModuleEditor/list:Item": ({"id"}, {"checked"}),
    "ModuleEditor/list:ListItemContent": ({"id"}, {"textStyle"}),
    "ModuleEditor/list:View": ({"id", "markerConfigId"}, set()),
    "ModuleEditor/paragraph2:View": ({"id", "textStyle"}, set()),
    "ModuleEditor/paragraph:View": ({"id", "textStyle"}, {"align", "indent"}),
    "ModuleEditor/spoiler:View": ({"id"}, set()),
    "ModuleEditor/table:Cell": ({"id"}, set()),
    "ModuleEditor/table:Row": ({"id"}, set()),
    "ModuleEditor/table:View": ({"columnsConfigs", "id", "stickyColumn", "stickyHeader"}, set()),
    "ModuleEditor/textLayout:View": ({"id"}, {"removable", "selectable"}),
    "SiteEditorWidgets/separator:View": ({"id"}, set()),
}
ALLOWED_MARKERS = {"bulletWhiteBulletSquare", "checkboxes", "numbers"}
ALLOWED_ALIGN = {"center", "justify", "right"}


def utf16len(s):
    return sum(2 if ord(c) > 0xFFFF else 1 for c in s)


def walk(node):
    yield node
    if isinstance(node, list):
        for ch in node:
            yield from walk(ch)


class Checker:
    def __init__(self, path):
        self.path = path
        self.errors = []

    def err(self, msg):
        self.errors.append(msg)

    def run(self):
        try:
            z = zipfile.ZipFile(self.path)
        except Exception as e:
            self.err(f"cannot open as ZIP: {e}")
            return self.errors
        if z.testzip() is not None:
            self.err("CRC mismatch in at least one file")

        try:
            content = json.loads(z.read("content.json"))
            site = json.loads(z.read("index.site"))
        except Exception as e:
            self.err(f"cannot parse JSON: {e}")
            return self.errors

        pages = site.get("pages") or []
        if len(pages) != 1:
            self.err(f"expected 1 page, found {len(pages)}")
            return self.errors
        page = pages[0]
        page_id = page.get("id")

        view = content.get("view") or {}
        if view.get("siteId") != page_id or view.get("originId") != page_id:
            self.err("content.json: siteId/originId != page id")
        if page.get("constructorId") != "SabyDocConstructor":
            self.err("page constructorId != SabyDocConstructor")

        frame = page.get("content")
        if not (isinstance(frame, list) and frame and frame[0] == "frame"):
            self.err("page content is not a frame")
            return self.errors

        # skeleton
        layout = frame[2] if len(frame) > 2 else None
        if not (isinstance(layout, list) and layout and layout[0] == "DocsEditor/documentLayout:View"):
            self.err("no DocsEditor/documentLayout:View in frame")
            return self.errors
        kids = [c for c in layout[2:] if isinstance(c, list)]
        if not kids or kids[0][0] != "DocsEditor/documentHeader:View":
            self.err("no documentHeader")
        roots = [c for c in kids if c[0] == "ModuleEditor/textLayout:View"]
        if len(roots) != 1:
            self.err(f"expected 1 root textLayout, found {len(roots)}")

        # id uniqueness
        ids = []
        header_ids = []
        for n in walk(frame):
            if isinstance(n, list) and len(n) >= 2 and isinstance(n[1], dict) and "id" in n[1]:
                ids.append(n[1]["id"])
                if isinstance(n[0], str) and "baseWidgets:Header" in n[0]:
                    header_ids.append(n[1]["id"])
        toc_ids = [t["id"] for t in (site.get("propertyValues") or {}).get("tableOfContents") or []]
        ids += toc_ids + [page_id]
        dup = {i for i in ids if ids.count(i) > 1}
        if dup:
            self.err(f"duplicate ids: {sorted(dup)[:5]}")

        # golden schema + invariants
        for n in walk(frame):
            if not (isinstance(n, list) and n and isinstance(n[0], str)):
                continue
            tag = n[0]
            if tag not in GOLDEN:
                continue
            if len(n) < 2 or not isinstance(n[1], dict):
                self.err(f"{tag}: no attributes")
                continue
            keys = set(n[1].keys())
            req, opt = GOLDEN[tag]
            missing = req - keys
            extra = keys - req - opt
            if missing:
                self.err(f"{tag}: missing required attributes {sorted(missing)}")
            if extra:
                self.err(f"{tag}: unexpected attributes {sorted(extra)}")
            if tag == "ModuleEditor/list:View" and n[1].get("markerConfigId") not in ALLOWED_MARKERS:
                self.err(f"unknown markerConfigId: {n[1].get('markerConfigId')}")
            if tag == "ModuleEditor/paragraph:View" and "align" in n[1] and n[1]["align"] not in ALLOWED_ALIGN:
                self.err(f"invalid align: {n[1]['align']}")
            if tag.startswith("ModuleEditor/baseWidgets:Header"):
                size = n[1].get("size")
                if size not in (1, 2, 3, 4, 5):
                    self.err(f"{tag}: size={size}")

        # formattedText: dichotomy + formats
        for n in walk(frame):
            if not (isinstance(n, list) and n and n[0] == "ModuleEditor/formattedText:View"):
                continue
            attrs = n[1] if len(n) > 1 and isinstance(n[1], dict) else {}
            text_children = [c for c in n[2:] if isinstance(c, str)] if len(n) > 2 else []
            has_text = len(text_children) == 1
            has_children_attr = "children" in attrs
            if has_text == has_children_attr:
                self.err("formattedText: dichotomy violation (text child XOR children:\"\")")
                continue
            if has_children_attr and attrs["children"] != "":
                self.err('formattedText: children must be ""')
            text = text_children[0] if has_text else ""
            L = utf16len(text)
            prev_end = -1
            fmts = attrs.get("formats", [])
            if not fmts:
                self.err("formattedText: empty formats")
            for f in fmts:
                off, ln = f[0], f[1]
                if not isinstance(off, int) or not isinstance(ln, int):
                    self.err(f"formats: non-integer fields {f[:2]}")
                    continue
                if off + ln > L:
                    self.err(f"formats [{off},{ln}] exceeds u16 length {L}: {text[:40]!r}")
                if ln > 0 and off < prev_end:
                    self.err(f"formats: overlap/unsorted at {off} (prev_end={prev_end})")
                if ln > 0:
                    prev_end = off + ln
                props = f[2] if len(f) > 2 else {}
                if not isinstance(props, dict):
                    self.err("formats: props is not an object")
                if "href" in props:
                    if props.get("visibleLinkHeading") != "visible":
                        self.err("formats: link missing visibleLinkHeading:visible")
            # links must be surrounded by zero anchors
            for i, f in enumerate(fmts):
                if len(f) > 2 and isinstance(f[2], dict) and "href" in f[2]:
                    ok_before = i > 0 and fmts[i - 1][1] == 0
                    ok_after = i + 1 < len(fmts) and fmts[i + 1][1] == 0
                    if not (ok_before and ok_after):
                        self.err(f"formats: link [{f[0]},{f[1]}] missing zero anchors on both sides")

        # TOC
        toc = (site.get("propertyValues") or {}).get("tableOfContents") or []
        if len(toc) != len(header_ids):
            self.err(f"TOC: {len(toc)} entries vs {len(header_ids)} headers")
        else:
            stack = []
            for i, t in enumerate(toc):
                if t["id"] != "toc_" + header_ids[i]:
                    self.err(f"TOC[{i}]: id does not match header")
                lvl = t["level"]
                while stack and stack[-1][0] >= lvl:
                    stack.pop()
                expect_parent = "toc_" + stack[-1][1] if stack else None
                if t["parentId"] != expect_parent:
                    self.err(f"TOC[{i}]: parentId {t['parentId']} != {expect_parent}")
                stack.append((lvl, header_ids[i]))
            has_child = {t["id"]: t["hasChild"] for t in toc}
            parents = {t["parentId"] for t in toc if t["parentId"]}
            for tid, hc in has_child.items():
                if hc != (tid in parents):
                    self.err(f"TOC: hasChild inconsistent at {tid}")

        # resources
        meta = frame[1] if len(frame) > 1 and isinstance(frame[1], dict) else {}
        resources = meta.get("resources") or {}
        names = set(z.namelist())
        for key, r in resources.items():
            path = r.get("value", "")
            if path.startswith("./"):
                path = path[2:]  # "./resources/x.png" -> "resources/x.png"
            if path not in names:
                self.err(f"resource {key}: file {path} not in archive")
            if not str(r.get("naturalWidth", 0)).isdigit() or r.get("naturalWidth", 0) <= 0:
                self.err(f"resource {key}: invalid naturalWidth")
        # image:View references valid resource keys
        for n in walk(frame):
            if isinstance(n, list) and n and n[0] == "ModuleEditor/image:View":
                v = n[1].get("value")
                if v not in resources:
                    self.err(f"image:View references unknown resource {v!r}")
                w = n[1].get(".style", {}).get("width", "")
                if not (isinstance(w, str) and w.endswith("px")):
                    self.err(f"image:View: invalid .style.width={w!r}")
        return self.errors


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    total_errors = 0
    for path in sys.argv[1:]:
        c = Checker(path)
        errs = c.run()
        if errs:
            print(f"FAIL {path}")
            for e in errs:
                print(f"  - {e}")
            total_errors += len(errs)
        else:
            print(f"OK   {path}")
    if total_errors:
        print(f"\nTotal errors: {total_errors}")
        return 1
    print("\nAll checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
