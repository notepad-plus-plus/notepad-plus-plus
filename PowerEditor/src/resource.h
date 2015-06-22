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


#ifndef RESOURCE_H
#define RESOURCE_H

#define NOTEPAD_PLUS_VERSION TEXT("Notepad++ v6.7.9.2")

// should be X.Y : ie. if VERSION_DIGITALVALUE == 4, 7, 1, 0 , then X = 4, Y = 71 
// ex : #define VERSION_VALUE TEXT("5.63\0")
#define VERSION_VALUE TEXT("6.792\0")
#define VERSION_DIGITALVALUE 6, 7, 9, 2

#ifndef IDC_STATIC
#define IDC_STATIC	-1
#endif

#define	IDI_M30ICON				100
#define	IDI_CHAMELEON			101
//#define	IDI_JESUISCHARLIE		102
#define IDR_RT_MANIFEST         103

#define	IDI_NEW_OFF_ICON      201
#define	IDI_OPEN_OFF_ICON     202
#define	IDI_CLOSE_OFF_ICON    203
#define	IDI_CLOSEALL_OFF_ICON 204
#define	IDI_SAVE_OFF_ICON     205
#define	IDI_SAVEALL_OFF_ICON  206
#define	IDI_CUT_OFF_ICON      207   
#define	IDI_COPY_OFF_ICON     208   
#define	IDI_PASTE_OFF_ICON    209   
#define	IDI_UNDO_OFF_ICON     210
#define	IDI_REDO_OFF_ICON     211   
#define	IDI_FIND_OFF_ICON     212   
#define	IDI_REPLACE_OFF_ICON  213
#define	IDI_ZOOMIN_OFF_ICON   214
#define	IDI_ZOOMOUT_OFF_ICON  215
#define	IDI_VIEW_UD_DLG_OFF_ICON 216
#define	IDI_PRINT_OFF_ICON    217
#define	IDI_VIEW_ALL_CHAR_ON_ICON  218
#define	IDI_VIEW_INDENT_ON_ICON 219
#define	IDI_VIEW_WRAP_ON_ICON 220


#define	IDI_STARTRECORD_OFF_ICON     221
#define	IDI_STARTRECORD_ON_ICON      222
#define	IDI_STARTRECORD_DISABLE_ICON 223
#define	IDI_STOPRECORD_OFF_ICON      224
#define	IDI_STOPRECORD_ON_ICON       225
#define	IDI_STOPRECORD_DISABLE_ICON  226
#define	IDI_PLAYRECORD_OFF_ICON      227
#define	IDI_PLAYRECORD_ON_ICON       228
#define	IDI_PLAYRECORD_DISABLE_ICON  229
#define	IDI_SAVERECORD_OFF_ICON  	 230
#define	IDI_SAVERECORD_ON_ICON  	 231
#define	IDI_SAVERECORD_DISABLE_ICON  232

// multi run macro
#define IDI_MMPLAY_DIS_ICON					233
#define IDI_MMPLAY_OFF_ICON					234
#define IDI_MMPLAY_ON_ICON					235

#define	IDI_NEW_ON_ICON      301
#define	IDI_OPEN_ON_ICON     302
#define	IDI_CLOSE_ON_ICON    303
#define	IDI_CLOSEALL_ON_ICON 304
#define	IDI_SAVE_ON_ICON     305
#define	IDI_SAVEALL_ON_ICON  306
#define	IDI_CUT_ON_ICON      307
#define	IDI_COPY_ON_ICON     308
#define	IDI_PASTE_ON_ICON    309
#define	IDI_UNDO_ON_ICON     310
#define	IDI_REDO_ON_ICON     311
#define	IDI_FIND_ON_ICON     312
#define	IDI_REPLACE_ON_ICON  313
#define	IDI_ZOOMIN_ON_ICON   314
#define	IDI_ZOOMOUT_ON_ICON  315
#define	IDI_VIEW_UD_DLG_ON_ICON 316
#define	IDI_PRINT_ON_ICON    317
#define	IDI_VIEW_ALL_CHAR_OFF_ICON  318
#define	IDI_VIEW_INDENT_OFF_ICON 319
#define	IDI_VIEW_WRAP_OFF_ICON 320

