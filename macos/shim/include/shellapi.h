#pragma once
// Win32 Shim: Shell API for macOS

#include "windef.h"

// ============================================================
// ShellExecute
// ============================================================
HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile,
                        LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
// Overload for macOS: std::filesystem::path::c_str() returns const char*
HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, const char* lpFile,
                        LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
#define ShellExecute ShellExecuteW

// ============================================================
// SHELLEXECUTEINFO
// ============================================================
typedef struct _SHELLEXECUTEINFOW {
	DWORD     cbSize;
	ULONG     fMask;
	HWND      hwnd;
	LPCWSTR   lpVerb;
	LPCWSTR   lpFile;
	LPCWSTR   lpParameters;
	LPCWSTR   lpDirectory;
	int       nShow;
	HINSTANCE hInstApp;
	LPVOID    lpIDList;
	LPCWSTR   lpClass;
	HKEY      hkeyClass;
	DWORD     dwHotKey;
	union {
		HANDLE hIcon;
		HANDLE hMonitor;
	};
	HANDLE    hProcess;
} SHELLEXECUTEINFOW, *LPSHELLEXECUTEINFOW;
typedef SHELLEXECUTEINFOW SHELLEXECUTEINFO;
typedef LPSHELLEXECUTEINFOW LPSHELLEXECUTEINFO;

BOOL ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo);
#define ShellExecuteEx ShellExecuteExW

#define SEE_MASK_DEFAULT         0x00000000
#define SEE_MASK_CLASSNAME       0x00000001
#define SEE_MASK_CLASSKEY        0x00000003
#define SEE_MASK_IDLIST          0x00000004
#define SEE_MASK_INVOKEIDLIST    0x0000000C
#define SEE_MASK_ICON            0x00000010
#define SEE_MASK_HOTKEY          0x00000020
#define SEE_MASK_NOCLOSEPROCESS  0x00000040
#define SEE_MASK_CONNECTNETDRV   0x00000080
#define SEE_MASK_NOASYNC         0x00000100
#define SEE_MASK_FLAG_DDEWAIT    SEE_MASK_NOASYNC
#define SEE_MASK_DOENVSUBST      0x00000200
#define SEE_MASK_FLAG_NO_UI      0x00000400
#define SEE_MASK_UNICODE         0x00004000
#define SEE_MASK_NO_CONSOLE      0x00008000
#define SEE_MASK_ASYNCOK         0x00100000
#define SEE_MASK_NOQUERYCLASSSTORE 0x01000000
#define SEE_MASK_HMONITOR        0x00200000
#define SEE_MASK_NOZONECHECKS    0x00800000
#define SEE_MASK_WAITFORINPUTIDLE 0x02000000
#define SEE_MASK_FLAG_LOG_USAGE  0x04000000

// ============================================================
// Drag and drop (HDROP)
// ============================================================
UINT DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);
#define DragQueryFile DragQueryFileW

BOOL DragQueryPoint(HDROP hDrop, LPPOINT lppt);
void DragAcceptFiles(HWND hWnd, BOOL fAccept);
void DragFinish(HDROP hDrop);

// ============================================================
// Notify icon (system tray - stub)
// ============================================================
typedef struct _NOTIFYICONDATAW {
	DWORD cbSize;
	HWND  hWnd;
	UINT  uID;
	UINT  uFlags;
	UINT  uCallbackMessage;
	HICON hIcon;
	WCHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	WCHAR szInfo[256];
	union {
		UINT uTimeout;
		UINT uVersion;
	};
	WCHAR szInfoTitle[64];
	DWORD dwInfoFlags;
} NOTIFYICONDATAW, *PNOTIFYICONDATAW;
typedef NOTIFYICONDATAW NOTIFYICONDATA;

#define NIM_ADD    0x00000000
#define NIM_MODIFY 0x00000001
#define NIM_DELETE 0x00000002
#define NIF_MESSAGE 0x00000001
#define NIF_ICON    0x00000002
#define NIF_TIP     0x00000004
#define NIF_STATE   0x00000008
#define NIF_INFO    0x00000010

BOOL Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW lpData);
#define Shell_NotifyIcon Shell_NotifyIconW

// ============================================================
// SHGetFolderPath / known folders
// ============================================================
#define CSIDL_APPDATA        0x001A
#define CSIDL_LOCAL_APPDATA  0x001C
#define CSIDL_COMMON_APPDATA 0x0023
#define CSIDL_PERSONAL       0x0005
#define CSIDL_DESKTOP        0x0000
#define CSIDL_PROGRAMS       0x0002
#define CSIDL_PROFILE        0x0028
#define CSIDL_PROGRAM_FILES  0x0026
#define CSIDL_PROGRAM_FILESX86 0x002A
#define CSIDL_SYSTEM         0x0025
#define CSIDL_WINDOWS        0x0024

HRESULT SHGetFolderPathW(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
#define SHGetFolderPath SHGetFolderPathW

// ============================================================
// ExtractIcon
// ============================================================
HICON ExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex);
#define ExtractIcon ExtractIconW

// ============================================================
// Shell file operations
// ============================================================
typedef struct _SHFILEOPSTRUCTW {
	HWND         hwnd;
	UINT         wFunc;
	LPCWSTR      pFrom;
	LPCWSTR      pTo;
	WORD         fFlags;
	BOOL         fAnyOperationsAborted;
	LPVOID       hNameMappings;
	LPCWSTR      lpszProgressTitle;
} SHFILEOPSTRUCTW, *LPSHFILEOPSTRUCTW;
typedef SHFILEOPSTRUCTW SHFILEOPSTRUCT;
typedef LPSHFILEOPSTRUCTW LPSHFILEOPSTRUCT;

#define FO_MOVE   0x0001
#define FO_COPY   0x0002
#define FO_DELETE 0x0003
#define FO_RENAME 0x0004

#define FOF_MULTIDESTFILES     0x0001
#define FOF_CONFIRMMOUSE       0x0002
#define FOF_SILENT             0x0004
#define FOF_RENAMEONCOLLISION  0x0008
#define FOF_NOCONFIRMATION     0x0010
#define FOF_ALLOWUNDO          0x0040
#define FOF_FILESONLY          0x0080
#define FOF_SIMPLEPROGRESS     0x0100
#define FOF_NOCONFIRMMKDIR     0x0200
#define FOF_NOERRORUI          0x0400
#define FOF_NOCOPYSECURITYATTRIBS 0x0800
#define FOF_NO_UI (FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR)

int SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp);
#define SHFileOperation SHFileOperationW
