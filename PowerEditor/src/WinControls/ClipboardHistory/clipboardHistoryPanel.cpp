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
#include "ScintillaEditView.h"
#include "clipboardFormats.h"
#include <algorithm>

void ClipboardHistoryPanel::destroyListBoxFont() noexcept
{
	if (_hListBoxFont)
	{
		::DeleteObject(_hListBoxFont);
		_hListBoxFont = nullptr;
	}
}

void ClipboardHistoryPanel::createListBoxFontAndHeight()
{
	destroyListBoxFont();

	UINT dpi = _dpiManager.getDpi();
	LOGFONT lf = DPIManagerV2::getDefaultGUIFontForDpi(dpi, DPIManagerV2::FontType::message);
	_hListBoxFont = ::CreateFontIndirect(&lf);

	HWND hListBox = ::GetDlgItem(_hSelf, IDC_LIST_CLIPBOARD);
	if (!hListBox)
		return;

	::SendMessage(hListBox, WM_SETFONT, reinterpret_cast<WPARAM>(_hListBoxFont), FALSE);

	HDC hdc = ::GetDC(hListBox);
	if (hdc)
	{
		HFONT hOldFont = static_cast<HFONT>(::SelectObject(hdc, _hListBoxFont));
		TEXTMETRIC tm{};
		::GetTextMetrics(hdc, &tm);
		::SelectObject(hdc, hOldFont);
		::ReleaseDC(hListBox, hdc);

		int padding = _dpiManager.scale(4);
		int itemHeight = tm.tmHeight + padding;

		::SendMessage(hListBox, LB_SETITEMHEIGHT, 0, static_cast<LPARAM>(itemHeight));
	}

	for (auto& item : _clipboardDataInfos)
	{
		item.clearDisplayCache();
	}
}

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
							const unsigned long* lpLen = (unsigned long*)GlobalLock(hglbLen);
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
				wchar_t* lpwchar = (wchar_t*)pData;
				size_t nbBytes = (lstrlenW(lpwchar) + 1) * sizeof(wchar_t);
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

// Search clipboard data in internal storage
// return -1 if not found, else return the index of internal array
int ClipboardHistoryPanel::getClipboardDataIndex(const ClipboardDataInfo& cbd)
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
				iFound = static_cast<int32_t>(i);
				break;
			}
		}
	}
	return iFound;
}

void ClipboardHistoryPanel::addToClipboadHistory(ClipboardDataInfo cbd)
{
	int i = getClipboardDataIndex(cbd);
	if (i == 0) return;
	if (i != -1)
	{
		_clipboardDataInfos.erase(_clipboardDataInfos.begin() + i);
		::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_DELETESTRING, i, 0);
	}
	_clipboardDataInfos.insert(_clipboardDataInfos.begin(), cbd);

	::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(L"")); // String will be added in drawItem()
}