//#define	IDI_NEW_DISABLE_ICON  401
//#define	IDI_OPEN_ON_ICON      402
#define	IDI_SAVE_DISABLE_ICON   403  
#define	IDI_SAVEALL_DISABLE_ICON 404
//#define	IDI_CLOSE_ON_ICON       405
//#define	IDI_CLOSEALL_ON_ICON    406
#define	IDI_CUT_DISABLE_ICON      407
#define	IDI_COPY_DISABLE_ICON     408
#define	IDI_PASTE_DISABLE_ICON   409
#define	IDI_UNDO_DISABLE_ICON    410
#define	IDI_REDO_DISABLE_ICON    411
#define	IDI_DELETE_ICON             412

#define	IDI_SYNCV_OFF_ICON		413
#define	IDI_SYNCV_ON_ICON		414
#define	IDI_SYNCV_DISABLE_ICON	415

#define	IDI_SYNCH_OFF_ICON		416
#define	IDI_SYNCH_ON_ICON		417
#define	IDI_SYNCH_DISABLE_ICON	418

#define	IDI_SAVED_ICON       501
#define	IDI_UNSAVED_ICON     502
#define	IDI_READONLY_ICON     503
#define	IDI_FIND_RESULT_ICON  504

#define	IDI_PROJECT_WORKSPACE		601
#define	IDI_PROJECT_WORKSPACEDIRTY	602
#define	IDI_PROJECT_PROJECT			603
#define	IDI_PROJECT_FOLDEROPEN		604
#define	IDI_PROJECT_FOLDERCLOSE		605
#define	IDI_PROJECT_FILE			606
#define	IDI_PROJECT_FILEINVALID		607

#define	IDI_FUNCLIST_ROOT			620
#define	IDI_FUNCLIST_NODE			621
#define	IDI_FUNCLIST_LEAF			622

#define	IDI_FUNCLIST_SORTBUTTON		631
#define	IDI_FUNCLIST_RELOADBUTTON	632

#define	IDC_MY_CUR     1402
#define	IDC_UP_ARROW  1403
#define	IDC_DRAG_TAB    1404
#define	IDC_DRAG_INTERDIT_TAB 1405
#define	IDC_DRAG_PLUS_TAB 1406
#define	IDC_DRAG_OUT_TAB 1407

#define	IDC_MACRO_RECORDING 1408

#define	IDR_SAVEALL			1500
#define	IDR_CLOSEFILE		1501
#define	IDR_CLOSEALL		1502
#define	IDR_FIND				1503
#define	IDR_REPLACE			1504
#define	IDR_ZOOMIN			1505
#define	IDR_ZOOMOUT		1506
#define	IDR_WRAP			1507
#define	IDR_INVISIBLECHAR	1508
#define	IDR_INDENTGUIDE		1509
#define	IDR_SHOWPANNEL		1510
#define	IDR_STARTRECORD		1511
#define	IDR_STOPRECORD		1512
#define	IDR_PLAYRECORD		1513
#define	IDR_SAVERECORD		1514
#define	IDR_SYNCV		1515
#define	IDR_SYNCH		1516
#define	IDR_FILENEW		1517
#define	IDR_FILEOPEN	1518
#define	IDR_FILESAVE	1519
#define	IDR_PRINT		1520
#define	IDR_CUT			1521
#define	IDR_COPY		1522
#define	IDR_PASTE		1523
#define	IDR_UNDO		1524
#define	IDR_REDO		1525
#define IDR_M_PLAYRECORD	1526
#define	IDR_DOCMAP		1527
#define IDR_FUNC_LIST	1528
#define	IDR_CLOSETAB     1530
#define	IDR_CLOSETAB_INACT     1531
#define	IDR_CLOSETAB_HOVER     1532
#define	IDR_CLOSETAB_PUSH     1533

