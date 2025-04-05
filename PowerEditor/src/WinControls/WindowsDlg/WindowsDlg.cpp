// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

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


#include <functional>
#include <algorithm>
#include "WindowsDlg.h"
#include "WindowsDlgRc.h"
#include "DocTabView.h"
#include "EncodingMapper.h"
#include "localization.h"

using namespace std;

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER     0x00010000
#endif

#define WD_ROOTNODE					"WindowsDlg"
#define WD_CLMNNAME					"ColumnName"
#define WD_CLMNPATH					"ColumnPath"
#define WD_CLMNTYPE					"ColumnType"
#define WD_CLMNSIZE					"ColumnSize"
#define WD_NBDOCSTOTAL				"NbDocsTotal"
#define WD_MENUCOPYNAME				"MenuCopyName"
#define WD_MENUCOPYPATH				"MenuCopyPath"

static const wchar_t *readonlyString = L" [Read Only]";
const UINT WDN_NOTIFY = RegisterWindowMessage(L"WDN_NOTIFY");
/*
inline static DWORD GetStyle(HWND hWnd) {
	return (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
}

inline static DWORD GetExStyle(HWND hWnd) {
	return (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
}

inline static BOOL ModifyStyle(HWND hWnd, DWORD dwRemove, DWORD dwAdd) {
	DWORD dwStyle = (DWORD)::GetWindowLongPtr(hWnd, GWL_STYLE);
	DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if (dwStyle == dwNewStyle)
		return FALSE;
	::SetWindowLongPtr(hWnd, GWL_STYLE, dwNewStyle);
	return TRUE;
}

inline static BOOL ModifyStyleEx(HWND hWnd, DWORD dwRemove, DWORD dwAdd) {
	DWORD dwStyle = (DWORD)::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if (dwStyle == dwNewStyle)
		return FALSE;
	::SetWindowLongPtr(hWnd, GWL_EXSTYLE, dwNewStyle);
	return TRUE;
}
*/

struct NumericStringEquivalence
{
	int operator()(const wchar_t* s1, const wchar_t* s2) const
	{
		return numstrcmp(s1, s2);
	}

	static inline int numstrcmp_get(const wchar_t **str, int *length)
	{
		const wchar_t *p = *str;
		int value = 0;
		for (*length = 0; isdigit(*p); ++(*length))
			value = value * 10 + *p++ - '0';
		*str = p;
		return (value);
	}

	static int numstrcmp(const wchar_t *str1, const wchar_t *str2)
	{
		wchar_t *p1 = nullptr, *p2 = nullptr;
		int c1 = 0, c2 = 0, lcmp = 0;
		for (;;)
		{
			if (*str1 == 0 || *str2 == 0)
			{
				if (*str1 != *str2)
					lcmp = *str1 - *str2;
				break;
			}

			if (_istdigit(*str1) && _istdigit(*str2))
			{
				lcmp = wcstol(str1, &p1, 10) - wcstol(str2, &p2, 10);
				if ( lcmp == 0 )
					lcmp = static_cast<int32_t>((p2 - str2) - (p1 - str1));
				if ( lcmp != 0 )
					break;
				str1 = p1, str2 = p2;
			}
			else
			{
				if (_istascii(*str1) && _istupper(*str1))
					c1 = towlower(*str1);
				else
					c1 = *str1;
				if (_istascii(*str2) && _istupper(*str2))
					c2 = towlower(*str2);
				else
					c2 = *str2;
				lcmp = (c1 - c2);
				if (lcmp != 0)
					break;
				++str1, ++str2;
			}
		}
		return ( lcmp < 0 ) ? -1 : (lcmp > 0 ? 1 : 0);
	}
};

