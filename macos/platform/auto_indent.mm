// auto_indent.mm — Auto-indentation on Enter
// Part of the Notepad++ macOS port modular refactor.

#include "auto_indent.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"
#include <string>
#include <vector>

void performAutoIndent(void* sci, int charAdded)
{
	if (!sci) return;

	// CRLF guard: when EOL mode is CRLF, Scintilla fires SCN_CHARADDED for both
	// '\r' and '\n'. Skip '\r' unless the EOL mode is CR-only.
	if (charAdded == '\r')
	{
		intptr_t eolMode = ScintillaBridge_sendMessage(sci, SCI_GETEOLMODE, 0, 0);
		if (eolMode != SC_EOL_CR)
			return;
	}

	// Only act on newline characters
	if (charAdded != '\n' && charAdded != '\r')
		return;

	intptr_t curPos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	intptr_t curLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, curPos, 0);

	// Need a previous line to copy indentation from
	if (curLine < 1)
		return;

	intptr_t prevLine = curLine - 1;

	// Quick check: if previous line has no indentation, nothing to do
	intptr_t indent = ScintillaBridge_sendMessage(sci, SCI_GETLINEINDENTATION, prevLine, 0);
	if (indent <= 0)
		return;

	// Get previous line content and extract its leading whitespace
	intptr_t prevLineLen = ScintillaBridge_sendMessage(sci, SCI_LINELENGTH, prevLine, 0);
	if (prevLineLen <= 0)
		return;

	std::vector<char> lineBuf(prevLineLen + 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_GETLINE, prevLine, (intptr_t)lineBuf.data());

	std::string leadingWS;
	for (intptr_t i = 0; i < prevLineLen; ++i)
	{
		if (lineBuf[i] == ' ' || lineBuf[i] == '\t')
			leadingWS += lineBuf[i];
		else
			break;
	}

	if (leadingWS.empty())
		return;

	// Insert the indentation on the new line as a single undoable action
	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACESEL, 0, (intptr_t)leadingWS.c_str());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}
