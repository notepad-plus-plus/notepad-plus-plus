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


#include "documentMap.h"
#include "ScintillaEditView.h"


void DocumentMap::reloadMap()
{
	if (_pScintillaEditView && _ppEditView)
	{
		Document currentDoc = (*_ppEditView)->execute(SCI_GETDOCPOINTER);
		_pScintillaEditView->execute(SCI_SETDOCPOINTER, 0, static_cast<LPARAM>(currentDoc));

		//
		// sync with the current document
		//

		Buffer *editBuf = (*_ppEditView)->getCurrentBuffer();
		_pScintillaEditView->setCurrentBuffer(editBuf);

		// folding
		std::vector<size_t> lineStateVector;
		(*_ppEditView)->getCurrentFoldStates(lineStateVector);
		_pScintillaEditView->syncFoldStateWith(lineStateVector);

		// Wrapping
		if ((*_ppEditView)->isWrap() && needToRecomputeWith())
		{
			wrapMap();
		}

		scrollMap();
	}
}

void DocumentMap::showInMapTemporarily(Buffer *buf2show, const ScintillaEditView *fromEditView)
{
	if (_pScintillaEditView && fromEditView)
	{
		_pScintillaEditView->execute(SCI_SETDOCPOINTER, 0, static_cast<LPARAM>(buf2show->getDocument()));
		_pScintillaEditView->setCurrentBuffer(buf2show);

		// folding
		const std::vector<size_t> & lineStateVector = buf2show->getHeaderLineState(fromEditView);
		_pScintillaEditView->syncFoldStateWith(lineStateVector);

		// Wrapping
		if (fromEditView->isWrap() && needToRecomputeWith(fromEditView))
		{
			wrapMap(fromEditView);
		}
		//_pScintillaEditView->restoreCurrentPos();
		scrollMap(fromEditView);

		/*
		Buffer * buf = buf2show;
		Position & pos = buf->getPosition(const_cast<ScintillaEditView *>(fromEditView));

		_pScintillaEditView->execute(SCI_GOTOPOS, 0);	//make sure first line visible by setting caret there, will scroll to top of document

		_pScintillaEditView->execute(SCI_SETSELECTIONMODE, pos._selMode);	//enable
		_pScintillaEditView->execute(SCI_SETANCHOR, pos._startPos);
		_pScintillaEditView->execute(SCI_SETCURRENTPOS, pos._endPos);
		_pScintillaEditView->execute(SCI_CANCEL);							//disable
		if (not _pScintillaEditView->isWrap()) //only offset if not wrapping, otherwise the offset isnt needed at all
		{
			_pScintillaEditView->execute(SCI_SETSCROLLWIDTH, pos._scrollWidth);
			_pScintillaEditView->execute(SCI_SETXOFFSET, pos._xOffset);
		}
		_pScintillaEditView->execute(SCI_CHOOSECARETX); // choose current x position

		int lineToShow = static_cast<int32_t>(_pScintillaEditView->execute(SCI_VISIBLEFROMDOCLINE, pos._firstVisibleLine));
		_pScintillaEditView->scroll(0, lineToShow);
		*/
	}
}

void DocumentMap::setSyntaxHiliting()
{
	Buffer *buf = _pScintillaEditView->getCurrentBuffer();
	_pScintillaEditView->defineDocType(buf->getLangType());
	_pScintillaEditView->showMargin(ScintillaEditView::_SC_MARGE_FOLDER, false);
}

bool DocumentMap::needToRecomputeWith(const ScintillaEditView *editView)
{
	const ScintillaEditView *pEditView = editView ? editView : *_ppEditView;

	auto currentZoom = pEditView->execute(SCI_GETZOOM);
	if (_displayZoom != currentZoom)
		return true;

	int currentTextZoneWidth = getEditorTextZoneWidth(editView);
	if (_displayWidth != currentTextZoneWidth)
		return true;

	return false;
}

