// Scintilla source code edit control
/** @file LexCOBOL.cxx
 ** Lexer for COBOL
 ** Based on LexPascal.cxx
 ** Written by Laurent le Tynevez
 ** Updated by Simon Steele <s.steele@pnotepad.org> September 2002
 ** Updated by Mathias Rauen <scite@madshi.net> May 2003 (Delphi adjustments)
 ** Updated by Rod Falck, Aug 2006 Converted to COBOL
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

#define IN_DIVISION 0x01
#define IN_DECLARATIVES 0x02
#define IN_SECTION 0x04
#define IN_PARAGRAPH 0x08
#define IN_FLAGS 0xF
#define NOT_HEADER 0x10

inline bool isCOBOLoperator(char ch)
    {
    return isoperator(ch);
    }

inline bool isCOBOLwordchar(char ch)
    {
    return IsASCII(ch) && (isalnum(ch) || ch == '-');

    }

inline bool isCOBOLwordstart(char ch)
    {
    return IsASCII(ch) && isalnum(ch);
    }

static int CountBits(int nBits)
	{
	int count = 0;
	for (int i = 0; i < 32; ++i)
		{
		count += nBits & 1;
		nBits >>= 1;
		}
	return count;
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

static void ColourTo(Accessor &styler, Sci_PositionU end, unsigned int attr) {
    styler.ColourTo(end, attr);
}


static int classifyWordCOBOL(Sci_PositionU start, Sci_PositionU end, /*WordList &keywords*/WordList *keywordlists[], Accessor &styler, int nContainment, bool *bAarea) {
    int ret = 0;

    WordList& a_keywords = *keywordlists[0];
    WordList& b_keywords = *keywordlists[1];
    WordList& c_keywords = *keywordlists[2];

    char s[100];
    s[0] = '\0';
    s[1] = '\0';
    getRange(start, end, styler, s, sizeof(s));

    char chAttr = SCE_C_IDENTIFIER;
    if (isdigit(s[0]) || (s[0] == '.') || (s[0] == 'v')) {
        chAttr = SCE_C_NUMBER;
		char *p = s + 1;
		while (*p) {
			if ((!isdigit(*p) && (*p) != 'v') && isCOBOLwordchar(*p)) {
				chAttr = SCE_C_IDENTIFIER;
			    break;
			}
			++p;
		}
    }
    else {
        if (a_keywords.InList(s)) {
            chAttr = SCE_C_WORD;
        }
        else if (b_keywords.InList(s)) {
            chAttr = SCE_C_WORD2;
        }
        else if (c_keywords.InList(s)) {
            chAttr = SCE_C_UUID;
        }
    }
    if (*bAarea) {
        if (strcmp(s, "division") == 0) {
            ret = IN_DIVISION;
			// we've determined the containment, anything else is just ignored for those purposes
			*bAarea = false;
		} else if (strcmp(s, "declaratives") == 0) {
            ret = IN_DIVISION | IN_DECLARATIVES;
			if (nContainment & IN_DECLARATIVES)
				ret |= NOT_HEADER | IN_SECTION;
			// we've determined the containment, anything else is just ignored for those purposes
			*bAarea = false;
		} else if (strcmp(s, "section") == 0) {
            ret = (nContainment &~ IN_PARAGRAPH) | IN_SECTION;
			// we've determined the containment, anything else is just ignored for those purposes
			*bAarea = false;
		} else if (strcmp(s, "end") == 0 && (nContainment & IN_DECLARATIVES)) {
            ret = IN_DIVISION | IN_DECLARATIVES | IN_SECTION | NOT_HEADER;
		} else {
			ret = nContainment | IN_PARAGRAPH;
        }
    }
    ColourTo(styler, end, chAttr);
    return ret;
}

