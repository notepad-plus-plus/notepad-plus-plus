/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2022-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#define PARAM_TO_STR(X)			#X
#define TO_STR(X)				PARAM_TO_STR(X)


#define VER_COPYRIGHT			"Copyright (C) 2025\0"

#define PLUGIN_VERSION			2.2.0
#define VER_FILEVERSION			2,2,0,0
#define IS_PRERELEASE			0

#if (IS_PRERELEASE == 1)
#define VER_PRERELEASE	VS_FF_PRERELEASE
#else
#define VER_PRERELEASE	0
#endif

#ifdef _DEBUG
#define VER_DEBUG		VS_FF_DEBUG
#else
#define VER_DEBUG		0
#endif

#define VER_FILEFLAGS	(VER_PRERELEASE | VER_DEBUG)

#ifdef WIN64
#define VER_PRODUCT_STR		"ComparePlus (64-bit)\0"
#else
#define VER_PRODUCT_STR		"ComparePlus (32-bit)\0"
#endif


#define IDDEFAULT						3

#define IDD_ABOUT_DIALOG				101
#define IDD_SETTINGS_DIALOG				102
#define IDD_COMPARE_OPTIONS_DIALOG		103
#define IDD_VISUAL_FILTERS_DIALOG		104
#define IDD_NAV_DIALOG					105

#define IDB_DOCKING_ICON				115

#define IDB_SETFIRST					120
#define IDB_SETFIRST_RTL				121
#define IDB_COMPARE						122
#define IDB_COMPARE_LINES				123
#define IDB_CLEARCOMPARE				124
#define IDB_FIRST						125
#define IDB_LAST						126
#define IDB_PREV						127
#define IDB_NEXT						128
#define IDB_DIFFS_FILTERS				129
#define IDB_NAVBAR						130

#define IDB_SETFIRST_FL					140
#define IDB_SETFIRST_RTL_FL				141
#define IDB_COMPARE_FL					142
#define IDB_COMPARE_LINES_FL			143
#define IDB_CLEARCOMPARE_FL				144
#define IDB_FIRST_FL					145
#define IDB_LAST_FL						146
#define IDB_PREV_FL						147
#define IDB_NEXT_FL						148
#define IDB_DIFFS_FILTERS_FL			149
#define IDB_NAVBAR_FL					150

#define IDB_SETFIRST_FL_DM				160
#define IDB_SETFIRST_RTL_FL_DM			161
#define IDB_COMPARE_FL_DM				162
#define IDB_COMPARE_LINES_FL_DM			163
#define IDB_CLEARCOMPARE_FL_DM			164
#define IDB_FIRST_FL_DM					165
#define IDB_LAST_FL_DM					166
#define IDB_PREV_FL_DM					167
#define IDB_NEXT_FL_DM					168
#define IDB_DIFFS_FILTERS_FL_DM			169
#define IDB_NAVBAR_FL_DM				170

#define IDC_CLOSE						1001
#define IDC_DONATE						1002
#define IDC_INFO						1003
#define IDC_VERSION						1004
#define IDC_BUILD_TIME					1005
#define IDC_AUTHOR						1006
#define IDC_LIBS						1007
#define IDC_GITLIB_VER					1008
#define IDC_SQLITE3_VER					1009
#define IDC_LINKS						1010
#define IDC_REPO_URL					1011
#define IDC_GUIDE_URL					1012

