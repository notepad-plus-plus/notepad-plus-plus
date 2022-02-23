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

#include <stdio.h>
#include <windows.h>
#include "StaticDialog.h"
#include "Common.h"
#include "NppDarkMode.h"

StaticDialog::~StaticDialog()
{
	if (isCreated())
	{
		// Prevent run_dlgProc from doing anything, since its virtual
		::SetWindowLongPtr(_hSelf, GWLP_USERDATA, 0);
		destroy();
	}
}

void StaticDialog::destroy()
{
	::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<WPARAM>(_hSelf));
	::DestroyWindow(_hSelf);
}

POINT StaticDialog::getTopPoint(HWND hwnd, bool isLeft) const
{
	RECT rc;
	::GetWindowRect(hwnd, &rc);

	POINT p;
	if (isLeft)
		p.x = rc.left;
	else
		p.x = rc.right;

	p.y = rc.top;
	::ScreenToClient(_hSelf, &p);
	return p;
}

void StaticDialog::goToCenter()
{
	RECT rc;
	::GetClientRect(_hParent, &rc);
	POINT center;
	center.x = rc.left + (rc.right - rc.left)/2;
	center.y = rc.top + (rc.bottom - rc.top)/2;
	::ClientToScreen(_hParent, &center);

	int x = center.x - (_rc.right - _rc.left)/2;
	int y = center.y - (_rc.bottom - _rc.top)/2;

	::SetWindowPos(_hSelf, HWND_TOP, x, y, _rc.right - _rc.left, _rc.bottom - _rc.top, SWP_SHOWWINDOW);
}

void StaticDialog::display(bool toShow, bool enhancedPositioningCheckWhenShowing) const
{
	if (toShow)
	{
		if (enhancedPositioningCheckWhenShowing)
		{
			RECT testPositionRc, candidateRc;

			getWindowRect(testPositionRc);

			candidateRc = getViewablePositionRect(testPositionRc);

			if ((testPositionRc.left != candidateRc.left) || (testPositionRc.top != candidateRc.top))
			{
				::MoveWindow(_hSelf, candidateRc.left, candidateRc.top, 
					candidateRc.right - candidateRc.left, candidateRc.bottom - candidateRc.top, TRUE);
			}
		}
		else
		{
			// If the user has switched from a dual monitor to a single monitor since we last
			// displayed the dialog, then ensure that it's still visible on the single monitor.
			RECT workAreaRect = {};
			RECT rc = {};
			::SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);
			::GetWindowRect(_hSelf, &rc);
			int newLeft = rc.left;
			int newTop = rc.top;
			int margin = ::GetSystemMetrics(SM_CYSMCAPTION);

			if (newLeft > ::GetSystemMetrics(SM_CXVIRTUALSCREEN) - margin)
				newLeft -= rc.right - workAreaRect.right;
			if (newLeft + (rc.right - rc.left) < ::GetSystemMetrics(SM_XVIRTUALSCREEN) + margin)
				newLeft = workAreaRect.left;
			if (newTop > ::GetSystemMetrics(SM_CYVIRTUALSCREEN) - margin)
				newTop -= rc.bottom - workAreaRect.bottom;
			if (newTop + (rc.bottom - rc.top) < ::GetSystemMetrics(SM_YVIRTUALSCREEN) + margin)
				newTop = workAreaRect.top;

			if ((newLeft != rc.left) || (newTop != rc.top)) // then the virtual screen size has shrunk
				// Remember that MoveWindow wants width/height.
				::MoveWindow(_hSelf, newLeft, newTop, rc.right - rc.left, rc.bottom - rc.top, TRUE);
		}
	}

	Window::display(toShow);
}