const std::wstring& ClipboardHistoryPanel::getDisplayString(HDC hdc, const ClipboardDataInfo& cbd, int maxPixelWidth) const
{
	static const std::wstring emptyStr = L"";
	if (cbd._data.empty() || maxPixelWidth <= 0)
		return emptyStr;

	if (cbd._cachedWidth == maxPixelWidth && !cbd._cachedDisplayStr.empty())
	{
		return cbd._cachedDisplayStr;
	}

	if (!cbd._isCleaned)
	{
		if (cbd._isBinaryContained)
		{
			constexpr size_t maxBytesToCheck = 2048;
			size_t examineLen = (std::min)(cbd._data.size(), maxBytesToCheck);
			std::string tempStr(reinterpret_cast<const char*>(cbd._data.data()), examineLen);
			cbd._cleanedFullStr = string2wstring(tempStr, CP_UTF8);
		}
		else
		{
			size_t totalWChars = cbd._data.size() / sizeof(wchar_t);
			const wchar_t* pWChars = reinterpret_cast<const wchar_t*>(cbd._data.data());
			while (totalWChars > 0 && pWChars[totalWChars - 1] == L'\0')
				--totalWChars;

			constexpr size_t maxCharsToCheck = 1024;
			size_t examineLen = (std::min)(totalWChars, maxCharsToCheck);
			cbd._cleanedFullStr.assign(pWChars, examineLen);
		}

		bool hasSpecialChar = std::any_of(cbd._cleanedFullStr.begin(), cbd._cleanedFullStr.end(), [](wchar_t ch) {
			return ch == L'\r' || ch == L'\n' || ch == L'\t' || ch == L'\0';
		});

		if (hasSpecialChar)
		{
			std::wstring escapedStr;
			escapedStr.reserve(cbd._cleanedFullStr.size() * 3 + 32);
			for (wchar_t ch : cbd._cleanedFullStr)
			{
				switch (ch)
				{
					case L'\0': escapedStr += L"[NUL]"; break;
					case L'\r': escapedStr += L"[CR]";   break;
					case L'\n': escapedStr += L"[LF]";   break;
					case L'\t': escapedStr += L"[TAB]";   break;
					default:    escapedStr += ch;       break;
				}
			}
			cbd._cleanedFullStr = std::move(escapedStr);
		}

		cbd._isCleaned = true;
	}

	if (cbd._cleanedFullStr.empty())
		return emptyStr;

	const wchar_t* ellipsis = L"...";
	const int ellipsisLen = 3;
	SIZE ellipsisSize{};
	::GetTextExtentPoint32W(hdc, ellipsis, ellipsisLen, &ellipsisSize);

	SIZE fullSize{};
	::GetTextExtentPoint32W(hdc, cbd._cleanedFullStr.c_str(), static_cast<int>(cbd._cleanedFullStr.length()), &fullSize);

	cbd._cachedWidth = maxPixelWidth;

	if (fullSize.cx <= maxPixelWidth)
	{
		cbd._cachedDisplayStr = cbd._cleanedFullStr;
		return cbd._cachedDisplayStr;
	}

	int targetWidth = maxPixelWidth - ellipsisSize.cx;
	if (targetWidth <= 0)
	{
		cbd._cachedDisplayStr = ellipsis;
		return cbd._cachedDisplayStr;
	}

	int fitCount = 0;
	SIZE fitSize{};
	if (!::GetTextExtentExPointW(hdc, cbd._cleanedFullStr.c_str(), static_cast<int>(cbd._cleanedFullStr.length()), targetWidth, &fitCount, nullptr, &fitSize))
	{
		cbd._cachedDisplayStr = ellipsis;
		return cbd._cachedDisplayStr;
	}

	if (fitCount > 0 && fitCount < static_cast<int>(cbd._cleanedFullStr.length()))
	{
		wchar_t lastChar = cbd._cleanedFullStr[fitCount - 1];
		if (lastChar >= 0xD800 && lastChar <= 0xDBFF)
			--fitCount;
	}

	cbd._cachedDisplayStr = cbd._cleanedFullStr.substr(0, fitCount) + ellipsis;
	return cbd._cachedDisplayStr;
}

void ClipboardHistoryPanel::drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	UINT i = lpDrawItemStruct->itemID;
	if (i == static_cast<UINT>(-1) || i >= _clipboardDataInfos.size())
		return;

	if (lpDrawItemStruct->itemAction == ODA_FOCUS)
		return;

	//printStr(L"OK");
	HDC hdc = lpDrawItemStruct->hDC;
	const RECT& rcItem = lpDrawItemStruct->rcItem;

	COLORREF fgColor = _lbFgColor == -1 ? black : _lbFgColor; // fg black by default
	COLORREF bgColor = _lbBgColor == -1 ? white : _lbBgColor; // bg white by default
	
	HBRUSH hBgBrush = ::CreateSolidBrush(bgColor);
	::FillRect(hdc, &rcItem, hBgBrush);
	::DeleteObject(hBgBrush);

	int paddingLeft = _dpiManager.scale(4);
	int paddingRight = _dpiManager.scale(4);
	int itemWidth = rcItem.right - rcItem.left;
	int availPixelWidth = (std::max)(0, itemWidth - paddingLeft - paddingRight);

	const std::wstring& displayStr = getDisplayString(hdc, _clipboardDataInfos[i], availPixelWidth);

	::SetTextColor(hdc, fgColor);
	::SetBkColor(hdc, bgColor);
	::SetBkMode(hdc, TRANSPARENT);

	RECT rcDraw = rcItem;
	rcDraw.left += paddingLeft;
	rcDraw.right -= paddingRight;

	::DrawText(hdc, displayStr.c_str(), static_cast<int>(displayStr.length()), &rcDraw, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
}

