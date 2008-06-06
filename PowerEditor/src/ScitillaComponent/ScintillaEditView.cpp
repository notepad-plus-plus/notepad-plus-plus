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
#include "constant.h"


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

//Array with all the names of all languages
LanguageName ScintillaEditView::langNames[L_EXTERNAL+1] = {
{"normal",		"Normal text",				"Normal text file",										L_TXT},					
{"php",			"PHP",						"PHP Hypertext Preprocessor file",						L_PHP},
{"c",			"C",						"C source file",										L_C},
{"cpp",			"C++",						"C++ source file",										L_CPP},
{"cs",			"C#",						"C# source file",										L_CS},
{"objc",		"Objective-C",				"Objective-C source file",								L_OBJC},
{"java",		"Java",						"Java source file",										L_JAVA},
{"rc",			"RC",						"Windows Resource file",								L_RC},
{"html",		"HTML",						"Hyper Text Markup Language file",						L_HTML},
{"xml",			"XML",						"eXtensible Markup Language file",						L_XML},
{"makefile",	"Makefile",					"Makefile",												L_MAKEFILE},
{"pascal",		"Pascal",					"Pascal source file",									L_PASCAL},
{"batch",		"Batch",					"Batch file",											L_BATCH},
{"ini",			"ini",						"MS ini file",											L_INI},
{"nfo",			"NFO",						"MSDOS Style/ASCII Art",								L_NFO},
{"udf",			"udf",						"User Define File",										L_USER},
{"asp",			"ASP",						"Active Server Pages script file",						L_ASP},
{"sql",			"SQL",						"Structured Query Language file",						L_SQL},
{"vb",			"VB",						"Visual Basic file",									L_VB},
{"javascript",	"JavaScript",				"JavaScript file",										L_JS},
{"css",			"CSS",						"Cascade Style Sheets File",							L_CSS},
{"perl",		"Perl",						"Perl source file",										L_PERL},
{"python",		"Python",					"Python file",											L_PYTHON},
{"lua",			"Lua",						"Lua source File",										L_LUA},
{"tex",			"TeX",						"TeX file",												L_TEX},
{"fortran",		"Fortran",					"Fortran source file",									L_FORTRAN},
{"bash",		"Shell",					"Unix script file",										L_BASH},
{"actionscript","Flash Action",				"Flash Action script file",								L_FLASH},		//WARNING, was "flash"
{"nsis",		"NSIS",						"Nullsoft Scriptable Install System script file",		L_NSIS},
{"tcl",			"TCL",						"Tool Command Language file",							L_TCL},
{"lisp",		"Lisp",						"List Processing language file",						L_LISP},
{"scheme",		"Scheme",					"Scheme file",											L_SCHEME},
{"asm",			"Assembly",					"Assembly language source file",						L_ASM},
{"diff",		"Diff",						"Diff file",											L_DIFF},
{"props",		"Properties file",			"Properties file",										L_PROPS},
{"postscript",	"Postscript",				"Postscript file",										L_PS},
{"ruby",		"Ruby",						"Ruby file",											L_RUBY},
{"smalltalk",	"Smalltalk",				"Smalltalk file",										L_SMALLTALK},
{"vhdl",		"VHDL",						"VHSIC Hardware Description Language file",				L_VHDL},
{"kix",			"KiXtart",					"KiXtart file",											L_KIX},
{"autoit",		"AutoIt",					"AutoIt",												L_AU3},
{"caml",		"CAML",						"Categorical Abstract Machine Language",				L_CAML},
{"ada",			"Ada",						"Ada file",												L_ADA},
{"verilog",		"Verilog",					"Verilog file",											L_VERILOG},
{"matlab",		"MATLAB",					"MATrix LABoratory",									L_MATLAB},
{"haskell",		"Haskell",					"Haskell",												L_HASKELL},
{"inno",		"Inno",						"Inno Setup script",									L_INNO},
{"searchResult","Internal Search",			"Internal Search",										L_SEARCHRESULT},
{"cmake",		"CMAKEFILE",				"CMAKEFILE",											L_CMAKE},
{"yaml",		"YAML",						"YAML Ain't Markup Language",							L_YAML},
{"ext",			"External",					"External",												L_EXTERNAL}
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
    execute(SCI_SETFOLDFLAGS, 16);
	execute(SCI_SETSCROLLWIDTHTRACKING, true);
	execute(SCI_SETSCROLLWIDTH, 1);	//default empty document: override default width of 2000

	// smart hilighting
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_2, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_INC, INDIC_ROUNDBOX);

	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_2, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_INC, 100);

	_pParameter = NppParameters::getInstance();
	
	_codepage = ::GetACP();
	_oemCodepage = ::GetOEMCP();

	//Use either Unicode or ANSI setwindowlong, depending on environment
	if (::IsWindowUnicode(_hSelf)) 
	{
		::SetWindowLongW(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
		_callWindowProc = CallWindowProcW;
		_scintillaDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongW(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(scintillaStatic_Proc)));
	}
	else 
	{
		::SetWindowLongA(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
		_callWindowProc = CallWindowProcA;
		_scintillaDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongA(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(scintillaStatic_Proc)));
	}

	//Get the startup document and make a buffer for it so it can be accessed like a file
	//attachDefaultDoc();	//Let Notepad_plus do it
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

		case WM_MOUSEHWHEEL :
		{
			::CallWindowProc(_scintillaDefaultProc, hwnd, WM_HSCROLL, ((short)HIWORD(wParam) > 0)?SB_LINERIGHT:SB_LINELEFT, NULL);
			break;
		}

		case WM_MOUSEWHEEL :
		{
			if (LOWORD(wParam) & MK_RBUTTON)
			{
				::SendMessage(_hParent, Message, wParam, lParam);
				return TRUE;
			}

			//Have to perform the scroll first, because the first/last line do not get updated untill after the scroll has been parsed
			LRESULT scrollResult = ::CallWindowProc(_scintillaDefaultProc, hwnd, Message, wParam, lParam);
			return scrollResult;
			break;
		}

		case WM_VSCROLL :
		{
			break;
		}
	}
	return _callWindowProc(_scintillaDefaultProc, hwnd, Message, wParam, lParam);
}

