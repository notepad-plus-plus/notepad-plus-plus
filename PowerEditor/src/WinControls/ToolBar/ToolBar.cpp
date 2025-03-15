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

#include <stdexcept>
#include <shlwapi.h>
#include "ToolBar.h"
#include "shortcut.h"
#include "Parameters.h"
#include "FindReplaceDlg_rc.h"
#include "NppDarkMode.h"

using namespace std;

constexpr DWORD WS_TOOLBARSTYLE = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | CCS_TOP | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER;

struct ToolbarIconIdUnit
{
	wstring _id;
	bool hasDisabledIcon = false;
};

ToolbarIconIdUnit toolbarIconIDs[] = {
	{ L"new", false },
	{ L"open", false },
	{ L"save", true },
	{ L"save-all", true },
	{ L"close", false },
	{ L"close-all", false },
	{ L"print", false },
	{ L"cut", true },
	{ L"copy", true },
	{ L"paste", true },
	{ L"undo", true },
	{ L"redo", true },
	{ L"find", false },
	{ L"replace", false },
	{ L"zoom-in", false },
	{ L"zoom-out", false },
	{ L"sync-vertical", false },
	{ L"sync-horizontal", false },
	{ L"word-wrap", false },
	{ L"all-chars", false },
	{ L"indent-guide", false },
	{ L"udl-dlg", false },
	{ L"doc-map", false },
	{ L"doc-list", false },
	{ L"function-list", false },
	{ L"folder-as-workspace", false },
	{ L"monitoring", true },
	{ L"record", true },
	{ L"stop-record", true },
	{ L"playback", true },
	{ L"playback-multiple", true },
	{ L"save-macro", true }
};

void ToolBar::initHideButtonsConf(TiXmlDocument* toolButtonsDocRoot, ToolBarButtonUnit* buttonUnitArray, int arraySize)
{
	TiXmlNode* toolButtons = toolButtonsDocRoot->FirstChild(L"NotepadPlus");
	if (toolButtons)
	{
		toolButtons = toolButtons->FirstChild(L"ToolbarButtons");
		if (toolButtons)
		{
			// Standard toolbar button
			TiXmlNode* standardToolButtons = toolButtons->FirstChild(L"Standard");
			if (standardToolButtons)
			{
				_toolbarStdButtonsConfArray = new bool[arraySize];

				TiXmlElement* stdBtnElement = standardToolButtons->ToElement();
				const wchar_t* isHideAll = stdBtnElement->Attribute(L"hideAll");
				if (isHideAll && (lstrcmp(isHideAll, L"yes") == 0))
				{
					for (int i = 0; i < arraySize; ++i)
						_toolbarStdButtonsConfArray[i] = false;
				}
				else
				{
					for (int i = 0; i < arraySize; ++i)
						_toolbarStdButtonsConfArray[i] = true;

					for (TiXmlNode* childNode = standardToolButtons->FirstChildElement(L"Button");
						childNode;
						childNode = childNode->NextSibling(L"Button"))
					{
						TiXmlElement* element = childNode->ToElement();
						int cmdID = 0;
						const wchar_t* cmdIDStr = element->Attribute(L"id", &cmdID);

						int index = 0;
						const wchar_t* orderStr = element->Attribute(L"index", &index);

						const wchar_t* isHide = element->Attribute(L"hide");

						if (cmdIDStr && orderStr && isHide && (lstrcmp(isHide, L"yes") == 0))
						{
							if (index < arraySize && buttonUnitArray[index]._cmdID == cmdID)
								_toolbarStdButtonsConfArray[index] = false;
						}
					}
				}
			}

			// Plugin toolbar button
			TiXmlNode* pluginToolButtons = toolButtons->FirstChild(L"Plugin");
			if (pluginToolButtons)
			{
				TiXmlElement* pluginBtnElement = pluginToolButtons->ToElement();
				const wchar_t* isHideAll = pluginBtnElement->Attribute(L"hideAll");
				if (isHideAll && (lstrcmp(isHideAll, L"yes") == 0))
				{
					_toolbarPluginButtonsConf._isHideAll = true;
					return;
				}

				for (TiXmlNode* childNode = pluginToolButtons->FirstChildElement(L"Button");
					childNode;
					childNode = childNode->NextSibling(L"Button"))
				{
					bool doShow = true;
					TiXmlElement* element = childNode->ToElement();
					const wchar_t* isHide = element->Attribute(L"hide");

					doShow = !(isHide && (lstrcmp(isHide, L"yes") == 0));
					_toolbarPluginButtonsConf._showPluginButtonsArray.push_back(doShow);
				}
			}
		}
	}
}

