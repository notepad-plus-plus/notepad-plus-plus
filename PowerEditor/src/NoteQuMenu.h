// Copyright (C) 2026 Berk Kivircik.
// SPDX-License-Identifier: GPL-3.0-only
// Created for NoteQu; publication date: 2026-10-28.

#pragma once
// clang-format off
#include <windows.h>
#include "NoteQuFormat.h"
#include <commctrl.h>
#include <windowsx.h>
// clang-format on
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

// The application's canonical menu retains its positions and handles for
// plugins, translations and native command updates. Only this owned display
// copy is filtered.
class NoteQuMenu {
public:
  static constexpr UINT Customize = 61090;
  struct Settings {
    std::vector<std::string> order;
    std::set<std::string> hidden;
    bool operator==(const Settings &) const = default;
  };

private:
  HWND owner = nullptr;
  HMENU source = nullptr, display = nullptr;
  std::map<HMENU, HMENU> copies;
  std::map<HMENU, std::pair<HMENU, std::string>> origins;
  std::filesystem::path path;
  Settings settings, draft;
  enum {
    Tree = 100, Reset, Note, Preview, Search, Summary, ExpandAll, CollapseAll,
    Counter, Empty, MoveRow = WM_APP + 1, PaintInitialTree
  };
  static constexpr wchar_t searchCue[] = L"Men\u00fc veya komut ara";
  enum class Section : LPARAM { None, Menus, Buttons };
  enum class Part { None, Chevron, Visibility, Up, Down };
  struct Row {
    std::string key;
    std::wstring label, shortcut;
    wchar_t glyph = 0;
    int depth = 0, commands = 0, rootIndex = -1;
    bool moved = false, canMoveUp = false, canMoveDown = false;
  };
  struct Entry {
    Row row;
    std::vector<Entry> children;
  };
  struct PreviewItem {
    int rootIndex;
    RECT bounds;
  };
  struct RowLayout {
    RECT bounds, chevron, visibility, up, down, label;
  };
  struct Colors {
    COLORREF surface, sunken, field, hover, selected, border, strongBorder,
        text, secondary, muted, disabled, accent, accentInk, accentHover,
        modified, switchOff;
  };
  static constexpr Colors darkColors{
      RGB(28, 33, 38), RGB(23, 27, 31), RGB(34, 40, 46), RGB(42, 49, 55),
      RGB(34, 54, 74), RGB(38, 44, 50), RGB(50, 58, 66), RGB(232, 236, 239),
      RGB(154, 165, 175), RGB(138, 148, 158), RGB(94, 103, 112),
      RGB(90, 169, 240), RGB(11, 22, 32), RGB(116, 183, 243),
      RGB(232, 163, 61), RGB(58, 67, 76)};
  static constexpr Colors lightColors{
      RGB(246, 247, 249), RGB(255, 255, 255), RGB(255, 255, 255),
      RGB(237, 240, 243), RGB(227, 238, 249), RGB(225, 229, 233),
      RGB(201, 208, 215), RGB(26, 31, 36), RGB(78, 88, 98), RGB(91, 102, 112),
      RGB(154, 163, 172), RGB(31, 111, 197), RGB(255, 255, 255),
      RGB(24, 93, 168), RGB(194, 122, 18), RGB(185, 193, 201)};
  const Colors *colors = &lightColors;
  HWND tree = nullptr, hoveredButton = nullptr;
  HWND preview = nullptr, search = nullptr, summary = nullptr,
       counter = nullptr, empty = nullptr, apply = nullptr;
  std::map<int, Entry> entries;
  std::map<std::string, int> hiddenBelow;
  std::set<std::string> expanded, collapsedResults;
  std::wstring query;
  std::string beforeSearch;
  std::vector<PreviewItem> previewItems;
  int hoveredPreview = -1;
  HTREEITEM hoveredRow = nullptr;
  Part hoveredPart = Part::None;
  POINT clickPoint{};
  int dpi = 96;
  HFONT bodyFont = nullptr, rootFont = nullptr, bodyStrike = nullptr,
        rootStrike = nullptr, smallFont = nullptr, captionFont = nullptr;
  HBRUSH surfaceBrush = nullptr, fieldBrush = nullptr, sunkenBrush = nullptr;
  RECT panel{}, searchFrame{};
  std::map<HTREEITEM, Row> treeRows;
  void (*themeDialog)(HWND, HWND) = nullptr;
  UINT (*windowDpi)(HWND) = nullptr;
  int px(int value) const { return MulDiv(value, dpi, 96); }

