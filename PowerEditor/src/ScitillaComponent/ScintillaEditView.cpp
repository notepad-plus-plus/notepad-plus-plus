//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <windows.h>
#include <ShellAPI.h>
#include "ScintillaEditView.h"
#include "Parameters.h"


// initialize the static variable
HINSTANCE ScintillaEditView::_hLib = ::LoadLibrary("SciLexer.DLL");
int ScintillaEditView::_refCount = 0;
UserDefineDialog ScintillaEditView::_userDefineDlg;

const int ScintillaEditView::_SC_MARGE_LINENUMBER = 0;
const int ScintillaEditView::_SC_MARGE_SYBOLE = 1;
const int ScintillaEditView::_SC_MARGE_FOLDER = 2;

const int ScintillaEditView::_MARGE_LINENUMBER_NB_CHIFFRE = 5;

WNDPROC ScintillaEditView::_scintillaDefaultProc = NULL;
/*
SC_MARKNUM_*     | Arrow               Plus/minus           Circle tree                 Box tree 
-------------------------------------------------------------------------------------------------------------
FOLDEROPEN       | SC_MARK_ARROWDOWN   SC_MARK_MINUS     SC_MARK_CIRCLEMINUS            SC_MARK_BOXMINUS 
FOLDER           | SC_MARK_ARROW       SC_MARK_PLUS      SC_MARK_CIRCLEPLUS             SC_MARK_BOXPLUS 
FOLDERSUB        | SC_MARK_EMPTY       SC_MARK_EMPTY     SC_MARK_VLINE                  SC_MARK_VLINE 
FOLDERTAIL       | SC_MARK_EMPTY       SC_MARK_EMPTY     SC_MARK_LCORNERCURVE           SC_MARK_LCORNER 
FOLDEREND        | SC_MARK_EMPTY       SC_MARK_EMPTY     SC_MARK_CIRCLEPLUSCONNECTED    SC_MARK_BOXPLUSCONNECTED 
FOLDEROPENMID    | SC_MARK_EMPTY       SC_MARK_EMPTY     SC_MARK_CIRCLEMINUSCONNECTED   SC_MARK_BOXMINUSCONNECTED 
FOLDERMIDTAIL    | SC_MARK_EMPTY       SC_MARK_EMPTY     SC_MARK_TCORNERCURVE           SC_MARK_TCORNER 
*/

const int ScintillaEditView::_markersArray[][NB_FOLDER_STATE] = {
  {SC_MARKNUM_FOLDEROPEN, SC_MARKNUM_FOLDER, SC_MARKNUM_FOLDERSUB, SC_MARKNUM_FOLDERTAIL, SC_MARKNUM_FOLDEREND,        SC_MARKNUM_FOLDEROPENMID,     SC_MARKNUM_FOLDERMIDTAIL},
  {SC_MARK_MINUS,         SC_MARK_PLUS,      SC_MARK_EMPTY,        SC_MARK_EMPTY,         SC_MARK_EMPTY,               SC_MARK_EMPTY,                SC_MARK_EMPTY},
  {SC_MARK_ARROWDOWN,     SC_MARK_ARROW,     SC_MARK_EMPTY,        SC_MARK_EMPTY,         SC_MARK_EMPTY,               SC_MARK_EMPTY,                SC_MARK_EMPTY},
  {SC_MARK_CIRCLEMINUS,   SC_MARK_CIRCLEPLUS,SC_MARK_VLINE,        SC_MARK_LCORNERCURVE,  SC_MARK_CIRCLEPLUSCONNECTED, SC_MARK_CIRCLEMINUSCONNECTED, SC_MARK_TCORNERCURVE},
  {SC_MARK_BOXMINUS,      SC_MARK_BOXPLUS,   SC_MARK_VLINE,        SC_MARK_LCORNER,       SC_MARK_BOXPLUSCONNECTED,    SC_MARK_BOXMINUSCONNECTED,    SC_MARK_TCORNER}
};

//const int MASK_RED   = 0xFF0000;
//const int MASK_GREEN = 0x00FF00;
//const int MASK_BLUE  = 0x0000FF;

void ScintillaEditView::init(HINSTANCE hInst, HWND hPere)
{
	if (!_hLib)
	{
		MessageBox( NULL, "Can not load the dynamic library", "SCINTILLA ERROR : ", MB_OK | MB_ICONSTOP);
		throw int(106901);
	}

	Window::init(hInst, hPere);
   _hSelf = ::CreateWindowEx(
					WS_EX_CLIENTEDGE,\
					"Scintilla",\
					"Notepad++",\
					WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_EX_RTLREADING,\
					0, 0, 100, 100,\
					_hParent,\
					NULL,\
					_hInst,\
					NULL);

	if (!_hSelf)
	{
		systemMessage("System Error");
		throw int(106901);
	}

	_pScintillaFunc = (SCINTILLA_FUNC)::SendMessage(_hSelf, SCI_GETDIRECTFUNCTION, 0, 0);
	_pScintillaPtr = (SCINTILLA_PTR)::SendMessage(_hSelf, SCI_GETDIRECTPOINTER, 0, 0);

    _userDefineDlg.init(_hInst, _hParent, this);

	if (!_pScintillaFunc || !_pScintillaPtr)
	{
		systemMessage("System Err");
		throw int(106901);
	}

    execute(SCI_SETMARGINMASKN, _SC_MARGE_FOLDER, SC_MASK_FOLDERS);
    showMargin(_SC_MARGE_FOLDER, true);

    execute(SCI_SETMARGINSENSITIVEN, _SC_MARGE_FOLDER, true);
    execute(SCI_SETMARGINSENSITIVEN, _SC_MARGE_SYBOLE, true);

    execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
    execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.html"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
    execute(SCI_SETFOLDFLAGS, 16, 0);

	_pParameter = NppParameters::getInstance();
	
	_codepage = ::GetACP();
	_oemCodepage = ::GetOEMCP();

	::SetWindowLong(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_scintillaDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(scintillaStatic_Proc)));
}

LRESULT ScintillaEditView::scintillaNew_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
{
	switch (Message)
	{
		case WM_CHAR :
		{
			//bool isAltHeld = lParam & 0x8000;
			if (execute(SCI_SELECTIONISRECTANGLE) && !(::GetKeyState(VK_LCONTROL) & 0x80000000))
			{
				execute(SCI_BEGINUNDOACTION);

				ColumnModeInfo colInfos = getColumnModeSelectInfo();
				columnReplace(colInfos, (char)wParam);

				execute(SCI_ENDUNDOACTION);
				execute(SCI_SETCURRENTPOS,colInfos[colInfos.size()-1].second);
				//execute(SCI_SETSEL, colInfos[0].first, colInfos[colInfos.size()-1].second);
				//execute(SCI_SETSELECTIONMODE, 1);
				return TRUE;
			}
			break;
		}

		case WM_MOUSEWHEEL :
		//case WM_RBUTTONDOWN :
		{
			if (LOWORD(wParam) & MK_RBUTTON)
			{
				::SendMessage(_hParent, Message, wParam, lParam);
				return TRUE;
			}

			//Have to perform the scroll first, because the first/last line do not get updated untill after the scroll has been parsed
			LRESULT scrollResult = ::CallWindowProc(_scintillaDefaultProc, hwnd, Message, wParam, lParam);
			recalcHorizontalScrollbar();
			return scrollResult;
			break;
		}
		case WM_VSCROLL :
		{
			if (LOWORD(wParam) == SB_ENDSCROLL)
				recalcHorizontalScrollbar();
			break;
		}
	}
	return ::CallWindowProc(_scintillaDefaultProc, hwnd, Message, wParam, lParam);
}
void ScintillaEditView::setSpecialStyle(int styleID, COLORREF fgColour, COLORREF bgColour, const char *fontName, int fontStyle, int fontSize)
{
    if (!((fgColour >> 24) & 0xFF))
	    execute(SCI_STYLESETFORE, styleID, fgColour);

    if (!((bgColour >> 24) & 0xFF))
	    execute(SCI_STYLESETBACK, styleID, bgColour);
    
    if ((!fontName)||(strcmp(fontName, "")))
		execute(SCI_STYLESETFONT, (WPARAM)styleID, (LPARAM)fontName);

    if (fontStyle != -1)
    {
        execute(SCI_STYLESETBOLD, (WPARAM)styleID, fontStyle & FONTSTYLE_BOLD);
        execute(SCI_STYLESETITALIC, (WPARAM)styleID, fontStyle & FONTSTYLE_ITALIC);
        execute(SCI_STYLESETUNDERLINE, (WPARAM)styleID, fontStyle & FONTSTYLE_UNDERLINE);
    }

	if (fontSize > 0)
		execute(SCI_STYLESETSIZE, styleID, fontSize);
}