void ToolBar::initTheme(TiXmlDocument *toolIconsDocRoot)
{
    _toolIcons =  toolIconsDocRoot->FirstChild(L"NotepadPlus");
	if (_toolIcons)
	{
		_toolIcons = _toolIcons->FirstChild(L"ToolBarIcons");
		if (_toolIcons)
		{
			wstring iconFolderDir = NppParameters::getInstance().getUserPath();
			wstring toolbarIconsRootFolderName = L"toolbarIcons";
			pathAppend(iconFolderDir, toolbarIconsRootFolderName);
			wstring folderName = (_toolIcons->ToElement())->Attribute(L"icoFolderName");
			if (folderName.empty())
				folderName = L"default";

			pathAppend(iconFolderDir, folderName);

			size_t i = 0;
			wstring disabled_suffix = L"_disabled";
			wstring ext = L".ico";
			for (const ToolbarIconIdUnit& icoUnit : toolbarIconIDs)
			{
				wstring locator = iconFolderDir;
				locator += L"\\";
				locator += icoUnit._id;
				locator += ext;
				if (doesFileExist(locator.c_str()))
				{
					_customIconVect.push_back(iconLocator(HLIST_DEFAULT, i, locator));
					_customIconVect.push_back(iconLocator(HLIST_DEFAULT2, i, locator));
					_customIconVect.push_back(iconLocator(HLIST_DEFAULT_DM, i, locator));
					_customIconVect.push_back(iconLocator(HLIST_DEFAULT_DM2, i, locator));
				}

				if (icoUnit.hasDisabledIcon)
				{
					wstring locator_dis = iconFolderDir;
					locator_dis += L"\\";
					locator_dis += icoUnit._id;
					locator_dis += disabled_suffix;
					locator_dis += ext;
					if (doesFileExist(locator_dis.c_str()))
					{
						_customIconVect.push_back(iconLocator(HLIST_DISABLE, i, locator_dis));
						_customIconVect.push_back(iconLocator(HLIST_DISABLE2, i, locator_dis));
						_customIconVect.push_back(iconLocator(HLIST_DISABLE_DM, i, locator_dis));
						_customIconVect.push_back(iconLocator(HLIST_DISABLE_DM2, i, locator_dis));
					}
				}
				i++;
			}
		}
	}
}

