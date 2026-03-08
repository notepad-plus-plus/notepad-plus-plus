#pragma once
// Win32 Shim: Shell Object API for macOS

#include "windef.h"
#include "shlwapi.h"

#include <cstdlib>

// COM basics are now in windows.h (via objbase.h on Windows)
// Include windows.h to get them if not already included
#ifndef _WINDOWS_H_SHIM_
#include "windows.h"
#endif

inline LPVOID CoTaskMemAlloc(SIZE_T cb)
{
	return malloc(cb);
}

// ============================================================
// SHGetFolderPath
// ============================================================
#define SHGFP_TYPE_CURRENT 0
#define SHGFP_TYPE_DEFAULT 1

// SHGetFolderPath is declared in shlwapi.h (our shim)
// but we also provide the W variant name here
#ifndef SHGetFolderPath
#define SHGetFolderPath SHGetFolderPathW
#endif

// ============================================================
// SHBrowseForFolder
// ============================================================
typedef struct _ITEMIDLIST {
	USHORT mkid;
} ITEMIDLIST;

typedef ITEMIDLIST* LPITEMIDLIST;
typedef const ITEMIDLIST* LPCITEMIDLIST;
typedef LPITEMIDLIST PIDLIST_ABSOLUTE;

typedef int (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

typedef struct _BROWSEINFOW {
	HWND          hwndOwner;
	LPCITEMIDLIST  pidlRoot;
	LPWSTR        pszDisplayName;
	LPCWSTR       lpszTitle;
	UINT          ulFlags;
	BFFCALLBACK   lpfn;
	LPARAM        lData;
	int           iImage;
} BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW;

#define BROWSEINFO BROWSEINFOW

#define BIF_RETURNONLYFSDIRS   0x00000001
#define BIF_DONTGOBELOWDOMAIN  0x00000002
#define BIF_STATUSTEXT         0x00000004
#define BIF_EDITBOX            0x00000010
#define BIF_NEWDIALOGSTYLE     0x00000040
#define BIF_USENEWUI           (BIF_EDITBOX | BIF_NEWDIALOGSTYLE)

#define BFFM_INITIALIZED  1
#define BFFM_SELCHANGED   2
#define BFFM_SETSELECTIONW (WM_USER + 103)
#define BFFM_SETSTATUSTEXTW (WM_USER + 104)
#define BFFM_SETSELECTION BFFM_SETSELECTIONW

inline LPITEMIDLIST SHBrowseForFolderW(LPBROWSEINFOW lpbi)
{
	(void)lpbi;
	return nullptr;
}
#define SHBrowseForFolder SHBrowseForFolderW

inline BOOL SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath)
{
	(void)pidl;
	if (pszPath) pszPath[0] = L'\0';
	return FALSE;
}
#define SHGetPathFromIDList SHGetPathFromIDListW

inline HRESULT SHParseDisplayName(LPCWSTR pszName, void* pbc, LPITEMIDLIST* ppidl, DWORD sfgaoIn, DWORD* psfgaoOut)
{
	(void)pszName; (void)pbc; (void)sfgaoIn; (void)psfgaoOut;
	if (ppidl) *ppidl = nullptr;
	return E_NOTIMPL;
}

inline HRESULT SHOpenFolderAndSelectItems(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST* apidl, DWORD dwFlags)
{
	(void)pidlFolder; (void)cidl; (void)apidl; (void)dwFlags;
	return E_NOTIMPL;
}

// ============================================================
// IShellItem / IFileDialog stubs (COM interfaces)
// ============================================================
// These are opaque stubs - the actual file dialog will be
// replaced by NSOpenPanel/NSSavePanel in Phase 2

// ============================================================
// IShellLink / IPersistFile stubs (for .lnk resolution)
// ============================================================
#define STGM_READ 0x00000000L
#define SLGP_SHORTPATH 0x0001
#define SLGP_UNCPRIORITY 0x0002
#define SLGP_RAWPATH 0x0004

#define CLSCTX_INPROC_SERVER 0x1

struct IPersistFile : public IUnknown {
	virtual HRESULT GetClassID(CLSID* pClassID) = 0;
	virtual HRESULT IsDirty() = 0;
	virtual HRESULT Load(LPCWSTR pszFileName, DWORD dwMode) = 0;
	virtual HRESULT Save(LPCWSTR pszFileName, BOOL fRemember) = 0;
	virtual HRESULT SaveCompleted(LPCWSTR pszFileName) = 0;
	virtual HRESULT GetCurFile(LPWSTR* ppszFileName) = 0;
};

