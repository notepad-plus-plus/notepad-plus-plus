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


#include "clipboardHistoryPanel.h"

#include <windows.h>

#include <commctrl.h>

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <stdexcept>
#include <string>

#include <Scintilla.h>

#include "Common.h"
#include "DockingDlgInterface.h"
#include "NppConstants.h"
#include "NppDarkMode.h"
#include "ScintillaEditView.h"
#include "Window.h"
#include "clipboardFormats.h"
#include "clipboardHistoryPanel_rc.h"
#include "dockingResource.h"
#include "menuCmdID.h"
#include "resource.h"

#define MAX_DISPLAY_LENGTH 64

ClipboardDataInfo ClipboardHistoryPanel::getClipboadData()
{
	ClipboardDataInfo clipboardData;
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
		return clipboardData;

	if (!OpenClipboard(NULL))
		return clipboardData;
	 
	HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
	if (hglb != NULL)
	{ 
		unsigned char* pData = static_cast<unsigned char*>(GlobalLock(hglb));
		if (pData != NULL)
		{
			UINT cf_nppTextLen = RegisterClipboardFormat(CF_NPPTEXTLEN);
			if (IsClipboardFormatAvailable(cf_nppTextLen))
			{
				HGLOBAL hglbLen = GetClipboardData(cf_nppTextLen); 
				if (hglbLen != NULL) 
				{
					HGLOBAL hglb_binText = GetClipboardData(CF_TEXT);
					if (hglb_binText != NULL)
					{
						unsigned char* pData_bin = static_cast<unsigned char*>(GlobalLock(hglb_binText));
						if (pData_bin != NULL)
						{
							const unsigned long* lpLen = static_cast<unsigned long*>(::GlobalLock(hglbLen));
							if (lpLen != NULL) // Special copy-paste: Binary data
							{
								SIZE_T availableBytes = GlobalSize(hglb_binText);
								size_t nbBytes = (*lpLen <= availableBytes) ? static_cast<size_t>(*lpLen) : static_cast<size_t>(availableBytes);
								for (size_t i = 0; i < nbBytes; ++i)
								{
									clipboardData._data.push_back(static_cast<unsigned char>(pData_bin[i]));
								}
								clipboardData._isBinaryContained = true;
								GlobalUnlock(hglbLen);
							}

						}
						GlobalUnlock(hglb_binText);
					}
				}
			}
			else // Not internal binary clipboard data
			{
				const wchar_t* lpwchar = reinterpret_cast<wchar_t*>(pData);
				const size_t nbBytes = (std::wcslen(lpwchar) + 1) * sizeof(wchar_t);
				for (size_t i = 0 ; i < nbBytes ; ++i)
				{
					clipboardData._data.push_back(static_cast<unsigned char>(pData[i]));
				}
			}
			GlobalUnlock(hglb);
		}
	}
	CloseClipboard();
	return clipboardData;
}

ByteArray::ByteArray(const ClipboardDataInfo& cd)
{
	_length = cd._data.size();
	if (!_length)
	{
		_pBytes = NULL;
		return;
	}
	_pBytes = new unsigned char[_length];
	for (size_t i = 0 ; i < _length ; ++i)
	{
		_pBytes[i] = cd._data[i];
	}
}

StringArray::StringArray(const ClipboardDataInfo& cd, size_t maxLen)
{
	size_t len = cd._data.size();

	if (!len)
	{
		_pBytes = NULL;
		return;
	}

	bool isCompleted = (len <= maxLen);
	_length = isCompleted?len:maxLen;

	
	_pBytes = new unsigned char[_length+(isCompleted?0:2)];
	size_t i = 0;
	for ( ; i < _length ; ++i)
	{
		if (!isCompleted && (i == _length-5 || i == _length-3 || i == _length-1))
			_pBytes[i] = 0;
		else if (!isCompleted && (i == _length-6 || i == _length-4 || i == _length-2))
			_pBytes[i] = '.';
		else
			_pBytes[i] = cd._data[i];
	}

	if (!isCompleted)
	{
		_pBytes[i++] = 0;
		_pBytes[i] = 0;
	}
}

