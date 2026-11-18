# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""Remove only the two bundled selection-upload commands without reserializing XML."""
from pathlib import Path
import argparse
import hashlib
import os
import tempfile
from xml.parsers import expat

REMOVED = {
    'Get PHP help': 'https://www.php.net/$(CURRENT_WORD)',
    'Wikipedia Search': 'https://en.wikipedia.org/wiki/Special:Search?search=$(CURRENT_WORD)',
}

def migrate(data):
    data.decode("utf-8-sig")  # Fail safely on unsupported encodings; byte offsets must remain exact.
    parser = expat.ParserCreate()
    stack, spans = [], []
    record = None
    def start(name, attrs):
        nonlocal record
        stack.append(name)
        if stack == ['NotepadPlus', 'UserDefinedCommands', 'Command']:
            record = [parser.CurrentByteIndex, attrs.get('name'), '']
    def text(value):
        if record is not None:
            record[2] += value
    def end(name):
        nonlocal record
        if stack == ['NotepadPlus', 'UserDefinedCommands', 'Command']:
            if record[1] in REMOVED and record[2] == REMOVED[record[1]]:
                finish = data.index(b'>', parser.CurrentByteIndex) + 1
                spans.append((record[0], finish))
            record = None
        stack.pop()
    parser.StartElementHandler, parser.CharacterDataHandler, parser.EndElementHandler = start, text, end
    parser.StartDoctypeDeclHandler = lambda *args: (_ for _ in ()).throw(ValueError('DOCTYPE is not supported.'))
    parser.Parse(data, True)
    result = data
    for first, last in reversed(spans):
        result = result[:first] + result[last:]
    return result, len(spans)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('shortcuts', type=Path)
    p.add_argument('--backup', type=Path, required=True)
    args = p.parse_args()
    original = args.shortcuts.read_bytes()
    changed, count = migrate(original)
    if not count:
        print('No bundled selection-upload commands found.')
        return
    # Caller must first close the application normally. Never print personal command contents.
    with args.backup.open('xb') as backup:
        backup.write(original)
    fd, temporary = tempfile.mkstemp(prefix='.notex-shortcuts-', dir=args.shortcuts.parent)
    try:
        with os.fdopen(fd, 'wb') as output:
            output.write(changed)
            output.flush()
            os.fsync(output.fileno())
        if hashlib.sha256(args.shortcuts.read_bytes()).digest() != hashlib.sha256(original).digest():
            raise RuntimeError('Shortcuts changed during migration; no replacement was made.')
        os.replace(temporary, args.shortcuts)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    print(f'Removed {count} bundled commands. All other bytes were preserved.')

if __name__ == '__main__':
    main()
