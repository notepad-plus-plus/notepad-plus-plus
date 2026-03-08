#pragma once
// Win32 Shim: Window management, messages, and user input for macOS

#include "windef.h"
#include <cwctype>
#include <cstring>

// ============================================================
// Window Messages
// ============================================================
#define WM_NULL            0x0000
#define WM_CREATE          0x0001
#define WM_DESTROY         0x0002
#define WM_MOVE            0x0003
#define WM_SIZE            0x0005
#define WM_ACTIVATE        0x0006
#define WM_SETFOCUS        0x0007
#define WM_KILLFOCUS       0x0008
#define WM_ENABLE          0x000A
#define WM_SETREDRAW       0x000B
#define WM_SETTEXT         0x000C
#define WM_GETTEXT         0x000D
#define WM_GETTEXTLENGTH   0x000E
#define WM_PAINT           0x000F
#define WM_CLOSE           0x0010
#define WM_QUERYENDSESSION 0x0011
#define WM_QUIT            0x0012
#define WM_QUERYOPEN       0x0013
#define WM_ERASEBKGND      0x0014
#define WM_SYSCOLORCHANGE  0x0015
#define WM_ENDSESSION      0x0016
#define WM_SHOWWINDOW      0x0018
#define WM_SETTINGCHANGE   0x001A
#define WM_WININICHANGE    0x001A
#define WM_DEVMODECHANGE   0x001B
#define WM_ACTIVATEAPP     0x001C
#define WM_FONTCHANGE      0x001D
#define WM_TIMECHANGE      0x001E
#define WM_CANCELMODE      0x001F
#define WM_SETCURSOR       0x0020
#define WM_MOUSEACTIVATE   0x0021
#define WM_CHILDACTIVATE   0x0022
#define WM_QUEUESYNC       0x0023
#define WM_GETMINMAXINFO   0x0024

#define WM_DRAWITEM        0x002B
#define WM_MEASUREITEM     0x002C
#define WM_DELETEITEM      0x002D
#define WM_VKEYTOITEM      0x002E
#define WM_CHARTOITEM      0x002F

#define WM_SETFONT         0x0030
#define WM_GETFONT         0x0031
#define WM_SETHOTKEY       0x0032
#define WM_GETHOTKEY       0x0033
#define WM_COMPAREITEM     0x0039
#define WM_GETOBJECT       0x003D
#define WM_NEXTDLGCTL      0x0028

#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED  0x0047
#define WM_NOTIFY            0x004E
#define WM_INPUTLANGCHANGEREQUEST 0x0050
#define WM_INPUTLANGCHANGE   0x0051
#define WM_HELP              0x0053
#define WM_USERCHANGED       0x0054
#define WM_NOTIFYFORMAT      0x0055

#define WM_CONTEXTMENU       0x007B
#define WM_STYLECHANGING     0x007C
#define WM_STYLECHANGED      0x007D
#define WM_DISPLAYCHANGE     0x007E
#define WM_GETICON           0x007F
#define WM_SETICON           0x0080

#define WM_NCCREATE          0x0081
#define WM_NCDESTROY         0x0082
#define WM_NCCALCSIZE        0x0083
#define WM_NCHITTEST         0x0084
#define WM_NCPAINT           0x0085
#define WM_NCACTIVATE        0x0086
#define WM_GETDLGCODE        0x0087

#define WM_NCMOUSEMOVE       0x00A0
#define WM_NCLBUTTONDOWN     0x00A1
#define WM_NCLBUTTONUP       0x00A2
#define WM_NCLBUTTONDBLCLK   0x00A3
#define WM_NCRBUTTONDOWN     0x00A4
#define WM_NCRBUTTONUP       0x00A5
#define WM_NCRBUTTONDBLCLK   0x00A6

#define WM_KEYDOWN           0x0100
#define WM_KEYUP             0x0101
#define WM_CHAR              0x0102
#define WM_DEADCHAR          0x0103
#define WM_SYSKEYDOWN        0x0104
#define WM_SYSKEYUP          0x0105
#define WM_SYSCHAR           0x0106
#define WM_SYSDEADCHAR       0x0107

#define WM_IME_STARTCOMPOSITION 0x010D
#define WM_IME_ENDCOMPOSITION   0x010E
#define WM_IME_COMPOSITION      0x010F
#define WM_IME_REQUEST          0x0288

#define WM_INITDIALOG        0x0110
#define WM_COMMAND           0x0111
#define WM_SYSCOMMAND        0x0112

// SysCommand values
#define SC_SIZE          0xF000
#define SC_MOVE          0xF010
#define SC_MINIMIZE      0xF020
#define SC_MAXIMIZE      0xF030
#define SC_CLOSE         0xF060
#define SC_RESTORE       0xF120
#define SC_KEYMENU       0xF100
#define SC_MOUSEMENU     0xF090

// End session parameters
#define ENDSESSION_CLOSEAPP  0x00000001
#define ENDSESSION_CRITICAL  0x40000000
#define ENDSESSION_LOGOFF    0x80000000
#define WM_TIMER             0x0113
#define WM_HSCROLL           0x0114
#define WM_VSCROLL           0x0115
#define WM_INITMENU          0x0116
#define WM_INITMENUPOPUP     0x0117
#define WM_MENUSELECT        0x011F
#define WM_MENUCHAR          0x0120
#define WM_ENTERIDLE         0x0121
#define WM_MENURBUTTONUP     0x0122
#define WM_MENUGETOBJECT     0x0124
#define WM_UNINITMENUPOPUP   0x0125
#define WM_MENUCOMMAND       0x0126

#define WM_CTLCOLORMSGBOX    0x0132
#define WM_CTLCOLOREDIT      0x0133
#define WM_CTLCOLORLISTBOX   0x0134
#define WM_CTLCOLORBTN       0x0135
#define WM_CTLCOLORDLG       0x0136
#define WM_CTLCOLORSCROLLBAR 0x0137
#define WM_CTLCOLORSTATIC    0x0138

#define WM_MOUSEMOVE         0x0200
#define WM_LBUTTONDOWN       0x0201
#define WM_LBUTTONUP         0x0202
#define WM_LBUTTONDBLCLK     0x0203
#define WM_RBUTTONDOWN       0x0204
#define WM_RBUTTONUP         0x0205
#define WM_RBUTTONDBLCLK     0x0206
#define WM_MBUTTONDOWN       0x0207
#define WM_MBUTTONUP         0x0208
#define WM_MBUTTONDBLCLK     0x0209
#define WM_MOUSEWHEEL        0x020A
#define WM_MOUSEHWHEEL       0x020E
#define WM_XBUTTONDOWN       0x020B
#define WM_XBUTTONUP         0x020C
#define WM_XBUTTONDBLCLK     0x020D

#define WM_PARENTNOTIFY      0x0210
#define WM_ENTERMENULOOP     0x0211
#define WM_EXITMENULOOP      0x0212
#define WM_NEXTMENU          0x0213
#define WM_SIZING            0x0214
#define WM_CAPTURECHANGED    0x0215
#define WM_MOVING            0x0216
#define WM_POWERBROADCAST    0x0218

#define WM_MDICREATE         0x0220
#define WM_MDIDESTROY        0x0221
#define WM_MDIACTIVATE       0x0222
#define WM_MDIRESTORE        0x0223
#define WM_MDINEXT           0x0224
#define WM_MDIMAXIMIZE       0x0225
#define WM_MDITILE           0x0226
#define WM_MDICASCADE        0x0227
#define WM_MDIICONARRANGE    0x0228
#define WM_MDIGETACTIVE      0x0229
#define WM_MDISETMENU        0x0230

#define WM_ENTERSIZEMOVE     0x0231
#define WM_EXITSIZEMOVE      0x0232
#define WM_DROPFILES         0x0233
#define WM_MDIREFRESHMENU    0x0234

#define WM_MOUSELEAVE        0x02A3
#define WM_MOUSEHOVER        0x02A1

#define WM_CUT               0x0300
#define WM_COPY              0x0301
#define WM_PASTE             0x0302
#define WM_CLEAR             0x0303
#define WM_UNDO              0x0304

#define WM_HOTKEY            0x0312
#define WM_SYNCPAINT         0x0088

#define WM_PRINT             0x0317
#define WM_PRINTCLIENT       0x0318
#define WM_THEMECHANGED      0x031A
#define WM_DPICHANGED        0x02E0
#define WM_CHANGEUISTATE     0x0127
#define WM_UPDATEUISTATE     0x0128
#define WM_QUERYUISTATE      0x0129
#define UIS_SET              1
#define UIS_CLEAR            2
#define UIS_INITIALIZE       3
#define UISF_HIDEFOCUS       0x1
#define UISF_HIDEACCEL       0x2
#define WM_COPYDATA          0x004A
#define WM_APPCOMMAND        0x0319

// COPYDATASTRUCT for WM_COPYDATA
typedef struct tagCOPYDATASTRUCT {
	ULONG_PTR dwData;
	DWORD     cbData;
	PVOID     lpData;
} COPYDATASTRUCT, *PCOPYDATASTRUCT;

