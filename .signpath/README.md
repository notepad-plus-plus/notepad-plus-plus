# Windows signing preparation

Status: candidate configuration only. No SignPath account, certificate, project, signing policy, token, or GitHub App grant is configured by these files. No signing request is submitted automatically.

`artifact-configuration.xml` selects exactly `NoteX.exe` and `NoteXUpdater.exe` at the root of an uploaded ZIP. Both require the NoteX product name, their respective original filenames, and the same supplied four-component product/file version. For the existing 1.6.1 binaries that value is `1.6.1.0`. There is no wildcard that would sign third-party executables or plugin DLLs. Upload only these two files as the signing artifact; the XML does not, by itself, forbid unrelated archive entries.

The XML is based on the provider's [artifact syntax](https://docs.signpath.io/artifact-configuration/syntax) and [PE metadata restrictions](https://docs.signpath.io/artifact-configuration/reference#file-metadata-restrictions). Local XML parsing and checks against the actual 1.6.1 PE metadata are preparation checks. Validation in SignPath's configuration editor and a genuine signed-artifact round trip remain required after enrollment.

## After the provider accepts the project

1. Ask the provider to approve `berkkivircik/NoteX-source`, branch `main`, as the signing source. It is a registered Notepad++ fork with verified upstream ancestry; see [Source provenance](../SOURCE_PROVENANCE.md). Keep `berkkivircik/NoteX` as the canonical publication/update repository, synchronized to the exact same commit. Disclose the snapshot-history transition and preserve historical NoteX tags and artifacts.
2. Enable and verify MFA for every participating GitHub and SignPath account. Configure the named committer, reviewer, submitter, and manual-approver roles. A CI submitter must not implicitly approve its own requests.
3. Configure GitHub.com as a trusted build system, the approved artifact configuration, and a production signing policy requiring manual approval. Grant the SignPath GitHub App only the approved repository if the provider's configuration requires it. Do not grant unrelated repositories.
4. Run the existing GitHub-hosted Windows build/tests in the approved source fork at the exact release commit. Require the source-provenance check to pass first. Copy the just-built editor and updater into a new two-file signing directory and upload it with `actions/upload-artifact`. Pass that step's actual artifact ID to the provider's official submission action. Do not upload binaries assembled on a workstation or fetched from an unrelated workflow as though they were built in that run.
5. Pin the submission action to a reviewed full commit SHA at integration time. Set organization/project/policy identifiers from the approved account. Store the API token only as a protected GitHub Actions secret, scoped to the submitter role. Never put it in source, logs, documentation, or chat.
6. Download the approved signed artifact to a separate directory. Verify both Authenticode signatures, the provider-approved publisher identity and certificate chain, timestamps, metadata, and correspondence to the submitted build. Fail the signing release on missing signatures; never fall back silently to unsigned outputs.
7. Package the signed editor via `package.py --binary <signed NoteX.exe>` and use the signed helper as `dist/NoteXUpdater.exe`. Generate the portable inventory only after signing. Run package and native checks again, then create the Ed25519 update inventory and checksums from these final bytes.
8. Publish a new version with its exact source revision. Do not replace unsigned historical assets. Download and verify the public artifacts and run the real updater path from the previous release. Add provider attribution to the homepage, download page, policy, and new release notes only when it is true.

The current certificate-store script is a separate local signing option; it is not a SignPath API integration. Keep the offline Ed25519 update key independent of Authenticode. Windows signing does not provide Apple Developer ID signing or notarization.

Integration reference: [SignPath GitHub build-system documentation](https://docs.signpath.io/trusted-build-systems/github).
