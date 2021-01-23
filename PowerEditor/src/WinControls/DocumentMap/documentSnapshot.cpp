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


#include "documentSnapshot.h"
#include "ScintillaEditView.h"

INT_PTR CALLBACK DocumentPeeker::run_dlgProc(UINT message, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	switch (message)
	{
        case WM_INITDIALOG :
		{
			HWND hwndScintilla = reinterpret_cast<HWND>(::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, reinterpret_cast<LPARAM>(_hSelf)));
			_pPeekerView = reinterpret_cast<ScintillaEditView *>(::SendMessage(_hParent, NPPM_INTERNAL_GETSCINTEDTVIEW, 0, reinterpret_cast<LPARAM>(hwndScintilla)));
			_pPeekerView->execute(SCI_SETZOOM, static_cast<WPARAM>(-10), 0);
			_pPeekerView->execute(SCI_SETVSCROLLBAR, FALSE, 0);
			_pPeekerView->execute(SCI_SETHSCROLLBAR, FALSE, 0);

			_pPeekerView->showIndentGuideLine(false);

			::MoveWindow(_pPeekerView->getHSelf(), 0, 0, _rc.right - _rc.left, _rc.bottom - _rc.top, TRUE);
			_pPeekerView->display();
		}
		break;
	}
	return FALSE;
}

void DocumentPeeker::doDialog(POINT p, Buffer *pBuf, ScintillaEditView & scintSource)
{
	if (!isCreated())
	{
		create(IDD_DOCUMENTSNAPSHOT);
	}

	syncDisplay(pBuf, scintSource);
    // Adjust the position of DocumentPeeker
	goTo(p);
}

void DocumentPeeker::goTo(POINT p)
{
	::SetWindowPos(_hSelf, HWND_TOP, p.x, p.y + 10, _rc.right - _rc.left, _rc.bottom - _rc.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}


void DocumentPeeker::syncDisplay(Buffer *buf, ScintillaEditView & scintSource)
{
	if (_pPeekerView)
	{
		_pPeekerView->execute(SCI_SETDOCPOINTER, 0, static_cast<LPARAM>(buf->getDocument()));
		_pPeekerView->setCurrentBuffer(buf);

		//
		// folding
		//
		const std::vector<size_t> & lineStateVector = buf->getHeaderLineState(&scintSource);
		_pPeekerView->syncFoldStateWith(lineStateVector);

		//
		// Wraping & scrolling
		//
		MapPosition mp = buf->getMapPosition();
		if (mp.isValid() && mp.canScroll())
		{
			scrollSnapshotWith(mp);
		}

		Buffer *buf = _pPeekerView->getCurrentBuffer();
		_pPeekerView->defineDocType(buf->getLangType());
		_pPeekerView->showMargin(ScintillaEditView::_SC_MARGE_FOLDER, false);

		_pPeekerView->showMargin(0, false);
		_pPeekerView->showMargin(1, false);
		_pPeekerView->showMargin(2, false);
		_pPeekerView->showMargin(3, false);

		_pPeekerView->execute(SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE);
	}
}


void DocumentPeeker::scrollSnapshotWith(const MapPosition & mapPos)
{
	if (_pPeekerView)
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
			::MoveWindow(_pPeekerView->getHSelf(), 0, 0, _rc.right - _rc.left, _rc.bottom - _rc.top, TRUE);
		//
		// Wrapping
		//
		_pPeekerView->wrap(mapPos._isWrap);
		_pPeekerView->execute(SCI_SETWRAPINDENTMODE, mapPos._wrapIndentMode);

		//
		// Reset to zero
		//
		_pPeekerView->execute(SCI_HOMEDISPLAY);

		//
		// Visible line for the code view
		//

		// scroll to the first visible display line
		_pPeekerView->execute(SCI_LINESCROLL, 0, mapPos._firstVisibleDisplayLine);
		
	}
}

void DocumentPeeker::saveCurrentSnapshot(ScintillaEditView & editView)
{
	if (_pPeekerView)
	{
		Buffer *buffer = editView.getCurrentBuffer();
		MapPosition mapPos = buffer->getMapPosition();

		// First visible document line for scrolling to this line
		mapPos._firstVisibleDisplayLine = static_cast<int32_t>(editView.execute(SCI_GETFIRSTVISIBLELINE));
		mapPos._firstVisibleDocLine = static_cast<int32_t>(editView.execute(SCI_DOCLINEFROMVISIBLE, mapPos._firstVisibleDisplayLine));
		mapPos._nbLine = static_cast<int32_t>(editView.execute(SCI_LINESONSCREEN, mapPos._firstVisibleDisplayLine));
		mapPos._lastVisibleDocLine = static_cast<int32_t>(editView.execute(SCI_DOCLINEFROMVISIBLE, mapPos._firstVisibleDisplayLine + mapPos._nbLine));

		auto lineHeight = _pPeekerView->execute(SCI_TEXTHEIGHT, mapPos._firstVisibleDocLine);
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

		// Length of document
		mapPos._KByteInDoc = editView.getCurrentDocLen() / 1024;

		// set current map position in buffer
		buffer->setMapPosition(mapPos);
	}
}
