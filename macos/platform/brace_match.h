#pragma once

// Perform brace matching for the given Scintilla view.
// Call this on every SCN_UPDATEUI notification.
void doBraceMatch(void* sci);

// Jump the caret to the matching brace.
void doGoToMatchingBrace(void* sci);

// Configure brace highlight styles for light/dark mode.
void configureBraceStyles(void* sci, bool isDark);
