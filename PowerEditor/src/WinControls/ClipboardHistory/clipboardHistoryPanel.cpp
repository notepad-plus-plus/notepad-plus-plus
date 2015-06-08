// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "clipboardHistoryPanel.h"
#include "ScintillaEditView.h"
#include "clipboardFormats.h"


#define CLIPBOARD_TEXTFORMAT CF_UNICODETEXT
#define MAX_DISPLAY_LENGTH 64

ClipboardData ClipboardHistoryPanel::getClipboadData()
{
	ClipboardData clipboardData;
	if (!IsClipboardFormatAvailable(CLIPBOARD_TEXTFORMAT))
		return clipboardData;

	if (!OpenClipboard(NULL))
		return clipboardData;
	 
	HGLOBAL hglb = GetClipboardData(CLIPBOARD_TEXTFORMAT); 
	if (hglb != NULL) 
	{ 
		char *lpchar = (char *)GlobalLock(hglb);
		wchar_t *lpWchar = (wchar_t *)GlobalLock(hglb);
		
		if (lpchar != NULL) 
		{
			UINT cf_nppTextLen = RegisterClipboardFormat(CF_NPPTEXTLEN);
			if (IsClipboardFormatAvailable(cf_nppTextLen))
			{
				HGLOBAL hglbLen = GetClipboardData(cf_nppTextLen); 
				if (hglbLen != NULL) 
				{ 
					unsigned long *lpLen = (unsigned long *)GlobalLock(hglbLen); 
					if (lpLen != NULL) 
					{
						for (size_t i = 0 ; i < (*lpLen) ; ++i)
						{
							clipboardData.push_back((unsigned char)lpchar[i]);
						}
						GlobalUnlock(hglb); 
					}
				}
			}
			else
			{
				int nbBytes = (lstrlenW(lpWchar) + 1) * sizeof(wchar_t);
				for (int i = 0 ; i < nbBytes ; ++i)
				{
					clipboardData.push_back((unsigned char)lpchar[i]);
				}
			}
			GlobalUnlock(hglb); 
		}
	}
	CloseClipboard();
	return clipboardData;
}

ByteArray::ByteArray(ClipboardData cd)
{
	_length = cd.size();
	if (!_length)
	{
		_pBytes = NULL;
		return;
	}
	_pBytes = new unsigned char[_length];
	for (size_t i = 0 ; i < _length ; ++i)
	{
		_pBytes[i] = cd[i];
	}
}

StringArray::StringArray(ClipboardData cd, size_t maxLen)
{
	if (!cd.size())
	{
		_pBytes = NULL;
		return;
	}

	bool isCompleted = (cd.size() <= maxLen);
	_length = isCompleted?cd.size():maxLen;

	
	_pBytes = new unsigned char[_length+(isCompleted?0:2)];
	size_t i = 0;
	for ( ; i < _length ; ++i)
	{
		if (!isCompleted && (i == _length-5 || i == _length-3 || i == _length-1))
			_pBytes[i] = 0;
		else if (!isCompleted && (i == _length-6 || i == _length-4 || i == _length-2))
			_pBytes[i] = '.';
		else
			_pBytes[i] = cd[i];
	}

	if (!isCompleted)
	{
		_pBytes[i++] = 0;
		_pBytes[i] = 0;
	}
}

// Search clipboard data in internal storage
// return -1 if not found, else return the index of internal array
int ClipboardHistoryPanel::getClipboardDataIndex(ClipboardData cbd)
{
	int iFound = -1;
	bool found = false; 
	for (size_t i = 0, len = _clipboardDataVector.size() ; i < len ; ++i)
	{
		if (cbd.size() == _clipboardDataVector[i].size())
		{
			for (size_t j = 0, len2 = cbd.size(); j < len2 ; ++j)
			{
				if (cbd[j] == _clipboardDataVector[i][j])
					found = true;
				else
				{
					found = false;
					break;
				}
			}

			if (found)
			{
				iFound = i;
				break;
			}
		}
	}
	return iFound;
}

