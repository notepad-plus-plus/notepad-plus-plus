# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""Remove only the two bundled selection-upload commands without reserializing XML."""
from pathlib import Path
import argparse
import hashlib
import hmac
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

def migrate_validation(config, original, changed, machine_key):
    """Preserve an existing valid shortcut trust decision; never validate an untrusted file."""
    config.decode('utf-8-sig')
    previous = hmac.new(machine_key, original, hashlib.sha256).hexdigest().encode()
    replacement = hmac.new(machine_key, changed, hashlib.sha256).hexdigest().encode()
    parser = expat.ParserCreate()
    spans = []
    stack = []
    def start(name, attrs):
        stack.append(name)
        if stack == ['NotepadPlus', 'GUIConfigs', 'GUIConfig'] and attrs.get('name') == 'shortcutsXmlHMAC' and attrs.get('value', '').encode() == previous:
            first = parser.CurrentByteIndex
            last = config.index(b'>', first) + 1
            spans.append((first, last))
    parser.StartElementHandler = start
    parser.EndElementHandler = lambda name: stack.pop()
    parser.StartDoctypeDeclHandler = lambda *args: (_ for _ in ()).throw(ValueError('DOCTYPE is not supported.'))
    parser.Parse(config, True)
    if len(spans) != 1:
        return config
    first, last = spans[0]
    tag = config[first:last]
    if tag.count(previous) != 1:
        return config
    return config[:first] + tag.replace(previous, replacement) + config[last:]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('shortcuts', type=Path)
    p.add_argument('--backup', type=Path, required=True)
    p.add_argument('--config', type=Path, help='Preserve an already valid shortcuts HMAC in this Windows profile.')
    args = p.parse_args()
    original = args.shortcuts.read_bytes()
    changed, count = migrate(original)
    if not count:
        print('No bundled selection-upload commands found.')
        return
    # Caller must first close the application normally. Never print personal command contents.
    with args.backup.open('xb') as backup:
        backup.write(original)
    old_config = new_config = None
    if args.config:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\Microsoft\Cryptography', 0, winreg.KEY_READ | winreg.KEY_WOW64_64KEY) as key:
            machine_key = winreg.QueryValueEx(key, 'MachineGuid')[0].encode('ascii')
        old_config = args.config.read_bytes()
        new_config = migrate_validation(old_config, original, changed, machine_key)
        if new_config != old_config:
            with args.backup.with_name(args.backup.name + '.config').open('xb') as backup:
                backup.write(old_config)
    fd, temporary = tempfile.mkstemp(prefix='.notex-shortcuts-', dir=args.shortcuts.parent)
    try:
        with os.fdopen(fd, 'wb') as output:
            output.write(changed)
            output.flush()
            os.fsync(output.fileno())
        if hashlib.sha256(args.shortcuts.read_bytes()).digest() != hashlib.sha256(original).digest():
            raise RuntimeError('Shortcuts changed during migration; no replacement was made.')
        os.replace(temporary, args.shortcuts)
        if new_config is not None and new_config != old_config:
            if args.config.read_bytes() != old_config:
                # The app must remain closed throughout migration. Restore the original shortcut bytes.
                args.shortcuts.write_bytes(original)
                raise RuntimeError('Configuration changed during migration; shortcut removal was rolled back.')
            fd, temporary_config = tempfile.mkstemp(prefix='.notex-config-', dir=args.config.parent)
            try:
                with os.fdopen(fd, 'wb') as output:
                    output.write(new_config); output.flush(); os.fsync(output.fileno())
                os.replace(temporary_config, args.config)
            finally:
                if os.path.exists(temporary_config): os.unlink(temporary_config)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    print(f'Removed {count} bundled commands. All other bytes were preserved.')

if __name__ == '__main__':
    main()
