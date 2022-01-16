#include "NppShell.h"
#include "Bitmap.h"
#include "resource.h"
#include <shellapi.h>
#include <algorithm>

#ifndef RGBA
#define RGBA(r,g,b,a)        ((COLORREF)( (((DWORD)(BYTE)(a))<<24) |     RGB(r,g,b) ))
#endif



//---------------------------------------------------------------------------
//  Global variables
//---------------------------------------------------------------------------
UINT _cRef = 0; // COM Reference count.
HINSTANCE _hModule = NULL; // DLL Module.

//Some global default values for registering the DLL

//Menu
TCHAR szNppName[] = TEXT("notepad++.exe");
TCHAR szDefaultMenutext[] = TEXT("Edit with &Notepad++");

#ifdef WIN64
TCHAR szShellExtensionTitle[] = TEXT("ANotepad++64");
TCHAR szShellExtensionKey[] = TEXT("*\\shellex\\ContextMenuHandlers\\ANotepad++64");
#else
TCHAR szShellExtensionTitle[] = TEXT("ANotepad++");
TCHAR szShellExtensionKey[] = TEXT("*\\shellex\\ContextMenuHandlers\\ANotepad++");
#endif

#define szHelpTextA "Edits the selected file(s) with Notepad++"
#define szHelpTextW L"Edits the selected file(s) with Notepad++"
TCHAR szMenuTitle[TITLE_SIZE];
TCHAR szDefaultCustomcommand[] = TEXT("");
//Icon
DWORD isDynamic = 1;
DWORD maxText = 25;
DWORD showIcon = 1;

//Forward function declarations
extern "C" int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);

BOOL RegisterServer();
BOOL UnregisterServer();
void MsgBox(LPCTSTR lpszMsg);
void MsgBoxError(LPCTSTR lpszMsg);
BOOL CheckNpp(LPCTSTR path);
intptr_t CALLBACK DlgProcSettings(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InvalidateIcon(HICON * iconSmall, HICON * iconLarge);

#ifdef UNICODE
#define _ttoi _wtoi
#else
#define _ttoi atoi
#endif



//Types
struct DOREGSTRUCT {
	HKEY	hRootKey;
	LPCTSTR	szSubKey;
	LPCTSTR	lpszValueName;
	DWORD	type;
	LPCTSTR	szData;
};

//---------------------------------------------------------------------------
// DllMain
//---------------------------------------------------------------------------
int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		_hModule = hInstance;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void) {
	return (_cRef == 0 ? S_OK : S_FALSE);
}

//---------------------------------------------------------------------------
// DllGetClassObject
//---------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut) {
	*ppvOut = NULL;
	if (IsEqualIID(rclsid, CLSID_ShellExtension)) {
		CShellExtClassFactory *pcf = new CShellExtClassFactory;
		return pcf->QueryInterface(riid, ppvOut);
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}

//---------------------------------------------------------------------------
// DllRegisterServer
//---------------------------------------------------------------------------
STDAPI DllRegisterServer(void) {
	return (RegisterServer() ? S_OK : E_FAIL);
}

//---------------------------------------------------------------------------
// DllUnregisterServer
//---------------------------------------------------------------------------
STDAPI DllUnregisterServer(void) {
	return (UnregisterServer() ? S_OK : E_FAIL);
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR /*pszCmdLine*/) {
	if (bInstall) {
		DialogBox(_hModule, MAKEINTRESOURCE(IDD_DIALOG_SETTINGS), NULL, (DLGPROC)&DlgProcSettings);
		return S_OK;
	} else {
		MsgBoxError(TEXT("Uninstalling not supported, use DllUnregisterServer instead"));
		return E_NOTIMPL;
	}
}

