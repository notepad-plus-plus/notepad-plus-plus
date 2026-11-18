// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"archive/zip"
	"bytes"
	"context"
	"crypto/ed25519"
	"crypto/rand"
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"
)

func fixture(t *testing.T) (Signed, ed25519.PublicKey, ed25519.PrivateKey) {
	t.Helper()
	pub, key, e := ed25519.GenerateKey(rand.Reader)
	if e != nil {
		t.Fatal(e)
	}
	files := map[string]File{"NoteX.exe": {SHA256: digest([]byte("app")), Size: 3}, "NoteXUpdater.exe": {SHA256: digest([]byte("helper")), Size: 6}}
	m := Manifest{Schema: 1, Repository: repository, Version: "1.6.1", Platform: "windows", Architecture: "amd64", Source: strings.Repeat("a", 40), URL: downloadBase + "v1.6.1/NoteX-1.6.1-windows-amd64-update.zip", Files: files}
	b, _ := json.Marshal(m)
	return Signed{Manifest: m, Bytes: b, Signature: ed25519.Sign(key, b)}, pub, key
}
func TestVersions(t *testing.T) {
	for _, c := range []struct {
		a, b string
		want bool
	}{{"1.6.1", "1.6.0", true}, {"1.10.0", "1.9.9", true}, {"1.6.0", "1.6.0", false}, {"1.5.9", "1.6.0", false}, {"1.6.1-beta", "1.6.0", false}, {"01.6.1", "1.6.0", false}, {"999999999999999999999.1.0", "1.0.0", false}} {
		if newer(c.a, c.b) != c.want {
			t.Errorf("%+v", c)
		}
	}
}
func TestSignedManifest(t *testing.T) {
	s, pub, key := fixture(t)
	if _, e := verify(s.Bytes, s.Signature, pub, "windows", "amd64"); e != nil {
		t.Fatal(e)
	}
	for _, c := range []struct {
		name, platform, arch string
		raw, sig, pub        []byte
	}{{"wrong OS", "darwin", "amd64", s.Bytes, s.Signature, pub}, {"wrong CPU", "windows", "arm64", s.Bytes, s.Signature, pub}, {"bad signature", "windows", "amd64", s.Bytes, make([]byte, 64), pub}, {"tampered", "windows", "amd64", append(s.Bytes, ' '), s.Signature, pub}, {"wrong key", "windows", "amd64", s.Bytes, s.Signature, make([]byte, 32)}} {
		t.Run(c.name, func(t *testing.T) {
			if _, e := verify(c.raw, c.sig, c.pub, c.platform, c.arch); e == nil {
				t.Fatal("accepted invalid manifest")
			}
		})
	}
	for _, p := range []string{"../note.txt", "config.xml", "plugins/Fake.dll", "themes/mine.xml", "session.xml", "NoteX.exe:stream", "CON", "/NoteX.exe"} {
		m := s.Manifest
		m.Files = map[string]File{}
		for name, f := range s.Manifest.Files {
			m.Files[name] = f
		}
		m.Files[p] = File{SHA256: strings.Repeat("a", 64)}
		b, _ := json.Marshal(m)
		if _, e := verify(b, ed25519.Sign(key, b), pub, "windows", "amd64"); e == nil {
			t.Errorf("accepted forbidden ownership %s", p)
		}
	}
}
func release(tag, platform, arch string) Release {
	r := Release{Tag: tag}
	name := "NoteX-update-" + platform + "-" + arch + ".json"
	for _, n := range []string{name, name + ".sig"} {
		r.Assets = append(r.Assets, struct {
			Name string `json:"name"`
			URL  string `json:"browser_download_url"`
		}{n, downloadBase + tag + "/" + n})
	}
	return r
}
func TestPlatformSelection(t *testing.T) {
	rs := []Release{release("v9.0.0", "darwin", "arm64"), release("v1.6.1", "windows", "amd64"), release("v1.7.0", "windows", "arm64"), release("v1.6.0", "windows", "amd64"), release("v1.8.0", "windows", "amd64"), release("v1.9.0", "windows", "amd64")}
	rs[4].Draft = true
	rs[5].Prerelease = true
	r := candidates(rs, "1.6.0", "windows", "amd64")
	if len(r) != 1 || r[0].Tag != "v1.6.1" {
		t.Fatalf("wrong release: %+v", r)
	}
	rs[1].Assets[0].URL = "https://example.com/update"
	if len(candidates(rs, "1.6.0", "windows", "amd64")) != 0 {
		t.Fatal("foreign asset accepted")
	}
}
func archive(t *testing.T, names []string, contents []string) []byte {
	t.Helper()
	var b bytes.Buffer
	z := zip.NewWriter(&b)
	for i, n := range names {
		w, e := z.Create(n)
		if e != nil {
			t.Fatal(e)
		}
		w.Write([]byte(contents[i]))
	}
	if e := z.Close(); e != nil {
		t.Fatal(e)
	}
	return b.Bytes()
}
func TestArchiveIntegrity(t *testing.T) {
	s, _, _ := fixture(t)
	valid := archive(t, []string{"NoteX.exe", "NoteXUpdater.exe"}, []string{"app", "helper"})
	if e := extract(valid, s, t.TempDir()); e != nil {
		t.Fatal(e)
	}
	for name, b := range map[string][]byte{"truncated": valid[:len(valid)/2], "tampered": archive(t, []string{"NoteX.exe", "NoteXUpdater.exe"}, []string{"bad", "helper"}), "missing": archive(t, []string{"NoteX.exe"}, []string{"app"}), "traversal": archive(t, []string{"../NoteX.exe"}, []string{"app"}), "duplicate": archive(t, []string{"NoteX.exe", "NoteX.exe"}, []string{"app", "app"}), "unowned": archive(t, []string{"config.xml"}, []string{"x"})} {
		t.Run(name, func(t *testing.T) {
			if e := extract(b, s, t.TempDir()); e == nil {
				t.Fatal("invalid archive accepted")
			}
		})
	}
}
func TestNetworkFailures(t *testing.T) {
	for _, kind := range []string{"timeout", "partial", "oversize", "status"} {
		t.Run(kind, func(t *testing.T) {
			server := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != "GET" || r.Header.Get("Authorization") != "" || r.URL.RawQuery != "" {
					t.Error("unexpected request data")
				}
				switch kind {
				case "timeout":
					time.Sleep(150 * time.Millisecond)
				case "partial":
					w.Header().Set("Content-Length", "100")
					fmt.Fprint(w, "short")
				case "oversize":
					fmt.Fprint(w, strings.Repeat("a", 100))
				case "status":
					w.WriteHeader(503)
				}
			}))
			defer server.Close()
			ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
			defer cancel()
			if _, e := fetch(ctx, server.Client(), server.URL, 50); e == nil {
				t.Fatal("network failure accepted")
			}
		})
	}
}
func TestRecoveryPreservesData(t *testing.T) {
	root, work := t.TempDir(), t.TempDir()
	must := func(e error) {
		t.Helper()
		if e != nil {
			t.Fatal(e)
		}
	}
	must(os.MkdirAll(stateDirectory(root), 0700))
	appPath := "NoteX.exe"
	oldPath := "UPDATE-BOOTSTRAP.md"
	if platformName() == "darwin" {
		appPath = "Contents/MacOS/NoteX"
		oldPath = "Contents/Resources/UPDATE-BOOTSTRAP.md"
	}
	protected := []string{"session.xml", "backup/new-note", "config.xml", "shortcuts.xml", "themes/custom.xml", "userDefineLangs/custom.xml", "plugins/Test/Test.dll", "plugins/config/Test.ini", "my-notes.txt"}
	for _, p := range protected {
		must(atomicWrite(filepath.Join(root, p), []byte("created after update: "+p), 0600))
	}
	must(atomicWrite(filepath.Join(work, "before", appPath), []byte("old app"), 0600))
	must(atomicWrite(filepath.Join(root, appPath), []byte("new app"), 0600))
	must(atomicWrite(filepath.Join(work, "before", oldPath), []byte("old doc"), 0600))
	j := Journal{Version: "1.6.1", Changes: []Change{{Path: appPath, Before: digest([]byte("old app")), After: digest([]byte("new app")), Existed: true, Mode: 0644}, {Path: oldPath, Before: digest([]byte("old doc")), Existed: true, Mode: 0644}}}
	must(saveJournal(work, j))
	must(recoverTransaction(root, work))
	must(recoverTransaction(root, work))
	for _, p := range protected {
		b, e := os.ReadFile(filepath.Join(root, p))
		must(e)
		if string(b) != "created after update: "+p {
			t.Fatal("personal data changed")
		}
	}
	b, e := os.ReadFile(filepath.Join(root, appPath))
	must(e)
	if string(b) != "old app" {
		t.Fatal("application not restored")
	}
	b, e = os.ReadFile(filepath.Join(root, oldPath))
	must(e)
	if string(b) != "old doc" {
		t.Fatal("deleted application file not restored")
	}
}
func TestRecoveryDoesNotOverwriteNewChanges(t *testing.T) {
	root, work := t.TempDir(), t.TempDir()
	p := "NoteX.exe"
	if platformName() == "darwin" {
		p = "Contents/MacOS/NoteX"
	}
	atomicWrite(filepath.Join(root, p), []byte("user changed later"), 0600)
	atomicWrite(filepath.Join(work, "before", p), []byte("old"), 0600)
	saveJournal(work, Journal{Changes: []Change{{Path: p, Before: digest([]byte("old")), After: digest([]byte("new")), Existed: true}}})
	if e := recoverTransaction(root, work); e == nil {
		t.Fatal("overwrote later change")
	}
	b, _ := os.ReadFile(filepath.Join(root, p))
	if string(b) != "user changed later" {
		t.Fatal("file changed")
	}
}
func TestLinkedTargetRejected(t *testing.T) {
	root, outside := t.TempDir(), t.TempDir()
	if e := os.Symlink(outside, filepath.Join(root, "linked")); e != nil {
		t.Skip("symlink creation not permitted")
	}
	if _, e := safeTarget(root, "linked/note.txt"); e == nil {
		t.Fatal("symlink target accepted")
	}
}
