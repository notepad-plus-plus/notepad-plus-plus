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

#pragma once


#include <cstdint>

#include <windows.h>
#include <wchar.h>
#include <commctrl.h>

#include <memory>


class ProgressDlg;
using progress_ptr = std::shared_ptr<ProgressDlg>;


class ProgressDlg
{
public:
	static progress_ptr& Open(const wchar_t* info = NULL);

	static progress_ptr& Get()
	{
		return Inst;
	}

	static void Close()
	{
		Inst.reset();
	}

	~ProgressDlg();

	inline void SetInfo(const wchar_t *info) const
	{
		::SendMessageW(_hPText, WM_SETTEXT, 0, (LPARAM)info);
	}

	void Show() const;

	bool IsCancelled() const;

	unsigned NextPhase();
	bool SetMaxCount(intptr_t max, unsigned phase = 0);
	bool SetCount(intptr_t cnt, unsigned phase = 0);
	bool Advance(intptr_t cnt = 1, unsigned phase = 0);

private:
	static const wchar_t cClassName[];
	static const int cBackgroundColor;
	static const int cPBwidth;
	static const int cPBheight;
	static const int cBTNwidth;
	static const int cBTNheight;

	static const int cInitialShowDelay_ms = 500;

	static const int cPhases[];

	static progress_ptr Inst;

	static DWORD WINAPI threadFunc(LPVOID data);
	static LRESULT CALLBACK keyHookProc(int code, WPARAM wParam, LPARAM lParam);
	static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	ProgressDlg();

	// Disable copy construction and operator=
	ProgressDlg(const ProgressDlg&);
	const ProgressDlg& operator=(const ProgressDlg&);

	HWND create();
	void cancel();
	void destroy();

	inline void setPos(intptr_t pos) const
	{
		::PostMessageW(_hPBar, PBM_SETPOS, (WPARAM)pos, 0);
	}

	void update();

	BOOL thread();
	BOOL createProgressWindow();
	RECT adjustSizeAndPos(int width, int height);

	HINSTANCE		_hInst;
	volatile HWND	_hwnd;
	HANDLE			_hThread;
	HANDLE			_hActiveState;
	HFONT			_hFont;
	HWND			_hPText;
	HWND			_hPBar;
	HWND			_hBtn;
	HHOOK			_hKeyHook;

	unsigned	_phase;
	unsigned	_phaseRange;
	unsigned	_phasePosOffset;
	intptr_t	_max;
	intptr_t	_count;

	unsigned	_pos;
};
