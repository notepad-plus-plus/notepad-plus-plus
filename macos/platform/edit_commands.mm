// edit_commands.mm — Edit menu commands (case, line ops, comments, sort, join)
// Part of the Notepad++ macOS port modular refactor.

#import <Foundation/Foundation.h>
#include "edit_commands.h"
#include "npp_constants.h"
#include "app_state.h"
#include "language_defs.h"
#include "scintilla_bridge.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <random>
#include <unordered_set>
#include <vector>

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
	if (selEnd == ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, endLine, 0) && endLine > startLine)
		--endLine;

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

void doTabsToSpaces()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t tabWidth = ScintillaBridge_sendMessage(sci, SCI_GETTABWIDTH, 0, 0);
	if (tabWidth <= 0) tabWidth = 4;
	std::string spaces(tabWidth, ' ');

	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	for (intptr_t line = lineCount - 1; line >= 0; --line)
	{
		intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
		intptr_t lineLen = ScintillaBridge_sendMessage(sci, SCI_LINELENGTH, line, 0);
		if (lineLen <= 0) continue;

		std::vector<char> buf(lineLen + 1, 0);
		ScintillaBridge_sendMessage(sci, SCI_GETLINE, line, (intptr_t)buf.data());

		std::string newLeading;
		intptr_t wsEnd = 0;
		for (intptr_t i = 0; i < lineLen; ++i)
		{
			if (buf[i] == '\t')
				newLeading += spaces;
			else if (buf[i] == ' ')
				newLeading += ' ';
			else
			{
				wsEnd = i;
				break;
			}
			wsEnd = i + 1;
		}

		if (wsEnd == 0) continue;

		std::string oldLeading(buf.data(), wsEnd);
		if (oldLeading == newLeading) continue;

		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, lineStart, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, lineStart + wsEnd, 0);
		ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, newLeading.size(), (intptr_t)newLeading.c_str());
	}
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doSpacesToTabs()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t tabWidth = ScintillaBridge_sendMessage(sci, SCI_GETTABWIDTH, 0, 0);
	if (tabWidth <= 0) tabWidth = 4;

	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	for (intptr_t line = lineCount - 1; line >= 0; --line)
	{
		intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
		intptr_t lineLen = ScintillaBridge_sendMessage(sci, SCI_LINELENGTH, line, 0);
		if (lineLen <= 0) continue;

		std::vector<char> buf(lineLen + 1, 0);
		ScintillaBridge_sendMessage(sci, SCI_GETLINE, line, (intptr_t)buf.data());

		// Count leading spaces
		intptr_t spaceCount = 0;
		intptr_t wsEnd = 0;
		for (intptr_t i = 0; i < lineLen; ++i)
		{
			if (buf[i] == ' ')
				++spaceCount;
			else if (buf[i] == '\t')
				spaceCount += tabWidth;
			else
			{
				wsEnd = i;
				break;
			}
			wsEnd = i + 1;
		}

		if (wsEnd == 0 || spaceCount == 0) continue;

		intptr_t numTabs = spaceCount / tabWidth;
		intptr_t extraSpaces = spaceCount % tabWidth;
		std::string newLeading(numTabs, '\t');
		newLeading.append(extraSpaces, ' ');

		std::string oldLeading(buf.data(), wsEnd);
		if (oldLeading == newLeading) continue;

		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, lineStart, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, lineStart + wsEnd, 0);
		ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, newLeading.size(), (intptr_t)newLeading.c_str());
	}
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void insertDateTimeShort()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
	[formatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
	NSString* dateStr = [formatter stringFromDate:[NSDate date]];
	const char* utf8 = [dateStr UTF8String];

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACESEL, 0, reinterpret_cast<intptr_t>(utf8));
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void insertDateTimeLong()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
	[formatter setDateStyle:NSDateFormatterFullStyle];
	[formatter setTimeStyle:NSDateFormatterMediumStyle];
	NSString* dateStr = [formatter stringFromDate:[NSDate date]];
	const char* utf8 = [dateStr UTF8String];

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACESEL, 0, reinterpret_cast<intptr_t>(utf8));
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

