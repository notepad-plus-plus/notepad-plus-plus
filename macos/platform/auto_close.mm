// auto_close.mm — Auto-close brackets and quotes.

#include "auto_close.h"
#include "app_state.h"
#include "language_defs.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"
#include "scintilla_notify.h"
#include "SciLexer.h"
#include <unordered_map>
#include <cctype>
#include <string>

namespace
{
struct PairDeleteInfo
{
	char deleted = '\0';
	intptr_t pos = -1;
	bool armed = false;
};

struct PendingWrapInfo
{
	bool armed = false;
	intptr_t position = -1;
	std::string deletedText;
};

std::unordered_map<void*, PairDeleteInfo> s_lastDeleteByView;
std::unordered_map<void*, PendingWrapInfo> s_pendingWrapByView;

bool isQuote(int ch)
{
	return ch == '"' || ch == '\'' || ch == '`';
}

bool isOpener(int ch)
{
	return ch == '(' || ch == '[' || ch == '{' || isQuote(ch);
}

bool isCloser(int ch)
{
	return ch == ')' || ch == ']' || ch == '}' || isQuote(ch);
}

char matchingClose(char opener)
{
	switch (opener)
	{
		case '(': return ')';
		case '[': return ']';
		case '{': return '}';
		case '"': return '"';
		case '\'': return '\'';
		case '`': return '`';
		default: return '\0';
	}
}

char matchingOpen(char closer)
{
	switch (closer)
	{
		case ')': return '(';
		case ']': return '[';
		case '}': return '{';
		case '"': return '"';
		case '\'': return '\'';
		case '`': return '`';
		default: return '\0';
	}
}

bool isBlankOrBoundary(char ch)
{
	return ch == '\0' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' ||
	       ch == ')' || ch == ']' || ch == '}';
}

bool isLikelyStringOrCommentStyle(int languageIndex, int style)
{
	switch (languageIndex)
	{
		case LANG_C:
		case LANG_CPP:
		case LANG_JAVA:
		case LANG_JAVASCRIPT:
		case LANG_TYPESCRIPT:
		case LANG_GO:
		case LANG_OBJC:
		case LANG_SWIFT:
		case LANG_RUST:
		case LANG_KOTLIN:
		case LANG_SCALA:
		case LANG_PHP:
			return style == SCE_C_COMMENT || style == SCE_C_COMMENTLINE ||
			       style == SCE_C_COMMENTDOC || style == SCE_C_STRING ||
			       style == SCE_C_CHARACTER || style == SCE_C_STRINGEOL ||
			       style == SCE_C_VERBATIM || style == SCE_C_COMMENTLINEDOC ||
			       style == SCE_C_COMMENTDOCKEYWORD || style == SCE_C_COMMENTDOCKEYWORDERROR ||
			       style == SCE_C_STRINGRAW || style == SCE_C_TRIPLEVERBATIM ||
			       style == SCE_C_HASHQUOTEDSTRING || style == SCE_C_PREPROCESSORCOMMENT ||
			       style == SCE_C_PREPROCESSORCOMMENTDOC || style == SCE_C_USERLITERAL ||
			       style == SCE_C_ESCAPESEQUENCE;
		case LANG_PYTHON:
			return style == SCE_P_COMMENTLINE || style == SCE_P_STRING ||
			       style == SCE_P_CHARACTER || style == SCE_P_TRIPLE ||
			       style == SCE_P_TRIPLEDOUBLE || style == SCE_P_COMMENTBLOCK ||
			       style == SCE_P_STRINGEOL || style == SCE_P_FSTRING ||
			       style == SCE_P_FCHARACTER || style == SCE_P_FTRIPLE ||
			       style == SCE_P_FTRIPLEDOUBLE;
		case LANG_SHELL:
			return style == SCE_SH_COMMENTLINE || style == SCE_SH_STRING ||
			       style == SCE_SH_CHARACTER || style == SCE_SH_BACKTICKS ||
			       style == SCE_SH_HERE_DELIM || style == SCE_SH_HERE_Q;
		case LANG_RUBY:
			return style == SCE_RB_COMMENTLINE || style == SCE_RB_POD ||
			       style == SCE_RB_STRING || style == SCE_RB_CHARACTER ||
			       style == SCE_RB_REGEX || style == SCE_RB_BACKTICKS ||
			       style == SCE_RB_DATASECTION || style == SCE_RB_HERE_DELIM ||
			       style == SCE_RB_HERE_Q || style == SCE_RB_HERE_QQ ||
			       style == SCE_RB_HERE_QX || style == SCE_RB_STRING_Q ||
			       style == SCE_RB_STRING_QQ || style == SCE_RB_STRING_QX ||
			       style == SCE_RB_STRING_QR || style == SCE_RB_STRING_QW ||
			       style == SCE_RB_STRING_W || style == SCE_RB_STRING_I ||
			       style == SCE_RB_STRING_QI || style == SCE_RB_STRING_QS;
		case LANG_PERL:
			return style == SCE_PL_COMMENTLINE || style == SCE_PL_POD ||
			       style == SCE_PL_STRING || style == SCE_PL_CHARACTER ||
			       style == SCE_PL_REGEX || style == SCE_PL_REGSUBST ||
			       style == SCE_PL_LONGQUOTE || style == SCE_PL_BACKTICKS ||
			       style == SCE_PL_DATASECTION || style == SCE_PL_HERE_DELIM ||
			       style == SCE_PL_HERE_Q || style == SCE_PL_HERE_QQ ||
			       style == SCE_PL_HERE_QX || style == SCE_PL_STRING_Q ||
			       style == SCE_PL_STRING_QQ || style == SCE_PL_STRING_QX ||
			       style == SCE_PL_STRING_QR || style == SCE_PL_STRING_QW ||
			       style == SCE_PL_POD_VERB || style == SCE_PL_FORMAT ||
			       style == SCE_PL_STRING_VAR || style == SCE_PL_XLAT ||
			       style == SCE_PL_REGEX_VAR || style == SCE_PL_REGSUBST_VAR ||
			       style == SCE_PL_BACKTICKS_VAR || style == SCE_PL_HERE_QQ_VAR ||
			       style == SCE_PL_HERE_QX_VAR || style == SCE_PL_STRING_QQ_VAR ||
			       style == SCE_PL_STRING_QX_VAR || style == SCE_PL_STRING_QR_VAR;
		case LANG_LUA:
			return style == SCE_LUA_COMMENT || style == SCE_LUA_COMMENTLINE ||
			       style == SCE_LUA_COMMENTDOC || style == SCE_LUA_STRING ||
			       style == SCE_LUA_CHARACTER || style == SCE_LUA_LITERALSTRING ||
			       style == SCE_LUA_STRINGEOL;
		default:
			return false;
	}
}

bool shouldSuppressQuotePair(void* sci, int languageIndex)
{
	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	if (pos <= 0)
		return false;

	int style = static_cast<int>(ScintillaBridge_sendMessage(sci, SCI_GETSTYLEAT, pos - 1, 0));
	return isLikelyStringOrCommentStyle(languageIndex, style);
}

void insertAtCaret(void* sci, const char* text)
{
	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_INSERTTEXT, pos, reinterpret_cast<intptr_t>(text));
}

