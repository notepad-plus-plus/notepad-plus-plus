     1|     1|     1|// This file is part of npminmin project
     2|     2|     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|     3|     3|
     4|     4|     4|// This program is free software: you can redistribute it and/or modify
     5|     5|     5|// it under the terms of the GNU General Public License as published by
     6|     6|     6|// the Free Software Foundation, either version 3 of the License, or
     7|     7|     7|// at your option any later version.
     8|     8|     8|//
     9|     9|     9|// This program is distributed in the hope that it will be useful,
    10|    10|    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|    11|    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|    12|    12|// GNU General Public License for more details.
    13|    13|    13|//
    14|    14|    14|// You should have received a copy of the GNU General Public License
    15|    15|    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|    16|    16|
    17|    17|    17|
    18|    18|    18|#include "Parameters.h"
    19|    19|    19|
    20|    20|    20|#include <windows.h>
    21|    21|    21|
    22|    22|    22|#include <shlobj.h>
    23|    23|    23|#include <shlwapi.h>
    24|    24|    24|
    25|    25|    25|#include <algorithm>
    26|    26|    26|#include <array>
    27|    27|    27|#include <cassert>
    28|    28|    28|#include <cstdio>
    29|    29|    29|#include <cstdlib>
    30|    30|    30|#include <cstring>
    31|    31|    31|#include <ctime>
    32|    32|    32|#include <cwchar>
    33|    33|    33|#include <exception>
    34|    34|    34|#include <locale>
    35|    35|    35|#include <map>
    36|    36|    36|#include <memory>
    37|    37|    37|#include <sstream>
    38|    38|    38|#include <stdexcept>
    39|    39|    39|#include <string>
    40|    40|    40|#include <utility>
    41|    41|    41|#include <vector>
    42|    42|    42|
    43|    43|    43|#include <SciLexer.h>
    44|    44|    44|#include <Scintilla.h>
    45|    45|    45|
    46|    46|    46|#include "Common.h"
    47|    47|    47|#include "ContextMenu.h"
    48|    48|    48|#include "Notepad_plus_Window.h"
    49|    49|    49|#include "Notepad_plus_msgs.h"
    50|    50|    50|#include "NppConstants.h"
    51|    51|    51|#include "NppDarkMode.h"
    52|    52|    52|#include "NppXml.h"
    53|    53|    53|#include "ScintillaEditView.h"
    54|    54|    54|#include "ToolBar.h"
    55|    55|    55|#include "UserDefineDialog.h"
    56|    56|    56|#include "keys.h"
    57|    57|    57|#include "localization.h"
    58|    58|    58|#include "localizationString.h"
    59|    59|    59|#include "menuCmdID.h"
    60|    60|    60|#include "resource.h"
    61|    61|    61|#include "shortcut.h"
    62|    62|    62|#include "verifySignedfile.h"
    63|    63|    63|
    64|    64|    64|#ifdef _MSC_VER
    65|    65|    65|#pragma warning(disable : 4996) // for GetVersionEx()
    66|    66|    66|#endif
    67|    67|    67|
    68|    68|    68|static constexpr const wchar_t localConfFile[] = L"doLocalConf.xml";
    69|    69|    69|static constexpr const wchar_t notepadStyleFile[] = L"asNotepad.xml";
    70|    70|    70|
    71|    71|    71|static constexpr int NB_MAX_FINDHISTORY_FIND = 30;
    72|    72|    72|static constexpr int NB_MAX_FINDHISTORY_REPLACE = 30;
    73|    73|    73|static constexpr int NB_MAX_FINDHISTORY_PATH = 30;
    74|    74|    74|static constexpr int NB_MAX_FINDHISTORY_FILTER = 20;
    75|    75|    75|
    76|    76|    76|namespace // anonymous namespace
    77|    77|    77|{
    78|    78|    78|
    79|    79|    79|
    80|    80|    80|struct WinMenuKeyDefinition // more or less matches accelerator table definition, easy copy/paste
    81|    81|    81|{
    82|    82|    82|	int vKey = 0;
    83|    83|    83|	int functionId = 0;
    84|    84|    84|	bool isCtrl = false;
    85|    85|    85|	bool isAlt = false;
    86|    86|    86|	bool isShift = false;
    87|    87|    87|	const wchar_t * specialName = nullptr; // Used when no real menu name exists (in case of toggle for example)
    88|    88|    88|};
    89|    89|    89|
    90|    90|    90|
    91|    91|    91|/*!
    92|    92|    92|** \brief array of accelerator keys for all std menu items
    93|    93|    93|**
    94|    94|    94|** values can be 0 for vKey, which means its unused
    95|    95|    95|*/
    96|    96|    96|static constexpr WinMenuKeyDefinition winKeyDefs[]
    97|    97|    97|{
    98|    98|    98|	// V_KEY,    COMMAND_ID,                                    Ctrl,  Alt,   Shift, cmdName
    99|    99|    99|	// -------------------------------------------------------------------------------------
   100|   100|   100|	//
   101|   101|   101|	{ VK_N,       IDM_FILE_NEW,                                 true,  false, false, nullptr },
   102|   102|   102|	{ VK_O,       IDM_FILE_OPEN,                                true,  false, false, nullptr },
   103|   103|   103|	{ VK_NULL,    IDM_FILE_OPEN_FOLDER,                         false, false, false, L"Open containing folder in Explorer" },
   104|   104|   104|	{ VK_NULL,    IDM_FILE_OPEN_CMD,                            false, false, false, L"Open containing folder in Command Prompt" },
   105|   105|   105|	{ VK_NULL,    IDM_FILE_OPEN_DEFAULT_VIEWER,                 false, false, false, nullptr },
   106|   106|   106|	{ VK_NULL,    IDM_FILE_OPENFOLDERASWORKSPACE,                false, false, false, nullptr },
   107|   107|   107|	{ VK_R,       IDM_FILE_RELOAD,                              true,  false, false, nullptr },
   108|   108|   108|	{ VK_S,       IDM_FILE_SAVE,                                true,  false, false, nullptr },
   109|   109|   109|	{ VK_S,       IDM_FILE_SAVEAS,                              true,  true,  false, nullptr },
   110|   110|   110|	{ VK_NULL,    IDM_FILE_SAVECOPYAS,                          false, false, false, nullptr },
   111|   111|   111|	{ VK_S,       IDM_FILE_SAVEALL,                             true,  false, true,  nullptr },
   112|   112|   112|	{ VK_NULL,    IDM_FILE_RENAME,                              false, false, false, nullptr },
   113|   113|   113|	{ VK_W,       IDM_FILE_CLOSE,                               true,  false, false, nullptr },
   114|   114|   114|	{ VK_W,       IDM_FILE_CLOSEALL,                            true,  false, true,  nullptr },
   115|   115|   115|	{ VK_NULL,    IDM_FILE_CLOSEALL_BUT_CURRENT,                false, false, false, nullptr },
   116|   116|   116|	{ VK_NULL,    IDM_FILE_CLOSEALL_BUT_PINNED,                 false, false, false, nullptr },
   117|   117|   117|	{ VK_NULL,    IDM_FILE_CLOSEALL_TOLEFT,                     false, false, false, nullptr },
   118|   118|   118|	{ VK_NULL,    IDM_FILE_CLOSEALL_TORIGHT,                    false, false, false, nullptr },
   119|   119|   119|	{ VK_NULL,    IDM_FILE_CLOSEALL_UNCHANGED,                  false, false, false, nullptr },
   120|   120|   120|	{ VK_NULL,    IDM_FILE_DELETE,                              false, false, false, nullptr },
   121|   121|   121|	{ VK_NULL,    IDM_FILE_LOADSESSION,                         false, false, false, nullptr },
   122|   122|   122|	{ VK_NULL,    IDM_FILE_SAVESESSION,                         false, false, false, nullptr },
   123|   123|   123|	{ VK_P,       IDM_FILE_PRINT,                               true,  false, false, nullptr },
   124|   124|   124|	{ VK_NULL,    IDM_FILE_PRINTNOW,                            false, false, false, nullptr },
   125|   125|   125|	{ VK_T,       IDM_FILE_RESTORELASTCLOSEDFILE,               true,  false, true,  L"Restore Recent Closed File" },
   126|   126|   126|	{ VK_F4,      IDM_FILE_EXIT,                                false, true,  false, nullptr },
   127|   127|   127|
   128|   128|   128|//	{ VK_NULL,    IDM_EDIT_UNDO,                                false, false, false, nullptr },
   129|   129|   129|//	{ VK_NULL,    IDM_EDIT_REDO,                                false, false, false, nullptr },
   130|   130|   130|
   131|   131|   131|	{ VK_DELETE,  IDM_EDIT_CUT,                                 false, false, true,  nullptr },
   132|   132|   132|	{ VK_X,       IDM_EDIT_CUT,                                 true,  false, false, nullptr },
   133|   133|   133|
   134|   134|   134|	{ VK_INSERT,  IDM_EDIT_COPY,                                true,  false, false, nullptr },
   135|   135|   135|	{ VK_C,       IDM_EDIT_COPY,                                true,  false, false, nullptr },
   136|   136|   136|
   137|   137|   137|	{ VK_INSERT,  IDM_EDIT_PASTE,                               false, false, true,  nullptr },
   138|   138|   138|	{ VK_V,       IDM_EDIT_PASTE,                               true,  false, false, nullptr },
   139|   139|   139|
   140|   140|   140|//	{ VK_NULL,    IDM_EDIT_DELETE,                              false, false, false, nullptr },
   141|   141|   141|//	{ VK_NULL,    IDM_EDIT_SELECTALL,                           false, false, false, nullptr },
   142|   142|   142|	{ VK_B,       IDM_EDIT_BEGINENDSELECT,                      true,  false, true,  nullptr },
   143|   143|   143|	{ VK_B,       IDM_EDIT_BEGINENDSELECT_COLUMNMODE,           false, true,  true,  nullptr },
   144|   144|   144|
   145|   145|   145|	{ VK_NULL,    IDM_EDIT_FULLPATHTOCLIP,                      false, false, false, nullptr },
   146|   146|   146|	{ VK_NULL,    IDM_EDIT_FILENAMETOCLIP,                      false, false, false, nullptr },
   147|   147|   147|	{ VK_NULL,    IDM_EDIT_CURRENTDIRTOCLIP,                    false, false, false, nullptr },
   148|   148|   148|	{ VK_NULL,    IDM_EDIT_COPY_ALL_NAMES,                      false, false, false, nullptr },
   149|   149|   149|	{ VK_NULL,    IDM_EDIT_COPY_ALL_PATHS,                      false, false, false, nullptr },
   150|   150|   150|
   151|   151|   151|	{ VK_NULL,    IDM_EDIT_INS_TAB,                             false, false, false, nullptr },
   152|   152|   152|	{ VK_NULL,    IDM_EDIT_RMV_TAB,                             false, false, false, nullptr },
   153|   153|   153|	{ VK_U,       IDM_EDIT_UPPERCASE,                           true,  false, true,  nullptr },
   154|   154|   154|	{ VK_U,       IDM_EDIT_LOWERCASE,                           true,  false, false, nullptr },
   155|   155|   155|	{ VK_U,       IDM_EDIT_PROPERCASE_FORCE,                    false, true,  false, nullptr },
   156|   156|   156|	{ VK_U,       IDM_EDIT_PROPERCASE_BLEND,                    false, true,  true,  nullptr },
   157|   157|   157|	{ VK_U,       IDM_EDIT_SENTENCECASE_FORCE,                  true,  true,  false, nullptr },
   158|   158|   158|	{ VK_U,       IDM_EDIT_SENTENCECASE_BLEND,                  true,  true,  true,  nullptr },
   159|   159|   159|	{ VK_NULL,    IDM_EDIT_INVERTCASE,                          false, false, false, nullptr },
   160|   160|   160|	{ VK_NULL,    IDM_EDIT_RANDOMCASE,                          false, false, false, nullptr },
   161|   161|   161|	{ VK_NULL,    IDM_EDIT_REMOVE_CONSECUTIVE_DUP_LINES,        false, false, false, nullptr },
   162|   162|   162|	{ VK_NULL,    IDM_EDIT_REMOVE_ANY_DUP_LINES,                false, false, false, nullptr },
   163|   163|   163|	{ VK_I,       IDM_EDIT_SPLIT_LINES,                         true,  false, false, nullptr },
   164|   164|   164|	{ VK_J,       IDM_EDIT_JOIN_LINES,                          true,  false, false, nullptr },
   165|   165|   165|	{ VK_UP,      IDM_EDIT_LINE_UP,                             true,  false, true,  nullptr },
   166|   166|   166|	{ VK_DOWN,    IDM_EDIT_LINE_DOWN,                           true,  false, true,  nullptr },
   167|   167|   167|	{ VK_NULL,    IDM_EDIT_REMOVEEMPTYLINES,                    false, false, false, nullptr },
   168|   168|   168|	{ VK_NULL,    IDM_EDIT_REMOVEEMPTYLINESWITHBLANK,           false, false, false, nullptr },
   169|   169|   169|	{ VK_RETURN,  IDM_EDIT_BLANKLINEABOVECURRENT,               true,  true,  false, nullptr },
   170|   170|   170|	{ VK_RETURN,  IDM_EDIT_BLANKLINEBELOWCURRENT,               true,  true,  true,  nullptr },
   171|   171|   171|	{ VK_NULL,    IDM_EDIT_SORTLINES_LENGTH_ASCENDING,          false, false, false, nullptr },
   172|   172|   172|	{ VK_NULL,    IDM_EDIT_SORTLINES_LENGTH_DESCENDING,         false, false, false, nullptr },
   173|   173|   173|	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING,   false, false, false, nullptr },
   174|   174|   174|	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING,  false, false, false, nullptr },
   175|   175|   175|	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_ASCENDING,   false, false, false, nullptr },
   176|   176|   176|	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_DESCENDING,  false, false, false, nullptr },
   177|   177|   177|	{ VK_NULL,    IDM_EDIT_SORTLINES_LOCALE_ASCENDING,          false, false, false, nullptr },
   178|   178|   178|	{ VK_NULL,    IDM_EDIT_SORTLINES_LOCALE_DESCENDING,         false, false, false, nullptr },
   179|   179|   179|	{ VK_NULL,    IDM_EDIT_SORTLINES_INTEGER_ASCENDING,         false, false, false, nullptr },
   180|   180|   180|	{ VK_NULL,    IDM_EDIT_SORTLINES_INTEGER_DESCENDING,        false, false, false, nullptr },
   181|   181|   181|	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING,    false, false, false, nullptr },
   182|   182|   182|	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING,   false, false, false, nullptr },
   183|   183|   183|	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING,      false, false, false, nullptr },
   184|   184|   184|	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING,     false, false, false, nullptr },
   185|   185|   185|	{ VK_NULL,    IDM_EDIT_SORTLINES_REVERSE_ORDER,             false, false, false, nullptr },
   186|   186|   186|	{ VK_NULL,    IDM_EDIT_SORTLINES_RANDOMLY,                  false, false, false, nullptr },
   187|   187|   187|	{ VK_Q,       IDM_EDIT_BLOCK_COMMENT,                       true,  false, false, nullptr },
   188|   188|   188|	{ VK_K,       IDM_EDIT_BLOCK_COMMENT_SET,                   true,  false, false, nullptr },
   189|   189|   189|	{ VK_K,       IDM_EDIT_BLOCK_UNCOMMENT,                     true,  false, true,  nullptr },
   190|   190|   190|	{ VK_Q,       IDM_EDIT_STREAM_COMMENT,                      true,  false, true,  nullptr },
   191|   191|   191|	{ VK_NULL,    IDM_EDIT_STREAM_UNCOMMENT,                    false, false, false, nullptr },
   192|   192|   192|	{ VK_SPACE,   IDM_EDIT_AUTOCOMPLETE,                        true,  false, false, nullptr },
   193|   193|   193|	{ VK_SPACE,   IDM_EDIT_AUTOCOMPLETE_PATH,                   true,  true,  false, nullptr },
   194|   194|   194|	{ VK_RETURN,  IDM_EDIT_AUTOCOMPLETE_CURRENTFILE,            true,  false, false, nullptr },
   195|   195|   195|	{ VK_SPACE,   IDM_EDIT_FUNCCALLTIP,                         true,  false, true,  nullptr },
   196|   196|   196|	{ VK_UP,      IDM_EDIT_FUNCCALLTIP_PREVIOUS,                false, true,  false, nullptr },
   197|   197|   197|	{ VK_DOWN,    IDM_EDIT_FUNCCALLTIP_NEXT,                    false, true,  false, nullptr },
   198|   198|   198|	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_SHORT,               false, false, false, nullptr },
   199|   199|   199|	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_LONG,                false, false, false, nullptr },
   200|   200|   200|	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_CUSTOMIZED,          false, false, false, nullptr },
   201|   201|   201|	{ VK_NULL,    IDM_FORMAT_TODOS,                             false, false, false, L"EOL Conversion to Windows (CR LF)" },
   202|   202|   202|	{ VK_NULL,    IDM_FORMAT_TOUNIX,                            false, false, false, L"EOL Conversion to Unix (LF)" },
   203|   203|   203|	{ VK_NULL,    IDM_FORMAT_TOMAC,                             false, false, false, L"EOL Conversion to Macintosh (CR)" },
   204|   204|   204|	{ VK_NULL,    IDM_EDIT_TRIMTRAILING,                        false, false, false, nullptr },
   205|   205|   205|	{ VK_NULL,    IDM_EDIT_TRIMLINEHEAD,                        false, false, false, nullptr },
   206|   206|   206|	{ VK_NULL,    IDM_EDIT_TRIM_BOTH,                           false, false, false, nullptr },
   207|   207|   207|	{ VK_NULL,    IDM_EDIT_EOL2WS,                              false, false, false, nullptr },
   208|   208|   208|	{ VK_NULL,    IDM_EDIT_TRIMALL,                             false, false, false, nullptr },
   209|   209|   209|	{ VK_NULL,    IDM_EDIT_TAB2SW,                              false, false, false, nullptr },
   210|   210|   210|	{ VK_NULL,    IDM_EDIT_SW2TAB_ALL,                          false, false, false, nullptr },
   211|   211|   211|	{ VK_NULL,    IDM_EDIT_SW2TAB_LEADING,                      false, false, false, nullptr },
   212|   212|   212|	{ VK_NULL,    IDM_EDIT_PASTE_AS_HTML,                       false, false, false, nullptr },
   213|   213|   213|	{ VK_NULL,    IDM_EDIT_PASTE_AS_RTF,                        false, false, false, nullptr },
   214|   214|   214|	{ VK_NULL,    IDM_EDIT_COPY_BINARY,                         false, false, false, nullptr },
   215|   215|   215|	{ VK_NULL,    IDM_EDIT_CUT_BINARY,                          false, false, false, nullptr },
   216|   216|   216|	{ VK_NULL,    IDM_EDIT_PASTE_BINARY,                        false, false, false, nullptr },
   217|   217|   217|	{ VK_NULL,    IDM_EDIT_OPENASFILE,                          false, false, false, nullptr },
   218|   218|   218|	{ VK_NULL,    IDM_EDIT_OPENINFOLDER,                        false, false, false, nullptr },
   219|   219|   219|	{ VK_NULL,    IDM_EDIT_SEARCHONINTERNET,                    false, false, false, nullptr },
   220|   220|   220|	{ VK_NULL,    IDM_EDIT_CHANGESEARCHENGINE,                  false, false, false, nullptr },
   221|   221|   221|	{ VK_NULL,    IDM_EDIT_MULTISELECTALL,                      false, false, false, L"Multi-select All: Ignore Case and Whole Word" },
   222|   222|   222|	{ VK_NULL,    IDM_EDIT_MULTISELECTALLMATCHCASE,             false, false, false, L"Multi-select All: Match Case Only" },
   223|   223|   223|	{ VK_NULL,    IDM_EDIT_MULTISELECTALLWHOLEWORD,             false, false, false, L"Multi-select All: Match Whole Word Only" },
   224|   224|   224|	{ VK_NULL,    IDM_EDIT_MULTISELECTALLMATCHCASEWHOLEWORD,    false, false, false, L"Multi-select All: Match Case and Whole Word" },
   225|   225|   225|	{ VK_NULL,    IDM_EDIT_MULTISELECTNEXT,                     false, false, false, L"Multi-select Next: Ignore Case and Whole Word" },
   226|   226|   226|	{ VK_NULL,    IDM_EDIT_MULTISELECTNEXTMATCHCASE,            false, false, false, L"Multi-select Next: Match Case Only" },
   227|   227|   227|	{ VK_NULL,    IDM_EDIT_MULTISELECTNEXTWHOLEWORD,            false, false, false, L"Multi-select Next: Match Whole Word Only" },
   228|   228|   228|	{ VK_NULL,    IDM_EDIT_MULTISELECTNEXTMATCHCASEWHOLEWORD,   false, false, false, L"Multi-select Next: Match Case and Whole Word" },
   229|   229|   229|	{ VK_NULL,    IDM_EDIT_MULTISELECTUNDO,                     false, false, false, nullptr },
   230|   230|   230|	{ VK_NULL,    IDM_EDIT_MULTISELECTSSKIP,                    false, false, false, nullptr },
   231|   231|   231|//  { VK_NULL,    IDM_EDIT_COLUMNMODETIP,                       false, false, false, nullptr },
   232|   232|   232|	{ VK_C,       IDM_EDIT_COLUMNMODE,                          false, true,  false, nullptr },
   233|   233|   233|	{ VK_NULL,    IDM_EDIT_CHAR_PANEL,                          false, false, false, L"Toggle Character Panel" },
   234|   234|   234|	{ VK_NULL,    IDM_EDIT_CLIPBOARDHISTORY_PANEL,              false, false, false, L"Toggle Clipboard History" },
   235|   235|   235|	{ VK_NULL,    IDM_EDIT_TOGGLEREADONLY,                      false, false, false, nullptr },
   236|   236|   236|	{ VK_NULL,    IDM_EDIT_SETREADONLYFORALLDOCS,               false, false, false, nullptr },
   237|   237|   237|	{ VK_NULL,    IDM_EDIT_CLEARREADONLYFORALLDOCS,             false, false, false, nullptr },
   238|   238|   238|	{ VK_NULL,    IDM_EDIT_TOGGLESYSTEMREADONLY,                false, false, false, nullptr },
   239|   239|   239|	{ VK_F,       IDM_SEARCH_FIND,                              true,  false, false, nullptr },
   240|   240|   240|	{ VK_F,       IDM_SEARCH_FINDINFILES,                       true,  false, true,  nullptr },
   241|   241|   241|	{ VK_F3,      IDM_SEARCH_FINDNEXT,                          false, false, false, nullptr },
   242|   242|   242|	{ VK_F3,      IDM_SEARCH_FINDPREV,                          false, false, true,  nullptr },
   243|   243|   243|	{ VK_F3,      IDM_SEARCH_SETANDFINDNEXT,                    true,  false, false, nullptr },
   244|   244|   244|	{ VK_F3,      IDM_SEARCH_SETANDFINDPREV,                    true,  false, true,  nullptr },
   245|   245|   245|	{ VK_F3,      IDM_SEARCH_VOLATILE_FINDNEXT,                 true,  true,  false, nullptr },
   246|   246|   246|	{ VK_F3,      IDM_SEARCH_VOLATILE_FINDPREV,                 true,  true,  true,  nullptr },
   247|   247|   247|	{ VK_H,       IDM_SEARCH_REPLACE,                           true,  false, false, nullptr },
   248|   248|   248|	{ VK_I,       IDM_SEARCH_FINDINCREMENT,                     true,  true,  false, nullptr },
   249|   249|   249|	{ VK_F7,      IDM_FOCUS_ON_FOUND_RESULTS,                   false, false, false, nullptr },
   250|   250|   250|	{ VK_F4,      IDM_SEARCH_GOTOPREVFOUND,                     false, false, true,  nullptr },
   251|   251|   251|	{ VK_F4,      IDM_SEARCH_GOTONEXTFOUND,                     false, false, false, nullptr },
   252|   252|   252|	{ VK_G,       IDM_SEARCH_GOTOLINE,                          true,  false, false, nullptr },
   253|   253|   253|	{ VK_B,       IDM_SEARCH_GOTOMATCHINGBRACE,                 true,  false, false, nullptr },
   254|   254|   254|	{ VK_B,       IDM_SEARCH_SELECTMATCHINGBRACES,              true,  true,  false, nullptr },
   255|   255|   255|	{ VK_NULL,    IDM_SEARCH_CHANGED_PREV,                      false, false, false, nullptr },
   256|   256|   256|	{ VK_NULL,    IDM_SEARCH_CHANGED_NEXT,                      false, false, false, nullptr },
   257|   257|   257|	{ VK_NULL,    IDM_SEARCH_CLEAR_CHANGE_HISTORY,              false, false, false, nullptr },
   258|   258|   258|	{ VK_M,       IDM_SEARCH_MARK,                              true,  false, false, nullptr },
   259|   259|   259|	{ VK_NULL,    IDM_SEARCH_MARKALLEXT1,                       false, false, false, L"Style all using 1st style" },
   260|   260|   260|	{ VK_NULL,    IDM_SEARCH_MARKALLEXT2,                       false, false, false, L"Style all using 2nd style" },
   261|   261|   261|	{ VK_NULL,    IDM_SEARCH_MARKALLEXT3,                       false, false, false, L"Style all using 3rd style" },
   262|   262|   262|	{ VK_NULL,    IDM_SEARCH_MARKALLEXT4,                       false, false, false, L"Style all using 4th style" },
   263|   263|   263|	{ VK_NULL,    IDM_SEARCH_MARKALLEXT5,                       false, false, false, L"Style all using 5th style" },
   264|   264|   264|	{ VK_NULL,    IDM_SEARCH_MARKONEEXT1,                       false, false, false, L"Style one using 1st style" },
   265|   265|   265|	{ VK_NULL,    IDM_SEARCH_MARKONEEXT2,                       false, false, false, L"Style one using 2nd style" },
   266|   266|   266|	{ VK_NULL,    IDM_SEARCH_MARKONEEXT3,                       false, false, false, L"Style one using 3rd style" },
   267|   267|   267|	{ VK_NULL,    IDM_SEARCH_MARKONEEXT4,                       false, false, false, L"Style one using 4th style" },
   268|   268|   268|	{ VK_NULL,    IDM_SEARCH_MARKONEEXT5,                       false, false, false, L"Style one using 5th style" },
   269|   269|   269|	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT1,                     false, false, false, L"Clear 1st style" },
   270|   270|   270|	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT2,                     false, false, false, L"Clear 2nd style" },
   271|   271|   271|	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT3,                     false, false, false, L"Clear 3rd style" },
   272|   272|   272|	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT4,                     false, false, false, L"Clear 4th style" },
   273|   273|   273|	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT5,                     false, false, false, L"Clear 5th style" },
   274|   274|   274|	{ VK_NULL,    IDM_SEARCH_CLEARALLMARKS,                     false, false, false, L"Clear all styles" },
   275|   275|   275|	{ VK_1,       IDM_SEARCH_GOPREVMARKER1,                     true,  false, true,  L"Previous style of 1st style" },
   276|   276|   276|	{ VK_2,       IDM_SEARCH_GOPREVMARKER2,                     true,  false, true,  L"Previous style of 2nd style" },
   277|   277|   277|	{ VK_3,       IDM_SEARCH_GOPREVMARKER3,                     true,  false, true,  L"Previous style of 3rd style" },
   278|   278|   278|	{ VK_4,       IDM_SEARCH_GOPREVMARKER4,                     true,  false, true,  L"Previous style of 4th style" },
   279|   279|   279|	{ VK_5,       IDM_SEARCH_GOPREVMARKER5,                     true,  false, true,  L"Previous style of 5th style" },
   280|   280|   280|	{ VK_0,       IDM_SEARCH_GOPREVMARKER_DEF,                  true,  false, true,  L"Previous style of Find Mark style" },
   281|   281|   281|	{ VK_1,       IDM_SEARCH_GONEXTMARKER1,                     true,  false, false, L"Next style of 1st style" },
   282|   282|   282|	{ VK_2,       IDM_SEARCH_GONEXTMARKER2,                     true,  false, false, L"Next style of 2nd style" },
   283|   283|   283|	{ VK_3,       IDM_SEARCH_GONEXTMARKER3,                     true,  false, false, L"Next style of 3rd style" },
   284|   284|   284|	{ VK_4,       IDM_SEARCH_GONEXTMARKER4,                     true,  false, false, L"Next style of 4th style" },
   285|   285|   285|	{ VK_5,       IDM_SEARCH_GONEXTMARKER5,                     true,  false, false, L"Next style of 5th style" },
   286|   286|   286|	{ VK_0,       IDM_SEARCH_GONEXTMARKER_DEF,                  true,  false, false, L"Next style of Find Mark style" },
   287|   287|   287|	{ VK_NULL,    IDM_SEARCH_STYLE1TOCLIP,                      false, false, false, L"Copy Styled Text of 1st Style" },
   288|   288|   288|	{ VK_NULL,    IDM_SEARCH_STYLE2TOCLIP,                      false, false, false, L"Copy Styled Text of 2nd Style" },
   289|   289|   289|	{ VK_NULL,    IDM_SEARCH_STYLE3TOCLIP,                      false, false, false, L"Copy Styled Text of 3rd Style" },
   290|   290|   290|	{ VK_NULL,    IDM_SEARCH_STYLE4TOCLIP,                      false, false, false, L"Copy Styled Text of 4th Style" },
   291|   291|   291|	{ VK_NULL,    IDM_SEARCH_STYLE5TOCLIP,                      false, false, false, L"Copy Styled Text of 5th Style" },
   292|   292|   292|	{ VK_NULL,    IDM_SEARCH_ALLSTYLESTOCLIP,                   false, false, false, L"Copy Styled Text of All Styles" },
   293|   293|   293|	{ VK_NULL,    IDM_SEARCH_MARKEDTOCLIP,                      false, false, false, L"Copy Styled Text of Find Mark style" },
   294|   294|   294|	{ VK_F2,      IDM_SEARCH_TOGGLE_BOOKMARK,                   true,  false, false, nullptr },
   295|   295|   295|	{ VK_F2,      IDM_SEARCH_NEXT_BOOKMARK,                     false, false, false, nullptr },
   296|   296|   296|	{ VK_F2,      IDM_SEARCH_PREV_BOOKMARK,                     false, false, true, nullptr  },
   297|   297|   297|	{ VK_NULL,    IDM_SEARCH_CLEAR_BOOKMARKS,                   false, false, false, nullptr },
   298|   298|   298|	{ VK_NULL,    IDM_SEARCH_CUTMARKEDLINES,                    false, false, false, nullptr },
   299|   299|   299|	{ VK_NULL,    IDM_SEARCH_COPYMARKEDLINES,                   false, false, false, nullptr },
   300|   300|   300|	{ VK_NULL,    IDM_SEARCH_PASTEMARKEDLINES,                  false, false, false, nullptr },
   301|   301|   301|	{ VK_NULL,    IDM_SEARCH_DELETEMARKEDLINES,                 false, false, false, nullptr },
   302|   302|   302|	{ VK_NULL,    IDM_SEARCH_DELETEUNMARKEDLINES,               false, false, false, nullptr },
   303|   303|   303|	{ VK_NULL,    IDM_SEARCH_INVERSEMARKS,                      false, false, false, nullptr },
   304|   304|   304|	{ VK_NULL,    IDM_SEARCH_FINDCHARINRANGE,                   false, false, false, nullptr },
   305|   305|   305|
   306|   306|   306|	{ VK_NULL,    IDM_VIEW_ALWAYSONTOP,                         false, false, false, nullptr },
   307|   307|   307|	{ VK_F11,     IDM_VIEW_FULLSCREENTOGGLE,                    false, false, false, nullptr },
   308|   308|   308|	{ VK_F12,     IDM_VIEW_POSTIT,                              false, false, false, nullptr },
   309|   309|   309|	{ VK_NULL,    IDM_VIEW_DISTRACTIONFREE,                     false, false, false, nullptr },
   310|   310|   310|
   311|   311|   311|	{ VK_NULL,    IDM_VIEW_IN_FIREFOX,                          false, false, false, L"View current file in Firefox" },
   312|   312|   312|	{ VK_NULL,    IDM_VIEW_IN_CHROME,                           false, false, false, L"View current file in Chrome" },
   313|   313|   313|	{ VK_NULL,    IDM_VIEW_IN_IE,                               false, false, false, L"View current file in IE" },
   314|   314|   314|	{ VK_NULL,    IDM_VIEW_IN_EDGE,                             false, false, false, L"View current file in Edge" },
   315|   315|   315|
   316|   316|   316|	{ VK_NULL,    IDM_VIEW_TAB_SPACE,                           false, false, false, nullptr },
   317|   317|   317|	{ VK_NULL,    IDM_VIEW_EOL,                                 false, false, false, nullptr },
   318|   318|   318|	{ VK_NULL,    IDM_VIEW_ALL_CHARACTERS,                      false, false, false, nullptr },
   319|   319|   319|	{ VK_NULL,    IDM_VIEW_NPC,                                 false, false, false, nullptr },
   320|   320|   320|	{ VK_NULL,    IDM_VIEW_NPC_CCUNIEOL,                        false, false, false, nullptr },
   321|   321|   321|	{ VK_NULL,    IDM_VIEW_INDENT_GUIDE,                        false, false, false, nullptr },
   322|   322|   322|	{ VK_NULL,    IDM_VIEW_WRAP_SYMBOL,                         false, false, false, nullptr },
   323|   323|   323|//  { VK_NULL,    IDM_VIEW_ZOOMIN,                              false, false, false, nullptr },
   324|   324|   324|//  { VK_NULL,    IDM_VIEW_ZOOMOUT,                             false, false, false, nullptr },
   325|   325|   325|//  { VK_NULL,    IDM_VIEW_ZOOMRESTORE,                         false, false, false, nullptr },
   326|   326|   326|	{ VK_NULL,    IDM_VIEW_GOTO_START,                          false, false, false, nullptr },
   327|   327|   327|	{ VK_NULL,    IDM_VIEW_GOTO_END,                            false, false, false, nullptr },
   328|   328|   328|	{ VK_NULL,    IDM_VIEW_GOTO_ANOTHER_VIEW,                   false, false, false, nullptr },
   329|   329|   329|	{ VK_NULL,    IDM_VIEW_CLONE_TO_ANOTHER_VIEW,               false, false, false, nullptr },
   330|   330|   330|	{ VK_NULL,    IDM_VIEW_GOTO_NEW_INSTANCE,                   false, false, false, nullptr },
   331|   331|   331|	{ VK_NULL,    IDM_VIEW_LOAD_IN_NEW_INSTANCE,                false, false, false, nullptr },
   332|   332|   332|
   333|   333|   333|	{ VK_NUMPAD1, IDM_VIEW_TAB1,                                true,  false, false, nullptr },
   334|   334|   334|	{ VK_NUMPAD2, IDM_VIEW_TAB2,                                true,  false, false, nullptr },
   335|   335|   335|	{ VK_NUMPAD3, IDM_VIEW_TAB3,                                true,  false, false, nullptr },
   336|   336|   336|	{ VK_NUMPAD4, IDM_VIEW_TAB4,                                true,  false, false, nullptr },
   337|   337|   337|	{ VK_NUMPAD5, IDM_VIEW_TAB5,                                true,  false, false, nullptr },
   338|   338|   338|	{ VK_NUMPAD6, IDM_VIEW_TAB6,                                true,  false, false, nullptr },
   339|   339|   339|	{ VK_NUMPAD7, IDM_VIEW_TAB7,                                true,  false, false, nullptr },
   340|   340|   340|	{ VK_NUMPAD8, IDM_VIEW_TAB8,                                true,  false, false, nullptr },
   341|   341|   341|	{ VK_NUMPAD9, IDM_VIEW_TAB9,                                true,  false, false, nullptr },
   342|   342|   342|	{ VK_NULL,    IDM_VIEW_TAB_START,                           false, false, false, nullptr },
   343|   343|   343|	{ VK_NULL,    IDM_VIEW_TAB_END,                             false, false, false, nullptr },
   344|   344|   344|	{ VK_NEXT,    IDM_VIEW_TAB_NEXT,                            true,  false, false, nullptr },
   345|   345|   345|	{ VK_PRIOR,   IDM_VIEW_TAB_PREV,                            true,  false, false, nullptr },
   346|   346|   346|	{ VK_NEXT,    IDM_VIEW_TAB_MOVEFORWARD,                     true,  false, true,  nullptr },
   347|   347|   347|	{ VK_PRIOR,   IDM_VIEW_TAB_MOVEBACKWARD,                    true,  false, true,  nullptr },
   348|   348|   348|	{ VK_TAB,     IDC_PREV_DOC,                                 true,  false, true,  L"Switch to previous document" },
   349|   349|   349|	{ VK_TAB,     IDC_NEXT_DOC,                                 true,  false, false, L"Switch to next document" },
   350|   350|   350|	{ VK_NULL,    IDM_VIEW_WRAP,                                false, false, false, nullptr },
   351|   351|   351|	{ VK_H,       IDM_VIEW_HIDELINES,                           false, true,  false, nullptr },
   352|   352|   352|	{ VK_F8,      IDM_VIEW_SWITCHTO_OTHER_VIEW,                 false, false, false, nullptr },
   353|   353|   353|
   354|   354|   354|	{ VK_0,       IDM_VIEW_FOLDALL,                             false, true,  false, nullptr },
   355|   355|   355|	{ VK_0,       IDM_VIEW_UNFOLDALL,                           false, true,  true,  nullptr },
   356|   356|   356|	{ VK_F,       IDM_VIEW_FOLD_CURRENT,                        true,  true,  false, nullptr },
   357|   357|   357|	{ VK_F,       IDM_VIEW_UNFOLD_CURRENT,                      true,  true,  true,  nullptr },
   358|   358|   358|	{ VK_1,       IDM_VIEW_FOLD_1,                              false, true,  false, L"Fold Level 1" },
   359|   359|   359|	{ VK_2,       IDM_VIEW_FOLD_2,                              false, true,  false, L"Fold Level 2" },
   360|   360|   360|	{ VK_3,       IDM_VIEW_FOLD_3,                              false, true,  false, L"Fold Level 3" },
   361|   361|   361|	{ VK_4,       IDM_VIEW_FOLD_4,                              false, true,  false, L"Fold Level 4" },
   362|   362|   362|	{ VK_5,       IDM_VIEW_FOLD_5,                              false, true,  false, L"Fold Level 5" },
   363|   363|   363|	{ VK_6,       IDM_VIEW_FOLD_6,                              false, true,  false, L"Fold Level 6" },
   364|   364|   364|	{ VK_7,       IDM_VIEW_FOLD_7,                              false, true,  false, L"Fold Level 7" },
   365|   365|   365|	{ VK_8,       IDM_VIEW_FOLD_8,                              false, true,  false, L"Fold Level 8" },
   366|   366|   366|
   367|   367|   367|	{ VK_1,       IDM_VIEW_UNFOLD_1,                            false, true,  true,  L"Unfold Level 1" },
   368|   368|   368|	{ VK_2,       IDM_VIEW_UNFOLD_2,                            false, true,  true,  L"Unfold Level 2" },
   369|   369|   369|	{ VK_3,       IDM_VIEW_UNFOLD_3,                            false, true,  true,  L"Unfold Level 3" },
   370|   370|   370|	{ VK_4,       IDM_VIEW_UNFOLD_4,                            false, true,  true,  L"Unfold Level 4" },
   371|   371|   371|	{ VK_5,       IDM_VIEW_UNFOLD_5,                            false, true,  true,  L"Unfold Level 5" },
   372|   372|   372|	{ VK_6,       IDM_VIEW_UNFOLD_6,                            false, true,  true,  L"Unfold Level 6" },
   373|   373|   373|	{ VK_7,       IDM_VIEW_UNFOLD_7,                            false, true,  true,  L"Unfold Level 7" },
   374|   374|   374|	{ VK_8,       IDM_VIEW_UNFOLD_8,                            false, true,  true,  L"Unfold Level 8" },
   375|   375|   375|	{ VK_NULL,    IDM_VIEW_SUMMARY,                             false, false, false, nullptr },
   376|   376|   376|	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_1,                     false, false, false, L"Toggle Project Panel 1" },
   377|   377|   377|	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_2,                     false, false, false, L"Toggle Project Panel 2" },
   378|   378|   378|	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_3,                     false, false, false, L"Toggle Project Panel 3" },
   379|   379|   379|	{ VK_NULL,    IDM_VIEW_FILEBROWSER,                         false, false, false, L"Toggle Folder as Workspace" },
   380|   380|   380|	{ VK_NULL,    IDM_VIEW_DOC_MAP,                             false, false, false, L"Toggle Document Map" },
   381|   381|   381|	{ VK_NULL,    IDM_VIEW_DOCLIST,                             false, false, false, L"Toggle Document List" },
   382|   382|   382|	{ VK_NULL,    IDM_VIEW_FUNC_LIST,                           false, false, false, L"Toggle Function List" },
   383|   383|   383|	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_1,            false, false, false, L"Switch to Project Panel 1" },
   384|   384|   384|	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_2,            false, false, false, L"Switch to Project Panel 2" },
   385|   385|   385|	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_3,            false, false, false, L"Switch to Project Panel 3" },
   386|   386|   386|	{ VK_NULL,    IDM_VIEW_SWITCHTO_FILEBROWSER,                false, false, false, L"Switch to Folder as Workspace" },
   387|   387|   387|	{ VK_NULL,    IDM_VIEW_SWITCHTO_FUNC_LIST,                  false, false, false, L"Switch to Function List" },
   388|   388|   388|	{ VK_NULL,    IDM_VIEW_SWITCHTO_DOCLIST,                    false, false, false, L"Switch to Document List" },
   389|   389|   389|	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_NONE,                     false, false, false, L"Remove Tab Colour" },
   390|   390|   390|	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_1,                        false, false, false, L"Apply Tab Colour 1" },
   391|   391|   391|	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_2,                        false, false, false, L"Apply Tab Colour 2" },
   392|   392|   392|	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_3,                        false, false, false, L"Apply Tab Colour 3" },
   393|   393|   393|	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_4,                        false, false, false, L"Apply Tab Colour 4" },
   394|   394|   394|	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_5,                        false, false, false, L"Apply Tab Colour 5" },
   395|   395|   395|	{ VK_NULL,    IDM_VIEW_SYNSCROLLV,                          false, false, false, nullptr },
   396|   396|   396|	{ VK_NULL,    IDM_VIEW_SYNSCROLLH,                          false, false, false, nullptr },
   397|   397|   397|	{ VK_R,       IDM_EDIT_RTL,                                 true,  true,  false, nullptr },
   398|   398|   398|	{ VK_L,       IDM_EDIT_LTR,                                 true,  true,  false, nullptr },
   399|   399|   399|	{ VK_NULL,    IDM_VIEW_MONITORING,                          false, false, false, nullptr },
   400|   400|   400|
   401|   401|   401|	{ VK_NULL,    IDM_FORMAT_ANSI,                              false, false, false, nullptr },
   402|   402|   402|	{ VK_NULL,    IDM_FORMAT_AS_UTF_8,                          false, false, false, nullptr },
   403|   403|   403|	{ VK_NULL,    IDM_FORMAT_UTF_8,                             false, false, false, nullptr },
   404|   404|   404|	{ VK_NULL,    IDM_FORMAT_UTF_16BE,                          false, false, false, nullptr },
   405|   405|   405|	{ VK_NULL,    IDM_FORMAT_UTF_16LE,                          false, false, false, nullptr },
   406|   406|   406|
   407|   407|   407|	{ VK_NULL,    IDM_FORMAT_ISO_8859_6,                        false, false, false, nullptr },
   408|   408|   408|	{ VK_NULL,    IDM_FORMAT_WIN_1256,                          false, false, false, nullptr },
   409|   409|   409|	{ VK_NULL,    IDM_FORMAT_ISO_8859_13,                       false, false, false, nullptr },
   410|   410|   410|	{ VK_NULL,    IDM_FORMAT_WIN_1257,                          false, false, false, nullptr },
   411|   411|   411|	{ VK_NULL,    IDM_FORMAT_ISO_8859_14,                       false, false, false, nullptr },
   412|   412|   412|	{ VK_NULL,    IDM_FORMAT_ISO_8859_5,                        false, false, false, nullptr },
   413|   413|   413|	{ VK_NULL,    IDM_FORMAT_MAC_CYRILLIC,                      false, false, false, nullptr },
   414|   414|   414|	{ VK_NULL,    IDM_FORMAT_KOI8R_CYRILLIC,                    false, false, false, nullptr },
   415|   415|   415|	{ VK_NULL,    IDM_FORMAT_KOI8U_CYRILLIC,                    false, false, false, nullptr },
   416|   416|   416|	{ VK_NULL,    IDM_FORMAT_WIN_1251,                          false, false, false, nullptr },
   417|   417|   417|	{ VK_NULL,    IDM_FORMAT_WIN_1250,                          false, false, false, nullptr },
   418|   418|   418|	{ VK_NULL,    IDM_FORMAT_DOS_437,                           false, false, false, nullptr },
   419|   419|   419|	{ VK_NULL,    IDM_FORMAT_DOS_720,                           false, false, false, nullptr },
   420|   420|   420|	{ VK_NULL,    IDM_FORMAT_DOS_737,                           false, false, false, nullptr },
   421|   421|   421|	{ VK_NULL,    IDM_FORMAT_DOS_775,                           false, false, false, nullptr },
   422|   422|   422|	{ VK_NULL,    IDM_FORMAT_DOS_850,                           false, false, false, nullptr },
   423|   423|   423|	{ VK_NULL,    IDM_FORMAT_DOS_852,                           false, false, false, nullptr },
   424|   424|   424|	{ VK_NULL,    IDM_FORMAT_DOS_855,                           false, false, false, nullptr },
   425|   425|   425|	{ VK_NULL,    IDM_FORMAT_DOS_857,                           false, false, false, nullptr },
   426|   426|   426|	{ VK_NULL,    IDM_FORMAT_DOS_858,                           false, false, false, nullptr },
   427|   427|   427|	{ VK_NULL,    IDM_FORMAT_DOS_860,                           false, false, false, nullptr },
   428|   428|   428|	{ VK_NULL,    IDM_FORMAT_DOS_861,                           false, false, false, nullptr },
   429|   429|   429|	{ VK_NULL,    IDM_FORMAT_DOS_862,                           false, false, false, nullptr },
   430|   430|   430|	{ VK_NULL,    IDM_FORMAT_DOS_863,                           false, false, false, nullptr },
   431|   431|   431|	{ VK_NULL,    IDM_FORMAT_DOS_865,                           false, false, false, nullptr },
   432|   432|   432|	{ VK_NULL,    IDM_FORMAT_DOS_866,                           false, false, false, nullptr },
   433|   433|   433|	{ VK_NULL,    IDM_FORMAT_DOS_869,                           false, false, false, nullptr },
   434|   434|   434|	{ VK_NULL,    IDM_FORMAT_BIG5,                              false, false, false, nullptr },
   435|   435|   435|	{ VK_NULL,    IDM_FORMAT_GB2312,                            false, false, false, nullptr },
   436|   436|   436|	{ VK_NULL,    IDM_FORMAT_ISO_8859_2,                        false, false, false, nullptr },
   437|   437|   437|	{ VK_NULL,    IDM_FORMAT_ISO_8859_7,                        false, false, false, nullptr },
   438|   438|   438|	{ VK_NULL,    IDM_FORMAT_WIN_1253,                          false, false, false, nullptr },
   439|   439|   439|	{ VK_NULL,    IDM_FORMAT_ISO_8859_8,                        false, false, false, nullptr },
   440|   440|   440|	{ VK_NULL,    IDM_FORMAT_WIN_1255,                          false, false, false, nullptr },
   441|   441|   441|	{ VK_NULL,    IDM_FORMAT_SHIFT_JIS,                         false, false, false, nullptr },
   442|   442|   442|	{ VK_NULL,    IDM_FORMAT_EUC_KR,                            false, false, false, nullptr },
   443|   443|   443|	//{ VK_NULL,    IDM_FORMAT_ISO_8859_10,                       false, false, false, nullptr },
   444|   444|   444|	{ VK_NULL,    IDM_FORMAT_ISO_8859_15,                       false, false, false, nullptr },
   445|   445|   445|	{ VK_NULL,    IDM_FORMAT_ISO_8859_4,                        false, false, false, nullptr },
   446|   446|   446|	//{ VK_NULL,    IDM_FORMAT_ISO_8859_16,                       false, false, false, nullptr },
   447|   447|   447|	{ VK_NULL,    IDM_FORMAT_ISO_8859_3,                        false, false, false, nullptr },
   448|   448|   448|	//{ VK_NULL,    IDM_FORMAT_ISO_8859_11,                       false, false, false, nullptr },
   449|   449|   449|	{ VK_NULL,    IDM_FORMAT_TIS_620,                           false, false, false, nullptr },
   450|   450|   450|	{ VK_NULL,    IDM_FORMAT_ISO_8859_9,                        false, false, false, nullptr },
   451|   451|   451|	{ VK_NULL,    IDM_FORMAT_WIN_1254,                          false, false, false, nullptr },
   452|   452|   452|	{ VK_NULL,    IDM_FORMAT_WIN_1252,                          false, false, false, nullptr },
   453|   453|   453|	{ VK_NULL,    IDM_FORMAT_ISO_8859_1,                        false, false, false, nullptr },
   454|   454|   454|	{ VK_NULL,    IDM_FORMAT_WIN_1258,                          false, false, false, nullptr },
   455|   455|   455|	{ VK_NULL,    IDM_FORMAT_CONV2_ANSI,                        false, false, false, nullptr },
   456|   456|   456|	{ VK_NULL,    IDM_FORMAT_CONV2_AS_UTF_8,                    false, false, false, nullptr },
   457|   457|   457|	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_8,                       false, false, false, nullptr },
   458|   458|   458|	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_16BE,                    false, false, false, nullptr },
   459|   459|   459|	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_16LE,                    false, false, false, nullptr },
   460|   460|   460|
   461|   461|   461|	{ VK_NULL,    IDM_LANG_USER_DLG,                            false, false, false, nullptr },
   462|   462|   462|	{ VK_NULL,    IDM_LANG_USER,                                false, false, false, nullptr },
   463|   463|   463|	{ VK_NULL,    IDM_LANG_OPENUDLDIR,                          false, false, false, nullptr },
   464|   464|   464|
   465|   465|   465|	{ VK_NULL,    IDM_SETTING_PREFERENCE,                       false, false, false, nullptr },
   466|   466|   466|	{ VK_NULL,    IDM_LANGSTYLE_CONFIG_DLG,                     false, false, false, nullptr },
   467|   467|   467|	{ VK_NULL,    IDM_SETTING_SHORTCUT_MAPPER,                  false, false, false, nullptr },
   468|   468|   468|	{ VK_NULL,    IDM_SETTING_IMPORTPLUGIN,                     false, false, false, nullptr },
   469|   469|   469|	{ VK_NULL,    IDM_SETTING_IMPORTSTYLETHEMES,                false, false, false, nullptr },
   470|   470|   470|	{ VK_NULL,    IDM_SETTING_EDITCONTEXTMENU,                  false, false, false, nullptr },
   471|   471|   471|
   472|   472|   472|	{ VK_R,       IDC_EDIT_TOGGLEMACRORECORDING,                true,  false, true,  L"Toggle macro recording" },
   473|   473|   473|	{ VK_NULL,    IDM_MACRO_STARTRECORDINGMACRO,                false, false, false, nullptr },
   474|   474|   474|	{ VK_NULL,    IDM_MACRO_STOPRECORDINGMACRO,                 false, false, false, nullptr },
   475|   475|   475|	{ VK_P,       IDM_MACRO_PLAYBACKRECORDEDMACRO,              true,  false, true,  nullptr },
   476|   476|   476|	{ VK_NULL,    IDM_MACRO_SAVECURRENTMACRO,                   false, false, false, nullptr },
   477|   477|   477|	{ VK_NULL,    IDM_MACRO_RUNMULTIMACRODLG,                   false, false, false, nullptr },
   478|   478|   478|
   479|   479|   479|	{ VK_F5,      IDM_EXECUTE,                                  false, false, false, nullptr },
   480|   480|   480|
   481|   481|   481|	{ VK_NULL,    IDM_WINDOW_WINDOWS,                           false, false, false, nullptr },
   482|   482|   482|	{ VK_NULL,    IDM_WINDOW_SORT_FN_ASC,                       false, false, false, L"Sort by Name A to Z" },
   483|   483|   483|	{ VK_NULL,    IDM_WINDOW_SORT_FN_DSC,                       false, false, false, L"Sort by Name Z to A" },
   484|   484|   484|	{ VK_NULL,    IDM_WINDOW_SORT_FP_ASC,                       false, false, false, L"Sort by Path A to Z" },
   485|   485|   485|	{ VK_NULL,    IDM_WINDOW_SORT_FP_DSC,                       false, false, false, L"Sort by Path Z to A" },
   486|   486|   486|	{ VK_NULL,    IDM_WINDOW_SORT_FT_ASC,                       false, false, false, L"Sort by Type A to Z" },
   487|   487|   487|	{ VK_NULL,    IDM_WINDOW_SORT_FT_DSC,                       false, false, false, L"Sort by Type Z to A" },
   488|   488|   488|	{ VK_NULL,    IDM_WINDOW_SORT_FS_ASC,                       false, false, false, L"Sort by Content Length Ascending" },
   489|   489|   489|	{ VK_NULL,    IDM_WINDOW_SORT_FS_DSC,                       false, false, false, L"Sort by Content Length Descending" },
   490|   490|   490|
   491|   491|   491|	{ VK_NULL,    IDM_CMDLINEARGUMENTS,                         false, false, false, nullptr },
   492|   492|   492|	{ VK_NULL,    IDM_HOMESWEETHOME,                            false, false, false, nullptr },
   493|   493|   493|	{ VK_NULL,    IDM_PROJECTPAGE,                              false, false, false, nullptr },
   494|   494|   494|	{ VK_NULL,    IDM_ONLINEDOCUMENT,                           false, false, false, nullptr },
   495|   495|   495|	{ VK_NULL,    IDM_FORUM,                                    false, false, false, nullptr },
   496|   496|   496|//	{ VK_NULL,    IDM_ONLINESUPPORT,                            false, false, false, nullptr },
   497|   497|   497|//	{ VK_NULL,    IDM_PLUGINSHOME,                              false, false, false, nullptr },
   498|   498|   498|
   499|   499|   499|	// The following two commands are not in menu if (nppGUI._doesExistUpdater == 0).
   500|   500|   500|	// They cannot be derived from menu then, only for this reason the text is specified here.
   501|