#define IDR_FUNC_LIST_ICO	1534
#define	IDR_DOCMAP_ICO		1535
#define	IDR_PROJECTPANEL_ICO		1536
#define	IDR_CLIPBOARDPANEL_ICO		1537
#define	IDR_ASCIIPANEL_ICO		1538
#define	IDR_DOCSWITCHER_ICO		1539

#define ID_MACRO 20000
#define ID_MACRO_LIMIT 20200

#define ID_USER_CMD 21000
#define ID_USER_CMD_LIMIT 21200

#define ID_PLUGINS_CMD 22000
#define ID_PLUGINS_CMD_LIMIT 22500

#define ID_PLUGINS_CMD_DYNAMIC       23000
#define ID_PLUGINS_CMD_DYNAMIC_LIMIT 24999 

#define MARKER_PLUGINS          3
#define MARKER_PLUGINS_LIMIT   19
/*UNLOAD
#define ID_PLUGINS_REMOVING 22501
#define ID_PLUGINS_REMOVING_END 22600
*/


//#define IDM 40000

#define IDCMD 50000
	//#define	IDM_EDIT_AUTOCOMPLETE    			(IDCMD+0)
	//#define	IDM_EDIT_AUTOCOMPLETE_CURRENTFILE	(IDCMD+1)
	
	#define	IDC_PREV_DOC					(IDCMD+3)
	#define	IDC_NEXT_DOC					(IDCMD+4)
	#define	IDC_EDIT_TOGGLEMACRORECORDING	(IDCMD+5)
	//#define	IDC_KEY_HOME					(IDCMD+6)
	//#define	IDC_KEY_END						(IDCMD+7)
	//#define	IDC_KEY_SELECT_2_HOME			(IDCMD+8)
	//#define	IDC_KEY_SELECT_2_END			(IDCMD+9)
	
#define	IDCMD_LIMIT		    			(IDCMD+20)

#define IDSCINTILLA 60000				
	#define	IDSCINTILLA_KEY_HOME        (IDSCINTILLA+0)
	#define	IDSCINTILLA_KEY_HOME_WRAP   (IDSCINTILLA+1)
	#define	IDSCINTILLA_KEY_END         (IDSCINTILLA+2)
	#define	IDSCINTILLA_KEY_END_WRAP    (IDSCINTILLA+3)
	#define	IDSCINTILLA_KEY_LINE_DUP    (IDSCINTILLA+4)
	#define	IDSCINTILLA_KEY_LINE_CUT    (IDSCINTILLA+5)
	#define	IDSCINTILLA_KEY_LINE_DEL    (IDSCINTILLA+6)
	#define	IDSCINTILLA_KEY_LINE_TRANS  (IDSCINTILLA+7)
	#define	IDSCINTILLA_KEY_LINE_COPY   (IDSCINTILLA+8)
	#define	IDSCINTILLA_KEY_CUT         (IDSCINTILLA+9)
	#define	IDSCINTILLA_KEY_COPY        (IDSCINTILLA+10)
	#define	IDSCINTILLA_KEY_PASTE       (IDSCINTILLA+11)
	#define	IDSCINTILLA_KEY_DEL         (IDSCINTILLA+12)
	#define	IDSCINTILLA_KEY_SELECTALL   (IDSCINTILLA+13)
	#define	IDSCINTILLA_KEY_OUTDENT     (IDSCINTILLA+14)
	#define	IDSCINTILLA_KEY_UNDO        (IDSCINTILLA+15)
	#define	IDSCINTILLA_KEY_REDO        (IDSCINTILLA+16)
#define	IDSCINTILLA_LIMIT		(IDSCINTILLA+30)

#define	IDD_FILEVIEW_DIALOG				1000

#define	IDC_MINIMIZED_TRAY         67001

#define IDD_CREATE_DIRECTORY			1100
#define IDC_STATIC_CURRENT_FOLDER       1101
#define IDC_EDIT_NEW_FOLDER             1102

