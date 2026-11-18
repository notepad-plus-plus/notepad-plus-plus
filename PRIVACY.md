# Privacy policy

Effective November 18, 2026. This policy describes the NoteQu application and its bundled updater. The public project is maintained by [Berk Kivircik](https://github.com/berkkivircik).

## Documents and local storage

NoteQu edits documents locally. Unsaved documents, session recovery, formatting companions, preferences, and custom commands are stored on the device. The bundled update checker does not upload document contents, selected text, document names, local paths, session data, or personal settings. NoteQu does not include a maintainer-operated analytics or advertising service.

The Windows portable edition normally keeps its profile beside the application; an explicitly selected profile directory can override that location. The native macOS edition stores its profile outside `NoteQu.app`. New profiles use `~/Library/Application Support/NoteQu`. An existing NoteX profile is opened in place if no NoteQu profile exists. Files saved in a cloud-synchronized directory can be synchronized by that provider independently of NoteQu. Clipboard history, recovery copies, and formatting companions can contain document material and should be treated as personal data.

## Automatic update checks

Beginning with version 1.6.0, the bundled updater checks public releases in `berkkivircik/NoteQu` on GitHub. Automatic checks are enabled by default and limited to once per 24 hours per profile. A user can also request an immediate check. The editor remains usable when a check fails.

The updater sends HTTPS GET requests to `api.github.com` for release metadata and to `github.com` for release files. Downloads may redirect to `release-assets.githubusercontent.com` or `objects.githubusercontent.com`. Requests use a fixed NoteQu updater user agent and do not include a GitHub account, access token, persistent NoteQu user identifier, or document/profile data. Requested artifact names identify the relevant operating system, architecture, and release. GitHub and network infrastructure receive ordinary connection information, including an IP address and request timing. See [GitHub's privacy statement](https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement).

Disable background checks with **Automatic Update Checks** in the Windows Help menu or the macOS NoteQu menu. **Check for NoteQu Updates** on Windows and **Check for Updates** on macOS remain available for manual checks. This control does not disable user-requested external tools, browser links, or plugins. A portable installation does not display an installer; review this policy and the update controls before first launch if you want to disable background checks. Whether this presentation meets a signing provider's enrollment requirements remains subject to that provider's review.

## User-requested external actions

Opening a web link launches the configured browser. User-defined Run commands and installed plugins can contact external systems and can use information supplied in their arguments. Their destinations and behavior depend on the command or plugin; review them before use. Plugin management can download third-party files when its required components are present. Optional plugin DLLs and the upstream updater are not included in the clean NoteQu portable distribution.

The two bundled commands that sent selected text to PHP help and Wikipedia have been removed. Built-in editor web-search commands remain inactive. General Run commands and other links remain available.

## Public support and distribution services

GitHub hosts the source, release downloads, and public issue discussions. Information deliberately posted to a public issue is public. Do not include private documents, recovery profiles, passwords, or tokens in an issue or diagnostic attachment. This policy does not replace the policies of GitHub, browser destinations, plugins, or a cloud-storage provider chosen by the user.

SignPath is not currently used by the installed application. If release signing is approved, reviewed build artifacts and their public source/build provenance will be submitted by release automation; users' documents and profiles are not signing inputs. The application will not need a SignPath account. Windows may independently perform certificate, revocation, and reputation checks according to its own security configuration.

## Removal and questions

Save any unsaved work and close NoteQu normally before removing it. Deleting the Windows portable application directory also deletes personal data stored there; preserve documents, profiles, backups, and companions first. On macOS, removing `NoteQu.app` leaves the separate profile intact. Delete that profile only after saving or exporting anything you need.

For questions about application behavior, open a public issue in the NoteQu repository using a synthetic example and no personal data. There is no NoteQu-hosted document account to close or server-side document copy maintained by this project.