static void ColouriseCOBOLDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
    Accessor &styler) {

    styler.StartAt(startPos);

    int state = initStyle;
    if (state == SCE_C_CHARACTER)   // Does not leak onto next line
        state = SCE_C_DEFAULT;
    char chPrev = ' ';
    char chNext = styler[startPos];
    Sci_PositionU lengthDoc = startPos + length;

    int nContainment;

    Sci_Position currentLine = styler.GetLine(startPos);
    if (currentLine > 0) {
        styler.SetLineState(currentLine, styler.GetLineState(currentLine-1));
        nContainment = styler.GetLineState(currentLine);
		nContainment &= ~NOT_HEADER;
    } else {
        styler.SetLineState(currentLine, 0);
        nContainment = 0;
    }

    styler.StartSegment(startPos);
    bool bNewLine = true;
    bool bAarea = !isspacechar(chNext);
	int column = 0;
    for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
        char ch = chNext;

        chNext = styler.SafeGetCharAt(i + 1);

		++column;

        if (bNewLine) {
			column = 0;
        }
		if (column <= 1 && !bAarea) {
			bAarea = !isspacechar(ch);
			}
        bool bSetNewLine = false;
        if ((ch == '\r' && chNext != '\n') || (ch == '\n')) {
            // Trigger on CR only (Mac style) or either on LF from CR+LF (Dos/Win) or on LF alone (Unix)
            // Avoid triggering two times on Dos/Win
            // End of line
            if (state == SCE_C_CHARACTER) {
                ColourTo(styler, i, state);
                state = SCE_C_DEFAULT;
            }
            styler.SetLineState(currentLine, nContainment);
            currentLine++;
            bSetNewLine = true;
			if (nContainment & NOT_HEADER)
				nContainment &= ~(NOT_HEADER | IN_DECLARATIVES | IN_SECTION);
        }

        if (styler.IsLeadByte(ch)) {
            chNext = styler.SafeGetCharAt(i + 2);
            chPrev = ' ';
            i += 1;
            continue;
        }

        if (state == SCE_C_DEFAULT) {
            if (isCOBOLwordstart(ch) || (ch == '$' && IsASCII(chNext) && isalpha(chNext))) {
                ColourTo(styler, i-1, state);
                state = SCE_C_IDENTIFIER;
            } else if (column == 6 && ch == '*') {
            // Cobol comment line: asterisk in column 7.
                ColourTo(styler, i-1, state);
                state = SCE_C_COMMENTLINE;
            } else if (ch == '*' && chNext == '>') {
            // Cobol inline comment: asterisk, followed by greater than.
                ColourTo(styler, i-1, state);
                state = SCE_C_COMMENTLINE;
            } else if (column == 0 && ch == '*' && chNext != '*') {
                ColourTo(styler, i-1, state);
                state = SCE_C_COMMENTLINE;
            } else if (column == 0 && ch == '/' && chNext != '*') {
                ColourTo(styler, i-1, state);
                state = SCE_C_COMMENTLINE;
            } else if (column == 0 && ch == '*' && chNext == '*') {
                ColourTo(styler, i-1, state);
                state = SCE_C_COMMENTDOC;
            } else if (column == 0 && ch == '/' && chNext == '*') {
                ColourTo(styler, i-1, state);
                state = SCE_C_COMMENTDOC;
            } else if (ch == '"') {
                ColourTo(styler, i-1, state);
                state = SCE_C_STRING;
            } else if (ch == '\'') {
                ColourTo(styler, i-1, state);
                state = SCE_C_CHARACTER;
            } else if (ch == '?' && column == 0) {
                ColourTo(styler, i-1, state);
                state = SCE_C_PREPROCESSOR;
            } else if (isCOBOLoperator(ch)) {
                ColourTo(styler, i-1, state);
                ColourTo(styler, i, SCE_C_OPERATOR);
            }
        } else if (state == SCE_C_IDENTIFIER) {
            if (!isCOBOLwordchar(ch)) {
                int lStateChange = classifyWordCOBOL(styler.GetStartSegment(), i - 1, keywordlists, styler, nContainment, &bAarea);

                if(lStateChange != 0) {
                    styler.SetLineState(currentLine, lStateChange);
                    nContainment = lStateChange;
                }

                state = SCE_C_DEFAULT;
                chNext = styler.SafeGetCharAt(i + 1);
                if (ch == '"') {
                    state = SCE_C_STRING;
                } else if (ch == '\'') {
                    state = SCE_C_CHARACTER;
                } else if (isCOBOLoperator(ch)) {
                    ColourTo(styler, i, SCE_C_OPERATOR);
                }
            }
        } else {
            if (state == SCE_C_PREPROCESSOR) {
                if ((ch == '\r' || ch == '\n') && !(chPrev == '\\' || chPrev == '\r')) {
                    ColourTo(styler, i-1, state);
                    state = SCE_C_DEFAULT;
                }
            } else if (state == SCE_C_COMMENT) {
                if (ch == '\r' || ch == '\n') {
                    ColourTo(styler, i, state);
                    state = SCE_C_DEFAULT;
                }
            } else if (state == SCE_C_COMMENTDOC) {
                if (ch == '\r' || ch == '\n') {
                    if (((i > styler.GetStartSegment() + 2) || (
                        (initStyle == SCE_C_COMMENTDOC) &&
                        (styler.GetStartSegment() == static_cast<Sci_PositionU>(startPos))))) {
                            ColourTo(styler, i, state);
                            state = SCE_C_DEFAULT;
                    }
                }
            } else if (state == SCE_C_COMMENTLINE) {
                if (ch == '\r' || ch == '\n') {
                    ColourTo(styler, i-1, state);
                    state = SCE_C_DEFAULT;
                }
            } else if (state == SCE_C_STRING) {
                if (ch == '"') {
                    ColourTo(styler, i, state);
                    state = SCE_C_DEFAULT;
                }
            } else if (state == SCE_C_CHARACTER) {
                if (ch == '\'') {
                    ColourTo(styler, i, state);
                    state = SCE_C_DEFAULT;
                }
            }
        }
        chPrev = ch;
        bNewLine = bSetNewLine;
		if (bNewLine)
			{
			bAarea = false;
			}
    }
    ColourTo(styler, lengthDoc - 1, state);
}

