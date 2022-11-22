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

#include <vector>
#include <windows.h>
#include <commctrl.h>
#include "Window.h"
#include "DockingCont.h"
#include "SplitterContainer.h"

#define DSPC_CLASS_NAME TEXT("dockingManager")
#define	CONT_MAP_MAX	50

class DockingSplitter;

class DockingManager : public Window
{
public :
	DockingManager();
	~DockingManager();

	void init(HINSTANCE hInst, HWND hWnd, Window ** ppWin);
	virtual void reSizeTo(RECT & rc);

	void setClientWnd(Window ** ppWin) {
		_ppWindow = ppWin;
		_ppMainWindow = ppWin;
	};

	void showFloatingContainers(bool show);

	void updateContainerInfo(HWND hClient);
	void createDockableDlg(tTbData data, int iCont = CONT_LEFT, bool isVisible = false);
	void setActiveTab(int iCont, int iItem);
	void showDockableDlg(HWND hDlg, BOOL view);
	void showDockableDlg(TCHAR* pszName, BOOL view);

	DockingCont* toggleActiveTb(DockingCont* pContSrc, UINT message, BOOL bNew = FALSE, LPRECT rcFloat = NULL);
	DockingCont* toggleVisTb(DockingCont* pContSrc, UINT message, LPRECT rcFloat = NULL);
	void		 toggleActiveTb(DockingCont* pContSrc, DockingCont* pContTgt);
	void		 toggleVisTb(DockingCont* pContSrc, DockingCont* pContTgt);

	// get number of container
	int  GetContainer(DockingCont* pCont); 

	// get all container in vector
	std::vector<DockingCont*> & getContainerInfo() {
		return _vContainer;
	};
	// get dock data (sized areas)
	void getDockInfo(tDockMgr *pDockData) {
		*pDockData	= _dockData;
	};

	// setting styles of docking
	void setStyleCaption(BOOL captionOnTop) {
		_vContainer[CONT_TOP]->setCaptionTop(captionOnTop);
		_vContainer[CONT_BOTTOM]->setCaptionTop(captionOnTop);
	};

	void setTabStyle(BOOL orangeLine) {
		for (size_t i = 0; i < _vContainer.size(); ++i)
			_vContainer[i]->setTabStyle(orangeLine);
	};

	int getDockedContSize(int iCont);
	void setDockedContSize(int iCont, int iSize);
	virtual void destroy();
	void resize();

private :
	Window						**_ppWindow = nullptr;
	RECT						_rcWork = {};
	RECT						_rect = {};
	Window						**_ppMainWindow = nullptr;
	std::vector<HWND>			_vImageList;
	HIMAGELIST					_hImageList = nullptr;
	std::vector<DockingCont*>	_vContainer;
	tDockMgr					_dockData;
	static BOOL					_isRegistered;
	BOOL						_isInitialized = FALSE;
	int							_iContMap[CONT_MAP_MAX] = { 0 };
	std::vector<DockingSplitter*>	_vSplitter;


	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void	toggleTb(DockingCont* pContSrc, DockingCont* pContTgt, tTbData TbData);

	// test if container exists
	BOOL ContExists(size_t iCont);
	int	 FindEmptyContainer();
	LRESULT SendNotify(HWND hWnd, UINT message);
	

};