// App command macros
#define APPCOMMAND_BROWSER_BACKWARD  1
#define APPCOMMAND_BROWSER_FORWARD   2
#define APPCOMMAND_BROWSER_REFRESH   3
#define APPCOMMAND_BROWSER_STOP      4
#define APPCOMMAND_BROWSER_SEARCH    5
#define APPCOMMAND_BROWSER_FAVORITES 6
#define APPCOMMAND_BROWSER_HOME      7
#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~0xF000))

#define WM_APP               0x8000
#define WM_USER              0x0400

// ============================================================
// WM_ACTIVATE states
// ============================================================
#define WA_INACTIVE    0
#define WA_ACTIVE      1
#define WA_CLICKACTIVE 2

// ============================================================
// WM_SIZE types
// ============================================================
#define SIZE_RESTORED  0
#define SIZE_MINIMIZED 1
#define SIZE_MAXIMIZED 2
#define SIZE_MAXSHOW   3
#define SIZE_MAXHIDE   4

// ============================================================
// Virtual Key Codes
// ============================================================
#define VK_LBUTTON   0x01
#define VK_RBUTTON   0x02
#define VK_CANCEL    0x03
#define VK_MBUTTON   0x04
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_CLEAR     0x0C
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_MENU      0x12  // Alt
#define VK_PAUSE     0x13
#define VK_CAPITAL   0x14  // Caps Lock
#define VK_ESCAPE    0x1B
#define VK_SPACE     0x20
#define VK_PRIOR     0x21  // Page Up
#define VK_NEXT      0x22  // Page Down
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_SELECT    0x29
#define VK_PRINT     0x2A
#define VK_EXECUTE   0x2B
#define VK_SNAPSHOT  0x2C  // Print Screen
#define VK_INSERT    0x2D
#define VK_DELETE    0x2E
#define VK_HELP      0x2F

// 0-9 keys are 0x30-0x39 (ASCII)
// A-Z keys are 0x41-0x5A (ASCII)

#define VK_LWIN      0x5B
#define VK_RWIN      0x5C
#define VK_APPS      0x5D
#define VK_SLEEP     0x5F

#define VK_NUMPAD0   0x60
#define VK_NUMPAD1   0x61
#define VK_NUMPAD2   0x62
#define VK_NUMPAD3   0x63
#define VK_NUMPAD4   0x64
#define VK_NUMPAD5   0x65
#define VK_NUMPAD6   0x66
#define VK_NUMPAD7   0x67
#define VK_NUMPAD8   0x68
#define VK_NUMPAD9   0x69
#define VK_MULTIPLY  0x6A
#define VK_ADD       0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT  0x6D
#define VK_DECIMAL   0x6E
#define VK_DIVIDE    0x6F

#define VK_F1        0x70
#define VK_F2        0x71
#define VK_F3        0x72
#define VK_F4        0x73
#define VK_F5        0x74
#define VK_F6        0x75
#define VK_F7        0x76
#define VK_F8        0x77
#define VK_F9        0x78
#define VK_F10       0x79
#define VK_F11       0x7A
#define VK_F12       0x7B
#define VK_F13       0x7C
#define VK_F14       0x7D
#define VK_F15       0x7E
#define VK_F16       0x7F
#define VK_F17       0x80
#define VK_F18       0x81
#define VK_F19       0x82
#define VK_F20       0x83
#define VK_F21       0x84
#define VK_F22       0x85
#define VK_F23       0x86
#define VK_F24       0x87

#define VK_NUMLOCK   0x90
#define VK_SCROLL    0x91

#define VK_LSHIFT    0xA0
#define VK_RSHIFT    0xA1
#define VK_LCONTROL  0xA2
#define VK_RCONTROL  0xA3
#define VK_LMENU     0xA4
#define VK_RMENU     0xA5

#define VK_OEM_1     0xBA  // ;:
#define VK_OEM_PLUS  0xBB  // =+
#define VK_OEM_COMMA 0xBC  // ,<
#define VK_OEM_MINUS 0xBD  // -_
#define VK_OEM_PERIOD 0xBE // .>
#define VK_OEM_2     0xBF  // /?
#define VK_OEM_3     0xC0  // `~
#define VK_OEM_4     0xDB  // [{
#define VK_OEM_5     0xDC  // \|
#define VK_OEM_6     0xDD  // ]}
#define VK_OEM_7     0xDE  // '"
#define VK_OEM_8     0xDF
#define VK_OEM_102   0xE2  // <> or \| on non-US

// ============================================================
// Key state masks for mouse messages
// ============================================================
#define MK_LBUTTON  0x0001
#define MK_RBUTTON  0x0002
#define MK_SHIFT    0x0004
#define MK_CONTROL  0x0008
#define MK_MBUTTON  0x0010
#define MK_XBUTTON1 0x0020
#define MK_XBUTTON2 0x0040

// ============================================================
// Mouse wheel delta
// ============================================================
#define WHEEL_DELTA 120
#define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))
#define GET_KEYSTATE_WPARAM(wParam)    (LOWORD(wParam))
#define GET_XBUTTON_WPARAM(wParam)     (HIWORD(wParam))

// ============================================================
// Window Styles
// ============================================================
#define WS_OVERLAPPED    0x00000000L
#define WS_POPUP         0x80000000L
#define WS_CHILD         0x40000000L
#define WS_MINIMIZE      0x20000000L
#define WS_VISIBLE       0x10000000L
#define WS_DISABLED      0x08000000L
#define WS_CLIPSIBLINGS  0x04000000L
#define WS_CLIPCHILDREN  0x02000000L
#define WS_MAXIMIZE      0x01000000L
#define WS_CAPTION       0x00C00000L
#define WS_BORDER        0x00800000L
#define WS_DLGFRAME      0x00400000L
#define WS_VSCROLL       0x00200000L
#define WS_HSCROLL       0x00100000L
#define WS_SYSMENU       0x00080000L
#define WS_THICKFRAME    0x00040000L
#define WS_GROUP         0x00020000L
#define WS_TABSTOP       0x00010000L
#define WS_MINIMIZEBOX   0x00020000L
#define WS_MAXIMIZEBOX   0x00010000L
#define WS_TILED         WS_OVERLAPPED
#define WS_ICONIC        WS_MINIMIZE
#define WS_SIZEBOX       WS_THICKFRAME

#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW      (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW      WS_CHILD

// Extended window styles
#define WS_EX_DLGMODALFRAME  0x00000001L
#define WS_EX_NOPARENTNOTIFY 0x00000004L
#define WS_EX_TOPMOST        0x00000008L
#define WS_EX_ACCEPTFILES    0x00000010L
#define WS_EX_TRANSPARENT    0x00000020L
#define WS_EX_MDICHILD       0x00000040L
#define WS_EX_TOOLWINDOW     0x00000080L
#define WS_EX_WINDOWEDGE     0x00000100L
#define WS_EX_CLIENTEDGE     0x00000200L
#define WS_EX_CONTEXTHELP    0x00000400L
#define WS_EX_RIGHT          0x00001000L
#define WS_EX_LEFT           0x00000000L
#define WS_EX_RTLREADING     0x00002000L
#define WS_EX_LTRREADING     0x00000000L
#define WS_EX_LEFTSCROLLBAR  0x00004000L
#define WS_EX_RIGHTSCROLLBAR 0x00000000L
#define WS_EX_CONTROLPARENT  0x00010000L
#define WS_EX_STATICEDGE     0x00020000L
#define WS_EX_APPWINDOW      0x00040000L
#define WS_EX_LAYERED        0x00080000L
#define WS_EX_NOINHERITLAYOUT 0x00100000L
#define WS_EX_LAYOUTRTL      0x00400000L
#define WS_EX_COMPOSITED     0x02000000L
#define WS_EX_NOACTIVATE     0x08000000L

#define WS_EX_OVERLAPPEDWINDOW (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)
#define WS_EX_PALETTEWINDOW   (WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST)

// ============================================================
// ShowWindow commands
// ============================================================
#define SW_HIDE            0
#define SW_SHOWNORMAL      1
#define SW_NORMAL          1
#define SW_SHOWMINIMIZED   2
#define SW_SHOWMAXIMIZED   3
#define SW_MAXIMIZE        3
#define SW_SHOWNOACTIVATE  4
#define SW_SHOW            5
#define SW_MINIMIZE        6
#define SW_SHOWMINNOACTIVE 7
#define SW_SHOWNA          8
#define SW_RESTORE         9
#define SW_SHOWDEFAULT     10
#define SW_FORCEMINIMIZE   11

// ============================================================
// SetWindowPos flags
// ============================================================
#define SWP_NOSIZE         0x0001
#define SWP_NOMOVE         0x0002
#define SWP_NOZORDER       0x0004
#define SWP_NOREDRAW       0x0008
#define SWP_NOACTIVATE     0x0010
#define SWP_FRAMECHANGED   0x0020
#define SWP_SHOWWINDOW     0x0040
#define SWP_HIDEWINDOW     0x0080
#define SWP_NOCOPYBITS     0x0100
#define SWP_NOOWNERZORDER  0x0200
#define SWP_NOSENDCHANGING 0x0400
#define SWP_DRAWFRAME      SWP_FRAMECHANGED
#define SWP_NOREPOSITION   SWP_NOOWNERZORDER
#define SWP_DEFERERASE     0x2000
#define SWP_ASYNCWINDOWPOS 0x4000

