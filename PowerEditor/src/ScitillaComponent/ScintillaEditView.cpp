// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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

#include <memory>
#include <shlwapi.h>
#include <cinttypes>
#include <windowsx.h>
#include "ScintillaEditView.h"
#include "Parameters.h"
#include "Sorters.h"
#include "verifySignedfile.h"

using namespace std;

// initialize the static variable

// get full ScinLexer.dll path to avoid hijack
TCHAR * getSciLexerFullPathName(TCHAR * moduleFileName, size_t len)
{
	::GetModuleFileName(NULL, moduleFileName, static_cast<int32_t>(len));
	::PathRemoveFileSpec(moduleFileName);
	::PathAppend(moduleFileName, TEXT("SciLexer.dll"));
	return moduleFileName;
};

HINSTANCE ScintillaEditView::_hLib = loadSciLexerDll();
int ScintillaEditView::_refCount = 0;
UserDefineDialog ScintillaEditView::_userDefineDlg;

const int ScintillaEditView::_SC_MARGE_LINENUMBER = 0;
const int ScintillaEditView::_SC_MARGE_SYBOLE = 1;
const int ScintillaEditView::_SC_MARGE_FOLDER = 2;

WNDPROC ScintillaEditView::_scintillaDefaultProc = NULL;
string ScintillaEditView::_defaultCharList = "";

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

// Array with all the names of all languages
// The order of lang type (enum LangType) must be respected
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
{TEXT("udf"),			TEXT("udf"),				TEXT("User Defined language file"),						L_USER,			SCLEX_USER},
{TEXT("asp"),			TEXT("ASP"),				TEXT("Active Server Pages script file"),				L_ASP,			SCLEX_HTML},
{TEXT("sql"),			TEXT("SQL"),				TEXT("Structured Query Language file"),					L_SQL,			SCLEX_SQL},
{TEXT("vb"),			TEXT("Visual Basic"),		TEXT("Visual Basic file"),								L_VB,			SCLEX_VB},
{TEXT("javascript"),	TEXT("JavaScript"),			TEXT("JavaScript file"),								L_JS,			SCLEX_CPP},
{TEXT("css"),			TEXT("CSS"),				TEXT("Cascade Style Sheets File"),						L_CSS,			SCLEX_CSS},
{TEXT("perl"),			TEXT("Perl"),				TEXT("Perl source file"),								L_PERL,			SCLEX_PERL},
{TEXT("python"),		TEXT("Python"),				TEXT("Python file"),									L_PYTHON,		SCLEX_PYTHON},
{TEXT("lua"),			TEXT("Lua"),				TEXT("Lua source File"),								L_LUA,			SCLEX_LUA},
{TEXT("tex"),			TEXT("TeX"),				TEXT("TeX file"),										L_TEX,			SCLEX_TEX},
{TEXT("fortran"),		TEXT("Fortran free form"),	TEXT("Fortran free form source file"),					L_FORTRAN,		SCLEX_FORTRAN},
{TEXT("bash"),			TEXT("Shell"),				TEXT("Unix script file"),								L_BASH,			SCLEX_BASH},
{TEXT("actionscript"),	TEXT("ActionScript"),		TEXT("Flash ActionScript file"),						L_FLASH,		SCLEX_CPP},
{TEXT("nsis"),			TEXT("NSIS"),				TEXT("Nullsoft Scriptable Install System script file"),	L_NSIS,			SCLEX_NSIS},
{TEXT("tcl"),			TEXT("TCL"),				TEXT("Tool Command Language file"),						L_TCL,			SCLEX_TCL},
{TEXT("lisp"),			TEXT("Lisp"),				TEXT("List Processing language file"),					L_LISP,			SCLEX_LISP},
{TEXT("scheme"),		TEXT("Scheme"),				TEXT("Scheme file"),									L_SCHEME,		SCLEX_LISP},
{TEXT("asm"),			TEXT("Assembly"),			TEXT("Assembly language source file"),					L_ASM,			SCLEX_ASM},
{TEXT("diff"),			TEXT("Diff"),				TEXT("Diff file"),										L_DIFF,			SCLEX_DIFF},
{TEXT("props"),			TEXT("Properties file"),	TEXT("Properties file"),								L_PROPS,		SCLEX_PROPERTIES},
{TEXT("postscript"),	TEXT("PostScript"),			TEXT("PostScript file"),								L_PS,			SCLEX_PS},
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
{TEXT("inno"),			TEXT("Inno Setup"),			TEXT("Inno Setup script"),								L_INNO,			SCLEX_INNOSETUP},
{TEXT("searchResult"),	TEXT("Internal Search"),	TEXT("Internal Search"),								L_SEARCHRESULT,	SCLEX_SEARCHRESULT},
{TEXT("cmake"),			TEXT("CMake"),				TEXT("CMake file"),										L_CMAKE,		SCLEX_CMAKE},
{TEXT("yaml"),			TEXT("YAML"),				TEXT("YAML Ain't Markup Language"),						L_YAML,			SCLEX_YAML},
{TEXT("cobol"),			TEXT("COBOL"),				TEXT("COmmon Business Oriented Language"),				L_COBOL,		SCLEX_COBOL},
{TEXT("gui4cli"),		TEXT("Gui4Cli"),			TEXT("Gui4Cli file"),									L_GUI4CLI,		SCLEX_GUI4CLI},
{TEXT("d"),				TEXT("D"),					TEXT("D programming language"),							L_D,			SCLEX_D},
{TEXT("powershell"),	TEXT("PowerShell"),			TEXT("Windows PowerShell"),								L_POWERSHELL,	SCLEX_POWERSHELL},
{TEXT("r"),				TEXT("R"),					TEXT("R programming language"),							L_R,			SCLEX_R},
{TEXT("jsp"),			TEXT("JSP"),				TEXT("JavaServer Pages script file"),					L_JSP,			SCLEX_HTML},
{TEXT("coffeescript"),	TEXT("CoffeeScript"),		TEXT("CoffeeScript file"),								L_COFFEESCRIPT,	SCLEX_COFFEESCRIPT},
{TEXT("json"),			TEXT("json"),				TEXT("JSON file"),										L_JSON,			SCLEX_JSON },
{TEXT("javascript.js"), TEXT("JavaScript"),			TEXT("JavaScript file"),								L_JAVASCRIPT,	SCLEX_CPP },
{TEXT("fortran77"),		TEXT("Fortran fixed form"),	TEXT("Fortran fixed form source file"),					L_FORTRAN_77,	SCLEX_F77},
{TEXT("baanc"),			TEXT("BaanC"),				TEXT("BaanC File"),										L_BAANC,		SCLEX_BAAN },
{TEXT("srec"),			TEXT("S-Record"),			TEXT("Motorola S-Record binary data"),					L_SREC,			SCLEX_SREC},
{TEXT("ihex"),			TEXT("Intel HEX"),			TEXT("Intel HEX binary data"),							L_IHEX,			SCLEX_IHEX},
{TEXT("tehex"),			TEXT("Tektronix extended HEX"),	TEXT("Tektronix extended HEX binary data"),			L_TEHEX,		SCLEX_TEHEX},
{TEXT("swift"),			TEXT("Swift"),              TEXT("Swift file"),										L_SWIFT,		SCLEX_CPP},
{TEXT("asn1"),			TEXT("ASN.1"),				TEXT("Abstract Syntax Notation One file"),				L_ASN1,			SCLEX_ASN1},
{TEXT("avs"),			TEXT("AviSynth"),			TEXT("AviSynth scripts files"),							L_AVS,			SCLEX_AVS},
{TEXT("blitzbasic"),	TEXT("BlitzBasic"),			TEXT("BlitzBasic file"),								L_BLITZBASIC,	SCLEX_BLITZBASIC},
{TEXT("purebasic"),		TEXT("PureBasic"),			TEXT("PureBasic file"),									L_PUREBASIC,	SCLEX_PUREBASIC},
{TEXT("freebasic"),		TEXT("FreeBasic"),			TEXT("FreeBasic file"),									L_FREEBASIC,	SCLEX_FREEBASIC},
{TEXT("csound"),		TEXT("Csound"),				TEXT("Csound file"),									L_CSOUND,		SCLEX_CSOUND},
{TEXT("erlang"),		TEXT("Erlang"),				TEXT("Erlang file"),									L_ERLANG,		SCLEX_ERLANG},
{TEXT("escript"),		TEXT("ESCRIPT"),			TEXT("ESCRIPT file"),									L_ESCRIPT,		SCLEX_ESCRIPT},
{TEXT("forth"),			TEXT("Forth"),				TEXT("Forth file"),										L_FORTH,		SCLEX_FORTH},
{TEXT("latex"),			TEXT("LaTeX"),				TEXT("LaTeX file"),										L_LATEX,		SCLEX_LATEX},
{TEXT("mmixal"),		TEXT("MMIXAL"),				TEXT("MMIXAL file"),									L_MMIXAL,		SCLEX_MMIXAL},
{TEXT("nimrod"),		TEXT("Nimrod"),				TEXT("Nimrod file"),									L_NIMROD,		SCLEX_NIMROD},
{TEXT("nncrontab"),		TEXT("Nncrontab"),			TEXT("extended crontab file"),							L_NNCRONTAB,	SCLEX_NNCRONTAB},
{TEXT("oscript"),		TEXT("OScript"),			TEXT("OScript source file"),							L_OSCRIPT,		SCLEX_OSCRIPT},
{TEXT("rebol"),			TEXT("REBOL"),				TEXT("REBOL file"),										L_REBOL,		SCLEX_REBOL},
{TEXT("registry"),		TEXT("registry"),			TEXT("registry file"),									L_REGISTRY,		SCLEX_REGISTRY},
{TEXT("rust"),			TEXT("Rust"),				TEXT("Rust file"),										L_RUST,			SCLEX_RUST},
{TEXT("spice"),			TEXT("Spice"),				TEXT("spice file"),										L_SPICE,		SCLEX_SPICE},
{TEXT("txt2tags"),		TEXT("txt2tags"),			TEXT("txt2tags file"),									L_TXT2TAGS,		SCLEX_TXT2TAGS},
{TEXT("visualprolog"),	TEXT("Visual Prolog"),		TEXT("Visual Prolog file"),								L_VISUALPROLOG,	SCLEX_VISUALPROLOG},
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

TCHAR moduleFileName[1024];

HMODULE loadSciLexerDll()
{
	generic_string sciLexerPath = getSciLexerFullPathName(moduleFileName, 1024);

	// Do not check dll signature if npp is running in debug mode
	// This is helpful for developers to skip signature checking
	// while analyzing issue or modifying the lexer dll
#ifndef _DEBUG
	SecurityGard securityGard;
	bool isOK = securityGard.checkModule(sciLexerPath, nm_scilexer);

	if (!isOK)
	{
		::MessageBox(NULL,
			TEXT("Authenticode check failed:\rsigning certificate or hash is not recognized"),
			TEXT("Library verification failed"),
			MB_OK | MB_ICONERROR);
		return nullptr;
	}
#endif // !_DEBUG

	return ::LoadLibrary(sciLexerPath.c_str());
}

