#pragma once
// Win32 Shim: Visual Styles constants for macOS

// VSCLASS names
#define VSCLASS_STATUS        L"STATUS"
#define VSCLASS_BUTTON        L"BUTTON"
#define VSCLASS_EDIT          L"EDIT"
#define VSCLASS_TAB           L"TAB"
#define VSCLASS_TOOLTIP       L"TOOLTIP"
#define VSCLASS_HEADER        L"HEADER"
#define VSCLASS_LISTVIEW      L"LISTVIEW"
#define VSCLASS_TREEVIEW      L"TREEVIEW"
#define VSCLASS_TOOLBAR       L"TOOLBAR"
#define VSCLASS_SCROLLBAR     L"SCROLLBAR"
#define VSCLASS_REBAR         L"REBAR"
#define VSCLASS_COMBOBOX      L"COMBOBOX"
#define VSCLASS_PROGRESS      L"PROGRESS"
#define VSCLASS_SPIN          L"SPIN"
#define VSCLASS_MENU          L"MENU"
#define VSCLASS_WINDOW        L"WINDOW"

// Status bar parts
#define SP_PANE     1
#define SP_GRIPPERPANE 2
#define SP_GRIPPER  3

// Theme sizes
#define TS_MIN     0
#define TS_TRUE    1
#define TS_DRAW    2

// Button parts
#define BP_PUSHBUTTON   1
#define BP_RADIOBUTTON  2
#define BP_CHECKBOX     3
#define BP_GROUPBOX     4
#define BP_COMMANDLINK  6

// Button states
#define PBS_NORMAL   1
#define PBS_HOT      2
#define PBS_PRESSED  3
#define PBS_DISABLED 4
#define PBS_DEFAULTED 5

// Tab parts
#define TABP_TABITEM          1
#define TABP_TABITEMLEFTEDGE  2
#define TABP_TABITEMRIGHTEDGE 3
#define TABP_TABITEMBOTHEDGE  4
#define TABP_TOPTABITEM       5
#define TABP_PANE             9
#define TABP_BODY             10

// Tab item states
#define TIS_NORMAL   1
#define TIS_HOT      2
#define TIS_SELECTED 3
#define TIS_DISABLED 4
#define TIS_FOCUSED  5

// Tooltip parts
#define TTP_STANDARD      1
#define TTP_STANDARDTITLE 2
#define TTP_BALLOON       3
#define TTP_BALLOONTITLE  4
#define TTP_CLOSE         5

// Edit parts
#define EP_EDITTEXT     1
#define EP_CARET        2
#define EP_BACKGROUND   3
#define EP_PASSWORD     4
#define EP_BACKGROUNDWITHBORDER 5
#define EP_EDITBORDER_NOSCROLL 6
#define EP_EDITBORDER_HSCROLL  7
#define EP_EDITBORDER_VSCROLL  8
#define EP_EDITBORDER_HVSCROLL 9

// Edit states
#define ETS_NORMAL   1
#define ETS_HOT      2
#define ETS_SELECTED 3
#define ETS_DISABLED 4
#define ETS_FOCUSED  5
#define ETS_READONLY 6
