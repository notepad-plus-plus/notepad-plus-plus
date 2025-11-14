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

#include <stdexcept>
#include "TaskList.h"
#include "TaskListDlg_rc.h"
#include "colors.h"

#include <commctrl.h>

#include <cwchar>

#include "NppConstants.h"
#include "NppDarkMode.h"
#include "dpiManagerV2.h"

void TaskList::init(HINSTANCE hInst, HWND parent, HIMAGELIST hImaLst, int nbItem, int index2set)
{
	Window::init(hInst, parent);

	_currentIndex = index2set;

	INITCOMMONCONTROLSEX icex{};
    
    // Ensure that the common control DLL is loaded. 
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

	_nbItem = nbItem;
    
    // Create the list-view window in report view with label editing enabled.
	int listViewStyles = LVS_REPORT | LVS_OWNERDATA | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER\
						| /*LVS_NOSCROLL |*/ LVS_SINGLESEL | LVS_AUTOARRANGE | LVS_OWNERDRAWFIXED\
						| LVS_SHAREIMAGELISTS/* | WS_BORDER*/;

	_hSelf = ::CreateWindow(WC_LISTVIEW, 
                                L"", 
                                WS_CHILD | listViewStyles,
                                0,
                                0, 
                                0,
                                0,
                                _hParent, 
                                NULL, 
                                hInst,
                                NULL);
	if (!_hSelf)
	{
		throw std::runtime_error("TaskList::init : CreateWindowEx() function return null");
	}

	::SetWindowSubclass(_hSelf, TaskListSelectProc, static_cast<UINT_PTR>(SubclassID::first), reinterpret_cast<DWORD_PTR>(this));

	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT | LVS_EX_DOUBLEBUFFER;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);


	LVCOLUMN lvColumn{};
	lvColumn.mask = LVCF_WIDTH;

	lvColumn.cx = 500;

	ListView_InsertColumn(_hSelf, 0, &lvColumn);

	ListView_SetItemCountEx(_hSelf, _nbItem, LVSICF_NOSCROLL);
	ListView_SetImageList(_hSelf, hImaLst, LVSIL_SMALL);

	ListView_SetItemState(_hSelf, _currentIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	ListView_SetBkColor(_hSelf, NppDarkMode::isEnabled() ? NppDarkMode::getBackgroundColor() : lightYellow);
}

void TaskList::destroy()
{
	TaskList::destroyFont();
	::DestroyWindow(_hSelf);
	_hSelf = nullptr;
}

RECT TaskList::adjustSize()
{
	RECT rc{};
	ListView_GetItemRect(_hSelf, 0, &rc, LVIR_ICON);
	const int imgWidth = rc.right - rc.left;
	const int aSpaceWidth = ListView_GetStringWidth(_hSelf, L" ");
	const int paddedBorder = ::GetSystemMetrics(SM_CXPADDEDBORDER);
	const int leftMarge = (::GetSystemMetrics(SM_CXFRAME) + paddedBorder) * 2 + aSpaceWidth * 4;

	// Temporary set "selected" font to get the worst case widths
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFontSelected), 0);
	int maxwidth = -1;

	_rc = { 0, 0, 0, 0 };
	wchar_t buf[MAX_PATH] = { '\0' };
	for (int i = 0 ; i < _nbItem ; ++i)
	{
		ListView_GetItemText(_hSelf, i, 0, buf, MAX_PATH);
		int width = ListView_GetStringWidth(_hSelf, buf);
		if (width > maxwidth)
			maxwidth = width;
		_rc.bottom += rc.bottom - rc.top;
	}

	_rc.right = maxwidth + imgWidth + leftMarge;
	ListView_SetColumnWidth(_hSelf, 0, _rc.right);
	::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);

	//if the tasklist exceeds the height of the display, leave some space at the bottom
	const LONG maxHeight = ::GetSystemMetrics(SM_CYSCREEN) - 120L;
	if (_rc.bottom > maxHeight)
	{
		_rc.bottom = maxHeight;
	}
	reSizeToWH(_rc);

	// Task List's border is 1px smaller than ::GetSystemMetrics(SM_CYFRAME) returns
	_rc.bottom += (::GetSystemMetrics(SM_CYFRAME) + paddedBorder - 1) * 2;
	return _rc;
}

