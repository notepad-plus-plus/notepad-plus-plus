# Code signing policy

NoteX release maintenance and approval are owned by the repository maintainer, `berkkivircik`. Changes from other contributors must be reviewed before release. Never place private signing keys, access tokens, or user profiles in source control or build artifacts.

Update manifests use an offline Ed25519 key and the embedded public trust anchor described in UPDATING.md. This signature authenticates release contents; it is separate from Windows Authenticode and Apple Developer ID signing.

## Responsibilities

| Role | Maintainer | Responsibility |
| --- | --- | --- |
| Author and committer | [Berk Kivircik (@berkkivircik)](https://github.com/berkkivircik) | Maintain source, build scripts, tests, and release inputs. |
| Reviewer | [@berkkivircik](https://github.com/berkkivircik) | Review outside contributions, especially changes to build and signing code. |
| Release and signing approver | [@berkkivircik](https://github.com/berkkivircik) | Review the exact source revision, CI results, and artifact before approving each signing request. |

NoteX currently has one designated maintainer. These roles do not imply independent two-person review. Adding another authorized contributor requires an explicit role update. Production signing must require manual approval and MFA for every participating source-control and signing account. Account enrollment and MFA verification are pending prerequisites, not completed certifications.

## Source provenance and application status

NoteX is an independent modified distribution of Notepad++ v8.9.8.1, based on upstream commit `dd40fe45598791e9aa38f3aeb200e10fb3d6338a`. See [the modification notice](MODIFICATIONS.md) and [third-party notices](THIRD_PARTY_NOTICES.md).

The public NoteX repository begins with a reviewed source snapshot. It is not registered as a GitHub fork and does not preserve the upstream commit ancestry. That distinction must be disclosed to SignPath Foundation and resolved before requesting production signing of the modified editor. No acceptance or signing-service affiliation is claimed. Existing tags and published files must remain unchanged.

## Windows public-trust signing

No Windows public-trust signing provider is currently configured. Do not describe an unsigned build as signed or promise that SmartScreen will accept it without a warning. The existing certificate-store signing script verifies the certificate purpose, publisher identity, chain trust, and timestamp before accepting a signed binary.

SignPath Foundation is a potential free signing provider for this open-source project. No application approval, certificate, or affiliation is implied. Its review includes source-to-binary provenance, repository control, project reputation, visible upstream fork provenance, multifactor authentication, release approval, and privacy disclosures. NoteX derives from Notepad++; the public publication repository's upstream relationship must be explicitly reviewed with the provider. Do not rewrite private or public history to manufacture eligibility.

If accepted, integrate the provider's verified CI artifact flow, require maintainer approval for each signature, and publish the provider's required attribution. Sign the executable and updater before constructing signed update inventories. No signing workflow should silently fall back from a requested trusted signature to an unsigned public artifact.

Microsoft Store MSIX distribution is a separate free-signing option. Store signing does not automatically sign the portable EXE published on GitHub. A Store edition would need its own packaging, storage, update behavior, and acceptance tests.

## Privacy

See the [privacy policy](PRIVACY.md) for local storage, automatic update checks, network destinations, user controls, and third-party services. The update checker contacts GitHub automatically by default; therefore a blanket statement that NoteX never communicates unless explicitly requested would be inaccurate.

## Removal

Close NoteX normally before removing the application. Removing the portable Windows program or macOS application must not remove the user's documents or recovery profiles. Users may separately remove their own settings and recovery data after saving any unsaved work.
