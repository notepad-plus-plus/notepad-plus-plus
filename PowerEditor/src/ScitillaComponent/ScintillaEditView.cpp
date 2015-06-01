// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <shlwapi.h>
#include "ScintillaEditView.h"
#include "Parameters.h"
#include "Sorters.h"
#include "TCHAR.h"
#include <memory>

using namespace std;

// initialize the static variable

// get full ScinLexer.dll path to avoid hijack
TCHAR * getSciLexerFullPathName(TCHAR * moduleFileName, size_t len){
	::GetModuleFileName(NULL, moduleFileName, len);
	::PathRemoveFileSpec(moduleFileName);
	::PathAppend(moduleFileName, TEXT("SciLexer.dll"));
	return moduleFileName;
};

TCHAR moduleFileName[1024];
HINSTANCE ScintillaEditView::_hLib = ::LoadLibrary(getSciLexerFullPathName(moduleFileName, 1024));
int ScintillaEditView::_refCount = 0;
UserDefineDialog ScintillaEditView::_userDefineDlg;

const int ScintillaEditView::_SC_MARGE_LINENUMBER = 0;
const int ScintillaEditView::_SC_MARGE_SYBOLE = 1;
const int ScintillaEditView::_SC_MARGE_FOLDER = 2;
//const int ScintillaEditView::_SC_MARGE_MODIFMARKER = 3;

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
{TEXT("normal"),		TEXT("Normal text"),		TEXT("Normal text file"),								L_TEXT,			SCLEX_NULL},
{TEXT("php"),			TEXT("PHP"),				TEXT("PHP Hypertext Preprocessor file"),				L_PHP,			SCLEX_HTML},
{TEXT("c"),				TEXT("C"),					TEXT("C source file"),									L_C,			SCLEX_CPP},
{TEXT("cpp"),			TEXT("C++"),				TEXT("C++ source file"),								L_CPP,			SCLEX_CPP},
{TEXT("cs"),			TEXT("C#"),					TEXT("C# source file"),									L_CS,			SCLEX_CPP},
{TEXT("objc"),			TEXT("Objective-C"),		TEXT("Objective-C source file"),						L_OBJC,			SCLEX_CPP},
{TEXT("java"),			TEXT("Java"),				TEXT("Java source file"),								L_JAVA,			SCLEX_CPP},
{TEXT("rc"),			TEXT("RC"),					TEXT("Windows Resource file"),							L_RC,			SCLEX_CPP},
{TEXT("html"),			TEXT("HTML"),				TEXT("Hyper Text Markup Language file"),				L_HTML,			SCLEX_HTML},
{TEXT("xml"),			TEXT("XML"),				TEXT("eXtensible Markup Language file"),				L_XML,			SCLEX_XML},
{TEXT("makefile"),		TEXT("Makefile"),			TEXT("Makefile"),										L_MAKEFILE,		SCLEX_MAKEFILE},
{TEXT("pascal"),		TEXT("Pascal"),				TEXT("Pascal source file"),								L_PASCAL,		SCLEX_PASCAL},
{TEXT("batch"),			TEXT("Batch"),				TEXT("Batch file"),										L_BATCH,		SCLEX_BATCH},
{TEXT("ini"),			TEXT("ini"),				TEXT("MS ini file"),									L_INI,			SCLEX_PROPERTIES},
{TEXT("nfo"),			TEXT("NFO"),				TEXT("MSDOS Style/ASCII Art"),							L_ASCII,		SCLEX_NULL},
{TEXT("udf"),			TEXT("udf"),				TEXT("User Define File"),								L_USER,			SCLEX_USER},
{TEXT("asp"),			TEXT("ASP"),				TEXT("Active Server Pages script file"),				L_ASP,			SCLEX_HTML},
{TEXT("sql"),			TEXT("SQL"),				TEXT("Structured Query Language file"),					L_SQL,			SCLEX_SQL},
{TEXT("vb"),			TEXT("VB"),					TEXT("Visual Basic file"),								L_VB,			SCLEX_VB},
{TEXT("javascript"),	TEXT("JavaScript"),			TEXT("JavaScript file"),								L_JS,			SCLEX_CPP},
{TEXT("css"),			TEXT("CSS"),				TEXT("Cascade Style Sheets File"),						L_CSS,			SCLEX_CSS},
{TEXT("perl"),			TEXT("Perl"),				TEXT("Perl source file"),								L_PERL,			SCLEX_PERL},
{TEXT("python"),		TEXT("Python"),				TEXT("Python file"),									L_PYTHON,		SCLEX_PYTHON},
{TEXT("lua"),			TEXT("Lua"),				TEXT("Lua source File"),								L_LUA,			SCLEX_LUA},
{TEXT("tex"),			TEXT("TeX"),				TEXT("TeX file"),										L_TEX,			SCLEX_TEX},
{TEXT("fortran"),		TEXT("Fortran"),			TEXT("Fortran source file"),							L_FORTRAN,		SCLEX_FORTRAN},
{TEXT("bash"),			TEXT("Shell"),				TEXT("Unix script file"),								L_BASH,			SCLEX_BASH},
{TEXT("actionscript"),	TEXT("Flash Action"),		TEXT("Flash Action script file"),						L_FLASH,		SCLEX_CPP},//WARNING, was "flash"
{TEXT("nsis"),			TEXT("NSIS"),				TEXT("Nullsoft Scriptable Install System script file"),	L_NSIS,			SCLEX_NSIS},
{TEXT("tcl"),			TEXT("TCL"),				TEXT("Tool Command Language file"),						L_TCL,			SCLEX_TCL},
{TEXT("lisp"),			TEXT("Lisp"),				TEXT("List Processing language file"),					L_LISP,			SCLEX_LISP},
{TEXT("scheme"),		TEXT("Scheme"),				TEXT("Scheme file"),									L_SCHEME,		SCLEX_LISP},
{TEXT("asm"),			TEXT("Assembly"),			TEXT("Assembly language source file"),					L_ASM,			SCLEX_ASM},
{TEXT("diff"),			TEXT("Diff"),				TEXT("Diff file"),										L_DIFF,			SCLEX_DIFF},
{TEXT("props"),			TEXT("Properties file"),	TEXT("Properties file"),								L_PROPS,		SCLEX_PROPERTIES},
{TEXT("postscript"),	TEXT("Postscript"),			TEXT("Postscript file"),								L_PS,			SCLEX_PS},
{TEXT("ruby"),			TEXT("Ruby"),				TEXT("Ruby file"),										L_RUBY,			SCLEX_RUBY},
{TEXT("smalltalk"),		TEXT("Smalltalk"),			TEXT("Smalltalk file"),									L_SMALLTALK,	SCLEX_SMALLTALK},
{TEXT("vhdl"),			TEXT("VHDL"),				TEXT("VHSIC Hardware Description Language file"),		L_VHDL,			SCLEX_VHDL},
{TEXT("kix"),			TEXT("KiXtart"),			TEXT("KiXtart file"),									L_KIX,			SCLEX_KIX},
{TEXT("autoit"),		TEXT("AutoIt"),				TEXT("AutoIt"),											L_AU3,			SCLEX_AU3},
{TEXT("caml"),			TEXT("CAML"),				TEXT("Categorical Abstract Machine Language"),			L_CAML,			SCLEX_CAML},
{TEXT("ada"),			TEXT("Ada"),				TEXT("Ada file"),										L_ADA,			SCLEX_ADA},
{TEXT("verilog"),		TEXT("Verilog"),			TEXT("Verilog file"),									L_VERILOG,		SCLEX_VERILOG},
{TEXT("matlab"),		TEXT("MATLAB"),				TEXT("MATrix LABoratory"),								L_MATLAB,		SCLEX_MATLAB},
{TEXT("haskell"),		TEXT("Haskell"),			TEXT("Haskell"),										L_HASKELL,		SCLEX_HASKELL},
{TEXT("inno"),			TEXT("Inno"),				TEXT("Inno Setup script"),								L_INNO,			SCLEX_INNOSETUP},
{TEXT("searchResult"),	TEXT("Internal Search"),	TEXT("Internal Search"),								L_SEARCHRESULT,	SCLEX_SEARCHRESULT},
{TEXT("cmake"),			TEXT("CMAKEFILE"),			TEXT("CMAKEFILE"),										L_CMAKE,		SCLEX_CMAKE},
{TEXT("yaml"),			TEXT("YAML"),				TEXT("YAML Ain't Markup Language"),						L_YAML,			SCLEX_YAML},
{TEXT("cobol"),			TEXT("COBOL"),				TEXT("COmmon Business Oriented Language"),				L_COBOL,		SCLEX_COBOL},
{TEXT("gui4cli"),		TEXT("Gui4Cli"),			TEXT("Gui4Cli file"),									L_GUI4CLI,		SCLEX_GUI4CLI},
{TEXT("d"),				TEXT("D"),					TEXT("D programming language"),							L_D,			SCLEX_D},
{TEXT("powershell"),	TEXT("PowerShell"),			TEXT("Windows PowerShell"),								L_POWERSHELL,	SCLEX_POWERSHELL},
{TEXT("r"),				TEXT("R"),					TEXT("R programming language"),							L_R,			SCLEX_R},
{TEXT("jsp"),			TEXT("JSP"),				TEXT("JavaServer Pages script file"),					L_JSP,			SCLEX_HTML},
{TEXT("coffeescript"),	TEXT("CoffeeScript"),		TEXT("CoffeeScript file"),								L_COFFEESCRIPT,	SCLEX_COFFEESCRIPT},
{TEXT("ext"),			TEXT("External"),			TEXT("External"),										L_EXTERNAL,		SCLEX_NULL}
};

//const int MASK_RED   = 0xFF0000;
//const int MASK_GREEN = 0x00FF00;
//const int MASK_BLUE  = 0x0000FF;

int getNbDigits(int aNum, int base)
{
	int nbChiffre = 1;
	int diviseur = base;
	
	for (;;)
	{
		int result = aNum / diviseur;
		if (!result)
			break;
		else
		{
			diviseur *= base;
			++nbChiffre;
		}
	}
	if ((base == 16) && (nbChiffre % 2 != 0))
		nbChiffre += 1;

	return nbChiffre;
}

