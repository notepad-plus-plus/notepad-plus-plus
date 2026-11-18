// Copyright (C) 2026 Berk Kivircik.
// SPDX-License-Identifier: GPL-3.0-only
// Created for NoteX; publication date: 2026-10-28.

// NoteX per-range formatting. Independent of lexer styles and file encoding.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace NoteX
{
// Previous release identifiers are read-only compatibility data, not output names.
inline constexpr char previousWireMagic[8] = {78, 80, 80, 80, 82, 79, 49, 0};
inline std::wstring previousBrand() { return {78, 112, 112, 80, 114, 111}; }
inline std::wstring previousSidecarExtension() {
    auto name = previousBrand();
    for (auto &c : name) if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c + L'a' - L'A');
    return L"." + name;
}
inline std::wstring previousClipboardName() { return previousBrand() + L".RichText.v1"; }
using Position = std::int64_t;
enum Field : unsigned
{
    Bold = 1,
    Italic = 2,
    Underline = 4,
    Strike = 8,
    Family = 16,
    Size = 32,
    Fore = 64,
    Back = 128,
    Highlight = 256,
    All = 511
};
constexpr unsigned GetFormat = 51001, ApplyFormat = 51002, GetState = 51003, SetState = 51004;
struct Format
{
    unsigned mask = 0;
    bool bold = false, italic = false, underline = false, strike = false;
    std::string family;
    int size = 1300; // hundredths of a point, matching Scintilla fractional font sizes
    std::uint32_t fore = 0, back = 0, highlight = 0;
    bool operator==(const Format &b) const
    {
        return mask == b.mask && bold == b.bold && italic == b.italic && underline == b.underline &&
               strike == b.strike && family == b.family && size == b.size && fore == b.fore &&
               back == b.back && highlight == b.highlight;
    }
    void merge(const Format &b, unsigned fields)
    {
        mask = (mask & ~fields) | (b.mask & fields);
        if (fields & Bold)
            bold = (b.mask & Bold) ? b.bold : false;
        if (fields & Italic)
            italic = (b.mask & Italic) ? b.italic : false;
        if (fields & Underline)
            underline = (b.mask & Underline) ? b.underline : false;
        if (fields & Strike)
            strike = (b.mask & Strike) ? b.strike : false;
        if (fields & Family)
            family = (b.mask & Family) ? b.family : "";
        if (fields & Size)
            size = (b.mask & Size) ? b.size : 1300;
        if (fields & Fore)
            fore = (b.mask & Fore) ? b.fore : 0;
        if (fields & Back)
            back = (b.mask & Back) ? b.back : 0;
        if (fields & Highlight)
            highlight = (b.mask & Highlight) ? b.highlight : 0;
    }
};
struct Run
{
    Position start = 0, end = 0;
    unsigned style = 0;
    bool operator==(const Run &b) const { return start == b.start && end == b.end && style == b.style; }
};
struct State
{
    std::vector<Format> formats{Format{}};
    std::vector<Run> runs;
};
struct SelectionFormat
{
    Position start = 0, end = 0;
    Format value;
    unsigned mixed = 0;
};
struct Change
{
    Position start = 0, end = 0;
    Format value;
    unsigned fields = All;
};
struct Restore
{
    State state;
    bool undoable = false;
};

// Versioned bounded wire format shared by sidecars and the private clipboard type.
inline std::string encode(const State &s)
{
    std::string out("NOTEX01\0", 8);
    auto number = [&](std::uint64_t n)
    {
        for (int i = 0; i < 8; ++i)
            out.push_back(static_cast<char>(n >> (i * 8)));
    };
    number(s.formats.size());
    number(s.runs.size());
    for (const auto &f : s.formats)
    {
        number(f.mask);
        number((f.bold ? 1 : 0) | (f.italic ? 2 : 0) | (f.underline ? 4 : 0) | (f.strike ? 8 : 0));
        number(f.size);
        number(f.fore);
        number(f.back);
        number(f.highlight);
        number(f.family.size());
        out += f.family;
    }
    for (auto r : s.runs)
    {
        number(r.start);
        number(r.end);
        number(r.style);
    }
    return out;
}
inline bool decode(const std::string &in, State &result, Position length)
{
    if (in.size() < 24 || (in.compare(0, 8, std::string("NOTEX01\0", 8)) &&
        in.compare(0, 8, std::string(previousWireMagic, 8))))
        return false;
    size_t p = 8;
    bool ok = true;
    auto number = [&]()
    {
        std::uint64_t n = 0;
        if (p + 8 > in.size())
        {
            ok = false;
            return n;
        }
        for (int i = 0; i < 8; ++i)
            n |= std::uint64_t(static_cast<unsigned char>(in[p++])) << (i * 8);
        return n;
    };
    auto nf = number(), nr = number();
    if (!nf || nf > 4096 || nr > 1000000 || nf * 56 + nr * 24 > in.size() - 24)
        return false;
    State s;
    s.formats.clear();
    for (size_t i = 0; i < nf; ++i)
    {
        Format f;
        auto mask = number(), flags = number(), size = number(), fore = number(), back = number(),
             highlight = number(), n = number();
        if (!ok || mask > All || flags > 15 || size < 600 || size > 9600 || fore > 0xffffff ||
            back > 0xffffff || highlight > 0xffffff || n > 256 || n > in.size() - p)
            return false;
        f.mask = static_cast<unsigned>(mask);
        f.bold = flags & 1;
        f.italic = flags & 2;
        f.underline = flags & 4;
        f.strike = flags & 8;
        f.size = static_cast<int>(size);
        f.fore = static_cast<unsigned>(fore);
        f.back = static_cast<unsigned>(back);
        f.highlight = static_cast<unsigned>(highlight);
        f.family = in.substr(p, static_cast<size_t>(n));
        p += static_cast<size_t>(n);
        if (f.family.find('\0') != std::string::npos)
            return false;
        s.formats.push_back(f);
    }
    if (!(s.formats[0] == Format{}))
        return false;
    Position previous = 0;
    for (size_t i = 0; i < nr; ++i)
    {
        auto a = number(), b = number(), id = number();
        if (!ok || a < static_cast<std::uint64_t>(previous) || b <= a ||
            b > static_cast<std::uint64_t>(length) || !id || id >= nf)
            return false;
        s.runs.push_back({static_cast<Position>(a), static_cast<Position>(b), static_cast<unsigned>(id)});
        previous = static_cast<Position>(b);
    }
    if (!ok || p != in.size())
        return false;
    result = std::move(s);
    return true;
}

