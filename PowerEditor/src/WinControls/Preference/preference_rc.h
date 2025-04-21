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

#pragma once

// Keep the value unicity of each control ID so the controls could be moved among the different dialogs if needs.
// Take the unused value as new control ID, and keep the value in order.

#define IDD_PREFERENCE_BOX                             6000
#define IDC_BUTTON_CLOSE                               6001
#define IDC_LIST_DLGTITLE                              6002

#define IDD_PREFERENCE_SUB_TOOLBAR                     6010
#define IDC_TOOLBAR_GB_COLORIZATION                    6011
#define IDC_RADIO_COMPLETE                             6012
#define IDC_RADIO_PARTIAL                              6013
#define IDC_TOOLBAR_GB_COLORCHOICE                     6014
#define IDC_RADIO_DEFAULTCOLOR                         6015
#define IDC_RADIO_RED                                  6016
#define IDC_RADIO_GREEN                                6017
#define IDC_RADIO_BLUE                                 6018
#define IDC_RADIO_PURPLE                               6019
#define IDC_RADIO_CYAN                                 6020
#define IDC_RADIO_OLIVE                                6021
#define IDC_RADIO_YELLOW                               6022
#define IDC_RADIO_ACCENTCOLOR                          6023
#define IDC_RADIO_CUSTOMCOLOR                          6024
#define IDD_ACCENT_TIP_STATIC                          6025

#define IDD_PREFERENCE_SUB_TABBAR                      6040

#define IDD_PREFERENCE_SUB_GENRAL                      6100

#define IDC_CHECK_HIDE                                 6102
#define IDC_RADIO_SMALLICON                            6103
#define IDC_RADIO_BIGICON                              6104
#define IDC_RADIO_STANDARD                             6105

#define IDC_CHECK_REDUCE                               6107
#define IDC_CHECK_LOCK                                 6108
#define IDC_CHECK_DRAWINACTIVE                         6109
#define IDC_CHECK_ORANGE                               6110
#define IDC_CHECK_INACTTABDRAWBUTTON                   6111
#define IDC_CHECK_ENABLETABCLOSE                       6112
#define IDC_CHECK_DBCLICK2CLOSE                        6113
#define IDC_CHECK_ENABLEDOCSWITCHER                    6114
#define IDC_CHECK_ENABLETABPIN                         6115
#define IDC_CHECK_KEEPINSAMEDIR                        6116
#define IDC_CHECK_STYLEMRU                             6117
#define IDC_CHECK_TAB_HIDE                             6118
#define IDC_CHECK_TAB_MULTILINE                        6119
#define IDC_CHECK_TAB_VERTICAL                         6120
#define IDC_CHECK_TAB_LAST_EXIT                        6121
#define IDC_CHECK_HIDEMENUBAR                          6122
#define IDC_LOCALIZATION_GB_STATIC                     6123
#define IDC_COMBO_LOCALIZATION                         6124


#define IDC_CHECK_TAB_ALTICONS                         6128
#define IDC_RADIO_SMALLICON2                           6129
#define IDC_RADIO_BIGICON2                             6130
#define IDC_MENU_GB_STATIC                             6131
#define IDC_CHECK_HIDERIGHTSHORTCUTSOFMENUBAR          6132
#define IDC_STATUSBAR_GB_STATIC                        6133
#define IDC_CHECK_HIDESTATUSBAR                        6134
#define IDC_CHECK_SHOWONLYPINNEDBUTTON                 6135

#define IDD_PREFERENCE_SUB_MULTIINSTANCE               6150
#define IDC_MULTIINST_GB_STATIC                        6151
#define IDC_SESSIONININST_RADIO                        6152
#define IDC_MULTIINST_RADIO                            6153
#define IDC_MONOINST_RADIO                             6154
#define IDD_STATIC_RESTARTNOTE                         6155


#define IDC_WORDCHARLIST_GB_STATIC                     6161
#define IDC_RADIO_WORDCHAR_DEFAULT                     6162
#define IDC_RADIO_WORDCHAR_CUSTOM                      6163
#define IDC_WORDCHAR_CUSTOM_EDIT                       6164
#define IDD_WORDCHAR_QUESTION_BUTTON                   6165
#define IDD_STATIC_WORDCHAR_WARNING                    6166


