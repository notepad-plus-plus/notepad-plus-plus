#pragma once

// Perform auto-indentation after a newline character is added.
// Call this from SCN_CHARADDED with the character that was added.
// languageIndex is used to gate brace-aware indentation to C-style languages.
void performAutoIndent(void* sci, int charAdded, int languageIndex = -1);