#define HWND_TOP       ((HWND)0)
#define HWND_BOTTOM    ((HWND)1)
#define HWND_TOPMOST   ((HWND)-1)
#define HWND_NOTOPMOST ((HWND)-2)

// ============================================================
// Window class styles
// ============================================================
#define CS_VREDRAW         0x0001
#define CS_HREDRAW         0x0002
#define CS_DBLCLKS         0x0008
#define CS_OWNDC           0x0020
#define CS_CLASSDC         0x0040
#define CS_PARENTDC        0x0080
#define CS_NOCLOSE         0x0200
#define CS_SAVEBITS        0x0800
#define CS_BYTEALIGNCLIENT 0x1000
#define CS_BYTEALIGNWINDOW 0x2000
#define CS_GLOBALCLASS     0x4000
#define CS_DROPSHADOW      0x00020000

// ============================================================
// GetWindow / GW_* constants
// ============================================================
#define GW_HWNDFIRST  0
#define GW_HWNDLAST   1
#define GW_HWNDNEXT   2
#define GW_HWNDPREV   3
#define GW_OWNER      4
#define GW_CHILD      5
#define GW_ENABLEDPOPUP 6

// GetAncestor flags
#define GA_PARENT    1
#define GA_ROOT      2
#define GA_ROOTOWNER 3

// ============================================================
// GetWindowLong / SetWindowLong indices
// ============================================================
#define GWL_WNDPROC    (-4)
#define GWL_HINSTANCE  (-6)
#define GWL_HWNDPARENT (-8)
#define GWL_STYLE      (-16)
#define GWL_EXSTYLE    (-20)
#define GWL_USERDATA   (-21)
#define GWL_ID         (-12)

#define GWLP_WNDPROC    GWL_WNDPROC
#define GWLP_HINSTANCE  GWL_HINSTANCE
#define GWLP_HWNDPARENT GWL_HWNDPARENT
#define GWLP_USERDATA   GWL_USERDATA
#define GWLP_ID         GWL_ID

#define DWLP_DLGPROC    4
#define DWLP_MSGRESULT  0
#define DWLP_USER       8

