#pragma once
// Win32 Shim: Shell Object IDL interfaces for macOS
// Minimal stubs for IFileDialog, IShellItem, etc.

#include "windef.h"
#include "shlobj.h"

// Forward declarations
struct IFileDialog;
struct IFileDialogEvents;
struct IFileDialogCustomize;
struct IShellItem;
struct IShellItemArray;

// SIGDN, FDE_SHAREVIOLATION_RESPONSE defined in shlobj.h
// Additional SIGDN values
#ifndef SIGDN_PARENTRELATIVEPARSING
#define SIGDN_PARENTRELATIVEPARSING ((SIGDN)0x80018001)
#define SIGDN_DESKTOPABSOLUTEPARSING ((SIGDN)0x80028000)
#define SIGDN_PARENTRELATIVEEDITING ((SIGDN)0x80031001)
#define SIGDN_DESKTOPABSOLUTEEDITING ((SIGDN)0x8004c000)
#define SIGDN_PARENTRELATIVEFORADDRESSBAR ((SIGDN)0x8007c001)
#define SIGDN_PARENTRELATIVE ((SIGDN)0x80080001)
#define SIGDN_PARENTRELATIVEFORUI ((SIGDN)0x80094001)
#endif

// FDAP - default folder path
#ifndef FDAP_BOTTOM
typedef enum {
	FDAP_BOTTOM = 0,
	FDAP_TOP = 1
} FDAP;
#endif

// FDE_OVERWRITE_RESPONSE already defined in shlobj.h

// COMDLG_FILTERSPEC
typedef struct {
	LPCWSTR pszName;
	LPCWSTR pszSpec;
} COMDLG_FILTERSPEC;

// FOS - file open/save dialog options
#define FOS_OVERWRITEPROMPT    0x00000002
#define FOS_STRICTFILETYPES    0x00000004
#define FOS_NOCHANGEDIR        0x00000008
#define FOS_PICKFOLDERS        0x00000020
#define FOS_FORCEFILESYSTEM    0x00000040
#define FOS_ALLNONSTORAGEITEMS 0x00000080
#define FOS_NOVALIDATE         0x00000100
#define FOS_ALLOWMULTISELECT   0x00000200
#define FOS_PATHMUSTEXIST      0x00000800
#define FOS_FILEMUSTEXIST      0x00001000
#define FOS_CREATEPROMPT       0x00002000
#define FOS_SHAREAWARE         0x00004000
#define FOS_NOREADONLYRETURN   0x00008000
#define FOS_NOTESTFILECREATE   0x00010000
#define FOS_HIDEMRUPLACES      0x00020000
#define FOS_HIDEPINNEDPLACES   0x00040000
#define FOS_NODEREFERENCELINKS 0x00100000
#define FOS_DONTADDTORECENT    0x02000000
#define FOS_FORCESHOWHIDDEN    0x10000000
#define FOS_DEFAULTNOMINIMODE  0x20000000
#define FOS_FORCEPREVIEWPANEON 0x40000000

typedef DWORD FILEOPENDIALOGOPTIONS;

// CDCONTROLSTATEF
typedef enum {
	CDCS_INACTIVE  = 0x00000000,
	CDCS_ENABLED   = 0x00000001,
	CDCS_VISIBLE   = 0x00000002,
	CDCS_ENABLEDVISIBLE = 0x00000003
} CDCONTROLSTATEF;

// IShellItem interface
struct IShellItem : public IUnknown {
	virtual HRESULT GetDisplayName(SIGDN sigdnName, LPWSTR* ppszName) = 0;
	virtual HRESULT GetParent(IShellItem** ppsi) = 0;
	virtual HRESULT GetAttributes(ULONG sfgaoMask, ULONG* psfgaoAttribs) = 0;
	virtual HRESULT Compare(IShellItem* psi, DWORD hint, int* piOrder) = 0;
	virtual HRESULT BindToHandler(void* pbc, REFGUID bhid, REFIID riid, void** ppv) = 0;
};

// IShellItemArray interface
struct IShellItemArray : public IUnknown {
	virtual HRESULT GetCount(DWORD* pdwNumItems) = 0;
	virtual HRESULT GetItemAt(DWORD dwIndex, IShellItem** ppsi) = 0;
};

// IModalWindow interface
struct IModalWindow : public IUnknown {
	virtual HRESULT Show(HWND hwndOwner) = 0;
};

