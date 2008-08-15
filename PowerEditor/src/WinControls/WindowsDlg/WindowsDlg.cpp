#include <windows.h>
#include "WindowsDlg.h"
#include "WindowsDlgRc.h"
#include "DocTabView.h"
#include <algorithm>
#include <functional>
#include <vector>
#include "UniConversion.h"

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER     0x00010000
#endif

static const wchar_t *readonlyString = L" [Read Only]";
const UINT WDN_NOTIFY = RegisterWindowMessage("WDN_NOTIFY");

inline static DWORD GetStyle(HWND hWnd) { 
	return (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE); 
}
inline static DWORD GetExStyle(HWND hWnd) { 
	return (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE); 
}

inline static BOOL ModifyStyle(HWND hWnd, DWORD dwRemove, DWORD dwAdd) {
	DWORD dwStyle = ::GetWindowLongPtr(hWnd, GWL_STYLE);
	DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if(dwStyle == dwNewStyle)
		return FALSE;
	::SetWindowLongPtr(hWnd, GWL_STYLE, dwNewStyle);
	return TRUE;
}

inline static BOOL ModifyStyleEx(HWND hWnd, DWORD dwRemove, DWORD dwAdd) {
	DWORD dwStyle = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if(dwStyle == dwNewStyle)
		return FALSE;
	::SetWindowLongPtr(hWnd, GWL_EXSTYLE, dwNewStyle);
	return TRUE;
}


struct NumericStringEquivalence
{
	bool operator()(const char* s1, const char* s2) const
	{ return numstrcmp(s1, s2) < 0; }
	static inline int numstrcmp_get(const char **str, int *length)
	{
		const char *p = *str;
		int value = 0;
		for (*length = 0; isdigit(*p); (*length)++)
			value = value * 10 + *p++ - '0';
		*str = p;
		return (value);
	}
	static int numstrcmp(const char *str1, const char *str2)
	{
		char *p1, *p2;
		int c1, c2, lcmp;
		for(;;)
		{
			c1 = tolower(*str1), c2 = tolower(*str2);
			if ( c1 == 0 || c2 == 0 )
				break;
			else if (isdigit(c1) && isdigit(c2))
			{			
				lcmp = strtol(str1, &p1, 10) - strtol(str2, &p2, 10);
				if ( lcmp == 0 )
					lcmp = (p2 - str2) - (p1 - str1);
				if ( lcmp != 0 )
					return (lcmp > 0 ? 1 : -1);
				str1 = p1, str2 = p2;
			}
			else
			{
				lcmp = (c1 - c2);
				if (lcmp != 0)
					return (lcmp > 0 ? 1 : -1);
				++str1, ++str2;
			}
		}
		lcmp = (c1 - c2);
		return ( lcmp < 0 ) ? -1 : (lcmp > 0 ? 1 : 0);
	}
};

struct BufferEquivalent
{
	NumericStringEquivalence _strequiv;
	DocTabView *_pTab;
	int _iColumn;
	bool _reverse;
	BufferEquivalent(DocTabView *pTab, int iColumn, bool reverse) 
		: _pTab(pTab), _iColumn(iColumn), _reverse(reverse)
	{}

	bool operator()(int i1, int i2) const
	{
		if (i1 == i2) return false; // equivalence test not equality
		if (_reverse) std::swap(i1, i2);
		return compare(i1, i2);
	}

	bool compare(int i1, int i2) const
	{ 
		BufferID bid1 = _pTab->getBufferByIndex(i1);
		BufferID bid2 = _pTab->getBufferByIndex(i2);
		Buffer * b1 = MainFileManager->getBufferByID(bid1);
		Buffer * b2 = MainFileManager->getBufferByID(bid2);
		if (_iColumn == 0)
		{
			const char *s1 = b1->getFileName();
			const char *s2 = b2->getFileName();
			return _strequiv(s1, s2);
		}
		else if (_iColumn == 1)
		{
			const char *s1 = b1->getFilePath();
			const char *s2 = b2->getFilePath();
			return _strequiv(s1, s2);	//we can compare the full path to sort on directory, since after sorting directories sorting files is the second thing to do (if directories are the same that is)
		}
		else if (_iColumn == 2)
		{
			int t1 = (int)b1->getLangType();
			int t2 = (int)b2->getLangType();
			return (t1 < t2); // yeah should be the name 
		}
		return false;
	}
};

