// Copyright (C) 2026 Berk Kivircik.
// SPDX-License-Identifier: GPL-3.0-only
// Created for NoteX; publication date: 2026-10-28.

// Modified for NoteX: 2026-11-18; use the system UI font and avoid private Inter registration.

#pragma once
#include "NppDarkMode.h"
#include "NoteXFormat.h"
#include "ScintillaEditView.h"
#include "dpiManagerV2.h"
#include <cmath>
#include <commctrl.h>
#include <commdlg.h>
#include <set>
#include <string>
#include <vector>
#include <windows.h>

// A native toolbar: commands always target the active Scintilla document.
class NoteXTools {
  HWND window = nullptr, owner = nullptr, tooltip = nullptr;
  ScintillaEditView **view = nullptr;
  HFONT font = nullptr;
  HBRUSH background = nullptr;
  std::vector<HWND> controls;
  NoteX::SelectionFormat current;
  NoteX::Format painter;
  bool hasPainter = false, updating = false;
  int panelHeight = 40, scale = 96, hovered = 0, listState = 0;
  bool mixedList = false;
  enum {
    Family = 1,
    Size,
    Bold,
    Italic,
    Underline,
    Strike,
    Fore,
    Back,
    Highlight,
    Bullets,
    Numbers,
    CopyFormat,
    PasteFormat,
    Clear,
    Find
  };
  int px(int n) const { return MulDiv(n, scale, 96); }
  HWND editor() const { return view && *view ? (*view)->getHSelf() : nullptr; }
  LRESULT sci(UINT m, WPARAM w = 0, LPARAM l = 0) const {
    return SendMessage(editor(), m, w, l);
  }
  static std::string utf8(const std::wstring &w) {
    int n =
        WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                            nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                        s.data(), n, nullptr, nullptr);
    return s;
  }
  static std::wstring wide(const std::string &s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                        w.data(), n);
    return w;
  }
  static int CALLBACK enumFont(const LOGFONTW *lf, const TEXTMETRICW *, DWORD,
                               LPARAM data) {
    if (lf->lfFaceName[0] != L'@')
      reinterpret_cast<std::set<std::wstring> *>(data)->insert(lf->lfFaceName);
    return 1;
  }
  static UINT_PTR CALLBACK colorHook(HWND h, UINT m, WPARAM w, LPARAM) {
    if (m == WM_INITDIALOG) {
      NppDarkMode::setDarkTitleBar(h);
      NppDarkMode::autoSubclassAndThemeChildControls(h);
    }
    if (m == WM_CTLCOLORDLG)
      return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(w));
    if (m == WM_CTLCOLORSTATIC)
      return NppDarkMode::onCtlColorDlgStaticText(reinterpret_cast<HDC>(w),
                                                  true);
    if (m == WM_CTLCOLOREDIT)
      return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(w));
    return 0;
  }
  static LRESULT CALLBACK comboProc(HWND h, UINT m, WPARAM w, LPARAM l,
                                    UINT_PTR, DWORD_PTR data) {
    auto self = reinterpret_cast<NoteXTools *>(data);
    if (m == WM_KEYDOWN && w == VK_RETURN) {
      int parentID = GetDlgCtrlID(GetParent(h));
      self->command(parentID == Size || parentID == Family ? parentID
                                                           : GetDlgCtrlID(h));
      return 0;
    }
    if (m == WM_KEYDOWN && w == VK_ESCAPE) {
      SetFocus(self->editor());
      return 0;
    }
    return DefSubclassProc(h, m, w, l);
  }
  static LRESULT CALLBACK buttonProc(HWND h, UINT m, WPARAM w, LPARAM l,
                                     UINT_PTR, DWORD_PTR data) {
    auto self = reinterpret_cast<NoteXTools *>(data);
    // Owner-drawn buttons paint their complete background in draw(). Do not
    // expose the standard button's white erase between notification bursts.
    if (m == WM_ERASEBKGND)
      return TRUE;
    if (m == WM_MOUSEMOVE && self->hovered != GetDlgCtrlID(h)) {
      self->hovered = GetDlgCtrlID(h);
      TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, h, 0};
      TrackMouseEvent(&track);
      InvalidateRect(h, nullptr, FALSE);
    }
    if (m == WM_MOUSELEAVE) {
      self->hovered = 0;
      InvalidateRect(h, nullptr, FALSE);
    }
    return DefSubclassProc(h, m, w, l);
  }
  static LRESULT CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    auto self =
        reinterpret_cast<NoteXTools *>(GetWindowLongPtr(h, GWLP_USERDATA));
    if (m == WM_NCCREATE) {
      self = static_cast<NoteXTools *>(
          reinterpret_cast<CREATESTRUCT *>(l)->lpCreateParams);
      SetWindowLongPtr(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self)
      return DefWindowProc(h, m, w, l);
    switch (m) {
    case WM_ERASEBKGND: {
      RECT r;
      GetClientRect(h, &r);
      FillRect(reinterpret_cast<HDC>(w), &r, self->background);
      auto dc = reinterpret_cast<HDC>(w);
      auto pen = CreatePen(PS_SOLID, 1, RGB(38, 44, 50));
      auto old = SelectObject(dc, pen);
      MoveToEx(dc, 0, r.bottom - 1, nullptr);
      LineTo(dc, r.right, r.bottom - 1);
      SelectObject(dc, old);
      DeleteObject(pen);
      return 1;
    }
    case WM_PAINT: {
      PAINTSTRUCT paint{};
      auto dc = BeginPaint(h, &paint);
      auto pen = CreatePen(PS_SOLID, 1, RGB(50, 58, 66));
      auto old = SelectObject(dc, pen);
      for (int index : {2, 6, 9, 11})
        if (self->controls.size() > static_cast<size_t>(index)) {
          RECT current{}, previous{};
          GetWindowRect(self->controls[index], &current);
          GetWindowRect(self->controls[index - 1], &previous);
          if (current.top == previous.top) {
            POINT pt{current.left, current.top};
            ScreenToClient(h, &pt);
            MoveToEx(dc, pt.x - self->px(6), pt.y + self->px(5), nullptr);
            LineTo(dc, pt.x - self->px(6), pt.y + self->px(25));
          }
        }
      SelectObject(dc, old);
      DeleteObject(pen);
      EndPaint(h, &paint);
      return 0;
    }
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
      SetTextColor(reinterpret_cast<HDC>(w), RGB(232, 236, 239));
      SetBkColor(reinterpret_cast<HDC>(w), RGB(28, 33, 38));
      return reinterpret_cast<LRESULT>(self->background);
    case WM_COMMAND:
      if (!self->updating &&
          (HIWORD(w) == BN_CLICKED || HIWORD(w) == CBN_SELENDOK)) {
        if (HIWORD(w) == CBN_SELENDOK) {
          HWND combo = reinterpret_cast<HWND>(l);
          auto index = SendMessage(combo, CB_GETCURSEL, 0, 0);
          wchar_t value[256]{};
          if (index != CB_ERR) {
            SendMessageW(combo, CB_GETLBTEXT, index,
                         reinterpret_cast<LPARAM>(value));
            SetWindowTextW(combo, value);
          }
        }
        self->command(LOWORD(w));
      }
      return 0;
    case WM_DRAWITEM:
      self->draw(*reinterpret_cast<DRAWITEMSTRUCT *>(l));
      return TRUE;
    }
    return DefWindowProc(h, m, w, l);
  }
  unsigned field(int id) const {
    switch (id) {
    case Bold:
      return NoteX::Bold;
    case Italic:
      return NoteX::Italic;
    case Underline:
      return NoteX::Underline;
    case Strike:
      return NoteX::Strike;
    case Fore:
      return NoteX::Fore;
    case Back:
      return NoteX::Back;
    case Highlight:
      return NoteX::Highlight;
    default:
      return 0;
    }
  }
  bool checked(int id) const {
    if (current.mixed & field(id))
      return false;
    switch (id) {
    case Bold:
      return current.value.bold;
    case Italic:
      return current.value.italic;
    case Underline:
      return current.value.underline;
    case Strike:
      return current.value.strike;
    case CopyFormat:
      return hasPainter;
    case Bullets:
      return listState == 1;
    case Numbers:
      return listState == 2;
    default:
      return false;
    }
  }
  void draw(const DRAWITEMSTRUCT &d) {
    const int id = static_cast<int>(d.CtlID);
    const bool disabled = (d.itemState & ODS_DISABLED) != 0,
               active = checked(id);
    const COLORREF bg =
        !disabled && ((d.itemState & ODS_SELECTED) || hovered == id)
            ? RGB(42, 49, 55)
        : active ? RGB(34, 54, 74)
                 : RGB(28, 33, 38);
    HDC dc = d.hDC;
    const int saved = SaveDC(dc);
    FillRect(dc, &d.rcItem, background);
    auto brush = CreateSolidBrush(bg);
    SelectObject(dc, brush);
    SelectObject(dc, GetStockObject(NULL_PEN));
    RoundRect(dc, d.rcItem.left, d.rcItem.top, d.rcItem.right, d.rcItem.bottom,
              px(12), px(12));
    const COLORREF ink = disabled ? RGB(94, 103, 112)
                         : active ? RGB(90, 169, 240)
                                  : RGB(210, 217, 223);
    auto pen = CreatePen(PS_SOLID, std::max(1, px(2)), ink);
    SelectObject(dc, pen);
    SelectObject(dc, GetStockObject(NULL_BRUSH));
    const int side = px(18), ox = (d.rcItem.right - side) / 2,
              oy = (d.rcItem.bottom - side) / 2;
    auto X = [&](int x) { return ox + MulDiv(x, side, 24); };
    auto Y = [&](int y) { return oy + MulDiv(y, side, 24); };
    auto line = [&](int x, int y, int x2, int y2) {
      MoveToEx(dc, X(x), Y(y), nullptr);
      LineTo(dc, X(x2), Y(y2));
    };
    auto box = [&](int x, int y, int x2, int y2) {
      Rectangle(dc, X(x), Y(y), X(x2), Y(y2));
    };
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, ink);
    SelectObject(dc, font);
    if (id == Bold || id == Italic || id == Strike || id == Fore) {
      LOGFONTW lf{};
      GetObjectW(font, sizeof(lf), &lf);
      lf.lfHeight = -px(17);
      lf.lfWeight = id == Bold ? FW_BOLD : FW_MEDIUM;
      lf.lfItalic = id == Italic;
      auto glyph = CreateFontIndirectW(&lf);
      auto previous = SelectObject(dc, glyph);
      RECT r = d.rcItem;
      r.bottom -= id == Fore ? px(3) : 0;
      const wchar_t *text = id == Bold     ? L"B"
                            : id == Italic ? L"I"
                            : id == Strike ? L"S"
                                           : L"A";
      DrawTextW(dc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      SelectObject(dc, previous);
      DeleteObject(glyph);
      if (id == Strike)
        line(3, 12, 21, 12);
    } else if (id == Underline) {
      line(6, 3, 6, 12);
      line(18, 3, 18, 12);
      Arc(dc, X(6), Y(6), X(18), Y(19), X(6), Y(12), X(18), Y(12));
      line(4, 22, 20, 22);
    } else if (id == Back) {
      box(4, 4, 20, 18);
      line(8, 8, 16, 8);
      line(8, 12, 16, 12);
    } else if (id == Highlight) {
      POINT points[] = {{X(15), Y(2)}, {X(21), Y(8)}, {X(12), Y(17)},
                        {X(6), Y(17)}, {X(6), Y(11)}, {X(15), Y(2)}};
      Polyline(dc, points, 6);
      line(12, 5, 18, 11);
    } else if (id == Bullets || id == Numbers) {
      for (int row = 0; row < 3; ++row) {
        const int y = 5 + row * 7;
        line(9, y, 22, y);
        if (id == Bullets) {
          SelectObject(dc, GetStockObject(DC_BRUSH));
          SetDCBrushColor(dc, ink);
          Ellipse(dc, X(2), Y(y - 1), X(5) + 1, Y(y + 2));
          SelectObject(dc, GetStockObject(NULL_BRUSH));
        } else {
          RECT r{X(0), Y(y - 5), X(7), Y(y + 5)};
          wchar_t n[2] = {wchar_t(L'1' + row), 0};
          DrawTextW(dc, n, 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
      }
    } else if (id == CopyFormat || id == PasteFormat) {
      box(3, 3, 18, 9);
      line(18, 6, 22, 6);
      line(22, 6, 22, 13);
      line(22, 13, 12, 13);
      line(12, 13, 12, 22);
      if (id == CopyFormat) {
        line(3, 16, 7, 16);
        line(5, 14, 5, 18);
      }
    } else if (id == Clear) {
      line(4, 5, 20, 5);
      line(12, 5, 8, 21);
      line(5, 21, 11, 21);
      line(15, 16, 21, 22);
      line(21, 16, 15, 22);
    }
    unsigned bit = field(id);
    RECT mark = d.rcItem;
    mark.left += px(5);
    mark.right -= px(5);
    mark.top = mark.bottom - px(3);
    if ((bit && (current.mixed & bit)) ||
        (mixedList && (id == Bullets || id == Numbers))) {
      auto b = CreateSolidBrush(RGB(232, 163, 61));
      FillRect(dc, &mark, b);
      DeleteObject(b);
    } else if (id >= Fore && id <= Highlight) {
      auto color = id == Fore   ? current.value.fore
                   : id == Back ? current.value.back
                                : current.value.highlight;
      if (disabled)
        color = RGB(94, 103, 112);
      auto b = CreateSolidBrush(color);
      FillRect(dc, &mark, b);
      DeleteObject(b);
    }
    if (d.itemState & ODS_FOCUS) {
      RECT focus = d.rcItem;
      InflateRect(&focus, -px(2), -px(2));
      DrawFocusRect(dc, &focus);
    }
    RestoreDC(dc, saved);
    DeleteObject(pen);
    DeleteObject(brush);
  }
  void apply(const NoteX::Format &f, unsigned fields) {
    sci(SCI_BEGINUNDOACTION);
    auto count = sci(SCI_GETSELECTIONS);
    for (LRESULT i = 0; i < count; ++i) {
      NoteX::Change c;
      c.start = sci(SCI_GETSELECTIONNSTART, i);
      c.end = sci(SCI_GETSELECTIONNEND, i);
      c.value = f;
      c.fields = fields;
      sci(NoteX::ApplyFormat, 0, reinterpret_cast<LPARAM>(&c));
    }
    sci(SCI_ENDUNDOACTION);
  }
  void list(bool numbered) {
    if (sci(SCI_GETREADONLY))
      return;
    auto start = sci(SCI_GETSELECTIONSTART), end = sci(SCI_GETSELECTIONEND);
    auto first = sci(SCI_LINEFROMPOSITION, start),
         last = sci(SCI_LINEFROMPOSITION, end);
    if (last > first && end == sci(SCI_POSITIONFROMLINE, last))
      --last;
    struct Line {
      LRESULT pos;
      std::string prefix;
      bool same;
    };
    std::vector<Line> lines;
    bool remove = true;
    const std::string bullet =
        sci(SCI_GETCODEPAGE) == SC_CP_UTF8 ? "\xE2\x80\xA2 " : "* ";
    for (auto line = first; line <= last; ++line) {
      auto pos = sci(SCI_POSITIONFROMLINE, line);
      auto len = sci(SCI_LINELENGTH, line);
      std::string text(static_cast<size_t>(len) + 1, 0);
      sci(SCI_GETLINE, line, reinterpret_cast<LPARAM>(text.data()));
      size_t indent = text.find_first_not_of(" \t");
      if (indent == std::string::npos)
        indent = 0;
      size_t n = indent;
      while (n < text.size() && text[n] >= '0' && text[n] <= '9')
        ++n;
      bool num = n > indent && n + 1 < text.size() && text[n] == '.' &&
                 text[n + 1] == ' ';
      std::string prefix;
      if (text.compare(indent, bullet.size(), bullet) == 0)
        prefix = bullet;
      else if (num)
        prefix = text.substr(indent, n + 2 - indent);
      bool same = numbered ? num : prefix == bullet;
      remove &= same;
      lines.push_back({pos + static_cast<LRESULT>(indent), prefix, same});
    }
    sci(SCI_BEGINUNDOACTION);
    for (size_t i = lines.size(); i-- > 0;) {
      auto &line = lines[i];
      std::string prefix = remove     ? ""
                           : numbered ? std::to_string(i + 1) + ". "
                                      : bullet;
      sci(SCI_SETTARGETSTART, line.pos);
      sci(SCI_SETTARGETEND, line.pos + line.prefix.size());
      sci(SCI_REPLACETARGET, prefix.size(),
          reinterpret_cast<LPARAM>(prefix.c_str()));
    }
    sci(SCI_ENDUNDOACTION);
    sci(SCI_SETSEL, sci(SCI_POSITIONFROMLINE, first),
        sci(SCI_GETLINEENDPOSITION, last));
  }
  void command(int id) {
    if (id == Find && editor()) {
      wchar_t term[256]{};
      GetWindowTextW(controls[Find - 1], term, 256);
      auto value = utf8(term);
      if (value.empty())
        return;
      sci(SCI_GOTOPOS, sci(SCI_GETSELECTIONEND));
      sci(SCI_SEARCHANCHOR);
      auto found =
          sci(SCI_SEARCHNEXT, 0, reinterpret_cast<LPARAM>(value.c_str()));
      if (found < 0) {
        sci(SCI_GOTOPOS, 0);
        sci(SCI_SEARCHANCHOR);
        found = sci(SCI_SEARCHNEXT, 0, reinterpret_cast<LPARAM>(value.c_str()));
      }
      if (found < 0)
        MessageBeep(MB_ICONINFORMATION);
      SetFocus(editor());
      sync();
      return;
    }
    if (!editor() || sci(SCI_GETREADONLY))
      return;
    auto f = current.value;
    unsigned bit = field(id);
    wchar_t text[256]{};
    if (id == Family) {
      GetWindowTextW(controls[Family - 1], text, 256);
      if (!*text)
        return;
      f.family = utf8(text);
      f.mask |= NoteX::Family;
      apply(f, NoteX::Family);
    } else if (id == Size) {
      GetWindowTextW(controls[Size - 1], text, 256);
      wchar_t *end = nullptr;
      double size = wcstod(text, &end);
      if (end == text || *end || !std::isfinite(size) || size < 6 ||
          size > 96) {
        MessageBeep(MB_ICONWARNING);
        return;
      }
      f.size = static_cast<int>(size * 100);
      f.mask |= NoteX::Size;
      apply(f, NoteX::Size);
    } else if (id >= Bold && id <= Strike) {
      bool on = (current.mixed & bit) || !checked(id);
      f.mask |= bit;
      if (id == Bold)
        f.bold = on;
      if (id == Italic)
        f.italic = on;
      if (id == Underline)
        f.underline = on;
      if (id == Strike)
        f.strike = on;
      apply(f, bit);
    } else if (id >= Fore && id <= Highlight) {
      static COLORREF custom[16]{};
      CHOOSECOLORW cc{sizeof(cc)};
      cc.hwndOwner = owner;
      cc.lpCustColors = custom;
      cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;
      cc.lpfnHook = colorHook;
      cc.rgbResult = id == Fore ? f.fore : id == Back ? f.back : f.highlight;
      if (NppDarkMode::darkChooseColorW(&cc)) {
        f.mask |= bit;
        if (id == Fore)
          f.fore = cc.rgbResult;
        if (id == Back)
          f.back = cc.rgbResult;
        if (id == Highlight)
          f.highlight = cc.rgbResult;
        apply(f, bit);
      }
    } else if (id == Bullets || id == Numbers)
      list(id == Numbers);
    else if (id == CopyFormat) {
      painter = current.value;
      painter.mask = NoteX::All;
      hasPainter = true;
    } else if (id == PasteFormat && hasPainter)
      apply(painter, NoteX::All);
    else if (id == Clear)
      apply(NoteX::Format{}, NoteX::All);
    SetFocus(editor());
    sync();
  }

public:
  static void loadFonts() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring base = path;
    base.resize(base.find_last_of(L"\\/") + 1);
    // Do not register Inter: its private font caused garbled UI glyphs on some PCs.
    // Keep this independent of old font files left behind by portable upgrades.
    for (auto name : {L"JetBrainsMono.ttf"})
      AddFontResourceExW((base + L"fonts\\" + name).c_str(), FR_PRIVATE,
                         nullptr);
  }
  void init(HINSTANCE instance, HWND parent, ScintillaEditView **active) {
    owner = parent;
    view = active;
    scale = static_cast<int>(DPIManagerV2::getDpiForWindow(parent));
    background = CreateSolidBrush(RGB(28, 33, 38));
    auto uiFont = DPIManagerV2::getDefaultGUIFontForDpi(static_cast<UINT>(scale));
    uiFont.lfHeight = -px(13);
    font = CreateFontIndirectW(&uiFont);
    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = instance;
    wc.lpszClassName = L"NoteXFormattingToolbar";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    window = CreateWindowExW(0, wc.lpszClassName, L"Text formatting",
                             WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 1,
                             1, parent, nullptr, instance, this);
    tooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
                              WS_POPUP | TTS_ALWAYSTIP, 0, 0, 0, 0, window,
                              nullptr, instance, nullptr);
    const wchar_t *labels[] = {
        L"Font family", L"Font size",  L"B",     L"I",     L"U",
        L"S̶",           L"A",          L"▣",     L"✎",     L"• List",
        L"1. List",     L"Copy style", L"Paint", L"Clear", L""};
    const wchar_t *tips[] = {
        L"Font family — selected text",
        L"Font size (6–96 pt) — press Enter",
        L"Bold",
        L"Italic",
        L"Underline",
        L"Strikethrough",
        L"Font color",
        L"Text background color",
        L"Highlight color",
        L"Toggle bullet list",
        L"Toggle numbered list",
        L"Format Painter: copy first selected character's appearance",
        L"Format Painter: apply copied appearance",
        L"Clear selected formatting",
        L"Find next — Enter"};
    for (int id = Family; id <= Find; ++id) {
      bool combo = id <= Size;
      DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                    (combo        ? (CBS_DROPDOWN | WS_VSCROLL)
                     : id == Find ? (ES_AUTOHSCROLL | WS_BORDER)
                                  : BS_OWNERDRAW);
      HWND h = CreateWindowExW(
          0,
          combo        ? L"COMBOBOX"
          : id == Find ? L"EDIT"
                       : L"BUTTON",
          labels[id - 1], style, 0, 0, 1, combo ? px(260) : px(30), window,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
      controls.push_back(h);
      SendMessage(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
      if (combo) {
        SetWindowSubclass(h, comboProc, 1, reinterpret_cast<DWORD_PTR>(this));
        COMBOBOXINFO info{sizeof(info)};
        if (GetComboBoxInfo(h, &info) && info.hwndItem)
          SetWindowSubclass(info.hwndItem, comboProc, 1,
                            reinterpret_cast<DWORD_PTR>(this));
        SendMessage(h, CB_LIMITTEXT, 255, 0);
      }
      if (id == Find) {
        SetWindowSubclass(h, comboProc, 1, reinterpret_cast<DWORD_PTR>(this));
        SendMessage(h, EM_SETCUEBANNER, 0,
                    reinterpret_cast<LPARAM>(L"Find in document…"));
        SendMessage(h, EM_LIMITTEXT, 255, 0);
      } else if (!combo)
        SetWindowSubclass(h, buttonProc, 1, reinterpret_cast<DWORD_PTR>(this));
      TOOLINFOW ti{sizeof(ti)};
      ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
      ti.hwnd = window;
      ti.uId = reinterpret_cast<UINT_PTR>(h);
      ti.lpszText = const_cast<wchar_t *>(tips[id - 1]);
      SendMessageW(tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
    }
    std::set<std::wstring> names;
    LOGFONTW lf{};
    lf.lfCharSet = DEFAULT_CHARSET;
    HDC dc = GetDC(window);
    EnumFontFamiliesExW(dc, &lf, enumFont, reinterpret_cast<LPARAM>(&names), 0);
    ReleaseDC(window, dc);
    for (auto &name : names)
      SendMessageW(controls[0], CB_ADDSTRING, 0,
                   reinterpret_cast<LPARAM>(name.c_str()));
    NppDarkMode::autoSubclassAndThemeChildControls(window);
    for (int size :
         {8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 24, 28, 32, 36, 48, 72, 96})
      SendMessageW(controls[1], CB_ADDSTRING, 0,
                   reinterpret_cast<LPARAM>(std::to_wstring(size).c_str()));
  }
  void resize(int width, int top) {
    if (!window)
      return;
    int x = px(10), y = px(8);
    int widths[] = {160, 56, 30, 30, 30, 30, 34, 34,
                    34,  30, 30, 30, 30, 30, 240};
    for (size_t i = 0; i < controls.size(); ++i) {
      if (i == 2 || i == 6 || i == 9 || i == 11 || i == 14)
        x += px(10);
      int w = std::min(px(widths[i]), std::max(px(20), width - px(20)));
      if (x + w > width - px(10) && x > px(10)) {
        x = px(10);
        y += px(36);
      }
      if (i == 14 && x + w < width - px(10))
        x = width - px(10) - w;
      MoveWindow(controls[i], x, y, w, i < 2 ? px(260) : px(30), TRUE);
      x += w + px(2);
    }
    panelHeight = y + px(36);
    MoveWindow(window, 0, top, width, panelHeight, TRUE);
  }
  int height() const { return window ? panelHeight : 0; }
  void dpiChanged(UINT dpi) {
    scale = static_cast<int>(dpi);
    auto old = font;
    auto uiFont = DPIManagerV2::getDefaultGUIFontForDpi(static_cast<UINT>(scale));
    uiFont.lfHeight = -px(13);
    font = CreateFontIndirectW(&uiFont);
    for (auto h : controls)
      SendMessage(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    if (old)
      DeleteObject(old);
    RECT r{};
    GetClientRect(owner, &r);
    RECT panel{};
    GetWindowRect(window, &panel);
    POINT pt{panel.left, panel.top};
    ScreenToClient(owner, &pt);
    resize(r.right, pt.y);
  }
  void sync() {
    if (!window || controls.size() != 15 || !editor() || updating)
      return;
    HWND focus = GetFocus();
    if (focus && IsChild(window, focus))
      return;
    updating = true;
    const auto previousValue = current.value;
    const auto previousMixed = current.mixed;
    const int previousList = listState;
    const bool previousMixedList = mixedList;
    current.start = sci(SCI_GETSELECTIONSTART);
    current.end = sci(SCI_GETSELECTIONEND);
    sci(NoteX::GetFormat, 0, reinterpret_cast<LPARAM>(&current));
    for (LRESULT i = 1; i < sci(SCI_GETSELECTIONS); ++i) {
      NoteX::SelectionFormat other;
      other.start = sci(SCI_GETSELECTIONNSTART, i);
      other.end = sci(SCI_GETSELECTIONNEND, i);
      sci(NoteX::GetFormat, 0, reinterpret_cast<LPARAM>(&other));
      current.mixed |= other.mixed;
      for (unsigned bit = 1; bit <= NoteX::Highlight; bit <<= 1) {
        NoteX::Format a, b;
        a.merge(current.value, bit);
        b.merge(other.value, bit);
        if (!(a == b))
          current.mixed |= bit;
      }
    }
    auto first = sci(SCI_LINEFROMPOSITION, current.start),
         last = sci(SCI_LINEFROMPOSITION, current.end);
    if (last > first && current.end == sci(SCI_POSITIONFROMLINE, last))
      --last;
    auto kind = [&](LRESULT line) {
      auto pos = sci(SCI_GETLINEINDENTPOSITION, line);
      auto len = std::min<LRESULT>(32, sci(SCI_GETLINEENDPOSITION, line) - pos);
      auto data =
          reinterpret_cast<const char *>(sci(SCI_GETRANGEPOINTER, pos, len));
      std::string prefix =
          data ? std::string(data, static_cast<size_t>(len)) : "";
      if (prefix.starts_with("\xE2\x80\xA2 ") || prefix.starts_with("* "))
        return 1;
      size_t n = 0;
      while (n < prefix.size() && prefix[n] >= '0' && prefix[n] <= '9')
        ++n;
      return n && n + 1 < prefix.size() && prefix[n] == '.' &&
                     prefix[n + 1] == ' '
                 ? 2
                 : 0;
    };
    listState = kind(first);
    mixedList = last - first > 10000;
    if (!mixedList)
      for (auto line = first + 1; line <= last; ++line)
        if (kind(line) != listState) {
          mixedList = true;
          break;
        }
    if (mixedList)
      listState = 0;
    // Unspecified attributes inherit the active lexer style, just as rendering
    // does.
    auto stylePosition = current.start;
    if (current.start == current.end && stylePosition > sci(SCI_POSITIONFROMLINE, first)) --stylePosition;
    auto style = sci(SCI_GETSTYLEAT, stylePosition);
    auto &f = current.value;
    if (!(f.mask & NoteX::Bold))
      f.bold = sci(SCI_STYLEGETBOLD, style) != 0;
    if (!(f.mask & NoteX::Italic))
      f.italic = sci(SCI_STYLEGETITALIC, style) != 0;
    if (!(f.mask & NoteX::Underline))
      f.underline = sci(SCI_STYLEGETUNDERLINE, style) != 0;
    if (!(f.mask & NoteX::Family)) {
      char name[256]{};
      sci(SCI_STYLEGETFONT, style, reinterpret_cast<LPARAM>(name));
      f.family = name;
    }
    if (!(f.mask & NoteX::Size))
      f.size = static_cast<int>(sci(SCI_STYLEGETSIZEFRACTIONAL, style));
    if (!(f.mask & NoteX::Fore))
      f.fore = static_cast<unsigned>(sci(SCI_STYLEGETFORE, style));
    if (!(f.mask & NoteX::Back))
      f.back = static_cast<unsigned>(sci(SCI_STYLEGETBACK, style));
    if (!(f.mask & NoteX::Highlight))
      f.highlight = f.back;
    auto name = wide(f.family);
    auto updateText = [](HWND control, const wchar_t *value) {
      wchar_t existing[256]{};
      GetWindowTextW(control, existing, 256);
      if (wcscmp(existing, value) != 0)
        SetWindowTextW(control, value);
      // Native combo boxes may select text during programmatic updates. This
      // sync runs only outside the toolbar, so keep inactive values unselected.
      const auto selection = SendMessageW(control, CB_GETEDITSEL, 0, 0);
      if (LOWORD(selection) != HIWORD(selection))
        SendMessageW(control, CB_SETEDITSEL, 0, MAKELPARAM(-1, 0));
    };
    updateText(controls[0], (current.mixed & NoteX::Family) ? L"" : name.c_str());
    wchar_t size[40]{};
    swprintf_s(size, L"%g", f.size / 100.0);
    updateText(controls[1], (current.mixed & NoteX::Size) ? L"" : size);
    const bool appearanceChanged = !(previousValue == current.value) ||
        previousMixed != current.mixed || previousList != listState || previousMixedList != mixedList;
    bool writable = !sci(SCI_GETREADONLY),
         selected = current.start != current.end;
    for (size_t i = 0; i < controls.size(); ++i) {
      int id = static_cast<int>(i) + 1;
      bool enabled = writable && (selected || sci(SCI_GETSELECTIONS) == 1 || id == Bullets || id == Numbers);
      if (id == PasteFormat)
        enabled &= hasPainter;
      if (id == Find)
        enabled = true;
      if ((IsWindowEnabled(controls[i]) != FALSE) != enabled)
        EnableWindow(controls[i], enabled);
      if (id >= Bold && id <= Clear && (appearanceChanged || id == CopyFormat))
        InvalidateRect(controls[i], nullptr, FALSE);
    }
    updating = false;
  }
  void destroy() {
    if (window)
      DestroyWindow(window);
    window = nullptr;
    if (font)
      DeleteObject(font);
    font = nullptr;
    if (background)
      DeleteObject(background);
    background = nullptr;
  }
};
