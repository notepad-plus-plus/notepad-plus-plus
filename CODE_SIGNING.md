# Code signing policy

NoteX release maintenance and approval are owned by the repository maintainer, `berkkivircik`. Changes from other contributors must be reviewed before release. Never place private signing keys, access tokens, or user profiles in source control or build artifacts.

Update manifests use an offline Ed25519 key and the embedded public trust anchor described in UPDATING.md. This signature authenticates release contents; it is separate from Windows Authenticode and Apple Developer ID signing.

## Windows public-trust signing

No Windows public-trust signing provider is currently configured. Do not describe an unsigned build as signed or promise that SmartScreen will accept it without a warning. The existing certificate-store signing script verifies the certificate purpose, publisher identity, chain trust, and timestamp before accepting a signed binary.

SignPath Foundation is a potential free signing provider for this open-source project. No application approval, certificate, or affiliation is implied. Its review includes source-to-binary provenance, repository control, project reputation, visible upstream fork provenance, multifactor authentication, release approval, and privacy disclosures. NoteX derives from Notepad++; the public publication repository's upstream relationship must be explicitly reviewed with the provider. Do not rewrite private or public history to manufacture eligibility.

If accepted, integrate the provider's verified CI artifact flow, require maintainer approval for each signature, and publish the provider's required attribution. Sign the executable and updater before constructing signed update inventories. No signing workflow should silently fall back from a requested trusted signature to an unsigned public artifact.

Microsoft Store MSIX distribution is a separate free-signing option. Store signing does not automatically sign the portable EXE published on GitHub. A Store edition would need its own packaging, storage, update behavior, and acceptance tests.

## Privacy

The update checker requests public GitHub release metadata and release assets without account credentials. Requests do not contain document text, selections, local paths, session data, or preferences. Automatic checks can be disabled. GitHub receives ordinary connection metadata. User-invoked external tools, plugin operations, and browser links are separate features. The two bundled selection-upload commands have been removed.

## Removal

Close NoteX normally before removing the application. Removing the portable Windows program or macOS application must not remove the user's documents or recovery profiles. Users may separately remove their own settings and recovery data after saving any unsaved work.
