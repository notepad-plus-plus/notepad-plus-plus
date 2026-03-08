#pragma once
// Win32 Shim: Common Controls for macOS
// TabControl, ListView, TreeView, Toolbar, StatusBar, etc.

#include "windef.h"
#include "winuser.h"
#include <cstring>

// Color constants
#define CLR_NONE    0xFFFFFFFFL
#define CLR_DEFAULT 0xFF000000L
#define CLR_INVALID 0xFFFFFFFFL

// Common control styles (CCS_*)
#define CCS_TOP           0x00000001L
#define CCS_NOMOVEY       0x00000002L
#define CCS_BOTTOM        0x00000003L
#define CCS_NORESIZE      0x00000004L
#define CCS_NOPARENTALIGN 0x00000008L
#define CCS_ADJUSTABLE    0x00000020L
#define CCS_NODIVIDER     0x00000040L
#define CCS_VERT          0x00000080L

// Image list constants
#define I_IMAGECALLBACK   (-1)
#define I_IMAGENONE       (-2)
#define I_INDENTCALLBACK  (-1)
#define I_CHILDRENCALLBACK (-1)
#define I_GROUPIDCALLBACK (-1)
#define I_GROUPIDNONE     (-2)

// ============================================================
// Common Control Classes
// ============================================================
#define WC_TABCONTROLW   L"SysTabControl32"
#define WC_TABCONTROL    WC_TABCONTROLW
#define WC_LISTVIEWW     L"SysListView32"
#define WC_LISTVIEW      WC_LISTVIEWW
#define WC_TREEVIEWW     L"SysTreeView32"
#define WC_TREEVIEW      WC_TREEVIEWW
#define WC_HEADERW       L"SysHeader32"
#define WC_HEADER        WC_HEADERW
#define TOOLBARCLASSNAMEW L"ToolbarWindow32"
#define TOOLBARCLASSNAME  TOOLBARCLASSNAMEW
#define STATUSCLASSNAMEW  L"msctls_statusbar32"
#define STATUSCLASSNAME   STATUSCLASSNAMEW
#define TOOLTIPS_CLASSW   L"tooltips_class32"
#define TOOLTIPS_CLASS    TOOLTIPS_CLASSW
#define REBARCLASSNAMEW   L"ReBarWindow32"
#define REBARCLASSNAME    REBARCLASSNAMEW
#define PROGRESS_CLASSW   L"msctls_progress32"
#define PROGRESS_CLASS    PROGRESS_CLASSW
#define WC_STATICW        L"Static"
#define WC_STATIC         WC_STATICW
#define WC_BUTTONW        L"Button"
#define WC_BUTTON         WC_BUTTONW
#define WC_EDITW          L"Edit"
#define WC_EDIT           WC_EDITW
#define WC_COMBOBOXW      L"ComboBox"
#define WC_COMBOBOX       WC_COMBOBOXW
#define WC_LISTBOXW       L"ListBox"
#define WC_LISTBOX        WC_LISTBOXW
#define UPDOWN_CLASSW     L"msctls_updown32"
#define UPDOWN_CLASS      UPDOWN_CLASSW

// ============================================================
// InitCommonControlsEx
// ============================================================
typedef struct tagINITCOMMONCONTROLSEX {
	DWORD dwSize;
	DWORD dwICC;
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;

#define ICC_LISTVIEW_CLASSES 0x00000001
#define ICC_TREEVIEW_CLASSES 0x00000002
#define ICC_BAR_CLASSES      0x00000004
#define ICC_TAB_CLASSES      0x00000008
#define ICC_UPDOWN_CLASS     0x00000010
#define ICC_PROGRESS_CLASS   0x00000020
#define ICC_HOTKEY_CLASS     0x00000040
#define ICC_ANIMATE_CLASS    0x00000080
#define ICC_WIN95_CLASSES    0x000000FF
#define ICC_DATE_CLASSES     0x00000100
#define ICC_USEREX_CLASSES   0x00000200
#define ICC_COOL_CLASSES     0x00000400
#define ICC_STANDARD_CLASSES 0x00004000
#define ICC_LINK_CLASS       0x00008000

inline BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX* picce) { return TRUE; }
inline void InitCommonControls() {}

// ============================================================
// Tab Control
// ============================================================
#define TCM_FIRST           0x1300
#define TCM_INSERTITEMW     (TCM_FIRST + 62)
#define TCM_INSERTITEM      TCM_INSERTITEMW
#define TCM_DELETEITEM      (TCM_FIRST + 8)
#define TCM_DELETEALLITEMS  (TCM_FIRST + 9)
#define TCM_GETITEMRECT     (TCM_FIRST + 10)
#define TCM_GETCURSEL       (TCM_FIRST + 11)
#define TCM_SETCURSEL       (TCM_FIRST + 12)
#define TCM_GETITEMCOUNT    (TCM_FIRST + 4)
#define TCM_GETITEMW        (TCM_FIRST + 60)
#define TCM_GETITEM         TCM_GETITEMW
#define TCM_SETITEMW        (TCM_FIRST + 61)
#define TCM_SETITEM         TCM_SETITEMW
#define TCM_ADJUSTRECT      (TCM_FIRST + 40)
#define TCM_SETITEMSIZE     (TCM_FIRST + 41)
#define TCM_REMOVEIMAGE     (TCM_FIRST + 42)
#define TCM_SETPADDING      (TCM_FIRST + 43)
#define TCM_GETROWCOUNT     (TCM_FIRST + 44)
#define TCM_GETTOOLTIPS     (TCM_FIRST + 45)
#define TCM_SETTOOLTIPS     (TCM_FIRST + 46)
#define TCM_GETCURFOCUS     (TCM_FIRST + 47)
#define TCM_SETCURFOCUS     (TCM_FIRST + 48)
#define TCM_SETMINTABWIDTH  (TCM_FIRST + 49)
#define TCM_SETIMAGELIST    (TCM_FIRST + 3)
#define TCM_GETIMAGELIST    (TCM_FIRST + 2)
#define TCM_HITTEST         (TCM_FIRST + 13)
#define TCM_HIGHLIGHTITEM   (TCM_FIRST + 51)
#define TCM_SETEXTENDEDSTYLE (TCM_FIRST + 52)
#define TCM_GETEXTENDEDSTYLE (TCM_FIRST + 53)

// Tab control styles
#define TCS_SCROLLOPPOSITE  0x0001
#define TCS_BOTTOM          0x0002
#define TCS_RIGHT           0x0002
#define TCS_MULTISELECT     0x0004
#define TCS_FLATBUTTONS     0x0008
#define TCS_FORCEICONLEFT   0x0010
#define TCS_FORCELABELLEFT  0x0020
#define TCS_HOTTRACK        0x0040
#define TCS_VERTICAL        0x0080
#define TCS_TABS            0x0000
#define TCS_BUTTONS         0x0100
#define TCS_SINGLELINE      0x0000
#define TCS_MULTILINE       0x0200
#define TCS_RIGHTJUSTIFY    0x0000
#define TCS_FIXEDWIDTH      0x0400
#define TCS_RAGGEDRIGHT     0x0800
#define TCS_FOCUSONBUTTONDOWN 0x1000
#define TCS_OWNERDRAWFIXED  0x2000
#define TCS_TOOLTIPS        0x4000
#define TCS_FOCUSNEVER      0x8000
#define TCS_EX_FLATSEPARATORS 0x00000001
#define TCS_EX_REGISTERDROP   0x00000002

// Tab notifications
#define TCN_FIRST       (-550)
#define TCN_SELCHANGE   (TCN_FIRST - 1)
#define TCN_SELCHANGING (TCN_FIRST - 2)
#define TCN_GETOBJECT   (TCN_FIRST - 3)
#define TCN_FOCUSCHANGE (TCN_FIRST - 4)

// Tab item mask
#define TCIF_TEXT    0x0001
#define TCIF_IMAGE   0x0002
#define TCIF_RTLREADING 0x0004
#define TCIF_PARAM   0x0008
#define TCIF_STATE   0x0010

#define TCIS_BUTTONPRESSED 0x0001
#define TCIS_HIGHLIGHTED   0x0002

typedef struct tagTCITEMW {
	UINT    mask;
	DWORD   dwState;
	DWORD   dwStateMask;
	LPWSTR  pszText;
	int     cchTextMax;
	int     iImage;
	LPARAM  lParam;
} TCITEMW, *LPTCITEMW;
typedef TCITEMW TCITEM;
typedef LPTCITEMW LPTCITEM;

typedef struct tagTCHITTESTINFO {
	POINT pt;
	UINT  flags;
} TCHITTESTINFO, *LPTCHITTESTINFO;

#define TCHT_NOWHERE     0x0001
#define TCHT_ONITEMICON  0x0002
#define TCHT_ONITEMLABEL 0x0004
#define TCHT_ONITEM      (TCHT_ONITEMICON | TCHT_ONITEMLABEL)

// ============================================================
// ListView
// ============================================================
#define LVM_FIRST             0x1000
#define LVM_GETITEMCOUNT      (LVM_FIRST + 4)
#define LVM_GETITEMW          (LVM_FIRST + 75)
#define LVM_GETITEM           LVM_GETITEMW
#define LVM_SETITEMW          (LVM_FIRST + 76)
#define LVM_SETITEM           LVM_SETITEMW
#define LVM_INSERTITEMW       (LVM_FIRST + 77)
#define LVM_INSERTITEM        LVM_INSERTITEMW
#define LVM_DELETEITEM        (LVM_FIRST + 8)
#define LVM_DELETEALLITEMS    (LVM_FIRST + 9)
#define LVM_GETCOLUMNW        (LVM_FIRST + 95)
#define LVM_GETCOLUMN         LVM_GETCOLUMNW
#define LVM_SETCOLUMNW        (LVM_FIRST + 96)
#define LVM_SETCOLUMN         LVM_SETCOLUMNW
#define LVM_INSERTCOLUMNW     (LVM_FIRST + 97)
#define LVM_INSERTCOLUMN      LVM_INSERTCOLUMNW
#define LVM_DELETECOLUMN      (LVM_FIRST + 28)
#define LVM_GETCOLUMNWIDTH    (LVM_FIRST + 29)
#define LVM_SETCOLUMNWIDTH    (LVM_FIRST + 30)
#define LVM_GETNEXTITEM       (LVM_FIRST + 12)
#define LVM_GETITEMSTATE      (LVM_FIRST + 44)
#define LVM_SETITEMSTATE      (LVM_FIRST + 43)
#define LVM_GETITEMTEXTW      (LVM_FIRST + 115)
#define LVM_GETITEMTEXT       LVM_GETITEMTEXTW
#define LVM_SETITEMTEXTW      (LVM_FIRST + 116)
#define LVM_SETITEMTEXT       LVM_SETITEMTEXTW
#define LVM_GETSELECTEDCOUNT  (LVM_FIRST + 50)
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 55)
#define LVM_SETBKCOLOR        (LVM_FIRST + 1)
#define LVM_SETTEXTCOLOR      (LVM_FIRST + 36)
#define LVM_SETTEXTBKCOLOR    (LVM_FIRST + 38)
#define LVM_SETIMAGELIST      (LVM_FIRST + 3)
#define LVM_GETIMAGELIST      (LVM_FIRST + 2)
#define LVM_GETITEMRECT       (LVM_FIRST + 14)
#define LVM_HITTEST           (LVM_FIRST + 18)
#define LVM_ENSUREVISIBLE     (LVM_FIRST + 19)
#define LVM_SCROLL            (LVM_FIRST + 20)
#define LVM_REDRAWITEMS       (LVM_FIRST + 21)
#define LVM_ARRANGE           (LVM_FIRST + 22)
#define LVM_EDITLABELW       (LVM_FIRST + 118)
#define LVM_EDITLABEL        LVM_EDITLABELW
#define LVM_GETEDITCONTROL   (LVM_FIRST + 24)
#define LVM_SORTITEMS        (LVM_FIRST + 48)
#define LVM_SETITEMPOSITION32 (LVM_FIRST + 49)
#define LVM_GETSELECTIONMARK (LVM_FIRST + 66)
#define LVM_SETSELECTIONMARK (LVM_FIRST + 67)
#define LVM_SETVIEW          (LVM_FIRST + 142)
#define LVM_GETVIEW          (LVM_FIRST + 143)
#define LVM_SETGROUPINFO     (LVM_FIRST + 147)
#define LVM_GETGROUPINFO     (LVM_FIRST + 149)
#define LVM_REMOVEALLGROUPS  (LVM_FIRST + 160)
#define LVM_INSERTGROUP      (LVM_FIRST + 145)
#define LVM_ENABLEGROUPVIEW  (LVM_FIRST + 157)
#define LVM_ISGROUPVIEWENABLED (LVM_FIRST + 175)

