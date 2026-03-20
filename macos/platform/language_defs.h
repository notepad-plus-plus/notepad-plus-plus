// language_defs.h — Language definitions and extension-based detection
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#include <string>

struct LangDef
{
	const char* name;          // Display name
	const char* lexerName;     // Scintilla lexer name
	const char* keywords;      // Primary keywords (set 0)
	const char* keywords2;     // Secondary keywords (set 1) - types, builtins
	int menuId;                // Menu command ID
};

extern const LangDef g_languages[];
extern const int g_numLanguages;

int guessLanguage(const std::wstring& filePath);

// Returns true if the language uses C-style { } block indentation.
bool isCStyleLanguage(int languageIndex);
