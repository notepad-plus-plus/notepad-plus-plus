// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXRES_H__
#define __AFXRES_H__

#ifdef RC_INVOKED
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
   #include "winres.h"           // extract from windows header
#endif
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

#ifdef APSTUDIO_INVOKED
#define APSTUDIO_HIDDEN_SYMBOLS
#endif

/////////////////////////////////////////////////////////////////////////////
// MFC resource types (see Technical note TN024 for implementation details)

#ifndef RC_INVOKED
#define RT_DLGINIT  MAKEINTRESOURCE(240)
#define RT_TOOLBAR  MAKEINTRESOURCE(241)
#endif

/////////////////////////////////////////////////////////////////////////////

#ifdef APSTUDIO_INVOKED
#undef APSTUDIO_HIDDEN_SYMBOLS
#endif

/////////////////////////////////////////////////////////////////////////////
// General style bits etc

// ControlBar styles
#define CBRS_ALIGN_LEFT     0x1000L
#define CBRS_ALIGN_TOP      0x2000L
#define CBRS_ALIGN_RIGHT    0x4000L
#define CBRS_ALIGN_BOTTOM   0x8000L
#define CBRS_ALIGN_ANY      0xF000L

#define CBRS_BORDER_LEFT    0x0100L
#define CBRS_BORDER_TOP     0x0200L
#define CBRS_BORDER_RIGHT   0x0400L
#define CBRS_BORDER_BOTTOM  0x0800L
#define CBRS_BORDER_ANY     0x0F00L

#define CBRS_TOOLTIPS       0x0010L
#define CBRS_FLYBY          0x0020L
#define CBRS_FLOAT_MULTI    0x0040L
#define CBRS_BORDER_3D      0x0080L
#define CBRS_HIDE_INPLACE   0x0008L
#define CBRS_SIZE_DYNAMIC   0x0004L
#define CBRS_SIZE_FIXED     0x0002L
#define CBRS_FLOATING       0x0001L

#define CBRS_GRIPPER        0x00400000L

#define CBRS_ORIENT_HORZ    (CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM)
#define CBRS_ORIENT_VERT    (CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT)
#define CBRS_ORIENT_ANY     (CBRS_ORIENT_HORZ|CBRS_ORIENT_VERT)

#define CBRS_ALL            0x0040FFFFL

// the CBRS_ style is made up of an alignment style and a draw border style
//  the alignment styles are mutually exclusive
//  the draw border styles may be combined
#define CBRS_NOALIGN        0x00000000L
#define CBRS_LEFT           (CBRS_ALIGN_LEFT|CBRS_BORDER_RIGHT)
#define CBRS_TOP            (CBRS_ALIGN_TOP|CBRS_BORDER_BOTTOM)
#define CBRS_RIGHT          (CBRS_ALIGN_RIGHT|CBRS_BORDER_LEFT)
#define CBRS_BOTTOM         (CBRS_ALIGN_BOTTOM|CBRS_BORDER_TOP)

/////////////////////////////////////////////////////////////////////////////
// Standard window components

// Mode indicators in status bar - these are routed like commands
#define ID_INDICATOR_EXT                0xE700  // extended selection indicator
#define ID_INDICATOR_CAPS               0xE701  // cap lock indicator
#define ID_INDICATOR_NUM                0xE702  // num lock indicator
#define ID_INDICATOR_SCRL               0xE703  // scroll lock indicator
#define ID_INDICATOR_OVR                0xE704  // overtype mode indicator
#define ID_INDICATOR_REC                0xE705  // record mode indicator
#define ID_INDICATOR_KANA               0xE706  // kana lock indicator

#define ID_SEPARATOR                    0   // special separator value

#ifndef RC_INVOKED  // code only
// Standard control bars (IDW = window ID)
#define AFX_IDW_CONTROLBAR_FIRST        0xE800
#define AFX_IDW_CONTROLBAR_LAST         0xE8FF

#define AFX_IDW_TOOLBAR                 0xE800  // main Toolbar for window
#define AFX_IDW_STATUS_BAR              0xE801  // Status bar window
#define AFX_IDW_PREVIEW_BAR             0xE802  // PrintPreview Dialog Bar
#define AFX_IDW_RESIZE_BAR              0xE803  // OLE in-place resize bar
#define AFX_IDW_REBAR                   0xE804  // COMCTL32 "rebar" Bar
#define AFX_IDW_DIALOGBAR               0xE805  // CDialogBar

// Note: If your application supports docking toolbars, you should
//  not use the following IDs for your own toolbars.  The IDs chosen
//  are at the top of the first 32 such that the bars will be hidden
//  while in print preview mode, and are not likely to conflict with
//  IDs your application may have used succesfully in the past.

#define AFX_IDW_DOCKBAR_TOP             0xE81B
#define AFX_IDW_DOCKBAR_LEFT            0xE81C
#define AFX_IDW_DOCKBAR_RIGHT           0xE81D
#define AFX_IDW_DOCKBAR_BOTTOM          0xE81E
#define AFX_IDW_DOCKBAR_FLOAT           0xE81F

// Macro for mapping standard control bars to bitmask (limit of 32)
#define AFX_CONTROLBAR_MASK(nIDC)   (1L << (nIDC - AFX_IDW_CONTROLBAR_FIRST))

