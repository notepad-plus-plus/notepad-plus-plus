# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Notepad++ is a Windows-native source code editor written in C++20. It wraps the Scintilla editor component and Lexilla lexer library, adding tabbed editing, plugin support, docking panels, and extensive configuration. It is a Win32 application using the Windows API directly (no cross-platform framework).

## Build Commands

### MSBuild (primary, requires Visual Studio 2022 v143 toolset)

```
msbuild PowerEditor\visual.net\notepadPlus.sln /p:Configuration=Debug /p:Platform=x64
```

Platforms: `x64`, `Win32`, `ARM64`. Configurations: `Debug`, `Release`. Building the solution automatically builds Scintilla and Lexilla as dependencies.

### GCC / MinGW-w64 (via MSYS2)

```
cd PowerEditor/gcc
mingw32-make                    # Release build
mingw32-make DEBUG=1            # Debug build
mingw32-make CXX=clang++        # Use Clang
mingw32-make VERBOSE=1          # Show commands
```

Output goes to `bin.x86_64/` or `bin.i686/` (with `-debug` suffix for debug builds).

### CMake

Requires pre-building Scintilla and Lexilla via nmake first:
```
cd scintilla\win32 && nmake -f scintilla.mak
cd lexilla\src && nmake -f lexilla.mak
cd PowerEditor\src && cmake . && cmake --build .
```

## Running Tests

Tests require Windows and a built `notepad++.exe` (Win32/Debug):

```powershell
# Function List parser tests
.\PowerEditor\Test\FunctionList\unitTestLauncher.ps1

# URL detection tests
.\PowerEditor\Test\UrlDetection\verifyUrlDetection.ps1

# XML validation (requires Python with lxml, rfc3987)
python PowerEditor\Test\xmlValidator\validator_xml.py
```

CI commit message flags: `[force all]`, `[force one]`, `[force xml]`, `[force none]` control which CI jobs run.

## Architecture

### Core Class Hierarchy

```
winmain.cpp (WinMain entry point)
  └─ Notepad_plus_Window     (main window, message loop)
       └─ Notepad_plus        (core logic, command dispatch)
            ├─ ScintillaEditView ×2   (main + sub edit views, wraps Scintilla)
            ├─ DocTabView ×2          (tab bars for each view)
            ├─ FileManager            (singleton, manages all Buffer objects)
            ├─ NppParameters          (singleton, all config/settings)
            ├─ PluginsManager         (loads plugin DLLs)
            ├─ DockingManager         (docking panel framework)
            ├─ FindReplaceDlg         (find/replace)
            └─ PreferenceDlg          (settings UI)
```

### Key Source Files

| File | Role |
|------|------|
| `PowerEditor/src/winmain.cpp` | Entry point, instance management |
| `PowerEditor/src/Notepad_plus.cpp` | Core app logic (~361KB), manages views, tabs, file ops |
| `PowerEditor/src/NppBigSwitch.cpp` | WM_COMMAND message dispatcher |
| `PowerEditor/src/NppCommands.cpp` | Menu command implementations |
| `PowerEditor/src/NppNotification.cpp` | Notification handlers |
| `PowerEditor/src/NppIO.cpp` | File I/O operations |
| `PowerEditor/src/Parameters.cpp` | NppParameters singleton (~352KB), config loading/saving |
| `PowerEditor/src/ScintillaComponent/ScintillaEditView.cpp` | Editor view wrapper |
| `PowerEditor/src/ScintillaComponent/Buffer.cpp` | Buffer/FileManager - document management |
| `PowerEditor/src/MISC/PluginsManager/PluginsManager.cpp` | Plugin loading and management |
| `PowerEditor/src/MISC/PluginsManager/Notepad_plus_msgs.h` | Plugin message API definitions |
| `PowerEditor/src/MISC/PluginsManager/PluginInterface.h` | Plugin DLL interface contract |

### Source Directory Layout (PowerEditor/src/)

- **ScintillaComponent/** - Editor views, buffers, find/replace, auto-completion, printing
- **WinControls/** - All UI controls: docking windows, dialogs, toolbars, panels (FunctionList, FileBrowser, DocumentMap, ProjectPanel, etc.)
- **MISC/** - Plugin manager, common utilities, hashing (md5/sha), registry, exception handling
- **DarkMode/** - Dark mode theming
- **uchardet/** - Character encoding detection (vendored)
- **pugixml/** - XML parsing (vendored)
- **json/** - JSON parsing

### Vendored Dependencies (not git submodules)

- **scintilla/** - Scintilla editor component, built as `libScintilla.lib`
- **lexilla/** - Lexilla lexer library (~130 lexers), built as `libLexilla.lib`
- **boostregex/** - Stripped Boost 1.90.0 for PCRE regex (used instead of ECMAScript regex)

### Plugin System

Plugins are DLLs exporting a C interface defined in `PluginInterface.h`. Required exports: `setInfo`, `getName`, `getFuncsArray`, `beNotified`, `messageProc`, `isUnicode`. Communication uses Windows messages defined in `Notepad_plus_msgs.h`.

## Coding Style

- **Braces**: Allman style (braces on their own line). Exception: single-line method definitions in headers may use Java-style.
- **Indentation**: Tabs (display width typically 4 spaces)
- **Naming**: PascalCase for classes, camelCase for methods/parameters, underscore prefix for member variables (`_memberVar`)
- **Operators**: Spaces around binary/ternary operators
- **Casts**: C++ casts only (`static_cast<>`, not C-style)
- **Logical ops**: Use `!`, `&&`, `||` (not `not`, `and`, `or`)
- **Increment**: Prefer pre-increment (`++i`)
- **Init**: Use `{}` brace initialization for non-primitives, `=` for primitives/enums
- **Modern C++**: Use C++11/14/17/20 features. Prefer `constexpr` over `const`. Prefer `unique_ptr` over `shared_ptr`. Avoid raw `new`.
- **Strings**: Use `empty()` not `!= ""`
- **Comments**: C++ line style (`//`), not C block style (`/* */`)
- **No Yoda conditions**: Write `if (x == true)` not `if (true == x)`
- **No `using namespace` in headers**

## CI

GitHub Actions workflow (`.github/workflows/CI_build.yml`) builds with MSBuild (all 6 platform/config combos), CMake (Release/x64), and MSYS2 GCC/Clang. Tests run on Win32/Debug only.
