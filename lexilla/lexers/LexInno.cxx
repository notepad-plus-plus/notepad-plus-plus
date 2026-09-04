// Scintilla source code edit control
/** @file LexInno.cxx
 ** Lexer for Inno Setup scripts.
 **/
// Written by Friedrich Vedder <fvedd@t-online.de>, using code from LexOthers.cxx.
// Modified by Michael Heath.
// The License.txt file describes the conditions under which this software may be distributed.

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

static bool innoIsBlank(int ch) {
	return (ch == ' ') || (ch == '\t');
}

static bool innoNextNotBlankIs(Sci_Position i, Accessor &styler, char needle) {
	char ch;

	while (i < styler.Length()) {
		ch = styler.SafeGetCharAt(i);

		if (ch == needle)
			return true;

		if (!innoIsBlank(ch))
			return false;

		i++;
	}
	return false;
}

static void ColouriseInnoDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *keywordLists[], Accessor &styler) {
	int state = SCE_INNO_DEFAULT;
	char chPrev;
	char ch = 0;
	char chNext = styler[startPos];
	Sci_Position lengthDoc = startPos + length;
	char *buffer = new char[length + 1];
	Sci_Position bufferCount = 0;
	bool isBOL, isEOL, isWS, isBOLWS = 0;

	// Save line state later with bitState and bitand with bit... to get line state
	int bitState = 0;
	int const bitCode = 1, bitMessages = 2, bitCommentCurly = 4, bitCommentRound = 8;

	// Get keyword lists
	WordList &sectionKeywords = *keywordLists[0];
	WordList &standardKeywords = *keywordLists[1];
	WordList &parameterKeywords = *keywordLists[2];
	WordList &preprocessorKeywords = *keywordLists[3];
	WordList &pascalKeywords = *keywordLists[4];
	WordList &userKeywords = *keywordLists[5];

	// Get line state
	Sci_Position curLine = styler.GetLine(startPos);
	int curLineState = curLine > 0 ? styler.GetLineState(curLine - 1) : 0;
	bool isCode = (curLineState & bitCode);
	bool isMessages = (curLineState & bitMessages);
	bool isCommentCurly = (curLineState & bitCommentCurly);
	bool isCommentRound = (curLineState & bitCommentRound);
	bool isCommentSlash = false;

	// Continue Pascal multline comment state
	if (isCommentCurly || isCommentRound)
		state = SCE_INNO_COMMENT_PASCAL;

	// Go through all provided text segment
	// using the hand-written state machine shown below
	styler.StartAt(startPos);
	styler.StartSegment(startPos);

	for (Sci_Position i = startPos; i < lengthDoc; i++) {
		chPrev = ch;
		ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if (styler.IsLeadByte(ch)) {
			chNext = styler.SafeGetCharAt(i + 2);
			i++;
			continue;
		}

		isBOL = (chPrev == 0) || (chPrev == '\n') || (chPrev == '\r' && ch != '\n');
		isBOLWS = (isBOL) ? 1 : (isBOLWS && (chPrev == ' ' || chPrev == '\t'));
		isEOL = (ch == '\n' || ch == '\r');
		isWS = (ch == ' ' || ch == '\t');

		if ((ch == '\r' && chNext != '\n') || (ch == '\n')) {
			// Remember the line state for future incremental lexing
			curLine = styler.GetLine(i);
			bitState = 0;

			if (isCode)
				bitState |= bitCode;

			if (isMessages)
				bitState |= bitMessages;

			if (isCommentCurly)
				bitState |= bitCommentCurly;

			if (isCommentRound)
				bitState |= bitCommentRound;

			styler.SetLineState(curLine, bitState);
		}

		switch(state) {
			case SCE_INNO_DEFAULT:
				if (!isCode && ch == ';' && isBOLWS) {
					// Start of a comment
					state = SCE_INNO_COMMENT;
					styler.ColourTo(i, SCE_INNO_COMMENT);
				} else if (ch == '[' && isBOLWS) {
					// Start of a section name
					state = SCE_INNO_SECTION;
					bufferCount = 0;
				} else if (ch == '#' && isBOLWS) {
					// Start of a preprocessor directive
					state = SCE_INNO_PREPROC;
				} else if (!isCode && ch == '{' && chNext != '{' && chPrev != '{') {
					// Start of an inline expansion
					state = SCE_INNO_INLINE_EXPANSION;
				} else if (isCode && ch == '{') {
					// Start of a Pascal comment
					state = SCE_INNO_COMMENT_PASCAL;
					isCommentCurly = true;
					styler.ColourTo(i, SCE_INNO_COMMENT_PASCAL);
				} else if (isCode && (ch == '(' && chNext == '*')) {
					// Start of a Pascal comment
					state = SCE_INNO_COMMENT_PASCAL;
					isCommentRound = true;
					styler.ColourTo(i + 1, SCE_INNO_COMMENT_PASCAL);
				} else if (isCode && ch == '/' && chNext == '/') {
					// Start of C-style comment
					state = SCE_INNO_COMMENT_PASCAL;
					isCommentSlash = true;
					styler.ColourTo(i + 1, SCE_INNO_COMMENT_PASCAL);
				} else if (!isMessages && ch == '"') {
					// Start of a double-quote string
					state = SCE_INNO_STRING_DOUBLE;
					styler.ColourTo(i, SCE_INNO_STRING_DOUBLE);
				} else if (!isMessages && ch == '\'') {
					// Start of a single-quote string
					state = SCE_INNO_STRING_SINGLE;
					styler.ColourTo(i, SCE_INNO_STRING_SINGLE);
				} else if (!isMessages && IsASCII(ch) && (isalpha(ch) || (ch == '_'))) {
					// Start of an identifier
					state = SCE_INNO_IDENTIFIER;
					bufferCount = 0;
					buffer[bufferCount++] = static_cast<char>(tolower(ch));
				} else {
					// Style it the default style
					styler.ColourTo(i, SCE_INNO_DEFAULT);
				}
				break;

			case SCE_INNO_COMMENT:
				if (isEOL) {
					state = SCE_INNO_DEFAULT;
					styler.ColourTo(i - 1, SCE_INNO_COMMENT);
					styler.ColourTo(i, SCE_INNO_DEFAULT);
				} else {
					styler.ColourTo(i, SCE_INNO_COMMENT);
				}
				break;

			case SCE_INNO_IDENTIFIER:
				if (IsASCII(ch) && (isalnum(ch) || (ch == '_'))) {
					buffer[bufferCount++] = static_cast<char>(tolower(ch));
				} else {
					state = SCE_INNO_DEFAULT;
					buffer[bufferCount] = '\0';

					// Check if the buffer contains a keyword
					if (!isCode && standardKeywords.InList(buffer) && innoNextNotBlankIs(i, styler, '=')) {
						styler.ColourTo(i - 1, SCE_INNO_KEYWORD);
					} else if (!isCode && parameterKeywords.InList(buffer) && innoNextNotBlankIs(i, styler, ':')) {
						styler.ColourTo(i - 1, SCE_INNO_PARAMETER);
					} else if (isCode && pascalKeywords.InList(buffer)) {
						styler.ColourTo(i - 1, SCE_INNO_KEYWORD_PASCAL);
					} else if (!isCode && userKeywords.InList(buffer)) {
						styler.ColourTo(i - 1, SCE_INNO_KEYWORD_USER);
					} else {
						styler.ColourTo(i - 1, SCE_INNO_DEFAULT);
					}

					// Push back the faulty character
					chNext = styler[i--];
					ch = chPrev;
				}
				break;

			case SCE_INNO_SECTION:
				if (ch == ']') {
					state = SCE_INNO_DEFAULT;
					buffer[bufferCount] = '\0';

					// Check if the buffer contains a section name
					if (sectionKeywords.InList(buffer)) {
						styler.ColourTo(i, SCE_INNO_SECTION);
						isCode = !CompareCaseInsensitive(buffer, "code");

						isMessages = isCode ? false : (
									!CompareCaseInsensitive(buffer, "custommessages")
									|| !CompareCaseInsensitive(buffer, "messages"));
					} else {
						styler.ColourTo(i, SCE_INNO_DEFAULT);
					}
				} else if (IsASCII(ch) && (isalnum(ch) || (ch == '_'))) {
					buffer[bufferCount++] = static_cast<char>(tolower(ch));
				} else {
					state = SCE_INNO_DEFAULT;
					styler.ColourTo(i, SCE_INNO_DEFAULT);
				}
				break;

			case SCE_INNO_PREPROC:
				if (isWS || isEOL) {
					if (IsASCII(chPrev) && isalpha(chPrev)) {
						state = SCE_INNO_DEFAULT;
						buffer[bufferCount] = '\0';

						// Check if the buffer contains a preprocessor directive
						if (preprocessorKeywords.InList(buffer)) {
							styler.ColourTo(i - 1, SCE_INNO_PREPROC);
						} else {
							styler.ColourTo(i - 1, SCE_INNO_DEFAULT);
						}

						// Push back the faulty character
						chNext = styler[i--];
						ch = chPrev;
					}
				} else if (IsASCII(ch) && isalpha(ch)) {
					if (chPrev == '#' || chPrev == ' ' || chPrev == '\t')
						bufferCount = 0;
					buffer[bufferCount++] = static_cast<char>(tolower(ch));
				}
				break;

			case SCE_INNO_STRING_DOUBLE:
				if (ch == '"') {
					state = SCE_INNO_DEFAULT;
					styler.ColourTo(i, SCE_INNO_STRING_DOUBLE);
				} else if (isEOL) {
					state = SCE_INNO_DEFAULT;
					styler.ColourTo(i - 1, SCE_INNO_STRING_DOUBLE);
					styler.ColourTo(i, SCE_INNO_DEFAULT);
				} else {
					styler.ColourTo(i, SCE_INNO_STRING_DOUBLE);
				}
				break;

			case SCE_INNO_STRING_SINGLE:
				if (ch == '\'') {
					state = SCE_INNO_DEFAULT;
					styler.ColourTo(i, SCE_INNO_STRING_SINGLE);
				} else if (isEOL) {
					state = SCE_INNO_DEFAULT;
					styler.ColourTo(i - 1, SCE_INNO_STRING_SINGLE);
					styler.ColourTo(i, SCE_INNO_DEFAULT);
				} else {
					styler.ColourTo(i, SCE_INNO_STRING_SINGLE);
				}
				break;

			case SCE_INNO_INLINE_EXPANSION:
				if (ch == '}') {
					state = SCE_INNO_DEFAULT;
					styler.ColourTo(i, SCE_INNO_INLINE_EXPANSION);
				} else if (isEOL) {
					state = SCE_INNO_DEFAULT;
					styler.ColourTo(i, SCE_INNO_DEFAULT);
				}
				break;

			case SCE_INNO_COMMENT_PASCAL:
				if (isCommentSlash) {
					if (isEOL) {
						state = SCE_INNO_DEFAULT;
						isCommentSlash = false;
						styler.ColourTo(i - 1, SCE_INNO_COMMENT_PASCAL);
						styler.ColourTo(i, SCE_INNO_DEFAULT);
					} else {
						styler.ColourTo(i, SCE_INNO_COMMENT_PASCAL);
					}
				} else if (isCommentCurly) {
					if (ch == '}') {
						state = SCE_INNO_DEFAULT;
						isCommentCurly = false;
					}
					styler.ColourTo(i, SCE_INNO_COMMENT_PASCAL);
				} else if (isCommentRound) {
					if (ch == ')' && chPrev == '*') {
						state = SCE_INNO_DEFAULT;
						isCommentRound = false;
					}
					styler.ColourTo(i, SCE_INNO_COMMENT_PASCAL);
				}
				break;

		}
	}
	delete []buffer;
}

