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

#include <time.h>

#include <shlobj.h>
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "keys.h"
#include "localization.h"
#include "localizationString.h"
#include "UserDefineDialog.h"
#include "WindowsDlgRc.h"
#include "Notepad_plus_Window.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996) // for GetVersionEx()
#endif

using namespace std;

namespace // anonymous namespace
{


struct WinMenuKeyDefinition // more or less matches accelerator table definition, easy copy/paste
{
	int vKey = 0;
	int functionId = 0;
	bool isCtrl = false;
	bool isAlt = false;
	bool isShift = false;
	const wchar_t * specialName = nullptr; // Used when no real menu name exists (in case of toggle for example)
};


/*!
** \brief array of accelerator keys for all std menu items
**
** values can be 0 for vKey, which means its unused
*/
static const WinMenuKeyDefinition winKeyDefs[] =
{
	// V_KEY,    COMMAND_ID,                                    Ctrl,  Alt,   Shift, cmdName
	// -------------------------------------------------------------------------------------
	//
	{ VK_N,       IDM_FILE_NEW,                                 true,  false, false, nullptr },
	{ VK_O,       IDM_FILE_OPEN,                                true,  false, false, nullptr },
	{ VK_NULL,    IDM_FILE_OPEN_FOLDER,                         false, false, false, L"Open containing folder in Explorer" },
	{ VK_NULL,    IDM_FILE_OPEN_CMD,                            false, false, false, L"Open containing folder in Command Prompt" },
	{ VK_NULL,    IDM_FILE_OPEN_DEFAULT_VIEWER,                 false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_OPENFOLDERASWORSPACE,                false, false, false, nullptr },
	{ VK_R,       IDM_FILE_RELOAD,                              true,  false, false, nullptr },
	{ VK_S,       IDM_FILE_SAVE,                                true,  false, false, nullptr },
	{ VK_S,       IDM_FILE_SAVEAS,                              true,  true,  false, nullptr },
	{ VK_NULL,    IDM_FILE_SAVECOPYAS,                          false, false, false, nullptr },
	{ VK_S,       IDM_FILE_SAVEALL,                             true,  false, true,  nullptr },
	{ VK_NULL,    IDM_FILE_RENAME,                              false, false, false, nullptr },
	{ VK_W,       IDM_FILE_CLOSE,                               true,  false, false, nullptr },
	{ VK_W,       IDM_FILE_CLOSEALL,                            true,  false, true,  nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_BUT_CURRENT,                false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_BUT_PINNED,                 false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_TOLEFT,                     false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_TORIGHT,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_UNCHANGED,                  false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_DELETE,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_LOADSESSION,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_SAVESESSION,                         false, false, false, nullptr },
	{ VK_P,       IDM_FILE_PRINT,                               true,  false, false, nullptr },
	{ VK_NULL,    IDM_FILE_PRINTNOW,                            false, false, false, nullptr },
	{ VK_T,       IDM_FILE_RESTORELASTCLOSEDFILE,               true,  false, true,  L"Restore Recent Closed File" },
	{ VK_F4,      IDM_FILE_EXIT,                                false, true,  false, nullptr },

//	{ VK_NULL,    IDM_EDIT_UNDO,                                false, false, false, nullptr },
//	{ VK_NULL,    IDM_EDIT_REDO,                                false, false, false, nullptr },

	{ VK_DELETE,  IDM_EDIT_CUT,                                 false, false, true,  nullptr },
	{ VK_X,       IDM_EDIT_CUT,                                 true,  false, false, nullptr },

	{ VK_INSERT,  IDM_EDIT_COPY,                                true,  false, false, nullptr },
	{ VK_C,       IDM_EDIT_COPY,                                true,  false, false, nullptr },

	{ VK_INSERT,  IDM_EDIT_PASTE,                               false, false, true,  nullptr },
	{ VK_V,       IDM_EDIT_PASTE,                               true,  false, false, nullptr },

//	{ VK_NULL,    IDM_EDIT_DELETE,                              false, false, false, nullptr },
//	{ VK_NULL,    IDM_EDIT_SELECTALL,                           false, false, false, nullptr },
	{ VK_B,       IDM_EDIT_BEGINENDSELECT,                      true,  false, true,  nullptr },
	{ VK_B,       IDM_EDIT_BEGINENDSELECT_COLUMNMODE,           false, true,  true,  nullptr },

	{ VK_NULL,    IDM_EDIT_FULLPATHTOCLIP,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_FILENAMETOCLIP,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_CURRENTDIRTOCLIP,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_COPY_ALL_NAMES,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_COPY_ALL_PATHS,                      false, false, false, nullptr },

	{ VK_NULL,    IDM_EDIT_INS_TAB,                             false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_RMV_TAB,                             false, false, false, nullptr },
	{ VK_U,       IDM_EDIT_UPPERCASE,                           true,  false, true,  nullptr },
	{ VK_U,       IDM_EDIT_LOWERCASE,                           true,  false, false, nullptr },
	{ VK_U,       IDM_EDIT_PROPERCASE_FORCE,                    false, true,  false, nullptr },
	{ VK_U,       IDM_EDIT_PROPERCASE_BLEND,                    false, true,  true,  nullptr },
	{ VK_U,       IDM_EDIT_SENTENCECASE_FORCE,                  true,  true,  false, nullptr },
	{ VK_U,       IDM_EDIT_SENTENCECASE_BLEND,                  true,  true,  true,  nullptr },
	{ VK_NULL,    IDM_EDIT_INVERTCASE,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_RANDOMCASE,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_REMOVE_CONSECUTIVE_DUP_LINES,        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_REMOVE_ANY_DUP_LINES,                false, false, false, nullptr },
	{ VK_I,       IDM_EDIT_SPLIT_LINES,                         true,  false, false, nullptr },
	{ VK_J,       IDM_EDIT_JOIN_LINES,                          true,  false, false, nullptr },
	{ VK_UP,      IDM_EDIT_LINE_UP,                             true,  false, true,  nullptr },
	{ VK_DOWN,    IDM_EDIT_LINE_DOWN,                           true,  false, true,  nullptr },
	{ VK_NULL,    IDM_EDIT_REMOVEEMPTYLINES,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_REMOVEEMPTYLINESWITHBLANK,           false, false, false, nullptr },
	{ VK_RETURN,  IDM_EDIT_BLANKLINEABOVECURRENT,               true,  true,  false, nullptr },
	{ VK_RETURN,  IDM_EDIT_BLANKLINEBELOWCURRENT,               true,  true,  true,  nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING,   false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING,  false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_ASCENDING,   false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_DESCENDING,  false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_INTEGER_ASCENDING,         false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_INTEGER_DESCENDING,        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING,    false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING,   false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING,      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING,     false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_REVERSE_ORDER,             false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_RANDOMLY,                  false, false, false, nullptr },
	{ VK_Q,       IDM_EDIT_BLOCK_COMMENT,                       true,  false, false, nullptr },
	{ VK_K,       IDM_EDIT_BLOCK_COMMENT_SET,                   true,  false, false, nullptr },
	{ VK_K,       IDM_EDIT_BLOCK_UNCOMMENT,                     true,  false, true,  nullptr },
	{ VK_Q,       IDM_EDIT_STREAM_COMMENT,                      true,  false, true,  nullptr },
	{ VK_NULL,    IDM_EDIT_STREAM_UNCOMMENT,                    false, false, false, nullptr },
	{ VK_SPACE,   IDM_EDIT_AUTOCOMPLETE,                        true,  false, false, nullptr },
	{ VK_SPACE,   IDM_EDIT_AUTOCOMPLETE_PATH,                   true,  true,  false, nullptr },
	{ VK_RETURN,  IDM_EDIT_AUTOCOMPLETE_CURRENTFILE,            true,  false, false, nullptr },
	{ VK_SPACE,   IDM_EDIT_FUNCCALLTIP,                         true,  false, true,  nullptr },
	{ VK_UP,      IDM_EDIT_FUNCCALLTIP_PREVIOUS,                false, true,  false, nullptr },
	{ VK_DOWN,    IDM_EDIT_FUNCCALLTIP_NEXT,                    false, true,  false, nullptr },
	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_SHORT,               false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_LONG,                false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_CUSTOMIZED,          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_TODOS,                             false, false, false, L"EOL Conversion to Windows (CR LF)" },
	{ VK_NULL,    IDM_FORMAT_TOUNIX,                            false, false, false, L"EOL Conversion to Unix (LF)" },
	{ VK_NULL,    IDM_FORMAT_TOMAC,                             false, false, false, L"EOL Conversion to Macintosh (CR)" },
	{ VK_NULL,    IDM_EDIT_TRIMTRAILING,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_TRIMLINEHEAD,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_TRIM_BOTH,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_EOL2WS,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_TRIMALL,                             false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_TAB2SW,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SW2TAB_ALL,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SW2TAB_LEADING,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_PASTE_AS_HTML,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_PASTE_AS_RTF,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_COPY_BINARY,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_CUT_BINARY,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_PASTE_BINARY,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_OPENASFILE,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_OPENINFOLDER,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SEARCHONINTERNET,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_CHANGESEARCHENGINE,                  false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_MULTISELECTALL,                      false, false, false, L"Multi-select All: Ignore Case and Whole Word" },
	{ VK_NULL,    IDM_EDIT_MULTISELECTALLMATCHCASE,             false, false, false, L"Multi-select All: Match Case Only" },
	{ VK_NULL,    IDM_EDIT_MULTISELECTALLWHOLEWORD,             false, false, false, L"Multi-select All: Match Whole Word Only" },
	{ VK_NULL,    IDM_EDIT_MULTISELECTALLMATCHCASEWHOLEWORD,    false, false, false, L"Multi-select All: Match Case and Whole Word" },
	{ VK_NULL,    IDM_EDIT_MULTISELECTNEXT,                     false, false, false, L"Multi-select Next: Ignore Case and Whole Word" },
	{ VK_NULL,    IDM_EDIT_MULTISELECTNEXTMATCHCASE,            false, false, false, L"Multi-select Next: Match Case Only" },
	{ VK_NULL,    IDM_EDIT_MULTISELECTNEXTWHOLEWORD,            false, false, false, L"Multi-select Next: Match Whole Word Only" },
	{ VK_NULL,    IDM_EDIT_MULTISELECTNEXTMATCHCASEWHOLEWORD,   false, false, false, L"Multi-select Next: Match Case and Whole Word" },
	{ VK_NULL,    IDM_EDIT_MULTISELECTUNDO,                     false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_MULTISELECTSSKIP,                    false, false, false, nullptr },
//  { VK_NULL,    IDM_EDIT_COLUMNMODETIP,                       false, false, false, nullptr },
	{ VK_C,       IDM_EDIT_COLUMNMODE,                          false, true,  false, nullptr },
	{ VK_NULL,    IDM_EDIT_CHAR_PANEL,                          false, false, false, L"Toggle Character Panel" },
	{ VK_NULL,    IDM_EDIT_CLIPBOARDHISTORY_PANEL,              false, false, false, L"Toggle Clipboard History" },
	{ VK_NULL,    IDM_EDIT_SETREADONLY,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_CLEARREADONLY,                       false, false, false, nullptr },
	{ VK_F,       IDM_SEARCH_FIND,                              true,  false, false, nullptr },
	{ VK_F,       IDM_SEARCH_FINDINFILES,                       true,  false, true,  nullptr },
	{ VK_F3,      IDM_SEARCH_FINDNEXT,                          false, false, false, nullptr },
	{ VK_F3,      IDM_SEARCH_FINDPREV,                          false, false, true,  nullptr },
	{ VK_F3,      IDM_SEARCH_SETANDFINDNEXT,                    true,  false, false, nullptr },
	{ VK_F3,      IDM_SEARCH_SETANDFINDPREV,                    true,  false, true,  nullptr },
	{ VK_F3,      IDM_SEARCH_VOLATILE_FINDNEXT,                 true,  true,  false, nullptr },
	{ VK_F3,      IDM_SEARCH_VOLATILE_FINDPREV,                 true,  true,  true,  nullptr },
	{ VK_H,       IDM_SEARCH_REPLACE,                           true,  false, false, nullptr },
	{ VK_I,       IDM_SEARCH_FINDINCREMENT,                     true,  true,  false, nullptr },
	{ VK_F7,      IDM_FOCUS_ON_FOUND_RESULTS,                   false, false, false, nullptr },
	{ VK_F4,      IDM_SEARCH_GOTOPREVFOUND,                     false, false, true,  nullptr },
	{ VK_F4,      IDM_SEARCH_GOTONEXTFOUND,                     false, false, false, nullptr },
	{ VK_G,       IDM_SEARCH_GOTOLINE,                          true,  false, false, nullptr },
	{ VK_B,       IDM_SEARCH_GOTOMATCHINGBRACE,                 true,  false, false, nullptr },
	{ VK_B,       IDM_SEARCH_SELECTMATCHINGBRACES,              true,  true,  false, nullptr },
	{ VK_NULL,    IDM_SEARCH_CHANGED_PREV,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_CHANGED_NEXT,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_CLEAR_CHANGE_HISTORY,              false, false, false, nullptr },
	{ VK_M,       IDM_SEARCH_MARK,                              true,  false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT1,                       false, false, false, L"Style all using 1st style" },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT2,                       false, false, false, L"Style all using 2nd style" },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT3,                       false, false, false, L"Style all using 3rd style" },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT4,                       false, false, false, L"Style all using 4th style" },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT5,                       false, false, false, L"Style all using 5th style" },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT1,                       false, false, false, L"Style one using 1st style" },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT2,                       false, false, false, L"Style one using 2nd style" },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT3,                       false, false, false, L"Style one using 3rd style" },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT4,                       false, false, false, L"Style one using 4th style" },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT5,                       false, false, false, L"Style one using 5th style" },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT1,                     false, false, false, L"Clear 1st style" },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT2,                     false, false, false, L"Clear 2nd style" },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT3,                     false, false, false, L"Clear 3rd style" },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT4,                     false, false, false, L"Clear 4th style" },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT5,                     false, false, false, L"Clear 5th style" },
	{ VK_NULL,    IDM_SEARCH_CLEARALLMARKS,                     false, false, false, L"Clear all styles" },
	{ VK_1,       IDM_SEARCH_GOPREVMARKER1,                     true,  false, true,  L"Previous style of 1st style" },
	{ VK_2,       IDM_SEARCH_GOPREVMARKER2,                     true,  false, true,  L"Previous style of 2nd style" },
	{ VK_3,       IDM_SEARCH_GOPREVMARKER3,                     true,  false, true,  L"Previous style of 3rd style" },
	{ VK_4,       IDM_SEARCH_GOPREVMARKER4,                     true,  false, true,  L"Previous style of 4th style" },
	{ VK_5,       IDM_SEARCH_GOPREVMARKER5,                     true,  false, true,  L"Previous style of 5th style" },
	{ VK_0,       IDM_SEARCH_GOPREVMARKER_DEF,                  true,  false, true,  L"Previous style of Find Mark style" },
	{ VK_1,       IDM_SEARCH_GONEXTMARKER1,                     true,  false, false, L"Next style of 1st style" },
	{ VK_2,       IDM_SEARCH_GONEXTMARKER2,                     true,  false, false, L"Next style of 2nd style" },
	{ VK_3,       IDM_SEARCH_GONEXTMARKER3,                     true,  false, false, L"Next style of 3rd style" },
	{ VK_4,       IDM_SEARCH_GONEXTMARKER4,                     true,  false, false, L"Next style of 4th style" },
	{ VK_5,       IDM_SEARCH_GONEXTMARKER5,                     true,  false, false, L"Next style of 5th style" },
	{ VK_0,       IDM_SEARCH_GONEXTMARKER_DEF,                  true,  false, false, L"Next style of Find Mark style" },
	{ VK_NULL,    IDM_SEARCH_STYLE1TOCLIP,                      false, false, false, L"Copy Styled Text of 1st Style" },
	{ VK_NULL,    IDM_SEARCH_STYLE2TOCLIP,                      false, false, false, L"Copy Styled Text of 2nd Style" },
	{ VK_NULL,    IDM_SEARCH_STYLE3TOCLIP,                      false, false, false, L"Copy Styled Text of 3rd Style" },
	{ VK_NULL,    IDM_SEARCH_STYLE4TOCLIP,                      false, false, false, L"Copy Styled Text of 4th Style" },
	{ VK_NULL,    IDM_SEARCH_STYLE5TOCLIP,                      false, false, false, L"Copy Styled Text of 5th Style" },
	{ VK_NULL,    IDM_SEARCH_ALLSTYLESTOCLIP,                   false, false, false, L"Copy Styled Text of All Styles" },
	{ VK_NULL,    IDM_SEARCH_MARKEDTOCLIP,                      false, false, false, L"Copy Styled Text of Find Mark style" },
	{ VK_F2,      IDM_SEARCH_TOGGLE_BOOKMARK,                   true,  false, false, nullptr },
	{ VK_F2,      IDM_SEARCH_NEXT_BOOKMARK,                     false, false, false, nullptr },
	{ VK_F2,      IDM_SEARCH_PREV_BOOKMARK,                     false, false, true, nullptr  },
	{ VK_NULL,    IDM_SEARCH_CLEAR_BOOKMARKS,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_CUTMARKEDLINES,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_COPYMARKEDLINES,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_PASTEMARKEDLINES,                  false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_DELETEMARKEDLINES,                 false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_DELETEUNMARKEDLINES,               false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_INVERSEMARKS,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_FINDCHARINRANGE,                   false, false, false, nullptr },
				 
	{ VK_NULL,    IDM_VIEW_ALWAYSONTOP,                         false, false, false, nullptr },
	{ VK_F11,     IDM_VIEW_FULLSCREENTOGGLE,                    false, false, false, nullptr },
	{ VK_F12,     IDM_VIEW_POSTIT,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_DISTRACTIONFREE,                     false, false, false, nullptr },

	{ VK_NULL,    IDM_VIEW_IN_FIREFOX,                          false, false, false, L"View current file in Firefox" },
	{ VK_NULL,    IDM_VIEW_IN_CHROME,                           false, false, false, L"View current file in Chrome" },
	{ VK_NULL,    IDM_VIEW_IN_IE,                               false, false, false, L"View current file in IE" },
	{ VK_NULL,    IDM_VIEW_IN_EDGE,                             false, false, false, L"View current file in Edge" },

	{ VK_NULL,    IDM_VIEW_TAB_SPACE,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_EOL,                                 false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_ALL_CHARACTERS,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_NPC,                                 false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_NPC_CCUNIEOL,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_INDENT_GUIDE,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_WRAP_SYMBOL,                         false, false, false, nullptr },
//  { VK_NULL,    IDM_VIEW_ZOOMIN,                              false, false, false, nullptr },
//  { VK_NULL,    IDM_VIEW_ZOOMOUT,                             false, false, false, nullptr },
//  { VK_NULL,    IDM_VIEW_ZOOMRESTORE,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_GOTO_START,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_GOTO_END,                            false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_GOTO_ANOTHER_VIEW,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_CLONE_TO_ANOTHER_VIEW,               false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_GOTO_NEW_INSTANCE,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_LOAD_IN_NEW_INSTANCE,                false, false, false, nullptr },

	{ VK_NUMPAD1, IDM_VIEW_TAB1,                                true,  false, false, nullptr },
	{ VK_NUMPAD2, IDM_VIEW_TAB2,                                true,  false, false, nullptr },
	{ VK_NUMPAD3, IDM_VIEW_TAB3,                                true,  false, false, nullptr },
	{ VK_NUMPAD4, IDM_VIEW_TAB4,                                true,  false, false, nullptr },
	{ VK_NUMPAD5, IDM_VIEW_TAB5,                                true,  false, false, nullptr },
	{ VK_NUMPAD6, IDM_VIEW_TAB6,                                true,  false, false, nullptr },
	{ VK_NUMPAD7, IDM_VIEW_TAB7,                                true,  false, false, nullptr },
	{ VK_NUMPAD8, IDM_VIEW_TAB8,                                true,  false, false, nullptr },
	{ VK_NUMPAD9, IDM_VIEW_TAB9,                                true,  false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_TAB_START,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_TAB_END,                             false, false, false, nullptr },
	{ VK_NEXT,    IDM_VIEW_TAB_NEXT,                            true,  false, false, nullptr },
	{ VK_PRIOR,   IDM_VIEW_TAB_PREV,                            true,  false, false, nullptr },
	{ VK_NEXT,    IDM_VIEW_TAB_MOVEFORWARD,                     true,  false, true,  nullptr },
	{ VK_PRIOR,   IDM_VIEW_TAB_MOVEBACKWARD,                    true,  false, true,  nullptr },
	{ VK_TAB,     IDC_PREV_DOC,                                 true,  false, true,  L"Switch to previous document" },
	{ VK_TAB,     IDC_NEXT_DOC,                                 true,  false, false, L"Switch to next document" },
	{ VK_NULL,    IDM_VIEW_WRAP,                                false, false, false, nullptr },
	{ VK_H,       IDM_VIEW_HIDELINES,                           false, true,  false, nullptr },
	{ VK_F8,      IDM_VIEW_SWITCHTO_OTHER_VIEW,                 false, false, false, nullptr },

	{ VK_0,       IDM_VIEW_FOLDALL,                             false, true,  false, nullptr },
	{ VK_0,       IDM_VIEW_UNFOLDALL,                           false, true,  true,  nullptr },
	{ VK_F,       IDM_VIEW_FOLD_CURRENT,                        true,  true,  false, nullptr },
	{ VK_F,       IDM_VIEW_UNFOLD_CURRENT,                      true,  true,  true,  nullptr },
	{ VK_1,       IDM_VIEW_FOLD_1,                              false, true,  false, L"Fold Level 1" },
	{ VK_2,       IDM_VIEW_FOLD_2,                              false, true,  false, L"Fold Level 2" },
	{ VK_3,       IDM_VIEW_FOLD_3,                              false, true,  false, L"Fold Level 3" },
	{ VK_4,       IDM_VIEW_FOLD_4,                              false, true,  false, L"Fold Level 4" },
	{ VK_5,       IDM_VIEW_FOLD_5,                              false, true,  false, L"Fold Level 5" },
	{ VK_6,       IDM_VIEW_FOLD_6,                              false, true,  false, L"Fold Level 6" },
	{ VK_7,       IDM_VIEW_FOLD_7,                              false, true,  false, L"Fold Level 7" },
	{ VK_8,       IDM_VIEW_FOLD_8,                              false, true,  false, L"Fold Level 8" },

	{ VK_1,       IDM_VIEW_UNFOLD_1,                            false, true,  true,  L"Unfold Level 1" },
	{ VK_2,       IDM_VIEW_UNFOLD_2,                            false, true,  true,  L"Unfold Level 2" },
	{ VK_3,       IDM_VIEW_UNFOLD_3,                            false, true,  true,  L"Unfold Level 3" },
	{ VK_4,       IDM_VIEW_UNFOLD_4,                            false, true,  true,  L"Unfold Level 4" },
	{ VK_5,       IDM_VIEW_UNFOLD_5,                            false, true,  true,  L"Unfold Level 5" },
	{ VK_6,       IDM_VIEW_UNFOLD_6,                            false, true,  true,  L"Unfold Level 6" },
	{ VK_7,       IDM_VIEW_UNFOLD_7,                            false, true,  true,  L"Unfold Level 7" },
	{ VK_8,       IDM_VIEW_UNFOLD_8,                            false, true,  true,  L"Unfold Level 8" },
	{ VK_NULL,    IDM_VIEW_SUMMARY,                             false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_1,                     false, false, false, L"Toggle Project Panel 1" },
	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_2,                     false, false, false, L"Toggle Project Panel 2" },
	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_3,                     false, false, false, L"Toggle Project Panel 3" },
	{ VK_NULL,    IDM_VIEW_FILEBROWSER,                         false, false, false, L"Toggle Folder as Workspace" },
	{ VK_NULL,    IDM_VIEW_DOC_MAP,                             false, false, false, L"Toggle Document Map" },
	{ VK_NULL,    IDM_VIEW_DOCLIST,                             false, false, false, L"Toggle Document List" },
	{ VK_NULL,    IDM_VIEW_FUNC_LIST,                           false, false, false, L"Toggle Function List" },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_1,            false, false, false, L"Switch to Project Panel 1" },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_2,            false, false, false, L"Switch to Project Panel 2" },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_3,            false, false, false, L"Switch to Project Panel 3" },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_FILEBROWSER,                false, false, false, L"Switch to Folder as Workspace" },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_FUNC_LIST,                  false, false, false, L"Switch to Function List" },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_DOCLIST,                    false, false, false, L"Switch to Document List" },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_NONE,                     false, false, false, L"Remove Tab Colour" },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_1,                        false, false, false, L"Apply Tab Colour 1" },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_2,                        false, false, false, L"Apply Tab Colour 2" },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_3,                        false, false, false, L"Apply Tab Colour 3" },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_4,                        false, false, false, L"Apply Tab Colour 4" },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_5,                        false, false, false, L"Apply Tab Colour 5" },
	{ VK_NULL,    IDM_VIEW_SYNSCROLLV,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_SYNSCROLLH,                          false, false, false, nullptr },
	{ VK_R,       IDM_EDIT_RTL,                                 true,  true,  false, nullptr },
	{ VK_L,       IDM_EDIT_LTR,                                 true,  true,  false, nullptr },
	{ VK_NULL,    IDM_VIEW_MONITORING,                          false, false, false, nullptr },

	{ VK_NULL,    IDM_FORMAT_ANSI,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_AS_UTF_8,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_UTF_8,                             false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_UTF_16BE,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_UTF_16LE,                          false, false, false, nullptr },

	{ VK_NULL,    IDM_FORMAT_ISO_8859_6,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1256,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_13,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1257,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_14,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_5,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_MAC_CYRILLIC,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_KOI8R_CYRILLIC,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_KOI8U_CYRILLIC,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1251,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1250,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_437,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_720,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_737,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_775,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_850,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_852,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_855,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_857,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_858,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_860,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_861,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_862,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_863,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_865,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_866,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_869,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_BIG5,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_GB2312,                            false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_2,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_7,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1253,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_8,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1255,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_SHIFT_JIS,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_EUC_KR,                            false, false, false, nullptr },
	//{ VK_NULL,    IDM_FORMAT_ISO_8859_10,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_15,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_4,                        false, false, false, nullptr },
	//{ VK_NULL,    IDM_FORMAT_ISO_8859_16,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_3,                        false, false, false, nullptr },
	//{ VK_NULL,    IDM_FORMAT_ISO_8859_11,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_TIS_620,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_9,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1254,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1252,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_1,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1258,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_ANSI,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_AS_UTF_8,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_8,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_16BE,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_16LE,                    false, false, false, nullptr },

	{ VK_NULL,    IDM_LANG_USER_DLG,                            false, false, false, nullptr },
	{ VK_NULL,    IDM_LANG_USER,                                false, false, false, nullptr },
	{ VK_NULL,    IDM_LANG_OPENUDLDIR,                          false, false, false, nullptr },

	{ VK_NULL,    IDM_SETTING_PREFERENCE,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_LANGSTYLE_CONFIG_DLG,                     false, false, false, nullptr },
	{ VK_NULL,    IDM_SETTING_SHORTCUT_MAPPER,                  false, false, false, nullptr },
	{ VK_NULL,    IDM_SETTING_IMPORTPLUGIN,                     false, false, false, nullptr },
	{ VK_NULL,    IDM_SETTING_IMPORTSTYLETHEMES,                false, false, false, nullptr },
	{ VK_NULL,    IDM_SETTING_EDITCONTEXTMENU,                  false, false, false, nullptr },

	{ VK_R,       IDC_EDIT_TOGGLEMACRORECORDING,                true,  false, true,  L"Toggle macro recording" },
	{ VK_NULL,    IDM_MACRO_STARTRECORDINGMACRO,                false, false, false, nullptr },
	{ VK_NULL,    IDM_MACRO_STOPRECORDINGMACRO,                 false, false, false, nullptr },
	{ VK_P,       IDM_MACRO_PLAYBACKRECORDEDMACRO,              true,  false, true,  nullptr },
	{ VK_NULL,    IDM_MACRO_SAVECURRENTMACRO,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_MACRO_RUNMULTIMACRODLG,                   false, false, false, nullptr },

	{ VK_F5,      IDM_EXECUTE,                                  false, false, false, nullptr },

	{ VK_NULL,    IDM_WINDOW_WINDOWS,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_WINDOW_SORT_FN_ASC,                       false, false, false, L"Sort by Name A to Z" },
	{ VK_NULL,    IDM_WINDOW_SORT_FN_DSC,                       false, false, false, L"Sort by Name Z to A" },
	{ VK_NULL,    IDM_WINDOW_SORT_FP_ASC,                       false, false, false, L"Sort by Path A to Z" },
	{ VK_NULL,    IDM_WINDOW_SORT_FP_DSC,                       false, false, false, L"Sort by Path Z to A" },
	{ VK_NULL,    IDM_WINDOW_SORT_FT_ASC,                       false, false, false, L"Sort by Type A to Z" },
	{ VK_NULL,    IDM_WINDOW_SORT_FT_DSC,                       false, false, false, L"Sort by Type Z to A" },
	{ VK_NULL,    IDM_WINDOW_SORT_FS_ASC,                       false, false, false, L"Sort by Content Length Ascending" },
	{ VK_NULL,    IDM_WINDOW_SORT_FS_DSC,                       false, false, false, L"Sort by Content Length Descending" },

	{ VK_NULL,    IDM_CMDLINEARGUMENTS,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_HOMESWEETHOME,                            false, false, false, nullptr },
	{ VK_NULL,    IDM_PROJECTPAGE,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_ONLINEDOCUMENT,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORUM,                                    false, false, false, nullptr },
//	{ VK_NULL,    IDM_ONLINESUPPORT,                            false, false, false, nullptr },
//	{ VK_NULL,    IDM_PLUGINSHOME,                              false, false, false, nullptr },

	// The following two commands are not in menu if (nppGUI._doesExistUpdater == 0).
	// They cannot be derived from menu then, only for this reason the text is specified here.
	// In localized environments, the text comes preferably from xml Menu/Main/Commands.
	{ VK_NULL,    IDM_UPDATE_NPP,                               false, false, false, L"Update Notepad++" },
	{ VK_NULL,    IDM_CONFUPDATERPROXY,                         false, false, false, L"Set Updater Proxy..." },
	{ VK_NULL,    IDM_DEBUGINFO,                                false, false, false, nullptr },
	{ VK_F1,      IDM_ABOUT,                                    false, false, false, nullptr }
};



struct ScintillaKeyDefinition
{
	const wchar_t* name = nullptr;
	int functionId = 0;
	bool isCtrl = false;
	bool isAlt = false;
	bool isShift = false;
	int vKey = 0;
	int redirFunctionId = 0; // this gets set when a function is being redirected through Notepad++ if Scintilla doesnt do it properly :)
};

/*!
** \brief array of accelerator keys for all possible scintilla functions
**
** values can be 0 for vKey, which means its unused
*/
static const ScintillaKeyDefinition scintKeyDefs[] =
{
    //Scintilla command name,             SCINTILLA_CMD_ID,            Ctrl,  Alt,   Shift, V_KEY,       NOTEPAD++_CMD_ID
	// -------------------------------------------------------------------------------------------------------------------
	//
	//{L"SCI_CUT",                     SCI_CUT,                     true,  false, false, VK_X,        IDM_EDIT_CUT},
	//{L"",                            SCI_CUT,                     false, false, true,  VK_DELETE,   0},
	//{L"SCI_COPY",                    SCI_COPY,                    true,  false, false, VK_C,        IDM_EDIT_COPY},
	//{L"",                            SCI_COPY,                    true,  false, false, VK_INSERT,   0},
	//{L"SCI_PASTE",                   SCI_PASTE,                   true,  false, false, VK_V,        IDM_EDIT_PASTE},
	//{L"SCI_PASTE",                   SCI_PASTE,                   false, false, true,  VK_INSERT,   IDM_EDIT_PASTE},
	{L"SCI_SELECTALL",               SCI_SELECTALL,               true,  false, false, VK_A,        IDM_EDIT_SELECTALL},
	{L"SCI_CLEAR",                   SCI_CLEAR,                   false, false, false, VK_DELETE,   IDM_EDIT_DELETE},
	{L"SCI_CLEARALL",                SCI_CLEARALL,                false, false, false, 0,           0},
	{L"SCI_UNDO",                    SCI_UNDO,                    true,  false, false, VK_Z,        IDM_EDIT_UNDO},
	{L"",                            SCI_UNDO,                    false, true,  false, VK_BACK,     0},
	{L"SCI_REDO",                    SCI_REDO,                    true,  false, false, VK_Y,        IDM_EDIT_REDO},
	{L"",                            SCI_REDO,                    true,  false, true,  VK_Z,        0},
	{L"SCI_NEWLINE",                 SCI_NEWLINE,                 false, false, false, VK_RETURN,   0},
	{L"",                            SCI_NEWLINE,                 false, false, true,  VK_RETURN,   0},
	{L"SCI_TAB",                     SCI_TAB,                     false, false, false, VK_TAB,      0},
	{L"SCI_BACKTAB",                 SCI_BACKTAB,                 false, false, true,  VK_TAB,      0},
	{L"SCI_FORMFEED",                SCI_FORMFEED,                false, false, false, 0,           0},
	{L"SCI_ZOOMIN",                  SCI_ZOOMIN,                  true,  false, false, VK_ADD,      IDM_VIEW_ZOOMIN},
	{L"SCI_ZOOMOUT",                 SCI_ZOOMOUT,                 true,  false, false, VK_SUBTRACT, IDM_VIEW_ZOOMOUT},
	{L"SCI_SETZOOM",                 SCI_SETZOOM,                 true,  false, false, VK_DIVIDE,   IDM_VIEW_ZOOMRESTORE},
	{L"SCI_SELECTIONDUPLICATE",      SCI_SELECTIONDUPLICATE,      true,  false, false, VK_D,        0},
	{L"SCI_LINESJOIN",               SCI_LINESJOIN,               false, false, false, 0,           0},
	{L"SCI_SCROLLCARET",             SCI_SCROLLCARET,             false, false, false, 0,           0},
	{L"SCI_EDITTOGGLEOVERTYPE",      SCI_EDITTOGGLEOVERTYPE,      false, false, false, VK_INSERT,   0},
	{L"SCI_MOVECARETINSIDEVIEW",     SCI_MOVECARETINSIDEVIEW,     false, false, false, 0,           0},
	{L"SCI_LINEDOWN",                SCI_LINEDOWN,                false, false, false, VK_DOWN,     0},
	{L"SCI_LINEDOWNEXTEND",          SCI_LINEDOWNEXTEND,          false, false, true,  VK_DOWN,     0},
	{L"SCI_LINEDOWNRECTEXTEND",      SCI_LINEDOWNRECTEXTEND,      false, true,  true,  VK_DOWN,     0},
	{L"SCI_LINESCROLLDOWN",          SCI_LINESCROLLDOWN,          true,  false, false, VK_DOWN,     0},
	{L"SCI_LINEUP",                  SCI_LINEUP,                  false, false, false, VK_UP,       0},
	{L"SCI_LINEUPEXTEND",            SCI_LINEUPEXTEND,            false, false, true,  VK_UP,       0},
	{L"SCI_LINEUPRECTEXTEND",        SCI_LINEUPRECTEXTEND,        false, true,  true,  VK_UP,       0},
	{L"SCI_LINESCROLLUP",            SCI_LINESCROLLUP,            true,  false, false, VK_UP,       0},
	{L"SCI_PARADOWN",                SCI_PARADOWN,                true,  false, false, VK_OEM_6,    0},
	{L"SCI_PARADOWNEXTEND",          SCI_PARADOWNEXTEND,          true,  false, true,  VK_OEM_6,    0},
	{L"SCI_PARAUP",                  SCI_PARAUP,                  true,  false, false, VK_OEM_4,    0},
	{L"SCI_PARAUPEXTEND",            SCI_PARAUPEXTEND,            true,  false, true,  VK_OEM_4,    0},
	{L"SCI_CHARLEFT",                SCI_CHARLEFT,                false, false, false, VK_LEFT,     0},
	{L"SCI_CHARLEFTEXTEND",          SCI_CHARLEFTEXTEND,          false, false, true,  VK_LEFT,     0},
	{L"SCI_CHARLEFTRECTEXTEND",      SCI_CHARLEFTRECTEXTEND,      false, true,  true,  VK_LEFT,     0},
	{L"SCI_CHARRIGHT",               SCI_CHARRIGHT,               false, false, false, VK_RIGHT,    0},
	{L"SCI_CHARRIGHTEXTEND",         SCI_CHARRIGHTEXTEND,         false, false, true,  VK_RIGHT,    0},
	{L"SCI_CHARRIGHTRECTEXTEND",     SCI_CHARRIGHTRECTEXTEND,     false, true,  true,  VK_RIGHT,    0},
	{L"SCI_WORDLEFT",                SCI_WORDLEFT,                true,  false, false, VK_LEFT,     0},
	{L"SCI_WORDLEFTEXTEND",          SCI_WORDLEFTEXTEND,          true,  false, true,  VK_LEFT,     0},
	{L"SCI_WORDRIGHT",               SCI_WORDRIGHT,               true,  false, false, VK_RIGHT,    0},
	{L"SCI_WORDRIGHTEXTEND",         SCI_WORDRIGHTEXTEND,         false, false, false, 0,           0},
	{L"SCI_WORDLEFTEND",             SCI_WORDLEFTEND,             false, false, false, 0,           0},
	{L"SCI_WORDLEFTENDEXTEND",       SCI_WORDLEFTENDEXTEND,       false, false, false, 0,           0},
	{L"SCI_WORDRIGHTEND",            SCI_WORDRIGHTEND,            false, false, false, 0,           0},
	{L"SCI_WORDRIGHTENDEXTEND",      SCI_WORDRIGHTENDEXTEND,      true,  false, true,  VK_RIGHT,    0},
	{L"SCI_WORDPARTLEFT",            SCI_WORDPARTLEFT,            true,  false, false, VK_OEM_2,    0},
	{L"SCI_WORDPARTLEFTEXTEND",      SCI_WORDPARTLEFTEXTEND,      true,  false, true,  VK_OEM_2,    0},
	{L"SCI_WORDPARTRIGHT",           SCI_WORDPARTRIGHT,           true,  false, false, VK_OEM_5,    0},
	{L"SCI_WORDPARTRIGHTEXTEND",     SCI_WORDPARTRIGHTEXTEND,     true,  false, true,  VK_OEM_5,    0},
	{L"SCI_HOME",                    SCI_HOME,                    false, false, false, 0,           0},
	{L"SCI_HOMEEXTEND",              SCI_HOMEEXTEND,              false, false, false, 0,           0},
	{L"SCI_HOMERECTEXTEND",          SCI_HOMERECTEXTEND,          false, false, false, 0,           0},
	{L"SCI_HOMEDISPLAY",             SCI_HOMEDISPLAY,             false, true,  false, VK_HOME,     0},
	{L"SCI_HOMEDISPLAYEXTEND",       SCI_HOMEDISPLAYEXTEND,       false, false, false, 0,           0},
	{L"SCI_HOMEWRAP",                SCI_HOMEWRAP,                false, false, false, 0,           0},
	{L"SCI_HOMEWRAPEXTEND",          SCI_HOMEWRAPEXTEND,          false, false, false, 0,           0},
	{L"SCI_VCHOME",                  SCI_VCHOME,                  false, false, false, 0,           0},
	{L"SCI_VCHOMEEXTEND",            SCI_VCHOMEEXTEND,            false, false, false, 0,           0},
	{L"SCI_VCHOMERECTEXTEND",        SCI_VCHOMERECTEXTEND,        false, true,  true,  VK_HOME,     0},
	{L"SCI_VCHOMEDISPLAY",           SCI_VCHOMEDISPLAY,           false, false, false, 0,           0},
	{L"SCI_VCHOMEDISPLAYEXTEND",     SCI_VCHOMEDISPLAYEXTEND,     false, false, false, 0,           0},
	{L"SCI_VCHOMEWRAP",              SCI_VCHOMEWRAP,              false, false, false, VK_HOME,     0},
	{L"SCI_VCHOMEWRAPEXTEND",        SCI_VCHOMEWRAPEXTEND,        false, false, true,  VK_HOME,     0},
	{L"SCI_LINEEND",                 SCI_LINEEND,                 false, false, false, 0,           0},
	{L"SCI_LINEENDWRAPEXTEND",       SCI_LINEENDWRAPEXTEND,       false, false, true,  VK_END,      0},
	{L"SCI_LINEENDRECTEXTEND",       SCI_LINEENDRECTEXTEND,       false, true,  true,  VK_END,      0},
	{L"SCI_LINEENDDISPLAY",          SCI_LINEENDDISPLAY,          false, true,  false, VK_END,      0},
	{L"SCI_LINEENDDISPLAYEXTEND",    SCI_LINEENDDISPLAYEXTEND,    false, false, false, 0,           0},
	{L"SCI_LINEENDWRAP",             SCI_LINEENDWRAP,             false, false, false, VK_END,      0},
	{L"SCI_LINEENDEXTEND",           SCI_LINEENDEXTEND,           false, false, false, 0,           0},
	{L"SCI_DOCUMENTSTART",           SCI_DOCUMENTSTART,           true,  false, false, VK_HOME,     0},
	{L"SCI_DOCUMENTSTARTEXTEND",     SCI_DOCUMENTSTARTEXTEND,     true,  false, true,  VK_HOME,     0},
	{L"SCI_DOCUMENTEND",             SCI_DOCUMENTEND,             true,  false, false, VK_END,      0},
	{L"SCI_DOCUMENTENDEXTEND",       SCI_DOCUMENTENDEXTEND,       true,  false, true,  VK_END,      0},
	{L"SCI_PAGEUP",                  SCI_PAGEUP,                  false, false, false, VK_PRIOR,    0},
	{L"SCI_PAGEUPEXTEND",            SCI_PAGEUPEXTEND,            false, false, true,  VK_PRIOR,    0},
	{L"SCI_PAGEUPRECTEXTEND",        SCI_PAGEUPRECTEXTEND,        false, true,  true,  VK_PRIOR,    0},
	{L"SCI_PAGEDOWN",                SCI_PAGEDOWN,                false, false, false, VK_NEXT,     0},
	{L"SCI_PAGEDOWNEXTEND",          SCI_PAGEDOWNEXTEND,          false, false, true,  VK_NEXT,     0},
	{L"SCI_PAGEDOWNRECTEXTEND",      SCI_PAGEDOWNRECTEXTEND,      false, true,  true,  VK_NEXT,     0},
	{L"SCI_STUTTEREDPAGEUP",         SCI_STUTTEREDPAGEUP,         false, false, false, 0,           0},
	{L"SCI_STUTTEREDPAGEUPEXTEND",   SCI_STUTTEREDPAGEUPEXTEND,   false, false, false, 0,           0},
	{L"SCI_STUTTEREDPAGEDOWN",       SCI_STUTTEREDPAGEDOWN,       false, false, false, 0,           0},
	{L"SCI_STUTTEREDPAGEDOWNEXTEND", SCI_STUTTEREDPAGEDOWNEXTEND, false, false, false, 0,           0},
	{L"SCI_DELETEBACK",              SCI_DELETEBACK,              false, false, false, VK_BACK,     0},
	{L"",                            SCI_DELETEBACK,              false, false, true,  VK_BACK,     0},
	{L"SCI_DELETEBACKNOTLINE",       SCI_DELETEBACKNOTLINE,       false, false, false, 0,           0},
	{L"SCI_DELWORDLEFT",             SCI_DELWORDLEFT,             true,  false, false, VK_BACK,     0},
	{L"SCI_DELWORDRIGHT",            SCI_DELWORDRIGHT,            true,  false, false, VK_DELETE,   0},
	{L"SCI_DELLINELEFT",             SCI_DELLINELEFT,             true,  false, true,  VK_BACK,     0},
	{L"SCI_DELLINERIGHT",            SCI_DELLINERIGHT,            true,  false, true,  VK_DELETE,   0},
	{L"SCI_LINEDELETE",              SCI_LINEDELETE,              true,  false, true,  VK_L,        0},
	{L"SCI_LINECUT",                 SCI_LINECUT,                 true,  false, false, VK_L,        0},
	{L"SCI_LINECOPY",                SCI_LINECOPY,                true,  false, true,  VK_X,        0},
	{L"SCI_LINETRANSPOSE",           SCI_LINETRANSPOSE,           true,  false, false, VK_T,        0},
	{L"SCI_LINEDUPLICATE",           SCI_LINEDUPLICATE,           false, false, false, 0,           IDM_EDIT_DUP_LINE},
	{L"SCI_CANCEL",                  SCI_CANCEL,                  false, false, false, VK_ESCAPE,   0},
	{L"SCI_SWAPMAINANCHORCARET",     SCI_SWAPMAINANCHORCARET,     false, false, false, 0,           0},
	{L"SCI_ROTATESELECTION",         SCI_ROTATESELECTION,         false, false, false, 0,           0}
};

#define NONEEDSHORTCUTSXMLBACKUP_FILENAME L"v852NoNeedShortcutsBackup.xml"
#define SHORTCUTSXML_FILENAME L"shortcuts.xml"

#define SESSION_BACKUP_EXT L".inCaseOfCorruption.bak"

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

int strVal(const wchar_t *str, int base)
{
	if (!str) return -1;
	if (!str[0]) return 0;

	wchar_t *finStr;
	int result = wcstol(str, &finStr, base);
	if (*finStr != '\0')
		return -1;
	return result;
}


int decStrVal(const wchar_t *str)
{
	return strVal(str, 10);
}

int hexStrVal(const wchar_t *str)
{
	return strVal(str, 16);
}

int getKwClassFromName(const wchar_t *str)
{
	if (!lstrcmp(L"instre1", str)) return LANG_INDEX_INSTR;
	if (!lstrcmp(L"instre2", str)) return LANG_INDEX_INSTR2;
	if (!lstrcmp(L"type1", str)) return LANG_INDEX_TYPE;
	if (!lstrcmp(L"type2", str)) return LANG_INDEX_TYPE2;
	if (!lstrcmp(L"type3", str)) return LANG_INDEX_TYPE3;
	if (!lstrcmp(L"type4", str)) return LANG_INDEX_TYPE4;
	if (!lstrcmp(L"type5", str)) return LANG_INDEX_TYPE5;
	if (!lstrcmp(L"type6", str)) return LANG_INDEX_TYPE6;
	if (!lstrcmp(L"type7", str)) return LANG_INDEX_TYPE7;
	if (!lstrcmp(L"substyle1", str)) return LANG_INDEX_SUBSTYLE1;
	if (!lstrcmp(L"substyle2", str)) return LANG_INDEX_SUBSTYLE2;
	if (!lstrcmp(L"substyle3", str)) return LANG_INDEX_SUBSTYLE3;
	if (!lstrcmp(L"substyle4", str)) return LANG_INDEX_SUBSTYLE4;
	if (!lstrcmp(L"substyle5", str)) return LANG_INDEX_SUBSTYLE5;
	if (!lstrcmp(L"substyle6", str)) return LANG_INDEX_SUBSTYLE6;
	if (!lstrcmp(L"substyle7", str)) return LANG_INDEX_SUBSTYLE7;
	if (!lstrcmp(L"substyle8", str)) return LANG_INDEX_SUBSTYLE8;

	if ((str[1] == '\0') && (str[0] >= '0') && (str[0] <= '8')) // up to KEYWORDSET_MAX
		return str[0] - '0';

	return -1;
}


} // anonymous namespace


void cutString(const wchar_t* str2cut, vector<std::wstring>& patternVect)
{
	if (str2cut == nullptr) return;

	const wchar_t *pBegin = str2cut;
	const wchar_t *pEnd = pBegin;

	while (*pEnd != '\0')
	{
		if (_istspace(*pEnd))
		{
			if (pBegin != pEnd)
				patternVect.emplace_back(pBegin, pEnd);
			pBegin = pEnd + 1;
		
		}
		++pEnd;
	}

	if (pBegin != pEnd)
		patternVect.emplace_back(pBegin, pEnd);
}

void cutStringBy(const wchar_t* str2cut, vector<std::wstring>& patternVect, char byChar, bool allowEmptyStr)
{
	if (str2cut == nullptr) return;

	const wchar_t* pBegin = str2cut;
	const wchar_t* pEnd = pBegin;

	while (*pEnd != '\0')
	{
		if (*pEnd == byChar)
		{
			if (allowEmptyStr)
				patternVect.emplace_back(pBegin, pEnd);
			else if (pBegin != pEnd)
				patternVect.emplace_back(pBegin, pEnd);
			pBegin = pEnd + 1;
		}
		++pEnd;
	}
	if (allowEmptyStr)
		patternVect.emplace_back(pBegin, pEnd);
	else if (pBegin != pEnd)
		patternVect.emplace_back(pBegin, pEnd);
}


std::wstring LocalizationSwitcher::getLangFromXmlFileName(const wchar_t *fn) const
{
	size_t nbItem = sizeof(localizationDefs)/sizeof(LocalizationSwitcher::LocalizationDefinition);
	for (size_t i = 0 ; i < nbItem ; ++i)
	{
		if (0 == wcsicmp(fn, localizationDefs[i]._xmlFileName))
			return localizationDefs[i]._langName;
	}
	return std::wstring();
}


std::wstring LocalizationSwitcher::getXmlFilePathFromLangName(const wchar_t *langName) const
{
	for (size_t i = 0, len = _localizationList.size(); i < len ; ++i)
	{
		if (0 == wcsicmp(langName, _localizationList[i].first.c_str()))
			return _localizationList[i].second;
	}
	return std::wstring();
}


bool LocalizationSwitcher::addLanguageFromXml(const std::wstring& xmlFullPath)
{
	const wchar_t * fn = ::PathFindFileNameW(xmlFullPath.c_str());
	wstring foundLang = getLangFromXmlFileName(fn);
	if (!foundLang.empty())
	{
		_localizationList.push_back(pair<wstring, wstring>(foundLang, xmlFullPath));
		return true;
	}
	return false;
}


bool LocalizationSwitcher::switchToLang(const wchar_t *lang2switch) const
{
	wstring langPath = getXmlFilePathFromLangName(lang2switch);
	if (langPath.empty())
		return false;

	return ::CopyFileW(langPath.c_str(), _nativeLangPath.c_str(), FALSE) != FALSE;
}


std::wstring ThemeSwitcher::getThemeFromXmlFileName(const wchar_t *xmlFullPath) const
{
	if (!xmlFullPath || !xmlFullPath[0])
		return std::wstring();
	std::wstring fn(::PathFindFileName(xmlFullPath));
	PathRemoveExtension(const_cast<wchar_t *>(fn.c_str()));
	return fn;
}

int DynamicMenu::getTopLevelItemNumber() const
{
	int nb = 0;
	std::wstring previousFolderName;
	for (const MenuItemUnit& i : _menuItems)
	{
		if (i._parentFolderName.empty())
		{
			++nb;
		}
		else
		{
			if (previousFolderName.empty())
			{
				++nb;
				previousFolderName = i._parentFolderName;
			}
			else // previousFolderName is not empty
			{
				if (i._parentFolderName.empty())
				{
					++nb;
					previousFolderName = i._parentFolderName;
				}
				else if (previousFolderName == i._parentFolderName)
				{
					// maintain the number and do nothinh
				}
				else
				{
					++nb;
					previousFolderName = i._parentFolderName;
				}
			}
		}
	}

	return nb;
}

bool DynamicMenu::attach(HMENU hMenu, unsigned int posBase, int lastCmd, const std::wstring& lastCmdLabel)
{
	if (!hMenu) return false;

	_hMenu = hMenu;
	_posBase = posBase;
	_lastCmd = lastCmd;
	_lastCmdLabel = lastCmdLabel;

	return createMenu();
}

bool DynamicMenu::clearMenu() const
{
	if (!_hMenu) return false;

	int nbTopItem = getTopLevelItemNumber();
	for (int i = nbTopItem + 1; i >= 0 ; --i)
	{
		::DeleteMenu(_hMenu, static_cast<int32_t>(_posBase) + i, MF_BYPOSITION);
	}

	return true;
}

bool DynamicMenu::createMenu() const
{
	if (!_hMenu) return false;

	bool lastIsSep = false;
	HMENU hParentFolder = NULL;
	std::wstring currentParentFolderStr;
	int j = 0;

	size_t nb = _menuItems.size();
	size_t i = 0;
	for (; i < nb; ++i)
	{
		const MenuItemUnit& item = _menuItems[i];
		if (item._parentFolderName.empty())
		{
			currentParentFolderStr.clear();
			hParentFolder = NULL;
			j = 0;
		}
		else
		{
			if (item._parentFolderName != currentParentFolderStr)
			{
				currentParentFolderStr = item._parentFolderName;
				hParentFolder = ::CreateMenu();
				j = 0;

				::InsertMenu(_hMenu, static_cast<UINT>(_posBase + i), MF_BYPOSITION | MF_POPUP, (UINT_PTR)hParentFolder, currentParentFolderStr.c_str());
			}
		}

		unsigned int flag = MF_BYPOSITION | ((item._cmdID == 0) ? MF_SEPARATOR : 0);
		if (hParentFolder)
		{
			::InsertMenu(hParentFolder, j++, flag, item._cmdID, item._itemName.c_str());
			lastIsSep = false;
		}
		else if ((i == 0 || i == _menuItems.size() - 1) && item._cmdID == 0)
		{
			lastIsSep = true;
		}
		else if (item._cmdID != 0)
		{
			::InsertMenu(_hMenu, static_cast<UINT>(_posBase + i), flag, item._cmdID, item._itemName.c_str());
			lastIsSep = false;
		}
		else if (item._cmdID == 0 && !lastIsSep)
		{
			::InsertMenu(_hMenu, static_cast<int32_t>(_posBase + i), flag, item._cmdID, item._itemName.c_str());
			lastIsSep = true;
		}
		else // last item is separator and current item is separator
		{
			lastIsSep = true;
		}
	}

	if (nb > 0)
	{
		::InsertMenu(_hMenu, static_cast<int32_t>(_posBase + i), MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
		::InsertMenu(_hMenu, static_cast<UINT>(_posBase + i + 2), MF_BYCOMMAND, _lastCmd, _lastCmdLabel.c_str());
	}

	return true;
}

winVer NppParameters::getWindowsVersion()
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	BOOL bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *)&osvi);
	if (!bOsVersionInfoEx)
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) )
			return WV_UNKNOWN;
	}

	pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
	if (pGNSI != NULL)
		pGNSI(&si);
	else
		GetSystemInfo(&si);

	switch (si.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_IA64:
		_platForm = PF_IA64;
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		_platForm = PF_X64;
		break;

	case PROCESSOR_ARCHITECTURE_INTEL:
		_platForm = PF_X86;
		break;

	case PROCESSOR_ARCHITECTURE_ARM64:
		_platForm = PF_ARM64;
		break;

	default:
		_platForm = PF_UNKNOWN;
	}

   switch (osvi.dwPlatformId)
   {
		case VER_PLATFORM_WIN32_NT:
		{
			if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber >= 22000)
				return WV_WIN11;

			if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0)
				return WV_WIN10;

			if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
				return WV_WIN81;

			if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
				return WV_WIN8;

			if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
				return WV_WIN7;

			if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
				return WV_VISTA;

			if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
			{
				if (osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
					return WV_XPX64;
				return WV_S2003;
			}

			if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
				return WV_XP;

			if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
				return WV_W2K;

			if (osvi.dwMajorVersion <= 4)
				return WV_NT;
			break;
		}

		// Test for the Windows Me/98/95.
		case VER_PLATFORM_WIN32_WINDOWS:
		{
			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
				return WV_95;

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
				return WV_98;

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
				return WV_ME;
			break;
		}

		case VER_PLATFORM_WIN32s:
			return WV_WIN32S;

		default:
			return WV_UNKNOWN;
   }

   return WV_UNKNOWN;
}


NppParameters::NppParameters()
{
	//Get windows version
	_winVersion = getWindowsVersion();

	// Prepare for default path
	wchar_t nppPath[MAX_PATH];
	::GetModuleFileName(NULL, nppPath, MAX_PATH);

	PathRemoveFileSpec(nppPath);
	_nppPath = nppPath;

	//Initialize current directory to startup directory
	wchar_t curDir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, curDir);
	_currentDirectory = curDir;

	_appdataNppDir.clear();
	std::wstring notepadStylePath(_nppPath);
	pathAppend(notepadStylePath, notepadStyleFile);

	_asNotepadStyle = (doesFileExist(notepadStylePath.c_str()) == TRUE);

	//Load initial accelerator key definitions
	initMenuKeys();
	initScintillaKeys();
}


NppParameters::~NppParameters()
{
	for (int i = 0 ; i < _nbLang ; ++i)
		delete _langList[i];
	for (int i = 0 ; i < _nbRecentFile ; ++i)
		delete _LRFileList[i];
	for (int i = 0 ; i < _nbUserLang ; ++i)
		delete _userLangArray[i];

	for (std::vector<TiXmlDocument *>::iterator it = _pXmlExternalLexerDoc.begin(), end = _pXmlExternalLexerDoc.end(); it != end; ++it )
		delete (*it);

	_pXmlExternalLexerDoc.clear();
}


bool NppParameters::reloadStylers(const wchar_t* stylePath)
{
	delete _pXmlUserStylerDoc;

	const wchar_t* stylePathToLoad = stylePath != nullptr ? stylePath : _stylerPath.c_str();
	_pXmlUserStylerDoc = new TiXmlDocument(stylePathToLoad);

	bool loadOkay = _pXmlUserStylerDoc->LoadFile();
	if (!loadOkay)
	{
		if (!_pNativeLangSpeaker)
		{
			::MessageBox(NULL, stylePathToLoad, L"Load stylers.xml failed", MB_OK);
		}
		else
		{
			_pNativeLangSpeaker->messageBox("LoadStylersFailed",
				NULL,
				L"Load \"$STR_REPLACE$\" failed!",
				L"Load stylers.xml failed",
				MB_OK,
				0,
				stylePathToLoad);
		}
		delete _pXmlUserStylerDoc;
		_pXmlUserStylerDoc = NULL;
		return false;
	}
	_lexerStylerVect.clear();
	_widgetStyleArray.clear();

	getUserStylersFromXmlTree();

	//  Reload plugin styles.
	for ( size_t i = 0; i < getExternalLexerDoc()->size(); ++i)
	{
		getExternalLexerFromXmlTree( getExternalLexerDoc()->at(i) );
	}
	return true;
}

bool NppParameters::reloadLang()
{
	// use user path
	std::wstring nativeLangPath(_localizationSwitcher._nativeLangPath);

	// if "nativeLang.xml" does not exist, use npp path
	if (!doesFileExist(nativeLangPath.c_str()))
	{
		nativeLangPath = _nppPath;
		pathAppend(nativeLangPath, std::wstring(L"nativeLang.xml"));
		if (!doesFileExist(nativeLangPath.c_str()))
			return false;
	}

	delete _pXmlNativeLangDocA;

	_pXmlNativeLangDocA = new TiXmlDocumentA();

	bool loadOkay = _pXmlNativeLangDocA->LoadUnicodeFilePath(nativeLangPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlNativeLangDocA;
		_pXmlNativeLangDocA = nullptr;
		return false;
	}
	return loadOkay;
}

std::wstring NppParameters::getSpecialFolderLocation(int folderKind)
{
	wchar_t path[MAX_PATH];
	const HRESULT specialLocationResult = SHGetFolderPath(nullptr, folderKind, nullptr, SHGFP_TYPE_CURRENT, path);

	std::wstring result;
	if (SUCCEEDED(specialLocationResult))
	{
		result = path;
	}
	return result;
}


std::wstring NppParameters::getSettingsFolder()
{
	if (_isLocal)
		return _nppPath;

	std::wstring settingsFolderPath = getSpecialFolderLocation(CSIDL_APPDATA);

	if (settingsFolderPath.empty())
		return _nppPath;

	pathAppend(settingsFolderPath, L"Notepad++");
	return settingsFolderPath;
}


bool NppParameters::load()
{
	L_END = L_EXTERNAL;
	bool isAllLoaded = true;

	_isx64 = sizeof(void *) == 8;

	// Make localConf.xml path
	std::wstring localConfPath(_nppPath);
	pathAppend(localConfPath, localConfFile);

	// Test if localConf.xml exist
	_isLocal = (doesFileExist(localConfPath.c_str()) == TRUE);

	// Under vista and windows 7, the usage of doLocalConf.xml is not allowed
	// if Notepad++ is installed in "program files" directory, because of UAC
	if (_isLocal)
	{
		// We check if OS is Vista or greater version
		if (_winVersion >= WV_VISTA)
		{
			std::wstring progPath = getSpecialFolderLocation(CSIDL_PROGRAM_FILES);
			wchar_t nppDirLocation[MAX_PATH];
			wcscpy_s(nppDirLocation, _nppPath.c_str());
			::PathRemoveFileSpec(nppDirLocation);

			if  (progPath == nppDirLocation)
				_isLocal = false;
		}
	}

	_pluginRootDir = _nppPath;
	pathAppend(_pluginRootDir, L"plugins");

	//
	// the 3rd priority: general default configuration
	//
	std::wstring nppPluginRootParent;
	if (_isLocal)
	{
		_userPath = nppPluginRootParent = _nppPath;
		_userPluginConfDir = _pluginRootDir;
		pathAppend(_userPluginConfDir, L"Config");
	}
	else
	{
		_userPath = getSpecialFolderLocation(CSIDL_APPDATA);

		pathAppend(_userPath, L"Notepad++");
		if (!doesDirectoryExist(_userPath.c_str()))
			::CreateDirectory(_userPath.c_str(), NULL);

		_appdataNppDir = _userPluginConfDir = _userPath;

		pathAppend(_userPluginConfDir, L"plugins");
		if (!doesDirectoryExist(_userPluginConfDir.c_str()))
			::CreateDirectory(_userPluginConfDir.c_str(), NULL);

		pathAppend(_userPluginConfDir, L"Config");
		if (!doesDirectoryExist(_userPluginConfDir.c_str()))
			::CreateDirectory(_userPluginConfDir.c_str(), NULL);

		// For PluginAdmin to launch the wingup with UAC
		setElevationRequired(true);
	}

	_pluginConfDir = _pluginRootDir; // for plugin list home
	pathAppend(_pluginConfDir, L"Config");

	if (!doesDirectoryExist(nppPluginRootParent.c_str()))
		::CreateDirectory(nppPluginRootParent.c_str(), NULL);
	if (!doesDirectoryExist(_pluginRootDir.c_str()))
		::CreateDirectory(_pluginRootDir.c_str(), NULL);

	_sessionPath = _userPath; // Session stock the absolute file path, it should never be on cloud

	// Detection cloud settings
	std::wstring cloudChoicePath{_userPath};
	cloudChoicePath += L"\\cloud\\choice";

	//
	// the 2nd priority: cloud Choice Path
	//
	_isCloud = doesFileExist(cloudChoicePath.c_str());
	if (_isCloud)
	{
		// Read cloud choice
		std::string cloudChoiceStr = getFileContent(cloudChoicePath.c_str());
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		std::wstring cloudChoiceStrW = wmc.char2wchar(cloudChoiceStr.c_str(), SC_CP_UTF8);

		if (!cloudChoiceStrW.empty() && doesDirectoryExist(cloudChoiceStrW.c_str()))
		{
			_userPath = cloudChoiceStrW;
			_nppGUI._cloudPath = cloudChoiceStrW;
			_initialCloudChoice = _nppGUI._cloudPath;
		}
		else
		{
			_isCloud = false;
		}
	}

	//
	// the 1st priority: custom settings dir via command line argument
	//
	if (!_cmdSettingsDir.empty())
	{
		if (!doesDirectoryExist(_cmdSettingsDir.c_str()))
		{
			// The following text is not translatable.
			// _pNativeLangSpeaker is initialized AFTER _userPath being dterminated because nativeLang.xml is from from _userPath.
			std::wstring errMsg = L"The given path\r";
			errMsg += _cmdSettingsDir;
			errMsg += L"\nvia command line \"-settingsDir=\" is not a valid directory.\rThis argument will be ignored.";
			::MessageBox(NULL, errMsg.c_str(), L"Invalid directory", MB_OK);
		}
		else
		{
			_userPath = _cmdSettingsDir;
			_sessionPath = _userPath; // reset session path
		}
	}

	//--------------------------//
	// langs.xml : for per user //
	//--------------------------//
	std::wstring langs_xml_path(_userPath);
	pathAppend(langs_xml_path, L"langs.xml");

	BOOL doRecover = FALSE;
	if (doesFileExist(langs_xml_path.c_str()))
	{
		WIN32_FILE_ATTRIBUTE_DATA attributes{};
		attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
		if (GetFileAttributesEx(langs_xml_path.c_str(), GetFileExInfoStandard, &attributes) != 0)
		{
			if (attributes.nFileSizeLow == 0 && attributes.nFileSizeHigh == 0)
			{
				if (_pNativeLangSpeaker)
				{
					doRecover = _pNativeLangSpeaker->messageBox("LoadLangsFailed",
						NULL,
						L"Load langs.xml failed!\rDo you want to recover your langs.xml?",
						L"Configurator",
						MB_YESNO);
				}
				else
				{
					doRecover = ::MessageBox(NULL, L"Load langs.xml failed!\rDo you want to recover your langs.xml?", L"Configurator", MB_YESNO);
				}
			}
		}
	}
	else
		doRecover = true;

	if (doRecover)
	{
		std::wstring srcLangsPath(_nppPath);
		pathAppend(srcLangsPath, L"langs.model.xml");
		::CopyFile(srcLangsPath.c_str(), langs_xml_path.c_str(), FALSE);
	}

	_pXmlDoc = new TiXmlDocument(langs_xml_path);


	bool loadOkay = _pXmlDoc->LoadFile();
	if (!loadOkay)
	{
		if (_pNativeLangSpeaker)
		{
			_pNativeLangSpeaker->messageBox("LoadLangsFailedFinal",
				NULL,
				L"Load langs.xml failed!",
				L"Configurator",
				MB_OK);
		}
		else
		{
			::MessageBox(NULL, L"Load langs.xml failed!", L"Configurator", MB_OK);
		}

		delete _pXmlDoc;
		_pXmlDoc = nullptr;
		isAllLoaded = false;
	}
	else
		getLangKeywordsFromXmlTree();

	//---------------------------//
	// config.xml : for per user //
	//---------------------------//
	std::wstring configPath(_userPath);
	pathAppend(configPath, L"config.xml");

	std::wstring srcConfigPath(_nppPath);
	pathAppend(srcConfigPath, L"config.model.xml");

	if (!doesFileExist(configPath.c_str()))
		::CopyFile(srcConfigPath.c_str(), configPath.c_str(), FALSE);

	_pXmlUserDoc = new TiXmlDocument(configPath);
	loadOkay = _pXmlUserDoc->LoadFile();
	
	if (!loadOkay)
	{
		TiXmlDeclaration* decl = new TiXmlDeclaration(L"1.0", L"UTF-8", L"");
		_pXmlUserDoc->LinkEndChild(decl);
	}
	else
	{
		getUserParametersFromXmlTree();
	}

	//----------------------------//
	// stylers.xml : for per user //
	//----------------------------//

	_stylerPath = _userPath;
	pathAppend(_stylerPath, L"stylers.xml");

	if (!doesFileExist(_stylerPath.c_str()))
	{
		std::wstring srcStylersPath(_nppPath);
		pathAppend(srcStylersPath, L"stylers.model.xml");

		::CopyFile(srcStylersPath.c_str(), _stylerPath.c_str(), TRUE);
	}

	if (_nppGUI._themeName.empty() || (!doesFileExist(_nppGUI._themeName.c_str())))
		_nppGUI._themeName.assign(_stylerPath);

	_pXmlUserStylerDoc = new TiXmlDocument(_nppGUI._themeName.c_str());

	loadOkay = _pXmlUserStylerDoc->LoadFile();
	if (!loadOkay)
	{
		if (_pNativeLangSpeaker)
		{
			_pNativeLangSpeaker->messageBox("LoadStylersFailed",
				NULL,
				L"Load \"$STR_REPLACE$\" failed!",
				L"Load stylers.xml failed",
				MB_OK,
				0,
				_stylerPath.c_str());
		}
		else
		{
			::MessageBox(NULL, _stylerPath.c_str(), L"Load stylers.xml failed", MB_OK);
		}
		delete _pXmlUserStylerDoc;
		_pXmlUserStylerDoc = NULL;
		isAllLoaded = false;
	}
	else
		getUserStylersFromXmlTree();

	_themeSwitcher._stylesXmlPath = _stylerPath;
	// Firstly, add the default theme
	_themeSwitcher.addDefaultThemeFromXml(_stylerPath);

	//-----------------------------------//
	// userDefineLang.xml : for per user //
	//-----------------------------------//
	_userDefineLangsFolderPath = _userDefineLangPath = _userPath;
	pathAppend(_userDefineLangPath, L"userDefineLang.xml");
	pathAppend(_userDefineLangsFolderPath, L"userDefineLangs");

	std::vector<std::wstring> udlFiles;
	getFilesInFolder(udlFiles, L"*.xml", _userDefineLangsFolderPath);

	_pXmlUserLangDoc = new TiXmlDocument(_userDefineLangPath);
	loadOkay = _pXmlUserLangDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlUserLangDoc;
		_pXmlUserLangDoc = nullptr;
		isAllLoaded = false;
	}
	else
	{
		auto r = addUserDefineLangsFromXmlTree(_pXmlUserLangDoc);
		if (r.second - r.first > 0)
			_pXmlUserLangsDoc.push_back(UdlXmlFileState(_pXmlUserLangDoc, false, true, r));
	}

	for (const auto& i : udlFiles)
	{
		auto udlDoc = new TiXmlDocument(i);
		loadOkay = udlDoc->LoadFile();
		if (!loadOkay)
		{
			delete udlDoc;
		}
		else
		{
			auto r = addUserDefineLangsFromXmlTree(udlDoc);
			if (r.second - r.first > 0)
				_pXmlUserLangsDoc.push_back(UdlXmlFileState(udlDoc, false, false, r));
		}
	}

	//----------------------------------------------//
	// nativeLang.xml : for per user				//
	// In case of absence of user's nativeLang.xml, //
	// We'll look in the Notepad++ Dir.			 //
	//----------------------------------------------//

	std::wstring nativeLangPath;
	nativeLangPath = _userPath;
	pathAppend(nativeLangPath, L"nativeLang.xml");

	// LocalizationSwitcher should use always user path
	_localizationSwitcher._nativeLangPath = nativeLangPath;

	if (!_startWithLocFileName.empty()) // localization argument detected, use user wished localization
	{
		// overwrite nativeLangPath variable
		nativeLangPath = _nppPath;
		pathAppend(nativeLangPath, L"localization\\");
		pathAppend(nativeLangPath, _startWithLocFileName);
	}
	else // use %appdata% location, or (if absence then) npp installed location
	{
		if (!doesFileExist(nativeLangPath.c_str()))
		{
			nativeLangPath = _nppPath;
			pathAppend(nativeLangPath, L"nativeLang.xml");
		}
	}


	_pXmlNativeLangDocA = new TiXmlDocumentA();

	loadOkay = _pXmlNativeLangDocA->LoadUnicodeFilePath(nativeLangPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlNativeLangDocA;
		_pXmlNativeLangDocA = nullptr;
		isAllLoaded = false;
	}

	//---------------------------------//
	// toolbarIcons.xml : for per user //
	//---------------------------------//
	std::wstring toolbarIconsPath(_userPath);
	pathAppend(toolbarIconsPath, L"toolbarIcons.xml");

	_pXmlToolIconsDoc = new TiXmlDocument(toolbarIconsPath);
	loadOkay = _pXmlToolIconsDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlToolIconsDoc;
		_pXmlToolIconsDoc = nullptr;
		isAllLoaded = false;
	}

	//---------------------------------------//
	// toolbarButtonsConf.xml : for per user //
	//---------------------------------------//
	std::wstring toolbarButtonsConfXmlPath(_userPath);
	pathAppend(toolbarButtonsConfXmlPath, L"toolbarButtonsConf.xml");

	_pXmlToolButtonsConfDoc = new TiXmlDocument(toolbarButtonsConfXmlPath);
	loadOkay = _pXmlToolButtonsConfDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlToolButtonsConfDoc;
		_pXmlToolButtonsConfDoc = nullptr;
		isAllLoaded = false;
	}

	//------------------------------//
	// shortcuts.xml : for per user //
	//------------------------------//
	wstring v852NoNeedShortcutsBackup;
	_shortcutsPath = v852NoNeedShortcutsBackup = _userPath;
	pathAppend(_shortcutsPath, SHORTCUTSXML_FILENAME);
	pathAppend(v852NoNeedShortcutsBackup, NONEEDSHORTCUTSXMLBACKUP_FILENAME);

	if (!doesFileExist(_shortcutsPath.c_str()))
	{
		std::wstring srcShortcutsPath(_nppPath);
		pathAppend(srcShortcutsPath, SHORTCUTSXML_FILENAME);

		::CopyFile(srcShortcutsPath.c_str(), _shortcutsPath.c_str(), TRUE);

		// Creat empty file v852NoNeedShortcutsBackup.xml for not giving warning, neither doing backup, in future use.
		HANDLE hFile = ::CreateFile(v852NoNeedShortcutsBackup.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);
	}

	_pXmlShortcutDocA = new TiXmlDocumentA();
	loadOkay = _pXmlShortcutDocA->LoadUnicodeFilePath(_shortcutsPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlShortcutDocA;
		_pXmlShortcutDocA = nullptr;
		isAllLoaded = false;
	}
	else
	{
		getShortcutsFromXmlTree();
		getMacrosFromXmlTree();
		getUserCmdsFromXmlTree();

		// fill out _scintillaModifiedKeys :
		// those user defined Scintilla key will be used remap Scintilla Key Array
		getScintKeysFromXmlTree();
	}

	//---------------------------------//
	// contextMenu.xml : for per user //
	//---------------------------------//
	_contextMenuPath = _userPath;
	pathAppend(_contextMenuPath, L"contextMenu.xml");

	if (!doesFileExist(_contextMenuPath.c_str()))
	{
		std::wstring srcContextMenuPath(_nppPath);
		pathAppend(srcContextMenuPath, L"contextMenu.xml");

		::CopyFile(srcContextMenuPath.c_str(), _contextMenuPath.c_str(), TRUE);
	}

	_pXmlContextMenuDocA = new TiXmlDocumentA();
	loadOkay = _pXmlContextMenuDocA->LoadUnicodeFilePath(_contextMenuPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlContextMenuDocA;
		_pXmlContextMenuDocA = nullptr;
		isAllLoaded = false;
	}

	//---------------------------------------------//
	// tabContextMenu.xml : for per user, optional //
	//---------------------------------------------//
	_tabContextMenuPath = _userPath;
	pathAppend(_tabContextMenuPath, L"tabContextMenu.xml");

	_pXmlTabContextMenuDocA = new TiXmlDocumentA();
	loadOkay = _pXmlTabContextMenuDocA->LoadUnicodeFilePath(_tabContextMenuPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlTabContextMenuDocA;
		_pXmlTabContextMenuDocA = nullptr;
	}

	//----------------------------//
	// session.xml : for per user //
	//----------------------------//

	pathAppend(_sessionPath, L"session.xml");

	// Don't load session.xml if not required in order to speed up!!
	const NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
	if (nppGUI._rememberLastSession)
	{
		TiXmlDocument* pXmlSessionDoc = new TiXmlDocument(_sessionPath);

		loadOkay = pXmlSessionDoc->LoadFile();
		if (loadOkay)
		{
			loadOkay = getSessionFromXmlTree(pXmlSessionDoc, _session);
		}
		
		if (!loadOkay)
		{
			wstring sessionInCaseOfCorruption_bak = _sessionPath;
			sessionInCaseOfCorruption_bak += SESSION_BACKUP_EXT;
			if (doesFileExist(sessionInCaseOfCorruption_bak.c_str()))
			{
				BOOL bFileSwapOk = false;
				if (doesFileExist(_sessionPath.c_str()))
				{
					// an invalid session.xml file exists
					bFileSwapOk = ::ReplaceFile(_sessionPath.c_str(), sessionInCaseOfCorruption_bak.c_str(), nullptr,
						REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, 0, 0);
				}
				else
				{
					// no session.xml file
					bFileSwapOk = ::MoveFileEx(sessionInCaseOfCorruption_bak.c_str(), _sessionPath.c_str(),
						MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
				}

				if (bFileSwapOk)
				{
					TiXmlDocument* pXmlSessionBackupDoc = new TiXmlDocument(_sessionPath);
					loadOkay = pXmlSessionBackupDoc->LoadFile();
					if (loadOkay)
						loadOkay = getSessionFromXmlTree(pXmlSessionBackupDoc, _session);

					delete pXmlSessionBackupDoc;
				}

				if (!loadOkay)
					isAllLoaded = false; // either the backup file is also invalid or cannot be swapped with the session.xml
			}
			else
			{
				// no backup file
				isAllLoaded = false;
			}
		}

		delete pXmlSessionDoc;

		for (size_t i = 0, len = _pXmlExternalLexerDoc.size() ; i < len ; ++i)
			if (_pXmlExternalLexerDoc[i])
				delete _pXmlExternalLexerDoc[i];
	}

	//-------------------------------------------------------------//
	// enableSelectFgColor.xml : for per user                      //
	// This empty xml file is optional - user adds this empty file //
	// manually in order to set selected text's foreground color.  //
	//-------------------------------------------------------------//
	std::wstring enableSelectFgColorPath = _userPath;
	pathAppend(enableSelectFgColorPath, L"enableSelectFgColor.xml");

	if (doesFileExist(enableSelectFgColorPath.c_str()))
	{
		_isSelectFgColorEnabled = true;
	}


	std::wstring filePath, filePath2, issueFileName;
	//-------------------------------------------------------------//
	// nppLogNetworkDriveIssue.xml                                 //
	// This empty xml file is optional - user adds this empty file //
	// It's for debugging use only                                 //
	//-------------------------------------------------------------//
	filePath = _nppPath;
	issueFileName = nppLogNetworkDriveIssue;
	issueFileName += L".xml";
	pathAppend(filePath, issueFileName);
	_doNppLogNetworkDriveIssue = doesFileExist(filePath.c_str());
	if (!_doNppLogNetworkDriveIssue)
	{
		filePath2 = _userPath;
		pathAppend(filePath2, issueFileName);
		_doNppLogNetworkDriveIssue = doesFileExist(filePath2.c_str());
	}

	//-------------------------------------------------------------//
	// nppLogNulContentCorruptionIssue.xml                         //
	// This empty xml file is optional - user adds this empty file //
	// It's for debugging use only                                 //
	//-------------------------------------------------------------//
	filePath = _nppPath;
	issueFileName = nppLogNulContentCorruptionIssue;
	issueFileName += L".xml";
	pathAppend(filePath, issueFileName);
	_doNppLogNulContentCorruptionIssue = doesFileExist(filePath.c_str());
	if (!_doNppLogNulContentCorruptionIssue)
	{
		filePath2 = _userPath;
		pathAppend(filePath2, issueFileName);
		_doNppLogNulContentCorruptionIssue = doesFileExist(filePath2.c_str());
	}

	//-------------------------------------------------------------//
	// noRestartAutomatically.xml                                  //
	// This empty xml file is optional - user adds this empty file //
	// manually in order to prevent Notepad++ registration         //
	// for the Win10+ OS app-restart feature.                      //
	//-------------------------------------------------------------//
	filePath = _nppPath;
	std::wstring noRegForOSAppRestartTrigger = L"noRestartAutomatically.xml";
	pathAppend(filePath, noRegForOSAppRestartTrigger);
	_isRegForOSAppRestartDisabled = doesFileExist(filePath.c_str());
	if (!_isRegForOSAppRestartDisabled)
	{
		filePath = _userPath;
		pathAppend(filePath, noRegForOSAppRestartTrigger);
		_isRegForOSAppRestartDisabled = doesFileExist(filePath.c_str());
	}

	return isAllLoaded;
}


void NppParameters::destroyInstance()
{
	delete _pXmlDoc;
	delete _pXmlUserDoc;
	delete _pXmlUserStylerDoc;
	
	//delete _pXmlUserLangDoc; will be deleted in the vector
	for (const auto& l : _pXmlUserLangsDoc)
	{
		delete l._udlXmlDoc;
	}

	delete _pXmlNativeLangDocA;
	delete _pXmlToolIconsDoc;
	delete _pXmlToolButtonsConfDoc;
	delete _pXmlShortcutDocA;
	delete _pXmlContextMenuDocA;
	delete _pXmlTabContextMenuDocA;
	delete 	getInstancePointer();
}


void NppParameters::saveConfig_xml()
{
	if (_pXmlUserDoc)
		_pXmlUserDoc->SaveFile();
}


void NppParameters::setWorkSpaceFilePath(int i, const wchar_t* wsFile)
{
	if (i < 0 || i > 2 || !wsFile)
		return;
	_workSpaceFilePathes[i] = wsFile;
}


void NppParameters::removeTransparent(HWND hwnd)
{
	if (hwnd != nullptr)
		::SetWindowLongPtr(hwnd, GWL_EXSTYLE, ::GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
}


void NppParameters::SetTransparent(HWND hwnd, int percent)
{
	::SetWindowLongPtr(hwnd, GWL_EXSTYLE, ::GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	if (percent > 255)
		percent = 255;
	else if (percent < 0)
		percent = 0;
	::SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(percent), LWA_ALPHA);
}


bool NppParameters::isExistingExternalLangName(const char* newName) const
{
	if ((!newName) || (!newName[0]))
		return true;

	for (int i = 0 ; i < _nbExternalLang ; ++i)
	{
		if (_externalLangArray[i]->_name == newName)
			return true;
	}
	return false;
}


const wchar_t* NppParameters::getUserDefinedLangNameFromExt(wchar_t *ext, wchar_t *fullName) const
{
	if ((!ext) || (!ext[0]))
		return nullptr;

	std::vector<std::wstring> extVect;
	int iMatched = -1;
	for (int i = 0 ; i < _nbUserLang ; ++i)
	{
		extVect.clear();
		cutString(_userLangArray[i]->_ext.c_str(), extVect);

		// Force to use dark mode UDL in dark mode or to use  light mode UDL in light mode
		for (size_t j = 0, len = extVect.size(); j < len; ++j)
		{
			if (!wcsicmp(extVect[j].c_str(), ext) || (wcschr(fullName, '.') && !wcsicmp(extVect[j].c_str(), fullName)))
			{
				// preserve ext matched UDL
				iMatched = i;

				if (((NppDarkMode::isEnabled() && _userLangArray[i]->_isDarkModeTheme)) ||
					((!NppDarkMode::isEnabled() && !_userLangArray[i]->_isDarkModeTheme)))
					return _userLangArray[i]->_name.c_str();
			}
		}
	}

	// In case that we are in dark mode but no dark UDL or we are in light mode but no light UDL
	// We use it anyway
	if (iMatched >= 0)
	{
		return _userLangArray[iMatched]->_name.c_str();
	}

	return nullptr;
}


int NppParameters::getExternalLangIndexFromName(const wchar_t* externalLangName) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	for (int i = 0 ; i < _nbExternalLang ; ++i)
	{
		if (!lstrcmp(externalLangName, wmc.char2wchar(_externalLangArray[i]->_name.c_str(), CP_ACP)))
			return i;
	}
	return -1;
}


UserLangContainer* NppParameters::getULCFromName(const wchar_t *userLangName)
{
	for (int i = 0 ; i < _nbUserLang ; ++i)
	{
		if (0 == lstrcmp(userLangName, _userLangArray[i]->_name.c_str()))
			return _userLangArray[i];
	}

	//qui doit etre jamais passer
	return nullptr;
}


COLORREF NppParameters::getCurLineHilitingColour()
{
	const Style * pStyle = _widgetStyleArray.findByName(L"Current line background colour");
	if (!pStyle)
		return COLORREF(-1);
	return pStyle->_bgColor;
}


void NppParameters::setCurLineHilitingColour(COLORREF colour2Set)
{
	Style * pStyle = _widgetStyleArray.findByName(L"Current line background colour");
	if (!pStyle)
		return;
	pStyle->_bgColor = colour2Set;
}



static int CALLBACK EnumFontFamExProc(const LOGFONT* lpelfe, const TEXTMETRIC*, DWORD, LPARAM lParam)
{
	std::vector<std::wstring>& strVect = *(std::vector<std::wstring> *)lParam;
	const int32_t vectSize = static_cast<int32_t>(strVect.size());
	const wchar_t* lfFaceName = ((ENUMLOGFONTEX*)lpelfe)->elfLogFont.lfFaceName;

	//Search through all the fonts, EnumFontFamiliesEx never states anything about order
	//Start at the end though, that's the most likely place to find a duplicate
	for (int i = vectSize - 1 ; i >= 0 ; i--)
	{
		if (0 == lstrcmp(strVect[i].c_str(), lfFaceName))
			return 1;	//we already have seen this typeface, ignore it
	}

	//We can add the font
	//Add the face name and not the full name, we do not care about any styles
	strVect.push_back(lfFaceName);
	return 1; // I want to get all fonts
}


void NppParameters::setFontList(HWND hWnd)
{
	//---------------//
	// Sys font list //
	//---------------//
	LOGFONT lf{};
	_fontlist.clear();
	_fontlist.reserve(64); // arbitrary
	_fontlist.push_back(std::wstring());

	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfFaceName[0]='\0';
	lf.lfPitchAndFamily = 0;
	HDC hDC = ::GetDC(hWnd);
	::EnumFontFamiliesEx(hDC, &lf, EnumFontFamExProc, reinterpret_cast<LPARAM>(&_fontlist), 0);
}

bool NppParameters::isInFontList(const std::wstring& fontName2Search) const
{
	if (fontName2Search.empty())
		return false;

	for (size_t i = 0, len = _fontlist.size(); i < len; i++)
	{
		if (_fontlist[i] == fontName2Search)
			return true;
	}
	return false;
}

void NppParameters::getLangKeywordsFromXmlTree()
{
	TiXmlNode *root =
		_pXmlDoc->FirstChild(L"NotepadPlus");
		if (!root) return;
	feedKeyWordsParameters(root);
}


void NppParameters::getExternalLexerFromXmlTree(TiXmlDocument* externalLexerDoc)
{
	TiXmlNode *root = externalLexerDoc->FirstChild(L"NotepadPlus");
		if (!root) return;
	feedKeyWordsParameters(root);
	feedStylerArray(root);
}


int NppParameters::addExternalLangToEnd(ExternalLangContainer * externalLang)
{
	_externalLangArray[_nbExternalLang] = externalLang;
	++_nbExternalLang;
	++L_END;
	return _nbExternalLang-1;
}


bool NppParameters::getUserStylersFromXmlTree()
{
	TiXmlNode *root = _pXmlUserStylerDoc->FirstChild(L"NotepadPlus");
		if (!root) return false;
	return feedStylerArray(root);
}


bool NppParameters::getUserParametersFromXmlTree()
{
	if (!_pXmlUserDoc)
		return false;

	TiXmlNode *root = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!root)
		return false;

	// Get GUI parameters
	feedGUIParameters(root);

	// Get History parameters
	feedFileListParameters(root);

	// Erase the History root
	TiXmlNode *node = root->FirstChildElement(L"History");
	root->RemoveChild(node);

	// Add a new empty History root
	TiXmlElement HistoryNode(L"History");
	root->InsertEndChild(HistoryNode);

	//Get Find history parameters
	feedFindHistoryParameters(root);

	//Get Project Panel parameters
	feedProjectPanelsParameters(root);

	//Get File browser parameters
	feedFileBrowserParameters(root);

	//Get Column editor parameters
	feedColumnEditorParameters(root);

	return true;
}


std::pair<unsigned char, unsigned char> NppParameters::addUserDefineLangsFromXmlTree(TiXmlDocument *tixmldoc)
{
	if (!tixmldoc)
		return std::make_pair(static_cast<unsigned char>(0), static_cast<unsigned char>(0));

	TiXmlNode *root = tixmldoc->FirstChild(L"NotepadPlus");
	if (!root)
		return std::make_pair(static_cast<unsigned char>(0), static_cast<unsigned char>(0));

	return feedUserLang(root);
}



bool NppParameters::getShortcutsFromXmlTree()
{
	if (!_pXmlShortcutDocA)
		return false;

	TiXmlNodeA *root = _pXmlShortcutDocA->FirstChild("NotepadPlus");
	if (!root)
		return false;

	feedShortcut(root);
	return true;
}


bool NppParameters::getMacrosFromXmlTree()
{
	if (!_pXmlShortcutDocA)
		return false;

	TiXmlNodeA *root = _pXmlShortcutDocA->FirstChild("NotepadPlus");
	if (!root)
		return false;

	feedMacros(root);
	return true;
}


bool NppParameters::getUserCmdsFromXmlTree()
{
	if (!_pXmlShortcutDocA)
		return false;

	TiXmlNodeA *root = _pXmlShortcutDocA->FirstChild("NotepadPlus");
	if (!root)
		return false;

	feedUserCmds(root);
	return true;
}


bool NppParameters::getPluginCmdsFromXmlTree()
{
	if (!_pXmlShortcutDocA)
		return false;

	TiXmlNodeA *root = _pXmlShortcutDocA->FirstChild("NotepadPlus");
	if (!root)
		return false;

	feedPluginCustomizedCmds(root);
	return true;
}


bool NppParameters::getScintKeysFromXmlTree()
{
	if (!_pXmlShortcutDocA)
		return false;

	TiXmlNodeA *root = _pXmlShortcutDocA->FirstChild("NotepadPlus");
	if (!root)
		return false;

	feedScintKeys(root);
	return true;
}

void NppParameters::initMenuKeys()
{
	int nbCommands = sizeof(winKeyDefs)/sizeof(WinMenuKeyDefinition);
	WinMenuKeyDefinition wkd;
	int previousFuncID = 0;
	for (int i = 0; i < nbCommands; ++i)
	{
		wkd = winKeyDefs[i];
		Shortcut sc((wkd.specialName ? wstring2string(wkd.specialName, CP_UTF8).c_str() : ""), wkd.isCtrl, wkd.isAlt, wkd.isShift, static_cast<unsigned char>(wkd.vKey));
		_shortcuts.push_back( CommandShortcut(sc, wkd.functionId, previousFuncID == wkd.functionId) );
		previousFuncID = wkd.functionId;
	}
}

void NppParameters::initScintillaKeys()
{
	int nbCommands = sizeof(scintKeyDefs)/sizeof(ScintillaKeyDefinition);

	//Warning! Matching function have to be consecutive
	ScintillaKeyDefinition skd;
	int prevIndex = -1;
	int prevID = -1;
	for (int i = 0; i < nbCommands; ++i)
	{
		skd = scintKeyDefs[i];
		if (skd.functionId == prevID)
		{
			KeyCombo kc;
			kc._isCtrl = skd.isCtrl;
			kc._isAlt = skd.isAlt;
			kc._isShift = skd.isShift;
			kc._key = static_cast<unsigned char>(skd.vKey);
			_scintillaKeyCommands[prevIndex].addKeyCombo(kc);
		}
		else
		{
			Shortcut s = Shortcut(wstring2string(skd.name, CP_UTF8).c_str(), skd.isCtrl, skd.isAlt, skd.isShift, static_cast<unsigned char>(skd.vKey));
			ScintillaKeyMap sm = ScintillaKeyMap(s, skd.functionId, skd.redirFunctionId);
			_scintillaKeyCommands.push_back(sm);
			++prevIndex;
		}
		prevID = skd.functionId;
	}
}

bool NppParameters::reloadContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu)
{
	_contextMenuItems.clear();
	return getContextMenuFromXmlTree(mainMenuHadle, pluginsMenu);
}

int NppParameters::getCmdIdFromMenuEntryItemName(HMENU mainMenuHadle, const std::wstring& menuEntryName, const std::wstring& menuItemName)
{
	int nbMenuEntry = ::GetMenuItemCount(mainMenuHadle);
	for (int i = 0; i < nbMenuEntry; ++i)
	{
		wchar_t menuEntryString[menuItemStrLenMax];
		::GetMenuString(mainMenuHadle, i, menuEntryString, menuItemStrLenMax, MF_BYPOSITION);
		if (wcsicmp(menuEntryName.c_str(), purgeMenuItemString(menuEntryString).c_str()) == 0)
		{
			vector< pair<HMENU, int> > parentMenuPos;
			HMENU topMenu = ::GetSubMenu(mainMenuHadle, i);
			int maxTopMenuPos = ::GetMenuItemCount(topMenu);
			HMENU currMenu = topMenu;
			int currMaxMenuPos = maxTopMenuPos;

			int currMenuPos = 0;
			bool notFound = false;

			do {
				if (::GetSubMenu(currMenu, currMenuPos))
				{
					//  Go into sub menu
					parentMenuPos.push_back(::make_pair(currMenu, currMenuPos));
					currMenu = ::GetSubMenu(currMenu, currMenuPos);
					currMenuPos = 0;
					currMaxMenuPos = ::GetMenuItemCount(currMenu);
				}
				else
				{
					//  Check current menu position.
					wchar_t cmdStr[menuItemStrLenMax];
					::GetMenuString(currMenu, currMenuPos, cmdStr, menuItemStrLenMax, MF_BYPOSITION);
					if (wcsicmp(menuItemName.c_str(), purgeMenuItemString(cmdStr).c_str()) == 0)
					{
						return ::GetMenuItemID(currMenu, currMenuPos);
					}

					if ((currMenuPos >= currMaxMenuPos) && (parentMenuPos.size() > 0))
					{
						currMenu = parentMenuPos.back().first;
						currMenuPos = parentMenuPos.back().second;
						parentMenuPos.pop_back();
						currMaxMenuPos = ::GetMenuItemCount(currMenu);
					}

					if ((currMenu == topMenu) && (currMenuPos >= maxTopMenuPos))
					{
						notFound = true;
					}
					else
					{
						++currMenuPos;
					}
				}
			} while (!notFound);
		}
	}
	return -1;
}

int NppParameters::getPluginCmdIdFromMenuEntryItemName(HMENU pluginsMenu, const std::wstring& pluginName, const std::wstring& pluginCmdName)
{
	int nbPlugins = ::GetMenuItemCount(pluginsMenu);
	for (int i = 0; i < nbPlugins; ++i)
	{
		wchar_t menuItemString[menuItemStrLenMax];
		::GetMenuString(pluginsMenu, i, menuItemString, menuItemStrLenMax, MF_BYPOSITION);
		if (wcsicmp(pluginName.c_str(), purgeMenuItemString(menuItemString).c_str()) == 0)
		{
			HMENU pluginMenu = ::GetSubMenu(pluginsMenu, i);
			int nbPluginCmd = ::GetMenuItemCount(pluginMenu);
			for (int j = 0; j < nbPluginCmd; ++j)
			{
				wchar_t pluginCmdStr[menuItemStrLenMax];
				::GetMenuString(pluginMenu, j, pluginCmdStr, menuItemStrLenMax, MF_BYPOSITION);
				if (wcsicmp(pluginCmdName.c_str(), purgeMenuItemString(pluginCmdStr).c_str()) == 0)
				{
					return ::GetMenuItemID(pluginMenu, j);
				}
			}
		}
	}
	return -1;
}

bool NppParameters::getContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu, bool isEditCM)
{
	TiXmlDocumentA* pXmlContextMenuDocA = isEditCM ? _pXmlContextMenuDocA : _pXmlTabContextMenuDocA;
	std::string cmName = isEditCM ? "ScintillaContextMenu" : "TabContextMenu";

	if (!pXmlContextMenuDocA)
		return false;
	TiXmlNodeA *root = pXmlContextMenuDocA->FirstChild("NotepadPlus");
	if (!root)
		return false;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	TiXmlNodeA *contextMenuRoot = root->FirstChildElement(cmName.c_str());
	if (contextMenuRoot)
	{
		std::vector<MenuItemUnit>& contextMenuItems = isEditCM ? _contextMenuItems : _tabContextMenuItems;

		for (TiXmlNodeA *childNode = contextMenuRoot->FirstChildElement("Item");
			childNode ;
			childNode = childNode->NextSibling("Item") )
		{
			const char *folderNameDefaultA = (childNode->ToElement())->Attribute("FolderName");
			const char *folderNameTranslateID_A = (childNode->ToElement())->Attribute("TranslateID");
			const char *displayAsA = (childNode->ToElement())->Attribute("ItemNameAs");

			std::wstring folderName;
			std::wstring displayAs;
			folderName = folderNameDefaultA ? wmc.char2wchar(folderNameDefaultA, SC_CP_UTF8) : L"";
			displayAs = displayAsA ? wmc.char2wchar(displayAsA, SC_CP_UTF8) : L"";

			if (folderNameTranslateID_A)
			{
				folderName = pNativeSpeaker->getLocalizedStrFromID(folderNameTranslateID_A, folderName);
			}

			int id;
			const char *idStr = (childNode->ToElement())->Attribute("id", &id);
			if (idStr)
			{
				contextMenuItems.push_back(MenuItemUnit(id, displayAs.c_str(), folderName.c_str()));
			}
			else
			{
				const char *menuEntryNameA = (childNode->ToElement())->Attribute("MenuEntryName");
				const char *menuItemNameA = (childNode->ToElement())->Attribute("MenuItemName");

				std::wstring menuEntryName;
				std::wstring menuItemName;
				menuEntryName = menuEntryNameA?wmc.char2wchar(menuEntryNameA, SC_CP_UTF8):L"";
				menuItemName = menuItemNameA?wmc.char2wchar(menuItemNameA, SC_CP_UTF8):L"";

				if (!menuEntryName.empty() && !menuItemName.empty())
				{
					int cmd = getCmdIdFromMenuEntryItemName(mainMenuHadle, menuEntryName, menuItemName);
					if (cmd != -1)
						contextMenuItems.push_back(MenuItemUnit(cmd, displayAs.c_str(), folderName.c_str()));
				}
				else
				{
					const char *pluginNameA = (childNode->ToElement())->Attribute("PluginEntryName");
					const char *pluginCmdNameA = (childNode->ToElement())->Attribute("PluginCommandItemName");

					std::wstring pluginName;
					std::wstring pluginCmdName;
					pluginName = pluginNameA ? wmc.char2wchar(pluginNameA, SC_CP_UTF8) : L"";
					pluginCmdName = pluginCmdNameA ? wmc.char2wchar(pluginCmdNameA, SC_CP_UTF8) : L"";

					// if plugin menu existing plls the value of PluginEntryName and PluginCommandItemName are valid
					if (pluginsMenu && !pluginName.empty() && !pluginCmdName.empty())
					{
						int pluginCmdId = getPluginCmdIdFromMenuEntryItemName(pluginsMenu, pluginName, pluginCmdName);
						if (pluginCmdId != -1)
							contextMenuItems.push_back(MenuItemUnit(pluginCmdId, displayAs.c_str(), folderName.c_str()));
					}
				}
			}
		}
	}
	return true;
}


void NppParameters::setWorkingDir(const wchar_t * newPath)
{
	if (newPath && newPath[0])
	{
		_currentDirectory = newPath;
	}
	else
	{
		if (doesDirectoryExist(_nppGUI._defaultDirExp))
			_currentDirectory = _nppGUI._defaultDirExp;
		else
			_currentDirectory = _nppPath;
	}
}

bool NppParameters::loadSession(Session& session, const wchar_t* sessionFileName, const bool bSuppressErrorMsg)
{
	TiXmlDocument* pXmlSessionDocument = new TiXmlDocument(sessionFileName);
	bool loadOkay = pXmlSessionDocument->LoadFile();
	if (loadOkay)
		loadOkay = getSessionFromXmlTree(pXmlSessionDocument, session);

	if (!loadOkay && !bSuppressErrorMsg)
	{
		_pNativeLangSpeaker->messageBox("SessionFileInvalidError",
			NULL,
			L"Session file is either corrupted or not valid.",
			L"Could not Load Session",
			MB_OK);
	}

	delete pXmlSessionDocument;
	return loadOkay;
}


bool NppParameters::getSessionFromXmlTree(TiXmlDocument *pSessionDoc, Session& session)
{
	if (!pSessionDoc)
		return false;
	
	TiXmlNode *root = pSessionDoc->FirstChild(L"NotepadPlus");
	if (!root)
		return false;

	TiXmlNode *sessionRoot = root->FirstChildElement(L"Session");
	if (!sessionRoot)
		return false;

	TiXmlElement *actView = sessionRoot->ToElement();
	int index = 0;
	const wchar_t *str = actView->Attribute(L"activeView", &index);
	if (str)
	{
		session._activeView = index;
	}

	const size_t nbView = 2;
	TiXmlNode *viewRoots[nbView];
	viewRoots[0] = sessionRoot->FirstChildElement(L"mainView");
	viewRoots[1] = sessionRoot->FirstChildElement(L"subView");
	for (size_t k = 0; k < nbView; ++k)
	{
		if (viewRoots[k])
		{
			int index2 = 0;
			TiXmlElement *actIndex = viewRoots[k]->ToElement();
			str = actIndex->Attribute(L"activeIndex", &index2);
			if (str)
			{
				if (k == 0)
					session._activeMainIndex = index2;
				else // k == 1
					session._activeSubIndex = index2;
			}
			for (TiXmlNode *childNode = viewRoots[k]->FirstChildElement(L"File");
				childNode ;
				childNode = childNode->NextSibling(L"File") )
			{
				const wchar_t *fileName = (childNode->ToElement())->Attribute(L"filename");
				if (fileName)
				{
					Position position;
					const wchar_t* posStr = (childNode->ToElement())->Attribute(L"firstVisibleLine");
					if (posStr)
						position._firstVisibleLine = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(L"xOffset");
					if (posStr)
						position._xOffset = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(L"startPos");
					if (posStr)
						position._startPos = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(L"endPos");
					if (posStr)
						position._endPos = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(L"selMode");
					if (posStr)
						position._selMode = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(L"scrollWidth");
					if (posStr)
						position._scrollWidth = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(L"offset");
					if (posStr)
						position._offset = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(L"wrapCount");
					if (posStr)
						position._wrapCount = static_cast<intptr_t>(_ttoi64(posStr));

					MapPosition mapPosition;
					const wchar_t* mapPosStr = (childNode->ToElement())->Attribute(L"mapFirstVisibleDisplayLine");
					if (mapPosStr)
						mapPosition._firstVisibleDisplayLine = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(L"mapFirstVisibleDocLine");
					if (mapPosStr)
						mapPosition._firstVisibleDocLine = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(L"mapLastVisibleDocLine");
					if (mapPosStr)
						mapPosition._lastVisibleDocLine = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(L"mapNbLine");
					if (mapPosStr)
						mapPosition._nbLine = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(L"mapHigherPos");
					if (mapPosStr)
						mapPosition._higherPos = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(L"mapWidth");
					if (mapPosStr)
						mapPosition._width = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(L"mapHeight");
					if (mapPosStr)
						mapPosition._height = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(L"mapKByteInDoc");
					if (mapPosStr)
						mapPosition._KByteInDoc = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(L"mapWrapIndentMode");
					if (mapPosStr)
						mapPosition._wrapIndentMode = static_cast<intptr_t>(_ttoi64(mapPosStr));
					const wchar_t *boolStr = (childNode->ToElement())->Attribute(L"mapIsWrap");
					if (boolStr)
						mapPosition._isWrap = (lstrcmp(L"yes", boolStr) == 0);

					const wchar_t *langName;
					langName = (childNode->ToElement())->Attribute(L"lang");
					int encoding = -1;
					const wchar_t *encStr = (childNode->ToElement())->Attribute(L"encoding", &encoding);

					const wchar_t *pBackupFilePath = (childNode->ToElement())->Attribute(L"backupFilePath");
					std::wstring currentBackupFilePath = NppParameters::getInstance().getUserPath() + L"\\backup\\"; 
					if (pBackupFilePath)
					{
						std::wstring backupFilePath = pBackupFilePath;
						if (!backupFilePath.starts_with(currentBackupFilePath))
						{
							// reconstruct backupFilePath
							wchar_t* fn = PathFindFileName(pBackupFilePath);
							currentBackupFilePath += fn;
							pBackupFilePath = currentBackupFilePath.c_str();
						}
					}

					FILETIME fileModifiedTimestamp{};
					(childNode->ToElement())->Attribute(L"originalFileLastModifTimestamp", reinterpret_cast<int32_t*>(&fileModifiedTimestamp.dwLowDateTime));
					(childNode->ToElement())->Attribute(L"originalFileLastModifTimestampHigh", reinterpret_cast<int32_t*>(&fileModifiedTimestamp.dwHighDateTime));

					bool isUserReadOnly = false;
					const wchar_t *boolStrReadOnly = (childNode->ToElement())->Attribute(L"userReadOnly");
					if (boolStrReadOnly)
						isUserReadOnly = _wcsicmp(L"yes", boolStrReadOnly) == 0;

					bool isPinned = false;
					const wchar_t* boolStrPinned = (childNode->ToElement())->Attribute(L"tabPinned");
					if (boolStrPinned)
						isPinned = _wcsicmp(L"yes", boolStrPinned) == 0;

					sessionFileInfo sfi(fileName, langName, encStr ? encoding : -1, isUserReadOnly, isPinned, position, pBackupFilePath, fileModifiedTimestamp, mapPosition);

					const wchar_t* intStrTabColour = (childNode->ToElement())->Attribute(L"tabColourId");
					if (intStrTabColour)
					{
						sfi._individualTabColour = _wtoi(intStrTabColour);
					}

					const wchar_t* rtlStr = (childNode->ToElement())->Attribute(L"RTL");
					if (rtlStr)
					{
						sfi._isRTL = _wcsicmp(L"yes", rtlStr) == 0;
					}

					for (TiXmlNode *markNode = childNode->FirstChildElement(L"Mark");
						markNode;
						markNode = markNode->NextSibling(L"Mark"))
					{
						const wchar_t* lineNumberStr = (markNode->ToElement())->Attribute(L"line");
						if (lineNumberStr)
						{
							sfi._marks.push_back(static_cast<size_t>(_ttoi64(lineNumberStr)));
						}
					}

					for (TiXmlNode *foldNode = childNode->FirstChildElement(L"Fold");
						foldNode;
						foldNode = foldNode->NextSibling(L"Fold"))
					{
						const wchar_t *lineNumberStr = (foldNode->ToElement())->Attribute(L"line");
						if (lineNumberStr)
						{
							sfi._foldStates.push_back(static_cast<size_t>(_ttoi64(lineNumberStr)));
						}
					}
					if (k == 0)
						session._mainViewFiles.push_back(sfi);
					else // k == 1
						session._subViewFiles.push_back(sfi);
				}
			}
		}
	}

	// Node structure and naming corresponds to config.xml
	TiXmlNode *fileBrowserRoot = sessionRoot->FirstChildElement(L"FileBrowser");
	if (fileBrowserRoot)
	{
		const wchar_t *selectedItemPath = (fileBrowserRoot->ToElement())->Attribute(L"latestSelectedItem");
		if (selectedItemPath)
		{
			session._fileBrowserSelectedItem = selectedItemPath;
		}

		for (TiXmlNode *childNode = fileBrowserRoot->FirstChildElement(L"root");
			childNode;
			childNode = childNode->NextSibling(L"root"))
		{
			const wchar_t *fileName = (childNode->ToElement())->Attribute(L"foldername");
			if (fileName)
			{
				session._fileBrowserRoots.push_back({ fileName });
			}
		}
	}

	return true;
}

void NppParameters::feedFileListParameters(TiXmlNode *node)
{
	TiXmlNode *historyRoot = node->FirstChildElement(L"History");
	if (!historyRoot) return;

	// nbMaxFile value
	int nbMaxFile = _nbMaxRecentFile;
	const wchar_t *strVal = (historyRoot->ToElement())->Attribute(L"nbMaxFile", &nbMaxFile);
	if (strVal && (nbMaxFile >= 0) && (nbMaxFile <= NB_MAX_LRF_FILE))
		_nbMaxRecentFile = nbMaxFile;

	// customLen value
	int customLen = RECENTFILES_SHOWFULLPATH;
	strVal = (historyRoot->ToElement())->Attribute(L"customLength", &customLen);
	if (strVal)
		_recentFileCustomLength = std::min<int>(customLen, NB_MAX_LRF_CUSTOMLENGTH);

	// inSubMenu value
	strVal = (historyRoot->ToElement())->Attribute(L"inSubMenu");
	if (strVal)
		_putRecentFileInSubMenu = (lstrcmp(strVal, L"yes") == 0);

	for (TiXmlNode *childNode = historyRoot->FirstChildElement(L"File");
		childNode && (_nbRecentFile < NB_MAX_LRF_FILE);
		childNode = childNode->NextSibling(L"File") )
	{
		const wchar_t *filePath = (childNode->ToElement())->Attribute(L"filename");
		if (filePath)
		{
			_LRFileList[_nbRecentFile] = new std::wstring(filePath);
			++_nbRecentFile;
		}
	}
}

void NppParameters::feedFileBrowserParameters(TiXmlNode *node)
{
	TiXmlNode *fileBrowserRoot = node->FirstChildElement(L"FileBrowser");
	if (!fileBrowserRoot) return;

	const wchar_t *selectedItemPath = (fileBrowserRoot->ToElement())->Attribute(L"latestSelectedItem");
	if (selectedItemPath)
	{
		_fileBrowserSelectedItemPath = selectedItemPath;
	}

	for (TiXmlNode *childNode = fileBrowserRoot->FirstChildElement(L"root");
		childNode;
		childNode = childNode->NextSibling(L"root") )
	{
		const wchar_t *filePath = (childNode->ToElement())->Attribute(L"foldername");
		if (filePath)
		{
			_fileBrowserRoot.push_back(filePath);
		}
	}
}

void NppParameters::feedProjectPanelsParameters(TiXmlNode *node)
{
	TiXmlNode *projPanelRoot = node->FirstChildElement(L"ProjectPanels");
	if (!projPanelRoot) return;

	for (TiXmlNode *childNode = projPanelRoot->FirstChildElement(L"ProjectPanel");
		childNode;
		childNode = childNode->NextSibling(L"ProjectPanel") )
	{
		int index = 0;
		const wchar_t *idStr = (childNode->ToElement())->Attribute(L"id", &index);
		if (idStr && (index >= 0 && index <= 2))
		{
			const wchar_t *filePath = (childNode->ToElement())->Attribute(L"workSpaceFile");
			if (filePath)
			{
				_workSpaceFilePathes[index] = filePath;
			}
		}
	}
}

void NppParameters::feedColumnEditorParameters(TiXmlNode *node)
{
	TiXmlNode * columnEditorRoot = node->FirstChildElement(L"ColumnEditor");
	if (!columnEditorRoot) return;

	const wchar_t* strVal = (columnEditorRoot->ToElement())->Attribute(L"choice");
	if (strVal)
	{
		if (lstrcmp(strVal, L"text") == 0)
			_columnEditParam._mainChoice = activeText;
		else
			_columnEditParam._mainChoice = activeNumeric;
	}
	TiXmlNode *childNode = columnEditorRoot->FirstChildElement(L"text");
	if (!childNode) return;

	const wchar_t* content = (childNode->ToElement())->Attribute(L"content");
	if (content)
	{
		_columnEditParam._insertedTextContent = content;
	}

	childNode = columnEditorRoot->FirstChildElement(L"number");
	if (!childNode) return;

	int val;
	strVal = (childNode->ToElement())->Attribute(L"initial", &val);
	if (strVal)
		_columnEditParam._initialNum = val;

	strVal = (childNode->ToElement())->Attribute(L"increase", &val);
	if (strVal)
		_columnEditParam._increaseNum = val;

	strVal = (childNode->ToElement())->Attribute(L"repeat", &val);
	if (strVal)
		_columnEditParam._repeatNum = val;

	strVal = (childNode->ToElement())->Attribute(L"formatChoice");
	if (strVal)
	{
		if (lstrcmp(strVal, L"hex") == 0)
			_columnEditParam._formatChoice = 1;
		else if (lstrcmp(strVal, L"oct") == 0)
			_columnEditParam._formatChoice = 2;
		else if (lstrcmp(strVal, L"bin") == 0)
			_columnEditParam._formatChoice = 3;
		else // "dec"
			_columnEditParam._formatChoice = 0;
	}

	strVal = (childNode->ToElement())->Attribute(L"leadingChoice");
	if (strVal)
	{
		_columnEditParam._leadingChoice = ColumnEditorParam::noneLeading;
		if (lstrcmp(strVal, L"zeros") == 0)
		{
			_columnEditParam._leadingChoice = ColumnEditorParam::zeroLeading;
		}
		else if (lstrcmp(strVal, L"spaces") == 0)
		{
			_columnEditParam._leadingChoice = ColumnEditorParam::spaceLeading;
		}
	}
}

void NppParameters::feedFindHistoryParameters(TiXmlNode *node)
{
	TiXmlNode *findHistoryRoot = node->FirstChildElement(L"FindHistory");
	if (!findHistoryRoot) return;

	(findHistoryRoot->ToElement())->Attribute(L"nbMaxFindHistoryPath", &_findHistory._nbMaxFindHistoryPath);
	if (_findHistory._nbMaxFindHistoryPath > NB_MAX_FINDHISTORY_PATH)
	{
		_findHistory._nbMaxFindHistoryPath = NB_MAX_FINDHISTORY_PATH;
	}
	if ((_findHistory._nbMaxFindHistoryPath > 0) && (_findHistory._nbMaxFindHistoryPath <= NB_MAX_FINDHISTORY_PATH))
	{
		for (TiXmlNode *childNode = findHistoryRoot->FirstChildElement(L"Path");
			childNode && (_findHistory._findHistoryPaths.size() < NB_MAX_FINDHISTORY_PATH);
			childNode = childNode->NextSibling(L"Path") )
		{
			const wchar_t *filePath = (childNode->ToElement())->Attribute(L"name");
			if (filePath)
			{
				_findHistory._findHistoryPaths.push_back(std::wstring(filePath));
			}
		}
	}

	(findHistoryRoot->ToElement())->Attribute(L"nbMaxFindHistoryFilter", &_findHistory._nbMaxFindHistoryFilter);
	if (_findHistory._nbMaxFindHistoryFilter > NB_MAX_FINDHISTORY_FILTER)
	{
		_findHistory._nbMaxFindHistoryFilter = NB_MAX_FINDHISTORY_FILTER;
	}
	if ((_findHistory._nbMaxFindHistoryFilter > 0) && (_findHistory._nbMaxFindHistoryFilter <= NB_MAX_FINDHISTORY_FILTER))
	{
		for (TiXmlNode *childNode = findHistoryRoot->FirstChildElement(L"Filter");
			childNode && (_findHistory._findHistoryFilters.size() < NB_MAX_FINDHISTORY_FILTER);
			childNode = childNode->NextSibling(L"Filter"))
		{
			const wchar_t *fileFilter = (childNode->ToElement())->Attribute(L"name");
			if (fileFilter)
			{
				_findHistory._findHistoryFilters.push_back(std::wstring(fileFilter));
			}
		}
	}

	(findHistoryRoot->ToElement())->Attribute(L"nbMaxFindHistoryFind", &_findHistory._nbMaxFindHistoryFind);
	if (_findHistory._nbMaxFindHistoryFind > NB_MAX_FINDHISTORY_FIND)
	{
		_findHistory._nbMaxFindHistoryFind = NB_MAX_FINDHISTORY_FIND;
	}
	if ((_findHistory._nbMaxFindHistoryFind > 0) && (_findHistory._nbMaxFindHistoryFind <= NB_MAX_FINDHISTORY_FIND))
	{
		for (TiXmlNode *childNode = findHistoryRoot->FirstChildElement(L"Find");
			childNode && (_findHistory._findHistoryFinds.size() < NB_MAX_FINDHISTORY_FIND);
			childNode = childNode->NextSibling(L"Find"))
		{
			const wchar_t *fileFind = (childNode->ToElement())->Attribute(L"name");
			if (fileFind)
			{
				_findHistory._findHistoryFinds.push_back(std::wstring(fileFind));
			}
		}
	}

	(findHistoryRoot->ToElement())->Attribute(L"nbMaxFindHistoryReplace", &_findHistory._nbMaxFindHistoryReplace);
	if (_findHistory._nbMaxFindHistoryReplace > NB_MAX_FINDHISTORY_REPLACE)
	{
		_findHistory._nbMaxFindHistoryReplace = NB_MAX_FINDHISTORY_REPLACE;
	}
	if ((_findHistory._nbMaxFindHistoryReplace > 0) && (_findHistory._nbMaxFindHistoryReplace <= NB_MAX_FINDHISTORY_REPLACE))
	{
		for (TiXmlNode *childNode = findHistoryRoot->FirstChildElement(L"Replace");
			childNode && (_findHistory._findHistoryReplaces.size() < NB_MAX_FINDHISTORY_REPLACE);
			childNode = childNode->NextSibling(L"Replace"))
		{
			const wchar_t *fileReplace = (childNode->ToElement())->Attribute(L"name");
			if (fileReplace)
			{
				_findHistory._findHistoryReplaces.push_back(std::wstring(fileReplace));
			}
		}
	}

	const wchar_t *boolStr = (findHistoryRoot->ToElement())->Attribute(L"matchWord");
	if (boolStr)
		_findHistory._isMatchWord = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"matchCase");
	if (boolStr)
		_findHistory._isMatchCase = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"wrap");
	if (boolStr)
		_findHistory._isWrap = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"directionDown");
	if (boolStr)
		_findHistory._isDirectionDown = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"fifRecuisive");
	if (boolStr)
		_findHistory._isFifRecuisive = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"fifInHiddenFolder");
	if (boolStr)
		_findHistory._isFifInHiddenFolder = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"fifProjectPanel1");
	if (boolStr)
		_findHistory._isFifProjectPanel_1 = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"fifProjectPanel2");
	if (boolStr)
		_findHistory._isFifProjectPanel_2 = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"fifProjectPanel3");
	if (boolStr)
		_findHistory._isFifProjectPanel_3 = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"fifFilterFollowsDoc");
	if (boolStr)
		_findHistory._isFilterFollowDoc = (lstrcmp(L"yes", boolStr) == 0);

	int mode = 0;
	boolStr = (findHistoryRoot->ToElement())->Attribute(L"searchMode", &mode);
	if (boolStr)
		_findHistory._searchMode = (FindHistory::searchMode)mode;

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"transparencyMode", &mode);
	if (boolStr)
		_findHistory._transparencyMode = (FindHistory::transparencyMode)mode;

	(findHistoryRoot->ToElement())->Attribute(L"transparency", &_findHistory._transparency);
	if (_findHistory._transparency <= 0 || _findHistory._transparency > 200)
		_findHistory._transparency = 150;

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"dotMatchesNewline");
	if (boolStr)
		_findHistory._dotMatchesNewline = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"isSearch2ButtonsMode");
	if (boolStr)
		_findHistory._isSearch2ButtonsMode = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"regexBackward4PowerUser");
	if (boolStr)
		_findHistory._regexBackward4PowerUser = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"bookmarkLine");
	if (boolStr)
		_findHistory._isBookmarkLine = (lstrcmp(L"yes", boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(L"purge");
	if (boolStr)
		_findHistory._isPurge = (lstrcmp(L"yes", boolStr) == 0);
}

void NppParameters::feedShortcut(TiXmlNodeA *node)
{
	TiXmlNodeA *shortcutsRoot = node->FirstChildElement("InternalCommands");
	if (!shortcutsRoot) return;

	for (TiXmlNodeA *childNode = shortcutsRoot->FirstChildElement("Shortcut");
		childNode ;
		childNode = childNode->NextSibling("Shortcut"))
	{
		int id = 0;
		const char* idStr = (childNode->ToElement())->Attribute("id", &id);
		if (idStr)
		{
			//find the commandid that matches this Shortcut sc and alter it, push back its index in the modified list, if not present
			size_t len = _shortcuts.size();
			bool isFound = false;
			for (size_t i = 0; i < len && !isFound; ++i)
			{
				if (_shortcuts[i].getID() == static_cast<unsigned long>(id))
				{	//found our match
					isFound = getInternalCommandShortcuts(childNode, _shortcuts[i]);

					if (isFound)
						addUserModifiedIndex(i);
				}
			}
		}
	}
}

void NppParameters::feedMacros(TiXmlNodeA *node)
{
	TiXmlNodeA *macrosRoot = node->FirstChildElement("Macros");
	if (!macrosRoot) return;

	for (TiXmlNodeA *childNode = macrosRoot->FirstChildElement("Macro");
		childNode ;
		childNode = childNode->NextSibling("Macro"))
	{
		Shortcut sc;
		string fdnm;
		if (getShortcuts(childNode, sc, &fdnm))
		{
			Macro macro;
			getActions(childNode, macro);
			int cmdID = ID_MACRO + static_cast<int32_t>(_macros.size());
			_macros.push_back(MacroShortcut(sc, macro, cmdID));
			_macroMenuItems.push_back(MenuItemUnit(cmdID, string2wstring(sc.getName(), CP_UTF8), string2wstring(fdnm, CP_UTF8)));
		}
	}
}


void NppParameters::getActions(TiXmlNodeA *node, Macro & macro)
{
	for (TiXmlNodeA *childNode = node->FirstChildElement("Action");
		childNode ;
		childNode = childNode->NextSibling("Action") )
	{
		int type;
		const char *typeStr = (childNode->ToElement())->Attribute("type", &type);
		if ((!typeStr) || (type > 3))
			continue;

		int msg = 0;
		(childNode->ToElement())->Attribute("message", &msg);

		int wParam = 0;
		(childNode->ToElement())->Attribute("wParam", &wParam);

		int lParam = 0;
		(childNode->ToElement())->Attribute("lParam", &lParam);

		const char *sParam = (childNode->ToElement())->Attribute("sParam");
		if (!sParam)
			sParam = "";
		recordedMacroStep step(msg, wParam, lParam, sParam, type);
		if (step.isValid())
			macro.push_back(step);

	}
}

void NppParameters::feedUserCmds(TiXmlNodeA *node)
{
	TiXmlNodeA *userCmdsRoot = node->FirstChildElement("UserDefinedCommands");
	if (!userCmdsRoot) return;

	for (TiXmlNodeA *childNode = userCmdsRoot->FirstChildElement("Command");
		childNode ;
		childNode = childNode->NextSibling("Command") )
	{
		Shortcut sc;
		string fdnm;
		if (getShortcuts(childNode, sc, &fdnm))
		{
			TiXmlNodeA *aNode = childNode->FirstChild();
			if (aNode)
			{
				const char* cmdStr = aNode->Value();
				if (cmdStr)
				{
					int cmdID = ID_USER_CMD + static_cast<int32_t>(_userCommands.size());
					_userCommands.push_back(UserCommand(sc, cmdStr, cmdID));
					_runMenuItems.push_back(MenuItemUnit(cmdID, string2wstring(sc.getName(), CP_UTF8), string2wstring(fdnm, CP_UTF8)));
				}
			}
		}
	}
}

void NppParameters::feedPluginCustomizedCmds(TiXmlNodeA *node)
{
	TiXmlNodeA *pluginCustomizedCmdsRoot = node->FirstChildElement("PluginCommands");
	if (!pluginCustomizedCmdsRoot) return;

	for (TiXmlNodeA *childNode = pluginCustomizedCmdsRoot->FirstChildElement("PluginCommand");
		childNode ;
		childNode = childNode->NextSibling("PluginCommand") )
	{
		const char *moduleName = (childNode->ToElement())->Attribute("moduleName");
		if (!moduleName)
			continue;

		int internalID = -1;
		const char *internalIDStr = (childNode->ToElement())->Attribute("internalID", &internalID);

		if (!internalIDStr)
			continue;

		//Find the corresponding plugincommand and alter it, put the index in the list
		size_t len = _pluginCommands.size();
		for (size_t i = 0; i < len; ++i)
		{
			PluginCmdShortcut & pscOrig = _pluginCommands[i];
			if (!strnicmp(pscOrig.getModuleName(), moduleName, strlen(moduleName)) && pscOrig.getInternalID() == internalID)
			{
				//Found matching command
				getShortcuts(childNode, _pluginCommands[i]);
				addPluginModifiedIndex(i);
				break;
			}
		}
	}
}

void NppParameters::feedScintKeys(TiXmlNodeA *node)
{
	TiXmlNodeA *scintKeysRoot = node->FirstChildElement("ScintillaKeys");
	if (!scintKeysRoot) return;

	for (TiXmlNodeA *childNode = scintKeysRoot->FirstChildElement("ScintKey");
		childNode ;
		childNode = childNode->NextSibling("ScintKey") )
	{
		int scintKey;
		const char *keyStr = (childNode->ToElement())->Attribute("ScintID", &scintKey);
		if (!keyStr)
			continue;

		int menuID;
		keyStr = (childNode->ToElement())->Attribute("menuCmdID", &menuID);
		if (!keyStr)
			continue;

		//Find the corresponding scintillacommand and alter it, put the index in the list
		size_t len = _scintillaKeyCommands.size();
		for (int32_t i = 0; i < static_cast<int32_t>(len); ++i)
		{
			ScintillaKeyMap & skmOrig = _scintillaKeyCommands[i];
			if (skmOrig.getScintillaKeyID() == (unsigned long)scintKey && skmOrig.getMenuCmdID() == menuID)
			{
				//Found matching command
				_scintillaKeyCommands[i].clearDups();
				getShortcuts(childNode, _scintillaKeyCommands[i]);
				_scintillaKeyCommands[i].setKeyComboByIndex(0, _scintillaKeyCommands[i].getKeyCombo());
				addScintillaModifiedIndex(i);
				KeyCombo kc;
				for (TiXmlNodeA *nextNode = childNode->FirstChildElement("NextKey");
					nextNode ;
					nextNode = nextNode->NextSibling("NextKey"))
				{
					const char *str = (nextNode->ToElement())->Attribute("Ctrl");
					if (!str)
						continue;
					kc._isCtrl = (strcmp("yes", str) == 0);

					str = (nextNode->ToElement())->Attribute("Alt");
					if (!str)
						continue;
					kc._isAlt = (strcmp("yes", str) == 0);

					str = (nextNode->ToElement())->Attribute("Shift");
					if (!str)
						continue;
					kc._isShift = (strcmp("yes", str) == 0);

					int key;
					str = (nextNode->ToElement())->Attribute("Key", &key);
					if (!str)
						continue;
					kc._key = static_cast<unsigned char>(key);
					_scintillaKeyCommands[i].addKeyCombo(kc);
				}
				break;
			}
		}
	}
}

bool NppParameters::getInternalCommandShortcuts(TiXmlNodeA *node, CommandShortcut & cs, string* folderName)
{
	if (!node) return false;

	const char* name = (node->ToElement())->Attribute("name");
	if (!name)
		name = "";

	bool isCtrl = false;
	const char* isCtrlStr = (node->ToElement())->Attribute("Ctrl");
	if (isCtrlStr)
		isCtrl = (strcmp("yes", isCtrlStr) == 0);

	bool isAlt = false;
	const char* isAltStr = (node->ToElement())->Attribute("Alt");
	if (isAltStr)
		isAlt = (strcmp("yes", isAltStr) == 0);

	bool isShift = false;
	const char* isShiftStr = (node->ToElement())->Attribute("Shift");
	if (isShiftStr)
		isShift = (strcmp("yes", isShiftStr) == 0);

	int key;
	const char* keyStr = (node->ToElement())->Attribute("Key", &key);
	if (!keyStr)
		return false;

	int nth = -1; // 0 based
	const char* nthStr = (node->ToElement())->Attribute("nth", &nth);
	if (nthStr && nth == 1)
	{
		if (cs.getNth() != nth)
			return false;
	}
		
	if (folderName)
	{
		const char* fn = (node->ToElement())->Attribute("FolderName");
		*folderName = fn ? fn : "";
	}

	cs = Shortcut(name, isCtrl, isAlt, isShift, static_cast<unsigned char>(key));
	return true;
}

bool NppParameters::getShortcuts(TiXmlNodeA *node, Shortcut & sc, string* folderName)
{
	if (!node) return false;

	const char* name = (node->ToElement())->Attribute("name");
	if (!name)
		name = "";

	bool isCtrl = false;
	const char* isCtrlStr = (node->ToElement())->Attribute("Ctrl");
	if (isCtrlStr)
		isCtrl = (strcmp("yes", isCtrlStr) == 0);

	bool isAlt = false;
	const char* isAltStr = (node->ToElement())->Attribute("Alt");
	if (isAltStr)
		isAlt = (strcmp("yes", isAltStr) == 0);

	bool isShift = false;
	const char* isShiftStr = (node->ToElement())->Attribute("Shift");
	if (isShiftStr)
		isShift = (strcmp("yes", isShiftStr) == 0);

	int key;
	const char* keyStr = (node->ToElement())->Attribute("Key", &key);
	if (!keyStr)
		return false;


	if (folderName)
	{
		const char* fn = (node->ToElement())->Attribute("FolderName");
		*folderName = fn ? fn : "";
	}

	sc = Shortcut(name, isCtrl, isAlt, isShift, static_cast<unsigned char>(key));
	return true;
}


std::pair<unsigned char, unsigned char> NppParameters::feedUserLang(TiXmlNode *node)
{
	int iBegin = _nbUserLang;

	for (TiXmlNode *childNode = node->FirstChildElement(L"UserLang");
		childNode && (_nbUserLang < NB_MAX_USER_LANG);
		childNode = childNode->NextSibling(L"UserLang") )
	{
		const wchar_t* name = (childNode->ToElement())->Attribute(L"name");
		const wchar_t* ext = (childNode->ToElement())->Attribute(L"ext");
		const wchar_t* darkModeTheme = (childNode->ToElement())->Attribute(L"darkModeTheme");
		const wchar_t* udlVersion = (childNode->ToElement())->Attribute(L"udlVersion");

		if (!name || !name[0] || !ext)
		{
			// UserLang name is missing, just ignore this entry
			continue;
		}

		bool isDarkModeTheme = false;

		if (darkModeTheme && darkModeTheme[0])
		{
			isDarkModeTheme = (lstrcmp(L"yes", darkModeTheme) == 0);
		}

		try {
			_userLangArray[_nbUserLang] = new UserLangContainer(name, ext, isDarkModeTheme, udlVersion ? udlVersion : L"");

			++_nbUserLang;

			TiXmlNode *settingsRoot = childNode->FirstChildElement(L"Settings");
			if (!settingsRoot)
				throw std::runtime_error("NppParameters::feedUserLang : Settings node is missing");

			feedUserSettings(settingsRoot);

			TiXmlNode *keywordListsRoot = childNode->FirstChildElement(L"KeywordLists");
			if (!keywordListsRoot)
				throw std::runtime_error("NppParameters::feedUserLang : KeywordLists node is missing");

			feedUserKeywordList(keywordListsRoot);

			TiXmlNode *stylesRoot = childNode->FirstChildElement(L"Styles");
			if (!stylesRoot)
				throw std::runtime_error("NppParameters::feedUserLang : Styles node is missing");

			feedUserStyles(stylesRoot);

			// styles that were not read from xml file should get default values
			for (int i = 0 ; i < SCE_USER_STYLE_TOTAL_STYLES ; ++i)
			{
				const Style * pStyle = _userLangArray[_nbUserLang - 1]->_styles.findByID(i);
				if (!pStyle)
					_userLangArray[_nbUserLang - 1]->_styles.addStyler(i, globalMappper().styleNameMapper[i]);
			}

		}
		catch (const std::exception&)
		{
			delete _userLangArray[--_nbUserLang];
		}
	}
	int iEnd = _nbUserLang;
	return pair<unsigned char, unsigned char>(static_cast<unsigned char>(iBegin), static_cast<unsigned char>(iEnd));
}

bool NppParameters::importUDLFromFile(const std::wstring& sourceFile)
{
	TiXmlDocument *pXmlUserLangDoc = new TiXmlDocument(sourceFile);

	bool loadOkay = pXmlUserLangDoc->LoadFile();
	if (loadOkay)
	{
		auto r = addUserDefineLangsFromXmlTree(pXmlUserLangDoc);
		loadOkay = (r.second - r.first) != 0;
		if (loadOkay)
		{
			_pXmlUserLangsDoc.push_back(UdlXmlFileState(nullptr, true, true, r));

			// imported UDL from xml file will be added into default udl, so we should make default udl dirty
			setUdlXmlDirtyFromXmlDoc(_pXmlUserLangDoc);
		}
	}
	delete pXmlUserLangDoc;
	return loadOkay;
}

bool NppParameters::exportUDLToFile(size_t langIndex2export, const std::wstring& fileName2save)
{
	if (langIndex2export >= NB_MAX_USER_LANG)
		return false;

	if (static_cast<int32_t>(langIndex2export) >= _nbUserLang)
		return false;

	TiXmlDocument *pNewXmlUserLangDoc = new TiXmlDocument(fileName2save);
	TiXmlNode *newRoot2export = pNewXmlUserLangDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));

	insertUserLang2Tree(newRoot2export, _userLangArray[langIndex2export]);
	bool result = pNewXmlUserLangDoc->SaveFile();

	delete pNewXmlUserLangDoc;
	return result;
}

LangType NppParameters::getLangFromExt(const wchar_t *ext)
{
	// first check a user defined extensions for styles
	LexerStylerArray &lexStyleList = getLStylerArray();
	for (size_t i = 0 ; i < lexStyleList.getNbLexer(); ++i)
	{
		LexerStyler &styler = lexStyleList.getLexerFromIndex(i);
		const wchar_t *extList = styler.getLexerUserExt();

		if (isInList(ext, extList))
			return getLangIDFromStr(styler.getLexerName());
	}

	// then check languages extensions
	int i = getNbLang() - 1;
	while (i >= 0)
	{
		Lang *l = getLangFromIndex(i--);
		const wchar_t *defList = l->getDefaultExtList();

		if (defList && isInList(ext, defList))
			return l->getLangID();
	}
	return L_TEXT;
}

void NppParameters::setCloudChoice(const wchar_t *pathChoice)
{
	std::wstring cloudChoicePath = getSettingsFolder();
	cloudChoicePath += L"\\cloud\\";

	if (!doesDirectoryExist(cloudChoicePath.c_str()))
	{
		::CreateDirectory(cloudChoicePath.c_str(), NULL);
	}
	cloudChoicePath += L"choice";

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	std::string cloudPathA = wmc.wchar2char(pathChoice, SC_CP_UTF8);

	writeFileContent(cloudChoicePath.c_str(), cloudPathA.c_str());
}

void NppParameters::removeCloudChoice()
{
	std::wstring cloudChoicePath = getSettingsFolder();

	cloudChoicePath += L"\\cloud\\choice";
	if (doesFileExist(cloudChoicePath.c_str()))
	{
		::DeleteFile(cloudChoicePath.c_str());
	}
}

bool NppParameters::isCloudPathChanged() const
{
	if (_initialCloudChoice == _nppGUI._cloudPath)
		return false;
	else if (_initialCloudChoice.size() - _nppGUI._cloudPath.size() == 1)
	{
		wchar_t c = _initialCloudChoice.at(_initialCloudChoice.size()-1);
		if (c == '\\' || c == '/')
		{
			if (_initialCloudChoice.find(_nppGUI._cloudPath) == 0)
				return false;
		}
	}
	else if (_nppGUI._cloudPath.size() - _initialCloudChoice.size() == 1)
	{
		wchar_t c = _nppGUI._cloudPath.at(_nppGUI._cloudPath.size() - 1);
		if (c == '\\' || c == '/')
		{
			if (_nppGUI._cloudPath.find(_initialCloudChoice) == 0)
				return false;
		}
	}
	return true;
}

bool NppParameters::writeSettingsFilesOnCloudForThe1stTime(const std::wstring & cloudSettingsPath)
{
	bool isOK = false;

	if (cloudSettingsPath.empty())
		return false;

	// config.xml
	std::wstring cloudConfigPath = cloudSettingsPath;
	pathAppend(cloudConfigPath, L"config.xml");
	if (!doesFileExist(cloudConfigPath.c_str()) && _pXmlUserDoc)
	{
		isOK = _pXmlUserDoc->SaveFile(cloudConfigPath.c_str());
		if (!isOK)
			return false;
	}

	// stylers.xml
	std::wstring cloudStylersPath = cloudSettingsPath;
	pathAppend(cloudStylersPath, L"stylers.xml");
	if (!doesFileExist(cloudStylersPath.c_str()) && _pXmlUserStylerDoc)
	{
		isOK = _pXmlUserStylerDoc->SaveFile(cloudStylersPath.c_str());
		if (!isOK)
			return false;
	}

	// langs.xml
	std::wstring cloudLangsPath = cloudSettingsPath;
	pathAppend(cloudLangsPath, L"langs.xml");
	if (!doesFileExist(cloudLangsPath.c_str()) && _pXmlUserDoc)
	{
		isOK = _pXmlDoc->SaveFile(cloudLangsPath.c_str());
		if (!isOK)
			return false;
	}

	// userDefineLang.xml
	std::wstring cloudUserLangsPath = cloudSettingsPath;
	pathAppend(cloudUserLangsPath, L"userDefineLang.xml");
	if (!doesFileExist(cloudUserLangsPath.c_str()) && _pXmlUserLangDoc)
	{
		isOK = _pXmlUserLangDoc->SaveFile(cloudUserLangsPath.c_str());
		if (!isOK)
			return false;
	}

	// shortcuts.xml
	std::wstring cloudShortcutsPath = cloudSettingsPath;
	pathAppend(cloudShortcutsPath, SHORTCUTSXML_FILENAME);
	if (!doesFileExist(cloudShortcutsPath.c_str()) && _pXmlShortcutDocA)
	{
		isOK = _pXmlShortcutDocA->SaveUnicodeFilePath(cloudShortcutsPath.c_str());
		if (!isOK)
			return false;
	}

	// contextMenu.xml
	std::wstring cloudContextMenuPath = cloudSettingsPath;
	pathAppend(cloudContextMenuPath, L"contextMenu.xml");
	if (!doesFileExist(cloudContextMenuPath.c_str()) && _pXmlContextMenuDocA)
	{
		isOK = _pXmlContextMenuDocA->SaveUnicodeFilePath(cloudContextMenuPath.c_str());
		if (!isOK)
			return false;
	}

	// nativeLang.xml
	std::wstring cloudNativeLangPath = cloudSettingsPath;
	pathAppend(cloudNativeLangPath, L"nativeLang.xml");
	if (!doesFileExist(cloudNativeLangPath.c_str()) && _pXmlNativeLangDocA)
	{
		isOK = _pXmlNativeLangDocA->SaveUnicodeFilePath(cloudNativeLangPath.c_str());
		if (!isOK)
			return false;
	}

	return true;
}

/*
Default UDL + Created + Imported

*/
void NppParameters::writeDefaultUDL()
{
	bool firstCleanDone = false;
	std::vector<std::pair<bool, bool>> deleteState; //vector< pair<toDel, isInDefaultSharedContainer> >
	for (const auto& udl : _pXmlUserLangsDoc)
	{
		if (!_pXmlUserLangDoc)
		{
			_pXmlUserLangDoc = new TiXmlDocument(_userDefineLangPath);
			TiXmlDeclaration* decl = new TiXmlDeclaration(L"1.0", L"UTF-8", L"");
			_pXmlUserLangDoc->LinkEndChild(decl);
			_pXmlUserLangDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
		}

		bool toDelete = (udl._indexRange.second - udl._indexRange.first) == 0;
		deleteState.push_back(std::pair(toDelete, udl._isInDefaultSharedContainer));
		if ((!udl._udlXmlDoc || udl._udlXmlDoc == _pXmlUserLangDoc) && udl._isDirty && !toDelete) // new created or/and imported UDL plus _pXmlUserLangDoc (if exist)
		{
			TiXmlNode *root = _pXmlUserLangDoc->FirstChild(L"NotepadPlus");
			if (root && !firstCleanDone)
			{
				_pXmlUserLangDoc->RemoveChild(root);
				_pXmlUserLangDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
				firstCleanDone = true;
			}

			root = _pXmlUserLangDoc->FirstChild(L"NotepadPlus");

			for (int i = udl._indexRange.first; i < udl._indexRange.second; ++i)
			{
				insertUserLang2Tree(root, _userLangArray[i]);
			}
		}
	}

	bool deleteAll = true;
	for (std::pair<bool, bool> udlState : deleteState)
	{
		if (!udlState.first && udlState.second) // if not marked to be delete udl is (&&) in default shared container (ie. "userDefineLang.xml" file) 
		{
			deleteAll = false; // let's keep "userDefineLang.xml" file
			break;
		}
	}

	if (firstCleanDone) // at least one udl is for saving, the udl to be deleted are ignored
	{
		_pXmlUserLangDoc->SaveFile();
	}
	else if (deleteAll)
	{
		if (doesFileExist(_userDefineLangPath.c_str()))
		{
			::DeleteFile(_userDefineLangPath.c_str());
		}
	}
	// else nothing to change, do nothing
}

void NppParameters::writeNonDefaultUDL()
{
	for (auto& udl : _pXmlUserLangsDoc)
	{
		if (udl._isDirty && udl._udlXmlDoc != nullptr && udl._udlXmlDoc != _pXmlUserLangDoc)
		{
			if (udl._indexRange.second == udl._indexRange.first) // no more udl for this xmldoc container
			{
				// no need to save, delete file
				const wchar_t* docFilePath = udl._udlXmlDoc->Value();
				if (docFilePath && doesFileExist(docFilePath))
				{
					::DeleteFile(docFilePath);
				}
			}
			else
			{
				TiXmlNode *root = udl._udlXmlDoc->FirstChild(L"NotepadPlus");
				if (root)
				{
					udl._udlXmlDoc->RemoveChild(root);
				}

				udl._udlXmlDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));

				root = udl._udlXmlDoc->FirstChild(L"NotepadPlus");

				for (int i = udl._indexRange.first; i < udl._indexRange.second; ++i)
				{
					insertUserLang2Tree(root, _userLangArray[i]);
				}
				udl._udlXmlDoc->SaveFile();
			}
		}
	}
}

void NppParameters::writeNeed2SaveUDL()
{
	writeDefaultUDL();
	writeNonDefaultUDL();
}


void NppParameters::insertCmd(TiXmlNodeA *shortcutsRoot, const CommandShortcut & cmd)
{
	const KeyCombo & key = cmd.getKeyCombo();
	TiXmlNodeA *sc = shortcutsRoot->InsertEndChild(TiXmlElementA("Shortcut"));
	sc->ToElement()->SetAttribute("id", cmd.getID());
	sc->ToElement()->SetAttribute("Ctrl", key._isCtrl ? "yes" : "no");
	sc->ToElement()->SetAttribute("Alt", key._isAlt ? "yes" : "no");
	sc->ToElement()->SetAttribute("Shift", key._isShift ? "yes" : "no");
	sc->ToElement()->SetAttribute("Key", key._key);
	if (cmd.getNth() != 0)
		sc->ToElement()->SetAttribute("nth", cmd.getNth());
}


void NppParameters::insertMacro(TiXmlNodeA *macrosRoot, const MacroShortcut & macro, const string& folderName)
{
	const KeyCombo & key = macro.getKeyCombo();
	TiXmlNodeA *macroRoot = macrosRoot->InsertEndChild(TiXmlElementA("Macro"));
	macroRoot->ToElement()->SetAttribute("name", macro.getMenuName());
	macroRoot->ToElement()->SetAttribute("Ctrl", key._isCtrl?"yes":"no");
	macroRoot->ToElement()->SetAttribute("Alt", key._isAlt?"yes":"no");
	macroRoot->ToElement()->SetAttribute("Shift", key._isShift?"yes":"no");
	macroRoot->ToElement()->SetAttribute("Key", key._key);
	if (!folderName.empty())
	{
		macroRoot->ToElement()->SetAttribute("FolderName", folderName);
	}

	for (size_t i = 0, len = macro._macro.size(); i < len ; ++i)
	{
		TiXmlNodeA *actionNode = macroRoot->InsertEndChild(TiXmlElementA("Action"));
		const recordedMacroStep & action = macro._macro[i];
		actionNode->ToElement()->SetAttribute("type", action._macroType);
		actionNode->ToElement()->SetAttribute("message", action._message);
		actionNode->ToElement()->SetAttribute("wParam", static_cast<int>(action._wParameter));
		actionNode->ToElement()->SetAttribute("lParam", static_cast<int>(action._lParameter));
		actionNode->ToElement()->SetAttribute("sParam", action._sParameter.c_str());
	}
}


void NppParameters::insertUserCmd(TiXmlNodeA *userCmdRoot, const UserCommand & userCmd, const string& folderName)
{
	const KeyCombo & key = userCmd.getKeyCombo();
	TiXmlNodeA *cmdRoot = userCmdRoot->InsertEndChild(TiXmlElementA("Command"));
	cmdRoot->ToElement()->SetAttribute("name", userCmd.getMenuName());
	cmdRoot->ToElement()->SetAttribute("Ctrl", key._isCtrl?"yes":"no");
	cmdRoot->ToElement()->SetAttribute("Alt", key._isAlt?"yes":"no");
	cmdRoot->ToElement()->SetAttribute("Shift", key._isShift?"yes":"no");
	cmdRoot->ToElement()->SetAttribute("Key", key._key);
	cmdRoot->InsertEndChild(TiXmlTextA(userCmd._cmd.c_str()));
	if (!folderName.empty())
	{
		cmdRoot->ToElement()->SetAttribute("FolderName", folderName);
	}
}


void NppParameters::insertPluginCmd(TiXmlNodeA *pluginCmdRoot, const PluginCmdShortcut & pluginCmd)
{
	const KeyCombo & key = pluginCmd.getKeyCombo();
	TiXmlNodeA *pluginCmdNode = pluginCmdRoot->InsertEndChild(TiXmlElementA("PluginCommand"));
	pluginCmdNode->ToElement()->SetAttribute("moduleName", pluginCmd.getModuleName());
	pluginCmdNode->ToElement()->SetAttribute("internalID", pluginCmd.getInternalID());
	pluginCmdNode->ToElement()->SetAttribute("Ctrl", key._isCtrl ? "yes" : "no");
	pluginCmdNode->ToElement()->SetAttribute("Alt", key._isAlt ? "yes" : "no");
	pluginCmdNode->ToElement()->SetAttribute("Shift", key._isShift ? "yes" : "no");
	pluginCmdNode->ToElement()->SetAttribute("Key", key._key);
}


void NppParameters::insertScintKey(TiXmlNodeA *scintKeyRoot, const ScintillaKeyMap & scintKeyMap)
{
	TiXmlNodeA *keyRoot = scintKeyRoot->InsertEndChild(TiXmlElementA("ScintKey"));
	keyRoot->ToElement()->SetAttribute("ScintID", scintKeyMap.getScintillaKeyID());
	keyRoot->ToElement()->SetAttribute("menuCmdID", scintKeyMap.getMenuCmdID());

	//Add main shortcut
	KeyCombo key = scintKeyMap.getKeyComboByIndex(0);
	keyRoot->ToElement()->SetAttribute("Ctrl", key._isCtrl ? "yes" : "no");
	keyRoot->ToElement()->SetAttribute("Alt", key._isAlt ? "yes" : "no");
	keyRoot->ToElement()->SetAttribute("Shift", key._isShift ? "yes" : "no");
	keyRoot->ToElement()->SetAttribute("Key", key._key);

	//Add additional shortcuts
	size_t size = scintKeyMap.getSize();
	if (size > 1)
	{
		for (size_t i = 1; i < size; ++i)
		{
			TiXmlNodeA *keyNext = keyRoot->InsertEndChild(TiXmlElementA("NextKey"));
			key = scintKeyMap.getKeyComboByIndex(i);
			keyNext->ToElement()->SetAttribute("Ctrl", key._isCtrl ? "yes" : "no");
			keyNext->ToElement()->SetAttribute("Alt", key._isAlt ? "yes" : "no");
			keyNext->ToElement()->SetAttribute("Shift", key._isShift ? "yes" : "no");
			keyNext->ToElement()->SetAttribute("Key", key._key);
		}
	}
}


void NppParameters::writeSession(const Session & session, const wchar_t *fileName)
{
	const wchar_t *sessionPathName = fileName ? fileName : _sessionPath.c_str();

	//
	// Make sure session file is not read-only
	//
	removeReadOnlyFlagFromFileAttributes(sessionPathName);

	// 
	// Backup session file before overriting it
	//
	wchar_t backupPathName[MAX_PATH]{};
	BOOL doesBackupCopyExist = FALSE;
	if (doesFileExist(sessionPathName))
	{
		_tcscpy(backupPathName, sessionPathName);
		_tcscat(backupPathName, SESSION_BACKUP_EXT);
		
		// Make sure backup file is not read-only, if it exists
		removeReadOnlyFlagFromFileAttributes(backupPathName);
		doesBackupCopyExist = CopyFile(sessionPathName, backupPathName, FALSE);
		if (!doesBackupCopyExist && !isEndSessionCritical())
		{
			wstring errTitle = L"Session file backup error: ";
			errTitle += GetLastErrorAsString(0);
			::MessageBox(nullptr, sessionPathName, errTitle.c_str(), MB_OK);
		}
	}

	//
	// Prepare for writing
	//
	TiXmlDocument* pXmlSessionDoc = new TiXmlDocument(sessionPathName);
	TiXmlDeclaration* decl = new TiXmlDeclaration(L"1.0", L"UTF-8", L"");
	pXmlSessionDoc->LinkEndChild(decl);
	TiXmlNode *root = pXmlSessionDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));

	if (root)
	{
		TiXmlNode *sessionNode = root->InsertEndChild(TiXmlElement(L"Session"));
		(sessionNode->ToElement())->SetAttribute(L"activeView", static_cast<int32_t>(session._activeView));

		struct ViewElem {
			TiXmlNode *viewNode;
			vector<sessionFileInfo> *viewFiles;
			size_t activeIndex;
		};
		const int nbElem = 2;
		ViewElem viewElems[nbElem];
		viewElems[0].viewNode = sessionNode->InsertEndChild(TiXmlElement(L"mainView"));
		viewElems[1].viewNode = sessionNode->InsertEndChild(TiXmlElement(L"subView"));
		viewElems[0].viewFiles = (vector<sessionFileInfo> *)(&(session._mainViewFiles));
		viewElems[1].viewFiles = (vector<sessionFileInfo> *)(&(session._subViewFiles));
		viewElems[0].activeIndex = session._activeMainIndex;
		viewElems[1].activeIndex = session._activeSubIndex;

		for (size_t k = 0; k < nbElem ; ++k)
		{
			(viewElems[k].viewNode->ToElement())->SetAttribute(L"activeIndex", static_cast<int32_t>(viewElems[k].activeIndex));
			vector<sessionFileInfo> & viewSessionFiles = *(viewElems[k].viewFiles);

			for (size_t i = 0, len = viewElems[k].viewFiles->size(); i < len ; ++i)
			{
				TiXmlNode *fileNameNode = viewElems[k].viewNode->InsertEndChild(TiXmlElement(L"File"));

				wchar_t szInt64[64];

				(fileNameNode->ToElement())->SetAttribute(L"firstVisibleLine", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._firstVisibleLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"xOffset", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._xOffset), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"scrollWidth", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._scrollWidth), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"startPos", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._startPos), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"endPos", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._endPos), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"selMode", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._selMode), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"offset", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._offset), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"wrapCount", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._wrapCount), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"lang", viewSessionFiles[i]._langName.c_str());
				(fileNameNode->ToElement())->SetAttribute(L"encoding", viewSessionFiles[i]._encoding);
				(fileNameNode->ToElement())->SetAttribute(L"userReadOnly", (viewSessionFiles[i]._isUserReadOnly && !viewSessionFiles[i]._isMonitoring) ? L"yes" : L"no");
				(fileNameNode->ToElement())->SetAttribute(L"filename", viewSessionFiles[i]._fileName.c_str());
				(fileNameNode->ToElement())->SetAttribute(L"backupFilePath", viewSessionFiles[i]._backupFilePath.c_str());
				(fileNameNode->ToElement())->SetAttribute(L"originalFileLastModifTimestamp", static_cast<int32_t>(viewSessionFiles[i]._originalFileLastModifTimestamp.dwLowDateTime));
				(fileNameNode->ToElement())->SetAttribute(L"originalFileLastModifTimestampHigh", static_cast<int32_t>(viewSessionFiles[i]._originalFileLastModifTimestamp.dwHighDateTime));
				(fileNameNode->ToElement())->SetAttribute(L"tabColourId", static_cast<int32_t>(viewSessionFiles[i]._individualTabColour));
				(fileNameNode->ToElement())->SetAttribute(L"RTL", viewSessionFiles[i]._isRTL ? L"yes" : L"no");
				(fileNameNode->ToElement())->SetAttribute(L"tabPinned", viewSessionFiles[i]._isPinned ? L"yes" : L"no");

				// docMap 
				(fileNameNode->ToElement())->SetAttribute(L"mapFirstVisibleDisplayLine", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._firstVisibleDisplayLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"mapFirstVisibleDocLine", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._firstVisibleDocLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"mapLastVisibleDocLine", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._lastVisibleDocLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"mapNbLine", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._nbLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"mapHigherPos", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._higherPos), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"mapWidth", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._width), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"mapHeight", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._height), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"mapKByteInDoc", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._KByteInDoc), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(L"mapWrapIndentMode", _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._wrapIndentMode), szInt64, 10));
				fileNameNode->ToElement()->SetAttribute(L"mapIsWrap", viewSessionFiles[i]._mapPos._isWrap ? L"yes" : L"no");

				for (size_t j = 0, len = viewSessionFiles[i]._marks.size() ; j < len ; ++j)
				{
					size_t markLine = viewSessionFiles[i]._marks[j];
					TiXmlNode *markNode = fileNameNode->InsertEndChild(TiXmlElement(L"Mark"));
					markNode->ToElement()->SetAttribute(L"line", _ui64tot(static_cast<ULONGLONG>(markLine), szInt64, 10));
				}

				for (size_t j = 0, len = viewSessionFiles[i]._foldStates.size() ; j < len ; ++j)
				{
					size_t foldLine = viewSessionFiles[i]._foldStates[j];
					TiXmlNode *foldNode = fileNameNode->InsertEndChild(TiXmlElement(L"Fold"));
					foldNode->ToElement()->SetAttribute(L"line", _ui64tot(static_cast<ULONGLONG>(foldLine), szInt64, 10));
				}
			}
		}

		if (session._includeFileBrowser)
		{
			// Node structure and naming corresponds to config.xml
			TiXmlNode* fileBrowserRootNode = sessionNode->InsertEndChild(TiXmlElement(L"FileBrowser"));
			fileBrowserRootNode->ToElement()->SetAttribute(L"latestSelectedItem", session._fileBrowserSelectedItem.c_str());
			for (const auto& fbRoot : session._fileBrowserRoots)
			{
				TiXmlNode *fileNameNode = fileBrowserRootNode->InsertEndChild(TiXmlElement(L"root"));
				(fileNameNode->ToElement())->SetAttribute(L"foldername", fbRoot.c_str());
			}
		}
	}

	//
	// Write the session file
	//
	bool sessionSaveOK = pXmlSessionDoc->SaveFile();

	//
	// Double checking: prevent written session file corrupted while writting
	//
	if (sessionSaveOK)
	{
		TiXmlDocument* pXmlSessionCheck = new TiXmlDocument(sessionPathName);
		sessionSaveOK = pXmlSessionCheck->LoadFile();
		if (sessionSaveOK)
		{
			Session sessionCheck;
			sessionSaveOK = getSessionFromXmlTree(pXmlSessionCheck, sessionCheck);
		}
		delete pXmlSessionCheck;
	}
	else if (!isEndSessionCritical())
	{
		::MessageBox(nullptr, sessionPathName, L"Error of saving session XML file", MB_OK | MB_APPLMODAL | MB_ICONWARNING);
	}

	//
	// If error after double checking, restore session backup file
	//
	if (!sessionSaveOK)
	{
		if (doesBackupCopyExist) // session backup file exists, restore it
		{
			if (!isEndSessionCritical())
				::MessageBox(nullptr, backupPathName, L"Saving session error - restoring from the backup:", MB_OK | MB_APPLMODAL | MB_ICONWARNING);

			wstring sessionPathNameFail2Load = sessionPathName;
			sessionPathNameFail2Load += L".fail2Load";
			ReplaceFile(sessionPathName, backupPathName, sessionPathNameFail2Load.c_str(), REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, 0, 0);
		}
	}
	/*
	 * Keep session backup file in case of corrupted session file
	 * 
	else
	{
		if (backupPathName[0]) // session backup file not useful, delete it
		{
			::DeleteFile(backupPathName);
		}
	}
	*/

	delete pXmlSessionDoc;
}


void NppParameters::writeShortcuts()
{
	if (!_isAnyShortcutModified) return;

	if (!_pXmlShortcutDocA)
	{
		//do the treatment
		_pXmlShortcutDocA = new TiXmlDocumentA();
		TiXmlDeclarationA* decl = new TiXmlDeclarationA("1.0", "UTF-8", "");
		_pXmlShortcutDocA->LinkEndChild(decl);
	}
	else
	{
		wchar_t v852NoNeedShortcutsBackup[MAX_PATH]{};
		::wcscpy_s(v852NoNeedShortcutsBackup, _shortcutsPath.c_str());
		::PathRemoveFileSpec(v852NoNeedShortcutsBackup);
		::PathAppend(v852NoNeedShortcutsBackup, NONEEDSHORTCUTSXMLBACKUP_FILENAME);

		if (!doesFileExist(v852NoNeedShortcutsBackup))
		{
			// Creat empty file v852NoNeedShortcutsBackup.xml for not giving warning, neither doing backup, in future use.
			HANDLE hFile = ::CreateFile(v852NoNeedShortcutsBackup, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			::FlushFileBuffers(hFile);
			::CloseHandle(hFile);

			// backup shortcuts file "shortcuts.xml" to "shortcuts.xml.v8.5.2.backup"
			// if the backup file already exists, it will not be overwritten.
			wstring v852ShortcutsBackupPath = _shortcutsPath;
			v852ShortcutsBackupPath += L".v8.5.2.backup";
			::CopyFile(_shortcutsPath.c_str(), v852ShortcutsBackupPath.c_str(), TRUE);

			// Warn User about the current shortcut will be changed and it has been backup. If users' the shortcuts.xml has been corrupted
			// due to recoded macro under v8.5.2 (or previous versions) being modified by v8.5.3 (or later versions),
			// user can always go back to Notepad++ v8.5.2 and use the backup of shortcuts.xml 
			_pNativeLangSpeaker->messageBox("MacroAndRunCmdlWarning",
				nullptr,
				L"Your Macro and Run commands saved in Notepad++ v.8.5.2 (or older) may not be compatible with the current version of Notepad++.\nPlease test those commands and, if needed, re-edit them.\n\nAlternatively, you can downgrade to Notepad++ v8.5.2 and restore your previous data.\nNotepad++ will backup your old \"shortcuts.xml\" and save it as \"shortcuts.xml.v8.5.2.backup\".\nRenaming \"shortcuts.xml.v8.5.2.backup\" -> \"shortcuts.xml\", your commands should be restored and work properly.",
				L"Macro and Run Commands Compatibility",
				MB_OK | MB_APPLMODAL | MB_ICONWARNING);
		}
	}

	TiXmlNodeA *root = _pXmlShortcutDocA->FirstChild("NotepadPlus");
	if (!root)
	{
		root = _pXmlShortcutDocA->InsertEndChild(TiXmlElementA("NotepadPlus"));
	}

	TiXmlNodeA *cmdRoot = root->FirstChild("InternalCommands");
	if (cmdRoot)
		root->RemoveChild(cmdRoot);

	cmdRoot = root->InsertEndChild(TiXmlElementA("InternalCommands"));
	for (size_t i = 0, len = _customizedShortcuts.size(); i < len ; ++i)
	{
		size_t index = _customizedShortcuts[i];
		CommandShortcut csc = _shortcuts[index];
		insertCmd(cmdRoot, csc);
	}

	TiXmlNodeA *macrosRoot = root->FirstChild("Macros");
	if (macrosRoot)
		root->RemoveChild(macrosRoot);

	macrosRoot = root->InsertEndChild(TiXmlElementA("Macros"));

	for (size_t i = 0, len = _macros.size(); i < len ; ++i)
	{
		insertMacro(macrosRoot, _macros[i], wstring2string(_macroMenuItems.getItemFromIndex(i)._parentFolderName, CP_UTF8));
	}

	TiXmlNodeA *userCmdRoot = root->FirstChild("UserDefinedCommands");
	if (userCmdRoot)
		root->RemoveChild(userCmdRoot);

	userCmdRoot = root->InsertEndChild(TiXmlElementA("UserDefinedCommands"));

	for (size_t i = 0, len = _userCommands.size(); i < len ; ++i)
	{
		insertUserCmd(userCmdRoot, _userCommands[i], wstring2string(_runMenuItems.getItemFromIndex(i)._parentFolderName, CP_UTF8));
	}

	TiXmlNodeA *pluginCmdRoot = root->FirstChild("PluginCommands");
	if (pluginCmdRoot)
		root->RemoveChild(pluginCmdRoot);

	pluginCmdRoot = root->InsertEndChild(TiXmlElementA("PluginCommands"));
	for (size_t i = 0, len = _pluginCustomizedCmds.size(); i < len ; ++i)
	{
		insertPluginCmd(pluginCmdRoot, _pluginCommands[_pluginCustomizedCmds[i]]);
	}

	TiXmlNodeA *scitillaKeyRoot = root->FirstChild("ScintillaKeys");
	if (scitillaKeyRoot)
		root->RemoveChild(scitillaKeyRoot);

	scitillaKeyRoot = root->InsertEndChild(TiXmlElementA("ScintillaKeys"));
	for (size_t i = 0, len = _scintillaModifiedKeyIndices.size(); i < len ; ++i)
	{
		insertScintKey(scitillaKeyRoot, _scintillaKeyCommands[_scintillaModifiedKeyIndices[i]]);
	}
	_pXmlShortcutDocA->SaveUnicodeFilePath(_shortcutsPath.c_str());
}


int NppParameters::addUserLangToEnd(const UserLangContainer & userLang, const wchar_t *newName)
{
	if (isExistingUserLangName(newName))
		return -1;
	unsigned char iBegin = _nbUserLang;
	_userLangArray[_nbUserLang] = new UserLangContainer();
	*(_userLangArray[_nbUserLang]) = userLang;
	_userLangArray[_nbUserLang]->_name = newName;
	++_nbUserLang;
	unsigned char iEnd = _nbUserLang;

	_pXmlUserLangsDoc.push_back(UdlXmlFileState(nullptr, true, true, make_pair(iBegin, iEnd)));

	// imported UDL from xml file will be added into default udl, so we should make default udl dirty
	setUdlXmlDirtyFromXmlDoc(_pXmlUserLangDoc);

	return _nbUserLang-1;
}


void NppParameters::removeUserLang(size_t index)
{
	if (static_cast<int32_t>(index) >= _nbUserLang)
		return;
	delete _userLangArray[index];

	for (int32_t i = static_cast<int32_t>(index); i < (_nbUserLang - 1); ++i)
		_userLangArray[i] = _userLangArray[i+1];
	_nbUserLang--;

	removeIndexFromXmlUdls(index);
}


void NppParameters::feedUserSettings(TiXmlNode *settingsRoot)
{
	const wchar_t *boolStr;
	TiXmlNode *globalSettingNode = settingsRoot->FirstChildElement(L"Global");
	if (globalSettingNode)
	{
		boolStr = (globalSettingNode->ToElement())->Attribute(L"caseIgnored");
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_isCaseIgnored = (lstrcmp(L"yes", boolStr) == 0);

		boolStr = (globalSettingNode->ToElement())->Attribute(L"allowFoldOfComments");
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_allowFoldOfComments = (lstrcmp(L"yes", boolStr) == 0);

		(globalSettingNode->ToElement())->Attribute(L"forcePureLC", &_userLangArray[_nbUserLang - 1]->_forcePureLC);
		(globalSettingNode->ToElement())->Attribute(L"decimalSeparator", &_userLangArray[_nbUserLang - 1]->_decimalSeparator);

		boolStr = (globalSettingNode->ToElement())->Attribute(L"foldCompact");
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_foldCompact = (lstrcmp(L"yes", boolStr) == 0);
	}

	TiXmlNode *prefixNode = settingsRoot->FirstChildElement(L"Prefix");
	if (prefixNode)
	{
		const wchar_t *udlVersion = _userLangArray[_nbUserLang - 1]->_udlVersion.c_str();
		if (!lstrcmp(udlVersion, L"2.1") || !lstrcmp(udlVersion, L"2.0"))
		{
			for (int i = 0 ; i < SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
			{
				boolStr = (prefixNode->ToElement())->Attribute(globalMappper().keywordNameMapper[i+SCE_USER_KWLIST_KEYWORDS1]);
				if (boolStr)
					_userLangArray[_nbUserLang - 1]->_isPrefix[i] = (lstrcmp(L"yes", boolStr) == 0);
			}
		}
		else	// support for old style (pre 2.0)
		{
			wchar_t names[SCE_USER_TOTAL_KEYWORD_GROUPS][7] = {L"words1", L"words2", L"words3", L"words4"};
			for (int i = 0 ; i < 4 ; ++i)
			{
				boolStr = (prefixNode->ToElement())->Attribute(names[i]);
				if (boolStr)
					_userLangArray[_nbUserLang - 1]->_isPrefix[i] = (lstrcmp(L"yes", boolStr) == 0);
			}
		}
	}
}


void NppParameters::feedUserKeywordList(TiXmlNode *node)
{
	const wchar_t * udlVersion = _userLangArray[_nbUserLang - 1]->_udlVersion.c_str();

	for (TiXmlNode *childNode = node->FirstChildElement(L"Keywords");
		childNode ;
		childNode = childNode->NextSibling(L"Keywords"))
	{
		const wchar_t * keywordsName = (childNode->ToElement())->Attribute(L"name");
		TiXmlNode *valueNode = childNode->FirstChild();
		if (valueNode)
		{
			const wchar_t *kwl = nullptr;
			if (!lstrcmp(udlVersion, L"") && !lstrcmp(keywordsName, L"Delimiters"))	// support for old style (pre 2.0)
			{
				basic_string<wchar_t> temp;
				kwl = valueNode->Value();

				temp += L"00";	 if (kwl[0] != '0') temp += kwl[0];	 temp += L" 01";
				temp += L" 02";	if (kwl[3] != '0') temp += kwl[3];
				temp += L" 03";	if (kwl[1] != '0') temp += kwl[1];	 temp += L" 04";
				temp += L" 05";	if (kwl[4] != '0') temp += kwl[4];
				temp += L" 06";	if (kwl[2] != '0') temp += kwl[2];	 temp += L" 07";
				temp += L" 08";	if (kwl[5] != '0') temp += kwl[5];

				temp += L" 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23";
				wcscpy_s(_userLangArray[_nbUserLang - 1]->_keywordLists[SCE_USER_KWLIST_DELIMITERS], temp.c_str());
			}
			else if (!lstrcmp(keywordsName, L"Comment"))
			{
				kwl = valueNode->Value();
				basic_string<wchar_t> temp{L" "};

				temp += kwl;
				size_t pos = 0;

				pos = temp.find(L" 0");
				while (pos != string::npos)
				{
					temp.replace(pos, 2, L" 00");
					pos = temp.find(L" 0", pos+1);
				}
				pos = temp.find(L" 1");
				while (pos != string::npos)
				{
					temp.replace(pos, 2, L" 03");
					pos = temp.find(L" 1");
				}
				pos = temp.find(L" 2");
				while (pos != string::npos)
				{
					temp.replace(pos, 2, L" 04");
					pos = temp.find(L" 2");
				}

				temp += L" 01 02";
				if (temp[0] == ' ')
					temp.erase(0, 1);

				wcscpy_s(_userLangArray[_nbUserLang - 1]->_keywordLists[SCE_USER_KWLIST_COMMENTS], temp.c_str());
			}
			else
			{
				kwl = valueNode->Value();
				if (globalMappper().keywordIdMapper.find(keywordsName) != globalMappper().keywordIdMapper.end())
				{
					int id = globalMappper().keywordIdMapper[keywordsName];
					if (wcslen(kwl) < max_char)
					{
						wcscpy_s(_userLangArray[_nbUserLang - 1]->_keywordLists[id], kwl);
					}
					else
					{
						wcscpy_s(_userLangArray[_nbUserLang - 1]->_keywordLists[id], L"imported string too long, needs to be < max_char(30720)");
					}
				}
			}
		}
	}
}

void NppParameters::feedUserStyles(TiXmlNode *node)
{
	for (TiXmlNode *childNode = node->FirstChildElement(L"WordsStyle");
		childNode ;
		childNode = childNode->NextSibling(L"WordsStyle"))
	{
		const wchar_t *styleName = (childNode->ToElement())->Attribute(L"name");
		if (styleName)
		{
			if (globalMappper().styleIdMapper.find(styleName) != globalMappper().styleIdMapper.end())
			{
				int id = globalMappper().styleIdMapper[styleName];
				_userLangArray[_nbUserLang - 1]->_styles.addStyler((id | L_USER << 16), childNode);
			}
		}
	}
}

bool NppParameters::feedStylerArray(TiXmlNode *node)
{
	TiXmlNode *styleRoot = node->FirstChildElement(L"LexerStyles");
	if (!styleRoot) return false;

	// For each lexer
	for (TiXmlNode *childNode = styleRoot->FirstChildElement(L"LexerType");
		 childNode ;
		 childNode = childNode->NextSibling(L"LexerType") )
	{
		TiXmlElement *element = childNode->ToElement();
		const wchar_t *lexerName = element->Attribute(L"name");
		const wchar_t *lexerDesc = element->Attribute(L"desc");
		const wchar_t *lexerUserExt = element->Attribute(L"ext");
		const wchar_t *lexerExcluded = element->Attribute(L"excluded");
		if (lexerName)
		{
			_lexerStylerVect.addLexerStyler(lexerName, lexerDesc, lexerUserExt, childNode);
			if (lexerExcluded != NULL && (lstrcmp(lexerExcluded, L"yes") == 0))
			{
				int index = getExternalLangIndexFromName(lexerName);
				if (index != -1)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)(index + L_EXTERNAL)));
			}
		}
	}

	_lexerStylerVect.sort();

	// The global styles for all lexers
	TiXmlNode *globalStyleRoot = node->FirstChildElement(L"GlobalStyles");
	if (!globalStyleRoot) return false;

	for (TiXmlNode *childNode = globalStyleRoot->FirstChildElement(L"WidgetStyle");
		 childNode ;
		 childNode = childNode->NextSibling(L"WidgetStyle") )
	{
		TiXmlElement *element = childNode->ToElement();
		const wchar_t *styleIDStr = element->Attribute(L"styleID");

		int styleID = -1;
		if ((styleID = decStrVal(styleIDStr)) != -1)
		{
			_widgetStyleArray.addStyler(styleID, childNode);
		}
	}

	constexpr auto rgbhex = [](COLORREF bbggrr) -> int {
		return
			((bbggrr & 0xFF0000) >> 16) |
			((bbggrr & 0x00FF00)) |
			((bbggrr & 0x0000FF) << 16);
	};

	auto addStyle = [&](const std::wstring& name,
		const std::wstring& fgColor = L"",
		const std::wstring& bgColor = L"",
		const std::wstring& fromStyle = L"",
		const std::wstring& styleID = L"0") -> int
		{
			int result = 0;
			const Style* pStyle = _widgetStyleArray.findByName(name);
			if (pStyle == nullptr)
			{
				TiXmlNode* newStyle = globalStyleRoot->InsertEndChild(TiXmlElement(L"WidgetStyle"));
				newStyle->ToElement()->SetAttribute(L"name", name);
				newStyle->ToElement()->SetAttribute(L"styleID", styleID);

				const Style* pStyleFrom = fromStyle.empty() ? nullptr : _widgetStyleArray.findByName(fromStyle);
				if (pStyleFrom != nullptr)
				{
					constexpr size_t bufSize = 7;
					if (!fgColor.empty())
					{
						wchar_t strColor[bufSize] = { '\0' };
						swprintf(strColor, bufSize, L"%6X", rgbhex(pStyleFrom->_fgColor));
						newStyle->ToElement()->SetAttribute(L"fgColor", strColor);
					}

					if (!bgColor.empty())
					{
						wchar_t strColor[bufSize] = { '\0' };
						swprintf(strColor, bufSize, L"%6X", rgbhex(pStyleFrom->_bgColor));
						newStyle->ToElement()->SetAttribute(L"bgColor", strColor);
					}

					result = 2;
				}
				else
				{
					if (!fgColor.empty())
					{
						newStyle->ToElement()->SetAttribute(L"fgColor", fgColor);
					}

					if (!bgColor.empty())
					{
						newStyle->ToElement()->SetAttribute(L"bgColor", bgColor);
					}

					result = 1;
				}


				if (!fgColor.empty() || !bgColor.empty())
				{
					_widgetStyleArray.addStyler(0, newStyle);
					return result;
				}
				return -1;
			}
			return result;
		};

	// check void ScintillaEditView::performGlobalStyles() for default colors

	addStyle(L"Multi-selected text color", L"", L"C0C0C0", L"Selected text colour"); // liteGrey
	addStyle(L"Multi-edit carets color", L"404040", L"", L"Caret colour"); // darkGrey

	addStyle(L"Change History modified", L"FF8000", L"FF8000");
	addStyle(L"Change History revert modified", L"A0C000", L"A0C000");
	addStyle(L"Change History revert origin", L"40A0BF", L"40A0BF");
	addStyle(L"Change History saved", L"00A000", L"00A000");

	addStyle(L"EOL custom color", L"DADADA");
	addStyle(g_npcStyleName, L"DADADA", L"", L"White space symbol");

	return true;
}

void LexerStylerArray::addLexerStyler(const wchar_t *lexerName, const wchar_t *lexerDesc, const wchar_t *lexerUserExt , TiXmlNode *lexerNode)
{
	_lexerStylerVect.emplace_back();
	LexerStyler & ls = _lexerStylerVect.back();
	ls.setLexerName(lexerName);
	if (lexerDesc)
		ls.setLexerDesc(lexerDesc);

	if (lexerUserExt)
		ls.setLexerUserExt(lexerUserExt);

	for (TiXmlNode *childNode = lexerNode->FirstChildElement(L"WordsStyle");
		 childNode ;
		 childNode = childNode->NextSibling(L"WordsStyle") )
	{
		TiXmlElement *element = childNode->ToElement();
		const wchar_t *styleIDStr = element->Attribute(L"styleID");

		if (styleIDStr)
		{
			int styleID = -1;
			if ((styleID = decStrVal(styleIDStr)) != -1)
			{
				ls.addStyler(styleID, childNode);
			}
		}
	}
}

void StyleArray::addStyler(int styleID, TiXmlNode *styleNode)
{
	bool isUser = styleID >> 16 == L_USER;
	if (isUser)
	{
		styleID = (styleID & 0xFFFF);
		if (styleID >= SCE_USER_STYLE_TOTAL_STYLES || findByID(styleID))
			return;
	}

	_styleVect.emplace_back();
	Style & s = _styleVect.back();
	s._styleID = styleID;

	if (styleNode)
	{
		TiXmlElement *element = styleNode->ToElement();

		// TODO: translate to English
		// Pour _fgColor, _bgColor :
		// RGB() | (result & 0xFF000000) c'est pour le cas de -1 (0xFFFFFFFF)
		// retourné par hexStrVal(str)
		const wchar_t *str = element->Attribute(L"name");
		if (str)
		{
			if (isUser)
				s._styleDesc = globalMappper().styleNameMapper[styleID];
			else
				s._styleDesc = str;
		}

		str = element->Attribute(L"fgColor");
		if (str)
		{
			unsigned long result = hexStrVal(str);
			s._fgColor = (RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000);

		}

		str = element->Attribute(L"bgColor");
		if (str)
		{
			unsigned long result = hexStrVal(str);
			s._bgColor = (RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000);
		}

		str = element->Attribute(L"colorStyle");
		if (str)
		{
			s._colorStyle = decStrVal(str);
		}

		str = element->Attribute(L"fontName");
		if (str)
		{
			s._fontName = str;
			s._isFontEnabled = true;
		}

		str = element->Attribute(L"fontStyle");
		if (str)
		{
			s._fontStyle = decStrVal(str);
		}

		str = element->Attribute(L"fontSize");
		if (str)
		{
			s._fontSize = decStrVal(str);
		}
		str = element->Attribute(L"nesting");

		if (str)
		{
			s._nesting = decStrVal(str);
		}

		str = element->Attribute(L"keywordClass");
		if (str)
		{
			s._keywordClass = getKwClassFromName(str);
		}

		TiXmlNode *v = styleNode->FirstChild();
		if (v)
		{
			s._keywords = v->Value();
		}
	}
}

bool NppParameters::writeRecentFileHistorySettings(int nbMaxFile) const
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
	}

	TiXmlNode *historyNode = nppRoot->FirstChildElement(L"History");
	if (!historyNode)
	{
		historyNode = nppRoot->InsertEndChild(TiXmlElement(L"History"));
	}

	(historyNode->ToElement())->SetAttribute(L"nbMaxFile", nbMaxFile!=-1?nbMaxFile:_nbMaxRecentFile);
	(historyNode->ToElement())->SetAttribute(L"inSubMenu", _putRecentFileInSubMenu ? L"yes" : L"no");
	(historyNode->ToElement())->SetAttribute(L"customLength", _recentFileCustomLength);
	return true;
}

bool NppParameters::writeColumnEditorSettings() const
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
	}

	TiXmlNode *oldColumnEditorNode = nppRoot->FirstChildElement(L"ColumnEditor");
	if (oldColumnEditorNode)
	{
		// Erase the Project Panel root
		nppRoot->RemoveChild(oldColumnEditorNode);
	}

	// Create the new ColumnEditor root
	TiXmlElement columnEditorRootNode{L"ColumnEditor"};
	(columnEditorRootNode.ToElement())->SetAttribute(L"choice", _columnEditParam._mainChoice == activeNumeric ? L"number" : L"text");

	TiXmlElement textNode{ L"text" };
	(textNode.ToElement())->SetAttribute(L"content", _columnEditParam._insertedTextContent.c_str());
	(columnEditorRootNode.ToElement())->InsertEndChild(textNode);

	TiXmlElement numberNode{ L"number" };
	(numberNode.ToElement())->SetAttribute(L"initial", _columnEditParam._initialNum);
	(numberNode.ToElement())->SetAttribute(L"increase", _columnEditParam._increaseNum);
	(numberNode.ToElement())->SetAttribute(L"repeat", _columnEditParam._repeatNum);
	wstring format = L"dec";
	if (_columnEditParam._formatChoice == 1)
		format = L"hex";
	else if (_columnEditParam._formatChoice == 2)
		format = L"oct";
	else if (_columnEditParam._formatChoice == 3)
		format = L"bin";
	(numberNode.ToElement())->SetAttribute(L"formatChoice", format);
	wstring leading = L"none";
	if (_columnEditParam._leadingChoice == ColumnEditorParam::zeroLeading)
		leading = L"zeros";
	else if (_columnEditParam._leadingChoice == ColumnEditorParam::spaceLeading)
		leading = L"spaces";
	(numberNode.ToElement())->SetAttribute(L"leadingChoice", leading);
	(columnEditorRootNode.ToElement())->InsertEndChild(numberNode);

	// (Re)Insert the Project Panel root
	(nppRoot->ToElement())->InsertEndChild(columnEditorRootNode);
	return true;
}

bool NppParameters::writeProjectPanelsSettings() const
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
	}

	TiXmlNode *oldProjPanelRootNode = nppRoot->FirstChildElement(L"ProjectPanels");
	if (oldProjPanelRootNode)
	{
		// Erase the Project Panel root
		nppRoot->RemoveChild(oldProjPanelRootNode);
	}

	// Create the Project Panel root
	TiXmlElement projPanelRootNode{L"ProjectPanels"};

	// Add 3 Project Panel parameters
	for (int32_t i = 0 ; i < 3 ; ++i)
	{
		TiXmlElement projPanelNode{L"ProjectPanel"};
		(projPanelNode.ToElement())->SetAttribute(L"id", i);
		(projPanelNode.ToElement())->SetAttribute(L"workSpaceFile", _workSpaceFilePathes[i]);

		(projPanelRootNode.ToElement())->InsertEndChild(projPanelNode);
	}

	// (Re)Insert the Project Panel root
	(nppRoot->ToElement())->InsertEndChild(projPanelRootNode);
	return true;
}

bool NppParameters::writeFileBrowserSettings(const vector<std::wstring> & rootPaths, const std::wstring & latestSelectedItemPath) const
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
	}

	TiXmlNode *oldFileBrowserRootNode = nppRoot->FirstChildElement(L"FileBrowser");
	if (oldFileBrowserRootNode)
	{
		// Erase the file broser root
		nppRoot->RemoveChild(oldFileBrowserRootNode);
	}

	// Create the file browser root
	TiXmlElement fileBrowserRootNode{ L"FileBrowser" };

	if (rootPaths.size() != 0)
	{
		fileBrowserRootNode.SetAttribute(L"latestSelectedItem", latestSelectedItemPath.c_str());

		// add roots
		size_t len = rootPaths.size();
		for (size_t i = 0; i < len; ++i)
		{
			TiXmlElement fbRootNode{ L"root" };
			(fbRootNode.ToElement())->SetAttribute(L"foldername", rootPaths[i].c_str());

			(fileBrowserRootNode.ToElement())->InsertEndChild(fbRootNode);
		}
	}

	// (Re)Insert the file browser root
	(nppRoot->ToElement())->InsertEndChild(fileBrowserRootNode);
	return true;
}

bool NppParameters::writeHistory(const wchar_t *fullpath)
{
	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
	}

	TiXmlNode *historyNode = nppRoot->FirstChildElement(L"History");
	if (!historyNode)
	{
		historyNode = nppRoot->InsertEndChild(TiXmlElement(L"History"));
	}

	TiXmlElement recentFileNode(L"File");
	(recentFileNode.ToElement())->SetAttribute(L"filename", fullpath);

	(historyNode->ToElement())->InsertEndChild(recentFileNode);
	return true;
}

TiXmlNode * NppParameters::getChildElementByAttribut(TiXmlNode *pere, const wchar_t *childName,\
			const wchar_t *attributName, const wchar_t *attributVal) const
{
	for (TiXmlNode *childNode = pere->FirstChildElement(childName);
		childNode ;
		childNode = childNode->NextSibling(childName))
	{
		TiXmlElement *element = childNode->ToElement();
		const wchar_t *val = element->Attribute(attributName);
		if (val)
		{
			if (!lstrcmp(val, attributVal))
				return childNode;
		}
	}
	return NULL;
}

// 2 restes : L_H, L_USER
LangType NppParameters::getLangIDFromStr(const wchar_t *langName)
{
	int lang = static_cast<int32_t>(L_TEXT);
	for (; lang < L_EXTERNAL; ++lang)
	{
		const wchar_t * name = ScintillaEditView::_langNameInfoArray[lang]._langName;
		if (!lstrcmp(name, langName)) //found lang?
		{
			return (LangType)lang;
		}
	}

	//Cannot find language, check if its an external one

	LangType l = (LangType)lang;
	if (l == L_EXTERNAL) //try find external lexer
	{
		int id = NppParameters::getInstance().getExternalLangIndexFromName(langName);
		if (id != -1) return (LangType)(id + L_EXTERNAL);
	}

	return L_TEXT;
}

std::wstring NppParameters::getLocPathFromStr(const std::wstring & localizationCode)
{
	if (localizationCode == L"en" || localizationCode == L"en-au" || localizationCode == L"en-bz" || localizationCode == L"en-ca" || localizationCode == L"en-cb" || localizationCode == L"en-gb" || localizationCode == L"en-ie" || localizationCode == L"en-jm" || localizationCode == L"en-nz" || localizationCode == L"en-ph" || localizationCode == L"en-tt" || localizationCode == L"en-us" || localizationCode == L"en-za" || localizationCode == L"en-zw")
		return L"english.xml";
	if (localizationCode == L"af")
		return L"afrikaans.xml";
	if (localizationCode == L"sq")
		return L"albanian.xml";
	if (localizationCode == L"ar" || localizationCode == L"ar-dz" || localizationCode == L"ar-bh" || localizationCode == L"ar-eg" ||localizationCode == L"ar-iq" || localizationCode == L"ar-jo" || localizationCode == L"ar-kw" || localizationCode == L"ar-lb" || localizationCode == L"ar-ly" || localizationCode == L"ar-ma" || localizationCode == L"ar-om" || localizationCode == L"ar-qa" || localizationCode == L"ar-sa" || localizationCode == L"ar-sy" || localizationCode == L"ar-tn" || localizationCode == L"ar-ae" || localizationCode == L"ar-ye")
		return L"arabic.xml";
	if (localizationCode == L"an")
		return L"aragonese.xml";
	if (localizationCode == L"az")
		return L"azerbaijani.xml";
	if (localizationCode == L"eu")
		return L"basque.xml";
	if (localizationCode == L"be")
		return L"belarusian.xml";
	if (localizationCode == L"bn")
		return L"bengali.xml";
	if (localizationCode == L"bs")
		return L"bosnian.xml";
	if (localizationCode == L"pt-br")
		return L"brazilian_portuguese.xml";
	if (localizationCode == L"br-fr")
		return L"breton.xml";
	if (localizationCode == L"bg")
		return L"bulgarian.xml";
	if (localizationCode == L"ca")
		return L"catalan.xml";
	if (localizationCode == L"zh-tw" || localizationCode == L"zh-hk" || localizationCode == L"zh-sg")
		return L"taiwaneseMandarin.xml";
	if (localizationCode == L"zh" || localizationCode == L"zh-cn")
		return L"chineseSimplified.xml";
	if (localizationCode == L"co" || localizationCode == L"co-fr")
		return L"corsican.xml";
	if (localizationCode == L"hr")
		return L"croatian.xml";
	if (localizationCode == L"cs")
		return L"czech.xml";
	if (localizationCode == L"da")
		return L"danish.xml";
	if (localizationCode == L"nl" || localizationCode == L"nl-be")
		return L"dutch.xml";
	if (localizationCode == L"eo")
		return L"esperanto.xml";
	if (localizationCode == L"et")
		return L"estonian.xml";
	if (localizationCode == L"fa")
		return L"farsi.xml";
	if (localizationCode == L"fi")
		return L"finnish.xml";
	if (localizationCode == L"fr" || localizationCode == L"fr-be" || localizationCode == L"fr-ca" || localizationCode == L"fr-fr" || localizationCode == L"fr-lu" || localizationCode == L"fr-mc" || localizationCode == L"fr-ch")
		return L"french.xml";
	if (localizationCode == L"fur")
		return L"friulian.xml";
	if (localizationCode == L"gl")
		return L"galician.xml";
	if (localizationCode == L"ka")
		return L"georgian.xml";
	if (localizationCode == L"de" || localizationCode == L"de-at" || localizationCode == L"de-de" || localizationCode == L"de-li" || localizationCode == L"de-lu" || localizationCode == L"de-ch")
		return L"german.xml";
	if (localizationCode == L"el")
		return L"greek.xml";
	if (localizationCode == L"gu")
		return L"gujarati.xml";
	if (localizationCode == L"he")
		return L"hebrew.xml";
	if (localizationCode == L"hi")
		return L"hindi.xml";
	if (localizationCode == L"hu")
		return L"hungarian.xml";
	if (localizationCode == L"id")
		return L"indonesian.xml";
	if (localizationCode == L"it" || localizationCode == L"it-ch")
		return L"italian.xml";
	if (localizationCode == L"ja")
		return L"japanese.xml";
	if (localizationCode == L"kn")
		return L"kannada.xml";
	if (localizationCode == L"kk")
		return L"kazakh.xml";
	if (localizationCode == L"ko" || localizationCode == L"ko-kp" || localizationCode == L"ko-kr")
		return L"korean.xml";
	if (localizationCode == L"ku")
		return L"kurdish.xml";
	if (localizationCode == L"ky")
		return L"kyrgyz.xml";
	if (localizationCode == L"lv")
		return L"latvian.xml";
	if (localizationCode == L"lt")
		return L"lithuanian.xml";
	if (localizationCode == L"lb")
		return L"luxembourgish.xml";
	if (localizationCode == L"mk")
		return L"macedonian.xml";
	if (localizationCode == L"ms")
		return L"malay.xml";
	if (localizationCode == L"mr")
		return L"marathi.xml";
	if (localizationCode == L"mn")
		return L"mongolian.xml";
	if (localizationCode == L"no" || localizationCode == L"nb")
		return L"norwegian.xml";
	if (localizationCode == L"nn")
		return L"nynorsk.xml";
	if (localizationCode == L"oc")
		return L"occitan.xml";
	if (localizationCode == L"pl")
		return L"polish.xml";
	if (localizationCode == L"pt" || localizationCode == L"pt-pt")
		return L"portuguese.xml";
	if (localizationCode == L"pa" || localizationCode == L"pa-in")
		return L"punjabi.xml";
	if (localizationCode == L"ro" || localizationCode == L"ro-mo")
		return L"romanian.xml";
	if (localizationCode == L"ru" || localizationCode == L"ru-mo")
		return L"russian.xml";
	if (localizationCode == L"sc")
		return L"sardinian.xml";
	if (localizationCode == L"sr")
		return L"serbian.xml";
	if (localizationCode == L"sr-cyrl-ba" || localizationCode == L"sr-cyrl-sp")
		return L"serbianCyrillic.xml";
	if (localizationCode == L"si")
		return L"sinhala.xml";
	if (localizationCode == L"sk")
		return L"slovak.xml";
	if (localizationCode == L"sl")
		return L"slovenian.xml";
	if (localizationCode == L"es" || localizationCode == L"es-bo" || localizationCode == L"es-cl" || localizationCode == L"es-co" || localizationCode == L"es-cr" || localizationCode == L"es-do" || localizationCode == L"es-ec" || localizationCode == L"es-sv" || localizationCode == L"es-gt" || localizationCode == L"es-hn" || localizationCode == L"es-mx" || localizationCode == L"es-ni" || localizationCode == L"es-pa" || localizationCode == L"es-py" || localizationCode == L"es-pe" || localizationCode == L"es-pr" || localizationCode == L"es-es" || localizationCode == L"es-uy" || localizationCode == L"es-ve")
		return L"spanish.xml";
	if (localizationCode == L"es-ar")
		return L"spanish_ar.xml";
	if (localizationCode == L"sv")
		return L"swedish.xml";
	if (localizationCode == L"tl")
		return L"tagalog.xml";
	if (localizationCode == L"tg-cyrl-tj")
		return L"tajikCyrillic.xml";
	if (localizationCode == L"ta")
		return L"tamil.xml";
	if (localizationCode == L"tt")
		return L"tatar.xml";
	if (localizationCode == L"te")
		return L"telugu.xml";
	if (localizationCode == L"th")
		return L"thai.xml";
	if (localizationCode == L"tr")
		return L"turkish.xml";
	if (localizationCode == L"uk")
		return L"ukrainian.xml";
	if (localizationCode == L"ur" || localizationCode == L"ur-pk")
		return L"urdu.xml";
	if (localizationCode == L"ug-cn")
		return L"uyghur.xml";
	if (localizationCode == L"uz")
		return L"uzbek.xml";
	if (localizationCode == L"uz-cyrl-uz")
		return L"uzbekCyrillic.xml";
	if (localizationCode == L"vec")
		return L"venetian.xml";
	if (localizationCode == L"vi" || localizationCode == L"vi-vn")
		return L"vietnamese.xml";
	if (localizationCode == L"cy-gb")
		return L"welsh.xml";
	if (localizationCode == L"zu" || localizationCode == L"zu-za")
		return L"zulu.xml";
	if (localizationCode == L"ne" || localizationCode == L"nep")
		return L"nepali.xml";
	if (localizationCode == L"oc-aranes")
		return L"aranese.xml";
	if (localizationCode == L"exy")
		return L"extremaduran.xml";
	if (localizationCode == L"kab")
		return L"kabyle.xml";
	if (localizationCode == L"lij")
		return L"ligurian.xml";
	if (localizationCode == L"ga")
		return L"irish.xml";
	if (localizationCode == L"sgs")
		return L"samogitian.xml";
	if (localizationCode == L"yue")
		return L"hongKongCantonese.xml";
	if (localizationCode == L"ab" || localizationCode == L"abk")
		return L"abkhazian.xml";

	return std::wstring();
}


void NppParameters::feedKeyWordsParameters(TiXmlNode *node)
{
	TiXmlNode *langRoot = node->FirstChildElement(L"Languages");
	if (!langRoot)
		return;

	for (TiXmlNode *langNode = langRoot->FirstChildElement(L"Language");
		langNode ;
		langNode = langNode->NextSibling(L"Language") )
	{
		if (_nbLang < NB_LANG)
		{
			TiXmlElement* element = langNode->ToElement();
			const wchar_t* name = element->Attribute(L"name");
			if (name)
			{
				_langList[_nbLang] = new Lang(getLangIDFromStr(name), name);
				_langList[_nbLang]->setDefaultExtList(element->Attribute(L"ext"));
				_langList[_nbLang]->setCommentLineSymbol(element->Attribute(L"commentLine"));
				_langList[_nbLang]->setCommentStart(element->Attribute(L"commentStart"));
				_langList[_nbLang]->setCommentEnd(element->Attribute(L"commentEnd"));

				int tabSettings;
				const wchar_t* tsVal = element->Attribute(L"tabSettings", &tabSettings);
				const wchar_t* buVal = element->Attribute(L"backspaceUnindent");
				_langList[_nbLang]->setTabInfo(tsVal ? tabSettings : -1, buVal && !lstrcmp(buVal, L"yes"));

				for (TiXmlNode *kwNode = langNode->FirstChildElement(L"Keywords");
					kwNode ;
					kwNode = kwNode->NextSibling(L"Keywords") )
				{
					const wchar_t *indexName = (kwNode->ToElement())->Attribute(L"name");
					TiXmlNode *kwVal = kwNode->FirstChild();
					const wchar_t *keyWords = L"";
					if ((indexName) && (kwVal))
						keyWords = kwVal->Value();

					int i = getKwClassFromName(indexName);

					if (i >= 0 && i <= KEYWORDSET_MAX)
						_langList[_nbLang]->setWords(keyWords, i);
				}
				++_nbLang;
			}
		}
	}
}

extern "C" {
typedef DWORD (WINAPI * EESFUNC) (LPCTSTR, LPTSTR, DWORD);
}

void NppParameters::feedGUIParameters(TiXmlNode *node)
{
	TiXmlNode *GUIRoot = node->FirstChildElement(L"GUIConfigs");
	if (nullptr == GUIRoot)
		return;

	for (TiXmlNode *childNode = GUIRoot->FirstChildElement(L"GUIConfig");
		childNode ;
		childNode = childNode->NextSibling(L"GUIConfig") )
	{
		TiXmlElement* element = childNode->ToElement();
		const wchar_t* nm = element->Attribute(L"name");
		if (nullptr == nm)
			continue;

		auto parseYesNoBoolAttribute = [&element](const wchar_t* name, bool defaultValue = false) -> bool {
			const wchar_t* val = element->Attribute(name);
			if (val != nullptr)
			{
				if (!lstrcmp(val, L"yes"))
					return true;
				else if (!lstrcmp(val, L"no"))
					return false;
			}
			return defaultValue;
		};

		if (!lstrcmp(nm, L"ToolBar"))
		{
			const wchar_t* val = element->Attribute(L"visible");
			if (val)
			{
				if (!lstrcmp(val, L"no"))
					_nppGUI._toolbarShow = false;
				else// if (!lstrcmp(val, L"yes"))
					_nppGUI._toolbarShow = true;
			}
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, L"small"))
						_nppGUI._toolBarStatus = TB_SMALL;
					else if (!lstrcmp(val, L"large"))
						_nppGUI._toolBarStatus = TB_LARGE;
					else if (!lstrcmp(val, L"small2"))
						_nppGUI._toolBarStatus = TB_SMALL2;
					else if (!lstrcmp(val, L"large2"))
						_nppGUI._toolBarStatus = TB_LARGE2;
					else //if (!lstrcmp(val, L"standard"))
						_nppGUI._toolBarStatus = TB_STANDARD;
				}
			}
		}
		else if (!lstrcmp(nm, L"StatusBar"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, L"hide"))
						_nppGUI._statusBarShow = false;
					else if (!lstrcmp(val, L"show"))
						_nppGUI._statusBarShow = true;
				}
			}
		}
		else if (!lstrcmp(nm, L"MenuBar"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, L"hide"))
						_nppGUI._menuBarShow = false;
					else if (!lstrcmp(val, L"show"))
						_nppGUI._menuBarShow = true;
				}
			}
		}
		else if (!lstrcmp(nm, L"TabBar"))
		{
			bool isFailed = false;
			int oldValue = _nppGUI._tabStatus;
			_nppGUI._tabStatus = 0;

			const wchar_t* val = element->Attribute(L"dragAndDrop");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_DRAGNDROP;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"drawTopBar");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_DRAWTOPBAR;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"drawInactiveTab");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_DRAWINACTIVETAB;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"reduce");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_REDUCE;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"closeButton");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_CLOSEBUTTON;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"pinButton");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_PINBUTTON;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}
			else
			{
				_nppGUI._tabStatus |= TAB_PINBUTTON;
			}

			val = element->Attribute(L"buttonsOninactiveTabs");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_INACTIVETABSHOWBUTTON;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"doubleClick2Close");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_DBCLK2CLOSE;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}
			val = element->Attribute(L"vertical");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_VERTICAL;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"multiLine");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_MULTILINE;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"hide");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_HIDE;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"quitOnEmpty");
			if (val)
			{
				if (!lstrcmp(val, L"yes"))
					_nppGUI._tabStatus |= TAB_QUITONEMPTY;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(L"iconSetNumber");
			if (val)
			{
				if (!lstrcmp(val, L"1"))
					_nppGUI._tabStatus |= TAB_ALTICONS;
				else if (!lstrcmp(val, L"0"))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			if (isFailed)
				_nppGUI._tabStatus = oldValue;
		}
		else if (!lstrcmp(nm, L"Auto-detection"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, L"yesOld"))
						_nppGUI._fileAutoDetection = cdEnabledOld;
					else if (!lstrcmp(val, L"autoOld"))
						_nppGUI._fileAutoDetection = (cdEnabledOld | cdAutoUpdate);
					else if (!lstrcmp(val, L"Update2EndOld"))
						_nppGUI._fileAutoDetection = (cdEnabledOld | cdGo2end);
					else if (!lstrcmp(val, L"autoUpdate2EndOld"))
						_nppGUI._fileAutoDetection = (cdEnabledOld | cdAutoUpdate | cdGo2end);
					else if (!lstrcmp(val, L"yes"))
						_nppGUI._fileAutoDetection = cdEnabledNew;
					else if (!lstrcmp(val, L"auto"))
						_nppGUI._fileAutoDetection = (cdEnabledNew | cdAutoUpdate);
					else if (!lstrcmp(val, L"Update2End"))
						_nppGUI._fileAutoDetection = (cdEnabledNew | cdGo2end);
					else if (!lstrcmp(val, L"autoUpdate2End"))
						_nppGUI._fileAutoDetection = (cdEnabledNew | cdAutoUpdate | cdGo2end);
					else //(!lstrcmp(val, L"no"))
						_nppGUI._fileAutoDetection = cdDisabled;
				}
			}
		}

		else if (!lstrcmp(nm, L"TrayIcon"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, L"no") == 0 || lstrcmp(val, L"0") == 0)
						_nppGUI._isMinimizedToTray = sta_none;
					else if (lstrcmp(val, L"yes") == 0|| lstrcmp(val, L"1") == 0)
						_nppGUI._isMinimizedToTray = sta_minimize;
					else if (lstrcmp(val, L"2") == 0)
						_nppGUI._isMinimizedToTray = sta_close;
					else if (lstrcmp(val, L"3") == 0)
						_nppGUI._isMinimizedToTray = sta_minimize_close;
				}
			}
		}
		else if (!lstrcmp(nm, L"RememberLastSession"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._rememberLastSession = true;
					else
						_nppGUI._rememberLastSession = false;
				}
			}
		}
		else if (!lstrcmp(nm, L"KeepSessionAbsentFileEntries"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._keepSessionAbsentFileEntries = true;
					else
						_nppGUI._keepSessionAbsentFileEntries = false;
				}
			}
		}
		else if (!lstrcmp(nm, L"DetectEncoding"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._detectEncoding = true;
					else
						_nppGUI._detectEncoding = false;
				}
			}
		}
		else if (!lstrcmp(nm, L"SaveAllConfirm"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._saveAllConfirm = true;
					else
						_nppGUI._saveAllConfirm = false;
				}
			}
		}
		else if (lstrcmp(nm, L"MaintainIndent") == 0 || 
			lstrcmp(nm, L"MaitainIndent") == 0) // typo - kept for the compatibility reason
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					// the retro-compatibility with the old values
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._maintainIndent = autoIndent_advanced;
					else if (lstrcmp(val, L"no") == 0)
						_nppGUI._maintainIndent = autoIndent_none;

					// the treatment of the new values
					else if (lstrcmp(val, L"0") == 0)
						_nppGUI._maintainIndent = autoIndent_none;
					else if (lstrcmp(val, L"1") == 0)
						_nppGUI._maintainIndent = autoIndent_advanced;
					else if (lstrcmp(val, L"2") == 0)
						_nppGUI._maintainIndent = autoIndent_basic;
					else // other values will be ignored - use the default value
						_nppGUI._maintainIndent = autoIndent_advanced;
				}
			}
		}
		// <GUIConfig name="MarkAll" matchCase="yes" wholeWordOnly="yes" </GUIConfig>
		else if (!lstrcmp(nm, L"MarkAll"))
		{
			const wchar_t* val = element->Attribute(L"matchCase");
			if (val)
			{
				if (lstrcmp(val, L"yes") == 0)
					_nppGUI._markAllCaseSensitive = true;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._markAllCaseSensitive = false;
			}

			val = element->Attribute(L"wholeWordOnly");
			if (val)
			{
				if (lstrcmp(val, L"yes") == 0)
					_nppGUI._markAllWordOnly = true;
				else if (!lstrcmp(val, L"no"))
					_nppGUI._markAllWordOnly = false;
			}
		}
		// <GUIConfig name="SmartHighLight" matchCase="yes" wholeWordOnly="yes" useFindSettings="no">yes</GUIConfig>
		else if (!lstrcmp(nm, L"SmartHighLight"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._enableSmartHilite = true;
					else
						_nppGUI._enableSmartHilite = false;
				}

				val = element->Attribute(L"matchCase");
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._smartHiliteCaseSensitive = true;
					else if (!lstrcmp(val, L"no"))
						_nppGUI._smartHiliteCaseSensitive = false;
				}

				val = element->Attribute(L"wholeWordOnly");
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._smartHiliteWordOnly = true;
					else if (!lstrcmp(val, L"no"))
						_nppGUI._smartHiliteWordOnly = false;
				}

				val = element->Attribute(L"useFindSettings");
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._smartHiliteUseFindSettings = true;
					else if (!lstrcmp(val, L"no"))
						_nppGUI._smartHiliteUseFindSettings = false;
				}

				val = element->Attribute(L"onAnotherView");
				if (val)
				{
					if (lstrcmp(val, L"yes") == 0)
						_nppGUI._smartHiliteOnAnotherView = true;
					else if (!lstrcmp(val, L"no"))
						_nppGUI._smartHiliteOnAnotherView = false;
				}
			}
		}

		else if (!lstrcmp(nm, L"TagsMatchHighLight"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					_nppGUI._enableTagsMatchHilite = !lstrcmp(val, L"yes");
					const wchar_t *tahl = element->Attribute(L"TagAttrHighLight");
					if (tahl)
						_nppGUI._enableTagAttrsHilite = !lstrcmp(tahl, L"yes");

					tahl = element->Attribute(L"HighLightNonHtmlZone");
					if (tahl)
						_nppGUI._enableHiliteNonHTMLZone = !lstrcmp(tahl, L"yes");
				}
			}
		}

		else if (!lstrcmp(nm, L"TaskList"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					_nppGUI._doTaskList = (!lstrcmp(val, L"yes"))?true:false;
				}
			}
		}

		else if (!lstrcmp(nm, L"MRU"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
					_nppGUI._styleMRU = (!lstrcmp(val, L"yes"));
			}
		}

		else if (!lstrcmp(nm, L"URL"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					int const i = _wtoi (val);
					if ((i >= urlMin) && (i <= urlMax))
						_nppGUI._styleURL = urlMode(i);
				}
			}
		}

		else if (!lstrcmp(nm, L"uriCustomizedSchemes"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				_nppGUI._uriSchemes = val;
			}
		}

		else if (!lstrcmp(nm, L"CheckHistoryFiles"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, L"no"))
						_nppGUI._checkHistoryFiles = false;
					else if (!lstrcmp(val, L"yes"))
						_nppGUI._checkHistoryFiles = true;
				}
			}
		}

		else if (!lstrcmp(nm, L"ScintillaViewsSplitter"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, L"vertical"))
						_nppGUI._splitterPos = POS_VERTICAL;
					else if (!lstrcmp(val, L"horizontal"))
						_nppGUI._splitterPos = POS_HORIZOTAL;
				}
			}
		}

		else if (!lstrcmp(nm, L"UserDefineDlg"))
		{
			bool isFailed = false;
			int oldValue = _nppGUI._userDefineDlgStatus;

			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, L"hide"))
						_nppGUI._userDefineDlgStatus = 0;
					else if (!lstrcmp(val, L"show"))
						_nppGUI._userDefineDlgStatus = UDD_SHOW;
					else
						isFailed = true;
				}
			}

			const wchar_t* val = element->Attribute(L"position");
			if (val)
			{
				if (!lstrcmp(val, L"docked"))
					_nppGUI._userDefineDlgStatus |= UDD_DOCKED;
				else if (!lstrcmp(val, L"undocked"))
					_nppGUI._userDefineDlgStatus |= 0;
				else
					isFailed = true;
			}
			if (isFailed)
				_nppGUI._userDefineDlgStatus = oldValue;
		}

		else if (!lstrcmp(nm, L"TabSetting"))
		{
			int i;
			const wchar_t* val = element->Attribute(L"size", &i);
			if (val)
				_nppGUI._tabSize = i;

			if ((_nppGUI._tabSize == -1) || (_nppGUI._tabSize == 0))
				_nppGUI._tabSize = 4;

			val = element->Attribute(L"replaceBySpace");
			if (val)
				_nppGUI._tabReplacedBySpace = (!lstrcmp(val, L"yes"));

			val = element->Attribute(L"backspaceUnindent");
			if (val)
				_nppGUI._backspaceUnindent = (!lstrcmp(val, L"yes"));
		}

		else if (!lstrcmp(nm, L"Caret"))
		{
			int i;
			const wchar_t* val = element->Attribute(L"width", &i);
			if (val)
				_nppGUI._caretWidth = i;

			val = element->Attribute(L"blinkRate", &i);
			if (val)
				_nppGUI._caretBlinkRate = i;
		}

		else if (!lstrcmp(nm, L"AppPosition"))
		{
			RECT oldRect = _nppGUI._appPos;
			bool fuckUp = true;
			int i;

			if (element->Attribute(L"x", &i))
			{
				_nppGUI._appPos.left = i;

				if (element->Attribute(L"y", &i))
				{
					_nppGUI._appPos.top = i;

					if (element->Attribute(L"width", &i))
					{
						_nppGUI._appPos.right = i;

						if (element->Attribute(L"height", &i))
						{
							_nppGUI._appPos.bottom = i;
							fuckUp = false;
						}
					}
				}
			}
			if (fuckUp)
				_nppGUI._appPos = oldRect;

			const wchar_t* val = element->Attribute(L"isMaximized");
			if (val)
				_nppGUI._isMaximized = (lstrcmp(val, L"yes") == 0);
		}

		else if (!lstrcmp(nm, L"FindWindowPosition"))
		{
			RECT oldRect = _nppGUI._findWindowPos;
			bool incomplete = true;
			int i;

			if (element->Attribute(L"left", &i))
			{
				_nppGUI._findWindowPos.left = i;

				if (element->Attribute(L"top", &i))
				{
					_nppGUI._findWindowPos.top = i;

					if (element->Attribute(L"right", &i))
					{
						_nppGUI._findWindowPos.right = i;

						if (element->Attribute(L"bottom", &i))
						{
							_nppGUI._findWindowPos.bottom = i;
							incomplete = false;
						}
					}
				}
			}
			if (incomplete)
			{
				_nppGUI._findWindowPos = oldRect;
			}

			const wchar_t* val = element->Attribute(L"isLessModeOn");
			if (val)
				_nppGUI._findWindowLessMode = (lstrcmp(val, L"yes") == 0);
		}

		else if (!lstrcmp(nm, L"FinderConfig"))
		{
			const wchar_t* val = element->Attribute(L"wrappedLines");
			if (val)
			{
				_nppGUI._finderLinesAreCurrentlyWrapped = (!lstrcmp(val, L"yes"));
			}

			val = element->Attribute(L"purgeBeforeEverySearch");
			if (val)
			{
				_nppGUI._finderPurgeBeforeEverySearch = (!lstrcmp(val, L"yes"));
			}

			val = element->Attribute(L"showOnlyOneEntryPerFoundLine");
			if (val)
			{
				_nppGUI._finderShowOnlyOneEntryPerFoundLine = (!lstrcmp(val, L"yes"));
			}
		}

		else if (!lstrcmp(nm, L"NewDocDefaultSettings"))
		{
			int i;
			if (element->Attribute(L"format", &i))
			{
				EolType newFormat = EolType::osdefault;
				switch (i)
				{
					case static_cast<LPARAM>(EolType::windows) :
						newFormat = EolType::windows;
						break;
					case static_cast<LPARAM>(EolType::macos) :
						newFormat = EolType::macos;
						break;
					case static_cast<LPARAM>(EolType::unix) :
						newFormat = EolType::unix;
						break;
					default:
						assert(false and "invalid buffer format - fallback to default");
				}
				_nppGUI._newDocDefaultSettings._format = newFormat;
			}

			if (element->Attribute(L"encoding", &i))
				_nppGUI._newDocDefaultSettings._unicodeMode = (UniMode)i;

			if (element->Attribute(L"lang", &i))
				_nppGUI._newDocDefaultSettings._lang = (LangType)i;

			if (element->Attribute(L"codepage", &i))
				_nppGUI._newDocDefaultSettings._codepage = (LangType)i;

			const wchar_t* val = element->Attribute(L"openAnsiAsUTF8");
			if (val)
				_nppGUI._newDocDefaultSettings._openAnsiAsUtf8 = (lstrcmp(val, L"yes") == 0);

			val = element->Attribute(L"addNewDocumentOnStartup");
			if (val)
				_nppGUI._newDocDefaultSettings._addNewDocumentOnStartup = (lstrcmp(val, L"yes") == 0);
		}

		else if (!lstrcmp(nm, L"langsExcluded"))
		{
			// TODO
			int g0 = 0; // up to 8
			int g1 = 0; // up to 16
			int g2 = 0; // up to 24
			int g3 = 0; // up to 32
			int g4 = 0; // up to 40
			int g5 = 0; // up to 48
			int g6 = 0; // up to 56
			int g7 = 0; // up to 64
			int g8 = 0; // up to 72
			int g9 = 0; // up to 80
			int g10= 0; // up to 88
			int g11= 0; // up to 96
			int g12= 0; // up to 104

			// TODO some refactoring needed here....
			{
				int i;
				if (element->Attribute(L"gr0", &i))
				{
					if (i <= 255)
						g0 = i;
				}
				if (element->Attribute(L"gr1", &i))
				{
					if (i <= 255)
						g1 = i;
				}
				if (element->Attribute(L"gr2", &i))
				{
					if (i <= 255)
						g2 = i;
				}
				if (element->Attribute(L"gr3", &i))
				{
					if (i <= 255)
						g3 = i;
				}
				if (element->Attribute(L"gr4", &i))
				{
					if (i <= 255)
						g4 = i;
				}
				if (element->Attribute(L"gr5", &i))
				{
					if (i <= 255)
						g5 = i;
				}
				if (element->Attribute(L"gr6", &i))
				{
					if (i <= 255)
						g6 = i;
				}
				if (element->Attribute(L"gr7", &i))
				{
					if (i <= 255)
						g7 = i;
				}
				if (element->Attribute(L"gr8", &i))
				{
					if (i <= 255)
						g8 = i;
				}
				if (element->Attribute(L"gr9", &i))
				{
					if (i <= 255)
						g9 = i;
				}
				if (element->Attribute(L"gr10", &i))
				{
					if (i <= 255)
						g10 = i;
				}
				if (element->Attribute(L"gr11", &i))
				{
					if (i <= 255)
						g11 = i;
				}
				if (element->Attribute(L"gr12", &i))
				{
					if (i <= 255)
						g12 = i;
				}
			}

			UCHAR mask = 1;
			for (int i = 0 ; i < 8 ; ++i)
			{
				if (mask & g0)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 8 ; i < 16 ; ++i)
			{
				if (mask & g1)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 16 ; i < 24 ; ++i)
			{
				if (mask & g2)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 24 ; i < 32 ; ++i)
			{
				if (mask & g3)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 32 ; i < 40 ; ++i)
			{
				if (mask & g4)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 40 ; i < 48 ; ++i)
			{
				if (mask & g5)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 48 ; i < 56 ; ++i)
			{
				if (mask & g6)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 56 ; i < 64 ; ++i)
			{
				if (mask & g7)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 64; i < 72; ++i)
			{
				if (mask & g8)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 72; i < 80; ++i)
			{
				if (mask & g9)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 80; i < 88; ++i)
			{
				if (mask & g10)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 88; i < 96; ++i)
			{
				if (mask & g11)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 96; i < 104; ++i)
			{
				if (mask & g12)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			const wchar_t* val = element->Attribute(L"langMenuCompact");
			if (val)
				_nppGUI._isLangMenuCompact = (!lstrcmp(val, L"yes"));
		}

		else if (!lstrcmp(nm, L"Print"))
		{
			const wchar_t* val = element->Attribute(L"lineNumber");
			if (val)
				_nppGUI._printSettings._printLineNumber = (!lstrcmp(val, L"yes"));

			int i;
			if (element->Attribute(L"printOption", &i))
				_nppGUI._printSettings._printOption = i;

			val = element->Attribute(L"headerLeft");
			if (val)
				_nppGUI._printSettings._headerLeft = val;

			val = element->Attribute(L"headerMiddle");
			if (val)
				_nppGUI._printSettings._headerMiddle = val;

			val = element->Attribute(L"headerRight");
			if (val)
				_nppGUI._printSettings._headerRight = val;


			val = element->Attribute(L"footerLeft");
			if (val)
				_nppGUI._printSettings._footerLeft = val;

			val = element->Attribute(L"footerMiddle");
			if (val)
				_nppGUI._printSettings._footerMiddle = val;

			val = element->Attribute(L"footerRight");
			if (val)
				_nppGUI._printSettings._footerRight = val;


			val = element->Attribute(L"headerFontName");
			if (val)
				_nppGUI._printSettings._headerFontName = val;

			val = element->Attribute(L"footerFontName");
			if (val)
				_nppGUI._printSettings._footerFontName = val;

			if (element->Attribute(L"headerFontStyle", &i))
				_nppGUI._printSettings._headerFontStyle = i;

			if (element->Attribute(L"footerFontStyle", &i))
				_nppGUI._printSettings._footerFontStyle = i;

			if (element->Attribute(L"headerFontSize", &i))
				_nppGUI._printSettings._headerFontSize = i;

			if (element->Attribute(L"footerFontSize", &i))
				_nppGUI._printSettings._footerFontSize = i;


			if (element->Attribute(L"margeLeft", &i))
				_nppGUI._printSettings._marge.left = i;

			if (element->Attribute(L"margeTop", &i))
				_nppGUI._printSettings._marge.top = i;

			if (element->Attribute(L"margeRight", &i))
				_nppGUI._printSettings._marge.right = i;

			if (element->Attribute(L"margeBottom", &i))
				_nppGUI._printSettings._marge.bottom = i;
		}

		else if (!lstrcmp(nm, L"ScintillaPrimaryView"))
		{
			feedScintillaParam(element);
		}

		else if (!lstrcmp(nm, L"Backup"))
		{
			int i;
			if (element->Attribute(L"action", &i))
				_nppGUI._backup = (BackupFeature)i;

			const wchar_t *bDir = element->Attribute(L"useCustumDir");
			if (bDir)
			{
				_nppGUI._useDir = (lstrcmp(bDir, L"yes") == 0);
			}
			const wchar_t *pDir = element->Attribute(L"dir");
			if (pDir)
				_nppGUI._backupDir = pDir;

			const wchar_t *isSnapshotModeStr = element->Attribute(L"isSnapshotMode");
			if (isSnapshotModeStr && !lstrcmp(isSnapshotModeStr, L"no"))
				_nppGUI._isSnapshotMode = false;

			int timing;
			if (element->Attribute(L"snapshotBackupTiming", &timing))
				_nppGUI._snapshotBackupTiming = timing;

		}
		else if (!lstrcmp(nm, L"DockingManager"))
		{
			feedDockingManager(element);
		}

		else if (!lstrcmp(nm, L"globalOverride"))
		{
			const wchar_t *bDir = element->Attribute(L"fg");
			if (bDir)
				_nppGUI._globalOverride.enableFg = (lstrcmp(bDir, L"yes") == 0);

			bDir = element->Attribute(L"bg");
			if (bDir)
				_nppGUI._globalOverride.enableBg = (lstrcmp(bDir, L"yes") == 0);

			bDir = element->Attribute(L"font");
			if (bDir)
				_nppGUI._globalOverride.enableFont = (lstrcmp(bDir, L"yes") == 0);

			bDir = element->Attribute(L"fontSize");
			if (bDir)
				_nppGUI._globalOverride.enableFontSize = (lstrcmp(bDir, L"yes") == 0);

			bDir = element->Attribute(L"bold");
			if (bDir)
				_nppGUI._globalOverride.enableBold = (lstrcmp(bDir, L"yes") == 0);

			bDir = element->Attribute(L"italic");
			if (bDir)
				_nppGUI._globalOverride.enableItalic = (lstrcmp(bDir, L"yes") == 0);

			bDir = element->Attribute(L"underline");
			if (bDir)
				_nppGUI._globalOverride.enableUnderLine = (lstrcmp(bDir, L"yes") == 0);
		}
		else if (!lstrcmp(nm, L"auto-completion"))
		{
			int i;
			if (element->Attribute(L"autoCAction", &i))
				_nppGUI._autocStatus = static_cast<NppGUI::AutocStatus>(i);

			if (element->Attribute(L"triggerFromNbChar", &i))
				_nppGUI._autocFromLen = i;

			const wchar_t * optName = element->Attribute(L"autoCIgnoreNumbers");
			if (optName)
				_nppGUI._autocIgnoreNumbers = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"insertSelectedItemUseENTER");
			if (optName)
				_nppGUI._autocInsertSelectedUseENTER = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"insertSelectedItemUseTAB");
			if (optName)
				_nppGUI._autocInsertSelectedUseTAB = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"autoCBrief");
			if (optName)
				_nppGUI._autocBrief = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"funcParams");
			if (optName)
				_nppGUI._funcParams = (lstrcmp(optName, L"yes") == 0);
		}
		else if (!lstrcmp(nm, L"auto-insert"))
		{
			const wchar_t * optName = element->Attribute(L"htmlXmlTag");
			if (optName)
				_nppGUI._matchedPairConf._doHtmlXmlTag = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"parentheses");
			if (optName)
				_nppGUI._matchedPairConf._doParentheses = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"brackets");
			if (optName)
				_nppGUI._matchedPairConf._doBrackets = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"curlyBrackets");
			if (optName)
				_nppGUI._matchedPairConf._doCurlyBrackets = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"quotes");
			if (optName)
				_nppGUI._matchedPairConf._doQuotes = (lstrcmp(optName, L"yes") == 0);

			optName = element->Attribute(L"doubleQuotes");
			if (optName)
				_nppGUI._matchedPairConf._doDoubleQuotes = (lstrcmp(optName, L"yes") == 0);

			for (TiXmlNode *subChildNode = childNode->FirstChildElement(L"UserDefinePair");
				 subChildNode;
				 subChildNode = subChildNode->NextSibling(L"UserDefinePair") )
			{
				int open = -1;
				int openVal = 0;
				const wchar_t *openValStr = (subChildNode->ToElement())->Attribute(L"open", &openVal);
				if (openValStr && (openVal >= 0 && openVal < 128))
					open = openVal;

				int close = -1;
				int closeVal = 0;
				const wchar_t *closeValStr = (subChildNode->ToElement())->Attribute(L"close", &closeVal);
				if (closeValStr && (closeVal >= 0 && closeVal <= 128))
					close = closeVal;

				if (open != -1 && close != -1)
					_nppGUI._matchedPairConf._matchedPairs.push_back(pair<char, char>(char(open), char(close)));
			}
		}

		else if (!lstrcmp(nm, L"sessionExt"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
					_nppGUI._definedSessionExt = val;
			}
		}

		else if (!lstrcmp(nm, L"workspaceExt"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
					_nppGUI._definedWorkspaceExt = val;
			}
		}

		else if (!lstrcmp(nm, L"noUpdate"))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const wchar_t* val = n->Value();
				if (val)
					_nppGUI._autoUpdateOpt._doAutoUpdate = (!lstrcmp(val, L"yes"))?false:true;

				int i;
				val = element->Attribute(L"intervalDays", &i);
				if (val)
					_nppGUI._autoUpdateOpt._intervalDays = i;

				val = element->Attribute(L"nextUpdateDate");
				if (val)
					_nppGUI._autoUpdateOpt._nextUpdateDate = Date(val);
			}
		}

		else if (!lstrcmp(nm, L"openSaveDir"))
		{
			const wchar_t * value = element->Attribute(L"value");
			if (value && value[0])
			{
				if (lstrcmp(value, L"1") == 0)
					_nppGUI._openSaveDir = dir_last;
				else if (lstrcmp(value, L"2") == 0)
					_nppGUI._openSaveDir = dir_userDef;
				else
					_nppGUI._openSaveDir = dir_followCurrent;
			}

			const wchar_t * path = element->Attribute(L"defaultDirPath");
			if (path && path[0])
			{
				lstrcpyn(_nppGUI._defaultDir, path, MAX_PATH);
				::ExpandEnvironmentStrings(_nppGUI._defaultDir, _nppGUI._defaultDirExp, MAX_PATH);
			}

			path = element->Attribute(L"lastUsedDirPath");
			if (path && path[0])
			{
				lstrcpyn(_nppGUI._lastUsedDir, path, MAX_PATH);
			}
 		}

		else if (!lstrcmp(nm, L"titleBar"))
		{
			const wchar_t * value = element->Attribute(L"short");
			_nppGUI._shortTitlebar = false;	//default state
			if (value && value[0])
			{
				if (lstrcmp(value, L"yes") == 0)
					_nppGUI._shortTitlebar = true;
				else if (lstrcmp(value, L"no") == 0)
					_nppGUI._shortTitlebar = false;
			}
		}

		else if (!lstrcmp(nm, L"insertDateTime"))
		{
			const wchar_t* customFormat = element->Attribute(L"customizedFormat");
			if (customFormat != NULL && customFormat[0])
				_nppGUI._dateTimeFormat = customFormat;

			const wchar_t* value = element->Attribute(L"reverseDefaultOrder");
			if (value && value[0])
			{
				if (lstrcmp(value, L"yes") == 0)
					_nppGUI._dateTimeReverseDefaultOrder = true;
				else if (lstrcmp(value, L"no") == 0)
					_nppGUI._dateTimeReverseDefaultOrder = false;
			}
		}

		else if (!lstrcmp(nm, L"wordCharList"))
		{
			const wchar_t * value = element->Attribute(L"useDefault");
			if (value && value[0])
			{
				if (lstrcmp(value, L"yes") == 0)
					_nppGUI._isWordCharDefault = true;
				else if (lstrcmp(value, L"no") == 0)
					_nppGUI._isWordCharDefault = false;
			}

			const wchar_t *charsAddedW = element->Attribute(L"charsAdded");
			if (charsAddedW)
			{
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
				_nppGUI._customWordChars = wmc.wchar2char(charsAddedW, SC_CP_UTF8);
			}
		}
		else if (!lstrcmp(nm, L"delimiterSelection"))
		{
			int leftmost = 0;
			element->Attribute(L"leftmostDelimiter", &leftmost);
			if (leftmost > 0 && leftmost < 256)
				_nppGUI._leftmostDelimiter = static_cast<char>(leftmost);

			int rightmost = 0;
			element->Attribute(L"rightmostDelimiter", &rightmost);
			if (rightmost > 0 && rightmost < 256)
				_nppGUI._rightmostDelimiter = static_cast<char>(rightmost);

			const wchar_t *delimiterSelectionOnEntireDocument = element->Attribute(L"delimiterSelectionOnEntireDocument");
			if (delimiterSelectionOnEntireDocument != NULL && !lstrcmp(delimiterSelectionOnEntireDocument, L"yes"))
				_nppGUI._delimiterSelectionOnEntireDocument = true;
			else
				_nppGUI._delimiterSelectionOnEntireDocument = false;
		}
		else if (!lstrcmp(nm, L"largeFileRestriction"))
		{
			int fileSizeLimit4StylingMB = 0;
			element->Attribute(L"fileSizeMB", &fileSizeLimit4StylingMB);
			if (fileSizeLimit4StylingMB > 0 && fileSizeLimit4StylingMB <= 4096)
				_nppGUI._largeFileRestriction._largeFileSizeDefInByte = (static_cast<int64_t>(fileSizeLimit4StylingMB) * 1024 * 1024);

			const wchar_t* boolVal = element->Attribute(L"isEnabled");
			if (boolVal != NULL && !lstrcmp(boolVal, L"no"))
				_nppGUI._largeFileRestriction._isEnabled = false;
			else
				_nppGUI._largeFileRestriction._isEnabled = true;

			boolVal = element->Attribute(L"allowAutoCompletion");
			if (boolVal != NULL && !lstrcmp(boolVal, L"yes"))
				_nppGUI._largeFileRestriction._allowAutoCompletion = true;
			else
				_nppGUI._largeFileRestriction._allowAutoCompletion = false;

			boolVal = element->Attribute(L"allowBraceMatch");
			if (boolVal != NULL && !lstrcmp(boolVal, L"yes"))
				_nppGUI._largeFileRestriction._allowBraceMatch = true;
			else
				_nppGUI._largeFileRestriction._allowBraceMatch = false;

			boolVal = element->Attribute(L"allowSmartHilite");
			if (boolVal != NULL && !lstrcmp(boolVal, L"yes"))
				_nppGUI._largeFileRestriction._allowSmartHilite = true;
			else
				_nppGUI._largeFileRestriction._allowSmartHilite = false;

			boolVal = element->Attribute(L"allowClickableLink");
			if (boolVal != NULL && !lstrcmp(boolVal, L"yes"))
				_nppGUI._largeFileRestriction._allowClickableLink = true;
			else
				_nppGUI._largeFileRestriction._allowClickableLink = false;

			boolVal = element->Attribute(L"deactivateWordWrap");
			if (boolVal != NULL && !lstrcmp(boolVal, L"no"))
				_nppGUI._largeFileRestriction._deactivateWordWrap = false;
			else
				_nppGUI._largeFileRestriction._deactivateWordWrap = true;

			boolVal = element->Attribute(L"suppress2GBWarning");
			if (boolVal != NULL && !lstrcmp(boolVal, L"yes"))
				_nppGUI._largeFileRestriction._suppress2GBWarning = true;
			else
				_nppGUI._largeFileRestriction._suppress2GBWarning = false;
		}
		else if (!lstrcmp(nm, L"multiInst"))
		{
			int val = 0;
			element->Attribute(L"setting", &val);
			if (val < 0 || val > 2)
				val = 0;
			_nppGUI._multiInstSetting = (MultiInstSetting)val;

			_nppGUI._clipboardHistoryPanelKeepState = parseYesNoBoolAttribute(L"clipboardHistory");
			_nppGUI._docListKeepState = parseYesNoBoolAttribute(L"documentList");
			_nppGUI._charPanelKeepState = parseYesNoBoolAttribute(L"characterPanel");
			_nppGUI._fileBrowserKeepState = parseYesNoBoolAttribute(L"folderAsWorkspace");
			_nppGUI._projectPanelKeepState = parseYesNoBoolAttribute(L"projectPanels");
			_nppGUI._docMapKeepState = parseYesNoBoolAttribute(L"documentMap");
			_nppGUI._funcListKeepState = parseYesNoBoolAttribute(L"fuctionList");
			_nppGUI._pluginPanelKeepState = parseYesNoBoolAttribute(L"pluginPanels");
		}
		else if (!lstrcmp(nm, L"searchEngine"))
		{
			int i;
			if (element->Attribute(L"searchEngineChoice", &i))
				_nppGUI._searchEngineChoice = static_cast<NppGUI::SearchEngineChoice>(i);

			const wchar_t * searchEngineCustom = element->Attribute(L"searchEngineCustom");
			if (searchEngineCustom && searchEngineCustom[0])
				_nppGUI._searchEngineCustom = searchEngineCustom;
		}
		else if (!lstrcmp(nm, L"Searching"))
		{
			const wchar_t* optNameMonoFont = element->Attribute(L"monospacedFontFindDlg");
			if (optNameMonoFont)
				_nppGUI._monospacedFontFindDlg = (lstrcmp(optNameMonoFont, L"yes") == 0);

			//This is an option from previous versions of notepad++.  It is handled for compatibility with older settings.
			const wchar_t* optStopFillingFindField = element->Attribute(L"stopFillingFindField");
			if (optStopFillingFindField) 
			{
				_nppGUI._fillFindFieldWithSelected = (lstrcmp(optStopFillingFindField, L"no") == 0);
				_nppGUI._fillFindFieldSelectCaret = _nppGUI._fillFindFieldWithSelected;
			}

			const wchar_t* optFillFindFieldWithSelected = element->Attribute(L"fillFindFieldWithSelected");
			if (optFillFindFieldWithSelected)
				_nppGUI._fillFindFieldWithSelected = (lstrcmp(optFillFindFieldWithSelected, L"yes") == 0);

			const wchar_t* optFillFindFieldSelectCaret = element->Attribute(L"fillFindFieldSelectCaret");
			if (optFillFindFieldSelectCaret)
				_nppGUI._fillFindFieldSelectCaret = (lstrcmp(optFillFindFieldSelectCaret, L"yes") == 0);

			const wchar_t* optFindDlgAlwaysVisible = element->Attribute(L"findDlgAlwaysVisible");
			if (optFindDlgAlwaysVisible)
				_nppGUI._findDlgAlwaysVisible = (lstrcmp(optFindDlgAlwaysVisible, L"yes") == 0);

			const wchar_t* optConfirmReplaceOpenDocs = element->Attribute(L"confirmReplaceInAllOpenDocs");
			if (optConfirmReplaceOpenDocs)
				_nppGUI._confirmReplaceInAllOpenDocs = (lstrcmp(optConfirmReplaceOpenDocs, L"yes") == 0);

			const wchar_t* optReplaceStopsWithoutFindingNext = element->Attribute(L"replaceStopsWithoutFindingNext");
			if (optReplaceStopsWithoutFindingNext)
				_nppGUI._replaceStopsWithoutFindingNext = (lstrcmp(optReplaceStopsWithoutFindingNext, L"yes") == 0);

			int inSelThresh;
			if (element->Attribute(L"inSelectionAutocheckThreshold", &inSelThresh) &&
				(inSelThresh >= 0 && inSelThresh <= FINDREPLACE_INSELECTION_THRESHOLD_DEFAULT))
			{
				_nppGUI._inSelectionAutocheckThreshold = inSelThresh;
			}
			else
			{
				_nppGUI._inSelectionAutocheckThreshold = FINDREPLACE_INSELECTION_THRESHOLD_DEFAULT;
			}

			const wchar_t* optFillDirFieldFromActiveDoc = element->Attribute(L"fillDirFieldFromActiveDoc");
			if (optFillDirFieldFromActiveDoc)
			{
				_nppGUI._fillDirFieldFromActiveDoc = (lstrcmp(optFillDirFieldFromActiveDoc, L"yes") == 0);
			}
		}
		else if (!lstrcmp(nm, L"MISC"))
		{
			const wchar_t * optName = element->Attribute(L"fileSwitcherWithoutExtColumn");
			if (optName)
				_nppGUI._fileSwitcherWithoutExtColumn = (lstrcmp(optName, L"yes") == 0);
			
			int i = 0;
			if (element->Attribute(L"fileSwitcherExtWidth", &i))
				_nppGUI._fileSwitcherExtWidth = i;

			const wchar_t * optNamePath = element->Attribute(L"fileSwitcherWithoutPathColumn");
			if (optNamePath)
				_nppGUI._fileSwitcherWithoutPathColumn = (lstrcmp(optNamePath, L"yes") == 0);

			if (element->Attribute(L"fileSwitcherPathWidth", &i))
				_nppGUI._fileSwitcherPathWidth = i;

			_nppGUI._fileSwitcherDisableListViewGroups = parseYesNoBoolAttribute(L"fileSwitcherNoGroups");

			const wchar_t * optNameBackSlashEscape = element->Attribute(L"backSlashIsEscapeCharacterForSql");
			if (optNameBackSlashEscape && !lstrcmp(optNameBackSlashEscape, L"no"))
				_nppGUI._backSlashIsEscapeCharacterForSql = false;

			const wchar_t * optNameWriteTechnologyEngine = element->Attribute(L"writeTechnologyEngine");
			if (optNameWriteTechnologyEngine)
			{
				if (lstrcmp(optNameWriteTechnologyEngine, L"0") == 0)
					_nppGUI._writeTechnologyEngine = defaultTechnology;
				else if (lstrcmp(optNameWriteTechnologyEngine, L"1") == 0)
					_nppGUI._writeTechnologyEngine = directWriteTechnology;
				else if (lstrcmp(optNameWriteTechnologyEngine, L"2") == 0)
					_nppGUI._writeTechnologyEngine = directWriteRetainTechnology;
				else if (lstrcmp(optNameWriteTechnologyEngine, L"3") == 0)
					_nppGUI._writeTechnologyEngine = directWriteDcTechnology;
				else if (lstrcmp(optNameWriteTechnologyEngine, L"4") == 0)
					_nppGUI._writeTechnologyEngine = directWriteDX11Technology;
				else if (lstrcmp(optNameWriteTechnologyEngine, L"5") == 0)
					_nppGUI._writeTechnologyEngine = directWriteTechnologyUnavailable;
				//else
					// retain default value preset
			}

			const wchar_t * optNameFolderDroppedOpenFiles = element->Attribute(L"isFolderDroppedOpenFiles");
			if (optNameFolderDroppedOpenFiles)
				_nppGUI._isFolderDroppedOpenFiles = (lstrcmp(optNameFolderDroppedOpenFiles, L"yes") == 0);

			const wchar_t * optDocPeekOnTab = element->Attribute(L"docPeekOnTab");
			if (optDocPeekOnTab)
				_nppGUI._isDocPeekOnTab = (lstrcmp(optDocPeekOnTab, L"yes") == 0);

			const wchar_t * optDocPeekOnMap = element->Attribute(L"docPeekOnMap");
			if (optDocPeekOnMap)
				_nppGUI._isDocPeekOnMap = (lstrcmp(optDocPeekOnMap, L"yes") == 0);

			const wchar_t* optSortFunctionList = element->Attribute(L"sortFunctionList");
			if (optSortFunctionList)
				_nppGUI._shouldSortFunctionList = (lstrcmp(optSortFunctionList, L"yes") == 0);

			const wchar_t* saveDlgExtFilterToAllTypes = element->Attribute(L"saveDlgExtFilterToAllTypes");
			if (saveDlgExtFilterToAllTypes)
				_nppGUI._setSaveDlgExtFiltToAllTypes = (lstrcmp(saveDlgExtFilterToAllTypes, L"yes") == 0);

			const wchar_t * optMuteSounds = element->Attribute(L"muteSounds");
			if (optMuteSounds)
				_nppGUI._muteSounds = lstrcmp(optMuteSounds, L"yes") == 0;

			const wchar_t * optEnableFoldCmdToggable = element->Attribute(L"enableFoldCmdToggable");
			if (optEnableFoldCmdToggable)
				_nppGUI._enableFoldCmdToggable = lstrcmp(optEnableFoldCmdToggable, L"yes") == 0;

			const wchar_t * hideMenuRightShortcuts = element->Attribute(L"hideMenuRightShortcuts");
			if (hideMenuRightShortcuts)
				_nppGUI._hideMenuRightShortcuts = lstrcmp(hideMenuRightShortcuts, L"yes") == 0;
		}
		else if (!lstrcmp(nm, L"commandLineInterpreter"))
		{
			TiXmlNode *cmdLineInterpreterNode = childNode->FirstChild();
			if (cmdLineInterpreterNode)
			{
				const wchar_t *cli = cmdLineInterpreterNode->Value();
				if (cli && cli[0])
					_nppGUI._commandLineInterpreter.assign(cli);
			}
		}
		else if (!lstrcmp(nm, L"DarkMode"))
		{
			_nppGUI._darkmode._isEnabled = parseYesNoBoolAttribute(L"enable");

			//_nppGUI._darkmode._isEnabledPlugin = parseYesNoBoolAttribute(L"enablePlugin", true));

			int i;
			const wchar_t* val;
			val = element->Attribute(L"colorTone", &i);
			if (val)
				_nppGUI._darkmode._colorTone = static_cast<NppDarkMode::ColorTone>(i);


			val = element->Attribute(L"customColorTop", &i);
			if (val)
				_nppGUI._darkmode._customColors.pureBackground = i;

			val = element->Attribute(L"customColorMenuHotTrack", &i);
			if (val)
				_nppGUI._darkmode._customColors.hotBackground = i;

			val = element->Attribute(L"customColorActive", &i);
			if (val)
				_nppGUI._darkmode._customColors.softerBackground = i;

			val = element->Attribute(L"customColorMain", &i);
			if (val)
				_nppGUI._darkmode._customColors.background = i;

			val = element->Attribute(L"customColorError", &i);
			if (val)
				_nppGUI._darkmode._customColors.errorBackground = i;

			val = element->Attribute(L"customColorText", &i);
			if (val)
				_nppGUI._darkmode._customColors.text = i;

			val = element->Attribute(L"customColorDarkText", &i);
			if (val)
				_nppGUI._darkmode._customColors.darkerText = i;

			val = element->Attribute(L"customColorDisabledText", &i);
			if (val)
				_nppGUI._darkmode._customColors.disabledText = i;

			val = element->Attribute(L"customColorLinkText", &i);
			if (val)
				_nppGUI._darkmode._customColors.linkText = i;

			val = element->Attribute(L"customColorEdge", &i);
			if (val)
				_nppGUI._darkmode._customColors.edge = i;

			val = element->Attribute(L"customColorHotEdge", &i);
			if (val)
				_nppGUI._darkmode._customColors.hotEdge = i;

			val = element->Attribute(L"customColorDisabledEdge", &i);
			if (val)
				_nppGUI._darkmode._customColors.disabledEdge = i;

			// advanced options section
			auto parseStringAttribute = [&element](const wchar_t* name, const wchar_t* defaultName = L"") -> const wchar_t* {
				const wchar_t* val = element->Attribute(name);
				if (val != nullptr && val[0])
				{
					return element->Attribute(name);
				}
				return defaultName;
			};

			auto parseToolBarIconsAttribute = [&element](const wchar_t* name, int defaultValue = -1) -> int {
				int val;
				const wchar_t* valStr = element->Attribute(name, &val);
				if (valStr != nullptr && (val >= 0 && val <= 4))
				{
					return val;
				}
				return defaultValue;
			};

			auto parseTabIconsAttribute = [&element](const wchar_t* name, int defaultValue = -1) -> int {
				int val;
				const wchar_t* valStr = element->Attribute(name, &val);
				if (valStr != nullptr && (val >= 0 && val <= 2))
				{
					return val;
				}
				return defaultValue;
			};

			auto& windowsMode = _nppGUI._darkmode._advOptions._enableWindowsMode;
			windowsMode = parseYesNoBoolAttribute(L"enableWindowsMode");

			auto& darkDefaults = _nppGUI._darkmode._advOptions._darkDefaults;
			auto& darkThemeName = darkDefaults._xmlFileName;
			darkThemeName = parseStringAttribute(L"darkThemeName", L"DarkModeDefault.xml");
			darkDefaults._toolBarIconSet = parseToolBarIconsAttribute(L"darkToolBarIconSet", 0);
			darkDefaults._tabIconSet = parseTabIconsAttribute(L"darkTabIconSet", 2);
			darkDefaults._tabUseTheme = parseYesNoBoolAttribute(L"darkTabUseTheme");

			auto& lightDefaults = _nppGUI._darkmode._advOptions._lightDefaults;
			auto& lightThemeName = lightDefaults._xmlFileName;
			lightThemeName = parseStringAttribute(L"lightThemeName");
			lightDefaults._toolBarIconSet = parseToolBarIconsAttribute(L"lightToolBarIconSet", 4);
			lightDefaults._tabIconSet = parseTabIconsAttribute(L"lightTabIconSet", 0);
			lightDefaults._tabUseTheme = parseYesNoBoolAttribute(L"lightTabUseTheme", true);

			// Windows mode is handled later in Notepad_plus_Window::init from Notepad_plus_Window.cpp
			if (!windowsMode)
			{
				std::wstring themePath;
				std::wstring xmlFileName = _nppGUI._darkmode._isEnabled ? darkThemeName : lightThemeName;
				const bool isLocalOnly = _isLocal && !_isCloud;

				if (!xmlFileName.empty() && lstrcmp(xmlFileName.c_str(), L"stylers.xml") != 0)
				{
					themePath = isLocalOnly ? _nppPath : _userPath;
					pathAppend(themePath, L"themes\\");
					pathAppend(themePath, xmlFileName);

					if (!isLocalOnly && !doesFileExist(themePath.c_str()))
					{
						themePath = _nppPath;
						pathAppend(themePath, L"themes\\");
						pathAppend(themePath, xmlFileName);
					}
				}
				else
				{
					themePath = isLocalOnly ? _nppPath : _userPath;
					pathAppend(themePath, L"stylers.xml");

					if (!isLocalOnly && !doesFileExist(themePath.c_str()))
					{
						themePath = _nppPath;
						pathAppend(themePath, L"stylers.xml");
					}
				}

				if (doesFileExist(themePath.c_str()))
				{
					_nppGUI._themeName.assign(themePath);
				}
			}
		}
	}
}

void NppParameters::feedScintillaParam(TiXmlNode *node)
{
	TiXmlElement* element = node->ToElement();

	auto parseYesNoBoolAttribute = [&element](const wchar_t* name, bool defaultValue = false) -> bool {
		const wchar_t* nm = element->Attribute(name);
		if (nm)
		{
			if (!lstrcmp(nm, L"yes"))
				return true;
			else if (!lstrcmp(nm, L"no"))
				return false;
		}
		return defaultValue;
	};

	auto parseShowHideBoolAttribute = [&element](const wchar_t* name, bool defaultValue = false) -> bool {
		const wchar_t* nm = element->Attribute(name);
		if (nm)
		{
			if (!lstrcmp(nm, L"show"))
				return true;
			else if (!lstrcmp(nm, L"hide"))
				return false;
		}
		return defaultValue;
	};

	// Line Number Margin
	const wchar_t *nm = element->Attribute(L"lineNumberMargin");
	if (nm)
	{
		if (!lstrcmp(nm, L"show"))
			_svp._lineNumberMarginShow = true;
		else if (!lstrcmp(nm, L"hide"))
			_svp._lineNumberMarginShow = false;
	}

	// Line Number Margin dynamic width
	nm = element->Attribute(L"lineNumberDynamicWidth");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._lineNumberMarginDynamicWidth = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._lineNumberMarginDynamicWidth = false;
	}

	// Bookmark Margin
	nm = element->Attribute(L"bookMarkMargin");
	if (nm)
	{
		if (!lstrcmp(nm, L"show"))
			_svp._bookMarkMarginShow = true;
		else if (!lstrcmp(nm, L"hide"))
			_svp._bookMarkMarginShow = false;
	}

	// Change History Margin
	int chState = 0;
	nm = element->Attribute(L"isChangeHistoryEnabled", &chState);
	if (nm)
	{
		if (!lstrcmp(nm, L"yes")) // for the retro-compatibility
			chState = 1;

		_svp._isChangeHistoryEnabled4NextSession = static_cast<changeHistoryState>(chState);
		switch (chState)
		{
			case changeHistoryState::disable:
				_svp._isChangeHistoryMarginEnabled = false;
				_svp._isChangeHistoryIndicatorEnabled = false;
				break;
			case changeHistoryState::margin:
				_svp._isChangeHistoryMarginEnabled = true;
				_svp._isChangeHistoryIndicatorEnabled = false;
				break;
			case changeHistoryState::indicator:
				_svp._isChangeHistoryMarginEnabled = false;
				_svp._isChangeHistoryIndicatorEnabled = true;
				break;
			case changeHistoryState::marginIndicator:
				_svp._isChangeHistoryMarginEnabled = true;
				_svp._isChangeHistoryIndicatorEnabled = true;
				break;
			default:
			_svp._isChangeHistoryMarginEnabled = true;
			_svp._isChangeHistoryIndicatorEnabled = false;
			_svp._isChangeHistoryEnabled4NextSession = changeHistoryState::marginIndicator;
		}
	}

	// Indent GuideLine
	nm = element->Attribute(L"indentGuideLine");
	if (nm)
	{
		if (!lstrcmp(nm, L"show"))
			_svp._indentGuideLineShow = true;
		else if (!lstrcmp(nm, L"hide"))
			_svp._indentGuideLineShow= false;
	}

	// Folder Mark Style
	nm = element->Attribute(L"folderMarkStyle");
	if (nm)
	{
		if (!lstrcmp(nm, L"box"))
			_svp._folderStyle = FOLDER_STYLE_BOX;
		else if (!lstrcmp(nm, L"circle"))
			_svp._folderStyle = FOLDER_STYLE_CIRCLE;
		else if (!lstrcmp(nm, L"arrow"))
			_svp._folderStyle = FOLDER_STYLE_ARROW;
		else if (!lstrcmp(nm, L"simple"))
			_svp._folderStyle = FOLDER_STYLE_SIMPLE;
		else if (!lstrcmp(nm, L"none"))
			_svp._folderStyle = FOLDER_STYLE_NONE;
	}

	// Line Wrap method
	nm = element->Attribute(L"lineWrapMethod");
	if (nm)
	{
		if (!lstrcmp(nm, L"default"))
			_svp._lineWrapMethod = LINEWRAP_DEFAULT;
		else if (!lstrcmp(nm, L"aligned"))
			_svp._lineWrapMethod = LINEWRAP_ALIGNED;
		else if (!lstrcmp(nm, L"indent"))
			_svp._lineWrapMethod = LINEWRAP_INDENT;
	}

	// Current Line Highlighting State
	nm = element->Attribute(L"currentLineHilitingShow");
	if (nm)
	{
		if (!lstrcmp(nm, L"show"))
			_svp._currentLineHiliteMode = LINEHILITE_HILITE;
		else
			_svp._currentLineHiliteMode = LINEHILITE_NONE;
	}
	else
	{
		const wchar_t* currentLineModeStr = element->Attribute(L"currentLineIndicator");
		if (currentLineModeStr && currentLineModeStr[0])
		{
			if (lstrcmp(currentLineModeStr, L"1") == 0)
				_svp._currentLineHiliteMode = LINEHILITE_HILITE;
			else if (lstrcmp(currentLineModeStr, L"2") == 0)
				_svp._currentLineHiliteMode = LINEHILITE_FRAME;
			else
				_svp._currentLineHiliteMode = LINEHILITE_NONE;
		}
	}

	// Current Line Frame Width
	nm = element->Attribute(L"currentLineFrameWidth");
	if (nm)
	{
		unsigned char frameWidth{ 1 };
		try
		{
			frameWidth = static_cast<unsigned char>(std::stoi(nm));
		}
		catch (...)
		{
			// do nothing. frameWidth is already set to '1'.
		}
		_svp._currentLineFrameWidth = (frameWidth < 1) ? 1 : (frameWidth > 6) ? 6 : frameWidth;
	}

	// Virtual Space
	nm = element->Attribute(L"virtualSpace");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._virtualSpace = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._virtualSpace = false;
	}

	// Scrolling Beyond Last Line State
	nm = element->Attribute(L"scrollBeyondLastLine");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._scrollBeyondLastLine = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._scrollBeyondLastLine = false;
	}

	// Do not change selection or caret position when right-clicking with mouse
	nm = element->Attribute(L"rightClickKeepsSelection");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._rightClickKeepsSelection = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._rightClickKeepsSelection = false;
	}

	// Disable Advanced Scrolling
	nm = element->Attribute(L"disableAdvancedScrolling");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._disableAdvancedScrolling = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._disableAdvancedScrolling = false;
	}

	// Current wrap symbol visibility State
	nm = element->Attribute(L"wrapSymbolShow");
	if (nm)
	{
		if (!lstrcmp(nm, L"show"))
			_svp._wrapSymbolShow = true;
		else if (!lstrcmp(nm, L"hide"))
			_svp._wrapSymbolShow = false;
	}

	// Do Wrap
	nm = element->Attribute(L"Wrap");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._doWrap = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._doWrap = false;
	}

	// Do Edge
	nm = element->Attribute(L"isEdgeBgMode");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._isEdgeBgMode = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._isEdgeBgMode = false;
	}

	// Do Scintilla border edge
	nm = element->Attribute(L"borderEdge");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._showBorderEdge = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._showBorderEdge = false;
	}

	nm = element->Attribute(L"edgeMultiColumnPos");
	if (nm)
	{
		str2numberVector(nm, _svp._edgeMultiColumnPos);
	}

	int val;
	nm = element->Attribute(L"zoom", &val);
	if (nm)
	{
		_svp._zoom = val;
	}

	nm = element->Attribute(L"zoom2", &val);
	if (nm)
	{
		_svp._zoom2 = val;
	}

	// White Space visibility State
	nm = element->Attribute(L"whiteSpaceShow");
	if (nm)
	{
		if (!lstrcmp(nm, L"show"))
			_svp._whiteSpaceShow = true;
		else if (!lstrcmp(nm, L"hide"))
			_svp._whiteSpaceShow = false;
	}

	// EOL visibility State
	nm = element->Attribute(L"eolShow");
	if (nm)
	{
		if (!lstrcmp(nm, L"show"))
			_svp._eolShow = true;
		else if (!lstrcmp(nm, L"hide"))
			_svp._eolShow = false;
	}

	nm = element->Attribute(L"eolMode", &val);
	if (nm)
	{
		if (val >= 0 && val <= 3)
			_svp._eolMode = static_cast<ScintillaViewParams::crlfMode>(val);
	}

	// Unicode control and ws characters visibility state
	_svp._npcShow = parseShowHideBoolAttribute(L"npcShow", true);

	nm = element->Attribute(L"npcMode", &val);
	if (nm)
	{
		if (val >= 1 && val <= 2)
			_svp._npcMode = static_cast<ScintillaViewParams::npcMode>(val);
	}

	_svp._npcCustomColor = parseYesNoBoolAttribute(L"npcCustomColor");
	_svp._npcIncludeCcUniEol = parseYesNoBoolAttribute(L"npcIncludeCcUniEOL");
	_svp._npcNoInputC0 = parseYesNoBoolAttribute(L"npcNoInputC0");

	// C0, C1 control and Unicode EOL visibility state
	_svp._ccUniEolShow = parseYesNoBoolAttribute(L"ccShow", true);

	nm = element->Attribute(L"borderWidth", &val);
	if (nm)
	{
		if (val >= 0 && val <= 30)
			_svp._borderWidth = val;
	}

	// Do antialiased font
	nm = element->Attribute(L"smoothFont");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._doSmoothFont = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._doSmoothFont = false;
	}

	nm = element->Attribute(L"paddingLeft", &val);
	if (nm)
	{
		if (val >= 0 && val <= 30)
			_svp._paddingLeft = static_cast<unsigned char>(val);
	}

	nm = element->Attribute(L"paddingRight", &val);
	if (nm)
	{
		if (val >= 0 && val <= 30)
			_svp._paddingRight = static_cast<unsigned char>(val);
	}

	nm = element->Attribute(L"distractionFreeDivPart", &val);
	if (nm)
	{
		if (val >= 3 && val <= 9)
			_svp._distractionFreeDivPart = static_cast<unsigned char>(val);
	}

	nm = element->Attribute(L"lineCopyCutWithoutSelection");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._lineCopyCutWithoutSelection = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._lineCopyCutWithoutSelection = false;
	}

	nm = element->Attribute(L"multiSelection");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes"))
			_svp._multiSelection = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._multiSelection = false;
	}

	nm = element->Attribute(L"columnSel2MultiEdit");
	if (nm)
	{
		if (!lstrcmp(nm, L"yes") && _svp._multiSelection)
			_svp._columnSel2MultiEdit = true;
		else if (!lstrcmp(nm, L"no"))
			_svp._columnSel2MultiEdit = false;
	}
}


void NppParameters::feedDockingManager(TiXmlNode *node)
{
	TiXmlElement *element = node->ToElement();

	SIZE maxMonitorSize{ ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) }; // use primary monitor as the default
	SIZE nppSize = maxMonitorSize;
	HWND hwndNpp = ::FindWindow(Notepad_plus_Window::getClassName(), NULL);
	if (hwndNpp)
	{
		// this code-branch is currently reached only if the Notepad++ multi-instance mode is ON and it is not the 1st Notepad++ instance
		// (the feedDockingManager() is called at the Notepad++ init via the wWinMain nppParameters.load()))

		HMONITOR hCurMon = ::MonitorFromWindow(hwndNpp, MONITOR_DEFAULTTONEAREST);
		if (hCurMon)
		{
			MONITORINFO mi{};
			mi.cbSize = sizeof(MONITORINFO);
			if (::GetMonitorInfo(hCurMon, &mi))
			{
				maxMonitorSize.cx = mi.rcMonitor.right - mi.rcMonitor.left;
				maxMonitorSize.cy = mi.rcMonitor.bottom - mi.rcMonitor.top;
				nppSize = maxMonitorSize;
			}
		}

		RECT rcNpp{};
		if (::GetClientRect(hwndNpp, &rcNpp))
		{
			// rcNpp RECT could have zero size here! (if the 1st instance of Notepad++ is minimized to the task-bar (systray is ok))
			if ((rcNpp.right > _nppGUI._dockingData._minDockedPanelVisibility) && (rcNpp.bottom > _nppGUI._dockingData._minDockedPanelVisibility))
			{
				// adjust according to the current Notepad++ client-wnd area
				nppSize.cx = rcNpp.right;
				nppSize.cy = rcNpp.bottom;
			}
		}
	}
	else
	{
		// no real Notepad++ wnd available, so try to use the previously saved config.xml data instead
		if (!_nppGUI._isMaximized)
		{
			if (((_nppGUI._appPos.right > DMD_PANEL_WH_DEFAULT) && (_nppGUI._appPos.right < maxMonitorSize.cx))
				&& ((_nppGUI._appPos.bottom > DMD_PANEL_WH_DEFAULT) && (_nppGUI._appPos.bottom < maxMonitorSize.cy)))
			{
				nppSize.cx = _nppGUI._appPos.right;
				nppSize.cy = _nppGUI._appPos.bottom;
			}
		}
	}

	int i;
	if (element->Attribute(L"leftWidth", &i))
	{
		if (i > _nppGUI._dockingData._minDockedPanelVisibility)
		{
			if  (i < (nppSize.cx - _nppGUI._dockingData._minDockedPanelVisibility))
				_nppGUI._dockingData._leftWidth = i;
			else
				_nppGUI._dockingData._leftWidth = nppSize.cx - _nppGUI._dockingData._minDockedPanelVisibility; // invalid, reset
		}
		else
		{
			// invalid, reset
			_nppGUI._dockingData._leftWidth = _nppGUI._dockingData._minDockedPanelVisibility;
		}
	}
	if (element->Attribute(L"rightWidth", &i))
	{
		if (i > _nppGUI._dockingData._minDockedPanelVisibility)
		{
			if (i < (nppSize.cx - _nppGUI._dockingData._minDockedPanelVisibility))
				_nppGUI._dockingData._rightWidth = i;
			else
				_nppGUI._dockingData._rightWidth = nppSize.cx - _nppGUI._dockingData._minDockedPanelVisibility; // invalid, reset
		}
		else
		{
			// invalid, reset
			_nppGUI._dockingData._rightWidth = _nppGUI._dockingData._minDockedPanelVisibility;
		}
	}
	if (element->Attribute(L"topHeight", &i))
	{
		if (i > _nppGUI._dockingData._minDockedPanelVisibility)
		{
			if (i < (nppSize.cy - _nppGUI._dockingData._minDockedPanelVisibility))
				_nppGUI._dockingData._topHeight = i;
			else
				_nppGUI._dockingData._topHeight = nppSize.cy - _nppGUI._dockingData._minDockedPanelVisibility;  // invalid, reset
		}
		else
		{
			// invalid, reset
			_nppGUI._dockingData._topHeight = _nppGUI._dockingData._minDockedPanelVisibility;
		}
	}
	if (element->Attribute(L"bottomHeight", &i))
	{
		if (i > _nppGUI._dockingData._minDockedPanelVisibility)
		{
			if (i < (nppSize.cy - _nppGUI._dockingData._minDockedPanelVisibility))
				_nppGUI._dockingData._bottomHeight = i;
			else
				_nppGUI._dockingData._bottomHeight = nppSize.cy - _nppGUI._dockingData._minDockedPanelVisibility; // invalid, reset
		}
		else
		{
			// invalid, reset
			_nppGUI._dockingData._bottomHeight = _nppGUI._dockingData._minDockedPanelVisibility;
		}
	}

	for (TiXmlNode *childNode = node->FirstChildElement(L"FloatingWindow");
		childNode ;
		childNode = childNode->NextSibling(L"FloatingWindow") )
	{
		TiXmlElement *floatElement = childNode->ToElement();
		int cont;
		if (floatElement->Attribute(L"cont", &cont))
		{
			int x = 0;
			int y = 0;
			int w = FWI_PANEL_WH_DEFAULT;
			int h = FWI_PANEL_WH_DEFAULT;

			bool bInputDataOk = false;
			if (floatElement->Attribute(L"x", &x))
			{
				if (floatElement->Attribute(L"y", &y))
				{
					if (floatElement->Attribute(L"width", &w))
					{
						if (floatElement->Attribute(L"height", &h))
						{
							RECT rect{ x,y,w,h };
							bInputDataOk = isWindowVisibleOnAnyMonitor(rect);
						}
					}
				}
			}

			if (!bInputDataOk)
			{
				// reset to adjusted factory defaults
				// (and the panel will automatically be on the current primary monitor due to the x,y == 0,0)
				x = 0;
				y = 0;
				w = _nppGUI._dockingData._minFloatingPanelSize.cx;
				h = _nppGUI._dockingData._minFloatingPanelSize.cy + FWI_PANEL_WH_DEFAULT;
			}

			_nppGUI._dockingData._floatingWindowInfo.push_back(FloatingWindowInfo(cont, x, y, w, h));
		}
	}

	for (TiXmlNode *childNode = node->FirstChildElement(L"PluginDlg");
		childNode ;
		childNode = childNode->NextSibling(L"PluginDlg") )
	{
		TiXmlElement *dlgElement = childNode->ToElement();
		const wchar_t *name = dlgElement->Attribute(L"pluginName");

		int id;
		const wchar_t *idStr = dlgElement->Attribute(L"id", &id);
		if (name && idStr)
		{
			int current = 0; // on left
			int prev = 0; // on left

			dlgElement->Attribute(L"curr", &current);
			dlgElement->Attribute(L"prev", &prev);
			bool isVisible = false;
			const wchar_t *val = dlgElement->Attribute(L"isVisible");
			if (val)
			{
				isVisible = (lstrcmp(val, L"yes") == 0);
			}

			_nppGUI._dockingData._pluginDockInfo.push_back(PluginDlgDockingInfo(name, id, current, prev, isVisible));
		}
	}

	for (TiXmlNode *childNode = node->FirstChildElement(L"ActiveTabs");
		childNode ;
		childNode = childNode->NextSibling(L"ActiveTabs") )
	{
		TiXmlElement *dlgElement = childNode->ToElement();

		int cont;
		if (dlgElement->Attribute(L"cont", &cont))
		{
			int activeTab = 0;
			dlgElement->Attribute(L"activeTab", &activeTab);
			_nppGUI._dockingData._containerTabInfo.push_back(ContainerTabInfo(cont, activeTab));
		}
	}
}

void NppParameters::duplicateDockingManager(TiXmlNode* dockMngNode, TiXmlElement* dockMngElmt2Clone)
{
	if (!dockMngNode || !dockMngElmt2Clone) return;

	TiXmlElement *dockMngElmt = dockMngNode->ToElement();
	
	int i;
	if (dockMngElmt->Attribute(L"leftWidth", &i))
		dockMngElmt2Clone->SetAttribute(L"leftWidth", i);

	if (dockMngElmt->Attribute(L"rightWidth", &i))
		dockMngElmt2Clone->SetAttribute(L"rightWidth", i);

	if (dockMngElmt->Attribute(L"topHeight", &i))
		dockMngElmt2Clone->SetAttribute(L"topHeight", i);

	if (dockMngElmt->Attribute(L"bottomHeight", &i))
		dockMngElmt2Clone->SetAttribute(L"bottomHeight", i);


	for (TiXmlNode *childNode = dockMngNode->FirstChildElement(L"FloatingWindow");
		childNode;
		childNode = childNode->NextSibling(L"FloatingWindow"))
	{
		TiXmlElement *floatElement = childNode->ToElement();
		int cont;
		if (floatElement->Attribute(L"cont", &cont))
		{
			TiXmlElement FWNode(L"FloatingWindow");
			FWNode.SetAttribute(L"cont", cont);

			int x = 0;
			int y = 0;
			int w = 100;
			int h = 100;

			floatElement->Attribute(L"x", &x);
			FWNode.SetAttribute(L"x", x);

			floatElement->Attribute(L"y", &y);
			FWNode.SetAttribute(L"y", y);
			
			floatElement->Attribute(L"width", &w);
			FWNode.SetAttribute(L"width", w);
			
			floatElement->Attribute(L"height", &h);
			FWNode.SetAttribute(L"height", h);

			dockMngElmt2Clone->InsertEndChild(FWNode);
		}
	}

	for (TiXmlNode *childNode = dockMngNode->FirstChildElement(L"PluginDlg");
		childNode;
		childNode = childNode->NextSibling(L"PluginDlg"))
	{
		TiXmlElement *dlgElement = childNode->ToElement();
		const wchar_t *name = dlgElement->Attribute(L"pluginName");
		TiXmlElement PDNode(L"PluginDlg");

		int id;
		const wchar_t *idStr = dlgElement->Attribute(L"id", &id);
		if (name && idStr)
		{
			int curr = 0; // on left
			int prev = 0; // on left

			dlgElement->Attribute(L"curr", &curr);
			dlgElement->Attribute(L"prev", &prev);

			bool isVisible = false;
			const wchar_t *val = dlgElement->Attribute(L"isVisible");
			if (val)
			{
				isVisible = (lstrcmp(val, L"yes") == 0);
			}

			PDNode.SetAttribute(L"pluginName", name);
			PDNode.SetAttribute(L"id", idStr);
			PDNode.SetAttribute(L"curr", curr);
			PDNode.SetAttribute(L"prev", prev);
			PDNode.SetAttribute(L"isVisible", isVisible ? L"yes" : L"no");

			dockMngElmt2Clone->InsertEndChild(PDNode);
		}
	}

	for (TiXmlNode *childNode = dockMngNode->FirstChildElement(L"ActiveTabs");
		childNode;
		childNode = childNode->NextSibling(L"ActiveTabs"))
	{
		TiXmlElement *dlgElement = childNode->ToElement();
		TiXmlElement CTNode(L"ActiveTabs");
		int cont;
		if (dlgElement->Attribute(L"cont", &cont))
		{
			int activeTab = 0;
			dlgElement->Attribute(L"activeTab", &activeTab);

			CTNode.SetAttribute(L"cont", cont);
			CTNode.SetAttribute(L"activeTab", activeTab);

			dockMngElmt2Clone->InsertEndChild(CTNode);
		}
	}
}

bool NppParameters::writeScintillaParams()
{
	if (!_pXmlUserDoc) return false;

	const wchar_t *pViewName = L"ScintillaPrimaryView";
	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
	}

	TiXmlNode *configsRoot = nppRoot->FirstChildElement(L"GUIConfigs");
	if (!configsRoot)
	{
		configsRoot = nppRoot->InsertEndChild(TiXmlElement(L"GUIConfigs"));
	}

	TiXmlNode *scintNode = getChildElementByAttribut(configsRoot, L"GUIConfig", L"name", pViewName);
	if (!scintNode)
	{
		scintNode = configsRoot->InsertEndChild(TiXmlElement(L"GUIConfig"));
		(scintNode->ToElement())->SetAttribute(L"name", pViewName);
	}

	auto setYesNoBoolAttribute = [&scintNode](const wchar_t* name, bool value) -> void {
		const wchar_t* pStr = value ? L"yes" : L"no";
		(scintNode->ToElement())->SetAttribute(name, pStr);
	};

	auto setShowHideBoolAttribute = [&scintNode](const wchar_t* name, bool value) -> void {
		const wchar_t* pStr = value ? L"show" : L"hide";
		(scintNode->ToElement())->SetAttribute(name, pStr);
	};

	(scintNode->ToElement())->SetAttribute(L"lineNumberMargin", _svp._lineNumberMarginShow ? L"show" : L"hide");
	(scintNode->ToElement())->SetAttribute(L"lineNumberDynamicWidth", _svp._lineNumberMarginDynamicWidth ? L"yes" : L"no");
	(scintNode->ToElement())->SetAttribute(L"bookMarkMargin", _svp._bookMarkMarginShow ? L"show" : L"hide");
	(scintNode->ToElement())->SetAttribute(L"indentGuideLine", _svp._indentGuideLineShow ? L"show" : L"hide");
	const wchar_t *pFolderStyleStr = (_svp._folderStyle == FOLDER_STYLE_SIMPLE) ? L"simple" :
									(_svp._folderStyle == FOLDER_STYLE_ARROW) ? L"arrow" :
										(_svp._folderStyle == FOLDER_STYLE_CIRCLE) ? L"circle" :
										(_svp._folderStyle == FOLDER_STYLE_NONE) ? L"none" : L"box";

	(scintNode->ToElement())->SetAttribute(L"folderMarkStyle", pFolderStyleStr);
	
	(scintNode->ToElement())->SetAttribute(L"isChangeHistoryEnabled", _svp._isChangeHistoryEnabled4NextSession); // no -> 0 (disable), yes -> 1 (margin), yes ->2 (indicator), yes-> 3 (margin + indicator)

	const wchar_t *pWrapMethodStr = (_svp._lineWrapMethod == LINEWRAP_ALIGNED) ? L"aligned" :
								(_svp._lineWrapMethod == LINEWRAP_INDENT) ? L"indent" : L"default";

	(scintNode->ToElement())->SetAttribute(L"lineWrapMethod", pWrapMethodStr);

	(scintNode->ToElement())->SetAttribute(L"currentLineIndicator", _svp._currentLineHiliteMode);
	(scintNode->ToElement())->SetAttribute(L"currentLineFrameWidth", _svp._currentLineFrameWidth);

	(scintNode->ToElement())->SetAttribute(L"virtualSpace", _svp._virtualSpace ? L"yes" : L"no");
	(scintNode->ToElement())->SetAttribute(L"scrollBeyondLastLine", _svp._scrollBeyondLastLine ? L"yes" : L"no");
	(scintNode->ToElement())->SetAttribute(L"rightClickKeepsSelection", _svp._rightClickKeepsSelection ? L"yes" : L"no");
	(scintNode->ToElement())->SetAttribute(L"disableAdvancedScrolling", _svp._disableAdvancedScrolling ? L"yes" : L"no");
	(scintNode->ToElement())->SetAttribute(L"wrapSymbolShow", _svp._wrapSymbolShow ? L"show" : L"hide");
	(scintNode->ToElement())->SetAttribute(L"Wrap", _svp._doWrap ? L"yes" : L"no");
	(scintNode->ToElement())->SetAttribute(L"borderEdge", _svp._showBorderEdge ? L"yes" : L"no");

	std::wstring edgeColumnPosStr;
	for (auto i : _svp._edgeMultiColumnPos)
	{
		std::string s = std::to_string(i);
		edgeColumnPosStr += std::wstring(s.begin(), s.end());
		edgeColumnPosStr += L" ";
	}
	(scintNode->ToElement())->SetAttribute(L"isEdgeBgMode", _svp._isEdgeBgMode ? L"yes" : L"no");
	(scintNode->ToElement())->SetAttribute(L"edgeMultiColumnPos", edgeColumnPosStr);
	(scintNode->ToElement())->SetAttribute(L"zoom", static_cast<int>(_svp._zoom));
	(scintNode->ToElement())->SetAttribute(L"zoom2", static_cast<int>(_svp._zoom2));
	(scintNode->ToElement())->SetAttribute(L"whiteSpaceShow", _svp._whiteSpaceShow ? L"show" : L"hide");
	(scintNode->ToElement())->SetAttribute(L"eolShow", _svp._eolShow ? L"show" : L"hide");
	(scintNode->ToElement())->SetAttribute(L"eolMode", _svp._eolMode);
	setShowHideBoolAttribute(L"npcShow", _svp._npcShow);
	(scintNode->ToElement())->SetAttribute(L"npcMode", static_cast<int>(_svp._npcMode));
	setYesNoBoolAttribute(L"npcCustomColor", _svp._npcCustomColor);
	setYesNoBoolAttribute(L"npcIncludeCcUniEOL", _svp._npcIncludeCcUniEol);
	setYesNoBoolAttribute(L"npcNoInputC0", _svp._npcNoInputC0);
	setYesNoBoolAttribute(L"ccShow", _svp._ccUniEolShow);
	(scintNode->ToElement())->SetAttribute(L"borderWidth", _svp._borderWidth);
	(scintNode->ToElement())->SetAttribute(L"smoothFont", _svp._doSmoothFont ? L"yes" : L"no");
	(scintNode->ToElement())->SetAttribute(L"paddingLeft", _svp._paddingLeft);
	(scintNode->ToElement())->SetAttribute(L"paddingRight", _svp._paddingRight);
	(scintNode->ToElement())->SetAttribute(L"distractionFreeDivPart", _svp._distractionFreeDivPart);
	(scintNode->ToElement())->SetAttribute(L"lineCopyCutWithoutSelection", _svp._lineCopyCutWithoutSelection ? L"yes" : L"no");

	(scintNode->ToElement())->SetAttribute(L"multiSelection", _svp._multiSelection ? L"yes" : L"no");
	bool canEnableColumnSel2MultiEdit = _svp._multiSelection && _svp._columnSel2MultiEdit;
	(scintNode->ToElement())->SetAttribute(L"columnSel2MultiEdit", canEnableColumnSel2MultiEdit ? L"yes" : L"no");

	return true;
}

void NppParameters::createXmlTreeFromGUIParams()
{
	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
	}

	TiXmlNode *oldGUIRoot = nppRoot->FirstChildElement(L"GUIConfigs");
	TiXmlElement* dockMngNodeDup = nullptr;
	TiXmlNode* dockMngNodeOriginal = nullptr;
	if (oldGUIRoot && _nppGUI._isCmdlineNosessionActivated)
	{
		for (TiXmlNode *childNode = oldGUIRoot->FirstChildElement(L"GUIConfig");
			childNode;
			childNode = childNode->NextSibling(L"GUIConfig"))
		{
			TiXmlElement* element = childNode->ToElement();
			const wchar_t* nm = element->Attribute(L"name");
			if (nullptr == nm)
				continue;

			if (!lstrcmp(nm, L"DockingManager"))
			{
				dockMngNodeOriginal = childNode;
				break;
			}
		}

		// Copy DockingParamNode
		if (dockMngNodeOriginal)
		{
			dockMngNodeDup = new TiXmlElement(L"GUIConfig");
			dockMngNodeDup->SetAttribute(L"name", L"DockingManager");

			duplicateDockingManager(dockMngNodeOriginal, dockMngNodeDup);
		}
	}

	// Remove the old root nod if it exist
	if (oldGUIRoot)
	{
		nppRoot->RemoveChild(oldGUIRoot);
	}

	TiXmlNode *newGUIRoot = nppRoot->InsertEndChild(TiXmlElement(L"GUIConfigs"));

	// <GUIConfig name="ToolBar" visible="yes">standard</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"ToolBar");
		const wchar_t *pStr = (_nppGUI._toolbarShow) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"visible", pStr);

		if (_nppGUI._toolBarStatus == TB_SMALL)
			pStr = L"small";
		else if (_nppGUI._toolBarStatus == TB_LARGE)
			pStr = L"large";
		else if (_nppGUI._toolBarStatus == TB_SMALL2)
			pStr = L"small2";
		else if (_nppGUI._toolBarStatus == TB_LARGE2)
			pStr = L"large2";
		else //if (_nppGUI._toolBarStatus == TB_STANDARD)
			pStr = L"standard";
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name="StatusBar">show</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"StatusBar");
		const wchar_t *pStr = _nppGUI._statusBarShow ? L"show" : L"hide";
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name="TabBar" dragAndDrop="yes" drawTopBar="yes" drawInactiveTab="yes" reduce="yes" closeButton="yes" doubleClick2Close="no" vertical="no" multiLine="no" hide="no" quitOnEmpty="no" iconSetNumber="0" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"TabBar");

		const wchar_t *pStr = (_nppGUI._tabStatus & TAB_DRAGNDROP) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"dragAndDrop", pStr);

		pStr = (_nppGUI._tabStatus & TAB_DRAWTOPBAR) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"drawTopBar", pStr);

		pStr = (_nppGUI._tabStatus & TAB_DRAWINACTIVETAB) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"drawInactiveTab", pStr);

		pStr = (_nppGUI._tabStatus & TAB_REDUCE) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"reduce", pStr);

		pStr = (_nppGUI._tabStatus & TAB_CLOSEBUTTON) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"closeButton", pStr);

		pStr = (_nppGUI._tabStatus & TAB_PINBUTTON) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"pinButton", pStr);

		pStr = (_nppGUI._tabStatus & TAB_INACTIVETABSHOWBUTTON) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"buttonsOninactiveTabs", pStr);

		pStr = (_nppGUI._tabStatus & TAB_DBCLK2CLOSE) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"doubleClick2Close", pStr);

		pStr = (_nppGUI._tabStatus & TAB_VERTICAL) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"vertical", pStr);

		pStr = (_nppGUI._tabStatus & TAB_MULTILINE) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"multiLine", pStr);

		pStr = (_nppGUI._tabStatus & TAB_HIDE) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"hide", pStr);

		pStr = (_nppGUI._tabStatus & TAB_QUITONEMPTY) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"quitOnEmpty", pStr);

		pStr = (_nppGUI._tabStatus & TAB_ALTICONS) ? L"1" : L"0";
		GUIConfigElement->SetAttribute(L"iconSetNumber", pStr);
	}

	// <GUIConfig name="ScintillaViewsSplitter">vertical</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"ScintillaViewsSplitter");
		const wchar_t *pStr = _nppGUI._splitterPos == POS_VERTICAL ? L"vertical" : L"horizontal";
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name="UserDefineDlg" position="undocked">hide</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"UserDefineDlg");
		const wchar_t *pStr = (_nppGUI._userDefineDlgStatus & UDD_DOCKED) ? L"docked" : L"undocked";
		GUIConfigElement->SetAttribute(L"position", pStr);
		pStr = (_nppGUI._userDefineDlgStatus & UDD_SHOW) ? L"show" : L"hide";
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name = "TabSetting" size = "4" replaceBySpace = "no" backspaceUnindent = "no" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"TabSetting");
		const wchar_t *pStr = _nppGUI._tabReplacedBySpace ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"replaceBySpace", pStr);
		GUIConfigElement->SetAttribute(L"size", _nppGUI._tabSize);
		pStr = _nppGUI._backspaceUnindent ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"backspaceUnindent", pStr);
	}

	// <GUIConfig name = "AppPosition" x = "3900" y = "446" width = "2160" height = "1380" isMaximized = "no" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"AppPosition");
		GUIConfigElement->SetAttribute(L"x", _nppGUI._appPos.left);
		GUIConfigElement->SetAttribute(L"y", _nppGUI._appPos.top);
		GUIConfigElement->SetAttribute(L"width", _nppGUI._appPos.right);
		GUIConfigElement->SetAttribute(L"height", _nppGUI._appPos.bottom);
		GUIConfigElement->SetAttribute(L"isMaximized", _nppGUI._isMaximized ? L"yes" : L"no");
	}

	// <GUIConfig name="FindWindowPosition" left="134" top="320" right="723" bottom="684" />
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"FindWindowPosition");
		GUIConfigElement->SetAttribute(L"left", _nppGUI._findWindowPos.left);
		GUIConfigElement->SetAttribute(L"top", _nppGUI._findWindowPos.top);
		GUIConfigElement->SetAttribute(L"right", _nppGUI._findWindowPos.right);
		GUIConfigElement->SetAttribute(L"bottom", _nppGUI._findWindowPos.bottom);
		GUIConfigElement->SetAttribute(L"isLessModeOn", _nppGUI._findWindowLessMode ? L"yes" : L"no");
	}

	// <GUIConfig name="FinderConfig" wrappedLines="no" purgeBeforeEverySearch="no" showOnlyOneEntryPerFoundLine="yes"/>
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"FinderConfig");
		const wchar_t* pStr = _nppGUI._finderLinesAreCurrentlyWrapped ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"wrappedLines", pStr);
		pStr = _nppGUI._finderPurgeBeforeEverySearch ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"purgeBeforeEverySearch", pStr);
		pStr = _nppGUI._finderShowOnlyOneEntryPerFoundLine ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"showOnlyOneEntryPerFoundLine", pStr);

	}

	// <GUIConfig name="noUpdate" intervalDays="15" nextUpdateDate="20161022">no</GUIConfig>
	{
		TiXmlElement *element = insertGUIConfigBoolNode(newGUIRoot, L"noUpdate", !_nppGUI._autoUpdateOpt._doAutoUpdate);
		element->SetAttribute(L"intervalDays", _nppGUI._autoUpdateOpt._intervalDays);
		element->SetAttribute(L"nextUpdateDate", _nppGUI._autoUpdateOpt._nextUpdateDate.toString().c_str());
	}

	// <GUIConfig name="Auto-detection">yes</GUIConfig>	
	{
		const wchar_t *pStr = L"no";

		if (_nppGUI._fileAutoDetection & cdEnabledOld)
		{
			pStr = L"yesOld";

			if ((_nppGUI._fileAutoDetection & cdAutoUpdate) && (_nppGUI._fileAutoDetection & cdGo2end))
			{
				pStr = L"autoUpdate2EndOld";
			}
			else if (_nppGUI._fileAutoDetection & cdAutoUpdate)
			{
				pStr = L"autoOld";
			}
			else if (_nppGUI._fileAutoDetection & cdGo2end)
			{
				pStr = L"Update2EndOld";
			}
		}
		else if (_nppGUI._fileAutoDetection & cdEnabledNew)
		{
			pStr = L"yes";

			if ((_nppGUI._fileAutoDetection & cdAutoUpdate) && (_nppGUI._fileAutoDetection & cdGo2end))
			{
				pStr = L"autoUpdate2End";
			}
			else if (_nppGUI._fileAutoDetection & cdAutoUpdate)
			{
				pStr = L"auto";
			}
			else if (_nppGUI._fileAutoDetection & cdGo2end)
			{
				pStr = L"Update2End";
			}
		}

		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"Auto-detection");
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name="CheckHistoryFiles">no</GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, L"CheckHistoryFiles", _nppGUI._checkHistoryFiles);
	}

	// <GUIConfig name="TrayIcon">0</GUIConfig>
	{
		wchar_t szStr[12] { '\0' };
		_itow(_nppGUI._isMinimizedToTray, szStr, 10);
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"TrayIcon");
		GUIConfigElement->InsertEndChild(TiXmlText(szStr));
	}

	// <GUIConfig name="MaintainIndent">yes</GUIConfig>
	{
		//insertGUIConfigBoolNode(newGUIRoot, L"MaintainIndent", _nppGUI._maintainIndent);
		wchar_t szStr[12] = L"0";
		_itow(_nppGUI._maintainIndent, szStr, 10);
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"MaintainIndent");
		GUIConfigElement->InsertEndChild(TiXmlText(szStr));
	}

	// <GUIConfig name = "TagsMatchHighLight" TagAttrHighLight = "yes" HighLightNonHtmlZone = "no">yes< / GUIConfig>
	{
		TiXmlElement * ele = insertGUIConfigBoolNode(newGUIRoot, L"TagsMatchHighLight", _nppGUI._enableTagsMatchHilite);
		ele->SetAttribute(L"TagAttrHighLight", _nppGUI._enableTagAttrsHilite ? L"yes" : L"no");
		ele->SetAttribute(L"HighLightNonHtmlZone", _nppGUI._enableHiliteNonHTMLZone ? L"yes" : L"no");
	}

	// <GUIConfig name = "RememberLastSession">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, L"RememberLastSession", _nppGUI._rememberLastSession);
	}

	// <GUIConfig name = "RememberLastSession">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, L"KeepSessionAbsentFileEntries", _nppGUI._keepSessionAbsentFileEntries);
	}

	// <GUIConfig name = "DetectEncoding">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, L"DetectEncoding", _nppGUI._detectEncoding);
	}
	
	// <GUIConfig name = "SaveAllConfirm">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, L"SaveAllConfirm", _nppGUI._saveAllConfirm);
	}

	// <GUIConfig name = "NewDocDefaultSettings" format = "0" encoding = "0" lang = "3" codepage = "-1" openAnsiAsUTF8 = "no" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"NewDocDefaultSettings");
		GUIConfigElement->SetAttribute(L"format", static_cast<int32_t>(_nppGUI._newDocDefaultSettings._format));
		GUIConfigElement->SetAttribute(L"encoding", _nppGUI._newDocDefaultSettings._unicodeMode);
		GUIConfigElement->SetAttribute(L"lang", _nppGUI._newDocDefaultSettings._lang);
		GUIConfigElement->SetAttribute(L"codepage", _nppGUI._newDocDefaultSettings._codepage);
		GUIConfigElement->SetAttribute(L"openAnsiAsUTF8", _nppGUI._newDocDefaultSettings._openAnsiAsUtf8 ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"addNewDocumentOnStartup", _nppGUI._newDocDefaultSettings._addNewDocumentOnStartup ? L"yes" : L"no");
	}

	// <GUIConfig name = "langsExcluded" gr0 = "0" gr1 = "0" gr2 = "0" gr3 = "0" gr4 = "0" gr5 = "0" gr6 = "0" gr7 = "0" langMenuCompact = "yes" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"langsExcluded");
		writeExcludedLangList(GUIConfigElement);
		GUIConfigElement->SetAttribute(L"langMenuCompact", _nppGUI._isLangMenuCompact ? L"yes" : L"no");
	}

	// <GUIConfig name="Print" lineNumber="no" printOption="0" headerLeft="$(FULL_CURRENT_PATH)" headerMiddle="" headerRight="$(LONG_DATE) $(TIME)" headerFontName="IBMPC" headerFontStyle="1" headerFontSize="8" footerLeft="" footerMiddle="-$(CURRENT_PRINTING_PAGE)-" footerRight="" footerFontName="" footerFontStyle="0" footerFontSize="9" margeLeft="0" margeTop="0" margeRight="0" margeBottom="0" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"Print");
		writePrintSetting(GUIConfigElement);
	}

	// <GUIConfig name="Backup" action="0" useCustumDir="no" dir="" isSnapshotMode="yes" snapshotBackupTiming="7000" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"Backup");
		GUIConfigElement->SetAttribute(L"action", _nppGUI._backup);
		GUIConfigElement->SetAttribute(L"useCustumDir", _nppGUI._useDir ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"dir", _nppGUI._backupDir.c_str());

		GUIConfigElement->SetAttribute(L"isSnapshotMode", _nppGUI._isSnapshotMode ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"snapshotBackupTiming", static_cast<int32_t>(_nppGUI._snapshotBackupTiming));
	}

	// <GUIConfig name = "TaskList">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, L"TaskList", _nppGUI._doTaskList);
	}

	// <GUIConfig name = "MRU">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, L"MRU", _nppGUI._styleMRU);
	}

	// <GUIConfig name="URL">2</GUIConfig>
	{
		wchar_t szStr [12] = L"0";
		_itow(_nppGUI._styleURL, szStr, 10);
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"URL");
		GUIConfigElement->InsertEndChild(TiXmlText(szStr));
	}

	// <GUIConfig name="uriCustomizedSchemes">svn://</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"uriCustomizedSchemes");
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._uriSchemes.c_str()));
	}
	// <GUIConfig name = "globalOverride" fg = "no" bg = "no" font = "no" fontSize = "no" bold = "no" italic = "no" underline = "no" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"globalOverride");
		GUIConfigElement->SetAttribute(L"fg", _nppGUI._globalOverride.enableFg ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"bg", _nppGUI._globalOverride.enableBg ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"font", _nppGUI._globalOverride.enableFont ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"fontSize", _nppGUI._globalOverride.enableFontSize ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"bold", _nppGUI._globalOverride.enableBold ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"italic", _nppGUI._globalOverride.enableItalic ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"underline", _nppGUI._globalOverride.enableUnderLine ? L"yes" : L"no");
	}

	// <GUIConfig name = "auto-completion" autoCAction = "3" triggerFromNbChar = "1" funcParams = "yes" autoCIgnoreNumbers = "yes" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"auto-completion");
		GUIConfigElement->SetAttribute(L"autoCAction", _nppGUI._autocStatus);
		GUIConfigElement->SetAttribute(L"triggerFromNbChar", static_cast<int32_t>(_nppGUI._autocFromLen));

		const wchar_t * pStr = _nppGUI._autocIgnoreNumbers ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"autoCIgnoreNumbers", pStr);

		pStr = _nppGUI._autocInsertSelectedUseENTER ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"insertSelectedItemUseENTER", pStr);

		pStr = _nppGUI._autocInsertSelectedUseTAB ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"insertSelectedItemUseTAB", pStr);

		pStr = _nppGUI._autocBrief ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"autoCBrief", pStr);

		pStr = _nppGUI._funcParams ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"funcParams", pStr);
	}

	// <GUIConfig name = "auto-insert" parentheses = "yes" brackets = "yes" curlyBrackets = "yes" quotes = "no" doubleQuotes = "yes" htmlXmlTag = "yes" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"auto-insert");

		GUIConfigElement->SetAttribute(L"parentheses", _nppGUI._matchedPairConf._doParentheses ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"brackets", _nppGUI._matchedPairConf._doBrackets ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"curlyBrackets", _nppGUI._matchedPairConf._doCurlyBrackets ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"quotes", _nppGUI._matchedPairConf._doQuotes ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"doubleQuotes", _nppGUI._matchedPairConf._doDoubleQuotes ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"htmlXmlTag", _nppGUI._matchedPairConf._doHtmlXmlTag ? L"yes" : L"no");

		TiXmlElement hist_element{ L"" };
		hist_element.SetValue(L"UserDefinePair");
		for (size_t i = 0, nb = _nppGUI._matchedPairConf._matchedPairs.size(); i < nb; ++i)
		{
			int open = _nppGUI._matchedPairConf._matchedPairs[i].first;
			int close = _nppGUI._matchedPairConf._matchedPairs[i].second;

			(hist_element.ToElement())->SetAttribute(L"open", open);
			(hist_element.ToElement())->SetAttribute(L"close", close);
			GUIConfigElement->InsertEndChild(hist_element);
		}
	}

	// <GUIConfig name = "sessionExt">< / GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"sessionExt");
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._definedSessionExt.c_str()));
	}

	// <GUIConfig name="workspaceExt"></GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"workspaceExt");
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._definedWorkspaceExt.c_str()));
	}

	// <GUIConfig name="MenuBar">show</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"MenuBar");
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._menuBarShow ? L"show" : L"hide"));
	}

	// <GUIConfig name="Caret" width="1" blinkRate="250" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"Caret");
		GUIConfigElement->SetAttribute(L"width", _nppGUI._caretWidth);
		GUIConfigElement->SetAttribute(L"blinkRate", _nppGUI._caretBlinkRate);
	}

	// <GUIConfig name="openSaveDir" value="0" defaultDirPath="" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"openSaveDir");
		GUIConfigElement->SetAttribute(L"value", _nppGUI._openSaveDir);
		GUIConfigElement->SetAttribute(L"defaultDirPath", _nppGUI._defaultDir);
		GUIConfigElement->SetAttribute(L"lastUsedDirPath", _nppGUI._lastUsedDir);
	}

	// <GUIConfig name="titleBar" short="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"titleBar");
		const wchar_t *pStr = (_nppGUI._shortTitlebar) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"short", pStr);
	}

	// <GUIConfig name="insertDateTime" path="C:\sources\notepad-plus-plus\PowerEditor\visual.net\..\bin\stylers.xml" />
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"insertDateTime");
		GUIConfigElement->SetAttribute(L"customizedFormat", _nppGUI._dateTimeFormat.c_str());
		const wchar_t* pStr = (_nppGUI._dateTimeReverseDefaultOrder) ? L"yes" : L"no";
		GUIConfigElement->SetAttribute(L"reverseDefaultOrder", pStr);
	}

	// <GUIConfig name="wordCharList" useDefault="yes" charsAdded=".$%"  />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"wordCharList");
		GUIConfigElement->SetAttribute(L"useDefault", _nppGUI._isWordCharDefault ? L"yes" : L"no");
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const wchar_t* charsAddStr = wmc.char2wchar(_nppGUI._customWordChars.c_str(), SC_CP_UTF8);
		GUIConfigElement->SetAttribute(L"charsAdded", charsAddStr);
	}

	// <GUIConfig name="delimiterSelection" leftmostDelimiter="40" rightmostDelimiter="41" delimiterSelectionOnEntireDocument="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"delimiterSelection");
		GUIConfigElement->SetAttribute(L"leftmostDelimiter", _nppGUI._leftmostDelimiter);
		GUIConfigElement->SetAttribute(L"rightmostDelimiter", _nppGUI._rightmostDelimiter);
		GUIConfigElement->SetAttribute(L"delimiterSelectionOnEntireDocument", _nppGUI._delimiterSelectionOnEntireDocument ? L"yes" : L"no");
	}

	// <GUIConfig name="largeFileRestriction" fileSizeMB="200" isEnabled="yes" allowAutoCompletion="no" allowBraceMatch="no" deactivateWordWrap="yes" allowClickableLink="no" suppress2GBWarning="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"largeFileRestriction");
		GUIConfigElement->SetAttribute(L"fileSizeMB", static_cast<int>((_nppGUI._largeFileRestriction._largeFileSizeDefInByte / 1024) / 1024));
		GUIConfigElement->SetAttribute(L"isEnabled", _nppGUI._largeFileRestriction._isEnabled ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"allowAutoCompletion", _nppGUI._largeFileRestriction._allowAutoCompletion ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"allowBraceMatch", _nppGUI._largeFileRestriction._allowBraceMatch ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"allowSmartHilite", _nppGUI._largeFileRestriction._allowSmartHilite ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"allowClickableLink", _nppGUI._largeFileRestriction._allowClickableLink ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"deactivateWordWrap", _nppGUI._largeFileRestriction._deactivateWordWrap ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"suppress2GBWarning", _nppGUI._largeFileRestriction._suppress2GBWarning ? L"yes" : L"no");
	}

	// <GUIConfig name="multiInst" setting="0" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"multiInst");
		GUIConfigElement->SetAttribute(L"setting", _nppGUI._multiInstSetting);

		auto setYesNoBoolAttribute = [&GUIConfigElement](const wchar_t* name, bool value) -> void {
			const wchar_t* pStr = value ? L"yes" : L"no";
			GUIConfigElement->SetAttribute(name, pStr);
		};

		setYesNoBoolAttribute(L"clipboardHistory", _nppGUI._clipboardHistoryPanelKeepState);
		setYesNoBoolAttribute(L"documentList", _nppGUI._docListKeepState);
		setYesNoBoolAttribute(L"characterPanel", _nppGUI._charPanelKeepState);
		setYesNoBoolAttribute(L"folderAsWorkspace", _nppGUI._fileBrowserKeepState);
		setYesNoBoolAttribute(L"projectPanels", _nppGUI._projectPanelKeepState);
		setYesNoBoolAttribute(L"documentMap", _nppGUI._docMapKeepState);
		setYesNoBoolAttribute(L"fuctionList", _nppGUI._funcListKeepState);
		setYesNoBoolAttribute(L"pluginPanels", _nppGUI._pluginPanelKeepState);
	}

	// <GUIConfig name="MISC" fileSwitcherWithoutExtColumn="no" backSlashIsEscapeCharacterForSql="yes" isFolderDroppedOpenFiles="no" saveDlgExtFilterToAllTypes="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"MISC");

		auto setYesNoBoolAttribute = [&GUIConfigElement](const wchar_t* name, bool value) -> void {
			const wchar_t* pStr = value ? L"yes" : L"no";
			GUIConfigElement->SetAttribute(name, pStr);
		};

		GUIConfigElement->SetAttribute(L"fileSwitcherWithoutExtColumn", _nppGUI._fileSwitcherWithoutExtColumn ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"fileSwitcherExtWidth", _nppGUI._fileSwitcherExtWidth);
		GUIConfigElement->SetAttribute(L"fileSwitcherWithoutPathColumn", _nppGUI._fileSwitcherWithoutPathColumn ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"fileSwitcherPathWidth", _nppGUI._fileSwitcherPathWidth);
		setYesNoBoolAttribute(L"fileSwitcherNoGroups", _nppGUI._fileSwitcherDisableListViewGroups);
		GUIConfigElement->SetAttribute(L"backSlashIsEscapeCharacterForSql", _nppGUI._backSlashIsEscapeCharacterForSql ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"writeTechnologyEngine", _nppGUI._writeTechnologyEngine);
		GUIConfigElement->SetAttribute(L"isFolderDroppedOpenFiles", _nppGUI._isFolderDroppedOpenFiles ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"docPeekOnTab", _nppGUI._isDocPeekOnTab ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"docPeekOnMap", _nppGUI._isDocPeekOnMap ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"sortFunctionList", _nppGUI._shouldSortFunctionList ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"saveDlgExtFilterToAllTypes", _nppGUI._setSaveDlgExtFiltToAllTypes ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"muteSounds", _nppGUI._muteSounds ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"enableFoldCmdToggable", _nppGUI._enableFoldCmdToggable ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"hideMenuRightShortcuts", _nppGUI._hideMenuRightShortcuts ? L"yes" : L"no");
	}

	// <GUIConfig name="Searching" "monospacedFontFindDlg"="no" stopFillingFindField="no" findDlgAlwaysVisible="no" confirmReplaceOpenDocs="yes" confirmMacroReplaceOpenDocs="yes" confirmReplaceInFiles="yes" confirmMacroReplaceInFiles="yes" replaceStopsWithoutFindingNext="no" inSelectionAutocheckThreshold="1024" />
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"Searching");

		GUIConfigElement->SetAttribute(L"monospacedFontFindDlg", _nppGUI._monospacedFontFindDlg ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"fillFindFieldWithSelected", _nppGUI._fillFindFieldWithSelected ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"fillFindFieldSelectCaret", _nppGUI._fillFindFieldSelectCaret ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"findDlgAlwaysVisible", _nppGUI._findDlgAlwaysVisible ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"confirmReplaceInAllOpenDocs", _nppGUI._confirmReplaceInAllOpenDocs ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"replaceStopsWithoutFindingNext", _nppGUI._replaceStopsWithoutFindingNext ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"inSelectionAutocheckThreshold", _nppGUI._inSelectionAutocheckThreshold);
		GUIConfigElement->SetAttribute(L"fillDirFieldFromActiveDoc", _nppGUI._fillDirFieldFromActiveDoc ? L"yes" : L"no");
	}

	// <GUIConfig name="searchEngine" searchEngineChoice="2" searchEngineCustom="" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"searchEngine");
		GUIConfigElement->SetAttribute(L"searchEngineChoice", _nppGUI._searchEngineChoice);
		GUIConfigElement->SetAttribute(L"searchEngineCustom", _nppGUI._searchEngineCustom);
	}

	// <GUIConfig name="MarkAll" matchCase="no" wholeWordOnly="yes" </GUIConfig>
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"MarkAll");
		GUIConfigElement->SetAttribute(L"matchCase", _nppGUI._markAllCaseSensitive ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"wholeWordOnly", _nppGUI._markAllWordOnly ? L"yes" : L"no");
	}

	// <GUIConfig name="SmartHighLight" matchCase="no" wholeWordOnly="yes" useFindSettings="no" onAnotherView="no">yes</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = insertGUIConfigBoolNode(newGUIRoot, L"SmartHighLight", _nppGUI._enableSmartHilite);
		GUIConfigElement->SetAttribute(L"matchCase", _nppGUI._smartHiliteCaseSensitive ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"wholeWordOnly", _nppGUI._smartHiliteWordOnly ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"useFindSettings", _nppGUI._smartHiliteUseFindSettings ? L"yes" : L"no");
		GUIConfigElement->SetAttribute(L"onAnotherView", _nppGUI._smartHiliteOnAnotherView ? L"yes" : L"no");
	}

	// <GUIConfig name="commandLineInterpreter">powershell</GUIConfig>
	if (_nppGUI._commandLineInterpreter.compare(CMD_INTERPRETER))
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"commandLineInterpreter");
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._commandLineInterpreter.c_str()));
	}

	// <GUIConfig name="DarkMode" enable="no" colorTone="0" />
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
		GUIConfigElement->SetAttribute(L"name", L"DarkMode");

		NppDarkMode::setAdvancedOptions();

		auto setYesNoBoolAttribute = [&GUIConfigElement](const wchar_t* name, bool value) {
			const wchar_t* pStr = value ? L"yes" : L"no";
			GUIConfigElement->SetAttribute(name, pStr);
		};

		setYesNoBoolAttribute(L"enable", _nppGUI._darkmode._isEnabled);
		GUIConfigElement->SetAttribute(L"colorTone", _nppGUI._darkmode._colorTone);

		GUIConfigElement->SetAttribute(L"customColorTop", _nppGUI._darkmode._customColors.pureBackground);
		GUIConfigElement->SetAttribute(L"customColorMenuHotTrack", _nppGUI._darkmode._customColors.hotBackground);
		GUIConfigElement->SetAttribute(L"customColorActive", _nppGUI._darkmode._customColors.softerBackground);
		GUIConfigElement->SetAttribute(L"customColorMain", _nppGUI._darkmode._customColors.background);
		GUIConfigElement->SetAttribute(L"customColorError", _nppGUI._darkmode._customColors.errorBackground);
		GUIConfigElement->SetAttribute(L"customColorText", _nppGUI._darkmode._customColors.text);
		GUIConfigElement->SetAttribute(L"customColorDarkText", _nppGUI._darkmode._customColors.darkerText);
		GUIConfigElement->SetAttribute(L"customColorDisabledText", _nppGUI._darkmode._customColors.disabledText);
		GUIConfigElement->SetAttribute(L"customColorLinkText", _nppGUI._darkmode._customColors.linkText);
		GUIConfigElement->SetAttribute(L"customColorEdge", _nppGUI._darkmode._customColors.edge);
		GUIConfigElement->SetAttribute(L"customColorHotEdge", _nppGUI._darkmode._customColors.hotEdge);
		GUIConfigElement->SetAttribute(L"customColorDisabledEdge", _nppGUI._darkmode._customColors.disabledEdge);

		// advanced options section
		setYesNoBoolAttribute(L"enableWindowsMode", _nppGUI._darkmode._advOptions._enableWindowsMode);

		GUIConfigElement->SetAttribute(L"darkThemeName", _nppGUI._darkmode._advOptions._darkDefaults._xmlFileName.c_str());
		GUIConfigElement->SetAttribute(L"darkToolBarIconSet", _nppGUI._darkmode._advOptions._darkDefaults._toolBarIconSet);
		GUIConfigElement->SetAttribute(L"darkTabIconSet", _nppGUI._darkmode._advOptions._darkDefaults._tabIconSet);
		setYesNoBoolAttribute(L"darkTabUseTheme", _nppGUI._darkmode._advOptions._darkDefaults._tabUseTheme);

		GUIConfigElement->SetAttribute(L"lightThemeName", _nppGUI._darkmode._advOptions._lightDefaults._xmlFileName.c_str());
		GUIConfigElement->SetAttribute(L"lightToolBarIconSet", _nppGUI._darkmode._advOptions._lightDefaults._toolBarIconSet);
		GUIConfigElement->SetAttribute(L"lightTabIconSet", _nppGUI._darkmode._advOptions._lightDefaults._tabIconSet);
		setYesNoBoolAttribute(L"lightTabUseTheme", _nppGUI._darkmode._advOptions._lightDefaults._tabUseTheme);
	}

	// <GUIConfig name="ScintillaPrimaryView" lineNumberMargin="show" bookMarkMargin="show" indentGuideLine="show" folderMarkStyle="box" lineWrapMethod="aligned" currentLineHilitingShow="show" scrollBeyondLastLine="no" rightClickKeepsSelection="no" disableAdvancedScrolling="no" wrapSymbolShow="hide" Wrap="no" borderEdge="yes" edge="no" edgeNbColumn="80" zoom="0" zoom2="0" whiteSpaceShow="hide" eolShow="hide" borderWidth="2" smoothFont="no" />
	writeScintillaParams();

	// <GUIConfig name="DockingManager" leftWidth="328" rightWidth="359" topHeight="200" bottomHeight="436">
	// ...
	if (_nppGUI._isCmdlineNosessionActivated && dockMngNodeDup)
	{
		newGUIRoot->InsertEndChild(*dockMngNodeDup);
		delete dockMngNodeDup;
	}
	else
	{
		insertDockingParamNode(newGUIRoot);
	}
}

bool NppParameters::writeFindHistory()
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(L"NotepadPlus");
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(L"NotepadPlus"));
	}

	TiXmlNode *findHistoryRoot = nppRoot->FirstChildElement(L"FindHistory");
	if (!findHistoryRoot)
	{
		TiXmlElement element(L"FindHistory");
		findHistoryRoot = nppRoot->InsertEndChild(element);
	}
	findHistoryRoot->Clear();

	(findHistoryRoot->ToElement())->SetAttribute(L"nbMaxFindHistoryPath",	_findHistory._nbMaxFindHistoryPath);
	(findHistoryRoot->ToElement())->SetAttribute(L"nbMaxFindHistoryFilter",  _findHistory._nbMaxFindHistoryFilter);
	(findHistoryRoot->ToElement())->SetAttribute(L"nbMaxFindHistoryFind",	_findHistory._nbMaxFindHistoryFind);
	(findHistoryRoot->ToElement())->SetAttribute(L"nbMaxFindHistoryReplace", _findHistory._nbMaxFindHistoryReplace);

	(findHistoryRoot->ToElement())->SetAttribute(L"matchWord",				_findHistory._isMatchWord ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"matchCase",				_findHistory._isMatchCase ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"wrap",					_findHistory._isWrap?L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"directionDown",			_findHistory._isDirectionDown ? L"yes" : L"no");

	(findHistoryRoot->ToElement())->SetAttribute(L"fifRecuisive",			_findHistory._isFifRecuisive ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"fifInHiddenFolder",		_findHistory._isFifInHiddenFolder ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"fifProjectPanel1",	    	_findHistory._isFifProjectPanel_1 ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"fifProjectPanel2",	      	_findHistory._isFifProjectPanel_2 ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"fifProjectPanel3",	       	_findHistory._isFifProjectPanel_3 ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"fifFilterFollowsDoc",	_findHistory._isFilterFollowDoc ? L"yes" : L"no");

	(findHistoryRoot->ToElement())->SetAttribute(L"searchMode", _findHistory._searchMode);
	(findHistoryRoot->ToElement())->SetAttribute(L"transparencyMode", _findHistory._transparencyMode);
	(findHistoryRoot->ToElement())->SetAttribute(L"transparency", _findHistory._transparency);
	(findHistoryRoot->ToElement())->SetAttribute(L"dotMatchesNewline",		_findHistory._dotMatchesNewline ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"isSearch2ButtonsMode",		_findHistory._isSearch2ButtonsMode ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"regexBackward4PowerUser",		_findHistory._regexBackward4PowerUser ? L"yes" : L"no");

	(findHistoryRoot->ToElement())->SetAttribute(L"bookmarkLine", _findHistory._isBookmarkLine ? L"yes" : L"no");
	(findHistoryRoot->ToElement())->SetAttribute(L"purge", _findHistory._isPurge ? L"yes" : L"no");

	TiXmlElement hist_element{L""};

	hist_element.SetValue(L"Path");
	for (size_t i = 0, len = _findHistory._findHistoryPaths.size(); i < len; ++i)
	{
		(hist_element.ToElement())->SetAttribute(L"name", _findHistory._findHistoryPaths[i].c_str());
		findHistoryRoot->InsertEndChild(hist_element);
	}

	hist_element.SetValue(L"Filter");
	for (size_t i = 0, len = _findHistory._findHistoryFilters.size(); i < len; ++i)
	{
		(hist_element.ToElement())->SetAttribute(L"name", _findHistory._findHistoryFilters[i].c_str());
		findHistoryRoot->InsertEndChild(hist_element);
	}

	hist_element.SetValue(L"Find");
	for (size_t i = 0, len = _findHistory._findHistoryFinds.size(); i < len; ++i)
	{
		(hist_element.ToElement())->SetAttribute(L"name", _findHistory._findHistoryFinds[i].c_str());
		findHistoryRoot->InsertEndChild(hist_element);
	}

	hist_element.SetValue(L"Replace");
	for (size_t i = 0, len = _findHistory._findHistoryReplaces.size(); i < len; ++i)
	{
		(hist_element.ToElement())->SetAttribute(L"name", _findHistory._findHistoryReplaces[i].c_str());
		findHistoryRoot->InsertEndChild(hist_element);
	}

	return true;
}

void NppParameters::insertDockingParamNode(TiXmlNode *GUIRoot)
{
	TiXmlElement DMNode(L"GUIConfig");
	DMNode.SetAttribute(L"name", L"DockingManager");
	DMNode.SetAttribute(L"leftWidth", _nppGUI._dockingData._leftWidth);
	DMNode.SetAttribute(L"rightWidth", _nppGUI._dockingData._rightWidth);
	DMNode.SetAttribute(L"topHeight", _nppGUI._dockingData._topHeight);
	DMNode.SetAttribute(L"bottomHeight", _nppGUI._dockingData._bottomHeight);

	for (size_t i = 0, len = _nppGUI._dockingData._floatingWindowInfo.size(); i < len ; ++i)
	{
		FloatingWindowInfo & fwi = _nppGUI._dockingData._floatingWindowInfo[i];
		TiXmlElement FWNode(L"FloatingWindow");
		FWNode.SetAttribute(L"cont", fwi._cont);
		FWNode.SetAttribute(L"x", fwi._pos.left);
		FWNode.SetAttribute(L"y", fwi._pos.top);
		FWNode.SetAttribute(L"width", fwi._pos.right);
		FWNode.SetAttribute(L"height", fwi._pos.bottom);

		DMNode.InsertEndChild(FWNode);
	}

	for (size_t i = 0, len = _nppGUI._dockingData._pluginDockInfo.size() ; i < len ; ++i)
	{
		PluginDlgDockingInfo & pdi = _nppGUI._dockingData._pluginDockInfo[i];
		TiXmlElement PDNode(L"PluginDlg");
		PDNode.SetAttribute(L"pluginName", pdi._name);
		PDNode.SetAttribute(L"id", pdi._internalID);
		PDNode.SetAttribute(L"curr", pdi._currContainer);
		PDNode.SetAttribute(L"prev", pdi._prevContainer);
		PDNode.SetAttribute(L"isVisible", pdi._isVisible ? L"yes" : L"no");

		DMNode.InsertEndChild(PDNode);
	}

	for (size_t i = 0, len = _nppGUI._dockingData._containerTabInfo.size(); i < len ; ++i)
	{
		ContainerTabInfo & cti = _nppGUI._dockingData._containerTabInfo[i];
		TiXmlElement CTNode(L"ActiveTabs");
		CTNode.SetAttribute(L"cont", cti._cont);
		CTNode.SetAttribute(L"activeTab", cti._activeTab);
		DMNode.InsertEndChild(CTNode);
	}

	GUIRoot->InsertEndChild(DMNode);
}

void NppParameters::writePrintSetting(TiXmlElement *element)
{
	const wchar_t *pStr = _nppGUI._printSettings._printLineNumber ? L"yes" : L"no";
	element->SetAttribute(L"lineNumber", pStr);

	element->SetAttribute(L"printOption", _nppGUI._printSettings._printOption);

	element->SetAttribute(L"headerLeft", _nppGUI._printSettings._headerLeft.c_str());
	element->SetAttribute(L"headerMiddle", _nppGUI._printSettings._headerMiddle.c_str());
	element->SetAttribute(L"headerRight", _nppGUI._printSettings._headerRight.c_str());
	element->SetAttribute(L"footerLeft", _nppGUI._printSettings._footerLeft.c_str());
	element->SetAttribute(L"footerMiddle", _nppGUI._printSettings._footerMiddle.c_str());
	element->SetAttribute(L"footerRight", _nppGUI._printSettings._footerRight.c_str());

	element->SetAttribute(L"headerFontName", _nppGUI._printSettings._headerFontName.c_str());
	element->SetAttribute(L"headerFontStyle", _nppGUI._printSettings._headerFontStyle);
	element->SetAttribute(L"headerFontSize", _nppGUI._printSettings._headerFontSize);
	element->SetAttribute(L"footerFontName", _nppGUI._printSettings._footerFontName.c_str());
	element->SetAttribute(L"footerFontStyle", _nppGUI._printSettings._footerFontStyle);
	element->SetAttribute(L"footerFontSize", _nppGUI._printSettings._footerFontSize);

	element->SetAttribute(L"margeLeft", _nppGUI._printSettings._marge.left);
	element->SetAttribute(L"margeRight", _nppGUI._printSettings._marge.right);
	element->SetAttribute(L"margeTop", _nppGUI._printSettings._marge.top);
	element->SetAttribute(L"margeBottom", _nppGUI._printSettings._marge.bottom);
}

void NppParameters::writeExcludedLangList(TiXmlElement *element)
{
	int g0 = 0; // up to 8
	int g1 = 0; // up to 16
	int g2 = 0; // up to 24
	int g3 = 0; // up to 32
	int g4 = 0; // up to 40
	int g5 = 0; // up to 48
	int g6 = 0; // up to 56
	int g7 = 0; // up to 64
	int g8 = 0; // up to 72
	int g9 = 0; // up to 80
	int g10= 0; // up to 88
	int g11= 0; // up to 96
	int g12= 0; // up to 104

	const int groupNbMember = 8;

	for (size_t i = 0, len = _nppGUI._excludedLangList.size(); i < len ; ++i)
	{
		LangType langType = _nppGUI._excludedLangList[i]._langType;
		if (langType >= L_EXTERNAL && langType < L_END)
			continue;

		int nGrp = langType / groupNbMember;
		int nMask = 1 << langType % groupNbMember;


		switch (nGrp)
		{
			case 0 :
				g0 |= nMask;
				break;
			case 1 :
				g1 |= nMask;
				break;
			case 2 :
				g2 |= nMask;
				break;
			case 3 :
				g3 |= nMask;
				break;
			case 4 :
				g4 |= nMask;
				break;
			case 5 :
				g5 |= nMask;
				break;
			case 6 :
				g6 |= nMask;
				break;
			case 7 :
				g7 |= nMask;
				break;
			case 8:
				g8 |= nMask;
				break;
			case 9:
				g9 |= nMask;
				break;
			case 10:
				g10 |= nMask;
				break;
			case 11:
				g11 |= nMask;
				break;
			case 12:
				g12 |= nMask;
				break;
		}
	}

	element->SetAttribute(L"gr0", g0);
	element->SetAttribute(L"gr1", g1);
	element->SetAttribute(L"gr2", g2);
	element->SetAttribute(L"gr3", g3);
	element->SetAttribute(L"gr4", g4);
	element->SetAttribute(L"gr5", g5);
	element->SetAttribute(L"gr6", g6);
	element->SetAttribute(L"gr7", g7);
	element->SetAttribute(L"gr8", g8);
	element->SetAttribute(L"gr9", g9);
	element->SetAttribute(L"gr10", g10);
	element->SetAttribute(L"gr11", g11);
	element->SetAttribute(L"gr12", g12);
}

TiXmlElement * NppParameters::insertGUIConfigBoolNode(TiXmlNode *r2w, const wchar_t *name, bool bVal)
{
	const wchar_t *pStr = bVal ? L"yes" : L"no";
	TiXmlElement *GUIConfigElement = (r2w->InsertEndChild(TiXmlElement(L"GUIConfig")))->ToElement();
	GUIConfigElement->SetAttribute(L"name", name);
	GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	return GUIConfigElement;
}

int RGB2int(COLORREF color)
{
	return (((((DWORD)color) & 0x0000FF) << 16) | ((((DWORD)color) & 0x00FF00)) | ((((DWORD)color) & 0xFF0000) >> 16));
}

int NppParameters::langTypeToCommandID(LangType lt) const
{
	int id;
	switch (lt)
	{
		case L_C :
			id = IDM_LANG_C; break;
		case L_CPP :
			id = IDM_LANG_CPP; break;
		case L_JAVA :
			id = IDM_LANG_JAVA;	break;
		case L_CS :
			id = IDM_LANG_CS; break;
		case L_OBJC :
			id = IDM_LANG_OBJC;	break;
		case L_HTML :
			id = IDM_LANG_HTML;	break;
		case L_XML :
			id = IDM_LANG_XML; break;
		case L_JS :
		case L_JAVASCRIPT:
			id = IDM_LANG_JS; break;
		case L_JSON:
			id = IDM_LANG_JSON; break;
		case L_JSON5:
			id = IDM_LANG_JSON5; break;
		case L_PHP :
			id = IDM_LANG_PHP; break;
		case L_ASP :
			id = IDM_LANG_ASP; break;
		case L_JSP :
			id = IDM_LANG_JSP; break;
		case L_CSS :
			id = IDM_LANG_CSS; break;
		case L_LUA :
			id = IDM_LANG_LUA; break;
		case L_PERL :
			id = IDM_LANG_PERL; break;
		case L_PYTHON :
			id = IDM_LANG_PYTHON; break;
		case L_BATCH :
			id = IDM_LANG_BATCH; break;
		case L_PASCAL :
			id = IDM_LANG_PASCAL; break;
		case L_MAKEFILE :
			id = IDM_LANG_MAKEFILE;	break;
		case L_INI :
			id = IDM_LANG_INI; break;
		case L_ASCII :
			id = IDM_LANG_ASCII; break;
		case L_RC :
			id = IDM_LANG_RC; break;
		case L_TEX :
			id = IDM_LANG_TEX; break;
		case L_FORTRAN :
			id = IDM_LANG_FORTRAN; break;
		case L_FORTRAN_77 :
			id = IDM_LANG_FORTRAN_77; break;
		case L_BASH :
			id = IDM_LANG_BASH; break;
		case L_FLASH :
			id = IDM_LANG_FLASH; break;
		case L_NSIS :
			id = IDM_LANG_NSIS; break;
		case L_USER :
			id = IDM_LANG_USER; break;
		case L_SQL :
			id = IDM_LANG_SQL; break;
		case L_MSSQL :
			id = IDM_LANG_MSSQL; break;
		case L_VB :
			id = IDM_LANG_VB; break;
		case L_TCL :
			id = IDM_LANG_TCL; break;

		case L_LISP :
			id = IDM_LANG_LISP; break;
		case L_SCHEME :
			id = IDM_LANG_SCHEME; break;
		case L_ASM :
			id = IDM_LANG_ASM; break;
		case L_DIFF :
			id = IDM_LANG_DIFF; break;
		case L_PROPS :
			id = IDM_LANG_PROPS; break;
		case L_PS :
			id = IDM_LANG_PS; break;
		case L_RUBY :
			id = IDM_LANG_RUBY; break;
		case L_SMALLTALK :
			id = IDM_LANG_SMALLTALK; break;
		case L_VHDL :
			id = IDM_LANG_VHDL; break;

		case L_ADA :
			id = IDM_LANG_ADA; break;
		case L_MATLAB :
			id = IDM_LANG_MATLAB; break;

		case L_HASKELL :
			id = IDM_LANG_HASKELL; break;

		case L_KIX :
			id = IDM_LANG_KIX; break;
		case L_AU3 :
			id = IDM_LANG_AU3; break;
		case L_VERILOG :
			id = IDM_LANG_VERILOG; break;
		case L_CAML :
			id = IDM_LANG_CAML; break;

		case L_INNO :
			id = IDM_LANG_INNO; break;

		case L_CMAKE :
			id = IDM_LANG_CMAKE; break;

		case L_YAML :
			id = IDM_LANG_YAML; break;

		case L_COBOL :
			id = IDM_LANG_COBOL; break;

		case L_D :
			id = IDM_LANG_D; break;

		case L_GUI4CLI :
			id = IDM_LANG_GUI4CLI; break;

		case L_POWERSHELL :
			id = IDM_LANG_POWERSHELL; break;

		case L_R :
			id = IDM_LANG_R; break;

		case L_COFFEESCRIPT :
			id = IDM_LANG_COFFEESCRIPT; break;

		case L_BAANC:
			id = IDM_LANG_BAANC; break;

		case L_SREC :
			id = IDM_LANG_SREC; break;

		case L_IHEX :
			id = IDM_LANG_IHEX; break;

		case L_TEHEX :
			id = IDM_LANG_TEHEX; break;

		case L_SWIFT:
			id = IDM_LANG_SWIFT; break;

		case L_ASN1 :
			id = IDM_LANG_ASN1; break;

        case L_AVS :
			id = IDM_LANG_AVS; break;

		case L_BLITZBASIC :
			id = IDM_LANG_BLITZBASIC; break;

		case L_PUREBASIC :
			id = IDM_LANG_PUREBASIC; break;

		case L_FREEBASIC :
			id = IDM_LANG_FREEBASIC; break;

		case L_CSOUND :
			id = IDM_LANG_CSOUND; break;

		case L_ERLANG :
			id = IDM_LANG_ERLANG; break;

		case L_ESCRIPT :
			id = IDM_LANG_ESCRIPT; break;

		case L_FORTH :
			id = IDM_LANG_FORTH; break;

		case L_LATEX :
			id = IDM_LANG_LATEX; break;

		case L_MMIXAL :
			id = IDM_LANG_MMIXAL; break;

		case L_NIM :
			id = IDM_LANG_NIM; break;

		case L_NNCRONTAB :
			id = IDM_LANG_NNCRONTAB; break;

		case L_OSCRIPT :
			id = IDM_LANG_OSCRIPT; break;

		case L_REBOL :
			id = IDM_LANG_REBOL; break;

		case L_REGISTRY :
			id = IDM_LANG_REGISTRY; break;

		case L_RUST :
			id = IDM_LANG_RUST; break;

		case L_SPICE :
			id = IDM_LANG_SPICE; break;

		case L_TXT2TAGS :
			id = IDM_LANG_TXT2TAGS; break;

		case L_VISUALPROLOG:
			id = IDM_LANG_VISUALPROLOG; break;

		case L_TYPESCRIPT:
			id = IDM_LANG_TYPESCRIPT; break;

		case L_GDSCRIPT:
			id = IDM_LANG_GDSCRIPT; break;

		case L_HOLLYWOOD:
			id = IDM_LANG_HOLLYWOOD; break;
			
		case L_GOLANG:
			id = IDM_LANG_GOLANG; break;
			
		case L_RAKU:
			id = IDM_LANG_RAKU; break;

		case L_TOML:
			id = IDM_LANG_TOML; break;
			
		case L_SAS:
			id = IDM_LANG_SAS; break;
			
		case L_SEARCHRESULT :
			id = -1;	break;

		case L_TEXT :
			id = IDM_LANG_TEXT;	break;


		default :
			if (lt >= L_EXTERNAL && lt < L_END)
				id = lt - L_EXTERNAL + IDM_LANG_EXTERNAL;
			else
				id = IDM_LANG_TEXT;
	}
	return id;
}

std::wstring NppParameters:: getWinVersionStr() const
{
	switch (_winVersion)
	{
		case WV_WIN32S: return L"Windows 3.1";
		case WV_95: return L"Windows 95";
		case WV_98: return L"Windows 98";
		case WV_ME: return L"Windows Millennium Edition";
		case WV_NT: return L"Windows NT";
		case WV_W2K: return L"Windows 2000";
		case WV_XP: return L"Windows XP";
		case WV_S2003: return L"Windows Server 2003";
		case WV_XPX64: return L"Windows XP 64 bits";
		case WV_VISTA: return L"Windows Vista";
		case WV_WIN7: return L"Windows 7";
		case WV_WIN8: return L"Windows 8";
		case WV_WIN81: return L"Windows 8.1";
		case WV_WIN10: return L"Windows 10";
		case WV_WIN11: return L"Windows 11";
		default: /*case WV_UNKNOWN:*/ return L"Windows unknown version";
	}
}

std::wstring NppParameters::getWinVerBitStr() const
{
	switch (_platForm)
	{
	case PF_X86:
		return L"32-bit";

	case PF_X64:
	case PF_IA64:
	case PF_ARM64:
		return L"64-bit";

	default:
		return L"Unknown-bit";
	}
}

std::wstring NppParameters::writeStyles(LexerStylerArray & lexersStylers, StyleArray & globalStylers)
{
	TiXmlNode *lexersRoot = (_pXmlUserStylerDoc->FirstChild(L"NotepadPlus"))->FirstChildElement(L"LexerStyles");
	for (TiXmlNode *childNode = lexersRoot->FirstChildElement(L"LexerType");
		childNode ;
		childNode = childNode->NextSibling(L"LexerType"))
	{
		TiXmlElement *element = childNode->ToElement();
		const wchar_t *nm = element->Attribute(L"name");

		LexerStyler *pLs = _lexerStylerVect.getLexerStylerByName(nm);
		LexerStyler *pLs2 = lexersStylers.getLexerStylerByName(nm);

		if (pLs)
		{
			const wchar_t *extStr = pLs->getLexerUserExt();
			element->SetAttribute(L"ext", extStr);
			for (TiXmlNode *grChildNode = childNode->FirstChildElement(L"WordsStyle");
					grChildNode ;
					grChildNode = grChildNode->NextSibling(L"WordsStyle"))
			{
				TiXmlElement *grElement = grChildNode->ToElement();
				const wchar_t *styleName = grElement->Attribute(L"name");
				const Style * pStyle = pLs->findByName(styleName);
				Style * pStyle2Sync = pLs2 ? pLs2->findByName(styleName) : nullptr;
				if (pStyle && pStyle2Sync)
				{
					writeStyle2Element(*pStyle, *pStyle2Sync, grElement);
				}
			}
		}
	}

	for (size_t x = 0; x < _pXmlExternalLexerDoc.size(); ++x)
	{
		TiXmlNode* lexersRoot2 = ( _pXmlExternalLexerDoc[x]->FirstChild(L"NotepadPlus"))->FirstChildElement(L"LexerStyles");
		for (TiXmlNode* childNode = lexersRoot2->FirstChildElement(L"LexerType");
			childNode ;
			childNode = childNode->NextSibling(L"LexerType"))
		{
			TiXmlElement *element = childNode->ToElement();
			const wchar_t *nm = element->Attribute(L"name");

			LexerStyler *pLs = _lexerStylerVect.getLexerStylerByName(nm);
			LexerStyler *pLs2 = lexersStylers.getLexerStylerByName(nm);

			if (pLs)
			{
				const wchar_t *extStr = pLs->getLexerUserExt();
				element->SetAttribute(L"ext", extStr);

				for (TiXmlNode *grChildNode = childNode->FirstChildElement(L"WordsStyle");
						grChildNode ;
						grChildNode = grChildNode->NextSibling(L"WordsStyle"))
				{
					TiXmlElement *grElement = grChildNode->ToElement();
					const wchar_t *styleName = grElement->Attribute(L"name");
					const Style * pStyle = pLs->findByName(styleName);
					Style * pStyle2Sync = pLs2 ? pLs2->findByName(styleName) : nullptr;
					if (pStyle && pStyle2Sync)
					{
						writeStyle2Element(*pStyle, *pStyle2Sync, grElement);
					}
				}
			}
		}
		_pXmlExternalLexerDoc[x]->SaveFile();
	}

	TiXmlNode *globalStylesRoot = (_pXmlUserStylerDoc->FirstChild(L"NotepadPlus"))->FirstChildElement(L"GlobalStyles");

	for (TiXmlNode *childNode = globalStylesRoot->FirstChildElement(L"WidgetStyle");
		childNode ;
		childNode = childNode->NextSibling(L"WidgetStyle"))
	{
		TiXmlElement *pElement = childNode->ToElement();
		const wchar_t *styleName = pElement->Attribute(L"name");
		const Style * pStyle = _widgetStyleArray.findByName(styleName);
		Style * pStyle2Sync = globalStylers.findByName(styleName);
		if (pStyle && pStyle2Sync)
		{
			writeStyle2Element(*pStyle, *pStyle2Sync, pElement);
		}
	}

	bool isSaved = _pXmlUserStylerDoc->SaveFile();
	if (!isSaved)
	{
		auto savePath = _themeSwitcher.getSavePathFrom(_pXmlUserStylerDoc->Value());
		if (!savePath.empty())
		{
			_pXmlUserStylerDoc->SaveFile(savePath.c_str());
			return savePath;
		}
	}
	return L"";
}


bool NppParameters::insertTabInfo(const wchar_t* langName, int tabInfo, bool backspaceUnindent)
{
	if (!_pXmlDoc) return false;
	TiXmlNode *langRoot = (_pXmlDoc->FirstChild(L"NotepadPlus"))->FirstChildElement(L"Languages");
	for (TiXmlNode *childNode = langRoot->FirstChildElement(L"Language");
		childNode ;
		childNode = childNode->NextSibling(L"Language"))
	{
		TiXmlElement *element = childNode->ToElement();
		const wchar_t *nm = element->Attribute(L"name");
		if (nm && lstrcmp(langName, nm) == 0)
		{
			childNode->ToElement()->SetAttribute(L"tabSettings", tabInfo);
			childNode->ToElement()->SetAttribute(L"backspaceUnindent", backspaceUnindent ? L"yes" : L"no");
			_pXmlDoc->SaveFile();
			return true;
		}
	}
	return false;
}

void NppParameters::writeStyle2Element(const Style & style2Write, Style & style2Sync, TiXmlElement *element)
{
	if (HIBYTE(HIWORD(style2Write._fgColor)) != 0xFF)
	{
		int rgbVal = RGB2int(style2Write._fgColor);
		wchar_t fgStr[7];
		wsprintf(fgStr, L"%.6X", rgbVal);
		element->SetAttribute(L"fgColor", fgStr);
	}

	if (HIBYTE(HIWORD(style2Write._bgColor)) != 0xFF)
	{
		int rgbVal = RGB2int(style2Write._bgColor);
		wchar_t bgStr[7];
		wsprintf(bgStr, L"%.6X", rgbVal);
		element->SetAttribute(L"bgColor", bgStr);
	}

	if (style2Write._colorStyle != COLORSTYLE_ALL)
	{
		element->SetAttribute(L"colorStyle", style2Write._colorStyle);
	}

	if (!style2Write._fontName.empty())
	{
		const wchar_t * oldFontName = element->Attribute(L"fontName");
		if (oldFontName && oldFontName != style2Write._fontName)
		{
			element->SetAttribute(L"fontName", style2Write._fontName);
			style2Sync._fontName = style2Write._fontName;
		}
	}

	if (style2Write._fontSize != STYLE_NOT_USED)
	{
		if (!style2Write._fontSize)
			element->SetAttribute(L"fontSize", L"");
		else
			element->SetAttribute(L"fontSize", style2Write._fontSize);
	}

	if (style2Write._fontStyle != STYLE_NOT_USED)
	{
		element->SetAttribute(L"fontStyle", style2Write._fontStyle);
	}


	TiXmlNode *teteDeNoeud = element->LastChild();

	if (teteDeNoeud)
		teteDeNoeud->SetValue(style2Write._keywords.c_str());
	else
		element->InsertEndChild(TiXmlText(style2Write._keywords.c_str()));

}

void NppParameters::insertUserLang2Tree(TiXmlNode *node, UserLangContainer *userLang)
{
	TiXmlElement *rootElement = (node->InsertEndChild(TiXmlElement(L"UserLang")))->ToElement();

	wchar_t temp[32];
	std::wstring udlVersion;
	udlVersion += _itow(SCE_UDL_VERSION_MAJOR, temp, 10);
	udlVersion += L".";
	udlVersion += _itow(SCE_UDL_VERSION_MINOR, temp, 10);

	rootElement->SetAttribute(L"name", userLang->_name);
	rootElement->SetAttribute(L"ext", userLang->_ext);
	if (userLang->_isDarkModeTheme)
		rootElement->SetAttribute(L"darkModeTheme", L"yes");
	rootElement->SetAttribute(L"udlVersion", udlVersion.c_str());

	TiXmlElement *settingsElement = (rootElement->InsertEndChild(TiXmlElement(L"Settings")))->ToElement();
	{
		TiXmlElement *globalElement = (settingsElement->InsertEndChild(TiXmlElement(L"Global")))->ToElement();
		globalElement->SetAttribute(L"caseIgnored",			userLang->_isCaseIgnored ? L"yes" : L"no");
		globalElement->SetAttribute(L"allowFoldOfComments",	userLang->_allowFoldOfComments ? L"yes" : L"no");
		globalElement->SetAttribute(L"foldCompact",			userLang->_foldCompact ? L"yes" : L"no");
		globalElement->SetAttribute(L"forcePureLC",			userLang->_forcePureLC);
		globalElement->SetAttribute(L"decimalSeparator",	   userLang->_decimalSeparator);

		TiXmlElement *prefixElement = (settingsElement->InsertEndChild(TiXmlElement(L"Prefix")))->ToElement();
		for (int i = 0 ; i < SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
			prefixElement->SetAttribute(globalMappper().keywordNameMapper[i+SCE_USER_KWLIST_KEYWORDS1], userLang->_isPrefix[i]?L"yes" : L"no");
	}

	TiXmlElement *kwlElement = (rootElement->InsertEndChild(TiXmlElement(L"KeywordLists")))->ToElement();

	for (int i = 0 ; i < SCE_USER_KWLIST_TOTAL ; ++i)
	{
		TiXmlElement *kwElement = (kwlElement->InsertEndChild(TiXmlElement(L"Keywords")))->ToElement();
		kwElement->SetAttribute(L"name", globalMappper().keywordNameMapper[i]);
		kwElement->InsertEndChild(TiXmlText(userLang->_keywordLists[i]));
	}

	TiXmlElement *styleRootElement = (rootElement->InsertEndChild(TiXmlElement(L"Styles")))->ToElement();

	for (const Style & style2Write : userLang->_styles)
	{
		TiXmlElement *styleElement = (styleRootElement->InsertEndChild(TiXmlElement(L"WordsStyle")))->ToElement();

		if (style2Write._styleID == -1)
			continue;

		styleElement->SetAttribute(L"name", style2Write._styleDesc);

		//if (HIBYTE(HIWORD(style2Write._fgColor)) != 0xFF)
		{
			int rgbVal = RGB2int(style2Write._fgColor);
			wchar_t fgStr[7];
			wsprintf(fgStr, L"%.6X", rgbVal);
			styleElement->SetAttribute(L"fgColor", fgStr);
		}

		//if (HIBYTE(HIWORD(style2Write._bgColor)) != 0xFF)
		{
			int rgbVal = RGB2int(style2Write._bgColor);
			wchar_t bgStr[7];
			wsprintf(bgStr, L"%.6X", rgbVal);
			styleElement->SetAttribute(L"bgColor", bgStr);
		}

		if (style2Write._colorStyle != COLORSTYLE_ALL)
		{
			styleElement->SetAttribute(L"colorStyle", style2Write._colorStyle);
		}

		if (!style2Write._fontName.empty())
		{
			styleElement->SetAttribute(L"fontName", style2Write._fontName);
		}

		if (style2Write._fontStyle == STYLE_NOT_USED)
		{
			styleElement->SetAttribute(L"fontStyle", L"0");
		}
		else
		{
			styleElement->SetAttribute(L"fontStyle", style2Write._fontStyle);
		}

		if (style2Write._fontSize != STYLE_NOT_USED)
		{
			if (!style2Write._fontSize)
				styleElement->SetAttribute(L"fontSize", L"");
			else
				styleElement->SetAttribute(L"fontSize", style2Write._fontSize);
		}

		styleElement->SetAttribute(L"nesting", style2Write._nesting);
	}
}

void NppParameters::addUserModifiedIndex(size_t index)
{
	size_t len = _customizedShortcuts.size();
	bool found = false;
	for (size_t i = 0; i < len; ++i)
	{
		if (_customizedShortcuts[i] == index)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		_customizedShortcuts.push_back(index);
	}
}

void NppParameters::addPluginModifiedIndex(size_t index)
{
	size_t len = _pluginCustomizedCmds.size();
	bool found = false;
	for (size_t i = 0; i < len; ++i)
	{
		if (_pluginCustomizedCmds[i] == index)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		_pluginCustomizedCmds.push_back(index);
	}
}

void NppParameters::addScintillaModifiedIndex(int index)
{
	size_t len = _scintillaModifiedKeyIndices.size();
	bool found = false;
	for (size_t i = 0; i < len; ++i)
	{
		if (_scintillaModifiedKeyIndices[i] == index)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		_scintillaModifiedKeyIndices.push_back(index);
	}
}

#ifndef	_WIN64
void NppParameters::safeWow64EnableWow64FsRedirection(BOOL Wow64FsEnableRedirection)
{
	HMODULE kernel = GetModuleHandle(L"kernel32");
	if (kernel)
	{
		BOOL isWow64 = FALSE;
		typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
		LPFN_ISWOW64PROCESS IsWow64ProcessFunc = (LPFN_ISWOW64PROCESS) GetProcAddress(kernel,"IsWow64Process");

		if (IsWow64ProcessFunc)
		{
			IsWow64ProcessFunc(GetCurrentProcess(),&isWow64);

			if (isWow64)
			{
				typedef BOOL (WINAPI *LPFN_WOW64ENABLEWOW64FSREDIRECTION)(BOOL);
				LPFN_WOW64ENABLEWOW64FSREDIRECTION Wow64EnableWow64FsRedirectionFunc = (LPFN_WOW64ENABLEWOW64FSREDIRECTION)GetProcAddress(kernel, "Wow64EnableWow64FsRedirection");
				if (Wow64EnableWow64FsRedirectionFunc)
				{
					Wow64EnableWow64FsRedirectionFunc(Wow64FsEnableRedirection);
				}
			}
		}
	}
}
#endif

void NppParameters::setUdlXmlDirtyFromIndex(size_t i)
{
	for (auto& uxfs : _pXmlUserLangsDoc)
	{
		if (i >= uxfs._indexRange.first && i < uxfs._indexRange.second)
		{
			uxfs._isDirty = true;
			return;
		}
	}
}

/*
Considering we have done:
load default UDL:  3 languges
load a UDL file:   1 languge
load a UDL file:   2 languges
create a UDL:      1 languge
imported a UDL:    1 languge

the evolution to remove UDL one by one:

0[D1]                        0[D1]                        0[D1]                         [D1]                         [D1]
1[D2]                        1[D2]                        1[D2]                        0[D2]                         [D2]
2[D3]  [DUDL, <0,3>]         2[D3]  [DUDL, <0,3>]         2[D3]  [DUDL, <0,3>]         1[D3]  [DUDL, <0,2>]          [D3]  [DUDL, <0,0>]
3[U1]  [NUDL, <3,4>]         3[U1]  [NUDL, <3,4>]         3[U1]  [NUDL, <3,4>]         2[U1]  [NUDL, <2,3>]          [U1]  [NUDL, <0,0>]
4[U2]                        4[U2]                         [U2]                         [U2]                         [U2]
5[U2]  [NUDL, <4,6>]         5[U2]  [NUDL, <4,6>]         4[U2]  [NUDL, <4,5>]         3[U2]  [NUDL, <3,4>]         0[U2]  [NUDL, <0,1>]
6[C1]  [NULL, <6,7>]          [C1]  [NULL, <6,6>]          [C1]  [NULL, <5,5>]          [C1]  [NULL, <4,4>]          [C1]  [NULL, <1,1>]
7[I1]  [NULL, <7,8>]         6[I1]  [NULL, <6,7>]         5[I1]  [NULL, <5,6>]         4[I1]  [NULL, <4,5>]         1[I1]  [NULL, <1,2>]
*/
void NppParameters::removeIndexFromXmlUdls(size_t i)
{
	bool isUpdateBegin = false;
	for (auto& uxfs : _pXmlUserLangsDoc)
	{
		// Find index
		if (!isUpdateBegin && (i >= uxfs._indexRange.first && i < uxfs._indexRange.second)) // found it
		{
			if (uxfs._indexRange.second > 0)
				uxfs._indexRange.second -= 1;
			uxfs._isDirty = true;

			isUpdateBegin = true;
		}

		// Update
		else if (isUpdateBegin)
		{
			if (uxfs._indexRange.first > 0)
				uxfs._indexRange.first -= 1;
			if (uxfs._indexRange.second > 0)
				uxfs._indexRange.second -= 1;
		}
	}
}

void NppParameters::setUdlXmlDirtyFromXmlDoc(const TiXmlDocument* xmlDoc)
{
	for (auto& uxfs : _pXmlUserLangsDoc)
	{
		if (xmlDoc == uxfs._udlXmlDoc)
		{
			uxfs._isDirty = true;
			return;
		}
	}
}

Date::Date(const wchar_t *dateStr)
{
	// timeStr should be Notepad++ date format : YYYYMMDD
	assert(dateStr);
	int D = lstrlen(dateStr);

	if ( 8==D )
	{
		std::wstring ds(dateStr);
		std::wstring yyyy(ds, 0, 4);
		std::wstring mm(ds, 4, 2);
		std::wstring dd(ds, 6, 2);

		int y = _wtoi(yyyy.c_str());
		int m = _wtoi(mm.c_str());
		int d = _wtoi(dd.c_str());

		if ((y > 0 && y <= 9999) && (m > 0 && m <= 12) && (d > 0 && d <= 31))
		{
			_year = y;
			_month = m;
			_day = d;
			return;
		}
	}
	now();
}

// The constructor which makes the date of number of days from now
// nbDaysFromNow could be negative if user want to make a date in the past
// if the value of nbDaysFromNow is 0 then the date will be now
Date::Date(int nbDaysFromNow)
{
	const time_t oneDay = (60 * 60 * 24);

	time_t rawtime;
	const tm* timeinfo;

	time(&rawtime);
	rawtime += (nbDaysFromNow * oneDay);

	timeinfo = localtime(&rawtime);
	if (timeinfo)
	{
		_year = timeinfo->tm_year + 1900;
		_month = timeinfo->tm_mon + 1;
		_day = timeinfo->tm_mday;
	}
}

void Date::now()
{
	time_t rawtime;
	const tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	if (timeinfo)
	{
		_year = timeinfo->tm_year + 1900;
		_month = timeinfo->tm_mon + 1;
		_day = timeinfo->tm_mday;
	}
}


EolType convertIntToFormatType(int value, EolType defvalue)
{
	switch (value)
	{
		case static_cast<LPARAM>(EolType::windows) :
			return EolType::windows;
		case static_cast<LPARAM>(EolType::macos) :
				return EolType::macos;
		case static_cast<LPARAM>(EolType::unix) :
			return EolType::unix;
		default:
			return defvalue;
	}
}

void NppParameters::initTabCustomColors()
{
	StyleArray& stylers = getMiscStylerArray();

	const Style* pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_1);
	if (pStyle)
	{
		individualTabHues[0].loadFromRGB(pStyle->_bgColor);
	}

	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_2);
	if (pStyle)
	{
		individualTabHues[1].loadFromRGB(pStyle->_bgColor);
	}

	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_3);
	if (pStyle)
	{
		individualTabHues[2].loadFromRGB(pStyle->_bgColor);
	}

	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_4);
	if (pStyle)
	{
		individualTabHues[3].loadFromRGB(pStyle->_bgColor);
	}

	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_5);
	if (pStyle)
	{
		individualTabHues[4].loadFromRGB(pStyle->_bgColor);
	}


	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_DM_1);
	if (pStyle)
	{
		individualTabHuesFor_Dark[0].loadFromRGB(pStyle->_bgColor);
	}

	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_DM_2);
	if (pStyle)
	{
		individualTabHuesFor_Dark[1].loadFromRGB(pStyle->_bgColor);
	}

	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_DM_3);
	if (pStyle)
	{
		individualTabHuesFor_Dark[2].loadFromRGB(pStyle->_bgColor);
	}

	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_DM_4);
	if (pStyle)
	{
		individualTabHuesFor_Dark[3].loadFromRGB(pStyle->_bgColor);
	}

	pStyle = stylers.findByName(TABBAR_INDIVIDUALCOLOR_DM_5);
	if (pStyle)
	{
		individualTabHuesFor_Dark[4].loadFromRGB(pStyle->_bgColor);
	}
}


void NppParameters::setIndividualTabColor(COLORREF colour2Set, int colourIndex, bool isDarkMode)
{
	if (colourIndex < 0 || colourIndex > 4) return;

	if (isDarkMode)
		individualTabHuesFor_Dark[colourIndex].loadFromRGB(colour2Set);
	else
		individualTabHues[colourIndex].loadFromRGB(colour2Set);

	return;
}

COLORREF NppParameters::getIndividualTabColor(int colourIndex, bool isDarkMode, bool saturated)
{
	if (colourIndex < 0 || colourIndex > 4) return {};

	HLSColour result;
	if (isDarkMode)
	{
		result = individualTabHuesFor_Dark[colourIndex];

		if (saturated)
		{
			result._lightness = 146U;
			result._saturation = std::min<WORD>(240U, result._saturation + 100U);
		}
	}
	else
	{
		result = individualTabHues[colourIndex];

		if (saturated)
		{
			result._lightness = 140U;
			result._saturation = std::min<WORD>(240U, result._saturation + 30U);
		}
	}

	return result.toRGB();
}

void NppParameters::initFindDlgStatusMsgCustomColors()
{
	StyleArray& stylers = getMiscStylerArray();

	const Style* pStyle = stylers.findByName(FINDDLG_STAUSNOTFOUND_COLOR);
	if (pStyle)
	{
		findDlgStatusMessageColor[0] = pStyle->_fgColor;
	}

	pStyle = stylers.findByName(FINDDLG_STAUSMESSAGE_COLOR);
	if (pStyle)
	{
		findDlgStatusMessageColor[1] = pStyle->_fgColor;
	}

	pStyle = stylers.findByName(FINDDLG_STAUSREACHED_COLOR);
	if (pStyle)
	{
		findDlgStatusMessageColor[2] = pStyle->_fgColor;
	}

}

void NppParameters::setFindDlgStatusMsgIndexColor(COLORREF colour2Set, int colourIndex)
{
	if (colourIndex < 0 || colourIndex > 2) return;

	findDlgStatusMessageColor[colourIndex] = colour2Set;

	return;
}

COLORREF NppParameters::getFindDlgStatusMsgColor(int colourIndex)
{
	if (colourIndex < 0 || colourIndex > 2) return black;

	return findDlgStatusMessageColor[colourIndex];
}