#define IDC_DATETIMEFORMAT_GB_STATIC                   6171
#define IDD_DATETIMEFORMAT_STATIC                      6172
#define IDC_DATETIMEFORMAT_EDIT                        6173
#define IDD_DATETIMEFORMAT_RESULT_STATIC               6174
#define IDD_DATETIMEFORMAT_REVERSEORDER_CHECK          6175


#define IDC_PANEL_IGNORESESSION_GB_STATIC              6181
#define IDD_STATIC_PANELSTATE_DESCRIPTION              6182
#define IDC_CHECK_CLIPBOARDHISTORY                     6183
#define IDC_CHECK_DOCLIST                              6184
#define IDC_CHECK_CHARPANEL                            6185
#define IDC_CHECK_FILEBROWSER                          6186
#define IDC_CHECK_PROJECTPANEL                         6187
#define IDC_CHECK_DOCMAP                               6188
#define IDC_CHECK_FUNCLIST                             6189
#define IDC_CHECK_PLUGINPANEL                          6190

#define IDD_PREFERENCE_SUB_EDITING                     6200
#define IDC_FMS_GB_STATIC                              6201
#define IDC_RADIO_SIMPLE                               6202
#define IDC_RADIO_ARROW                                6203
#define IDC_RADIO_CIRCLE                               6204
#define IDC_RADIO_BOX                                  6205

#define IDC_CHECK_LINENUMBERMARGE                      6206
#define IDC_CHECK_BOOKMARKMARGE                        6207

#define IDC_PADDING_STATIC                             6208
#define IDC_PADDINGLEFT_STATIC                         6209
#define IDC_PADDINGRIGHT_STATIC                        6210

#define IDC_VES_GB_STATIC                              6211
#define IDC_DISTRACTIONFREE_STATIC                     6212
#define IDC_CHECK_EDGEBGMODE                           6213
#define IDC_CHECK_LINECUTCOPYWITHOUTSELECTION          6214
#define IDC_CHECK_SMOOTHFONT                           6215

#define IDC_CARETSETTING_STATIC                        6216
#define IDC_WIDTH_STATIC                               6217
#define IDC_WIDTH_COMBO                                6218
#define IDC_BLINKRATE_STATIC                           6219
#define IDC_CARETBLINKRATE_SLIDER                      6220
#define IDC_CARETBLINKRATE_F_STATIC                    6221
#define IDC_CARETBLINKRATE_S_STATIC                    6222
#define IDC_CHECK_CHANGHISTORYMARGIN                   6223
#define IDC_DISTRACTIONFREE_SLIDER                     6224
#define IDC_CHECK_SELECTEDTEXTSINGLECOLOR              6225

#define IDC_RADIO_FOLDMARGENONE                        6226

#define IDC_LW_GB_STATIC                               6227
#define IDC_RADIO_LWDEF                                6228
#define IDC_RADIO_LWALIGN                              6229
#define IDC_RADIO_LWINDENT                             6230

#define IDC_BORDERWIDTH_STATIC                         6231
#define IDC_BORDERWIDTHVAL_STATIC                      6232
#define IDC_BORDERWIDTH_SLIDER                         6233
#define IDC_CHECK_DISABLEADVANCEDSCROLL                6234
#define IDC_CHECK_NOEDGE                               6235
#define IDC_CHECK_SCROLLBEYONDLASTLINE                 6236

#define IDC_STATIC_MULTILNMODE_TIP                     6237
#define IDC_COLUMNPOS_EDIT                             6238
#define IDC_CHECK_RIGHTCLICKKEEPSSELECTION             6239
#define IDC_PADDINGLEFT_SLIDER                         6240
#define IDC_PADDINGRIGHT_SLIDER                        6241
#define IDC_PADDINGLEFTVAL_STATIC                      6242
#define IDC_PADDINGRIGHTVAL_STATIC                     6243
#define IDC_DISTRACTIONFREEVAL_STATIC                  6244

#define IDC_CHECK_VIRTUALSPACE                         6245

#define IDC_CHECK_FOLDINGTOGGLE                        6246

#define IDC_GB_STATIC_CRLF                             6247
#define IDC_RADIO_ROUNDCORNER_CRLF                     6248
#define IDC_RADIO_PLEINTEXT_CRLF                       6249
#define IDC_CHECK_WITHCUSTOMCOLOR_CRLF                 6250
#define IDC_BUTTON_LAUNCHSTYLECONF_CRLF                6251

