// This file is part of Notepad++ project
// Copyright (C)2025 Don HO <don.h@free.fr>

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


#pragma once

#include <windows.h>

#include <cstdint>

inline constexpr bool dirUp = true;
inline constexpr bool dirDown = false;

inline constexpr int NPP_CP_WIN_1252 = 1252;
inline constexpr int NPP_CP_DOS_437 = 437;
inline constexpr int NPP_CP_BIG5 = 950;

#define LINKTRIGGERED WM_USER+555

#define BCKGRD_COLOR (RGB(255,102,102))
#define TXT_COLOR    (RGB(255,255,255))

#ifndef __MINGW32__
#define WCSTOK wcstok
#else
#define WCSTOK wcstok_s
#endif


#define NPP_INTERNAL_FUNCTION_STR L"Notepad++::InternalFunction"

#define REBARBAND_SIZE sizeof(REBARBANDINFO)

#define IDT_HIDE_TOOLTIP 1001

#define NPP_UAC_SAVE_SIGN L"#UAC-SAVE#"
#define NPP_UAC_SETFILEATTRIBUTES_SIGN L"#UAC-SETFILEATTRIBUTES#"
#define NPP_UAC_MOVEFILE_SIGN L"#UAC-MOVEFILE#"
#define NPP_UAC_CREATEEMPTYFILE_SIGN L"#UAC-CREATEEMPTYFILE#"

enum class SubclassID : unsigned int
{
	darkMode = 42,
	first = 666,
	second = 1984
};

// ScintillaRef

enum changeHistoryState
{
	disable,
	margin,
	indicator,
	marginIndicator
};

enum folderStyle
{
	FOLDER_TYPE,
	FOLDER_STYLE_SIMPLE,
	FOLDER_STYLE_ARROW,
	FOLDER_STYLE_CIRCLE,
	FOLDER_STYLE_BOX,
	FOLDER_STYLE_NONE
};

enum lineWrapMethod
{
	LINEWRAP_DEFAULT,
	LINEWRAP_ALIGNED,
	LINEWRAP_INDENT
};

enum lineHiliteMode
{
	LINEHILITE_NONE,
	LINEHILITE_HILITE,
	LINEHILITE_FRAME
};

// ScintillaRef

// ScintillaEditView

inline constexpr bool fold_expand = true;
inline constexpr bool fold_collapse = false;

inline constexpr int MODEVENTMASK_OFF = 0;

enum TextCase : unsigned char
{
	UPPERCASE,
	LOWERCASE,
	PROPERCASE_FORCE,
	PROPERCASE_BLEND,
	SENTENCECASE_FORCE,
	SENTENCECASE_BLEND,
	INVERTCASE,
	RANDOMCASE
};

enum NumBase : unsigned char
{
	BASE_10,
	BASE_16,
	BASE_08,
	BASE_02,
	BASE_16_UPPERCASE
};

// ScintillaEditView

// documentMap

inline constexpr UINT DOCUMENTMAP_MOUSEWHEEL = (WM_USER + 3);

// documentMap

// Parameters

inline constexpr bool POS_VERTICAL = true;
inline constexpr bool POS_HORIZONTAL = false;

inline constexpr int UDD_SHOW   = 0x01; // 0000 0001
inline constexpr int UDD_DOCKED = 0x02; // 0000 0010

// 0 : 0000 0000 hide & undocked
// 1 : 0000 0001 show & undocked
// 2 : 0000 0010 hide & docked
// 3 : 0000 0011 show & docked

