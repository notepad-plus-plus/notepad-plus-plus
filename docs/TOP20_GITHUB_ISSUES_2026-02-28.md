# Top 20 GitHub Issues Snapshot (Notepad++)

Snapshot date: 2026-02-28

| # | Title | Comments | Status in this patch |
|---|---|---:|---|
| [14959](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/14959) | [DPI] Elements required works to properly support hiDPI | 124 | Not directly fixed |
| [13568](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/13568) | Context menu entry greyed out on Win10 | 60 | Not directly fixed |
| [6284](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/6284) | Notepad++ doesn't handle scaling with multi-monitor setup correctly | 56 | Not directly fixed |
| [11401](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/11401) | "Paste" is wrongly disabled in some cases | 44 | Not directly fixed |
| [13201](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/13201) | WANTED: "Edit with Notepad++" translation | 42 | Not directly fixed |
| [5232](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/5232) | Feature Request - word wrap per tab | 40 | Not directly fixed |
| [15683](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/15683) | [BUG] NppShell.Dll Throws Exception Crashes Explorer | 38 | Not directly fixed |
| [14103](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/14103) | Reload file message if saved on network drive back in notepad++ v8.5.6 64bit | 38 | Not directly fixed |
| [17519](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/17519) | [BUG] <NP++ 8.9.1 crashing when mouse scroll many lines | 36 | Not directly fixed |
| [13491](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/13491) | Two identical "Edit with Notepad++" appear in Total Commander | 36 | Not directly fixed |
| [5321](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/5321) | Block Select (Alt-Shift) ONLY Using Numeric Keypad Automatically Replaces Selected Text with Garbage Characters | 35 | Not directly fixed |
| [9445](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/9445) | Add option ot search only text files - speed up all files search | 33 | Partially addressed by performance work |
| [3116](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/3116) | feature request - highlight on scrollbar | 33 | Not directly fixed |
| [16977](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/16977) | [BUG] File Explorer Unclickable Navigation Pane | 32 | Not directly fixed |
| [17134](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/17134) | Strange crash - possible cause AMD Software Adrenalin Edition | 30 | Not directly fixed |
| [15138](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/15138) | Error of saving session XML File and Saving session error with Roaming profiles | 30 | Not directly fixed |
| [10070](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/10070) | Tab Resize, Tab Font Resize & Style, and Active Tab Background Color Feature Request | 30 | Not directly fixed |
| [10046](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/10046) | Document map is sluggish and laggy. | 30 | Not directly fixed |
| [9951](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/9951) | Inability to display some fonts | 29 | Not directly fixed |
| [8660](https://github.com/notepad-plus-plus/notepad-plus-plus/issues/8660) | Maximised Notepad++ has 1px margin next to "X"-button | 28 | Not directly fixed |

## What this patch delivers

- Native `NppConsole` integration into Visual Studio solution, NSIS/MSI installers, and portable package pipeline.
- Plugin startup scan refactor in `PluginsManager::loadPlugins` to reduce duplicated work and startup overhead.
- `NppConsole` runtime stability/performance hardening (Ctrl/C hook handling, process lifecycle cleanup, safer command persistence, faster restart path).