void DocumentMap::initWrapMap()
{
	if (_pScintillaEditView && _ppEditView)
	{
		RECT rect;
		getClientRect(rect);
		::MoveWindow(_pScintillaEditView->getHSelf(), 0, 0, rect.right - rect.left, rect.bottom-rect.top, TRUE);
		_pScintillaEditView->wrap(false);
		_pScintillaEditView->redraw(true);

		bool isRTL = (*_ppEditView)->isTextDirectionRTL();
		if (_pScintillaEditView->isTextDirectionRTL() != isRTL)
			_pScintillaEditView->changeTextDirection(isRTL);
	}
}

void DocumentMap::changeTextDirection(bool isRTL)
{
	_pScintillaEditView->changeTextDirection(isRTL);
}

/*
double ddd = (double)Xlength1/(double)Xlength2;
char dchar[256];
sprintf(dchar, "%f", ddd);
::MessageBoxA(NULL, dchar, "", MB_OK);
		
		// -10    => 1
		// -9     => 1
		// -8     => 1
		// -7     => 1
		// -6     => 1.5
		// -5     => 2
		// -4     => 2.5
		// -3     => 2.5
		// -2     => 3.5
		// -1     => 3.5
		// 0: -10 => 4
		// 1      => 4.5
		// 2      => 5
		// 3      => 5
		// 4      => 5.5
		// 5      => 6
		// 6      => 6.5
		// 7      => 7
		// 8      => 7
		// 9      => 7.5
		// 10     => 8
		// 11     => 8.5
		// 12     => 8.5
		// 13     => 9.5
		// 14     => 9.5
		// 15     => 10
		// 16     => 10.5
		// 17     => 11
		// 18     => 11
		// 19     => 11.5
		// 20     => 12
*/
double zoomRatio[] = {1, 1, 1, 1, 1.5, 2, 2.5, 2.5, 3.5, 3.5,\
4, 4.5, 5, 5, 5.5, 6, 6.5, 7, 7, 7.5, 8, 8.5, 8.5, 9.5, 9.5, 10, 10.5, 11, 11, 11.5, 12};

void DocumentMap::wrapMap(const ScintillaEditView *editView)
{
	const ScintillaEditView *pEditView = editView ? editView : *_ppEditView;
	RECT rect;
	getClientRect(rect);
	if (pEditView->isWrap())
	{
		// get current scintilla width W1
		int editZoneWidth = getEditorTextZoneWidth(editView);

		// update the wrap needed data
		_displayWidth = editZoneWidth;
		_displayZoom = static_cast<int32_t>(pEditView->execute(SCI_GETZOOM));
		double zr = zoomRatio[_displayZoom + 10];

		// compute doc map width: dzw/ezw = 1/zoomRatio
		double docMapWidth = editZoneWidth / zr;

		::MoveWindow(_pScintillaEditView->getHSelf(), 0, 0, int(docMapWidth), rect.bottom-rect.top, TRUE);
		_pScintillaEditView->wrap(true);

		// sync wrapping indent mode
		_pScintillaEditView->execute(SCI_SETWRAPINDENTMODE, pEditView->execute(SCI_GETWRAPINDENTMODE));

	}
}

