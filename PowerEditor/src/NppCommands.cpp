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

#include <memory>
#include <shlwapi.h>
#include "Notepad_plus_Window.h"
#include "EncodingMapper.h"
#include "ShortcutMapper.h"
#include "TaskListDlg.h"
#include "clipboardFormats.h"
#include "VerticalFileSwitcher.h"
#include "documentMap.h"
#include "functionListPanel.h"
#include "Sorters.h"
#include "LongRunningOperation.h"

using namespace std;

void Notepad_plus::macroPlayback(Macro macro)
{
	_pEditView->execute(SCI_BEGINUNDOACTION);

	for (Macro::iterator step = macro.begin(); step != macro.end(); ++step)
	{
		if (step->isPlayable())
			step->PlayBack(this->_pPublicInterface, _pEditView);
		else
			_findReplaceDlg.execSavedCommand(step->message, step->lParameter, step->sParameter);
	}

	_pEditView->execute(SCI_ENDUNDOACTION);
}

void Notepad_plus::command(int id) 
{
	switch (id)
	{
		case IDM_FILE_NEW:
		{
			fileNew();
		}
		break;

		case IDM_FILE_OPEN:
		{
			fileOpen();
		}
		break;

		case IDM_FILE_OPEN_FOLDER:
		{
			Command cmd(TEXT("explorer /select,$(FULL_CURRENT_PATH)"));
			cmd.run(_pPublicInterface->getHSelf());
		}
		break;

		case IDM_FILE_OPEN_CMD:
		{
			Command cmd(TEXT("cmd /K cd /d $(CURRENT_DIRECTORY)"));
			cmd.run(_pPublicInterface->getHSelf());
		}
		break;

		case IDM_FILE_RELOAD:
			fileReload();
			break;

		case IDM_FILESWITCHER_FILESCLOSE:
		case IDM_FILESWITCHER_FILESCLOSEOTHERS:
			if (_pFileSwitcherPanel)
			{
				vector<SwitcherFileInfo> files = _pFileSwitcherPanel->getSelectedFiles(id == IDM_FILESWITCHER_FILESCLOSEOTHERS);
				for (size_t i = 0, len = files.size(); i < len; ++i)
				{
					fileClose((BufferID)files[i]._bufID, files[i]._iView);
				}
				if (id == IDM_FILESWITCHER_FILESCLOSEOTHERS)
				{
					// Get current buffer and its view
					_pFileSwitcherPanel->activateItem(int(_pEditView->getCurrentBufferID()), currentView());
				}
			}
			break;

		case IDM_FILE_CLOSE:
			if (fileClose())
                checkDocState();
			break;

		case IDM_FILE_DELETE:
			if (fileDelete())
                checkDocState();
			break;

		case IDM_FILE_RENAME:
			fileRename();
			break;

		case IDM_FILE_CLOSEALL:
		{
			bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
			fileCloseAll(isSnapshotMode, false);
            checkDocState();
			break;
		}
		
		case IDM_FILE_CLOSEALL_BUT_CURRENT :
			fileCloseAllButCurrent();
            checkDocState();
			break;

		case IDM_FILE_CLOSEALL_TOLEFT :
			fileCloseAllToLeft();
			checkDocState();
			break;

		case IDM_FILE_CLOSEALL_TORIGHT :
			fileCloseAllToRight();
			checkDocState();
			break;

		case IDM_FILE_SAVE :
			fileSave();
			break;

		case IDM_FILE_SAVEALL :
			fileSaveAll();
			break;

		case IDM_FILE_SAVEAS :
			fileSaveAs();
			break;

		case IDM_FILE_SAVECOPYAS :
			fileSaveAs(BUFFER_INVALID, true);
			break;

		case IDM_FILE_LOADSESSION:
			fileLoadSession();
			break;

		case IDM_FILE_SAVESESSION:
			fileSaveSession();
			break;

		case IDM_FILE_PRINTNOW :
			filePrint(false);
			break;

		case IDM_FILE_PRINT :
			filePrint(true);
			break;

		case IDM_FILE_EXIT:
			::SendMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
			break;

		case IDM_EDIT_UNDO:
		{
			LongRunningOperation op;
			_pEditView->execute(WM_UNDO);
			checkClipboard();
			checkUndoState();
			break;
		}

		case IDM_EDIT_REDO:
		{
			LongRunningOperation op;
			_pEditView->execute(SCI_REDO);
			checkClipboard();
			checkUndoState();
			break;
		}

		case IDM_EDIT_CUT:
			_pEditView->execute(WM_CUT);
			checkClipboard();
			break;

		case IDM_EDIT_COPY:
			_pEditView->execute(WM_COPY);
			checkClipboard();
			break;

		case IDM_EDIT_COPY_BINARY:
		case IDM_EDIT_CUT_BINARY:
		{
			int textLen = _pEditView->execute(SCI_GETSELTEXT, 0, 0) - 1;
			if (!textLen)
				return;

			char *pBinText = new char[textLen + 1];
			_pEditView->getSelectedText(pBinText, textLen + 1);

			// Open the clipboard, and empty it.
			if (!OpenClipboard(NULL))
				return;
			EmptyClipboard();

			// Allocate a global memory object for the text.
			HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (textLen + 1) * sizeof(unsigned char));
			if (hglbCopy == NULL)
			{
				CloseClipboard();
				return;
			}

			// Lock the handle and copy the text to the buffer.
			unsigned char *lpucharCopy = (unsigned char *)GlobalLock(hglbCopy);
			memcpy(lpucharCopy, pBinText, textLen * sizeof(unsigned char));
			lpucharCopy[textLen] = 0;    // null character
			
			GlobalUnlock(hglbCopy);
	 
			// Place the handle on the clipboard.
			SetClipboardData(CF_TEXT, hglbCopy);
			

			// Allocate a global memory object for the text length.
			HGLOBAL hglbLenCopy = GlobalAlloc(GMEM_MOVEABLE, sizeof(unsigned long));
			if (hglbLenCopy == NULL)
			{
				CloseClipboard();
				return;
			}
	 
			// Lock the handle and copy the text to the buffer. 
			unsigned long *lpLenCopy = (unsigned long *)GlobalLock(hglbLenCopy); 
			*lpLenCopy = textLen;
			
			GlobalUnlock(hglbLenCopy); 
	 
			// Place the handle on the clipboard.
			UINT f = RegisterClipboardFormat(CF_NPPTEXTLEN);
			SetClipboardData(f, hglbLenCopy);

			CloseClipboard();

			if (id == IDM_EDIT_CUT_BINARY)
				_pEditView->execute(SCI_REPLACESEL, 0, (LPARAM)"");
		}
		break;

		case IDM_EDIT_PASTE:
		{
			LongRunningOperation op;
			int eolMode = int(_pEditView->execute(SCI_GETEOLMODE));
			_pEditView->execute(SCI_PASTE);
			_pEditView->execute(SCI_CONVERTEOLS, eolMode);
		}
		break;

		case IDM_EDIT_PASTE_BINARY:
		{			
			LongRunningOperation op;
			if (!IsClipboardFormatAvailable(CF_TEXT))
				return;

			if (!OpenClipboard(NULL))
				return; 
	 
			HGLOBAL hglb = GetClipboardData(CF_TEXT); 
			if (hglb != NULL) 
			{ 
				char *lpchar = (char *)GlobalLock(hglb); 
				if (lpchar != NULL) 
				{
					UINT cf_nppTextLen = RegisterClipboardFormat(CF_NPPTEXTLEN);
					if (IsClipboardFormatAvailable(cf_nppTextLen))
					{
						HGLOBAL hglbLen = GetClipboardData(cf_nppTextLen); 
						if (hglbLen != NULL) 
						{ 
							unsigned long *lpLen = (unsigned long *)GlobalLock(hglbLen); 
							if (lpLen != NULL) 
							{
								_pEditView->execute(SCI_REPLACESEL, 0, (LPARAM)"");
								_pEditView->execute(SCI_ADDTEXT, *lpLen, (LPARAM)lpchar);

								GlobalUnlock(hglb); 
							}
						}
					}
					else
					{

						_pEditView->execute(SCI_REPLACESEL, 0, (LPARAM)lpchar);
					}
					GlobalUnlock(hglb); 
				}
			}
			CloseClipboard();

		}
		break;

		case IDM_EDIT_PASTE_AS_RTF:
		case IDM_EDIT_PASTE_AS_HTML:
		{
			LongRunningOperation op;
			UINT f = RegisterClipboardFormat(id==IDM_EDIT_PASTE_AS_HTML?CF_HTML:CF_RTF);

			if (!IsClipboardFormatAvailable(f)) 
				return;
				
			if (!OpenClipboard(NULL))
				return; 
	 
			HGLOBAL hglb = GetClipboardData(f); 
			if (hglb != NULL) 
			{ 
				LPSTR lptstr = (LPSTR)GlobalLock(hglb); 
				if (lptstr != NULL) 
				{ 
					// Call the application-defined ReplaceSelection 
					// function to insert the text and repaint the 
					// window. 
					_pEditView->execute(SCI_REPLACESEL, 0, (LPARAM)lptstr);

					GlobalUnlock(hglb); 
				}
			}
			CloseClipboard(); 
		}
		break;

		case IDM_EDIT_BEGINENDSELECT:
		{
			::CheckMenuItem(_mainMenuHandle, IDM_EDIT_BEGINENDSELECT, MF_BYCOMMAND | (_pEditView->beginEndSelectedIsStarted() ? MF_UNCHECKED : MF_CHECKED));
			_pEditView->beginOrEndSelect();
		}
		break;

		case IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING:
		case IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING:
		case IDM_EDIT_SORTLINES_INTEGER_ASCENDING:
		case IDM_EDIT_SORTLINES_INTEGER_DESCENDING:
		case IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING:
		case IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING:
		case IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING:
		case IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING:
		{
			LongRunningOperation op;

			size_t fromLine = 0, toLine = 0;
			size_t fromColumn = 0, toColumn = 0;

			bool hasLineSelection = false;
			if (_pEditView->execute(SCI_GETSELECTIONS) > 1)
			{
				if (_pEditView->execute(SCI_SELECTIONISRECTANGLE))
				{
					ColumnModeInfos colInfos = _pEditView->getColumnModeSelectInfo();
					int leftPos = colInfos.begin()->_selLpos;
					int rightPos = colInfos.rbegin()->_selRpos;
					int startPos = min(leftPos, rightPos);
					int endPos = max(leftPos, rightPos);
					fromLine = _pEditView->execute(SCI_LINEFROMPOSITION, startPos);
					toLine = _pEditView->execute(SCI_LINEFROMPOSITION, endPos);
					fromColumn = _pEditView->execute(SCI_GETCOLUMN, leftPos);
					toColumn = _pEditView->execute(SCI_GETCOLUMN, rightPos);
				}
				else
				{
					return;
				}
			}
			else
			{
				int selStart = _pEditView->execute(SCI_GETSELECTIONSTART);
				int selEnd = _pEditView->execute(SCI_GETSELECTIONEND);
				hasLineSelection = selStart != selEnd;
				if (hasLineSelection)
				{
					pair<int, int> lineRange = _pEditView->getSelectionLinesRange();
					// One single line selection is not allowed.
					if (lineRange.first == lineRange.second)
					{
						return;
					}
					fromLine = lineRange.first;
					toLine = lineRange.second;
				}
				else
				{
					// No selection.
					fromLine = 0;
					toLine = _pEditView->execute(SCI_GETLINECOUNT) - 1;
				}
			}

			bool isDescending = id == IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING ||
								id == IDM_EDIT_SORTLINES_INTEGER_DESCENDING ||
								id == IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING ||
								id == IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING;

			_pEditView->execute(SCI_BEGINUNDOACTION);
			std::unique_ptr<ISorter> pSorter;
			if (id == IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING || id == IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING)
			{
				pSorter = std::unique_ptr<ISorter>(new LexicographicSorter(isDescending, fromColumn, toColumn));
			}
			else if (id == IDM_EDIT_SORTLINES_INTEGER_DESCENDING || id == IDM_EDIT_SORTLINES_INTEGER_ASCENDING)
			{
				pSorter = std::unique_ptr<ISorter>(new IntegerSorter(isDescending, fromColumn, toColumn));
			}
			else if (id == IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING || id == IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING)
			{
				pSorter = std::unique_ptr<ISorter>(new DecimalCommaSorter(isDescending, fromColumn, toColumn));
			}
			else
			{
				pSorter = std::unique_ptr<ISorter>(new DecimalDotSorter(isDescending, fromColumn, toColumn));
			}
			try
			{
				_pEditView->sortLines(fromLine, toLine, pSorter.get());
			}
			catch (size_t& failedLineIndex)
			{
				generic_string lineNo = std::to_wstring(1 + fromLine + failedLineIndex);
				_nativeLangSpeaker.messageBox("SortingError",
					_pPublicInterface->getHSelf(),
					TEXT("Unable to perform numeric sort due to line $STR_REPLACE$."),
					TEXT("Sorting Error"),
					MB_OK | MB_ICONINFORMATION | MB_APPLMODAL,
					0,
					lineNo.c_str()); // We don't use intInfo since it would require casting size_t -> int.
			}
			_pEditView->execute(SCI_ENDUNDOACTION);

			if (hasLineSelection) // there was 1 selection, so we restore it
			{
				int posStart = _pEditView->execute(SCI_POSITIONFROMLINE, fromLine);
				int posEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, toLine);
				_pEditView->execute(SCI_SETSELECTIONSTART, posStart);
				_pEditView->execute(SCI_SETSELECTIONEND, posEnd);
			}
		}
		break;

		case IDM_EDIT_BLANKLINEABOVECURRENT:
		{
			_pEditView->insertNewLineAboveCurrentLine();
		}
		break;

		case IDM_EDIT_BLANKLINEBELOWCURRENT:
		{
			_pEditView->insertNewLineBelowCurrentLine();
		}
		break;

		case IDM_EDIT_CHAR_PANEL:
		{
			launchAnsiCharPanel();
		}
		break;

		case IDM_EDIT_CLIPBOARDHISTORY_PANEL:
		{
			launchClipboardHistoryPanel();
		}
		break;

		case IDM_VIEW_FILESWITCHER_PANEL:
		{
			launchFileSwitcherPanel();
		}
		break;

		case IDM_VIEW_PROJECT_PANEL_1:
		{
			launchProjectPanel(id, &_pProjectPanel_1, 0);
		}
		break;
		case IDM_VIEW_PROJECT_PANEL_2:
		{
			launchProjectPanel(id, &_pProjectPanel_2, 1);
		}
		break;
		case IDM_VIEW_PROJECT_PANEL_3:
		{
			launchProjectPanel(id, &_pProjectPanel_3, 2);
		}
		break;

		case IDM_VIEW_DOC_MAP:
		{
			if (_pDocMap && (!_pDocMap->isClosed()))
			{
				_pDocMap->display(false);
				_pDocMap->vzDlgDisplay(false);
				_pDocMap->setClosed(true);
				checkMenuItem(IDM_VIEW_DOC_MAP, false);
				_toolBar.setCheck(IDM_VIEW_DOC_MAP, false);
			}
			else
			{
				launchDocMap();
				if (_pDocMap)
				{
					checkMenuItem(IDM_VIEW_DOC_MAP, true);
					_toolBar.setCheck(IDM_VIEW_DOC_MAP, true);
					_pDocMap->setClosed(false);
				}
			}
		}
		break;

		case IDM_VIEW_FUNC_LIST:
		{
			if (_pFuncList && (!_pFuncList->isClosed()))
			{
				_pFuncList->display(false);
				_pFuncList->setClosed(true);
				checkMenuItem(IDM_VIEW_FUNC_LIST, false);
				_toolBar.setCheck(IDM_VIEW_FUNC_LIST, false);
			}
			else
			{
				checkMenuItem(IDM_VIEW_FUNC_LIST, true);
				_toolBar.setCheck(IDM_VIEW_FUNC_LIST, true);
				launchFunctionList();
				_pFuncList->setClosed(false);
			}
		}
		break;
		
		case IDM_VIEW_TAB1:
		case IDM_VIEW_TAB2:
		case IDM_VIEW_TAB3:
		case IDM_VIEW_TAB4:
		case IDM_VIEW_TAB5:
		case IDM_VIEW_TAB6:
		case IDM_VIEW_TAB7:
		case IDM_VIEW_TAB8:
		case IDM_VIEW_TAB9:
		{
			const int index = id - IDM_VIEW_TAB1;
			BufferID buf = _pDocTab->getBufferByIndex(index);
			if(buf == BUFFER_INVALID)
			{
				// No buffer at chosen index, select the very last buffer instead.
				const int last_index = _pDocTab->getItemCount() - 1;
				if(last_index > 0)
					switchToFile(_pDocTab->getBufferByIndex(last_index));
			}
			else
				switchToFile(buf);
		}
		break;

		case IDM_VIEW_TAB_NEXT:
		{
			const int current_index = _pDocTab->getCurrentTabIndex();
			const int last_index = _pDocTab->getItemCount() - 1;
			if(current_index < last_index)
				switchToFile(_pDocTab->getBufferByIndex(current_index + 1));
			else
				switchToFile(_pDocTab->getBufferByIndex(0)); // Loop around.
		}
		break;

		case IDM_VIEW_TAB_PREV:
		{
			const int current_index = _pDocTab->getCurrentTabIndex();
			if(current_index > 0)
				switchToFile(_pDocTab->getBufferByIndex(current_index - 1));
			else
			{
				const int last_index = _pDocTab->getItemCount() - 1;
				switchToFile(_pDocTab->getBufferByIndex(last_index)); // Loop around.
			}
		}
		break;

		case IDM_EDIT_DELETE:
			_pEditView->execute(WM_CLEAR);
			break;

		case IDM_MACRO_STARTRECORDINGMACRO:
		case IDM_MACRO_STOPRECORDINGMACRO:
		case IDC_EDIT_TOGGLEMACRORECORDING:
		{
			if (_recordingMacro)
			{
				// STOP !!!
				_mainEditView.execute(SCI_STOPRECORD);
				_subEditView.execute(SCI_STOPRECORD);
				
				_mainEditView.execute(SCI_SETCURSOR, (WPARAM)SC_CURSORNORMAL);
				_subEditView.execute(SCI_SETCURSOR, (WPARAM)SC_CURSORNORMAL);
				
				_recordingMacro = false;
				_runMacroDlg.initMacroList();
			}
			else
			{
				_mainEditView.execute(SCI_SETCURSOR, 9);
				_subEditView.execute(SCI_SETCURSOR, 9);
				_macro.clear();

				// START !!!
				_mainEditView.execute(SCI_STARTRECORD);
				_subEditView.execute(SCI_STARTRECORD);
				_recordingMacro = true;
			}
			checkMacroState();
			break;
		}

		case IDM_MACRO_PLAYBACKRECORDEDMACRO:
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
				macroPlayback(_macro);
			break;

		case IDM_MACRO_RUNMULTIMACRODLG :
		{
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
			{
				bool isFirstTime = !_runMacroDlg.isCreated();
				_runMacroDlg.doDialog(_nativeLangSpeaker.isRTL());
				
				if (isFirstTime)
				{
					_nativeLangSpeaker.changeDlgLang(_runMacroDlg.getHSelf(), "MultiMacro");
				}
				break;
				
			}
		}
		break;

		case IDM_MACRO_SAVECURRENTMACRO :
		{
			if (addCurrentMacro())
				_runMacroDlg.initMacroList();
			break;
		}
		case IDM_EDIT_FULLPATHTOCLIP :
		case IDM_EDIT_CURRENTDIRTOCLIP :
		case IDM_EDIT_FILENAMETOCLIP :
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			if (id == IDM_EDIT_FULLPATHTOCLIP)
			{
				str2Cliboard(buf->getFullPathName());
			}
			else if (id == IDM_EDIT_CURRENTDIRTOCLIP)
			{
				generic_string dir(buf->getFullPathName());
				PathRemoveFileSpec(dir);
				str2Cliboard(dir);
			}
			else if (id == IDM_EDIT_FILENAMETOCLIP)
			{
				str2Cliboard(buf->getFileName());
			}
		}
		break;

		case IDM_SEARCH_FIND :
		case IDM_SEARCH_REPLACE :
		case IDM_SEARCH_MARK :
		{
			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];

			bool isFirstTime = !_findReplaceDlg.isCreated();
			
			DIALOG_TYPE dlgID = FIND_DLG;
			if (id == IDM_SEARCH_REPLACE)
				dlgID = REPLACE_DLG;
			else if (id == IDM_SEARCH_MARK)
				dlgID = MARK_DLG;
			_findReplaceDlg.doDialog(dlgID, _nativeLangSpeaker.isRTL());

			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str);
			setFindReplaceFolderFilter(NULL, NULL);

			if (isFirstTime)
				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);
			break;
		}

		case IDM_SEARCH_FINDINFILES:
		{
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_LAUNCHFINDINFILESDLG, 0, 0);
			break;
		}

		case IDM_SEARCH_FINDINCREMENT :
		{
			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];

			_pEditView->getGenericSelectedText(str, strSize, false);
			if (0 != str[0])         // the selected text is not empty, then use it
				_incrementFindDlg.setSearchText(str, _pEditView->getCurrentBuffer()->getUnicodeMode() != uni8Bit);

			_incrementFindDlg.display();
		}
		break;

		case IDM_SEARCH_FINDNEXT :
		case IDM_SEARCH_FINDPREV :
		{
			if (!_findReplaceDlg.isCreated())
				return;

			FindOption op = _findReplaceDlg.getCurrentOptions();
			op._whichDirection = (id == IDM_SEARCH_FINDNEXT?DIR_DOWN:DIR_UP);
			generic_string s = _findReplaceDlg.getText2search();
			FindStatus status = FSNoMessage;
			_findReplaceDlg.processFindNext(s.c_str(), &op, &status);
			if (status == FSEndReached)
				_findReplaceDlg.setStatusbarMessage(TEXT("Find: Found the 1st occurrence from the top. The end of document has been reached."), FSEndReached);
			else if (status == FSTopReached)
				_findReplaceDlg.setStatusbarMessage(TEXT("Find: Found the 1st occurrence from the bottom. The begin of document has been reached."), FSTopReached);
			break;
		}
		break;

        case IDM_SEARCH_SETANDFINDNEXT :
		case IDM_SEARCH_SETANDFINDPREV :
        {
            bool isFirstTime = !_findReplaceDlg.isCreated();
			if (isFirstTime)
				_findReplaceDlg.doDialog(FIND_DLG, _nativeLangSpeaker.isRTL(), false);

			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];
			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str);
			_findReplaceDlg._env->_str2Search = str;
			setFindReplaceFolderFilter(NULL, NULL);
			if (isFirstTime)
				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);

			FindOption op = _findReplaceDlg.getCurrentOptions();
			op._whichDirection = (id == IDM_SEARCH_SETANDFINDNEXT?DIR_DOWN:DIR_UP);

			FindStatus status = FSNoMessage;
			_findReplaceDlg.processFindNext(str, &op, &status);
			if (status == FSEndReached)
				_findReplaceDlg.setStatusbarMessage(TEXT("Find: Found the 1st occurrence from the top. The end of document has been reached."), FSEndReached);
			else if (status == FSTopReached)
				_findReplaceDlg.setStatusbarMessage(TEXT("Find: Found the 1st occurrence from the bottom. The begin of document has been reached."), FSTopReached);
			break;
        }

		case IDM_SEARCH_GOTONEXTFOUND:
		{
			_findReplaceDlg.gotoNextFoundResult();
			break;
		}
		case IDM_SEARCH_GOTOPREVFOUND:
		{
			_findReplaceDlg.gotoNextFoundResult(-1);
			break;
		}
		case IDM_FOCUS_ON_FOUND_RESULTS:
		{
			if (GetFocus() == _findReplaceDlg.getHFindResults())
				// focus already on found results, switch to current edit view
				switchEditViewTo(currentView());
			else
				_findReplaceDlg.focusOnFinder();
			break;
		}

		case IDM_SEARCH_VOLATILE_FINDNEXT :
		case IDM_SEARCH_VOLATILE_FINDPREV :
		{
			TCHAR text2Find[MAX_PATH];
			_pEditView->getGenericSelectedText(text2Find, MAX_PATH);

			FindOption op;
			op._isWholeWord = false;
			op._whichDirection = (id == IDM_SEARCH_VOLATILE_FINDNEXT?DIR_DOWN:DIR_UP);

			FindStatus status = FSNoMessage;
			_findReplaceDlg.processFindNext(text2Find, &op, &status);
			if (status == FSEndReached)
				_findReplaceDlg.setStatusbarMessage(TEXT("Find: Found the 1st occurrence from the top. The end of document has been reached."), FSEndReached);
			else if (status == FSTopReached)
				_findReplaceDlg.setStatusbarMessage(TEXT("Find: Found the 1st occurrence from the bottom. The begin of document has been reached."), FSTopReached);

			break;
		}

		case IDM_SEARCH_MARKALLEXT1 :
		case IDM_SEARCH_MARKALLEXT2 :
		case IDM_SEARCH_MARKALLEXT3 :
		case IDM_SEARCH_MARKALLEXT4 :
		case IDM_SEARCH_MARKALLEXT5 :
		{
			int styleID;
			if (id == IDM_SEARCH_MARKALLEXT1)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT1;
			else if (id == IDM_SEARCH_MARKALLEXT2)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT2;
			else if (id == IDM_SEARCH_MARKALLEXT3)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT3;
			else if (id == IDM_SEARCH_MARKALLEXT4)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT4;
			else // (id == IDM_SEARCH_MARKALLEXT5)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT5;

			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR text2Find[strSize];
			TCHAR text2Find2[strSize];

			_pEditView->getGenericSelectedText(text2Find, strSize, false);
			_pEditView->getGenericWordOnCaretPos(text2Find2, strSize);

            if (text2Find[0] == '\0')
            {
                _findReplaceDlg.markAll(text2Find2, styleID, true);
            }
			else
			{
				_findReplaceDlg.markAll(text2Find, styleID, lstrlen(text2Find) == lstrlen(text2Find2));
			}
			break;
		}
		case IDM_SEARCH_UNMARKALLEXT1 :
		case IDM_SEARCH_UNMARKALLEXT2 :
		case IDM_SEARCH_UNMARKALLEXT3 :
		case IDM_SEARCH_UNMARKALLEXT4 :
		case IDM_SEARCH_UNMARKALLEXT5 :
		{
			int styleID;
			if (id == IDM_SEARCH_UNMARKALLEXT1)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT1;
			else if (id == IDM_SEARCH_UNMARKALLEXT2)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT2;
			else if (id == IDM_SEARCH_UNMARKALLEXT3)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT3;
			else if (id == IDM_SEARCH_UNMARKALLEXT4)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT4;
			else // (id == IDM_SEARCH_UNMARKALLEXT5)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT5;

			_pEditView->clearIndicator(styleID);
			break;
		}

		case IDM_SEARCH_GONEXTMARKER1 :
		case IDM_SEARCH_GONEXTMARKER2 :
		case IDM_SEARCH_GONEXTMARKER3 :
		case IDM_SEARCH_GONEXTMARKER4 :
		case IDM_SEARCH_GONEXTMARKER5 :
		case IDM_SEARCH_GONEXTMARKER_DEF :
		{
			int styleID;
			if (id == IDM_SEARCH_GONEXTMARKER1)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT1;
			else if (id == IDM_SEARCH_GONEXTMARKER2)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT2;
			else if (id == IDM_SEARCH_GONEXTMARKER3)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT3;
			else if (id == IDM_SEARCH_GONEXTMARKER4)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT4;
			else if (id == IDM_SEARCH_GONEXTMARKER5)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT5;
			else // (id == IDM_SEARCH_GONEXTMARKER_DEF)
				styleID = SCE_UNIVERSAL_FOUND_STYLE;

			goToNextIndicator(styleID);

			break;
		}
		
		case IDM_SEARCH_GOPREVMARKER1 :
		case IDM_SEARCH_GOPREVMARKER2 :
		case IDM_SEARCH_GOPREVMARKER3 :
		case IDM_SEARCH_GOPREVMARKER4 :
		case IDM_SEARCH_GOPREVMARKER5 :
		case IDM_SEARCH_GOPREVMARKER_DEF :
		{
			int styleID;
			if (id == IDM_SEARCH_GOPREVMARKER1)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT1;
			else if (id == IDM_SEARCH_GOPREVMARKER2)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT2;
			else if (id == IDM_SEARCH_GOPREVMARKER3)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT3;
			else if (id == IDM_SEARCH_GOPREVMARKER4)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT4;
			else if (id == IDM_SEARCH_GOPREVMARKER5)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT5;
			else // (id == IDM_SEARCH_GOPREVMARKER_DEF)
				styleID = SCE_UNIVERSAL_FOUND_STYLE;

			goToPreviousIndicator(styleID);

			break;
		}

		case IDM_SEARCH_CLEARALLMARKS :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT1);
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT2);
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT3);
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT4);
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT5);
			break;
		}

        case IDM_SEARCH_GOTOLINE :
		{
			bool isFirstTime = !_goToLineDlg.isCreated();
			_goToLineDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_goToLineDlg.getHSelf(), "GoToLine");
			break;
		}

		case IDM_SEARCH_FINDCHARINRANGE :
		{
			bool isFirstTime = !_findCharsInRangeDlg.isCreated();
			_findCharsInRangeDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_findCharsInRangeDlg.getHSelf(), "FindCharsInRange");
			break;
		}

        case IDM_EDIT_COLUMNMODETIP :
		{
			_nativeLangSpeaker.messageBox("ColumnModeTip",
					_pPublicInterface->getHSelf(),
					TEXT("Please use \"ALT+Mouse Selection\" or \"Alt+Shift+Arrow key\" to switch to column mode."),
					TEXT("Column Mode Tip"),
					MB_OK|MB_APPLMODAL);
			break;
		}

        case IDM_EDIT_COLUMNMODE :
		{
			bool isFirstTime = !_colEditorDlg.isCreated();
			_colEditorDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_colEditorDlg.getHSelf(), "ColumnEditor");
			break;
		}

		case IDM_SEARCH_GOTOMATCHINGBRACE :
		case IDM_SEARCH_SELECTMATCHINGBRACES :
		{
			int braceAtCaret = -1;
			int braceOpposite = -1;
			findMatchingBracePos(braceAtCaret, braceOpposite);

			if (braceOpposite != -1)
			{
				if(id == IDM_SEARCH_GOTOMATCHINGBRACE)
					_pEditView->execute(SCI_GOTOPOS, braceOpposite);
				else
					_pEditView->execute(SCI_SETSEL, min(braceAtCaret, braceOpposite), max(braceAtCaret, braceOpposite) + 1); // + 1 so we always include the ending brace in the selection.
			}
			break;
		}

        case IDM_SEARCH_TOGGLE_BOOKMARK :
	        bookmarkToggle(-1);
            break;

	    case IDM_SEARCH_NEXT_BOOKMARK:
		    bookmarkNext(true);
		    break;

	    case IDM_SEARCH_PREV_BOOKMARK:
		    bookmarkNext(false);
		    break;

	    case IDM_SEARCH_CLEAR_BOOKMARKS:
			bookmarkClearAll();
		    break;
			
        case IDM_LANG_USER_DLG :
        {
		    bool isUDDlgVisible = false;
                
		    UserDefineDialog *udd = _pEditView->getUserDefineDlg();

		    if (!udd->isCreated())
		    {
			    _pEditView->doUserDefineDlg(true, _nativeLangSpeaker.isRTL());
				_nativeLangSpeaker.changeUserDefineLang(udd);
				if (_isUDDocked)
					::SendMessage(udd->getHSelf(), WM_COMMAND, IDC_DOCK_BUTTON, 0);

		    }
			else
			{
				isUDDlgVisible = udd->isVisible();
				bool isUDDlgDocked = udd->isDocked();

				if ((isUDDlgDocked)&&(isUDDlgVisible))
				{
					::ShowWindow(_pMainSplitter->getHSelf(), SW_HIDE);

					if (bothActive())
						_pMainWindow = &_subSplitter;
					else
						_pMainWindow = _pDocTab;

					::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
					
					udd->display(false);
					_mainWindowStatus &= ~WindowUserActive;
				}
				else if ((isUDDlgDocked)&&(!isUDDlgVisible))
				{
                    if (!_pMainSplitter)
                    {
                        _pMainSplitter = new SplitterContainer;
                        _pMainSplitter->init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf());

                        Window *pWindow;
                        if (bothActive())
                            pWindow = &_subSplitter;
                        else
                            pWindow = _pDocTab;

                        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), 8, RIGHT_FIX, 45); 
                    }

					_pMainWindow = _pMainSplitter;

					_pMainSplitter->setWin0((bothActive())?(Window *)&_subSplitter:(Window *)_pDocTab);

					::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
					_pMainWindow->display();

					_mainWindowStatus |= WindowUserActive;
				}
				else if ((!isUDDlgDocked)&&(isUDDlgVisible))
				{
					udd->display(false);
				}
				else //((!isUDDlgDocked)&&(!isUDDlgVisible))
					udd->display();
			}
			checkMenuItem(IDM_LANG_USER_DLG, !isUDDlgVisible);
			_toolBar.setCheck(IDM_LANG_USER_DLG, !isUDDlgVisible);

            break;
        }

		case IDM_EDIT_SELECTALL:
			_pEditView->execute(SCI_SELECTALL);
			checkClipboard();
			break;

		case IDM_EDIT_INS_TAB:
			_pEditView->execute(SCI_TAB);
			break;

		case IDM_EDIT_RMV_TAB:
			_pEditView->execute(SCI_BACKTAB);
			break;

		case IDM_EDIT_DUP_LINE:
			_pEditView->execute(SCI_LINEDUPLICATE);
			break;

		case IDM_EDIT_SPLIT_LINES:
			_pEditView->execute(SCI_TARGETFROMSELECTION);
			if (_pEditView->execute(SCI_GETEDGEMODE) == EDGE_NONE)
				_pEditView->execute(SCI_LINESSPLIT);
			else
				_pEditView->execute(SCI_LINESSPLIT, _pEditView->execute(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"P") * _pEditView->execute(SCI_GETEDGECOLUMN));
			break;

		case IDM_EDIT_JOIN_LINES:
			_pEditView->execute(SCI_TARGETFROMSELECTION);
			_pEditView->execute(SCI_LINESJOIN);
			break;

		case IDM_EDIT_LINE_UP:
			_pEditView->currentLinesUp();
			break;

		case IDM_EDIT_LINE_DOWN:
			_pEditView->currentLinesDown();
			break;

		case IDM_EDIT_REMOVEEMPTYLINES:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			removeEmptyLine(false);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_REMOVEEMPTYLINESWITHBLANK:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			removeEmptyLine(true);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_UPPERCASE:
            _pEditView->convertSelectedTextToUpperCase();
			break;

		case IDM_EDIT_LOWERCASE:
            _pEditView->convertSelectedTextToLowerCase();
			break;

		case IDM_EDIT_BLOCK_COMMENT:
			doBlockComment(cm_toggle);
 			break;
 
		case IDM_EDIT_BLOCK_COMMENT_SET:
			doBlockComment(cm_comment);
			break;

		case IDM_EDIT_BLOCK_UNCOMMENT:
			doBlockComment(cm_uncomment);
			break;

		case IDM_EDIT_STREAM_COMMENT:
			doStreamComment();
			break;

		case IDM_EDIT_STREAM_UNCOMMENT:
			undoStreamComment();
			break;

		case IDM_EDIT_TRIMTRAILING:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			doTrim(lineTail);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_TRIMLINEHEAD:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			doTrim(lineHeader);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_TRIM_BOTH:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			doTrim(lineTail);
			doTrim(lineHeader);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_EOL2WS:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			_pEditView->execute(SCI_SETTARGETSTART, 0);
			_pEditView->execute(SCI_SETTARGETEND, _pEditView->getCurrentDocLen());
			_pEditView->execute(SCI_LINESJOIN);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_TRIMALL:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			doTrim(lineTail);
			doTrim(lineHeader);
			_pEditView->execute(SCI_SETTARGETSTART, 0);
			_pEditView->execute(SCI_SETTARGETEND, _pEditView->getCurrentDocLen());
			_pEditView->execute(SCI_LINESJOIN);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_TAB2SW:
			wsTabConvert(tab2Space);
			break;

		case IDM_EDIT_SW2TAB_LEADING:
			wsTabConvert(space2TabLeading);
			break;

		case IDM_EDIT_SW2TAB_ALL:
			wsTabConvert(space2TabAll);
			break;

		case IDM_EDIT_SETREADONLY:
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			buf->setUserReadOnly(!buf->getUserReadOnly());
		}
		break;

		case IDM_EDIT_CLEARREADONLY:
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			
			DWORD dwFileAttribs = ::GetFileAttributes(buf->getFullPathName());
			dwFileAttribs ^= FILE_ATTRIBUTE_READONLY; 

			::SetFileAttributes(buf->getFullPathName(), dwFileAttribs); 
			buf->setFileReadOnly(false);
		}
		break;

		case IDM_SEARCH_CUTMARKEDLINES :
			cutMarkedLines();
			break;

		case IDM_SEARCH_COPYMARKEDLINES :
			copyMarkedLines();
			break;

		case IDM_SEARCH_PASTEMARKEDLINES :
			pasteToMarkedLines();
			break;

		case IDM_SEARCH_DELETEMARKEDLINES :
			deleteMarkedLines(true);
			break;

		case IDM_SEARCH_DELETEUNMARKEDLINES :
			deleteMarkedLines(false);
			break;

		case IDM_SEARCH_INVERSEMARKS :
			inverseMarks();
			break;

		case IDM_VIEW_FULLSCREENTOGGLE :
			fullScreenToggle();
			break;

	    case IDM_VIEW_ALWAYSONTOP:
		{
			int check = (::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED)?MF_UNCHECKED:MF_CHECKED;
			::CheckMenuItem(_mainMenuHandle, id, MF_BYCOMMAND | check);
			SetWindowPos(_pPublicInterface->getHSelf(), check == MF_CHECKED?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		}
		break;


		case IDM_VIEW_FOLD_CURRENT :
		case IDM_VIEW_UNFOLD_CURRENT :
			_pEditView->foldCurrentPos((id==IDM_VIEW_FOLD_CURRENT)?fold_collapse:fold_uncollapse);
			break;

		case IDM_VIEW_TOGGLE_FOLDALL:
		case IDM_VIEW_TOGGLE_UNFOLDALL:
		{
			_isFolding = true; // So we can ignore events while folding is taking place
			bool doCollapse = (id==IDM_VIEW_TOGGLE_FOLDALL)?fold_collapse:fold_uncollapse;
 			_pEditView->foldAll(doCollapse);
			if (_pDocMap)
			{
				_pDocMap->foldAll(doCollapse);
			}
			_isFolding = false;
		}
		break;

		case IDM_VIEW_FOLD_1:
		case IDM_VIEW_FOLD_2:
		case IDM_VIEW_FOLD_3:
		case IDM_VIEW_FOLD_4:
		case IDM_VIEW_FOLD_5:
		case IDM_VIEW_FOLD_6:
		case IDM_VIEW_FOLD_7:
		case IDM_VIEW_FOLD_8:
			_isFolding = true; // So we can ignore events while folding is taking place
 			_pEditView->collapse(id - IDM_VIEW_FOLD - 1, fold_collapse);
			_isFolding = false;
			break;

		case IDM_VIEW_UNFOLD_1:
		case IDM_VIEW_UNFOLD_2:
		case IDM_VIEW_UNFOLD_3:
		case IDM_VIEW_UNFOLD_4:
		case IDM_VIEW_UNFOLD_5:
		case IDM_VIEW_UNFOLD_6:
		case IDM_VIEW_UNFOLD_7:
		case IDM_VIEW_UNFOLD_8:
			_isFolding = true; // So we can ignore events while folding is taking place
 			_pEditView->collapse(id - IDM_VIEW_UNFOLD - 1, fold_uncollapse);
			_isFolding = false;
			break;


		case IDM_VIEW_TOOLBAR_REDUCE:
		{
            toolBarStatusType state = _toolBar.getState();

            if (state != TB_SMALL)
            {
			    _toolBar.reduce();
			    changeToolBarIcons();
            }
		}
		break;

		case IDM_VIEW_TOOLBAR_ENLARGE:
		{
            toolBarStatusType state = _toolBar.getState();

            if (state != TB_LARGE)
            {
			    _toolBar.enlarge();
			    changeToolBarIcons();
            }
		}
		break;

		case IDM_VIEW_TOOLBAR_STANDARD:
		{
			toolBarStatusType state = _toolBar.getState();

            if (state != TB_STANDARD)
            {
				_toolBar.setToUglyIcons();
			}
		}
		break;

		case IDM_VIEW_REDUCETABBAR :
		{
			_toReduceTabBar = !_toReduceTabBar;

			//Resize the  icon
			int iconDpiDynamicalSize = NppParameters::getInstance()->_dpiManager.scaleY(_toReduceTabBar?12:18);

			//Resize the tab height
			int tabDpiDynamicalWidth = NppParameters::getInstance()->_dpiManager.scaleX(45);
			int tabDpiDynamicalHeight = NppParameters::getInstance()->_dpiManager.scaleY(_toReduceTabBar?20:25);
			TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			_docTabIconList.setIconSize(iconDpiDynamicalSize);

			//change the font
			int stockedFont = _toReduceTabBar?DEFAULT_GUI_FONT:SYSTEM_FONT;
			HFONT hf = (HFONT)::GetStockObject(stockedFont);

			if (hf)
			{
				::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
				::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
			}

			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			break;
		}
		
		case IDM_VIEW_REFRESHTABAR :
		{
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			break;
		}
        case IDM_VIEW_LOCKTABBAR:
		{
			bool isDrag = TabBarPlus::doDragNDropOrNot();
            TabBarPlus::doDragNDrop(!isDrag);
            break;
		}


		case IDM_VIEW_DRAWTABBAR_INACIVETAB:
		{
			TabBarPlus::setDrawInactiveTab(!TabBarPlus::drawInactiveTab());
			break;
		}
		case IDM_VIEW_DRAWTABBAR_TOPBAR:
		{
			TabBarPlus::setDrawTopBar(!TabBarPlus::drawTopBar());
			break;
		}

		case IDM_VIEW_DRAWTABBAR_CLOSEBOTTUN :
		{
			TabBarPlus::setDrawTabCloseButton(!TabBarPlus::drawTabCloseButton());

			// This part is just for updating (redraw) the tabs
			{	
				int tabDpiDynamicalHeight = NppParameters::getInstance()->_dpiManager.scaleY(TabBarPlus::drawTabCloseButton()?21:20);
				int tabDpiDynamicalWidth = NppParameters::getInstance()->_dpiManager.scaleX(TabBarPlus::drawTabCloseButton() ? 60:45);
				TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
				TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			}
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			break;
		}

		case IDM_VIEW_DRAWTABBAR_DBCLK2CLOSE :
		{
			TabBarPlus::setDbClk2Close(!TabBarPlus::isDbClk2Close());
			break;
		}
		
		case IDM_VIEW_DRAWTABBAR_VERTICAL :
		{
			TabBarPlus::setVertical(!TabBarPlus::isVertical());
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			break;
		}
		
		case IDM_VIEW_DRAWTABBAR_MULTILINE :
		{
			TabBarPlus::setMultiLine(!TabBarPlus::isMultiLine());
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);		
			break;
		}

		case IDM_VIEW_POSTIT :
		{
			postItToggle();
		}
		break;

		case IDM_VIEW_TAB_SPACE:
		{
			bool isChecked = !(::GetMenuState(_mainMenuHandle, IDM_VIEW_TAB_SPACE, MF_BYCOMMAND) == MF_CHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_EOL, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_ALL_CHARACTERS, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_TAB_SPACE, MF_BYCOMMAND | (isChecked?MF_CHECKED:MF_UNCHECKED));
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, false);
			_mainEditView.showEOL(false);
			_mainEditView.showWSAndTab(isChecked);
			_subEditView.showEOL(false);
			_subEditView.showWSAndTab(isChecked);

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance())->getSVP();
            svp1._whiteSpaceShow = isChecked;
            svp1._eolShow = false;
			break;
		}
		case IDM_VIEW_EOL:
		{
			bool isChecked = !(::GetMenuState(_mainMenuHandle, IDM_VIEW_EOL, MF_BYCOMMAND) == MF_CHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_TAB_SPACE, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_EOL, MF_BYCOMMAND | (isChecked?MF_CHECKED:MF_UNCHECKED));
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_ALL_CHARACTERS, MF_BYCOMMAND | MF_UNCHECKED);
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, false);
			_mainEditView.showEOL(isChecked);
			_subEditView.showEOL(isChecked);
			_mainEditView.showWSAndTab(false);
			_subEditView.showWSAndTab(false);

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance())->getSVP();
            svp1._whiteSpaceShow = false;
            svp1._eolShow = isChecked;
			break;
		}
		case IDM_VIEW_ALL_CHARACTERS:
		{
			bool isChecked = !(::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_EOL, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_TAB_SPACE, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_ALL_CHARACTERS, MF_BYCOMMAND | (isChecked?MF_CHECKED:MF_UNCHECKED));
			_mainEditView.showInvisibleChars(isChecked);
			_subEditView.showInvisibleChars(isChecked);
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, isChecked);

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance())->getSVP();
            svp1._whiteSpaceShow = isChecked;
            svp1._eolShow = isChecked;
			break;
		}

		case IDM_VIEW_INDENT_GUIDE:
		{
			_mainEditView.showIndentGuideLine(!_pEditView->isShownIndentGuide());
			_subEditView.showIndentGuideLine(!_pEditView->isShownIndentGuide());
            _toolBar.setCheck(IDM_VIEW_INDENT_GUIDE, _pEditView->isShownIndentGuide());
			checkMenuItem(IDM_VIEW_INDENT_GUIDE, _pEditView->isShownIndentGuide());

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance())->getSVP();
            svp1._indentGuideLineShow = _pEditView->isShownIndentGuide();
			break;
		}

		case IDM_VIEW_WRAP:
		{
			bool isWraped = !_pEditView->isWrap();
			//--FLS: ViewMoveAtWrappingDisableFix: Disable wrapping messes up visible lines. Therefore save view position before in IDM_VIEW_WRAP and restore after SCN_PAINTED, as Scintilla-Doc. says
			if (!isWraped)
			{
				_mainEditView.saveCurrentPos();
				_mainEditView.setWrapRestoreNeeded(true);
				_subEditView.saveCurrentPos();
				_subEditView.setWrapRestoreNeeded(true);
			}
			_mainEditView.wrap(isWraped);
			_subEditView.wrap(isWraped);
			_toolBar.setCheck(IDM_VIEW_WRAP, isWraped);
			checkMenuItem(IDM_VIEW_WRAP, isWraped);

			ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance())->getSVP();
			svp1._doWrap = isWraped;

			if (_pDocMap)
			{
				_pDocMap->initWrapMap();
				_pDocMap->wrapMap();
			}
			break;
		}
		case IDM_VIEW_WRAP_SYMBOL:
		{
			_mainEditView.showWrapSymbol(!_pEditView->isWrapSymbolVisible());
			_subEditView.showWrapSymbol(!_pEditView->isWrapSymbolVisible());
			checkMenuItem(IDM_VIEW_WRAP_SYMBOL, _pEditView->isWrapSymbolVisible());

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance())->getSVP();
            svp1._wrapSymbolShow = _pEditView->isWrapSymbolVisible();
			break;
		}

		case IDM_VIEW_HIDELINES:
		{
			_pEditView->hideLines();
			break;
		}

		case IDM_VIEW_ZOOMIN:
		{
			_pEditView->execute(SCI_ZOOMIN);
			break;
		}
		case IDM_VIEW_ZOOMOUT:
			_pEditView->execute(SCI_ZOOMOUT);
			break;

		case IDM_VIEW_ZOOMRESTORE:
			//Zoom factor of 0 points means default view
			_pEditView->execute(SCI_SETZOOM, 0);//_zoomOriginalValue);
			break;

		case IDM_VIEW_SYNSCROLLV:
		{
            bool isSynScollV = !_syncInfo._isSynScollV;
			
			checkMenuItem(IDM_VIEW_SYNSCROLLV, isSynScollV);
			_toolBar.setCheck(IDM_VIEW_SYNSCROLLV, isSynScollV);

            _syncInfo._isSynScollV = isSynScollV;
			if (_syncInfo._isSynScollV)
			{
				int mainCurrentLine = _mainEditView.execute(SCI_GETFIRSTVISIBLELINE);
				int subCurrentLine = _subEditView.execute(SCI_GETFIRSTVISIBLELINE);
				_syncInfo._line = mainCurrentLine - subCurrentLine;
			}
			
		}
		break;

		case IDM_VIEW_SYNSCROLLH:
		{
            bool isSynScollH = !_syncInfo._isSynScollH;
			checkMenuItem(IDM_VIEW_SYNSCROLLH, isSynScollH);
			_toolBar.setCheck(IDM_VIEW_SYNSCROLLH, isSynScollH);

            _syncInfo._isSynScollH = isSynScollH;
			if (_syncInfo._isSynScollH)
			{
				int mxoffset = _mainEditView.execute(SCI_GETXOFFSET);
				int pixel = int(_mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
				int mainColumn = mxoffset/pixel;

				int sxoffset = _subEditView.execute(SCI_GETXOFFSET);
				pixel = int(_subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
				int subColumn = sxoffset/pixel;
				_syncInfo._column = mainColumn - subColumn;
			}
		}
		break;

		case IDM_VIEW_SUMMARY:
		{
			generic_string characterNumber = TEXT("");

			Buffer * curBuf = _pEditView->getCurrentBuffer();
			int fileLen = curBuf->getFileLength();

			if (fileLen != -1)
			{
				TCHAR *filePathLabel = TEXT("Full file path: ");
				TCHAR *fileCreateTimeLabel = TEXT("Created: ");
				TCHAR *fileModifyTimeLabel = TEXT("Modified: ");
				TCHAR *fileLenLabel = TEXT("File length (in byte): ");

				characterNumber += filePathLabel;
				characterNumber += curBuf->getFullPathName();
				characterNumber += TEXT("\r");

				characterNumber += fileCreateTimeLabel;
				characterNumber += curBuf->getFileTime(Buffer::ft_created);
				characterNumber += TEXT("\r");

				characterNumber += fileModifyTimeLabel;
				characterNumber += curBuf->getFileTime(Buffer::ft_modified);
				characterNumber += TEXT("\r");

				TCHAR fileLenStr[64];
				generic_sprintf(fileLenStr, TEXT("%I64u"), static_cast<UINT64>( fileLen ) );
				characterNumber += fileLenLabel;
				characterNumber += fileLenStr;
				characterNumber += TEXT("\r");
				characterNumber += TEXT("\r");
			}
			TCHAR *nbCharLabel = TEXT("Characters (without blanks): ");
			TCHAR *nbWordLabel = TEXT("Words: ");
			TCHAR *nbLineLabel = TEXT("Lines: ");
			TCHAR *nbByteLabel = TEXT("Current document length: ");
			TCHAR *nbSelLabel1 = TEXT(" selected characters (");
			TCHAR *nbSelLabel2 = TEXT(" bytes) in ");
			TCHAR *nbRangeLabel = TEXT(" ranges");

			UniMode um = _pEditView->getCurrentBuffer()->getUnicodeMode();
			int nbChar = getCurrentDocCharCount(um);
			int nbWord = wordCount();
			size_t nbLine = _pEditView->execute(SCI_GETLINECOUNT);
			int nbByte = _pEditView->execute(SCI_GETLENGTH);
			int nbSel = getSelectedCharNumber(um);
			int nbSelByte = getSelectedBytes();
			int nbRange = getSelectedAreas();

			TCHAR nbCharStr[32];
			TCHAR nbWordStr[16];
			TCHAR nbByteStr[32];
			TCHAR nbLineStr[32];
			TCHAR nbSelStr[32];
			TCHAR nbSelByteStr[32];
			TCHAR nbRangeStr[8];

			generic_sprintf(nbCharStr, TEXT("%d"), nbChar);
			characterNumber += nbCharLabel;
			characterNumber += nbCharStr;
			characterNumber += TEXT("\r");

			generic_sprintf(nbWordStr, TEXT("%d"), nbWord);
			characterNumber += nbWordLabel;
			characterNumber += nbWordStr;
			characterNumber += TEXT("\r");

			generic_sprintf(nbLineStr, TEXT("%d"), static_cast<int>( nbLine ) );
			characterNumber += nbLineLabel;
			characterNumber += nbLineStr;
			characterNumber += TEXT("\r");

			generic_sprintf(nbByteStr, TEXT("%d"), nbByte);
			characterNumber += nbByteLabel;
			characterNumber += nbByteStr;
			characterNumber += TEXT("\r");

			generic_sprintf(nbSelStr, TEXT("%d"), nbSel);
			generic_sprintf(nbSelByteStr, TEXT("%d"), nbSelByte);
			generic_sprintf(nbRangeStr, TEXT("%d"), nbRange);
			characterNumber += nbSelStr;
			characterNumber += nbSelLabel1;
			characterNumber += nbSelByteStr;
			characterNumber += nbSelLabel2;
			characterNumber += nbRangeStr;
			characterNumber += nbRangeLabel;
			characterNumber += TEXT("\r");

			::MessageBox(_pPublicInterface->getHSelf(), characterNumber.c_str(), TEXT("Summary"), MB_OK|MB_APPLMODAL);
		}
		break;

		case IDM_EXECUTE:
		{
			bool isFirstTime = !_runDlg.isCreated();
			_runDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_runDlg.getHSelf(), "Run");

			break;
		}

		case IDM_FORMAT_TODOS :
		case IDM_FORMAT_TOUNIX :
		case IDM_FORMAT_TOMAC :
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			
			int f = int((id == IDM_FORMAT_TODOS)?SC_EOL_CRLF:(id == IDM_FORMAT_TOUNIX)?SC_EOL_LF:SC_EOL_CR);

			buf->setFormat((formatType)f);
			_pEditView->execute(SCI_CONVERTEOLS, buf->getFormat());
			break;
		}

		case IDM_FORMAT_ANSI :
		case IDM_FORMAT_UTF_8 :	
		case IDM_FORMAT_UCS_2BE :
		case IDM_FORMAT_UCS_2LE :
		case IDM_FORMAT_AS_UTF_8 :
		{
			Buffer * buf = _pEditView->getCurrentBuffer();

			UniMode um;
			bool shoulBeDirty = true;
			switch (id)
			{
				case IDM_FORMAT_AS_UTF_8:
					shoulBeDirty = buf->getUnicodeMode() != uni8Bit;
					um = uniCookie;
					break;
				
				case IDM_FORMAT_UTF_8:
					um = uniUTF8;
					break;

				case IDM_FORMAT_UCS_2BE:
					um = uni16BE;
					break;

				case IDM_FORMAT_UCS_2LE:
					um = uni16LE;
					break;

				default : // IDM_FORMAT_ANSI
					shoulBeDirty = buf->getUnicodeMode() != uniCookie;
					um = uni8Bit;
			}

			if (buf->getEncoding() != -1)
			{
				if (buf->isDirty())
				{
					int answer = _nativeLangSpeaker.messageBox("SaveCurrentModifWarning",
						NULL,
						TEXT("You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?"),
						TEXT("Save Current Modification"),
						MB_YESNO);

					if (answer == IDYES)
					{
						fileSave();
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
					}
					else
						return;
				}

				if (_pEditView->execute(SCI_CANUNDO) == TRUE)
				{
					generic_string msg, title;
					int answer = _nativeLangSpeaker.messageBox("LoseUndoAbilityWarning",
						NULL,
						TEXT("You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?"),
						TEXT("Lose Undo Ability Waning"),
						MB_YESNO);
					if (answer == IDYES)
					{
						// Do nothing
					}
					else
						return;
				}

				buf->setEncoding(-1);

				if (um == uni8Bit)
					_pEditView->execute(SCI_SETCODEPAGE, CP_ACP);
				else
					buf->setUnicodeMode(um);
				fileReload();
			}
			else
			{
				if (buf->getUnicodeMode() != um)
				{
					buf->setUnicodeMode(um);
					if (shoulBeDirty)
						buf->setDirty(true);
				}
			}
			break;
		}

        case IDM_FORMAT_WIN_1250 :
        case IDM_FORMAT_WIN_1251 :
        case IDM_FORMAT_WIN_1252 :
        case IDM_FORMAT_WIN_1253 :
        case IDM_FORMAT_WIN_1254 :
        case IDM_FORMAT_WIN_1255 :
        case IDM_FORMAT_WIN_1256 :
        case IDM_FORMAT_WIN_1257 :
        case IDM_FORMAT_WIN_1258 :
        case IDM_FORMAT_ISO_8859_1  :
        case IDM_FORMAT_ISO_8859_2  :
        case IDM_FORMAT_ISO_8859_3  :
        case IDM_FORMAT_ISO_8859_4  :
        case IDM_FORMAT_ISO_8859_5  :
        case IDM_FORMAT_ISO_8859_6  :
        case IDM_FORMAT_ISO_8859_7  :
        case IDM_FORMAT_ISO_8859_8  :
        case IDM_FORMAT_ISO_8859_9  :
        case IDM_FORMAT_ISO_8859_10 :
        case IDM_FORMAT_ISO_8859_11 :
        case IDM_FORMAT_ISO_8859_13 :
        case IDM_FORMAT_ISO_8859_14 :
        case IDM_FORMAT_ISO_8859_15 :
        case IDM_FORMAT_ISO_8859_16 :
        case IDM_FORMAT_DOS_437 :
        case IDM_FORMAT_DOS_720 :
        case IDM_FORMAT_DOS_737 :
        case IDM_FORMAT_DOS_775 :
        case IDM_FORMAT_DOS_850 :
        case IDM_FORMAT_DOS_852 :
        case IDM_FORMAT_DOS_855 :
        case IDM_FORMAT_DOS_857 :
        case IDM_FORMAT_DOS_858 :
        case IDM_FORMAT_DOS_860 :
        case IDM_FORMAT_DOS_861 :
        case IDM_FORMAT_DOS_862 :
        case IDM_FORMAT_DOS_863 :
        case IDM_FORMAT_DOS_865 :
        case IDM_FORMAT_DOS_866 :
        case IDM_FORMAT_DOS_869 :
        case IDM_FORMAT_BIG5 :
        case IDM_FORMAT_GB2312 :
        case IDM_FORMAT_SHIFT_JIS :
        case IDM_FORMAT_KOREAN_WIN :
        case IDM_FORMAT_EUC_KR :
        case IDM_FORMAT_TIS_620 :
        case IDM_FORMAT_MAC_CYRILLIC : 
        case IDM_FORMAT_KOI8U_CYRILLIC :
        case IDM_FORMAT_KOI8R_CYRILLIC :
        {
			int index = id - IDM_FORMAT_ENCODE;

			EncodingMapper *em = EncodingMapper::getInstance();
			int encoding = em->getEncodingFromIndex(index);
			if (encoding == -1)
			{
				//printStr(TEXT("Encoding problem. Command is not added in encoding_table?"));
				return;
			}

            Buffer * buf = _pEditView->getCurrentBuffer();
            if (buf->isDirty())
            {
				generic_string warning, title;
				int answer = _nativeLangSpeaker.messageBox("SaveCurrentModifWarning",
					NULL,
					TEXT("You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?"),
					TEXT("Save Current Modification"),
					MB_YESNO);

                if (answer == IDYES)
                {
                    fileSave();
					_pEditView->execute(SCI_EMPTYUNDOBUFFER);
                }
                else
                    return;
            }

            if (_pEditView->execute(SCI_CANUNDO) == TRUE)
            {
				generic_string msg, title;
				int answer = _nativeLangSpeaker.messageBox("LoseUndoAbilityWarning",
					NULL,
					TEXT("You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?"),
					TEXT("Lose Undo Ability Waning"),
					MB_YESNO);
				
                if (answer == IDYES)
                {
                    // Do nothing
                }
                else
                    return;
            }

            if (!buf->isDirty())
            {
				Buffer *buf = _pEditView->getCurrentBuffer();
				buf->setEncoding(encoding);
				buf->setUnicodeMode(uniCookie);
				fileReload();
            }
			break;
		}


		case IDM_FORMAT_CONV2_ANSI:
		case IDM_FORMAT_CONV2_AS_UTF_8:
		case IDM_FORMAT_CONV2_UTF_8:
		case IDM_FORMAT_CONV2_UCS_2BE: 
		case IDM_FORMAT_CONV2_UCS_2LE:
		{
			int idEncoding = -1;
			Buffer *buf = _pEditView->getCurrentBuffer();
            UniMode um = buf->getUnicodeMode();
            int encoding = buf->getEncoding();

			switch(id)
			{
				case IDM_FORMAT_CONV2_ANSI:
				{
                    if (encoding != -1)
                    {
                        // do nothing
                        return;
                    }
                    else
                    {
					    if (um == uni8Bit)
						    return;
						
                        // set scintilla to ANSI
					    idEncoding = IDM_FORMAT_ANSI;
                    }
					break;
				}
				case IDM_FORMAT_CONV2_AS_UTF_8:
				{
                    if (encoding != -1)
                    {
                        buf->setDirty(true);
                        buf->setUnicodeMode(uniCookie);
                        buf->setEncoding(-1);
                        return;
                    }

					idEncoding = IDM_FORMAT_AS_UTF_8;
					if (um == uniCookie)
						return;

					if (um != uni8Bit)
					{
						::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, idEncoding, 0);
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
						return;
					}

					break;
				}
				case IDM_FORMAT_CONV2_UTF_8:
				{
                    if (encoding != -1)
                    {
                        buf->setDirty(true);
                        buf->setUnicodeMode(uniUTF8);
                        buf->setEncoding(-1);
                        return;
                    }

					idEncoding = IDM_FORMAT_UTF_8;
					if (um == uniUTF8)
						return;

					if (um != uni8Bit)
					{
						::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, idEncoding, 0);
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
						return;
					}
					break;
				}
		
				case IDM_FORMAT_CONV2_UCS_2BE:
				{
                    if (encoding != -1)
                    {
                        buf->setDirty(true);
                        buf->setUnicodeMode(uni16BE);
                        buf->setEncoding(-1);
                        return;
                    }

					idEncoding = IDM_FORMAT_UCS_2BE;
					if (um == uni16BE)
						return;

					if (um != uni8Bit)
					{
						::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, idEncoding, 0);
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
						return;
					}
					break;
				}
		
				case IDM_FORMAT_CONV2_UCS_2LE:
				{
                    if (encoding != -1)
                    {
                        buf->setDirty(true);
                        buf->setUnicodeMode(uni16LE);
                        buf->setEncoding(-1);
                        return;
                    }

					idEncoding = IDM_FORMAT_UCS_2LE;
					if (um == uni16LE)
						return;
					if (um != uni8Bit)
					{
						::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, idEncoding, 0);
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
						return;
					}
					break;
				}
			}

			if (idEncoding != -1)
			{
				// Save the current clipboard content
				::OpenClipboard(_pPublicInterface->getHSelf());
				HANDLE clipboardData = ::GetClipboardData(CF_TEXT);
				int len = ::GlobalSize(clipboardData);
				LPVOID clipboardDataPtr = ::GlobalLock(clipboardData);

				HANDLE allocClipboardData = ::GlobalAlloc(GMEM_MOVEABLE, len);
				LPVOID clipboardData2 = ::GlobalLock(allocClipboardData);

				::memcpy(clipboardData2, clipboardDataPtr, len);
				::GlobalUnlock(clipboardData);	
				::GlobalUnlock(allocClipboardData);	
				::CloseClipboard();

				_pEditView->saveCurrentPos();

				// Cut all text
				int docLen = _pEditView->getCurrentDocLen();
				_pEditView->execute(SCI_COPYRANGE, 0, docLen);
				_pEditView->execute(SCI_CLEARALL);

				// Change to the proper buffer, save buffer status
				
				::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, idEncoding, 0);

				// Paste the texte, restore buffer status
				_pEditView->execute(SCI_PASTE);
				_pEditView->restoreCurrentPos();

				// Restore the previous clipboard data
				::OpenClipboard(_pPublicInterface->getHSelf());
				::EmptyClipboard(); 
				::SetClipboardData(CF_TEXT, clipboardData2);
				::CloseClipboard();

				//Do not free anything, EmptyClipboard does that
				_pEditView->execute(SCI_EMPTYUNDOBUFFER);
			}
			break;
		}

		case IDM_SETTING_IMPORTPLUGIN :
        {
            // get plugin source path
            TCHAR *extFilterName = TEXT("Notepad++ plugin");
            TCHAR *extFilter = TEXT(".dll");
            TCHAR *destDir = TEXT("plugins");

            vector<generic_string> copiedFiles = addNppComponents(destDir, extFilterName, extFilter);

            // load plugin
            vector<generic_string> dll2Remove;
            for (size_t i = 0, len = copiedFiles.size() ; i < len ; ++i)
            {
                int index = _pluginsManager.loadPlugin(copiedFiles[i].c_str(), dll2Remove);
                if (_pluginsManager.getMenuHandle())
                    _pluginsManager.addInMenuFromPMIndex(index);
            }
            if (!_pluginsManager.getMenuHandle())
                _pluginsManager.setMenu(_mainMenuHandle, NULL);
            ::DrawMenuBar(_pPublicInterface->getHSelf());
            break;
        }

        case IDM_SETTING_IMPORTSTYLETHEMS :
        {
            // get plugin source path
            TCHAR *extFilterName = TEXT("Notepad++ style theme");
            TCHAR *extFilter = TEXT(".xml");
            TCHAR *destDir = TEXT("themes");

            // load styler
            NppParameters *pNppParams = NppParameters::getInstance();
            ThemeSwitcher & themeSwitcher = pNppParams->getThemeSwitcher();

            vector<generic_string> copiedFiles = addNppComponents(destDir, extFilterName, extFilter);
            for (size_t i = 0, len = copiedFiles.size(); i < len ; ++i)
            {
                generic_string themeName(themeSwitcher.getThemeFromXmlFileName(copiedFiles[i].c_str()));
		        if (!themeSwitcher.themeNameExists(themeName.c_str())) 
		        {
			        themeSwitcher.addThemeFromXml(copiedFiles[i].c_str());
                    if (_configStyleDlg.isCreated())
                    {
                        _configStyleDlg.addLastThemeEntry();
                    }
		        }
            }
            break;
        }

		case IDM_SETTING_SHORTCUT_MAPPER :
		case IDM_SETTING_SHORTCUT_MAPPER_MACRO :
        case IDM_SETTING_SHORTCUT_MAPPER_RUN :
		{
            GridState st = id==IDM_SETTING_SHORTCUT_MAPPER_MACRO?STATE_MACRO:id==IDM_SETTING_SHORTCUT_MAPPER_RUN?STATE_USER:STATE_MENU;
			ShortcutMapper shortcutMapper;
            shortcutMapper.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), st);
            _nativeLangSpeaker.changeShortcutmapperLang(&shortcutMapper);
			shortcutMapper.doDialog(_nativeLangSpeaker.isRTL());
			shortcutMapper.destroy();
			break;
		}
		case IDM_SETTING_PREFERECE :
		{
			bool isFirstTime = !_preference.isCreated();
			_preference.doDialog(_nativeLangSpeaker.isRTL());
			
			if (isFirstTime)
			{
				_nativeLangSpeaker.changePrefereceDlgLang(_preference);
			}
			break;
		}

        case IDM_SETTING_EDITCONTEXTMENU :
        {
			//if (contion)
			{
				generic_string warning, title;
				_nativeLangSpeaker.messageBox("ContextMenuXmlEditWarning",
					_pPublicInterface->getHSelf(),
					TEXT("Editing contextMenu.xml allows you to modify your Notepad++ popup context menu on edit zone.\rYou have to restart your Notepad++ to take effect after modifying contextMenu.xml."),
					TEXT("Editing contextMenu"),
					MB_OK|MB_APPLMODAL);
			}
            NppParameters *pNppParams = NppParameters::getInstance();
            BufferID bufID = doOpen((pNppParams->getContextMenuPath()).c_str());
			switchToFile(bufID);
            break;
        }

        case IDM_VIEW_GOTO_ANOTHER_VIEW:
            docGotoAnotherEditView(TransferMove);
			checkSyncState();
            break;

        case IDM_VIEW_CLONE_TO_ANOTHER_VIEW:
            docGotoAnotherEditView(TransferClone);
			checkSyncState();
            break;

        case IDM_VIEW_GOTO_NEW_INSTANCE :
            docOpenInNewInstance(TransferMove);
            break;

        case IDM_VIEW_LOAD_IN_NEW_INSTANCE:
            docOpenInNewInstance(TransferClone);
            break;

		case IDM_VIEW_SWITCHTO_OTHER_VIEW:
		{
			int view_to_focus;
			HWND wnd = GetFocus();
			if (_pEditView->getHSelf() == wnd)
			{
				view_to_focus = otherView();
				if (!viewVisible(view_to_focus)) view_to_focus = _activeView;
			}
			else
			{
				view_to_focus = currentView();
			}
			switchEditViewTo(view_to_focus);
			break;
		}

        case IDM_ABOUT:
		{
			bool doAboutDlg = false;
			const int maxSelLen = 32;
			int textLen = _pEditView->execute(SCI_GETSELTEXT, 0, 0) - 1;
			if (!textLen)
				doAboutDlg = true;
			if (textLen > maxSelLen)
				doAboutDlg = true;

			if (!doAboutDlg)
			{
				char author[maxSelLen+1] = "";
				_pEditView->getSelectedText(author, maxSelLen + 1);
				int iQuote = getQuoteIndexFrom(author);
				
				if (iQuote == -1)
				{
					doAboutDlg = true;
				}
				else if (iQuote == -2)
				{
					generic_string noEasterEggsPath((NppParameters::getInstance())->getNppPath());
					noEasterEggsPath.append(TEXT("\\noEasterEggs.xml"));
					if (!::PathFileExists(noEasterEggsPath.c_str()))
						showAllQuotes();
					return;
				}
				if (iQuote != -1)
				{
					generic_string noEasterEggsPath((NppParameters::getInstance())->getNppPath());
					noEasterEggsPath.append(TEXT("\\noEasterEggs.xml"));
					if (!::PathFileExists(noEasterEggsPath.c_str()))
						showQuoteFromIndex(iQuote);
					return;
				}	
			}
			if (doAboutDlg)
			{
				bool isFirstTime = !_aboutDlg.isCreated();
				_aboutDlg.doDialog();
				if (isFirstTime && _nativeLangSpeaker.getNativeLangA())
				{
					if (_nativeLangSpeaker.getLangEncoding() == NPP_CP_BIG5)
					{
						char *authorName = "«J¤µ§^";
						HWND hItem = ::GetDlgItem(_aboutDlg.getHSelf(), IDC_AUTHOR_NAME);

						WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
						const wchar_t *authorNameW = wmc->char2wchar(authorName, NPP_CP_BIG5);
						::SetWindowText(hItem, authorNameW);
					}
				}
			}
			break;
		}

		case IDM_HELP :
		{
			generic_string tmp((NppParameters::getInstance())->getNppPath());
			generic_string nppHelpPath = tmp.c_str();

			nppHelpPath += TEXT("\\user.manual\\documentation\\notepad-online-document.html");
			if (::PathFileExists(nppHelpPath.c_str()))
				::ShellExecute(NULL, TEXT("open"), nppHelpPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			else
			{
				generic_string msg = nppHelpPath;
				generic_string warning, title;
				if (!_nativeLangSpeaker.getMsgBoxLang("NppHelpAbsentWarning", title, warning))
				{
					title = TEXT("File does not exist");
					warning = TEXT("\rdoesn't exist. Please download it on Notepad++ site.");
				}
				msg += warning;
				::MessageBox(_pPublicInterface->getHSelf(), msg.c_str(), title.c_str(), MB_OK);
			}
		}
		break;

		case IDM_HOMESWEETHOME :
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("https://notepad-plus-plus.org/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}
		case IDM_PROJECTPAGE :
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/donho/notepad-plus-plus/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_ONLINEHELP:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("http://docs.notepad-plus-plus.org/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_CMDLINEARGUMENTS:
		{
			::MessageBox(NULL, COMMAND_ARG_HELP, TEXT("Notepad++ Command Argument Help"), MB_OK);
			break;
		}
		/*
		case IDM_FORUM:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT(""), NULL, NULL, SW_SHOWNORMAL);
			break;
		}
		*/
		case IDM_ONLINESUPPORT:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("https://gitter.im/notepad-plus-plus/notepad-plus-plus"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_PLUGINSHOME:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("http://docs.notepad-plus-plus.org/index.php/Plugin_Central"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_UPDATE_NPP :
		case IDM_CONFUPDATERPROXY :
		{
			// wingup doesn't work with the obsolet security layer (API) under xp since downloadings are secured with SSL on notepad_plus_plus.org
			winVer ver = NppParameters::getInstance()->getWinVersion();
			if (ver <= WV_XP)
			{
				long res = ::MessageBox(NULL, TEXT("Notepad++ updater is not compatible with XP due to the obsolet security layer under XP.\rDo you want to go to Notepad++ page to download the latest version?"), TEXT("Notepad++ Updater"), MB_YESNO);
				if (res == IDYES)
				{
					::ShellExecute(NULL, TEXT("open"), TEXT("https://notepad-plus-plus.org/download/"), NULL, NULL, SW_SHOWNORMAL);
				}
			}
			else
			{
				generic_string updaterDir = (NppParameters::getInstance())->getNppPath();
				PathAppend(updaterDir, TEXT("updater"));

				generic_string updaterFullPath = updaterDir;
				PathAppend(updaterFullPath, TEXT("gup.exe"));

				generic_string param = TEXT("-verbose -v");
				param += VERSION_VALUE;

				if (id == IDM_CONFUPDATERPROXY)
					param = TEXT("-options");

				Process updater(updaterFullPath.c_str(), param.c_str(), updaterDir.c_str());

				updater.run();
			}
			break;
		}

		case IDM_EDIT_AUTOCOMPLETE :
			showAutoComp();
			break;

		case IDM_EDIT_AUTOCOMPLETE_CURRENTFILE :
			autoCompFromCurrentFile();
			break;

		case IDM_EDIT_AUTOCOMPLETE_PATH :
			showPathCompletion();
			break;

		case IDM_EDIT_FUNCCALLTIP :
			showFunctionComp();
			break;

        case IDM_LANGSTYLE_CONFIG_DLG :
		{
			bool isFirstTime = !_configStyleDlg.isCreated();
			_configStyleDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
                _nativeLangSpeaker.changeConfigLang(_configStyleDlg.getHSelf());
			break;
		}

        case IDM_LANG_C	:
        case IDM_LANG_CPP :
        case IDM_LANG_JAVA :
        case IDM_LANG_CS :
        case IDM_LANG_HTML :
        case IDM_LANG_XML :
        case IDM_LANG_JS :
        case IDM_LANG_PHP :
        case IDM_LANG_ASP :
        case IDM_LANG_CSS :
        case IDM_LANG_LUA :
        case IDM_LANG_PERL :
        case IDM_LANG_PYTHON :
        case IDM_LANG_PASCAL :
        case IDM_LANG_BATCH :
        case IDM_LANG_OBJC :
        case IDM_LANG_VB :
        case IDM_LANG_SQL :
        case IDM_LANG_ASCII :
        case IDM_LANG_TEXT :
        case IDM_LANG_RC :
        case IDM_LANG_MAKEFILE :
        case IDM_LANG_INI :
        case IDM_LANG_TEX :
        case IDM_LANG_FORTRAN :
        case IDM_LANG_BASH :
        case IDM_LANG_FLASH :
		case IDM_LANG_NSIS :
		case IDM_LANG_TCL :
		case IDM_LANG_LISP :
		case IDM_LANG_SCHEME :
		case IDM_LANG_ASM :
		case IDM_LANG_DIFF :
		case IDM_LANG_PROPS :
		case IDM_LANG_PS:
		case IDM_LANG_RUBY:
		case IDM_LANG_SMALLTALK:
		case IDM_LANG_VHDL :
        case IDM_LANG_KIX :
        case IDM_LANG_CAML :
        case IDM_LANG_ADA :
        case IDM_LANG_VERILOG :
		case IDM_LANG_MATLAB :
		case IDM_LANG_HASKELL :
        case IDM_LANG_AU3 :
		case IDM_LANG_INNO :
		case IDM_LANG_CMAKE :
		case IDM_LANG_YAML :
        case IDM_LANG_COBOL :
        case IDM_LANG_D :
        case IDM_LANG_GUI4CLI :
        case IDM_LANG_POWERSHELL :
        case IDM_LANG_R :
        case IDM_LANG_JSP :
		case IDM_LANG_COFFEESCRIPT:
		case IDM_LANG_USER :
		{
            setLanguage(menuID2LangType(id));
			if (_pDocMap)
			{
				_pDocMap->setSyntaxHiliting();
			}
		}
        break;

        case IDC_PREV_DOC :
        case IDC_NEXT_DOC :
        {
			int nbDoc = viewVisible(MAIN_VIEW)?_mainDocTab.nbItem():0;
			nbDoc += viewVisible(SUB_VIEW)?_subDocTab.nbItem():0;
			
			bool doTaskList = ((NppParameters::getInstance())->getNppGUI())._doTaskList;
			if (nbDoc > 1)
			{
				bool direction = (id == IDC_NEXT_DOC)?dirDown:dirUp;

				if (!doTaskList)
				{
					activateNextDoc(direction);
				}
				else
				{		
					TaskListDlg tld;
					HIMAGELIST hImgLst = _docTabIconList.getHandle();
					tld.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), hImgLst, direction);
					tld.doDialog();
				}
			}
			_linkTriggered = true;
		}
        break;

		case IDM_OPEN_ALL_RECENT_FILE :
		{
			BufferID lastOne = BUFFER_INVALID;
			int size = _lastRecentFileList.getSize();
			for (int i = size - 1; i >= 0; i--)
			{
				BufferID test = doOpen(_lastRecentFileList.getIndex(i).c_str());
				if (test != BUFFER_INVALID)
					lastOne = test;
			}
			if (lastOne != BUFFER_INVALID)
			{
				switchToFile(lastOne);
			}
			break;
		}

		case IDM_CLEAN_RECENT_FILE_LIST :
			_lastRecentFileList.clear();
			break;

		case IDM_EDIT_RTL :
		case IDM_EDIT_LTR :
		{
			_pEditView->changeTextDirection(id == IDM_EDIT_RTL);

			// Wrap then !wrap to fix problem of mirror characters
			bool isWraped = _pEditView->isWrap();
			_pEditView->wrap(!isWraped);
			_pEditView->wrap(isWraped);
			if (_pDocMap)
			{
				_pDocMap->changeTextDirection(id == IDM_EDIT_RTL);
			}
		}
		break;

		case IDM_WINDOW_WINDOWS :
		{
			WindowsDlg _windowsDlg;
			_windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			
            const TiXmlNodeA *nativeLangA = _nativeLangSpeaker.getNativeLangA();
			TiXmlNodeA *dlgNode = NULL;
			if (nativeLangA)
			{
				dlgNode = nativeLangA->FirstChild("Dialog");
				if (dlgNode)
					dlgNode = _nativeLangSpeaker.searchDlgNode(dlgNode, "Window");
			}
			_windowsDlg.doDialog(dlgNode);
		}
		break;


		case IDM_SYSTRAYPOPUP_NEWDOC:
		{
			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			fileNew();
		}
		break;

		case IDM_SYSTRAYPOPUP_ACTIVATE :
		{
			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
		}
		break;

		case IDM_SYSTRAYPOPUP_NEW_AND_PASTE:
		{
			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			BufferID bufferID = _pEditView->getCurrentBufferID();
			Buffer * buf = MainFileManager->getBufferByID(bufferID);
			if (!buf->isUntitled() || buf->docLength() != 0)
			{
				fileNew();
			}	
			command(IDM_EDIT_PASTE);
		}
		break;
		
		case IDM_SYSTRAYPOPUP_OPENFILE:
		{
			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			fileOpen();
		}
		break;

		case IDM_SYSTRAYPOPUP_CLOSE:
		{
			_pPublicInterface->setIsPrelaunch(false);
			_pTrayIco->doTrayIcon(REMOVE);
			if (!::IsWindowVisible(_pPublicInterface->getHSelf()))
				::SendMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0,0);
		}
		break;

		case IDM_FILE_RESTORELASTCLOSEDFILE:
		{
			generic_string lastOpenedFullPath = _lastRecentFileList.getFirstItem();
			if (lastOpenedFullPath != TEXT(""))
			{
				BufferID lastOpened = doOpen(lastOpenedFullPath.c_str());
				if (lastOpened != BUFFER_INVALID)
				{
					switchToFile(lastOpened);
				}
			}
		}
		break;

		default :
			if (id > IDM_FILEMENU_LASTONE && id < (IDM_FILEMENU_LASTONE + _lastRecentFileList.getMaxNbLRF() + 1))
			{
				BufferID lastOpened = doOpen(_lastRecentFileList.getItem(id).c_str());
				if (lastOpened != BUFFER_INVALID)
				{
					switchToFile(lastOpened);
				}
			}
			else if ((id > IDM_LANG_USER) && (id < IDM_LANG_USER_LIMIT))
			{
				TCHAR langName[langNameLenMax];
				::GetMenuString(_mainMenuHandle, id, langName, langNameLenMax, MF_BYCOMMAND);
				_pEditView->getCurrentBuffer()->setLangType(L_USER, langName);
			}
			else if ((id >= IDM_LANG_EXTERNAL) && (id <= IDM_LANG_EXTERNAL_LIMIT))
			{
				setLanguage((LangType)(id - IDM_LANG_EXTERNAL + L_EXTERNAL));
			}
			else if ((id >= ID_MACRO) && (id < ID_MACRO_LIMIT))
			{
				int i = id - ID_MACRO;
				vector<MacroShortcut> & theMacros = (NppParameters::getInstance())->getMacroList();
				macroPlayback(theMacros[i].getMacro());				
			}
			else if ((id >= ID_USER_CMD) && (id < ID_USER_CMD_LIMIT))
			{
				int i = id - ID_USER_CMD;
				vector<UserCommand> & theUserCommands = (NppParameters::getInstance())->getUserCommandList();
				UserCommand ucmd = theUserCommands[i];

				Command cmd(ucmd.getCmd());
				cmd.run(_pPublicInterface->getHSelf());
			}
			else if ((id >= ID_PLUGINS_CMD) && (id < ID_PLUGINS_CMD_LIMIT))
			{
				int i = id - ID_PLUGINS_CMD;
				_pluginsManager.runPluginCommand(i);
			}
			else if (_pluginsManager.inDynamicRange(id)) // in the dynamic range allocated with NPPM_ALLOCATECMDID
			{
				_pluginsManager.relayNppMessages(WM_COMMAND, id, 0);
			}
/*UNLOAD 
			else if ((id >= ID_PLUGINS_REMOVING) && (id < ID_PLUGINS_REMOVING_END))
			{
				int i = id - ID_PLUGINS_REMOVING;
				_pluginsManager.unloadPlugin(i, _pPublicInterface->getHSelf());
			}
*/
			else if ((id >= IDM_WINDOW_MRU_FIRST) && (id <= IDM_WINDOW_MRU_LIMIT))
			{
				activateDoc(id-IDM_WINDOW_MRU_FIRST);				
			}
	}
	
	if (_recordingMacro) 
		switch (id)
		{
			case IDM_FILE_NEW :
			case IDM_FILE_CLOSE :
			case IDM_FILE_CLOSEALL :
			case IDM_FILE_CLOSEALL_BUT_CURRENT :
			case IDM_FILE_CLOSEALL_TOLEFT :
			case IDM_FILE_CLOSEALL_TORIGHT :
			case IDM_FILE_SAVE :
			case IDM_FILE_SAVEALL :
			case IDM_FILE_RELOAD:
			case IDM_EDIT_UNDO:
			case IDM_EDIT_REDO:
			case IDM_EDIT_CUT:
			case IDM_EDIT_COPY:
			//case IDM_EDIT_PASTE:
			case IDM_EDIT_DELETE:
			case IDM_SEARCH_FINDNEXT :
			case IDM_SEARCH_FINDPREV :
            case IDM_SEARCH_SETANDFINDNEXT :
			case IDM_SEARCH_SETANDFINDPREV :
			case IDM_SEARCH_GOTOMATCHINGBRACE :
			case IDM_SEARCH_SELECTMATCHINGBRACES :
			case IDM_SEARCH_TOGGLE_BOOKMARK :
			case IDM_SEARCH_NEXT_BOOKMARK:
			case IDM_SEARCH_PREV_BOOKMARK:
			case IDM_SEARCH_CLEAR_BOOKMARKS:
			case IDM_EDIT_SELECTALL:
			case IDM_EDIT_INS_TAB:
			case IDM_EDIT_RMV_TAB:
			case IDM_EDIT_DUP_LINE:
			case IDM_EDIT_TRANSPOSE_LINE:
			case IDM_EDIT_SPLIT_LINES:
			case IDM_EDIT_JOIN_LINES:
			case IDM_EDIT_LINE_UP:
			case IDM_EDIT_LINE_DOWN:
			case IDM_EDIT_UPPERCASE:
			case IDM_EDIT_LOWERCASE:
			case IDM_EDIT_BLOCK_COMMENT:
			case IDM_EDIT_BLOCK_COMMENT_SET:
			case IDM_EDIT_BLOCK_UNCOMMENT:
			case IDM_EDIT_STREAM_COMMENT:
			case IDM_EDIT_TRIMTRAILING:
			case IDM_EDIT_TRIMLINEHEAD:
			case IDM_EDIT_TRIM_BOTH:
			case IDM_EDIT_EOL2WS:
			case IDM_EDIT_TRIMALL:
			case IDM_EDIT_TAB2SW:
			case IDM_EDIT_SW2TAB_ALL:
			case IDM_EDIT_SW2TAB_LEADING:
			case IDM_EDIT_SETREADONLY :
			case IDM_EDIT_FULLPATHTOCLIP :
			case IDM_EDIT_FILENAMETOCLIP :
			case IDM_EDIT_CURRENTDIRTOCLIP :
			case IDM_EDIT_CLEARREADONLY :
			case IDM_EDIT_RTL :
			case IDM_EDIT_LTR :
			case IDM_EDIT_BEGINENDSELECT:
			case IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING:
			case IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING:
			case IDM_EDIT_SORTLINES_INTEGER_ASCENDING:
			case IDM_EDIT_SORTLINES_INTEGER_DESCENDING:
			case IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING:
			case IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING:
			case IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING:
			case IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING:
			case IDM_EDIT_BLANKLINEABOVECURRENT:
			case IDM_EDIT_BLANKLINEBELOWCURRENT:
			case IDM_VIEW_FULLSCREENTOGGLE :
			case IDM_VIEW_ALWAYSONTOP :
			case IDM_VIEW_WRAP :
			case IDM_VIEW_FOLD_CURRENT :
			case IDM_VIEW_UNFOLD_CURRENT :
			case IDM_VIEW_TOGGLE_FOLDALL:
			case IDM_VIEW_TOGGLE_UNFOLDALL:
			case IDM_VIEW_FOLD_1:
			case IDM_VIEW_FOLD_2:
			case IDM_VIEW_FOLD_3:
			case IDM_VIEW_FOLD_4:
			case IDM_VIEW_FOLD_5:
			case IDM_VIEW_FOLD_6:
			case IDM_VIEW_FOLD_7:
			case IDM_VIEW_FOLD_8:
			case IDM_VIEW_UNFOLD_1:
			case IDM_VIEW_UNFOLD_2:
			case IDM_VIEW_UNFOLD_3:
			case IDM_VIEW_UNFOLD_4:
			case IDM_VIEW_UNFOLD_5:
			case IDM_VIEW_UNFOLD_6:
			case IDM_VIEW_UNFOLD_7:
			case IDM_VIEW_UNFOLD_8:
			case IDM_VIEW_GOTO_ANOTHER_VIEW:
			case IDM_VIEW_SYNSCROLLV:
			case IDM_VIEW_SYNSCROLLH:
			case IDM_VIEW_TAB1:	
			case IDM_VIEW_TAB2:	
			case IDM_VIEW_TAB3:	
			case IDM_VIEW_TAB4:	
			case IDM_VIEW_TAB5:	
			case IDM_VIEW_TAB6:	
			case IDM_VIEW_TAB7:	
			case IDM_VIEW_TAB8:	
			case IDM_VIEW_TAB9:	
			case IDM_VIEW_TAB_NEXT:
			case IDM_VIEW_TAB_PREV:
			case IDC_PREV_DOC :
			case IDC_NEXT_DOC :
			case IDM_SEARCH_GOPREVMARKER1   :
			case IDM_SEARCH_GOPREVMARKER2   :
			case IDM_SEARCH_GOPREVMARKER3   :
			case IDM_SEARCH_GOPREVMARKER4   :
			case IDM_SEARCH_GOPREVMARKER5   :
			case IDM_SEARCH_GOPREVMARKER_DEF:
			case IDM_SEARCH_GONEXTMARKER1   :
			case IDM_SEARCH_GONEXTMARKER2   :
			case IDM_SEARCH_GONEXTMARKER3   :
			case IDM_SEARCH_GONEXTMARKER4   :
			case IDM_SEARCH_GONEXTMARKER5   :
			case IDM_SEARCH_GONEXTMARKER_DEF:
			case IDM_SEARCH_VOLATILE_FINDNEXT:
			case IDM_SEARCH_VOLATILE_FINDPREV:
			case IDM_SEARCH_CUTMARKEDLINES   :
			case IDM_SEARCH_COPYMARKEDLINES     :
			case IDM_SEARCH_PASTEMARKEDLINES    :
			case IDM_SEARCH_DELETEMARKEDLINES   :
			case IDM_SEARCH_DELETEUNMARKEDLINES :
			case IDM_SEARCH_MARKALLEXT1      :
			case IDM_SEARCH_UNMARKALLEXT1    :
			case IDM_SEARCH_MARKALLEXT2      :
			case IDM_SEARCH_UNMARKALLEXT2    :
			case IDM_SEARCH_MARKALLEXT3      :
			case IDM_SEARCH_UNMARKALLEXT3    :
			case IDM_SEARCH_MARKALLEXT4      :
			case IDM_SEARCH_UNMARKALLEXT4    :
			case IDM_SEARCH_MARKALLEXT5      :
			case IDM_SEARCH_UNMARKALLEXT5    :
			case IDM_SEARCH_CLEARALLMARKS    :
				_macro.push_back(recordedMacroStep(id));
				break;
		}
}
