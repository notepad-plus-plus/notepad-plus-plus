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
#include "documentMap.h"
#include "ScintillaEditView.h"


void DocumentMap::reloadMap()
{
	if (_pScintillaEditView && _ppEditView)
	{
		Document currentDoc = (*_ppEditView)->execute(SCI_GETDOCPOINTER);
		::SendMessage(_pScintillaEditView->getHSelf(), SCI_SETDOCPOINTER, 0, (LPARAM)currentDoc);
		//_pScintillaEditView->wrap((*_ppEditView)->isWrap());

		// sync with the current document
		// Lexing
		_pScintillaEditView->defineDocType((*_ppEditView)->getCurrentBuffer()->getLangType());
		_pScintillaEditView->showMargin(ScintillaEditView::_SC_MARGE_FOLDER, false);

		// folding
		_pScintillaEditView->syncFoldStateWith((*_ppEditView)->getCurrentFoldStates());

		scrollMap();
		
	}
}

void DocumentMap::scrollMap()
{
	if (_pScintillaEditView && _ppEditView)
	{
		// Visible document line for the code view (but not displayed line)
		int firstVisibleDisplayLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
		int firstVisibleDocLine = (*_ppEditView)->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine);
		int nbLine = (*_ppEditView)->execute(SCI_LINESONSCREEN, firstVisibleDisplayLine);
		int lastVisibleDocLine = (*_ppEditView)->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine + nbLine);

		// Visible document line for the map view
		int firstVisibleDisplayLineMap = _pScintillaEditView->execute(SCI_GETFIRSTVISIBLELINE);
		int firstVisibleDocLineMap = _pScintillaEditView->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLineMap);
		int nbLineMap = _pScintillaEditView->execute(SCI_LINESONSCREEN, firstVisibleDocLineMap);
		int lastVisibleDocLineMap = (*_ppEditView)->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLineMap + nbLineMap);

		// If part of editor view is out of map, then scroll map
		if (lastVisibleDocLineMap < lastVisibleDocLine)
			_pScintillaEditView->execute(SCI_GOTOLINE, lastVisibleDocLine);
		else
			_pScintillaEditView->execute(SCI_GOTOLINE, firstVisibleDocLine);

		// Get the editor's higher/lower Y, then compute the map's higher/lower Y
		int higherPos = _pScintillaEditView->execute(SCI_POSITIONFROMLINE, firstVisibleDocLine);
		int lowerPos = _pScintillaEditView->execute(SCI_POSITIONFROMLINE, lastVisibleDocLine);
		int higherY = _pScintillaEditView->execute(SCI_POINTYFROMPOSITION, 0, higherPos);
		int lowerY = _pScintillaEditView->execute(SCI_POINTYFROMPOSITION, 0, lowerPos);
		if (lowerY == 0)
		{
			int lineHeight = _pScintillaEditView->execute(SCI_TEXTHEIGHT, firstVisibleDocLine);
			lowerY = nbLine * lineHeight + firstVisibleDocLine;
		}

		// Update view zone in map
		_vzDlg.drawZone(higherY, lowerY);		
	}
}

void DocumentMap::doMove()
{
	RECT rc;
	POINT pt = {0,0};
	::ClientToScreen(_pScintillaEditView->getHSelf(), &pt);
	_pScintillaEditView->getClientRect(rc);
	::MoveWindow(_vzDlg.getHSelf(), pt.x, pt.y, (rc.right - rc.left), (rc.bottom - rc.top), TRUE);
}

void DocumentMap::fold(int line, bool foldOrNot)
{
	//bool isExpanded = _pScintillaEditView->execute(SCI_GETFOLDEXPANDED, line) != 0;
	_pScintillaEditView->fold(line, foldOrNot);
}

void DocumentMap::scrollMap(bool direction, moveMode whichMode)
{
	// Visible line for the code view
	int firstVisibleDisplayLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
	int nbLine = (*_ppEditView)->execute(SCI_LINESONSCREEN, firstVisibleDisplayLine);
	int lastVisibleDisplayLine = firstVisibleDisplayLine + nbLine;

	int nbLine2go = (whichMode == perLine?1:nbLine);
	int line2go = 1;

	if (direction == moveDown)
	{
		line2go = (*_ppEditView)->execute(SCI_DOCLINEFROMVISIBLE, lastVisibleDisplayLine + nbLine2go);
	}
	else
	{
		line2go = (*_ppEditView)->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine - nbLine2go);
	}
	(*_ppEditView)->execute(SCI_GOTOLINE, line2go);

	scrollMap();
}

/*
void DocumentMap::wrapScintilla(bool doWrap) 
{
	_pScintillaEditView->wrap(doWrap);
}
*/

