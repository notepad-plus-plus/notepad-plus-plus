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


#include "ShortcutMapper.h"
#include "Notepad_plus.h"

#ifdef UNICODE
#define numToStr std::to_wstring
#else
#define numToStr std::to_string
#endif //UNICODE

using namespace std;

void ShortcutMapper::initTabs() {
	HWND hTab = _hTabCtrl = ::GetDlgItem(_hSelf, IDC_BABYGRID_TABBAR);
	TCITEM tie;
	tie.mask = TCIF_TEXT;
	tie.pszText = tabNames[0];
	::SendMessage(hTab, TCM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tie));
	tie.pszText = tabNames[1];
	::SendMessage(hTab, TCM_INSERTITEM, 1, reinterpret_cast<LPARAM>(&tie));
	tie.pszText = tabNames[2];
	::SendMessage(hTab, TCM_INSERTITEM, 2, reinterpret_cast<LPARAM>(&tie));
	tie.pszText = tabNames[3];
	::SendMessage(hTab, TCM_INSERTITEM, 3, reinterpret_cast<LPARAM>(&tie));
	tie.pszText = tabNames[4];
	::SendMessage(hTab, TCM_INSERTITEM, 4, reinterpret_cast<LPARAM>(&tie));

    TabCtrl_SetCurSel(_hTabCtrl, int(_currentState));

	// force alignment to babygrid
	RECT rcTab;
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	::GetWindowPlacement(hTab, &wp);
	::SendMessage(hTab, TCM_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&rcTab));

	wp.rcNormalPosition.bottom = NppParameters::getInstance()->_dpiManager.scaleY(30);
	wp.rcNormalPosition.top = wp.rcNormalPosition.bottom - rcTab.bottom;

	::SetWindowPlacement(hTab, &wp);
}

void ShortcutMapper::getClientRect(RECT & rc) const 
{
		Window::getClientRect(rc);

		rc.top += NppParameters::getInstance()->_dpiManager.scaleY(30);
		rc.bottom -= NppParameters::getInstance()->_dpiManager.scaleY(108);
		rc.left += NppParameters::getInstance()->_dpiManager.scaleX(5);
		rc.right -= NppParameters::getInstance()->_dpiManager.scaleX(5);
}

void ShortcutMapper::translateTab(int index, const TCHAR * newname) {
	if (index < 0 || index > 4)
		return;
	generic_strncpy(tabNames[index], newname, maxTabName);
}

void ShortcutMapper::initBabyGrid() {
	RECT rect;
	getClientRect(rect);

	_lastHomeRow.resize(5, 1);
	_lastCursorRow.resize(5, 1);

	_hGridFonts.resize(MAX_GRID_FONTS);
	_hGridFonts.at(GFONT_HEADER) = ::CreateFont(
		NppParameters::getInstance()->_dpiManager.scaleY(18), 0, 0, 0, FW_BOLD,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		TEXT("MS Shell Dlg"));
	_hGridFonts.at(GFONT_ROWS) = ::CreateFont(
		NppParameters::getInstance()->_dpiManager.scaleY(16), 0, 0, 0, FW_NORMAL,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		TEXT("MS Shell Dlg"));
	
	_babygrid.init(_hInst, _hSelf, IDD_BABYGRID_ID1);

	_babygrid.setHeaderFont(_hGridFonts.at(GFONT_HEADER));
	_babygrid.setRowFont(_hGridFonts.at(GFONT_ROWS));
	
	_babygrid.reSizeToWH(rect);
	_babygrid.hideCursor();
	_babygrid.makeColAutoWidth(false);
	_babygrid.setAutoRow(false);
	_babygrid.setColsNumbered(false);
	_babygrid.setColWidth(0, NppParameters::getInstance()->_dpiManager.scaleX(30));
	_babygrid.setColWidth(1, NppParameters::getInstance()->_dpiManager.scaleX(290));
	_babygrid.setColWidth(2, NppParameters::getInstance()->_dpiManager.scaleX(140));
	_babygrid.setColWidth(3, NppParameters::getInstance()->_dpiManager.scaleX(40));
	_babygrid.setHeaderHeight(NppParameters::getInstance()->_dpiManager.scaleY(21));
	_babygrid.setRowHeight(NppParameters::getInstance()->_dpiManager.scaleY(21));

	_babygrid.setHighlightColorNoFocus(RGB(200,200,210));
	_babygrid.setProtectColor(RGB(255,130,120));
	_babygrid.setHighlightColorProtect(RGB(244,10,20));
	_babygrid.setHighlightColorProtectNoFocus(RGB(230,194,190));
}