void ScintillaEditView::setSpecialIndicator(Style & styleToSet)
{
	execute(SCI_INDICSETFORE, styleToSet._styleID, styleToSet._bgColor);
}


void ScintillaEditView::setSpecialStyle(Style & styleToSet)
{
	int styleID = styleToSet._styleID;
	if ( styleToSet._colorStyle & COLORSTYLE_FOREGROUND )
	    execute(SCI_STYLESETFORE, styleID, styleToSet._fgColor);

    if ( styleToSet._colorStyle & COLORSTYLE_BACKGROUND )
	    execute(SCI_STYLESETBACK, styleID, styleToSet._bgColor);
    
    if ((!styleToSet._fontName)||(strcmp(styleToSet._fontName, "")))
		execute(SCI_STYLESETFONT, (WPARAM)styleID, (LPARAM)styleToSet._fontName);

	int fontStyle = styleToSet._fontStyle;
    if (fontStyle != -1)
    {
        execute(SCI_STYLESETBOLD,		(WPARAM)styleID, fontStyle & FONTSTYLE_BOLD);
        execute(SCI_STYLESETITALIC,		(WPARAM)styleID, fontStyle & FONTSTYLE_ITALIC);
        execute(SCI_STYLESETUNDERLINE,	(WPARAM)styleID, fontStyle & FONTSTYLE_UNDERLINE);
    }

	if (styleToSet._fontSize > 0)
		execute(SCI_STYLESETSIZE, styleID, styleToSet._fontSize);
}

