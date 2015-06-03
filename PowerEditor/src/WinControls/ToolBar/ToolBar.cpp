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



#include "ToolBar.h"
#include "Shortcut.h"
#include "Parameters.h"
#include "FindReplaceDlg_rc.h"

const int WS_TOOLBARSTYLE = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS |TBSTYLE_FLAT | CCS_TOP | BTNS_AUTOSIZE | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER;

void ToolBar::initTheme(TiXmlDocument *toolIconsDocRoot)
{
    _toolIcons =  toolIconsDocRoot->FirstChild(TEXT("NotepadPlus"));
	if (_toolIcons)
	{
		_toolIcons = _toolIcons->FirstChild(TEXT("ToolBarIcons"));
		if (_toolIcons)
		{
			_toolIcons = _toolIcons->FirstChild(TEXT("Theme"));
			if (_toolIcons)
			{
				const TCHAR *themeDir = (_toolIcons->ToElement())->Attribute(TEXT("pathPrefix"));

				for (TiXmlNode *childNode = _toolIcons->FirstChildElement(TEXT("Icon"));
					 childNode ;
					 childNode = childNode->NextSibling(TEXT("Icon")))
				{
					int iIcon;
					const TCHAR *res = (childNode->ToElement())->Attribute(TEXT("id"), &iIcon);
					if (res)
					{
						TiXmlNode *grandChildNode = childNode->FirstChildElement(TEXT("normal"));
						if (grandChildNode)
						{
							TiXmlNode *valueNode = grandChildNode->FirstChild();
							//putain, enfin!!!
							if (valueNode)
							{
								generic_string locator = themeDir?themeDir:TEXT("");
								
								locator += valueNode->Value();
								_customIconVect.push_back(iconLocator(0, iIcon, locator));
							}
						}

						grandChildNode = childNode->FirstChildElement(TEXT("hover"));
						if (grandChildNode)
						{
							TiXmlNode *valueNode = grandChildNode->FirstChild();
							//putain, enfin!!!
							if (valueNode)
							{
								generic_string locator = themeDir?themeDir:TEXT("");
								
								locator += valueNode->Value();
								_customIconVect.push_back(iconLocator(1, iIcon, locator));
							}
						}

						grandChildNode = childNode->FirstChildElement(TEXT("disabled"));
						if (grandChildNode)
						{
							TiXmlNode *valueNode = grandChildNode->FirstChild();
							//putain, enfin!!!
							if (valueNode)
							{
								generic_string locator = themeDir?themeDir:TEXT("");
								
								locator += valueNode->Value();
								_customIconVect.push_back(iconLocator(2, iIcon, locator));
							}
						}
					}
				}
			}
		}
	}
}

