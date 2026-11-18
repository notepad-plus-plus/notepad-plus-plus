# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""Native Windows integration-test driver. Only controls the specified test executable."""
import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
import time

u, k = c.WinDLL('user32'), c.WinDLL('kernel32')
Callback = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
def bind(lib, name, restype, args):
    fn = getattr(lib, name); fn.restype = restype; fn.argtypes = args; return fn
bind(u, 'EnumWindows', w.BOOL, [Callback, w.LPARAM])
bind(u, 'EnumChildWindows', w.BOOL, [w.HWND, Callback, w.LPARAM])
bind(u, 'GetWindowThreadProcessId', w.DWORD, [w.HWND, c.POINTER(w.DWORD)])
bind(u, 'GetClassNameW', c.c_int, [w.HWND, w.LPWSTR, c.c_int])
bind(u, 'SendMessageW', c.c_ssize_t, [w.HWND, w.UINT, w.WPARAM, w.LPARAM])
bind(u, 'PostMessageW', w.BOOL, [w.HWND, w.UINT, w.WPARAM, w.LPARAM])
bind(u, 'GetMenu', w.HMENU, [w.HWND])
bind(u, 'GetSubMenu', w.HMENU, [w.HMENU, c.c_int])
bind(u, 'GetMenuItemCount', c.c_int, [w.HMENU])
bind(u, 'GetMenuItemID', w.UINT, [w.HMENU, c.c_int])
bind(u, 'GetMenuStringW', c.c_int, [w.HMENU, w.UINT, w.LPWSTR, c.c_int, w.UINT])
bind(k, 'OpenProcess', w.HANDLE, [w.DWORD, w.BOOL, w.DWORD])
bind(k, 'CloseHandle', w.BOOL, [w.HANDLE])
bind(k, 'QueryFullProcessImageNameW', w.BOOL, [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)])
bind(k, 'VirtualAllocEx', w.LPVOID, [w.HANDLE, w.LPVOID, c.c_size_t, w.DWORD, w.DWORD])
bind(k, 'VirtualFreeEx', w.BOOL, [w.HANDLE, w.LPVOID, c.c_size_t, w.DWORD])
bind(k, 'WriteProcessMemory', w.BOOL, [w.HANDLE, w.LPVOID, w.LPCVOID, c.c_size_t, w.LPVOID])
bind(k, 'ReadProcessMemory', w.BOOL, [w.HANDLE, w.LPCVOID, w.LPVOID, c.c_size_t, w.LPVOID])

def windows(exe):
    found = []
    @Callback
    def visit(hwnd, unused):
        cls = c.create_unicode_buffer(100); u.GetClassNameW(hwnd, cls, 100)
        if cls.value != 'NoteX': return True
        pid = w.DWORD(); u.GetWindowThreadProcessId(hwnd, c.byref(pid))
        handle = k.OpenProcess(0x1000, False, pid.value)
        if handle:
            buf = c.create_unicode_buffer(32768); count = w.DWORD(len(buf))
            ok = k.QueryFullProcessImageNameW(handle, 0, buf, c.byref(count)); k.CloseHandle(handle)
            if ok and Path(buf.value).resolve() == Path(exe).resolve(): found.append((pid.value, hwnd))
        return True
    u.EnumWindows(visit, 0); return found

def wait_window(exe, timeout=30):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        found = windows(exe)
        if found:
            time.sleep(1); return found[0]
        time.sleep(.1)
    raise RuntimeError('The isolated NoteX window did not open.')

def editor(hwnd):
    found = []
    @Callback
    def visit(child, unused):
        cls = c.create_unicode_buffer(100); u.GetClassNameW(child, cls, 100)
        if cls.value == 'Scintilla': found.append(child)
        return True
    u.EnumChildWindows(hwnd, visit, 0)
    if not found: raise RuntimeError('The native editor was not found.')
    return found[0]

def text(pid, hwnd, value=None):
    view = editor(hwnd); handle = k.OpenProcess(0x38, False, pid)
    if not handle: raise c.WinError()
    remote = k.VirtualAllocEx(handle, None, 65536, 0x3000, 4)
    try:
        if not remote: raise c.WinError()
        if value is not None:
            data = value.encode('utf-8') + b'\0'; assert len(data) < 65536
            assert k.WriteProcessMemory(handle, remote, data, len(data), None)
            u.SendMessageW(view, 2181, 0, remote)
        size = u.SendMessageW(view, 2183, 0, 0)
        assert size < 65536
        u.SendMessageW(view, 2182, size + 1, remote)
        buf = c.create_string_buffer(size + 1)
        assert k.ReadProcessMemory(handle, remote, buf, len(buf), None)
        return buf.value.decode('utf-8')
    finally:
        if remote: k.VirtualFreeEx(handle, remote, 0, 0x8000)
        k.CloseHandle(handle)

def menus(hwnd):
    result = {}
    def walk(menu):
        for i in range(u.GetMenuItemCount(menu)):
            label = c.create_unicode_buffer(512); u.GetMenuStringW(menu, i, label, 512, 0x400)
            sub = u.GetSubMenu(menu, i)
            if sub: walk(sub)
            else: result[label.value.replace('&', '').split('\t')[0]] = u.GetMenuItemID(menu, i)
    walk(u.GetMenu(hwnd)); return result

def command(hwnd, title):
    items = menus(hwnd)
    if title not in items: raise AssertionError('Missing native command: ' + title)
    u.PostMessageW(hwnd, 0x111, items[title], 0)

def close(exe, timeout=30):
    for pid, hwnd in windows(exe): u.PostMessageW(hwnd, 0x10, 0, 0)
    deadline = time.monotonic() + timeout
    while windows(exe) and time.monotonic() < deadline: time.sleep(.1)
    if windows(exe): raise RuntimeError('Normal close was canceled; test did not force termination.')
