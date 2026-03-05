# Phase 4B Auto Validation

- Timestamp: Thu Mar  5 22:01:18 AEST 2026
- Build: PASS
- Validation Mode: internal self-test (no Accessibility automation)
- Report: /Users/samsin/Documents/Antigravity_Folder/notepad-plus-plus_macos/build/reports/phase4b_auto_validate_20260305_220114.md

## Machine Summary (PASS/FAIL Matrix)

| Sample | Expected Lexer | Expected Source | Expected EOL | Expected Encoding | Actual Lexer | Actual Source | Actual EOL | Actual Encoding | Status Snapshot | Lexer Routing | Source Routing | EOL Routing | Encoding Routing | 0->1->0 Transition | SelfTest Tag | Exit Code | Result |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| sample.cpp | cpp | extension | LF | UTF-8 | cpp | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.py | python | extension | LF | UTF-8 | python | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.js | cpp | extension | LF | UTF-8 | cpp | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.ts | cpp | extension | LF | UTF-8 | cpp | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.html | hypertext | extension | LF | UTF-8 | hypertext | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.xml | xml | extension | LF | UTF-8 | xml | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.sql | sql | extension | LF | UTF-8 | sql | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.sh | bash | extension | LF | UTF-8 | bash | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.rs | rust | extension | LF | UTF-8 | rust | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.toml | toml | extension | LF | UTF-8 | toml | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.yaml | yaml | extension | LF | UTF-8 | yaml | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.md | markdown | extension | LF | UTF-8 | markdown | extension | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| script_python | python | shebang | LF | UTF-8 | python | shebang | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| CMakeLists.txt | cmake | filename | LF | UTF-8 | cmake | filename | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Makefile | makefile | filename | LF | UTF-8 | makefile | filename | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| plain_text_note | null | default | LF | UTF-8 | null | default | LF | UTF-8 | encoding:UTF-8;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample_crlf.txt | null | extension | CRLF | UTF-8 | null | extension | CRLF | UTF-8 | encoding:UTF-8;eol:CRLF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sample_utf16.txt | null | extension | LF | UTF-16 | null | extension | LF | UTF-16 | encoding:UTF-16;eol:LF;readonly:0 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |

FINAL_RESULT=PASS

## Human Checklist

- [ ] Visual syntax quality checked for: C++, Python, JS/TS, HTML/XML, SQL, Bash, Rust, Markdown, JSON, YAML, TOML
- [ ] Window title language indicator checked on file open
- [ ] Status bar language + Edited/Saved state checked manually

## Notes

- Automated Phase 4B gate requires FINAL_RESULT=PASS.
- Full Phase 4B completion additionally requires all manual checklist items marked done.