// ListView group flags
#define LVGF_NONE       0x00000000
#define LVGF_HEADER     0x00000001
#define LVGF_FOOTER     0x00000002
#define LVGF_STATE      0x00000004
#define LVGF_ALIGN      0x00000008
#define LVGF_GROUPID    0x00000010

#define LVGS_NORMAL      0x00000000
#define LVGS_COLLAPSED   0x00000001
#define LVGS_HIDDEN      0x00000002
#define LVGS_COLLAPSIBLE 0x00000008

// ListView view modes
#define LV_VIEW_ICON      0x0000
#define LV_VIEW_DETAILS   0x0001
#define LV_VIEW_SMALLICON 0x0002
#define LV_VIEW_LIST      0x0003
#define LV_VIEW_TILE      0x0004

typedef struct tagLVGROUP {
	UINT   cbSize;
	UINT   mask;
	LPWSTR pszHeader;
	int    cchHeader;
	LPWSTR pszFooter;
	int    cchFooter;
	int    iGroupId;
	UINT   stateMask;
	UINT   state;
	UINT   uAlign;
} LVGROUP, *PLVGROUP;

#define ListView_InsertGroup(hwnd, index, pgrp) \
	(int)SendMessage((hwnd), LVM_INSERTGROUP, (WPARAM)(index), (LPARAM)(pgrp))
#define ListView_EnableGroupView(hwnd, fEnable) \
	(int)SendMessage((hwnd), LVM_ENABLEGROUPVIEW, (WPARAM)(fEnable), 0)
#define LVM_GETITEMCOUNT     (LVM_FIRST + 4)
#define LVM_GETSUBITEMRECT   (LVM_FIRST + 56)
#define LVM_SUBITEMHITTEST   (LVM_FIRST + 57)
#define LVM_SETCOLUMNORDERARRAY (LVM_FIRST + 58)
#define LVM_GETCOLUMNORDERARRAY (LVM_FIRST + 59)
#define LVM_GETHEADER        (LVM_FIRST + 31)
#define LVM_GETTOOLTIPS      (LVM_FIRST + 78)
#define LVM_SETTOOLTIPS      (LVM_FIRST + 74)
#define LVM_GETSTRINGWIDTHW  (LVM_FIRST + 87)
#define LVM_GETSTRINGWIDTH   LVM_GETSTRINGWIDTHW
#define LVM_SETSELECTEDCOLUMN (LVM_FIRST + 140)

// ListView icon/selection flags
#define LVIR_BOUNDS    0
#define LVIR_ICON      1
#define LVIR_LABEL     2
#define LVIR_SELECTBOUNDS 3

#define LVSICF_NOINVALIDATEALL 0x00000001
#define LVSICF_NOSCROLL        0x00000002

// ListView styles
#define LVS_ICON            0x0000
#define LVS_REPORT          0x0001
#define LVS_SMALLICON       0x0002
#define LVS_LIST            0x0003
#define LVS_TYPEMASK        0x0003
#define LVS_SINGLESEL       0x0004
#define LVS_SHOWSELALWAYS   0x0008
#define LVS_SORTASCENDING   0x0010
#define LVS_SORTDESCENDING  0x0020
#define LVS_SHAREIMAGELISTS 0x0040
#define LVS_NOLABELWRAP     0x0080
#define LVS_AUTOARRANGE     0x0100
#define LVS_EDITLABELS      0x0200
#define LVS_OWNERDATA       0x1000
#define LVS_NOSCROLL        0x2000
#define LVS_ALIGNTOP        0x0000
#define LVS_ALIGNLEFT       0x0800
#define LVS_NOCOLUMNHEADER  0x4000
#define LVS_NOSORTHEADER    0x8000
#define LVS_OWNERDRAWFIXED  0x0400
#define LVS_EX_GRIDLINES        0x00000001
#define LVS_EX_SUBITEMIMAGES    0x00000002
#define LVS_EX_CHECKBOXES      0x00000004
#define LVS_EX_FULLROWSELECT   0x00000020
#define LVS_EX_BORDERSELECT    0x00008000
#define LVS_EX_DOUBLEBUFFER    0x00010000
#define LVS_EX_INFOTIP         0x00000400
#define LVS_EX_LABELTIP        0x00004000
#define LVS_EX_HEADERDRAGDROP  0x00000010

#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 55)
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#define ListView_GetExtendedListViewStyle(hwnd) \
	(DWORD)SendMessage((hwnd), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0)
#define ListView_SetExtendedListViewStyle(hwnd, dw) \
	(DWORD)SendMessage((hwnd), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)(DWORD)(dw))

#define INDEXTOSTATEIMAGEMASK(i) ((i) << 12)

// ListView item
#define LVIF_TEXT       0x0001
#define LVIF_IMAGE      0x0002
#define LVIF_PARAM      0x0004
#define LVIF_STATE      0x0008
#define LVIF_INDENT     0x0010
#define LVIF_GROUPID    0x0100

#define LVIS_FOCUSED    0x0001
#define LVIS_SELECTED   0x0002
#define LVIS_CUT        0x0004
#define LVIS_DROPHILITED 0x0008
#define LVIS_STATEIMAGEMASK 0xF000

#define LVNI_ALL        0x0000
#define LVNI_FOCUSED    0x0001
#define LVNI_SELECTED   0x0002

typedef struct tagLVITEMW {
	UINT      mask;
	int       iItem;
	int       iSubItem;
	UINT      state;
	UINT      stateMask;
	LPWSTR    pszText;
	int       cchTextMax;
	int       iImage;
	LPARAM    lParam;
	int       iIndent;
	int       iGroupId;
	UINT      cColumns;
	PUINT     puColumns;
} LVITEMW, *LPLVITEMW;
typedef LVITEMW LVITEM;
typedef LPLVITEMW LPLVITEM;

#define LPSTR_TEXTCALLBACKW ((LPWSTR)-1)
#define LPSTR_TEXTCALLBACK  LPSTR_TEXTCALLBACKW

typedef struct tagLVHITTESTINFO {
	POINT pt;
	UINT  flags;
	int   iItem;
	int   iSubItem;
	int   iGroup;
} LVHITTESTINFO, *LPLVHITTESTINFO;

typedef struct tagNMITEMACTIVATE {
	NMHDR  hdr;
	int    iItem;
	int    iSubItem;
	UINT   uNewState;
	UINT   uOldState;
	UINT   uChanged;
	POINT  ptAction;
	LPARAM lParam;
	UINT   uKeyFlags;
} NMITEMACTIVATE, *LPNMITEMACTIVATE;

typedef struct tagLVCOLUMNW {
	UINT  mask;
	int   fmt;
	int   cx;
	LPWSTR pszText;
	int   cchTextMax;
	int   iSubItem;
	int   iImage;
	int   iOrder;
} LVCOLUMNW, *LPLVCOLUMNW;
typedef LVCOLUMNW LVCOLUMN;

#define LVCF_FMT      0x0001
#define LVCF_WIDTH    0x0002
#define LVCF_TEXT     0x0004
#define LVCF_SUBITEM  0x0008
#define LVCF_IMAGE    0x0010
#define LVCF_ORDER    0x0020

#define LVCFMT_LEFT   0x0000
#define LVCFMT_RIGHT  0x0001
#define LVCFMT_CENTER 0x0002

// ListView notifications
#define LVN_FIRST       (-100)
#define LVN_ITEMCHANGING (LVN_FIRST - 0)
#define LVN_ITEMCHANGED  (LVN_FIRST - 1)
#define LVN_INSERTITEM   (LVN_FIRST - 2)
#define LVN_DELETEITEM   (LVN_FIRST - 3)
#define LVN_DELETEALLITEMS (LVN_FIRST - 4)
#define LVN_BEGINLABELEDITW (LVN_FIRST - 75)
#define LVN_ENDLABELEDITW (LVN_FIRST - 76)
#define LVN_BEGINLABELEDIT LVN_BEGINLABELEDITW
#define LVN_ENDLABELEDIT LVN_ENDLABELEDITW
#define LVN_COLUMNCLICK  (LVN_FIRST - 8)
#define LVN_BEGINDRAG    (LVN_FIRST - 9)
#define LVN_BEGINRDRAG   (LVN_FIRST - 11)
#define LVN_ITEMACTIVATE (LVN_FIRST - 14)
#define LVN_GETDISPINFOW (LVN_FIRST - 77)
#define LVN_GETDISPINFO  LVN_GETDISPINFOW
#define LVN_SETDISPINFOW (LVN_FIRST - 78)
#define LVN_SETDISPINFO  LVN_SETDISPINFOW
#define LVN_KEYDOWN      (LVN_FIRST - 55)
#define LVN_ODSTATECHANGED (LVN_FIRST - 101)

typedef int (CALLBACK *PFNLVCOMPARE)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

typedef struct tagNMLISTVIEW {
	NMHDR  hdr;
	int    iItem;
	int    iSubItem;
	UINT   uNewState;
	UINT   uOldState;
	UINT   uChanged;
	POINT  ptAction;
	LPARAM lParam;
} NMLISTVIEW, *LPNMLISTVIEW;

typedef struct tagLVDISPINFOW {
	NMHDR    hdr;
	LVITEMW  item;
} NMLVDISPINFOW, *LPNMLVDISPINFOW;
typedef NMLVDISPINFOW NMLVDISPINFO;

typedef struct tagLVKEYDOWN {
	NMHDR hdr;
	WORD  wVKey;
	UINT  flags;
} NMLVKEYDOWN, *LPNMLVKEYDOWN;

