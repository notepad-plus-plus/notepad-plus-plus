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



#include "Notepad_plus_Window.h"
#include "functionListPanel.h"
#include "xmlMatchedTagsHighlighter.h"
#include "VerticalFileSwitcher.h"
#include "ProjectPanel.h"
#include "documentMap.h"
#include "Common.h"
#include <stack>

using namespace std;

// Only for 2 main Scintilla editors
BOOL Notepad_plus::notify(SCNotification *notification)
{
	//Important, keep track of which element generated the message
	bool isFromPrimary = (_mainEditView.getHSelf() == notification->nmhdr.hwndFrom || _mainDocTab.getHSelf() == notification->nmhdr.hwndFrom);
	bool isFromSecondary = !isFromPrimary && (_subEditView.getHSelf() == notification->nmhdr.hwndFrom || _subDocTab.getHSelf() == notification->nmhdr.hwndFrom);
	
	ScintillaEditView * notifyView = nullptr;
	if (isFromPrimary)
		notifyView = &_mainEditView;
	else if (isFromSecondary)
		notifyView = &_subEditView;

	DocTabView *notifyDocTab = isFromPrimary?&_mainDocTab:&_subDocTab;
	TBHDR * tabNotification = (TBHDR*) notification;
	switch (notification->nmhdr.code)
	{
		case SCN_MODIFIED:
		{
			if (!notifyView) return FALSE;

			if (notification->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
			{
				_pEditView->updateBeginEndSelectPosition(notification->modificationType & SC_MOD_INSERTTEXT, notification->position, notification->length);
				_linkTriggered = true;
				::InvalidateRect(notifyView->getHSelf(), NULL, TRUE);
			}

			if (notification->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT | SC_PERFORMED_UNDO | SC_PERFORMED_REDO))
			{
				// for the backup system
				_pEditView->getCurrentBuffer()->setModifiedStatus(true);
			}

			if (notification->modificationType & SC_MOD_CHANGEINDICATOR)
			{
				::InvalidateRect(notifyView->getHSelf(), NULL, FALSE);
			}
			break;
		}

		case SCN_SAVEPOINTREACHED:
		case SCN_SAVEPOINTLEFT:
		{
			//if (!notifyView) return FALSE; // Could be _invisibleEditView or _fileEditView (see the following code)

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
					id = MainFileManager.getBufferFromDocument(_invisibleEditView.execute(SCI_GETDOCPOINTER));
				}
				else if (notification->nmhdr.hwndFrom == _fileEditView.getHSelf())
				{
					id = MainFileManager.getBufferFromDocument(_fileEditView.execute(SCI_GETDOCPOINTER));
				}
				else
					break;	//wrong scintilla

				if (id != BUFFER_INVALID)
				{
					buf = MainFileManager.getBufferByID(id);
				}
				else
					break;
			}

			bool isDirty = notification->nmhdr.code == SCN_SAVEPOINTLEFT;
			bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
			if (isSnapshotMode && !isDirty)
			{
				bool canUndo = _pEditView->execute(SCI_CANUNDO) == TRUE;
				if (!canUndo && buf->isLoadedDirty() && buf->isDirty())
					isDirty = true;
			}

			if (buf->isUnsync()) // buffer in Notepad++ is not syncronized with the file on disk - in this case the buffer is always dirty 
				isDirty = true;

			if (buf->isSavePointDirty())
				isDirty = true;

			buf->setDirty(isDirty);
			break;
		}

		case SCN_MARGINCLICK:
		{
			if (!notifyView) return FALSE;

			if (notification->nmhdr.hwndFrom == _mainEditView.getHSelf())
				switchEditViewTo(MAIN_VIEW);
			else if (notification->nmhdr.hwndFrom == _subEditView.getHSelf())
				switchEditViewTo(SUB_VIEW);

			intptr_t lineClick = _pEditView->execute(SCI_LINEFROMPOSITION, notification->position);

			if (notification->margin == ScintillaEditView::_SC_MARGE_FOLDER)
			{
				_pEditView->marginClick(notification->position, notification->modifiers);
				if (_pDocMap)
					_pDocMap->fold(lineClick, _pEditView->isFolded(lineClick));

				ScintillaEditView* unfocusView = isFromPrimary ? &_subEditView : &_mainEditView;

				_smartHighlighter.highlightView(_pEditView, unfocusView);
			}
			else if ((notification->margin == ScintillaEditView::_SC_MARGE_SYMBOL) && !notification->modifiers)
			{
				if (!_pEditView->markerMarginClick(lineClick))
					bookmarkToggle(lineClick);
			}
			break;
		}

		case SCN_MARGINRIGHTCLICK:
		{
			if (!notifyView) return FALSE;

			if (notification->nmhdr.hwndFrom == _mainEditView.getHSelf())
				switchEditViewTo(MAIN_VIEW);
			else if (notification->nmhdr.hwndFrom == _subEditView.getHSelf())
				switchEditViewTo(SUB_VIEW);

			if ((notification->margin == ScintillaEditView::_SC_MARGE_SYMBOL) && !notification->modifiers)
			{
				POINT p;
				::GetCursorPos(&p);
				MenuPosition& menuPos = getMenuPosition("search-bookmark");
				HMENU hSearchMenu = ::GetSubMenu(_mainMenuHandle, menuPos._x);
				if (hSearchMenu)
				{
					HMENU hBookmarkMenu = ::GetSubMenu(hSearchMenu, menuPos._y);
					if (hBookmarkMenu)
					{
						TrackPopupMenu(hBookmarkMenu, 0, p.x, p.y, 0, _pPublicInterface->getHSelf(), NULL);
					}
				}
			}
			break;
		}

		case SCN_FOLDINGSTATECHANGED:
		{
			if ((notification->nmhdr.hwndFrom == _mainEditView.getHSelf()) || (notification->nmhdr.hwndFrom == _subEditView.getHSelf()))
			{
				size_t lineClicked = notification->line;

				if (!_isFolding)
				{
					int urlAction = (NppParameters::getInstance()).getNppGUI()._styleURL;
					Buffer* currentBuf = _pEditView->getCurrentBuffer();
					if (urlAction != urlDisable && currentBuf->allowClickableLink())
					{
						addHotSpot();
					}
				}

				if (_pDocMap)
					_pDocMap->fold(lineClicked, _pEditView->isFolded(lineClicked));
			}
			return TRUE;
		}

		case SCN_CHARADDED:
		{
			if (!notifyView) return FALSE;

			if (!_recordingMacro && !_playingBackMacro) // No macro recording or playing back
			{
				const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
				if (nppGui._maintainIndent != autoIndent_none)
					maintainIndentation(static_cast<wchar_t>(notification->ch));

				Buffer* currentBuf = _pEditView->getCurrentBuffer();
				if (currentBuf->allowAutoCompletion())
				{
					AutoCompletion* autoC = isFromPrimary ? &_autoCompleteMain : &_autoCompleteSub;
					bool isColumnMode = _pEditView->execute(SCI_GETSELECTIONS) > 1; // Multi-Selection || Column mode)
					if (nppGui._matchedPairConf.hasAnyPairsPair() && !isColumnMode)
						autoC->insertMatchedChars(notification->ch, nppGui._matchedPairConf);
					autoC->update(notification->ch);
				}
			}
			break;
		}

		case SCN_DOUBLECLICK:
		{
			if (!notifyView) return FALSE;

			if (notification->modifiers == SCMOD_CTRL)
			{
				const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();

				std::string bufstring;

				size_t position_of_click;
				// For some reason Ctrl+DoubleClick on an empty line means that notification->position == 1.
				// In that case we use SCI_GETCURRENTPOS to get the position.
				if (notification->position != -1)
					position_of_click = notification->position;
				else
					position_of_click = _pEditView->execute(SCI_GETCURRENTPOS);

				// Anonymous scope to limit use of the buf pointer (much easier to deal with std::string).
				{
					char* buf;

					if (nppGUI._delimiterSelectionOnEntireDocument)
					{
						// Get entire document.
						auto length = notifyView->execute(SCI_GETLENGTH);
						buf = new char[length + 1];
						notifyView->execute(SCI_GETTEXT, length + 1, reinterpret_cast<LPARAM>(buf));
					}
					else
					{
						// Get single line.
						auto length = notifyView->execute(SCI_GETCURLINE);
						buf = new char[length + 1];
						notifyView->execute(SCI_GETCURLINE, length, reinterpret_cast<LPARAM>(buf));

						// Compute the position of the click (relative to the beginning of the line).
						const auto line_position = notifyView->execute(SCI_POSITIONFROMLINE, notifyView->getCurrentLineNumber());
						position_of_click = position_of_click - line_position;
					}

					bufstring = buf;
					delete[] buf;
				}

				int leftmost_position = -1;
				int rightmost_position = -1;

				if (nppGUI._rightmostDelimiter == nppGUI._leftmostDelimiter)
				{
					// If the delimiters are the same (e.g. they are both a quotation mark), choose the ones
					// which are closest to the clicked position.
					for (int32_t i = static_cast<int32_t>(position_of_click); i >= 0; --i)
					{
						if (i >= static_cast<int32_t>(bufstring.size()))
							return FALSE;

						if (bufstring.at(i) == nppGUI._leftmostDelimiter)
						{
							// Respect escaped quotation marks.
							if (nppGUI._leftmostDelimiter == '"')
							{
								if (!(i > 0 && bufstring.at(i - 1) == '\\'))
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
					for (size_t i = position_of_click; i < bufstring.length(); ++i)
					{
						if (bufstring.at(i) == nppGUI._rightmostDelimiter)
						{
							// Respect escaped quotation marks.
							if (nppGUI._rightmostDelimiter == '"')
							{
								if (!(i > 0 && bufstring.at(i - 1) == '\\'))
								{
									rightmost_position = static_cast<int32_t>(i);
									break;
								}
							}
							else
							{
								rightmost_position = static_cast<int32_t>(i);
								break;
							}
						}
					}
				}
				else
				{
					// Find matching pairs of delimiters (e.g. parentheses).
					// The pair where the distance from the left delimiter to position_of_click is at a minimum is the one we're looking for.
					// Of course position_of_click must lie between the delimiters.

					// This logic is required to handle cases like this:
					// (size_t i = function(); i < _buffers.size(); i++)

					std::stack<unsigned int> leftmost_delimiter_positions;

					for (unsigned int i = 0; i < bufstring.length(); ++i)
					{
						if (bufstring.at(i) == nppGUI._leftmostDelimiter)
							leftmost_delimiter_positions.push(i);
						else if (bufstring.at(i) == nppGUI._rightmostDelimiter && !leftmost_delimiter_positions.empty())
						{
							unsigned int matching_leftmost = leftmost_delimiter_positions.top();
							leftmost_delimiter_positions.pop();

							// We have either 1) chosen neither the left- or rightmost position, or 2) chosen both left- and rightmost position.
							assert((leftmost_position == -1 && rightmost_position == -1) || (leftmost_position >= 0 && rightmost_position >= 0));

							// Note: cast of leftmost_position to unsigned int is safe, since if leftmost_position is not -1 then it is guaranteed to be positive.
							// If it was possible, leftmost_position and rightmost_position should be of type optional<unsigned int>.
							if (matching_leftmost <= position_of_click && i >= position_of_click && (leftmost_position == -1 || matching_leftmost > static_cast<unsigned int>(leftmost_position)))
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
						const auto line_position = notifyView->execute(SCI_POSITIONFROMLINE, notifyView->getCurrentLineNumber());
						notifyView->execute(SCI_SETCURRENTPOS, line_position + rightmost_position);
						notifyView->execute(SCI_SETANCHOR, line_position + leftmost_position + 1);
					}
				}
			}
			else
			{ // Double click with no modifiers
				// Check whether cursor is within URL
				auto indicMsk = notifyView->execute(SCI_INDICATORALLONFOR, notification->position);
				if (!(indicMsk & (1 << URL_INDIC)))
					break;

				auto startPos = notifyView->execute(SCI_INDICATORSTART, URL_INDIC, notification->position);
				auto endPos = notifyView->execute(SCI_INDICATOREND, URL_INDIC, notification->position);
				if ((notification->position < startPos) || (notification->position > endPos))
					break;

				// WM_LBUTTONUP goes to opening browser instead of Scintilla here, because the mouse is not captured.
				// The missing message causes mouse cursor flicker as soon as the mouse cursor is moved to a position outside the text editing area.
				::PostMessage(notifyView->getHSelf(), WM_LBUTTONUP, 0, 0);

				// Revert selection of current word. Best to this early, otherwise the
				// selected word is visible all the time while the browser is starting
				notifyView->execute(SCI_SETSEL, notification->position, notification->position);

				// Open URL
				wstring url = notifyView->getGenericTextAsString(static_cast<size_t>(startPos), static_cast<size_t>(endPos));
				::ShellExecute(_pPublicInterface->getHSelf(), L"open", url.c_str(), NULL, NULL, SW_SHOW);
			}
			break;
		}

		case SCN_UPDATEUI:
		{
			if (!notifyView) return FALSE;

			NppParameters& nppParam = NppParameters::getInstance();
			NppGUI& nppGui = nppParam.getNppGUI();

			Buffer* currentBuf = notifyView->getCurrentBuffer();
			
			// replacement for obsolete custom SCN_SCROLLED
			if (notification->updated & SC_UPDATE_V_SCROLL)
			{
				int urlAction = (NppParameters::getInstance()).getNppGUI()._styleURL;
				if (urlAction != urlDisable && currentBuf->allowClickableLink())
				{
					addHotSpot(notifyView);
				}
			}

			// if it's searching/replacing, then do nothing
			if (nppParam._isFindReplacing)
				break;

			if (notification->nmhdr.hwndFrom != _pEditView->getHSelf() && currentBuf->allowSmartHilite()) // notification come from unfocus view - both views ae visible
			{
				if (nppGui._smartHiliteOnAnotherView)
				{
					wchar_t selectedText[1024];
					_pEditView->getGenericSelectedText(selectedText, sizeof(selectedText) / sizeof(wchar_t), false);
					_smartHighlighter.highlightViewWithWord(notifyView, selectedText);
				}
				break;
			}

			braceMatch();

			if (nppGui._enableTagsMatchHilite)
			{
				XmlMatchedTagsHighlighter xmlTagMatchHiliter(_pEditView);
				xmlTagMatchHiliter.tagMatch(nppGui._enableTagAttrsHilite);
			}

			if (nppGui._enableSmartHilite && currentBuf->allowSmartHilite())
			{
				if (nppGui._disableSmartHiliteTmp)
					nppGui._disableSmartHiliteTmp = false;
				else
				{
					ScintillaEditView* anbotherView = isFromPrimary ? &_subEditView : &_mainEditView;
					_smartHighlighter.highlightView(notifyView, anbotherView);
				}
			}

			bool selectionIsChanged = (notification->updated & SC_UPDATE_SELECTION) != 0;
			// note: changing insert/overwrite mode will cause Scintilla to notify with SC_UPDATE_SELECTION
			bool contentIsChanged = (notification->updated & SC_UPDATE_CONTENT) != 0;
			if (selectionIsChanged || contentIsChanged)
			{
				updateStatusBar();
			}

			if (_pFuncList && (!_pFuncList->isClosed()) && _pFuncList->isVisible())
				_pFuncList->markEntry();
			AutoCompletion* autoC = isFromPrimary ? &_autoCompleteMain : &_autoCompleteSub;
			autoC->update(0);

			break;
		}

		case SCN_ZOOM:
		{
			if (!notifyView) return FALSE;

			ScintillaEditView* unfocusView = isFromPrimary ? &_subEditView : &_mainEditView;
			_smartHighlighter.highlightView(notifyView, unfocusView);
			break;
		}

		case SCN_MACRORECORD:
		{
			_macro.push_back(
				recordedMacroStep(
					notification->message,
					notification->wParam,
					notification->lParam
				)
			);
			break;
		}

		case SCN_PAINTED:
		{
			if (!notifyView) return FALSE;

			// Check if a restore position is needed. 
			// Restoring a position must done after SCN_PAINTED notification so that it works in every circumstances (including wrapped large file)
			_mainEditView.restoreCurrentPosPostStep();
			_subEditView.restoreCurrentPosPostStep();

			// ViewMoveAtWrappingDisableFix: Disable wrapping messes up visible lines.
			// Therefore save view position before in IDM_VIEW_WRAP and restore after SCN_PAINTED, as doc. says
			if (_mainEditView.isWrapRestoreNeeded())
			{
				_mainEditView.restoreCurrentPosPreStep();
				_mainEditView.setWrapRestoreNeeded(false);
			}

			if (_subEditView.isWrapRestoreNeeded())
			{
				_subEditView.restoreCurrentPosPreStep();
				_subEditView.setWrapRestoreNeeded(false);
			}

			notifyView->updateLineNumberWidth();

			if (_syncInfo.doSync())
				doSynScorll(HWND(notification->nmhdr.hwndFrom));

			const NppParameters& nppParam = NppParameters::getInstance();

			// if it's searching/replacing, then do nothing
			if ((_linkTriggered && !nppParam._isFindReplacing) || notification->wParam == LINKTRIGGERED)
			{
				int urlAction = (NppParameters::getInstance()).getNppGUI()._styleURL;
				Buffer* currentBuf = _pEditView->getCurrentBuffer();
				if (urlAction != urlDisable && currentBuf->allowClickableLink())
				{
					addHotSpot();
				}
				_linkTriggered = false;
			}

			if (_pDocMap && (!_pDocMap->isClosed()) && _pDocMap->isVisible() && !_pDocMap->isTemporarilyShowing())
			{
				_pDocMap->wrapMap();
				_pDocMap->scrollMap();
			}
			break;
		}

		case SCN_CALLTIPCLICK:
		{
			if (!notifyView) return FALSE;

			AutoCompletion* autoC = isFromPrimary ? &_autoCompleteMain : &_autoCompleteSub;
			autoC->callTipClick(notification->position);
			break;
		}

		case SCN_AUTOCSELECTION:
		{
			if (!notifyView) return FALSE;

			const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();

			// if autocompletion is disabled and it is triggered manually, then both ENTER & TAB will insert the selection 
			if (nppGui._autocStatus == NppGUI::AutocStatus::autoc_none)
			{
				break;
			}

			if (notification->listCompletionMethod == SC_AC_NEWLINE && !nppGui._autocInsertSelectedUseENTER)
			{
				notifyView->execute(SCI_AUTOCCANCEL);
				notifyView->execute(SCI_NEWLINE);
			}

			if (notification->listCompletionMethod == SC_AC_TAB && !nppGui._autocInsertSelectedUseTAB)
			{
				notifyView->execute(SCI_AUTOCCANCEL);
				notifyView->execute(SCI_TAB);
			}
			break;
		}

		//
		// ======= End of SCN_*
		//


		case TCN_MOUSEHOVERING:
		case TCN_MOUSEHOVERSWITCHING:
		{
			NppParameters& nppParam = NppParameters::getInstance();
			bool doPeekOnTab = nppParam.getNppGUI()._isDocPeekOnTab;
			bool doPeekOnMap = nppParam.getNppGUI()._isDocPeekOnMap;

			if (doPeekOnTab)
			{
				TBHDR *tbHdr = reinterpret_cast<TBHDR *>(notification);
				DocTabView *pTabDocView = isFromPrimary ? &_mainDocTab : (isFromSecondary ? &_subDocTab : nullptr);

				if (pTabDocView)
				{
					BufferID id = pTabDocView->getBufferByIndex(tbHdr->_tabOrigin);
					Buffer *pBuf = MainFileManager.getBufferByID(id);

					const Buffer* currentBufMain = _mainEditView.getCurrentBuffer();
					const Buffer* currentBufSub = _subEditView.getCurrentBuffer();

					RECT rect{};
					TabCtrl_GetItemRect(pTabDocView->getHSelf(), tbHdr->_tabOrigin, &rect);
					POINT p{};
					p.x = rect.left;
					p.y = rect.bottom;
					::ClientToScreen(pTabDocView->getHSelf(), &p);

					if (pBuf != currentBufMain && pBuf != currentBufSub) // if hover on other tab
					{
						_documentPeeker.doDialog(p, pBuf, *(const_cast<ScintillaEditView*>(pTabDocView->getScintillaEditView())));
					}
					else  // if hover on current active tab
					{
						_documentPeeker.display(false);
					}
				}
			}

			if (doPeekOnMap && _pDocMap && (!_pDocMap->isClosed()) && _pDocMap->isVisible())
			{
				TBHDR *tbHdr = reinterpret_cast<TBHDR *>(notification);
				DocTabView *pTabDocView = isFromPrimary ? &_mainDocTab : (isFromSecondary ? &_subDocTab : nullptr);
				if (pTabDocView)
				{
					BufferID id = pTabDocView->getBufferByIndex(tbHdr->_tabOrigin);
					Buffer* pBuf = MainFileManager.getBufferByID(id);

					const Buffer* currentBufMain = _mainEditView.getCurrentBuffer();
					const Buffer* currentBufSub = _subEditView.getCurrentBuffer();

					if (pBuf != currentBufMain && pBuf != currentBufSub) // if hover on other tab
					{
						_pDocMap->showInMapTemporarily(pBuf, notifyView);
						_pDocMap->setSyntaxHiliting();
					}
					else  // if hover on current active tab
					{
						_pDocMap->reloadMap();
						_pDocMap->setSyntaxHiliting();
					}
					_pDocMap->setTemporarilyShowing(true);
				}
			}

			break;
		}

		case TCN_MOUSELEAVING:
		{
			NppParameters& nppParam = NppParameters::getInstance();
			bool doPeekOnTab = nppParam.getNppGUI()._isDocPeekOnTab;
			bool doPeekOnMap = nppParam.getNppGUI()._isDocPeekOnMap;

			if (doPeekOnTab)
			{
				_documentPeeker.display(false);
			}

			if (doPeekOnMap && _pDocMap && (!_pDocMap->isClosed()) && _pDocMap->isVisible())
			{
				_pDocMap->reloadMap();
				_pDocMap->setSyntaxHiliting();

				_pDocMap->setTemporarilyShowing(false);
			}
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
						wchar_t goToView[32] = L"Move to Other View";
						wchar_t cloneToView[32] = L"Clone to Other View";
						vector<MenuItemUnit> itemUnitArray;
						itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_ANOTHER_VIEW, goToView));
						itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, cloneToView));
						_tabPopupDropMenu.create(_pPublicInterface->getHSelf(), itemUnitArray, _mainMenuHandle);
						_nativeLangSpeaker.changeLangTabDropContextMenu(_tabPopupDropMenu.getMenuHandle());
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
					RECT nppZone{};
					::GetWindowRect(_pPublicInterface->getHSelf(), &nppZone);
					bool isInNppZone = (((p.x >= nppZone.left) && (p.x <= nppZone.right)) && (p.y >= nppZone.top) && (p.y <= nppZone.bottom));
					if (isInNppZone)
					{
						// Do nothing
						return TRUE;
					}
					wstring quotFileName = L"\"";
					quotFileName += _pEditView->getCurrentBuffer()->getFullPathName();
					quotFileName += L"\"";
					COPYDATASTRUCT fileNamesData{};
					fileNamesData.dwData = COPYDATA_FILENAMESW;
					fileNamesData.lpData = (void *)quotFileName.c_str();
					fileNamesData.cbData = static_cast<DWORD>((quotFileName.length() + 1) * sizeof(wchar_t));

					HWND hWinParent = ::GetParent(hWin);
					const rsize_t classNameBufferSize = MAX_PATH;
					wchar_t className[classNameBufferSize];
					::GetClassName(hWinParent,className, classNameBufferSize);
					if (lstrcmp(className, _pPublicInterface->getClassName()) == 0 && hWinParent != _pPublicInterface->getHSelf()) // another Notepad++
					{
						int index = _pDocTab->getCurrentTabIndex();
						BufferID bufferToClose = notifyDocTab->getBufferByIndex(index);
						Buffer * buf = MainFileManager.getBufferByID(bufferToClose);
						int iView = isFromPrimary?MAIN_VIEW:SUB_VIEW;
						if (buf->isDirty())
						{
							_nativeLangSpeaker.messageBox("CannotMoveDoc",
								_pPublicInterface->getHSelf(),
								L"Document is modified, save it then try again.",
								L"Move to new Notepad++ Instance",
								MB_OK);
						}
						else
						{
							::SendMessage(hWinParent, NPPM_INTERNAL_SWITCHVIEWFROMHWND, 0, reinterpret_cast<LPARAM>(hWin));
							::SendMessage(hWinParent, WM_COPYDATA, reinterpret_cast<WPARAM>(_pPublicInterface->getHinst()), reinterpret_cast<LPARAM>(&fileNamesData));
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
			int index = tabNotification->_tabOrigin;
			BufferID bufferToClose = notifyDocTab->getBufferByIndex(index);
			Buffer * buf = MainFileManager.getBufferByID(bufferToClose);
			int iView = isFromPrimary ? MAIN_VIEW : SUB_VIEW;
			if (buf->isDirty())
			{
				activateBuffer(bufferToClose, iView);
			}

			BufferID bufferToClose2ndCheck = notifyDocTab->getBufferByIndex(index);

			if ((bufferToClose == bufferToClose2ndCheck) // Here we make sure the buffer is the same to prevent from the situation that the buffer to be close was already closed,
			                                             // because the precedent call "activateBuffer(bufferToClose, iView)" could finally lead "doClose" call as well (in case of file non-existent).
				&& fileClose(bufferToClose, iView))
				checkDocState();

			break;
		}

		case TCN_TABPINNED:
		{
			int index = tabNotification->_tabOrigin;
			BufferID bufferToBePinned = notifyDocTab->getBufferByIndex(index);
			Buffer * buf = MainFileManager.getBufferByID(bufferToBePinned);

			bool isPinned = buf->isPinned();

			if (_mainDocTab.getHSelf() == notification->nmhdr.hwndFrom)
			{
				if (!isPinned)
					_mainDocTab.tabToStart(index);
				else
					_mainDocTab.tabToEnd(index);
			}
			else if (_subDocTab.getHSelf() == notification->nmhdr.hwndFrom)
			{
				if (!isPinned)
					_subDocTab.tabToStart(index);
				else
					_subDocTab.tabToEnd(index);
			}
			else
				return FALSE;

			buf->setPinned(!isPinned);

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

			// save map position before switch to a new document
			_documentPeeker.saveCurrentSnapshot(*_pEditView);

			switchEditViewTo(iView);
			BufferID bufid = _pDocTab->getBufferByIndex(_pDocTab->getCurrentTabIndex());
			if (bufid != BUFFER_INVALID)
			{
				_isFolding = true; // So we can ignore events while folding is taking place
				activateBuffer(bufid, iView);
				_isFolding = false;
			}
			_documentPeeker.display(false);
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
					_statusBar.setText((_pEditView->execute(SCI_GETOVERTYPE)) ? L"OVR" : L"INS", STATUSBAR_TYPING_MODE);
				}
			}
			else if (notification->nmhdr.hwndFrom == _mainDocTab.getHSelf())
			{
				if (_activeView == SUB_VIEW)
				{
					bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
					if (isSnapshotMode)
					{
						// Before switching off, synchronize backup file
						MainFileManager.backupCurrentBuffer();
					}
				}
				// Switch off
				switchEditViewTo(MAIN_VIEW);
			}
			else if (notification->nmhdr.hwndFrom == _subDocTab.getHSelf())
			{
				if (_activeView == MAIN_VIEW)
				{
					bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
					if (isSnapshotMode)
					{
						// Before switching off, synchronize backup file
						MainFileManager.backupCurrentBuffer();
					}
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
					command(IDM_SEARCH_GOTOLINE);
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
				else if (lpnm->dwItemSpec == DWORD(STATUSBAR_UNICODE_TYPE))
				{
					POINT p;
					::GetCursorPos(&p);
					HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FORMAT);
					TrackPopupMenu(hLangMenu, 0, p.x, p.y, 0, _pPublicInterface->getHSelf(), NULL);
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
					HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);
					TrackPopupMenu(hLangMenu, 0, p.x, p.y, 0, _pPublicInterface->getHSelf(), NULL);
				}
				else if (lpnm->dwItemSpec == DWORD(STATUSBAR_EOF_FORMAT))
				{
					MenuPosition & menuPos = getMenuPosition("edit-eolConversion");
					HMENU hEditMenu = ::GetSubMenu(_mainMenuHandle, menuPos._x);
					if (!hEditMenu)
						return TRUE;
					HMENU hEolFormatMenu = ::GetSubMenu(hEditMenu, menuPos._y);
					if (!hEolFormatMenu)
						return TRUE;
					TrackPopupMenu(hEolFormatMenu, 0, p.x, p.y, 0, _pPublicInterface->getHSelf(), NULL);
				}
				else if (lpnm->dwItemSpec == DWORD(STATUSBAR_UNICODE_TYPE))
				{
					POINT cursorPos;
					::GetCursorPos(&cursorPos);
					HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FORMAT);
					TrackPopupMenu(hLangMenu, 0, cursorPos.x, cursorPos.y, 0, _pPublicInterface->getHSelf(), NULL);
				}
				return TRUE;
			}
			else if (_pDocumentListPanel && notification->nmhdr.hwndFrom == _pDocumentListPanel->getHSelf())
			{
				// Already switched, so do nothing here.

				if (_pDocumentListPanel->nbSelectedFiles() > 1)
				{
					if (!_fileSwitcherMultiFilePopupMenu.isCreated())
					{
						vector<MenuItemUnit> itemUnitArray;
						itemUnitArray.push_back(MenuItemUnit(IDM_DOCLIST_FILESCLOSE, L"Close Selected files"));
						itemUnitArray.push_back(MenuItemUnit(IDM_DOCLIST_FILESCLOSEOTHERS, L"Close Other files"));
						itemUnitArray.push_back(MenuItemUnit(IDM_DOCLIST_COPYNAMES, L"Copy Selected Names"));
						itemUnitArray.push_back(MenuItemUnit(IDM_DOCLIST_COPYPATHS, L"Copy Selected Pathnames"));

						for (auto&& x : itemUnitArray)
						{
							const wstring menuItem = _nativeLangSpeaker.getNativeLangMenuString(x._cmdID);
							if (!menuItem.empty())
								x._itemName = menuItem;
						}

						_fileSwitcherMultiFilePopupMenu.create(_pPublicInterface->getHSelf(), itemUnitArray);
					}
					_fileSwitcherMultiFilePopupMenu.display(p);
					return TRUE;
				}
			}
			else // From tool bar
				return TRUE;
			//break;

			NppParameters& nppParam = NppParameters::getInstance();

			if (!_tabPopupMenu.isCreated())
			{
				std::vector<MenuItemUnit> itemUnitArray;

				if (nppParam.hasCustomTabContextMenu())
				{
					itemUnitArray = nppParam.getTabContextMenuItems();
				}
				else // default tab context menu
				{
					// IMPORTANT: If any submenu entry is added/moved/removed, you have to change the value of tabCmSubMenuEntryPos[] in localization.cpp file
					
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSE, L"Close"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_BUT_CURRENT, L"Close All BUT This", L"Close Multiple Tabs"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_BUT_PINNED, L"Close All BUT Pinned", L"Close Multiple Tabs"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_TOLEFT, L"Close All to the Left", L"Close Multiple Tabs"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_TORIGHT, L"Close All to the Right", L"Close Multiple Tabs"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_UNCHANGED, L"Close All Unchanged", L"Close Multiple Tabs"));
					itemUnitArray.push_back(MenuItemUnit(IDM_PINTAB, L"Pin Tab"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_SAVE, L"Save"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_SAVEAS, L"Save As..."));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_OPEN_FOLDER, L"Open Containing Folder in Explorer", L"Open into"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_OPEN_CMD, L"Open Containing Folder in cmd", L"Open into"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CONTAININGFOLDERASWORKSPACE, L"Open Containing Folder as Workspace", L"Open into"));
					itemUnitArray.push_back(MenuItemUnit(0, NULL, L"Open into"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_OPEN_DEFAULT_VIEWER, L"Open in Default Viewer", L"Open into"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_RENAME, L"Rename"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_DELETE, L"Move to Recycle Bin"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_RELOAD, L"Reload"));
					itemUnitArray.push_back(MenuItemUnit(IDM_FILE_PRINT, L"Print"));
					itemUnitArray.push_back(MenuItemUnit(0, NULL));
					itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_SETREADONLY, L"Read-Only"));
					itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_CLEARREADONLY, L"Clear Read-Only Flag"));
					itemUnitArray.push_back(MenuItemUnit(0, NULL));
					itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_FULLPATHTOCLIP, L"Copy Full File Path", L"Copy to Clipboard"));
					itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_FILENAMETOCLIP, L"Copy Filename", L"Copy to Clipboard"));
					itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_CURRENTDIRTOCLIP, L"Copy Current Dir. Path", L"Copy to Clipboard"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_START, L"Move to Start", L"Move Document"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_END, L"Move to End", L"Move Document"));
					itemUnitArray.push_back(MenuItemUnit(0, NULL, L"Move Document"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_ANOTHER_VIEW, L"Move to Other View", L"Move Document"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, L"Clone to Other View", L"Move Document"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_NEW_INSTANCE, L"Move to New Instance", L"Move Document"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_LOAD_IN_NEW_INSTANCE, L"Open in New Instance", L"Move Document"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_TAB_COLOUR_1, L"Apply Color 1", L"Apply Color to Tab"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_TAB_COLOUR_2, L"Apply Color 2", L"Apply Color to Tab"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_TAB_COLOUR_3, L"Apply Color 3", L"Apply Color to Tab"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_TAB_COLOUR_4, L"Apply Color 4", L"Apply Color to Tab"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_TAB_COLOUR_5, L"Apply Color 5", L"Apply Color to Tab"));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_TAB_COLOUR_NONE, L"Remove Color", L"Apply Color to Tab"));

					// IMPORTANT: If any submenu entry is added/moved/removed, you have to change the value of tabCmSubMenuEntryPos[] in localization.cpp file
				}
				_tabPopupMenu.create(_pPublicInterface->getHSelf(), itemUnitArray, _mainMenuHandle);
				_nativeLangSpeaker.changeLangTabContextMenu(_tabPopupMenu.getMenuHandle());
			}

			// Adds colour icons
			for (int i = 0; i < 5; ++i)
			{
				COLORREF colour = nppParam.getIndividualTabColor(i, NppDarkMode::isDarkMenuEnabled(), true);
				HBITMAP hBitmap = generateSolidColourMenuItemIcon(colour);
				SetMenuItemBitmaps(_tabPopupMenu.getMenuHandle(), IDM_VIEW_TAB_COLOUR_1 + i, MF_BYCOMMAND, hBitmap, hBitmap);
			}

			bool isEnable = ((::GetMenuState(_mainMenuHandle, IDM_FILE_SAVE, MF_BYCOMMAND)&MF_DISABLED) == 0);
			_tabPopupMenu.enableItem(IDM_FILE_SAVE, isEnable);

			Buffer* buf = _pEditView->getCurrentBuffer();
			bool isUserReadOnly = buf->getUserReadOnly();
			_tabPopupMenu.checkItem(IDM_EDIT_SETREADONLY, isUserReadOnly);

			bool isSysReadOnly = buf->getFileReadOnly();
			bool isInaccessible = buf->isInaccessible();
			_tabPopupMenu.enableItem(IDM_EDIT_SETREADONLY, !isSysReadOnly && !buf->isMonitoringOn());
			_tabPopupMenu.enableItem(IDM_EDIT_CLEARREADONLY, isSysReadOnly);
			if (isInaccessible)
				_tabPopupMenu.enableItem(IDM_EDIT_CLEARREADONLY, false);

			bool isFileExisting = doesFileExist(buf->getFullPathName());
			_tabPopupMenu.enableItem(IDM_FILE_DELETE, isFileExisting);
			_tabPopupMenu.enableItem(IDM_FILE_RELOAD, isFileExisting);
			_tabPopupMenu.enableItem(IDM_FILE_OPEN_FOLDER, isFileExisting);
			_tabPopupMenu.enableItem(IDM_FILE_OPEN_CMD, isFileExisting);
			_tabPopupMenu.enableItem(IDM_FILE_CONTAININGFOLDERASWORKSPACE, isFileExisting);

			_tabPopupMenu.enableItem(IDM_FILE_OPEN_DEFAULT_VIEWER, isAssoCommandExisting(buf->getFullPathName()));

			bool isDirty = buf->isDirty();
			bool isUntitled = buf->isUntitled();
			_tabPopupMenu.enableItem(IDM_VIEW_GOTO_ANOTHER_VIEW, !isInaccessible);
			_tabPopupMenu.enableItem(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, !isInaccessible);
			_tabPopupMenu.enableItem(IDM_VIEW_GOTO_NEW_INSTANCE, !isInaccessible && !isDirty && !isUntitled);
			_tabPopupMenu.enableItem(IDM_VIEW_LOAD_IN_NEW_INSTANCE, !isInaccessible && !isDirty && !isUntitled);

			_tabPopupMenu.enableItem(IDM_FILE_SAVEAS, !isInaccessible);
			_tabPopupMenu.enableItem(IDM_FILE_RENAME, !isInaccessible);

			bool isTabPinEnabled = TabBarPlus::drawTabPinButton();
			wstring newName;
			if (isTabPinEnabled)
			{
				wstring defaultName;
				bool isAlternative;
				if (buf->isPinned())
				{
					defaultName = L"Unpin Tab";
					isAlternative = true;
				}
				else
				{
					defaultName = L"Pin Tab";
					isAlternative = false;
				}
				_nativeLangSpeaker.getAlternativeNameFromTabContextMenu(newName, IDM_PINTAB, isAlternative, defaultName);
				::ModifyMenu(_tabPopupMenu.getMenuHandle(), IDM_PINTAB, MF_BYCOMMAND, IDM_PINTAB, newName.c_str());
			}
			else
			{
				_nativeLangSpeaker.getAlternativeNameFromTabContextMenu(newName, IDM_PINTAB, false, L"Pin Tab");
				::ModifyMenu(_tabPopupMenu.getMenuHandle(), IDM_PINTAB, MF_BYCOMMAND, IDM_PINTAB, newName.c_str());
			}

			_tabPopupMenu.enableItem(IDM_PINTAB, isTabPinEnabled);

			_tabPopupMenu.display(p);
			return TRUE;
		}

		case TTN_GETDISPINFO:
		{
			try
			{
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)notification;
				lpttt->hinst = NULL;

				POINT p;
				::GetCursorPos(&p);
				::MapWindowPoints(NULL, _pPublicInterface->getHSelf(), &p, 1);
				HWND hWin = ::ChildWindowFromPointEx(_pPublicInterface->getHSelf(), p, CWP_SKIPINVISIBLE);
				const int tipMaxLen = 1024;
				static wchar_t docTip[tipMaxLen];
				docTip[0] = '\0';

				wstring tipTmp(L"");
				int id = int(lpttt->hdr.idFrom);

				if (hWin == _rebarTop.getHSelf())
				{
					getNameStrFromCmd(id, tipTmp);
					if (tipTmp.length() >= 80)
						return FALSE;

					wcscpy_s(lpttt->szText, tipTmp.c_str());
					return TRUE;
				}
				else
				{
					BufferID idd = BUFFER_INVALID;
					if (hWin == _mainDocTab.getHSelf())
						idd = _mainDocTab.getBufferByIndex(id);
					else if (hWin == _subDocTab.getHSelf())
						idd = _subDocTab.getBufferByIndex(id);
					else
						return FALSE;

					Buffer* buf = MainFileManager.getBufferByID(idd);
					if (buf == nullptr)
						return FALSE;

					tipTmp = buf->getFullPathName();


					if (buf->isUntitled())
					{
						wstring tabCreatedTime = buf->tabCreatedTimeString();
						if (!tabCreatedTime.empty())
						{
							tipTmp += L"\r";
							tipTmp += tabCreatedTime;
							SendMessage(lpttt->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 200);
						}
					}
					else
					{
						SendMessage(lpttt->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, -1);
					}

					if (tipTmp.length() >= tipMaxLen)
						return FALSE;

					wcscpy_s(docTip, tipTmp.c_str());
					lpttt->lpszText = docTip;
					return TRUE;
				}
			}
			catch (...)
			{
				//printStr(L"ToolTip crash is caught!"));
			}
			break;
		}

		case RBN_HEIGHTCHANGE:
		{
			SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			break;
		}

		case RBN_CHEVRONPUSHED:
		{
			NMREBARCHEVRON * lpnm = reinterpret_cast<NMREBARCHEVRON *>(notification);
			ReBar * notifRebar = &_rebarTop;
			if (_rebarBottom.getHSelf() == lpnm->hdr.hwndFrom)
				notifRebar = &_rebarBottom;

			//If Notepad++ ID, use proper object
			if (lpnm->wID == REBAR_BAR_TOOLBAR)
			{
				POINT pt{};
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
			::SendMessage(notifRebar->getHSelf(), RB_GETBANDINFO, lpnm->uBand, reinterpret_cast<LPARAM>(&rbBand));
			::SendMessage(rbBand.hwndChild, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(lpnm));
			break;
		}

		default:
			break;

	}
	return FALSE;
}
