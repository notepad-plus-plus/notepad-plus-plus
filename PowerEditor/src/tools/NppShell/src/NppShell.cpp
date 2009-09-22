#include "NppShell.h"
#include "resource.h"

//---------------------------------------------------------------------------
//  Global variables
//---------------------------------------------------------------------------
UINT _cRef = 0; // COM Reference count.
HINSTANCE _hModule = NULL; // DLL Module.

//Menu
TCHAR szNppName[] = TEXT("notepad++.exe");
#ifdef UNICODE
TCHAR szDefaultMenutext[] = TEXT("Edit with &Notepad++");
#else
TCHAR szDefaultMenutext[] = TEXT("Edit with &Notepad++");
#endif

#ifdef WIN64
TCHAR szShellExtensionTitle[] = TEXT("Notepad++64");
#define sz64 TEXT("64")
#else
TCHAR szShellExtensionTitle[] = TEXT("Notepad++");
#define sz64 TEXT("")
#endif

#define szHelpTextA "Edits the selected file(s) with Notepad++"
#define szHelpTextW L"Edits the selected file(s) with Notepad++"
TCHAR szMenuTitle[TITLE_SIZE];
TCHAR szDefaultCustomcommand[] = TEXT("");
//Icon
DWORD isDynamic = 1;
DWORD maxText = 25;
DWORD iconID = 0;
DWORD showIcon = 1;

//Forward function declarations
BOOL RegisterServer();
BOOL UnregisterServer();
void MsgBox(LPTSTR lpszMsg);
void MsgBoxError(LPTSTR lpszMsg);
BOOL CheckNpp(LPCTSTR path);
INT_PTR CALLBACK DlgProcSettings(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef UNICODE
#define _ttoi _wtoi
#else
#define _ttoi atoi
#endif



//Types
struct DOREGSTRUCT {
	HKEY	hRootKey;
	LPTSTR	szSubKey;
	LPTSTR	lpszValueName;
	DWORD	type;
	LPTSTR	szData;
};


//---------------------------------------------------------------------------
// DllMain
//---------------------------------------------------------------------------
extern "C" int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
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

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine) {
	DialogBox(_hModule, MAKEINTRESOURCE(IDD_DIALOG_SETTINGS), NULL, (DLGPROC)&DlgProcSettings);

	return S_OK;
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
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s"),									NULL,					REG_SZ,		szShellExtensionTitle,
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\InprocServer32"),					NULL,					REG_SZ,		szModule,
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\InprocServer32"),					TEXT("ThreadingModel"),	REG_SZ,		TEXT("Apartment"),
		
		//Settings
		// Context menu
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Title"),			REG_SZ,		szDefaultMenutext,
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Path"),			REG_SZ,		szDefaultPath,
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Custom"),			REG_SZ,		szDefaultCustomcommand,
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("ShowIcon"),		REG_DWORD,	(LPTSTR)&showIcon,
		// Icon
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Dynamic"),		REG_DWORD,	(LPTSTR)&isDynamic,
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("Maxtext"),		REG_DWORD,	(LPTSTR)&maxText,
		HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),						TEXT("IconID"),			REG_DWORD,	(LPTSTR)&iconID,

		//Registration
		// Context menu
		HKEY_CLASSES_ROOT,	TEXT("*\\shellex\\ContextMenuHandlers\\Notepad++")sz64,	NULL,					REG_SZ,		szGUID,
		// Icon
		//HKEY_CLASSES_ROOT,	TEXT("Notepad++_file\\shellex\\IconHandler"),		NULL,					REG_SZ,		szGUID,

		NULL,				NULL,												NULL,					REG_SZ,		NULL
	};

	static DOREGSTRUCT IconEntry = {HKEY_CLASSES_ROOT,	TEXT("Notepad++_file\\shellex\\IconHandler"),		NULL,					REG_SZ,		szGUID};

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

	wsprintf(szKeyTemp, TEXT("*\\shellex\\ContextMenuHandlers\\%s"), szShellExtensionTitle);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);

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
void MsgBox(LPTSTR lpszMsg) {
	MessageBox(NULL,
		lpszMsg,
		TEXT("Notepad++ Extension"),
		MB_OK);
}

