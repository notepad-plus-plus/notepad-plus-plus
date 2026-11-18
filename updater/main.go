// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
package main

import (
	"context"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"time"
)

func platformName() string { return runtime.GOOS }
func architecture() string { return runtime.GOARCH }
func main() {
	if e := run(); e != nil {
		fmt.Fprintln(os.Stderr, e)
		os.Exit(1)
	}
}
func run() error {
	mode := flag.String("mode", "manual", "auto, manual, settings, install, or recover")
	root := flag.String("root", "", "installation directory")
	profile := flag.String("profile", "", "local settings directory; never transmitted")
	current := flag.String("version", "", "running application version")
	parent := flag.Int("parent", 0, "running application process")
	work := flag.String("work", "", "verified staging directory")
	flag.Parse()
	if !filepath.IsAbs(*root) || !filepath.IsAbs(*profile) || !versionPattern.MatchString(*current) {
		return errors.New("invalid local updater arguments")
	}
	configureTestConsent(*profile)
	if *mode == "install" || *mode == "recover" {
		return install(*root, *profile, *current, *parent, *work, *mode == "recover")
	}
	state := stateDirectory(*root)
	if b, e := os.ReadFile(filepath.Join(state, "lock")); e == nil {
		pid, _ := strconv.Atoi(string(b))
		if pid > 0 && processAlive(pid) {
			return nil
		}
	}
	if _, e := os.Stat(filepath.Join(state, "pending.json")); e == nil {
		if !ask("An interrupted NoteX update needs recovery. Close NoteX normally after continuing. Only application files will be restored.") {
			return nil
		}
		previous, e := os.ReadFile(filepath.Join(state, "pending.json"))
		if e != nil {
			return e
		}
		var oldWork string
		if e = json.Unmarshal(previous, &oldWork); e != nil {
			return e
		}
		self, e := os.Executable()
		if e != nil {
			return e
		}
		helper := filepath.Join(oldWork, filepath.Base(self))
		cmd := exec.Command(helper, "-mode", "recover", "-root", *root, "-profile", *profile, "-version", *current, "-parent", strconv.Itoa(*parent), "-work", oldWork)
		detach(cmd)
		if e = cmd.Start(); e != nil {
			return e
		}
		return cmd.Process.Release()
	}
	configPath := filepath.Join(*profile, "notex-updates.json")
	settings := map[string]any{}
	if b, e := os.ReadFile(configPath); e == nil {
		if e = json.Unmarshal(b, &settings); e != nil {
			return e
		}
	} else if !os.IsNotExist(e) {
		return e
	}
	if *mode == "settings" {
		disabled, _ := settings["disabled"].(bool)
		prompt := "Automatic update checks are enabled. Disable them?"
		if disabled {
			prompt = "Automatic update checks are disabled. Enable them?"
		}
		if ask(prompt) {
			settings["disabled"] = !disabled
			b, _ := json.MarshalIndent(settings, "", "  ")
			return atomicWrite(configPath, b, 0600)
		}
		return nil
	}
	if *mode != "auto" && *mode != "manual" {
		return errors.New("unknown updater action")
	}
	if *mode == "auto" {
		disabled, _ := settings["disabled"].(bool)
		last, _ := settings["lastCheck"].(float64)
		if disabled || time.Since(time.Unix(int64(last), 0)) < 24*time.Hour {
			return nil
		}
	}
	settings["lastCheck"] = time.Now().Unix()
	b, _ := json.MarshalIndent(settings, "", "  ")
	if e := atomicWrite(configPath, b, 0600); e != nil {
		return e
	}
	ctx, cancel := context.WithTimeout(context.Background(), 60*time.Second)
	defer cancel()
	s, e := discover(ctx, *current)
	if e != nil {
		if *mode == "manual" {
			tell("The update check could not be completed. Your installation has not changed. Try again later.")
		}
		return e
	}
	if s == nil {
		if *mode == "manual" {
			tell("You are using the latest stable NoteX version available for this platform.")
		}
		return nil
	}
	event("discovered: " + s.Manifest.Version)
	if !ask("NoteX " + s.Manifest.Version + " is available. Download and verify the update?") {
		return nil
	}
	if e = establishOwnership(*root, *current); e != nil {
		tell("The installed application could not be matched to its signed release inventory. No files were changed. Reinstall the matching application files before updating.")
		return e
	}
	dir, e := os.MkdirTemp("", "NoteX-update-")
	if e != nil {
		return e
	}
	// Staging is retained on failure for recovery, and is never used without re-verification.
	downloadCtx, done := context.WithTimeout(context.Background(), 3*time.Minute)
	defer done()
	if e = stage(downloadCtx, *s, filepath.Join(dir, "payload")); e != nil {
		tell("The update could not be verified. No application files were changed.")
		return e
	}
	if e = atomicWrite(filepath.Join(dir, receiptName), s.Bytes, 0600); e != nil {
		return e
	}
	if e = atomicWrite(filepath.Join(dir, receiptName+".sig"), s.Signature, 0600); e != nil {
		return e
	}
	event("verified: " + s.Manifest.Version)
	if !ask("The update is verified. Install it after you close NoteX normally? Your documents, settings, and plugins will be preserved. NoteX will reopen after installation.") {
		return nil
	}
	self, e := os.Executable()
	if e != nil {
		return e
	}
	data, e := os.ReadFile(self)
	if e != nil {
		return e
	}
	helper := filepath.Join(dir, filepath.Base(self))
	if e = atomicWrite(helper, data, 0700); e != nil {
		return e
	}
	args := []string{"-mode", "install", "-root", *root, "-profile", *profile, "-version", *current, "-parent", strconv.Itoa(*parent), "-work", dir}
	cmd := exec.Command(helper, args...)
	detach(cmd)
	if e = cmd.Start(); e != nil {
		return e
	}
	return cmd.Process.Release()
}
func install(root, profile, current string, parent int, work string, recovery bool) error {
	if !filepath.IsAbs(work) || parent <= 0 {
		return errors.New("invalid installation request")
	}
	event("waiting for normal exit")
	// Open a process handle before waiting (Windows) to avoid PID reuse.
	if e := waitForExit(parent); e != nil {
		return e
	}
	if applicationRunning(root) {
		return errors.New("another NoteX instance is still using this installation; staged update retained")
	}
	state := stateDirectory(root)
	if e := os.MkdirAll(state, 0700); e != nil {
		return e
	}
	if b, e := os.ReadFile(filepath.Join(state, "lock")); e == nil {
		pid, _ := strconv.Atoi(string(b))
		if pid <= 0 || processAlive(pid) {
			return errors.New("another updater is active")
		}
		if e = os.Remove(filepath.Join(state, "lock")); e != nil {
			return e
		}
	}
	lock, e := os.OpenFile(filepath.Join(state, "lock"), os.O_CREATE|os.O_EXCL|os.O_WRONLY, 0600)
	if e != nil {
		return errors.New("another update or recovery is pending")
	}
	_, e = lock.WriteString(strconv.Itoa(os.Getpid()))
	if e == nil {
		e = lock.Sync()
	}
	lock.Close()
	if e != nil {
		return e
	}
	defer os.Remove(filepath.Join(state, "lock"))
	pendingPath := filepath.Join(state, "pending.json")
	if previous, e := os.ReadFile(pendingPath); e == nil {
		var oldWork string
		if e = json.Unmarshal(previous, &oldWork); e != nil {
			return e
		}
		if e = recoverTransaction(root, oldWork); e != nil {
			return e
		}
		if e = os.Remove(pendingPath); e != nil {
			return e
		}
	} else if !os.IsNotExist(e) {
		return e
	}
	if recovery {
		_, e = launchApplication(root, profile)
		return e
	}
	raw, e := os.ReadFile(filepath.Join(work, receiptName))
	if e != nil {
		return e
	}
	sig, e := os.ReadFile(filepath.Join(work, receiptName+".sig"))
	if e != nil {
		return e
	}
	s, e := verify(raw, sig, publicKey(), runtime.GOOS, runtime.GOARCH)
	if e != nil {
		return e
	}
	if !newer(s.Manifest.Version, current) {
		return errors.New("non-upgrade rejected")
	}
	work, e = durableStage(root, work, s)
	if e != nil {
		return e
	}
	b, _ := json.Marshal(work)
	if e = atomicWrite(pendingPath, b, 0600); e != nil {
		return e
	}
	if e = apply(root, filepath.Join(work, "payload"), work, s); e != nil {
		rollback := recoverTransaction(root, work)
		if rollback == nil {
			os.Remove(pendingPath)
		}
		tell("The update did not complete. Application recovery was attempted; your personal files were not changed.")
		return fmt.Errorf("install: %w; recovery: %v", e, rollback)
	}
	event("applied: " + s.Manifest.Version)
	health := filepath.Join(state, "healthy-"+s.Manifest.Version)
	os.Remove(health)
	proc, e := launchApplication(root, profile)
	if e != nil {
		_ = recoverTransaction(root, work)
		return e
	}
	exited := make(chan struct{})
	go func() { proc.Wait(); close(exited) }()
	alive := func() bool {
		select {
		case <-exited:
			return false
		default:
			return true
		}
	}
	for i := 0; i < 60; i++ {
		if data, e := os.ReadFile(health); e == nil && string(data) == s.Manifest.Version {
			time.Sleep(2 * time.Second)
			if !alive() {
				break
			}
			if e = commitTransaction(work); e != nil {
				return e
			}
			event("committed: " + s.Manifest.Version)
			return os.Remove(pendingPath)
		}
		if !alive() {
			break
		}
		time.Sleep(time.Second)
	}
	if alive() {
		tell("NoteX started, but startup verification is pending. Close NoteX normally before running recovery. Your personal files have not been rolled back.")
		return errors.New("startup confirmation missing; recovery retained")
	}
	if e = recoverTransaction(root, work); e != nil {
		return e
	}
	os.Remove(pendingPath)
	_, e = launchApplication(root, profile)
	return e
}
