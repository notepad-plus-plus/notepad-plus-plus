// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestUnknownPreferencePrecision(t *testing.T) {
	p := filepath.Join(t.TempDir(), "settings.json")
	os.WriteFile(p, []byte(`{"disabled":true,"lastCheck":123,"future":{"counter":9007199254740993123456789,"values":[null,true,"keep"]}}`), 0600)
	m, e := readSettings(p)
	if e != nil {
		t.Fatal(e)
	}
	if !checksDisabled(m) || lastCheck(m) != 123 {
		t.Fatal("known preferences changed")
	}
	m["lastCheck"] = json.RawMessage("456")
	b, e := json.Marshal(m)
	if e != nil {
		t.Fatal(e)
	}
	if !strings.Contains(string(b), "9007199254740993123456789") {
		t.Fatal("unknown preference precision was lost")
	}
}
func TestMalformedPreferencesFailClosed(t *testing.T) {
	for _, data := range []string{`null`, `[]`, `{"disabled":"yes"}`, `{"lastCheck":"yesterday"}`} {
		p := filepath.Join(t.TempDir(), "settings.json")
		os.WriteFile(p, []byte(data), 0600)
		if _, e := readSettings(p); e == nil {
			t.Fatal("invalid preferences accepted")
		}
	}
}