inline constexpr int TAB_DRAWTOPBAR            =    0x0001;    // 0000 0000 0000 0001
inline constexpr int TAB_DRAWINACTIVETAB       =    0x0002;    // 0000 0000 0000 0010
inline constexpr int TAB_DRAGNDROP             =    0x0004;    // 0000 0000 0000 0100
inline constexpr int TAB_REDUCE                =    0x0008;    // 0000 0000 0000 1000
inline constexpr int TAB_CLOSEBUTTON           =    0x0010;    // 0000 0000 0001 0000
inline constexpr int TAB_DBCLK2CLOSE           =    0x0020;    // 0000 0000 0010 0000
inline constexpr int TAB_VERTICAL              =    0x0040;    // 0000 0000 0100 0000
inline constexpr int TAB_MULTILINE             =    0x0080;    // 0000 0000 1000 0000
inline constexpr int TAB_HIDE                  =    0x0100;    // 0000 0001 0000 0000
inline constexpr int TAB_QUITONEMPTY           =    0x0200;    // 0000 0010 0000 0000
inline constexpr int TAB_ALTICONS              =    0x0400;    // 0000 0100 0000 0000
inline constexpr int TAB_PINBUTTON             =    0x0800;    // 0000 1000 0000 0000
inline constexpr int TAB_INACTIVETABSHOWBUTTON =    0x1000;    // 0001 0000 0000 0000
inline constexpr int TAB_SHOWONLYPINNEDBUTTON  =    0x2000;    // 0010 0000 0000 0000

inline constexpr bool activeText = true;
inline constexpr bool activeNumeric = false;

enum class EolType : std::uint8_t
{
	windows,
	macos,
	unix,

	// special values
	unknown, // cannot be the first value for legacy code
	osdefault = windows,
};

enum UniMode
{
	uni8Bit       = 0,  // ANSI
	uniUTF8       = 1,  // UTF-8 with BOM
	uni16BE       = 2,  // UTF-16 Big Endian with BOM
	uni16LE       = 3,  // UTF-16 Little Endian with BOM
	uniUTF8_NoBOM = 4,  // UTF-8 without BOM
	uni7Bit       = 5,  // 0 - 127 ASCII
	uni16BE_NoBOM = 6,  // UTF-16 Big Endian without BOM
	uni16LE_NoBOM = 7,  // UTF-16 Little Endian without BOM
	uniEnd
};

enum ChangeDetect
{
	cdDisabled      = 0x00,
	cdEnabledOld    = 0x01,
	cdEnabledNew    = 0x02,
	cdAutoUpdate    = 0x04,
	cdGo2end        = 0x08
};

enum BackupFeature
{
	bak_none,
	bak_simple,
	bak_verbose
};

enum OpenSaveDirSetting
{
	dir_followCurrent,
	dir_last,
	dir_userDef
};

enum MultiInstSetting
{
	monoInst,
	multiInstOnSession,
	multiInst
};

enum writeTechnologyEngine
{
	defaultTechnology,
	directWriteTechnology,
	directWriteRetainTechnology,
	directWriteDcTechnology,
	directWriteDX11Technology,
	directWriteTechnologyUnavailable
};

enum urlMode
{
	urlDisable,
	urlNoUnderLineFg,
	urlUnderLineFg,
	urlNoUnderLineBg,
	urlUnderLineBg,

	urlMin = urlDisable,
	urlMax = urlUnderLineBg
};

enum AutoIndentMode
{
	autoIndent_none,
	autoIndent_advanced,
	autoIndent_basic
};
enum SysTrayAction
{
	sta_none,
	sta_minimize,
	sta_close,
	sta_minimize_close
};

enum LangIdxStyle
{
	LANG_INDEX_INSTR = 0,
	LANG_INDEX_INSTR2 = 1,
	LANG_INDEX_TYPE = 2,
	LANG_INDEX_TYPE2 = 3,
	LANG_INDEX_TYPE3 = 4,
	LANG_INDEX_TYPE4 = 5,
	LANG_INDEX_TYPE5 = 6,
	LANG_INDEX_TYPE6 = 7,
	LANG_INDEX_TYPE7 = 8,
	LANG_INDEX_SUBSTYLE1 = 9,
	LANG_INDEX_SUBSTYLE2 = 10,
	LANG_INDEX_SUBSTYLE3 = 11,
	LANG_INDEX_SUBSTYLE4 = 12,
	LANG_INDEX_SUBSTYLE5 = 13,
	LANG_INDEX_SUBSTYLE6 = 14,
	LANG_INDEX_SUBSTYLE7 = 15,
	LANG_INDEX_SUBSTYLE8 = 16
};

