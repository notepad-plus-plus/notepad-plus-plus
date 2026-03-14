# npminmin

A secure, portable fork of the classic source code editor — with the auto-updater stripped out entirely.

## Why this fork?

In 2025, state-sponsored attackers compromised the Notepad++ update infrastructure (WinGup/GUP.exe),
delivering trojanized installers to targeted users for months. The attack surface was the auto-updater.

**npminmin removes that attack surface by design:**

- No WinGup, no GUP.exe, no auto-updater of any kind
- No Plugin Admin marketplace — no plugin list fetched from the internet  
- No outbound network calls from the editor or any bundled plugin
- Portable-only: runs from a folder, no installer, no registry writes
- Curated, frozen plugin set vendored directly in the repo at known-good versions
- Air-gap safe — works with zero internet access

## Portable Usage

1. Download the latest release zip from [Releases](https://github.com/ridermw/np-minus-minus/releases)
2. Extract anywhere — USB drive, network share, local folder
3. Run `npminmin.exe`
4. Config and session data are stored next to the executable (portable mode)

No admin rights required. No registry entries. Delete the folder to uninstall completely.

## Bundled Plugins (frozen, no network)

| Plugin | Version | Purpose |
|--------|---------|---------|
| ComparePlus | v2.2.0 | Side-by-side file diff |
| JsonViewer | v2.1.1.0 | JSON tree view and formatter |
| HexEditor | v0.9.14 | Binary/hex file editing |
| XMLTools | v3.1.1.13 | XML format, validate, XPath |
| NppExec | v0.8.10 | Run shell commands from editor |
| CSVLint | v0.4.7 | CSV/TSV validation and SQL export |
| Explorer | v1.8.2.32 | Dockable file browser panel |
| MultiReplace | v5.0.0.35 | Batch find/replace with lists |
| DSpellCheck | v1.5.0 | Spell checker (Hunspell) |
| PythonScript | v2.1.0 | Python scripting engine |
| ColumnTools | v1.4.5.1 | Column ruler for fixed-width data |
| MarkdownViewerPlusPlus | v0.8.2 | Live Markdown preview |
| NppExport | bundled | Export code to RTF/HTML |
| mimeTools | bundled | Base64/URL encode-decode |
| NppConverter | bundled | Hex/binary/decimal conversion |

Plugin Admin is **disabled** — no marketplace, no plugin list network fetch.
To add plugins manually, place a `PluginName/PluginName.dll` folder in the `plugins/` directory.

## Security Properties

- Zero outbound network connections from editor or plugins
- No code signing infrastructure tied to third-party servers
- Plugin set auditable: all DLLs are committed at fixed versions
- Suitable for air-gapped and high-security environments
- No auto-update mechanism that could be compromised

## Building

See [BUILD.md](BUILD.md) for build instructions. Requires Visual Studio 2022 or later.

The build produces `npminmin.exe` (x64). The `doLocalConf.xml` file in the same directory
activates portable mode automatically.

## License

GNU General Public License v3 — see [LICENSE](LICENSE).

Based on [Notepad++](https://github.com/notepad-plus-plus/notepad-plus-plus) by Don HO.