void ScintillaEditView::init(HINSTANCE hInst, HWND hPere)
{
	if (!_hLib)
	{
		throw std::runtime_error("ScintillaEditView::init : SCINTILLA ERROR - Can not load the dynamic library");
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
		throw std::runtime_error("ScintillaEditView::init : CreateWindowEx() function return null");
	}

	_pScintillaFunc = (SCINTILLA_FUNC)::SendMessage(_hSelf, SCI_GETDIRECTFUNCTION, 0, 0);
	_pScintillaPtr = (SCINTILLA_PTR)::SendMessage(_hSelf, SCI_GETDIRECTPOINTER, 0, 0);

    _userDefineDlg.init(_hInst, _hParent, this);

	if (!_pScintillaFunc)
	{
		throw std::runtime_error("ScintillaEditView::init : SCI_GETDIRECTFUNCTION message failed");
	}

	if (!_pScintillaPtr)
	{
		throw std::runtime_error("ScintillaEditView::init : SCI_GETDIRECTPOINTER message failed");
	}

    execute(SCI_SETMARGINMASKN, _SC_MARGE_FOLDER, SC_MASK_FOLDERS);
    showMargin(_SC_MARGE_FOLDER, true);

    execute(SCI_SETMARGINMASKN, _SC_MARGE_SYBOLE, (1<<MARK_BOOKMARK) | (1<<MARK_HIDELINESBEGIN) | (1<<MARK_HIDELINESEND) | (1<<MARK_HIDELINESUNDERLINE));

	execute(SCI_MARKERSETALPHA, MARK_BOOKMARK, 70);

	execute(SCI_MARKERDEFINE, MARK_HIDELINESUNDERLINE, SC_MARK_UNDERLINE);
	execute(SCI_MARKERSETBACK, MARK_HIDELINESUNDERLINE, 0x77CC77);

	if (NppParameters::getInstance()._dpiManager.scaleX(100) >= 150)
	{
		execute(SCI_RGBAIMAGESETWIDTH, 18);
		execute(SCI_RGBAIMAGESETHEIGHT, 18);
		execute(SCI_MARKERDEFINERGBAIMAGE, MARK_BOOKMARK, reinterpret_cast<LPARAM>(bookmark18));
		execute(SCI_MARKERDEFINERGBAIMAGE, MARK_HIDELINESBEGIN, reinterpret_cast<LPARAM>(hidelines_begin18));
		execute(SCI_MARKERDEFINERGBAIMAGE, MARK_HIDELINESEND, reinterpret_cast<LPARAM>(hidelines_end18));
	}
	else
	{
		execute(SCI_RGBAIMAGESETWIDTH, 14);
		execute(SCI_RGBAIMAGESETHEIGHT, 14);
		execute(SCI_MARKERDEFINERGBAIMAGE, MARK_BOOKMARK, reinterpret_cast<LPARAM>(bookmark14));
		execute(SCI_MARKERDEFINERGBAIMAGE, MARK_HIDELINESBEGIN, reinterpret_cast<LPARAM>(hidelines_begin14));
		execute(SCI_MARKERDEFINERGBAIMAGE, MARK_HIDELINESEND, reinterpret_cast<LPARAM>(hidelines_end14));
	}

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

	if ((NppParameters::getInstance()).getNppGUI()._writeTechnologyEngine == directWriteTechnology)
		execute(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE);
	// If useDirectWrite is turned off, leave the technology setting untouched,
	// so that existing plugins using SCI_SETTECHNOLOGY behave like before

	_codepage = ::GetACP();

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	_callWindowProc = CallWindowProc;
	_scintillaDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(scintillaStatic_Proc)));

	if (_defaultCharList.empty())
	{
		auto defaultCharListLen = execute(SCI_GETWORDCHARS);
		char *defaultCharList = new char[defaultCharListLen + 1];
		execute(SCI_GETWORDCHARS, 0, reinterpret_cast<LPARAM>(defaultCharList));
		defaultCharList[defaultCharListLen] = '\0';
		_defaultCharList = defaultCharList;
		delete[] defaultCharList;
	}
	//Get the startup document and make a buffer for it so it can be accessed like a file
	attachDefaultDoc();
}

LRESULT CALLBACK ScintillaEditView::scintillaStatic_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	ScintillaEditView *pScint = (ScintillaEditView *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));

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
		bool makeTouchPadCompetible = ((NppParameters::getInstance()).getSVP())._disableAdvancedScrolling;

		if (pScint && (isSynpnatic || makeTouchPadCompetible))
			return (pScint->scintillaNew_Proc(hwnd, Message, wParam, lParam));

		ScintillaEditView *pScintillaOnMouse = (ScintillaEditView *)(::GetWindowLongPtr(hwndOnMouse, GWLP_USERDATA));
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

			if (LOWORD(wParam) & MK_SHIFT)
			{
				// move 3 columns at a time
				::CallWindowProc(_scintillaDefaultProc, hwnd, WM_HSCROLL, ((short)HIWORD(wParam) < 0) ? SB_LINERIGHT : SB_LINELEFT, NULL);
				::CallWindowProc(_scintillaDefaultProc, hwnd, WM_HSCROLL, ((short)HIWORD(wParam) < 0) ? SB_LINERIGHT : SB_LINELEFT, NULL);
				::CallWindowProc(_scintillaDefaultProc, hwnd, WM_HSCROLL, ((short)HIWORD(wParam) < 0) ? SB_LINERIGHT : SB_LINELEFT, NULL);
				return TRUE;
			}

			//Have to perform the scroll first, because the first/last line do not get updated untill after the scroll has been parsed
			LRESULT scrollResult = ::CallWindowProc(_scintillaDefaultProc, hwnd, Message, wParam, lParam);
			return scrollResult;
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

				UINT codepage = static_cast<UINT>(execute(SCI_GETCODEPAGE));

				// get the current text selection

				Sci_CharacterRange range = getSelection();
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
				SCNotification notification = {};
				notification.nmhdr.code = SCN_PAINTED;
				notification.nmhdr.hwndFrom = _hSelf;
				notification.nmhdr.idFrom = ::GetDlgCtrlID(_hSelf);
				::SendMessage(_hParent, WM_NOTIFY, LINKTRIGGERED, reinterpret_cast<LPARAM>(&notification));

			}
			break;
		}

		case WM_VSCROLL :
		{
			break;
		}

		case WM_RBUTTONDOWN:
		{
			bool rightClickKeepsSelection = ((NppParameters::getInstance()).getSVP())._rightClickKeepsSelection;
			if (rightClickKeepsSelection)
			{
				int clickX = GET_X_LPARAM(lParam);
				int marginX = static_cast<int>(execute(SCI_POINTXFROMPOSITION, 0, 0));
				if (clickX >= marginX)
				{
					// if right-click in the editing area (not the margins!),
					// don't let this go to Scintilla because it will 
					// move the caret to the right-clicked location,
					// cancelling any selection made by the user
					return TRUE;
				}
			}
			break;
		}
	}
	return _callWindowProc(_scintillaDefaultProc, hwnd, Message, wParam, lParam);
}

#define DEFAULT_FONT_NAME "Courier New"

void ScintillaEditView::setSpecialStyle(const Style & styleToSet)
{
	int styleID = styleToSet._styleID;
	if ( styleToSet._colorStyle & COLORSTYLE_FOREGROUND )
	    execute(SCI_STYLESETFORE, styleID, styleToSet._fgColor);

    if ( styleToSet._colorStyle & COLORSTYLE_BACKGROUND )
	    execute(SCI_STYLESETBACK, styleID, styleToSet._bgColor);

    if (styleToSet._fontName && lstrcmp(styleToSet._fontName, TEXT("")) != 0)
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

		if (!NppParameters::getInstance().isInFontList(styleToSet._fontName))
		{
			execute(SCI_STYLESETFONT, styleID, reinterpret_cast<LPARAM>(DEFAULT_FONT_NAME));
		}
		else
		{
			const char * fontNameA = wmc.wchar2char(styleToSet._fontName, CP_UTF8);
			execute(SCI_STYLESETFONT, styleID, reinterpret_cast<LPARAM>(fontNameA));
		}
	}
	int fontStyle = styleToSet._fontStyle;
    if (fontStyle != STYLE_NOT_USED)
    {
        execute(SCI_STYLESETBOLD,		styleID, fontStyle & FONTSTYLE_BOLD);
        execute(SCI_STYLESETITALIC,		styleID, fontStyle & FONTSTYLE_ITALIC);
        execute(SCI_STYLESETUNDERLINE,	styleID, fontStyle & FONTSTYLE_UNDERLINE);
    }

	if (styleToSet._fontSize > 0)
		execute(SCI_STYLESETSIZE, styleID, styleToSet._fontSize);
}

void ScintillaEditView::setHotspotStyle(Style& styleToSet)
{
	StyleMap* styleMap;
	if ( _hotspotStyles.find(_currentBuffer) == _hotspotStyles.end() )
	{
		_hotspotStyles[_currentBuffer] = new StyleMap;
	}
	styleMap = _hotspotStyles[_currentBuffer];
	(*styleMap)[styleToSet._styleID] = styleToSet;

	setStyle(styleToSet);
}

void ScintillaEditView::setStyle(Style styleToSet)
{
	GlobalOverride & go = NppParameters::getInstance().getGlobalOverrideStyle();

	if (go.isEnable())
	{
		StyleArray & stylers = NppParameters::getInstance().getMiscStylerArray();
		int i = stylers.getStylerIndexByName(TEXT("Global override"));
		if (i != -1)
		{
			Style & style = stylers.getStyler(i);

			if (go.enableFg)
			{
				if (style._colorStyle & COLORSTYLE_FOREGROUND)
				{
					styleToSet._colorStyle |= COLORSTYLE_FOREGROUND;
					styleToSet._fgColor = style._fgColor;
				}
				else
				{
					if (styleToSet._styleID == STYLE_DEFAULT) //if global is set to transparent, use default style color
						styleToSet._colorStyle |= COLORSTYLE_FOREGROUND;
					else
						styleToSet._colorStyle &= ~COLORSTYLE_FOREGROUND;
				}
			}

			if (go.enableBg)
			{
				if (style._colorStyle & COLORSTYLE_BACKGROUND)
				{
					styleToSet._colorStyle |= COLORSTYLE_BACKGROUND;
					styleToSet._bgColor = style._bgColor;
				}
				else
				{
					if (styleToSet._styleID == STYLE_DEFAULT) 	//if global is set to transparent, use default style color
						styleToSet._colorStyle |= COLORSTYLE_BACKGROUND;
					else
						styleToSet._colorStyle &= ~COLORSTYLE_BACKGROUND;
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

		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.xml.allow.scripts"), reinterpret_cast<LPARAM>("0"));
	}
	else if ((type == L_HTML) || (type == L_PHP) || (type == L_ASP) || (type == L_JSP))
	{
        execute(SCI_SETLEXER, SCLEX_HTML);
        const TCHAR *htmlKeyWords_generic = NppParameters::getInstance().getWordList(L_HTML, LANG_INDEX_INSTR);

		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char *htmlKeyWords = wmc.wchar2char(htmlKeyWords_generic, CP_ACP);
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

	execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, L_JS, LANG_INDEX_INSTR)));
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_DEFAULT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENTDOC, true);
}