// parts of Main Frame
#define AFX_IDW_PANE_FIRST              0xE900  // first pane (256 max)
#define AFX_IDW_PANE_LAST               0xE9ff
#define AFX_IDW_HSCROLL_FIRST           0xEA00  // first Horz scrollbar (16 max)
#define AFX_IDW_VSCROLL_FIRST           0xEA10  // first Vert scrollbar (16 max)

#define AFX_IDW_SIZE_BOX                0xEA20  // size box for splitters
#define AFX_IDW_PANE_SAVE               0xEA21  // to shift AFX_IDW_PANE_FIRST
#endif //!RC_INVOKED

#ifndef APSTUDIO_INVOKED

// common style for form views
#define AFX_WS_DEFAULT_VIEW             (WS_CHILD | WS_VISIBLE | WS_BORDER)

#endif //!APSTUDIO_INVOKED

/////////////////////////////////////////////////////////////////////////////
// Standard app configurable strings

// for application title (defaults to EXE name or name in constructor)
#define AFX_IDS_APP_TITLE               0xE000
// idle message bar line
#define AFX_IDS_IDLEMESSAGE             0xE001
// message bar line when in shift-F1 help mode
#define AFX_IDS_HELPMODEMESSAGE         0xE002
// document title when editing OLE embedding
#define AFX_IDS_APP_TITLE_EMBEDDING     0xE003
// company name
#define AFX_IDS_COMPANY_NAME            0xE004
// object name when server is inplace
#define AFX_IDS_OBJ_TITLE_INPLACE       0xE005

/////////////////////////////////////////////////////////////////////////////
// Standard Commands

// File commands
#define ID_FILE_NEW                     0xE100
#define ID_FILE_OPEN                    0xE101
#define ID_FILE_CLOSE                   0xE102
#define ID_FILE_SAVE                    0xE103
#define ID_FILE_SAVE_AS                 0xE104
#define ID_FILE_PAGE_SETUP              0xE105
#define ID_FILE_PRINT_SETUP             0xE106
#define ID_FILE_PRINT                   0xE107
#define ID_FILE_PRINT_DIRECT            0xE108
#define ID_FILE_PRINT_PREVIEW           0xE109
#define ID_FILE_UPDATE                  0xE10A
#define ID_FILE_SAVE_COPY_AS            0xE10B
#define ID_FILE_SEND_MAIL               0xE10C
#define ID_FILE_NEW_FRAME               0xE10D

#define ID_FILE_MRU_FIRST               0xE110
#define ID_FILE_MRU_FILE1               0xE110          // range - 16 max
#define ID_FILE_MRU_FILE2               0xE111
#define ID_FILE_MRU_FILE3               0xE112
#define ID_FILE_MRU_FILE4               0xE113
#define ID_FILE_MRU_FILE5               0xE114
#define ID_FILE_MRU_FILE6               0xE115
#define ID_FILE_MRU_FILE7               0xE116
#define ID_FILE_MRU_FILE8               0xE117
#define ID_FILE_MRU_FILE9               0xE118
#define ID_FILE_MRU_FILE10              0xE119
#define ID_FILE_MRU_FILE11              0xE11A
#define ID_FILE_MRU_FILE12              0xE11B
#define ID_FILE_MRU_FILE13              0xE11C
#define ID_FILE_MRU_FILE14              0xE11D
#define ID_FILE_MRU_FILE15              0xE11E
#define ID_FILE_MRU_FILE16              0xE11F
#define ID_FILE_MRU_LAST                0xE11F

// Edit commands
#define ID_EDIT_CLEAR                   0xE120
#define ID_EDIT_CLEAR_ALL               0xE121
#define ID_EDIT_COPY                    0xE122
#define ID_EDIT_CUT                     0xE123
#define ID_EDIT_FIND                    0xE124
#define ID_EDIT_PASTE                   0xE125
#define ID_EDIT_PASTE_LINK              0xE126
#define ID_EDIT_PASTE_SPECIAL           0xE127
#define ID_EDIT_REPEAT                  0xE128
#define ID_EDIT_REPLACE                 0xE129
#define ID_EDIT_SELECT_ALL              0xE12A
#define ID_EDIT_UNDO                    0xE12B
#define ID_EDIT_REDO                    0xE12C

// Window commands
#define ID_WINDOW_NEW                   0xE130
#define ID_WINDOW_ARRANGE               0xE131
#define ID_WINDOW_CASCADE               0xE132
#define ID_WINDOW_TILE_HORZ             0xE133
#define ID_WINDOW_TILE_VERT             0xE134
#define ID_WINDOW_SPLIT                 0xE135
#ifndef RC_INVOKED      // code only
#define AFX_IDM_WINDOW_FIRST            0xE130
#define AFX_IDM_WINDOW_LAST             0xE13F
#define AFX_IDM_FIRST_MDICHILD          0xFF00  // window list starts here
#endif //!RC_INVOKED

// Help and App commands
#define ID_APP_ABOUT                    0xE140
#define ID_APP_EXIT                     0xE141
#define ID_HELP_INDEX                   0xE142
#define ID_HELP_FINDER                  0xE143
#define ID_HELP_USING                   0xE144
#define ID_CONTEXT_HELP                 0xE145      // shift-F1
// special commands for processing help
#define ID_HELP                         0xE146      // first attempt for F1
#define ID_DEFAULT_HELP                 0xE147      // last attempt

// Misc
#define ID_NEXT_PANE                    0xE150
#define ID_PREV_PANE                    0xE151

