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
							unsigned long* lpLen = (unsigned long*)GlobalLock(hglbLen);
							if (lpLen != NULL) // Special copy-paste: Binary data
							{
								size_t nbBytes = (*lpLen);
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


void ClipboardHistoryPanel::drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	UINT i = lpDrawItemStruct->itemID;
	if (i >= _clipboardDataInfos.size())
		return;

	//printStr(TEXT("OK"));
	COLORREF fgColor = _lbFgColor == -1?black:_lbFgColor; // fg black by default
	COLORREF bgColor = _lbBgColor == -1?white:_lbBgColor; // bg white by default
	
	ClipboardDataInfo& cbd = _clipboardDataInfos[i];
	StringArray sa(cbd, MAX_DISPLAY_LENGTH);
	TCHAR* displayStr = nullptr;
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	if (cbd._isBinaryContained)
	{
		char* displayStrA = (char*)sa.getPointer();
		displayStr = (TCHAR*)wmc.char2wchar(displayStrA, SC_CP_UTF8);
	}
	else
	{
		displayStr = (TCHAR*)sa.getPointer();
	}

	::SetTextColor(lpDrawItemStruct->hDC, fgColor);
	::SetBkColor(lpDrawItemStruct->hDC, bgColor);
	
	::DrawText(lpDrawItemStruct->hDC, displayStr, lstrlen(displayStr), &(lpDrawItemStruct->rcItem), DT_SINGLELINE | DT_VCENTER | DT_LEFT);
}

intptr_t CALLBACK ClipboardHistoryPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_hwndNextCbViewer = ::SetClipboardViewer(_hSelf);
			NppDarkMode::setDarkScrollBar(::GetDlgItem(_hSelf, IDC_LIST_CLIPBOARD));
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
									(*_ppEditView)->getFocus();
									delete[] c;
								}
							}
							catch (...)
							{
								MessageBox(_hSelf,	TEXT("Cannot process this clipboard data in the history:\nThe data is too large to be treated."), TEXT("Clipboard problem"), MB_OK | MB_APPLMODAL);
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
			if (((LPNMHDR)lParam)->code == DMN_CLOSE)
				::SendMessage(_hParent, WM_COMMAND, IDM_EDIT_CLIPBOARDHISTORY_PANEL, 0);
			break;
		}
		
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
			::MoveWindow(::GetDlgItem(_hSelf, IDC_LIST_CLIPBOARD), 0, 0, width, height, TRUE);
            break;
        }

		case WM_CTLCOLORLISTBOX:
		{
			if (_lbBgColor != -1)
				return reinterpret_cast<LRESULT>(::CreateSolidBrush(_lbBgColor));
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