// ============================================================
// TreeView
// ============================================================
#define TVM_FIRST               0x1100
#define TVM_INSERTITEMW         (TVM_FIRST + 50)
#define TVM_INSERTITEM          TVM_INSERTITEMW
#define TVM_DELETEITEM          (TVM_FIRST + 1)
#define TVM_EXPAND              (TVM_FIRST + 2)
#define TVM_GETITEMRECT         (TVM_FIRST + 4)
#define TVM_GETCOUNT            (TVM_FIRST + 5)
#define TVM_GETINDENT           (TVM_FIRST + 6)
#define TVM_SETINDENT           (TVM_FIRST + 7)
#define TVM_GETIMAGELIST        (TVM_FIRST + 8)
#define TVM_SETIMAGELIST        (TVM_FIRST + 9)
#define TVM_GETNEXTITEM         (TVM_FIRST + 10)
#define TVM_SELECTITEM          (TVM_FIRST + 11)
#define TVM_GETITEMW            (TVM_FIRST + 62)
#define TVM_GETITEM             TVM_GETITEMW
#define TVM_SETITEMW            (TVM_FIRST + 63)
#define TVM_SETITEM             TVM_SETITEMW
#define TVM_EDITLABELW          (TVM_FIRST + 65)
#define TVM_EDITLABEL           TVM_EDITLABELW
#define TVM_GETEDITCONTROL      (TVM_FIRST + 15)
#define TVM_GETVISIBLECOUNT     (TVM_FIRST + 16)
#define TVM_HITTEST             (TVM_FIRST + 17)
#define TVM_SORTCHILDREN        (TVM_FIRST + 19)
#define TVM_ENSUREVISIBLE       (TVM_FIRST + 20)
#define TVM_SORTCHILDRENCB      (TVM_FIRST + 21)
#define TVM_ENDEDITLABELNOW     (TVM_FIRST + 22)
#define TVM_SETTOOLTIPS         (TVM_FIRST + 24)
#define TVM_GETTOOLTIPS         (TVM_FIRST + 25)
#define TVM_SETBKCOLOR          (TVM_FIRST + 29)
#define TVM_SETTEXTCOLOR        (TVM_FIRST + 30)
#define TVM_GETBKCOLOR          (TVM_FIRST + 31)
#define TVM_GETTEXTCOLOR        (TVM_FIRST + 32)
#define TVM_SETITEMHEIGHT       (TVM_FIRST + 27)
#define TVM_GETITEMHEIGHT       (TVM_FIRST + 28)
#define TVM_CREATEDRAGIMAGE     (TVM_FIRST + 18)
#define TVM_SETSCROLLTIME       (TVM_FIRST + 33)
#define TVM_GETSCROLLTIME       (TVM_FIRST + 34)

// TreeView styles
#define TVS_HASBUTTONS     0x0001
#define TVS_HASLINES       0x0002
#define TVS_LINESATROOT    0x0004
#define TVS_EDITLABELS     0x0008
#define TVS_DISABLEDRAGDROP 0x0010
#define TVS_SHOWSELALWAYS  0x0020
#define TVS_CHECKBOXES     0x0100
#define TVS_TRACKSELECT    0x0200
#define TVS_SINGLEEXPAND   0x0400
#define TVS_INFOTIP        0x0800
#define TVS_FULLROWSELECT  0x1000
#define TVS_NOSCROLL       0x2000
#define TVS_NONEVENHEIGHT  0x4000

// TreeView expand constants
#define TVE_COLLAPSE 0x0001
#define TVE_EXPAND   0x0002
#define TVE_TOGGLE   0x0003

// TreeView next item constants
#define TVGN_ROOT       0x0000
#define TVGN_NEXT       0x0001
#define TVGN_PREVIOUS   0x0002
#define TVGN_PARENT     0x0003
#define TVGN_CHILD      0x0004
#define TVGN_FIRSTVISIBLE 0x0005
#define TVGN_NEXTVISIBLE  0x0006
#define TVGN_PREVIOUSVISIBLE 0x0007
#define TVGN_DROPHILITE  0x0008
#define TVGN_CARET       0x0009
#define TVGN_LASTVISIBLE 0x000A

#define TVI_ROOT  ((HTREEITEM)(ULONG_PTR)-0x10000)
#define TVI_FIRST ((HTREEITEM)(ULONG_PTR)-0x0FFFF)
#define TVI_LAST  ((HTREEITEM)(ULONG_PTR)-0x0FFFE)
#define TVI_SORT  ((HTREEITEM)(ULONG_PTR)-0x0FFFD)

// TreeView item mask
#define TVIF_TEXT       0x0001
#define TVIF_IMAGE      0x0002
#define TVIF_PARAM      0x0004
#define TVIF_STATE      0x0008
#define TVIF_HANDLE     0x0010
#define TVIF_SELECTEDIMAGE 0x0020
#define TVIF_CHILDREN   0x0040
#define TVIF_INTEGRAL   0x0080

// TreeView item states
#define TVIS_SELECTED     0x0002
#define TVIS_CUT          0x0004
#define TVIS_DROPHILITED  0x0008
#define TVIS_BOLD         0x0010
#define TVIS_EXPANDED     0x0020
#define TVIS_EXPANDEDONCE 0x0040
#define TVIS_EXPANDPARTIAL 0x0080
#define TVIS_STATEIMAGEMASK 0xF000

typedef struct tagTVITEMW {
	UINT      mask;
	HTREEITEM hItem;
	UINT      state;
	UINT      stateMask;
	LPWSTR    pszText;
	int       cchTextMax;
	int       iImage;
	int       iSelectedImage;
	int       cChildren;
	LPARAM    lParam;
} TVITEMW, *LPTVITEMW;
typedef TVITEMW TVITEM;

typedef struct tagTVITEMEXW {
	UINT      mask;
	HTREEITEM hItem;
	UINT      state;
	UINT      stateMask;
	LPWSTR    pszText;
	int       cchTextMax;
	int       iImage;
	int       iSelectedImage;
	int       cChildren;
	LPARAM    lParam;
	int       iIntegral;
	UINT      uStateEx;
	HWND      hwnd;
	int       iExpandedImage;
	int       iReserved;
} TVITEMEXW, *LPTVITEMEXW;
typedef TVITEMEXW TVITEMEX;

typedef struct tagTVINSERTSTRUCTW {
	HTREEITEM hParent;
	HTREEITEM hInsertAfter;
	union {
		TVITEMW    item;
		TVITEMEXW  itemex;
	};
} TVINSERTSTRUCTW, *LPTVINSERTSTRUCTW;
typedef TVINSERTSTRUCTW TVINSERTSTRUCT;

// TreeView hit test
typedef struct tagTVHITTESTINFO {
	POINT     pt;
	UINT      flags;
	HTREEITEM hItem;
} TVHITTESTINFO, *LPTVHITTESTINFO;

#define TVHT_NOWHERE      0x0001
#define TVHT_ONITEMICON   0x0002
#define TVHT_ONITEMLABEL  0x0004
#define TVHT_ONITEMINDENT 0x0008
#define TVHT_ONITEMBUTTON 0x0010
#define TVHT_ONITEMRIGHT  0x0020
#define TVHT_ONITEMSTATEICON 0x0040
#define TVHT_ONITEM       (TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)
#define TVHT_ABOVE        0x0100
#define TVHT_BELOW        0x0200
#define TVHT_TORIGHT      0x0400
#define TVHT_TOLEFT       0x0800

// TreeView notifications
#define TVN_FIRST        (-400)
#define TVN_SELCHANGINGW (TVN_FIRST - 50)
#define TVN_SELCHANGEDW  (TVN_FIRST - 51)
#define TVN_SELCHANGING  TVN_SELCHANGINGW
#define TVN_SELCHANGED   TVN_SELCHANGEDW
#define TVN_GETDISPINFOW (TVN_FIRST - 52)
#define TVN_GETDISPINFO  TVN_GETDISPINFOW
#define TVN_SETDISPINFOW (TVN_FIRST - 53)
#define TVN_SETDISPINFO  TVN_SETDISPINFOW
#define TVN_ITEMEXPANDINGW (TVN_FIRST - 54)
#define TVN_ITEMEXPANDING TVN_ITEMEXPANDINGW
#define TVN_ITEMEXPANDEDW (TVN_FIRST - 55)
#define TVN_ITEMEXPANDED TVN_ITEMEXPANDEDW
#define TVN_BEGINDRAGW   (TVN_FIRST - 56)
#define TVN_BEGINDRAG    TVN_BEGINDRAGW
#define TVN_BEGINRDRAGW  (TVN_FIRST - 57)
#define TVN_BEGINRDRAG   TVN_BEGINRDRAGW
#define TVN_DELETEITEMW  (TVN_FIRST - 58)
#define TVN_DELETEITEM   TVN_DELETEITEMW
#define TVN_BEGINLABELEDITW (TVN_FIRST - 59)
#define TVN_BEGINLABELEDIT TVN_BEGINLABELEDITW
#define TVN_ENDLABELEDITW (TVN_FIRST - 60)
#define TVN_ENDLABELEDIT TVN_ENDLABELEDITW
#define TVN_KEYDOWN      (TVN_FIRST - 12)
#define TVN_GETINFOTIPW  (TVN_FIRST - 14)
#define TVN_GETINFOTIP   TVN_GETINFOTIPW

typedef struct tagNMTREEVIEWW {
	NMHDR   hdr;
	UINT    action;
	TVITEMW itemOld;
	TVITEMW itemNew;
	POINT   ptDrag;
} NMTREEVIEWW, *LPNMTREEVIEWW;
typedef NMTREEVIEWW NMTREEVIEW;
#define LPNMTREEVIEW LPNMTREEVIEWW

// TreeView display info notification
typedef struct tagNMTVDISPINFOW {
	NMHDR   hdr;
	TVITEMW item;
} NMTVDISPINFOW, *LPNMTVDISPINFOW;
typedef NMTVDISPINFOW NMTVDISPINFO;
typedef LPNMTVDISPINFOW LPNMTVDISPINFO;

// TreeView get info tip notification
typedef struct tagNMTVGETINFOTIPW {
	NMHDR     hdr;
	LPWSTR    pszText;
	int       cchTextMax;
	HTREEITEM hItem;
	LPARAM    lParam;
} NMTVGETINFOTIPW, *LPNMTVGETINFOTIPW;
typedef NMTVGETINFOTIPW NMTVGETINFOTIP;
typedef LPNMTVGETINFOTIPW LPNMTVGETINFOTIP;

typedef struct tagTVKEYDOWN {
	NMHDR hdr;
	WORD  wVKey;
	UINT  flags;
} NMTVKEYDOWN, *LPNMTVKEYDOWN;

// TreeView macros
#define TreeView_InsertItem(hwnd, lpis) \
	(HTREEITEM)SendMessage((hwnd), TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)(lpis))
#define TreeView_DeleteItem(hwnd, hitem) \
	(BOOL)SendMessage((hwnd), TVM_DELETEITEM, 0, (LPARAM)(HTREEITEM)(hitem))
#define TreeView_DeleteAllItems(hwnd) \
	(BOOL)SendMessage((hwnd), TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT)
#define TreeView_Expand(hwnd, hitem, code) \
	(BOOL)SendMessage((hwnd), TVM_EXPAND, (WPARAM)(code), (LPARAM)(HTREEITEM)(hitem))
#define TreeView_GetNextItem(hwnd, hitem, code) \
	(HTREEITEM)SendMessage((hwnd), TVM_GETNEXTITEM, (WPARAM)(code), (LPARAM)(HTREEITEM)(hitem))
#define TreeView_GetRoot(hwnd) TreeView_GetNextItem(hwnd, NULL, TVGN_ROOT)
#define TreeView_GetChild(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_CHILD)
#define TreeView_GetParent(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_PARENT)
#define TreeView_GetNextSibling(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_NEXT)
#define TreeView_GetSelection(hwnd) TreeView_GetNextItem(hwnd, NULL, TVGN_CARET)
#define TreeView_SelectItem(hwnd, hitem) \
	(BOOL)SendMessage((hwnd), TVM_SELECTITEM, TVGN_CARET, (LPARAM)(HTREEITEM)(hitem))