void ShortcutMapper::fillOutBabyGrid()
{
	NppParameters *nppParam = NppParameters::getInstance();
	_babygrid.clear();
	_babygrid.setInitialContent(true);

	size_t nbItems = 0;

	_babygrid.setText(0, 1, TEXT("Name"));
	_babygrid.setText(0, 2, TEXT("Shortcut"));
	
	switch(_currentState) {
		case STATE_MENU: {
			nbItems = nppParam->getUserShortcuts().size();
			_babygrid.setLineColNumber(nbItems, 3);
			_babygrid.setText(0, 3, TEXT("Category"));
			break; }
		case STATE_MACRO: {
			nbItems = nppParam->getMacroList().size();
			_babygrid.setLineColNumber(nbItems, 2);
			break; }
		case STATE_USER: {
			nbItems = nppParam->getUserCommandList().size();
			_babygrid.setLineColNumber(nbItems, 2);
			break; }
		case STATE_PLUGIN: {
			nbItems = nppParam->getPluginCommandList().size();
			_babygrid.setLineColNumber(nbItems, 3);
			_babygrid.setText(0, 3, TEXT("Plugin"));
			break; }
		case STATE_SCINTILLA: {
			nbItems = nppParam->getScintillaKeyList().size();
			_babygrid.setLineColNumber(nbItems, 2);
			break; }
	}

	bool isMarker = false;

	switch(_currentState) {
		case STATE_MENU: {
			vector<CommandShortcut> & cshortcuts = nppParam->getUserShortcuts();
			for(size_t i = 0; i < nbItems; ++i)
			{
				if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
					isMarker = _babygrid.setMarker(true);

				_babygrid.setText(i+1, 1, cshortcuts[i].getName());
				if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
					_babygrid.setText(i+1, 2, cshortcuts[i].toString().c_str());
				_babygrid.setText(i+1, 3, cshortcuts[i].getCategory());
				if (isMarker)
					isMarker = _babygrid.setMarker(false);
			}
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
			break; }
		case STATE_MACRO: {
			vector<MacroShortcut> & cshortcuts = nppParam->getMacroList();
			for(size_t i = 0; i < nbItems; ++i)
			{
				if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
					isMarker = _babygrid.setMarker(true);

				_babygrid.setText(i+1, 1, cshortcuts[i].getName());
				if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
					_babygrid.setText(i+1, 2, cshortcuts[i].toString().c_str());

				if (isMarker)
					isMarker = _babygrid.setMarker(false);
			}
            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), shouldBeEnabled);
			break; }
		case STATE_USER: {
			vector<UserCommand> & cshortcuts = nppParam->getUserCommandList();
			for(size_t i = 0; i < nbItems; ++i)
			{
				if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
					isMarker = _babygrid.setMarker(true);

				_babygrid.setText(i+1, 1, cshortcuts[i].getName());
				if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
					_babygrid.setText(i+1, 2, cshortcuts[i].toString().c_str());

				if (isMarker)
					isMarker = _babygrid.setMarker(false);
			}
            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), shouldBeEnabled);
			break; }
		case STATE_PLUGIN: {
			vector<PluginCmdShortcut> & cshortcuts = nppParam->getPluginCommandList();
			for(size_t i = 0; i < nbItems; ++i)
			{
				if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
					isMarker = _babygrid.setMarker(true);

				_babygrid.setText(i+1, 1, cshortcuts[i].getName());
				if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
					_babygrid.setText(i+1, 2, cshortcuts[i].toString().c_str());
				_babygrid.setText(i+1, 3, cshortcuts[i].getModuleName());

				if (isMarker)
					isMarker = _babygrid.setMarker(false);
			}
            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
			break; }
		case STATE_SCINTILLA: {
			vector<ScintillaKeyMap> & cshortcuts = nppParam->getScintillaKeyList();
			for(size_t i = 0; i < nbItems; ++i)
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

				_babygrid.setText(i+1, 1, cshortcuts[i].getName());
				if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
					_babygrid.setText(i+1, 2, cshortcuts[i].toString().c_str());

				if (isMarker)
					isMarker = _babygrid.setMarker(false);
			}
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), false);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
			break; }
	}
	if (nbItems > 0)
		//restore the last view
		_babygrid.setLastView(_lastHomeRow[_currentState], _lastCursorRow[_currentState]);
	else
		//clear the info area
		::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, 0);
	_babygrid.setInitialContent(false);
}