#define IDC_MAIN						1020
#define IDC_FIRST						1021
#define IDC_FIRST_NEW					1022
#define IDC_FIRST_OLD					1023
#define IDC_FILES_POS					1024
#define IDC_NEW_IN_SUB					1025
#define IDC_OLD_IN_SUB					1026
#define IDC_DEFAULT_COMPARE				1027
#define IDC_COMPARE_TO_PREV				1028
#define IDC_COMPARE_TO_NEXT				1029
#define IDC_STATUS_BAR					1030
#define IDC_DIFFS_SUMMARY				1031
#define IDC_COMPARE_OPTIONS				1032
#define IDC_STATUS_DISABLED				1033
#define IDC_MISC						1034
#define IDC_ENCODINGS_CHECK				1035
#define IDC_SIZES_CHECK					1036
#define IDC_MANUAL_SYNC_CHECK			1037
#define IDC_CLOSE_ON_MATCH				1038
#define IDC_HIDE_MARGIN					1039
#define IDC_NEVER_MARK_IGNORED			1040
#define IDC_FOLLOWING_CARET				1041
#define IDC_WRAP_AROUND					1042
#define IDC_GOTO_FIRST_DIFF				1043
#define IDC_COLORS						1044
#define IDC_COLOR_LIST					1045
#define IDC_ADDED						1046
#define IDC_COMBO_ADDED_COLOR			1047
#define IDC_REMOVED						1048
#define IDC_COMBO_REMOVED_COLOR			1049
#define IDC_MOVED						1050
#define IDC_COMBO_MOVED_COLOR			1051
#define IDC_CHANGED						1052
#define IDC_COMBO_CHANGED_COLOR			1053
#define IDC_ADDED_PART					1054
#define IDC_COMBO_ADDED_PART_COLOR		1055
#define IDC_REMOVED_PART				1056
#define IDC_COMBO_REMOVED_PART_COLOR	1057
#define IDC_MOVED_PART					1058
#define IDC_COMBO_MOVED_PART_COLOR		1059
#define IDC_PART_TRANSP					1060
#define IDC_PART_TRANSP_EDIT			1061
#define IDC_PART_TRANSP_SPIN			1062
#define IDC_CARET_TRANSP				1063
#define IDC_CARET_TRANSP_EDIT			1064
#define IDC_CARET_TRANSP_SPIN			1065
#define IDC_CHANGE_RESEMBL				1066
#define IDC_CHANGE_RES_EDIT				1067
#define IDC_CHANGE_RES_SPIN				1068
#define IDC_TOOLBAR						1069
#define IDC_ENABLE_TOOLBAR				1070
#define IDC_SET_AS_FIRST_TB				1071
#define IDC_COMPARE_TB					1072
#define IDC_COMPARE_SELECTIONS_TB		1073
#define IDC_CLEAR_COMPARE_TB			1074
#define IDC_NAVIGATION_TB				1075
#define IDC_DIFFS_FILTERS_TB			1076
#define IDC_NAV_BAR_TB					1077

#define IDC_DETECT						1090
#define IDC_DETECT_MOVES				1091
#define IDC_DETECT_SUB_BLOCK_DIFFS		1092
#define IDC_DETECT_SUB_LINE_MOVES		1093
#define IDC_DETECT_CHAR_DIFFS			1094
#define IDC_IGNORE						1095
#define IDC_IGNORE_EMPTY_LINES			1096
#define IDC_IGNORE_FOLDED_LINES			1097
#define IDC_IGNORE_HIDDEN_LINES			1098
#define IDC_IGNORE_CHANGED_SPACES		1099
#define IDC_IGNORE_ALL_SPACES			1100
#define IDC_IGNORE_EOL					1101
#define IDC_IGNORE_CASE					1102
#define IDC_REGEX						1103
#define IDC_IGNORE_REGEX				1104
#define IDC_IGNORE_REGEX_STR			1105
#define IDC_REGEX_MODE_IGNORE			1106
#define IDC_REGEX_MODE_MATCH			1107
#define IDC_REGEX_INCL_NOMATCH_LINES	1108
#define IDC_HIGHLIGHT_REGEX_IGNORES		1109

#define IDC_NOTE						1120
#define IDC_FILTERS						1121
#define IDC_HIDE_MATCHES				1122
#define IDC_HIDE_ALL_DIFFS				1123
#define IDC_HIDE_NEW_LINES				1124
#define IDC_HIDE_CHANGED_LINES			1125
#define IDC_HIDE_MOVED_LINES			1126
#define IDC_SHOW_ONLY_SELECTIONS		1127

#define IDC_STATIC						-1

// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        220
#define _APS_NEXT_COMMAND_VALUE         20001
#define _APS_NEXT_CONTROL_VALUE         1200
#define _APS_NEXT_SYMED_VALUE           120
#endif
#endif
