// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"context"
	"errors"
	"os"
	"path/filepath"
	"time"
)

// A clean installation obtains its original signed inventory from its exact release.
// Existing configuration and plugins are not part of that inventory.
func establishOwnership(root, current string) error {
	old, e := readReceipt(root)
	if e != nil {
		return e
	}
	if old != nil {
		if old.Manifest.Version != current {
			return errors.New("running version and installed inventory differ")
		}
		return nil
	}
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()
	base := downloadBase + "v" + current + "/NoteX-update-" + platformName() + "-" + architecture() + ".json"
	raw, e := fetch(ctx, client(), base, 2<<20)
	if e != nil {
		return e
	}
	sig, e := fetch(ctx, client(), base+".sig", 128)
	if e != nil {
		return e
	}
	s, e := verify(raw, sig, publicKey(), platformName(), architecture())
	if e != nil {
		return e
	}
	if s.Manifest.Version != current {
		return errors.New("bootstrap inventory version mismatch")
	}
	if e = validateInstalled(root, s); e != nil {
		return e
	}
	if e = atomicWrite(filepath.Join(receiptDirectory(root), receiptName+".sig"), sig, 0600); e != nil {
		return e
	}
	return atomicWrite(filepath.Join(receiptDirectory(root), receiptName), raw, 0600)
}
func durableStage(root, work string, s Signed) (string, error) {
	dir, e := os.MkdirTemp(stateDirectory(root), "transaction-")
	if e != nil {
		return "", e
	}
	for p, f := range s.Manifest.Files {
		b, e := os.ReadFile(filepath.Join(work, "payload", filepath.FromSlash(p)))
		if e != nil || digest(b) != f.SHA256 {
			return "", errors.New("staged payload changed")
		}
		mode := os.FileMode(0644)
		if f.Executable {
			mode = 0755
		}
		if e = atomicWrite(filepath.Join(dir, "payload", filepath.FromSlash(p)), b, mode); e != nil {
			return "", e
		}
	}
	for _, p := range []string{receiptName, receiptName + ".sig"} {
		b := s.Bytes
		if p != receiptName {
			b = s.Signature
		}
		if e = atomicWrite(filepath.Join(dir, p), b, 0600); e != nil {
			return "", e
		}
	}
	// Preserve an independent recovery executable if installation is interrupted.
	exe, e := os.Executable()
	if e != nil {
		return "", e
	}
	b, e := os.ReadFile(exe)
	if e != nil {
		return "", e
	}
	if e = atomicWrite(filepath.Join(dir, filepath.Base(exe)), b, 0700); e != nil {
		return "", e
	}
	return dir, nil
}
