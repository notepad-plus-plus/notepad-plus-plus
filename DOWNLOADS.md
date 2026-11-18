# Download NoteQu

NoteQu is an independent text and code editor with inline formatting and recovery for unsaved documents. The Windows edition is based on Notepad++; the native Apple Silicon edition uses AppKit and Cocoa Scintilla.

Get the current build from [GitHub Releases](https://github.com/berkkivircik/NoteQu/releases/latest).

- **Windows x64:** Download the portable ZIP, extract it to a writable folder, and run `NoteQu.exe`.
- **macOS Apple Silicon:** Download the DMG or ZIP, then copy `NoteQu.app` to Applications. macOS 14 or later is required.

The release includes the matching source archive, signed update inventories, and SHA-256 checksums. Packages published before the rename are still available from their [original releases](https://github.com/berkkivircik/NoteQu/releases).

To build NoteQu, follow [BUILD.md](BUILD.md) or the [macOS guide](macOS/README.md).

Read the [privacy policy](PRIVACY.md) before first launch. Automatic GitHub update checks are enabled by default and can be disabled in the application's **Automatic Update Checks** menu. For an existing installation, follow [Updating NoteQu](UPDATING.md); never overlay personal settings with the clean package's defaults.

## Code signing policy

Read the [Code signing policy](CODE_SIGNING.md). The current Windows package is unsigned and may display SmartScreen warnings. The current macOS package uses an ad hoc signature and has no Apple notarization. The separate update-manifest signature does not replace either platform's publisher identity checks.

NoteQu is preparing an application to SignPath Foundation for free open-source Windows signing. The project has not been approved, and no current release is represented as signed by SignPath Foundation. Required provider attribution will be added only after approval and actual signing.

## Remove NoteQu

Save unsaved work and close the application. On Windows, preserve personal files and profiles before removing application files or the portable directory. On macOS, remove `NoteQu.app`; your separate Application Support profile remains until you choose to remove it. See [Privacy policy: removal](PRIVACY.md#removal-and-questions) for details.
