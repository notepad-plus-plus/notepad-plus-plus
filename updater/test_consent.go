// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
)

var testProfile string

// Integration tests may answer consent dialogs in an explicitly isolated temporary profile.
// Discovery, version checks, downloads, signatures, ownership, shutdown waits and recovery
// use the production paths unchanged. There is no test server or signature/version override.
func configureTestConsent(profile string) {
	if os.Getenv("NOTEX_TEST_UPDATE_PROFILE") != profile || !strings.HasPrefix(filepath.Base(profile), "test-update-") {
		return
	}
	relative, e := filepath.Rel(os.TempDir(), profile)
	if e != nil || relative == ".." || strings.HasPrefix(relative, ".."+string(os.PathSeparator)) {
		return
	}
	if st, e := os.Stat(filepath.Join(profile, "allow-update-test")); e != nil || !st.Mode().IsRegular() {
		return
	}
	testProfile = profile
}
func event(state string) {
	if testProfile == "" {
		return
	}
	b, _ := json.Marshal(map[string]string{"state": state})
	f, e := os.OpenFile(filepath.Join(testProfile, "update-events.jsonl"), os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0600)
	if e == nil {
		f.Write(append(b, '\n'))
		f.Sync()
		f.Close()
	}
}
func ask(s string) bool {
	if testProfile != "" {
		event("consent")
		return true
	}
	return askPlatform(s)
}
func tell(s string) {
	if testProfile != "" {
		event("message: " + s)
		return
	}
	tellPlatform(s)
}
