/*
this file is part of Function List Plugin for Notepad++
Copyright (C)2005 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef DOCKINGDLGINTERFACE_H
#define DOCKINGDLGINTERFACE_H

#ifndef DOCKING_RESOURCE_H
#include "dockingResource.h"
#endif //DOCKING_RESOURCE_H

#ifndef DOCKING_H
#include "Docking.h"
#endif //DOCKING_H

class DockingDlgInterface : public StaticDialog
{
public:
	DockingDlgInterface(): StaticDialog(), _HSource(NULL),\
		_dlgID(-1), _isFloating(TRUE), _iDockedPos(0), _pluginName(TEXT("")) {};

	DockingDlgInterface(int dlgID): StaticDialog(), _HSource(NULL),\
		_dlgID(dlgID), _isFloating(TRUE), _iDockedPos(0), _pluginName(TEXT("")) {};
	
	virtual void init(HINSTANCE hInst, HWND parent)	{
		StaticDialog::init(hInst, parent);
		TCHAR temp[MAX_PATH];
		::GetModuleFileName((HMODULE)hInst, temp, MAX_PATH);
		_moduleName = PathFindFileName(temp);
	};

    void create(tTbData * data, bool isRTL = false){
		StaticDialog::create(_dlgID, isRTL);
		TCHAR temp[MAX_PATH];
		::GetWindowText(_hSelf, temp, MAX_PATH);
		_pluginName = temp;
        // user information
		data->hClient		= _hSelf;
		data->pszName		= (TCHAR *)_pluginName.c_str();

		// supported features by plugin
		data->uMask			= 0;

		// additional info
		data->pszAddInfo	= NULL;
	};

	virtual void updateDockingDlg() {
		::SendMessage(_hParent, NPPM_DMMUPDATEDISPINFO, 0, (LPARAM)_hSelf);
	}

    virtual void destroy() {
    };

	virtual void display(bool toShow = true) const {
		::SendMessage(_hParent, toShow?NPPM_DMMSHOW:NPPM_DMMHIDE, 0, (LPARAM)_hSelf);
	};

	const TCHAR * getPluginFileName() const {
		return _moduleName.c_str();
	};

protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM, LPARAM lParam)
	{
		switch (message) 
		{

			case WM_NOTIFY: 
			{
				LPNMHDR	pnmh	= (LPNMHDR)lParam;

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
	
	// Handles
    HWND			_HSource;
	int				_dlgID;
	bool            _isFloating;
	int				_iDockedPos;
	generic_string  _moduleName;
	generic_string  _pluginName;
};

#endif // DOCKINGDLGINTERFACE_H