void ScintillaEditView::setJsonLexer()
{
	execute(SCI_SETLEXER, SCLEX_JSON);

	const TCHAR *pKwArray[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

	makeStyle(L_JSON, pKwArray);

	string keywordList;
	string keywordList2;
	if (pKwArray[LANG_INDEX_INSTR])
	{
		wstring kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordList = wstring2string(kwlW, CP_ACP);
	}

	if (pKwArray[LANG_INDEX_INSTR2])
	{
		wstring kwlW = pKwArray[LANG_INDEX_INSTR2];
		keywordList2 = wstring2string(kwlW, CP_ACP);
	}

	execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, L_JSON, LANG_INDEX_INSTR)));
	execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList2, L_JSON, LANG_INDEX_INSTR2)));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
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

	execute(SCI_SETKEYWORDS, 4, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, L_PHP, LANG_INDEX_INSTR)));

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

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("asp.default.language"), reinterpret_cast<LPARAM>("2"));

	execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, L_VB, LANG_INDEX_INSTR)));

    execute(SCI_STYLESETEOLFILLED, SCE_HBA_DEFAULT, true);
}

void ScintillaEditView::setUserLexer(const TCHAR *userLangName)
{
	int setKeywordsCounter = 0;
    execute(SCI_SETLEXER, SCLEX_USER);

	UserLangContainer * userLangContainer = userLangName? NppParameters::getInstance().getULCFromName(userLangName):_userDefineDlg._pCurrentUserLang;

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
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("userDefine.isCaseIgnored"),		  reinterpret_cast<LPARAM>(userLangContainer->_isCaseIgnored ? "1":"0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("userDefine.allowFoldOfComments"),  reinterpret_cast<LPARAM>(userLangContainer->_allowFoldOfComments ? "1":"0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("userDefine.foldCompact"),		  reinterpret_cast<LPARAM>(userLangContainer->_foldCompact ? "1":"0"));

    char name[] = "userDefine.prefixKeywords0";
	for (int i=0 ; i<SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
	{
		itoa(i+1, (name+25), 10);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>(name), reinterpret_cast<LPARAM>(userLangContainer->_isPrefix[i] ? "1" : "0"));
	}

	for (int i = 0 ; i < SCE_USER_KWLIST_TOTAL ; ++i)
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char * keyWords_char = wmc.wchar2char(userLangContainer->_keywordLists[i], codepage);

		if (globalMappper().setLexerMapper.find(i) != globalMappper().setLexerMapper.end())
		{
			execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>(globalMappper().setLexerMapper[i].c_str()), reinterpret_cast<LPARAM>(keyWords_char));
		}
		else // OPERATORS2, FOLDERS_IN_CODE2, FOLDERS_IN_COMMENT, KEYWORDS1-8
		{
			char temp[max_char];
			bool inDoubleQuote = false;
			bool inSingleQuote = false;
			bool nonWSFound = false;
			int index = 0;
			for (size_t j=0, len = strlen(keyWords_char); j<len && index < (max_char-1); ++j)
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

 	char intBuffer[32];

	sprintf(intBuffer, "%d", userLangContainer->_forcePureLC);
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("userDefine.forcePureLC"), reinterpret_cast<LPARAM>(intBuffer));

	sprintf(intBuffer, "%d", userLangContainer->_decimalSeparator);
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("userDefine.decimalSeparator"), reinterpret_cast<LPARAM>(intBuffer));

	// at the end (position SCE_USER_KWLIST_TOTAL) send id values
	sprintf(intBuffer, "%" PRIuPTR, reinterpret_cast<uintptr_t>(userLangContainer->getName())); // use numeric value of TCHAR pointer
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("userDefine.udlName"), reinterpret_cast<LPARAM>(intBuffer));

	sprintf(intBuffer, "%" PRIuPTR, reinterpret_cast<uintptr_t>(_currentBufferID)); // use numeric value of BufferID pointer
    execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("userDefine.currentBufferID"), reinterpret_cast<LPARAM>(intBuffer));

	for (int i = 0 ; i < SCE_USER_STYLE_TOTAL_STYLES ; ++i)
	{
		Style & style = userLangContainer->_styleArray.getStyler(i);

		if (style._styleID == STYLE_NOT_USED)
			continue;

		char nestingBuffer[32];
		sprintf(nestingBuffer, "userDefine.nesting.%02d", i );
		sprintf(intBuffer, "%d", style._nesting);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>(nestingBuffer), reinterpret_cast<LPARAM>(intBuffer));

		setStyle(style);
	}
}

void ScintillaEditView::setExternalLexer(LangType typeDoc)
{
	int id = typeDoc - L_EXTERNAL;
	TCHAR * name = NppParameters::getInstance().getELCFromIndex(id)._name;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	const char *pName = wmc.wchar2char(name, CP_ACP);

	execute(SCI_SETLEXERLANGUAGE, 0, reinterpret_cast<LPARAM>(pName));

	LexerStyler *pStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(name);
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
				execute(SCI_SETKEYWORDS, style._keywordClass, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, typeDoc, style._keywordClass)));
			}
		}
	}
}

void ScintillaEditView::setCppLexer(LangType langType)
{
    const char *cppInstrs;
    const char *cppTypes;
    const TCHAR *doxygenKeyWords  = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);

    execute(SCI_SETLEXER, SCLEX_CPP);

	if (langType != L_RC)
    {
        if (doxygenKeyWords)
		{
			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			const char * doxygenKeyWords_char = wmc.wchar2char(doxygenKeyWords, CP_ACP);
			execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(doxygenKeyWords_char));
		}
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

	execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(cppInstrs));
	execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(cppTypes));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));

	// Disable track preprocessor to avoid incorrect detection.
	// In the most of cases, the symbols are defined outside of file.
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.track.preprocessor"), reinterpret_cast<LPARAM>("0"));
}

void ScintillaEditView::setJsLexer()
{
	const TCHAR *doxygenKeyWords = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);

	execute(SCI_SETLEXER, SCLEX_CPP);
	const TCHAR *pKwArray[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	makeStyle(L_JAVASCRIPT, pKwArray);

	if (doxygenKeyWords)
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char * doxygenKeyWords_char = wmc.wchar2char(doxygenKeyWords, CP_ACP);
		execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(doxygenKeyWords_char));
	}

	const TCHAR *newLexerName = ScintillaEditView::langNames[L_JAVASCRIPT].lexerName;
	LexerStyler *pNewStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(newLexerName);
	if (pNewStyler) // New js styler is available, so we can use it do more modern styling
	{
		for (int i = 0, nb = pNewStyler->getNbStyler(); i < nb; ++i)
		{
			Style & style = pNewStyler->getStyler(i);
			setStyle(style);
		}

		basic_string<char> keywordListInstruction("");
		basic_string<char> keywordListType("");
		basic_string<char> keywordListInstruction2("");

		if (pKwArray[LANG_INDEX_INSTR])
		{
			basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
			keywordListInstruction = wstring2string(kwlW, CP_ACP);
		}
		const char *jsInstrs = getCompleteKeywordList(keywordListInstruction, L_JAVASCRIPT, LANG_INDEX_INSTR);

		if (pKwArray[LANG_INDEX_TYPE])
		{
			basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_TYPE];
			keywordListType = wstring2string(kwlW, CP_ACP);
		}
		const char *jsTypes = getCompleteKeywordList(keywordListType, L_JAVASCRIPT, LANG_INDEX_TYPE);

		if (pKwArray[LANG_INDEX_INSTR2])
		{
			basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR2];
			keywordListInstruction2 = wstring2string(kwlW, CP_ACP);
		}
		const char *jsInstrs2 = getCompleteKeywordList(keywordListInstruction2, L_JAVASCRIPT, LANG_INDEX_INSTR2);

		execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(jsInstrs));
		execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(jsTypes));
		execute(SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(jsInstrs2));
	}
	else // New js styler is not available, we use the old styling for the sake of retro-compatibility
	{
		const TCHAR *lexerName = ScintillaEditView::langNames[L_JS].lexerName;
		LexerStyler *pOldStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(lexerName);

		if (pOldStyler)
		{
			for (int i = 0, nb = pOldStyler->getNbStyler(); i < nb; ++i)
			{
				Style style = pOldStyler->getStyler(i);	//not by reference, but copy
				int cppID = style._styleID;

				switch (style._styleID)
				{
					case SCE_HJ_DEFAULT: cppID = SCE_C_DEFAULT; break;
					case SCE_HJ_WORD: cppID = SCE_C_IDENTIFIER; break;
					case SCE_HJ_SYMBOLS: cppID = SCE_C_OPERATOR; break;
					case SCE_HJ_COMMENT: cppID = SCE_C_COMMENT; break;
					case SCE_HJ_COMMENTLINE: cppID = SCE_C_COMMENTLINE; break;
					case SCE_HJ_COMMENTDOC: cppID = SCE_C_COMMENTDOC; break;
					case SCE_HJ_NUMBER: cppID = SCE_C_NUMBER; break;
					case SCE_HJ_KEYWORD: cppID = SCE_C_WORD; break;
					case SCE_HJ_DOUBLESTRING: cppID = SCE_C_STRING; break;
					case SCE_HJ_SINGLESTRING: cppID = SCE_C_CHARACTER; break;
					case SCE_HJ_REGEX: cppID = SCE_C_REGEX; break;
				}
				style._styleID = cppID;
				setStyle(style);
			}
		}
		execute(SCI_STYLESETEOLFILLED, SCE_C_DEFAULT, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENTLINE, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENT, true);
		execute(SCI_STYLESETEOLFILLED, SCE_C_COMMENTDOC, true);

		makeStyle(L_JS, pKwArray);

		basic_string<char> keywordListInstruction("");
		if (pKwArray[LANG_INDEX_INSTR])
		{
			basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
			keywordListInstruction = wstring2string(kwlW, CP_ACP);
		}
		const char *jsEmbeddedInstrs = getCompleteKeywordList(keywordListInstruction, L_JS, LANG_INDEX_INSTR);
		execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(jsEmbeddedInstrs));
	}

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));

	// Disable track preprocessor to avoid incorrect detection.
	// In the most of cases, the symbols are defined outside of file.
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.track.preprocessor"), reinterpret_cast<LPARAM>("0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.backquoted.strings"), reinterpret_cast<LPARAM>("1"));
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

	execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(tclInstrs));
	execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(tclTypes));
}

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
	const TCHAR *doxygenKeyWordsW = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);
	if (doxygenKeyWordsW)
	{
		doxygenKeyWordsString = wstring2string(doxygenKeyWordsW, CP_ACP);
	}
	const char *doxygenKeyWords = doxygenKeyWordsString.c_str();

	execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(objcInstrs));
    execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(objcTypes));
	execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(doxygenKeyWords));
	execute(SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(objCDirective));
	execute(SCI_SETKEYWORDS, 4, reinterpret_cast<LPARAM>(objCQualifier));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::setKeywords(LangType langType, const char *keywords, int index)
{
	std::basic_string<char> wordList;
	wordList = (keywords)?keywords:"";
	execute(SCI_SETKEYWORDS, index, reinterpret_cast<LPARAM>(getCompleteKeywordList(wordList, langType, index)));
}

