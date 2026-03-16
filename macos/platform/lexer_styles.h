// lexer_styles.h — Per-lexer style tables and language switching
// Part of the Notepad++ macOS port modular refactor.

#pragma once

struct StyleDef
{
	int styleId;
	int lightFore;  // BGR color
	int darkFore;   // BGR color
	bool bold;
	bool italic;
};

struct LexerStyles
{
	const char* lexerName;
	const StyleDef* styles;
	int numStyles;
};

const LexerStyles* findLexerStyles(const char* lexerName);
void applyLanguageToView(void* sci, int langIndex);
void applyLanguage(int langIndex);
