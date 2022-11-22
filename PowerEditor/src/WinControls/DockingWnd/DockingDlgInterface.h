// This file is part of Notepad++ project
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#include "dockingResource.h"
#include "Docking.h"

#include <assert.h>
#include <shlwapi.h>
#include "Common.h"
#include "StaticDialog.h"
#include "NppDarkMode.h"



class DockingDlgInterface : public StaticDialog
{
public:
	DockingDlgInterface() = default;
	explicit DockingDlgInterface(int dlgID): _dlgID(dlgID) {}

	virtual void init(HINSTANCE hInst, HWND parent) {
		StaticDialog::init(hInst, parent);
		TCHAR temp[MAX_PATH];
		::GetModuleFileName(reinterpret_cast<HMODULE>(hInst), temp, MAX_PATH);
		_moduleName = ::PathFindFileName(temp);
	}

    void create(tTbData* data, bool isRTL = false) {
		assert(data != nullptr);
		StaticDialog::create(_dlgID, isRTL);
		TCHAR temp[MAX_PATH];
		::GetWindowText(_hSelf, temp, MAX_PATH);
		_pluginName = temp;

        // user information
		data->hClient = _hSelf;
		data->pszName = _pluginName.c_str();

		// supported features by plugin
		data->uMask = 0;

		// additional info
		data->pszAddInfo = NULL;
	}

	virtual void updateDockingDlg() {
		::SendMessage(_hParent, NPPM_DMMUPDATEDISPINFO, 0, reinterpret_cast<LPARAM>(_hSelf));
	}

    virtual void destroy() {
		StaticDialog::destroy();
	}

	virtual void setBackgroundColor(COLORREF) {}
	virtual void setForegroundColor(COLORREF) {}

	virtual void display(bool toShow = true) const {
		::SendMessage(_hParent, toShow ? NPPM_DMMSHOW : NPPM_DMMHIDE, 0, reinterpret_cast<LPARAM>(_hSelf));
	}

	bool isClosed() const {
		return _isClosed;
	}

	void setClosed(bool toClose) {
		_isClosed = toClose;
	}

	const TCHAR * getPluginFileName() const {
		return _moduleName.c_str();
	}

protected :
	int	_dlgID = -1;
	bool _isFloating = true;
	int _iDockedPos = 0;
	generic_string _moduleName;
	generic_string _pluginName;
	bool _isClosed = false;

	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) {
		switch (message)
		{
			case WM_ERASEBKGND:
			{
				if (!NppDarkMode::isEnabled())
				{
					break;
				}

				RECT rc = {};
				getClientRect(rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
				return TRUE;
			}
			case WM_NOTIFY: 
			{
				LPNMHDR	pnmh = reinterpret_cast<LPNMHDR>(lParam);

				if (pnmh->hwndFrom == _hParent)
				{
					switch (LOWORD(pnmh->code))
					{
						case DMN_CLOSE:
						{
							break;
						}
						case DMN_FLOAT:
						{
							_isFloating = true;
							break;
						}
						case DMN_DOCK:
						{
							_iDockedPos = HIWORD(pnmh->code);
							_isFloating = false;
							break;
						}
						default:
							break;
					}
				}
				break;
			}
			default:
				break;
		}
		return FALSE;
	};
};