  struct Item {
    MENUITEMINFOW info{sizeof(MENUITEMINFOW)};
    std::wstring text;
    Item(HMENU menu, int position) {
      info.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU |
                   MIIM_DATA | MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_STRING;
      GetMenuItemInfoW(menu, position, TRUE, &info);
      text.resize(static_cast<size_t>(info.cch) + 1);
      info.dwTypeData = text.data();
      info.cch = static_cast<UINT>(text.size());
      GetMenuItemInfoW(menu, position, TRUE, &info);
      text.resize(wcslen(text.c_str()));
    }
  };
  static std::wstring label(std::wstring s) {
    s.erase(std::remove(s.begin(), s.end(), L'&'), s.end());
    const auto tab = s.find(L'\t');
    if (tab != std::wstring::npos)
      s.resize(tab);
    return s;
  }
  static std::string nameKey(const std::wstring &text) {
    const char *hex = "0123456789abcdef";
    std::string result;
    for (wchar_t c : label(text))
      for (int shift = 12; shift >= 0; shift -= 4)
        result += hex[(c >> shift) & 15];
    return result;
  }
  static UINT firstCommand(HMENU menu) {
    for (int i = 0; i < GetMenuItemCount(menu); ++i) {
      Item item(menu, i);
      if (item.info.hSubMenu) {
        if (UINT id = firstCommand(item.info.hSubMenu))
          return id;
      } else if (!(item.info.fType & MFT_SEPARATOR) && item.info.wID >= 40000 &&
                 item.info.wID < 60000)
        return item.info.wID;
    }
    return 0;
  }
  static std::string key(const Item &item, const std::string &parent,
                         int index) {
    if (parent.empty())
      return "r" + std::to_string(index);
    if (item.info.hSubMenu) {
      UINT id = firstCommand(item.info.hSubMenu);
      return parent +
             (id ? "/p" + std::to_string(id) : "/n" + nameKey(item.text));
    }
    // Plugin/macro/run IDs are allocated at runtime: use their names and
    // ancestry.
    if (item.info.wID >= 20000 && item.info.wID < 40000)
      return parent + "/n" + nameKey(item.text);
    return parent + "/c" + std::to_string(item.info.wID);
  }
  static bool transient(UINT id) {
    // Window/document lists must always reflect the currently open documents.
    return (id >= 11020 && id <= 11059) || (id >= 14020 && id < 15000) ||
           (id >= 41028 && id < 41058);
  }
  std::vector<int> positions(HMENU menu, const std::string &parent,
                             const Settings &s) const {
    std::vector<int> result;
    const int count = GetMenuItemCount(menu);
    if (parent.empty()) {
      for (const auto &k : s.order)
        for (int i = 0; i < count; ++i)
          if (k == "r" + std::to_string(i) &&
              std::find(result.begin(), result.end(), i) == result.end())
            result.push_back(i);
    }
    for (int i = 0; i < count; ++i)
      if (std::find(result.begin(), result.end(), i) == result.end())
        result.push_back(i);
    return result;
  }
  void fill(HMENU target, HMENU original, const std::string &parent) {
    while (GetMenuItemCount(target) > 0)
      RemoveMenu(target, 0, MF_BYPOSITION);
    bool pendingSeparator = false;
    for (int i : positions(original, parent, settings)) {
      Item item(original, i);
      if (item.info.fType & MFT_SEPARATOR) {
        pendingSeparator = GetMenuItemCount(target) > 0;
        continue;
      }
      const auto k = key(item, parent, i);
      if (!transient(item.info.wID) && settings.hidden.count(k))
        continue;
      if (item.info.hSubMenu) {
        const HMENU childSource = item.info.hSubMenu;
        auto &child = copies[childSource];
        if (!child)
          child = CreatePopupMenu();
        origins[child] = {childSource, k};
        fill(child, childSource, k);
        item.info.hSubMenu = child;
      }
      if (pendingSeparator)
        AppendMenuW(target, MF_SEPARATOR, 0, nullptr);
      pendingSeparator = false;
      item.info.dwTypeData = item.text.data();
      InsertMenuItemW(target, static_cast<UINT>(GetMenuItemCount(target)), TRUE,
                      &item.info);
    }
  }
  static bool validKey(const std::string &s) {
    return !s.empty() && s.size() < 4096 && s[0] == 'r' &&
           s.find_first_not_of("r0123456789/pcnabcdef") == std::string::npos;
  }
  static wchar_t buttonGlyph(const std::wstring &text) {
    if (text == L"+" || text == L"\uff0b")
      return L'+';
    if (text == L"\u25bc" || text == L"\u25be")
      return L'\u25bc';
    if (text == L"\u00d7" || text == L"\u2715")
      return L'\u00d7';
    return 0;
  }
  static const wchar_t *buttonLabel(wchar_t glyph) {
    switch (glyph) {
    case L'+': return L"Yeni sekme";
    case L'\u25bc': return L"A\u00e7\u0131k sekmeler listesi";
    default: return L"Etkin sekmeyi kapat";
    }
  }
  std::vector<int> rootPositions(const Settings &value) const {
    auto roots = positions(source, "", value);
    std::erase_if(roots, [&](int index) { return !entries.count(index); });
    return roots;
  }
  std::array<std::vector<int>, 2> sections(const Settings &value) const {
    std::array<std::vector<int>, 2> result;
    for (int index : rootPositions(value))
      result[entries.at(index).row.glyph ? 1 : 0].push_back(index);
    return result;
  }
  bool hidden(const Row &row) const { return draft.hidden.count(row.key) != 0; }
  bool changed(const Row &row) const {
    return hidden(row) != (settings.hidden.count(row.key) != 0) || row.moved;
  }
  void updateAccessibleText(HTREEITEM node, const Row &row) const {
    // Preserve the native item and expose its visibility in its accessible name.
    auto name = row.label;
    if (!row.shortcut.empty())
      name += L", " + row.shortcut;
    name += hidden(row) ? L", gizli" : L", g\u00f6r\u00fcn\u00fcr";
    TVITEMW item{};
    item.mask = TVIF_TEXT;
    item.hItem = node;
    item.pszText = name.data();
    TreeView_SetItem(tree, &item);
  }
  HTREEITEM insert(HTREEITEM parent, const wchar_t *text, Section section) {
    TVINSERTSTRUCTW ins{};
    ins.hParent = parent;
    ins.hInsertAfter = TVI_LAST;
    ins.item.mask = TVIF_TEXT | TVIF_PARAM;
    ins.item.pszText = const_cast<wchar_t *>(text);
    ins.item.lParam = static_cast<LPARAM>(section);
    return TreeView_InsertItem(tree, &ins);
  }
  Entry readEntry(const Item &item, int index, const std::string &prefix, int depth) {
    Entry entry;
    auto &row = entry.row;
    row.key = key(item, prefix, index);
    row.label = label(item.text);
    row.depth = depth;
    const auto tab = item.text.find(L'\t');
    if (tab != std::wstring::npos)
      row.shortcut = item.text.substr(tab + 1);
    if (depth == 0) {
      row.rootIndex = index;
      row.glyph = buttonGlyph(row.label);
      if (row.glyph)
        row.label = buttonLabel(row.glyph);
    }
    const int count = item.info.hSubMenu ? GetMenuItemCount(item.info.hSubMenu) : 0;
    for (int i = 0; i < count; ++i) {
      const Item child(item.info.hSubMenu, i);
      if ((child.info.fType & MFT_SEPARATOR) || transient(child.info.wID))
        continue;
      entry.children.push_back(readEntry(child, i, row.key, depth + 1));
      row.commands += (std::max)(1, entry.children.back().row.commands);
    }
    return entry;
  }
  int match(const std::wstring &label, int &length) const {
    if (query.empty() || label.empty())
      return -1;
    return FindNLSStringEx(L"tr-TR", FIND_FROMSTART | LINGUISTIC_IGNORECASE | NORM_LINGUISTIC_CASING,
                           label.data(), static_cast<int>(label.size()),
                           query.data(), static_cast<int>(query.size()), &length,
                           nullptr, nullptr, 0);
  }
  bool collectMatches(const Entry &entry, std::set<const Entry *> &matches) const {
    int length = 0;
    bool visible = match(entry.row.label, length) >= 0;
    for (const auto &child : entry.children)
      if (collectMatches(child, matches))
        visible = true;
    if (visible)
      matches.insert(&entry);
    return visible;
  }
  HTREEITEM populate(const Entry &entry, HTREEITEM parent,
                     const std::set<const Entry *> &matches) {
    if (!query.empty() && !matches.count(&entry))
      return nullptr;
    const auto &row = entry.row;
    const auto node = insert(parent, row.label.c_str(), Section::None);
    if (!node)
      return nullptr;
    treeRows.emplace(node, row);
    updateAccessibleText(node, row);
    for (const auto &child : entry.children)
      populate(child, node, matches);
    if (TreeView_GetChild(tree, node) &&
        (query.empty() ? expanded.count(row.key) != 0 : !collapsedResults.count(row.key)))
      TreeView_Expand(tree, node, TVE_EXPAND);
    return node;
  }
  std::string selectedKey() const {
    const auto selected = treeRows.find(TreeView_GetSelection(tree));
    return selected == treeRows.end() ? std::string{} : selected->second.key;
  }
  void rememberExpansion() {
    const bool filtered = !query.empty();
    auto &state = filtered ? collapsedResults : expanded;
    for (const auto &[node, row] : treeRows) {
      if (!TreeView_GetChild(tree, node))
        continue;
      const bool open = (TreeView_GetItemState(tree, node, TVIS_EXPANDED) & TVIS_EXPANDED) != 0;
      const bool tracked = filtered ? !open : open;
      if (tracked)
        state.insert(row.key);
      else
        state.erase(row.key);
    }
  }
  void rebuildTree(const std::string &selected, bool firstOpen = false) {
    if (!firstOpen)
      SendMessageW(tree, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(tree);
    treeRows.clear();
    hoveredRow = nullptr;
    hoveredPart = Part::None;
    const auto current = sections(draft);
    const auto saved = sections(settings);
    std::set<const Entry *> matches;
    if (!query.empty())
      for (const auto &[index, entry] : entries) {
        (void)index;
        collectMatches(entry, matches);
      }
    HTREEITEM first = nullptr, selection = nullptr;
    for (size_t section = 0; section < current.size(); ++section) {
      const auto &items = current[section];
      if (std::none_of(items.begin(), items.end(), [&](int i) {
            return query.empty() || matches.count(&entries.at(i));
          }))
        continue;
      insert(TVI_ROOT, section == 0 ? L"MEN\u00dcLER"
                                    : L"MEN\u00dc \u00c7UBU\u011eU D\u00dc\u011eMELER\u0130",
             section == 0 ? Section::Menus : Section::Buttons);
      for (size_t i = 0; i < items.size(); ++i) {
        auto node = populate(entries.at(items[i]), TVI_ROOT, matches);
        if (!node)
          continue;
        auto &row = treeRows.at(node);
        row.moved = items[i] != saved[section][i];
        row.canMoveUp = i > 0;
        row.canMoveDown = i + 1 < items.size();
        if (!first)
          first = node;
      }
    }
    for (const auto &[node, row] : treeRows)
      if (row.key == selected) {
        selection = node;
        break;
      }
    TreeView_SelectItem(tree, selection ? selection : first);
    if (!selection && first)
      TreeView_SelectSetFirstVisible(tree, TreeView_GetRoot(tree));
    if (!firstOpen)
      SendMessageW(tree, WM_SETREDRAW, TRUE, 0);
    const auto message = first || query.empty() ? std::wstring{}
        : L"\u201c" + query + L"\u201d ile e\u015fle\u015fen men\u00fc veya komut yok.";
    SetWindowTextW(empty, message.c_str());
    ShowWindow(tree, message.empty() ? SW_SHOWNA : SW_HIDE);
    ShowWindow(empty, message.empty() ? SW_HIDE : SW_SHOWNA);
    InvalidateRect(tree, nullptr, FALSE);
  }
  void toggle(HTREEITEM node) {
    const auto it = treeRows.find(node);
    if (it == treeRows.end())
      return;
    const auto &row = it->second;
    if (hidden(row))
      draft.hidden.erase(row.key);
    else
      draft.hidden.insert(row.key);
    for (const auto &[item, entry] : treeRows)
      if (entry.key == row.key)
        updateAccessibleText(item, entry);
    updateStatus();
    InvalidateRect(tree, nullptr, FALSE);
  }
  void move(bool up) {
    const auto it = treeRows.find(TreeView_GetSelection(tree));
    if (it == treeRows.end() || !(up ? it->second.canMoveUp : it->second.canMoveDown))
      return;
    const auto key = it->second.key;
    const auto groups = sections(draft);
    const auto &group = groups[it->second.glyph ? 1 : 0];
    const auto index = std::find(group.begin(), group.end(), it->second.rootIndex);
    const int neighbor = *(index + (up ? -1 : 1));
    // Use the complete section order, even when the search hides the neighbor.
    // Swap in the flat order to preserve the interleaving of older settings.
    const auto order = rootPositions(draft);
    draft.order.clear();
    for (int root : order)
      draft.order.push_back(entries.at(root).row.key);
    std::iter_swap(std::find(draft.order.begin(), draft.order.end(), key),
                   std::find(draft.order.begin(), draft.order.end(), entries.at(neighbor).row.key));
    rememberExpansion();
    rebuildTree(key);
    updateStatus();
    SetFocus(tree);
  }
  static std::wstring windowText(HWND h) {
    const int size = GetWindowTextLengthW(h);
    std::wstring value(static_cast<size_t>(size) + 1, L'\0');
    value.resize(GetWindowTextW(h, value.data(), size + 1));
    return value;
  }
  void searchChanged() {
    auto value = windowText(search);
    const auto start = value.find_first_not_of(L" \t\r\n");
    value = start == std::wstring::npos ? L""
        : value.substr(start, value.find_last_not_of(L" \t\r\n") - start + 1);
    if (query == value)
      return;
    auto selected = selectedKey();
    if (query.empty())
      beforeSearch = selected;
    rememberExpansion();
    query = std::move(value);
    collapsedResults.clear();
    rebuildTree(query.empty() ? beforeSearch : selected);
    InvalidateRect(preview, nullptr, FALSE);
  }
  void selectPreview(int index) {
    SetWindowTextW(search, L"");
    for (const auto &[node, row] : treeRows)
      if (row.rootIndex == index) {
        TreeView_SelectItem(tree, node);
        SetFocus(tree);
        break;
      }
  }
  void expandBranch(HTREEITEM node, bool open) const {
    const auto first = TreeView_GetChild(tree, node);
    if (!first)
      return;
    if (open)
      TreeView_Expand(tree, node, TVE_EXPAND);
    for (auto child = first; child; child = TreeView_GetNextSibling(tree, child))
      expandBranch(child, open);
    if (!open)
      TreeView_Expand(tree, node, TVE_COLLAPSE);
  }
  void expandAll(bool open) const {
    for (auto node = TreeView_GetRoot(tree); node; node = TreeView_GetNextSibling(tree, node))
      expandBranch(node, open);
    if (!open)
      TreeView_SelectSetFirstVisible(tree, TreeView_GetRoot(tree));
  }
  int countHidden(const Entry &entry) const {
    int count = 0;
    for (const auto &child : entry.children)
      count += (hidden(child.row) ? 1 : 0) + countHidden(child);
    return count;
  }
  size_t changeCount(const std::array<std::vector<int>, 2> &current) const {
    if (draft == settings)
      return 0;
    size_t count = 0;
    for (const auto &key : draft.hidden)
      count += settings.hidden.count(key) == 0;
    for (const auto &key : settings.hidden)
      count += draft.hidden.count(key) == 0;
    const auto saved = sections(settings);
    const int orderChanges = (current[0] != saved[0]) + (current[1] != saved[1]);
    // A legacy order can also interleave the two sections differently.
    return count + (orderChanges ? orderChanges : rootPositions(draft) != rootPositions(settings));
  }
  void updateStatus() {
    const auto groups = sections(draft);
    int visible = 0, hiddenCount = 0;
    for (const auto &group : groups) {
      for (int index : group) {
        const auto &entry = entries.at(index);
        const int count = countHidden(entry);
        hiddenBelow[entry.row.key] = count;
        hiddenCount += count;
        visible += !entry.row.glyph && !hidden(entry.row);
      }
    }
    auto status = std::to_wstring(visible) + L"/" + std::to_wstring(groups[0].size())
        + L" men\u00fc g\u00f6r\u00fcn\u00fcr";
    if (hiddenCount)
      status += L" \u00b7 " + std::to_wstring(hiddenCount) + L" komut gizli";
    SetWindowTextW(summary, status.c_str());

    std::wstring previewName = L"Men\u00fc \u00e7ubu\u011fu \u00f6nizlemesi";
    for (const auto &group : groups)
      for (int i : group)
        if (const auto &row = entries.at(i).row; !hidden(row))
          previewName += L", " + row.label;
    SetWindowTextW(preview, previewName.c_str());
    InvalidateRect(preview, nullptr, FALSE);

    const auto changes = changeCount(groups);
    const auto count = changes ? std::to_wstring(changes) + L" de\u011fi\u015fiklik" : L"";
    SetWindowTextW(counter, count.c_str());
    InvalidateRect(counter, nullptr, FALSE);
    EnableWindow(apply, changes != 0);
    InvalidateRect(apply, nullptr, FALSE);
  }
  RECT centered(int left, int middle, int width, int height) const {
    const int top = middle - px(height) / 2;
    return {left, top, left + px(width), top + px(height)};
  }
  RowLayout geometry(HTREEITEM node, const Row &row) const {
    RowLayout layout{};
    RECT client{};
    GetClientRect(tree, &client);
    TreeView_GetItemRect(tree, node, &layout.bounds, FALSE);
    layout.bounds.left = 0;
    layout.bounds.right = client.right;
    const int mid = (layout.bounds.top + layout.bounds.bottom) / 2;
    const int right = client.right - px(6);
    int left = px(24);
    if (row.depth == 0) {
      layout.visibility = centered(right - px(30), mid, 30, 16);
      layout.down = centered(layout.visibility.left - px(28), mid, 22, 22);
      layout.up = centered(layout.down.left - px(28), mid, 22, 22);
      layout.label.right = layout.up.left - px(6);
    } else {
      left = px(30 + (row.depth - 1) * 22);
      layout.visibility = centered(left + px(22), mid, 16, 16);
      layout.label.right = right;
    }
    layout.chevron = centered(left, mid, 16, 22);
    layout.label.left = (row.depth == 0 ? layout.chevron.right : layout.visibility.right) + px(6);
    layout.label.top = layout.bounds.top;
    layout.label.bottom = layout.bounds.bottom;
    return layout;
  }
  bool inheritedHidden(HTREEITEM node) const {
    for (auto parent = TreeView_GetParent(tree, node); parent;
         parent = TreeView_GetParent(tree, parent))
      if (hidden(treeRows.at(parent)))
        return true;
    return false;
  }
  void rounded(HDC dc, RECT r, int radius, COLORREF fill,
               COLORREF border = CLR_INVALID) const {
    const auto oldBrush = SelectObject(dc, GetStockObject(DC_BRUSH));
    const auto pen = border == CLR_INVALID ? nullptr : CreatePen(PS_SOLID, px(1), border);
    const auto oldPen = SelectObject(dc, pen ? pen : GetStockObject(NULL_PEN));
    SetDCBrushColor(dc, fill);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, px(radius * 2), px(radius * 2));
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    if (pen)
      DeleteObject(pen);
  }
  static void solid(HDC dc, const RECT &r, COLORREF color) {
    SetDCBrushColor(dc, color);
    FillRect(dc, &r, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
  }
  void disc(HDC dc, int x, int y, int diameter, COLORREF color) const {
    rounded(dc, centered(x - px(diameter) / 2, y, diameter, diameter),
            diameter / 2, color);
  }
  template<size_t N>
  void stroke(HDC dc, int x, int y, COLORREF ink, const POINT (&points)[N]) const {
    POINT scaled[N];
    for (size_t i = 0; i < N; ++i)
      scaled[i] = {x + px(points[i].x), y + px(points[i].y)};
    const auto pen = CreatePen(PS_SOLID, px(2), ink);
    const auto oldPen = SelectObject(dc, pen);
    Polyline(dc, scaled, static_cast<int>(N));
    SelectObject(dc, oldPen);
    DeleteObject(pen);
  }
  static int measure(HDC dc, const std::wstring &value) {
    SIZE size{};
    GetTextExtentPoint32W(dc, value.c_str(), static_cast<int>(value.size()), &size);
    return size.cx;
  }
  static void textLine(HDC dc, const wchar_t *value, RECT r, COLORREF ink,
                       UINT align = DT_LEFT) {
    if (r.right <= r.left)
      return;
    SetTextColor(dc, ink);
    DrawTextW(dc, value, -1, &r,
              align | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
  }
  void drawLabel(HDC dc, const Row &row, RECT bounds, COLORREF ink) const {
    textLine(dc, row.label.c_str(), bounds, ink);
    int length = 0;
    const int start = match(row.label, length);
    if (start < 0 || length <= 0)
      return;
    const int left = bounds.left + measure(dc, row.label.substr(0, start));
    const int right = bounds.left + measure(dc, row.label.substr(0, start + length));
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, left, bounds.top, (std::min)(right, static_cast<int>(bounds.right)),
                      bounds.bottom);
    textLine(dc, row.label.c_str(), bounds, colors->accent);
    RestoreDC(dc, saved);
  }
  int previewHit(POINT point) const {
    for (const auto &item : previewItems)
      if (PtInRect(&item.bounds, point))
        return item.rootIndex;
    return -1;
  }
  void drawPreview(const DRAWITEMSTRUCT &draw) {
    const HDC dc = draw.hDC;
    const auto &c = *colors;
    const int saved = SaveDC(dc);
    solid(dc, draw.rcItem, c.surface);
    rounded(dc, draw.rcItem, 6, c.sunken, c.border);
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, bodyFont);
    previewItems.clear();
    const auto groups = sections(draft);
    const auto visibleButton = [&](int index) { return !hidden(entries.at(index).row); };
    const int buttonCount = static_cast<int>(std::count_if(groups[1].begin(), groups[1].end(), visibleButton));
    const int middle = (draw.rcItem.top + draw.rcItem.bottom) / 2;
    const int slot = px(22);
    const int divider = draw.rcItem.right - px(4) - (buttonCount + 1) * slot - px(5);
    const auto selection = selectedKey();
    auto drawItem = [&](int index, RECT bounds, const wchar_t *text, bool clipped) {
      const auto &row = entries.at(index).row;
      const bool selected = selection == row.key;
      const auto ink = selected ? c.accent : c.text;
      const auto background = selected ? c.selected : hoveredPreview == index ? c.hover : c.sunken;
      if (background != c.sunken)
        rounded(dc, bounds, 4, background);
      if (!clipped) {
        textLine(dc, text, bounds, ink, DT_CENTER);
        return;
      }
      // Blend the clipped label into its background in the last few pixels.
      for (int step = 0; step < 4; ++step) {
        const int right = divider - px(5 * (3 - step));
        const int left = step ? right - px(5) : static_cast<int>(bounds.left);
        const auto blend = [&](int foreground, int base) {
          return (foreground * (4 - step) + base * step) / 4;
        };
        const auto faded = RGB(blend(GetRValue(ink), GetRValue(background)),
                               blend(GetGValue(ink), GetGValue(background)),
                               blend(GetBValue(ink), GetBValue(background)));
        const int clip = SaveDC(dc);
        IntersectClipRect(dc, left, bounds.top, right, bounds.bottom);
        textLine(dc, text, bounds, faded, DT_CENTER);
        RestoreDC(dc, clip);
      }
    };
    const int leftClip = SaveDC(dc);
    IntersectClipRect(dc, draw.rcItem.left + px(3), draw.rcItem.top + px(2),
                      divider, draw.rcItem.bottom - px(2));
    int left = draw.rcItem.left + px(4);
    for (int i : groups[0]) {
      const auto &row = entries.at(i).row;
      if (hidden(row))
        continue;
      const int width = measure(dc, row.label) + px(12);
      auto bounds = centered(left, middle, 0, 24);
      bounds.right = left + width;
      if (left >= divider)
        break;
      const bool clipped = bounds.right > divider;
      drawItem(i, bounds, row.label.c_str(), clipped);
      bounds.right = (std::min)(bounds.right, static_cast<LONG>(divider));
      previewItems.push_back({i, bounds});
      if (clipped)
        break;
      left += width;
    }
    RestoreDC(dc, leftClip);
    solid(dc, {divider, draw.rcItem.top + px(4), divider + px(1),
               draw.rcItem.bottom - px(4)}, c.border);
    left = divider + px(5);
    for (int i : groups[1]) {
      const auto &row = entries.at(i).row;
      if (hidden(row))
        continue;
      const auto bounds = centered(left, middle, 22, 24);
      const wchar_t glyph[] = {row.glyph, 0};
      drawItem(i, bounds, glyph, false);
      previewItems.push_back({i, bounds});
      left += slot;
    }
    textLine(dc, L"\u2699", centered(left, middle, 22, 24), c.secondary, DT_CENTER);
    RestoreDC(dc, saved);
  }
  void drawCounter(const DRAWITEMSTRUCT &draw) const {
    const HDC dc = draw.hDC;
    const int saved = SaveDC(dc);
    solid(dc, draw.rcItem, colors->surface);
    const auto value = windowText(draw.hwndItem);
    if (!value.empty()) {
      SelectObject(dc, smallFont);
      SetBkMode(dc, TRANSPARENT);
      const int left = draw.rcItem.right - measure(dc, value);
      disc(dc, left - px(9), (draw.rcItem.top + draw.rcItem.bottom) / 2, 6, colors->modified);
      textLine(dc, value.c_str(), draw.rcItem, colors->secondary, DT_RIGHT);
    }
    RestoreDC(dc, saved);
  }
  void paintSearch(HDC dc) const {
    const bool focused = GetFocus() == search;
    rounded(dc, searchFrame, 6, colors->field,
            focused ? colors->accent : colors->strongBorder);
    if (focused) {
      auto ring = searchFrame;
      InflateRect(&ring, -px(1), -px(1));
      rounded(dc, ring, 5, colors->field, colors->accent);
    }
    const int y = (searchFrame.top + searchFrame.bottom) / 2;
    rounded(dc, centered(searchFrame.left + px(11), y - px(1), 9, 9),
            4, colors->field, colors->muted);
    const POINT handle[] = {{0, 0}, {4, 4}};
    stroke(dc, searchFrame.left + px(18), y + px(2), colors->muted, handle);
    const auto hint = centered(searchFrame.right - px(52), y, 44, 18);
    rounded(dc, hint, 4, colors->field, colors->strongBorder);
    const auto previous = SelectObject(dc, smallFont);
    SetBkMode(dc, TRANSPARENT);
    textLine(dc, L"Ctrl+F", hint, colors->muted, DT_CENTER);
    SelectObject(dc, previous);
  }
  void drawArrow(HDC dc, RECT r, bool up, bool enabled, bool hot) const {
    if (enabled && hot)
      rounded(dc, r, 4, colors->surface);
    const int x = (r.left + r.right) / 2, y = (r.top + r.bottom) / 2;
    const LONG dir = up ? -1 : 1;
    const POINT arrow[] = {{0, -4 * dir}, {0, 4 * dir},
                           {-3, dir}, {0, 4 * dir}, {3, dir}};
    stroke(dc, x, y, enabled ? colors->secondary : colors->disabled, arrow);
  }
  void drawRow(const NMTVCUSTOMDRAW &draw) const {
    const auto node = reinterpret_cast<HTREEITEM>(draw.nmcd.dwItemSpec);
    const HDC dc = draw.nmcd.hdc;
    const int saved = SaveDC(dc);
    SetBkMode(dc, TRANSPARENT);
    if (draw.nmcd.lItemlParam != static_cast<LPARAM>(Section::None)) {
      RECT bounds{};
      TreeView_GetItemRect(tree, node, &bounds, FALSE);
      solid(dc, bounds, colors->sunken);
      bounds.left = px(8);
      SelectObject(dc, captionFont);
      const auto title = draw.nmcd.lItemlParam == static_cast<LPARAM>(Section::Menus)
          ? L"MEN\u00dcLER" : L"MEN\u00dc \u00c7UBU\u011eU D\u00dc\u011eMELER\u0130";
      textLine(dc, title, bounds, colors->secondary);
    } else if (auto it = treeRows.find(node); it != treeRows.end()) {
      const auto &row = it->second;
      const auto layout = geometry(node, row);
      const int mid = (layout.bounds.top + layout.bounds.bottom) / 2;
      const bool selected = TreeView_GetSelection(tree) == node;
      const bool hover = hoveredRow == node;
      const bool off = hidden(row);
      const bool hasChildren = TreeView_GetChild(tree, node) != nullptr;
      const bool root = row.depth == 0;
      solid(dc, layout.bounds, colors->sunken);
      if (selected || hover)
        rounded(dc, layout.bounds, 4, selected ? colors->selected : colors->hover);
      if (selected)
        rounded(dc, centered(0, mid, 3, 16), 1, colors->accent);
      if (hasChildren) {
        const bool open = (TreeView_GetItemState(tree, node, TVIS_EXPANDED) & TVIS_EXPANDED) != 0;
        const int x = (layout.chevron.left + layout.chevron.right) / 2;
        const POINT right[] = {{-2, -3}, {1, 0}, {-2, 3}};
        const POINT down[] = {{-3, -1}, {0, 2}, {3, -1}};
        stroke(dc, x, mid, colors->secondary, open ? down : right);
      }
      if (root) {
        if (selected || hover) {
          for (int y : {-4, 0, 4})
            for (int x : {9, 13})
              disc(dc, px(x), mid + px(y), 2, colors->disabled);
          drawArrow(dc, layout.up, true, row.canMoveUp, hover && hoveredPart == Part::Up);
          drawArrow(dc, layout.down, false, row.canMoveDown, hover && hoveredPart == Part::Down);
        }
        rounded(dc, layout.visibility, 8, off ? colors->switchOff : colors->accent);
        disc(dc, off ? layout.visibility.left + px(8) : layout.visibility.right - px(8),
             mid, 10, off ? colors->secondary : colors->accentInk);
      } else {
        rounded(dc, layout.visibility, 4, off ? colors->sunken : colors->accent,
                off ? colors->muted : CLR_INVALID);
        if (!off) {
          const POINT tick[] = {{-3, 0}, {-1, 2}, {3, -3}};
          stroke(dc, (layout.visibility.left + layout.visibility.right) / 2,
                 mid, colors->accentInk, tick);
        }
      }

      auto name = layout.label;
      const bool parentOff = inheritedHidden(node);
      const auto ink = parentOff ? colors->disabled : off ? colors->muted : colors->text;
      if (row.glyph) {
        SelectObject(dc, rootFont);
        const auto glyph = centered(name.left, mid, 18, 18);
        rounded(dc, glyph, 4, colors->field, colors->border);
        const wchar_t value[] = {row.glyph, 0};
        textLine(dc, value, glyph, ink, DT_CENTER);
        name.left = glyph.right + px(6);
      }
      SelectObject(dc, smallFont);
      if (root && row.commands && !row.glyph) {
        const int count = hiddenBelow.at(row.key);
        const auto badge = count ? std::to_wstring(count) + L" gizli"
                                 : std::to_wstring(row.commands) + L" komut";
        auto badgeRect = name;
        badgeRect.left = badgeRect.right - measure(dc, badge);
        textLine(dc, badge.c_str(), badgeRect,
                 count ? colors->modified : colors->secondary, DT_RIGHT);
        name.right = badgeRect.left - px(8);
      } else if (!row.shortcut.empty()) {
        auto shortcut = name;
        shortcut.left = shortcut.right - measure(dc, row.shortcut);
        textLine(dc, row.shortcut.c_str(), shortcut,
                 parentOff ? colors->disabled : colors->muted, DT_RIGHT);
        name.right = shortcut.left - px(8);
      }
      if (changed(row)) {
        name.right -= px(10);
        disc(dc, name.right + px(5), mid, 6, colors->modified);
      }
      SelectObject(dc, root ? (off ? rootStrike : rootFont)
                            : (off ? bodyStrike : bodyFont));
      drawLabel(dc, row, name, ink);
    }
    RestoreDC(dc, saved);
  }
  HTREEITEM hit(POINT p, Part &part) const {
    part = Part::None;
    TVHITTESTINFO test{};
    test.pt = p;
    const auto node = TreeView_HitTest(tree, &test);
    const auto it = treeRows.find(node);
    if (it == treeRows.end())
      return node;
    const auto layout = geometry(node, it->second);
    if (PtInRect(&layout.visibility, p))
      part = Part::Visibility;
    else if (TreeView_GetChild(tree, node) && PtInRect(&layout.chevron, p))
      part = Part::Chevron;
    else if (PtInRect(&layout.up, p))
      part = Part::Up;
    else if (PtInRect(&layout.down, p))
      part = Part::Down;
    return node;
  }
  bool navigate(UINT key) const {
    const auto selected = TreeView_GetSelection(tree);
    if (!selected)
      return false;
    HTREEITEM next = nullptr;
    if (key == VK_HOME)
      next = TreeView_GetRoot(tree);
    else if (key == VK_END)
      next = TreeView_GetLastVisible(tree);
    else if (key == VK_UP || key == VK_DOWN) {
      next = key == VK_UP ? TreeView_GetPrevVisible(tree, selected)
                          : TreeView_GetNextVisible(tree, selected);
      if (!next || treeRows.count(next))
        return false;
    } else {
      return false;
    }
    while (next && !treeRows.count(next))
      next = key == VK_UP || key == VK_END ? TreeView_GetPrevVisible(tree, next)
                                           : TreeView_GetNextVisible(tree, next);
    if (next)
      TreeView_SelectItem(tree, next);
    return true;
  }
  void drawSearchCue(HWND edit) const {
    const HDC dc = GetDC(edit);
    if (!dc)
      return;
    const BOOL caretHidden = HideCaret(edit);
    RECT bounds{};
    GetClientRect(edit, &bounds);
    FillRect(dc, &bounds, fieldBrush);
    const auto font = SelectObject(dc, bodyFont);
    SetBkMode(dc, TRANSPARENT);
    bounds.left += px(1);
    textLine(dc, searchCue, bounds, colors->muted);
    SelectObject(dc, font);
    ReleaseDC(edit, dc);
    if (caretHidden)
      ShowCaret(edit);
  }
  static LRESULT CALLBACK controlProc(HWND h, UINT message, WPARAM w, LPARAM l,
                                      UINT_PTR id, DWORD_PTR data) {
    auto &self = *reinterpret_cast<NoteQuMenu *>(data);
    if (message == WM_NCDESTROY)
      RemoveWindowSubclass(h, controlProc, id);
    if (message == WM_MOUSEMOVE && h != self.search) {
      TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, h, 0};
      TrackMouseEvent(&track);
      const POINT point{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
      if (h == self.tree) {
        Part part{};
        const auto row = self.hit(point, part);
        if (row != self.hoveredRow || part != self.hoveredPart) {
          self.hoveredRow = row;
          self.hoveredPart = part;
          InvalidateRect(h, nullptr, FALSE);
        }
      } else if (h == self.preview) {
        const int item = self.previewHit(point);
        if (item != self.hoveredPreview) {
          self.hoveredPreview = item;
          InvalidateRect(h, nullptr, FALSE);
        }
      } else if (h != self.hoveredButton) {
        self.hoveredButton = h;
        InvalidateRect(h, nullptr, FALSE);
      }
    } else if (message == WM_MOUSELEAVE) {
      if (h == self.tree) {
        self.hoveredRow = nullptr;
        self.hoveredPart = Part::None;
      } else if (h == self.preview) {
        self.hoveredPreview = -1;
      } else if (h == self.hoveredButton) {
        self.hoveredButton = nullptr;
      }
      InvalidateRect(h, nullptr, FALSE);
    }
    if (h == self.preview) {
      if (message == WM_LBUTTONUP) {
        const int item = self.previewHit({GET_X_LPARAM(l), GET_Y_LPARAM(l)});
        if (item >= 0)
          self.selectPreview(item);
      }
      return DefSubclassProc(h, message, w, l);
    }

    const HWND dialog = GetParent(h);
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (message == WM_GETDLGCODE) {
      const auto msg = reinterpret_cast<const MSG *>(l);
      if (msg && msg->message == WM_KEYDOWN &&
          ((msg->wParam == VK_RETURN && ctrl) ||
           (h == self.search && msg->wParam == VK_ESCAPE && GetWindowTextLengthW(h))))
        return DLGC_WANTMESSAGE;
    }
    if (message == WM_KEYDOWN && ctrl) {
      if (w == 'F') {
        SetFocus(self.search);
        SendMessageW(self.search, EM_SETSEL, 0, -1);
        return 0;
      }
      if (w == VK_RETURN) {
        SendMessageW(dialog, WM_COMMAND, IDOK, 0);
        return 0;
      }
    }
    if (message == WM_CHAR && (w == 6 || (w == VK_RETURN && ctrl)))
      return 0;
    if (h == self.search) {
      if (message == WM_PAINT && !GetWindowTextLengthW(h)) {
        const auto result = DefSubclassProc(h, message, w, l);
        self.drawSearchCue(h);
        return result;
      }
      if (message == WM_SETFOCUS || message == WM_KILLFOCUS)
        InvalidateRect(dialog, &self.searchFrame, FALSE);
      if (message == WM_KEYDOWN && w == VK_ESCAPE && GetWindowTextLengthW(h)) {
        SetWindowTextW(h, L"");
        return 0;
      }
      if (message == WM_KEYDOWN && w == VK_DOWN && !self.treeRows.empty()) {
        self.navigate(VK_HOME);
        SetFocus(self.tree);
        return 0;
      }
      return DefSubclassProc(h, message, w, l);
    }
    if (h == self.tree) {
      if (message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK)
        self.clickPoint = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
      if (message == WM_VSCROLL || message == WM_MOUSEWHEEL) {
        self.hoveredRow = nullptr;
        self.hoveredPart = Part::None;
        InvalidateRect(h, nullptr, FALSE);
      }
      if (message == WM_KEYDOWN && self.navigate(static_cast<UINT>(w)))
        return 0;
      if (message == WM_SYSKEYDOWN && (w == VK_UP || w == VK_DOWN) &&
          (GetKeyState(VK_MENU) & 0x8000)) {
        self.move(w == VK_UP);
        return 0;
      }
    } else {
      if (message == WM_GETDLGCODE)
        return DLGC_BUTTON | (GetDlgCtrlID(h) == IDOK ? DLGC_DEFPUSHBUTTON
                                                     : DLGC_UNDEFPUSHBUTTON);
      // Preserve owner draw when the dialog manager changes the default style.
      if (message == BM_SETSTYLE && (w & BS_TYPEMASK) != BS_OWNERDRAW) {
        InvalidateRect(h, nullptr, FALSE);
        return 0;
      }
    }
    return DefSubclassProc(h, message, w, l);
  }
  void drawButton(const DRAWITEMSTRUCT &draw) const {
    const auto &c = *colors;
    const HDC dc = draw.hDC;
    const int saved = SaveDC(dc);
    const bool primary = draw.CtlID == IDOK;
    const bool reset = draw.CtlID == Reset;
    const bool ghost = reset || draw.CtlID == ExpandAll || draw.CtlID == CollapseAll;
    const bool disabled = (draw.itemState & ODS_DISABLED) != 0;
    const bool hot = !disabled && (draw.hwndItem == hoveredButton || (draw.itemState & ODS_SELECTED));
    const auto ink = disabled ? c.disabled : primary ? c.accentInk
                              : ghost && !hot ? c.secondary : c.text;
    const auto fill = primary ? (disabled ? c.switchOff : hot ? c.accentHover : c.accent)
                            : hot ? c.hover : ghost ? c.surface : c.field;
    solid(dc, draw.rcItem, c.surface);
    RECT face = draw.rcItem;
    if (draw.itemState & ODS_FOCUS) {
      rounded(dc, face, 6, c.accent);
      InflateRect(&face, -px(2), -px(2));
      rounded(dc, face, 4, c.surface);
      InflateRect(&face, -px(1), -px(1));
      rounded(dc, face, 3, fill);
    } else {
      rounded(dc, face, 6, fill, primary || ghost ? CLR_INVALID : c.strongBorder);
    }
    auto text = draw.rcItem;
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, primary ? rootFont : bodyFont);
    if (reset) {
      const int x = text.left + px(18), y = (text.top + text.bottom) / 2;
      const auto pen = CreatePen(PS_SOLID, px(1), ink);
      const auto oldPen = SelectObject(dc, pen);
      Arc(dc, x - px(5), y - px(5), x + px(6), y + px(6),
          x - px(5), y + px(1), x - px(4), y - px(3));
      const POINT tip[] = {{-5, -6}, {-5, -2}, {-1, -2}};
      stroke(dc, x, y, ink, tip);
      SelectObject(dc, oldPen);
      DeleteObject(pen);
      text.left += px(32);
    }
    wchar_t title[64]{};
    GetWindowTextW(draw.hwndItem, title, static_cast<int>(std::size(title)));
    textLine(dc, title, text, ink, reset ? DT_LEFT : DT_CENTER);
    RestoreDC(dc, saved);
  }
  void paint(HWND h) const {
    PAINTSTRUCT ps{};
    const HDC dc = BeginPaint(h, &ps);
    RECT client{};
    GetClientRect(h, &client);
    FillRect(dc, &ps.rcPaint, surfaceBrush);
    paintSearch(dc);
    rounded(dc, panel, 6, colors->sunken, colors->border);
    const int footer = client.bottom - px(56);
    solid(dc, {0, footer, client.right, footer + px(1)}, colors->border);
    const auto info = centered(px(18), panel.bottom + px(19), 12, 12);
    rounded(dc, info, 6, colors->surface, colors->secondary);
    const int x = (info.left + info.right) / 2, y = (info.top + info.bottom) / 2;
    solid(dc, {x, y - px(3), x + px(1), y - px(2)}, colors->secondary);
    solid(dc, {x, y, x + px(1), y + px(3)}, colors->secondary);
    EndPaint(h, &ps);
  }
  INT_PTR notify(HWND h, const NMHDR &header) {
    if (header.hwndFrom != tree)
      return FALSE;
    switch (header.code) {
    case NM_CUSTOMDRAW: {
      const auto &draw = reinterpret_cast<const NMTVCUSTOMDRAW &>(header);
      LRESULT result = CDRF_DODEFAULT;
      if (draw.nmcd.dwDrawStage == CDDS_PREPAINT) {
        result = CDRF_NOTIFYITEMDRAW;
      } else if (draw.nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        drawRow(draw);
        result = CDRF_SKIPDEFAULT;
      }
      SetWindowLongPtrW(h, DWLP_MSGRESULT, result);
      return TRUE;
    }
    case TVN_SELCHANGINGW:
      if (reinterpret_cast<const NMTREEVIEWW &>(header).itemNew.lParam) {
        SetWindowLongPtrW(h, DWLP_MSGRESULT, TRUE);
        return TRUE;
      }
      break;
    case TVN_SELCHANGEDW:
      InvalidateRect(preview, nullptr, FALSE);
      [[fallthrough]];
    case NM_SETFOCUS:
    case NM_KILLFOCUS:
      InvalidateRect(tree, nullptr, FALSE);
      break;
    case TVN_KEYDOWN:
      if (reinterpret_cast<const NMTVKEYDOWN &>(header).wVKey == VK_SPACE)
        toggle(TreeView_GetSelection(tree));
      break;
    case NM_CLICK: {
      Part part{};
      const auto node = hit(clickPoint, part);
      if (part == Part::Visibility)
        toggle(node);
      else if (part == Part::Chevron)
        TreeView_Expand(tree, node, TVE_TOGGLE);
      else if (part == Part::Up || part == Part::Down)
        PostMessageW(h, MoveRow, part == Part::Up, reinterpret_cast<LPARAM>(node));
      break;
    }
    }
    return FALSE;
  }
  static INT_PTR CALLBACK dialogProc(HWND h, UINT message, WPARAM w, LPARAM l) {
    auto self = reinterpret_cast<NoteQuMenu *>(GetWindowLongPtrW(h, DWLP_USER));
    if (message == WM_INITDIALOG) {
      self = reinterpret_cast<NoteQuMenu *>(l);
      SetWindowLongPtrW(h, DWLP_USER, l);
      self->createControls(h);
      return TRUE;
    }
    if (!self)
      return FALSE;
    switch (message) {
    case WM_CTLCOLORDLG:
      return reinterpret_cast<INT_PTR>(self->surfaceBrush);
    case WM_CTLCOLOREDIT:
      SetTextColor(reinterpret_cast<HDC>(w), self->colors->text);
      SetBkColor(reinterpret_cast<HDC>(w), self->colors->field);
      return reinterpret_cast<INT_PTR>(self->fieldBrush);
    case WM_CTLCOLORSTATIC:
      if (reinterpret_cast<HWND>(l) == self->empty) {
        SetBkColor(reinterpret_cast<HDC>(w), self->colors->sunken);
        SetTextColor(reinterpret_cast<HDC>(w), self->colors->secondary);
        return reinterpret_cast<INT_PTR>(self->sunkenBrush);
      }
      SetBkMode(reinterpret_cast<HDC>(w), TRANSPARENT);
      SetTextColor(reinterpret_cast<HDC>(w), self->colors->secondary);
      return reinterpret_cast<INT_PTR>(self->surfaceBrush);
    case WM_PAINT:
      self->paint(h);
      return TRUE;
    case WM_DRAWITEM: {
      const auto &draw = *reinterpret_cast<const DRAWITEMSTRUCT *>(l);
      if (draw.CtlID == Preview)
        self->drawPreview(draw);
      else if (draw.CtlID == Counter)
        self->drawCounter(draw);
      else
        self->drawButton(draw);
      return TRUE;
    }
    case PaintInitialTree:
      // Keep the section heading above the selected menu on first open.
      TreeView_SelectSetFirstVisible(self->tree, TreeView_GetRoot(self->tree));
      RedrawWindow(self->tree, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
      return TRUE;
    case MoveRow:
      if (self->treeRows.count(reinterpret_cast<HTREEITEM>(l))) {
        TreeView_SelectItem(self->tree, reinterpret_cast<HTREEITEM>(l));
        self->move(w != 0);
      }
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(w)) {
      case Search:
        if (HIWORD(w) == EN_CHANGE && self->search == reinterpret_cast<HWND>(l))
          self->searchChanged();
        return TRUE;
      case ExpandAll:
      case CollapseAll:
        self->expandAll(LOWORD(w) == ExpandAll);
        return TRUE;
      case Reset:
        self->rememberExpansion();
        self->draft = {};
        self->updateStatus();
        self->rebuildTree(self->selectedKey());
        return TRUE;
      case IDOK:
        if (!IsWindowEnabled(self->apply))
          return TRUE;
        // Visibility is updated directly in the draft, including collapsed rows.
        if (!self->save(self->draft)) {
          MessageBoxW(h,
                      L"Men\u00fc ayarlar\u0131 kaydedilemedi. Ayar "
                      L"klas\u00f6r\u00fcn\u00fcn yaz\u0131labilir "
                      L"oldu\u011funu kontrol edin.",
                      L"NoteQu", MB_OK | MB_ICONERROR);
          return TRUE;
        }
        self->settings = self->draft;
        self->refresh();
        EndDialog(h, IDOK);
        return TRUE;
      case IDCANCEL:
        EndDialog(h, IDCANCEL);
        return TRUE;
      }
      break;
    case WM_NOTIFY:
      return self->notify(h, *reinterpret_cast<const NMHDR *>(l));
    case WM_CLOSE:
      EndDialog(h, IDCANCEL);
      return TRUE;
    }
    return FALSE;
  }
  void sizeDialog(HWND h) const {
    RECT unit{0, 0, 4, 8}, frame{}, client{}, parent{};
    MapDialogRect(h, &unit);
    GetWindowRect(h, &frame);
    GetClientRect(h, &client);
    GetWindowRect(owner, &parent);
    MONITORINFO monitor{sizeof(MONITORINFO)};
    GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor);
    const auto &work = monitor.rcWork;
    const int frameWidth = frame.right - frame.left - client.right;
    const int frameHeight = frame.bottom - frame.top - client.bottom;
    const int width = (std::min)(px(520), static_cast<int>(work.right - work.left));
    const int height = (std::min)(px(600), static_cast<int>(work.bottom - work.top));
    RECT dlus{0, 0, MulDiv(width - frameWidth, 4, unit.right),
                    MulDiv(height - frameHeight, 8, unit.bottom)};
    MapDialogRect(h, &dlus);
    const LONG w = dlus.right + frameWidth, ht = dlus.bottom + frameHeight;
    const LONG x = std::clamp((parent.left + parent.right - w) / 2,
                              work.left, (std::max)(work.left, work.right - w));
    const LONG y = std::clamp((parent.top + parent.bottom - ht) / 2,
                              work.top, (std::max)(work.top, work.bottom - ht));
    SetWindowPos(h, nullptr, x, y, w, ht, SWP_NOZORDER | SWP_NOACTIVATE);
  }
  void createControls(HWND h) {
    SetWindowTextW(h, L"Men\u00fc \u00e7ubu\u011funu \u00f6zelle\u015ftir");
    dpi = static_cast<int>(windowDpi(h));
    sizeDialog(h);
    auto font = [&](int size, int weight, bool strike = false) {
      return CreateFontW(-px(size), 0, 0, 0, weight, FALSE, FALSE, strike,
                         DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    };
    bodyFont = font(13, FW_NORMAL);
    rootFont = font(13, FW_SEMIBOLD);
    bodyStrike = font(13, FW_NORMAL, true);
    rootStrike = font(13, FW_SEMIBOLD, true);
    smallFont = font(12, FW_NORMAL);
    captionFont = font(11, FW_SEMIBOLD);
    surfaceBrush = CreateSolidBrush(colors->surface);
    fieldBrush = CreateSolidBrush(colors->field);
    sunkenBrush = CreateSolidBrush(colors->sunken);
    auto add = [&](const wchar_t *cls, const wchar_t *text, DWORD style,
                   RECT r, int id, HFONT type) {
      const HWND child = CreateWindowExW(
          0, cls, text, WS_CHILD | WS_VISIBLE | style, r.left, r.top,
          r.right - r.left, r.bottom - r.top, h,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
          GetModuleHandleW(nullptr), nullptr);
      SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(type), FALSE);
      return child;
    };
    RECT client{};
    GetClientRect(h, &client);
    const int left = px(16), right = client.right - px(16);
    const int footer = client.bottom - px(56);
    auto subclass = [&](HWND child) {
      SetWindowSubclass(child, controlProc, 1, reinterpret_cast<DWORD_PTR>(this));
    };
    auto button = [&](int id, const wchar_t *title, RECT bounds) {
      const auto child = add(L"BUTTON", title, WS_TABSTOP | BS_OWNERDRAW,
                             bounds, id, bodyFont);
      subclass(child);
      return child;
    };

    add(L"STATIC", L"\u00d6N\u0130ZLEME", SS_NOPREFIX,
        {left, px(4), right, px(22)}, -1, captionFont);
    preview = add(L"STATIC", L"", SS_OWNERDRAW | SS_NOTIFY,
                  {left, px(26), right, px(56)}, Preview, bodyFont);
    subclass(preview);
    searchFrame = {left, px(66), right, px(98)};
    // Native edit accessibility uses the preceding static as its label.
    add(L"STATIC", searchCue, SS_NOPREFIX, {}, -1, bodyFont);
    search = add(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL,
                 {left + px(32), px(73), right - px(59), px(92)}, Search, bodyFont);
    subclass(search);
    SendMessageW(search, EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(searchCue));
    SendMessageW(search, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
    summary = add(L"STATIC", L"", SS_NOPREFIX | SS_CENTERIMAGE,
                  {left, px(103), right - px(158), px(130)}, Summary, smallFont);
    button(ExpandAll, L"T\u00fcm\u00fcn\u00fc a\u00e7",
           {right - px(158), px(103), right - px(82), px(130)});
    button(CollapseAll, L"T\u00fcm\u00fcn\u00fc kapat",
           {right - px(82), px(103), right, px(130)});

    panel = {left, px(139), right, footer - px(58)};
    RECT content = panel;
    InflateRect(&content, -px(5), -px(5));
    tree = add(WC_TREEVIEWW, L"Men\u00fcler ve komutlar",
               WS_TABSTOP | TVS_FULLROWSELECT | TVS_NOHSCROLL |
                   TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_NONEVENHEIGHT,
               content, Tree, bodyFont);
    TreeView_SetExtendedStyle(tree, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
    TreeView_SetBkColor(tree, colors->sunken);
    TreeView_SetTextColor(tree, colors->text);
    TreeView_SetIndent(tree, px(22));
    TreeView_SetItemHeight(tree, px(30));
    subclass(tree);
    empty = add(L"STATIC", L"", SS_CENTER | SS_NOPREFIX | SS_CENTERIMAGE,
                {left + px(10), panel.top + px(18), right - px(10), panel.top + px(50)},
                Empty, bodyFont);
    ShowWindow(empty, SW_HIDE);
    add(L"STATIC",
        L"Gizlenen komutlar\u0131n k\u0131sayollar\u0131 \u00e7al\u0131\u015fmaya devam eder. "
        L"S\u0131ray\u0131 Alt+\u2191 / Alt+\u2193 ile de\u011fi\u015ftirin.",
        SS_NOPREFIX, {px(40), panel.bottom + px(11), right, footer - px(8)},
        Note, smallFont);
    const int y = footer + px(12), bottom = y + px(32);
    button(Reset, L"Varsay\u0131lana s\u0131f\u0131rla",
           {left, y, left + px(158), bottom});
    counter = add(L"STATIC", L"", SS_OWNERDRAW,
                  {left + px(158), y, right - px(194), bottom}, Counter, smallFont);
    button(IDCANCEL, L"Vazge\u00e7", {right - px(184), y, right - px(96), bottom});
    apply = button(IDOK, L"Uygula", {right - px(88), y, right, bottom});
    SendMessageW(h, DM_SETDEFID, IDOK, 0);
    if (themeDialog)
      themeDialog(h, tree);

    for (int i = 0, count = GetMenuItemCount(source); i < count; ++i) {
      const Item item(source, i);
      if (!(item.info.fType & MFT_SEPARATOR) && !transient(item.info.wID))
        entries.emplace(i, readEntry(item, i, "", 0));
    }
    const auto groups = sections(draft);
    if (!groups[0].empty())
      expanded.insert(entries.at(groups[0].front()).row.key);
    updateStatus();
    rebuildTree({}, true);
    PostMessageW(h, PaintInitialTree, 0, 0);
  }

public:
  const Settings &getSettings() const { return settings; }
  HMENU handle() const { return display ? display : source; }
  HMENU canonical(HMENU menu) const {
    auto i = origins.find(menu);
    return i == origins.end() ? menu : i->second.first;
  }
  void popup(HMENU menu) {
    auto i = origins.find(menu);
    if (i != origins.end()) {
      const auto original = i->second;
      fill(menu, original.first, original.second);
    }
  }
  void init(HWND window, HMENU menu, const std::filesystem::path &file) {
    owner = window;
    source = menu;
    path = file;
    std::error_code migrationError;
    if (path.filename() == L"NoteX-menu.cfg" && !std::filesystem::exists(path, migrationError) && !migrationError) {
      const auto previous = path.parent_path() / (NoteQu::previousBrand() + L"-menu.cfg");
      MoveFileExW(previous.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH);
    }
    load();
    display = CreateMenu();
    refresh();
    if (GetMenu(owner))
      SetMenu(owner, display);
    DrawMenuBar(owner);
  }
  void refresh() {
    if (!display)
      return;
    fill(display, source, "");
    AppendMenuW(display, MF_STRING | MF_RIGHTJUSTIFY, Customize, L"\u2699");
    DrawMenuBar(owner);
  }
  void destroy() {
    if (owner && GetMenu(owner) == display)
      SetMenu(owner, source);
    // Detach first: a popup can have become orphaned after canonical menu
    // changes.
    for (const auto &[original, copy] : copies) {
      (void)original;
      while (GetMenuItemCount(copy) > 0)
        RemoveMenu(copy, 0, MF_BYPOSITION);
    }
    if (display) {
      while (GetMenuItemCount(display) > 0)
        RemoveMenu(display, 0, MF_BYPOSITION);
      DestroyMenu(display);
    }
    for (const auto &[original, copy] : copies) {
      (void)original;
      DestroyMenu(copy);
    }
    copies.clear();
    origins.clear();
    display = nullptr;
  }
  bool load() {
    settings = {};
    std::error_code error;
    if (!std::filesystem::exists(path, error))
      return true;
    if (std::filesystem::file_size(path, error) > 512 * 1024 || error)
      return false;
    std::ifstream in(path);
    std::string line;
    if (!std::getline(in, line) || (line != "NOTEXMENU1" && line != "NPPMENU1"))
      return false;
    Settings loaded;
    while (std::getline(in, line)) {
      if (line.size() < 3 || line[1] != ' ' || !validKey(line.substr(2)))
        return false;
      if (line[0] == 'H')
        loaded.hidden.insert(line.substr(2));
      else if (line[0] == 'O' && line.find('/') == std::string::npos) {
        if (std::find(loaded.order.begin(), loaded.order.end(),
                      line.substr(2)) == loaded.order.end())
          loaded.order.push_back(line.substr(2));
      } else
        return false;
    }
    if (!in.eof())
      return false;
    settings = std::move(loaded);
    return true;
  }
  bool save(const Settings &value) const {
    auto temporary = path;
    temporary += L".tmp-" + std::to_wstring(GetCurrentProcessId());
    std::ofstream out(temporary, std::ios::trunc);
    out << "NOTEXMENU1\n";
    for (const auto &k : value.order)
      out << "O " << k << '\n';
    for (const auto &k : value.hidden)
      out << "H " << k << '\n';
    out.close();
    if (!out ||
        !MoveFileExW(temporary.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
      DeleteFileW(temporary.c_str());
      return false;
    }
    return true;
  }
  void customize(bool dark, void (*theme)(HWND, HWND), UINT (*getDpi)(HWND)) {
    draft = settings;
    colors = dark ? &darkColors : &lightColors;
    themeDialog = theme;
    windowDpi = getDpi;
    // Empty dialog template; native controls are created at WM_INITDIALOG.
    struct Template {
      DLGTEMPLATE dialog;
      WORD menu, windowClass, title;
    } t{};
    t.dialog.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | DS_MODALFRAME;
    t.dialog.dwExtendedStyle = WS_EX_CONTROLPARENT;
    // WM_INITDIALOG measures the actual dialog units and sizes the window.
    t.dialog.cx = 1;
    t.dialog.cy = 1;
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), &t.dialog, owner,
                            dialogProc, reinterpret_cast<LPARAM>(this));
    for (auto font : {bodyFont, rootFont, bodyStrike, rootStrike, smallFont, captionFont})
      if (font)
        DeleteObject(font);
    bodyFont = rootFont = bodyStrike = rootStrike = smallFont = captionFont = nullptr;
    for (auto brush : {surfaceBrush, fieldBrush, sunkenBrush})
      if (brush)
        DeleteObject(brush);
    surfaceBrush = fieldBrush = sunkenBrush = nullptr;
    tree = hoveredButton = preview = search = summary = counter = empty = apply = nullptr;
    hoveredRow = nullptr;
    hoveredPreview = -1;
    treeRows.clear();
    entries.clear();
    hiddenBelow.clear();
    expanded.clear();
    collapsedResults.clear();
    previewItems.clear();
    query.clear();
    beforeSearch.clear();
  }
};
