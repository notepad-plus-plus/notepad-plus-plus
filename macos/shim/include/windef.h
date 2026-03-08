#pragma once
// Win32 Shim: Core type definitions for macOS
// Maps Win32 types to macOS equivalents

#include <cstdint>
#include <cstddef>
#include <climits>
#include <xlocale.h>

// MSVC _locale_t → POSIX locale_t
#ifndef _locale_t
typedef locale_t _locale_t;
#endif

// Calling conventions (no-ops on macOS)
#define WINAPI
#define CALLBACK
#define APIENTRY
#define PASCAL
#define STDMETHODCALLTYPE
#define CDECL
#define __cdecl
#define __stdcall
#define __fastcall
#define __thiscall

// Linkage specifiers
#define DECLSPEC_IMPORT
#define DECLSPEC_NORETURN __attribute__((noreturn))
#define __declspec(x)

// SAL annotations (no-ops)
#define _In_
#define _In_opt_
#define _Out_
#define _Out_opt_
#define _Inout_
#define _Inout_opt_
#define _In_z_
#define _In_reads_(s)
#define _In_reads_opt_(s)
#define _In_reads_bytes_(s)
#define _Out_writes_(s)
#define _Out_writes_opt_(s)
#define _Out_writes_bytes_(s)
#define _Out_writes_to_(s, c)
#define __inout
#define __in
#define __out
#define __in_opt
#define __out_opt
#define __inout_opt
#define _Outptr_
#define _Outptr_opt_
#define _When_(c, a)
#define _Return_type_success_(c)
#define _Success_(c)
#define _Check_return_
#define _Null_terminated_
#define _Pre_maybenull_
#define _Post_valid_
#define _Printf_format_string_
#define _Analysis_assume_(e)
#define _Field_size_(s)
#define _Field_size_opt_(s)
#define _Ret_maybenull_

// Basic integer types
// On macOS ARM64 with ObjC, BOOL is defined as bool in objc/objc.h.
// Win32 BOOL is int. In ObjC++ (.mm) files, we skip the typedef since
// ObjC already defines BOOL. Win32 code uses BOOL as 0/1 anyway,
// so bool vs int difference doesn't matter in practice.
#ifndef __OBJC__
typedef int                 BOOL;
#endif
typedef unsigned char       BYTE;
typedef unsigned char       UCHAR;
typedef unsigned short      WORD;
typedef unsigned int        DWORD;
typedef unsigned long       ULONG;
typedef long                LONG;
typedef long long           LONGLONG;
typedef unsigned long long  ULONGLONG;
#define __int64 long long
typedef int                 INT;
typedef unsigned int        UINT;
typedef short               SHORT;
typedef unsigned short      USHORT;
typedef float               FLOAT;
typedef char                CHAR;
typedef wchar_t             WCHAR;

#define CONST const
#define VOID  void

typedef DWORD*              LPDWORD;
typedef DWORD*              PDWORD;
typedef DWORD               LCID;
typedef DWORD               LCTYPE;
typedef WORD                LANGID;
typedef WORD                CLIPFORMAT;
typedef WORD*               LPWORD;
typedef BYTE*               LPBYTE;
typedef const BYTE*         LPCBYTE;
typedef BOOL*               LPBOOL;
typedef BOOL*               PBOOL;
typedef void*               LPVOID;
typedef void*               PVOID;
typedef const void*         LPCVOID;
typedef INT*                LPINT;
typedef UINT*               LPUINT;
typedef UINT*               PUINT;

// Pointer-sized integer types
typedef intptr_t            INT_PTR;
typedef uintptr_t           UINT_PTR;
typedef intptr_t            LONG_PTR;
typedef uintptr_t           ULONG_PTR;
typedef uintptr_t           DWORD_PTR;

// Windows WPARAM/LPARAM/LRESULT
typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;

// Size types
typedef size_t              SIZE_T;
typedef ULONG_PTR           size_type;

// String types
typedef char*               LPSTR;
typedef const char*         LPCSTR;
typedef wchar_t*            LPWSTR;
typedef const wchar_t*      LPCWSTR;
typedef wchar_t*            PWSTR;
typedef const wchar_t*      PCWSTR;

// TCHAR - always wide on macOS (matching Unicode build)
typedef wchar_t             TCHAR;
typedef wchar_t*            LPTSTR;
typedef wchar_t*            PTSTR;
typedef const wchar_t*      LPCTSTR;
typedef const wchar_t*      PCTSTR;
#define _T(x) L##x
#define TEXT(x) L##x

// Handle types (opaque pointers)
typedef void*               HANDLE;
typedef void*               HWND;
typedef void*               HDC;
typedef void*               HINSTANCE;
typedef void*               HMODULE;
typedef void*               HICON;
typedef void*               HCURSOR;
typedef void*               HBRUSH;
typedef void*               HPEN;
typedef void*               HFONT;
typedef void*               HBITMAP;
typedef void*               HGDIOBJ;
typedef void*               HRGN;
typedef void*               HMENU;
typedef void*               HACCEL;
typedef void*               HGLOBAL;
typedef void*               HLOCAL;
typedef void*               HRSRC;
typedef void*               HTREEITEM;
typedef void*               HIMAGELIST;
typedef void*               HPALETTE;
typedef void*               HMONITOR;
typedef void*               HDROP;
typedef void*               HRESULT_TYPEDEF;
typedef void*               HKEY;
typedef void*               HTHEME;
typedef void*               HDWP;
typedef void*               HCOLORSPACE;

