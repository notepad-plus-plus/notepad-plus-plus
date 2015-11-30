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
	if (this->getIndexByBuffer(buffer) != -1)	//no duplicates
		return;
	Buffer * buf = MainFileManager->getBufferByID(buffer);
	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;

	int index = -1;
	if (_hasImgLst)
	{
		index = addIconForFilename(buf->getFileName());
	}

	tie.iImage = index;
	tie.pszText = (TCHAR *)buf->getFileName();
	tie.lParam = (LPARAM)buffer;
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
	return (BufferID)getBufferByIndex(index);
}


BufferID DocTabView::findBufferByName(const TCHAR * fullfilename) //-1 if not found, something else otherwise
{
	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	for(size_t i = 0; i < _nbItem; ++i)
	{
		::SendMessage(_hSelf, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tie));
		BufferID id = (BufferID)tie.lParam;
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (!lstrcmp(fullfilename, buf->getFullPathName()))
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
	for(int i = 0; i < (int)_nbItem; ++i)
	{
		::SendMessage(_hSelf, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tie));
		if ((BufferID)tie.lParam == id)
			return i;
	}
	return -1;
}


BufferID DocTabView::getBufferByIndex(int index)
{
	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	::SendMessage(_hSelf, TCM_GETITEM, index, reinterpret_cast<LPARAM>(&tie));

	return (BufferID)tie.lParam;
}


void DocTabView::bufferUpdated(Buffer * buffer, int mask)
{
	int index = getIndexByBuffer(buffer->getID());
	if (index == -1)
		return;

	TCITEM current;
	current.mask = TCIF_IMAGE;
	::SendMessage(_hSelf, TCM_GETITEM, index, reinterpret_cast<LPARAM>(&current));

	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_IMAGE;
	tie.iImage = current.iImage;

	//We must make space for the added ampersand characters.
	TCHAR encodedLabel[2 * MAX_PATH];

	if (mask & BufferChangeFilename)
	{
		tie.mask |= TCIF_TEXT;
		tie.iImage = addIconForFilename(buffer->getFileName());
		tie.pszText = (TCHAR *)encodedLabel;

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


void DocTabView::setBuffer(int index, BufferID id)
{
	if (index < 0 || index >= (int)_nbItem)
		return;

	TCITEM tie;
	tie.lParam = (LPARAM)id;
	tie.mask = TCIF_PARAM;
	::SendMessage(_hSelf, TCM_SETITEM, index, reinterpret_cast<LPARAM>(&tie));

	bufferUpdated(MainFileManager->getBufferByID(id), BufferChangeMask);	//update tab, everything has changed

	::SendMessage(_hParent, WM_SIZE, 0, 0);
}


void DocTabView::reSizeTo(RECT & rc)
{
	int borderWidth = ((NppParameters::getInstance())->getSVP())._borderWidth;
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
}

int DocTabView::addIconForFilename(const TCHAR * name)
{
	int index = -1;
	SHFILEINFO fileInfo;
	SHGetFileInfo(
		name,
		FILE_ATTRIBUTE_NORMAL,
		&fileInfo,
		sizeof(fileInfo),
		SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_ICON);

	if (fileInfo.iIcon != 0)
		index = _pIconList->addExternalIcon(CopyIcon(fileInfo.hIcon));
	if (index == -1)
		index = 0;
	return index;
}

void DocTabView::drawImage(int tabIndex, int imgIndex, HDC hDC, int xPos, int yPos)
{
	if (tabIndex < 0 || imgIndex < 0)
		return;

	BufferID id = getBufferByIndex(tabIndex);
	Buffer * buf = MainFileManager->getBufferByID(id);
	HIMAGELIST imgLst = (HIMAGELIST)::SendMessage(_hSelf, TCM_GETIMAGELIST, 0, 0);

	bool unsaved = buf->isDirty();
	bool readOnly = buf->isReadOnly();

	IMAGELISTDRAWPARAMS params;
	params.cbSize = sizeof(params);
	params.himl = imgLst;
	params.hdcDst = hDC;
	params.i = imgIndex;
	params.x = xPos;
	params.y = yPos;
	params.cx = 0;
	params.cy = 0;
	params.xBitmap = 0;
	params.yBitmap = 0;
	params.rgbBk = CLR_NONE;
	params.rgbFg = CLR_NONE;
	params.crEffect = 0;

	// Draw a red background and then the image
	HBRUSH hbrush = CreateSolidBrush(darkRed);
	HBRUSH holdbrush = (HBRUSH)SelectObject(hDC, hbrush);

	params.fStyle = unsaved ? ILD_MASK | ILD_ROP : ILD_TRANSPARENT;
	params.dwRop = unsaved ? 0x00B8074A : 0;
	params.fState = readOnly ? ILS_SATURATE: 0;
	params.Frame = 0;
	ImageList_DrawIndirect(&params);

	SelectObject(hDC, holdbrush);

	if (unsaved)
	{
		params.fStyle = ILD_TRANSPARENT;
		params.dwRop = 0;
		params.fState = ILS_ALPHA;
		params.Frame = UNSAVED_IMG_ALPHA;
		ImageList_DrawIndirect(&params);
	}

}
