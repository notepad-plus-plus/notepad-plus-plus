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
TEXT("This program is free software; you can redistribute it and/or \
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
along with this program. If not, see <https://www.gnu.org/licenses/>.")

#define ID_YES_ALL IDRETRY
#define ID_NO_ALL IDIGNORE

class AboutDlg : public StaticDialog
{
public :
	AboutDlg() = default;

	void doDialog();

	void destroy() override {
		//_emailLink.destroy();
		_pageLink.destroy();
	};

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private :
	//URLCtrl _emailLink;
	URLCtrl _pageLink;
};


class DebugInfoDlg : public StaticDialog
{
public:
	DebugInfoDlg() = default;

	void init(HINSTANCE hInst, HWND parent, bool isAdmin, const generic_string& loadedPlugins) {
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
	generic_string _debugInfoStr;
	generic_string _debugInfoDisplay;
	const generic_string _cmdLinePlaceHolder { L"$COMMAND_LINE_PLACEHOLDER$" };
	bool _isAdmin = false;
	generic_string _loadedPlugins;
};

class DoSaveOrNotBox : public StaticDialog
{
public:
	DoSaveOrNotBox() = default;

	enum BoxType { t_default, t_keepFiles };
	void init(HINSTANCE hInst, HWND parent, const TCHAR* fn, bool isMulti, BoxType boxType = t_default) {
		Window::init(hInst, parent);
		if (fn)
			_fn = fn;

		_isMulti = isMulti;
		_boxType = boxType;
	};

	void doDialog(bool isRTL = false);

	void destroy() override {};

	int getClickedButtonId() const {
		return _clickedButtonId;
	};

	void changeLang();

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	int _clickedButtonId = -1;
	generic_string _fn;
	bool _isMulti = false;
	BoxType _boxType = t_default;
};

class DoSaveAllBox : public StaticDialog
{
public:
	DoSaveAllBox() = default;

	void doDialog(bool isRTL = false);

	void destroy() override {};

	int getClickedButtonId() const {
		return _clickedButtonId;
	};

	void changeLang();

protected:
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	int _clickedButtonId = -1;
};
