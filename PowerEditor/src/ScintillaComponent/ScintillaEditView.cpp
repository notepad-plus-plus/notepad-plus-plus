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
#include <bitset>
#include <shlwapi.h>
#include <cinttypes>
#include <windowsx.h>
#include <versionhelpers.h>
#include "ScintillaEditView.h"
#include "Parameters.h"
#include "localization.h"
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
const int ScintillaEditView::_SC_MARGE_SYMBOL = 1;
const int ScintillaEditView::_SC_MARGE_CHANGEHISTORY = 2;
const int ScintillaEditView::_SC_MARGE_FOLDER = 3;

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
	//_langName           _shortName                 _longName                                             _langID            _lexerID
	//
	{L"normal",           L"Normal text",            L"Normal text file",                                  L_TEXT,            "null"},
	{L"php",              L"PHP",                    L"PHP Hypertext Preprocessor file",                   L_PHP,             "phpscript"},
	{L"c",                L"C",                      L"C source file",                                     L_C,               "cpp"},
	{L"cpp",              L"C++",                    L"C++ source file",                                   L_CPP,             "cpp"},
	{L"cs",               L"C#",                     L"C# source file",                                    L_CS,              "cpp"},
	{L"objc",             L"Objective-C",            L"Objective-C source file",                           L_OBJC,            "objc"},
	{L"java",             L"Java",                   L"Java source file",                                  L_JAVA,            "cpp"},
	{L"rc",               L"RC",                     L"Windows Resource file",                             L_RC,              "cpp"},
	{L"html",             L"HTML",                   L"Hyper Text Markup Language file",                   L_HTML,            "hypertext"},
	{L"xml",              L"XML",                    L"eXtensible Markup Language file",                   L_XML,             "xml"},
	{L"makefile",         L"Makefile",               L"Makefile",                                          L_MAKEFILE,        "makefile"},
	{L"pascal",           L"Pascal",                 L"Pascal source file",                                L_PASCAL,          "pascal"},
	{L"batch",            L"Batch",                  L"Batch file",                                        L_BATCH,           "batch"},
	{L"ini",              L"ini",                    L"MS ini file",                                       L_INI,             "props"},
	{L"nfo",              L"NFO",                    L"MSDOS Style/ASCII Art",                             L_ASCII,           "null"},
	{L"udf",              L"udf",                    L"User Defined language file",                        L_USER,            "user"},
	{L"asp",              L"ASP",                    L"Active Server Pages script file",                   L_ASP,             "hypertext"},
	{L"sql",              L"SQL",                    L"Structured Query Language file",                    L_SQL,             "sql"},
	{L"vb",               L"Visual Basic",           L"Visual Basic file",                                 L_VB,              "vb"},
	{L"javascript",       L"JavaScript",             L"JavaScript file",                                   L_JS,              "cpp"},
	{L"css",              L"CSS",                    L"Cascade Style Sheets File",                         L_CSS,             "css"},
	{L"perl",             L"Perl",                   L"Perl source file",                                  L_PERL,            "perl"},
	{L"python",           L"Python",                 L"Python file",                                       L_PYTHON,          "python"},
	{L"lua",              L"Lua",                    L"Lua source File",                                   L_LUA,             "lua"},
	{L"tex",              L"TeX",                    L"TeX file",                                          L_TEX,             "tex"},
	{L"fortran",          L"Fortran free form",      L"Fortran free form source file",                     L_FORTRAN,         "fortran"},
	{L"bash",             L"Shell",                  L"Unix script file",                                  L_BASH,            "bash"},
	{L"actionscript",     L"ActionScript",           L"Flash ActionScript file",                           L_FLASH,           "cpp"},
	{L"nsis",             L"NSIS",                   L"Nullsoft Scriptable Install System script file",    L_NSIS,            "nsis"},
	{L"tcl",              L"TCL",                    L"Tool Command Language file",                        L_TCL,             "tcl"},
	{L"lisp",             L"Lisp",                   L"List Processing language file",                     L_LISP,            "lisp"},
	{L"scheme",           L"Scheme",                 L"Scheme file",                                       L_SCHEME,          "lisp"},
	{L"asm",              L"Assembly",               L"Assembly language source file",                     L_ASM,             "asm"},
	{L"diff",             L"Diff",                   L"Diff file",                                         L_DIFF,            "diff"},
	{L"props",            L"Properties file",        L"Properties file",                                   L_PROPS,           "props"},
	{L"postscript",       L"PostScript",             L"PostScript file",                                   L_PS,              "ps"},
	{L"ruby",             L"Ruby",                   L"Ruby file",                                         L_RUBY,            "ruby"},
	{L"smalltalk",        L"Smalltalk",              L"Smalltalk file",                                    L_SMALLTALK,       "smalltalk"},
	{L"vhdl",             L"VHDL",                   L"VHSIC Hardware Description Language file",          L_VHDL,            "vhdl"},
	{L"kix",              L"KiXtart",                L"KiXtart file",                                      L_KIX,             "kix"},
	{L"autoit",           L"AutoIt",                 L"AutoIt",                                            L_AU3,             "au3"},
	{L"caml",             L"CAML",                   L"Categorical Abstract Machine Language",             L_CAML,            "caml"},
	{L"ada",              L"Ada",                    L"Ada file",                                          L_ADA,             "ada"},
	{L"verilog",          L"Verilog",                L"Verilog file",                                      L_VERILOG,         "verilog"},
	{L"matlab",           L"MATLAB",                 L"MATrix LABoratory",                                 L_MATLAB,          "matlab"},
	{L"haskell",          L"Haskell",                L"Haskell",                                           L_HASKELL,         "haskell"},
	{L"inno",             L"Inno Setup",             L"Inno Setup script",                                 L_INNO,            "inno"},
	{L"searchResult",     L"Internal Search",        L"Internal Search",                                   L_SEARCHRESULT,    "searchResult"},
	{L"cmake",            L"CMake",                  L"CMake file",                                        L_CMAKE,           "cmake"},
	{L"yaml",             L"YAML",                   L"YAML Ain't Markup Language",                        L_YAML,            "yaml"},
	{L"cobol",            L"COBOL",                  L"COmmon Business Oriented Language",                 L_COBOL,           "COBOL"},
	{L"gui4cli",          L"Gui4Cli",                L"Gui4Cli file",                                      L_GUI4CLI,         "gui4cli"},
	{L"d",                L"D",                      L"D programming language",                            L_D,               "d"},
	{L"powershell",       L"PowerShell",             L"Windows PowerShell",                                L_POWERSHELL,      "powershell"},
	{L"r",                L"R",                      L"R programming language",                            L_R,               "r"},
	{L"jsp",              L"JSP",                    L"JavaServer Pages script file",                      L_JSP,             "hypertext"},
	{L"coffeescript",     L"CoffeeScript",           L"CoffeeScript file",                                 L_COFFEESCRIPT,    "coffeescript"},
	{L"json",             L"json",                   L"JSON file",                                         L_JSON,            "json"},
	{L"javascript.js",    L"JavaScript",             L"JavaScript file",                                   L_JAVASCRIPT,      "cpp"},
	{L"fortran77",        L"Fortran fixed form",     L"Fortran fixed form source file",                    L_FORTRAN_77,      "f77"},
	{L"baanc",            L"BaanC",                  L"BaanC File",                                        L_BAANC,           "baan"},
	{L"srec",             L"S-Record",               L"Motorola S-Record binary data",                     L_SREC,            "srec"},
	{L"ihex",             L"Intel HEX",              L"Intel HEX binary data",                             L_IHEX,            "ihex"},
	{L"tehex",            L"Tektronix extended HEX", L"Tektronix extended HEX binary data",                L_TEHEX,           "tehex"},
	{L"swift",            L"Swift",                  L"Swift file",                                        L_SWIFT,           "cpp"},
	{L"asn1",             L"ASN.1",                  L"Abstract Syntax Notation One file",                 L_ASN1,            "asn1"},
	{L"avs",              L"AviSynth",               L"AviSynth scripts files",                            L_AVS,             "avs"},
	{L"blitzbasic",       L"BlitzBasic",             L"BlitzBasic file",                                   L_BLITZBASIC,      "blitzbasic"},
	{L"purebasic",        L"PureBasic",              L"PureBasic file",                                    L_PUREBASIC,       "purebasic"},
	{L"freebasic",        L"FreeBasic",              L"FreeBasic file",                                    L_FREEBASIC,       "freebasic"},
	{L"csound",           L"Csound",                 L"Csound file",                                       L_CSOUND,          "csound"},
	{L"erlang",           L"Erlang",                 L"Erlang file",                                       L_ERLANG,          "erlang"},
	{L"escript",          L"ESCRIPT",                L"ESCRIPT file",                                      L_ESCRIPT,         "escript"},
	{L"forth",            L"Forth",                  L"Forth file",                                        L_FORTH,           "forth"},
	{L"latex",            L"LaTeX",                  L"LaTeX file",                                        L_LATEX,           "latex"},
	{L"mmixal",           L"MMIXAL",                 L"MMIXAL file",                                       L_MMIXAL,          "mmixal"},
	{L"nim",              L"Nim",                    L"Nim file",                                          L_NIM,             "nimrod"},
	{L"nncrontab",        L"Nncrontab",              L"extended crontab file",                             L_NNCRONTAB,       "nncrontab"},
	{L"oscript",          L"OScript",                L"OScript source file",                               L_OSCRIPT,         "oscript"},
	{L"rebol",            L"REBOL",                  L"REBOL file",                                        L_REBOL,           "rebol"},
	{L"registry",         L"registry",               L"registry file",                                     L_REGISTRY,        "registry"},
	{L"rust",             L"Rust",                   L"Rust file",                                         L_RUST,            "rust"},
	{L"spice",            L"Spice",                  L"spice file",                                        L_SPICE,           "spice"},
	{L"txt2tags",         L"txt2tags",               L"txt2tags file",                                     L_TXT2TAGS,        "txt2tags"},
	{L"visualprolog",     L"Visual Prolog",          L"Visual Prolog file",                                L_VISUALPROLOG,    "visualprolog"},
	{L"typescript",       L"TypeScript",             L"TypeScript file",                                   L_TYPESCRIPT,      "cpp"},
	{L"json5",            L"json5",                  L"JSON5 file",                                        L_JSON5,           "json"},
	{L"mssql",            L"mssql",                  L"Microsoft Transact-SQL (SQL Server) file",          L_MSSQL,           "mssql"},
	{L"gdscript",         L"GDScript",               L"GDScript file",                                     L_GDSCRIPT,        "gdscript"},
	{L"hollywood",        L"Hollywood",              L"Hollywood script",                                  L_HOLLYWOOD,       "hollywood"},
	{L"go",               L"Go",                     L"Go source file",                                    L_GOLANG,          "cpp"},
	{L"raku",             L"Raku",                   L"Raku source file",                                  L_RAKU,            "raku"},
	{L"toml",             L"TOML",                   L"Tom's Obvious Minimal Language file",               L_TOML,            "toml"},
	{L"ext",              L"External",               L"External",                                          L_EXTERNAL,        "null"}
};