void ScintillaEditView::setStyle(Style styleToSet)
{
	GlobalOverride & go = _pParameter->getGlobalOverrideStyle();
	//go.enableBg = true;

	if (go.isEnable())
	{
		StyleArray & stylers = _pParameter->getMiscStylerArray();
		int i = stylers.getStylerIndexByName("Global override");
		if (i != -1)
		{
			Style & style = stylers.getStyler(i);

			if (go.enableFg) {
				if (style._colorStyle & COLORSTYLE_FOREGROUND) {
					styleToSet._colorStyle |= COLORSTYLE_FOREGROUND;
					styleToSet._fgColor = style._fgColor;
				} else {
					if (styleToSet._styleID == STYLE_DEFAULT) {	//if global is set to transparent, use default style color
						styleToSet._colorStyle |= COLORSTYLE_FOREGROUND;
					} else {
						styleToSet._colorStyle &= ~COLORSTYLE_FOREGROUND;
					}
				}
			}
			if (go.enableBg) {
				if (style._colorStyle & COLORSTYLE_BACKGROUND) {
					styleToSet._colorStyle |= COLORSTYLE_BACKGROUND;
					styleToSet._bgColor = style._bgColor;
				} else {
					if (styleToSet._styleID == STYLE_DEFAULT) {	//if global is set to transparent, use default style color
						styleToSet._colorStyle |= COLORSTYLE_BACKGROUND;
					} else {
						styleToSet._colorStyle &= ~COLORSTYLE_BACKGROUND;
					}
				}
			}
			if (go.enableFont && style._fontName && style._fontName[0])
				styleToSet._fontName = style._fontName;
			if (go.enableFontSize && (style._fontSize > 0))
				styleToSet._fontSize = style._fontSize;

			if (style._fontStyle != -1)
			{	
				if (go.enableBold)
				{
					if (style._fontStyle & FONTSTYLE_BOLD)
						styleToSet._fontStyle |= FONTSTYLE_BOLD;
					else
						styleToSet._fontStyle &= ~FONTSTYLE_BOLD;
				}
				if (go.enableItalic)
				{
					if (style._fontStyle & FONTSTYLE_ITALIC)
						styleToSet._fontStyle |= FONTSTYLE_ITALIC;
					else 
						styleToSet._fontStyle &= ~FONTSTYLE_ITALIC;
				}
				if (go.enableUnderLine)
				{
					if (style._fontStyle & FONTSTYLE_UNDERLINE)
						styleToSet._fontStyle |= FONTSTYLE_UNDERLINE;
					else
						styleToSet._fontStyle &= ~FONTSTYLE_UNDERLINE;
				}
			}
		}
	}
	setSpecialStyle(styleToSet);
}


void ScintillaEditView::setXmlLexer(LangType type)
{
	if (type == L_XML)
	{
        execute(SCI_SETLEXER, SCLEX_HTML);
		for (int i = 0 ; i < 4 ; i++)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(""));

        makeStyle(type);
	}
	else if ((type == L_HTML) || (type == L_PHP) || (type == L_ASP))
	{

        execute(SCI_SETLEXER, SCLEX_XML);

        const char *htmlKeyWords =_pParameter->getWordList(L_HTML, LANG_INDEX_INSTR);
        execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(htmlKeyWords?htmlKeyWords:""));

		makeStyle(type);
		
        setEmbeddedJSLexer();
        setPhpEmbeddedLexer();
		setEmbeddedAspLexer();
	}
}

void ScintillaEditView::setEmbeddedJSLexer()
{
	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle(L_JS, pKwArray);

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
	makeStyle(L_PHP, pKwArray);

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
	makeStyle(L_ASP, pKwArray);

	std::string keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
		keywordList = pKwArray[LANG_INDEX_INSTR];

	execute(SCI_SETKEYWORDS, 2, (LPARAM)getCompleteKeywordList(keywordList, L_VB, LANG_INDEX_INSTR));

    execute(SCI_STYLESETEOLFILLED, SCE_HBA_DEFAULT, true);
}