// ============================================================
// Dialog templates
// ============================================================
#pragma pack(push, 2)
typedef struct {
	DWORD style;
	DWORD dwExtendedStyle;
	WORD  cdit;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATE, *LPDLGTEMPLATE;
typedef const DLGTEMPLATE* LPCDLGTEMPLATE;

// DLGTEMPLATEEX is defined in StaticDialog.h (N++ source)

typedef struct {
	DWORD style;
	DWORD dwExtendedStyle;
	short x;
	short y;
	short cx;
	short cy;
	WORD  id;
} DLGITEMTEMPLATE, *LPDLGITEMTEMPLATE;
#pragma pack(pop)

// ============================================================
// Dialog styles
// ============================================================
#define DS_ABSALIGN      0x01L
#define DS_SYSMODAL      0x02L
#define DS_LOCALEDIT     0x20L
#define DS_SETFONT       0x40L
#define DS_MODALFRAME    0x80L
#define DS_NOIDLEMSG     0x100L
#define DS_SETFOREGROUND 0x200L
#define DS_3DLOOK        0x0004L
#define DS_FIXEDSYS      0x0008L
#define DS_NOFAILCREATE  0x0010L
#define DS_CONTROL       0x0400L
#define DS_CENTER        0x0800L
#define DS_CENTERMOUSE   0x1000L
#define DS_CONTEXTHELP   0x2000L
#define DS_SHELLFONT     (DS_SETFONT | DS_FIXEDSYS)

// ============================================================
// Button styles
// ============================================================
#define BS_PUSHBUTTON      0x00000000L
#define BS_DEFPUSHBUTTON   0x00000001L
#define BS_CHECKBOX        0x00000002L
#define BS_AUTOCHECKBOX    0x00000003L
#define BS_RADIOBUTTON     0x00000004L
#define BS_3STATE          0x00000005L
#define BS_AUTO3STATE      0x00000006L
#define BS_GROUPBOX        0x00000007L
#define BS_USERBUTTON      0x00000008L
#define BS_AUTORADIOBUTTON 0x00000009L
#define BS_OWNERDRAW       0x0000000BL
#define BS_LEFTTEXT        0x00000020L
#define BS_TEXT            0x00000000L
#define BS_ICON            0x00000040L
#define BS_BITMAP          0x00000080L
#define BS_LEFT            0x00000100L
#define BS_RIGHT           0x00000200L
#define BS_CENTER          0x00000300L
#define BS_TOP             0x00000400L
#define BS_BOTTOM          0x00000800L
#define BS_VCENTER         0x00000C00L
#define BS_PUSHLIKE        0x00001000L
#define BS_MULTILINE       0x00002000L
#define BS_NOTIFY          0x00004000L
#define BS_FLAT            0x00008000L
#define BS_TYPEMASK        0x0000000FL
#define BS_SPLITBUTTON     0x0000000CL
#define BS_DEFSPLITBUTTON  0x0000000DL
#define BS_COMMANDLINK     0x0000000EL
#define BS_DEFCOMMANDLINK  0x0000000FL

// ============================================================
// Button messages
// ============================================================
#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
#define BM_GETSTATE 0x00F2
#define BM_SETSTATE 0x00F3
#define BM_SETSTYLE 0x00F4
#define BM_CLICK    0x00F5
#define BM_GETIMAGE 0x00F6
#define BM_SETIMAGE 0x00F7

#define BST_UNCHECKED     0x0000
#define BST_CHECKED       0x0001
#define BST_INDETERMINATE 0x0002
#define BST_PUSHED        0x0004
#define BST_FOCUS         0x0008
#define BST_HOT           0x0200

// Button notifications
#define BN_CLICKED 0
#define BN_PAINT   1

// ============================================================
// Edit control styles
// ============================================================
#define ES_LEFT        0x0000L
#define ES_CENTER      0x0001L
#define ES_RIGHT       0x0002L
#define ES_MULTILINE   0x0004L
#define ES_UPPERCASE   0x0008L
#define ES_LOWERCASE   0x0010L
#define ES_PASSWORD    0x0020L
#define ES_AUTOVSCROLL 0x0040L
#define ES_AUTOHSCROLL 0x0080L
#define ES_NOHIDESEL   0x0100L
#define ES_READONLY    0x0800L
#define ES_WANTRETURN  0x1000L
#define ES_NUMBER      0x2000L

// Edit control messages
#define EM_GETSEL              0x00B0
#define EM_SETSEL              0x00B1
#define EM_GETRECT             0x00B2
#define EM_SETRECT             0x00B3
#define EM_SETRECTNP           0x00B4
#define EM_SCROLL              0x00B5
#define EM_LINESCROLL          0x00B6
#define EM_GETMODIFY           0x00B8
#define EM_SETMODIFY           0x00B9
#define EM_GETLINECOUNT        0x00BA
#define EM_LINEINDEX           0x00BB
#define EM_SETHANDLE           0x00BC
#define EM_GETHANDLE           0x00BD
#define EM_GETTHUMB            0x00BE
#define EM_LINELENGTH          0x00C1
#define EM_REPLACESEL          0x00C2
#define EM_GETLINE             0x00C4
#define EM_LIMITTEXT           0x00C5
#define EM_CANUNDO             0x00C6
#define EM_UNDO                0x00C7
#define EM_FMTLINES            0x00C8
#define EM_LINEFROMCHAR        0x00C9
#define EM_SETTABSTOPS         0x00CB
#define EM_SETPASSWORDCHAR     0x00CC
#define EM_EMPTYUNDOBUFFER     0x00CD
#define EM_GETFIRSTVISIBLELINE 0x00CE
#define EM_SETREADONLY         0x00CF
#define EM_SETWORDBREAKPROC    0x00D0
#define EM_GETWORDBREAKPROC    0x00D1
#define EM_GETPASSWORDCHAR     0x00D2
#define EM_SETMARGINS          0x00D3
#define EM_GETMARGINS          0x00D4
#define EM_SETLIMITTEXT        EM_LIMITTEXT
#define EM_GETLIMITTEXT        0x00D5

// Edit control notifications
#define EN_SETFOCUS  0x0100
#define EN_KILLFOCUS 0x0200
#define EN_CHANGE    0x0300
#define EN_UPDATE    0x0400
#define EN_ERRSPACE  0x0500
#define EN_MAXTEXT   0x0501
#define EN_HSCROLL   0x0601
#define EN_VSCROLL   0x0602

// ============================================================
// Static control styles
// ============================================================
#define SS_LEFT           0x00000000L
#define SS_CENTER         0x00000001L
#define SS_RIGHT          0x00000002L
#define SS_ICON           0x00000003L
#define SS_BLACKRECT      0x00000004L
#define SS_GRAYRECT       0x00000005L
#define SS_WHITERECT      0x00000006L
#define SS_BLACKFRAME     0x00000007L
#define SS_GRAYFRAME      0x00000008L
#define SS_WHITEFRAME     0x00000009L
#define SS_SIMPLE         0x0000000BL
#define SS_LEFTNOWORDWRAP 0x0000000CL
#define SS_OWNERDRAW      0x0000000DL
#define SS_BITMAP         0x0000000EL
#define SS_ENHMETAFILE    0x0000000FL
#define SS_ETCHEDHORZ     0x00000010L
#define SS_ETCHEDVERT     0x00000011L
#define SS_ETCHEDFRAME    0x00000012L
#define SS_NOTIFY         0x00000100L
#define SS_CENTERIMAGE    0x00000200L
#define SS_SUNKEN         0x00001000L

#define STN_CLICKED 0
#define STN_DBLCLK  1

// ============================================================
// Listbox styles
// ============================================================
#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_NOREDRAW          0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL       0x0800L
#define LBS_DISABLENOSCROLL   0x1000L
#define LBS_NOSEL             0x4000L
#define LBS_COMBOBOX          0x8000L
#define LBS_STANDARD          (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

// Listbox messages
#define LB_ADDSTRING      0x0180
#define LB_INSERTSTRING   0x0181
#define LB_DELETESTRING   0x0182
#define LB_SELITEMRANGEEX 0x0183
#define LB_RESETCONTENT   0x0184
#define LB_SETSEL         0x0185
#define LB_SETCURSEL      0x0186
#define LB_GETSEL         0x0187
#define LB_GETCURSEL      0x0188
#define LB_GETTEXT        0x0189
#define LB_GETTEXTLEN     0x018A
#define LB_GETCOUNT        0x018B
#define LB_SELECTSTRING   0x018C
#define LB_GETITEMDATA    0x0199
#define LB_SETITEMDATA    0x019A
#define LB_GETSELCOUNT    0x0190
#define LB_GETSELITEMS    0x0191
#define LB_GETITEMRECT    0x0198
#define LB_SETITEMHEIGHT  0x01A0
#define LB_GETITEMHEIGHT  0x01A1
#define LB_FINDSTRING      0x018F
#define LB_FINDSTRINGEXACT 0x01A2
#define LB_GETTOPINDEX    0x018E
#define LB_SETTOPINDEX    0x0197
#define LB_ERR            (-1)

// Listbox notifications
#define LBN_ERRSPACE  (-2)
#define LBN_SELCHANGE 1
#define LBN_DBLCLK    2
#define LBN_SELCANCEL 3
#define LBN_SETFOCUS  4
#define LBN_KILLFOCUS 5

// ============================================================
// ComboBox styles
// ============================================================
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L

// ComboBox messages
#define CB_GETEDITSEL       0x0140
#define CB_LIMITTEXT        0x0141
#define CB_SETEDITSEL       0x0142
#define CB_ADDSTRING        0x0143
#define CB_DELETESTRING     0x0144
#define CB_DIR              0x0145
#define CB_GETCOUNT         0x0146
#define CB_GETCURSEL        0x0147
#define CB_GETLBTEXT        0x0148
#define CB_GETLBTEXTLEN     0x0149
#define CB_INSERTSTRING     0x014A
#define CB_RESETCONTENT     0x014B
#define CB_FINDSTRING       0x014C
#define CB_SELECTSTRING     0x014D
#define CB_SETCURSEL        0x014E
#define CB_SHOWDROPDOWN     0x014F
#define CB_GETITEMDATA      0x0150
#define CB_SETITEMDATA      0x0151
#define CB_GETDROPPEDCONTROLRECT 0x0152
#define CB_SETITEMHEIGHT    0x0153
#define CB_GETITEMHEIGHT    0x0154
#define CB_SETEXTENDEDUI    0x0155
#define CB_GETEXTENDEDUI    0x0156
#define CB_GETDROPPEDSTATE  0x0157
#define CB_FINDSTRINGEXACT  0x0158
#define CB_SETLOCALE        0x0159
#define CB_GETLOCALE        0x015A
#define CB_GETTOPINDEX      0x015B
#define CB_SETTOPINDEX      0x015C
#define CB_GETHORIZONTALEXTENT 0x015D
#define CB_SETHORIZONTALEXTENT 0x015E
#define CB_GETDROPPEDWIDTH  0x015F
#define CB_SETDROPPEDWIDTH  0x0160
#define CB_INITSTORAGE      0x0161
#define CB_ERR              (-1)
#define CB_ERRSPACE         (-2)

// ComboBox notifications
#define CBN_SELCHANGE  1
#define CBN_DBLCLK     2
#define CBN_SETFOCUS   3
#define CBN_KILLFOCUS  4
#define CBN_EDITCHANGE 5
#define CBN_EDITUPDATE 6
#define CBN_DROPDOWN   7
#define CBN_CLOSEUP    8
#define CBN_SELENDOK   9
#define CBN_SELENDCANCEL 10

// ============================================================
// Scrollbar
// ============================================================
#define SB_HORZ 0
#define SB_VERT 1
#define SB_CTL  2
#define SB_BOTH 3

#define SB_LINEUP        0
#define SB_LINELEFT      0
#define SB_LINEDOWN      1
#define SB_LINERIGHT     1
#define SB_PAGEUP        2
#define SB_PAGELEFT      2
#define SB_PAGEDOWN      3
#define SB_PAGERIGHT     3
#define SB_THUMBPOSITION 4
#define SB_THUMBTRACK    5
#define SB_TOP           6
#define SB_LEFT          6
#define SB_BOTTOM        7
#define SB_RIGHT         7
#define SB_ENDSCROLL     8

#define SIF_RANGE           0x0001
#define SIF_PAGE            0x0002
#define SIF_POS             0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS        0x0010
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

typedef struct tagSCROLLINFO {
	UINT cbSize;
	UINT fMask;
	int  nMin;
	int  nMax;
	UINT nPage;
	int  nPos;
	int  nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;
typedef const SCROLLINFO* LPCSCROLLINFO;

// ============================================================
// MessageBox
// ============================================================
#define MB_OK                0x00000000L
#define MB_OKCANCEL          0x00000001L
#define MB_ABORTRETRYIGNORE  0x00000002L
#define MB_YESNOCANCEL       0x00000003L
#define MB_YESNO             0x00000004L
#define MB_RETRYCANCEL       0x00000005L
#define MB_CANCELTRYCONTINUE 0x00000006L

#define MB_ICONHAND          0x00000010L
#define MB_ICONQUESTION      0x00000020L
#define MB_ICONEXCLAMATION   0x00000030L
#define MB_ICONASTERISK      0x00000040L
#define MB_ICONWARNING       MB_ICONEXCLAMATION
#define MB_ICONERROR         MB_ICONHAND
#define MB_ICONINFORMATION   MB_ICONASTERISK
#define MB_ICONSTOP          MB_ICONHAND

#define MB_DEFBUTTON1        0x00000000L
#define MB_DEFBUTTON2        0x00000100L
#define MB_DEFBUTTON3        0x00000200L
#define MB_DEFBUTTON4        0x00000300L

#define MB_APPLMODAL         0x00000000L
#define MB_SYSTEMMODAL       0x00001000L
#define MB_TASKMODAL         0x00002000L
#define MB_SETFOREGROUND     0x00010000L
#define MB_TOPMOST           0x00040000L
#define MB_RIGHT             0x00080000L
#define MB_RTLREADING        0x00100000L

#define IDOK       1
#define IDCANCEL   2
#define IDABORT    3
#define IDRETRY    4
#define IDIGNORE   5
#define IDYES      6
#define IDNO       7
#define IDCLOSE    8
#define IDHELP     9
#define IDTRYAGAIN 10
#define IDCONTINUE 11

// ============================================================
// Cursor IDs
// ============================================================
#define IDC_ARROW       ((LPCWSTR)32512)
#define IDC_IBEAM       ((LPCWSTR)32513)
#define IDC_WAIT        ((LPCWSTR)32514)
#define IDC_CROSS       ((LPCWSTR)32515)
#define IDC_UPARROW     ((LPCWSTR)32516)
#define IDC_SIZE        ((LPCWSTR)32640)
#define IDC_ICON        ((LPCWSTR)32641)
#define IDC_SIZENWSE    ((LPCWSTR)32642)
#define IDC_SIZENESW    ((LPCWSTR)32643)
#define IDC_SIZEWE      ((LPCWSTR)32644)
#define IDC_SIZENS      ((LPCWSTR)32645)
#define IDC_SIZEALL     ((LPCWSTR)32646)
#define IDC_NO          ((LPCWSTR)32648)
#define IDC_HAND        ((LPCWSTR)32649)

// OEM cursor resource IDs
#define OCR_NORMAL   32512
#define OCR_HAND     32649
#define IDC_APPSTARTING ((LPCWSTR)32650)
#define IDC_HELP_CURSOR ((LPCWSTR)32651)

// Icon IDs
#define IDI_APPLICATION ((LPCWSTR)32512)
#define IDI_HAND        ((LPCWSTR)32513)
#define IDI_QUESTION    ((LPCWSTR)32514)
#define IDI_EXCLAMATION ((LPCWSTR)32515)
#define IDI_ASTERISK    ((LPCWSTR)32516)
#define IDI_WINLOGO     ((LPCWSTR)32517)
#define IDI_WARNING     IDI_EXCLAMATION
#define IDI_ERROR       IDI_HAND
#define IDI_INFORMATION IDI_ASTERISK

// ============================================================
// Image types
// ============================================================
#define IMAGE_BITMAP 0
#define IMAGE_ICON   1
#define IMAGE_CURSOR 2

#define LR_DEFAULTCOLOR     0x00000000
#define LR_MONOCHROME       0x00000001
#define LR_COLOR            0x00000002
#define LR_COPYRETURNORG    0x00000004
#define LR_COPYDELETEORG    0x00000008
#define LR_LOADFROMFILE     0x00000010
#define LR_LOADTRANSPARENT  0x00000020
#define LR_DEFAULTSIZE      0x00000040
#define LR_LOADMAP3DCOLORS  0x00001000
#define LR_CREATEDIBSECTION 0x00002000
#define LR_SHARED           0x00008000

// ============================================================
// Clipboard formats
// ============================================================
#define CF_TEXT         1
#define CF_BITMAP       2
#define CF_METAFILEPICT 3
#define CF_SYLK         4
#define CF_DIF          5
#define CF_TIFF         6
#define CF_OEMTEXT      7
#define CF_DIB          8
#define CF_PALETTE      9
#define CF_PENDATA      10
#define CF_RIFF         11
#define CF_WAVE         12
#define CF_UNICODETEXT  13
#define CF_ENHMETAFILE  14
#define CF_HDROP        15
#define CF_LOCALE       16

// Clipboard chain messages
#define WM_CHANGECBCHAIN   0x030D
#define WM_DRAWCLIPBOARD   0x0308

// Clipboard functions
HWND SetClipboardViewer(HWND hWndNewViewer);
BOOL ChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext);
BOOL OpenClipboard(HWND hWndNewOwner);
BOOL CloseClipboard();
BOOL EmptyClipboard();
HANDLE SetClipboardData(UINT uFormat, HANDLE hMem);
HANDLE GetClipboardData(UINT uFormat);
BOOL IsClipboardFormatAvailable(UINT format);
int CountClipboardFormats();
UINT EnumClipboardFormats(UINT format);
HWND GetClipboardOwner();

// ============================================================
// Menu flags
// ============================================================
#define MF_INSERT          0x00000000L
#define MF_CHANGE          0x00000080L
#define MF_APPEND          0x00000100L
#define MF_DELETE          0x00000200L
#define MF_REMOVE          0x00001000L
#define MF_BYCOMMAND       0x00000000L
#define MF_BYPOSITION      0x00000400L
#define MF_SEPARATOR       0x00000800L
#define MF_ENABLED         0x00000000L
#define MF_GRAYED          0x00000001L
#define MF_DISABLED        0x00000002L
#define MF_UNCHECKED       0x00000000L
#define MF_CHECKED         0x00000008L
#define MF_USECHECKBITMAPS 0x00000200L
#define MF_STRING          0x00000000L
#define MF_BITMAP          0x00000004L
#define MF_OWNERDRAW       0x00000100L
#define MF_POPUP           0x00000010L
#define MF_MENUBARBREAK    0x00000020L
#define MF_MENUBREAK       0x00000040L
#define MF_HILITE          0x00000080L
#define MF_DEFAULT         0x00001000L
#define MF_SYSMENU         0x00002000L
#define MF_HELP            0x00004000L
#define MF_RIGHTJUSTIFY    0x00004000L

#define MFT_STRING       MF_STRING
#define MFT_BITMAP       MF_BITMAP
#define MFT_MENUBARBREAK MF_MENUBARBREAK
#define MFT_MENUBREAK    MF_MENUBREAK
#define MFT_OWNERDRAW    MF_OWNERDRAW
#define MFT_RADIOCHECK   0x00000200L
#define MFT_SEPARATOR    MF_SEPARATOR
#define MFT_RIGHTORDER   0x00002000L
#define MFT_RIGHTJUSTIFY MF_RIGHTJUSTIFY

#define MFS_GRAYED    0x00000003L
#define MFS_DISABLED  MFS_GRAYED
#define MFS_CHECKED   MF_CHECKED
#define MFS_HILITE    MF_HILITE
#define MFS_ENABLED   MF_ENABLED
#define MFS_UNCHECKED MF_UNCHECKED
#define MFS_UNHILITE  0x00000000L
#define MFS_DEFAULT   MF_DEFAULT

#define MIIM_STATE      0x00000001
#define MIIM_ID         0x00000002
#define MIIM_SUBMENU    0x00000004
#define MIIM_CHECKMARKS 0x00000008
#define MIIM_TYPE       0x00000010
#define MIIM_DATA       0x00000020
#define MIIM_STRING     0x00000040
#define MIIM_BITMAP     0x00000080
#define MIIM_FTYPE      0x00000100

#define TPM_LEFTBUTTON   0x0000L
#define TPM_RIGHTBUTTON  0x0002L
#define TPM_LEFTALIGN    0x0000L
#define TPM_CENTERALIGN  0x0004L
#define TPM_RIGHTALIGN   0x0008L
#define TPM_TOPALIGN     0x0000L
#define TPM_VCENTERALIGN 0x0010L
#define TPM_BOTTOMALIGN  0x0020L
#define TPM_RETURNCMD    0x0100L
#define TPM_NONOTIFY     0x0080L
#define TPM_HORIZONTAL   0x0000L
#define TPM_VERTICAL     0x0040L
#define TPM_LAYOUTRTL    0x8000L

typedef struct tagTPMPARAMS {
	UINT cbSize;
	RECT rcExclude;
} TPMPARAMS, *LPTPMPARAMS;

BOOL TrackPopupMenuEx(HMENU hMenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);

// ============================================================
// Timer
// ============================================================
// SetTimer/KillTimer are declared in winbase.h (function declarations)

// ============================================================
// Redraw / InvalidateRect
// ============================================================
#define RDW_INVALIDATE    0x0001
#define RDW_INTERNALPAINT 0x0002
#define RDW_ERASE         0x0004
#define RDW_VALIDATE      0x0008
#define RDW_NOINTERNALPAINT 0x0010
#define RDW_NOERASE       0x0020
#define RDW_NOCHILDREN    0x0040
#define RDW_ALLCHILDREN   0x0080
#define RDW_UPDATENOW     0x0100
#define RDW_ERASENOW      0x0200
#define RDW_FRAME         0x0400
#define RDW_NOFRAME       0x0800

// ============================================================
// Hit test values
// ============================================================
#define HTERROR       (-2)
#define HTTRANSPARENT (-1)
#define HTNOWHERE     0
#define HTCLIENT      1
#define HTCAPTION     2
#define HTSYSMENU     3
#define HTGROWBOX     4
#define HTSIZE        HTGROWBOX
#define HTMENU        5
#define HTHSCROLL     6
#define HTVSCROLL     7
#define HTMINBUTTON   8
#define HTMAXBUTTON   9
#define HTLEFT        10
#define HTRIGHT       11
#define HTTOP         12
#define HTTOPLEFT     13
#define HTTOPRIGHT    14
#define HTBOTTOM      15
#define HTBOTTOMLEFT  16
#define HTBOTTOMRIGHT 17
#define HTBORDER      18
#define HTCLOSE       20
#define HTHELP        21

// ============================================================
// System colors
// ============================================================
#define COLOR_SCROLLBAR         0
#define COLOR_BACKGROUND        1
#define COLOR_ACTIVECAPTION     2
#define COLOR_INACTIVECAPTION   3
#define COLOR_MENU              4
#define COLOR_WINDOW            5
#define COLOR_WINDOWFRAME       6
#define COLOR_MENUTEXT          7
#define COLOR_WINDOWTEXT        8
#define COLOR_CAPTIONTEXT       9
#define COLOR_ACTIVEBORDER      10
#define COLOR_INACTIVEBORDER    11
#define COLOR_APPWORKSPACE      12
#define COLOR_HIGHLIGHT         13
#define COLOR_HIGHLIGHTTEXT     14
#define COLOR_BTNFACE           15
#define COLOR_BTNSHADOW         16
#define COLOR_GRAYTEXT          17
#define COLOR_BTNTEXT           18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT      20
#define COLOR_3DDKSHADOW        21
#define COLOR_3DLIGHT           22
#define COLOR_INFOTEXT          23
#define COLOR_INFOBK            24
#define COLOR_HOTLIGHT          26
#define COLOR_GRADIENTACTIVECAPTION 27
#define COLOR_GRADIENTINACTIVECAPTION 28
#define COLOR_MENUBAR           30
#define COLOR_MENUHILIGHT       29
#define COLOR_DESKTOP           COLOR_BACKGROUND

// ============================================================
// System metrics
// ============================================================
#define SM_CXSCREEN        0
#define SM_CYSCREEN        1
#define SM_CXVSCROLL       2
#define SM_CYHSCROLL       3
#define SM_CYCAPTION       4
#define SM_CXBORDER        5
#define SM_CYBORDER        6
#define SM_CXDLGFRAME      7
#define SM_CYDLGFRAME      8
#define SM_CXFIXEDFRAME    SM_CXDLGFRAME
#define SM_CYFIXEDFRAME    SM_CYDLGFRAME
#define SM_CYVTHUMB        9
#define SM_CXHTHUMB        10
#define SM_CXICON          11
#define SM_CYICON          12
#define SM_CXCURSOR        13
#define SM_CYCURSOR        14
#define SM_CYMENU          15
#define SM_CXFULLSCREEN    16
#define SM_CYFULLSCREEN    17
#define SM_CYKANJIWINDOW   18
#define SM_MOUSEPRESENT    19
#define SM_CYVSCROLL       20
#define SM_CXHSCROLL       21
#define SM_DEBUG           22
#define SM_SWAPBUTTON      23
#define SM_CXMIN           28
#define SM_CYMIN           29
#define SM_CXSIZE          30
#define SM_CYSIZE          31
#define SM_CXFRAME         32
#define SM_CYFRAME         33
#define SM_CXSIZEFRAME     SM_CXFRAME
#define SM_CYSIZEFRAME     SM_CYFRAME
#define SM_CXMINTRACK      34
#define SM_CYMINTRACK      35
#define SM_CXDOUBLECLK     36
#define SM_CYDOUBLECLK     37
#define SM_CXICONSPACING   38
#define SM_CYICONSPACING   39
#define SM_MENUDROPALIGNMENT 40
#define SM_PENWINDOWS      41
#define SM_DBCSENABLED     42
#define SM_CMOUSEBUTTONS   43
#define SM_CXEDGE          45
#define SM_CYEDGE          46
#define SM_CXMINSPACING    47
#define SM_CYMINSPACING    48
#define SM_CXSMICON        49
#define SM_CYSMICON        50
#define SM_CYSMCAPTION     51
#define SM_CXSMSIZE        52
#define SM_CYSMSIZE        53
#define SM_CXMENUSIZE      54
#define SM_CYMENUSIZE      55
#define SM_ARRANGE         56
#define SM_CXMINIMIZED     57
#define SM_CYMINIMIZED     58
#define SM_CXMAXTRACK      59
#define SM_CYMAXTRACK      60
#define SM_CXMAXIMIZED     61
#define SM_CYMAXIMIZED     62
#define SM_NETWORK         63
#define SM_XVIRTUALSCREEN  76
#define SM_YVIRTUALSCREEN  77
#define SM_CXVIRTUALSCREEN 78
#define SM_CYVIRTUALSCREEN 79
#define SM_CMONITORS       80
#define SM_SAMEDISPLAYFORMAT 81
#define SM_CXPADDEDBORDER    92

// ============================================================
// Structures
// ============================================================

// MSG
typedef struct tagMSG {
	HWND   hwnd;
	UINT   message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD  time;
	POINT  pt;
} MSG, *PMSG, *LPMSG;

// WNDCLASSEX
typedef struct tagWNDCLASSEXW {
	UINT      cbSize;
	UINT      style;
	WNDPROC   lpfnWndProc;
	int       cbClsExtra;
	int       cbWndExtra;
	HINSTANCE hInstance;
	HICON     hIcon;
	HCURSOR   hCursor;
	HBRUSH    hbrBackground;
	LPCWSTR   lpszMenuName;
	LPCWSTR   lpszClassName;
	HICON     hIconSm;
} WNDCLASSEXW, *PWNDCLASSEXW, *LPWNDCLASSEXW;
typedef WNDCLASSEXW WNDCLASSEX;
typedef LPWNDCLASSEXW LPWNDCLASSEX;

// WNDCLASS
typedef struct tagWNDCLASSW {
	UINT      style;
	WNDPROC   lpfnWndProc;
	int       cbClsExtra;
	int       cbWndExtra;
	HINSTANCE hInstance;
	HICON     hIcon;
	HCURSOR   hCursor;
	HBRUSH    hbrBackground;
	LPCWSTR   lpszMenuName;
	LPCWSTR   lpszClassName;
} WNDCLASSW, *PWNDCLASSW, *LPWNDCLASSW;
typedef WNDCLASSW WNDCLASS;

// CREATESTRUCT
typedef struct tagCREATESTRUCTW {
	LPVOID    lpCreateParams;
	HINSTANCE hInstance;
	HMENU     hMenu;
	HWND      hwndParent;
	int       cy;
	int       cx;
	int       y;
	int       x;
	LONG      style;
	LPCWSTR   lpszName;
	LPCWSTR   lpszClass;
	DWORD     dwExStyle;
} CREATESTRUCTW, *LPCREATESTRUCTW;
typedef CREATESTRUCTW CREATESTRUCT;
typedef LPCREATESTRUCTW LPCREATESTRUCT;

// WINDOWPOS
typedef struct tagWINDOWPOS {
	HWND hwnd;
	HWND hwndInsertAfter;
	int  x;
	int  y;
	int  cx;
	int  cy;
	UINT flags;
} WINDOWPOS, *LPWINDOWPOS, *PWINDOWPOS;

// NMHDR (notification header)
typedef struct tagNMHDR {
	HWND     hwndFrom;
	UINT_PTR idFrom;
	UINT     code;
} NMHDR, *LPNMHDR;

// MINMAXINFO
typedef struct tagMINMAXINFO {
	POINT ptReserved;
	POINT ptMaxSize;
	POINT ptMaxPosition;
	POINT ptMinTrackSize;
	POINT ptMaxTrackSize;
} MINMAXINFO, *PMINMAXINFO, *LPMINMAXINFO;

// PAINTSTRUCT
typedef struct tagPAINTSTRUCT {
	HDC  hdc;
	BOOL fErase;
	RECT rcPaint;
	BOOL fRestore;
	BOOL fIncUpdate;
	BYTE rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *LPPAINTSTRUCT;

// DRAWITEMSTRUCT
typedef struct tagDRAWITEMSTRUCT {
	UINT      CtlType;
	UINT      CtlID;
	UINT      itemID;
	UINT      itemAction;
	UINT      itemState;
	HWND      hwndItem;
	HDC       hDC;
	RECT      rcItem;
	ULONG_PTR itemData;
} DRAWITEMSTRUCT, *LPDRAWITEMSTRUCT;

// MEASUREITEMSTRUCT
typedef struct tagMEASUREITEMSTRUCT {
	UINT      CtlType;
	UINT      CtlID;
	UINT      itemID;
	UINT      itemWidth;
	UINT      itemHeight;
	ULONG_PTR itemData;
} MEASUREITEMSTRUCT, *LPMEASUREITEMSTRUCT;

// MENUITEMINFO
typedef struct tagMENUITEMINFOW {
	UINT    cbSize;
	UINT    fMask;
	UINT    fType;
	UINT    fState;
	UINT    wID;
	HMENU   hSubMenu;
	HBITMAP hbmpChecked;
	HBITMAP hbmpUnchecked;
	ULONG_PTR dwItemData;
	LPWSTR  dwTypeData;
	UINT    cch;
	HBITMAP hbmpItem;
} MENUITEMINFOW, *LPMENUITEMINFOW;
typedef MENUITEMINFOW MENUITEMINFO;
typedef LPMENUITEMINFOW LPMENUITEMINFO;
typedef const MENUITEMINFOW* LPCMENUITEMINFOW;
typedef LPCMENUITEMINFOW LPCMENUITEMINFO;

// ACCEL
typedef struct tagACCEL {
	BYTE fVirt;
	WORD key;
	WORD cmd;
} ACCEL, *LPACCEL;

#define FVIRTKEY  TRUE
#define FNOINVERT 0x02
#undef FSHIFT
#define FSHIFT    0x04
#define FCONTROL  0x08
#define FALT      0x10

// TRACKMOUSEEVENT
typedef struct tagTRACKMOUSEEVENT {
	DWORD cbSize;
	DWORD dwFlags;
	HWND  hwndTrack;
	DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

#define TME_HOVER  0x00000001
#define TME_LEAVE  0x00000002
#define TME_NONCLIENT 0x00000010
#define TME_QUERY  0x40000000
#define TME_CANCEL 0x80000000

// mouse_event flags
#define MOUSEEVENTF_MOVE       0x0001
#define MOUSEEVENTF_LEFTDOWN   0x0002
#define MOUSEEVENTF_LEFTUP     0x0004
#define MOUSEEVENTF_RIGHTDOWN  0x0008
#define MOUSEEVENTF_RIGHTUP    0x0010
#define MOUSEEVENTF_MIDDLEDOWN 0x0020
#define MOUSEEVENTF_MIDDLEUP   0x0040
#define MOUSEEVENTF_WHEEL      0x0800
#define MOUSEEVENTF_ABSOLUTE   0x8000

inline void mouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo) {}

// NCCALCSIZE_PARAMS
typedef struct tagNCCALCSIZE_PARAMS {
	RECT       rgrc[3];
	WINDOWPOS* lppos;
} NCCALCSIZE_PARAMS, *LPNCCALCSIZE_PARAMS;

// ============================================================
// CW_USEDEFAULT
// ============================================================
#define CW_USEDEFAULT ((int)0x80000000)

// ============================================================
// GWL helpers
// ============================================================
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

// ============================================================
// WM_COMMAND helpers
// ============================================================
#define GET_WM_COMMAND_ID(wParam, lParam)   LOWORD(wParam)
#define GET_WM_COMMAND_HWND(wParam, lParam) ((HWND)(lParam))
#define GET_WM_COMMAND_CMD(wParam, lParam)  HIWORD(wParam)

// ============================================================
// Notification codes
// ============================================================
#define NM_FIRST       0U
#define NM_OUTOFMEMORY (NM_FIRST - 1)
#define NM_CLICK       (NM_FIRST - 2)
#define NM_DBLCLK      (NM_FIRST - 3)
#define NM_RETURN      (NM_FIRST - 4)
#define NM_RCLICK      (NM_FIRST - 5)
#define NM_RDBLCLK     (NM_FIRST - 6)
#define NM_SETFOCUS_NM (NM_FIRST - 7)
#define NM_KILLFOCUS_NM (NM_FIRST - 8)
#define NM_CUSTOMDRAW  (NM_FIRST - 12)
#define NM_HOVER       (NM_FIRST - 13)
#define NM_TOOLTIPSCREATED (NM_FIRST - 19)

// ============================================================
// Owner draw types
// ============================================================
#define ODT_MENU     1
#define ODT_LISTBOX  2
#define ODT_COMBOBOX 3
#define ODT_BUTTON   4
#define ODT_STATIC   5
#define ODT_TAB      101

#define ODA_DRAWENTIRE 0x0001
#define ODA_SELECT     0x0002
#define ODA_FOCUS      0x0004

#define ODS_SELECTED   0x0001
#define ODS_GRAYED     0x0002
#define ODS_DISABLED   0x0004
#define ODS_CHECKED    0x0008
#define ODS_FOCUS      0x0010
#define ODS_DEFAULT    0x0020
#define ODS_HOTLIGHT      0x0040
#define ODS_INACTIVE      0x0080
#define ODS_NOACCEL       0x0100
#define ODS_NOFOCUSRECT   0x0200

// ============================================================
// DT_ DrawText flags
// ============================================================
#define DT_TOP             0x00000000
#define DT_LEFT            0x00000000
#define DT_CENTER          0x00000001
#define DT_RIGHT           0x00000002
#define DT_VCENTER         0x00000004
#define DT_BOTTOM          0x00000008
#define DT_WORDBREAK       0x00000010
#define DT_SINGLELINE      0x00000020
#define DT_EXPANDTABS      0x00000040
#define DT_TABSTOP         0x00000080
#define DT_NOCLIP          0x00000100
#define DT_EXTERNALLEADING 0x00000200
#define DT_CALCRECT        0x00000400
#define DT_NOPREFIX        0x00000800
#define DT_INTERNAL        0x00001000
#define DT_EDITCONTROL     0x00002000
#define DT_PATH_ELLIPSIS   0x00004000
#define DT_END_ELLIPSIS    0x00008000
#define DT_MODIFYSTRING    0x00010000
#define DT_RTLREADING      0x00020000
#define DT_WORD_ELLIPSIS   0x00040000
#define DT_HIDEPREFIX      0x00100000
#define DT_PREFIXONLY      0x00200000

// ============================================================
// Predefined window class names
// ============================================================
#define WC_DIALOG L"#32770"

// ============================================================
// DPI Awareness (stubs)
// ============================================================
#define USER_DEFAULT_SCREEN_DPI 96
#define DPI_AWARENESS_CONTEXT_UNAWARE          ((HANDLE)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE     ((HANDLE)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ((HANDLE)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED ((HANDLE)-5)
typedef HANDLE DPI_AWARENESS_CONTEXT;

// ============================================================
// Monitor info
// ============================================================
#define MONITOR_DEFAULTTONULL    0x00000000
#define MONITOR_DEFAULTTOPRIMARY 0x00000001
#define MONITOR_DEFAULTTONEAREST 0x00000002

typedef struct tagMONITORINFO {
	DWORD cbSize;
	RECT  rcMonitor;
	RECT  rcWork;
	DWORD dwFlags;
} MONITORINFO, *LPMONITORINFO;

#define MONITORINFOF_PRIMARY 0x00000001

// ============================================================
// WM_PRINT / WM_PRINTCLIENT
// ============================================================
#define PRF_CHECKVISIBLE 0x00000001L
#define PRF_NONCLIENT    0x00000002L
#define PRF_CLIENT       0x00000004L
#define PRF_ERASEBKGND   0x00000008L
#define PRF_CHILDREN     0x00000010L
#define PRF_OWNED        0x00000020L

// ============================================================
// Misc constants
// ============================================================
#define GCL_HCURSOR (-12)
#define GCLP_HCURSOR GCL_HCURSOR

#define DLGC_WANTARROWS   0x0001
#define DLGC_WANTTAB      0x0002
#define DLGC_WANTALLKEYS  0x0004
#define DLGC_WANTMESSAGE  0x0004
#define DLGC_HASSETSEL    0x0008
#define DLGC_DEFPUSHBUTTON 0x0010
#define DLGC_UNDEFPUSHBUTTON 0x0020
#define DLGC_RADIOBUTTON  0x0040
#define DLGC_WANTCHARS    0x0080
#define DLGC_STATIC       0x0100
#define DLGC_BUTTON       0x2000

// Dialog manager messages
#define DM_GETDEFID       (WM_USER + 0)
#define DM_SETDEFID       (WM_USER + 1)
#define DM_REPOSITION     (WM_USER + 2)

// Window hooks
typedef HANDLE HHOOK;
#define WH_KEYBOARD       2
#define WH_GETMESSAGE     3
#define WH_CALLWNDPROC    4
#define WH_CBT            5
#define WH_MOUSE          7
#define WH_KEYBOARD_LL    13
#define WH_MOUSE_LL       14

typedef LRESULT (CALLBACK* HOOKPROC)(int nCode, WPARAM wParam, LPARAM lParam);

// Low-level hook structs
typedef struct tagMSLLHOOKSTRUCT {
	POINT     pt;
	DWORD     mouseData;
	DWORD     flags;
	DWORD     time;
	ULONG_PTR dwExtraInfo;
} MSLLHOOKSTRUCT, *LPMSLLHOOKSTRUCT, *PMSLLHOOKSTRUCT;

typedef struct tagKBDLLHOOKSTRUCT {
	DWORD     vkCode;
	DWORD     scanCode;
	DWORD     flags;
	DWORD     time;
	ULONG_PTR dwExtraInfo;
} KBDLLHOOKSTRUCT, *LPKBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;

#define HC_ACTION      0
#define HC_GETNEXT     1
#define HC_SKIP        2
#define HC_NOREMOVE    3

// CWPSTRUCT for WH_CALLWNDPROC hook
typedef struct tagCWPSTRUCT {
	LPARAM lParam;
	WPARAM wParam;
	UINT   message;
	HWND   hwnd;
} CWPSTRUCT, *PCWPSTRUCT;

HHOOK SetWindowsHookExW(int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId);
#define SetWindowsHookEx SetWindowsHookExW
BOOL UnhookWindowsHookEx(HHOOK hhk);
LRESULT CallNextHookEx(HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam);

// Window placement
typedef struct tagWINDOWPLACEMENT {
	UINT  length;
	UINT  flags;
	UINT  showCmd;
	POINT ptMinPosition;
	POINT ptMaxPosition;
	RECT  rcNormalPosition;
} WINDOWPLACEMENT;

#define WPF_SETMINPOSITION      0x0001
#define WPF_RESTORETOMAXIMIZED  0x0002
#define WPF_ASYNCWINDOWPLACEMENT 0x0004

BOOL GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT* lpwndpl);
BOOL SetWindowPlacement(HWND hWnd, const WINDOWPLACEMENT* lpwndpl);

// LoadImage flags
#define LR_DEFAULTCOLOR  0x00000000
#define LR_MONOCHROME    0x00000001
#define LR_LOADFROMFILE  0x00000010
#define LR_LOADTRANSPARENT 0x00000020
#define LR_DEFAULTSIZE   0x00000040
#define LR_CREATEDIBSECTION 0x00002000
#define LR_SHARED        0x00008000

// LoadImage types
#define IMAGE_BITMAP  0
#define IMAGE_ICON    1
#define IMAGE_CURSOR  2

// ============================================================
// IME Reconversion
// ============================================================
#define IMR_RECONVERTSTRING       0x0004
#define IMR_CONFIRMRECONVERTSTRING 0x0005
#define IMR_QUERYCHARPOSITION      0x0006
#define IMR_DOCUMENTFEED           0x0007

typedef struct tagRECONVERTSTRING {
	DWORD dwSize;
	DWORD dwVersion;
	DWORD dwStrLen;
	DWORD dwStrOffset;
	DWORD dwCompStrLen;
	DWORD dwCompStrOffset;
	DWORD dwTargetStrLen;
	DWORD dwTargetStrOffset;
} RECONVERTSTRING, *PRECONVERTSTRING, *LPRECONVERTSTRING;

// ============================================================
// Character classification
// ============================================================
inline BOOL IsCharAlphaW(WCHAR ch) { return iswalpha(ch) ? TRUE : FALSE; }
inline BOOL IsCharAlphaNumericW(WCHAR ch) { return iswalnum(ch) ? TRUE : FALSE; }
inline BOOL IsCharUpperW(WCHAR ch) { return iswupper(ch) ? TRUE : FALSE; }
inline BOOL IsCharLowerW(WCHAR ch) { return iswlower(ch) ? TRUE : FALSE; }
#define IsCharAlpha IsCharAlphaW
#define IsCharAlphaNumeric IsCharAlphaNumericW
#define IsCharUpper IsCharUpperW
#define IsCharLower IsCharLowerW

// ============================================================
// GetClassName
// ============================================================
inline int GetClassNameA(HWND hWnd, LPSTR lpClassName, int nMaxCount)
{
	(void)hWnd;
	if (lpClassName && nMaxCount > 0) lpClassName[0] = '\0';
	return 0;
}
inline int GetClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount)
{
	(void)hWnd;
	if (lpClassName && nMaxCount > 0) lpClassName[0] = L'\0';
	return 0;
}
#define GetClassName GetClassNameW