struct BufferEquivalent
{
	NumericStringEquivalence _strequiv{};
	DocTabView* _pTab = nullptr;
	int _iColumn = 0;
	bool _reverse = false;
	BufferEquivalent(DocTabView* pTab, int iColumn, bool reverse)
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
		if (_iColumn >= 0 && _iColumn <= 3)
		{
			BufferID bid1 = _pTab->getBufferByIndex(i1);
			BufferID bid2 = _pTab->getBufferByIndex(i2);
			Buffer * b1 = MainFileManager.getBufferByID(bid1);
			Buffer * b2 = MainFileManager.getBufferByID(bid2);

			if (_iColumn == 0)
			{
				const wchar_t *s1 = b1->getFileName();
				const wchar_t *s2 = b2->getFileName();
				int result = _strequiv(s1, s2);

				if (result != 0) // default to filepath sorting when equivalent
					return result < 0;
			}
			else if (_iColumn == 2)
			{
				NppParameters & nppParameters = NppParameters::getInstance();
				const wchar_t *s1;
				const wchar_t *s2;
				//const wchar_t empty[] = ;
				Lang *lang1 = nppParameters.getLangFromID(b1->getLangType());

				if (lang1)
				{
					s1 = lang1->getLangName();
				}
				else
					s1 = L"";

				Lang *lang2 = nppParameters.getLangFromID(b2->getLangType());
				if (lang2)
				{
					s2 = lang2->getLangName();
				}
				else
					s2 = L"";

				int result = _strequiv(s1, s2);

				if (result != 0) // default to filepath sorting when equivalent
					return result < 0;

			}
			else if (_iColumn == 3)
			{
				auto t1 = b1->docLength();
				auto t2 = b2->docLength();

				if (t1 != t2) // default to filepath sorting when equivalent
					return (t1 < t2);
			}

			// _iColumn == 1
			const wchar_t *s1 = b1->getFullPathName();
			const wchar_t *s2 = b2->getFullPathName();
			return _strequiv(s1, s2) < 0;	//we can compare the full path to sort on directory, since after sorting directories sorting files is the second thing to do (if directories are the same that is)
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

WindowsDlg::WindowsDlg() : MyBaseClass(WindowsDlgMap)
{
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

intptr_t CALLBACK WindowsDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG :
		{
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			pNativeSpeaker->changeDlgLang(_hSelf, "Window");

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
			NppDarkMode::autoSubclassAndThemeWindowNotify(_hSelf);

			return MyBaseClass::run_dlgProc(message, wParam, lParam);
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_COMMAND :
		{
			switch (wParam)
			{
				case IDOK:
				{
					activateCurrent();
					return TRUE;
				}
				case IDCANCEL:
				{
					::GetWindowRect(_hSelf, &_lastKnownLocation);
					EndDialog(_hSelf, IDCANCEL);
					return TRUE;
				}
				case IDC_WINDOWS_SAVE:
				{
					doSave();
					return TRUE;
				}
				case IDC_WINDOWS_CLOSE:
				{
					doClose();
					return TRUE;
				}
				case IDC_WINDOWS_SORT:
				{
					// they never set a column to sort by, so assume they wanted filename
					if (_currentColumn == -1)
					{
						_currentColumn = 0;
						_reverseSort = false;
						_lastSort = _currentColumn;

						updateColumnNames();
						doColumnSort();
					}

					doSortToTabs();

					// must re-sort because tab indexes changed
					doColumnSort();
					break;
				}

				default:
					if (HIWORD(wParam) == 0)
					{
						// Menu
						switch (LOWORD(wParam))
						{
						case IDM_WINDOW_COPY_NAME:
							putItemsToClipboard(false);
							break;
						case IDM_WINDOW_COPY_PATH:
							putItemsToClipboard(true);
							break;
						}
					}
					break;
			}
			break;
		}

		case WM_DESTROY:
		{
			//destroy();
			return TRUE;
		}

		case WM_NOTIFY :
		{
			if (wParam == IDC_WINDOWS_LIST)
			{
				NMHDR* pNMHDR = reinterpret_cast<NMHDR*>(lParam);
				if (pNMHDR->code == LVN_GETDISPINFO)
				{
					NMLVDISPINFO *pLvdi = (NMLVDISPINFO *)pNMHDR;

					if (pLvdi->item.mask & LVIF_TEXT)
					{
						pLvdi->item.pszText[0] = 0;
						Buffer* buf = getBuffer(pLvdi->item.iItem);
						if (!buf)
							return FALSE;
						wstring text;
						if (pLvdi->item.iSubItem == 0) // file name
						{
							text = buf->getFileName();
							if (buf->isDirty())
							{
								text += '*';
							}
							else if (buf->isReadOnly())
							{
								text += readonlyString;
							}
						}
						else if (pLvdi->item.iSubItem == 1) // directory
						{
							const wchar_t *fullName = buf->getFullPathName();
							const wchar_t *fileName = buf->getFileName();
							int len = lstrlen(fullName)-lstrlen(fileName);
							if (!len) {
								len = 1;
								fullName = L"";
							}
							text.assign(fullName, len);
						}
						else if (pLvdi->item.iSubItem == 2) // Type
						{
							NppParameters& nppParameters = NppParameters::getInstance();
							Lang *lang = nppParameters.getLangFromID(buf->getLangType());
							if (NULL != lang)
							{
								text = lang->getLangName();
							}
						}
						else if (pLvdi->item.iSubItem == 3) // size
						{
							size_t docSize = buf->docLength();
							string docSizeText = to_string(docSize);
							text = wstring(docSizeText.begin(), docSizeText.end());
						}

						if (static_cast<int>(text.length()) < pLvdi->item.cchTextMax)
						{
							// Copy the resulting text to destination with a null terminator.
							wcscpy_s(pLvdi->item.pszText, text.length() + 1, text.c_str());
						}
					}
					return TRUE;
				}
				else if (pNMHDR->code == LVN_COLUMNCLICK) // sort columns with stable sort
				{
					const NMLISTVIEW *pNMLV = (NMLISTVIEW *)pNMHDR;
					if (pNMLV->iItem == -1)
					{
						_currentColumn = pNMLV->iSubItem;

						if (_lastSort == _currentColumn)
						{
							_reverseSort = true;
							_lastSort = -1;
						}
						else
						{
							_reverseSort = false;
							_lastSort = _currentColumn;
						}

						updateColumnNames();
						doColumnSort();
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
					return TRUE;
				}
				else if (pNMHDR->code == LVN_KEYDOWN)
				{
					const NMLVKEYDOWN *lvkd = (NMLVKEYDOWN *)pNMHDR;
					short ctrl = GetKeyState(VK_CONTROL);
					short alt = GetKeyState(VK_MENU);
					short shift = GetKeyState(VK_SHIFT);
					if (lvkd->wVKey == 'A' && ctrl<0 && alt>=0 && shift>=0)
					{
						// Ctrl + A
						for (int i=0, n=ListView_GetItemCount(_hList); i<n; ++i)
							ListView_SetItemState(_hList, i, LVIS_SELECTED, LVIS_SELECTED);
					}
					else if (lvkd->wVKey == 'C' && ctrl & 0x80)
					{
						// Ctrl + C
						if (ListView_GetSelectedCount(_hList) != 0)
							putItemsToClipboard(true);
					}
					return TRUE;
				}
			}
			break;
		}

		case WM_CONTEXTMENU:
			{
				if (!_listMenu.isCreated())
				{
					NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
					const std::vector<MenuItemUnit> itemUnitArray
					{
						{IDM_WINDOW_COPY_NAME, pNativeSpeaker->getAttrNameStr(L"Copy Name(s)", WD_ROOTNODE, WD_MENUCOPYNAME)},
						{IDM_WINDOW_COPY_PATH, pNativeSpeaker->getAttrNameStr(L"Copy Pathname(s)", WD_ROOTNODE, WD_MENUCOPYPATH)}
					};
					_listMenu.create(_hSelf, itemUnitArray);
				}

				const bool enableMenu = ListView_GetSelectedCount(_hList) != 0;
				_listMenu.enableItem(IDM_WINDOW_COPY_NAME, enableMenu);
				_listMenu.enableItem(IDM_WINDOW_COPY_PATH, enableMenu);

				POINT p{};
				::GetCursorPos(&p);
				_listMenu.display(p);
			}
			return TRUE;
	}
	return MyBaseClass::run_dlgProc(message, wParam, lParam);
}

void WindowsDlg::doColumnSort()
{
	if (_currentColumn == -1)
		return;

	size_t i = 0;
	size_t n = _idxMap.size();
	vector<int> sortMap;
	sortMap.resize(n);
	for (; i < n; ++i)
		sortMap[_idxMap[i]] = ListView_GetItemState(_hList, i, LVIS_SELECTED);

	stable_sort(_idxMap.begin(), _idxMap.end(), BufferEquivalent(_pTab, _currentColumn, _reverseSort));
	for (i = 0; i < n; ++i)
		ListView_SetItemState(_hList, i, sortMap[_idxMap[i]] ? LVIS_SELECTED : 0, LVIS_SELECTED);

	::InvalidateRect(_hList, &_rc, FALSE);
	updateButtonState();
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
	EnableWindow(GetDlgItem(_hSelf, IDC_WINDOWS_SORT), TRUE);
}

int WindowsDlg::doDialog()
{
	const auto dpiContext = DPIManagerV2::setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);

	int result = static_cast<int>(DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_WINDOWS), _hParent, dlgProc, reinterpret_cast<LPARAM>(this)));

