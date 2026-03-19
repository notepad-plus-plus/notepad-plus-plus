// keyboard_shortcuts.h — Native macOS keyboard shortcut configuration
// Part of the Notepad++ macOS port modular refactor.

#pragma once

// Configure native macOS keyboard shortcuts on a Scintilla view.
// Must be called after each ScintillaView is created.
void configureKeyboardShortcuts(void* sci);