#define TreeView_GetItem(hwnd, pitem) \
	(BOOL)SendMessage((hwnd), TVM_GETITEM, 0, (LPARAM)(TVITEMW*)(pitem))
#define TreeView_SetItem(hwnd, pitem) \
	(BOOL)SendMessage((hwnd), TVM_SETITEM, 0, (LPARAM)(const TVITEMW*)(pitem))
#define TreeView_GetPrevSibling(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_PREVIOUS)
#define TreeView_GetLastVisible(hwnd) TreeView_GetNextItem(hwnd, NULL, TVGN_LASTVISIBLE)
#define TreeView_GetCount(hwnd) \
	(UINT)SendMessage((hwnd), TVM_GETCOUNT, 0, 0)
#define TreeView_GetItemState(hwnd, hitem, mask) \
	(UINT)SendMessage((hwnd), TVM_GETITEMSTATE, (WPARAM)(hitem), (LPARAM)(mask))
#define TreeView_SetBkColor(hwnd, clr) \
	(COLORREF)SendMessage((hwnd), TVM_SETBKCOLOR, 0, (LPARAM)(clr))
#define TreeView_SetTextColor(hwnd, clr) \
	(COLORREF)SendMessage((hwnd), TVM_SETTEXTCOLOR, 0, (LPARAM)(clr))
#define TreeView_SetLineColor(hwnd, clr) \
	(COLORREF)SendMessage((hwnd), TVM_SETLINECOLOR, 0, (LPARAM)(clr))
#define TreeView_SetImageList(hwnd, himl, iImage) \
	(HIMAGELIST)SendMessage((hwnd), TVM_SETIMAGELIST, (WPARAM)(iImage), (LPARAM)(HIMAGELIST)(himl))
#define TreeView_EditLabel(hwnd, hitem) \
	(HWND)SendMessage((hwnd), TVM_EDITLABEL, 0, (LPARAM)(HTREEITEM)(hitem))
#define TreeView_EnsureVisible(hwnd, hitem) \
	(BOOL)SendMessage((hwnd), TVM_ENSUREVISIBLE, 0, (LPARAM)(HTREEITEM)(hitem))
#define TreeView_SortChildrenCB(hwnd, psort, fRecurse) \
	(BOOL)SendMessage((hwnd), TVM_SORTCHILDRENCB, (WPARAM)(fRecurse), (LPARAM)(LPTVSORTCB)(psort))
#define TreeView_HitTest(hwnd, lpht) \
	(HTREEITEM)SendMessage((hwnd), TVM_HITTEST, 0, (LPARAM)(LPTVHITTESTINFO)(lpht))
#define TreeView_GetEditControl(hwnd) \
	(HWND)SendMessage((hwnd), TVM_GETEDITCONTROL, 0, 0)
#define TreeView_SetItemHeight(hwnd, iHeight) \
	(int)SendMessage((hwnd), TVM_SETITEMHEIGHT, (WPARAM)(iHeight), 0)
#define TreeView_GetItemRect(hwnd, hitem, prc, code) \
	(*(HTREEITEM*)(prc) = (hitem), (BOOL)SendMessage((hwnd), TVM_GETITEMRECT, (WPARAM)(code), (LPARAM)(RECT*)(prc)))