void ScintillaEditView::setUserLexer(const char *userLangName)
{
    execute(SCI_SETLEXER, SCLEX_USER);

	UserLangContainer & userLangContainer = userLangName?NppParameters::getInstance()->getULCFromName(userLangName):*(_userDefineDlg._pCurrentUserLang);
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
		setStyle(style);
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

			setStyle(style);

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

	const char *lexerName = ScintillaEditView::langNames[langType].lexerName;
	/*char *lexerName;
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
	}*/

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
		LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName(lexerName);	
		if (pStyler)
		{
			for (int i = 0 ; i < pStyler->getNbStyler() ; i++)
			{
				Style style = pStyler->getStyler(i);	//not by reference, but copy
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
				style._styleID = cppID;
				setStyle(style);
			}
		}
		execute(SCI_STYLESETEOLFILLED, SCE_C_DEFAULT, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENTLINE, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENT, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENTDOC, true);
	}

	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle(langType, pKwArray);

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

	//const char *lexerName = "objc";
	//if (langType == L_FLASH)
	//	lexerName = "actionscript";

	makeStyle(langType, pKwArray);
	
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

void ScintillaEditView::setLexer(int lexerID, LangType langType, int whichList)
{
	execute(SCI_SETLEXER, lexerID);

	const char *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	
	makeStyle(langType, pKwArray);

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

void ScintillaEditView::makeStyle(LangType language, const char **keywordArray)
{
	const char * lexerName = ScintillaEditView::langNames[language].lexerName;
	LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName(lexerName);
	if (pStyler)
	{
		for (int i = 0 ; i < pStyler->getNbStyler() ; i++)
		{
			Style & style = pStyler->getStyler(i);
			setStyle(style);
			if (keywordArray)
			{
				if ((style._keywordClass != -1) && (style._keywords))
					keywordArray[style._keywordClass] = style._keywords->c_str();
			}
		}
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
		styleDefault._colorStyle = COLORSTYLE_ALL;	//override transparency
	    setStyle(styleDefault);
    }

    execute(SCI_STYLECLEARALL);

    int iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE);
    if (iFind != -1)
    {
        Style & styleFind = stylers.getStyler(iFind);
	    setSpecialIndicator(styleFind);
    }

	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_2);
    if (iFind != -1)
    {
        Style & styleFind = stylers.getStyler(iFind);
	    //setSpecialStyle(styleFind);
		setSpecialIndicator(styleFind);
    }

	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_INC);
    if (iFind != -1)
    {
        Style & styleFind = stylers.getStyler(iFind);
	    //setSpecialStyle(styleFind);
		setSpecialIndicator(styleFind);
    }

	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_SELECT_STYLE);
    if (iFind != -1)
    {
        Style & styleFind = stylers.getStyler(iFind);
	    setSpecialStyle(styleFind);
    }
    
    int caretWidth = 1;
    
	
    // Il faut surtout faire un test ici avant d'exécuter SCI_SETCODEPAGE
    // Sinon y'aura un soucis de performance!
	if (isCJK())
	{
		if (getCurrentBuffer()->getUnicodeMode() == uni8Bit)
			execute(SCI_SETCODEPAGE, _codepage);
	}

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
			
		case L_USER : {
			const char * langExt = _currentBuffer->getUserDefineLangName();
			if (langExt[0])
				setUserLexer(langExt); 
			else
				setUserLexer();
			break; }

        case L_NFO :
		{
			LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName("nfo");
			COLORREF bg = black;
			COLORREF fg = liteGrey;
			Style nfoStyle;
			nfoStyle._styleID = STYLE_DEFAULT;
			nfoStyle._fontName = "MS LineDraw";
			if (pStyler)
			{
				int i = pStyler->getStylerIndexByName("DEFAULT");
				if (i != -1)
				{
					Style & style = pStyler->getStyler(i);
					nfoStyle._bgColor = style._bgColor;
					nfoStyle._fgColor = style._fgColor;
					nfoStyle._colorStyle = style._colorStyle;
				}
			}

			setStyle(nfoStyle);
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
            setSchemeLexer(); break;

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
	    setStyle(styleIG);
    }

	static int indexOfBraceLight = stylers.getStylerIndexByID(STYLE_BRACELIGHT);
	if (indexOfBraceLight != -1)
    {
        static Style & styleBL = stylers.getStyler(indexOfBraceLight);
	    setStyle(styleBL);
    }
	//setStyle(STYLE_CONTROLCHAR, liteGrey);

	static int indexBadBrace = stylers.getStylerIndexByID(STYLE_BRACEBAD);
	if (indexBadBrace != -1)
    {
        static Style & styleBB = stylers.getStyler(indexBadBrace);
	    setStyle(styleBB);
    }

	static int indexLineNumber = stylers.getStylerIndexByID(STYLE_LINENUMBER);
	if (indexLineNumber != -1)
    {
        static Style & styleLN = stylers.getStyler(indexLineNumber);
	    setSpecialStyle(styleLN);
    }

	execute(SCI_SETTABWIDTH, ((NppParameters::getInstance())->getNppGUI())._tabSize);
	execute(SCI_SETUSETABS, !((NppParameters::getInstance())->getNppGUI())._tabReplacedBySpace);

	int bitsNeeded = execute(SCI_GETSTYLEBITSNEEDED);
	execute(SCI_SETSTYLEBITS, bitsNeeded);
}

