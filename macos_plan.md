# Notepad++ macOS Port via Win32 Shim Layer

## Context

Notepad++ is a Windows-only C++ text editor deeply coupled to Win32 APIs (~291 distinct functions, ~4,100+ call sites across 284 files). The user wants to build it for macOS by creating a compatibility shim that translates Win32 calls to Cocoa/AppKit/CoreGraphics equivalents, keeping the existing source code largely unmodified.

**Key insight**: The vendored Scintilla already has **complete macOS/Cocoa support** (`scintilla/cocoa/`) with Xcode projects, and Lexilla is fully platform-independent. The editor core is already portable — the challenge is the application shell and UI layer.

**Goal**: MVP first — get a minimal working editor (Scintilla + file open/save + tabs + menus) on macOS, then iterate.

---

## Architecture: Win32 Compatibility Shim

### Approach
Create a library of Win32 type definitions and API implementations backed by Cocoa/AppKit. Notepad++ source `#include`s this shim instead of `<windows.h>`. Same `.cpp` files compile on both platforms with `#ifdef __APPLE__` only where unavoidable.

### What gets shimmed vs. replaced

| Shimmed (Win32 API → Cocoa) | Replaced (macOS-native rewrite) |
|---|---|
| Window management (HWND → NSView) | `winmain.cpp` → `main.mm` + NSApplicationDelegate |
| Message dispatch (SendMessage → WndProc routing) | `DarkMode/` → NSAppearance-based (~200 lines) |
| GDI drawing → CoreGraphics | `dpiManagerV2` → NSScreen backingScaleFactor |
| Menus → NSMenu | `ReadDirectoryChanges/` → FSEvents |
| Dialogs → programmatic NSView creation | `Processus.cpp` → NSTask/posix_spawn |
| Common controls → AppKit controls | `regExtDlg.cpp` → stub/UTI |
| File I/O → POSIX/NSFileManager | `MiniDumper`/`Win32Exception` → signal handlers |
| DLL loading → dlopen/dlsym | `verifySignedfile.cpp` → Security.framework or stub |

### String strategy
Use macOS native 32-bit `wchar_t`. `L""` literals work naturally. Convert to UTF-16 only at Scintilla and file I/O boundaries via `WideCharToMultiByte`/`MultiByteToWideChar` shim implementations.

### Handle registry pattern
HWND is an opaque pointer. A global `HandleRegistry` maps HWND ↔ NSView* bidirectionally, stores WNDPROC and user data per handle. All existing code passes HWNDs unchanged; the shim resolves to NSView* internally.

---

## Project Structure

```
notepad-plus-plus/
  macos/
    CMakeLists.txt              # Top-level macOS build
    shim/
      include/
        windows.h               # Master shim (replaces <windows.h>)
        windef.h                 # HWND, HDC, HBRUSH, COLORREF, RECT, etc.
        winbase.h                # File I/O, DLL, string functions
        winuser.h                # Window/message/dialog/menu APIs
        wingdi.h                 # GDI → CoreGraphics
        commctrl.h               # ListView, TreeView, TabControl, etc.
        tchar.h, shellapi.h, shlwapi.h, commdlg.h, uxtheme.h, richedit.h
      src/
        handle_registry.mm       # HWND ↔ NSView* mapping
        win32_window.mm          # CreateWindowEx, ShowWindow, etc.
        win32_message.mm         # SendMessage, PostMessage dispatch
        win32_gdi.mm             # GDI → CoreGraphics
        win32_dialog.mm          # Dialog management
        win32_controls.mm        # Common controls
        win32_file.mm            # File I/O → POSIX
        win32_menu.mm            # Menu → NSMenu
        win32_string.mm          # lstrlen, wsprintf, WideCharToMultiByte
        win32_dll.mm             # LoadLibrary → dlopen
        win32_misc.mm            # Registry stubs, system info, clipboard
    platform/
      main.mm                    # NSApplication entry point
      AppDelegate.mm             # Bootstraps Notepad_plus_Window
      NppDarkMode_mac.mm         # NSAppearance integration
      ScintillaEditView_mac.mm   # ScintillaView (Cocoa) bridge
      FileMonitor_mac.mm         # FSEvents replacement
    resources/
      Info.plist
```

### Build system: CMake
- Adds `macos/shim/include/` first on include path (overrides `<windows.h>`)
- Compiles `.mm` files for all shim/platform code
- Builds Scintilla from `scintilla/cocoa/` (wrap existing Xcode project or add CMakeLists)
- Builds Lexilla via existing makefile (already works on macOS)
- Links: AppKit, CoreGraphics, CoreText, CoreFoundation, Security frameworks
- Excludes Windows-only files, substitutes macOS replacements

