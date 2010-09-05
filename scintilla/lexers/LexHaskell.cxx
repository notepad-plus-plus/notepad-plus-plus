/******************************************************************
 *    LexHaskell.cxx
 *
 *    A haskell lexer for the scintilla code control.
 *    Some stuff "lended" from LexPython.cxx and LexCPP.cxx.
 *    External lexer stuff inspired from the caml external lexer.
 *
 *    Written by Tobias Engvall - tumm at dtek dot chalmers dot se
 *
 *    Several bug fixes by Krasimir Angelov - kr.angelov at gmail.com
 *
 *    TODO:
 *    * Implement a folder :)
 *    * Nice Character-lexing (stuff inside '\''), LexPython has
 *      this.
 *
 *
 *****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

#ifdef BUILD_AS_EXTERNAL_LEXER

#include "ExternalLexer.h"
#include "WindowAccessor.h"

#define BUILD_EXTERNAL_LEXER 0

#endif

#define HA_MODE_DEFAULT     0
#define HA_MODE_IMPORT1     1
#define HA_MODE_IMPORT2     2
#define HA_MODE_IMPORT3     3
#define HA_MODE_MODULE      4
#define HA_MODE_FFI         5
#define HA_MODE_TYPE        6

static inline bool IsNewline(const int ch) {
   return (ch == '\n' || ch == '\r');
}

static inline bool IsWhitespace(const int ch) {
   return (  ch == ' '
          || ch == '\t'
          || IsNewline(ch) );
}

static inline bool IsAWordStart(const int ch) {
   return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static inline bool IsAWordChar(const int ch) {
   return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_' || ch == '\'');
}

static void ColorizeHaskellDoc(unsigned int startPos, int length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {

   WordList &keywords = *keywordlists[0];
   WordList &ffi      = *keywordlists[1];

   StyleContext sc(startPos, length, initStyle, styler);

   int lineCurrent = styler.GetLine(startPos);
   int state = lineCurrent ? styler.GetLineState(lineCurrent-1)
                           : HA_MODE_DEFAULT;
   int mode  = state & 0xF;
   int xmode = state >> 4;

   while (sc.More()) {
      // Check for state end

         // Operator
      if (sc.state == SCE_HA_OPERATOR) {
         if (isascii(sc.ch) && isoperator(static_cast<char>(sc.ch))) {
            sc.Forward();
         } else {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.ChangeState(SCE_HA_DEFAULT);
         }
      }
         // String
      else if (sc.state == SCE_HA_STRING) {
         if (sc.ch == '\"') {
			sc.Forward();
            styler.ColourTo(sc.currentPos-1, sc.state);
            sc.ChangeState(SCE_HA_DEFAULT);
         } else if (sc.ch == '\\') {
            sc.Forward(2);
         } else if (sc.atLineEnd) {
			styler.ColourTo(sc.currentPos-1, sc.state);
			sc.ChangeState(SCE_HA_DEFAULT);
		 } else {
			sc.Forward();
		 }
      }
         // Char
      else if (sc.state == SCE_HA_CHARACTER) {
         if (sc.ch == '\'') {
			sc.Forward();
            styler.ColourTo(sc.currentPos-1, sc.state);
            sc.ChangeState(SCE_HA_DEFAULT);
         } else if (sc.ch == '\\') {
            sc.Forward(2);
         } else if (sc.atLineEnd) {
			styler.ColourTo(sc.currentPos-1, sc.state);
			sc.ChangeState(SCE_HA_DEFAULT);
		 } else {
			sc.Forward();
		 }
      }
         // Number
      else if (sc.state == SCE_HA_NUMBER) {
         if (IsADigit(sc.ch, xmode)) {
            sc.Forward();
         } else if ((xmode == 10) &&
                    (sc.ch == 'e' || sc.ch == 'E') &&
                    (IsADigit(sc.chNext) || sc.chNext == '+' || sc.chNext == '-')) {
			sc.Forward();
			if (sc.ch == '+' || sc.ch == '-')
				sc.Forward();
         } else {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.ChangeState(SCE_HA_DEFAULT);
         }
      }
         // Identifier
      else if (sc.state == SCE_HA_IDENTIFIER) {
         if (IsAWordChar(sc.ch)) {
            sc.Forward();
         } else {
            char s[100];
            sc.GetCurrent(s, sizeof(s));
            int style = sc.state;
            int new_mode = 0;
            if (keywords.InList(s)) {
               style = SCE_HA_KEYWORD;
            } else if (isupper(s[0])) {
               if (mode >= HA_MODE_IMPORT1 && mode <= HA_MODE_IMPORT3) {
                  style    = SCE_HA_MODULE;
                  new_mode = HA_MODE_IMPORT2;
               } else if (mode == HA_MODE_MODULE)
                  style = SCE_HA_MODULE;
               else
                  style = SCE_HA_CAPITAL;
            } else if (mode == HA_MODE_IMPORT1 &&
                       strcmp(s,"qualified") == 0) {
                style    = SCE_HA_KEYWORD;
                new_mode = HA_MODE_IMPORT1;
            } else if (mode == HA_MODE_IMPORT2) {
                if (strcmp(s,"as") == 0) {
                   style    = SCE_HA_KEYWORD;
                   new_mode = HA_MODE_IMPORT3;
               } else if (strcmp(s,"hiding") == 0) {
                   style     = SCE_HA_KEYWORD;
               }
            } else if (mode == HA_MODE_FFI) {
			   if (ffi.InList(s)) {
                  style = SCE_HA_KEYWORD;
                  new_mode = HA_MODE_FFI;
               }
            }
            else if (mode == HA_MODE_TYPE) {
               if (strcmp(s,"family") == 0)
                  style    = SCE_HA_KEYWORD;
			}
            styler.ColourTo(sc.currentPos - 1, style);
            if (strcmp(s,"import") == 0 && mode != HA_MODE_FFI)
               new_mode = HA_MODE_IMPORT1;
            else if (strcmp(s,"module") == 0)
               new_mode = HA_MODE_MODULE;
            else if (strcmp(s,"foreign") == 0)
               new_mode = HA_MODE_FFI;
            else if (strcmp(s,"type") == 0)
               new_mode = HA_MODE_TYPE;
            sc.ChangeState(SCE_HA_DEFAULT);
            mode = new_mode;
         }
      }

         // Comments
            // Oneliner
      else if (sc.state == SCE_HA_COMMENTLINE) {
         if (sc.atLineEnd) {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.ChangeState(SCE_HA_DEFAULT);
         } else {
            sc.Forward();
         }
      }
            // Nested
      else if (sc.state == SCE_HA_COMMENTBLOCK) {
         if (sc.Match("{-")) {
            sc.Forward(2);
            xmode++;
         }
         else if (sc.Match("-}")) {
            sc.Forward(2);
            xmode--;
            if (xmode == 0) {
               styler.ColourTo(sc.currentPos - 1, sc.state);
               sc.ChangeState(SCE_HA_DEFAULT);
            }
         } else {
            if (sc.atLineEnd) {
				// Remember the line state for future incremental lexing
				styler.SetLineState(lineCurrent, (xmode << 4) | mode);
				lineCurrent++;
			}
            sc.Forward();
         }
      }
      // New state?
      if (sc.state == SCE_HA_DEFAULT) {
         // Digit
         if (IsADigit(sc.ch) ||
             (sc.ch == '.' && IsADigit(sc.chNext)) ||
             (sc.ch == '-' && IsADigit(sc.chNext))) {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.ChangeState(SCE_HA_NUMBER);
            if (sc.ch == '0' && (sc.chNext == 'X' || sc.chNext == 'x')) {
				// Match anything starting with "0x" or "0X", too
				sc.Forward(2);
				xmode = 16;
            } else if (sc.ch == '0' && (sc.chNext == 'O' || sc.chNext == 'o')) {
				// Match anything starting with "0x" or "0X", too
				sc.Forward(2);
				xmode = 8;
            } else {
				sc.Forward();
				xmode = 10;
			}
            mode = HA_MODE_DEFAULT;
         }
         // Comment line
         else if (sc.Match("--")) {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.Forward(2);
            sc.ChangeState(SCE_HA_COMMENTLINE);
         // Comment block
         }
         else if (sc.Match("{-")) {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.Forward(2);
            sc.ChangeState(SCE_HA_COMMENTBLOCK);
            xmode = 1;
         }
         // String
         else if (sc.Match('\"')) {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.Forward();
            sc.ChangeState(SCE_HA_STRING);
         }
         // Character
         else if (sc.Match('\'')) {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.Forward();
            sc.ChangeState(SCE_HA_CHARACTER);
         }
         else if (sc.ch == '(' || sc.ch == ')' ||
                  sc.ch == '{' || sc.ch == '}' ||
                  sc.ch == '[' || sc.ch == ']') {
			styler.ColourTo(sc.currentPos - 1, sc.state);
			sc.Forward();
			styler.ColourTo(sc.currentPos - 1, SCE_HA_OPERATOR);
			mode = HA_MODE_DEFAULT;
		 }
         // Operator
         else if (isascii(sc.ch) && isoperator(static_cast<char>(sc.ch))) {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.Forward();
            sc.ChangeState(SCE_HA_OPERATOR);
            mode = HA_MODE_DEFAULT;
         }
         // Keyword
         else if (IsAWordStart(sc.ch)) {
            styler.ColourTo(sc.currentPos - 1, sc.state);
            sc.Forward();
            sc.ChangeState(SCE_HA_IDENTIFIER);
         } else {
            if (sc.atLineEnd) {
				// Remember the line state for future incremental lexing
				styler.SetLineState(lineCurrent, (xmode << 4) | mode);
				lineCurrent++;
			}
            sc.Forward();
         }
      }
   }
   sc.Complete();
}

// External stuff - used for dynamic-loading, not implemented in wxStyledTextCtrl yet.
// Inspired by the caml external lexer - Credits to Robert Roessler - http://www.rftp.com
#ifdef BUILD_EXTERNAL_LEXER
static const char* LexerName = "haskell";

void EXT_LEXER_DECL Lex(unsigned int lexer, unsigned int startPos, int length, int initStyle,
                        char *words[], WindowID window, char *props)
{
   PropSetSimple ps;
   ps.SetMultiple(props);
   WindowAccessor wa(window, ps);

   int nWL = 0;
   for (; words[nWL]; nWL++) ;
   WordList** wl = new WordList* [nWL + 1];
   int i = 0;
   for (; i<nWL; i++)
   {
      wl[i] = new WordList();
      wl[i]->Set(words[i]);
   }
   wl[i] = 0;

   ColorizeHaskellDoc(startPos, length, initStyle, wl, wa);
   wa.Flush();
   for (i=nWL-1;i>=0;i--)
      delete wl[i];
   delete [] wl;
}

void EXT_LEXER_DECL Fold (unsigned int lexer, unsigned int startPos, int length, int initStyle,
                        char *words[], WindowID window, char *props)
{

}

int EXT_LEXER_DECL GetLexerCount()
{
   return 1;
}

void EXT_LEXER_DECL GetLexerName(unsigned int Index, char *name, int buflength)
{
   if (buflength > 0) {
      buflength--;
      int n = strlen(LexerName);
      if (n > buflength)
         n = buflength;
      memcpy(name, LexerName, n), name[n] = '\0';
   }
}
#endif

LexerModule lmHaskell(SCLEX_HASKELL, ColorizeHaskellDoc, "haskell");
