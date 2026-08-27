# Notepad++ `.deb` package (Windows build via Wine)

This directory builds a Debian/Ubuntu `.deb` that installs the **genuine Windows
build of Notepad++** — cross-compiled from *this* repository with MinGW-w64 — and
runs it under **Wine**, with full desktop and file-type integration.

## Why Wine, and not a native build?

Notepad++'s source is 100% Win32 (window classes, `WM_*` messages, Win32 dialogs,
Scintilla's Windows platform layer, GDI, UAC, registry). There is **no native
Linux target** — a real native port would be a ground-up rewrite against GTK/Qt,
which is what separate projects (Notepadqq, CudaText) are.

Running the real `.exe` under Wine is therefore the *only* way to get **every**
Notepad++ feature working faithfully: plugins, user-defined languages, sessions,
macros, themes, column mode, the lot. It behaves exactly like on Windows because
it *is* the Windows program.

> This package is **not** a native application; it depends on Wine at runtime.

## What the package contains

| Path | Contents |
|------|----------|
| `/opt/notepad-plus-plus/` | `notepad++.exe` + runtime data (langs, stylers, themes, autoCompletion, functionList, localization, userDefineLangs) |
| `/usr/bin/notepad-plus-plus` | Wine launcher (also symlinked as `notepad++`) |
| `/usr/share/applications/notepad-plus-plus.desktop` | App-menu entry + MIME associations |
| `/usr/share/icons/hicolor/*/apps/notepad-plus-plus.png` | Icons extracted from `npp.ico` |
| `/usr/share/doc/notepad-plus-plus/` | This README + copyright |

User configuration is stored **per user** inside an isolated Wine prefix at
`~/.local/share/notepad-plus-plus/wineprefix/` — it never touches your default
`~/.wine`, and `/opt` stays read-only.

## Build prerequisites

```bash
sudo apt update
sudo apt install mingw-w64 dpkg-dev fakeroot python3-pil
```

`mingw-w64` cross-compiles the `.exe`; `python3-pil` (Pillow) converts the icon;
`dpkg-dev` provides `dpkg-deb`.

## Build the package

From the repository root:

```bash
./linux-deb/build-deb.sh
```

The result lands in `linux-deb/out/notepad-plus-plus_<version>_amd64.deb`.

### Build options

| Invocation | Effect |
|------------|--------|
| `./linux-deb/build-deb.sh` | Cross-compile (incremental) then package |
| `./linux-deb/build-deb.sh --clean` | Force a full rebuild first |
| `./linux-deb/build-deb.sh --skip-build` | Reuse the last cross-compile, just repackage |
| `./linux-deb/build-deb.sh --tree DIR` | Skip building; package an existing runtime tree (e.g. an unpacked official *Notepad++ portable* zip that already contains `notepad++.exe`) |

Env overrides: `CROSS_COMPILE`, `DEB_REVISION`, `MAINTAINER`, `JOBS`.

### Self-guarding / reproducibility

The build script is designed to stay correct without manual babysitting:

- **POSIX-thread compiler auto-selected.** It prefers `…-g++-posix` (Notepad++
  needs POSIX `std::mutex`/`std::thread`) and refuses to build with the win32
  threading model, pointing you at `update-alternatives` if that is all you have.
- **Mixed-object guard.** It fingerprints the toolchain (compiler path + target
  triple + version + thread model) and automatically wipes the object tree when
  that fingerprint changes — so a distro GCC upgrade or a win32→posix switch can
  never silently mix incompatible objects. Unchanged toolchain → fast incremental
  rebuilds. `--clean` forces a wipe on demand.
- **Layout-drift tolerant.** The built `notepad++.exe` is located by search, not a
  hard-coded path, so an upstream output-dir rename does not break packaging. The
  version is read from `resource.h`. Missing runtime-data dirs or an unexpectedly
  non-static exe are reported clearly instead of producing a broken package.
- **Post-build verification.** The finished `.deb` is checked to actually contain
  the exe and launcher (and linted if `lintian` is installed).

Nothing in the upstream Notepad++ source or its makefiles is modified; everything
lives in `linux-deb/`. Re-running the script reproduces the package deterministically.

## Install

```bash
sudo apt install ./linux-deb/out/notepad-plus-plus_*_amd64.deb
```

Using `apt install ./file.deb` (rather than `dpkg -i`) pulls in the **Wine**
dependency automatically. If you used `dpkg -i`, fix dependencies with
`sudo apt-get -f install`.

Wine itself, if not already present:

```bash
sudo apt install wine
```

## Usage

- **App menu:** launch *Notepad++* like any other app.
- **Terminal:**
  ```bash
  notepad-plus-plus file1.txt file2.cpp     # or: notepad++ file.txt
  ```
  Linux paths are translated to Windows paths automatically.
- **File manager:** right-click a text file → *Open With* → *Notepad++*.
- **Notepad++ CLI options** pass straight through, e.g.
  `notepad-plus-plus -nosession -n42 file.log` (open at line 42, no session).

First launch is slightly slower while Wine initialises the per-user prefix.

### Environment knobs

| Variable | Effect |
|----------|--------|
| `NPP_WINEPREFIX` | Use a different Wine prefix directory |
| `NPP_WINE` | Use a specific `wine` binary |
| `WINEDEBUG` | Defaults to `-all` (quiet); set e.g. `warn+all` to debug |

## Uninstall

```bash
sudo apt remove notepad-plus-plus     # or: sudo dpkg -r notepad-plus-plus
```

Per-user settings/prefix are intentionally left behind. Remove them with:

```bash
rm -rf ~/.local/share/notepad-plus-plus
```

## Limitations

- Requires Wine; not a native app.
- Windows-specific plugins that call the Win32 shell or external `.exe` updaters
  (e.g. the built-in auto-updater / Plugins Admin network installs) may be flaky
  under Wine. Core editing, syntax, UDL, macros, sessions, and most plugins work.
- Deep OS integration (shell context-menu entries created by the Windows
  installer, Explorer thumbnailing) does not apply on Linux.
