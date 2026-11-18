# Third-party notices

NoteX is an independent modified distribution of Notepad++ 8.9.8.1.
Original copyright, attribution, license, and warranty notices remain in the
source files. The work as a whole is distributed under GNU GPL version 3;
separately licensed components retain their original permissions and conditions.

| Component | Distributed location | License and attribution |
| --- | --- | --- |
| Notepad++ | PowerEditor | GNU GPL v3; Don HO and contributors. See LICENSE and LICENSES/Notepad-plus-plus-LICENSE.txt. |
| Scintilla | scintilla | Neil Hodgson and contributors; permissive Scintilla license. See LICENSES/Scintilla.txt. NoteX modifications are identified separately. |
| Lexilla | lexilla | Neil Hodgson and contributors; permissive Scintilla/Lexilla license. See LICENSES/Lexilla.txt. |
| Boost.Regex and included Boost headers | boostregex | Boost Software License 1.0. See LICENSES/Boost-1.0.txt and original file notices. |
| pugixml | PowerEditor/src/pugixml | MIT; Arseny Kapoulkine and contributors. See LICENSES/pugixml-MIT.txt. |
| JSON for Modern C++ 3.12.0 | PowerEditor/src/json | MIT; Niels Lohmann. See LICENSES/nlohmann-json-MIT.txt. |
| uchardet / Mozilla universal charset detector | PowerEditor/src/uchardet | Original MPL 1.1 / GPL 2.0-or-later / LGPL 2.1-or-later notices are preserved. This combined distribution uses the GPL option under GPLv3. |
| Windows dark-mode menu helpers | PowerEditor/src/DarkMode | MIT; Adam D. Walling and PolyHook contributors. See LICENSES/DarkMode-MIT.txt and LICENSES/PolyHook-2.0-MIT.txt. |
| MD5 implementation | PowerEditor/src/MISC/md5 | RSA Data Security, Inc. MD5 Message-Digest Algorithm, including derived implementation. Original RSA notice is included in LICENSES/BUNDLED_SOURCE_NOTICES.txt. |
| SHA implementations, dark-mode helpers, and other embedded contributions | PowerEditor/src | See the authors' original per-file notices, also collected in LICENSES/BUNDLED_SOURCE_NOTICES.txt. |
| Markdown-plus-plus UDLs | PowerEditor/Packaging/NoteX/assets/userDefineLangs | MIT; Edditoria. See LICENSES/Markdown-plus-plus-MIT.txt. |
| Theme resources | PowerEditor/installer/themes and the packaged NoteX.xml | Original theme authors retain their rights. DarkModeDefault, Zenburn, and the derived NoteX palette use GPL v2. Other themes carry their embedded MIT, CC BY 3.0, or other upstream notices. See LICENSES/THEME_NOTICES.txt, LICENSES/GPL-2.0.txt, and LICENSES/CC-BY-3.0.txt. |
| Inter 4.001 | PowerEditor/Packaging/NoteX/assets/fonts/Inter.ttf | Copyright 2016 The Inter Project Authors; SIL Open Font License 1.1. Complete notice beside the font. |
| JetBrains Mono 2.211 | PowerEditor/Packaging/NoteX/assets/fonts/JetBrainsMono.ttf | Copyright 2020 The JetBrains Mono Project Authors; SIL Open Font License 1.1. Complete notice beside the font. |

The fonts are unmodified bundled resources. Their copyright and full OFL texts
are supplied beside the fonts in both source and portable distributions. The
OFL does not apply to documents created with those fonts.

The native macOS binary links the modified Scintilla Cocoa backend and Lexilla
and compiles the included Boost.Regex headers. It also bundles the Notepad++
Function List XML rules, retaining their original notices.
For Cocoa files offered under either LGPL v2.1 or the Scintilla license, this
distribution uses the Scintilla license option. The original Oracle, Sun,
Mike Lischke, Evan Jones, Neil Hodgson, and other contributor notices remain
in their source files. Cocoa, Core Text, Quartz Core, and CommonCrypto are
provided by the operating system and are not copied into the application.
The macOS application bundle does not contain the Windows PowerEditor binary,
pugixml, JSON for Modern C++, or Windows dark-mode helpers.

LICENSES/BUNDLED_SOURCE_NOTICES.txt preserves additional license and copyright
comment blocks verbatim. It supplements the source notices and complete license
files; it does not replace or relicense them. Some source-only tools and tests
have their own notices, which remain in the corresponding source directories.

Optional third-party plugin DLLs, plugin catalogs, and the upstream updater are
not included in the public portable package. Their source and distribution
obligations must be reviewed before adding them to a future release.

Notepad++ and other project names identify the original projects. No endorsement
of NoteX by those projects or their authors is implied.

Theme XML files and fonts are separate data resources bundled with the GPLv3
editor executable. Their licenses are not replaced by the editor license.
The upstream DansLeRuSH-Dark theme is excluded because its header also includes
noncommercial terms. The public package does not impose those terms on NoteX.

## Go runtime and standard library

The NoteX updater is built with Go and includes its runtime and standard library. Copyright The Go Authors. BSD-style license: LICENSES/Go.txt. The offline release signing tool additionally requires Python cryptography; it is a development dependency and is not embedded in NoteX.
