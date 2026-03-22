#!/usr/bin/env bash
#
# sign-and-notarize.sh -- Sign and notarize MacNote++.app and its DMG
#
# Usage:
#   ./sign-and-notarize.sh [options]
#
# Required environment variables:
#   CODESIGN_IDENTITY    - Signing identity (e.g., "Developer ID Application: Name (TEAMID)")
#
# For notarization (skip with --skip-notarize):
#   Password auth:  APPLE_ID + APPLE_TEAM_ID + APPLE_APP_PASSWORD
#   API key auth:   APPLE_API_KEY + APPLE_API_ISSUER (+ optional APPLE_API_KEY_PATH)
#
# Options:
#   --app-path PATH      Path to .app bundle (default: macos/dist/MacNote++.app)
#   --dmg-path PATH      Path to .dmg file (auto-detected from dist/)
#   --entitlements PATH  Path to entitlements (default: macos/platform/MacNote.entitlements)
#   --skip-notarize      Only sign, do not notarize
#   --skip-dmg           Only sign/notarize the app, not the DMG
#   --verbose            Enable verbose output
#   --help               Show this help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
MACOS_DIR="$REPO_ROOT/macos"

# Defaults
APP_PATH="${MACOS_DIR}/dist/MacNote++.app"
DMG_PATH=""
ENTITLEMENTS="${MACOS_DIR}/platform/MacNote.entitlements"
SKIP_NOTARIZE=false
SKIP_DMG=false
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
	case "$1" in
		--app-path)      APP_PATH="$2"; shift 2 ;;
		--dmg-path)      DMG_PATH="$2"; shift 2 ;;
		--entitlements)  ENTITLEMENTS="$2"; shift 2 ;;
		--skip-notarize) SKIP_NOTARIZE=true; shift ;;
		--skip-dmg)      SKIP_DMG=true; shift ;;
		--verbose)       VERBOSE=true; shift ;;
		--help)          head -24 "$0" | tail -22; exit 0 ;;
		*)               echo "Unknown option: $1" >&2; exit 1 ;;
	esac
done

# Logging
log()     { echo "==> $*"; }
warn()    { echo "WARNING: $*" >&2; }
die()     { echo "ERROR: $*" >&2; exit 1; }
verbose() { $VERBOSE && echo "    $*" || true; }

# --- Validation --------------------------------------------------------

[[ -z "${CODESIGN_IDENTITY:-}" ]] && die "CODESIGN_IDENTITY is not set"
[[ -d "$APP_PATH" ]] || die "App bundle not found: $APP_PATH"
[[ -f "$ENTITLEMENTS" ]] || die "Entitlements not found: $ENTITLEMENTS"

AUTH_MODE=""
if ! $SKIP_NOTARIZE; then
	if [[ -n "${APPLE_API_KEY:-}" ]]; then
		[[ -n "${APPLE_API_ISSUER:-}" ]] || die "APPLE_API_ISSUER required with APPLE_API_KEY"
		AUTH_MODE="apikey"
	elif [[ -n "${APPLE_ID:-}" ]]; then
		[[ -n "${APPLE_TEAM_ID:-}" ]]    || die "APPLE_TEAM_ID required with APPLE_ID"
		[[ -n "${APPLE_APP_PASSWORD:-}" ]] || die "APPLE_APP_PASSWORD required with APPLE_ID"
		AUTH_MODE="password"
	else
		die "Set APPLE_API_KEY+APPLE_API_ISSUER or APPLE_ID+APPLE_TEAM_ID+APPLE_APP_PASSWORD for notarization"
	fi
fi

# Auto-detect DMG path
if [[ -z "$DMG_PATH" ]] && ! $SKIP_DMG; then
	if [[ -f "${MACOS_DIR}/dist/MacNote++.dmg" ]]; then
		DMG_PATH="${MACOS_DIR}/dist/MacNote++.dmg"
	elif [[ -f "${MACOS_DIR}/dist/MacNote++-unsigned.dmg" ]]; then
		DMG_PATH="${MACOS_DIR}/dist/MacNote++-unsigned.dmg"
	else
		warn "No DMG found in dist/. Use --dmg-path or --skip-dmg."
		SKIP_DMG=true
	fi
fi

# --- Build notarytool auth flags as array (avoids word-splitting issues) ---

