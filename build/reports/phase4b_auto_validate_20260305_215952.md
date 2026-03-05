# Phase 4B Auto Validation

- Timestamp: Thu Mar  5 21:59:56 AEST 2026
- Build: PASS
- Validation Mode: internal self-test (no Accessibility automation)
- Report: /Users/samsin/Documents/Antigravity_Folder/notepad-plus-plus_macos/build/reports/phase4b_auto_validate_20260305_215952.md

## Machine Summary (PASS/FAIL Matrix)

| Sample | Expected Lexer | Expected Source | Expected EOL | Actual Lexer | Actual Source | Actual EOL | Status Snapshot | Lexer Routing | Source Routing | EOL Routing | 0->1->0 Transition | SelfTest Tag | Exit Code | Result |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| sample.cpp | cpp | extension | LF | cpp | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.py | python | extension | LF | python | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.js | cpp | extension | LF | cpp | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.ts | cpp | extension | LF | cpp | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.html | hypertext | extension | LF | hypertext | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.xml | xml | extension | LF | xml | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.sql | sql | extension | LF | sql | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.sh | bash | extension | LF | bash | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.rs | rust | extension | LF | rust | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.toml | toml | extension | LF | toml | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.yaml | yaml | extension | LF | yaml | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.md | markdown | extension | LF | markdown | extension | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| script_python | python | shebang | LF | python | shebang | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| CMakeLists.txt | cmake | filename | LF | cmake | filename | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Makefile | makefile | filename | LF | makefile | filename | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| plain_text_note | null | default | LF | null | default | LF | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample_crlf.txt | null | extension | CRLF | null | extension | CRLF | encoding:UTF-8;eol:CRLF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |

FINAL_RESULT=PASS

## Human Checklist

- [ ] Visual syntax quality checked for: C++, Python, JS/TS, HTML/XML, SQL, Bash, Rust, Markdown, JSON, YAML, TOML
- [ ] Window title language indicator checked on file open
- [ ] Status bar language + Edited/Saved state checked manually

## Notes

- Automated Phase 4B gate requires FINAL_RESULT=PASS.
- Full Phase 4B completion additionally requires all manual checklist items marked done.
