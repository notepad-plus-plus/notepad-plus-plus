# Phase 4B Smoke Report (Automated)

- Timestamp: 2026-03-05 21:03 AEST
- Method: script launch + macOS unified log parsing (`[NPP][Lexer]` messages)
- Command run:
  - `./scripts/phase4b_smoke.sh`
  - `/usr/bin/log show --style compact --last 10m --predicate 'process == "notepadpp" AND eventMessage CONTAINS "[NPP][Lexer]"'`

## Automated Checks

- [x] App launched and processed all smoke sample open requests.
- [x] Lexer assignment observed: `hypertext` (HTML sample).
- [x] Lexer assignment observed: `cpp` (C++ and JS/TS path).
- [x] Lexer assignment observed: `markdown`.
- [x] Lexer assignment observed: `python`.
- [x] Lexer assignment observed: `rust`.
- [x] Lexer assignment observed: `bash`.
- [x] Lexer assignment observed: `sql`.
- [x] Lexer assignment observed: `toml`.
- [x] Lexer assignment observed: `xml`.
- [x] Lexer assignment observed: `yaml`.

## Not Verifiable Non-Interactively

- [ ] Visual color rendering quality per lexer (requires app UI observation).
- [ ] Title `Edited` state transitions during typing/save.
- [ ] Undo/redo savepoint behavior from user interactions.

## Evidence (excerpt)

Observed log lines include:
- `[NPP][Lexer] ... -> hypertext`
- `[NPP][Lexer] ... -> cpp`
- `[NPP][Lexer] ... -> markdown`
- `[NPP][Lexer] ... -> python`
- `[NPP][Lexer] ... -> rust`
- `[NPP][Lexer] ... -> bash`
- `[NPP][Lexer] ... -> sql`
- `[NPP][Lexer] ... -> toml`
- `[NPP][Lexer] ... -> xml`
- `[NPP][Lexer] ... -> yaml`

## Result

Automated validation: **PASS** for lexer routing and open-path smoke.
Manual UI validation still required for visual styling and edited/savepoint UX behavior.
