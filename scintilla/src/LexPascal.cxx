// Scintilla source code edit control
/** @file LexPascal.cxx
 ** Lexer for Pascal.
 ** Written by Laurent le Tynevez
 ** Updated by Simon Steele <s.steele@pnotepad.org> September 2002
 ** Updated by Mathias Rauen <scite@madshi.net> May 2003 (Delphi adjustments)
 **/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "StyleContext.h"

static void getRange(unsigned int start,
		unsigned int end,
		Accessor &styler,
		char *s,
		unsigned int len) {
	unsigned int i = 0;
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

static void ColourTo(Accessor &styler, unsigned int end, unsigned int attr, bool bInAsm) {
	if ((bInAsm) && (attr == SCE_C_OPERATOR || attr == SCE_C_NUMBER || attr == SCE_C_DEFAULT || attr == SCE_C_WORD || attr == SCE_C_IDENTIFIER)) {
		styler.ColourTo(end, SCE_C_REGEX);
	} else
		styler.ColourTo(end, attr);
}

// returns 1 if the item starts a class definition, and -1 if the word is "end", and 2 if the word is "asm"
static int classifyWordPascal(unsigned int start, unsigned int end, /*WordList &keywords*/WordList *keywordlists[], Accessor &styler, bool bInClass, bool bInAsm) {
	int ret = 0;

	WordList& keywords = *keywordlists[0];
	WordList& classwords = *keywordlists[1];

	char s[100];
	getRange(start, end, styler, s, sizeof(s));

	char chAttr = SCE_C_IDENTIFIER;
	if (isdigit(s[0]) || (s[0] == '.') ||(s[0] == '$')) {
		chAttr = SCE_C_NUMBER;
	}
	else {
		if (s[0] == '#') {
			chAttr = SCE_C_CHARACTER;
		}
		else {
			if (keywords.InList(s)) {
				chAttr = SCE_C_WORD;

				if(strcmp(s, "class") == 0) {
					ret = 1;
				}
				else if (strcmp(s, "asm") == 0) {
					ret = 2;
				}
				else if (strcmp(s, "end") == 0) {
					ret = -1;
				}
			} else if (bInClass) {
				if (classwords.InList(s)) {
					chAttr = SCE_C_WORD;
				}
			}
		}
	}
	ColourTo(styler, end, chAttr, (bInAsm && ret != -1));
	return ret;
}

static int classifyFoldPointPascal(const char* s) {
	int lev = 0;
	if (!(isdigit(s[0]) || (s[0] == '.'))) {
		if (strcmp(s, "begin") == 0 ||
			strcmp(s, "object") == 0 ||
			strcmp(s, "case") == 0 ||
			strcmp(s, "class") == 0 ||
			strcmp(s, "record") == 0 ||
			strcmp(s, "try") == 0) {
			lev=1;
		} else if (strcmp(s, "end") == 0) {
			lev=-1;
		}
	}
	return lev;
}

static void ColourisePascalDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
	Accessor &styler) {

	styler.StartAt(startPos);

	int state = initStyle;
	if (state == SCE_C_CHARACTER)	// Does not leak onto next line
		state = SCE_C_DEFAULT;
	char chPrev = ' ';
	char chNext = styler[startPos];
	unsigned int lengthDoc = startPos + length;

	bool bInClassDefinition;

	int currentLine = styler.GetLine(startPos);
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
	for (unsigned int i = startPos; i < lengthDoc; i++) {
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
			if (iswordstart(ch) || ch == '#' || ch == '$' || (ch == '@' && bInAsm)) {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_IDENTIFIER;
			} else if (ch == '{' && chNext != '$' && chNext != '&') {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_COMMENT;
			} else if (ch == '(' && chNext == '*'
						&& styler.SafeGetCharAt(i + 2) != '$'
						&& styler.SafeGetCharAt(i + 2) != '&') {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_COMMENTDOC;
			} else if (ch == '/' && chNext == '/') {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_COMMENTLINE;
			} else if (ch == '\'') {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_CHARACTER;
			} else if (ch == '{' && (chNext == '$' || chNext=='&')) {
				ColourTo(styler, i-1, state, bInAsm);
				state = SCE_C_PREPROCESSOR;
			} else if (isoperator(ch)) {
				ColourTo(styler, i-1, state, bInAsm);
				ColourTo(styler, i, SCE_C_OPERATOR, bInAsm);

			}
		} else if (state == SCE_C_IDENTIFIER) {
			bool bDoublePoint = ((ch == '.') && (chPrev == '.'));
			if ((!iswordchar(ch) && ch != '$' && ch != '#' && (ch != '@' || !bInAsm)) || bDoublePoint) {
				if (bDoublePoint) i--;
				int lStateChange = classifyWordPascal(styler.GetStartSegment(), i - 1, keywordlists, styler, bInClassDefinition, bInAsm);

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
				if (bDoublePoint) {
					i++;
					ColourTo(styler, i-1, SCE_C_DEFAULT, bInAsm);
				}

				state = SCE_C_DEFAULT;
				chNext = styler.SafeGetCharAt(i + 1);
				if (ch == '{' && chNext != '$' && chNext != '&') {
					state = SCE_C_COMMENT;
				} else if (ch == '(' && chNext == '*'
						&& styler.SafeGetCharAt(i + 2) != '$'
						&& styler.SafeGetCharAt(i + 2) != '&') {
					ColourTo(styler, i-1, state, bInAsm);
					state = SCE_C_COMMENTDOC;
				} else if (ch == '/' && chNext == '/') {
					state = SCE_C_COMMENTLINE;
				} else if (ch == '\'') {
					state = SCE_C_CHARACTER;
				} else if (isoperator(ch)) {
					ColourTo(styler, i, SCE_C_OPERATOR, bInAsm);
				}
			}
		} else {
			if (state == SCE_C_PREPROCESSOR) {
				if (ch=='}'){
					ColourTo(styler, i, state, bInAsm);
					state = SCE_C_DEFAULT;
				} else {
					if ((ch == '\r' || ch == '\n') && !(chPrev == '\\' || chPrev == '\r')) {
						ColourTo(styler, i-1, state, bInAsm);
						state = SCE_C_DEFAULT;
					}
				}
			} else if (state == SCE_C_COMMENT) {
				if (ch == '}' ) {
					ColourTo(styler, i, state, bInAsm);
					state = SCE_C_DEFAULT;
				}
			} else if (state == SCE_C_COMMENTDOC) {
				if (ch == ')' && chPrev == '*') {
					if (((i > styler.GetStartSegment() + 2) || (
						(initStyle == SCE_C_COMMENTDOC) &&
						(styler.GetStartSegment() == static_cast<unsigned int>(startPos))))) {
							ColourTo(styler, i, state, bInAsm);
							state = SCE_C_DEFAULT;
					}
				}
			} else if (state == SCE_C_COMMENTLINE) {
				if (ch == '\r' || ch == '\n') {
					ColourTo(styler, i-1, state, bInAsm);
					state = SCE_C_DEFAULT;
				}
			} else if (state == SCE_C_CHARACTER) {
				if (ch == '\'') {
					ColourTo(styler, i, state, bInAsm);
					state = SCE_C_DEFAULT;
				}
			}
		}
		chPrev = ch;
	}
	ColourTo(styler, lengthDoc - 1, state, bInAsm);
}

static void FoldPascalDoc(unsigned int startPos, int length, int initStyle, WordList *[],
                            Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;

	int lastStart = 0;

	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (stylePrev == SCE_C_DEFAULT && style == SCE_C_WORD)
		{
			// Store last word start point.
			lastStart = i;
		}

		if (stylePrev == SCE_C_WORD) {
			if(iswordchar(ch) && !iswordchar(chNext)) {
				char s[100];
				getRange(lastStart, i, styler, s, sizeof(s));
				levelCurrent += classifyFoldPointPascal(s);
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
				unsigned int j=i+2; // skip {$
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
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}

		if (!isspacechar(ch))
			visibleChars++;
	}

	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const pascalWordListDesc[] = {
	"Keywords",
	"Classwords",
	0
};

LexerModule lmPascal(SCLEX_PASCAL, ColourisePascalDoc, "pascal", FoldPascalDoc, pascalWordListDesc);