// Format
#define ID_FORMAT_FONT                  0xE160

// OLE commands
#define ID_OLE_INSERT_NEW               0xE200
#define ID_OLE_EDIT_LINKS               0xE201
#define ID_OLE_EDIT_CONVERT             0xE202
#define ID_OLE_EDIT_CHANGE_ICON         0xE203
#define ID_OLE_EDIT_PROPERTIES          0xE204
#define ID_OLE_VERB_FIRST               0xE210     // range - 16 max
#ifndef RC_INVOKED      // code only
#define ID_OLE_VERB_LAST                0xE21F
#endif //!RC_INVOKED

// for print preview dialog bar
#define AFX_ID_PREVIEW_CLOSE            0xE300
#define AFX_ID_PREVIEW_NUMPAGE          0xE301      // One/Two Page button
#define AFX_ID_PREVIEW_NEXT             0xE302
#define AFX_ID_PREVIEW_PREV             0xE303
#define AFX_ID_PREVIEW_PRINT            0xE304
#define AFX_ID_PREVIEW_ZOOMIN           0xE305
#define AFX_ID_PREVIEW_ZOOMOUT          0xE306

// View commands (same number used as IDW used for control bar)
#define ID_VIEW_TOOLBAR                 0xE800
#define ID_VIEW_STATUS_BAR              0xE801
#define ID_VIEW_REBAR                   0xE804
#define ID_VIEW_AUTOARRANGE         0xE805
   // E810 -> E81F must be kept in order for RANGE macros
#define ID_VIEW_SMALLICON               0xE810
#define ID_VIEW_LARGEICON               0xE811
#define ID_VIEW_LIST                   0xE812
#define ID_VIEW_DETAILS                 0xE813
#define ID_VIEW_LINEUP                  0xE814
#define ID_VIEW_BYNAME                  0xE815
#define AFX_ID_VIEW_MINIMUM              ID_VIEW_SMALLICON
#define AFX_ID_VIEW_MAXIMUM              ID_VIEW_BYNAME
   // E800 -> E8FF reserved for other control bar commands

// RecordForm commands
#define ID_RECORD_FIRST                 0xE900
#define ID_RECORD_LAST                  0xE901
#define ID_RECORD_NEXT                  0xE902
#define ID_RECORD_PREV                  0xE903

/////////////////////////////////////////////////////////////////////////////
// Standard control IDs

#ifdef IDC_STATIC
#undef IDC_STATIC
#endif
#define IDC_STATIC              (-1)     // all static controls

/////////////////////////////////////////////////////////////////////////////
// Standard string error/warnings

#ifndef RC_INVOKED      // code only
#define AFX_IDS_SCFIRST                 0xEF00
#endif //!RC_INVOKED

#define AFX_IDS_SCSIZE                  0xEF00
#define AFX_IDS_SCMOVE                  0xEF01
#define AFX_IDS_SCMINIMIZE              0xEF02
#define AFX_IDS_SCMAXIMIZE              0xEF03
#define AFX_IDS_SCNEXTWINDOW            0xEF04
#define AFX_IDS_SCPREVWINDOW            0xEF05
#define AFX_IDS_SCCLOSE                 0xEF06
#define AFX_IDS_SCRESTORE               0xEF12
#define AFX_IDS_SCTASKLIST              0xEF13

#define AFX_IDS_MDICHILD                0xEF1F

#define AFX_IDS_DESKACCESSORY           0xEFDA

// General strings
#define AFX_IDS_OPENFILE                0xF000
#define AFX_IDS_SAVEFILE                0xF001
#define AFX_IDS_ALLFILTER               0xF002
#define AFX_IDS_UNTITLED                0xF003
#define AFX_IDS_SAVEFILECOPY            0xF004
#define AFX_IDS_PREVIEW_CLOSE           0xF005
#define AFX_IDS_UNNAMED_FILE            0xF006
#define AFX_IDS_HIDE                    0xF011

// MFC Standard Exception Error messages
#define AFX_IDP_NO_ERROR_AVAILABLE      0xF020
#define AFX_IDS_NOT_SUPPORTED_EXCEPTION 0xF021
#define AFX_IDS_RESOURCE_EXCEPTION      0xF022
#define AFX_IDS_MEMORY_EXCEPTION        0xF023
#define AFX_IDS_USER_EXCEPTION          0xF024
#define AFX_IDS_INVALID_ARG_EXCEPTION   0xF025

// Printing and print preview strings
#define AFX_IDS_PRINTONPORT             0xF040
#define AFX_IDS_ONEPAGE                 0xF041
#define AFX_IDS_TWOPAGE                 0xF042
#define AFX_IDS_PRINTPAGENUM            0xF043
#define AFX_IDS_PREVIEWPAGEDESC         0xF044
#define AFX_IDS_PRINTDEFAULTEXT         0xF045
#define AFX_IDS_PRINTDEFAULT            0xF046
#define AFX_IDS_PRINTFILTER             0xF047
#define AFX_IDS_PRINTCAPTION            0xF048
#define AFX_IDS_PRINTTOFILE             0xF049