bool ToolBar::init( HINSTANCE hInst, HWND hPere, toolBarStatusType type, 
					ToolBarButtonUnit *buttonUnitArray, int arraySize)
{
	Window::init(hInst, hPere);
	_state = type;
	int iconSize = NppParameters::getInstance()->_dpiManager.scaleX(_state == TB_LARGE?32:16);

	_toolBarIcons.init(buttonUnitArray, arraySize);
	_toolBarIcons.create(_hInst, iconSize);
	
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_WIN95_CLASSES|ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_USEREX_CLASSES;
	InitCommonControlsEx(&icex);

	//Create the list of buttons
	_nrButtons    = arraySize;
	_nrDynButtons = _vDynBtnReg.size();
	_nrTotalButtons = _nrButtons + (_nrDynButtons ? _nrDynButtons + 1 : 0);
	_pTBB = new TBBUTTON[_nrTotalButtons];	//add one for the extra separator

	int cmd = 0;
	int bmpIndex = -1, style;
	size_t i = 0;
	for (; i < _nrButtons ; ++i)
	{
		cmd = buttonUnitArray[i]._cmdID;
		if (cmd != 0)
		{
			++bmpIndex;
			style = BTNS_BUTTON;
		}
		else
		{
			style = BTNS_SEP;
		}

		_pTBB[i].iBitmap = (cmd != 0?bmpIndex:0);
		_pTBB[i].idCommand = cmd;
		_pTBB[i].fsState = TBSTATE_ENABLED;
		_pTBB[i].fsStyle = (BYTE)style; 
		_pTBB[i].dwData = 0; 
		_pTBB[i].iString = 0;
	}

	if (_nrDynButtons > 0) {
		//add separator
		_pTBB[i].iBitmap = 0;
		_pTBB[i].idCommand = 0;
		_pTBB[i].fsState = TBSTATE_ENABLED;
		_pTBB[i].fsStyle = BTNS_SEP;
		_pTBB[i].dwData = 0; 
		_pTBB[i].iString = 0;
		++i;
		//add plugin buttons
		for (size_t j = 0; j < _nrDynButtons ; ++j, ++i)
		{
			cmd = _vDynBtnReg[j].message;
			++bmpIndex;

			_pTBB[i].iBitmap = bmpIndex;
			_pTBB[i].idCommand = cmd;
			_pTBB[i].fsState = TBSTATE_ENABLED;
			_pTBB[i].fsStyle = BTNS_BUTTON; 
			_pTBB[i].dwData = 0; 
			_pTBB[i].iString = 0;
		}
	}

	reset(true);	//load icons etc

	return true;
}

void ToolBar::destroy() {
	if (_pRebar) {
		_pRebar->removeBand(_rbBand.wID);
		_pRebar = NULL;
	}
	delete [] _pTBB;
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
	_toolBarIcons.destroy();
};

int ToolBar::getWidth() const {
	RECT btnRect;
	int totalWidth = 0;
	for(size_t i = 0; i < _nrCurrentButtons; ++i) {
		::SendMessage(_hSelf, TB_GETITEMRECT, i, (LPARAM)&btnRect);
		totalWidth += btnRect.right - btnRect.left;
	}
	return totalWidth;
}

int ToolBar::getHeight() const {
	DWORD size = (DWORD)SendMessage(_hSelf, TB_GETBUTTONSIZE, 0, 0);
    DWORD padding = (DWORD)SendMessage(_hSelf, TB_GETPADDING, 0,0);
	int totalHeight = HIWORD(size) + HIWORD(padding);
	return totalHeight;
}

void ToolBar::reduce() 
{
	if (_state == TB_SMALL)
		return;

	int iconDpiDynamicalSize = NppParameters::getInstance()->_dpiManager.scaleX(16);
	_toolBarIcons.resizeIcon(iconDpiDynamicalSize);
	bool recreate = (_state == TB_STANDARD);
	setState(TB_SMALL);
	reset(recreate);	//recreate toolbar if std icons were used
	Window::redraw();
}

void ToolBar::enlarge()
{
	if (_state == TB_LARGE)
		return;

	int iconDpiDynamicalSize = NppParameters::getInstance()->_dpiManager.scaleX(32);
	_toolBarIcons.resizeIcon(iconDpiDynamicalSize);
	bool recreate = (_state == TB_STANDARD);
	setState(TB_LARGE);
	reset(recreate);	//recreate toolbar if std icons were used
	Window::redraw();
}

void ToolBar::setToUglyIcons()
{
	if (_state == TB_STANDARD) 
		return;
	bool recreate = true;
	setState(TB_STANDARD);
	reset(recreate);	//must recreate toolbar if setting to internal bitmaps
	Window::redraw();
}


