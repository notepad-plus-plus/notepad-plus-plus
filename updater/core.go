// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Created for NoteQu on 2026-11-18.
package main

import (
	"archive/zip"
	"bytes"
	"context"
	"crypto/ed25519"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/url"
	"os"
	"path"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"time"
)

const repository = "berkkivircik/NoteQu"
const downloadBase = "https://github.com/" + repository + "/releases/download/"
const maxPackage = 256 << 20
const receiptName = "notequ-owned.json"

var versionPattern = regexp.MustCompile(`^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$`)

type File struct {
	SHA256     string `json:"sha256"`
	Size       int64  `json:"size"`
	Executable bool   `json:"executable,omitempty"`
}
type Manifest struct {
	Schema       int             `json:"schema"`
	Repository   string          `json:"repository"`
	Version      string          `json:"version"`
	Platform     string          `json:"platform"`
	Architecture string          `json:"architecture"`
	Source       string          `json:"source"`
	URL          string          `json:"url"`
	Files        map[string]File `json:"files"`
}
type Signed struct {
	Manifest         Manifest
	Bytes, Signature []byte
}

func newer(a, b string) bool {
	if !versionPattern.MatchString(a) || !versionPattern.MatchString(b) {
		return false
	}
	aa, bb := strings.Split(a, "."), strings.Split(b, ".")
	for i := range aa {
		x, e := strconv.ParseUint(aa[i], 10, 32)
		if e != nil {
			return false
		}
		y, e := strconv.ParseUint(bb[i], 10, 32)
		if e != nil {
			return false
		}
		if x != y {
			return x > y
		}
	}
	return false
}
func validPath(p string) bool {
	if p == "" || path.Clean(p) != p || strings.ContainsAny(p, "\\:\x00") || strings.HasPrefix(p, "/") {
		return false
	}
	for _, s := range strings.Split(p, "/") {
		if s == ".." || s == "." || strings.TrimRight(s, " .") != s {
			return false
		}
		u := strings.ToUpper(strings.Split(s, ".")[0])
		if u == "CON" || u == "PRN" || u == "AUX" || u == "NUL" || regexp.MustCompile(`^(COM|LPT)[0-9]$`).MatchString(u) {
			return false
		}
	}
	return true
}

