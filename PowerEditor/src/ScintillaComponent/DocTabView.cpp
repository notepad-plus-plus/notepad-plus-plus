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

#include <windows.h>

#include <cwchar>

#include "Buffer.h"
#include "NppConstants.h"
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "TabBar.h"
#include "resource.h"

enum ImgIdx
{
	SAVED_IMG_INDEX,
	UNSAVED_IMG_INDEX,
	REDONLY_IMG_INDEX,
	REDONLYSYS_IMG_INDEX,
	MONITORING_IMG_INDEX,
};

static constexpr int docTabIconIDs[] = { IDI_SAVED_ICON, IDI_UNSAVED_ICON, IDI_READONLY_ICON, IDI_READONLYSYS_ICON, IDI_MONITORING_ICON };
static constexpr int docTabIconIDs_darkMode[] = { IDI_SAVED_DM_ICON, IDI_UNSAVED_DM_ICON, IDI_READONLY_DM_ICON, IDI_READONLYSYS_DM_ICON, IDI_MONITORING_DM_ICON };
static constexpr int docTabIconIDs_alt[] = { IDI_SAVED_ALT_ICON, IDI_UNSAVED_ALT_ICON, IDI_READONLY_ALT_ICON, IDI_READONLYSYS_ALT_ICON, IDI_MONITORING_ICON };

void DocTabView::init(HINSTANCE hInst, HWND parent, ScintillaEditView* pView, unsigned char indexChoice, unsigned char buttonsStatus)
{
	TabBarPlus::init(hInst, parent, false, false, buttonsStatus);
	_pView = pView;

	createIconSets();

	_pIconListVector.push_back(&_docTabIconList);         // 0
	_pIconListVector.push_back(&_docTabIconListAlt);      // 1
	_pIconListVector.push_back(&_docTabIconListDarkMode); // 2

	if (indexChoice >= _pIconListVector.size())
		_iconListIndexChoice = 0;
	else
		_iconListIndexChoice = indexChoice;

	if (_iconListIndexChoice != -1)
		TabBar::setImageList(_pIconListVector[_iconListIndexChoice]->getHandle());
	return;
}

void DocTabView::createIconSets()
{
	int iconDpiDynamicalSize = _dpiManager.scale(g_TabIconSize);

	_docTabIconList.create(iconDpiDynamicalSize, _hInst, docTabIconIDs, sizeof(docTabIconIDs) / sizeof(int));
	_docTabIconListAlt.create(iconDpiDynamicalSize, _hInst, docTabIconIDs_alt, sizeof(docTabIconIDs_alt) / sizeof(int));
	_docTabIconListDarkMode.create(iconDpiDynamicalSize, _hInst, docTabIconIDs_darkMode, sizeof(docTabIconIDs_darkMode) / sizeof(int));
}

void DocTabView::addBuffer(BufferID buffer)
{
	if (buffer == BUFFER_INVALID)	//valid only
		return;
	if (getIndexByBuffer(buffer) != -1)	//no duplicates
		return;
	const Buffer* buf = MainFileManager.getBufferByID(buffer);
	TCITEM tie{};
	tie.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;

	int index = -1;
	if (_hasImgLst)
	{
		// Match the icon that bufferUpdated would set, so that inside a
		// batch (where we skip the bufferUpdated call) lazy/dirty state is
		// visible immediately. Without this, session-restored dirty
		// untitled tabs looked "saved" until the user clicked them.
		index = 0;
		if (buf->isDirty())
			index = UNSAVED_IMG_INDEX;
		if (buf->isMonitoringOn())
			index = MONITORING_IMG_INDEX;
		else if (buf->getFileReadOnly())
			index = REDONLYSYS_IMG_INDEX;
		else if (buf->getUserReadOnly())
			index = REDONLY_IMG_INDEX;
	}
	tie.iImage = index;
	tie.pszText = const_cast<wchar_t*>(buf->getCompactFileName());
	tie.lParam = reinterpret_cast<LPARAM>(buffer);
	::SendMessage(_hSelf, TCM_INSERTITEM, _nbItem++, reinterpret_cast<LPARAM>(&tie));

	if (_batchInsertDepth == 0)
	{
		// Normal path: re-run bufferUpdated to apply status-specific icon +
		// ampersand-encoded label. Inside a batch we skip this — addBuffer has
		// already set a reasonable label and image via TCITEM above, and
		// bufferUpdated is O(N) (linear getIndexByBuffer scan), so calling it
		// once per tab during session restore is quadratic over 300+ tabs.
		bufferUpdated(buf, BufferChangeMask);
		::SendMessage(_hParent, WM_SIZE, 0, 0);
	}
}

