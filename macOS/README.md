# NoteX for macOS

The native macOS edition uses AppKit, Cocoa Scintilla, the shared NoteX formatting
engine, and Lexilla. It does not use Wine, Rosetta, Electron, or a web view.
Version 1.5.0 adds automatic lists, left-aligned wrapping tabs, a compact preferences
window, and native equivalents for the Windows edition's editing and workbench
commands. See FEATURE_PARITY.md and WINDOWS_COMMANDS.csv for the comparison,
platform substitutions, and verification boundaries.

## Install

Requires an Apple Silicon Mac and macOS 14 or later. Open the release DMG and
drag NoteX.app to Applications, or extract the ZIP and move NoteX.app there.
The DMG and ZIP contain the same application. The application is ad-hoc signed,
not Apple Developer ID signed or notarized. If macOS blocks it, review the
download source and use System Settings > Privacy & Security > Open Anyway.
Do not disable Gatekeeper globally.

## Documents and updates

Session snapshots, including unsaved text, formatting, cursor positions, bookmarks,
project trees, menu layouts, and preferences,
are stored in `~/Library/Application Support/NoteX`, outside the application.
Quit NoteX normally before replacing NoteX.app. Keep the Application Support
folder. Replacing the application does not replace your session.
Split layout, independent view positions, shortcut assignments, custom languages,
and themes are also stored in the session folder, outside NoteX.app.

Command-Q and closing the window preserve open tabs. Explicitly closing a tab
asks whether to save or discard unsaved changes. Snapshots run while you edit;
an abrupt power failure can still lose edits since the last completed snapshot.
Save important work to a document and maintain backups.

Saved formatting uses the same `.notex` companion format as Windows. Keep the
document and its companion together. A missing font may be substituted by macOS.
Windows session.xml files and absolute Windows paths are not imported.

## Build

Install Xcode Command Line Tools, then run `python3 macOS/build.py` from a clean
source checkout. The output is `dist/macOS/NoteX.app`. No package manager or
downloaded runtime is needed. Run `python3 macOS/test.py` for isolated native
integration tests. Test data is synthetic and never comes from a user profile.

## Menus and panels

Choose NoteX > Customize Menus to hide commands and reorder top-level menus or
commands within a menu. Apply saves the layout; Cancel keeps the previous layout.
The application menu, Quit, and menu recovery commands remain accessible.
NoteX > Reset Menu Layout restores the defaults.

View > Function List toggles the sidebar and shows functions and classes for the active document. Double-click
a symbol to navigate to its definition. Filter narrows the tree; A–Z switches between
name order and source order. Rules lets you select a custom Notepad++ Function List
XML parser or return to automatic selection. The app bundles the upstream parser
rules and uses Boost.Regex. Parsing runs off the UI thread and supports documents
up to 16 MB. A syntax lexer does not necessarily have a Function List parser.
Valid definitions at the end of a file remain visible, including definitions that
the upstream parser omits when a match ends exactly at its search boundary.

View > Project Panel 1, 2, or 3 opens an independent workspace. Add creates a project;
Folder creates a logical group; Files adds existing files or directories. Rename,
Remove, and the arrows organize references without renaming or deleting disk files.
Double-click a file to open it. Open XML and Save XML import and export Notepad++
workspace files. Relative paths resolve beside the XML file. Absolute Windows paths
remain references until you select the file and use Relink to choose its Mac location.
An externally changed workspace is protected against silent replacement.

View > Folder as Workspace browses selected directories. Refresh rereads the directory
tree. Hidden files and symbolic links are skipped. The sidebar selector switches
between the five views; View > Toggle Sidebar hides or shows it. Selecting the
currently visible panel from the View menu hides it; selecting a different panel
switches to that panel. New profiles start with the sidebar hidden. Existing
profiles retain their visibility setting, including across app updates.

## Split and clone views

Choose View > Clone to Other View to show the same document in a second editor.
Both views share text, formatting, bookmarks, and undo history. Their selections,
scroll positions, and zoom are independent. View > Split Vertically or Split
Horizontally changes the arrangement; drag the divider to resize the panes.
The document selector above the second editor can show a different open document.
Commands and formatting apply to the focused editor. Close View closes the second
view without discarding the original document. View > Synchronize Scrolling links
the visible positions of the two panes. The split arrangement survives a restart.

## Find and replace in files

