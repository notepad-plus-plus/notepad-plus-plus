// This file is part of Notepad++ project
// Copyright (C)2023 Don HO <don.h@free.fr>

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


//************ Notepad++ version **************************

#define NOTEPAD_PLUS_VERSION L"Notepad++ v8.7.9"

// should be X.Y : ie. if VERSION_DIGITALVALUE == 4, 7, 1, 0 , then X = 4, Y = 71
// ex : #define VERSION_VALUE L"5.63\0"
#define VERSION_INTERNAL_VALUE L"8.79\0"

#define VERSION_PRODUCT_VALUE L"8.7.9\0"
#define VERSION_DIGITALVALUE 8, 7, 9, 0

//**********************************************************



#ifndef IDC_STATIC
#define IDC_STATIC    -1
#endif

#define IDI_NPPABOUT_LOGO           99
#define IDI_M30ICON                100
#define IDI_CHAMELEON              101
#define IDI_CHAMELEON_DM           102
//#define IDI_JESUISCHARLIE        102
//#define IDI_GILETJAUNE           102
//#define IDI_SAMESEXMARRIAGE      102
//#define IDI_TAIWANSSOVEREIGNTY     105
//#define IDI_TAIWANSSOVEREIGNTY_DM  106
#define IDI_WITHUKRAINE            105
#define IDR_RT_MANIFEST            103
#define IDI_ICONABSENT             104

//
// TOOLBAR ICO - set 1
//
#define IDI_NEW_ICON                      201
#define IDI_OPEN_ICON                     202
#define IDI_CLOSE_ICON                    203
#define IDI_CLOSEALL_ICON                 204
#define IDI_SAVE_ICON                     205
#define IDI_SAVEALL_ICON                  206
#define IDI_CUT_ICON                      207
#define IDI_COPY_ICON                     208
#define IDI_PASTE_ICON                    209
#define IDI_UNDO_ICON                     210
#define IDI_REDO_ICON                     211
#define IDI_FIND_ICON                     212
#define IDI_REPLACE_ICON                  213
#define IDI_ZOOMIN_ICON                   214
#define IDI_ZOOMOUT_ICON                  215
#define IDI_VIEW_UD_DLG_ICON              216
#define IDI_PRINT_ICON                    217
#define IDI_STARTRECORD_ICON              218
#define IDI_STARTRECORD_DISABLE_ICON      219
#define IDI_STOPRECORD_ICON               220
#define IDI_STOPRECORD_DISABLE_ICON       221
#define IDI_PLAYRECORD_ICON               222
#define IDI_PLAYRECORD_DISABLE_ICON       223
#define IDI_SAVERECORD_ICON               224
#define IDI_SAVERECORD_DISABLE_ICON       225
#define IDI_MMPLAY_DIS_ICON               226
#define IDI_MMPLAY_ICON                   227
#define IDI_VIEW_ALL_CHAR_ICON            228
#define IDI_VIEW_INDENT_ICON              229
#define IDI_VIEW_WRAP_ICON                230
#define IDI_SAVE_DISABLE_ICON             231
#define IDI_SAVEALL_DISABLE_ICON          232
#define IDI_CUT_DISABLE_ICON              233
#define IDI_COPY_DISABLE_ICON             234
#define IDI_PASTE_DISABLE_ICON            235
#define IDI_UNDO_DISABLE_ICON             236
#define IDI_REDO_DISABLE_ICON             237
#define IDI_SYNCV_ICON                    238
#define IDI_SYNCV_DISABLE_ICON            239
#define IDI_SYNCH_ICON                    240
#define IDI_SYNCH_DISABLE_ICON            241
#define IDI_VIEW_DOC_MAP_ICON             242
#define IDI_VIEW_FILEBROWSER_ICON         243
#define IDI_VIEW_FUNCLIST_ICON            244
#define IDI_VIEW_MONITORING_ICON          245
#define IDI_VIEW_MONITORING_DIS_ICON      246
#define IDI_VIEW_DOCLIST_ICON             247

//
// TOOLBAR ICO - set 1, Dark Mode
//
#define IDI_NEW_ICON_DM                   251
#define IDI_OPEN_ICON_DM                  252
#define IDI_CLOSE_ICON_DM                 253
#define IDI_CLOSEALL_ICON_DM              254
#define IDI_SAVE_ICON_DM                  255
#define IDI_SAVEALL_ICON_DM               256
#define IDI_CUT_ICON_DM                   257
#define IDI_COPY_ICON_DM                  258
#define IDI_PASTE_ICON_DM                 259
#define IDI_UNDO_ICON_DM                  260
#define IDI_REDO_ICON_DM                  261
#define IDI_FIND_ICON_DM                  262
#define IDI_REPLACE_ICON_DM               263
#define IDI_ZOOMIN_ICON_DM                264
#define IDI_ZOOMOUT_ICON_DM               265
#define IDI_VIEW_UD_DLG_ICON_DM           266
#define IDI_PRINT_ICON_DM                 267
#define IDI_STARTRECORD_ICON_DM           268
#define IDI_STARTRECORD_DISABLE_ICON_DM   269
#define IDI_STOPRECORD_ICON_DM            270
#define IDI_STOPRECORD_DISABLE_ICON_DM    271
#define IDI_PLAYRECORD_ICON_DM            272
#define IDI_PLAYRECORD_DISABLE_ICON_DM    273
#define IDI_SAVERECORD_ICON_DM            274
#define IDI_SAVERECORD_DISABLE_ICON_DM    275
#define IDI_MMPLAY_DIS_ICON_DM            276
#define IDI_MMPLAY_ICON_DM                277
#define IDI_VIEW_ALL_CHAR_ICON_DM         278
#define IDI_VIEW_INDENT_ICON_DM           279
#define IDI_VIEW_WRAP_ICON_DM             280
#define IDI_SAVE_DISABLE_ICON_DM          281
#define IDI_SAVEALL_DISABLE_ICON_DM       282
#define IDI_CUT_DISABLE_ICON_DM           283
#define IDI_COPY_DISABLE_ICON_DM          284
#define IDI_PASTE_DISABLE_ICON_DM         285
#define IDI_UNDO_DISABLE_ICON_DM          286
#define IDI_REDO_DISABLE_ICON_DM          287
#define IDI_SYNCV_ICON_DM                 288
#define IDI_SYNCV_DISABLE_ICON_DM         289
#define IDI_SYNCH_ICON_DM                 290
#define IDI_SYNCH_DISABLE_ICON_DM         291
#define IDI_VIEW_DOC_MAP_ICON_DM          292
#define IDI_VIEW_FILEBROWSER_ICON_DM      293
#define IDI_VIEW_FUNCLIST_ICON_DM         294
#define IDI_VIEW_MONITORING_ICON_DM       295
#define IDI_VIEW_MONITORING_DIS_ICON_DM   296
#define IDI_VIEW_DOCLIST_ICON_DM          297