#define IDC_GB_STATIC_NPC                              6252
#define IDC_BUTTON_NPC_NOTE                            6253
#define IDC_RADIO_NPC_ABBREVIATION                     6254
#define IDC_RADIO_NPC_CODEPOINT                        6255
#define IDC_CHECK_NPC_COLOR                            6256
#define IDC_BUTTON_NPC_LAUNCHSTYLECONF                 6257
#define IDC_CHECK_NPC_INCLUDECCUNIEOL                  6258
#define IDC_CHECK_NPC_NOINPUTC0                        6259
#define IDC_STATIC_NPC_APPEARANCE                      6260

#define IDD_PREFERENCE_SUB_DELIMITER                   6250
#define IDC_DELIMITERSETTINGS_GB_STATIC                6251
#define IDD_STATIC_OPENDELIMITER                       6252
#define IDC_EDIT_OPENDELIMITER                         6253
#define IDC_EDIT_CLOSEDELIMITER                        6254
#define IDD_STATIC_CLOSEDELIMITER                      6255
#define IDD_SEVERALLINEMODEON_CHECK                    6256
#define IDD_STATIC_BLABLA                              6257
#define IDD_STATIC_BLABLA2NDLINE                       6258

#define IDD_PREFERENCE_SUB_CLOUD_LINK                  6260
#define IDC_SETTINGSONCLOUD_WARNING_STATIC             6261
#define IDC_SETTINGSONCLOUD_GB_STATIC                  6262
#define IDC_NOCLOUD_RADIO                              6263
#define IDC_URISCHEMES_STATIC                          6264
#define IDC_URISCHEMES_EDIT                            6265

#define IDC_WITHCLOUD_RADIO                            6267
#define IDC_CLOUDPATH_EDIT                             6268
#define IDD_CLOUDPATH_BROWSE_BUTTON                    6269

#define IDD_PREFERENCE_SUB_SEARCHENGINE                6270
#define IDC_SEARCHENGINES_GB_STATIC                    6271
#define IDC_SEARCHENGINE_DUCKDUCKGO_RADIO              6272
#define IDC_SEARCHENGINE_GOOGLE_RADIO                  6273

#define IDC_SEARCHENGINE_YAHOO_RADIO                   6275
#define IDC_SEARCHENGINE_CUSTOM_RADIO                  6276
#define IDC_SEARCHENGINE_EDIT                          6277
#define IDD_SEARCHENGINE_NOTE_STATIC                   6278
#define IDC_SEARCHENGINE_STACKOVERFLOW_RADIO           6279

#define IDD_PREFERENCE_SUB_MARGING_BORDER_EDGE         6290
#define IDC_LINENUMBERMARGE_GB_STATIC                  6291
#define IDC_RADIO_DYNAMIC                              6292
#define IDC_RADIO_CONSTANT                             6293
#define IDC_BUTTON_VES_TIP                             6294
#define IDC_GB_CHANGHISTORY                            6295
#define IDC_CHECK_CHANGHISTORYINDICATOR                6296

#define IDD_PREFERENCE_SUB_MISC                        6300
#define IDC_TABSETTING_GB_STATIC                       6301
#define IDC_RADIO_REPLACEBYSPACE                       6302
#define IDC_TABSIZE_STATIC                             6303
#define IDC_HISTORY_GB_STATIC                          6304
#define IDC_CHECK_DONTCHECKHISTORY                     6305
#define IDC_MAXNBFILE_STATIC                           6306
#define IDC_COMBO_SYSTRAY_ACTION_CHOICE                6307
#define IDC_SYSTRAY_STATIC                             6308
#define IDC_CHECK_REMEMBERSESSION                      6309
#define IDC_INDENTUSING_STATIC                         6310
#define IDC_RADIO_USINGTAB                             6311
#define IDC_FILEAUTODETECTION_STATIC                   6312
#define IDC_CHECK_UPDATESILENTLY                       6313
#define IDC_RADIO_BKNONE                               6315
#define IDC_RADIO_BKSIMPLE                             6316
#define IDC_RADIO_BKVERBOSE                            6317
#define IDC_CLICKABLELINK_STATIC                       6318
#define IDC_CHECK_CLICKABLELINK_ENABLE                 6319
#define IDC_CHECK_CLICKABLELINK_NOUNDERLINE            6320
#define IDC_EDIT_SESSIONFILEEXT                        6321
#define IDC_SESSIONFILEEXT_STATIC                      6322
#define IDC_CHECK_AUTOUPDATE                           6323
#define IDC_DOCUMENTSWITCHER_STATIC                    6324
#define IDC_CHECK_UPDATEGOTOEOF                        6325
#define IDC_CHECK_ENABLSMARTHILITE                     6326
#define IDC_CHECK_ENABLTAGSMATCHHILITE                 6327
#define IDC_CHECK_ENABLTAGATTRHILITE                   6328
#define IDC_TAGMATCHEDHILITE_STATIC                    6329
#define IDC_CHECK_HIGHLITENONEHTMLZONE                 6330
#define IDC_CHECK_SHORTTITLE                           6331
#define IDC_CHECK_SMARTHILITECASESENSITIVE             6332
#define IDC_SMARTHILITING_STATIC                       6333
#define IDC_CHECK_DETECTENCODING                       6334
#define IDC_CHECK_BACKSLASHISESCAPECHARACTERFORSQL     6335
#define IDC_EDIT_WORKSPACEFILEEXT                      6336
#define IDC_WORKSPACEFILEEXT_STATIC                    6337
#define IDC_CHECK_SMARTHILITEWHOLEWORDONLY             6338
#define IDC_CHECK_SMARTHILITEUSEFINDSETTINGS           6339
#define IDC_CHECK_SMARTHILITEANOTHERRVIEW              6340


