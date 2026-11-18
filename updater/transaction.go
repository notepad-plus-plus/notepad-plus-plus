// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"sort"
)

type Change struct {
	Path    string
	Before  string
	After   string
	Existed bool
	Mode    uint32
}
type Journal struct {
	Version   string
	Changes   []Change
	Committed bool
}

func safeTarget(root, p string) (string, error) {
	if !validPath(p) {
		return "", errors.New("invalid target path")
	}
	cur := root
	// Reject links, junctions and non-directory ancestors, including the install root.
	if st, e := os.Lstat(cur); e != nil || !st.IsDir() || st.Mode()&os.ModeSymlink != 0 {
		return "", errors.New("unsafe installation root")
	}
	for _, part := range splitPath(p) {
		cur = filepath.Join(cur, part)
		st, e := os.Lstat(cur)
		if os.IsNotExist(e) {
			continue
		}
		if e != nil {
			return "", e
		}
		if st.Mode()&os.ModeSymlink != 0 || !st.Mode().IsRegular() && !st.IsDir() {
			return "", errors.New("linked or special update target")
		}
	}
	return cur, nil
}
func splitPath(p string) []string {
	var out []string
	for p != "." && p != "" {
		d, b := filepath.Split(filepath.FromSlash(p))
		out = append([]string{b}, out...)
		p = filepath.Clean(d)
	}
	return out
}
func stateDirectory(root string) string {
	if platformName() == "darwin" {
		return filepath.Join(filepath.Dir(root), "."+filepath.Base(root)+"-update")
	}
	return filepath.Join(root, ".notex-update")
}
func receiptDirectory(root string) string { return stateDirectory(root) }
func transactionTarget(root, p string) (string, error) {
	if p == receiptName || p == receiptName+".sig" {
		return safeTarget(receiptDirectory(root), p)
	}
	return safeTarget(root, p)
}
func readReceipt(root string) (*Signed, error) {
	raw, e := os.ReadFile(filepath.Join(receiptDirectory(root), receiptName))
	if os.IsNotExist(e) {
		return nil, nil
	}
	if e != nil {
		return nil, e
	}
	sig, e := os.ReadFile(filepath.Join(receiptDirectory(root), receiptName+".sig"))
	if e != nil {
		return nil, e
	}
	s, e := verify(raw, sig, publicKey(), platformName(), architecture())
	if e != nil {
		return nil, e
	}
	return &s, nil
}
func saveJournal(dir string, j Journal) error {
	b, e := json.Marshal(j)
	if e != nil {
		return e
	}
	return atomicWrite(filepath.Join(dir, "journal.json"), b, 0600)
}