void ClipboardHistoryPanel::addToClipboadHistory(ClipboardData cbd)
{
	int i = getClipboardDataIndex(cbd);
	if (i == 0) return;
	if (i != -1)
	{
		_clipboardDataVector.erase(_clipboardDataVector.begin() + i);
		::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_DELETESTRING, i, 0);
	}
	_clipboardDataVector.insert(_clipboardDataVector.begin(), cbd);

	StringArray sa(cbd, MAX_DISPLAY_LENGTH);
	TCHAR *displayStr = (TCHAR *)sa.getPointer();
	::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_INSERTSTRING, 0, (LPARAM)displayStr);
}


void ClipboardHistoryPanel::drawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct->itemID >= _clipboardDataVector.size())
		return;

	//printStr(TEXT("OK"));
	COLORREF fgColor = _lbFgColor == -1?black:_lbFgColor; // fg black by default
	COLORREF bgColor = _lbBgColor == -1?white:_lbBgColor; // bg white by default
	
	StringArray sa(_clipboardDataVector[lpDrawItemStruct->itemID], MAX_DISPLAY_LENGTH);
	TCHAR *ptStr = (TCHAR *)sa.getPointer();

	//printStr(ptStr);
	::SetTextColor(lpDrawItemStruct->hDC, fgColor);
	::SetBkColor(lpDrawItemStruct->hDC, bgColor);
	
	::DrawText(lpDrawItemStruct->hDC, ptStr, lstrlen(ptStr), &(lpDrawItemStruct->rcItem), DT_SINGLELINE | DT_VCENTER | DT_LEFT);
}

INT_PTR CALLBACK ClipboardHistoryPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			_hwndNextCbViewer = ::SetClipboardViewer(_hSelf);
            return TRUE;
        }

		case WM_CHANGECBCHAIN:
			if (_hwndNextCbViewer == (HWND)wParam)
				_hwndNextCbViewer = (HWND)lParam;
			else if (_hwndNextCbViewer)
				::SendMessage(_hwndNextCbViewer, message, wParam, lParam);
			return TRUE;

		case WM_DRAWCLIPBOARD :
		{
			ClipboardData clipboardData = getClipboadData();
			if (clipboardData.size())
				addToClipboadHistory(clipboardData);
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
						int i = ::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_GETCURSEL, 0, 0);
						if (i != LB_ERR)
						{
							int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
							if (codepage == -1)
							{
								int cp = (*_ppEditView)->execute(SCI_GETCODEPAGE);
								codepage = cp==SC_CP_UTF8?SC_CP_UTF8:0;
							}
							else
								codepage = SC_CP_UTF8;

							ByteArray ba(_clipboardDataVector[i]);

							int nbChar = WideCharToMultiByte(codepage, 0, (wchar_t *)ba.getPointer(), ba.getLength(), NULL, 0, NULL, NULL);

							char *c = new char[nbChar+1];
							WideCharToMultiByte(codepage, 0, (wchar_t *)ba.getPointer(), ba.getLength(), c, nbChar+1, NULL, NULL);

							(*_ppEditView)->execute(SCI_REPLACESEL, 0, (LPARAM)"");
							(*_ppEditView)->execute(SCI_ADDTEXT, strlen(c), (LPARAM)c);
							(*_ppEditView)->getFocus();
							delete [] c;
						}
					}
					return TRUE;
				}
			}
		}
		break;
		
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
			::MoveWindow(::GetDlgItem(_hSelf, IDC_LIST_CLIPBOARD), 0, 0, width, height, TRUE);
            break;
        }
/*
		case WM_VKEYTOITEM:
		{
			if (LOWORD(wParam) == VK_RETURN)
			{
				int i = ::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_GETCURSEL, 0, 0);
				printInt(i);
				return TRUE;
			}//return TRUE;
			break;
		}
*/
		case WM_CTLCOLORLISTBOX:
		{
			if (_lbBgColor != -1)
				return (LRESULT)::CreateSolidBrush((COLORREF)_lbBgColor);
			break;
		}

		case WM_DRAWITEM:
		{
			drawItem((DRAWITEMSTRUCT *)lParam);
			break;
		}
        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