// IFileDialog interface
struct IFileDialog : public IModalWindow {
	virtual HRESULT SetFileTypes(UINT cFileTypes, const COMDLG_FILTERSPEC* rgFilterSpec) = 0;
	virtual HRESULT SetFileTypeIndex(UINT iFileType) = 0;
	virtual HRESULT GetFileTypeIndex(UINT* piFileType) = 0;
	virtual HRESULT Advise(IFileDialogEvents* pfde, DWORD* pdwCookie) = 0;
	virtual HRESULT Unadvise(DWORD dwCookie) = 0;
	virtual HRESULT SetOptions(FILEOPENDIALOGOPTIONS fos) = 0;
	virtual HRESULT GetOptions(FILEOPENDIALOGOPTIONS* pfos) = 0;
	virtual HRESULT SetDefaultFolder(IShellItem* psi) = 0;
	virtual HRESULT SetFolder(IShellItem* psi) = 0;
	virtual HRESULT GetFolder(IShellItem** ppsi) = 0;
	virtual HRESULT GetCurrentSelection(IShellItem** ppsi) = 0;
	virtual HRESULT SetFileName(LPCWSTR pszName) = 0;
	virtual HRESULT GetFileName(LPWSTR* pszName) = 0;
	virtual HRESULT SetTitle(LPCWSTR pszTitle) = 0;
	virtual HRESULT SetOkButtonLabel(LPCWSTR pszText) = 0;
	virtual HRESULT SetFileNameLabel(LPCWSTR pszLabel) = 0;
	virtual HRESULT GetResult(IShellItem** ppsi) = 0;
	virtual HRESULT AddPlace(IShellItem* psi, FDAP fdap) = 0;
	virtual HRESULT SetDefaultExtension(LPCWSTR pszDefaultExtension) = 0;
	virtual HRESULT Close(HRESULT hr) = 0;
	virtual HRESULT SetClientGuid(REFGUID guid) = 0;
	virtual HRESULT ClearClientData() = 0;
	virtual HRESULT SetFilter(void* pFilter) = 0;
};

// IFileOpenDialog
struct IFileOpenDialog : public IFileDialog {
	virtual HRESULT GetResults(IShellItemArray** ppenum) = 0;
	virtual HRESULT GetSelectedItems(IShellItemArray** ppsai) = 0;
};

// IFileSaveDialog
struct IFileSaveDialog : public IFileDialog {
	virtual HRESULT SetSaveAsItem(IShellItem* psi) = 0;
	virtual HRESULT SetProperties(void* pStore) = 0;
	virtual HRESULT SetCollectedProperties(void* pList, BOOL fAppendDefault) = 0;
	virtual HRESULT GetProperties(void** ppStore) = 0;
	virtual HRESULT ApplyProperties(IShellItem* psi, void* pStore, HWND hwnd, void* pSink) = 0;
};

// IFileDialogEvents
struct IFileDialogEvents : public IUnknown {
	virtual HRESULT OnFileOk(IFileDialog* pfd) = 0;
	virtual HRESULT OnFolderChanging(IFileDialog* pfd, IShellItem* psiFolder) = 0;
	virtual HRESULT OnFolderChange(IFileDialog* pfd) = 0;
	virtual HRESULT OnSelectionChange(IFileDialog* pfd) = 0;
	virtual HRESULT OnShareViolation(IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse) = 0;
	virtual HRESULT OnTypeChange(IFileDialog* pfd) = 0;
	virtual HRESULT OnOverwrite(IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse) = 0;
};