Choose Edit > Find in Files. Search a folder, all open documents (including unsaved
tabs), or files referenced by the three project panels. File filters and exclusions
accept semicolon-separated wildcards. Subfolders and hidden files are optional.
Literal, case-sensitive, whole-word, Boost.Regex, and ICU regular-expression searches
are available. Document search defaults to Boost.Regex. The engine selector keeps
previous macOS ICU expressions available. Regex replacements use `$1`, `$2`, and other capture-group references.

Find / Preview searches without changing files. Results include the file, line,
matching text, and proposed replacement. Double-click a result to navigate to it.
The replacement plan captures the replacement text entered before the search;
run the search again if you change the query or replacement.

Apply Previewed Replacements asks for confirmation. It checks that the searched
content is still current, skips dirty saved tabs and externally changed files,
backs up original documents and formatting companions, then saves through the
normal NoteX path. UTF-16 BOMs, formatting companions, and file permission bits
are preserved. Open Documents results, including dirty saved tabs and untitled tabs, are edited
with a single undo action and are not silently saved to disk. Successful changes remain applied if another file fails;
the result report lists skipped files and errors.

Open Backups in the search window opens the recovery directory.
Backups, `recovery.json`, and `report.json` are stored under
`~/Library/Application Support/NoteX/ReplaceBackups/<batch ID>`.
The recovery map is written before each file is changed, so original paths remain
available if the app stops during a batch. The final report records outcomes. Keep these backups
until you have reviewed the changes. Restore copies only after closing the file
in NoteX, and keep its formatting companion beside it.

Search is cancellable and bounded to 10,000 files and 20,000 matches. Unsupported
encodings, binary files, unreadable files, symbolic links, application bundles,
and files larger than 16 MB are skipped. A complex regular expression can be
stopped by the per-document time limit. Narrow the search if a limit is reached.

## Preferences, shortcuts, languages, and themes

NoteX > Preferences edits font, size, indentation, whitespace, wrapping, appearance,
caret behavior, the column guide, new-document encoding and line endings, and the
recovery interval. Reset restores defaults in the draft. Apply saves the changes;
Cancel leaves the current preferences unchanged. A longer recovery interval means
more recent edits can be lost if the process stops before the next snapshot.

NoteX > Shortcut Editor lists native commands, including hidden menu commands.
Choose a command, enter a character or F1–F20, select modifiers, and click Assign.
Apply checks conflicts and saves the draft. An empty key removes an assignment.
Text shortcuts require Command or Control; function keys can be unmodified.
Command-Q remains reserved for Quit. Reset All restores default shortcuts in the
draft. Menu visibility and shortcut assignments can be reset independently.

NoteX > Language Editor creates, duplicates, and deletes user-defined languages.
Configure filename extensions without a leading dot, eight keyword groups,
case sensitivity, line/block comments, ASCII string quote and escape characters,
and folding tokens. Open matching files to apply an extension rule, or choose
View > Choose User-Defined Language for an existing tab. Keyword matching supports
Unicode identifiers. User-defined syntax is styled for documents up to 16 MB.
The built-in Lexilla language selector remains available for standard languages.

NoteX > Theme Editor edits the editor background, text, caret, selection, current
line, line numbers, keyword groups, comments, strings, numbers, and operators.
Keyword bold and comment italic options are included. Choose a theme through
View > Choose Theme. Lexilla semantic style names map built-in syntax to theme
roles; custom languages use all eight keyword colors. Explicit per-range NoteX
formatting takes precedence over syntax colors.

The preferences, language, and theme editors support validated JSON import/export.
Import replaces the current draft; it does not change active settings until Apply.
Names must be unique. Language comment delimiters must be paired. Exported settings
contain configuration, not document text or session recovery data.

## Toolbar lists and line numbers

The Bulleted List and Numbered List toolbar buttons apply to the selected lines
or the current line when there is no selection. They use the same commands as
the Format menu, target the focused split view, and can be undone in one action.
A selection ending at the start of the next line leaves that next line unchanged.
Line-number margins adjust to the number of digits and the editor font instead
of reserving a fixed-width blank area. Bookmark and folding controls remain available.

## Other features
- NoteX > Preferences controls the default font, size, tab width, tabs/spaces,
  line numbers, whitespace, automatic indentation, brace matching, and word completion.
- Bookmarks supports toggle, next, previous, and clear. The editor margins also support
  bookmarks and syntax folding; View includes Fold All and Unfold All.
- File includes Save All, Reload from Disk, Print, and formatted RTF/HTML export.
- Format includes Copy Formatting, Paste Formatting, and Copy as Rich Text.
- Edit includes duplicate/delete line, case conversion, whitespace trimming,
  selected-line sorting, duplicate-line removal, and word completion.