size_t getNbDigits(size_t aNum, size_t base)
{
	size_t nbDigits = 0;

	do
	{
		++nbDigits;
		aNum /= base;
	} while (aNum != 0);

	return nbDigits;
}

bool isCharSingleQuote(__inout wchar_t const c)
{
    if (c == L'\'' || c == L'\u2019' || c == L'\u2018') return true;
    else return false;
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
					L"Scintilla",\
					L"Notepad++",\
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

	execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
	execute(SCI_SETIDLESTYLING, SC_IDLESTYLING_ALL, 0);
	execute(SCI_SETMARGINMASKN, _SC_MARGE_FOLDER, SC_MASK_FOLDERS);
	showMargin(_SC_MARGE_FOLDER, true);

	execute(SCI_SETMARGINMASKN, _SC_MARGE_SYMBOL, (1 << MARK_BOOKMARK) | (1 << MARK_HIDELINESBEGIN) | (1 << MARK_HIDELINESEND));

	execute(SCI_SETMARGINMASKN, _SC_MARGE_CHANGEHISTORY, (1 << SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN) | (1 << SC_MARKNUM_HISTORY_SAVED) | (1 << SC_MARKNUM_HISTORY_MODIFIED) | (1 << SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED));
	COLORREF modifiedColor = RGB(255, 128, 0);
	//COLORREF savedColor = RGB(0, 255, 0);
	//COLORREF revertedToModifiedColor = RGB(255, 255, 0);
	//COLORREF revertedToOriginColor = RGB(0, 0, 255);
	execute(SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_MODIFIED, modifiedColor);
	//execute(SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_SAVED, savedColor);
	//execute(SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED, revertedToModifiedColor);
	//execute(SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN, revertedToOriginColor);

	execute(SCI_MARKERSETALPHA, MARK_BOOKMARK, 70);

	const COLORREF hiddenLinesGreen = RGB(0x77, 0xCC, 0x77);
	long hiddenLinesGreenWithAlpha = hiddenLinesGreen | 0xFF000000;
	setElementColour(SC_ELEMENT_HIDDEN_LINE, hiddenLinesGreenWithAlpha);

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

    execute(SCI_SETMARGINSENSITIVEN, _SC_MARGE_FOLDER, true); // Make margin sensitive for getting notification on mouse click
    execute(SCI_SETMARGINSENSITIVEN, _SC_MARGE_SYMBOL, true); // Make margin sensitive for getting notification on mouse click

    execute(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED);
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

	NppGUI& nppGui = (NppParameters::getInstance()).getNppGUI();

	HMODULE hNtdllModule = ::GetModuleHandle(L"ntdll.dll");
	FARPROC isWINE = nullptr;
	if (hNtdllModule)
		isWINE = ::GetProcAddress(hNtdllModule, "wine_get_version");

	if (isWINE || // There is a performance issue under WINE when DirectWrite is ON, so we turn it off if user uses Notepad++ under WINE
		::IsWindowsServer()) // In the case of Windows Server Core, DirectWrite cannot be on.
		nppGui._writeTechnologyEngine = defaultTechnology;

	if (nppGui._writeTechnologyEngine == directWriteTechnology)
	{
		execute(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE);
		// If useDirectWrite is turned off, leave the technology setting untouched,
		// so that existing plugins using SCI_SETTECHNOLOGY behave like before
	}

	_codepage = ::GetACP();

	::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	_callWindowProc = CallWindowProc;
	_scintillaDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(scintillaStatic_Proc)));

	if (_defaultCharList.empty())
	{
		auto defaultCharListLen = execute(SCI_GETWORDCHARS);
		char *defaultCharList = new char[defaultCharListLen + 1];
		if(defaultCharList)
		{
			execute(SCI_GETWORDCHARS, 0, reinterpret_cast<LPARAM>(defaultCharList));
			defaultCharList[defaultCharListLen] = '\0';
			_defaultCharList = defaultCharList;
			delete[] defaultCharList;
		}
	}
	execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
	//Get the startup document and make a buffer for it so it can be accessed like a file
	attachDefaultDoc();
}

LRESULT CALLBACK ScintillaEditView::scintillaStatic_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	ScintillaEditView *pScint = (ScintillaEditView *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (Message == WM_MOUSEWHEEL || Message == WM_MOUSEHWHEEL)
	{
		POINT pt{};
		POINTS pts = MAKEPOINTS(lParam);
		POINTSTOPOINT(pt, pts);
		HWND hwndOnMouse = WindowFromPoint(pt);

		//Hack for Synaptics TouchPad Driver
		char synapticsHack[26]{};
		GetClassNameA(hwndOnMouse, (LPSTR)&synapticsHack, 26);
		bool isSynpnatic = std::string(synapticsHack) == "SynTrackCursorWindowClass";
		bool makeTouchPadCompetible = ((NppParameters::getInstance()).getSVP())._disableAdvancedScrolling;

		if (pScint && (isSynpnatic || makeTouchPadCompetible))
			return (pScint->scintillaNew_Proc(hwnd, Message, wParam, lParam));

		const ScintillaEditView* pScintillaOnMouse = reinterpret_cast<const ScintillaEditView *>(::GetWindowLongPtr(hwndOnMouse, GWLP_USERDATA));
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
				intptr_t					textLength = 0;
				intptr_t					selectSize = 0;
				char				smallTextBuffer[128] = { '\0' };
				char			  *	selectedStr = smallTextBuffer;
				RECONVERTSTRING   *	reconvert = (RECONVERTSTRING *)lParam;

				// does nothing with a rectangular selection
				if (execute(SCI_SELECTIONISRECTANGLE, 0, 0))
					return 0;

				// get the codepage of the text

				size_t cp = execute(SCI_GETCODEPAGE);
				UINT codepage = static_cast<UINT>(cp);

				// get the current text selection

				Sci_CharacterRangeFull range = getSelection();
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
					textLength = sizeof(wchar_t) * ::MultiByteToWideChar(codepage, 0, selectedStr, (int)selectSize, NULL, 0);
				}
				else
				{
					// convert the selection to Unicode, and store it at the end of the structure.
					// Beware: For a Unicode IME, dwStrLen , dwCompStrLen, and dwTargetStrLen
					// are wchar_t values, that is, character counts. The members dwStrOffset,
					// dwCompStrOffset, and dwTargetStrOffset specify byte counts.

					textLength = ::MultiByteToWideChar(	codepage, 0,
														selectedStr, (int)selectSize,
														(LPWSTR)((LPSTR)reconvert + sizeof(RECONVERTSTRING)),
														static_cast<int>(reconvert->dwSize - sizeof(RECONVERTSTRING)));

					// fill the structure
					reconvert->dwVersion		 = 0;
					reconvert->dwStrLen			 = static_cast<DWORD>(textLength);
					reconvert->dwStrOffset		 = sizeof(RECONVERTSTRING);
					reconvert->dwCompStrLen		 = static_cast<DWORD>(textLength);
					reconvert->dwCompStrOffset	 = 0;
					reconvert->dwTargetStrLen	 = reconvert->dwCompStrLen;
					reconvert->dwTargetStrOffset = reconvert->dwCompStrOffset;

					textLength *= sizeof(wchar_t);
				}

				if (selectedStr != smallTextBuffer)
					delete [] selectedStr;

				// return the total length of the structure
				return sizeof(RECONVERTSTRING) + textLength;
			}
			break;
		}

		case WM_CHAR:
		{
			// prevent "control characters" from being entered in text
			// (don't need to be concerned about Tab or CR or LF etc here)
			if ((NppParameters::getInstance()).getSVP()._npcNoInputC0 &&
				(wParam <= 31 || wParam == 127))
			{
				return FALSE;
			}
			break;
		}

		case WM_KEYUP:
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

		case WM_KEYDOWN:
		{
			struct MultiCaretInfo {
				int _len2remove;
				size_t _selIndex;
				MultiCaretInfo(int len, size_t n) : _len2remove(len), _selIndex(n) {};
			};

			SHORT ctrl = GetKeyState(VK_CONTROL);
			SHORT alt = GetKeyState(VK_MENU);
			SHORT shift = GetKeyState(VK_SHIFT);
			bool isColumnSelection = (execute(SCI_GETSELECTIONMODE) == SC_SEL_RECTANGLE) || (execute(SCI_GETSELECTIONMODE) == SC_SEL_THIN);
			bool column2MultSelect = (NppParameters::getInstance()).getSVP()._columnSel2MultiEdit;

			if (wParam == VK_DELETE)
			{
				if (!(shift & 0x8000) && !(ctrl & 0x8000) && !(alt & 0x8000)) // DEL & Multi-edit
				{
					size_t nbSelections = execute(SCI_GETSELECTIONS);
					if (nbSelections > 1) // Multi-edit
					{
						vector<MultiCaretInfo> edgeOfEol; // parir <start, end>, pair <len2remove, selN>
						int nbCaseForScint = 0;

						for (size_t i = 0; i < nbSelections; ++i)
						{
							LRESULT posStart = execute(SCI_GETSELECTIONNSTART, i);
							LRESULT posEnd = execute(SCI_GETSELECTIONNEND, i);
							if (posStart != posEnd)
							{
								++nbCaseForScint;
							}
							else // posStart == posEnd)
							{
								size_t docLen = getCurrentDocLen();

								char eolStr[3];
								Sci_TextRangeFull tr;
								tr.chrg.cpMin = posStart;
								tr.chrg.cpMax = posEnd + 2;
								if (tr.chrg.cpMax > static_cast<Sci_Position>(docLen))
								{
									tr.chrg.cpMax = docLen;
								}
								tr.lpstrText = eolStr;

								if (tr.chrg.cpMin != tr.chrg.cpMax)
									execute(SCI_GETTEXTRANGEFULL, 0, reinterpret_cast<LPARAM>(&tr));

								// Remember EOL length
								// in the case of other characters let Scintilla do its job
								int len2remove = -1;

								if (eolStr[0] == '\r' && eolStr[1] == '\n')
									len2remove = 2;
								else if (eolStr[0] == '\r' || eolStr[0] == '\n')
									len2remove = 1;

								if (len2remove == -1)
									++nbCaseForScint;
								else
									edgeOfEol.push_back(MultiCaretInfo(len2remove, i));
							}
						}

						execute(SCI_BEGINUNDOACTION);

						// Let Scitilla do its job, if any
						if (nbCaseForScint)
							_callWindowProc(_scintillaDefaultProc, hwnd, Message, wParam, lParam);

						// then do our job, if it's not column mode
						if (!isColumnSelection)
						{
							for (const auto& i : edgeOfEol)
							{
								// because the current caret modification will change the other caret positions,
								// so we get them dynamically in the loop.
								LRESULT posStart = execute(SCI_GETSELECTIONNSTART, i._selIndex);
								LRESULT posEnd = execute(SCI_GETSELECTIONNEND, i._selIndex);

								replaceTarget(L"", posStart, posEnd + i._len2remove);
								execute(SCI_SETSELECTIONNSTART, i._selIndex, posStart);
								execute(SCI_SETSELECTIONNEND, i._selIndex, posStart);
							}
						}

						execute(SCI_ENDUNDOACTION);

						return TRUE;

					}
				}
			}
			else if (isColumnSelection && column2MultSelect)
			{
				//
				// Transform the column selection to multi-edit
				//
				switch (wParam)
				{
					case VK_LEFT:
					case VK_RIGHT:
					case VK_UP:
					case VK_DOWN:
					case VK_HOME:
					case VK_END:
					case VK_RETURN:
					case VK_BACK:
						execute(SCI_SETSELECTIONMODE, SC_SEL_STREAM); // When it's rectangular selection and the arrow keys are pressed, we switch the mode for having multiple carets.

						execute(SCI_SETSELECTIONMODE, SC_SEL_STREAM); // the 2nd call for removing the unwanted selection while moving carets.
																	  // Solution suggested by Neil Hodgson. See:
																	  // https://sourceforge.net/p/scintilla/bugs/2412/
						break;
	
					case VK_ESCAPE:
					{
						int selection = static_cast<int>(execute(SCI_GETMAINSELECTION, 0, 0));
						int caret = static_cast<int>(execute(SCI_GETSELECTIONNCARET, selection, 0));
						execute(SCI_SETSELECTION, caret, caret);
						execute(SCI_SETSELECTIONMODE, SC_SEL_STREAM);
						break;
					}

					default:
						break;

				}

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

void ScintillaEditView::setHotspotStyle(const Style& styleToSet)
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
		const Style * pStyle = NppParameters::getInstance().getMiscStylerArray().findByName(L"Global override");
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
		const wchar_t *pKwArray[NB_LIST] = {NULL};
		
		setLexerFromLangID(L_XML);
		
		for (int i = 0 ; i < 4 ; ++i)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(L""));

        makeStyle(type, pKwArray);

		// the XML portion of the lexer only allows substyles for attributes, not for tags (since it treats all tags the same),
		//	so allocate all 8 substyles to attributes
		populateSubStyleKeywords(type, SCE_H_ATTRIBUTE, 8, LANG_INDEX_SUBSTYLE1, pKwArray);

		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.xml.allow.scripts"), reinterpret_cast<LPARAM>("0"));
	}
	else if ((type == L_HTML) || (type == L_PHP) || (type == L_ASP) || (type == L_JSP))
	{
		setLexerFromLangID(L_HTML);

        setHTMLLexer();
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

void ScintillaEditView::setHTMLLexer()
{
	const wchar_t *pKwArray[NB_LIST] = {NULL};
	makeStyle(L_HTML, pKwArray);

	basic_string<char> keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordList = wstring2string(kwlW, CP_ACP);
	}

	execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, L_HTML, LANG_INDEX_INSTR)));
	
	// HTML allows substyle lists for both tags and attributes, so allocate four of each
	populateSubStyleKeywords(L_HTML, SCE_H_TAG, 4, LANG_INDEX_SUBSTYLE1, pKwArray);
	populateSubStyleKeywords(L_HTML, SCE_H_ATTRIBUTE, 4, LANG_INDEX_SUBSTYLE5, pKwArray);
}