//////////////////
// Window map tells CWinMgr how to position dialog controls
//
BEGIN_WINDOW_MAP(WindowsDlgMap)
	BEGINROWS(WRCT_REST,0,RCMARGINS(8,8))
		BEGINCOLS(WRCT_REST,0,0)                       // Begin list control column
			BEGINROWS(WRCT_REST,0,0)
				RCREST(IDC_WINDOWS_LIST)
				RCSPACE(20)
			ENDGROUP()
			RCSPACE(12)
			BEGINROWS(WRCT_TOFIT,0,0)
			RCSPACE(12)
			RCTOFIT(IDOK)
			RCSPACE(-12)
			RCTOFIT(IDC_WINDOWS_SAVE)
			RCSPACE(-12)
			RCTOFIT(IDC_WINDOWS_CLOSE)
			RCSPACE(-12)
			RCTOFIT(IDC_WINDOWS_SORT)
			RCREST(-1)
			RCTOFIT(IDCANCEL)
			ENDGROUP()
		ENDGROUP()
	ENDGROUP()
END_WINDOW_MAP()

RECT WindowsDlg::_lastKnownLocation;

WindowsDlg::WindowsDlg() : MyBaseClass(WindowsDlgMap), _isSorted(false)
{
	_szMinButton = SIZEZERO;
	_szMinListCtrl = SIZEZERO;
}

void WindowsDlg::init(HINSTANCE hInst, HWND parent, DocTabView *pTab)
{
	MyBaseClass::init(hInst, parent);
	_pTab = pTab;
}

void WindowsDlg::init(HINSTANCE hInst, HWND parent)
{
	assert(!"Call other initialize method");
	MyBaseClass::init(hInst, parent);
	_pTab = NULL;
}