// The Windows payload deliberately excludes every mutable configuration and plugin directory.
// New defaults are distributed in the clean portable package, never written over an existing profile.
func ownedPath(p, platform string) bool {
	if !validPath(p) {
		return false
	}
	if platform == "darwin" {
		return strings.HasPrefix(p, "Contents/") && !strings.Contains(strings.ToLower(p), "/plugins/")
	}
	switch p {
	case "NoteQu.exe", "NoteQuUpdater.exe", "LICENSE.txt", "README.md", "CHANGELOG.md", "MODIFICATIONS.md", "THIRD_PARTY_NOTICES.md", "BUILD.md", "SOURCE_CODE.txt", "UPDATING.md", "UPDATE-BOOTSTRAP.md":
		return true
	}
	return strings.HasPrefix(p, "LICENSES/")
}
func verify(raw, sig, key []byte, platform, arch string) (Signed, error) {
	var out Signed
	if len(raw) > 2<<20 || len(key) != ed25519.PublicKeySize || !ed25519.Verify(key, raw, sig) {
		return out, errors.New("update signature is invalid")
	}
	d := json.NewDecoder(bytes.NewReader(raw))
	d.DisallowUnknownFields()
	if e := d.Decode(&out.Manifest); e != nil {
		return out, e
	}
	if d.Decode(new(any)) != io.EOF {
		return out, errors.New("trailing manifest data")
	}
	m := out.Manifest
	if m.Schema != 1 || m.Repository != repository || m.Platform != platform || m.Architecture != arch || !versionPattern.MatchString(m.Version) || len(m.Source) != 40 || len(m.Files) == 0 || len(m.Files) > 8192 {
		return out, errors.New("incompatible update manifest")
	}
	if m.URL != downloadBase+"v"+m.Version+"/NoteQu-"+m.Version+"-"+platform+"-"+arch+"-update.zip" {
		return out, errors.New("unexpected package location")
	}
	seen := map[string]bool{}
	var total int64
	for p, f := range m.Files {
		k := strings.ToLower(p)
		if !ownedPath(p, platform) || seen[k] || f.Size < 0 || f.Size > maxPackage || len(f.SHA256) != 64 {
			return out, errors.New("invalid owned file")
		}
		if _, e := hex.DecodeString(f.SHA256); e != nil {
			return out, e
		}
		seen[k] = true
		total += f.Size
		if total > maxPackage {
			return out, errors.New("update too large")
		}
	}
	entry := "NoteQu.exe"
	helper := "NoteQuUpdater.exe"
	if platform == "darwin" {
		entry = "Contents/MacOS/NoteQu"
		helper = "Contents/MacOS/NoteQuUpdater"
	}
	if _, ok := m.Files[entry]; !ok {
		return out, errors.New("missing application")
	}
	if _, ok := m.Files[helper]; !ok {
		return out, errors.New("missing updater")
	}
	out.Bytes = raw
	out.Signature = sig
	return out, nil
}
func publicKey() []byte { k, _ := base64.StdEncoding.DecodeString(trustedKey); return k }
func client() *http.Client {
	return &http.Client{Timeout: 45 * time.Second, Transport: &http.Transport{Proxy: http.ProxyFromEnvironment, DialContext: (&net.Dialer{Timeout: 5 * time.Second}).DialContext, TLSHandshakeTimeout: 5 * time.Second, ResponseHeaderTimeout: 10 * time.Second}, CheckRedirect: func(r *http.Request, via []*http.Request) error {
		if len(via) > 5 || r.URL.Scheme != "https" {
			return errors.New("unsafe redirect")
		}
		switch r.URL.Hostname() {
		case "github.com", "release-assets.githubusercontent.com", "objects.githubusercontent.com":
			return nil
		}
		return errors.New("unexpected download host")
	}}
}
func fetch(ctx context.Context, c *http.Client, address string, limit int64) ([]byte, error) {
	u, e := url.Parse(address)
	if e != nil || u.Scheme != "https" || u.User != nil {
		return nil, errors.New("invalid HTTPS URL")
	}
	req, e := http.NewRequestWithContext(ctx, http.MethodGet, address, nil)
	if e != nil {
		return nil, e
	}
	req.Header.Set("User-Agent", "NoteQu-Updater")
	req.Header.Set("Accept", "application/vnd.github+json")
	res, e := c.Do(req)
	if e != nil {
		return nil, e
	}
	defer res.Body.Close()
	if res.StatusCode != 200 {
		return nil, fmt.Errorf("GitHub returned HTTP %d", res.StatusCode)
	}
	if res.ContentLength > limit {
		return nil, errors.New("download exceeds limit")
	}
	b, e := io.ReadAll(io.LimitReader(res.Body, limit+1))
	if int64(len(b)) > limit {
		return nil, errors.New("download exceeds limit")
	}
	return b, e
}

type Release struct {
	Tag        string `json:"tag_name"`
	Draft      bool   `json:"draft"`
	Prerelease bool   `json:"prerelease"`
	Assets     []struct {
		Name string `json:"name"`
		URL  string `json:"browser_download_url"`
	} `json:"assets"`
}

