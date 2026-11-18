// Copyright (C) 2026 Berk Kivircik.
// SPDX-License-Identifier: GPL-3.0-only
// Created for NoteX; publication date: 2026-10-28.

#define NOMINMAX
#include "NoteXFormat.h"
#include "NoteXSidecar.h"
#include "MISC/Common/NppConstants.h"
#include "Scintilla.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <windows.h>
int checks = 0;
int formatNotifications = 0;
LRESULT CALLBACK trackFormatNotifications(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (message == WM_NOTIFY) {
    const auto *notification = reinterpret_cast<const SCNotification *>(lParam);
    if (notification->nmhdr.code == SCN_MODIFIED &&
        (notification->modificationType & SC_MOD_CHANGESTYLE) &&
        notification->token == 0x4e505046)
      ++formatNotifications;
  }
  return DefWindowProcW(hwnd, message, wParam, lParam);
}
void check(bool condition, const char *name) {
  ++checks;
  if (!condition)
    throw std::runtime_error(name);
  std::cout << "PASS " << name << std::endl;
}
int main() {
  try {
    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    std::wstring fixture = executable;
    fixture.resize(fixture.find_last_of(L"\\/") + 1);
    fixture += L"native-sidecar-fixture.txt";
    auto instance = GetModuleHandle(nullptr);
    check(Scintilla_RegisterClasses(instance) != 0,
          "register native Scintilla");
    HWND parent = CreateWindowExW(0, L"STATIC", L"NoteX integration tests",
                                  WS_OVERLAPPEDWINDOW, 0, 0, 900, 500, nullptr,
                                  nullptr, instance, nullptr);
    HWND editor =
        CreateWindowExW(0, L"Scintilla", L"", WS_CHILD | WS_VISIBLE, 0, 0, 900,
                        450, parent, nullptr, instance, nullptr);
    check(editor != nullptr, "create editor");
    SetWindowLongPtrW(parent, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(trackFormatNotifications));
    auto send = [&](UINT m, WPARAM w = 0, LPARAM l = 0) {
      return SendMessage(editor, m, w, l);
    };
    auto text = [&]() {
      std::string s(static_cast<size_t>(send(SCI_GETLENGTH)) + 1, 0);
      send(SCI_GETTEXT, s.size(), reinterpret_cast<LPARAM>(s.data()));
      s.pop_back();
      return s;
    };
    auto query = [&](int a, int b) {
      NoteX::SelectionFormat q;
      q.start = a;
      q.end = b;
      send(NoteX::GetFormat, 0, reinterpret_cast<LPARAM>(&q));
      return q;
    };
    auto apply = [&](int a, int b, NoteX::Format f, unsigned fields) {
      NoteX::Change c;
      c.start = a;
      c.end = b;
      c.value = f;
      c.fields = fields;
      return send(NoteX::ApplyFormat, 0, reinterpret_cast<LPARAM>(&c));
    };
    auto reset = [&](const char *s) {
      send(SCI_SETREADONLY, 0);
      send(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(s));
      NoteX::Restore r;
      send(NoteX::SetState, 0, reinterpret_cast<LPARAM>(&r));
      send(SCI_EMPTYUNDOBUFFER);
      send(SCI_SETSAVEPOINT);
    };
    send(SCI_SETCODEPAGE, SC_CP_UTF8);
    send(SCI_SETMODEVENTMASK, NPP_SCINTILLA_MODEVENTMASK);
    reset("alpha beta\r\ngamma");
    NoteX::Format f;
    f.mask = NoteX::All;
    f.bold = true;
    f.italic = true;
    f.underline = true;
    f.strike = true;
    f.family = "Arial";
    f.size = 2400;
    f.fore = RGB(30, 140, 240);
    f.back = RGB(50, 60, 70);
    f.highlight = RGB(160, 120, 20);
    check(apply(0, 5, f, NoteX::All), "apply complete combined format");
    check(query(0, 5).value == f, "all nine attributes round trip");
    check(text() == "alpha beta\r\ngamma", "formatting leaves text unchanged");
    check(send(SCI_GETMODIFY), "format action marks document dirty");
    check(formatNotifications > 0,
          "application event mask delivers format changes to snapshot tracking");
    auto color = f;
    color.fore = RGB(180, 20, 40);
    apply(0, 5, color, NoteX::Fore);
    auto got = query(0, 5).value;
    f.fore = color.fore;
    check(got == f, "color preserves font and other styles");
    check(query(0, 10).mixed != 0, "mixed selection reported");
    send(SCI_UNDO);
    check(query(0, 5).value.fore == RGB(30, 140, 240), "undo color");
    send(SCI_REDO);
    check(query(0, 5).value == f, "redo color");
    send(SCI_INSERTTEXT, 2, reinterpret_cast<LPARAM>("XYZ"));
    check(query(2, 5).value == f, "insert inherits format");
    check(query(7, 8).value == f, "format shifts on insert");
    send(SCI_UNDO);
    check(text() == "alpha beta\r\ngamma" && query(0, 5).value == f,
          "undo text restores spans");
    send(SCI_REDO);
    check(query(0, 8).value == f, "redo text restores spans");
    send(SCI_UNDO);
    send(SCI_SETSEL, 1, 4);
    send(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>("Q"));
    send(SCI_UNDO);
    check(text() == "alpha beta\r\ngamma" && query(0, 5).value == f,
          "undo replacement restores formatting");
    send(SCI_BEGINUNDOACTION);
    apply(6, 10, f, NoteX::All);
    apply(12, 17, f, NoteX::All);
    send(SCI_ENDUNDOACTION);
    send(SCI_UNDO);
    check(query(6, 10).value.mask == 0 && query(12, 17).value.mask == 0,
          "grouped format undo");
    send(SCI_REDO);
    check(query(6, 10).value == f && query(12, 17).value == f,
          "grouped format redo");
    send(SCI_SETREADONLY, 1);
    check(!apply(0, 5, NoteX::Format{}, NoteX::All),
          "read-only format rejected");
    send(SCI_SETREADONLY, 0);
    check(NoteX::saveSidecar(editor, fixture.c_str()), "atomic sidecar save");
    NoteX::Restore empty;
    send(NoteX::SetState, 0, reinterpret_cast<LPARAM>(&empty));
    NoteX::loadSidecar(editor, fixture.c_str());
    check(query(0, 5).value == f, "sidecar restores combined format");
    const auto newCompanion = NoteX::sidecarPath(fixture.c_str());
    const auto oldCompanion = NoteX::previousSidecarPath(fixture.c_str());
    auto readBytes = [](const std::wstring &path) {
      std::ifstream stream(std::filesystem::path(path), std::ios::binary);
      return std::string(std::istreambuf_iterator<char>(stream), {});
    };
    auto writeBytes = [](const std::wstring &path, const std::string &bytes) {
      std::ofstream stream(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
      stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
      stream.close();
      return static_cast<bool>(stream);
    };
    auto oldBytes = readBytes(newCompanion);
    check(oldBytes.substr(0, 8) == "NOTEXSC1" && oldBytes.substr(40, 8) == std::string("NOTEX01\0", 8),
          "new companion files use NoteX signatures");
    oldBytes.replace(0, 8, "NPPSIDE1");
    oldBytes.replace(40, 8, std::string(NoteX::previousWireMagic, 8));
    check(writeBytes(oldCompanion, oldBytes) && DeleteFileW(newCompanion.c_str()),
          "prepare a previous-release companion fixture");
    send(NoteX::SetState, 0, reinterpret_cast<LPARAM>(&empty));
    NoteX::loadSidecar(editor, fixture.c_str());
    check(query(0, 5).value == f && readBytes(newCompanion) == oldBytes,
          "previous companion restores all attributes with unchanged bytes");
    check(GetFileAttributesW(oldCompanion.c_str()) == INVALID_FILE_ATTRIBUTES,
          "validated previous companion is renamed to the NoteX extension");
    check(writeBytes(oldCompanion, "unrelated companion content"), "prepare a companion collision fixture");
    NoteX::loadSidecar(editor, fixture.c_str());
    check(query(0, 5).value == f && readBytes(oldCompanion) == "unrelated companion content",
          "a current companion wins without overwriting a conflicting previous file");
    DeleteFileW(oldCompanion.c_str());
    check(NoteX::saveSidecar(editor, fixture.c_str()) && readBytes(newCompanion).substr(0, 8) == "NOTEXSC1",
          "saving an upgraded companion writes the new NoteX format");
    NoteX::State state;
    send(NoteX::GetState, 0, reinterpret_cast<LPARAM>(&state));
    auto packed = NoteX::encode(state);
    NoteX::State parsed;
    check(NoteX::decode(packed, parsed, 17) && parsed.runs == state.runs,
          "wire roundtrip");
    auto previousPacked = packed;
    previousPacked.replace(0, 8, std::string(NoteX::previousWireMagic, 8));
    check(NoteX::decode(previousPacked, parsed, 17) && parsed.runs == state.runs,
          "previous wire version remains readable after the rename");
    check(!NoteX::decode(packed.substr(0, packed.size() - 1), parsed, 17),
          "truncated sidecar rejected");
    packed[8] = static_cast<char>(255);
    check(!NoteX::decode(packed, parsed, 17), "invalid format count rejected");
    send(SCI_SETSEL, 0, 5);
    send(SCI_COPY);
    send(SCI_SETSEL, 17, 17);
    send(SCI_PASTE);
    check(text() == "alpha beta\r\ngammaalpha", "clipboard text paste");
    check(query(17, 22).value == f, "clipboard combined styles paste");
    send(SCI_UNDO);
    check(text() == "alpha beta\r\ngamma", "paste undo is one action");
    send(SCI_REDO);
    check(query(17, 22).value == f, "paste redo restores formatting");
    NoteX::State oldClipboardState;
    oldClipboardState.formats.push_back(f);
    oldClipboardState.runs.push_back({0, 5, 1});
    auto oldClipboardPayload = NoteX::encode(oldClipboardState);
    oldClipboardPayload.replace(0, 8, std::string(NoteX::previousWireMagic, 8));
    check(OpenClipboard(editor) != FALSE, "open clipboard for previous-version fixture");
    EmptyClipboard();
    auto oldTextHandle = GlobalAlloc(GMEM_MOVEABLE, 6 * sizeof(wchar_t));
    auto oldFormatHandle = GlobalAlloc(GMEM_MOVEABLE, oldClipboardPayload.size() + 8);
    check(oldTextHandle && oldFormatHandle, "allocate previous clipboard fixture");
    auto oldText = GlobalLock(oldTextHandle);
    memcpy(oldText, L"alpha", 6 * sizeof(wchar_t));
    GlobalUnlock(oldTextHandle);
    auto oldFormatData = static_cast<char *>(GlobalLock(oldFormatHandle));
    const auto oldPayloadSize = static_cast<std::uint64_t>(oldClipboardPayload.size());
    memcpy(oldFormatData, &oldPayloadSize, 8);
    memcpy(oldFormatData + 8, oldClipboardPayload.data(), oldClipboardPayload.size());
    GlobalUnlock(oldFormatHandle);
    SetClipboardData(CF_UNICODETEXT, oldTextHandle);
    SetClipboardData(RegisterClipboardFormatW(NoteX::previousClipboardName().c_str()), oldFormatHandle);
    CloseClipboard();
    reset("");
    send(SCI_PASTE);
    check(text() == "alpha" && query(0, 5).value == f,
          "previous-release rich clipboard preserves text and all formatting");
    reset("\xC4\xB0\xC4\x9F\xC3\xBC hello");
    apply(0, 6, f, NoteX::All);
    send(SCI_SETSEL, 0, 6);
    send(SCI_COPY);
    send(SCI_SETSEL, 12, 12);
    send(SCI_PASTE);
    check(query(12, 18).value == f, "Unicode byte positions survive clipboard");
    reset("one\r\ntwo");
    apply(5, 8, f, NoteX::All);
    send(SCI_SETSEL, 0, 8);
    send(SCI_COPY);
    reset("");
    send(SCI_SETEOLMODE, SC_EOL_LF);
    send(SCI_PASTE);
    check(text() == "one\ntwo" && query(4, 7).value == f,
          "clipboard EOL conversion maps format positions");
    reset("abc");
    apply(0, 3, f, NoteX::All);
    send(SCI_UNDO);
    auto italic = f;
    italic.bold = false;
    apply(0, 3, italic, NoteX::Italic);
    check(!send(SCI_CANREDO), "new format invalidates redo branch");
    check(query(0, 3).value.mask == NoteX::Italic,
          "branch does not resurrect previous style");
    // Exercise real layout and both rendering paths, including wrapping and
    // large fonts.
    send(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE);
    send(SCI_SETWRAPMODE, SC_WRAP_WORD);
    send(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>("Consolas"));
    apply(0, 3, f, NoteX::All);
    ShowWindow(parent, SW_SHOWNOACTIVATE);
    UpdateWindow(parent);
    UpdateWindow(editor);
    check(send(SCI_TEXTHEIGHT, 0) > 20,
          "rich font contributes to layout height");
    send(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DEFAULT);
    InvalidateRect(editor, nullptr, TRUE);
    UpdateWindow(editor);
    check(send(SCI_GETSTATUS) == 0, "GDI and DirectWrite rendering complete");
    auto largeHeight = send(SCI_TEXTHEIGHT, 0);
    apply(0, 3, NoteX::Format{}, NoteX::All);
    check(send(SCI_TEXTHEIGHT, 0) < largeHeight,
          "clear format restores line height");
    reset("small\nBIG\nsmall\ntiny");
    send(SCI_SETWRAPMODE, SC_WRAP_NONE);
    send(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
    send(SCI_STYLECLEARALL);
    NoteX::Format sized;
    sized.mask = NoteX::Size;
    sized.size = 3000;
    apply(6, 9, sized, NoteX::Size);
    sized.size = 600;
    apply(16, 20, sized, NoteX::Size);
    auto heightSmall = send(SCI_TEXTHEIGHT, 0);
    auto heightBig = send(SCI_TEXTHEIGHT, 1);
    check(heightBig > heightSmall * 2 && send(SCI_TEXTHEIGHT, 2) == heightSmall,
          "large font changes only its own row height");
    check(send(SCI_TEXTHEIGHT, 3) < heightSmall,
          "smaller than default font has smaller row height");
    send(SCI_GRABFOCUS);
    auto caretHeight = [&](int pos) {
      send(SCI_GOTOPOS, pos);
      InvalidateRect(editor, nullptr, FALSE);
      UpdateWindow(editor);
      MSG msg{};
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      GUITHREADINFO info{sizeof(info)};
      return GetGUIThreadInfo(GetCurrentThreadId(), &info)
                 ? info.rcCaret.bottom - info.rcCaret.top
                 : -1;
    };
    const auto smallCaret = caretHeight(1), bigCaret = caretHeight(7),
               tinyCaret = caretHeight(17);
    std::cout << "CARET " << smallCaret << " " << bigCaret << " " << tinyCaret
              << std::endl;
    check(smallCaret > 0 && bigCaret > smallCaret * 2 && tinyCaret < smallCaret,
          "native caret height follows current character font");
    auto y = [&](int pos) { return send(SCI_POINTYFROMPOSITION, 0, pos); };
    check(y(6) - y(0) == heightSmall && y(10) - y(6) == heightBig,
          "document positions use variable row heights");
    auto x = send(SCI_POINTXFROMPOSITION, 0, 11);
    check(send(SCI_POSITIONFROMPOINTCLOSE, x, y(10) + heightSmall / 2) == 11,
          "mouse hit testing matches variable row positions");
    send(SCI_GOTOPOS, 1);
    send(SCI_LINEDOWN);
    check(send(SCI_LINEFROMPOSITION, send(SCI_GETCURRENTPOS)) == 1,
          "down arrow enters large row");
    send(SCI_LINEDOWN);
    check(send(SCI_LINEFROMPOSITION, send(SCI_GETCURRENTPOS)) == 2,
          "down arrow leaves large row in one step");
    send(SCI_LINEUP);
    check(send(SCI_LINEFROMPOSITION, send(SCI_GETCURRENTPOS)) == 1,
          "up arrow enters large row in one step");
    send(SCI_HIDELINES, 1, 1);
    check(y(10) - y(0) == heightSmall,
          "hidden large row contributes no vertical space");
    send(SCI_SHOWLINES, 1, 1);
    check(y(10) - y(6) == heightBig,
          "showing large row restores vertical space");
    send(SCI_UNDO);
    send(SCI_UNDO);
    check(send(SCI_TEXTHEIGHT, 1) == heightSmall && y(10) == heightSmall * 2,
          "undo restores row metrics and coordinates");
    send(SCI_REDO);
    check(send(SCI_TEXTHEIGHT, 1) == heightBig,
          "redo restores local row height");
    reset("small BIG small");
    sized.size=3000;apply(6,9,sized,NoteX::Size);
    const auto mixedSmallCaret=caretHeight(1), mixedBigCaret=caretHeight(7);
    check(mixedBigCaret>mixedSmallCaret*2, "caret follows character font within one mixed-size row");
    reset("small\nBIG\nsmall");apply(6,9,sized,NoteX::Size);
    send(SCI_INSERTTEXT,0,reinterpret_cast<LPARAM>("prefix\n"));
    check(send(SCI_TEXTHEIGHT,2)>send(SCI_TEXTHEIGHT,1)*2, "inserting a line shifts local font metrics");
    send(SCI_UNDO);
    check(send(SCI_TEXTHEIGHT,1)>send(SCI_TEXTHEIGHT,2)*2, "undo line insertion restores local font metrics");
    std::string wrapped(90, 'W');
    wrapped += "\nsmall";
    reset(wrapped.c_str());
    sized.size = 3000;
    apply(0, 90, sized, NoteX::Size);
    MoveWindow(editor, 0, 0, 220, 250, TRUE);
    send(SCI_SETWRAPMODE, SC_WRAP_CHAR);
    InvalidateRect(editor, nullptr, FALSE);
    UpdateWindow(editor);
    const auto wrappedRows = send(SCI_WRAPCOUNT, 0);
    const auto wrappedHeight = send(SCI_TEXTHEIGHT, 0);
    check(wrappedRows > 1 && y(91) - y(0) == wrappedRows * wrappedHeight,
          "wrapped rows use their document line font height");
    send(SCI_GOTOPOS, 94);
    send(SCI_SCROLLCARET);
    check(y(94) >= 0 && y(94) + send(SCI_TEXTHEIGHT, 1) <= 250,
          "caret scroll keeps small row visible after wrapped large row");
    x = send(SCI_POINTXFROMPOSITION, 0, 93);
    check(send(SCI_POSITIONFROMPOINTCLOSE, x, y(93) + 5) == 93,
          "scrolled hit testing uses the same variable height origin");
    send(SCI_SETWRAPMODE, SC_WRAP_NONE);
    MoveWindow(editor, 0, 0, 900, 450, TRUE);
    reset("abcdef");
    send(SCI_STARTSTYLING, 0);
    send(SCI_SETSTYLING, 6, 7);
    apply(0, 6, f, NoteX::All);
    check(send(SCI_GETSTYLEAT, 2) == 7, "formatting preserves lexer style IDs");
    send(SCI_EMPTYUNDOBUFFER);
    send(SCI_INSERTTEXT, 3, reinterpret_cast<LPARAM>("X"));
    send(SCI_UNDO);
    check(query(0, 6).value == f,
          "undo after clearing history preserves restored spans");
    auto invalid = f;
    invalid.size = 1000000;
    check(!apply(0, 6, invalid, NoteX::Size), "invalid font size rejected");
    reset("one two");
    apply(0, 3, f, NoteX::All);
    send(SCI_SETSEL, 0, 3);
    send(SCI_COPY);
    send(SCI_SETSEL, 4, 7);
    send(SCI_PASTE);
    check(text() == "one one" && query(4, 7).value == f,
          "formatted paste replaces unformatted target");
    send(SCI_UNDO);
    check(text() == "one two" && query(4, 7).value.mask == 0,
          "paste undo restores unformatted target");
    reset("abcd");
    send(SCI_BEGINUNDOACTION);
    send(SCI_INSERTTEXT, 0, reinterpret_cast<LPARAM>("A"));
    apply(0, 1, f, NoteX::All);
    send(SCI_INSERTTEXT, 5, reinterpret_cast<LPARAM>("Z"));
    send(SCI_ENDUNDOACTION);
    send(SCI_UNDO);
    check(text() == "abcd" && query(0, 4).value.mask == 0,
          "mixed text and format transaction undo");
    send(SCI_REDO);
    check(text() == "AabcdZ" && query(0, 1).value == f,
          "mixed transaction redo");
    send(SCI_SETSAVEPOINT);
    apply(1, 3, f, NoteX::Bold);
    check(send(SCI_GETMODIFY), "format-only edit leaves save point");
    send(SCI_UNDO);
    check(!send(SCI_GETMODIFY), "undo format reaches save point");
    send(SCI_REDO);
    check(send(SCI_GETMODIFY), "redo format leaves save point");
    HWND second = CreateWindowExW(0, L"Scintilla", L"", WS_CHILD, 0, 0, 100,
                                  100, parent, nullptr, instance, nullptr);
    SendMessage(second, SCI_SETDOCPOINTER, 0, send(SCI_GETDOCPOINTER));
    NoteX::SelectionFormat shared;
    shared.start = 0;
    shared.end = 1;
    SendMessage(second, NoteX::GetFormat, 0,
                reinterpret_cast<LPARAM>(&shared));
    check(shared.value == f, "split views share formatting");
    SendMessage(second, SCI_UNDO, 0, 0);
    check(!send(SCI_GETMODIFY), "undo from split view updates shared document");
    DestroyWindow(second);
    reset("left");
    send(SCI_GOTOPOS, 4);
    NoteX::Format typing;
    typing.mask = NoteX::All;
    typing.bold = true; typing.size = 2800; typing.fore = RGB(130, 70, 190);
    typing.family = "Arial"; typing.underline = true; typing.strike = true;
    typing.back = RGB(40, 48, 54); typing.highlight = RGB(100, 90, 30);
    check(apply(4, 4, typing, typing.mask), "choose typing format without selection");
    check(!send(SCI_GETMODIFY) && !send(SCI_CANUNDO) && query(0, 4).value.mask == 0,
          "typing choice neither edits existing text nor creates undo action");
    NoteX::Format italicTyping;
    italicTyping.mask = NoteX::Italic; italicTyping.italic = true;
    apply(4, 4, italicTyping, NoteX::Italic);
    typing.merge(italicTyping, NoteX::Italic);
    check(query(4, 4).value == typing, "pending typing attributes merge and round trip");
    send(WM_CHAR, 'X'); send(WM_CHAR, 'Y');
    check(text() == "leftXY" && query(4, 6).value == typing && query(0, 4).value.mask == 0,
          "typed characters use pending format without changing preceding text");
    send(SCI_UNDO);
    check(text() == "left" && !send(SCI_GETMODIFY), "one undo removes typed text and its format");
    send(SCI_REDO);
    check(text() == "leftXY" && query(4, 6).value == typing, "redo restores typed formatting");
    send(SCI_GOTOPOS, 6);
    check(query(6, 6).value == typing, "end of line controls inherit preceding character");
    apply(6, 6, NoteX::Format{}, NoteX::All);
    send(WM_CHAR, 'Z');
    check(query(6, 7).value.mask == 0 && query(4, 6).value == typing,
          "clear typing style leaves preceding formatted text intact");
    apply(7, 7, typing, NoteX::All);
    send(SCI_GOTOPOS, 0); send(SCI_GOTOPOS, 7);
    check(query(7, 7).value.mask == 0, "moving caret clears pending typing choice");
    reset(""); send(SCI_GOTOPOS, 0);
    apply(0, 0, typing, NoteX::All);
    send(WM_CHAR, 0x011f); // Turkish g-breve, encoded as two UTF-8 bytes
    check(text() == "\xc4\x9f" && query(0, 2).value == typing,
          "typing style supports empty documents and Unicode characters");
    send(SCI_NEWLINE); send(WM_CHAR, 'B');
    check(query(static_cast<int>(send(SCI_GETLENGTH)) - 1, static_cast<int>(send(SCI_GETLENGTH))).value == typing,
          "typing choice continues across Enter");
    send(SCI_SETREADONLY, 1);
    const auto caret = static_cast<int>(send(SCI_GETCURRENTPOS));
    check(!apply(caret, caret, NoteX::Format{}, NoteX::All), "read-only document rejects pending format");
    reset("ab"); send(SCI_GOTOPOS, 2);
    apply(2, 2, typing, NoteX::All);
    send(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>("paste"));
    check(text() == "abpaste" && query(2, 7).value == typing,
          "replacement at empty caret uses typing choice");
    reset("plain "); send(SCI_GOTOPOS, 6);
    apply(6, 6, typing, NoteX::All);
    check(OpenClipboard(parent) != 0, "open clipboard for external plain text fixture");
    EmptyClipboard();
    auto plainHandle = GlobalAlloc(GMEM_MOVEABLE, 12 * sizeof(wchar_t));
    auto plainPointer = static_cast<wchar_t *>(GlobalLock(plainHandle));
    wcscpy_s(plainPointer, 12, L"external");
    GlobalUnlock(plainHandle);
    SetClipboardData(CF_UNICODETEXT, plainHandle); CloseClipboard();
    send(SCI_PASTE);
    check(text() == "plain external" && query(6, 14).value == typing,
          "external plain paste uses chosen insertion formatting");
    send(SCI_UNDO);
    check(text() == "plain " && query(0, 6).value.mask == 0,
          "plain paste and insertion format undo together");
    reset("fresh"); send(SCI_GOTOPOS, 5);
    check(query(5, 5).value.mask == 0, "restoring document state clears typing choice");

    reset("snapshot text");
    check(NoteX::saveSidecar(editor, fixture.c_str()), "save initial unformatted snapshot");
    auto formatEvents = formatNotifications;
    apply(0, 8, f, NoteX::All);
    check(formatNotifications > formatEvents,
          "format-only change after snapshot emits backup notification");
    check(NoteX::saveSidecar(editor, fixture.c_str()), "save format-only snapshot update");
    formatEvents = formatNotifications;
    send(SCI_UNDO);
    check(formatNotifications > formatEvents && query(0, 8).value.mask == 0,
          "format-only undo emits backup notification");
    formatEvents = formatNotifications;
    send(SCI_REDO);
    check(formatNotifications > formatEvents && query(0, 8).value == f,
          "format-only redo emits backup notification");
    reset("snapshot text");
    NoteX::loadSidecar(editor, fixture.c_str());
    check(query(0, 8).value == f, "snapshot reload restores format without text edits");
    formatEvents = formatNotifications;
    send(SCI_STARTSTYLING, 0);
    send(SCI_SETSTYLING, 8, 7);
    check(formatNotifications == formatEvents,
          "ordinary lexer styling is not a persistent format change");

    NoteX::Formatting model;
    std::vector<NoteX::Format> reference(30);
    std::mt19937 random(7301);
    bool matches = true;
    for (int step = 0; step < 500; ++step) {
      size_t pos = random() % (reference.size() + 1);
      int op = random() % 3;
      if (op == 0) {
        auto inherited =
            reference.empty() ? NoteX::Format{} : reference[pos ? pos - 1 : 0];
        model.insert(pos, 1);
        reference.insert(reference.begin() + pos, inherited);
      } else if (op == 1 && pos < reference.size()) {
        model.erase(pos, 1);
        reference.erase(reference.begin() + pos);
      } else if (pos < reference.size()) {
        size_t end = pos + 1 + random() % (reference.size() - pos);
        NoteX::Change c;
        c.start = pos;
        c.end = end;
        c.fields = (random() % 2) ? NoteX::Bold : NoteX::Fore;
        c.value.mask = c.fields;
        c.value.bold = true;
        c.value.fore = RGB(10, 40, 170);
        model.apply(c);
        for (size_t i = pos; i < end; ++i) {
          reference[i].mask |= c.fields;
          if (c.fields == NoteX::Bold)
            reference[i].bold = true;
          else
            reference[i].fore = c.value.fore;
        }
      }
      for (size_t i = 0; i < reference.size(); ++i)
        matches &= model.state.formats[model.at(i)] == reference[i];
    }
    check(matches,
          "500 deterministic span edits match per-character reference");
    DestroyWindow(parent);
    Scintilla_ReleaseResources();
    std::cout << "TOTAL " << checks << " passed" << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << std::endl;
    return 1;
  }
}