void ScintillaEditView::setEmbeddedJSLexer()
{
	const wchar_t *pKwArray[NB_LIST] = {NULL};
	makeStyle(L_JS, pKwArray);

	basic_string<char> keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordList = wstring2string(kwlW, CP_ACP);
	}

	execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, L_JS, LANG_INDEX_INSTR)));
	populateSubStyleKeywords(L_JS, SCE_HJ_WORD, 8, LANG_INDEX_SUBSTYLE1, pKwArray);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_DEFAULT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_COMMENTDOC, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJ_TEMPLATELITERAL, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HJA_TEMPLATELITERAL, true);
}

void ScintillaEditView::setJsonLexer(bool isJson5)
{
	setLexerFromLangID(isJson5 ? L_JSON5 : L_JSON);

	const wchar_t *pKwArray[NB_LIST] = {NULL};

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

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.json.escape.sequence"), reinterpret_cast<LPARAM>("1"));

	if (isJson5)
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.json.allow.comments"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::setEmbeddedPhpLexer()
{
	const wchar_t *pKwArray[NB_LIST] = {NULL};
	makeStyle(L_PHP, pKwArray);

	basic_string<char> keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordList = wstring2string(kwlW, CP_ACP);
	}

	execute(SCI_SETKEYWORDS, 4, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, L_PHP, LANG_INDEX_INSTR)));
	populateSubStyleKeywords(L_PHP, SCE_HPHP_WORD, 8, LANG_INDEX_SUBSTYLE1, pKwArray);

	execute(SCI_STYLESETEOLFILLED, SCE_HPHP_DEFAULT, true);
	execute(SCI_STYLESETEOLFILLED, SCE_HPHP_COMMENT, true);
}

void ScintillaEditView::setEmbeddedAspLexer()
{
	const wchar_t *pKwArray[NB_LIST] = {NULL};
	makeStyle(L_ASP, pKwArray);

	basic_string<char> keywordList("");
	if (pKwArray[LANG_INDEX_INSTR])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR];
		keywordList = wstring2string(kwlW, CP_ACP);
	}

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("asp.default.language"), reinterpret_cast<LPARAM>("2"));

	execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(getCompleteKeywordList(keywordList, L_VB, LANG_INDEX_INSTR)));

	populateSubStyleKeywords(L_ASP, SCE_HB_WORD, 8, LANG_INDEX_SUBSTYLE1, pKwArray);

    execute(SCI_STYLESETEOLFILLED, SCE_HBA_DEFAULT, true);
}

void ScintillaEditView::setUserLexer(const wchar_t *userLangName)
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
	sprintf(intBuffer, "%" PRIuPTR, reinterpret_cast<uintptr_t>(userLangContainer->getName())); // use numeric value of wchar_t pointer
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

	::SendMessage(_hParent, NPPM_INTERNAL_EXTERNALLEXERBUFFER, 0, (LPARAM)getCurrentBufferID());

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
    const char *cppGlobalclass;
    const wchar_t *doxygenKeyWords  = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);

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

	const wchar_t *pKwArray[NB_LIST] = {NULL};
	makeStyle(langType, pKwArray);

	basic_string<char> keywordListInstruction("");
	basic_string<char> keywordListType("");
	basic_string<char> keywordListGlobalclass("");
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

	if (pKwArray[LANG_INDEX_INSTR2])
	{
		basic_string<wchar_t> kwlW = pKwArray[LANG_INDEX_INSTR2];
		keywordListGlobalclass = wstring2string(kwlW, CP_ACP);
	}
	cppGlobalclass = getCompleteKeywordList(keywordListGlobalclass, langType, LANG_INDEX_INSTR2);

	execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(cppInstrs));
	execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(cppTypes));
	execute(SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(cppGlobalclass));

	populateSubStyleKeywords(langType, SCE_C_IDENTIFIER, 8, LANG_INDEX_SUBSTYLE1, pKwArray);

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
	const wchar_t *doxygenKeyWords = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);

	setLexerFromLangID(L_JAVASCRIPT);
	const wchar_t *pKwArray[NB_LIST] = {NULL};
	makeStyle(L_JAVASCRIPT, pKwArray);

	if (doxygenKeyWords)
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char * doxygenKeyWords_char = wmc.wchar2char(doxygenKeyWords, CP_ACP);
		execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(doxygenKeyWords_char));
	}

	const wchar_t *newLexerName = ScintillaEditView::_langNameInfoArray[L_JAVASCRIPT]._langName;
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

		populateSubStyleKeywords(L_JAVASCRIPT, SCE_C_IDENTIFIER, 8, LANG_INDEX_SUBSTYLE1, pKwArray);

	}
	else // New js styler is not available, we use the old styling for the sake of retro-compatibility
	{
		const wchar_t *lexerName = ScintillaEditView::_langNameInfoArray[L_JS]._langName;
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
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.backquoted.strings"), reinterpret_cast<LPARAM>("2"));
}