void ScintillaEditView::setStyle(int styleID, COLORREF fgColour, COLORREF bgColour, const char *fontName, int fontStyle, int fontSize)
{
	GlobalOverride & go = _pParameter->getGlobalOverrideStyle();
	//go.enableBg = true;

	const char *localFn = fontName;

	if (go.isEnable())
	{
		StyleArray & stylers = _pParameter->getMiscStylerArray();
		int i = stylers.getStylerIndexByName("Global override");
		if (i != -1)
		{
			Style & style = stylers.getStyler(i);

			if (go.enableFg)
				fgColour = style._fgColor;
			if (go.enableBg)
				bgColour = style._bgColor;
			if (go.enableFont && style._fontName && style._fontName[0])
				localFn = style._fontName;
			if (go.enableFontSize && (style._fontSize > 0))
				fontSize = style._fontSize;

			if (style._fontStyle != -1)
			{	
				if (go.enableBold)
				{
					if (style._fontStyle & FONTSTYLE_BOLD)
						fontStyle |= FONTSTYLE_BOLD;
					else
						fontStyle &= ~FONTSTYLE_BOLD;
				}
				if (go.enableItalic)
				{
					if (style._fontStyle & FONTSTYLE_ITALIC)
						fontStyle |= FONTSTYLE_ITALIC;
					else 
						fontStyle &= ~FONTSTYLE_ITALIC;
				}
				if (go.enableUnderLine)
				{
					if (style._fontStyle & FONTSTYLE_UNDERLINE)
						fontStyle |= FONTSTYLE_UNDERLINE;
					else
						fontStyle &= ~FONTSTYLE_UNDERLINE;
				}
			}
		}
	}
	setSpecialStyle(styleID, fgColour, bgColour, localFn, fontStyle, fontSize);
}


void ScintillaEditView::setXmlLexer(LangType type)
{

    execute(SCI_SETSTYLEBITS, 7, 0);

	if (type == L_XML)
	{
        execute(SCI_SETLEXER, SCLEX_HTML);
		for (int i = 0 ; i < 4 ; i++)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(""));

        makeStyle("xml");
	}
	else if ((type == L_HTML) || (type == L_PHP) || (type == L_ASP))
	{

        execute(SCI_SETLEXER, SCLEX_XML);

        const char *htmlKeyWords =_pParameter->getWordList(L_HTML, LANG_INDEX_INSTR);
        execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(htmlKeyWords?htmlKeyWords:""));

		makeStyle("html");
		
        setEmbeddedJSLexer();
        setPhpEmbeddedLexer();
		setEmbeddedAspLexer();
	}
}

void ScintillaEditView::setEmbeddedJSLexer()
{
	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle("javascript", pKwArray);

	std::string keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
		keywordList = pKwArray[LANG_INDEX_INSTR];

	execute(SCI_SETKEYWORDS, 1, (LPARAM)getCompleteKeywordList(keywordList, L_JS, LANG_INDEX_INSTR));

	execute(SCI_STYLESETEOLFILLED, SCE_HJ_DEFAULT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENTDOC, true);
}

void ScintillaEditView::setPhpEmbeddedLexer()
{
	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle("php", pKwArray);

	std::string keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
		keywordList = pKwArray[LANG_INDEX_INSTR];

	execute(SCI_SETKEYWORDS, 4, (LPARAM)getCompleteKeywordList(keywordList, L_PHP, LANG_INDEX_INSTR));

	execute(SCI_STYLESETEOLFILLED, SCE_HPHP_DEFAULT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HPHP_COMMENT, true);
}

void ScintillaEditView::setEmbeddedAspLexer()
{
	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle("asp", pKwArray);

	std::string keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
		keywordList = pKwArray[LANG_INDEX_INSTR];

	execute(SCI_SETKEYWORDS, 2, (LPARAM)getCompleteKeywordList(keywordList, L_VB, LANG_INDEX_INSTR));

    execute(SCI_STYLESETEOLFILLED, SCE_HBA_DEFAULT, true);
}

void ScintillaEditView::setUserLexer()
{
    execute(SCI_SETLEXER, SCLEX_USER);
	UserLangContainer & userLangContainer = *(_userDefineDlg._pCurrentUserLang);
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.ignoreCase", (LPARAM)(userLangContainer._isCaseIgnored?"1":"0"));
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.commentLineSymbol", (LPARAM)(userLangContainer._isCommentLineSymbol?"1":"0"));
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.commentSymbol", (LPARAM)(userLangContainer._isCommentSymbol?"1":"0"));

	const char strArray[4][20] = {"userDefine.g1Prefix", "userDefine.g2Prefix", "userDefine.g3Prefix", "userDefine.g4Prefix"};
	for (int i = 0 ; i < 4 ; i++)
		execute(SCI_SETPROPERTY, (WPARAM)strArray[i], (LPARAM)(userLangContainer._isPrefix[i]?"1":"0"));
	
	for (int i = 0 ; i < userLangContainer.getNbKeywordList() ; i++)
	{
		execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(userLangContainer._keywordLists[i]));
	}

	for (int i = 0 ; i < userLangContainer._styleArray.getNbStyler() ; i++)
	{
		Style & style = userLangContainer._styleArray.getStyler(i);
		setStyle(style._styleID, style._fgColor, style._bgColor, style._fontName, style._fontStyle, style._fontSize);
	}
}

void ScintillaEditView::setUserLexer(const char *userLangName)
{
	
    execute(SCI_SETLEXER, SCLEX_USER);

	UserLangContainer & userLangContainer = NppParameters::getInstance()->getULCFromName(userLangName);
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.ignoreCase", (LPARAM)(userLangContainer._isCaseIgnored?"1":"0"));
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.commentLineSymbol", (LPARAM)(userLangContainer._isCommentLineSymbol?"1":"0"));
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.commentSymbol", (LPARAM)(userLangContainer._isCommentSymbol?"1":"0"));

	const char strArray[4][20] = {"userDefine.g1Prefix", "userDefine.g2Prefix", "userDefine.g3Prefix", "userDefine.g4Prefix"};
	for (int i = 0 ; i < 4 ; i++)
		execute(SCI_SETPROPERTY, (WPARAM)strArray[i], (LPARAM)(userLangContainer._isPrefix[i]?"1":"0"));

	for (int i = 0 ; i < userLangContainer.getNbKeywordList() ; i++)
	{
		execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(userLangContainer._keywordLists[i]));
	}

	for (int i = 0 ; i < userLangContainer._styleArray.getNbStyler() ; i++)
	{
		Style & style = userLangContainer._styleArray.getStyler(i);
		setStyle(style._styleID, style._fgColor, style._bgColor, style._fontName, style._fontStyle, style._fontSize);
	}
}

