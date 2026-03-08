#pragma once
// Win32 Shim: Visual Styles symbol definitions for macOS
#include "vsstyle.h"

// TMT property IDs used by GetThemeColor/GetThemeInt etc.
#define TMT_TEXTCOLOR         3803
#define TMT_FILLCOLOR         3802
#define TMT_BORDERCOLOR       3801
#define TMT_EDGEFILLCOLOR     3808
#define TMT_EDGEHIGHLIGHTCOLOR 3826
#define TMT_EDGESHADOWCOLOR   3827
#define TMT_WINDOW            1606
#define TMT_FONT              210
#define TMT_DIBDATA           2
#define TMT_GLYPHDIBDATA      8
#define TMT_ENUM              200
#define TMT_STRING            201
#define TMT_INT               202
#define TMT_BOOL              203
#define TMT_COLOR             204
#define TMT_MARGINS           205
#define TMT_FILENAME          206
#define TMT_SIZE              207
#define TMT_POSITION          208
#define TMT_RECT              209

// Scrollbar parts
#define SBP_ARROWBTN     1
#define SBP_THUMBBTNHORZ 2
#define SBP_THUMBBTNVERT 3
#define SBP_LOWERTRACKHORZ 4
#define SBP_UPPERTRACKHORZ 5
#define SBP_LOWERTRACKVERT 6
#define SBP_UPPERTRACKVERT 7
#define SBP_GRIPPERHORZ  8
#define SBP_GRIPPERVERT  9
#define SBP_SIZEBOX      10

// Arrow button states
#define ABS_UPNORMAL    1
#define ABS_UPHOT       2
#define ABS_UPPRESSED   3
#define ABS_UPDISABLED  4
#define ABS_DOWNNORMAL  5
#define ABS_DOWNHOT     6
#define ABS_DOWNPRESSED 7
#define ABS_DOWNDISABLED 8
#define ABS_LEFTNORMAL  9
#define ABS_LEFTHOT     10
#define ABS_LEFTPRESSED 11
#define ABS_LEFTDISABLED 12
#define ABS_RIGHTNORMAL 13
#define ABS_RIGHTHOT    14
#define ABS_RIGHTPRESSED 15
#define ABS_RIGHTDISABLED 16

// Scrollbar thumb states
#define SCRBS_NORMAL   1
#define SCRBS_HOT      2
#define SCRBS_PRESSED  3
#define SCRBS_DISABLED 4

// Header parts
#define HP_HEADERITEM      1
#define HP_HEADERITEMLEFT  2
#define HP_HEADERITEMRIGHT 3
#define HP_HEADERSORTARROW 4

// Header states
#define HIS_NORMAL  1
#define HIS_HOT     2
#define HIS_PRESSED 3

// Listview parts
#define LVP_LISTITEM      1
#define LVP_LISTGROUP     2
#define LVP_LISTDETAIL    3
#define LVP_LISTSORTEDDETAIL 4
#define LVP_EMPTYTEXT     5
#define LVP_GROUPHEADER   6

// Menu parts
#define MENU_MENUITEM_TMSCHEMA  1
#define MENU_MENUDROPDOWN_TMSCHEMA 2
#define MENU_MENUBARITEM_TMSCHEMA 3
#define MENU_MENUBARDROPDOWN_TMSCHEMA 4
#define MENU_CHEVRON_TMSCHEMA  5
#define MENU_SEPARATOR_TMSCHEMA 6
#define MENU_BARBACKGROUND 7
#define MENU_BARITEM       8
#define MENU_POPUPBACKGROUND 9
#define MENU_POPUPBORDERS  10
#define MENU_POPUPCHECK    11
#define MENU_POPUPCHECKBACKGROUND 12
#define MENU_POPUPGUTTER   13
#define MENU_POPUPITEM     14
#define MENU_POPUPSEPARATOR 15
#define MENU_POPUPSUBMENU  16
#define MENU_SYSTEMCLOSE   17
#define MENU_SYSTEMMAXIMIZE 18
#define MENU_SYSTEMMINIMIZE 19
#define MENU_SYSTEMRESTORE 20