void ScintillaEditView::init(HINSTANCE hInst, HWND hPere)
{
	if (!_hLib)
	{
		throw std::exception("ScintillaEditView::init : SCINTILLA ERROR - Can not load the dynamic library");
	}

	Window::init(hInst, hPere);
   _hSelf = ::CreateWindowEx(
					WS_EX_CLIENTEDGE,\
					TEXT("Scintilla"),\
					TEXT("Notepad++"),\
					WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_EX_RTLREADING,\
					0, 0, 100, 100,\
					_hParent,\
					NULL,\
					_hInst,\
					NULL);

	if (!_hSelf)
	{
		throw std::exception("ScintillaEditView::init : CreateWindowEx() function return null");
	}

	_pScintillaFunc = (SCINTILLA_FUNC)::SendMessage(_hSelf, SCI_GETDIRECTFUNCTION, 0, 0);
	_pScintillaPtr = (SCINTILLA_PTR)::SendMessage(_hSelf, SCI_GETDIRECTPOINTER, 0, 0);

    _userDefineDlg.init(_hInst, _hParent, this);

	if (!_pScintillaFunc)
	{
		throw std::exception("ScintillaEditView::init : SCI_GETDIRECTFUNCTION message failed");
	}

	if (!_pScintillaPtr)
	{
		throw std::exception("ScintillaEditView::init : SCI_GETDIRECTPOINTER message failed");
	}

    execute(SCI_SETMARGINMASKN, _SC_MARGE_FOLDER, SC_MASK_FOLDERS);
    showMargin(_SC_MARGE_FOLDER, true);

    execute(SCI_SETMARGINMASKN, _SC_MARGE_SYBOLE, (1<<MARK_BOOKMARK) | (1<<MARK_HIDELINESBEGIN) | (1<<MARK_HIDELINESEND));

	execute(SCI_MARKERSETALPHA, MARK_BOOKMARK, 70);
	execute(SCI_MARKERDEFINEPIXMAP, MARK_BOOKMARK, (LPARAM)bookmark_xpm);
	execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESBEGIN, (LPARAM)acTop_xpm);
	execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESEND, (LPARAM)acBottom_xpm);

    execute(SCI_SETMARGINSENSITIVEN, _SC_MARGE_FOLDER, true);
    execute(SCI_SETMARGINSENSITIVEN, _SC_MARGE_SYBOLE, true);

    execute(SCI_SETFOLDFLAGS, 16);
	execute(SCI_SETSCROLLWIDTHTRACKING, true);
	execute(SCI_SETSCROLLWIDTH, 1);	//default empty document: override default width of 2000

	// smart hilighting
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_SMART, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_INC, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_TAGMATCH, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_TAGATTR, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_EXT1, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_EXT2, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_EXT3, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_EXT4, INDIC_ROUNDBOX);
	execute(SCI_INDICSETSTYLE, SCE_UNIVERSAL_FOUND_STYLE_EXT5, INDIC_ROUNDBOX);	

	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_SMART, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_INC, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_TAGMATCH, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_TAGATTR, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_EXT1, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_EXT2, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_EXT3, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_EXT4, 100);
	execute(SCI_INDICSETALPHA, SCE_UNIVERSAL_FOUND_STYLE_EXT5, 100);	

	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_FOUND_STYLE_SMART, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_FOUND_STYLE, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_FOUND_STYLE_INC, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_TAGMATCH, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_TAGATTR, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_FOUND_STYLE_EXT1, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_FOUND_STYLE_EXT2, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_FOUND_STYLE_EXT3, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_FOUND_STYLE_EXT4, true);
	execute(SCI_INDICSETUNDER, SCE_UNIVERSAL_FOUND_STYLE_EXT5, true);	
	_pParameter = NppParameters::getInstance();
	
	_codepage = ::GetACP();

	//Use either Unicode or ANSI setwindowlong, depending on environment
	if (::IsWindowUnicode(_hSelf))
	{
		::SetWindowLongPtrW(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
		_callWindowProc = CallWindowProcW;
		_scintillaDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(scintillaStatic_Proc)));
	}
	else 
	{
		::SetWindowLongPtrA(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
		_callWindowProc = CallWindowProcA;
		_scintillaDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrA(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(scintillaStatic_Proc)));
	}

	//Get the startup document and make a buffer for it so it can be accessed like a file
	attachDefaultDoc();
}

LRESULT CALLBACK ScintillaEditView::scintillaStatic_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	ScintillaEditView *pScint = (ScintillaEditView *)(::GetWindowLongPtr(hwnd, GWL_USERDATA));

	if (Message == WM_MOUSEWHEEL || Message == WM_MOUSEHWHEEL)
	{			
		POINT pt;
		POINTS pts = MAKEPOINTS(lParam);
		POINTSTOPOINT(pt, pts);
		HWND hwndOnMouse = WindowFromPoint(pt);

		//Hack for Synaptics TouchPad Driver
		char synapticsHack[26];
		GetClassNameA(hwndOnMouse, (LPSTR)&synapticsHack, 26);
		bool isSynpnatic = std::string(synapticsHack) == "SynTrackCursorWindowClass";
		bool makeTouchPadCompetible = ((NppParameters::getInstance())->getSVP())._disableAdvancedScrolling;

		if (isSynpnatic || makeTouchPadCompetible)
			return (pScint->scintillaNew_Proc(hwnd, Message, wParam, lParam));

		ScintillaEditView *pScintillaOnMouse = (ScintillaEditView *)(::GetWindowLongPtr(hwndOnMouse, GWL_USERDATA));
		if (pScintillaOnMouse != pScint)
			return ::SendMessage(hwndOnMouse, Message, wParam, lParam);
	}
	if (pScint)
		return (pScint->scintillaNew_Proc(hwnd, Message, wParam, lParam));
	else
		return ::DefWindowProc(hwnd, Message, wParam, lParam);

}
LRESULT ScintillaEditView::scintillaNew_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
{
	switch (Message)
	{
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

		case WM_IME_REQUEST:
		{
		
			if (wParam == IMR_RECONVERTSTRING)
			{
				int					textLength;
				int					selectSize;
				char				smallTextBuffer[128];
				char			  *	selectedStr = smallTextBuffer;
				RECONVERTSTRING   *	reconvert = (RECONVERTSTRING *)lParam;

				// does nothing with a rectangular selection
				if (execute(SCI_SELECTIONISRECTANGLE, 0, 0))
					return 0;

				// get the codepage of the text

				unsigned int codepage = execute(SCI_GETCODEPAGE);

				// get the current text selection

				CharacterRange range = getSelection();
				if (range.cpMax == range.cpMin)
				{
					// no selection: select the current word instead

					expandWordSelection();
					range = getSelection();
				}
				selectSize = range.cpMax - range.cpMin;

				// does nothing if still no luck with the selection

				if (selectSize == 0)
					return 0;

				if (selectSize + 1 > sizeof(smallTextBuffer))
					selectedStr = new char[selectSize + 1];
				getText(selectedStr, range.cpMin, range.cpMax);

				if (reconvert == NULL)
				{
					// convert the selection to Unicode, and get the number
					// of bytes required for the converted text
					textLength = sizeof(WCHAR) * ::MultiByteToWideChar(codepage, 0, selectedStr, selectSize, NULL, 0);
				}
				else
				{
					// convert the selection to Unicode, and store it at the end of the structure.
					// Beware: For a Unicode IME, dwStrLen , dwCompStrLen, and dwTargetStrLen
					// are TCHAR values, that is, character counts. The members dwStrOffset,
					// dwCompStrOffset, and dwTargetStrOffset specify byte counts.

					textLength = ::MultiByteToWideChar(	codepage, 0,
														selectedStr, selectSize,
														(LPWSTR)((LPSTR)reconvert + sizeof(RECONVERTSTRING)),
														reconvert->dwSize - sizeof(RECONVERTSTRING));

					// fill the structure
					reconvert->dwVersion		 = 0;
					reconvert->dwStrLen			 = textLength;
					reconvert->dwStrOffset		 = sizeof(RECONVERTSTRING);
					reconvert->dwCompStrLen		 = textLength;
					reconvert->dwCompStrOffset	 = 0;
					reconvert->dwTargetStrLen	 = reconvert->dwCompStrLen;
					reconvert->dwTargetStrOffset = reconvert->dwCompStrOffset;

					textLength *= sizeof(WCHAR);
				}

				if (selectedStr != smallTextBuffer)
					delete [] selectedStr;

				// return the total length of the structure
				return sizeof(RECONVERTSTRING) + textLength;
			}
			break;
		}

		case WM_KEYUP :
		{
			if (wParam == VK_PRIOR || wParam == VK_NEXT)
			{
				// find hotspots
				/*
				NMHDR nmhdr;
				nmhdr.code = SCN_PAINTED;
				nmhdr.hwndFrom = _hSelf;
				nmhdr.idFrom = ::GetDlgCtrlID(nmhdr.hwndFrom);
				::SendMessage(_hParent, WM_NOTIFY, (WPARAM)LINKTRIGGERED, (LPARAM)&nmhdr);
				*/
				SCNotification notification = {};
				notification.nmhdr.code = SCN_PAINTED;
				notification.nmhdr.hwndFrom = _hSelf;
				notification.nmhdr.idFrom = ::GetDlgCtrlID(_hSelf);
				::SendMessage(_hParent, WM_NOTIFY, (WPARAM)LINKTRIGGERED, (LPARAM)&notification);

			}			
			break;
		}

		case WM_VSCROLL :
		{
			break;
		}
	}
	return _callWindowProc(_scintillaDefaultProc, hwnd, Message, wParam, lParam);
}


void ScintillaEditView::setSpecialStyle(const Style & styleToSet)
{
	int styleID = styleToSet._styleID;
	if ( styleToSet._colorStyle & COLORSTYLE_FOREGROUND )
	    execute(SCI_STYLESETFORE, styleID, styleToSet._fgColor);

    if ( styleToSet._colorStyle & COLORSTYLE_BACKGROUND )
	    execute(SCI_STYLESETBACK, styleID, styleToSet._bgColor);
    
    if (styleToSet._fontName && lstrcmp(styleToSet._fontName, TEXT("")) != 0)
	{
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		const char * fontNameA = wmc->wchar2char(styleToSet._fontName, CP_ACP);
		execute(SCI_STYLESETFONT, (WPARAM)styleID, (LPARAM)fontNameA);
	}
	int fontStyle = styleToSet._fontStyle;
    if (fontStyle != STYLE_NOT_USED)
    {
        execute(SCI_STYLESETBOLD,		(WPARAM)styleID, fontStyle & FONTSTYLE_BOLD);
        execute(SCI_STYLESETITALIC,		(WPARAM)styleID, fontStyle & FONTSTYLE_ITALIC);
        execute(SCI_STYLESETUNDERLINE,	(WPARAM)styleID, fontStyle & FONTSTYLE_UNDERLINE);
    }

	if (styleToSet._fontSize > 0)
		execute(SCI_STYLESETSIZE, styleID, styleToSet._fontSize);
}

void ScintillaEditView::setHotspotStyle(Style& styleToSet)
{
	StyleMap* styleMap;
	if( _hotspotStyles.find(_currentBuffer) == _hotspotStyles.end() )
	{
		_hotspotStyles[_currentBuffer] = new StyleMap;
	}
	styleMap = _hotspotStyles[_currentBuffer];
	(*styleMap)[styleToSet._styleID] = styleToSet;
	
	setStyle(styleToSet);
}

void ScintillaEditView::setStyle(Style styleToSet)
{
	GlobalOverride & go = _pParameter->getGlobalOverrideStyle();

	if (go.isEnable())
	{
		StyleArray & stylers = _pParameter->getMiscStylerArray();
		int i = stylers.getStylerIndexByName(TEXT("Global override"));
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

			if (style._fontStyle != STYLE_NOT_USED)
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
        execute(SCI_SETLEXER, SCLEX_XML);
		for (int i = 0 ; i < 4 ; ++i)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(TEXT("")));

        makeStyle(type);
	}
	else if ((type == L_HTML) || (type == L_PHP) || (type == L_ASP) || (type == L_JSP))
	{
        execute(SCI_SETLEXER, SCLEX_HTML);
        const TCHAR *htmlKeyWords_generic =_pParameter->getWordList(L_HTML, LANG_INDEX_INSTR);

		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		const char *htmlKeyWords = wmc->wchar2char(htmlKeyWords_generic, CP_ACP);
		execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(htmlKeyWords?htmlKeyWords:""));
		makeStyle(L_HTML);
		
        setEmbeddedJSLexer();
        setEmbeddedPhpLexer();
		setEmbeddedAspLexer();
	}
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.html"), reinterpret_cast<LPARAM>("1"));
	// This allow to fold comment strem in php/javascript code
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.hypertext.comment"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::setEmbeddedJSLexer()
{
	const TCHAR *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle(L_JS, pKwArray);

	basic_string<char> keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordList = wstring2string(kwlW, CP_ACP);
	}

	execute(SCI_SETKEYWORDS, 1, (LPARAM)getCompleteKeywordList(keywordList, L_JS, LANG_INDEX_INSTR));
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_DEFAULT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENTDOC, true);
}

