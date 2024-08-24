// Scintilla source code edit control
/** @file LexProgress.cxx
 **  Lexer for Progress 4GL.
 ** Based on LexCPP.cxx of Neil Hodgson <neilh@scintilla.org>
  **/
// Copyright 2006-2016 by Yuval Papish <Yuval@YuvCom.com>
// The License.txt file describes the conditions under which this software may be distributed.

/** TODO:

SpeedScript support in html lexer
Differentiate between labels and variables
  Option 1: By symbols table
  Option 2: As a single unidentified symbol in a sytactical line

**/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "SparseState.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {
   // Use an unnamed namespace to protect the functions and classes from name conflicts

   void highlightTaskMarker(StyleContext &sc, LexAccessor &styler, WordList &markerList){
      if ((isoperator(sc.chPrev) || IsASpace(sc.chPrev)) && markerList.Length()) {
         const int lengthMarker = 50;
         char marker[lengthMarker+1];
         Sci_Position currPos = (Sci_Position) sc.currentPos;
         Sci_Position i = 0;
         while (i < lengthMarker) {
            char ch = styler.SafeGetCharAt(currPos + i);
            if (IsASpace(ch) || isoperator(ch)) {
               break;
            }
            marker[i] = ch;
            i++;
         }
         marker[i] = '\0';
         if (markerList.InListAbbreviated (marker,'(')) {
            sc.SetState(SCE_ABL_TASKMARKER);
         }
      }
   }

   bool IsStreamCommentStyle(int style) {
      return style == SCE_ABL_COMMENT;
             // style == SCE_ABL_LINECOMMENT;  Only block comments are used for folding
   }

   // Options used for LexerABL
   struct OptionsABL {
      bool fold;
      bool foldSyntaxBased;
      bool foldComment;
      bool foldCommentMultiline;
      bool foldCompact;
      OptionsABL() {
         fold = false;
         foldSyntaxBased = true;
         foldComment = true;
         foldCommentMultiline = true;
         foldCompact = false;
      }
   };

   const char *const ablWordLists[] = {
               "Primary keywords and identifiers",
               "Keywords that opens a block, only when used to begin a syntactic line",
               "Keywords that opens a block anywhere in a syntactic line",
               "Task Marker", /* "END MODIFY START TODO" */
               0,
   };

   struct OptionSetABL : public OptionSet<OptionsABL> {
      OptionSetABL() {
         DefineProperty("fold", &OptionsABL::fold);

         DefineProperty("fold.abl.syntax.based", &OptionsABL::foldSyntaxBased,
            "Set this property to 0 to disable syntax based folding.");

         DefineProperty("fold.comment", &OptionsABL::foldComment,
            "This option enables folding multi-line comments and explicit fold points when using the ABL lexer. ");

         DefineProperty("fold.abl.comment.multiline", &OptionsABL::foldCommentMultiline,
            "Set this property to 0 to disable folding multi-line comments when fold.comment=1.");

         DefineProperty("fold.compact", &OptionsABL::foldCompact);

         DefineWordListSets(ablWordLists);
      }
   };
}

class LexerABL : public DefaultLexer {
   CharacterSet setWord;
   CharacterSet setNegationOp;
   CharacterSet setArithmethicOp;
   CharacterSet setRelOp;
   CharacterSet setLogicalOp;
   CharacterSet setWordStart;
   WordList keywords1;      // regular keywords
   WordList keywords2;      // block opening keywords, only when isSentenceStart
   WordList keywords3;      // block opening keywords
   WordList keywords4;      // Task Marker
   OptionsABL options;
   OptionSetABL osABL;
public:
   LexerABL() :
      DefaultLexer("abl", SCLEX_PROGRESS),
      setWord(CharacterSet::setAlphaNum, "_", 0x80, true),
      setNegationOp(CharacterSet::setNone, "!"),
      setArithmethicOp(CharacterSet::setNone, "+-/*%"),
      setRelOp(CharacterSet::setNone, "=!<>"),
      setLogicalOp(CharacterSet::setNone, "|&"){
   }
   virtual ~LexerABL() {
   }
   void SCI_METHOD Release() override {
      delete this;
   }
   int SCI_METHOD Version() const override {
      return lvRelease5;
   }
   const char * SCI_METHOD PropertyNames() override {
      return osABL.PropertyNames();
   }
   int SCI_METHOD PropertyType(const char *name) override {
      return osABL.PropertyType(name);
   }
   const char * SCI_METHOD DescribeProperty(const char *name) override {
      return osABL.DescribeProperty(name);
   }
   Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override ;
   const char * SCI_METHOD PropertyGet(const char *key) override {
	   return osABL.PropertyGet(key);
   }

