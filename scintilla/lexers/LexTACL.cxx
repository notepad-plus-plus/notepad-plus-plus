// Scintilla source code edit control
/** @file LexTAL.cxx
 ** Lexer for TAL
 ** Based on LexPascal.cxx
 ** Written by Laurent le Tynevez
 ** Updated by Simon Steele <s.steele@pnotepad.org> September 2002
 ** Updated by Mathias Rauen <scite@madshi.net> May 2003 (Delphi adjustments)
 ** Updated by Rod Falck, Aug 2006 Converted to TACL
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

inline bool isTACLoperator(char ch)
	{
	return ch == '\'' || isoperator(ch);
	}

inline bool isTACLwordchar(char ch)
	{
	return ch == '#' || ch == '^' || ch == '|' || ch == '_' || iswordchar(ch);
	}

inline bool isTACLwordstart(char ch)
	{
	return ch == '#' || ch == '|' || ch == '_' || iswordstart(ch);
	}

static void getRange(Sci_PositionU start,
		Sci_PositionU end,
		Accessor &styler,
		char *s,
		Sci_PositionU len) {
	Sci_PositionU i = 0;
	while ((i < end - start + 1) && (i < len-1)) {
		s[i] = static_cast<char>(tolower(styler[start + i]));
		i++;
	}
	s[i] = '\0';
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_C_COMMENT ||
		style == SCE_C_COMMENTDOC ||
		style == SCE_C_COMMENTDOCKEYWORD ||
		style == SCE_C_COMMENTDOCKEYWORDERROR;
}

static void ColourTo(Accessor &styler, Sci_PositionU end, unsigned int attr, bool bInAsm) {
	if ((bInAsm) && (attr == SCE_C_OPERATOR || attr == SCE_C_NUMBER || attr == SCE_C_DEFAULT || attr == SCE_C_WORD || attr == SCE_C_IDENTIFIER)) {
		styler.ColourTo(end, SCE_C_REGEX);
	} else
		styler.ColourTo(end, attr);
}

// returns 1 if the item starts a class definition, and -1 if the word is "end", and 2 if the word is "asm"
static int classifyWordTACL(Sci_PositionU start, Sci_PositionU end, /*WordList &keywords*/WordList *keywordlists[], Accessor &styler, bool bInAsm) {
	int ret = 0;

	WordList& keywords = *keywordlists[0];
	WordList& builtins = *keywordlists[1];
	WordList& commands = *keywordlists[2];

	char s[100];
	getRange(start, end, styler, s, sizeof(s));

	char chAttr = SCE_C_IDENTIFIER;
	if (isdigit(s[0]) || (s[0] == '.')) {
		chAttr = SCE_C_NUMBER;
	}
	else {
		if (s[0] == '#' || keywords.InList(s)) {
			chAttr = SCE_C_WORD;

			if (strcmp(s, "asm") == 0) {
				ret = 2;
			}
			else if (strcmp(s, "end") == 0) {
				ret = -1;
			}
		}
		else if (s[0] == '|' || builtins.InList(s)) {
			chAttr = SCE_C_WORD2;
		}
		else if (commands.InList(s)) {
			chAttr = SCE_C_UUID;
		}
		else if (strcmp(s, "comment") == 0) {
			chAttr = SCE_C_COMMENTLINE;
			ret = 3;
		}
	}
	ColourTo(styler, end, chAttr, (bInAsm && ret != -1));
	return ret;
}

static int classifyFoldPointTACL(const char* s) {
	int lev = 0;
	if (s[0] == '[')
		lev=1;
	else if (s[0] == ']')
		lev=-1;
	return lev;
}