---

## Key Type Mappings

```
HWND        → opaque pointer (resolved to NSView* via HandleRegistry)
HDC         → wrapper struct around CGContextRef + selected GDI objects
HINSTANCE   → NSBundle* token
HMODULE     → void* from dlopen()
HFONT       → NSFont* wrapper
HBRUSH      → NSColor*/CGColorRef wrapper
HMENU       → NSMenu* wrapper
COLORREF    → uint32_t (same 0x00BBGGRR format)
WPARAM/LPARAM/LRESULT → intptr_t
MSG         → struct with hwnd, message, wParam, lParam, time, pt
RECT/POINT/SIZE → struct with matching fields
```

---

## Message Loop Bridge

Cocoa events → Win32 MSG → WndProc dispatch:

1. `main.mm` creates NSApplication, sets AppDelegate
2. AppDelegate creates main NSWindow, initializes Notepad_plus_Window
3. Custom `Win32WindowView` (NSView subclass) translates Cocoa events to WM_* messages
4. `SendMessage(hwnd, msg, w, l)` → looks up WNDPROC in HandleRegistry → calls directly
5. `PostMessage(hwnd, msg, w, l)` → enqueues via `dispatch_async` on main thread
6. `SetTimer`/`KillTimer` → NSTimer

Key event mappings: mouseDown→WM_LBUTTONDOWN, keyDown→WM_KEYDOWN/WM_CHAR, drawRect→WM_PAINT, setFrameSize→WM_SIZE, NSWindowWillClose→WM_CLOSE.

---

## Phased MVP Plan

### Phase 0: Foundation (Weeks 1-4)
**Goal**: Shim headers compile. Non-GUI code builds.

1. Create directory structure under `macos/`
2. Write all shim headers with type definitions (~50 Win32 types, ~142 WM_* constants, WS_* styles, VK_* keycodes)
3. Implement trivial functions: `lstrlen`→`wcslen`, `lstrcpy`→`wcscpy`, `wsprintf`→`swprintf`, `OutputDebugString`→`NSLog`, `GetLastError`/`SetLastError`, `MulDiv`
4. Stub SAL annotations (`_In_`, `_Out_`), calling conventions (`WINAPI`, `CALLBACK`), `__declspec`
5. Build Scintilla Cocoa + Lexilla for macOS
6. Set up CMake that compiles shim + pugixml + uchardet + `Parameters.cpp`

**Files to create**: All `macos/shim/include/*.h`, `macos/CMakeLists.txt`
**Verification**: `cmake --build .` succeeds for non-GUI subset

### Phase 1: Window + Scintilla (Weeks 5-10)
**Goal**: A window appears with a working Scintilla editor.

1. Implement `main.mm` with NSApplicationDelegate
2. Implement HandleRegistry (`handle_registry.mm`)
3. Implement `RegisterClass`, `CreateWindowEx`, `DestroyWindow`
4. Implement `ShowWindow`, `MoveWindow`, `GetClientRect`, `SetWindowPos`, `InvalidateRect`
5. Create `ScintillaEditView_mac.mm` — instantiate Cocoa `ScintillaView` instead of `CreateWindowEx("Scintilla",...)`; hook up direct function pointer via `SCI_GETDIRECTFUNCTION`
6. Implement `SendMessage` dispatch to registered WndProc
7. Implement Win32WindowView (NSView subclass routing Cocoa events → WM_* → WndProc)
8. Stub enough of `Notepad_plus_Window::init()` to show main window

**Key files to modify (ifdef guards)**:
- `ScintillaEditView.cpp:348-378` — Scintilla init
- `winmain.cpp` — replaced by `main.mm`

**Verification**: App launches, Scintilla view visible, can type text

### Phase 2: File I/O + Menus (Weeks 11-18)
**Goal**: Open, edit, and save files. Menu bar works.

1. File I/O shim: `CreateFile`→`open()`, `ReadFile`→`read()`, `WriteFile`→`write()`, `FindFirstFile`→`opendir()`, path separator conversion
2. Menu shim: `CreateMenu`→`NSMenu`, `InsertMenuItem`→`NSMenuItem`, `CheckMenuItem`, `EnableMenuItem`, `SetMenu`→`NSApp.mainMenu`, `TrackPopupMenu`→`popUpContextMenu`
3. File dialog: `CustomFileDialog` → `NSOpenPanel`/`NSSavePanel`
4. Keyboard accelerators: custom lookup table, Ctrl→Cmd mapping for standard shortcuts
5. `WideCharToMultiByte`/`MultiByteToWideChar` via `iconv` or CoreFoundation
6. Wire menu command IDs to `WM_COMMAND` dispatch

