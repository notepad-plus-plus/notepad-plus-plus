#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPORT_DIR="${ROOT_DIR}/build/reports"
SAMPLES_DIR="${ROOT_DIR}/build/phase4b_smoke_samples"
BINARY="${ROOT_DIR}/build/bin/notepadpp.app/Contents/MacOS/notepadpp"
TS="$(date +%Y%m%d_%H%M%S)"
REPORT_PATH="${REPORT_DIR}/phase4b_auto_validate_${TS}.md"
TMP_DIR="$(mktemp -d)"
RESULTS_TSV="${TMP_DIR}/results.tsv"

cleanup() {
  rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

mkdir -p "${REPORT_DIR}"

make -C "${ROOT_DIR}/build" -j"$(sysctl -n hw.ncpu)" >/dev/null
"${ROOT_DIR}/scripts/phase4b_smoke.sh" --prepare-only >/dev/null

if [[ ! -x "${BINARY}" ]]; then
  echo "Binary not found: ${BINARY}" >&2
  exit 1
fi

FILES="sample.cpp sample.py sample.js sample.ts sample.html sample.xml sample.sql sample.sh sample.rs sample.toml sample.yaml sample.md script_python CMakeLists.txt Makefile plain_text_note sample_crlf.txt sample_utf16.txt"
overall="PASS"

expected_lexer_for_sample() {
  case "$1" in
    sample.cpp) echo "cpp" ;;
    sample.py) echo "python" ;;
    sample.js) echo "cpp" ;;
    sample.ts) echo "cpp" ;;
    sample.html) echo "hypertext" ;;
    sample.xml) echo "xml" ;;
    sample.sql) echo "sql" ;;
    sample.sh) echo "bash" ;;
    sample.rs) echo "rust" ;;
    sample.toml) echo "toml" ;;
    sample.yaml) echo "yaml" ;;
    sample.md) echo "markdown" ;;
    script_python) echo "python" ;;
    CMakeLists.txt) echo "cmake" ;;
    Makefile) echo "makefile" ;;
    plain_text_note) echo "null" ;;
    sample_crlf.txt) echo "null" ;;
    sample_utf16.txt) echo "null" ;;
    *) echo "null" ;;
  esac
}

expected_source_for_sample() {
  case "$1" in
    script_python) echo "shebang" ;;
    CMakeLists.txt) echo "filename" ;;
    Makefile) echo "filename" ;;
    plain_text_note) echo "default" ;;
    sample_crlf.txt) echo "extension" ;;
    sample_utf16.txt) echo "extension" ;;
    *) echo "extension" ;;
  esac
}

expected_eol_for_sample() {
  case "$1" in
    sample_crlf.txt) echo "CRLF" ;;
    *) echo "LF" ;;
  esac
}

expected_encoding_for_sample() {
  case "$1" in
    sample_utf16.txt) echo "UTF-16" ;;
    *) echo "UTF-8" ;;
  esac
}

extract_value() {
  local key="$1"
  local file="$2"
  grep -F "[NPP][SelfTest] ${key}=" "${file}" | tail -n 1 | sed -E "s/.*${key}=//"
}

extract_status_snapshot() {
  local file="$1"
  grep -F "[NPP][SelfTest] STATUS=" "${file}" | tail -n 1 | sed -E 's/.*STATUS=//'
}

: > "${RESULTS_TSV}"

