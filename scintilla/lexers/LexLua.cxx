// Scintilla source code edit control
/** @file LexLua.cxx
 ** Lexer for Lua language.
 **
 ** Written by Paul Winwood.
 ** Folder by Alexey Yutkin.
 ** Modified by Marcos E. Wurzius & Philippe Lhoste
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "StringCopy.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Scintilla;

// Test for [=[ ... ]=] delimiters, returns 0 if it's only a [ or ],
// return 1 for [[ or ]], returns >=2 for [=[ or ]=] and so on.
// The maximum number of '=' characters allowed is 254.
static int LongDelimCheck(StyleContext &sc) {
	int sep = 1;
	while (sc.GetRelative(sep) == '=' && sep < 0xFF)
		sep++;
	if (sc.GetRelative(sep) == sc.ch)
		return sep;
	return 0;
}

static void ColouriseLuaDoc(
	Sci_PositionU startPos,
	Sci_Position length,
	int initStyle,
	WordList *keywordlists[],
	Accessor &styler) {

	const WordList &keywords = *keywordlists[0];
	const WordList &keywords2 = *keywordlists[1];
	const WordList &keywords3 = *keywordlists[2];
	const WordList &keywords4 = *keywordlists[3];
	const WordList &keywords5 = *keywordlists[4];
	const WordList &keywords6 = *keywordlists[5];
	const WordList &keywords7 = *keywordlists[6];
	const WordList &keywords8 = *keywordlists[7];

	// Accepts accented characters
	CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, true);
	CharacterSet setWord(CharacterSet::setAlphaNum, "_", 0x80, true);
	// Not exactly following number definition (several dots are seen as OK, etc.)
	// but probably enough in most cases. [pP] is for hex floats.
	CharacterSet setNumber(CharacterSet::setDigits, ".-+abcdefpABCDEFP");
	CharacterSet setExponent(CharacterSet::setNone, "eEpP");
	CharacterSet setLuaOperator(CharacterSet::setNone, "*/-+()={}~[];<>,.^%:#&|");
	CharacterSet setEscapeSkip(CharacterSet::setNone, "\"'\\");

	Sci_Position currentLine = styler.GetLine(startPos);
	// Initialize long string [[ ... ]] or block comment --[[ ... ]] nesting level,
	// if we are inside such a string. Block comment was introduced in Lua 5.0,
	// blocks with separators [=[ ... ]=] in Lua 5.1.
	// Continuation of a string (\z whitespace escaping) is controlled by stringWs.
	int nestLevel = 0;
	int sepCount = 0;
	int stringWs = 0;
	if (initStyle == SCE_LUA_LITERALSTRING || initStyle == SCE_LUA_COMMENT ||
		initStyle == SCE_LUA_STRING || initStyle == SCE_LUA_CHARACTER) {
		const int lineState = styler.GetLineState(currentLine - 1);
		nestLevel = lineState >> 9;
		sepCount = lineState & 0xFF;
		stringWs = lineState & 0x100;
	}

	// results of identifier/keyword matching
	Sci_Position idenPos = 0;
	Sci_Position idenWordPos = 0;
	int idenStyle = SCE_LUA_IDENTIFIER;
	bool foundGoto = false;

	// Do not leak onto next line
	if (initStyle == SCE_LUA_STRINGEOL || initStyle == SCE_LUA_COMMENTLINE || initStyle == SCE_LUA_PREPROCESSOR) {
		initStyle = SCE_LUA_DEFAULT;
	}

	StyleContext sc(startPos, length, initStyle, styler);
	if (startPos == 0 && sc.ch == '#' && sc.chNext == '!') {
		// shbang line: "#!" is a comment only if located at the start of the script
		sc.SetState(SCE_LUA_COMMENTLINE);
	}
	for (; sc.More(); sc.Forward()) {
		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			currentLine = styler.GetLine(sc.currentPos);
			switch (sc.state) {
			case SCE_LUA_LITERALSTRING:
			case SCE_LUA_COMMENT:
			case SCE_LUA_STRING:
			case SCE_LUA_CHARACTER:
				// Inside a literal string, block comment or string, we set the line state
				styler.SetLineState(currentLine, (nestLevel << 9) | stringWs | sepCount);
				break;
			default:
				// Reset the line state
				styler.SetLineState(currentLine, 0);
				break;
			}
		}
		if (sc.atLineStart && (sc.state == SCE_LUA_STRING)) {
			// Prevent SCE_LUA_STRINGEOL from leaking back to previous line
			sc.SetState(SCE_LUA_STRING);
		}

		// Handle string line continuation
		if ((sc.state == SCE_LUA_STRING || sc.state == SCE_LUA_CHARACTER) &&
				sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_LUA_OPERATOR) {
			if (sc.ch == ':' && sc.chPrev == ':') {	// :: <label> :: forward scan
				sc.Forward();
				Sci_Position ln = 0;
				while (IsASpaceOrTab(sc.GetRelative(ln)))	// skip over spaces/tabs
					ln++;
				Sci_Position ws1 = ln;
				if (setWordStart.Contains(sc.GetRelative(ln))) {
					int c, i = 0;
					char s[100];
					while (setWord.Contains(c = sc.GetRelative(ln))) {	// get potential label
						if (i < 90)
							s[i++] = static_cast<char>(c);
						ln++;
					}
					s[i] = '\0'; Sci_Position lbl = ln;
					if (!keywords.InList(s)) {
						while (IsASpaceOrTab(sc.GetRelative(ln)))	// skip over spaces/tabs
							ln++;
						Sci_Position ws2 = ln - lbl;
						if (sc.GetRelative(ln) == ':' && sc.GetRelative(ln + 1) == ':') {
							// final :: found, complete valid label construct
							sc.ChangeState(SCE_LUA_LABEL);
							if (ws1) {
								sc.SetState(SCE_LUA_DEFAULT);
								sc.ForwardBytes(ws1);
							}
							sc.SetState(SCE_LUA_LABEL);
							sc.ForwardBytes(lbl - ws1);
							if (ws2) {
								sc.SetState(SCE_LUA_DEFAULT);
								sc.ForwardBytes(ws2);
							}
							sc.SetState(SCE_LUA_LABEL);
							sc.ForwardBytes(2);
						}
					}
				}
			}
			sc.SetState(SCE_LUA_DEFAULT);
		} else if (sc.state == SCE_LUA_NUMBER) {
			// We stop the number definition on non-numerical non-dot non-eEpP non-sign non-hexdigit char
			if (!setNumber.Contains(sc.ch)) {
				sc.SetState(SCE_LUA_DEFAULT);
			} else if (sc.ch == '-' || sc.ch == '+') {
				if (!setExponent.Contains(sc.chPrev))
					sc.SetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.state == SCE_LUA_IDENTIFIER) {
			idenPos--;			// commit already-scanned identitier/word parts
			if (idenWordPos > 0) {
				idenWordPos--;
				sc.ChangeState(idenStyle);
				sc.ForwardBytes(idenWordPos);
				idenPos -= idenWordPos;
				if (idenPos > 0) {
					sc.SetState(SCE_LUA_IDENTIFIER);
					sc.ForwardBytes(idenPos);
				}
			} else {
				sc.ForwardBytes(idenPos);
			}
			sc.SetState(SCE_LUA_DEFAULT);
			if (foundGoto) {					// goto <label> forward scan
				while (IsASpaceOrTab(sc.ch) && !sc.atLineEnd)
					sc.Forward();
				if (setWordStart.Contains(sc.ch)) {
					sc.SetState(SCE_LUA_LABEL);
					sc.Forward();
					while (setWord.Contains(sc.ch))
						sc.Forward();
					char s[100];
					sc.GetCurrent(s, sizeof(s));
					if (keywords.InList(s))		// labels cannot be keywords
						sc.ChangeState(SCE_LUA_WORD);
				}
				sc.SetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.state == SCE_LUA_COMMENTLINE || sc.state == SCE_LUA_PREPROCESSOR) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.state == SCE_LUA_STRING) {
			if (stringWs) {
				if (!IsASpace(sc.ch))
					stringWs = 0;
			}
			if (sc.ch == '\\') {
				if (setEscapeSkip.Contains(sc.chNext)) {
					sc.Forward();
				} else if (sc.chNext == 'z') {
					sc.Forward();
					stringWs = 0x100;
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			} else if (stringWs == 0 && sc.atLineEnd) {
				sc.ChangeState(SCE_LUA_STRINGEOL);
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.state == SCE_LUA_CHARACTER) {
			if (stringWs) {
				if (!IsASpace(sc.ch))
					stringWs = 0;
			}
			if (sc.ch == '\\') {
				if (setEscapeSkip.Contains(sc.chNext)) {
					sc.Forward();
				} else if (sc.chNext == 'z') {
					sc.Forward();
					stringWs = 0x100;
				}
			} else if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			} else if (stringWs == 0 && sc.atLineEnd) {
				sc.ChangeState(SCE_LUA_STRINGEOL);
				sc.ForwardSetState(SCE_LUA_DEFAULT);
			}
		} else if (sc.state == SCE_LUA_LITERALSTRING || sc.state == SCE_LUA_COMMENT) {
			if (sc.ch == '[') {
				const int sep = LongDelimCheck(sc);
				if (sep == 1 && sepCount == 1) {    // [[-only allowed to nest
					nestLevel++;
					sc.Forward();
				}
			} else if (sc.ch == ']') {
				int sep = LongDelimCheck(sc);
				if (sep == 1 && sepCount == 1) {    // un-nest with ]]-only
					nestLevel--;
					sc.Forward();
					if (nestLevel == 0) {
						sc.ForwardSetState(SCE_LUA_DEFAULT);
					}
				} else if (sep > 1 && sep == sepCount) {   // ]=]-style delim
					sc.Forward(sep);
					sc.ForwardSetState(SCE_LUA_DEFAULT);
				}
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_LUA_DEFAULT) {
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_LUA_NUMBER);
				if (sc.ch == '0' && toupper(sc.chNext) == 'X') {
					sc.Forward();
				}
			} else if (setWordStart.Contains(sc.ch)) {
				// For matching various identifiers with dots and colons, multiple
				// matches are done as identifier segments are added. Longest match is
				// set to a word style. The non-matched part is in identifier style.
				std::string ident;
				idenPos = 0;
				idenWordPos = 0;
				idenStyle = SCE_LUA_IDENTIFIER;
				foundGoto = false;
				int cNext;
				do {
					int c;
					const Sci_Position idenPosOld = idenPos;
					std::string identSeg;
					identSeg += static_cast<char>(sc.GetRelative(idenPos++));
					while (setWord.Contains(c = sc.GetRelative(idenPos))) {
						identSeg += static_cast<char>(c);
						idenPos++;
					}
					if (keywords.InList(identSeg.c_str()) && (idenPosOld > 0)) {
						idenPos = idenPosOld - 1;	// keywords cannot mix
						ident.pop_back();
						break;
					}
					ident += identSeg;
					const char* s = ident.c_str();
					int newStyle = SCE_LUA_IDENTIFIER;
					if (keywords.InList(s)) {
						newStyle = SCE_LUA_WORD;
					} else if (keywords2.InList(s)) {
						newStyle = SCE_LUA_WORD2;
					} else if (keywords3.InList(s)) {
						newStyle = SCE_LUA_WORD3;
					} else if (keywords4.InList(s)) {
						newStyle = SCE_LUA_WORD4;
					} else if (keywords5.InList(s)) {
						newStyle = SCE_LUA_WORD5;
					} else if (keywords6.InList(s)) {
						newStyle = SCE_LUA_WORD6;
					} else if (keywords7.InList(s)) {
						newStyle = SCE_LUA_WORD7;
					} else if (keywords8.InList(s)) {
						newStyle = SCE_LUA_WORD8;
					}
					if (newStyle != SCE_LUA_IDENTIFIER) {
						idenStyle = newStyle;
						idenWordPos = idenPos;
					}
					if (idenStyle == SCE_LUA_WORD)	// keywords cannot mix
						break;
					cNext = sc.GetRelative(idenPos + 1);
					if ((c == '.' || c == ':') && setWordStart.Contains(cNext)) {
						ident += static_cast<char>(c);
						idenPos++;
					} else {
						cNext = 0;
					}
				} while (cNext);
				if ((idenStyle == SCE_LUA_WORD) && (ident.compare("goto") == 0)) {
					foundGoto = true;
				}
				sc.SetState(SCE_LUA_IDENTIFIER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_LUA_STRING);
				stringWs = 0;
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_LUA_CHARACTER);
				stringWs = 0;
			} else if (sc.ch == '[') {
				sepCount = LongDelimCheck(sc);
				if (sepCount == 0) {
					sc.SetState(SCE_LUA_OPERATOR);
				} else {
					nestLevel = 1;
					sc.SetState(SCE_LUA_LITERALSTRING);
					sc.Forward(sepCount);
				}
			} else if (sc.Match('-', '-')) {
				sc.SetState(SCE_LUA_COMMENTLINE);
				if (sc.Match("--[")) {
					sc.Forward(2);
					sepCount = LongDelimCheck(sc);
					if (sepCount > 0) {
						nestLevel = 1;
						sc.ChangeState(SCE_LUA_COMMENT);
						sc.Forward(sepCount);
					}
				} else {
					sc.Forward();
				}
			} else if (sc.atLineStart && sc.Match('$')) {
				sc.SetState(SCE_LUA_PREPROCESSOR);	// Obsolete since Lua 4.0, but still in old code
			} else if (setLuaOperator.Contains(sc.ch)) {
				sc.SetState(SCE_LUA_OPERATOR);
			}
		}
	}

	sc.Complete();
}