	if (dpiContext != NULL)
	{
		DPIManagerV2::setThreadDpiAwarenessContext(dpiContext);
	}
	return result;
}

BOOL WindowsDlg::onInitDialog()
{
	_winMgr.InitToFitSizeFromCurrent(_hSelf);

	// save min size for OK/Cancel buttons
	_szMinButton = RectToSize(_winMgr.GetRect(IDOK));
	_szMinListCtrl = RectToSize(_winMgr.GetRect(IDC_WINDOWS_LIST));
	_currentColumn = -1;
	_lastSort = -1;
	_reverseSort = false;

	_winMgr.CalcLayout(_hSelf);
	_winMgr.SetWindowPositions(_hSelf);
	getClientRect(_rc);

	_hList = ::GetDlgItem(_hSelf, IDC_WINDOWS_LIST);
	DWORD exStyle = ListView_GetExtendedListViewStyle(_hList);
	exStyle |= LVS_EX_HEADERDRAGDROP|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER;
	ListView_SetExtendedListViewStyle(_hList, exStyle);

	COLORREF fgColor = (NppParameters::getInstance()).getCurrentDefaultFgColor();
	COLORREF bgColor = (NppParameters::getInstance()).getCurrentDefaultBgColor();

	ListView_SetBkColor(_hList, bgColor);
	ListView_SetTextBkColor(_hList, bgColor);
	ListView_SetTextColor(_hList, fgColor);

	RECT rc{};
	GetClientRect(_hList, &rc);
	LONG width = rc.right - rc.left;

	LVCOLUMN lvColumn{};
	lvColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;

	wstring columnText;
	NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	columnText = L"⇵ " + pNativeSpeaker->getAttrNameStr(L"Name", WD_ROOTNODE, WD_CLMNNAME);
	lvColumn.pszText = const_cast<wchar_t *>(columnText.c_str());
	lvColumn.cx = width / 4;
	SendMessage(_hList, LVM_INSERTCOLUMN, 0, LPARAM(&lvColumn));

	columnText = L"⇵ " + pNativeSpeaker->getAttrNameStr(L"Path", WD_ROOTNODE, WD_CLMNPATH);
	lvColumn.pszText = const_cast<wchar_t *>(columnText.c_str());
	lvColumn.cx = 300;
	SendMessage(_hList, LVM_INSERTCOLUMN, 1, LPARAM(&lvColumn));

	lvColumn.fmt = LVCFMT_CENTER;
	columnText = L"⇵ " + pNativeSpeaker->getAttrNameStr(L"Type", WD_ROOTNODE, WD_CLMNTYPE);
	lvColumn.pszText = const_cast<wchar_t *>(columnText.c_str());
	lvColumn.cx = 100;
	SendMessage(_hList, LVM_INSERTCOLUMN, 2, LPARAM(&lvColumn));

	columnText = L"⇵ " + pNativeSpeaker->getAttrNameStr(L"Size", WD_ROOTNODE, WD_CLMNSIZE);
	lvColumn.pszText = const_cast<wchar_t *>(columnText.c_str());
	lvColumn.cx = 100;
	SendMessage(_hList, LVM_INSERTCOLUMN, 3, LPARAM(&lvColumn));

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

void WindowsDlg::updateColumnNames()
{
	LVCOLUMN lvColumn{};
	lvColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;

	wstring columnText;
	NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	columnText = pNativeSpeaker->getAttrNameStr(L"Name", WD_ROOTNODE, WD_CLMNNAME);
	if (_currentColumn != 0)
	{
		columnText = L"⇵ " + columnText;
	}
	else if (_reverseSort)
	{
		columnText = L"△ " + columnText;
	}
	else
	{
		columnText = L"▽ " + columnText;
	}
	lvColumn.pszText = const_cast<wchar_t *>(columnText.c_str());
	lvColumn.cx = static_cast<int>(SendMessage(_hList, LVM_GETCOLUMNWIDTH, 0, 0));
	SendMessage(_hList, LVM_SETCOLUMN, 0, LPARAM(&lvColumn));

	columnText = pNativeSpeaker->getAttrNameStr(L"Path", WD_ROOTNODE, WD_CLMNPATH);
	if (_currentColumn != 1)
	{
		columnText = L"⇵ " + columnText;
	}
	else if (_reverseSort)
	{
		columnText = L"△ " + columnText;
	}
	else
	{
		columnText = L"▽ " + columnText;
	}
	lvColumn.pszText = const_cast<wchar_t *>(columnText.c_str());
	lvColumn.cx = static_cast<int>(SendMessage(_hList, LVM_GETCOLUMNWIDTH, 1, 0));
	SendMessage(_hList, LVM_SETCOLUMN, 1, LPARAM(&lvColumn));

	lvColumn.fmt = LVCFMT_CENTER;
	columnText = pNativeSpeaker->getAttrNameStr(L"Type", WD_ROOTNODE, WD_CLMNTYPE);
	if (_currentColumn != 2)
	{
		columnText = L"⇵ " + columnText;
	}
	else if (_reverseSort)
	{
		columnText = L"△ " + columnText;
	}
	else
	{
		columnText = L"▽ " + columnText;
	}
	lvColumn.pszText = const_cast<wchar_t *>(columnText.c_str());
	lvColumn.cx = static_cast<int>(SendMessage(_hList, LVM_GETCOLUMNWIDTH, 2, 0));
	SendMessage(_hList, LVM_SETCOLUMN, 2, LPARAM(&lvColumn));

	columnText = pNativeSpeaker->getAttrNameStr(L"Size", WD_ROOTNODE, WD_CLMNSIZE);
	if (_currentColumn != 3)
	{
		columnText = L"⇵ " + columnText;
	}
	else if (_reverseSort)
	{
		columnText = L"△ " + columnText;
	}
	else
	{
		columnText = L"▽ " + columnText;
	}
	lvColumn.pszText = const_cast<wchar_t *>(columnText.c_str());
	lvColumn.cx = static_cast<int>(SendMessage(_hList, LVM_GETCOLUMNWIDTH, 3, 0));
	SendMessage(_hList, LVM_SETCOLUMN, 3, LPARAM(&lvColumn));
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
	NMWINMGR &nmw = *reinterpret_cast<NMWINMGR *>(lp);
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
				for (size_t i = lo; i < count; ++i)
					_idxMap[i] = int(i);
			}
			LPARAM lp = invalidate ? LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL : LVSICF_NOSCROLL;
			::SendMessage(_hList, LVM_SETITEMCOUNT, count, lp);
			::InvalidateRect(_hList, &_rc, FALSE);

			resetSelection();
			updateButtonState();
			doCount();
		}
	}
}