void ScintillaEditView::setEmbeddedPhpLexer()
{
	const TCHAR *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle(L_PHP, pKwArray);

	basic_string<char> keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordList = wstring2string(kwlW, CP_ACP);
	}

	execute(SCI_SETKEYWORDS, 4, (LPARAM)getCompleteKeywordList(keywordList, L_PHP, LANG_INDEX_INSTR));

	execute(SCI_STYLESETEOLFILLED, SCE_HPHP_DEFAULT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HPHP_COMMENT, true);
}

void ScintillaEditView::setEmbeddedAspLexer()
{
	const TCHAR *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle(L_ASP, pKwArray);

	basic_string<char> keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordList = wstring2string(kwlW, CP_ACP);
	}

	execute(SCI_SETKEYWORDS, 2, (LPARAM)getCompleteKeywordList(keywordList, L_VB, LANG_INDEX_INSTR));

    execute(SCI_STYLESETEOLFILLED, SCE_HBA_DEFAULT, true);
}

void ScintillaEditView::setUserLexer(const TCHAR *userLangName)
{
	int setKeywordsCounter = 0;
    execute(SCI_SETLEXER, SCLEX_USER);

	UserLangContainer * userLangContainer = userLangName?_pParameter->getULCFromName(userLangName):_userDefineDlg._pCurrentUserLang;

	if (!userLangContainer)
		return;

	UINT codepage = CP_ACP;
	UniMode unicodeMode = _currentBuffer->getUnicodeMode();
	int encoding = _currentBuffer->getEncoding();
	if (encoding == -1)
	{
		if (unicodeMode == uniUTF8 || unicodeMode == uniCookie)
			codepage = CP_UTF8;
	}
	else
	{
		codepage = CP_OEMCP;	// system OEM code page might not match user selection for character set, 
								// but this is the best match WideCharToMultiByte offers
	}

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.isCaseIgnored",		  (LPARAM)(userLangContainer->_isCaseIgnored ? "1":"0"));
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.allowFoldOfComments",    (LPARAM)(userLangContainer->_allowFoldOfComments ? "1":"0"));
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.foldCompact",		      (LPARAM)(userLangContainer->_foldCompact ? "1":"0"));

    char name[] = "userDefine.prefixKeywords0";
	for (int i=0 ; i<SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
	{	
		itoa(i+1, (name+25), 10);
		execute(SCI_SETPROPERTY, (WPARAM)name, (LPARAM)(userLangContainer->_isPrefix[i]?"1":"0"));
	}

	for (int i = 0 ; i < SCE_USER_KWLIST_TOTAL ; ++i)
	{
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		const char * keyWords_char = wmc->wchar2char(userLangContainer->_keywordLists[i], codepage);

		if (globalMappper().setLexerMapper.find(i) != globalMappper().setLexerMapper.end())
		{
            execute(SCI_SETPROPERTY, (WPARAM)globalMappper().setLexerMapper[i].c_str(), reinterpret_cast<LPARAM>(keyWords_char));
		}
		else // OPERATORS2, FOLDERS_IN_CODE2, FOLDERS_IN_COMMENT, KEYWORDS1-8
		{
			char temp[max_char];
			bool inDoubleQuote = false;
			bool inSingleQuote = false;
			bool nonWSFound = false;
			int index = 0;
			for (size_t j=0, len = strlen(keyWords_char); j<len; ++j)
			{
				if (!inSingleQuote && keyWords_char[j] == '"')
				{
					inDoubleQuote = !inDoubleQuote;
					continue;
				}

				if (!inDoubleQuote && keyWords_char[j] == '\'')
				{
					inSingleQuote = !inSingleQuote;
					continue;
				}

				if (keyWords_char[j] == '\\' && (keyWords_char[j+1] == '"' || keyWords_char[j+1] == '\'' || keyWords_char[j+1] == '\\'))
				{
					++j;
					temp[index++] = keyWords_char[j];
					continue;
				}

				if (inDoubleQuote || inSingleQuote)
				{
					if (keyWords_char[j] > ' ')		// copy non-whitespace unconditionally
					{
						temp[index++] = keyWords_char[j];
						if (nonWSFound == false)
							nonWSFound = true;
					}
					else if (nonWSFound == true && keyWords_char[j-1] != '"' && keyWords_char[j+1] != '"' && keyWords_char[j+1] > ' ')
					{
						temp[index++] = inDoubleQuote ? '\v' : '\b';
					}
					else
						continue;
				}
				else
				{
					temp[index++] = keyWords_char[j];
				}

			}
			temp[index++] = 0;
			execute(SCI_SETKEYWORDS, setKeywordsCounter++, reinterpret_cast<LPARAM>(temp));
		}
	}

 	char intBuffer[15];
	char nestingBuffer[] = "userDefine.nesting.00";

    itoa(userLangContainer->_forcePureLC, intBuffer, 10);
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.forcePureLC", reinterpret_cast<LPARAM>(intBuffer));

    itoa(userLangContainer->_decimalSeparator, intBuffer, 10);
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.decimalSeparator", reinterpret_cast<LPARAM>(intBuffer));

	// at the end (position SCE_USER_KWLIST_TOTAL) send id values
    itoa((int)userLangContainer->getName(), intBuffer, 10); // use numeric value of TCHAR pointer
	execute(SCI_SETPROPERTY, (WPARAM)"userDefine.udlName", reinterpret_cast<LPARAM>(intBuffer));

    itoa((int)_currentBufferID, intBuffer, 10); // use numeric value of BufferID pointer
    execute(SCI_SETPROPERTY, (WPARAM)"userDefine.currentBufferID", reinterpret_cast<LPARAM>(intBuffer));
	
	for (int i = 0 ; i < SCE_USER_STYLE_TOTAL_STYLES ; ++i)
	{
		Style & style = userLangContainer->_styleArray.getStyler(i);

		if (style._styleID == STYLE_NOT_USED)
			continue;

		if (i < 10)	itoa(i, (nestingBuffer+20), 10);
		else		itoa(i, (nestingBuffer+19), 10);
		execute(SCI_SETPROPERTY, (WPARAM)nestingBuffer, (LPARAM)(itoa(style._nesting, intBuffer, 10)));

		setStyle(style);
	}
}

void ScintillaEditView::setExternalLexer(LangType typeDoc)
{
	int id = typeDoc - L_EXTERNAL;
	TCHAR * name = _pParameter->getELCFromIndex(id)._name;
	
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	const char *pName = wmc->wchar2char(name, CP_ACP);

	execute(SCI_SETLEXERLANGUAGE, 0, (LPARAM)pName);

	LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName(name);	
	if (pStyler)
	{
		for (int i = 0 ; i < pStyler->getNbStyler() ; ++i)
		{
			Style & style = pStyler->getStyler(i);

			setStyle(style);

			if (style._keywordClass >= 0 && style._keywordClass <= KEYWORDSET_MAX)
			{			
				basic_string<char> keywordList("");
				if (style._keywords)
				{
					keywordList = wstring2string(*(style._keywords), CP_ACP);
				}
				execute(SCI_SETKEYWORDS, style._keywordClass, (LPARAM)getCompleteKeywordList(keywordList, typeDoc, style._keywordClass));
			}
		}
	}
}

void ScintillaEditView::setCppLexer(LangType langType)
{
    const char *cppInstrs;
    const char *cppTypes;
    const TCHAR *doxygenKeyWords  = _pParameter->getWordList(L_CPP, LANG_INDEX_TYPE2);

	const TCHAR *lexerName = ScintillaEditView::langNames[langType].lexerName;

    execute(SCI_SETLEXER, SCLEX_CPP);

	if ((langType != L_RC) && (langType != L_JS))
    {
        if (doxygenKeyWords)
		{
			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			const char * doxygenKeyWords_char = wmc->wchar2char(doxygenKeyWords, CP_ACP);
			execute(SCI_SETKEYWORDS, 2, (LPARAM)doxygenKeyWords_char);
		}
    }

	if (langType == L_JS)
	{
		LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName(lexerName);	
		if (pStyler)
		{
			for (int i = 0, nb = pStyler->getNbStyler() ; i < nb ; ++i)
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

	const TCHAR *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle(langType, pKwArray);

	basic_string<char> keywordListInstruction("");
	basic_string<char> keywordListType("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordListInstruction = wstring2string(kwlW, CP_ACP);
	}
	cppInstrs = getCompleteKeywordList(keywordListInstruction, langType, LANG_INDEX_INSTR);

	if (pKwArray[LANG_INDEX_TYPE])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_TYPE];
		keywordListType = wstring2string(kwlW, CP_ACP);
	}
	cppTypes = getCompleteKeywordList(keywordListType, langType, LANG_INDEX_TYPE);

	execute(SCI_SETKEYWORDS, 0, (LPARAM)cppInstrs);
	execute(SCI_SETKEYWORDS, 1, (LPARAM)cppTypes);

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));

	// Disable track preprocessor to avoid incorrect detection.
	// In the most of cases, the symbols are defined outside of file.
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.track.preprocessor"), reinterpret_cast<LPARAM>("0"));
}

void ScintillaEditView::setTclLexer()
{
	const char *tclInstrs;
    const char *tclTypes;


    execute(SCI_SETLEXER, SCLEX_TCL); 

	const TCHAR *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	makeStyle(L_TCL, pKwArray);

	basic_string<char> keywordListInstruction("");
	basic_string<char> keywordListType("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordListInstruction = wstring2string(kwlW, CP_ACP);
	}
	tclInstrs = getCompleteKeywordList(keywordListInstruction, L_TCL, LANG_INDEX_INSTR);

	if (pKwArray[LANG_INDEX_TYPE])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_TYPE];
		keywordListType = wstring2string(kwlW, CP_ACP);
	}
	tclTypes = getCompleteKeywordList(keywordListType, L_TCL, LANG_INDEX_TYPE);

	execute(SCI_SETKEYWORDS, 0, (LPARAM)tclInstrs);
	execute(SCI_SETKEYWORDS, 1, (LPARAM)tclTypes);
}

