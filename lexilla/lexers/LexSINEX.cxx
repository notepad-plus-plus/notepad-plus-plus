// Scintilla source code edit control
// Encoding: UTF-8
/** @file LexSINEX.cxx
 ** Lexer for SINEX (Solution INdependent EXchange format) files
 ** https://www.iers.org/SharedDocs/Publikationen/EN/IERS/Documents/ac/sinex/sinex_v202_pdf.pdf
 **
 ** Written by Franck Reinquin
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

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

#include "LexAccessor.h"
#include "Accessor.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

namespace {
// Use an unnamed namespace to protect the functions and classes from name conflicts


// States when parsing a real number
typedef enum {
		NOTHING=0,    // parsing no started
		SIGN_1=1,     // a sign (+/-) was encountered for the first time
		MANTISSA_1=2, // one or more consecutive digits were encountered for the first time
		DOT=3,        // the scientific notation letter (e,E,d,D) was encountered
		MANTISSA_2=4, // one or more consecutive digits were encountered after a dot
		D_OR_E=5,     // the scientific notation letter (e,E,d,D) was encountered
		SIGN_2=6,     // a sign (+/-) was encountered for the second time
		EXPONENT=7    // one or more consecutive digits were encountered 
} E_REAL_PARSING_STATE ;


// Check if end of line encountered. Possible terminators : '\n', '\r', '\r\n'
// For '\r\n' terminators, the EOL is reached at the '\n' character.
inline bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}


inline bool IsCommentLine(Accessor &styler, Sci_Position line) {
	Sci_Position pos = styler.LineStart(line);
	return (styler.StyleAt(pos) == SCE_SINEX_COMMENTLINE);
}


// Check whether the string is number, either integer or float ; the
// scientific representation is also accepted.
// Implemented as a finite-state machine
// Mostly equivalent to REGEX : ^[+-]?\d*\.?\d+([eE][+-]?\d+)?$
inline bool IsSINEXNumber(const char *text, Sci_PositionU len) {

	E_REAL_PARSING_STATE	parsingState = NOTHING ;
	bool   firstDigitsFound = false;
	
	if (len == 0)
		return false;

	for (Sci_PositionU i = 0 ; i < len ; i++) {
		if ((text[i] == '-') || (text[i] == '+')) {
			// valid only at the beginning and after an exponent letter
			if (parsingState == NOTHING) {
				parsingState = SIGN_1 ;
			} else if (parsingState == D_OR_E) {
				parsingState = SIGN_2;
			} else {
				return false;
			}

		} else if (text[i] == '.') {
			// valid only after the first digits, which can be absent (e.g. '-.12')
			if ((parsingState == NOTHING) || (parsingState == SIGN_1) \
			 || (parsingState == MANTISSA_1)) {
				parsingState = DOT;
			} else {
				return false;
			}

		} else if ((text[i] == 'e') || (text[i] == 'E') || (text[i] == 'd') \
		     || (text[i] == 'D')) {
			// valid only after the first digits ('.e+7' is NOK)
			if (! firstDigitsFound) return false;
			if ((parsingState == MANTISSA_1) || (parsingState == MANTISSA_2) \
             || (parsingState == DOT)) {
				parsingState = D_OR_E;
			} else {
				return false;
			}

		} else if (isdigit(text[i])) {
			if ((parsingState == NOTHING) || (parsingState == SIGN_1) \
			|| (parsingState == MANTISSA_1)) {
				parsingState = MANTISSA_1;
                firstDigitsFound = true;
			} else if ((parsingState == DOT) || (parsingState == MANTISSA_2)) {
				parsingState = MANTISSA_2;
                firstDigitsFound = true;
			} else if ((parsingState == D_OR_E) || (parsingState == SIGN_2) \
			       || (parsingState == EXPONENT)) {
				parsingState = EXPONENT;
			} else {
				return false;
			}
		
		} else {
			// other characters are not valid
			return false ;
		}
	}
	return (firstDigitsFound && (parsingState != D_OR_E) && (parsingState != SIGN_2));
}


// Check whether the string is a SINEX date (YY:DDD:SSSSS)
// For the record : YY = 2-digit year (!), DDD = Day Of Year, SSSSS = seconds
// in the day
inline bool IsSINEXDate(const char *text, Sci_PositionU len) {

	if (len < 11) return false;
	return (IsADigit(text[0]) && IsADigit(text[1]) && text[2] == ':' &&
	        IsADigit(text[3]) && IsADigit(text[4]) && IsADigit(text[5]) && text[6] == ':' &&
	        IsADigit(text[7]) && IsADigit(text[8]) && IsADigit(text[9]) && 
	        IsADigit(text[10]) && IsADigit(text[11]));
}

// Find next space in the string : return an offset in the string >= start
// or len if not found
static Sci_PositionU FindNextSpace(const char *text, 
                                   Sci_PositionU start, Sci_PositionU len) {
	Sci_PositionU  pos ;
	for (pos = start ; pos < len ; pos ++ ) {
		if (IsASpace(text[pos]))
			return pos;
	}
    return pos;
}

// Colourization logic for one line
void ColouriseSinexLine(
	const char *lineBuffer,
	Sci_PositionU lengthLine,
	Sci_PositionU startLine,
	Sci_PositionU endPos,
	Accessor &styler) {

	if (lengthLine == 0) 
		return;

	// comment line
	if (lineBuffer[0] == '*') {
		styler.ColourTo(endPos, SCE_SINEX_COMMENTLINE);

	// block start (+BLOCK_NAME)
	} else if (lineBuffer[0] == '+') {
		styler.ColourTo(endPos, SCE_SINEX_BLOCK_START);

	// block end (-BLOCK_NAME)
	} else if (lineBuffer[0] == '-') {
		styler.ColourTo(endPos, SCE_SINEX_BLOCK_END);

	// Other lines : parse content
	} else {
		Sci_PositionU i = 0 ;
		Sci_PositionU nextSpace ;
		
		// process word by word
		while ((nextSpace = FindNextSpace(lineBuffer,i,lengthLine)) < lengthLine) {
			// Detect dates YY:DDD:SSSSS (first test aims at speeding up detection)
			if (IsADigit(lineBuffer[i]) && IsSINEXDate(&lineBuffer[i], nextSpace-i)) {
				styler.ColourTo(startLine+nextSpace-1, SCE_SINEX_DATE);
			// Numbers (integers or floats, including scientific notation)
			} else if (IsSINEXNumber(&lineBuffer[i], nextSpace-i)) {
				styler.ColourTo(startLine+nextSpace-1, SCE_SINEX_NUMBER);
			}
			// consume all spaces
			for (i=nextSpace ; (i < lengthLine) && IsASpace(lineBuffer[i]) ; i++)
				;
			styler.ColourTo(startLine+i-1, SCE_SINEX_DEFAULT);
		}
		styler.ColourTo(endPos, SCE_SINEX_DEFAULT);
	}
}


// Colourization logic for a whole area
// The area is split into lines which are separately processed
void ColouriseSinexDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
    // initStyle not needed as each line is independent
	std::string lineBuffer;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU startLine = startPos;

	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer.push_back(styler[i]);
		if (AtEOL(styler, i)) {
			// End of line (or of line buffer) met, colourise it
			ColouriseSinexLine(lineBuffer.c_str(), lineBuffer.length(), startLine, i, styler);
			lineBuffer.clear();
			startLine = i + 1;
		}
	}
	if (!lineBuffer.empty()) {	// Last line does not have ending characters
		ColouriseSinexLine(lineBuffer.c_str(), lineBuffer.length(), startLine, startPos + length - 1, styler);
	}
}


// Folding logic
void FoldSinexDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {

	if (AtEOL(styler, startPos)) 
		return ;

	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	Sci_PositionU endPos = startPos + length;
	int styleCurrent;

	// One-line comments are not folded, multi-line comments may be folded
	// (see "fold.comment" property) : we  need to check the lines before the
	// first line.
	// Possible cases :
	// *  start of a comment block : first line = comment, 1 comment line before
	// *  continuation of a comment block : first line = comment , >1 comment
	//    lines before
	// * end of a comment line : first line != comment, >1 comment lines before
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int nbCommentLines = 0 ;
	while (lineCurrent > 0) {
		if (!IsCommentLine(styler, lineCurrent-1))
			break ;
		nbCommentLines++;
		lineCurrent--;
	}

	// Go back to the start of the comment block, if any. Level at that line
	// is known
	Sci_Position newStartPos = (nbCommentLines == 0) ? startPos : styler.LineStart(lineCurrent);

	// Now go through the provided text
	int levelCurrent = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelPrev = levelCurrent;
	int levelNext = levelCurrent;
	int indComment = 0;

	for (Sci_PositionU i = newStartPos; i < endPos; i++) {
		if (AtEOL(styler, i)) {
			levelCurrent = levelNext ;
			styleCurrent  = styler.StyleAt(i);
			if (foldComment) {
				if (styleCurrent == SCE_SINEX_COMMENTLINE) {
					indComment++ ;
					if (indComment==2) {
						// second comment line ->increase level
						// (do nothing for single comment lines)
						levelCurrent++;
					}
				} else {
					// not a comment line : decrease level if it follows a
					// multi-line comment
					if (indComment >= 2) {
						levelCurrent--;
					}
					indComment = 0;
				}
				levelNext = levelCurrent ;
			}
			switch (styleCurrent) {
			case SCE_SINEX_BLOCK_START:
				levelNext++;
				break;
			case SCE_SINEX_BLOCK_END:
				levelNext--;
				break;
			}
			styler.SetLevel(lineCurrent, levelCurrent);

			// now update previous line state (if header)
			if (levelCurrent > levelPrev) {
				int lev = levelPrev;
				lev |= SC_FOLDLEVELHEADERFLAG;
				//lev |= 1<<16;
				if (lev != styler.LevelAt(lineCurrent-1)) {
					styler.SetLevel(lineCurrent-1, lev);
				}
			}
			lineCurrent++;
			levelPrev = levelCurrent ;
		}

	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelNext | flagsNext);
}

const char * const sinexWordListDesc[] = {
	"SNX",
	0
};

}  // unnamed namespace end

extern const LexerModule lmSINEX(SCLEX_SINEX, ColouriseSinexDoc, "sinex", FoldSinexDoc, sinexWordListDesc);