int DocumentMap::getEditorTextZoneWidth(const ScintillaEditView *editView)
{
	const ScintillaEditView *pEditView = editView ? editView : *_ppEditView;

	RECT editorRect;
	pEditView->getClientRect(editorRect);

	int marginWidths = 0;
	for (int m = 0; m < 4; ++m)
	{
		marginWidths += static_cast<int32_t>(pEditView->execute(SCI_GETMARGINWIDTHN, m));
	}
	return editorRect.right - editorRect.left - marginWidths;
}
/*
struct mapPosition {
	int32_t _firstVisibleDocLine;
	int32_t _nbLine;
	int32_t _lastVisibleDocLine;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
};
*/
void DocumentMap::scrollMap(const ScintillaEditView *editView)
{
	const ScintillaEditView *pEditView = editView ? editView : *_ppEditView;

	if (_pScintillaEditView && pEditView)
	{
		// Visible document line for the code view (but not displayed line)
		auto firstVisibleDisplayLine = pEditView->execute(SCI_GETFIRSTVISIBLELINE);
		auto firstVisibleDocLine = pEditView->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine);
		auto nbLine = pEditView->execute(SCI_LINESONSCREEN, firstVisibleDisplayLine);
		auto lastVisibleDocLine = pEditView->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine + nbLine);

		// Visible document line for the map view
		auto firstVisibleDisplayLineMap = _pScintillaEditView->execute(SCI_GETFIRSTVISIBLELINE);
		auto firstVisibleDocLineMap = _pScintillaEditView->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLineMap);
		auto nbLineMap = _pScintillaEditView->execute(SCI_LINESONSCREEN, firstVisibleDocLineMap);
		auto lastVisibleDocLineMap = pEditView->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLineMap + nbLineMap);

		// If part of editor view is out of map, then scroll map
		if (lastVisibleDocLineMap < lastVisibleDocLine)
			_pScintillaEditView->execute(SCI_GOTOLINE, lastVisibleDocLine);
		else
			_pScintillaEditView->execute(SCI_GOTOLINE, firstVisibleDocLine);

		// Get the editor's higher/lower Y, then compute the map's higher/lower Y
		LRESULT higherY = 0;
		LRESULT lowerY = 0;
		if (not pEditView->isWrap())
		{
			auto higherPos = _pScintillaEditView->execute(SCI_POSITIONFROMLINE, firstVisibleDocLine);
			auto lowerPos = _pScintillaEditView->execute(SCI_POSITIONFROMLINE, lastVisibleDocLine);
			higherY = _pScintillaEditView->execute(SCI_POINTYFROMPOSITION, 0, higherPos);
			lowerY = _pScintillaEditView->execute(SCI_POINTYFROMPOSITION, 0, lowerPos);
			if (lowerY == 0)
			{
				auto lineHeight = _pScintillaEditView->execute(SCI_TEXTHEIGHT, firstVisibleDocLine);
				lowerY = nbLine * lineHeight + firstVisibleDocLine;
			}
		}
		else
		{
			auto higherPos = pEditView->execute(SCI_POSITIONFROMPOINT, 0, 0);
			higherY = _pScintillaEditView->execute(SCI_POINTYFROMPOSITION, 0, higherPos);
			auto lineHeight = _pScintillaEditView->execute(SCI_TEXTHEIGHT, firstVisibleDocLine);
			lowerY = nbLine * lineHeight + higherY;
		}

		// Update view zone in map
		_vzDlg.drawZone(static_cast<long>(higherY), static_cast<long>(lowerY));
	}
}

void DocumentMap::doMove()
{
	RECT rc;
	POINT pt = {0,0};
	::ClientToScreen(_hSelf, &pt);
	getClientRect(rc);
	::MoveWindow(_vzDlg.getHSelf(), pt.x, pt.y, (rc.right - rc.left), (rc.bottom - rc.top), TRUE);
}

void DocumentMap::fold(int line, bool foldOrNot)
{
	_pScintillaEditView->fold(line, foldOrNot);
}

void DocumentMap::foldAll(bool mode)
{
	_pScintillaEditView->foldAll(mode);
}

void DocumentMap::scrollMap(bool direction, moveMode whichMode)
{
	// Visible line for the code view
	auto firstVisibleDisplayLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
	auto nbLine = (*_ppEditView)->execute(SCI_LINESONSCREEN, firstVisibleDisplayLine);
	auto nbLine2go = (whichMode == perLine ? 1 : nbLine);
	(*_ppEditView)->execute(SCI_LINESCROLL, 0, (direction == moveDown) ? nbLine2go : -nbLine2go);

	scrollMap();
}

void DocumentMap::redraw(bool) const
{
	_pScintillaEditView->execute(SCI_COLOURISE, 0, -1);
}