func candidates(rs []Release, current, platform, arch string) []Release {
	var out []Release
	name := "NoteQu-update-" + platform + "-" + arch + ".json"
	for _, r := range rs {
		if r.Draft || r.Prerelease || !strings.HasPrefix(r.Tag, "v") || !newer(strings.TrimPrefix(r.Tag, "v"), current) {
			continue
		}
		have := map[string]bool{}
		for _, a := range r.Assets {
			if a.URL == downloadBase+r.Tag+"/"+a.Name {
				have[a.Name] = true
			}
		}
		if have[name] && have[name+".sig"] {
			out = append(out, r)
		}
	}
	sort.Slice(out, func(i, j int) bool { return newer(out[i].Tag[1:], out[j].Tag[1:]) })
	return out
}
func discover(ctx context.Context, current string) (*Signed, error) {
	c := client()
	var all []Release
	// Paginate the release list: a newer release for another platform must not mask ours.
	for page := 1; page <= 20; page++ {
		b, e := fetch(ctx, c, fmt.Sprintf("https://api.github.com/repos/%s/releases?per_page=100&page=%d", repository, page), 8<<20)
		if e != nil {
			return nil, e
		}
		var rs []Release
		if e = json.Unmarshal(b, &rs); e != nil {
			return nil, e
		}
		all = append(all, rs...)
		if len(rs) < 100 {
			break
		}
		if page == 20 {
			return nil, errors.New("release history exceeds supported limit")
		}
	}
	rs := candidates(all, current, runtime.GOOS, runtime.GOARCH)
	if len(rs) == 0 {
		return nil, nil
	}
	r := rs[0]
	base := downloadBase + r.Tag + "/NoteQu-update-" + runtime.GOOS + "-" + runtime.GOARCH + ".json"
	raw, e := fetch(ctx, c, base, 2<<20)
	if e != nil {
		return nil, e
	}
	sig, e := fetch(ctx, c, base+".sig", 128)
	if e != nil {
		return nil, e
	}
	s, e := verify(raw, sig, publicKey(), runtime.GOOS, runtime.GOARCH)
	if e != nil {
		return nil, e
	}
	if s.Manifest.Version != r.Tag[1:] {
		return nil, errors.New("release version mismatch")
	}
	return &s, nil
}
func digest(b []byte) string { h := sha256.Sum256(b); return hex.EncodeToString(h[:]) }
func atomicWrite(p string, b []byte, mode os.FileMode) error {
	if e := os.MkdirAll(filepath.Dir(p), 0700); e != nil {
		return e
	}
	f, e := os.CreateTemp(filepath.Dir(p), ".notequ-write-")
	if e != nil {
		return e
	}
	tmp := f.Name()
	defer os.Remove(tmp)
	if _, e = f.Write(b); e == nil {
		e = f.Sync()
	}
	if ce := f.Close(); e == nil {
		e = ce
	}
	if e != nil {
		return e
	}
	if e = os.Chmod(tmp, mode); e != nil {
		return e
	}
	return replaceFile(tmp, p)
}
func stage(ctx context.Context, s Signed, dir string) error {
	b, e := fetch(ctx, client(), s.Manifest.URL, maxPackage)
	if e != nil {
		return e
	}
	if e = extract(b, s, dir); e != nil {
		return e
	}
	return validateExecutables(dir, s)
}
func extract(b []byte, s Signed, dir string) error {
	z, e := zip.NewReader(bytes.NewReader(b), int64(len(b)))
	if e != nil {
		return e
	}
	seen := map[string]bool{}
	for _, f := range z.File {
		if f.FileInfo().IsDir() {
			continue
		}
		if !validPath(f.Name) || f.Mode()&os.ModeSymlink != 0 || seen[strings.ToLower(f.Name)] {
			return errors.New("unsafe archive entry")
		}
		seen[strings.ToLower(f.Name)] = true
		spec, ok := s.Manifest.Files[f.Name]
		if !ok {
			return errors.New("unexpected archive file")
		}
		if f.UncompressedSize64 != uint64(spec.Size) {
			return errors.New("archive size mismatch")
		}
		r, e := f.Open()
		if e != nil {
			return e
		}
		data, e := io.ReadAll(io.LimitReader(r, spec.Size+1))
		r.Close()
		if e != nil || int64(len(data)) != spec.Size || digest(data) != spec.SHA256 {
			return errors.New("archive file checksum mismatch")
		}
		mode := os.FileMode(0644)
		if spec.Executable {
			mode = 0755
		}
		if e = atomicWrite(filepath.Join(dir, filepath.FromSlash(f.Name)), data, mode); e != nil {
			return e
		}
	}
	if len(seen) != len(s.Manifest.Files) {
		return errors.New("incomplete archive")
	}
	return nil
}
