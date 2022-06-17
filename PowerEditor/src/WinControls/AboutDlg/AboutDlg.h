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


class AboutDlg : public StaticDialog
{
public :
	AboutDlg() = default;

	void doDialog();

    virtual void destroy() {
        _emailLink.destroy();
        _pageLink.destroy();
    };

protected :
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
    URLCtrl _emailLink;
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

	virtual void destroy() {
		_copyToClipboardLink.destroy();
	};

protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	typedef const CHAR * (__cdecl * PWINEGETVERSION)();
	generic_string _debugInfoStr;
	generic_string _debugInfoDisplay;
	const generic_string _cmdLinePlaceHolder { L"$COMMAND_LINE_PLACEHOLDER$" };
	bool _isAdmin = false;
	generic_string _loadedPlugins;
	URLCtrl _copyToClipboardLink;
};

class DoSaveOrNotBox : public StaticDialog
{
public:
	DoSaveOrNotBox() = default;

	void init(HINSTANCE hInst, HWND parent, const TCHAR* fn, bool isMulti) {
		Window::init(hInst, parent);
		if (fn)
			_fn = fn;

		_isMulti = isMulti;
	};

	void doDialog(bool isRTL = false);

	virtual void destroy() {};

	int getClickedButtonId() const {
		return clickedButtonId;
	};

	void changeLang();

protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	int clickedButtonId = -1;
	generic_string _fn;
	bool _isMulti = false;
};

class DoSaveAllBox : public StaticDialog
{
public:
	DoSaveAllBox() = default;

	void doDialog(bool isRTL = false);

	virtual void destroy() {};

	int getClickedButtonId() const {
		return clickedButtonId;
	};

	void changeLang();

protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	int clickedButtonId = -1;
};
