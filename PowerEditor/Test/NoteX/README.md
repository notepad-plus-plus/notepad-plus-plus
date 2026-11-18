# NoteX native integration tests

`build-release.ps1` performs a clean x64 Release build and then invokes `run.ps1`.
The native tests link the application's Scintilla library. They exercise range
formatting, edit tracking, undo/redo, serialization, companion-file persistence,
clipboard conversion, variable line/caret metrics, insertion formats, snapshot
notifications, and menu customization/persistence. Clipboard cases temporarily
replace the Windows clipboard with synthetic data.

`smoke_release.py` runs four upstream Function List fixtures using isolated
directories and profiles. Tests do not read the maintainer's active application
profile. Generated outputs are excluded from source distribution. These checks
do not constitute full GUI, plugin, or all-language acceptance.