static const char * const innoWordListDesc[] = {
	"Sections",
	"Keywords",
	"Parameters",
	"Preprocessor directives",
	"Pascal keywords",
	"User defined keywords",
	0
};

static void FoldInnoDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	Sci_PositionU endPos = startPos + length;
	char chNext = styler[startPos];

	Sci_Position lineCurrent = styler.GetLine(startPos);

	bool sectionFlag = false;
	int levelPrev = lineCurrent > 0 ? styler.LevelAt(lineCurrent - 1) : SC_FOLDLEVELBASE;
	int level;

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler[i + 1];
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		int style = styler.StyleAt(i);

		if (style == SCE_INNO_SECTION)
			sectionFlag = true;

		if (atEOL || i == endPos - 1) {
			if (sectionFlag) {
				level = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
				if (level == levelPrev)
					styler.SetLevel(lineCurrent - 1, levelPrev & ~SC_FOLDLEVELHEADERFLAG);
			} else {
				level = levelPrev & SC_FOLDLEVELNUMBERMASK;
				if (levelPrev & SC_FOLDLEVELHEADERFLAG)
					level++;
			}

			styler.SetLevel(lineCurrent, level);

			levelPrev = level;
			lineCurrent++;
			sectionFlag = false;
		}
	}
}

extern const LexerModule lmInno(SCLEX_INNOSETUP, ColouriseInnoDoc, "inno", FoldInnoDoc, innoWordListDesc);
