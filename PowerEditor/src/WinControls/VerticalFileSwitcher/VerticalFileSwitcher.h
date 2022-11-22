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

#include "DockingDlgInterface.h"
#include "VerticalFileSwitcher_rc.h"
#include "VerticalFileSwitcherListView.h"

#define FS_PROJECTPANELTITLE		TEXT("Document List")

struct sortCompareData {
  HWND hListView = nullptr;
  int columnIndex = 0;
  int sortDirection = 0;
};

class VerticalFileSwitcher : public DockingDlgInterface {
public:
	VerticalFileSwitcher(): DockingDlgInterface(IDD_DOCLIST) {};

	void init(HINSTANCE hInst, HWND hPere, HIMAGELIST hImaLst) {
		DockingDlgInterface::init(hInst, hPere);
		_hImaLst = hImaLst;
	};

	virtual void display(bool toShow = true) const; 

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };
	
	//Activate document in scintilla by using the internal index
	void activateDoc(TaskLstFnStatus *tlfs) const;

	int newItem(BufferID bufferID, int iView){
		return _fileListView.newItem(bufferID, iView);
	};

	int closeItem(BufferID bufferID, int iView){
		return _fileListView.closeItem(bufferID, iView);
	};

	void activateItem(BufferID bufferID, int iView) {
		_fileListView.activateItem(bufferID, iView);
	};

	void setItemIconStatus(BufferID bufferID) {
		_fileListView.setItemIconStatus(bufferID) ;
	};

	generic_string getFullFilePath(size_t i) const {
		return _fileListView.getFullFilePath(i);
	};

	int setHeaderOrder(int columnIndex);
	void updateHeaderArrow();

	int nbSelectedFiles() const {
		return _fileListView.nbSelectedFiles();
	};

	std::vector<SwitcherFileInfo> getSelectedFiles(bool reverse = false) const {
		return _fileListView.getSelectedFiles(reverse);
	};
	
	void startColumnSort();
	
	void reload(){
		_fileListView.reload();
		startColumnSort();
	};
	
	void updateTabOrder(){
		if (_lastSortingDirection == SORT_DIRECTION_NONE) {
			_fileListView.reload();
		}
	};

	virtual void setBackgroundColor(COLORREF bgColour) {
		_fileListView.setBackgroundColor(bgColour);
    };

	virtual void setForegroundColor(COLORREF fgColour) {
		_fileListView.setForegroundColor(fgColour);
    };
protected:
	HMENU _hGlobalMenu = NULL;

	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void initPopupMenus();
	void popupMenuCmd(int cmdID);
private:
	bool colHeaderRClick = false;
	int _lastSortingColumn = 0;
	int _lastSortingDirection = SORT_DIRECTION_NONE;
	VerticalFileSwitcherListView _fileListView;
	HIMAGELIST _hImaLst = nullptr;
};