NOTARY_AUTH_FLAGS=()
if ! $SKIP_NOTARIZE; then
	if [[ "$AUTH_MODE" == "apikey" ]]; then
		key_path="${APPLE_API_KEY_PATH:-$HOME/.private_keys/AuthKey_${APPLE_API_KEY}.p8}"
		NOTARY_AUTH_FLAGS=(--key "$key_path" --key-id "$APPLE_API_KEY" --issuer "$APPLE_API_ISSUER")
	else
		NOTARY_AUTH_FLAGS=(--apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD")
	fi
fi

# --- Step 1: Sign the .app bundle --------------------------------------

log "Signing app bundle: $APP_PATH"
verbose "Identity: $CODESIGN_IDENTITY"
verbose "Entitlements: $ENTITLEMENTS"

# Strip extended attributes (resource forks, Finder info) that block codesign
xattr -cr "$APP_PATH"

# NOTE: --deep is acceptable while the bundle has no nested frameworks/helpers.
# Replace with explicit per-component signing if nested bundles are added.
codesign --force --options runtime \
	--sign "$CODESIGN_IDENTITY" \
	--entitlements "$ENTITLEMENTS" \
	--timestamp \
	--deep \
	"$APP_PATH"

log "Verifying app signature..."
codesign --verify --deep --strict --verbose=2 "$APP_PATH"

log "Checking Gatekeeper acceptance..."
if spctl --assess --type execute --verbose=2 "$APP_PATH" 2>&1; then
	log "App passes Gatekeeper assessment"
else
	warn "App does not yet pass Gatekeeper (expected before notarization)"
fi

# --- Step 2: Sign the DMG ----------------------------------------------

if ! $SKIP_DMG; then
	log "Signing DMG: $DMG_PATH"
	codesign --force --sign "$CODESIGN_IDENTITY" --timestamp "$DMG_PATH"
	codesign --verify --verbose=2 "$DMG_PATH"
fi

# --- Step 3: Notarize --------------------------------------------------

if ! $SKIP_NOTARIZE; then
	if ! $SKIP_DMG && [[ -f "$DMG_PATH" ]]; then
		NOTARIZE_TARGET="$DMG_PATH"
	else
		# Create a temporary zip for notarization
		NOTARIZE_TARGET="${APP_PATH%.app}.zip"
		log "Creating zip for notarization: $NOTARIZE_TARGET"
		ditto -c -k --keepParent "$APP_PATH" "$NOTARIZE_TARGET"
	fi

	log "Submitting for notarization: $NOTARIZE_TARGET"
	verbose "This may take several minutes..."

	xcrun notarytool submit "$NOTARIZE_TARGET" \
		"${NOTARY_AUTH_FLAGS[@]}" \
		--wait \
		--timeout 30m

	# --- Step 4: Staple ------------------------------------------------

	if ! $SKIP_DMG && [[ -f "$DMG_PATH" ]]; then
		log "Stapling notarization ticket to DMG: $DMG_PATH"
		xcrun stapler staple "$DMG_PATH"

		# Rename unsigned DMG to signed name
		if [[ "$DMG_PATH" == *"-unsigned.dmg" ]]; then
			SIGNED_DMG="${DMG_PATH%-unsigned.dmg}.dmg"
			mv "$DMG_PATH" "$SIGNED_DMG"
			log "Renamed to: $SIGNED_DMG"
			DMG_PATH="$SIGNED_DMG"
		fi
	fi

	log "Stapling notarization ticket to app: $APP_PATH"
	xcrun stapler staple "$APP_PATH"

	# Clean up temp zip
	if [[ "${NOTARIZE_TARGET:-}" == *.zip ]]; then
		rm -f "$NOTARIZE_TARGET"
	fi
fi

# --- Final verification -------------------------------------------------

log "Final verification..."
codesign --verify --deep --strict --verbose=2 "$APP_PATH"

if ! $SKIP_NOTARIZE; then
	spctl --assess --type execute --verbose=2 "$APP_PATH" || true
	if ! $SKIP_DMG && [[ -f "$DMG_PATH" ]]; then
		spctl --assess --type open --context context:primary-signature --verbose=2 "$DMG_PATH" || true
	fi
fi

log "Done. Artifacts:"
log "  App: $APP_PATH"
if ! $SKIP_DMG && [[ -n "$DMG_PATH" ]]; then
	log "  DMG: $DMG_PATH"
fi
