// document_data.h — Per-tab document state, encoding types
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "npp_constants.h"

// Encoding constants
enum DocEncoding
{
	ENC_UTF8 = 0,
	ENC_UTF8_BOM = 1,
	ENC_UTF16_LE = 2,
	ENC_UTF16_BE = 3,
	ENC_ANSI = 4,
};

inline const char* encodingName(int enc)
{
	switch (enc)
	{
		case ENC_UTF8:     return "UTF-8";
		case ENC_UTF8_BOM: return "UTF-8 BOM";
		case ENC_UTF16_LE: return "UTF-16 LE";
		case ENC_UTF16_BE: return "UTF-16 BE";
		case ENC_ANSI:     return "ANSI";
		default:           return "UTF-8";
	}
}

inline const char* eolName(int mode)
{
	switch (mode)
	{
		case SC_EOL_CRLF: return "CRLF";
		case SC_EOL_CR:   return "CR";
		case SC_EOL_LF:   return "LF";
		default:          return "LF";
	}
}

struct DocumentData
{
	std::wstring filePath;
	std::wstring title = L"Untitled";
	std::string content;
	intptr_t cursorPos = 0;
	intptr_t anchorPos = 0;
	intptr_t firstVisibleLine = 0;
	bool modified = false;
	bool savePointValid = true; // false when Scintilla's save point doesn't reflect real saved state
	int languageIndex = 2; // Default: C++
	std::vector<int> bookmarkedLines; // Persisted across tab switches
	int encoding = ENC_UTF8;
	int eolMode = SC_EOL_LF;
	int zoomLevel = 0;
	uint64_t functionListDocumentId = 0;
	uint64_t functionListRevision = 0;
};