// ============================================================
// Window state queries
// ============================================================
inline BOOL IsZoomed(HWND hWnd) { (void)hWnd; return FALSE; }
inline BOOL IsIconic(HWND hWnd) { (void)hWnd; return FALSE; }
BOOL IsWindowVisible(HWND hWnd);

// ============================================================
// HWND_DESKTOP and child window queries
// ============================================================
#define HWND_DESKTOP ((HWND)0)

#define CWP_ALL             0x0000
#define CWP_SKIPINVISIBLE   0x0001
#define CWP_SKIPDISABLED    0x0002
#define CWP_SKIPTRANSPARENT 0x0004

inline HWND ChildWindowFromPointEx(HWND hWndParent, POINT pt, UINT uFlags)
{
	(void)hWndParent; (void)pt; (void)uFlags;
	return hWndParent;
}

int MapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints);

// ============================================================
// Menu bitmaps
// ============================================================
inline BOOL SetMenuItemBitmaps(HMENU hMenu, UINT uPosition, UINT uFlags, HBITMAP hBitmapUnchecked, HBITMAP hBitmapChecked)
{
	(void)hMenu; (void)uPosition; (void)uFlags; (void)hBitmapUnchecked; (void)hBitmapChecked;
	return TRUE;
}

// ============================================================
// Sound
// ============================================================
inline BOOL MessageBeep(UINT uType) { (void)uType; return TRUE; }

