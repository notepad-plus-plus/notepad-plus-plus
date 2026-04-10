/*
 * This file is part of ComparePlus plugin for Notepad++
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma comment (lib, "comctl32")


#include <windowsx.h>
#include <cstdlib>

#include "Compare.h"
#include "ProgressDlg.h"
#include "Tools.h"
#include "Strings.h"


const wchar_t ProgressDlg::cClassName[]		= L"ComparePlusProgressClass";
const int ProgressDlg::cBackgroundColor		= COLOR_3DFACE;
const int ProgressDlg::cPBwidth				= 600;
const int ProgressDlg::cPBheight			= 10;
const int ProgressDlg::cBTNwidth			= 80;
const int ProgressDlg::cBTNheight			= 25;


// Different compare phases progress end positions
const int ProgressDlg::cPhases[] = {
	5,		// Docs1 hashes
	10,		// Docs2 hashes
	20,		// Docs diff
	90,		// Blocks diff
	100,	// Results colorization and presentation
};


progress_ptr ProgressDlg::Inst;


progress_ptr& ProgressDlg::Open(const wchar_t* info)
{
	if (Inst)
		return Inst;

	Inst.reset(new ProgressDlg);

	if (Inst)
	{
		if (Inst->create() == NULL)
		{
			Inst.reset();
		}
		else
		{
			::EnableWindow(nppData._nppHandle, FALSE);

			if (info)
				Inst->SetInfo(info);
		}
	}

	return Inst;
}


void ProgressDlg::Show() const
{
	if (_hwnd)
	{
		if (_phase + 1 == _countof(cPhases))
		{
			::SendMessageW(_hwnd, WM_CLOSE, 0, 0);
		}
		else
		{
			::ShowWindow(_hwnd, SW_SHOWNORMAL);
			::UpdateWindow(_hwnd);
		}

		::KillTimer(_hwnd, 1);
	}
}


bool ProgressDlg::IsCancelled() const
{
	return (::WaitForSingleObject(_hActiveState, 0) != WAIT_OBJECT_0);
}


unsigned ProgressDlg::NextPhase()
{
	if (IsCancelled())
		return 0;

	if (_phase + 1 < _countof(cPhases))
	{
		_phasePosOffset = cPhases[_phase++];
		_phaseRange = cPhases[_phase] - _phasePosOffset;
		_max = _phaseRange;
		_count = 0;
		setPos(_phasePosOffset);
	}
	else
	{
		setPos(cPhases[_countof(cPhases) - 1]);
	}

	return _phase + 1;
}


bool ProgressDlg::SetMaxCount(intptr_t max, unsigned phase)
{
	if (IsCancelled())
		return false;

	if (phase == 0 || phase - 1 == _phase)
	{
		_max = max;
		_count = 0;
	}

	return true;
}


bool ProgressDlg::SetCount(intptr_t cnt, unsigned phase)
{
	if (IsCancelled())
		return false;

	if ((phase == 0 || phase - 1 == _phase) && _count < cnt && cnt <= _max)
	{
		_count = cnt;
		update();
	}

	return true;
}


bool ProgressDlg::Advance(intptr_t cnt, unsigned phase)
{
	if (IsCancelled())
		return false;

	if (phase == 0 || phase - 1 == _phase)
	{
		_count += cnt;
		update();
	}

	return true;
}


ProgressDlg::ProgressDlg() : _hwnd(NULL), _hFont(NULL), _hKeyHook(NULL),
		_phase(0), _phaseRange(cPhases[0]), _phasePosOffset(0), _max(cPhases[0]), _count(0), _pos(0)
{
	::GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_PIN, cClassName, &_hInst);

	WNDCLASSEXW wcex;

	::SecureZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize           = sizeof(wcex);
	wcex.style            = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc      = wndProc;
	wcex.hInstance        = _hInst;
	wcex.hCursor          = (HCURSOR)::LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wcex.hbrBackground    = ::GetSysColorBrush(cBackgroundColor);
	wcex.lpszClassName    = cClassName;

	::RegisterClassExW(&wcex);

	INITCOMMONCONTROLSEX icex;

	::SecureZeroMemory(&icex, sizeof(icex));
	icex.dwSize = sizeof(icex);
	icex.dwICC  = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;

	::InitCommonControlsEx(&icex);
}


ProgressDlg::~ProgressDlg()
{
	if (_hKeyHook)
		::UnhookWindowsHookEx(_hKeyHook);

	destroy();

	::EnableWindow(nppData._nppHandle, TRUE);
	::SetForegroundWindow(nppData._nppHandle);

	::UnregisterClassW(cClassName, _hInst);
}


HWND ProgressDlg::create()
{
	// Create manually reset non-signaled event
	_hActiveState = ::CreateEventW(NULL, TRUE, FALSE, NULL);
	if (!_hActiveState)
		return NULL;

	for (HWND hwnd = nppData._nppHandle; hwnd; hwnd = ::GetParent(hwnd))
		::UpdateWindow(hwnd);

	_hThread = ::CreateThread(NULL, 0, threadFunc, this, 0, NULL);
	if (!_hThread)
	{
		::CloseHandle(_hActiveState);
		return NULL;
	}

	// Wait for the progress window to be created
	::WaitForSingleObject(_hActiveState, INFINITE);

	// On progress window create fail
	if (!_hwnd)
	{
		::WaitForSingleObject(_hThread, INFINITE);
		::CloseHandle(_hThread);
		::CloseHandle(_hActiveState);
	}

	return _hwnd;
}


void ProgressDlg::cancel()
{
	::ResetEvent(_hActiveState);
	::EnableWindow(_hBtn, FALSE);

	SetInfo(Strings::get()["COMPARE_CANCELLING"].c_str());
}


void ProgressDlg::destroy()
{
	if (_hwnd)
	{
		::KillTimer(_hwnd, 1);
		::PostMessageW(_hwnd, WM_CLOSE, 0, 0);
		_hwnd = NULL;

		::WaitForSingleObject(_hThread, INFINITE);
		::CloseHandle(_hThread);
		::CloseHandle(_hActiveState);

		if (_hFont)
			::DeleteObject(_hFont);
	}
}


void ProgressDlg::update()
{
	const unsigned newPos = static_cast<unsigned>(((_count * _phaseRange) / _max) + _phasePosOffset);

	if (newPos > _pos)
	{
		_pos = newPos;
		setPos(newPos);
	}
}


DWORD WINAPI ProgressDlg::threadFunc(LPVOID data)
{
	ProgressDlg* pw = static_cast<ProgressDlg*>(data);
	return (DWORD)pw->thread();
}


BOOL ProgressDlg::thread()
{
	BOOL r = createProgressWindow();
	::SetEvent(_hActiveState);
	if (!r)
		return r;

	// Window message loop
	MSG msg;
	while ((r = ::GetMessageW(&msg, NULL, 0, 0)) != 0 && r != -1)
		::DispatchMessageW(&msg);

	return r;
}


BOOL ProgressDlg::createProgressWindow()
{
	_hwnd = ::CreateWindowExW(
			WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_OVERLAPPEDWINDOW,
			cClassName, PLUGIN_NAME, WS_POPUP | WS_CAPTION,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, _hInst, (LPVOID)this);
	if (!_hwnd)
		return FALSE;

	int width = cPBwidth + 10;
	int height = cPBheight + cBTNheight + 40;
	RECT win = adjustSizeAndPos(width, height);
	::MoveWindow(_hwnd, win.left, win.top, win.right - win.left, win.bottom - win.top, TRUE);

	::GetClientRect(_hwnd, &win);
	width = win.right - win.left;
	height = win.bottom - win.top;

	_hPText = ::CreateWindowExW(0, L"STATIC", L"",
			WS_CHILD | WS_VISIBLE | BS_TEXT | SS_PATHELLIPSIS,
			5, 5, width - 10, 20, _hwnd, NULL, _hInst, NULL);

	_hPBar = ::CreateWindowExW(0, PROGRESS_CLASS, L"Progress Bar",
			WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
			5, 30, width - 10, cPBheight,
			_hwnd, NULL, _hInst, NULL);
	::SendMessageW(_hPBar, PBM_SETRANGE, 0, MAKELPARAM(0, cPhases[_countof(cPhases) - 1]));

	_hBtn = ::CreateWindowExW(0, L"BUTTON", Strings::get()["IDCANCEL"].c_str(),
			WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_TEXT,
			(width - cBTNwidth) / 2, height - cBTNheight - 5,
			cBTNwidth, cBTNheight, _hwnd, NULL, _hInst, NULL);

	_hFont = createFontFromSystemDefault(SysFont::Message);
	if (_hFont)
	{
		::SendMessageW(_hPText, WM_SETFONT, (WPARAM)_hFont, MAKELPARAM(TRUE, 0));
		::SendMessageW(_hBtn, WM_SETFONT, (WPARAM)_hFont, MAKELPARAM(TRUE, 0));
	}

	_hKeyHook = ::SetWindowsHookExW(WH_KEYBOARD, keyHookProc, _hInst, GetCurrentThreadId());

	::ShowWindow(_hwnd, SW_HIDE);

	::SetTimer(_hwnd, 1, cInitialShowDelay_ms, NULL);

	return TRUE;
}


RECT ProgressDlg::adjustSizeAndPos(int width, int height)
{
	RECT maxWin;
	maxWin.left		= ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	maxWin.top		= ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	maxWin.right	= ::GetSystemMetrics(SM_CXVIRTUALSCREEN) + maxWin.left;
	maxWin.bottom	= ::GetSystemMetrics(SM_CYVIRTUALSCREEN) + maxWin.top;

	POINT center;

	{
		RECT biasWin;
		::GetWindowRect(nppData._nppHandle, &biasWin);
		center.x = (biasWin.left + biasWin.right) / 2;
		center.y = (biasWin.top + biasWin.bottom) / 2;
	}

	RECT win = maxWin;
	win.right = win.left + width;
	win.bottom = win.top + height;

	::AdjustWindowRectEx(&win, (DWORD)::GetWindowLongPtrW(_hwnd, GWL_STYLE), FALSE,
			(DWORD)::GetWindowLongPtrW(_hwnd, GWL_EXSTYLE));

	width = win.right - win.left;
	height = win.bottom - win.top;

	if (width < maxWin.right - maxWin.left)
	{
		win.left = center.x - width / 2;
		if (win.left < maxWin.left)
			win.left = maxWin.left;
		win.right = win.left + width;
		if (win.right > maxWin.right)
		{
			win.right = maxWin.right;
			win.left = win.right - width;
		}
	}
	else
	{
		win.left = maxWin.left;
		win.right = maxWin.right;
	}

	if (height < maxWin.bottom - maxWin.top)
	{
		win.top = center.y - height / 2;
		if (win.top < maxWin.top)
			win.top = maxWin.top;
		win.bottom = win.top + height;
		if (win.bottom > maxWin.bottom)
		{
			win.bottom = maxWin.bottom;
			win.top = win.bottom - height;
		}
	}
	else
	{
		win.top = maxWin.top;
		win.bottom = maxWin.bottom;
	}

	return win;
}


LRESULT CALLBACK ProgressDlg::keyHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0 && Inst)
	{
		if (Inst->_hBtn == ::GetFocus())
		{
			// Key is pressed
			if (!(lParam & (1 << 31)))
			{
				if (wParam == VK_RETURN || wParam == VK_ESCAPE)
				{
					Inst->cancel();
					return 1;
				}
			}
		}
	}

	return ::CallNextHookEx(NULL, code, wParam, lParam);
}


LRESULT APIENTRY ProgressDlg::wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
		case WM_CREATE:
			return 0;

		case WM_SETFOCUS:
			::SetFocus(Inst->_hBtn);
			return 0;

		case WM_COMMAND:
			if (HIWORD(wparam) == BN_CLICKED)
			{
				Inst->cancel();
				return 0;
			}
			break;

		case WM_TIMER:
			Inst->Show();
			return 0;

		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
	}

	return ::DefWindowProcW(hwnd, umsg, wparam, lparam);
}
