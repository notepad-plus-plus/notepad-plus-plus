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
		scrollMap();
	}
}

void DocumentMap::scrollMap()
{
	if (_pScintillaEditView && _ppEditView)
	{
		// Visible line for the code view
		int firstVisibleLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
		int nbLine = (*_ppEditView)->execute(SCI_LINESONSCREEN, firstVisibleLine);
		//int nbTotalLine  = (*_ppEditView)->getCurrentLineNumber();
		
		int lastVisibleLine = firstVisibleLine + nbLine;

		// Visible line for the map view
		int firstVisibleLineMap = _pScintillaEditView->execute(SCI_GETFIRSTVISIBLELINE);
		int nbLineMap = _pScintillaEditView->execute(SCI_LINESONSCREEN, firstVisibleLine);
		int lastVisibleLineMap = firstVisibleLineMap + nbLineMap;

		if (lastVisibleLineMap < lastVisibleLine)
			_pScintillaEditView->execute(SCI_GOTOLINE, lastVisibleLine);
		else
			_pScintillaEditView->execute(SCI_GOTOLINE, firstVisibleLine);

		int higherPos = _pScintillaEditView->execute(SCI_POSITIONFROMLINE, firstVisibleLine);
		int lowerPos = _pScintillaEditView->execute(SCI_POSITIONFROMLINE, lastVisibleLine);
		int higherY = _pScintillaEditView->execute(SCI_POINTYFROMPOSITION, 0, higherPos);
		int lowerY = _pScintillaEditView->execute(SCI_POINTYFROMPOSITION, 0, lowerPos);
		if (lowerY == 0)
		{
			int lineHeight = _pScintillaEditView->execute(SCI_TEXTHEIGHT, firstVisibleLine);
			lowerY = nbLine * lineHeight + firstVisibleLine;
		}
		_vzDlg.drawZone(higherY, lowerY);		
	}
}

void DocumentMap::scrollMap(bool direction, moveMode whichMode)
{
	// Visible line for the code view
	int firstVisibleLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
	int nbLine = (*_ppEditView)->execute(SCI_LINESONSCREEN, firstVisibleLine);
	int lastVisibleLine = firstVisibleLine + nbLine;

	int nbLine2go = (whichMode == perLine?1:nbLine);
	int line2go = 1;

	if (direction == moveDown)
	{
		line2go = lastVisibleLine + nbLine2go;
	}
	else
	{
		line2go = firstVisibleLine - nbLine2go;
	}
	(*_ppEditView)->execute(SCI_GOTOLINE, line2go);

	scrollMap();
}


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
			_pScintillaEditView->defineDocType((*_ppEditView)->getCurrentBuffer()->getLangType());			
			
			_pScintillaEditView->showMargin(0, false);
			_pScintillaEditView->showMargin(1, false);
			_pScintillaEditView->showMargin(2, false);
			_pScintillaEditView->showMargin(3, false);

			_pScintillaEditView->display();
/*
			_glassHandle = ::CreateWindowEx(0,//TEXT("Static"), 
                                TEXT("STATIC"), TEXT("STATIC"),
                                WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                0,
                                0, 
                                100,
                                100,
                                _hSelf, 
                                (HMENU) NULL, 
								::GetModuleHandle(NULL),
                                NULL);
			::ShowWindow(_glassHandle, SW_SHOW);
*/
			_vzDlg.init(::GetModuleHandle(NULL), _hSelf);
			_vzDlg.doDialog();
			(NppParameters::getInstance())->SetTransparent(_vzDlg.getHSelf(), 100); // 0 <= transparancy < 256
            return TRUE;
        }
		case WM_DESTROY:
		{
			//::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)_pScintillaEditView->getHSelf());
			//printStr(TEXT("fw"));
		}
		return TRUE;

        case WM_SIZE:
        {
			if (_pScintillaEditView)
			{
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);
				::MoveWindow(_pScintillaEditView->getHSelf(), 0, 0, width , height, TRUE);
				
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
					return TRUE;
				}

				case DMN_SWITCHOFF:
				{
					_vzDlg.display(false);
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
				int firstVisibleLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
				firstVisibleLine -= nbLine2jump;
				if (firstVisibleLine < 0)
					firstVisibleLine = 0;
				(*_ppEditView)->execute(SCI_GOTOLINE, firstVisibleLine);
			}
			else
			{
				jumpDistance = newPosY - currentCenterPosY;
				int nbLine2jump = jumpDistance/pixelPerLine;
				int firstVisibleLine = (*_ppEditView)->execute(SCI_GETFIRSTVISIBLELINE);
				int nbLine = (*_ppEditView)->execute(SCI_LINESONSCREEN, firstVisibleLine);
				int lastVisibleLine = firstVisibleLine + nbLine;

				lastVisibleLine += nbLine2jump;
				(*_ppEditView)->execute(SCI_GOTOLINE, lastVisibleLine);
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

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void ViewZoneDlg::drawPreviewZone(DRAWITEMSTRUCT *pdis)
{
	//switch (pdis->itemAction)
	{
		//case ODA_DRAWENTIRE:
			//switch (pdis->CtlID)
			{
				//case IDC_GLASS:
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
					//FrameRect(pdis->hDC, &rc, (HBRUSH) GetStockObject(GRAY_BRUSH));
					//break;
				}
			}
			// *** FALL THROUGH ***
		/*
		case ODA_SELECT:
			break;
		case ODA_FOCUS:
			break;
		default:
			break;
		*/
	}
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
				//::SetWindowPos(_pScintillaEditView->getHSelf(), _glassHandle,  0, 0, width, height, SWP_SHOWWINDOW);
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
			::MessageBoxA(NULL,"Destroy","",MB_OK);
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