//---------------------------------------------------------------------------
// RegisterServer
// Create registry entries and setup the shell extension
//---------------------------------------------------------------------------
BOOL RegisterServer() {
	int      i;
	HKEY     hKey;
	LRESULT  lResult;
	DWORD    dwDisp;
	TCHAR    szSubKey[MAX_PATH];
	TCHAR    szModule[MAX_PATH];
	TCHAR    szDefaultPath[MAX_PATH];

	GetModuleFileName(_hModule, szDefaultPath, MAX_PATH);
	TCHAR* pDest = StrRChr(szDefaultPath, NULL, TEXT('\\'));
	pDest++;
	pDest[0] = 0;
	lstrcat(szDefaultPath, szNppName);

	if (!CheckNpp(szDefaultPath)) {
		MsgBoxError(TEXT("To register the Notepad++ shell extension properly,\r\nplace NppShell.dll in the same directory as the Notepad++ executable."));
		//return FALSE;
	}

	//get this app's path and file name
	GetModuleFileName(_hModule, szModule, MAX_PATH);

	static DOREGSTRUCT ClsidEntries[] = {
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s"),									NULL,					REG_SZ,		szShellExtensionTitle},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\InprocServer32"),					NULL,					REG_SZ,		szModule},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\InprocServer32"),					TEXT("ThreadingModel"),	REG_SZ,		TEXT("Apartment")},

		//Settings
		// Context menu
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Title"),			REG_SZ,		szDefaultMenutext},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Path"),			REG_SZ,		szDefaultPath},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Custom"),			REG_SZ,		szDefaultCustomcommand},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("ShowIcon"),		REG_DWORD,	(LPTSTR)&showIcon},
		// Icon
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Dynamic"),		REG_DWORD,	(LPTSTR)&isDynamic},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Maxtext"),		REG_DWORD,	(LPTSTR)&maxText},

		//Registration
		// Context menu
		{HKEY_CLASSES_ROOT,	szShellExtensionKey,	NULL,					REG_SZ,		szGUID},
		// Icon
		//{HKEY_CLASSES_ROOT,	TEXT("Notepad++_file\\shellex\\IconHandler"),		NULL,					REG_SZ,		szGUID},

		{NULL,				NULL,												NULL,					REG_SZ,		NULL}
	};

	// First clear any old entries
	UnregisterServer();

	// Register the CLSID entries
	for(i = 0; ClsidEntries[i].hRootKey; i++) {
		wsprintf(szSubKey, ClsidEntries[i].szSubKey, szGUID);
		lResult = RegCreateKeyEx(ClsidEntries[i].hRootKey, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);
		if (NOERROR == lResult) {
			TCHAR szData[MAX_PATH];
			// If necessary, create the value string
			if (ClsidEntries[i].type == REG_SZ) {
				wsprintf(szData, ClsidEntries[i].szData, szModule);
				lResult = RegSetValueEx(hKey, ClsidEntries[i].lpszValueName, 0, ClsidEntries[i].type, (LPBYTE)szData, (lstrlen(szData) + 1) * sizeof(TCHAR));
			} else {
				lResult = RegSetValueEx(hKey, ClsidEntries[i].lpszValueName, 0, ClsidEntries[i].type, (LPBYTE)ClsidEntries[i].szData, sizeof(DWORD));
			}
			RegCloseKey(hKey);
		}
		else
			return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
// UnregisterServer
//---------------------------------------------------------------------------
BOOL UnregisterServer() {
	TCHAR szKeyTemp[MAX_PATH + GUID_STRING_SIZE];

	RegDeleteKey(HKEY_CLASSES_ROOT, szShellExtensionKey);

	wsprintf(szKeyTemp, TEXT("Notepad++_file\\shellex\\IconHandler"));
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);
	wsprintf(szKeyTemp, TEXT("Notepad++_file\\shellex"));
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);

	wsprintf(szKeyTemp, TEXT("CLSID\\%s\\InprocServer32"), szGUID);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);
	wsprintf(szKeyTemp, TEXT("CLSID\\%s\\Settings"), szGUID);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);
	wsprintf(szKeyTemp, TEXT("CLSID\\%s"), szGUID);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);

	return TRUE;
}

//---------------------------------------------------------------------------
// MsgBox
//---------------------------------------------------------------------------
void MsgBox(LPCTSTR lpszMsg) {
	MessageBox(NULL,
		lpszMsg,
		TEXT("Notepad++ Extension"),
		MB_OK);
}

//---------------------------------------------------------------------------
// MsgBoxError
//---------------------------------------------------------------------------
void MsgBoxError(LPCTSTR lpszMsg) {
	MessageBox(NULL,
		lpszMsg,
		TEXT("Notepad++ Extension: Error"),
		MB_OK | MB_ICONWARNING);
}

//---------------------------------------------------------------------------
// CheckNpp
// Check if the shell handler resides in the same directory as notepad++
//---------------------------------------------------------------------------
BOOL CheckNpp(LPCTSTR path) {
	WIN32_FIND_DATA fd;
	HANDLE findHandle;

	findHandle = FindFirstFile(path, &fd);
	if (findHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	} else {
		FindClose(findHandle);
	}
	return TRUE;
}

