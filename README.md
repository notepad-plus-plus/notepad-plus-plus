<p align="center">
  <img src="docs/assets/notequ-icon.png" alt="" width="80" height="80">
</p>

<h1 align="center">NoteQu</h1>

<p align="center"><strong>A text and code editor for Windows and macOS</strong></p>

<p align="center">
  Customize your menus, format text in place, and pick up where you left off with session recovery.
</p>

<p align="center">
  <a href="https://github.com/berkkivircik/NoteQu/releases/latest"><img src="https://img.shields.io/github/v/release/berkkivircik/NoteQu?style=flat-square&amp;label=stable&amp;color=176B80" alt="Latest stable release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-536071?style=flat-square" alt="License: GPLv3"></a>
  <a href="https://www.linkedin.com/in/berkkivircik/"><img src="https://img.shields.io/badge/LinkedIn-Berk%20Kivircik-0A66C2?style=flat-square&amp;logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyBmaWxsPSIjZmZmIiByb2xlPSJpbWciIHZpZXdCb3g9IjAgMCAyNCAyNCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48dGl0bGU%2BTGlua2VkSW48L3RpdGxlPjxwYXRoIGQ9Ik0yMC40NDcgMjAuNDUyaC0zLjU1NHYtNS41NjljMC0xLjMyOC0uMDI3LTMuMDM3LTEuODUyLTMuMDM3LTEuODUzIDAtMi4xMzYgMS40NDUtMi4xMzYgMi45Mzl2NS42NjdIOS4zNTFWOWgzLjQxNHYxLjU2MWguMDQ2Yy40NzctLjkgMS42MzctMS44NSAzLjM3LTEuODUgMy42MDEgMCA0LjI2NyAyLjM3IDQuMjY3IDUuNDU1djYuMjg2ek01LjMzNyA3LjQzM2MtMS4xNDQgMC0yLjA2My0uOTI2LTIuMDYzLTIuMDY1IDAtMS4xMzguOTItMi4wNjMgMi4wNjMtMi4wNjMgMS4xNCAwIDIuMDY0LjkyNSAyLjA2NCAyLjA2MyAwIDEuMTM5LS45MjUgMi4wNjUtMi4wNjQgMi4wNjV6bTEuNzgyIDEzLjAxOUgzLjU1NVY5aDMuNTY0djExLjQ1MnpNMjIuMjI1IDBIMS43NzFDLjc5MiAwIDAgLjc3NCAwIDEuNzI5djIwLjU0MkMwIDIzLjIyNy43OTIgMjQgMS43NzEgMjRoMjAuNDUxQzIzLjIgMjQgMjQgMjMuMjI3IDI0IDIyLjI3MVYxLjcyOUMyNCAuNzc0IDIzLjIgMCAyMi4yMjIgMGguMDAzeiIvPjwvc3ZnPg%3D%3D" alt="Berk Kivircik on LinkedIn"></a>
</p>

<p align="center">
  <strong><a href="#download-and-run">Download NoteQu</a></strong> &nbsp;·&nbsp;
  <a href="#features">Features</a> &nbsp;·&nbsp;
  <a href="CHANGELOG.md">Changelog</a> &nbsp;·&nbsp;
  <a href="BUILD.md">Build from source</a>
</p>

## Download and run

Get the latest stable version from [GitHub Releases](https://github.com/berkkivircik/NoteQu/releases/latest).
See the [download guide](DOWNLOADS.md) for package details and checksums.
If you're upgrading from NoteX, follow the [manual transition guide](UPDATING.md#from-notex-to-notequ)
to preserve your profile and formatting companions.

### Windows portable

On Windows x64, extract the ZIP to a writable folder and run `NoteQu.exe`. A clean
package starts with a fresh profile. The Windows binaries are unsigned.

### Native macOS

On Apple Silicon Macs running macOS 14 or later, open the DMG or extract the ZIP,
then copy `NoteQu.app` to Applications. The native app uses AppKit and Cocoa
Scintilla. It is ad hoc signed and has no Apple notarization. See the [macOS guide](macOS/README.md)
for native features and [compatibility limits](macOS/README.md#compatibility-limits).

## Features

The Windows portable edition includes a redesigned toolbar and the following editing features. See the [macOS guide](macOS/README.md) for native workflows and settings.

- Apply fonts, sizes, bold, italic, underline, strikethrough, and colors to text ranges.
- Set the format for text you type next, copy formatting, and create lists.
- Use line and caret heights that follow the formatting on each line.
- Reorder and hide menu items with persistent menu customization.
- On Windows, use the ellipsis on the formatting toolbar to show, hide, reorder,
  or restore its items. NoteQu saves the layout in your profile.
- Recover unsaved text and formatting through session snapshots.
- Use English, Turkish, or Spanish interface translations; syntax highlighting
  support is independent of the interface language.

Formatting is stored separately in `.notex` companion files. Previous portable-release companions and menu settings are migrated without
resetting document formatting or menu preferences.
The Windows package uses GDI rendering and ships with the NoteQu updater. The upstream Notepad++ updater and optional third-party plugins are excluded from the clean package.

## Source and build

NoteQu is an independent fork of Notepad++ 8.9.8.1 maintained by Berk Kivircik.
This is the active repository for development, issues, signing preparation, and
releases. See [source provenance](SOURCE_PROVENANCE.md) for the upstream base
and verification steps.

The repository includes the modified editor, Scintilla, Lexilla, Boost.Regex,
build scripts, tests, and portable packaging inputs. See [BUILD.md](BUILD.md).
Each public release identifies its matching source tag and archive. Use the exact source revision for any binary you redistribute.

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

The first upgrade from NoteX (before 1.7.0) is manual on both platforms. See [From NoteX to NoteQu](UPDATING.md#from-notex-to-notequ) for the platform-specific steps.

On Windows, follow these steps to preserve your existing portable profile.

Close the application normally and back up the complete existing directory.
Keep the same installation path. Preserve `config.xml`, `session.xml*`, `backup/`,
`.notex` companions, menu settings, user shortcuts, translations, and customized
syntax/theme files. Replace application files without deleting the profile.
The fresh-install ZIP contains default configuration; do not overwrite a user's
configuration with those defaults. Earlier companion filenames are accepted
and migrated when the corresponding document is loaded. A current companion is
preferred when both versions exist; conflicting files are never overwritten.

If an existing Windows shortcut still shows the old name, rename it to `NoteQu`
and confirm that it points to the current `NoteQu.exe`.

## Updates and code signing

NoteQu checks GitHub for updates automatically by default. Use **Automatic Update Checks** in the Windows Help menu or macOS NoteQu menu to turn off background checks. Manual checks remain available on both platforms.

The updater verifies a signed inventory before installing application files and preserves user profiles and documents. See [Updating NoteQu](UPDATING.md) for installation and recovery, the [code signing policy](CODE_SIGNING.md) for platform signing status, and the [privacy policy](PRIVACY.md) for network and local storage behavior.