//---------------------------------------------------------------------------
// MsgBoxError
//---------------------------------------------------------------------------
void MsgBoxError(LPTSTR lpszMsg) {
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

INT_PTR CALLBACK DlgProcSettings(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static TCHAR customCommand[MAX_PATH] = {0};
	static TCHAR customText[TITLE_SIZE] = {0};
	static TCHAR szKeyTemp[MAX_PATH + GUID_STRING_SIZE];

	static DWORD showMenu = 2;	//0 off, 1 on, 2 unknown
	static DWORD showIcon = 2;

	static DWORD isDynamic = 1;	//0 off, 1 on
	static DWORD useMenuIcon = 1;	// 0 off, otherwise on
	static DWORD iconType = 0;	//0 classic, 1 modern

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
				result = RegQueryValueEx(settingKey, TEXT("IconID"), NULL, NULL, (BYTE*)(&iconType), &size);
				if (result != ERROR_SUCCESS) {
					iconType = 0;
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

			Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_CLASSIC), iconType!=0?BST_CHECKED:BST_UNCHECKED);
			Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_MODERN), iconType==0?BST_CHECKED:BST_UNCHECKED);

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

						result = RegSetValueEx(settingKey, TEXT("IconID"), 0, REG_DWORD, (LPBYTE)&iconType, sizeof(DWORD));
						result = RegSetValueEx(settingKey, TEXT("Dynamic"), 0, REG_DWORD, (LPBYTE)&isDynamic, sizeof(DWORD));
						result = RegSetValueEx(settingKey, TEXT("ShowIcon"), 0, REG_DWORD, (LPBYTE)&useMenuIcon, sizeof(DWORD));

						RegCloseKey(settingKey);
					}

					if (showMenu == 1) {
						result = RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT("*\\shellex\\ContextMenuHandlers\\Notepad++")sz64, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &settingKey, NULL);
						if (result == ERROR_SUCCESS) {
							result = RegSetValueEx(settingKey, NULL, 0,REG_SZ, (LPBYTE)szGUID, (lstrlen(szGUID)+1)*sizeof(TCHAR));
							RegCloseKey(settingKey);
						}
					} else if (showMenu == 0) {
						wsprintf(szKeyTemp, TEXT("*\\shellex\\ContextMenuHandlers\\%s")sz64, szShellExtensionTitle);
						RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);
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
				case IDC_RADIO_CLASSIC: {
					int state = Button_GetCheck((HWND)lParam);
					if (state == BST_CHECKED)
						iconType = 1;
					else
						iconType = 0;
					break; }
				case IDC_RADIO_MODERN: {
					int state = Button_GetCheck((HWND)lParam);
					if (state == BST_CHECKED)
						iconType = 0;
					else
						iconType = 1;
					break; }
			}

			return TRUE;
			break; }
		case WM_CLOSE: {
			EndDialog(hwndDlg, 0);
			return TRUE;
			break; }
	}

	return FALSE;
}