void ScintillaEditView::setLexer(int lexerID, LangType langType, int whichList)
{
	execute(SCI_SETLEXER, lexerID);

	const TCHAR *pKwArray[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

	makeStyle(langType, pKwArray);

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();

	if (whichList & LIST_0)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_INSTR], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_INSTR);
	}

	if (whichList & LIST_1)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_INSTR2], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_INSTR2);
	}

	if (whichList & LIST_2)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_TYPE], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE);
	}

	if (whichList & LIST_3)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_TYPE2], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE2);
	}

	if (whichList & LIST_4)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_TYPE3], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE3);
	}

	if (whichList & LIST_5)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_TYPE4], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE4);
	}

	if (whichList & LIST_6)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_TYPE5], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE5);
	}

	if (whichList & LIST_7)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_TYPE6], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE6);
	}

	if (whichList & LIST_8)
	{
		const char * keyWords_char = wmc.wchar2char(pKwArray[LANG_INDEX_TYPE7], CP_ACP);
		setKeywords(langType, keyWords_char, LANG_INDEX_TYPE7);
	}

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::makeStyle(LangType language, const TCHAR **keywordArray)
{
	const TCHAR * lexerName = ScintillaEditView::langNames[language].lexerName;
	LexerStyler *pStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(lexerName);
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

void ScintillaEditView::restoreDefaultWordChars()
{
	execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>(_defaultCharList.c_str()));
}

void ScintillaEditView::addCustomWordChars()
{
	NppParameters& nppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = nppParam.getNppGUI();

	if (nppGUI._customWordChars.empty())
		return;

	string chars2addStr;
	for (size_t i = 0; i < nppGUI._customWordChars.length(); ++i)
	{
		bool found = false;
		char char2check = nppGUI._customWordChars[i];
		for (size_t j = 0; j < _defaultCharList.length(); ++j)
		{
			char wordChar = _defaultCharList[j];
			if (char2check == wordChar)
			{
				found = true;
				break;
			}
		}
		if (not found)
		{
			chars2addStr.push_back(char2check);
		}
	}

	if (not chars2addStr.empty())
	{
		string newCharList = _defaultCharList;
		newCharList += chars2addStr;
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>(newCharList.c_str()));
	}
}

void ScintillaEditView::setWordChars()
{
	NppParameters& nppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = nppParam.getNppGUI();
	if (nppGUI._isWordCharDefault)
		restoreDefaultWordChars();
	else
		addCustomWordChars();
}

void ScintillaEditView::defineDocType(LangType typeDoc)
{
    StyleArray & stylers = NppParameters::getInstance().getMiscStylerArray();
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

	ScintillaViewParams & svp = (ScintillaViewParams &)NppParameters::getInstance().getSVP();
	if (svp._folderStyle != FOLDER_STYLE_NONE)
		showMargin(_SC_MARGE_FOLDER, isNeededFolderMarge(typeDoc));

	switch (typeDoc)
	{
		case L_C :
		case L_CPP :
		case L_JAVA :
		case L_RC :
		case L_CS :
		case L_FLASH :
		case L_SWIFT:
			setCppLexer(typeDoc); break;

		case L_JS:
		case L_JAVASCRIPT:
			setJsLexer(); break;

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

		case L_JSON:
			setJsonLexer(); break;

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
			LexerStyler *pStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(TEXT("nfo"));

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

			Buffer * buf = MainFileManager.getBufferByID(_currentBufferID);

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

		case L_FORTRAN_77 :
			setFortran77Lexer(); break;

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

		case L_BAANC:
			setBaanCLexer(); break;

		case L_SREC :
			setSrecLexer(); break;

		case L_IHEX :
			setIHexLexer(); break;

		case L_TEHEX :
			setTEHexLexer(); break;

		case L_ASN1 :
			setAsn1Lexer(); break;

		case L_AVS :
			setAVSLexer(); break;

		case L_BLITZBASIC :
			setBlitzBasicLexer(); break;

		case L_PUREBASIC :
			setPureBasicLexer(); break;

		case L_FREEBASIC :
			setFreeBasicLexer(); break;

		case L_CSOUND :
			setCsoundLexer(); break;

		case L_ERLANG :
			setErlangLexer(); break;

		case L_ESCRIPT :
			setESCRIPTLexer(); break;

		case L_FORTH :
			setForthLexer(); break;

		case L_LATEX :
			setLatexLexer(); break;

		case L_MMIXAL :
			setMMIXALLexer(); break;

		case L_NIMROD :
			setNimrodLexer(); break;

		case L_NNCRONTAB :
			setNncrontabLexer(); break;

		case L_OSCRIPT :
			setOScriptLexer(); break;

		case L_REBOL :
			setREBOLLexer(); break;

		case L_REGISTRY :
			setRegistryLexer(); break;

		case L_RUST :
			setRustLexer(); break;

		case L_SPICE :
			setSpiceLexer(); break;

		case L_TXT2TAGS :
			setTxt2tagsLexer(); break;

		case L_VISUALPROLOG:
			setVisualPrologLexer(); break;

		case L_TEXT :
		default :
			if (typeDoc >= L_EXTERNAL && typeDoc < NppParameters::getInstance().L_END)
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
    setTabSettings(NppParameters::getInstance().getLangFromID(typeDoc));
	
	if (svp._indentGuideLineShow)
	{
		const auto currentIndentMode = execute(SCI_GETINDENTATIONGUIDES);
		// Python like indentation, excludes lexers (Nim, VB, YAML, etc.)
		// that includes tailing empty or whitespace only lines in folding block.
		const bool pythonLike = (typeDoc == L_PYTHON || typeDoc == L_COFFEESCRIPT || typeDoc == L_HASKELL);
		const int docIndentMode = pythonLike ? SC_IV_LOOKFORWARD : SC_IV_LOOKBOTH;
		if (currentIndentMode != docIndentMode)
			execute(SCI_SETINDENTATIONGUIDES, docIndentMode);
	}
}

BufferID ScintillaEditView::attachDefaultDoc()
{
	// get the doc pointer attached (by default) on the view Scintilla
	Document doc = execute(SCI_GETDOCPOINTER, 0, 0);
	execute(SCI_ADDREFDOCUMENT, 0, doc);
	BufferID id = MainFileManager.bufferFromDocument(doc, false, true);//true, true);	//keep counter on 1
	Buffer * buf = MainFileManager.getBufferByID(id);

	MainFileManager.addBufferReference(id, this);	//add a reference. Notepad only shows the buffer in tabbar

	_currentBufferID = id;
	_currentBuffer = buf;
	bufferUpdated(buf, BufferChangeMask);	//make sure everything is in sync with the buffer, since no reference exists

	return id;
}

void ScintillaEditView::saveCurrentPos()
{
	//Save data so, that the current topline becomes visible again after restoring.
	int32_t displayedLine = static_cast<int32_t>(execute(SCI_GETFIRSTVISIBLELINE));
	int32_t docLine = static_cast<int32_t>(execute(SCI_DOCLINEFROMVISIBLE, displayedLine));		//linenumber of the line displayed in the top
	int32_t offset = displayedLine - static_cast<int32_t>(execute(SCI_VISIBLEFROMDOCLINE, docLine));		//use this to calc offset of wrap. If no wrap this should be zero
	int wrapCount = static_cast<int32_t>(execute(SCI_WRAPCOUNT, docLine));

	Buffer * buf = MainFileManager.getBufferByID(_currentBufferID);

	Position pos;
	// the correct visible line number
	pos._firstVisibleLine = docLine;
	pos._startPos = static_cast<int>(execute(SCI_GETANCHOR));
	pos._endPos = static_cast<int>(execute(SCI_GETCURRENTPOS));
	pos._xOffset = static_cast<int>(execute(SCI_GETXOFFSET));
	pos._selMode = static_cast<int32_t>(execute(SCI_GETSELECTIONMODE));
	pos._scrollWidth = static_cast<int32_t>(execute(SCI_GETSCROLLWIDTH));
	pos._offset = offset;
	pos._wrapCount = wrapCount;

	buf->setPosition(pos, this);
}

// restore current position is executed in two steps.
// The detection wrap state done in the pre step function:
// if wrap is enabled, then _positionRestoreNeeded is activated
// so post step function will be cakked in the next SCN_PAINTED message
void ScintillaEditView::restoreCurrentPosPreStep()
{
	Buffer * buf = MainFileManager.getBufferByID(_currentBufferID);
	Position & pos = buf->getPosition(this);

	execute(SCI_SETSELECTIONMODE, pos._selMode);	//enable
	execute(SCI_SETANCHOR, pos._startPos);
	execute(SCI_SETCURRENTPOS, pos._endPos);
	execute(SCI_CANCEL);							//disable
	if (not isWrap()) //only offset if not wrapping, otherwise the offset isnt needed at all
	{
		execute(SCI_SETSCROLLWIDTH, pos._scrollWidth);
		execute(SCI_SETXOFFSET, pos._xOffset);
	}
	execute(SCI_CHOOSECARETX); // choose current x position
	int lineToShow = static_cast<int32_t>(execute(SCI_VISIBLEFROMDOCLINE, pos._firstVisibleLine));
	execute(SCI_SETFIRSTVISIBLELINE, lineToShow);
	if (isWrap())
	{
		// Enable flag 'positionRestoreNeeded' so that function restoreCurrentPosPostStep get called
		// once scintilla send SCN_PAITED notification
		_positionRestoreNeeded = true;
	}
	_restorePositionRetryCount = 0;

}

// If wrap is enabled, the post step function will be called in the next SCN_PAINTED message
// to scroll several lines to set the first visible line to the correct wrapped line.
void ScintillaEditView::restoreCurrentPosPostStep()
{
	if (!_positionRestoreNeeded)
		return;

	Buffer * buf = MainFileManager.getBufferByID(_currentBufferID);
	Position & pos = buf->getPosition(this);

	++_restorePositionRetryCount;

	// Scintilla can send several SCN_PAINTED notifications before the buffer is ready to be displayed. 
	// this post step function is therefore iterated several times in a maximum of 8 iterations. 
	// 8 is an arbitrary number. 2 is a minimum. Maximum value is unknown.
	if (_restorePositionRetryCount > 8)
	{
		// Abort the position restoring  process. Buffer topology may have changed
		_positionRestoreNeeded = false;
		return;
	}
	
	int32_t displayedLine = static_cast<int32_t>(execute(SCI_GETFIRSTVISIBLELINE));
	int32_t docLine = static_cast<int32_t>(execute(SCI_DOCLINEFROMVISIBLE, displayedLine));		//linenumber of the line displayed in the 
	

	// check docLine must equals saved position
	if (docLine != pos._firstVisibleLine)
	{
		
		// Scintilla has paint the buffer but the position is not correct.
		int lineToShow = static_cast<int32_t>(execute(SCI_VISIBLEFROMDOCLINE, pos._firstVisibleLine));
		execute(SCI_SETFIRSTVISIBLELINE, lineToShow);
	}
	else if (pos._offset > 0)
	{
		// don't scroll anything if the wrap count is different than the saved one.
		// Buffer update may be in progress (in case wrap is enabled)
		int wrapCount = static_cast<int32_t>(execute(SCI_WRAPCOUNT, docLine));
		if (wrapCount == pos._wrapCount)
		{
			scroll(0, pos._offset);
			_positionRestoreNeeded = false;
		}
	}
	else
	{
		// Buffer position is correct, and there is no scroll to apply
		_positionRestoreNeeded = false;
	}
}

void ScintillaEditView::restyleBuffer()
{
	execute(SCI_CLEARDOCUMENTSTYLE);
	execute(SCI_COLOURISE, 0, -1);
	_currentBuffer->setNeedsLexing(false);
}

void ScintillaEditView::styleChange()
{
	defineDocType(_currentBuffer->getLangType());
	restyleBuffer();
}

void ScintillaEditView::activateBuffer(BufferID buffer)
{
	if (buffer == BUFFER_INVALID)
		return;
	if (buffer == _currentBuffer)
		return;
	Buffer * newBuf = MainFileManager.getBufferByID(buffer);

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

	setWordChars();

	if (_currentBuffer->getNeedsLexing())
	{
		restyleBuffer();
	}

	// Everything should be updated, but the language
	bufferUpdated(_currentBuffer, (BufferChangeMask & ~BufferChangeLanguage));

	// restore the collapsed info
	const std::vector<size_t> & lineStateVectorNew = newBuf->getHeaderLineState(this);
	syncFoldStateWith(lineStateVectorNew);

	restoreCurrentPosPreStep();

	//setup line number margin
	int numLines = static_cast<int32_t>(execute(SCI_GETLINECOUNT));

	char numLineStr[32];
	itoa(numLines, numLineStr, 10);

	runMarkers(true, 0, true, false);
    return;	//all done
}

void ScintillaEditView::getCurrentFoldStates(std::vector<size_t> & lineStateVector)
{
	// xCodeOptimization1304: For active document get folding state from Scintilla.
	// The code using SCI_CONTRACTEDFOLDNEXT is usually 10%-50% faster than checking each line of the document!!
	size_t contractedFoldHeaderLine = 0;

	do {
		contractedFoldHeaderLine = static_cast<size_t>(execute(SCI_CONTRACTEDFOLDNEXT, contractedFoldHeaderLine));
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
	size_t nbLineState = lineStateVectorNew.size();
	for (size_t i = 0 ; i < nbLineState ; ++i)
	{
		auto line = lineStateVectorNew.at(i);
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
			execute(SCI_SETEOLMODE, static_cast<int>(_currentBuffer->getEolFormat()));
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

bool ScintillaEditView::isFoldIndentationBased() const
{
	const auto lexer = execute(SCI_GETLEXER);
	// search IndentAmount in scintilla\lexers folder
	return lexer == SCLEX_PYTHON
		|| lexer == SCLEX_COFFEESCRIPT
		|| lexer == SCLEX_HASKELL
		|| lexer == SCLEX_NIMROD
		|| lexer == SCLEX_VB
		|| lexer == SCLEX_YAML
	;
}

namespace {

struct FoldLevelStack
{
	int levelCount = 0; // 1-based level number
	int levelStack[MAX_FOLD_COLLAPSE_LEVEL]{};

	void push(int level)
	{
		while (levelCount != 0 && level <= levelStack[levelCount - 1])
		{
			--levelCount;
		}
		levelStack[levelCount++] = level;
	}
};

}

void ScintillaEditView::collapseFoldIndentationBased(int level2Collapse, bool mode)
{
	execute(SCI_COLOURISE, 0, -1);

	FoldLevelStack levelStack;
	++level2Collapse; // 1-based level number

	const int maxLine = static_cast<int32_t>(execute(SCI_GETLINECOUNT));
	int line = 0;

	while (line < maxLine)
	{
		int level = static_cast<int32_t>(execute(SCI_GETFOLDLEVEL, line));
		if (level & SC_FOLDLEVELHEADERFLAG)
		{
			level &= SC_FOLDLEVELNUMBERMASK;
			// don't need the actually level number, only the relationship.
			levelStack.push(level);
			if (level2Collapse == levelStack.levelCount)
			{
				if (isFolded(line) != mode)
				{
					fold(line, mode);
				}
				// skip all children lines, required to avoid buffer overrun.
				line = static_cast<int32_t>(execute(SCI_GETLASTCHILD, line, -1));
			}
		}
		++line;
	}

	runMarkers(true, 0, true, false);
}

void ScintillaEditView::collapse(int level2Collapse, bool mode)
{
	if (isFoldIndentationBased())
	{
		collapseFoldIndentationBased(level2Collapse, mode);
		return;
	}

	execute(SCI_COLOURISE, 0, -1);

	int maxLine = static_cast<int32_t>(execute(SCI_GETLINECOUNT));

	for (int line = 0; line < maxLine; ++line)
	{
		int level = static_cast<int32_t>(execute(SCI_GETFOLDLEVEL, line));
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
	auto currentLine = this->getCurrentLineNumber();
	fold(currentLine, mode);
}

void ScintillaEditView::fold(size_t line, bool mode)
{
    auto endStyled = execute(SCI_GETENDSTYLED);
    auto len = execute(SCI_GETTEXTLENGTH);

    if (endStyled < len)
        execute(SCI_COLOURISE, 0, -1);

	int headerLine;
	auto level = execute(SCI_GETFOLDLEVEL, line);

	if (level & SC_FOLDLEVELHEADERFLAG)
		headerLine = static_cast<int32_t>(line);
	else
	{
		headerLine = static_cast<int32_t>(execute(SCI_GETFOLDPARENT, line));
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

		::SendMessage(_hParent, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scnN));
	}
}

void ScintillaEditView::foldAll(bool mode)
{
	auto maxLine = execute(SCI_GETLINECOUNT);

	for (int line = 0; line < maxLine; ++line)
	{
		auto level = execute(SCI_GETFOLDLEVEL, line);
		if (level & SC_FOLDLEVELHEADERFLAG)
			if (isFolded(line) != mode)
				fold(line, mode);
	}
}

void ScintillaEditView::getText(char *dest, size_t start, size_t end) const
{
	Sci_TextRange tr;
	tr.chrg.cpMin = static_cast<long>(start);
	tr.chrg.cpMax = static_cast<long>(end);
	tr.lpstrText = dest;
	execute(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}

generic_string ScintillaEditView::getGenericTextAsString(size_t start, size_t end) const
{
	assert(end > start);
	const size_t bufSize = end - start + 1;
	TCHAR *buf = new TCHAR[bufSize];
	getGenericText(buf, bufSize, start, end);
	generic_string text = buf;
	delete[] buf;
	return text;
}

void ScintillaEditView::getGenericText(TCHAR *dest, size_t destlen, size_t start, size_t end) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	char *destA = new char[end - start + 1];
	getText(destA, start, end);
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const TCHAR *destW = wmc.char2wchar(destA, cp);
	_tcsncpy_s(dest, destlen, destW, _TRUNCATE);
	delete [] destA;
}

// "mstart" and "mend" are pointers to indexes in the read string,
// which are converted to the corresponding indexes in the returned TCHAR string.

void ScintillaEditView::getGenericText(TCHAR *dest, size_t destlen, int start, int end, int *mstart, int *mend) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	char *destA = new char[end - start + 1];
	getText(destA, start, end);
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE))    ;
	const TCHAR *destW = wmc.char2wchar(destA, cp, mstart, mend);
	_tcsncpy_s(dest, destlen, destW, _TRUNCATE);
	delete [] destA;
}

void ScintillaEditView::insertGenericTextFrom(size_t position, const TCHAR *text2insert) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *text2insertA = wmc.wchar2char(text2insert, cp);
	execute(SCI_INSERTTEXT, position, reinterpret_cast<LPARAM>(text2insertA));
}

