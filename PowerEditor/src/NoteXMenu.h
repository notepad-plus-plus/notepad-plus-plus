// Copyright (C) 2026 Berk Kivircik.
// SPDX-License-Identifier: GPL-3.0-only
// Created for NoteX; publication date: 2026-10-28.

#pragma once
// clang-format off
#include <windows.h>
#include "NoteXFormat.h"
#include <commctrl.h>
// clang-format on
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

// The application's canonical menu retains its positions and handles for
// plugins, translations and native command updates. Only this owned display
// copy is filtered.
class NoteXMenu {
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
  HWND tree = nullptr;
  HFONT dialogFont = nullptr;
  std::map<HTREEITEM, std::string> treeKeys;
  void (*themeDialog)(HWND) = nullptr;
  INT_PTR (*colorDialog)(UINT, WPARAM) = nullptr;
  enum { Tree = 100, Up, Down, Reset };

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
  void harvest() {
    for (const auto &[node, k] : treeKeys) {
      if (TreeView_GetCheckState(tree, node))
        draft.hidden.erase(k);
      else
        draft.hidden.insert(k);
    }
  }
  void populate(HMENU menu, HTREEITEM parent, const std::string &prefix) {
    for (int i : positions(menu, prefix, draft)) {
      Item item(menu, i);
      if ((item.info.fType & MFT_SEPARATOR) || transient(item.info.wID))
        continue;
      auto text = label(item.text);
      auto k = key(item, prefix, i);
      TVINSERTSTRUCTW ins{};
      ins.hParent = parent;
      ins.hInsertAfter = TVI_LAST;
      ins.item.mask = TVIF_TEXT | TVIF_STATE;
      ins.item.pszText = text.data();
      ins.item.stateMask = TVIS_STATEIMAGEMASK;
      ins.item.state = INDEXTOSTATEIMAGEMASK(draft.hidden.count(k) ? 1 : 2);
      HTREEITEM node = TreeView_InsertItem(tree, &ins);
      treeKeys[node] = k;
      if (item.info.hSubMenu)
        populate(item.info.hSubMenu, node, k);
    }
  }
  void rebuildTree(const std::string &select = {}) {
    SendMessageW(tree, WM_SETREDRAW, FALSE, 0);
    TreeView_DeleteAllItems(tree);
    treeKeys.clear();
    populate(source, TVI_ROOT, "");
    for (const auto &[node, k] : treeKeys)
      if (k == select)
        TreeView_SelectItem(tree, node);
    SendMessageW(tree, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(tree, nullptr, TRUE);
  }
  void buttons(HWND h) {
    auto selected = TreeView_GetSelection(tree);
    bool root = selected && !TreeView_GetParent(tree, selected);
    EnableWindow(GetDlgItem(h, Up),
                 root && TreeView_GetPrevSibling(tree, selected));
    EnableWindow(GetDlgItem(h, Down),
                 root && TreeView_GetNextSibling(tree, selected));
  }
  void move(HWND h, bool up) {
    auto selected = TreeView_GetSelection(tree);
    if (!selected || TreeView_GetParent(tree, selected))
      return;
    auto other = up ? TreeView_GetPrevSibling(tree, selected)
                    : TreeView_GetNextSibling(tree, selected);
    if (!other)
      return;
    harvest();
    const auto k = treeKeys.at(selected);
    draft.order.clear();
    for (auto node = TreeView_GetRoot(tree); node;
         node = TreeView_GetNextSibling(tree, node))
      draft.order.push_back(treeKeys.at(node));
    auto a = std::find(draft.order.begin(), draft.order.end(), k);
    auto b =
        std::find(draft.order.begin(), draft.order.end(), treeKeys.at(other));
    std::iter_swap(a, b);
    rebuildTree(k);
    buttons(h);
    SetFocus(tree);
  }
  static INT_PTR CALLBACK dialogProc(HWND h, UINT message, WPARAM w, LPARAM l) {
    auto self = reinterpret_cast<NoteXMenu *>(GetWindowLongPtrW(h, DWLP_USER));
    if (message == WM_INITDIALOG) {
      self = reinterpret_cast<NoteXMenu *>(l);
      SetWindowLongPtrW(h, DWLP_USER, l);
      self->createControls(h);
      return TRUE;
    }
    if (!self)
      return FALSE;
    if (self->colorDialog) {
      if (auto result = self->colorDialog(message, w))
        return result;
    }
    switch (message) {
    case WM_COMMAND:
      switch (LOWORD(w)) {
      case Up:
        self->move(h, true);
        return TRUE;
      case Down:
        self->move(h, false);
        return TRUE;
      case Reset:
        self->draft = {};
        self->rebuildTree();
        self->buttons(h);
        return TRUE;
      case IDOK:
        self->harvest();
        if (!self->save(self->draft)) {
          MessageBoxW(h,
                      L"Men\u00fc ayarlar\u0131 kaydedilemedi. Ayar "
                      L"klas\u00f6r\u00fcn\u00fcn yaz\u0131labilir "
                      L"oldu\u011funu kontrol edin.",
                      L"NoteX", MB_OK | MB_ICONERROR);
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
      if (reinterpret_cast<NMHDR *>(l)->code == TVN_SELCHANGEDW)
        self->buttons(h);
      break;
    case WM_CLOSE:
      EndDialog(h, IDCANCEL);
      return TRUE;
    }
    return FALSE;
  }
  void createControls(HWND h) {
    SetWindowTextW(h, L"Men\u00fc \u00e7ubu\u011funu \u00f6zelle\u015ftir");
    HDC dc = GetDC(h);
    const int dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(h, dc);
    dialogFont = CreateFontW(-MulDiv(10, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE,
                             FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
                             CLEARTYPE_QUALITY, 0, L"Segoe UI");
    auto add = [&](const wchar_t *cls, const wchar_t *text, DWORD style, int x,
                   int y, int width, int height, int id) {
      RECT rect{x, y, x + width, y + height};
      MapDialogRect(h, &rect);
      HWND child = CreateWindowExW(
          wcscmp(cls, WC_TREEVIEWW) == 0 ? WS_EX_CLIENTEDGE : 0, cls, text,
          WS_CHILD | WS_VISIBLE | style, rect.left, rect.top,
          rect.right - rect.left, rect.bottom - rect.top, h,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
          GetModuleHandleW(nullptr), nullptr);
      SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(dialogFont),
                   TRUE);
      return child;
    };
    add(L"STATIC",
        L"G\u00f6r\u00fcnecek men\u00fc ve komutlar\u0131 i\u015faretleyin. "
        L"Alt \u00f6\u011feler i\u00e7in men\u00fcy\u00fc geni\u015fletin.",
        0, 12, 10, 408, 22, -1);
    tree = add(WC_TREEVIEWW, L"Men\u00fcler ve komutlar",
               WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
                   TVS_CHECKBOXES | TVS_SHOWSELALWAYS,
               12, 36, 310, 254, Tree);
    add(L"BUTTON", L"Yukar\u0131 ta\u015f\u0131", WS_TABSTOP, 332, 36, 86, 22,
        Up);
    add(L"BUTTON", L"A\u015fa\u011f\u0131 ta\u015f\u0131", WS_TABSTOP, 332, 64,
        86, 22, Down);
    add(L"STATIC",
        L"Ta\u015f\u0131ma d\u00fc\u011fmeleri ana men\u00fclerin "
        L"s\u0131ras\u0131n\u0131 de\u011fi\u015ftirir.\n\nGizlenen "
        L"komutlar\u0131n klavye k\u0131sayollar\u0131 "
        L"\u00e7al\u0131\u015fmaya devam eder.",
        0, 332, 102, 86, 106, -1);
    add(L"BUTTON", L"Varsay\u0131lana s\u0131f\u0131rla", WS_TABSTOP, 12, 304,
        112, 24, Reset);
    add(L"BUTTON", L"Uygula", WS_TABSTOP | BS_DEFPUSHBUTTON, 248, 304, 80, 24,
        IDOK);
    add(L"BUTTON", L"Vazge\u00e7", WS_TABSTOP, 338, 304, 80, 24, IDCANCEL);
    rebuildTree();
    buttons(h);
    if (themeDialog)
      themeDialog(h);
    // Centre on the owning window, clamped to the current monitor work area.
    RECT r{}, p{};
    GetWindowRect(h, &r);
    GetWindowRect(owner, &p);
    MONITORINFO monitor{sizeof(MONITORINFO)};
    GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST),
                    &monitor);
    int x = (std::max)(monitor.rcWork.left,
                       (std::min)((p.left + p.right - (r.right - r.left)) / 2,
                                  monitor.rcWork.right - (r.right - r.left)));
    int y = (std::max)(monitor.rcWork.top,
                       (std::min)((p.top + p.bottom - (r.bottom - r.top)) / 2,
                                  monitor.rcWork.bottom - (r.bottom - r.top)));
    SetWindowPos(h, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
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
      const auto previous = path.parent_path() / (NoteX::previousBrand() + L"-menu.cfg");
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
  void customize(void (*theme)(HWND),
                 INT_PTR (*colors)(UINT, WPARAM) = nullptr) {
    draft = settings;
    themeDialog = theme;
    colorDialog = colors;
    // Empty dialog template; native controls are created at WM_INITDIALOG.
    struct Template {
      DLGTEMPLATE dialog;
      WORD menu, windowClass, title;
    } t{};
    t.dialog.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
    t.dialog.dwExtendedStyle = WS_EX_CONTROLPARENT;
    t.dialog.cx = 430;
    t.dialog.cy = 340;
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), &t.dialog, owner,
                            dialogProc, reinterpret_cast<LPARAM>(this));
    if (dialogFont)
      DeleteObject(dialogFont);
    dialogFont = nullptr;
    tree = nullptr;
    treeKeys.clear();
  }
};