intptr_t CALLBACK DlgProcSettings(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static TCHAR customCommand[MAX_PATH] = {0};
	static TCHAR customText[TITLE_SIZE] = {0};
	static TCHAR szKeyTemp[MAX_PATH + GUID_STRING_SIZE];

	static DWORD showMenu = 2;	//0 off, 1 on, 2 unknown
	static DWORD useMenuIcon = 1;	// 0 off, otherwise on

	HKEY settingKey;
	LONG result;
	DWORD size = 0;

	switch(uMsg) {
		case WM_INITDIALOG: {
			wsprintf(szKeyTemp, TEXT("CLSID\\%s\\Settings"), szGUID);
			result = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKeyTemp, 0, KEY_READ, &settingKey);
			if (result == ERROR_SUCCESS) {
				size = sizeof(TCHAR)*TITLE_SIZE;
				result = RegQueryValueEx(settingKey, TEXT("Title"), NULL, NULL, (LPBYTE)(customText), &size);
				if (result != ERROR_SUCCESS) {
					lstrcpyn(customText, szDefaultMenutext, TITLE_SIZE);
				}

				size = sizeof(TCHAR)*MAX_PATH;
				result = RegQueryValueEx(settingKey, TEXT("Custom"), NULL, NULL, (LPBYTE)(customCommand), &size);
				if (result != ERROR_SUCCESS) {
					lstrcpyn(customCommand, TEXT(""), MAX_PATH);
				}

				size = sizeof(DWORD);
				result = RegQueryValueEx(settingKey, TEXT("Dynamic"), NULL, NULL, (BYTE*)(&isDynamic), &size);
				if (result != ERROR_SUCCESS) {
					isDynamic = 1;
				}

				size = sizeof(DWORD);
				result = RegQueryValueEx(settingKey, TEXT("ShowIcon"), NULL, NULL, (BYTE*)(&useMenuIcon), &size);
				if (result != ERROR_SUCCESS) {
					useMenuIcon = 1;
				}

				RegCloseKey(settingKey);
			}

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_USECONTEXT), BST_INDETERMINATE);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_USEICON), BST_INDETERMINATE);

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_CONTEXTICON), useMenuIcon?BST_CHECKED:BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK_ISDYNAMIC), isDynamic?BST_CHECKED:BST_UNCHECKED);

			SetDlgItemText(hwndDlg, IDC_EDIT_MENU, customText);
			SetDlgItemText(hwndDlg, IDC_EDIT_COMMAND, customCommand);

			return TRUE;
			break; }
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDOK: {
					//Store settings
					GetDlgItemText(hwndDlg, IDC_EDIT_MENU, customText, TITLE_SIZE);
					GetDlgItemText(hwndDlg, IDC_EDIT_COMMAND, customCommand, MAX_PATH);
					int textLen = lstrlen(customText);
					int commandLen = lstrlen(customCommand);

					wsprintf(szKeyTemp, TEXT("CLSID\\%s\\Settings"), szGUID);
					result = RegCreateKeyEx(HKEY_CLASSES_ROOT, szKeyTemp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &settingKey, NULL);
					if (result == ERROR_SUCCESS) {

						result = RegSetValueEx(settingKey, TEXT("Title"), 0,REG_SZ, (LPBYTE)customText, (textLen+1)*sizeof(TCHAR));
						result = RegSetValueEx(settingKey, TEXT("Custom"), 0,REG_SZ, (LPBYTE)customCommand, (commandLen+1)*sizeof(TCHAR));

						result = RegSetValueEx(settingKey, TEXT("Dynamic"), 0, REG_DWORD, (LPBYTE)&isDynamic, sizeof(DWORD));
						result = RegSetValueEx(settingKey, TEXT("ShowIcon"), 0, REG_DWORD, (LPBYTE)&useMenuIcon, sizeof(DWORD));

						RegCloseKey(settingKey);
					}

					if (showMenu == 1) {
						result = RegCreateKeyEx(HKEY_CLASSES_ROOT, szShellExtensionKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &settingKey, NULL);
						if (result == ERROR_SUCCESS) {
							result = RegSetValueEx(settingKey, NULL, 0,REG_SZ, (LPBYTE)szGUID, (lstrlen(szGUID)+1)*sizeof(TCHAR));
							RegCloseKey(settingKey);
						}
					} else if (showMenu == 0) {
						RegDeleteKey(HKEY_CLASSES_ROOT, szShellExtensionKey);
					}

					if (showIcon == 1) {
						result = RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT("Notepad++_file\\shellex\\IconHandler"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &settingKey, NULL);
						if (result == ERROR_SUCCESS) {
							result = RegSetValueEx(settingKey, NULL, 0,REG_SZ, (LPBYTE)szGUID, (lstrlen(szGUID)+1)*sizeof(TCHAR));
							RegCloseKey(settingKey);
						}
					} else if (showIcon == 0) {
						RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Notepad++_file\\shellex\\IconHandler"));
						RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Notepad++_file\\shellex"));
					}

					PostMessage(hwndDlg, WM_CLOSE, 0, 0);
					break; }
				case IDC_CHECK_USECONTEXT: {
					int state = Button_GetCheck((HWND)lParam);
					if (state == BST_CHECKED)
						showMenu = 1;
					else if (state == BST_UNCHECKED)
						showMenu = 0;
					else
						showMenu = 2;
					break; }
				case IDC_CHECK_USEICON: {
					int state = Button_GetCheck((HWND)lParam);
					if (state == BST_CHECKED)
						showIcon = 1;
					else if (state == BST_UNCHECKED)
						showIcon = 0;
					else
						showIcon = 2;
					break; }
				case IDC_CHECK_CONTEXTICON: {
					int state = Button_GetCheck((HWND)lParam);
					if (state == BST_CHECKED)
						useMenuIcon = 1;
					else
						useMenuIcon = 0;
					break; }
				case IDC_CHECK_ISDYNAMIC: {
					int state = Button_GetCheck((HWND)lParam);
					if (state == BST_CHECKED)
						isDynamic = 1;
					else
						isDynamic = 0;
					break; }
				default:
					break;
			}

			return TRUE;
			break; }
		case WM_CLOSE: {
			EndDialog(hwndDlg, 0);
			return TRUE;
			break; }
		default:
			break;
	}

	return FALSE;
}

// --- CShellExtClassFactory ---
CShellExtClassFactory::CShellExtClassFactory() :
	m_cRef(0L)
{
	_cRef++;
}

CShellExtClassFactory::~CShellExtClassFactory() {
	_cRef--;
}