void ScintillaEditView::setTclLexer()
{
	const char *tclInstrs;
    const char *tclTypes;


	setLexerFromLangID(L_TCL);

	const wchar_t *pKwArray[NB_LIST] = {NULL};
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

	const wchar_t *pKwArray[NB_LIST] = {NULL};

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
	const wchar_t *doxygenKeyWordsW = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);
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
	const wchar_t* doxygenKeyWords = NppParameters::getInstance().getWordList(L_CPP, LANG_INDEX_TYPE2);
	setLexerFromLangID(L_TYPESCRIPT);

	if (doxygenKeyWords)
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char* doxygenKeyWords_char = wmc.wchar2char(doxygenKeyWords, CP_ACP);
		execute(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(doxygenKeyWords_char));
	}

	const wchar_t *pKwArray[NB_LIST] = {NULL};
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

	std::string keywordListInstruction = getKeywordList(LANG_INDEX_INSTR);
	const char* tsInstructions = getCompleteKeywordList(keywordListInstruction, L_TYPESCRIPT, LANG_INDEX_INSTR);

	string keywordListType = getKeywordList(LANG_INDEX_TYPE);
	const char* tsTypes = getCompleteKeywordList(keywordListType, L_TYPESCRIPT, LANG_INDEX_TYPE);

	execute(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(tsInstructions));
	execute(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(tsTypes));

	populateSubStyleKeywords(L_TYPESCRIPT, SCE_C_IDENTIFIER, 8, LANG_INDEX_SUBSTYLE1, pKwArray);

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

void ScintillaEditView::populateSubStyleKeywords(LangType langType, int baseStyleID, int numSubStyles, int firstLangIndex, const wchar_t **pKwArray)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	int firstID = execute(SCI_ALLOCATESUBSTYLES, baseStyleID, numSubStyles) & 0xFF;

	if(pKwArray && (firstID>=0))
	{
		for (int i = 0; i < numSubStyles; i++)
		{
			int ss = firstLangIndex + i;
			int styleID = firstID + i;
			basic_string<char> userWords = pKwArray[ss] ? wmc.wchar2char(pKwArray[ss], CP_ACP) : "";
			execute(SCI_SETIDENTIFIERS, styleID, reinterpret_cast<LPARAM>(getCompleteKeywordList(userWords, langType, ss)));
		}
	}
}

void ScintillaEditView::setLexer(LangType langType, int whichList, int baseStyleID, int numSubStyles)
{
	setLexerFromLangID(langType);

	const wchar_t *pKwArray[NB_LIST] = {NULL};

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
	
	if (baseStyleID != STYLE_NOT_USED)
	{
		populateSubStyleKeywords(langType, baseStyleID, numSubStyles, LANG_INDEX_SUBSTYLE1, pKwArray);
	}

	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));
	execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
}

void ScintillaEditView::makeStyle(LangType language, const wchar_t **keywordArray)
{
	const wchar_t * lexerName = ScintillaEditView::_langNameInfoArray[language]._langName;
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
		Style* pStyle = stylers.findByName(L"EOL custom color");
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

void ScintillaEditView::setNpcAndCcUniEOL(long color)
{
	NppParameters& nppParams = NppParameters::getInstance();
	const ScintillaViewParams& svp = nppParams.getSVP();

	COLORREF npcCustomColor = liteGrey;

	if (color == -1)
	{
		StyleArray& stylers = nppParams.getMiscStylerArray();
		Style* pStyle = stylers.findByName(g_npcStyleName);
		if (pStyle)
		{
			npcCustomColor = pStyle->_fgColor;
		}
	}
	else
	{
		npcCustomColor = color;
	}

	const long appearance = svp._npcCustomColor ? SC_REPRESENTATION_BLOB | SC_REPRESENTATION_COLOUR : SC_REPRESENTATION_BLOB;
	const long alphaNpcCustomColor = npcCustomColor | 0xFF000000; // add alpha color to make DirectWrite mode work

	if (svp._npcShow)
	{
		for (const auto& invChar : g_nonPrintingChars)
		{
			execute(SCI_SETREPRESENTATIONCOLOUR, reinterpret_cast<WPARAM>(invChar.at(0)), alphaNpcCustomColor);
			execute(SCI_SETREPRESENTATIONAPPEARANCE, reinterpret_cast<WPARAM>(invChar.at(0)), appearance);
		}
	}

	if (svp._ccUniEolShow && svp._npcIncludeCcUniEol)
	{
		for (const auto& invChar : g_ccUniEolChars)
		{
			execute(SCI_SETREPRESENTATIONCOLOUR, reinterpret_cast<WPARAM>(invChar.at(0)), alphaNpcCustomColor);
			execute(SCI_SETREPRESENTATIONAPPEARANCE, reinterpret_cast<WPARAM>(invChar.at(0)), appearance);
		}
	}

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
		case L_GOLANG:
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
		case L_JSON5:
			setJsonLexer(true); break;

		case L_CSS :
			setCssLexer(); break;

		case L_LUA :
			setLuaLexer(); break;

		case L_MAKEFILE :
			setMakefileLexer(); break;

		case L_INI :
			setPropsLexer(false); break;

		case L_USER : {
			const wchar_t * langExt = _currentBuffer->getUserDefineLangName();
			if (langExt[0])
				setUserLexer(langExt);
			else
				setUserLexer();
			break; }

		case L_ASCII :
		{
			LexerStyler *pStyler = (NppParameters::getInstance().getLStylerArray()).getLexerStylerByName(L"nfo");

			Style nfoStyle;
			nfoStyle._styleID = STYLE_DEFAULT;
			nfoStyle._fontName = L"Lucida Console";
			nfoStyle._fontSize = 10;

			if (pStyler)
			{
				const Style * pDefStyle = pStyler->findByName(L"DEFAULT");
				if (pDefStyle)
				{
					nfoStyle._bgColor = pDefStyle->_bgColor;
					nfoStyle._fgColor = pDefStyle->_fgColor;
					nfoStyle._colorStyle = pDefStyle->_colorStyle;
				}
			}
			setSpecialStyle(nfoStyle);
			execute(SCI_STYLECLEARALL);

			Buffer* buf = MainFileManager.getBufferByID(_currentBufferID);

			if (buf->getEncoding() == NPP_CP_DOS_437 && !buf->isDirty())
			{
				MainFileManager.reloadBuffer(buf);
			}
		}
		break;

		case L_SQL :
			setSqlLexer(); break;

		case L_MSSQL :
			setMSSqlLexer(); break;

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

		case L_GDSCRIPT:
			setGDScriptLexer(); break;

		case L_HOLLYWOOD:
			setHollywoodLexer(); break;

		case L_RAKU:
			setRakuLexer(); break;

		case L_TOML:
			setTomlLexer(); break;

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

Document ScintillaEditView::getBlankDocument()
{
	if(_blankDocument==0)
	{
		_blankDocument=static_cast<Document>(execute(SCI_CREATEDOCUMENT,0,SC_DOCUMENTOPTION_TEXT_LARGE));
		execute(SCI_ADDREFDOCUMENT,0,_blankDocument);
	}
	return _blankDocument;
}

BufferID ScintillaEditView::attachDefaultDoc()
{
	// get the doc pointer attached (by default) on the view Scintilla
	Document doc = execute(SCI_GETDOCPOINTER, 0, 0);
	execute(SCI_ADDREFDOCUMENT, 0, doc);
	BufferID id = MainFileManager.bufferFromDocument(doc, _isMainEditZone);
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
	const Position & pos = buf->getPosition(this);

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
	const Position & pos = buf->getPosition(this);

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
	if (buffer == BUFFER_INVALID) return;
	if (!force && buffer == _currentBuffer)	return;

	Buffer * newBuf = MainFileManager.getBufferByID(buffer);

	// before activating another document, we get the current position
	// from the Scintilla view then save it to the current document
	saveCurrentPos();

	// get foldStateInfo of current doc
	std::vector<size_t> lineStateVector;
	getCurrentFoldStates(lineStateVector);

	// put the state into the future ex buffer
	_currentBuffer->setHeaderLineState(lineStateVector, this);

	_prevBuffer = _currentBuffer;

	_currentBufferID = buffer;	//the magical switch happens here
	_currentBuffer = newBuf;

	const bool isSameLangType = (_prevBuffer != nullptr) && (_prevBuffer->getLangType() == _currentBuffer->getLangType());
	const int currentLangInt = static_cast<int>(_currentBuffer->getLangType());
	const bool isFirstActiveBuffer = (_currentBuffer->getLastLangType() != currentLangInt);

	if (isFirstActiveBuffer)  // Entering the tab for the 1st time
	{
		// change the doc, this operation will decrease
		// the ref count of old current doc and increase the one of the new doc. FileManager should manage the rest
		// Note that the actual reference in the Buffer itself is NOT decreased, Notepad_plus does that if neccessary
		execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
		execute(SCI_SETDOCPOINTER, 0, _currentBuffer->getDocument());
		execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);

		// Due to execute(SCI_CLEARDOCUMENTSTYLE); in defineDocType() function
		// defineDocType() function should be called here, but not be after the fold info loop
		defineDocType(_currentBuffer->getLangType());
	}
	else if (isSameLangType) // After the 2nd entering with the same language type
	{
		// No need to call defineDocType() since it's the same language type
		execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
		execute(SCI_SETDOCPOINTER, 0, _currentBuffer->getDocument());
		execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);

		if (force)
			defineDocType(_currentBuffer->getLangType());
	}
	else // Entering the tab for the 2nd or more times, with the different language type
	{
		// In order to improve the performance of switch-in on the 2nd or more times for the large files,
		// a blank document is used for accelerate defineDocType() call.
		execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
		execute(SCI_SETDOCPOINTER, 0, getBlankDocument());
		execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);

		defineDocType(_currentBuffer->getLangType());

		execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
		execute(SCI_SETDOCPOINTER, 0, _currentBuffer->getDocument());
		execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
	}

	_currentBuffer->setLastLangType(currentLangInt);

	setWordChars();

	if (_currentBuffer->getNeedsLexing())
	{
		restyleBuffer();
	}

	maintainStateForNpc();

	// Everything should be updated, but the language
	bufferUpdated(_currentBuffer, (BufferChangeMask & ~BufferChangeLanguage));

	// restore the collapsed info
	const std::vector<size_t> & lineStateVectorNew = newBuf->getHeaderLineState(this);
	syncFoldStateWith(lineStateVectorNew);

	restoreCurrentPosPreStep();

	//runMarkers(true, 0, true, false);
	restoreHiddenLines();

	setCRLF();

	NppParameters& nppParam = NppParameters::getInstance();
	const ScintillaViewParams& svp = nppParam.getSVP();

	int enabledCHFlag = SC_CHANGE_HISTORY_DISABLED;
	if (svp._isChangeHistoryMarginEnabled || svp._isChangeHistoryIndicatorEnabled)
	{
		enabledCHFlag = SC_CHANGE_HISTORY_ENABLED;

		if (svp._isChangeHistoryMarginEnabled)
			enabledCHFlag |= SC_CHANGE_HISTORY_MARKERS;

		if (svp._isChangeHistoryIndicatorEnabled)
			enabledCHFlag |= SC_CHANGE_HISTORY_INDICATORS;
	}
	execute(SCI_SETCHANGEHISTORY, enabledCHFlag);

	if (isTextDirectionRTL() != buffer->isRTL())
		changeTextDirection(buffer->isRTL());

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

		SCNotification scnN{};
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