// --- CShellExtClassFactory ---
CShellExtClassFactory::CShellExtClassFactory() {
	m_cRef = 0L;
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

STDMETHODIMP CShellExtClassFactory::LockServer(BOOL fLock) {
	return NOERROR;
}

// --- CShellExt ---
CShellExt::CShellExt() {
	TCHAR szKeyTemp [MAX_PATH + GUID_STRING_SIZE];

	m_cRef = 0L;
	m_pDataObj = NULL;
	_cRef++;

	m_useCustom = false;
	m_nameLength = 0;
	m_nameMaxLength = maxText;
	m_isDynamic = false;
	m_iconID = 0;
	m_showIcon = true;
	GetModuleFileName(_hModule, m_szModule, MAX_PATH);

	HKEY settingKey;
	LONG result;
	DWORD size = 0;
	DWORD dyn = 0, siz = 0, id = 0, showicon;

	wsprintf(szKeyTemp, TEXT("CLSID\\%s\\Settings"), szGUID);
	result = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKeyTemp, 0, KEY_READ, &settingKey);
	if (result == ERROR_SUCCESS) {
		size = sizeof(TCHAR)*TITLE_SIZE;
		result = RegQueryValueEx(settingKey, TEXT("Title"), NULL, NULL, (LPBYTE)(m_szMenuTitle), &size);
		if (result != ERROR_SUCCESS) {
			lstrcpyn(m_szMenuTitle, szDefaultMenutext, TITLE_SIZE);
		}

		size = sizeof(DWORD);
		result = RegQueryValueEx(settingKey, TEXT("IconID"), NULL, NULL, (BYTE*)(&id), &size);
		if (result == ERROR_SUCCESS) {
			m_iconID = max(0,id);
		}

		size = sizeof(DWORD);
		result = RegQueryValueEx(settingKey, TEXT("Dynamic"), NULL, NULL, (BYTE*)(&dyn), &size);
		if (result == ERROR_SUCCESS && dyn != 0) {
			m_isDynamic = true;
		}

		size = sizeof(DWORD);
		result = RegQueryValueEx(settingKey, TEXT("Maxtext"), NULL, NULL, (BYTE*)(&siz), &size);
		if (result == ERROR_SUCCESS) {
			m_nameMaxLength = max(0,siz);
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
STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hRegKey) {
	HRESULT hres = 0;
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
STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
	UINT idCmd = idCmdFirst;
	DWORD size = TITLE_SIZE;

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
	
	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = HBMMENU_CALLBACK;
	SetMenuItemInfo(hMenu, nIndex, MF_BYPOSITION, &mii);

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
		}
	}
	return hr;
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax) {
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

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) {

	//Setup popup menu stuff (ownerdrawn)
	DWORD menuIconWidth = GetSystemMetrics(SM_CXMENUCHECK);
	DWORD menuIconHeight = GetSystemMetrics(SM_CYMENUCHECK);
	DWORD menuIconPadding = 2;	//+2 pixels on each side, is this fixed?

	switch(uMsg) {
		case WM_MEASUREITEM: {	//for owner drawn menu
			MEASUREITEMSTRUCT * pmis = (MEASUREITEMSTRUCT*) lParam;

			//Keep 0, as we use the space used for checkmarks and whatnot
			pmis->itemWidth = 0;//menuIconWidth + menuIconPadding;
			pmis->itemHeight = 0;//menuItemHeight + menuIconPadding;

			if (plResult)
				*plResult = TRUE;

			break; }
		case WM_DRAWITEM: {		//for owner drawn menu
			//Assumes proper font already been set
			DRAWITEMSTRUCT * lpdis = (DRAWITEMSTRUCT*) lParam;

			UINT icon = IDI_ICON_NPP_BASE + m_iconID;
			if (icon > IDI_ICON_NPP_MAX)
				icon = IDI_ICON_NPP_MAX;

			if (m_showIcon) {
				HICON nppIcon = (HICON)LoadImage(_hModule, MAKEINTRESOURCE(icon), IMAGE_ICON, menuIconWidth, menuIconHeight, 0);
				if (m_useCustom) {
					HICON customIcon = (HICON)LoadImage(NULL, m_szCustomPath, IMAGE_ICON, menuIconWidth, menuIconHeight, LR_DEFAULTCOLOR|LR_LOADFROMFILE);
					if (customIcon != NULL) {
						DestroyIcon(nppIcon);
						nppIcon = customIcon;
					}
				}
				DrawIconEx(lpdis->hDC, menuIconPadding, menuIconPadding, nppIcon, menuIconWidth, menuIconHeight, 0, NULL, DI_NORMAL);
				DestroyIcon(nppIcon);
			}

			if (plResult)
				*plResult = TRUE;

			break; }
	}

	return S_OK;
}