#define IDC_DOCUMENTPEEK_STATIC                        6344
#define IDC_CHECK_ENABLEDOCPEEKER                      6345
#define IDC_CHECK_ENABLEDOCPEEKONMAP                   6346
#define IDC_COMBO_FILEUPDATECHOICE                     6347
#define IDC_CHECK_DIRECTWRITE_ENABLE                   6349
#define IDC_CHECK_CLICKABLELINK_FULLBOXMODE            6350
#define IDC_MARKALL_STATIC                             6351
#define IDC_CHECK_MARKALLCASESENSITIVE                 6352
#define IDC_CHECK_MARKALLWHOLEWORDONLY                 6353
#define IDC_SMARTHILITEMATCHING_STATIC                 6354
#define IDC_CHECK_MUTE_SOUNDS                          6360
#define IDC_CHECK_SAVEALLCONFIRM                       6361

#define IDC_COMBO_SC_TECHNOLOGY_CHOICE                 6362
#define IDC_SC_TECHNOLOGY_STATIC                       6363

#define IDD_PREFERENCE_SUB_NEWDOCUMENT                 6400
#define IDC_FORMAT_GB_STATIC                           6401
#define IDC_RADIO_F_WIN                                6402
#define IDC_RADIO_F_UNIX                               6403
#define IDC_RADIO_F_MAC                                6404
#define IDC_ENCODING_STATIC                            6405
#define IDC_RADIO_ANSI                                 6406
#define IDC_RADIO_UTF8SANSBOM                          6407
#define IDC_RADIO_UTF8                                 6408
#define IDC_RADIO_UTF16BIG                             6409
#define IDC_RADIO_UTF16SMALL                           6410
#define IDC_DEFAULTLANG_STATIC                         6411
#define IDC_COMBO_DEFAULTLANG                          6412
#define IDC_OPENSAVEDIR_GR_STATIC                      6413
#define IDC_OPENSAVEDIR_FOLLOWCURRENT_RADIO            6414
#define IDC_OPENSAVEDIR_REMEMBERLAST_RADIO             6415
#define IDC_OPENSAVEDIR_ALWAYSON_RADIO                 6416
#define IDC_OPENSAVEDIR_ALWAYSON_EDIT                  6417
#define IDD_OPENSAVEDIR_ALWAYSON_BROWSE_BUTTON         6418
#define IDC_NEWDOCUMENT_GR_STATIC                      6419
#define IDC_CHECK_OPENANSIASUTF8                       6420
#define IDC_RADIO_OTHERCP                              6421
#define IDC_COMBO_OTHERCP                              6422
#define IDC_GP_STATIC_RECENTFILES                      6423
#define IDC_CHECK_INSUBMENU                            6424
#define IDC_RADIO_ONLYFILENAME                         6425
#define IDC_RADIO_FULLFILENAMEPATH                     6426
#define IDC_RADIO_CUSTOMIZELENTH                       6427

#define IDC_DISPLAY_STATIC                             6429
#define IDC_OPENSAVEDIR_CHECK_DROPFOLDEROPENFILES      6431
#define IDC_CHECK_ADDNEWDOCONSTARTUP                   6432