// ============================================================
// ScrollWindow
// ============================================================
inline BOOL ScrollWindow(HWND hWnd, int XAmount, int YAmount, const RECT* lpRect, const RECT* lpClipRect)
{
	(void)hWnd; (void)XAmount; (void)YAmount; (void)lpRect; (void)lpClipRect;
	return TRUE;
}

// ============================================================
// Window properties
// ============================================================
inline HANDLE GetPropW(HWND hWnd, LPCWSTR lpString) { (void)hWnd; (void)lpString; return nullptr; }
inline BOOL SetPropW(HWND hWnd, LPCWSTR lpString, HANDLE hData) { (void)hWnd; (void)lpString; (void)hData; return TRUE; }
inline HANDLE RemovePropW(HWND hWnd, LPCWSTR lpString) { (void)hWnd; (void)lpString; return nullptr; }
#define GetProp GetPropW
#define SetProp SetPropW
#define RemoveProp RemovePropW

// ============================================================
// FlashWindow
// ============================================================
#define FLASHW_STOP      0x00000000
#define FLASHW_CAPTION   0x00000001
#define FLASHW_TRAY      0x00000002
#define FLASHW_ALL       (FLASHW_CAPTION | FLASHW_TRAY)
#define FLASHW_TIMER     0x00000004
#define FLASHW_TIMERNOFG 0x0000000C

