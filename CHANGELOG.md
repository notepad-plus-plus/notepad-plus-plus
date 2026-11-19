# Changelog

## Unreleased

- On Windows, customize the formatting toolbar to show, hide, reorder, and
  restore its items. The layout persists across restarts.

## 1.7.1

- Corrected the download instructions and the source paths in the modification notice.
- Updated the macOS Run Command help to show `NOTEQU_*` variables and made `.notequsession` the default extension for new session exports. Existing commands, profile overrides, and session files remain compatible.
- Published the Windows x64 portable ZIP and Apple Silicon macOS DMG and ZIP from the same source revision, with a matching source archive, SHA-256 checksums, and signed update inventories for both platforms.
- Verified the in-app update from 1.7.0 on Windows, macOS 15, and macOS 26. The Windows test recovered an unsaved session and retained custom commands and a macro. The macOS tests recovered unsaved text and inline formatting and retained custom preferences.

## 1.7.0

- Renamed the project and the Windows and macOS apps to NoteQu. Updated system metadata, menus, executable and bundle names, and the GitHub release source.
- Added a [manual transition from NoteX](UPDATING.md#from-notex-to-notequ) to preserve existing profiles, settings, and `.notex` formatting companions.

## 1.6.2 — Consistent About dialog version

- The Windows About dialog now uses the same version definition for its heading and license notice. The notice no longer shows an outdated application version or modification date.
- Preserve upstream attribution, copyright notices, and the GNU GPL version 3 license text.
- Windows and native Apple Silicon macOS packages share the 1.6.2 release version.

## 1.6.1 — Update verification and Windows helper identity

- Add explicit NoteX product, version, and executable metadata to the Windows update helper.
- Extend interruption, binary architecture, and migration validation.
- Retire the bootstrap-only distribution notice while preserving unowned and customized files.
- Keep public-trust Windows signing and Apple notarization status separate from signed update inventories.


## 1.6.0 — Authenticated updates

- Add a platform-specific GitHub updater with signed file inventories, normal-shutdown installation, and application-only recovery.
- Disable the upstream editor updater in code while preserving plugin administration.
- Remove the bundled PHP-help and Wikipedia-selection commands, including their default shortcuts.
- Keep Windows profile and plugin files outside the update payload, and keep native macOS profile data outside the application bundle.
- Separate update authentication from Windows publisher signing and Apple notarization.


## 1.3.3 Windows - 2026-11-18

- Use NoteX consistently in the executable's internal name and embedded application manifest, alongside the existing NoteX product name and file description.
- Validate the compiled executable's numeric version, all version-string translations, and manifest before packaging and when verifying a release.
- Retain the Windows font compatibility fix and previous-format recovery support.
- Windows shortcut labels are independent of EXE metadata. Rename a shortcut created under an older name to NoteX, or create a new shortcut to the updated NoteX.exe.

## 1.3.2 Windows - 2026-11-18

- Fix garbled document-tab labels reported on Windows 11 when the bundled Inter font is enabled, including names of new, unsaved tabs.
- Use the Windows UI font for document tabs and formatting controls, including after DPI changes.
- Stop automatically registering the bundled Inter font. Existing portable installations can retain the old font file without reactivating the issue.
- Preserve document content, formatting records, settings, and session recovery during an executable-only update.

## 1.5.0 macOS Preview 5 - 2026-11-01

- Continue and exit typed or toolbar-created lists with Return, preserving indentation and grouped undo.
- Add left-aligned wrapping tabs, bounded scrolling, document search, reordering, pins, colors, and group closing.
- Replace the redundant preferences sidebar with compact, aligned groups.
- Expand native editing, selections, columns, comments, sorts, bookmarks, marks, completion, navigation, and display commands.
- Add Boost.Regex search, captured-selection results, explicit legacy encodings, and lossless conversion.
- Add shared language definitions, advanced UDL rules, and per-language syntax style editing.
- Add pane transfer, document map, clipboard history, context menu customization, native sessions, independent windows, monitoring, and command/hash tools.
- Preserve older native snapshots and formatting companions; verify upgrades from the four prior native releases.
- Ship matching DMG and ZIP applications with corresponding source and SHA-256 checksums.
- Keep macro parity, Windows data migration, Windows DLL plugins, and Windows-only integrations outside this release.

## 1.4.1 macOS Preview 4 - 2026-11-01

- Toggle the selected sidebar panel from its View menu command and show its checked state.
- Start new profiles with the sidebar hidden and preserve existing visibility preferences.
- Fit line-number margins to their digit count and font size.
- Add accessible Bulleted List and Numbered List toolbar buttons.
- Keep list changes inside the selected lines, retain focused-clone editing, and group undo.
- Verify upgrading sessions created by the published 1.4.0 native binary.

## 1.4.0 macOS Preview 3 - 2026-11-01

- Add horizontal and vertical split views, shared-document clones, independent
  view state, a second-view document selector, and synchronized scrolling.
- Add background file search across folders, open tabs, and project references,
  with filters, regex, Unicode match navigation, and previewed replacements.
- Protect replace-in-files operations with stale-content checks, recovery copies,
  result reports, encoding preservation, and formatting-aware normal saves.
- Add a native preferences editor with reset and JSON import/export.
- Add a shortcut editor with conflict detection and persistent command assignments.
- Add user-defined language editing, extension associations, keyword groups,
  comments, strings, folding, and Unicode keyword matching.
- Add theme editing, syntax colors, font traits, and JSON import/export.
- Preserve new configuration and split state across restarts and app replacement.
- Preserve existing file permission bits during atomic macOS file writes.
- Keep Windows binaries and historical releases unchanged.

## 1.3.3 macOS Preview 2 - 2026-11-01

- Add persistent native menu visibility and ordering, with Apply, Cancel, and Reset.
- Add Function List using the upstream XML rules and Boost.Regex, including class
  groups, filtering, sorting, custom parsers, and navigation to UTF-8 byte positions.
- Add three independent project panels, Notepad++ workspace XML import/export,
  relative paths, reference management, relinking, and Folder as Workspace.
- Add editing macro recording, named macros, repeat playback, and grouped undo.
- Add editor preferences, bookmarks, folding, automatic indentation, brace matching,
  word completion, line operations, encoding and line-ending conversion.
- Add format copying, rich-text copying, RTF/HTML export, Save All, Reload, and Print.
- Keep project, menu, macro, and bookmark state outside the application bundle.
- Extend isolated native tests to upstream Function List fixtures, workspace safety,
  feature persistence, and upgrading a session created by the published first preview.
- Preserve the Windows release and document remaining platform differences.

## 1.3.2 macOS Preview 1 - 2026-11-01

- Add a native Apple Silicon application with AppKit windows, menus, tabs,
  file dialogs, and Mac keyboard shortcuts; no Windows compatibility runtime.
- Reuse NoteX's Scintilla formatting and undo engine with the Cocoa renderer.
- Add native rich-text clipboard support and the bundled Lexilla syntax lexers.
- Preserve Unicode text, formatting, selections, and open tabs in atomic session
  snapshots outside the application bundle, with a previous-snapshot fallback.
- Read and write Windows-compatible formatting companions. Reject mismatched
  companions, unrelated companion files, and externally changed documents.
- Add isolated native tests for formatting, clipboard, file persistence,
  restart, application replacement, and recovery after process termination.
- Document preview limitations separately from the Windows feature set.
- Keep the Windows 1.3.1 binaries and their corresponding source unchanged.

## 1.3.1 - 2026-10-31

- Complete the NoteX naming migration across source, namespaces, classes, native
  window identities, filenames, themes, scripts, tests, and package inputs.
- Write formatting companions with the `.notex` extension and NoteX signatures.
- Read previous companions and rich clipboard payloads, and migrate valid
  companion filenames without rewriting text or overwriting current companions.
- Migrate the previous menu settings filename and theme preference.
- Include the fix for selected-looking inactive font-family and font-size fields.
- Preserve original upstream copyright and license notices.

## 1.3.0 - 2026-10-28

First public NoteX editor release.
Based on Notepad++ 8.9.8.1.

- Publish the complete modified editor source and build/packaging scripts.
- Rename the visible product, executable package, and version metadata to NoteX.
- Include visual range formatting, pending typing formats, format painting,
  lists, variable line/caret heights, and formatting-aware undo/redo.
- Include companion-file persistence, session snapshot integration, and
  persistent menu customization.
- Preserve GDI defaults, session recovery, and disabled upstream auto-updates.
- Ship English, Turkish, and Spanish interface packages.
- Add dated modification notices, corresponding-source instructions, original
  copyright attribution, and complete font/component license notices.
- Build a fresh public profile without personal documents, histories, screenshots,
  validation logs, optional plugin DLLs, or the upstream updater.

The earlier NoteX feature work is summarized here; this date identifies its
NoteX publication and does not claim that all features were authored on one day.