void CHPListView::init(HINSTANCE hInst, HWND hParent)
{
	if (_hSelf != nullptr)
	{
		return;
	}

	Window::init(hInst, hParent);

	static constexpr DWORD lvStyles = LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER;
	_hSelf = ::CreateWindowExW(
		0,
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_BORDER | lvStyles,
		0, 0, 0, 0,
		hParent,
		reinterpret_cast<HMENU>(IDC_LIST_CLIPBOARD),
		_hInst,
		nullptr);

	if (_hSelf == nullptr)
	{
		throw std::runtime_error("CHPListView::init : CreateWindowEx() function returns null");
	}

	::SetWindowSubclass(_hParent, CHPListView::ListViewNotifyCustomDrawSubclass, static_cast<UINT_PTR>(SubclassID::first), 0);

	const DWORD exStyle = ListView_GetExtendedListViewStyle(_hSelf)
		| LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_BORDERSELECT | LVS_EX_DOUBLEBUFFER;
	ListView_SetExtendedListViewStyle(_hSelf, exStyle);

	setDarkTheme();

	RECT rcList{};
	::GetClientRect(_hSelf, &rcList);

	LVCOLUMN column{};
	column.mask = LVCF_WIDTH;
	column.cx = rcList.right - rcList.left;
	ListView_InsertColumn(_hSelf, 0, &column);
}

void CHPListView::destroy()
{
	if (_hSelf != nullptr)
	{
		::DestroyWindow(_hSelf);
		_hSelf = nullptr;
	}
}

void CHPListView::reSizeToWH(int width, int height)
{
	::SetWindowPos(_hSelf, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	RECT rcList{};
	::GetClientRect(_hSelf, &rcList);
	ListView_SetColumnWidth(_hSelf, 0, rcList.right - rcList.left);
}

LRESULT CHPListView::listViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
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

			if (isThemeDark && (isSelected || isHot))
			{
				const COLORREF bgColor = isSelected ? NppDarkMode::getCtrlBackgroundColor() : NppDarkMode::getHotBackgroundColor();

				lplvcd->clrText = NppDarkMode::getTextColor();
				lplvcd->clrTextBk = bgColor;

				const auto oldClr = ::SetDCBrushColor(lplvcd->nmcd.hdc, bgColor);
				::FillRect(lplvcd->nmcd.hdc, &rcRow, static_cast<HBRUSH>(::GetStockObject(DC_BRUSH)));
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

LRESULT CALLBACK CHPListView::ListViewNotifyCustomDrawSubclass(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	[[maybe_unused]] DWORD_PTR dwRefData
) noexcept
{
	switch (uMsg)
	{
		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, CHPListView::ListViewNotifyCustomDrawSubclass, uIdSubclass);
			break;
		}

		case WM_NOTIFY:
		{
			const auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);
			if (nmhdr->idFrom == IDC_LIST_CLIPBOARD && nmhdr->code == NM_CUSTOMDRAW)
			{
				return CHPListView::listViewNotifyCustomDraw(hWnd, uMsg, wParam, lParam);
			}
			break;
		}
	}
	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// Search clipboard data in internal storage
// return -1 if not found, else return the index of internal array
int ClipboardHistoryPanel::getClipboardDataIndex(const ClipboardDataInfo& cbd) const
{
	int iFound = -1;
	bool found = false; 
	for (size_t i = 0, len = _clipboardDataInfos.size() ; i < len ; ++i)
	{
		if (cbd._data.size() == _clipboardDataInfos[i]._data.size())
		{
			for (size_t j = 0, len2 = cbd._data.size(); j < len2 ; ++j)
			{
				if (cbd._data[j] == _clipboardDataInfos[i]._data[j])
					found = true;
				else
				{
					found = false;
					break;
				}
			}

			if (found)
			{
				iFound = static_cast<int>(i);
				break;
			}
		}
	}
	return iFound;
}

void ClipboardHistoryPanel::addToClipboadHistory(const ClipboardDataInfo& cbd)
{
	const int idx = getClipboardDataIndex(cbd);
	if (idx == 0)
		return;

	// WM_DRAWCLIPBOARD can happen before WM_INITDIALOG
	// so include init here to have current buffer shown in listview
	_listView.init(_hInst, _hSelf);

	if (auto it = _clipboardDataInfos.begin();
		idx != -1)
	{
		std::rotate(it, it + idx, it + idx + 1);
		ListView_DeleteItem(_listView, idx);
	}
	else
	{
		_clipboardDataInfos.insert(it, cbd);
	}

	StringArray sa(cbd, MAX_DISPLAY_LENGTH);
	std::wstring displayStr;
	if (cbd._isBinaryContained)
	{
		const char* displayStrA = reinterpret_cast<const char*>(sa.getPointer());
		displayStr = string2wstring(displayStrA);
	}
	else
	{
		displayStr = reinterpret_cast<const wchar_t*>(sa.getPointer());
	}

	LVITEM item{};
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	item.pszText = displayStr.data();
	ListView_InsertItem(_listView, &item);
}

