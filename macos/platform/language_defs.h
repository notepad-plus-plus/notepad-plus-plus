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

// Named language indices for type-safe references
constexpr int LANG_NORMAL_TEXT = 0;
constexpr int LANG_C = 1;
constexpr int LANG_CPP = 2;
constexpr int LANG_JAVA = 3;
constexpr int LANG_PYTHON = 4;
constexpr int LANG_JAVASCRIPT = 5;
constexpr int LANG_HTML = 6;
constexpr int LANG_CSS = 7;
constexpr int LANG_XML = 8;
constexpr int LANG_JSON = 9;
constexpr int LANG_MARKDOWN = 10;
constexpr int LANG_SQL = 11;
constexpr int LANG_SHELL = 12;
constexpr int LANG_RUST = 13;
constexpr int LANG_GO = 14;
constexpr int LANG_OBJC = 15;
constexpr int LANG_SWIFT = 16;
constexpr int LANG_TYPESCRIPT = 17;
constexpr int LANG_PHP = 18;
constexpr int LANG_RUBY = 19;
constexpr int LANG_PERL = 20;
constexpr int LANG_LUA = 21;
constexpr int LANG_YAML = 22;
constexpr int LANG_TOML = 23;
constexpr int LANG_INI = 24;
constexpr int LANG_MAKEFILE = 25;
constexpr int LANG_DIFF = 26;
constexpr int LANG_DOCKERFILE = 27;
constexpr int LANG_CMAKE = 28;
constexpr int LANG_POWERSHELL = 29;
constexpr int LANG_R = 30;
constexpr int LANG_KOTLIN = 31;
constexpr int LANG_SCALA = 32;
constexpr int LANG_LATEX = 33;

extern const LangDef g_languages[];
extern const int g_numLanguages;

int guessLanguage(const std::wstring& filePath);

// Returns true if the language uses C-style { } block indentation.
bool isCStyleLanguage(int languageIndex);
