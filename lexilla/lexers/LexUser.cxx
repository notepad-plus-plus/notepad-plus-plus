/*------------------------------------------------------------------------------------
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
----------------------------------------------------------------------------------------*/

#include <string>
#include <map>
#include <vector>
#include <assert.h>
#include <windows.h>

#include "ILexer.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "WordList.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "LexerModule.h"
#include "PropSetSimple.h"

using namespace Lexilla;

#define CL_CURRENT  0x1
#define CL_PREV     0x2
#define CL_PREVPREV 0x4

#define FOLD_NONE   0
#define FOLD_OPEN   1
#define FOLD_MIDDLE 2
#define FOLD_CLOSE  3

#define COMMENTLINE_NO              0
#define COMMENTLINE_YES             1
#define COMMENTLINE_SKIP_TESTING    2

#define SEPARATOR_DOT      0
#define SEPARATOR_COMMA    1
#define SEPARATOR_BOTH     2

#define NI_OPEN     0
#define NI_CLOSE    1

#define NO_DELIMITER                    0
#define FORWARD_WHITESPACE_FOUND        1
#define FORWARD_KEYWORD_FOUND           2

#define SC_ISCOMMENTLINE      0x8000
#define MULTI_PART_LIMIT      100

#define PURE_LC_NONE    0   // must be in synch with the same values in PowerEditor/src/Parameters.h
#define PURE_LC_BOL     1
#define PURE_LC_WSP     2

#define EOL_DEFAULT_VALUE   0
#define EOL_SKIP_CHECK      1
#define EOL_FORCE_CHECK     2

#define MAPPER_TOTAL 15
#define FW_VECTORS_TOTAL SCE_USER_TOTAL_DELIMITERS + 9

const int maskMapper[MAPPER_TOTAL] =
{
    SCE_USER_MASK_NESTING_OPERATORS2,
    SCE_USER_MASK_NESTING_FOLDERS_IN_CODE2_OPEN,
    SCE_USER_MASK_NESTING_FOLDERS_IN_CODE2_MIDDLE,
    SCE_USER_MASK_NESTING_FOLDERS_IN_CODE2_CLOSE,
    SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_OPEN,
    SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_MIDDLE,
    SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_CLOSE,
    SCE_USER_MASK_NESTING_KEYWORD1,
    SCE_USER_MASK_NESTING_KEYWORD2,
    SCE_USER_MASK_NESTING_KEYWORD3,
    SCE_USER_MASK_NESTING_KEYWORD4,
    SCE_USER_MASK_NESTING_KEYWORD5,
    SCE_USER_MASK_NESTING_KEYWORD6,
    SCE_USER_MASK_NESTING_KEYWORD7,
    SCE_USER_MASK_NESTING_KEYWORD8
};

const int styleMapper[MAPPER_TOTAL] =
{
    SCE_USER_STYLE_OPERATOR,
    SCE_USER_STYLE_FOLDER_IN_CODE2,
    SCE_USER_STYLE_FOLDER_IN_CODE2,
    SCE_USER_STYLE_FOLDER_IN_CODE2,
    SCE_USER_STYLE_FOLDER_IN_COMMENT,
    SCE_USER_STYLE_FOLDER_IN_COMMENT,
    SCE_USER_STYLE_FOLDER_IN_COMMENT,
    SCE_USER_STYLE_KEYWORD1,
    SCE_USER_STYLE_KEYWORD2,
    SCE_USER_STYLE_KEYWORD3,
    SCE_USER_STYLE_KEYWORD4,
    SCE_USER_STYLE_KEYWORD5,
    SCE_USER_STYLE_KEYWORD6,
    SCE_USER_STYLE_KEYWORD7,
    SCE_USER_STYLE_KEYWORD8
};

const int foldingtMapper[MAPPER_TOTAL] =
{
    FOLD_NONE,
    FOLD_OPEN,
    FOLD_MIDDLE,
    FOLD_CLOSE,
    FOLD_OPEN,
    FOLD_MIDDLE,
    FOLD_CLOSE,
    FOLD_NONE,
    FOLD_NONE,
    FOLD_NONE,
    FOLD_NONE,
    FOLD_NONE,
    FOLD_NONE,
    FOLD_NONE,
    FOLD_NONE
};

using namespace std;
typedef vector<vector<string>> vvstring;

// static vector<int> * foldVectorStatic;  // foldVectorStatic is used for debugging only, it should be commented out in production code !

struct forwardStruct
{
    vvstring * _vec;
    int _sceID;
    int _maskID;

    forwardStruct(): _vec(0), _sceID(0), _maskID(0) {};    // constructor, useless but obligatory

    forwardStruct * Set (vvstring * vec, int sceID, int maskID) {
        _vec = vec;
        _sceID = sceID;
        _maskID = maskID;
        return this;
    }

}FWS;   // just one instance

struct nestedInfo {
    size_t _position;
    int _nestedLevel;
    int _index;
    int _state;
    int _opener;

    // constructor, useless but obligatory
    nestedInfo():_position(0), _nestedLevel(0), _index(0), _state(0), _opener(0) {};

    nestedInfo * Set (size_t position, int nestedLevel, int index, int state, int opener) {
        _position = position;
        _nestedLevel = nestedLevel;
        _index = index;
        _state = state;
        _opener = opener;
        return this;
    }
};
static nestedInfo NI;   // also just one instance

struct udlKeywordsMapStruct
{
    vvstring commentLineOpen, commentLineContinue, commentLineClose;
    vvstring commentOpen, commentClose;
    vvstring delim1Open, delim1Escape, delim1Close;
    vvstring delim2Open, delim2Escape, delim2Close;
    vvstring delim3Open, delim3Escape, delim3Close;
    vvstring delim4Open, delim4Escape, delim4Close;
    vvstring delim5Open, delim5Escape, delim5Close;
    vvstring delim6Open, delim6Escape, delim6Close;
    vvstring delim7Open, delim7Escape, delim7Close;
    vvstring delim8Open, delim8Escape, delim8Close;
    vvstring operators1;
    vvstring foldersInCode1Open, foldersInCode1Middle, foldersInCode1Close;
    vvstring foldersInCode2Open, foldersInCode2Middle, foldersInCode2Close;
    vector<string> prefixTokens1;
    vector<string> prefixTokens2;
    vector<string> suffixTokens1;
    vector<string> suffixTokens2;
    vector<string> extrasTokens1;
    vector<string> extrasTokens2;
    vector<string> rangeTokens;
    vector<string> negativePrefixTokens1;
    vector<string> negativePrefixTokens2;
    vector<string> negativeExtrasTokens2;
};

// key value is of type "int" so it could receive pointer value !!
// UDL name is defined as "const char *" in UserLangContainer class
// so, map will use pointer value (not value pointed to!) as the key
typedef map<int, udlKeywordsMapStruct> udlMapType;
static udlMapType udlKeywordsMap;

// key value is of type "int" so it could receive pointer value !!
// currentBufferID is defined as "Buffer *" in ScintillaEditView class
// so, map will use pointer value (not value pointed to!) as the key
typedef map<int, vector<nestedInfo> > nestedMapType;
static nestedMapType nestedMap;

static inline bool isWhiteSpace(const int ch)
{
    return (ch > 0 && ch < 0x21);
}

static inline bool isWhiteSpace2(unsigned char ch, int & nlCount, unsigned char excludeNewLine=0, unsigned char chNext=0)
{
    // multi-part keywords come in two flavors:
    // 1. "else if" (internally mapped to "else\vif") where '\v' can be replaced by spaces, tabs and new lines
    // 2. 'else if" (internally mapped to "else\bif") where '\b' can be replaced by spaces, tabs but not new lines
    // 'excludeNewLine' parameter is used to differentiate the two

    if ((ch == '\n') || (ch == '\r' && chNext != '\n'))
        ++nlCount;

    if (excludeNewLine == '\b')
        return (ch == ' ') || (ch == '\t');
    else
        return isWhiteSpace(ch);
}