#define IDD_PREFERENCE_SUB_DEFAULTDIRECTORY            6450
#define IDD_PREFERENCE_SUB_RECENTFILESHISTORY          6460
#define IDC_EDIT_MAXNBFILEVAL                          6461
#define IDC_MAXNBFILE_RANGE_STATIC                     6462
#define IDC_EDIT_CUSTOMIZELENGTHVAL                    6463
#define IDC_CUSTOMIZELENGTH_RANGE_STATIC               6464

#define IDD_PREFERENCE_SUB_LANGUAGE                    6500
#define IDC_LIST_ENABLEDLANG                           6501
#define IDC_LIST_DISABLEDLANG                          6502
#define IDC_BUTTON_REMOVE                              6503
#define IDC_BUTTON_RESTORE                             6504
#define IDC_ENABLEDITEMS_STATIC                        6505
#define IDC_DISABLEDITEMS_STATIC                       6506
#define IDC_CHECK_LANGMENUCOMPACT                      6507
#define IDC_CHECK_LANGMENU_GR_STATIC                   6508
#define IDC_LIST_TABSETTNG                             6509
#define IDC_CHECK_DEFAULTTABVALUE                      6510
#define IDC_GR_TABVALUE_STATIC                         6511
#define IDC_CHECK_BACKSPACEUNINDENT                    6512
#define IDC_EDIT_TABSIZEVAL                            6513

#define IDD_PREFERENCE_SUB_EDITING2                    6520
#define IDC_GB_STATIC_MULTIEDITING                     6521
#define IDC_CHECK_MULTISELECTION                       6522
#define IDC_CHECK_COLUMN2MULTIEDITING                  6523

#define IDD_PREFERENCE_SUB_HIGHLIGHTING                6550

#define IDD_PREFERENCE_SUB_PRINT                       6600
#define IDC_CHECK_PRINTLINENUM                         6601
#define IDC_COLOUROPT_STATIC                           6602
#define IDC_RADIO_WYSIWYG                              6603
#define IDC_RADIO_INVERT                               6604
#define IDC_RADIO_BW                                   6605
#define IDC_RADIO_NOBG                                 6606
#define IDC_MARGESETTINGS_STATIC                       6607
#define IDC_EDIT_ML                                    6608
#define IDC_EDIT_MT                                    6609
#define IDC_EDIT_MR                                    6610
#define IDC_EDIT_MB                                    6611
#define IDC_ML_STATIC                                  6612
#define IDC_MT_STATIC                                  6613
#define IDC_MR_STATIC                                  6614
#define IDC_MB_STATIC                                  6615

#define IDC_CURRENTLINEMARK_STATIC                     6651
#define IDC_RADIO_CLM_NONE                             6652
#define IDC_RADIO_CLM_HILITE                           6653
#define IDC_RADIO_CLM_FRAME                            6654
#define IDC_CARETLINEFRAME_WIDTH_STATIC                6655
#define IDC_CARETLINEFRAME_WIDTH_SLIDER                6656
#define IDC_CARETLINEFRAME_WIDTH_DISPLAY               6657


#define IDC_EDIT_HLEFT                                 6701
#define IDC_EDIT_HMIDDLE                               6702
#define IDC_EDIT_HRIGHT                                6703
#define IDC_COMBO_HFONTNAME                            6704
#define IDC_COMBO_HFONTSIZE                            6705
#define IDC_CHECK_HBOLD                                6706
#define IDC_CHECK_HITALIC                              6707
#define IDC_HGB_STATIC                                 6708
#define IDC_HL_STATIC                                  6709
#define IDC_HM_STATIC                                  6710
#define IDC_HR_STATIC                                  6711
#define IDC_EDIT_FLEFT                                 6712
#define IDC_EDIT_FMIDDLE                               6713
#define IDC_EDIT_FRIGHT                                6714
#define IDC_COMBO_FFONTNAME                            6715
#define IDC_COMBO_FFONTSIZE                            6716
#define IDC_CHECK_FBOLD                                6717
#define IDC_CHECK_FITALIC                              6718
#define IDC_FGB_STATIC                                 6719
#define IDC_FL_STATIC                                  6720
#define IDC_FM_STATIC                                  6721
#define IDC_FR_STATIC                                  6722
#define IDC_BUTTON_ADDVAR                              6723
#define IDC_COMBO_VARLIST                              6724
#define IDC_VAR_STATIC                                 6725
#define IDC_EDIT_VIEWPANEL                             6726
#define IDC_WHICHPART_STATIC                           6727
#define IDC_HEADERFPPTER_GR_STATIC                     6728

