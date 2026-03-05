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

## 2026-03-05 21:51:16 AEST (4C Metrics Status Pass)

- Added document metrics status indicator:
  - `Lines: <n> Len: <n>`
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_215116.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:52:10 AEST (4C Language-Source Metadata Pass)

- Added language detection source metadata to status bar language field:
  - `extension` / `filename` / `shebang` / `default`
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_215210.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:53:07 AEST (Expanded Inference Matrix - Failure Found)

- Expanded automated sample set to include:
  - `script_python` (shebang inference)
  - `CMakeLists.txt` (filename inference)
  - `Makefile` (filename inference)
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` completed with failure
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_215303.md`
- Result:
  - `FINAL_RESULT=FAIL`
  - root cause: `CMakeLists.txt` was detected as extension `txt` before filename-special-case mapping.

## 2026-03-05 21:53:30 AEST (Expanded Inference Matrix - Fixed)

- Fixed normalization order to prioritize filename-special cases before generic extension extraction.
- Rebuilt and re-ran:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_215330.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:54:38 AEST (Source-Routing Gate Added)

- Extended self-test log contract:
  - `[NPP][SelfTest] LANG_SOURCE=<extension|filename|shebang|default>`
- Extended validator matrix with source routing columns:
  - `Expected Source`
  - `Actual Source`
  - `Source Routing`
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_215433.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:55:24 AEST (Default-Route Coverage Added)

- Added extensionless plain-text sample (`plain_text_note`) to validate default route:
  - expected lexer: `null`
  - expected source: `default`
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_215524.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:57:23 AEST (4C File-Format Metadata Pass)

- Added dynamic file-format metadata behavior:
  - status encoding now reflects decoded file encoding on open
  - status EOL now reflects line-ending mode detected from opened content
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_215723.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 21:59:00 AEST (Status Snapshot + EOL Gate Pass)

- Extended self-test report matrix with status snapshot metadata:
  - `encoding:<...>;eol:<...>;readonly:<...>`
- Added CRLF sample (`sample_crlf.txt`) and enforced EOL routing in automated gate:
  - expected vs actual EOL now checked per sample.
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_215952.md`
- Automated result:
  - `FINAL_RESULT=PASS`

## 2026-03-05 22:01:14 AEST (Encoding Gate Added)

- Added UTF-16 sample (`sample_utf16.txt`) to automated matrix.
- Added encoding routing checks from self-test status snapshot:
  - expected vs actual encoding now gated per sample.
- Rebuilt and validated:
  - `make -C build -j$(sysctl -n hw.ncpu)` passed
  - `./scripts/phase4b_auto_validate.sh` passed
- Generated report:
  - `build/reports/phase4b_auto_validate_20260305_220114.md`
- Automated result:
  - `FINAL_RESULT=PASS`
