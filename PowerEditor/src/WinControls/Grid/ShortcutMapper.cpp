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


#include "ShortcutMapper.h"
#include "Notepad_plus.h"


using namespace std;

void ShortcutMapper::initTabs()
{
	HWND hTab = _hTabCtrl = ::GetDlgItem(_hSelf, IDC_BABYGRID_TABBAR);
	NppDarkMode::subclassTabControl(hTab);
	TCITEM tie;
	tie.mask = TCIF_TEXT;

	for (size_t i = 0; i < _nbTab; ++i)
	{
		_tabNames[i] = getTabString(i);

		tie.pszText = const_cast<LPWSTR>(_tabNames[i].c_str());
		::SendMessage(hTab, TCM_INSERTITEM, i, reinterpret_cast<LPARAM>(&tie));
	}

    TabCtrl_SetCurSel(_hTabCtrl, int(_currentState));

	// force alignment to babygrid
	RECT rcTab;
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	::GetWindowPlacement(hTab, &wp);
	::SendMessage(hTab, TCM_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&rcTab));

	wp.rcNormalPosition.bottom = NppParameters::getInstance()._dpiManager.scaleY(30);
	wp.rcNormalPosition.top = wp.rcNormalPosition.bottom - rcTab.bottom;

	::SetWindowPlacement(hTab, &wp);
}

void ShortcutMapper::getClientRect(RECT & rc) const 
{
		Window::getClientRect(rc);

		RECT tabRect, btnRect;
		::GetClientRect(::GetDlgItem(_hSelf, IDC_BABYGRID_TABBAR), &tabRect);
		int tabH = tabRect.bottom - tabRect.top;
		int paddingTop = tabH / 2;
		rc.top += tabH + paddingTop;

		RECT infoRect, filterRect;
		::GetClientRect(::GetDlgItem(_hSelf, IDC_BABYGRID_INFO), &infoRect);
		::GetClientRect(::GetDlgItem(_hSelf, IDC_BABYGRID_FILTER), &filterRect);
		::GetClientRect(::GetDlgItem(_hSelf, IDOK), &btnRect);
		int infoH = infoRect.bottom - infoRect.top;
		int filterH = filterRect.bottom - filterRect.top;
		int btnH = btnRect.bottom - btnRect.top;
		int paddingBottom = btnH + NppParameters::getInstance()._dpiManager.scaleY(16);
		rc.bottom -= btnH + filterH + infoH + paddingBottom;

		rc.left += NppParameters::getInstance()._dpiManager.scaleX(5);
		rc.right -= NppParameters::getInstance()._dpiManager.scaleX(5);
}

generic_string ShortcutMapper::getTabString(size_t i) const
{
	if (i >= _nbTab)
		return TEXT("");

	NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
	switch (i)
	{
		case 1:
			return nativeLangSpeaker->getShortcutMapperLangStr("MacrosTab", TEXT("Macros"));

		case 2:
			return nativeLangSpeaker->getShortcutMapperLangStr("RunCommandsTab", TEXT("Run commands"));

		case 3:
			return nativeLangSpeaker->getShortcutMapperLangStr("PluginCommandsTab", TEXT("Plugin commands"));

		case 4:
			return nativeLangSpeaker->getShortcutMapperLangStr("ScintillaCommandsTab", TEXT("Scintilla commands"));

		default: //0
			return nativeLangSpeaker->getShortcutMapperLangStr("MainMenuTab", TEXT("Main menu"));
	}
}