// *** IUnknown methods ***
STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID riid, LPVOID FAR *ppv) {
	*ppv = NULL;
	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
		*ppv = (LPCLASSFACTORY)this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef() {
	return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
{
	if (--m_cRef)
		return m_cRef;
	delete this;
	return 0L;
}

// *** IClassFactory methods ***
STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) {
	*ppvObj = NULL;
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
	CShellExt * pShellExt = new CShellExt();
	if (!pShellExt)
		return E_OUTOFMEMORY;
	return pShellExt->QueryInterface(riid, ppvObj);
}

STDMETHODIMP CShellExtClassFactory::LockServer(BOOL /*fLock*/) {
	return NOERROR;
}

// --- CShellExt ---
CShellExt::CShellExt() :
	m_cRef(0L),
	m_cbFiles(0),
	m_pDataObj(NULL),
	m_menuID(0),
	m_hMenu(NULL),
	m_showIcon(true),
	m_useCustom(false),
	m_nameLength(0),
	m_nameMaxLength(maxText),
	m_isDynamic(false),
	m_winVer(0),
	m_hBitmap(NULL)
{
	TCHAR szKeyTemp [MAX_PATH + GUID_STRING_SIZE];
	ZeroMemory(&m_stgMedium, sizeof(m_stgMedium));
	_cRef++;

	GetModuleFileName(_hModule, m_szModule, MAX_PATH);

	OSVERSIONINFOEX inf;
	ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
    inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&inf);
	m_winVer = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);

	if (m_winVer >= WINVER_VISTA) {
		InitTheming();
	}

	HKEY settingKey;
	LONG result;
	DWORD size = 0;
	DWORD dyn = 0, siz = 0, showicon = 0;

	wsprintf(szKeyTemp, TEXT("CLSID\\%s\\Settings"), szGUID);
	result = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKeyTemp, 0, KEY_READ, &settingKey);
	if (result == ERROR_SUCCESS) {
		size = sizeof(TCHAR)*TITLE_SIZE;
		result = RegQueryValueEx(settingKey, TEXT("Title"), NULL, NULL, (LPBYTE)(m_szMenuTitle), &size);
		if (result != ERROR_SUCCESS) {
			lstrcpyn(m_szMenuTitle, szDefaultMenutext, TITLE_SIZE);
		}

		size = sizeof(DWORD);
		result = RegQueryValueEx(settingKey, TEXT("Dynamic"), NULL, NULL, (BYTE*)(&dyn), &size);
		if (result == ERROR_SUCCESS && dyn != 0) {
			m_isDynamic = true;
		}

		size = sizeof(DWORD);
		result = RegQueryValueEx(settingKey, TEXT("Maxtext"), NULL, NULL, (BYTE*)(&siz), &size);
		if (result == ERROR_SUCCESS) {
			m_nameMaxLength = std::max((DWORD)0,siz);
		}

		size = sizeof(DWORD);
		result = RegQueryValueEx(settingKey, TEXT("ShowIcon"), NULL, NULL, (BYTE*)(&showicon), &size);
		if (result == ERROR_SUCCESS) {
			m_showIcon = (showicon != 0);
		}

		result = RegQueryValueEx(settingKey, TEXT("CustomIcon"), NULL, NULL, NULL, NULL);
		if (result == ERROR_SUCCESS) {
			m_useCustom = true;
			size = MAX_PATH;
			RegQueryValueEx(settingKey, TEXT("CustomIcon"), NULL, NULL, (BYTE*)m_szCustomPath, &size);
		}

		RegCloseKey(settingKey);
	}
}