typedef HINSTANCE           HMODULE_TYPEDEF;

// HRESULT as long
typedef long                HRESULT;

// ATOM
typedef WORD                ATOM;

// Color
typedef DWORD               COLORREF;
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))

// Geometry types
typedef struct tagPOINT {
	LONG x;
	LONG y;
} POINT, *LPPOINT;

typedef struct tagSIZE {
	LONG cx;
	LONG cy;
} SIZE, *LPSIZE;

typedef struct tagPOINTS {
	SHORT x;
	SHORT y;
} POINTS, *LPPOINTS;

#define MAKEPOINTS(l)    (*((POINTS*)&(l)))
#define POINTSTOPOINT(pt, pts) { (pt).x = (LONG)(SHORT)LOWORD(*(LONG*)&pts); (pt).y = (LONG)(SHORT)HIWORD(*(LONG*)&pts); }

typedef struct tagRECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT, *LPRECT;
typedef const RECT* LPCRECT;

// Boolean constants
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// NULL
#ifndef NULL
#define NULL nullptr
#endif

// INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

// Success/failure macros for HRESULT
#define S_OK       ((HRESULT)0L)
#define NOERROR    0L
#define S_FALSE    ((HRESULT)1L)
#define E_FAIL     ((HRESULT)0x80004005L)
#define E_NOTIMPL  ((HRESULT)0x80004001L)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_UNEXPECTED ((HRESULT)0x8000FFFFL)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define E_INVALIDARG ((HRESULT)0x80070057L)
#define E_POINTER   ((HRESULT)0x80004003L)
#define E_ABORT     ((HRESULT)0x80004004L)
#define E_ACCESSDENIED ((HRESULT)0x80070005L)
#define RPC_E_CHANGED_MODE ((HRESULT)0x80010106L)

#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr)    (((HRESULT)(hr)) < 0)
#define HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT)(((x) & 0x0000FFFF) | (7 << 16) | 0x80000000)))

// HIWORD/LOWORD/MAKELONG/MAKEWPARAM/MAKELPARAM
#define LOWORD(l)     ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)     ((WORD)(((DWORD_PTR)(l) >> 16) & 0xffff))
#define LOBYTE(w)     ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)     ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xff))
#define MAKELONG(a, b)    ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define MAKEWPARAM(l, h)  ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELPARAM(l, h)  ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h) ((LRESULT)(DWORD)MAKELONG(l, h))

// MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// INFINITE
#define INFINITE 0xFFFFFFFF

// Callback typedefs
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef INT_PTR (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void (*TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef BOOL (*WNDENUMPROC)(HWND, LPARAM);

// LARGE_INTEGER / ULARGE_INTEGER
typedef union _LARGE_INTEGER {
	struct {
		DWORD LowPart;
		LONG  HighPart;
	};
	struct {
		DWORD LowPart;
		LONG  HighPart;
	} u;
	LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
	struct {
		DWORD LowPart;
		DWORD HighPart;
	};
	struct {
		DWORD LowPart;
		DWORD HighPart;
	} u;
	ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

// FILETIME
typedef struct _FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

// SYSTEMTIME
typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

// GUID
typedef struct _GUID {
	DWORD Data1;
	WORD  Data2;
	WORD  Data3;
	BYTE  Data4[8];
} GUID, IID, CLSID;
typedef const GUID& REFGUID;
typedef const IID& REFIID;
typedef const CLSID& REFCLSID;

// SECURITY_ATTRIBUTES
typedef struct _SECURITY_ATTRIBUTES {
	DWORD  nLength;
	LPVOID lpSecurityDescriptor;
	BOOL   bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

// OVERLAPPED
typedef struct _OVERLAPPED {
	ULONG_PTR Internal;
	ULONG_PTR InternalHigh;
	union {
		struct {
			DWORD Offset;
			DWORD OffsetHigh;
		};
		LPVOID Pointer;
	};
	HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

// Critical section (mapped to pthread mutex)
#include <pthread.h>
typedef struct _CRITICAL_SECTION {
	pthread_mutex_t mutex;
} CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;

// Interface macros
#define DECLARE_HANDLE(name) typedef void* name
#define DECLARE_INTERFACE(iface) struct iface
#define DECLARE_INTERFACE_(iface, base) struct iface : public base

// COM-like macros
#define STDMETHOD(method) virtual HRESULT method
#define STDMETHOD_(type, method) virtual type method
#define STDMETHODIMP HRESULT
#define STDMETHODIMP_(type) type
#define PURE = 0
#define THIS_
#define THIS

// IUnknown minimal definition
struct IUnknown {
	virtual HRESULT QueryInterface(REFIID riid, void** ppvObject) = 0;
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	virtual ~IUnknown() = default;
};

// min/max macros (Windows-style, though often problematic)
#ifndef NOMINMAX
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#endif