typedef struct {
	UINT  cbSize;
	HWND  hwnd;
	DWORD dwFlags;
	UINT  uCount;
	DWORD dwTimeout;
} FLASHWINFO, *PFLASHWINFO;

inline BOOL FlashWindowEx(PFLASHWINFO pfwi) { (void)pfwi; return TRUE; }

// ============================================================
// DrawIcon
// ============================================================
#define DI_MASK    0x0001
#define DI_IMAGE   0x0002
#define DI_NORMAL  (DI_MASK | DI_IMAGE)

// Icon info
typedef struct _ICONINFO {
	BOOL    fIcon;
	DWORD   xHotspot;
	DWORD   yHotspot;
	HBITMAP hbmMask;
	HBITMAP hbmColor;
} ICONINFO, *PICONINFO;

inline BOOL GetIconInfo(HICON hIcon, PICONINFO piconinfo) {
	if (piconinfo) memset(piconinfo, 0, sizeof(ICONINFO));
	return FALSE;
}
inline HICON CreateIconIndirect(PICONINFO piconinfo) { return nullptr; }
inline BOOL DestroyIcon(HICON hIcon) { return TRUE; }

inline BOOL DrawIconEx(HDC hdc, int xLeft, int yTop, HICON hIcon, int cxWidth, int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{
	(void)hdc; (void)xLeft; (void)yTop; (void)hIcon; (void)cxWidth; (void)cyWidth; (void)istepIfAniCur; (void)hbrFlickerFreeDraw; (void)diFlags;
	return FALSE;
}

// ============================================================
// System colors
// ============================================================
#define COLOR_SCROLLBAR        0
#define COLOR_BACKGROUND       1
#define COLOR_ACTIVECAPTION    2
#define COLOR_INACTIVECAPTION  3
#define COLOR_MENU             4
#define COLOR_WINDOW           5
#define COLOR_WINDOWFRAME      6
#define COLOR_MENUTEXT         7
#define COLOR_WINDOWTEXT       8
#define COLOR_CAPTIONTEXT      9
#define COLOR_ACTIVEBORDER     10
#define COLOR_INACTIVEBORDER   11
#define COLOR_APPWORKSPACE     12
#define COLOR_HIGHLIGHT        13
#define COLOR_HIGHLIGHTTEXT    14
#define COLOR_BTNFACE          15
#define COLOR_3DFACE           COLOR_BTNFACE
#define COLOR_BTNSHADOW        16
#define COLOR_3DSHADOW         COLOR_BTNSHADOW
#define COLOR_3DHIGHLIGHT      COLOR_BTNHIGHLIGHT
#define COLOR_3DHILIGHT        COLOR_BTNHIGHLIGHT
#define COLOR_GRAYTEXT         17
#define COLOR_BTNTEXT          18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT     20
#define COLOR_HOTLIGHT         26
#define COLOR_GRADIENTACTIVECAPTION 27
#define COLOR_GRADIENTINACTIVECAPTION 28

// ============================================================
// Static control styles
// ============================================================
#define SS_LEFT          0x00000000L
#define SS_CENTER        0x00000001L
#define SS_RIGHT         0x00000002L
#define SS_ICON          0x00000003L
#define SS_BLACKRECT     0x00000004L
#define SS_SIMPLE        0x0000000BL
#define SS_LEFTNOWORDWRAP 0x0000000CL
#define SS_OWNERDRAW     0x0000000DL
#define SS_BITMAP        0x0000000EL
#define SS_PATHELLIPSIS  0x00008000L
#define SS_ENDELLIPSIS   0x00004000L
#define SS_WORDELLIPSIS  0x0000C000L
#define SS_NOTIFY        0x00000100L

// ============================================================
// MENUBARINFO
// ============================================================
#define OBJID_MENU  (-3L)
#define OBJID_CLIENT (-4L)

typedef struct tagMENUBARINFO {
	DWORD cbSize;
	RECT  rcBar;
	HMENU hMenu;
	HWND  hwndMenu;
	BOOL  fBarFocused : 1;
	BOOL  fFocused : 1;
} MENUBARINFO, *PMENUBARINFO, *LPMENUBARINFO;

inline BOOL GetMenuBarInfo(HWND hwnd, LONG idObject, LONG idItem, PMENUBARINFO pmbi)
{
	if (pmbi) memset(pmbi, 0, pmbi->cbSize);
	return FALSE;
}

// ============================================================
// EnumThreadWindows / EnumWindows
// ============================================================
typedef BOOL (CALLBACK* WNDENUMPROC)(HWND, LPARAM);
inline BOOL EnumThreadWindows(DWORD dwThreadId, WNDENUMPROC lpfn, LPARAM lParam)
{
	(void)dwThreadId; (void)lpfn; (void)lParam;
	return TRUE;
}
inline BOOL EnumWindows(WNDENUMPROC lpEnumFunc, LPARAM lParam)
{
	(void)lpEnumFunc; (void)lParam;
	return TRUE;
}
BOOL EnumChildWindows(HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam);
