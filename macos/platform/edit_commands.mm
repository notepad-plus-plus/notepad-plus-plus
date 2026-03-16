// edit_commands.mm — Edit menu commands (case, line ops, comments, sort, join)
// Part of the Notepad++ macOS port modular refactor.

#include "edit_commands.h"
#include "npp_constants.h"
#include "app_state.h"
#include "language_defs.h"
#include "scintilla_bridge.h"
#include <algorithm>

void doTitleCase()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;
	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
	if (selStart == selEnd) return;

	intptr_t len = selEnd - selStart;
	std::string text(len + 1, '\0');
	ScintillaBridge_sendMessage(sci, SCI_GETSELTEXT, 0, (intptr_t)text.data());
	text.resize(len);

	bool capitalize = true;
	for (size_t i = 0; i < text.size(); ++i)
	{
		if (isspace(static_cast<unsigned char>(text[i])) || text[i] == '-' || text[i] == '_')
			capitalize = true;
		else if (capitalize)
		{
			text[i] = toupper(static_cast<unsigned char>(text[i]));
			capitalize = false;
		}
		else
			text[i] = tolower(static_cast<unsigned char>(text[i]));
	}

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, len, (intptr_t)text.c_str());
	ScintillaBridge_sendMessage(sci, SCI_SETSEL, selStart, selStart + (intptr_t)text.size());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doTrimTrailingWhitespace()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;
	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);

	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
	for (intptr_t line = lineCount - 1; line >= 0; --line)
	{
		intptr_t lineEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, line, 0);
		intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
		intptr_t trimPos = lineEnd;
		while (trimPos > lineStart)
		{
			char ch = (char)ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, trimPos - 1, 0);
			if (ch != ' ' && ch != '\t') break;
			--trimPos;
		}
		if (trimPos < lineEnd)
			ScintillaBridge_sendMessage(sci, SCI_DELETERANGE, trimPos, lineEnd - trimPos);
	}
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doRemoveEmptyLines()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;
	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);

	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
	for (intptr_t line = lineCount - 1; line >= 0; --line)
	{
		intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
		intptr_t lineEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, line, 0);
		if (lineStart == lineEnd)
		{
			intptr_t nextLineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line + 1, 0);
			if (nextLineStart > lineStart)
				ScintillaBridge_sendMessage(sci, SCI_DELETERANGE, lineStart, nextLineStart - lineStart);
		}
	}
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doToggleLineComment()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	const char* prefix = "//";
	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
	{
		int langIdx = docs[tabIdx].languageIndex;
		if (langIdx >= 0 && langIdx < g_numLanguages)
		{
			const char* lexer = g_languages[langIdx].lexerName;
			if (strcmp(lexer, "python") == 0 || strcmp(lexer, "bash") == 0)
				prefix = "#";
			else if (strcmp(lexer, "sql") == 0)
				prefix = "--";
		}
	}

	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
	intptr_t startLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, selStart, 0);
	intptr_t endLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, selEnd, 0);
	if (selEnd == ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, endLine, 0) && endLine > startLine)
		--endLine;

	size_t prefixLen = strlen(prefix);

	bool allCommented = true;
	for (intptr_t line = startLine; line <= endLine; ++line)
	{
		intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
		intptr_t lineEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, line, 0);
		if (lineEnd - lineStart < (intptr_t)prefixLen)
		{
			allCommented = false;
			break;
		}
		bool match = true;
		for (size_t j = 0; j < prefixLen; ++j)
		{
			if ((char)ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, lineStart + j, 0) != prefix[j])
			{
				match = false;
				break;
			}
		}
		if (!match) { allCommented = false; break; }
	}

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	if (allCommented)
	{
		for (intptr_t line = endLine; line >= startLine; --line)
		{
			intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
			ScintillaBridge_sendMessage(sci, SCI_DELETERANGE, lineStart, prefixLen);
		}
	}
	else
	{
		for (intptr_t line = endLine; line >= startLine; --line)
		{
			intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
			ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, lineStart, 0);
			ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, lineStart, 0);
			ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, prefixLen, (intptr_t)prefix);
		}
	}
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doSortLines(bool ascending)
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
	if (selStart == selEnd) return;

	intptr_t startLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, selStart, 0);
	intptr_t endLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, selEnd, 0);

	std::vector<std::string> lines;
	for (intptr_t line = startLine; line <= endLine; ++line)
	{
		intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
		intptr_t lineEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, line, 0);
		intptr_t len = lineEnd - lineStart;
		std::string text(len, '\0');
		for (intptr_t i = 0; i < len; ++i)
			text[i] = (char)ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, lineStart + i, 0);
		lines.push_back(text);
	}

	if (ascending)
		std::sort(lines.begin(), lines.end());
	else
		std::sort(lines.begin(), lines.end(), std::greater<std::string>());

	std::string result;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		result += lines[i];
		if (i + 1 < lines.size()) result += '\n';
	}

	intptr_t replStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, startLine, 0);
	intptr_t replEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, endLine, 0);

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, replStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, replEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, result.size(), (intptr_t)result.c_str());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doJoinLines()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
	if (selStart == selEnd) return;

	intptr_t len = selEnd - selStart;
	std::string text(len + 1, '\0');
	ScintillaBridge_sendMessage(sci, SCI_GETSELTEXT, 0, (intptr_t)text.data());
	text.resize(len);

	std::string result;
	result.reserve(text.size());
	for (size_t i = 0; i < text.size(); ++i)
	{
		if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
		{
			result += ' ';
			++i;
		}
		else if (text[i] == '\r' || text[i] == '\n')
			result += ' ';
		else
			result += text[i];
	}

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, result.size(), (intptr_t)result.c_str());
	ScintillaBridge_sendMessage(sci, SCI_SETSEL, selStart, selStart + (intptr_t)result.size());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}
