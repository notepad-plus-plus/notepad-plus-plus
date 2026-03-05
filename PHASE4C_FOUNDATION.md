# Phase 4C Foundation

## Scope Delivered

This file records the initial Phase 4C status bar foundation delivered on 2026-03-05.

- Added status bar container to main window layout.
- Added read-only status fields:
  - current language label
  - modified state (`Edited` / `Saved`)
  - cursor position (`Ln <line>, Col <col>`)
  - encoding (`UTF-8`)
  - line ending mode (`LF` / `CRLF` / `CR`)
  - read-only state (`ReadOnly: On` / `ReadOnly: Off`)
  - selection length + caret count (`Sel: <n> | Carets: <m>`)
  - indent mode (`Indent: Tabs/<n>` or `Indent: Spaces/<n>`)
  - document metrics (`Lines: <n> Len: <n>`)
  - language detection source (`extension` / `filename` / `shebang` / `default`) in language field
- Encoding now reflects actual file decode result on open (`usedEncoding`), not a fixed value.
- EOL mode now reflects detected file content line endings on open.
- Wired live updates from editor notifications (`SCN_UPDATEUI`) for cursor display.
- Reused existing language resolution and modified-state tracking from Phase 4B.
- Wired Scintilla read-only mode from file writability on open/save/new paths.

## Out of Scope (Not Yet Implemented)

- Multi-document/tab-aware status details.
- Interactive status controls.
- Find/replace integration or command widgets.

## Validation Notes

- Build pass:
  - `make -C build -j$(sysctl -n hw.ncpu)`
- Regression guard:
  - `./scripts/phase4b_auto_validate.sh`
  - latest known report: `build/reports/phase4b_auto_validate_20260305_220114.md` (`FINAL_RESULT=PASS`)

## Next 4C Steps

1. Add tab-aware status updates when multi-document support lands.
2. Add command/status affordances for future find/replace and navigation.
3. Add richer document metadata (file format details and language override state).
