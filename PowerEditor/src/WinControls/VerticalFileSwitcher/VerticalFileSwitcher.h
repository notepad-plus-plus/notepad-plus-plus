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

#define FS_PROJECTPANELTITLE		L"Document List"

struct sortCompareData {
  HWND hListView = nullptr;
  int columnIndex = 0;
  int sortDirection = 0;
};

LRESULT run_listViewProc(WNDPROC oldEditProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

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

	void closeDoc(TaskLstFnStatus *tlfs) const;

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

	void setItemColor(BufferID bufferID) {
		_fileListView.setItemColor(bufferID);
	};

	std::wstring getFullFilePath(size_t i) const {
		return _fileListView.getFullFilePath(i);
	};

	int setHeaderOrder(int columnIndex);
	void updateHeaderArrow();

	int nbSelectedFiles() const {
		return _fileListView.nbSelectedFiles();
	};

	std::vector<BufferViewInfo> getSelectedFiles(bool reverse = false) const {
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
		
		auto r = GetRValue(bgColour);
		auto g = GetGValue(bgColour);
		auto b = GetBValue(bgColour);

		constexpr int luminenceIncrementBy = 333; // 33.3 %

		// main color is blue
		// but difference must be high
		// can have similar blue color as header
		constexpr int difference = 12;
		const auto bAdjusted = static_cast<BYTE>(std::max<int>(0, static_cast<int>(b) - difference));
		if (bAdjusted > r && bAdjusted > g)
		{
			// using values from NppDarkMode.cpp
			// from double calculatePerceivedLighness(COLORREF c)
			// double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;
			// values multiplied by 1024 and then shift result by 10 - "fake" divide by 1024
			// for performance
			const auto grayscale = static_cast<BYTE>((r * 218 + g * 732 + b * 74) >> 10);
			_bgColor = ::ColorAdjustLuma(RGB(grayscale, grayscale, grayscale), luminenceIncrementBy, TRUE);
		}
		else
		{
			_bgColor = ::ColorAdjustLuma(bgColour, luminenceIncrementBy, TRUE);
		}
	};

	virtual void setForegroundColor(COLORREF fgColour) {
		_fileListView.setForegroundColor(fgColour);
    };
protected:
	HMENU _hGlobalMenu = NULL;
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void initPopupMenus();
	void popupMenuCmd(int cmdID);

	static LRESULT CALLBACK listViewStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		const auto dlg = (VerticalFileSwitcher*)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return (run_listViewProc(dlg->_defaultListViewProc, hwnd, message, wParam, lParam));
	};
private:
	bool colHeaderRClick = false;
	int _lastSortingColumn = 0;
	int _lastSortingDirection = SORT_DIRECTION_NONE;
	VerticalFileSwitcherListView _fileListView;
	HIMAGELIST _hImaLst = nullptr;
	WNDPROC _defaultListViewProc = nullptr;

	static COLORREF _bgColor;
	static const UINT_PTR _fileSwitcherNotifySubclassID = 42;
	static LRESULT listViewNotifyCustomDraw(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK FileSwitcherNotifySubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	void autoSubclassWindowNotify(HWND hParent);
};
