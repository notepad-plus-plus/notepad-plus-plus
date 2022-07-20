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



#include "DocTabView.h"
#include "ScintillaEditView.h"

#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE


bool DocTabView::_hideTabBarStatus = false;

void DocTabView::addBuffer(BufferID buffer)
{
	if (buffer == BUFFER_INVALID)	//valid only
		return;
	if (getIndexByBuffer(buffer) != -1)	//no duplicates
		return;
	Buffer * buf = MainFileManager.getBufferByID(buffer);
	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;

	int index = -1;
	if (_hasImgLst)
		index = 0;
	tie.iImage = index;
	tie.pszText = const_cast<TCHAR *>(buf->getFileName());
	tie.lParam = reinterpret_cast<LPARAM>(buffer);
	::SendMessage(_hSelf, TCM_INSERTITEM, _nbItem++, reinterpret_cast<LPARAM>(&tie));
	bufferUpdated(buf, BufferChangeMask);

	::SendMessage(_hParent, WM_SIZE, 0, 0);
}


void DocTabView::closeBuffer(BufferID buffer)
{
	int indexToClose = getIndexByBuffer(buffer);
	deletItemAt((size_t)indexToClose);
	::SendMessage(_hParent, WM_SIZE, 0, 0);
}


bool DocTabView::activateBuffer(BufferID buffer)
{
	int indexToActivate = getIndexByBuffer(buffer);
	if (indexToActivate == -1)
		return false;	//cannot activate

	activateAt(indexToActivate);
	return true;
}


BufferID DocTabView::activeBuffer()
{
	int index = getCurrentTabIndex();
	return static_cast<BufferID>(getBufferByIndex(index));
}


BufferID DocTabView::findBufferByName(const TCHAR * fullfilename) //-1 if not found, something else otherwise
{
	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	for (size_t i = 0; i < _nbItem; ++i)
	{
		::SendMessage(_hSelf, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tie));
		BufferID id = reinterpret_cast<BufferID>(tie.lParam);
		Buffer * buf = MainFileManager.getBufferByID(id);
		if (OrdinalIgnoreCaseCompareStrings(fullfilename, buf->getFullPathName()) == 0)
		{
			return id;
		}
	}
	return BUFFER_INVALID;
}


int DocTabView::getIndexByBuffer(BufferID id)
{
	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	for (size_t i = 0; i < _nbItem; ++i)
	{
		::SendMessage(_hSelf, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tie));
		if (reinterpret_cast<BufferID>(tie.lParam) == id)
			return static_cast<int>(i);
	}
	return -1;
}


BufferID DocTabView::getBufferByIndex(size_t index)
{
	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	::SendMessage(_hSelf, TCM_GETITEM, index, reinterpret_cast<LPARAM>(&tie));

	return reinterpret_cast<BufferID>(tie.lParam);
}


void DocTabView::bufferUpdated(Buffer * buffer, int mask)
{
	int index = getIndexByBuffer(buffer->getID());
	if (index == -1)
		return;

	TCITEM tie;
	tie.lParam = -1;
	tie.mask = 0;

	if (mask & BufferChangeReadonly || mask & BufferChangeDirty)
	{
		tie.mask |= TCIF_IMAGE;
		tie.iImage = buffer->isDirty()?UNSAVED_IMG_INDEX:SAVED_IMG_INDEX;
		if (buffer->isMonitoringOn())
		{
			tie.iImage = MONITORING_IMG_INDEX;
		}
		else if (buffer->isReadOnly())
		{
			tie.iImage = REDONLY_IMG_INDEX;
		}
	}

	//We must make space for the added ampersand characters.
	TCHAR encodedLabel[2 * MAX_PATH];

	if (mask & BufferChangeFilename)
	{
		tie.mask |= TCIF_TEXT;
		tie.pszText = const_cast<TCHAR *>(encodedLabel);

		{
			const TCHAR* in = buffer->getFileName();
			TCHAR* out = encodedLabel;

			//This code will read in one character at a time and duplicate every first ampersand(&).
			//ex. If input is "test & test && test &&&" then output will be "test && test &&& test &&&&".
			//Tab's caption must be encoded like this because otherwise tab control would make tab too small or too big for the text.

			while (*in != 0)
			if (*in == '&')
			{
				*out++ = '&';
				*out++ = '&';
				while (*(++in) == '&')
					*out++ = '&';
			}
			else
				*out++ = *in++;
			*out = '\0';
		}
	}

	::SendMessage(_hSelf, TCM_SETITEM, index, reinterpret_cast<LPARAM>(&tie));

	// send WM_SIZE only when change tab
	// It is needed while a tab is closed (so tab changed) in multi-line tab mode
	if (mask & BufferChangeRecentTag)
		::SendMessage(_hParent, WM_SIZE, 0, 0);
}


void DocTabView::setBuffer(size_t index, BufferID id)
{
	if (index < 0 || index >= _nbItem)
		return;

	TCITEM tie;
	tie.lParam = reinterpret_cast<LPARAM>(id);
	tie.mask = TCIF_PARAM;
	::SendMessage(_hSelf, TCM_SETITEM, index, reinterpret_cast<LPARAM>(&tie));

	bufferUpdated(MainFileManager.getBufferByID(id), BufferChangeMask);	//update tab, everything has changed

	::SendMessage(_hParent, WM_SIZE, 0, 0);
}


void DocTabView::reSizeTo(RECT & rc)
{
	int borderWidth = ((NppParameters::getInstance()).getSVP())._borderWidth;
	if (_hideTabBarStatus)
	{
		RECT rcTmp = rc;
		TabBar::reSizeTo(rcTmp);
		_pView->reSizeTo(rc);
	}
	else
	{
		TabBar::reSizeTo(rc);
		rc.left	 += borderWidth;
		rc.right -= borderWidth * 2;
		rc.top   += borderWidth;
		rc.bottom -= (borderWidth * 2);
		_pView->reSizeTo(rc);
	}
	SendMessage(_hParent, NPPM_INTERNAL_UPDATECLICKABLELINKS, reinterpret_cast<WPARAM>(_pView), 0);
}

