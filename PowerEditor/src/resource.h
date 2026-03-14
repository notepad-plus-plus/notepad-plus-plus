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
    17|#pragma once
    18|
    19|
    20|//************ npminmin version **************************
    21|
    22|#define NOTEPAD_PLUS_VERSION L"npminmin v8.9.2"
    23|
    24|// should be X.Y : ie. if VERSION_DIGITALVALUE == 4, 7, 1, 0 , then X = 4, Y = 71
    25|// ex : #define VERSION_VALUE L"5.63\0"
    26|#define VERSION_INTERNAL_VALUE L"8.92\0"
    27|
    28|#define VERSION_PRODUCT_VALUE L"8.9.2\0"
    29|#define VERSION_DIGITALVALUE 8, 9, 2, 0
    30|
    31|//**********************************************************
    32|
    33|#define INFO_URL L"https://github.com/ridermw/np-minus-minus/update/getDownloadUrl.php"
    34|#define FORCED_DOWNLOAD_DOMAIN L"https://github.com/notepad-plus-plus/notepad-plus-plus/"
    35|
    36|
    37|#ifndef IDC_STATIC
    38|#define IDC_STATIC    -1
    39|#endif
    40|
    41|#define IDI_NPPABOUT_LOGO           99
    42|#define IDI_M30ICON                100
    43|#define IDI_CHAMELEON              101
    44|#define IDI_CHAMELEON_DM           102
    45|//#define IDI_JESUISCHARLIE        102
    46|//#define IDI_GILETJAUNE           102
    47|//#define IDI_SAMESEXMARRIAGE      102
    48|//#define IDI_TAIWANSSOVEREIGNTY     105
    49|//#define IDI_TAIWANSSOVEREIGNTY_DM  106
    50|//#define IDI_WITHUKRAINE            105
    51|#define IDR_RT_MANIFEST            103
    52|#define IDI_ICONABSENT             104
    53|
    54|//
    55|// TOOLBAR ICO - set 1
    56|//
    57|#define IDI_NEW_ICON                      201
    58|#define IDI_OPEN_ICON                     202
    59|#define IDI_CLOSE_ICON                    203
    60|#define IDI_CLOSEALL_ICON                 204
    61|#define IDI_SAVE_ICON                     205
    62|#define IDI_SAVEALL_ICON                  206
    63|#define IDI_CUT_ICON                      207
    64|#define IDI_COPY_ICON                     208
    65|#define IDI_PASTE_ICON                    209
    66|#define IDI_UNDO_ICON                     210
    67|#define IDI_REDO_ICON                     211
    68|#define IDI_FIND_ICON                     212
    69|#define IDI_REPLACE_ICON                  213
    70|#define IDI_ZOOMIN_ICON                   214
    71|#define IDI_ZOOMOUT_ICON                  215
    72|#define IDI_VIEW_UD_DLG_ICON              216
    73|#define IDI_PRINT_ICON                    217
    74|#define IDI_STARTRECORD_ICON              218
    75|#define IDI_STARTRECORD_DISABLE_ICON      219
    76|#define IDI_STOPRECORD_ICON               220
    77|#define IDI_STOPRECORD_DISABLE_ICON       221
    78|#define IDI_PLAYRECORD_ICON               222
    79|#define IDI_PLAYRECORD_DISABLE_ICON       223
    80|#define IDI_SAVERECORD_ICON               224
    81|#define IDI_SAVERECORD_DISABLE_ICON       225
    82|#define IDI_MMPLAY_DIS_ICON               226
    83|#define IDI_MMPLAY_ICON                   227
    84|#define IDI_VIEW_ALL_CHAR_ICON            228
    85|#define IDI_VIEW_INDENT_ICON              229
    86|#define IDI_VIEW_WRAP_ICON                230
    87|#define IDI_SAVE_DISABLE_ICON             231
    88|#define IDI_SAVEALL_DISABLE_ICON          232
    89|#define IDI_CUT_DISABLE_ICON              233
    90|#define IDI_COPY_DISABLE_ICON             234
    91|#define IDI_PASTE_DISABLE_ICON            235
    92|#define IDI_UNDO_DISABLE_ICON             236
    93|#define IDI_REDO_DISABLE_ICON             237
    94|#define IDI_SYNCV_ICON                    238
    95|#define IDI_SYNCV_DISABLE_ICON            239
    96|#define IDI_SYNCH_ICON                    240
    97|#define IDI_SYNCH_DISABLE_ICON            241
    98|#define IDI_VIEW_DOC_MAP_ICON             242
    99|#define IDI_VIEW_FILEBROWSER_ICON         243
   100|#define IDI_VIEW_FUNCLIST_ICON            244
   101|#define IDI_VIEW_MONITORING_ICON          245
   102|#define IDI_VIEW_MONITORING_DIS_ICON      246
   103|#define IDI_VIEW_DOCLIST_ICON             247
   104|
   105|//
   106|// TOOLBAR ICO - set 1, Dark Mode
   107|//
   108|#define IDI_NEW_ICON_DM                   251
   109|#define IDI_OPEN_ICON_DM                  252
   110|#define IDI_CLOSE_ICON_DM                 253
   111|#define IDI_CLOSEALL_ICON_DM              254
   112|#define IDI_SAVE_ICON_DM                  255
   113|#define IDI_SAVEALL_ICON_DM               256
   114|#define IDI_CUT_ICON_DM                   257
   115|#define IDI_COPY_ICON_DM                  258
   116|#define IDI_PASTE_ICON_DM                 259
   117|#define IDI_UNDO_ICON_DM                  260
   118|#define IDI_REDO_ICON_DM                  261
   119|#define IDI_FIND_ICON_DM                  262
   120|#define IDI_REPLACE_ICON_DM               263
   121|#define IDI_ZOOMIN_ICON_DM                264
   122|#define IDI_ZOOMOUT_ICON_DM               265
   123|#define IDI_VIEW_UD_DLG_ICON_DM           266
   124|#define IDI_PRINT_ICON_DM                 267
   125|#define IDI_STARTRECORD_ICON_DM           268
   126|#define IDI_STARTRECORD_DISABLE_ICON_DM   269
   127|#define IDI_STOPRECORD_ICON_DM            270
   128|#define IDI_STOPRECORD_DISABLE_ICON_DM    271
   129|#define IDI_PLAYRECORD_ICON_DM            272
   130|#define IDI_PLAYRECORD_DISABLE_ICON_DM    273
   131|#define IDI_SAVERECORD_ICON_DM            274
   132|#define IDI_SAVERECORD_DISABLE_ICON_DM    275
   133|#define IDI_MMPLAY_DIS_ICON_DM            276
   134|#define IDI_MMPLAY_ICON_DM                277
   135|#define IDI_VIEW_ALL_CHAR_ICON_DM         278
   136|#define IDI_VIEW_INDENT_ICON_DM           279
   137|#define IDI_VIEW_WRAP_ICON_DM             280
   138|#define IDI_SAVE_DISABLE_ICON_DM          281
   139|#define IDI_SAVEALL_DISABLE_ICON_DM       282
   140|#define IDI_CUT_DISABLE_ICON_DM           283
   141|#define IDI_COPY_DISABLE_ICON_DM          284
   142|#define IDI_PASTE_DISABLE_ICON_DM         285
   143|#define IDI_UNDO_DISABLE_ICON_DM          286
   144|#define IDI_REDO_DISABLE_ICON_DM          287
   145|#define IDI_SYNCV_ICON_DM                 288
   146|#define IDI_SYNCV_DISABLE_ICON_DM         289
   147|#define IDI_SYNCH_ICON_DM                 290
   148|#define IDI_SYNCH_DISABLE_ICON_DM         291
   149|#define IDI_VIEW_DOC_MAP_ICON_DM          292
   150|#define IDI_VIEW_FILEBROWSER_ICON_DM      293
   151|#define IDI_VIEW_FUNCLIST_ICON_DM         294
   152|#define IDI_VIEW_MONITORING_ICON_DM       295
   153|#define IDI_VIEW_MONITORING_DIS_ICON_DM   296
   154|#define IDI_VIEW_DOCLIST_ICON_DM          297
   155|
   156|//
   157|// TOOLBAR ICO - set 2
   158|//
   159|#define IDI_NEW_ICON2                     301
   160|#define IDI_OPEN_ICON2                    302
   161|#define IDI_CLOSE_ICON2                   303
   162|#define IDI_CLOSEALL_ICON2                304
   163|#define IDI_SAVE_ICON2                    305
   164|#define IDI_SAVEALL_ICON2                 306
   165|#define IDI_CUT_ICON2                     307
   166|#define IDI_COPY_ICON2                    308
   167|#define IDI_PASTE_ICON2                   309
   168|#define IDI_UNDO_ICON2                    310
   169|#define IDI_REDO_ICON2                    311
   170|#define IDI_FIND_ICON2                    312
   171|#define IDI_REPLACE_ICON2                 313
   172|#define IDI_ZOOMIN_ICON2                  314
   173|#define IDI_ZOOMOUT_ICON2                 315
   174|#define IDI_VIEW_UD_DLG_ICON2             316
   175|#define IDI_PRINT_ICON2                   317
   176|#define IDI_STARTRECORD_ICON2             318
   177|#define IDI_STARTRECORD_DISABLE_ICON2     319
   178|#define IDI_STOPRECORD_ICON2              320
   179|#define IDI_STOPRECORD_DISABLE_ICON2      321
   180|#define IDI_PLAYRECORD_ICON2              322
   181|#define IDI_PLAYRECORD_DISABLE_ICON2      323
   182|#define IDI_SAVERECORD_ICON2              324
   183|#define IDI_SAVERECORD_DISABLE_ICON2      325
   184|#define IDI_MMPLAY_DIS_ICON2              326
   185|#define IDI_MMPLAY_ICON2                  327
   186|#define IDI_VIEW_ALL_CHAR_ICON2           328
   187|#define IDI_VIEW_INDENT_ICON2             329
   188|#define IDI_VIEW_WRAP_ICON2               330
   189|#define IDI_SAVE_DISABLE_ICON2            331
   190|#define IDI_SAVEALL_DISABLE_ICON2         332
   191|#define IDI_CUT_DISABLE_ICON2             333
   192|#define IDI_COPY_DISABLE_ICON2            334
   193|#define IDI_PASTE_DISABLE_ICON2           335
   194|#define IDI_UNDO_DISABLE_ICON2            336
   195|#define IDI_REDO_DISABLE_ICON2            337
   196|#define IDI_SYNCV_ICON2                   338
   197|#define IDI_SYNCV_DISABLE_ICON2           339
   198|#define IDI_SYNCH_ICON2                   340
   199|#define IDI_SYNCH_DISABLE_ICON2           341
   200|#define IDI_VIEW_DOC_MAP_ICON2            342
   201|#define IDI_VIEW_FILEBROWSER_ICON2        343
   202|#define IDI_VIEW_FUNCLIST_ICON2           344
   203|#define IDI_VIEW_MONITORING_ICON2         345
   204|#define IDI_VIEW_MONITORING_DIS_ICON2     346
   205|#define IDI_VIEW_DOCLIST_ICON2            347
   206|
   207|//
   208|// TOOLBAR ICO - set 2, Dark Mode
   209|//
   210|#define IDI_NEW_ICON_DM2                  351
   211|#define IDI_OPEN_ICON_DM2                 352
   212|#define IDI_CLOSE_ICON_DM2                353
   213|#define IDI_CLOSEALL_ICON_DM2             354
   214|#define IDI_SAVE_ICON_DM2                 355
   215|#define IDI_SAVEALL_ICON_DM2              356
   216|#define IDI_CUT_ICON_DM2                  357
   217|#define IDI_COPY_ICON_DM2                 358
   218|#define IDI_PASTE_ICON_DM2                359
   219|#define IDI_UNDO_ICON_DM2                 360
   220|#define IDI_REDO_ICON_DM2                 361
   221|#define IDI_FIND_ICON_DM2                 362
   222|#define IDI_REPLACE_ICON_DM2              363
   223|#define IDI_ZOOMIN_ICON_DM2               364
   224|#define IDI_ZOOMOUT_ICON_DM2              365
   225|#define IDI_VIEW_UD_DLG_ICON_DM2          366
   226|#define IDI_PRINT_ICON_DM2                367
   227|#define IDI_STARTRECORD_ICON_DM2          368
   228|#define IDI_STARTRECORD_DISABLE_ICON_DM2  369
   229|#define IDI_STOPRECORD_ICON_DM2           370
   230|#define IDI_STOPRECORD_DISABLE_ICON_DM2   371
   231|#define IDI_PLAYRECORD_ICON_DM2           372
   232|#define IDI_PLAYRECORD_DISABLE_ICON_DM2   373
   233|#define IDI_SAVERECORD_ICON_DM2           374
   234|#define IDI_SAVERECORD_DISABLE_ICON_DM2   375
   235|#define IDI_MMPLAY_DIS_ICON_DM2           376
   236|#define IDI_MMPLAY_ICON_DM2               377
   237|#define IDI_VIEW_ALL_CHAR_ICON_DM2        378
   238|#define IDI_VIEW_INDENT_ICON_DM2          379
   239|#define IDI_VIEW_WRAP_ICON_DM2            380
   240|#define IDI_SAVE_DISABLE_ICON_DM2         381
   241|#define IDI_SAVEALL_DISABLE_ICON_DM2      382
   242|#define IDI_CUT_DISABLE_ICON_DM2          383
   243|#define IDI_COPY_DISABLE_ICON_DM2         384
   244|#define IDI_PASTE_DISABLE_ICON_DM2        385
   245|#define IDI_UNDO_DISABLE_ICON_DM2         386
   246|#define IDI_REDO_DISABLE_ICON_DM2         387
   247|#define IDI_SYNCV_ICON_DM2                388
   248|#define IDI_SYNCV_DISABLE_ICON_DM2        389
   249|#define IDI_SYNCH_ICON_DM2                390
   250|#define IDI_SYNCH_DISABLE_ICON_DM2        391
   251|#define IDI_VIEW_DOC_MAP_ICON_DM2         392
   252|#define IDI_VIEW_FILEBROWSER_ICON_DM2     393
   253|#define IDI_VIEW_FUNCLIST_ICON_DM2        394
   254|#define IDI_VIEW_MONITORING_ICON_DM2      395
   255|#define IDI_VIEW_MONITORING_DIS_ICON_DM2  396
   256|#define IDI_VIEW_DOCLIST_ICON_DM2         397
   257|
   258|
   259|
   260|#define IDI_SAVED_ICON           501
   261|#define IDI_UNSAVED_ICON         502
   262|#define IDI_READONLY_ICON        503
   263|#define IDI_FIND_RESULT_ICON     504
   264|#define IDI_MONITORING_ICON      505
   265|#define IDI_SAVED_ALT_ICON       506
   266|#define IDI_UNSAVED_ALT_ICON     507
   267|#define IDI_READONLY_ALT_ICON    508
   268|#define IDI_SAVED_DM_ICON        509
   269|#define IDI_UNSAVED_DM_ICON      510
   270|#define IDI_MONITORING_DM_ICON   511
   271|#define IDI_READONLY_DM_ICON     512
   272|#define IDI_READONLYSYS_ICON     513
   273|#define IDI_READONLYSYS_DM_ICON  514
   274|#define IDI_READONLYSYS_ALT_ICON 515
   275|
   276|#define IDI_PROJECT_WORKSPACE          601
   277|#define IDI_PROJECT_WORKSPACEDIRTY     602
   278|#define IDI_PROJECT_PROJECT            603
   279|#define IDI_PROJECT_FOLDEROPEN         604
   280|#define IDI_PROJECT_FOLDERCLOSE        605
   281|#define IDI_PROJECT_FILE               606
   282|#define IDI_PROJECT_FILEINVALID        607
   283|#define IDI_FB_ROOTOPEN                608
   284|#define IDI_FB_ROOTCLOSE               609
   285|
   286|#define IDI_PROJECT_WORKSPACE_DM       611
   287|#define IDI_PROJECT_WORKSPACEDIRTY_DM  612
   288|#define IDI_PROJECT_PROJECT_DM         613
   289|#define IDI_PROJECT_FOLDEROPEN_DM      614
   290|#define IDI_PROJECT_FOLDERCLOSE_DM     615
   291|#define IDI_PROJECT_FILE_DM            616
   292|#define IDI_PROJECT_FILEINVALID_DM     617
   293|#define IDI_FB_ROOTOPEN_DM             618
   294|#define IDI_FB_ROOTCLOSE_DM            619
   295|
   296|#define IDI_PROJECT_WORKSPACE2         621
   297|#define IDI_PROJECT_WORKSPACEDIRTY2    622
   298|#define IDI_PROJECT_PROJECT2           623
   299|#define IDI_PROJECT_FOLDEROPEN2        624
   300|#define IDI_PROJECT_FOLDERCLOSE2       625
   301|#define IDI_PROJECT_FILE2              626
   302|#define IDI_PROJECT_FILEINVALID2       627
   303|#define IDI_FB_ROOTOPEN2               628
   304|#define IDI_FB_ROOTCLOSE2              629
   305|
   306|#define IDI_FB_SELECTCURRENTFILE       630
   307|#define IDI_FB_FOLDALL                 631
   308|#define IDI_FB_EXPANDALL               632
   309|#define IDI_FB_SELECTCURRENTFILE_DM    633
   310|#define IDI_FB_FOLDALL_DM              634
   311|#define IDI_FB_EXPANDALL_DM            635
   312|
   313|#define IDI_FUNCLIST_ROOT              IDI_PROJECT_FILE // using same file
   314|#define IDI_FUNCLIST_NODE              641
   315|#define IDI_FUNCLIST_LEAF              642
   316|
   317|#define IDI_FUNCLIST_ROOT_DM           IDI_PROJECT_FILE_DM // using same file
   318|#define IDI_FUNCLIST_NODE_DM           644
   319|#define IDI_FUNCLIST_LEAF_DM           645
   320|
   321|#define IDI_FUNCLIST_ROOT2             IDI_PROJECT_FILE2 // using same file
   322|#define IDI_FUNCLIST_NODE2             647
   323|#define IDI_FUNCLIST_LEAF2             648
   324|
   325|#define IDI_FUNCLIST_SORTBUTTON              651
   326|#define IDI_FUNCLIST_RELOADBUTTON            652
   327|#define IDI_FUNCLIST_PREFERENCEBUTTON        653
   328|#define IDI_FUNCLIST_SORTBUTTON_DM           654
   329|#define IDI_FUNCLIST_RELOADBUTTON_DM         655
   330|#define IDI_FUNCLIST_PREFERENCEBUTTON_DM     656
   331|
   332|
   333|
   334|
   335|#define IDI_GET_INFO_FROM_TOOLTIP               661
   336|
   337|
   338|
   339|#define IDC_MY_CUR     1402
   340|#define IDC_UP_ARROW  1403
   341|#define IDC_DRAG_TAB    1404
   342|#define IDC_DRAG_INTERDIT_TAB 1405
   343|#define IDC_DRAG_PLUS_TAB 1406
   344|#define IDC_DRAG_OUT_TAB 1407
   345|
   346|#define IDC_MACRO_RECORDING 1408
   347|
   348|#define IDR_SAVEALL                 1500
   349|#define IDR_CLOSEFILE               1501
   350|#define IDR_CLOSEALL                1502
   351|#define IDR_FIND                    1503
   352|#define IDR_REPLACE                 1504
   353|#define IDR_ZOOMIN                  1505
   354|#define IDR_ZOOMOUT                 1506
   355|#define IDR_WRAP                    1507
   356|#define IDR_INVISIBLECHAR           1508
   357|#define IDR_INDENTGUIDE             1509
   358|#define IDR_SHOWPANNEL              1510
   359|#define IDR_STARTRECORD             1511
   360|#define IDR_STOPRECORD              1512
   361|#define IDR_PLAYRECORD              1513
   362|#define IDR_SAVERECORD              1514
   363|#define IDR_SYNCV                   1515
   364|#define IDR_SYNCH                   1516
   365|#define IDR_FILENEW                 1517
   366|#define IDR_FILEOPEN                1518
   367|#define IDR_FILESAVE                1519
   368|#define IDR_PRINT                   1520
   369|#define IDR_CUT                     1521
   370|#define IDR_COPY                    1522
   371|#define IDR_PASTE                   1523
   372|#define IDR_UNDO                    1524
   373|#define IDR_REDO                    1525
   374|#define IDR_M_PLAYRECORD            1526
   375|#define IDR_DOCMAP                  1527
   376|#define IDR_FUNC_LIST               1528
   377|#define IDR_FILEBROWSER             1529
   378|#define IDR_CLOSETAB                1530
   379|#define IDR_CLOSETAB_INACT          1531
   380|#define IDR_CLOSETAB_INACT_EMPTY    1532
   381|#define IDR_CLOSETAB_HOVERIN        1533
   382|#define IDR_CLOSETAB_HOVERONTAB     1534
   383|#define IDR_CLOSETAB_PUSH           1535
   384|#define IDR_FUNC_LIST_ICO           1536
   385|#define IDR_DOCMAP_ICO              1537
   386|#define IDR_PROJECTPANEL_ICO        1538
   387|#define IDR_CLIPBOARDPANEL_ICO      1539
   388|#define IDR_ASCIIPANEL_ICO          1540
   389|#define IDR_DOCSWITCHER_ICO         1541
   390|#define IDR_FILEBROWSER_ICO         1542
   391|#define IDR_FILEMONITORING          1543
   392|#define IDR_CLOSETAB_DM             1544
   393|#define IDR_CLOSETAB_INACT_DM       1545
   394|#define IDR_CLOSETAB_INACT_EMPTY_DM 1546
   395|#define IDR_CLOSETAB_HOVERIN_DM     1547
   396|#define IDR_CLOSETAB_HOVERONTAB_DM  1548
   397|#define IDR_CLOSETAB_PUSH_DM        1549
   398|#define IDR_DOCLIST                 1550
   399|#define IDR_DOCLIST_ICO             1551
   400|
   401|#define IDR_FILEBROWSER_ICO2        1552
   402|#define IDR_FILEBROWSER_ICO_DM      1553
   403|#define IDR_FUNC_LIST_ICO2          1554
   404|#define IDR_FUNC_LIST_ICO_DM        1555
   405|#define IDR_DOCMAP_ICO2             1556
   406|#define IDR_DOCMAP_ICO_DM           1557
   407|#define IDR_DOCLIST_ICO2            1558
   408|#define IDR_DOCLIST_ICO_DM          1559
   409|#define IDR_PROJECTPANEL_ICO2       1560
   410|#define IDR_PROJECTPANEL_ICO_DM     1561
   411|#define IDR_CLIPBOARDPANEL_ICO2     1562
   412|#define IDR_CLIPBOARDPANEL_ICO_DM   1563
   413|#define IDR_ASCIIPANEL_ICO2         1564
   414|#define IDR_ASCIIPANEL_ICO_DM       1565
   415|#define IDR_FIND_RESULT_ICO2        1566
   416|#define IDR_FIND_RESULT_ICO_DM      1567
   417|
   418|#define IDR_PINTAB                  1568
   419|#define IDR_PINTAB_INACT            1569
   420|#define IDR_PINTAB_INACT_EMPTY      1570
   421|#define IDR_PINTAB_HOVERIN          1571
   422|#define IDR_PINTAB_HOVERONTAB       1572
   423|#define IDR_PINTAB_PINNED           1573
   424|#define IDR_PINTAB_PINNEDHOVERIN    1574
   425|#define IDR_PINTAB_DM               1575
   426|#define IDR_PINTAB_INACT_DM         1576
   427|#define IDR_PINTAB_INACT_EMPTY_DM   1577
   428|#define IDR_PINTAB_HOVERIN_DM       1578
   429|#define IDR_PINTAB_HOVERONTAB_DM    1579
   430|#define IDR_PINTAB_PINNED_DM        1580
   431|#define IDR_PINTAB_PINNEDHOVERIN_DM 1581
   432|
   433|#define ID_MACRO                           20000
   434|//                                     O     .
   435|//                                     C     .
   436|//                                     C     .
   437|//                                     U     .
   438|//                                     P     .
   439|//                                     I     .
   440|//                                     E     .
   441|//                                     D     .
   442|#define ID_MACRO_LIMIT                     20499
   443|
   444|
   445|#define ID_USER_CMD                        21000
   446|//                                     O     .
   447|//                                     C     .
   448|//                                     C     .
   449|//                                     U     .
   450|//                                     P     .
   451|//                                     I     .
   452|//                                     E     .
   453|//                                     D     .
   454|#define ID_USER_CMD_LIMIT                  21499
   455|
   456|
   457|#define ID_PLUGINS_CMD                     22000
   458|//                                     O     .
   459|//                                     C     .
   460|//                                     C     .
   461|//                                     U     .
   462|//                                     P     .
   463|//                                     I     .
   464|//                                     E     .
   465|//                                     D     .
   466|#define ID_PLUGINS_CMD_LIMIT               22999
   467|
   468|
   469|#define ID_PLUGINS_CMD_DYNAMIC             23000
   470|//                                     O     .
   471|//                                     C     .
   472|//                                     C     .
   473|//                                     U     .
   474|//                                     P     .
   475|//                                     I     .
   476|//                                     E     .
   477|//                                     D     .
   478|#define ID_PLUGINS_CMD_DYNAMIC_LIMIT       24999
   479|
   480|
   481|#define MARKER_PLUGINS          1
   482|#define MARKER_PLUGINS_LIMIT   15
   483|
   484|#define INDICATOR_PLUGINS          9  // indicators 8 and below are reserved by npminmin (URL_INDIC=8)
   485|#define INDICATOR_PLUGINS_LIMIT   20  // indicators 21 and up are reserved by npminmin (SCE_UNIVERSAL_FOUND_STYLE_EXT5=21)
   486|
   487|
   488|
   489|//#define IDM 40000
   490|
   491|#define IDCMD 50000
   492|    //#define IDM_EDIT_AUTOCOMPLETE                (IDCMD+0)
   493|    //#define IDM_EDIT_AUTOCOMPLETE_CURRENTFILE    (IDCMD+1)
   494|
   495|	#define IDC_PREV_DOC                    (IDCMD+3)
   496|	#define IDC_NEXT_DOC                    (IDCMD+4)
   497|	#define IDC_EDIT_TOGGLEMACRORECORDING    (IDCMD+5)
   498|    //#define IDC_KEY_HOME                    (IDCMD+6)
   499|    //#define IDC_KEY_END                        (IDCMD+7)
   500|    //#define IDC_KEY_SELECT_2_HOME            (IDCMD+8)
   501|