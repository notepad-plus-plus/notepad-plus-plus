// This file is part of Notepad++ project
// Copyright (C)2024 Don HO <don.h@free.fr>

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

#include <windows.h>
#include <cstring>

#include "StaticDialog.h"
#include "Common.h"
#include "NppDarkMode.h"
#include "dpiManagerV2.h"

StaticDialog::~StaticDialog()
{
	if (StaticDialog::isCreated())
	{
		// Prevent run_dlgProc from doing anything, since its virtual
		::SetWindowLongPtr(_hSelf, GWLP_USERDATA, 0);
		StaticDialog::destroy();
	}
}

void StaticDialog::destroy()
{
	::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<WPARAM>(_hSelf));
	::DestroyWindow(_hSelf);
}

void StaticDialog::getMappedChildRect(HWND hChild, RECT& rcChild) const
{
	::GetClientRect(hChild, &rcChild);
	::MapWindowPoints(hChild, _hSelf, reinterpret_cast<LPPOINT>(&rcChild), 2);
}

void StaticDialog::getMappedChildRect(int idChild, RECT& rcChild) const
{
	const HWND hChild = ::GetDlgItem(_hSelf, idChild);
	getMappedChildRect(hChild, rcChild);
}

void StaticDialog::redrawDlgItem(const int nIDDlgItem, bool forceUpdate) const
{
	RECT rcDlgItem{};
	const HWND hDlgItem = ::GetDlgItem(_hSelf, nIDDlgItem);
	getMappedChildRect(hDlgItem, rcDlgItem);
	::InvalidateRect(_hSelf, &rcDlgItem, TRUE);

	if (forceUpdate)
		::UpdateWindow(hDlgItem);
}

POINT StaticDialog::getTopPoint(HWND hwnd, bool isLeft) const
{
	RECT rc{};
	::GetWindowRect(hwnd, &rc);

	POINT p{};
	if (isLeft)
		p.x = rc.left;
	else
		p.x = rc.right;

	p.y = rc.top;
	::ScreenToClient(_hSelf, &p);
	return p;
}

void StaticDialog::goToCenter(UINT swpFlags)
{
	RECT rc{};
	::GetClientRect(_hParent, &rc);
	if ((rc.left == rc.right) || (rc.top == rc.bottom))
		swpFlags |= SWP_NOSIZE; // sizing has no sense here

	POINT center{};
	center.x = rc.left + (rc.right - rc.left)/2;
	center.y = rc.top + (rc.bottom - rc.top)/2;
	::ClientToScreen(_hParent, &center);
	if ((center.x == -32000) && (center.y == -32000)) // https://devblogs.microsoft.com/oldnewthing/20041028-00/?p=37453
		swpFlags |= SWP_NOMOVE; // moving has no sense here (owner wnd is minimized)

	int x = center.x - (_rc.right - _rc.left)/2;
	int y = center.y - (_rc.bottom - _rc.top)/2;

	::SetWindowPos(_hSelf, HWND_TOP, x, y, _rc.right - _rc.left, _rc.bottom - _rc.top, swpFlags);
	if (((swpFlags & SWP_NOMOVE) != SWP_NOMOVE) && ((swpFlags & SWP_SHOWWINDOW) == SWP_SHOWWINDOW))
		::SendMessageW(_hSelf, DM_REPOSITION, 0, 0);
}

bool StaticDialog::moveForDpiChange()
{
	if (_dpiManager.getDpi() != _dpiManager.getDpiForWindow(_hParent))
	{
		goToCenter(SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOACTIVATE);
		return true;
	}
	return false;
}