class Formatting
{
  public:
    State state;
    std::uint64_t revision = 0;
    std::map<int, std::vector<Run>> history;
    unsigned at(Position pos) const
    {
        auto i = std::upper_bound(state.runs.begin(), state.runs.end(), pos,
                                  [](Position p, const Run &r) { return p < r.start; });
        if (i == state.runs.begin())
            return 0;
        --i;
        return pos < i->end ? i->style : 0;
    }
    unsigned intern(const Format &f)
    {
        auto i = std::find(state.formats.begin(), state.formats.end(), f);
        if (i != state.formats.end())
            return static_cast<unsigned>(i - state.formats.begin());
        state.formats.push_back(f);
        return static_cast<unsigned>(state.formats.size() - 1);
    }
    void normalize()
    {
        std::vector<Run> out;
        for (auto r : state.runs)
        {
            if (r.end <= r.start || !r.style)
                continue;
            if (!out.empty() && out.back().end == r.start && out.back().style == r.style)
                out.back().end = r.end;
            else
                out.push_back(r);
        }
        state.runs = std::move(out);
        ++revision;
    }
    void checkpoint(int action, bool truncate = false)
    {
        if (state.runs.empty() && history.empty())
            return;
        if (truncate)
            history.erase(history.upper_bound(action), history.end());
        history[action] = state.runs;
    }
    void restoreHistory(int action)
    {
        if (history.empty())
            return;
        auto i = history.upper_bound(action);
        state.runs = i == history.begin() ? std::vector<Run>{} : (--i)->second;
        ++revision;
    }
    void clearHistory() noexcept { history.clear(); }
    bool apply(const Change &c)
    {
        if (c.start < 0 || c.end <= c.start)
            return false;
        std::vector<Position> cuts{c.start, c.end};
        for (auto r : state.runs)
        {
            cuts.push_back(r.start);
            cuts.push_back(r.end);
        }
        std::sort(cuts.begin(), cuts.end());
        cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
        std::vector<Run> next;
        for (size_t n = 1; n < cuts.size(); ++n)
        {
            auto a = cuts[n - 1], b = cuts[n];
            unsigned id = at(a);
            if (a >= c.start && b <= c.end)
            {
                Format f = state.formats[id];
                f.merge(c.value, c.fields);
                id = intern(f);
            }
            if (id)
                next.push_back({a, b, id});
        }
        auto old = state.runs;
        state.runs = std::move(next);
        normalize();
        return old != state.runs;
    }
    void insert(Position pos, Position count)
    {
        if (count <= 0 || state.runs.empty())
            return;
        unsigned inherited = at(pos > 0 ? pos - 1 : pos);
        std::vector<Run> next;
        for (auto r : state.runs)
        {
            if (r.end <= pos)
                next.push_back(r);
            else if (r.start >= pos)
                next.push_back({r.start + count, r.end + count, r.style});
            else
            {
                next.push_back({r.start, pos, r.style});
                next.push_back({pos + count, r.end + count, r.style});
            }
        }
        if (inherited)
            next.push_back({pos, pos + count, inherited});
        std::sort(next.begin(), next.end(), [](const Run &a, const Run &b) { return a.start < b.start; });
        state.runs = std::move(next);
        normalize();
    }
    void erase(Position pos, Position count)
    {
        if (count <= 0 || state.runs.empty())
            return;
        std::vector<Run> next;
        for (auto r : state.runs)
        {
            if (r.end <= pos)
                next.push_back(r);
            else if (r.start >= pos + count)
                next.push_back({r.start - count, r.end - count, r.style});
            else
            {
                if (r.start < pos)
                    next.push_back({r.start, pos, r.style});
                if (r.end > pos + count)
                    next.push_back({pos, r.end - count, r.style});
            }
        }
        state.runs = std::move(next);
        normalize();
    }
    SelectionFormat query(Position start, Position end) const
    {
        SelectionFormat q;
        q.start = start;
        q.end = end;
        q.value = state.formats[at(start)];
        std::vector<Position> points{start};
        for (auto r : state.runs)
        {
            if (r.start > start && r.start < end)
                points.push_back(r.start);
            if (r.end > start && r.end < end)
                points.push_back(r.end);
        }
        for (auto p : points)
            for (unsigned bit = 1; bit <= Highlight; bit <<= 1)
            {
                Format a, b;
                a.merge(q.value, bit);
                b.merge(state.formats[at(p)], bit);
                if (!(a == b))
                    q.mixed |= bit;
            }
        return q;
    }
};
} // namespace NoteX
