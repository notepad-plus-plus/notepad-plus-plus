#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPORT_DIR="${ROOT_DIR}/build/reports"
SAMPLES_DIR="${ROOT_DIR}/build/phase4b_smoke_samples"
TS="$(date +%Y%m%d_%H%M%S)"
REPORT_PATH="${REPORT_DIR}/phase4b_smoke_${TS}.md"

mkdir -p "${REPORT_DIR}"
mkdir -p "${SAMPLES_DIR}"

if [[ ! -f "${SAMPLES_DIR}/sample.cpp" ]]; then
  "${ROOT_DIR}/scripts/phase4b_smoke.sh" --prepare-only >/dev/null
fi

{
  printf "# Phase 4B Smoke Report\n\n"
  printf -- "- Timestamp: %s\n" "$(date)"
  printf -- "- Workspace: %s\n" "${ROOT_DIR}"
  printf -- "- Binary: %s\n\n" "${ROOT_DIR}/build/bin/notepadpp.app/Contents/MacOS/notepadpp"
  cat <<'REPORT'
## Checklist

- [ ] Open `sample.cpp` and confirm keyword/comment/string highlighting.
- [ ] Open `sample.py` and confirm Python keyword highlighting.
- [ ] Open `sample.js` and confirm JS keyword highlighting.
- [ ] Open `sample.ts` and confirm TS keyword highlighting.
- [ ] Open `sample.html` and confirm tag/attribute/string highlighting.
- [ ] Open `sample.xml` and confirm XML highlighting.
- [ ] Open `sample.sql` and confirm SQL keyword highlighting.
- [ ] Open `sample.sh` and confirm shell keyword/comment highlighting.
- [ ] Open `sample.rs` and confirm Rust keyword highlighting.
- [ ] Edit in each file and verify title updates with `Edited`.
- [ ] Save and verify `Edited` clears.
- [ ] Undo/redo around savepoint and verify modified-state remains correct.

## Notes

- Build command used:
  - `./build_macos.sh Debug arm64`
- Sample directory:
REPORT
  printf '  - `%s`\n\n' "${SAMPLES_DIR}"
  cat <<'REPORT'
## Failures / Observations

- 
REPORT
} > "${REPORT_PATH}"

echo "Created report template: ${REPORT_PATH}"
