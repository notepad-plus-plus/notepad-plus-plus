// Scintilla source code edit control
/** @file LexVerilog.cxx
 ** Lexer for Verilog.
 ** Written by Avi Yegudin, based on C++ lexer by Neil Hodgson
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
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

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_' || ch == '\'');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '$');
}

static void ColouriseVerilogDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];

	// Do not leak onto next line
	if (initStyle == SCE_V_STRINGEOL)
		initStyle = SCE_V_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart && (sc.state == SCE_V_STRING)) {
			// Prevent SCE_V_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_V_STRING);
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_V_OPERATOR) {
			sc.SetState(SCE_V_DEFAULT);
		} else if (sc.state == SCE_V_NUMBER) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_V_DEFAULT);
			}
		} else if (sc.state == SCE_V_IDENTIFIER) {
			if (!IsAWordChar(sc.ch) || (sc.ch == '.')) {
				char s[100];
                                sc.GetCurrent(s, sizeof(s));
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_V_WORD);
				} else if (keywords2.InList(s)) {
					sc.ChangeState(SCE_V_WORD2);
				} else if (keywords3.InList(s)) {
					sc.ChangeState(SCE_V_WORD3);
                                } else if (keywords4.InList(s)) {
					sc.ChangeState(SCE_V_USER);
				}
				sc.SetState(SCE_V_DEFAULT);
			}
		} else if (sc.state == SCE_V_PREPROCESSOR) {
                        if (!IsAWordChar(sc.ch)) {
                                sc.SetState(SCE_V_DEFAULT);
                        }
		} else if (sc.state == SCE_V_COMMENT) {
			if (sc.Match('*', '/')) {
				sc.Forward();
				sc.ForwardSetState(SCE_V_DEFAULT);
			}
		} else if (sc.state == SCE_V_COMMENTLINE || sc.state == SCE_V_COMMENTLINEBANG) {
			if (sc.atLineStart) {
				sc.SetState(SCE_V_DEFAULT);
			}
                } else if (sc.state == SCE_V_STRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_V_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_V_STRINGEOL);
				sc.ForwardSetState(SCE_V_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_V_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '\'') || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_V_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_V_IDENTIFIER);
			} else if (sc.Match('/', '*')) {
                                sc.SetState(SCE_V_COMMENT);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				if (sc.Match("//!"))	// Nice to have a different comment style
					sc.SetState(SCE_V_COMMENTLINEBANG);
				else
					sc.SetState(SCE_V_COMMENTLINE);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_V_STRING);
			} else if (sc.ch == '`') {
				sc.SetState(SCE_V_PREPROCESSOR);
				// Skip whitespace between ` and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(SCE_V_DEFAULT);
				}
			} else if (isoperator(static_cast<char>(sc.ch)) || sc.ch == '@' || sc.ch == '#') {
				sc.SetState(SCE_V_OPERATOR);
			}
		}
	}
	sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_V_COMMENT;
}

static bool IsCommentLine(int line, Accessor &styler) {
	int pos = styler.LineStart(line);
	int eolPos = styler.LineStart(line + 1) - 1;
	for (int i = pos; i < eolPos; i++) {
		char ch = styler[i];
		char chNext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i);
		if (ch == '/' && chNext == '/' &&
		   (style == SCE_V_COMMENTLINE || style == SCE_V_COMMENTLINEBANG)) {
			return true;
		} else if (!IsASpaceOrTab(ch)) {
			return false;
		}
	}
	return false;
}
// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".
static void FoldNoBoxVerilogDoc(unsigned int startPos, int length, int initStyle,
                            Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) != 0;
        // Verilog specific folding options:
        // fold_at_module -
        //      Generally used methodology in verilog code is
        //      one module per file, so folding at module definition is useless.
        // fold_at_brace/parenthese -
        //      Folding of long port lists can be convenient.
	bool foldAtModule = styler.GetPropertyInt("fold.verilog.flags", 0) != 0;
	bool foldAtBrace  = 1;
	bool foldAtParenthese  = 1;

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
		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (foldComment && atEOL && IsCommentLine(lineCurrent, styler))
		{
			if (!IsCommentLine(lineCurrent - 1, styler)
			    && IsCommentLine(lineCurrent + 1, styler))
				levelNext++;
			else if (IsCommentLine(lineCurrent - 1, styler)
			         && !IsCommentLine(lineCurrent+1, styler))
				levelNext--;
		}
		if (foldComment && (style == SCE_V_COMMENTLINE)) {
			if ((ch == '/') && (chNext == '/')) {
				char chNext2 = styler.SafeGetCharAt(i + 2);
				if (chNext2 == '{') {
					levelNext++;
				} else if (chNext2 == '}') {
					levelNext--;
				}
			}
		}
		if (foldPreprocessor && (style == SCE_V_PREPROCESSOR)) {
			if (ch == '`') {
				unsigned int j = i + 1;
				while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
					j++;
				}
				if (styler.Match(j, "if")) {
					levelNext++;
				} else if (styler.Match(j, "end")) {
					levelNext--;
				}
			}
		}
                if (style == SCE_V_OPERATOR) {
                    if (foldAtParenthese) {
			if (ch == '(') {
				levelNext++;
			} else if (ch == ')') {
				levelNext--;
			}
                    }
		}
                if (style == SCE_V_OPERATOR) {
                    if (foldAtBrace) {
			if (ch == '{') {
				levelNext++;
			} else if (ch == '}') {
				levelNext--;
			}
                    }
		}
                if (style == SCE_V_WORD && stylePrev != SCE_V_WORD) {
                        unsigned int j = i;
                        if (styler.Match(j, "case") ||
                            styler.Match(j, "casex") ||
                            styler.Match(j, "casez") ||
                            styler.Match(j, "function") ||
                            styler.Match(j, "fork") ||
                            styler.Match(j, "table") ||
                            styler.Match(j, "task") ||
                            styler.Match(j, "generate") ||
                            styler.Match(j, "specify") ||
                            styler.Match(j, "primitive") ||
                            (styler.Match(j, "module") && foldAtModule) ||
                            styler.Match(j, "begin")) {
                                levelNext++;
                        } else if (styler.Match(j, "endcase") ||
                                   styler.Match(j, "endfunction") ||
                                   styler.Match(j, "join") ||
                                   styler.Match(j, "endtask") ||
                                   styler.Match(j, "endgenerate") ||
                                   styler.Match(j, "endtable") ||
                                   styler.Match(j, "endspecify") ||
                                   styler.Match(j, "endprimitive") ||
                                   (styler.Match(j, "endmodule") && foldAtModule) ||
                                   (styler.Match(j, "end") && !IsAWordChar(styler.SafeGetCharAt(j+3)))) {
                                levelNext--;
                        }
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
		if (!isspacechar(ch))
			visibleChars++;
	}
}

static void FoldVerilogDoc(unsigned int startPos, int length, int initStyle, WordList *[],
                       Accessor &styler) {
	FoldNoBoxVerilogDoc(startPos, length, initStyle, styler);
}

static const char * const verilogWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "System Tasks",
            "User defined tasks and identifiers",
            "Unused",
            0,
        };


LexerModule lmVerilog(SCLEX_VERILOG, ColouriseVerilogDoc, "verilog", FoldVerilogDoc, verilogWordLists);