//
// TOOLBAR ICO - set 2
//
#define IDI_NEW_ICON2                     301
#define IDI_OPEN_ICON2                    302
#define IDI_CLOSE_ICON2                   303
#define IDI_CLOSEALL_ICON2                304
#define IDI_SAVE_ICON2                    305
#define IDI_SAVEALL_ICON2                 306
#define IDI_CUT_ICON2                     307
#define IDI_COPY_ICON2                    308
#define IDI_PASTE_ICON2                   309
#define IDI_UNDO_ICON2                    310
#define IDI_REDO_ICON2                    311
#define IDI_FIND_ICON2                    312
#define IDI_REPLACE_ICON2                 313
#define IDI_ZOOMIN_ICON2                  314
#define IDI_ZOOMOUT_ICON2                 315
#define IDI_VIEW_UD_DLG_ICON2             316
#define IDI_PRINT_ICON2                   317
#define IDI_STARTRECORD_ICON2             318
#define IDI_STARTRECORD_DISABLE_ICON2     319
#define IDI_STOPRECORD_ICON2              320
#define IDI_STOPRECORD_DISABLE_ICON2      321
#define IDI_PLAYRECORD_ICON2              322
#define IDI_PLAYRECORD_DISABLE_ICON2      323
#define IDI_SAVERECORD_ICON2              324
#define IDI_SAVERECORD_DISABLE_ICON2      325
#define IDI_MMPLAY_DIS_ICON2              326
#define IDI_MMPLAY_ICON2                  327
#define IDI_VIEW_ALL_CHAR_ICON2           328
#define IDI_VIEW_INDENT_ICON2             329
#define IDI_VIEW_WRAP_ICON2               330
#define IDI_SAVE_DISABLE_ICON2            331
#define IDI_SAVEALL_DISABLE_ICON2         332
#define IDI_CUT_DISABLE_ICON2             333
#define IDI_COPY_DISABLE_ICON2            334
#define IDI_PASTE_DISABLE_ICON2           335
#define IDI_UNDO_DISABLE_ICON2            336
#define IDI_REDO_DISABLE_ICON2            337
#define IDI_SYNCV_ICON2                   338
#define IDI_SYNCV_DISABLE_ICON2           339
#define IDI_SYNCH_ICON2                   340
#define IDI_SYNCH_DISABLE_ICON2           341
#define IDI_VIEW_DOC_MAP_ICON2            342
#define IDI_VIEW_FILEBROWSER_ICON2        343
#define IDI_VIEW_FUNCLIST_ICON2           344
#define IDI_VIEW_MONITORING_ICON2         345
#define IDI_VIEW_MONITORING_DIS_ICON2     346
#define IDI_VIEW_DOCLIST_ICON2            347

//
// TOOLBAR ICO - set 2, Dark Mode
//
#define IDI_NEW_ICON_DM2                  351
#define IDI_OPEN_ICON_DM2                 352
#define IDI_CLOSE_ICON_DM2                353
#define IDI_CLOSEALL_ICON_DM2             354
#define IDI_SAVE_ICON_DM2                 355
#define IDI_SAVEALL_ICON_DM2              356
#define IDI_CUT_ICON_DM2                  357
#define IDI_COPY_ICON_DM2                 358
#define IDI_PASTE_ICON_DM2                359
#define IDI_UNDO_ICON_DM2                 360
#define IDI_REDO_ICON_DM2                 361
#define IDI_FIND_ICON_DM2                 362
#define IDI_REPLACE_ICON_DM2              363
#define IDI_ZOOMIN_ICON_DM2               364
#define IDI_ZOOMOUT_ICON_DM2              365
#define IDI_VIEW_UD_DLG_ICON_DM2          366
#define IDI_PRINT_ICON_DM2                367
#define IDI_STARTRECORD_ICON_DM2          368
#define IDI_STARTRECORD_DISABLE_ICON_DM2  369
#define IDI_STOPRECORD_ICON_DM2           370
#define IDI_STOPRECORD_DISABLE_ICON_DM2   371
#define IDI_PLAYRECORD_ICON_DM2           372
#define IDI_PLAYRECORD_DISABLE_ICON_DM2   373
#define IDI_SAVERECORD_ICON_DM2           374
#define IDI_SAVERECORD_DISABLE_ICON_DM2   375
#define IDI_MMPLAY_DIS_ICON_DM2           376
#define IDI_MMPLAY_ICON_DM2               377
#define IDI_VIEW_ALL_CHAR_ICON_DM2        378
#define IDI_VIEW_INDENT_ICON_DM2          379
#define IDI_VIEW_WRAP_ICON_DM2            380
#define IDI_SAVE_DISABLE_ICON_DM2         381
#define IDI_SAVEALL_DISABLE_ICON_DM2      382
#define IDI_CUT_DISABLE_ICON_DM2          383
#define IDI_COPY_DISABLE_ICON_DM2         384
#define IDI_PASTE_DISABLE_ICON_DM2        385
#define IDI_UNDO_DISABLE_ICON_DM2         386
#define IDI_REDO_DISABLE_ICON_DM2         387
#define IDI_SYNCV_ICON_DM2                388
#define IDI_SYNCV_DISABLE_ICON_DM2        389
#define IDI_SYNCH_ICON_DM2                390
#define IDI_SYNCH_DISABLE_ICON_DM2        391
#define IDI_VIEW_DOC_MAP_ICON_DM2         392
#define IDI_VIEW_FILEBROWSER_ICON_DM2     393
#define IDI_VIEW_FUNCLIST_ICON_DM2        394
#define IDI_VIEW_MONITORING_ICON_DM2      395
#define IDI_VIEW_MONITORING_DIS_ICON_DM2  396
#define IDI_VIEW_DOCLIST_ICON_DM2         397



