// Scintilla source code edit control
/** @file LexCPP.cxx
 ** Lexer for C++, C, Java, Javascript, Resource File and Objective-C
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
// Modified by Don <don.h@free.fr> 2004 to add lexer Object C

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "WordList.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#include "Scintilla.h"
#include "SciLexer.h"

using namespace Lexilla;

constexpr auto KEYWORD_BOXHEADER = 1;

static bool IsOKBeforeRE(const int ch) {
	return (ch == '(') || (ch == '=') || (ch == ',');
}

static inline bool IsAWordChar(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
}

static inline bool IsAWordStart(const int ch) {
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

inline bool IsASpace(unsigned int ch) {
    return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

static inline bool IsADoxygenChar(const int ch) {
	return (islower(ch) || ch == '$' || ch == '@' ||
	        ch == '\\' || ch == '&' || ch == '<' ||
	        ch == '>' || ch == '#' || ch == '{' ||
	        ch == '}' || ch == '[' || ch == ']');
}

static void ColouriseObjCDoc(size_t startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler, bool caseSensitive) {

	WordList &mainInstrsList = *keywordlists[0]; //Commun Instriction
	WordList &mainTypesList = *keywordlists[1]; //Commun Type
	WordList &DoxygenList = *keywordlists[2]; //Doxygen keyword
	WordList &objcDirectiveList = *keywordlists[3]; // objC Directive
	WordList &objcQualifierList = *keywordlists[4]; //objC Qualifier

	bool stylingWithinPreprocessor = styler.GetPropertyInt("styling.within.preprocessor") != 0;

	// Do not leak onto next line
	if (initStyle == SCE_C_STRINGEOL)
		initStyle = SCE_C_DEFAULT;

	int chPrevNonWhite = ' ';
	int visibleChars = 0;
	bool lastWordWasUUID = false;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) 
	{
		if (sc.atLineStart && (sc.state == SCE_C_STRING)) 
		{
			// Prevent SCE_C_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_C_STRING);
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') 
		{
			if (sc.chNext == '\n' || sc.chNext == '\r') 
			{
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') 
				{
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate.
		switch (sc.state)
		{
			case SCE_C_OPERATOR :
			{
				sc.SetState(SCE_C_DEFAULT);
				break;
			} 
			case SCE_C_NUMBER :
			{
				if (!IsAWordChar(sc.ch)) 
					sc.SetState(SCE_C_DEFAULT);
				break;
			} 
			case SCE_C_IDENTIFIER : 
			{
				if (!IsAWordChar(sc.ch) || (sc.ch == '.')) 
				{
					char s[100];
					sc.GetCurrent(s, sizeof(s));
					if (s[0] == '@')
					{
						char *ps = s + 1;
						if (objcDirectiveList.InList(ps))
							sc.ChangeState(SCE_OBJC_DIRECTIVE);
					}
					else 
					{
						if (mainInstrsList.InList(s)) 
						{
							lastWordWasUUID = strcmp(s, "uuid") == 0;
							sc.ChangeState(SCE_C_WORD);
						} 
						else if (mainTypesList.InList(s)) 
						{
							sc.ChangeState(SCE_C_WORD2);
						}
						else if (objcQualifierList.InList(s)) 
						{
							sc.ChangeState(SCE_OBJC_QUALIFIER);
						}
					}
					sc.SetState(SCE_C_DEFAULT);
				}
				break;
			} 
			case SCE_C_PREPROCESSOR :
			{
				if (stylingWithinPreprocessor) 
				{
					if (IsASpace(sc.ch)) 
						sc.SetState(SCE_C_DEFAULT);
				} 
				else 
				{
					if ((sc.atLineEnd) || (sc.Match('/', '*')) || (sc.Match('/', '/'))) 
						sc.SetState(SCE_C_DEFAULT);
				}
				break;
			} 
			case SCE_C_COMMENT :
			{
				if (sc.Match('*', '/')) 
				{
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT);
				}
				break;
			} 
			case SCE_C_COMMENTDOC :
			{
				if (sc.Match('*', '/')) 
				{
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT);
				} 
				else if (sc.ch == '@' || sc.ch == '\\') 
				{
					sc.SetState(SCE_C_COMMENTDOCKEYWORD);
				}
				break;
			} 
			case SCE_C_COMMENTLINE :
			case SCE_C_COMMENTLINEDOC :
			{
				if (sc.atLineEnd) 
				{
					sc.SetState(SCE_C_DEFAULT);
					visibleChars = 0;
				}
				break;
			} 
			case SCE_C_COMMENTDOCKEYWORD :
			{
				if (sc.Match('*', '/')) 
				{
					sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT);
				} 
				else if (!IsADoxygenChar(sc.ch)) 
				{
					char s[100];
					if (caseSensitive) 
						sc.GetCurrent(s, sizeof(s));
					else 
						sc.GetCurrentLowered(s, sizeof(s));

					if (!isspace(sc.ch) || !DoxygenList.InList(s + 1)) 
						sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);

					sc.SetState(SCE_C_COMMENTDOC);
				}
				break;
			}
			case SCE_C_STRING :
			{
				if (sc.ch == '\\') 
				{
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\')
						sc.Forward();
				} 
				else if (sc.ch == '\"') 
				{
					sc.ForwardSetState(SCE_C_DEFAULT);
				} 
				else if (sc.atLineEnd) 
				{
					sc.ChangeState(SCE_C_STRINGEOL);
					sc.ForwardSetState(SCE_C_DEFAULT);
					visibleChars = 0;
				}
				break;
			} 
			case SCE_C_CHARACTER :
			{
				if (sc.atLineEnd) 
				{
					sc.ChangeState(SCE_C_STRINGEOL);
					sc.ForwardSetState(SCE_C_DEFAULT);
					visibleChars = 0;
				} 
				else if (sc.ch == '\\') 
				{
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') 
					{
						sc.Forward();
					}
				} 
				else if (sc.ch == '\'') 
				{
					sc.ForwardSetState(SCE_C_DEFAULT);
				}
				break;
			} 
			case SCE_C_REGEX :
			{
				if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == '/') 
				{
					sc.ForwardSetState(SCE_C_DEFAULT);
				} 
				else if (sc.ch == '\\') 
				{
					// Gobble up the quoted character
					if (sc.chNext == '\\' || sc.chNext == '/') 
					{
						sc.Forward();
					}
				}
				break;
			} 
			case SCE_C_VERBATIM :
			{
				if (sc.ch == '\"') 
				{
					if (sc.chNext == '\"') 
						sc.Forward();
					else 
						sc.ForwardSetState(SCE_C_DEFAULT);
				}
				break;
			} 
			case SCE_C_UUID :
			{
				if (sc.ch == '\r' || sc.ch == '\n' || sc.ch == ')') 
					sc.SetState(SCE_C_DEFAULT);
				break;
			}
			default :
				break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_C_DEFAULT) 
		{
			if (sc.Match('@', '\"')) 
			{
				sc.SetState(SCE_C_VERBATIM);
				sc.Forward();
			}
			else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) 
			{
				if (lastWordWasUUID) 
				{
					sc.SetState(SCE_C_UUID);
					lastWordWasUUID = false;
				} 
				else 
				{
					sc.SetState(SCE_C_NUMBER);
				}
			} 
			else if (IsAWordStart(sc.ch) || (sc.ch == '@')) 
			{
				if (lastWordWasUUID) 
				{
					sc.SetState(SCE_C_UUID);
					lastWordWasUUID = false;
				} 
				else 
				{
					sc.SetState(SCE_C_IDENTIFIER);
				}
			} 
			else if (sc.Match('/', '*')) 
			{
				if (sc.Match("/**") || sc.Match("/*!")) 
					// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_C_COMMENTDOC);
				else 
					sc.SetState(SCE_C_COMMENT);
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} 
			else if (sc.Match('/', '/')) 
			{
				if (sc.Match("///") || sc.Match("//!"))	// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_C_COMMENTLINEDOC);
				else
					sc.SetState(SCE_C_COMMENTLINE);
			} 
			else if (sc.ch == '/' && IsOKBeforeRE(chPrevNonWhite)) 
			{
				sc.SetState(SCE_C_REGEX);
			} 
			else if (sc.ch == '\"') 
			{
				sc.SetState(SCE_C_STRING);
			} 
			else if (sc.ch == '\'') 
			{
				sc.SetState(SCE_C_CHARACTER);
			} 
			else if (sc.ch == '#' && visibleChars == 0) 
			{
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_C_PREPROCESSOR);
				// Skip whitespace between # and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) 
					sc.SetState(SCE_C_DEFAULT);
			}
			else if (isoperator(static_cast<char>(sc.ch)))
			{
				sc.SetState(SCE_C_OPERATOR);
			}
		}

		if (sc.atLineEnd) {
			// Reset states to begining of colourise so no surprises
			// if different sets of lines lexed.
			chPrevNonWhite = ' ';
			visibleChars = 0;
			lastWordWasUUID = false;
		}
		if (!IsASpace(sc.ch)) {
			chPrevNonWhite = sc.ch;
			visibleChars++;
		}
	}
	sc.Complete();
}

static bool IsStreamCommentStyle(int style) {
	return style == SCE_C_COMMENT ||
	       style == SCE_C_COMMENTDOC ||
	       style == SCE_C_COMMENTDOCKEYWORD ||
	       style == SCE_C_COMMENTDOCKEYWORDERROR;
}

static bool matchKeyword(size_t start, WordList &keywords, Accessor &styler, int keywordtype) {
	bool FoundKeyword = false;

	for (unsigned int i = 0;
			strlen(keywords.WordAt(i)) > 0 && !FoundKeyword;
	        i++) {
		if (atoi(keywords.WordAt(i)) == keywordtype) {
			FoundKeyword = styler.Match(start, ((char *)keywords.WordAt(i)) + 2);
		}
	}
	return FoundKeyword;
}

static bool IsCommentLine(int line, Accessor &styler) {
	unsigned int Pos = styler.LineStart(line);
	while (styler.GetLine(Pos) == line) {
		int PosStyle = styler.StyleAt(Pos);

		if (	!IsStreamCommentStyle(PosStyle)
		        &&
		        PosStyle != SCE_C_COMMENTLINEDOC
		        &&
		        PosStyle != SCE_C_COMMENTLINE
		        &&
		        !IsASpace(styler.SafeGetCharAt(Pos))
		   )
			return false;
		Pos++;
	}

	return true;
}


static void FoldObjCDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
                       Accessor &styler) {
	WordList &keywords4 = *keywordlists[3];

	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool firstLine = true;
	size_t endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	int levelPrevPrev;
	int levelUnindent = 0;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;

	if (lineCurrent == 0) {
		levelPrevPrev = levelPrev;
	} else {
		levelPrevPrev = styler.LevelAt(static_cast<Sci_Position>(lineCurrent) - 1) & SC_FOLDLEVELNUMBERMASK;
	}

	for (size_t i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);

		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (foldComment && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelCurrent++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelCurrent--;
			}
		}
		if (foldPreprocessor && (style == SCE_C_PREPROCESSOR)) {
			if (ch == '#') {
				size_t j = i + 1;
				while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
					j++;
				}

				if (styler.Match(j, "region") || styler.Match(j, "if")) {
					levelCurrent++;
				} else if (styler.Match(j, "end")) {
					levelCurrent--;
				}
			}
		}

		if (style == SCE_C_OPERATOR
		        ||
		        style == SCE_C_COMMENT
		        ||
		        style == SCE_C_COMMENTLINE) {

			if (ch == '{') {
				levelCurrent++;
				// Special handling if line has closing brace followed by opening brace.
				if (levelCurrent == levelPrev) {
					if (firstLine)
						levelUnindent = 1;
					else
						levelUnindent = -1;
				}
			} else if (ch == '}') {
				levelCurrent--;
			}
		}

		if (style == SCE_OBJC_DIRECTIVE)
		{
			if (ch == '@') 
			{
				size_t j = i + 1;
				if (styler.Match(j, "interface") || styler.Match(j, "implementation") || styler.Match(j, "protocol")) 
				{
					levelCurrent++;
				} 
				else if (styler.Match(j, "end")) 
				{
					levelCurrent--;
				}
			}
		}
		
		/* Check for fold header keyword at beginning of word */
		if ((style == SCE_C_WORD || style == SCE_C_COMMENT || style == SCE_C_COMMENTLINE)
		 && (style != stylePrev)) {
			if (matchKeyword(i, keywords4, styler, KEYWORD_BOXHEADER)) {
				int line;
				// Loop backwards all empty or comment lines
				for (line = lineCurrent - 1;
				        line >= 0
				        &&
				        levelCurrent == (styler.LevelAt(line) & SC_FOLDLEVELNUMBERMASK)
				        &&
				        IsCommentLine(line, styler);
				        line--) {
					// just loop backwards;
				}
				line++;
                if (line == lineCurrent) {
                    // in current line
                } else {
                    // at top of all preceding comment lines
                    styler.SetLevel(line, styler.LevelAt(line));
                }

			}
		}

		if (atEOL) {
			int lev;
			// Compute level correction for special case: '} else {'
			if (levelUnindent < 0) {
				levelPrev += levelUnindent;
			} else {
				levelCurrent += levelUnindent;
			}

			lev = levelPrev;
			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;

			// Produce footer line at line before (special handling for '} else {'
			if (levelPrev < levelPrevPrev) {
				styler.SetLevel(static_cast<Sci_Position>(lineCurrent) - 1, styler.LevelAt(static_cast<Sci_Position>(lineCurrent) - 1));
			}
			// Mark the fold header (the line that is always visible)
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;

			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}

			lineCurrent++;
			levelPrevPrev = levelPrev;
			levelPrev = levelCurrent;
			levelUnindent = 0;
			visibleChars = 0;
			firstLine = false;
		}

		if (!isspacechar(ch))
			visibleChars++;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const cppWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "Documentation comment keywords",
            "Fold header keywords",
            0,
        };

static void ColouriseObjCDocSensitive(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[],
                                     Accessor &styler) {
	ColouriseObjCDoc(startPos, length, initStyle, keywordlists, styler, true);
}

extern const LexerModule lmObjC(SCLEX_OBJC, ColouriseObjCDocSensitive, "objc", FoldObjCDoc, cppWordLists);
