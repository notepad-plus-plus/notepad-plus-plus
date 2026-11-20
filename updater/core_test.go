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
	"io"
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
	files := map[string]File{"NoteQu.exe": {SHA256: digest([]byte("app")), Size: 3}, "NoteQuUpdater.exe": {SHA256: digest([]byte("helper")), Size: 6}}
	m := Manifest{Schema: 1, Repository: repository, Version: "1.6.1", Platform: "windows", Architecture: "amd64", Source: strings.Repeat("a", 40), URL: downloadBase + "v1.6.1/NoteQu-1.6.1-windows-amd64-update.zip", Files: files}
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
	for _, p := range []string{"../note.txt", "config.xml", "plugins/Fake.dll", "themes/mine.xml", "session.xml", "NoteQu.exe:stream", "CON", "/NoteQu.exe"} {
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
	name := "NoteQu-update-" + platform + "-" + arch + ".json"
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

type testHTTPTransport func(*http.Request) (*http.Response, error)

func (f testHTTPTransport) RoundTrip(r *http.Request) (*http.Response, error) { return f(r) }

func TestReleaseDiscoveryWithRESTLimits(t *testing.T) {
	s, pub, _ := fixture(t)
	name := "NoteQu-update-windows-amd64.json"
	pinned := downloadBase + "v1.6.1/" + name
	api := "https://api.github.com/repos/" + repository + "/releases?per_page=100&page=1"
	alias := releaseBase + "latest/download/" + name
	for _, c := range []struct {
		name, current, location        string
		apiStatus, aliasStatus         int
		corrupt, wantSigned, wantError bool
	}{
		{name: "normal discovery", current: "1.6.0", apiStatus: 200, wantSigned: true},
		{name: "forbidden API", current: "1.6.0", apiStatus: 403, aliasStatus: 302, location: pinned, wantSigned: true},
		{name: "too many API requests", current: "1.6.0", apiStatus: 429, aliasStatus: 302, location: pinned, wantSigned: true},
		{name: "already current", current: "1.6.1", apiStatus: 403, aliasStatus: 302, location: pinned},
		{name: "no downgrade", current: "1.7.0", apiStatus: 403, aliasStatus: 302, location: pinned},
		{name: "other platform published", current: "1.6.0", apiStatus: 403, aliasStatus: 404, wantError: true},
		{name: "untrusted redirect", current: "1.6.0", apiStatus: 403, aliasStatus: 302, location: "https://example.com/" + name, wantError: true},
		{name: "different asset", current: "1.6.0", apiStatus: 403, aliasStatus: 302, location: pinned + ".sig", wantError: true},
		{name: "prerelease tag", current: "1.6.0", apiStatus: 403, aliasStatus: 302, location: downloadBase + "v1.7.0-rc/" + name, wantError: true},
		{name: "signature damaged", current: "1.6.0", apiStatus: 403, aliasStatus: 302, location: pinned, corrupt: true, wantError: true},
		{name: "signed version differs from tag", current: "1.6.0", apiStatus: 403, aliasStatus: 302, location: downloadBase + "v1.7.0/" + name, wantError: true},
		{name: "ordinary server failure", current: "1.6.0", apiStatus: 503, wantError: true},
	} {
		t.Run(c.name, func(t *testing.T) {
			var requested []string
			client := &http.Client{Transport: testHTTPTransport(func(req *http.Request) (*http.Response, error) {
				u := req.URL.String()
				requested = append(requested, u)
				if req.Method != http.MethodGet || req.Header.Get("Authorization") != "" {
					t.Errorf("unexpected metadata request: %s", req.Method)
				}
				res := &http.Response{StatusCode: 200, Header: make(http.Header)}
				var body []byte
				switch u {
				case api:
					res.StatusCode = c.apiStatus
					body, _ = json.Marshal([]Release{release("v1.6.1", "windows", "amd64"), release("v1.8.0", "darwin", "arm64")})
				case alias:
					if c.aliasStatus == 0 {
						t.Fatal("public release link requested without API throttling")
					}
					res.StatusCode = c.aliasStatus
					res.Header.Set("Location", c.location)
				case pinned, downloadBase + "v1.7.0/" + name:
					body = s.Bytes
				case pinned + ".sig", downloadBase + "v1.7.0/" + name + ".sig":
					body = s.Signature
					if c.corrupt {
						body = make([]byte, ed25519.SignatureSize)
					}
				default:
					t.Fatalf("unexpected URL: %s", u)
				}
				res.Body = io.NopCloser(bytes.NewReader(body))
				return res, nil
			})}
			found, err := discoverFor(context.Background(), client, c.current, "windows", "amd64", pub)
			if (err != nil) != c.wantError || (found != nil) != c.wantSigned {
				t.Fatalf("discovery = %v, %v; wanted signed=%v error=%v", found, err, c.wantSigned, c.wantError)
			}
			if found != nil && (found.Manifest.Version != "1.6.1" || requested[len(requested)-1] != pinned+".sig") {
				t.Fatalf("unexpected release or unpinned signature: %v", requested)
			}
		})
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
	valid := archive(t, []string{"NoteQu.exe", "NoteQuUpdater.exe"}, []string{"app", "helper"})
	if e := extract(valid, s, t.TempDir()); e != nil {
		t.Fatal(e)
	}
	for name, b := range map[string][]byte{"truncated": valid[:len(valid)/2], "tampered": archive(t, []string{"NoteQu.exe", "NoteQuUpdater.exe"}, []string{"bad", "helper"}), "missing": archive(t, []string{"NoteQu.exe"}, []string{"app"}), "traversal": archive(t, []string{"../NoteQu.exe"}, []string{"app"}), "duplicate": archive(t, []string{"NoteQu.exe", "NoteQu.exe"}, []string{"app", "app"}), "unowned": archive(t, []string{"config.xml"}, []string{"x"})} {
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
	appPath := "NoteQu.exe"
	oldPath := "UPDATE-BOOTSTRAP.md"
	if platformName() == "darwin" {
		appPath = "Contents/MacOS/NoteQu"
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
	p := "NoteQu.exe"
	if platformName() == "darwin" {
		p = "Contents/MacOS/NoteQu"
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
