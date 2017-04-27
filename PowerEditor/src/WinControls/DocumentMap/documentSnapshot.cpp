// This file is part of Notepad++ project
// Copyright (C)2003-2017 Don HO <don.h@free.fr>
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


#include "documentSnapshot.h"
#include "ScintillaEditView.h"

INT_PTR CALLBACK DocumentSnapshot::run_dlgProc(UINT message, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	switch (message)
	{
        case WM_INITDIALOG :
		{
			HWND hwndScintilla = reinterpret_cast<HWND>(::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, reinterpret_cast<LPARAM>(_hSelf)));
			_pSnapshotView = reinterpret_cast<ScintillaEditView *>(::SendMessage(_hParent, NPPM_INTERNAL_GETSCINTEDTVIEW, 0, reinterpret_cast<LPARAM>(hwndScintilla)));
			_pSnapshotView->execute(SCI_SETZOOM, static_cast<WPARAM>(-10), 0);
			_pSnapshotView->execute(SCI_SETVSCROLLBAR, FALSE, 0);
			_pSnapshotView->execute(SCI_SETHSCROLLBAR, FALSE, 0);

			_pSnapshotView->showIndentGuideLine(false);

			::MoveWindow(_pSnapshotView->getHSelf(), 0, 0, _rc.right - _rc.left, _rc.bottom - _rc.top, TRUE);
			_pSnapshotView->display();
		}
		break;
	}
	return FALSE;
}

void DocumentSnapshot::doDialog(POINT p, Buffer *pBuf, ScintillaEditView & scintSource)
{
	if (!isCreated())
	{
		create(IDD_DOCUMENTSNAPSHOT);
	}

	syncDisplay(pBuf, scintSource);
    // Adjust the position of DocumentSnapshot
	goTo(p);
}

void DocumentSnapshot::goTo(POINT p)
{
	::SetWindowPos(_hSelf, HWND_TOP, p.x, p.y + 10, _rc.right - _rc.left, _rc.bottom - _rc.top, SWP_SHOWWINDOW);
}

void DocumentSnapshot::syncDisplay(Buffer *buf, ScintillaEditView & scintSource)
{
	if (_pSnapshotView)
	{
		_pSnapshotView->execute(SCI_SETDOCPOINTER, 0, static_cast<LPARAM>(buf->getDocument()));
		_pSnapshotView->setCurrentBuffer(buf);

		//
		// folding
		//
		const std::vector<size_t> & lineStateVector = buf->getHeaderLineState(&scintSource);
		_pSnapshotView->syncFoldStateWith(lineStateVector);

		//
		// Wraping & scrolling
		//
		MapPosition mp = buf->getMapPosition();
		if (mp.isValid())
			scrollSnapshotWith(mp);

		Buffer *buf = _pSnapshotView->getCurrentBuffer();
		_pSnapshotView->defineDocType(buf->getLangType());
		_pSnapshotView->showMargin(ScintillaEditView::_SC_MARGE_FOLDER, false);

		_pSnapshotView->showMargin(0, false);
		_pSnapshotView->showMargin(1, false);
		_pSnapshotView->showMargin(2, false);
		_pSnapshotView->showMargin(3, false);

		_pSnapshotView->execute(SCI_SETREADONLY, true);
		_pSnapshotView->execute(SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE);
		Window::display();
	}
}


void DocumentSnapshot::scrollSnapshotWith(const MapPosition & mapPos)
{
	if (_pSnapshotView)
	{
		bool hasBeenChanged = false;
		//
		// if window size has been changed, resize windows
		//
		if (mapPos._height != -1 && _rc.bottom != _rc.top + mapPos._height)
		{
			_rc.bottom = _rc.top + mapPos._height;
			hasBeenChanged = true;
		}
		if (mapPos._width != -1 && _rc.right != _rc.left + mapPos._width)
		{
			_rc.right = _rc.left + mapPos._width;
			hasBeenChanged = true;
		}
		if (hasBeenChanged)
			::MoveWindow(_pSnapshotView->getHSelf(), 0, 0, _rc.right - _rc.left, _rc.bottom - _rc.top, TRUE);
		//
		// Wrapping
		//
		_pSnapshotView->wrap(mapPos._isWrap);
		_pSnapshotView->execute(SCI_SETWRAPINDENTMODE, mapPos._wrapIndentMode);

		//
		// Reset to zero
		//
		_pSnapshotView->execute(SCI_HOMEDISPLAY);

		//
		// Visible line for the code view
		//

		// scroll to the first visible display line
		_pSnapshotView->execute(SCI_LINESCROLL, 0,mapPos._firstVisibleDisplayLine);
		
	}
}

void DocumentSnapshot::saveCurrentSnapshot(ScintillaEditView & editView)
{
	if (_pSnapshotView)
	{
		Buffer *buffer = editView.getCurrentBuffer();
		MapPosition mapPos = buffer->getMapPosition();

		// First visible document line for scrolling to this line
		mapPos._firstVisibleDisplayLine = static_cast<int32_t>(editView.execute(SCI_GETFIRSTVISIBLELINE));
		mapPos._firstVisibleDocLine = static_cast<int32_t>(editView.execute(SCI_DOCLINEFROMVISIBLE, mapPos._firstVisibleDisplayLine));
		mapPos._nbLine = static_cast<int32_t>(editView.execute(SCI_LINESONSCREEN, mapPos._firstVisibleDisplayLine));
		mapPos._lastVisibleDocLine = static_cast<int32_t>(editView.execute(SCI_DOCLINEFROMVISIBLE, mapPos._firstVisibleDisplayLine + mapPos._nbLine));

		auto lineHeight = _pSnapshotView->execute(SCI_TEXTHEIGHT, mapPos._firstVisibleDocLine);
		mapPos._height = static_cast<int32_t>(mapPos._nbLine * lineHeight);

		// Width
		RECT editorRect;
		editView.getClientRect(editorRect);
		int marginWidths = 0;
		for (int m = 0; m < 4; ++m)
		{
			marginWidths += static_cast<int32_t>(editView.execute(SCI_GETMARGINWIDTHN, m));
		}
		double editViewWidth = editorRect.right - editorRect.left - marginWidths;
		double editViewHeight = editorRect.bottom - editorRect.top;
		mapPos._width = static_cast<int32_t>((editViewWidth / editViewHeight) * static_cast<double>(mapPos._height));

		mapPos._wrapIndentMode = static_cast<int32_t>(editView.execute(SCI_GETWRAPINDENTMODE));
		mapPos._isWrap = static_cast<int32_t>(editView.isWrap());
		if (editView.isWrap())
		{
			mapPos._higherPos = static_cast<int32_t>(editView.execute(SCI_POSITIONFROMPOINT, 0, 0));
		}

		// set current map position in buffer
		buffer->setMapPosition(mapPos);
	}
}