BufferID ScintillaEditView::attachDefaultDoc()
{
	// get the doc pointer attached (by default) on the view Scintilla
	Document doc = execute(SCI_GETDOCPOINTER, 0, 0);
	BufferID id = MainFileManager->bufferFromDocument(doc, false);//true);	//keep counter on 1
	Buffer * buf = MainFileManager->getBufferByID(id);

	MainFileManager->addBufferReference(id, this);	//add a reference. Notepad only shows the buffer in tabbar

	_currentBufferID = id;
	_currentBuffer = buf;
	bufferUpdated(buf, BufferChangeMask);	//make sure everything is in sync with the buffer, since no reference exists

	return id;
}

void ScintillaEditView::saveCurrentPos()
{
	//Save data so, that the current topline becomes visible again after restoring.
	int displayedLine = static_cast<int>(execute(SCI_GETFIRSTVISIBLELINE));
	int docLine = execute(SCI_DOCLINEFROMVISIBLE, displayedLine);		//linenumber of the line displayed in the top
	//int offset = displayedLine - execute(SCI_VISIBLEFROMDOCLINE, docLine);		//use this to calc offset of wrap. If no wrap this should be zero

	Buffer * buf = MainFileManager->getBufferByID(_currentBufferID);

	Position pos;
	// the correct visible line number
	pos._firstVisibleLine = docLine;
	pos._startPos = static_cast<int>(execute(SCI_GETSELECTIONSTART));
	pos._endPos = static_cast<int>(execute(SCI_GETSELECTIONEND));
	pos._xOffset = static_cast<int>(execute(SCI_GETXOFFSET));
	pos._selMode = execute(SCI_GETSELECTIONMODE);
	pos._scrollWidth = execute(SCI_GETSCROLLWIDTH);

	buf->setPosition(pos, this);
}

void ScintillaEditView::restoreCurrentPos()
{
	Buffer * buf = MainFileManager->getBufferByID(_currentBufferID);
	Position & pos = buf->getPosition(this);

	execute(SCI_GOTOPOS, 0);	//make sure first line visible by setting caret there, will scroll to top of document

	execute(SCI_SETSELECTIONMODE, pos._selMode);
	execute(SCI_SETSELECTIONSTART, pos._startPos);
	execute(SCI_SETSELECTIONEND, pos._endPos);
	if (!isWrap()) {	//only offset if not wrapping, otherwise the offset isnt needed at all
		execute(SCI_SETSCROLLWIDTH, pos._scrollWidth);
		execute(SCI_SETXOFFSET, pos._xOffset);
	}

	int lineToShow = execute(SCI_VISIBLEFROMDOCLINE, pos._firstVisibleLine);
	scroll(0, lineToShow);
}

//! \brief this method activates the doc and the corresponding sub tab
//! \brief return the index of previeus current doc
void ScintillaEditView::restyleBuffer() {
	execute(SCI_CLEARDOCUMENTSTYLE);
	execute(SCI_COLOURISE, 0, -1);
	_currentBuffer->setNeedsLexing(false);
}

