// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// List editing rules, added 2026-11-01.
#pragma once
#include <string>
#include <string_view>
#include <cctype>
#include <limits>

struct NXListPrefix {
    size_t indent = 0, end = 0;
    bool numbered = false;
    std::string marker;
};
inline NXListPrefix NXParseList(std::string_view line) {
    NXListPrefix p;
    while (p.indent < line.size() && (line[p.indent] == ' ' || line[p.indent] == '\t')) ++p.indent;
    size_t at = p.indent;
    if (line.substr(at, 3) == "\xe2\x80\xa2") at += 3;
    else if (at < line.size() && (line[at] == '-' || line[at] == '*')) ++at;
    else {
        while (at < line.size() && line[at] >= '0' && line[at] <= '9') ++at;
        if (at == p.indent || at >= line.size() || line[at] != '.') return p;
        p.numbered = true; ++at;
    }
    if (at >= line.size() || line[at] != ' ') { p.numbered = false; return p; }
    p.marker = std::string(line.substr(p.indent, at - p.indent));
    p.end = at + 1;
    return p;
}
inline std::string NXNextListMarker(const NXListPrefix &p) {
    if (!p.numbered) return p.marker;
    // Decimal string arithmetic also handles arbitrarily large list numbers.
    std::string n = p.marker.substr(0, p.marker.size() - 1);
    size_t i = n.size();
    while (i && n[i - 1] == '9') n[--i] = '0';
    if (i) ++n[i - 1]; else n.insert(0, "1");
    return n + ".";
}
