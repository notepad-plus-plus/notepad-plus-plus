//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "DocTabView.h"

#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

#include <commctrl.h>
#include <shlwapi.h>

bool DocTabView::_hideTabBarStatus = false;

void DocTabView::addBuffer(BufferID buffer) {
	if (buffer == BUFFER_INVALID)	//valid only
		return;
	if (this->getIndexByBuffer(buffer) != -1)	//no duplicates
		return;
	Buffer * buf = MainFileManager->getBufferByID(buffer);
	TCITEM tie; 
	tie.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;

	int index = -1;
	if (_hasImgLst)
		index = 0;
	tie.iImage = index; 
	tie.pszText = (TCHAR *)buf->getFileName();
	tie.lParam = (LPARAM)buffer;
	::SendMessage(_hSelf, TCM_INSERTITEM, _nbItem++, reinterpret_cast<LPARAM>(&tie));
	bufferUpdated(buf, BufferChangeMask);

	::SendMessage(_hParent, WM_SIZE, 0, 0);
}

void DocTabView::closeBuffer(BufferID buffer) {
	int indexToClose = getIndexByBuffer(buffer);
	deletItemAt(indexToClose);

	::SendMessage(_hParent, WM_SIZE, 0, 0);
}

bool DocTabView::activateBuffer(BufferID buffer) {
	int indexToActivate = getIndexByBuffer(buffer);
	if (indexToActivate == -1)
		return false;	//cannot activate
	activateAt(indexToActivate);
	return true;
}

BufferID DocTabView::activeBuffer() {
	int index = getCurrentTabIndex();
	return (BufferID)getBufferByIndex(index);
}

BufferID DocTabView::findBufferByName(const TCHAR * fullfilename) {	//-1 if not found, something else otherwise
	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	for(size_t i = 0; i < _nbItem; i++) {
		::SendMessage(_hSelf, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tie));
		BufferID id = (BufferID)tie.lParam;
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (!lstrcmp(fullfilename, buf->getFullPathName())) {
			return id;
		}
	}
	return BUFFER_INVALID;
}

int DocTabView::getIndexByBuffer(BufferID id) {
	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	for(int i = 0; i < (int)_nbItem; i++) {
		::SendMessage(_hSelf, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tie));
		if ((BufferID)tie.lParam == id)
			return i;
	}
	return -1;
}

BufferID DocTabView::getBufferByIndex(int index) {
	TCITEM tie;
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	::SendMessage(_hSelf, TCM_GETITEM, index, reinterpret_cast<LPARAM>(&tie));

	return (BufferID)tie.lParam;
}

void DocTabView::bufferUpdated(Buffer * buffer, int mask) {
	int index = getIndexByBuffer(buffer->getID());
	if (index == -1)
		return;

	TCITEM tie;
	tie.lParam = -1;
	tie.mask = 0;
	

	if (mask & BufferChangeReadonly || mask & BufferChangeDirty) {
		tie.mask |= TCIF_IMAGE;
		tie.iImage = buffer->isDirty()?UNSAVED_IMG_INDEX:SAVED_IMG_INDEX;
		if (buffer->isReadOnly()) {
			tie.iImage = REDONLY_IMG_INDEX;
		}
	}

	if (mask & BufferChangeFilename) {
		tie.mask |= TCIF_TEXT;
		tie.pszText = (TCHAR *)buffer->getFileName();
	}

	::SendMessage(_hSelf, TCM_SETITEM, index, reinterpret_cast<LPARAM>(&tie));

	::SendMessage(_hParent, WM_SIZE, 0, 0);
}

void DocTabView::setBuffer(int index, BufferID id) {
	if (index < 0 || index >= (int)_nbItem)
		return;

	TCITEM tie;
	tie.lParam = (LPARAM)id;
	tie.mask = TCIF_PARAM;
	::SendMessage(_hSelf, TCM_SETITEM, index, reinterpret_cast<LPARAM>(&tie));

	bufferUpdated(MainFileManager->getBufferByID(id), BufferChangeMask);	//update tab, everything has changed

	::SendMessage(_hParent, WM_SIZE, 0, 0);
}
