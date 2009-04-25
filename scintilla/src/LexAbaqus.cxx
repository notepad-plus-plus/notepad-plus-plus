// Scintilla source code edit control
/** @file LexABAQUS.cxx
 ** Lexer for ABAQUS. Based on the lexer for APDL by Hadar Raz.
 ** By Sergio Lucato.
 ** Sort of completely rewritten by Gertjan Kloosterman
 **/
// The License.txt file describes the conditions under which this software may be distributed.

// Code folding copyied and modified from LexBasic.cxx

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
	return (ch < 0x80 && (isalnum(ch) || (ch == '_')));
}

static inline bool IsAKeywordChar(const int ch) {
	return (ch < 0x80 && (isalnum(ch) || (ch == '_') || (ch == ' ')));
}

static inline bool IsASetChar(const int ch) {
	return (ch < 0x80 && (isalnum(ch) || (ch == '_') || (ch == '.') || (ch == '-')));
}

static inline bool IsAnOperator(char ch) {
	// '.' left out as it is used to make up numbers
	if (ch == '*' || ch == '/' || ch == '-' || ch == '+' ||
		ch == '(' || ch == ')' || ch == '=' || ch == '^' ||
		ch == '[' || ch == ']' || ch == '<' || ch == '&' ||
		ch == '>' || ch == ',' || ch == '|' || ch == '~' ||
		ch == '$' || ch == ':' || ch == '%')
		return true;
	return false;
}