INT_PTR CALLBACK DocumentMap::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			HWND hwndScintilla = reinterpret_cast<HWND>(::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, reinterpret_cast<LPARAM>(_hSelf)));
			_pScintillaEditView = reinterpret_cast<ScintillaEditView *>(::SendMessage(_hParent, NPPM_INTERNAL_GETSCINTEDTVIEW, 0, reinterpret_cast<LPARAM>(hwndScintilla)));
			_pScintillaEditView->execute(SCI_SETZOOM, static_cast<WPARAM>(-10), 0);
			_pScintillaEditView->execute(SCI_SETVSCROLLBAR, FALSE, 0);
			_pScintillaEditView->execute(SCI_SETHSCROLLBAR, FALSE, 0);

			_pScintillaEditView->showIndentGuideLine(false);
			_pScintillaEditView->display();
			
			reloadMap();

			_vzDlg.init(::GetModuleHandle(NULL), _hSelf);
			_vzDlg.doDialog();
			(NppParameters::getInstance())->SetTransparent(_vzDlg.getHSelf(), 50); // 0 <= transparancy < 256

			setSyntaxHiliting();
			
			_pScintillaEditView->showMargin(0, false);
			_pScintillaEditView->showMargin(1, false);
			_pScintillaEditView->showMargin(2, false);
			_pScintillaEditView->showMargin(3, false);
			
            return TRUE;
        }

        case WM_SIZE:
        {
			if (_pScintillaEditView)
			{
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);

				if (_vzDlg.isCreated())
				{
					POINT pt = {0,0};
					::ClientToScreen(_hSelf, &pt);
					if (!_pScintillaEditView->isWrap())
						::MoveWindow(_pScintillaEditView->getHSelf(), 0, 0, width, height, TRUE);
						
					::MoveWindow(_vzDlg.getHSelf(), pt.x, pt.y, width, height, TRUE);
				}
			}
            break;
        }

		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case DMN_CLOSE:
				{
					::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_DOC_MAP, 0);
					return TRUE;
				}

				case DMN_SWITCHIN:
				{
					_vzDlg.display();
					reloadMap();
					setSyntaxHiliting();
					return TRUE;
				}

				case DMN_SWITCHOFF:
				{
					_vzDlg.display(false);
					return TRUE;
				}

				case DMN_FLOATDROPPED:
				{
					RECT rc;
					getClientRect(rc);
					int width = rc.right - rc.left;
					int height = rc.bottom - rc.top;

					//RECT scinrc;
					//_pScintillaEditView->getClientRect(scinrc);
					//int scinrcWidth = scinrc.right - scinrc.left;
					//::MoveWindow(_pScintillaEditView->getHSelf(), 0, 0, scinrcWidth, height, TRUE);

					POINT pt = {0,0};
					::ClientToScreen(_hSelf, &pt);
					::MoveWindow(_vzDlg.getHSelf(), pt.x, pt.y, width, height, TRUE);
					scrollMap();
					return TRUE;
				}

				case NM_DBLCLK:
				{
					return TRUE;
				}

				default:
					break;
				
			}
	
		}
		return TRUE;

		case DOCUMENTMAP_SCROLL:
		{
			bool dir = (wParam != 0);
			moveMode mode = (lParam == 0)?perLine:perPage;
			scrollMap(dir, mode);
		}
		return TRUE;

		case DOCUMENTMAP_MOUSECLICKED:
		{
			int newPosY = HIWORD(lParam);
			int currentCenterPosY = _vzDlg.getCurrentCenterPosY();
			int pixelPerLine = static_cast<int32_t>(_pScintillaEditView->execute(SCI_TEXTHEIGHT, 0));
			int jumpDistance = newPosY - currentCenterPosY;
			int nbLine2jump = jumpDistance/pixelPerLine;
			(*_ppEditView)->execute(SCI_LINESCROLL, 0, nbLine2jump);

			scrollMap();
		}
		return TRUE;

		case DOCUMENTMAP_MOUSEWHEEL:
		{
			(*_ppEditView)->mouseWheel(wParam, lParam);
		}
		return TRUE;



        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void ViewZoneDlg::drawPreviewZone(DRAWITEMSTRUCT *pdis)
{
	RECT rc = pdis->rcItem;
	
	const COLORREF orange = RGB(0xFF, 0x80, 0x00);
	const COLORREF white = RGB(0xFF, 0xFF, 0xFF);
	HBRUSH hbrushFg = CreateSolidBrush(orange);
	HBRUSH hbrushBg = CreateSolidBrush(white);					
	FillRect(pdis->hDC, &rc, hbrushBg);

	rc.top = _higherY;
	rc.bottom = _lowerY;
	FillRect(pdis->hDC, &rc, hbrushFg);
	
	DeleteObject(hbrushFg);
	DeleteObject(hbrushBg);
}

