// Scintilla source code edit control
/** @file LexSorcus.cxx
** Lexer for SORCUS installation files
** Written by Eugen Bitter and Christoph Baumann at SORCUS Computer, Heidelberg Germany
** Based on the ASM Lexer by The Black Horus
**/

// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif


//each character a..z and A..Z + '_' can be part of a keyword
//additionally numbers that follow 'M' can be contained in a keyword
static inline bool IsSWordStart(const int ch, const int prev_ch)
{
    if (isalpha(ch) || (ch == '_') || ((isdigit(ch)) && (prev_ch == 'M')))
        return true;

    return false;
}


//only digits that are not preceded by 'M' count as a number
static inline bool IsSorcusNumber(const int ch, const int prev_ch)
{
    if ((isdigit(ch)) && (prev_ch != 'M'))
        return true;

    return false;
}


//only = is a valid operator
static inline bool IsSorcusOperator(const int ch)
{
    if (ch == '=')
        return true;

    return false;
}


static void ColouriseSorcusDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                               Accessor &styler)
{

    WordList &Command = *keywordlists[0];
    WordList &Parameter = *keywordlists[1];
    WordList &Constant = *keywordlists[2];

    // Do not leak onto next line
    if (initStyle == SCE_SORCUS_STRINGEOL)
        initStyle = SCE_SORCUS_DEFAULT;

    StyleContext sc(startPos, length, initStyle, styler);

    for (; sc.More(); sc.Forward())
    {

        // Prevent SCE_SORCUS_STRINGEOL from leaking back to previous line
        if (sc.atLineStart && (sc.state == SCE_SORCUS_STRING))
        {
            sc.SetState(SCE_SORCUS_STRING);
        }

        // Determine if the current state should terminate.
        if (sc.state == SCE_SORCUS_OPERATOR)
        {
            if (!IsSorcusOperator(sc.ch))
            {
                sc.SetState(SCE_SORCUS_DEFAULT);
            }
        }
        else if(sc.state == SCE_SORCUS_NUMBER)
        {
            if(!IsSorcusNumber(sc.ch, sc.chPrev))
            {
                sc.SetState(SCE_SORCUS_DEFAULT);
            }
        }
        else if (sc.state == SCE_SORCUS_IDENTIFIER)
        {
            if (!IsSWordStart(sc.ch, sc.chPrev))
            {
                char s[100];

                sc.GetCurrent(s, sizeof(s));

                if (Command.InList(s))
                {
                    sc.ChangeState(SCE_SORCUS_COMMAND);
                }
                else if (Parameter.InList(s))
                {
                    sc.ChangeState(SCE_SORCUS_PARAMETER);
                }
                else if (Constant.InList(s))
                {
                    sc.ChangeState(SCE_SORCUS_CONSTANT);
                }

                sc.SetState(SCE_SORCUS_DEFAULT);
            }
        }
        else if (sc.state == SCE_SORCUS_COMMENTLINE )
        {
            if (sc.atLineEnd)
            {
                sc.SetState(SCE_SORCUS_DEFAULT);
            }
        }
        else if (sc.state == SCE_SORCUS_STRING)
        {
            if (sc.ch == '\"')
            {
                sc.ForwardSetState(SCE_SORCUS_DEFAULT);
            }
            else if (sc.atLineEnd)
            {
                sc.ChangeState(SCE_SORCUS_STRINGEOL);
                sc.ForwardSetState(SCE_SORCUS_DEFAULT);
            }
        }

        // Determine if a new state should be entered.
        if (sc.state == SCE_SORCUS_DEFAULT)
        {
            if ((sc.ch == ';') || (sc.ch == '\''))
            {
                sc.SetState(SCE_SORCUS_COMMENTLINE);
            }
            else if (IsSWordStart(sc.ch, sc.chPrev))
            {
                sc.SetState(SCE_SORCUS_IDENTIFIER);
            }
            else if (sc.ch == '\"')
            {
                sc.SetState(SCE_SORCUS_STRING);
            }
            else if (IsSorcusOperator(sc.ch))
            {
                sc.SetState(SCE_SORCUS_OPERATOR);
            }
            else if (IsSorcusNumber(sc.ch, sc.chPrev))
            {
                sc.SetState(SCE_SORCUS_NUMBER);
            }
        }

    }
    sc.Complete();
}


static const char* const SorcusWordListDesc[] = {"Command","Parameter", "Constant", 0};

LexerModule lmSorc(SCLEX_SORCUS, ColouriseSorcusDoc, "sorcins", 0, SorcusWordListDesc);






