wstring ScintillaEditView::getGenericTextAsString(size_t start, size_t end) const
{
	assert(end > start);
	const size_t bufSize = end - start + 1;
	wchar_t *buf = new wchar_t[bufSize];
	getGenericText(buf, bufSize, start, end);
	wstring text = buf;
	delete[] buf;
	return text;
}

void ScintillaEditView::getGenericText(wchar_t *dest, size_t destlen, size_t start, size_t end) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	char *destA = new char[end - start + 1];
	getText(destA, start, end);
	size_t cp = execute(SCI_GETCODEPAGE);
	const wchar_t *destW = wmc.char2wchar(destA, cp);
	wcsncpy_s(dest, destlen, destW, _TRUNCATE);
	delete [] destA;
}

// "mstart" and "mend" are pointers to indexes in the read string,
// which are converted to the corresponding indexes in the returned wchar_t string.

void ScintillaEditView::getGenericText(wchar_t *dest, size_t destlen, size_t start, size_t end, intptr_t* mstart, intptr_t* mend) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	char *destA = new char[end - start + 1];
	getText(destA, start, end);
	size_t cp = execute(SCI_GETCODEPAGE)    ;
	const wchar_t *destW = wmc.char2wchar(destA, cp, mstart, mend);
	wcsncpy_s(dest, destlen, destW, _TRUNCATE);
	delete [] destA;
}

void ScintillaEditView::insertGenericTextFrom(size_t position, const wchar_t *text2insert) const
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
	RECT rcEditView{};
	getClientRect(rcEditView);
	LRESULT pos = execute(SCI_POSITIONFROMPOINT, 0, 0);
	LRESULT line = execute(SCI_LINEFROMPOSITION, pos);
	*startPos = execute(SCI_POSITIONFROMLINE, line);
	pos = execute(SCI_POSITIONFROMPOINT, static_cast<WPARAM>(rcEditView.right - rcEditView.left), static_cast<LPARAM>(rcEditView.bottom - rcEditView.top));
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

wchar_t * ScintillaEditView::getGenericWordOnCaretPos(wchar_t * txt, int size)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	char *txtA = new char[size + 1];
	getWordOnCaretPos(txtA, size);

	const wchar_t * txtW = wmc.char2wchar(txtA, cp);
	wcscpy_s(txt, size, txtW);
	delete [] txtA;
	return txt;
}

char * ScintillaEditView::getSelectedText(char * txt, size_t size, bool expand)
{
	if (!size)
		return NULL;
	Sci_CharacterRangeFull range = getSelection();
	if (range.cpMax == range.cpMin && expand)
	{
		expandWordSelection();
		range = getSelection();
	}
	if (!(static_cast<Sci_Position>(size) > (range.cpMax - range.cpMin)))	//there must be atleast 1 byte left for zero terminator
	{
		range.cpMax = range.cpMin + size -1;	//keep room for zero terminator
	}
	//getText(txt, range.cpMin, range.cpMax);
	return getWordFromRange(txt, size, range.cpMin, range.cpMax);
}

wchar_t * ScintillaEditView::getGenericSelectedText(wchar_t * txt, int size, bool expand)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	char *txtA = new char[size + 1];
	getSelectedText(txtA, size, expand);

	const wchar_t * txtW = wmc.char2wchar(txtA, cp);
	wcscpy_s(txt, size, txtW);
	delete [] txtA;
	return txt;
}

intptr_t ScintillaEditView::searchInTarget(const wchar_t * text2Find, size_t lenOfText2Find, size_t fromPos, size_t toPos) const
{
	execute(SCI_SETTARGETRANGE, fromPos, toPos);

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2FindA = wmc.wchar2char(text2Find, cp);
	size_t text2FindALen = strlen(text2FindA);
   	size_t len = (lenOfText2Find > text2FindALen) ? lenOfText2Find : text2FindALen;
	return execute(SCI_SEARCHINTARGET, len, reinterpret_cast<LPARAM>(text2FindA));
}

void ScintillaEditView::appandGenericText(const wchar_t * text2Append) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2AppendA =wmc.wchar2char(text2Append, cp);
	execute(SCI_APPENDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

void ScintillaEditView::addGenericText(const wchar_t * text2Append) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2AppendA =wmc.wchar2char(text2Append, cp);
	execute(SCI_ADDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

void ScintillaEditView::addGenericText(const wchar_t * text2Append, intptr_t* mstart, intptr_t* mend) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *text2AppendA =wmc.wchar2char(text2Append, cp, mstart, mend);
	execute(SCI_ADDTEXT, strlen(text2AppendA), reinterpret_cast<LPARAM>(text2AppendA));
}

intptr_t ScintillaEditView::replaceTarget(const wchar_t * str2replace, intptr_t fromTargetPos, intptr_t toTargetPos) const
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

intptr_t ScintillaEditView::replaceTargetRegExMode(const wchar_t * re, intptr_t fromTargetPos, intptr_t toTargetPos) const
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

void ScintillaEditView::showAutoComletion(size_t lenEntered, const wchar_t* list)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *listA = wmc.wchar2char(list, cp);
	execute(SCI_AUTOCSHOW, lenEntered, reinterpret_cast<LPARAM>(listA));
	NppDarkMode::setDarkAutoCompletion();
}

void ScintillaEditView::showCallTip(size_t startPos, const wchar_t * def)
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	size_t cp = execute(SCI_GETCODEPAGE);
	const char *defA = wmc.wchar2char(def, cp);
	execute(SCI_CALLTIPSHOW, startPos, reinterpret_cast<LPARAM>(defA));
}

wstring ScintillaEditView::getLine(size_t lineNumber) const
{
	size_t lineLen = execute(SCI_LINELENGTH, lineNumber);
	const size_t bufSize = lineLen + 1;
	std::unique_ptr<wchar_t[]> buf = std::make_unique<wchar_t[]>(bufSize);
	getLine(lineNumber, buf.get(), bufSize);
	return buf.get();
}

void ScintillaEditView::getLine(size_t lineNumber, wchar_t * line, size_t lineBufferLen) const
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
	const wchar_t *lineW = wmc.char2wchar(lineA, cp);
	lstrcpyn(line, lineW, static_cast<int>(lineBufferLen));
	delete [] lineA;
}

void ScintillaEditView::addText(size_t length, const char *buf)
{
	execute(SCI_ADDTEXT, length, reinterpret_cast<LPARAM>(buf));
}

void ScintillaEditView::beginOrEndSelect(bool isColumnMode)
{
	auto currPos = execute(SCI_GETCURRENTPOS);

	if (_beginSelectPosition == -1)
	{
		_beginSelectPosition = currPos;
	}
	else
	{
		execute(SCI_CHANGESELECTIONMODE, static_cast<WPARAM>(isColumnMode ? SC_SEL_RECTANGLE : SC_SEL_STREAM));
		execute(isColumnMode ? SCI_SETANCHOR : SCI_SETSEL, static_cast<WPARAM>(_beginSelectPosition), static_cast<LPARAM>(currPos));
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
		if (whichMarge == _SC_MARGE_SYMBOL)
			width = dpiManager.scaleX(16);
		else if (whichMarge == _SC_MARGE_FOLDER)
			width = dpiManager.scaleX(14);

		execute(SCI_SETMARGINWIDTHN, whichMarge, willBeShowed ? width : 0);
	}
}