#define IDI_SAVED_ICON           501
#define IDI_UNSAVED_ICON         502
#define IDI_READONLY_ICON        503
#define IDI_FIND_RESULT_ICON     504
#define IDI_MONITORING_ICON      505
#define IDI_SAVED_ALT_ICON       506
#define IDI_UNSAVED_ALT_ICON     507
#define IDI_READONLY_ALT_ICON    508
#define IDI_SAVED_DM_ICON        509
#define IDI_UNSAVED_DM_ICON      510
#define IDI_MONITORING_DM_ICON   511
#define IDI_READONLY_DM_ICON     512


#define IDI_PROJECT_WORKSPACE          601
#define IDI_PROJECT_WORKSPACEDIRTY     602
#define IDI_PROJECT_PROJECT            603
#define IDI_PROJECT_FOLDEROPEN         604
#define IDI_PROJECT_FOLDERCLOSE        605
#define IDI_PROJECT_FILE               606
#define IDI_PROJECT_FILEINVALID        607
#define IDI_FB_ROOTOPEN                608
#define IDI_FB_ROOTCLOSE               609

#define IDI_PROJECT_WORKSPACE_DM       611
#define IDI_PROJECT_WORKSPACEDIRTY_DM  612
#define IDI_PROJECT_PROJECT_DM         613
#define IDI_PROJECT_FOLDEROPEN_DM      614
#define IDI_PROJECT_FOLDERCLOSE_DM     615
#define IDI_PROJECT_FILE_DM            616
#define IDI_PROJECT_FILEINVALID_DM     617
#define IDI_FB_ROOTOPEN_DM             618
#define IDI_FB_ROOTCLOSE_DM            619

#define IDI_PROJECT_WORKSPACE2         621
#define IDI_PROJECT_WORKSPACEDIRTY2    622
#define IDI_PROJECT_PROJECT2           623
#define IDI_PROJECT_FOLDEROPEN2        624
#define IDI_PROJECT_FOLDERCLOSE2       625
#define IDI_PROJECT_FILE2              626
#define IDI_PROJECT_FILEINVALID2       627
#define IDI_FB_ROOTOPEN2               628
#define IDI_FB_ROOTCLOSE2              629

#define IDI_FB_SELECTCURRENTFILE       630
#define IDI_FB_FOLDALL                 631
#define IDI_FB_EXPANDALL               632
#define IDI_FB_SELECTCURRENTFILE_DM    633
#define IDI_FB_FOLDALL_DM              634
#define IDI_FB_EXPANDALL_DM            635

#define IDI_FUNCLIST_ROOT              IDI_PROJECT_FILE // using same file
#define IDI_FUNCLIST_NODE              641
#define IDI_FUNCLIST_LEAF              642

#define IDI_FUNCLIST_ROOT_DM           IDI_PROJECT_FILE_DM // using same file
#define IDI_FUNCLIST_NODE_DM           644
#define IDI_FUNCLIST_LEAF_DM           645

#define IDI_FUNCLIST_ROOT2             IDI_PROJECT_FILE2 // using same file
#define IDI_FUNCLIST_NODE2             647
#define IDI_FUNCLIST_LEAF2             648

#define IDI_FUNCLIST_SORTBUTTON              651
#define IDI_FUNCLIST_RELOADBUTTON            652
#define IDI_FUNCLIST_PREFERENCEBUTTON        653
#define IDI_FUNCLIST_SORTBUTTON_DM           654
#define IDI_FUNCLIST_RELOADBUTTON_DM         655
#define IDI_FUNCLIST_PREFERENCEBUTTON_DM     656




#define IDI_GET_INFO_FROM_TOOLTIP               661



#define IDC_MY_CUR     1402
#define IDC_UP_ARROW  1403
#define IDC_DRAG_TAB    1404
#define IDC_DRAG_INTERDIT_TAB 1405
#define IDC_DRAG_PLUS_TAB 1406
#define IDC_DRAG_OUT_TAB 1407

#define IDC_MACRO_RECORDING 1408