// *** IPersistFile methods ***
HRESULT STDMETHODCALLTYPE CShellExt::Load(LPCOLESTR pszFileName, DWORD dwMode) {
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
	int copySize = min(m_nameMaxLength+1, MAX_PATH);	//+1 to take zero terminator in account
	lstrcpyn(m_szFilePath, ext, copySize);
	m_nameLength = lstrlen(m_szFilePath);
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
		LPTSTR stringEnd = szIconFile+len;
		lstrcpyn(szIconFile, m_szFilePath, cchMax-len);
	}
	*piIndex = 0;
	*pwFlags |= GIL_NOTFILENAME;//|GIL_DONTCACHE|GIL_PERINSTANCE;

	return S_OK;
}

STDMETHODIMP CShellExt::Extract(LPCTSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize) {
	WORD sizeSmall = HIWORD(nIconSize);
	WORD sizeLarge = LOWORD(nIconSize);
	ICONINFO iconinfo;

	int iconID = IDI_ICON_NPP_BASE + m_iconID;
	if (iconID > IDI_ICON_NPP_MAX)
		iconID = IDI_ICON_NPP_BASE;

	HICON iconSmall = (HICON)LoadImage(_hModule, MAKEINTRESOURCE(iconID), IMAGE_ICON, sizeSmall, sizeSmall, LR_DEFAULTCOLOR);
	HICON iconLarge = (HICON)LoadImage(_hModule, MAKEINTRESOURCE(iconID), IMAGE_ICON, sizeLarge, sizeLarge, LR_DEFAULTCOLOR);

	*phiconSmall = iconSmall;
	*phiconLarge = iconLarge;

	if (m_useCustom) {
		HICON customSmall = (HICON)LoadImage(NULL, m_szCustomPath, IMAGE_ICON, sizeSmall, sizeSmall, LR_DEFAULTCOLOR|LR_LOADFROMFILE);
		HICON customLarge = (HICON)LoadImage(NULL, m_szCustomPath, IMAGE_ICON, sizeLarge, sizeLarge, LR_DEFAULTCOLOR|LR_LOADFROMFILE);

		if (customSmall != NULL) {
			DestroyIcon(*phiconSmall);
			*phiconSmall = customSmall;
		}
		
		if (customLarge != NULL) {
			DestroyIcon(*phiconLarge);
			*phiconLarge = customLarge;
		}
	}

	if (!m_isDynamic)
		return S_OK;

	HICON newIconLarge;
	HDC dcEditColor, dcEditMask;
	HGDIOBJ oldBitmapColor, oldBitmapMask, oldFontColor;
	HFONT font;
	HBRUSH brush;

	GetIconInfo(*phiconLarge, &iconinfo);
	DestroyIcon(*phiconLarge);

	dcEditColor = CreateCompatibleDC(GetDC(0));
	dcEditMask = CreateCompatibleDC(GetDC(0));
	oldBitmapColor = SelectObject(dcEditColor, iconinfo.hbmColor);
	oldBitmapMask = SelectObject(dcEditMask, iconinfo.hbmMask);

	LONG calSize = (LONG)(sizeLarge*2/5);
	LOGFONT lf = {0};
	lf.lfHeight = min(calSize, 15);	//this is in pixels. Make no larger than 15 pixels (but smaller is allowed for small icons)
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lstrcpyn(lf.lfFaceName, TEXT("Bitstream Vera Sans Mono"), LF_FACESIZE);
	LOGBRUSH lbrush;
	lbrush.lbStyle = BS_SOLID;
	lbrush.lbHatch = 0;
	RECT rect = {0};
	COLORREF backGround = RGB(1, 1, 1);
	COLORREF textColor = RGB(255,255,255);
	//Grab the topleft pixel as the background color
	COLORREF maskBack = GetPixel(dcEditColor, 0, 0);
	if (backGround == maskBack)
		backGround++;	//add one, shouldn't be very visible

	font = CreateFontIndirect(&lf);
	lbrush.lbColor = backGround;
	brush = CreateBrushIndirect(&lbrush);
	oldFontColor = SelectObject(dcEditColor, font);

	SetBkMode(dcEditColor, TRANSPARENT);	//dont clear background when drawing text (doesnt change much, colors are the same)
	SetBkColor(dcEditColor,  backGround);
	SetTextColor(dcEditColor, textColor);

	SIZE stringSize;
	GetTextExtentPoint32(dcEditColor, m_szFilePath, m_nameLength, &stringSize);
	stringSize.cx = min(stringSize.cx, sizeLarge-2);
	stringSize.cy = min(stringSize.cy, sizeLarge-2);

	rect.top = sizeLarge - stringSize.cy - 2;
	rect.left = sizeLarge - stringSize.cx - 1;
	rect.bottom = sizeLarge;
	rect.right = sizeLarge-1;	
	FillRect(dcEditColor, &rect, brush);
	FillRect(dcEditMask, &rect, brush);

	rect.top += 1;
	rect.left -= 1;
	rect.bottom -= 1;
	rect.right += 1;	
	FillRect(dcEditColor, &rect, brush);
	FillRect(dcEditMask, &rect, brush);

	rect.left += 1;
	DrawText(dcEditColor, m_szFilePath, m_nameLength, &rect, DT_BOTTOM|DT_SINGLELINE|DT_LEFT);

	SetBkColor(dcEditColor,  maskBack);
	//BitBlt(dcEditMask, 0, 0, sizeLarge, sizeLarge, dcEditColor, 0, 0, SRCCOPY);

	SelectObject(dcEditColor, oldFontColor);
	SelectObject(dcEditColor, oldBitmapColor);
	SelectObject(dcEditMask, oldBitmapMask);
	DeleteDC(dcEditColor);
	DeleteDC(dcEditMask);
	DeleteBrush(brush);


	newIconLarge = CreateIconIndirect(&iconinfo);
	*phiconLarge = newIconLarge;

	return S_OK;
}

