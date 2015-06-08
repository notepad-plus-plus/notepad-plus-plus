// Scintilla source code edit control
/** @file LexDMIS.cxx
 ** Lexer for DMIS.
  **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// Copyright 2013-2014 by Andreas Tscharner <andy@vis.ethz.ch>
// The License.txt file describes the conditions under which this software may be distributed.


#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif


static const char *const DMISWordListDesc[] = {
	"DMIS Major Words",
	"DMIS Minor Words",
	"Unsupported DMIS Major Words",
	"Unsupported DMIS Minor Words",
	"Keywords for code folding start",
	"Corresponding keywords for code folding end",
	0
};


class LexerDMIS : public ILexer
{
	private:
		char *m_wordListSets;
		WordList m_majorWords;
		WordList m_minorWords;
		WordList m_unsupportedMajor;
		WordList m_unsupportedMinor;
		WordList m_codeFoldingStart;
		WordList m_codeFoldingEnd;

		char * SCI_METHOD UpperCase(char *item);
		void SCI_METHOD InitWordListSets(void);

	public:
		LexerDMIS(void);
		virtual ~LexerDMIS(void);

		int SCI_METHOD Version() const {
			return lvOriginal;
		}

		void SCI_METHOD Release() {
			delete this;
		}

		const char * SCI_METHOD PropertyNames() {
			return NULL;
		}

		int SCI_METHOD PropertyType(const char *) {
			return -1;
		}

		const char * SCI_METHOD DescribeProperty(const char *) {
			return NULL;
		}

		int SCI_METHOD PropertySet(const char *, const char *) {
			return -1;
		}

		int SCI_METHOD WordListSet(int n, const char *wl);

		void * SCI_METHOD PrivateCall(int, void *) {
			return NULL;
		}

		static ILexer *LexerFactoryDMIS() {
			return new LexerDMIS;
		}

		const char * SCI_METHOD DescribeWordListSets();
		void SCI_METHOD Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
		void SCI_METHOD Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
};


char * SCI_METHOD LexerDMIS::UpperCase(char *item)
{
	char *itemStart;


	itemStart = item;
	while (item && *item) {
		*item = toupper(*item);
		item++;
	};
	return itemStart;
}

void SCI_METHOD LexerDMIS::InitWordListSets(void)
{
	size_t totalLen = 0;


	for (int i=0; DMISWordListDesc[i]; i++) {
		totalLen += strlen(DMISWordListDesc[i]);
		totalLen++;
	};

	totalLen++;
	this->m_wordListSets = new char[totalLen];
	memset(this->m_wordListSets, 0, totalLen);

	for (int i=0; DMISWordListDesc[i]; i++) {
		strcat(this->m_wordListSets, DMISWordListDesc[i]);
		strcat(this->m_wordListSets, "\n");
	};
}


LexerDMIS::LexerDMIS(void) {
	this->InitWordListSets();

	this->m_majorWords.Clear();
	this->m_minorWords.Clear();
	this->m_unsupportedMajor.Clear();
	this->m_unsupportedMinor.Clear();
	this->m_codeFoldingStart.Clear();
	this->m_codeFoldingEnd.Clear();
}

LexerDMIS::~LexerDMIS(void) {
	delete[] this->m_wordListSets;
}

int SCI_METHOD LexerDMIS::WordListSet(int n, const char *wl)
{
	switch (n) {
		case 0:
			this->m_majorWords.Clear();
			this->m_majorWords.Set(wl);
			break;
		case 1:
			this->m_minorWords.Clear();
			this->m_minorWords.Set(wl);
			break;
		case 2:
			this->m_unsupportedMajor.Clear();
			this->m_unsupportedMajor.Set(wl);
			break;
		case 3:
			this->m_unsupportedMinor.Clear();
			this->m_unsupportedMinor.Set(wl);
			break;
		case 4:
			this->m_codeFoldingStart.Clear();
			this->m_codeFoldingStart.Set(wl);
			break;
		case 5:
			this->m_codeFoldingEnd.Clear();
			this->m_codeFoldingEnd.Set(wl);
			break;
		default:
			return -1;
			break;
	}

	return 0;
}

const char * SCI_METHOD LexerDMIS::DescribeWordListSets()
{
	return this->m_wordListSets;
}

void SCI_METHOD LexerDMIS::Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess)
{
	const unsigned int MAX_STR_LEN = 100;

	LexAccessor styler(pAccess);
	StyleContext scCTX(startPos, lengthDoc, initStyle, styler);
	CharacterSet setDMISNumber(CharacterSet::setDigits, ".-+eE");
	CharacterSet setDMISWordStart(CharacterSet::setAlpha, "-234", 0x80, true);
	CharacterSet setDMISWord(CharacterSet::setAlpha);


	bool isIFLine = false;

	for (; scCTX.More(); scCTX.Forward()) {
		if (scCTX.atLineEnd) {
			isIFLine = false;
		};

		switch (scCTX.state) {
			case SCE_DMIS_DEFAULT:
				if (scCTX.Match('$', '$')) {
					scCTX.SetState(SCE_DMIS_COMMENT);
					scCTX.Forward();
				};
				if (scCTX.Match('\'')) {
					scCTX.SetState(SCE_DMIS_STRING);
				};
				if (IsADigit(scCTX.ch) || ((scCTX.Match('-') || scCTX.Match('+')) && IsADigit(scCTX.chNext))) {
					scCTX.SetState(SCE_DMIS_NUMBER);
					break;
				};
				if (setDMISWordStart.Contains(scCTX.ch)) {
					scCTX.SetState(SCE_DMIS_KEYWORD);
				};
				if (scCTX.Match('(') && (!isIFLine)) {
					scCTX.SetState(SCE_DMIS_LABEL);
				};
				break;

			case SCE_DMIS_COMMENT:
				if (scCTX.atLineEnd) {
					scCTX.SetState(SCE_DMIS_DEFAULT);
				};
				break;

			case SCE_DMIS_STRING:
				if (scCTX.Match('\'')) {
					scCTX.SetState(SCE_DMIS_DEFAULT);
				};
				break;

			case SCE_DMIS_NUMBER:
				if (!setDMISNumber.Contains(scCTX.ch)) {
					scCTX.SetState(SCE_DMIS_DEFAULT);
				};
				break;

			case SCE_DMIS_KEYWORD:
				if (!setDMISWord.Contains(scCTX.ch)) {
					char tmpStr[MAX_STR_LEN];
					memset(tmpStr, 0, MAX_STR_LEN*sizeof(char));
					scCTX.GetCurrent(tmpStr, (MAX_STR_LEN-1));
					strncpy(tmpStr, this->UpperCase(tmpStr), (MAX_STR_LEN-1));

					if (this->m_minorWords.InList(tmpStr)) {
						scCTX.ChangeState(SCE_DMIS_MINORWORD);
					};
					if (this->m_majorWords.InList(tmpStr)) {
						isIFLine = (strcmp(tmpStr, "IF") == 0);
						scCTX.ChangeState(SCE_DMIS_MAJORWORD);
					};
					if (this->m_unsupportedMajor.InList(tmpStr)) {
						scCTX.ChangeState(SCE_DMIS_UNSUPPORTED_MAJOR);
					};
					if (this->m_unsupportedMinor.InList(tmpStr)) {
						scCTX.ChangeState(SCE_DMIS_UNSUPPORTED_MINOR);
					};

					if (scCTX.Match('(') && (!isIFLine)) {
						scCTX.SetState(SCE_DMIS_LABEL);
					} else {
						scCTX.SetState(SCE_DMIS_DEFAULT);
					};
				};
				break;

			case SCE_DMIS_LABEL:
				if (scCTX.Match(')')) {
					scCTX.SetState(SCE_DMIS_DEFAULT);
				};
				break;
		};
	};
	scCTX.Complete();
}

void SCI_METHOD LexerDMIS::Fold(unsigned int startPos, int lengthDoc, int, IDocument *pAccess)
{
	const int MAX_STR_LEN = 100;

	LexAccessor styler(pAccess);
	unsigned int endPos = startPos + lengthDoc;
	char chNext = styler[startPos];
	int lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	int strPos = 0;
	bool foldWordPossible = false;
	CharacterSet setDMISFoldWord(CharacterSet::setAlpha);
	char *tmpStr;


	tmpStr = new char[MAX_STR_LEN];
	memset(tmpStr, 0, MAX_STR_LEN*sizeof(char));

	for (unsigned int i=startPos; i<endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i+1);

		bool atEOL = ((ch == '\r' && chNext != '\n') || (ch == '\n'));

		if (strPos >= (MAX_STR_LEN-1)) {
			strPos = MAX_STR_LEN-1;
		};

		int style = styler.StyleAt(i);
		bool noFoldPos = ((style == SCE_DMIS_COMMENT) || (style == SCE_DMIS_STRING));

		if (foldWordPossible) {
			if (setDMISFoldWord.Contains(ch)) {
				tmpStr[strPos++] = ch;
			} else {
				tmpStr = this->UpperCase(tmpStr);
				if (this->m_codeFoldingStart.InList(tmpStr) && (!noFoldPos)) {
					levelCurrent++;
				};
				if (this->m_codeFoldingEnd.InList(tmpStr) && (!noFoldPos)) {
					levelCurrent--;
				};
				memset(tmpStr, 0, MAX_STR_LEN*sizeof(char));
				strPos = 0;
				foldWordPossible = false;
			};
		} else {
			if (setDMISFoldWord.Contains(ch)) {
				tmpStr[strPos++] = ch;
				foldWordPossible = true;
			};
		};

		if (atEOL || (i == (endPos-1))) {
			int lev = levelPrev;

			if (levelCurrent > levelPrev) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			};
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			};
			lineCurrent++;
			levelPrev = levelCurrent;
		};
	};
	delete[] tmpStr;
}


LexerModule lmDMIS(SCLEX_DMIS, LexerDMIS::LexerFactoryDMIS, "DMIS", DMISWordListDesc);