void ToolBar::reset(bool create) 
{

	if(create && _hSelf) {
		//Store current button state information
		TBBUTTON tempBtn;
		for(size_t i = 0; i < _nrCurrentButtons; ++i) {
			::SendMessage(_hSelf, TB_GETBUTTON, (WPARAM)i, (LPARAM)&tempBtn);
			_pTBB[i].fsState = tempBtn.fsState;
		}
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
	}

	if(!_hSelf) {
		_hSelf = ::CreateWindowEx(
					WS_EX_PALETTEWINDOW,
					TOOLBARCLASSNAME,
					TEXT(""),
					WS_TOOLBARSTYLE,
					0, 0,
					0, 0,
					_hParent,
					NULL,
					_hInst,
					0);
		// Send the TB_BUTTONSTRUCTSIZE message, which is required for 
		// backward compatibility.
		::SendMessage(_hSelf, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		::SendMessage(_hSelf, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_HIDECLIPPEDBUTTONS);
	}

	if (!_hSelf)
	{
		throw std::runtime_error("ToolBar::reset : CreateWindowEx() function return null");
	}

	if (_state != TB_STANDARD)
	{
		//If non standard icons, use custom imagelists
		setDefaultImageList();
		setHotImageList();
		setDisableImageList();
	}
	else
	{
		//Else set the internal imagelist with standard bitmaps
		int iconDpiDynamicalSize = NppParameters::getInstance()->_dpiManager.scaleX(16);;
		::SendMessage(_hSelf, TB_SETBITMAPSIZE, 0, MAKELPARAM(iconDpiDynamicalSize, iconDpiDynamicalSize));

		//TBADDBITMAP addbmp = {_hInst, 0};
		TBADDBITMAP addbmp = {0, 0};
		TBADDBITMAP addbmpdyn = {0, 0};
		for (size_t i = 0 ; i < _nrButtons ; ++i)
		{
			HBITMAP hBmp = (HBITMAP)::LoadImage(_hInst, MAKEINTRESOURCE(_toolBarIcons.getStdIconAt(i)), IMAGE_BITMAP, iconDpiDynamicalSize, iconDpiDynamicalSize, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
			addbmp.nID = (UINT_PTR)hBmp;

			//addbmp.nID = _toolBarIcons.getStdIconAt(i);
			::SendMessage(_hSelf, TB_ADDBITMAP, 1, (LPARAM)&addbmp);
		}
		if (_nrDynButtons > 0)
		{
			for (size_t j = 0; j < _nrDynButtons; ++j)
			{
				addbmpdyn.nID = (UINT_PTR)_vDynBtnReg.at(j).hBmp;
				::SendMessage(_hSelf, TB_ADDBITMAP, 1, (LPARAM)&addbmpdyn);
			}
		}
	}

	if (create)
	{	//if the toolbar has been recreated, readd the buttons
		size_t nrBtnToAdd = (_state == TB_STANDARD?_nrTotalButtons:_nrButtons);
		_nrCurrentButtons = nrBtnToAdd;
		WORD btnSize = (_state == TB_LARGE?32:16);
		::SendMessage(_hSelf, TB_SETBUTTONSIZE , (WPARAM)0, (LPARAM)MAKELONG (btnSize, btnSize));
		::SendMessage(_hSelf, TB_ADDBUTTONS, (WPARAM)nrBtnToAdd, (LPARAM)_pTBB);
	}
	::SendMessage(_hSelf, TB_AUTOSIZE, 0, 0);

	if (_pRebar)
	{
		_rbBand.hwndChild	= getHSelf();
		_rbBand.cxMinChild	= 0;
		_rbBand.cyIntegral	= 1;
		_rbBand.cyMinChild	= _rbBand.cyMaxChild = getHeight();
		_rbBand.cxIdeal		= getWidth();

		_pRebar->reNew(REBAR_BAR_TOOLBAR, &_rbBand);
	}
}

void ToolBar::registerDynBtn(UINT messageID, toolbarIcons* tIcon)
{
	// Note: Register of buttons only possible before init!
	if ((_hSelf == NULL) && (messageID != 0) && (tIcon->hToolbarBmp != NULL))
	{
		tDynamicList		dynList;
		dynList.message		= messageID;
		dynList.hBmp		= tIcon->hToolbarBmp;
		dynList.hIcon		= tIcon->hToolbarIcon;
		_vDynBtnReg.push_back(dynList);
	}
}

void ToolBar::doPopop(POINT chevPoint) {
	//first find hidden buttons
	int width = Window::getWidth();

	size_t start = 0;
	RECT btnRect = {0,0,0,0};
	while(start < _nrCurrentButtons) {
		::SendMessage(_hSelf, TB_GETITEMRECT, start, (LPARAM)&btnRect);
		if(btnRect.right > width)
			break;
		++start;
	}

	if (start < _nrCurrentButtons) {	//some buttons are hidden
		HMENU menu = ::CreatePopupMenu();
		int cmd;
		generic_string text;
		while (start < _nrCurrentButtons) {
			cmd = _pTBB[start].idCommand;
			getNameStrFromCmd(cmd, text);
			if (_pTBB[start].idCommand != 0) {
				if (::SendMessage(_hSelf, TB_ISBUTTONENABLED, cmd, 0) != 0)
					AppendMenu(menu, MF_ENABLED, cmd, text.c_str());
				else
					AppendMenu(menu, MF_DISABLED|MF_GRAYED, cmd, text.c_str());
			} else
				AppendMenu(menu, MF_SEPARATOR, 0, TEXT(""));
			++start;
		}
		TrackPopupMenu(menu, 0, chevPoint.x, chevPoint.y, 0, _hSelf, NULL);
	}
}

void ToolBar::addToRebar(ReBar * rebar) 
{
	if (_pRebar)
		return;
	_pRebar = rebar;
	ZeroMemory(&_rbBand, REBARBAND_SIZE);
	_rbBand.cbSize  = REBARBAND_SIZE;


	_rbBand.fMask   = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE |
					  RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_ID;

	_rbBand.fStyle		= RBBS_VARIABLEHEIGHT | RBBS_USECHEVRON;
	_rbBand.hwndChild	= getHSelf();
	_rbBand.wID			= REBAR_BAR_TOOLBAR;	//ID REBAR_BAR_TOOLBAR for toolbar
	_rbBand.cxMinChild	= 0;
	_rbBand.cyIntegral	= 1;
	_rbBand.cyMinChild	= _rbBand.cyMaxChild	= getHeight();
	_rbBand.cxIdeal		= _rbBand.cx			= getWidth();

	_pRebar->addBand(&_rbBand, true);

	_rbBand.fMask   = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_SIZE;
}

void ReBar::init(HINSTANCE hInst, HWND hPere)
{
	Window::init(hInst, hPere);
	
	_hSelf = CreateWindowEx(WS_EX_TOOLWINDOW,
							REBARCLASSNAME,
							NULL,
							WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_VARHEIGHT|
							RBS_BANDBORDERS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
							0,0,0,0, _hParent, NULL, _hInst, NULL);

	REBARINFO rbi;
	ZeroMemory(&rbi, sizeof(REBARINFO));
	rbi.cbSize = sizeof(REBARINFO);
	rbi.fMask  = 0;
	rbi.himl   = (HIMAGELIST)NULL;
	::SendMessage(_hSelf, RB_SETBARINFO, 0, (LPARAM)&rbi);
}

bool ReBar::addBand(REBARBANDINFO * rBand, bool useID) 
{
	if (rBand->fMask & RBBIM_STYLE)
	{
		if (!(rBand->fStyle & RBBS_NOGRIPPER))
			rBand->fStyle |= RBBS_GRIPPERALWAYS;
	}
	else
		rBand->fStyle = RBBS_GRIPPERALWAYS;
	rBand->fMask |= RBBIM_ID | RBBIM_STYLE;
	if (useID) {
		if (isIDTaken(rBand->wID))
			return false;

	} else {
		rBand->wID = getNewID();
	}
	::SendMessage(_hSelf, RB_INSERTBAND, (WPARAM)-1, (LPARAM)rBand);	//add to end of list
	return true;
}

void ReBar::reNew(int id, REBARBANDINFO * rBand) 
{
	int index = (int)SendMessage(_hSelf, RB_IDTOINDEX, (WPARAM)id, 0);
	::SendMessage(_hSelf, RB_SETBANDINFO, (WPARAM)index, (LPARAM)rBand);
};

void ReBar::removeBand(int id) 
{
	int index = (int)SendMessage(_hSelf, RB_IDTOINDEX, (WPARAM)id, 0);
	if (id >= REBAR_BAR_EXTERNAL)
		releaseID(id);
	::SendMessage(_hSelf, RB_DELETEBAND, (WPARAM)index, (LPARAM)0);
}

void ReBar::setIDVisible(int id, bool show) 
{
	int index = (int)SendMessage(_hSelf, RB_IDTOINDEX, (WPARAM)id, 0);
	if (index == -1 )
		return;	//error

	REBARBANDINFO rbBand;
	ZeroMemory(&rbBand, REBARBAND_SIZE);
	rbBand.cbSize  = REBARBAND_SIZE;


	rbBand.fMask = RBBIM_STYLE;
	::SendMessage(_hSelf, RB_GETBANDINFO, (WPARAM)index, (LPARAM)&rbBand);
	if (show)
		rbBand.fStyle &= (RBBS_HIDDEN ^ -1);
	else
		rbBand.fStyle |= RBBS_HIDDEN;
	::SendMessage(_hSelf, RB_SETBANDINFO, (WPARAM)index, (LPARAM)&rbBand);
}

bool ReBar::getIDVisible(int id)
{
	int index = (int)SendMessage(_hSelf, RB_IDTOINDEX, (WPARAM)id, 0);
	if (index == -1 )
		return false;	//error
	REBARBANDINFO rbBand;
	ZeroMemory(&rbBand, REBARBAND_SIZE);
	rbBand.cbSize  = REBARBAND_SIZE;

	rbBand.fMask = RBBIM_STYLE;
	::SendMessage(_hSelf, RB_GETBANDINFO, (WPARAM)index, (LPARAM)&rbBand);
	return ((rbBand.fStyle & RBBS_HIDDEN) == 0);
}


void ReBar::setGrayBackground(int id) 
{
	int index = (int)SendMessage(_hSelf, RB_IDTOINDEX, (WPARAM)id, 0);
	if (index == -1 )
		return;	//error
	REBARBANDINFO rbBand;
	ZeroMemory(&rbBand, REBARBAND_SIZE);
	rbBand.cbSize  = REBARBAND_SIZE;
	rbBand.fMask = RBBIM_BACKGROUND;
	rbBand.hbmBack = LoadBitmap((HINSTANCE)::GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_INCREMENTAL_BG));
	::SendMessage(_hSelf, RB_SETBANDINFO, (WPARAM)index, (LPARAM)&rbBand);
}

int ReBar::getNewID()
{
	int idToUse = REBAR_BAR_EXTERNAL;
	int curVal = 0;
	size_t size = usedIDs.size();
	for(size_t i = 0; i < size; ++i)
	{
		curVal = usedIDs.at(i);
		if (curVal < idToUse)
		{
			continue;
		}
		else if (curVal == idToUse)
		{
			++idToUse;
		}
		else
		{
			break;		//found gap
		}
	}

	usedIDs.push_back(idToUse);
	return idToUse;
}

void ReBar::releaseID(int id)
{
	size_t size = usedIDs.size();
	for(size_t i = 0; i < size; ++i)
	{
		if (usedIDs.at(i) == id)
		{
			usedIDs.erase(usedIDs.begin()+i);
			break;
		}
	}
}

bool ReBar::isIDTaken(int id)
{
	size_t size = usedIDs.size();
	for(size_t i = 0; i < size; ++i)
	{
		if (usedIDs.at(i) == id)
		{
			return true;
		}
	}
	return false;
}