void ScintillaEditView::styleChange() {
	defineDocType(_currentBuffer->getLangType());
}

void ScintillaEditView::activateBuffer(BufferID buffer)
{
	if (buffer == BUFFER_INVALID)
		return;
	if (buffer == _currentBuffer)
		return;
	Buffer * newBuf = MainFileManager->getBufferByID(buffer);

	// before activating another document, we get the current position
	// from the Scintilla view then save it to the current document
	saveCurrentPos();

	// get foldStateInfo of current doc
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
	_currentBuffer->setHeaderLineState(lineStateVector, this);

	_currentBufferID = buffer;	//the magical switch happens here
	_currentBuffer = newBuf;
	// change the doc, this operation will decrease 
	// the ref count of old current doc and increase the one of the new doc. FileManager should manage the rest
	// Note that the actual reference in the Buffer itself is NOT decreased, Notepad_plus does that if neccessary
	execute(SCI_SETDOCPOINTER, 0, _currentBuffer->getDocument());

	// Due to execute(SCI_CLEARDOCUMENTSTYLE); in defineDocType() function
	// defineDocType() function should be called here, but not be after the fold info loop
	defineDocType(_currentBuffer->getLangType());

	if (_currentBuffer->getNeedsLexing()) {
		restyleBuffer();
	}

	// restore the collapsed info
	std::vector<HeaderLineState> & lineStateVectorNew = newBuf->getHeaderLineState(this);
	int nbLineState = lineStateVectorNew.size();
	for (int i = 0 ; i < nbLineState ; i++)
	{
		HeaderLineState & hls = lineStateVectorNew.at(i);
		bool expanded = (execute(SCI_GETFOLDEXPANDED, hls._headerLineNumber) != 0);
		// set line to state folded
		if (hls._isExpanded != expanded)
			execute(SCI_TOGGLEFOLD, hls._headerLineNumber);
	}

	restoreCurrentPos();

	bufferUpdated(_currentBuffer, (BufferChangeMask & ~BufferChangeLanguage));	//everything should be updated, but the language (which undoes some operations done here like folding)

	//setup line number margin
	int numLines = execute(SCI_GETLINECOUNT);

	char numLineStr[32];
	itoa(numLines, numLineStr, 10);
	int nbDigit = strlen(numLineStr);

	if (increaseMaxNbDigit(nbDigit))
		setLineNumberWidth(hasMarginShowed(ScintillaEditView::_SC_MARGE_LINENUMBER));

	runMarkers(true, 0, true, false);
    return;	//all done
}
void ScintillaEditView::bufferUpdated(Buffer * buffer, int mask) {
	//actually only care about language and lexing etc
	if (buffer == _currentBuffer) {
		if (mask & BufferChangeLanguage) {
			defineDocType(buffer->getLangType());
			foldAll(fold_uncollapse);
			if (buffer->getNeedsLexing()) {
				restyleBuffer();
			}	
		}
		if (mask & BufferChangeFormat) {
			execute(SCI_SETEOLMODE, _currentBuffer->getFormat());
		}
		if (mask & BufferChangeReadonly) {
			execute(SCI_SETREADONLY, _currentBuffer->isReadOnly());
		}
		if (mask & BufferChangeUnicode) {
			if (_currentBuffer->getUnicodeMode() == uni8Bit) {	//either 0 or CJK codepage
				if (isCJK()) {
					execute(SCI_SETCODEPAGE, _codepage);	//you may also want to set charsets here, not yet implemented
				} else {
					execute(SCI_SETCODEPAGE, 0);
				}
			} else {	//CP UTF8 for all unicode
				execute(SCI_SETCODEPAGE, SC_CP_UTF8);
			}
		}
	}
}

void ScintillaEditView::collapse(int level2Collapse, bool mode)
{
	execute(SCI_COLOURISE, 0, -1);	//TODO: is this needed?
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

	runMarkers(true, 0, true, false);
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
}

