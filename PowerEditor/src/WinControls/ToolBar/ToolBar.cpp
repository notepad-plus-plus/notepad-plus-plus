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

//#include "..\..\resource.h"
#include "ToolBar.h"
#include "SysMsg.h"

const bool ToolBar::REDUCED = true;
const bool ToolBar::ENLARGED = false;
const int WS_TOOLBARSTYLE = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS |TBSTYLE_FLAT | CCS_TOP | BTNS_AUTOSIZE;

bool ToolBar::init(HINSTANCE hInst, HWND hPere, int iconSize, 
				   ToolBarButtonUnit *buttonUnitArray, int arraySize,
				   bool doUglyStandardIcon, int *bmpArray, int bmpArraySize)
{
	Window::init(hInst, hPere);
	_state = doUglyStandardIcon?TB_STANDARD:(iconSize >= 32?TB_LARGE:TB_SMALL);
	_bmpArray = bmpArray;
	_bmpArraySize = bmpArraySize;

	_toolBarIcons.init(buttonUnitArray, arraySize);
	_toolBarIcons.create(_hInst, iconSize);
	
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_WIN95_CLASSES|ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_USEREX_CLASSES;
	InitCommonControlsEx(&icex);

	_hSelf = ::CreateWindowEx(
	               WS_EX_PALETTEWINDOW ,
	               TOOLBARCLASSNAME,
	               "",
	               WS_TOOLBARSTYLE,
	               0, 0,
	               0, 0,
	               _hParent,
				   NULL,
	               _hInst,
	               0);

	if (!_hSelf)
	{
		systemMessage("System Err");
		throw int(9);
	}

	// Send the TB_BUTTONSTRUCTSIZE message, which is required for 
	// backward compatibility.
	::SendMessage(_hSelf, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	
	if (!doUglyStandardIcon)
	{
		setDefaultImageList();
		setHotImageList();
		setDisableImageList();
	}
	else
	{
		::SendMessage(_hSelf, TB_LOADIMAGES, IDB_STD_SMALL_COLOR, reinterpret_cast<LPARAM>(HINST_COMMCTRL));

		if (bmpArray)
		{
			TBADDBITMAP addbmp = {_hInst, 0};
			for (int i = 0 ; i < _bmpArraySize ; i++)
			{
				if ((i == _bmpArraySize - 1) && (_vDynBtnReg.size() != 0))
				{
					TBADDBITMAP addbmpdyn = {0, 0};
					for (size_t j = 0; j < _vDynBtnReg.size(); j++)
					{
						addbmpdyn.nID = (INT_PTR)_vDynBtnReg[j].hBmp;
						::SendMessage(_hSelf, TB_ADDBITMAP, 1, (LPARAM)&addbmpdyn);
					}
				}

				addbmp.nID = _bmpArray[i];
				::SendMessage(_hSelf, TB_ADDBITMAP, 1, (LPARAM)&addbmp);
			}
		}
		
	}

	_pTBB = new TBBUTTON[_toolBarIcons.getNbCommand() + (_vDynBtnReg.size() ? _vDynBtnReg.size() + 1 : 0)];
	unsigned int nbElement    = _toolBarIcons.getNbCommand();
	unsigned int nbDynIncPos	 = nbElement;

	if (doUglyStandardIcon)
	{
		nbDynIncPos -= 2;
		nbElement   += (_vDynBtnReg.size() ? _vDynBtnReg.size() + 1 : 0);
	}

	int inc = 1;

	for (size_t i = 0, j = 0, k = 0; i < nbElement; i++)
	{
		int cmd = 0;
		int bmpIndex, style;

		if ((i > nbDynIncPos) && (_vDynBtnReg.size() != 0) && (j <= _vDynBtnReg.size()))
		{
			if (j < _vDynBtnReg.size())
			{
				cmd = _vDynBtnReg[j].message;
				bmpIndex = (STD_PRINT + (inc++));
				style = BTNS_BUTTON;
			}
			else
			{
				bmpIndex = 0;
				style = BTNS_SEP;
			}
			j++;
		}
		else
		{
			if ((cmd = _toolBarIcons.getCommandAt(i - j)) != 0)
			{
				if (doUglyStandardIcon)
				{
					int ibmp = _toolBarIcons.getUglyIconAt(i - j);
					bmpIndex = (ibmp == -1)?(STD_PRINT + (inc++)):ibmp;
				}
				else
					bmpIndex = k++;

				style = BTNS_BUTTON;
			}
			else
			{
				bmpIndex = 0;
				style = BTNS_SEP;
			}
		}
		_pTBB[i].iBitmap = bmpIndex;
		_pTBB[i].idCommand = cmd;
		_pTBB[i].fsState = TBSTATE_ENABLED;
		_pTBB[i].fsStyle = style; 
		_pTBB[i].dwData = 0; 
		_pTBB[i].iString = 0;

	}

	setButtonSize(iconSize, iconSize);

	::SendMessage(_hSelf, TB_ADDBUTTONS, (WPARAM)nbElement, (LPARAM)_pTBB); 
	::SendMessage(_hSelf, TB_AUTOSIZE, 0, 0);

	return true;
}

void ToolBar::reset() 
{
	setDefaultImageList();
	setHotImageList();
	setDisableImageList();

	if (_state == TB_STANDARD)
	{
		int cmdElement = _toolBarIcons.getNbCommand();
		int nbElement  = cmdElement + (_vDynBtnReg.size() ? _vDynBtnReg.size() + 1 : 0);

		for (int i = 0, j = 0, k = nbElement-1 ; i < nbElement ; i++, k--)
		{
			int cmd = 0;
			int bmpIndex, style;

			::SendMessage(_hSelf, TB_DELETEBUTTON, k, 0);

			if (i >= cmdElement)
			{
				bmpIndex = -1;
				cmd			 = 0;
				style		 = 0;
			}
			else if ((cmd = _toolBarIcons.getCommandAt(i)) != 0)
			{
				bmpIndex = j++;
				style = BTNS_BUTTON;
			}
			else
			{
				bmpIndex = 0;
				style = BTNS_SEP;
			}
			_pTBB[i].iBitmap = bmpIndex;
			_pTBB[i].idCommand = cmd;
			_pTBB[i].fsState = TBSTATE_ENABLED;
			_pTBB[i].fsStyle = style; 
			_pTBB[i].dwData = 0; 
			_pTBB[i].iString = 0;
		}

		::SendMessage(_hSelf, TB_ADDBUTTONS, (WPARAM)nbElement, (LPARAM)_pTBB); 
	}

	::SendMessage(_hSelf, TB_AUTOSIZE, 0, 0);
}

void ToolBar::setToUglyIcons() 
{
	if (_state == TB_STANDARD) 
		return;

	// Due to the drawback of toolbar control (in-coexistence of Imagelist - custom icons and Bitmap - Std icons),
	// We have to destroy the control then re-initialize it
	::DestroyWindow(_hSelf);

	//_state = REDUCED;

	_hSelf = ::CreateWindowEx(
	               WS_EX_PALETTEWINDOW ,
	               TOOLBARCLASSNAME,
	               "",
	               WS_TOOLBARSTYLE,
	               0, 0,
	               0, 0,
	               _hParent,
				   NULL,
	               _hInst,
	               0);

	if (!_hSelf)
	{
		systemMessage("System Err");
		throw int(9);
	}

	// Send the TB_BUTTONSTRUCTSIZE message, which is required for 
	// backward compatibility.
	::SendMessage(_hSelf, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	::SendMessage(_hSelf, TB_LOADIMAGES, IDB_STD_SMALL_COLOR, reinterpret_cast<LPARAM>(HINST_COMMCTRL));

	if (_bmpArray)
	{
		TBADDBITMAP addbmp = {_hInst, 0};
		for (int i = 0 ; i < _bmpArraySize ; i++)
		{
			if ((i == _bmpArraySize - 1) && (_vDynBtnReg.size() != 0))
			{
				TBADDBITMAP addbmpdyn = {0, 0};
				for (size_t j = 0; j < _vDynBtnReg.size(); j++)
				{
					addbmpdyn.nID = (INT_PTR)_vDynBtnReg[j].hBmp;
					::SendMessage(_hSelf, TB_ADDBITMAP, 1, (LPARAM)&addbmpdyn);
				}
			}

			addbmp.nID = _bmpArray[i];
			::SendMessage(_hSelf, TB_ADDBITMAP, 1, (LPARAM)&addbmp);
		}
	}

	unsigned int nbElement    = _toolBarIcons.getNbCommand() + (_vDynBtnReg.size() ? _vDynBtnReg.size() + 1 : 0);
	unsigned int nbDynIncPos	 = _toolBarIcons.getNbCommand() - 2;
	int inc = 1;

	for (size_t i = 0, j = 0 ; i < nbElement ; i++)
	{
		int cmd = 0;
		int bmpIndex, style;

		if ((i > nbDynIncPos) && (_vDynBtnReg.size() != 0) && (j <= _vDynBtnReg.size()))
		{
			if (j < _vDynBtnReg.size())
			{
				cmd = _vDynBtnReg[j].message;
				bmpIndex = (STD_PRINT + (inc++));
				style = BTNS_BUTTON;
			}
			else
			{
				bmpIndex = 0;
				style = BTNS_SEP;
			}
			j++;
		}
		else
		{
			if ((cmd = _toolBarIcons.getCommandAt(i - j)) != 0)
			{
				int ibmp = _toolBarIcons.getUglyIconAt(i - j);
				bmpIndex = (ibmp == -1)?(STD_PRINT + (inc++)):ibmp;
				style = BTNS_BUTTON;
			}
			else
			{
				bmpIndex = 0;
				style = BTNS_SEP;
			}
		}
		_pTBB[i].iBitmap = bmpIndex;
		_pTBB[i].idCommand = cmd;
		_pTBB[i].fsState = TBSTATE_ENABLED;
		_pTBB[i].fsStyle = style; 
		_pTBB[i].dwData = 0; 
		_pTBB[i].iString = 0;

	}

	setButtonSize(16, 16);

	::SendMessage(_hSelf, TB_ADDBUTTONS, (WPARAM)nbElement, (LPARAM)_pTBB); 
	::SendMessage(_hSelf, TB_AUTOSIZE, 0, 0);
	_state = TB_STANDARD;
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

void ReBar::init(HINSTANCE hInst, HWND hPere, ToolBar *pToolBar)
{
	Window::init(hInst, hPere);
	_pToolBar = pToolBar;
	_hSelf = CreateWindowEx(WS_EX_TOOLWINDOW,
							REBARCLASSNAME,
							NULL,
							WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
							WS_CLIPCHILDREN|RBS_VARHEIGHT|
							CCS_NODIVIDER,
							0,0,0,0, _hParent, NULL, _hInst, NULL);


	::SendMessage(_hSelf, RB_SETBARINFO, 0, (LPARAM)&_rbi);


	_rbBand.hwndChild  = _pToolBar->getHSelf();

	int dwBtnSize = SendMessage(_pToolBar->getHSelf(), TB_GETBUTTONSIZE, 0,0);

	_rbBand.cxMinChild = 34;//nbElement;
	_rbBand.cyMinChild = HIWORD(dwBtnSize);
	_rbBand.cx         = 250;
	::SendMessage(_hSelf, RB_INSERTBAND, (WPARAM)0, (LPARAM)&_rbBand);
}