#define IDR_SAVEALL                 1500
#define IDR_CLOSEFILE               1501
#define IDR_CLOSEALL                1502
#define IDR_FIND                    1503
#define IDR_REPLACE                 1504
#define IDR_ZOOMIN                  1505
#define IDR_ZOOMOUT                 1506
#define IDR_WRAP                    1507
#define IDR_INVISIBLECHAR           1508
#define IDR_INDENTGUIDE             1509
#define IDR_SHOWPANNEL              1510
#define IDR_STARTRECORD             1511
#define IDR_STOPRECORD              1512
#define IDR_PLAYRECORD              1513
#define IDR_SAVERECORD              1514
#define IDR_SYNCV                   1515
#define IDR_SYNCH                   1516
#define IDR_FILENEW                 1517
#define IDR_FILEOPEN                1518
#define IDR_FILESAVE                1519
#define IDR_PRINT                   1520
#define IDR_CUT                     1521
#define IDR_COPY                    1522
#define IDR_PASTE                   1523
#define IDR_UNDO                    1524
#define IDR_REDO                    1525
#define IDR_M_PLAYRECORD            1526
#define IDR_DOCMAP                  1527
#define IDR_FUNC_LIST               1528
#define IDR_FILEBROWSER             1529
#define IDR_CLOSETAB                1530
#define IDR_CLOSETAB_INACT          1531
#define IDR_CLOSETAB_INACT_EMPTY    1532
#define IDR_CLOSETAB_HOVERIN        1533
#define IDR_CLOSETAB_HOVERONTAB     1534
#define IDR_CLOSETAB_PUSH           1535
#define IDR_FUNC_LIST_ICO           1536
#define IDR_DOCMAP_ICO              1537
#define IDR_PROJECTPANEL_ICO        1538
#define IDR_CLIPBOARDPANEL_ICO      1539
#define IDR_ASCIIPANEL_ICO          1540
#define IDR_DOCSWITCHER_ICO         1541
#define IDR_FILEBROWSER_ICO         1542
#define IDR_FILEMONITORING          1543
#define IDR_CLOSETAB_DM             1544
#define IDR_CLOSETAB_INACT_DM       1545
#define IDR_CLOSETAB_INACT_EMPTY_DM 1546
#define IDR_CLOSETAB_HOVERIN_DM     1547
#define IDR_CLOSETAB_HOVERONTAB_DM  1548
#define IDR_CLOSETAB_PUSH_DM        1549
#define IDR_DOCLIST                 1550
#define IDR_DOCLIST_ICO             1551

#define IDR_FILEBROWSER_ICO2        1552
#define IDR_FILEBROWSER_ICO_DM      1553
#define IDR_FUNC_LIST_ICO2          1554
#define IDR_FUNC_LIST_ICO_DM        1555
#define IDR_DOCMAP_ICO2             1556
#define IDR_DOCMAP_ICO_DM           1557
#define IDR_DOCLIST_ICO2            1558
#define IDR_DOCLIST_ICO_DM          1559
#define IDR_PROJECTPANEL_ICO2       1560
#define IDR_PROJECTPANEL_ICO_DM     1561
#define IDR_CLIPBOARDPANEL_ICO2     1562
#define IDR_CLIPBOARDPANEL_ICO_DM   1563
#define IDR_ASCIIPANEL_ICO2         1564
#define IDR_ASCIIPANEL_ICO_DM       1565
#define IDR_FIND_RESULT_ICO2        1566
#define IDR_FIND_RESULT_ICO_DM      1567

#define IDR_PINTAB                  1568
#define IDR_PINTAB_INACT            1569
#define IDR_PINTAB_INACT_EMPTY      1570
#define IDR_PINTAB_HOVERIN          1571
#define IDR_PINTAB_HOVERONTAB       1572
#define IDR_PINTAB_PINNED           1573
#define IDR_PINTAB_PINNEDHOVERIN    1574
#define IDR_PINTAB_DM               1575
#define IDR_PINTAB_INACT_DM         1576
#define IDR_PINTAB_INACT_EMPTY_DM   1577
#define IDR_PINTAB_HOVERIN_DM       1578
#define IDR_PINTAB_HOVERONTAB_DM    1579
#define IDR_PINTAB_PINNED_DM        1580
#define IDR_PINTAB_PINNEDHOVERIN_DM 1581

#define ID_MACRO                           20000
//                                     O     .
//                                     C     .
//                                     C     .
//                                     U     .
//                                     P     .
//                                     I     .
//                                     E     .
//                                     D     .
#define ID_MACRO_LIMIT                     20499


#define ID_USER_CMD                        21000
//                                     O     .
//                                     C     .
//                                     C     .
//                                     U     .
//                                     P     .
//                                     I     .
//                                     E     .
//                                     D     .
#define ID_USER_CMD_LIMIT                  21499


#define ID_PLUGINS_CMD                     22000
//                                     O     .
//                                     C     .
//                                     C     .
//                                     U     .
//                                     P     .
//                                     I     .
//                                     E     .
//                                     D     .
#define ID_PLUGINS_CMD_LIMIT               22999


#define ID_PLUGINS_CMD_DYNAMIC             23000
//                                     O     .
//                                     C     .
//                                     C     .
//                                     U     .
//                                     P     .
//                                     I     .
//                                     E     .
//                                     D     .
#define ID_PLUGINS_CMD_DYNAMIC_LIMIT       24999


#define MARKER_PLUGINS          1
#define MARKER_PLUGINS_LIMIT   15

#define INDICATOR_PLUGINS          9  // indicators 8 and below are reserved by Notepad++ (URL_INDIC=8)
#define INDICATOR_PLUGINS_LIMIT   20  // indicators 21 and up are reserved by Notepad++ (SCE_UNIVERSAL_FOUND_STYLE_EXT5=21)



//#define IDM 40000

#define IDCMD 50000
    //#define IDM_EDIT_AUTOCOMPLETE                (IDCMD+0)
    //#define IDM_EDIT_AUTOCOMPLETE_CURRENTFILE    (IDCMD+1)

	#define IDC_PREV_DOC                    (IDCMD+3)
	#define IDC_NEXT_DOC                    (IDCMD+4)
	#define IDC_EDIT_TOGGLEMACRORECORDING    (IDCMD+5)
    //#define IDC_KEY_HOME                    (IDCMD+6)
    //#define IDC_KEY_END                        (IDCMD+7)
    //#define IDC_KEY_SELECT_2_HOME            (IDCMD+8)
    //#define IDC_KEY_SELECT_2_END            (IDCMD+9)

	#define IDC_NEXT_TAB                   IDC_NEXT_DOC
	#define IDC_PREV_TAB                   IDC_PREV_DOC

