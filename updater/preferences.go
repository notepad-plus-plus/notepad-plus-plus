// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"encoding/json"
	"errors"
	"os"
)

func readSettings(path string) (map[string]json.RawMessage, error) {
	settings := map[string]json.RawMessage{}
	b, e := os.ReadFile(path)
	if os.IsNotExist(e) {
		return settings, nil
	}
	if e != nil {
		return nil, e
	}
	if e = json.Unmarshal(b, &settings); e != nil {
		return nil, e
	}
	if settings == nil {
		return nil, errors.New("update settings must be an object")
	}
	if b, ok := settings["disabled"]; ok {
		var disabled bool
		if e = json.Unmarshal(b, &disabled); e != nil {
			return nil, e
		}
	}
	if b, ok := settings["lastCheck"]; ok {
		var last int64
		if e = json.Unmarshal(b, &last); e != nil {
			return nil, e
		}
	}
	return settings, nil
}
func checksDisabled(settings map[string]json.RawMessage) bool {
	var disabled bool
	_ = json.Unmarshal(settings["disabled"], &disabled)
	return disabled
}
func lastCheck(settings map[string]json.RawMessage) int64 {
	var last int64
	_ = json.Unmarshal(settings["lastCheck"], &last)
	return last
}
