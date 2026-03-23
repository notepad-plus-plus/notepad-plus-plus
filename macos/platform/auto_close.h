#pragma once

struct SciNotification;

// Called from SCN_CHARADDED (before auto-indent) for pair insertion/skip behavior.
void handleAutoCloseCharAdded(void* sci, int ch, int languageIndex);

// Called from SCN_MODIFIED for backspace pair-delete and selection-wrap tracking.
void handleAutoCloseModified(void* sci, const SciNotification* scn, int languageIndex);

// Clears cached per-view auto-close state before a Scintilla view is destroyed.
void autoCloseOnViewDestroyed(void* sci);