void ShortcutMapper::initBabyGrid()
{
	RECT rect;
	getClientRect(rect);

	_lastHomeRow.resize(5, 1);
	_lastCursorRow.resize(5, 1);

	_hGridFonts.resize(MAX_GRID_FONTS);
	_hGridFonts.at(GFONT_HEADER) = ::CreateFont(
		NppParameters::getInstance()._dpiManager.scaleY(18), 0, 0, 0, FW_BOLD,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		TEXT("MS Shell Dlg"));
	_hGridFonts.at(GFONT_ROWS) = ::CreateFont(
		NppParameters::getInstance()._dpiManager.scaleY(16), 0, 0, 0, FW_NORMAL,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		TEXT("MS Shell Dlg"));
	
	_babygrid.init(_hInst, _hSelf, IDD_BABYGRID_ID1);

	NppDarkMode::setDarkScrollBar(_babygrid.getHSelf());

	_babygrid.setHeaderFont(_hGridFonts.at(GFONT_HEADER));
	_babygrid.setRowFont(_hGridFonts.at(GFONT_ROWS));
	
	_babygrid.reSizeToWH(rect);
	_babygrid.hideCursor();
	_babygrid.makeColAutoWidth(true);
	_babygrid.setAutoRow(true);
	_babygrid.setColsNumbered(false);
	_babygrid.setColWidth(0, NppParameters::getInstance()._dpiManager.scaleX(30));  // Force the first col to be small, others col will be automatically sized
	_babygrid.setHeaderHeight(NppParameters::getInstance()._dpiManager.scaleY(21));
	_babygrid.setRowHeight(NppParameters::getInstance()._dpiManager.scaleY(21));

	if (NppDarkMode::isEnabled())
	{
		_babygrid.setTextColor(NppDarkMode::getDarkerTextColor());
		_babygrid.setHighlightTextColor(NppDarkMode::getTextColor());
		_babygrid.setTitleTextColor(NppDarkMode::getTextColor());

		_babygrid.setUnprotectColor(NppDarkMode::getBackgroundColor());
		_babygrid.setTitleColor(NppDarkMode::getBackgroundColor());

		_babygrid.setBackgroundColor(NppDarkMode::getDarkerBackgroundColor());

		_babygrid.setHighlightColor(NppDarkMode::getHotBackgroundColor());
		_babygrid.setHighlightColorNoFocus(NppDarkMode::getSofterBackgroundColor());
		_babygrid.setProtectColor(NppDarkMode::getErrorBackgroundColor());
		_babygrid.setHighlightColorProtect(RGB(244, 10, 20));
		_babygrid.setHighlightColorProtectNoFocus(RGB(230, 100, 110));

		_babygrid.setGridlinesColor(NppDarkMode::getEdgeColor());
		_babygrid.setTitleGridlinesColor(NppDarkMode::getHotEdgeColor());
	}
	else
	{
		_babygrid.setTextColor(RGB(0, 0, 0));
		_babygrid.setHighlightTextColor(RGB(255, 255, 255));
		_babygrid.setTitleTextColor(RGB(0, 0, 0));

		_babygrid.setUnprotectColor(RGB(255, 255, 255));
		_babygrid.setTitleColor(::GetSysColor(COLOR_BTNFACE));

		_babygrid.setBackgroundColor(::GetSysColor(COLOR_BTNFACE));

		_babygrid.setHighlightColor(RGB(0, 0, 128));
		_babygrid.setHighlightColorNoFocus(RGB(200, 200, 210));
		_babygrid.setProtectColor(RGB(255, 130, 120));
		_babygrid.setHighlightColorProtect(RGB(244, 10, 20));
		_babygrid.setHighlightColorProtectNoFocus(RGB(230, 194, 190));

		_babygrid.setGridlinesColor(RGB(220, 220, 220));
		_babygrid.setTitleGridlinesColor(RGB(120, 120, 120));
	}

	NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
	nativeLangSpeaker->changeDlgLang(_hSelf, "ShortcutMapper");
	_conflictInfoOk = nativeLangSpeaker->getShortcutMapperLangStr("ConflictInfoOk", TEXT("No shortcut conflicts for this item."));
	_conflictInfoEditing = nativeLangSpeaker->getShortcutMapperLangStr("ConflictInfoEditing", TEXT("No conflicts . . ."));
}

generic_string ShortcutMapper::getTextFromCombo(HWND hCombo)
{
	const int NB_MAX(128);
	TCHAR str[NB_MAX](TEXT("\0"));
	::SendMessage(hCombo, WM_GETTEXT, NB_MAX, reinterpret_cast<LPARAM>(str));
	generic_string res(str);
	return stringToLower(res);
};

bool ShortcutMapper::isFilterValid(Shortcut sc)
{
	if (_shortcutFilter.empty())
		return true;

	generic_string shortcut_name = stringToLower(generic_string(sc.getName()));
	generic_string shortcut_value = stringToLower(sc.toString());

	// test the filter on the shortcut name and value
	return (shortcut_name.find(_shortcutFilter) != std::string::npos) || 
		(shortcut_value.find(_shortcutFilter) != std::string::npos);
}

bool ShortcutMapper::isFilterValid(PluginCmdShortcut sc)
{
	// Do like a classic search on shortcut name, then search on the plugin name.
	Shortcut shortcut = sc;
	bool match = false;
	generic_string module_name = stringToLower(generic_string(sc.getModuleName()));
	if (isFilterValid(shortcut)){
		return true;
	}
	size_t match_pos = module_name.find(_shortcutFilter);
	if (match_pos != std::string::npos){
		match = true;
	}

	return match;
}

