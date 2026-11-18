# Updating NoteQu

NoteQu checks only public releases in `berkkivircik/NoteQu`. Checks run outside the editor process, at most once every 24 hours per profile. Use **Check for NoteQu Updates** (Windows Help menu) or **Check for Updates** (macOS NoteQu menu) to check immediately. **Automatic Update Checks** enables or disables background checks. Existing upstream Notepad++ update settings do not control this updater.

The updater selects the newest stable release with a signed manifest for its own operating system and processor architecture. Drafts, prereleases, equal versions, older versions, and other platforms are excluded. A newer macOS-only release does not hide an available Windows release. Network failures leave the editor usable. Public downloads do not require a GitHub account or token.

Only standard HTTPS requests for public release metadata and assets are sent. They contain a fixed updater user agent, not document contents, selected text, file paths, session data, or preferences. GitHub and its download infrastructure necessarily receive connection information such as the client's IP address. General Run commands, plugin tools, and links that the user explicitly invokes remain separate network-capable features.

## Authentication and installation

An Ed25519 public verification key is compiled into the updater. Each platform manifest is signed by the offline release key and binds the repository, numeric version, source revision, platform, architecture, package URL, and the hash and size of every application-owned file. ZIP contents are verified against that authenticated inventory. SHA-256 checksum files are also published for manual checks, but are not the updater's trust anchor.

After download and verification, approve installation and close NoteQu normally. The helper waits for the process to exit; it does not terminate the editor. If another instance still uses the installation, the staged update is retained. Reopen NoteQu and check again after closing the other instance. Installation requires write access to the application directory. The updater never silently elevates privileges.

Windows updates replace only the application executable, updater executable, and the explicitly inventoried distribution documents and licenses. They never overlay configuration files, themes, language customizations, recovery snapshots, documents, or plugin directories. New default files are available in the separate clean portable package. Keeping plugin files does not prove compatibility with every new editor version; plugin API compatibility must be tested separately.

macOS updates operate on the inventoried files inside NoteQu.app. Profiles are outside the bundle. NoteQu uses `~/Library/Application Support/NoteQu` for new profiles and opens the existing `~/Library/Application Support/NoteX` profile if a NoteQu profile does not already exist. Update receipts and recovery state are stored beside the bundle, outside its code signature. Signed update manifests do not replace Apple Developer ID signing or notarization.

## Recovery

Before modifying application files, the helper writes durable backups and a transaction journal. It removes an obsolete owned file only when its contents still match the previous authenticated inventory. Unknown files and modified obsolete files are retained. Recovery restores application files only; it never restores an old copy of user documents or settings.

The new application must start and confirm its version before the transaction is committed. If startup fails and the process has exited, the helper attempts to roll back the application. A running application is never killed to force a rollback. Interrupted updates retain their recovery helper and journal in `.notequ-update` on Windows or `.NoteQu.app-update` beside the macOS bundle. On the next launch, the updater offers recovery before checking for another update. If the executable itself cannot start, run the retained helper in recovery mode with the installation path, profile path, current version, and the PID of an existing process that you will close normally. Keep recovery records until the updated application has been verified.

## From NoteX to NoteQu

The first upgrade requires a manual install because NoteX's updater only accepts the old repository identity and executable names. Old NoteX update checks may report a network error after the repository rename.

- **Windows:** Close NoteX normally and back up the portable directory. Put `NoteQu.exe` and `NoteQuUpdater.exe` in the existing directory and point your shortcuts to `NoteQu.exe`. Keep your profile and custom assets there, including `config.xml`, `session.xml`, recovery backups, menu settings, themes, plugins, and `.notex` companions. If you use a separate profile directory, pass the same `-settingsDir` option. Verify the restored session before retiring the old executables.
- **macOS:** Quit NoteX before launching NoteQu. Install `NoteQu.app` from the new package. The app opens the existing NoteX Application Support profile in place if no NoteQu profile exists. If both folders already exist, back up both before exchanging or merging anything.
- Retain any old updater recovery files with your backup. NoteQu uses its own update receipts.

## Earlier NoteX releases

Versions before 1.6.0 do not contain this updater. Install the first updater-enabled release separately, after a normal shutdown. Do not extract a clean portable package over an existing profile. Copy only the reviewed application-owned files and retain all personal configuration and plugin files. The first check authenticates the installed version against the signed inventory from that exact GitHub release before applying a later release.

## Release signing

Build and test the exact committed source. Apply platform code signatures before creating file inventories. On Windows, `PowerEditor/Packaging/NoteQu/sign-windows.ps1` supports a trusted code signing identity in the current user's certificate store. It requires successful trust, publisher, and timestamp verification. Do not pass certificate passwords or private keys in command-line arguments.

The independent update signing tool is `updater/sign_release.py` (Python with `cryptography`). Initialize the key once with `--init-key`. The private Ed25519 key is stored outside the repository under the current Windows account's local application data, encrypted with DPAPI and restricted to that account. Back up the account's protected key material through an approved secure backup process. Losing the key requires a separately installed trust-anchor transition; never silently generate a replacement for an existing embedded key. CI does not receive this key. After reviewing CI artifacts, sign each platform's update inventory offline using `--root`, `--output`, `--platform`, `--architecture`, `--version`, and `--source`.

Publish application packages, update ZIPs, the matching JSON manifests and detached `.sig` files, checksums, licenses, and the exact source archive together. Do not replace historical release assets with a different build. Download the public assets again and verify their signatures, inventories, and checksums.

## Integration tests

Updater unit tests run with `cd updater && go test ./...` on Windows and macOS. Real-release integration tests start the actual native application in a synthetic temporary profile. Supply two published NoteQu versions, with the target set to the latest compatible release. Test consent automation requires an explicitly named `test-update-*` profile under the operating system's temporary directory, an `allow-update-test` marker, and an exact `NOTEX_TEST_UPDATE_PROFILE` match. It answers consent dialogs only. It does not alter the release source, version comparison, signature verification, platform validation, shutdown wait, or installation checks.

## Platform signing status

The manifest signature authenticates NoteQu updates to an already trusted NoteQu installation. It is not a Windows Authenticode certificate and does not establish SmartScreen publisher reputation. Windows releases remain unsigned until a public-trust signing provider is approved and configured. Newly signed binaries can still trigger reputation warnings. macOS ad-hoc signatures verify bundle consistency but do not provide Apple notarization. Neither protection should be disabled to install an update.
