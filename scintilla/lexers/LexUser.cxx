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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <windows.h>

#include "Platform.h"

#include "ILexer.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "WordList.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "CharClassify.h"
#include "LexerModule.h"

#define KEYWORD_BOXHEADER 1
#define KEYWORD_FOLDCONTRACTED 2


static inline bool IsADigit(char ch) {
	return isascii(ch) && isdigit(ch);
}

static inline bool isspacechar(unsigned char ch) {
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

static bool isInOpList(WordList & opList, char op)
{
	for (int i = 0 ; i < opList.len ; i++)
		if (op == *(opList.words[i]))
			return true;
	return false;
}

static int cmpString(const void *a1, const void *a2) {
	// Can't work out the correct incantation to use modern casts here
	return strcmp(*(char**)(a1), *(char**)(a2));
}


static bool isInList(WordList & list, const char *s, bool specialMode, bool ignoreCase) 
{
	if (!list.words)
		return false;

	unsigned char firstChar = s[0];
	int j = list.starts[firstChar];

	if (j >= 0)
	{
		while ((ignoreCase?toupper(list.words[j][0]):list.words[j][0]) == (ignoreCase?toupper(s[0]):s[0]))
		{
			if (!list.words[j][1])
			{
				if (specialMode)
				{
					return true;
				}
				else
				{
					if (!s[1])
						return true;
				}
			}
			int a1 = ignoreCase?toupper(list.words[j][1]):list.words[j][1];
			int b1 = ignoreCase?toupper(s[1]):s[1];
			if (a1 == b1) 
			{
				
				const char *a = list.words[j] + 1;
				int An = ignoreCase?toupper((int)*a):(int)*a;
				
				const char *b = s + 1;
				int Bn = ignoreCase?toupper((int)*b):(int)*b;

				
				while (An && (An == Bn))
				{
					a++;
					An = ignoreCase?toupper((int)*a):(int)*a;
					b++;
					Bn = ignoreCase?toupper((int)*b):(int)*b;
				}
				if (specialMode)
				{
					if (!An)
						return true;
				}
				else
				{
					if (!An && !Bn)
						return true;
				}
			}
			j++;
		}
	}

	if (ignoreCase)
	{
		// if the 1st char is not a letter, no need to test one more time
		if (!isalpha(s[0]))
			return false;

		firstChar = isupper(s[0])?tolower(s[0]):toupper(s[0]);
		j = list.starts[firstChar];
		if (j >= 0) 
		{
			while (toupper(list.words[j][0]) == toupper(s[0])) 
			{
				if (!list.words[j][1])
				{
					if (specialMode)
					{
						return true;
					}
					else
					{
						if (!s[1])
							return true;
					}
				}
				int a1 = toupper(list.words[j][1]);
				int b1 = toupper(s[1]);
				if (a1 == b1) 
				{
					const char *a = list.words[j] + 1;
					int An = toupper((int)*a);

					const char *b = s + 1;
					int Bn = toupper((int)*b);

					while (An && (An == Bn))
					{
						a++;
						An = toupper((int)*a);
						b++;
						Bn = toupper((int)*b);
					}
					if (specialMode)
					{
						if (!*a)
							return true;
					}
					else
					{
						if (!*a && !*b)
							return true;
					}
				}
				j++;
			}
		}
	}

	return false;
}
/*
static void getRange(unsigned int start, unsigned int end, Accessor &styler, char *s, unsigned int len) 
{
	unsigned int i = 0;
	while ((i < end - start + 1) && (i < len-1)) 
	{
		s[i] = static_cast<char>(styler[start + i]);
		i++;
	}
	s[i] = '\0';
}
*/
static inline bool isAWordChar(const int ch) {
	//return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
	return ((ch > 0x20) && (ch <= 0xFF) && (ch != ' ') && (ch != '\n'));
}

static inline bool isAWordStart(const int ch) {
	return isAWordChar(ch);
}

static void ColouriseUserDoc(unsigned int startPos, int length, int initStyle, WordList *keywordlists[],
                            Accessor &styler) {

	// It seems that there're only 9 keywordlists
	WordList &keywords = *keywordlists[0];
	WordList &blockOpenWords = *keywordlists[1];
	WordList &blockCloseWords = *keywordlists[2];
	WordList &symbols = *keywordlists[3];
	WordList &comments = *keywordlists[4];
	WordList &keywords5 = *keywordlists[5];
	WordList &keywords6 = *keywordlists[6];
    WordList &keywords7 = *keywordlists[7];
	WordList &keywords8 = *keywordlists[8];
	//WordList &keywords9 = *keywordlists[9];
	//WordList &keywords10 = *keywordlists[10];

	int chPrevNonWhite = ' ';
	int visibleChars = 0;

	bool isCaseIgnored = styler.GetPropertyInt("userDefine.ignoreCase", 0) != 0;
	bool isCommentLineSymbol = styler.GetPropertyInt("userDefine.commentLineSymbol", 0) != 0;
	bool isCommentSymbol = styler.GetPropertyInt("userDefine.commentSymbol", 0) != 0;

	bool doPrefix4G1 = styler.GetPropertyInt("userDefine.g1Prefix", 0) != 0;
	bool doPrefix4G2 = styler.GetPropertyInt("userDefine.g2Prefix", 0) != 0;
	bool doPrefix4G3 = styler.GetPropertyInt("userDefine.g3Prefix", 0) != 0;
	bool doPrefix4G4 = styler.GetPropertyInt("userDefine.g4Prefix", 0) != 0;
	
	char delimOpen[3];
	char delimClose[3];
    bool escaped = false;
	char escapeChar = static_cast<char>(styler.GetPropertyInt("userDefine.escapeChar", 0));

	bool hasDot = false;
	bool hasExp = false;

	for (int i = 0 ; i < 3 ; i++)
	{
		delimOpen[i] = keywords.words[0][i] == '0'?'\0':keywords.words[0][i];
		delimClose[i] = keywords.words[0][i+3] == '0'?'\0':keywords.words[0][i+3];
	}

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward()) 
	{
		// Determine if the current state should terminate.
		switch (sc.state)
		{			
			case SCE_USER_NUMBER :
			{
				if (sc.ch == '.')
				{
					if (!hasDot)
					{
						hasDot = true;
						break;
					}
				}
				//if (!isAWordChar(sc.ch))
				if (sc.ch == 'e' || sc.ch == 'E')
				{
					if (!hasExp && sc.More())
					{
						hasExp = true;
						sc.Forward();
						if (IsADigit(sc.ch)) break;
						if (sc.More())
							if (sc.ch == '+' || sc.ch == '-')
							{
								sc.Forward();
								if (IsADigit(sc.ch)) break;
							}
					}
				}
				if (!IsADigit(sc.ch))
    			{
					sc.SetState(SCE_USER_DEFAULT);
					hasDot = false;
					hasExp = false;
				}
				break;
			}

			case SCE_USER_DELIMITER1 :
			{
				if (sc.ch == escapeChar)
					escaped = !escaped;
				else
				{
					if (delimClose[0] && (sc.ch == delimClose[0]) && !escaped)
                        sc.ForwardSetState(SCE_USER_DEFAULT);
                    escaped = false;
                }
				break;
			}

			case SCE_USER_DELIMITER2 :
			{
				if (sc.ch == escapeChar)
					escaped = !escaped;
				else
				{
					if (delimClose[0] && (sc.ch == delimClose[1]) && !escaped)
                        sc.ForwardSetState(SCE_USER_DEFAULT);
                    escaped = false;
				}
				break;
			}

			case SCE_USER_DELIMITER3 :
			{
				if (sc.ch == escapeChar)
					escaped = !escaped;
				else
				{
					if (delimClose[0] && (sc.ch == delimClose[2]) && !escaped)
                        sc.ForwardSetState(SCE_USER_DEFAULT);
                    escaped = false;
                }
				break;
			}
			
			case SCE_USER_IDENTIFIER : 
			{
				bool isSymbol = isInOpList(symbols, sc.ch);

				if (!isAWordChar(sc.ch)  || isSymbol)
				{
					bool doDefault = true;
					const int tokenLen = 100;
					char s[tokenLen];
					sc.GetCurrent(s, sizeof(s));
					char commentLineStr[tokenLen+10] = "0";
					char *p = commentLineStr+1;
					strcpy(p, s);
					char commentOpen[tokenLen+10] = "1";
					p = commentOpen+1;
					strcpy(p, s);
					
					if (isInList(keywords5, s, doPrefix4G1, isCaseIgnored))
						sc.ChangeState(SCE_USER_WORD1);
					else if (isInList(keywords6, s, doPrefix4G2, isCaseIgnored))
						sc.ChangeState(SCE_USER_WORD2);
					else if (isInList(keywords7, s, doPrefix4G3, isCaseIgnored))
						sc.ChangeState(SCE_USER_WORD3);
					else if (isInList(keywords8, s, doPrefix4G4, isCaseIgnored)) 
						sc.ChangeState(SCE_USER_WORD4);

					//else if (blockOpenWords.InList(s)) 
					else if (isInList(blockOpenWords, s, false, isCaseIgnored)) 
						sc.ChangeState(SCE_USER_BLOCK_OPERATOR_OPEN);
					//else if (blockCloseWords.InList(s)) 
					else if (isInList(blockCloseWords, s, false, isCaseIgnored))
						sc.ChangeState(SCE_USER_BLOCK_OPERATOR_CLOSE);
					else if (isInList(comments,commentLineStr, isCommentLineSymbol, isCaseIgnored))
					{
						sc.ChangeState(SCE_USER_COMMENTLINE);
						doDefault = false;
					}
					else if (isInList(comments, commentOpen, isCommentSymbol, isCaseIgnored)) 
					{
					      sc.ChangeState(SCE_USER_COMMENT);
					      doDefault = false;
					}
					if (doDefault)
						sc.SetState(SCE_USER_DEFAULT);
				}
				break;
			}
			
			case SCE_USER_COMMENT :
			{
				char *pCommentClose = NULL;
				for (int i = 0 ; i < comments.len ; i++)
				{
					if (comments.words[i][0] == '2')
					{
						pCommentClose = comments.words[i] + 1;
						break;
					}
				}
				if (pCommentClose)
				{
					int len = strlen(pCommentClose);
					if (len == 1)
					{
						if (sc.Match(pCommentClose[0])) 
						{
							sc.Forward();
							sc.SetState(SCE_USER_DEFAULT);
						}
					}
					else 
					{
						if (sc.Match(pCommentClose)) 
						{
							for (int i = 0 ; i < len ; i++)
								sc.Forward();
							sc.SetState(SCE_USER_DEFAULT);
						}
					}
				}
				break;
			} 
			
			case SCE_USER_COMMENTLINE :
			{
				if (sc.atLineEnd) 
				{
					sc.SetState(SCE_USER_DEFAULT);
					visibleChars = 0;
				}
				break;
			} 
			
			case SCE_USER_OPERATOR :
			{
				sc.SetState(SCE_USER_DEFAULT);
				break;
			} 
			
			default :
				break;
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_USER_DEFAULT) 
		{
			//char aSymbol[2] = {sc.ch, '\0'};

			if (IsADigit(sc.ch))
				sc.SetState(SCE_USER_NUMBER);
			//else if (symbols.InList(aSymbol))
			else if (delimOpen[0] && (sc.ch == delimOpen[0]))
            {
                escaped = false;
				sc.SetState(SCE_USER_DELIMITER1);
            }
			else if (delimOpen[0] && (sc.ch == delimOpen[1]))
            {
                escaped = false;
				sc.SetState(SCE_USER_DELIMITER2);
            }
			else if (delimOpen[0] && (sc.ch == delimOpen[2]))
            {
                escaped = false;
				sc.SetState(SCE_USER_DELIMITER3);
            }
			else if (isInOpList(symbols, sc.ch))
				sc.SetState(SCE_USER_OPERATOR);
			else if (sc.ch == '-' && IsADigit(sc.chNext))
				sc.SetState(SCE_USER_NUMBER);
			else if (sc.ch == '.' && IsADigit(sc.chNext))
			{
				hasDot = true;
                sc.SetState(SCE_USER_NUMBER);
			}
			else if (isAWordStart(sc.ch)) 
				sc.SetState(SCE_USER_IDENTIFIER);
		}

		if (sc.atLineEnd) 
		{
			// Reset states to begining of colourise so no surprises
			// if different sets of lines lexed.
		   if (sc.state == SCE_USER_COMMENTLINE)
				sc.SetState(SCE_USER_DEFAULT);
			
			chPrevNonWhite = ' ';
			visibleChars = 0;
		}
		if (!isspacechar(sc.ch)) {
			chPrevNonWhite = sc.ch;
			visibleChars++;
		}
		
	}
	sc.Complete();

}


static void FoldUserDoc(unsigned int startPos, int length, int initStyle, WordList *[],  Accessor &styler) 
{
	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	
	for (unsigned int i = startPos; i < endPos; i++) 
	{
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (stylePrev != SCE_USER_BLOCK_OPERATOR_OPEN && style == SCE_USER_BLOCK_OPERATOR_OPEN)
		{
			levelCurrent++;
		} 

		if (stylePrev != SCE_USER_BLOCK_OPERATOR_CLOSE && style == SCE_USER_BLOCK_OPERATOR_CLOSE)
		{
			levelCurrent--;
		}

		if (atEOL) 
		{
			int lev = levelPrev;

			if ((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
			{
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}



static const char * const userDefineWordLists[] = {
            "Primary keywords and identifiers",
            "Secondary keywords and identifiers",
            "Documentation comment keywords",
            "Fold header keywords",
            0,
        };



LexerModule lmUserDefine(SCLEX_USER, ColouriseUserDoc, "user", FoldUserDoc, userDefineWordLists);
