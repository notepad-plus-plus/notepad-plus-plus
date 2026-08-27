#!/usr/bin/env bash
#
# build-deb.sh - Build a .deb of Notepad++ (Windows build) that runs under Wine.
#
# Two stages:
#   1. Cross-compile notepad++.exe from this repo with MinGW-w64 (the upstream
#      PowerEditor/gcc makefile), producing a self-contained runtime tree.
#   2. Assemble that tree + launcher + desktop integration into a .deb.
#
# The script is self-guarding: it auto-detects the POSIX MinGW compiler, wipes
# the object tree whenever the toolchain fingerprint changes (so win32/native
# objects can never mix into a POSIX build), locates the built exe even if the
# upstream output-dir name changes, and validates the payload before packaging.
#
# Usage:
#   linux-deb/build-deb.sh                 # cross-compile, then package
#   linux-deb/build-deb.sh --clean         # force a full rebuild first
#   linux-deb/build-deb.sh --skip-build    # reuse a previous cross-compile
#   linux-deb/build-deb.sh --tree DIR      # skip the build; package an existing
#                                          #   runtime tree (must contain
#                                          #   notepad++.exe), e.g. an unpacked
#                                          #   official Notepad++ portable zip
#
# Env overrides:
#   CROSS_COMPILE   MinGW prefix (default x86_64-w64-mingw32-)
#   DEB_REVISION    Debian revision suffix (default 1)
#   MAINTAINER      "Name <email>" for the control file
#   JOBS            parallel compile jobs (default: nproc)
#
set -euo pipefail

# --- locations ---------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"          # repo root
GCC_DIR="$ROOT/PowerEditor/gcc"
OUT_DIR="$SCRIPT_DIR/out"                       # where the .deb lands
STAGE="$SCRIPT_DIR/.stage"                      # dpkg staging root

CROSS_COMPILE="${CROSS_COMPILE:-x86_64-w64-mingw32-}"
DEB_REVISION="${DEB_REVISION:-1}"
MAINTAINER="${MAINTAINER:-Notepad++ Wine packaging <noreply@example.com>}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"
ARCH="amd64"
PKG="notepad-plus-plus"

TREE=""
SKIP_BUILD=0
DO_CLEAN=0
while [ $# -gt 0 ]; do
	case "$1" in
		--tree)       TREE="${2:?--tree needs a directory}"; shift 2 ;;
		--skip-build) SKIP_BUILD=1; shift ;;
		--clean)      DO_CLEAN=1; shift ;;
		-h|--help)    grep -E '^#( |$)' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
		*) echo "unknown arg: $1" >&2; exit 2 ;;
	esac
done

say()  { printf '\033[1;32m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33mWARN:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

require_cmd() {
	command -v "$1" >/dev/null 2>&1 || die "missing required tool '$1'${2:+  ($2)}"
}

# --- prerequisites (packaging tools are always needed) ----------------------
require_cmd dpkg-deb "sudo apt install dpkg-dev"
require_cmd python3  "sudo apt install python3"
python3 -c 'import PIL' 2>/dev/null \
	|| die "Python Pillow is required for icon conversion:  sudo apt install python3-pil"

# --- version from the source of truth ---------------------------------------
RESOURCE_H="$ROOT/PowerEditor/src/resource.h"
[ -f "$RESOURCE_H" ] || die "cannot find $RESOURCE_H (unexpected repo layout)"
VERSION_RAW="$(grep -m1 'NOTEPAD_PLUS_VERSION' "$RESOURCE_H" \
	| sed -E 's/.*"Notepad\+\+ v([0-9.]+)".*/\1/')"
case "$VERSION_RAW" in
	''|*[!0-9.]*) die "could not parse a clean version from resource.h (got '${VERSION_RAW:-empty}')" ;;
esac
DEB_VERSION="${VERSION_RAW}-${DEB_REVISION}"
say "Notepad++ version: $VERSION_RAW  (deb: $DEB_VERSION)"