typedef int (CALLBACK *PFNTVCOMPARE)(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
typedef struct tagTVSORTCB {
	HTREEITEM    hParent;
	PFNTVCOMPARE lpfnCompare;
	LPARAM       lParam;
} TVSORTCB, *LPTVSORTCB;

// ============================================================
// Toolbar
// ============================================================
#define TB_ENABLEBUTTON   (WM_USER + 1)
#define TB_CHECKBUTTON    (WM_USER + 2)
#define TB_PRESSBUTTON    (WM_USER + 3)
#define TB_HIDEBUTTON     (WM_USER + 4)
#define TB_INDETERMINATE  (WM_USER + 5)
#define TB_ISBUTTONENABLED   (WM_USER + 9)
#define TB_ISBUTTONCHECKED   (WM_USER + 10)
#define TB_ISBUTTONPRESSED   (WM_USER + 11)
#define TB_ISBUTTONHIDDEN    (WM_USER + 12)
#define TB_BUTTONSTRUCTSIZE  (WM_USER + 30)
#define TB_SETBITMAPSIZE     (WM_USER + 32)
#define TB_AUTOSIZE          (WM_USER + 33)
#define TB_GETTOOLTIPS       (WM_USER + 35)
#define TB_SETTOOLTIPS       (WM_USER + 36)
#define TB_SETPARENT         (WM_USER + 37)
#define TB_SETROWS           (WM_USER + 39)
#define TB_GETROWS           (WM_USER + 40)
#define TB_GETBITMAPFLAGS    (WM_USER + 41)
#define TB_SETCMDID          (WM_USER + 42)
#define TB_SETIMAGELIST      (WM_USER + 48)
#define TB_GETIMAGELIST      (WM_USER + 49)
#define TB_SETHOTIMAGELIST   (WM_USER + 52)
#define TB_SETDISABLEDIMAGELIST (WM_USER + 54)
#define TB_ADDBUTTONS        (WM_USER + 68)
#define TB_GETBUTTONSIZE     (WM_USER + 58)
#define TB_SETBUTTONSIZE     (WM_USER + 31)
#define TB_GETBUTTONINFOW    (WM_USER + 63)
#define TB_SETBUTTONINFOW    (WM_USER + 64)
#define TB_GETBUTTONINFO     TB_GETBUTTONINFOW
#define TB_SETBUTTONINFO     TB_SETBUTTONINFOW
#define TB_INSERTBUTTONW     (WM_USER + 67)
#define TB_INSERTBUTTON      TB_INSERTBUTTONW
#define TB_DELETEBUTTON      (WM_USER + 22)
#define TB_COMMANDTOINDEX    (WM_USER + 25)
#define TB_GETITEMRECT       (WM_USER + 29)
#define TB_BUTTONCOUNT       (WM_USER + 24)
#define TB_SETSTATE          (WM_USER + 17)
#define TB_GETSTATE          (WM_USER + 18)
#define TB_GETRECT           (WM_USER + 51)
#define TB_SETMAXTEXTROWS    (WM_USER + 60)
#define TB_GETPADDING        (WM_USER + 86)
#define TB_SETPADDING        (WM_USER + 87)
#define TB_SETDRAWTEXTFLAGS  (WM_USER + 70)
#define TB_GETBUTTON         (WM_USER + 23)
#define TB_ADDBITMAP         (WM_USER + 19)
#define TB_SAVERESTORE       (WM_USER + 76)
#define TB_CUSTOMIZE         (WM_USER + 27)

typedef struct tagTBADDBITMAP {
	HINSTANCE hInst;
	UINT_PTR  nID;
} TBADDBITMAP, *LPTBADDBITMAP;

#define TBSTATE_CHECKED       0x01
#define TBSTATE_PRESSED       0x02
#define TBSTATE_ENABLED       0x04
#define TBSTATE_HIDDEN        0x08
#define TBSTATE_INDETERMINATE 0x10
#define TBSTATE_WRAP          0x20

#define TBSTYLE_BUTTON    0x0000
#define TBSTYLE_SEP       0x0001
#define TBSTYLE_CHECK     0x0002
#define TBSTYLE_GROUP     0x0004
#define TBSTYLE_DROPDOWN  0x0008
#define TBSTYLE_AUTOSIZE  0x0010
#define TBSTYLE_FLAT      0x0800
#define TBSTYLE_LIST      0x1000
#define TBSTYLE_TOOLTIPS  0x0100
#define TBSTYLE_WRAPABLE  0x0200
#define TBSTYLE_ALTDRAG   0x0400
#define TBSTYLE_TRANSPARENT   0x8000
#define TBSTYLE_CUSTOMERASE   0x2000
#define TBSTYLE_REGISTERDROP  0x4000

// Toolbar extended styles
#define TBSTYLE_EX_DRAWDDARROWS        0x00000001
#define TBSTYLE_EX_MIXEDBUTTONS        0x00000008
#define TBSTYLE_EX_HIDECLIPPEDBUTTONS  0x00000010
#define TBSTYLE_EX_DOUBLEBUFFER        0x00000080

#define TB_GETEXTENDEDSTYLE  (WM_USER + 85)
#define TB_SETEXTENDEDSTYLE  (WM_USER + 84)

typedef struct _TBBUTTON {
	int   iBitmap;
	int   idCommand;
	BYTE  fsState;
	BYTE  fsStyle;
	BYTE  bReserved[2];
	DWORD_PTR dwData;
	INT_PTR   iString;
} TBBUTTON, *PTBBUTTON, *LPTBBUTTON;
typedef const TBBUTTON* LPCTBBUTTON;

// Toolbar notifications
#define TBN_FIRST     (-700)
#define TBN_DROPDOWN  (TBN_FIRST - 10)
#define TBN_GETINFOTIPW (TBN_FIRST - 19)
#define TBN_GETINFOTIP TBN_GETINFOTIPW

typedef struct tagNMTBGETINFOTIPW {
	NMHDR  hdr;
	LPWSTR pszText;
	int    cchTextMax;
	int    iItem;
	LPARAM lParam;
} NMTBGETINFOTIPW, *LPNMTBGETINFOTIPW;
typedef NMTBGETINFOTIPW NMTBGETINFOTIP;

typedef struct tagNMTOOLBARW {
	NMHDR    hdr;
	int      iItem;
	TBBUTTON tbButton;
	int      cchText;
	LPWSTR   pszText;
	RECT     rcButton;
} NMTOOLBARW, *LPNMTOOLBARW;
typedef NMTOOLBARW NMTOOLBAR;

// Toolbar button info
typedef struct {
	UINT  cbSize;
	DWORD dwMask;
	int   idCommand;
	int   iImage;
	BYTE  fsState;
	BYTE  fsStyle;
	WORD  cx;
	DWORD_PTR lParam;
	LPWSTR  pszText;
	int     cchText;
} TBBUTTONINFOW, *LPTBBUTTONINFOW;
typedef TBBUTTONINFOW TBBUTTONINFO;
typedef LPTBBUTTONINFOW LPTBBUTTONINFO;

#define TBIF_IMAGE   0x0001
#define TBIF_TEXT    0x0002
#define TBIF_STATE   0x0004
#define TBIF_STYLE   0x0008
#define TBIF_LPARAM  0x0010
#define TBIF_COMMAND 0x0020
#define TBIF_SIZE    0x0040
#define TBIF_BYINDEX 0x80000000

#define BTNS_BUTTON      0x0000
#define BTNS_SEP         0x0001
#define BTNS_CHECK       0x0002
#define BTNS_GROUP       0x0004
#define BTNS_CHECKGROUP  (BTNS_GROUP | BTNS_CHECK)
#define BTNS_DROPDOWN    0x0008
#define BTNS_AUTOSIZE    0x0010
#define BTNS_NOPREFIX    0x0020
#define BTNS_SHOWTEXT    0x0040
#define BTNS_WHOLEDROPDOWN 0x0080

#define TB_GETITEMDROPDOWNRECT (WM_USER + 103)

// ============================================================
// StatusBar
// ============================================================
#define SB_SETTEXTW       (WM_USER + 11)
#define SB_SETTEXT        SB_SETTEXTW
#define SB_GETTEXTW       (WM_USER + 13)
#define SB_GETTEXT        SB_GETTEXTW
#define SB_GETTEXTLENGTHW (WM_USER + 12)
#define SB_GETTEXTLENGTH  SB_GETTEXTLENGTHW
#define SB_SETPARTS       (WM_USER + 4)
#define SB_GETPARTS       (WM_USER + 6)
#define SB_GETBORDERS     (WM_USER + 7)
#define SB_SETMINHEIGHT   (WM_USER + 8)
#define SB_SIMPLE         (WM_USER + 9)
#define SB_GETRECT        (WM_USER + 10)
#define SB_SETICON        (WM_USER + 15)
#define SB_SETTIPTEXTW    (WM_USER + 17)
#define SB_SETTIPTEXT     SB_SETTIPTEXTW
#define SB_GETTIPTEXTW    (WM_USER + 19)
#define SB_GETTIPTEXT     SB_GETTIPTEXTW
#define SB_ISSIMPLE       (WM_USER + 14)

#define SBT_OWNERDRAW  0x1000
#define SBT_NOBORDERS  0x0100
#define SBT_POPOUT     0x0200
#define SBT_RTLREADING 0x0400
#define SBT_TOOLTIPS   0x0800

#define SBARS_SIZEGRIP 0x0100
#define SBARS_TOOLTIPS 0x0800

// ============================================================
// ToolTip
// ============================================================
#define TTM_ACTIVATE    (WM_USER + 1)
#define TTM_SETDELAYTIME (WM_USER + 3)
#define TTM_ADDTOOLW    (WM_USER + 50)
#define TTM_ADDTOOL     TTM_ADDTOOLW
#define TTM_DELTOOLW    (WM_USER + 51)
#define TTM_DELTOOL     TTM_DELTOOLW
#define TTM_NEWTOOLRECTW (WM_USER + 52)
#define TTM_NEWTOOLRECT TTM_NEWTOOLRECTW
#define TTM_GETTOOLINFOW (WM_USER + 53)
#define TTM_GETTOOLINFO TTM_GETTOOLINFOW
#define TTM_SETTOOLINFOW (WM_USER + 54)
#define TTM_SETTOOLINFO TTM_SETTOOLINFOW
#define TTM_HITTEST     (WM_USER + 55)
#define TTM_GETTEXTW    (WM_USER + 56)
#define TTM_GETTEXT     TTM_GETTEXTW
#define TTM_UPDATETIPTEXTW (WM_USER + 57)
#define TTM_UPDATETIPTEXT TTM_UPDATETIPTEXTW
#define TTM_GETTOOLCOUNT (WM_USER + 13)
#define TTM_ENUMTOOLSW  (WM_USER + 58)
#define TTM_ENUMTOOLS   TTM_ENUMTOOLSW
#define TTM_GETCURRENTTOOLW (WM_USER + 59)
#define TTM_GETCURRENTTOOL TTM_GETCURRENTTOOLW
#define TTM_WINDOWFROMPOINT (WM_USER + 16)
#define TTM_TRACKACTIVATE (WM_USER + 17)
#define TTM_TRACKPOSITION (WM_USER + 18)
#define TTM_SETTITLEA   (WM_USER + 20)
#define TTM_SETTITLEW   (WM_USER + 33)
#define TTM_SETTITLE    TTM_SETTITLEW
#define TTM_SETMAXTIPWIDTH (WM_USER + 24)
#define TTM_GETMAXTIPWIDTH (WM_USER + 25)
#define TTM_SETMARGIN   (WM_USER + 26)
#define TTM_GETMARGIN   (WM_USER + 27)
#define TTM_POP         (WM_USER + 28)
#define TTM_UPDATE      (WM_USER + 29)
#define TTM_POPUP       (WM_USER + 34)

#define TTF_IDISHWND   0x0001
#define TTF_CENTERTIP  0x0002
#define TTF_RTLREADING 0x0004
#define TTF_SUBCLASS   0x0010
#define TTF_TRACK      0x0020
#define TTF_ABSOLUTE   0x0080
#define TTF_TRANSPARENT 0x0100
#define TTF_DI_SETITEM 0x8000

#define TTDT_AUTOMATIC 0
#define TTDT_RESHOW    1
#define TTDT_AUTOPOP   2
#define TTDT_INITIAL   3

#define TTS_ALWAYSTIP 0x01
#define TTS_NOPREFIX  0x02
#define TTS_NOANIMATE 0x10
#define TTS_NOFADE    0x20
#define TTS_BALLOON   0x40
#define TTS_CLOSE     0x80

typedef struct tagTOOLINFOW {
	UINT      cbSize;
	UINT      uFlags;
	HWND      hwnd;
	UINT_PTR  uId;
	RECT      rect;
	HINSTANCE hinst;
	LPWSTR    lpszText;
	LPARAM    lParam;
	void*     lpReserved;
} TTTOOLINFOW, *LPTTTOOLINFOW;
typedef TTTOOLINFOW TOOLINFOW;
typedef TTTOOLINFOW TOOLINFO;
typedef LPTTTOOLINFOW LPTOOLINFO;

#define TTN_FIRST       (-520)
#define TTN_GETDISPINFOW (TTN_FIRST - 10)
#define TTN_GETDISPINFO  TTN_GETDISPINFOW
#define TTN_NEEDTEXTW    TTN_GETDISPINFOW
#define TTN_NEEDTEXT     TTN_GETDISPINFO

typedef struct tagNMTTDISPINFOW {
	NMHDR     hdr;
	LPWSTR    lpszText;
	WCHAR     szText[80];
	HINSTANCE hinst;
	UINT      uFlags;
	LPARAM    lParam;
} NMTTDISPINFOW, *LPNMTTDISPINFOW;
typedef NMTTDISPINFOW NMTTDISPINFO;
typedef NMTTDISPINFOW TOOLTIPTEXTW;
typedef NMTTDISPINFOW TOOLTIPTEXT;
typedef NMTTDISPINFOW* LPTOOLTIPTEXT;

// NMMOUSE
typedef struct tagNMMOUSE {
	NMHDR    hdr;
	DWORD_PTR dwItemSpec;
	DWORD_PTR dwItemData;
	POINT    pt;
	LPARAM   dwHitInfo;
} NMMOUSE, *LPNMMOUSE;

// ============================================================
// Rebar
// ============================================================
#define RB_INSERTBANDW  (WM_USER + 10)
#define RB_INSERTBAND   RB_INSERTBANDW
#define RB_DELETEBAND   (WM_USER + 2)
#define RB_GETBARINFO   (WM_USER + 3)
#define RB_SETBARINFO   (WM_USER + 4)
#define RB_GETBANDCOUNT (WM_USER + 12)
#define RB_GETROWCOUNT  (WM_USER + 13)
#define RB_GETROWHEIGHT (WM_USER + 14)
#define RB_SETBANDINFOW (WM_USER + 11)
#define RB_SETBANDINFO  RB_SETBANDINFOW
#define RB_GETBANDINFOW (WM_USER + 28)
#define RB_GETBANDINFO  RB_GETBANDINFOW
#define RB_IDTOINDEX    (WM_USER + 16)
#define RB_SIZETORECT   (WM_USER + 23)
#define RB_SETBKCOLOR   (WM_USER + 19)
#define RB_GETBKCOLOR   (WM_USER + 20)
#define RB_SETTEXTCOLOR (WM_USER + 22)
#define RB_GETTEXTCOLOR (WM_USER + 21)
#define RB_SHOWBAND     (WM_USER + 35)
#define RB_MOVEBAND     (WM_USER + 39)
#define RB_GETBARHEIGHT (WM_USER + 27)

#define RBBIM_STYLE     0x00000001
#define RBBIM_COLORS    0x00000002
#define RBBIM_TEXT      0x00000004
#define RBBIM_IMAGE     0x00000008
#define RBBIM_CHILD     0x00000010
#define RBBIM_CHILDSIZE 0x00000020
#define RBBIM_SIZE      0x00000040
#define RBBIM_BACKGROUND 0x00000080
#define RBBIM_ID        0x00000100
#define RBBIM_IDEALSIZE 0x00000200
#define RBBIM_LPARAM    0x00000400
#define RBBIM_HEADERSIZE       0x00000800
#define RBBIM_CHEVRONLOCATION  0x00001000
#define RBBIM_CHEVRONSTATE     0x00002000

#define RBBS_BREAK      0x00000001
#define RBBS_FIXEDSIZE  0x00000002
#define RBBS_CHILDEDGE  0x00000004
#define RBBS_HIDDEN     0x00000008
#define RBBS_NOVERT     0x00000010
#define RBBS_FIXEDBMP   0x00000020
#define RBBS_VARIABLEHEIGHT 0x00000040
#define RBBS_GRIPPERALWAYS  0x00000080
#define RBBS_NOGRIPPER      0x00000100
#define RBBS_USECHEVRON     0x00000200

#define RBS_VARHEIGHT   0x0200
#define RBS_BANDBORDERS 0x0400
#define RBS_FIXEDORDER  0x0800

typedef struct tagREBARINFO {
	UINT       cbSize;
	UINT       fMask;
	HIMAGELIST himl;
} REBARINFO, *LPREBARINFO;

typedef struct tagREBARBANDINFOW {
	UINT     cbSize;
	UINT     fMask;
	UINT     fStyle;
	COLORREF clrFore;
	COLORREF clrBack;
	LPWSTR   lpText;
	UINT     cch;
	int      iImage;
	HWND     hwndChild;
	UINT     cxMinChild;
	UINT     cyMinChild;
	UINT     cx;
	HBITMAP  hbmBack;
	UINT     wID;
	UINT     cyChild;
	UINT     cyMaxChild;
	UINT     cyIntegral;
	UINT     cxIdeal;
	LPARAM   lParam;
	UINT     cxHeader;
	RECT     rcChevronLocation;
	UINT     uChevronState;
} REBARBANDINFOW, *LPREBARBANDINFOW;
typedef REBARBANDINFOW REBARBANDINFO;

#define RBN_FIRST       (-831)
#define RBN_HEIGHTCHANGE    (RBN_FIRST - 0)
#define RBN_CHEVRONPUSHED   (RBN_FIRST - 10)

typedef struct tagNMREBARCHEVRON {
	NMHDR  hdr;
	UINT   uBand;
	UINT   wID;
	LPARAM lParam;
	RECT   rc;
	LPARAM lParamNM;
} NMREBARCHEVRON, *LPNMREBARCHEVRON;

// ============================================================
// Image lists
// ============================================================
#define ILC_COLOR    0x0000
#define ILC_COLOR4   0x0004
#define ILC_COLOR8   0x0008
#define ILC_COLOR16  0x0010
#define ILC_COLOR24  0x0018
#define ILC_COLOR32  0x0020
#define ILC_MASK     0x0001
#define ILC_MIRROR   0x00002000

#define ILD_NORMAL      0x0000
#define ILD_TRANSPARENT 0x0001
#define ILD_BLEND25     0x0002
#define ILD_BLEND50     0x0004
#define ILD_MASK        0x0010
#define ILD_IMAGE       0x0020
#define ILD_SELECTED    0x0004
#define ILD_FOCUS       0x0002
#define ILD_OVERLAYMASK 0x0F00

#define LVSIL_NORMAL   0
#define LVSIL_SMALL    1
#define LVSIL_STATE    2
#define TVSIL_NORMAL   0
#define TVSIL_STATE    2

HIMAGELIST ImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow);
BOOL ImageList_Destroy(HIMAGELIST himl);
int ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask);
int ImageList_AddMasked(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask);
int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon);
BOOL ImageList_Remove(HIMAGELIST himl, int i);
int ImageList_GetImageCount(HIMAGELIST himl);
BOOL ImageList_SetImageCount(HIMAGELIST himl, UINT uNewCount);
BOOL ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle);