void ScintillaEditView::showChangeHistoryMargin(bool willBeShowed)
{
	DPIManager& dpiManager = NppParameters::getInstance()._dpiManager;
	int	width = dpiManager.scaleX(9);
	execute(SCI_SETMARGINWIDTHN, _SC_MARGE_CHANGEHISTORY, willBeShowed ? width : 0);
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
		pStyle = stylers.findByName(L"Current line background colour");
		if (pStyle)
		{
			setElementColour(SC_ELEMENT_CARET_LINE_BACK, pStyle->_bgColor);
		}
	}

	execute(SCI_SETCARETLINEFRAME, (svp._currentLineHiliteMode == LINEHILITE_FRAME) ? svp._currentLineFrameWidth : 0);

	COLORREF selectColorBack = grey;
	COLORREF selectColorFore = black;
	pStyle = stylers.findByName(L"Selected text colour");
	if (pStyle)
	{
		selectColorBack = pStyle->_bgColor;
		selectColorFore = pStyle->_fgColor;
	}
	//execute(SCI_SETSELBACK, 1, selectColorBack);
	setElementColour(SC_ELEMENT_SELECTION_BACK, selectColorBack); // SCI_SETSELBACK is deprecated
	setElementColour(SC_ELEMENT_SELECTION_INACTIVE_BACK, selectColorBack);


	COLORREF selectMultiSelectColorBack = liteGrey;
	pStyle = stylers.findByName(L"Multi-selected text color");
	if (pStyle)
	{
		selectMultiSelectColorBack = pStyle->_bgColor;
	}
	setElementColour(SC_ELEMENT_SELECTION_ADDITIONAL_BACK, selectMultiSelectColorBack);

	if (nppParams.isSelectFgColorEnabled())
	{
		//execute(SCI_SETSELFORE, 1, selectColorFore);
		setElementColour(SC_ELEMENT_SELECTION_TEXT, selectColorFore);  // SCI_SETSELFORE is deprecated
		setElementColour(SC_ELEMENT_SELECTION_INACTIVE_TEXT, selectColorFore);
		setElementColour(SC_ELEMENT_SELECTION_ADDITIONAL_TEXT, selectColorFore);
	}

	COLORREF caretColor = black;
	pStyle = stylers.findByName(L"Caret colour");
	if (pStyle)
	{
		caretColor = pStyle->_fgColor;
	}
	//execute(SCI_SETCARETFORE, caretColor);
	setElementColour(SC_ELEMENT_CARET, caretColor); // SCI_SETCARETFORE is deprecated

	COLORREF multiEditCaretColor = darkGrey;
	pStyle = stylers.findByName(L"Multi-edit carets color");

	if (pStyle)
		multiEditCaretColor = pStyle->_fgColor;

	setElementColour(SC_ELEMENT_CARET_ADDITIONAL, multiEditCaretColor);

	COLORREF edgeColor = liteGrey;
	pStyle = stylers.findByName(L"Edge colour");
	if (pStyle)
	{
		edgeColor = pStyle->_fgColor;
	}
	execute(SCI_SETEDGECOLOUR, edgeColor);
	::SendMessage(_hParent, NPPM_INTERNAL_EDGEMULTISETSIZE, 0, 0);

	COLORREF foldMarginColor = grey;
	COLORREF foldMarginHiColor = white;
	pStyle = stylers.findByName(L"Fold margin");
	if (pStyle)
	{
		foldMarginHiColor = pStyle->_fgColor;
		foldMarginColor = pStyle->_bgColor;
	}
	execute(SCI_SETFOLDMARGINCOLOUR, true, foldMarginColor);
	execute(SCI_SETFOLDMARGINHICOLOUR, true, foldMarginHiColor);

	COLORREF bookmarkMarginColor = veryLiteGrey;
	pStyle = stylers.findByName(L"Bookmark margin");
	if (!pStyle)
	{
		pStyle = stylers.findByName(L"Line number margin"); // "Line number margin" is used only for getting the bg color for _SC_MARGE_SYMBOL.
		if (pStyle)                                              // "Line number margin" has its own style (styleID="33") for setting its bg & fg color
		{
			bookmarkMarginColor = pStyle->_bgColor;
		}
	}
	else
	{
		bookmarkMarginColor = pStyle->_bgColor;
	}
	execute(SCI_SETMARGINTYPEN, _SC_MARGE_SYMBOL, SC_MARGIN_COLOUR);
	execute(SCI_SETMARGINBACKN, _SC_MARGE_SYMBOL, bookmarkMarginColor);

	COLORREF changeHistoryMarginColor = veryLiteGrey;
	pStyle = stylers.findByName(L"Change History margin");
	if (!pStyle)
	{
		pStyle = stylers.findByName(L"Line number margin");
		if (pStyle)
		{
			changeHistoryMarginColor = pStyle->_bgColor;
		}
	}
	else
	{
		changeHistoryMarginColor = pStyle->_bgColor;
	}
	execute(SCI_SETMARGINTYPEN, _SC_MARGE_CHANGEHISTORY, SC_MARGIN_COLOUR);
	execute(SCI_SETMARGINBACKN, _SC_MARGE_CHANGEHISTORY, changeHistoryMarginColor);

	COLORREF changeModifiedfgColor = orange;
	COLORREF changeModifiedbgColor = orange;
	pStyle = stylers.findByName(L"Change History modified");
	if (pStyle)
	{
		changeModifiedfgColor = pStyle->_fgColor;
		changeModifiedbgColor = pStyle->_bgColor;
	}
	execute(SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_MODIFIED, changeModifiedfgColor);
	execute(SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_MODIFIED, changeModifiedbgColor);
	execute(SCI_INDICSETFORE, INDICATOR_HISTORY_MODIFIED_INSERTION, changeModifiedfgColor);
	execute(SCI_INDICSETFORE, INDICATOR_HISTORY_MODIFIED_DELETION, changeModifiedfgColor);

	COLORREF changeRevertModifiedfgColor = yellowGreen;
	COLORREF changeRevertModifiedbgColor = yellowGreen;
	pStyle = stylers.findByName(L"Change History revert modified");
	if (pStyle)
	{
		changeRevertModifiedfgColor = pStyle->_fgColor;
		changeRevertModifiedbgColor = pStyle->_bgColor;
	}
	execute(SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED, changeRevertModifiedfgColor);
	execute(SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED, changeRevertModifiedbgColor);
	execute(SCI_INDICSETFORE, INDICATOR_HISTORY_REVERTED_TO_MODIFIED_INSERTION, changeRevertModifiedfgColor);
	execute(SCI_INDICSETFORE, INDICATOR_HISTORY_REVERTED_TO_MODIFIED_DELETION, changeRevertModifiedfgColor);	

	COLORREF changeRevertOriginfgColor = darkCyan;
	COLORREF changeRevertOriginbgColor = darkCyan;
	pStyle = stylers.findByName(L"Change History revert origin");
	if (pStyle)
	{
		changeRevertOriginfgColor = pStyle->_fgColor;
		changeRevertOriginbgColor = pStyle->_bgColor;
	}
	execute(SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN, changeRevertOriginfgColor);
	execute(SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN, changeRevertOriginbgColor);
	execute(SCI_INDICSETFORE, INDICATOR_HISTORY_REVERTED_TO_ORIGIN_INSERTION, changeRevertOriginfgColor);
	execute(SCI_INDICSETFORE, INDICATOR_HISTORY_REVERTED_TO_ORIGIN_DELETION, changeRevertOriginfgColor);

	COLORREF changeSavedfgColor = midGreen;
	COLORREF changeSavedbgColor = midGreen;
	pStyle = stylers.findByName(L"Change History saved");
	if (pStyle)
	{
		changeSavedfgColor = pStyle->_fgColor;
		changeSavedbgColor = pStyle->_bgColor;
	}
	execute(SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_SAVED, changeSavedfgColor);
	execute(SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_SAVED, changeSavedbgColor);
	execute(SCI_INDICSETFORE, INDICATOR_HISTORY_SAVED_INSERTION, changeSavedfgColor);
	execute(SCI_INDICSETFORE, INDICATOR_HISTORY_SAVED_DELETION, changeSavedfgColor);

	COLORREF urlHoveredFG = grey;
	pStyle = stylers.findByName(L"URL hovered");
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
	pStyle = stylers.findByName(L"White space symbol");
	if (pStyle)
	{
		wsSymbolFgColor = pStyle->_fgColor;
	}
	execute(SCI_SETWHITESPACEFORE, true, wsSymbolFgColor);

	COLORREF eolCustomColor = liteGrey;
	pStyle = stylers.findByName(L"EOL custom color");
	if (pStyle)
	{
		eolCustomColor = pStyle->_fgColor;
	}
	setCRLF(eolCustomColor);

	COLORREF npcCustomColor = liteGrey;
	pStyle = stylers.findByName(g_npcStyleName);
	if (pStyle)
	{
		npcCustomColor = pStyle->_fgColor;
	}
	setNpcAndCcUniEOL(npcCustomColor);
}

void ScintillaEditView::showNpc(bool willBeShowed, bool isSearchResult)
{
	const auto& svp = NppParameters::getInstance().getSVP();

	if (willBeShowed)
	{
		const auto& mode = static_cast<size_t>(svp._npcMode);
		for (const auto& invChar : g_nonPrintingChars)
		{
			execute(SCI_SETREPRESENTATION, reinterpret_cast<WPARAM>(invChar.at(0)), reinterpret_cast<LPARAM>(invChar.at(mode)));
		}

		if (svp._npcCustomColor)
		{
			setNpcAndCcUniEOL();
		}

		// in some case npc representation is not redrawn correctly on first line
		// therefore use of showEOL(isShownEol()) instead of redraw()
		showEOL(isShownEol());
	}
	else
	{
		execute(SCI_CLEARALLREPRESENTATIONS);

		// SCI_CLEARALLREPRESENTATIONS will also reset CRLF and CcUniEOL
		if (!isSearchResult && svp._eolMode != svp.roundedRectangleText)
		{
			setCRLF();
		}

		showCcUniEol(svp._ccUniEolShow);
	}
}

void ScintillaEditView::showCcUniEol(bool willBeShowed, bool isSearchResult)
{
	const auto& svp = NppParameters::getInstance().getSVP();

	if (willBeShowed)
	{
		const auto& mode = static_cast<size_t>(svp._npcIncludeCcUniEol ? svp._npcMode : ScintillaViewParams::npcMode::abbreviation);
		for (const auto& invChar : g_ccUniEolChars)
		{
			execute(SCI_SETREPRESENTATION, reinterpret_cast<WPARAM>(invChar.at(0)), reinterpret_cast<LPARAM>(invChar.at(mode)));
		}

		if (svp._npcIncludeCcUniEol && svp._npcCustomColor)
		{
			setNpcAndCcUniEOL();
		}
	}
	else
	{
		execute(SCI_CLEARALLREPRESENTATIONS);

		for (const auto& invChar : g_ccUniEolChars)
		{
			execute(SCI_SETREPRESENTATION, reinterpret_cast<WPARAM>(invChar.at(0)), reinterpret_cast<LPARAM>(g_ZWSP));
			execute(SCI_SETREPRESENTATIONAPPEARANCE, reinterpret_cast<WPARAM>(invChar.at(0)), SC_REPRESENTATION_PLAIN);
		}

		// SCI_CLEARALLREPRESENTATIONS will also reset CRLF and NPC
		if (!isSearchResult && svp._eolMode != svp.roundedRectangleText)
		{
			setCRLF();
		}

		if (svp._npcShow)
		{
			showNpc();
			return; // showEOL(isShownEol()) already in showNpc()
		}
	}

	// in some case C0, C1 and  Unicode EOL representations are not redrawn correctly on first line
	// therefore use of showEOL(isShownEol()) instead of redraw()
	showEOL(isShownEol());
}

void ScintillaEditView::showIndentGuideLine(bool willBeShowed)
{
	auto typeDoc = _currentBuffer->getLangType();
	const int docIndentMode = isPythonStyleIndentation(typeDoc) ? SC_IV_LOOKFORWARD : SC_IV_LOOKBOTH;
	execute(SCI_SETINDENTATIONGUIDES, willBeShowed ? docIndentMode : SC_IV_NONE);
}

