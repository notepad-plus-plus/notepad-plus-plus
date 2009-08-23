// Scintilla source code edit control
/** @file LexCmake.cxx
 ** Lexer for Cmake
 **/
// Copyright 2007 by Cristian Adam <cristian [dot] adam [at] gmx [dot] net>
// based on the NSIS lexer
// The License.txt file describes the conditions under which this software may be distributed.
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "CharClassify.h"
#include "PropSet.h"
#include "Accessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static bool isCmakeNumber(char ch)
{
    return(ch >= '0' && ch <= '9');
}

static bool isCmakeChar(char ch)
{
    return(ch == '.' ) || (ch == '_' ) || isCmakeNumber(ch) || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

static bool isCmakeLetter(char ch)
{
    return(ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

static bool CmakeNextLineHasElse(unsigned int start, unsigned int end, Accessor &styler)
{
    int nNextLine = -1;
    for ( unsigned int i = start; i < end; i++ ) {
        char cNext = styler.SafeGetCharAt( i );
        if ( cNext == '\n' ) {
            nNextLine = i+1;
            break;
        }
    }

    if ( nNextLine == -1 ) // We never foudn the next line...
        return false;

    for ( unsigned int firstChar = nNextLine; firstChar < end; firstChar++ ) {
        char cNext = styler.SafeGetCharAt( firstChar );
        if ( cNext == ' ' )
            continue;
        if ( cNext == '\t' )
            continue;
        if ( styler.Match(firstChar, "ELSE")  || styler.Match(firstChar, "else"))
            return true;
        break;
    }

    return false;
}

static int calculateFoldCmake(unsigned int start, unsigned int end, int foldlevel, Accessor &styler, bool bElse)
{
    // If the word is too long, it is not what we are looking for
    if ( end - start > 20 )
        return foldlevel;

    int newFoldlevel = foldlevel;

    char s[20]; // The key word we are looking for has atmost 13 characters
    for (unsigned int i = 0; i < end - start + 1 && i < 19; i++) {
        s[i] = static_cast<char>( styler[ start + i ] );
        s[i + 1] = '\0';
    }

    if ( CompareCaseInsensitive(s, "IF") == 0 || CompareCaseInsensitive(s, "WHILE") == 0
         || CompareCaseInsensitive(s, "MACRO") == 0 || CompareCaseInsensitive(s, "FOREACH") == 0
         || CompareCaseInsensitive(s, "ELSEIF") == 0 )
        newFoldlevel++;
    else if ( CompareCaseInsensitive(s, "ENDIF") == 0 || CompareCaseInsensitive(s, "ENDWHILE") == 0
              || CompareCaseInsensitive(s, "ENDMACRO") == 0 || CompareCaseInsensitive(s, "ENDFOREACH") == 0)
        newFoldlevel--;
    else if ( bElse && CompareCaseInsensitive(s, "ELSEIF") == 0 )
        newFoldlevel++;
    else if ( bElse && CompareCaseInsensitive(s, "ELSE") == 0 )
        newFoldlevel++;

    return newFoldlevel;
}

static int classifyWordCmake(unsigned int start, unsigned int end, WordList *keywordLists[], Accessor &styler )
{
    char word[100] = {0};
    char lowercaseWord[100] = {0};

    WordList &Commands = *keywordLists[0];
    WordList &Parameters = *keywordLists[1];
    WordList &UserDefined = *keywordLists[2];

    for (unsigned int i = 0; i < end - start + 1 && i < 99; i++) {
        word[i] = static_cast<char>( styler[ start + i ] );
        lowercaseWord[i] = static_cast<char>(tolower(word[i]));
    }

    // Check for special words...
    if ( CompareCaseInsensitive(word, "MACRO") == 0 || CompareCaseInsensitive(word, "ENDMACRO") == 0 )
        return SCE_CMAKE_MACRODEF;

    if ( CompareCaseInsensitive(word, "IF") == 0 ||  CompareCaseInsensitive(word, "ENDIF") == 0 )
        return SCE_CMAKE_IFDEFINEDEF;

    if ( CompareCaseInsensitive(word, "ELSEIF") == 0  || CompareCaseInsensitive(word, "ELSE") == 0 )
        return SCE_CMAKE_IFDEFINEDEF;

    if ( CompareCaseInsensitive(word, "WHILE") == 0 || CompareCaseInsensitive(word, "ENDWHILE") == 0)
        return SCE_CMAKE_WHILEDEF;

    if ( CompareCaseInsensitive(word, "FOREACH") == 0 || CompareCaseInsensitive(word, "ENDFOREACH") == 0)
        return SCE_CMAKE_FOREACHDEF;

    if ( Commands.InList(lowercaseWord) )
        return SCE_CMAKE_COMMANDS;

    if ( Parameters.InList(word) )
        return SCE_CMAKE_PARAMETERS;


    if ( UserDefined.InList(word) )
        return SCE_CMAKE_USERDEFINED;

    if ( strlen(word) > 3 ) {
        if ( word[1] == '{' && word[strlen(word)-1] == '}' )
            return SCE_CMAKE_VARIABLE;
    }

    // To check for numbers
    if ( isCmakeNumber( word[0] ) ) {
        bool bHasSimpleCmakeNumber = true;
        for (unsigned int j = 1; j < end - start + 1 && j < 99; j++) {
            if ( !isCmakeNumber( word[j] ) ) {
                bHasSimpleCmakeNumber = false;
                break;
            }
        }

        if ( bHasSimpleCmakeNumber )
            return SCE_CMAKE_NUMBER;
    }

    return SCE_CMAKE_DEFAULT;
}

static void ColouriseCmakeDoc(unsigned int startPos, int length, int, WordList *keywordLists[], Accessor &styler)
{
    int state = SCE_CMAKE_DEFAULT;
    if ( startPos > 0 )
        state = styler.StyleAt(startPos-1); // Use the style from the previous line, usually default, but could be commentbox

    styler.StartAt( startPos );
    styler.GetLine( startPos );

    unsigned int nLengthDoc = startPos + length;
    styler.StartSegment( startPos );

    char cCurrChar;
    bool bVarInString = false;
    bool bClassicVarInString = false;

    unsigned int i;
    for ( i = startPos; i < nLengthDoc; i++ ) {
        cCurrChar = styler.SafeGetCharAt( i );
        char cNextChar = styler.SafeGetCharAt(i+1);

        switch (state) {
        case SCE_CMAKE_DEFAULT:
            if ( cCurrChar == '#' ) { // we have a comment line
                styler.ColourTo(i-1, state );
                state = SCE_CMAKE_COMMENT;
                break;
            }
            if ( cCurrChar == '"' ) {
                styler.ColourTo(i-1, state );
                state = SCE_CMAKE_STRINGDQ;
                bVarInString = false;
                bClassicVarInString = false;
                break;
            }
            if ( cCurrChar == '\'' ) {
                styler.ColourTo(i-1, state );
                state = SCE_CMAKE_STRINGRQ;
                bVarInString = false;
                bClassicVarInString = false;
                break;
            }
            if ( cCurrChar == '`' ) {
                styler.ColourTo(i-1, state );
                state = SCE_CMAKE_STRINGLQ;
                bVarInString = false;
                bClassicVarInString = false;
                break;
            }

            // CMake Variable
            if ( cCurrChar == '$' || isCmakeChar(cCurrChar)) {
                styler.ColourTo(i-1,state);
                state = SCE_CMAKE_VARIABLE;

                // If it is a number, we must check and set style here first...
                if ( isCmakeNumber(cCurrChar) && (cNextChar == '\t' || cNextChar == ' ' || cNextChar == '\r' || cNextChar == '\n' ) )
                    styler.ColourTo( i, SCE_CMAKE_NUMBER);

                break;
            }

            break;
        case SCE_CMAKE_COMMENT:
            if ( cNextChar == '\n' || cNextChar == '\r' ) {
                // Special case:
                if ( cCurrChar == '\\' ) {
                    styler.ColourTo(i-2,state);
                    styler.ColourTo(i,SCE_CMAKE_DEFAULT);
                }
                else {
                    styler.ColourTo(i,state);
                    state = SCE_CMAKE_DEFAULT;
                }
            }
            break;
        case SCE_CMAKE_STRINGDQ:
        case SCE_CMAKE_STRINGLQ:
        case SCE_CMAKE_STRINGRQ:

            if ( styler.SafeGetCharAt(i-1) == '\\' && styler.SafeGetCharAt(i-2) == '$' )
                break; // Ignore the next character, even if it is a quote of some sort

            if ( cCurrChar == '"' && state == SCE_CMAKE_STRINGDQ ) {
                styler.ColourTo(i,state);
                state = SCE_CMAKE_DEFAULT;
                break;
            }

            if ( cCurrChar == '`' && state == SCE_CMAKE_STRINGLQ ) {
                styler.ColourTo(i,state);
                state = SCE_CMAKE_DEFAULT;
                break;
            }

            if ( cCurrChar == '\'' && state == SCE_CMAKE_STRINGRQ ) {
                styler.ColourTo(i,state);
                state = SCE_CMAKE_DEFAULT;
                break;
            }

            if ( cNextChar == '\r' || cNextChar == '\n' ) {
                int nCurLine = styler.GetLine(i+1);
                int nBack = i;
                // We need to check if the previous line has a \ in it...
                bool bNextLine = false;

                while ( nBack > 0 ) {
                    if ( styler.GetLine(nBack) != nCurLine )
                        break;

                    char cTemp = styler.SafeGetCharAt(nBack, 'a'); // Letter 'a' is safe here

                    if ( cTemp == '\\' ) {
                        bNextLine = true;
                        break;
                    }
                    if ( cTemp != '\r' && cTemp != '\n' && cTemp != '\t' && cTemp != ' ' )
                        break;

                    nBack--;
                }

                if ( bNextLine ) {
                    styler.ColourTo(i+1,state);
                }
                if ( bNextLine == false ) {
                    styler.ColourTo(i,state);
                    state = SCE_CMAKE_DEFAULT;
                }
            }
            break;

        case SCE_CMAKE_VARIABLE:

            // CMake Variable:
            if ( cCurrChar == '$' )
                state = SCE_CMAKE_DEFAULT;
            else if ( cCurrChar == '\\' && (cNextChar == 'n' || cNextChar == 'r' || cNextChar == 't' ) )
                state = SCE_CMAKE_DEFAULT;
            else if ( (isCmakeChar(cCurrChar) && !isCmakeChar( cNextChar) && cNextChar != '}') || cCurrChar == '}' ) {
                state = classifyWordCmake( styler.GetStartSegment(), i, keywordLists, styler );
                styler.ColourTo( i, state);
                state = SCE_CMAKE_DEFAULT;
            }
            else if ( !isCmakeChar( cCurrChar ) && cCurrChar != '{' && cCurrChar != '}' ) {
                if ( classifyWordCmake( styler.GetStartSegment(), i-1, keywordLists, styler) == SCE_CMAKE_NUMBER )
                    styler.ColourTo( i-1, SCE_CMAKE_NUMBER );

                state = SCE_CMAKE_DEFAULT;

                if ( cCurrChar == '"' ) {
                    state = SCE_CMAKE_STRINGDQ;
                    bVarInString = false;
                    bClassicVarInString = false;
                }
                else if ( cCurrChar == '`' ) {
                    state = SCE_CMAKE_STRINGLQ;
                    bVarInString = false;
                    bClassicVarInString = false;
                }
                else if ( cCurrChar == '\'' ) {
                    state = SCE_CMAKE_STRINGRQ;
                    bVarInString = false;
                    bClassicVarInString = false;
                }
                else if ( cCurrChar == '#' ) {
                    state = SCE_CMAKE_COMMENT;
                }
            }
            break;
        }

        if ( state == SCE_CMAKE_COMMENT) {
            styler.ColourTo(i,state);
        }
        else if ( state == SCE_CMAKE_STRINGDQ || state == SCE_CMAKE_STRINGLQ || state == SCE_CMAKE_STRINGRQ ) {
            bool bIngoreNextDollarSign = false;

            if ( bVarInString && cCurrChar == '$' ) {
                bVarInString = false;
                bIngoreNextDollarSign = true;
            }
            else if ( bVarInString && cCurrChar == '\\' && (cNextChar == 'n' || cNextChar == 'r' || cNextChar == 't' || cNextChar == '"' || cNextChar == '`' || cNextChar == '\'' ) ) {
                styler.ColourTo( i+1, SCE_CMAKE_STRINGVAR);
                bVarInString = false;
                bIngoreNextDollarSign = false;
            }

            else if ( bVarInString && !isCmakeChar(cNextChar) ) {
                int nWordState = classifyWordCmake( styler.GetStartSegment(), i, keywordLists, styler);
                if ( nWordState == SCE_CMAKE_VARIABLE )
                    styler.ColourTo( i, SCE_CMAKE_STRINGVAR);
                bVarInString = false;
            }
            // Covers "${TEST}..."
            else if ( bClassicVarInString && cNextChar == '}' ) {
                styler.ColourTo( i+1, SCE_CMAKE_STRINGVAR);
                bClassicVarInString = false;
            }

            // Start of var in string
            if ( !bIngoreNextDollarSign && cCurrChar == '$' && cNextChar == '{' ) {
                styler.ColourTo( i-1, state);
                bClassicVarInString = true;
                bVarInString = false;
            }
            else if ( !bIngoreNextDollarSign && cCurrChar == '$' ) {
                styler.ColourTo( i-1, state);
                bVarInString = true;
                bClassicVarInString = false;
            }
        }
    }

    // Colourise remaining document
    styler.ColourTo(nLengthDoc-1,state);
}

static void FoldCmakeDoc(unsigned int startPos, int length, int, WordList *[], Accessor &styler)
{
    // No folding enabled, no reason to continue...
    if ( styler.GetPropertyInt("fold") == 0 )
        return;

    bool foldAtElse = styler.GetPropertyInt("fold.at.else", 0) == 1;

    int lineCurrent = styler.GetLine(startPos);
    unsigned int safeStartPos = styler.LineStart( lineCurrent );

    bool bArg1 = true;
    int nWordStart = -1;

    int levelCurrent = SC_FOLDLEVELBASE;
    if (lineCurrent > 0)
        levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
    int levelNext = levelCurrent;

    for (unsigned int i = safeStartPos; i < startPos + length; i++) {
        char chCurr = styler.SafeGetCharAt(i);

        if ( bArg1 ) {
            if ( nWordStart == -1 && (isCmakeLetter(chCurr)) ) {
                nWordStart = i;
            }
            else if ( isCmakeLetter(chCurr) == false && nWordStart > -1 ) {
                int newLevel = calculateFoldCmake( nWordStart, i-1, levelNext, styler, foldAtElse);

                if ( newLevel == levelNext ) {
                    if ( foldAtElse ) {
                        if ( CmakeNextLineHasElse(i, startPos + length, styler) )
                            levelNext--;
                    }
                }
                else
                    levelNext = newLevel;
                bArg1 = false;
            }
        }

        if ( chCurr == '\n' ) {
            if ( bArg1 && foldAtElse) {
                if ( CmakeNextLineHasElse(i, startPos + length, styler) )
                    levelNext--;
            }

            // If we are on a new line...
            int levelUse = levelCurrent;
            int lev = levelUse | levelNext << 16;
            if (levelUse < levelNext )
                lev |= SC_FOLDLEVELHEADERFLAG;
            if (lev != styler.LevelAt(lineCurrent))
                styler.SetLevel(lineCurrent, lev);

            lineCurrent++;
            levelCurrent = levelNext;
            bArg1 = true; // New line, lets look at first argument again
            nWordStart = -1;
        }
    }

    int levelUse = levelCurrent;
    int lev = levelUse | levelNext << 16;
    if (levelUse < levelNext)
        lev |= SC_FOLDLEVELHEADERFLAG;
    if (lev != styler.LevelAt(lineCurrent))
        styler.SetLevel(lineCurrent, lev);
}

static const char * const cmakeWordLists[] = {
    "Commands",
    "Parameters",
    "UserDefined",
    0,
    0,};

LexerModule lmCmake(SCLEX_CMAKE, ColouriseCmakeDoc, "cmake", FoldCmakeDoc, cmakeWordLists);