void clearPendingWrap(void* sci)
{
	auto it = s_pendingWrapByView.find(sci);
	if (it == s_pendingWrapByView.end())
		return;
	it->second.armed = false;
	it->second.position = -1;
	it->second.deletedText.clear();
}
}

void autoCloseOnViewDestroyed(void* sci)
{
	if (!sci)
		return;
	s_lastDeleteByView.erase(sci);
	s_pendingWrapByView.erase(sci);
}

void handleAutoCloseCharAdded(void* sci, int ch, int languageIndex)
{
	if (!ctx().autoCloseBrackets || !sci || ctx().autoCloseInternalEdit)
		return;
	if (!isOpener(ch) && !isCloser(ch))
	{
		clearPendingWrap(sci);
		return;
	}

	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	if (pos < 1)
	{
		clearPendingWrap(sci);
		return;
	}

	PendingWrapInfo& pendingWrap = s_pendingWrapByView[sci];

	// Skip-over: if the character after the caret matches what was typed, move over it.
	// For bracket closers ()/]/}), always attempt skip-over and return.
	// For quotes, attempt skip-over; if no match, fall through to the opener/pairing logic.
	if (isCloser(ch))
	{
		int nextChar = static_cast<int>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, pos, 0));
		if (nextChar == ch)
		{
			ctx().autoCloseInternalEdit = true;
			ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
			ScintillaBridge_sendMessage(sci, SCI_DELETERANGE, pos - 1, 1);
			ScintillaBridge_sendMessage(sci, SCI_GOTOPOS, pos, 0);
			ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
			ctx().autoCloseInternalEdit = false;
			clearPendingWrap(sci);
			return;
		}
	}

	// Pure closers (not quotes) that didn't skip-over have nothing else to do.
	if (isCloser(ch) && !isQuote(ch))
	{
		clearPendingWrap(sci);
		return;
	}

	const char opener = static_cast<char>(ch);
	const char closer = matchingClose(opener);
	if (closer == '\0')
	{
		clearPendingWrap(sci);
		return;
	}

	if (isQuote(ch) && shouldSuppressQuotePair(sci, languageIndex))
	{
		clearPendingWrap(sci);
		return;
	}

	// Wrap deleted selection when the opener replaced it.
	if (pendingWrap.armed && pendingWrap.position == (pos - 1) && !pendingWrap.deletedText.empty())
	{
		std::string wrapText = pendingWrap.deletedText;
		wrapText.push_back(closer);

		ctx().autoCloseInternalEdit = true;
		ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
		insertAtCaret(sci, wrapText.c_str());
		intptr_t selStart = pos;
		intptr_t selEnd = pos + static_cast<intptr_t>(pendingWrap.deletedText.size());
		ScintillaBridge_sendMessage(sci, SCI_SETSEL, selStart, selEnd);
		ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
		ctx().autoCloseInternalEdit = false;

		clearPendingWrap(sci);
		return;
	}

	char prevChar = '\0';
	if (pos >= 2)
		prevChar = static_cast<char>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, pos - 2, 0));
	const char nextChar = static_cast<char>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, pos, 0));

	// Only auto-pair in boundary-ish contexts to avoid noisy insertions mid-token.
	if (!isBlankOrBoundary(nextChar))
	{
		clearPendingWrap(sci);
		return;
	}
	if (isQuote(ch) && !isBlankOrBoundary(prevChar) && prevChar != '(' && prevChar != '[' && prevChar != '{')
	{
		clearPendingWrap(sci);
		return;
	}

	char insertText[2] = { closer, '\0' };
	ctx().autoCloseInternalEdit = true;
	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	insertAtCaret(sci, insertText);
	ScintillaBridge_sendMessage(sci, SCI_GOTOPOS, pos, 0);
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
	ctx().autoCloseInternalEdit = false;
	clearPendingWrap(sci);
}