CShellExt::~CShellExt() {
	if (m_winVer >= WINVER_VISTA) {
		DeinitTheming();
	}

	if (m_hBitmap != NULL && m_hBitmap != HBMMENU_CALLBACK)
		DeleteBitmap(m_hBitmap);

	if (m_pDataObj)
		m_pDataObj->Release();
	_cRef--;
}
// *** IUnknown methods ***
STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv) {
	*ppv = NULL;
	if (IsEqualIID(riid, IID_IUnknown)) {
		//*ppv = (LPUNKNOWN)this;
		*ppv = (LPSHELLEXTINIT)this;
	} else if (IsEqualIID(riid, IID_IShellExtInit)) {
		*ppv = (LPSHELLEXTINIT)this;
	} else if (IsEqualIID(riid, IID_IContextMenu)) {
		*ppv = (LPCONTEXTMENU)this;
	} else if (IsEqualIID(riid, IID_IContextMenu2)) {
		*ppv = (LPCONTEXTMENU2)this;
	} else if (IsEqualIID(riid, IID_IContextMenu3)) {
		*ppv = (LPCONTEXTMENU3)this;
	} else if (IsEqualIID(riid, IID_IPersistFile)) {
		*ppv = (LPPERSISTFILE)this;
	} else if (IsEqualIID(riid, IID_IExtractIcon)) {
		*ppv = (LPEXTRACTICON)this;
	}
	if (*ppv) {
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef() {
	return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release() {
	if (--m_cRef)
		return m_cRef;
	delete this;
	return 0L;
}

// *** IShellExtInit methods ***
STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST /*pIDFolder*/, LPDATAOBJECT pDataObj, HKEY /*hRegKey*/) {
	if (m_pDataObj) {
		m_pDataObj->Release();
		m_pDataObj = NULL;
	}
	if (pDataObj) {
		m_pDataObj = pDataObj;
		pDataObj->AddRef();
	}
	return NOERROR;
}

// *** IContextMenu methods ***
STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT /*idCmdLast*/, UINT /*uFlags*/) {
	UINT idCmd = idCmdFirst;

	FORMATETC fmte = {
		CF_HDROP,
		(DVTARGETDEVICE FAR *)NULL,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};

	HRESULT hres = m_pDataObj->GetData(&fmte, &m_stgMedium);

	if (SUCCEEDED(hres)) {
		if (m_stgMedium.hGlobal)
			m_cbFiles = DragQueryFile((HDROP)m_stgMedium.hGlobal, (UINT)-1, 0, 0);
	}

	UINT nIndex = indexMenu++;

	InsertMenu(hMenu, nIndex, MF_STRING|MF_BYPOSITION, idCmd++, m_szMenuTitle);


	if (m_showIcon) {
		HBITMAP icon = NULL;
		if (m_winVer >= WINVER_VISTA) {
			icon = NULL;
			HICON hicon;
			DWORD menuIconWidth = GetSystemMetrics(SM_CXMENUCHECK);
			DWORD menuIconHeight = GetSystemMetrics(SM_CYMENUCHECK);
			HRESULT hr = LoadShellIcon(menuIconWidth, menuIconHeight, &hicon);
			if (SUCCEEDED(hr)) {
				icon = IconToBitmapPARGB32(hicon, menuIconWidth, menuIconHeight);
				DestroyIcon(hicon);
			}
		} else {
			icon = HBMMENU_CALLBACK;
		}

		if (icon != NULL) {
			MENUITEMINFO mii;
			ZeroMemory(&mii, sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_BITMAP;
			mii.hbmpItem = icon;
			//mii.hbmpChecked = icon;
			//mii.hbmpUnchecked = icon;

			SetMenuItemInfo(hMenu, nIndex, MF_BYPOSITION, &mii);

			if (m_winVer >= WINVER_VISTA) {
				MENUINFO MenuInfo;
				MenuInfo.cbSize = sizeof(MenuInfo);
				MenuInfo.fMask = MIM_STYLE;
				MenuInfo.dwStyle = MNS_CHECKORBMP;

				SetMenuInfo(hMenu, &MenuInfo);
			}

		}
		m_hBitmap = icon;
	}

	m_hMenu = hMenu;
	m_menuID = idCmd;

	return ResultFromShort(idCmd-idCmdFirst);
}

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) {
	HRESULT hr = E_INVALIDARG;

	if (!HIWORD(lpcmi->lpVerb)) {
		UINT idCmd = LOWORD(lpcmi->lpVerb);
		switch(idCmd) {
			case 0:
				hr = InvokeNPP(lpcmi->hwnd, lpcmi->lpDirectory, lpcmi->lpVerb, lpcmi->lpParameters, lpcmi->nShow);
				break;
			default:
				break;
		}
	}
	return hr;
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR, UINT uFlags, UINT FAR *, LPSTR pszName, UINT cchMax) {
	LPWSTR wBuffer = (LPWSTR) pszName;
	if (uFlags == GCS_HELPTEXTA) {
		lstrcpynA(pszName, szHelpTextA, cchMax);
		return S_OK;
	} else if (uFlags == GCS_HELPTEXTW) {
		lstrcpynW(wBuffer, szHelpTextW, cchMax);
		return S_OK;
	}
	return E_NOTIMPL;
}

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, LRESULT *plResult) {

	//Setup popup menu stuff (ownerdrawn)
	DWORD menuIconWidth = GetSystemMetrics(SM_CXMENUCHECK);
	DWORD menuIconHeight = GetSystemMetrics(SM_CYMENUCHECK);
	DWORD menuIconPadding = 2;	//+1 pixels on each side, is this fixed?

	switch(uMsg) {
		case WM_MEASUREITEM: {	//for owner drawn menu
			MEASUREITEMSTRUCT * lpdis = (MEASUREITEMSTRUCT*) lParam;

			if (lpdis == NULL)// || lpdis->itemID != m_menuID)
				break;

			if (m_showIcon) {
				lpdis->itemWidth = 0;	//0 seems to work for 98 and up
				if (lpdis->itemHeight < menuIconHeight)
					lpdis->itemHeight = menuIconHeight;
			}

			if (plResult)
				*plResult = TRUE;
			break; }
		case WM_DRAWITEM: {		//for owner drawn menu
			//Assumes proper font already been set
			DRAWITEMSTRUCT * lpdis = (DRAWITEMSTRUCT*) lParam;
			if ((lpdis == NULL) || (lpdis->CtlType != ODT_MENU))
				break;

			if (m_showIcon) {
				HICON nppIcon = NULL;

				HRESULT hr = LoadShellIcon(menuIconWidth, menuIconHeight, &nppIcon);

				if (SUCCEEDED(hr)) {
					DrawIconEx(lpdis->hDC, menuIconPadding, menuIconPadding, nppIcon, menuIconWidth, menuIconHeight, 0, NULL, DI_NORMAL);
					DestroyIcon(nppIcon);
				}
			}

			if (plResult)
				*plResult = TRUE;

			break; }
		default:
			break;
	}

	return S_OK;
}