// OLE strings
#define AFX_IDS_OBJECT_MENUITEM         0xF080
#define AFX_IDS_EDIT_VERB               0xF081
#define AFX_IDS_ACTIVATE_VERB           0xF082
#define AFX_IDS_CHANGE_LINK             0xF083
#define AFX_IDS_AUTO                    0xF084
#define AFX_IDS_MANUAL                  0xF085
#define AFX_IDS_FROZEN                  0xF086
#define AFX_IDS_ALL_FILES               0xF087
// dynamically changing menu items
#define AFX_IDS_SAVE_MENU               0xF088
#define AFX_IDS_UPDATE_MENU             0xF089
#define AFX_IDS_SAVE_AS_MENU            0xF08A
#define AFX_IDS_SAVE_COPY_AS_MENU       0xF08B
#define AFX_IDS_EXIT_MENU               0xF08C
#define AFX_IDS_UPDATING_ITEMS          0xF08D
// COlePasteSpecialDialog defines
#define AFX_IDS_METAFILE_FORMAT         0xF08E
#define AFX_IDS_DIB_FORMAT              0xF08F
#define AFX_IDS_BITMAP_FORMAT           0xF090
#define AFX_IDS_LINKSOURCE_FORMAT       0xF091
#define AFX_IDS_EMBED_FORMAT            0xF092
// other OLE utility strings
#define AFX_IDS_PASTELINKEDTYPE         0xF094
#define AFX_IDS_UNKNOWNTYPE             0xF095
#define AFX_IDS_RTF_FORMAT              0xF096
#define AFX_IDS_TEXT_FORMAT             0xF097
// OLE datatype format error strings
#define AFX_IDS_INVALID_CURRENCY        0xF098
#define AFX_IDS_INVALID_DATETIME        0xF099
#define AFX_IDS_INVALID_DATETIMESPAN    0xF09A

// General error / prompt strings
#define AFX_IDP_INVALID_FILENAME        0xF100
#define AFX_IDP_FAILED_TO_OPEN_DOC      0xF101
#define AFX_IDP_FAILED_TO_SAVE_DOC      0xF102
#define AFX_IDP_ASK_TO_SAVE             0xF103
#define AFX_IDP_FAILED_TO_CREATE_DOC    0xF104
#define AFX_IDP_FILE_TOO_LARGE          0xF105
#define AFX_IDP_FAILED_TO_START_PRINT   0xF106
#define AFX_IDP_FAILED_TO_LAUNCH_HELP   0xF107
#define AFX_IDP_INTERNAL_FAILURE        0xF108      // general failure
#define AFX_IDP_COMMAND_FAILURE         0xF109      // command failure
#define AFX_IDP_FAILED_MEMORY_ALLOC     0xF10A
#define AFX_IDP_UNREG_DONE              0xF10B
#define AFX_IDP_UNREG_FAILURE           0xF10C
#define AFX_IDP_DLL_LOAD_FAILED         0xF10D
#define AFX_IDP_DLL_BAD_VERSION         0xF10E

// DDV parse errors
#define AFX_IDP_PARSE_INT               0xF110
#define AFX_IDP_PARSE_REAL              0xF111
#define AFX_IDP_PARSE_INT_RANGE         0xF112
#define AFX_IDP_PARSE_REAL_RANGE        0xF113
#define AFX_IDP_PARSE_STRING_SIZE       0xF114
#define AFX_IDP_PARSE_RADIO_BUTTON      0xF115
#define AFX_IDP_PARSE_BYTE              0xF116
#define AFX_IDP_PARSE_UINT              0xF117
#define AFX_IDP_PARSE_DATETIME          0xF118
#define AFX_IDP_PARSE_CURRENCY          0xF119
#define AFX_IDP_PARSE_GUID              0xF11A
#define AFX_IDP_PARSE_TIME              0xF11B
#define AFX_IDP_PARSE_DATE              0xF11C

// CFile/CArchive error strings for user failure
#define AFX_IDP_FAILED_INVALID_FORMAT   0xF120
#define AFX_IDP_FAILED_INVALID_PATH     0xF121
#define AFX_IDP_FAILED_DISK_FULL        0xF122
#define AFX_IDP_FAILED_ACCESS_READ      0xF123
#define AFX_IDP_FAILED_ACCESS_WRITE     0xF124
#define AFX_IDP_FAILED_IO_ERROR_READ    0xF125
#define AFX_IDP_FAILED_IO_ERROR_WRITE   0xF126

// Script errors / prompt strings
#define AFX_IDP_SCRIPT_ERROR            0xF130
#define AFX_IDP_SCRIPT_DISPATCH_EXCEPTION 0xF131

// OLE errors / prompt strings
#define AFX_IDP_STATIC_OBJECT           0xF180
#define AFX_IDP_FAILED_TO_CONNECT       0xF181
#define AFX_IDP_SERVER_BUSY             0xF182
#define AFX_IDP_BAD_VERB                0xF183
#define AFX_IDS_NOT_DOCOBJECT           0xF184
#define AFX_IDP_FAILED_TO_NOTIFY        0xF185
#define AFX_IDP_FAILED_TO_LAUNCH        0xF186
#define AFX_IDP_ASK_TO_UPDATE           0xF187
#define AFX_IDP_FAILED_TO_UPDATE        0xF188
#define AFX_IDP_FAILED_TO_REGISTER      0xF189
#define AFX_IDP_FAILED_TO_AUTO_REGISTER 0xF18A
#define AFX_IDP_FAILED_TO_CONVERT       0xF18B
#define AFX_IDP_GET_NOT_SUPPORTED       0xF18C
#define AFX_IDP_SET_NOT_SUPPORTED       0xF18D
#define AFX_IDP_ASK_TO_DISCARD          0xF18E
#define AFX_IDP_FAILED_TO_CREATE        0xF18F

