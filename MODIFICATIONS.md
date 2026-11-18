# NoteX modification notice

## 2026-11-18 — NoteX update infrastructure

Modified the Windows startup, Help menu, command defaults and loader, obsolete update preferences, packaging, and version resources. Added an independent signed updater shared with native macOS, native entry points, recovery tests, offline signing tools, and release documentation. Original component copyright and license notices remain intact.


Modified distributions: NoteX 1.3.3 for Windows and 1.5.0 macOS Preview 5.
Modifier: Berk Kivircik. Latest modification date: November 18, 2026.
NoteX modifications: Copyright (C) 2026 Berk Kivircik.
The modified work as a whole is distributed under GNU GPL version 3.
Original copyright, license, and warranty notices remain in place.

## Upstream base

- Project: Notepad++.
- Release: v8.9.8.1.
- Repository: https://github.com/notepad-plus-plus/notepad-plus-plus
- Base commit: `dd40fe45598791e9aa38f3aeb200e10fb3d6338a`.

This repository starts with a reviewed source snapshot. It does not include the
maintainer's local Git history or personal workspace. Upstream authorship is
preserved through the original files, notices, and the exact base reference.

## Changes from the upstream base

The editor adds a visual formatting toolbar, per-range styles, pending insertion
formats, formatting-aware clipboard and undo operations, companion-file storage,
snapshot recovery integration, custom menu ordering/visibility, and updated tabs
and toolbar presentation. Scintilla changes implement style storage, edit tracking,
line/caret metrics, and rendering. Editor web-search commands are disabled.
The NoteX publication updates branding, legal notices, build discovery, and clean
packaging. Legacy `.notex` files and menu settings remain compatible.

## Modified upstream code and project files

The fourth native preview updates the macOS application, workspace, and feature
commands on November 1, 2026 for sidebar toggles, compact line-number margins,
and toolbar list commands. Native regression and upgrade tests cover these changes.

The third native preview adds `macOS/Configuration`, `macOS/FileSearch`,
`macOS/Workbench.inc`, and workbench integration tests. It extends the native
menu, application, storage, and build/test code for split views, file search,
preferences, shortcuts, language and theme editors, and recovery compatibility.
These changes were made November 1, 2026. Existing upstream notices are retained.

The second native preview adds `macOS/MenuLayout`, `macOS/Workspace`,
`macOS/FunctionIndex`, and the native feature commands and regression tests.
It extends native session validation and persistence, and bundles the original
Notepad++ Function List rules with their existing notices. Native builds now use
the included Boost.Regex headers. These additions were made November 1, 2026.
`PowerEditor/installer/functionList/fortran77.xml` corrects an unbalanced
comment expression, with a dated modification notice in the file.

The November 1, 2026 macOS port adds the AppKit application, storage, build,
and native integration tests in `macOS/`. It modifies
`scintilla/cocoa/ScintillaCocoa.mm` for formatting-aware clipboard handling and
guards an unused Windows-only include in `lexilla/lexers/LexUser.cxx`.
Original notices in both files are retained. The macOS port uses the same
NoteX editing engine but does not include every Windows-specific feature;
the platform guide lists the current boundary.

These files contain dated NoteX notices in addition to their original notices:

