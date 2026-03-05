# Phase 4B Auto Validation

- Timestamp: Thu Mar  5 21:07:17 AEST 2026
- Build: pass
- Scope: launch + lexer routing + log evidence

## Checks

- [x] Lexer routing observed: cpp
- [x] Lexer routing observed: python
- [x] Lexer routing observed: markdown
- [x] Lexer routing observed: hypertext
- [x] Lexer routing observed: xml
- [x] Lexer routing observed: sql
- [x] Lexer routing observed: bash
- [x] Lexer routing observed: rust
- [x] Lexer routing observed: yaml
- [x] Lexer routing observed: toml
- [ ] Edited/savepoint interaction check via keystroke automation
  - Blocked: `osascript` is not allowed to send keystrokes (System Events error 1002).

## Log Excerpt

```text
2026-03-05 20:58:59.306 Df notepadpp[57237:8ae20] [NPP][Lexer] <private> -> cpp
2026-03-05 20:58:59.400 Df notepadpp[57237:8ae20] [NPP][Lexer] <private> -> xml
2026-03-05 20:58:59.472 Df notepadpp[57237:8ae20] [NPP][Lexer] <private> -> yaml
2026-03-05 21:03:27.132 Df notepadpp[60234:8d30f] [NPP][Lexer] (none) -> null
2026-03-05 21:03:27.175 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> hypertext
2026-03-05 21:03:27.183 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> cpp
2026-03-05 21:03:27.227 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> markdown
2026-03-05 21:03:27.290 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> python
2026-03-05 21:03:27.369 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> rust
2026-03-05 21:03:27.457 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> bash
2026-03-05 21:03:27.517 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> sql
2026-03-05 21:03:27.600 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> toml
2026-03-05 21:03:27.666 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> cpp
2026-03-05 21:03:27.778 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> xml
2026-03-05 21:03:27.889 Df notepadpp[60234:8d30f] [NPP][Lexer] <private> -> yaml
2026-03-05 21:05:10.601 Df notepadpp[60789:8db7a] [NPP][Lexer] (none) -> null
2026-03-05 21:06:56.772 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> cpp
2026-03-05 21:06:56.823 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> hypertext
2026-03-05 21:06:56.910 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> cpp
2026-03-05 21:06:57.019 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> markdown
2026-03-05 21:06:57.078 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> python
2026-03-05 21:06:57.141 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> rust
2026-03-05 21:06:57.217 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> bash
2026-03-05 21:06:57.296 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> sql
2026-03-05 21:06:57.368 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> toml
2026-03-05 21:06:57.458 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> cpp
2026-03-05 21:06:57.545 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> xml
2026-03-05 21:06:57.609 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> yaml
2026-03-05 21:07:13.748 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> cpp
2026-03-05 21:07:13.823 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> hypertext
2026-03-05 21:07:13.892 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> cpp
2026-03-05 21:07:13.967 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> markdown
2026-03-05 21:07:14.045 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> python
2026-03-05 21:07:14.109 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> rust
2026-03-05 21:07:14.177 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> bash
2026-03-05 21:07:14.259 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> sql
2026-03-05 21:07:14.334 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> toml
2026-03-05 21:07:14.424 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> cpp
2026-03-05 21:07:14.550 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> xml
2026-03-05 21:07:14.624 Df notepadpp[60789:8db7a] [NPP][Lexer] <private> -> yaml
```