- Encoding converts UTF-8/BOM and UTF-16 LE/BE and LF/CRLF/CR line endings.

## Compatibility limits

Windows DLL plugins cannot run in the native application. Native settings exports
use JSON; Windows session, shortcut, menu, macro, UDL, and theme configuration files
are not automatically imported. Notepad++ project workspace XML and NoteX formatting
companions are supported as described above. Both Boost.Regex and ICU are selectable; expressions must match the selected engine.
Macro parity and Windows settings/data migration are outside this release. Existing
native macro records remain readable for backward compatibility. The interface is US English.
Files use UTF-8, BOM-marked UTF-16, or an explicitly selected legacy encoding.
Undefined bytes and unrepresentable characters are rejected rather than silently converted. The Windows release remains available separately.


## Lists and many open documents

Return continues typed or toolbar-created `• `, `- `, `* `, and numbered `1. ` lists.
An empty list item exits the list and retains its indentation. Splitting an item
preserves the text after the caret. Return and the new marker undo together.
Continuation supports LF, CRLF, CR, and independent multiple carets. Pasting or
restoring text does not trigger continuation. List buttons toggle the current list
style or convert between bullets and numbers without duplicating markers.

Tabs start at the left, wrap into at most four visible rows, and scroll vertically
when more rows are needed. Previous/next buttons and Control-Tab cycle through
primary-view documents. The searchable Open Documents picker includes both views.
Drag to reorder tabs; the context menu provides pinning, colors, and group closing.
Move to Other View removes the document from the primary strip without copying its
text. Move it back or close the second view to return it to the primary strip.

## Editing and navigation

The Edit menu includes case conversion, line transforms, numeric and locale sorts,
comment/uncomment, rectangular and multiple selections, text and number column
editors, date/time insertion, RTF and HTML paste, binary clipboard content, redaction,
and clipboard history. Numeric sorts retain arbitrary precision. Numeric column
sequences support signed 64-bit values, increments, repeats, leading zeroes, and
binary/octal/decimal/hexadecimal output; overflow cancels the complete edit.

Search Options controls case, whole words, regular expressions, captured selection
scope, and incremental search. Find All retains that scope in the results window.
Search results and style marks support next/previous navigation. Clear Change and
Undo History discards undo records while preserving the text and unsaved status. Bookmarks support
copy, cut, replacement, deletion, and inversion. View includes a clickable document
map, change navigation, individual fold levels, RTL/LTR text, display modes, and
independent horizontal/vertical scroll synchronization.

Preferences groups Editing, Appearance, and Files & Recovery in a single compact
form. Optional automatic bracket/quote pairs wrap a single selection; multiple
selections retain normal typing. Function completion and parameter hints use the
bundled API definitions. Language includes 95 shared built-in language definitions;
this is separate from Function List parser coverage. Advanced rules in Language
Editor use the shared UDL lexer for prefix keyword groups, operators, numeric
formats, delimiters, nesting, and folding. Syntax Style Editor edits the selected
lexer's individual colors, font, size, and traits. Apply commits drafts; Cancel does
not change active settings. Settings import/export uses the native JSON format.

## Native sessions, independent windows, and tools

Save Session exports an explicitly chosen native session containing open document
text, including unsaved text. Treat these exports as private documents. Load Session
adds documents without discarding current tabs; duplicate file identities become
recovered untitled documents. This is a native session feature, not a Windows data
migration tool.

New Independent Window and Open in New Instance use a separate recoverable profile
under the current profile's `Instances` directory. The original document remains
open. Reopen Independent Window restores one of those profiles after it closes;
concurrent writers to the same profile are blocked. Keep the entire Application
Support directory when updating.

Run Command executes an explicitly entered or selected command only after Run is
pressed. It uses zsh with quoted `$NOTEX_FILE`, `$NOTEX_DIRECTORY`, `$NOTEX_SELECTION`,
and `$NOTEX_LINE` environment variables. Document values are passed as data, not
substituted into shell source. Output is limited to 1 MB; Stop sends termination to
the process group. Commands requiring interactive terminal input should be opened
in Terminal instead. Saved commands remain local. Hash tools support text,
selection, and file input for MD5, SHA-1, SHA-256, and SHA-512.

Finder, Terminal, Trash, the native Character Viewer, printing, and installed
macOS browsers provide the platform integrations. Monitoring is an explicit,
read-only tail mode for a saved document and stops when preserving edits requires it.