//used by Objective-C and Actionscript
void ScintillaEditView::setObjCLexer(LangType langType) 
{
    execute(SCI_SETLEXER, SCLEX_OBJC);

	const TCHAR *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

	makeStyle(langType, pKwArray);
	
	basic_string<char> objcInstr1Kwl("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		objcInstr1Kwl = wstring2string(pKwArray[LANG_INDEX_INSTR], CP_ACP);
	}
	const char *objcInstrs = getCompleteKeywordList(objcInstr1Kwl, langType, LANG_INDEX_INSTR);
	
	basic_string<char> objcInstr2Kwl("");
	if (pKwArray[LANG_INDEX_INSTR2])
	{
		objcInstr2Kwl = wstring2string(pKwArray[LANG_INDEX_INSTR2], CP_ACP);
	}
	const char *objCDirective = getCompleteKeywordList(objcInstr2Kwl, langType, LANG_INDEX_INSTR2);

	basic_string<char> objcTypeKwl("");
	if (pKwArray[LANG_INDEX_TYPE])
	{
		objcTypeKwl = wstring2string(pKwArray[LANG_INDEX_TYPE], CP_ACP);
	}
	const char *objcTypes = getCompleteKeywordList(objcTypeKwl, langType, LANG_INDEX_TYPE);
	

	basic_string<char> objcType2Kwl("");
	if (pKwArray[LANG_INDEX_TYPE2])
	{
		objcType2Kwl = wstring2string(pKwArray[LANG_INDEX_TYPE2], CP_ACP);
	}
	const char *objCQualifier = getCompleteKeywordList(objcType2Kwl, langType, LANG_INDEX_TYPE2);
	
	
	
	basic_string<char> doxygenKeyWordsString("");
	const TCHAR *doxygenKeyWordsW = _pParameter->getWordList(L_CPP, LANG_INDEX_TYPE2);
	if (doxygenKeyWordsW)
	{
		doxygenKeyWordsString = wstring2string(doxygenKeyWordsW, CP_ACP);
	}
	const char *doxygenKeyWords = doxygenKeyWordsString.c_str();

	execute(SCI_SETKEYWORDS, 0, (LPARAM)objcInstrs);
    execute(SCI_SETKEYWORDS, 1, (LPARAM)objcTypes);
	execute(SCI_SETKEYWORDS, 2, (LPARAM)doxygenKeyWords);
	execute(SCI_SETKEYWORDS, 3, (LPARAM)objCDirective);
	execute(SCI_SETKEYWORDS, 4, (LPARAM)objCQualifier);

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::setKeywords(LangType langType, const char *keywords, int index)
{
	std::basic_string<char> wordList;
	wordList = (keywords)?keywords:"";
	execute(SCI_SETKEYWORDS, index, (LPARAM)getCompleteKeywordList(wordList, langType, index));
}

void ScintillaEditView::setLexer(int lexerID, LangType langType, int whichList)
{
	execute(SCI_SETLEXER, lexerID);

	const TCHAR *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	
	makeStyle(langType, pKwArray);

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	if (whichList & LIST_0)
	{
		const char * keyWords_char = wmc->wchar2char(pKwArray[LANG_INDEX_INSTR], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_INSTR);
	}	

	if (whichList & LIST_1)
	{
		const char * keyWords_char = wmc->wchar2char(pKwArray[LANG_INDEX_INSTR2], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_INSTR2);
	}

	if (whichList & LIST_2)
	{
		const char * keyWords_char = wmc->wchar2char(pKwArray[LANG_INDEX_TYPE], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE);
	}

	if (whichList & LIST_3)
	{
		const char * keyWords_char = wmc->wchar2char(pKwArray[LANG_INDEX_TYPE2], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE2);
	}

	if (whichList & LIST_4)
	{
		const char * keyWords_char = wmc->wchar2char(pKwArray[LANG_INDEX_TYPE3], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE3);
	}

	if (whichList & LIST_5)
	{
		const char * keyWords_char = wmc->wchar2char(pKwArray[LANG_INDEX_TYPE4], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE4);
	}

	if (whichList & LIST_6)
	{
		const char * keyWords_char = wmc->wchar2char(pKwArray[LANG_INDEX_TYPE5], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE5);
	}
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::makeStyle(LangType language, const TCHAR **keywordArray)
{
	const TCHAR * lexerName = ScintillaEditView::langNames[language].lexerName;
	LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName(lexerName);
	if (pStyler)
	{
		for (int i = 0, nb = pStyler->getNbStyler(); i < nb ; ++i)
		{
			Style & style = pStyler->getStyler(i);
			setStyle(style);
			if (keywordArray)
			{
				if ((style._keywordClass != STYLE_NOT_USED) && (style._keywords))
					keywordArray[style._keywordClass] = style._keywords->c_str();
			}
		}
	}
}

void ScintillaEditView::defineDocType(LangType typeDoc)
{
    StyleArray & stylers = _pParameter->getMiscStylerArray();
    int iStyleDefault = stylers.getStylerIndexByID(STYLE_DEFAULT);
    if (iStyleDefault != -1)
    {
        Style & styleDefault = stylers.getStyler(iStyleDefault);
		styleDefault._colorStyle = COLORSTYLE_ALL;	//override transparency
	    setStyle(styleDefault);
    }

    execute(SCI_STYLECLEARALL);

	Style *pStyle;
	Style defaultIndicatorStyle;

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE;
	defaultIndicatorStyle._bgColor = red;
	pStyle = &defaultIndicatorStyle;
    int iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));  
    }
	setSpecialIndicator(*pStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_SMART;
	defaultIndicatorStyle._bgColor = liteGreen;
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_SMART);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_INC;
	defaultIndicatorStyle._bgColor = blue;
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_INC);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_TAGMATCH;
	defaultIndicatorStyle._bgColor = RGB(0x80, 0x00, 0xFF);
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_TAGMATCH);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_TAGATTR;
	defaultIndicatorStyle._bgColor = yellow;
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_TAGATTR);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);


	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT1;
	defaultIndicatorStyle._bgColor = cyan;
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_EXT1);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT2;
	defaultIndicatorStyle._bgColor = orange;
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_EXT2);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT3;
	defaultIndicatorStyle._bgColor = yellow;
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_EXT3);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT4;
	defaultIndicatorStyle._bgColor = purple;
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_EXT4);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT5;
	defaultIndicatorStyle._bgColor = darkGreen;
	pStyle = &defaultIndicatorStyle;
	iFind = stylers.getStylerIndexByID(SCE_UNIVERSAL_FOUND_STYLE_EXT5);
    if (iFind != -1)
    {
        pStyle = &(stylers.getStyler(iFind));
    }
	setSpecialIndicator(*pStyle);
	
    // Il faut surtout faire un test ici avant d'exécuter SCI_SETCODEPAGE
    // Sinon y'aura un soucis de performance!
	if (isCJK())
	{
		if (getCurrentBuffer()->getUnicodeMode() == uni8Bit)
		{
			if (typeDoc == L_CSS || typeDoc == L_CAML || typeDoc == L_ASM || typeDoc == L_MATLAB)
				execute(SCI_SETCODEPAGE, CP_ACP);
			else
				execute(SCI_SETCODEPAGE, _codepage);
		}
	}

	ScintillaViewParams & svp = (ScintillaViewParams &)_pParameter->getSVP();
	if (svp._folderStyle != FOLDER_STYLE_NONE)
		showMargin(_SC_MARGE_FOLDER, isNeededFolderMarge(typeDoc));

	switch (typeDoc)
	{
		case L_C :
		case L_CPP :
		case L_JS:
		case L_JAVA :
		case L_RC :
		case L_CS :
		case L_FLASH :
			setCppLexer(typeDoc); break;

		case L_TCL :
            setTclLexer(); break;

		
        case L_OBJC :
            setObjCLexer(typeDoc); break;
		
	    case L_PHP :
		case L_ASP :
        case L_JSP :
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
			const TCHAR * langExt = _currentBuffer->getUserDefineLangName();
			if (langExt[0])
				setUserLexer(langExt); 
			else
				setUserLexer();
			break; }

        case L_ASCII :
		{
			LexerStyler *pStyler = (_pParameter->getLStylerArray()).getLexerStylerByName(TEXT("nfo"));

			Style nfoStyle;
			nfoStyle._styleID = STYLE_DEFAULT;
			nfoStyle._fontName = TEXT("Lucida Console");
			nfoStyle._fontSize = 10;

			if (pStyler)
			{
				int i = pStyler->getStylerIndexByName(TEXT("DEFAULT"));
				if (i != -1)
				{
					Style & style = pStyler->getStyler(i);
					nfoStyle._bgColor = style._bgColor;
					nfoStyle._fgColor = style._fgColor;
					nfoStyle._colorStyle = style._colorStyle;
				}
			}
			setSpecialStyle(nfoStyle);
			execute(SCI_STYLECLEARALL);

			Buffer * buf = MainFileManager->getBufferByID(_currentBufferID);

			if (buf->getEncoding() != NPP_CP_DOS_437)
			{
			   buf->setEncoding(NPP_CP_DOS_437);
			   ::SendMessage(_hParent, WM_COMMAND, IDM_FILE_RELOAD, 0);
			}
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

        case L_COBOL :
			setCobolLexer(); break;

        case L_GUI4CLI :
			setGui4CliLexer(); break;

        case L_D :
			setDLexer(); break;

        case L_POWERSHELL :
			setPowerShellLexer(); break;

        case L_R :
			setRLexer(); break;

		case L_COFFEESCRIPT :
			setCoffeeScriptLexer(); break;

		case L_TEXT :
		default :
			if (typeDoc >= L_EXTERNAL && typeDoc < _pParameter->L_END)
				setExternalLexer(typeDoc);
			else
				execute(SCI_SETLEXER, (_codepage == CP_CHINESE_TRADITIONAL)?SCLEX_MAKEFILE:SCLEX_NULL);
			break;

	}
	//All the global styles should put here
	int indexOfIndentGuide = stylers.getStylerIndexByID(STYLE_INDENTGUIDE);
	if (indexOfIndentGuide != -1)
    {
        Style & styleIG = stylers.getStyler(indexOfIndentGuide);
	    setStyle(styleIG);
    }
	int indexOfBraceLight = stylers.getStylerIndexByID(STYLE_BRACELIGHT);
	if (indexOfBraceLight != -1)
    {
        Style & styleBL = stylers.getStyler(indexOfBraceLight);
	    setStyle(styleBL);
    }
	//setStyle(STYLE_CONTROLCHAR, liteGrey);
	int indexBadBrace = stylers.getStylerIndexByID(STYLE_BRACEBAD);
	if (indexBadBrace != -1)
    {
        Style & styleBB = stylers.getStyler(indexBadBrace);
	    setStyle(styleBB);
    }
	int indexLineNumber = stylers.getStylerIndexByID(STYLE_LINENUMBER);
	if (indexLineNumber != -1)
    {
        Style & styleLN = stylers.getStyler(indexLineNumber);
	    setSpecialStyle(styleLN);
    }
    setTabSettings(_pParameter->getLangFromID(typeDoc));
	execute(SCI_SETSTYLEBITS, 8);	// Always use 8 bit mask in Document class (Document::stylingBitsMask),
									// in that way Editor::PositionIsHotspot will return correct hotspot styleID.
									// This value has no effect on LexAccessor::mask.
}