static void FoldLuaDoc(Sci_PositionU startPos, Sci_Position length, int /* initStyle */, WordList *[],
                       Accessor &styler) {
	const Sci_PositionU lengthDoc = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	const bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	int styleNext = styler.StyleAt(startPos);

	for (Sci_PositionU i = startPos; i < lengthDoc; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		const int style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (style == SCE_LUA_WORD) {
			if (ch == 'i' || ch == 'd' || ch == 'f' || ch == 'e' || ch == 'r' || ch == 'u') {
				char s[10] = "";
				for (Sci_PositionU j = 0; j < 8; j++) {
					if (!iswordchar(styler[i + j])) {
						break;
					}
					s[j] = styler[i + j];
					s[j + 1] = '\0';
				}

				if ((strcmp(s, "if") == 0) || (strcmp(s, "do") == 0) || (strcmp(s, "function") == 0) || (strcmp(s, "repeat") == 0)) {
					levelCurrent++;
				}
				if ((strcmp(s, "end") == 0) || (strcmp(s, "elseif") == 0) || (strcmp(s, "until") == 0)) {
					levelCurrent--;
				}
			}
		} else if (style == SCE_LUA_OPERATOR) {
			if (ch == '{' || ch == '(') {
				levelCurrent++;
			} else if (ch == '}' || ch == ')') {
				levelCurrent--;
			}
		} else if (style == SCE_LUA_LITERALSTRING || style == SCE_LUA_COMMENT) {
			if (ch == '[') {
				levelCurrent++;
			} else if (ch == ']') {
				levelCurrent--;
			}
		}

		if (atEOL) {
			int lev = levelPrev;
			if (visibleChars == 0 && foldCompact) {
				lev |= SC_FOLDLEVELWHITEFLAG;
			}
			if ((levelCurrent > levelPrev) && (visibleChars > 0)) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch)) {
			visibleChars++;
		}
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later

	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const luaWordListDesc[] = {
	"Keywords",
	"Basic functions",
	"String, (table) & math functions",
	"(coroutines), I/O & system facilities",
	"user1",
	"user2",
	"user3",
	"user4",
	0
};

namespace {

LexicalClass lexicalClasses[] = {
	// Lexer Lua SCLEX_LUA SCE_LUA_:
	0, "SCE_LUA_DEFAULT", "default", "White space: Visible only in View Whitespace mode (or if it has a back colour)",
	1, "SCE_LUA_COMMENT", "comment", "Block comment (Lua 5.0)",
	2, "SCE_LUA_COMMENTLINE", "comment line", "Line comment",
	3, "SCE_LUA_COMMENTDOC", "comment documentation", "Doc comment -- Not used in Lua (yet?)",
	4, "SCE_LUA_NUMBER", "literal numeric", "Number",
	5, "SCE_LUA_WORD", "keyword", "Keyword",
	6, "SCE_LUA_STRING", "literal string", "(Double quoted) String",
	7, "SCE_LUA_CHARACTER", "literal string character", "Character (Single quoted string)",
	8, "SCE_LUA_LITERALSTRING", "literal string", "Literal string",
	9, "SCE_LUA_PREPROCESSOR", "preprocessor", "Preprocessor (obsolete in Lua 4.0 and up)",
	10, "SCE_LUA_OPERATOR", "operator", "Operators",
	11, "SCE_LUA_IDENTIFIER", "identifier", "Identifier (everything else...)",
	12, "SCE_LUA_STRINGEOL", "error literal string", "End of line where string is not closed",
	13, "SCE_LUA_WORD2", "identifier", "Other keywords",
	14, "SCE_LUA_WORD3", "identifier", "Other keywords",
	15, "SCE_LUA_WORD4", "identifier", "Other keywords",
	16, "SCE_LUA_WORD5", "identifier", "Other keywords",
	17, "SCE_LUA_WORD6", "identifier", "Other keywords",
	18, "SCE_LUA_WORD7", "identifier", "Other keywords",
	19, "SCE_LUA_WORD8", "identifier", "Other keywords",
	20, "SCE_LUA_LABEL", "label", "Labels",
};

}

LexerModule lmLua(SCLEX_LUA, ColouriseLuaDoc, "lua", FoldLuaDoc, luaWordListDesc, lexicalClasses, ELEMENTS(lexicalClasses));
