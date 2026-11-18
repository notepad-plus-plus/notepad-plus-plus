# Building and packaging NoteQu

For the separate native Apple Silicon target, see [macOS/README.md](macOS/README.md).
The instructions below build the Windows edition.

## Requirements

- Windows x64.
- Visual Studio 2022 or Build Tools 2022 with MSVC v143, C++ desktop tools,
  and a Windows SDK. The release build uses C++20 and the static MSVC runtime.
- PowerShell and Python 3.9 or later for packaging. Basic packaging scripts use
  the standard library; offline update signing and independent signature
  verification additionally require `updater/signing-requirements.txt`.

All editor and embedded library sources are in this repository. No private SDK,
local user profile, plugin DLL, or installed Notepad++ is needed.

## Build

From the repository root:

```powershell
& .\PowerEditor\Test\NoteQu\build-release.ps1
```

The script finds Visual Studio using `vswhere.exe`. Alternatively, pass
`-BuildTools 'C:\path\to\Visual Studio Build Tools'`.
It runs solution Clean and Build separately, then the native integration tests.
The binary is generated at `PowerEditor/bin64/notepad++.exe`.
You may also open `PowerEditor/visual.net/notepadPlus.sln` and select Release/x64.

## Create a fresh portable package

```powershell
python .\PowerEditor\Packaging\NoteQu\package.py --output .\dist\NoteQu
python .\PowerEditor\Packaging\NoteQu\verify_package.py .\dist\NoteQu
python .\PowerEditor\Test\NoteQu\smoke_release.py --binary .\dist\NoteQu\NoteQu.exe
```

The output must be a new or empty directory. The script never copies an active
installation. It copies only reviewed source-tree assets and fresh defaults,
adds licenses and source-access instructions, and writes a SHA-256 manifest.
Do not use an existing user profile as a packaging input.
English is the initial interface language; Turkish and Spanish are also included.
Session snapshots and GDI rendering are enabled. The upstream automatic updater
is disabled. NoteQu's own background GitHub checks are enabled by default and can
be disabled independently; see [UPDATING.md](UPDATING.md).

## Modify and install your own build

Edit the source and repeat the build and packaging commands. Extract your package
to a new writable directory and run `NoteQu.exe`. No activation key, signature
check, account, or installation lock is required for a modified build.
The distributed build is not signed. Do not overwrite user settings, sessions,
backups, documents, or `.notex` companion files during an upgrade.

## Release the matching source

Follow [RELEASING.md](RELEASING.md). Publish source for the exact revision used
to compile the binary, including the build and packaging inputs. Compiler tools
and Windows system libraries are external prerequisites. A matching source
revision does not imply byte-for-byte reproducibility: compiler versions and
embedded build timestamps can affect the executable.

## Authenticated updater

Install Go 1.27.4. Before Windows packaging, run `./updater/build_windows.ps1`. Native macOS builds compile the helper automatically. Run `go -C updater test ./...` and `python -m unittest discover -s updater -p test_migration.py`. See UPDATING.md for offline manifest signing and publication. Trusted platform signatures must be applied before creating update inventories.

The Windows updater build uses the pinned `github.com/tc-hib/go-winres@v0.3.3` build tool to generate its PE resources. The tool is downloaded through Go module verification and is not linked into the updater. The updater itself uses only the Go standard library.
