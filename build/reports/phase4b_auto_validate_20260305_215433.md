# Phase 4B Auto Validation

- Timestamp: Thu Mar  5 21:54:38 AEST 2026
- Build: PASS
- Validation Mode: internal self-test (no Accessibility automation)
- Report: /Users/samsin/Documents/Antigravity_Folder/notepad-plus-plus_macos/build/reports/phase4b_auto_validate_20260305_215433.md

## Machine Summary (PASS/FAIL Matrix)

| Sample | Expected Lexer | Expected Source | Actual Lexer | Actual Source | Lexer Routing | Source Routing | 0->1->0 Transition | SelfTest Tag | Exit Code | Result |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| sample.cpp | cpp | extension | cpp | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.py | python | extension | python | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.js | cpp | extension | cpp | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.ts | cpp | extension | cpp | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.html | hypertext | extension | hypertext | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.xml | xml | extension | xml | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.sql | sql | extension | sql | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.sh | bash | extension | bash | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.rs | rust | extension | rust | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.toml | toml | extension | toml | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.yaml | yaml | extension | yaml | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| sample.md | markdown | extension | markdown | extension | PASS | PASS | PASS | PASS | PASS | PASS |
| script_python | python | shebang | python | shebang | PASS | PASS | PASS | PASS | PASS | PASS |
| CMakeLists.txt | cmake | filename | cmake | filename | PASS | PASS | PASS | PASS | PASS | PASS |
| Makefile | makefile | filename | makefile | filename | PASS | PASS | PASS | PASS | PASS | PASS |

FINAL_RESULT=PASS

## Human Checklist

- [ ] Visual syntax quality checked for: C++, Python, JS/TS, HTML/XML, SQL, Bash, Rust, Markdown, JSON, YAML, TOML
- [ ] Window title language indicator checked on file open
- [ ] Status bar language + Edited/Saved state checked manually

## Notes

- Automated Phase 4B gate requires FINAL_RESULT=PASS.
- Full Phase 4B completion additionally requires all manual checklist items marked done.
