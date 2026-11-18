# Copyright (C) 2026 Berk Kivircik.
# SPDX-License-Identifier: GPL-3.0-only
# Modified for NoteX publication: 2026-11-18.
"""Build a fresh NoteX portable package from reviewed source-tree inputs."""
from binary_metadata import require_metadata

import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import xml.etree.ElementTree as ET


def require_x64(file):
    data = file.read_bytes()
    if len(data) < 64 or data[:2] != b'MZ':
        raise ValueError(f'Not a PE binary: {file.name}')
    pe = struct.unpack_from('<I', data, 60)[0]
    if (pe + 6 > len(data) or data[pe:pe + 4] != b'PE\0\0'
            or struct.unpack_from('<H', data, pe + 4)[0] != 0x8664):
        raise ValueError('Expected a Windows x64 executable.')


def main():
    packaging = Path(__file__).resolve().parent
    repo = packaging.parents[2]
    assets = packaging / 'assets'
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=repo / 'dist/NoteX')
    parser.add_argument('--binary', type=Path, default=repo / 'PowerEditor/bin64/notepad++.exe')
    args = parser.parse_args()
    target = args.output.resolve()
    binary = args.binary.resolve(strict=True)
    if target.exists() and (not target.is_dir() or any(target.iterdir())):
        parser.error('Output must be a new or empty directory. Never overwrite an active profile.')
    require_x64(binary)
    require_metadata(binary, "1.6.2")
    require_metadata(repo / "dist/NoteXUpdater.exe", "1.6.2", updater=True)
    source_ref = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip()
    if subprocess.run(['git', 'diff', '--quiet', 'HEAD', '--'], cwd=repo).returncode:
        parser.error('Commit source changes before packaging so the source reference is accurate.')
    languages = ('english.xml', 'turkish.xml', 'spanish.xml')
    for name in languages:
        ET.parse(repo / 'PowerEditor/installer/nativeLang' / name)
    ET.parse(assets / 'config.xml')
    target.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binary, target / 'NoteX.exe')
    shutil.copy2(repo / 'dist/NoteXUpdater.exe', target / 'NoteXUpdater.exe')
    for name in ('UPDATING.md',):
        shutil.copy2(repo / name, target / name)
    for name in ('langs.model.xml', 'stylers.model.xml'):
        shutil.copy2(repo / 'PowerEditor/src' / name, target / name)
    for source, output in (('APIs', 'autoCompletion'), ('functionList', 'functionList'), ('themes', 'themes')):
        shutil.copytree(repo / 'PowerEditor/installer' / source, target / output)
    (target / 'localization').mkdir()
    for name in languages:
        shutil.copy2(repo / 'PowerEditor/installer/nativeLang' / name, target / 'localization' / name)
    # Only these reviewed asset directories are allowed; never copy an entire profile.
    for name in ('fonts', 'themes', 'userDefineLangs'):
        shutil.copytree(assets / name, target / name, dirs_exist_ok=True)
    shutil.copy2(assets / 'config.xml', target / 'config.xml')
    for name in ('doLocalConf.xml', 'disableNppAutoUpdate.xml'):
        (target / name).write_bytes(b'')
    shutil.copy2(repo / 'LICENSE', target / 'LICENSE.txt')
    for name in ('README.md', 'CHANGELOG.md', 'MODIFICATIONS.md', 'THIRD_PARTY_NOTICES.md', 'BUILD.md'):
        shutil.copy2(repo / name, target / name)
    shutil.copytree(repo / 'LICENSES', target / 'LICENSES')
    (target / 'SOURCE_CODE.txt').write_text(
        'NoteX 1.6.2 - corresponding source\n\n'
        f'This package was prepared from source commit: {source_ref}\n'
        f'Browse: https://github.com/berkkivircik/NoteX/tree/{source_ref}\n'
        f'Download: https://github.com/berkkivircik/NoteX/archive/{source_ref}.zip\n'
        'Release source: https://github.com/berkkivircik/NoteX/releases/tag/v1.6.2\n\n'
        'The matching source includes the modified editor, embedded libraries, tests,\n'
        'build scripts, packaging inputs, and licenses. See BUILD.md to compile and\n'
        'install modified versions. Source is available without an additional charge.\n'
        'NoteX is distributed under GNU GPL version 3, without warranty.\n'
        'Original component and font notices are included in LICENSES and fonts.\n', encoding='utf-8')
    manifest = {
        'version': 'NoteX 1.6.2',
        'configuration': 'Release',
        'architecture': 'x64',
        'created_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'source_commit': source_ref,
        'ui_languages': ['en', 'tr', 'es'],
        'files': {p.relative_to(target).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in sorted(target.rglob('*')) if p.is_file()},
    }
    (target / 'release-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(json.dumps({'package': str(target), 'files': len(manifest['files']), 'source_commit': source_ref}))


if __name__ == '__main__':
    main()
