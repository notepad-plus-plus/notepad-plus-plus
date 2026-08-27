#!/bin/sh
# Notepad++ (Windows build) launcher for Linux, via Wine.
#
# Installed as /usr/bin/notepad-plus-plus. Runs the bundled notepad++.exe in an
# isolated, per-user Wine prefix and translates Linux file paths to Windows
# paths so files passed on the command line (or by a file manager) open.
set -eu

APPDIR="/opt/notepad-plus-plus"
EXE="$APPDIR/notepad++.exe"

# Isolated prefix so we never disturb the user's default ~/.wine
: "${NPP_WINEPREFIX:=${XDG_DATA_HOME:-$HOME/.local/share}/notepad-plus-plus/wineprefix}"
export WINEPREFIX="$NPP_WINEPREFIX"
# Quiet Wine unless the user asked for noise
export WINEDEBUG="${WINEDEBUG:--all}"

WINE="${NPP_WINE:-$(command -v wine 2>/dev/null || true)}"
if [ -z "$WINE" ]; then
	msg="Notepad++: 'wine' is not installed.\nInstall it with:  sudo apt install wine"
	if command -v zenity >/dev/null 2>&1; then
		printf '%b' "$msg" | zenity --error --no-wrap 2>/dev/null || true
	fi
	printf '%b\n' "$msg" >&2
	exit 1
fi

if [ ! -f "$EXE" ]; then
	echo "Notepad++: $EXE is missing (broken install?)." >&2
	exit 1
fi

# First-run prefix bootstrap (quiet, non-interactive).
if [ ! -d "$WINEPREFIX/drive_c" ]; then
	mkdir -p "$WINEPREFIX"
	# Skip Mono/Gecko prompts; Notepad++ needs neither.
	WINEDLLOVERRIDES="mscoree=d;mshtml=d" "$WINE" wineboot -i >/dev/null 2>&1 || true
	"${WINE}server" -w 2>/dev/null || true
fi

# Rebuild the argument list, converting existing files to Windows paths.
count=$#
while [ "$count" -gt 0 ]; do
	arg=$1
	shift
	case "$arg" in
		-*)
			: # a Notepad++ option (e.g. -n, -multiInst, -nosession); leave as-is
			;;
		*)
			if [ -e "$arg" ]; then
				abs=$(readlink -f "$arg" 2>/dev/null || printf '%s' "$arg")
				arg=$("$WINE" winepath -w "$abs" 2>/dev/null || printf '%s' "$abs")
			fi
			;;
	esac
	set -- "$@" "$arg"
	count=$((count - 1))
done

exec "$WINE" "$EXE" "$@"
