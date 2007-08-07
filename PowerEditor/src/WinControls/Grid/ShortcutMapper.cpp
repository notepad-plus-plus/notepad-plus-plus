/*
this file is part of notepad++
Copyright (C)2003 Don HO ( donho@altern.org )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ShortcutMapper.h"
#include "Notepad_plus.h"

void ShortcutMapper::initBabyGrid() {
	RECT rect;
	getClientRect(rect);
	
	_babygrid.init(_hInst, _hSelf, IDD_BABYGRID_ID1);
	_babygrid.reSizeTo(rect);
	_babygrid.hideCursor();
	_babygrid.makeColAutoWidth();
	_babygrid.setColsNumbered(false);
	_babygrid.setColWidth(0, 30);
	_babygrid.setColWidth(1, 250);
}

void ShortcutMapper::fillOutBabyGrid()
{
	NppParameters *nppParam = NppParameters::getInstance();
	vector<ScintillaKeyMap> &skms = nppParam->getScintillaKeyList();
	int nbScitillaMsg = skms.size();
	_pAccel = nppParam->getAccelerator();
	_babygrid.setLineColNumber(_pAccel->_nbAccelItems + nbScitillaMsg, 2);
	_babygrid.setText(0, 1, "Name");
	_babygrid.setText(0, 2, "Shortcut");

	int i = 0 ;
	for ( ; i < _pAccel->_nbAccelItems ; i++)
	{
		unsigned char vFlg = _pAccel->_pAccelArray[i].fVirt;
		string shortcut = (vFlg & FCONTROL)?"Ctrl+":"";
		shortcut += (vFlg & FALT)?"Alt+":"";
		shortcut += (vFlg & FSHIFT)?"Shift+":"";
		string key;
		getKeyStrFromVal((unsigned char)_pAccel->_pAccelArray[i].key, key);
		shortcut += key;
		_babygrid.setText(i+1, 2, shortcut.c_str());

		string shortcutName;
		getNameStrFromCmd(_pAccel->_pAccelArray[i].cmd, shortcutName);
		_babygrid.setText(i+1, 1, shortcutName.c_str());
	}

	for (size_t j = 0 ; i < _pAccel->_nbAccelItems + nbScitillaMsg ; i++, j++)
	{
		_babygrid.setText(i+1, 1, skms[j]._name);
		_babygrid.setText(i+1, 2, skms[j].toString().c_str());
	}
}

BOOL CALLBACK ShortcutMapper::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			initBabyGrid();
			fillOutBabyGrid();
			_babygrid.display();	
			goToCenter();
			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
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

				case IDM_BABYGRID_MODIFY :
				{
					NppParameters *nppParam = NppParameters::getInstance();
					int row = _babygrid.getSelectedRow();

					if (row <= _pAccel->_nbAccelItems)
					{
						ACCEL accel = _pAccel->_pAccelArray[row-1];
						string shortcutName;
						DWORD cmdID = _pAccel->_pAccelArray[row-1].cmd;
						ShortcutType st = getNameStrFromCmd(cmdID, shortcutName);

						Shortcut shortcut(shortcutName.c_str(), (accel.fVirt & FCONTROL) != 0, (accel.fVirt & FALT) != 0, (accel.fVirt & FSHIFT) != 0, (unsigned char)accel.key);
						shortcut.init(_hInst, _hSelf);
						shortcut.setNameReadOnly((st != TYPE_CMD) && (st != TYPE_PLUGINCMD));
					
						Shortcut oldSc = shortcut;

						if ((shortcut.doDialog() != -1) && (oldSc != shortcut))
						{
							// Update the key map
							_pAccel->_pAccelArray[row-1].fVirt = FVIRTKEY | (shortcut._isCtrl?FCONTROL:0) | (shortcut._isAlt?FALT:0) | (shortcut._isShift?FSHIFT:0);
							_pAccel->_pAccelArray[row-1].key = shortcut._key;
							_pAccel->reNew();
							
							// Update the GUI
							string sc = shortcut.toString();
							_babygrid.setText(row, 2, sc.c_str());

							// Add (or update) shortcut to vector in order to save what we have done
							if (st == TYPE_CMD)
							{
								int index = -1;
								vector<CommandShortcut> & shortcutList = nppParam->getUserShortcuts();

								for (size_t i = 0 ; i < shortcutList.size() ; i++)
								{
									if (cmdID == shortcutList[i].getID())
									{
										index = i;
										break;
									}
								}

								if (index != -1)
									shortcutList[index].copyShortcut(shortcut);
								else
									shortcutList.push_back(CommandShortcut(cmdID, shortcut));

								::SendMessage(_hParent, NPPM_INTERNAL_CMDLIST_MODIFIED, (WPARAM)sc.c_str(), cmdID);
							}
							else if (st == TYPE_MACRO)
							{
								vector<MacroShortcut> & theMacros = nppParam->getMacroList();
								const int i = cmdID - ID_MACRO;
								theMacros[i].copyShortcut(shortcut);

								HMENU hMenu = ::GetSubMenu(::GetMenu(_hParent), MENUINDEX_MACRO);
								::ModifyMenu(hMenu, cmdID, MF_BYCOMMAND, cmdID, theMacros[i].toMenuItemString().c_str());

								::SendMessage(_hParent, NPPM_INTERNAL_MACROLIST_MODIFIED, 0, 0);
							}
							else if (st == TYPE_USERCMD)
							{
								vector<UserCommand> & theUserCmds = nppParam->getUserCommandList();
								const int i = cmdID - ID_USER_CMD;
								theUserCmds[i].copyShortcut(shortcut);

								HMENU hMenu = ::GetSubMenu(::GetMenu(_hParent), MENUINDEX_RUN);
								::ModifyMenu(hMenu, cmdID, MF_BYCOMMAND, cmdID, theUserCmds[i].toMenuItemString().c_str());

								::SendMessage(_hParent, NPPM_INTERNAL_USERCMDLIST_MODIFIED, 0, 0);
							}
							
							else if (st == TYPE_PLUGINCMD)
							{
								vector<PluginCmdShortcut> & thePluginCmds = (NppParameters::getInstance())->getPluginCommandList();
								int i = -1;
								for (size_t j = 0 ; j < thePluginCmds.size() ; j++)
								{
									if (cmdID == thePluginCmds[j].getID())
									{										
										i = j;
										break;
									}
								}

								thePluginCmds[i].copyShortcut(shortcut);

								HMENU hMenu = ::GetMenu(_hParent);
								::ModifyMenu(hMenu, cmdID, MF_BYCOMMAND, cmdID, thePluginCmds[i].toMenuItemString().c_str());
								
								vector<PluginCmdShortcut> & theModifyedPluginCmds = (NppParameters::getInstance())->getPluginCustomizedCmds();
								
								int k = -1;
								for (size_t j = 0 ; j < theModifyedPluginCmds.size() ; j++)
								{
									if (cmdID == theModifyedPluginCmds[j].getID())
									{										
										k = j;
										break;
									}
								}
								if (k != -1)
									theModifyedPluginCmds[k].copyShortcut(thePluginCmds[i]);
								else
									theModifyedPluginCmds.push_back(thePluginCmds[i]);

								::SendMessage(_hParent, NPPM_INTERNAL_PLUGINCMDLIST_MODIFIED, 0, 0);
							}
							_babygrid.setText(row, 1, shortcut._name);
						}
					}
					else // Scintilla Key shortcut
					{
						vector<ScintillaKeyMap> & scintillaShortcuts = nppParam->getScintillaKeyList();
						const int index = row - _pAccel->_nbAccelItems - 1;
						ScintillaKeyMap scintillaSc = scintillaShortcuts[index];
						scintillaSc.init(_hInst, _hSelf);
						scintillaSc.setNameReadOnly(false);

						if (scintillaSc.doDialog() != -1)
						{
							// Add this modification into the list to save it later
							bool found = false;
							vector<ScintillaKeyMap> & userDefScintillaKeys = nppParam->getScintillaModifiedKeys();
							for (size_t i = 0 ; i < userDefScintillaKeys.size() ; i++)
							{
								if (scintillaSc.getID() == userDefScintillaKeys[i].getID())
								{
									userDefScintillaKeys[i].copyShortcut(scintillaSc);
									found = true;
								}
							}
							if (!found)
								userDefScintillaKeys.push_back(scintillaSc);

							// remap the key
							::SendMessage(_hParent, NPPM_INTERNAL_BINDSCINTILLAKEY, scintillaSc.toKeyDef(), scintillaSc.getScintillaKey());
							::SendMessage(_hParent, NPPM_INTERNAL_CLEARSCINTILLAKEY, scintillaShortcuts[index].toKeyDef(), 0);
							
							// update the key in the scintilla key list
							scintillaShortcuts[index].copyShortcut(scintillaSc);

							// if this shortcut is linked to a menu item, change to shortcut string in menu item
							if (int cmdID = scintillaShortcuts[index].getMenuCmdID())
							{
								HMENU hMenu = ::GetMenu(_hParent);
								::ModifyMenu(hMenu, cmdID, MF_BYCOMMAND, cmdID, scintillaShortcuts[index].toMenuItemString(cmdID).c_str());
							}

							// Update UI
							_babygrid.setText(row, 2, scintillaSc.toString().c_str());

							::SendMessage(_hParent, NPPM_INTERNAL_SCINTILLAKEYMODIFIED, 0, 0);
						}
					}
					return TRUE;
				}
				case IDM_BABYGRID_DELETE :
				{
					NppParameters *nppParam = NppParameters::getInstance();
					if (::MessageBox(_hSelf, "Are you sure to delete this shortcut?", "Are you sure?", MB_OKCANCEL) == IDOK)
					{
						const int row = _babygrid.getSelectedRow();
						DWORD cmdID = _pAccel->_pAccelArray[row-1].cmd;
						
						// Menu data
						int erasePos;
						size_t posBase;
						size_t nbElem;
						HMENU hMenu;

						if ((cmdID >= ID_MACRO) && (cmdID < ID_MACRO_LIMIT))
						{
							vector<MacroShortcut> & theMacros = nppParam->getMacroList();
							vector<MacroShortcut>::iterator it = theMacros.begin();
							erasePos = cmdID - ID_MACRO;
							theMacros.erase(it + erasePos);
							_pAccel->uptdateShortcuts();
							_babygrid.clear();
							fillOutBabyGrid();
							
							// preparing to remove from menu
							posBase = 3;
							nbElem = theMacros.size();
							hMenu = ::GetSubMenu(::GetMenu(_hParent), MENUINDEX_MACRO);

							::SendMessage(_hParent, NPPM_INTERNAL_MACROLIST_MODIFIED, 0, 0);
						}
						else if ((cmdID >= ID_USER_CMD) && (cmdID < ID_USER_CMD_LIMIT))
						{
							vector<UserCommand> & theUserCmds = nppParam->getUserCommandList();
							vector<UserCommand>::iterator it = theUserCmds.begin();
							erasePos = cmdID - ID_USER_CMD;
							theUserCmds.erase(it + erasePos);
							_pAccel->uptdateShortcuts();
							_babygrid.clear();
							fillOutBabyGrid();
							
							// preparing to remove from menu
							posBase = 0;
							nbElem = theUserCmds.size();
							hMenu = ::GetSubMenu(::GetMenu(_hParent), MENUINDEX_RUN);
							
							::SendMessage(_hParent, NPPM_INTERNAL_USERCMDLIST_MODIFIED, 0, 0);
						}
						else // should never happen
						{
							return FALSE;
						}

						// remove from menu
						::RemoveMenu(hMenu, cmdID++, MF_BYCOMMAND);

						for (size_t i = erasePos ; i < nbElem ; i++)
						{
							char cmdName[64];
							::GetMenuString(hMenu, cmdID, cmdName, sizeof(cmdName), MF_BYCOMMAND);
							::ModifyMenu(hMenu, cmdID, MF_BYCOMMAND, cmdID-1, cmdName);
							cmdID++;
						}
						if (nbElem == 0)
							::RemoveMenu(hMenu, posBase + 1, MF_BYPOSITION);

					}
					return TRUE;
				}
				default :
					if (LOWORD(wParam) == IDD_BABYGRID_ID1)
					{
						if(HIWORD(wParam) == BGN_CELLDBCLICKED) //a cell was clicked in the properties grid
						{
							return ::SendMessage(_hSelf, WM_COMMAND, IDM_BABYGRID_MODIFY, LOWORD(lParam));
						}
						else if(HIWORD(wParam) == BGN_CELLRCLICKED) //a cell was clicked in the properties grid
						{
							int row = LOWORD(lParam);
							DWORD cmdID = _pAccel->_pAccelArray[row-1].cmd;

							POINT p;
							::GetCursorPos(&p);
							if (!_rightClickMenu.isCreated())
							{
								vector<MenuItemUnit> itemUnitArray;
								itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_MODIFY, "Modify"));
								itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_DELETE, "Delete"));
								_rightClickMenu.create(_hSelf, itemUnitArray);
							}
							bool b = (!((cmdID >= ID_MACRO) && (cmdID < ID_MACRO_LIMIT))) && (!((cmdID >= ID_USER_CMD) && (cmdID < ID_USER_CMD_LIMIT)));
							_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, !b);
							_rightClickMenu.display(p);
							return TRUE;
						}
					}
			}
		}
		default:
			return FALSE;
	}
	return FALSE;
}
