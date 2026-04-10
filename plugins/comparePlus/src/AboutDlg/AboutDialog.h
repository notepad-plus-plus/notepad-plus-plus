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


#include <string>

#include "PluginInterface.h"
#include "DockingFeature/StaticDialog.h"
#include "URLCtrl.h"


class AboutDialog : public StaticDialog
{

public:
	AboutDialog(HINSTANCE hInst, HWND hWnd, const std::wstring& libGit2Ver, const std::wstring& sqlite3Ver) :
		_libGit2Ver(libGit2Ver), _sqlite3Ver(sqlite3Ver)
	{
		Window::init(hInst, hWnd);
	}

	~AboutDialog()
	{
		destroy();
	}

	UINT doDialog();

	virtual void destroy()
	{
		_urlRepo.destroy();
		_helpLink.destroy();
	};

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	void updateLocalization();

	URLCtrl			_urlRepo;
	URLCtrl			_helpLink;

	std::wstring	_libGit2Ver;
	std::wstring	_sqlite3Ver;
};