void ScintillaEditView::getText(char *dest, int start, int end) const
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
			runMarkers(true, lineClick, true, false);
		}
	}
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

	runMarkers(true, 0, true, false);
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
/*
	i = stylers.getStylerIndexByName("Mark colour");
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		execute(SCI_MARKERSETFORE, 1, style._fgColor);
		execute(SCI_MARKERSETBACK, 1, style._bgColor);
	}
*/
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
	UniMode um = getCurrentBuffer()->getUnicodeMode();
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

void ScintillaEditView::foldChanged(int line, int levelNow, int levelPrev)
{
	if (levelNow & SC_FOLDLEVELHEADERFLAG)		//line can be folded
	{
		if (!(levelPrev & SC_FOLDLEVELHEADERFLAG))	//but previously couldnt
		{
			// Adding a fold point.
			execute(SCI_SETFOLDEXPANDED, line, 1);
			expand(line, true, false, 0, levelPrev);
		}
	}
	else if (levelPrev & SC_FOLDLEVELHEADERFLAG)
	{
		if (!execute(SCI_GETFOLDEXPANDED, line))
		{
			// Removing the fold from one that has been contracted so should expand
			// otherwise lines are left invisible with no way to make them visible
			execute(SCI_SETFOLDEXPANDED, line, 1);
			expand(line, true, false, 0, levelPrev);
		}
	}
	else if (!(levelNow & SC_FOLDLEVELWHITEFLAG) &&
	        ((levelPrev & SC_FOLDLEVELNUMBERMASK) > (levelNow & SC_FOLDLEVELNUMBERMASK)))
	{
		// See if should still be hidden
		int parentLine = execute(SCI_GETFOLDPARENT, line);
		if ((parentLine < 0) || (execute(SCI_GETFOLDEXPANDED, parentLine) && execute(SCI_GETLINEVISIBLE, parentLine)))
			execute(SCI_SHOWLINES, line, line);
	}
}

void ScintillaEditView::hideLines() {
	//Folding can screw up hide lines badly if it unfolds a hidden section.
	//Adding runMarkers(hide, foldstart) directly (folding on single document) can help

	//Special func on buffer. If markers are added, create notification with location of start, and hide bool set to true
	int startLine = execute(SCI_LINEFROMPOSITION, execute(SCI_GETSELECTIONSTART));
	int endLine = execute(SCI_LINEFROMPOSITION, execute(SCI_GETSELECTIONEND));
	//perform range check: cannot hide very first and very last lines
	//Offset them one off the edges, and then check if they are within the reasonable
	int nrLines = execute(SCI_GETLINECOUNT);
	if (nrLines < 3)
		return;	//cannot possibly hide anything
	if (!startLine)
		startLine++;
	if (endLine == (nrLines-1))
		endLine--;

	if (startLine > endLine)
		return;	//tried to hide line at edge

	//Hide the lines. We add marks on the outside of the hidden section and hide the lines
	//execute(SCI_HIDELINES, startLine, endLine);
	//Add markers
	execute(SCI_MARKERADD, startLine-1, MARK_HIDELINESBEGIN);
	execute(SCI_MARKERADD, endLine+1, MARK_HIDELINESEND);

	//remove any markers in between
	int scope = 0;
	for(int i = startLine; i <= endLine; i++) {
		int state = execute(SCI_MARKERGET, i);
		bool closePresent = ((state & (1 << MARK_HIDELINESEND)) != 0);	//check close first, then open, since close closes scope
		bool openPresent = ((state & (1 << MARK_HIDELINESBEGIN)) != 0);
		if (closePresent) {
			execute(SCI_MARKERDELETE, i, MARK_HIDELINESEND);
			if (scope > 0) scope--;
		}
		if (openPresent) {
			execute(SCI_MARKERDELETE, i, MARK_HIDELINESBEGIN);
			scope++;
		}
	}
	if (scope != 0) {	//something went wrong
		//Someone managed to make overlapping hidelines sections.
		//We cant do anything since this isnt supposed to happen
	}

	_currentBuffer->setHideLineChanged(true, startLine-1);
}

