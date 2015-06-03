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


#include "FindCharsInRange.h"
#include "FindCharsInRange_rc.h"

INT_PTR CALLBACK FindCharsInRangeDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			::SendDlgItemMessage(_hSelf, IDC_RANGESTART_EDIT, EM_LIMITTEXT, 3, 0);
			::SendDlgItemMessage(_hSelf, IDC_RANGEEND_EDIT, EM_LIMITTEXT, 3, 0);
			::SendDlgItemMessage(_hSelf, IDC_NONASCCI_RADIO, BM_SETCHECK, TRUE, 0);
			::SendDlgItemMessage(_hSelf, ID_FINDCHAR_DIRDOWN, BM_SETCHECK, TRUE, 0);
			goToCenter();
			return TRUE;
		}
		case WM_COMMAND :
		{
			switch (wParam)
			{
				case IDCANCEL : // Close
					display(false);
					return TRUE;

				case ID_FINDCHAR_NEXT:
				{
					int currentPos = (*_ppEditView)->execute(SCI_GETCURRENTPOS);
					unsigned char startRange = 0;
					unsigned char endRange = 255;
					bool direction = dirDown;
					bool isWrap = true;
					if (!getRangeFromUI(startRange, endRange))
					{
						//STOP!
						::MessageBox(_hSelf, TEXT("You should type between from 0 to 255."), TEXT("Range Value problem"), MB_OK);
						return TRUE;
					}
					getDirectionFromUI(direction, isWrap);
					findCharInRange(startRange, endRange, currentPos, direction, isWrap);
					return TRUE;
				}

				default :
				{
					break;
				}
			}
		}
		default :
			return FALSE;
	}
}

bool FindCharsInRangeDlg::findCharInRange(unsigned char beginRange, unsigned char endRange, int startPos, bool direction, bool wrap)
{
	int totalSize = (*_ppEditView)->getCurrentDocLen();
	if (startPos == -1)
		startPos = direction==dirDown?0:totalSize-1;
	if (startPos > totalSize)
		return false;

	char *content = new char[totalSize+1];
	(*_ppEditView)->getText(content, 0, totalSize);
	int found = -1;

	for (int i = startPos-(direction == dirUp?1:0); 
		(direction == dirDown)?i < totalSize:i >= 0 ;
		(direction == dirDown)?(++i):(--i))
	{
		if ((unsigned char)content[i] >= beginRange && (unsigned char)content[i] <= endRange)
		{
			found = i;
			break;
		}
	}
	
	if (found == -1)
	{
		if (wrap)
		{
			for (int i = (direction == dirUp?totalSize-1:0); 
				(direction == dirDown)?i < totalSize:i >= 0 ;
				(direction == dirDown)?(++i):(--i))
			{
				if ((unsigned char)content[i] >= beginRange && (unsigned char)content[i] <= endRange)
				{
					found = i;
					break;
				}
			}
		}
	}

	if (found != -1)
	{
		//printInt(found);
		int sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, found);
		(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
		(*_ppEditView)->execute(SCI_GOTOPOS, found);
		(*_ppEditView)->execute(SCI_SETSEL, (direction == dirDown)?found:found+1, (direction == dirDown)?found+1:found);
	}
	delete [] content;
	return (found != -1);
}

void FindCharsInRangeDlg::getDirectionFromUI(bool & whichDirection, bool & isWrap)
{
	whichDirection = isCheckedOrNot(ID_FINDCHAR_DIRUP);
	isWrap = isCheckedOrNot(ID_FINDCHAR_WRAP);
}

bool FindCharsInRangeDlg::getRangeFromUI(unsigned char & startRange, unsigned char & endRange)
{
	if (isCheckedOrNot(IDC_NONASCCI_RADIO))
	{
		startRange = 128;
		endRange = 255;
		return true;
	}
	if (isCheckedOrNot(IDC_ASCCI_RADIO))
	{
		startRange = 0;
		endRange = 127;
		return true;
	}
	
	if (isCheckedOrNot(IDC_MYRANGE_RADIO))
	{
		BOOL startBool, endBool;
		int start = ::GetDlgItemInt(_hSelf, IDC_RANGESTART_EDIT, &startBool, FALSE);
		int end = ::GetDlgItemInt(_hSelf, IDC_RANGEEND_EDIT, &endBool, FALSE);

		if (!startBool || !endBool)
			return false;
		if (start > 255 || end > 255)
			return false;
		if (start > end)
			return false;
		startRange = (unsigned char)start;
		endRange = (unsigned char)end;
		return true;
	}
	
	return false;
}
