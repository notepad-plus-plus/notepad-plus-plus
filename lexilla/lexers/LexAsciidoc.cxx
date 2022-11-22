/******************************************************************
 *  LexAsciidoc.cxx
 *
 *  A simple Asciidoc lexer for scintilla.
 *
 *  Based on the LexMarkdown.cxx by Jon Strait - jstrait@moonloop.net
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

namespace {

typedef struct {
    bool start;
    int len1;
    int len2;
    const char *name;
} MacroItem;

static const MacroItem MacroList[] = {
    // Directives
    {true,  5, 2, "ifdef::"},
    {true,  6, 2, "ifeval::"},
    {true,  6, 2, "ifndef::"},
    {true,  5, 2, "endif::"},
    // Macros
    {true,  5, 2, "audio::"},
    {true,  7, 2, "include::"},
    {true,  5, 2, "image::"},
    {true,  5, 2, "video::"},
    {false, 8, 1, "asciimath:"},
    {false, 3, 1, "btn:"},
    {false, 5, 1, "image:"},
    {false, 3, 1, "kbd:"},
    {false, 9, 1, "latexmath:"},
    {false, 4, 1, "link:"},
    {false, 6, 1, "mailto:"},
    {false, 4, 1, "menu:"},
    {false, 4, 1, "pass:"},
    {false, 4, 1, "stem:"},
    {false, 4, 1, "xref:"},
    // Admonitions
    {true,  7, 1, "CAUTION:"},
    {true,  9, 1, "IMPORTANT:"},
    {true,  4, 1, "NOTE:"},
    {true,  3, 1, "TIP:"},
    {true,  7, 1, "WARNING:"},
    {false, 0, 0, NULL}
};

constexpr bool IsNewline(const int ch) {
    // sc.GetRelative(i) returns '\0' if out of range
    return (ch == '\n' || ch == '\r' || ch == '\0');
}

}

static bool AtTermStart(StyleContext &sc) {
    return sc.currentPos == 0 || sc.chPrev == 0 || isspacechar(sc.chPrev);
}

static void ColorizeAsciidocDoc(Sci_PositionU startPos, Sci_Position length, int initStyle,
                                WordList **, Accessor &styler) {
    bool freezeCursor = false;

    StyleContext sc(startPos, static_cast<Sci_PositionU>(length), initStyle, styler);

    while (sc.More()) {
        // Skip past escaped characters
        if (sc.ch == '\\') {
            sc.Forward();
            continue;
        }

        // Skip newline.
        if (IsNewline(sc.ch)) {
            // Newline doesn't end blocks
            if (sc.state != SCE_ASCIIDOC_CODEBK && \
                sc.state != SCE_ASCIIDOC_PASSBK && \
                sc.state != SCE_ASCIIDOC_COMMENTBK && \
                sc.state != SCE_ASCIIDOC_LITERALBK) {
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            sc.Forward();
            continue;
        }

        // Conditional state-based actions
        switch (sc.state) {

        // Strong
        case SCE_ASCIIDOC_STRONG1:
            if (sc.ch == '*' && sc.chPrev != ' ') {
                sc.Forward();
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
                freezeCursor = true;
            }
            break;
        case SCE_ASCIIDOC_STRONG2:
            if (sc.Match("**") && sc.chPrev != ' ') {
                sc.Forward(2);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
                freezeCursor = true;
            }
            break;

        // Emphasis
        case SCE_ASCIIDOC_EM1:
            if (sc.ch == '_' && sc.chPrev != ' ') {
                sc.Forward();
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
                freezeCursor = true;
            }
            break;
        case SCE_ASCIIDOC_EM2:
            if (sc.Match("__") && sc.chPrev != ' ') {
                sc.Forward(2);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
                freezeCursor = true;
            }
            break;

        // Link
        case SCE_ASCIIDOC_LINK:
            if (sc.ch == ']' && sc.chPrev != '\\') {
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            break;

        // Code block
        case SCE_ASCIIDOC_CODEBK:
            if (sc.atLineStart && sc.Match("----") && IsNewline(sc.GetRelative(4))) {
                sc.Forward(4);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            break;

        // Passthrough block
        case SCE_ASCIIDOC_PASSBK:
            if (sc.atLineStart && sc.Match("++++") && IsNewline(sc.GetRelative(4))) {
                sc.Forward(4);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            break;

        // Comment block
        case SCE_ASCIIDOC_COMMENTBK:
            if (sc.atLineStart && sc.Match("////") && IsNewline(sc.GetRelative(4))) {
                sc.Forward(4);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            break;

        // Literal
        case SCE_ASCIIDOC_LITERAL:
            if (sc.ch == '+' && sc.chPrev != '\\') {
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            break;

        // Literal block
        case SCE_ASCIIDOC_LITERALBK:
            if (sc.atLineStart && sc.Match("....") && IsNewline(sc.GetRelative(4))) {
                sc.Forward(4);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            break;

        // Attribute
        case SCE_ASCIIDOC_ATTRIB:
            if (sc.ch == ':' && sc.chPrev != ' ' && sc.chNext == ' ') {
                sc.Forward();
                sc.SetState(SCE_ASCIIDOC_ATTRIBVAL);
            }
            break;

        // Macro
        case SCE_ASCIIDOC_MACRO:
            if (sc.ch == ']' && sc.chPrev != '\\') {
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            break;

        // Default
        case SCE_ASCIIDOC_DEFAULT:
            // Headers
            if (sc.atLineStart && sc.Match("====== ")) {
                sc.SetState(SCE_ASCIIDOC_HEADER6);
                sc.Forward(6);
            }
            else if (sc.atLineStart && sc.Match("===== ")) {
                sc.SetState(SCE_ASCIIDOC_HEADER5);
                sc.Forward(5);
            }
            else if (sc.atLineStart && sc.Match("==== ")) {
                sc.SetState(SCE_ASCIIDOC_HEADER4);
                sc.Forward(4);
            }
            else if (sc.atLineStart && sc.Match("=== ")) {
                sc.SetState(SCE_ASCIIDOC_HEADER3);
                sc.Forward(3);
            }
            else if (sc.atLineStart && sc.Match("== ")) {
                sc.SetState(SCE_ASCIIDOC_HEADER2);
                sc.Forward(2);
            }
            else if (sc.atLineStart && sc.Match("= ")) {
                sc.SetState(SCE_ASCIIDOC_HEADER1);
                sc.Forward(1);
            }
            // Unordered list item
            else if (sc.atLineStart && sc.Match("****** ")) {
                sc.SetState(SCE_ASCIIDOC_ULIST_ITEM);
                sc.Forward(6);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match("***** ")) {
                sc.SetState(SCE_ASCIIDOC_ULIST_ITEM);
                sc.Forward(5);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match("**** ")) {
                sc.SetState(SCE_ASCIIDOC_ULIST_ITEM);
                sc.Forward(4);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match("*** ")) {
                sc.SetState(SCE_ASCIIDOC_ULIST_ITEM);
                sc.Forward(3);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match("** ")) {
                sc.SetState(SCE_ASCIIDOC_ULIST_ITEM);
                sc.Forward(2);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match("* ")) {
                sc.SetState(SCE_ASCIIDOC_ULIST_ITEM);
                sc.Forward(1);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            // Ordered list item
            else if (sc.atLineStart && sc.Match("...... ")) {
                sc.SetState(SCE_ASCIIDOC_OLIST_ITEM);
                sc.Forward(6);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match("..... ")) {
                sc.SetState(SCE_ASCIIDOC_OLIST_ITEM);
                sc.Forward(5);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match(".... ")) {
                sc.SetState(SCE_ASCIIDOC_OLIST_ITEM);
                sc.Forward(4);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match("... ")) {
                sc.SetState(SCE_ASCIIDOC_OLIST_ITEM);
                sc.Forward(3);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match(".. ")) {
                sc.SetState(SCE_ASCIIDOC_OLIST_ITEM);
                sc.Forward(2);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            else if (sc.atLineStart && sc.Match(". ")) {
                sc.SetState(SCE_ASCIIDOC_OLIST_ITEM);
                sc.Forward(1);
                sc.SetState(SCE_ASCIIDOC_DEFAULT);
            }
            // Blockquote
            else if (sc.atLineStart && sc.Match("> ")) {
                sc.SetState(SCE_ASCIIDOC_BLOCKQUOTE);
                sc.Forward();
            }
            // Link
            else if (!sc.atLineStart && sc.ch == '[' && sc.chPrev != '\\' && sc.chNext != ' ') {
                sc.Forward();
                sc.SetState(SCE_ASCIIDOC_LINK);
                freezeCursor = true;
            }
            // Code block
            else if (sc.atLineStart && sc.Match("----") && IsNewline(sc.GetRelative(4))) {
                sc.SetState(SCE_ASCIIDOC_CODEBK);
                sc.Forward(4);
            }
            // Passthrough block
            else if (sc.atLineStart && sc.Match("++++") && IsNewline(sc.GetRelative(4))) {
                sc.SetState(SCE_ASCIIDOC_PASSBK);
                sc.Forward(4);
            }
            // Comment block
            else if (sc.atLineStart && sc.Match("////") && IsNewline(sc.GetRelative(4))) {
                sc.SetState(SCE_ASCIIDOC_COMMENTBK);
                sc.Forward(4);
            }
            // Comment
            else if (sc.atLineStart && sc.Match("//")) {
                sc.SetState(SCE_ASCIIDOC_COMMENT);
                sc.Forward();
            }
            // Literal
            else if (sc.ch == '+' && sc.chPrev != '\\' && sc.chNext != ' ' && AtTermStart(sc)) {
                sc.Forward();
                sc.SetState(SCE_ASCIIDOC_LITERAL);
                freezeCursor = true;
            }
            // Literal block
            else if (sc.atLineStart && sc.Match("....") && IsNewline(sc.GetRelative(4))) {
                sc.SetState(SCE_ASCIIDOC_LITERALBK);
                sc.Forward(4);
            }
            // Attribute
            else if (sc.atLineStart && sc.ch == ':' && sc.chNext != ' ') {
                sc.SetState(SCE_ASCIIDOC_ATTRIB);
            }
            // Strong
            else if (sc.Match("**") && sc.GetRelative(2) != ' ') {
                sc.SetState(SCE_ASCIIDOC_STRONG2);
                sc.Forward();
            }
            else if (sc.ch == '*' && sc.chNext != ' ' && AtTermStart(sc)) {
                sc.SetState(SCE_ASCIIDOC_STRONG1);
            }
            // Emphasis
            else if (sc.Match("__") && sc.GetRelative(2) != ' ') {
                sc.SetState(SCE_ASCIIDOC_EM2);
                sc.Forward();
            }
            else if (sc.ch == '_' && sc.chNext != ' ' && AtTermStart(sc)) {
                sc.SetState(SCE_ASCIIDOC_EM1);
            }
            // Macro
            else if (sc.atLineStart && sc.ch == '[' && sc.chNext != ' ') {
                sc.Forward();
                sc.SetState(SCE_ASCIIDOC_MACRO);
                freezeCursor = true;
            }
            else {
                int i = 0;
                bool found = false;
                while (!found && MacroList[i].name != NULL) {
                    if (MacroList[i].start)
                        found = sc.atLineStart && sc.Match(MacroList[i].name);
                    else
                        found = sc.Match(MacroList[i].name);
                    if (found) {
                        sc.SetState(SCE_ASCIIDOC_MACRO);
                        sc.Forward(MacroList[i].len1);
                        sc.SetState(SCE_ASCIIDOC_DEFAULT);
                        if (MacroList[i].len2 > 1)
                            sc.Forward(MacroList[i].len2 - 1);
                    }
                    i++;
                }
            }
            break;
        }
        // Advance if not holding back the cursor for this iteration.
        if (!freezeCursor)
            sc.Forward();
        freezeCursor = false;
    }
    sc.Complete();
}

LexerModule lmAsciidoc(SCLEX_ASCIIDOC, ColorizeAsciidocDoc, "asciidoc");
