# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""Independently verify public Ed25519 release inventories and update archives."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import re
import zipfile
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey

def verify(manifest, signature, archive, platform, architecture, version):
    trust = (Path(__file__).parent / 'trust.go').read_text()
    key = Ed25519PublicKey.from_public_bytes(base64.b64decode(re.search(r'trustedKey = "([^"]+)"', trust)[1]))
    key.verify(signature, manifest)
    m = json.loads(manifest)
    assert m['repository'] == 'berkkivircik/NoteX' and m['schema'] == 1
    assert (m['platform'], m['architecture'], m['version']) == (platform, architecture, version)
    assert m['url'] == f'https://github.com/berkkivircik/NoteX/releases/download/v{version}/NoteX-{version}-{platform}-{architecture}-update.zip'
    with zipfile.ZipFile(archive) as z:
        assert set(z.namelist()) == set(m['files']) and len(z.namelist()) == len(m['files'])
        for name, spec in m['files'].items():
            assert not name.startswith('/') and '\\' not in name and ':' not in name and '..' not in name.split('/')
            data = z.read(name)
            assert len(data) == spec['size'] and hashlib.sha256(data).hexdigest() == spec['sha256']
    return m

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ['manifest', 'signature', 'archive']: p.add_argument('--' + name, type=Path, required=True)
    for name in ['platform', 'architecture', 'version']: p.add_argument('--' + name, required=True)
    a = p.parse_args()
    m = verify(a.manifest.read_bytes(), a.signature.read_bytes(), a.archive, a.platform, a.architecture, a.version)
    print(json.dumps({'verified': True, 'version': m['version'], 'platform': m['platform'], 'source': m['source'], 'files': len(m['files'])}))
