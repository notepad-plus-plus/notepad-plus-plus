// Copyright (C) 2026 Berk Kivircik.
// SPDX-License-Identifier: GPL-3.0-only
// Created for NoteX; publication date: 2026-10-28.

#include "NoteXMenu.h"
#include <iostream>
static int passed = 0, failed = 0;
static void check(bool ok, const char *name) {
  std::cout << (ok ? "PASS " : "FAIL ") << name << '\n';
  ok ? ++passed : ++failed;
}
static std::wstring text(HMENU m, int pos) {
  wchar_t buffer[256]{};
  GetMenuStringW(m, pos, buffer, 256, MF_BYPOSITION);
  return buffer;
}
int main() {
  auto dir = std::filesystem::temp_directory_path() /
             (L"NoteX-menu-tests-" + std::to_wstring(GetCurrentProcessId()));
  std::filesystem::create_directory(dir);
  auto cfg = dir / L"menu.cfg";
  HWND window = CreateWindowW(L"STATIC", L"Menu test", WS_OVERLAPPEDWINDOW, 0,
                              0, 400, 400, nullptr, nullptr, nullptr, nullptr);
  HMENU source = CreateMenu(), file = CreatePopupMenu(),
        edit = CreatePopupMenu(), plugin = CreatePopupMenu();
  AppendMenuW(file, MF_STRING, 41001, L"New\tCtrl+N");
  AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(file, MF_STRING, 41002, L"Open");
  AppendMenuW(edit, MF_STRING | MF_GRAYED, 42001, L"Undo");
  AppendMenuW(edit, MF_STRING | MF_CHECKED, 42002, L"Checked");
  AppendMenuW(plugin, MF_STRING, 22001, L"Plugin command");
  AppendMenuW(source, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"&File");
  AppendMenuW(source, MF_POPUP, reinterpret_cast<UINT_PTR>(edit), L"&Edit");
  AppendMenuW(source, MF_POPUP, reinterpret_cast<UINT_PTR>(plugin),
              L"&Plugins");
  SetMenu(window, source);
  NoteXMenu menu;
  cfg = dir / L"NoteX-menu.cfg";
  const auto previousConfig = dir / (NoteX::previousBrand() + L"-menu.cfg");
  { std::ofstream previous(previousConfig); previous << "NPPMENU1\nO r1\nO r0\nO r2\n"; }
  menu.init(window, source, cfg);
  check(std::filesystem::exists(cfg) && !std::filesystem::exists(previousConfig) &&
            menu.getSettings().order == std::vector<std::string>{"r1", "r0", "r2"},
        "previous menu filename and settings migrate without reset");
  menu.save(NoteXMenu::Settings{});
  menu.load();
  menu.refresh();
  auto display = menu.handle();
  check(GetMenu(window) == display && display != source,
        "display copy attached");
  check(GetMenuItemCount(display) == 4 &&
            GetMenuItemID(display, 3) == NoteXMenu::Customize,
        "customizer always last");
  auto fileCopy = GetSubMenu(display, 0);
  check(fileCopy != file && menu.canonical(fileCopy) == file,
        "canonical popup mapping");
  check(GetMenuState(GetSubMenu(display, 1), 42001, MF_BYCOMMAND) & MF_GRAYED,
        "disabled state copied");
  check(GetMenuState(GetSubMenu(display, 1), 42002, MF_BYCOMMAND) & MF_CHECKED,
        "checked state copied");
  NoteXMenu::Settings custom{{"r1", "r0", "r2"}, {"r0/c41001", "r2"}};
  check(menu.save(custom) && menu.load() && menu.getSettings() == custom,
        "settings round trip");
  menu.refresh();
  check(text(display, 0) == L"&Edit" && text(display, 1) == L"&File",
        "top menu reorder");
  check(GetMenuItemCount(display) == 3, "hide top menu");
  check(GetMenuItemCount(GetSubMenu(display, 1)) == 1 &&
            GetMenuItemID(GetSubMenu(display, 1), 0) == 41002,
        "hide child and trim separator");
  check(GetMenuItemCount(source) == 3 && GetMenuItemCount(file) == 3 &&
            GetSubMenu(source, 0) == file,
        "canonical menu unchanged");
  check(GetSubMenu(display, 1) == fileCopy,
        "popup handles retained across refresh");
  EnableMenuItem(edit, 42001, MF_BYCOMMAND | MF_ENABLED);
  CheckMenuItem(edit, 42002, MF_BYCOMMAND | MF_UNCHECKED);
  menu.popup(GetSubMenu(display, 0));
  check(
      !(GetMenuState(GetSubMenu(display, 0), 42001, MF_BYCOMMAND) & MF_GRAYED),
      "live enabled state refreshed");
  check(
      !(GetMenuState(GetSubMenu(display, 0), 42002, MF_BYCOMMAND) & MF_CHECKED),
      "live check state refreshed");
  AppendMenuW(file, MF_STRING, 41003, L"New dynamic command");
  menu.popup(fileCopy);
  check(GetMenuItemID(fileCopy, 1) == 41003, "new canonical command appears");
  custom.hidden = {"r0", "r1", "r2"};
  menu.save(custom);
  menu.load();
  menu.refresh();
  check(GetMenuItemCount(display) == 1 &&
            GetMenuItemID(display, 0) == NoteXMenu::Customize,
        "all menus hidden retains recovery icon");
  check(menu.save({}) && menu.load(), "reset persists");
  menu.refresh();
  check(text(display, 0) == L"&File" && GetMenuItemCount(display) == 4 &&
            GetMenuItemCount(fileCopy) == 4,
        "reset restores order and children");
  // Names persist for plugin commands even when their allocated IDs change.
  custom = {};
  custom.hidden.insert(
      "r2/n0050006c007500670069006e00200063006f006d006d0061006e0064");
  menu.save(custom);
  menu.load();
  menu.refresh();
  check(GetMenuItemCount(GetSubMenu(display, 2)) == 0,
        "plugin command hidden by name");
  ModifyMenuW(plugin, 0, MF_BYPOSITION | MF_STRING, 22008, L"Plugin command");
  menu.refresh();
  check(GetMenuItemCount(GetSubMenu(display, 2)) == 0,
        "plugin ID reallocation retains identity");
  {
    std::ofstream out(cfg);
    out << "NPPMENU1\nH r0\nBAD\n";
  }
  check(!menu.load() && menu.getSettings() == NoteXMenu::Settings{},
        "malformed settings fail to defaults atomically");
  menu.refresh();
  menu.destroy();
  check(IsMenu(source) && IsMenu(file) && GetMenu(window) == source,
        "destroy preserves canonical ownership");
  SetMenu(window, nullptr);
  DestroyMenu(source);
  DestroyWindow(window);
  std::filesystem::remove(cfg);
  std::filesystem::remove(dir);
  std::cout << "TOTAL " << passed << " passed, " << failed << " failed\n";
  return failed ? 1 : 0;
}
