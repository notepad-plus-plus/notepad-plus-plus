     1|     1|     1|     1|// This file is part of npminmin project
     2|     2|     2|     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|     3|     3|     3|
     4|     4|     4|     4|// This program is free software: you can redistribute it and/or modify
     5|     5|     5|     5|// it under the terms of the GNU General Public License as published by
     6|     6|     6|     6|// the Free Software Foundation, either version 3 of the License, or
     7|     7|     7|     7|// at your option any later version.
     8|     8|     8|     8|//
     9|     9|     9|     9|// This program is distributed in the hope that it will be useful,
    10|    10|    10|    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|    11|    11|    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|    12|    12|    12|// GNU General Public License for more details.
    13|    13|    13|    13|//
    14|    14|    14|    14|// You should have received a copy of the GNU General Public License
    15|    15|    15|    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|    16|    16|    16|
    17|    17|    17|    17|#include "Notepad_plus.h"
    18|    18|    18|    18|
    19|    19|    19|    19|#include <shlwapi.h>
    20|    20|    20|    20|#include <wininet.h>
    21|    21|    21|    21|
    22|    22|    22|    22|#include <ctime>
    23|    23|    23|    23|#include <memory>
    24|    24|    24|    24|
    25|    25|    25|    25|#include "NppXml.h"
    26|    26|    26|    26|#include "Notepad_plus_Window.h"
    27|    27|    27|    27|#include "CustomFileDialog.h"
    28|    28|    28|    28|#include "Printer.h"
    29|    29|    29|    29|#include "FileNameStringSplitter.h"
    30|    30|    30|    30|#include "lesDlgs.h"
    31|    31|    31|    31|#include "Utf8_16.h"
    32|    32|    32|    32|#include "RunDlg.h"
    33|    33|    33|    33|#include "preferenceDlg.h"
    34|    34|    34|    34|#include "TaskListDlg.h"
    35|    35|    35|    35|#include "EncodingMapper.h"
    36|    36|    36|    36|#include "ansiCharPanel.h"
    37|    37|    37|    37|#include "clipboardHistoryPanel.h"
    38|    38|    38|    38|#include "VerticalFileSwitcher.h"
    39|    39|    39|    39|#include "ProjectPanel.h"
    40|    40|    40|    40|#include "documentMap.h"
    41|    41|    41|    41|#include "functionListPanel.h"
    42|    42|    42|    42|#include "fileBrowser.h"
    43|    43|    43|    43|#include "Common.h"
    44|    44|    44|    44|#include "NppDarkMode.h"
    45|    45|    45|    45|#include "dpiManagerV2.h"
    46|    46|    46|    46|#include "ImageListSet.h"
    47|    47|    47|    47|
    48|    48|    48|    48|using namespace std;
    49|    49|    49|    49|
    50|    50|    50|    50|chrono::steady_clock::duration g_pluginsLoadingTime{};
    51|    51|    51|    51|
    52|    52|    52|    52|enum tb_stat {tb_saved, tb_unsaved, tb_ro, tb_monitored};
    53|    53|    53|    53|#define DIR_LEFT true
    54|    54|    54|    54|#define DIR_RIGHT false
    55|    55|    55|    55|
    56|    56|    56|    56|static constexpr int IDI_SEPARATOR_ICON = -1;
    57|    57|    57|    57|
    58|    58|    58|    58|static constexpr ToolBarButtonUnit toolBarIcons[]{
    59|    59|    59|    59|    {IDM_FILE_NEW,                     IDI_NEW_ICON,               IDI_NEW_ICON,                  IDI_NEW_ICON2,              IDI_NEW_ICON2,                 IDI_NEW_ICON_DM,               IDI_NEW_ICON_DM,                  IDI_NEW_ICON_DM2,              IDI_NEW_ICON_DM2,                 IDR_FILENEW},
    60|    60|    60|    60|    {IDM_FILE_OPEN,                    IDI_OPEN_ICON,              IDI_OPEN_ICON,                 IDI_OPEN_ICON2,             IDI_OPEN_ICON2,                IDI_OPEN_ICON_DM,              IDI_OPEN_ICON_DM,                 IDI_OPEN_ICON_DM2,             IDI_OPEN_ICON_DM2,                IDR_FILEOPEN},
    61|    61|    61|    61|    {IDM_FILE_SAVE,                    IDI_SAVE_ICON,              IDI_SAVE_DISABLE_ICON,         IDI_SAVE_ICON2,             IDI_SAVE_DISABLE_ICON2,        IDI_SAVE_ICON_DM,              IDI_SAVE_DISABLE_ICON_DM,         IDI_SAVE_ICON_DM2,             IDI_SAVE_DISABLE_ICON_DM2,        IDR_FILESAVE},
    62|    62|    62|    62|    {IDM_FILE_SAVEALL,                 IDI_SAVEALL_ICON,           IDI_SAVEALL_DISABLE_ICON,      IDI_SAVEALL_ICON2,          IDI_SAVEALL_DISABLE_ICON2,     IDI_SAVEALL_ICON_DM,           IDI_SAVEALL_DISABLE_ICON_DM,      IDI_SAVEALL_ICON_DM2,          IDI_SAVEALL_DISABLE_ICON_DM2,     IDR_SAVEALL},
    63|    63|    63|    63|    {IDM_FILE_CLOSE,                   IDI_CLOSE_ICON,             IDI_CLOSE_ICON,                IDI_CLOSE_ICON2,            IDI_CLOSE_ICON2,               IDI_CLOSE_ICON_DM,             IDI_CLOSE_ICON_DM,                IDI_CLOSE_ICON_DM2,            IDI_CLOSE_ICON_DM2,               IDR_CLOSEFILE},
    64|    64|    64|    64|    {IDM_FILE_CLOSEALL,                IDI_CLOSEALL_ICON,          IDI_CLOSEALL_ICON,             IDI_CLOSEALL_ICON2,         IDI_CLOSEALL_ICON2,            IDI_CLOSEALL_ICON_DM,          IDI_CLOSEALL_ICON_DM,             IDI_CLOSEALL_ICON_DM2,         IDI_CLOSEALL_ICON_DM2,            IDR_CLOSEALL},
    65|    65|    65|    65|    {IDM_FILE_PRINT,                   IDI_PRINT_ICON,             IDI_PRINT_ICON,                IDI_PRINT_ICON2,            IDI_PRINT_ICON2,               IDI_PRINT_ICON_DM,             IDI_PRINT_ICON_DM,                IDI_PRINT_ICON_DM2,            IDI_PRINT_ICON_DM2,               IDR_PRINT},
    66|    66|    66|    66|
    67|    67|    67|    67|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    68|    68|    68|    68|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    69|    69|    69|    69|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    70|    70|    70|    70|
    71|    71|    71|    71|    {IDM_EDIT_CUT,                     IDI_CUT_ICON,               IDI_CUT_DISABLE_ICON,          IDI_CUT_ICON2,              IDI_CUT_DISABLE_ICON2,         IDI_CUT_ICON_DM,               IDI_CUT_DISABLE_ICON_DM,          IDI_CUT_ICON_DM2,              IDI_CUT_DISABLE_ICON_DM2,         IDR_CUT},
    72|    72|    72|    72|    {IDM_EDIT_COPY,                    IDI_COPY_ICON,              IDI_COPY_DISABLE_ICON,         IDI_COPY_ICON2,             IDI_COPY_DISABLE_ICON2,        IDI_COPY_ICON_DM,              IDI_COPY_DISABLE_ICON_DM,         IDI_COPY_ICON_DM2,             IDI_COPY_DISABLE_ICON_DM2,        IDR_COPY},
    73|    73|    73|    73|    {IDM_EDIT_PASTE,                   IDI_PASTE_ICON,             IDI_PASTE_DISABLE_ICON,        IDI_PASTE_ICON2,            IDI_PASTE_DISABLE_ICON2,       IDI_PASTE_ICON_DM,             IDI_PASTE_DISABLE_ICON_DM,        IDI_PASTE_ICON_DM2,            IDI_PASTE_DISABLE_ICON_DM2,       IDR_PASTE},
    74|    74|    74|    74|
    75|    75|    75|    75|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    76|    76|    76|    76|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    77|    77|    77|    77|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    78|    78|    78|    78|
    79|    79|    79|    79|    {IDM_EDIT_UNDO,                    IDI_UNDO_ICON,              IDI_UNDO_DISABLE_ICON,         IDI_UNDO_ICON2,             IDI_UNDO_DISABLE_ICON2,        IDI_UNDO_ICON_DM,              IDI_UNDO_DISABLE_ICON_DM,         IDI_UNDO_ICON_DM2,             IDI_UNDO_DISABLE_ICON_DM2,        IDR_UNDO},
    80|    80|    80|    80|    {IDM_EDIT_REDO,                    IDI_REDO_ICON,              IDI_REDO_DISABLE_ICON,         IDI_REDO_ICON2,             IDI_REDO_DISABLE_ICON2,        IDI_REDO_ICON_DM,              IDI_REDO_DISABLE_ICON_DM,         IDI_REDO_ICON_DM2,             IDI_REDO_DISABLE_ICON_DM2,        IDR_REDO},
    81|    81|    81|    81|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    82|    82|    82|    82|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    83|    83|    83|    83|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    84|    84|    84|    84|
    85|    85|    85|    85|    {IDM_SEARCH_FIND,                  IDI_FIND_ICON,              IDI_FIND_ICON,                 IDI_FIND_ICON2,             IDI_FIND_ICON2,                IDI_FIND_ICON_DM,              IDI_FIND_ICON_DM,                 IDI_FIND_ICON_DM2,             IDI_FIND_ICON_DM2,                IDR_FIND},
    86|    86|    86|    86|    {IDM_SEARCH_REPLACE,               IDI_REPLACE_ICON,           IDI_REPLACE_ICON,              IDI_REPLACE_ICON2,          IDI_REPLACE_ICON2,             IDI_REPLACE_ICON_DM,           IDI_REPLACE_ICON_DM,              IDI_REPLACE_ICON_DM2,          IDI_REPLACE_ICON_DM2,             IDR_REPLACE},
    87|    87|    87|    87|
    88|    88|    88|    88|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    89|    89|    89|    89|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    90|    90|    90|    90|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    91|    91|    91|    91|    {IDM_VIEW_ZOOMIN,                  IDI_ZOOMIN_ICON,            IDI_ZOOMIN_ICON,               IDI_ZOOMIN_ICON2,           IDI_ZOOMIN_ICON2,              IDI_ZOOMIN_ICON_DM,            IDI_ZOOMIN_ICON_DM,               IDI_ZOOMIN_ICON_DM2,           IDI_ZOOMIN_ICON_DM2,              IDR_ZOOMIN},
    92|    92|    92|    92|    {IDM_VIEW_ZOOMOUT,                 IDI_ZOOMOUT_ICON,           IDI_ZOOMOUT_ICON,              IDI_ZOOMOUT_ICON2,          IDI_ZOOMOUT_ICON2,             IDI_ZOOMOUT_ICON_DM,           IDI_ZOOMOUT_ICON_DM,              IDI_ZOOMOUT_ICON_DM2,          IDI_ZOOMOUT_ICON_DM2,             IDR_ZOOMOUT},
    93|    93|    93|    93|
    94|    94|    94|    94|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    95|    95|    95|    95|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
    96|    96|    96|    96|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
    97|    97|    97|    97|    {IDM_VIEW_SYNSCROLLV,              IDI_SYNCV_ICON,             IDI_SYNCV_DISABLE_ICON,        IDI_SYNCV_ICON2,            IDI_SYNCV_DISABLE_ICON2,       IDI_SYNCV_ICON_DM,             IDI_SYNCV_DISABLE_ICON_DM,        IDI_SYNCV_ICON_DM2,            IDI_SYNCV_DISABLE_ICON_DM2,       IDR_SYNCV},
    98|    98|    98|    98|    {IDM_VIEW_SYNSCROLLH,              IDI_SYNCH_ICON,             IDI_SYNCH_DISABLE_ICON,        IDI_SYNCH_ICON2,            IDI_SYNCH_DISABLE_ICON2,       IDI_SYNCH_ICON_DM,             IDI_SYNCH_DISABLE_ICON_DM,        IDI_SYNCH_ICON_DM2,            IDI_SYNCH_DISABLE_ICON_DM2,       IDR_SYNCH},
    99|    99|    99|    99|
   100|   100|   100|   100|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
   101|   101|   101|   101|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
   102|   102|   102|   102|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
   103|   103|   103|   103|    {IDM_VIEW_WRAP,                    IDI_VIEW_WRAP_ICON,         IDI_VIEW_WRAP_ICON,            IDI_VIEW_WRAP_ICON2,        IDI_VIEW_WRAP_ICON2,           IDI_VIEW_WRAP_ICON_DM,         IDI_VIEW_WRAP_ICON_DM,            IDI_VIEW_WRAP_ICON_DM2,        IDI_VIEW_WRAP_ICON_DM2,           IDR_WRAP},
   104|   104|   104|   104|    {IDM_VIEW_ALL_CHARACTERS,          IDI_VIEW_ALL_CHAR_ICON,     IDI_VIEW_ALL_CHAR_ICON,        IDI_VIEW_ALL_CHAR_ICON2,    IDI_VIEW_ALL_CHAR_ICON2,       IDI_VIEW_ALL_CHAR_ICON_DM,     IDI_VIEW_ALL_CHAR_ICON_DM,        IDI_VIEW_ALL_CHAR_ICON_DM2,    IDI_VIEW_ALL_CHAR_ICON_DM2,       IDR_INVISIBLECHAR},
   105|   105|   105|   105|    {IDM_VIEW_INDENT_GUIDE,            IDI_VIEW_INDENT_ICON,       IDI_VIEW_INDENT_ICON,          IDI_VIEW_INDENT_ICON2,      IDI_VIEW_INDENT_ICON2,         IDI_VIEW_INDENT_ICON_DM,       IDI_VIEW_INDENT_ICON_DM,          IDI_VIEW_INDENT_ICON_DM2,      IDI_VIEW_INDENT_ICON_DM2,         IDR_INDENTGUIDE},
   106|   106|   106|   106|
   107|   107|   107|   107|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
   108|   108|   108|   108|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
   109|   109|   109|   109|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
   110|   110|   110|   110|
   111|   111|   111|   111|    {IDM_LANG_USER_DLG,                IDI_VIEW_UD_DLG_ICON,       IDI_VIEW_UD_DLG_ICON,          IDI_VIEW_UD_DLG_ICON2,      IDI_VIEW_UD_DLG_ICON2,         IDI_VIEW_UD_DLG_ICON_DM,       IDI_VIEW_UD_DLG_ICON_DM,          IDI_VIEW_UD_DLG_ICON_DM2,      IDI_VIEW_UD_DLG_ICON_DM2,         IDR_SHOWPANNEL},
   112|   112|   112|   112|    {IDM_VIEW_DOC_MAP,                 IDI_VIEW_DOC_MAP_ICON,      IDI_VIEW_DOC_MAP_ICON,         IDI_VIEW_DOC_MAP_ICON2,     IDI_VIEW_DOC_MAP_ICON2,        IDI_VIEW_DOC_MAP_ICON_DM,      IDI_VIEW_DOC_MAP_ICON_DM,         IDI_VIEW_DOC_MAP_ICON_DM2,     IDI_VIEW_DOC_MAP_ICON_DM2,        IDR_DOCMAP},
   113|   113|   113|   113|    {IDM_VIEW_DOCLIST,                 IDI_VIEW_DOCLIST_ICON,      IDI_VIEW_DOCLIST_ICON,         IDI_VIEW_DOCLIST_ICON2,     IDI_VIEW_DOCLIST_ICON2,        IDI_VIEW_DOCLIST_ICON_DM,      IDI_VIEW_DOCLIST_ICON_DM,         IDI_VIEW_DOCLIST_ICON_DM2,     IDI_VIEW_DOCLIST_ICON_DM2,        IDR_DOCLIST},
   114|   114|   114|   114|    {IDM_VIEW_FUNC_LIST,               IDI_VIEW_FUNCLIST_ICON,     IDI_VIEW_FUNCLIST_ICON,        IDI_VIEW_FUNCLIST_ICON2,    IDI_VIEW_FUNCLIST_ICON2,       IDI_VIEW_FUNCLIST_ICON_DM,     IDI_VIEW_FUNCLIST_ICON_DM,        IDI_VIEW_FUNCLIST_ICON_DM2,    IDI_VIEW_FUNCLIST_ICON_DM2,       IDR_FUNC_LIST},
   115|   115|   115|   115|    {IDM_VIEW_FILEBROWSER,             IDI_VIEW_FILEBROWSER_ICON,  IDI_VIEW_FILEBROWSER_ICON,     IDI_VIEW_FILEBROWSER_ICON2, IDI_VIEW_FILEBROWSER_ICON2,    IDI_VIEW_FILEBROWSER_ICON_DM,  IDI_VIEW_FILEBROWSER_ICON_DM,     IDI_VIEW_FILEBROWSER_ICON_DM2, IDI_VIEW_FILEBROWSER_ICON_DM2,    IDR_FILEBROWSER},
   116|   116|   116|   116| 
   117|   117|   117|   117|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
   118|   118|   118|   118|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
   119|   119|   119|   119|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
   120|   120|   120|   120|
   121|   121|   121|   121|    {IDM_VIEW_MONITORING,              IDI_VIEW_MONITORING_ICON,   IDI_VIEW_MONITORING_DIS_ICON,  IDI_VIEW_MONITORING_ICON2,  IDI_VIEW_MONITORING_DIS_ICON2, IDI_VIEW_MONITORING_ICON_DM,   IDI_VIEW_MONITORING_DIS_ICON_DM,  IDI_VIEW_MONITORING_ICON_DM2,  IDI_VIEW_MONITORING_DIS_ICON_DM2, IDR_FILEMONITORING},
   122|   122|   122|   122|
   123|   123|   123|   123|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
   124|   124|   124|   124|    {0,                                IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,         IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,               IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON,            IDI_SEPARATOR_ICON},
   125|   125|   125|   125|    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
   126|   126|   126|   126|
   127|   127|   127|   127|    {IDM_MACRO_STARTRECORDINGMACRO,    IDI_STARTRECORD_ICON,       IDI_STARTRECORD_DISABLE_ICON,  IDI_STARTRECORD_ICON2,      IDI_STARTRECORD_DISABLE_ICON2, IDI_STARTRECORD_ICON_DM,       IDI_STARTRECORD_DISABLE_ICON_DM,  IDI_STARTRECORD_ICON_DM2,      IDI_STARTRECORD_DISABLE_ICON_DM2, IDR_STARTRECORD},
   128|   128|   128|   128|    {IDM_MACRO_STOPRECORDINGMACRO,     IDI_STOPRECORD_ICON,        IDI_STOPRECORD_DISABLE_ICON,   IDI_STOPRECORD_ICON2,       IDI_STOPRECORD_DISABLE_ICON2,  IDI_STOPRECORD_ICON_DM,        IDI_STOPRECORD_DISABLE_ICON_DM,   IDI_STOPRECORD_ICON_DM2,       IDI_STOPRECORD_DISABLE_ICON_DM2,  IDR_STOPRECORD},
   129|   129|   129|   129|    {IDM_MACRO_PLAYBACKRECORDEDMACRO,  IDI_PLAYRECORD_ICON,        IDI_PLAYRECORD_DISABLE_ICON,   IDI_PLAYRECORD_ICON2,       IDI_PLAYRECORD_DISABLE_ICON2,  IDI_PLAYRECORD_ICON_DM,        IDI_PLAYRECORD_DISABLE_ICON_DM,   IDI_PLAYRECORD_ICON_DM2,       IDI_PLAYRECORD_DISABLE_ICON_DM2,  IDR_PLAYRECORD},
   130|   130|   130|   130|    {IDM_MACRO_RUNMULTIMACRODLG,       IDI_MMPLAY_ICON,            IDI_MMPLAY_DIS_ICON,           IDI_MMPLAY_ICON2,           IDI_MMPLAY_DIS_ICON2,          IDI_MMPLAY_ICON_DM,            IDI_MMPLAY_DIS_ICON_DM,           IDI_MMPLAY_ICON_DM2,           IDI_MMPLAY_DIS_ICON_DM2,          IDR_M_PLAYRECORD},
   131|   131|   131|   131|    {IDM_MACRO_SAVECURRENTMACRO,       IDI_SAVERECORD_ICON,        IDI_SAVERECORD_DISABLE_ICON,   IDI_SAVERECORD_ICON2,       IDI_SAVERECORD_DISABLE_ICON2,  IDI_SAVERECORD_ICON_DM,        IDI_SAVERECORD_DISABLE_ICON_DM,   IDI_SAVERECORD_ICON_DM2,       IDI_SAVERECORD_DISABLE_ICON_DM2,  IDR_SAVERECORD}
   132|   132|   132|   132|};
   133|   133|   133|   133|
   134|   134|   134|   134|
   135|   135|   135|   135|
   136|   136|   136|   136|Notepad_plus::Notepad_plus()
   137|   137|   137|   137|	: _autoCompleteMain(&_mainEditView)
   138|   138|   138|   138|	, _autoCompleteSub(&_subEditView)
   139|   139|   139|   139|	, _smartHighlighter(&_findReplaceDlg)
   140|   140|   140|   140|{
   141|   141|   141|   141|	ZeroMemory(&_prevSelectedRange, sizeof(_prevSelectedRange));
   142|   142|   142|   142|
   143|   143|   143|   143|	NppParameters& nppParam = NppParameters::getInstance();
   144|   144|   144|   144|	NppXml::Document nativeLangDocRoot = nppParam.getNativeLang();
   145|   145|   145|   145|	_nativeLangSpeaker.init(nativeLangDocRoot);
   146|   146|   146|   146|
   147|   147|   147|   147|	LocalizationSwitcher & localizationSwitcher = nppParam.getLocalizationSwitcher();
   148|   148|   148|   148|    const char *fn = _nativeLangSpeaker.getFileName();
   149|   149|   149|   149|    if (fn)
   150|   150|   150|   150|    {
   151|   151|   151|   151|        localizationSwitcher.setFileName(fn);
   152|   152|   152|   152|    }
   153|   153|   153|   153|
   154|   154|   154|   154|	nppParam.setNativeLangSpeaker(&_nativeLangSpeaker);
   155|   155|   155|   155|
   156|   156|   156|   156|	NppXml::Document toolButtonsDocRoot = nppParam.getCustomizedToolButtons();
   157|   157|   157|   157|
   158|   158|   158|   158|	if (toolButtonsDocRoot)
   159|   159|   159|   159|	{
   160|   160|   160|   160|		_toolBar.initTheme(toolButtonsDocRoot);
   161|   161|   161|   161|		_toolBar.initHideButtonsConf(toolButtonsDocRoot, toolBarIcons, sizeof(toolBarIcons) / sizeof(ToolBarButtonUnit));
   162|   162|   162|   162|	}
   163|   163|   163|   163|
   164|   164|   164|   164|	// Determine if user is administrator.
   165|   165|   165|   165|	BOOL is_admin;
   166|   166|   166|   166|	winVer ver = nppParam.getWinVersion();
   167|   167|   167|   167|	if (ver >= WV_VISTA || ver == WV_UNKNOWN)
   168|   168|   168|   168|	{
   169|   169|   169|   169|		SID_IDENTIFIER_AUTHORITY NtAuthority=***
   170|   170|   170|   170|		PSID AdministratorsGroup;
   171|   171|   171|   171|		is_admin = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
   172|   172|   172|   172|		if (is_admin)
   173|   173|   173|   173|		{
   174|   174|   174|   174|			if (!CheckTokenMembership(NULL, AdministratorsGroup, &is_admin))
   175|   175|   175|   175|				is_admin = FALSE;
   176|   176|   176|   176|			FreeSid(AdministratorsGroup);
   177|   177|   177|   177|		}
   178|   178|   178|   178|	}
   179|   179|   179|   179|	else
   180|   180|   180|   180|		is_admin = false;
   181|   181|   181|   181|
   182|   182|   182|   182|	nppParam.setAdminMode(is_admin == TRUE);
   183|   183|   183|   183|	_isAdministrator = is_admin ? true : false;
   184|   184|   184|   184|}
   185|   185|   185|   185|
   186|   186|   186|   186|Notepad_plus::~Notepad_plus()
   187|   187|   187|   187|{
   188|   188|   188|   188|	// ATTENTION : the order of the destruction is very important
   189|   189|   189|   189|	// because if the parent's window handle is destroyed before
   190|   190|   190|   190|	// the destruction of its children windows' handles,
   191|   191|   191|   191|	// its children windows' handles will be destroyed automatically!
   192|   192|   192|   192|
   193|   193|   193|   193|	(NppParameters::getInstance()).destroyInstance();
   194|   194|   194|   194|
   195|   195|   195|   195|	delete _pTrayIco;
   196|   196|   196|   196|	delete _pAnsiCharPanel;
   197|   197|   197|   197|	delete _pClipboardHistoryPanel;
   198|   198|   198|   198|	delete _pDocumentListPanel;
   199|   199|   199|   199|	delete _pProjectPanel_1;
   200|   200|   200|   200|	delete _pProjectPanel_2;
   201|   201|   201|   201|	delete _pProjectPanel_3;
   202|   202|   202|   202|	delete _pDocMap;
   203|   203|   203|   203|	delete _pFuncList;
   204|   204|   204|   204|	delete _pFileBrowser;
   205|   205|   205|   205|}
   206|   206|   206|   206|
   207|   207|   207|   207|
   208|   208|   208|   208|
   209|   209|   209|   209|LRESULT Notepad_plus::init(HWND hwnd)
   210|   210|   210|   210|{
   211|   211|   211|   211|	NppParameters& nppParam = NppParameters::getInstance();
   212|   212|   212|   212|	NppGUI & nppGUI = nppParam.getNppGUI();
   213|   213|   213|   213|	const UINT dpi = DPIManagerV2::getDpiForWindow(hwnd);
   214|   214|   214|   214|
   215|   215|   215|   215|	// Menu
   216|   216|   216|   216|	_mainMenuHandle = ::GetMenu(hwnd);
   217|   217|   217|   217|	int langPos2BeRemoved = MENUINDEX_LANGUAGE + 1;
   218|   218|   218|   218|	if (nppGUI._isLangMenuCompact)
   219|   219|   219|   219|		langPos2BeRemoved = MENUINDEX_LANGUAGE;
   220|   220|   220|   220|	::RemoveMenu(_mainMenuHandle, langPos2BeRemoved, MF_BYPOSITION);
   221|   221|   221|   221|
   222|   222|   222|   222|	//Views
   223|   223|   223|   223|	_pDocTab = &_mainDocTab;
   224|   224|   224|   224|	_pEditView = &_mainEditView;
   225|   225|   225|   225|	_pNonDocTab = &_subDocTab;
   226|   226|   226|   226|	_pNonEditView = &_subEditView;
   227|   227|   227|   227|
   228|   228|   228|   228|	_mainEditView.init(_pPublicInterface->getHinst(), hwnd);
   229|   229|   229|   229|	_subEditView.init(_pPublicInterface->getHinst(), hwnd);
   230|   230|   230|   230|
   231|   231|   231|   231|	_fileEditView.init(_pPublicInterface->getHinst(), hwnd);
   232|   232|   232|   232|	_fileEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF); // Turn off the modification event
   233|   233|   233|   233|	MainFileManager.init(this, &_fileEditView); //get it up and running asap.
   234|   234|   234|   234|
   235|   235|   235|   235|	nppParam.setFontList(hwnd);
   236|   236|   236|   236|
   237|   237|   237|   237|
   238|   238|   238|   238|	_mainWindowStatus = WindowMainActive;
   239|   239|   239|   239|	_activeView = MAIN_VIEW;
   240|   240|   240|   240|
   241|   241|   241|   241|	const ScintillaViewParams & svp = nppParam.getSVP();
   242|   242|   242|   242|
   243|   243|   243|   243|	int tabBarStatus = nppGUI._tabStatus;
   244|   244|   244|   244|
   245|   245|   245|   245|	const int tabIconSet = NppDarkMode::getTabIconSet(NppDarkMode::isEnabled());
   246|   246|   246|   246|	unsigned char indexDocTabIcon = 0;
   247|   247|   247|   247|	switch (tabIconSet)
   248|   248|   248|   248|	{
   249|   249|   249|   249|		case 0:
   250|   250|   250|   250|		{
   251|   251|   251|   251|			nppGUI._tabStatus &= ~TAB_ALTICONS;
   252|   252|   252|   252|			break;
   253|   253|   253|   253|		}
   254|   254|   254|   254|		case 1:
   255|   255|   255|   255|		{
   256|   256|   256|   256|			nppGUI._tabStatus |= TAB_ALTICONS;
   257|   257|   257|   257|			indexDocTabIcon = 1;
   258|   258|   258|   258|			break;
   259|   259|   259|   259|		}
   260|   260|   260|   260|		case 2:
   261|   261|   261|   261|		{
   262|   262|   262|   262|			nppGUI._tabStatus &= ~TAB_ALTICONS;
   263|   263|   263|   263|			indexDocTabIcon = 2;
   264|   264|   264|   264|			break;
   265|   265|   265|   265|		}
   266|   266|   266|   266|		//case -1:
   267|   267|   267|   267|		default:
   268|   268|   268|   268|		{
   269|   269|   269|   269|			indexDocTabIcon = ((tabBarStatus & TAB_ALTICONS) == TAB_ALTICONS) ? 1 : (NppDarkMode::isEnabled() ? 2 : 0);
   270|   270|   270|   270|		}
   271|   271|   271|   271|	}
   272|   272|   272|   272|
   273|   273|   273|   273|	_mainDocTab.dpiManager().setDpiWithParent(hwnd);
   274|   274|   274|   274|	_subDocTab.dpiManager().setDpiWithParent(hwnd);
   275|   275|   275|   275|
   276|   276|   276|   276|	unsigned char buttonsStatus = 0;
   277|   277|   277|   277|	buttonsStatus |= (tabBarStatus & TAB_CLOSEBUTTON) ? 1 : 0;
   278|   278|   278|   278|	buttonsStatus |= (tabBarStatus & TAB_PINBUTTON) ? 2 : 0;
   279|   279|   279|   279|
   280|   280|   280|   280|	_mainDocTab.init(_pPublicInterface->getHinst(), hwnd, &_mainEditView, indexDocTabIcon, buttonsStatus);
   281|   281|   281|   281|	_subDocTab.init(_pPublicInterface->getHinst(), hwnd, &_subEditView, indexDocTabIcon, buttonsStatus);
   282|   282|   282|   282|
   283|   283|   283|   283|	_mainEditView.display();
   284|   284|   284|   284|
   285|   285|   285|   285|	_invisibleEditView.init(_pPublicInterface->getHinst(), hwnd);
   286|   286|   286|   286|	_invisibleEditView.execute(SCI_SETUNDOCOLLECTION);
   287|   287|   287|   287|	_invisibleEditView.execute(SCI_EMPTYUNDOBUFFER);
   288|   288|   288|   288|	_invisibleEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF); // Turn off the modification event
   289|   289|   289|   289|	_invisibleEditView.wrap(false); // Make sure no slow down
   290|   290|   290|   290|
   291|   291|   291|   291|	// Configuration of 2 scintilla views
   292|   292|   292|   292|	_mainEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp._lineNumberMarginShow);
   293|   293|   293|   293|	_subEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp._lineNumberMarginShow);
   294|   294|   294|   294|	_mainEditView.showMargin(ScintillaEditView::_SC_MARGE_SYMBOL, svp._bookMarkMarginShow);
   295|   295|   295|   295|	_subEditView.showMargin(ScintillaEditView::_SC_MARGE_SYMBOL, svp._bookMarkMarginShow);
   296|   296|   296|   296|
   297|   297|   297|   297|	_mainEditView.showIndentGuideLine(svp._indentGuideLineShow);
   298|   298|   298|   298|	_subEditView.showIndentGuideLine(svp._indentGuideLineShow);
   299|   299|   299|   299|
   300|   300|   300|   300|	::SendMessage(hwnd, NPPM_INTERNAL_SETCARETWIDTH, 0, 0);
   301|   301|   301|   301|	::SendMessage(hwnd, NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);
   302|   302|   302|   302|
   303|   303|   303|   303|	_configStyleDlg.init(_pPublicInterface->getHinst(), hwnd);
   304|   304|   304|   304|	_preference.init(_pPublicInterface->getHinst(), hwnd);
   305|   305|   305|   305|	_pluginsAdminDlg.init(_pPublicInterface->getHinst(), hwnd);
   306|   306|   306|   306|
   307|   307|   307|   307|	//Marker Margin config
   308|   308|   308|   308|	_mainEditView.setMakerStyle(svp._folderStyle);
   309|   309|   309|   309|	_subEditView.setMakerStyle(svp._folderStyle);
   310|   310|   310|   310|	_mainEditView.defineDocType(_mainEditView.getCurrentBuffer()->getLangType());
   311|   311|   311|   311|	_subEditView.defineDocType(_subEditView.getCurrentBuffer()->getLangType());
   312|   312|   312|   312|
   313|   313|   313|   313|	//Line wrap method
   314|   314|   314|   314|	_mainEditView.setWrapMode(svp._lineWrapMethod);
   315|   315|   315|   315|	_subEditView.setWrapMode(svp._lineWrapMethod);
   316|   316|   316|   316|
   317|   317|   317|   317|	_mainEditView.execute(SCI_SETENDATLASTLINE, !svp._scrollBeyondLastLine);
   318|   318|   318|   318|	_subEditView.execute(SCI_SETENDATLASTLINE, !svp._scrollBeyondLastLine);
   319|   319|   319|   319|
   320|   320|   320|   320|	if (svp._doSmoothFont)
   321|   321|   321|   321|	{
   322|   322|   322|   322|		_mainEditView.execute(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);
   323|   323|   323|   323|		_subEditView.execute(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);
   324|   324|   324|   324|	}
   325|   325|   325|   325|
   326|   326|   326|   326|	_mainEditView.setBorderEdge(svp._showBorderEdge);
   327|   327|   327|   327|	_subEditView.setBorderEdge(svp._showBorderEdge);
   328|   328|   328|   328|
   329|   329|   329|   329|	_mainEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);
   330|   330|   330|   330|	_subEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);
   331|   331|   331|   331|
   332|   332|   332|   332|	_mainEditView.wrap(svp._doWrap);
   333|   333|   333|   333|	_subEditView.wrap(svp._doWrap);
   334|   334|   334|   334|
   335|   335|   335|   335|	::SendMessage(hwnd, NPPM_INTERNAL_EDGEMULTISETSIZE, 0, 0);
   336|   336|   336|   336|
   337|   337|   337|   337|	_mainEditView.showEOL(svp._eolShow);
   338|   338|   338|   338|	_subEditView.showEOL(svp._eolShow);
   339|   339|   339|   339|
   340|   340|   340|   340|	_mainEditView.showWSAndTab(svp._whiteSpaceShow);
   341|   341|   341|   341|	_subEditView.showWSAndTab(svp._whiteSpaceShow);
   342|   342|   342|   342|
   343|   343|   343|   343|	_mainEditView.showWrapSymbol(svp._wrapSymbolShow);
   344|   344|   344|   344|	_subEditView.showWrapSymbol(svp._wrapSymbolShow);
   345|   345|   345|   345|
   346|   346|   346|   346|	_mainEditView.performGlobalStyles();
   347|   347|   347|   347|	_subEditView.performGlobalStyles();
   348|   348|   348|   348|
   349|   349|   349|   349|	_zoomOriginalValue = _pEditView->execute(SCI_GETZOOM);
   350|   350|   350|   350|	_mainEditView.execute(SCI_SETZOOM, svp._zoom);
   351|   351|   351|   351|	_subEditView.execute(SCI_SETZOOM, svp._zoom2);
   352|   352|   352|   352|
   353|   353|   353|   353|	::SendMessage(hwnd, NPPM_INTERNAL_SETMULTISELECTION, 0, 0);
   354|   354|   354|   354|
   355|   355|   355|   355|	// Make backspace or delete work with multiple selections
   356|   356|   356|   356|	_mainEditView.execute(SCI_SETADDITIONALSELECTIONTYPING, true);
   357|   357|   357|   357|	_subEditView.execute(SCI_SETADDITIONALSELECTIONTYPING, true);
   358|   358|   358|   358|
   359|   359|   359|   359|	// Turn virtual space on
   360|   360|   360|   360|	int virtualSpaceOptions = SCVS_RECTANGULARSELECTION;
   361|   361|   361|   361|	if(svp._virtualSpace)
   362|   362|   362|   362|		virtualSpaceOptions |= SCVS_USERACCESSIBLE | SCVS_NOWRAPLINESTART;
   363|   363|   363|   363|
   364|   364|   364|   364|	_mainEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, virtualSpaceOptions);
   365|   365|   365|   365|	_subEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, virtualSpaceOptions);
   366|   366|   366|   366|
   367|   367|   367|   367|	// Turn multi-paste on
   368|   368|   368|   368|	_mainEditView.execute(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
   369|   369|   369|   369|	_subEditView.execute(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
   370|   370|   370|   370|
   371|   371|   371|   371|	// Turn auto-completion into each multi-select on
   372|   372|   372|   372|	_mainEditView.execute(SCI_AUTOCSETMULTI, SC_MULTIAUTOC_EACH);
   373|   373|   373|   373|	_subEditView.execute(SCI_AUTOCSETMULTI, SC_MULTIAUTOC_EACH);
   374|   374|   374|   374|
   375|   375|   375|   375|	// allow user to start selecting as a stream block, then switch to a column block by adding Alt keypress
   376|   376|   376|   376|	_mainEditView.execute(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, true);
   377|   377|   377|   377|	_subEditView.execute(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, true);
   378|   378|   378|   378|
   379|   379|   379|   379|	// Let Scintilla deal with some of the folding functionality
   380|   380|   380|   380|	_mainEditView.execute(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE);
   381|   381|   381|   381|	_subEditView.execute(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE);
   382|   382|   382|   382|
   383|   383|   383|   383|	// Set padding info
   384|   384|   384|   384|	_mainEditView.execute(SCI_SETMARGINLEFT, 0, svp._paddingLeft);
   385|   385|   385|   385|	_mainEditView.execute(SCI_SETMARGINRIGHT, 0, svp._paddingRight);
   386|   386|   386|   386|	_subEditView.execute(SCI_SETMARGINLEFT, 0, svp._paddingLeft);
   387|   387|   387|   387|	_subEditView.execute(SCI_SETMARGINRIGHT, 0, svp._paddingRight);
   388|   388|   388|   388|
   389|   389|   389|   389|	// Improvement of the switching into the wrapped long line document
   390|   390|   390|   390|	_mainEditView.execute(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT, true);
   391|   391|   391|   391|	_subEditView.execute(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT, true);
   392|   392|   392|   392|
   393|   393|   393|   393|	// Restore also the possible previous selection for each undo/redo op (memory cost min 150B for each op)
   394|   394|   394|   394|	_mainEditView.execute(SCI_SETUNDOSELECTIONHISTORY, SC_UNDO_SELECTION_HISTORY_ENABLED | SC_UNDO_SELECTION_HISTORY_SCROLL);
   395|   395|   395|   395|	_subEditView.execute(SCI_SETUNDOSELECTIONHISTORY, SC_UNDO_SELECTION_HISTORY_ENABLED | SC_UNDO_SELECTION_HISTORY_SCROLL);
   396|   396|   396|   396|
   397|   397|   397|   397|	const auto& hf = _mainDocTab.getFont(nppGUI._tabStatus & TAB_REDUCE);
   398|   398|   398|   398|	if (hf)
   399|   399|   399|   399|	{
   400|   400|   400|   400|		::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));
   401|   401|   401|   401|		::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));
   402|   402|   402|   402|	}
   403|   403|   403|   403|
   404|   404|   404|   404|	int tabDpiDynamicalHeight = _mainDocTab.dpiManager().scale(nppGUI._tabStatus & TAB_REDUCE ? g_TabHeight : g_TabHeightLarge);
   405|   405|   405|   405|	int tabDpiDynamicalWidth = _mainDocTab.dpiManager().scale(nppGUI._tabStatus & TAB_PINBUTTON ? g_TabWidthButton : g_TabWidth);
   406|   406|   406|   406|
   407|   407|   407|   407|	TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
   408|   408|   408|   408|	TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
   409|   409|   409|   409|
   410|   410|   410|   410|	_mainDocTab.display();
   411|   411|   411|   411|
   412|   412|   412|   412|	if (nppGUI._tabStatus & TAB_VERTICAL)
   413|   413|   413|   413|		TabBarPlus::doVertical();
   414|   414|   414|   414|
   415|   415|   415|   415|	TabBarPlus::triggerOwnerDrawTabbar(&(_mainDocTab.dpiManager()));
   416|   416|   416|   416|	drawTabbarColoursFromStylerArray();
   417|   417|   417|   417|
   418|   418|   418|   418|
   419|   419|   419|   419|	//
   420|   420|   420|   420|	// Initialize the default foreground & background color
   421|   421|   421|   421|	//
   422|   422|   422|   422|	const Style* pStyle = nppParam.getGlobalStylers().findByID(STYLE_DEFAULT);
   423|   423|   423|   423|	if (pStyle)
   424|   424|   424|   424|	{
   425|   425|   425|   425|		nppParam.setCurrentDefaultFgColor(pStyle->_fgColor);
   426|   426|   426|   426|		nppParam.setCurrentDefaultBgColor(pStyle->_bgColor);
   427|   427|   427|   427|		drawAutocompleteColoursFromTheme(pStyle->_fgColor, pStyle->_bgColor);
   428|   428|   428|   428|	}
   429|   429|   429|   429|	
   430|   430|   430|   430|	// Autocomplete list and calltip
   431|   431|   431|   431|	AutoCompletion::drawAutocomplete(_pEditView);
   432|   432|   432|   432|	AutoCompletion::drawAutocomplete(_pNonEditView);
   433|   433|   433|   433|
   434|   434|   434|   434|	// Document Map
   435|   435|   435|   435|	drawDocumentMapColoursFromStylerArray();
   436|   436|   436|   436|
   437|   437|   437|   437|	//--Splitter Section--//
   438|   438|   438|   438|	bool isVertical = (nppGUI._splitterPos == POS_VERTICAL);
   439|   439|   439|   439|
   440|   440|   440|   440|	const int splitterSizeDyn = DPIManagerV2::scale(splitterSize, dpi);
   441|   441|   441|   441|	_subSplitter.init(_pPublicInterface->getHinst(), hwnd);
   442|   442|   442|   442|	_subSplitter.create(&_mainDocTab, &_subDocTab, splitterSizeDyn, SplitterMode::DYNAMIC, 50, isVertical);
   443|   443|   443|   443|
   444|   444|   444|   444|	//--Status Bar Section--//
   445|   445|   445|   445|	bool willBeShown = nppGUI._statusBarShow;
   446|   446|   446|   446|	_statusBar.init(_pPublicInterface->getHinst(), hwnd, 6);
   447|   447|   447|   447|	_statusBar.setPartWidth(STATUSBAR_DOC_SIZE, DPIManagerV2::scale(220, dpi));
   448|   448|   448|   448|	_statusBar.setPartWidth(STATUSBAR_CUR_POS, DPIManagerV2::scale(260, dpi));
   449|   449|   449|   449|	_statusBar.setPartWidth(STATUSBAR_EOF_FORMAT, DPIManagerV2::scale(110, dpi));
   450|   450|   450|   450|	_statusBar.setPartWidth(STATUSBAR_UNICODE_TYPE, DPIManagerV2::scale(120, dpi));
   451|   451|   451|   451|	_statusBar.setPartWidth(STATUSBAR_TYPING_MODE, DPIManagerV2::scale(30, dpi));
   452|   452|   452|   452|	_statusBar.display(willBeShown);
   453|   453|   453|   453|
   454|   454|   454|   454|	_pMainWindow = &_mainDocTab;
   455|   455|   455|   455|
   456|   456|   456|   456|	_dockingManager.init(_pPublicInterface->getHinst(), hwnd, &_pMainWindow);
   457|   457|   457|   457|
   458|   458|   458|   458|	if (nppGUI._isMinimizedToTray != sta_none && _pTrayIco == nullptr)
   459|   459|   459|   459|	{
   460|   460|   460|   460|		HICON icon = nullptr;
   461|   461|   461|   461|		Notepad_plus_Window::loadTrayIcon(_pPublicInterface->getHinst(), &icon);
   462|   462|   462|   462|		_pTrayIco = new trayIconControler(hwnd, IDI_M30ICON, NPPM_INTERNAL_MINIMIZED_TRAY, icon, L"");
   463|   463|   463|   463|	}
   464|   464|   464|   464|
   465|   465|   465|   465|	checkSyncState();
   466|   466|   466|   466|
   467|   467|   467|   467|	// Plugin Manager
   468|   468|   468|   468|	NppData nppData;
   469|   469|   469|   469|	nppData._nppHandle = hwnd;
   470|   470|   470|   470|	nppData._scintillaMainHandle = _mainEditView.getHSelf();
   471|   471|   471|   471|	nppData._scintillaSecondHandle = _subEditView.getHSelf();
   472|   472|   472|   472|
   473|   473|   473|   473|	_scintillaCtrls4Plugins.init(_pPublicInterface->getHinst(), hwnd);
   474|   474|   474|   474|	_pluginsManager.init(nppData);
   475|   475|   475|   475|
   476|   476|   476|   476|	bool enablePluginAdmin = false; // npminmin: Plugin Admin disabled
   477|   477|   477|   477|	std::chrono::steady_clock::time_point pluginsLoadingStartTP = std::chrono::steady_clock::now();
   478|   478|   478|   478|	_pluginsManager.loadPlugins(nppParam.getPluginRootDir(), enablePluginAdmin ? &_pluginsAdminDlg.getAvailablePluginUpdateInfoList() : nullptr, enablePluginAdmin ? &_pluginsAdminDlg.getIncompatibleList() : nullptr);
   479|   479|   479|   479|	g_pluginsLoadingTime = std::chrono::steady_clock::now() - pluginsLoadingStartTP;
   480|   480|   480|   480|	_restoreButton.init(_pPublicInterface->getHinst(), hwnd);
   481|   481|   481|   481|
   482|   482|   482|   482|	// ------------ //
   483|   483|   483|   483|	// Menu Section //
   484|   484|   484|   484|	// ------------ //
   485|   485|   485|   485|	nppParam.initTabCustomColors();
   486|   486|   486|   486|	nppParam.initFindDlgStatusMsgCustomColors();
   487|   487|   487|   487|	setupColorSampleBitmapsOnMainMenuItems();
   488|   488|   488|   488|
   489|   489|   489|   489|	// Macro Menu
   490|   490|   490|   490|	HMENU hMacroMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_MACRO);
   491|   491|   491|   491|	size_t const macroPosBase = 6;
   492|   492|   492|   492|	DynamicMenu& macroMenuItems = nppParam.getMacroMenuItems();
   493|   493|   493|   493|	size_t nbMacroTopLevelItem = macroMenuItems.getTopLevelItemNumber();
   494|   494|   494|   494|	if (nbMacroTopLevelItem >= 1)
   495|   495|   495|   495|		::InsertMenu(hMacroMenu, macroPosBase - 1, MF_BYPOSITION, static_cast<UINT>(-1), 0);
   496|   496|   496|   496|
   497|   497|   497|   497|	macroMenuItems.attach(hMacroMenu, macroPosBase, IDM_SETTING_SHORTCUT_MAPPER_MACRO, L"Modify Shortcut/Delete Macro...");
   498|   498|   498|   498|
   499|   499|   499|   499|
   500|   500|   500|   500|	// Run Menu
   501|