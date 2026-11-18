// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"errors"
	"os"
	"os/exec"
	"path/filepath"
	"syscall"
	"unsafe"
)

var kernel = syscall.NewLazyDLL("kernel32.dll")

func replaceFile(src, dst string) error {
	a, e := syscall.UTF16PtrFromString(src)
	if e != nil {
		return e
	}
	b, e := syscall.UTF16PtrFromString(dst)
	if e != nil {
		return e
	}
	r, _, e := kernel.NewProc("MoveFileExW").Call(uintptr(unsafe.Pointer(a)), uintptr(unsafe.Pointer(b)), 0x1|0x8)
	if r == 0 {
		return e
	}
	return nil
}
func message(s string, flags uintptr) uintptr {
	t, _ := syscall.UTF16PtrFromString("NoteQu Updates")
	b, _ := syscall.UTF16PtrFromString(s)
	r, _, _ := syscall.NewLazyDLL("user32.dll").NewProc("MessageBoxW").Call(0, uintptr(unsafe.Pointer(b)), uintptr(unsafe.Pointer(t)), flags|0x10000)
	return r
}
func askPlatform(s string) bool { return message(s, 4|0x20|0x100) == 6 }
func tellPlatform(s string)     { message(s, 0x40) }
func detach(c *exec.Cmd) {
	c.SysProcAttr = &syscall.SysProcAttr{HideWindow: true, CreationFlags: 0x08000000}
}
func waitForExit(pid int) error {
	h, e := syscall.OpenProcess(0x100000, false, uint32(pid))
	if e != nil {
		if e == syscall.Errno(87) {
			return nil
		}
		return e
	}
	defer syscall.CloseHandle(h)
	s, e := syscall.WaitForSingleObject(h, syscall.INFINITE)
	if e != nil {
		return e
	}
	if s != 0 {
		return errors.New("could not wait for normal application exit")
	}
	return nil
}
func processAlive(pid int) bool {
	h, e := syscall.OpenProcess(0x100000, false, uint32(pid))
	if e != nil {
		return false
	}
	defer syscall.CloseHandle(h)
	s, _ := syscall.WaitForSingleObject(h, 0)
	return s == 258
}
func applicationRunning(root string) bool {
	// Windows refuses write/delete sharing while the executable is mapped. Test without mutation.
	p, _ := syscall.UTF16PtrFromString(filepath.Join(root, "NoteQu.exe"))
	h, e := syscall.CreateFile(p, syscall.GENERIC_READ|syscall.GENERIC_WRITE, 0, nil, syscall.OPEN_EXISTING, 0, 0)
	if e != nil {
		return true
	}
	syscall.CloseHandle(h)
	return false
}
func verifyPlatform(root string) error { return nil } // Ed25519 authenticates every executable byte before launch.
func launchApplication(root, profile string) (*os.Process, error) {
	c := exec.Command(filepath.Join(root, "NoteQu.exe"), "-multiInst", "-settingsDir="+profile)
	c.Dir = root
	detach(c)
	if e := c.Start(); e != nil {
		return nil, e
	}
	return c.Process, nil
}