void ShortcutMapper::fillOutBabyGrid()
{
	NppParameters& nppParam = NppParameters::getInstance();
	_babygrid.clear();
	_babygrid.setInitialContent(true);
	_shortcutIndex.clear();

	size_t nbItems = 0;
	NativeLangSpeaker* nativeLangSpeaker = nppParam.getNativeLangSpeaker();
	generic_string nameStr = nativeLangSpeaker->getShortcutMapperLangStr("ColumnName", TEXT("Name"));
	generic_string shortcutStr = nativeLangSpeaker->getShortcutMapperLangStr("ColumnShortcut", TEXT("Shortcut"));

	_babygrid.setText(0, 1, nameStr.c_str());
	_babygrid.setText(0, 2, shortcutStr.c_str());
	
	switch(_currentState)
	{
		case STATE_MENU:
		{
			nbItems = nppParam.getUserShortcuts().size();
			_babygrid.setLineColNumber(nbItems, 3);
			generic_string categoryStr = nativeLangSpeaker->getShortcutMapperLangStr("ColumnCategory", TEXT("Category"));
			_babygrid.setText(0, 3, categoryStr.c_str());
		}
		break;

		case STATE_MACRO:
		{
			nbItems = nppParam.getMacroList().size();
			_babygrid.setLineColNumber(nbItems, 2);
		}
		break;

		case STATE_USER:
		{
			nbItems = nppParam.getUserCommandList().size();
			_babygrid.setLineColNumber(nbItems, 2);
		}
		break;

		case STATE_PLUGIN:
		{
			nbItems = nppParam.getPluginCommandList().size();
			_babygrid.setLineColNumber(nbItems, 3);
			generic_string pluginStr = nativeLangSpeaker->getShortcutMapperLangStr("ColumnPlugin", TEXT("Plugin"));
			_babygrid.setText(0, 3, pluginStr.c_str());
		}
		break;

		case STATE_SCINTILLA:
		{
			nbItems = nppParam.getScintillaKeyList().size();
			_babygrid.setLineColNumber(nbItems, 2);
		}
		break;
	}

	bool isMarker = false;
	size_t cs_index = 0;
	_shortcutFilter = getTextFromCombo(::GetDlgItem(_hSelf, IDC_BABYGRID_FILTER));

	switch(_currentState) 
	{
		case STATE_MENU:
		{
			vector<CommandShortcut> & cshortcuts = nppParam.getUserShortcuts();
			cs_index = 1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
						isMarker = _babygrid.setMarker(true);

					_babygrid.setText(cs_index, 1, cshortcuts[i].getName());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, cshortcuts[i].toString().c_str());
					_babygrid.setText(cs_index, 3, cshortcuts[i].getCategory());
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 3);
			::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), true);
			::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), true);
			::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
		}
		break;

		case STATE_MACRO:
		{
			vector<MacroShortcut> & cshortcuts = nppParam.getMacroList();
			cs_index = 1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
						isMarker = _babygrid.setMarker(true);

					_babygrid.setText(cs_index, 1, cshortcuts[i].getName());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, cshortcuts[i].toString().c_str());
	
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 2);
            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), shouldBeEnabled);
		}
		break;

		case STATE_USER:
		{
			vector<UserCommand> & cshortcuts = nppParam.getUserCommandList();
			cs_index = 1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
						isMarker = _babygrid.setMarker(true);

					_babygrid.setText(cs_index, 1, cshortcuts[i].getName());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, cshortcuts[i].toString().c_str());
	
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 2);

            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), shouldBeEnabled);
		}
		break;

		case STATE_PLUGIN:
		{
			vector<PluginCmdShortcut> & cshortcuts = nppParam.getPluginCommandList();
			cs_index = 1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
						isMarker = _babygrid.setMarker(true);

					_babygrid.setText(cs_index, 1, cshortcuts[i].getName());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, cshortcuts[i].toString().c_str());
					_babygrid.setText(cs_index, 3, cshortcuts[i].getModuleName());
	
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 3);
            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
		}
		break;

		case STATE_SCINTILLA:
		{
			vector<ScintillaKeyMap> & cshortcuts = nppParam.getScintillaKeyList();
			cs_index=1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (cshortcuts[i].isEnabled())
					{
						size_t sciCombos = cshortcuts[i].getSize();
						for (size_t sciIndex = 0; sciIndex < sciCombos; ++sciIndex)
						{
							if (findKeyConflicts(nullptr, cshortcuts[i].getKeyComboByIndex(sciIndex), i))
							{
								isMarker = _babygrid.setMarker(true);
								break;
							}
						}
					}

					_babygrid.setText(cs_index, 1, cshortcuts[i].getName());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, cshortcuts[i].toString().c_str());
	
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 2);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), false);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
		}
		break;
	}
	
	if (nbItems > 0) //restore the last view
		_babygrid.setLastView(_lastHomeRow[_currentState], _lastCursorRow[_currentState]);
	else //clear the info area
		::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, 0);

	_babygrid.setInitialContent(false);
}