void DocTabView::addBufferAt(size_t index, BufferID buffer)
{
	if (buffer == BUFFER_INVALID)
		return;
	if (getIndexByBuffer(buffer) != -1)
		return;
	if (index > _nbItem)
		index = _nbItem; // safety — TCM_INSERTITEM would ignore out-of-range

	const Buffer* buf = MainFileManager.getBufferByID(buffer);
	TCITEM tie{};
	tie.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;

	int iImage = -1;
	if (_hasImgLst)
	{
		iImage = 0;
		if (buf->isDirty())
			iImage = UNSAVED_IMG_INDEX;
		if (buf->isMonitoringOn())
			iImage = MONITORING_IMG_INDEX;
		else if (buf->getFileReadOnly())
			iImage = REDONLYSYS_IMG_INDEX;
		else if (buf->getUserReadOnly())
			iImage = REDONLY_IMG_INDEX;
	}
	tie.iImage = iImage;
	tie.pszText = const_cast<wchar_t*>(buf->getCompactFileName());
	tie.lParam = reinterpret_cast<LPARAM>(buffer);

	::SendMessage(_hSelf, TCM_INSERTITEM, index, reinterpret_cast<LPARAM>(&tie));
	++_nbItem;

	if (_batchInsertDepth == 0)
	{
		bufferUpdated(buf, BufferChangeMask);
		::SendMessage(_hParent, WM_SIZE, 0, 0);
	}
}

void DocTabView::beginBatchInsert()
{
	if (_batchInsertDepth++ == 0 && _hSelf)
	{
		// Freeze tab-control drawing while we mass-insert. Each TCM_INSERTITEM
		// with custom-drawn tabs would otherwise trigger a partial repaint.
		::SendMessage(_hSelf, WM_SETREDRAW, FALSE, 0);
	}
}

void DocTabView::endBatchInsert()
{
	if (_batchInsertDepth == 0)
		return;
	if (--_batchInsertDepth == 0 && _hSelf)
	{
		::SendMessage(_hSelf, WM_SETREDRAW, TRUE, 0);
		// Force one clean repaint of the full tab bar; without the Update the
		// TRUE-redraw alone may leave stale pixels until the next event.
		::RedrawWindow(_hSelf, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW | RDW_ALLCHILDREN);
		::SendMessage(_hParent, WM_SIZE, 0, 0);
	}
}

void DocTabView::closeBuffer(BufferID buffer)
{
	int indexToClose = getIndexByBuffer(buffer);
	deletItemAt(static_cast<size_t>(indexToClose));
	::SendMessage(_hParent, WM_SIZE, 0, 0);
}

void DocTabView::setIndividualTabColour(BufferID bufferId, int colorId)
{
	bufferId->setDocColorId(colorId);
}

int DocTabView::getIndividualTabColourId(int tabIndex)
{
	BufferID bufferId = getBufferByIndex(tabIndex);
	if (!bufferId)
		return -1;

	return bufferId->getDocColorId();
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
	return getBufferByIndex(index);
}


BufferID DocTabView::findBufferByName(const wchar_t * fullfilename) //-1 if not found, something else otherwise
{
	TCITEM tie{};
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	for (size_t i = 0; i < _nbItem; ++i)
	{
		::SendMessage(_hSelf, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tie));
		BufferID id = reinterpret_cast<BufferID>(tie.lParam);
		const Buffer* buf = MainFileManager.getBufferByID(id);
		if (_wcsicmp(fullfilename, buf->getFullPathName()) == 0)
		{
			return id;
		}
	}
	return BUFFER_INVALID;
}


int DocTabView::getIndexByBuffer(BufferID id)
{
	TCITEM tie{};
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
	TCITEM tie{};
	tie.lParam = -1;
	tie.mask = TCIF_PARAM;
	::SendMessage(_hSelf, TCM_GETITEM, index, reinterpret_cast<LPARAM>(&tie));

	return reinterpret_cast<BufferID>(tie.lParam);
}


void DocTabView::bufferUpdated(const Buffer* buffer, int mask)
{
	int index = getIndexByBuffer(buffer->getID());
	if (index == -1)
		return;

	TCITEM tie{};
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
		else if (buffer->getFileReadOnly())
		{
			tie.iImage = REDONLYSYS_IMG_INDEX;
		}
		else if (buffer->getUserReadOnly())
		{
			tie.iImage = REDONLY_IMG_INDEX;
		}
	}

	//We must make space for the added ampersand characters.
	wchar_t encodedLabel[2 * MAX_PATH] = { '\0' };

	if (mask & BufferChangeFilename)
	{
		tie.mask |= TCIF_TEXT;
		tie.pszText = encodedLabel;

		{
			const wchar_t* in = buffer->getCompactFileName();
			wchar_t* out = encodedLabel;

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
	if (index >= _nbItem)
		return;

	TCITEM tie{};
	tie.lParam = reinterpret_cast<LPARAM>(id);
	tie.mask = TCIF_PARAM;
	::SendMessage(_hSelf, TCM_SETITEM, index, reinterpret_cast<LPARAM>(&tie));

	bufferUpdated(MainFileManager.getBufferByID(id), BufferChangeMask);	//update tab, everything has changed

	::SendMessage(_hParent, WM_SIZE, 0, 0);
}


void DocTabView::reSizeTo(RECT & rc)
{
	NppParameters& nppParam = NppParameters::getInstance();
	int borderWidth = nppParam.getSVP()._borderWidth;
	const NppGUI& nppGUI = nppParam.getNppGUI();
	if (nppGUI._tabStatus & TAB_HIDE)
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
