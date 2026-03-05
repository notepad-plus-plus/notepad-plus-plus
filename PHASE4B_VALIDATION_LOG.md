# Phase 4B Validation Log

## 2026-03-05 20:59:05 AEST

- Build verification: `make -C build -j$(sysctl -n hw.ncpu)` passed.
- Smoke sample generation: `./scripts/phase4b_smoke.sh` completed.
- App launch requests issued for sample files:
  - `sample.cpp`, `sample.py`, `sample.js`, `sample.ts`, `sample.html`, `sample.xml`,
    `sample.sql`, `sample.sh`, `sample.rs`, `sample.toml`, `sample.yaml`, `sample.md`.
- Report template generated:
  - `build/reports/phase4b_smoke_20260305_205857.md`

Manual result entry pending:
- Fill checkbox outcomes in the generated report after visual verification inside the app.

## 2026-03-05 21:03:00 AEST (Automated)

- Ran smoke launch:
  - `./scripts/phase4b_smoke.sh`
- Collected lexer-routing evidence from unified logs:
  - `/usr/bin/log show --style compact --last 10m --predicate 'process == "notepadpp" AND eventMessage CONTAINS "[NPP][Lexer]"'`
- Generated automated validation report:
  - `build/reports/phase4b_smoke_auto_20260305_2103.md`

Automated status:
- PASS for file-open and lexer-assignment routing.
- Manual UI checks still required for visual styling quality and edited/savepoint interactions.

## 2026-03-05 21:07:17 AEST (Auto Script)

- Ran: `./scripts/phase4b_auto_validate.sh`
- Generated report: `build/reports/phase4b_auto_validate_20260305_210713.md`
- Result: lexer routing checks PASS, interaction checks blocked by macOS keystroke permission.

## 2026-03-05 21:13:37 AEST (Self-Test Automation)

- Ran: `./scripts/phase4b_auto_validate.sh`
- Validation mode: in-app self-test (`NPP_SELFTEST=1`) with per-file deterministic log parsing.
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_211330.md`
- Automated result:
  - `FINAL_RESULT=PASS`
  - lexer routing matrix: PASS for all smoke samples
  - edited/savepoint transition matrix (`0 -> 1 -> 0`): PASS for all smoke samples
- Remaining for full 4B sign-off:
  - complete manual visual checklist items in report.

## 2026-03-05 21:17:13 AEST (Post-4C-Foundation Pass)

- Rebuilt after status bar cursor update:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
- Re-ran automation:
  - `./scripts/phase4b_auto_validate.sh`
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_211713.md`
- Automated result:
  - `FINAL_RESULT=PASS`
  - no regression in lexer routing or edited/savepoint transition checks.

## 2026-03-05 21:20:14 AEST (Strict Exit-Code Gate)

- Updated self-test app behavior:
  - `NPP_SELFTEST=1` now exits process with code `0` on pass and `1` on fail.
- Updated validator:
  - requires both `[NPP][SelfTest] PASS` tag and process exit code pass.
  - matrix now includes `Exit Code` column.
- Rebuilt and re-ran:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_212014.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:21:02 AEST (Post-Doc Sanity Run)

- Ran: `./scripts/phase4b_auto_validate.sh`
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_212102.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:21:36 AEST (4C Encoding/EOL Pass)

- Added read-only status indicators in 4C bar:
  - encoding (`UTF-8`)
  - EOL mode (`LF` / `CRLF` / `CR`)
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_212136.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:23:16 AEST (4C Read-Only Indicator Pass)

- Added read-only status indicator in 4C bar and wired Scintilla read-only mode to file writability.
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_212316.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:24:19 AEST (4C Selection-Length Pass)

- Added selection-length status indicator (`Sel: <n>`) in 4C status bar.
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_212419.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:25:09 AEST (4C Multi-Caret Status Pass)

- Expanded selection indicator to include caret count:
  - `Sel: <n> | Carets: <m>`
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_212509.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:30:31 AEST (4C Indent Status Pass)

- Added indent status indicator:
  - `Indent: Tabs/<n>` or `Indent: Spaces/<n>`
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_213031.md`
- Automated result:
  - `FINAL_RESULT=PASS`