void ScintillaEditView::setExternalLexer(LangType typeDoc)
{
	int id = typeDoc - L_EXTERNAL;
	char * name = NppParameters::getInstance()->getELCFromIndex(id)._name;
	execute(SCI_SETLEXERLANGUAGE, 0, (LPARAM)name);

	LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName(name);	
	if (pStyler)
	{
		for (int i = 0 ; i < pStyler->getNbStyler() ; i++)
		{
			Style & style = pStyler->getStyler(i);

			setStyle(style._styleID, style._fgColor, style._bgColor, style._fontName, style._fontStyle, style._fontSize);

			if (style._keywordClass >= 0 && style._keywordClass <= KEYWORDSET_MAX)
			{
				string kwl("");
				if (style._keywords)
					kwl = *(style._keywords);

				execute(SCI_SETKEYWORDS, style._keywordClass, (LPARAM)getCompleteKeywordList(kwl, typeDoc, style._keywordClass));
			}
		}
	}
}

void ScintillaEditView::setCppLexer(LangType langType)
{
    const char *cppInstrs;
    const char *cppTypes;
    const char *doxygenKeyWords  = _pParameter->getWordList(L_CPP, LANG_INDEX_TYPE2);

	char *lexerName;
	switch (langType)
	{
		case L_C:
			lexerName = "c"; break;

		case L_CPP:
			lexerName = "cpp"; break;

		case L_JAVA:
			lexerName = "java"; break;

		case L_JS:
			lexerName = "javascript"; break;

		case L_RC:
			lexerName = "rc"; break;

		case L_CS:
			lexerName = "cs"; break;

		case L_TCL:
			lexerName = "tcl"; break;

		default:
			return;
	}

    execute(SCI_SETLEXER, SCLEX_CPP); 
	if (isCJK())
	{
		int charSet = codepage2CharSet();
		if (charSet)
			execute(SCI_STYLESETCHARACTERSET, SCE_C_STRING, charSet);
	}

	if ((langType != L_RC) && (langType != L_JS))
    {
        if (doxygenKeyWords)
            execute(SCI_SETKEYWORDS, 2, (LPARAM)doxygenKeyWords);
    }

	if (langType == L_JS)
	{
		LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName("javascript");	
		if (pStyler)
		{
			for (int i = 0 ; i < pStyler->getNbStyler() ; i++)
			{
				Style & style = pStyler->getStyler(i);
				int cppID = style._styleID; 
				switch (style._styleID)
				{
					case SCE_HJ_DEFAULT : cppID = SCE_C_DEFAULT; break;
					case SCE_HJ_WORD : cppID = SCE_C_IDENTIFIER; break;
					case SCE_HJ_SYMBOLS : cppID = SCE_C_OPERATOR; break;
					case SCE_HJ_COMMENT : cppID = SCE_C_COMMENT; break;
					case SCE_HJ_COMMENTLINE : cppID = SCE_C_COMMENTLINE; break;
					case SCE_HJ_COMMENTDOC : cppID = SCE_C_COMMENTDOC; break;
					case SCE_HJ_NUMBER : cppID = SCE_C_NUMBER; break;
					case SCE_HJ_KEYWORD : cppID = SCE_C_WORD; break;
					case SCE_HJ_DOUBLESTRING : cppID = SCE_C_STRING; break;
					case SCE_HJ_SINGLESTRING : cppID = SCE_C_CHARACTER; break;
					case SCE_HJ_REGEX : cppID = SCE_C_REGEX; break;
				}
				setStyle(cppID, style._fgColor, style._bgColor, style._fontName, style._fontStyle, style._fontSize);
			}
		}
		execute(SCI_STYLESETEOLFILLED, SCE_C_DEFAULT, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENTLINE, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENT, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENTDOC, true);
	}

	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle(lexerName, pKwArray);

	std::string instr1("");
	if (pKwArray[LANG_INDEX_INSTR])
		instr1 = pKwArray[LANG_INDEX_INSTR];
	cppInstrs = getCompleteKeywordList(instr1, langType, LANG_INDEX_INSTR);

	std::string type1("");
	if (pKwArray[LANG_INDEX_TYPE])
		type1 = pKwArray[LANG_INDEX_TYPE];
	cppTypes = getCompleteKeywordList(type1, langType, LANG_INDEX_TYPE);

	execute(SCI_SETKEYWORDS, 0, (LPARAM)cppInstrs);
	execute(SCI_SETKEYWORDS, 1, (LPARAM)cppTypes);

}

//used by Objective-C and Actionscript
void ScintillaEditView::setObjCLexer(LangType langType) 
{
    execute(SCI_SETLEXER, SCLEX_OBJC);
	const char *doxygenKeyWords = _pParameter->getWordList(L_CPP, LANG_INDEX_TYPE2);

	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

	const char *lexerName = "objc";
	if (langType == L_FLASH)
		lexerName = "actionscript";

	makeStyle(lexerName, pKwArray);
	
	std::string objcInstr1Kwl("");
	if (pKwArray[LANG_INDEX_INSTR])
		objcInstr1Kwl = pKwArray[LANG_INDEX_INSTR];
	const char *objcInstrs = getCompleteKeywordList(objcInstr1Kwl, langType, LANG_INDEX_INSTR);
	
	std::string objcInstr2Kwl("");
	if (pKwArray[LANG_INDEX_INSTR2])
		objcInstr2Kwl = pKwArray[LANG_INDEX_INSTR2];
	const char *objCDirective = getCompleteKeywordList(objcInstr2Kwl, langType, LANG_INDEX_INSTR2);

	std::string objcTypeKwl("");
	if (pKwArray[LANG_INDEX_TYPE])
		objcTypeKwl = pKwArray[LANG_INDEX_TYPE];
	const char *objcTypes = getCompleteKeywordList(objcTypeKwl, langType, LANG_INDEX_TYPE);
	
	std::string objcType2Kwl("");
	if (pKwArray[LANG_INDEX_TYPE2])
		objcType2Kwl = pKwArray[LANG_INDEX_TYPE2];
	const char *objCQualifier = getCompleteKeywordList(objcType2Kwl, langType, LANG_INDEX_TYPE2);
	
	execute(SCI_SETKEYWORDS, 0, (LPARAM)objcInstrs);
    execute(SCI_SETKEYWORDS, 1, (LPARAM)objcTypes);
	execute(SCI_SETKEYWORDS, 2, (LPARAM)(doxygenKeyWords?doxygenKeyWords:""));
	execute(SCI_SETKEYWORDS, 3, (LPARAM)objCDirective);
	execute(SCI_SETKEYWORDS, 4, (LPARAM)objCQualifier);
}

void ScintillaEditView::setKeywords(LangType langType, const char *keywords, int index)
{
	std::string wordList;
	wordList = (keywords)?keywords:"";
	execute(SCI_SETKEYWORDS, index, (LPARAM)getCompleteKeywordList(wordList, langType, index));
}

void ScintillaEditView::setLexer(int lexerID, LangType langType, const char *lexerName, int whichList)
{
	execute(SCI_SETLEXER, lexerID);

	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	
	makeStyle(lexerName, pKwArray);

	if (whichList & LIST_0)
	{
		setKeywords(langType, pKwArray[LANG_INDEX_INSTR], LANG_INDEX_INSTR);
	}	

	if (whichList & LIST_1)
	{
		setKeywords(langType, pKwArray[LANG_INDEX_INSTR2], LANG_INDEX_INSTR2);
	}

	if (whichList & LIST_2)
	{
		setKeywords(langType, pKwArray[LANG_INDEX_TYPE], LANG_INDEX_TYPE);
	}

	if (whichList & LIST_3)
	{
		setKeywords(langType, pKwArray[LANG_INDEX_TYPE2], LANG_INDEX_TYPE2);
	}

	if (whichList & LIST_4)
	{
		setKeywords(langType, pKwArray[LANG_INDEX_TYPE3], LANG_INDEX_TYPE3);
	}

	if (whichList & LIST_5)
	{
		setKeywords(langType, pKwArray[LANG_INDEX_TYPE4], LANG_INDEX_TYPE4);
	}

	if (whichList & LIST_6)
	{
		setKeywords(langType, pKwArray[LANG_INDEX_TYPE5], LANG_INDEX_TYPE5);
	}
}

