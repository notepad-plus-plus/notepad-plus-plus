// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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
#include "fileBrowser.h"
#include "Sorters.h"
#include "verifySignedfile.h"
#include "md5.h"
#include "sha-256.h"

using namespace std;

std::mutex command_mutex;

void Notepad_plus::macroPlayback(Macro macro)
{
	_playingBackMacro = true;
	_pEditView->execute(SCI_BEGINUNDOACTION);

	for (Macro::iterator step = macro.begin(); step != macro.end(); ++step)
	{
		if (step->isScintillaMacro())
			step->PlayBack(this->_pPublicInterface, _pEditView);
		else
			_findReplaceDlg.execSavedCommand(step->_message, step->_lParameter, step->_sParameter);
	}

	_pEditView->execute(SCI_ENDUNDOACTION);
	_playingBackMacro = false;
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
			Command cmd(TEXT("explorer /select,\"$(FULL_CURRENT_PATH)\""));
			cmd.run(_pPublicInterface->getHSelf());
		}
		break;

		case IDM_FILE_OPEN_CMD:
		{
			Command cmd(NppParameters::getInstance().getNppGUI()._commandLineInterpreter.c_str());
			cmd.run(_pPublicInterface->getHSelf(), TEXT("$(CURRENT_DIRECTORY)"));
		}
		break;
		
		case IDM_FILE_OPEN_DEFAULT_VIEWER:
		{
			// Opens file in its default viewer. 
            // Has the same effect as double–clicking this file in Windows Explorer.
            BufferID buf = _pEditView->getCurrentBufferID();
			HINSTANCE res = ::ShellExecute(NULL, TEXT("open"), buf->getFullPathName(), NULL, NULL, SW_SHOW);

			// As per MSDN (https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx)
			// If the function succeeds, it returns a value greater than 32.
			// If the function fails, it returns an error value that indicates the cause of the failure.
			int retResult = static_cast<int>(reinterpret_cast<INT_PTR>(res));
			if (retResult <= 32)
			{
				generic_string errorMsg;
				errorMsg += GetLastErrorAsString(retResult);
				errorMsg += TEXT("An attempt was made to execute the below command.");
				errorMsg += TEXT("\n----------------------------------------------------------");
				errorMsg += TEXT("\nCommand: ");
				errorMsg += buf->getFullPathName();
				errorMsg += TEXT("\nError Code: ");
				errorMsg += intToString(retResult);
				errorMsg += TEXT("\n----------------------------------------------------------");
				
				::MessageBox(_pPublicInterface->getHSelf(), errorMsg.c_str(), TEXT("ShellExecute - ERROR"), MB_ICONINFORMATION | MB_APPLMODAL);
			}
		}
		break;

		case IDM_FILE_OPENFOLDERASWORSPACE:
		{
			generic_string folderPath = folderBrowser(_pPublicInterface->getHSelf(), TEXT("Select a folder to add in Folder as Workspace panel"));
			if (not folderPath.empty())
			{
				if (_pFileBrowser == nullptr) // first launch, check in params to open folders
				{
					vector<generic_string> dummy;
					launchFileBrowser(dummy);
					if (_pFileBrowser != nullptr)
					{
						checkMenuItem(IDM_VIEW_FILEBROWSER, true);
						_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
						_pFileBrowser->setClosed(false);
					}
					else // problem
						return;
				}
				else
				{
					if (_pFileBrowser->isClosed())
					{
						_pFileBrowser->display();
						checkMenuItem(IDM_VIEW_FILEBROWSER, true);
						_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
						_pFileBrowser->setClosed(false);
					}
				}
				_pFileBrowser->addRootFolder(folderPath);
			}
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
					_pFileSwitcherPanel->activateItem(_pEditView->getCurrentBufferID(), currentView());
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
			bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
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

		case IDM_FILE_CLOSEALL_UNCHANGED:
			fileCloseAllUnchanged();
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
			std::lock_guard<std::mutex> lock(command_mutex);
			_pEditView->execute(WM_UNDO);
			checkClipboard();
			checkUndoState();
			break;
		}

		case IDM_EDIT_REDO:
		{
			std::lock_guard<std::mutex> lock(command_mutex);
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
			int textLen = static_cast<int32_t>(_pEditView->execute(SCI_GETSELTEXT, 0, 0)) - 1;
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
				_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
		}
		break;

		case IDM_EDIT_PASTE:
		{
			std::lock_guard<std::mutex> lock(command_mutex);
			int eolMode = int(_pEditView->execute(SCI_GETEOLMODE));
			_pEditView->execute(SCI_PASTE);
			_pEditView->execute(SCI_CONVERTEOLS, eolMode);
		}
		break;

		case IDM_EDIT_PASTE_BINARY:
		{
			std::lock_guard<std::mutex> lock(command_mutex);
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
								_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
								_pEditView->execute(SCI_ADDTEXT, *lpLen, reinterpret_cast<LPARAM>(lpchar));

								GlobalUnlock(hglbLen);
							}
						}
					}
					else
					{
						_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(lpchar));
					}
					GlobalUnlock(hglb);
				}
			}
			CloseClipboard();

		}
		break;

		case IDM_EDIT_OPENINFOLDER:
		case IDM_EDIT_OPENASFILE:
		{
			if (_pEditView->execute(SCI_GETSELECTIONS) != 1) // Multi-Selection || Column mode || no selection
				return;

			HWND hwnd = _pPublicInterface->getHSelf();
			TCHAR curentWord[CURRENTWORD_MAXLENGTH];
			::SendMessage(hwnd, NPPM_GETFILENAMEATCURSOR, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(curentWord));
			
			TCHAR cmd2Exec[CURRENTWORD_MAXLENGTH];
			if (id == IDM_EDIT_OPENINFOLDER)
			{
				wcscpy_s(cmd2Exec, TEXT("explorer"));
			}
			else
			{
				::SendMessage(hwnd, NPPM_GETNPPFULLFILEPATH, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(cmd2Exec));
			}

			// Full file path
			if (::PathFileExists(curentWord))
			{
				generic_string fullFilePath = id == IDM_EDIT_OPENINFOLDER ? TEXT("/select,") : TEXT("");
				fullFilePath += TEXT("\"");
				fullFilePath += curentWord;
				fullFilePath += TEXT("\"");

				if (id == IDM_EDIT_OPENINFOLDER ||
					(id == IDM_EDIT_OPENASFILE && not ::PathIsDirectory(curentWord)))
					::ShellExecute(hwnd, TEXT("open"), cmd2Exec, fullFilePath.c_str(), TEXT("."), SW_SHOW);
			}
			else // Full file path - need concatenate with current full file path
			{
				TCHAR currentDir[CURRENTWORD_MAXLENGTH];
				::SendMessage(hwnd, NPPM_GETCURRENTDIRECTORY, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(currentDir));

				generic_string fullFilePath = id == IDM_EDIT_OPENINFOLDER ? TEXT("/select,") : TEXT("");
				fullFilePath += TEXT("\"");
				fullFilePath += currentDir;
				fullFilePath += TEXT("\\");
				fullFilePath += curentWord;

				if ((id == IDM_EDIT_OPENASFILE &&
					(not::PathFileExists(fullFilePath.c_str() + 1) || ::PathIsDirectory(fullFilePath.c_str() + 1))))
				{
					_nativeLangSpeaker.messageBox("FilePathNotFoundWarning",
						_pPublicInterface->getHSelf(),
						TEXT("The file you're trying to open doesn't exist."),
						TEXT("File Open"),
						MB_OK | MB_APPLMODAL);
					return;
				}
				fullFilePath += TEXT("\"");
				::ShellExecute(hwnd, TEXT("open"), cmd2Exec, fullFilePath.c_str(), TEXT("."), SW_SHOW);
			}
		}
		break;

		case IDM_EDIT_SEARCHONINTERNET:
		{
			if (_pEditView->execute(SCI_GETSELECTIONS) != 1) // Multi-Selection || Column mode || no selection
				return;

			const NppGUI & nppGui = (NppParameters::getInstance()).getNppGUI();
			generic_string url;
			if (nppGui._searchEngineChoice == nppGui.se_custom)
			{
				url = nppGui._searchEngineCustom;
				remove_if(url.begin(), url.end(), _istspace);

				auto httpPos = url.find(TEXT("http://"));
				auto httpsPos = url.find(TEXT("https://"));

				if (url.empty() || (httpPos != 0 && httpsPos != 0)) // if string is not a url (for launching only browser)
				{
					url = TEXT("https://www.google.com/search?q=$(CURRENT_WORD)");
				}
			}
			else if (nppGui._searchEngineChoice == nppGui.se_duckDuckGo)
			{
				url = TEXT("https://duckduckgo.com/?q=$(CURRENT_WORD)");
			}
			else if (nppGui._searchEngineChoice == nppGui.se_google)
			{
				url = TEXT("https://www.google.com/search?q=$(CURRENT_WORD)");
			}
			else if (nppGui._searchEngineChoice == nppGui.se_bing)
			{
				url = TEXT("https://www.bing.com/search?q=$(CURRENT_WORD)");
			}
			else if (nppGui._searchEngineChoice == nppGui.se_yahoo)
			{
				url = TEXT("https://search.yahoo.com/search?q=$(CURRENT_WORD)");
			}
			else if (nppGui._searchEngineChoice == nppGui.se_stackoverflow)
			{
				url = TEXT("https://stackoverflow.com/search?q=$(CURRENT_WORD)");
			}

			Command cmd(url.c_str());
			cmd.run(_pPublicInterface->getHSelf());	
		}
		break;

		case IDM_EDIT_CHANGESEARCHENGINE:
		{
			command(IDM_SETTING_PREFERENCE);
			_preference.showDialogByName(TEXT("SearchEngine"));
		}
		break;

		case IDM_EDIT_PASTE_AS_RTF:
		case IDM_EDIT_PASTE_AS_HTML:
		{
			std::lock_guard<std::mutex> lock(command_mutex);
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
					_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(lptstr));

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
			std::lock_guard<std::mutex> lock(command_mutex);

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
				auto selStart = _pEditView->execute(SCI_GETSELECTIONSTART);
				auto selEnd = _pEditView->execute(SCI_GETSELECTIONEND);
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
				pSorter = std::unique_ptr<ISorter>(new NaturalSorter(isDescending, fromColumn, toColumn));
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
				size_t lineNo = 1 + fromLine + failedLineIndex;

				_nativeLangSpeaker.messageBox("SortingError",
					_pPublicInterface->getHSelf(),
					TEXT("Unable to perform numeric sorting due to line $INT_REPLACE$."),
					TEXT("Sorting Error"),
					MB_OK | MB_ICONINFORMATION | MB_APPLMODAL,
					static_cast<int>(lineNo),
					0);
			}
			_pEditView->execute(SCI_ENDUNDOACTION);

			if (hasLineSelection) // there was 1 selection, so we restore it
			{
				auto posStart = _pEditView->execute(SCI_POSITIONFROMLINE, fromLine);
				auto posEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, toLine);
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

		case IDM_VIEW_FILEBROWSER:
		{
			if (_pFileBrowser == nullptr) // first launch, check in params to open folders
			{
				NppParameters& nppParam = NppParameters::getInstance();
				launchFileBrowser(nppParam.getFileBrowserRoots());
				if (_pFileBrowser != nullptr)
				{
					checkMenuItem(IDM_VIEW_FILEBROWSER, true);
					_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
					_pFileBrowser->setClosed(false);
				}
			}
			else
			{
				if (not _pFileBrowser->isClosed())
				{
					_pFileBrowser->display(false);
					_pFileBrowser->setClosed(true);
					checkMenuItem(IDM_VIEW_FILEBROWSER, false);
					_toolBar.setCheck(IDM_VIEW_FILEBROWSER, false);
				}
				else
				{
					vector<generic_string> dummy;
					launchFileBrowser(dummy);
					checkMenuItem(IDM_VIEW_FILEBROWSER, true);
					_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
					_pFileBrowser->setClosed(false);
				}
			}
		}
		break;

		case IDM_VIEW_DOC_MAP:
		{
			if (_pDocMap && (not _pDocMap->isClosed()))
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
			if (_pFuncList && (not _pFuncList->isClosed()))
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
			_isFolding = true;
			if (buf == BUFFER_INVALID)
			{
				// No buffer at chosen index, select the very last buffer instead.
				const int last_index = _pDocTab->getItemCount() - 1;
				if (last_index > 0)
					switchToFile(_pDocTab->getBufferByIndex(last_index));
			}
			else
			{
				switchToFile(buf);
			}
			_isFolding = false;
		}
		break;

		case IDM_VIEW_TAB_NEXT:
		{
			const int current_index = _pDocTab->getCurrentTabIndex();
			const int last_index = _pDocTab->getItemCount() - 1;
			_isFolding = true;
			if (current_index < last_index)
				switchToFile(_pDocTab->getBufferByIndex(current_index + 1));
			else
			{
				switchToFile(_pDocTab->getBufferByIndex(0)); // Loop around.
			}
			_isFolding = false;
		}
		break;

		case IDM_VIEW_TAB_PREV:
		{
			const int current_index = _pDocTab->getCurrentTabIndex();
			_isFolding = true;
			if (current_index > 0)
				switchToFile(_pDocTab->getBufferByIndex(current_index - 1));
			else
			{
				const int last_index = _pDocTab->getItemCount() - 1;
				switchToFile(_pDocTab->getBufferByIndex(last_index)); // Loop around.
			}
			_isFolding = false;
		}
		break;

		case IDM_VIEW_TAB_MOVEFORWARD:
		case IDM_VIEW_TAB_MOVEBACKWARD:
		{
			const int currentTabIndex = _pDocTab->getCurrentTabIndex();
			const int lastTabIndex = _pDocTab->getItemCount() - 1;
			int newTabIndex = currentTabIndex;

			if (id == IDM_VIEW_TAB_MOVEFORWARD)
			{
				if (currentTabIndex >= lastTabIndex)
					return;
				++newTabIndex;
			}
			else
			{
				if (currentTabIndex < 1)
					return;
				--newTabIndex;
			}

			TCITEM tciMove, tciShift;
			tciMove.mask = tciShift.mask = TCIF_IMAGE | TCIF_TEXT | TCIF_PARAM;

			const int strSizeMax = 256;
			TCHAR strMove[strSizeMax];
			TCHAR strShift[strSizeMax];

			tciMove.pszText = strMove;
			tciMove.cchTextMax = strSizeMax;

			tciShift.pszText = strShift;
			tciShift.cchTextMax = strSizeMax;

			::SendMessage(_pDocTab->getHSelf(), TCM_GETITEM, currentTabIndex, reinterpret_cast<LPARAM>(&tciMove));

			::SendMessage(_pDocTab->getHSelf(), TCM_GETITEM, newTabIndex, reinterpret_cast<LPARAM>(&tciShift));
			::SendMessage(_pDocTab->getHSelf(), TCM_SETITEM, currentTabIndex, reinterpret_cast<LPARAM>(&tciShift));

			::SendMessage(_pDocTab->getHSelf(), TCM_SETITEM, newTabIndex, reinterpret_cast<LPARAM>(&tciMove));

			::SendMessage(_pDocTab->getHSelf(), TCM_SETCURSEL, newTabIndex, 0);

			// Notify plugins that the document order has changed
			::SendMessage(_pDocTab->getHParent(), NPPM_INTERNAL_DOCORDERCHANGED, 0, newTabIndex);
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

				_mainEditView.execute(SCI_SETCURSOR, static_cast<WPARAM>(SC_CURSORNORMAL));
				_subEditView.execute(SCI_SETCURSOR, static_cast<WPARAM>(SC_CURSORNORMAL));

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
			_recordingSaved = false;
			checkMacroState();
			break;
		}

		case IDM_MACRO_PLAYBACKRECORDEDMACRO:
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
			{
				macroPlayback(_macro);
			}
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
			{
				_recordingSaved = true;
				_runMacroDlg.initMacroList();
				checkMacroState();
			}
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

			const NppGUI & nppGui = (NppParameters::getInstance()).getNppGUI();
			if (!nppGui._stopFillingFindField)
			{
				_pEditView->getGenericSelectedText(str, strSize);
				_findReplaceDlg.setSearchText(str);
			}

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
			if (_findReplaceDlg.isCreated())
			{
				FindOption op = _findReplaceDlg.getCurrentOptions();
				if ((id == IDM_SEARCH_FINDPREV) && (op._searchType == FindRegex))
				{
					// regex upward search is disabled
					// make this a no-action command
				}
				else
				{
					op._whichDirection = (id == IDM_SEARCH_FINDNEXT ? DIR_DOWN : DIR_UP);
					generic_string s = _findReplaceDlg.getText2search();
					FindStatus status = FSNoMessage;
					_findReplaceDlg.processFindNext(s.c_str(), &op, &status);
					if (status == FSEndReached)
					{
						generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-end-reached", TEXT("Find: Found the 1st occurrence from the top. The end of the document has been reached."));
						_findReplaceDlg.setStatusbarMessage(msg, FSEndReached);
					}
					else if (status == FSTopReached)
					{
						generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-top-reached", TEXT("Find: Found the 1st occurrence from the bottom. The beginning of the document has been reached."));
						_findReplaceDlg.setStatusbarMessage(msg, FSTopReached);
					}
				}
			}
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
			{
				generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-end-reached", TEXT("Find: Found the 1st occurrence from the top. The end of the document has been reached."));
				_findReplaceDlg.setStatusbarMessage(msg, FSEndReached);
			}
			else if (status == FSTopReached)
			{
				generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-top-reached", TEXT("Find: Found the 1st occurrence from the bottom. The beginning of the document has been reached."));
				_findReplaceDlg.setStatusbarMessage(msg, FSTopReached);
			}
        }
		break;

		case IDM_SEARCH_GOTONEXTFOUND:
		{
			_findReplaceDlg.gotoNextFoundResult();
		}
		break;

		case IDM_SEARCH_GOTOPREVFOUND:
		{
			_findReplaceDlg.gotoNextFoundResult(-1);
		}
		break;

		case IDM_FOCUS_ON_FOUND_RESULTS:
		{
			if (GetFocus() == _findReplaceDlg.getHFindResults())
				// focus already on found results, switch to current edit view
				switchEditViewTo(currentView());
			else
				_findReplaceDlg.focusOnFinder();
		}
		break;

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
			{
				generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-end-reached", TEXT("Find: Found the 1st occurrence from the top. The end of the document has been reached."));
				_findReplaceDlg.setStatusbarMessage(msg, FSEndReached);
			}
			else if (status == FSTopReached)
			{
				generic_string msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-top-reached", TEXT("Find: Found the 1st occurrence from the bottom. The beginning of the document has been reached."));
				_findReplaceDlg.setStatusbarMessage(msg, FSTopReached);
			}
		}
		break;

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
		}
		break;

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
		}
		break;

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
		}
		break;

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
		}
		break;

		case IDM_SEARCH_CLEARALLMARKS :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT1);
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT2);
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT3);
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT4);
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_EXT5);
		}
		break;

        case IDM_SEARCH_GOTOLINE :
		{
			bool isFirstTime = !_goToLineDlg.isCreated();
			_goToLineDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_goToLineDlg.getHSelf(), "GoToLine");
		}
		break;

		case IDM_SEARCH_FINDCHARINRANGE :
		{
			bool isFirstTime = !_findCharsInRangeDlg.isCreated();
			_findCharsInRangeDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_findCharsInRangeDlg.getHSelf(), "FindCharsInRange");
		}
		break;

        case IDM_EDIT_COLUMNMODETIP :
		{
			_nativeLangSpeaker.messageBox("ColumnModeTip",
					_pPublicInterface->getHSelf(),
					TEXT("Please use \"ALT+Mouse Selection\" or \"Alt+Shift+Arrow key\" to switch to column mode."),
					TEXT("Column Mode Tip"),
					MB_OK|MB_APPLMODAL);
		}
		break;

        case IDM_EDIT_COLUMNMODE :
		{
			bool isFirstTime = !_colEditorDlg.isCreated();
			_colEditorDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_colEditorDlg.getHSelf(), "ColumnEditor");
		}
		break;

		case IDM_SEARCH_GOTOMATCHINGBRACE :
		case IDM_SEARCH_SELECTMATCHINGBRACES :
		{
			int braceAtCaret = -1;
			int braceOpposite = -1;
			findMatchingBracePos(braceAtCaret, braceOpposite);

			if (braceOpposite != -1)
			{
				if (id == IDM_SEARCH_GOTOMATCHINGBRACE)
					_pEditView->execute(SCI_GOTOPOS, braceOpposite);
				else
					_pEditView->execute(SCI_SETSEL, min(braceAtCaret, braceOpposite), max(braceAtCaret, braceOpposite) + 1); // + 1 so we always include the ending brace in the selection.
			}
		}
		break;

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

                        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), 8, SplitterMode::RIGHT_FIX, 45);
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
        }
		break;

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

		case IDM_EDIT_REMOVE_DUP_LINES:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			removeDuplicateLines();
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_SPLIT_LINES:
			_pEditView->execute(SCI_TARGETFROMSELECTION);
			if (_pEditView->execute(SCI_GETEDGEMODE) == EDGE_NONE)
			{
				_pEditView->execute(SCI_LINESSPLIT);
			}
			else
			{
				auto textWidth = _pEditView->execute(SCI_TEXTWIDTH, STYLE_LINENUMBER, reinterpret_cast<LPARAM>("P"));
				auto edgeCol = _pEditView->execute(SCI_GETEDGECOLUMN);
				_pEditView->execute(SCI_LINESSPLIT, textWidth * edgeCol);
			}
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

		case IDM_EDIT_PROPERCASE_FORCE:
			_pEditView->convertSelectedTextToNewerCase(TITLECASE_FORCE);
			break;

		case IDM_EDIT_PROPERCASE_BLEND:
			_pEditView->convertSelectedTextToNewerCase(TITLECASE_BLEND);
			break;

		case IDM_EDIT_SENTENCECASE_FORCE:
			_pEditView->convertSelectedTextToNewerCase(SENTENCECASE_FORCE);
			break;

		case IDM_EDIT_SENTENCECASE_BLEND:
			_pEditView->convertSelectedTextToNewerCase(SENTENCECASE_BLEND);
			break;

		case IDM_EDIT_INVERTCASE:
			_pEditView->convertSelectedTextToNewerCase(INVERTCASE);
			break;

		case IDM_EDIT_RANDOMCASE:
			_pEditView->convertSelectedTextToNewerCase(RANDOMCASE);
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
		{
			std::lock_guard<std::mutex> lock(command_mutex);

			_pEditView->execute(SCI_BEGINUNDOACTION);
			doTrim(lineTail);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;
		}

		case IDM_EDIT_TRIMLINEHEAD:
		{
			std::lock_guard<std::mutex> lock(command_mutex);

			_pEditView->execute(SCI_BEGINUNDOACTION);
			doTrim(lineHeader);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;
		}

		case IDM_EDIT_TRIM_BOTH:
		{
			std::lock_guard<std::mutex> lock(command_mutex);

			_pEditView->execute(SCI_BEGINUNDOACTION);
			doTrim(lineTail);
			doTrim(lineHeader);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;
		}

		case IDM_EDIT_EOL2WS:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			_pEditView->execute(SCI_TARGETWHOLEDOCUMENT);
			_pEditView->execute(SCI_LINESJOIN);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_TRIMALL:
		{
			std::lock_guard<std::mutex> lock(command_mutex);

			_pEditView->execute(SCI_BEGINUNDOACTION);
			doTrim(lineTail);
			doTrim(lineHeader);
			_pEditView->execute(SCI_TARGETWHOLEDOCUMENT);
			_pEditView->execute(SCI_LINESJOIN);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;
		}

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
			dwFileAttribs &= ~FILE_ATTRIBUTE_READONLY;

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
			int iconDpiDynamicalSize = NppParameters::getInstance()._dpiManager.scaleY(_toReduceTabBar?12:18);

			//Resize the tab height
			int tabDpiDynamicalWidth = NppParameters::getInstance()._dpiManager.scaleX(45);
			int tabDpiDynamicalHeight = NppParameters::getInstance()._dpiManager.scaleY(_toReduceTabBar?22:25);
			TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			_docTabIconList.setIconSize(iconDpiDynamicalSize);

			//change the font
			int stockedFont = _toReduceTabBar?DEFAULT_GUI_FONT:SYSTEM_FONT;
			HFONT hf = (HFONT)::GetStockObject(stockedFont);

			if (hf)
			{
				::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));
				::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));
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
			int tabDpiDynamicalHeight = NppParameters::getInstance()._dpiManager.scaleY(22);
			int tabDpiDynamicalWidth = NppParameters::getInstance()._dpiManager.scaleX(TabBarPlus::drawTabCloseButton() ? 60 : 45);
			TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);

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

		case IDM_VIEW_IN_FIREFOX:
		case IDM_VIEW_IN_CHROME:
		case IDM_VIEW_IN_IE:
		{
			auto currentBuf = _pEditView->getCurrentBuffer();
			if (!currentBuf->isUntitled())
			{
				generic_string appName;

				if (id == IDM_VIEW_IN_FIREFOX)
				{
					appName = TEXT("firefox.exe");
				}
				else if (id == IDM_VIEW_IN_CHROME)
				{
					appName = TEXT("chrome.exe");
				}
				else // if (id == IDM_VIEW_IN_IE)
				{
					appName = TEXT("IEXPLORE.EXE");
				}

				TCHAR valData[MAX_PATH] = {'\0'};
				DWORD valDataLen = MAX_PATH * sizeof(TCHAR);
				DWORD valType;
				HKEY hKey2Check = nullptr;
				generic_string appEntry = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
				appEntry += appName;
				::RegOpenKeyEx(HKEY_LOCAL_MACHINE, appEntry.c_str(), 0, KEY_READ, &hKey2Check);
				::RegQueryValueEx(hKey2Check, TEXT(""), nullptr, &valType, reinterpret_cast<LPBYTE>(valData), &valDataLen);


				generic_string fullCurrentPath = TEXT("\"");
				fullCurrentPath += currentBuf->getFullPathName();
				fullCurrentPath += TEXT("\"");

				if (hKey2Check && valData[0] != '\0')
				{
					::ShellExecute(NULL, TEXT("open"), valData, fullCurrentPath.c_str(), NULL, SW_SHOWNORMAL);
				}
				else
				{
					_nativeLangSpeaker.messageBox("ViewInBrowser",
						_pPublicInterface->getHSelf(),
						TEXT("Application cannot be found in your system."),
						TEXT("View Current File in Browser"),
						MB_OK);
				}
				::RegCloseKey(hKey2Check);
			}
		}
		break;
		
		case IDM_VIEW_IN_EDGE:
		{
			auto currentBuf = _pEditView->getCurrentBuffer();
			if (!currentBuf->isUntitled())
			{
				// Don't put the quots for Edge, otherwise it doesn't work
				//fullCurrentPath = TEXT("\"");
				generic_string fullCurrentPath = currentBuf->getFullPathName();
				//fullCurrentPath += TEXT("\"");

				ShellExecute(NULL, TEXT("open"), TEXT("shell:Appsfolder\\Microsoft.MicrosoftEdge_8wekyb3d8bbwe!MicrosoftEdge"), fullCurrentPath.c_str(), NULL, SW_SHOW);
			}
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

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance()).getSVP();
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

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance()).getSVP();
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

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance()).getSVP();
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

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance()).getSVP();
            svp1._indentGuideLineShow = _pEditView->isShownIndentGuide();
			break;
		}

		case IDM_VIEW_WRAP:
		{
			bool isWraped = !_pEditView->isWrap();
			// ViewMoveAtWrappingDisableFix: Disable wrapping messes up visible lines. Therefore save view position before in IDM_VIEW_WRAP and restore after SCN_PAINTED, as Scintilla-Doc. says
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

			ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance()).getSVP();
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

            ScintillaViewParams & svp1 = (ScintillaViewParams &)(NppParameters::getInstance()).getSVP();
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
				int mainCurrentLine = static_cast<int32_t>(_mainEditView.execute(SCI_GETFIRSTVISIBLELINE));
				int subCurrentLine = static_cast<int32_t>(_subEditView.execute(SCI_GETFIRSTVISIBLELINE));
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
				int mxoffset = static_cast<int32_t>(_mainEditView.execute(SCI_GETXOFFSET));
				int pixel = static_cast<int32_t>(_mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P")));
				int mainColumn = mxoffset/pixel;

				int sxoffset = static_cast<int32_t>(_subEditView.execute(SCI_GETXOFFSET));
				pixel = int(_subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P")));
				int subColumn = sxoffset/pixel;
				_syncInfo._column = mainColumn - subColumn;
			}
		}
		break;

		case IDM_VIEW_SUMMARY:
		{
			generic_string characterNumber = TEXT("");

			Buffer * curBuf = _pEditView->getCurrentBuffer();
			int64_t fileLen = curBuf->getFileLength();

			// localization for summary date
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			if (pNativeSpeaker)
			{

				if (fileLen != -1)
				{
					generic_string filePathLabel = pNativeSpeaker->getLocalizedStrFromID("summary-filepath", TEXT("Full file path: "));
					generic_string fileCreateTimeLabel = pNativeSpeaker->getLocalizedStrFromID("summary-filecreatetime", TEXT("Created: "));
					generic_string fileModifyTimeLabel = pNativeSpeaker->getLocalizedStrFromID("summary-filemodifytime", TEXT("Modified: "));

					characterNumber += filePathLabel;
					characterNumber += curBuf->getFullPathName();
					characterNumber += TEXT("\r");

					characterNumber += fileCreateTimeLabel;
					characterNumber += curBuf->getFileTime(Buffer::ft_created);
					characterNumber += TEXT("\r");

					characterNumber += fileModifyTimeLabel;
					characterNumber += curBuf->getFileTime(Buffer::ft_modified);
					characterNumber += TEXT("\r");
				}
				generic_string nbCharLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbchar", TEXT("Characters (without line endings): "));
				generic_string nbWordLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbword", TEXT("Words: "));
				generic_string nbLineLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbline", TEXT("Lines: "));
				generic_string nbByteLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbbyte", TEXT("Document length: "));
				generic_string nbSelLabel1 = pNativeSpeaker->getLocalizedStrFromID("summary-nbsel1", TEXT(" selected characters ("));
				generic_string nbSelLabel2 = pNativeSpeaker->getLocalizedStrFromID("summary-nbsel2", TEXT(" bytes) in "));
				generic_string nbRangeLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbrange", TEXT(" ranges"));

				UniMode um = _pEditView->getCurrentBuffer()->getUnicodeMode();
				auto nbChar = getCurrentDocCharCount(um);
				int nbWord = wordCount();
				auto nbLine = _pEditView->execute(SCI_GETLINECOUNT);
				auto nbByte = _pEditView->execute(SCI_GETLENGTH);
				auto nbSel = getSelectedCharNumber(um);
				auto nbSelByte = getSelectedBytes();
				auto nbRange = getSelectedAreas();

				characterNumber += nbCharLabel;
				characterNumber += commafyInt(nbChar).c_str();
				characterNumber += TEXT("\r");

				characterNumber += nbWordLabel;
				characterNumber += commafyInt(nbWord).c_str();
				characterNumber += TEXT("\r");

				characterNumber += nbLineLabel;
				characterNumber += commafyInt(static_cast<int>(nbLine)).c_str();
				characterNumber += TEXT("\r");

				characterNumber += nbByteLabel;
				characterNumber += commafyInt(nbByte).c_str();
				characterNumber += TEXT("\r");

				characterNumber += commafyInt(nbSel).c_str();
				characterNumber += nbSelLabel1;
				characterNumber += commafyInt(nbSelByte).c_str();
				characterNumber += nbSelLabel2;
				characterNumber += commafyInt(nbRange).c_str();
				characterNumber += nbRangeLabel;
				characterNumber += TEXT("\r");

				generic_string summaryLabel = pNativeSpeaker->getLocalizedStrFromID("summary", TEXT("Summary"));

				::MessageBox(_pPublicInterface->getHSelf(), characterNumber.c_str(), summaryLabel.c_str(), MB_OK|MB_APPLMODAL);
			}
		}
		break;

		case IDM_VIEW_MONITORING:
		{
			Buffer * curBuf = _pEditView->getCurrentBuffer();
			if (curBuf->isMonitoringOn())
			{
				monitoringStartOrStopAndUpdateUI(curBuf, false);
			}
			else
			{
				const TCHAR *longFileName = curBuf->getFullPathName();
				if (::PathFileExists(longFileName))
				{
					if (curBuf->isDirty())
					{
						_nativeLangSpeaker.messageBox("DocTooDirtyToMonitor",
							_pPublicInterface->getHSelf(),
							TEXT("The document is dirty. Please save the modification before monitoring it."),
							TEXT("Monitoring problem"),
							MB_OK);
					}
					else
					{
						// Monitoring firstly for making monitoring icon
						monitoringStartOrStopAndUpdateUI(curBuf, true);
						
						MonitorInfo *monitorInfo = new MonitorInfo(curBuf, _pPublicInterface->getHSelf());
						HANDLE hThread = ::CreateThread(NULL, 0, monitorFileOnChange, (void *)monitorInfo, 0, NULL); // will be deallocated while quitting thread
						::CloseHandle(hThread);
					}
				}
				else
				{
					_nativeLangSpeaker.messageBox("DocNoExistToMonitor",
						_pPublicInterface->getHSelf(),
						TEXT("The file should exist to be monitored."),
						TEXT("Monitoring problem"),
						MB_OK);
				}
			}

			break;
		}

		case IDM_EXECUTE:
		{
			bool isFirstTime = !_runDlg.isCreated();
			_runDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_runDlg.getHSelf(), "Run");

			break;
		}

		case IDM_FORMAT_TODOS:
		case IDM_FORMAT_TOUNIX:
		case IDM_FORMAT_TOMAC:
		{
			EolType newFormat = (id == IDM_FORMAT_TODOS)
				? EolType::windows
				: (id == IDM_FORMAT_TOUNIX) ? EolType::unix : EolType::macos;

			Buffer* buf = _pEditView->getCurrentBuffer();

			if (not buf->isReadOnly())
			{
				std::lock_guard<std::mutex> lock(command_mutex);
				buf->setEolFormat(newFormat);
				_pEditView->execute(SCI_CONVERTEOLS, static_cast<int>(buf->getEolFormat()));
			}

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
						_pPublicInterface->getHSelf(),
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
						_pPublicInterface->getHSelf(),
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
        case IDM_FORMAT_ISO_8859_13 :
        case IDM_FORMAT_ISO_8859_14 :
        case IDM_FORMAT_ISO_8859_15 :
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

			EncodingMapper& em = EncodingMapper::getInstance();
			int encoding = em.getEncodingFromIndex(index);
			if (encoding == -1)
			{
				//printStr(TEXT("Encoding problem. Command is not added in encoding_table?"));
				return;
			}

            Buffer* buf = _pEditView->getCurrentBuffer();
            if (buf->isDirty())
            {
				generic_string warning, title;
				int answer = _nativeLangSpeaker.messageBox("SaveCurrentModifWarning",
					_pPublicInterface->getHSelf(),
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
					_pPublicInterface->getHSelf(),
					TEXT("You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?"),
					TEXT("Lose Undo Ability Waning"),
					MB_YESNO);

                if (answer != IDYES)
                    return;
            }

            if (not buf->isDirty())
            {
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
				int len = static_cast<int32_t>(::GlobalSize(clipboardData));
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
				_pEditView->restoreCurrentPosPreStep();

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
			// Copy plugins to Plugins Home
            const TCHAR *extFilterName = TEXT("Notepad++ plugin");
            const TCHAR *extFilter = TEXT(".dll");
            vector<generic_string> copiedFiles = addNppPlugins(extFilterName, extFilter);

            // Tell users to restart Notepad++ to load plugin
			if (copiedFiles.size())
			{
				NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
				pNativeSpeaker->messageBox("NeedToRestartToLoadPlugins",
					NULL,
					TEXT("You have to restart Notepad++ to load plugins you installed."),
					TEXT("Notepad++ need to be relaunched"),
					MB_OK | MB_APPLMODAL);
			}
            break;
        }

        case IDM_SETTING_IMPORTSTYLETHEMS :
        {
            // get plugin source path
            const TCHAR *extFilterName = TEXT("Notepad++ style theme");
            const TCHAR *extFilter = TEXT(".xml");
            const TCHAR *destDir = TEXT("themes");

            // load styler
            NppParameters& nppParams = NppParameters::getInstance();
            ThemeSwitcher & themeSwitcher = nppParams.getThemeSwitcher();

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

		case IDM_SETTING_PLUGINADM:
		{
			bool isFirstTime = !_pluginsAdminDlg.isCreated();
			_pluginsAdminDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
			{
				_nativeLangSpeaker.changePluginsAdminDlgLang(_pluginsAdminDlg);
				_pluginsAdminDlg.updateListAndLoadFromJson();
			}
			break;
		}

		case IDM_SETTING_OPENPLUGINSDIR:
		{
			const TCHAR* pluginHomePath = NppParameters::getInstance().getPluginRootDir();
			if (pluginHomePath && pluginHomePath[0])
			{
				::ShellExecute(NULL, NULL, pluginHomePath, NULL, NULL, SW_SHOWNORMAL);
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
			shortcutMapper.doDialog(_nativeLangSpeaker.isRTL());
			shortcutMapper.destroy();
			break;
		}
		case IDM_SETTING_PREFERENCE:
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
			_nativeLangSpeaker.messageBox("ContextMenuXmlEditWarning",
				_pPublicInterface->getHSelf(),
				TEXT("Editing contextMenu.xml allows you to modify your Notepad++ popup context menu on edit zone.\rYou have to restart your Notepad++ to take effect after modifying contextMenu.xml."),
				TEXT("Editing contextMenu"),
				MB_OK|MB_APPLMODAL);

            NppParameters& nppParams = NppParameters::getInstance();
            BufferID bufID = doOpen((nppParams.getContextMenuPath()));
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

		case IDM_TOOL_MD5_GENERATE:
		{
			bool isFirstTime = !_md5FromTextDlg.isCreated();
			_md5FromTextDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_md5FromTextDlg.getHSelf(), "MD5FromTextDlg");
		}
		break;

		case IDM_TOOL_MD5_GENERATEFROMFILE:
		{
			bool isFirstTime = !_md5FromFilesDlg.isCreated();
			_md5FromFilesDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_md5FromFilesDlg.getHSelf(), "MD5FromFilesDlg");
		}
		break;

		case IDM_TOOL_MD5_GENERATEINTOCLIPBOARD:
		{
			if (_pEditView->execute(SCI_GETSELECTIONS) == 1)
			{
				size_t selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
				size_t selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);

				int32_t strLen = static_cast<int32_t>(selectionEnd - selectionStart);
				if (strLen)
				{
					int strSize = strLen + 1;
					char *selectedStr = new char[strSize];
					_pEditView->execute(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(selectedStr));

					MD5 md5;
					std::string md5ResultA = md5.digestString(selectedStr);
					std::wstring md5ResultW(md5ResultA.begin(), md5ResultA.end());
					str2Clipboard(md5ResultW, _pPublicInterface->getHSelf());
					
					delete [] selectedStr;
				}
			}
		}
		break;

		case IDM_TOOL_SHA256_GENERATE:
		{
			bool isFirstTime = !_sha2FromTextDlg.isCreated();
			_sha2FromTextDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_sha2FromTextDlg.getHSelf(), "SHA256FromTextDlg");
		}
		break;

		case IDM_TOOL_SHA256_GENERATEFROMFILE:
		{
			bool isFirstTime = !_sha2FromFilesDlg.isCreated();
			_sha2FromFilesDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_sha2FromFilesDlg.getHSelf(), "SHA256FromFilesDlg");
		}
		break;

		case IDM_TOOL_SHA256_GENERATEINTOCLIPBOARD:
		{
			if (_pEditView->execute(SCI_GETSELECTIONS) == 1)
			{
				size_t selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
				size_t selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);

				int32_t strLen = static_cast<int32_t>(selectionEnd - selectionStart);
				if (strLen)
				{
					int strSize = strLen + 1;
					char *selectedStr = new char[strSize];
					_pEditView->execute(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(selectedStr));

					uint8_t sha2hash[32];
					calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(selectedStr), strlen(selectedStr));

					wchar_t sha2hashStr[65] = { '\0' };
					for (size_t i = 0; i < 32; i++)
						wsprintf(sha2hashStr + i * 2, TEXT("%02x"), sha2hash[i]);

					str2Clipboard(sha2hashStr, _pPublicInterface->getHSelf());

					delete[] selectedStr;
				}
			}
		}
		break;

		case IDM_DEBUGINFO:
		{
			_debugInfoDlg.doDialog();
			break;
		}

        case IDM_ABOUT:
		{
			bool doAboutDlg = false;
			const int maxSelLen = 32;
			auto textLen = _pEditView->execute(SCI_GETSELTEXT, 0, 0) - 1;
			if (!textLen)
				doAboutDlg = true;
			if (textLen > maxSelLen)
				doAboutDlg = true;

			if (!doAboutDlg)
			{
				char author[maxSelLen+1] = "";
				_pEditView->getSelectedText(author, maxSelLen + 1);
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
				const wchar_t * authorW = wmc.char2wchar(author, _nativeLangSpeaker.getLangEncoding());
				int iQuote = getQuoteIndexFrom(authorW);

				if (iQuote == -1)
				{
					doAboutDlg = true;
				}
				else if (iQuote == -2)
				{
					generic_string noEasterEggsPath((NppParameters::getInstance()).getNppPath());
					noEasterEggsPath.append(TEXT("\\noEasterEggs.xml"));
					if (!::PathFileExists(noEasterEggsPath.c_str()))
						showAllQuotes();
					return;
				}
				if (iQuote != -1)
				{
					generic_string noEasterEggsPath((NppParameters::getInstance()).getNppPath());
					noEasterEggsPath.append(TEXT("\\noEasterEggs.xml"));
					if (!::PathFileExists(noEasterEggsPath.c_str()))
						showQuoteFromIndex(iQuote);
					return;
				}
			}

			if (doAboutDlg)
			{
				//bool isFirstTime = !_aboutDlg.isCreated();
				_aboutDlg.doDialog();
				/*
				if (isFirstTime && _nativeLangSpeaker.getNativeLangA())
				{
					if (_nativeLangSpeaker.getLangEncoding() == NPP_CP_BIG5)
					{
						const char *authorName = "«J¤µ§^";
						HWND hItem = ::GetDlgItem(_aboutDlg.getHSelf(), IDC_AUTHOR_NAME);

						WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
						const wchar_t *authorNameW = wmc.char2wchar(authorName, NPP_CP_BIG5);
						::SetWindowText(hItem, authorNameW);
					}
				}
				*/
			}
			break;
		}

		case IDM_HELP :
		{
			generic_string tmp((NppParameters::getInstance()).getNppPath());
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
			::ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/notepad-plus-plus/notepad-plus-plus/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_ONLINEDOCUMENT:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("https://npp-user-manual.org/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_CMDLINEARGUMENTS:
		{
			// No translattable
			::MessageBox(_pPublicInterface->getHSelf(), COMMAND_ARG_HELP, TEXT("Notepad++ Command Argument Help"), MB_OK | MB_APPLMODAL);
			break;
		}

		case IDM_FORUM:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("https://community.notepad-plus-plus.org/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_ONLINESUPPORT:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("https://gitter.im/notepad-plus-plus/notepad-plus-plus"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_UPDATE_NPP :
		case IDM_CONFUPDATERPROXY :
		{
			// wingup doesn't work with the obsolete security layer (API) under xp since downloadings are secured with SSL on notepad_plus_plus.org
			winVer ver = NppParameters::getInstance().getWinVersion();
			if (ver <= WV_XP)
			{
				long res = _nativeLangSpeaker.messageBox("XpUpdaterProblem",
					_pPublicInterface->getHSelf(),
					TEXT("Notepad++ updater is not compatible with XP due to the obsolete security layer under XP.\rDo you want to go to Notepad++ page to download the latest version?"),
					TEXT("Notepad++ Updater"),
					MB_YESNO);

				if (res == IDYES)
				{
					::ShellExecute(NULL, TEXT("open"), TEXT("https://notepad-plus-plus.org/downloads/"), NULL, NULL, SW_SHOWNORMAL);
				}
			}
			else
			{
				generic_string updaterDir = (NppParameters::getInstance()).getNppPath();
				PathAppend(updaterDir, TEXT("updater"));

				generic_string updaterFullPath = updaterDir;
				PathAppend(updaterFullPath, TEXT("gup.exe"));


#ifdef DEBUG // if not debug, then it's release
				bool isCertifVerified = true;
#else //RELEASE
				// check the signature on updater
				SecurityGard securityGard;
				bool isCertifVerified = securityGard.checkModule(updaterFullPath, nm_gup);
#endif
				if (isCertifVerified)
				{
					generic_string param;
					if (id == IDM_CONFUPDATERPROXY)
					{
						if (!_isAdministrator)
						{
							_nativeLangSpeaker.messageBox("GUpProxyConfNeedAdminMode",
								_pPublicInterface->getHSelf(),
								TEXT("Please relaunch Notepad++ in Admin mode to configure proxy."),
								TEXT("Proxy Settings"),
								MB_OK | MB_APPLMODAL);
							return;
						}
						param = TEXT("-options");
					}
					else
					{
						param = TEXT("-verbose -v");
						param += VERSION_VALUE;

						if (NppParameters::getInstance().isx64())
						{
							param += TEXT(" -px64");
						}
					}
					Process updater(updaterFullPath.c_str(), param.c_str(), updaterDir.c_str());

					updater.run();
				}
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
		case IDM_LANG_JSON :
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
		case IDM_LANG_FORTRAN_77 :
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
		case IDM_LANG_BAANC:
		case IDM_LANG_SREC:
		case IDM_LANG_IHEX:
		case IDM_LANG_TEHEX:
		case IDM_LANG_SWIFT:
        case IDM_LANG_ASN1 :
        case IDM_LANG_AVS :
        case IDM_LANG_BLITZBASIC :
        case IDM_LANG_PUREBASIC :
        case IDM_LANG_FREEBASIC :
        case IDM_LANG_CSOUND :
        case IDM_LANG_ERLANG :
        case IDM_LANG_ESCRIPT :
        case IDM_LANG_FORTH :
        case IDM_LANG_LATEX :
        case IDM_LANG_MMIXAL :
        case IDM_LANG_NIMROD :
        case IDM_LANG_NNCRONTAB :
        case IDM_LANG_OSCRIPT :
        case IDM_LANG_REBOL :
        case IDM_LANG_REGISTRY :
        case IDM_LANG_RUST :
        case IDM_LANG_SPICE :
        case IDM_LANG_TXT2TAGS :
        case IDM_LANG_VISUALPROLOG:
		case IDM_LANG_USER :
		{
            setLanguage(menuID2LangType(id));
			// Manually set language, don't change language even file extension changes.
			Buffer *buffer = _pEditView->getCurrentBuffer();
			buffer->langHasBeenSetFromMenu();

			if (_pDocMap)
			{
				_pDocMap->setSyntaxHiliting();
			}
		}
        break;
		
		case IDM_LANG_OPENUDLDIR:
		{
			generic_string userDefineLangFolderPath = NppParameters::getInstance().getUserDefineLangFolderPath();
			::ShellExecute(_pPublicInterface->getHSelf(), TEXT("open"), userDefineLangFolderPath.c_str(), NULL, NULL, SW_SHOW);
			break;
		}

        case IDC_PREV_DOC :
        case IDC_NEXT_DOC :
        {
			size_t nbDoc = viewVisible(MAIN_VIEW) ? _mainDocTab.nbItem() : 0;
			nbDoc += viewVisible(SUB_VIEW)?_subDocTab.nbItem():0;

			bool doTaskList = ((NppParameters::getInstance()).getNppGUI())._doTaskList;
			_isFolding = true;
			if (nbDoc > 1)
			{
				bool direction = (id == IDC_NEXT_DOC)?dirDown:dirUp;
				if (!doTaskList)
				{
					activateNextDoc(direction);
				}
				else
				{
					if (TaskListDlg::_instanceCount == 0)
					{
						TaskListDlg tld;
						HIMAGELIST hImgLst = _docTabIconList.getHandle();
						tld.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), hImgLst, direction);
						tld.doDialog();
					}
				}
			}
			_isFolding = false;
			_linkTriggered = true;
		}
        break;

		case IDM_OPEN_ALL_RECENT_FILE :
		{
			BufferID lastOne = BUFFER_INVALID;
			int size = _lastRecentFileList.getSize();
			for (int i = size - 1; i >= 0; i--)
			{
				BufferID test = doOpen(_lastRecentFileList.getIndex(i));
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
			_windowsDlg.doDialog();
		}
		break;


		case IDM_SYSTRAYPOPUP_NEWDOC:
		{
			NppGUI & nppGUI = const_cast<NppGUI &>((NppParameters::getInstance()).getNppGUI());
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			fileNew();
		}
		break;

		case IDM_SYSTRAYPOPUP_ACTIVATE :
		{
			NppGUI & nppGUI = const_cast<NppGUI &>((NppParameters::getInstance()).getNppGUI());
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);

			// Send sizing info to make window fit (specially to show tool bar. Fixed issue #2600)
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
		}
		break;

		case IDM_SYSTRAYPOPUP_NEW_AND_PASTE:
		{
			NppGUI & nppGUI = const_cast<NppGUI &>((NppParameters::getInstance()).getNppGUI());
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			BufferID bufferID = _pEditView->getCurrentBufferID();
			Buffer * buf = MainFileManager.getBufferByID(bufferID);
			if (!buf->isUntitled() || buf->docLength() != 0)
			{
				fileNew();
			}
			command(IDM_EDIT_PASTE);
		}
		break;

		case IDM_SYSTRAYPOPUP_OPENFILE:
		{
			NppGUI & nppGUI = const_cast<NppGUI &>((NppParameters::getInstance()).getNppGUI());
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);

			// Send sizing info to make window fit (specially to show tool bar. Fixed issue #2600)
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
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
			if (not lastOpenedFullPath.empty())
			{
				BufferID lastOpened = doOpen(lastOpenedFullPath);
				if (lastOpened != BUFFER_INVALID)
					switchToFile(lastOpened);
			}
		}
		break;

		case IDM_VIEW_LINENUMBER:
		case IDM_VIEW_SYMBOLMARGIN:
		case IDM_VIEW_DOCCHANGEMARGIN:
		{
			int margin;
			if (id == IDM_VIEW_LINENUMBER)
				margin = ScintillaEditView::_SC_MARGE_LINENUMBER;
			else //if (id == IDM_VIEW_SYMBOLMARGIN)
				margin = ScintillaEditView::_SC_MARGE_SYBOLE;

			if (_mainEditView.hasMarginShowed(margin))
			{
				_mainEditView.showMargin(margin, false);
				_subEditView.showMargin(margin, false);
			}
			else
			{
				_mainEditView.showMargin(margin);
				_subEditView.showMargin(margin);
			}
		}
		break;

		case IDM_VIEW_FOLDERMAGIN_SIMPLE:
		case IDM_VIEW_FOLDERMAGIN_ARROW:
		case IDM_VIEW_FOLDERMAGIN_CIRCLE:
		case IDM_VIEW_FOLDERMAGIN_BOX:
		case IDM_VIEW_FOLDERMAGIN:
		{
			folderStyle fStyle = (id == IDM_VIEW_FOLDERMAGIN_SIMPLE) ? FOLDER_STYLE_SIMPLE : \
				(id == IDM_VIEW_FOLDERMAGIN_ARROW) ? FOLDER_STYLE_ARROW : \
				(id == IDM_VIEW_FOLDERMAGIN_CIRCLE) ? FOLDER_STYLE_CIRCLE : \
				(id == IDM_VIEW_FOLDERMAGIN) ? FOLDER_STYLE_NONE : FOLDER_STYLE_BOX;

			_mainEditView.setMakerStyle(fStyle);
			_subEditView.setMakerStyle(fStyle);
		}
		break;

		case IDM_VIEW_CURLINE_HILITING:
		{
			COLORREF colour = (NppParameters::getInstance()).getCurLineHilitingColour();
			_mainEditView.setCurrentLineHiLiting(!_pEditView->isCurrentLineHiLiting(), colour);
			_subEditView.setCurrentLineHiLiting(!_pEditView->isCurrentLineHiLiting(), colour);
		}
		break;

		case IDM_VIEW_LWDEF:
		case IDM_VIEW_LWALIGN:
		case IDM_VIEW_LWINDENT:
		{
			int mode = (id == IDM_VIEW_LWALIGN) ? SC_WRAPINDENT_SAME : \
				(id == IDM_VIEW_LWINDENT) ? SC_WRAPINDENT_INDENT : SC_WRAPINDENT_FIXED;
			_mainEditView.execute(SCI_SETWRAPINDENTMODE, mode);
			_subEditView.execute(SCI_SETWRAPINDENTMODE, mode);
		}
		break;

		default :
			if (id > IDM_FILEMENU_LASTONE && id < (IDM_FILEMENU_LASTONE + _lastRecentFileList.getMaxNbLRF() + 1))
			{
				BufferID lastOpened = doOpen(_lastRecentFileList.getItem(id));
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
				if (_pDocMap)
				{
					_pDocMap->setSyntaxHiliting();
				}
			}
			else if ((id >= IDM_LANG_EXTERNAL) && (id <= IDM_LANG_EXTERNAL_LIMIT))
			{
				setLanguage((LangType)(id - IDM_LANG_EXTERNAL + L_EXTERNAL));
				if (_pDocMap)
				{
					_pDocMap->setSyntaxHiliting();
				}
			}
			else if ((id >= ID_MACRO) && (id < ID_MACRO_LIMIT))
			{
				int i = id - ID_MACRO;
				vector<MacroShortcut> & theMacros = (NppParameters::getInstance()).getMacroList();
				macroPlayback(theMacros[i].getMacro());
			}
			else if ((id >= ID_USER_CMD) && (id < ID_USER_CMD_LIMIT))
			{
				int i = id - ID_USER_CMD;
				vector<UserCommand> & theUserCommands = (NppParameters::getInstance()).getUserCommandList();
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
			case IDM_FILE_CLOSEALL_UNCHANGED:
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
			case IDM_SEARCH_INVERSEMARKS:
			case IDM_EDIT_SELECTALL:
			case IDM_EDIT_INS_TAB:
			case IDM_EDIT_RMV_TAB:
			case IDM_EDIT_DUP_LINE:
			case IDM_EDIT_REMOVE_DUP_LINES:
			case IDM_EDIT_TRANSPOSE_LINE:
			case IDM_EDIT_SPLIT_LINES:
			case IDM_EDIT_JOIN_LINES:
			case IDM_EDIT_LINE_UP:
			case IDM_EDIT_LINE_DOWN:
			case IDM_EDIT_REMOVEEMPTYLINES:
			case IDM_EDIT_REMOVEEMPTYLINESWITHBLANK:
			case IDM_EDIT_UPPERCASE:
			case IDM_EDIT_LOWERCASE:
			case IDM_EDIT_PROPERCASE_FORCE:
			case IDM_EDIT_PROPERCASE_BLEND:
			case IDM_EDIT_SENTENCECASE_FORCE:
			case IDM_EDIT_SENTENCECASE_BLEND:
			case IDM_EDIT_INVERTCASE:
			case IDM_EDIT_RANDOMCASE:
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
			case IDM_VIEW_TAB_MOVEFORWARD:
			case IDM_VIEW_TAB_MOVEBACKWARD:
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
			case IDM_FORMAT_TODOS  :
			case IDM_FORMAT_TOUNIX :
			case IDM_FORMAT_TOMAC  :
			case IDM_VIEW_IN_FIREFOX :
			case IDM_VIEW_IN_CHROME  :
			case IDM_VIEW_IN_EDGE    :
			case IDM_VIEW_IN_IE      :
				_macro.push_back(recordedMacroStep(id));
				break;
		}
}