BOOL CALLBACK DocumentMap::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {
			//_glassHandle = ::GetDlgItem(_hSelf, IDC_GLASS);
			HWND hwndScintilla = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
			_pScintillaEditView = (ScintillaEditView *)::SendMessage(_hParent, NPPM_INTERNAL_GETSCINTEDTVIEW, 0, (LPARAM)hwndScintilla);
			::SendMessage(_pScintillaEditView->getHSelf(), SCI_SETZOOM, (WPARAM)-10, 0);
			::SendMessage(_pScintillaEditView->getHSelf(), SCI_SETVSCROLLBAR, FALSE, 0);
			::SendMessage(_pScintillaEditView->getHSelf(), SCI_SETHSCROLLBAR, FALSE, 0);
			reloadMap();

			_pScintillaEditView->showIndentGuideLine(false);
			
			_pScintillaEditView->showMargin(0, false);
			_pScintillaEditView->showMargin(1, false);
			_pScintillaEditView->showMargin(2, false);
			_pScintillaEditView->showMargin(3, false);

			_pScintillaEditView->display();

			_vzDlg.init(::GetModuleHandle(NULL), _hSelf);
			_vzDlg.doDialog();
			(NppParameters::getInstance())->SetTransparent(_vzDlg.getHSelf(), 50); // 0 <= transparancy < 256
            return TRUE;
        }
		case WM_DESTROY:
		{
			//::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)_pScintillaEditView->getHSelf());
			//printStr(TEXT("fw"));
		}
		return TRUE;