// *** Private methods ***
STDMETHODIMP CShellExt::InvokeNPP(HWND hParent, LPCSTR pszWorkingDir, LPCSTR pszCmd, LPCSTR pszParam, int iShowCmd) {
	TCHAR szFilename[MAX_PATH];
	TCHAR szCustom[MAX_PATH];
	LPTSTR pszCommand;
	size_t bytesRequired = 1;

	TCHAR szKeyTemp[MAX_PATH + GUID_STRING_SIZE];
	DWORD regSize = 0;
	DWORD pathSize = MAX_PATH;
	HKEY settingKey;
	LONG result;

	FORMATETC fmte = {
		CF_HDROP,
		(DVTARGETDEVICE FAR *)NULL,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};

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
	lstrcat(pszCommand, TEXT("\""));
	lstrcat(pszCommand, szFilename);
	lstrcat(pszCommand, TEXT("\""));
	result = RegQueryValueEx(settingKey, TEXT("Custom"), NULL, NULL, (LPBYTE)(szCustom), &pathSize);
	if (result == ERROR_SUCCESS) {
		lstrcat(pszCommand, TEXT(" "));
		lstrcat(pszCommand, szCustom);
	}
	RegCloseKey(settingKey);

	for (UINT i = 0; i < m_cbFiles; i++) {
		DragQueryFile((HDROP)m_stgMedium.hGlobal, i, szFilename, MAX_PATH);
		lstrcat(pszCommand, TEXT(" \""));
		lstrcat(pszCommand, szFilename);
		lstrcat(pszCommand, TEXT("\""));
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = iShowCmd;	//SW_RESTORE;
	if (!CreateProcess (NULL, pszCommand, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		DWORD errorCode = GetLastError();
		TCHAR * message = new TCHAR[512+bytesRequired];
		wsprintf(message, TEXT("Error in CreateProcess (%d): Is this command correct?\r\n%s"), errorCode, pszCommand);
		MsgBoxError(message);
		delete [] message;
	}

	CoTaskMemFree(pszCommand);
	return NOERROR;
}