void ScintillaEditView::replaceSelWith(const char * replaceText)
{
	execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(replaceText));
}

void ScintillaEditView::getVisibleStartAndEndPosition(int * startPos, int * endPos)
{
	assert(startPos != NULL && endPos != NULL);
	// Get the position of the 1st and last showing chars from the edit view
	RECT rcEditView;
	getClientRect(rcEditView);
	LRESULT pos = execute(SCI_POSITIONFROMPOINT, 0, 0);
	LRESULT line = execute(SCI_LINEFROMPOSITION, pos);
	*startPos = static_cast<int32_t>(execute(SCI_POSITIONFROMLINE, line));
	pos = execute(SCI_POSITIONFROMPOINT, rcEditView.right - rcEditView.left, rcEditView.bottom - rcEditView.top);
	line = execute(SCI_LINEFROMPOSITION, pos);
	*endPos = static_cast<int32_t>(execute(SCI_GETLINEENDPOSITION, line));
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
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	char *txtA = new char[size + 1];
	getWordOnCaretPos(txtA, size);

	const TCHAR * txtW = wmc.char2wchar(txtA, cp);
	wcscpy_s(txt, size, txtW);
	delete [] txtA;
	return txt;
}

char * ScintillaEditView::getSelectedText(char * txt, int size, bool expand)
{
	if (!size)
		return NULL;
	Sci_CharacterRange range = getSelection();
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
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	char *txtA = new char[size + 1];
	getSelectedText(txtA, size, expand);

	const TCHAR * txtW = wmc.char2wchar(txtA, cp);
	wcscpy_s(txt, size, txtW);
	delete [] txtA;
	return txt;
}

int ScintillaEditView::searchInTarget(const TCHAR * text2Find, size_t lenOfText2Find, size_t fromPos, size_t toPos) const
{
	execute(SCI_SETTARGETRANGE, fromPos, toPos);

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *text2FindA = wmc.wchar2char(text2Find, cp);
	size_t text2FindALen = strlen(text2FindA);
   	size_t len = (lenOfText2Find > text2FindALen) ? lenOfText2Find : text2FindALen;
	return static_cast<int32_t>(execute(SCI_SEARCHINTARGET, len, reinterpret_cast<LPARAM>(text2FindA)));
}

