# Copyright (C) 2026 Berk Kivircik.
# SPDX-License-Identifier: GPL-3.0-only
# Created for NoteX; publication date: 2026-10-28.

"""Run four upstream Function List fixtures against the locally built x64 EXE."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

repo = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=repo/'PowerEditor/bin64/Notepad++.exe')
parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parent/'out/function-list')
args = parser.parse_args()
built = args.binary.resolve(strict=True)
work = args.output.resolve()
work.mkdir(parents=True, exist_ok=True)
app = work / 'smoke-app'
profile = work / 'smoke-profile'
fixtures = work / 'smoke-fixtures'
for p in (app, profile, fixtures):
    p.mkdir(exist_ok=True)
shutil.copy2(built, app / 'notepad++.exe')
for name in ('langs.model.xml', 'stylers.model.xml'):
    shutil.copy2(repo / 'PowerEditor/src' / name, app / name)
shutil.copytree(repo / 'PowerEditor/installer/functionList', app / 'functionList', dirs_exist_ok=True)
(app / 'doLocalConf.xml').write_bytes(b'')

results = []
for lang in ('cpp', 'python', 'javascript', 'xml'):
    case = fixtures / lang
    case.mkdir(exist_ok=True)
    upstream = repo / 'PowerEditor/Test/FunctionList' / lang
    for name in ('unitTest', 'unitTest.expected.result'):
        shutil.copy2(upstream / name, case / name)
    output = case / 'unitTest.result.json'
    if output.exists():
        # Only this script's previously generated fixture output.
        assert output.resolve().is_relative_to(fixtures.resolve())
        output.unlink()
    args = [str(app / 'notepad++.exe'), '-multiInst', '-nosession', '-noPlugin',
            f'-settingsDir={profile}', '-export=functionList', f'-l{lang}', str(case / 'unitTest')]
    started = time.monotonic()
    proc = subprocess.Popen(args, cwd=app, creationflags=subprocess.CREATE_NO_WINDOW)
    timeout = False
    try:
        code = proc.wait(timeout=40)
    except subprocess.TimeoutExpired:
        timeout = True
        proc.kill()  # The exact child process handle created above.
        code = proc.wait(timeout=10)
    actual = output.read_text(encoding='utf-8-sig') if output.exists() else None
    expected = (case / 'unitTest.expected.result').read_text(encoding='utf-8-sig')
    normalize = lambda s: s.replace('\\r\\n', '\\n').replace('\r\n', '\n').strip()
    matched = actual is not None and normalize(actual) == normalize(expected)
    result = {'language': lang, 'exit_code': code, 'timeout': timeout, 'output_exists': output.exists(),
              'matches_upstream_expected': matched, 'seconds': round(time.monotonic() - started, 3)}
    results.append(result)
    print(json.dumps(result), flush=True)

summary = {'binary': str(built), 'sha256': hashlib.sha256(built.read_bytes()).hexdigest(),
           'scope': 'four Function List export smoke fixtures; not full GUI/plugin acceptance',
           'results': results, 'passed': all(x['matches_upstream_expected'] and x['exit_code'] == 0 and not x['timeout'] for x in results)}
(work / 'build-smoke-results.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
raise SystemExit(0 if summary['passed'] else 1)