BufferID ScintillaEditView::attachDefaultDoc()
{
	// get the doc pointer attached (by default) on the view Scintilla
	Document doc = execute(SCI_GETDOCPOINTER, 0, 0);
	execute(SCI_ADDREFDOCUMENT, 0, doc);
	BufferID id = MainFileManager->bufferFromDocument(doc, false, true);//true, true);	//keep counter on 1
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
	pos._startPos = static_cast<int>(execute(SCI_GETANCHOR));
	pos._endPos = static_cast<int>(execute(SCI_GETCURRENTPOS));
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

	execute(SCI_SETSELECTIONMODE, pos._selMode);	//enable
	execute(SCI_SETANCHOR, pos._startPos);
	execute(SCI_SETCURRENTPOS, pos._endPos);
	execute(SCI_CANCEL);							//disable
	if (!isWrap()) {	//only offset if not wrapping, otherwise the offset isnt needed at all
		execute(SCI_SETSCROLLWIDTH, pos._scrollWidth);
		execute(SCI_SETXOFFSET, pos._xOffset);
	}
	execute(SCI_CHOOSECARETX); // choose current x position

	int lineToShow = execute(SCI_VISIBLEFROMDOCLINE, pos._firstVisibleLine);
	scroll(0, lineToShow);
}

void ScintillaEditView::restyleBuffer() {
	execute(SCI_CLEARDOCUMENTSTYLE);
	execute(SCI_COLOURISE, 0, -1);
	_currentBuffer->setNeedsLexing(false);
}

void ScintillaEditView::styleChange() {
	defineDocType(_currentBuffer->getLangType());
	restyleBuffer();
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
	std::vector<size_t> lineStateVector;
	getCurrentFoldStates(lineStateVector);
	
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
    setTabSettings(_currentBuffer->getCurrentLang());

	if (_currentBuffer->getNeedsLexing()) {
		restyleBuffer();
	}

	// restore the collapsed info
	const std::vector<size_t> & lineStateVectorNew = newBuf->getHeaderLineState(this);
	syncFoldStateWith(lineStateVectorNew);

	restoreCurrentPos();

	bufferUpdated(_currentBuffer, (BufferChangeMask & ~BufferChangeLanguage));	//everything should be updated, but the language (which undoes some operations done here like folding)

	//setup line number margin
	int numLines = execute(SCI_GETLINECOUNT);

	char numLineStr[32];
	itoa(numLines, numLineStr, 10);

	runMarkers(true, 0, true, false);
    return;	//all done
}

void ScintillaEditView::getCurrentFoldStates(std::vector<size_t> & lineStateVector)
{
	//-- FLS: xCodeOptimization1304: For active document get folding state from Scintilla.
	//--      The code using SCI_CONTRACTEDFOLDNEXT is usually 10%-50% faster than checking each line of the document!!
	int contractedFoldHeaderLine = 0;

	do {
		contractedFoldHeaderLine = execute(SCI_CONTRACTEDFOLDNEXT, contractedFoldHeaderLine);
		if (contractedFoldHeaderLine != -1) 
		{
			//-- Store contracted line
			lineStateVector.push_back(contractedFoldHeaderLine);
			//-- Start next search with next line
			++contractedFoldHeaderLine;
		}
	} while (contractedFoldHeaderLine != -1);
}

void ScintillaEditView::syncFoldStateWith(const std::vector<size_t> & lineStateVectorNew)
{
	int nbLineState = lineStateVectorNew.size();
	for (int i = 0 ; i < nbLineState ; ++i)
	{
		int line = lineStateVectorNew.at(i);
		fold(line, false);
	}
}

void ScintillaEditView::bufferUpdated(Buffer * buffer, int mask)
{
	//actually only care about language and lexing etc
	if (buffer == _currentBuffer) 
	{
		if (mask & BufferChangeLanguage) 
		{
			defineDocType(buffer->getLangType());
			foldAll(fold_uncollapse);
		}

		if (mask & BufferChangeLexing)
		{
			if (buffer->getNeedsLexing())
			{
				restyleBuffer();	//sets to false, this will apply to any other view aswell
			}	//else nothing, otherwise infinite loop
		}

		if (mask & BufferChangeFormat) 
		{
			execute(SCI_SETEOLMODE, _currentBuffer->getFormat());
		}
		if (mask & BufferChangeReadonly) 
		{
			execute(SCI_SETREADONLY, _currentBuffer->isReadOnly());
		}
		if (mask & BufferChangeUnicode) 
		{
            int enc = CP_ACP;
			if (buffer->getUnicodeMode() == uni8Bit) 
			{	//either 0 or CJK codepage
				LangType typeDoc = buffer->getLangType();
				if (isCJK())
				{
					if (typeDoc == L_CSS || typeDoc == L_CAML || typeDoc == L_ASM || typeDoc == L_MATLAB)
						enc = CP_ACP;	//you may also want to set charsets here, not yet implemented
					else 
						enc = _codepage;
				}
                else
                    enc = CP_ACP;
			} 
			else	//CP UTF8 for all unicode
				enc = SC_CP_UTF8;
            execute(SCI_SETCODEPAGE, enc);
		}
	}
}

void ScintillaEditView::collapse(int level2Collapse, bool mode)
{
	execute(SCI_COLOURISE, 0, -1);

	int maxLine = execute(SCI_GETLINECOUNT);

	for (int line = 0; line < maxLine; ++line) 
	{
		int level = execute(SCI_GETFOLDLEVEL, line);
		if (level & SC_FOLDLEVELHEADERFLAG) 
		{
			level -= SC_FOLDLEVELBASE;
			if (level2Collapse == (level & SC_FOLDLEVELNUMBERMASK))
				if (isFolded(line) != mode)
				{
					fold(line, mode);
				}
		}
	}

	runMarkers(true, 0, true, false);
}

void ScintillaEditView::foldCurrentPos(bool mode)
{
	int currentLine = this->getCurrentLineNumber();
	fold(currentLine, mode);
}

void ScintillaEditView::fold(int line, bool mode)
{
    int endStyled = execute(SCI_GETENDSTYLED);
    int len = execute(SCI_GETTEXTLENGTH);

    if (endStyled < len)
        execute(SCI_COLOURISE, 0, -1);

	int headerLine;
	int level = execute(SCI_GETFOLDLEVEL, line);
		
	if (level & SC_FOLDLEVELHEADERFLAG)
		headerLine = line;
	else
	{
		headerLine = execute(SCI_GETFOLDPARENT, line);
		if (headerLine == -1)
			return;
	}

	if (isFolded(headerLine) != mode)
	{
		execute(SCI_TOGGLEFOLD, headerLine);

		SCNotification scnN;
		scnN.nmhdr.code = SCN_FOLDINGSTATECHANGED;
		scnN.nmhdr.hwndFrom = _hSelf;
		scnN.nmhdr.idFrom = 0;
		scnN.line = headerLine;
		scnN.foldLevelNow = isFolded(headerLine)?1:0; //folded:1, unfolded:0

		::SendMessage(_hParent, WM_NOTIFY, 0, (LPARAM)&scnN);
	}
}

void ScintillaEditView::foldAll(bool mode)
{
	int maxLine = execute(SCI_GETLINECOUNT);

	for (int line = 0; line < maxLine; ++line) 
	{
		int level = execute(SCI_GETFOLDLEVEL, line);
		if (level & SC_FOLDLEVELHEADERFLAG) 
			if (isFolded(line) != mode)
				fold(line, mode);
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

generic_string ScintillaEditView::getGenericTextAsString(int start, int end) const
{
	assert(end > start);
	const int bufSize = end - start + 1;
	TCHAR *buf = new TCHAR[bufSize];
	getGenericText(buf, bufSize, start, end);
	generic_string text = buf;
	delete[] buf;
	return text;
}

void ScintillaEditView::getGenericText(TCHAR *dest, size_t destlen, int start, int end) const
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	char *destA = new char[end - start + 1];
	getText(destA, start, end);
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const TCHAR *destW = wmc->char2wchar(destA, cp);
	_tcsncpy_s(dest, destlen, destW, _TRUNCATE);
	delete [] destA;
}

// "mstart" and "mend" are pointers to indexes in the read string,
// which are converted to the corresponding indexes in the returned TCHAR string.

void ScintillaEditView::getGenericText(TCHAR *dest, size_t destlen, int start, int end, int *mstart, int *mend) const
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	char *destA = new char[end - start + 1];
	getText(destA, start, end);
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const TCHAR *destW = wmc->char2wchar(destA, cp, mstart, mend);
	_tcsncpy_s(dest, destlen, destW, _TRUNCATE);
	delete [] destA;
}

void ScintillaEditView::insertGenericTextFrom(int position, const TCHAR *text2insert) const
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE);
	const char *text2insertA = wmc->wchar2char(text2insert, cp);
	execute(SCI_INSERTTEXT, position, (WPARAM)text2insertA);
}

void ScintillaEditView::replaceSelWith(const char * replaceText)
{
	execute(SCI_REPLACESEL, 0, (WPARAM)replaceText);
}

void ScintillaEditView::getVisibleStartAndEndPosition(int * startPos, int * endPos)
{
	assert(startPos != NULL && endPos != NULL);

	int firstVisibleLine = execute(SCI_GETFIRSTVISIBLELINE);
	*startPos = execute(SCI_POSITIONFROMLINE, execute(SCI_DOCLINEFROMVISIBLE, firstVisibleLine));
	int linesOnScreen = execute(SCI_LINESONSCREEN);
	int lineCount = execute(SCI_GETLINECOUNT);
	*endPos = execute(SCI_POSITIONFROMLINE, execute(SCI_DOCLINEFROMVISIBLE, firstVisibleLine + min(linesOnScreen, lineCount)));
	if (*endPos == -1) *endPos = execute(SCI_GETLENGTH);
}

char * ScintillaEditView::getWordFromRange(char * txt, int size, int pos1, int pos2)
{
    if (!size)
		return NULL;
    if (pos1 > pos2)
    {
        int tmp = pos1;
        pos1 = pos2;
        pos2 = tmp;
    }

    if (size < pos2-pos1)
        return NULL;

    getText(txt, pos1, pos2);
	return txt;
}

char * ScintillaEditView::getWordOnCaretPos(char * txt, int size)
{
    if (!size)
		return NULL;

    pair<int,int> range = getWordRange();
    return getWordFromRange(txt, size, range.first, range.second);
}

TCHAR * ScintillaEditView::getGenericWordOnCaretPos(TCHAR * txt, int size)
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE);
	char *txtA = new char[size + 1];
	getWordOnCaretPos(txtA, size);
 
	const TCHAR * txtW = wmc->char2wchar(txtA, cp);
	lstrcpy(txt, txtW);
	delete [] txtA;
	return txt;
}

char * ScintillaEditView::getSelectedText(char * txt, int size, bool expand)
{
	if (!size)
		return NULL;
	CharacterRange range = getSelection();
	if (range.cpMax == range.cpMin && expand)
	{
		expandWordSelection();
		range = getSelection();
	}
	if (!(size > (range.cpMax - range.cpMin)))	//there must be atleast 1 byte left for zero terminator
	{
		range.cpMax = range.cpMin+size-1;	//keep room for zero terminator
	}
	//getText(txt, range.cpMin, range.cpMax);
	return getWordFromRange(txt, size, range.cpMin, range.cpMax);
}

TCHAR * ScintillaEditView::getGenericSelectedText(TCHAR * txt, int size, bool expand)
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE);
	char *txtA = new char[size + 1];
	getSelectedText(txtA, size, expand);
 
	const TCHAR * txtW = wmc->char2wchar(txtA, cp);
	lstrcpy(txt, txtW);
	delete [] txtA;
	return txt;
}