bool ToolBar::init( HINSTANCE hInst, HWND hPere, toolBarStatusType type, ToolBarButtonUnit* buttonUnitArray, int arraySize)
{
	Window::init(hInst, hPere);
	
	_state = type;

	_dpiManager.setDpi(hPere);

	int iconSize = _dpiManager.scale(_state == TB_LARGE || _state == TB_LARGE2 ? 32 : 16);

	_toolBarIcons.init(buttonUnitArray, arraySize, _vDynBtnReg);
	_toolBarIcons.create(_hInst, iconSize);
	
	INITCOMMONCONTROLSEX icex{};
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_WIN95_CLASSES|ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_USEREX_CLASSES;
	InitCommonControlsEx(&icex);

	//Create the list of buttons
	_nbButtons = arraySize;
	_nbDynButtons = _vDynBtnReg.size();
	_nbTotalButtons = _nbButtons + (_nbDynButtons ? _nbDynButtons + 1 : 0);
	_pTBB = new TBBUTTON[_nbTotalButtons];	//add one for the extra separator

	int cmd = 0;
	int bmpIndex = -1;
	BYTE style = 0;
	size_t i = 0;

	for (; i < _nbButtons && i < _nbTotalButtons; ++i)
	{
		cmd = buttonUnitArray[i]._cmdID;
		switch (cmd)
		{
			case 0:
			{
				style = BTNS_SEP;
			}
			break;

			case IDM_VIEW_ALL_CHARACTERS:
			{
				++bmpIndex;
				style = BTNS_DROPDOWN;
			}
			break;

			default:
			{
				++bmpIndex;
				style = BTNS_BUTTON;
			}
		}

		_pTBB[i].iBitmap = (cmd != 0 ? bmpIndex : 0);
		_pTBB[i].idCommand = cmd;
		_pTBB[i].fsState = TBSTATE_ENABLED | (_toolbarStdButtonsConfArray ? (_toolbarStdButtonsConfArray[i] ? 0 : TBSTATE_HIDDEN) : 0);
		_pTBB[i].fsStyle = style;
		_pTBB[i].dwData = 0; 
		_pTBB[i].iString = 0;
	}

	bool doHideAllPluginButtons = _toolbarPluginButtonsConf._isHideAll;
	size_t nbPluginButtonsConf = _toolbarPluginButtonsConf._showPluginButtonsArray.size();

	if (_nbDynButtons > 0 && i < _nbTotalButtons)
	{
		unsigned char addedStateFlag = doHideAllPluginButtons ? TBSTATE_HIDDEN : (nbPluginButtonsConf > 0 ? (_toolbarPluginButtonsConf._showPluginButtonsArray[0] ? 0 : TBSTATE_HIDDEN) : 0);

		//add separator
		_pTBB[i].iBitmap = 0;
		_pTBB[i].idCommand = 0;
		_pTBB[i].fsState = TBSTATE_ENABLED | addedStateFlag;
		_pTBB[i].fsStyle = BTNS_SEP;
		_pTBB[i].dwData = 0; 
		_pTBB[i].iString = 0;
		++i;

		//add plugin buttons
		for (size_t j = 0; j < _nbDynButtons && i < _nbTotalButtons; ++j, ++i)
		{
			cmd = _vDynBtnReg[j]._message;
			++bmpIndex;

			addedStateFlag = doHideAllPluginButtons ? TBSTATE_HIDDEN : (nbPluginButtonsConf > j + 1 ? (_toolbarPluginButtonsConf._showPluginButtonsArray[j + 1] ? 0 : TBSTATE_HIDDEN) : 0);

			_pTBB[i].iBitmap = bmpIndex;
			_pTBB[i].idCommand = cmd;
			_pTBB[i].fsState = TBSTATE_ENABLED | addedStateFlag;
			_pTBB[i].fsStyle = BTNS_BUTTON; 
			_pTBB[i].dwData = 0; 
			_pTBB[i].iString = 0;
		}
	}

	reset(true);	//load icons etc

	return true;
}

void ToolBar::destroy()
{
	if (_pRebar)
	{
		_pRebar->removeBand(_rbBand.wID);
		_pRebar = NULL;
	}
	delete [] _pTBB;
	::DestroyWindow(_hSelf);
	_hSelf = NULL;
	_toolBarIcons.destroy();
}

int ToolBar::getWidth() const
{
	RECT btnRect{};
	int totalWidth = 0;
	for (size_t i = 0; i < _nbCurrentButtons; ++i)
	{
		::SendMessage(_hSelf, TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&btnRect));
		totalWidth += btnRect.right - btnRect.left;
	}
	return totalWidth;
}

