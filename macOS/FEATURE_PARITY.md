# Native macOS feature comparison

Release target: NoteX 1.5.0, native Apple Silicon, macOS 14 or later.
Review date: November 1, 2026. Maintainer: Berk Kivircik.

This comparison concerns the features of the NoteX Windows application. It does
not describe importing a Windows installation, profile, session, or personal data.
Macro parity and Windows DLL plugins are excluded. Existing native macro records
remain readable so an upgrade does not discard older profile data.

`WINDOWS_COMMANDS.csv` inventories 478 distinct commands reachable from the shared
Windows menu resource. Its entries are source mappings, not 478 independent test
results. Runtime-generated NoteX formatting controls, native language catalogs,
and preferences are covered separately below. Similar command names alone are not
evidence of equivalent behavior.

## Feature and verification matrix

| Windows capability | Native macOS implementation | Behavioral verification |
| --- | --- | --- |
| Per-range fonts, sizes, colors, bold, italic, underline, strike, pending format, rich copy | Shared NoteX engine; native toolbar and Format menu | Format ranges, mixed selections, line height, clipboard, shared undo, `.notex` round trips in SelfTest and FeatureTests |
| Bulleted and numbered lists | Toolbar/menu toggles and native Return handling | Typed prefixes, conversion, empty-item exit, indentation, middle-line splitting, LF/CRLF/CR, multiple carets, read-only, undo/redo in ParityTests |
| Many open documents | Left-aligned wrapping tabs, at most four visible rows, vertical scrolling, previous/next, Control-Tab, searchable picker, drag order, pins, colors, group close | 100-document layout, selection preservation, wraparound, pin records; real AppKit screenshots |
| File operations and recovery | Save/copy/rename/reload/all-save, Trash, recent/reopen, native session export/load, atomic snapshots outside the app | Collision protection, stale disk checks, encoding/format preservation, previous snapshot fallback, restart, replacement, forced termination after a completed snapshot |
| Split/clone/move to another view | Shared Scintilla document; independent selection, scroll and zoom; horizontal/vertical splits; separate axis synchronization | Shared text/format/undo, focused editor commands, pane transfer and read-only clones in WorkbenchTests and ParityTests |
| Independent instance | Separate process and recoverable profile; original document remains open; reopen saved instance | Child process verifies exact unsaved Unicode text and formatting; profile locking tests prevent concurrent writers |
| Find/replace, selection scope, mark, brace navigation | Boost.Regex or ICU, case/whole-word flags, incremental search, captured selection, results window, six mark styles | UTF-8 offsets, captures, invalid expressions, scoped results, grouped replacement undo; stale captured selections fail closed |
| Find/replace in files | Folder/open-document/project/current-document scopes, filters, preview, cancellation, backups, stale checks | Unicode and UTF-16 searches, exclusions, replacement plans, changed-file rejection, backup manifests, real editor saves and undo in WorkbenchTests |
| Line, case, comment and selection operations | Native commands plus shared Scintilla actions | Representative line reversal/removal/case operations and grouped undo; numeric parser tests include very large integers and signed decimals |
| Column mode and editor | Rectangular selection, text columns, checked signed numeric sequences with repeat/base/width | Numeric formatting, repeats, negative values, overflow rejection; editing stays in the focused document |
| Bookmarks and change navigation | Shared markers; next/previous, copy/cut/replace/delete/invert; change markers | Bookmark persistence and line operations; shared engine behavior retained |
| Syntax and completion | 95 shared language definitions, linked Lexilla engines, bundled API definitions, word/function/path completion and parameter hints | Every catalog lexer is instantiated; representative Python API loading and actual lexer styling tested; not every language grammar or API overload is exhaustively tested |
| User-defined languages | Native editor with legacy-compatible rules and optional shared UDL lexer rules: keywords/prefixes, comments, operators, numbers, delimiters/nesting and folding | Validation, Unicode custom styling, prefix keywords and delimiter styling, native configuration round trips |
| Themes and syntax styles | Native theme editor plus per-lexer style colors, fonts, sizes and traits | Draft validation/cancel, theme application, fractional sizes and actual syntax style application |
| Preferences and shortcuts | Compact grouped preferences; native shortcut editor with conflicts and reset; native JSON settings | Real editor settings, cancel behavior, fractional fonts, conflict rejection, persistence; screenshots of the actual dialogs |
| Menu and context menu customization | Stable command IDs, visibility/order, independent reset and protected recovery commands | Existing menu persistence/reset tests; native command catalog and context configuration validation |
| Function List and project panels | Shared parser rules and Boost.Regex; three project panels plus folder workspace | Parser fixtures, asynchronous outline population, symbol navigation, XML workspace round trips, stale workspace protection |
| Encoding and EOL | UTF-8/BOM, UTF-16 LE/BE, explicit single/multibyte legacy encodings, CR/LF/CRLF | All 256 entries in each of 42 shipped single-byte maps are checked for defined-scalar round trips or rejection; representative native legacy conversion and Unicode save tests |
| Clipboard and text tools | RTF/HTML paste, binary clipboard, app-scoped history, redaction, dates, native Character Viewer | RTF Unicode/formatting and grouped undo; history is session-local and limited to NoteX clipboard actions |
| Document map and display | Clickable line overview; folding levels, whitespace/EOL/Unicode representations, RTL/LTR, fullscreen, focus and floating modes | Native document-map/tab screenshots; editor display preferences and layout tests |
| Run and hashes | Explicit zsh command runner, native environment variables, stop/output limits, saved commands; MD5/SHA-1/SHA-256/SHA-512 | Literal Unicode environment values cannot become shell source; process-group stop; standard SHA-256 vector |
| OS integration | Finder, Terminal, Trash, installed macOS browsers, AppKit print dialog/print-now | Source-reviewed native API paths; physical printers and third-party browser installations are environment-dependent and not exercised by hosted CI |