   const char * SCI_METHOD DescribeWordListSets() override {
      return osABL.DescribeWordListSets();
   }
   Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
   void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
   void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

   void * SCI_METHOD PrivateCall(int, void *) override {
      return 0;
   }
   int SCI_METHOD LineEndTypesSupported() override {
      return SC_LINE_END_TYPE_DEFAULT;
   }
   static ILexer5 *LexerFactoryABL() {
      return new LexerABL();
   }
};

Sci_Position SCI_METHOD LexerABL::PropertySet(const char *key, const char *val) {
   if (osABL.PropertySet(&options, key, val)) {
      return 0;
   }
   return -1;
}

Sci_Position SCI_METHOD LexerABL::WordListSet(int n, const char *wl) {
   WordList *wordListN = 0;
   switch (n) {
   case 0:
      wordListN = &keywords1;
      break;
   case 1:
      wordListN = &keywords2;
      break;
   case 2:
      wordListN = &keywords3;
      break;
   case 3:
      wordListN = &keywords4;
      break;
   }
   Sci_Position firstModification = -1;
   if (wordListN) {
      WordList wlNew;
      wlNew.Set(wl);
      if (*wordListN != wlNew) {
         wordListN->Set(wl);
         firstModification = 0;
      }
   }
   return firstModification;
}