// MAPI errors / prompt strings
#define AFX_IDP_FAILED_MAPI_LOAD        0xF190
#define AFX_IDP_INVALID_MAPI_DLL        0xF191
#define AFX_IDP_FAILED_MAPI_SEND        0xF192

#define AFX_IDP_FILE_NONE               0xF1A0
#define AFX_IDP_FILE_GENERIC            0xF1A1
#define AFX_IDP_FILE_NOT_FOUND          0xF1A2
#define AFX_IDP_FILE_BAD_PATH           0xF1A3
#define AFX_IDP_FILE_TOO_MANY_OPEN      0xF1A4
#define AFX_IDP_FILE_ACCESS_DENIED      0xF1A5
#define AFX_IDP_FILE_INVALID_FILE       0xF1A6
#define AFX_IDP_FILE_REMOVE_CURRENT     0xF1A7
#define AFX_IDP_FILE_DIR_FULL           0xF1A8
#define AFX_IDP_FILE_BAD_SEEK           0xF1A9
#define AFX_IDP_FILE_HARD_IO            0xF1AA
#define AFX_IDP_FILE_SHARING            0xF1AB
#define AFX_IDP_FILE_LOCKING            0xF1AC
#define AFX_IDP_FILE_DISKFULL           0xF1AD
#define AFX_IDP_FILE_EOF                0xF1AE

#define AFX_IDP_ARCH_NONE               0xF1B0
#define AFX_IDP_ARCH_GENERIC            0xF1B1
#define AFX_IDP_ARCH_READONLY           0xF1B2
#define AFX_IDP_ARCH_ENDOFFILE          0xF1B3
#define AFX_IDP_ARCH_WRITEONLY          0xF1B4
#define AFX_IDP_ARCH_BADINDEX           0xF1B5
#define AFX_IDP_ARCH_BADCLASS           0xF1B6
#define AFX_IDP_ARCH_BADSCHEMA          0xF1B7

#define AFX_IDS_OCC_SCALEUNITS_PIXELS   0xF1C0

// 0xf200-0xf20f reserved

// font names and point sizes
#define AFX_IDS_STATUS_FONT             0xF230
#define AFX_IDS_TOOLTIP_FONT            0xF231
#define AFX_IDS_UNICODE_FONT            0xF232
#define AFX_IDS_MINI_FONT               0xF233

// ODBC Database errors / prompt strings
#ifndef RC_INVOKED      // code only
#define AFX_IDP_SQL_FIRST                       0xF280
#endif //!RC_INVOKED
#define AFX_IDP_SQL_CONNECT_FAIL                0xF281
#define AFX_IDP_SQL_RECORDSET_FORWARD_ONLY      0xF282
#define AFX_IDP_SQL_EMPTY_COLUMN_LIST           0xF283
#define AFX_IDP_SQL_FIELD_SCHEMA_MISMATCH       0xF284
#define AFX_IDP_SQL_ILLEGAL_MODE                0xF285
#define AFX_IDP_SQL_MULTIPLE_ROWS_AFFECTED      0xF286
#define AFX_IDP_SQL_NO_CURRENT_RECORD           0xF287
#define AFX_IDP_SQL_NO_ROWS_AFFECTED            0xF288
#define AFX_IDP_SQL_RECORDSET_READONLY          0xF289
#define AFX_IDP_SQL_SQL_NO_TOTAL                0xF28A
#define AFX_IDP_SQL_ODBC_LOAD_FAILED            0xF28B
#define AFX_IDP_SQL_DYNASET_NOT_SUPPORTED       0xF28C
#define AFX_IDP_SQL_SNAPSHOT_NOT_SUPPORTED      0xF28D
#define AFX_IDP_SQL_API_CONFORMANCE             0xF28E
#define AFX_IDP_SQL_SQL_CONFORMANCE             0xF28F
#define AFX_IDP_SQL_NO_DATA_FOUND               0xF290
#define AFX_IDP_SQL_ROW_UPDATE_NOT_SUPPORTED    0xF291
#define AFX_IDP_SQL_ODBC_V2_REQUIRED            0xF292
#define AFX_IDP_SQL_NO_POSITIONED_UPDATES       0xF293
#define AFX_IDP_SQL_LOCK_MODE_NOT_SUPPORTED     0xF294
#define AFX_IDP_SQL_DATA_TRUNCATED              0xF295
#define AFX_IDP_SQL_ROW_FETCH                   0xF296
#define AFX_IDP_SQL_INCORRECT_ODBC              0xF297
#define AFX_IDP_SQL_UPDATE_DELETE_FAILED        0xF298
#define AFX_IDP_SQL_DYNAMIC_CURSOR_NOT_SUPPORTED    0xF299
#define AFX_IDP_SQL_FIELD_NOT_FOUND             0xF29A
#define AFX_IDP_SQL_BOOKMARKS_NOT_SUPPORTED     0xF29B
#define AFX_IDP_SQL_BOOKMARKS_NOT_ENABLED       0xF29C

// ODBC Database strings
#define AFX_IDS_DELETED                   0xF29D

// DAO Database errors / prompt strings
#ifndef RC_INVOKED      // code only
#define AFX_IDP_DAO_FIRST                       0xF2B0
#endif //!RC_INVOKED
#define AFX_IDP_DAO_ENGINE_INITIALIZATION       0xF2B0
#define AFX_IDP_DAO_DFX_BIND                    0xF2B1
#define AFX_IDP_DAO_OBJECT_NOT_OPEN             0xF2B2

