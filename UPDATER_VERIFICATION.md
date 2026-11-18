# NoteX 1.6.1 update verification

The released application packages and source archive correspond to commit `c336ef2a802927710a24b63c5fed753f338dbcc4`. Later CI-only changes select the verifier's virtual-environment Python explicitly; they do not change the released application or verification key.

## Build and package checks

- Windows: 101 native formatting checks, 22 menu checks, Function List fixtures, updater tests, and selective shortcut migration tests passed. Clean-package verification checked 142 files and the compiled identity of both executables.
- Native macOS ARM64: 11,875 suite assertions passed on both macOS 15.8.2 and macOS 26.7.2. Normal restart, application replacement, older-profile recovery, and post-snapshot process-termination checks also passed.
- The DMG was verified, mounted read-only, and compared with the ZIP application. Both bundles passed strict ad-hoc signature verification. This is not Apple notarization.
- All 11 public release assets were downloaded again without GitHub credentials. Their checksums, both Ed25519 manifest signatures, every inventoried update file, and the corresponding source revision were verified.

Build evidence: [Windows](https://github.com/berkkivircik/NoteX/actions/runs/44307703714), [macOS 15 and 26](https://github.com/berkkivircik/NoteX/actions/runs/44307703763).

## Actual public-release update

The test installs published 1.6.0 application files into an isolated installation, starts the actual editor, invokes its update command, and lets its production updater discover and install published 1.6.1. Consent dialogs are automated only in explicitly marked temporary test profiles. Repository selection, version comparison, platform checks, signatures, ownership checks, and normal shutdown remain enabled.

| Target | First transition | Actual updater transition | Result |
| --- | --- | --- | --- |
| Windows x64, local and hosted Windows | Published 1.3.3 to separately installed 1.6.0 | Published 1.6.0 to published 1.6.1 | Passed |
| macOS 15 ARM64 | Published 1.5.0-macos.5 to separately installed 1.6.0 | Published 1.6.0 to published 1.6.1 | Passed |
| macOS 26 ARM64 | Published 1.5.0-macos.5 to separately installed 1.6.0 | Published 1.6.0 to published 1.6.1 | Passed |

Each successful run discovered the compatible release, downloaded and verified its real package, waited for normal shutdown, replaced only application-owned files, restarted the new version, and removed the unchanged obsolete bootstrap notice. Synthetic unsaved text and settings survived. macOS additionally verified restored inline formatting. Windows preserved custom commands and a macro, and loaded and executed a genuine synthetic DLL plugin both before and after updating. This demonstrates compatibility for that fixture, not every third-party plugin.

Per-platform evidence is in the `updater-e2e-*` artifacts: [macOS 15 successful first run](https://github.com/berkkivircik/NoteX/actions/runs/44310251876), [Windows and macOS 26 successful jobs](https://github.com/berkkivircik/NoteX/actions/runs/44310607909). Other attempts received GitHub HTTP 403 during discovery and stopped without replacing application files. Such attempts are failures, not successful updates; no token or alternate release endpoint was used to make them pass. The first Windows CI attempt failed because the harness selected a Python interpreter without the verifier dependency; the workflow correction fixed that test-environment issue.

## Failure and preservation coverage

Updater tests reject wrong signatures, changed manifests, corrupt or truncated archives, wrong architectures, wrong platforms, duplicate or unsafe paths, unexpected file inventories, equal or older versions, drafts, and prereleases. Network fixtures cover timeouts, incomplete responses, oversized responses, and server errors. Interruption tests terminate a test-owned update worker after an actual durable-write boundary and recover the previous application. Recovery preserves user data created after the update and refuses to overwrite subsequently changed application files.

The updater does not force-close editors, migrate Windows user data to macOS, or replace plugin and personal configuration directories. Two bundled selection-upload commands are removed selectively; custom commands and macros remain supported. The old upstream automatic updater has no startup or shutdown launch path. Plugin administration remains separate.

## Remaining external signing limits

The Windows EXEs remain without a public-trust Authenticode signature and may trigger SmartScreen. The update manifest signature does not establish Windows publisher reputation. Native macOS packages remain ad-hoc signed and are not notarized. Physical-device testing on a user's Mac is separate from these hosted ARM64 results. No personal documents or active user profile were used as test fixtures or included in publication.
