// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef VERTICALFILESWITCHER_H
#define  VERTICALFILESWITCHER_H

#include "DockingDlgInterface.h"
#include "VerticalFileSwitcher_rc.h"
#include "VerticalFileSwitcherListView.h"

#define FS_PROJECTPANELTITLE		TEXT("Doc Switcher")

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

	int nbSelectedFiles() const {
		return _fileListView.nbSelectedFiles();
	};

	std::vector<SwitcherFileInfo> getSelectedFiles(bool reverse = false) const {
		return _fileListView.getSelectedFiles(reverse);
	};

	void reload(){
		_fileListView.deleteColumn(1);
		_fileListView.deleteColumn(0);
		_fileListView.reload();
	};

	virtual void setBackgroundColor(COLORREF bgColour) {
		_fileListView.setBackgroundColor(bgColour);
    };

	virtual void setForegroundColor(COLORREF fgColour) {
		_fileListView.setForegroundColor(fgColour);
    };

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
	VerticalFileSwitcherListView _fileListView;
	HIMAGELIST _hImaLst;
};
#endif // VERTICALFILESWITCHER_H
