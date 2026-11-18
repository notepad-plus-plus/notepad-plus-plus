// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"time"
)

func replaceFile(src, dst string) error { return os.Rename(src, dst) }
func askPlatform(s string) bool {
	b, e := exec.Command("/usr/bin/osascript", "-e", "display dialog "+strconv.Quote(s)+" with title \"NoteQu Updates\" buttons {\"Cancel\", \"Continue\"} default button \"Cancel\"").Output()
	return e == nil && strings.Contains(string(b), "Continue")
}
func tellPlatform(s string) {
	_ = exec.Command("/usr/bin/osascript", "-e", "display dialog "+strconv.Quote(s)+" with title \"NoteQu Updates\" buttons {\"OK\"} default button \"OK\"").Run()
}
func detach(c *exec.Cmd)        { c.SysProcAttr = &syscall.SysProcAttr{Setsid: true} }
func processAlive(pid int) bool { return syscall.Kill(pid, 0) == nil }
func waitForExit(pid int) error {
	for processAlive(pid) {
		time.Sleep(time.Second)
	}
	return nil
}
func applicationRunning(root string) bool {
	b, e := exec.Command("/usr/sbin/lsof", "-t", filepath.Join(root, "Contents/MacOS/NoteQu")).Output()
	return e == nil && len(strings.TrimSpace(string(b))) > 0
}
func verifyPlatform(root string) error {
	return exec.Command("/usr/bin/codesign", "--verify", "--strict", root).Run()
}
func launchApplication(root, profile string) (*os.Process, error) {
	c := exec.Command(filepath.Join(root, "Contents/MacOS/NoteQu"))
	c.Env = append(os.Environ(), "NOTEQU_PROFILE="+profile, "NOTEX_PROFILE="+profile)
	detach(c)
	if e := c.Start(); e != nil {
		return nil, e
	}
	return c.Process, nil
}
