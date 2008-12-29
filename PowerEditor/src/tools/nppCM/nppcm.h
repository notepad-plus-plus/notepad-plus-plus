//---------------------------------------------------------------------------
// Copyright 2002-2003 by Andre Burgaud <andre@burgaud.com>
// See license.txt
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// wscitecm.h
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Modified by Don HO <don.h@free.fr>
// to meet the need of Notepad++
//---------------------------------------------------------------------------

// {120b94b5-2e6a-4f13-94d0-414bcb64fa0f}
DEFINE_GUID(CLSID_ShellExtension, 0x120b94b5, 0x2e6a, 0x4f13, 0x94, 0xd0, 0x41, 0x4b, 0xcb, 0x64, 0xfa, 0x0f);


class CShellExtClassFactory : public IClassFactory {
protected:
  ULONG m_cRef;

public:
  CShellExtClassFactory();
  ~CShellExtClassFactory();

  STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();
  STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
  STDMETHODIMP LockServer(BOOL);
};
typedef CShellExtClassFactory *LPCSHELLEXTCLASSFACTORY;

class CShellExt : public IContextMenu, IShellExtInit {
public:
protected:
  ULONG m_cRef;
  UINT m_cbFiles;
  STGMEDIUM m_stgMedium;
  LPDATAOBJECT m_pDataObj;
  HBITMAP m_hNppBmp;
  LPMALLOC m_pAlloc;
  TCHAR m_szDllDir [MAX_PATH];

  STDMETHODIMP InvokeNpp(HWND hParent,
    LPCSTR pszWorkingDir,
    LPCSTR pszCmd,
    LPCSTR pszParam,
    int iShowCmd);

public:
  CShellExt();
  ~CShellExt();

  STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  STDMETHODIMP QueryContextMenu(HMENU hMenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags);

  STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);

  STDMETHODIMP GetCommandString(UINT idCmd,
    UINT uFlags,
    UINT FAR *reserved,
    LPSTR pszName,
    UINT cchMax);

  STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder,
    LPDATAOBJECT pDataObj,
    HKEY hKeyID);
};

typedef CShellExt *LPCSHELLEXT;