void ScintillaEditView::appandGenericText(const TCHAR * text2Append) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *text2AppendA =wmc.wchar2char(text2Append, cp);
	execute(SCI_APPENDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

void ScintillaEditView::addGenericText(const TCHAR * text2Append) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *text2AppendA =wmc.wchar2char(text2Append, cp);
	execute(SCI_ADDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

void ScintillaEditView::addGenericText(const TCHAR * text2Append, long *mstart, long *mend) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *text2AppendA =wmc.wchar2char(text2Append, cp, mstart, mend);
	execute(SCI_ADDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

int32_t ScintillaEditView::replaceTarget(const TCHAR * str2replace, int fromTargetPos, int toTargetPos) const
{
	if (fromTargetPos != -1 || toTargetPos != -1)
	{
		execute(SCI_SETTARGETRANGE, fromTargetPos, toTargetPos);
	}
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *str2replaceA = wmc.wchar2char(str2replace, cp);
	return static_cast<int32_t>(execute(SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(str2replaceA)));
}

int ScintillaEditView::replaceTargetRegExMode(const TCHAR * re, int fromTargetPos, int toTargetPos) const
{
	if (fromTargetPos != -1 || toTargetPos != -1)
	{
		execute(SCI_SETTARGETRANGE, fromTargetPos, toTargetPos);
	}
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *reA = wmc.wchar2char(re, cp);
	return static_cast<int32_t>(execute(SCI_REPLACETARGETRE, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(reA)));
}

void ScintillaEditView::showAutoComletion(size_t lenEntered, const TCHAR* list)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *listA = wmc.wchar2char(list, cp);
	execute(SCI_AUTOCSHOW, lenEntered, reinterpret_cast<LPARAM>(listA));
}

void ScintillaEditView::showCallTip(int startPos, const TCHAR * def)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	const char *defA = wmc.wchar2char(def, cp);
	execute(SCI_CALLTIPSHOW, startPos, reinterpret_cast<LPARAM>(defA));
}

generic_string ScintillaEditView::getLine(size_t lineNumber)
{
	int32_t lineLen = static_cast<int32_t>(execute(SCI_LINELENGTH, lineNumber));
	const int bufSize = lineLen + 1;
	std::unique_ptr<TCHAR[]> buf = std::make_unique<TCHAR[]>(bufSize);
	getLine(lineNumber, buf.get(), bufSize);
	return buf.get();
}

void ScintillaEditView::getLine(size_t lineNumber, TCHAR * line, int lineBufferLen)
{
	// make sure the buffer length is enough to get the whole line
	auto lineLen = execute(SCI_LINELENGTH, lineNumber);
	if (lineLen >= lineBufferLen)
		return;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
	char *lineA = new char[lineBufferLen];
	// From Scintilla documentation for SCI_GETLINE: "The buffer is not terminated by a 0 character."
	memset(lineA, 0x0, sizeof(char) * lineBufferLen);
	
	execute(SCI_GETLINE, lineNumber, reinterpret_cast<LPARAM>(lineA));
	const TCHAR *lineW = wmc.char2wchar(lineA, cp);
	lstrcpyn(line, lineW, lineBufferLen);
	delete [] lineA;
}

void ScintillaEditView::addText(size_t length, const char *buf)
{
	execute(SCI_ADDTEXT, length, reinterpret_cast<LPARAM>(buf));
}

void ScintillaEditView::beginOrEndSelect()
{
	if (_beginSelectPosition == -1)
	{
		_beginSelectPosition = static_cast<int32_t>(execute(SCI_GETCURRENTPOS));
	}
	else
	{
		execute(SCI_SETANCHOR, static_cast<WPARAM>(_beginSelectPosition));
		_beginSelectPosition = -1;
	}
}

void ScintillaEditView::updateBeginEndSelectPosition(bool is_insert, size_t position, size_t length)
{
	if (_beginSelectPosition != -1 && static_cast<long long>(position) < _beginSelectPosition - 1)
	{
		if (is_insert)
			_beginSelectPosition += static_cast<long long>(length);
		else
			_beginSelectPosition -= static_cast<long long>(length);

		assert(_beginSelectPosition >= 0);
	}
}

void ScintillaEditView::marginClick(Sci_Position position, int modifiers)
{
	size_t lineClick = execute(SCI_LINEFROMPOSITION, position, 0);
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

void ScintillaEditView::expand(size_t& line, bool doExpand, bool force, int visLevels, int level)
{
	size_t lineMaxSubord = execute(SCI_GETLASTCHILD, line, level & SC_FOLDLEVELNUMBERMASK);
	++line;
	while (line <= lineMaxSubord)
    {
		if (force)
        {
			execute(((visLevels > 0) ? SCI_SHOWLINES : SCI_HIDELINES), line, line);
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
					expand(line, false, force, visLevels - 1);
			}
		}
        else
			++line;
	}

	runMarkers(true, 0, true, false);
}


void ScintillaEditView::performGlobalStyles()
{
	StyleArray & stylers = NppParameters::getInstance().getMiscStylerArray();

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
	::SendMessage(_hParent, NPPM_INTERNAL_EDGEMULTISETSIZE, 0, 0);

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

	COLORREF bookmarkMarginColor = veryLiteGrey;
	i = stylers.getStylerIndexByName(TEXT("Bookmark margin"));
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		bookmarkMarginColor = style._bgColor;
	}
	execute(SCI_SETMARGINTYPEN, _SC_MARGE_SYBOLE, SC_MARGIN_COLOUR);
	execute(SCI_SETMARGINBACKN, _SC_MARGE_SYBOLE, bookmarkMarginColor);

	COLORREF urlHoveredFG = grey;
	i = stylers.getStylerIndexByName(TEXT("URL hovered"));
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		urlHoveredFG = style._fgColor;
	}
	execute(SCI_INDICSETHOVERFORE, URL_INDIC, urlHoveredFG);

	COLORREF foldfgColor = white, foldbgColor = grey, activeFoldFgColor = red;
	getFoldColor(foldfgColor, foldbgColor, activeFoldFgColor);

	ScintillaViewParams & svp = (ScintillaViewParams &)NppParameters::getInstance().getSVP();
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

void ScintillaEditView::showIndentGuideLine(bool willBeShowed)
{
	auto typeDoc = _currentBuffer->getLangType();
	const bool pythonLike = (typeDoc == L_PYTHON || typeDoc == L_COFFEESCRIPT || typeDoc == L_HASKELL);
	const int docIndentMode = pythonLike ? SC_IV_LOOKFORWARD : SC_IV_LOOKBOTH;
	execute(SCI_SETINDENTATIONGUIDES, willBeShowed ? docIndentMode : SC_IV_NONE);
}

void ScintillaEditView::setLineIndent(int line, int indent) const
{
	if (indent < 0)
		return;
	Sci_CharacterRange crange = getSelection();
	int posBefore = static_cast<int32_t>(execute(SCI_GETLINEINDENTPOSITION, line));
	execute(SCI_SETLINEINDENTATION, line, indent);
	int32_t posAfter = static_cast<int32_t>(execute(SCI_GETLINEINDENTPOSITION, line));
	int posDifference = posAfter - posBefore;
	if (posAfter > posBefore)
	{
		// Move selection on
		if (crange.cpMin >= posBefore)
		{
			crange.cpMin += posDifference;
		}
		if (crange.cpMax >= posBefore)
		{
			crange.cpMax += posDifference;
		}
	}
	else if (posAfter < posBefore)
	{
		// Move selection back
		if (crange.cpMin >= posAfter)
		{
			if (crange.cpMin >= posBefore)
				crange.cpMin += posDifference;
			else
				crange.cpMin = posAfter;
		}

		if (crange.cpMax >= posAfter)
		{
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
		auto linesVisible = execute(SCI_LINESONSCREEN);
		if (linesVisible)
		{
			auto firstVisibleLineVis = execute(SCI_GETFIRSTVISIBLELINE);
			auto lastVisibleLineVis = linesVisible + firstVisibleLineVis + 1;

			auto lastVisibleLineDoc = execute(SCI_DOCLINEFROMVISIBLE, lastVisibleLineVis);

			int nbDigits = 3; // minimum number of digit should be 3
			if (lastVisibleLineDoc < 1000) {} //nbDigits = 3;
			else if (lastVisibleLineDoc < 10000) nbDigits = 4;
			else if (lastVisibleLineDoc < 100000) nbDigits = 5;
			else if (lastVisibleLineDoc < 1000000) nbDigits = 6;
			else // rare case
			{
				nbDigits = 7;
				lastVisibleLineDoc /= 1000000;

				while (lastVisibleLineDoc)
				{
					lastVisibleLineDoc /= 10;
					++nbDigits;
				}
			}
			auto pixelWidth = 8 + nbDigits * execute(SCI_TEXTWIDTH, STYLE_LINENUMBER, reinterpret_cast<LPARAM>("8"));
			execute(SCI_SETMARGINWIDTHN, _SC_MARGE_LINENUMBER, pixelWidth);
		}
	}
}

const char * ScintillaEditView::getCompleteKeywordList(std::basic_string<char> & kwl, LangType langType, int keywordIndex)
{
	kwl += " ";
	const TCHAR *defKwl_generic = NppParameters::getInstance().getWordList(langType, keywordIndex);

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	const char * defKwl = wmc.wchar2char(defKwl_generic, CP_ACP);
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

// Get selection range (fromLine, toLine) for the specified selection
// specify selectionNumber = -1 for the MAIN selection
pair<int, int> ScintillaEditView::getSelectionLinesRange(int selectionNumber /* = -1 */) const
{
	int numSelections = static_cast<int>(execute(SCI_GETSELECTIONS));

	int start_pos, end_pos;

	if ((selectionNumber < 0) || (selectionNumber >= numSelections))
	{
		start_pos = static_cast<int>(execute(SCI_GETSELECTIONSTART));
		end_pos = static_cast<int>(execute(SCI_GETSELECTIONEND));
	}
	else
	{
		start_pos = static_cast<int>(execute(SCI_GETSELECTIONNSTART, selectionNumber));
		end_pos = static_cast<int>(execute(SCI_GETSELECTIONNEND, selectionNumber));
	}

	int line1 = static_cast<int>(execute(SCI_LINEFROMPOSITION, start_pos));
	int line2 = static_cast<int>(execute(SCI_LINEFROMPOSITION, end_pos));

	if ((line1 != line2) && (execute(SCI_POSITIONFROMLINE, line2) == end_pos))
	{
		// if the end of the selection includes the line-ending, 
		// then don't include the following line in the range
		--line2;
	}

	return pair<int, int>(line1, line2);
}

void ScintillaEditView::currentLinesUp() const
{
	execute(SCI_MOVESELECTEDLINESUP);
}

void ScintillaEditView::currentLinesDown() const
{
	execute(SCI_MOVESELECTEDLINESDOWN);

	// Ensure the selection is within view
	execute(SCI_SCROLLRANGE, execute(SCI_GETSELECTIONEND), execute(SCI_GETSELECTIONSTART));
}

void ScintillaEditView::changeCase(__inout wchar_t * const strWToConvert, const int & nbChars, const TextCase & caseToConvert) const
{
	if (strWToConvert == nullptr || nbChars == NULL)
		return;

	switch (caseToConvert)
	{
		case UPPERCASE:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW((LPWSTR)strWToConvert[i]);
			}
			break; 
		} //case UPPERCASE
		case LOWERCASE:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW((LPWSTR)strWToConvert[i]);
			}
			break; 
		} //case LOWERCASE
		case TITLECASE_FORCE:
		case TITLECASE_BLEND:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				if (::IsCharAlphaW(strWToConvert[i]))
				{
					if ((i < 1) ? true : not ::IsCharAlphaNumericW(strWToConvert[i - 1]))
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW((LPWSTR)strWToConvert[i]);
					else if (caseToConvert == TITLECASE_FORCE)
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW((LPWSTR)strWToConvert[i]);
					//An exception
					if ((i < 2) ? false : (strWToConvert[i - 1] == L'\'' && ::IsCharAlphaW(strWToConvert[i - 2])))
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW((LPWSTR)strWToConvert[i]);
				}
			}
			break; 
		} //case TITLECASE
		case SENTENCECASE_FORCE:
		case SENTENCECASE_BLEND:
		{
			bool isNewSentence = true;
			bool wasEolR = false;
			bool wasEolN = false;
			for (int i = 0; i < nbChars; ++i)
			{
				if (::IsCharAlphaW(strWToConvert[i]))
				{
					if (isNewSentence)
					{
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW((LPWSTR)strWToConvert[i]);
						isNewSentence = false;
					}
					else if (caseToConvert == SENTENCECASE_FORCE)
					{
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW((LPWSTR)strWToConvert[i]);
					}
					wasEolR = false;
					wasEolN = false;
					//An exception
					if (strWToConvert[i] == L'i' &&
						((i < 1) ? false : (::iswspace(strWToConvert[i - 1]) || strWToConvert[i - 1] == L'(' || strWToConvert[i - 1] == L'"')) &&
						((i + 1 == nbChars) ? false : (::iswspace(strWToConvert[i + 1]) || strWToConvert[i + 1] == L'\'')))
					{
						strWToConvert[i] = L'I';
					}
				}
				else if (strWToConvert[i] == L'.' || strWToConvert[i] == L'!' || strWToConvert[i] == L'?')
				{
					if ((i + 1 == nbChars) ? true : ::IsCharAlphaNumericW(strWToConvert[i + 1]))
						isNewSentence = false;
					else
						isNewSentence = true;
				}
				else if (strWToConvert[i] == L'\r')
				{
					if (wasEolR)
						isNewSentence = true;
					else
						wasEolR = true;
				}
				else if (strWToConvert[i] == L'\n')
				{
					if (wasEolN)
						isNewSentence = true;
					else
						wasEolN = true;
				}
			}
			break;
		} //case SENTENCECASE
		case INVERTCASE:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				if (::IsCharLowerW(strWToConvert[i]))
					strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW((LPWSTR)strWToConvert[i]);
				else
					strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW((LPWSTR)strWToConvert[i]);
			}
			break; 
		} //case INVERTCASE
		case RANDOMCASE:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				if (::IsCharAlphaW(strWToConvert[i]))
				{
					if (std::rand() & true)
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW((LPWSTR)strWToConvert[i]);
					else
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW((LPWSTR)strWToConvert[i]);
				}
			}
			break; 
		} //case RANDOMCASE
	} //switch (caseToConvert)
}

