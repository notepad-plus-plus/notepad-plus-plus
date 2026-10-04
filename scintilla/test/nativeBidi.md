# Native Windows bidirectional regression test

`nativeBidi.cxx` links to the same static Scintilla library as Notepad++.
It creates a hidden control and renders through DirectWriteDC to a memory
bitmap using `WM_PRINTCLIENT`. It does not show a desktop window or operate
an existing application.

Build after building Notepad++ with MinGW/Clang (from the repository root):

```powershell
clang++ -std=c++17 -O2 -static -I scintilla/include scintilla/test/nativeBidi.cxx PowerEditor/gcc/bin.clang.x86_64.build/libscintilla.a -luser32 -lgdi32 -limm32 -lole32 -luuid -loleaut32 -ladvapi32 -o nativeBidi.exe
./nativeBidi.exe
```

Adjust the library path for another build configuration or use the library
from `scintilla/win32`. A Windows display session and DirectWrite are required.

Coverage:

- Seven mixed Hebrew/English samples, both control directions and all three
  painting phases: 42 combinations, including numbers, parentheses, tabs,
  Hebrew combining marks and emoji.
- Selection must preserve the positions of the rendered glyphs, including
  artificial syntax-style boundaries with identical font attributes.
- Selection backgrounds must agree with native character hit testing.
  The selected text must match the logical source substring for copying.
  A one-pixel band around fractional cluster boundaries is excluded because
  backgrounds snap to pixels. Mouse selection endpoints use shaped clusters.
- Wrapped and multiline selection with CRLF, indentation and empty lines.
- Visual left/right movement through Hebrew clusters and row edges.
- Empty RTL caret placement and absence of `WS_EX_LAYOUTRTL` mirroring.

Passing an argument skips the 42-combination matrix and runs the additional
wrapping/caret tests. The normal invocation runs everything.

The unmodified renderer fails the selection/glyph-position regression on
`abc אבג def`. These tests do not claim Word-equivalent behavior for every
editing command. Ctrl+word movement, rectangular selection, virtual space,
paragraph bidi levels across wrapped rows, caret affinity at run boundaries,
long RTL horizontal scrolling and non-Windows platforms need separate review.
- Absolute painted glyph order for four samples, including the Hebrew-first
  `כתוב נכון ABC אני עכשיו בודק אם` in LTR, in Segoe UI, Consolas and
  Courier New, both directions and all painting phases. Distinct glyph colours
  are located in the bitmap and checked against a known visual sequence;
  this assertion does not derive its expectation from Scintilla hit tests.
- First-strong paragraph direction for Unicode 16.0.0, including number/emoji
  prefixes, supplementary scripts, direction marks and nested isolates.
  Wrapped rows inherit direction from the complete logical paragraph.
- The previous native renderer fails the exact Hebrew-first LTR sentence's
  painted-order test. The changed renderer passes 352,438 assertions.
