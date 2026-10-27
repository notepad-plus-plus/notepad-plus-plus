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
#include <string>

#include <Scintilla.h>

#include "Common.h"
#include "DockingDlgInterface.h"
#include "ListView.h"
#include "NppDarkMode.h"
#include "ScintillaEditView.h"
#include "Window.h"
#include "clipboardFormats.h"
#include "clipboardHistoryPanel_rc.h"
#include "dockingResource.h"
#include "menuCmdID.h"
#include "resource.h"

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

void CBHListView::init(HINSTANCE hInst, HWND hParent)
{
	if (_hSelf != nullptr)
	{
		return;
	}

	ListViewBase::init(hInst, hParent, IDC_LIST_CLIPBOARD,
		LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
		LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_BORDERSELECT | LVS_EX_DOUBLEBUFFER);

	if (_hSelf == nullptr)
	{
		return;
	}

	RECT rcList{};
	::GetClientRect(_hSelf, &rcList);

	LVCOLUMN column{};
	column.mask = LVCF_WIDTH;
	column.cx = rcList.right - rcList.left;
	ListView_InsertColumn(_hSelf, 0, &column);
}

void CBHListView::reSizeToWH(int width, int height)
{
	::SetWindowPos(_hSelf, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE); // SWP_NOMOVE must not be used due to RTL

	RECT rcList{};
	::GetClientRect(_hSelf, &rcList);
	ListView_SetColumnWidth(_hSelf, 0, rcList.right - rcList.left);
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

std::wstring ClipboardHistoryPanel::getDisplayStr(const ClipboardDataInfo& cbd)
{
	const auto& data = cbd._data;
	static constexpr size_t maxLen = MAX_PATH - 3; // list view label hard limit
	const size_t nBytes = data.size();
	std::wstring displayStr;
	bool truncated = false;

	if (cbd._isBinaryContained)
	{
		const auto len = std::min<size_t>(nBytes, maxLen);
		truncated = nBytes > len;

		const auto raw = std::string(data.begin(), data.begin() + len);
		displayStr = string2wstring(raw);
	}
	else if (nBytes % sizeof(wchar_t) == 0)
	{
		size_t wLen = nBytes / sizeof(wchar_t);
		auto wStr = reinterpret_cast<const wchar_t*>(data.data());

		// Remove extra null terminator
		if (wLen > 0 && wStr[wLen - 1] == L'\0')
		{
			--wLen;
		}

		const size_t len = std::min<size_t>(wLen, maxLen - 1);
		truncated = wLen > len;

		displayStr.assign(wStr, len);
	}
	else
	{
		// Cannot display this clipboard data.
		displayStr = L"";
	}

	if (truncated)
	{
		// this will be over maxLen, but it is already accounted
		displayStr += L"...";
	}

	return displayStr;
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

	std::wstring displayStr = getDisplayStr(cbd);

	LVITEM item{};
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	item.pszText = displayStr.data();
	ListView_InsertItem(_listView, &item);
}

void ClipboardHistoryPanel::onDoubleClick(HWND hList)
{
	const int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
	if (i < 0 || static_cast<size_t>(i) >= _clipboardDataInfos.size())
	{
		return;
	}

	int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
	if (codepage == -1)
	{
		const auto cp = (*_ppEditView)->execute(SCI_GETCODEPAGE);
		codepage = (cp == SC_CP_UTF8) ? SC_CP_UTF8 : 0;
	}
	else
	{
		codepage = SC_CP_UTF8;
	}

	char* c = nullptr;
	try
	{
		if (const auto& data = _clipboardDataInfos[i]._data;
			_clipboardDataInfos[i]._isBinaryContained)
		{
			(*_ppEditView)->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
			(*_ppEditView)->execute(SCI_ADDTEXT, static_cast<WPARAM>(data.size()), reinterpret_cast<LPARAM>(data.data()));
		}
		else
		{
			const int nbChar = ::WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(data.data()), static_cast<int>(data.size()), nullptr, 0, nullptr, nullptr);

			c = new char[nbChar + 1];

			::WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(data.data()), static_cast<int>(data.size()), c, nbChar + 1, nullptr, nullptr);

			c[nbChar] = '\0';

			(*_ppEditView)->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
			(*_ppEditView)->execute(SCI_ADDTEXT, static_cast<WPARAM>(std::strlen(c)), reinterpret_cast<LPARAM>(c));
			delete[] c;
		}
		(*_ppEditView)->grabFocus();
	}
	catch (...)
	{
		NppDarkMode::darkMessageBoxW(_hSelf, L"Cannot process this clipboard data in the history:\nThe data is too large to be treated.", L"Clipboard problem", MB_OK | MB_APPLMODAL);
		if (c)
			delete[] c;
	}
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
		{
			if (_hwndNextCbViewer == reinterpret_cast<HWND>(wParam))
				_hwndNextCbViewer = reinterpret_cast<HWND>(lParam);
			else if (_hwndNextCbViewer)
				::SendMessage(_hwndNextCbViewer, message, wParam, lParam);
			return TRUE;
		}

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
				onDoubleClick(nmhdr->hwndFrom);
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