// *** IPersistFile methods ***
HRESULT STDMETHODCALLTYPE CShellExt::Load(LPCOLESTR pszFileName, DWORD /*dwMode*/) {
	LPTSTR file[MAX_PATH];
#ifdef UNICODE
	lstrcpyn((LPWSTR)file, pszFileName, MAX_PATH);
#else
	WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, (LPSTR)file, MAX_PATH, NULL, NULL);
#endif
	m_szFilePath[0] = 0;

	LPTSTR ext = PathFindExtension((LPTSTR)file);
	if (ext[0] == '.') {
		ext++;
	}
	int copySize = std::min(m_nameMaxLength+1, MAX_PATH);	//+1 to take zero terminator in account
	lstrcpyn(m_szFilePath, ext, copySize);
	m_nameLength = lstrlen(m_szFilePath);
	CharUpperBuff(m_szFilePath, m_nameLength);
	return S_OK;
}

// *** IExtractIcon methods ***
STDMETHODIMP CShellExt::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags) {
	*pwFlags = 0;
	if (uFlags & GIL_DEFAULTICON || m_szFilePath[0] == 0 || !m_isDynamic) {	//return regular N++ icon if requested OR the extension is bad OR static icon
		if (!m_useCustom) {
			lstrcpyn(szIconFile, m_szModule, cchMax);
			*piIndex = 0;
		} else {
			lstrcpyn(szIconFile, m_szCustomPath, cchMax);
			*piIndex = 0;
		}
		return S_OK;
	}

	if(cchMax > 0) {
		lstrcpyn(szIconFile, TEXT("NppShellIcon"), cchMax);
		int len = lstrlen(szIconFile);
		lstrcpyn(szIconFile, m_szFilePath, cchMax-len);
	}
	*piIndex = 0;
	*pwFlags |= GIL_NOTFILENAME;//|GIL_DONTCACHE|GIL_PERINSTANCE;

	return S_OK;
}

