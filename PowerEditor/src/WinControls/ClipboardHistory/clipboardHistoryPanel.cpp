/*
this file is part of notepad++
Copyright (C)2011 Don HO <donho@altern.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a Copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "precompiledHeaders.h"
#include "clipboardHistoryPanel.h"
#include "ScintillaEditView.h"
#include "clipboardFormats.h"

void ClipboardHistoryPanel::switchEncoding()
{
	//int codepage = (*_ppEditView)->getCurrentBuffer()->getEncoding();
}

ClipboardData ClipboardHistoryPanel::getClipboadData()
{
	ClipboardData clipboardData;
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
		return clipboardData;

	if (!OpenClipboard(NULL))
		return clipboardData; 
	 
	HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT); 
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
						for (size_t i = 0 ; i < (*lpLen) ; i++)
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
				for (int i = 0 ; i < nbBytes ; i++)
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
	for (size_t i = 0 ; i < _length ; i++)
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
	for ( ; i < _length ; i++)
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

void ClipboardHistoryPanel::addToClipboadHistory(ClipboardData cbd)
{
	_clipboardDataVector.insert(_clipboardDataVector.begin(), cbd);
	//wstring s = clipboardDataToDisplayString(cbd);
	StringArray sa(cbd, 64);
	TCHAR *displayStr = (TCHAR *)sa.getPointer();
	::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_INSERTSTRING, 0, (LPARAM)displayStr);
}

BOOL CALLBACK ClipboardHistoryPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
            ::SetClipboardViewer(_hSelf);
            return TRUE;
        }
		case WM_DRAWCLIPBOARD :
		{
			//::MessageBoxA(NULL, "Catch u", "", MB_OK);
			ClipboardData clipboardData = getClipboadData();
			addToClipboadHistory(clipboardData);
			return TRUE;
		}
		
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

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}