void ScintillaEditView::setLineIndent(size_t line, size_t indent) const
{
	size_t nbSelections = execute(SCI_GETSELECTIONS);

	if (nbSelections == 1)
	{
		Sci_CharacterRangeFull crange = getSelection();
		int64_t posBefore = execute(SCI_GETLINEINDENTPOSITION, line);
		execute(SCI_SETLINEINDENTATION, line, indent);
		int64_t posAfter = execute(SCI_GETLINEINDENTPOSITION, line);
		long long posDifference = posAfter - posBefore;
		if (posAfter > posBefore)
		{
			// Move selection on
			if (crange.cpMin >= posBefore)
			{
				crange.cpMin += static_cast<Sci_Position>(posDifference);
			}
			if (crange.cpMax >= posBefore)
			{
				crange.cpMax += static_cast<Sci_Position>(posDifference);
			}
		}
		else if (posAfter < posBefore)
		{
			// Move selection back
			if (crange.cpMin >= posAfter)
			{
				if (crange.cpMin >= posBefore)
					crange.cpMin += static_cast<Sci_Position>(posDifference);
				else
					crange.cpMin = static_cast<Sci_Position>(posAfter);
			}

			if (crange.cpMax >= posAfter)
			{
				if (crange.cpMax >= posBefore)
					crange.cpMax += static_cast<Sci_Position>(posDifference);
				else
					crange.cpMax = static_cast<Sci_Position>(posAfter);
			}
		}
		execute(SCI_SETSEL, crange.cpMin, crange.cpMax);
	}
	else
	{
		execute(SCI_BEGINUNDOACTION);
		for (size_t i = 0; i < nbSelections; ++i)
		{
			LRESULT posStart = execute(SCI_GETSELECTIONNSTART, i);
			LRESULT posEnd = execute(SCI_GETSELECTIONNEND, i);
			

			size_t l = execute(SCI_LINEFROMPOSITION, posStart);
			
			int64_t posBefore = execute(SCI_GETLINEINDENTPOSITION, l);
			execute(SCI_SETLINEINDENTATION, l, indent);
			int64_t posAfter = execute(SCI_GETLINEINDENTPOSITION, l);

			long long posDifference = posAfter - posBefore;
			if (posAfter > posBefore)
			{
				// Move selection on
				if (posStart >= posBefore)
				{
					posStart += static_cast<Sci_Position>(posDifference);
				}
				if (posEnd >= posBefore)
				{
					posEnd += static_cast<Sci_Position>(posDifference);
				}
			}
			else if (posAfter < posBefore)
			{
				// Move selection back
				if (posStart >= posAfter)
				{
					if (posStart >= posBefore)
						posStart += static_cast<Sci_Position>(posDifference);
					else
						posStart = static_cast<Sci_Position>(posAfter);
				}

				if (posEnd >= posAfter)
				{
					if (posEnd >= posBefore)
						posEnd += static_cast<Sci_Position>(posDifference);
					else
						posEnd = static_cast<Sci_Position>(posAfter);
				}
			}

			execute(SCI_SETSELECTIONNSTART, i, posStart);
			execute(SCI_SETSELECTIONNEND, i, posEnd);
		}
		execute(SCI_ENDUNDOACTION);
	}
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
	const wchar_t *defKwl_generic = NppParameters::getInstance().getWordList(langType, keywordIndex);

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
	size_t nbSelections = execute(SCI_GETSELECTIONS);

	size_t start_pos, end_pos;

	if ((selectionNumber < 0) || (static_cast<size_t>(selectionNumber) >= nbSelections))
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
				strWToConvert[i] = (wchar_t)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
			}
			break; 
		} //case UPPERCASE
		case LOWERCASE:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				strWToConvert[i] = (wchar_t)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
			}
			break; 
		} //case LOWERCASE
		case PROPERCASE_FORCE:
		case PROPERCASE_BLEND:
		{
			for (int i = 0; i < nbChars; ++i)
			{
				if (::IsCharAlphaW(strWToConvert[i]))
				{
					// Exception for single quote and smart single quote
					if ((i < 2) ? false :
						(isCharSingleQuote(strWToConvert[i - 1]) && ::IsCharAlphaNumericW(strWToConvert[i - 2])))
					{
						if (caseToConvert == PROPERCASE_FORCE)
							strWToConvert[i] = (wchar_t)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
					}
					else if ((i < 1) ? true : !::IsCharAlphaNumericW(strWToConvert[i - 1]))
						strWToConvert[i] = (wchar_t)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
					else if (caseToConvert == PROPERCASE_FORCE)
						strWToConvert[i] = (wchar_t)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
				}
			}
			break;
		} //case PROPERCASE
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
						strWToConvert[i] = (wchar_t)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
						isNewSentence = false;
					}
					else if (caseToConvert == SENTENCECASE_FORCE)
					{
						strWToConvert[i] = (wchar_t)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
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
					strWToConvert[i] = (wchar_t)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
				else
					strWToConvert[i] = (wchar_t)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
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
						strWToConvert[i] = (wchar_t)(UINT_PTR)::CharUpperW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
					else
						strWToConvert[i] = (wchar_t)(UINT_PTR)::CharLowerW(reinterpret_cast<LPWSTR>(strWToConvert[i]));
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
    pair<size_t, size_t> wordRange = getWordRange();
    if (wordRange.first != wordRange.second)
	{
        execute(SCI_SETSELECTIONSTART, wordRange.first);
        execute(SCI_SETSELECTIONEND, wordRange.second);
		return true;
	}
	return false;
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

void ScintillaEditView::columnReplace(ColumnModeInfos & cmi, const wchar_t *str)
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

void ScintillaEditView::columnReplace(ColumnModeInfos & cmi, size_t initial, size_t incr, size_t repeat, UCHAR format, ColumnEditorParam::leadingChoice lead)
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

	//Defined in ScintillaEditView.h :
	//const UCHAR MASK_FORMAT = 0x03;

	UCHAR f = format & MASK_FORMAT;

	int base = 10;
	if (f == BASE_16)
		base = 16;
	else if (f == BASE_08)
		base = 8;
	else if (f == BASE_02)
		base = 2;

	const int stringSize = 512;
	char str[stringSize];

	// Compute the numbers to be placed at each column.
	std::vector<size_t> numbers;

	size_t curNumber = initial;
	const size_t kiMaxSize = cmi.size();
	while (numbers.size() < kiMaxSize)
	{
		for (size_t i = 0; i < repeat; i++)
		{
			numbers.push_back(curNumber);
			if (numbers.size() >= kiMaxSize)
			{
				break;
			}
		}
		curNumber += incr;
	}

	const size_t kibEnd = getNbDigits(*numbers.rbegin(), base);
	const size_t kibInit = getNbDigits(initial, base);
	const size_t kib = std::max<size_t>(kibInit, kibEnd);

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

			variedFormatNumber2String<char>(str, stringSize, numbers.at(i), base, kib, lead);

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
			execute(SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(str));

			if (hasVirtualSpc)
			{
				totalDiff += cmi[i]._nbVirtualAnchorSpc + strlen(str);
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
}


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

	int scope = 0;
	bool recentMarkerWasOpen = false;

	auto removeMarker = [this, &scope, &recentMarkerWasOpen](size_t line, int markerMask)
	{
		auto state = execute(SCI_MARKERGET, line) & markerMask;
		bool closePresent = (state & (1 << MARK_HIDELINESEND)) != 0;
		bool openPresent = (state & (1 << MARK_HIDELINESBEGIN)) != 0;

		if (closePresent)
		{
			execute(SCI_MARKERDELETE, line, MARK_HIDELINESEND);
			recentMarkerWasOpen = false;
			--scope;
		}

		if (openPresent)
		{
			execute(SCI_MARKERDELETE, line, MARK_HIDELINESBEGIN);
			recentMarkerWasOpen = true;
			++scope;
		}
	};

	size_t startMarker = startLine - 1;
	size_t endMarker = endLine + 1;

	// Previous markers must be removed in the selected region:

	removeMarker(startMarker, 1 << MARK_HIDELINESBEGIN);
	for (size_t i = startLine; i <= endLine; ++i)
		removeMarker(i, (1 << MARK_HIDELINESBEGIN) | (1 << MARK_HIDELINESEND));
	removeMarker(endMarker, 1 << MARK_HIDELINESEND);

	// When hiding lines just below/above other hidden lines,
	// merge them into one hidden section:

	if (scope == 0 && recentMarkerWasOpen)
	{
		// Special case: user wants to hide every line in between other hidden sections.
		// Both "while" loops are executed (merge with above AND below hidden section):

		while (scope == 0 && static_cast<intptr_t>(startMarker) >= 0)
			removeMarker(--startMarker, 1 << MARK_HIDELINESBEGIN);

		while (scope != 0 && endMarker < nbLines)
			removeMarker(++endMarker, 1 << MARK_HIDELINESEND);
	}
	else
	{
		// User wants to hide some lines below/above other hidden section.
		// If true, only one "while" loop is executed (merge with adjacent hidden section):

		while (scope < 0 && static_cast<intptr_t>(startMarker) >= 0)
			removeMarker(--startMarker, 1 << MARK_HIDELINESBEGIN);

		while (scope > 0 && endMarker < nbLines)
			removeMarker(++endMarker, 1 << MARK_HIDELINESEND);
	}

	execute(SCI_MARKERADD, startMarker, MARK_HIDELINESBEGIN);
	execute(SCI_MARKERADD, endMarker, MARK_HIDELINESEND);

	_currentBuffer->setHideLineChanged(true, startMarker);
}

