"""Generate the native language catalog from the reviewed shared source.
Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""
import json
import re
from pathlib import Path
import xml.etree.ElementTree as ET


def catalog(root: Path):
    source = (root / "PowerEditor/src/ScintillaComponent/ScintillaEditView.cpp").read_text(encoding="utf-8")
    mappings = re.findall(r'\{L"([^"]+)",\s*L"([^"]+)",\s*L"[^"]*",\s*L_\w+,\s*"([^"]+)"\}', source)
    definitions = {node.get("name"): node for node in ET.parse(root / "PowerEditor/src/langs.model.xml").findall("./Languages/Language")}
    def words(name, key):
        node = definitions.get(name)
        if node is None:
            return ""
        item = node.find(f"Keywords[@name='{key}']")
        return item.text or "" if item is not None else ""

    def keywords(name, lexer):
        # XML order is not a lexer keyword-set index. Preserve empty slots.
        keys = ["instre1", "instre2"] + [f"type{i}" for i in range(1, 8)]
        values = [words(name, key) for key in keys]
        if lexer in ("cpp", "javascript"):
            values = [words(name, "instre1"), words(name, "type1"), words("cpp", "type2"), words(name, "instre2")]
        if name == "objc":
            values += [words(name, "type2")]
        if lexer == "tcl":
            values[1], values[2] = values[2], values[1]
        if lexer in ("hypertext", "xml"):
            values = [words("html", "instre1"), words("javascript", "instre1"), words("vb", "instre1"), words("python", "instre1"), words("php", "instre1"), words("xml" if lexer == "xml" else "html", "instre2")]
        if lexer == "json":
            values = [words("json", "instre1"), words("json", "instre2")]
        return values

    result = []
    for name, title, lexer in mappings:
        node = definitions.get(name)
        if node is None or lexer == "user":
            continue
        result.append({"id": name, "name": title, "lexer": lexer, "extensions": node.get("ext", "").split(),
                       "lineComment": node.get("commentLine", ""), "blockOpen": node.get("commentStart", ""),
                       "blockClose": node.get("commentEnd", ""),
                       "keywords": keywords(name, lexer)})
    if len(result) < 70:
        raise ValueError("The shared language catalog could not be parsed completely.")
    return result


if __name__ == "__main__":
    print(json.dumps(catalog(Path(__file__).resolve().parents[1]), ensure_ascii=False, indent=2))
