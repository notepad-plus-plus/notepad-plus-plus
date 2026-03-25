// auto_indent.mm — Auto-indentation on Enter + smart brace indentation
// Part of the Notepad++ macOS port modular refactor.

#include "auto_indent.h"
#include "npp_constants.h"
#include "language_defs.h"
#include "scintilla_bridge.h"
#include <string>
#include <vector>

// Find the last significant character on a C-style source line.
// Trailing comments and quoted content are ignored.
static char lastSignificantCStyleChar(const char* buf, intptr_t len)
{
	char last = '\0';
	bool inLineComment = false;
	bool inBlockComment = false;
	bool inString = false;
	bool inChar = false;
	bool escaped = false;

	for (intptr_t i = 0; i < len; ++i)
	{
		char c = buf[i];

		if (inLineComment)
		{
			if (c == '\r' || c == '\n')
				inLineComment = false;
			continue;
		}

		if (inBlockComment)
		{
			if (c == '*' && (i + 1) < len && buf[i + 1] == '/')
			{
				inBlockComment = false;
				++i;
			}
			continue;
		}

		if (inString)
		{
			if (escaped)
			{
				escaped = false;
				continue;
			}
			if (c == '\\')
			{
				escaped = true;
				continue;
			}
			if (c == '"')
				inString = false;
			continue;
		}

		if (inChar)
		{
			if (escaped)
			{
				escaped = false;
				continue;
			}
			if (c == '\\')
			{
				escaped = true;
				continue;
			}
			if (c == '\'')
				inChar = false;
			continue;
		}

		if (c == '/' && (i + 1) < len)
		{
			if (buf[i + 1] == '/')
			{
				inLineComment = true;
				++i;
				continue;
			}
			if (buf[i + 1] == '*')
			{
				inBlockComment = true;
				++i;
				continue;
			}
		}

		if (c == '"')
		{
			inString = true;
			continue;
		}
		if (c == '\'')
		{
			inChar = true;
			continue;
		}

		if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
			continue;

		last = c;
	}

	return last;
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
				char prevSigChar = lastSignificantCStyleChar(prevBuf.data(), prevLineLen);

				intptr_t newIndent;
				if (prevSigChar == '{')
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
		char prevSigChar = lastSignificantCStyleChar(lineBuf.data(), prevLineLen);
		if (prevSigChar == '{')
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

	// Check if the cursor sits immediately before a closing brace.
	// If so, split the brace onto its own line with base indentation.
	bool splitBrace = false;
	if (addIndent)
	{
		char nextCh = static_cast<char>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, curPos, 0));
		splitBrace = (nextCh == '}');
	}

	std::string toInsert = indent;
	if (splitBrace)
	{
		intptr_t eolMode = ScintillaBridge_sendMessage(sci, SCI_GETEOLMODE, 0, 0);
		const char* eolStr = (eolMode == SC_EOL_CR) ? "\r" : (eolMode == SC_EOL_CRLF) ? "\r\n" : "\n";
		toInsert += eolStr;
		toInsert += leadingWS;
	}

	// Insert the indentation on the new line as a single undoable action
	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACESEL, 0, (intptr_t)toInsert.c_str());
	if (splitBrace)
	{
		// Position cursor on the middle line (after indent, before the closing brace line)
		intptr_t newPos = curPos + static_cast<intptr_t>(indent.size());
		ScintillaBridge_sendMessage(sci, SCI_GOTOPOS, newPos, 0);
	}
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}