BOOL CALLBACK WindowsDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			changeDlgLang();
			return MyBaseClass::run_dlgProc(message, wParam, lParam);	
		}
		case WM_COMMAND : 
		{
			switch (wParam)
			{
			case IDOK :
				activateCurrent();
				return TRUE;

			case IDCANCEL :
				::GetWindowRect(_hSelf, &_lastKnownLocation);
				EndDialog(_hSelf, IDCANCEL);
				return TRUE;

			case IDC_WINDOWS_SAVE:
				doSave();
				return TRUE;

			case IDC_WINDOWS_CLOSE:
				doClose();
				return TRUE;

			case IDC_WINDOWS_SORT:
				doSortToTabs();
				_isSorted = false;
				updateButtonState();
				break;

			default :
				break;
			}
		}

		case WM_DESTROY :
			//destroy();
			return TRUE;

		case WM_NOTIFY :
		{		
			if (wParam == IDC_WINDOWS_LIST)
			{
				NMHDR* pNMHDR = (NMHDR*)lParam;
				if (pNMHDR->code == LVN_GETDISPINFOW)
				{
					NMLVDISPINFOW *pLvdi = (NMLVDISPINFOW *)pNMHDR;
					//if(pLvdi->item.mask & LVIF_IMAGE)
					//	; 
					if(pLvdi->item.mask & LVIF_TEXT)
					{
						pLvdi->item.pszText[0] = 0;
						int index = pLvdi->item.iItem;
						if (index >= _pTab->nbItem() || index >= (int)_idxMap.size())
							return FALSE;
						index = _idxMap[index];

						//const Buffer& buffer = _pView->getBufferAt(index);
						BufferID bufID = _pTab->getBufferByIndex(index);
						Buffer * buf = MainFileManager->getBufferByID(bufID);
						if (pLvdi->item.iSubItem == 0) // file name
						{
							int len = pLvdi->item.cchTextMax;
							const wchar_t *fileName = buf->getFileNameW();
							wcsncpy(pLvdi->item.pszText, fileName, len-1);
							pLvdi->item.pszText[len-1] = 0;
							len = wcslen(pLvdi->item.pszText);
							if (buf->isDirty())
							{
								if (len < pLvdi->item.cchTextMax)
								{
									pLvdi->item.pszText[len++] = '*';
									pLvdi->item.pszText[len] = 0;
								}
							}
							else if (buf->isReadOnly())
							{
								len += wcslen(readonlyString);
								if (len <= pLvdi->item.cchTextMax)
									wcscat(pLvdi->item.pszText, readonlyString);
							}
						}
						else if (pLvdi->item.iSubItem == 1) // directory
						{
							const wchar_t *fullName = buf->getFilePathW();
							const wchar_t *fileName = buf->getFileNameW();
							int len = wcslen(fullName)-wcslen(fileName);
							if (!len) {
								len = 1;
								fullName = L"";
							}
							if (pLvdi->item.cchTextMax < len)
								len = pLvdi->item.cchTextMax;
							wcsncpy(pLvdi->item.pszText, fullName, len-1);
							pLvdi->item.pszText[len-1] = 0;
						}
						else if (pLvdi->item.iSubItem == 2) // Type
						{
							int len = pLvdi->item.cchTextMax;
							NppParameters *pNppParameters = NppParameters::getInstance();
							Lang *lang = pNppParameters->getLangFromID(buf->getLangType());
							if (NULL != lang)
							{
								wchar_t langName[LANG_NAME_LEN];
								char2wchar(lang->getLangName(), langName);
								wcsncpy(pLvdi->item.pszText, langName, len-1);
							}
						}
					}
					return TRUE;
				}
				else if (pNMHDR->code == LVN_COLUMNCLICK) // sort columns with stable sort
				{
					NMLISTVIEW *pNMLV = (NMLISTVIEW *)pNMHDR;
					if (pNMLV->iItem == -1)
					{
						bool reverse = false;
						int iColumn = pNMLV->iSubItem;
						if (_lastSort == iColumn)
						{
							reverse = true;
							_lastSort = -1;
						}
						else
						{
							_lastSort = iColumn;
						}
						int i;
						int n = _idxMap.size();
						vector<int> sortMap;
						sortMap.resize(n);
						for (i=0; i<n; ++i) sortMap[_idxMap[i]] = ListView_GetItemState(_hList, i, LVIS_SELECTED);
						stable_sort(_idxMap.begin(), _idxMap.end(), BufferEquivalent(_pTab, iColumn, reverse));
						for (i=0; i<n; ++i) ListView_SetItemState(_hList, i, sortMap[_idxMap[i]] ? LVIS_SELECTED : 0, LVIS_SELECTED);

						::InvalidateRect(_hList, &_rc, FALSE);
						_isSorted = true;
						updateButtonState();
					}
					return TRUE;
				}
				else if (pNMHDR->code == LVN_ITEMACTIVATE || pNMHDR->code == LVN_ITEMCHANGED || pNMHDR->code == LVN_ODSTATECHANGED)
				{
					updateButtonState();
					return TRUE;
				}
				else if (pNMHDR->code == NM_DBLCLK)
				{
					::PostMessage(_hSelf, WM_COMMAND, IDOK, 0);
					//activateCurrent();
					return TRUE;
				}
				else if (pNMHDR->code == LVN_KEYDOWN)
				{
					NMLVKEYDOWN *lvkd = (NMLVKEYDOWN *)pNMHDR;
					// Ctrl+A
					short ctrl = GetKeyState(VK_CONTROL);
					short alt = GetKeyState(VK_MENU);
					short shift = GetKeyState(VK_SHIFT);
					if (lvkd->wVKey == 0x41/*a*/ && ctrl<0 && alt>=0 && shift>=0)
					{
						for (int i=0, n=ListView_GetItemCount(_hList); i<n; ++i)
							ListView_SetItemState(_hList, i, LVIS_SELECTED, LVIS_SELECTED);
					}
					return TRUE;
				}
			}
		}
		break;
	}
	return MyBaseClass::run_dlgProc(message, wParam, lParam);	
}

void WindowsDlg::updateButtonState()
{
	int nSelection = ListView_GetSelectedCount(_hList);
	if (nSelection == 0)
	{
		EnableWindow(GetDlgItem(_hSelf, IDOK), FALSE);
		EnableWindow(GetDlgItem(_hSelf, IDC_WINDOWS_SAVE), FALSE);
		EnableWindow(GetDlgItem(_hSelf, IDC_WINDOWS_CLOSE), FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(_hSelf, IDC_WINDOWS_SAVE), TRUE);
		EnableWindow(GetDlgItem(_hSelf, IDC_WINDOWS_CLOSE), TRUE);
		if (nSelection == 1)
			EnableWindow(GetDlgItem(_hSelf, IDOK), TRUE);
		else
			EnableWindow(GetDlgItem(_hSelf, IDOK), FALSE);
	}
	EnableWindow(GetDlgItem(_hSelf, IDC_WINDOWS_SORT), _isSorted);
}