#define IDCMD_LIMIT                        (IDCMD+20)

#define IDSCINTILLA 60000
	#define IDSCINTILLA_KEY_HOME        (IDSCINTILLA+0)
	#define IDSCINTILLA_KEY_HOME_WRAP   (IDSCINTILLA+1)
	#define IDSCINTILLA_KEY_END         (IDSCINTILLA+2)
	#define IDSCINTILLA_KEY_END_WRAP    (IDSCINTILLA+3)
	#define IDSCINTILLA_KEY_LINE_DUP    (IDSCINTILLA+4)
	#define IDSCINTILLA_KEY_LINE_CUT    (IDSCINTILLA+5)
	#define IDSCINTILLA_KEY_LINE_DEL    (IDSCINTILLA+6)
	#define IDSCINTILLA_KEY_LINE_TRANS  (IDSCINTILLA+7)
	#define IDSCINTILLA_KEY_LINE_COPY   (IDSCINTILLA+8)
	#define IDSCINTILLA_KEY_CUT         (IDSCINTILLA+9)
	#define IDSCINTILLA_KEY_COPY        (IDSCINTILLA+10)
	#define IDSCINTILLA_KEY_PASTE       (IDSCINTILLA+11)
	#define IDSCINTILLA_KEY_DEL         (IDSCINTILLA+12)
	#define IDSCINTILLA_KEY_SELECTALL   (IDSCINTILLA+13)
	#define IDSCINTILLA_KEY_OUTDENT     (IDSCINTILLA+14)
	#define IDSCINTILLA_KEY_UNDO        (IDSCINTILLA+15)
	#define IDSCINTILLA_KEY_REDO        (IDSCINTILLA+16)
#define IDSCINTILLA_LIMIT        (IDSCINTILLA+30)

#define IDD_FILEVIEW_DIALOG                1000

#define IDD_CREATE_DIRECTORY            1100
#define IDC_STATIC_CURRENT_FOLDER       1101
#define IDC_EDIT_NEW_FOLDER             1102

#define IDD_INSERT_INPUT_TEXT            1200
#define IDC_EDIT_INPUT_VALUE            1201
#define IDC_STATIC_INPUT_TITLE            1202
#define IDC_ICON_INPUT_ICON                1203

#define IDR_M30_MENU                    1500

#define IDR_SYSTRAYPOPUP_MENU            1501

// #define IDD_FIND_REPLACE_DLG        1600

#define IDD_ABOUTBOX 1700
#define IDC_LICENCE_EDIT 1701
#define IDC_HOME_ADDR        1702
#define IDC_EMAIL_ADDR        1703
#define IDC_ONLINEHELP_ADDR 1704
#define IDC_AUTHOR_NAME 1705
#define IDC_BUILD_DATETIME 1706
#define IDC_VERSION_BIT 1707

#define IDD_DEBUGINFOBOX 1750
#define IDC_DEBUGINFO_EDIT 1751
#define IDC_DEBUGINFO_COPYLINK 1752

#define IDD_DOSAVEORNOTBOX  1760
#define IDC_DOSAVEORNOTTEXT 1761

#define IDD_DOSAVEALLBOX    1765
#define IDC_DOSAVEALLTEXT   1766

//#define IDD_USER_DEFINE_BOX       1800
//#define IDD_RUN_DLG               1900
//#define IDD_MD5FROMFILES_DLG      1920
//#define IDD_MD5FROMTEXT_DLG       1930

#define IDD_GOLINE        2000
#define ID_GOLINE_EDIT    (IDD_GOLINE + 1)
#define ID_CURRLINE_EDIT   (IDD_GOLINE + 2)
#define ID_LASTLINE        (IDD_GOLINE + 3)
#define ID_URHERE_STATIC           (IDD_GOLINE + 4)
#define ID_UGO_STATIC                 (IDD_GOLINE + 5)
#define ID_NOMORETHAN_STATIC   (IDD_GOLINE + 6)
#define IDC_RADIO_GOTOLINE   (IDD_GOLINE + 7)
#define IDC_RADIO_GOTOOFFSET   (IDD_GOLINE + 8)

// voir columnEditor_rc.h
//#define IDD_COLUMNEDIT   2020


//#define IDD_COLOUR_POPUP   2100

// See WordStyleDlgRes.h
//#define IDD_STYLER_DLG    2200
//#define IDD_GLOBAL_STYLER_DLG    2300

//#define IDD_VALUE_DLG       2400
//#define IDC_VALUE_STATIC  2401
//#define IDC_VALUE_EDIT      2402

#define IDD_BUTTON_DLG       2410
#define IDC_RESTORE_BUTTON  2411

// see TaskListDlg_rc.h
//#define IDD_TASKLIST_DLG    2450
#define IDD_SETTING_DLG    2500



//See ShortcutMapper_rc.h
//#define IDD_SHORTCUTMAPPER_DLG      2600

//See ansiCharPanel_rc.h
//#define IDD_ANSIASCII_PANEL      2700

//See clipboardHistoryPanel_rc.h
//#define IDD_CLIPBOARDHISTORY_PANEL      2800

//See findCharsInRange_rc.h
//#define IDD_FINDCHARACTERS      2900

//See VerticalFileSwitcher_rc.h
//#define IDD_DOCLIST      3000

//See ProjectPanel_rc.h
//#define IDD_PROJECTPANEL      3100
//#define IDD_FILERELOCALIZER_DIALOG  3200

