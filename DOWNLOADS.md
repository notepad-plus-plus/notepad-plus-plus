# Download NoteX

NoteX is an independent text and code editor with inline formatting and recovery for unsaved documents. The Windows edition is based on Notepad++; the native Apple Silicon edition uses AppKit and Cocoa Scintilla.

Get the current packages from [GitHub Releases](https://github.com/berkkivircik/NoteX/releases/latest). The [1.6.2 release](https://github.com/berkkivircik/NoteX/releases/tag/v1.6.2) includes:

- Windows x64: `NoteX-1.6.2-portable-x64.zip`. Extract to a new writable directory and run `NoteX.exe`.
- macOS Apple Silicon: `NoteX-1.6.2-macos-arm64.dmg` or the matching ZIP. Copy `NoteX.app` to Applications. macOS 14 or later is required.
- Matching source, update payloads, signed update inventories, and SHA-256 checksums.

Read the [privacy policy](PRIVACY.md) before first launch. Automatic GitHub update checks are enabled by default and can be disabled in the application's **Automatic Update Checks** menu. For an existing installation, follow [Updating NoteX](UPDATING.md); never overlay personal settings with the clean package's defaults.

## Code signing policy

Read the [Code signing policy](CODE_SIGNING.md). Windows 1.6.2 is unsigned and may display SmartScreen warnings. macOS 1.6.2 is ad-hoc signed and is not Apple notarized. The separate update-manifest signature does not replace either platform's publisher identity checks.

NoteX is preparing an application to SignPath Foundation for free open-source Windows signing. The project has not been approved, and no current release is represented as signed by SignPath Foundation. Required provider attribution will be added only after approval and actual signing.

## Remove NoteX

Save unsaved work and close the application. On Windows, preserve personal files and profiles before removing application files or the portable directory. On macOS, remove `NoteX.app`; your separate Application Support profile remains until you choose to remove it. See [Privacy policy: removal](PRIVACY.md#removal-and-questions) for details.