void ScintillaEditView::convertSelectedTextTo(const TextCase & caseToConvert)
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

			changeCase(destStr, nbChar, caseToConvert);

			::WideCharToMultiByte(codepage, 0, destStr, len, srcStr, len, NULL, NULL);

			execute(SCI_SETTARGETRANGE, start, end);
			execute(SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(srcStr));

			delete [] srcStr;
			delete [] destStr;
		}

		setMultiSelections(cmi);

		execute(SCI_ENDUNDOACTION);
		return;
	}

	size_t selectionStart = execute(SCI_GETSELECTIONSTART);
	size_t selectionEnd = execute(SCI_GETSELECTIONEND);

	int32_t strLen = static_cast<int32_t>(selectionEnd - selectionStart);
	if (strLen)
	{
		int strSize = strLen + 1;
		char *selectedStr = new char[strSize];
		int strWSize = strSize * 2;
		wchar_t *selectedStrW = new wchar_t[strWSize+3];

		execute(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(selectedStr));

		int nbChar = ::MultiByteToWideChar(codepage, 0, selectedStr, strSize, selectedStrW, strWSize);

		changeCase(selectedStrW, nbChar, caseToConvert);

		::WideCharToMultiByte(codepage, 0, selectedStrW, strWSize, selectedStr, strSize, NULL, NULL);

		execute(SCI_SETTARGETRANGE, selectionStart, selectionEnd);
		execute(SCI_REPLACETARGET, strLen, reinterpret_cast<LPARAM>(selectedStr));
		execute(SCI_SETSEL, selectionStart, selectionEnd);
		delete [] selectedStr;
		delete [] selectedStrW;
	}
}



pair<int, int> ScintillaEditView::getWordRange()
{
    auto caretPos = execute(SCI_GETCURRENTPOS, 0, 0);
	int startPos = static_cast<int>(execute(SCI_WORDSTARTPOSITION, caretPos, true));
	int endPos = static_cast<int>(execute(SCI_WORDENDPOSITION, caretPos, true));
    return pair<int, int>(startPos, endPos);
}

bool ScintillaEditView::expandWordSelection()
{
    pair<int, int> wordRange = 	getWordRange();
    if (wordRange.first != wordRange.second)
	{
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
			wcscpy_s(str, strLen, j);
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
		int nbSel = static_cast<int32_t>(execute(SCI_GETSELECTIONS));

		for (int i = 0 ; i < nbSel ; ++i)
		{
			int absPosSelStartPerLine = static_cast<int32_t>(execute(SCI_GETSELECTIONNANCHOR, i));
			int absPosSelEndPerLine = static_cast<int32_t>(execute(SCI_GETSELECTIONNCARET, i));
			int nbVirtualAnchorSpc = static_cast<int32_t>(execute(SCI_GETSELECTIONNANCHORVIRTUALSPACE, i));
			int nbVirtualCaretSpc = static_cast<int32_t>(execute(SCI_GETSELECTIONNCARETVIRTUALSPACE, i));

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
					execute(SCI_INSERTTEXT, k, reinterpret_cast<LPARAM>(" "));
				}
				cmi[i]._selLpos += cmi[i]._nbVirtualAnchorSpc;
				cmi[i]._selRpos += cmi[i]._nbVirtualCaretSpc;
			}

			execute(SCI_SETTARGETRANGE, cmi[i]._selLpos, cmi[i]._selRpos);

			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
			const char *strA = wmc.wchar2char(str, cp);
			execute(SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(strA));

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

	// If there is no column mode info available, no need to do anything
	// If required a message can be shown to user, that select column properly or something similar
	// It is just a double check as taken in callee method (in case this method is called from multiple places)
	if (cmi.size() <= 0)
		return;
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
		const size_t kiMaxSize = cmi.size();
		while (numbers.size() < kiMaxSize)
		{
			for (int i = 0; i < repeat; i++)
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
					execute(SCI_INSERTTEXT, k, reinterpret_cast<LPARAM>(" "));
				}
				cmi[i]._selLpos += cmi[i]._nbVirtualAnchorSpc;
				cmi[i]._selRpos += cmi[i]._nbVirtualCaretSpc;
			}
			execute(SCI_SETTARGETRANGE, cmi[i]._selLpos, cmi[i]._selRpos);

			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			UINT cp = static_cast<UINT>(execute(SCI_GETCODEPAGE));
			const char *strA = wmc.wchar2char(str, cp);
			execute(SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(strA));

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


void ScintillaEditView::foldChanged(size_t line, int levelNow, int levelPrev)
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
		int parentLine = static_cast<int32_t>(execute(SCI_GETFOLDPARENT, line));
		if ((parentLine < 0) || !isFolded(parentLine && execute(SCI_GETLINEVISIBLE, parentLine)))
			execute(SCI_SHOWLINES, line, line);
	}
}


void ScintillaEditView::scrollPosToCenter(size_t pos)
{
	_positionRestoreNeeded = false;

	execute(SCI_GOTOPOS, pos);
	int line = static_cast<int32_t>(execute(SCI_LINEFROMPOSITION, pos));

	int firstVisibleDisplayLine = static_cast<int32_t>(execute(SCI_GETFIRSTVISIBLELINE));
	int firstVisibleDocLine = static_cast<int32_t>(execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine));
	int nbLine = static_cast<int32_t>(execute(SCI_LINESONSCREEN, firstVisibleDisplayLine));
	int lastVisibleDocLine = static_cast<int32_t>(execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine + nbLine));

	int middleLine;
	if (line - firstVisibleDocLine < lastVisibleDocLine - line)
		middleLine = firstVisibleDocLine + nbLine/2;
	else
		middleLine = lastVisibleDocLine -  nbLine/2;
	int nbLines2scroll =  line - middleLine;
	scroll(0, nbLines2scroll);
}

void ScintillaEditView::hideLines()
{
	//Folding can screw up hide lines badly if it unfolds a hidden section.
	//Adding runMarkers(hide, foldstart) directly (folding on single document) can help

	//Special func on buffer. If markers are added, create notification with location of start, and hide bool set to true
	int startLine = static_cast<int32_t>(execute(SCI_LINEFROMPOSITION, execute(SCI_GETSELECTIONSTART)));
	int endLine = static_cast<int32_t>(execute(SCI_LINEFROMPOSITION, execute(SCI_GETSELECTIONEND)));
	//perform range check: cannot hide very first and very last lines
	//Offset them one off the edges, and then check if they are within the reasonable
	int nbLines = static_cast<int32_t>(execute(SCI_GETLINECOUNT));
	if (nbLines < 3)
		return;	//cannot possibly hide anything
	if (!startLine)
		++startLine;
	if (endLine == (nbLines-1))
		--endLine;

	if (startLine > endLine)
		return;	//tried to hide line at edge

	//Hide the lines. We add marks on the outside of the hidden section and hide the lines
	//execute(SCI_HIDELINES, startLine, endLine);
	//Add markers
	execute(SCI_MARKERADD, startLine-1, MARK_HIDELINESBEGIN);
	execute(SCI_MARKERADD, startLine-1, MARK_HIDELINESUNDERLINE);
	execute(SCI_MARKERADD, endLine+1, MARK_HIDELINESEND);

	//remove any markers in between
	int scope = 0;
	for (int i = startLine; i <= endLine; ++i)
	{
		auto state = execute(SCI_MARKERGET, i);
		bool closePresent = ((state & (1 << MARK_HIDELINESEND)) != 0);	//check close first, then open, since close closes scope
		bool openPresent = ((state & (1 << MARK_HIDELINESBEGIN | 1 << MARK_HIDELINESUNDERLINE)) != 0);
		if (closePresent)
		{
			execute(SCI_MARKERDELETE, i, MARK_HIDELINESEND);
			if (scope > 0) scope--;
		}

		if (openPresent)
		{
			execute(SCI_MARKERDELETE, i, MARK_HIDELINESBEGIN);
			execute(SCI_MARKERDELETE, i, MARK_HIDELINESUNDERLINE);
			++scope;
		}
	}
	if (scope != 0)
	{	//something went wrong
		//Someone managed to make overlapping hidelines sections.
		//We cant do anything since this isnt supposed to happen
	}

	_currentBuffer->setHideLineChanged(true, startLine-1);
}

bool ScintillaEditView::markerMarginClick(int lineNumber)
{
	auto state = execute(SCI_MARKERGET, lineNumber);
	bool openPresent = ((state & (1 << MARK_HIDELINESBEGIN | 1 << MARK_HIDELINESUNDERLINE)) != 0);
	bool closePresent = ((state & (1 << MARK_HIDELINESEND)) != 0);

	if (!openPresent && !closePresent)
		return false;

	//Special func on buffer. First call show with location of opening marker. Then remove the marker manually
	if (openPresent)
	{
		_currentBuffer->setHideLineChanged(false, lineNumber);
	}

	if (closePresent)
	{
		openPresent = false;
		for (lineNumber--; lineNumber >= 0 && !openPresent; lineNumber--)
		{
			state = execute(SCI_MARKERGET, lineNumber);
			openPresent = ((state & (1 << MARK_HIDELINESBEGIN | 1 << MARK_HIDELINESUNDERLINE)) != 0);
		}

		if (openPresent)
		{
			_currentBuffer->setHideLineChanged(false, lineNumber);
		}
	}

	return true;
}

void ScintillaEditView::notifyMarkers(Buffer * buf, bool isHide, int location, bool del)
{
	if (buf != _currentBuffer)	//if not visible buffer dont do a thing
		return;
	runMarkers(isHide, location, false, del);
}