static void FoldCOBOLDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[],
                            Accessor &styler) {
    bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
    Sci_PositionU endPos = startPos + length;
    int visibleChars = 0;
    Sci_Position lineCurrent = styler.GetLine(startPos);
    int levelPrev = lineCurrent > 0 ? styler.LevelAt(lineCurrent - 1) & SC_FOLDLEVELNUMBERMASK : 0xFFF;
    char chNext = styler[startPos];

    bool bNewLine = true;
    bool bAarea = !isspacechar(chNext);
	int column = 0;
	bool bComment = false;
    for (Sci_PositionU i = startPos; i < endPos; i++) {
        char ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);
		++column;

        if (bNewLine) {
			column = 0;
			bComment = (ch == '*' || ch == '/' || ch == '?');
        }
		if (column <= 1 && !bAarea) {
			bAarea = !isspacechar(ch);
			}
        bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
        if (atEOL) {
			int nContainment = styler.GetLineState(lineCurrent);
            int lev = CountBits(nContainment & IN_FLAGS) | SC_FOLDLEVELBASE;
			if (bAarea && !bComment)
				--lev;
            if (visibleChars == 0 && foldCompact)
                lev |= SC_FOLDLEVELWHITEFLAG;
            if ((bAarea) && (visibleChars > 0) && !(nContainment & NOT_HEADER) && !bComment)
                lev |= SC_FOLDLEVELHEADERFLAG;
            if (lev != styler.LevelAt(lineCurrent)) {
                styler.SetLevel(lineCurrent, lev);
            }
			if ((lev & SC_FOLDLEVELNUMBERMASK) <= (levelPrev & SC_FOLDLEVELNUMBERMASK)) {
				// this level is at the same level or less than the previous line
				// therefore these is nothing for the previous header to collapse, so remove the header
				styler.SetLevel(lineCurrent - 1, levelPrev & ~SC_FOLDLEVELHEADERFLAG);
			}
            levelPrev = lev;
            visibleChars = 0;
			bAarea = false;
            bNewLine = true;
            lineCurrent++;
        } else {
            bNewLine = false;
        }


        if (!isspacechar(ch))
            visibleChars++;
    }

    // Fill in the real level of the next line, keeping the current flags as they will be filled in later
    int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
    styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const COBOLWordListDesc[] = {
    "A Keywords",
    "B Keywords",
    "Extended Keywords",
    0
};

LexerModule lmCOBOL(SCLEX_COBOL, ColouriseCOBOLDoc, "COBOL", FoldCOBOLDoc, COBOLWordListDesc);