//See documentMap_rc.h
//#define IDD_DOCUMENTMAP      3300

//See functionListPanel_rc.h
//#define IDD_FUNCLIST_PANEL   3400

//See fileBrowser_rc.h
//#define IDD_FILEBROWSER 3500

//See documentSnapshot_rc.h
//#define IDD_DOCUMENSNAPSHOT 3600

// See regExtDlg.h
//#define IDD_REGEXT 4000

// See shortcutRc.h
//#define IDD_SHORTCUT_DLG      5000

// See pluginsAdminRes.h
//#define IDD_PLUGINSADMIN_DLG 5500

// See preference.rc
//#define IDD_PREFERENCE_BOX 6000

#define NOTEPADPLUS_USER_INTERNAL     (WM_USER + 0000)
	#define NPPM_INTERNAL_USERCMDLIST_MODIFIED          (NOTEPADPLUS_USER_INTERNAL + 1)
	#define NPPM_INTERNAL_CMDLIST_MODIFIED              (NOTEPADPLUS_USER_INTERNAL + 2)
	#define NPPM_INTERNAL_MACROLIST_MODIFIED            (NOTEPADPLUS_USER_INTERNAL + 3)
	#define NPPM_INTERNAL_PLUGINCMDLIST_MODIFIED        (NOTEPADPLUS_USER_INTERNAL + 4)
	#define NPPM_INTERNAL_CLEARSCINTILLAKEY             (NOTEPADPLUS_USER_INTERNAL + 5)
	#define NPPM_INTERNAL_BINDSCINTILLAKEY              (NOTEPADPLUS_USER_INTERNAL + 6)
	#define NPPM_INTERNAL_SCINTILLAKEYMODIFIED          (NOTEPADPLUS_USER_INTERNAL + 7)
	#define NPPM_INTERNAL_SCINTILLAFINDERCOLLAPSE       (NOTEPADPLUS_USER_INTERNAL + 8)
	#define NPPM_INTERNAL_SCINTILLAFINDERUNCOLLAPSE     (NOTEPADPLUS_USER_INTERNAL + 9)
	#define NPPM_INTERNAL_DISABLEAUTOUPDATE             (NOTEPADPLUS_USER_INTERNAL + 10)
	#define NPPM_INTERNAL_SETTING_HISTORY_SIZE          (NOTEPADPLUS_USER_INTERNAL + 11)
	#define NPPM_INTERNAL_ISTABBARREDUCED               (NOTEPADPLUS_USER_INTERNAL + 12)
	#define NPPM_INTERNAL_ISFOCUSEDTAB                  (NOTEPADPLUS_USER_INTERNAL + 13)
	#define NPPM_INTERNAL_GETMENU                       (NOTEPADPLUS_USER_INTERNAL + 14)
	#define NPPM_INTERNAL_CLEARINDICATOR                (NOTEPADPLUS_USER_INTERNAL + 15)
	#define NPPM_INTERNAL_SCINTILLAFINDERCOPY           (NOTEPADPLUS_USER_INTERNAL + 16)
	#define NPPM_INTERNAL_SCINTILLAFINDERSELECTALL      (NOTEPADPLUS_USER_INTERNAL + 17)
	#define NPPM_INTERNAL_SETCARETWIDTH                 (NOTEPADPLUS_USER_INTERNAL + 18)
	#define NPPM_INTERNAL_SETCARETBLINKRATE             (NOTEPADPLUS_USER_INTERNAL + 19)
	#define NPPM_INTERNAL_CLEARINDICATORTAGMATCH        (NOTEPADPLUS_USER_INTERNAL + 20)
	#define NPPM_INTERNAL_CLEARINDICATORTAGATTR         (NOTEPADPLUS_USER_INTERNAL + 21)
	#define NPPM_INTERNAL_SWITCHVIEWFROMHWND            (NOTEPADPLUS_USER_INTERNAL + 22)
	#define NPPM_INTERNAL_UPDATETITLEBAR                (NOTEPADPLUS_USER_INTERNAL + 23)
	#define NPPM_INTERNAL_CANCEL_FIND_IN_FILES          (NOTEPADPLUS_USER_INTERNAL + 24)
	#define NPPM_INTERNAL_RELOADNATIVELANG              (NOTEPADPLUS_USER_INTERNAL + 25)
	#define NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED        (NOTEPADPLUS_USER_INTERNAL + 26)
	#define NPPM_INTERNAL_SCINTILLAFINDERCLEARALL       (NOTEPADPLUS_USER_INTERNAL + 27)
	#define NPPM_INTERNAL_CHANGETABBARICONSET           (NOTEPADPLUS_USER_INTERNAL + 28)
	#define NPPM_INTERNAL_SET_TAB_SETTINGS              (NOTEPADPLUS_USER_INTERNAL + 29)
	#define NPPM_INTERNAL_SETTOOLICONSSET               (NOTEPADPLUS_USER_INTERNAL + 30)
	#define NPPM_INTERNAL_RELOADSTYLERS                 (NOTEPADPLUS_USER_INTERNAL + 31)
	#define NPPM_INTERNAL_DOCORDERCHANGED               (NOTEPADPLUS_USER_INTERNAL + 32)
	#define NPPM_INTERNAL_SETMULTISELCTION              (NOTEPADPLUS_USER_INTERNAL + 33)
	#define NPPM_INTERNAL_SCINTILLAFINDEROPENALL        (NOTEPADPLUS_USER_INTERNAL + 34)
	#define NPPM_INTERNAL_RECENTFILELIST_UPDATE         (NOTEPADPLUS_USER_INTERNAL + 35)
	#define NPPM_INTERNAL_RECENTFILELIST_SWITCH         (NOTEPADPLUS_USER_INTERNAL + 36)
	#define NPPM_INTERNAL_GETSCINTEDTVIEW               (NOTEPADPLUS_USER_INTERNAL + 37)
	#define NPPM_INTERNAL_ENABLESNAPSHOT                (NOTEPADPLUS_USER_INTERNAL + 38)
	#define NPPM_INTERNAL_SAVECURRENTSESSION            (NOTEPADPLUS_USER_INTERNAL + 39)
	#define NPPM_INTERNAL_FINDINFINDERDLG               (NOTEPADPLUS_USER_INTERNAL + 40)
	#define NPPM_INTERNAL_REMOVEFINDER                  (NOTEPADPLUS_USER_INTERNAL + 41)
	#define NPPM_INTERNAL_RELOADSCROLLTOEND			    (NOTEPADPLUS_USER_INTERNAL + 42)  // Used by Monitoring feature
	#define NPPM_INTERNAL_FINDKEYCONFLICTS              (NOTEPADPLUS_USER_INTERNAL + 43)
	#define NPPM_INTERNAL_SCROLLBEYONDLASTLINE          (NOTEPADPLUS_USER_INTERNAL + 44)
	#define NPPM_INTERNAL_SETWORDCHARS                  (NOTEPADPLUS_USER_INTERNAL + 45)
	#define NPPM_INTERNAL_EXPORTFUNCLISTANDQUIT         (NOTEPADPLUS_USER_INTERNAL + 46)
	#define NPPM_INTERNAL_PRNTANDQUIT                   (NOTEPADPLUS_USER_INTERNAL + 47)
	#define NPPM_INTERNAL_SAVEBACKUP        		    (NOTEPADPLUS_USER_INTERNAL + 48)
	#define NPPM_INTERNAL_STOPMONITORING                (NOTEPADPLUS_USER_INTERNAL + 49)  // Used by Monitoring feature
	#define NPPM_INTERNAL_EDGEBACKGROUND                (NOTEPADPLUS_USER_INTERNAL + 50)
	#define NPPM_INTERNAL_EDGEMULTISETSIZE              (NOTEPADPLUS_USER_INTERNAL + 51)
	#define NPPM_INTERNAL_UPDATECLICKABLELINKS          (NOTEPADPLUS_USER_INTERNAL + 52)
	#define NPPM_INTERNAL_SCINTILLAFINDERWRAP           (NOTEPADPLUS_USER_INTERNAL + 53)
	#define NPPM_INTERNAL_MINIMIZED_TRAY                (NOTEPADPLUS_USER_INTERNAL + 54)
	#define NPPM_INTERNAL_SCINTILLAFINDERCOPYVERBATIM   (NOTEPADPLUS_USER_INTERNAL + 55)
	#define NPPM_INTERNAL_FINDINPROJECTS                (NOTEPADPLUS_USER_INTERNAL + 56)
	#define NPPM_INTERNAL_SCINTILLAFINDERPURGE          (NOTEPADPLUS_USER_INTERNAL + 57)
	#define NPPM_INTERNAL_UPDATETEXTZONEPADDING         (NOTEPADPLUS_USER_INTERNAL + 58)
	#define NPPM_INTERNAL_REFRESHDARKMODE				(NOTEPADPLUS_USER_INTERNAL + 59)
	#define NPPM_INTERNAL_SCINTILLAFINDERCOPYPATHS      (NOTEPADPLUS_USER_INTERNAL + 60)
	#define NPPM_INTERNAL_REFRESHWORKDIR                (NOTEPADPLUS_USER_INTERNAL + 61)
	#define NPPM_INTERNAL_VIRTUALSPACE                  (NOTEPADPLUS_USER_INTERNAL + 62)
	#define NPPM_INTERNAL_CARETLINEFRAME                (NOTEPADPLUS_USER_INTERNAL + 63)
	#define NPPM_INTERNAL_CRLFFORMCHANGED               (NOTEPADPLUS_USER_INTERNAL + 64)
	#define NPPM_INTERNAL_CRLFLAUNCHSTYLECONF           (NOTEPADPLUS_USER_INTERNAL + 65)
	#define NPPM_INTERNAL_LAUNCHPREFERENCES             (NOTEPADPLUS_USER_INTERNAL + 66)
	#define NPPM_INTERNAL_ENABLECHANGEHISTORY           (NOTEPADPLUS_USER_INTERNAL + 67)
	#define NPPM_INTERNAL_CLEANSMARTHILITING            (NOTEPADPLUS_USER_INTERNAL + 68)
	#define NPPM_INTERNAL_CLEANBRACEMATCH               (NOTEPADPLUS_USER_INTERNAL + 69)
	#define NPPM_INTERNAL_WINDOWSSESSIONEXIT            (NOTEPADPLUS_USER_INTERNAL + 70)
	#define NPPM_INTERNAL_RESTOREFROMTRAY               (NOTEPADPLUS_USER_INTERNAL + 71)
	#define NPPM_INTERNAL_SETNPC                        (NOTEPADPLUS_USER_INTERNAL + 72)
	#define NPPM_INTERNAL_NPCFORMCHANGED                (NOTEPADPLUS_USER_INTERNAL + 73)
	#define NPPM_INTERNAL_NPCLAUNCHSTYLECONF            (NOTEPADPLUS_USER_INTERNAL + 74)
	#define NPPM_INTERNAL_CLOSEDOC                      (NOTEPADPLUS_USER_INTERNAL + 75)
	#define NPPM_INTERNAL_EXTERNALLEXERBUFFER           (NOTEPADPLUS_USER_INTERNAL + 76)
	#define NPPM_INTERNAL_CHECKUNDOREDOSTATE            (NOTEPADPLUS_USER_INTERNAL + 77)
	#define NPPM_INTERNAL_LINECUTCOPYWITHOUTSELECTION   (NOTEPADPLUS_USER_INTERNAL + 78)
	#define NPPM_INTERNAL_DOCMODIFIEDBYREPLACEALL       (NOTEPADPLUS_USER_INTERNAL + 79)
	#define NPPM_INTERNAL_DRAWTABBARPINBUTTON           (NOTEPADPLUS_USER_INTERNAL + 80)
	#define NPPM_INTERNAL_DRAWTABBARCLOSEBUTTON         (NOTEPADPLUS_USER_INTERNAL + 81)
	#define NPPM_INTERNAL_DRAWINACTIVETABBARBUTTON      (NOTEPADPLUS_USER_INTERNAL + 82)
	#define NPPM_INTERNAL_REDUCETABBAR                  (NOTEPADPLUS_USER_INTERNAL + 83)
	//#define NPPM_INTERNAL_LOCKTABBAR                    (NOTEPADPLUS_USER_INTERNAL + 84)
	#define NPPM_INTERNAL_DRAWINACIVETAB                (NOTEPADPLUS_USER_INTERNAL + 85)
	#define NPPM_INTERNAL_DRAWTABTOPBAR                 (NOTEPADPLUS_USER_INTERNAL + 86)
	//#define NPPM_INTERNAL_TABDBCLK2CLOSE                (NOTEPADPLUS_USER_INTERNAL + 87)
	#define NPPM_INTERNAL_VERTICALTABBAR                (NOTEPADPLUS_USER_INTERNAL + 88)
	#define NPPM_INTERNAL_MULTILINETABBAR               (NOTEPADPLUS_USER_INTERNAL + 89)
	#define NPPM_INTERNAL_TOOLBARREDUCE                 (NOTEPADPLUS_USER_INTERNAL + 90)
	#define NPPM_INTERNAL_TOOLBARREDUCESET2             (NOTEPADPLUS_USER_INTERNAL + 91)
	#define NPPM_INTERNAL_TOOLBARENLARGE                (NOTEPADPLUS_USER_INTERNAL + 92)
	#define NPPM_INTERNAL_TOOLBARENLARGESET2            (NOTEPADPLUS_USER_INTERNAL + 93)
	#define NPPM_INTERNAL_TOOLBARSTANDARD               (NOTEPADPLUS_USER_INTERNAL + 94)
	#define NPPM_INTERNAL_LINENUMBER                    (NOTEPADPLUS_USER_INTERNAL + 95)
	#define NPPM_INTERNAL_SYMBOLMARGIN                  (NOTEPADPLUS_USER_INTERNAL + 96)
	#define NPPM_INTERNAL_HILITECURRENTLINE             (NOTEPADPLUS_USER_INTERNAL + 97)
	#define NPPM_INTERNAL_FOLDSYMBOLSIMPLE              (NOTEPADPLUS_USER_INTERNAL + 98)
	#define NPPM_INTERNAL_FOLDSYMBOLARROW               (NOTEPADPLUS_USER_INTERNAL + 99)
	#define NPPM_INTERNAL_FOLDSYMBOLCIRCLE              (NOTEPADPLUS_USER_INTERNAL + 100)
	#define NPPM_INTERNAL_FOLDSYMBOLBOX                 (NOTEPADPLUS_USER_INTERNAL + 101)
	#define NPPM_INTERNAL_FOLDSYMBOLNONE                (NOTEPADPLUS_USER_INTERNAL + 102)
	#define NPPM_INTERNAL_LWDEF                         (NOTEPADPLUS_USER_INTERNAL + 103)
	#define NPPM_INTERNAL_LWALIGN                       (NOTEPADPLUS_USER_INTERNAL + 104)
	#define NPPM_INTERNAL_LWINDENT                      (NOTEPADPLUS_USER_INTERNAL + 105)
	#define NPPM_INTERNAL_CHECKDOCSTATUS                (NOTEPADPLUS_USER_INTERNAL + 106)
	#define NPPM_INTERNAL_HIDEMENURIGHTSHORTCUTS        (NOTEPADPLUS_USER_INTERNAL + 107)
	//#define NPPM_INTERNAL_                            (NOTEPADPLUS_USER_INTERNAL + 108)
	#define NPPM_INTERNAL_SQLBACKSLASHESCAPE            (NOTEPADPLUS_USER_INTERNAL + 109)