intptr_t CALLBACK ShortcutMapper::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			initBabyGrid();
			initTabs();
			fillOutBabyGrid();
			_babygrid.display();	
			goToCenter();

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			RECT rect;
			Window::getClientRect(rect);
			_clientWidth = rect.right - rect.left;
			_clientHeight = rect.bottom - rect.top;

			int cy_border = GetSystemMetrics(SM_CYFRAME);
			int cy_caption = GetSystemMetrics(SM_CYCAPTION);
			_initClientWidth = _clientWidth;
			_initClientHeight = _clientHeight + cy_caption + cy_border;
			_dialogInitDone = true;

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_GETMINMAXINFO :
		{
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			if (_dialogInitDone)
			{
				mmi->ptMinTrackSize.x = _initClientWidth;
				mmi->ptMinTrackSize.y = _initClientHeight;
			}
			return 0;
		}

		case WM_DESTROY:
		{
			for (const HFONT & hFont : _hGridFonts)
				::DeleteObject(hFont);

			_hGridFonts.clear();
			_hGridFonts.shrink_to_fit();
			break;
		}

		case WM_SIZE:
		{
			LONG newWidth = LOWORD(lParam);
			LONG newHeight = HIWORD(lParam);
			RECT rect;

			LONG addWidth = newWidth - _clientWidth;
			LONG addHeight = newHeight - _clientHeight;
			_clientWidth = newWidth;
			_clientHeight = newHeight;

			getClientRect(rect);
			_babygrid.reSizeToWH(rect);
			
			//elements that need to be moved
			const auto moveWindowIDs = {
				IDM_BABYGRID_MODIFY, IDM_BABYGRID_CLEAR, IDM_BABYGRID_DELETE, IDOK
			};
			const UINT flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;
			Window::getClientRect(rect);

			for (int moveWndID : moveWindowIDs)
			{
				HWND moveHwnd = ::GetDlgItem(_hSelf, moveWndID);
				::GetWindowRect(moveHwnd, &rect);
				::MapWindowPoints(NULL, _hSelf, (LPPOINT)&rect, 2);
				::SetWindowPos(moveHwnd, NULL, rect.left + addWidth / 2, rect.top + addHeight, 0, 0, SWP_NOSIZE | flags);
			}
			HWND moveHwnd = ::GetDlgItem(_hSelf, IDC_BABYGRID_STATIC);
			::GetWindowRect(moveHwnd, &rect);
			::MapWindowPoints(NULL, _hSelf, (LPPOINT)&rect, 2);
			::SetWindowPos(moveHwnd, NULL, rect.left, rect.top + addHeight, 0, 0, SWP_NOSIZE | flags);
			
			// Move and resize IDC_BABYGRID_INFO and IDC_BABYGRID_FILTER
			// Move the Y position, Resize the width
			HWND resizeHwnd = ::GetDlgItem(_hSelf, IDC_BABYGRID_INFO);
			::GetWindowRect(resizeHwnd, &rect);
			::MapWindowPoints(NULL, _hSelf, (LPPOINT)&rect, 2);
			::SetWindowPos(resizeHwnd, NULL, rect.left, rect.top + addHeight, rect.right - rect.left + addWidth, rect.bottom - rect.top, flags);
			
			resizeHwnd = ::GetDlgItem(_hSelf, IDC_BABYGRID_FILTER);
			::GetWindowRect(resizeHwnd, &rect);
			::MapWindowPoints(NULL, _hSelf, (LPPOINT)&rect, 2);
			::SetWindowPos(resizeHwnd, NULL, rect.left, rect.top + addHeight, rect.right - rect.left + addWidth, rect.bottom - rect.top, flags);

			break;
		}
		break;

		case WM_NOTIFY:
		{
			NMHDR nmh = *((NMHDR*)lParam);
			if (nmh.hwndFrom == _hTabCtrl)
			{
				if (nmh.code == TCN_SELCHANGE)
				{
					//save the current view
					_lastHomeRow[_currentState] = _babygrid.getHomeRow();
					_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
					int index = TabCtrl_GetCurSel(_hTabCtrl);

					switch (index)
					{
						case 0:
							_currentState = STATE_MENU;
							break;

						case 1:
							_currentState = STATE_MACRO;
							break;

						case 2:
							_currentState = STATE_USER;
							break;

						case 3:
							_currentState = STATE_PLUGIN;
							break;

						case 4:
							_currentState = STATE_SCINTILLA;
							break;
					}
					fillOutBabyGrid();
				}
			}
		}
		break;

		case NPPM_INTERNAL_FINDKEYCONFLICTS:
		{
			if (!wParam || !lParam)
				break;

			generic_string conflictInfo;

			// In case of using filter will make the filtered items change index, so here we get its real index
			size_t realIndexOfSelectedItem = _shortcutIndex[_babygrid.getSelectedRow() - 1];

			const bool isConflict = findKeyConflicts(&conflictInfo, *reinterpret_cast<KeyCombo*>(wParam), realIndexOfSelectedItem);
			*reinterpret_cast<bool*>(lParam) = isConflict;

			if (isConflict)
				::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(conflictInfo.c_str()));
			else
				::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_conflictInfoEditing.c_str()));

			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL :
				{
					::EndDialog(_hSelf, -1);
					return TRUE;
				}
				case IDOK :
				{
					::EndDialog(_hSelf, 0);
					return TRUE;
				}

				case IDM_BABYGRID_CLEAR :
				{
					if (_babygrid.getNumberRows() < 1)
						return TRUE;

					NppParameters& nppParam = NppParameters::getInstance();
					int row = _babygrid.getSelectedRow();
					size_t shortcutIndex = _shortcutIndex[row-1];
					bool isModified = false;

					switch(_currentState)
					{
						case STATE_MENU:
						{
							//Get CommandShortcut corresponding to row
							vector<CommandShortcut> & shortcuts = nppParam.getUserShortcuts();
							CommandShortcut csc = shortcuts[shortcutIndex];
							csc.clear();
							shortcuts[shortcutIndex] = csc;
							//shortcut was altered
							nppParam.addUserModifiedIndex(shortcutIndex);

							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam.getAccelerator()->updateShortcuts();
							nppParam.setShortcutDirty();
						}
						break;

						case STATE_MACRO: 
						{
							//Get MacroShortcut corresponding to row
							vector<MacroShortcut> & shortcuts = nppParam.getMacroList();
							MacroShortcut msc = shortcuts[shortcutIndex];
							msc.clear();
							shortcuts[shortcutIndex] = msc;
							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam.getAccelerator()->updateShortcuts();
							nppParam.setShortcutDirty();
						}
						break;

						case STATE_USER: 
						{
							//Get UserCommand corresponding to row
							vector<UserCommand> & shortcuts = nppParam.getUserCommandList();
							UserCommand ucmd = shortcuts[shortcutIndex];
							ucmd.clear();

							//shortcut was altered
							shortcuts[shortcutIndex] = ucmd;

							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam.getAccelerator()->updateShortcuts();
							nppParam.setShortcutDirty();
						}
						break;

						case STATE_PLUGIN: 
						{
							//Get PluginCmdShortcut corresponding to row
							vector<PluginCmdShortcut> & shortcuts = nppParam.getPluginCommandList();
							PluginCmdShortcut pcsc = shortcuts[shortcutIndex];
							pcsc.clear();
							//shortcut was altered
							nppParam.addPluginModifiedIndex(shortcutIndex);
							shortcuts[shortcutIndex] = pcsc;

							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam.getAccelerator()->updateShortcuts();
							unsigned long cmdID = pcsc.getID();
							ShortcutKey shortcut;
							shortcut._isAlt = FALSE;
							shortcut._isCtrl = FALSE;
							shortcut._isShift = FALSE;
							shortcut._key = '\0';

							::SendMessage(_hParent, NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED, cmdID, reinterpret_cast<LPARAM>(&shortcut));
							nppParam.setShortcutDirty();
						}
						break;

						case STATE_SCINTILLA: 
						{
							// Do nothing
						}
						break;

					}

					if (!isModified)
						::SendMessage(_hSelf, WM_COMMAND, MAKEWPARAM(IDD_BABYGRID_ID1, BGN_ROWCHANGED), row);
					
					return TRUE;
				}

				case IDM_BABYGRID_MODIFY :
				{
					if (_babygrid.getNumberRows() < 1)
						return TRUE;

					NppParameters& nppParam = NppParameters::getInstance();
					int row = _babygrid.getSelectedRow();
					size_t shortcutIndex = _shortcutIndex[row-1];
					bool isModified = false;

					switch(_currentState)
					{
						case STATE_MENU:
						{
							//Get CommandShortcut corresponding to row
							vector<CommandShortcut> & shortcuts = nppParam.getUserShortcuts();
							CommandShortcut csc = shortcuts[shortcutIndex], prevcsc = shortcuts[shortcutIndex];
							csc.init(_hInst, _hSelf);
							if (csc.doDialog() != -1 && prevcsc != csc)
							{
								//shortcut was altered
								nppParam.addUserModifiedIndex(shortcutIndex);
								shortcuts[shortcutIndex] = csc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam.getAccelerator()->updateShortcuts();
								nppParam.setShortcutDirty();
							}
						}
						break;

						case STATE_MACRO: 
						{
							//Get MacroShortcut corresponding to row
							vector<MacroShortcut> & shortcuts = nppParam.getMacroList();
							MacroShortcut msc = shortcuts[shortcutIndex], prevmsc = shortcuts[shortcutIndex];
							msc.init(_hInst, _hSelf);
							if (msc.doDialog() != -1 && prevmsc != msc)
							{
								//shortcut was altered
								shortcuts[shortcutIndex] = msc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam.getAccelerator()->updateShortcuts();
								nppParam.setShortcutDirty();
							}
						}
						break; 

						case STATE_USER: 
						{
							//Get UserCommand corresponding to row
							vector<UserCommand> & shortcuts = nppParam.getUserCommandList();
							UserCommand ucmd = shortcuts[shortcutIndex];
							ucmd.init(_hInst, _hSelf);
							UserCommand prevucmd = ucmd;
							if (ucmd.doDialog() != -1 && prevucmd != ucmd)
							{
								//shortcut was altered
								shortcuts[shortcutIndex] = ucmd;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam.getAccelerator()->updateShortcuts();
								nppParam.setShortcutDirty();
							}
						}
						break; 

						case STATE_PLUGIN:
						{
							//Get PluginCmdShortcut corresponding to row
							vector<PluginCmdShortcut> & shortcuts = nppParam.getPluginCommandList();
							PluginCmdShortcut pcsc = shortcuts[shortcutIndex];
							pcsc.init(_hInst, _hSelf);
							PluginCmdShortcut prevpcsc = pcsc;
							if (pcsc.doDialog() != -1 && prevpcsc != pcsc)
							{
								//shortcut was altered
								nppParam.addPluginModifiedIndex(shortcutIndex);
								shortcuts[shortcutIndex] = pcsc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam.getAccelerator()->updateShortcuts();
								unsigned long cmdID = pcsc.getID();
								ShortcutKey shortcut;
								shortcut._isAlt = pcsc.getKeyCombo()._isAlt;
								shortcut._isCtrl = pcsc.getKeyCombo()._isCtrl;
								shortcut._isShift = pcsc.getKeyCombo()._isShift;
								shortcut._key = pcsc.getKeyCombo()._key;

								::SendMessage(_hParent, NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED, cmdID, reinterpret_cast<LPARAM>(&shortcut));
								nppParam.setShortcutDirty();
							}
						}
						break;

						case STATE_SCINTILLA:
						{
							//Get ScintillaKeyMap corresponding to row
							vector<ScintillaKeyMap> & shortcuts = nppParam.getScintillaKeyList();
							ScintillaKeyMap skm = shortcuts[shortcutIndex], prevskm = shortcuts[shortcutIndex];
							skm.init(_hInst, _hSelf);
							if (skm.doDialog() != -1 && prevskm != skm)
							{
								//shortcut was altered
								nppParam.addScintillaModifiedIndex((int)shortcutIndex);
								shortcuts[shortcutIndex] = skm;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();
								_babygrid.updateView();

								isModified = true;

								//Notify current Accelerator class to update key
								nppParam.getScintillaAccelerator()->updateKeys();
								nppParam.setShortcutDirty();
							}
						}
						break;  
					}

					if (!isModified)
						::SendMessage(_hSelf, WM_COMMAND, MAKEWPARAM(IDD_BABYGRID_ID1, BGN_ROWCHANGED), row);

					return TRUE;
				}

				case IDM_BABYGRID_DELETE :
				{
					if (_babygrid.getNumberRows() < 1)
						return TRUE;

					NppParameters& nppParam = NppParameters::getInstance();
					int res = nppParam.getNativeLangSpeaker()->messageBox("SCMapperDoDeleteOrNot",
						_hSelf,
						TEXT("Are you sure you want to delete this shortcut?"),
						TEXT("Are you sure?"),
						MB_OKCANCEL);

					if (res == IDOK)
					{
						const int row = _babygrid.getSelectedRow();
						size_t shortcutIndex = _shortcutIndex[row-1];
						DWORD cmdID = 0;
						
						// Menu data
						int32_t posBase = 0;
						size_t nbElem = 0;
						HMENU hMenu = NULL;
                        int modifCmd = IDM_SETTING_SHORTCUT_MAPPER_RUN;
						switch(_currentState) 
						{
							case STATE_MENU:
							case STATE_PLUGIN:
							case STATE_SCINTILLA: 
							{
								return FALSE;
							}

							case STATE_MACRO: 
							{
								vector<MacroShortcut> & theMacros = nppParam.getMacroList();
								vector<MacroShortcut>::iterator it = theMacros.begin();
								cmdID = theMacros[shortcutIndex].getID();
								theMacros.erase(it + shortcutIndex);

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();

								const size_t numberRows = _babygrid.getNumberRows();
								if (_lastHomeRow[_currentState] == numberRows)
									--_lastHomeRow[_currentState];
								if (_lastCursorRow[_currentState] == numberRows)
									--_lastCursorRow[_currentState];

								fillOutBabyGrid();

								// preparing to remove from menu
								posBase = 6;
								nbElem = theMacros.size();
								HMENU m = reinterpret_cast<HMENU>(::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0));
								hMenu = ::GetSubMenu(m, MENUINDEX_MACRO);

								modifCmd = IDM_SETTING_SHORTCUT_MAPPER_MACRO;
								for (size_t i = shortcutIndex; i < nbElem; ++i)	//lower the IDs of the remaining items so there are no gaps
								{
									MacroShortcut ms = theMacros[i];
									ms.setID(ms.getID() - 1);	//shift all IDs
									theMacros[i] = ms;
								}
							}
							break; 

							case STATE_USER: 
							{
								vector<UserCommand> & theUserCmds = nppParam.getUserCommandList();
								vector<UserCommand>::iterator it = theUserCmds.begin();
								cmdID = theUserCmds[shortcutIndex].getID();
								theUserCmds.erase(it + shortcutIndex);

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();

								const size_t numberRows = _babygrid.getNumberRows();
								if (_lastHomeRow[_currentState] == numberRows)
									--_lastHomeRow[_currentState];
								if (_lastCursorRow[_currentState] == numberRows)
									--_lastCursorRow[_currentState];

								fillOutBabyGrid();

								// preparing to remove from menu
								posBase = 2;
								nbElem = theUserCmds.size();
								HMENU m = reinterpret_cast<HMENU>(::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0));
								hMenu = ::GetSubMenu(m, MENUINDEX_RUN);

								modifCmd = IDM_SETTING_SHORTCUT_MAPPER_RUN;
								for (size_t i = shortcutIndex; i < nbElem; ++i)	//lower the IDs of the remaining items so there are no gaps
								{
									UserCommand uc = theUserCmds[i];
									uc.setID(uc.getID() - 1);	//shift all IDs
									theUserCmds[i] = uc;
								}
							}
							break;
						}

                        // updateShortcuts() will update all menu item - the menu items will be shifted
						nppParam.getAccelerator()->updateShortcuts();
						nppParam.setShortcutDirty();

						if (!hMenu) return FALSE;

                        // All menu items are shifted up. So we delete the last item
						::RemoveMenu(hMenu, posBase + static_cast<int32_t>(nbElem), MF_BYPOSITION);

                        if (nbElem == 0) 
                        {
                            ::RemoveMenu(hMenu, modifCmd, MF_BYCOMMAND);
                            
                            //remove separator
							::RemoveMenu(hMenu, posBase-1, MF_BYPOSITION);
                            ::RemoveMenu(hMenu, posBase-1, MF_BYPOSITION);
						}
					}
					return TRUE;
				}

				case IDD_BABYGRID_ID1: 
				{
					switch (HIWORD(wParam))
					{
						case BGN_CELLDBCLICKED: //a cell was clicked in the properties grid
						{
							return ::SendMessage(_hSelf, WM_COMMAND, IDM_BABYGRID_MODIFY, LOWORD(lParam));
						}

						case BGN_CELLRCLICKED: //a cell was clicked in the properties grid
						{
							POINT p;
							::GetCursorPos(&p);
							if (!_rightClickMenu.isCreated())
							{
								vector<MenuItemUnit> itemUnitArray;
								itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_MODIFY, TEXT("Modify")));
								itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_DELETE, TEXT("Delete")));
								itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_CLEAR, TEXT("Clear")));
								_rightClickMenu.create(_hSelf, itemUnitArray);
							}

							if (_babygrid.getNumberRows() < 1)
							{
								_rightClickMenu.enableItem(IDM_BABYGRID_MODIFY, false);
								_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, false);
								_rightClickMenu.enableItem(IDM_BABYGRID_CLEAR, false);
							}
							else
							{
								_rightClickMenu.enableItem(IDM_BABYGRID_MODIFY, true);
								_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, true);
								if (_currentState == STATE_SCINTILLA)
									_rightClickMenu.enableItem(IDM_BABYGRID_CLEAR, false);
								else
									_rightClickMenu.enableItem(IDM_BABYGRID_CLEAR, true);

								switch(_currentState)
								{
									case STATE_MACRO:
									case STATE_USER:
									{
										_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, true);
									}
									break;

									case STATE_MENU:
									case STATE_PLUGIN:
									case STATE_SCINTILLA:
									{
										_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, false);
									}
									break;
								}
							}

							_rightClickMenu.display(p);
							return TRUE;
						}

						case BGN_DELETECELL: //VK_DELETE
						{
							switch(_currentState) 
							{
								case STATE_MACRO:
								case STATE_USER:
									return ::SendMessage(_hSelf, WM_COMMAND, IDM_BABYGRID_DELETE, 0);
							}
							return TRUE;
						}

						case BGN_ROWCHANGED:
						{
							if (_babygrid.getNumberRows() < 1)
								return TRUE;

							NppParameters& nppParam = NppParameters::getInstance();
							const size_t currentIndex = LOWORD(lParam) - 1;

							// In case of using filter will make the filtered items change index, so here we get its real index
							size_t realIndexOfSelectedItem = _shortcutIndex[currentIndex];

							generic_string conflictInfo;

							switch (_currentState)
							{
								case STATE_MENU:
								{
									vector<CommandShortcut> & vShortcuts = nppParam.getUserShortcuts();
									findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyCombo(), realIndexOfSelectedItem);
								}
								break;

								case STATE_MACRO:
								{
									vector<MacroShortcut> & vShortcuts = nppParam.getMacroList();
									findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyCombo(), realIndexOfSelectedItem);
								}
								break;

								case STATE_USER:
								{
									vector<UserCommand> & vShortcuts = nppParam.getUserCommandList();
									findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyCombo(), realIndexOfSelectedItem);
								}
								break;

								case STATE_PLUGIN:
								{
									vector<PluginCmdShortcut> & vShortcuts = nppParam.getPluginCommandList();
									findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyCombo(), realIndexOfSelectedItem);
								}
								break;

								case STATE_SCINTILLA:
								{
									vector<ScintillaKeyMap> & vShortcuts = nppParam.getScintillaKeyList();
									size_t sciCombos = vShortcuts[realIndexOfSelectedItem].getSize();
									for (size_t sciIndex = 0; sciIndex < sciCombos; ++sciIndex)
										findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyComboByIndex(sciIndex), realIndexOfSelectedItem);
								}
								break;
							}

							if (conflictInfo.empty())
								::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_conflictInfoOk.c_str()));
							else
								::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(conflictInfo.c_str()));

							return TRUE;
						}
					}
				}
				case IDC_BABYGRID_FILTER:
				{
					if (HIWORD(wParam) == EN_CHANGE)
					{
						fillOutBabyGrid();
					}
					return TRUE;
				}
			}
		}

		default:
			return FALSE;

	}

	return FALSE;
}