STDMETHODIMP CShellExt::Extract(LPCTSTR /*pszFile*/, UINT /*nIconIndex*/, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize) {
	WORD sizeSmall = HIWORD(nIconSize);
	WORD sizeLarge = LOWORD(nIconSize);
	ICONINFO iconinfo;
	BOOL res;
	HRESULT hrSmall = S_OK, hrLarge = S_OK;

	if (phiconSmall)
		hrSmall = LoadShellIcon(sizeSmall, sizeSmall, phiconSmall);
	if (phiconLarge)
		hrLarge = LoadShellIcon(sizeLarge, sizeLarge, phiconLarge);

	if (FAILED(hrSmall) || FAILED(hrLarge)) {
		InvalidateIcon(phiconSmall, phiconLarge);
		return S_FALSE;
	}

	if (!m_isDynamic || !phiconLarge || sizeLarge < 32)	//No modifications required
		return S_OK;

	HDC dcEditColor, dcEditMask, dcEditTemp;
	HFONT font;
	HBRUSH brush;
	HPEN pen;
	BITMAPINFO bmi;
    HBITMAP hbm;
    LPDWORD pPix;

	res = GetIconInfo(*phiconLarge, &iconinfo);
	if (!res)
		return S_OK;	//abort, the icon is still valid

	res = DestroyIcon(*phiconLarge);
	if (!res)
		return S_OK;
	else
		*phiconLarge = NULL;

	dcEditColor = CreateCompatibleDC(GetDC(0));
	dcEditMask = CreateCompatibleDC(GetDC(0));
	dcEditTemp = CreateCompatibleDC(GetDC(0));

    // Create temp bitmap to render rectangle to
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = sizeLarge;
    bmi.bmiHeader.biHeight = sizeLarge;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    hbm = CreateDIBSection(dcEditTemp, &bmi, DIB_RGB_COLORS, (VOID**)&pPix, NULL, 0);
    memset(pPix, 0x00FFFFFF, sizeof(DWORD)*sizeLarge*sizeLarge);	//initialize to white pixels, no alpha

	SelectObject(dcEditColor, iconinfo.hbmColor);
	SelectObject(dcEditMask, iconinfo.hbmMask);
	SelectObject(dcEditTemp, hbm);

	LONG calSize = (LONG)(sizeLarge*2/5);

	LOGFONT lf = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}};
	lf.lfHeight = calSize;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lstrcpyn(lf.lfFaceName, TEXT("Courier New"), LF_FACESIZE);
	RECT rectText = {0, 0, 0, 0};
	RECT rectBox = {0, 0, 0, 0};
	COLORREF backGround = RGB(1, 1, 60);
	COLORREF textColor = RGB(250,250,250);

	font = CreateFontIndirect(&lf);
	brush = CreateSolidBrush(backGround);
	pen = CreatePen(PS_NULL, 0, backGround);
	SelectObject(dcEditTemp, font);
	SelectObject(dcEditTemp, brush);
	SelectObject(dcEditTemp, pen);
	SetBkMode(dcEditTemp, TRANSPARENT);	//dont clear background when drawing text
	SetBkColor(dcEditTemp,  backGround);
	SetTextColor(dcEditTemp, textColor);

	//Calculate size of the displayed string
	SIZE stringSize;
	GetTextExtentPoint32(dcEditTemp, m_szFilePath, m_nameLength, &stringSize);
	stringSize.cx = std::min(stringSize.cx, (LONG)sizeLarge-2);
	stringSize.cy = std::min(stringSize.cy, (LONG)sizeLarge-2);

	rectText.top = sizeLarge - stringSize.cy - 1;
	rectText.left = sizeLarge - stringSize.cx - 1;
	rectText.bottom = sizeLarge - 1;
	rectText.right = sizeLarge - 1;

	rectBox.top = sizeLarge - stringSize.cy - 2;
	rectBox.left = sizeLarge - stringSize.cx - 2;
	rectBox.bottom = sizeLarge;
	rectBox.right = sizeLarge;

	//Draw the background (rounded) rectangle
	int elipsSize = calSize/3;
	RoundRect(dcEditTemp, rectBox.left, rectBox.top, rectBox.right, rectBox.bottom, elipsSize, elipsSize);
	//Draw text in the rectangle
	DrawText(dcEditTemp, m_szFilePath, m_nameLength, &rectText, DT_BOTTOM|DT_SINGLELINE|DT_LEFT);

	//set alpha of non white pixels back to 255
	//premultiply alpha
	//Fill in the mask bitmap (anything not 100% alpha is transparent)
	int red, green, blue, alpha;
	for(int y = 0; y < sizeLarge; y++) {
		for(int x = 0; x < sizeLarge; x++) {
			DWORD * pix = pPix+(y*sizeLarge+x);
			red = *pix & 0xFF;
			green = *pix >> 8 & 0xFF;
			blue = *pix >> 16 & 0xFF;
			alpha = *pix >> 24 & 0xFF;
			if ((*pix << 8) == 0xFFFFFF00)
				alpha = 0x00;
			else
				alpha = 0xFF;
			red = (red*alpha)/0xFF;
			green = (green*alpha)/0xFF;
			blue = (blue*alpha)/0xFF;
			*pix = RGBA(red, green, blue, alpha);
		}
	}

	BLENDFUNCTION ftn = { AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA };
	int width = rectBox.right - rectBox.left;
	int height = rectBox.bottom - rectBox.top;
	AlphaBlend(dcEditColor, rectBox.left, rectBox.top, stringSize.cx, stringSize.cy, dcEditTemp, rectBox.left, rectBox.top, width, height, ftn);

	//Adjust the mask image: simply draw the rectangle to it
	backGround = RGB(0, 0, 0);
	DeleteBrush(brush);
	DeletePen(pen);
	brush = CreateSolidBrush(backGround);
	pen = CreatePen(PS_NULL, 0, backGround);
	SelectObject(dcEditMask, brush);
	SelectObject(dcEditMask, pen);
	RoundRect(dcEditMask, rectBox.left, rectBox.top, rectBox.right, rectBox.bottom, elipsSize, elipsSize);


	DeleteDC(dcEditColor);
	DeleteDC(dcEditMask);
	DeleteDC(dcEditTemp);
	DeleteBrush(brush);
	DeletePen(pen);
	DeleteFont(font);
	DeleteBitmap(hbm);

	*phiconLarge = CreateIconIndirect(&iconinfo);
	DeleteBitmap(iconinfo.hbmColor);
	DeleteBitmap(iconinfo.hbmMask);

	if (*phiconLarge == NULL) {
		InvalidateIcon(phiconSmall, phiconLarge);
		return S_FALSE;
	}

	return S_OK;
}

void InvalidateIcon(HICON * iconSmall, HICON * iconLarge) {
	if (iconSmall && *iconSmall) {
		DestroyIcon(*iconSmall);
		*iconSmall = NULL;
	}
	if (iconLarge && *iconLarge) {
		DestroyIcon(*iconLarge);
		*iconLarge = NULL;
	}
}