#define	IDD_INSERT_INPUT_TEXT			1200
#define	IDC_EDIT_INPUT_VALUE			1201
#define	IDC_STATIC_INPUT_TITLE			1202
#define	IDC_ICON_INPUT_ICON				1203

#define	IDR_M30_MENU					1500

#define	IDR_SYSTRAYPOPUP_MENU			1501

// #define	IDD_FIND_REPLACE_DLG		1600

#define	IDD_ABOUTBOX 1700
#define	IDC_LICENCE_EDIT 1701
#define	IDC_HOME_ADDR		1702
#define	IDC_EMAIL_ADDR		1703
#define	IDC_ONLINEHELP_ADDR 1704
#define	IDC_AUTHOR_NAME 1705
#define IDC_BUILD_DATETIME 1706		//LS: CompileDateInAboutDialog: Automatically insert compile date as additional version info in About-dialog!
//#define	IDD_USER_DEFINE_BOX 				 1800

//#define	IDD_RUN_DLG      1900

#define	IDD_GOLINE		2000
#define	ID_GOLINE_EDIT	(IDD_GOLINE + 1)
#define	ID_CURRLINE		(IDD_GOLINE + 2)
#define	ID_LASTLINE		(IDD_GOLINE + 3)
#define	ID_URHERE_STATIC           (IDD_GOLINE + 4)
#define	ID_UGO_STATIC                 (IDD_GOLINE + 5)
#define	ID_NOMORETHAN_STATIC   (IDD_GOLINE + 6)
#define	IDC_RADIO_GOTOLINE   (IDD_GOLINE + 7)
#define	IDC_RADIO_GOTOOFFSET   (IDD_GOLINE + 8)

// voir columnEditor_rc.h
//#define	IDD_COLUMNEDIT   2020


//#define	IDD_COLOUR_POPUP   2100

// See WordStyleDlgRes.h
//#define	IDD_STYLER_DLG	2200
//#define IDD_GLOBAL_STYLER_DLG	2300

#define	IDD_VALUE_DLG       2400
#define	IDC_VALUE_STATIC  2401
#define	IDC_VALUE_EDIT      2402

#define	IDD_BUTTON_DLG       2410
#define	IDC_RESTORE_BUTTON  2411

// see TaskListDlg_rc.h
//#define	IDD_TASKLIST_DLG    2450
#define	IDD_SETTING_DLG    2500



//See ShortcutMapper_rc.h
//#define	IDD_SHORTCUTMAPPER_DLG      2600

//See ansiCharPanel_rc.h
//#define	IDD_ANSIASCII_PANEL      2700

//See clipboardHistoryPanel_rc.h
//#define	IDD_CLIPBOARDHISTORY_PANEL      2800

//See findCharsInRange_rc.h
//#define	IDD_FINDCHARACTERS      2900

//See VerticalFileSwitcher_rc.h
//#define	IDD_FILESWITCHER_PANEL      3000

//See ProjectPanel_rc.h
//#define	IDD_PROJECTPANEL      3100
//#define	IDD_FILERELOCALIZER_DIALOG  3200

//See documentMap_rc.h
//#define	IDD_DOCUMENTMAP      3300

//See functionListPanel_rc.h
//#define	IDD_FUNCLIST_PANEL   3400


// See regExtDlg.h
//#define	IDD_REGEXT 4000

// See shortcutRc.h
//#define	IDD_SHORTCUT_DLG      5000

// See preference.rc
//#define	IDD_PREFERENCE_BOX 6000

