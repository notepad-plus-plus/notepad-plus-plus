/******************************************************************
 *  LexTxt2tags.cxx
 *
 *  A simple Txt2tags lexer for scintilla.
 *
 *
 *  Adapted by Eric Forgeot
 *  Based on the LexMarkdown.cxx by Jon Strait - jstrait@moonloop.net
 *
 *  What could be improved:
 *   - Verbatim lines could be like for raw lines : when there is no space between the ``` and the following text, the first letter should be colored so the user would understand there must be a space for a valid tag.
 *   - marks such as bold, italic, strikeout, underline should begin to be highlighted only when they are closed and valid.
 *   - verbatim and raw area should be highlighted too.
 *
 *  The License.txt file describes the conditions under which this
 *  software may be distributed.
 *
 *****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

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



static inline bool IsNewline(const int ch) {
    return (ch == '\n' || ch == '\r');
}

// True if can follow ch down to the end with possibly trailing whitespace
static bool FollowToLineEnd(const int ch, const int state, const Sci_PositionU endPos, StyleContext &sc) {
    Sci_PositionU i = 0;
    while (sc.GetRelative(++i) == ch)
        ;
    // Skip over whitespace
    while (IsASpaceOrTab(sc.GetRelative(i)) && sc.currentPos + i < endPos)
        ++i;
    if (IsNewline(sc.GetRelative(i)) || sc.currentPos + i == endPos) {
        sc.Forward(i);
        sc.ChangeState(state);
        sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
        return true;
    }
    else return false;
}

// Does the previous line have more than spaces and tabs?
static bool HasPrevLineContent(StyleContext &sc) {
    Sci_Position i = 0;
    // Go back to the previous newline
    while ((--i + sc.currentPos) && !IsNewline(sc.GetRelative(i)))
        ;
    while (--i + sc.currentPos) {
        if (IsNewline(sc.GetRelative(i)))
            break;
        if (!IsASpaceOrTab(sc.GetRelative(i)))
            return true;
    }
    return false;
}

// Separator line
static bool IsValidHrule(const Sci_PositionU endPos, StyleContext &sc) {
    int count = 1;
    Sci_PositionU i = 0;
    for (;;) {
        ++i;
        int c = sc.GetRelative(i);
        if (c == sc.ch)
            ++count;
        // hit a terminating character
        else if (!IsASpaceOrTab(c) || sc.currentPos + i == endPos) {
            // Are we a valid HRULE
            if ((IsNewline(c) || sc.currentPos + i == endPos) &&
                    count >= 20 && !HasPrevLineContent(sc)) {
                sc.SetState(SCE_TXT2TAGS_HRULE);
                sc.Forward(i);
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
                return true;
            }
            else {
                sc.SetState(SCE_TXT2TAGS_DEFAULT);
		return false;
            }
        }
    }
}

static void ColorizeTxt2tagsDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                               WordList **, Accessor &styler) {
    Sci_PositionU endPos = startPos + length;
    int precharCount = 0;
    // Don't advance on a new loop iteration and retry at the same position.
    // Useful in the corner case of having to start at the beginning file position
    // in the default state.
    bool freezeCursor = false;

    StyleContext sc(startPos, length, initStyle, styler);

    while (sc.More()) {
        // Skip past escaped characters
        if (sc.ch == '\\') {
            sc.Forward();
            continue;
        }

        // A blockquotes resets the line semantics
        if (sc.state == SCE_TXT2TAGS_BLOCKQUOTE){
            sc.Forward(2);
            sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
        }
        // An option colors the whole line
        if (sc.state == SCE_TXT2TAGS_OPTION){
            FollowToLineEnd('%', SCE_TXT2TAGS_OPTION, endPos, sc);
        }
        if (sc.state == SCE_TXT2TAGS_POSTPROC){
            FollowToLineEnd('%', SCE_TXT2TAGS_POSTPROC, endPos, sc);
        }
        if (sc.state == SCE_TXT2TAGS_PREPROC){
            FollowToLineEnd('%', SCE_TXT2TAGS_PREPROC, endPos, sc);
        }
        // A comment colors the whole line
        if (sc.state == SCE_TXT2TAGS_COMMENT){
            FollowToLineEnd('%', SCE_TXT2TAGS_COMMENT, endPos, sc);
        }
        // Conditional state-based actions
        if (sc.state == SCE_TXT2TAGS_CODE2) {
        if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
            if (sc.Match("``") && sc.GetRelative(-2) != ' ') {
                sc.Forward(2);
                sc.SetState(SCE_TXT2TAGS_DEFAULT);
            }
        }
        // Table
        else if (sc.state == SCE_TXT2TAGS_CODE) {
        if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
            if (sc.ch == '|' && sc.chPrev != ' ')
                sc.ForwardSetState(SCE_TXT2TAGS_DEFAULT);
        }
        // Strong
        else if (sc.state == SCE_TXT2TAGS_STRONG1) {
        if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
            if (sc.Match("**") && sc.chPrev != ' ') {
                sc.Forward(2);
                sc.SetState(SCE_TXT2TAGS_DEFAULT);
            }
        }
        // Emphasis
        else if (sc.state == SCE_TXT2TAGS_EM1) {
        if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
            if (sc.Match("//") && sc.chPrev != ' ') {
                sc.Forward(2);
                sc.ForwardSetState(SCE_TXT2TAGS_DEFAULT);
           }
        }
        // Underline
        else if (sc.state == SCE_TXT2TAGS_EM2) {
        if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
            if (sc.Match("__") && sc.chPrev != ' ') {
                sc.Forward(2);
                sc.ForwardSetState(SCE_TXT2TAGS_DEFAULT);
           }
        }
        // codeblock
        else if (sc.state == SCE_TXT2TAGS_CODEBK) {
                if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
            if (sc.atLineStart && sc.Match("```")) {
                Sci_Position i = 1;
                while (!IsNewline(sc.GetRelative(i)) && sc.currentPos + i < endPos)
                    i++;
                sc.Forward(i);
                sc.SetState(SCE_TXT2TAGS_DEFAULT);
            }
        }
        // strikeout
        else if (sc.state == SCE_TXT2TAGS_STRIKEOUT) {
        if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
            if (sc.Match("--") && sc.chPrev != ' ') {
                sc.Forward(2);
                sc.SetState(SCE_TXT2TAGS_DEFAULT);
            }
        }
        // Headers
        else if (sc.state == SCE_TXT2TAGS_LINE_BEGIN) {
            if (sc.Match("======"))
                {
                sc.SetState(SCE_TXT2TAGS_HEADER6);
                sc.Forward();
                }
            else if (sc.Match("====="))
                {
                sc.SetState(SCE_TXT2TAGS_HEADER5);
                sc.Forward();
                }
            else if (sc.Match("===="))
                {
                sc.SetState(SCE_TXT2TAGS_HEADER4);
                sc.Forward();
                }
            else if (sc.Match("==="))
                {
                sc.SetState(SCE_TXT2TAGS_HEADER3);
                sc.Forward();
                }
                //SetStateAndZoom(SCE_TXT2TAGS_HEADER3, 3, '=', sc);
            else if (sc.Match("==")) {
                sc.SetState(SCE_TXT2TAGS_HEADER2);
                sc.Forward();
                }
                //SetStateAndZoom(SCE_TXT2TAGS_HEADER2, 2, '=', sc);
            else if (sc.Match("=")) {
                // Catch the special case of an unordered list
                if (sc.chNext == '.' && IsASpaceOrTab(sc.GetRelative(2))) {
                    precharCount = 0;
                    sc.SetState(SCE_TXT2TAGS_PRECHAR);
                }
                else
                    {
                    sc.SetState(SCE_TXT2TAGS_HEADER1);
                    sc.Forward();
                    }
                    //SetStateAndZoom(SCE_TXT2TAGS_HEADER1, 1, '=', sc);
            }

            // Numbered title
            else if (sc.Match("++++++"))
                {
                sc.SetState(SCE_TXT2TAGS_HEADER6);
                sc.Forward();
                }
            else if (sc.Match("+++++"))
                {
                sc.SetState(SCE_TXT2TAGS_HEADER5);
                sc.Forward();
                }
            else if (sc.Match("++++"))
                {
                sc.SetState(SCE_TXT2TAGS_HEADER4);
                sc.Forward();
                }
            else if (sc.Match("+++"))
                {
                sc.SetState(SCE_TXT2TAGS_HEADER3);
                sc.Forward();
                }
                //SetStateAndZoom(SCE_TXT2TAGS_HEADER3, 3, '+', sc);
            else if (sc.Match("++")) {
                sc.SetState(SCE_TXT2TAGS_HEADER2);
                sc.Forward();
                }
                //SetStateAndZoom(SCE_TXT2TAGS_HEADER2, 2, '+', sc);
            else if (sc.Match("+")) {
                // Catch the special case of an unordered list
                if (sc.chNext == ' ' && IsASpaceOrTab(sc.GetRelative(1))) {
                 //    if (IsNewline(sc.ch)) {
                     	//precharCount = 0;
                //		sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
                		//sc.SetState(SCE_TXT2TAGS_PRECHAR);
				//	}
                //    else {
                //    precharCount = 0;
                    sc.SetState(SCE_TXT2TAGS_OLIST_ITEM);
                    sc.Forward(2);
                    sc.SetState(SCE_TXT2TAGS_DEFAULT);
               //     sc.SetState(SCE_TXT2TAGS_PRECHAR);
				//	}
                }
                else
                    {
                    sc.SetState(SCE_TXT2TAGS_HEADER1);
                    sc.Forward();
                    }
            }


            // Codeblock
            else if (sc.Match("```")) {
                if (!HasPrevLineContent(sc))
              //  if (!FollowToLineEnd(sc))
                    sc.SetState(SCE_TXT2TAGS_CODEBK);
                else
                    sc.SetState(SCE_TXT2TAGS_DEFAULT);
            }

            // Preproc
            else if (sc.Match("%!preproc")) {
                sc.SetState(SCE_TXT2TAGS_PREPROC);
            }
            // Postproc
            else if (sc.Match("%!postproc")) {
                sc.SetState(SCE_TXT2TAGS_POSTPROC);
            }
            // Option
            else if (sc.Match("%!")) {
                sc.SetState(SCE_TXT2TAGS_OPTION);
            }

             // Comment
            else if (sc.ch == '%') {
                sc.SetState(SCE_TXT2TAGS_COMMENT);
            }
            // list
            else if (sc.ch == '-') {
                    precharCount = 0;
                    sc.SetState(SCE_TXT2TAGS_PRECHAR);
            }
            // def list
            else if (sc.ch == ':') {
                    precharCount = 0;
                   sc.SetState(SCE_TXT2TAGS_OLIST_ITEM);
                   sc.Forward(1);
                   sc.SetState(SCE_TXT2TAGS_PRECHAR);
            }
            else if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
            else {
                precharCount = 0;
                sc.SetState(SCE_TXT2TAGS_PRECHAR);
            }
        }

        // The header lasts until the newline
        else if (sc.state == SCE_TXT2TAGS_HEADER1 || sc.state == SCE_TXT2TAGS_HEADER2 ||
                sc.state == SCE_TXT2TAGS_HEADER3 || sc.state == SCE_TXT2TAGS_HEADER4 ||
                sc.state == SCE_TXT2TAGS_HEADER5 || sc.state == SCE_TXT2TAGS_HEADER6) {
            if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
        }

        // New state only within the initial whitespace
        if (sc.state == SCE_TXT2TAGS_PRECHAR) {
            // Blockquote
            if (sc.Match("\"\"\"") && precharCount < 5){

                sc.SetState(SCE_TXT2TAGS_BLOCKQUOTE);
                sc.Forward(1);
                }
            /*
            // Begin of code block
            else if (!HasPrevLineContent(sc) && (sc.chPrev == '\t' || precharCount >= 4))
                sc.SetState(SCE_TXT2TAGS_CODEBK);
            */
            // HRule - Total of 20 or more hyphens, asterisks, or underscores
            // on a line by themselves
            else if ((sc.ch == '-' ) && IsValidHrule(endPos, sc))
                ;
            // Unordered list
            else if ((sc.ch == '-') && IsASpaceOrTab(sc.chNext)) {
                sc.SetState(SCE_TXT2TAGS_ULIST_ITEM);
                sc.ForwardSetState(SCE_TXT2TAGS_DEFAULT);
            }
            // Ordered list
            else if (IsADigit(sc.ch)) {
                Sci_Position digitCount = 0;
                while (IsADigit(sc.GetRelative(++digitCount)))
                    ;
                if (sc.GetRelative(digitCount) == '.' &&
                        IsASpaceOrTab(sc.GetRelative(digitCount + 1))) {
                    sc.SetState(SCE_TXT2TAGS_OLIST_ITEM);
                    sc.Forward(digitCount + 1);
                    sc.SetState(SCE_TXT2TAGS_DEFAULT);
                }
            }
            // Alternate Ordered list
            else if (sc.ch == '+' && sc.chNext == ' ' && IsASpaceOrTab(sc.GetRelative(2))) {
            //    sc.SetState(SCE_TXT2TAGS_OLIST_ITEM);
            //    sc.Forward(2);
             //   sc.SetState(SCE_TXT2TAGS_DEFAULT);
            }
            else if (sc.ch != ' ' || precharCount > 2)
                sc.SetState(SCE_TXT2TAGS_DEFAULT);
            else
                ++precharCount;
        }

        // New state anywhere in doc
        if (sc.state == SCE_TXT2TAGS_DEFAULT) {
         //   if (sc.atLineStart && sc.ch == '#') {
         //       sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
         //       freezeCursor = true;
         //   }
            // Links and Images
            if (sc.Match("![") || sc.ch == '[') {
                Sci_Position i = 0, j = 0, k = 0;
                Sci_Position len = endPos - sc.currentPos;
                while (i < len && (sc.GetRelative(++i) != ']' || sc.GetRelative(i - 1) == '\\'))
                    ;
                if (sc.GetRelative(i) == ']') {
                    j = i;
                    if (sc.GetRelative(++i) == '(') {
                        while (i < len && (sc.GetRelative(++i) != '(' || sc.GetRelative(i - 1) == '\\'))
                            ;
                        if (sc.GetRelative(i) == '(')
                            k = i;
                    }

                    else if (sc.GetRelative(i) == '[' || sc.GetRelative(++i) == '[') {
                        while (i < len && (sc.GetRelative(++i) != ']' || sc.GetRelative(i - 1) == '\\'))
                            ;
                        if (sc.GetRelative(i) == ']')
                            k = i;
                    }
                }
                // At least a link text
                if (j) {
                    sc.SetState(SCE_TXT2TAGS_LINK);
                    sc.Forward(j);
                    // Also has a URL or reference portion
                    if (k)
                        sc.Forward(k - j);
                    sc.ForwardSetState(SCE_TXT2TAGS_DEFAULT);
                }
            }
            // Code - also a special case for alternate inside spacing
            if (sc.Match("``") && sc.GetRelative(3) != ' ') {
                sc.SetState(SCE_TXT2TAGS_CODE2);
                sc.Forward();
            }
            else if (sc.ch == '|' && sc.GetRelative(3) != ' ') {
                sc.SetState(SCE_TXT2TAGS_CODE);
            }
            // Strong
            else if (sc.Match("**") && sc.GetRelative(2) != ' ') {
                sc.SetState(SCE_TXT2TAGS_STRONG1);
                sc.Forward();
           }
            // Emphasis
            else if (sc.Match("//") && sc.GetRelative(2) != ' ') {
                sc.SetState(SCE_TXT2TAGS_EM1);
                sc.Forward();
            }
            else if (sc.Match("__") && sc.GetRelative(2) != ' ') {
                sc.SetState(SCE_TXT2TAGS_EM2);
                sc.Forward();
            }
            // Strikeout
            else if (sc.Match("--") && sc.GetRelative(2) != ' ') {
                sc.SetState(SCE_TXT2TAGS_STRIKEOUT);
                sc.Forward();
            }

            // Beginning of line
            else if (IsNewline(sc.ch))
                sc.SetState(SCE_TXT2TAGS_LINE_BEGIN);
        }
        // Advance if not holding back the cursor for this iteration.
        if (!freezeCursor)
            sc.Forward();
        freezeCursor = false;
    }
    sc.Complete();
}

extern const LexerModule lmTxt2tags(SCLEX_TXT2TAGS, ColorizeTxt2tagsDoc, "txt2tags");


