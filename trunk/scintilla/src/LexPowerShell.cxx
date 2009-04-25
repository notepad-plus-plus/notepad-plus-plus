// Scintilla source code edit control
/** @file LexPowerShell.cxx
 ** Lexer for PowerShell scripts.
 **/
// Copyright 2008 by Tim Gerundt <tim@gerundt.de>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

// Extended to accept accented characters
static inline bool IsAWordChar(int ch) {
	return ch >= 0x80 || isalnum(ch) || ch == '-';
}

static void ColourisePowerShellDoc(unsigned int startPos, int length, int initStyle,
                           WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];

	styler.StartAt(startPos);

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.state == SCE_POWERSHELL_COMMENT) {
			if (sc.atLineEnd) {
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_STRING) {
			// This is a doubles quotes string
			if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_CHARACTER) {
			// This is a single quote string
			if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_NUMBER) {
			if (!IsADigit(sc.ch)) {
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_VARIABLE) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_OPERATOR) {
			if (!isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		} else if (sc.state == SCE_POWERSHELL_IDENTIFIER) {
			if (!IsAWordChar(sc.ch)) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));

				if (keywords.InList(s)) {
					sc.ChangeState(SCE_POWERSHELL_KEYWORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_POWERSHELL_CMDLET);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_POWERSHELL_ALIAS);
				}
				sc.SetState(SCE_POWERSHELL_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_POWERSHELL_DEFAULT) {
			if (sc.ch == '#') {
				sc.SetState(SCE_POWERSHELL_COMMENT);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_POWERSHELL_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_POWERSHELL_CHARACTER);
			} else if (sc.ch == '$') {
				sc.SetState(SCE_POWERSHELL_VARIABLE);
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_POWERSHELL_NUMBER);
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_POWERSHELL_OPERATOR);
			} else if (IsAWordChar(sc.ch)) {
				sc.SetState(SCE_POWERSHELL_IDENTIFIER);
			}
		}
	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldPowerShellDoc(unsigned int startPos, int length, int,
                           WordList *[], Accessor &styler) {
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (style == SCE_POWERSHELL_OPERATOR) {
			if (ch == '{') {
				// Measure the minimum before a '{' to allow
				// folding on "} else {"
				if (levelMinCurrent > levelNext) {
					levelMinCurrent = levelNext;
				}
				levelNext++;
			} else if (ch == '}') {
				levelNext--;
			}
		}
		if (!IsASpace(ch))
			visibleChars++;
		if (atEOL || (i == endPos-1)) {
			int levelUse = levelCurrent;
			if (foldAtElse) {
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			visibleChars = 0;
		}
	}
}

static const char * const powershellWordLists[] = {
	"Commands",
	"Cmdlets",
	"Aliases",
	0
};

LexerModule lmPowerShell(SCLEX_POWERSHELL, ColourisePowerShellDoc, "powershell", FoldPowerShellDoc, powershellWordLists);