int ScintillaEditView::searchInTarget(const TCHAR * text2Find, int lenOfText2Find, int fromPos, int toPos) const
{
	execute(SCI_SETTARGETSTART, fromPos);
	execute(SCI_SETTARGETEND, toPos);

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const char *text2FindA = wmc->wchar2char(text2Find, cp);
	size_t text2FindALen = strlen(text2FindA);
	int len = (lenOfText2Find > (int)text2FindALen)?lenOfText2Find:text2FindALen;
	int targetFound = execute(SCI_SEARCHINTARGET, (WPARAM)len, (LPARAM)text2FindA);
	return targetFound;
}

void ScintillaEditView::appandGenericText(const TCHAR * text2Append) const
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const char *text2AppendA =wmc->wchar2char(text2Append, cp);
	execute(SCI_APPENDTEXT, strlen(text2AppendA), (LPARAM)text2AppendA);
}

void ScintillaEditView::addGenericText(const TCHAR * text2Append) const
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const char *text2AppendA =wmc->wchar2char(text2Append, cp);
	execute(SCI_ADDTEXT, strlen(text2AppendA), (LPARAM)text2AppendA);
}

void ScintillaEditView::addGenericText(const TCHAR * text2Append, long *mstart, long *mend) const
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const char *text2AppendA =wmc->wchar2char(text2Append, cp, mstart, mend);
	execute(SCI_ADDTEXT, strlen(text2AppendA), (LPARAM)text2AppendA);
}

int ScintillaEditView::replaceTarget(const TCHAR * str2replace, int fromTargetPos, int toTargetPos) const
{
	if (fromTargetPos != -1 || toTargetPos != -1)
	{
		execute(SCI_SETTARGETSTART, fromTargetPos);
		execute(SCI_SETTARGETEND, toTargetPos);
	}
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const char *str2replaceA = wmc->wchar2char(str2replace, cp);
	return execute(SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)str2replaceA);
}

int ScintillaEditView::replaceTargetRegExMode(const TCHAR * re, int fromTargetPos, int toTargetPos) const
{
	if (fromTargetPos != -1 || toTargetPos != -1)
	{
		execute(SCI_SETTARGETSTART, fromTargetPos);
		execute(SCI_SETTARGETEND, toTargetPos);
	}
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE);
	const char *reA = wmc->wchar2char(re, cp);
	return execute(SCI_REPLACETARGETRE, (WPARAM)-1, (LPARAM)reA);
}

void ScintillaEditView::showAutoComletion(int lenEntered, const TCHAR * list)
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const char *listA = wmc->wchar2char(list, cp);
	execute(SCI_AUTOCSHOW, lenEntered, WPARAM(listA));
}

void ScintillaEditView::showCallTip(int startPos, const TCHAR * def)
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE); 
	const char *defA = wmc->wchar2char(def, cp);
	execute(SCI_CALLTIPSHOW, startPos, LPARAM(defA));
}

generic_string ScintillaEditView::getLine(int lineNumber)
{
	int lineLen = execute(SCI_LINELENGTH, lineNumber);
	const int bufSize = lineLen + 1;
	std::unique_ptr<TCHAR[]> buf = std::make_unique<TCHAR[]>(bufSize);
	getLine(lineNumber, buf.get(), bufSize);
	return buf.get();
}

void ScintillaEditView::getLine(int lineNumber, TCHAR * line, int lineBufferLen)
{
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	unsigned int cp = execute(SCI_GETCODEPAGE);
	char *lineA = new char[lineBufferLen];
	// From Scintilla documentation for SCI_GETLINE: "The buffer is not terminated by a 0 character."
	memset(lineA, '\0', sizeof(char) * lineBufferLen);
	execute(SCI_GETLINE, lineNumber, (LPARAM)lineA);
	const TCHAR *lineW = wmc->char2wchar(lineA, cp);
	lstrcpyn(line, lineW, lineBufferLen);
	delete [] lineA;
}

void ScintillaEditView::addText(int length, const char *buf)
{
	execute(SCI_ADDTEXT, length, (LPARAM)buf);
}

void ScintillaEditView::beginOrEndSelect()
{
	if(_beginSelectPosition == -1)
	{
		_beginSelectPosition = execute(SCI_GETCURRENTPOS);
	}
	else
	{
		execute(SCI_SETANCHOR, (WPARAM)_beginSelectPosition);
		_beginSelectPosition = -1;
	}
}

void ScintillaEditView::updateBeginEndSelectPosition(const bool is_insert, const int position, const int length)
{
	if(_beginSelectPosition != -1 && position < _beginSelectPosition - 1)
	{
		if(is_insert)
			_beginSelectPosition += length;
		else
			_beginSelectPosition -= length;

		assert(_beginSelectPosition >= 0);
	}
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
			if (isFolded(lineClick)) 
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
			bool mode = isFolded(lineClick);
			fold(lineClick, !mode);
			runMarkers(true, lineClick, true, false);
		}
	}
}

void ScintillaEditView::expand(int &line, bool doExpand, bool force, int visLevels, int level)
{
	int lineMaxSubord = int(execute(SCI_GETLASTCHILD, line, level & SC_FOLDLEVELNUMBERMASK));
	++line;
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
					if (!isFolded(line))
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
			++line;
		}
	}

	runMarkers(true, 0, true, false);
}

void ScintillaEditView::performGlobalStyles() 
{
	StyleArray & stylers = _pParameter->getMiscStylerArray();

	int i = stylers.getStylerIndexByName(TEXT("Current line background colour"));
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		execute(SCI_SETCARETLINEBACK, style._bgColor);
	}

    COLORREF selectColorBack = grey;

	i = stylers.getStylerIndexByName(TEXT("Selected text colour"));
	if (i != -1)
    {
        Style & style = stylers.getStyler(i);
		selectColorBack = style._bgColor;
    }
	execute(SCI_SETSELBACK, 1, selectColorBack);

    COLORREF caretColor = black;
	i = stylers.getStylerIndexByID(SCI_SETCARETFORE);
	if (i != -1)
    {
        Style & style = stylers.getStyler(i);
        caretColor = style._fgColor;
    }
    execute(SCI_SETCARETFORE, caretColor);

	COLORREF edgeColor = liteGrey;
	i = stylers.getStylerIndexByName(TEXT("Edge colour"));
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		edgeColor = style._fgColor;
	}
	execute(SCI_SETEDGECOLOUR, edgeColor);

	COLORREF foldMarginColor = grey;
	COLORREF foldMarginHiColor = white;
	i = stylers.getStylerIndexByName(TEXT("Fold margin"));
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
	i = stylers.getStylerIndexByName(TEXT("Fold"));
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		foldfgColor = style._bgColor;
		foldbgColor = style._fgColor;
	}

	COLORREF activeFoldFgColor = red;
	i = stylers.getStylerIndexByName(TEXT("Fold active"));
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		activeFoldFgColor = style._fgColor;
	}

	ScintillaViewParams & svp = (ScintillaViewParams &)_pParameter->getSVP();
	for (int j = 0 ; j < NB_FOLDER_STATE ; ++j)
		defineMarker(_markersArray[FOLDER_TYPE][j], _markersArray[svp._folderStyle][j], foldfgColor, foldbgColor, activeFoldFgColor);

	execute(SCI_MARKERENABLEHIGHLIGHT, true);

	COLORREF wsSymbolFgColor = black;
	i = stylers.getStylerIndexByName(TEXT("White space symbol"));
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

void ScintillaEditView::updateLineNumberWidth() 
{
	if (_lineNumbersShown)
	{
		int linesVisible = (int) execute(SCI_LINESONSCREEN);
		if (linesVisible)
		{
			int firstVisibleLineVis = (int) execute(SCI_GETFIRSTVISIBLELINE);
			int lastVisibleLineVis = linesVisible + firstVisibleLineVis + 1;

			if (execute(SCI_GETWRAPMODE) != SC_WRAP_NONE)
			{
				int numLinesDoc = (int) execute(SCI_GETLINECOUNT);
				int prevLineDoc = (int) execute(SCI_DOCLINEFROMVISIBLE, firstVisibleLineVis);
				for (int i = firstVisibleLineVis + 1; i <= lastVisibleLineVis; ++i)
				{
					int lineDoc = (int) execute(SCI_DOCLINEFROMVISIBLE, i);
					if (lineDoc == numLinesDoc)
						break;
					if (lineDoc == prevLineDoc)
						lastVisibleLineVis++;
					prevLineDoc = lineDoc;
				}
			}

			int lastVisibleLineDoc = (int) execute(SCI_DOCLINEFROMVISIBLE, lastVisibleLineVis);
			int i = 0;

			while (lastVisibleLineDoc)
			{
				lastVisibleLineDoc /= 10;
				++i;
			}

			i = max(i, 3);
			int pixelWidth = int(8 + i * execute(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"8"));
			execute(SCI_SETMARGINWIDTHN, _SC_MARGE_LINENUMBER, pixelWidth);
		}
	}
}

const char * ScintillaEditView::getCompleteKeywordList(std::basic_string<char> & kwl, LangType langType, int keywordIndex)
{
	kwl += " ";
	const TCHAR *defKwl_generic = _pParameter->getWordList(langType, keywordIndex);
	
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	const char * defKwl = wmc->wchar2char(defKwl_generic, CP_ACP);
	kwl += defKwl?defKwl:"";

	return kwl.c_str();
}

void ScintillaEditView::setMultiSelections(const ColumnModeInfos & cmi)
{
	for (size_t i = 0, len = cmi.size(); i < len ; ++i)
	{
		if (cmi[i].isValid())
		{
			int selStart = cmi[i]._direction == L2R?cmi[i]._selLpos:cmi[i]._selRpos;
			int selEnd   = cmi[i]._direction == L2R?cmi[i]._selRpos:cmi[i]._selLpos;
			execute(SCI_SETSELECTIONNSTART, i, selStart);
			execute(SCI_SETSELECTIONNEND, i, selEnd);
		}
		//if (cmi[i].hasVirtualSpace())
		//{
		if (cmi[i]._nbVirtualAnchorSpc)
			execute(SCI_SETSELECTIONNANCHORVIRTUALSPACE, i, cmi[i]._nbVirtualAnchorSpc);
		if (cmi[i]._nbVirtualCaretSpc)
			execute(SCI_SETSELECTIONNCARETVIRTUALSPACE, i, cmi[i]._nbVirtualCaretSpc);
		//}
	}
}

void ScintillaEditView::currentLineUp() const 
{
	int currentLine = getCurrentLineNumber();
	if (currentLine != 0)
	{
		execute(SCI_BEGINUNDOACTION);
		currentLine--;
		execute(SCI_LINETRANSPOSE);
		execute(SCI_GOTOLINE, currentLine);
		execute(SCI_ENDUNDOACTION);
	}
}

void ScintillaEditView::currentLineDown() const 
{
	int currentLine = getCurrentLineNumber();
	if (currentLine != (execute(SCI_GETLINECOUNT) - 1))
	{
		execute(SCI_BEGINUNDOACTION);
		++currentLine;
		execute(SCI_GOTOLINE, currentLine);
		execute(SCI_LINETRANSPOSE);
		execute(SCI_ENDUNDOACTION);
	}
}

