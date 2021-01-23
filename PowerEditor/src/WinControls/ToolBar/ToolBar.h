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

#include "Common.h"
#include "Window.h"
#include "Notepad_plus_msgs.h"
#include "ImageListSet.h"

#define REBAR_BAR_TOOLBAR		0
#define REBAR_BAR_SEARCH		1

#define REBAR_BAR_EXTERNAL		10
#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

enum toolBarStatusType {TB_SMALL, TB_LARGE, TB_STANDARD};


typedef struct {
	UINT		message;		// identification of icon in tool bar (menu ID)
	HBITMAP		hBmp;			// bitmap for toolbar
	HICON		hIcon;			// icon for toolbar
} tDynamicList;

struct iconLocator {
	int listIndex;
	int iconIndex;
	generic_string iconLocation;

	iconLocator(int iList, int iIcon, const generic_string& iconLoc)
		: listIndex(iList), iconIndex(iIcon), iconLocation(iconLoc){};
};

class ReBar;
class TiXmlDocument;
class TiXmlNode;

class ToolBar : public Window
{
public :
	ToolBar() = default;
	virtual ~ToolBar() = default;

    void initTheme(TiXmlDocument *toolIconsDocRoot);
	virtual bool init(HINSTANCE hInst, HWND hPere, toolBarStatusType type, 
		ToolBarButtonUnit *buttonUnitArray, int arraySize);

	virtual void destroy();
	void enable(int cmdID, bool doEnable) const {
		::SendMessage(_hSelf, TB_ENABLEBUTTON, cmdID, static_cast<LPARAM>(doEnable));
	};

	int getWidth() const;
	int getHeight() const;

	void reduce();
	void enlarge();
	void setToUglyIcons();

	bool getCheckState(int ID2Check) const {
		return bool(::SendMessage(_hSelf, TB_GETSTATE, ID2Check, 0) & TBSTATE_CHECKED);
	};

	void setCheck(int ID2Check, bool willBeChecked) const {
		::SendMessage(_hSelf, TB_CHECKBUTTON, ID2Check, MAKELONG(willBeChecked, 0));
	};

	toolBarStatusType getState() const {
		return _state;
	};

    bool changeIcons() {    
	    if (!_toolIcons)
		    return false;
	    for (size_t i = 0, len = _customIconVect.size(); i < len; ++i)
		    changeIcons(_customIconVect[i].listIndex, _customIconVect[i].iconIndex, (_customIconVect[i].iconLocation).c_str());
        return true;
    };

	bool changeIcons(int whichLst, int iconIndex, const TCHAR *iconLocation){
		return _toolBarIcons.replaceIcon(whichLst, iconIndex, iconLocation);
	};

	void registerDynBtn(UINT message, toolbarIcons* hBmp);

	void doPopop(POINT chevPoint);	//show the popup if buttons are hidden

	void addToRebar(ReBar * rebar);

private :
	TBBUTTON *_pTBB = nullptr;
	ToolBarIcons _toolBarIcons;
	toolBarStatusType _state = TB_SMALL;
	std::vector<tDynamicList> _vDynBtnReg;
	size_t _nbButtons = 0;
	size_t _nbDynButtons = 0;
	size_t _nbTotalButtons = 0;
	size_t _nbCurrentButtons = 0;
	ReBar * _pRebar = nullptr;
	REBARBANDINFO _rbBand;
    std::vector<iconLocator> _customIconVect;
    TiXmlNode *_toolIcons = nullptr;

	void setDefaultImageList() {
		::SendMessage(_hSelf, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(_toolBarIcons.getDefaultLst()));
	};
	void setHotImageList() {
		::SendMessage(_hSelf, TB_SETHOTIMAGELIST, 0, reinterpret_cast<LPARAM>(_toolBarIcons.getHotLst()));
	};
	void setDisableImageList() {
		::SendMessage(_hSelf, TB_SETDISABLEDIMAGELIST, 0, reinterpret_cast<LPARAM>(_toolBarIcons.getDisableLst()));
	};

	void reset(bool create = false);
	void setState(toolBarStatusType state) {
		_state = state;
	}
	
};

class ReBar : public Window
{
public :
	ReBar():Window() { usedIDs.clear(); };

	virtual void destroy() {
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
		usedIDs.clear();
	};

	void init(HINSTANCE hInst, HWND hPere);
	bool addBand(REBARBANDINFO * rBand, bool useID);	//useID true if ID from info should be used (false for plugins). wID in bandinfo will be set to used ID
	void reNew(int id, REBARBANDINFO * rBand);					//wID from bandinfo is used for update
	void removeBand(int id);

	void setIDVisible(int id, bool show);
	bool getIDVisible(int id);
	void setGrayBackground(int id);

private:
	std::vector<int> usedIDs;

	int getNewID();
	void releaseID(int id);
	bool isIDTaken(int id);
};