# --- pick the MinGW compiler (POSIX threads required for std::mutex) ---------
# Needed even in --tree mode is skipped: only set when we actually build.
select_compiler() {
	# Notepad++ uses std::thread / std::mutex, which require MinGW's POSIX
	# threading model. Debian's default alternative is often the win32 model
	# (no <mutex> support), so prefer the explicit -posix variant when present.
	if command -v "${CROSS_COMPILE}g++-posix" >/dev/null 2>&1; then
		CXX_BIN="${CROSS_COMPILE}g++-posix"
		CC_BIN="${CROSS_COMPILE}gcc-posix"
	else
		CXX_BIN="${CROSS_COMPILE}g++"
		CC_BIN="${CROSS_COMPILE}gcc"
	fi
}

# Fingerprint that must invalidate the object tree when it changes: compiler
# path + target triple + full version + thread model. A distro GCC upgrade or
# a win32->posix switch therefore forces a clean rebuild automatically.
toolchain_fingerprint() {
	printf '%s|%s|%s|%s' \
		"$CXX_BIN" \
		"$("$CXX_BIN" -dumpmachine 2>/dev/null || echo '?')" \
		"$("$CXX_BIN" -dumpfullversion -dumpversion 2>/dev/null || echo '?')" \
		"$("$CXX_BIN" -v 2>&1 | sed -n 's/.*Thread model: //p' | head -1)"
}

# --- stage 1: obtain the runtime tree ---------------------------------------
if [ -n "$TREE" ]; then
	[ -f "$TREE/notepad++.exe" ] || die "--tree '$TREE' has no notepad++.exe"
	RUNTIME="$TREE"
	say "Using provided runtime tree: $RUNTIME"
else
	select_compiler

	if [ "$SKIP_BUILD" -eq 0 ]; then
		command -v "$CXX_BIN" >/dev/null 2>&1 \
			|| die "MinGW-w64 not found ($CXX_BIN). Install it:  sudo apt install mingw-w64"
		for t in "${CROSS_COMPILE}ar" "${CROSS_COMPILE}ranlib" "${CROSS_COMPILE}windres" "${CROSS_COMPILE}objdump"; do
			require_cmd "$t" "part of mingw-w64 binutils"
		done

		# Guard against the win32 threading model, which cannot compile <mutex>.
		if ! "$CXX_BIN" -v 2>&1 | grep -q 'Thread model: posix'; then
			die "$CXX_BIN uses a non-POSIX thread model; Notepad++ needs POSIX threads.
    Fix: sudo update-alternatives --config ${CROSS_COMPILE}g++   (choose the -posix entry)"
		fi

		# Stale/mixed-object guard. The upstream makefile does incremental builds
		# and does NOT know the compiler changed, so switching toolchains would
		# silently mix incompatible objects. We stamp the fingerprint and wipe the
		# object tree whenever it differs (or on --clean).
		STAMP="$GCC_DIR/.npp-toolchain-stamp"
		FP="$(toolchain_fingerprint)"
		if [ "$DO_CLEAN" -eq 1 ]; then
			say "Cleaning object tree (--clean)"
			rm -rf "$GCC_DIR"/bin.gcc.* "$STAMP"
		elif [ -f "$STAMP" ] && [ "$(cat "$STAMP" 2>/dev/null)" != "$FP" ]; then
			warn "Toolchain changed since last build - wiping object tree to avoid mixed objects"
			warn "  was: $(cat "$STAMP" 2>/dev/null)"
			warn "  now: $FP"
			rm -rf "$GCC_DIR"/bin.gcc.*
		fi

		say "Generating NppLibsVersion.h"
		sh "$SCRIPT_DIR/scripts/gen-version-header.sh" "$ROOT"

		say "Cross-compiling notepad++.exe with $CXX_BIN (this takes a while)"
		# The Scintilla and Lexilla sub-makefiles do NOT honor CROSS_COMPILE;
		# they fall back to make's default g++ (native). Pass the whole cross
		# toolchain explicitly so command-line vars propagate into the sub-makes
		# and override their defaults.
		# PREBUILD_EVENT_CMD=true replaces the Windows-only `cmd //C ...bat`
		# (we already generated the header above).
		make -C "$GCC_DIR" \
			CROSS_COMPILE="$CROSS_COMPILE" \
			CXX="$CXX_BIN" \
			CC="$CC_BIN" \
			AR="${CROSS_COMPILE}ar" \
			RANLIB="${CROSS_COMPILE}ranlib" \
			WINDRES="${CROSS_COMPILE}windres" \
			PREBUILD_EVENT_CMD=true \
			-j"$JOBS"

		# Record the fingerprint only after a successful build.
		printf '%s' "$FP" > "$STAMP"
	fi

	# Locate the freshly built exe defensively: prefer the conventional
	# bin.gcc.<cpu> dir, but fall back to the newest notepad++.exe anywhere
	# under the gcc dir, so an upstream output-dir rename does not break us.
	EXE_PATH="$(find "$GCC_DIR" -maxdepth 2 -type f -name 'notepad++.exe' \
		-printf '%T@ %p\n' 2>/dev/null | sort -rn | head -1 | cut -d' ' -f2-)"
	[ -n "$EXE_PATH" ] || die "build produced no notepad++.exe under $GCC_DIR
    (did the compile fail above? re-run with --clean, or check the output.)"
	RUNTIME="$(dirname "$EXE_PATH")"
	say "Runtime tree: $RUNTIME"