static bool isInListForward2(vvstring * fwEndVectors[], int totalVectors, StyleContext & sc, bool ignoreCase, int forward)
{
    // forward check for multi-part keywords and numbers
    // this is differnt from 'isInListForward' function because
    // search for keyword is not performed at sc.currentPos but rather
    // at some position forward of sc.currentPos

    vvstring::iterator iter1;// = openVector.begin();
    vector<string>::iterator iter2;
    string::iterator iter3;
    int index = 0;
    int a = 0;
    int b = 0;

    for (int i=0; i<totalVectors; ++i)
    {
        if (fwEndVectors[i] && !fwEndVectors[i]->empty())
        {
            index = 0;
            a = 0;
            b = 0;

            for (iter1 = fwEndVectors[i]->begin(); iter1 != fwEndVectors[i]->end(); ++iter1)
            {
                iter2 = iter1->begin();
                for (; iter2 != iter1->end(); ++iter2)
                {
                    iter3 = iter2->begin();
                    index = 0;
                    for (; ; ++iter3)
                    {
                        a = ignoreCase?toupper(*iter3):*iter3;
                        b = ignoreCase?toupper(sc.GetRelative(forward + index++)):sc.GetRelative(forward + index++);
                        if (a != b)
                            break;
                        if (iter3 != iter2->end())
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

static bool isInListForward3(vector<string> * tokens, StyleContext & sc, bool ignoreCase, Sci_Position offset, size_t & moveForward)
{
    // forward check for vector<string> keywords, with offset

    moveForward = 0;

    unsigned char a = 0;
    unsigned char b = 0;
    Sci_Position indexb = 0;
    bool isFound = false;

    vector<string>::iterator iter1;
    string::iterator iter2;

    for (iter1 = tokens->begin(); iter1 != tokens->end(); ++iter1)
    {
        a = 0;
        b = 0;
        indexb = 0;
        isFound = true;
        for (iter2 = iter1->begin(); iter2 != iter1->end(); ++iter2)
        {
            a = static_cast<unsigned char>(ignoreCase?toupper(*iter2):*iter2);
            b = static_cast<unsigned char>(ignoreCase?toupper(sc.GetRelative(offset + indexb++)):sc.GetRelative(offset + indexb++));
            if (a != b)
            {
                isFound = false;
                break;
            }
        }
        if (isFound == true)
        {
            moveForward += iter1->length();
            break;
        }
    }

    return isFound;
}

static inline bool IsADigit(char ch)
{
    return isascii(ch) && isdigit(ch);
}

static bool IsNumber(StyleContext & sc, vector<string> * numberTokens[], vvstring * fwEndVectors[],
                     bool ignoreCase, int  decSeparator, size_t & moveForward )
{
    moveForward = 0;

    bool hasDot = false;
    bool hasPrefix1 = false;
    bool hasPrefix2 = false;
    bool hasSuffix1 = false;
    bool hasSuffix2 = false;
    bool hasExtras2 = false;
    bool hasRange = false;
    bool hasExp = false;
    bool previousWasRange = false;
    Sci_Position offset = 0;

    vector<string> * prefixTokens1          = numberTokens[0];
    vector<string> * prefixTokens2          = numberTokens[1];
    vector<string> * extrasTokens1          = numberTokens[2];
    vector<string> * extrasTokens2          = numberTokens[3];
    vector<string> * suffixTokens1          = numberTokens[4];
    vector<string> * suffixTokens2          = numberTokens[5];
    vector<string> * rangeTokens            = numberTokens[6];
    vector<string> * negativePrefixTokens1  = numberTokens[7];
    vector<string> * negativePrefixTokens2  = numberTokens[8];
    vector<string> * negativeExtrasTokens2  = numberTokens[9];

    // treat .1234 as correct number sequence
    if (((decSeparator == SEPARATOR_BOTH || decSeparator == SEPARATOR_DOT) && sc.ch == '.') ||
        ((decSeparator == SEPARATOR_BOTH || decSeparator == SEPARATOR_COMMA) && sc.ch == ','))
    {
        if (IsADigit(sc.chNext))
        {
            hasDot = true;
            offset = 2;
        }
    }
    else
    {
        // or is it a prefixed number?
        vector<string>::iterator iter = prefixTokens2->begin();
        vector<string>::iterator last = prefixTokens2->end();
        if (sc.ch == '-')
        {
            iter = negativePrefixTokens2->begin();
            last = negativePrefixTokens2->end();
        }
        for (; iter != last; ++iter)
        {
            if (ignoreCase?sc.MatchIgnoreCase2(iter->c_str()) : sc.Match(iter->c_str()))
                break;
        }
        if (iter != last)
        {
            // prefix2 is styled as number only if followed by an actual number or NBR_EXTRA_CHAR
            size_t skipForward = 0;

            if (isInListForward3(extrasTokens1, sc, ignoreCase, iter->length(), skipForward))
            {
                offset += iter->length() + skipForward;
                hasPrefix2 = true;
                hasExp = true;  // can't be a scientific E notation
            }
            else if (IsADigit(sc.GetRelative(iter->length())))
            {
                offset += iter->length() + 1;
                hasPrefix2 = true;
                hasExp = true;  // can't be a scientific E notation
            }
        }

        if (hasPrefix2 == false)
        {
            // or is it a prefixed1 number?
            vector<string>::iterator iter = prefixTokens1->begin();
            vector<string>::iterator last = prefixTokens1->end();
            if (sc.ch == '-')
            {
                iter = negativePrefixTokens1->begin();
                last = negativePrefixTokens1->end();
            }
            for (; iter != last; ++iter)
            {
                if (ignoreCase?sc.MatchIgnoreCase2(iter->c_str()) : sc.Match(iter->c_str()))
                    break;
            }
            if (iter != last)
            {
                // prefix1 is styled as number only if followed by an actual number (decimal digit)
                if (IsADigit(sc.GetRelative(iter->length())))
                {
                    offset += iter->length() + 1;
                    hasPrefix1 = true;
                    hasPrefix2 = false;     // can't have any EXTRA_CHARs
                    hasExp = true;          // can't be a scientific E notation
                }
            }
        }
        if (hasPrefix1 == false && hasPrefix2 == false)
        {
            // or is it a suffixed1 number with extras2?
            vector<string>::iterator iter = extrasTokens2->begin();
            vector<string>::iterator last = extrasTokens2->end();

            if (sc.ch == '-')
            {
                iter = negativeExtrasTokens2->begin();
                last = negativeExtrasTokens2->end();
            }
            for (; iter != last; ++iter)
            {
                if (ignoreCase?sc.MatchIgnoreCase2(iter->c_str()) : sc.Match(iter->c_str()))
                    break;
            }
            if (iter != last)
            {
                offset += iter->length();
                hasExtras2 = true;
                hasExp = true;  // can't be a scientific E notation
            }
        }
    }
    // is it a simple digit?
    if (offset == 0)
    {
        if (IsADigit(sc.ch))
        {
            offset = 1;
        }
        // or prefixed simple digit?
        else if ((sc.ch == '-' || sc.ch == '+') && IsADigit(sc.chNext) && !IsADigit(sc.chPrev))
        {
            offset = 2;
        }

        if (offset == 0)
            return false;
    }

    size_t skipForward = 0;
    for (;;)
    {
        skipForward = 0;

        // if (isInListForward2(fwEndVectors, (*fwEndVectors)->size(), sc, ignoreCase, offset)  || isWhiteSpace(sc.GetRelative(offset)))
        if (isWhiteSpace(sc.GetRelative(offset)) || isInListForward2(fwEndVectors, 12, sc, ignoreCase, offset))
        {
            if (hasExtras2 == true && hasSuffix1 == false)
                return false;

            moveForward = offset;
            return true;    // yay, finally we have a number
        }

        if (hasRange == false)
        {
            if (isInListForward3(rangeTokens, sc, ignoreCase, offset, skipForward))
            {
                if (hasExtras2 == true && hasSuffix1 == false)
                    return false;

                offset += skipForward;
                hasSuffix1 = false;
                hasSuffix2 = false;
                hasDot = false;
                hasRange = true;
                hasExp = false;
                hasExtras2 = false;
                previousWasRange = true;
                continue;
            }
        }

        if (hasSuffix2 == true)  // only RANGE_CHARs are allowed after SUFFIX_CHARs
            return false;

        if (hasPrefix2 == true)
        {
            if (isInListForward3(extrasTokens1, sc, ignoreCase, offset, skipForward))
            {
                offset += skipForward;
                continue;
            }
        }

        if (hasSuffix1 == false && hasPrefix1 == false && hasPrefix2 == false)
        {
            if (isInListForward3(suffixTokens1, sc, ignoreCase, offset, skipForward))
            {
                offset += skipForward;
                hasExtras2 = false;
                hasSuffix1 = true;
                continue;
            }
            
            if (isInListForward3(extrasTokens2, sc, ignoreCase, offset, skipForward))
            {
                offset += skipForward;
                hasExtras2 = true;
                hasExp = true;  // can't be a scientific E notation
                continue;
            }
        }

        if (hasSuffix2 == false)
        {
            if (isInListForward3(suffixTokens2, sc, ignoreCase, offset, skipForward))
            {
                offset += skipForward;
                hasSuffix2 = true;
                continue;
            }
        }

        if (previousWasRange == true)   // prefix in the middle is an error case, so any number is treated as if it had a prefix
        {                               // the only acceptable position for prefix is immediatelly after range char, e.g. 0x10--0x15
            if (isInListForward3(prefixTokens2, sc, ignoreCase, offset, skipForward))
            {
                offset += skipForward;
                hasExp = false;
                hasPrefix2 = true;
                continue;
            }

            if (isInListForward3(prefixTokens1, sc, ignoreCase, offset, skipForward))
            {
                offset += skipForward;
                hasExp = false;
                hasPrefix1 = true;
                continue;
            }
        }

        if (IsADigit(sc.GetRelative(offset)))
        {
            offset += 1;
            continue;
        }

        if (hasDot == false)
        {
            // treat .1234 (or ,1234) as correct number sequence
            if (((decSeparator == SEPARATOR_BOTH || decSeparator == SEPARATOR_DOT) &&
                    (sc.GetRelative(offset) == '.'))
                    ||
                ((decSeparator == SEPARATOR_BOTH || decSeparator == SEPARATOR_COMMA) &&
                    (sc.GetRelative(offset) == ',')))
            {
                if (IsADigit(sc.GetRelative(offset + 1)))
                {
                    if (IsADigit(sc.GetRelative(offset - 1)) || previousWasRange == true)
                    {
                        offset += 2;
                        hasDot = true;
                        continue;
                    }
                }
            }
        }

        if (hasExp == false)
        {
            if (toupper(sc.GetRelative(offset)) == 'E') // treat E as scientific notation only if it does not match extra chars!!
            {
                unsigned char chPrev = sc.GetRelative(offset - 1);
                unsigned char chNext = sc.GetRelative(offset + 1);
                unsigned char chNextNext = sc.GetRelative(offset + 2);
                if (IsADigit(chPrev))
                {
                    int move = 0;
                    if (IsADigit(chNext))
                    {
                        move = 1;
                    }
                    else if ((chNext == '+' || chNext == '-') && IsADigit(chNextNext))
                    {
                        move = 2;
                    }

                    if (move > 0)
                    {
                        offset += move;
                        hasPrefix2 = false; // EXTRA_CHARs are not allowed in E notation
                        hasDot    = false;
                        hasExp    = true;
                        continue;
                    }
                }
            }
        }

        // not a number
        return false;
    }
}

static inline void SubGroup(const char * s, vvstring & vec, bool group=false)
{
    size_t length = strlen(s);
    char * temp = new char[length+1];
    unsigned int index = 0;
    vector<string> subvector;
    unsigned int i = 0;

    for (unsigned int j=0; j<length+1; ++j)
        temp[j] = 0;

    if (length >= 2 && s[0] == '(' && s[1]  == '(')
    {
        i = 2;
        group = true;
    }

	if (length >= 2 && s[length - 1] == ')' && s[length - 2] == ')')
        length -= 2;

    if (!group && *s)
    {
        subvector.push_back(s);
    }
    else
    {
        for (; i<length; ++i)
        {
            if (s[i] == ' ')
            {
                if (*temp)
                {
                    if (!strcmp(temp, "EOL"))
                    {
                        subvector.push_back("\r\n");
                        subvector.push_back("\n");
                        subvector.push_back("\r");
                    }
                    else
                        subvector.push_back(temp);

                    index = 0;
                    for (unsigned int j=0; j<length; ++j)
                        temp[j] = 0;
                }
            }
            else if (i == length-1)
            {
                temp[index++] = s[i];
                if (*temp)
                {
                    if (!strcmp(temp, "EOL"))
                    {
                        subvector.push_back("\r\n");
                        subvector.push_back("\n");
                        subvector.push_back("\r");
                    }
                    else
                        subvector.push_back(temp);
                }
            }
            else
            {
                temp[index++] = s[i];
            }
        }
    }

    if (!subvector.empty())
        vec.push_back(subvector);

    delete [] temp;
}

static inline void GenerateVector(vvstring & vec, const char * s, const char * prefix, size_t minLength)
{
    size_t length = strlen(s);
    char * temp = new char[length];
    unsigned int index = 0;
    bool copy = false;
    bool inGroup = false;

    for (unsigned int j=0; j<length; ++j)
        temp[j] = 0;

    vec.clear();
    for (unsigned int i=0; i<length; ++i)
    {
        if (copy && !inGroup && s[i] == ' ')
        {
            SubGroup(temp, vec, inGroup);
            index = 0;
            copy = false;
            for (unsigned int j=0; j<length; ++j)
                temp[j] = 0;
        }

        if ( (s[i] == ' ' && s[i+1] == prefix[0] && s[i+2] == prefix[1] && s[i+3] != ' ') ||
             (   i == 0   &&   s[0] == prefix[0] &&   s[1] == prefix[1] && s[i+2] != ' ') )
        {
            if (i > 0)  i += 1; // skip space
            i += 2;             // skip prefix
            copy = true;

            if (s[i] == ' ')
                continue;

            if (s[i] == '(' && s[i+1] == '(')
                inGroup = true;
        }

        if (inGroup && s[i] == ')' && s[i+1] == ')')
            inGroup = false;

        if (copy)
            temp[index++] = s[i];
    }

    if (length)
        SubGroup(temp, vec, inGroup);

    vector<string> emptyVector;
    for (size_t i = vec.size(); i < minLength; ++i)
    {
        vec.push_back(emptyVector);
    }

    delete [] temp;
}

static inline void StringToVector(char * original, vector<string> & tokenVector, bool negative=false)
{
    // this is rarely used, so I chose std::string for simplicity reasons
    // for better performance C-strings could be used

    string temp = "";
    char * pch = original;
    while (*pch != '\0')
    {
        if (*pch != ' ')
            temp += *pch;   //
        else if (temp.size() > 0)
        {
            if (negative)
                tokenVector.push_back("-" + temp);
            else
                tokenVector.push_back(temp);

            temp = "";
        }
        ++pch;
    }

    if (temp.size() > 0)
    {
        if (negative)
            tokenVector.push_back("-" + temp);
        else
            tokenVector.push_back(temp);
    }
}

static inline void ReColoringCheck(Sci_PositionU & startPos, int & nestedLevel, int & initStyle, int & openIndex,
                                   int & isCommentLine, bool & isInComment, Accessor & styler, vector<nestedInfo> & lastNestedGroup,
                                   vector<nestedInfo> & nestedVector, /* vector<int> & foldVector, */ int & continueCommentBlock)
{
    if (startPos > 0)
    {
        // re-coloring always starts at line beginning !!

        // special exception for multipart keywords
        initStyle = styler.StyleAt(startPos-1); // check style of previous new line character
        if ( (initStyle >= SCE_USER_STYLE_KEYWORD1 && initStyle < (SCE_USER_STYLE_KEYWORD1+SCE_USER_TOTAL_KEYWORD_GROUPS))    // keywords1-8
              || initStyle == SCE_USER_STYLE_FOLDER_IN_COMMENT
              || initStyle == SCE_USER_STYLE_FOLDER_IN_CODE2 )
        {
            // we are in middle of multi-part keyword that contains newline characters, go back until current style ends
            while (startPos > 0 && styler.StyleAt(--startPos) == initStyle);
        }
    }

    if (startPos > 0)
    {
        // go back until first EOL char
        char ch = 0;
        do
        {
            ch = styler.SafeGetCharAt(--startPos);
        }
        while(ch != '\r' && ch != '\n' && startPos > 0);

        if (startPos > 0)
            startPos += 1;  // compensate for decrement operation
    }

    if (startPos == 0)
    {
        // foldVector.clear();
        nestedVector.clear();
        lastNestedGroup.clear();
        initStyle = SCE_USER_STYLE_IDENTIFIER;
        return;
    }

    // clear all data on positions forward of 'startPos' as we
    // are about to re-color that part of the document.
    vector<nestedInfo>::iterator iter = nestedVector.begin();
    for (; iter != nestedVector.end(); ++iter)
    {
        if (iter->_position >= startPos)
        {
            nestedVector.erase(iter, nestedVector.end());
            break;
        }
    }

    if (!nestedVector.empty())
    {
        // go back to last nesting level '1' (or beginning of vector if no level '1' is found)
        iter = --nestedVector.end();
        lastNestedGroup.clear();
        while (iter->_nestedLevel > 1 && iter != nestedVector.begin())
            --iter;
    }
    else
    {
        iter = nestedVector.end();
    }

    // recreate lastNestedGroup, skip adjecent OPEN/CLOSE pairs
    // nesting group is something like:
    // "first delimiter 'nested delimiter 1 /*nested delimiter 2*/ delimiter 1 again' first delimiter again"
    // if user is editing somewhere inside this group, than 'lastNestedGroup' provides info about nesting
    // this is much more convinient that trying to obtain the same info from 'nestedVector'
    vector<nestedInfo>::iterator last;
    while (iter != nestedVector.end())
    {
        if (iter->_opener == NI_OPEN)
            lastNestedGroup.push_back(*iter);
        else if (iter->_opener == NI_CLOSE && !lastNestedGroup.empty())
        {
            last = --lastNestedGroup.end();
            if (last->_opener == NI_OPEN)
                if (last->_nestedLevel == iter->_nestedLevel)
                    if (last->_state == iter->_state)
                        if (last->_index == iter->_index)
                            lastNestedGroup.erase(last);
        }
        ++iter;
    }

    if (!lastNestedGroup.empty())
    {
        last = --lastNestedGroup.end();
        initStyle = last->_state;
        openIndex = last->_index;
        nestedLevel = last->_nestedLevel;

        // are we nested somewhere in comment?
        for (; ; --last)
        {
            if (last->_state == SCE_USER_STYLE_COMMENT)
            {
                isInComment = true;
                isCommentLine = COMMENTLINE_YES;
            }
            if (last->_state == SCE_USER_STYLE_COMMENTLINE)
            {
                isCommentLine = COMMENTLINE_YES;
            }
            if (last == lastNestedGroup.begin())
                break;
        }
    }
    else
    {
        initStyle = SCE_USER_STYLE_IDENTIFIER;
        openIndex = -1;
        nestedLevel = 0;
    }

    // are we in fold block of comment lines?
    int lineCurrent = styler.GetLine(startPos);

    if ((styler.LevelAt(lineCurrent) & SC_ISCOMMENTLINE) != 0)
        continueCommentBlock |= CL_CURRENT;

    if (lineCurrent >= 1)
        if ((styler.LevelAt(lineCurrent - 1) & SC_ISCOMMENTLINE) != 0)
            continueCommentBlock |= CL_PREV;

    if (lineCurrent >= 2)
        if ((styler.LevelAt(lineCurrent - 2) & SC_ISCOMMENTLINE) != 0)
            if (continueCommentBlock & CL_PREV)
                continueCommentBlock |= CL_PREVPREV;

    // foldVector.erase(foldVector.begin() + lineCurrent, foldVector.end());
}

static bool isInListForward(vvstring & openVector, StyleContext & sc, bool ignoreCase, int & openIndex, size_t & skipForward)
{
    // forward check for standard (sigle part) keywords

    skipForward = 0;
    vector<vector<string>>::iterator iter1 = openVector.begin();
    vector<string>::iterator iter2;

    for (; iter1 != openVector.end(); ++iter1)
    {
        iter2 = iter1->begin();
        for (; iter2 != iter1->end(); ++iter2)
        {
            if (ignoreCase?sc.MatchIgnoreCase2(iter2->c_str()):sc.Match(iter2->c_str()))
            {
                openIndex = iter1 - openVector.begin();
                skipForward = iter2->length();
                return true;
            }
        }
    }

    return false;
}

static bool isInListBackward(WordList & list, StyleContext & sc, bool specialMode, bool ignoreCase,
                             int & moveForward, vvstring * fwEndVectors[], int & nlCount, size_t docLength)
{
    // backward search
    // this function compares last identified 'word' (text surrounded by spaces or other forward keywords)
    // with all keywords within 'WordList' object

    // 'isInListBackward' can search for multi-part keywords too. Such keywords have variable length,
    // in case 'isInListBackward' finds such keywords it will set 'moveForward' parameter so algorythm could adjust position

    if (!list.Length())
        return false;

    moveForward = 0;

    int offset = -1 * sc.LengthCurrent();   // length of 'word' that needs to be investigated
    unsigned char a = 0;    // iterator for user defined keywords
    unsigned char b = 0;    // iterator for text in the file
    unsigned char bNext = 0;
    unsigned char wsChar = 0;
    unsigned char firstChar = sc.GetRelative(offset);
    int fwDelimiterFound = NO_DELIMITER;
    int nlCountTemp = 0;
    int indexa = 0;
    int indexb = 0;
    int i = list.StartAt(firstChar);
    bool doUpperLoop = ignoreCase;

    if (ignoreCase)
    {
        i = list.StartAt(tolower(firstChar));
        if (i == -1)
        {
            i = list.StartAt(toupper(firstChar));
            if (i == -1)
                return false;

            doUpperLoop = false;
        }
    }

    while (i >= 0)
    {
        while (static_cast<unsigned char>(ignoreCase?toupper(list.WordAt(i)[0]):list.WordAt(i)[0]) == (ignoreCase?toupper(firstChar):firstChar))
        {
            a = 0;
            b = 0;
            bNext = 0;
            indexa = 0;
            indexb = 0;
            wsChar = 0;
            fwDelimiterFound = NO_DELIMITER;

            do
            {
                a = static_cast<unsigned char>(ignoreCase?toupper(list.WordAt(i)[indexa++]):list.WordAt(i)[indexa++]);
                if (a == '\v' || a == '\b')
                {
                    wsChar = a;
                    b = sc.GetRelative(offset + indexb++);
                    bNext = sc.GetRelative(offset + indexb);
                    if (isWhiteSpace2(b, nlCountTemp, wsChar, bNext))
                    {
                        do {
                            b = sc.GetRelative(offset + indexb++);
                            bNext = sc.GetRelative(offset + indexb);
                        }
                        while((sc.currentPos + offset + indexb) <= docLength && isWhiteSpace2(b, nlCountTemp, wsChar, bNext));

                        a = static_cast<unsigned char>(ignoreCase?toupper(list.WordAt(i)[indexa++]):list.WordAt(i)[indexa++]);
                    }
                    b = ignoreCase?toupper(b):b;
                }
                else
                    b = ignoreCase?toupper(sc.GetRelative(offset + indexb++)):sc.GetRelative(offset + indexb++);
            }
            while (a && (a == b));

            if (!a)
            {
                --indexb;   // decrement indexb to compensate for comparing with '\0' in previous loop
                if (wsChar)
                {
                    // multi-part keyword is found,
                    // but it must be followed by whitespace (or 'forward' keyword)
                    // otherwise "else if" might wrongly match "else iff"
                    bNext = sc.GetRelative(indexb + offset);
                    if (isWhiteSpace(bNext))
                        fwDelimiterFound = FORWARD_WHITESPACE_FOUND;

                    if (fwDelimiterFound == NO_DELIMITER)
                    {
                        if (isInListForward2(fwEndVectors, FW_VECTORS_TOTAL, sc, ignoreCase, indexb + offset))
                        {
                            fwDelimiterFound = FORWARD_KEYWORD_FOUND;
                        }
                    }

                    // special case when multi-part keywords have 'prefix' option enabled
                    // then the next word in the text file must be treated as part of multi-part keyword
                    // e.g. prefixed "else if" matches "else if nextWord", but not "else iffy"
                    if (specialMode)
                    {
                        if (fwDelimiterFound == FORWARD_WHITESPACE_FOUND)    // there must be a white space !!
                        {
                            // skip whitespace (all of it)
                            int savedPosition = indexb;     // return here if whitespace is not followed by another word
                            for (;;)
                            {
                                if ((sc.currentPos + offset + indexb) > docLength)
                                    break;
                                if (!isWhiteSpace2(sc.GetRelative(offset + indexb), nlCountTemp, wsChar, sc.GetRelative(offset + indexb + 1)))
                                    break;
                                ++indexb;
                            }

                            // skip next "word" (if next word is not found, go back to end of multi-part keyword)
                            // it is not necessary to check EOF position here, because sc.GetRelative returns '\0' beyond EOF by default
                            // return ' ' on EOF instead to not loop indefinitely
                            bool nextWordFound = false;
                            while (!isWhiteSpace2(sc.GetRelative(indexb + offset, ' '), nlCountTemp, wsChar, sc.GetRelative(offset + indexb + 1, ' ')))
                            {
                                if (isInListForward2(fwEndVectors, FW_VECTORS_TOTAL, sc, ignoreCase, indexb + offset))
                                {
                                    break;
                                }
                                ++indexb;
                                nextWordFound = true;
                            }
                            if (nextWordFound == false)
                                indexb = savedPosition;
                        }
                    }
                }
                // keyword is read fully, decide if we can leave this function
                nlCount += nlCountTemp;
                moveForward = indexb + offset;  // offset is already negative

                if (wsChar)
                {
                    if (fwDelimiterFound != NO_DELIMITER)
                        return true;    // multi part keyword found
                }
                else if (moveForward == 0)
                    return true;    // single part keyword found
                else if (specialMode)
                    return true;    // prefixed single part keyword found
            }
            nlCountTemp = 0;
            ++i;
        }
        // run one more time for capital letter version
        if (doUpperLoop)
        {
            i = list.StartAt(toupper(firstChar));
            doUpperLoop = false;
        }
        else
            break;
    }

    return false;
}

static void setBackwards(WordList * kwLists[], StyleContext & sc, bool prefixes[], bool ignoreCase,
                         int nestedKey, vvstring * fwEndVectors[], int & levelMinCurrent,
                         int & levelNext, int & nlCount, bool & dontMove, size_t docLength)
{
    if (sc.LengthCurrent() == 0)
        return;

    int folding = FOLD_NONE;
    int moveForward = 0;

    for (int i=0; i<MAPPER_TOTAL; ++i)
    {
        if (nestedKey & maskMapper[i])
        {
            if (isInListBackward(*kwLists[i], sc, prefixes[i], ignoreCase, moveForward, fwEndVectors, nlCount, docLength))
            {
                folding = foldingtMapper[i];
                if (moveForward > 0)
                {
                    sc.Forward(moveForward);
                    dontMove = true;
                }
                sc.ChangeState(styleMapper[i]);
                break;
            }
        }
    }

    if (folding == FOLD_MIDDLE)
    {
        // treat middle point as a sequence of: FOLD_CLOSE followed by FOLD_OPEN
        levelNext--;
        folding = FOLD_OPEN;
    }

    if (folding == FOLD_OPEN)
    {
        if (levelMinCurrent > levelNext)
            levelMinCurrent = levelNext;
        levelNext++;
    }
    else if (folding == FOLD_CLOSE)
    {
        levelNext--;
    }
}

static bool isInListNested(int nestedKey, vector<forwardStruct> & forwards, StyleContext & sc,
                           bool ignoreCase, int & openIndex, size_t & skipForward, int & newState, int pureLC,
                           bool visibleChars, vector<string> * numberTokens[], vvstring ** numberDelims, int decSeparator)
{
    // check if some other delimiter is nested within current delimiter
    // all delimiters are freely checked but line comments must be synched with property 'pureLC'

    int backup = openIndex;
    vector<forwardStruct>::iterator iter = forwards.begin();

    for (; iter != forwards.end(); ++iter)
    {
        if (nestedKey & iter->_maskID)
        {
            if ((iter->_maskID != SCE_USER_MASK_NESTING_COMMENT_LINE) ||
                (iter->_maskID == SCE_USER_MASK_NESTING_COMMENT_LINE &&
                    ((pureLC == PURE_LC_NONE) ||
                     (pureLC == PURE_LC_BOL && (sc.chPrev == '\r' || sc.chPrev == '\n')) ||
                     (pureLC == PURE_LC_WSP && visibleChars == false))))

            {
                if (isInListForward(*(iter->_vec), sc, ignoreCase, openIndex, skipForward))
                {
                    newState = iter->_sceID;
                    return true;
                }
            }
        }
    }

    if (nestedKey & SCE_USER_MASK_NESTING_NUMBERS)
    {
        if (IsNumber(sc, numberTokens, numberDelims, ignoreCase, decSeparator, skipForward))
        {
            newState = SCE_USER_STYLE_NUMBER;
            return true;
        }
    }

    openIndex = backup;
    return false;
}

static void readLastNested(vector<nestedInfo> & lastNestedGroup, int & newState, int & openIndex)
{
    // after delimiter ends we need to determine whether we are entering some other delimiter (in case of nesting)
    // or do we simply start over from default style.

    newState = SCE_USER_STYLE_IDENTIFIER;
    openIndex = -1;
    if (!lastNestedGroup.empty())
    {
        lastNestedGroup.erase(lastNestedGroup.end()-1);
        if (!lastNestedGroup.empty())
        {
            newState = (--lastNestedGroup.end())->_state;
            openIndex = (--lastNestedGroup.end())->_index;
        }
    }
}

static void ColouriseUserDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *kwLists[], Accessor &styler)
{
    bool foldComments = styler.GetPropertyInt("userDefine.allowFoldOfComments", 0) != 0;
    bool ignoreCase   = styler.GetPropertyInt("userDefine.isCaseIgnored",       0) != 0;
    bool foldCompact  = styler.GetPropertyInt("userDefine.foldCompact",         0) != 0;

    int pureLC = styler.GetPropertyInt("userDefine.forcePureLC", 0);

    bool prefixes[MAPPER_TOTAL];

    for (int i=0; i<MAPPER_TOTAL; ++i)    // only KEYWORDS1-8 can be prefixed
        prefixes[i] = false;

    // positions are hardcoded and they must be in synch with positions in "styleMapper" array!!
    prefixes[7]  = styler.GetPropertyInt("userDefine.prefixKeywords1", 0) != 0;
    prefixes[8]  = styler.GetPropertyInt("userDefine.prefixKeywords2", 0) != 0;
    prefixes[9]  = styler.GetPropertyInt("userDefine.prefixKeywords3", 0) != 0;
    prefixes[10] = styler.GetPropertyInt("userDefine.prefixKeywords4", 0) != 0;
    prefixes[11] = styler.GetPropertyInt("userDefine.prefixKeywords5", 0) != 0;
    prefixes[12] = styler.GetPropertyInt("userDefine.prefixKeywords6", 0) != 0;
    prefixes[13] = styler.GetPropertyInt("userDefine.prefixKeywords7", 0) != 0;
    prefixes[14] = styler.GetPropertyInt("userDefine.prefixKeywords8", 0) != 0;

    char nestingBuffer[] = "userDefine.nesting.00";     // "00" is only a placeholder, the actual number is set by _itoa
    _itoa(SCE_USER_STYLE_COMMENT,       (nestingBuffer+20), 10);    int commentNesting      = styler.GetPropertyInt(nestingBuffer, 0);
    _itoa(SCE_USER_STYLE_COMMENTLINE,   (nestingBuffer+20), 10);    int lineCommentNesting  = styler.GetPropertyInt(nestingBuffer, 0);
    _itoa(SCE_USER_STYLE_DELIMITER1,    (nestingBuffer+19), 10);    int delim1Nesting       = styler.GetPropertyInt(nestingBuffer, 0);  // one byte difference
    _itoa(SCE_USER_STYLE_DELIMITER2,    (nestingBuffer+19), 10);    int delim2Nesting       = styler.GetPropertyInt(nestingBuffer, 0);  // for two-digit numbers
    _itoa(SCE_USER_STYLE_DELIMITER3,    (nestingBuffer+19), 10);    int delim3Nesting       = styler.GetPropertyInt(nestingBuffer, 0);
    _itoa(SCE_USER_STYLE_DELIMITER4,    (nestingBuffer+19), 10);    int delim4Nesting       = styler.GetPropertyInt(nestingBuffer, 0);
    _itoa(SCE_USER_STYLE_DELIMITER5,    (nestingBuffer+19), 10);    int delim5Nesting       = styler.GetPropertyInt(nestingBuffer, 0);
    _itoa(SCE_USER_STYLE_DELIMITER6,    (nestingBuffer+19), 10);    int delim6Nesting       = styler.GetPropertyInt(nestingBuffer, 0);
    _itoa(SCE_USER_STYLE_DELIMITER7,    (nestingBuffer+19), 10);    int delim7Nesting       = styler.GetPropertyInt(nestingBuffer, 0);
    _itoa(SCE_USER_STYLE_DELIMITER8,    (nestingBuffer+19), 10);    int delim8Nesting       = styler.GetPropertyInt(nestingBuffer, 0);

    commentNesting  |= SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_OPEN
                    |  SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_MIDDLE
                    |  SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_CLOSE;

    lineCommentNesting  |= SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_OPEN
                        |  SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_MIDDLE
                        |  SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_CLOSE;

    const int bwNesting = SCE_USER_MASK_NESTING_KEYWORD1
                        | SCE_USER_MASK_NESTING_KEYWORD2
                        | SCE_USER_MASK_NESTING_KEYWORD3
                        | SCE_USER_MASK_NESTING_KEYWORD4
                        | SCE_USER_MASK_NESTING_KEYWORD5
                        | SCE_USER_MASK_NESTING_KEYWORD6
                        | SCE_USER_MASK_NESTING_KEYWORD7
                        | SCE_USER_MASK_NESTING_KEYWORD8
                        | SCE_USER_MASK_NESTING_OPERATORS2
                        | SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_OPEN
                        | SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_MIDDLE
                        | SCE_USER_MASK_NESTING_FOLDERS_IN_COMMENT_CLOSE
                        | SCE_USER_MASK_NESTING_FOLDERS_IN_CODE2_OPEN
                        | SCE_USER_MASK_NESTING_FOLDERS_IN_CODE2_MIDDLE
                        | SCE_USER_MASK_NESTING_FOLDERS_IN_CODE2_CLOSE;

    // creation of vvstring (short for vector<vector<string>>) objects is expensive,
    // therefore these objects are created only at beginning of file, and saved to
    // global std::map objects udlKeywordsMap and nestedMap

    int currentBufferID = styler.GetPropertyInt("userDefine.currentBufferID", 0);
    if (nestedMap.find(currentBufferID) == nestedMap.end())
    {
        nestedMap[currentBufferID] = vector<nestedInfo>();
    }
    vector<nestedInfo> & nestedVector = nestedMap[currentBufferID];

    int sUdlName = styler.GetPropertyInt("userDefine.udlName", 0);
    if (udlKeywordsMap.find(sUdlName) == udlKeywordsMap.end())
    {
        udlKeywordsMap[sUdlName] = udlKeywordsMapStruct();
    }

    vvstring & commentLineOpen      = udlKeywordsMap[sUdlName].commentLineOpen;
    vvstring & commentLineContinue  = udlKeywordsMap[sUdlName].commentLineContinue;
    vvstring & commentLineClose     = udlKeywordsMap[sUdlName].commentLineClose;
    vvstring & commentOpen          = udlKeywordsMap[sUdlName].commentOpen;
    vvstring & commentClose         = udlKeywordsMap[sUdlName].commentClose;
    vvstring & delim1Open           = udlKeywordsMap[sUdlName].delim1Open;
    vvstring & delim1Escape         = udlKeywordsMap[sUdlName].delim1Escape;
    vvstring & delim1Close          = udlKeywordsMap[sUdlName].delim1Close;
    vvstring & delim2Open           = udlKeywordsMap[sUdlName].delim2Open;
    vvstring & delim2Escape         = udlKeywordsMap[sUdlName].delim2Escape;
    vvstring & delim2Close          = udlKeywordsMap[sUdlName].delim2Close;
    vvstring & delim3Open           = udlKeywordsMap[sUdlName].delim3Open;
    vvstring & delim3Escape         = udlKeywordsMap[sUdlName].delim3Escape;
    vvstring & delim3Close          = udlKeywordsMap[sUdlName].delim3Close;
    vvstring & delim4Open           = udlKeywordsMap[sUdlName].delim4Open;
    vvstring & delim4Escape         = udlKeywordsMap[sUdlName].delim4Escape;
    vvstring & delim4Close          = udlKeywordsMap[sUdlName].delim4Close;
    vvstring & delim5Open           = udlKeywordsMap[sUdlName].delim5Open;
    vvstring & delim5Escape         = udlKeywordsMap[sUdlName].delim5Escape;
    vvstring & delim5Close          = udlKeywordsMap[sUdlName].delim5Close;
    vvstring & delim6Open           = udlKeywordsMap[sUdlName].delim6Open;
    vvstring & delim6Escape         = udlKeywordsMap[sUdlName].delim6Escape;
    vvstring & delim6Close          = udlKeywordsMap[sUdlName].delim6Close;
    vvstring & delim7Open           = udlKeywordsMap[sUdlName].delim7Open;
    vvstring & delim7Escape         = udlKeywordsMap[sUdlName].delim7Escape;
    vvstring & delim7Close          = udlKeywordsMap[sUdlName].delim7Close;
    vvstring & delim8Open           = udlKeywordsMap[sUdlName].delim8Open;
    vvstring & delim8Escape         = udlKeywordsMap[sUdlName].delim8Escape;
    vvstring & delim8Close          = udlKeywordsMap[sUdlName].delim8Close;
    vvstring & operators1           = udlKeywordsMap[sUdlName].operators1;
    vvstring & foldersInCode1Open   = udlKeywordsMap[sUdlName].foldersInCode1Open;
    vvstring & foldersInCode1Middle = udlKeywordsMap[sUdlName].foldersInCode1Middle;
    vvstring & foldersInCode1Close  = udlKeywordsMap[sUdlName].foldersInCode1Close;

    vector<string> & prefixTokens1          = udlKeywordsMap[sUdlName].prefixTokens1;
    vector<string> & prefixTokens2          = udlKeywordsMap[sUdlName].prefixTokens2;
    vector<string> & extrasTokens1          = udlKeywordsMap[sUdlName].extrasTokens1;
    vector<string> & extrasTokens2          = udlKeywordsMap[sUdlName].extrasTokens2;
    vector<string> & suffixTokens1          = udlKeywordsMap[sUdlName].suffixTokens1;
    vector<string> & suffixTokens2          = udlKeywordsMap[sUdlName].suffixTokens2;
    vector<string> & rangeTokens            = udlKeywordsMap[sUdlName].rangeTokens;
    vector<string> & negativePrefixTokens1  = udlKeywordsMap[sUdlName].negativePrefixTokens1;
    vector<string> & negativePrefixTokens2  = udlKeywordsMap[sUdlName].negativePrefixTokens2;
    vector<string> & negativeExtrasTokens2  = udlKeywordsMap[sUdlName].negativeExtrasTokens2;

    if (startPos == 0)
    {
        // in keyword list objects, put longer multi-part string first,
        // e.g. "else if" should go in front of "else"
        bool equal = true;
        bool isMultiPart = false;
        bool switchPerformed = true;

        while (switchPerformed)
        {
            switchPerformed = false;
            for (int i=0; i<MAPPER_TOTAL; ++i)  // for each keyword list object
            {
                for (int j=0; j<kwLists[i]->Length(); ++j)   // for each keyword within object
                {
                    equal = true;
                    int z = 0;
                    for (; kwLists[i]->WordAt(j)[z]; ++z)    // for each letter within keyword
                    {
                        if (kwLists[i]->WordAt(j+1)[z] == '\v' || kwLists[i]->WordAt(j+1)[z] == '\b')
                            isMultiPart = true;

                        if (kwLists[i]->WordAt(j)[z] != kwLists[i]->WordAt(j+1)[z])
                        {
                            equal = false;
                            break;
                        }
                    }
                    if (!isMultiPart)   // is next word multi part keyword?
                    {
                        for (int k=0; kwLists[i]->WordAt(j+1)[k]; ++k)
                        {
                            if (kwLists[i]->WordAt(j+1)[k] == '\v' || kwLists[i]->WordAt(j+1)[k] == '\b')
                            {
                                isMultiPart = true;
                                break;
                            }
                        }
                    }

                    if (equal && isMultiPart && kwLists[i]->WordAt(j+1)[z])  // perform switch only if next word is longer !
                    {
                        const char * temp = kwLists[i]->WordAt(j);
                        kwLists[i]->SetWordAt(j, kwLists[i]->WordAt(j+1));
                        kwLists[i]->SetWordAt(j+1, temp);
                        switchPerformed = true;
                    }
                }
            }
        }

        // if this is BOF, re-generate stuff in global map objects (udlKeywordsMap and nestedMap)
        const char * sFoldersInCode1Open     = styler.pprops->Get("userDefine.foldersInCode1Open");
        const char * sFoldersInCode1Middle   = styler.pprops->Get("userDefine.foldersInCode1Middle");
        const char * sFoldersInCode1Close    = styler.pprops->Get("userDefine.foldersInCode1Close");

        const char * sDelimiters             = styler.pprops->Get("userDefine.delimiters");
        const char * sOperators1             = styler.pprops->Get("userDefine.operators1");
        const char * sComments               = styler.pprops->Get("userDefine.comments");

        // 'GenerateVector' converts strings into vvstring objects
        GenerateVector(commentLineOpen,     sComments,   "00", 0);
        GenerateVector(commentLineContinue, sComments,   "01", commentLineOpen.size());
        GenerateVector(commentLineClose,    sComments,   "02", commentLineOpen.size());
        GenerateVector(commentOpen,         sComments,   "03", 0);
        GenerateVector(commentClose,        sComments,   "04", commentOpen.size());

        GenerateVector(delim1Open,          sDelimiters, "00", 0);
        GenerateVector(delim1Escape,        sDelimiters, "01", delim1Open.size());
        GenerateVector(delim1Close,         sDelimiters, "02", delim1Open.size());
        GenerateVector(delim2Open,          sDelimiters, "03", 0);
        GenerateVector(delim2Escape,        sDelimiters, "04", delim2Open.size());
        GenerateVector(delim2Close,         sDelimiters, "05", delim2Open.size());
        GenerateVector(delim3Open,          sDelimiters, "06", 0);
        GenerateVector(delim3Escape,        sDelimiters, "07", delim3Open.size());
        GenerateVector(delim3Close,         sDelimiters, "08", delim3Open.size());
        GenerateVector(delim4Open,          sDelimiters, "09", 0);
        GenerateVector(delim4Escape,        sDelimiters, "10", delim4Open.size());
        GenerateVector(delim4Close,         sDelimiters, "11", delim4Open.size());
        GenerateVector(delim5Open,          sDelimiters, "12", 0);
        GenerateVector(delim5Escape,        sDelimiters, "13", delim5Open.size());
        GenerateVector(delim5Close,         sDelimiters, "14", delim5Open.size());
        GenerateVector(delim6Open,          sDelimiters, "15", 0);
        GenerateVector(delim6Escape,        sDelimiters, "16", delim6Open.size());
        GenerateVector(delim6Close,         sDelimiters, "17", delim6Open.size());
        GenerateVector(delim7Open,          sDelimiters, "18", 0);
        GenerateVector(delim7Escape,        sDelimiters, "19", delim7Open.size());
        GenerateVector(delim7Close,         sDelimiters, "20", delim7Open.size());
        GenerateVector(delim8Open,          sDelimiters, "21", 0);
        GenerateVector(delim8Escape,        sDelimiters, "22", delim8Open.size());
        GenerateVector(delim8Close,         sDelimiters, "23", delim8Open.size());

        operators1.clear();
        foldersInCode1Open.clear();
        foldersInCode1Middle.clear();
        foldersInCode1Close.clear();

        SubGroup(sFoldersInCode1Open,     foldersInCode1Open,       true);
        SubGroup(sFoldersInCode1Middle,   foldersInCode1Middle,     true);
        SubGroup(sFoldersInCode1Close,    foldersInCode1Close,      true);
        SubGroup(sOperators1,             operators1,               true);

        char * numberPrefix1        = (char *)styler.pprops->Get("userDefine.numberPrefix1");
        char * numberPrefix2        = (char *)styler.pprops->Get("userDefine.numberPrefix2");
        char * numberExtras1        = (char *)styler.pprops->Get("userDefine.numberExtras1");
        char * numberExtras2        = (char *)styler.pprops->Get("userDefine.numberExtras2");
        char * numberSuffix1        = (char *)styler.pprops->Get("userDefine.numberSuffix1");
        char * numberSuffix2        = (char *)styler.pprops->Get("userDefine.numberSuffix2");
        char * numberRange          = (char *)styler.pprops->Get("userDefine.numberRange");

        prefixTokens1.clear();
        prefixTokens2.clear();
        extrasTokens1.clear();
        extrasTokens2.clear();
        suffixTokens1.clear();
        suffixTokens2.clear();
        rangeTokens.clear();
        negativePrefixTokens1.clear();
        negativePrefixTokens2.clear();
        negativeExtrasTokens2.clear();

        // 'StringToVector' converts strings into vector<string> objects
        StringToVector(numberPrefix1, prefixTokens1);
        StringToVector(numberPrefix1, negativePrefixTokens1, true);
        StringToVector(numberPrefix2, prefixTokens2);
        StringToVector(numberPrefix2, negativePrefixTokens2, true);
        StringToVector(numberExtras1, extrasTokens1);
        StringToVector(numberExtras2, extrasTokens2);
        StringToVector(numberExtras2, negativeExtrasTokens2, true);
        StringToVector(numberSuffix1, suffixTokens1);
        StringToVector(numberSuffix2, suffixTokens2);
        StringToVector(numberRange,   rangeTokens);
    }

    // forward strings are actually kept in forwardStruct's, this allows easy access to ScintillaID and MaskID
    // FWS is a single global object used only to create temporary forwardStruct objects that are copied into vector
    vector<forwardStruct> forwards;
    forwards.push_back(*FWS.Set(&delim1Open,        SCE_USER_STYLE_DELIMITER1,      SCE_USER_MASK_NESTING_DELIMITER1));
    forwards.push_back(*FWS.Set(&delim2Open,        SCE_USER_STYLE_DELIMITER2,      SCE_USER_MASK_NESTING_DELIMITER2));
    forwards.push_back(*FWS.Set(&delim3Open,        SCE_USER_STYLE_DELIMITER3,      SCE_USER_MASK_NESTING_DELIMITER3));
    forwards.push_back(*FWS.Set(&delim4Open,        SCE_USER_STYLE_DELIMITER4,      SCE_USER_MASK_NESTING_DELIMITER4));
    forwards.push_back(*FWS.Set(&delim5Open,        SCE_USER_STYLE_DELIMITER5,      SCE_USER_MASK_NESTING_DELIMITER5));
    forwards.push_back(*FWS.Set(&delim6Open,        SCE_USER_STYLE_DELIMITER6,      SCE_USER_MASK_NESTING_DELIMITER6));
    forwards.push_back(*FWS.Set(&delim7Open,        SCE_USER_STYLE_DELIMITER7,      SCE_USER_MASK_NESTING_DELIMITER7));
    forwards.push_back(*FWS.Set(&delim8Open,        SCE_USER_STYLE_DELIMITER8,      SCE_USER_MASK_NESTING_DELIMITER8));
    forwards.push_back(*FWS.Set(&commentOpen,       SCE_USER_STYLE_COMMENT,         SCE_USER_MASK_NESTING_COMMENT));
    forwards.push_back(*FWS.Set(&commentLineOpen,   SCE_USER_STYLE_COMMENTLINE,     SCE_USER_MASK_NESTING_COMMENT_LINE));
    forwards.push_back(*FWS.Set(&operators1,        SCE_USER_STYLE_OPERATOR,        SCE_USER_MASK_NESTING_OPERATORS1));

    // keep delimiter open strings in an array for easier looping
    vvstring * delimStart[SCE_USER_TOTAL_DELIMITERS];
    delimStart[0] = &delim1Open;
    delimStart[1] = &delim2Open;
    delimStart[2] = &delim3Open;
    delimStart[3] = &delim4Open;
    delimStart[4] = &delim5Open;
    delimStart[5] = &delim6Open;
    delimStart[6] = &delim7Open;
    delimStart[7] = &delim8Open;

    vvstring * fwEndVectors[FW_VECTORS_TOTAL];  // array of forward end vectors for multi-part forward search
    fwEndVectors[0]  = &operators1;
    fwEndVectors[1]  = &commentLineOpen;
    fwEndVectors[2]  = &commentLineContinue;
    fwEndVectors[3]  = &commentLineClose;
    fwEndVectors[4]  = &commentOpen;
    fwEndVectors[5]  = &commentClose;
    fwEndVectors[6]  = &foldersInCode1Open;
    fwEndVectors[7]  = &foldersInCode1Middle;
    fwEndVectors[8]  = &foldersInCode1Close;
    fwEndVectors[9]  = &delim1Close;
    fwEndVectors[10] = &delim2Close;
    fwEndVectors[11] = &delim3Close;
    fwEndVectors[12] = &delim4Close;
    fwEndVectors[13] = &delim5Close;
    fwEndVectors[14] = &delim6Close;
    fwEndVectors[15] = &delim7Close;
    fwEndVectors[16] = &delim8Close;

    // keep delimiter escape/close strings in an array for easier looping
    vvstring * delimVectors[(SCE_USER_TOTAL_DELIMITERS+2) * 2];
    delimVectors[0]  = &delim1Escape;
    delimVectors[1]  = &delim1Close;
    delimVectors[2]  = &delim2Escape;
    delimVectors[3]  = &delim2Close;
    delimVectors[4]  = &delim3Escape;
    delimVectors[5]  = &delim3Close;
    delimVectors[6]  = &delim4Escape;
    delimVectors[7]  = &delim4Close;
    delimVectors[8]  = &delim5Escape;
    delimVectors[9]  = &delim5Close;
    delimVectors[10] = &delim6Escape;
    delimVectors[11] = &delim6Close;
    delimVectors[12] = &delim7Escape;
    delimVectors[13] = &delim7Close;
    delimVectors[14] = &delim8Escape;
    delimVectors[15] = &delim8Close;
    // last four are needed just to create numberDelimSeparators
    // they are not used anywhere else
    delimVectors[16] = NULL;;
    delimVectors[17] = &commentClose;
    delimVectors[18] = NULL;
    delimVectors[19] = NULL;

    // again, loops make our lifes easier
    int delimNestings[SCE_USER_TOTAL_DELIMITERS+2];
    delimNestings[0] = delim1Nesting;
    delimNestings[1] = delim2Nesting;
    delimNestings[2] = delim3Nesting;
    delimNestings[3] = delim4Nesting;
    delimNestings[4] = delim5Nesting;
    delimNestings[5] = delim6Nesting;
    delimNestings[6] = delim7Nesting;
    delimNestings[7] = delim8Nesting;
    // last two are needed just to create numberDelimSeparators
    // they are not used anywhere else
    delimNestings[8] = commentNesting;
    delimNestings[9] = lineCommentNesting;

    vvstring * numberDelimSeparators[SCE_USER_TOTAL_DELIMITERS+6][SCE_USER_TOTAL_DELIMITERS+6]; // TODO: define hardcoded values as constants (syncy also for FW_VECTORS_TOTAL)
    for (int i=0; i<SCE_USER_TOTAL_DELIMITERS+2; ++i)
    {
        numberDelimSeparators[i][0]  = delimVectors[i*2 + 1];
        numberDelimSeparators[i][1]  = (delimNestings[i] & SCE_USER_MASK_NESTING_DELIMITER1)    ? delimStart[0]    : NULL;
        numberDelimSeparators[i][2]  = (delimNestings[i] & SCE_USER_MASK_NESTING_DELIMITER2)    ? delimStart[1]    : NULL;
        numberDelimSeparators[i][3]  = (delimNestings[i] & SCE_USER_MASK_NESTING_DELIMITER3)    ? delimStart[2]    : NULL;
        numberDelimSeparators[i][4]  = (delimNestings[i] & SCE_USER_MASK_NESTING_DELIMITER4)    ? delimStart[3]    : NULL;
        numberDelimSeparators[i][5]  = (delimNestings[i] & SCE_USER_MASK_NESTING_DELIMITER5)    ? delimStart[4]    : NULL;
        numberDelimSeparators[i][6]  = (delimNestings[i] & SCE_USER_MASK_NESTING_DELIMITER6)    ? delimStart[5]    : NULL;
        numberDelimSeparators[i][7]  = (delimNestings[i] & SCE_USER_MASK_NESTING_DELIMITER7)    ? delimStart[6]    : NULL;
        numberDelimSeparators[i][8]  = (delimNestings[i] & SCE_USER_MASK_NESTING_DELIMITER8)    ? delimStart[7]    : NULL;
        numberDelimSeparators[i][9]  = (delimNestings[i] & SCE_USER_MASK_NESTING_COMMENT)       ? &commentOpen     : NULL;
        numberDelimSeparators[i][10] = (delimNestings[i] & SCE_USER_MASK_NESTING_COMMENT_LINE)  ? &commentLineOpen : NULL;
        numberDelimSeparators[i][11] = (delimNestings[i] & SCE_USER_MASK_NESTING_OPERATORS1)    ? &operators1      : NULL;
    }

    vector<string> * numberTokens[10];
    numberTokens[0] = &prefixTokens1;
    numberTokens[1] = &prefixTokens2;
    numberTokens[2] = &extrasTokens1;
    numberTokens[3] = &extrasTokens2;
    numberTokens[4] = &suffixTokens1;
    numberTokens[5] = &suffixTokens2;
    numberTokens[6] = &rangeTokens;
    numberTokens[7] = &negativePrefixTokens1;
    numberTokens[8] = &negativePrefixTokens2;
    numberTokens[9] = &negativeExtrasTokens2;

    int levelCurrent = SC_FOLDLEVELBASE;
    int lineCurrent = 0;
    int levelMinCurrent = 0;
    int levelNext = 0;
    int levelPrev = 0;
    int lev = 0;

    bool visibleChars = false;
    bool skipVisibleCheck = false;

    bool dontMove = false;
    bool finished = true;
    int checkEOL = EOL_DEFAULT_VALUE;

    int nestedLevel = 0;
    int openIndex = 0;
    size_t skipForward = 0;
    int prevState = 0;

    int isCommentLine = COMMENTLINE_NO;
    int isPrevLineComment = COMMENTLINE_NO;
    bool isInCommentBlock = false;
    bool isInComment = false;
    int newState = 0;
    int nlCount = 0;

    int continueCommentBlock = 0;
    int decSeparator = SEPARATOR_DOT;

    vector<nestedInfo> lastNestedGroup;

    vvstring * delimEscape = NULL;
    vvstring * delimClose  = NULL;
    vvstring ** numberDelims = NULL;
    int delimNesting = 0;
    size_t docLength = startPos + length;

    if (startPos == 0)
    {
        // foldVector.clear();
        nestedVector.clear();
        lastNestedGroup.clear();
        initStyle = SCE_USER_STYLE_IDENTIFIER;
    }
    else
    {
        size_t oldStartPos = startPos;
        ReColoringCheck(startPos, nestedLevel, initStyle, openIndex, isCommentLine, isInComment,
                        styler, lastNestedGroup, nestedVector, /* foldVector, */ continueCommentBlock);

        // offset move to previous line
        length += (oldStartPos - startPos);
        docLength = startPos + length;
    }

    lineCurrent = styler.GetLine(startPos);
    if (lineCurrent > 0)
        levelCurrent = styler.LevelAt(lineCurrent - 1) >> 16;

    levelMinCurrent = levelCurrent;
    levelNext = levelCurrent;

    StyleContext sc(startPos, length, initStyle, styler);
    for (; finished; )
    {
        dontMove = false;
        checkEOL = EOL_DEFAULT_VALUE;
        if (sc.More() == false)
            finished = false;   // colorize last word, even if file does not end with whitespace char

        switch (sc.state)
        {
            case SCE_USER_STYLE_DELIMITER1:
            case SCE_USER_STYLE_DELIMITER2:
            case SCE_USER_STYLE_DELIMITER3:
            case SCE_USER_STYLE_DELIMITER4:
            case SCE_USER_STYLE_DELIMITER5:
            case SCE_USER_STYLE_DELIMITER6:
            case SCE_USER_STYLE_DELIMITER7:
            case SCE_USER_STYLE_DELIMITER8:
            {
                int index    = sc.state - SCE_USER_STYLE_DELIMITER1;
                delimEscape  = delimVectors[index*2];
                delimClose   = delimVectors[index*2 + 1];
                delimNesting = delimNestings[index];
                numberDelims = numberDelimSeparators[index];
                prevState    = sc.state;
                newState     = sc.state;

                // first, check escape sequence
                bool loopEscape = true;
                vector<string>::iterator iter;
                while (loopEscape == true)
                {
                    loopEscape = false;
                    iter = (*delimEscape)[openIndex].begin();
                    for (; iter != (*delimEscape)[openIndex].end(); ++iter)
                    {
                        if (ignoreCase?sc.MatchIgnoreCase2(iter->c_str()):sc.Match(iter->c_str()))
                        {
                            sc.Forward(iter->length() + 1); // escape is found, skip escape string and one char after it.
                            loopEscape = true;
                            //break;
                        }
                    }
                }

                // second, check end of delimiter sequence
                iter = (*delimClose)[openIndex].begin();
                for (; iter != (*delimClose)[openIndex].end(); ++iter)
                {
                    if (ignoreCase ? sc.MatchIgnoreCase2(iter->c_str()):sc.Match(iter->c_str()))
                    {
                        // record end of delimiter sequence (NI_CLOSE)
                        nestedVector.push_back(*NI.Set(sc.currentPos + iter->length() - 1, nestedLevel--, openIndex, sc.state, NI_CLOSE));
                        // is there anything on the left side? (any backward keyword 'glued' with end of delimiter sequence)
                        setBackwards(kwLists, sc, prefixes, ignoreCase, delimNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                        // paint backward keyword
                        sc.SetState(prevState);
                        // was current delimiter sequence nested, or do we start over from SCE_USER_STYLE_IDENTIFIER?
                        readLastNested(lastNestedGroup, newState, openIndex);
                        // for delimiters that end with ((EOL))
                        if (newState != SCE_USER_STYLE_COMMENTLINE || (sc.ch != '\r' && sc.ch != '\n'))
                            sc.Forward(iter->length());
                        if (sc.atLineStart)
                            checkEOL = EOL_FORCE_CHECK;
                        else if (sc.atLineEnd)
                            checkEOL = EOL_SKIP_CHECK;

                        // paint end of delimiter sequence
                        sc.SetState(newState);

                        dontMove = true;
                        break; // break out of 'for', not 'case'
                    }
                }

                // out of current state?
                if (prevState != newState)
                    break;

                // quick replacement for SCE_USER_STYLE_DEFAULT (important for nested keywords)
                if (isWhiteSpace(sc.ch) && !isWhiteSpace(sc.chPrev))
                {
                    setBackwards(kwLists, sc, prefixes, ignoreCase, delimNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                    sc.SetState(prevState);
                }
                else if ((!isWhiteSpace(sc.ch) && isWhiteSpace(sc.chPrev)))
                {
                    // create new 'compare point' (AKA beginning of nested keyword) before checking for numbers
                    sc.SetState(prevState);
                }

                // third, check nested delimiter sequence
                if (isInListNested(delimNesting, forwards, sc, ignoreCase, openIndex, skipForward,
                                    newState, pureLC, visibleChars, numberTokens, numberDelims, decSeparator))
                {
                    // any backward keyword 'glued' on the left side?
                    setBackwards(kwLists, sc, prefixes, ignoreCase, delimNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);

                    if (newState != SCE_USER_STYLE_OPERATOR && newState != SCE_USER_STYLE_NUMBER)
                    {
                        // record delimiter sequence in BOTH vectors
                        nestedVector.push_back(*NI.Set(sc.currentPos, ++nestedLevel, openIndex, newState, NI_OPEN));
                        lastNestedGroup.push_back(NI);
                    }

                    sc.SetState(newState);  // yes, both 'SetState' calls are needed
                    sc.Forward(skipForward);
                    sc.SetState(newState);

                    if (newState == SCE_USER_STYLE_OPERATOR || newState == SCE_USER_STYLE_NUMBER)
                        sc.ChangeState(prevState);

                    dontMove = true;
                    break;
                }
                break;
            }

            case SCE_USER_STYLE_COMMENT:
            {
                numberDelims = numberDelimSeparators[SCE_USER_TOTAL_DELIMITERS];
                // first, check end of comment sequence
                vector<string>::iterator iter = commentClose[openIndex].begin();
                for (; iter != commentClose[openIndex].end(); ++iter)
                {
                    if (ignoreCase?sc.MatchIgnoreCase2(iter->c_str()):sc.Match(iter->c_str()))
                    {
                        // record end of comment sequence (NI_CLOSE)
                        nestedVector.push_back(*NI.Set(sc.currentPos + iter->length() - 1, nestedLevel--, openIndex, SCE_USER_STYLE_COMMENT, NI_CLOSE));
                        // is there anything on the left side? (any backward keyword 'glued' with end of comment sequence)
                        setBackwards(kwLists, sc, prefixes, ignoreCase, commentNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                        // paint backward keyword and move on
                        sc.SetState(SCE_USER_STYLE_COMMENT);
                        sc.Forward(iter->length());
                        // was current comment sequence nested, or do we start over from SCE_USER_STYLE_IDENTIFIER?
                        readLastNested(lastNestedGroup, newState, openIndex);
                        // paint end of comment sequence
                        sc.SetState(newState);

                        isInComment = false;
                        dontMove = true;
                        break;
                    }
                }

                if (sc.state != SCE_USER_STYLE_COMMENT)
                    break;

                // quick replacement for SCE_USER_STYLE_DEFAULT (important for nested keywords)
                if (isWhiteSpace(sc.ch) && !isWhiteSpace(sc.chPrev))
                {
                    setBackwards(kwLists, sc, prefixes, ignoreCase, commentNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                    sc.SetState(SCE_USER_STYLE_COMMENT);
                }
                else if (!isWhiteSpace(sc.ch) && isWhiteSpace(sc.chPrev))
                {
                    // create new 'compare point' (AKA beginning of nested keyword) before checking for numbers
                    sc.SetState(SCE_USER_STYLE_COMMENT);
                }

                // third, check nested delimiter sequence
                if (isInListNested(commentNesting, forwards, sc, ignoreCase, openIndex, skipForward,
                                    newState, pureLC, visibleChars, numberTokens, numberDelims, decSeparator))
                {
                    // any backward keyword 'glued' on the left side?
                    setBackwards(kwLists, sc, prefixes, ignoreCase, commentNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);

                    if (newState != SCE_USER_STYLE_OPERATOR && newState != SCE_USER_STYLE_NUMBER)
                    {
                        // record delimiter sequence in BOTH vectors
                        nestedVector.push_back(*NI.Set(sc.currentPos, ++nestedLevel, openIndex, newState, NI_OPEN));
                        lastNestedGroup.push_back(NI);
                    }

                    sc.SetState(newState);    // yes, both 'SetState' calls are needed
                    sc.Forward(skipForward);
                    sc.SetState(newState);

                    if (newState == SCE_USER_STYLE_OPERATOR || newState == SCE_USER_STYLE_NUMBER)
                        sc.ChangeState(SCE_USER_STYLE_COMMENT);

                    dontMove = true;
                    break;
                }
                break;
            }

            case SCE_USER_STYLE_COMMENTLINE:
            {
                numberDelims = numberDelimSeparators[SCE_USER_TOTAL_DELIMITERS + 1];

                // first, check end of line comment sequence (in rare cases when line comments can end before new line char)
                vector<string>::iterator iter = commentLineClose[openIndex].begin();
                for (; iter != commentLineClose[openIndex].end(); ++iter)
                {
                    if (ignoreCase?sc.MatchIgnoreCase2(iter->c_str()):sc.Match(iter->c_str()))
                    {
                        // record end of line comment sequence (NI_CLOSE)
                        nestedVector.push_back(*NI.Set(sc.currentPos + iter->length() - 1, nestedLevel--, openIndex, SCE_USER_STYLE_COMMENTLINE, NI_CLOSE));
                        // is there anything on the left side? (any backward keyword 'glued' with end of line comment sequence)
                        setBackwards(kwLists, sc, prefixes, ignoreCase, lineCommentNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                        // paint backward keyword and move on
                        sc.SetState(SCE_USER_STYLE_COMMENTLINE);
                        sc.Forward(iter->length());
                        // was current line comment sequence nested, or do we start over from SCE_USER_STYLE_IDENTIFIER?
                        readLastNested(lastNestedGroup, newState, openIndex);
                        // paint end of line comment sequence
                        sc.SetState(newState);

                        dontMove = true;
                        break;  // break out of 'for', not 'case'
                    }
                }

                if (sc.state != SCE_USER_STYLE_COMMENTLINE)
                    break;

                // quick replacement for SCE_USER_STYLE_DEFAULT (important for nested keywords)
                if (isWhiteSpace(sc.ch) && !isWhiteSpace(sc.chPrev))
                {
                    setBackwards(kwLists, sc, prefixes, ignoreCase, lineCommentNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                    sc.SetState(SCE_USER_STYLE_COMMENTLINE);
                }
                else if (!isWhiteSpace(sc.ch) && isWhiteSpace(sc.chPrev))
                {
                    // create new 'compare point' (AKA beginning of nested keyword) before checking for numbers
                    sc.SetState(SCE_USER_STYLE_COMMENTLINE);
                }

                // second, check line comment continuation
                if (sc.atLineEnd)
                {
                    bool lineContinuation = false;
                    int offset = 0;
                    if (sc.chPrev == '\r')
                       offset = 1;

                    vector<string>::iterator iter = commentLineContinue[openIndex].begin();
                    for (; iter != commentLineContinue[openIndex].end(); ++iter)
                    {
                        size_t length = iter->length();
                        if (length == 0)
                        {
                            if (!dontMove)
                                sc.Forward();
                            continue;
                        }

                        lineContinuation = true;
                        for (size_t i = 0; i < length; ++i)
                        {
                            if (ignoreCase)
                            {
                                if (toupper((*iter)[i]) != toupper(styler.SafeGetCharAt(sc.currentPos - length + i - offset, 0)))
                                {
                                    lineContinuation = false;
                                    break;
                                }
                            }
                            else if ((*iter)[i] != styler.SafeGetCharAt(sc.currentPos - length + i - offset, 0))
                            {
                                lineContinuation = false;
                                break;
                            }
                        }
                        // if line comment continuation string is found at EOL, treat next line as a comment line
                        if (lineContinuation)
                        {
                            isCommentLine = COMMENTLINE_YES;
                            break;  // break out of 'for', not 'case'
                        }
                    }

                    sc.ChangeState(SCE_USER_STYLE_COMMENTLINE); // no need to paint, only change state for now
                    if (!lineContinuation)
                    {
                        // paint \n character too (or \r for old MAc format)
                        sc.Forward();
                        dontMove = true;
                        checkEOL = EOL_FORCE_CHECK;
                        // record end of line comment sequence (NI_CLOSE)
                        nestedVector.push_back(*NI.Set(sc.currentPos - 1, nestedLevel--, openIndex, SCE_USER_STYLE_COMMENTLINE, NI_CLOSE));
                        // was current line comment sequence nested, or do we start over from SCE_USER_STYLE_IDENTIFIER?
                        readLastNested(lastNestedGroup, newState, openIndex);
                        // paint entire line comment sequence in one step
                        sc.SetState(newState);
                    }

                    lineContinuation = false;
                    break;
                }

                if (sc.state != SCE_USER_STYLE_COMMENTLINE)
                    break;

                // third, check nested delimiter sequence
                if (isInListNested(lineCommentNesting, forwards, sc, ignoreCase, openIndex, skipForward,
                                    newState, pureLC, visibleChars, numberTokens, numberDelims, decSeparator))
                {
                    // any backward keyword 'glued' on the left side?
                    setBackwards(kwLists, sc, prefixes, ignoreCase, lineCommentNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);

                    if (newState != SCE_USER_STYLE_OPERATOR && newState != SCE_USER_STYLE_NUMBER)
                    {
                        // record delimiter sequence in BOTH vectors
                        nestedVector.push_back(*NI.Set(sc.currentPos, ++nestedLevel, openIndex, newState, NI_OPEN));
                        lastNestedGroup.push_back(NI);
                    }

                    sc.SetState(newState);    // yes, both 'SetState' calls are needed
                    sc.Forward(skipForward);
                    sc.SetState(newState);

                    if (newState == SCE_USER_STYLE_OPERATOR || newState == SCE_USER_STYLE_NUMBER)
                        sc.ChangeState(SCE_USER_STYLE_COMMENTLINE);

                    dontMove = true;
                    break;
                }

                break;
            }

            case SCE_USER_STYLE_DEFAULT:
            {
                if (isWhiteSpace(sc.ch))
                {
                    setBackwards(kwLists, sc, prefixes, ignoreCase, bwNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                    sc.SetState(SCE_USER_STYLE_IDENTIFIER);
                    break;
                }

                if (!commentLineOpen.empty())
                {
                    if ((pureLC == PURE_LC_NONE) ||
                        (pureLC == PURE_LC_BOL && (sc.chPrev == '\r' || sc.chPrev == '\n')) ||
                        (pureLC == PURE_LC_WSP && visibleChars == false) )
                    {
                        if (isInListForward(commentLineOpen, sc, ignoreCase, openIndex, skipForward))
                        {
                            if (foldComments && isCommentLine != COMMENTLINE_SKIP_TESTING)
                                isCommentLine = COMMENTLINE_YES;

                            // any backward keyword 'glued' on the left side?
                            setBackwards(kwLists, sc, prefixes, ignoreCase, bwNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                            // paint up to start of line comment sequence
                            sc.SetState(SCE_USER_STYLE_COMMENTLINE);
                            // record start of line comment sequence (NI_OPEN) in BOTH vectors
                            nestedVector.push_back(*NI.Set(sc.currentPos, ++nestedLevel, openIndex, SCE_USER_STYLE_COMMENTLINE, NI_OPEN));
                            lastNestedGroup.push_back(NI);
                            // paint start of line comment sequence
                            sc.Forward(skipForward);
                            sc.SetState(SCE_USER_STYLE_COMMENTLINE);
                            dontMove = true;
                            if (sc.atLineEnd)
                                checkEOL = EOL_SKIP_CHECK;

                            break;
                        }
                    }
                }

                if (!commentOpen.empty())
                {
                    if (isInListForward(commentOpen, sc, ignoreCase, openIndex, skipForward))
                    {
                        if (foldComments)
                        {
                            isInComment = true;
                            if (isCommentLine != COMMENTLINE_SKIP_TESTING)
                                isCommentLine = COMMENTLINE_YES;
                        }

                        // any backward keyword 'glued' on the left side?
                        setBackwards(kwLists, sc, prefixes, ignoreCase, bwNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                        // paint up to start of comment sequence
                        sc.SetState(SCE_USER_STYLE_COMMENT);
                        // record start of comment sequence (NI_OPEN) in BOTH nesting vectors
                        nestedVector.push_back(*NI.Set(sc.currentPos, ++nestedLevel, openIndex, SCE_USER_STYLE_COMMENT, NI_OPEN));
                        lastNestedGroup.push_back(NI);
                        // paint start of comment sequence
                        sc.Forward(skipForward);
                        sc.SetState(SCE_USER_STYLE_COMMENT);
                        dontMove = true;
                        if (sc.atLineEnd)
                            checkEOL = EOL_SKIP_CHECK;

                        break;
                    }
                }

                for (int i=0; i<SCE_USER_TOTAL_DELIMITERS; ++i)
                {
                    if (!delimStart[i]->empty())
                    {
                        if (isInListForward(*delimStart[i], sc, ignoreCase, openIndex, skipForward))
                        {
                            // any backward keyword 'glued' on the left side?
                            setBackwards(kwLists, sc, prefixes, ignoreCase, bwNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                            // paint up to start of delimiter sequence
                            sc.SetState(i+SCE_USER_STYLE_DELIMITER1);
                            // record start of delimiter sequence (NI_OPEN) in BOTH nesting vectors
                            nestedVector.push_back(*NI.Set(sc.currentPos, ++nestedLevel, openIndex, i+SCE_USER_STYLE_DELIMITER1, NI_OPEN));
                            lastNestedGroup.push_back(NI);
                            // paint start of delimiter sequence
                            sc.Forward(skipForward);
                            sc.SetState(i+SCE_USER_STYLE_DELIMITER1);
                            dontMove = true;
                            if (sc.atLineEnd)
                                checkEOL = EOL_SKIP_CHECK;
                            break;  // break from nested 'for' loop, not 'case' statement
                        }
                    }
                }

                if (dontMove == true)
                    break;  // delimiter start found, break from case SCE_USER_STYLE_DEFAULT

                if (!operators1.empty())
                {
                    if (isInListForward(operators1, sc, ignoreCase, openIndex, skipForward))
                    {
                        // any backward keyword 'glued' on the left side?
                        setBackwards(kwLists, sc, prefixes, ignoreCase, bwNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                        // paint up to start of sequence
                        sc.SetState(SCE_USER_STYLE_OPERATOR);
                        // paint sequence
                        sc.Forward(skipForward);
                        //sc.ChangeState(SCE_USER_STYLE_OPERATOR);
                        // no closing sequence, start over from default
                        sc.SetState(SCE_USER_STYLE_IDENTIFIER);
                        dontMove = true;
                        if (sc.atLineEnd)
                            checkEOL = EOL_SKIP_CHECK;
                        break;
                    }
                }

                if (!foldersInCode1Open.empty())
                {
                    if (isInListForward(foldersInCode1Open, sc, ignoreCase, openIndex, skipForward))
                    {
                        // any backward keyword 'glued' on the left side?
                        setBackwards(kwLists, sc, prefixes, ignoreCase, bwNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                        // paint up to start of sequence
                        sc.SetState(SCE_USER_STYLE_FOLDER_IN_CODE1);
                        // paint sequence
                        sc.Forward(skipForward);
                        //sc.ChangeState(SCE_USER_STYLE_FOLDER_IN_CODE1);
                        // no closing sequence, start over from default
                        sc.SetState(SCE_USER_STYLE_IDENTIFIER);
                        dontMove = true;
                        if (sc.atLineEnd)
                            checkEOL = EOL_SKIP_CHECK;

                        if (levelMinCurrent > levelNext)
                            levelMinCurrent = levelNext;

                        levelNext++;
                        break;
                    }
                }

                if (!foldersInCode1Middle.empty())
                {
                    if (isInListForward(foldersInCode1Middle, sc, ignoreCase, openIndex, skipForward))
                    {
                        // any backward keyword 'glued' on the left side?
                        setBackwards(kwLists, sc, prefixes, ignoreCase, bwNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                        // paint up to start of sequence
                        sc.SetState(SCE_USER_STYLE_FOLDER_IN_CODE1);
                        // paint sequence
                        sc.Forward(skipForward);
                        //sc.ChangeState(SCE_USER_STYLE_FOLDER_IN_CODE1);
                        // no closing sequence, start over from default
                        sc.SetState(SCE_USER_STYLE_IDENTIFIER);
                        dontMove = true;
                        if (sc.atLineEnd)
                            checkEOL = EOL_SKIP_CHECK;

                        levelNext--;
                        if (levelMinCurrent > levelNext)
                            levelMinCurrent = levelNext;

                        levelNext++;
                        break;
                    }
                }

                if (!foldersInCode1Close.empty())
                {
                    if (isInListForward(foldersInCode1Close, sc, ignoreCase, openIndex, skipForward))
                    {
                        // any backward keyword 'glued' on the left side?
                        setBackwards(kwLists, sc, prefixes, ignoreCase, bwNesting, fwEndVectors, levelMinCurrent, levelNext, nlCount, dontMove, docLength);
                        // paint up to start of sequence
                        sc.SetState(SCE_USER_STYLE_FOLDER_IN_CODE1);
                        // paint sequence
                        sc.Forward(skipForward);
                        //sc.ChangeState(SCE_USER_STYLE_FOLDER_IN_CODE1);
                        // no closing sequence, start over from default
                        sc.SetState(SCE_USER_STYLE_IDENTIFIER);
                        if (sc.atLineEnd)
                            checkEOL = EOL_SKIP_CHECK;

                        dontMove = true;
                        levelNext--;
                        break;
                    }
                }

                if (foldComments && isCommentLine != COMMENTLINE_SKIP_TESTING)
                    isCommentLine = COMMENTLINE_SKIP_TESTING;

                break;
            }

            // determine if a new state should be entered.
            case SCE_USER_STYLE_IDENTIFIER:
            {
                if (isWhiteSpace(sc.ch))
                    break;

                if (IsNumber(sc, numberTokens, fwEndVectors, ignoreCase, decSeparator, skipForward))
                {
                    // paint up to start of sequence
                    sc.SetState(SCE_USER_STYLE_NUMBER);
                    // paint sequence
                    sc.Forward(skipForward);
                    //sc.ChangeState(SCE_USER_STYLE_NUMBER);
                    // start over from default
                    sc.SetState(SCE_USER_STYLE_IDENTIFIER);

                    if (isWhiteSpace(sc.ch))
                        break;
                }

                if (!isWhiteSpace(sc.ch))// && isWhiteSpace(sc.chPrev)) // word start
                {
                    sc.SetState(SCE_USER_STYLE_DEFAULT);
                    skipVisibleCheck = true;
                    dontMove = true;
                    break;
                }
                break;
            }

            default:
                break;
        }

        if (foldComments)
            if (isInComment == false)
                if (isCommentLine == COMMENTLINE_NO)
                    if (sc.state != SCE_USER_STYLE_COMMENTLINE)
                        if (sc.state != SCE_USER_STYLE_IDENTIFIER)
                            if (sc.state != SCE_USER_STYLE_DEFAULT)
                                if (!isWhiteSpace(sc.ch))
                                    isCommentLine = COMMENTLINE_SKIP_TESTING;

        if (skipVisibleCheck == true)
            skipVisibleCheck = false;
        else if (visibleChars == false && !isWhiteSpace(sc.ch))
            visibleChars = true;

        if ((sc.atLineEnd == true && checkEOL != EOL_SKIP_CHECK) || (sc.atLineEnd == false && checkEOL == EOL_FORCE_CHECK))
        {
            if (foldComments == true)
            {
                if (levelCurrent != levelNext)
                    isCommentLine = COMMENTLINE_SKIP_TESTING;

                if (continueCommentBlock > 0)
                {
                    if (continueCommentBlock & CL_PREVPREV)
                    {
                        isInCommentBlock = true;
                        isPrevLineComment = COMMENTLINE_YES;

                        if (!(continueCommentBlock & CL_CURRENT))
                        {
                            levelNext++;
                            levelMinCurrent++;
                            levelCurrent++;
                            levelPrev = (levelMinCurrent | levelNext << 16) | SC_ISCOMMENTLINE;
                        }
                    }
                    else if (continueCommentBlock & CL_PREV)
                    {
                        isPrevLineComment = COMMENTLINE_YES;
                        if (continueCommentBlock & CL_CURRENT)
                        {
                            levelMinCurrent--;
                            levelNext--;
                            levelCurrent--;
                            levelPrev = (levelMinCurrent | levelNext << 16) | SC_ISCOMMENTLINE;
                        }
                    }
                    continueCommentBlock = 0;
                }

                if (isInCommentBlock && isCommentLine != COMMENTLINE_YES && isPrevLineComment == COMMENTLINE_YES)
                {
                    levelNext--;
                    levelPrev = (levelMinCurrent | levelNext << 16) | SC_ISCOMMENTLINE;
                    levelMinCurrent--;
                    isInCommentBlock = false;
                }

                if (!isInCommentBlock && isCommentLine == COMMENTLINE_YES && isPrevLineComment == COMMENTLINE_YES)
                {
                    levelNext++;
                    levelPrev = (levelMinCurrent | levelNext << 16) | SC_FOLDLEVELHEADERFLAG | SC_ISCOMMENTLINE;
                    levelMinCurrent = levelNext;
                    isInCommentBlock = true;
                }

                if (levelPrev != 0)
                {
                    // foldVector[lineCurrent - 1] = levelPrev;
                    styler.SetLevel(lineCurrent - 1, levelPrev);
                    levelPrev = 0;
                }
            }

            lev = levelMinCurrent | levelNext << 16;
            if (foldComments && isCommentLine == COMMENTLINE_YES)
                lev |= SC_ISCOMMENTLINE;
            if (visibleChars == false && foldCompact)
                lev |= SC_FOLDLEVELWHITEFLAG;
            if (levelMinCurrent < levelNext)
                lev |= SC_FOLDLEVELHEADERFLAG;
            // foldVector.push_back(lev);
            styler.SetLevel(lineCurrent, lev);

            for (int i=0; i<nlCount; ++i)   // multi-line multi-part keyword
            {
                // foldVector.push_back(levelNext | levelNext << 16);  // TODO: what about SC_ISCOMMENTLINE?
                styler.SetLevel(lineCurrent++, levelNext | levelNext << 16);
            }
            nlCount = 0;

            lineCurrent++;
            levelCurrent = levelNext;
            levelMinCurrent = levelCurrent;
            visibleChars = false;
            if (foldComments)
            {
                isPrevLineComment = isCommentLine==COMMENTLINE_YES ? COMMENTLINE_YES:COMMENTLINE_NO;
                isCommentLine = isInComment ? COMMENTLINE_YES:COMMENTLINE_NO;
            }
        }

        if (!dontMove)
            sc.Forward();
    }
    sc.Complete();
}

static void FoldUserDoc(Sci_PositionU /* startPos */, Sci_Position /* length */, int /*initStyle*/, WordList *[],  Accessor & /* styler */)
{
    // this function will not be used in final version of the code.
    // it should remain commented out as it is useful for debugging purposes !!!
    // perhaps ifdef block would be a wiser choice, but commenting out works just fine for the time being

    // int lineCurrent = styler.GetLine(startPos);
    // vector<int>::iterator iter = foldVectorStatic->begin() + lineCurrent;

    // for (; iter != foldVectorStatic->end(); ++iter)
    // {
        // styler.SetLevel(lineCurrent++, *iter);
    // }
}

static const char * const userDefineWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "Documentation comment keywords",
            "Fold header keywords",
            0,
        };

LexerModule lmUserDefine(SCLEX_USER, ColouriseUserDoc, "user", FoldUserDoc, userDefineWordLists);