#define IDD_PREFERENCE_SUB_BACKUP                      6800
#define IDC_BACKUPDIR_GRP_STATIC                       6801
#define IDC_BACKUPDIR_USERCUSTOMDIR_GRPSTATIC          6802
#define IDD_BACKUPDIR_STATIC                           6803
#define IDC_BACKUPDIR_CHECK                            6804
#define IDC_BACKUPDIR_EDIT                             6805
#define IDD_BACKUPDIR_BROWSE_BUTTON                    6806
#define IDD_AUTOC_GRPSTATIC                            6807
#define IDD_AUTOC_ENABLECHECK                          6808
#define IDD_AUTOC_FUNCRADIO                            6809
#define IDD_AUTOC_WORDRADIO                            6810
#define IDD_AUTOC_STATIC_FROM                          6811
#define IDD_AUTOC_STATIC_N                             6812
#define IDD_AUTOC_STATIC_CHAR                          6813

#define IDD_FUNC_CHECK                                 6815
#define IDD_AUTOC_BOTHRADIO                            6816
#define IDC_BACKUPDIR_RESTORESESSION_GRP_STATIC        6817
#define IDC_BACKUPDIR_RESTORESESSION_CHECK             6818
#define IDD_BACKUPDIR_RESTORESESSION_STATIC1           6819
#define IDC_BACKUPDIR_RESTORESESSION_EDIT              6820
#define IDD_BACKUPDIR_RESTORESESSION_STATIC2           6821
#define IDD_BACKUPDIR_RESTORESESSION_PATHLABEL_STATIC  6822
#define IDD_BACKUPDIR_RESTORESESSION_PATH_EDIT         6823
#define IDD_AUTOC_IGNORENUMBERS                        6824
#define IDC_CHECK_KEEPABSENTFILESINSESSION             6825

#define IDD_PREFERENCE_SUB_AUTOCOMPLETION              6850
#define IDD_AUTOCINSERT_GRPSTATIC                      6851
#define IDD_AUTOCPARENTHESES_CHECK                     6852
#define IDD_AUTOCBRACKET_CHECK                         6853
#define IDD_AUTOCCURLYBRACKET_CHECK                    6854
#define IDD_AUTOC_DOUBLEQUOTESCHECK                    6855
#define IDD_AUTOC_QUOTESCHECK                          6856
#define IDD_AUTOCTAG_CHECK                             6857
#define IDC_MACHEDPAIROPEN_STATIC                      6858
#define IDC_MACHEDPAIRCLOSE_STATIC                     6859
#define IDC_MACHEDPAIR_STATIC1                         6860
#define IDC_MACHEDPAIROPEN_EDIT1                       6861
#define IDC_MACHEDPAIRCLOSE_EDIT1                      6862
#define IDC_MACHEDPAIR_STATIC2                         6863
#define IDC_MACHEDPAIROPEN_EDIT2                       6864
#define IDC_MACHEDPAIRCLOSE_EDIT2                      6865
#define IDC_MACHEDPAIR_STATIC3                         6866
#define IDC_MACHEDPAIROPEN_EDIT3                       6867
#define IDC_MACHEDPAIRCLOSE_EDIT3                      6868
#define IDD_AUTOC_USEKEY_GRP_STATIC                    6869
#define IDD_AUTOC_USETAB                               6870
#define IDD_AUTOC_USEENTER                             6871
#define IDD_AUTOC_BRIEF_CHECK                          6872
#define IDC_AUTOC_CHAR_SLIDER                          6873
#define IDD_AUTOC_SLIDER_MIN_STATIC                    6874
#define IDD_AUTOC_SLIDER_MAX_STATIC                    6875

#define IDD_PREFERENCE_SUB_SEARCHING                   6900