// IFileDialogCustomize
struct IFileDialogCustomize : public IUnknown {
	virtual HRESULT EnableOpenDropDown(DWORD dwIDCtl) = 0;
	virtual HRESULT AddMenu(DWORD dwIDCtl, LPCWSTR pszLabel) = 0;
	virtual HRESULT AddPushButton(DWORD dwIDCtl, LPCWSTR pszLabel) = 0;
	virtual HRESULT AddComboBox(DWORD dwIDCtl) = 0;
	virtual HRESULT AddRadioButtonList(DWORD dwIDCtl) = 0;
	virtual HRESULT AddCheckButton(DWORD dwIDCtl, LPCWSTR pszLabel, BOOL bChecked) = 0;
	virtual HRESULT AddEditBox(DWORD dwIDCtl, LPCWSTR pszText) = 0;
	virtual HRESULT AddSeparator(DWORD dwIDCtl) = 0;
	virtual HRESULT AddText(DWORD dwIDCtl, LPCWSTR pszText) = 0;
	virtual HRESULT SetControlLabel(DWORD dwIDCtl, LPCWSTR pszLabel) = 0;
	virtual HRESULT GetControlState(DWORD dwIDCtl, CDCONTROLSTATEF* pdwState) = 0;
	virtual HRESULT SetControlState(DWORD dwIDCtl, CDCONTROLSTATEF dwState) = 0;
	virtual HRESULT GetEditBoxText(DWORD dwIDCtl, WCHAR** ppszText) = 0;
	virtual HRESULT SetEditBoxText(DWORD dwIDCtl, LPCWSTR pszText) = 0;
	virtual HRESULT GetCheckButtonState(DWORD dwIDCtl, BOOL* pbChecked) = 0;
	virtual HRESULT SetCheckButtonState(DWORD dwIDCtl, BOOL bChecked) = 0;
	virtual HRESULT AddControlItem(DWORD dwIDCtl, DWORD dwIDItem, LPCWSTR pszLabel) = 0;
	virtual HRESULT RemoveControlItem(DWORD dwIDCtl, DWORD dwIDItem) = 0;
	virtual HRESULT RemoveAllControlItems(DWORD dwIDCtl) = 0;
	virtual HRESULT GetSelectedControlItem(DWORD dwIDCtl, DWORD* pdwIDItem) = 0;
	virtual HRESULT SetSelectedControlItem(DWORD dwIDCtl, DWORD dwIDItem) = 0;
	virtual HRESULT StartVisualGroup(DWORD dwIDCtl, LPCWSTR pszLabel) = 0;
	virtual HRESULT EndVisualGroup() = 0;
	virtual HRESULT MakeProminent(DWORD dwIDCtl) = 0;
	virtual HRESULT SetControlItemText(DWORD dwIDCtl, DWORD dwIDItem, LPCWSTR pszLabel) = 0;
};

// GUIDs for file dialogs
static const GUID CLSID_FileOpenDialog = {0xDC1C5A9C, 0xE88A, 0x4DDE, {0xA5,0xA1,0x60,0xF8,0x2A,0x20,0xAE,0xF7}};
static const GUID CLSID_FileSaveDialog = {0xC0B4E2F3, 0xBA21, 0x4773, {0x8D,0xBA,0x33,0x5E,0xC9,0x46,0xEB,0x8B}};
static const GUID IID_IFileOpenDialog  = {0xD57C7288, 0xD4AD, 0x4768, {0xBE,0x02,0x9D,0x96,0x95,0x32,0xD9,0x60}};
static const GUID IID_IFileSaveDialog  = {0x84BCCD23, 0x5FDE, 0x4CDB, {0xAE,0xA4,0xAF,0x64,0xB8,0x3D,0x78,0xAB}};
static const GUID IID_IFileDialogEvents = {0x973510DB, 0x7D7F, 0x452B, {0x89,0x75,0x74,0xA8,0x58,0x28,0xD3,0x54}};
static const GUID IID_IFileDialogCustomize = {0xE6FDD21A, 0x163F, 0x4975, {0x9C,0x8C,0xA6,0x9F,0x1B,0xA3,0x70,0x34}};
static const GUID IID_IShellItem       = {0x43826D1E, 0xE718, 0x42EE, {0xBC,0x55,0xA1,0xE2,0x61,0xC3,0x7B,0xFE}};
static const GUID IID_IShellItemArray  = {0xB63EA76D, 0x1F85, 0x456F, {0xA1,0x9C,0x48,0x15,0x9E,0xFA,0x85,0x8B}};

// IID_PPV_ARGS helper
#ifndef IID_PPV_ARGS
template<typename T>
void** IID_PPV_ARGS_Helper(T** pp) { return reinterpret_cast<void**>(pp); }
#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
#endif

// __uuidof emulation for macOS
// Each COM interface maps to its IID via template specialization
template<typename T> struct __uuidof_impl;
#define __uuidof(T) __uuidof_impl<std::remove_pointer_t<std::decay_t<T>>>::value()

#include <type_traits>
template<> struct __uuidof_impl<IShellItem> { static const GUID& value() { return IID_IShellItem; } };
template<> struct __uuidof_impl<IShellItemArray> { static const GUID& value() { return IID_IShellItemArray; } };
template<> struct __uuidof_impl<IFileOpenDialog> { static const GUID& value() { return IID_IFileOpenDialog; } };
template<> struct __uuidof_impl<IFileSaveDialog> { static const GUID& value() { return IID_IFileSaveDialog; } };
template<> struct __uuidof_impl<IFileDialogEvents> { static const GUID& value() { return IID_IFileDialogEvents; } };
template<> struct __uuidof_impl<IFileDialogCustomize> { static const GUID& value() { return IID_IFileDialogCustomize; } };