intptr_t CALLBACK ClipboardHistoryPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_hwndNextCbViewer = ::SetClipboardViewer(_hSelf);
			createListBoxFontAndHeight();
			NppDarkMode::setDarkScrollBar(::GetDlgItem(_hSelf, IDC_LIST_CLIPBOARD));
			return TRUE;
		}

		case WM_DPICHANGED:
		{
			_dpiManager.setDpiWP(wParam);
			setPositionDpi(lParam);
			createListBoxFontAndHeight();
			return TRUE;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::setDarkScrollBar(GetDlgItem(_hSelf, IDC_LIST_CLIPBOARD));
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
			destroyListBoxFont();
			::ChangeClipboardChain(_hSelf, _hwndNextCbViewer);
			break;

		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDC_LIST_CLIPBOARD:
				{
					if (HIWORD(wParam) == LBN_DBLCLK)
					{
						auto i = ::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_GETCURSEL, 0, 0);
						if (i != LB_ERR)
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
							try {
								if (_clipboardDataInfos[i]._isBinaryContained)
								{
									(*_ppEditView)->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
									(*_ppEditView)->execute(SCI_ADDTEXT, _clipboardDataInfos[i]._data.size(), reinterpret_cast<LPARAM>(&(_clipboardDataInfos[i]._data[0])));
								}
								else
								{
									ByteArray ba(_clipboardDataInfos[i]);
									int nbChar = WideCharToMultiByte(codepage, 0, (wchar_t*)ba.getPointer(), static_cast<int32_t>(ba.getLength()), NULL, 0, NULL, NULL);

									c = new char[nbChar + 1];
									WideCharToMultiByte(codepage, 0, (wchar_t*)ba.getPointer(), static_cast<int32_t>(ba.getLength()), c, nbChar + 1, NULL, NULL);

									(*_ppEditView)->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
									(*_ppEditView)->execute(SCI_ADDTEXT, strlen(c), reinterpret_cast<LPARAM>(c));
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
			}
		}
		break;

		case WM_NOTIFY:
		{
			if (reinterpret_cast<LPNMHDR>(lParam)->code == DMN_CLOSE)
				::SendMessage(_hParent, WM_COMMAND, IDM_EDIT_CLIPBOARDHISTORY_PANEL, 0);
			break;
		}
		
		case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			HWND hListBox = ::GetDlgItem(_hSelf, IDC_LIST_CLIPBOARD);
			::MoveWindow(hListBox, 0, 0, width, height, TRUE);

			if (_lastClientWidth != -1 && width != _lastClientWidth)
			{
				::InvalidateRect(hListBox, NULL, FALSE);
			}
			_lastClientWidth = width;
			break;
		}

		case WM_CTLCOLORLISTBOX:
		{
			if (_lbBgColor != -1)
			{
				::SetDCBrushColor(reinterpret_cast<HDC>(wParam), _lbBgColor);
				return reinterpret_cast<LRESULT>(::GetStockObject(DC_BRUSH));
			}
			break;
		}

		case WM_DRAWITEM:
		{
			drawItem(reinterpret_cast<DRAWITEMSTRUCT *>(lParam));
			break;
		}
		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