bool ShortcutMapper::findKeyConflicts(__inout_opt generic_string * const keyConflictLocation,
										const KeyCombo & itemKeyComboToTest, const size_t & itemIndexToTest) const
{
	if (itemKeyComboToTest._key == 0) //no key assignment
		return false;

	bool retIsConflict = false; //returns true when a conflict is found
	NppParameters& nppParam = NppParameters::getInstance();

	for (size_t gridState = STATE_MENU; gridState <= STATE_SCINTILLA; ++gridState)
	{
		switch (gridState)
		{
			case STATE_MENU:
			{
				vector<CommandShortcut> & vShortcuts = nppParam.getUserShortcuts();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					if (isConflict(vShortcuts[itemIndex].getKeyCombo(), itemKeyComboToTest))
					{
						retIsConflict = true;
						if (keyConflictLocation == nullptr)
							return retIsConflict;
						else
						{
							if (!keyConflictLocation->empty())
								*keyConflictLocation += TEXT("\r\n");
							*keyConflictLocation += _tabNames[gridState];
							*keyConflictLocation += TEXT("  |  ");
							*keyConflictLocation += std::to_wstring(itemIndex + 1);
							*keyConflictLocation += TEXT("   ");
							*keyConflictLocation += vShortcuts[itemIndex].getName();
							*keyConflictLocation += TEXT("  ( ");
							*keyConflictLocation += vShortcuts[itemIndex].toString();
							*keyConflictLocation += TEXT(" )");
						}
					}
				}
				break;
			} //case STATE_MENU
			case STATE_MACRO:
			{
				vector<MacroShortcut> & vShortcuts = nppParam.getMacroList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					if (isConflict(vShortcuts[itemIndex].getKeyCombo(), itemKeyComboToTest))
					{
						retIsConflict = true;
						if (keyConflictLocation == nullptr)
							return retIsConflict;
						else
						{
							if (!keyConflictLocation->empty())
								*keyConflictLocation += TEXT("\r\n");
							*keyConflictLocation += _tabNames[gridState];
							*keyConflictLocation += TEXT("  |  ");
							*keyConflictLocation += std::to_wstring(itemIndex + 1);
							*keyConflictLocation += TEXT("   ");
							*keyConflictLocation += vShortcuts[itemIndex].getName();
							*keyConflictLocation += TEXT("  ( ");
							*keyConflictLocation += vShortcuts[itemIndex].toString();
							*keyConflictLocation += TEXT(" )");
						}
					}
				}
				break;
			} //case STATE_MACRO
			case STATE_USER:
			{
				vector<UserCommand> & vShortcuts = nppParam.getUserCommandList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					if (isConflict(vShortcuts[itemIndex].getKeyCombo(), itemKeyComboToTest))
					{
						retIsConflict = true;
						if (keyConflictLocation == nullptr)
							return retIsConflict;
						else
						{
							if (!keyConflictLocation->empty())
								*keyConflictLocation += TEXT("\r\n");
							*keyConflictLocation += _tabNames[gridState];
							*keyConflictLocation += TEXT("  |  ");
							*keyConflictLocation += std::to_wstring(itemIndex + 1);
							*keyConflictLocation += TEXT("   ");
							*keyConflictLocation += vShortcuts[itemIndex].getName();
							*keyConflictLocation += TEXT("  ( ");
							*keyConflictLocation += vShortcuts[itemIndex].toString();
							*keyConflictLocation += TEXT(" )");
						}
					}
				}
				break;
			} //case STATE_USER
			case STATE_PLUGIN:
			{
				vector<PluginCmdShortcut> & vShortcuts = nppParam.getPluginCommandList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					if (isConflict(vShortcuts[itemIndex].getKeyCombo(), itemKeyComboToTest))
					{
						retIsConflict = true;
						if (keyConflictLocation == nullptr)
							return retIsConflict;
						else
						{
							if (!keyConflictLocation->empty())
								*keyConflictLocation += TEXT("\r\n");
							*keyConflictLocation += _tabNames[gridState];
							*keyConflictLocation += TEXT("  |  ");
							*keyConflictLocation += std::to_wstring(itemIndex + 1);
							*keyConflictLocation += TEXT("   ");
							*keyConflictLocation += vShortcuts[itemIndex].getName();
							*keyConflictLocation += TEXT("  ( ");
							*keyConflictLocation += vShortcuts[itemIndex].toString();
							*keyConflictLocation += TEXT(" )");
						}
					}
				}
				break;
			} //case STATE_PLUGIN
			case STATE_SCINTILLA:
			{
				vector<ScintillaKeyMap> & vShortcuts = nppParam.getScintillaKeyList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					size_t sciCombos = vShortcuts[itemIndex].getSize();
					for (size_t sciIndex = 0; sciIndex < sciCombos; ++sciIndex)
					{
						if (isConflict(vShortcuts[itemIndex].getKeyComboByIndex(sciIndex), itemKeyComboToTest))
						{
							retIsConflict = true;
							if (keyConflictLocation == nullptr)
								return retIsConflict;
							else
							{
								if (!keyConflictLocation->empty())
									*keyConflictLocation += TEXT("\r\n");
								*keyConflictLocation += _tabNames[gridState];
								*keyConflictLocation += TEXT("  |  ");
								*keyConflictLocation += std::to_wstring(itemIndex + 1);
								if (sciIndex > 0)
									*keyConflictLocation += TEXT("*   ");
								else
									*keyConflictLocation += TEXT("   ");
								*keyConflictLocation += vShortcuts[itemIndex].getName();
								*keyConflictLocation += TEXT("  ( ");
								*keyConflictLocation += vShortcuts[itemIndex].toString(sciIndex);
								*keyConflictLocation += TEXT(" )");
							}
						}
					}
				}
				break;
			}
		}
	}
	return retIsConflict;
}
