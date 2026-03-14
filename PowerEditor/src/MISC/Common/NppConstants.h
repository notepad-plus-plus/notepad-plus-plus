     1|// This file is part of npminmin project
     2|// Copyright (C)2025 Don HO <don.h@free.fr>
     3|
     4|// This program is free software: you can redistribute it and/or modify
     5|// it under the terms of the GNU General Public License as published by
     6|// the Free Software Foundation, either version 3 of the License, or
     7|// at your option any later version.
     8|//
     9|// This program is distributed in the hope that it will be useful,
    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|// GNU General Public License for more details.
    13|//
    14|// You should have received a copy of the GNU General Public License
    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|
    17|
    18|#pragma once
    19|
    20|#include <windows.h>
    21|
    22|#include <cstdint>
    23|
    24|inline constexpr bool dirUp = true;
    25|inline constexpr bool dirDown = false;
    26|
    27|inline constexpr int NPP_CP_WIN_1252 = 1252;
    28|inline constexpr int NPP_CP_DOS_437 = 437;
    29|inline constexpr int NPP_CP_BIG5 = 950;
    30|
    31|#define LINKTRIGGERED WM_USER+555
    32|
    33|#define BCKGRD_COLOR (RGB(255,102,102))
    34|#define TXT_COLOR    (RGB(255,255,255))
    35|
    36|#ifndef __MINGW32__
    37|#define WCSTOK wcstok
    38|#else
    39|#define WCSTOK wcstok_s
    40|#endif
    41|
    42|
    43|#define NPP_INTERNAL_FUNCTION_STR L"npminmin::InternalFunction"
    44|
    45|#define REBARBAND_SIZE sizeof(REBARBANDINFO)
    46|
    47|#define IDT_HIDE_TOOLTIP 1001
    48|
    49|#define NPP_UAC_SAVE_SIGN L"#UAC-SAVE#"
    50|#define NPP_UAC_SETFILEATTRIBUTES_SIGN L"#UAC-SETFILEATTRIBUTES#"
    51|#define NPP_UAC_MOVEFILE_SIGN L"#UAC-MOVEFILE#"
    52|#define NPP_UAC_CREATEEMPTYFILE_SIGN L"#UAC-CREATEEMPTYFILE#"
    53|
    54|enum class SubclassID : unsigned int
    55|{
    56|	darkMode = 42,
    57|	first = 666,
    58|	second = 1984
    59|};
    60|
    61|// ScintillaRef
    62|
    63|enum changeHistoryState
    64|{
    65|	disable,
    66|	margin,
    67|	indicator,
    68|	marginIndicator
    69|};
    70|
    71|enum folderStyle
    72|{
    73|	FOLDER_TYPE,
    74|	FOLDER_STYLE_SIMPLE,
    75|	FOLDER_STYLE_ARROW,
    76|	FOLDER_STYLE_CIRCLE,
    77|	FOLDER_STYLE_BOX,
    78|	FOLDER_STYLE_NONE
    79|};
    80|
    81|enum lineWrapMethod
    82|{
    83|	LINEWRAP_DEFAULT,
    84|	LINEWRAP_ALIGNED,
    85|	LINEWRAP_INDENT
    86|};
    87|
    88|enum lineHiliteMode
    89|{
    90|	LINEHILITE_NONE,
    91|	LINEHILITE_HILITE,
    92|	LINEHILITE_FRAME
    93|};
    94|
    95|// ScintillaRef
    96|
    97|// ScintillaEditView
    98|
    99|inline constexpr bool fold_expand = true;
   100|inline constexpr bool fold_collapse = false;
   101|
   102|inline constexpr int MODEVENTMASK_OFF = 0;
   103|
   104|enum TextCase : unsigned char
   105|{
   106|	UPPERCASE,
   107|	LOWERCASE,
   108|	PROPERCASE_FORCE,
   109|	PROPERCASE_BLEND,
   110|	SENTENCECASE_FORCE,
   111|	SENTENCECASE_BLEND,
   112|	INVERTCASE,
   113|	RANDOMCASE
   114|};
   115|
   116|enum class NumBase : unsigned char
   117|{
   118|	BASE_10,
   119|	BASE_16,
   120|	BASE_08,
   121|	BASE_02,
   122|	BASE_16_UPPERCASE
   123|};
   124|
   125|// ScintillaEditView
   126|
   127|// documentMap
   128|
   129|inline constexpr UINT DOCUMENTMAP_MOUSEWHEEL = (WM_USER + 3);
   130|
   131|// documentMap
   132|
   133|// Parameters
   134|
   135|inline constexpr bool POS_VERTICAL = true;
   136|inline constexpr bool POS_HORIZONTAL = false;
   137|
   138|inline constexpr int UDD_SHOW   = 0x01; // 0000 0001
   139|inline constexpr int UDD_DOCKED = 0x02; // 0000 0010
   140|
   141|// 0 : 0000 0000 hide & undocked
   142|// 1 : 0000 0001 show & undocked
   143|// 2 : 0000 0010 hide & docked
   144|// 3 : 0000 0011 show & docked
   145|
   146|inline constexpr int TAB_DRAWTOPBAR            =    0x0001;    // 0000 0000 0000 0001
   147|inline constexpr int TAB_DRAWINACTIVETAB       =    0x0002;    // 0000 0000 0000 0010
   148|inline constexpr int TAB_DRAGNDROP             =    0x0004;    // 0000 0000 0000 0100
   149|inline constexpr int TAB_REDUCE                =    0x0008;    // 0000 0000 0000 1000
   150|inline constexpr int TAB_CLOSEBUTTON           =    0x0010;    // 0000 0000 0001 0000
   151|inline constexpr int TAB_DBCLK2CLOSE           =    0x0020;    // 0000 0000 0010 0000
   152|inline constexpr int TAB_VERTICAL              =    0x0040;    // 0000 0000 0100 0000
   153|inline constexpr int TAB_MULTILINE             =    0x0080;    // 0000 0000 1000 0000
   154|inline constexpr int TAB_HIDE                  =    0x0100;    // 0000 0001 0000 0000
   155|inline constexpr int TAB_QUITONEMPTY           =    0x0200;    // 0000 0010 0000 0000
   156|inline constexpr int TAB_ALTICONS              =    0x0400;    // 0000 0100 0000 0000
   157|inline constexpr int TAB_PINBUTTON             =    0x0800;    // 0000 1000 0000 0000
   158|inline constexpr int TAB_INACTIVETABSHOWBUTTON =    0x1000;    // 0001 0000 0000 0000
   159|inline constexpr int TAB_SHOWONLYPINNEDBUTTON  =    0x2000;    // 0010 0000 0000 0000
   160|
   161|inline constexpr bool activeText = true;
   162|inline constexpr bool activeNumeric = false;
   163|
   164|enum class EolType : std::uint8_t
   165|{
   166|	windows,
   167|	macos,
   168|	unix,
   169|
   170|	// special values
   171|	unknown, // cannot be the first value for legacy code
   172|	osdefault = windows,
   173|};
   174|
   175|enum UniMode
   176|{
   177|	uni8Bit       = 0,  // ANSI
   178|	uniUTF8       = 1,  // UTF-8 with BOM
   179|	uni16BE       = 2,  // UTF-16 Big Endian with BOM
   180|	uni16LE       = 3,  // UTF-16 Little Endian with BOM
   181|	uniUTF8_NoBOM = 4,  // UTF-8 without BOM
   182|	uni7Bit       = 5,  // 0 - 127 ASCII
   183|	uni16BE_NoBOM = 6,  // UTF-16 Big Endian without BOM
   184|	uni16LE_NoBOM = 7,  // UTF-16 Little Endian without BOM
   185|	uniEnd
   186|};
   187|
   188|enum ChangeDetect
   189|{
   190|	cdDisabled      = 0x00,
   191|	cdEnabledOld    = 0x01,
   192|	cdEnabledNew    = 0x02,
   193|	cdAutoUpdate    = 0x04,
   194|	cdGo2end        = 0x08
   195|};
   196|
   197|enum BackupFeature
   198|{
   199|	bak_none,
   200|	bak_simple,
   201|	bak_verbose
   202|};
   203|
   204|enum OpenSaveDirSetting
   205|{
   206|	dir_followCurrent,
   207|	dir_last,
   208|	dir_userDef
   209|};
   210|
   211|enum MultiInstSetting
   212|{
   213|	monoInst,
   214|	multiInstOnSession,
   215|	multiInst
   216|};
   217|
   218|enum writeTechnologyEngine
   219|{
   220|	defaultTechnology,
   221|	directWriteTechnology,
   222|	directWriteRetainTechnology,
   223|	directWriteDcTechnology,
   224|	directWriteDX11Technology,
   225|	directWriteTechnologyUnavailable
   226|};
   227|
   228|enum urlMode
   229|{
   230|	urlDisable,
   231|	urlNoUnderLineFg,
   232|	urlUnderLineFg,
   233|	urlNoUnderLineBg,
   234|	urlUnderLineBg,
   235|
   236|	urlMin = urlDisable,
   237|	urlMax = urlUnderLineBg
   238|};
   239|
   240|enum AutoIndentMode
   241|{
   242|	autoIndent_none,
   243|	autoIndent_advanced,
   244|	autoIndent_basic
   245|};
   246|enum SysTrayAction
   247|{
   248|	sta_none,
   249|	sta_minimize,
   250|	sta_close,
   251|	sta_minimize_close
   252|};
   253|
   254|enum LangIdxStyle
   255|{
   256|	LANG_INDEX_INSTR = 0,
   257|	LANG_INDEX_INSTR2 = 1,
   258|	LANG_INDEX_TYPE = 2,
   259|	LANG_INDEX_TYPE2 = 3,
   260|	LANG_INDEX_TYPE3 = 4,
   261|	LANG_INDEX_TYPE4 = 5,
   262|	LANG_INDEX_TYPE5 = 6,
   263|	LANG_INDEX_TYPE6 = 7,
   264|	LANG_INDEX_TYPE7 = 8,
   265|	LANG_INDEX_SUBSTYLE1 = 9,
   266|	LANG_INDEX_SUBSTYLE2 = 10,
   267|	LANG_INDEX_SUBSTYLE3 = 11,
   268|	LANG_INDEX_SUBSTYLE4 = 12,
   269|	LANG_INDEX_SUBSTYLE5 = 13,
   270|	LANG_INDEX_SUBSTYLE6 = 14,
   271|	LANG_INDEX_SUBSTYLE7 = 15,
   272|	LANG_INDEX_SUBSTYLE8 = 16
   273|};
   274|
   275|enum CopyDataParam
   276|{
   277|	COPYDATA_PARAMS = 0,
   278|	//COPYDATA_FILENAMESA = 1, // obsolete, no more useful
   279|	COPYDATA_FILENAMESW = 2,
   280|	COPYDATA_FULL_CMDLINE = 3
   281|};
   282|
   283|enum UdlDecSep
   284|{
   285|	DECSEP_DOT,
   286|	DECSEP_COMMA,
   287|	DECSEP_BOTH
   288|};
   289|
   290|inline constexpr int NPP_STYLING_FILESIZE_LIMIT_DEFAULT = (200 * 1024 * 1024); // 200MB+ file won't be styled
   291|
   292|inline constexpr int FINDREPLACE_MAXLENGTH = 16384; // the maximum length of the string (decrease 1 for '\0') to search in the editor
   293|
   294|inline constexpr int FINDREPLACE_INSELECTION_THRESHOLD_DEFAULT = 1024;
   295|inline constexpr int FILL_FINDWHAT_THRESHOLD_DEFAULT = 1024;
   296|
   297|inline constexpr const wchar_t fontSizeStrs[][3]{ L"", L"5", L"6", L"7", L"8", L"9", L"10", L"11", L"12", L"14", L"16", L"18", L"20", L"22", L"24", L"26", L"28" };
   298|
   299|enum FontStyle
   300|{
   301|	FONTSTYLE_NONE = 0,
   302|	FONTSTYLE_BOLD = 1,
   303|	FONTSTYLE_ITALIC = 2,
   304|	FONTSTYLE_UNDERLINE = 4
   305|};
   306|
   307|inline constexpr int STYLE_NOT_USED = -1;
   308|
   309|inline constexpr int COLORSTYLE_FOREGROUND = 0x01;
   310|inline constexpr int COLORSTYLE_BACKGROUND = 0x02;
   311|inline constexpr int COLORSTYLE_ALL = COLORSTYLE_FOREGROUND | COLORSTYLE_BACKGROUND;
   312|
   313|inline constexpr COLORREF g_cDefaultMainDark = RGB(0xDE, 0xDE, 0xDE);
   314|inline constexpr COLORREF g_cDefaultSecondaryDark = RGB(0x4C, 0xC2, 0xFF);
   315|inline constexpr COLORREF g_cDefaultMainLight = RGB(0x21, 0x21, 0x21);
   316|inline constexpr COLORREF g_cDefaultSecondaryLight = RGB(0x00, 0x78, 0xD4);
   317|
   318|enum class FluentColor
   319|{
   320|	defaultColor,
   321|	red,
   322|	green,
   323|	blue,
   324|	purple,
   325|	cyan,
   326|	olive,
   327|	yellow,
   328|	accent,
   329|	custom,
   330|	maxValue
   331|};
   332|
   333|inline constexpr int NB_LIST = 20;
   334|inline constexpr int NB_MAX_LRF_FILE = 30;
   335|inline constexpr int NB_MAX_USER_LANG = 30;
   336|inline constexpr int NB_MAX_EXTERNAL_LANG = 30;
   337|inline constexpr int NB_MAX_IMPORTED_UDL = 50;
   338|
   339|inline constexpr int NB_DEFAULT_LRF_CUSTOMLENGTH = 100;
   340|inline constexpr int NB_MAX_LRF_CUSTOMLENGTH = MAX_PATH - 1;
   341|
   342|inline constexpr int NB_MAX_TAB_COMPACT_LABEL_LEN = MAX_PATH - 3; // -3 for the possible ending ellipsis (...)
   343|
   344|inline constexpr int MAX_EXTERNAL_LEXER_NAME_LEN = 128;
   345|
   346|inline constexpr int RECENTFILES_SHOWFULLPATH = -1;
   347|inline constexpr int RECENTFILES_SHOWONLYFILENAME = 0;
   348|
   349|inline constexpr const wchar_t nppLogNetworkDriveIssue[] = L"nppLogNetworkDriveIssue"; // issue xml/log file name
   350|
   351|// Parameters
   352|
   353|// UserDefineLangReference
   354|
   355|inline constexpr int langNameLenMax = 64;
   356|inline constexpr int extsLenMax = 256;
   357|inline constexpr int max_char = 1024 * 30;
   358|
   359|// UserDefineLangReference
   360|
   361|// TabBar
   362|
   363|inline constexpr int g_TabIconSize = 16;
   364|inline constexpr int g_TabHeight = 22;
   365|inline constexpr int g_TabHeightLarge = 25;
   366|inline constexpr int g_TabWidth = 45;
   367|inline constexpr int g_TabWidthButton = 60;
   368|inline constexpr int g_TabCloseBtnSize = 11;
   369|inline constexpr int g_TabPinBtnSize = 11;
   370|inline constexpr int g_TabCloseBtnSize_DM = 16;
   371|inline constexpr int g_TabPinBtnSize_DM = 16;
   372|
   373|// TabBar
   374|
   375|// Style names
   376|
   377|inline constexpr const wchar_t TABBAR_ACTIVEFOCUSEDINDCATOR[] = L"Active tab focused indicator";
   378|inline constexpr const wchar_t TABBAR_ACTIVEUNFOCUSEDINDCATOR[] = L"Active tab unfocused indicator";
   379|inline constexpr const wchar_t TABBAR_ACTIVETEXT[] = L"Active tab text";
   380|inline constexpr const wchar_t TABBAR_INACTIVETEXT[] = L"Inactive tabs";
   381|
   382|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_1[] = L"Tab color 1";
   383|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_2[] = L"Tab color 2";
   384|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_3[] = L"Tab color 3";
   385|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_4[] = L"Tab color 4";
   386|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_5[] = L"Tab color 5";
   387|
   388|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_DM_1[] = L"Tab color dark mode 1";
   389|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_DM_2[] = L"Tab color dark mode 2";
   390|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_DM_3[] = L"Tab color dark mode 3";
   391|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_DM_4[] = L"Tab color dark mode 4";
   392|inline constexpr const wchar_t TABBAR_INDIVIDUALCOLOR_DM_5[] = L"Tab color dark mode 5";
   393|
   394|inline constexpr const wchar_t VIEWZONE_DOCUMENTMAP[] = L"Document map";
   395|
   396|inline constexpr const wchar_t FINDDLG_STAUSNOTFOUND_COLOR[] = L"Find status: Not found";
   397|inline constexpr const wchar_t FINDDLG_STAUSMESSAGE_COLOR[] = L"Find status: Message";
   398|inline constexpr const wchar_t FINDDLG_STAUSREACHED_COLOR[] = L"Find status: Search end reached";
   399|
   400|inline constexpr const wchar_t g_npcStyleName[] = L"Non-printing characters custom color";
   401|
   402|// Style names
   403|