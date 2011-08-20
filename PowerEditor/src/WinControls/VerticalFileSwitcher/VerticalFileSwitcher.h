/*
this file is part of notepad++
Copyright (C)2011 Don HO <donho@altern.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a Copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef VERTICALFILESWITCHER_H
#define  VERTICALFILESWITCHER_H

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "VerticalFileSwitcher_rc.h"
#include "VerticalFileSwitcherListView.h"

class VerticalFileSwitcher : public DockingDlgInterface {
public:
	VerticalFileSwitcher(): DockingDlgInterface(IDD_FILESWITCHER_PANEL) {};

	void init(HINSTANCE hInst, HWND hPere, HIMAGELIST hImaLst) {
		DockingDlgInterface::init(hInst, hPere);
		_hImaLst = hImaLst;
	};

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };
	
	//Activate document in scintilla by using the internal index
	void activateDoc(TaskLstFnStatus *tlfs) const;

	int newItem(int bufferID, int iView){
		return _fileListView.newItem(bufferID, iView);
	};

	int closeItem(int bufferID, int iView){
		return _fileListView.closeItem(bufferID, iView);
	};

	void activateItem(int bufferID, int iView) {
		_fileListView.activateItem(bufferID, iView);
	};

	void setItemIconStatus(int bufferID) {
		_fileListView.setItemIconStatus(bufferID) ;
	};

	generic_string getFullFilePath(size_t i) const {
		return _fileListView.getFullFilePath(i);
	};

	int setHeaderOrder(LPNMLISTVIEW pnm_list_view);

protected:
	virtual BOOL CALLBACK VerticalFileSwitcher::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	VerticalFileSwitcherListView _fileListView;
	HIMAGELIST _hImaLst;
};
#endif // VERTICALFILESWITCHER_H