// ============================================================
// Helper: read selected lines from the active editor
// ============================================================
static bool readSelectedLines(void*& sci, intptr_t& startLine, intptr_t& endLine,
                              intptr_t& replStart, intptr_t& replEnd,
                              std::vector<std::string>& lines)
{
	sci = ctx().activeScintillaView();
	if (!sci) return false;

	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
	if (selStart == selEnd) return false;

	startLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, selStart, 0);
	endLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, selEnd, 0);
	if (selEnd == ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, endLine, 0) && endLine > startLine)
		--endLine;

	for (intptr_t line = startLine; line <= endLine; ++line)
	{
		intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, line, 0);
		intptr_t lineEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, line, 0);
		intptr_t contentLen = lineEnd - lineStart;
		// Use SCI_GETLINE for bulk read (includes EOL), then truncate to content length
		intptr_t fullLen = ScintillaBridge_sendMessage(sci, SCI_LINELENGTH, line, 0);
		if (fullLen > 0)
		{
			std::string buf(static_cast<size_t>(fullLen) + 1, '\0');
			ScintillaBridge_sendMessage(sci, SCI_GETLINE, line, reinterpret_cast<intptr_t>(buf.data()));
			lines.push_back(buf.substr(0, static_cast<size_t>(contentLen)));
		}
		else
		{
			lines.push_back(std::string());
		}
	}

	replStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, startLine, 0);
	replEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, endLine, 0);
	return true;
}

// Helper: replace the selected line range with reordered lines
static void replaceSelectedLines(void* sci, intptr_t replStart, intptr_t replEnd,
                                 const std::vector<std::string>& lines)
{
	// Use the document's EOL mode so we don't silently rewrite CRLF/CR to LF
	const char* eol = "\n";
	int eolLen = 1;
	int eolMode = static_cast<int>(ScintillaBridge_sendMessage(sci, SCI_GETEOLMODE, 0, 0));
	if (eolMode == SC_EOL_CRLF) { eol = "\r\n"; eolLen = 2; }
	else if (eolMode == SC_EOL_CR) { eol = "\r"; eolLen = 1; }

	std::string result;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		result += lines[i];
		if (i + 1 < lines.size()) result.append(eol, eolLen);
	}

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, replStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, replEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, result.size(), (intptr_t)result.c_str());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

// ============================================================
// Sprint P2 — Case conversions
// ============================================================

void doSentenceCase()
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
		unsigned char ch = static_cast<unsigned char>(text[i]);
		if (capitalize && isalpha(ch))
		{
			text[i] = toupper(ch);
			capitalize = false;
		}
		else if (isalpha(ch))
		{
			text[i] = tolower(ch);
		}

		// Sentence boundary: . ! ? followed by whitespace
		if ((text[i] == '.' || text[i] == '!' || text[i] == '?') &&
		    i + 1 < text.size() && isspace(static_cast<unsigned char>(text[i + 1])))
		{
			capitalize = true;
		}
	}

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, len, (intptr_t)text.c_str());
	ScintillaBridge_sendMessage(sci, SCI_SETSEL, selStart, selStart + (intptr_t)text.size());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doInvertCase()
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

	for (size_t i = 0; i < text.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(text[i]);
		if (isupper(ch))
			text[i] = tolower(ch);
		else if (islower(ch))
			text[i] = toupper(ch);
	}

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, len, (intptr_t)text.c_str());
	ScintillaBridge_sendMessage(sci, SCI_SETSEL, selStart, selStart + (intptr_t)text.size());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doCamelCase()
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

	// Split on whitespace, underscores, hyphens, and punctuation runs
	std::vector<std::string> tokens;
	std::string token;
	for (size_t i = 0; i < text.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(text[i]);
		if (isspace(ch) || ch == '_' || ch == '-' || (ispunct(ch) && ch != '\''))
		{
			if (!token.empty())
			{
				tokens.push_back(token);
				token.clear();
			}
		}
		else
		{
			token += text[i];
		}
	}
	if (!token.empty())
		tokens.push_back(token);

	// First token all lowercase, subsequent tokens leading-capitalized
	std::string result;
	for (size_t t = 0; t < tokens.size(); ++t)
	{
		std::string& tok = tokens[t];
		if (t == 0)
		{
			for (size_t i = 0; i < tok.size(); ++i)
				tok[i] = tolower(static_cast<unsigned char>(tok[i]));
		}
		else
		{
			tok[0] = toupper(static_cast<unsigned char>(tok[0]));
			for (size_t i = 1; i < tok.size(); ++i)
				tok[i] = tolower(static_cast<unsigned char>(tok[i]));
		}
		result += tok;
	}

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, result.size(), (intptr_t)result.c_str());
	ScintillaBridge_sendMessage(sci, SCI_SETSEL, selStart, selStart + (intptr_t)result.size());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

