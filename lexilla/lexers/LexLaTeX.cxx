// Scintilla source code edit control
/** @file LexLaTeX.cxx
 ** Lexer for LaTeX2e.
  **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// Modified by G. HU in 2013. Added folding, syntax highting inside math environments, and changed some minor behaviors.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>
#include <vector>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "DefaultLexer.h"
#include "LexerBase.h"

using namespace Scintilla;
using namespace Lexilla;

using namespace std;

struct latexFoldSave {
	latexFoldSave() : structLev(0) {
		for (int i = 0; i < 8; ++i) openBegins[i] = 0;
	}
	latexFoldSave(const latexFoldSave &save) : structLev(save.structLev) {
		for (int i = 0; i < 8; ++i) openBegins[i] = save.openBegins[i];
	}
	latexFoldSave &operator=(const latexFoldSave &save) {
		if (this != &save) {
			structLev = save.structLev;
			for (int i = 0; i < 8; ++i) openBegins[i] = save.openBegins[i];
		}
		return *this;
	}
	int openBegins[8];
	Sci_Position structLev;
};

class LexerLaTeX : public LexerBase {
private:
	vector<int> modes;
	void setMode(Sci_Position line, int mode) {
		if (line >= static_cast<Sci_Position>(modes.size())) modes.resize(line + 1, 0);
		modes[line] = mode;
	}
	int getMode(Sci_Position line) {
		if (line >= 0 && line < static_cast<Sci_Position>(modes.size())) return modes[line];
		return 0;
	}
	void truncModes(Sci_Position numLines) {
		if (static_cast<Sci_Position>(modes.size()) > numLines * 2 + 256)
			modes.resize(numLines + 128);
	}

	vector<latexFoldSave> saves;
	void setSave(Sci_Position line, const latexFoldSave &save) {
		if (line >= static_cast<Sci_Position>(saves.size())) saves.resize(line + 1);
		saves[line] = save;
	}
	void getSave(Sci_Position line, latexFoldSave &save) {
		if (line >= 0 && line < static_cast<Sci_Position>(saves.size())) save = saves[line];
		else {
			save.structLev = 0;
			for (int i = 0; i < 8; ++i) save.openBegins[i] = 0;
		}
	}
	void truncSaves(Sci_Position numLines) {
		if (static_cast<Sci_Position>(saves.size()) > numLines * 2 + 256)
			saves.resize(numLines + 128);
	}
public:
	static ILexer5 *LexerFactoryLaTeX() {
		return new LexerLaTeX();
	}
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	// ILexer5 methods
	const char * SCI_METHOD GetName() override {
		return "latex";
	}
	int SCI_METHOD  GetIdentifier() override {
		return SCLEX_LATEX;
	}
};

static bool latexIsSpecial(int ch) {
	return (ch == '#') || (ch == '$') || (ch == '%') || (ch == '&') || (ch == '_') ||
		   (ch == '{') || (ch == '}') || (ch == ' ');
}

static bool latexIsBlank(int ch) {
	return (ch == ' ') || (ch == '\t');
}

static bool latexIsBlankAndNL(int ch) {
	return (ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n');
}

static bool latexIsLetter(int ch) {
	return IsASCII(ch) && isalpha(ch);
}

static bool latexIsTagValid(Sci_Position &i, Sci_Position l, Accessor &styler) {
	while (i < l) {
		if (styler.SafeGetCharAt(i) == '{') {
			while (i < l) {
				i++;
				if (styler.SafeGetCharAt(i) == '}') {
					return true;
				}	else if (!latexIsLetter(styler.SafeGetCharAt(i)) &&
                   styler.SafeGetCharAt(i)!='*') {
					return false;
				}
			}
		} else if (!latexIsBlank(styler.SafeGetCharAt(i))) {
			return false;
		}
		i++;
	}
	return false;
}

static bool latexNextNotBlankIs(Sci_Position i, Accessor &styler, char needle) {
  char ch;
	while (i < styler.Length()) {
    ch = styler.SafeGetCharAt(i);
		if (!latexIsBlankAndNL(ch) && ch != '*') {
      if (ch == needle)
        return true;
      else
        return false;
		}
		i++;
	}
	return false;
}

static bool latexLastWordIs(Sci_Position start, Accessor &styler, const char *needle) {
	Sci_PositionU i = 0;
	Sci_PositionU l = static_cast<Sci_PositionU>(strlen(needle));
	Sci_Position ini = start-l+1;
	char s[32];

	while (i < l && i < 31) {
		s[i] = styler.SafeGetCharAt(ini + i);
		i++;
	}
	s[i] = '\0';

	return (strcmp(s, needle) == 0);
}

static bool latexLastWordIsMathEnv(Sci_Position pos, Accessor &styler) {
	Sci_Position i, j;
	char s[32];
	const char *mathEnvs[] = { "align", "alignat", "flalign", "gather",
		"multiline", "displaymath", "eqnarray", "equation" };
	if (styler.SafeGetCharAt(pos) != '}') return false;
	for (i = pos - 1; i >= 0; --i) {
		if (styler.SafeGetCharAt(i) == '{') break;
		if (pos - i >= 20) return false;
	}
	if (i < 0 || i == pos - 1) return false;
	++i;
	for (j = 0; i + j < pos; ++j)
		s[j] = styler.SafeGetCharAt(i + j);
	s[j] = '\0';
	if (j == 0) return false;
	if (s[j - 1] == '*') s[--j] = '\0';
	for (i = 0; i < static_cast<int>(sizeof(mathEnvs) / sizeof(const char *)); ++i)
		if (strcmp(s, mathEnvs[i]) == 0) return true;
	return false;
}

static inline void latexStateReset(int &mode, int &state) {
	switch (mode) {
	case 1:     state = SCE_L_MATH; break;
	case 2:     state = SCE_L_MATH2; break;
	default:    state = SCE_L_DEFAULT; break;
	}
}

// There are cases not handled correctly, like $abcd\textrm{what is $x+y$}z+w$.
// But I think it's already good enough.
void SCI_METHOD LexerLaTeX::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	// startPos is assumed to be the first character of a line
	Accessor styler(pAccess, &props);
	styler.StartAt(startPos);
	int mode = getMode(styler.GetLine(startPos) - 1);
	int state = initStyle;
	if (state == SCE_L_ERROR || state == SCE_L_SHORTCMD || state == SCE_L_SPECIAL)   // should not happen
		latexStateReset(mode, state);

	char chNext = styler.SafeGetCharAt(startPos);
	char chVerbatimDelim = '\0';
	styler.StartSegment(startPos);
	Sci_Position lengthDoc = startPos + length;

	for (Sci_Position i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if (styler.IsLeadByte(ch)) {
			i++;
			chNext = styler.SafeGetCharAt(i + 1);
			continue;
		}

		if (ch == '\r' || ch == '\n')
			setMode(styler.GetLine(i), mode);

		switch (state) {
		case SCE_L_DEFAULT :
			switch (ch) {
			case '\\' :
				styler.ColourTo(i - 1, state);
				if (latexIsLetter(chNext)) {
					state = SCE_L_COMMAND;
				} else if (latexIsSpecial(chNext)) {
					styler.ColourTo(i + 1, SCE_L_SPECIAL);
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
				} else if (chNext == '\r' || chNext == '\n') {
					styler.ColourTo(i, SCE_L_ERROR);
				} else if (IsASCII(chNext)) {
					styler.ColourTo(i + 1, SCE_L_SHORTCMD);
					if (chNext == '(') {
						mode = 1;
						state = SCE_L_MATH;
					} else if (chNext == '[') {
						mode = 2;
						state = SCE_L_MATH2;
					}
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
				}
				break;
			case '$' :
				styler.ColourTo(i - 1, state);
				if (chNext == '$') {
					styler.ColourTo(i + 1, SCE_L_SHORTCMD);
					mode = 2;
					state = SCE_L_MATH2;
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
				} else {
					styler.ColourTo(i, SCE_L_SHORTCMD);
					mode = 1;
					state = SCE_L_MATH;
				}
				break;
			case '%' :
				styler.ColourTo(i - 1, state);
				state = SCE_L_COMMENT;
				break;
			}
			break;
		// These 3 will never be reached.
		case SCE_L_ERROR:
		case SCE_L_SPECIAL:
		case SCE_L_SHORTCMD:
			break;
		case SCE_L_COMMAND :
			if (!latexIsLetter(chNext)) {
				styler.ColourTo(i, state);
				if (latexNextNotBlankIs(i + 1, styler, '[' )) {
					state = SCE_L_CMDOPT;
				} else if (latexLastWordIs(i, styler, "\\begin")) {
					state = SCE_L_TAG;
				} else if (latexLastWordIs(i, styler, "\\end")) {
					state = SCE_L_TAG2;
				} else if (latexLastWordIs(i, styler, "\\verb") && chNext != '*' && chNext != ' ') {
					chVerbatimDelim = chNext;
					state = SCE_L_VERBATIM;
				} else {
					latexStateReset(mode, state);
				}
			}
			break;
		case SCE_L_CMDOPT :
			if (ch == ']') {
				styler.ColourTo(i, state);
				latexStateReset(mode, state);
			}
			break;
		case SCE_L_TAG :
			if (latexIsTagValid(i, lengthDoc, styler)) {
				styler.ColourTo(i, state);
				latexStateReset(mode, state);
				if (latexLastWordIs(i, styler, "{verbatim}")) {
					state = SCE_L_VERBATIM;
				} else if (latexLastWordIs(i, styler, "{lstlisting}")) {
					state = SCE_L_VERBATIM;
				} else if (latexLastWordIs(i, styler, "{comment}")) {
					state = SCE_L_COMMENT2;
				} else if (latexLastWordIs(i, styler, "{math}") && mode == 0) {
					mode = 1;
					state = SCE_L_MATH;
				} else if (latexLastWordIsMathEnv(i, styler) && mode == 0) {
					mode = 2;
					state = SCE_L_MATH2;
				}
			} else {
				styler.ColourTo(i, SCE_L_ERROR);
				latexStateReset(mode, state);
				ch = styler.SafeGetCharAt(i);
				if (ch == '\r' || ch == '\n') setMode(styler.GetLine(i), mode);
			}
			chNext = styler.SafeGetCharAt(i+1);
			break;
		case SCE_L_TAG2 :
			if (latexIsTagValid(i, lengthDoc, styler)) {
				styler.ColourTo(i, state);
				latexStateReset(mode, state);
			} else {
				styler.ColourTo(i, SCE_L_ERROR);
				latexStateReset(mode, state);
				ch = styler.SafeGetCharAt(i);
				if (ch == '\r' || ch == '\n') setMode(styler.GetLine(i), mode);
			}
			chNext = styler.SafeGetCharAt(i+1);
			break;
		case SCE_L_MATH :
			switch (ch) {
			case '\\' :
				styler.ColourTo(i - 1, state);
				if (latexIsLetter(chNext)) {
					Sci_Position match = i + 3;
					if (latexLastWordIs(match, styler, "\\end")) {
						match++;
						if (latexIsTagValid(match, lengthDoc, styler)) {
							if (latexLastWordIs(match, styler, "{math}"))
								mode = 0;
						}
					}
					state = SCE_L_COMMAND;
				} else if (latexIsSpecial(chNext)) {
					styler.ColourTo(i + 1, SCE_L_SPECIAL);
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
				} else if (chNext == '\r' || chNext == '\n') {
					styler.ColourTo(i, SCE_L_ERROR);
				} else if (IsASCII(chNext)) {
					if (chNext == ')') {
						mode = 0;
						state = SCE_L_DEFAULT;
					}
					styler.ColourTo(i + 1, SCE_L_SHORTCMD);
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
				}
				break;
			case '$' :
				styler.ColourTo(i - 1, state);
				styler.ColourTo(i, SCE_L_SHORTCMD);
				mode = 0;
				state = SCE_L_DEFAULT;
				break;
			case '%' :
				styler.ColourTo(i - 1, state);
				state = SCE_L_COMMENT;
				break;
			}
			break;
		case SCE_L_MATH2 :
			switch (ch) {
			case '\\' :
				styler.ColourTo(i - 1, state);
				if (latexIsLetter(chNext)) {
					Sci_Position match = i + 3;
					if (latexLastWordIs(match, styler, "\\end")) {
						match++;
						if (latexIsTagValid(match, lengthDoc, styler)) {
							if (latexLastWordIsMathEnv(match, styler))
								mode = 0;
						}
					}
					state = SCE_L_COMMAND;
				} else if (latexIsSpecial(chNext)) {
					styler.ColourTo(i + 1, SCE_L_SPECIAL);
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
				} else if (chNext == '\r' || chNext == '\n') {
					styler.ColourTo(i, SCE_L_ERROR);
				} else if (IsASCII(chNext)) {
					if (chNext == ']') {
						mode = 0;
						state = SCE_L_DEFAULT;
					}
					styler.ColourTo(i + 1, SCE_L_SHORTCMD);
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
				}
				break;
			case '$' :
				styler.ColourTo(i - 1, state);
				if (chNext == '$') {
					styler.ColourTo(i + 1, SCE_L_SHORTCMD);
					i++;
					chNext = styler.SafeGetCharAt(i + 1);
					mode = 0;
					state = SCE_L_DEFAULT;
				} else { // This may not be an error, e.g. \begin{equation}\text{$a$}\end{equation}
					styler.ColourTo(i, SCE_L_SHORTCMD);
				}
				break;
			case '%' :
				styler.ColourTo(i - 1, state);
				state = SCE_L_COMMENT;
				break;
			}
			break;
		case SCE_L_COMMENT :
			if (ch == '\r' || ch == '\n') {
				styler.ColourTo(i - 1, state);
				latexStateReset(mode, state);
			}
			break;
		case SCE_L_COMMENT2 :
			if (ch == '\\') {
				Sci_Position match = i + 3;
				if (latexLastWordIs(match, styler, "\\end")) {
					match++;
					if (latexIsTagValid(match, lengthDoc, styler)) {
						if (latexLastWordIs(match, styler, "{comment}")) {
							styler.ColourTo(i - 1, state);
							state = SCE_L_COMMAND;
						}
					}
				}
			}
			break;
		case SCE_L_VERBATIM :
			if (ch == '\\') {
				Sci_Position match = i + 3;
				if (latexLastWordIs(match, styler, "\\end")) {
					match++;
					if (latexIsTagValid(match, lengthDoc, styler)) {
						if (latexLastWordIs(match, styler, "{verbatim}")) {
							styler.ColourTo(i - 1, state);
							state = SCE_L_COMMAND;
						} else if (latexLastWordIs(match, styler, "{lstlisting}")) {
							styler.ColourTo(i - 1, state);
							state = SCE_L_COMMAND;
						}
					}
				}
			} else if (chNext == chVerbatimDelim) {
				styler.ColourTo(i + 1, state);
				latexStateReset(mode, state);
				chVerbatimDelim = '\0';
				i++;
				chNext = styler.SafeGetCharAt(i + 1);
			} else if (chVerbatimDelim != '\0' && (ch == '\n' || ch == '\r')) {
				styler.ColourTo(i, SCE_L_ERROR);
				latexStateReset(mode, state);
				chVerbatimDelim = '\0';
			}
			break;
		}
	}
	if (lengthDoc == styler.Length()) truncModes(styler.GetLine(lengthDoc - 1));
	styler.ColourTo(lengthDoc - 1, state);
	styler.Flush();
}

static int latexFoldSaveToInt(const latexFoldSave &save) {
	int sum = 0;
	for (int i = 0; i <= save.structLev; ++i)
		sum += save.openBegins[i];
	return ((sum + save.structLev + SC_FOLDLEVELBASE) & SC_FOLDLEVELNUMBERMASK);
}

// Change folding state while processing a line
// Return the level before the first relevant command
void SCI_METHOD LexerLaTeX::Fold(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess) {
	const char *structWords[7] = {"part", "chapter", "section", "subsection",
		"subsubsection", "paragraph", "subparagraph"};
	Accessor styler(pAccess, &props);
	Sci_PositionU endPos = startPos + length;
	Sci_Position curLine = styler.GetLine(startPos);
	latexFoldSave save;
	getSave(curLine - 1, save);
	do {
		char ch, buf[16];
		Sci_Position i, j;
		int lev = -1;
		bool needFold = false;
		for (i = static_cast<Sci_Position>(startPos); i < static_cast<Sci_Position>(endPos); ++i) {
			ch = styler.SafeGetCharAt(i);
			if (ch == '\r' || ch == '\n') break;
			if (ch != '\\' || styler.StyleAt(i) != SCE_L_COMMAND) continue;
			for (j = 0; j < 15 && i + 1 < static_cast<Sci_Position>(endPos); ++j, ++i) {
				buf[j] = styler.SafeGetCharAt(i + 1);
				if (!latexIsLetter(buf[j])) break;
			}
			buf[j] = '\0';
			if (strcmp(buf, "begin") == 0) {
				if (lev < 0) lev = latexFoldSaveToInt(save);
				++save.openBegins[save.structLev];
				needFold = true;
			}
			else if (strcmp(buf, "end") == 0) {
				while (save.structLev > 0 && save.openBegins[save.structLev] == 0)
					--save.structLev;
				if (lev < 0) lev = latexFoldSaveToInt(save);
				if (save.openBegins[save.structLev] > 0) --save.openBegins[save.structLev];
			}
			else {
				for (j = 0; j < 7; ++j)
					if (strcmp(buf, structWords[j]) == 0) break;
				if (j >= 7) continue;
				save.structLev = j;   // level before the command
				for (j = save.structLev + 1; j < 8; ++j) {
					save.openBegins[save.structLev] += save.openBegins[j];
					save.openBegins[j] = 0;
				}
				if (lev < 0) lev = latexFoldSaveToInt(save);
				++save.structLev;   // level after the command
				needFold = true;
			}
		}
		if (lev < 0) lev = latexFoldSaveToInt(save);
		if (needFold) lev |= SC_FOLDLEVELHEADERFLAG;
		styler.SetLevel(curLine, lev);
		setSave(curLine, save);
		++curLine;
		startPos = styler.LineStart(curLine);
		if (static_cast<Sci_Position>(startPos) == styler.Length()) {
			lev = latexFoldSaveToInt(save);
			styler.SetLevel(curLine, lev);
			setSave(curLine, save);
			truncSaves(curLine);
		}
	} while (startPos < endPos);
	styler.Flush();
}

static const char *const emptyWordListDesc[] = {
	0
};

LexerModule lmLatex(SCLEX_LATEX, LexerLaTeX::LexerFactoryLaTeX, "latex", emptyWordListDesc);