/*
		case 2230:
		{
			if (_pScintillaEditView->isWrap())
			{
				int width = wParam;
				int height = lParam;
				int docWrapLineCount = (*_ppEditView)->getCurrentDocWrapLineCount();
				int mapWrapLineCount = _pScintillaEditView->getCurrentDocWrapLineCount();
				while (mapWrapLineCount != docWrapLineCount)
				{
					if (mapWrapLineCount < docWrapLineCount)
						width--;
					else
						width++;
					::MoveWindow(_pScintillaEditView->getHSelf(), 0, 0, width , height, TRUE);
					mapWrapLineCount = _pScintillaEditView->getCurrentDocWrapLineCount();
				}
			}
		}
		return TRUE;
*/
        case WM_SIZE:
        {
			if (_pScintillaEditView)
			{
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);
				if (_pScintillaEditView->isWrap())
				{
					int mapStringWidth = _pScintillaEditView->execute(SCI_TEXTWIDTH, 0, (LPARAM)"aiueolW");
					int normalStringWidth = (*_ppEditView)->execute(SCI_TEXTWIDTH, 0, (LPARAM)"aiueolW");
					
					width = ((*_ppEditView)->getWidth())*mapStringWidth/normalStringWidth;
					::MoveWindow(_pScintillaEditView->getHSelf(), 0, 0, width , height, TRUE);
				}
				else
				{
					::MoveWindow(_pScintillaEditView->getHSelf(), 0, 0, width , height, TRUE);
				}

				if (_vzDlg.isCreated())
				{
					POINT pt = {0,0};
					::ClientToScreen(_pScintillaEditView->getHSelf(), &pt);
					::MoveWindow(_vzDlg.getHSelf(), pt.x, pt.y, width-4, height-4, TRUE);
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
					_vzDlg.display(false);
					return TRUE;
				}

				case DMN_SWITCHIN:
				{
					_vzDlg.display();
					reloadMap();
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

					POINT pt = {0,0};
					::ClientToScreen(_pScintillaEditView->getHSelf(), &pt);
					::MoveWindow(_vzDlg.getHSelf(), pt.x, pt.y, width-4, height-4, TRUE);

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
			//int currentHeight = _vzDlg.getViewerHeight();

			bool dir = (newPosY < currentCenterPosY)?moveUp:moveDown;
			int pixelPerLine = _pScintillaEditView->execute(SCI_TEXTHEIGHT, 0); 

			int jumpDistance;
			if (dir == moveUp)
			{
				jumpDistance = currentCenterPosY - newPosY;
				int nbLine2jump = jumpDistance/pixelPerLine;
				int firstVisibleDisplayLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
				firstVisibleDisplayLine -= nbLine2jump;
				if (firstVisibleDisplayLine < 0)
					firstVisibleDisplayLine = 0;
				(*_ppEditView)->execute(SCI_GOTOLINE, (*_ppEditView)->execute(SCI_DOCLINEFROMVISIBLE, firstVisibleDisplayLine));
			}
			else
			{
				jumpDistance = newPosY - currentCenterPosY;
				int nbLine2jump = jumpDistance/pixelPerLine;
				int firstVisibleDisplayLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
				int nbLine = (*_ppEditView)->execute(SCI_LINESONSCREEN, firstVisibleDisplayLine);
				int lastVisibleDisplayLine = firstVisibleDisplayLine + nbLine;

				lastVisibleDisplayLine += nbLine2jump;
				(*_ppEditView)->execute(SCI_GOTOLINE, (*_ppEditView)->execute(SCI_DOCLINEFROMVISIBLE, lastVisibleDisplayLine));
			}
			scrollMap();
			
/*
			int newHigher = newPosY - (currentHeight / 2);
			int newLower = newPosY + (currentHeight / 2);

			if (newHigher < 0)
			{
				newHigher = 0;
				newLower = currentHeight;
			}
*/

		}
		return TRUE;

		case DOCUMENTMAP_MOUSEWHEEL:
		{
			//::SendMessage((*_ppEditView)->getHSelf(), WM_MOUSEWHEEL, wParam, lParam);
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
	
	//const COLORREF red = RGB(0xFF, 0x00, 0x00);
	//const COLORREF yellow = RGB(0xFF, 0xFF, 0);
	//const COLORREF grey = RGB(128, 128, 128);
	const COLORREF orange = RGB(0xFF, 0x80, 0x00);
	//const COLORREF liteGrey = RGB(192, 192, 192);
	const COLORREF white = RGB(0xFF, 0xFF, 0xFF);
	HBRUSH hbrushFg = CreateSolidBrush(orange);
	HBRUSH hbrushBg = CreateSolidBrush(white);					
	FillRect(pdis->hDC, &rc, hbrushBg);

	rc.top = _higherY;
	rc.bottom = _lowerY;
	FillRect(pdis->hDC, &rc, hbrushFg);
	/*
	HPEN hpen = CreatePen(PS_SOLID, 1, RGB(0x00, 0x00, 0x00));
	HPEN holdPen = (HPEN)SelectObject(pdis->hDC, hpen);
	
	::MoveToEx(pdis->hDC, 0 , _higherY , NULL);
	::LineTo(pdis->hDC, rc.left, _higherY);
	::MoveToEx(pdis->hDC, 0 , _lowerY , NULL);
	::LineTo(pdis->hDC, rc.left, _lowerY);

	SelectObject(pdis->hDC, holdPen);
	DeleteObject(hpen);
	*/
	DeleteObject(hbrushFg);
	DeleteObject(hbrushBg);
}

void ViewZoneDlg::doDialog()
{
	if (!isCreated())
		create(IDD_VIEWZONE);
	display();
};

BOOL CALLBACK ViewZoneDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
        case WM_INITDIALOG :
		{
			_viewZoneCanvas = ::GetDlgItem(_hSelf, IDC_VIEWZONECANVAS);
			::SetWindowLongPtrW(_viewZoneCanvas, GWL_USERDATA, reinterpret_cast<LONG>(this));
			_canvasDefaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_viewZoneCanvas, GWL_WNDPROC, reinterpret_cast<LONG>(canvasStaticProc)));
			return TRUE;
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
			if (_viewZoneCanvas)
			{
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);
				::MoveWindow(_viewZoneCanvas, 0, 0, width , height, TRUE);
			}
            break;
        }
		case WM_COMMAND : 
		{/*
			switch (wParam)
			{
				case IDCANCEL :
				case IDOK : 
					display(false);
					return TRUE;

				default :
					break;
			}
		*/
			return TRUE;
		}

		case WM_MOUSEWHEEL :
		{
			/*
			if (LOWORD(wParam) & MK_RBUTTON)
			{
				::SendMessage(_hParent, Message, wParam, lParam);
				return TRUE;
			}
			*/

			//Have to perform the scroll first, because the first/last line do not get updated untill after the scroll has been parsed
			::SendMessage(_hParent, DOCUMENTMAP_MOUSEWHEEL, wParam, lParam);
		}
		return TRUE;

		case WM_DESTROY :
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CALLBACK ViewZoneDlg::canvasStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	ViewZoneDlg *pViewZoneDlg = reinterpret_cast<ViewZoneDlg *>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
	if (!pViewZoneDlg)
		return FALSE;
	return pViewZoneDlg->canvas_runProc(hwnd, message, wParam, lParam);
}

BOOL CALLBACK ViewZoneDlg::canvas_runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
				::SendMessage(_hParent, DOCUMENTMAP_SCROLL, (WPARAM)moveUp, 0);
			}
			if (wParam == VK_DOWN)
			{
				::SendMessage(_hParent, DOCUMENTMAP_SCROLL, (WPARAM)moveDown, 0);
			}
			if (wParam == VK_PRIOR)
			{
				::SendMessage(_hParent, DOCUMENTMAP_SCROLL, (WPARAM)moveUp, 1);
			}
			if (wParam == VK_NEXT)
			{
				::SendMessage(_hParent, DOCUMENTMAP_SCROLL, (WPARAM)moveDown, 1);
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