enum CopyDataParam
{
	COPYDATA_PARAMS = 0,
	//COPYDATA_FILENAMESA = 1, // obsolete, no more useful
	COPYDATA_FILENAMESW = 2,
	COPYDATA_FULL_CMDLINE = 3
};

enum UdlDecSep
{
	DECSEP_DOT,
	DECSEP_COMMA,
	DECSEP_BOTH
};

inline constexpr int NPP_STYLING_FILESIZE_LIMIT_DEFAULT = (200 * 1024 * 1024); // 200MB+ file won't be styled

inline constexpr int FINDREPLACE_MAXLENGTH = 16384; // the maximum length of the string (decrease 1 for '\0') to search in the editor

inline constexpr int FINDREPLACE_INSELECTION_THRESHOLD_DEFAULT = 1024;
inline constexpr int FILL_FINDWHAT_THRESHOLD_DEFAULT = 1024;

inline constexpr const wchar_t fontSizeStrs[][3]{ L"", L"5", L"6", L"7", L"8", L"9", L"10", L"11", L"12", L"14", L"16", L"18", L"20", L"22", L"24", L"26", L"28" };

enum FontStyle
{
	FONTSTYLE_NONE = 0,
	FONTSTYLE_BOLD = 1,
	FONTSTYLE_ITALIC = 2,
	FONTSTYLE_UNDERLINE = 4
};

inline constexpr int STYLE_NOT_USED = -1;

inline constexpr int COLORSTYLE_FOREGROUND = 0x01;
inline constexpr int COLORSTYLE_BACKGROUND = 0x02;
inline constexpr int COLORSTYLE_ALL = COLORSTYLE_FOREGROUND | COLORSTYLE_BACKGROUND;

inline constexpr COLORREF g_cDefaultMainDark = RGB(0xDE, 0xDE, 0xDE);
inline constexpr COLORREF g_cDefaultSecondaryDark = RGB(0x4C, 0xC2, 0xFF);
inline constexpr COLORREF g_cDefaultMainLight = RGB(0x21, 0x21, 0x21);
inline constexpr COLORREF g_cDefaultSecondaryLight = RGB(0x00, 0x78, 0xD4);

enum class FluentColor
{
	defaultColor,
	red,
	green,
	blue,
	purple,
	cyan,
	olive,
	yellow,
	accent,
	custom,
	maxValue
};

inline constexpr int NB_LIST = 20;
inline constexpr int NB_MAX_LRF_FILE = 30;
inline constexpr int NB_MAX_USER_LANG = 30;
inline constexpr int NB_MAX_EXTERNAL_LANG = 30;
inline constexpr int NB_MAX_IMPORTED_UDL = 50;

inline constexpr int NB_DEFAULT_LRF_CUSTOMLENGTH = 100;
inline constexpr int NB_MAX_LRF_CUSTOMLENGTH = MAX_PATH - 1;

inline constexpr int NB_MAX_TAB_COMPACT_LABEL_LEN = MAX_PATH - 3; // -3 for the possible ending ellipsis (...)

inline constexpr int MAX_EXTERNAL_LEXER_NAME_LEN = 128;

inline constexpr int RECENTFILES_SHOWFULLPATH = -1;
inline constexpr int RECENTFILES_SHOWONLYFILENAME = 0;

// Parameters

// UserDefineLangReference

inline constexpr int langNameLenMax = 64;
inline constexpr int extsLenMax = 256;
inline constexpr int max_char = 1024 * 30;

// UserDefineLangReference