// ICDAORecordset::GetRows Errors
//  These are not placed in DAO Errors collection
//  and must be handled directly by MFC.
#define AFX_IDP_DAO_ROWTOOSHORT                 0xF2B3
#define AFX_IDP_DAO_BADBINDINFO                 0xF2B4
#define AFX_IDP_DAO_COLUMNUNAVAILABLE           0xF2B5

/////////////////////////////////////////////////////////////////////////////
// Strings for ISAPI support

#define AFX_IDS_HTTP_TITLE              0xF2D1
#define AFX_IDS_HTTP_NO_TEXT            0xF2D2
#define AFX_IDS_HTTP_BAD_REQUEST        0xF2D3
#define AFX_IDS_HTTP_AUTH_REQUIRED      0xF2D4
#define AFX_IDS_HTTP_FORBIDDEN          0xF2D5
#define AFX_IDS_HTTP_NOT_FOUND          0xF2D6
#define AFX_IDS_HTTP_SERVER_ERROR       0xF2D7
#define AFX_IDS_HTTP_NOT_IMPLEMENTED    0xF2D8

/////////////////////////////////////////////////////////////////////////////
// Strings for Accessibility support for CCheckListBox
#define AFX_IDS_CHECKLISTBOX_UNCHECK	0xF2E1
#define AFX_IDS_CHECKLISTBOX_CHECK		0xF2E2
#define AFX_IDS_CHECKLISTBOX_MIXED		0xF2E3

/////////////////////////////////////////////////////////////////////////////
// AFX implementation - control IDs (AFX_IDC)

// Parts of dialogs
#define AFX_IDC_LISTBOX                 100
#define AFX_IDC_CHANGE                  101
#define AFX_IDC_BROWSER             102

// for print dialog
#define AFX_IDC_PRINT_DOCNAME           201
#define AFX_IDC_PRINT_PRINTERNAME       202
#define AFX_IDC_PRINT_PORTNAME          203
#define AFX_IDC_PRINT_PAGENUM           204

// Property Sheet control id's (determined with Spy++)
#define ID_APPLY_NOW                    0x3021
#define ID_WIZBACK                      0x3023
#define ID_WIZNEXT                      0x3024
#define ID_WIZFINISH                    0x3025
#define AFX_IDC_TAB_CONTROL             0x3020

/////////////////////////////////////////////////////////////////////////////
// IDRs for standard components

#ifndef RC_INVOKED  // code only
// These are really COMMDLG dialogs, so there usually isn't a resource
// for them, but these IDs are used as help IDs.
#define AFX_IDD_FILEOPEN                28676
#define AFX_IDD_FILESAVE                28677
#define AFX_IDD_FONT                    28678
#define AFX_IDD_COLOR                   28679
#define AFX_IDD_PRINT                   28680
#define AFX_IDD_PRINTSETUP              28681
#define AFX_IDD_FIND                    28682
#define AFX_IDD_REPLACE                 28683
#endif //!RC_INVOKED

// Standard dialogs app should leave alone (0x7801->)
#define AFX_IDD_NEWTYPEDLG              30721
#define AFX_IDD_PRINTDLG                30722
#define AFX_IDD_PREVIEW_TOOLBAR         30723

// Dialogs defined for OLE2UI library
#define AFX_IDD_INSERTOBJECT            30724
#define AFX_IDD_CHANGEICON              30725
#define AFX_IDD_CONVERT                 30726
#define AFX_IDD_PASTESPECIAL            30727
#define AFX_IDD_EDITLINKS               30728
#define AFX_IDD_FILEBROWSE              30729
#define AFX_IDD_BUSY                    30730

#define AFX_IDD_OBJECTPROPERTIES        30732
#define AFX_IDD_CHANGESOURCE            30733

// Standard cursors (0x7901->)
   // AFX_IDC = Cursor resources
#define AFX_IDC_CONTEXTHELP             30977       // context sensitive help
#define AFX_IDC_MAGNIFY                 30978       // print preview zoom
#define AFX_IDC_SMALLARROWS             30979       // splitter
#define AFX_IDC_HSPLITBAR               30980       // splitter
#define AFX_IDC_VSPLITBAR               30981       // splitter
#define AFX_IDC_NODROPCRSR              30982       // No Drop Cursor
#define AFX_IDC_TRACKNWSE               30983       // tracker
#define AFX_IDC_TRACKNESW               30984       // tracker
#define AFX_IDC_TRACKNS                 30985       // tracker
#define AFX_IDC_TRACKWE                 30986       // tracker
#define AFX_IDC_TRACK4WAY               30987       // tracker
#define AFX_IDC_MOVE4WAY                30988       // resize bar (server only)

// Wheel mouse cursors
// NOTE: values must be in this order!  See CScrollView::OnTimer()
#define AFX_IDC_MOUSE_PAN_NW            30998       // pan east
#define AFX_IDC_MOUSE_PAN_N             30999       // pan northeast
#define AFX_IDC_MOUSE_PAN_NE            31000       // pan north
#define AFX_IDC_MOUSE_PAN_W             31001       // pan northwest
#define AFX_IDC_MOUSE_PAN_HV            31002       // pan both axis
#define AFX_IDC_MOUSE_PAN_E             31003       // pan west
#define AFX_IDC_MOUSE_PAN_SW            31004       // pan south-west
#define AFX_IDC_MOUSE_PAN_S             31005       // pan south
#define AFX_IDC_MOUSE_PAN_SE            31006       // pan south-east
#define AFX_IDC_MOUSE_PAN_HORZ          31007       // pan X-axis
#define AFX_IDC_MOUSE_PAN_VERT          31008       // pan Y-axis

