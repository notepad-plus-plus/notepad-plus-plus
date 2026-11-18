// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"encoding/binary"
	"errors"
	"os"
	"path/filepath"
)

func binaryMatches(data []byte, platform, arch string) bool {
	if platform == "windows" {
		if len(data) < 64 || string(data[:2]) != "MZ" {
			return false
		}
		off := uint64(binary.LittleEndian.Uint32(data[60:64]))
		if off+6 > uint64(len(data)) || string(data[off:off+4]) != "PE\x00\x00" {
			return false
		}
		machine := binary.LittleEndian.Uint16(data[off+4 : off+6])
		return arch == "amd64" && machine == 0x8664 || arch == "arm64" && machine == 0xaa64
	}
	if platform == "darwin" {
		if len(data) < 32 || binary.LittleEndian.Uint32(data[:4]) != 0xfeedfacf {
			return false
		}
		cpu := binary.LittleEndian.Uint32(data[4:8])
		return arch == "arm64" && cpu == 0x0100000c || arch == "amd64" && cpu == 0x01000007
	}
	return false
}
func validateExecutables(root string, s Signed) error {
	names := []string{"NoteX.exe", "NoteXUpdater.exe"}
	if s.Manifest.Platform == "darwin" {
		names = []string{"Contents/MacOS/NoteX", "Contents/MacOS/NoteXUpdater"}
	}
	for _, p := range names {
		b, e := os.ReadFile(filepath.Join(root, filepath.FromSlash(p)))
		if e != nil {
			return e
		}
		if !binaryMatches(b, s.Manifest.Platform, s.Manifest.Architecture) {
			return errors.New("executable platform or architecture does not match signed manifest")
		}
	}
	return nil
}