static void ColouriseABAQUSDoc(unsigned int startPos, int length, int initStyle, WordList*[] /* *keywordlists[] */,
                            Accessor &styler) {
	enum localState { KW_LINE_KW, KW_LINE_COMMA, KW_LINE_PAR, KW_LINE_EQ, KW_LINE_VAL, \
					  DAT_LINE_VAL, DAT_LINE_COMMA,\
					  COMMENT_LINE,\
					  ST_ERROR, LINE_END } state ;

	// Do not leak onto next line
	state = LINE_END ;
	initStyle = SCE_ABAQUS_DEFAULT;
	StyleContext sc(startPos, length, initStyle, styler);

	// Things are actually quite simple
	// we have commentlines
	// keywordlines and datalines
	// On a data line there will only be colouring of numbers
	// a keyword line is constructed as
	// *word,[ paramname[=paramvalue]]*
	// if the line ends with a , the keyword line continues onto the new line

	for (; sc.More(); sc.Forward()) {
		switch ( state ) {
        case KW_LINE_KW :
            if ( sc.atLineEnd ) {
                // finished the line in keyword state, switch to LINE_END
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = LINE_END ;
            } else if ( IsAKeywordChar(sc.ch) ) {
                // nothing changes
                state = KW_LINE_KW ;
            } else if ( sc.ch == ',' ) {
                // Well well we say a comma, arguments *MUST* follow
                sc.SetState(SCE_ABAQUS_OPERATOR) ;
                state = KW_LINE_COMMA ;
            } else {
                // Flag an error
                sc.SetState(SCE_ABAQUS_PROCESSOR) ;
                state = ST_ERROR ;
            }
            // Done with processing
            break ;
        case KW_LINE_COMMA :
            // acomma on a keywordline was seen
            if ( IsAKeywordChar(sc.ch)) {
                sc.SetState(SCE_ABAQUS_ARGUMENT) ;
                state = KW_LINE_PAR ;
            } else if ( sc.atLineEnd || (sc.ch == ',') ) {
                // we remain in keyword mode
                state = KW_LINE_COMMA ;
            } else if ( sc.ch == ' ' ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = KW_LINE_COMMA ;
            } else {
                // Anything else constitutes an error
                sc.SetState(SCE_ABAQUS_PROCESSOR) ;
                state = ST_ERROR ;
            }
            break ;
        case KW_LINE_PAR :
            if ( sc.atLineEnd ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = LINE_END ;
            } else if ( IsAKeywordChar(sc.ch) || (sc.ch == '-') ) {
                // remain in this state
                state = KW_LINE_PAR ;
            } else if ( sc.ch == ',' ) {
                sc.SetState(SCE_ABAQUS_OPERATOR) ;
                state = KW_LINE_COMMA ;
            } else if ( sc.ch == '=' ) {
                sc.SetState(SCE_ABAQUS_OPERATOR) ;
                state = KW_LINE_EQ ;
            } else {
                // Anything else constitutes an error
                sc.SetState(SCE_ABAQUS_PROCESSOR) ;
                state = ST_ERROR ;
            }
            break ;
        case KW_LINE_EQ :
            if ( sc.ch == ' ' ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                // remain in this state
                state = KW_LINE_EQ ;
            } else if ( IsADigit(sc.ch) || (sc.ch == '-') || (sc.ch == '.' && IsADigit(sc.chNext)) ) {
                sc.SetState(SCE_ABAQUS_NUMBER) ;
                state = KW_LINE_VAL ;
            } else if ( IsAKeywordChar(sc.ch) ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = KW_LINE_VAL ;
            } else if ( (sc.ch == '\'') || (sc.ch == '\"') ) {
                sc.SetState(SCE_ABAQUS_STRING) ;
                state = KW_LINE_VAL ;
            } else {
                sc.SetState(SCE_ABAQUS_PROCESSOR) ;
                state = ST_ERROR ;
            }
            break ;
        case KW_LINE_VAL :
            if ( sc.atLineEnd ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = LINE_END ;
            } else if ( IsASetChar(sc.ch) && (sc.state == SCE_ABAQUS_DEFAULT) ) {
                // nothing changes
                state = KW_LINE_VAL ;
            } else if (( (IsADigit(sc.ch) || sc.ch == '.' || (sc.ch == 'e' || sc.ch == 'E') ||
                    ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E')))) &&
                    (sc.state == SCE_ABAQUS_NUMBER)) {
                // remain in number mode
                state = KW_LINE_VAL ;
            } else if (sc.state == SCE_ABAQUS_STRING) {
                // accept everything until a closing quote
                if ( sc.ch == '\'' || sc.ch == '\"' ) {
                    sc.SetState(SCE_ABAQUS_DEFAULT) ;
                    state = KW_LINE_VAL ;
                }
            } else if ( sc.ch == ',' ) {
                sc.SetState(SCE_ABAQUS_OPERATOR) ;
                state = KW_LINE_COMMA ;
            } else {
                // anything else is an error
                sc.SetState(SCE_ABAQUS_PROCESSOR) ;
                state = ST_ERROR ;
            }
            break ;
        case DAT_LINE_VAL :
            if ( sc.atLineEnd ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = LINE_END ;
            } else if ( IsASetChar(sc.ch) && (sc.state == SCE_ABAQUS_DEFAULT) ) {
                // nothing changes
                state = DAT_LINE_VAL ;
            } else if (( (IsADigit(sc.ch) || sc.ch == '.' || (sc.ch == 'e' || sc.ch == 'E') ||
                    ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E')))) &&
                    (sc.state == SCE_ABAQUS_NUMBER)) {
                // remain in number mode
                state = DAT_LINE_VAL ;
            } else if (sc.state == SCE_ABAQUS_STRING) {
                // accept everything until a closing quote
                if ( sc.ch == '\'' || sc.ch == '\"' ) {
                    sc.SetState(SCE_ABAQUS_DEFAULT) ;
                    state = DAT_LINE_VAL ;
                }
            } else if ( sc.ch == ',' ) {
                sc.SetState(SCE_ABAQUS_OPERATOR) ;
                state = DAT_LINE_COMMA ;
            } else {
                // anything else is an error
                sc.SetState(SCE_ABAQUS_PROCESSOR) ;
                state = ST_ERROR ;
            }
            break ;
        case DAT_LINE_COMMA :
            // a comma on a data line was seen
            if ( sc.atLineEnd ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = LINE_END ;
            } else if ( sc.ch == ' ' ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = DAT_LINE_COMMA ;
            } else if (sc.ch == ',')  {
                sc.SetState(SCE_ABAQUS_OPERATOR) ;
                state = DAT_LINE_COMMA ;
            } else if ( IsADigit(sc.ch) || (sc.ch == '-')|| (sc.ch == '.' && IsADigit(sc.chNext)) ) {
                sc.SetState(SCE_ABAQUS_NUMBER) ;
                state = DAT_LINE_VAL ;
            } else if ( IsAKeywordChar(sc.ch) ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = DAT_LINE_VAL ;
            } else if ( (sc.ch == '\'') || (sc.ch == '\"') ) {
                sc.SetState(SCE_ABAQUS_STRING) ;
                state = DAT_LINE_VAL ;
            } else {
                sc.SetState(SCE_ABAQUS_PROCESSOR) ;
                state = ST_ERROR ;
            }
            break ;
        case COMMENT_LINE :
            if ( sc.atLineEnd ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = LINE_END ;
            }
            break ;
        case ST_ERROR :
            if ( sc.atLineEnd ) {
                sc.SetState(SCE_ABAQUS_DEFAULT) ;
                state = LINE_END ;
            }
            break ;
        case LINE_END :
            if ( sc.atLineEnd || sc.ch == ' ' ) {
                // nothing changes
                state = LINE_END ;
            } else if ( sc.ch == '*' ) {
                if ( sc.chNext == '*' ) {
                    state = COMMENT_LINE ;
                    sc.SetState(SCE_ABAQUS_COMMENT) ;
                } else {
                    state = KW_LINE_KW ;
                    sc.SetState(SCE_ABAQUS_STARCOMMAND) ;
                }
            } else {
                // it must be a data line, things are as if we are in DAT_LINE_COMMA
                if ( sc.ch == ',' ) {
                    sc.SetState(SCE_ABAQUS_OPERATOR) ;
                    state = DAT_LINE_COMMA ;
                } else if ( IsADigit(sc.ch) || (sc.ch == '-')|| (sc.ch == '.' && IsADigit(sc.chNext)) ) {
                    sc.SetState(SCE_ABAQUS_NUMBER) ;
                    state = DAT_LINE_VAL ;
                } else if ( IsAKeywordChar(sc.ch) ) {
                    sc.SetState(SCE_ABAQUS_DEFAULT) ;
                    state = DAT_LINE_VAL ;
                } else if ( (sc.ch == '\'') || (sc.ch == '\"') ) {
                    sc.SetState(SCE_ABAQUS_STRING) ;
                    state = DAT_LINE_VAL ;
                } else {
                    sc.SetState(SCE_ABAQUS_PROCESSOR) ;
                    state = ST_ERROR ;
                }
            }
            break ;
		  }
   }
   sc.Complete();
}

//------------------------------------------------------------------------------
// This copyied and modified from LexBasic.cxx
//------------------------------------------------------------------------------

/* Bits:
 * 1  - whitespace
 * 2  - operator
 * 4  - identifier
 * 8  - decimal digit
 * 16 - hex digit
 * 32 - bin digit
 */
static int character_classification[128] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  1,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  2,  0,  2,  2,  2,  2,  2,  2,  2,  6,  2,  2,  2,  10, 6,
    60, 60, 28, 28, 28, 28, 28, 28, 28, 28, 2,  2,  2,  2,  2,  2,
    2,  20, 20, 20, 20, 20, 20, 4,  4,  4,  4,  4,  4,  4,  4,  4,
    4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  4,
    2,  20, 20, 20, 20, 20, 20, 4,  4,  4,  4,  4,  4,  4,  4,  4,
    4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  2,  2,  2,  2,  0
};

static bool IsSpace(int c) {
	return c < 128 && (character_classification[c] & 1);
}

static bool IsIdentifier(int c) {
	return c < 128 && (character_classification[c] & 4);
}

static int LowerCase(int c)
{
	if (c >= 'A' && c <= 'Z')
		return 'a' + c - 'A';
	return c;
}

static int LineEnd(int line, Accessor &styler)
{
    const int docLines = styler.GetLine(styler.Length() - 1);  // Available last line
    int eol_pos ;
    // if the line is the last line, the eol_pos is styler.Length()
    // eol will contain a new line, or a virtual new line
    if ( docLines == line )
        eol_pos = styler.Length() ;
    else
        eol_pos = styler.LineStart(line + 1) - 1;
    return eol_pos ;
}

static int LineStart(int line, Accessor &styler)
{
    return styler.LineStart(line) ;
}

// LineType
//
// bits determines the line type
// 1  : data line
// 2  : only whitespace
// 3  : data line with only whitespace
// 4  : keyword line
// 5  : block open keyword line
// 6  : block close keyword line
// 7  : keyword line in error
// 8  : comment line
static int LineType(int line, Accessor &styler) {
    int pos = LineStart(line, styler) ;
    int eol_pos = LineEnd(line, styler) ;

    int c ;
    char ch = ' ';

    int i = pos ;
    while ( i < eol_pos ) {
        c = styler.SafeGetCharAt(i);
        ch = static_cast<char>(LowerCase(c));
        // We can say something as soon as no whitespace
        // was encountered
        if ( !IsSpace(c) )
            break ;
        i++ ;
    }

    if ( i >= eol_pos ) {
        // This is a whitespace line, currently
        // classifies as data line
        return 3 ;
    }

    if ( ch != '*' ) {
        // This is a data line
        return 1 ;
    }

    if ( i == eol_pos - 1 ) {
        // Only a single *, error but make keyword line
        return 4+3 ;
    }

    // This means we can have a second character
    // if that is also a * this means a comment
    // otherwise it is a keyword.
    c = styler.SafeGetCharAt(i+1);
    ch = static_cast<char>(LowerCase(c));
    if ( ch == '*' ) {
        return 8 ;
    }

    // At this point we know this is a keyword line
    // the character at position i is a *
    // it is not a comment line
    char word[256] ;
    int  wlen = 0;

    word[wlen] = '*' ;
	wlen++ ;

    i++ ;
    while ( (i < eol_pos) && (wlen < 255) ) {
        c = styler.SafeGetCharAt(i);
        ch = static_cast<char>(LowerCase(c));

        if ( (!IsSpace(c)) && (!IsIdentifier(c)) )
            break ;

        if ( IsIdentifier(c) ) {
            word[wlen] = ch ;
			wlen++ ;
		}

        i++ ;
    }

    word[wlen] = 0 ;

    // Make a comparison
	if ( !strcmp(word, "*step") ||
         !strcmp(word, "*part") ||
         !strcmp(word, "*instance") ||
         !strcmp(word, "*assembly")) {
       return 4+1 ;
    }

	if ( !strcmp(word, "*endstep") ||
         !strcmp(word, "*endpart") ||
         !strcmp(word, "*endinstance") ||
         !strcmp(word, "*endassembly")) {
       return 4+2 ;
    }

    return 4 ;
}

static void SafeSetLevel(int line, int level, Accessor &styler)
{
    if ( line < 0 )
        return ;

    int mask = ((~SC_FOLDLEVELHEADERFLAG) | (~SC_FOLDLEVELWHITEFLAG));

    if ( (level & mask) < 0 )
        return ;

    if ( styler.LevelAt(line) != level )
        styler.SetLevel(line, level) ;
}

static void FoldABAQUSDoc(unsigned int startPos, int length, int,
WordList *[], Accessor &styler) {
    int startLine = styler.GetLine(startPos) ;
    int endLine   = styler.GetLine(startPos+length-1) ;

    // bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
    // We want to deal with all the cases
    // To know the correct indentlevel, we need to look back to the
    // previous command line indentation level
	// order of formatting keyline datalines commentlines
    int beginData    = -1 ;
    int beginComment = -1 ;
    int prvKeyLine   = startLine ;
    int prvKeyLineTp =  0 ;

    // Scan until we find the previous keyword line
    // this will give us the level reference that we need
    while ( prvKeyLine > 0 ) {
        prvKeyLine-- ;
        prvKeyLineTp = LineType(prvKeyLine, styler) ;
        if ( prvKeyLineTp & 4 )
            break ;
    }

    // Determine the base line level of all lines following
    // the previous keyword
    // new keyword lines are placed on this level
    //if ( prvKeyLineTp & 4 ) {
    int level = styler.LevelAt(prvKeyLine) & ~SC_FOLDLEVELHEADERFLAG ;
    //}

    // uncomment line below if weird behaviour continues
    prvKeyLine = -1 ;

    // Now start scanning over the lines.
    for ( int line = startLine; line <= endLine; line++ ) {
        int lineType = LineType(line, styler) ;

        // Check for comment line
        if ( lineType == 8 ) {
            if ( beginComment < 0 ) {
                beginComment = line ;
			}
        }

        // Check for data line
        if ( (lineType == 1) || (lineType == 3) ) {
            if ( beginData < 0 ) {
                if ( beginComment >= 0 ) {
                    beginData = beginComment ;
                } else {
                    beginData = line ;
                }
            }
			beginComment = -1 ;
		}

        // Check for keywordline.
        // As soon as a keyword line is encountered, we can set the
        // levels of everything from the previous keyword line to this one
        if ( lineType & 4 ) {
            // this is a keyword, we can now place the previous keyword
            // all its data lines and the remainder

            // Write comments and data line
            if ( beginComment < 0 ) {
                beginComment = line ;
			}

            if ( beginData < 0 ) {
                beginData = beginComment ;
				if ( prvKeyLineTp != 5 )
					SafeSetLevel(prvKeyLine, level, styler) ;
				else
					SafeSetLevel(prvKeyLine, level | SC_FOLDLEVELHEADERFLAG, styler) ;
            } else {
                SafeSetLevel(prvKeyLine, level | SC_FOLDLEVELHEADERFLAG, styler) ;
            }

            int datLevel = level + 1 ;
			if ( !(prvKeyLineTp & 4) ) {
				datLevel = level ;
			}

            for ( int ll = beginData; ll < beginComment; ll++ )
                SafeSetLevel(ll, datLevel, styler) ;

            // The keyword we just found is going to be written at another level
            // if we have a type 5 and type 6
            if ( prvKeyLineTp == 5 ) {
                level += 1 ;
			}

            if ( prvKeyLineTp == 6 ) {
                level -= 1 ;
				if ( level < 0 ) {
					level = 0 ;
				}
            }

            for ( int lll = beginComment; lll < line; lll++ )
                SafeSetLevel(lll, level, styler) ;

            // wrap and reset
            beginComment = -1 ;
            beginData    = -1 ;
            prvKeyLine   = line ;
            prvKeyLineTp = lineType ;
        }

    }

    if ( beginComment < 0 ) {
        beginComment = endLine + 1 ;
    } else {
        // We need to find out whether this comment block is followed by
        // a data line or a keyword line
        const int docLines = styler.GetLine(styler.Length() - 1);

        for ( int line = endLine + 1; line <= docLines; line++ ) {
            int lineType = LineType(line, styler) ;

            if ( lineType != 8 ) {
				if ( !(lineType & 4) )  {
					beginComment = endLine + 1 ;
				}
                break ;
			}
        }
    }

    if ( beginData < 0 ) {
        beginData = beginComment ;
		if ( prvKeyLineTp != 5 )
			SafeSetLevel(prvKeyLine, level, styler) ;
		else
			SafeSetLevel(prvKeyLine, level | SC_FOLDLEVELHEADERFLAG, styler) ;
    } else {
        SafeSetLevel(prvKeyLine, level | SC_FOLDLEVELHEADERFLAG, styler) ;
    }

    int datLevel = level + 1 ;
	if ( !(prvKeyLineTp & 4) ) {
		datLevel = level ;
	}

    for ( int ll = beginData; ll < beginComment; ll++ )
        SafeSetLevel(ll, datLevel, styler) ;

	if ( prvKeyLineTp == 5 ) {
		level += 1 ;
	}

	if ( prvKeyLineTp == 6 ) {
		level -= 1 ;
	}
	for ( int m = beginComment; m <= endLine; m++ )
        SafeSetLevel(m, level, styler) ;
}

static const char * const abaqusWordListDesc[] = {
    "processors",
    "commands",
    "slashommands",
    "starcommands",
    "arguments",
    "functions",
    0
};

LexerModule lmAbaqus(SCLEX_ABAQUS, ColouriseABAQUSDoc, "abaqus", FoldABAQUSDoc, abaqusWordListDesc);