int WindowsDlg::doDialog(TiXmlNode *dlgNode)
{
	_dlgNode = dlgNode;
	return ::DialogBoxParamW(_hInst, MAKEINTRESOURCEW(IDD_WINDOWS), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
};

bool WindowsDlg::changeDlgLang()
{
	if (!_dlgNode) return false;

	// Set Title
	const char *titre = (_dlgNode->ToElement())->Attribute("title");
	if (titre && titre[0])
	{
		::SetWindowText(_hSelf, titre);
	}

	// Set the text of child control
	for (TiXmlNode *childNode = _dlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(_hSelf, id);
			if (hItem)
				::SetWindowText(hItem, name);
		}
	}
	return true;
}

BOOL WindowsDlg::onInitDialog()
{
	_winMgr.InitToFitSizeFromCurrent(_hSelf);

	// save min size for OK/Cancel buttons
	_szMinButton = RectToSize(_winMgr.GetRect(IDOK));
	_szMinListCtrl = RectToSize(_winMgr.GetRect(IDC_WINDOWS_LIST));
	_lastSort = -1;

	_winMgr.CalcLayout(_hSelf);
	_winMgr.SetWindowPositions(_hSelf);
	getClientRect(_rc);

	_hList = ::GetDlgItem(_hSelf, IDC_WINDOWS_LIST);
	DWORD exStyle = ListView_GetExtendedListViewStyle(_hList);
	exStyle |= LVS_EX_HEADERDRAGDROP|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER;
	ListView_SetExtendedListViewStyle(_hList, exStyle);
	RECT rc;
	GetClientRect(_hList, &rc);
	LONG width = rc.right - rc.left;

	LVCOLUMNW lvColumn;
	memset(&lvColumn, 0, sizeof(lvColumn));
	lvColumn.mask = LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = L"Name";
	lvColumn.cx = width/4;
	SendMessage(_hList, LVM_INSERTCOLUMNW, 0, LPARAM(&lvColumn));

	lvColumn.pszText = L"Path";
	lvColumn.cx = 300;
	SendMessage(_hList, LVM_INSERTCOLUMNW, 1, LPARAM(&lvColumn));

	lvColumn.fmt = LVCFMT_CENTER;
	lvColumn.pszText = L"Type";
	lvColumn.cx = 40;
	SendMessage(_hList, LVM_INSERTCOLUMNW, 2, LPARAM(&lvColumn));

	fitColumnsToSize();

	if (_lastKnownLocation.bottom > 0 && _lastKnownLocation.right > 0)
	{
		SetWindowPos(_hSelf, NULL, _lastKnownLocation.left, _lastKnownLocation.top, 
			_lastKnownLocation.right-_lastKnownLocation.left, _lastKnownLocation.bottom-_lastKnownLocation.top, SWP_SHOWWINDOW);
	}
	else
	{
		goToCenter();
	}

	doRefresh(true);
	return TRUE;
}

void WindowsDlg::onSize(UINT nType, int cx, int cy)
{
	MyBaseClass::onSize(nType, cx, cy);
	fitColumnsToSize();
}

void WindowsDlg::onGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	MyBaseClass::onGetMinMaxInfo(lpMMI);
}

LRESULT WindowsDlg::onWinMgr(WPARAM wp, LPARAM lp)
{
	NMWINMGR &nmw = *(NMWINMGR *)lp;
	if (nmw.code==NMWINMGR::GET_SIZEINFO) {
		switch(wp)
		{
		case IDOK:
		case IDCANCEL:
		case IDC_WINDOWS_SAVE:
		case IDC_WINDOWS_CLOSE:
		case IDC_WINDOWS_SORT:
			nmw.sizeinfo.szMin = _szMinButton;
			nmw.processed = TRUE;
			return TRUE;

		case IDC_WINDOWS_LIST:
			nmw.sizeinfo.szMin = _szMinListCtrl;
			nmw.processed = TRUE;
			return TRUE;
		}	
	}
	return MyBaseClass::onWinMgr(wp, lp);
}