int ToolBar::getHeight() const
{
	DWORD size = static_cast<DWORD>(SendMessage(_hSelf, TB_GETBUTTONSIZE, 0, 0));
	DWORD padding = static_cast<DWORD>(SendMessage(_hSelf, TB_GETPADDING, 0, 0));
	int totalHeight = HIWORD(size) + HIWORD(padding) - 3;
	return totalHeight;
}

void ToolBar::reduce() 
{
	int iconDpiDynamicalSize = _dpiManager.scale(16);
	_toolBarIcons.resizeIcon(iconDpiDynamicalSize);
	setState(TB_SMALL);
	reset(true);	//recreate toolbar if previous state was Std icons or Big icons
	Window::redraw();
}

void ToolBar::enlarge()
{
	int iconDpiDynamicalSize = _dpiManager.scale(32);
	_toolBarIcons.resizeIcon(iconDpiDynamicalSize);
	setState(TB_LARGE);
	reset(true);	//recreate toolbar if previous state was Std icons or Small icons
	Window::redraw();
}

void ToolBar::reduceToSet2()
{
	int iconDpiDynamicalSize = _dpiManager.scale(16);
	_toolBarIcons.resizeIcon(iconDpiDynamicalSize);

	setState(TB_SMALL2);
	reset(true);
	Window::redraw();
}

void ToolBar::enlargeToSet2()
{
	int iconDpiDynamicalSize = _dpiManager.scale(32);
	_toolBarIcons.resizeIcon(iconDpiDynamicalSize);
	setState(TB_LARGE2);
	reset(true);	//recreate toolbar if previous state was Std icons or Small icons
	Window::redraw();
}

void ToolBar::setToBmpIcons()
{
	bool recreate = true;
	setState(TB_STANDARD);
	reset(recreate);	//must recreate toolbar if setting to internal bitmaps
	Window::redraw();
}