#define NOTEPADPLUS_USER_INTERNAL     (WM_USER + 0000)
	#define NPPM_INTERNAL_USERCMDLIST_MODIFIED      (NOTEPADPLUS_USER_INTERNAL + 1)
	#define NPPM_INTERNAL_CMDLIST_MODIFIED          (NOTEPADPLUS_USER_INTERNAL + 2)
	#define NPPM_INTERNAL_MACROLIST_MODIFIED        (NOTEPADPLUS_USER_INTERNAL + 3)
	#define NPPM_INTERNAL_PLUGINCMDLIST_MODIFIED    (NOTEPADPLUS_USER_INTERNAL + 4)
	#define NPPM_INTERNAL_CLEARSCINTILLAKEY         (NOTEPADPLUS_USER_INTERNAL + 5)
	#define NPPM_INTERNAL_BINDSCINTILLAKEY	        (NOTEPADPLUS_USER_INTERNAL + 6)
	#define NPPM_INTERNAL_SCINTILLAKEYMODIFIED      (NOTEPADPLUS_USER_INTERNAL + 7)
	#define NPPM_INTERNAL_SCINTILLAFINFERCOLLAPSE   (NOTEPADPLUS_USER_INTERNAL + 8)
	#define NPPM_INTERNAL_SCINTILLAFINFERUNCOLLAPSE (NOTEPADPLUS_USER_INTERNAL + 9)
	#define NPPM_INTERNAL_DISABLEAUTOUPDATE			(NOTEPADPLUS_USER_INTERNAL + 10)
	#define NPPM_INTERNAL_SETTING_HISTORY_SIZE		(NOTEPADPLUS_USER_INTERNAL + 11)
	#define NPPM_INTERNAL_ISTABBARREDUCED			(NOTEPADPLUS_USER_INTERNAL + 12)
	#define NPPM_INTERNAL_ISFOCUSEDTAB				(NOTEPADPLUS_USER_INTERNAL + 13)
	#define NPPM_INTERNAL_GETMENU					(NOTEPADPLUS_USER_INTERNAL + 14)
	#define NPPM_INTERNAL_CLEARINDICATOR			(NOTEPADPLUS_USER_INTERNAL + 15)
	#define NPPM_INTERNAL_SCINTILLAFINFERCOPY       (NOTEPADPLUS_USER_INTERNAL + 16)
	#define NPPM_INTERNAL_SCINTILLAFINFERSELECTALL  (NOTEPADPLUS_USER_INTERNAL + 17)
	#define NPPM_INTERNAL_SETCARETWIDTH             (NOTEPADPLUS_USER_INTERNAL + 18)
	#define NPPM_INTERNAL_SETCARETBLINKRATE         (NOTEPADPLUS_USER_INTERNAL + 19)
	#define NPPM_INTERNAL_CLEARINDICATORTAGMATCH	(NOTEPADPLUS_USER_INTERNAL + 20)
	#define NPPM_INTERNAL_CLEARINDICATORTAGATTR		(NOTEPADPLUS_USER_INTERNAL + 21)
	#define NPPM_INTERNAL_SWITCHVIEWFROMHWND		(NOTEPADPLUS_USER_INTERNAL + 22)
	#define NPPM_INTERNAL_UPDATETITLEBAR			(NOTEPADPLUS_USER_INTERNAL + 23)
	#define NPPM_INTERNAL_CANCEL_FIND_IN_FILES		(NOTEPADPLUS_USER_INTERNAL + 24)
	#define NPPM_INTERNAL_RELOADNATIVELANG			(NOTEPADPLUS_USER_INTERNAL + 25)
	#define NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED	(NOTEPADPLUS_USER_INTERNAL + 26)
	#define NPPM_INTERNAL_SCINTILLAFINFERCLEARALL	(NOTEPADPLUS_USER_INTERNAL + 27)
	#define	NPPM_INTERNAL_SETTING_EDGE_SIZE			(NOTEPADPLUS_USER_INTERNAL + 28)
	#define	NPPM_INTERNAL_SETTING_TAB_REPLCESPACE	(NOTEPADPLUS_USER_INTERNAL + 29)
	#define	NPPM_INTERNAL_SETTING_TAB_SIZE			(NOTEPADPLUS_USER_INTERNAL + 30)
	#define	NPPM_INTERNAL_RELOADSTYLERS				(NOTEPADPLUS_USER_INTERNAL + 31)
	#define NPPM_INTERNAL_DOCORDERCHANGED			(NOTEPADPLUS_USER_INTERNAL + 32)
	#define NPPM_INTERNAL_SETMULTISELCTION          (NOTEPADPLUS_USER_INTERNAL + 33)
	#define	NPPM_INTERNAL_SCINTILLAFINFEROPENALL 	(NOTEPADPLUS_USER_INTERNAL + 34)
	#define	NPPM_INTERNAL_RECENTFILELIST_UPDATE 	(NOTEPADPLUS_USER_INTERNAL + 35)
	#define	NPPM_INTERNAL_RECENTFILELIST_SWITCH 	(NOTEPADPLUS_USER_INTERNAL + 36)
	#define	NPPM_INTERNAL_GETSCINTEDTVIEW       	(NOTEPADPLUS_USER_INTERNAL + 37)
	#define NPPM_INTERNAL_ENABLESNAPSHOT			(NOTEPADPLUS_USER_INTERNAL + 38) 
	#define NPPM_INTERNAL_SAVECURRENTSESSION		(NOTEPADPLUS_USER_INTERNAL + 39)


	//wParam: 0
	//lParam: document new index