void ScintillaEditView::defineDocType(LangType typeDoc)
{
	//setStyle(STYLE_DEFAULT, black, white, "Verdana", 0, 9);
    
    StyleArray & stylers = _pParameter->getMiscStylerArray();
    int iStyleDefault = stylers.getStylerIndexByID(STYLE_DEFAULT);
    if (iStyleDefault != -1)
    {
        Style & styleDefault = stylers.getStyler(iStyleDefault);
	    setStyle(styleDefault._styleID, styleDefault._fgColor, styleDefault._bgColor, styleDefault._fontName, styleDefault._fontStyle, styleDefault._fontSize);
    }

    execute(SCI_STYLECLEARALL);

    int iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE);
    if (iFind != -1)
    {
        Style & styleFind = stylers.getStyler(iFind);
	    setSpecialStyle(styleFind._styleID, styleFind._fgColor, styleFind._bgColor, styleFind._fontName, styleFind._fontStyle, styleFind._fontSize);
    }

	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_SELECT_STYLE);
    if (iFind != -1)
    {
        Style & styleFind = stylers.getStyler(iFind);
	    setSpecialStyle(styleFind._styleID, styleFind._fgColor, styleFind._bgColor, styleFind._fontName, styleFind._fontStyle, styleFind._fontSize);
    }
    
    int caretWidth = 1;
    
	
    // Il faut surtout faire un test ici avant d'exécuter SCI_SETCODEPAGE
    // Sinon y'aura un soucis de performance!
	if (isCJK())
	{
		if (getCurrentBuffer()._unicodeMode == uni8Bit)
			execute(SCI_SETCODEPAGE, _codepage);

	}

	execute(SCI_SETSTYLEBITS, 5);

	showMargin(_SC_MARGE_FOLDER, isNeededFolderMarge(typeDoc));
	switch (typeDoc)
	{
		case L_C :
		case L_CPP :
		case L_JS:
		case L_JAVA :
		case L_RC :
		case L_CS :
		case L_TCL :
            setCppLexer(typeDoc); break;

		case L_FLASH :
        case L_OBJC :
            setObjCLexer(typeDoc); break;
		
	    case L_PHP :
		case L_ASP :
		case L_HTML :
		case L_XML :
			setXmlLexer(typeDoc); break;

		case L_CSS :
			setCssLexer(); break;

		case L_LUA :
			setLuaLexer(); break;

		case L_MAKEFILE :
			setMakefileLexer(); break;

		case L_INI :
			setIniLexer(); break;
			
        case L_USER :
			if (_buffers[_currentIndex]._userLangExt[0])
				setUserLexer(_buffers[_currentIndex]._userLangExt); 
			else
				setUserLexer();
			break;

        case L_NFO :
		{
			LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName("nfo");
			COLORREF bg = black;
			COLORREF fg = liteGrey;
			if (pStyler)
			{
				int i = pStyler->getStylerIndexByName("DEFAULT");
				if (i != -1)
				{
					Style & style = pStyler->getStyler(i);
					bg = style._bgColor;
					fg = style._fgColor;
				}
			}

			setStyle(STYLE_DEFAULT, fg, bg, "MS LineDraw");
			execute(SCI_STYLECLEARALL);
		}
		break;

		case L_SQL :
			setSqlLexer(); break;

		case L_VB :
			setVBLexer(); break;

		case L_PASCAL :
			setPascalLexer(); break;

		case L_PERL :
			setPerlLexer(); break;

		case L_PYTHON :
			setPythonLexer(); break;

		case L_BATCH :
			setBatchLexer(); break;

		case L_TEX : 
			setTeXLexer(); break;

		case L_NSIS :
			setNsisLexer(); break;

		case L_BASH :
			setBashLexer(); break;

		case L_FORTRAN : 
			setFortranLexer(); break;

		case L_LISP :
            setLispLexer(); break;

		case L_SCHEME :
            setShemeLexer(); break;

		case L_ASM :
            setAsmLexer(); break;

		case L_DIFF :
            setDiffLexer(); break;

		case L_PROPS :
            setPropsLexer(); break;

		case L_PS :
            setPostscriptLexer(); break;

		case L_RUBY :
            setRubyLexer(); break;

		case L_SMALLTALK :
            setSmalltalkLexer(); break;

		case L_VHDL :
            setVhdlLexer(); break;

		case L_KIX :
            setKixLexer(); break;

		case L_CAML :
            setCamlLexer(); break;

		case L_ADA :
            setAdaLexer(); break;

		case L_VERILOG :
            setVerilogLexer(); break;

		case L_AU3 :
            setAutoItLexer(); break;

		case L_MATLAB :
            setMatlabLexer(); break;

		case L_HASKELL :
            setHaskellLexer(); break;

		case L_INNO :
			setInnoLexer(); break;

		case L_CMAKE :
			setCmakeLexer(); break;

		case L_YAML :
			setYamlLexer(); break;

		case L_TXT :
		default :
			if (typeDoc >= L_EXTERNAL && typeDoc < NppParameters::getInstance()->L_END)
				setExternalLexer(typeDoc);
			else
				execute(SCI_SETLEXER, (_codepage == CP_CHINESE_TRADITIONAL)?SCLEX_MAKEFILE:SCLEX_NULL);
			break;

	}

	//All the global styles should put here
	static int indexOfIndentGuide = stylers.getStylerIndexByID(STYLE_INDENTGUIDE);
	if (indexOfIndentGuide != -1)
    {
        static Style & styleIG = stylers.getStyler(indexOfIndentGuide);
	    setStyle(styleIG._styleID, styleIG._fgColor, styleIG._bgColor, styleIG._fontName, styleIG._fontStyle, styleIG._fontSize);
    }

	static int indexOfBraceLight = stylers.getStylerIndexByID(STYLE_BRACELIGHT);
	if (indexOfBraceLight != -1)
    {
        static Style & styleBL = stylers.getStyler(indexOfBraceLight);
	    setStyle(styleBL._styleID, styleBL._fgColor, styleBL._bgColor, styleBL._fontName, styleBL._fontStyle, styleBL._fontSize);
    }
	//setStyle(STYLE_CONTROLCHAR, liteGrey);

	static int indexBadBrace = stylers.getStylerIndexByID(STYLE_BRACEBAD);
	if (indexBadBrace != -1)
    {
        static Style & styleBB = stylers.getStyler(indexBadBrace);
	    setStyle(styleBB._styleID, styleBB._fgColor, styleBB._bgColor, styleBB._fontName, styleBB._fontStyle, styleBB._fontSize);
    }

	static int indexLineNumber = stylers.getStylerIndexByID(STYLE_LINENUMBER);
	if (indexLineNumber != -1)
    {
        static Style & styleLN = stylers.getStyler(indexLineNumber);
	    setSpecialStyle(styleLN._styleID, styleLN._fgColor, styleLN._bgColor, styleLN._fontName, styleLN._fontStyle, styleLN._fontSize);
    }

	execute(SCI_SETTABWIDTH, ((NppParameters::getInstance())->getNppGUI())._tabSize);
	execute(SCI_SETUSETABS, !((NppParameters::getInstance())->getNppGUI())._tabReplacedBySpace);

    execute(SCI_COLOURISE, 0, -1);
}