void ToolBar::reset(bool create)
{
	if (create && _hSelf)
	{
		//Store current button state information
		TBBUTTON tempBtn{};
		for (size_t i = 0; i < _nbCurrentButtons; ++i)
		{
			::SendMessage(_hSelf, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tempBtn));
			_pTBB[i].fsState = tempBtn.fsState;
		}
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
	}

	if (!_hSelf)
	{
		DWORD dwExtraStyle = 0;
		if (NppDarkMode::isEnabled())
		{
			dwExtraStyle = TBSTYLE_CUSTOMERASE;
		}

		_hSelf = ::CreateWindowEx(
			WS_EX_PALETTEWINDOW,
			TOOLBARCLASSNAME,
			L"",
			WS_TOOLBARSTYLE | dwExtraStyle,
			0, 0,
			0, 0,
			_hParent,
			NULL,
			_hInst,
			0);

		NppDarkMode::setDarkTooltips(_hSelf, NppDarkMode::ToolTipsType::toolbar);

		// Send the TB_BUTTONSTRUCTSIZE message, which is required for 
		// backward compatibility.
		::SendMessage(_hSelf, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
		::SendMessage(_hSelf, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER);
		
		change2CustomIconsIfAny();
	}

	if (!_hSelf)
	{
		throw std::runtime_error("ToolBar::reset : CreateWindowEx() function return null");
	}

	bool doOverrideToolbarIcons = _customIconVect.size() > 0;
	if (doOverrideToolbarIcons)
	{
		if (_state == TB_STANDARD)
			_state = TB_SMALL;
	}

	if (_state != TB_STANDARD) //If non standard icons, use custom imagelists
	{
		if (_state == TB_SMALL || _state == TB_LARGE)
		{
			if (NppDarkMode::isEnabled())
			{
				setDefaultImageListDM();
				setDisableImageListDM();
			}
			else
			{
				setDefaultImageList();
				setDisableImageList();
			}
		}
		else
		{
			if (NppDarkMode::isEnabled())
			{
				setDefaultImageListDM2();
				setDisableImageListDM2();
			}
			else
			{
				setDefaultImageList2();
				setDisableImageList2();
			}
		}
	}
	else
	{
		//Else set the internal imagelist with standard bitmaps
		int iconDpiDynamicalSize = _dpiManager.scale(16);
		::SendMessage(_hSelf, TB_SETBITMAPSIZE, 0, MAKELPARAM(iconDpiDynamicalSize, iconDpiDynamicalSize));

		TBADDBITMAP addbmp = { 0, 0 };
		TBADDBITMAP addbmpdyn = { 0, 0 };
		for (size_t i = 0; i < _nbButtons; ++i)
		{
			int icoID = _toolBarIcons.getStdIconAt(static_cast<int32_t>(i));
			HBITMAP hBmp = static_cast<HBITMAP>(::LoadImage(_hInst, MAKEINTRESOURCE(icoID), IMAGE_BITMAP, iconDpiDynamicalSize, iconDpiDynamicalSize, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT));
			addbmp.nID = reinterpret_cast<UINT_PTR>(hBmp);
			::SendMessage(_hSelf, TB_ADDBITMAP, 1, reinterpret_cast<LPARAM>(&addbmp));
		}

		if (_nbDynButtons > 0)
		{
			for (size_t j = 0; j < _nbDynButtons; ++j)
			{
				addbmpdyn.nID = reinterpret_cast<UINT_PTR>(_vDynBtnReg.at(j)._hBmp);
				::SendMessage(_hSelf, TB_ADDBITMAP, 1, reinterpret_cast<LPARAM>(&addbmpdyn));
			}
		}
	}

	if (create)
	{	//if the toolbar has been recreated, readd the buttons
		_nbCurrentButtons = _nbTotalButtons;
		WORD btnSize = static_cast<WORD>(_dpiManager.scale((_state == TB_LARGE || _state == TB_LARGE2) ? 32 : 16));
		::SendMessage(_hSelf, TB_SETBUTTONSIZE , 0, MAKELONG(btnSize, btnSize));
		::SendMessage(_hSelf, TB_ADDBUTTONS, _nbTotalButtons, reinterpret_cast<LPARAM>(_pTBB));
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

void ToolBar::registerDynBtn(UINT messageID, toolbarIcons* iconHandles, HICON absentIco)
{
	// Note: Register of buttons only possible before init!
	if ((_hSelf == NULL) && (messageID != 0) && (iconHandles->hToolbarBmp != NULL))
	{
		DynamicCmdIcoBmp dynList{};
		dynList._message = messageID;
		dynList._hBmp = iconHandles->hToolbarBmp;

		if (iconHandles->hToolbarIcon)
		{
			dynList._hIcon = iconHandles->hToolbarIcon;
		}
		else
		{
			BITMAP bmp{};
			int nbByteBmp = ::GetObject(dynList._hBmp, sizeof(BITMAP), &bmp);
			if (!nbByteBmp)
			{
				dynList._hIcon = absentIco;
			}
			else
			{
				HBITMAP hbmMask = ::CreateCompatibleBitmap(::GetDC(NULL), bmp.bmWidth, bmp.bmHeight);

				ICONINFO iconinfoDest = {};
				iconinfoDest.fIcon = TRUE;
				iconinfoDest.hbmColor = iconHandles->hToolbarBmp;
				iconinfoDest.hbmMask = hbmMask;

				dynList._hIcon = ::CreateIconIndirect(&iconinfoDest);
				::DeleteObject(hbmMask);
			}
		}

		dynList._hIcon_DM = dynList._hIcon;

		_vDynBtnReg.push_back(dynList);
	}
}

void ToolBar::registerDynBtnDM(UINT messageID, toolbarIconsWithDarkMode* iconHandles)
{
	// Note: Register of buttons only possible before init!
	if ((_hSelf == NULL) && (messageID != 0) && (iconHandles->hToolbarBmp != NULL) && 
		(iconHandles->hToolbarIcon != NULL) && (iconHandles->hToolbarIconDarkMode != NULL))
	{
		DynamicCmdIcoBmp dynList{};
		dynList._message = messageID;
		dynList._hBmp = iconHandles->hToolbarBmp;
		dynList._hIcon = iconHandles->hToolbarIcon;
		dynList._hIcon_DM = iconHandles->hToolbarIconDarkMode;
		_vDynBtnReg.push_back(dynList);
	}
}

void ToolBar::doPopop(POINT chevPoint)
{
	//first find hidden buttons
	int width = Window::getWidth();

	size_t start = 0;
	RECT btnRect = {0,0,0,0};
	while (start < _nbCurrentButtons)
	{
		::SendMessage(_hSelf, TB_GETITEMRECT, start, reinterpret_cast<LPARAM>(&btnRect));
		if (btnRect.right > width)
			break;
		++start;
	}

	if (start < _nbCurrentButtons)
	{	//some buttons are hidden
		HMENU menu = ::CreatePopupMenu();
		wstring text;
		while (start < _nbCurrentButtons)
		{
			int cmd = _pTBB[start].idCommand;
			getNameStrFromCmd(cmd, text);
			if (_pTBB[start].idCommand != 0)
			{
				if (::SendMessage(_hSelf, TB_ISBUTTONENABLED, cmd, 0) != 0)
					AppendMenu(menu, MF_ENABLED, cmd, text.c_str());
				else
					AppendMenu(menu, MF_DISABLED|MF_GRAYED, cmd, text.c_str());
			} else
				AppendMenu(menu, MF_SEPARATOR, 0, L"");
			
			++start;
		}
		TrackPopupMenu(menu, 0, chevPoint.x, chevPoint.y, 0, _hSelf, NULL);
	}
}

void ToolBar::resizeIconsDpi(UINT dpi)
{
	_dpiManager.setDpi(dpi);
	const int iconDpiDynamicalSize = _dpiManager.scale((_state == TB_LARGE || _state == TB_LARGE2) ? 32 : 16);
	_toolBarIcons.resizeIcon(iconDpiDynamicalSize);
	reset(true);
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

	_rbBand.fStyle		= RBBS_VARIABLEHEIGHT | RBBS_USECHEVRON | RBBS_NOGRIPPER;
	_rbBand.hwndChild	= getHSelf();
	_rbBand.wID			= REBAR_BAR_TOOLBAR;	//ID REBAR_BAR_TOOLBAR for toolbar
	_rbBand.cxMinChild	= 0;
	_rbBand.cyIntegral	= 1;
	_rbBand.cyMinChild	= _rbBand.cyMaxChild	= getHeight();
	_rbBand.cxIdeal		= _rbBand.cx			= getWidth();

	_pRebar->addBand(&_rbBand, true);

	_rbBand.fMask   = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_SIZE;
}

constexpr UINT_PTR g_rebarSubclassID = 42;

LRESULT CALLBACK RebarSubclass(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	UNREFERENCED_PARAMETER(dwRefData);
	UNREFERENCED_PARAMETER(uIdSubclass);

	switch (uMsg)
	{
		case WM_ERASEBKGND:
			if (NppDarkMode::isEnabled())
			{
				RECT rc{};
				GetClientRect(hWnd, &rc);
				FillRect((HDC)wParam, &rc, NppDarkMode::getDarkerBackgroundBrush());
				return TRUE;
			}
			else
			{
				break;
			}

		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, RebarSubclass, g_rebarSubclassID);
			break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void ReBar::init(HINSTANCE hInst, HWND hPere)
{
	Window::init(hInst, hPere);
	_hSelf = CreateWindowEx(WS_EX_TOOLWINDOW,
							REBARCLASSNAME,
							NULL,
							WS_CHILD|WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | CCS_NODIVIDER | CCS_NOPARENTALIGN,
							0,0,0,0, _hParent, NULL, _hInst, NULL);

	SetWindowSubclass(_hSelf, RebarSubclass, g_rebarSubclassID, 0);

	REBARINFO rbi{};
	ZeroMemory(&rbi, sizeof(REBARINFO));
	rbi.cbSize = sizeof(REBARINFO);
	rbi.fMask  = 0;
	rbi.himl   = (HIMAGELIST)NULL;
	::SendMessage(_hSelf, RB_SETBARINFO, 0, reinterpret_cast<LPARAM>(&rbi));
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
	if (useID)
	{
		if (isIDTaken(rBand->wID))
			return false;
	}
	else
	{
		rBand->wID = getNewID();
	}
	::SendMessage(_hSelf, RB_INSERTBAND, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(rBand));	//add to end of list
	return true;
}

void ReBar::reNew(int id, REBARBANDINFO * rBand) 
{
	auto index = SendMessage(_hSelf, RB_IDTOINDEX, id, 0);
	::SendMessage(_hSelf, RB_SETBANDINFO, index, reinterpret_cast<LPARAM>(rBand));
}

void ReBar::removeBand(int id) 
{
	auto index = SendMessage(_hSelf, RB_IDTOINDEX, id, 0);
	if (id >= REBAR_BAR_EXTERNAL)
		releaseID(id);
	::SendMessage(_hSelf, RB_DELETEBAND, index, 0);
}

void ReBar::setIDVisible(int id, bool show) 
{
	auto index = SendMessage(_hSelf, RB_IDTOINDEX, id, 0);
	if (index == -1 )
		return;	//error

	REBARBANDINFO rbBand;
	ZeroMemory(&rbBand, REBARBAND_SIZE);
	rbBand.cbSize  = REBARBAND_SIZE;


	rbBand.fMask = RBBIM_STYLE;
	::SendMessage(_hSelf, RB_GETBANDINFO, index, reinterpret_cast<LPARAM>(&rbBand));
	if (show)
		rbBand.fStyle &= (RBBS_HIDDEN ^ -1);
	else
		rbBand.fStyle |= RBBS_HIDDEN;
	::SendMessage(_hSelf, RB_SETBANDINFO, index, reinterpret_cast<LPARAM>(&rbBand));
}

bool ReBar::getIDVisible(int id)
{
	auto index = SendMessage(_hSelf, RB_IDTOINDEX, id, 0);
	if (index == -1 )
		return false;	//error
	REBARBANDINFO rbBand;
	ZeroMemory(&rbBand, REBARBAND_SIZE);
	rbBand.cbSize  = REBARBAND_SIZE;

	rbBand.fMask = RBBIM_STYLE;
	::SendMessage(_hSelf, RB_GETBANDINFO, index, reinterpret_cast<LPARAM>(&rbBand));
	return ((rbBand.fStyle & RBBS_HIDDEN) == 0);
}


void ReBar::setGrayBackground(int id) 
{
	auto index = SendMessage(_hSelf, RB_IDTOINDEX, id, 0);
	if (index == -1 )
		return;	//error
	REBARBANDINFO rbBand;
	ZeroMemory(&rbBand, REBARBAND_SIZE);
	rbBand.cbSize  = REBARBAND_SIZE;
	rbBand.fMask = RBBIM_BACKGROUND;
	rbBand.hbmBack = LoadBitmap((HINSTANCE)::GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_INCREMENTAL_BG));
	::SendMessage(_hSelf, RB_SETBANDINFO, index, reinterpret_cast<LPARAM>(&rbBand));
}

int ReBar::getNewID()
{
	int idToUse = REBAR_BAR_EXTERNAL;
	size_t size = usedIDs.size();
	for (size_t i = 0; i < size; ++i)
	{
		int curVal = usedIDs.at(i);
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
	for (size_t i = 0; i < size; ++i)
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
	for (size_t i = 0; i < size; ++i)
	{
		if (usedIDs.at(i) == id)
		{
			return true;
		}
	}
	return false;
}
