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

// Platform-agnostic type definitions
// This header provides compatibility types for cross-platform development

#ifdef __APPLE__
    // macOS platform types
    #include <cstdint>
    #include <cstddef>
    #ifdef __OBJC__
        #include <objc/objc.h>
    #endif
    
    // Window and instance handles
    typedef void* HWND;
    typedef void* HINSTANCE;
    typedef void* HMENU;
    typedef void* HICON;
    typedef void* HCURSOR;
    typedef void* HBRUSH;
    typedef void* HDC;
    typedef void* HBITMAP;
    typedef void* HFONT;
    typedef void* HMODULE;
    typedef void* HANDLE;
    typedef void* HGLOBAL;
    typedef void* HDROP;
    typedef void* HACCEL;
    
    // Integer types
    typedef long LONG;
    typedef unsigned long ULONG;
    typedef unsigned long DWORD;
    typedef unsigned int UINT;
    typedef int INT;
    typedef unsigned char BYTE;
    typedef unsigned short WORD;
    typedef unsigned char UCHAR;
    #ifndef __OBJC__
    typedef int BOOL;
    #endif
    typedef long long LONGLONG;
    typedef unsigned long long ULONGLONG;
    
    // Pointer types
    typedef intptr_t INT_PTR;
    typedef uintptr_t UINT_PTR;
    typedef intptr_t LONG_PTR;
    typedef uintptr_t ULONG_PTR;
    typedef uintptr_t DWORD_PTR;
    typedef LONG_PTR LPARAM;
    typedef UINT_PTR WPARAM;
    typedef LONG_PTR LRESULT;
    
    // String types
    typedef char CHAR;
    typedef wchar_t WCHAR;
    typedef char* LPSTR;
    typedef const char* LPCSTR;
    typedef wchar_t* LPWSTR;
    typedef const wchar_t* LPCWSTR;
    typedef wchar_t* PWSTR;
    typedef const wchar_t* PCWSTR;
    typedef void* PVOID;
    typedef void* LPVOID;
    typedef const void* LPCVOID;
    
    // Boolean values
    #ifndef TRUE
    #define TRUE 1
    #endif
    #ifndef FALSE
    #define FALSE 0
    #endif
    
    // Common constants
    #define MAX_PATH 260
    #define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
    
    // Rectangle structure
    typedef struct tagRECT {
        LONG left;
        LONG top;
        LONG right;
        LONG bottom;
    } RECT, *PRECT, *LPRECT;
    
    // Point structure
    typedef struct tagPOINT {
        LONG x;
        LONG y;
    } POINT, *PPOINT, *LPPOINT;
    
    // Size structure
    typedef struct tagSIZE {
        LONG cx;
        LONG cy;
    } SIZE, *PSIZE, *LPSIZE;
    
    // Message structure (simplified)
    typedef struct tagMSG {
        HWND hwnd;
        UINT message;
        WPARAM wParam;
        LPARAM lParam;
        DWORD time;
        POINT pt;
    } MSG, *PMSG, *LPMSG;
    
    // Window placement (simplified)
    typedef struct tagWINDOWPLACEMENT {
        UINT length;
        UINT flags;
        UINT showCmd;
        POINT ptMinPosition;
        POINT ptMaxPosition;
        RECT rcNormalPosition;
    } WINDOWPLACEMENT;
    
    // File time structure
    typedef struct _FILETIME {
        DWORD dwLowDateTime;
        DWORD dwHighDateTime;
    } FILETIME, *PFILETIME, *LPFILETIME;
    
    // File attribute data
    typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
        DWORD dwFileAttributes;
        FILETIME ftCreationTime;
        FILETIME ftLastAccessTime;
        FILETIME ftLastWriteTime;
        DWORD nFileSizeHigh;
        DWORD nFileSizeLow;
    } WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;
    
    // Common Windows constants
    #define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
    #define FILE_ATTRIBUTE_READONLY 0x00000001
    #define FILE_ATTRIBUTE_HIDDEN 0x00000002
    #define FILE_ATTRIBUTE_SYSTEM 0x00000004
    #define FILE_ATTRIBUTE_DIRECTORY 0x00000010
    #define FILE_ATTRIBUTE_ARCHIVE 0x00000020
    #define FILE_ATTRIBUTE_NORMAL 0x00000080
    
    // Error codes
    #define ERROR_SUCCESS 0L
    #define ERROR_FILE_NOT_FOUND 2L
    #define ERROR_ACCESS_DENIED 5L
    #define ERROR_INVALID_HANDLE 6L
    #define ERROR_INVALID_PARAMETER 87L
    #define ERROR_ALREADY_EXISTS 183L
    #define ERROR_FILE_EXISTS 80L
    #define NO_ERROR 0L
    
    // Window messages (placeholders - will be mapped to Cocoa events)
    #define WM_NULL 0x0000
    #define WM_CREATE 0x0001
    #define WM_DESTROY 0x0002
    #define WM_MOVE 0x0003
    #define WM_SIZE 0x0005
    #define WM_ACTIVATE 0x0006
    #define WM_SETFOCUS 0x0007
    #define WM_KILLFOCUS 0x0008
    #define WM_PAINT 0x000F
    #define WM_CLOSE 0x0010
    #define WM_QUIT 0x0012
    #define WM_ERASEBKGND 0x0014
    #define WM_SHOWWINDOW 0x0018
    #define WM_ACTIVATEAPP 0x001C
    #define WM_SETCURSOR 0x0020
    #define WM_MOUSEACTIVATE 0x0021
    #define WM_GETMINMAXINFO 0x0024
    #define WM_WINDOWPOSCHANGING 0x0046
    #define WM_WINDOWPOSCHANGED 0x0047
    #define WM_NOTIFY 0x004E
    #define WM_COMMAND 0x0111
    #define WM_SYSCOMMAND 0x0112
    #define WM_TIMER 0x0113
    #define WM_HSCROLL 0x0114
    #define WM_VSCROLL 0x0115
    #define WM_INITMENU 0x0116
    #define WM_INITMENUPOPUP 0x0117
    #define WM_MENUSELECT 0x011F
    #define WM_MENUCHAR 0x0120
    #define WM_ENTERIDLE 0x0121
    #define WM_CTLCOLORMSGBOX 0x0132
    #define WM_CTLCOLOREDIT 0x0133
    #define WM_CTLCOLORLISTBOX 0x0134
    #define WM_CTLCOLORBTN 0x0135
    #define WM_CTLCOLORDLG 0x0136
    #define WM_CTLCOLORSCROLLBAR 0x0137
    #define WM_CTLCOLORSTATIC 0x0138
    #define WM_MOUSEMOVE 0x0200
    #define WM_LBUTTONDOWN 0x0201
    #define WM_LBUTTONUP 0x0202
    #define WM_LBUTTONDBLCLK 0x0203
    #define WM_RBUTTONDOWN 0x0204
    #define WM_RBUTTONUP 0x0205
    #define WM_RBUTTONDBLCLK 0x0206
    #define WM_MBUTTONDOWN 0x0207
    #define WM_MBUTTONUP 0x0208
    #define WM_MBUTTONDBLCLK 0x0209
    #define WM_MOUSEWHEEL 0x020A
    #define WM_KEYDOWN 0x0100
    #define WM_KEYUP 0x0101
    #define WM_CHAR 0x0102
    #define WM_SYSKEYDOWN 0x0104
    #define WM_SYSKEYUP 0x0105
    #define WM_SYSCHAR 0x0106
    #define WM_COPYDATA 0x004A
    #define WM_DROPFILES 0x0233
    #define WM_USER 0x0400
    
    // Window styles (placeholders)
    #define WS_OVERLAPPED 0x00000000L
    #define WS_POPUP 0x80000000L
    #define WS_CHILD 0x40000000L
    #define WS_MINIMIZE 0x20000000L
    #define WS_VISIBLE 0x10000000L
    #define WS_DISABLED 0x08000000L
    #define WS_CLIPSIBLINGS 0x04000000L
    #define WS_CLIPCHILDREN 0x02000000L
    #define WS_MAXIMIZE 0x01000000L
    #define WS_CAPTION 0x00C00000L
    #define WS_BORDER 0x00800000L
    #define WS_DLGFRAME 0x00400000L
    #define WS_VSCROLL 0x00200000L
    #define WS_HSCROLL 0x00100000L
    #define WS_SYSMENU 0x00080000L
    #define WS_THICKFRAME 0x00040000L
    #define WS_GROUP 0x00020000L
    #define WS_TABSTOP 0x00010000L
    #define WS_MINIMIZEBOX 0x00020000L
    #define WS_MAXIMIZEBOX 0x00010000L
    #define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
    
    // Show window commands
    #define SW_HIDE 0
    #define SW_SHOWNORMAL 1
    #define SW_NORMAL 1
    #define SW_SHOWMINIMIZED 2
    #define SW_SHOWMAXIMIZED 3
    #define SW_MAXIMIZE 3
    #define SW_SHOWNOACTIVATE 4
    #define SW_SHOW 5
    #define SW_MINIMIZE 6
    #define SW_SHOWMINNOACTIVE 7
    #define SW_SHOWNA 8
    #define SW_RESTORE 9
    
    // Menu flags
    #define MF_BYCOMMAND 0x00000000L
    #define MF_BYPOSITION 0x00000400L
    #define MF_CHECKED 0x00000008L
    #define MF_UNCHECKED 0x00000000L
    #define MF_ENABLED 0x00000000L
    #define MF_GRAYED 0x00000001L
    #define MF_DISABLED 0x00000002L
    
    // Message box flags
    #define MB_OK 0x00000000L
    #define MB_OKCANCEL 0x00000001L
    #define MB_YESNOCANCEL 0x00000003L
    #define MB_YESNO 0x00000004L
    #define MB_ICONERROR 0x00000010L
    #define MB_ICONQUESTION 0x00000020L
    #define MB_ICONWARNING 0x00000030L
    #define MB_ICONINFORMATION 0x00000040L
    
    // Message box return values
    #define IDOK 1
    #define IDCANCEL 2
    #define IDABORT 3
    #define IDRETRY 4
    #define IDIGNORE 5
    #define IDYES 6
    #define IDNO 7
    
    // Virtual key codes (subset - add more as needed)
    #define VK_BACK 0x08
    #define VK_TAB 0x09
    #define VK_RETURN 0x0D
    #define VK_SHIFT 0x10
    #define VK_CONTROL 0x11
    #define VK_MENU 0x12
    #define VK_ESCAPE 0x1B
    #define VK_SPACE 0x20
    #define VK_PRIOR 0x21
    #define VK_NEXT 0x22
    #define VK_END 0x23
    #define VK_HOME 0x24
    #define VK_LEFT 0x25
    #define VK_UP 0x26
    #define VK_RIGHT 0x27
    #define VK_DOWN 0x28
    #define VK_INSERT 0x2D
    #define VK_DELETE 0x2E
    #define VK_F1 0x70
    #define VK_F2 0x71
    #define VK_F3 0x72
    #define VK_F4 0x73
    #define VK_F5 0x74
    #define VK_F6 0x75
    #define VK_F7 0x76
    #define VK_F8 0x77
    #define VK_F9 0x78
    #define VK_F10 0x79
    #define VK_F11 0x7A
    #define VK_F12 0x7B
    
    // COPYDATA structure
    typedef struct tagCOPYDATASTRUCT {
        ULONG_PTR dwData;
        DWORD cbData;
        PVOID lpData;
    } COPYDATASTRUCT, *PCOPYDATASTRUCT;
    
#elif defined(_WIN32)
    // On Windows, use native types
    #include <windows.h>
#else
    #error "Unsupported platform"
#endif

// Cross-platform helper macros
#ifdef __APPLE__
    #define WINAPI
    #define CALLBACK
    #define APIENTRY
    #define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
    #define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
    #define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
    #define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
    #define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
    #define MAKELRESULT(l, h) ((LRESULT)(DWORD)MAKELONG(l, h))
#endif