RECT StaticDialog::getViewablePositionRect(RECT testPositionRc) const
{
	HMONITOR hMon = ::MonitorFromRect(&testPositionRc, MONITOR_DEFAULTTONULL);

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);

	bool rectPosViewableWithoutChange = false;

	if (hMon != NULL)
	{
		// rect would be at least partially visible on a monitor

		::GetMonitorInfo(hMon, &mi);
		
		int margin = ::GetSystemMetrics(SM_CYBORDER) + ::GetSystemMetrics(SM_CYSIZEFRAME) + ::GetSystemMetrics(SM_CYCAPTION);

		// require that the title bar of the window be in a viewable place so the user can see it to grab it with the mouse
		if ((testPositionRc.top >= mi.rcWork.top) && (testPositionRc.top + margin <= mi.rcWork.bottom) &&
			// require that some reasonable amount of width of the title bar be in the viewable area:
			(testPositionRc.right - (margin * 2) > mi.rcWork.left) && (testPositionRc.left + (margin * 2) < mi.rcWork.right))
		{
			rectPosViewableWithoutChange = true;
		}
	}
	else
	{
		// rect would not have been visible on a monitor; get info about the nearest monitor to it

		hMon = ::MonitorFromRect(&testPositionRc, MONITOR_DEFAULTTONEAREST);

		::GetMonitorInfo(hMon, &mi);
	}

	RECT returnRc = testPositionRc;

	if (!rectPosViewableWithoutChange)
	{
		// reposition rect so that it would be viewable on current/nearest monitor, centering if reasonable
		
		LONG testRectWidth = testPositionRc.right - testPositionRc.left;
		LONG testRectHeight = testPositionRc.bottom - testPositionRc.top;
		LONG monWidth = mi.rcWork.right - mi.rcWork.left;
		LONG monHeight = mi.rcWork.bottom - mi.rcWork.top;

		returnRc.left = mi.rcWork.left;
		if (testRectWidth < monWidth) returnRc.left += (monWidth - testRectWidth) / 2;
		returnRc.right = returnRc.left + testRectWidth;

		returnRc.top = mi.rcWork.top;
		if (testRectHeight < monHeight) returnRc.top += (monHeight - testRectHeight) / 2;
		returnRc.bottom = returnRc.top + testRectHeight;
	}

	return returnRc;
}

HGLOBAL StaticDialog::makeRTLResource(int dialogID, DLGTEMPLATE **ppMyDlgTemplate)
{
	// Get Dlg Template resource
	HRSRC  hDialogRC = ::FindResource(_hInst, MAKEINTRESOURCE(dialogID), RT_DIALOG);
	if (!hDialogRC)
		return NULL;

	HGLOBAL  hDlgTemplate = ::LoadResource(_hInst, hDialogRC);
	if (!hDlgTemplate)
		return NULL;

	DLGTEMPLATE *pDlgTemplate = static_cast<DLGTEMPLATE *>(::LockResource(hDlgTemplate));
	if (!pDlgTemplate)
		return NULL;

	// Duplicate Dlg Template resource
	unsigned long sizeDlg = ::SizeofResource(_hInst, hDialogRC);
	HGLOBAL hMyDlgTemplate = ::GlobalAlloc(GPTR, sizeDlg);
	if (!hMyDlgTemplate) return nullptr;

	*ppMyDlgTemplate = static_cast<DLGTEMPLATE *>(::GlobalLock(hMyDlgTemplate));
	if (!*ppMyDlgTemplate) return nullptr;

	::memcpy(*ppMyDlgTemplate, pDlgTemplate, sizeDlg);

	DLGTEMPLATEEX* pMyDlgTemplateEx = reinterpret_cast<DLGTEMPLATEEX *>(*ppMyDlgTemplate);
	if (!pMyDlgTemplateEx) return nullptr;

	if (pMyDlgTemplateEx->signature == 0xFFFF)
		pMyDlgTemplateEx->exStyle |= WS_EX_LAYOUTRTL;
	else
		(*ppMyDlgTemplate)->dwExtendedStyle |= WS_EX_LAYOUTRTL;

	return hMyDlgTemplate;
}

void StaticDialog::create(int dialogID, bool isRTL, bool msgDestParent)
{
	if (isRTL)
	{
		DLGTEMPLATE *pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(dialogID, &pMyDlgTemplate);
		_hSelf = ::CreateDialogIndirectParam(_hInst, pMyDlgTemplate, _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
		::GlobalFree(hMyDlgTemplate);
	}
	else
		_hSelf = ::CreateDialogParam(_hInst, MAKEINTRESOURCE(dialogID), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));

	if (!_hSelf)
	{
		generic_string errMsg = TEXT("CreateDialogParam() return NULL.\rGetLastError(): ");
		errMsg += GetLastErrorAsString();
		::MessageBox(NULL, errMsg.c_str(), TEXT("In StaticDialog::create()"), MB_OK);
		return;
	}

	NppDarkMode::setDarkTitleBar(_hSelf);

	// if the destination of message NPPM_MODELESSDIALOG is not its parent, then it's the grand-parent
	::SendMessage(msgDestParent ? _hParent : (::GetParent(_hParent)), NPPM_MODELESSDIALOG, MODELESSDIALOGADD, reinterpret_cast<WPARAM>(_hSelf));
}

intptr_t CALLBACK StaticDialog::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::setDarkTitleBar(hwnd);

			StaticDialog *pStaticDlg = reinterpret_cast<StaticDialog *>(lParam);
			pStaticDlg->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(lParam));
			::GetWindowRect(hwnd, &(pStaticDlg->_rc));
			pStaticDlg->run_dlgProc(message, wParam, lParam);

			return TRUE;
		}

		default:
		{
			StaticDialog *pStaticDlg = reinterpret_cast<StaticDialog *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (!pStaticDlg)
				return FALSE;
			return pStaticDlg->run_dlgProc(message, wParam, lParam);
		}
	}
}