char * ScintillaEditView::attatchDefaultDoc(int nb)
{
	char title[10];
	char nb_str[4];
	strcat(strcpy(title, UNTITLED_STR), _itoa(nb, nb_str, 10));

	// get the doc pointer attached (by default) on the view Scintilla
	Document doc = execute(SCI_GETDOCPOINTER, 0, 0);

	// create the entry for our list
	_buffers.push_back(Buffer(doc, title));

	// set current index to 0
	_currentIndex = 0;

	return _buffers[_currentIndex]._fullPathName;
}


int ScintillaEditView::findDocIndexByName(const char *fn) const
{
	int index = -1;
	for (int i = 0 ; i < int(_buffers.size()) ; i++)
	{
		if (!stricmp(_buffers[i]._fullPathName, fn))
		{
			index = i;
			break;
		}
	}
	return index;
}

void ScintillaEditView::saveCurrentPos()
{
	int displayedLine = static_cast<int>(execute(SCI_GETFIRSTVISIBLELINE));
	
	int docLine = execute(SCI_DOCLINEFROMVISIBLE, displayedLine);

	int nbInvisibleLine = 0;
	
	//Calculate nb of invisible line
	for (int i = 0 ; i < docLine ; i++)
		if (execute(SCI_GETLINEVISIBLE, i) == FALSE)
			nbInvisibleLine++;
	
 	Buffer & buf = _buffers[_currentIndex];

	// the correct visible line number
	buf._pos._firstVisibleLine = docLine - nbInvisibleLine;
	buf._pos._startPos = static_cast<int>(execute(SCI_GETSELECTIONSTART));
	buf._pos._endPos = static_cast<int>(execute(SCI_GETSELECTIONEND));
	buf._pos._xOffset = static_cast<int>(execute(SCI_GETXOFFSET));
	buf._pos._selMode = execute(SCI_GETSELECTIONMODE);
}

void ScintillaEditView::restoreCurrentPos(const Position & prevPos)
{
	int scroll2Top = 0 - (int(execute(SCI_GETLINECOUNT)) + 1);
	scroll(0, scroll2Top);
	Buffer & buf = _buffers[_currentIndex];

	scroll(0, buf._pos._firstVisibleLine);
	if (buf._pos._selMode == SC_SEL_RECTANGLE)
	{
		execute(SCI_SETSELECTIONMODE, buf._pos._selMode);
	}
	execute(SCI_SETSELECTIONSTART, buf._pos._startPos);
	execute(SCI_SETSELECTIONEND, buf._pos._endPos);
	execute(SCI_SETXOFFSET, buf._pos._xOffset);
}

//! \brief this method activates the doc and the corresponding sub tab
//! \brief return the index of previeus current doc
char * ScintillaEditView::activateDocAt(int index)
{
	::SendMessage(_hParent, NPPM_INTERNAL_DOCSWITCHOFF, 0, (LPARAM)_hSelf);

	// before activating another document, we get the current position
	// from the Scintilla view then save it to the current document
	saveCurrentPos();
	Position & prevDocPos = _buffers[_currentIndex]._pos;

	// get foldStateIOnfo of current doc
	std::vector<HeaderLineState> lineStateVector;
	int maxLine = execute(SCI_GETLINECOUNT);

	for (int line = 0; line < maxLine; line++) 
	{
		int level = execute(SCI_GETFOLDLEVEL, line);
		if (level & SC_FOLDLEVELHEADERFLAG) 
		{
			bool expanded = (execute(SCI_GETFOLDEXPANDED, line) != 0);
			lineStateVector.push_back(HeaderLineState(line, expanded));
		}
	}
	
	// put the state into the future ex buffer
	_buffers[_currentIndex]._foldState = lineStateVector;

	// increase current doc ref count to 2 
	execute(SCI_ADDREFDOCUMENT, 0, _buffers[_currentIndex]._doc);

	// change the doc, this operation will decrease 
	// the ref count of old current doc to 1
	// then increase the new current doc to 2
	execute(SCI_SETDOCPOINTER, 0, _buffers[index]._doc);

	// Important : to avoid the leak of memory
	// Now keep the ref counter of new current doc as 1
	int refCtr = execute(SCI_RELEASEDOCUMENT, 0, _buffers[index]._doc);
	
	// NOW WE TAKE NEW DOC AND WE THROW OUT THE OLD ONE
	_currentIndex = index;
	
	_buffers[_currentIndex].increaseRecentTag();

	// restore the collapsed info
	int nbLineState = _buffers[_currentIndex]._foldState.size();
	for (int i = 0 ; i < nbLineState ; i++)
	{
		HeaderLineState &hls = _buffers[_currentIndex]._foldState[i];
		bool expanded = (execute(SCI_GETFOLDEXPANDED, hls._headerLineNumber) != 0);
		// set line to state folded
		if (hls._isCollapsed && !expanded)
			execute(SCI_TOGGLEFOLD, hls._headerLineNumber);

		if (!hls._isCollapsed && expanded)
			execute(SCI_TOGGLEFOLD, hls._headerLineNumber);
	}

    //if (isDocTypeDiff)
    defineDocType(_buffers[_currentIndex]._lang);

	restoreCurrentPos(prevDocPos);

	execute(SCI_SETEOLMODE, _buffers[_currentIndex]._format);
	::SendMessage(_hParent, NPPM_INTERNAL_DOCSWITCHIN, 0, (LPARAM)_hSelf);

	recalcHorizontalScrollbar();		//Update scrollbar after switching file

    return _buffers[_currentIndex]._fullPathName;
}

// this method creates a new doc ,and adds it into 
// the end of the doc list and a last sub tab, then activate it
// it returns the name of this created doc (that's the current doc also)
char * ScintillaEditView::createNewDoc(const char *fn)
{
	Document newDoc = execute(SCI_CREATEDOCUMENT);
	_buffers.push_back(Buffer(newDoc, fn));
	_buffers[_buffers.size()-1].checkIfReadOnlyFile();
	return activateDocAt(int(_buffers.size())-1);
}

char * ScintillaEditView::createNewDoc(int nbNew)
{
	char title[10];
	char nb[4];
	strcat(strcpy(title, UNTITLED_STR), _itoa(nbNew, nb, 10));
	char * newTitle = createNewDoc(title);
	if (getCurrentBuffer()._unicodeMode != uni8Bit)
		execute(SCI_SETCODEPAGE, SC_CP_UTF8);
	return newTitle;
}

void ScintillaEditView::collapse(int level2Collapse, bool mode)
{
	execute(SCI_COLOURISE, 0, -1);
	int maxLine = execute(SCI_GETLINECOUNT);

	for (int line = 0; line < maxLine; line++) 
	{
		int level = execute(SCI_GETFOLDLEVEL, line);
		if (level & SC_FOLDLEVELHEADERFLAG) 
		{
			level -= SC_FOLDLEVELBASE;
			if (level2Collapse == (level & SC_FOLDLEVELNUMBERMASK))
				if ((execute(SCI_GETFOLDEXPANDED, line) != 0) != mode)
					execute(SCI_TOGGLEFOLD, line);
		}
	}
	recalcHorizontalScrollbar();		//Update scrollbar after folding
}

void ScintillaEditView::foldCurrentPos(bool mode)
{
	execute(SCI_COLOURISE, 0, -1);
	int currentLine = this->getCurrentLineNumber();

	int headerLine;
	int level = execute(SCI_GETFOLDLEVEL, currentLine);
		
	if (level & SC_FOLDLEVELHEADERFLAG)
		headerLine = currentLine;
	else
	{
		headerLine = execute(SCI_GETFOLDPARENT, currentLine);
		if (headerLine == -1)
			return;
	}
	if ((execute(SCI_GETFOLDEXPANDED, headerLine) != 0) != mode)
		execute(SCI_TOGGLEFOLD, headerLine);

	recalcHorizontalScrollbar();		//Update scrollbar after folding
}

