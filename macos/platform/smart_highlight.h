#pragma once

// Configure smart highlighting indicator styles for the given Scintilla view.
// Call this once during Scintilla setup.
void configureSmartHighlight(void* sci);

// Perform smart highlighting for the given Scintilla view.
// Call this on every SCN_UPDATEUI notification.
void doSmartHighlight(void* sci);

// Clear all smart highlight indicators from the given Scintilla view.
void clearSmartHighlight(void* sci);
