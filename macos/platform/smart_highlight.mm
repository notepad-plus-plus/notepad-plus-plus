// smart_highlight.mm — Smart highlighting (double-click word match)
// Part of the Notepad++ macOS port.

#include "smart_highlight.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"

#include <cstring>
#include <cctype>

// Indicator index matching Windows Notepad++ SCE_UNIVERSAL_FOUND_STYLE_SMART
static constexpr int SMART_HIGHLIGHT_INDICATOR = 29;

// Maximum lines to scan (matches Windows MAXLINEHIGHLIGHT)
static constexpr int MAX_HIGHLIGHT_LINES = 400;

// Maximum selection length to highlight
static constexpr int MAX_HIGHLIGHT_LENGTH = 1024;

void configureSmartHighlight(void* sci)
{
	if (!sci) return;

	ScintillaBridge_sendMessage(sci, SCI_INDICSETSTYLE, SMART_HIGHLIGHT_INDICATOR, INDIC_ROUNDBOX);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETALPHA, SMART_HIGHLIGHT_INDICATOR, 100);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETUNDER, SMART_HIGHLIGHT_INDICATOR, 1);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETFORE, SMART_HIGHLIGHT_INDICATOR, 0x00FF00); // green (BGR)
}

void clearSmartHighlight(void* sci)
{
	if (!sci) return;

	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETINDICATORCURRENT, SMART_HIGHLIGHT_INDICATOR, 0);
	ScintillaBridge_sendMessage(sci, SCI_INDICATORCLEARRANGE, 0, docLen);
}

void doSmartHighlight(void* sci)
{
	if (!sci) return;

	// Always clear existing highlights first
	clearSmartHighlight(sci);

	// Check if there is a selection
	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
	if (selStart == selEnd) return; // no selection

	intptr_t selLen = selEnd - selStart;
	if (selLen <= 0 || selLen > MAX_HIGHLIGHT_LENGTH) return;

	// Validate selection is a whole word
	intptr_t wordStart = ScintillaBridge_sendMessage(sci, SCI_WORDSTARTPOSITION, selStart, 1);
	intptr_t wordEnd = ScintillaBridge_sendMessage(sci, SCI_WORDENDPOSITION, selEnd, 1);
	if (wordStart != selStart || wordEnd != selEnd) return;

	// Extract selected text
	char buf[MAX_HIGHLIGHT_LENGTH + 1];
	ScintillaBridge_sendMessage(sci, SCI_GETSELTEXT, 0, reinterpret_cast<intptr_t>(buf));
	buf[selLen] = '\0';

	// Skip if text contains whitespace
	for (int i = 0; i < selLen; ++i)
	{
		if (std::isspace(static_cast<unsigned char>(buf[i])))
			return;
	}

	// Save existing target range to avoid interfering with find/replace
	intptr_t savedTargetStart = ScintillaBridge_sendMessage(sci, SCI_GETTARGETSTART, 0, 0);
	intptr_t savedTargetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);

	// Determine visible line range (capped at MAX_HIGHLIGHT_LINES)
	intptr_t firstVisLine = ScintillaBridge_sendMessage(sci, SCI_GETFIRSTVISIBLELINE, 0, 0);
	intptr_t linesOnScreen = ScintillaBridge_sendMessage(sci, SCI_LINESONSCREEN, 0, 0);

	// Convert visible lines to document lines (accounts for folding/wrapping)
	intptr_t firstDocLine = ScintillaBridge_sendMessage(sci, SCI_DOCLINEFROMVISIBLE, firstVisLine, 0);
	intptr_t lastDocLine = ScintillaBridge_sendMessage(sci, SCI_DOCLINEFROMVISIBLE, firstVisLine + linesOnScreen, 0);

	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
	if (lastDocLine >= lineCount)
		lastDocLine = lineCount - 1;

	// Cap at MAX_HIGHLIGHT_LINES
	if (lastDocLine - firstDocLine > MAX_HIGHLIGHT_LINES)
		lastDocLine = firstDocLine + MAX_HIGHLIGHT_LINES;

	intptr_t rangeStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE, firstDocLine, 0);
	intptr_t rangeEnd = ScintillaBridge_sendMessage(sci, SCI_GETLINEENDPOSITION, lastDocLine, 0);

	// Search for all occurrences in visible range
	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, SCFIND_WHOLEWORD | SCFIND_MATCHCASE, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETINDICATORCURRENT, SMART_HIGHLIGHT_INDICATOR, 0);

	intptr_t searchPos = rangeStart;
	while (searchPos < rangeEnd)
	{
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, searchPos, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, rangeEnd, 0);

		intptr_t matchPos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET, selLen,
			reinterpret_cast<intptr_t>(buf));
		if (matchPos < 0) break;

		// Don't highlight the selection itself
		if (matchPos != selStart)
		{
			ScintillaBridge_sendMessage(sci, SCI_INDICATORFILLRANGE, matchPos, selLen);
		}

		searchPos = matchPos + selLen;
	}

	// Restore target range
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, savedTargetStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, savedTargetEnd, 0);
}
