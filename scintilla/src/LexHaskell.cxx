/******************************************************************
 *    LexHaskell.cxx
 *
 *    A haskell lexer for the scintilla code control.
 *    Some stuff "lended" from LexPython.cxx and LexCPP.cxx.
 *    External lexer stuff inspired from the caml external lexer.
 *
 *    Written by Tobias Engvall - tumm at dtek dot chalmers dot se
 *
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

#ifdef BUILD_AS_EXTERNAL_LEXER

#include "ExternalLexer.h"
#include "WindowAccessor.h"

#define BUILD_EXTERNAL_LEXER 0

#endif

// Max level of nested comments
#define SCE_HA_COMMENTMAX SCE_HA_COMMENTBLOCK3


enum kwType { kwOther, kwClass, kwData, kwInstance, kwImport, kwModule, kwType};

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

   int kwLast = kwOther;

   StyleContext sc(startPos, length, initStyle, styler);

   for (; sc.More(); sc.Forward()) {

      // Check for state end
         // Operator
      if (sc.state == SCE_HA_OPERATOR) {
         kwLast = kwOther;
         sc.SetState(SCE_HA_DEFAULT);
      }
         // String
      else if (sc.state == SCE_HA_STRING) {
         if (sc.ch == '\"') {
            sc.ForwardSetState(SCE_HA_DEFAULT);
         }
      }
         // Char
      else if (sc.state == SCE_HA_CHARACTER) {
         if (sc.ch == '\'') {
            sc.ForwardSetState(SCE_HA_DEFAULT);
         }
      }
         // Number
      else if (sc.state == SCE_HA_NUMBER) {
         if (!IsADigit(sc.ch)) {
            sc.SetState(SCE_HA_DEFAULT);
         }
      }
         // Types, constructors, etc.
      else if (sc.state == SCE_HA_CAPITAL) {
         if (!IsAWordChar(sc.ch) || sc.ch == '.') {
            sc.SetState(SCE_HA_DEFAULT);
         }
      }
         // Identifier
      else if (sc.state == SCE_HA_IDENTIFIER) {
         if (!IsAWordChar(sc.ch)) {
            char s[100];
            sc.GetCurrent(s, sizeof(s));
            int style = SCE_HA_IDENTIFIER;
            if ((kwLast == kwImport) || (strcmp(s,"qualified") == 0) || (strcmp(s,"as") == 0)) {
               style = SCE_HA_IMPORT;
            } else if (keywords.InList(s)) {
               style = SCE_HA_KEYWORD;
            } else if (kwLast == kwData) {
               style = SCE_HA_DATA;
            } else if (kwLast == kwClass) {
               style = SCE_HA_CLASS;
            } else if (kwLast == kwModule) {
               style = SCE_HA_MODULE;
            } else if (isupper(s[0])) {
               style = SCE_HA_CAPITAL;
            }
            sc.ChangeState(style);
            sc.SetState(SCE_HA_DEFAULT);
            if (style == SCE_HA_KEYWORD) {
               if (0 == strcmp(s, "class"))
                  kwLast = kwClass;
               else if (0 == strcmp(s, "data"))
                  kwLast = kwData;
               else if (0 == strcmp(s, "instance"))
                  kwLast = kwInstance;
               else if (0 == strcmp(s, "import"))
                  kwLast = kwImport;
               else if (0 == strcmp(s, "module"))
                  kwLast = kwModule;
               else
                  kwLast = kwOther;
            } else if (style == SCE_HA_CLASS || style == SCE_HA_IMPORT ||
                       style == SCE_HA_MODULE || style == SCE_HA_CAPITAL ||
                       style == SCE_HA_DATA || style == SCE_HA_INSTANCE) {
               kwLast = kwOther;
            }
         }
      }
         // Comments
            // Oneliner
      else if (sc.state == SCE_HA_COMMENTLINE) {
         if (IsNewline(sc.ch))
            sc.SetState(SCE_HA_DEFAULT);
      }
            // Nested
      else if (sc.state >= SCE_HA_COMMENTBLOCK) {
         if (sc.Match("{-")) {
            if (sc.state < SCE_HA_COMMENTMAX)
               sc.SetState(sc.state + 1);
         }
         else if (sc.Match("-}")) {
            sc.Forward();
            if (sc.state == SCE_HA_COMMENTBLOCK)
               sc.ForwardSetState(SCE_HA_DEFAULT);
            else
               sc.ForwardSetState(sc.state - 1);
         }
      }
      // New state?
      if (sc.state == SCE_HA_DEFAULT) {
         // Digit
         if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
            sc.SetState(SCE_HA_NUMBER);
         }
         // Comment line
         else if (sc.Match("--")) {
            sc.SetState(SCE_HA_COMMENTLINE);
         // Comment block
         }
         else if (sc.Match("{-")) {
            sc.SetState(SCE_HA_COMMENTBLOCK);
         }
         // String
         else if (sc.Match('\"')) {
            sc.SetState(SCE_HA_STRING);
         }
         // Character
         else if (sc.Match('\'') && IsWhitespace(sc.GetRelative(-1)) ) {
            sc.SetState(SCE_HA_CHARACTER);
         }
         // Stringstart
         else if (sc.Match('\"')) {
            sc.SetState(SCE_HA_STRING);
         }
         // Operator
         else if (isascii(sc.ch) && isoperator(static_cast<char>(sc.ch))) {
            sc.SetState(SCE_HA_OPERATOR);
         }
         // Keyword
         else if (IsAWordStart(sc.ch)) {
               sc.SetState(SCE_HA_IDENTIFIER);
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
   PropSet ps;
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

