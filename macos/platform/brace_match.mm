// brace_match.mm — Brace matching and highlighting
// Part of the Notepad++ macOS port modular refactor.

#include "brace_match.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"

static bool isBraceChar(int ch)
{
	return ch == '(' || ch == ')' || ch == '[' || ch == ']' ||
	       ch == '{' || ch == '}';
}

void doBraceMatch(void* sci)
{
	if (!sci) return;

	intptr_t caretPos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	intptr_t bracePos = -1;

	// Check character before caret first (higher priority, matches Windows N++ behavior)
	if (caretPos > 0)
	{
		int ch = static_cast<int>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, caretPos - 1, 0));
		if (isBraceChar(ch))
			bracePos = caretPos - 1;
	}

	// Then check character at caret
	if (bracePos < 0)
	{
		int ch = static_cast<int>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, caretPos, 0));
		if (isBraceChar(ch))
			bracePos = caretPos;
	}

	if (bracePos >= 0)
	{
		intptr_t matchPos = ScintillaBridge_sendMessage(sci, SCI_BRACEMATCH, bracePos, 0);
		if (matchPos >= 0)
			ScintillaBridge_sendMessage(sci, SCI_BRACEHIGHLIGHT, bracePos, matchPos);
		else
			ScintillaBridge_sendMessage(sci, SCI_BRACEBADLIGHT, bracePos, 0);
	}
	else
	{
		// No brace at cursor — clear both highlight states
		ScintillaBridge_sendMessage(sci, SCI_BRACEHIGHLIGHT, -1, -1);
		ScintillaBridge_sendMessage(sci, SCI_BRACEBADLIGHT, -1, 0);
	}
}

void configureBraceStyles(void* sci, bool isDark)
{
	if (!sci) return;

	// Style 34: STYLE_BRACELIGHT — matched braces
	ScintillaBridge_sendMessage(sci, SCI_STYLESETFORE, 34, isDark ? 0x00FF00 : 0xCC0000);
	ScintillaBridge_sendMessage(sci, SCI_STYLESETBACK, 34, isDark ? 0x3A3A3A : 0xE0E0E0);
	ScintillaBridge_sendMessage(sci, SCI_STYLESETBOLD, 34, 1);

	// Style 35: STYLE_BRACEBADLIGHT — unmatched brace
	ScintillaBridge_sendMessage(sci, SCI_STYLESETFORE, 35, 0x0000FF);
	ScintillaBridge_sendMessage(sci, SCI_STYLESETBACK, 35, isDark ? 0x2A2A3A : 0xFFE0E0);
	ScintillaBridge_sendMessage(sci, SCI_STYLESETBOLD, 35, 1);
}