// Get selection range : (fromLine, toLine) 
// return (-1, -1) if multi-selection
pair<int, int> ScintillaEditView::getSelectionLinesRange() const
{
    pair<int, int> range(-1, -1);
    if (execute(SCI_GETSELECTIONS) > 1) // multi-selection
        return range;
    int start = execute(SCI_GETSELECTIONSTART);
    int end = execute(SCI_GETSELECTIONEND);

    range.first = execute(SCI_LINEFROMPOSITION, start);
    range.second = execute(SCI_LINEFROMPOSITION, end);
    if (range.first > range.second)
	{
		int temp = range.first;
		range.first = range.second;
		range.second = temp;
	}
    return range;
}

void ScintillaEditView::currentLinesUp() const 
{
	pair<int, int> lineRange = getSelectionLinesRange();
    if ((lineRange.first == -1 || lineRange.first == 0))
        return;

	bool noSel = lineRange.first == lineRange.second;
    int nbSelLines = lineRange.second - lineRange.first + 1;

    int line2swap = lineRange.first - 1;
    int nbChar = execute(SCI_LINELENGTH, line2swap);

    int posStart = execute(SCI_POSITIONFROMLINE, lineRange.first);
    int posEnd = execute(SCI_GETLINEENDPOSITION, lineRange.second);

    execute(SCI_BEGINUNDOACTION);
    execute(SCI_GOTOLINE, line2swap);

    for (int i = 0 ; i < nbSelLines ; ++i)
    {
        currentLineDown();
    }
	execute(SCI_ENDUNDOACTION);

    execute(SCI_SETSELECTIONSTART, posStart - nbChar);
	execute(SCI_SETSELECTIONEND, noSel?posStart - nbChar:posEnd - nbChar);
}

void ScintillaEditView::currentLinesDown() const 
{
	pair<int, int> lineRange = getSelectionLinesRange();
	
	if ((lineRange.first == -1 || lineRange.second >= execute(SCI_LINEFROMPOSITION, getCurrentDocLen())))
        return;

	bool noSel = lineRange.first == lineRange.second;
    int nbSelLines = lineRange.second - lineRange.first + 1;

	int line2swap = lineRange.second + 1;
    int nbChar = execute(SCI_LINELENGTH, line2swap);

	if ((line2swap + 1) == execute(SCI_GETLINECOUNT))
		nbChar += (execute(SCI_GETEOLMODE)==SC_EOL_CRLF?2:1);

	int posStart = execute(SCI_POSITIONFROMLINE, lineRange.first);
    int posEnd = execute(SCI_GETLINEENDPOSITION, lineRange.second);

    execute(SCI_BEGINUNDOACTION);
    execute(SCI_GOTOLINE, line2swap);

    for (int i = 0 ; i < nbSelLines ; ++i)
    {
        currentLineUp();
    }
	execute(SCI_ENDUNDOACTION);

	execute(SCI_SETSELECTIONSTART, posStart + nbChar);
	execute(SCI_SETSELECTIONEND, noSel?posStart + nbChar:posEnd + nbChar);

}

void ScintillaEditView::convertSelectedTextTo(bool Case)
{
	unsigned int codepage = _codepage;
	UniMode um = getCurrentBuffer()->getUnicodeMode();
	if (um != uni8Bit)
	codepage = CP_UTF8;

	if (execute(SCI_GETSELECTIONS) > 1) // Multi-Selection || Column mode
	{
        execute(SCI_BEGINUNDOACTION);
  
		ColumnModeInfos cmi = getColumnModeSelectInfo();

		for (size_t i = 0, cmiLen = cmi.size(); i < cmiLen ; ++i)
		{
			const int len = cmi[i]._selRpos - cmi[i]._selLpos;
			char *srcStr = new char[len+1];
			wchar_t *destStr = new wchar_t[len+1];

			int start = cmi[i]._selLpos;
			int end = cmi[i]._selRpos;
			getText(srcStr, start, end);

			int nbChar = ::MultiByteToWideChar(codepage, 0, srcStr, len, destStr, len);

			for (int j = 0 ; j < nbChar ; ++j)
			{
				if (Case == UPPERCASE)
					destStr[j] = (wchar_t)(UINT_PTR)::CharUpperW((LPWSTR)destStr[j]);
				else
					destStr[j] = (wchar_t)(UINT_PTR)::CharLowerW((LPWSTR)destStr[j]);
			}
			::WideCharToMultiByte(codepage, 0, destStr, len, srcStr, len, NULL, NULL);

			execute(SCI_SETTARGETSTART, start);
			execute(SCI_SETTARGETEND, end);
			execute(SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)srcStr);

			delete [] srcStr;
			delete [] destStr;
		}

		setMultiSelections(cmi);

		//execute(SCI_SETSELECTIONSTART, selStart);
		//execute(SCI_SETSELECTIONEND, selEnd);

		execute(SCI_ENDUNDOACTION);
		return;
	}

	size_t selectionStart = execute(SCI_GETSELECTIONSTART);
	size_t selectionEnd = execute(SCI_GETSELECTIONEND);
   
	int strSize = ((selectionEnd > selectionStart)?(selectionEnd - selectionStart):(selectionStart - selectionEnd))+1;
	if (strSize)
	{
		char *selectedStr = new char[strSize+1];
		int strWSize = strSize * 2;
		wchar_t *selectedStrW = new wchar_t[strWSize+3];

		execute(SCI_GETSELTEXT, 0, (LPARAM)selectedStr);

		int nbChar = ::MultiByteToWideChar(codepage, 0, selectedStr, strSize, selectedStrW, strWSize);

		for (int i = 0 ; i < nbChar ; ++i)
		{
			if (Case == UPPERCASE)
				selectedStrW[i] = (WCHAR)(UINT_PTR)::CharUpperW((LPWSTR)selectedStrW[i]);
			else
				selectedStrW[i] = (WCHAR)(UINT_PTR)::CharLowerW((LPWSTR)selectedStrW[i]);
		}
		::WideCharToMultiByte(codepage, 0, selectedStrW, strWSize, selectedStr, strSize, NULL, NULL);

		execute(SCI_SETTARGETSTART, selectionStart);
		execute(SCI_SETTARGETEND, selectionEnd);
		execute(SCI_REPLACETARGET, strSize - 1, (LPARAM)selectedStr);
		execute(SCI_SETSEL, selectionStart, selectionEnd);
		delete [] selectedStr;
		delete [] selectedStrW;
	}
}



pair<int, int> ScintillaEditView::getWordRange()
{
    int caretPos = execute(SCI_GETCURRENTPOS, 0, 0);
	int startPos = static_cast<int>(execute(SCI_WORDSTARTPOSITION, caretPos, true));
	int endPos = static_cast<int>(execute(SCI_WORDENDPOSITION, caretPos, true));
    return pair<int, int>(startPos, endPos);
}

bool ScintillaEditView::expandWordSelection()
{
    pair<int, int> wordRange = 	getWordRange();
    if (wordRange.first != wordRange.second) {
        execute(SCI_SETSELECTIONSTART, wordRange.first);
        execute(SCI_SETSELECTIONEND, wordRange.second);
		return true;
	}
	return false;
}

TCHAR * int2str(TCHAR *str, int strLen, int number, int base, int nbChiffre, bool isZeroLeading) 
{
	if (nbChiffre >= strLen) return NULL;
	TCHAR f[64];
	TCHAR fStr[2] = TEXT("d");
	if (base == 16)
		fStr[0] = 'X';
	else if (base == 8)
		fStr[0] = 'o';
	else if (base == 2)
	{
		const unsigned int MASK_ULONG_BITFORT = 0x80000000;
		int nbBits = sizeof(unsigned int) * 8;
		int nbBit2Shift = (nbChiffre >= nbBits)?nbBits:(nbBits - nbChiffre);
		unsigned long mask = MASK_ULONG_BITFORT >> nbBit2Shift;
		int i = 0; 
		for (; mask > 0 ; ++i)
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
			TCHAR *j = str;
			for ( ; *j != '\0' ; ++j)
				if (*j == '1')
					break;
			lstrcpy(str, j);
		}
		else
		{
			// use sprintf or swprintf instead of wsprintf
			// to make octal format work
			generic_sprintf(f, TEXT("%%%s"), fStr);
			generic_sprintf(str, f, number);
		}
		int i = lstrlen(str);
		for ( ; i < nbChiffre ; ++i)
			str[i] = ' ';
		str[i] = '\0';
	}
	else
	{
		if (base != 2)
		{
			// use sprintf or swprintf instead of wsprintf
			// to make octal format work
			generic_sprintf(f, TEXT("%%.%d%s"), nbChiffre, fStr);
			generic_sprintf(str, f, number);
		}
		// else already done.
	}
	return str;
}

ColumnModeInfos ScintillaEditView::getColumnModeSelectInfo()
{
	ColumnModeInfos columnModeInfos;
	if (execute(SCI_GETSELECTIONS) > 1) // Multi-Selection || Column mode
	{
		int nbSel = execute(SCI_GETSELECTIONS);

		for (int i = 0 ; i < nbSel ; ++i)
		{
			int absPosSelStartPerLine = execute(SCI_GETSELECTIONNANCHOR, i);
			int absPosSelEndPerLine = execute(SCI_GETSELECTIONNCARET, i);
			int nbVirtualAnchorSpc = execute(SCI_GETSELECTIONNANCHORVIRTUALSPACE, i);
			int nbVirtualCaretSpc = execute(SCI_GETSELECTIONNCARETVIRTUALSPACE, i);
			
			if (absPosSelStartPerLine == absPosSelEndPerLine && execute(SCI_SELECTIONISRECTANGLE))
			{
				bool dir = nbVirtualAnchorSpc<nbVirtualCaretSpc?L2R:R2L;
				columnModeInfos.push_back(ColumnModeInfo(absPosSelStartPerLine, absPosSelEndPerLine, i, dir, nbVirtualAnchorSpc, nbVirtualCaretSpc));
			}
			else if (absPosSelStartPerLine > absPosSelEndPerLine)
				columnModeInfos.push_back(ColumnModeInfo(absPosSelEndPerLine, absPosSelStartPerLine, i, R2L, nbVirtualAnchorSpc, nbVirtualCaretSpc));
			else
				columnModeInfos.push_back(ColumnModeInfo(absPosSelStartPerLine, absPosSelEndPerLine, i, L2R, nbVirtualAnchorSpc, nbVirtualCaretSpc));
		}
	}
	return columnModeInfos;
}

void ScintillaEditView::columnReplace(ColumnModeInfos & cmi, const TCHAR *str)
{
	int totalDiff = 0;
	for (size_t i = 0, len = cmi.size(); i < len ; ++i)
	{
		if (cmi[i].isValid())
		{
			int len2beReplace = cmi[i]._selRpos - cmi[i]._selLpos;
			int diff = lstrlen(str) - len2beReplace;

			cmi[i]._selLpos += totalDiff;
			cmi[i]._selRpos += totalDiff;
			bool hasVirtualSpc = cmi[i]._nbVirtualAnchorSpc > 0;

			if (hasVirtualSpc) // if virtual space is present, then insert space
			{
				for (int j = 0, k = cmi[i]._selLpos; j < cmi[i]._nbVirtualCaretSpc ; ++j, ++k)
				{
					execute(SCI_INSERTTEXT, k, (LPARAM)" ");
				}
				cmi[i]._selLpos += cmi[i]._nbVirtualAnchorSpc;
				cmi[i]._selRpos += cmi[i]._nbVirtualCaretSpc;
			}

			execute(SCI_SETTARGETSTART, cmi[i]._selLpos);
			execute(SCI_SETTARGETEND, cmi[i]._selRpos);

			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			unsigned int cp = execute(SCI_GETCODEPAGE);
			const char *strA = wmc->wchar2char(str, cp);
			execute(SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)strA);
			
			if (hasVirtualSpc) 
			{
				totalDiff += cmi[i]._nbVirtualAnchorSpc + lstrlen(str);

				// Now there's no more virtual space
				cmi[i]._nbVirtualAnchorSpc = 0;
				cmi[i]._nbVirtualCaretSpc = 0;
			}
			else
			{
				totalDiff += diff;
			}
			cmi[i]._selRpos += diff;
		}
	}
}

