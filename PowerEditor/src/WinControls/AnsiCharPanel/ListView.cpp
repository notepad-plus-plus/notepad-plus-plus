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


#include "ListView.h"

#include <windows.h>

#include <commctrl.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "NppConstants.h"
#include "NppDarkMode.h"
#include "Window.h"

void ListViewBase::init(HINSTANCE hInst, HWND hParent, UINT_PTR listViewID, DWORD style, DWORD exStyle)
{
	Window::init(hInst, hParent);

	INITCOMMONCONTROLSEX icex{};
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	::InitCommonControlsEx(&icex);

	_hSelf = ::CreateWindowExW(
		0,
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_BORDER | style,
		0, 0, 0, 0,
		_hParent,
		listViewID == 0 ? nullptr : reinterpret_cast<HMENU>(listViewID),
		hInst,
		nullptr);

	if (!_hSelf)
	{
		throw std::runtime_error("ListView::init : CreateWindowEx() function return null");
	}

	NppDarkMode::subclassListViewControl(_hSelf);

	subclassParentNotifyListViewCustomDraw(_hParent, listViewID);

	const DWORD lvExStyle = ListView_GetExtendedListViewStyle(_hSelf) | exStyle;
	ListView_SetExtendedListViewStyle(_hSelf, lvExStyle);

	setDarkTheme();
}

void ListViewBase::destroy()
{
	if (_hSelf != nullptr)
	{
		::DestroyWindow(_hSelf);
		_hSelf = nullptr;
	}
}

void ListViewBase::subclassParentNotifyListViewCustomDraw(HWND hParent, UINT_PTR listViewID)
{
	if (::GetWindowSubclass(hParent, ListViewBase::ListViewParentNotifyCustomDrawSubclass, static_cast<UINT_PTR>(SubclassID::first), nullptr) == FALSE)
	{
		::SetWindowSubclass(hParent, ListViewBase::ListViewParentNotifyCustomDrawSubclass, static_cast<UINT_PTR>(SubclassID::first), static_cast<DWORD_PTR>(listViewID));
	}
}

LRESULT ListViewBase::listViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	const auto lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

	switch (lplvcd->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		case CDDS_ITEMPREPAINT:
		{
			const RECT& rcRow = lplvcd->nmcd.rc;

			const bool isThemeDark = NppDarkMode::isThemeDark();
			const bool isSelected = ListView_GetItemState(lplvcd->nmcd.hdr.hwndFrom, lplvcd->nmcd.dwItemSpec, LVIS_SELECTED) == LVIS_SELECTED;
			const bool isHot = (lplvcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT;

			const auto style = ::GetWindowLongPtrW(lplvcd->nmcd.hdr.hwndFrom, GWL_STYLE);
			const bool hasHeader = ((style & LVS_REPORT) != 0) && ((style & LVS_NOCOLUMNHEADER) == 0);

			if (isThemeDark && (isSelected || isHot))
			{
				const COLORREF bgColor = isSelected ? NppDarkMode::getCtrlBackgroundColor() : NppDarkMode::getHotBackgroundColor();

				lplvcd->clrText = NppDarkMode::getTextColor();
				lplvcd->clrTextBk = bgColor;

				const auto oldClr = ::SetDCBrushColor(lplvcd->nmcd.hdc, bgColor);
				const auto hBrush = static_cast<HBRUSH>(::GetStockObject(DC_BRUSH));

				if (!hasHeader)
				{
					::FillRect(lplvcd->nmcd.hdc, &rcRow, hBrush);
				}
				else
				{
					const auto hHeader = ListView_GetHeader(lplvcd->nmcd.hdr.hwndFrom);
					const auto colCount = Header_GetItemCount(hHeader);

					const LONG paddingLeft = isThemeDark ? 1 : 0;
					const LONG paddingRight = isThemeDark ? 2 : 1;

					for (int i = 0; i < colCount; ++i)
					{
						RECT rcSubItem{ rcRow };

						if (i > 0)
						{
							RECT rcColumn{};
							Header_GetItemRect(hHeader, i, &rcColumn);
							rcSubItem.left = rcColumn.left - paddingLeft;
						}

						if (i + 1 < colCount)
						{
							RECT rcNextColumn{};
							Header_GetItemRect(hHeader, i + 1, &rcNextColumn);
							rcSubItem.right = rcNextColumn.left - paddingRight;
						}
						else
						{
							rcSubItem.right -= paddingRight;
						}

						::FillRect(lplvcd->nmcd.hdc, &rcSubItem, hBrush);
					}
				}

				::SetDCBrushColor(lplvcd->nmcd.hdc, oldClr);
			}

			if (isSelected)
			{
				::DrawFocusRect(lplvcd->nmcd.hdc, &rcRow);
			}
			else if (isHot)
			{
				::FrameRect(lplvcd->nmcd.hdc, &rcRow, isThemeDark ? NppDarkMode::getHotEdgeBrush() : ::GetSysColorBrush(COLOR_WINDOWTEXT));
			}

			return CDRF_DODEFAULT;
		}

		default:
		{
			break;
		}
	}
	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ListViewBase::ListViewParentNotifyCustomDrawSubclass(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
) noexcept
{
	switch (uMsg)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, ListViewBase::ListViewParentNotifyCustomDrawSubclass, uIdSubclass);
			break;
		}

		case WM_NOTIFY:
		{
			const auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);
			if (nmhdr->idFrom == static_cast<UINT_PTR>(dwRefData) && nmhdr->code == NM_CUSTOMDRAW)
			{
				return ListViewBase::listViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam);
			}
			break;
		}
	}
	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void ListView::init(HINSTANCE hInst, HWND parent)
{
	Window::init(hInst, parent);
	INITCOMMONCONTROLSEX icex{};

	// Ensure that the common control DLL is loaded.
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// Create the list-view window in report view with label editing enabled.
	static constexpr DWORD listViewStyles = LVS_REPORT | LVS_NOSORTHEADER
						| LVS_SINGLESEL | LVS_AUTOARRANGE
						| LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS;

	_hSelf = ::CreateWindow(WC_LISTVIEW,
                                L"",
								WS_CHILD | WS_BORDER | listViewStyles,
                                0,
                                0,
                                0,
                                0,
                                _hParent,
                                nullptr,
                                hInst,
                                nullptr);
	if (!_hSelf)
	{
		throw std::runtime_error("ListView::init : CreateWindowEx() function return null");
	}

	DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf);
	exStyle |= LVS_EX_FULLROWSELECT | LVS_EX_BORDERSELECT | LVS_EX_DOUBLEBUFFER | _extraStyle;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);

	if (!_columnInfos.empty())
	{
		LVCOLUMN lvColumn{};
		lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;

		short i = 0;
		for (auto& colInfo : _columnInfos)
		{
			lvColumn.cx = static_cast<int>(colInfo._width);
			lvColumn.pszText = colInfo._label.data();
			ListView_InsertColumn(_hSelf, ++i, &lvColumn);  // index is not 0 based but 1 based
		}
	}
}

