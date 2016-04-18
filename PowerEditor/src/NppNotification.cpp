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



#include "Notepad_plus_Window.h"
#include "xmlMatchedTagsHighlighter.h"
#include "VerticalFileSwitcher.h"
#include "ProjectPanel.h"
#include "documentMap.h"
#include <stack>

using namespace std;





BOOL Notepad_plus::notify(SCNotification *notification)
{
	//Important, keep track of which element generated the message
	bool isFromPrimary = (_mainEditView.getHSelf() == notification->nmhdr.hwndFrom || _mainDocTab.getHSelf() == notification->nmhdr.hwndFrom);
	bool isFromSecondary = !isFromPrimary && (_subEditView.getHSelf() == notification->nmhdr.hwndFrom || _subDocTab.getHSelf() == notification->nmhdr.hwndFrom);
	ScintillaEditView * notifyView = isFromPrimary?&_mainEditView:&_subEditView;
	DocTabView *notifyDocTab = isFromPrimary?&_mainDocTab:&_subDocTab;
	TBHDR * tabNotification = (TBHDR*) notification;
	switch (notification->nmhdr.code)
	{
		case SCN_MODIFIED:
		{
			static bool prevWasEdit = false;
			if (notification->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
			{
				_pEditView->updateBeginEndSelectPosition(notification->modificationType & SC_MOD_INSERTTEXT, notification->position, notification->length);
				prevWasEdit = true;
				_linkTriggered = true;
				::InvalidateRect(notifyView->getHSelf(), NULL, TRUE);
			}

			if (notification->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT | SC_PERFORMED_UNDO | SC_PERFORMED_REDO))
			{
				// for the backup system
				_pEditView->getCurrentBuffer()->setModifiedStatus(true);
			}

			if (notification->modificationType & SC_MOD_CHANGEFOLD)
			{
				if (prevWasEdit)
				{
					notifyView->foldChanged(notification->line, notification->foldLevelNow, notification->foldLevelPrev);
					prevWasEdit = false;
				}
			}
			else if (!(notification->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT)))
			{
				prevWasEdit = false;
			}

			break;
		}

		case SCN_SAVEPOINTREACHED:
		case SCN_SAVEPOINTLEFT:
		{
			Buffer * buf = 0;
			if (isFromPrimary)
			{
				buf = _mainEditView.getCurrentBuffer();
			}
			else if (isFromSecondary)
			{
				buf = _subEditView.getCurrentBuffer();
			}
			else
			{
				//Done by invisibleEditView?
				BufferID id = BUFFER_INVALID;
				if (notification->nmhdr.hwndFrom == _invisibleEditView.getHSelf())
				{
					id = MainFileManager->getBufferFromDocument(_invisibleEditView.execute(SCI_GETDOCPOINTER));
				}
				else if (notification->nmhdr.hwndFrom == _fileEditView.getHSelf())
				{
					id = MainFileManager->getBufferFromDocument(_fileEditView.execute(SCI_GETDOCPOINTER));
				}
				else
					break;	//wrong scintilla

				if (id != BUFFER_INVALID)
				{
					buf = MainFileManager->getBufferByID(id);
				}
				else
					break;
			}

			bool isDirty = notification->nmhdr.code == SCN_SAVEPOINTLEFT;
			bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
			if (isSnapshotMode && !isDirty)
			{
				bool canUndo = _pEditView->execute(SCI_CANUNDO) == TRUE;
				if (!canUndo && buf->isLoadedDirty() && buf->isDirty())
					isDirty = true;
			}
			buf->setDirty(isDirty);
			break;
		}

		case SCN_MODIFYATTEMPTRO:
		{
			// nothing to do
			break;
		}

		case SCN_KEY:
		{
			break;
		}

		case TCN_TABDROPPEDOUTSIDE:
		case TCN_TABDROPPED:
		{
			TabBarPlus *sender = reinterpret_cast<TabBarPlus *>(notification->nmhdr.idFrom);
			bool isInCtrlStat = (::GetKeyState(VK_LCONTROL) & 0x80000000) != 0;
			if (notification->nmhdr.code == TCN_TABDROPPEDOUTSIDE)
			{
				POINT p = sender->getDraggingPoint();

				//It's the coordinate of screen, so we can call
				//"WindowFromPoint" function without converting the point
				HWND hWin = ::WindowFromPoint(p);
				if (hWin == _pEditView->getHSelf()) // In the same view group
				{
					if (!_tabPopupDropMenu.isCreated())
					{
						TCHAR goToView[32] = TEXT("Move to other view");
						TCHAR cloneToView[32] = TEXT("Clone to other View");
						vector<MenuItemUnit> itemUnitArray;
						itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_ANOTHER_VIEW, goToView));
						itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, cloneToView));
						_tabPopupDropMenu.create(_pPublicInterface->getHSelf(), itemUnitArray, _mainMenuHandle);
						_nativeLangSpeaker.changeLangTabDrapContextMenu(_tabPopupDropMenu.getMenuHandle());
					}
					_tabPopupDropMenu.display(p);
				}
				else if ((hWin == _pNonDocTab->getHSelf()) ||
						 (hWin == _pNonEditView->getHSelf())) // In the another view group
				{
					docGotoAnotherEditView(isInCtrlStat?TransferClone:TransferMove);
				}
				else
				{
					RECT nppZone;
					::GetWindowRect(_pPublicInterface->getHSelf(), &nppZone);
					bool isInNppZone = (((p.x >= nppZone.left) && (p.x <= nppZone.right)) && (p.y >= nppZone.top) && (p.y <= nppZone.bottom));
					if (isInNppZone)
					{
						// Do nothing
						return TRUE;
					}
					generic_string quotFileName = TEXT("\"");
					quotFileName += _pEditView->getCurrentBuffer()->getFullPathName();
					quotFileName += TEXT("\"");
					COPYDATASTRUCT fileNamesData;
					fileNamesData.dwData = COPYDATA_FILENAMES;
					fileNamesData.lpData = (void *)quotFileName.c_str();
					fileNamesData.cbData = long(quotFileName.length() + 1)*(sizeof(TCHAR));

					HWND hWinParent = ::GetParent(hWin);
					const rsize_t classNameBufferSize = MAX_PATH;
					TCHAR className[classNameBufferSize];
					::GetClassName(hWinParent,className, classNameBufferSize);
					if (lstrcmp(className, _pPublicInterface->getClassName()) == 0 && hWinParent != _pPublicInterface->getHSelf()) // another Notepad++
					{
						int index = _pDocTab->getCurrentTabIndex();
						BufferID bufferToClose = notifyDocTab->getBufferByIndex(index);
						Buffer * buf = MainFileManager->getBufferByID(bufferToClose);
						int iView = isFromPrimary?MAIN_VIEW:SUB_VIEW;
						if (buf->isDirty())
						{
							generic_string msg, title;
							_nativeLangSpeaker.messageBox("CannotMoveDoc",
								_pPublicInterface->getHSelf(),
								TEXT("Document is modified, save it then try again."),
								TEXT("Move to new Notepad++ Instance"),
								MB_OK);
						}
						else
						{
							::SendMessage(hWinParent, NPPM_INTERNAL_SWITCHVIEWFROMHWND, 0, (LPARAM)hWin);
							::SendMessage(hWinParent, WM_COPYDATA, (WPARAM)_pPublicInterface->getHinst(), (LPARAM)&fileNamesData);
							if (!isInCtrlStat)
							{
								fileClose(bufferToClose, iView);
								if (noOpenedDoc())
									::SendMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
							}
						}
					}
					else // Not Notepad++, we open it here
					{
						docOpenInNewInstance(isInCtrlStat?TransferClone:TransferMove, p.x, p.y);
					}
				}
			}
			//break;
			sender->resetDraggingPoint();
			return TRUE;
		}

		case TCN_TABDELETE:
		{
			int index = tabNotification->tabOrigin;
			BufferID bufferToClose = notifyDocTab->getBufferByIndex(index);
			Buffer * buf = MainFileManager->getBufferByID(bufferToClose);
			int iView = isFromPrimary?MAIN_VIEW:SUB_VIEW;
			if (buf->isDirty())
			{
				activateBuffer(bufferToClose, iView);
			}

			if (fileClose(bufferToClose, iView))
				checkDocState();

			break;
		}

		case TCN_SELCHANGE:
		{
			int iView = -1;
			if (notification->nmhdr.hwndFrom == _mainDocTab.getHSelf())
			{
				iView = MAIN_VIEW;
			}
			else if (notification->nmhdr.hwndFrom == _subDocTab.getHSelf())
			{
				iView = SUB_VIEW;
			}
			else
				break;

			switchEditViewTo(iView);
			BufferID bufid = _pDocTab->getBufferByIndex(_pDocTab->getCurrentTabIndex());
			if (bufid != BUFFER_INVALID)
			{
				_isFolding = true; // So we can ignore events while folding is taking place
				activateBuffer(bufid, iView);
				_isFolding = false;
			}
			break;
		}

		case NM_CLICK :
		{
			if (notification->nmhdr.hwndFrom == _statusBar.getHSelf())
			{
				LPNMMOUSE lpnm = (LPNMMOUSE)notification;
				if (lpnm->dwItemSpec == DWORD(STATUSBAR_TYPING_MODE))
				{
					bool isOverTypeMode = (_pEditView->execute(SCI_GETOVERTYPE) != 0);
					_pEditView->execute(SCI_SETOVERTYPE, !isOverTypeMode);
					_statusBar.setText((_pEditView->execute(SCI_GETOVERTYPE))?TEXT("OVR"):TEXT("INS"), STATUSBAR_TYPING_MODE);
				}
			}
			else if (notification->nmhdr.hwndFrom == _mainDocTab.getHSelf() && _activeView == SUB_VIEW)
			{
				bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
				if (isSnapshotMode)
				{
					// Before switching off, synchronize backup file
					MainFileManager->backupCurrentBuffer();
				}
				// Switch off
				switchEditViewTo(MAIN_VIEW);
			}
			else if (notification->nmhdr.hwndFrom == _subDocTab.getHSelf() && _activeView == MAIN_VIEW)
			{
				bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
				if (isSnapshotMode)
				{
					// Before switching off, synchronize backup file
					MainFileManager->backupCurrentBuffer();
				}
				// Switch off
				switchEditViewTo(SUB_VIEW);
			}

			break;
		}

		case NM_DBLCLK :
		{
			if (notification->nmhdr.hwndFrom == _statusBar.getHSelf())
			{
				LPNMMOUSE lpnm = (LPNMMOUSE)notification;
				if (lpnm->dwItemSpec == DWORD(STATUSBAR_CUR_POS))
				{
					bool isFirstTime = !_goToLineDlg.isCreated();
					_goToLineDlg.doDialog(_nativeLangSpeaker.isRTL());
					if (isFirstTime)
						_nativeLangSpeaker.changeDlgLang(_goToLineDlg.getHSelf(), "GoToLine");
				}
				else if (lpnm->dwItemSpec == DWORD(STATUSBAR_DOC_SIZE))
				{
					command(IDM_VIEW_SUMMARY);
				}
				else if (lpnm->dwItemSpec == DWORD(STATUSBAR_DOC_TYPE))
				{
					POINT p;
					::GetCursorPos(&p);
					HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);
					TrackPopupMenu(hLangMenu, 0, p.x, p.y, 0, _pPublicInterface->getHSelf(), NULL);
				}
				else if (lpnm->dwItemSpec == DWORD(STATUSBAR_EOF_FORMAT))
				{
					POINT p;
					::GetCursorPos(&p);
					MenuPosition & menuPos = getMenuPosition("edit-eolConversion");
					HMENU hEditMenu = ::GetSubMenu(_mainMenuHandle, menuPos._x);
					if (!hEditMenu)
						return TRUE;
					HMENU hEolFormatMenu = ::GetSubMenu(hEditMenu, menuPos._y);
					if (!hEolFormatMenu)
						return TRUE;
					TrackPopupMenu(hEolFormatMenu, 0, p.x, p.y, 0, _pPublicInterface->getHSelf(), NULL);
				}
			}
			break;
		}

		case NM_RCLICK :
		{
			POINT p;
			::GetCursorPos(&p);

			if (notification->nmhdr.hwndFrom == _mainDocTab.getHSelf())
			{
				switchEditViewTo(MAIN_VIEW);
			}
			else if (notification->nmhdr.hwndFrom == _subDocTab.getHSelf())
			{
				switchEditViewTo(SUB_VIEW);
			}
			else if (notification->nmhdr.hwndFrom == _statusBar.getHSelf())  // From Status Bar
			{
				LPNMMOUSE lpnm = (LPNMMOUSE)notification;
				if (lpnm->dwItemSpec == DWORD(STATUSBAR_DOC_TYPE))
				{
					POINT p;
					::GetCursorPos(&p);
					HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);
					TrackPopupMenu(hLangMenu, 0, p.x, p.y, 0, _pPublicInterface->getHSelf(), NULL);
				}
				else if (lpnm->dwItemSpec == DWORD(STATUSBAR_EOF_FORMAT))
				{
					POINT p;
					::GetCursorPos(&p);
					MenuPosition & menuPos = getMenuPosition("edit-eolConversion");
					HMENU hEditMenu = ::GetSubMenu(_mainMenuHandle, menuPos._x);
					if (!hEditMenu)
						return TRUE;
					HMENU hEolFormatMenu = ::GetSubMenu(hEditMenu, menuPos._y);
					if (!hEolFormatMenu)
						return TRUE;
					TrackPopupMenu(hEolFormatMenu, 0, p.x, p.y, 0, _pPublicInterface->getHSelf(), NULL);
				}
				return TRUE;
			}
			else if (_pFileSwitcherPanel && notification->nmhdr.hwndFrom == _pFileSwitcherPanel->getHSelf())
			{
				// Already switched, so do nothing here.

				if (_pFileSwitcherPanel->nbSelectedFiles() > 1)
				{
					if (!_fileSwitcherMultiFilePopupMenu.isCreated())
					{
						vector<MenuItemUnit> itemUnitArray;
						itemUnitArray.push_back(MenuItemUnit(IDM_FILESWITCHER_FILESCLOSE, TEXT("Close Selected files")));
						itemUnitArray.push_back(MenuItemUnit(IDM_FILESWITCHER_FILESCLOSEOTHERS, TEXT("Close others files")));

						_fileSwitcherMultiFilePopupMenu.create(_pPublicInterface->getHSelf(), itemUnitArray);
						_nativeLangSpeaker.changeLangTabContextMenu(_fileSwitcherMultiFilePopupMenu.getMenuHandle());
					}
					_fileSwitcherMultiFilePopupMenu.display(p);
					return TRUE;
				}
			}
			else // From tool bar
				return TRUE;
			//break;

			if (!_tabPopupMenu.isCreated())
			{
				std::vector<MenuItemUnit> itemUnitArray;
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSE, TEXT("Close")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_BUT_CURRENT, TEXT("Close All BUT This")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_TOLEFT, TEXT("Close All to the Left")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_TORIGHT, TEXT("Close All to the Right")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_SAVE, TEXT("Save")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_SAVEAS, TEXT("Save As...")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_RENAME, TEXT("Rename")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_DELETE, TEXT("Move to Recycle Bin")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_RELOAD, TEXT("Reload")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_PRINT, TEXT("Print")));
				itemUnitArray.push_back(MenuItemUnit(0, NULL));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_OPEN_FOLDER, TEXT("Open Containing Folder in Explorer")));
				itemUnitArray.push_back(MenuItemUnit(IDM_FILE_OPEN_CMD, TEXT("Open Containing Folder in cmd")));
				itemUnitArray.push_back(MenuItemUnit(0, NULL));
				itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_SETREADONLY,   TEXT("Read-Only")));
				itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_CLEARREADONLY, TEXT("Clear Read-Only Flag")));
				itemUnitArray.push_back(MenuItemUnit(0, NULL));
				itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_FULLPATHTOCLIP,   TEXT("Full File Path to Clipboard")));
				itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_FILENAMETOCLIP,   TEXT("Filename to Clipboard")));
				itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_CURRENTDIRTOCLIP, TEXT("Current Dir. Path to Clipboard")));
				itemUnitArray.push_back(MenuItemUnit(0, NULL));
				itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_ANOTHER_VIEW, TEXT("Move to Other View")));
				itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, TEXT("Clone to Other View")));
				itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_NEW_INSTANCE, TEXT("Move to New Instance")));
				itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_LOAD_IN_NEW_INSTANCE, TEXT("Open in New Instance")));

				_tabPopupMenu.create(_pPublicInterface->getHSelf(), itemUnitArray);
				_nativeLangSpeaker.changeLangTabContextMenu(_tabPopupMenu.getMenuHandle());
			}

			bool isEnable = ((::GetMenuState(_mainMenuHandle, IDM_FILE_SAVE, MF_BYCOMMAND)&MF_DISABLED) == 0);
			_tabPopupMenu.enableItem(IDM_FILE_SAVE, isEnable);

			Buffer * buf = _pEditView->getCurrentBuffer();
			bool isUserReadOnly = buf->getUserReadOnly();
			_tabPopupMenu.checkItem(IDM_EDIT_SETREADONLY, isUserReadOnly);

			bool isSysReadOnly = buf->getFileReadOnly();
			_tabPopupMenu.enableItem(IDM_EDIT_SETREADONLY, !isSysReadOnly);
			_tabPopupMenu.enableItem(IDM_EDIT_CLEARREADONLY, isSysReadOnly);

			bool isFileExisting = PathFileExists(buf->getFullPathName()) != FALSE;
			_tabPopupMenu.enableItem(IDM_FILE_DELETE, isFileExisting);
			_tabPopupMenu.enableItem(IDM_FILE_RENAME, isFileExisting);

			bool isDirty = buf->isDirty();
			bool isUntitled = buf->isUntitled();
			_tabPopupMenu.enableItem(IDM_VIEW_GOTO_NEW_INSTANCE, !(isDirty||isUntitled));
			_tabPopupMenu.enableItem(IDM_VIEW_LOAD_IN_NEW_INSTANCE, !(isDirty||isUntitled));

			_tabPopupMenu.display(p);
			return TRUE;
		}


		case SCN_MARGINCLICK:
		{
			if (notification->nmhdr.hwndFrom == _mainEditView.getHSelf())
				switchEditViewTo(MAIN_VIEW);
			else if (notification->nmhdr.hwndFrom == _subEditView.getHSelf())
				switchEditViewTo(SUB_VIEW);

			int lineClick = int(_pEditView->execute(SCI_LINEFROMPOSITION, notification->position));

			if (notification->margin == ScintillaEditView::_SC_MARGE_FOLDER)
			{
				_pEditView->marginClick(notification->position, notification->modifiers);
				if (_pDocMap)
					_pDocMap->fold(lineClick, _pEditView->isFolded(lineClick));
			}
			else if ((notification->margin == ScintillaEditView::_SC_MARGE_SYBOLE) && !notification->modifiers)
			{
				if (!_pEditView->markerMarginClick(lineClick))
					bookmarkToggle(lineClick);
			}
			break;
		}

		case SCN_FOLDINGSTATECHANGED :
		{
			if ((notification->nmhdr.hwndFrom == _mainEditView.getHSelf()) || (notification->nmhdr.hwndFrom == _subEditView.getHSelf()))
			{
				int lineClicked = notification->line;

				if (!_isFolding)
				{
					int urlAction = (NppParameters::getInstance())->getNppGUI()._styleURL;
					if ((urlAction == 1) || (urlAction == 2))
						addHotSpot();
				}

				if (_pDocMap)
					_pDocMap->fold(lineClicked, _pEditView->isFolded(lineClicked));
			}
			return TRUE;
		}

		case SCN_CHARADDED:
		{
			if (!_recordingMacro && !_playingBackMacro) // No macro recording or playing back
			{
				const NppGUI & nppGui = NppParameters::getInstance()->getNppGUI();
				bool indentMaintain = nppGui._maitainIndent;
				if (indentMaintain)
					maintainIndentation(static_cast<TCHAR>(notification->ch));

				AutoCompletion * autoC = isFromPrimary ? &_autoCompleteMain : &_autoCompleteSub;
				bool isColumnMode = _pEditView->execute(SCI_GETSELECTIONS) > 1; // Multi-Selection || Column mode)
				if (nppGui._matchedPairConf.hasAnyPairsPair() && !isColumnMode)
					autoC->insertMatchedChars(notification->ch, nppGui._matchedPairConf);
				autoC->update(notification->ch);
			}
			break;
		}

		case SCN_DOUBLECLICK:
		{
			if (notification->modifiers == SCMOD_CTRL)
			{
				const NppGUI & nppGUI = NppParameters::getInstance()->getNppGUI();

				std::string bufstring;

				unsigned int position_of_click;
				// For some reason Ctrl+DoubleClick on an empty line means that notification->position == 1.
				// In that case we use SCI_GETCURRENTPOS to get the position.
				if (notification->position != -1)
					position_of_click = notification->position;
				else
					position_of_click = int(_pEditView->execute(SCI_GETCURRENTPOS));

				// Anonymous scope to limit use of the buf pointer (much easier to deal with std::string).
				{
					char *buf;
					int length;

					if (nppGUI._delimiterSelectionOnEntireDocument)
					{
						// Get entire document.
						length = notifyView->execute(SCI_GETLENGTH);
						buf = new char[length + 1];
						notifyView->execute(SCI_GETTEXT, (LPARAM)(length + 1), (WPARAM)buf);
					}
					else
					{
						// Get single line.
						length = notifyView->execute(SCI_GETCURLINE);
						buf = new char[length + 1];
						notifyView->execute(SCI_GETCURLINE, (WPARAM)length, (LPARAM)buf);

						// Compute the position of the click (relative to the beginning of the line).
						const int line_position = notifyView->execute(SCI_POSITIONFROMLINE, notifyView->getCurrentLineNumber());
						position_of_click = position_of_click - line_position;
					}

					bufstring = buf;
					delete [] buf;
				}

				int leftmost_position = -1;
				int rightmost_position = -1;

				if (nppGUI._rightmostDelimiter == nppGUI._leftmostDelimiter)
				{
					// If the delimiters are the same (e.g. they are both a quotation mark), choose the ones
					// which are closest to the clicked position.
					for (int i = position_of_click; i >= 0; --i)
					{
						if (bufstring.at(i) == nppGUI._leftmostDelimiter)
						{
							// Respect escaped quotation marks.
							if (nppGUI._leftmostDelimiter == '"')
							{
								if (! (i > 0 && bufstring.at(i - 1) == '\\'))
								{
									leftmost_position = i;
									break;
								}
							}
							else
							{
								leftmost_position = i;
								break;
							}
						}
					}

					if (leftmost_position == -1)
						break;

					// Scan for right delimiter.
					for (unsigned int i = position_of_click; i < bufstring.length(); ++i)
					{
						if (bufstring.at(i) == nppGUI._rightmostDelimiter)
						{
							// Respect escaped quotation marks.
							if (nppGUI._rightmostDelimiter == '"')
							{
								if (! (i > 0 && bufstring.at(i - 1) == '\\'))
								{
									rightmost_position = i;
									break;
								}
							}
							else
							{
								rightmost_position = i;
								break;
							}
						}
					}
				}
				else
				{
					// Find matching pairs of delimiters (e.g. parantheses).
					// The pair where the distance from the left delimiter to position_of_click is at a minimum is the one we're looking for.
					// Of course position_of_click must lie between the delimiters.

					// This logic is required to handle cases like this:
					// (size_t i = function(); i < _buffers.size(); i++)

					std::stack<unsigned int> leftmost_delimiter_positions;

					for (unsigned int i = 0; i < bufstring.length(); ++i)
					{
						if (bufstring.at(i) == nppGUI._leftmostDelimiter)
							leftmost_delimiter_positions.push(i);
						else if (bufstring.at(i) == nppGUI._rightmostDelimiter && ! leftmost_delimiter_positions.empty())
						{
							unsigned int matching_leftmost = leftmost_delimiter_positions.top();
							leftmost_delimiter_positions.pop();

							// We have either 1) chosen neither the left- or rightmost position, or 2) chosen both left- and rightmost position.
							assert( (leftmost_position == -1 && rightmost_position == -1) || (leftmost_position >= 0 && rightmost_position >= 0) );

							// Note: cast of leftmost_position to unsigned int is safe, since if leftmost_position is not -1 then it is guaranteed to be positive.
							// If it was possible, leftmost_position and rightmost_position should be of type optional<unsigned int>.
							if ( matching_leftmost <= position_of_click && i >= position_of_click &&  (leftmost_position == -1 ||  matching_leftmost > (unsigned int)leftmost_position) )
							{
								leftmost_position = matching_leftmost;
								rightmost_position = i;
							}
						}
					}
				}

				// Set selection to the position we found (if any).
				if (rightmost_position != -1 && leftmost_position != -1)
				{
					if (nppGUI._delimiterSelectionOnEntireDocument)
					{
						notifyView->execute(SCI_SETCURRENTPOS, rightmost_position);
						notifyView->execute(SCI_SETANCHOR, leftmost_position + 1);
					}
					else
					{
						const int line_position = notifyView->execute(SCI_POSITIONFROMLINE, notifyView->getCurrentLineNumber());
						notifyView->execute(SCI_SETCURRENTPOS, line_position + rightmost_position);
						notifyView->execute(SCI_SETANCHOR, line_position + leftmost_position + 1);
					}
				}
			}
			else if (_isHotspotDblClicked)
			{
				int pos = notifyView->execute(SCI_GETCURRENTPOS);
				notifyView->execute(SCI_SETCURRENTPOS, pos);
				notifyView->execute(SCI_SETANCHOR, pos);
				_isHotspotDblClicked = false;
			}

			break;
		}

		case SCN_UPDATEUI:
		{
			NppParameters *nppParam = NppParameters::getInstance();

			// replacement for obsolete custom SCN_SCROLLED
			if (notification->updated & SC_UPDATE_V_SCROLL)
			{
				int urlAction = (NppParameters::getInstance())->getNppGUI()._styleURL;
				if ((urlAction == 1) || (urlAction == 2))
					addHotSpot();
			}

			// if it's searching/replacing, then do nothing
			if (nppParam->_isFindReplacing)
				break;

			if (notification->nmhdr.hwndFrom != _pEditView->getHSelf())
				break;

			braceMatch();

			NppGUI & nppGui = (NppGUI &)nppParam->getNppGUI();

			if (nppGui._enableTagsMatchHilite)
			{
				XmlMatchedTagsHighlighter xmlTagMatchHiliter(_pEditView);
				xmlTagMatchHiliter.tagMatch(nppGui._enableTagAttrsHilite);
			}

			if (nppGui._enableSmartHilite)
			{
				if (nppGui._disableSmartHiliteTmp)
					nppGui._disableSmartHiliteTmp = false;
				else
					_smartHighlighter.highlightView(notifyView);
			}

			updateStatusBar();
			AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
			autoC->update(0);

			break;
		}

		case TTN_GETDISPINFO:
		{
			try
			{
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)notification;

				//Joce's fix
				lpttt->hinst = NULL;

				POINT p;
				::GetCursorPos(&p);
				::ScreenToClient(_pPublicInterface->getHSelf(), &p);
				HWND hWin = ::RealChildWindowFromPoint(_pPublicInterface->getHSelf(), p);
				const int tipMaxLen = 1024;
				static TCHAR docTip[tipMaxLen];
				docTip[0] = '\0';

				generic_string tipTmp(TEXT(""));
				int id = int(lpttt->hdr.idFrom);

				if (hWin == _rebarTop.getHSelf())
				{
					getNameStrFromCmd(id, tipTmp);
					if (tipTmp.length() >= 80)
						return FALSE;

					lstrcpy(lpttt->szText, tipTmp.c_str());
					return TRUE;
				}
				else if (hWin == _mainDocTab.getHSelf())
				{
					BufferID idd = _mainDocTab.getBufferByIndex(id);
					Buffer * buf = MainFileManager->getBufferByID(idd);
					tipTmp = buf->getFullPathName();

					if (tipTmp.length() >= tipMaxLen)
						return FALSE;
					lstrcpy(docTip, tipTmp.c_str());
					lpttt->lpszText = docTip;
					return TRUE;
				}
				else if (hWin == _subDocTab.getHSelf())
				{
					BufferID idd = _subDocTab.getBufferByIndex(id);
					Buffer * buf = MainFileManager->getBufferByID(idd);
					tipTmp = buf->getFullPathName();

					if (tipTmp.length() >= tipMaxLen)
						return FALSE;
					lstrcpy(docTip, tipTmp.c_str());
					lpttt->lpszText = docTip;
					return TRUE;
				}
				else
					return FALSE;
			}
			catch (...)
			{
				//printStr(TEXT("ToolTip crash is caught!"));
			}
			break;
		}


		case SCN_ZOOM:
		{
			break;
		}

		case SCN_MACRORECORD:
		{
			_macro.push_back(recordedMacroStep(notification->message, notification->wParam, notification->lParam, _pEditView->execute(SCI_GETCODEPAGE)));
			break;
		}

		case SCN_PAINTED:
		{
			//--FLS: ViewMoveAtWrappingDisableFix: Disable wrapping messes up visible lines. Therefore save view position before in IDM_VIEW_WRAP and restore after SCN_PAINTED, as doc. says
			if (_mainEditView.isWrapRestoreNeeded())
			{
				_mainEditView.restoreCurrentPos();
				_mainEditView.setWrapRestoreNeeded(false);
			}

			if (_subEditView.isWrapRestoreNeeded())
			{
				_subEditView.restoreCurrentPos();
				_subEditView.setWrapRestoreNeeded(false);
			}
			notifyView->updateLineNumberWidth();
			if (_syncInfo.doSync())
				doSynScorll(HWND(notification->nmhdr.hwndFrom));

			NppParameters *nppParam = NppParameters::getInstance();

			// if it's searching/replacing, then do nothing
			if ((_linkTriggered && !nppParam->_isFindReplacing) || notification->wParam == LINKTRIGGERED)
			{
				int urlAction = (NppParameters::getInstance())->getNppGUI()._styleURL;
				if ((urlAction == 1) || (urlAction == 2))
					addHotSpot();
				_linkTriggered = false;
			}

			if (_pDocMap)
			{
				_pDocMap->wrapMap();
				_pDocMap->scrollMap();
			}
			break;
		}

		case SCN_HOTSPOTDOUBLECLICK:
		{
			notifyView->execute(SCI_SETWORDCHARS, 0, (LPARAM)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-+*.,:;!?$&@=/%#()");
		
			int pos = notifyView->execute(SCI_GETCURRENTPOS);
			int startPos = static_cast<int>(notifyView->execute(SCI_WORDSTARTPOSITION, pos, false));
			int endPos = static_cast<int>(notifyView->execute(SCI_WORDENDPOSITION, pos, false));

			notifyView->execute(SCI_SETTARGETRANGE, startPos, endPos);

			int posFound = notifyView->execute(SCI_SEARCHINTARGET, strlen(URL_REG_EXPR), (LPARAM)URL_REG_EXPR);
			if (posFound != -2)
			{
				if (posFound != -1)
				{
					startPos = int(notifyView->execute(SCI_GETTARGETSTART));
					endPos = int(notifyView->execute(SCI_GETTARGETEND));
				}

				// Prevent buffer overflow in getGenericText().
				if(endPos - startPos > 2*MAX_PATH)
					endPos = startPos + 2*MAX_PATH;

				TCHAR currentWord[2*MAX_PATH];

				notifyView->getGenericText(currentWord, MAX_PATH*2, startPos, endPos);

				// This treatment would fail on some valid URLs where there's actually supposed to be a comma or parenthesis at the end.
				int lastCharIndex = _tcsnlen(currentWord, MAX_PATH*2) - 1;
				if(lastCharIndex >= 0 && (currentWord[lastCharIndex] == ',' || currentWord[lastCharIndex] == ')' || currentWord[lastCharIndex] == '('))
					currentWord[lastCharIndex] = '\0';

				::ShellExecute(_pPublicInterface->getHSelf(), TEXT("open"), currentWord, NULL, NULL, SW_SHOW);
				_isHotspotDblClicked = true;
				notifyView->execute(SCI_SETCHARSDEFAULT);
			}
			break;
		}

		case SCN_NEEDSHOWN:
		{
			break;
		}

		case SCN_CALLTIPCLICK:
		{
			AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
			autoC->callTipClick(notification->position);
			break;
		}

		case RBN_HEIGHTCHANGE:
		{
			SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			break;
		}

		case RBN_CHEVRONPUSHED:
		{
			NMREBARCHEVRON * lpnm = (NMREBARCHEVRON*) notification;
			ReBar * notifRebar = &_rebarTop;
			if (_rebarBottom.getHSelf() == lpnm->hdr.hwndFrom)
				notifRebar = &_rebarBottom;

			//If N++ ID, use proper object
			if (lpnm->wID == REBAR_BAR_TOOLBAR)
			{
				POINT pt;
				pt.x = lpnm->rc.left;
				pt.y = lpnm->rc.bottom;
				ClientToScreen(notifRebar->getHSelf(), &pt);
				_toolBar.doPopop(pt);
				return TRUE;
			}

			//Else forward notification to window of rebarband
			REBARBANDINFO rbBand;
			ZeroMemory(&rbBand, REBARBAND_SIZE);
			rbBand.cbSize = REBARBAND_SIZE;

			rbBand.fMask = RBBIM_CHILD;
			::SendMessage(notifRebar->getHSelf(), RB_GETBANDINFO, lpnm->uBand, (LPARAM)&rbBand);
			::SendMessage(rbBand.hwndChild, WM_NOTIFY, 0, (LPARAM)lpnm);
			break;
		}

		default:
			break;

	}
	return FALSE;
}

