#pragma once

// Perform smart highlighting for the given Scintilla view.
void doSmartHighlight(void* sci);

// Clear smart-highlight indicators from the given view.
void clearSmartHighlight(void* sci);

// Configure smart-highlight indicator styles.
void configureSmartHighlightIndicator(void* sci, bool isDark);

// Shared helper: highlight all occurrences of text using the given indicator.
// Returns the number of matches highlighted.
int highlightAllOccurrences(void* sci, const char* utf8Text, int searchFlags,
                            int indicatorNum, int maxMatches = 2000);
