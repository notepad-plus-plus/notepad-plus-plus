// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"encoding/binary"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"testing"
	"time"
)

func executableFixture(system, arch string) []byte {
	b := make([]byte, 128)
	if system == "windows" {
		copy(b, "MZ")
		binary.LittleEndian.PutUint32(b[60:], 64)
		copy(b[64:], "PE\x00\x00")
		machine := uint16(0x8664)
		if arch == "arm64" {
			machine = 0xaa64
		}
		binary.LittleEndian.PutUint16(b[68:], machine)
	} else {
		binary.LittleEndian.PutUint32(b, 0xfeedfacf)
		cpu := uint32(0x0100000c)
		if arch == "amd64" {
			cpu = 0x01000007
		}
		binary.LittleEndian.PutUint32(b[4:], cpu)
	}
	return b
}
func TestExecutableArchitecture(t *testing.T) {
	for _, system := range []string{"windows", "darwin"} {
		for _, arch := range []string{"amd64", "arm64"} {
			b := executableFixture(system, arch)
			if !binaryMatches(b, system, arch) {
				t.Fatal("valid header rejected")
			}
			other := "arm64"
			if arch == other {
				other = "amd64"
			}
			if binaryMatches(b, system, other) {
				t.Fatal("wrong CPU accepted")
			}
			otherOS := "windows"
			if system == otherOS {
				otherOS = "darwin"
			}
			if binaryMatches(b, otherOS, arch) || binaryMatches(b[:12], system, arch) {
				t.Fatal("wrong OS or truncated header accepted")
			}
		}
	}
}
func TestInterruptedUpdateProcess(t *testing.T) {
	if root := os.Getenv("NOTEQU_TRANSACTION_CHILD"); root != "" {
		work := filepath.Join(root, "transaction")
		staged := filepath.Join(root, "stage")
		app, helper := "NoteQu.exe", "NoteQuUpdater.exe"
		if runtime.GOOS == "darwin" {
			app = "Contents/MacOS/NoteQu"
			helper = "Contents/MacOS/NoteQuUpdater"
		}
		b := executableFixture(runtime.GOOS, runtime.GOARCH)
		s := Signed{Manifest: Manifest{Version: "1.6.1", Platform: runtime.GOOS, Architecture: runtime.GOARCH, Files: map[string]File{app: {SHA256: digest(b), Size: int64(len(b)), Executable: true}, helper: {SHA256: digest(b), Size: int64(len(b)), Executable: true}}}}
		for _, p := range []string{app, helper} {
			if e := atomicWrite(filepath.Join(staged, p), b, 0755); e != nil {
				t.Fatal(e)
			}
		}
		transactionCheckpoint = func() {
			if e := atomicWrite(filepath.Join(root, "checkpoint"), []byte("one file replaced; journal durable"), 0600); e != nil {
				panic(e)
			}
			for {
				time.Sleep(time.Second)
			}
		}
		if e := apply(root, staged, work, s); e != nil {
			t.Fatal(e)
		}
		t.Fatal("checkpoint was not reached")
		return
	}
	root := t.TempDir()
	if e := os.MkdirAll(stateDirectory(root), 0700); e != nil {
		t.Fatal(e)
	}
	app, helper := "NoteQu.exe", "NoteQuUpdater.exe"
	if runtime.GOOS == "darwin" {
		app = "Contents/MacOS/NoteQu"
		helper = "Contents/MacOS/NoteQuUpdater"
	}
	for _, p := range []string{app, helper} {
		if e := atomicWrite(filepath.Join(root, p), []byte("old application: "+p), 0755); e != nil {
			t.Fatal(e)
		}
	}
	cmd := exec.Command(os.Args[0], "-test.run=^TestInterruptedUpdateProcess$")
	cmd.Env = append(os.Environ(), "NOTEQU_TRANSACTION_CHILD="+root)
	if e := cmd.Start(); e != nil {
		t.Fatal(e)
	}
	defer func() {
		if cmd.ProcessState == nil {
			cmd.Process.Kill()
			cmd.Wait()
		}
	}()
	deadline := time.Now().Add(15 * time.Second)
	for {
		if _, e := os.Stat(filepath.Join(root, "checkpoint")); e == nil {
			break
		}
		if time.Now().After(deadline) {
			t.Fatal("transaction checkpoint not reached")
		}
		time.Sleep(10 * time.Millisecond)
	}
	// Terminate only this test-owned updater worker, after a real atomic application-file replacement.
	if e := cmd.Process.Kill(); e != nil {
		t.Fatal(e)
	}
	cmd.Wait()
	note := filepath.Join(root, "new-note-after-interruption.txt")
	if e := atomicWrite(note, []byte("keep new work"), 0600); e != nil {
		t.Fatal(e)
	}
	if e := recoverTransaction(root, filepath.Join(root, "transaction")); e != nil {
		t.Fatal(e)
	}
	for _, p := range []string{app, helper} {
		b, e := os.ReadFile(filepath.Join(root, p))
		if e != nil || string(b) != "old application: "+p {
			t.Fatal("interrupted update did not restore original application")
		}
	}
	b, e := os.ReadFile(note)
	if e != nil || string(b) != "keep new work" {
		t.Fatal("recovery overwrote new user data")
	}
}