void TaskList::setFont(int fontSize, const wchar_t* fontName)
{
	TaskList::destroyFont();

	auto lf = LOGFONT{ DPIManagerV2::getDefaultGUIFontForDpi(_hParent) };

	static const int fontSizeFactor = DPIManagerV2::scaleFontForFactor(fontSize);
	lf.lfHeight = DPIManagerV2::scaleFont(fontSizeFactor, _hParent);

	if (fontName != nullptr && std::wcslen(fontName) < LF_FACESIZE)
	{
		::ZeroMemory(lf.lfFaceName, LF_FACESIZE);
		::wcsncpy_s(lf.lfFaceName, fontName, LF_FACESIZE);
	}

	_hFont = ::CreateFontIndirectW(&lf);

	lf.lfWeight = FW_BOLD;

	_hFontSelected = ::CreateFontIndirectW(&lf);

	if (_hFont)
		::SendMessage(_hSelf, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);
}

void TaskList::destroyFont()
{
	if (_hFont != nullptr)
	{
		::DeleteObject(_hFont);
		_hFont = nullptr;
	}

	if (_hFontSelected != nullptr)
	{
		::DeleteObject(_hFontSelected);
		_hFontSelected = nullptr;
	}
}

int TaskList::updateCurrentIndex()
{
	for (int i = 0 ; i < _nbItem ; ++i)
	{
		int isSelected = ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
		if (isSelected == LVIS_SELECTED)
		{
			_currentIndex = i;
			return _currentIndex;
		}
	}
	return _currentIndex;
}

void TaskList::moveSelection(int direction)
{
	auto getNextIndex = [&]() -> int
	{
		const int next = _currentIndex + direction;
		if (next < 0)
			return _nbItem - 1;
		if (next >= _nbItem)
			return 0;
		return next;
	};

	const int newIndex = getNextIndex();

	// Clear current
	ListView_SetItemState(_hSelf, _currentIndex, 0, LVIS_SELECTED | LVIS_FOCUSED);
	ListView_RedrawItems(_hSelf, _currentIndex, _currentIndex);

	// Set new
	ListView_SetItemState(_hSelf, newIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ListView_RedrawItems(_hSelf, newIndex, newIndex);
	::UpdateWindow(_hSelf);

	_currentIndex = newIndex;
	ListView_EnsureVisible(_hSelf, _currentIndex, TRUE);
}

LRESULT CALLBACK TaskList::TaskListSelectProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	auto* pRefData = reinterpret_cast<TaskList*>(dwRefData);

	switch (uMsg)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, TaskList::TaskListSelectProc, uIdSubclass);
			break;
		}

		case WM_KEYUP:
		{
			if (wParam == VK_CONTROL)
			{
				::SendMessage(pRefData->_hParent, WM_COMMAND, ID_PICKEDUP, pRefData->_currentIndex);
			}
			return 0;
		}

		case WM_MOUSEWHEEL:
		{
			const auto zDelta = static_cast<short>(HIWORD(wParam));
			pRefData->moveSelection(zDelta > 0 ? -1 : 1);
			return 0;
		}

		case WM_KEYDOWN:
		{
			return 0;
		}

		case WM_GETDLGCODE:
		{
			const auto* msg = reinterpret_cast<MSG*>(lParam);

			if (msg != nullptr)
			{
				if ((msg->message == WM_KEYDOWN) && (0x80 & GetKeyState(VK_CONTROL)))
				{
					// Shift+Tab is cool but I think VK_UP and VK_LEFT are also cool :-)
					if (((msg->wParam == VK_TAB) && (0x80 & GetKeyState(VK_SHIFT)))
						|| (msg->wParam == VK_UP)
						|| (msg->wParam == VK_LEFT))
					{
						pRefData->moveSelection(-1);
					}
					// VK_DOWN and VK_RIGHT do the same as VK_TAB does
					else if ((msg->wParam == VK_TAB)
						|| (msg->wParam == VK_DOWN)
						|| (msg->wParam == VK_RIGHT))
					{
						pRefData->moveSelection(1);
					}
				}
				else
				{
					return 0;
				}
			}
			return DLGC_WANTALLKEYS;
		}

		default:
			break;
	}
	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