void WindowsDlg::doRefresh(bool invalidate /*= false*/)
{
	if (_hSelf != NULL && isVisible())
	{
		if (_hList != NULL)
		{
			size_t count = (_pTab != NULL) ? _pTab->nbItem() : 0;
			size_t oldSize = _idxMap.size();
			if (!invalidate && count == oldSize)
				return;

			if (count != oldSize)
			{
				size_t lo = 0;
				_idxMap.resize(count);
				if (oldSize < count)
					lo = oldSize;
				for (size_t i=lo; i<count; ++i)
					_idxMap[i] = i;
			}
			LPARAM lp = invalidate ? LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL : LVSICF_NOSCROLL;
			//ListView_SetItemCountEx(_hList, count, lp);
			::SendMessage(_hList, LVM_SETITEMCOUNT, (WPARAM)count, lp);
			::InvalidateRect(_hList, &_rc, FALSE);

			resetSelection();
			updateButtonState();
		}
	}
}

void WindowsDlg::fitColumnsToSize()
{
	// perhaps make the path column auto size
	RECT rc;
	if (GetClientRect(_hList, &rc))
	{
		int len = (rc.right - rc.left);
		len -= (int)SendMessage(_hList, LVM_GETCOLUMNWIDTH, 0, 0);
		len -= (int)SendMessage(_hList, LVM_GETCOLUMNWIDTH, 2, 0);
		len -= GetSystemMetrics(SM_CXVSCROLL);
		len -= 1;
		SendMessage(_hList, LVM_SETCOLUMNWIDTH, 1, len);
	}
}

void WindowsDlg::resetSelection()
{
	int curSel = _pTab->getCurrentTabIndex();
	int pos = 0;
	for (vector<int>::iterator itr = _idxMap.begin(), end = _idxMap.end(); itr != end; ++itr, ++pos)
	{
		if (*itr == curSel)
		{
			ListView_SetItemState(_hList, pos, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED)
		}
		else
		{
			ListView_SetItemState(_hList, pos, 0, LVIS_SELECTED);
		}
	}
}

void WindowsDlg::doSave()
{
	NMWINDLG nmdlg;
	nmdlg.type = WDT_SAVE;
	nmdlg.curSel = ListView_GetNextItem(_hList, -1, LVNI_SELECTED); 
	nmdlg.hwndFrom = _hSelf;
	nmdlg.code = WDN_NOTIFY;
	nmdlg.nItems = ListView_GetSelectedCount(_hList);
	nmdlg.Items = new UINT[nmdlg.nItems];
	for (UINT i=-1, j=0;;++j) {
		i = ListView_GetNextItem(_hList, i, LVNI_SELECTED); 
		if (i == -1) break;						
		nmdlg.Items[j] = _idxMap[i];
	}
	SendMessage(_hParent, WDN_NOTIFY, 0, LPARAM(&nmdlg));
	delete[] nmdlg.Items;
	::InvalidateRect(_hList, &_rc, FALSE);
	ListView_RedrawItems(_hList, 0, ListView_GetSelectedCount(_hList));
}

void WindowsDlg::destroy()
{
	::GetWindowRect(_hSelf, &_lastKnownLocation);

	HWND hSelf = _hSelf;
	_hSelf = NULL;
	::DestroyWindow(hSelf);
	
}

void WindowsDlg::activateCurrent()
{
	if (ListView_GetSelectedCount(_hList) == 1)
	{
		NMWINDLG nmdlg;
		nmdlg.type = WDT_ACTIVATE;
		//nmdlg.curSel = ListView_GetNextItem(_hList, -1, LVNI_ALL|LVNI_SELECTED);
		nmdlg.curSel = _idxMap[ListView_GetNextItem(_hList, -1, LVNI_ALL|LVNI_SELECTED)];
		nmdlg.hwndFrom = _hSelf;
		nmdlg.code = WDN_NOTIFY;
		SendMessage(_hParent, WDN_NOTIFY, 0, LPARAM(&nmdlg));

		::GetWindowRect(_hSelf, &_lastKnownLocation);
		EndDialog(_hSelf, IDOK);
	}
}