void handleAutoCloseModified(void* sci, const SciNotification* scn, int languageIndex)
{
	(void)languageIndex;
	if (!ctx().autoCloseBrackets || !sci || !scn || ctx().autoCloseInternalEdit)
		return;
	if (scn->nmhdr.code != SCN_MODIFIED)
		return;

	PairDeleteInfo& info = s_lastDeleteByView[sci];
	PendingWrapInfo& pendingWrap = s_pendingWrapByView[sci];

	if (scn->modificationType & SC_MOD_BEFOREDELETE)
	{
		// Reset pending wrap state; it will be re-armed below if this is a selection delete.
		pendingWrap.armed = false;
		pendingWrap.position = -1;
		pendingWrap.deletedText.clear();

		intptr_t selectionEmpty = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEMPTY, 0, 0);
		if (selectionEmpty == 0 && scn->position >= 0 && scn->length > 0)
		{
			// Selection is being replaced — capture deleted text so opener can wrap it.
			pendingWrap.position = scn->position;
			pendingWrap.deletedText.reserve(static_cast<size_t>(scn->length));
			for (intptr_t i = 0; i < scn->length; ++i)
			{
				char c = static_cast<char>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, scn->position + i, 0));
				pendingWrap.deletedText.push_back(c);
			}
			pendingWrap.armed = !pendingWrap.deletedText.empty();
		}
		else if (scn->length == 1 && scn->position >= 0)
		{
			// Single-char backspace — capture deleted char for pair-delete.
			char deleted = static_cast<char>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, scn->position, 0));
			info.deleted = deleted;
			info.pos = scn->position;
			info.armed = true;
		}
		return;
	}

	// After deletion, if we deleted an opener and caret sits before its closer, delete closer too.
	if ((scn->modificationType & SC_MOD_DELETETEXT) && info.armed && scn->length == 1)
	{
		info.armed = false;
		if (!isOpener(info.deleted))
			return;

		const char closer = matchingClose(info.deleted);
		if (closer == '\0')
			return;

		intptr_t caretPos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
		char next = static_cast<char>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, caretPos, 0));
		if (next != closer)
			return;

		ctx().autoCloseInternalEdit = true;
		ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_DELETERANGE, caretPos, 1);
		ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
		ctx().autoCloseInternalEdit = false;
	}
}