#define ImageList_AddIcon(himl, hicon) ImageList_ReplaceIcon(himl, -1, hicon)
#define ImageList_RemoveAll(himl) ImageList_Remove(himl, -1)
BOOL ImageList_SetIconSize(HIMAGELIST himl, int cx, int cy);
HICON ImageList_GetIcon(HIMAGELIST himl, int i, UINT flags);
BOOL ImageList_GetIconSize(HIMAGELIST himl, int* cx, int* cy);

typedef struct _IMAGEINFO {
	HBITMAP hbmImage;
	HBITMAP hbmMask;
	int     Unused1;
	int     Unused2;
	RECT    rcImage;
} IMAGEINFO, *LPIMAGEINFO;

BOOL ImageList_GetImageInfo(HIMAGELIST himl, int i, IMAGEINFO* pImageInfo);
BOOL ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle);

// ImageList drag functions
BOOL ImageList_BeginDrag(HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot);
BOOL ImageList_DragEnter(HWND hwndLock, int x, int y);
BOOL ImageList_DragMove(int x, int y);
BOOL ImageList_DragShowNolock(BOOL fShow);
BOOL ImageList_DragLeave(HWND hwndLock);
void ImageList_EndDrag();
HIMAGELIST ImageList_Merge(HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy);

// ============================================================
// Header
// ============================================================
#define HDM_FIRST     0x1200
#define HDM_GETITEMCOUNT (HDM_FIRST + 0)
#define HDM_INSERTITEMW  (HDM_FIRST + 10)
#define HDM_INSERTITEM   HDM_INSERTITEMW
#define HDM_DELETEITEM   (HDM_FIRST + 2)
#define HDM_GETITEMW     (HDM_FIRST + 11)
#define HDM_GETITEM      HDM_GETITEMW
#define HDM_SETITEMW     (HDM_FIRST + 12)
#define HDM_SETITEM      HDM_SETITEMW
#define HDM_LAYOUT       (HDM_FIRST + 5)

#define HDI_WIDTH   0x0001
#define HDI_HEIGHT  HDI_WIDTH
#define HDI_TEXT    0x0002
#define HDI_FORMAT  0x0004
#define HDI_LPARAM  0x0008
#define HDI_BITMAP  0x0010
#define HDI_IMAGE   0x0020
#define HDI_ORDER   0x0080
#define HDI_FILTER  0x0100

#define HDF_LEFT    0x0000
#define HDF_RIGHT   0x0001
#define HDF_CENTER  0x0002
#define HDF_SORTDOWN 0x0200
#define HDF_SORTUP   0x0400
#define HDF_STRING   0x4000
#define HDF_OWNERDRAW 0x8000

typedef struct _HD_ITEMW {
	UINT    mask;
	int     cxy;
	LPWSTR  pszText;
	HBITMAP hbm;
	int     cchTextMax;
	int     fmt;
	LPARAM  lParam;
	int     iImage;
	int     iOrder;
	UINT    type;
	LPVOID  pvFilter;
} HDITEMW, *LPHDITEMW;
typedef HDITEMW HDITEM;

// Header notifications
#define HDN_FIRST       (-300)
#define HDN_ITEMCHANGINGW (HDN_FIRST - 20)
#define HDN_ITEMCHANGEDW  (HDN_FIRST - 21)
#define HDN_ITEMCLICKW    (HDN_FIRST - 22)
#define HDN_ITEMDBLCLICKW (HDN_FIRST - 23)
#define HDN_DIVIDERDBLCLICKW (HDN_FIRST - 25)
#define HDN_BEGINTRACKW   (HDN_FIRST - 26)
#define HDN_ENDTRACKW     (HDN_FIRST - 27)
#define HDN_TRACKW        (HDN_FIRST - 28)
#define HDN_ITEMCHANGING  HDN_ITEMCHANGINGW
#define HDN_ITEMCHANGED   HDN_ITEMCHANGEDW
#define HDN_ITEMCLICK     HDN_ITEMCLICKW
#define HDN_ITEMDBLCLICK  HDN_ITEMDBLCLICKW
#define HDN_DIVIDERDBLCLICK HDN_DIVIDERDBLCLICKW
#define HDN_BEGINTRACK    HDN_BEGINTRACKW
#define HDN_ENDTRACK      HDN_ENDTRACKW
#define HDN_TRACK         HDN_TRACKW

typedef struct tagNMHEADERW {
	NMHDR   hdr;
	int     iItem;
	int     iButton;
	HDITEMW* pitem;
} NMHEADERW, *LPNMHEADERW;
typedef NMHEADERW NMHEADER;

// ============================================================
// Custom draw
// ============================================================
#define CDDS_PREPAINT    0x00000001
#define CDDS_POSTPAINT   0x00000002
#define CDDS_PREERASE    0x00000003
#define CDDS_POSTERASE   0x00000004
#define CDDS_ITEM        0x00010000
#define CDDS_ITEMPREPAINT (CDDS_ITEM | CDDS_PREPAINT)
#define CDDS_ITEMPOSTPAINT (CDDS_ITEM | CDDS_POSTPAINT)
#define CDDS_ITEMPREERASE (CDDS_ITEM | CDDS_PREERASE)
#define CDDS_ITEMPOSTERASE (CDDS_ITEM | CDDS_POSTERASE)
#define CDDS_SUBITEM     0x00020000

#define CDRF_DODEFAULT   0x00000000
#define CDRF_NEWFONT     0x00000002
#define CDRF_SKIPDEFAULT 0x00000004
#define CDRF_NOTIFYPOSTPAINT 0x00000010
#define CDRF_NOTIFYITEMDRAW  0x00000020
#define CDRF_NOTIFYSUBITEMDRAW 0x00000020
#define CDRF_NOTIFYPOSTERASE 0x00000040

// Toolbar-specific custom draw return flags
#define TBCDRF_NOEDGES       0x00010000
#define TBCDRF_HILITEHOTTRACK 0x00020000
#define TBCDRF_NOOFFSET      0x00040000
#define TBCDRF_NOMARK        0x00080000
#define TBCDRF_NOETCHEDEFFECT 0x00100000
#define TBCDRF_BLENDICON     0x00200000
#define TBCDRF_NOBACKGROUND  0x00400000
#define TBCDRF_USECDCOLORS   0x00800000

// Toolbar dropdown return values
#define TBDDRET_DEFAULT     0
#define TBDDRET_NODEFAULT   1
#define TBDDRET_TREATPRESSED 2

// Accessibility state constants
#define STATE_SYSTEM_UNAVAILABLE     0x00000001
#define STATE_SYSTEM_SELECTED        0x00000002
#define STATE_SYSTEM_FOCUSED         0x00000004
#define STATE_SYSTEM_PRESSED         0x00000008
#define STATE_SYSTEM_CHECKED         0x00000010
#define STATE_SYSTEM_HOTTRACKED      0x00000080
#define STATE_SYSTEM_DEFAULT         0x00000100

#define CDIS_SELECTED 0x0001
#define CDIS_GRAYED   0x0002
#define CDIS_DISABLED 0x0004
#define CDIS_CHECKED  0x0008
#define CDIS_FOCUS    0x0010
#define CDIS_DEFAULT  0x0020
#define CDIS_HOT      0x0040
#define CDIS_MARKED   0x0080

typedef struct tagNMCUSTOMDRAWINFO {
	NMHDR     hdr;
	DWORD     dwDrawStage;
	HDC       hdc;
	RECT      rc;
	DWORD_PTR dwItemSpec;
	UINT      uItemState;
	LPARAM    lItemlParam;
} NMCUSTOMDRAW, *LPNMCUSTOMDRAW;

typedef struct tagNMLVCUSTOMDRAW {
	NMCUSTOMDRAW nmcd;
	COLORREF     clrText;
	COLORREF     clrTextBk;
	int          iSubItem;
	DWORD        dwItemType;
	COLORREF     clrFace;
	int          iIconEffect;
	int          iIconPhase;
	int          iPartId;
	int          iStateId;
	RECT         rcText;
	UINT         uAlign;
} NMLVCUSTOMDRAW, *LPNMLVCUSTOMDRAW;

typedef struct tagNMTVCUSTOMDRAW {
	NMCUSTOMDRAW nmcd;
	COLORREF     clrText;
	COLORREF     clrTextBk;
	int          iLevel;
} NMTVCUSTOMDRAW, *LPNMTVCUSTOMDRAW;

typedef struct tagNMTBCUSTOMDRAW {
	NMCUSTOMDRAW nmcd;
	HBRUSH       hbrMonoDither;
	HBRUSH       hbrLines;
	HPEN         hpenLines;
	COLORREF     clrText;
	COLORREF     clrMark;
	COLORREF     clrTextHighlight;
	COLORREF     clrBtnFace;
	COLORREF     clrBtnHighlight;
	COLORREF     clrHighlightHotTrack;
	RECT         rcText;
	int          nStringBkMode;
	int          nHLStringBkMode;
	int          iListGap;
} NMTBCUSTOMDRAW, *LPNMTBCUSTOMDRAW;

// ============================================================
// Progress bar
// ============================================================
#define PBM_SETRANGE   (WM_USER + 1)
#define PBM_SETPOS     (WM_USER + 2)
#define PBM_DELTAPOS   (WM_USER + 3)
#define PBM_SETSTEP    (WM_USER + 4)
#define PBM_STEPIT     (WM_USER + 5)
#define PBM_SETRANGE32 (WM_USER + 6)
#define PBM_GETRANGE   (WM_USER + 7)
#define PBM_GETPOS     (WM_USER + 8)
#define PBM_SETBARCOLOR (WM_USER + 9)
#define PBM_SETBKCOLOR  (0x2001)

#define PBS_SMOOTH   0x01
#define PBS_VERTICAL 0x04
#define PBS_MARQUEE  0x08

// ============================================================
// Up-Down control
// ============================================================
#define UDM_SETRANGE   (WM_USER + 101)
#define UDM_GETRANGE   (WM_USER + 102)
#define UDM_SETPOS     (WM_USER + 103)
#define UDM_GETPOS     (WM_USER + 104)
#define UDM_SETBUDDY   (WM_USER + 105)
#define UDM_GETBUDDY   (WM_USER + 106)
#define UDM_SETRANGE32 (WM_USER + 111)
#define UDM_GETRANGE32 (WM_USER + 112)
#define UDM_SETPOS32   (WM_USER + 113)
#define UDM_GETPOS32   (WM_USER + 114)

#define UDS_WRAP        0x0001
#define UDS_SETBUDDYINT 0x0002
#define UDS_ALIGNRIGHT  0x0004
#define UDS_ALIGNLEFT   0x0008
#define UDS_AUTOBUDDY   0x0010
#define UDS_ARROWKEYS   0x0020
#define UDS_HORZ        0x0040
#define UDS_NOTHOUSANDS 0x0080
#define UDS_HOTTRACK    0x0100

#define UDN_FIRST        (-721)
#define UDN_DELTAPOS     (UDN_FIRST - 1)

typedef struct _NM_UPDOWN {
	NMHDR hdr;
	int   iPos;
	int   iDelta;
} NMUPDOWN, *LPNMUPDOWN;

// ============================================================
// Miscellaneous helper macros
// ============================================================
#define ListView_InsertItem(hwnd, pitem) \
	(int)SendMessage((hwnd), LVM_INSERTITEM, 0, (LPARAM)(const LVITEMW*)(pitem))