// Wheel mouse bitmaps
#define AFX_IDC_MOUSE_ORG_HORZ          31009       // anchor for horz only
#define AFX_IDC_MOUSE_ORG_VERT          31010       // anchor for vert only
#define AFX_IDC_MOUSE_ORG_HV            31011       // anchor for horz/vert
#define AFX_IDC_MOUSE_MASK              31012

// Mini frame window bitmap ID
#define AFX_IDB_MINIFRAME_MENU          30994

// CheckListBox checks bitmap ID
#define AFX_IDB_CHECKLISTBOX_95         30996

// AFX standard accelerator resources
#define AFX_IDR_PREVIEW_ACCEL           30997

// AFX standard ICON IDs (for MFC V1 apps) (0x7A01->)
#define AFX_IDI_STD_MDIFRAME            31233
#define AFX_IDI_STD_FRAME               31234

/////////////////////////////////////////////////////////////////////////////
// AFX OLE control implementation - control IDs (AFX_IDC)

// Font property page
#define AFX_IDC_FONTPROP                1000
#define AFX_IDC_FONTNAMES               1001
#define AFX_IDC_FONTSTYLES              1002
#define AFX_IDC_FONTSIZES               1003
#define AFX_IDC_STRIKEOUT               1004
#define AFX_IDC_UNDERLINE               1005
#define AFX_IDC_SAMPLEBOX               1006

// Color property page
#define AFX_IDC_COLOR_BLACK             1100
#define AFX_IDC_COLOR_WHITE             1101
#define AFX_IDC_COLOR_RED               1102
#define AFX_IDC_COLOR_GREEN             1103
#define AFX_IDC_COLOR_BLUE              1104
#define AFX_IDC_COLOR_YELLOW            1105
#define AFX_IDC_COLOR_MAGENTA           1106
#define AFX_IDC_COLOR_CYAN              1107
#define AFX_IDC_COLOR_GRAY              1108
#define AFX_IDC_COLOR_LIGHTGRAY         1109
#define AFX_IDC_COLOR_DARKRED           1110
#define AFX_IDC_COLOR_DARKGREEN         1111
#define AFX_IDC_COLOR_DARKBLUE          1112
#define AFX_IDC_COLOR_LIGHTBROWN        1113
#define AFX_IDC_COLOR_DARKMAGENTA       1114
#define AFX_IDC_COLOR_DARKCYAN          1115
#define AFX_IDC_COLORPROP               1116
#define AFX_IDC_SYSTEMCOLORS            1117

// Picture porperty page
#define AFX_IDC_PROPNAME                1201
#define AFX_IDC_PICTURE                 1202
#define AFX_IDC_BROWSE                  1203
#define AFX_IDC_CLEAR                   1204

/////////////////////////////////////////////////////////////////////////////
// IDRs for OLE control standard components

// Standard propery page dialogs app should leave alone (0x7E01->)
#define AFX_IDD_PROPPAGE_COLOR         32257
#define AFX_IDD_PROPPAGE_FONT          32258
#define AFX_IDD_PROPPAGE_PICTURE       32259

#define AFX_IDB_TRUETYPE               32384

/////////////////////////////////////////////////////////////////////////////
// Standard OLE control strings

// OLE Control page strings
#define AFX_IDS_PROPPAGE_UNKNOWN        0xFE01
#define AFX_IDS_COLOR_DESKTOP           0xFE04
#define AFX_IDS_COLOR_APPWORKSPACE      0xFE05
#define AFX_IDS_COLOR_WNDBACKGND        0xFE06
#define AFX_IDS_COLOR_WNDTEXT           0xFE07
#define AFX_IDS_COLOR_MENUBAR           0xFE08
#define AFX_IDS_COLOR_MENUTEXT          0xFE09
#define AFX_IDS_COLOR_ACTIVEBAR         0xFE0A
#define AFX_IDS_COLOR_INACTIVEBAR       0xFE0B
#define AFX_IDS_COLOR_ACTIVETEXT        0xFE0C
#define AFX_IDS_COLOR_INACTIVETEXT      0xFE0D
#define AFX_IDS_COLOR_ACTIVEBORDER      0xFE0E
#define AFX_IDS_COLOR_INACTIVEBORDER    0xFE0F
#define AFX_IDS_COLOR_WNDFRAME          0xFE10
#define AFX_IDS_COLOR_SCROLLBARS        0xFE11
#define AFX_IDS_COLOR_BTNFACE           0xFE12
#define AFX_IDS_COLOR_BTNSHADOW         0xFE13
#define AFX_IDS_COLOR_BTNTEXT           0xFE14
#define AFX_IDS_COLOR_BTNHIGHLIGHT      0xFE15
#define AFX_IDS_COLOR_DISABLEDTEXT      0xFE16
#define AFX_IDS_COLOR_HIGHLIGHT         0xFE17
#define AFX_IDS_COLOR_HIGHLIGHTTEXT     0xFE18
#define AFX_IDS_REGULAR                 0xFE19
#define AFX_IDS_BOLD                    0xFE1A
#define AFX_IDS_ITALIC                  0xFE1B
#define AFX_IDS_BOLDITALIC              0xFE1C
#define AFX_IDS_SAMPLETEXT              0xFE1D
#define AFX_IDS_DISPLAYSTRING_FONT      0xFE1E
#define AFX_IDS_DISPLAYSTRING_COLOR     0xFE1F
#define AFX_IDS_DISPLAYSTRING_PICTURE   0xFE20
#define AFX_IDS_PICTUREFILTER           0xFE21
#define AFX_IDS_PICTYPE_UNKNOWN         0xFE22
#define AFX_IDS_PICTYPE_NONE            0xFE23
#define AFX_IDS_PICTYPE_BITMAP          0xFE24
#define AFX_IDS_PICTYPE_METAFILE        0xFE25
#define AFX_IDS_PICTYPE_ICON            0xFE26
#define AFX_IDS_COLOR_PPG               0xFE28
#define AFX_IDS_COLOR_PPG_CAPTION       0xFE29
#define AFX_IDS_FONT_PPG                0xFE2A
#define AFX_IDS_FONT_PPG_CAPTION        0xFE2B
#define AFX_IDS_PICTURE_PPG             0xFE2C
#define AFX_IDS_PICTURE_PPG_CAPTION     0xFE2D
#define AFX_IDS_PICTUREBROWSETITLE      0xFE30
#define AFX_IDS_BORDERSTYLE_0           0xFE31
#define AFX_IDS_BORDERSTYLE_1           0xFE32