// *** Private methods ***
STDMETHODIMP CShellExt::InvokeNPP(HWND /*hParent*/, LPCSTR /*pszWorkingDir*/, LPCSTR /*pszCmd*/, LPCSTR /*pszParam*/, int iShowCmd) {
	TCHAR szFilename[MAX_PATH];
	TCHAR szCustom[MAX_PATH];
	TCHAR szNotepadExecutableFilename[3 * MAX_PATH]; // Should be able to contain szFilename plus szCustom plus some additional characters.
	LPTSTR pszCommand;
	size_t bytesRequired = 1;

	memset(szNotepadExecutableFilename, 0, sizeof(TCHAR) * 3 * MAX_PATH);

	TCHAR szKeyTemp[MAX_PATH + GUID_STRING_SIZE];
	DWORD regSize = 0;
	DWORD pathSize = MAX_PATH;
	HKEY settingKey;
	LONG result;

	wsprintf(szKeyTemp, TEXT("CLSID\\%s\\Settings"), szGUID);
	result = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKeyTemp, 0, KEY_READ, &settingKey);
	if (result != ERROR_SUCCESS) {
		MsgBoxError(TEXT("Unable to open registry key."));
		return E_FAIL;
	}

	result = RegQueryValueEx(settingKey, TEXT("Path"), NULL, NULL, NULL, &regSize);
	if (result == ERROR_SUCCESS) {
		bytesRequired += regSize+2;
	} else {
		MsgBoxError(TEXT("Cannot read path to executable."));
		RegCloseKey(settingKey);
		return E_FAIL;
	}

	result = RegQueryValueEx(settingKey, TEXT("Custom"), NULL, NULL, NULL, &regSize);
	if (result == ERROR_SUCCESS) {
		bytesRequired += regSize;
	}

	for (UINT i = 0; i < m_cbFiles; i++) {
		bytesRequired += DragQueryFile((HDROP)m_stgMedium.hGlobal, i, NULL, 0);
		bytesRequired += 3;
	}

	bytesRequired *= sizeof(TCHAR);
	pszCommand = (LPTSTR)CoTaskMemAlloc(bytesRequired);
	if (!pszCommand) {
		MsgBoxError(TEXT("Insufficient memory available."));
		RegCloseKey(settingKey);
		return E_FAIL;
	}
	*pszCommand = 0;

	regSize = (DWORD)MAX_PATH*sizeof(TCHAR);
	result = RegQueryValueEx(settingKey, TEXT("Path"), NULL, NULL, (LPBYTE)(szFilename), &regSize);
	szFilename[MAX_PATH-1] = 0;
	lstrcat(szNotepadExecutableFilename, TEXT("\""));
	lstrcat(szNotepadExecutableFilename, szFilename);
	lstrcat(szNotepadExecutableFilename, TEXT("\""));
	result = RegQueryValueEx(settingKey, TEXT("Custom"), NULL, NULL, (LPBYTE)(szCustom), &pathSize);
	if (result == ERROR_SUCCESS) {
		lstrcat(szNotepadExecutableFilename, TEXT(" "));
		lstrcat(szNotepadExecutableFilename, szCustom);
	}
	RegCloseKey(settingKey);

	// We have to open the files in batches. A command on the command-line can be at most
	// 2048 characters in XP and 32768 characters in Win7. In the degenerate case where all
	// paths are of length MAX_PATH, we can open at most x files at once, where:
	// 260 * (x + 2) = 2048 or 32768 <=> x = 5 or x = 124.
	// Note the +2 to account for the path to notepad++.exe.
	// http://stackoverflow.com/questions/3205027/maximum-length-of-command-line-string

	const UINT kiBatchSize = m_winVer > WINVER_XP ? 100 : 4;

	UINT iFileIndex = 0;
	while(iFileIndex < m_cbFiles) {
		memset(pszCommand, 0, bytesRequired);
		lstrcat(pszCommand, szNotepadExecutableFilename);
		for (UINT iBatchSizeCounter = 0; iFileIndex < m_cbFiles && iBatchSizeCounter < kiBatchSize; iBatchSizeCounter++) {
			DragQueryFile((HDROP)m_stgMedium.hGlobal, iFileIndex, szFilename, MAX_PATH);
			lstrcat(pszCommand, TEXT(" \""));
			lstrcat(pszCommand, szFilename);
			lstrcat(pszCommand, TEXT("\""));
			iFileIndex++;
		}

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = (WORD)iShowCmd;	//SW_RESTORE;
		if (!CreateProcess (NULL, pszCommand, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			DWORD errorCode = GetLastError();
			if (errorCode == ERROR_ELEVATION_REQUIRED) {	//Fallback to shellexecute
				CoInitializeEx(NULL, 0);
				HINSTANCE execVal = ShellExecute(NULL, TEXT("runas"), pszCommand, NULL, NULL, iShowCmd);
				CoUninitialize();
				if (execVal <= (HINSTANCE)32) {
					TCHAR * message = new TCHAR[512+bytesRequired];
					wsprintf(message, TEXT("ShellExecute failed (%d): Is this command correct?\r\n%s"), execVal, pszCommand);
					MsgBoxError(message);
					delete [] message;
				}
			} else {
				TCHAR * message = new TCHAR[512+bytesRequired];
				wsprintf(message, TEXT("Error in CreateProcess (%d): Is this command correct?\r\n%s"), errorCode, pszCommand);
				MsgBoxError(message);
				delete [] message;
			}
		}
	}



	CoTaskMemFree(pszCommand);
	return NOERROR;
}

STDMETHODIMP CShellExt::LoadShellIcon(int cx, int cy, HICON * phicon) {
	HRESULT hr = E_OUTOFMEMORY;
	HICON hicon = NULL;

	if (m_useCustom) {
		hicon = (HICON)LoadImage(NULL, m_szCustomPath, IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	}

	//Either no custom defined, or failed and use fallback
	if (hicon == NULL) {
		hicon = (HICON)LoadImage(_hModule, MAKEINTRESOURCE(IDI_ICON_NPP), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
	}

	if (hicon == NULL) {
		hr = E_OUTOFMEMORY;
		*phicon = NULL;
	} else {
		hr = S_OK;
		*phicon = hicon;
	}

	return hr;
}
