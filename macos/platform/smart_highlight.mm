// smart_highlight.mm — Smart highlighting (mark all occurrences)
// Part of the Notepad++ macOS port.

#include "smart_highlight.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"
#include <cstring>
#include <cctype>

void configureSmartHighlightIndicator(void* sci, bool isDark)
{
	if (!sci) return;

	ScintillaBridge_sendMessage(sci, SCI_INDICSETSTYLE, INDIC_SMART_HIGHLIGHT, INDIC_ROUNDBOX);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETALPHA, INDIC_SMART_HIGHLIGHT, 100);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETOUTLINEALPHA, INDIC_SMART_HIGHLIGHT, 255);
	ScintillaBridge_sendMessage(sci, SCI_INDICSETUNDER, INDIC_SMART_HIGHLIGHT, 1);

	// Green highlight in both modes (Scintilla colors are BGR)
	// Light: bright green, Dark: softer green
	int color = isDark ? 0x60FF60 : 0x00CC00;
	ScintillaBridge_sendMessage(sci, SCI_INDICSETFORE, INDIC_SMART_HIGHLIGHT, color);
}

void clearSmartHighlight(void* sci)
{
	if (!sci) return;

	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	if (docLen <= 0) return;

	ScintillaBridge_sendMessage(sci, SCI_SETINDICATORCURRENT, INDIC_SMART_HIGHLIGHT, 0);
	ScintillaBridge_sendMessage(sci, SCI_INDICATORCLEARRANGE, 0, docLen);
}

int highlightAllOccurrences(void* sci, const char* utf8Text, int searchFlags,
                            int indicatorNum, int maxMatches)
{
	if (!sci || !utf8Text || utf8Text[0] == '\0') return 0;

	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	if (docLen <= 0) return 0;

	size_t textLen = strlen(utf8Text);

	// Save current target range
	intptr_t savedTargetStart = ScintillaBridge_sendMessage(sci, SCI_GETTARGETSTART, 0, 0);
	intptr_t savedTargetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);

	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, searchFlags, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETINDICATORCURRENT, indicatorNum, 0);

	int count = 0;
	intptr_t searchStart = 0;

	while (searchStart < docLen && count < maxMatches)
	{
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, searchStart, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, docLen, 0);

		intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
		                                           textLen, (intptr_t)utf8Text);
		if (pos < 0) break;

		intptr_t targetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);
		intptr_t matchLen = targetEnd - pos;
		if (matchLen <= 0)
		{
			++searchStart;
			continue;
		}

		ScintillaBridge_sendMessage(sci, SCI_INDICATORFILLRANGE, pos, matchLen);
		++count;
		searchStart = targetEnd;
	}

	// Restore target range
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, savedTargetStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, savedTargetEnd, 0);

	return count;
}

void doSmartHighlight(void* sci)
{
	if (!sci) return;

	// Clear existing highlights first
	clearSmartHighlight(sci);

	// Get selection bounds
	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
	intptr_t selLen = selEnd - selStart;

	// Bail if selection is empty, too short, or too long
	if (selLen < 2 || selLen > 1024) return;

	// Get selected text
	char selBuf[1026] = {};
	ScintillaBridge_sendMessage(sci, SCI_GETSELTEXT, 0, (intptr_t)selBuf);

	// Bail if text contains whitespace
	for (int i = 0; selBuf[i] != '\0'; ++i)
	{
		if (std::isspace(static_cast<unsigned char>(selBuf[i])))
			return;
	}

	// Check whole-word boundaries: the selection should start at a word boundary
	// and end at a word boundary
	intptr_t wordStart = ScintillaBridge_sendMessage(sci, SCI_WORDSTARTPOSITION, selStart, 1);
	intptr_t wordEnd = ScintillaBridge_sendMessage(sci, SCI_WORDENDPOSITION, selEnd - 1, 1);
	if (wordStart != selStart || wordEnd != selEnd) return;

	// Bail if document is too large (> 5MB)
	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	if (docLen > 5 * 1024 * 1024) return;

	// Highlight all occurrences
	highlightAllOccurrences(sci, selBuf, SCFIND_MATCHCASE | SCFIND_WHOLEWORD,
	                        INDIC_SMART_HIGHLIGHT);
}