intptr_t CALLBACK ClipboardHistoryPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_hwndNextCbViewer = ::SetClipboardViewer(_hSelf);

			_listView.init(_hInst, _hSelf);
			_listView.display();

			return TRUE;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			_listView.setDarkTheme();
			return TRUE;
		}

		case WM_CHANGECBCHAIN:
			if (_hwndNextCbViewer == reinterpret_cast<HWND>(wParam))
				_hwndNextCbViewer = reinterpret_cast<HWND>(lParam);
			else if (_hwndNextCbViewer)
				::SendMessage(_hwndNextCbViewer, message, wParam, lParam);
			return TRUE;

		case WM_DRAWCLIPBOARD:
		{
			if (_isTrackingClipboardOps)
			{
				ClipboardDataInfo clipboardData = getClipboadData();
				if (clipboardData._data.size())
				{
					addToClipboadHistory(clipboardData);
				}
			}

			if (_hwndNextCbViewer)
				::SendMessage(_hwndNextCbViewer, message, wParam, lParam);
			return TRUE;
		}

		case WM_DESTROY:
		{
			::ChangeClipboardChain(_hSelf, _hwndNextCbViewer);
			_listView.destroy();
			break;
		}

		case WM_NOTIFY:
		{
			const auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);

			if (nmhdr->idFrom == IDC_LIST_CLIPBOARD && nmhdr->code == NM_DBLCLK)
			{
				HWND hList = nmhdr->hwndFrom;
				const int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

				if (i >= 0)
				{
					if (static_cast<size_t>(i) < _clipboardDataInfos.size())
					{
						int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
						if (codepage == -1)
						{
							auto cp = (*_ppEditView)->execute(SCI_GETCODEPAGE);
							codepage = cp == SC_CP_UTF8 ? SC_CP_UTF8 : 0;
						}
						else
							codepage = SC_CP_UTF8;

						char* c = nullptr;
						try
						{
							if (_clipboardDataInfos[i]._isBinaryContained)
							{
								(*_ppEditView)->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
								(*_ppEditView)->execute(SCI_ADDTEXT, static_cast<WPARAM>(_clipboardDataInfos[i]._data.size()), reinterpret_cast<LPARAM>(_clipboardDataInfos[i]._data.data()));
							}
							else
							{
								ByteArray ba(_clipboardDataInfos[i]);
								const int nbChar = ::WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(ba.getPointer()), static_cast<int>(ba.getLength()), nullptr, 0, nullptr, nullptr);

								c = new char[nbChar + 1];

								::WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(ba.getPointer()), static_cast<int>(ba.getLength()), c, nbChar + 1, nullptr, nullptr);

								c[nbChar] = '\0';

								(*_ppEditView)->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
								(*_ppEditView)->execute(SCI_ADDTEXT, static_cast<WPARAM>(std::strlen(c)), reinterpret_cast<LPARAM>(c));
								(*_ppEditView)->grabFocus();
								delete[] c;
							}
						}
						catch (...)
						{
							NppDarkMode::darkMessageBoxW(_hSelf, L"Cannot process this clipboard data in the history:\nThe data is too large to be treated.", L"Clipboard problem", MB_OK | MB_APPLMODAL);
							if (c)
								delete[] c;
						}
					}
				}
				return TRUE;
			}

			if (nmhdr->code == DMN_CLOSE)
			{
				::SendMessage(_hParent, WM_COMMAND, IDM_EDIT_CLIPBOARDHISTORY_PANEL, 0);
				return TRUE;
			}
			break;
		}
		
		case WM_SIZE:
		{
			const int width = LOWORD(lParam);
			const int height = HIWORD(lParam);
			_listView.reSizeToWH(width, height);
			break;
		}

		default:
			break;
	}
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}