void ScintillaEditView::foldAll(bool mode)
{
	execute(SCI_COLOURISE, 0, -1);
	int maxLine = execute(SCI_GETLINECOUNT);

	for (int line = 0; line < maxLine; line++) 
	{
		int level = execute(SCI_GETFOLDLEVEL, line);
		if (level & SC_FOLDLEVELHEADERFLAG) 
			if ((execute(SCI_GETFOLDEXPANDED, line) != 0) != mode)
				execute(SCI_TOGGLEFOLD, line);
	}
	recalcHorizontalScrollbar();		//Update scrollbar after folding
}

// return the index to close then (argument) the index to activate
int ScintillaEditView::closeCurrentDoc(int & i2Activate)
{
	int oldCurrent = _currentIndex;

    Position & prevDocPos = _buffers[_currentIndex]._pos;

	// if the file 2 delete is the last one
	if (_currentIndex == int(_buffers.size()) - 1)
    {
		// if current index is 0, ie. the current is the only one
		if (!_currentIndex)
		{
			_currentIndex = 0;
		}
		// the current is NOT the only one and it is the last one,
		// we set it to the index which precedes it
		else
			_currentIndex -= 1;
    }
	// else the next current index will be the same,
	// we do nothing

	// get the iterator and calculate its position with the old current index value
	buf_vec_t::iterator posIt = _buffers.begin() + oldCurrent;

	// erase the position given document from our list
	_buffers.erase(posIt);

	// set another document, so the ref count of old active document owned
	// by Scintilla view will be decreased to 0 by SCI_SETDOCPOINTER message
	// then increase the new current doc to 2
	execute(SCI_SETDOCPOINTER, 0, _buffers[_currentIndex]._doc);

	// Important : to avoid the leak of memory
	// Now keep the ref counter of new current doc as 1
	execute(SCI_RELEASEDOCUMENT, 0, _buffers[_currentIndex]._doc);

	defineDocType(_buffers[_currentIndex]._lang);
	restoreCurrentPos(prevDocPos);
	
	// restore the collapsed info
	int nbLineState = _buffers[_currentIndex]._foldState.size();
	for (int i = 0 ; i < nbLineState ; i++)
	{
		HeaderLineState &hls = _buffers[_currentIndex]._foldState[i];
		bool expanded = (execute(SCI_GETFOLDEXPANDED, hls._headerLineNumber) != 0);
		// set line to state folded
		if (hls._isCollapsed && !expanded)
			execute(SCI_TOGGLEFOLD, hls._headerLineNumber);

		if (!hls._isCollapsed && expanded)
			execute(SCI_TOGGLEFOLD, hls._headerLineNumber);
	}

    i2Activate = _currentIndex;
	
	return oldCurrent;
}

void ScintillaEditView::closeDocAt(int i2Close)
{
		execute(SCI_RELEASEDOCUMENT, 0, _buffers[i2Close]._doc);

	// get the iterator and calculate its position with the old current index value
	buf_vec_t::iterator posIt = _buffers.begin() + i2Close;

	// erase the position given document from our list
	_buffers.erase(posIt);

    _currentIndex -= (i2Close < _currentIndex)?1:0;
}

void ScintillaEditView::removeAllUnusedDocs()
{
	// unreference all docs  from list of Scintilla
	// by sending SCI_RELEASEDOCUMENT message
	for (int i = 0 ; i < int(_buffers.size()) ; i++)
		if (i != _currentIndex)
			execute(SCI_RELEASEDOCUMENT, 0, _buffers[i]._doc);
	
	// remove all docs except the current doc from list
	_buffers.clear();
}

void ScintillaEditView::getText(char *dest, int start, int end) 
{
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = dest;
	execute(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}

void ScintillaEditView::marginClick(int position, int modifiers)
{
	int lineClick = int(execute(SCI_LINEFROMPOSITION, position, 0));
	int levelClick = int(execute(SCI_GETFOLDLEVEL, lineClick, 0));
	if (levelClick & SC_FOLDLEVELHEADERFLAG)
    {
		if (modifiers & SCMOD_SHIFT)
        {
			// Ensure all children visible
			execute(SCI_SETFOLDEXPANDED, lineClick, 1);
			expand(lineClick, true, true, 100, levelClick);
		}
        else if (modifiers & SCMOD_CTRL) 
        {
			if (execute(SCI_GETFOLDEXPANDED, lineClick, 0)) 
            {
				// Contract this line and all children
				execute(SCI_SETFOLDEXPANDED, lineClick, 0);
				expand(lineClick, false, true, 0, levelClick);
			} 
            else 
            {
				// Expand this line and all children
				execute(SCI_SETFOLDEXPANDED, lineClick, 1);
				expand(lineClick, true, true, 100, levelClick);
			}
		} 
        else 
        {
			// Toggle this line
			execute(SCI_TOGGLEFOLD, lineClick, 0);
		}
	}
	recalcHorizontalScrollbar();		//Update scrollbar after folding
}

void ScintillaEditView::expand(int &line, bool doExpand, bool force, int visLevels, int level)
{
	int lineMaxSubord = int(execute(SCI_GETLASTCHILD, line, level & SC_FOLDLEVELNUMBERMASK));
	line++;
	while (line <= lineMaxSubord)
    {
		if (force) 
        {
			if (visLevels > 0)
				execute(SCI_SHOWLINES, line, line);
			else
				execute(SCI_HIDELINES, line, line);
		} 
        else 
        {
			if (doExpand)
				execute(SCI_SHOWLINES, line, line);
		}
		int levelLine = level;
		if (levelLine == -1)
			levelLine = int(execute(SCI_GETFOLDLEVEL, line, 0));
		if (levelLine & SC_FOLDLEVELHEADERFLAG)
        {
			if (force) 
            {
				if (visLevels > 1)
					execute(SCI_SETFOLDEXPANDED, line, 1);
				else
					execute(SCI_SETFOLDEXPANDED, line, 0);
				expand(line, doExpand, force, visLevels - 1);
			} 
            else
            {
				if (doExpand)
                {
					if (!execute(SCI_GETFOLDEXPANDED, line, 0))
						execute(SCI_SETFOLDEXPANDED, line, 1);

					expand(line, true, force, visLevels - 1);
				} 
                else 
                {
					expand(line, false, force, visLevels - 1);
				}
			}
		}
        else
        {
			line++;
		}
	}
	recalcHorizontalScrollbar();		//Update scrollbar after folding
}

void ScintillaEditView::makeStyle(const char *lexerName, const char **keywordArray)
{
	LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName(lexerName);
	if (pStyler)
	{
		for (int i = 0 ; i < pStyler->getNbStyler() ; i++)
		{
			Style & style = pStyler->getStyler(i);
			setStyle(style._styleID, style._fgColor, style._bgColor, style._fontName, style._fontStyle, style._fontSize);
			if (keywordArray)
			{
				if ((style._keywordClass != -1) && (style._keywords))
					keywordArray[style._keywordClass] = style._keywords->c_str();
			}
		}
	}
}

void ScintillaEditView::performGlobalStyles() 
{
	StyleArray & stylers = _pParameter->getMiscStylerArray();

	int i = stylers.getStylerIndexByName("Current line background colour");
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		execute(SCI_SETCARETLINEBACK, style._bgColor);
	}

	i = stylers.getStylerIndexByName("Mark colour");
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		execute(SCI_MARKERSETFORE, 1, style._fgColor);
		execute(SCI_MARKERSETBACK, 1, style._bgColor);
	}

    COLORREF selectColorBack = grey;

	i = stylers.getStylerIndexByName("Selected text colour");
	if (i != -1)
    {
        Style & style = stylers.getStyler(i);
		selectColorBack = style._bgColor;
		execute(SCI_SETSELBACK, 1, selectColorBack);
    }

    COLORREF caretColor = black;
	i = stylers.getStylerIndexByID(SCI_SETCARETFORE);
	if (i != -1)
    {
        Style & style = stylers.getStyler(i);
        caretColor = style._fgColor;
    }
    execute(SCI_SETCARETFORE, caretColor);

	COLORREF edgeColor = liteGrey;
	i = stylers.getStylerIndexByName("Edge colour");
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		edgeColor = style._fgColor;
	}
	execute(SCI_SETEDGECOLOUR, edgeColor);

	COLORREF foldMarginColor = grey;
	COLORREF foldMarginHiColor = white;
	i = stylers.getStylerIndexByName("Fold margin");
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		foldMarginHiColor = style._fgColor;
		foldMarginColor = style._bgColor;
	}
	execute(SCI_SETFOLDMARGINCOLOUR, true, foldMarginColor);
	execute(SCI_SETFOLDMARGINHICOLOUR, true, foldMarginHiColor);

	COLORREF foldfgColor = white;
	COLORREF foldbgColor = grey;
	i = stylers.getStylerIndexByName("Fold");

	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		foldfgColor = style._bgColor;
		foldbgColor = style._fgColor;
	}
	for (int j = 0 ; j < NB_FOLDER_STATE ; j++)
        defineMarker(_markersArray[FOLDER_TYPE][j], _markersArray[_folderStyle][j], foldfgColor, foldbgColor);

	COLORREF wsSymbolFgColor = black;
	i = stylers.getStylerIndexByName("White space symbol");
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		wsSymbolFgColor = style._fgColor;
	}
	execute(SCI_SETWHITESPACEFORE, true, wsSymbolFgColor);
}