void WindowsDlg::doClose()
{
	NMWINDLG nmdlg;
	nmdlg.type = WDT_CLOSE;
	//nmdlg.curSel = ListView_GetNextItem(_hList, -1, LVNI_SELECTED);
	int index = ListView_GetNextItem(_hList, -1, LVNI_ALL|LVNI_SELECTED);
	if (index == -1) return;
	
	nmdlg.curSel = _idxMap[index];
	nmdlg.hwndFrom = _hSelf;
	nmdlg.code = WDN_NOTIFY;
	UINT n = nmdlg.nItems = ListView_GetSelectedCount(_hList);
	nmdlg.Items = new UINT[nmdlg.nItems];
	vector<int> key;
	key.resize(n, 0x7fffffff);
	for(UINT i=-1, j=0;; ++j) {
		i = ListView_GetNextItem(_hList, i, LVNI_SELECTED); 
		if (i == -1) break;
		nmdlg.Items[j] = _idxMap[i];
		key[j] = i;
	}
	SendMessage(_hParent, WDN_NOTIFY, 0, LPARAM(&nmdlg));
	if (nmdlg.processed)
	{
		// Trying to retain sort order. fairly sure there is a much better algorithm for this
		vector<int>::iterator kitr = key.begin();
		for (UINT i=0; i<n; ++i, ++kitr)
		{
			if (nmdlg.Items[i] == -1)
			{
				int oldVal = _idxMap[*kitr];
				_idxMap[*kitr] = -1;
				for (vector<int>::iterator itr = _idxMap.begin(), end = _idxMap.end(); itr != end; ++itr)
					if (*itr > oldVal)
						--(*itr);
			}
		}
		_idxMap.erase(std::remove_if(_idxMap.begin(), _idxMap.end(), bind2nd(equal_to<int>(), -1)), _idxMap.end());
	}
	delete[] nmdlg.Items;

	if (_pTab->nbItem() != _idxMap.size())
		doRefresh(true);
	else
	{
		ListView_RedrawItems(_hList, 0, ListView_GetSelectedCount(_hList));
		ListView_SetItemCount(_hList, _idxMap.size());
	}
}

void WindowsDlg::doSortToTabs()
{
	int curSel = ListView_GetNextItem(_hList, -1, LVNI_SELECTED);

	if (curSel == -1)
		curSel = 0;

	NMWINDLG nmdlg;
	nmdlg.type = WDT_SORT;
	nmdlg.hwndFrom = _hSelf;
	//nmdlg.curSel = curSel;
	nmdlg.curSel = _idxMap[curSel];
	nmdlg.code = WDN_NOTIFY;
	UINT n = nmdlg.nItems = ListView_GetItemCount(_hList);
	nmdlg.Items = new UINT[nmdlg.nItems];
	vector<int> key;
	key.resize(n, 0x7fffffff);
	for(UINT i=-1, j=0;; ++j) {
		i = ListView_GetNextItem(_hList, i, LVNI_ALL); 
		if (i == -1) break;
		nmdlg.Items[j] = _idxMap[i];
		if (i == curSel)
			nmdlg.curSel = j;
		key[j] = i;
	}
	
	SendMessage(_hParent, WDN_NOTIFY, 0, LPARAM(&nmdlg));
	if (nmdlg.processed)
	{
		_idxMap.clear();
		doRefresh(true);
	}
	delete[] nmdlg.Items;
}

WindowsMenu::WindowsMenu()
{}

WindowsMenu::~WindowsMenu()
{
	if (_hMenu)
		DestroyMenu(_hMenu);
}

void WindowsMenu::init(HINSTANCE hInst, HMENU hMainMenu, const char *translation)
{
	_hMenu = ::LoadMenu(hInst, MAKEINTRESOURCE(IDR_WINDOWS_MENU));

	if (translation && translation[0])
	{
		string windowStr(translation);
		windowStr += "...";
		::ModifyMenu(_hMenu, IDM_WINDOW_WINDOWS, MF_BYCOMMAND, IDM_WINDOW_WINDOWS, windowStr.c_str());
	}

	UINT pos = 0;
	for(pos = GetMenuItemCount(hMainMenu) - 1; pos > 0; --pos)
	{
		if ((GetMenuState(hMainMenu, pos, MF_BYPOSITION) & MF_POPUP) != MF_POPUP)
			continue;
		break;
	}

	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING|MIIM_SUBMENU;

	char buffer[32];
	LoadString(hInst, IDR_WINDOWS_MENU, buffer, 32);
	mii.dwTypeData = (LPSTR)((translation && translation[0])?translation:buffer);
	mii.hSubMenu = _hMenu;
	InsertMenuItem(hMainMenu, pos, TRUE, &mii);
}