fi

EXE="$RUNTIME/notepad++.exe"

# Sanity-check the exe really is a Windows PE binary (not a stray native build).
if command -v file >/dev/null 2>&1; then
	file -b "$EXE" | grep -qiE 'PE32|MS Windows' \
		|| die "$EXE is not a Windows PE executable (native build leaked in?). Try --clean."
fi

# --- stage 2: assemble the .deb ---------------------------------------------
say "Staging package tree"
rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN" \
	"$STAGE/opt/$PKG" \
	"$STAGE/usr/bin" \
	"$STAGE/usr/share/applications" \
	"$STAGE/usr/share/icons/hicolor" \
	"$STAGE/usr/share/doc/$PKG"

# payload: the whole runtime tree (exe + langs/themes/autoCompletion/... )
cp -a "$RUNTIME/." "$STAGE/opt/$PKG/"
# Drop doLocalConf.xml so Notepad++ stores user config under the (writable)
# Wine prefix's %APPDATA% instead of trying to write into read-only /opt.
rm -f "$STAGE/opt/$PKG/doLocalConf.xml"
# The Windows auto-updater cannot run here; keep it disabled if present.
# (disableNppAutoUpdate.xml, if shipped by the makefile, is intentionally kept.)

# Bundle any MinGW runtime DLLs the exe imports (libstdc++, libgcc,
# libwinpthread). With static linking there are usually none; if the exe DOES
# import one and we cannot find it, that is fatal - the package would not run.
# Skipped for --tree (that tree is expected to be self-contained already).
if [ -z "$TREE" ]; then
	say "Checking runtime DLL dependencies"
	OBJDUMP="${CROSS_COMPILE}objdump"
	dll_missing=0
	dll_bundled=0
	for dll in $("$OBJDUMP" -p "$EXE" \
			| awk '/DLL Name:/{print $3}' | grep -iE '^lib.*\.dll$' | sort -u); do
		# -print-file-name resolves against the chosen compiler's own search
		# dirs, so we get the matching (POSIX) variant of each runtime DLL.
		src="$("${CXX_BIN}" -print-file-name="$dll")"
		if [ -f "$src" ]; then
			cp -f "$src" "$STAGE/opt/$PKG/"
			echo "    + bundled $dll  ($src)"
			dll_bundled=$((dll_bundled + 1))
		else
			warn "exe imports $dll but it could not be located"
			dll_missing=1
		fi
	done
	[ "$dll_missing" -eq 0 ] || die "missing runtime DLL(s) above; the package would fail under Wine"
	[ "$dll_bundled" -ne 0 ] || echo "    (statically linked - no extra DLLs needed)"
