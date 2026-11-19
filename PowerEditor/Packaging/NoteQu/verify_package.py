# Copyright (C) 2026 Berk Kivircik.
# SPDX-License-Identifier: GPL-3.0-only
# Modified for NoteQu on 2026-11-18: validate compiled application identity.
"""Validate the public package manifest, clean profile, and license inputs."""
from binary_metadata import require_metadata

import argparse
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package', type=Path)
    args = parser.parse_args()
    root = args.package.resolve(strict=True)
    manifest = json.loads((root / 'release-manifest.json').read_text(encoding='utf-8'))
    require_metadata(root / 'NoteQu.exe', manifest['version'].removeprefix('NoteQu '))
    require_metadata(root / 'NoteQuUpdater.exe', manifest['version'].removeprefix('NoteQu '), updater=True)
    expected = manifest['files']
    actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
    if actual != set(expected) | {'release-manifest.json'}:
        raise ValueError('Package contains missing or unexpected files.')
    for name, digest in expected.items():
        file = root / name
        if not file.resolve().is_relative_to(root) or file.is_symlink():
            raise ValueError('Package contains an unsafe path.')
        if hashlib.sha256(file.read_bytes()).hexdigest() != digest:
            raise ValueError(f'Package hash mismatch: {name}')
        parts = {p.lower() for p in Path(name).parts}
        base = file.name.lower()
        if (parts & {'backup', 'plugins', 'updater', 'validation', '.git', '.claude-flow'}
                or base.startswith('session.xml') or base.endswith(('.notex', '.bak', '.log', '.pdb'))
                or base in {'notex-menu.cfg', 'notequ-menu.cfg', 'notequ-format-toolbar.cfg',
                            'notex-updates.json', 'shortcuts.xml', 'nativelang.xml'}
                or base.startswith('notequ-format-toolbar.cfg.tmp-')):
            raise ValueError(f'Private or unreviewed runtime input: {name}')
    if {p.name for p in (root / 'localization').iterdir()} != {'english.xml', 'turkish.xml', 'spanish.xml'}:
        raise ValueError('Unexpected interface-language set.')
    config = ET.parse(root / 'config.xml').getroot()
    if config.find('History') is not None or config.find('FindHistory') is not None:
        raise ValueError('Configuration contains history.')
    configs = {e.attrib['name']: e for e in config.findall('./GUIConfigs/GUIConfig')}
    if configs['MISC'].attrib['writeTechnologyEngine'] != '0':
        raise ValueError('Expected GDI rendering default.')
    if configs['Backup'].attrib['dir'] or configs['Backup'].attrib['isSnapshotMode'] != 'yes':
        raise ValueError('Unexpected snapshot configuration.')
    if configs['AutoUpdate'].text != 'no':
        raise ValueError('Upstream automatic updates must be disabled.')
    if not re.fullmatch('[0-9a-f]{40}', manifest['source_commit']):
        raise ValueError('Missing exact source revision.')
    if manifest['source_commit'] not in (root / 'SOURCE_CODE.txt').read_text():
        raise ValueError('Source instructions do not identify the manifest revision.')
    if 'GNU GENERAL PUBLIC LICENSE' not in (root / 'LICENSE.txt').read_text():
        raise ValueError('Missing GPL license text.')
    for font in ('Inter', 'JetBrainsMono'):
        notice = (root / 'fonts' / (font + '.ttf.LICENSE.txt')).read_text()
        if 'SIL OPEN FONT LICENSE Version 1.1' not in notice or len(notice) < 3500:
            raise ValueError('Incomplete font license.')
    print(json.dumps({'passed': True, 'files_verified': len(expected),
                      'source_commit': manifest['source_commit'], 'profile': 'fresh'}))


if __name__ == '__main__':
    main()
