# Releasing NoteX

1. Update the version, changelog, and dated modification notices. Commit the
   complete source, embedded libraries, tests, build scripts, packaging inputs,
   and notices. Run the clean Release build and native tests.
2. Package to a new directory. Run the Function List smoke fixtures and verify
   that the package has no documents, session.xml, backup directory, history,
   personalized settings, plugin configuration, sidecars, or local reports.
3. Check every packaged file's license. This public recipe excludes optional
   third-party plugin DLLs and the upstream updater. Do not add them without
   reviewing their licenses and providing any required matching source.
4. Tag the exact source revision used for the build. Create a source archive
   with `git archive --format=zip --prefix=NoteX-1.3.1/ -o NoteX-1.3.1-source.zip v1.3.1`.
   Include the complete source archive alongside the portable ZIP in the release.
5. Publish SHA256SUMS.txt and release notes identifying the source tag and commit,
   license, changes, prerequisites, validation scope, and unsigned status.
   Confirm both ZIP assets can be downloaded and their hashes match.
   Include links labeled **Code signing policy** and **Privacy policy** in new
   release notes, pointing to `CODE_SIGNING.md` and `PRIVACY.md`. State the actual
   signing status of that release. Do not claim SignPath sponsorship or signing
   before approval and verification. Historical release notes and assets remain
   unchanged.
6. Keep matching source and build inputs accessible for as long as the binary
   is offered. Under GPLv3 section 6(d), source must be available through
   equivalent access without an additional charge. A link to a changing main
   branch alone does not identify a historical binary's source.

No additional restrictions on recipients' GPL rights may be imposed. Commercial
use and redistribution are permitted under the license. This release uses online
source distribution; it does not substitute a written source offer.

For proposed SignPath enrollment and the required signing-before-inventory
order, see [.signpath/README.md](.signpath/README.md). Those preparation files do
not enable signing or change the current unsigned release process.