// See Notepad_plus_msgs.h
//#define NPPMSG   (WM_USER + 1000)


#define SCINTILLA_USER     (WM_USER + 2000)


#define MACRO_USER    (WM_USER + 4000)
//	#define WM_GETCURRENTMACROSTATUS (MACRO_USER + 01) // Replaced with NPPM_GETCURRENTMACROSTATUS
	#define WM_MACRODLGRUNMACRO       (MACRO_USER + 02)


// See Notepad_plus_msgs.h
//#define RUNCOMMAND_USER    (WM_USER + 3000)
#define SPLITTER_USER      (WM_USER + 4000)
#define WORDSTYLE_USER     (WM_USER + 5000)
#define COLOURPOPUP_USER   (WM_USER + 6000)
#define BABYGRID_USER      (WM_USER + 7000)

//#define IDD_DOCKING_MNG (IDM  + 7000)

#define MENUINDEX_FILE     0
#define MENUINDEX_EDIT     1
#define MENUINDEX_SEARCH   2
#define MENUINDEX_VIEW     3
#define MENUINDEX_FORMAT   4
#define MENUINDEX_LANGUAGE 5
#define MENUINDEX_SETTINGS 6
#define MENUINDEX_TOOLS    7
#define MENUINDEX_MACRO    8
#define MENUINDEX_RUN      9
#define MENUINDEX_PLUGINS  10
#define MENUINDEX_WINDOW   11
#define MENUINDEX_HELP     12
#define MENUINDEX_LIST     14
