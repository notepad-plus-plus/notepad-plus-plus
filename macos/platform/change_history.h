// change_history.h — Change history margin markers
// Part of the Notepad++ macOS port modular refactor.

#pragma once

// Configure the change-history margin and enable tracking on a Scintilla view.
// Must be called before content is loaded.
void configureChangeHistory(void* sci);

// Apply theme-aware marker colors for change history markers.
void applyChangeHistoryColors(void* sci, bool isDark);

// Show or hide the change-history margin on a single view.
void setChangeHistoryMarginVisible(void* sci, bool visible);
