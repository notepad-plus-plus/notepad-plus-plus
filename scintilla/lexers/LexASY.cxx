// Scintilla source code edit control
//Author: instanton (email: soft_share<at>126<dot>com)
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

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

static void ColouriseAsyDoc(unsigned int startPos, int length, int initStyle,
		WordList *keywordlists[], Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];

	CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
	CharacterSet setWord(CharacterSet::setAlphaNum, "._", 0x80, true);

	int visibleChars = 0;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			if (sc.state == SCE_ASY_STRING) {
				sc.SetState(SCE_ASY_STRING);
			}
			visibleChars = 0;
		}

		if (sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
//				continuationLine = true;
				continue;
			}
		}

		// Determine if the current state should terminate.
		switch (sc.state) {
			case SCE_ASY_OPERATOR:
				sc.SetState(SCE_ASY_DEFAULT);
				break;
			case SCE_ASY_NUMBER:
				if (!setWord.Contains(sc.ch)) {
					sc.SetState(SCE_ASY_DEFAULT);
				}
				break;
			case SCE_ASY_IDENTIFIER:
				if (!setWord.Contains(sc.ch) || (sc.ch == '.')) {
					char s[1000];
					sc.GetCurrentLowered(s, sizeof(s));
					if (keywords.InList(s)) {
						sc.ChangeState(SCE_ASY_WORD);
					} else if (keywords2.InList(s)) {
						sc.ChangeState(SCE_ASY_WORD2);
					}
					sc.SetState(SCE_ASY_DEFAULT);
				}
				break;
			case SCE_ASY_COMMENT:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_ASY_DEFAULT);
				}
				break;
			case SCE_ASY_COMMENTLINE:
				if (sc.atLineStart) {
					sc.SetState(SCE_ASY_DEFAULT);
				}
				break;
			case SCE_ASY_STRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_ASY_STRINGEOL);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_ASY_DEFAULT);
				}
				break;
			case SCE_ASY_CHARACTER:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_ASY_STRINGEOL);
				} else 	if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					sc.ForwardSetState(SCE_ASY_DEFAULT);
				}
				break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_ASY_DEFAULT) {
			if (setWordStart.Contains(sc.ch) || (sc.ch == '@')) {
				sc.SetState(SCE_ASY_IDENTIFIER);
			} else if (sc.Match('/', '*')) {
				sc.SetState(SCE_ASY_COMMENT);
				sc.Forward();	//
			} else if (sc.Match('/', '/')) {
				sc.SetState(SCE_ASY_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_ASY_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_ASY_CHARACTER);
			} else if (sc.ch == '#' && visibleChars == 0) {
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(SCE_ASY_DEFAULT);
				}
			} else if (isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_ASY_OPERATOR);
			}
		}

	}
	sc.Complete();
}

static bool IsAsyCommentStyle(int style) {
	return style == SCE_ASY_COMMENT;
}


static inline bool isASYidentifier(int ch) {
	return
      ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) ;
}

static int ParseASYWord(unsigned int pos, Accessor &styler, char *word)
{
  int length=0;
  char ch=styler.SafeGetCharAt(pos);
  *word=0;

  while(isASYidentifier(ch) && length<100){
          word[length]=ch;
          length++;
          ch=styler.SafeGetCharAt(pos+length);
  }
  word[length]=0;
  return length;
}

static bool IsASYDrawingLine(int line, Accessor &styler) {
	int pos = styler.LineStart(line);
	int eol_pos = styler.LineStart(line + 1) - 1;

	int startpos = pos;
	char buffer[100]="";

	while (startpos<eol_pos){
		char ch = styler[startpos];
		ParseASYWord(startpos,styler,buffer);
		bool drawcommands = strncmp(buffer,"draw",4)==0||
			strncmp(buffer,"pair",4)==0||strncmp(buffer,"label",5)==0;
		if (!drawcommands && ch!=' ') return false;
		else if (drawcommands) return true;
		startpos++;
	}
	return false;
}

static void FoldAsyDoc(unsigned int startPos, int length, int initStyle,
					   WordList *[], Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
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
	int style = initStyle;
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (foldComment && IsAsyCommentStyle(style)) {
			if (!IsAsyCommentStyle(stylePrev) && (stylePrev != SCE_ASY_COMMENTLINEDOC)) {
				levelNext++;
			} else if (!IsAsyCommentStyle(styleNext) && (styleNext != SCE_ASY_COMMENTLINEDOC) && !atEOL) {
				levelNext--;
			}
		}
		if (style == SCE_ASY_OPERATOR) {
			if (ch == '{') {
				if (levelMinCurrent > levelNext) {
					levelMinCurrent = levelNext;
				}
				levelNext++;
			} else if (ch == '}') {
				levelNext--;
			}
		}

		if (atEOL && IsASYDrawingLine(lineCurrent, styler)){
			if (lineCurrent==0 && IsASYDrawingLine(lineCurrent + 1, styler))
				levelNext++;
			else if (lineCurrent!=0 && !IsASYDrawingLine(lineCurrent - 1, styler)
				&& IsASYDrawingLine(lineCurrent + 1, styler)
				)
				levelNext++;
			else if (lineCurrent!=0 && IsASYDrawingLine(lineCurrent - 1, styler) &&
				!IsASYDrawingLine(lineCurrent+1, styler))
				levelNext--;
		}

		if (atEOL) {
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
		if (!IsASpace(ch))
			visibleChars++;
	}
}

static const char * const asyWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            0,
        };

LexerModule lmASY(SCLEX_ASYMPTOTE, ColouriseAsyDoc, "asy", FoldAsyDoc, asyWordLists);