**Verification**: Open file → edit → save cycle works. Menus visible and functional.

### Phase 3: Tabs + Multi-Document (Weeks 19-26)
**Goal**: Tabbed editing with multiple files.

1. Tab control shim: `TCM_*` messages → custom NSView tab bar
2. `WM_NOTIFY` dispatch for `TCN_SELCHANGE`
3. StatusBar as simple NSView at window bottom
4. Basic toolbar as NSView button bar
5. `WM_TIMER` → NSTimer
6. File monitoring: FSEvents replacement for `ReadDirectoryChanges/`
7. Basic splitter via NSSplitView

**Verification**: Open multiple files in tabs, switch between them, status bar shows info.

### Phase 4: Find/Replace + Essential Dialogs (Weeks 27-36)
**Goal**: Key dialogs work.

1. Dialog subsystem: `CreateDialogIndirectParam` with DLGTEMPLATEEX parsing → NSView hierarchy
2. `GetDlgItem`, `SetDlgItemText`, `SendDlgItemMessage`, `CheckDlgButton`
3. Basic controls: NSButton, NSTextField, NSComboBox, NSCheckbox
4. FindReplaceDlg implementation
5. GoToLineDlg
6. macOS dark mode via NSAppearance

**Verification**: Find/Replace works (find, replace, find in files). Go to line works.

---

## Files Requiring Direct Modification (ifdef __APPLE__)

| File | Change |
|---|---|
| `winmain.cpp` | Skip entirely; replaced by `macos/platform/main.mm` |
| `ScintillaEditView.cpp` (lines 348-378) | Use ScintillaView (Cocoa) instead of CreateWindowEx("Scintilla") |
| `NppDarkMode.cpp` / `DarkMode.cpp` | Skip entirely; replaced by `NppDarkMode_mac.mm` |
| `dpiManagerV2.cpp` | Skip entirely; replaced by NSScreen backing scale |
| `ReadDirectoryChanges/*.cpp` | Skip entirely; replaced by `FileMonitor_mac.mm` |
| `Processus.cpp` | Skip; replaced with NSTask |
| `MiniDumper.cpp` / `Win32Exception.cpp` | Skip; replaced with signal handlers |
| `verifySignedfile.cpp` | Stub out |

## Files That Should Work Unchanged (via shim)

- `Notepad_plus.cpp`, `NppBigSwitch.cpp`, `NppCommands.cpp` — core logic uses Win32 types but via shim they compile
- `Parameters.cpp` — XML config, uses pugixml (cross-platform)
- `Buffer.cpp` / `FileManager` — document management
- All `WinControls/` classes — use shim's HWND/SendMessage/GDI implementations
- `ScintillaEditView.cpp` (aside from init) — uses direct function pointers

## Key Existing Code to Reuse

- `scintilla/cocoa/ScintillaView.h` — Cocoa Scintilla NSView (already complete)
- `scintilla/cocoa/Scintilla.xcodeproj` — build Scintilla for macOS
- `lexilla/src/Lexilla/Lexilla.xcodeproj` — build Lexilla for macOS
- `lexilla/include/Lexilla.h` — already detects `__APPLE__` and uses `.dylib`
- `pugixml/` — cross-platform XML parser (used by Parameters.cpp)
- `uchardet/` — cross-platform encoding detection

---

## Risks

1. **`wchar_t` 32-bit vs 16-bit**: Some code may assume 2 bytes/char for buffer sizing. Need careful audit of `sizeof(wchar_t)` assumptions.
2. **Dialog template parsing**: DLGTEMPLATEEX → NSView requires DLU-to-pixel conversion using font metrics. Layout may differ.
3. **Message ordering**: Win32 guarantees specific message sequences (WM_CREATE before WM_SIZE). Cocoa event model differs.
4. **Common control fidelity**: TreeView/ListView have dozens of messages with specific semantics. Missing messages cause subtle bugs.

## Verification Strategy

After each phase:
1. Build with `cmake --build macos/build`
2. Run the app and test the target functionality
3. Compare behavior with Windows Notepad++ for the same operations
4. For Phase 0: compilation success is the test
5. For Phases 1-4: manual functional testing of each capability
