// Scintilla source code edit control
/** @file LexOScript.cxx
 ** Lexer for OScript sources; ocx files and/or OSpace dumps.
 ** OScript is a programming language used to develop applications for the
 ** Livelink server platform.
 **/
// Written by Ferdinand Prantl <prantlf@gmail.com>, inspired by the code from
// LexVB.cxx and LexPascal.cxx. The License.txt file describes the conditions
// under which this software may be distributed.

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

// -----------------------------------------
// Functions classifying a single character.

// This function is generic and should be probably moved to CharSet.h where
// IsAlphaNumeric the others reside.
inline bool IsAlpha(int ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static inline bool IsIdentifierChar(int ch) {
	// Identifiers cannot contain non-ASCII letters; a word with non-English
	// language-specific characters cannot be an identifier.
	return IsAlphaNumeric(ch) || ch == '_';
}

static inline bool IsIdentifierStart(int ch) {
	// Identifiers cannot contain non-ASCII letters; a word with non-English
	// language-specific characters cannot be an identifier.
	return IsAlpha(ch) || ch == '_';
}

static inline bool IsNumberChar(int ch, int chNext) {
	// Numeric constructs are not checked for lexical correctness. They are
	// expected to look like +1.23-E9 but actually any bunch of the following
	// characters will be styled as number.
	// KNOWN PROBLEM: if you put + or - operators immediately after a number
	// and the next operand starts with the letter E, the operator will not be
	// recognized and it will be styled together with the preceding number.
	// This should not occur; at least not often. The coding style recommends
	// putting spaces around operators.
	return IsADigit(ch) || toupper(ch) == 'E' || ch == '.' ||
		   ((ch == '-' || ch == '+') && toupper(chNext) == 'E');
}

// This function checks for the start or a natural number without any symbols
// or operators as a prefix; the IsPrefixedNumberStart should be called
// immediately after this one to cover all possible numeric constructs.
static inline bool IsNaturalNumberStart(int ch) {
	return IsADigit(ch) != 0;
}

static inline bool IsPrefixedNumberStart(int ch, int chNext) {
	// KNOWN PROBLEM: if you put + or - operators immediately before a number
	// the operator will not be recognized and it will be styled together with
	// the succeeding number. This should not occur; at least not often. The
	// coding style recommends putting spaces around operators.
	return (ch == '.' || ch == '-' || ch == '+') && IsADigit(chNext);
}

static inline bool IsOperator(int ch) {
	return strchr("%^&*()-+={}[]:;<>,/?!.~|\\", ch) != NULL;
}

// ---------------------------------------------------------------
// Functions classifying a token currently processed in the lexer.

// Checks if the current line starts with the preprocessor directive used
// usually to introduce documentation comments: #ifdef DOC. This method is
// supposed to be called if the line has been recognized as a preprocessor
// directive already.
static bool IsDocCommentStart(StyleContext &sc) {
	// Check the line back to its start only if the end looks promising.
	if (sc.LengthCurrent() == 10 && !IsAlphaNumeric(sc.ch)) {
		char s[11];
		sc.GetCurrentLowered(s, sizeof(s));
		return strcmp(s, "#ifdef doc") == 0;
	}
	return false;
}

// Checks if the current line starts with the preprocessor directive that
// is complementary to the #ifdef DOC start: #endif. This method is supposed
// to be called if the current state point to the documentation comment.
// QUESTIONAL ASSUMPTION: The complete #endif directive is not checked; just
// the starting #e. However, there is no other preprocessor directive with
// the same starting letter and thus this optimization should always work.
static bool IsDocCommentEnd(StyleContext &sc) {
	return sc.ch == '#' && sc.chNext == 'e';
}

class IdentifierClassifier {
	WordList &keywords;  // Passed from keywords property.
	WordList &constants; // Passed from keywords2 property.
	WordList &operators; // Passed from keywords3 property.
	WordList &types;     // Passed from keywords4 property.
	WordList &functions; // Passed from keywords5 property.
	WordList &objects;   // Passed from keywords6 property.

	IdentifierClassifier(IdentifierClassifier const&);
	IdentifierClassifier& operator=(IdentifierClassifier const&);

public:
	IdentifierClassifier(WordList *keywordlists[]) :
		keywords(*keywordlists[0]), constants(*keywordlists[1]),
		operators(*keywordlists[2]), types(*keywordlists[3]),
		functions(*keywordlists[4]), objects(*keywordlists[5])
	{}

	void ClassifyIdentifier(StyleContext &sc) {
		// Opening parenthesis following an identifier makes it a possible
		// function call.
		// KNOWN PROBLEM: If some whitespace is inserted between the
		// identifier and the parenthesis they will not be able to be
		// recognized as a function call. This should not occur; at
		// least not often. Such coding style would be weird.
		if (sc.Match('(')) {
			char s[100];
			sc.GetCurrentLowered(s, sizeof(s));
			// Before an opening brace can be control statements and
			// operators too; function call is the last option.
			if (keywords.InList(s)) {
				sc.ChangeState(SCE_OSCRIPT_KEYWORD);
			} else if (operators.InList(s)) {
				sc.ChangeState(SCE_OSCRIPT_OPERATOR);
			} else if (functions.InList(s)) {
				sc.ChangeState(SCE_OSCRIPT_FUNCTION);
			} else {
				sc.ChangeState(SCE_OSCRIPT_METHOD);
			}
			sc.SetState(SCE_OSCRIPT_OPERATOR);
		} else {
			char s[100];
			sc.GetCurrentLowered(s, sizeof(s));
			// A dot following an identifier means an access to an object
			// member. The related object identifier can be special.
			// KNOWN PROBLEM: If there is whitespace between the identifier
			// and the following dot, the identifier will not be recognized
			// as an object in an object member access. If it is one of the
			// listed static objects it will not be styled.
			if (sc.Match('.') && objects.InList(s)) {
				sc.ChangeState(SCE_OSCRIPT_OBJECT);
				sc.SetState(SCE_OSCRIPT_OPERATOR);
			} else {
				if (keywords.InList(s)) {
					sc.ChangeState(SCE_OSCRIPT_KEYWORD);
				} else if (constants.InList(s)) {
					sc.ChangeState(SCE_OSCRIPT_CONSTANT);
				} else if (operators.InList(s)) {
					sc.ChangeState(SCE_OSCRIPT_OPERATOR);
				} else if (types.InList(s)) {
					sc.ChangeState(SCE_OSCRIPT_TYPE);
				} else if (functions.InList(s)) {
					sc.ChangeState(SCE_OSCRIPT_FUNCTION);
				}
				sc.SetState(SCE_OSCRIPT_DEFAULT);
			}
		}
	}
};

// ------------------------------------------------
// Function colourising an excerpt of OScript code.

static void ColouriseOScriptDoc(Sci_PositionU startPos, Sci_Position length,
								int initStyle, WordList *keywordlists[],
								Accessor &styler) {
	// I wonder how whole-line styles ended by EOLN can escape the resetting
	// code in the loop below and overflow to the next line. Let us make sure
	// that a new line does not start with them carried from the previous one.
	// NOTE: An overflowing string is intentionally not checked; it reminds
	// the developer that the string must be ended on the same line.
	if (initStyle == SCE_OSCRIPT_LINE_COMMENT ||
			initStyle == SCE_OSCRIPT_PREPROCESSOR) {
		initStyle = SCE_OSCRIPT_DEFAULT;
	}

	styler.StartAt(startPos);
	StyleContext sc(startPos, length, initStyle, styler);
	IdentifierClassifier identifierClassifier(keywordlists);

	// It starts with true at the beginning of a line and changes to false as
	// soon as the first non-whitespace character has been processed.
	bool isFirstToken = true;
	// It starts with true at the beginning of a line and changes to false as
	// soon as the first identifier on the line is passed by.
	bool isFirstIdentifier = true;
	// It becomes false when #ifdef DOC (the preprocessor directive often
	// used to start a documentation comment) is encountered and remain false
	// until the end of the documentation block is not detected. This is done
	// by checking for the complementary #endif preprocessor directive.
	bool endDocComment = false;

	for (; sc.More(); sc.Forward()) {

		if (sc.atLineStart) {
			isFirstToken = true;
			isFirstIdentifier = true;
		// Detect the current state is neither whitespace nor identifier. It
		// means that no next identifier can be the first token on the line.
		} else if (isFirstIdentifier && sc.state != SCE_OSCRIPT_DEFAULT &&
				   sc.state != SCE_OSCRIPT_IDENTIFIER) {
			isFirstIdentifier = false;
		}

		// Check if the current state should be changed.
		if (sc.state == SCE_OSCRIPT_OPERATOR) {
			// Multiple-symbol operators are marked by single characters.
			sc.SetState(SCE_OSCRIPT_DEFAULT);
		} else if (sc.state == SCE_OSCRIPT_IDENTIFIER) {
			if (!IsIdentifierChar(sc.ch)) {
				// Colon after an identifier makes it a label if it is the
				// first token on the line.
				// KNOWN PROBLEM: If some whitespace is inserted between the
				// identifier and the colon they will not be recognized as a
				// label. This should not occur; at least not often. It would
				// make the code structure less legible and examples in the
				// Livelink documentation do not show it.
				if (sc.Match(':') && isFirstIdentifier) {
					sc.ChangeState(SCE_OSCRIPT_LABEL);
					sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
				} else {
					identifierClassifier.ClassifyIdentifier(sc);
				}
				// Avoid a sequence of two words be mistaken for a label. A
				// switch case would be an example.
				isFirstIdentifier = false;
			}
		} else if (sc.state == SCE_OSCRIPT_GLOBAL) {
			if (!IsIdentifierChar(sc.ch)) {
				sc.SetState(SCE_OSCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_OSCRIPT_PROPERTY) {
			if (!IsIdentifierChar(sc.ch)) {
				// Any member access introduced by the dot operator is
				// initially marked as a property access. If an opening
				// parenthesis is detected later it is changed to method call.
				// KNOWN PROBLEM: The same as at the function call recognition
				// for SCE_OSCRIPT_IDENTIFIER above.
				if (sc.Match('(')) {
					sc.ChangeState(SCE_OSCRIPT_METHOD);
				}
				sc.SetState(SCE_OSCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_OSCRIPT_NUMBER) {
			if (!IsNumberChar(sc.ch, sc.chNext)) {
				sc.SetState(SCE_OSCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_OSCRIPT_SINGLEQUOTE_STRING) {
			if (sc.ch == '\'') {
				// Two consequential apostrophes convert to a single one.
				if (sc.chNext == '\'') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
				}
			} else if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_OSCRIPT_DOUBLEQUOTE_STRING) {
			if (sc.ch == '\"') {
				// Two consequential quotation marks convert to a single one.
				if (sc.chNext == '\"') {
					sc.Forward();
				} else {
					sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
				}
			} else if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_OSCRIPT_BLOCK_COMMENT) {
			if (sc.Match('*', '/')) {
				sc.Forward();
				sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_OSCRIPT_LINE_COMMENT) {
			if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_OSCRIPT_PREPROCESSOR) {
			if (IsDocCommentStart(sc)) {
				sc.ChangeState(SCE_OSCRIPT_DOC_COMMENT);
				endDocComment = false;
			} else if (sc.atLineEnd) {
				sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
			}
		} else if (sc.state == SCE_OSCRIPT_DOC_COMMENT) {
			// KNOWN PROBLEM: The first line detected that would close a
			// conditional preprocessor block (#endif) the documentation
			// comment block will end. (Nested #if-#endif blocks are not
			// supported. Hopefully it will not occur often that a line
			// within the text block would stat with #endif.
			if (isFirstToken && IsDocCommentEnd(sc)) {
				endDocComment = true;
			} else if (sc.atLineEnd && endDocComment) {
				sc.ForwardSetState(SCE_OSCRIPT_DEFAULT);
			}
		}

		// Check what state starts with the current character.
		if (sc.state == SCE_OSCRIPT_DEFAULT) {
			if (sc.Match('\'')) {
				sc.SetState(SCE_OSCRIPT_SINGLEQUOTE_STRING);
			} else if (sc.Match('\"')) {
				sc.SetState(SCE_OSCRIPT_DOUBLEQUOTE_STRING);
			} else if (sc.Match('/', '/')) {
				sc.SetState(SCE_OSCRIPT_LINE_COMMENT);
				sc.Forward();
			} else if (sc.Match('/', '*')) {
				sc.SetState(SCE_OSCRIPT_BLOCK_COMMENT);
				sc.Forward();
			} else if (isFirstToken && sc.Match('#')) {
				sc.SetState(SCE_OSCRIPT_PREPROCESSOR);
			} else if (sc.Match('$')) {
				// Both process-global ($xxx) and thread-global ($$xxx)
				// variables are handled as one global.
				sc.SetState(SCE_OSCRIPT_GLOBAL);
			} else if (IsNaturalNumberStart(sc.ch)) {
				sc.SetState(SCE_OSCRIPT_NUMBER);
			} else if (IsPrefixedNumberStart(sc.ch, sc.chNext)) {
				sc.SetState(SCE_OSCRIPT_NUMBER);
				sc.Forward();
			} else if (sc.Match('.') && IsIdentifierStart(sc.chNext)) {
				// Every object member access is marked as a property access
				// initially. The decision between property and method is made
				// after parsing the identifier and looking what comes then.
				// KNOWN PROBLEM: If there is whitespace between the following
				// identifier and the dot, the dot will not be recognized
				// as a member accessing operator. In turn, the identifier
				// will not be recognizable as a property or a method too.
				sc.SetState(SCE_OSCRIPT_OPERATOR);
				sc.Forward();
				sc.SetState(SCE_OSCRIPT_PROPERTY);
			} else if (IsIdentifierStart(sc.ch)) {
				sc.SetState(SCE_OSCRIPT_IDENTIFIER);
			} else if (IsOperator(sc.ch)) {
				sc.SetState(SCE_OSCRIPT_OPERATOR);
			}
		}

		if (isFirstToken && !IsASpaceOrTab(sc.ch)) {
			isFirstToken = false;
		}
	}

	sc.Complete();
}

// ------------------------------------------
// Functions supporting OScript code folding.

static inline bool IsBlockComment(int style) {
	return style == SCE_OSCRIPT_BLOCK_COMMENT;
}

static bool IsLineComment(Sci_Position line, Accessor &styler) {
	Sci_Position pos = styler.LineStart(line);
	Sci_Position eolPos = styler.LineStart(line + 1) - 1;
	for (Sci_Position i = pos; i < eolPos; i++) {
		char ch = styler[i];
		char chNext = styler.SafeGetCharAt(i + 1);
		int style = styler.StyleAt(i);
		if (ch == '/' && chNext == '/' && style == SCE_OSCRIPT_LINE_COMMENT) {
			return true;
		} else if (!IsASpaceOrTab(ch)) {
			return false;
		}
	}
	return false;
}

static inline bool IsPreprocessor(int style) {
	return style == SCE_OSCRIPT_PREPROCESSOR ||
		   style == SCE_OSCRIPT_DOC_COMMENT;
}

static void GetRangeLowered(Sci_PositionU start, Sci_PositionU end,
							Accessor &styler, char *s, Sci_PositionU len) {
	Sci_PositionU i = 0;
	while (i < end - start + 1 && i < len - 1) {
		s[i] = static_cast<char>(tolower(styler[start + i]));
		i++;
	}
	s[i] = '\0';
}

static void GetForwardWordLowered(Sci_PositionU start, Accessor &styler,
								  char *s, Sci_PositionU len) {
	Sci_PositionU i = 0;
	while (i < len - 1 && IsAlpha(styler.SafeGetCharAt(start + i))) {
		s[i] = static_cast<char>(tolower(styler.SafeGetCharAt(start + i)));
		i++;
	}
	s[i] = '\0';
}

static void UpdatePreprocessorFoldLevel(int &levelCurrent,
		Sci_PositionU startPos, Accessor &styler) {
	char s[7]; // Size of the longest possible keyword + null.
	GetForwardWordLowered(startPos, styler, s, sizeof(s));

	if (strcmp(s, "ifdef") == 0 ||
		strcmp(s, "ifndef") == 0) {
		levelCurrent++;
	} else if (strcmp(s, "endif") == 0) {
		levelCurrent--;
		if (levelCurrent < SC_FOLDLEVELBASE) {
			levelCurrent = SC_FOLDLEVELBASE;
		}
	}
}

static void UpdateKeywordFoldLevel(int &levelCurrent, Sci_PositionU lastStart,
		Sci_PositionU currentPos, Accessor &styler) {
	char s[9];
	GetRangeLowered(lastStart, currentPos, styler, s, sizeof(s));

	if (strcmp(s, "if") == 0 || strcmp(s, "for") == 0 ||
		strcmp(s, "switch") == 0 || strcmp(s, "function") == 0 ||
		strcmp(s, "while") == 0 || strcmp(s, "repeat") == 0) {
		levelCurrent++;
	} else if (strcmp(s, "end") == 0 || strcmp(s, "until") == 0) {
		levelCurrent--;
		if (levelCurrent < SC_FOLDLEVELBASE) {
			levelCurrent = SC_FOLDLEVELBASE;
		}
	}
}

// ------------------------------
// Function folding OScript code.

static void FoldOScriptDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
						   WordList *[], Accessor &styler) {
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldPreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_Position endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	Sci_Position lastStart = 0;

	for (Sci_Position i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atLineEnd = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (foldComment && IsBlockComment(style)) {
			if (!IsBlockComment(stylePrev)) {
				levelCurrent++;
			} else if (!IsBlockComment(styleNext) && !atLineEnd) {
				// Comments do not end at end of line and the next character
				// may not be styled.
				levelCurrent--;
			}
		}
		if (foldComment && atLineEnd && IsLineComment(lineCurrent, styler)) {
			if (!IsLineComment(lineCurrent - 1, styler) &&
				IsLineComment(lineCurrent + 1, styler))
				levelCurrent++;
			else if (IsLineComment(lineCurrent - 1, styler) &&
					 !IsLineComment(lineCurrent+1, styler))
				levelCurrent--;
		}
		if (foldPreprocessor) {
			if (ch == '#' && IsPreprocessor(style)) {
				UpdatePreprocessorFoldLevel(levelCurrent, i + 1, styler);
			}
		}

		if (stylePrev != SCE_OSCRIPT_KEYWORD && style == SCE_OSCRIPT_KEYWORD) {
			lastStart = i;
		}
		if (stylePrev == SCE_OSCRIPT_KEYWORD) {
			if(IsIdentifierChar(ch) && !IsIdentifierChar(chNext)) {
				UpdateKeywordFoldLevel(levelCurrent, lastStart, i, styler);
			}
		}

		if (!IsASpace(ch))
			visibleChars++;

		if (atLineEnd) {
			int level = levelPrev;
			if (visibleChars == 0 && foldCompact)
				level |= SC_FOLDLEVELWHITEFLAG;
			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				level |= SC_FOLDLEVELHEADERFLAG;
			if (level != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, level);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
	}

	// If we did not reach EOLN in the previous loop, store the line level and
	// whitespace information. The rest will be filled in later.
	int lev = levelPrev;
	if (visibleChars == 0 && foldCompact)
		lev |= SC_FOLDLEVELWHITEFLAG;
	styler.SetLevel(lineCurrent, lev);
}

// --------------------------------------------
// Declaration of the OScript lexer descriptor.

static const char * const oscriptWordListDesc[] = {
	"Keywords and reserved words",
	"Literal constants",
	"Literal operators",
	"Built-in value and reference types",
	"Built-in global functions",
	"Built-in static objects",
	0
};

LexerModule lmOScript(SCLEX_OSCRIPT, ColouriseOScriptDoc, "oscript", FoldOScriptDoc, oscriptWordListDesc);