void StaticDialog::display(bool toShow, bool enhancedPositioningCheckWhenShowing) const
{
	if (toShow)
	{
		if (enhancedPositioningCheckWhenShowing)
		{
			RECT testPositionRc{}, candidateRc{};

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
			RECT workAreaRect{};
			RECT rc{};
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
				::SetWindowPos(_hSelf, nullptr, newLeft, newTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			else
				::SendMessageW(_hSelf, DM_REPOSITION, 0, 0);
		}
	}

	Window::display(toShow);
}

RECT StaticDialog::getViewablePositionRect(RECT testPositionRc) const
{
	HMONITOR hMon = ::MonitorFromRect(&testPositionRc, MONITOR_DEFAULTTONULL);

	MONITORINFO mi{};
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

[[nodiscard]] static bool dupDlgTemplate(HINSTANCE hInst, int dialogID, std::vector<std::byte>& dlgTemplateData)
{
	// Get Dlg Template resource
	HRSRC hDialogRC = ::FindResourceW(hInst, MAKEINTRESOURCE(dialogID), RT_DIALOG);
	if (!hDialogRC)
		return false;

	HGLOBAL hDlgTemplate = ::LoadResource(hInst, hDialogRC);
	if (!hDlgTemplate)
		return false;

	const auto* pDlgTemplate = static_cast<DLGTEMPLATE*>(::LockResource(hDlgTemplate));
	if (!pDlgTemplate)
		return false;

	// Duplicate Dlg Template resource
	const size_t sizeDlg = ::SizeofResource(hInst, hDialogRC);
	dlgTemplateData.resize(sizeDlg);
	::memcpy(dlgTemplateData.data(), pDlgTemplate, sizeDlg);

	return true;
}

[[nodiscard]] static bool setRTLResource(std::vector<std::byte>& dlgTemplateData)
{
	auto* pMyDlgTemplateEx = reinterpret_cast<DLGTEMPLATEEX*>(dlgTemplateData.data());
	if (!pMyDlgTemplateEx)
		return false;

	if (pMyDlgTemplateEx->signature == 0xFFFF && pMyDlgTemplateEx->dlgVer == 1)
	{
		pMyDlgTemplateEx->exStyle |= WS_EX_LAYOUTRTL;
	}
	else
	{
		auto* pMyDlgTemplate = reinterpret_cast<DLGTEMPLATE*>(dlgTemplateData.data());
		pMyDlgTemplate->dwExtendedStyle |= WS_EX_LAYOUTRTL;
	}

	return true;
}

// inspired by https://stackoverflow.com/questions/14370238
[[nodiscard]] static std::byte* skipSz(std::byte* pData)
{
	auto* str = reinterpret_cast<WCHAR*>(pData); // string
	const size_t length = std::wcslen(str);
	return reinterpret_cast<std::byte*>(str + length + 1);
}

[[nodiscard]] static std::byte* skipSzOrOrd(std::byte* pData)
{
	auto* ptrElement = reinterpret_cast<WORD*>(pData);

	if (*ptrElement == 0xFFFF) // ordinal
	{
		ptrElement += 2;
	}
	else // string or no element, same as skipSz
	{
		const auto* str = reinterpret_cast<WCHAR*>(ptrElement);
		ptrElement += (std::wcslen(str) + 1);
	}

	return reinterpret_cast<std::byte*>(ptrElement);
}

[[nodiscard]] static int setFontResource(std::vector<std::byte>& dlgTemplateData, WORD fontSize)
{
	enum result { failed = -1, noFont, success };

	auto* pMyDlgTemplateEx = reinterpret_cast<DLGTEMPLATEEX*>(dlgTemplateData.data());
	if (!pMyDlgTemplateEx || pMyDlgTemplateEx->signature != 0xFFFF || pMyDlgTemplateEx->dlgVer != 1)
		return failed;

	// No need to check DS_SHELLFONT, as it already includes DS_SETFONT (DS_SETFONT | DS_FIXEDSYS)
	if ((pMyDlgTemplateEx->style & DS_SETFONT) != DS_SETFONT)
		return noFont; // allow RTL set before

	auto* pData = reinterpret_cast<std::byte*>(pMyDlgTemplateEx);
	pData += sizeof(DLGTEMPLATEEX);
	// sz_Or_Ord menu
	pData = skipSzOrOrd(pData);
	// sz_Or_Ord windowClass;
	pData = skipSzOrOrd(pData);
	// WCHAR title[titleLen]
	pData = skipSz(pData);
	// WORD pointSize;
	auto* pointSize = reinterpret_cast<WORD*>(pData);
	if (fontSize > 0)
	{
		*pointSize = static_cast<WORD>(DPIManagerV2::scaleFontForFactor(fontSize));
	}

	return success;
}

[[nodiscard]] static bool modifyResource(
	HINSTANCE hInst,
	int dialogID,
	std::vector<std::byte>& dlgTemplateData,
	bool isRTL,
	WORD fontSize)
{
	if (!dupDlgTemplate(hInst, dialogID, dlgTemplateData))
		return false;

	if (isRTL && !setRTLResource(dlgTemplateData))
		return false;

	if (fontSize != 0 && setFontResource(dlgTemplateData, fontSize) < 0)
		return false;

	return true;
}

HWND StaticDialog::myCreateDialogIndirectParam(int dialogID, bool isRTL, WORD fontSize, DLGPROC myDlgProc)
{
	std::vector<std::byte> dlgTemplateData;

	if (!modifyResource(_hInst, dialogID, dlgTemplateData, isRTL, fontSize))
		return ::CreateDialogParam(_hInst, MAKEINTRESOURCE(dialogID), _hParent, myDlgProc, reinterpret_cast<LPARAM>(this));

	return ::CreateDialogIndirectParam(_hInst, reinterpret_cast<DLGTEMPLATE*>(dlgTemplateData.data()), _hParent, myDlgProc, reinterpret_cast<LPARAM>(this));
}

INT_PTR StaticDialog::myCreateDialogBoxIndirectParam(int dialogID, bool isRTL, WORD fontSize)
{
	std::vector<std::byte> dlgTemplateData;

	if (!modifyResource(_hInst, dialogID, dlgTemplateData, isRTL, fontSize))
		return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(dialogID), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));

	return ::DialogBoxIndirectParam(_hInst, reinterpret_cast<DLGTEMPLATE*>(dlgTemplateData.data()), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
}

void StaticDialog::create(int dialogID, bool isRTL, bool msgDestParent, WORD fontSize)
{
	_hSelf = StaticDialog::myCreateDialogIndirectParam(dialogID, isRTL, fontSize);

	if (!_hSelf)
	{
		std::wstring errMsg = L"CreateDialogParam() return NULL.\rGetLastError(): ";
		errMsg += GetLastErrorAsString();
		::MessageBox(NULL, errMsg.c_str(), L"In StaticDialog::create()", MB_OK);
		return;
	}

	setDpi();

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
