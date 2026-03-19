#pragma once

// Perform auto-indentation after a newline character is added.
// Call this from SCN_CHARADDED with the character that was added.
void performAutoIndent(void* sci, int charAdded);
