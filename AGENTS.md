# AGENTS.md

## Before making changes

- Read [Contribution Rules](CONTRIBUTING.md) and follow its requirements.
- Check the relevant documentation and existing code patterns before editing.
- Keep changes focused and small, avoid unrelated refactoring or formatting.

## Project constraints

- Do not modify `scintilla/` or `lexilla/`.
- Do not submit refactoring-only or typo-fix-only changes. They will be rejected.
- Support Windows only, do not add support for other platforms.

## Validation

- Enable GitHub Actions on your fork.
- Submit a PR only after all required build workflows pass.
- Run the relevant build and test it locally before opening a PR.

## PR instructions

- Use [pull request template](.github/pull_request_template.md).
- Contributors must manually check the confirmation box in the PR template.
- Link every PR to an issue using `fix #NNNNN`.
- Disclose AI assistance, including the model. If possible also the prompt.
- Include AI agent as a co-author using a `Co-Authored-By` trailer.
- Contributors are encouraged to write their own PR descriptions.
- Comments and replies should be written by the contributors themselves.