#define ListView_SetItem(hwnd, pitem) \
	(BOOL)SendMessage((hwnd), LVM_SETITEM, 0, (LPARAM)(const LVITEMW*)(pitem))
#define ListView_GetItem(hwnd, pitem) \
	(BOOL)SendMessage((hwnd), LVM_GETITEM, 0, (LPARAM)(LVITEMW*)(pitem))
#define ListView_DeleteItem(hwnd, i) \
	(BOOL)SendMessage((hwnd), LVM_DELETEITEM, (WPARAM)(int)(i), 0L)
#define ListView_DeleteAllItems(hwnd) \
	(BOOL)SendMessage((hwnd), LVM_DELETEALLITEMS, 0, 0L)
#define ListView_GetItemCount(hwnd) \
	(int)SendMessage((hwnd), LVM_GETITEMCOUNT, 0, 0L)
#define ListView_InsertColumn(hwnd, iCol, pcol) \
	(int)SendMessage((hwnd), LVM_INSERTCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(const LVCOLUMNW*)(pcol))
#define ListView_SetColumnWidth(hwnd, iCol, cx) \
	(BOOL)SendMessage((hwnd), LVM_SETCOLUMNWIDTH, (WPARAM)(int)(iCol), MAKELPARAM((cx), 0))
#define ListView_GetNextItem(hwnd, i, flags) \
	(int)SendMessage((hwnd), LVM_GETNEXTITEM, (WPARAM)(int)(i), MAKELPARAM((flags), 0))
#define ListView_SetItemState(hwnd, i, data, mask) \
	{ LVITEM _lvi; _lvi.stateMask = mask; _lvi.state = data; SendMessage((hwnd), LVM_SETITEMSTATE, (WPARAM)(i), (LPARAM)&_lvi); }
#define ListView_GetSelectedCount(hwnd) \
	(UINT)SendMessage((hwnd), LVM_GETSELECTEDCOUNT, 0, 0L)
#define ListView_SetImageList(hwnd, himl, iImageList) \
	(HIMAGELIST)SendMessage((hwnd), LVM_SETIMAGELIST, (WPARAM)(iImageList), (LPARAM)(HIMAGELIST)(himl))
#define ListView_SetColumn(hwnd, iCol, pcol) \
	(BOOL)SendMessage((hwnd), LVM_SETCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(const LVCOLUMN*)(pcol))
#define ListView_GetSelectionMark(hwnd) \
	(int)SendMessage((hwnd), LVM_GETSELECTIONMARK, 0, 0)
#define ListView_SetSelectionMark(hwnd, i) \
	(int)SendMessage((hwnd), LVM_SETSELECTIONMARK, 0, (LPARAM)(i))
#define ListView_EnsureVisible(hwnd, i, fPartialOK) \
	(BOOL)SendMessage((hwnd), LVM_ENSUREVISIBLE, (WPARAM)(int)(i), MAKELPARAM((fPartialOK), 0))
#define ListView_GetItemText(hwnd, i, iSubItem_, pszText_, cchTextMax_) \
	{ LVITEM _lvi; _lvi.iSubItem = iSubItem_; _lvi.cchTextMax = cchTextMax_; _lvi.pszText = pszText_; \
	SendMessage((hwnd), LVM_GETITEMTEXT, (WPARAM)(i), (LPARAM)&_lvi); }
#define ListView_SetItemText(hwnd, i, iSubItem_, pszText_) \
	{ LVITEM _lvi; _lvi.iSubItem = iSubItem_; _lvi.pszText = pszText_; \
	SendMessage((hwnd), LVM_SETITEMTEXT, (WPARAM)(i), (LPARAM)&_lvi); }
#define ListView_GetItemState(hwnd, i, mask) \
	(UINT)SendMessage((hwnd), LVM_GETITEMSTATE, (WPARAM)(i), (LPARAM)(mask))
#define ListView_GetCheckState(hwnd, i) \
	((((UINT)(SendMessage((hwnd), LVM_GETITEMSTATE, (WPARAM)(i), LVIS_STATEIMAGEMASK))) >> 12) - 1)
#define ListView_SetCheckState(hwnd, i, fCheck) \
	ListView_SetItemState(hwnd, i, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), LVIS_STATEIMAGEMASK)
#define ListView_SortItems(hwnd, pfnCompare, lPrm) \
	(BOOL)SendMessage((hwnd), LVM_SORTITEMS, (WPARAM)(LPARAM)(lPrm), (LPARAM)(PFNLVCOMPARE)(pfnCompare))
#define ListView_GetColumnWidth(hwnd, iCol) \
	(int)SendMessage((hwnd), LVM_GETCOLUMNWIDTH, (WPARAM)(int)(iCol), 0)
#define ListView_Scroll(hwnd, dx, dy) \
	(BOOL)SendMessage((hwnd), LVM_SCROLL, (WPARAM)(int)(dx), (LPARAM)(int)(dy))
#define ListView_SubItemHitTest(hwnd, plvhti) \
	(int)SendMessage((hwnd), LVM_SUBITEMHITTEST, 0, (LPARAM)(LPLVHITTESTINFO)(plvhti))
#define ListView_SetBkColor(hwnd, clrBk) \
	(BOOL)SendMessage((hwnd), LVM_SETBKCOLOR, 0, (LPARAM)(COLORREF)(clrBk))
#define ListView_SetTextBkColor(hwnd, clrTextBk) \
	(BOOL)SendMessage((hwnd), LVM_SETTEXTBKCOLOR, 0, (LPARAM)(COLORREF)(clrTextBk))
#define ListView_SetTextColor(hwnd, clrText) \
	(BOOL)SendMessage((hwnd), LVM_SETTEXTCOLOR, 0, (LPARAM)(COLORREF)(clrText))

#define LVM_SORTITEMSEX  (LVM_FIRST + 81)
#define ListView_SortItemsEx(hwnd, pfnCompare, lPrm) \
	(BOOL)SendMessage((hwnd), LVM_SORTITEMSEX, (WPARAM)(LPARAM)(lPrm), (LPARAM)(PFNLVCOMPARE)(pfnCompare))
#define ListView_GetHeader(hwnd) \
	(HWND)SendMessage((hwnd), LVM_GETHEADER, 0, 0)

// ListView custom draw / group rect
#define LVCDI_ITEM  0x00000000
#define LVCDI_GROUP 0x00000001

#define LVGGR_GROUP     0
#define LVGGR_HEADER    1
#define LVGGR_LABEL     2
#define LVGGR_SUBSETLINK 3

#define LVM_GETGROUPRECT (LVM_FIRST + 98)
#define ListView_GetGroupRect(hwnd, iGroupId, type, prc) \
	(BOOL)SendMessage((hwnd), LVM_GETGROUPRECT, (WPARAM)(iGroupId), ((prc) ? (((RECT*)(prc))->top = (type)), (LPARAM)(RECT*)(prc) : (LPARAM)(RECT*)NULL))

// ListView infotip notification
#define LVN_GETINFOTIPW  (LVN_FIRST - 58)
#define LVN_GETINFOTIP   LVN_GETINFOTIPW

typedef struct tagNMLVGETINFOTIPW {
	NMHDR  hdr;
	DWORD  dwFlags;
	LPWSTR pszText;
	int    cchTextMax;
	int    iItem;
	int    iSubItem;
	LPARAM lParam;
} NMLVGETINFOTIPW, *LPNMLVGETINFOTIPW;
typedef NMLVGETINFOTIPW NMLVGETINFOTIP;
typedef LPNMLVGETINFOTIPW LPNMLVGETINFOTIP;

// NMHEADER alias
typedef NMHEADERW NMHEADER;
typedef LPNMHEADERW LPNMHEADER;

// Header macros
#define Header_GetItem(hwndHD, i, phdi) \
	(BOOL)SendMessage((hwndHD), HDM_GETITEM, (WPARAM)(int)(i), (LPARAM)(HDITEMW*)(phdi))
#define Header_SetItem(hwndHD, i, phdi) \
	(BOOL)SendMessage((hwndHD), HDM_SETITEM, (WPARAM)(int)(i), (LPARAM)(HDITEMW*)(phdi))
#define Header_GetItemCount(hwndHD) \
	(int)SendMessage((hwndHD), HDM_GETITEMCOUNT, 0, 0L)
#define HDM_GETITEMRECT  (HDM_FIRST + 7)
#define Header_GetItemRect(hwndHD, i, lprc) \
	(BOOL)SendMessage((hwndHD), HDM_GETITEMRECT, (WPARAM)(int)(i), (LPARAM)(RECT*)(lprc))

#define TabCtrl_InsertItem(hwnd, iItem, pitem) \
	(int)SendMessage((hwnd), TCM_INSERTITEM, (WPARAM)(int)(iItem), (LPARAM)(const TCITEMW*)(pitem))
#define TabCtrl_DeleteItem(hwnd, i) \
	(BOOL)SendMessage((hwnd), TCM_DELETEITEM, (WPARAM)(int)(i), 0L)
#define TabCtrl_DeleteAllItems(hwnd) \
	(BOOL)SendMessage((hwnd), TCM_DELETEALLITEMS, 0, 0L)
#define TabCtrl_GetCurSel(hwnd) \
	(int)SendMessage((hwnd), TCM_GETCURSEL, 0, 0)
#define TabCtrl_SetCurSel(hwnd, i) \
	(int)SendMessage((hwnd), TCM_SETCURSEL, (WPARAM)(i), 0)
#define TabCtrl_GetItemCount(hwnd) \
	(int)SendMessage((hwnd), TCM_GETITEMCOUNT, 0, 0L)
#define TabCtrl_GetItem(hwnd, iItem, pitem) \
	(BOOL)SendMessage((hwnd), TCM_GETITEM, (WPARAM)(int)(iItem), (LPARAM)(TCITEMW*)(pitem))
#define TabCtrl_SetItem(hwnd, iItem, pitem) \
	(BOOL)SendMessage((hwnd), TCM_SETITEM, (WPARAM)(int)(iItem), (LPARAM)(const TCITEMW*)(pitem))
#define TabCtrl_SetItemSize(hwnd, cx, cy) \
	(DWORD)SendMessage((hwnd), TCM_SETITEMSIZE, 0, MAKELPARAM((cx),(cy)))
#define TabCtrl_GetItemRect(hwnd, i, prc) \
	(BOOL)SendMessage((hwnd), TCM_GETITEMRECT, (WPARAM)(int)(i), (LPARAM)(RECT*)(prc))
#define TabCtrl_AdjustRect(hwnd, bLarger, prc) \
	(int)SendMessage((hwnd), TCM_ADJUSTRECT, (WPARAM)(BOOL)(bLarger), (LPARAM)(RECT*)(prc))
#define TabCtrl_SetPadding(hwnd, cx, cy) \
	(void)SendMessage((hwnd), TCM_SETPADDING, 0, MAKELPARAM(cx, cy))

#define TCM_SETITEMSIZE   (TCM_FIRST + 41)
#define TCM_GETITEMRECT   (TCM_FIRST + 10)
#define TCM_ADJUSTRECT    (TCM_FIRST + 40)
#define TCM_SETPADDING    (TCM_FIRST + 43)
#define TCM_GETCURFOCUS   (TCM_FIRST + 47)
#define TCM_SETCURFOCUS   (TCM_FIRST + 48)

#define TabCtrl_GetCurFocus(hwnd) \
	(int)SendMessage((hwnd), TCM_GETCURFOCUS, 0, 0)
