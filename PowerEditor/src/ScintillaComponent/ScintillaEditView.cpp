// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <memory>
#include <shlwapi.h>
#include <cinttypes>
#include <windowsx.h>
#include "ScintillaEditView.h"
#include "Parameters.h"
#include "Sorters.h"
#include "verifySignedfile.h"
#include "ILexer.h"
#include "Lexilla.h"

using namespace std;

// initialize the static variable
bool ScintillaEditView::_SciInit = false;
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
LanguageNameInfo ScintillaEditView::_langNameInfoArray[L_EXTERNAL + 1] = {
	//
	// _langName			_shortName					_longName												_langID			_lexerID
	//
	{TEXT("normal"),		TEXT("Normal text"),		TEXT("Normal text file"),								L_TEXT,			"null"},
	{TEXT("php"),			TEXT("PHP"),				TEXT("PHP Hypertext Preprocessor file"),				L_PHP,			"phpscript"},
	{TEXT("c"),				TEXT("C"),					TEXT("C source file"),									L_C,			"cpp"},
	{TEXT("cpp"),			TEXT("C++"),				TEXT("C++ source file"),								L_CPP,			"cpp"},
	{TEXT("cs"),			TEXT("C#"),					TEXT("C# source file"),									L_CS,			"cpp"},
	{TEXT("objc"),			TEXT("Objective-C"),		TEXT("Objective-C source file"),						L_OBJC,			"objc"},
	{TEXT("java"),			TEXT("Java"),				TEXT("Java source file"),								L_JAVA,			"cpp"},
	{TEXT("rc"),			TEXT("RC"),					TEXT("Windows Resource file"),							L_RC,			"cpp"},
	{TEXT("html"),			TEXT("HTML"),				TEXT("Hyper Text Markup Language file"),				L_HTML,			"hypertext"},
	{TEXT("xml"),			TEXT("XML"),				TEXT("eXtensible Markup Language file"),				L_XML,			"xml"},
	{TEXT("makefile"),		TEXT("Makefile"),			TEXT("Makefile"),										L_MAKEFILE,		"makefile"},
	{TEXT("pascal"),		TEXT("Pascal"),				TEXT("Pascal source file"),								L_PASCAL,		"pascal"},
	{TEXT("batch"),			TEXT("Batch"),				TEXT("Batch file"),										L_BATCH,		"batch"},
	{TEXT("ini"),			TEXT("ini"),				TEXT("MS ini file"),									L_INI,			"props"},
	{TEXT("nfo"),			TEXT("NFO"),				TEXT("MSDOS Style/ASCII Art"),							L_ASCII,		"null"},
	{TEXT("udf"),			TEXT("udf"),				TEXT("User Defined language file"),						L_USER,			"user"},
	{TEXT("asp"),			TEXT("ASP"),				TEXT("Active Server Pages script file"),				L_ASP,			"hypertext"},
	{TEXT("sql"),			TEXT("SQL"),				TEXT("Structured Query Language file"),					L_SQL,			"sql"},
	{TEXT("vb"),			TEXT("Visual Basic"),		TEXT("Visual Basic file"),								L_VB,			"vb"},
	{TEXT("javascript"),	TEXT("JavaScript"),			TEXT("JavaScript file"),								L_JS,			"cpp"},
	{TEXT("css"),			TEXT("CSS"),				TEXT("Cascade Style Sheets File"),						L_CSS,			"css"},
	{TEXT("perl"),			TEXT("Perl"),				TEXT("Perl source file"),								L_PERL,			"perl"},
	{TEXT("python"),		TEXT("Python"),				TEXT("Python file"),									L_PYTHON,		"python"},
	{TEXT("lua"),			TEXT("Lua"),				TEXT("Lua source File"),								L_LUA,			"lua"},
	{TEXT("tex"),			TEXT("TeX"),				TEXT("TeX file"),										L_TEX,			"tex"},
	{TEXT("fortran"),		TEXT("Fortran free form"),	TEXT("Fortran free form source file"),					L_FORTRAN,		"fortran"},
	{TEXT("bash"),			TEXT("Shell"),				TEXT("Unix script file"),								L_BASH,			"bash"},
	{TEXT("actionscript"),	TEXT("ActionScript"),		TEXT("Flash ActionScript file"),						L_FLASH,		"cpp"},
	{TEXT("nsis"),			TEXT("NSIS"),				TEXT("Nullsoft Scriptable Install System script file"),	L_NSIS,			"nsis"},
	{TEXT("tcl"),			TEXT("TCL"),				TEXT("Tool Command Language file"),						L_TCL,			"tcl"},
	{TEXT("lisp"),			TEXT("Lisp"),				TEXT("List Processing language file"),					L_LISP,			"lisp"},
	{TEXT("scheme"),		TEXT("Scheme"),				TEXT("Scheme file"),									L_SCHEME,		"lisp"},
	{TEXT("asm"),			TEXT("Assembly"),			TEXT("Assembly language source file"),					L_ASM,			"asm"},
	{TEXT("diff"),			TEXT("Diff"),				TEXT("Diff file"),										L_DIFF,			"diff"},
	{TEXT("props"),			TEXT("Properties file"),	TEXT("Properties file"),								L_PROPS,		"props"},
	{TEXT("postscript"),	TEXT("PostScript"),			TEXT("PostScript file"),								L_PS,			"ps"},
	{TEXT("ruby"),			TEXT("Ruby"),				TEXT("Ruby file"),										L_RUBY,			"ruby"},
	{TEXT("smalltalk"),		TEXT("Smalltalk"),			TEXT("Smalltalk file"),									L_SMALLTALK,	"smalltalk"},
	{TEXT("vhdl"),			TEXT("VHDL"),				TEXT("VHSIC Hardware Description Language file"),		L_VHDL,			"vhdl"},
	{TEXT("kix"),			TEXT("KiXtart"),			TEXT("KiXtart file"),									L_KIX,			"kix"},
	{TEXT("autoit"),		TEXT("AutoIt"),				TEXT("AutoIt"),											L_AU3,			"au3"},
	{TEXT("caml"),			TEXT("CAML"),				TEXT("Categorical Abstract Machine Language"),			L_CAML,			"caml"},
	{TEXT("ada"),			TEXT("Ada"),				TEXT("Ada file"),										L_ADA,			"ada"},
	{TEXT("verilog"),		TEXT("Verilog"),			TEXT("Verilog file"),									L_VERILOG,		"verilog"},
	{TEXT("matlab"),		TEXT("MATLAB"),				TEXT("MATrix LABoratory"),								L_MATLAB,		"matlab"},
	{TEXT("haskell"),		TEXT("Haskell"),			TEXT("Haskell"),										L_HASKELL,		"haskell"},
	{TEXT("inno"),			TEXT("Inno Setup"),			TEXT("Inno Setup script"),								L_INNO,			"inno"},
	{TEXT("searchResult"),	TEXT("Internal Search"),	TEXT("Internal Search"),								L_SEARCHRESULT,	"searchResult"},
	{TEXT("cmake"),			TEXT("CMake"),				TEXT("CMake file"),										L_CMAKE,		"cmake"},
	{TEXT("yaml"),			TEXT("YAML"),				TEXT("YAML Ain't Markup Language"),						L_YAML,			"yaml"},
	{TEXT("cobol"),			TEXT("COBOL"),				TEXT("COmmon Business Oriented Language"),				L_COBOL,		"COBOL"},
	{TEXT("gui4cli"),		TEXT("Gui4Cli"),			TEXT("Gui4Cli file"),									L_GUI4CLI,		"gui4cli"},
	{TEXT("d"),				TEXT("D"),					TEXT("D programming language"),							L_D,			"d"},
	{TEXT("powershell"),	TEXT("PowerShell"),			TEXT("Windows PowerShell"),								L_POWERSHELL,	"powershell"},
	{TEXT("r"),				TEXT("R"),					TEXT("R programming language"),							L_R,			"r"},
	{TEXT("jsp"),			TEXT("JSP"),				TEXT("JavaServer Pages script file"),					L_JSP,			"hypertext"},
	{TEXT("coffeescript"),	TEXT("CoffeeScript"),		TEXT("CoffeeScript file"),								L_COFFEESCRIPT,	"coffeescript"},
	{TEXT("json"),			TEXT("json"),				TEXT("JSON file"),										L_JSON,			"json"},
	{TEXT("javascript.js"), TEXT("JavaScript"),			TEXT("JavaScript file"),								L_JAVASCRIPT,	"cpp"},
	{TEXT("fortran77"),		TEXT("Fortran fixed form"),	TEXT("Fortran fixed form source file"),					L_FORTRAN_77,	"f77"},
	{TEXT("baanc"),			TEXT("BaanC"),				TEXT("BaanC File"),										L_BAANC,		"baan"},
	{TEXT("srec"),			TEXT("S-Record"),			TEXT("Motorola S-Record binary data"),					L_SREC,			"srec"},
	{TEXT("ihex"),			TEXT("Intel HEX"),			TEXT("Intel HEX binary data"),							L_IHEX,			"ihex"},
	{TEXT("tehex"),			TEXT("Tektronix extended HEX"),	TEXT("Tektronix extended HEX binary data"),			L_TEHEX,		"tehex"},
	{TEXT("swift"),			TEXT("Swift"),              TEXT("Swift file"),										L_SWIFT,		"cpp"},
	{TEXT("asn1"),			TEXT("ASN.1"),				TEXT("Abstract Syntax Notation One file"),				L_ASN1,			"asn1"},
	{TEXT("avs"),			TEXT("AviSynth"),			TEXT("AviSynth scripts files"),							L_AVS,			"avs"},
	{TEXT("blitzbasic"),	TEXT("BlitzBasic"),			TEXT("BlitzBasic file"),								L_BLITZBASIC,	"blitzbasic"},
	{TEXT("purebasic"),		TEXT("PureBasic"),			TEXT("PureBasic file"),									L_PUREBASIC,	"purebasic"},
	{TEXT("freebasic"),		TEXT("FreeBasic"),			TEXT("FreeBasic file"),									L_FREEBASIC,	"freebasic"},
	{TEXT("csound"),		TEXT("Csound"),				TEXT("Csound file"),									L_CSOUND,		"csound"},
	{TEXT("erlang"),		TEXT("Erlang"),				TEXT("Erlang file"),									L_ERLANG,		"erlang"},
	{TEXT("escript"),		TEXT("ESCRIPT"),			TEXT("ESCRIPT file"),									L_ESCRIPT,		"escript"},
	{TEXT("forth"),			TEXT("Forth"),				TEXT("Forth file"),										L_FORTH,		"forth"},
	{TEXT("latex"),			TEXT("LaTeX"),				TEXT("LaTeX file"),										L_LATEX,		"latex"},
	{TEXT("mmixal"),		TEXT("MMIXAL"),				TEXT("MMIXAL file"),									L_MMIXAL,		"mmixal"},
	{TEXT("nim"),			TEXT("Nim"),				TEXT("Nim file"),										L_NIM,			"nimrod"},
	{TEXT("nncrontab"),		TEXT("Nncrontab"),			TEXT("extended crontab file"),							L_NNCRONTAB,	"nncrontab"},
	{TEXT("oscript"),		TEXT("OScript"),			TEXT("OScript source file"),							L_OSCRIPT,		"oscript"},
	{TEXT("rebol"),			TEXT("REBOL"),				TEXT("REBOL file"),										L_REBOL,		"rebol"},
	{TEXT("registry"),		TEXT("registry"),			TEXT("registry file"),									L_REGISTRY,		"registry"},
	{TEXT("rust"),			TEXT("Rust"),				TEXT("Rust file"),										L_RUST,			"rust"},
	{TEXT("spice"),			TEXT("Spice"),				TEXT("spice file"),										L_SPICE,		"spice"},
	{TEXT("txt2tags"),		TEXT("txt2tags"),			TEXT("txt2tags file"),									L_TXT2TAGS,		"txt2tags"},
	{TEXT("visualprolog"),	TEXT("Visual Prolog"),		TEXT("Visual Prolog file"),								L_VISUALPROLOG,	"visualprolog"},
	{TEXT("typescript"),	TEXT("TypeScript"),			TEXT("TypeScript file"),								L_TYPESCRIPT,	"cpp"},
	{TEXT("ext"),			TEXT("External"),			TEXT("External"),										L_EXTERNAL,		"null"}
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
	if (!_SciInit)
	{
		if (!Scintilla_RegisterClasses(hInst))
		{
			throw std::runtime_error("ScintillaEditView::init : SCINTILLA ERROR - Scintilla_RegisterClasses failed");
		}
		_SciInit = true;
	}

	Window::init(hInst, hPere);
	_hSelf = ::CreateWindowEx(
					0,\
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

	NppDarkMode::setDarkScrollBar(_hSelf);

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
		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::setDarkScrollBar(_hSelf);
			return TRUE;
		}

		case WM_MOUSEHWHEEL :
		{
			::CallWindowProc(_scintillaDefaultProc, hwnd, WM_HSCROLL, ((short)HIWORD(wParam) > 0)?SB_LINERIGHT:SB_LINELEFT, 0);
			return TRUE;
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
				::CallWindowProc(_scintillaDefaultProc, hwnd, WM_HSCROLL, ((short)HIWORD(wParam) < 0) ? SB_LINERIGHT : SB_LINELEFT, 0);
				::CallWindowProc(_scintillaDefaultProc, hwnd, WM_HSCROLL, ((short)HIWORD(wParam) < 0) ? SB_LINERIGHT : SB_LINELEFT, 0);
				::CallWindowProc(_scintillaDefaultProc, hwnd, WM_HSCROLL, ((short)HIWORD(wParam) < 0) ? SB_LINERIGHT : SB_LINELEFT, 0);
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
				intptr_t					textLength;
				intptr_t					selectSize;
				char				smallTextBuffer[128];
				char			  *	selectedStr = smallTextBuffer;
				RECONVERTSTRING   *	reconvert = (RECONVERTSTRING *)lParam;

				// does nothing with a rectangular selection
				if (execute(SCI_SELECTIONISRECTANGLE, 0, 0))
					return 0;

				// get the codepage of the text

				size_t cp = execute(SCI_GETCODEPAGE);
				UINT codepage = static_cast<UINT>(cp);

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

				if (static_cast<size_t>(selectSize + 1) > sizeof(smallTextBuffer))
					selectedStr = new char[selectSize + 1];
				getText(selectedStr, range.cpMin, range.cpMax);

				if (reconvert == NULL)
				{
					// convert the selection to Unicode, and get the number
					// of bytes required for the converted text
					textLength = sizeof(WCHAR) * ::MultiByteToWideChar(codepage, 0, selectedStr, (int)selectSize, NULL, 0);
				}
				else
				{
					// convert the selection to Unicode, and store it at the end of the structure.
					// Beware: For a Unicode IME, dwStrLen , dwCompStrLen, and dwTargetStrLen
					// are TCHAR values, that is, character counts. The members dwStrOffset,
					// dwCompStrOffset, and dwTargetStrOffset specify byte counts.

					textLength = ::MultiByteToWideChar(	codepage, 0,
														selectedStr, (int)selectSize,
														(LPWSTR)((LPSTR)reconvert + sizeof(RECONVERTSTRING)),
														reconvert->dwSize - sizeof(RECONVERTSTRING));

					// fill the structure
					reconvert->dwVersion		 = 0;
					reconvert->dwStrLen			 = static_cast<DWORD>(textLength);
					reconvert->dwStrOffset		 = sizeof(RECONVERTSTRING);
					reconvert->dwCompStrLen		 = static_cast<DWORD>(textLength);
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
				size_t clickX = GET_X_LPARAM(lParam);
				size_t marginX = execute(SCI_POINTXFROMPOSITION, 0, 0);
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

    if (!styleToSet._fontName.empty())
	{
		if (!NppParameters::getInstance().isInFontList(styleToSet._fontName))
		{
			execute(SCI_STYLESETFONT, styleID, reinterpret_cast<LPARAM>(DEFAULT_FONT_NAME));
		}
		else
		{
			std::string fontNameA = wstring2string(styleToSet._fontName, CP_UTF8);
			execute(SCI_STYLESETFONT, styleID, reinterpret_cast<LPARAM>(fontNameA.c_str()));
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
		const Style * pStyle = NppParameters::getInstance().getMiscStylerArray().findByName(TEXT("Global override"));
		if (pStyle)
		{
			if (go.enableFg)
			{
				if (pStyle->_colorStyle & COLORSTYLE_FOREGROUND)
				{
					styleToSet._colorStyle |= COLORSTYLE_FOREGROUND;
					styleToSet._fgColor = pStyle->_fgColor;
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
				if (pStyle->_colorStyle & COLORSTYLE_BACKGROUND)
				{
					styleToSet._colorStyle |= COLORSTYLE_BACKGROUND;
					styleToSet._bgColor = pStyle->_bgColor;
				}
				else
				{
					if (styleToSet._styleID == STYLE_DEFAULT) 	//if global is set to transparent, use default style color
						styleToSet._colorStyle |= COLORSTYLE_BACKGROUND;
					else
						styleToSet._colorStyle &= ~COLORSTYLE_BACKGROUND;
				}
			}
			if (go.enableFont && !pStyle->_fontName.empty())
				styleToSet._fontName = pStyle->_fontName;
			if (go.enableFontSize && (pStyle->_fontSize > 0))
				styleToSet._fontSize = pStyle->_fontSize;

			if (pStyle->_fontStyle != STYLE_NOT_USED)
			{
				if (go.enableBold)
				{
					if (pStyle->_fontStyle & FONTSTYLE_BOLD)
						styleToSet._fontStyle |= FONTSTYLE_BOLD;
					else
						styleToSet._fontStyle &= ~FONTSTYLE_BOLD;
				}
				if (go.enableItalic)
				{
					if (pStyle->_fontStyle & FONTSTYLE_ITALIC)
						styleToSet._fontStyle |= FONTSTYLE_ITALIC;
					else
						styleToSet._fontStyle &= ~FONTSTYLE_ITALIC;
				}
				if (go.enableUnderLine)
				{
					if (pStyle->_fontStyle & FONTSTYLE_UNDERLINE)
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
		setLexerFromLangID(L_XML);
		
		for (int i = 0 ; i < 4 ; ++i)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(TEXT("")));

        makeStyle(type);

		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.xml.allow.scripts"), reinterpret_cast<LPARAM>("0"));
	}
	else if ((type == L_HTML) || (type == L_PHP) || (type == L_ASP) || (type == L_JSP))
	{
		setLexerFromLangID(L_HTML);
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
	setLexerFromLangID(L_JSON);

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
	setLexerFromLangID(L_USER);

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

	char* temp = new char[max_char];
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
	delete[] temp;

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

	for (const Style & style : userLangContainer->_styles)
	{
		if (style._styleID == STYLE_NOT_USED)
			continue;

		char nestingBuffer[32];
		sprintf(nestingBuffer, "userDefine.nesting.%02d", style._styleID);
		sprintf(intBuffer, "%d", style._nesting);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>(nestingBuffer), reinterpret_cast<LPARAM>(intBuffer));

		setStyle(style);
	}
}

void ScintillaEditView::setExternalLexer(LangType typeDoc)
{
	int id = typeDoc - L_EXTERNAL;

	ExternalLangContainer& externalLexer = NppParameters::getInstance().getELCFromIndex(id);
	if (!externalLexer.fnCL)
		return;
	ILexer5* iLex5 = externalLexer.fnCL(externalLexer._name.c_str());
	if (!iLex5)
		return;
	execute(SCI_SETILEXER, 0, reinterpret_cast<LPARAM>(iLex5));

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	const wchar_t* lexerNameW = wmc.char2wchar(externalLexer._name.c_str(), CP_ACP);
	LexerStyler *pStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(lexerNameW);
	if (pStyler)
	{
		for (const Style & style : *pStyler)
		{
			setStyle(style);

			if (style._keywordClass >= 0 && style._keywordClass <= KEYWORDSET_MAX)
			{
				basic_string<char> keywordList("");
				if (!style._keywords.empty())
				{
					keywordList = wstring2string(style._keywords, CP_ACP);
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

    setLexerFromLangID(L_CPP);

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
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.cpp.comment.explicit"), reinterpret_cast<LPARAM>("0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));

	// Disable track preprocessor to avoid incorrect detection.
	// In the most of cases, the symbols are defined outside of file.
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.track.preprocessor"), reinterpret_cast<LPARAM>("0"));
}

void ScintillaEditView::setJsLexer()
{
	const TCHAR *doxygenKeyWords = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);

	setLexerFromLangID(L_JAVASCRIPT);
	const TCHAR *pKwArray[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	makeStyle(L_JAVASCRIPT, pKwArray);

	if (doxygenKeyWords)
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char * doxygenKeyWords_char = wmc.wchar2char(doxygenKeyWords, CP_ACP);
		execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(doxygenKeyWords_char));
	}

	const TCHAR *newLexerName = ScintillaEditView::_langNameInfoArray[L_JAVASCRIPT]._langName;
	LexerStyler *pNewStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(newLexerName);
	if (pNewStyler) // New js styler is available, so we can use it do more modern styling
	{
		for (const Style & style : *pNewStyler)
		{
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
		const TCHAR *lexerName = ScintillaEditView::_langNameInfoArray[L_JS]._langName;
		LexerStyler *pOldStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(lexerName);

		if (pOldStyler)
		{
			for (Style style : *pOldStyler) //not by reference, but copy
			{
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
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.cpp.comment.explicit"), reinterpret_cast<LPARAM>("0"));
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


	setLexerFromLangID(L_TCL);

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
	setLexerFromLangID(L_OBJC);

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
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.cpp.comment.explicit"), reinterpret_cast<LPARAM>("0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::setTypeScriptLexer()
{
	const TCHAR* doxygenKeyWords = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);
	setLexerFromLangID(L_TYPESCRIPT);

	if (doxygenKeyWords)
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char* doxygenKeyWords_char = wmc.wchar2char(doxygenKeyWords, CP_ACP);
		execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(doxygenKeyWords_char));
	}

	const TCHAR* pKwArray[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	makeStyle(L_TYPESCRIPT, pKwArray);

	auto getKeywordList = [&pKwArray](const int i) 
	{
		if (pKwArray[i])
		{
			basic_string<wchar_t> kwlW = pKwArray[i];
			return wstring2string(kwlW, CP_ACP);
		}
		return basic_string<char>("");
	};

	auto keywordListInstruction = getKeywordList(LANG_INDEX_INSTR);
	const char* tsInstructions = getCompleteKeywordList(keywordListInstruction, L_TYPESCRIPT, LANG_INDEX_INSTR);

	string keywordListType = getKeywordList(LANG_INDEX_TYPE);
	const char* tsTypes = getCompleteKeywordList(keywordListType, L_TYPESCRIPT, LANG_INDEX_TYPE);

	execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(tsInstructions));
	execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(tsTypes));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.cpp.comment.explicit"), reinterpret_cast<LPARAM>("0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.track.preprocessor"), reinterpret_cast<LPARAM>("0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.backquoted.strings"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::setKeywords(LangType langType, const char *keywords, int index)
{
	std::basic_string<char> wordList;
	wordList = (keywords)?keywords:"";
	execute(SCI_SETKEYWORDS, index, reinterpret_cast<LPARAM>(getCompleteKeywordList(wordList, langType, index)));
}

void ScintillaEditView::setLexer(LangType langType, int whichList)
{
	setLexerFromLangID(langType);

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
	const TCHAR * lexerName = ScintillaEditView::_langNameInfoArray[language]._langName;
	LexerStyler *pStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(lexerName);
	if (pStyler)
	{
		for (const Style & style : *pStyler)
		{
			setStyle(style);
			if (keywordArray)
			{
				if ((style._keywordClass != STYLE_NOT_USED) && (!style._keywords.empty()))
					keywordArray[style._keywordClass] = style._keywords.c_str();
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
		if (!found)
		{
			chars2addStr.push_back(char2check);
		}
	}

	if (!chars2addStr.empty())
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

void ScintillaEditView::setCRLF(long color)
{
	NppParameters& nppParams = NppParameters::getInstance();
	const ScintillaViewParams& svp = nppParams.getSVP();
	
	COLORREF eolCustomColor = liteGrey;

	if (color == -1)
	{
		StyleArray& stylers = nppParams.getMiscStylerArray();
		Style* pStyle = stylers.findByName(TEXT("EOL custom color"));
		if (pStyle)
		{
			eolCustomColor = pStyle->_fgColor;
		}
	}
	else
	{
		eolCustomColor = color;
	}

	ScintillaViewParams::crlfMode eolMode = svp._eolMode;
	long appearance = SC_REPRESENTATION_BLOB;

	if (eolMode == ScintillaViewParams::crlfMode::plainText)
		appearance = SC_REPRESENTATION_PLAIN;
	else if (eolMode == ScintillaViewParams::crlfMode::plainTextCustomColor)
		appearance = SC_REPRESENTATION_PLAIN | SC_REPRESENTATION_COLOUR;
	else if (eolMode == ScintillaViewParams::crlfMode::roundedRectangleText)
		appearance = SC_REPRESENTATION_BLOB;
	else if (eolMode == ScintillaViewParams::crlfMode::roundedRectangleTextCustomColor)
		appearance = SC_REPRESENTATION_BLOB | SC_REPRESENTATION_COLOUR;

	const wchar_t* cr = L"\x0d";
	const wchar_t* lf = L"\x0a";
	
	long alphaEolCustomColor = eolCustomColor;
	alphaEolCustomColor |= 0xFF000000; // add alpha color to make DirectWrite mode work

	execute(SCI_SETREPRESENTATIONCOLOUR, reinterpret_cast<WPARAM>(cr), alphaEolCustomColor);
	execute(SCI_SETREPRESENTATIONCOLOUR, reinterpret_cast<WPARAM>(lf), alphaEolCustomColor);

	execute(SCI_SETREPRESENTATIONAPPEARANCE, reinterpret_cast<WPARAM>(cr), appearance);
	execute(SCI_SETREPRESENTATIONAPPEARANCE, reinterpret_cast<WPARAM>(lf), appearance);

	redraw();
}

void ScintillaEditView::defineDocType(LangType typeDoc)
{
	StyleArray & stylers = NppParameters::getInstance().getMiscStylerArray();
	Style * pStyleDefault = stylers.findByID(STYLE_DEFAULT);
	if (pStyleDefault)
	{
		pStyleDefault->_colorStyle = COLORSTYLE_ALL;	//override transparency
		setStyle(*pStyleDefault);
	}

	execute(SCI_STYLECLEARALL);

	Style defaultIndicatorStyle;
	const Style * pStyle;

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE;
	defaultIndicatorStyle._bgColor = red;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_SMART;
	defaultIndicatorStyle._bgColor = liteGreen;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_INC;
	defaultIndicatorStyle._bgColor = blue;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_TAGMATCH;
	defaultIndicatorStyle._bgColor = RGB(0x80, 0x00, 0xFF);
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_TAGATTR;
	defaultIndicatorStyle._bgColor = yellow;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);


	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT1;
	defaultIndicatorStyle._bgColor = cyan;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT2;
	defaultIndicatorStyle._bgColor = orange;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT3;
	defaultIndicatorStyle._bgColor = yellow;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT4;
	defaultIndicatorStyle._bgColor = purple;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

	defaultIndicatorStyle._styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT5;
	defaultIndicatorStyle._bgColor = darkGreen;
	pStyle = stylers.findByID(defaultIndicatorStyle._styleID);
	setSpecialIndicator(pStyle ? *pStyle : defaultIndicatorStyle);

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
				const Style * pStyle = pStyler->findByName(TEXT("DEFAULT"));
				if (pStyle)
				{
					nfoStyle._bgColor = pStyle->_bgColor;
					nfoStyle._fgColor = pStyle->_fgColor;
					nfoStyle._colorStyle = pStyle->_colorStyle;
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

		case L_NIM :
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

		case L_TYPESCRIPT:
			setTypeScriptLexer(); break;

		case L_TEXT :
		default :
			if (typeDoc >= L_EXTERNAL && typeDoc < NppParameters::getInstance().L_END)
				setExternalLexer(typeDoc);
			else
				setLexerFromLangID(L_TEXT);
			break;

	}
	//All the global styles should put here
	pStyle = stylers.findByID(STYLE_INDENTGUIDE);
	if (pStyle)
	{
		setStyle(*pStyle);
	}
	pStyle = stylers.findByID(STYLE_BRACELIGHT);
	if (pStyle)
	{
		setStyle(*pStyle);
	}
	//setStyle(STYLE_CONTROLCHAR, liteGrey);
	pStyle = stylers.findByID(STYLE_BRACEBAD);
	if (pStyle)
	{
		setStyle(*pStyle);
	}
	pStyle = stylers.findByID(STYLE_LINENUMBER);
	if (pStyle)
	{
		setSpecialStyle(*pStyle);
	}
	setTabSettings(NppParameters::getInstance().getLangFromID(typeDoc));
	
	if (svp._indentGuideLineShow)
	{
		const auto currentIndentMode = execute(SCI_GETINDENTATIONGUIDES);
		// Python like indentation, excludes lexers (Nim, VB, YAML, etc.)
		// that includes tailing empty or whitespace only lines in folding block.
		const int docIndentMode = isPythonStyleIndentation(typeDoc) ? SC_IV_LOOKFORWARD : SC_IV_LOOKBOTH;
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
	size_t displayedLine = execute(SCI_GETFIRSTVISIBLELINE);
	size_t docLine = execute(SCI_DOCLINEFROMVISIBLE, displayedLine);		//linenumber of the line displayed in the top
	size_t offset = displayedLine - execute(SCI_VISIBLEFROMDOCLINE, docLine);		//use this to calc offset of wrap. If no wrap this should be zero
	size_t wrapCount = execute(SCI_WRAPCOUNT, docLine);

	Buffer * buf = MainFileManager.getBufferByID(_currentBufferID);

	Position pos;
	// the correct visible line number
	pos._firstVisibleLine = docLine;
	pos._startPos = execute(SCI_GETANCHOR);
	pos._endPos = execute(SCI_GETCURRENTPOS);
	pos._xOffset = execute(SCI_GETXOFFSET);
	pos._selMode = execute(SCI_GETSELECTIONMODE);
	pos._scrollWidth = execute(SCI_GETSCROLLWIDTH);
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
	if (!isWrap()) //only offset if not wrapping, otherwise the offset isnt needed at all
	{
		execute(SCI_SETSCROLLWIDTH, pos._scrollWidth);
		execute(SCI_SETXOFFSET, pos._xOffset);
	}
	execute(SCI_CHOOSECARETX); // choose current x position
	intptr_t lineToShow = execute(SCI_VISIBLEFROMDOCLINE, pos._firstVisibleLine);
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
	
	intptr_t displayedLine = execute(SCI_GETFIRSTVISIBLELINE);
	intptr_t docLine = execute(SCI_DOCLINEFROMVISIBLE, displayedLine);		//linenumber of the line displayed in the 
	

	// check docLine must equals saved position
	if (docLine != pos._firstVisibleLine)
	{
		
		// Scintilla has paint the buffer but the position is not correct.
		intptr_t lineToShow = execute(SCI_VISIBLEFROMDOCLINE, pos._firstVisibleLine);
		execute(SCI_SETFIRSTVISIBLELINE, lineToShow);
	}
	else if (pos._offset > 0)
	{
		// don't scroll anything if the wrap count is different than the saved one.
		// Buffer update may be in progress (in case wrap is enabled)
		intptr_t wrapCount = execute(SCI_WRAPCOUNT, docLine);
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

bool ScintillaEditView::setLexerFromLangID(int langID) // Internal lexer only
{
	if (langID >= L_EXTERNAL)
		return false;

	const char* lexerNameID = _langNameInfoArray[langID]._lexerID;
	execute(SCI_SETILEXER, 0, reinterpret_cast<LPARAM>(CreateLexer(lexerNameID)));
	return true;
}


void ScintillaEditView::activateBuffer(BufferID buffer, bool force)
{
	if (buffer == BUFFER_INVALID)
		return;
	if (!force && buffer == _currentBuffer)
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

	runMarkers(true, 0, true, false);

	setCRLF();

    return;	//all done
}

void ScintillaEditView::getCurrentFoldStates(std::vector<size_t> & lineStateVector)
{
	// xCodeOptimization1304: For active document get folding state from Scintilla.
	// The code using SCI_CONTRACTEDFOLDNEXT is usually 10%-50% faster than checking each line of the document!!
	size_t contractedFoldHeaderLine = 0;

	do {
		contractedFoldHeaderLine = execute(SCI_CONTRACTEDFOLDNEXT, contractedFoldHeaderLine);
		if (static_cast<intptr_t>(contractedFoldHeaderLine) != -1)
		{
			//-- Store contracted line
			lineStateVector.push_back(contractedFoldHeaderLine);
			//-- Start next search with next line
			++contractedFoldHeaderLine;
		}
	} while (static_cast<intptr_t>(contractedFoldHeaderLine) != -1);
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
	intptr_t levelStack[MAX_FOLD_COLLAPSE_LEVEL]{};

	void push(intptr_t level)
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

	const intptr_t maxLine = execute(SCI_GETLINECOUNT);
	intptr_t line = 0;

	while (line < maxLine)
	{
		intptr_t level = execute(SCI_GETFOLDLEVEL, line);
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
				line = execute(SCI_GETLASTCHILD, line, -1);
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

	intptr_t maxLine = execute(SCI_GETLINECOUNT);

	for (int line = 0; line < maxLine; ++line)
	{
		intptr_t level = execute(SCI_GETFOLDLEVEL, line);
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
	auto currentLine = getCurrentLineNumber();
	fold(currentLine, mode);
}

bool ScintillaEditView::isCurrentLineFolded() const
{
	auto currentLine = getCurrentLineNumber();

	intptr_t headerLine;
	auto level = execute(SCI_GETFOLDLEVEL, currentLine);

	if (level & SC_FOLDLEVELHEADERFLAG)
		headerLine = currentLine;
	else
	{
		headerLine = execute(SCI_GETFOLDPARENT, currentLine);
		if (headerLine == -1)
			return false;
	}

	bool isExpanded = execute(SCI_GETFOLDEXPANDED, headerLine);
	return !isExpanded;
}

void ScintillaEditView::fold(size_t line, bool mode)
{
    auto endStyled = execute(SCI_GETENDSTYLED);
    auto len = execute(SCI_GETTEXTLENGTH);

    if (endStyled < len)
        execute(SCI_COLOURISE, 0, -1);

	intptr_t headerLine;
	auto level = execute(SCI_GETFOLDLEVEL, line);

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
	Sci_TextRangeFull tr{};
	tr.chrg.cpMin = static_cast<Sci_Position>(start);
	tr.chrg.cpMax = static_cast<Sci_Position>(end);
	tr.lpstrText = dest;
	execute(SCI_GETTEXTRANGEFULL, 0, reinterpret_cast<LPARAM>(&tr));
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
	size_t cp = execute(SCI_GETCODEPAGE);
	const TCHAR *destW = wmc.char2wchar(destA, cp);
	_tcsncpy_s(dest, destlen, destW, _TRUNCATE);
	delete [] destA;
}

// "mstart" and "mend" are pointers to indexes in the read string,
// which are converted to the corresponding indexes in the returned TCHAR string.

void ScintillaEditView::getGenericText(TCHAR *dest, size_t destlen, size_t start, size_t end, intptr_t* mstart, intptr_t* mend) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	char *destA = new char[end - start + 1];
	getText(destA, start, end);
	size_t cp = execute(SCI_GETCODEPAGE)    ;
	const TCHAR *destW = wmc.char2wchar(destA, cp, mstart, mend);
	_tcsncpy_s(dest, destlen, destW, _TRUNCATE);
	delete [] destA;
}

void ScintillaEditView::insertGenericTextFrom(size_t position, const TCHAR *text2insert) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2insertA = wmc.wchar2char(text2insert, cp);
	execute(SCI_INSERTTEXT, position, reinterpret_cast<LPARAM>(text2insertA));
}

void ScintillaEditView::replaceSelWith(const char * replaceText)
{
	execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(replaceText));
}

void ScintillaEditView::getVisibleStartAndEndPosition(intptr_t* startPos, intptr_t* endPos)
{
	assert(startPos != NULL && endPos != NULL);
	// Get the position of the 1st and last showing chars from the edit view
	RECT rcEditView;
	getClientRect(rcEditView);
	LRESULT pos = execute(SCI_POSITIONFROMPOINT, 0, 0);
	LRESULT line = execute(SCI_LINEFROMPOSITION, pos);
	*startPos = execute(SCI_POSITIONFROMLINE, line);
	pos = execute(SCI_POSITIONFROMPOINT, rcEditView.right - rcEditView.left, rcEditView.bottom - rcEditView.top);
	line = execute(SCI_LINEFROMPOSITION, pos);
	*endPos = execute(SCI_GETLINEENDPOSITION, line);
}

char * ScintillaEditView::getWordFromRange(char * txt, size_t size, size_t pos1, size_t pos2)
{
    if (!size)
		return NULL;
    if (pos1 > pos2)
    {
        size_t tmp = pos1;
        pos1 = pos2;
        pos2 = tmp;
    }

    if (size < pos2 - pos1)
        return NULL;

    getText(txt, pos1, pos2);
	return txt;
}

char * ScintillaEditView::getWordOnCaretPos(char * txt, size_t size)
{
    if (!size)
		return NULL;

    pair<size_t, size_t> range = getWordRange();
    return getWordFromRange(txt, size, range.first, range.second);
}

TCHAR * ScintillaEditView::getGenericWordOnCaretPos(TCHAR * txt, int size)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	char *txtA = new char[size + 1];
	getWordOnCaretPos(txtA, size);

	const TCHAR * txtW = wmc.char2wchar(txtA, cp);
	wcscpy_s(txt, size, txtW);
	delete [] txtA;
	return txt;
}

char * ScintillaEditView::getSelectedText(char * txt, size_t size, bool expand)
{
	if (!size)
		return NULL;
	Sci_CharacterRange range = getSelection();
	if (range.cpMax == range.cpMin && expand)
	{
		expandWordSelection();
		range = getSelection();
	}
	if (!(static_cast<Sci_PositionCR>(size) > (range.cpMax - range.cpMin)))	//there must be atleast 1 byte left for zero terminator
	{
		range.cpMax = range.cpMin + (Sci_PositionCR)size -1;	//keep room for zero terminator
	}
	//getText(txt, range.cpMin, range.cpMax);
	return getWordFromRange(txt, size, range.cpMin, range.cpMax);
}

TCHAR * ScintillaEditView::getGenericSelectedText(TCHAR * txt, int size, bool expand)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	char *txtA = new char[size + 1];
	getSelectedText(txtA, size, expand);

	const TCHAR * txtW = wmc.char2wchar(txtA, cp);
	wcscpy_s(txt, size, txtW);
	delete [] txtA;
	return txt;
}

intptr_t ScintillaEditView::searchInTarget(const TCHAR * text2Find, size_t lenOfText2Find, size_t fromPos, size_t toPos) const
{
	execute(SCI_SETTARGETRANGE, fromPos, toPos);

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2FindA = wmc.wchar2char(text2Find, cp);
	size_t text2FindALen = strlen(text2FindA);
   	size_t len = (lenOfText2Find > text2FindALen) ? lenOfText2Find : text2FindALen;
	return execute(SCI_SEARCHINTARGET, len, reinterpret_cast<LPARAM>(text2FindA));
}

void ScintillaEditView::appandGenericText(const TCHAR * text2Append) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2AppendA =wmc.wchar2char(text2Append, cp);
	execute(SCI_APPENDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

void ScintillaEditView::addGenericText(const TCHAR * text2Append) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2AppendA =wmc.wchar2char(text2Append, cp);
	execute(SCI_ADDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

void ScintillaEditView::addGenericText(const TCHAR * text2Append, intptr_t* mstart, intptr_t* mend) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2AppendA =wmc.wchar2char(text2Append, cp, mstart, mend);
	execute(SCI_ADDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

intptr_t ScintillaEditView::replaceTarget(const TCHAR * str2replace, intptr_t fromTargetPos, intptr_t toTargetPos) const
{
	if (fromTargetPos != -1 || toTargetPos != -1)
	{
		execute(SCI_SETTARGETRANGE, fromTargetPos, toTargetPos);
	}
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *str2replaceA = wmc.wchar2char(str2replace, cp);
	return execute(SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(str2replaceA));
}

intptr_t ScintillaEditView::replaceTargetRegExMode(const TCHAR * re, intptr_t fromTargetPos, intptr_t toTargetPos) const
{
	if (fromTargetPos != -1 || toTargetPos != -1)
	{
		execute(SCI_SETTARGETRANGE, fromTargetPos, toTargetPos);
	}
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *reA = wmc.wchar2char(re, cp);
	return execute(SCI_REPLACETARGETRE, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(reA));
}

void ScintillaEditView::showAutoComletion(size_t lenEntered, const TCHAR* list)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *listA = wmc.wchar2char(list, cp);
	execute(SCI_AUTOCSHOW, lenEntered, reinterpret_cast<LPARAM>(listA));
	NppDarkMode::setDarkAutoCompletion();
}

void ScintillaEditView::showCallTip(size_t startPos, const TCHAR * def)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *defA = wmc.wchar2char(def, cp);
	execute(SCI_CALLTIPSHOW, startPos, reinterpret_cast<LPARAM>(defA));
}

generic_string ScintillaEditView::getLine(size_t lineNumber)
{
	size_t lineLen = execute(SCI_LINELENGTH, lineNumber);
	const size_t bufSize = lineLen + 1;
	std::unique_ptr<TCHAR[]> buf = std::make_unique<TCHAR[]>(bufSize);
	getLine(lineNumber, buf.get(), bufSize);
	return buf.get();
}

void ScintillaEditView::getLine(size_t lineNumber, TCHAR * line, size_t lineBufferLen)
{
	// make sure the buffer length is enough to get the whole line
	size_t lineLen = execute(SCI_LINELENGTH, lineNumber);
	if (lineLen >= lineBufferLen)
		return;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	char *lineA = new char[lineBufferLen];
	// From Scintilla documentation for SCI_GETLINE: "The buffer is not terminated by a 0 character."
	memset(lineA, 0x0, sizeof(char) * lineBufferLen);
	
	execute(SCI_GETLINE, lineNumber, reinterpret_cast<LPARAM>(lineA));
	const TCHAR *lineW = wmc.char2wchar(lineA, cp);
	lstrcpyn(line, lineW, static_cast<int>(lineBufferLen));
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
		_beginSelectPosition = execute(SCI_GETCURRENTPOS);
	}
	else
	{
		execute(SCI_SETANCHOR, static_cast<WPARAM>(_beginSelectPosition));
		_beginSelectPosition = -1;
	}
}

void ScintillaEditView::showMargin(int whichMarge, bool willBeShowed)
{
	if (whichMarge == _SC_MARGE_LINENUMBER)
	{
		bool forcedToHide = !willBeShowed;
		updateLineNumbersMargin(forcedToHide);
	}
	else
	{
		DPIManager& dpiManager = NppParameters::getInstance()._dpiManager;
		int width = dpiManager.scaleX(3);
		if (whichMarge == _SC_MARGE_SYBOLE)
			width = dpiManager.scaleX(16);
		else if (whichMarge == _SC_MARGE_FOLDER)
			width = dpiManager.scaleX(14);
		execute(SCI_SETMARGINWIDTHN, whichMarge, willBeShowed ? width : 0);
	}
}

void ScintillaEditView::updateBeginEndSelectPosition(bool is_insert, size_t position, size_t length)
{
	if (_beginSelectPosition != -1 && static_cast<intptr_t>(position) < _beginSelectPosition - 1)
	{
		if (is_insert)
			_beginSelectPosition += length;
		else
			_beginSelectPosition -= length;

		assert(_beginSelectPosition >= 0);
	}
}

void ScintillaEditView::marginClick(Sci_Position position, int modifiers)
{
	size_t lineClick = execute(SCI_LINEFROMPOSITION, position, 0);
	intptr_t levelClick = execute(SCI_GETFOLDLEVEL, lineClick, 0);
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

void ScintillaEditView::expand(size_t& line, bool doExpand, bool force, intptr_t visLevels, intptr_t level)
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

		intptr_t levelLine = level;
		if (levelLine == -1)
			levelLine = execute(SCI_GETFOLDLEVEL, line, 0);

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
	NppParameters& nppParams = NppParameters::getInstance();
	const ScintillaViewParams& svp = nppParams.getSVP();
	
	StyleArray& stylers = nppParams.getMiscStylerArray();
	const Style* pStyle{};

	if (svp._currentLineHiliteMode != LINEHILITE_NONE)
	{
		pStyle = stylers.findByName(TEXT("Current line background colour"));
		if (pStyle)
		{
			execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK, pStyle->_bgColor);
		}
	}

	execute(SCI_SETCARETLINEFRAME, (svp._currentLineHiliteMode == LINEHILITE_FRAME) ? svp._currentLineFrameWidth : 0);

	COLORREF selectColorBack = grey;
	COLORREF selectColorFore = black;
	pStyle = stylers.findByName(TEXT("Selected text colour"));
	if (pStyle)
	{
		selectColorBack = pStyle->_bgColor;
		selectColorFore = pStyle->_fgColor;
	}
	execute(SCI_SETSELBACK, 1, selectColorBack);
	execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_INACTIVE_BACK, selectColorBack);

	if (nppParams.isSelectFgColorEnabled())
	{
		execute(SCI_SETSELFORE, 1, selectColorFore);

		long alphaSelectColorFore = selectColorFore;
		alphaSelectColorFore |= 0xFF000000; // add alpha color to make DirectWrite mode work
		execute(SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_INACTIVE_TEXT, alphaSelectColorFore);
	}

	COLORREF caretColor = black;
	pStyle = stylers.findByID(SCI_SETCARETFORE);
	if (pStyle)
	{
		caretColor = pStyle->_fgColor;
	}
	execute(SCI_SETCARETFORE, caretColor);

	COLORREF edgeColor = liteGrey;
	pStyle = stylers.findByName(TEXT("Edge colour"));
	if (pStyle)
	{
		edgeColor = pStyle->_fgColor;
	}
	execute(SCI_SETEDGECOLOUR, edgeColor);
	::SendMessage(_hParent, NPPM_INTERNAL_EDGEMULTISETSIZE, 0, 0);

	COLORREF foldMarginColor = grey;
	COLORREF foldMarginHiColor = white;
	pStyle = stylers.findByName(TEXT("Fold margin"));
	if (pStyle)
	{
		foldMarginHiColor = pStyle->_fgColor;
		foldMarginColor = pStyle->_bgColor;
	}
	execute(SCI_SETFOLDMARGINCOLOUR, true, foldMarginColor);
	execute(SCI_SETFOLDMARGINHICOLOUR, true, foldMarginHiColor);

	COLORREF bookmarkMarginColor = veryLiteGrey;
	pStyle = stylers.findByName(TEXT("Bookmark margin"));
	if (!pStyle)
	{
		pStyle = stylers.findByName(TEXT("Line number margin"));
		if (pStyle)
		{
			bookmarkMarginColor = pStyle->_bgColor;
		}
	}
	else
	{
		bookmarkMarginColor = pStyle->_bgColor;
	}
	execute(SCI_SETMARGINTYPEN, _SC_MARGE_SYBOLE, SC_MARGIN_COLOUR);
	execute(SCI_SETMARGINBACKN, _SC_MARGE_SYBOLE, bookmarkMarginColor);

	COLORREF urlHoveredFG = grey;
	pStyle = stylers.findByName(TEXT("URL hovered"));
	if (pStyle)
	{
		urlHoveredFG = pStyle->_fgColor;
	}
	execute(SCI_INDICSETHOVERFORE, URL_INDIC, urlHoveredFG);

	COLORREF foldfgColor = white, foldbgColor = grey, activeFoldFgColor = red;
	getFoldColor(foldfgColor, foldbgColor, activeFoldFgColor);

	for (int j = 0 ; j < NB_FOLDER_STATE ; ++j)
		defineMarker(_markersArray[FOLDER_TYPE][j], _markersArray[svp._folderStyle][j], foldfgColor, foldbgColor, activeFoldFgColor);

	execute(SCI_MARKERENABLEHIGHLIGHT, true);

	COLORREF wsSymbolFgColor = black;
	pStyle = stylers.findByName(TEXT("White space symbol"));
	if (pStyle)
	{
		wsSymbolFgColor = pStyle->_fgColor;
	}
	execute(SCI_SETWHITESPACEFORE, true, wsSymbolFgColor);

	COLORREF eolCustomColor = liteGrey;
	pStyle = stylers.findByName(TEXT("EOL custom color"));
	if (pStyle)
	{
		eolCustomColor = pStyle->_fgColor;
	}
	setCRLF(eolCustomColor);
}

void ScintillaEditView::showIndentGuideLine(bool willBeShowed)
{
	auto typeDoc = _currentBuffer->getLangType();
	const int docIndentMode = isPythonStyleIndentation(typeDoc) ? SC_IV_LOOKFORWARD : SC_IV_LOOKBOTH;
	execute(SCI_SETINDENTATIONGUIDES, willBeShowed ? docIndentMode : SC_IV_NONE);
}

void ScintillaEditView::setLineIndent(size_t line, size_t indent) const
{
	Sci_CharacterRange crange = getSelection();
	size_t posBefore = execute(SCI_GETLINEINDENTPOSITION, line);
	execute(SCI_SETLINEINDENTATION, line, indent);
	size_t posAfter = execute(SCI_GETLINEINDENTPOSITION, line);
	long long posDifference = posAfter - posBefore;
	if (posAfter > posBefore)
	{
		// Move selection on
		if (crange.cpMin >= static_cast<Sci_PositionCR>(posBefore))
		{
			crange.cpMin += static_cast<Sci_PositionCR>(posDifference);
		}
		if (crange.cpMax >= static_cast<Sci_PositionCR>(posBefore))
		{
			crange.cpMax += static_cast<Sci_PositionCR>(posDifference);
		}
	}
	else if (posAfter < posBefore)
	{
		// Move selection back
		if (crange.cpMin >= static_cast<Sci_PositionCR>(posAfter))
		{
			if (crange.cpMin >= static_cast<Sci_PositionCR>(posBefore))
				crange.cpMin += static_cast<Sci_PositionCR>(posDifference);
			else
				crange.cpMin = static_cast<Sci_PositionCR>(posAfter);
		}

		if (crange.cpMax >= static_cast<Sci_PositionCR>(posAfter))
		{
			if (crange.cpMax >= static_cast<Sci_PositionCR>(posBefore))
				crange.cpMax += static_cast<Sci_PositionCR>(posDifference);
			else
				crange.cpMax = static_cast<Sci_PositionCR>(posAfter);
		}
	}
	execute(SCI_SETSEL, crange.cpMin, crange.cpMax);
}

void ScintillaEditView::updateLineNumberWidth()
{
	const ScintillaViewParams& svp = NppParameters::getInstance().getSVP();
	if (svp._lineNumberMarginShow)
	{
		auto linesVisible = execute(SCI_LINESONSCREEN);
		if (linesVisible)
		{
			int nbDigits = 0;

			if (svp._lineNumberMarginDynamicWidth)
			{
				auto firstVisibleLineVis = execute(SCI_GETFIRSTVISIBLELINE);
				auto lastVisibleLineVis = linesVisible + firstVisibleLineVis + 1;
				auto lastVisibleLineDoc = execute(SCI_DOCLINEFROMVISIBLE, lastVisibleLineVis);

				nbDigits = nbDigitsFromNbLines(lastVisibleLineDoc);
				nbDigits = nbDigits < 3 ? 3 : nbDigits;
			}
			else
			{
				auto nbLines = execute(SCI_GETLINECOUNT);
				nbDigits = nbDigitsFromNbLines(nbLines);
				nbDigits = nbDigits < 4 ? 4 : nbDigits;
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
			intptr_t selStart = cmi[i]._direction == L2R?cmi[i]._selLpos:cmi[i]._selRpos;
			intptr_t selEnd   = cmi[i]._direction == L2R?cmi[i]._selRpos:cmi[i]._selLpos;
			execute(SCI_SETSELECTIONNSTART, i, selStart);
			execute(SCI_SETSELECTIONNEND, i, selEnd);
		}

		if (cmi[i]._nbVirtualAnchorSpc)
			execute(SCI_SETSELECTIONNANCHORVIRTUALSPACE, i, cmi[i]._nbVirtualAnchorSpc);
		if (cmi[i]._nbVirtualCaretSpc)
			execute(SCI_SETSELECTIONNCARETVIRTUALSPACE, i, cmi[i]._nbVirtualCaretSpc);
	}
}

// Get selection range (fromLine, toLine) for the specified selection
// specify selectionNumber = -1 for the MAIN selection
pair<size_t, size_t> ScintillaEditView::getSelectionLinesRange(intptr_t selectionNumber /* = -1 */) const
{
	size_t numSelections = execute(SCI_GETSELECTIONS);

	size_t start_pos, end_pos;

	if ((selectionNumber < 0) || (static_cast<size_t>(selectionNumber) >= numSelections))
	{
		start_pos = execute(SCI_GETSELECTIONSTART);
		end_pos = execute(SCI_GETSELECTIONEND);
	}
	else
	{
		start_pos = execute(SCI_GETSELECTIONNSTART, selectionNumber);
		end_pos = execute(SCI_GETSELECTIONNEND, selectionNumber);
	}

	size_t line1 = execute(SCI_LINEFROMPOSITION, start_pos);
	size_t line2 = execute(SCI_LINEFROMPOSITION, end_pos);

	if ((line1 != line2) && (static_cast<size_t>(execute(SCI_POSITIONFROMLINE, line2)) == end_pos))
	{
		// if the end of the selection includes the line-ending, 
		// then don't include the following line in the range
		--line2;
	}

	return pair<size_t, size_t>(line1, line2);
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

// Case converts the document byte range [start:end] in place and returns the
// change in its length in bytes. On any error, does nothing and returns zero.
intptr_t ScintillaEditView::caseConvertRange(intptr_t start, intptr_t end, TextCase caseToConvert)
{
	if (end <= start || uintptr_t(end) - uintptr_t(start) > INT_MAX/2)
		return 0;

	unsigned codepage = getCurrentBuffer()->getUnicodeMode() == uni8Bit ? _codepage : CP_UTF8;

	int mbLen = int(end - start);
	const int mbLenMax = 2 * mbLen + 1;  // allow final NUL + substantial expansion

	char *mbStr = new char[mbLenMax];
	getText(mbStr, start, end);

	if (int wideLen = ::MultiByteToWideChar(codepage, 0, mbStr, mbLen, NULL, 0))
	{
		wchar_t *wideStr = new wchar_t[wideLen];  // not NUL terminated
		::MultiByteToWideChar(codepage, 0, mbStr, mbLen, wideStr, wideLen);

		changeCase(wideStr, wideLen, caseToConvert);

		if (int mbLenOut = ::WideCharToMultiByte(codepage, 0, wideStr, wideLen, mbStr, mbLenMax, NULL, NULL))
		{
			// mbStr isn't NUL terminated either at this point
			mbLen = mbLenOut;

			execute(SCI_SETTARGETRANGE, start, end);
			execute(SCI_REPLACETARGET, mbLen, reinterpret_cast<LPARAM>(mbStr));
		}

		delete [] wideStr;
	}

	delete [] mbStr;

	return (start + mbLen) - end;
}

void ScintillaEditView::changeCase(__inout wchar_t * const strWToConvert, const int & nbChars, const TextCase & caseToConvert) const
{
	if (strWToConvert == nullptr || nbChars == 0)
		return;

	switch (caseToConvert)
	{
		case UPPERCASE:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
			}
			break; 
		} //case UPPERCASE
		case LOWERCASE:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
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
					if ((i < 1) ? true : !::IsCharAlphaNumericW(strWToConvert[i - 1]))
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
					else if (caseToConvert == TITLECASE_FORCE)
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
					//An exception
					if ((i < 2) ? false : (strWToConvert[i - 1] == L'\'' && ::IsCharAlphaW(strWToConvert[i - 2])))
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
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
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
						isNewSentence = false;
					}
					else if (caseToConvert == SENTENCECASE_FORCE)
					{
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
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
					strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
				else
					strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
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
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
					else
						strWToConvert[i] = (WCHAR)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
				}
			}
			break; 
		} //case RANDOMCASE
	} //switch (caseToConvert)
}

void ScintillaEditView::convertSelectedTextTo(const TextCase & caseToConvert)
{
	if (execute(SCI_GETSELECTIONS) > 1) // Multi-Selection || Column mode
	{
        execute(SCI_BEGINUNDOACTION);

		ColumnModeInfos cmi = getColumnModeSelectInfo();
		// The fixup logic needs the selections to be sorted, but that has visible side effects,
		// like the highlighted row jumping around, so try to restore the original order afterwards.
		bool reversed = !cmi.empty() && cmi.back()._selLpos < cmi.front()._selLpos;
		std::sort(cmi.begin(), cmi.end(), SortInPositionOrder());

		intptr_t sizedelta = 0;
		for (ColumnModeInfo& info : cmi)
		{
			info._selLpos += sizedelta;
			sizedelta += caseConvertRange(info._selLpos, info._selRpos + sizedelta, caseToConvert);
			info._selRpos += sizedelta;
		}

		if (reversed)
			std::reverse(cmi.begin(), cmi.end());
		setMultiSelections(cmi);

		execute(SCI_ENDUNDOACTION);
		return;
	}

	size_t selectionStart = execute(SCI_GETSELECTIONSTART);
	size_t selectionEnd = execute(SCI_GETSELECTIONEND);

	if (selectionStart < selectionEnd)
	{
		selectionEnd += caseConvertRange(selectionStart, selectionEnd, caseToConvert);
		execute(SCI_SETSEL, selectionStart, selectionEnd);
	}
}


pair<size_t, size_t> ScintillaEditView::getWordRange()
{
	size_t caretPos = execute(SCI_GETCURRENTPOS, 0, 0);
	size_t startPos = execute(SCI_WORDSTARTPOSITION, caretPos, true);
	size_t endPos = execute(SCI_WORDENDPOSITION, caretPos, true);
    return pair<size_t, size_t>(startPos, endPos);
}

bool ScintillaEditView::expandWordSelection()
{
    pair<size_t, size_t> wordRange = 	getWordRange();
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
		intptr_t nbSel = execute(SCI_GETSELECTIONS);

		for (int i = 0 ; i < nbSel ; ++i)
		{
			intptr_t absPosSelStartPerLine = execute(SCI_GETSELECTIONNANCHOR, i);
			intptr_t absPosSelEndPerLine = execute(SCI_GETSELECTIONNCARET, i);
			intptr_t nbVirtualAnchorSpc = execute(SCI_GETSELECTIONNANCHORVIRTUALSPACE, i);
			intptr_t nbVirtualCaretSpc = execute(SCI_GETSELECTIONNCARETVIRTUALSPACE, i);

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
	intptr_t totalDiff = 0;
	for (size_t i = 0, len = cmi.size(); i < len ; ++i)
	{
		if (cmi[i].isValid())
		{
			intptr_t len2beReplace = cmi[i]._selRpos - cmi[i]._selLpos;
			intptr_t diff = lstrlen(str) - len2beReplace;

			cmi[i]._selLpos += totalDiff;
			cmi[i]._selRpos += totalDiff;
			bool hasVirtualSpc = cmi[i]._nbVirtualAnchorSpc > 0;

			if (hasVirtualSpc) // if virtual space is present, then insert space
			{
				for (intptr_t j = 0, k = cmi[i]._selLpos; j < cmi[i]._nbVirtualCaretSpc ; ++j, ++k)
				{
					execute(SCI_INSERTTEXT, k, reinterpret_cast<LPARAM>(" "));
				}
				cmi[i]._selLpos += cmi[i]._nbVirtualAnchorSpc;
				cmi[i]._selRpos += cmi[i]._nbVirtualCaretSpc;
			}

			execute(SCI_SETTARGETRANGE, cmi[i]._selLpos, cmi[i]._selRpos);

			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			size_t cp = execute(SCI_GETCODEPAGE);
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

	intptr_t totalDiff = 0;
	const size_t len = cmi.size();
	for (size_t i = 0 ; i < len ; i++)
	{
		if (cmi[i].isValid())
		{
			const intptr_t len2beReplaced = cmi[i]._selRpos - cmi[i]._selLpos;
			const intptr_t diff = kib - len2beReplaced;

			cmi[i]._selLpos += totalDiff;
			cmi[i]._selRpos += totalDiff;

			int2str(str, stringSize, numbers.at(i), base, kib, isZeroLeading);

			const bool hasVirtualSpc = cmi[i]._nbVirtualAnchorSpc > 0;
			if (hasVirtualSpc) // if virtual space is present, then insert space
			{
				for (intptr_t j = 0, k = cmi[i]._selLpos; j < cmi[i]._nbVirtualCaretSpc ; ++j, ++k)
				{
					execute(SCI_INSERTTEXT, k, reinterpret_cast<LPARAM>(" "));
				}
				cmi[i]._selLpos += cmi[i]._nbVirtualAnchorSpc;
				cmi[i]._selRpos += cmi[i]._nbVirtualCaretSpc;
			}
			execute(SCI_SETTARGETRANGE, cmi[i]._selLpos, cmi[i]._selRpos);

			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			size_t cp = execute(SCI_GETCODEPAGE);
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

bool ScintillaEditView::getIndicatorRange(size_t indicatorNumber, size_t* from, size_t* to, size_t* cur)
{
	size_t curPos = execute(SCI_GETCURRENTPOS);
	size_t indicMsk = execute(SCI_INDICATORALLONFOR, curPos);
	if (!(static_cast<int>(indicMsk) & (1 << indicatorNumber)))
		return false;
	size_t startPos = execute(SCI_INDICATORSTART, indicatorNumber, curPos);
	size_t endPos = execute(SCI_INDICATOREND, indicatorNumber, curPos);
	if ((curPos < startPos) || (curPos > endPos))
		return false;
	if (from) *from = startPos;
	if (to) *to = endPos;
	if (cur) *cur = curPos;
	return true;
};


void ScintillaEditView::scrollPosToCenter(size_t pos)
{
	_positionRestoreNeeded = false;

	execute(SCI_GOTOPOS, pos);
	size_t line = execute(SCI_LINEFROMPOSITION, pos);

	size_t firstVisibleDisplayLine = execute(SCI_GETFIRSTVISIBLELINE);
	size_t firstVisibleDocLine = execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine);
	size_t nbLine = execute(SCI_LINESONSCREEN, firstVisibleDisplayLine);
	size_t lastVisibleDocLine = execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine + nbLine);

	size_t middleLine;
	if (line - firstVisibleDocLine < lastVisibleDocLine - line)
		middleLine = firstVisibleDocLine + nbLine/2;
	else
		middleLine = lastVisibleDocLine -  nbLine/2;
	size_t nbLines2scroll =  line - middleLine;
	scroll(0, nbLines2scroll);
	execute(SCI_ENSUREVISIBLEENFORCEPOLICY, line);
}

void ScintillaEditView::hideLines()
{
	//Folding can screw up hide lines badly if it unfolds a hidden section.
	//Adding runMarkers(hide, foldstart) directly (folding on single document) can help

	//Special func on buffer. If markers are added, create notification with location of start, and hide bool set to true
	size_t startLine = execute(SCI_LINEFROMPOSITION, execute(SCI_GETSELECTIONSTART));
	size_t endLine = execute(SCI_LINEFROMPOSITION, execute(SCI_GETSELECTIONEND));
	//perform range check: cannot hide very first and very last lines
	//Offset them one off the edges, and then check if they are within the reasonable
	size_t nbLines = execute(SCI_GETLINECOUNT);
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
	for (size_t i = startLine; i <= endLine; ++i)
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

bool ScintillaEditView::markerMarginClick(size_t lineNumber)
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

void ScintillaEditView::notifyMarkers(Buffer * buf, bool isHide, size_t location, bool del)
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
		execute(SCI_SETTABWIDTH, nppgui._tabSize  > 0 ? nppgui._tabSize : 4);
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
		const auto position = execute(SCI_POSITIONFROMLINE, current_line) - eol_length;
		insertGenericTextFrom(position, newline.c_str());
	}
	execute(SCI_SETEMPTYSELECTION, execute(SCI_POSITIONFROMLINE, current_line));
}


void ScintillaEditView::insertNewLineBelowCurrentLine()
{
	generic_string newline = getEOLString();
	const auto line_count = execute(SCI_GETLINECOUNT);
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
		replaceTarget(joined.c_str(), startPos, endPos);
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
	exStyle = isRTL ? (exStyle | WS_EX_LAYOUTRTL) : (exStyle & (~WS_EX_LAYOUTRTL));
	::SetWindowLongPtr(_hSelf, GWL_EXSTYLE, exStyle);

	if (isRTL)
	{
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT, SCI_CHARLEFT);
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT + (SCMOD_SHIFT << 16), SCI_CHARLEFTEXTEND);
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT + ((SCMOD_SHIFT + SCMOD_ALT) << 16), SCI_CHARLEFTRECTEXTEND);
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT + (SCMOD_CTRL << 16), SCI_WORDLEFT);
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT + ((SCMOD_SHIFT + SCMOD_CTRL) << 16), SCI_WORDLEFTEXTEND);

		execute(SCI_ASSIGNCMDKEY, SCK_LEFT, SCI_CHARRIGHT);
		execute(SCI_ASSIGNCMDKEY, SCK_LEFT + (SCMOD_SHIFT << 16), SCI_CHARRIGHTEXTEND);
		execute(SCI_ASSIGNCMDKEY, SCK_LEFT + ((SCMOD_SHIFT + SCMOD_ALT) << 16), SCI_CHARRIGHTRECTEXTEND);
		execute(SCI_ASSIGNCMDKEY, SCK_LEFT + (SCMOD_CTRL << 16), SCI_WORDRIGHT);
		execute(SCI_ASSIGNCMDKEY, SCK_LEFT + ((SCMOD_SHIFT + SCMOD_CTRL) << 16), SCI_WORDRIGHTEXTEND);
	}
	else
	{
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT, SCI_CHARRIGHT);
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT + (SCMOD_SHIFT << 16), SCI_CHARRIGHTEXTEND);
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT + ((SCMOD_SHIFT + SCMOD_ALT) << 16), SCI_CHARRIGHTRECTEXTEND);
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT + (SCMOD_CTRL << 16), SCI_WORDRIGHT);
		execute(SCI_ASSIGNCMDKEY, SCK_RIGHT + ((SCMOD_SHIFT + SCMOD_CTRL) << 16), SCI_WORDRIGHTEXTEND);

		execute(SCI_ASSIGNCMDKEY, SCK_LEFT, SCI_CHARLEFT);
		execute(SCI_ASSIGNCMDKEY, SCK_LEFT + (SCMOD_SHIFT << 16), SCI_CHARLEFTEXTEND);
		execute(SCI_ASSIGNCMDKEY, SCK_LEFT + ((SCMOD_SHIFT + SCMOD_ALT) << 16), SCI_CHARLEFTRECTEXTEND);
		execute(SCI_ASSIGNCMDKEY, SCK_LEFT + (SCMOD_CTRL << 16), SCI_WORDLEFT);
		execute(SCI_ASSIGNCMDKEY, SCK_LEFT + ((SCMOD_SHIFT + SCMOD_CTRL) << 16), SCI_WORDLEFTEXTEND);
	}
}

