// auto_indent.mm — Auto-indentation on Enter + smart brace indentation
// Part of the Notepad++ macOS port modular refactor.

#include "auto_indent.h"
#include "npp_constants.h"
#include "language_defs.h"
#include "scintilla_bridge.h"
#include <string>
#include <vector>

// Find the last non-whitespace character on a line (excluding EOL chars).
static char lastNonWS(const char* buf, intptr_t len)
{
	for (intptr_t i = len - 1; i >= 0; --i)
	{
		char c = buf[i];
		if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
			continue;
		return c;
	}
	return '\0';
}

// Check if a line contains only whitespace before the given position.
static bool isOnlyWhitespaceBefore(const char* buf, intptr_t len)
{
	for (intptr_t i = 0; i < len; ++i)
	{
		if (buf[i] != ' ' && buf[i] != '\t')
			return false;
	}
	return true;
}

void performAutoIndent(void* sci, int charAdded, int languageIndex)
{
	if (!sci) return;

	bool isCStyle = isCStyleLanguage(languageIndex);

	// --- Handle '}' outdent for C-style languages ---
	if (isCStyle && charAdded == '}')
	{
		intptr_t curPos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
		intptr_t curLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, curPos, 0);

		// Get current line content up to cursor
		intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, curLine, 0);
		intptr_t prefixLen = curPos - lineStart - 1; // -1 for the '}' just typed
		if (prefixLen >= 0)
		{
			std::vector<char> buf(prefixLen + 1, 0);
			for (intptr_t i = 0; i < prefixLen; ++i)
				buf[i] = static_cast<char>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, lineStart + i, 0));

			if (isOnlyWhitespaceBefore(buf.data(), prefixLen) && curLine > 0)
			{
				intptr_t prevLine = curLine - 1;
				intptr_t prevIndent = ScintillaBridge_sendMessage(sci, SCI_GETLINEINDENTATION, prevLine, 0);
				intptr_t tabWidth = ScintillaBridge_sendMessage(sci, SCI_GETTABWIDTH, 0, 0);

				// Check if previous line ends with '{' (empty block case).
				// If so, align '}' with the '{' line rather than outdenting further.
				intptr_t prevLineLen = ScintillaBridge_sendMessage(sci, SCI_LINELENGTH, prevLine, 0);
				std::vector<char> prevBuf(prevLineLen + 1, 0);
				ScintillaBridge_sendMessage(sci, SCI_GETLINE, prevLine, (intptr_t)prevBuf.data());
				char lastCh = lastNonWS(prevBuf.data(), prevLineLen);

				intptr_t newIndent;
				if (lastCh == '{')
					newIndent = prevIndent;
				else
				{
					newIndent = prevIndent - tabWidth;
					if (newIndent < 0) newIndent = 0;
				}

				ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
				ScintillaBridge_sendMessage(sci, SCI_SETLINEINDENTATION, curLine, newIndent);
				// Position cursor after the indentation + the '}'
				intptr_t newLineEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, curLine, 0);
				ScintillaBridge_sendMessage(sci, SCI_GOTOPOS, newLineEnd, 0);
				ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
			}
		}
		return;
	}

	// --- Handle newline auto-indent ---

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

	// Check if previous line ends with '{' for C-style smart indent
	bool addIndent = false;
	if (isCStyle)
	{
		char last = lastNonWS(lineBuf.data(), prevLineLen);
		if (last == '{')
			addIndent = true;
	}

	if (leadingWS.empty() && !addIndent)
		return;

	std::string indent = leadingWS;
	if (addIndent)
	{
		// Add one indent unit using the current tab/space setting
		intptr_t useTabs = ScintillaBridge_sendMessage(sci, SCI_GETUSETABS, 0, 0);
		intptr_t tabWidth = ScintillaBridge_sendMessage(sci, SCI_GETTABWIDTH, 0, 0);
		if (useTabs)
			indent += '\t';
		else
			indent.append(tabWidth, ' ');
	}

	// Insert the indentation on the new line as a single undoable action
	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACESEL, 0, (intptr_t)indent.c_str());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}