#define IDC_CHECK_MONOSPACEDFONT_FINDDLG               6902
#define IDC_CHECK_FINDDLG_ALWAYS_VISIBLE               6903
#define IDC_CHECK_CONFIRMREPLOPENDOCS                  6904
#define IDC_CHECK_REPLACEANDSTOP                       6905
#define IDC_CHECK_SHOWONCEPERFOUNDLINE                 6906
#define IDD_FILL_FIND_FIELD_GRP_STATIC                 6907
#define IDC_CHECK_FILL_FIND_FIELD_WITH_SELECTED        6908
#define IDC_CHECK_FILL_FIND_FIELD_SELECT_CARET         6909
#define IDC_INSELECTION_THRESHOLD_STATIC               6910
#define IDC_INSELECTION_THRESHOLD_EDIT                 6911
#define IDC_INSELECTION_THRESH_QUESTION_BUTTON         6912
#define IDC_CHECK_FILL_DIR_FIELD_FROM_ACTIVE_DOC       6913

#define IDD_PREFERENCE_SUB_DARKMODE                    7100

#define IDC_RADIO_DARKMODE_BLACK                       7102
#define IDC_RADIO_DARKMODE_RED                         7103
#define IDC_RADIO_DARKMODE_GREEN                       7104
#define IDC_RADIO_DARKMODE_BLUE                        7105

#define IDC_RADIO_DARKMODE_PURPLE                      7107
#define IDC_RADIO_DARKMODE_CYAN                        7108
#define IDC_RADIO_DARKMODE_OLIVE                       7109


#define IDC_RADIO_DARKMODE_CUSTOMIZED                  7115
#define IDD_CUSTOMIZED_COLOR1_STATIC                   7116
#define IDD_CUSTOMIZED_COLOR2_STATIC                   7117
#define IDD_CUSTOMIZED_COLOR3_STATIC                   7118
#define IDD_CUSTOMIZED_COLOR4_STATIC                   7119
#define IDD_CUSTOMIZED_COLOR5_STATIC                   7120
#define IDD_CUSTOMIZED_COLOR6_STATIC                   7121
#define IDD_CUSTOMIZED_COLOR7_STATIC                   7122
#define IDD_CUSTOMIZED_COLOR8_STATIC                   7123
#define IDD_CUSTOMIZED_COLOR9_STATIC                   7124
#define IDD_CUSTOMIZED_COLOR10_STATIC                  7125
#define IDD_CUSTOMIZED_COLOR11_STATIC                  7126
#define IDD_CUSTOMIZED_COLOR12_STATIC                  7127

#define IDD_CUSTOMIZED_RESET_BUTTON                    7130
#define IDC_RADIO_DARKMODE_LIGHTMODE                   7131
#define IDC_RADIO_DARKMODE_DARKMODE                    7132
#define IDC_RADIO_DARKMODE_FOLLOWWINDOWS               7133

#define IDC_DARKMODE_TONES_GB_STATIC                   7135
#define IDD_DROPDOWN_RESET_RED                         7136
#define IDD_DROPDOWN_RESET_GREEN                       7137
#define IDD_DROPDOWN_RESET_BLUE                        7138
#define IDD_DROPDOWN_RESET_PURPLE                      7139
#define IDD_DROPDOWN_RESET_CYAN                        7140
#define IDD_DROPDOWN_RESET_OLIVE                       7141

#define IDD_PREFERENCE_SUB_PERFORMANCE                 7140
#define IDC_GROUPSTATIC_PERFORMANCE_RESTRICTION        7141
#define IDD_PERFORMANCE_TIP_QUESTION_BUTTON            7142
#define IDC_CHECK_PERFORMANCE_ENABLE                   7143
#define IDC_STATIC_PERFORMANCE_FILESIZE                7144
#define IDC_EDIT_PERFORMANCE_FILESIZE                  7145
#define IDC_STATIC_PERFORMANCE_MB                      7146
#define IDC_CHECK_PERFORMANCE_ALLOWBRACEMATCH          7147
#define IDC_CHECK_PERFORMANCE_ALLOWAUTOCOMPLETION      7148
#define IDC_CHECK_PERFORMANCE_ALLOWSMARTHILITE         7149
#define IDC_CHECK_PERFORMANCE_DEACTIVATEWORDWRAP       7150
#define IDC_CHECK_PERFORMANCE_ALLOWCLICKABLELINK       7151
#define IDC_CHECK_PERFORMANCE_SUPPRESS2GBWARNING       7152

#define IDD_PREFERENCE_SUB_INDENTATION                 7160
#define IDC_GROUPSTATIC_AUTOINDENT                     7161
#define IDC_RADIO_AUTOINDENT_NONE                      7162
#define IDC_RADIO_AUTOINDENT_BASIC                     7163
#define IDC_RADIO_AUTOINDENT_ADVANCED                  7164