void doSnakeCase()
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

	// Split on whitespace/hyphens/underscores AND camelCase transitions
	std::vector<std::string> tokens;
	std::string token;
	for (size_t i = 0; i < text.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(text[i]);
		if (isspace(ch) || ch == '-' || ch == '_')
		{
			if (!token.empty())
			{
				tokens.push_back(token);
				token.clear();
			}
		}
		else if (isupper(ch) && !token.empty() &&
		         islower(static_cast<unsigned char>(token.back())))
		{
			// camelCase transition: lowercase followed by uppercase
			tokens.push_back(token);
			token.clear();
			token += text[i];
		}
		else
		{
			token += text[i];
		}
	}
	if (!token.empty())
		tokens.push_back(token);

	// Join with underscores, all lowercase
	std::string result;
	for (size_t t = 0; t < tokens.size(); ++t)
	{
		if (t > 0) result += '_';
		for (size_t i = 0; i < tokens[t].size(); ++i)
			result += tolower(static_cast<unsigned char>(tokens[t][i]));
	}

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET, result.size(), (intptr_t)result.c_str());
	ScintillaBridge_sendMessage(sci, SCI_SETSEL, selStart, selStart + (intptr_t)result.size());
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

// ============================================================
// Sprint P2 — Sort / line operation variants
// ============================================================

void doSortLinesCaseInsensitive()
{
	void* sci = nullptr;
	intptr_t startLine = 0, endLine = 0, replStart = 0, replEnd = 0;
	std::vector<std::string> lines;
	if (!readSelectedLines(sci, startLine, endLine, replStart, replEnd, lines))
		return;

	std::sort(lines.begin(), lines.end(), [](const std::string& a, const std::string& b)
	{
		size_t minLen = (a.size() < b.size()) ? a.size() : b.size();
		for (size_t i = 0; i < minLen; ++i)
		{
			int ca = tolower(static_cast<unsigned char>(a[i]));
			int cb = tolower(static_cast<unsigned char>(b[i]));
			if (ca != cb) return ca < cb;
		}
		return a.size() < b.size();
	});

	replaceSelectedLines(sci, replStart, replEnd, lines);
}

void doSortLinesReverse()
{
	void* sci = nullptr;
	intptr_t startLine = 0, endLine = 0, replStart = 0, replEnd = 0;
	std::vector<std::string> lines;
	if (!readSelectedLines(sci, startLine, endLine, replStart, replEnd, lines))
		return;

	std::reverse(lines.begin(), lines.end());
	replaceSelectedLines(sci, replStart, replEnd, lines);
}

void doRemoveDuplicateLines()
{
	void* sci = nullptr;
	intptr_t startLine = 0, endLine = 0, replStart = 0, replEnd = 0;
	std::vector<std::string> lines;
	if (!readSelectedLines(sci, startLine, endLine, replStart, replEnd, lines))
		return;

	// Preserve first-occurrence order
	std::unordered_set<std::string> seen;
	std::vector<std::string> unique;
	for (auto& line : lines)
	{
		if (seen.insert(line).second)
			unique.push_back(line);
	}

	replaceSelectedLines(sci, replStart, replEnd, unique);
}

void doSortLinesNumeric()
{
	void* sci = nullptr;
	intptr_t startLine = 0, endLine = 0, replStart = 0, replEnd = 0;
	std::vector<std::string> lines;
	if (!readSelectedLines(sci, startLine, endLine, replStart, replEnd, lines))
		return;

	std::sort(lines.begin(), lines.end(), [](const std::string& a, const std::string& b)
	{
		char* endA = nullptr;
		char* endB = nullptr;
		long numA = strtol(a.c_str(), &endA, 10);
		long numB = strtol(b.c_str(), &endB, 10);
		bool aIsNum = (endA != a.c_str());
		bool bIsNum = (endB != b.c_str());

		// Numeric lines sort before non-numeric
		if (aIsNum && !bIsNum) return true;
		if (!aIsNum && bIsNum) return false;
		if (!aIsNum && !bIsNum) return a < b;
		if (numA != numB) return numA < numB;
		return a < b;
	});

	replaceSelectedLines(sci, replStart, replEnd, lines);
}

void doSortLinesRandom()
{
	void* sci = nullptr;
	intptr_t startLine = 0, endLine = 0, replStart = 0, replEnd = 0;
	std::vector<std::string> lines;
	if (!readSelectedLines(sci, startLine, endLine, replStart, replEnd, lines))
		return;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::shuffle(lines.begin(), lines.end(), gen);

	replaceSelectedLines(sci, replStart, replEnd, lines);
}
