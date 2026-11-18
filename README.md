# NoteQu

NoteQu was previously released as NoteX.

NoteQu is a portable Windows text and code editor based on Notepad++ 8.9.8.1.
It adds visual text formatting, a redesigned
toolbar, customizable menus, and formatting-aware session recovery.
This is an independent modified distribution maintained by Berk Kivircik.

[NoteQu](https://github.com/berkkivircik/NoteQu) is a GitHub fork of Notepad++ and
the single active repository for development, signing preparation, issues, and
releases. See [source provenance](SOURCE_PROVENANCE.md) for the upstream base,
the transparent history transition, and verification steps.

## See [Downloads](DOWNLOADS.md) for published binaries. A NoteQu Windows build
runs as `NoteQu.exe` on Windows x64 from a writable folder, without an installer.
The portable package starts with a fresh profile and no documents or session history.

See the [download guide](DOWNLOADS.md), [Code signing policy](CODE_SIGNING.md),
and [privacy policy](PRIVACY.md) before first launch. Automatic GitHub update
checks are enabled by default and can be disabled in the application's
**Automatic Update Checks** menu.

Windows shortcuts have their own display names. If a shortcut still shows an
older application name, rename the shortcut to `NoteQu` and verify that its target
is your current `NoteQu.exe`. Updating the EXE does not rename existing shortcuts.

### Native macOS

The native Apple Silicon edition runs as `NoteQu.app`.
It uses AppKit and Cocoa Scintilla, without Wine or Rosetta. Requires macOS 14
or later. See [the macOS guide](macOS/README.md) for installation, recovery,
build instructions, native workbench features, and platform compatibility.
This edition is ad-hoc signed and is not notarized by Apple.

## Features

- Apply fonts, sizes, bold, italic, underline, strikethrough, and colors to text ranges.
- Set the format for text you type next, copy formatting, and create lists.
- Use line and caret heights that follow the formatting on each line.
- Reorder and hide menu items with persistent menu customization.
- Recover unsaved text and formatting through session snapshots.
- Use English, Turkish, or Spanish interface translations; syntax highlighting
  support is independent of the interface language.

Formatting is stored separately in `.notex` companion files. Previous portable-release companions and menu settings are migrated without
resetting document formatting or menu preferences.
The package uses GDI rendering and disables upstream automatic updates.
Optional third-party plugins and the upstream updater are not bundled in this
public release. They are not required for the editor's built-in features.

## Source and build

The repository includes the modified editor, Scintilla, Lexilla, Boost.Regex,
build scripts, tests, and portable packaging inputs. See [BUILD.md](BUILD.md).
Each public release identifies its matching source tag and archive.
The historical 1.6.2 binaries use the `v1.6.2` tag and
`NoteX-1.6.2-source.zip`. Use the exact source revision for the binary you redistribute.

## License and attribution

NoteQu is distributed under the **GNU General Public License, version 3**.
See [LICENSE](LICENSE) for the complete license. The original Notepad++ license
and copyright notice are preserved in [LICENSES/Notepad-plus-plus-LICENSE.txt](LICENSES/Notepad-plus-plus-LICENSE.txt).
NoteQu modifications: Copyright (C) 2026 Berk Kivircik.
Original Notepad++ copyrights, including Don HO's, are preserved.
See [MODIFICATIONS.md](MODIFICATIONS.md), [CHANGELOG.md](CHANGELOG.md),
and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

You may use NoteQu commercially, study and modify its source, and redistribute
it under the applicable license terms. No additional license fee or permission
from the NoteQu maintainer is required. The software is provided without warranty.
Separate components and fonts retain their own license terms.

NoteQu is not an official Notepad++ release and is not endorsed by the upstream
project. Report NoteQu issues in the [canonical issue tracker](https://github.com/berkkivircik/NoteQu/issues). Upstream documentation is
retained in the `UPSTREAM_*.md` files for reference.

## Upgrading an existing portable installation

The first upgrade from NoteX to NoteQu is manual. See [Updating NoteQu](UPDATING.md#from-notex-to-notequ).

Close the application normally and back up the complete existing directory.
Keep the same installation path. Preserve `config.xml`, `session.xml*`, `backup/`,
`.notex` companions, menu settings, user shortcuts, translations, and customized
syntax/theme files. Replace application files without deleting the profile.
The fresh-install ZIP contains default configuration; do not overwrite a user's
configuration with those defaults. Earlier companion filenames are accepted
and migrated when the corresponding document is loaded. A current companion is
preferred when both versions exist; conflicting files are never overwritten.

## Updates and code signing

See [Updating NoteQu](UPDATING.md) for authenticated updates, first-install migration, data preservation, and recovery. See the [Code signing policy](https://github.com/berkkivircik/NoteQu/blob/main/CODE_SIGNING.md) for the separate Windows publisher and macOS signing status.