## Platform substitutions and boundaries

- Windows DLL loading, plugin administration, Windows file attributes, Internet
  Explorer, Windows shell commands, and Windows-specific command-line switches do
  not run on macOS. Corresponding native editor workflows use the APIs listed above.
- Automatic upstream updates remain disabled, matching NoteX's distribution policy.
- No Windows settings/data migration is added. Native configuration uses validated
  JSON. Native session exports contain document text and must be treated as private.
  Existing project XML and `.notex` compatibility are retained.
- The interface is US English. Native dialogs and menu organization differ from
  Win32. Preferences expose native editing, appearance, and recovery behavior rather
  than reproducing Windows registry, rendering, or installation controls.
- The document map is a lightweight line overview. AppKit substitutes fonts absent
  from the Mac. Clipboard history records only this app's clipboard actions.
- Automatic bracket/quote pairing is optional and applies to one selection; typing
  with multiple selections uses the normal Scintilla path. List continuation does
  support multiple carets.
- Search/Function List input is bounded to 16 MB per document, 10,000 files and
  20,000 search matches. Complex expressions have execution limits. A failed or
  truncated replacement plan is not silently applied.
- Encoding choices are explicit. Undefined bytes, unsupported encodings and lossy
  conversions fail rather than corrupting text. Boost and ICU expressions follow
  their selected engine; they are not universally interchangeable.
- CI builds one ARM64 application on macOS 15 and tests that same binary on macOS
  26. It captures real AppKit windows. This does not establish acceptance on a
  particular M5 device or macOS 27 installation, nor does it test every possible
  combination of language, theme, shortcut and document.

## Release verification

`macOS/test.py` uses isolated synthetic profiles. It checks native integration,
normal restart, replacing the app, recovery after process termination, and profiles
seeded by the actual published 1.3.2, 1.3.3, 1.4.0 and 1.4.1 binaries. Old downloads
are pinned by SHA-256. No user profile is used for tests or packaging.

The workflow verifies the ad-hoc signature, mounts the generated DMG read-only,
compares its application with the ZIP's build input, and checks the Applications
shortcut. Publication includes the matching source archive and checksums. The
application remains ad-hoc signed, without Apple notarization; a DMG does not change
that signing status. Release test counts belong to the exact published commit and
are recorded in its workflow artifacts and release notes.