// See Notepad_plus_msgs.h
//#define NOTEPADPLUS_USER   (WM_USER + 1000)

	//
	// Used by Doc Monitor plugin
	//
	#define NPPM_INTERNAL_CHECKDOCSTATUS (NPPMSG + 53)
	// VOID NPPM_CHECKDOCSTATUS(BOOL, 0)
	// check all opened documents status.
	// If files are modified, then reloaod (with or without prompt, it depends on settings).
	// if files are deleted, then prompt user to close the documents

	#define NPPM_INTERNAL_ENABLECHECKDOCOPT (NPPMSG + 54)
	// VOID NPPM_ENABLECHECKDOCOPT(OPT, 0)
		// where OPT is :
		#define CHECKDOCOPT_NONE 0
		#define CHECKDOCOPT_UPDATESILENTLY 1
		#define CHECKDOCOPT_UPDATEGO2END 2

	#define NPPM_INTERNAL_GETCHECKDOCOPT (NPPMSG + 55)
	// INT NPPM_GETCHECKDOCOPT(0, 0)
	#define NPPM_INTERNAL_SETCHECKDOCOPT (NPPMSG + 56)
	// INT NPPM_SETCHECKDOCOPT(OPT, 0)

	//
	// Used by netnote plugin
	//
	#define NPPM_INTERNAL_SETFILENAME (NPPMSG + 63)
	//wParam: BufferID to rename
	//lParam: name to set (TCHAR*)
	//Buffer must have been previously unnamed (eg "new 1" document types)



#define SCINTILLA_USER     (WM_USER + 2000)


#define	MACRO_USER    (WM_USER + 4000)
	#define WM_GETCURRENTMACROSTATUS (MACRO_USER + 01)
	#define WM_MACRODLGRUNMACRO       (MACRO_USER + 02)


// See Notepad_plus_msgs.h
//#define	RUNCOMMAND_USER    (WM_USER + 3000)
#define SPLITTER_USER      (WM_USER + 4000)
#define WORDSTYLE_USER     (WM_USER + 5000)
#define COLOURPOPUP_USER   (WM_USER + 6000)
#define BABYGRID_USER      (WM_USER + 7000)

//#define	IDD_DOCKING_MNG 	(IDM  + 7000)

#define MENUINDEX_FILE     0
#define MENUINDEX_EDIT     1
#define MENUINDEX_SEARCH   2
#define MENUINDEX_VIEW     3
#define MENUINDEX_FORMAT   4
#define MENUINDEX_LANGUAGE 5
#define MENUINDEX_SETTINGS 6
#define MENUINDEX_MACRO    7
#define MENUINDEX_RUN      8
#define MENUINDEX_PLUGINS  9 

#endif // RESOURCE_H