- `PowerEditor/src/MISC/Common/NppConstants.h`
- `PowerEditor/src/Notepad_plus.cpp`
- `PowerEditor/src/Notepad_plus.h`
- `PowerEditor/src/Notepad_plus.rc`
- `PowerEditor/src/Notepad_plus_Window.cpp`
- `PowerEditor/src/Notepad_plus_Window.h`
- `PowerEditor/src/NppBigSwitch.cpp`
- `PowerEditor/src/NppCommands.cpp`
- `PowerEditor/src/NppNotification.cpp`
- `PowerEditor/src/Parameters.h`
- `PowerEditor/src/ScintillaComponent/Buffer.cpp`
- `PowerEditor/src/WinControls/AboutDlg/AboutDlg.cpp`
- `PowerEditor/src/WinControls/AboutDlg/AboutDlg.h`
- `PowerEditor/src/WinControls/ContextMenu/ContextMenu.cpp`
- `PowerEditor/src/WinControls/Preference/preferenceDlg.cpp`
- `PowerEditor/src/WinControls/TabBar/TabBar.cpp`
- `PowerEditor/src/WinControls/ToolBar/ToolBar.cpp`
- `PowerEditor/src/resource.h`
- `PowerEditor/src/winmain.cpp`
- `PowerEditor/visual.net/notepadPlus.Cpp.props`
- `PowerEditor/visual.net/notepadPlus.vcxproj`
- `scintilla/src/ContractionState.cxx`
- `scintilla/src/ContractionState.h`
- `scintilla/src/Document.cxx`
- `scintilla/src/Document.h`
- `scintilla/src/EditModel.cxx`
- `scintilla/src/EditModel.h`
- `scintilla/src/EditView.cxx`
- `scintilla/src/EditView.h`
- `scintilla/src/Editor.cxx`
- `scintilla/src/Editor.h`
- `scintilla/src/MarginView.cxx`
- `scintilla/src/PositionCache.cxx`
- `scintilla/src/PositionCache.h`
- `scintilla/src/Style.h`
- `scintilla/src/ViewStyle.cxx`
- `scintilla/src/ViewStyle.h`
- `scintilla/win32/ScintillaWin.cxx`

## Added implementation and support files

- `PowerEditor/src/NoteXMenu.h`: menu customization and persistence.
- `PowerEditor/src/NoteXSidecar.h`: formatting companion-file persistence.
- `PowerEditor/src/NoteXTools.h`: formatting controls and editing commands.
- `scintilla/include/NoteXFormat.h`: formatting model and serialization.
- `PowerEditor/Test/NoteX/`: native tests, clean build, and smoke scripts.
- `PowerEditor/Packaging/NoteX/`: clean package recipe and reviewed runtime assets.
- Root documentation, licenses, release instructions, and privacy exclusions.

Modified source files identify this publication date. Earlier individual edit
dates are not asserted. Unchanged upstream components retain their original terms.

Upstream CI/release automation and redundant update-reference source ZIPs are
excluded. The optional LuaScript DLLs used only by upstream URL-detection tests
are omitted; those upstream tests require their own separately obtained tools.
The embedded libraries used to build NoteX remain included in full.

The NoteX palette theme carries restored DarkModeDefault/Zenburn attribution
and the original GPLv2 resource license. The upstream DansLeRuSH-Dark theme is
omitted because its header includes noncommercial terms. Markdown UDLs retain
their Edditoria attribution and include the complete MIT license.

The repository root LICENSE uses the standard GPLv3 text for license detection.
The original Notepad++ license file, including its copyright preamble, is retained
verbatim at LICENSES/Notepad-plus-plus-LICENSE.txt. The v1.3.0 release tag remains
unchanged and continues to identify its exact published source and artifacts.

The 1.3.1 naming migration also changes Parameters.cpp and the progress-window
class identifier in FindReplaceDlg.cpp. Previous formatting wire versions and
clipboard payloads remain readable; new output uses NoteX names and signatures.

## Native command and usability expansion - November 1, 2026

Berk Kivircik modified macOS/main.mm and the native editor, configuration, storage,
workspace, menu, test, and build modules. New native command modules implement list
continuation, wrapping tabs, compact preferences, editing/search/navigation tools,
legacy encoding maps, shared language/API data, syntax styles, UDL rules, separate
instances, and native application integrations. The macOS workflow packages and
checks matching DMG and ZIP applications. See macOS/FEATURE_PARITY.md for scope and
macOS/WINDOWS_COMMANDS.csv for the reviewed Windows command inventory. Original
shared-engine and language-data copyrights and license notices are retained.

## Windows font compatibility fix - November 18, 2026

Document tabs and formatting controls now use the Windows UI font. NoteX no
longer registers its bundled Inter font automatically, preventing the reported
garbled tab labels even when an older portable installation retains Inter.ttf.
The editor does not rewrite document text, formatting records, or session data
as part of this change.

FindReplaceDlg.cpp also moves a misplaced UTF-8 byte-order mark to the start
of the file so the reviewed source builds cleanly with MSVC.

## Windows application identity - November 18, 2026

Notepad_plus.rc and notepad++.exe.manifest now use NoteX for the application
identity and internal executable name. Packaging validates actual compiled
version resources and the embedded manifest. Original upstream copyright and
license notices and readers for previous document formats remain intact.