void ViewZoneDlg::doDialog()
{
	if (!isCreated())
		create(IDD_VIEWZONE);
	display();
};

INT_PTR CALLBACK ViewZoneDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
        case WM_INITDIALOG :
		{
			_viewZoneCanvas = ::GetDlgItem(_hSelf, IDC_VIEWZONECANVAS);
			if (NULL != _viewZoneCanvas)
			{
				::SetWindowLongPtr(_viewZoneCanvas, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
				_canvasDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_viewZoneCanvas, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(canvasStaticProc)));
				return TRUE;
			}
			break;
		}

		case WM_LBUTTONDOWN:
		{
			::SendMessage(_hParent, DOCUMENTMAP_MOUSECLICKED, wParam, lParam);
			break;
		}

		case WM_MOUSEMOVE:
		{
			if (wParam & MK_LBUTTON)
				::SendMessage(_hParent, DOCUMENTMAP_MOUSECLICKED, wParam, lParam);
			break;
		}

		case WM_DRAWITEM :
		{
			drawPreviewZone((DRAWITEMSTRUCT *)lParam);
			return TRUE;
		}

		case WM_SIZE:
        {
			if (NULL != _viewZoneCanvas)
			{
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);
				::MoveWindow(_viewZoneCanvas, 0, 0, width , height, TRUE);
			}
            break;
        }

		case WM_MOUSEWHEEL :
		{
			//Have to perform the scroll first, because the first/last line do not get updated untill after the scroll has been parsed
			::SendMessage(_hParent, DOCUMENTMAP_MOUSEWHEEL, wParam, lParam);
			return TRUE;
		}

		case WM_DESTROY :
		{
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CALLBACK ViewZoneDlg::canvasStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	ViewZoneDlg *pViewZoneDlg = reinterpret_cast<ViewZoneDlg *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (!pViewZoneDlg)
		return FALSE;
	return pViewZoneDlg->canvas_runProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK ViewZoneDlg::canvas_runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
    {
		case WM_DESTROY:
		{
			//::MessageBoxA(NULL,"Destroy","",MB_OK);
		}
		return TRUE;

		case WM_KEYDOWN:
			if (wParam == VK_UP)
			{
				::SendMessage(_hParent, DOCUMENTMAP_SCROLL, static_cast<WPARAM>(moveUp), 0);
			}
			if (wParam == VK_DOWN)
			{
				::SendMessage(_hParent, DOCUMENTMAP_SCROLL, static_cast<WPARAM>(moveDown), 0);
			}
			if (wParam == VK_PRIOR)
			{
				::SendMessage(_hParent, DOCUMENTMAP_SCROLL, static_cast<WPARAM>(moveUp), 1);
			}
			if (wParam == VK_NEXT)
			{
				::SendMessage(_hParent, DOCUMENTMAP_SCROLL, static_cast<WPARAM>(moveDown), 1);
			}
			break;

        case WM_SIZE:
        {
            break;
        }

		case WM_NOTIFY:
		{
		}
		return TRUE;

        default :
            return _canvasDefaultProc(hwnd, message, wParam, lParam);
    }
	return _canvasDefaultProc(hwnd, message, wParam, lParam);
}