void WindowsDlg::fitColumnsToSize()
{
	// perhaps make the path column auto size
	RECT rc{};
	if (GetClientRect(_hList, &rc))
	{
		int len = (rc.right - rc.left);
		len -= static_cast<int>(SendMessage(_hList, LVM_GETCOLUMNWIDTH, 0, 0));
		len -= static_cast<int>(SendMessage(_hList, LVM_GETCOLUMNWIDTH, 2, 0));
		len -= static_cast<int>(SendMessage(_hList, LVM_GETCOLUMNWIDTH, 3, 0));
		len -= GetSystemMetrics(SM_CXVSCROLL);
		len -= 1;
		SendMessage(_hList, LVM_SETCOLUMNWIDTH, 1, len);
	}
}

void WindowsDlg::resetSelection()
{
	if (!_pTab)
		return;

	auto curSel = _pTab->getCurrentTabIndex();
	int pos = 0;
	for (vector<int>::iterator itr = _idxMap.begin(), end = _idxMap.end(); itr != end; ++itr, ++pos)
	{
		if (*itr == curSel)
		{
			ListView_SetItemState(_hList, pos, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}
		else
		{
			ListView_SetItemState(_hList, pos, 0, LVIS_SELECTED);
		}
	}
}

void WindowsDlg::doSave()
{
	NMWINDLG nmdlg{};
	nmdlg.type = WDT_SAVE;
	nmdlg.curSel = ListView_GetNextItem(_hList, -1, LVNI_SELECTED);
	nmdlg.hwndFrom = _hSelf;
	nmdlg.code = WDN_NOTIFY;
	nmdlg.nItems = ListView_GetSelectedCount(_hList);
	nmdlg.Items = new UINT[nmdlg.nItems];

	int i = -1;
	for (UINT j = 0; j < nmdlg.nItems; ++j)
	{
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
		NMWINDLG nmdlg{};
		nmdlg.type = WDT_ACTIVATE;
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
	NMWINDLG nmdlg{};
	nmdlg.type = WDT_CLOSE;
	int index = ListView_GetNextItem(_hList, -1, LVNI_ALL|LVNI_SELECTED);
	if (index == -1) return;

	nmdlg.curSel = _idxMap[index];
	nmdlg.hwndFrom = _hSelf;
	nmdlg.code = WDN_NOTIFY;
	UINT n = nmdlg.nItems = ListView_GetSelectedCount(_hList);
	nmdlg.Items = new UINT[nmdlg.nItems];
	vector<int> key;
	key.resize(n, 0x7fffffff);

	int i = -1;
	for (UINT j = 0; j < n; ++j)
	{
		i = ListView_GetNextItem(_hList, i, LVNI_SELECTED);
		if (i == -1) break;
		ListView_SetItemState(_hList, i, 0, LVIS_SELECTED); // deselect
		nmdlg.Items[j] = _idxMap[i];
		key[j] = i;
	}
	SendMessage(_hParent, WDN_NOTIFY, 0, LPARAM(&nmdlg));
	if (nmdlg.processed)
	{
		// Trying to retain sort order. fairly sure there is a much better algorithm for this
		vector<int>::iterator kitr = key.begin();
		for (UINT k = 0; k < n; ++k, ++kitr)
		{
			if (nmdlg.Items[k] == ((UINT)-1))
			{
				int oldVal = _idxMap[*kitr];
				_idxMap[*kitr] = -1;
				for (vector<int>::iterator itr = _idxMap.begin(), end = _idxMap.end(); itr != end; ++itr)
					if (*itr > oldVal)
						--(*itr);
			}
		}
		_idxMap.erase(remove_if(_idxMap.begin(), _idxMap.end(), bind(equal_to<int>(), placeholders::_1, -1)), _idxMap.end());
	}
	delete[] nmdlg.Items;

	if (_idxMap.size() < 1)
		::SendMessage(_hSelf, WM_CLOSE, 0, 0);
	else if (_pTab->nbItem() != _idxMap.size())
		doRefresh(true);
	else
	{
		// select first previously selected item (or last one if only the last one was removed)
		if (index == static_cast<int>(_idxMap.size()))
			index -= 1;

		if (index >= 0)
		{
			ListView_SetItemState(_hList, index, LVIS_SELECTED, LVIS_SELECTED);
			ListView_RedrawItems(_hList, 0, _idxMap.size() - 1);
		}
		ListView_SetItemCount(_hList, _idxMap.size());
	}
	doCount();
}

//this function will be called everytime when close is performed
//as well as each time refresh is performed to keep updated
void WindowsDlg::doCount()
{
	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	wstring msg = pNativeSpeaker->getAttrNameStr(L"Windows", "Dialog", "Window", "title");
	msg += L" - ";
	msg += pNativeSpeaker->getAttrNameStr(L"Total documents: ", WD_ROOTNODE, WD_NBDOCSTOTAL);
	msg += L" ";
	msg += to_wstring(_idxMap.size());
	SetWindowText(_hSelf,msg.c_str());
}

void WindowsDlg::doSort()
{
	if (!_pTab)
		return;

	size_t count =  _pTab->nbItem();
	std::vector<UINT> items(count);
	auto currrentTabIndex = _pTab->getCurrentTabIndex();
	NMWINDLG nmdlg{};
	nmdlg.type = WDT_SORT;
	nmdlg.hwndFrom = _hSelf;
	nmdlg.curSel = currrentTabIndex;
	nmdlg.code = WDN_NOTIFY;
	nmdlg.nItems = static_cast<UINT>(count);
	nmdlg.Items = items.data();
	for (size_t i=0; i < count; ++i)
	{
		nmdlg.Items[i] = _idxMap[i];
	}
	SendMessage(_hParent, WDN_NOTIFY, 0, LPARAM(&nmdlg));
	if (nmdlg.processed)
	{
		_idxMap.clear();
		refreshMap();
	}

	//After sorting, need to open the active tab before sorting
	//This will be helpful when large number of documents are opened
	__int64 newPosition = -1;
	std::vector<int>::iterator it = std::find(_idxMap.begin(), _idxMap.end(), currrentTabIndex);
	if (it != _idxMap.end())
	{
		newPosition = it - _idxMap.begin();
	}
	nmdlg.type = WDT_ACTIVATE;
	nmdlg.curSel = static_cast<UINT>(newPosition);
	nmdlg.hwndFrom = _hSelf;
	nmdlg.code = WDN_NOTIFY;
	SendMessage(_hParent, WDN_NOTIFY, 0, LPARAM(&nmdlg));
}

void WindowsDlg::sort(int columnID, bool reverseSort)
{
	refreshMap();
	_currentColumn = columnID;
	_reverseSort = reverseSort;
	stable_sort(_idxMap.begin(), _idxMap.end(), BufferEquivalent(_pTab, _currentColumn, _reverseSort));
}

void WindowsDlg::sortFileNameASC()
{
	sort(0, false);
}

void WindowsDlg::sortFileNameDSC()
{
	sort(0, true);
}

void WindowsDlg::sortFilePathASC()
{
	sort(1, false);
}

void WindowsDlg::sortFilePathDSC()
{
	sort(1, true);
}

void WindowsDlg::sortFileTypeASC()
{
	sort(2, false);
}

void WindowsDlg::sortFileTypeDSC()
{
	sort(2, true);
}

void WindowsDlg::sortFileSizeASC()
{
	sort(3, false);
}

void WindowsDlg::sortFileSizeDSC()
{
	sort(3, true);
}

void WindowsDlg::refreshMap()
{
	size_t count = (_pTab != NULL) ? _pTab->nbItem() : 0;
	size_t oldSize = _idxMap.size();
	if (count == oldSize)
		return;

	size_t lo = 0;
	_idxMap.resize(count);
	if (oldSize < count)
		lo = oldSize;

	for (size_t i = lo; i < count; ++i)
		_idxMap[i] = int(i);
}

void WindowsDlg::doSortToTabs()
{
	int curSel = ListView_GetNextItem(_hList, -1, LVNI_SELECTED);

	if (curSel == -1)
		curSel = 0;

	NMWINDLG nmdlg{};
	nmdlg.type = WDT_SORT;
	nmdlg.hwndFrom = _hSelf;
	nmdlg.curSel = _idxMap[curSel];
	nmdlg.code = WDN_NOTIFY;
	nmdlg.nItems = ListView_GetItemCount(_hList);
	nmdlg.Items = new UINT[nmdlg.nItems];

	int i = -1;
	for (UINT j = 0; j < nmdlg.nItems; ++j)
	{
		i = ListView_GetNextItem(_hList, i, LVNI_ALL);
		if (i == -1)
			break;
		nmdlg.Items[j] = _idxMap[i];
		if (i == curSel)
			nmdlg.curSel = j;
	}

	SendMessage(_hParent, WDN_NOTIFY, 0, LPARAM(&nmdlg));
	if (nmdlg.processed)
	{
		_idxMap.clear();
		doRefresh(true);
	}
	delete[] nmdlg.Items;
}

void WindowsDlg::putItemsToClipboard(bool isFullPath)
{
	std::vector<Buffer*> buffers;
	int i = -1;
	do
	{
		i = ListView_GetNextItem(_hList, i, LVNI_SELECTED);
		// Get the file name.
		// Do not use ListView_GetItemText() because 1st column may contain "*" or "[Read Only]".
		buffers.push_back(getBuffer(i));
	}
	while (i >= 0);

	buf2Clipboard(buffers, isFullPath, _hList);
}

Buffer* WindowsDlg::getBuffer(int index) const
{
	if (index < 0 || index >= static_cast<int>(_idxMap.size()))
		return nullptr;

	index = _idxMap[index];
	if (index < 0 || !_pTab || index >= static_cast<int>(_pTab->nbItem()))
		return nullptr;

	BufferID bufID = _pTab->getBufferByIndex(index);
	return MainFileManager.getBufferByID(bufID);
}

void WindowsMenu::init(HMENU hMainMenu)
{
	_hMenu = ::GetSubMenu(hMainMenu, MENUINDEX_WINDOW);
	_hMenuList = ::GetSubMenu(hMainMenu, MENUINDEX_LIST);
}

void WindowsMenu::initPopupMenu(HMENU hMenu, DocTabView* pTab)
{
	bool isDropListMenu = false;

	UINT firstId = 0;
	UINT limitId = 0;
	UINT menuPosId = 0;

	if (hMenu == _hMenu)
	{
		firstId = IDM_WINDOW_MRU_FIRST;
		limitId = IDM_WINDOW_MRU_LIMIT;
		menuPosId = IDM_WINDOW_WINDOWS;
	}
	else if (hMenu == _hMenuList)
	{
		isDropListMenu = true;

		if (_limitPrev < pTab->nbItem())
		{
			_limitPrev = static_cast<UINT>(pTab->nbItem());
		}

		firstId = IDM_DROPLIST_MRU_FIRST;
		limitId = IDM_DROPLIST_MRU_FIRST + _limitPrev - 1;
		menuPosId = IDM_DROPLIST_LIST;
	}

	if (firstId > 0 && limitId > 0 && menuPosId > 0)
	{
		auto curDoc = pTab->getCurrentTabIndex();
		size_t nMaxDoc = static_cast<size_t>(limitId) - firstId + 1;
		size_t nDoc = pTab->nbItem();
		nDoc = std::min<size_t>(nDoc, nMaxDoc);
		UINT id = firstId;
		UINT guard = firstId + static_cast<int32_t>(nDoc);
		size_t pos = 0;
		for (; id < guard; ++id, ++pos)
		{
			BufferID bufID = pTab->getBufferByIndex(pos);
			Buffer* buf = MainFileManager.getBufferByID(bufID);

			MENUITEMINFO mii{};
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;

			wstring strBuffer(BuildMenuFileName(60, static_cast<int32_t>(pos), buf->getFileName(), !isDropListMenu));
			std::vector<wchar_t> vBuffer(strBuffer.begin(), strBuffer.end());
			vBuffer.push_back('\0');
			mii.dwTypeData = (&vBuffer[0]);

			mii.fState &= ~(MF_GRAYED | MF_DISABLED | MF_CHECKED);
			if (static_cast<int32_t>(pos) == curDoc)
			{
				mii.fState |= MF_CHECKED;
			}
			mii.wID = id;

			UINT state = GetMenuState(hMenu, id, MF_BYCOMMAND);
			if (state == static_cast<UINT>(-1))
			{
				InsertMenuItem(hMenu, menuPosId, TRUE, &mii);
				if (isDropListMenu)
				{
					DeleteMenu(hMenu, menuPosId, FALSE);
				}
			}
			else
			{
				SetMenuItemInfo(hMenu, id, FALSE, &mii);
			}
		}
		for (; id <= limitId; ++id)
		{
			DeleteMenu(hMenu, id, FALSE);
		}

		if (isDropListMenu)
		{
			_limitPrev = static_cast<UINT>(pTab->nbItem());
		}
	}
}