void SCI_METHOD LexerABL::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
   LexAccessor styler(pAccess);

   setWordStart = CharacterSet(CharacterSet::setAlpha, "_", 0x80, true);

   int visibleChars1 = 0;
   int styleBeforeTaskMarker = SCE_ABL_DEFAULT;
   bool continuationLine = false;
   int commentNestingLevel = 0;
   bool isSentenceStart = true;
   bool possibleOOLChange = false;

   Sci_Position lineCurrent = styler.GetLine(startPos);
   if (initStyle == SCE_ABL_PREPROCESSOR) {
      // Set continuationLine if last character of previous line is '~'
      if (lineCurrent > 0) {
         Sci_Position endLinePrevious = styler.LineEnd(lineCurrent-1);
         if (endLinePrevious > 0) {
            continuationLine = styler.SafeGetCharAt(endLinePrevious-1) == '~';
         }
      }
   }

   // Initialize the block comment /* */ nesting level if lexing is starting inside
   // a block comment.
   if (initStyle == SCE_ABL_COMMENT && lineCurrent > 0) {
       commentNestingLevel = styler.GetLineState(lineCurrent - 1);
   }

    // Look back to set variables that are actually invisible secondary states. The reason to avoid formal states is to cut down on state's bits
   if (startPos > 0) {
      Sci_Position back = startPos;
      bool checkIsSentenceStart = (initStyle == SCE_ABL_DEFAULT || initStyle == SCE_ABL_IDENTIFIER);
      char ch;
      char st;
      char chPrev;
      char chPrev_1;
      char chPrev_2;
      char chPrev_3;

      while (back >= 0 && checkIsSentenceStart) {
         ch = styler.SafeGetCharAt(back);
         styler.Flush();  // looking at styles so need to flush
         st = styler.StyleAt(back);

         chPrev = styler.SafeGetCharAt(back-1);
         // isSentenceStart is a non-visible state, used to identify where statements and preprocessor declerations can start
         if (st != SCE_ABL_COMMENT && st != SCE_ABL_LINECOMMENT && st != SCE_ABL_CHARACTER  && st != SCE_ABL_STRING ) {
            chPrev_1 = styler.SafeGetCharAt(back-2);
            chPrev_2 = styler.SafeGetCharAt(back-3);
            chPrev_3 = styler.SafeGetCharAt(back-4);
            if ((chPrev == '.' || chPrev == ':' || chPrev == '}' ||
               (chPrev_3 == 'e' && chPrev_2 == 'l' && chPrev_1 == 's' &&  chPrev == 'e') ||
               (chPrev_3 == 't' && chPrev_2 == 'h' && chPrev_1 == 'e' &&  chPrev == 'n')) &&
               (IsASpace(ch) || (ch == '/' && styler.SafeGetCharAt(back+1) == '*'))
               ) {
                  checkIsSentenceStart = false;
                  isSentenceStart = true;
            }
            else if (IsASpace(chPrev) && ch == '{') {
               checkIsSentenceStart = false;
               isSentenceStart = false;
            }
         }
         --back;
      }
   }

   StyleContext sc(startPos, length, initStyle, styler, static_cast<unsigned char>(0xff));
   Sci_Position lineEndNext = styler.LineEnd(lineCurrent);

   for (; sc.More();) {

      if (sc.atLineStart) {
         visibleChars1 = 0;
      }

      if (sc.atLineEnd) {
          // Update the line state, so it can be seen by next line
          if (sc.state == SCE_ABL_COMMENT) {
              // Inside a block comment; store the nesting level
              styler.SetLineState(lineCurrent, commentNestingLevel);
          }
          else {
              // Not inside a block comment; nesting level is 0
              styler.SetLineState(lineCurrent, 0);
          }

          lineCurrent++;
          lineEndNext = styler.LineEnd(lineCurrent);
      }

      // Handle line continuation generically.
      if (sc.ch == '~') {
         if (static_cast<Sci_Position>((sc.currentPos+1)) >= lineEndNext) {
            lineCurrent++;
            lineEndNext = styler.LineEnd(lineCurrent);
            sc.Forward();
            if (sc.ch == '\r' && sc.chNext == '\n') {
               sc.Forward();
            }
            continuationLine = true;
            sc.Forward();
            continue;
         }
      }

      const bool atLineEndBeforeSwitch = sc.atLineEnd;
      // Determine if the current state should terminate.
      switch (sc.state) {
         case SCE_ABL_OPERATOR:
            sc.SetState(SCE_ABL_DEFAULT);
            break;
         case SCE_ABL_NUMBER:
            // We accept almost anything because of hex. and maybe number suffixes and scientific notations in the future
            if (!(setWord.Contains(sc.ch)
				   || ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E' ||
				                                          sc.chPrev == 'p' || sc.chPrev == 'P')))) {
               sc.SetState(SCE_ABL_DEFAULT);
            }
            break;
         case SCE_ABL_IDENTIFIER:
            if (sc.atLineStart || sc.atLineEnd || (!setWord.Contains(sc.ch) && sc.ch != '-')) {
               char s[1000];
               sc.GetCurrentLowered(s, sizeof(s));
               bool isLastWordEnd = (s[0] == 'e' && s[1] =='n' && s[2] == 'd' && !IsAlphaNumeric(s[3]) && s[3] != '-');  // helps to identify "end trigger" phrase
               if ((isSentenceStart && keywords2.InListAbbreviated (s,'(')) || (!isLastWordEnd && keywords3.InListAbbreviated (s,'('))) {
                  sc.ChangeState(SCE_ABL_BLOCK);
                  isSentenceStart = false;
               }
               else if (keywords1.InListAbbreviated (s,'(')) {
                  if (isLastWordEnd ||
                     (s[0] == 'f' && s[1] =='o' && s[2] == 'r' && s[3] == 'w' && s[4] =='a' && s[5] == 'r' && s[6] == 'd'&& !IsAlphaNumeric(s[7]))) {
                     sc.ChangeState(SCE_ABL_END);
                     isSentenceStart = false;
                  }
                  else if ((s[0] == 'e' && s[1] =='l' && s[2] == 's' && s[3] == 'e') ||
                         (s[0] == 't' && s[1] =='h' && s[2] == 'e' && s[3] == 'n')) {
                     sc.ChangeState(SCE_ABL_WORD);
                     isSentenceStart = true;
                  }
                  else {
                     sc.ChangeState(SCE_ABL_WORD);
                     isSentenceStart = false;
                  }
               }
               sc.SetState(SCE_ABL_DEFAULT);
            }
            break;
         case SCE_ABL_PREPROCESSOR:
            if (sc.atLineStart && !continuationLine) {
               sc.SetState(SCE_ABL_DEFAULT);
               // Force Scintilla to acknowledge changed stated even though this change might happen outside of the current line
               possibleOOLChange = true;
               isSentenceStart = true;
            }
            break;
         case SCE_ABL_LINECOMMENT:
            if (sc.atLineStart && !continuationLine) {
               sc.SetState(SCE_ABL_DEFAULT);
               isSentenceStart = true;
            } else {
               styleBeforeTaskMarker = SCE_ABL_LINECOMMENT;
               highlightTaskMarker(sc, styler, keywords4);
            }
            break;
         case SCE_ABL_TASKMARKER:
            if (isoperator(sc.ch) || IsASpace(sc.ch)) {
               sc.SetState(styleBeforeTaskMarker);
               styleBeforeTaskMarker = SCE_ABL_DEFAULT;
            }
            // fall through
         case SCE_ABL_COMMENT:
            if (sc.Match('*', '/')) {
               sc.Forward();
               commentNestingLevel--;
               if (commentNestingLevel == 0) {
                  sc.ForwardSetState(SCE_ABL_DEFAULT);
                  possibleOOLChange = true;
               }
            } else if (sc.Match('/', '*')) {
               commentNestingLevel++;
               sc.Forward();
            }
            if (commentNestingLevel > 0) {
               styleBeforeTaskMarker = SCE_ABL_COMMENT;
               possibleOOLChange = true;
               highlightTaskMarker(sc, styler, keywords4);
            }
            break;
         case SCE_ABL_STRING:
            if (sc.ch == '~') {
               sc.Forward(); // Skip a character after a tilde
            } else if (sc.ch == '\"') {
                  sc.ForwardSetState(SCE_ABL_DEFAULT);
            }
            break;
         case SCE_ABL_CHARACTER:
            if (sc.ch == '~') {
               sc.Forward(); // Skip a character after a tilde
            } else if (sc.ch == '\'') {
                  sc.ForwardSetState(SCE_ABL_DEFAULT);
            }
            break;
      }

      if (sc.atLineEnd && !atLineEndBeforeSwitch) {
         // State exit processing consumed characters up to end of line.
         lineCurrent++;
         lineEndNext = styler.LineEnd(lineCurrent);
      }

      // Determine if a new state should be entered.
      if (sc.state == SCE_ABL_DEFAULT) {
         if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
               sc.SetState(SCE_ABL_NUMBER);
               isSentenceStart = false;
         } else if (!sc.atLineEnd && (setWordStart.Contains(sc.ch)) && sc.chPrev != '&') {
               sc.SetState(SCE_ABL_IDENTIFIER);
         } else if (sc.Match('/', '*')) {
            if (sc.chPrev == '.' || sc.chPrev == ':' || sc.chPrev == '}') {
               isSentenceStart = true;
            }
            sc.SetState(SCE_ABL_COMMENT);
            possibleOOLChange = true;
            commentNestingLevel++;
            sc.Forward();   // Eat the * so it isn't used for the end of the comment
         } else if (sc.ch == '\"') {
               sc.SetState(SCE_ABL_STRING);
               isSentenceStart = false;
         } else if (sc.ch == '\'') {
            sc.SetState(SCE_ABL_CHARACTER);
            isSentenceStart = false;
         } else if (sc.ch == '&' && visibleChars1 == 0 && isSentenceStart) {
            // Preprocessor commands are alone on their line
            sc.SetState(SCE_ABL_PREPROCESSOR);
            // Force Scintilla to acknowledge changed stated even though this change might happen outside of the current line
            possibleOOLChange = true;
            // Skip whitespace between & and preprocessor word
            do {
               sc.Forward();
            } while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
            if (sc.atLineEnd) {
               sc.SetState(SCE_ABL_DEFAULT);
            }
         } else if (sc.Match('/','/') && (IsASpace(sc.chPrev) || isSentenceStart)) {
            // Line comments are valid after a white space or EOL
            sc.SetState(SCE_ABL_LINECOMMENT);
            // Skip whitespace between // and preprocessor word
            do {
               sc.Forward();
            } while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
            if (sc.atLineEnd) {
               sc.SetState(SCE_ABL_DEFAULT);
            }
         } else if (isoperator(sc.ch)) {
            sc.SetState(SCE_ABL_OPERATOR);
            /*    This code allows highlight of handles. Alas, it would cause the phrase "last-event:function"
               to be recognized as a BlockBegin */
               isSentenceStart = false;
         }
         else if ((sc.chPrev == '.' || sc.chPrev == ':' || sc.chPrev == '}') && (IsASpace(sc.ch))) {
            isSentenceStart = true;
         }
      }
      if (!IsASpace(sc.ch)) {
         visibleChars1++;
      }
      continuationLine = false;
      sc.Forward();
   }
	if (possibleOOLChange)
		styler.ChangeLexerState(startPos, startPos + length);
   sc.Complete();
}


// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".

void SCI_METHOD LexerABL::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {

   if (!options.fold)
      return;

   LexAccessor styler(pAccess);

   Sci_PositionU endPos = startPos + length;
   int visibleChars = 0;
   Sci_Position lineCurrent = styler.GetLine(startPos);
   int levelCurrent = SC_FOLDLEVELBASE;
   if (lineCurrent > 0)
      levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
   Sci_PositionU lineStartNext = styler.LineStart(lineCurrent+1);
   int levelNext = levelCurrent;
   char chNext = styler[startPos];
   int styleNext = styler.StyleAt(startPos);
   int style = initStyle;
   for (Sci_PositionU i = startPos; i < endPos; i++) {
      chNext = static_cast<char>(tolower(chNext));  // check tolower
      char ch = chNext;
      chNext = styler.SafeGetCharAt(i+1);
      int stylePrev = style;
      style = styleNext;
      styleNext = styler.StyleAt(i+1);
      bool atEOL = i == (lineStartNext-1);
      if (options.foldComment && options.foldCommentMultiline && IsStreamCommentStyle(style)) {
         if (!IsStreamCommentStyle(stylePrev)) {
            levelNext++;
         } else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
            // Comments don't end at end of line and the next character may be unstyled.
            levelNext--;
         }
      }
      if (options.foldSyntaxBased) {
         if (style == SCE_ABL_BLOCK && !IsAlphaNumeric(chNext)) {
            levelNext++;
         }
         else if (style == SCE_ABL_END  && (ch == 'e' || ch == 'f')) {
            levelNext--;
         }
      }
      if (!IsASpace(ch))
         visibleChars++;
      if (atEOL || (i == endPos-1)) {
         int lev = levelCurrent | levelNext << 16;
         if (visibleChars == 0 && options.foldCompact)
            lev |= SC_FOLDLEVELWHITEFLAG;
         if (levelCurrent < levelNext)
            lev |= SC_FOLDLEVELHEADERFLAG;
         if (lev != styler.LevelAt(lineCurrent)) {
            styler.SetLevel(lineCurrent, lev);
         }
         lineCurrent++;
         lineStartNext = styler.LineStart(lineCurrent+1);
         levelCurrent = levelNext;
         if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length()-1))) {
            // There is an empty line at end of file so give it same level and empty
            styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
         }
         visibleChars = 0;
      }
   }
}

extern const LexerModule lmProgress(SCLEX_PROGRESS, LexerABL::LexerFactoryABL, "abl", ablWordLists);
