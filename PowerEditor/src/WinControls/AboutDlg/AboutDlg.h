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


#pragma once

#include "URLCtrl.h"
#include "resource.h"
#include "StaticDialog.h"

#define LICENCE_TXT \
L"This program is free software; you can redistribute it and/or \
modify it under the terms of the GNU General Public License \
as published by the Free Software Foundation; either \
version 3 of the License, or at your option any later version.\r\n\
\r\n\
This program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the \
GNU General Public License for more details. \r\n\
\r\n\
You should have received a copy of the GNU General Public License \
along with this program. If not, see <https://www.gnu.org/licenses/>."


class AboutDlg : public StaticDialog
{
public :
	AboutDlg() = default;

	void doDialog();

	void destroy() override {
		//_emailLink.destroy();
		_pageLink.destroy();
		if (_hIcon != nullptr)
		{
			::DestroyIcon(_hIcon);
			_hIcon = nullptr;
		}
	};

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private :
	//URLCtrl _emailLink;
	URLCtrl _pageLink;
	HICON _hIcon = nullptr;
};


class DebugInfoDlg : public StaticDialog
{
public:
	DebugInfoDlg() = default;

	void init(HINSTANCE hInst, HWND parent, bool isAdmin, const std::wstring& loadedPlugins) {
		_isAdmin = isAdmin;
		_loadedPlugins = loadedPlugins;
		Window::init(hInst, parent);
	};

	void doDialog();

	void refreshDebugInfo();

	void destroy() override {};

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	typedef const CHAR * (__cdecl * PWINEGETVERSION)();
	std::wstring _debugInfoStr;
	std::wstring _debugInfoDisplay;
	const std::wstring _cmdLinePlaceHolder { L"$COMMAND_LINE_PLACEHOLDER$" };
	bool _isAdmin = false;
	std::wstring _loadedPlugins;
};

class DoSaveOrNotBox : public StaticDialog
{
public:
	DoSaveOrNotBox() = default;

	void init(HINSTANCE hInst, HWND parent, const wchar_t* fn, bool isMulti) {
		Window::init(hInst, parent);
		if (fn)
			_fn = fn;

		_isMulti = isMulti;
	};

	void doDialog(bool isRTL = false);

	void destroy() override {};

	int getClickedButtonId() const {
		return clickedButtonId;
	};

	void changeLang();

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	int clickedButtonId = -1;
	std::wstring _fn;
	bool _isMulti = false;
};

class DoSaveAllBox : public StaticDialog
{
public:
	DoSaveAllBox() = default;

	void doDialog(bool isRTL = false);

	void destroy() override {};

	int getClickedButtonId() const {
		return clickedButtonId;
	};

	void changeLang();

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	int clickedButtonId = -1;
};