//Run through full document. When switching in or opening folding
//hide is false only when user click on margin
void ScintillaEditView::runMarkers(bool doHide, size_t searchStart, bool endOfDoc, bool doDelete)
{
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
	size_t maxLines = execute(SCI_GETLINECOUNT);
	if (doHide)
	{
		auto startHiding = searchStart;
		bool isInSection = false;
		for (auto i = searchStart; i < maxLines; ++i)
		{
			auto state = execute(SCI_MARKERGET, i);
			if ( ((state & (1 << MARK_HIDELINESEND)) != 0) )
			{
				if (isInSection)
				{
					execute(SCI_HIDELINES, startHiding, i-1);
					if (!endOfDoc)
					{
						return;	//done, only single section requested
					}	//otherwise keep going
				}
				isInSection = false;
			}
			if ( ((state & (1 << MARK_HIDELINESBEGIN | 1 << MARK_HIDELINESUNDERLINE)) != 0) )
			{
				isInSection = true;
				startHiding = i+1;
			}

		}
	}
	else
	{
		auto startShowing = searchStart;
		bool isInSection = false;
		for (auto i = searchStart; i < maxLines; ++i)
		{
			auto state = execute(SCI_MARKERGET, i);
			if ( ((state & (1 << MARK_HIDELINESEND)) != 0) )
			{
				if (doDelete)
				{
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESEND);
					if (!endOfDoc)
					{
						return;	//done, only single section requested
					}	//otherwise keep going
				}
				 else if (isInSection)
				 {
					if (startShowing >= i)
					{	//because of fold skipping, we passed the close tag. In that case we cant do anything
						if (!endOfDoc)
						{
							return;
						}
						else
						{
							continue;
						}
					}
					execute(SCI_SHOWLINES, startShowing, i-1);
					if (!endOfDoc)
					{
						return;	//done, only single section requested
					}	//otherwise keep going
					isInSection = false;
				}
			}
			if ( ((state & (1 << MARK_HIDELINESBEGIN | 1 << MARK_HIDELINESUNDERLINE)) != 0) )
			{
				if (doDelete)
				{
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESBEGIN);
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESUNDERLINE);
				}
				else
				{
					isInSection = true;
					startShowing = i+1;
				}
			}

			auto levelLine = execute(SCI_GETFOLDLEVEL, i, 0);
			if (levelLine & SC_FOLDLEVELHEADERFLAG)
			{	//fold section. Dont show lines if fold is closed
				if (isInSection && !isFolded(i))
				{
					execute(SCI_SHOWLINES, startShowing, i);
					//startShowing = execute(SCI_GETLASTCHILD, i, (levelLine & SC_FOLDLEVELNUMBERMASK));
				}
			}
		}
	}
}


void ScintillaEditView::setTabSettings(Lang *lang)
{
    if (lang && lang->_tabSize != -1 && lang->_tabSize != 0)
    {
		if (lang->_langID == L_JAVASCRIPT)
		{
			Lang *ljs = NppParameters::getInstance().getLangFromID(L_JS);
			execute(SCI_SETTABWIDTH, ljs->_tabSize > 0 ? ljs->_tabSize : lang->_tabSize);
			execute(SCI_SETUSETABS, !ljs->_isTabReplacedBySpace);
			return;
		}
        execute(SCI_SETTABWIDTH, lang->_tabSize);
        execute(SCI_SETUSETABS, !lang->_isTabReplacedBySpace);
    }
    else
    {
        const NppGUI & nppgui = NppParameters::getInstance().getNppGUI();
        execute(SCI_SETTABWIDTH, nppgui._tabSize  > 0 ? nppgui._tabSize : lang->_tabSize);
		execute(SCI_SETUSETABS, !nppgui._tabReplacedBySpace);
    }
}

void ScintillaEditView::insertNewLineAboveCurrentLine()
{
	generic_string newline = getEOLString();
	const auto current_line = getCurrentLineNumber();
	if (current_line == 0)
	{
		// Special handling if caret is at first line.
		insertGenericTextFrom(0, newline.c_str());
	}
	else
	{
		const auto eol_length = newline.length();
		const auto position = static_cast<size_t>(execute(SCI_POSITIONFROMLINE, current_line)) - eol_length;
		insertGenericTextFrom(position, newline.c_str());
	}
	execute(SCI_SETEMPTYSELECTION, execute(SCI_POSITIONFROMLINE, current_line));
}


void ScintillaEditView::insertNewLineBelowCurrentLine()
{
	generic_string newline = getEOLString();
	const auto line_count = static_cast<size_t>(execute(SCI_GETLINECOUNT));
	const auto current_line = getCurrentLineNumber();
	if (current_line == line_count - 1)
	{
		// Special handling if caret is at last line.
		appandGenericText(newline.c_str());
	}
	else
	{
		const auto eol_length = newline.length();
		const auto position = eol_length + execute(SCI_GETLINEENDPOSITION, current_line);
		insertGenericTextFrom(position, newline.c_str());
	}
	execute(SCI_SETEMPTYSELECTION, execute(SCI_POSITIONFROMLINE, current_line + 1));
}

void ScintillaEditView::sortLines(size_t fromLine, size_t toLine, ISorter* pSort)
{
	if (fromLine >= toLine)
	{
		return;
	}

	const auto startPos = execute(SCI_POSITIONFROMLINE, fromLine);
	const auto endPos = execute(SCI_POSITIONFROMLINE, toLine) + execute(SCI_LINELENGTH, toLine);
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
	generic_string joined = stringJoin(sortedText, getEOLString());
	if (sortEntireDocument)
	{
		assert(joined.length() == text.length());
	}
	else
	{
		assert(joined.length() + getEOLString().length() == text.length());
		joined += getEOLString();
	}
	if (text != joined)
	{
		replaceTarget(joined.c_str(), int(startPos), int(endPos));
	}
}

bool ScintillaEditView::isTextDirectionRTL() const
{
	long exStyle = static_cast<long>(::GetWindowLongPtr(_hSelf, GWL_EXSTYLE));
	return (exStyle & WS_EX_LAYOUTRTL) != 0;
}

void ScintillaEditView::changeTextDirection(bool isRTL)
{
	long exStyle = static_cast<long>(::GetWindowLongPtr(_hSelf, GWL_EXSTYLE));
	exStyle = isRTL ? exStyle | WS_EX_LAYOUTRTL : exStyle&(~WS_EX_LAYOUTRTL);
	::SetWindowLongPtr(_hSelf, GWL_EXSTYLE, exStyle);
}

generic_string ScintillaEditView::getEOLString()
{
	const int eol_mode = int(execute(SCI_GETEOLMODE));
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

void ScintillaEditView::setBorderEdge(bool doWithBorderEdge)
{
	long exStyle = static_cast<long>(::GetWindowLongPtr(_hSelf, GWL_EXSTYLE));

	if (doWithBorderEdge)
		exStyle |= WS_EX_CLIENTEDGE;
	else
		exStyle &= ~WS_EX_CLIENTEDGE;

	::SetWindowLongPtr(_hSelf, GWL_EXSTYLE, exStyle);
	::SetWindowPos(_hSelf, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void ScintillaEditView::getFoldColor(COLORREF& fgColor, COLORREF& bgColor, COLORREF& activeFgColor)
{
	StyleArray & stylers = NppParameters::getInstance().getMiscStylerArray();

	int i = stylers.getStylerIndexByName(TEXT("Fold"));
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		fgColor = style._bgColor;
		bgColor = style._fgColor;
	}

	i = stylers.getStylerIndexByName(TEXT("Fold active"));
	if (i != -1)
	{
		Style & style = stylers.getStyler(i);
		activeFgColor = style._fgColor;
	}
}

pair<int, int> ScintillaEditView::getSelectedCharsAndLinesCount(int maxSelectionsForLineCount /* = -1 */) const
{
	pair<int, int> selectedCharsAndLines(0, 0);

	selectedCharsAndLines.first = getUnicodeSelectedLength();

	int numSelections = static_cast<int>(execute(SCI_GETSELECTIONS));

	if (numSelections == 1)
	{
		pair<int, int> lineRange = getSelectionLinesRange();
		selectedCharsAndLines.second = lineRange.second - lineRange.first + 1;
	}
	else if (execute(SCI_SELECTIONISRECTANGLE))
	{
		selectedCharsAndLines.second = numSelections;
	}
	else if ((maxSelectionsForLineCount == -1) ||  // -1 means process ALL of the selections
		(numSelections <= maxSelectionsForLineCount))
	{
		// selections are obtained from Scintilla in the order user creates them,
		// not in a lowest-to-highest position-based order;
		// to be able to get a line-count that can't count the same line more than once,
		// we have to reorder the lines touched
		// by selection into low-to-high line number order before processing them further

		vector< pair <int, int> > v;
		for (int s = 0; s < numSelections; ++s)
		{
			v.push_back(getSelectionLinesRange(s));
		}
		sort(v.begin(), v.end());
		int previousSecondLine = -1;
		for (auto lineRange : v)
		{
			selectedCharsAndLines.second += lineRange.second - lineRange.first;
			if (lineRange.first != previousSecondLine)
			{
				++selectedCharsAndLines.second;
			}
			previousSecondLine = lineRange.second;
		}
	}

	return selectedCharsAndLines;
};

int ScintillaEditView::getUnicodeSelectedLength() const
{
	int length = 0;
	int numSelections = static_cast<int>(execute(SCI_GETSELECTIONS));

	for (int s = 0; s < numSelections; ++s)
	{
		int start = static_cast<int>(execute(SCI_GETSELECTIONNSTART, s));
		int end = static_cast<int>(execute(SCI_GETSELECTIONNEND, s));
		length += static_cast<int>(execute(SCI_COUNTCHARACTERS, start, end));
	}

	return length;
};


void ScintillaEditView::markedTextToClipboard(int indiStyle, bool doAll /*= false*/)
{
	int styleIndicators[] =
	{
		SCE_UNIVERSAL_FOUND_STYLE_EXT1,
		SCE_UNIVERSAL_FOUND_STYLE_EXT2,
		SCE_UNIVERSAL_FOUND_STYLE_EXT3,
		SCE_UNIVERSAL_FOUND_STYLE_EXT4,
		SCE_UNIVERSAL_FOUND_STYLE_EXT5,
		-1  // end signifier
	};

	if (!doAll)
	{
		styleIndicators[0] = indiStyle;
		styleIndicators[1] = -1;
	}

	// vector of pairs: starting position of styled text, and styled text
	std::vector<std::pair<int, generic_string>> styledVect;

	const generic_string cr = TEXT("\r");
	const generic_string lf = TEXT("\n");

	bool textContainsLineEndingChar = false;

	for (int si = 0; styleIndicators[si] != -1; ++si)
	{
		int pos = static_cast<int>(execute(SCI_INDICATOREND, styleIndicators[si], 0));
		if (pos > 0)
		{
			bool atEndOfIndic = execute(SCI_INDICATORVALUEAT, styleIndicators[si], 0) != 0;
			int prevPos = pos;
			if (atEndOfIndic) prevPos = 0;

			do
			{
				if (atEndOfIndic)
				{
					generic_string styledText = getGenericTextAsString(prevPos, pos);
					if (!textContainsLineEndingChar)
					{
						if (styledText.find(cr) != std::string::npos ||
							styledText.find(lf) != std::string::npos)
						{
							textContainsLineEndingChar = true;
						}
					}
					styledVect.push_back(::make_pair(prevPos, styledText));
				}
				atEndOfIndic = !atEndOfIndic;
				prevPos = pos;
				pos = static_cast<int>(execute(SCI_INDICATOREND, styleIndicators[si], pos));
			} while (pos != prevPos);
		}
	}

	if (styledVect.size() > 0)
	{
		if (doAll)
		{
			// sort by starting position of styled text
			std::sort(styledVect.begin(), styledVect.end());
		}

		const generic_string delim =
			(textContainsLineEndingChar && styledVect.size() > 1) ?
			TEXT("\r\n----\r\n") : TEXT("\r\n");

		generic_string joined;
		for (auto item : styledVect)
		{
			joined += delim + item.second;
		}
		joined = joined.substr(delim.length());
		if (styledVect.size() > 1)
		{
			joined += TEXT("\r\n");
		}

		str2Clipboard(joined, NULL);
	}
}