fi

# Validate the essential runtime data made it into the payload.
[ -f "$STAGE/opt/$PKG/notepad++.exe" ] || die "notepad++.exe missing from payload"
[ ! -e "$STAGE/opt/$PKG/doLocalConf.xml" ] || die "doLocalConf.xml still present (would break config in read-only /opt)"
for d in themes localization autoCompletion functionList; do
	[ -d "$STAGE/opt/$PKG/$d" ] \
		|| warn "runtime data '$d/' is absent - upstream layout may have changed; the exe still runs but that feature's defaults are missing"
done

# launcher + friendly alias
install -m0755 "$SCRIPT_DIR/scripts/npp-launcher.sh" "$STAGE/usr/bin/$PKG"
ln -sf "$PKG" "$STAGE/usr/bin/notepad++"

# desktop entry
install -m0644 "$SCRIPT_DIR/debian/notepad-plus-plus.desktop" \
	"$STAGE/usr/share/applications/$PKG.desktop"

# icons (extracted from the app .ico via Pillow)
ICO="$ROOT/PowerEditor/src/icons/npp.ico"
[ -f "$ICO" ] || die "app icon not found: $ICO"
say "Generating icons"
python3 "$SCRIPT_DIR/scripts/make-icons.py" "$ICO" "$STAGE/usr/share/icons/hicolor"

# docs
install -m0644 "$SCRIPT_DIR/debian/copyright" "$STAGE/usr/share/doc/$PKG/copyright"
install -m0644 "$SCRIPT_DIR/README.md" "$STAGE/usr/share/doc/$PKG/README.md" 2>/dev/null || true

# maintainer scripts
for f in postinst prerm postrm; do
	if [ -f "$SCRIPT_DIR/debian/$f" ]; then
		install -m0755 "$SCRIPT_DIR/debian/$f" "$STAGE/DEBIAN/$f"
	fi
done

# control (with computed installed-size)
[ -f "$SCRIPT_DIR/debian/control.in" ] || die "missing debian/control.in"
SIZE_KB="$(du -sk --exclude=DEBIAN "$STAGE" | cut -f1)"
sed -e "s|@VERSION@|$DEB_VERSION|" \
	-e "s|@ARCH@|$ARCH|" \
	-e "s|@SIZE@|$SIZE_KB|" \
	-e "s|@MAINTAINER@|$MAINTAINER|" \
	"$SCRIPT_DIR/debian/control.in" > "$STAGE/DEBIAN/control"

# build
mkdir -p "$OUT_DIR"
DEB="$OUT_DIR/${PKG}_${DEB_VERSION}_${ARCH}.deb"
say "Building $DEB"
dpkg-deb --root-owner-group --build "$STAGE" "$DEB" >/dev/null

# post-build verification: the archive must list the exe and the launcher.
# (Capture once - piping into `grep -q` would SIGPIPE dpkg-deb and trip pipefail.)
CONTENTS="$(dpkg-deb --contents "$DEB")"
printf '%s\n' "$CONTENTS" | grep -Fq 'opt/notepad-plus-plus/notepad++.exe' \
	|| die "built .deb is missing the exe (packaging bug)"
printf '%s\n' "$CONTENTS" | grep -Fq 'usr/bin/notepad-plus-plus' \
	|| die "built .deb is missing the launcher (packaging bug)"
if command -v lintian >/dev/null 2>&1; then
	say "lintian (informational)"
	lintian "$DEB" 2>/dev/null | sed 's/^/    /' || true
fi

say "Done."
echo
dpkg-deb --info "$DEB" | sed 's/^/    /'
echo
echo "Install with:"
echo "    sudo apt install ./${DEB#"$ROOT/"}     # from repo root, resolves Wine dependency"
echo "  or:"
echo "    sudo dpkg -i $DEB && sudo apt-get -f install"
