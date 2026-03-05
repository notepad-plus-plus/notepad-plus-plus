#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SAMPLES_DIR="${ROOT_DIR}/build/phase4b_smoke_samples"
APP_PATH="${ROOT_DIR}/build/bin/notepadpp.app"
PREPARE_ONLY="${1:-}"

mkdir -p "${SAMPLES_DIR}"

cat > "${SAMPLES_DIR}/sample.cpp" <<'CPP'
#include <iostream>
int main() {
  std::cout << "hello" << std::endl;
  return 0;
}
CPP

cat > "${SAMPLES_DIR}/sample.py" <<'PY'
def greet(name: str) -> str:
    return f"hello {name}"

print(greet("world"))
PY

cat > "${SAMPLES_DIR}/sample.js" <<'JS'
const add = (a, b) => a + b;
console.log(add(2, 3));
JS

cat > "${SAMPLES_DIR}/sample.ts" <<'TS'
function add(a: number, b: number): number {
  return a + b;
}
console.log(add(2, 3));
TS

cat > "${SAMPLES_DIR}/sample.html" <<'HTML'
<!doctype html>
<html><body><h1>Hello</h1></body></html>
HTML

cat > "${SAMPLES_DIR}/sample.xml" <<'XML'
<?xml version="1.0"?>
<root><item key="value">text</item></root>
XML

cat > "${SAMPLES_DIR}/sample.sql" <<'SQL'
SELECT id, name FROM users WHERE active = 1 ORDER BY name;
SQL

cat > "${SAMPLES_DIR}/sample.rs" <<'RS'
fn main() {
    println!("hello");
}
RS

cat > "${SAMPLES_DIR}/sample.sh" <<'SH'
#!/usr/bin/env bash
echo "hello"
SH

cat > "${SAMPLES_DIR}/sample.toml" <<'TOML'
name = "notepadpp"
version = "0.1.0"
TOML

cat > "${SAMPLES_DIR}/sample.yaml" <<'YAML'
name: notepadpp
version: 0.1.0
YAML

cat > "${SAMPLES_DIR}/sample.md" <<'MD'
# Heading

`code` and **bold**.
MD

echo "Prepared smoke files in: ${SAMPLES_DIR}"

if [[ "${PREPARE_ONLY}" == "--prepare-only" ]]; then
  echo "Prepare-only mode: not launching app."
  exit 0
fi

if [[ ! -d "${APP_PATH}" ]]; then
  echo "App bundle not found at ${APP_PATH}"
  echo "Build first: ./build_macos.sh Debug arm64"
  exit 1
fi

echo "Launching Notepad++ with smoke files..."
for f in "${SAMPLES_DIR}"/*; do
  echo "  open: $(basename "$f")"
  open -a "${APP_PATH}" "$f"
done

echo "Done. Use PHASE4B_IN_PROGRESS.md checklist to mark pass/fail."