void ScintillaEditView::setLineIndent(int line, int indent) const {
	if (indent < 0)
		return;
	CharacterRange crange = getSelection();
	int posBefore = execute(SCI_GETLINEINDENTPOSITION, line);
	execute(SCI_SETLINEINDENTATION, line, indent);
	int posAfter = execute(SCI_GETLINEINDENTPOSITION, line);
	int posDifference = posAfter - posBefore;
	if (posAfter > posBefore) {
		// Move selection on
		if (crange.cpMin >= posBefore) {
			crange.cpMin += posDifference;
		}
		if (crange.cpMax >= posBefore) {
			crange.cpMax += posDifference;
		}
	} else if (posAfter < posBefore) {
		// Move selection back
		if (crange.cpMin >= posAfter) {
			if (crange.cpMin >= posBefore)
				crange.cpMin += posDifference;
			else
				crange.cpMin = posAfter;
		}
		if (crange.cpMax >= posAfter) {
			if (crange.cpMax >= posBefore)
				crange.cpMax += posDifference;
			else
				crange.cpMax = posAfter;
		}
	}
	execute(SCI_SETSEL, crange.cpMin, crange.cpMax);
}

const char * ScintillaEditView::getCompleteKeywordList(std::string & kwl, LangType langType, int keywordIndex)
{
	kwl += " ";
	const char *defKwl = _pParameter->getWordList(langType, keywordIndex);
	kwl += defKwl?defKwl:"";
	return kwl.c_str();
}

void ScintillaEditView::convertSelectedTextTo(bool Case) 
{
	unsigned int codepage = _codepage;
	UniMode um = getCurrentBuffer().getUnicodeMode();
	if (um != uni8Bit)
		codepage = CP_UTF8;

	if (execute(SCI_SELECTIONISRECTANGLE))
	{
		execute(SCI_BEGINUNDOACTION);

		ColumnModeInfo cmi = getColumnModeSelectInfo();
		const int len = cmi[0].second - cmi[0].first;
		char *srcStr = new char[len];
		wchar_t *destStr = new wchar_t[len];
		for (size_t i = 0 ; i < cmi.size() ; i++)
		{
			int start = cmi[i].first;
			int end = cmi[i].second;
			getText(srcStr, start, end);

			int nbChar = ::MultiByteToWideChar(codepage, 0, srcStr, len, destStr, len);

			for (int j = 0 ; j < nbChar ; j++)
			{
				if (Case == UPPERCASE)
					destStr[j] = (wchar_t)::CharUpperW((LPWSTR)destStr[j]);
				else
					destStr[j] = (wchar_t)::CharLowerW((LPWSTR)destStr[j]);
			}
			::WideCharToMultiByte(codepage, 0, destStr, len, srcStr, len, NULL, NULL);

			execute(SCI_SETTARGETSTART, start);
			execute(SCI_SETTARGETEND, end);
			execute(SCI_REPLACETARGET, -1, (LPARAM)srcStr);
		}

		delete [] srcStr;
		delete [] destStr;

		execute(SCI_ENDUNDOACTION);
		return;
	}

	size_t selectionStart = execute(SCI_GETSELECTIONSTART);
	size_t selectionEnd = execute(SCI_GETSELECTIONEND);
	
	int strSize = ((selectionEnd > selectionStart)?(selectionEnd - selectionStart):(selectionStart - selectionEnd))+1;
	
	if (strSize)
	{
		char *selectedStr = new char[strSize];
		int strWSize = strSize * 2;
		WCHAR *selectedStrW = new WCHAR[strWSize];

		execute(SCI_GETSELTEXT, 0, (LPARAM)selectedStr);

		int nbChar = ::MultiByteToWideChar(codepage, 0, selectedStr, strSize, selectedStrW, strWSize);

		for (int i = 0 ; i < nbChar ; i++)
		{
			if (Case == UPPERCASE)
				selectedStrW[i] = (WCHAR)::CharUpperW((LPWSTR)selectedStrW[i]);
			else
				selectedStrW[i] = (WCHAR)::CharLowerW((LPWSTR)selectedStrW[i]);
		}
		::WideCharToMultiByte(codepage, 0, selectedStrW, strWSize, selectedStr, strSize, NULL, NULL);

		execute(SCI_REPLACESEL, strSize, (LPARAM)selectedStr);
		execute(SCI_SETSEL, selectionStart, selectionEnd);
		delete [] selectedStr;
		delete [] selectedStrW;
	}
}

bool ScintillaEditView::expandWordSelection()
{
	int caretPos = execute(SCI_GETCURRENTPOS, 0, 0);
	int startPos = static_cast<int>(execute(SCI_WORDSTARTPOSITION, caretPos, true));
	int endPos = static_cast<int>(execute(SCI_WORDENDPOSITION, caretPos, true));
	if (startPos != endPos) {
		execute(SCI_SETSELECTIONSTART, startPos);
		execute(SCI_SETSELECTIONEND, endPos);
		return true;
	}
	return false;
}


void ScintillaEditView::arrangeBuffers(UINT nItems, UINT *items) {
	// Do nothing if item size mismatches
	if (nItems != getNbDoc())
		return;
	int ncurpos = getCurrentDocIndex();
	int newpos = 0;
	UINT i;
	buf_vec_t tmp;
	for (i=0; i<nItems; ++i) 
	{
		tmp.push_back(_buffers[items[i]]);
	}
	for (i=0; i<nItems; ++i) 
	{
		if (tmp[i]._fullPathName[0] == 0)
			return; // abort if we find an invalid buffer.
		if (items[i] == ncurpos)
			newpos = i;
	}
	tmp.swap(_buffers);
	setCurrentIndex(newpos);
}

