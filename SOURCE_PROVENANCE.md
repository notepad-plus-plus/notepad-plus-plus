# Source provenance

## One active repository

[berkkivircik/NoteX](https://github.com/berkkivircik/NoteX) is the single active repository for development, signing preparation, issue tracking, tags, downloads, and updates. It is a registered GitHub fork of [notepad-plus-plus/notepad-plus-plus](https://github.com/notepad-plus-plus/notepad-plus-plus). Its GitHub repository ID is `1462906809`, and its NoteX development branch is `main`. The inherited upstream branch is not a NoteX release branch.

The original standalone repository (ID `1436883386`) is retained as the read-only [NoteX_old archive](https://github.com/berkkivircik/NoteX_old). It is not maintained as a second development or release repository. The fork was initially created under a temporary source-repository name, then renamed to NoteX during consolidation on November 18, 2026.

All 11 existing NoteX releases and their 50 uploaded assets were reproduced in the active fork. Original tag objects, source commits, release titles and notes, prerelease flags, filenames, file bytes, and hashes were preserved. GitHub assigns new release/asset IDs, publication timestamps, and download counters when records are recreated; the archive retains the original records. The current stable release is still v1.6.1. This repository migration is not a new application version or a retroactive signing operation.

The updater continues to use only `berkkivircik/NoteX`. Its signed manifests already bind that exact repository and download URL, so the migration preserves the existing signatures and trust key. No runtime or private-profile change is required. Historic uploaded source ZIP files remain byte-identical to the originals.

## Transparent history transition

The original public NoteX history consisted of reviewed source snapshots. It did not include the maintainer's private development history. On November 18, 2026, the following additive transition was made using only the existing public NoteX repository and the official public upstream repository:

| Record | Commit |
| --- | --- |
| Verified Notepad++ v8.9.8.1 base | `dd40fe45598791e9aa38f3aeb200e10fb3d6338a` |
| Existing public NoteX snapshot | `0458e7536ecd7988089be6b3395bc5577fdbca94` |
| Snapshot applied to the upstream base | `e6c7cde6c9d0b90ee1a1e7ec78b3099d4d5be8a7` |
| Normal merge retaining both histories | `baa4ccad3b06a57f69adb846e8ac2b47eb55f97f` |

The import is a new commit whose parent is the verified upstream release commit. Its tree is byte-for-byte identical to the existing public NoteX snapshot. A normal merge then joins that upstream-based branch into the existing NoteX `main`; the merge tree is also identical. The common tree ID is `e719ea979004c936228ffbe29fa75e8d7a85e37d`.

This is an explicit present-day import, not a claim that the older NoteX commits were developed inside a GitHub fork. No history was rewritten, no grafts or replacement refs were used, and no private commits or user profiles were imported. The base's 7,097 public upstream commits are retained. Historic NoteX release tags and source archives remain exactly as published and keep their original ancestry. A future signed release must be built from a new reviewed commit in the source fork.

## Reproduce the checks

Use a full clone, not a shallow clone:

```sh
git clone https://github.com/berkkivircik/NoteX.git
cd NoteX
python tools/verify_source_provenance.py
git ls-remote https://github.com/berkkivircik/NoteX.git refs/heads/main
```

The verification script checks ancestry, exact import and merge parents, tree equality, full history, and the absence of replacement refs. The GitHub fork relationship is independently visible on the source fork's homepage and in `GET /repos/berkkivircik/NoteX` (`fork: true`, parent `notepad-plus-plus/notepad-plus-plus`).

## Future changes and signing

1. Develop and review changes in `berkkivircik/NoteX`. Review upstream code changes and build/signing changes before release. A history check does not substitute for code review.
2. Use the reviewed `main` branch and exact release commit. Never mirror-push private repositories or force-update old release tags. Do not synchronize new changes into the archived repository.
3. Require the provenance check and the native Windows/macOS workflows at that commit. Build signing inputs in the provider-approved source fork's trusted hosted CI, not from workstation binaries.
4. After SignPath approves the project and configuration, require manual signing approval. Verify returned signatures before generating final packages, update manifests, and checksums.
5. Publish each new version in the same NoteX repository with its matching source revision and archives. Keep historical artifacts unchanged.

The updater end-to-end workflow and release API now refer to this same active fork under the NoteX name. Run that workflow against actual NoteX releases. CI build artifacts alone do not constitute a published release.

## Provider review still required

[SignPath Foundation's conditions](https://signpath.org/terms) require a visible upstream fork, a release branch based on upstream signed-release branches, and review of modified upstream code. The pinned base is the official Notepad++ v8.9.8.1 release; upstream publishes Authenticode-signed Windows binaries. This transition provides concrete fork and ancestry evidence for review. It does not grant eligibility, certify all code changes, or imply SignPath acceptance. MFA, project reputation, privacy and license review, the trusted build configuration, and manual approval remain separate prerequisites. See [Code signing policy](CODE_SIGNING.md).