struct IShellLinkW : public IUnknown {
	virtual HRESULT GetPath(LPWSTR pszFile, int cch, void* pfd, DWORD fFlags) = 0;
	virtual HRESULT GetIDList(LPITEMIDLIST* ppidl) = 0;
	virtual HRESULT SetIDList(LPCITEMIDLIST pidl) = 0;
	virtual HRESULT GetDescription(LPWSTR pszName, int cch) = 0;
	virtual HRESULT SetDescription(LPCWSTR pszName) = 0;
	virtual HRESULT GetWorkingDirectory(LPWSTR pszDir, int cch) = 0;
	virtual HRESULT SetWorkingDirectory(LPCWSTR pszDir) = 0;
	virtual HRESULT GetArguments(LPWSTR pszArgs, int cch) = 0;
	virtual HRESULT SetArguments(LPCWSTR pszArgs) = 0;
	virtual HRESULT GetHotkey(WORD* pwHotkey) = 0;
	virtual HRESULT SetHotkey(WORD wHotkey) = 0;
	virtual HRESULT GetShowCmd(int* piShowCmd) = 0;
	virtual HRESULT SetShowCmd(int iShowCmd) = 0;
	virtual HRESULT GetIconLocation(LPWSTR pszIconPath, int cch, int* piIcon) = 0;
	virtual HRESULT SetIconLocation(LPCWSTR pszIconPath, int iIcon) = 0;
	virtual HRESULT SetRelativePath(LPCWSTR pszPathRel, DWORD dwReserved) = 0;
	virtual HRESULT Resolve(HWND hwnd, DWORD fFlags) = 0;
	virtual HRESULT SetPath(LPCWSTR pszFile) = 0;
};
typedef IShellLinkW IShellLink;
#define IID_IShellLink IID_IShellLinkW

// Static GUIDs (stub values)
static const IID IID_IShellLinkW = {0x000214F9, 0, 0, {0xC0,0,0,0,0,0,0,0x46}};
static const IID IID_IPersistFile = {0x0000010b, 0, 0, {0xC0,0,0,0,0,0,0,0x46}};
static const CLSID CLSID_ShellLink = {0x00021401, 0, 0, {0xC0,0,0,0,0,0,0,0x46}};

#ifndef __IShellItem_FWD_DEFINED__
#define __IShellItem_FWD_DEFINED__
typedef struct IShellItem IShellItem;
#endif

#ifndef __IFileDialog_FWD_DEFINED__
#define __IFileDialog_FWD_DEFINED__
typedef struct IFileDialog IFileDialog;
#endif

#ifndef __IFileDialogEvents_FWD_DEFINED__
#define __IFileDialogEvents_FWD_DEFINED__
typedef struct IFileDialogEvents IFileDialogEvents;
#endif

#ifndef __IFileDialogCustomize_FWD_DEFINED__
#define __IFileDialogCustomize_FWD_DEFINED__
typedef struct IFileDialogCustomize IFileDialogCustomize;
#endif

#ifndef __IFileDialogControlEvents_FWD_DEFINED__
#define __IFileDialogControlEvents_FWD_DEFINED__
typedef struct IFileDialogControlEvents IFileDialogControlEvents;
#endif

typedef enum {
	SIGDN_NORMALDISPLAY = 0,
	SIGDN_FILESYSPATH   = 0x80058000,
	SIGDN_URL           = 0x80068000
} SIGDN;

typedef enum {
	FDE_SHAREVIOLATION_DEFAULT = 0,
	FDE_SHAREVIOLATION_ACCEPT  = 1,
	FDE_SHAREVIOLATION_REFUSE  = 2
} FDE_SHAREVIOLATION_RESPONSE;

typedef enum {
	FDE_OVERWRITE_DEFAULT = 0,
	FDE_OVERWRITE_ACCEPT  = 1,
	FDE_OVERWRITE_REFUSE  = 2
} FDE_OVERWRITE_RESPONSE;

// SHCreateItemFromParsingName stub
inline HRESULT SHCreateItemFromParsingName(LPCWSTR pszPath, void* pbc, REFIID riid, void** ppv)
{
	(void)pszPath;
	(void)pbc;
	(void)riid;
	if (ppv) *ppv = nullptr;
	return E_NOTIMPL;
}

// ============================================================
// Known Folder IDs (GUIDs)
// ============================================================
// These are extern const GUIDs on Windows; we define them as macros mapping to CSIDL
// The actual GUID values don't matter since SHGetKnownFolderPath is stubbed

inline HRESULT SHGetKnownFolderPath(REFGUID rfid, DWORD dwFlags, HANDLE hToken, PWSTR* ppszPath)
{
	(void)rfid;
	(void)dwFlags;
	(void)hToken;
	if (ppszPath) *ppszPath = nullptr;
	return E_NOTIMPL;
}

// ============================================================
// Misc shell functions
// ============================================================
inline HRESULT SHCreateDirectoryExW(HWND hwnd, LPCWSTR pszPath, void* psa)
{
	(void)hwnd;
	(void)psa;
	(void)pszPath;
	return S_OK;
}
#define SHCreateDirectoryEx SHCreateDirectoryExW

inline int SHCreateDirectory(HWND hwnd, LPCWSTR pszPath)
{
	(void)hwnd; (void)pszPath;
	return 0; // ERROR_SUCCESS
}

// IFACEMETHODIMP - used in COM interface implementations
#ifndef IFACEMETHODIMP
#define IFACEMETHODIMP HRESULT STDMETHODCALLTYPE
#endif

#ifndef IFACEMETHODIMP_
#define IFACEMETHODIMP_(type) type STDMETHODCALLTYPE
#endif