// All backup data is application-owned. No profile, session, document or plugin is copied.
func apply(root, staged, work string, next Signed) error {
	if e := validateExecutables(staged, next); e != nil {
		return e
	}
	old, e := readReceipt(root)
	if e != nil {
		return e
	}
	if old != nil && !newer(next.Manifest.Version, old.Manifest.Version) {
		return errors.New("update would not advance installed version")
	}
	paths := map[string]bool{}
	for p := range next.Manifest.Files {
		paths[p] = true
	}
	if old != nil {
		for p := range old.Manifest.Files {
			paths[p] = true
		}
	}
	var ordered []string
	for p := range paths {
		ordered = append(ordered, p)
	}
	sort.Strings(ordered)
	j := Journal{Version: next.Manifest.Version}
	for _, p := range ordered {
		target, e := safeTarget(root, p)
		if e != nil {
			return e
		}
		before, e := os.ReadFile(target)
		exists := e == nil
		if e != nil && !os.IsNotExist(e) {
			return e
		}
		spec, keep := next.Manifest.Files[p]
		if exists && keep && old != nil {
			previous, registered := old.Manifest.Files[p]
			if !registered {
				return fmt.Errorf("new application file conflicts with an unowned local file: %s", p)
			}
			if digest(before) != previous.SHA256 && digest(before) != spec.SHA256 {
				return fmt.Errorf("locally modified application file preserved: %s", p)
			}
		}
		if !keep {
			previous := old.Manifest.Files[p]
			if !exists || digest(before) != previous.SHA256 {
				continue
			}
		} // Retain locally modified obsolete files.
		mode := os.FileMode(0644)
		if exists {
			st, e := os.Stat(target)
			if e != nil {
				return e
			}
			mode = st.Mode().Perm()
		}
		after := ""
		if keep {
			data, e := os.ReadFile(filepath.Join(staged, filepath.FromSlash(p)))
			if e != nil || digest(data) != spec.SHA256 {
				return errors.New("staged payload changed")
			}
			after = spec.SHA256
		}
		if exists && keep && digest(before) == after {
			continue
		}
		// Unknown existing non-binary files may be customized: preserve them instead of claiming ownership.
		if exists && old == nil && p != "NoteX.exe" && p != "NoteXUpdater.exe" && platformName() == "windows" {
			return fmt.Errorf("bootstrap needs a reviewed ownership inventory before replacing %s", p)
		}
		c := Change{Path: p, After: after, Existed: exists, Mode: uint32(mode)}
		if exists {
			c.Before = digest(before)
			if e = atomicWrite(filepath.Join(work, "before", filepath.FromSlash(p)), before, 0600); e != nil {
				return e
			}
		}
		j.Changes = append(j.Changes, c)
	}
	// Receipt files are journaled as application metadata, not as user data.
	for _, p := range []string{receiptName, receiptName + ".sig"} {
		b, e := os.ReadFile(filepath.Join(receiptDirectory(root), p))
		exists := e == nil
		if e != nil && !os.IsNotExist(e) {
			return e
		}
		after := next.Bytes
		if p != receiptName {
			after = next.Signature
		}
		c := Change{Path: p, After: digest(after), Existed: exists, Mode: 0644}
		if exists {
			c.Before = digest(b)
			if e = atomicWrite(filepath.Join(work, "before", p), b, 0600); e != nil {
				return e
			}
		}
		j.Changes = append(j.Changes, c)
	}
	if e = saveJournal(work, j); e != nil {
		return e
	}
	for _, c := range j.Changes {
		target, e := transactionTarget(root, c.Path)
		if e != nil {
			return e
		}
		if c.After == "" {
			e = os.Remove(target)
		} else {
			var data []byte
			mode := os.FileMode(0644)
			switch c.Path {
			case receiptName:
				data = next.Bytes
			case receiptName + ".sig":
				data = next.Signature
			default:
				data, e = os.ReadFile(filepath.Join(staged, filepath.FromSlash(c.Path)))
				if next.Manifest.Files[c.Path].Executable {
					mode = 0755
				}
			}
			if e == nil && digest(data) != c.After {
				return errors.New("staged file changed before replacement")
			}
			if e == nil {
				e = atomicWrite(target, data, mode)
			}
		}
		if e != nil {
			return e
		}
	}
	if e = validateInstalled(root, next); e != nil {
		return e
	}
	return nil
}
func validateInstalled(root string, s Signed) error {
	for p, f := range s.Manifest.Files {
		target, e := safeTarget(root, p)
		if e != nil {
			return e
		}
		b, e := os.ReadFile(target)
		if e != nil || digest(b) != f.SHA256 {
			return errors.New("installed payload verification failed")
		}
	}
	return verifyPlatform(root)
}
func recoverTransaction(root, work string) error {
	b, e := os.ReadFile(filepath.Join(work, "journal.json"))
	if os.IsNotExist(e) {
		return nil
	}
	if e != nil {
		return e
	}
	var j Journal
	if e = json.Unmarshal(b, &j); e != nil {
		return e
	}
	if j.Committed {
		return nil
	}
	for i := len(j.Changes) - 1; i >= 0; i-- {
		c := j.Changes[i]
		if !ownedPath(c.Path, platformName()) && c.Path != receiptName && c.Path != receiptName+".sig" {
			return errors.New("unsafe recovery journal")
		}
		target, e := transactionTarget(root, c.Path)
		if e != nil {
			return e
		}
		current, e := os.ReadFile(target)
		if e != nil && !os.IsNotExist(e) {
			return e
		}
		if e == nil && digest(current) != c.Before && digest(current) != c.After {
			return errors.New("application file changed since update; recovery retained both versions")
		}
		if c.Existed {
			before, e := os.ReadFile(filepath.Join(work, "before", filepath.FromSlash(c.Path)))
			if e != nil || digest(before) != c.Before {
				return errors.New("invalid recovery backup")
			}
			if e = atomicWrite(target, before, os.FileMode(c.Mode)); e != nil {
				return e
			}
		} else if e == nil {
			if e = os.Remove(target); e != nil {
				return e
			}
		}
	}
	j.Committed = true
	return saveJournal(work, j)
}
func commitTransaction(work string) error {
	b, e := os.ReadFile(filepath.Join(work, "journal.json"))
	if e != nil {
		return e
	}
	var j Journal
	if e = json.Unmarshal(b, &j); e != nil {
		return e
	}
	j.Committed = true
	return saveJournal(work, j)
}
