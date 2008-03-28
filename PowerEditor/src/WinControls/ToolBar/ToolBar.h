//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#ifndef TOOL_BAR_H
#define TOOL_BAR_H

#include "Window.h"
#include "resource.h"
#include "Notepad_plus_msgs.h"



#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

#include <commctrl.h>
#include <vector>
using namespace std;


#ifndef TB_SETIMAGELIST
#define TB_SETIMAGELIST	(WM_USER+48)
#endif

#ifndef TB_SETHOTIMAGELIST
#define TB_SETHOTIMAGELIST	(WM_USER+52)
#endif

#ifndef TB_SETDISABLEDIMAGELIST
#define TB_SETDISABLEDIMAGELIST (WM_USER+54)
#endif

enum toolBarStatusType {TB_HIDE, TB_SMALL, TB_LARGE, TB_STANDARD};

#include "ImageListSet.h"


typedef struct {
	UINT		message;		// identification of icon in tool bar (menu ID)
	HBITMAP		hBmp;			// bitmap for toolbar
	HICON		hIcon;			// icon for toolbar
} tDynamicList;


class ToolBar : public Window
{
public :
	ToolBar():Window(), _pTBB(NULL), _nrButtons(0), _nrDynButtons(0), _nrTotalButtons(0), _nrCurrentButtons(0), _visible(true) {};
	virtual ~ToolBar(){};

	virtual bool init(HINSTANCE hInst, HWND hPere, toolBarStatusType type, 
		ToolBarButtonUnit *buttonUnitArray, int arraySize);

	virtual void destroy() {
		delete [] _pTBB;
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
		_toolBarIcons.destroy();
	};
	void enable(int cmdID, bool doEnable) const {
		::SendMessage(_hSelf, TB_ENABLEBUTTON, cmdID, (LPARAM)doEnable);
	};

	int getHeight() const {
		if (!::IsWindowVisible(_hSelf))
			return 0;
		return Window::getHeight();
	};

	int getWidth() const;

	void reduce() {
		if (_state == TB_SMALL)
			return;

		_toolBarIcons.resizeIcon(16);
		bool recreate = (_state == TB_STANDARD);
		setState(TB_SMALL);
		reset(recreate);	//recreate toolbar if std icons were used
		Window::redraw();
	};
	void enlarge() {
		if (_state == TB_LARGE)
			return;

		_toolBarIcons.resizeIcon(32);
		bool recreate = (_state == TB_STANDARD);
		setState(TB_LARGE);
		reset(recreate);	//recreate toolbar if std icons were used
		Window::redraw();
	};
	void setToUglyIcons() {
		if (_state == TB_STANDARD) 
			return;
		bool recreate = true;
		setState(TB_STANDARD);
		reset(recreate);	//must recreate toolbar if setting to internal bitmaps
		Window::redraw();
	}
	void hide() {
		if (getState() == TB_HIDE)
			return;
		display(false);
	}

	void display(bool toShow = true) {
		Window::display(toShow);
		_visible = toShow;
	};

	bool getCheckState(int ID2Check) const {
		return bool(::SendMessage(_hSelf, TB_GETSTATE, (WPARAM)ID2Check, 0) & TBSTATE_CHECKED);
	};

	void setCheck(int ID2Check, bool willBeChecked) const {
		::SendMessage(_hSelf, TB_CHECKBUTTON, (WPARAM)ID2Check, (LPARAM)MAKELONG(willBeChecked, 0));
	};

	toolBarStatusType getState() const {
		bool test = _visible;
		return _visible?_state:TB_HIDE;
	};

	bool changeIcons(int whichLst, int iconIndex, const char *iconLocation){
		return _toolBarIcons.replaceIcon(whichLst, iconIndex, iconLocation);
	};

	void registerDynBtn(UINT message, toolbarIcons* hBmp);

private :
	TBBUTTON *_pTBB;
	ToolBarIcons _toolBarIcons;
	toolBarStatusType _state;
	bool _visible;
	vector<tDynamicList> _vDynBtnReg;
	size_t _nrButtons;
	size_t _nrDynButtons;
	size_t _nrTotalButtons;
	size_t _nrCurrentButtons;


	void setDefaultImageList() {
		::SendMessage(_hSelf, TB_SETIMAGELIST , (WPARAM)0, (LPARAM)_toolBarIcons.getDefaultLst());
	};
	void setHotImageList() {
		::SendMessage(_hSelf, TB_SETHOTIMAGELIST , (WPARAM)0, (LPARAM)_toolBarIcons.getHotLst());
	};
	void setDisableImageList() {
		::SendMessage(_hSelf, TB_SETDISABLEDIMAGELIST, (WPARAM)0, (LPARAM)_toolBarIcons.getDisableLst());
	};

	void reset(bool create = false);
	void setState(toolBarStatusType state) {
		if(state == TB_HIDE) {	//do not set the state to something else
			_visible = false;
		} else {
			_visible = true;
			_state = state;
		}
	}
	
};

class ReBar : public Window
{
public :
	ReBar():Window(), _pToolBar(NULL) {};

	virtual void destroy() {
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
	};

	void init(HINSTANCE hInst, HWND hPere, ToolBar *pToolBar);
	void reNew();

private:
	REBARINFO _rbi;
	REBARBANDINFO _rbBand;
	ToolBar *_pToolBar;
};

#endif // TOOL_BAR_H