bool ScintillaEditView::markerMarginClick(intptr_t lineNumber)
{
	auto state = execute(SCI_MARKERGET, lineNumber);
	bool openPresent = (state & (1 << MARK_HIDELINESBEGIN)) != 0;
	bool closePresent = (state & (1 << MARK_HIDELINESEND)) != 0;

	if (!openPresent && !closePresent)
		return false;
		
	//Special func on buffer. First call show with location of opening marker. Then remove the marker manually
	if (openPresent)
	{
		closePresent = false; // when there are two overlapping markers, always open the lower section
		_currentBuffer->setHideLineChanged(false, lineNumber);
	}

	if (closePresent)
	{
		openPresent = false;
		intptr_t i = lineNumber - 1;
		for (; i >= 0 && !openPresent; i--)
		{
			state = execute(SCI_MARKERGET, i);
			openPresent = (state & (1 << MARK_HIDELINESBEGIN)) != 0;
		}

		if (openPresent)
		{
			_currentBuffer->setHideLineChanged(false, i + 1);
		}
		else // problem -> only close but no open: let's remove the errno close marker
		{
			execute(SCI_MARKERDELETE, lineNumber, MARK_HIDELINESEND);
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
			if ((state & (1 << MARK_HIDELINESBEGIN)) != 0)
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
			if ((state & (1 << MARK_HIDELINESBEGIN)) != 0 && !isInSection)
			{
				isInSection = true;
				if (doDelete)
				{
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESBEGIN);
				}
				else
				{
					startShowing = i + 1;
				}
			}
			else if ( (state & (1 << MARK_HIDELINESEND)) != 0)
			{
				if (doDelete)
				{
					execute(SCI_MARKERDELETE, i, MARK_HIDELINESEND);
					if (!endOfDoc)
					{
						return;	//done, only single section requested
					}	//otherwise keep going
					isInSection = false;
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
							isInSection = false; // assume we passed the close tag
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

void ScintillaEditView::restoreHiddenLines()
{
	int line = 0;

	while (line != -1)
	{
		line = static_cast<int>(execute(SCI_MARKERNEXT, line, 1 << MARK_HIDELINESBEGIN));
		
		if (line != -1)
		{
			int startHiding = line + 1;
			line = static_cast<int>(execute(SCI_MARKERNEXT, line, 1 << MARK_HIDELINESEND));

			if (line != -1)
			{
				execute(SCI_HIDELINES, startHiding, line - 1);

			}
		}
	}
}




void ScintillaEditView::setTabSettings(Lang* lang)
{
	if (lang && lang->_tabSize != -1 && lang->_tabSize != 0)
	{
		if (lang->_langID == L_JAVASCRIPT)
		{
			Lang* ljs = NppParameters::getInstance().getLangFromID(L_JS);
			execute(SCI_SETTABWIDTH, ljs->_tabSize > 0 ? ljs->_tabSize : lang->_tabSize);
			execute(SCI_SETUSETABS, !ljs->_isTabReplacedBySpace);
			execute(SCI_SETBACKSPACEUNINDENTS, ljs->_isBackspaceUnindent);
		}
		else
		{
			execute(SCI_SETTABWIDTH, lang->_tabSize);
			execute(SCI_SETUSETABS, !lang->_isTabReplacedBySpace);
			execute(SCI_SETBACKSPACEUNINDENTS, lang->_isBackspaceUnindent);
		}
	}
	else
	{
		const NppGUI& nppgui = NppParameters::getInstance().getNppGUI();
		execute(SCI_SETTABWIDTH, nppgui._tabSize > 0 ? nppgui._tabSize : 4);
		execute(SCI_SETUSETABS, !nppgui._tabReplacedBySpace);
		execute(SCI_SETBACKSPACEUNINDENTS, nppgui._backspaceUnindent);
	}
}

void ScintillaEditView::insertNewLineAboveCurrentLine()
{
	wstring newline = getEOLString();
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
	wstring newline = getEOLString();
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
	const wstring text = getGenericTextAsString(startPos, endPos);
	std::vector<wstring> splitText;
	stringSplit(text, getEOLString(), splitText);
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
	pSort->sort(splitText);
	wstring joined;
	stringJoin(splitText, getEOLString(), joined);

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
	if (isTextDirectionRTL() == isRTL)
		return;

	NppParameters& nppParamInst = NppParameters::getInstance();
	if (isRTL && nppParamInst.getNppGUI()._writeTechnologyEngine == directWriteTechnology) // RTL is not compatible with Direct Write Technology
	{
		static bool theWarningIsGiven = false;

		if (!theWarningIsGiven)
		{
			(nppParamInst.getNativeLangSpeaker())->messageBox("RTLvsDirectWrite",
				getHSelf(),
				L"RTL is not compatible with Direct Write mode. Please disable DirectWrite mode in MISC. section of Preferences dialog, and restart Notepad++.",
				L"Cannot run RTL",
				MB_OK | MB_APPLMODAL);

			theWarningIsGiven = true;
		}
		return;
	}

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

	Buffer* buf = getCurrentBuffer();
	buf->setRTL(isRTL);
}

wstring ScintillaEditView::getEOLString() const
{
	intptr_t eol_mode = execute(SCI_GETEOLMODE);
	if (eol_mode == SC_EOL_CRLF)
	{
		return L"\r\n";
	}
	else if (eol_mode == SC_EOL_LF)
	{
		return L"\n";
	}
	else
	{
		return L"\r";
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

	const Style * pStyle = stylers.findByName(L"Fold");
	if (pStyle)
	{
		fgColor = pStyle->_bgColor;
		bgColor = pStyle->_fgColor;
	}

	pStyle = stylers.findByName(L"Fold active");
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

	size_t nbSelections = execute(SCI_GETSELECTIONS);

	if (nbSelections == 1)
	{
		pair<size_t, size_t> lineRange = getSelectionLinesRange();
		selectedCharsAndLines.second = lineRange.second - lineRange.first + 1;
	}
	else if (execute(SCI_SELECTIONISRECTANGLE))
	{
		selectedCharsAndLines.second = nbSelections;
	}
	else if ((maxSelectionsForLineCount == -1) ||  // -1 means process ALL of the selections
		(nbSelections <= static_cast<size_t>(maxSelectionsForLineCount)))
	{
		// selections are obtained from Scintilla in the order user creates them,
		// not in a lowest-to-highest position-based order;
		// to be able to get a line-count that can't count the same line more than once,
		// we have to reorder the lines touched
		// by selection into low-to-high line number order before processing them further

		vector< pair <size_t, size_t> > v;
		for (size_t s = 0; s < nbSelections; ++s)
		{
			v.push_back(getSelectionLinesRange(s));
		}
		sort(v.begin(), v.end());
		intptr_t previousSecondLine = -1;
		for (const auto& lineRange : v)
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
}

size_t ScintillaEditView::getUnicodeSelectedLength() const
{
	size_t length = 0;
	size_t nbSelections = execute(SCI_GETSELECTIONS);

	for (size_t s = 0; s < nbSelections; ++s)
	{
		size_t start = execute(SCI_GETSELECTIONNSTART, s);
		size_t end = execute(SCI_GETSELECTIONNEND, s);
		length += execute(SCI_COUNTCHARACTERS, start, end);
	}

	return length;
}


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
	std::vector<std::pair<size_t, wstring>> styledVect;

	const wstring cr = L"\r";
	const wstring lf = L"\n";

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
					wstring styledText = getGenericTextAsString(prevPos, pos);
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

		const wstring delim =
			(textContainsLineEndingChar && styledVect.size() > 1) ?
			L"\r\n----\r\n" : L"\r\n";

		wstring joined;
		for (const auto& item : styledVect)
		{
			joined += delim + item.second;
		}
		joined = joined.substr(delim.length());
		if (styledVect.size() > 1)
		{
			joined += L"\r\n";
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
	const wstring text = getGenericTextAsString(startPos, endPos);
	std::vector<wstring> linesVect;
	stringSplit(text, getEOLString(), linesVect);
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
		wstring joined;
		stringJoin(linesVect, getEOLString(), joined);
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

bool ScintillaEditView::pasteToMultiSelection() const
{
	size_t nbSelections = execute(SCI_GETSELECTIONS);
	if (nbSelections <= 1)
		return false;

	// "MSDEVColumnSelect" is column format from Scintilla 
	CLIPFORMAT cfColumnSelect = static_cast<CLIPFORMAT>(::RegisterClipboardFormat(L"MSDEVColumnSelect"));
	if (!::IsClipboardFormatAvailable(cfColumnSelect) || !::OpenClipboard(NULL))
		return false;

	HANDLE clipboardData = ::GetClipboardData(CF_UNICODETEXT);
	if (!clipboardData)
	{
		::CloseClipboard();
		return false;
	}

	LPVOID clipboardDataPtr = ::GlobalLock(clipboardData);
	if (!clipboardDataPtr)
	{
		::CloseClipboard();
		return false;
	}

	wstring clipboardStr = static_cast<const wchar_t*>(clipboardDataPtr);
	::GlobalUnlock(clipboardData);
	::CloseClipboard();

	vector<wstring> clipboardStrings;
	stringSplit(clipboardStr, getEOLString(), clipboardStrings);
	clipboardStrings.erase(clipboardStrings.cend() - 1); // remove the last empty string
	size_t nbClipboardStr = clipboardStrings.size();

	if (nbSelections >= nbClipboardStr) // enough holes for every insertion, keep holes empty if there are some left
	{
		execute(SCI_BEGINUNDOACTION);
		for (size_t i = 0; i < nbClipboardStr; ++i)
		{
			LRESULT posStart = execute(SCI_GETSELECTIONNSTART, i);
			LRESULT posEnd = execute(SCI_GETSELECTIONNEND, i);
			replaceTarget(clipboardStrings[i].c_str(), posStart, posEnd);
			posStart += clipboardStrings[i].length();
			execute(SCI_SETSELECTIONNSTART, i, posStart);
			execute(SCI_SETSELECTIONNEND, i, posStart);
		}
		execute(SCI_ENDUNDOACTION);
		return true;
	}
	else if (nbSelections < nbClipboardStr) // not enough holes for insertion, every hole has several insertions
	{
		size_t nbStr2takeFromClipboard = nbClipboardStr / nbSelections;

		execute(SCI_BEGINUNDOACTION);
		size_t j = 0;
		for (size_t i = 0; i < nbSelections; ++i)
		{
			LRESULT posStart = execute(SCI_GETSELECTIONNSTART, i);
			LRESULT posEnd = execute(SCI_GETSELECTIONNEND, i);
			wstring severalStr;
			wstring eol = getEOLString();
			for (size_t k = 0; k < nbStr2takeFromClipboard && j < nbClipboardStr; ++k)
			{
				severalStr += clipboardStrings[j];
				severalStr += eol;
				++j;
			}

			// remove the latest added EOL
			severalStr.erase(severalStr.length() - eol.length());

			replaceTarget(severalStr.c_str(), posStart, posEnd);
			posStart += severalStr.length();
			execute(SCI_SETSELECTIONNSTART, i, posStart);
			execute(SCI_SETSELECTIONNEND, i, posStart);
		}
		execute(SCI_ENDUNDOACTION);
		return true;
	}

	return false;
}