INT_PTR CALLBACK ShortcutMapper::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
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
			return TRUE;
		}

		case WM_DESTROY:
		{
			for (const HFONT & hFont : _hGridFonts)
				::DeleteObject(hFont);
			_hGridFonts.clear();
			_hGridFonts.shrink_to_fit();
			break;
		}

		case WM_NOTIFY: {
			NMHDR nmh = *((NMHDR*)lParam);
			if (nmh.hwndFrom == _hTabCtrl) {
				if (nmh.code == TCN_SELCHANGE) {
					//save the current view
					_lastHomeRow[_currentState] = _babygrid.getHomeRow();
					_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
					int index = TabCtrl_GetCurSel(_hTabCtrl);
					switch (index) {
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
			break; }

		case NPPM_INTERNAL_FINDKEYCONFLICTS:
		{
			if (not wParam || not lParam)
				break;

			generic_string conflictInfo;

			const bool isConflict = findKeyConflicts(&conflictInfo, *reinterpret_cast<KeyCombo*>(wParam), _babygrid.getSelectedRow() - 1);
			*reinterpret_cast<bool*>(lParam) = isConflict;
			if (isConflict)
				::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(conflictInfo.c_str()));
			else
				::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_assignInfo.c_str()));

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

					NppParameters *nppParam = NppParameters::getInstance();
					int row = _babygrid.getSelectedRow();
					bool isModified = false;

					switch(_currentState)
					{
						case STATE_MENU:
						{
							//Get CommandShortcut corresponding to row
							vector<CommandShortcut> & shortcuts = nppParam->getUserShortcuts();
							CommandShortcut csc = shortcuts[row - 1];
							csc.clear();
							shortcuts[row - 1] = csc;
							//shortcut was altered
							nppParam->addUserModifiedIndex(row-1);
						
							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam->getAccelerator()->updateShortcuts();
							nppParam->setShortcutDirty();
							break;
						}
						case STATE_MACRO: 
						{
							//Get MacroShortcut corresponding to row
							vector<MacroShortcut> & shortcuts = nppParam->getMacroList();
							MacroShortcut msc = shortcuts[row - 1];
							msc.clear();
							shortcuts[row - 1] = msc;
							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam->getAccelerator()->updateShortcuts();
							nppParam->setShortcutDirty();
							break;
						}
						case STATE_USER: 
						{
							//Get UserCommand corresponding to row
							vector<UserCommand> & shortcuts = nppParam->getUserCommandList();
							UserCommand ucmd = shortcuts[row - 1];
							ucmd.clear();
						
							//shortcut was altered
							shortcuts[row - 1] = ucmd;

							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam->getAccelerator()->updateShortcuts();
							nppParam->setShortcutDirty();
							break;
						}
						case STATE_PLUGIN: 
						{
							//Get PluginCmdShortcut corresponding to row
							vector<PluginCmdShortcut> & shortcuts = nppParam->getPluginCommandList();
							PluginCmdShortcut pcsc = shortcuts[row - 1];
							pcsc.clear();
							//shortcut was altered
							nppParam->addPluginModifiedIndex(row-1);
							shortcuts[row - 1] = pcsc;
						
							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam->getAccelerator()->updateShortcuts();
							unsigned long cmdID = pcsc.getID();
							ShortcutKey shortcut;
							shortcut._isAlt = FALSE;
							shortcut._isCtrl = FALSE;
							shortcut._isShift = FALSE;
							shortcut._key = '\0';

							::SendMessage(_hParent, NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED, cmdID, reinterpret_cast<LPARAM>(&shortcut));
							nppParam->setShortcutDirty();
							break;
						}
						case STATE_SCINTILLA: 
						{
							// Do nothing
							break;
						}
					}
					if (not isModified)
						::SendMessage(_hSelf, WM_COMMAND, MAKEWPARAM(IDD_BABYGRID_ID1, BGN_ROWCHANGED), row);
					return TRUE;
				}
				case IDM_BABYGRID_MODIFY :
				{
					if (_babygrid.getNumberRows() < 1)
						return TRUE;

					NppParameters *nppParam = NppParameters::getInstance();
					int row = _babygrid.getSelectedRow();
					bool isModified = false;

					switch(_currentState)
					{
						case STATE_MENU:
						{
							//Get CommandShortcut corresponding to row
							vector<CommandShortcut> & shortcuts = nppParam->getUserShortcuts();
							CommandShortcut csc = shortcuts[row - 1], prevcsc = shortcuts[row - 1];
							csc.init(_hInst, _hSelf);
							if (csc.doDialog() != -1 && prevcsc != csc)
							{
								//shortcut was altered
								nppParam->addUserModifiedIndex(row-1);
								shortcuts[row - 1] = csc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								nppParam->setShortcutDirty();
							}
							break;
						}

						case STATE_MACRO: 
						{
							//Get MacroShortcut corresponding to row
							vector<MacroShortcut> & shortcuts = nppParam->getMacroList();
							MacroShortcut msc = shortcuts[row - 1], prevmsc = shortcuts[row - 1];
							msc.init(_hInst, _hSelf);
							if (msc.doDialog() != -1 && prevmsc != msc)
							{
								//shortcut was altered
								shortcuts[row - 1] = msc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								nppParam->setShortcutDirty();
							}
							break; 
						}

						case STATE_USER: 
						{
							//Get UserCommand corresponding to row
							vector<UserCommand> & shortcuts = nppParam->getUserCommandList();
							UserCommand ucmd = shortcuts[row - 1];
							ucmd.init(_hInst, _hSelf);
							UserCommand prevucmd = ucmd;
							if (ucmd.doDialog() != -1 && prevucmd != ucmd)
							{	
								//shortcut was altered
								shortcuts[row - 1] = ucmd;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								nppParam->setShortcutDirty();
							}
							break; 
						}

						case STATE_PLUGIN:
						{
							//Get PluginCmdShortcut corresponding to row
							vector<PluginCmdShortcut> & shortcuts = nppParam->getPluginCommandList();
							PluginCmdShortcut pcsc = shortcuts[row - 1];
							pcsc.init(_hInst, _hSelf);
							PluginCmdShortcut prevpcsc = pcsc;
							if (pcsc.doDialog() != -1 && prevpcsc != pcsc)
							{
								//shortcut was altered
								nppParam->addPluginModifiedIndex(row-1);
								shortcuts[row - 1] = pcsc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								unsigned long cmdID = pcsc.getID();
								ShortcutKey shortcut;
								shortcut._isAlt = pcsc.getKeyCombo()._isAlt;
								shortcut._isCtrl = pcsc.getKeyCombo()._isCtrl;
								shortcut._isShift = pcsc.getKeyCombo()._isShift;
								shortcut._key = pcsc.getKeyCombo()._key;

								::SendMessage(_hParent, NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED, cmdID, reinterpret_cast<LPARAM>(&shortcut));
								nppParam->setShortcutDirty();
							}
							break;
						}

						case STATE_SCINTILLA:
						{
							//Get ScintillaKeyMap corresponding to row
							vector<ScintillaKeyMap> & shortcuts = nppParam->getScintillaKeyList();
							ScintillaKeyMap skm = shortcuts[row - 1], prevskm = shortcuts[row-1];
							skm.init(_hInst, _hSelf);
							if (skm.doDialog() != -1 && prevskm != skm) 
							{
								//shortcut was altered
								nppParam->addScintillaModifiedIndex(row-1);
								shortcuts[row-1] = skm;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();
								_babygrid.updateView();

								isModified = true;

								//Notify current Accelerator class to update key
								nppParam->getScintillaAccelerator()->updateKeys();
								nppParam->setShortcutDirty();
							}
							break; 
						}
					}
					if (not isModified)
						::SendMessage(_hSelf, WM_COMMAND, MAKEWPARAM(IDD_BABYGRID_ID1, BGN_ROWCHANGED), row);
					return TRUE;
				}

				case IDM_BABYGRID_DELETE :
				{
					if (_babygrid.getNumberRows() < 1)
						return TRUE;

					if (::MessageBox(_hSelf, TEXT("Are you sure you want to delete this shortcut?"), TEXT("Are you sure?"), MB_OKCANCEL) == IDOK)
					{
						NppParameters *nppParam = NppParameters::getInstance();
						const int row = _babygrid.getSelectedRow();
						int shortcutIndex = row-1;
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
								return FALSE;			//this is bad
							}

							case STATE_MACRO: 
							{
								vector<MacroShortcut> & theMacros = nppParam->getMacroList();
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
								for (size_t i = shortcutIndex ; i < nbElem ; ++i)	//lower the IDs of the remaining items so there are no gaps
								{
									MacroShortcut ms = theMacros[i];
									ms.setID(ms.getID() - 1);	//shift all IDs
									theMacros[i] = ms;
								}
								break; 
							}

							case STATE_USER: 
							{
								vector<UserCommand> & theUserCmds = nppParam->getUserCommandList();
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
								for (size_t i = shortcutIndex ; i < nbElem ; ++i)	//lower the IDs of the remaining items so there are no gaps
								{
									UserCommand uc = theUserCmds[i];
									uc.setID(uc.getID() - 1);	//shift all IDs
									theUserCmds[i] = uc;
								}
								break;
							}
						}

                        // updateShortcuts() will update all menu item - the menu items will be shifted
						nppParam->getAccelerator()->updateShortcuts();
						nppParam->setShortcutDirty();

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
								switch(_currentState) {
									case STATE_MACRO:
									case STATE_USER: {
										_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, true);
										break; }
									case STATE_MENU:
									case STATE_PLUGIN:
									case STATE_SCINTILLA: {
										_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, false);
										break; }
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

							NppParameters *nppParam = NppParameters::getInstance();
							const size_t currentIndex = LOWORD(lParam) - 1;
							generic_string conflictInfo;

							switch (_currentState)
							{
								case STATE_MENU:
								{
									vector<CommandShortcut> & vShortcuts = nppParam->getUserShortcuts();
									findKeyConflicts(&conflictInfo, vShortcuts[currentIndex].getKeyCombo(), currentIndex);
									break;
								}
								case STATE_MACRO:
								{
									vector<MacroShortcut> & vShortcuts = nppParam->getMacroList();
									findKeyConflicts(&conflictInfo, vShortcuts[currentIndex].getKeyCombo(), currentIndex);
									break;
								}
								case STATE_USER:
								{
									vector<UserCommand> & vShortcuts = nppParam->getUserCommandList();
									findKeyConflicts(&conflictInfo, vShortcuts[currentIndex].getKeyCombo(), currentIndex);
									break;
								}
								case STATE_PLUGIN:
								{
									vector<PluginCmdShortcut> & vShortcuts = nppParam->getPluginCommandList();
									findKeyConflicts(&conflictInfo, vShortcuts[currentIndex].getKeyCombo(), currentIndex);
									break;
								}
								case STATE_SCINTILLA:
								{
									vector<ScintillaKeyMap> & vShortcuts = nppParam->getScintillaKeyList();
									size_t sciCombos = vShortcuts[currentIndex].getSize();
									for (size_t sciIndex = 0; sciIndex < sciCombos; ++sciIndex)
										findKeyConflicts(&conflictInfo, vShortcuts[currentIndex].getKeyComboByIndex(sciIndex), currentIndex);
									break;
								}
							}

							if (conflictInfo.empty())
								::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_defaultInfo.c_str()));
							else
								::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(conflictInfo.c_str()));

							return TRUE;
						}
					} //switch (HIWORD(wParam))
				}
			} //switch (LOWORD(wParam))
		}
		default:
			return FALSE;
	} //switch (message)
	return FALSE;
}