char * int2str(char *str, int strLen, int number, int base, int nbChiffre, bool isZeroLeading) 
{
	if (nbChiffre >= strLen) return NULL;
	char f[64];
	char fStr[2] = "d";
	if (base == 16)
		fStr[0] = 'X';
	else if (base == 8)
		fStr[0] = 'o';
	else if (base == 2)
	{
		//printInt(nbChiffre);
		const unsigned int MASK_ULONG_BITFORT = 0x80000000;
		int nbBits = sizeof(unsigned int) * 8;
		int nbBit2Shift = (nbChiffre >= nbBits)?nbBits:(nbBits - nbChiffre);
		unsigned long mask = MASK_ULONG_BITFORT >> nbBit2Shift;
		int i = 0; 
		for (; mask > 0 ; i++)
		{
			str[i] = (mask & number)?'1':'0';
			mask >>= 1;
		}
		str[i] = '\0';
	}

	if (!isZeroLeading)
	{
		if (base == 2)
		{
			char *j = str;
			for ( ; *j != '\0' ; j++)
				if (*j == '1')
					break;
			strcpy(str, j);
		}
		else
		{
			sprintf(f, "%%%s", fStr);
			sprintf(str, f, number);
		}
		int i = strlen(str);
		for ( ; i < nbChiffre ; i++)
			str[i] = ' ';
		str[i] = '\0';
	}
	else
	{
		if (base != 2)
		{
			sprintf(f, "%%.%d%s", nbChiffre, fStr);
			sprintf(str, f, number);
		}
		// else already done.
	}
	return str;
}

ColumnModeInfo ScintillaEditView::getColumnModeSelectInfo()
{
	ColumnModeInfo columnModeInfo;
	if (execute(SCI_SELECTIONISRECTANGLE))
	{
		int selStartAbsPos = execute(SCI_GETSELECTIONSTART);
		int selEndAbsPos = execute(SCI_GETSELECTIONEND);

		int startCol = execute(SCI_GETCOLUMN, selStartAbsPos);
		int endCol = execute(SCI_GETCOLUMN, selEndAbsPos);

		int startLine = execute(SCI_LINEFROMPOSITION, selStartAbsPos);
		int endLine = execute(SCI_LINEFROMPOSITION, selEndAbsPos);
		
		if (endCol < startCol)// another way of selection
		{
			int tmp = startCol;
			startCol = endCol;
			endCol = tmp;

			selStartAbsPos = execute(SCI_FINDCOLUMN, startLine, startCol);
			selEndAbsPos = execute(SCI_FINDCOLUMN, endLine, endCol);
		}

		bool zeroCharSelMode = true;
		for (int i = startLine ; i <= endLine ; i++)
		{		
			int absPosSelStartPerLine =  execute(SCI_FINDCOLUMN, i, startCol);
			int absPosSelEndPerLine = execute(SCI_FINDCOLUMN, i, endCol);

			if (absPosSelStartPerLine != absPosSelEndPerLine)
			{	
				zeroCharSelMode = false;
			}
			columnModeInfo.push_back(pair<int, int>(absPosSelStartPerLine, absPosSelEndPerLine));
		}

		if (!zeroCharSelMode)
		{
			for (int i = columnModeInfo.size() - 1 ; i >= 0 ; i--)
			{
				ColumnModeInfo::iterator it = columnModeInfo.begin() + i;
				if (it->first == it->second)
					columnModeInfo.erase(it);
			}
		}
	}
	return columnModeInfo;
}

void ScintillaEditView::columnReplace(ColumnModeInfo & cmi, const char *str)
{
	//for (int i = (int)cmi.size() - 1 ; i >= 0 ; i--)
	int totalDiff = 0;
	for (size_t i = 0 ; i < cmi.size() ; i++)
	{
		int len2beReplace = cmi[i].second - cmi[i].first;
		int diff = strlen(str) - len2beReplace;

		cmi[i].first += totalDiff;
		cmi[i].second += totalDiff;

		execute(SCI_SETTARGETSTART, cmi[i].first);
		execute(SCI_SETTARGETEND, cmi[i].second);
		execute(SCI_REPLACETARGET, -1, (LPARAM)str);

		totalDiff += diff;
		cmi[i].second += diff;
		//printStr("fin");
	}
}


void ScintillaEditView::columnReplace(ColumnModeInfo & cmi, int initial, int incr, unsigned char format)
{
	// 0000 00 00 : Dec BASE_10
	// 0000 00 01 : Hex BASE_16
	// 0000 00 10 : Oct BASE_08
	// 0000 00 11 : Bin BASE_02

	// 0000 01 00 : 0 leading

	//Defined in ScintillaEditView.h :
	//const unsigned char MASK_FORMAT = 0x03;
	//const unsigned char MASK_ZERO_LEADING = 0x04;

	unsigned char f = format & MASK_FORMAT;
	bool isZeroLeading = (MASK_ZERO_LEADING & format) != 0;
	
	int base = 10;
	if (f == BASE_16)
		base = 16;
	else if (f == BASE_08)
		base = 8;
	else if (f == BASE_02)
		base = 2;

	int endNumber = initial + incr * (cmi.size() - 1);
	int nbEnd = getNbChiffre(endNumber, base);
	int nbInit = getNbChiffre(initial, base);
	int nb = max(nbInit, nbEnd);

	char str[512];

	int totalDiff = 0;
	for (size_t i = 0 ; i < cmi.size() ; i++)
	{
		int len2beReplace = cmi[i].second - cmi[i].first;
		int diff = nb - len2beReplace;

		cmi[i].first += totalDiff;
		cmi[i].second += totalDiff;

		int2str(str, sizeof(str), initial, base, nb, isZeroLeading);
		
		execute(SCI_SETTARGETSTART, cmi[i].first);
		execute(SCI_SETTARGETEND, cmi[i].second);
		execute(SCI_REPLACETARGET, -1, (LPARAM)str);

		initial += incr;
		
		totalDiff += diff;
		cmi[i].second += diff;
	}
}


void ScintillaEditView::columnReplace(const ColumnModeInfo & cmi, const char ch)
{
	for (size_t i = 0 ; i < cmi.size() ; i++)
	{
		int len = cmi[i].second - cmi[i].first;
		string str(len, ch);
		execute(SCI_SETTARGETSTART, cmi[i].first);
		execute(SCI_SETTARGETEND, cmi[i].second);
		execute(SCI_REPLACETARGET, -1, (LPARAM)str.c_str());
	}
}
//This method recalculates the horizontal scrollbar based
//on the current visible text and styler.
void ScintillaEditView::recalcHorizontalScrollbar() 
{
	int curOffset = execute(SCI_GETXOFFSET);
	int maxPixel = 0, curLen;
	int numLines = int(execute(SCI_GETLINECOUNT));
	int startLine = execute(SCI_GETFIRSTVISIBLELINE);
	int endLine =  startLine + execute(SCI_LINESONSCREEN);
	if ( endLine >= (execute(SCI_GETLINECOUNT) - 1) )
		endLine--;
	long beginPosition, endPosition;

	int visibleLine = 0;
	for( int i = startLine ; i <= endLine ; i++ ) 
	{	
		//for all _visible_ lines
		visibleLine = (int) execute(SCI_DOCLINEFROMVISIBLE, i);			//get actual visible line, folding may offset lines
		endPosition = execute(SCI_GETLINEENDPOSITION, visibleLine);		//get character position from begin
        beginPosition = execute(SCI_POSITIONFROMLINE, visibleLine);		//and end of line

		curLen = execute(SCI_POINTXFROMPOSITION, 0, endPosition) -		//Then let Scintilla get pixel width with
				 execute(SCI_POINTXFROMPOSITION, 0, beginPosition);		//current styler
		if (maxPixel < curLen) {										//If its the largest line yet
			maxPixel = curLen;											//Use that length
		}
	}
	
	if (maxPixel == 0)
		maxPixel++;														//make sure maxPixel is valid

	int currentLength = execute(SCI_GETSCROLLWIDTH);					//Get current scrollbar size
	if (currentLength != maxPixel)										//And if it is not the same
		execute(SCI_SETSCROLLWIDTH, maxPixel);							//update it
}