// OLE Control verb names
#define AFX_IDS_VERB_EDIT               0xFE40
#define AFX_IDS_VERB_PROPERTIES         0xFE41

// OLE Control internal error messages
#define AFX_IDP_PICTURECANTOPEN         0xFE83
#define AFX_IDP_PICTURECANTLOAD         0xFE84
#define AFX_IDP_PICTURETOOLARGE         0xFE85
#define AFX_IDP_PICTUREREADFAILED       0xFE86

// Standard OLE Control error strings
#define AFX_IDP_E_ILLEGALFUNCTIONCALL       0xFEA0
#define AFX_IDP_E_OVERFLOW                  0xFEA1
#define AFX_IDP_E_OUTOFMEMORY               0xFEA2
#define AFX_IDP_E_DIVISIONBYZERO            0xFEA3
#define AFX_IDP_E_OUTOFSTRINGSPACE          0xFEA4
#define AFX_IDP_E_OUTOFSTACKSPACE           0xFEA5
#define AFX_IDP_E_BADFILENAMEORNUMBER       0xFEA6
#define AFX_IDP_E_FILENOTFOUND              0xFEA7
#define AFX_IDP_E_BADFILEMODE               0xFEA8
#define AFX_IDP_E_FILEALREADYOPEN           0xFEA9
#define AFX_IDP_E_DEVICEIOERROR             0xFEAA
#define AFX_IDP_E_FILEALREADYEXISTS         0xFEAB
#define AFX_IDP_E_BADRECORDLENGTH           0xFEAC
#define AFX_IDP_E_DISKFULL                  0xFEAD
#define AFX_IDP_E_BADRECORDNUMBER           0xFEAE
#define AFX_IDP_E_BADFILENAME               0xFEAF
#define AFX_IDP_E_TOOMANYFILES              0xFEB0
#define AFX_IDP_E_DEVICEUNAVAILABLE         0xFEB1
#define AFX_IDP_E_PERMISSIONDENIED          0xFEB2
#define AFX_IDP_E_DISKNOTREADY              0xFEB3
#define AFX_IDP_E_PATHFILEACCESSERROR       0xFEB4
#define AFX_IDP_E_PATHNOTFOUND              0xFEB5
#define AFX_IDP_E_INVALIDPATTERNSTRING      0xFEB6
#define AFX_IDP_E_INVALIDUSEOFNULL          0xFEB7
#define AFX_IDP_E_INVALIDFILEFORMAT         0xFEB8
#define AFX_IDP_E_INVALIDPROPERTYVALUE      0xFEB9
#define AFX_IDP_E_INVALIDPROPERTYARRAYINDEX 0xFEBA
#define AFX_IDP_E_SETNOTSUPPORTEDATRUNTIME  0xFEBB
#define AFX_IDP_E_SETNOTSUPPORTED           0xFEBC
#define AFX_IDP_E_NEEDPROPERTYARRAYINDEX    0xFEBD
#define AFX_IDP_E_SETNOTPERMITTED           0xFEBE
#define AFX_IDP_E_GETNOTSUPPORTEDATRUNTIME  0xFEBF
#define AFX_IDP_E_GETNOTSUPPORTED           0xFEC0
#define AFX_IDP_E_PROPERTYNOTFOUND          0xFEC1
#define AFX_IDP_E_INVALIDCLIPBOARDFORMAT    0xFEC2
#define AFX_IDP_E_INVALIDPICTURE            0xFEC3
#define AFX_IDP_E_PRINTERERROR              0xFEC4
#define AFX_IDP_E_CANTSAVEFILETOTEMP        0xFEC5
#define AFX_IDP_E_SEARCHTEXTNOTFOUND        0xFEC6
#define AFX_IDP_E_REPLACEMENTSTOOLONG       0xFEC7

/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#endif //__AFXRES_H__

/////////////////////////////////////////////////////////////////////////////