bool ShortcutMapper::findKeyConflicts(__inout_opt generic_string * const keyConflictLocation,
										const KeyCombo & itemKeyComboToTest, const size_t & itemIndexToTest) const
{
	if (itemKeyComboToTest._key == NULL) //no key assignment
		return false;

	bool retIsConflict = false; //returns true when a conflict is found
	NppParameters * nppParam = NppParameters::getInstance();

	for (size_t gridState = STATE_MENU; gridState <= STATE_SCINTILLA; ++gridState)
	{
		switch (gridState)
		{
			case STATE_MENU:
			{
				vector<CommandShortcut> & vShortcuts = nppParam->getUserShortcuts();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (not vShortcuts[itemIndex].isEnabled()) //no key assignment
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
							if (not keyConflictLocation->empty())
								*keyConflictLocation += TEXT("\r\n");
							*keyConflictLocation += tabNames[gridState];
							*keyConflictLocation += TEXT("  |  ");
							*keyConflictLocation += numToStr(itemIndex + 1);
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
				vector<MacroShortcut> & vShortcuts = nppParam->getMacroList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (not vShortcuts[itemIndex].isEnabled()) //no key assignment
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
							if (not keyConflictLocation->empty())
								*keyConflictLocation += TEXT("\r\n");
							*keyConflictLocation += tabNames[gridState];
							*keyConflictLocation += TEXT("  |  ");
							*keyConflictLocation += numToStr(itemIndex + 1);
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
				vector<UserCommand> & vShortcuts = nppParam->getUserCommandList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (not vShortcuts[itemIndex].isEnabled()) //no key assignment
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
							if (not keyConflictLocation->empty())
								*keyConflictLocation += TEXT("\r\n");
							*keyConflictLocation += tabNames[gridState];
							*keyConflictLocation += TEXT("  |  ");
							*keyConflictLocation += numToStr(itemIndex + 1);
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
				vector<PluginCmdShortcut> & vShortcuts = nppParam->getPluginCommandList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (not vShortcuts[itemIndex].isEnabled()) //no key assignment
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
							if (not keyConflictLocation->empty())
								*keyConflictLocation += TEXT("\r\n");
							*keyConflictLocation += tabNames[gridState];
							*keyConflictLocation += TEXT("  |  ");
							*keyConflictLocation += numToStr(itemIndex + 1);
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
				vector<ScintillaKeyMap> & vShortcuts = nppParam->getScintillaKeyList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (not vShortcuts[itemIndex].isEnabled()) //no key assignment
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
								if (not keyConflictLocation->empty())
									*keyConflictLocation += TEXT("\r\n");
								*keyConflictLocation += tabNames[gridState];
								*keyConflictLocation += TEXT("  |  ");
								*keyConflictLocation += numToStr(itemIndex + 1);
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
			} //case STATE_SCINTILLA
		} //switch (gridState)
	} //for (...)
	return retIsConflict;
}