for sample in ${FILES}; do
  sample_path="${SAMPLES_DIR}/${sample}"
  run_log="${TMP_DIR}/${sample}.log"
  expected_lexer="$(expected_lexer_for_sample "${sample}")"
  expected_source="$(expected_source_for_sample "${sample}")"
  expected_eol="$(expected_eol_for_sample "${sample}")"
  expected_encoding="$(expected_encoding_for_sample "${sample}")"

  if [[ ! -f "${sample_path}" ]]; then
    echo "${sample}|${expected_lexer}|${expected_source}|${expected_eol}|${expected_encoding}|(missing file)|unknown|unknown|unknown|unknown|FAIL|FAIL|FAIL|FAIL|FAIL|FAIL|FAIL" >> "${RESULTS_TSV}"
    overall="FAIL"
    continue
  fi

  set +e
  NPP_SELFTEST=1 NPP_SELFTEST_FILE="${sample_path}" "${BINARY}" >"${run_log}" 2>&1
  app_exit=$?
  set -e

  lexer="$(extract_value "LEXER" "${run_log}" || true)"
  lang_source="$(extract_value "LANG_SOURCE" "${run_log}" || true)"
  status_snapshot="$(extract_status_snapshot "${run_log}" || true)"
  modify_before="$(extract_value "MODIFY_BEFORE" "${run_log}" || true)"
  modify_after_insert="$(extract_value "MODIFY_AFTER_INSERT" "${run_log}" || true)"
  modify_after_savepoint="$(extract_value "MODIFY_AFTER_SAVEPOINT" "${run_log}" || true)"

  if [[ -z "${lexer}" ]]; then
    lexer="unknown"
  fi
  if [[ -z "${lang_source}" ]]; then
    lang_source="unknown"
  fi
  if [[ -z "${status_snapshot}" ]]; then
    status_snapshot="unknown"
  fi
  actual_eol="$(printf '%s' "${status_snapshot}" | sed -n -E 's/.*eol:([^;]+).*/\1/p')"
  actual_encoding="$(printf '%s' "${status_snapshot}" | sed -n -E 's/.*encoding:([^;]+).*/\1/p')"
  if [[ -z "${actual_eol}" ]]; then
    actual_eol="unknown"
  fi
  if [[ -z "${actual_encoding}" ]]; then
    actual_encoding="unknown"
  fi

  if [[ "${lexer}" == "${expected_lexer}" ]]; then
    lexer_status="PASS"
  else
    lexer_status="FAIL"
  fi

  if [[ "${lang_source}" == "${expected_source}" ]]; then
    source_status="PASS"
  else
    source_status="FAIL"
  fi

  if [[ "${actual_eol}" == "${expected_eol}" ]]; then
    eol_status="PASS"
  else
    eol_status="FAIL"
  fi

  if [[ "${actual_encoding}" == "${expected_encoding}" ]]; then
    encoding_status="PASS"
  else
    encoding_status="FAIL"
  fi

  if [[ "${modify_before}" == "0" && "${modify_after_insert}" == "1" && "${modify_after_savepoint}" == "0" ]]; then
    modify_status="PASS"
  else
    modify_status="FAIL"
  fi

  if grep -Fq "[NPP][SelfTest] PASS" "${run_log}"; then
    selftest_tag="PASS"
  else
    selftest_tag="FAIL"
  fi

  if [[ "${app_exit}" == "0" ]]; then
    exit_status="PASS"
  else
    exit_status="FAIL"
  fi

  if [[ "${lexer_status}" == "PASS" && "${source_status}" == "PASS" && "${eol_status}" == "PASS" && "${encoding_status}" == "PASS" && "${modify_status}" == "PASS" && "${selftest_tag}" == "PASS" && "${exit_status}" == "PASS" ]]; then
    result="PASS"
  else
    result="FAIL"
    overall="FAIL"
  fi

  echo "${sample}|${expected_lexer}|${expected_source}|${expected_eol}|${expected_encoding}|${lexer}|${lang_source}|${actual_eol}|${actual_encoding}|${status_snapshot}|${lexer_status}|${source_status}|${eol_status}|${encoding_status}|${modify_status}|${selftest_tag}|${exit_status}|${result}" >> "${RESULTS_TSV}"
done

{
  printf "# Phase 4B Auto Validation\n\n"
  printf -- "- Timestamp: %s\n" "$(date)"
  printf -- "- Build: PASS\n"
  printf -- "- Validation Mode: internal self-test (no Accessibility automation)\n"
  printf -- "- Report: %s\n\n" "${REPORT_PATH}"

  printf "## Machine Summary (PASS/FAIL Matrix)\n\n"
  printf "| Sample | Expected Lexer | Expected Source | Expected EOL | Expected Encoding | Actual Lexer | Actual Source | Actual EOL | Actual Encoding | Status Snapshot | Lexer Routing | Source Routing | EOL Routing | Encoding Routing | 0->1->0 Transition | SelfTest Tag | Exit Code | Result |\n"
  printf "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |\n"

  while IFS='|' read -r sample expected_lexer expected_source expected_eol expected_encoding actual_lexer actual_source actual_eol actual_encoding status_snapshot lexer_status source_status eol_status encoding_status modify_status selftest_tag exit_status result; do
    printf "| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |\n" \
      "${sample}" "${expected_lexer}" "${expected_source}" "${expected_eol}" "${expected_encoding}" "${actual_lexer}" "${actual_source}" "${actual_eol}" "${actual_encoding}" "${status_snapshot}" "${lexer_status}" "${source_status}" "${eol_status}" "${encoding_status}" "${modify_status}" "${selftest_tag}" "${exit_status}" "${result}"
  done < "${RESULTS_TSV}"

  printf "\nFINAL_RESULT=%s\n\n" "${overall}"

  printf "## Human Checklist\n\n"
  printf -- "- [ ] Visual syntax quality checked for: C++, Python, JS/TS, HTML/XML, SQL, Bash, Rust, Markdown, JSON, YAML, TOML\n"
  printf -- "- [ ] Window title language indicator checked on file open\n"
  printf -- "- [ ] Status bar language + Edited/Saved state checked manually\n"

  printf "\n## Notes\n\n"
  printf -- "- Automated Phase 4B gate requires FINAL_RESULT=PASS.\n"
  printf -- "- Full Phase 4B completion additionally requires all manual checklist items marked done.\n"
} > "${REPORT_PATH}"

echo "Created: ${REPORT_PATH}"