static void ColouriseTACLDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
	Accessor &styler) {

	styler.StartAt(startPos);

	int state = initStyle;
	if (state == SCE_C_CHARACTER)	// Does not leak onto next line
		state = SCE_C_DEFAULT;
	char chPrev = ' ';
	char chNext = styler[startPos];
	Sci_PositionU lengthDoc = startPos + length;

	bool bInClassDefinition;

	Sci_Position currentLine = styler.GetLine(startPos);
	if (currentLine > 0) {
		styler.SetLineState(currentLine, styler.GetLineState(currentLine-1));
		bInClassDefinition = (styler.GetLineState(currentLine) == 1);
	} else {
		styler.SetLineState(currentLine, 0);
		bInClassDefinition = false;
	}

	bool bInAsm = (state == SCE_C_REGEX);
	if (bInAsm)
		state = SCE_C_DEFAULT;

	styler.StartSegment(startPos);
	int visibleChars = 0;
	Sci_PositionU i;
	for (i = startPos; i < lengthDoc; i++) {
		char ch = chNext;

		chNext = styler.SafeGetCharAt(i + 1);

		if ((ch == '\r' && chNext != '\n') || (ch == '\n')) {
			// Trigger on CR only (Mac style) or either on LF from CR+LF (Dos/Win) or on LF alone (Unix)
			// Avoid triggering two times on Dos/Win
			// End of line
			if (state == SCE_C_CHARACTER) {
				ColourTo(styler, i, state, bInAsm);
				state = SCE_C_DEFAULT;
			}
			visibleChars = 0;
			currentLine++;
			styler.SetLineState(currentLine, (bInClassDefinition ? 1 : 0));
		}

		if (styler.IsLeadByte(ch)) {
			chNext = styler.SafeGetCharAt(i + 2);
			chPrev = ' ';
			i += 1;
			continue;
		}

		if (state == SCE_C_DEFAULT) {
			if (isTACLwordstart(ch)) {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_IDENTIFIER;
			} else if (ch == '{') {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_COMMENT;
			} else if (ch == '{' && chNext == '*') {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_COMMENTDOC;
			} else if (ch == '=' && chNext == '=') {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_COMMENTLINE;
			} else if (ch == '"') {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_STRING;
			} else if (ch == '?' && visibleChars == 0) {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_PREPROCESSOR;
			} else if (isTACLoperator(ch)) {
				ColourTo(styler, i-1, state, bInAsm);
				ColourTo(styler, i, SCE_C_OPERATOR, bInAsm);
			}
		} else if (state == SCE_C_IDENTIFIER) {
			if (!isTACLwordchar(ch)) {
				int lStateChange = classifyWordTACL(styler.GetStartSegment(), i - 1, keywordlists, styler, bInAsm);

				if(lStateChange == 1) {
					styler.SetLineState(currentLine, 1);
					bInClassDefinition = true;
				} else if(lStateChange == 2) {
					bInAsm = true;
				} else if(lStateChange == -1) {
					styler.SetLineState(currentLine, 0);
					bInClassDefinition = false;
					bInAsm = false;
				}

				if (lStateChange == 3) {
					 state = SCE_C_COMMENTLINE;
				}
				else {
					state = SCE_C_DEFAULT;
					chNext = styler.SafeGetCharAt(i + 1);
					if (ch == '{') {
						state = SCE_C_COMMENT;
					} else if (ch == '{' && chNext == '*') {
						ColourTo(styler, i-1, state, bInAsm);
						state = SCE_C_COMMENTDOC;
					} else if (ch == '=' && chNext == '=') {
						state = SCE_C_COMMENTLINE;
					} else if (ch == '"') {
						state = SCE_C_STRING;
					} else if (isTACLoperator(ch)) {
						ColourTo(styler, i, SCE_C_OPERATOR, bInAsm);
					}
				}
			}
		} else {
			if (state == SCE_C_PREPROCESSOR) {
				if ((ch == '\r' || ch == '\n') && !(chPrev == '\\' || chPrev == '\r')) {
					ColourTo(styler, i-1, state, bInAsm);
					state = SCE_C_DEFAULT;
				}
			} else if (state == SCE_C_COMMENT) {
				if (ch == '}' || (ch == '\r' || ch == '\n') ) {
					ColourTo(styler, i, state, bInAsm);
					state = SCE_C_DEFAULT;
				}
			} else if (state == SCE_C_COMMENTDOC) {
				if (ch == '}' || (ch == '\r' || ch == '\n')) {
					if (((i > styler.GetStartSegment() + 2) || (
						(initStyle == SCE_C_COMMENTDOC) &&
						(styler.GetStartSegment() == static_cast<Sci_PositionU>(startPos))))) {
							ColourTo(styler, i, state, bInAsm);
							state = SCE_C_DEFAULT;
					}
				}
			} else if (state == SCE_C_COMMENTLINE) {
				if (ch == '\r' || ch == '\n') {
					ColourTo(styler, i-1, state, bInAsm);
					state = SCE_C_DEFAULT;
				}
			} else if (state == SCE_C_STRING) {
				if (ch == '"' || ch == '\r' || ch == '\n') {
					ColourTo(styler, i, state, bInAsm);
					state = SCE_C_DEFAULT;
				}
			}
		}
        if (!isspacechar(ch))
            visibleChars++;
		chPrev = ch;
	}

	// Process to end of document
	if (state == SCE_C_IDENTIFIER) {
		classifyWordTACL(styler.GetStartSegment(), i - 1, keywordlists, styler, bInAsm);
		}
	else
		ColourTo(styler, lengthDoc - 1, state, bInAsm);
}

static void FoldTACLDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *[],
                            Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	bool section = false;

	Sci_Position lastStart = 0;

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (stylePrev == SCE_C_DEFAULT && (style == SCE_C_WORD || style == SCE_C_PREPROCESSOR))
		{
			// Store last word start point.
			lastStart = i;
		}

		if (stylePrev == SCE_C_WORD || stylePrev == SCE_C_PREPROCESSOR) {
			if(isTACLwordchar(ch) && !isTACLwordchar(chNext)) {
				char s[100];
				getRange(lastStart, i, styler, s, sizeof(s));
				if (stylePrev == SCE_C_PREPROCESSOR && strcmp(s, "?section") == 0)
					{
					section = true;
					levelCurrent = 1;
					levelPrev = 0;
					}
				else if (stylePrev == SCE_C_WORD)
					levelCurrent += classifyFoldPointTACL(s);
			}
		}

		if (style == SCE_C_OPERATOR) {
			if (ch == '[') {
				levelCurrent++;
			} else if (ch == ']') {
				levelCurrent--;
			}
		}
		if (foldComment && (style == SCE_C_COMMENTLINE)) {
			if ((ch == '/') && (chNext == '/')) {
				char chNext2 = styler.SafeGetCharAt(i + 2);
				if (chNext2 == '{') {
					levelCurrent++;
				} else if (chNext2 == '}') {
					levelCurrent--;
				}
			}
		}

		if (foldPreprocessor && (style == SCE_C_PREPROCESSOR)) {
			if (ch == '{' && chNext == '$') {
				Sci_PositionU j=i+2; // skip {$
				while ((j<endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
					j++;
				}
				if (styler.Match(j, "region") || styler.Match(j, "if")) {
					levelCurrent++;
				} else if (styler.Match(j, "end")) {
					levelCurrent--;
				}
			}
		}

		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelCurrent++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}
		if (atEOL) {
			int lev = levelPrev | SC_FOLDLEVELBASE;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev || section) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
			section = false;
		}

		if (!isspacechar(ch))
			visibleChars++;
	}

	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const TACLWordListDesc[] = {
	"Builtins",
	"Labels",
	"Commands",
	0
};

LexerModule lmTACL(SCLEX_TACL, ColouriseTACLDoc, "TACL", FoldTACLDoc, TACLWordListDesc);