#define TabCtrl_SetCurFocus(hwnd, i) \
	SendMessage((hwnd), TCM_SETCURFOCUS, (WPARAM)(i), 0)
#define TabCtrl_GetRowCount(hwnd) \
	(int)SendMessage((hwnd), TCM_GETROWCOUNT, 0, 0)
#define TabCtrl_HitTest(hwnd, pinfo) \
	(int)SendMessage((hwnd), TCM_HITTEST, 0, (LPARAM)(LPTCHITTESTINFO)(pinfo))
#define TabCtrl_GetImageList(hwnd) \
	(HIMAGELIST)SendMessage((hwnd), TCM_GETIMAGELIST, 0, 0)
#define TabCtrl_SetImageList(hwnd, himl) \
	(HIMAGELIST)SendMessage((hwnd), TCM_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST)(himl))

#define ListView_GetStringWidth(hwnd, psz) \
	(int)SendMessage((hwnd), LVM_GETSTRINGWIDTH, 0, (LPARAM)(LPCWSTR)(psz))
#define ListView_GetItemRect(hwnd, i, prc, code) \
	(BOOL)SendMessage((hwnd), LVM_GETITEMRECT, (WPARAM)(int)(i), \
	((prc) ? (((RECT*)(prc))->left = (code), (LPARAM)(RECT*)(prc)) : (LPARAM)nullptr))

// Old-style typedef aliases
#define TC_HITTESTINFO TCHITTESTINFO

#define ListView_GetImageList(hwnd, iImageList) \
	(HIMAGELIST)SendMessage((hwnd), LVM_GETIMAGELIST, (WPARAM)(iImageList), 0L)
#define ListView_DeleteColumn(hwnd, iCol) \
	(BOOL)SendMessage((hwnd), LVM_DELETECOLUMN, (WPARAM)(int)(iCol), 0)
#define ListView_GetColumn(hwnd, iCol, pcol) \
	(BOOL)SendMessage((hwnd), LVM_GETCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(LVCOLUMNW*)(pcol))
#define ListView_HitTest(hwnd, pinfo) \
	(int)SendMessage((hwnd), LVM_HITTEST, 0, (LPARAM)(LPLVHITTESTINFO)(pinfo))
#define ListView_RedrawItems(hwnd, iFirst, iLast) \
	(BOOL)SendMessage((hwnd), LVM_REDRAWITEMS, (WPARAM)(int)(iFirst), (LPARAM)(int)(iLast))
#define ListView_GetTopIndex(hwnd) \
	(int)SendMessage((hwnd), LVM_GETTOPINDEX, 0, 0)
#define ListView_GetCountPerPage(hwnd) \
	(int)SendMessage((hwnd), LVM_GETCOUNTPERPAGE, 0, 0)
#define ListView_SetItemCountEx(hwnd, cItems, dwFlags) \
	(void)SendMessage((hwnd), LVM_SETITEMCOUNT, (WPARAM)(cItems), (LPARAM)(dwFlags))
#define ListView_SetItemCount(hwnd, cItems) \
	(void)SendMessage((hwnd), LVM_SETITEMCOUNT, (WPARAM)(cItems), 0)

// Note: LVM_GETIMAGELIST, LVM_DELETECOLUMN, LVM_GETCOLUMN, etc. defined above
#ifndef LVM_HITTEST
#define LVM_HITTEST          (LVM_FIRST + 18)
#endif
#ifndef LVM_REDRAWITEMS
#define LVM_REDRAWITEMS      (LVM_FIRST + 21)
#endif
#ifndef LVM_GETTOPINDEX
#define LVM_GETTOPINDEX      (LVM_FIRST + 39)
#endif
#ifndef LVM_GETCOUNTPERPAGE
#define LVM_GETCOUNTPERPAGE  (LVM_FIRST + 40)
#endif
#ifndef LVM_SETITEMCOUNT
#define LVM_SETITEMCOUNT     (LVM_FIRST + 47)
#endif

// ============================================================
// Trackbar (slider) messages
// ============================================================
#define TBM_GETPOS     (WM_USER)
#define TBM_GETRANGEMIN (WM_USER + 1)
#define TBM_GETRANGEMAX (WM_USER + 2)
#define TBM_SETPOS     (WM_USER + 5)
#define TBM_SETRANGE   (WM_USER + 6)
#define TBM_SETRANGEMIN (WM_USER + 7)
#define TBM_SETRANGEMAX (WM_USER + 8)
#define TBM_SETTIC     (WM_USER + 4)
#define TBM_SETPAGESIZE (WM_USER + 21)
#define TBM_GETPAGESIZE (WM_USER + 22)
#define TBM_SETLINESIZE (WM_USER + 23)
#define TBM_GETLINESIZE (WM_USER + 24)

// ============================================================
// Button control notification
// ============================================================
#define BCN_FIRST      (-1250)
#define BCN_DROPDOWN   (BCN_FIRST + 0x0002)

// ============================================================
// ComboBoxInfo
// ============================================================
typedef struct tagCOMBOBOXINFO {
	DWORD cbSize;
	RECT  rcItem;
	RECT  rcButton;
	DWORD stateButton;
	HWND  hwndCombo;
	HWND  hwndItem;
	HWND  hwndList;
} COMBOBOXINFO, *PCOMBOBOXINFO, *LPCOMBOBOXINFO;

inline BOOL GetComboBoxInfo(HWND hwndCombo, PCOMBOBOXINFO pcbi)
{
	(void)hwndCombo;
	if (pcbi) memset(pcbi, 0, pcbi->cbSize);
	return FALSE;
}

// ============================================================
// Edit balloon tips
// ============================================================
#define EM_SHOWBALLOONTIP  (WM_USER + 1503)
#define EM_HIDEBALLOONTIP  (WM_USER + 1504)

#define TTI_NONE           0
#define TTI_INFO           1
#define TTI_WARNING        2
#define TTI_ERROR          3
#define TTI_INFO_LARGE     4
#define TTI_WARNING_LARGE  5
#define TTI_ERROR_LARGE    6

typedef struct _tagEDITBALLOONTIP {
	DWORD   cbStruct;
	LPCWSTR pszTitle;
	LPCWSTR pszText;
	INT     ttiIcon;
} EDITBALLOONTIP, *PEDITBALLOONTIP;

// ============================================================
// Icon loading with DPI scaling (stub)
// ============================================================
inline HRESULT LoadIconWithScaleDown(HINSTANCE hinst, LPCWSTR pszName, int cx, int cy, HICON* phico)
{
	(void)hinst; (void)pszName; (void)cx; (void)cy;
	if (phico) *phico = nullptr;
	return 0x80004005L; // E_FAIL
}

// ============================================================
// Button control macros
// ============================================================
#define BCM_FIRST           0x1600
#define BCM_GETIDEALSIZE    (BCM_FIRST + 0x0001)
#define BCM_SETIMAGELIST    (BCM_FIRST + 0x0002)
#define BCM_GETIMAGELIST    (BCM_FIRST + 0x0003)
#define BCM_SETTEXTMARGIN   (BCM_FIRST + 0x0004)
#define BCM_GETTEXTMARGIN   (BCM_FIRST + 0x0005)
#define BCM_SETDROPDOWNSTATE (BCM_FIRST + 0x0006)
#define BCM_SETSPLITINFO    (BCM_FIRST + 0x0007)
#define BCM_GETSPLITINFO    (BCM_FIRST + 0x0008)
#define BCM_SETNOTE         (BCM_FIRST + 0x0009)
#define BCM_GETNOTE         (BCM_FIRST + 0x000A)
#define BCM_GETNOTELENGTH   (BCM_FIRST + 0x000B)
#define BCM_SETSHIELD       (BCM_FIRST + 0x000C)

#define Button_GetIdealSize(hwnd, psize) \
	(BOOL)SendMessage((hwnd), BCM_GETIDEALSIZE, 0, (LPARAM)(psize))

// ============================================================
// Progress bar messages
// ============================================================
#define PBM_SETRANGE    (WM_USER + 1)
#define PBM_SETPOS      (WM_USER + 2)
#define PBM_DELTAPOS    (WM_USER + 3)
#define PBM_SETSTEP     (WM_USER + 4)
#define PBM_STEPIT      (WM_USER + 5)
#define PBM_SETRANGE32  (WM_USER + 6)
#define PBM_GETRANGE    (WM_USER + 7)
#define PBM_GETPOS      (WM_USER + 8)
#define PBM_SETBARCOLOR (WM_USER + 9)
#define PBM_SETBKCOLOR  0x2001
#define PBM_SETMARQUEE  (WM_USER + 10)
#define PBM_SETSTATE    (WM_USER + 16)
#define PBM_GETSTATE    (WM_USER + 17)

#define PBST_NORMAL  0x0001
#define PBST_ERROR   0x0002
#define PBST_PAUSED  0x0003

typedef struct {
	int iLow;
	int iHigh;
} PBRANGE, *PPBRANGE;

// ============================================================
// TrackMouseEvent HOVER_DEFAULT
// ============================================================
#ifndef HOVER_DEFAULT
#define HOVER_DEFAULT 0xFFFFFFFF
#endif

// ============================================================
// SysLink control
// ============================================================
#define WC_LINK  L"SysLink"

#define LM_HITTEST        (WM_USER + 0x0300)
#define LM_GETIDEALHEIGHT (WM_USER + 0x0301)
#define LM_SETITEM        (WM_USER + 0x0302)
#define LM_GETITEM        (WM_USER + 0x0303)

#define LIF_ITEMINDEX 0x00000001
#define LIF_STATE     0x00000002
#define LIF_ITEMID    0x00000004
#define LIF_URL       0x00000008

#define LIS_FOCUSED       0x00000001
#define LIS_ENABLED       0x00000002
#define LIS_VISITED       0x00000004
#define LIS_HOTTRACK      0x00000008
#define LIS_DEFAULTCOLORS 0x00000010

#define MAX_LINKID_TEXT  48

typedef struct tagLITEM {
	UINT    mask;
	int     iLink;
	UINT    state;
	UINT    stateMask;
	WCHAR   szID[MAX_LINKID_TEXT];
	WCHAR   szUrl[2083]; // INTERNET_MAX_URL_LENGTH
} LITEM, *PLITEM;

typedef struct tagNMLINK {
	NMHDR  hdr;
	LITEM  item;
} NMLINK, *PNMLINK;

// ============================================================
// ComboBoxEx class
// ============================================================
#define WC_COMBOBOXEXW  L"ComboBoxEx32"
#define WC_COMBOBOXEX   WC_COMBOBOXEXW

// ============================================================
// Trackbar class
// ============================================================
#define TRACKBAR_CLASSW L"msctls_trackbar32"
#define TRACKBAR_CLASS  TRACKBAR_CLASSW

// Trackbar custom draw constants
#define TBCD_TICS    0x0001
#define TBCD_THUMB   0x0002
#define TBCD_CHANNEL 0x0003

// ============================================================
// Color scheme
// ============================================================
typedef struct tagCOLORSCHEME {
	DWORD    dwSize;
	COLORREF clrBtnHighlight;
	COLORREF clrBtnShadow;
} COLORSCHEME, *LPCOLORSCHEME;

#define TB_SETCOLORSCHEME (0x2000 + 2)
#define TB_GETCOLORSCHEME (0x2000 + 3)
#define RB_SETCOLORSCHEME (0x2000 + 2)
#define RB_GETCOLORSCHEME (0x2000 + 3)
