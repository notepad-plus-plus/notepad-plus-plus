//this file is part of docking functionality for Notepad++
//Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef DOCKINGMANAGER_H
#define DOCKINGMANAGER_H

#include "Docking.h"
#include "Window.h"
#include "DockingCont.h"
#include "DockingSplitter.h"
#include <vector>
#include <commctrl.h>
#include "SplitterContainer.h"
#include "dockingResource.h"
#include "Parameters.h"

#define DSPC_CLASS_NAME TEXT("dockingManager")

using namespace std;


#define	CONT_MAP_MAX	50


class DockingManager : public Window
{
public :
	DockingManager();
	~DockingManager(){
		// delete 4 splitters
		for (int i = 0; i < DOCKCONT_MAX; i++)
		{
			delete _vSplitter[i];
		}
	};

	void init(HINSTANCE hInst, HWND hWnd, Window ** ppWin);
	virtual void reSizeTo(RECT & rc);

	void setClientWnd(Window ** ppWin) {
		_ppWindow = ppWin;
		_ppMainWindow = ppWin;
	};

	void showContainer(HWND hCont, BOOL view = TRUE) {
		for (size_t iCont = 0; iCont < _vContainer.size(); iCont++)
		{
			if (_vContainer[iCont]->getHSelf() == hCont)
				showContainer(iCont, view);
		}
	}

	void showContainer(UINT	uCont, BOOL view = TRUE) {
		_vContainer[uCont]->doDialog((view == TRUE));
		onSize();
	}

	void updateContainerInfo(HWND hClient) {
		for (size_t iCont = 0; iCont < _vContainer.size(); iCont++)
		{
			if (_vContainer[iCont]->updateInfo(hClient) == TRUE)
			{
				break;
			}
		}
	};

	void createDockableDlg(tTbData data, int iCont = CONT_LEFT, bool isVisible = false);
	void setActiveTab(int iCont, int iItem);

	void showDockableDlg(HWND hDlg, BOOL view) {
		tTbData*	pTbData =	NULL;

		for (size_t i = 0; i < _vContainer.size(); i++)
		{
			pTbData = _vContainer[i]->findToolbarByWnd(hDlg);
			if (pTbData != NULL)
			{
				_vContainer[i]->showToolbar(pTbData, view);
				return;
			}
		}
	};

	void showDockableDlg(TCHAR* pszName, BOOL view) {
		tTbData*	pTbData =	NULL;

		for (size_t i = 0; i < _vContainer.size(); i++)
		{
			pTbData = _vContainer[i]->findToolbarByName(pszName);
			if (pTbData != NULL)
			{
				_vContainer[i]->showToolbar(pTbData, view);
				return;
			}
		}
	};

	DockingCont* toggleActiveTb(DockingCont* pContSrc, UINT message, BOOL bNew = FALSE, LPRECT rcFloat = NULL);
	DockingCont* toggleVisTb(DockingCont* pContSrc, UINT message, LPRECT rcFloat = NULL);
	void		 toggleActiveTb(DockingCont* pContSrc, DockingCont* pContTgt);
	void		 toggleVisTb(DockingCont* pContSrc, DockingCont* pContTgt);

	/* get number of container */
	int  GetContainer(DockingCont* pCont); 

	/* get all container in vector */
	vector<DockingCont*> & getContainerInfo(void) {
		return _vContainer;
	};
	/* get dock data (sized areas) */
	void getDockInfo(tDockMgr *pDockData) {
		*pDockData	= _dockData;
	};

	/* setting styles of docking */
	void setStyleCaption(BOOL captionOnTop) {
		_vContainer[CONT_TOP]->setCaptionTop(captionOnTop);
		_vContainer[CONT_BOTTOM]->setCaptionTop(captionOnTop);
	};

	void setTabStyle(BOOL orangeLine) {
		for (size_t i = 0; i < _vContainer.size(); i++)
			_vContainer[i]->setTabStyle(orangeLine);
	};

	int getDockedContSize(int iCont)
	{
		if ((iCont == CONT_TOP) || (iCont == CONT_BOTTOM))
			return _dockData.rcRegion[iCont].bottom;
		else if ((iCont == CONT_LEFT) || (iCont == CONT_RIGHT))
			return _dockData.rcRegion[iCont].right;
		else
			return -1;
	};

	void setDockedContSize(int iCont, int iSize)
	{
		if ((iCont == CONT_TOP) || (iCont == CONT_BOTTOM))
			_dockData.rcRegion[iCont].bottom = iSize;
		else if ((iCont == CONT_LEFT) || (iCont == CONT_RIGHT))
			_dockData.rcRegion[iCont].right = iSize;
		else
			return;
		onSize();
	};

	virtual void destroy() {
		::DestroyWindow(_hSelf);
	};

private :

	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	void onSize(void);

	void	toggleTb(DockingCont* pContSrc, DockingCont* pContTgt, tTbData TbData);

	/* test if container exists */
	BOOL ContExists(size_t iCont);
	int	 FindEmptyContainer(void);

	LRESULT SendNotify(HWND hWnd, UINT message) {
		NMHDR	nmhdr;

		nmhdr.code		= message;
		nmhdr.hwndFrom	= _hParent;
		nmhdr.idFrom	= ::GetDlgCtrlID(_hParent);
		::SendMessage(hWnd, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
		return ::GetWindowLongPtr(hWnd, DWL_MSGRESULT);
	};

private:
	/* Handles */
	Window						**_ppWindow;

	RECT						_rcWork;
	RECT						_rect;
	Window						**_ppMainWindow;

	/* handles all the icons */
	vector<HWND>				_vImageList;
	HIMAGELIST					_hImageList;

	vector<DockingCont*>		_vContainer;
	tDockMgr					_dockData;

	static BOOL					_isRegistered;
	BOOL						_isInitialized;

	/* container map for startup (restore settings) */
	int							_iContMap[CONT_MAP_MAX];

	/* splitter data */
	vector<DockingSplitter*>	_vSplitter;
};

#endif //DOCKINGMANAGER_H