generic_string ScintillaEditView::getEOLString()
{
	intptr_t eol_mode = execute(SCI_GETEOLMODE);
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
	long style = static_cast<long>(::GetWindowLongPtr(_hSelf, GWL_STYLE));
	long exStyle = static_cast<long>(::GetWindowLongPtr(_hSelf, GWL_EXSTYLE));

	if (NppDarkMode::isEnabled())
	{
		exStyle &= ~WS_EX_CLIENTEDGE;

		if (doWithBorderEdge)
			style |= WS_BORDER;
		else
			style &= ~WS_BORDER;
	}
	else
	{
		style &= ~WS_BORDER;

		if (doWithBorderEdge)
			exStyle |= WS_EX_CLIENTEDGE;
		else
			exStyle &= ~WS_EX_CLIENTEDGE;
	}

	::SetWindowLongPtr(_hSelf, GWL_STYLE, style);
	::SetWindowLongPtr(_hSelf, GWL_EXSTYLE, exStyle);
	::SetWindowPos(_hSelf, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void ScintillaEditView::getFoldColor(COLORREF& fgColor, COLORREF& bgColor, COLORREF& activeFgColor)
{
	StyleArray & stylers = NppParameters::getInstance().getMiscStylerArray();

	const Style * pStyle = stylers.findByName(TEXT("Fold"));
	if (pStyle)
	{
		fgColor = pStyle->_bgColor;
		bgColor = pStyle->_fgColor;
	}

	pStyle = stylers.findByName(TEXT("Fold active"));
	if (pStyle)
	{
		activeFgColor = pStyle->_fgColor;
	}
}

int ScintillaEditView::getTextZoneWidth() const
{
	RECT editorRect;
	getClientRect(editorRect);

	intptr_t marginWidths = 0;
	for (int m = 0; m < 4; ++m)
	{
		marginWidths += execute(SCI_GETMARGINWIDTHN, m);
	}
	return editorRect.right - editorRect.left - static_cast<LONG>(marginWidths);
}

pair<size_t, size_t> ScintillaEditView::getSelectedCharsAndLinesCount(long long maxSelectionsForLineCount /* = -1 */) const
{
	pair<size_t, size_t> selectedCharsAndLines(0, 0);

	selectedCharsAndLines.first = getUnicodeSelectedLength();

	size_t numSelections = execute(SCI_GETSELECTIONS);

	if (numSelections == 1)
	{
		pair<size_t, size_t> lineRange = getSelectionLinesRange();
		selectedCharsAndLines.second = lineRange.second - lineRange.first + 1;
	}
	else if (execute(SCI_SELECTIONISRECTANGLE))
	{
		selectedCharsAndLines.second = numSelections;
	}
	else if ((maxSelectionsForLineCount == -1) ||  // -1 means process ALL of the selections
		(numSelections <= static_cast<size_t>(maxSelectionsForLineCount)))
	{
		// selections are obtained from Scintilla in the order user creates them,
		// not in a lowest-to-highest position-based order;
		// to be able to get a line-count that can't count the same line more than once,
		// we have to reorder the lines touched
		// by selection into low-to-high line number order before processing them further

		vector< pair <size_t, size_t> > v;
		for (size_t s = 0; s < numSelections; ++s)
		{
			v.push_back(getSelectionLinesRange(s));
		}
		sort(v.begin(), v.end());
		intptr_t previousSecondLine = -1;
		for (auto lineRange : v)
		{
			selectedCharsAndLines.second += lineRange.second - lineRange.first;
			if (lineRange.first != static_cast<size_t>(previousSecondLine))
			{
				++selectedCharsAndLines.second;
			}
			previousSecondLine = static_cast<intptr_t>(lineRange.second);
		}
	}

	return selectedCharsAndLines;
};

size_t ScintillaEditView::getUnicodeSelectedLength() const
{
	size_t length = 0;
	size_t numSelections = execute(SCI_GETSELECTIONS);

	for (size_t s = 0; s < numSelections; ++s)
	{
		size_t start = execute(SCI_GETSELECTIONNSTART, s);
		size_t end = execute(SCI_GETSELECTIONNEND, s);
		length += execute(SCI_COUNTCHARACTERS, start, end);
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
	std::vector<std::pair<size_t, generic_string>> styledVect;

	const generic_string cr = TEXT("\r");
	const generic_string lf = TEXT("\n");

	bool textContainsLineEndingChar = false;

	for (int si = 0; styleIndicators[si] != -1; ++si)
	{
		size_t pos = execute(SCI_INDICATOREND, styleIndicators[si], 0);
		if (pos > 0)
		{
			bool atEndOfIndic = execute(SCI_INDICATORVALUEAT, styleIndicators[si], 0) != 0;
			size_t prevPos = pos;
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
				pos = execute(SCI_INDICATOREND, styleIndicators[si], pos);
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

void ScintillaEditView::removeAnyDuplicateLines()
{
	size_t fromLine = 0, toLine = 0;
	bool hasLineSelection = false;

	auto selStart = execute(SCI_GETSELECTIONSTART);
	auto selEnd = execute(SCI_GETSELECTIONEND);
	hasLineSelection = selStart != selEnd;

	if (hasLineSelection)
	{
		const pair<size_t, size_t> lineRange = getSelectionLinesRange();
		// One single line selection is not allowed.
		if (lineRange.first == lineRange.second)
		{
			return;
		}
		fromLine = lineRange.first;
		toLine = lineRange.second;
	}
	else
	{
		// No selection.
		fromLine = 0;
		toLine = execute(SCI_GETLINECOUNT) - 1;
	}

	if (fromLine >= toLine)
	{
		return;
	}

	const auto startPos = execute(SCI_POSITIONFROMLINE, fromLine);
	const auto endPos = execute(SCI_POSITIONFROMLINE, toLine) + execute(SCI_LINELENGTH, toLine);
	const generic_string text = getGenericTextAsString(startPos, endPos);
	std::vector<generic_string> linesVect = stringSplit(text, getEOLString());
	const size_t lineCount = execute(SCI_GETLINECOUNT);

	const bool doingEntireDocument = toLine == lineCount - 1;
	if (!doingEntireDocument)
	{
		if (linesVect.rbegin()->empty())
		{
			linesVect.pop_back();
		}
	}

	size_t origSize = linesVect.size();
	size_t newSize = vecRemoveDuplicates(linesVect);
	if (origSize != newSize)
	{
		generic_string joined = stringJoin(linesVect, getEOLString());
		if (!doingEntireDocument)
		{
			joined += getEOLString();
		}
		if (text != joined)
		{
			replaceTarget(joined.c_str(), startPos, endPos);
		}
	}
}
