// Copyright (C) 2026 Berk Kivircik.
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <windows.h>

namespace NoteQuToolbar {
enum Item {
  Family = 1, Size, Bold, Italic, Underline, Strike, Fore, Back, Highlight,
  Bullets, Numbers, CopyFormat, PasteFormat, Clear, Find
};

struct Tool {
  std::string_view key;
  const wchar_t *name;
  int width;
  int group;
};

inline constexpr std::array<Tool, Find> tools{{
    {"font-family", L"Font family", 160, 0},
    {"font-size", L"Font size", 56, 0},
    {"bold", L"Bold", 30, 1},
    {"italic", L"Italic", 30, 1},
    {"underline", L"Underline", 30, 1},
    {"strikethrough", L"Strikethrough", 30, 1},
    {"font-color", L"Font color", 34, 2},
    {"text-background", L"Text background color", 34, 2},
    {"highlight", L"Highlight color", 34, 2},
    {"bullets", L"Bullet list", 30, 3},
    {"numbering", L"Numbered list", 30, 3},
    {"copy-style", L"Copy formatting", 30, 4},
    {"paste-style", L"Apply copied formatting", 30, 4},
    {"clear-style", L"Clear formatting", 30, 4},
    {"find", L"Find in document", 240, 5},
}};

struct Choice {
  Item id{};
  bool visible = true;
  bool operator==(const Choice &) const = default;
};
using Layout = std::array<Choice, tools.size()>;

inline constexpr Layout defaults() {
  Layout layout{};
  for (size_t i = 0; i < layout.size(); ++i)
    layout[i].id = static_cast<Item>(i + Family);
  return layout;
}

inline Layout load(const std::filesystem::path &path) {
  std::error_code error;
  if (std::filesystem::file_size(path, error) > 8192 || error)
    return defaults();
  std::ifstream input(path);
  std::string line;
  if (!std::getline(input, line) || line != "NOTEQUTOOLBAR1")
    return defaults();
  Layout layout{};
  std::array<bool, tools.size()> seen{};
  size_t count = 0;
  while (std::getline(input, line)) {
    if (line.size() < 3 || (line[0] != '0' && line[0] != '1') || line[1] != ' ')
      return defaults();
    const auto key = std::string_view(line).substr(2);
    const auto tool = std::find_if(tools.begin(), tools.end(),
                                   [&](const Tool &t) { return t.key == key; });
    if (tool == tools.end())
      continue;
    const size_t index = static_cast<size_t>(tool - tools.begin());
    if (seen[index])
      return defaults();
    seen[index] = true;
    layout[count++] = {static_cast<Item>(index + Family), line[0] == '1'};
  }
  if (input.bad())
    return defaults();
  for (const auto &choice : defaults())
    if (!seen[choice.id - Family])
      layout[count++] = choice;
  return layout;
}

inline bool save(const std::filesystem::path &path, const Layout &layout) {
  auto temporary = path;
  temporary += L".tmp-" + std::to_wstring(GetCurrentProcessId());
  std::ofstream output(temporary, std::ios::trunc);
  output << "NOTEQUTOOLBAR1\n";
  for (const auto &choice : layout)
    output << (choice.visible ? '1' : '0') << ' '
           << tools[choice.id - Family].key << '\n';
  output.close();
  if (output && MoveFileExW(temporary.c_str(), path.c_str(),
                            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    return true;
  DeleteFileW(temporary.c_str());
  return false;
}
} // namespace NoteQuToolbar