void WindowsMenu::initPopupMenu(HMENU hMenu, DocTabView *pTab)
{
	if (hMenu == _hMenu)
	{
		int curDoc = pTab->getCurrentTabIndex();
		int nMaxDoc = IDM_WINDOW_MRU_LIMIT - IDM_WINDOW_MRU_FIRST + 1;
		int nDoc = pTab->nbItem();
		nDoc = min(nDoc, nMaxDoc);
		int id, pos;
		for (id=IDM_WINDOW_MRU_FIRST, pos=0; id<IDM_WINDOW_MRU_FIRST + nDoc; ++id, ++pos)
		{
			wchar_t buffer[MAX_PATH];
			BufferID bufID = pTab->getBufferByIndex(pos);
			Buffer * buf = MainFileManager->getBufferByID(bufID);

			MENUITEMINFOW mii;
			memset(&mii, 0, sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING|MIIM_STATE|MIIM_ID;
			mii.dwTypeData = buildFileName(buffer, 60, pos, buf->getFileNameW());
			mii.fState &= ~(MF_GRAYED|MF_DISABLED|MF_CHECKED);
			if (pos == curDoc)
				mii.fState |= MF_CHECKED;
			mii.wID = id;

			UINT state = GetMenuState(hMenu, id, MF_BYCOMMAND);
			if (state == -1)
				InsertMenuItemW(hMenu, IDM_WINDOW_WINDOWS, FALSE, &mii);
			else
				SetMenuItemInfoW(hMenu, id, FALSE, &mii);
		}
		for ( ; id<=IDM_WINDOW_MRU_LIMIT; ++id)
		{
			DeleteMenu(hMenu, id, FALSE);
		}
	}
}
/*
void WindowsMenu::uninitPopupMenu(HMENU hMenu, ScintillaEditView *pView)
{
	if (hMenu == _hMenu)
	{

	}
}
*/
static wchar_t* convertFileName(wchar_t *buffer, const wchar_t *filename)
{
	wchar_t *b = buffer;
	const wchar_t *p = filename;
	while (*p)
	{
		if (*p == '&') *b++ = '&';
		*b++ = *p++;
	}
	*b = 0;	
	return buffer;
}

wchar_t *WindowsMenu::buildFileName(wchar_t *buffer, int len, int pos, const wchar_t *filename)
{
	wchar_t cwd[MAX_PATH];
	buffer[0] = 0;
	GetCurrentDirectoryW(_countof(cwd), cwd);
	wcscat(cwd, L"\\");

	wchar_t *itr = buffer;
	wchar_t *end = buffer + len - 1;
	if (pos < 9)
	{
		*itr++ = '&';
		*itr++ = '1' + pos;
	}
	else if (pos == 9)
	{
		*itr++ = '1';
		*itr++ = '&';
		*itr++ = '0';
	}
	else
	{
		itr = _itow(pos+1, itr, 10) + wcslen(itr);
	}
	*itr++ = ':';
	*itr++ = ' ';
	if (0 == wcsnicmp(filename, cwd, wcslen(cwd)))
	{
		wchar_t cnvName[MAX_PATH];
		const wchar_t *s1 = PathFindFileNameW(filename);
		int len = wcslen(s1);
		if (len < (end-itr))
		{
			wcscpy(cnvName, s1);
		}
		else
		{
			int n = (len-3-(itr-buffer))/2;
			wcsncpy(cnvName, s1, n);
			wcscpy(cnvName+n, L"...");
			wcscat(cnvName, s1 + wcslen(s1) - n);
		}
		convertFileName(itr, cnvName);
	}
	else
	{
		wchar_t cnvName[MAX_PATH*2];
		const wchar_t *s1 = convertFileName(cnvName, filename);
		PathCompactPathExW(itr, filename, len - (itr-buffer), 0);
	}
	return buffer;
}