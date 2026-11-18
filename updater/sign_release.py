# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""Offline Windows release signer. Private Ed25519 keys remain DPAPI encrypted outside the repository."""
import argparse
import base64
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import plistlib
import zipfile
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding, PrivateFormat, PublicFormat, NoEncryption

class Blob(ctypes.Structure):
    _fields_ = [('size', wintypes.DWORD), ('data', ctypes.POINTER(ctypes.c_byte))]

def dpapi(data, encrypt):
    if os.name != 'nt':
        raise RuntimeError('The offline signing key is protected by Windows DPAPI.')
    buf = ctypes.create_string_buffer(data)
    source = Blob(len(data), ctypes.cast(buf, ctypes.POINTER(ctypes.c_byte)))
    target = Blob()
    crypt = ctypes.WinDLL('crypt32', use_last_error=True)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    fn = crypt.CryptProtectData if encrypt else crypt.CryptUnprotectData
    if not fn(ctypes.byref(source), None, None, None, None, 1, ctypes.byref(target)):
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        return ctypes.string_at(target.data, target.size)
    finally:
        kernel.LocalFree.argtypes = [ctypes.c_void_p]
        kernel.LocalFree(target.data)

def load_key():
    # Preserve the existing verification key across the product rename.
    location = Path(os.environ['LOCALAPPDATA']) / 'NoteXReleaseSigning'
    location.mkdir(exist_ok=True)
    # DPAPI already binds decryption to this Windows account. Tighten directory ACLs as well.
    identity = subprocess.check_output(['whoami', '/user', '/fo', 'csv', '/nh'], text=True).strip().split(',')[-1].strip('"')
    subprocess.run(['icacls', str(location), '/inheritance:r', '/grant:r', f'*{identity}:(OI)(CI)F'], check=True, stdout=subprocess.DEVNULL)
    keyfile = location / 'release-key.dpapi'
    if keyfile.exists():
        return Ed25519PrivateKey.from_private_bytes(dpapi(keyfile.read_bytes(), False))
    key = Ed25519PrivateKey.generate()
    with keyfile.open('xb') as file:
        file.write(dpapi(key.private_bytes(Encoding.Raw, PrivateFormat.Raw, NoEncryption()), True))
    return key

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--init-key', action='store_true')
    parser.add_argument('--root', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--platform', choices=['windows', 'darwin'])
    parser.add_argument('--architecture', choices=['amd64', 'arm64'])
    parser.add_argument('--version')
    parser.add_argument('--source')
    args = parser.parse_args()
    key = load_key()
    public = base64.b64encode(key.public_key().public_bytes(Encoding.Raw, PublicFormat.Raw)).decode()
    repo = Path(__file__).resolve().parents[1]
    if args.init_key:
        target = repo / 'updater/trust.go'
        content = '// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only\npackage main\nconst trustedKey = "' + public + '"\n'
        if target.exists() and target.read_text() != content:
            raise RuntimeError('Existing trust anchor differs. Key rotation requires a reviewed transition.')
        target.write_text(content, encoding='utf-8')
        print('Public verification key initialized. Private key remains encrypted outside the repository.')
        return
    if public not in (repo / 'updater/trust.go').read_text():
        raise RuntimeError('Signing identity does not match the compiled trust anchor.')
    if not all([args.root, args.output, args.platform, args.architecture, args.version, args.source]):
        parser.error('All package fields are required.')
    if subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip() != args.source:
        raise RuntimeError('Source revision differs from the reviewed checkout.')
    if subprocess.check_output(['git', 'status', '--porcelain'], cwd=repo, text=True).strip():
        raise RuntimeError('Commit and review all public source changes before signing.')
    if args.platform == 'windows':
        sys.path.insert(0, str(repo / 'PowerEditor/Packaging/NoteQu'))
        from binary_metadata import require_metadata
        require_metadata(args.root / 'NoteQu.exe', args.version)
        require_metadata(args.root / 'NoteQuUpdater.exe', args.version, updater=True)
        if args.source not in (args.root / 'SOURCE_CODE.txt').read_text():
            raise RuntimeError('Windows package source revision mismatch.')
    else:
        info = plistlib.loads((args.root / 'Contents/Info.plist').read_bytes())
        if info['CFBundleShortVersionString'] != args.version or args.source not in (args.root / 'Contents/Resources/SOURCE.txt').read_text():
            raise RuntimeError('macOS package version or source revision mismatch.')
    allowed = {'NoteQu.exe', 'NoteQuUpdater.exe', 'LICENSE.txt', 'README.md', 'CHANGELOG.md', 'MODIFICATIONS.md',
               'THIRD_PARTY_NOTICES.md', 'BUILD.md', 'SOURCE_CODE.txt', 'UPDATING.md', 'UPDATE-BOOTSTRAP.md'}
    files = {}
    for p in sorted(args.root.rglob('*')):
        if p.is_symlink():
            raise RuntimeError('Symlinks are not supported in update payloads.')
        if not p.is_file():
            continue
        name = p.relative_to(args.root).as_posix()
        if args.platform == 'windows' and name not in allowed and not name.startswith('LICENSES/'):
            continue
        if args.platform == 'darwin' and not name.startswith('Contents/'):
            continue
        data = p.read_bytes()
        files[name] = {'sha256': hashlib.sha256(data).hexdigest(), 'size': len(data),
                       'executable': name.startswith('Contents/MacOS/') or name.endswith('.exe')}
    name = f'NoteQu-{args.version}-{args.platform}-{args.architecture}-update.zip'
    manifest = {'schema': 1, 'repository': 'berkkivircik/NoteQu', 'version': args.version,
                'platform': args.platform, 'architecture': args.architecture, 'source': args.source,
                'url': f'https://github.com/berkkivircik/NoteQu/releases/download/v{args.version}/{name}', 'files': files}
    args.output.mkdir(parents=True, exist_ok=True)
    package = args.output / name
    with zipfile.ZipFile(package, 'x', zipfile.ZIP_DEFLATED) as archive:
        for name in files:
            archive.write(args.root / name, name)
    raw = json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()
    signature = key.sign(raw)
    key.public_key().verify(signature, raw)
    target = args.output / f'NoteQu-update-{args.platform}-{args.architecture}.json'
    with target.open('xb') as file:
        file.write(raw)
    with target.with_suffix('.json.sig').open('xb') as file:
        file.write(signature)
    print(json.dumps({'version': args.version, 'platform': args.platform, 'files': len(files),
                      'package_sha256': hashlib.sha256(package.read_bytes()).hexdigest()}))

if __name__ == '__main__':
    main()