void ScintillaEditView::columnReplace(ColumnModeInfos & cmi, int initial, int incr, int repeat, UCHAR format)
{
	assert(repeat > 0);

	// 0000 00 00 : Dec BASE_10
	// 0000 00 01 : Hex BASE_16
	// 0000 00 10 : Oct BASE_08
	// 0000 00 11 : Bin BASE_02

	// 0000 01 00 : 0 leading

	//Defined in ScintillaEditView.h :
	//const UCHAR MASK_FORMAT = 0x03;
	//const UCHAR MASK_ZERO_LEADING = 0x04;

	UCHAR f = format & MASK_FORMAT;
	bool isZeroLeading = (MASK_ZERO_LEADING & format) != 0;
	
	int base = 10;
	if (f == BASE_16)
		base = 16;
	else if (f == BASE_08)
		base = 8;
	else if (f == BASE_02)
		base = 2;

	const int stringSize = 512;
	TCHAR str[stringSize];

	// Compute the numbers to be placed at each column.
	std::vector<int> numbers;
	{
		int curNumber = initial;
		const unsigned int kiMaxSize = cmi.size();
		while(numbers.size() < kiMaxSize)
		{
			for(int i = 0; i < repeat; i++)
			{
				numbers.push_back(curNumber);
				if (numbers.size() >= kiMaxSize)
				{
					break;
				}
			}
			curNumber += incr;
		}
	}

	assert(numbers.size()> 0);

	const int kibEnd = getNbDigits(*numbers.rbegin(), base);
	const int kibInit = getNbDigits(initial, base);
	const int kib = std::max<int>(kibInit, kibEnd);

	int totalDiff = 0;
	const size_t len = cmi.size();
	for (size_t i = 0 ; i < len ; i++)
	{
		if (cmi[i].isValid())
		{
			const int len2beReplaced = cmi[i]._selRpos - cmi[i]._selLpos;
			const int diff = kib - len2beReplaced;

			cmi[i]._selLpos += totalDiff;
			cmi[i]._selRpos += totalDiff;

			int2str(str, stringSize, numbers.at(i), base, kib, isZeroLeading);

			const bool hasVirtualSpc = cmi[i]._nbVirtualAnchorSpc > 0;
			if (hasVirtualSpc) // if virtual space is present, then insert space
			{
				for (int j = 0, k = cmi[i]._selLpos; j < cmi[i]._nbVirtualCaretSpc ; ++j, ++k)
				{
					execute(SCI_INSERTTEXT, k, (LPARAM)" ");
				}
				cmi[i]._selLpos += cmi[i]._nbVirtualAnchorSpc;
				cmi[i]._selRpos += cmi[i]._nbVirtualCaretSpc;
			}
			execute(SCI_SETTARGETSTART, cmi[i]._selLpos);
			execute(SCI_SETTARGETEND, cmi[i]._selRpos);

			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			unsigned int cp = execute(SCI_GETCODEPAGE);
			const char *strA = wmc->wchar2char(str, cp);
			execute(SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)strA);

			if (hasVirtualSpc) 
			{
				totalDiff += cmi[i]._nbVirtualAnchorSpc + lstrlen(str);
				// Now there's no more virtual space
				cmi[i]._nbVirtualAnchorSpc = 0;
				cmi[i]._nbVirtualCaretSpc = 0;
			}
			else
			{
				totalDiff += diff;
			}
			cmi[i]._selRpos += diff;
		}
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
		if (isFolded(line))
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
		if ((parentLine < 0) || !isFolded(parentLine && execute(SCI_GETLINEVISIBLE, parentLine)))
			execute(SCI_SHOWLINES, line, line);
	}
}


void ScintillaEditView::scrollPosToCenter(int pos)
{
	execute(SCI_GOTOPOS, pos);
	int line = execute(SCI_LINEFROMPOSITION, pos);

	int firstVisibleDisplayLine = execute(SCI_GETFIRSTVISIBLELINE);
	int firstVisibleDocLine = execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine);
	int nbLine = execute(SCI_LINESONSCREEN, firstVisibleDisplayLine);
	int lastVisibleDocLine = execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine + nbLine);

	int middleLine;
	if (line - firstVisibleDocLine < lastVisibleDocLine - line)
		middleLine = firstVisibleDocLine + nbLine/2;
	else
		middleLine = lastVisibleDocLine -  nbLine/2;
	int nbLines2scroll =  line - middleLine;
	scroll(0, nbLines2scroll);
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
		++startLine;
	if (endLine == (nrLines-1))
		--endLine;

	if (startLine > endLine)
		return;	//tried to hide line at edge

	//Hide the lines. We add marks on the outside of the hidden section and hide the lines
	//execute(SCI_HIDELINES, startLine, endLine);
	//Add markers
	execute(SCI_MARKERADD, startLine-1, MARK_HIDELINESBEGIN);
	execute(SCI_MARKERADD, endLine+1, MARK_HIDELINESEND);

	//remove any markers in between
	int scope = 0;
	for(int i = startLine; i <= endLine; ++i) {
		int state = execute(SCI_MARKERGET, i);
		bool closePresent = ((state & (1 << MARK_HIDELINESEND)) != 0);	//check close first, then open, since close closes scope
		bool openPresent = ((state & (1 << MARK_HIDELINESBEGIN)) != 0);
		if (closePresent) {
			execute(SCI_MARKERDELETE, i, MARK_HIDELINESEND);
			if (scope > 0) scope--;
		}
		if (openPresent) {
			execute(SCI_MARKERDELETE, i, MARK_HIDELINESBEGIN);
			++scope;
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
		for(int i = searchStart; i < maxLines; ++i) {
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
		for(int i = searchStart; i < maxLines; ++i) {
			int state = execute(SCI_MARKERGET, i);
			if ( ((state & (1 << MARK_HIDELINESEND)) != 0) ) {
				if (doDelete)
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESEND);
				 else if (isInSection) {
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
					isInSection = false;
				}
			}
			if ( ((state & (1 << MARK_HIDELINESBEGIN)) != 0) ) {
				if (doDelete)
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESBEGIN);
				else {
					isInSection = true;
					startShowing = i+1;
				}
			}

			int levelLine = execute(SCI_GETFOLDLEVEL, i, 0);
			if (levelLine & SC_FOLDLEVELHEADERFLAG) {	//fold section. Dont show lines if fold is closed
				if (isInSection && !isFolded(i)) {
					execute(SCI_SHOWLINES, startShowing, i);
					startShowing = execute(SCI_GETLASTCHILD, i, (levelLine & SC_FOLDLEVELNUMBERMASK));
				}
			}
		}
	}
}


void ScintillaEditView::setTabSettings(Lang *lang)
{
    if (lang && lang->_tabSize != -1 && lang->_tabSize != 0)
    {
        execute(SCI_SETTABWIDTH, lang->_tabSize);
        execute(SCI_SETUSETABS, !lang->_isTabReplacedBySpace);
    }
    else
    {
        const NppGUI & nppgui = _pParameter->getNppGUI();
        execute(SCI_SETTABWIDTH, nppgui._tabSize);
		execute(SCI_SETUSETABS, !nppgui._tabReplacedBySpace);
    }
}

void ScintillaEditView::insertNewLineAboveCurrentLine()
{
	generic_string newline = getEOLString();
	const int current_line = getCurrentLineNumber();
	if(current_line == 0)
	{
		// Special handling if caret is at first line.
		insertGenericTextFrom(0, newline.c_str());
	}
	else
	{
		const int eol_length = newline.length();
		const long position = execute(SCI_POSITIONFROMLINE, current_line) - eol_length;
		insertGenericTextFrom(position, newline.c_str());
	}
	execute(SCI_SETEMPTYSELECTION, execute(SCI_POSITIONFROMLINE, current_line));
}


void ScintillaEditView::insertNewLineBelowCurrentLine()
{
	generic_string newline = getEOLString();
	const int line_count = execute(SCI_GETLINECOUNT);
	const int current_line = getCurrentLineNumber();
	if(current_line == line_count - 1)
	{
		// Special handling if caret is at last line.
		appandGenericText(newline.c_str());
	}
	else
	{
		const int eol_length = newline.length();
		const long position = eol_length + execute(SCI_GETLINEENDPOSITION, current_line);
		insertGenericTextFrom(position, newline.c_str());
	}
	execute(SCI_SETEMPTYSELECTION, execute(SCI_POSITIONFROMLINE, current_line + 1));
}

void ScintillaEditView::sortLines(size_t fromLine, size_t toLine, ISorter *pSort)
{
	if (fromLine >= toLine)
	{
		return;
	}

	const int startPos = execute(SCI_POSITIONFROMLINE, fromLine);
	const int endPos = execute(SCI_POSITIONFROMLINE, toLine) + execute(SCI_LINELENGTH, toLine);
	const generic_string text = getGenericTextAsString(startPos, endPos);
	std::vector<generic_string> splitText = stringSplit(text, getEOLString());
	const size_t lineCount = execute(SCI_GETLINECOUNT);
	const bool sortEntireDocument = toLine == lineCount - 1;
	if (!sortEntireDocument)
	{
		if (splitText.rbegin()->empty())
		{
			splitText.pop_back();
		}
	}
	assert(toLine - fromLine + 1 == splitText.size());
	const std::vector<generic_string> sortedText = pSort->sort(splitText);
	const generic_string joined = stringJoin(sortedText, getEOLString());
	if (sortEntireDocument)
	{
		assert(joined.length() == text.length());
		replaceTarget(joined.c_str(), startPos, endPos);
	}
	else
	{
		assert(joined.length() + getEOLString().length() == text.length());
		replaceTarget((joined + getEOLString()).c_str(), startPos, endPos);
	}
}

bool ScintillaEditView::isTextDirectionRTL() const
{
	long exStyle = ::GetWindowLongPtr(_hSelf, GWL_EXSTYLE);
	return (exStyle & WS_EX_LAYOUTRTL) != 0;
}

void ScintillaEditView::changeTextDirection(bool isRTL)
{
	long exStyle = ::GetWindowLongPtr(_hSelf, GWL_EXSTYLE);
	exStyle = isRTL ? exStyle | WS_EX_LAYOUTRTL : exStyle&(~WS_EX_LAYOUTRTL);
	::SetWindowLongPtr(_hSelf, GWL_EXSTYLE, exStyle);
}

generic_string ScintillaEditView::getEOLString()
{
	const int eol_mode = int(execute(SCI_GETEOLMODE));
	string newline;
	if (eol_mode == SC_EOL_CRLF)
	{
		return TEXT("\r\n");
	}
	else if (eol_mode == SC_EOL_LF)
	{
		return TEXT("\n");
	}
	else
	{
		return TEXT("\r");
	}
}