bool ScintillaEditView::markerMarginClick(int lineNumber) {

	int state = execute(SCI_MARKERGET, lineNumber);
	bool openPresent = ((state & (1 << MARK_HIDELINESBEGIN)) != 0);
	bool closePresent = ((state & (1 << MARK_HIDELINESEND)) != 0);

	if (!openPresent && !closePresent)
		return false;

	//Special func on buffer. First call show with location of opening marker. Then remove the marker manually
	if (openPresent) {
		_currentBuffer->setHideLineChanged(false, lineNumber);
	}
	if (closePresent) {
		openPresent = false;
		for(lineNumber--; lineNumber >= 0 && !openPresent; lineNumber--) {
			state = execute(SCI_MARKERGET, lineNumber);
			openPresent = ((state & (1 << MARK_HIDELINESBEGIN)) != 0);
		}
		if (openPresent) {
			_currentBuffer->setHideLineChanged(false, lineNumber);
		}
	}

	return true;
}

void ScintillaEditView::notifyMarkers(Buffer * buf, bool isHide, int location, bool del) {
	if (buf != _currentBuffer)	//if not visible buffer dont do a thing
		return;
	runMarkers(isHide, location, false, del);
}
//Run through full document. When switching in or opening folding
//hide is false only when user click on margin
void ScintillaEditView::runMarkers(bool doHide, int searchStart, bool endOfDoc, bool doDelete) {
	//Removes markers if opening
	/*
	AllLines = (start,ENDOFDOCUMENT)
	Hide:
		Run through all lines.
			Find open hiding marker:
				set hiding start
			Find closing:
				if (hiding):
					Hide lines between now and start
					if (endOfDoc = false)
						return
					else
						search for other hidden sections
		
	Show:
		Run through all lines
			Find open hiding marker
				set last start
			Find closing:
				Show from last start. Stop.
			Find closed folding header:
				Show from last start to folding header
				Skip to LASTCHILD
				Set last start to lastchild
	*/
	int maxLines = execute(SCI_GETLINECOUNT);
	if (doHide) {
		int startHiding = searchStart;
		bool isInSection = false;
		for(int i = searchStart; i < maxLines; i++) {
			int state = execute(SCI_MARKERGET, i);
			if ( ((state & (1 << MARK_HIDELINESEND)) != 0) ) {
				if (isInSection) {
					execute(SCI_HIDELINES, startHiding, i-1);
					if (!endOfDoc) {
						return;	//done, only single section requested
					}	//otherwise keep going
				}
				isInSection = false;
			}
			if ( ((state & (1 << MARK_HIDELINESBEGIN)) != 0) ) {
				isInSection = true;
				startHiding = i+1;
			}

		}
	} else {
		int startShowing = searchStart;
		bool isInSection = false;
		for(int i = searchStart; i < maxLines; i++) {
			int state = execute(SCI_MARKERGET, i);
			if ( ((state & (1 << MARK_HIDELINESEND)) != 0) ) {
				if (doDelete)
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESEND);
				if (isInSection) {
					if (startShowing >= i) {	//because of fold skipping, we passed the close tag. In that case we cant do anything
						if (!endOfDoc) {
							return;
						} else {
							continue;
						}
					}
					execute(SCI_SHOWLINES, startShowing, i-1);
					if (!endOfDoc) {
						return;	//done, only single section requested
					}	//otherwise keep going
				}
				isInSection = false;
			}
			if ( ((state & (1 << MARK_HIDELINESBEGIN)) != 0) ) {
				isInSection = true;
				startShowing = i+1;
				if (doDelete)
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESBEGIN);
			}

			int levelLine = execute(SCI_GETFOLDLEVEL, i, 0);
			if (levelLine & SC_FOLDLEVELHEADERFLAG) {	//fold section. Dont show lines if fold is closed
				if (isInSection && execute(SCI_GETFOLDEXPANDED, i) == 0) {
					execute(SCI_SHOWLINES, startShowing, i);
					startShowing = execute(SCI_GETLASTCHILD, i, (levelLine & SC_FOLDLEVELNUMBERMASK));
				}
			}
		}
	}
}