// Menu bar item states
#define MBI_NORMAL     1
#define MBI_HOT        2
#define MBI_PUSHED     3
#define MBI_DISABLED   4
#define MBI_DISABLEDHOT 5
#define MBI_DISABLEDPUSHED 6

// Menu popup item states
#define MPI_NORMAL     1
#define MPI_HOT        2
#define MPI_DISABLED   3
#define MPI_DISABLEDHOT 4

// Toolbar parts
#define TP_BUTTON       1
#define TP_DROPDOWNBUTTON 2
#define TP_SPLITBUTTON  3
#define TP_SPLITBUTTONDROPDOWN 4
#define TP_SEPARATOR    5
#define TP_SEPARATORVERT 6

// Toolbar button states
#define TS_NORMAL   1
#define TS_HOT      2
#define TS_PRESSED  3
#define TS_DISABLED 4
#define TS_CHECKED  5
#define TS_HOTCHECKED 6
#define TS_NEARHOT  7

// Combobox parts
#define CP_DROPDOWNBUTTON 1
#define CP_BACKGROUND     2
#define CP_TRANSPARENTBACKGROUND 3
#define CP_BORDER         4
#define CP_READONLY       5
#define CP_DROPDOWNBUTTONRIGHT 6
#define CP_DROPDOWNBUTTONLEFT 7
#define CP_CUEBANNER      8

// Combobox states
#define CBXS_NORMAL   1
#define CBXS_HOT      2
#define CBXS_PRESSED  3
#define CBXS_DISABLED 4

// Treeview parts
#define TVP_TREEITEM  1
#define TVP_GLYPH     2
#define TVP_BRANCH    3
#define TVP_HOTGLYPH  4

// Treeview glyph states
#define GLPS_CLOSED 1
#define GLPS_OPENED 2

// Treeview item states
#define TREIS_NORMAL      1
#define TREIS_HOT         2
#define TREIS_SELECTED    3
#define TREIS_DISABLED    4
#define TREIS_SELECTEDNOTFOCUS 5
#define TREIS_HOTSELECTED 6

// Radio button states (BP_RADIOBUTTON)
#define RBS_UNCHECKEDNORMAL    1
#define RBS_UNCHECKEDHOT       2
#define RBS_UNCHECKEDPRESSED   3
#define RBS_UNCHECKEDDISABLED  4
#define RBS_CHECKEDNORMAL      5
#define RBS_CHECKEDHOT         6
#define RBS_CHECKEDPRESSED     7
#define RBS_CHECKEDDISABLED    8

// Checkbox states (BP_CHECKBOX)
#define CBS_UNCHECKEDNORMAL    1
#define CBS_UNCHECKEDHOT       2
#define CBS_UNCHECKEDPRESSED   3
#define CBS_UNCHECKEDDISABLED  4
#define CBS_CHECKEDNORMAL      5
#define CBS_CHECKEDHOT         6
#define CBS_CHECKEDPRESSED     7
#define CBS_CHECKEDDISABLED    8
#define CBS_MIXEDNORMAL        9
#define CBS_MIXEDHOT           10
#define CBS_MIXEDPRESSED       11
#define CBS_MIXEDDISABLED      12

// Groupbox states
#define GBS_NORMAL   1
#define GBS_DISABLED 2

// Combobox readonly dropdown states (CBXSR_*)
#define CBXSR_NORMAL   1
#define CBXSR_HOT      2
#define CBXSR_PRESSED  3
#define CBXSR_DISABLED 4

// Progress bar parts
#define PP_BAR       1
#define PP_BARVERT   2
#define PP_CHUNK     3
#define PP_CHUNKVERT 4
#define PP_FILL      5
#define PP_FILLVERT  6
#define PP_PULSEOVERLAY 7
#define PP_MOVEOVERLAY  8
#define PP_PULSEOVERLAYVERT 9
#define PP_MOVEOVERLAYVERT  10
#define PP_TRANSPARENTBAR   11
#define PP_TRANSPARENTBARVERT 12

// Progress bar fill states
#define PBFS_NORMAL  1
#define PBFS_ERROR   2
#define PBFS_PAUSED  3
#define PBFS_PARTIAL 4

// TMT transition durations
#define TMT_TRANSITIONDURATIONS 6000