void ListView::destroy()
{
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
}

void ListView::addLine(std::vector<std::wstring>& values2Add, LPARAM lParam, int pos2insert)
{
	if (!values2Add.size())
		return;

	if (pos2insert == -1)
		pos2insert = static_cast<int>(nbItem());

	auto it = values2Add.begin();

	LVITEM item{};
	item.mask = LVIF_TEXT | LVIF_PARAM;

	item.pszText = it->data();
	item.iItem = pos2insert;
	item.iSubItem = 0;
	item.lParam = lParam;
	ListView_InsertItem(_hSelf, &item);
	++it;

	int j = 0;
	for (; it != values2Add.end(); ++it)
	{
		ListView_SetItemText(_hSelf, pos2insert, ++j, it->data());
	}
}

size_t ListView::findAlphabeticalOrderPos(const std::wstring& string2Cmp, SortDirection sortDir)
{
	const size_t itemCount = nbItem();
	if (!itemCount)
		return 0;

	for (size_t i = 0; i < itemCount; ++i)
	{
		wchar_t str[MAX_PATH] = { '\0' };
		ListView_GetItemText(_hSelf, i, 0, str, sizeof(str));

		const int res = string2Cmp.compare(str);

		if (res < 0) // string2Cmp < str
		{
			if (sortDir == sortEncrease)
			{
				return i;
			}
		}
		else // str2Cmp >= str
		{
			if (sortDir == sortDecrease)
			{
				return i;
			}
		}
	}
	return itemCount;
}


LPARAM ListView::getLParamFromIndex(int itemIndex) const
{
	LVITEM item{};
	item.mask = LVIF_PARAM;
	item.iItem = itemIndex;
	ListView_GetItem(_hSelf, &item);

	return item.lParam;
}

std::vector<size_t> ListView::getCheckedIndexes() const
{
	std::vector<size_t> checkedIndexes;
	const size_t itemCount = nbItem();
	for (size_t i = 0; i < itemCount; ++i)
	{
		UINT st = ListView_GetItemState(_hSelf, i, LVIS_STATEIMAGEMASK);
		if (st == INDEXTOSTATEIMAGEMASK(2)) // checked
			checkedIndexes.push_back(i);
	}
	return checkedIndexes;
}
