# Copyright (C) 2026 Berk Kivircik.
# SPDX-License-Identifier: GPL-3.0-only
# Created for NoteQu on 2026-11-18.
"""Check the built executable's Windows properties and embedded identity."""
import ctypes as C
from ctypes import wintypes as W
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET


def require_metadata(file: Path, version: str, *, updater=False):
    if sys.platform != 'win32':
        raise RuntimeError('Validate Windows executable metadata on Windows.')
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        raise ValueError('Expected a three-part NoteQu release version.')
    api = C.WinDLL('version', use_last_error=True)
    api.GetFileVersionInfoSizeW.argtypes = [W.LPCWSTR, C.POINTER(W.DWORD)]
    api.GetFileVersionInfoSizeW.restype = W.DWORD
    api.GetFileVersionInfoW.argtypes = [W.LPCWSTR, W.DWORD, W.DWORD, W.LPVOID]
    api.GetFileVersionInfoW.restype = W.BOOL
    api.VerQueryValueW.argtypes = [W.LPCVOID, W.LPCWSTR, C.POINTER(W.LPVOID), C.POINTER(W.UINT)]
    api.VerQueryValueW.restype = W.BOOL
    path = str(file.resolve(strict=True))
    size = api.GetFileVersionInfoSizeW(path, None)
    if not size:
        raise ValueError('Executable has no Windows version information.')
    data = C.create_string_buffer(size)
    if not api.GetFileVersionInfoW(path, 0, size, data):
        raise C.WinError(C.get_last_error())

    def query(key):
        pointer, length = W.LPVOID(), W.UINT()
        if not api.VerQueryValueW(data, key, C.byref(pointer), C.byref(length)):
            raise ValueError(f'Missing executable metadata: {key}')
        return pointer, length.value

    fixed, length = query('\\')
    if length < 52:
        raise ValueError('Truncated fixed version information.')
    values = C.cast(fixed, C.POINTER(W.DWORD))
    parts = tuple(map(int, version.split('.'))) + (0,)
    expected_fixed = (parts[0] << 16 | parts[1], parts[2] << 16 | parts[3])
    if values[0] != 0xFEEF04BD or tuple(values[2:4]) != expected_fixed or tuple(values[4:6]) != expected_fixed:
        raise ValueError('Executable numeric version does not match this release.')
    translations, length = query('\\VarFileInfo\\Translation')
    if not length or length % 4:
        raise ValueError('Invalid executable translation table.')
    pairs = C.cast(translations, C.POINTER(W.WORD))
    description_name = 'NoteQu'
    file_name = 'NoteQuUpdater.exe' if updater else 'NoteQu.exe'
    identity_name = 'NoteQu.Updater' if updater else 'NoteQu'
    expected = {'FileDescription': description_name, 'ProductName': 'NoteQu',
                'InternalName': file_name, 'OriginalFilename': file_name,
                'FileVersion': version + '.0', 'ProductVersion': version + '.0'}
    for i in range(0, length // 2, 2):
        table = f'{pairs[i]:04x}{pairs[i + 1]:04x}'
        for key, value in expected.items():
            pointer, count = query(f'\\StringFileInfo\\{table}\\{key}')
            actual = C.wstring_at(pointer, count).rstrip('\0')
            if actual != value:
                raise ValueError(f'Executable {key} must be {value!r}; found {actual!r}.')

    kernel = C.WinDLL('kernel32', use_last_error=True)
    kernel.LoadLibraryExW.argtypes = [W.LPCWSTR, W.HANDLE, W.DWORD]
    kernel.LoadLibraryExW.restype = W.HMODULE
    kernel.FindResourceW.argtypes = [W.HMODULE, W.LPCWSTR, W.LPCWSTR]
    kernel.FindResourceW.restype = W.HANDLE
    kernel.SizeofResource.argtypes = [W.HMODULE, W.HANDLE]
    kernel.SizeofResource.restype = W.DWORD
    kernel.LoadResource.argtypes = [W.HMODULE, W.HANDLE]
    kernel.LoadResource.restype = W.HANDLE
    kernel.LockResource.argtypes = [W.HANDLE]
    kernel.LockResource.restype = W.LPVOID
    kernel.FreeLibrary.argtypes = [W.HMODULE]
    kernel.FreeLibrary.restype = W.BOOL
    # Read resources as data; never execute the candidate or load its imports.
    module = kernel.LoadLibraryExW(path, None, 0x22)
    if not module:
        raise C.WinError(C.get_last_error())
    try:
        resource = kernel.FindResourceW(module, C.cast(1, W.LPCWSTR), C.cast(24, W.LPCWSTR))
        if not resource:
            raise ValueError('Missing embedded application manifest.')
        size = kernel.SizeofResource(module, resource)
        pointer = kernel.LockResource(kernel.LoadResource(module, resource))
        if not size or not pointer:
            raise ValueError('Unreadable embedded application manifest.')
        manifest = ET.fromstring(C.string_at(pointer, size))
        namespace = '{urn:schemas-microsoft-com:asm.v1}'
        identity = manifest.find(namespace + 'assemblyIdentity')
        description = manifest.find(namespace + 'description')
        if identity is None or identity.get('name') != identity_name:
            raise ValueError('Embedded application identity must be NoteQu.')
        if description is None or description.text != description_name:
            raise ValueError('Embedded application description must be NoteQu.')
    finally:
        kernel.FreeLibrary(module)
    return expected


if __name__ == '__main__':
    import argparse
    import json
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    print(json.dumps(require_metadata(args.binary, args.version), indent=2))
