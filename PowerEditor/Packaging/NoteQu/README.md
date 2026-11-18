# NoteQu portable packaging

Run `python PowerEditor/Packaging/NoteQu/package.py --output dist/NoteQu` after
building. See the root BUILD.md and RELEASING.md. All inputs are in this source
tree. The historical directory name remains for development compatibility.

Assets contain fresh configuration defaults, the NoteQu-compatible theme,
Inter and JetBrains Mono with complete OFL notices, and Markdown UDL definitions.
The public package includes no optional plugins, updater, screenshots, private
documents, session backups, validation logs, or machine-specific release reports.
Never use a personal Release directory as input or output.
