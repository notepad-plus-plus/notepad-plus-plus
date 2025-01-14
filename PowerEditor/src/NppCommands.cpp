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

#include <memory>
#include <regex>
#include <shlwapi.h>
#include "Notepad_plus_Window.h"
#include "EncodingMapper.h"
#include "ShortcutMapper.h"
#include "TaskListDlg.h"
#include "clipboardFormats.h"
#include "VerticalFileSwitcher.h"
#include "documentMap.h"
#include "functionListPanel.h"
#include "ProjectPanel.h"
#include "fileBrowser.h"
#include "clipboardHistoryPanel.h"
#include "ansiCharPanel.h"
#include "Sorters.h"
#include "verifySignedfile.h"
#include "md5.h"
#include "sha-256.h"
#include "calc_sha1.h"
#include "sha512.h"

using namespace std;

std::mutex command_mutex;

void Notepad_plus::macroPlayback(Macro macro)
{
	_playingBackMacro = true;
	_pEditView->execute(SCI_BEGINUNDOACTION);

	for (Macro::iterator step = macro.begin(); step != macro.end(); ++step)
	{
		if (step->isScintillaMacro())
			step->PlayBack(_pPublicInterface, _pEditView);
		else
			_findReplaceDlg.execSavedCommand(step->_message, step->_lParameter, string2wstring(step->_sParameter, CP_UTF8));
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

		case IDM_EDIT_INSERT_DATETIME_SHORT:
		case IDM_EDIT_INSERT_DATETIME_LONG:
		{
			SYSTEMTIME currentTime = {};
			::GetLocalTime(&currentTime);

			wchar_t dateStr[128] = { '\0' };
			wchar_t timeStr[128] = { '\0' };

			int dateFlag = (id == IDM_EDIT_INSERT_DATETIME_SHORT) ? DATE_SHORTDATE : DATE_LONGDATE;
			GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, dateFlag, &currentTime, NULL, dateStr, sizeof(dateStr) / sizeof(dateStr[0]), NULL);
			GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, &currentTime, NULL, timeStr, sizeof(timeStr) / sizeof(timeStr[0]));

			wstring dateTimeStr;
			if (NppParameters::getInstance().getNppGUI()._dateTimeReverseDefaultOrder)
			{
				// reverse default order: DATE + TIME
				dateTimeStr = dateStr;
				dateTimeStr += L" ";
				dateTimeStr += timeStr;
			}
			else
			{
				// default: TIME + DATE (Microsoft Notepad behaviour)
				dateTimeStr = timeStr;
				dateTimeStr += L" ";
				dateTimeStr += dateStr;
			}
			_pEditView->execute(SCI_BEGINUNDOACTION);

			_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
			_pEditView->addGenericText(dateTimeStr.c_str());

			_pEditView->execute(SCI_ENDUNDOACTION);
		}
		break;

		case IDM_EDIT_INSERT_DATETIME_CUSTOMIZED:
		{
			SYSTEMTIME currentTime = {};
			::GetLocalTime(&currentTime);

			NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
			wstring dateTimeStr = getDateTimeStrFrom(nppGUI._dateTimeFormat, currentTime);

			_pEditView->execute(SCI_BEGINUNDOACTION);

			_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
			_pEditView->addGenericText(dateTimeStr.c_str());

			_pEditView->execute(SCI_ENDUNDOACTION);
		}
		break;

		case IDM_FILE_OPEN:
		{
			fileOpen();
		}
		break;

		case IDM_FILE_OPEN_FOLDER:
		{
			Command cmd(L"explorer /select,\"$(FULL_CURRENT_PATH)\"");
			cmd.run(_pPublicInterface->getHSelf());
		}
		break;

		case IDM_FILE_OPEN_CMD:
		{
			Command cmd(NppParameters::getInstance().getNppGUI()._commandLineInterpreter.c_str());
			cmd.run(_pPublicInterface->getHSelf(), L"$(CURRENT_DIRECTORY)");
		}
		break;

		case IDM_FILE_CONTAININGFOLDERASWORKSPACE:
		{
			wchar_t currentFile[CURRENTWORD_MAXLENGTH] = { '\0' };
			wchar_t currentDir[CURRENTWORD_MAXLENGTH] = { '\0' };
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_GETFULLCURRENTPATH, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(currentFile));
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_GETCURRENTDIRECTORY, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(currentDir));
	
			if (!_pFileBrowser)
			{
				command(IDM_VIEW_FILEBROWSER);
			}

			vector<wstring> folders;
			folders.push_back(currentDir);
			
			launchFileBrowser(folders, currentFile);
		}
		break;

		case IDM_FILE_OPEN_DEFAULT_VIEWER:
		{
			// Opens file in its default viewer. 
            // Has the same effect as double–clicking this file in Windows Explorer.
            BufferID buf = _pEditView->getCurrentBufferID();
			HINSTANCE res = ::ShellExecute(NULL, L"open", buf->getFullPathName(), NULL, NULL, SW_SHOW);

			// As per MSDN (https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx)
			// If the function succeeds, it returns a value greater than 32.
			// If the function fails, it returns an error value that indicates the cause of the failure.
			int retResult = static_cast<int>(reinterpret_cast<intptr_t>(res));
			if (retResult <= 32)
			{
				wstring errorMsg;
				errorMsg += GetLastErrorAsString(retResult);
				errorMsg += L"An attempt was made to execute the below command.";
				errorMsg += L"\n----------------------------------------------------------";
				errorMsg += L"\nCommand: ";
				errorMsg += buf->getFullPathName();
				errorMsg += L"\nError Code: ";
				errorMsg += intToString(retResult);
				errorMsg += L"\n----------------------------------------------------------";
				
				::MessageBox(_pPublicInterface->getHSelf(), errorMsg.c_str(), L"ShellExecute - ERROR", MB_ICONINFORMATION | MB_APPLMODAL);
			}
		}
		break;

		case IDM_FILE_OPENFOLDERASWORSPACE:
		{
			const NativeLangSpeaker* pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
			wstring openWorkspaceStr = pNativeSpeaker->getAttrNameStr(L"Select a folder to add in Folder as Workspace panel",
				FOLDERASWORKSPACE_NODE, "SelectFolderFromBrowserString");
			wstring folderPath = folderBrowser(_pPublicInterface->getHSelf(), openWorkspaceStr);
			if (!folderPath.empty())
			{
				if (_pFileBrowser == nullptr) // first launch, check in params to open folders
				{
					vector<wstring> dummy;
					wstring emptyStr;
					launchFileBrowser(dummy, emptyStr);
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

		case IDM_DOCLIST_FILESCLOSE:
		case IDM_DOCLIST_FILESCLOSEOTHERS:
			if (_pDocumentListPanel)
			{
				vector<BufferViewInfo> bufs2Close = _pDocumentListPanel->getSelectedFiles(id == IDM_DOCLIST_FILESCLOSEOTHERS);

				fileCloseAllGiven(bufs2Close);

				if (id == IDM_DOCLIST_FILESCLOSEOTHERS)
				{
					// Get current buffer and its view
					_pDocumentListPanel->activateItem(_pEditView->getCurrentBufferID(), currentView());
				}
			}
			break;

		case IDM_DOCLIST_COPYNAMES:
		case IDM_DOCLIST_COPYPATHS:
			if (_pDocumentListPanel)
			{
				std::vector<Buffer*> buffers;
				auto files = _pDocumentListPanel->getSelectedFiles(false);
				for (auto&& sel : files)
					buffers.push_back(MainFileManager.getBufferByID(sel._bufID));
				buf2Clipboard(buffers, id == IDM_DOCLIST_COPYPATHS, _pDocumentListPanel->getHSelf());
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

		case IDM_FILE_CLOSEALL_BUT_PINNED :
			fileCloseAllButPinned();
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
			::PostMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
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
		{
			HWND focusedHwnd = ::GetFocus();
			if (focusedHwnd == _pEditView->getHSelf())
			{
				if (_pEditView->hasSelection()) // Cut normally
				{
					_pEditView->execute(WM_CUT);
				}
				else if (NppParameters::getInstance().getSVP()._lineCopyCutWithoutSelection) // Cut the entire line with EOL
				{
					_pEditView->execute(SCI_COPYALLOWLINE);
					_pEditView->execute(SCI_LINEDELETE);
				}
			}
			else
			{
				::SendMessage(focusedHwnd, WM_CUT, 0, 0);
			}
			break;
		}

		case IDM_EDIT_COPY:
		{
			HWND focusedHwnd = ::GetFocus();
			if (focusedHwnd == _pEditView->getHSelf())
			{
				if (_pEditView->hasSelection())
				{
					_pEditView->execute(WM_COPY);
				}
				else if (NppParameters::getInstance().getSVP()._lineCopyCutWithoutSelection)
				{
					_pEditView->execute(SCI_COPYALLOWLINE); // Copy without selected text, it will copy the whole line with EOL, for pasting before any line where the caret is.
				}

			}
			else
			{
				Finder* finder = _findReplaceDlg.getFinderFrom(focusedHwnd);
				if (finder)  // Search result
					finder->scintillaExecute(WM_COPY);
				else
					::SendMessage(focusedHwnd, WM_COPY, 0, 0);
			}

			break;
		}

		case IDM_EDIT_COPY_LINK:
		{
			size_t startPos = 0, endPos = 0, curPos = 0;
			if (_pEditView->getIndicatorRange(URL_INDIC, &startPos, &endPos, &curPos))
			{
				_pEditView->execute(SCI_SETSEL, startPos, endPos);
				_pEditView->execute(WM_COPY);
				checkClipboard();
				_pEditView->execute(SCI_SETSEL, curPos, curPos);
			}
			break;
		}

		case IDM_EDIT_COPY_BINARY:
		case IDM_EDIT_CUT_BINARY:
		{
			size_t textLen = _pEditView->execute(SCI_GETSELTEXT, 0, 0);
			if (!textLen)
				return;

			char *pBinText = new char[textLen + 1];
			_pEditView->getSelectedText(pBinText, textLen + 1);

			// Open the clipboard and empty it.
			if (!::OpenClipboard(NULL))
				return;
			if (!::EmptyClipboard())
			{
				::CloseClipboard();
				return;
			}

			// Allocate a global memory object for the text.
			HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, (textLen + 1) * sizeof(unsigned char));
			if (!hglbCopy)
			{
				::CloseClipboard();
				return;
			}

			// Lock the handle and copy the text to the buffer.
			unsigned char *lpucharCopy = (unsigned char *)::GlobalLock(hglbCopy);
			if (!lpucharCopy)
			{
				::GlobalFree(hglbCopy);
				::CloseClipboard();
				return;
			}
			memcpy(lpucharCopy, pBinText, textLen * sizeof(unsigned char));
			lpucharCopy[textLen] = 0;    // null character

			delete[] pBinText;

			::GlobalUnlock(hglbCopy);

			// Place the handle on the clipboard.
			if (!::SetClipboardData(CF_TEXT, hglbCopy))
			{
				::GlobalFree(hglbCopy);
				::CloseClipboard();
				return;
			}

			// Allocate a global memory object for the text length.
			HGLOBAL hglbLenCopy = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(unsigned long));
			if (!hglbLenCopy)
			{
				::CloseClipboard();
				return;
			}

			// Lock the handle and copy the text to the buffer.
			unsigned long *lpLenCopy = (unsigned long *)::GlobalLock(hglbLenCopy);
			if (!lpLenCopy)
			{
				::CloseClipboard();
				return;
			}
			*lpLenCopy = static_cast<unsigned long>(textLen);

			::GlobalUnlock(hglbLenCopy);

			// Place the handle on the clipboard.
			UINT cf_nppTextLen = ::RegisterClipboardFormat(CF_NPPTEXTLEN);
			::SetClipboardData(cf_nppTextLen, hglbLenCopy);

			::CloseClipboard();

			if (id == IDM_EDIT_CUT_BINARY)
				_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
		}
		break;

		case IDM_EDIT_PASTE:
		{
			std::lock_guard<std::mutex> lock(command_mutex);
			HWND focusedHwnd = ::GetFocus();
			if (focusedHwnd == _pEditView->getHSelf())
			{
				size_t nbSelections = _pEditView->execute(SCI_GETSELECTIONS);
				Buffer* buf = getCurrentBuffer();
				bool isRO = buf->isReadOnly();
				LRESULT selectionMode = _pEditView->execute(SCI_GETSELECTIONMODE);
				if (nbSelections > 1 && !isRO && selectionMode == SC_SEL_STREAM)
				{
					bool isPasteDone = _pEditView->pasteToMultiSelection();
					if (isPasteDone)
						return;
				}

				_pEditView->execute(SCI_PASTE);
			}
			else
			{
				::SendMessage(focusedHwnd, WM_PASTE, 0, 0);
			}
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
			wchar_t curentWord[CURRENTWORD_MAXLENGTH] = { '\0' };
			::SendMessage(hwnd, NPPM_GETFILENAMEATCURSOR, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(curentWord));
			
			wchar_t cmd2Exec[CURRENTWORD_MAXLENGTH] = { '\0' };
			if (id == IDM_EDIT_OPENINFOLDER)
			{
				wcscpy_s(cmd2Exec, L"explorer");
			}
			else
			{
				::SendMessage(hwnd, NPPM_GETNPPFULLFILEPATH, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(cmd2Exec));
			}

			// Full file path: could be a folder or a file
			if (doesPathExist(curentWord))
			{
				wstring fullFilePath = id == IDM_EDIT_OPENINFOLDER ? L"/select," : L"";
				fullFilePath += L"\"";
				fullFilePath += curentWord;
				fullFilePath += L"\"";

				if (id == IDM_EDIT_OPENINFOLDER ||
					(id == IDM_EDIT_OPENASFILE && !doesDirectoryExist(curentWord)))
					::ShellExecute(hwnd, L"open", cmd2Exec, fullFilePath.c_str(), L".", SW_SHOW);
			}
			else // Relative file path - need concatenate with current full file path
			{
				wchar_t currentDir[CURRENTWORD_MAXLENGTH] = { '\0' };
				::SendMessage(hwnd, NPPM_GETCURRENTDIRECTORY, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(currentDir));

				wstring fullFilePath = id == IDM_EDIT_OPENINFOLDER ? L"/select," : L"";
				fullFilePath += L"\"";
				fullFilePath += currentDir;
				fullFilePath += L"\\";
				fullFilePath += curentWord;

				if ((id == IDM_EDIT_OPENASFILE && 
					(!doesFileExist(fullFilePath.c_str() + 1)))) // + 1 for skipping the 1st char '"'
				{
					_nativeLangSpeaker.messageBox("FilePathNotFoundWarning",
						_pPublicInterface->getHSelf(),
						L"The file you're trying to open doesn't exist.",
						L"File Open",
						MB_OK | MB_APPLMODAL);
					return;
				}
				// else id == IDM_EDIT_OPENINFOLDER - do it anyway. (even the last part does not exist, it doesn't matter)

				fullFilePath += L"\"";
				::ShellExecute(hwnd, L"open", cmd2Exec, fullFilePath.c_str(), L".", SW_SHOW);
			}
		}
		break;

		case IDM_EDIT_SEARCHONINTERNET:
		{
			if (_pEditView->execute(SCI_GETSELECTIONS) != 1) // Multi-Selection || Column mode || no selection
				return;

			const NppGUI & nppGui = (NppParameters::getInstance()).getNppGUI();
			wstring url;
			if (nppGui._searchEngineChoice == nppGui.se_custom)
			{
				url = nppGui._searchEngineCustom;
				url.erase(std::remove_if(url.begin(), url.end(), [](_TUCHAR x) {return _istspace(x); }),
					url.end());

				auto httpPos = url.find(L"http://");
				auto httpsPos = url.find(L"https://");

				if (url.empty() || (httpPos != 0 && httpsPos != 0)) // if string is not a url (for launching only browser)
				{
					url = L"https://www.google.com/search?q=$(CURRENT_WORD)";
				}
			}
			else if (nppGui._searchEngineChoice == nppGui.se_duckDuckGo || nppGui._searchEngineChoice == nppGui.se_bing)
			{
				url = L"https://duckduckgo.com/?q=$(CURRENT_WORD)";
			}
			else if (nppGui._searchEngineChoice == nppGui.se_google)
			{
				url = L"https://www.google.com/search?q=$(CURRENT_WORD)";
			}
			else if (nppGui._searchEngineChoice == nppGui.se_yahoo)
			{
				url = L"https://search.yahoo.com/search?q=$(CURRENT_WORD)";
			}
			else if (nppGui._searchEngineChoice == nppGui.se_stackoverflow)
			{
				url = L"https://stackoverflow.com/search?q=$(CURRENT_WORD)";
			}

			Command cmd(url.c_str());
			cmd.run(_pPublicInterface->getHSelf());	
		}
		break;

		case IDM_EDIT_CHANGESEARCHENGINE:
		{
			command(IDM_SETTING_PREFERENCE);
			_preference.showDialogByName(L"SearchEngine");
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
		case IDM_EDIT_BEGINENDSELECT_COLUMNMODE:
		{
			_pEditView->beginOrEndSelect(id == IDM_EDIT_BEGINENDSELECT_COLUMNMODE);
			bool isStarted = _pEditView->beginEndSelectedIsStarted();
			::CheckMenuItem(_mainMenuHandle, id, MF_BYCOMMAND | (isStarted ? MF_CHECKED : MF_UNCHECKED));
			int otherId = (id == IDM_EDIT_BEGINENDSELECT) ? IDM_EDIT_BEGINENDSELECT_COLUMNMODE : IDM_EDIT_BEGINENDSELECT;
			::EnableMenuItem(_mainMenuHandle, otherId, MF_BYCOMMAND | (isStarted ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED));
		}
		break;

		case IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING:
		case IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING:
		case IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_ASCENDING:
		case IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_DESCENDING:
		case IDM_EDIT_SORTLINES_INTEGER_ASCENDING:
		case IDM_EDIT_SORTLINES_INTEGER_DESCENDING:
		case IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING:
		case IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING:
		case IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING:
		case IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING:
		case IDM_EDIT_SORTLINES_REVERSE_ORDER:
		case IDM_EDIT_SORTLINES_RANDOMLY:
		{
			std::lock_guard<std::mutex> lock(command_mutex);

			size_t fromLine = 0, toLine = 0;
			size_t fromColumn = 0, toColumn = 0;

			bool hasLineSelection = false;
			if (_pEditView->execute(SCI_GETSELECTIONS) > 1)
			{
				if (_pEditView->execute(SCI_SELECTIONISRECTANGLE) || _pEditView->execute(SCI_GETSELECTIONMODE) == SC_SEL_THIN)
				{
					size_t rectSelAnchor = _pEditView->execute(SCI_GETRECTANGULARSELECTIONANCHOR);
					size_t rectSelCaret = _pEditView->execute(SCI_GETRECTANGULARSELECTIONCARET);
					size_t anchorLine = _pEditView->execute(SCI_LINEFROMPOSITION, rectSelAnchor);
					size_t caretLine = _pEditView->execute(SCI_LINEFROMPOSITION, rectSelCaret);
					fromLine = std::min<size_t>(anchorLine, caretLine);
					toLine = std::max<size_t>(anchorLine, caretLine);
					size_t anchorLineOffset = rectSelAnchor - _pEditView->execute(SCI_POSITIONFROMLINE, anchorLine) + _pEditView->execute(SCI_GETRECTANGULARSELECTIONANCHORVIRTUALSPACE);
					size_t caretLineOffset = rectSelCaret - _pEditView->execute(SCI_POSITIONFROMLINE, caretLine) + _pEditView->execute(SCI_GETRECTANGULARSELECTIONCARETVIRTUALSPACE);
					fromColumn = std::min<size_t>(anchorLineOffset, caretLineOffset);
					toColumn = std::max<size_t>(anchorLineOffset, caretLineOffset);
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
					const pair<size_t, size_t> lineRange = _pEditView->getSelectionLinesRange();
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
								id == IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING ||
								id == IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_DESCENDING;

			_pEditView->execute(SCI_BEGINUNDOACTION);
			std::unique_ptr<ISorter> pSorter;
			if (id == IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING || id == IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING)
			{
				pSorter = std::unique_ptr<ISorter>(new LexicographicSorter(isDescending, fromColumn, toColumn));
			}
			else if (id == IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_DESCENDING || id == IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_ASCENDING)
			{
				pSorter = std::unique_ptr<ISorter>(new LexicographicCaseInsensitiveSorter(isDescending, fromColumn, toColumn));
			}
			else if (id == IDM_EDIT_SORTLINES_INTEGER_DESCENDING || id == IDM_EDIT_SORTLINES_INTEGER_ASCENDING)
			{
				pSorter = std::unique_ptr<ISorter>(new IntegerSorter(isDescending, fromColumn, toColumn));
			}
			else if (id == IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING || id == IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING)
			{
				pSorter = std::unique_ptr<ISorter>(new DecimalCommaSorter(isDescending, fromColumn, toColumn));
			}
			else if (id == IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING || id == IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING)
			{
				pSorter = std::unique_ptr<ISorter>(new DecimalDotSorter(isDescending, fromColumn, toColumn));
			}
			else if (id == IDM_EDIT_SORTLINES_REVERSE_ORDER)
			{
				pSorter = std::unique_ptr<ISorter>(new ReverseSorter(isDescending, fromColumn, toColumn));
			}
			else
			{
				pSorter = std::unique_ptr<ISorter>(new RandomSorter(isDescending, fromColumn, toColumn));
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
					L"Unable to perform numeric sorting due to line $INT_REPLACE$.",
					L"Sorting Error",
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
			if (_pAnsiCharPanel && (!_pAnsiCharPanel->isClosed()))
			{
				_pAnsiCharPanel->display(false);
				_pAnsiCharPanel->setClosed(true);
				checkMenuItem(IDM_EDIT_CHAR_PANEL, false);
			}
			else
			{
				checkMenuItem(IDM_EDIT_CHAR_PANEL, true);
				launchAnsiCharPanel();
				_pAnsiCharPanel->setClosed(false);
			}
		}
		break;

		case IDM_EDIT_CLIPBOARDHISTORY_PANEL:
		{
			if (_pClipboardHistoryPanel && (!_pClipboardHistoryPanel->isClosed()))
			{
				_pClipboardHistoryPanel->display(false);
				_pClipboardHistoryPanel->setClosed(true);
				checkMenuItem(IDM_EDIT_CLIPBOARDHISTORY_PANEL, false);
			}
			else
			{
				checkMenuItem(IDM_EDIT_CLIPBOARDHISTORY_PANEL, true);
				launchClipboardHistoryPanel();
				_pClipboardHistoryPanel->setClosed(false);
			}
		}
		break;

		case IDM_VIEW_SWITCHTO_DOCLIST:
		{
			if (_pDocumentListPanel && _pDocumentListPanel->isVisible())
			{
				_pDocumentListPanel->grabFocus();
			}
			else
			{
				checkMenuItem(IDM_VIEW_DOCLIST, true);
				_toolBar.setCheck(IDM_VIEW_DOCLIST, true);
				launchDocumentListPanel();
				_pDocumentListPanel->setClosed(false);
			}
		}
		break;

		case IDM_VIEW_DOCLIST:
		{
			if (_pDocumentListPanel && (!_pDocumentListPanel->isClosed()))
			{
				_pDocumentListPanel->display(false);
				_pDocumentListPanel->setClosed(true);
				checkMenuItem(IDM_VIEW_DOCLIST, false);
				_toolBar.setCheck(IDM_VIEW_DOCLIST, false);
			}
			else
			{
				launchDocumentListPanel();
				if (_pDocumentListPanel)
				{
					checkMenuItem(IDM_VIEW_DOCLIST, true);
					_toolBar.setCheck(IDM_VIEW_DOCLIST, true);
					_pDocumentListPanel->setClosed(false);
				}
			}
		}
		break;

		case IDM_VIEW_PROJECT_PANEL_1:
		case IDM_VIEW_PROJECT_PANEL_2:
		case IDM_VIEW_PROJECT_PANEL_3:
		{
			ProjectPanel** pp [] = {&_pProjectPanel_1, &_pProjectPanel_2, &_pProjectPanel_3};
			int idx = id - IDM_VIEW_PROJECT_PANEL_1;
			if (*pp [idx] == nullptr)
			{
				launchProjectPanel(id, pp [idx], idx);
			}
			else
			{
				if (!(*pp[idx])->isClosed())
				{
					if ((*pp[idx])->checkIfNeedSave())
					{
						if (::IsChild((*pp[idx])->getHSelf(), ::GetFocus()))
							::SetFocus(_pEditView->getHSelf());
						(*pp[idx])->display(false);
						(*pp[idx])->setClosed(true);
						checkMenuItem(id, false);
						checkProjectMenuItem();
					}
				}
				else
				{
					launchProjectPanel(id, pp [idx], idx);
				}
			}
		}
		break;

		case IDM_VIEW_SWITCHTO_PROJECT_PANEL_1:
		case IDM_VIEW_SWITCHTO_PROJECT_PANEL_2:
		case IDM_VIEW_SWITCHTO_PROJECT_PANEL_3:
		{
			ProjectPanel** pp [] = {&_pProjectPanel_1, &_pProjectPanel_2, &_pProjectPanel_3};
			int idx = id - IDM_VIEW_SWITCHTO_PROJECT_PANEL_1;
			launchProjectPanel(id - IDM_VIEW_SWITCHTO_PROJECT_PANEL_1 + IDM_VIEW_PROJECT_PANEL_1, pp [idx], idx);
		}
		break;


		case IDM_VIEW_FILEBROWSER:
		case IDM_VIEW_SWITCHTO_FILEBROWSER:
		{
			if (_pFileBrowser == nullptr) // first launch, check in params to open folders
			{
				const NppParameters& nppParam = NppParameters::getInstance();
				launchFileBrowser(nppParam.getFileBrowserRoots(), nppParam.getFileBrowserSelectedItemPath());
				if (_pFileBrowser != nullptr)
				{
					checkMenuItem(IDM_VIEW_FILEBROWSER, true);
					_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
					_pFileBrowser->setClosed(false);
				}
			}
			else
			{
				if (!_pFileBrowser->isClosed() && (id != IDM_VIEW_SWITCHTO_FILEBROWSER))
				{
					_pFileBrowser->display(false);
					_pFileBrowser->setClosed(true);
					checkMenuItem(IDM_VIEW_FILEBROWSER, false);
					_toolBar.setCheck(IDM_VIEW_FILEBROWSER, false);
				}
				else
				{
					vector<wstring> dummy;
					wstring emptyStr;
					launchFileBrowser(dummy, emptyStr);
					checkMenuItem(IDM_VIEW_FILEBROWSER, true);
					_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
					_pFileBrowser->setClosed(false);
				}
			}
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

		case IDM_VIEW_SWITCHTO_FUNC_LIST:
		{
			if (_pFuncList && _pFuncList->isVisible())
			{
				_pFuncList->grabFocus();
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

		case IDM_VIEW_TAB_COLOUR_NONE:
		case IDM_VIEW_TAB_COLOUR_1:
		case IDM_VIEW_TAB_COLOUR_2:
		case IDM_VIEW_TAB_COLOUR_3:
		case IDM_VIEW_TAB_COLOUR_4:
		case IDM_VIEW_TAB_COLOUR_5:
		{
			const int color_id = (id - IDM_VIEW_TAB_COLOUR_NONE) - 1;
			const auto current_index = _pDocTab->getCurrentTabIndex();
			BufferID buffer_id = _pDocTab->getBufferByIndex(current_index);
			_pDocTab->setIndividualTabColour(buffer_id, color_id);
			_pDocTab->redraw();

			if (_pDocumentListPanel != nullptr)
			{
				_pDocumentListPanel->setItemColor(buffer_id);
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

		case IDM_VIEW_TAB_START:
		case IDM_VIEW_TAB_END:
		{
			size_t index = id == IDM_VIEW_TAB_START ? 0 : _pDocTab->nbItem() - 1;
			switchToFile(_pDocTab->getBufferByIndex(index));
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

			TCITEM tciMove{}, tciShift{};
			tciMove.mask = tciShift.mask = TCIF_IMAGE | TCIF_TEXT | TCIF_PARAM;

			const int strSizeMax = 256;
			wchar_t strMove[strSizeMax] = { '\0' };
			wchar_t strShift[strSizeMax] = { '\0' };

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
				wstring dir(buf->getFullPathName());
				pathRemoveFileSpec(dir);
				str2Cliboard(dir);
			}
			else if (id == IDM_EDIT_FILENAMETOCLIP)
			{
				str2Cliboard(buf->getFileName());
			}
		}
		break;

		case IDM_EDIT_COPY_ALL_NAMES:
		case IDM_EDIT_COPY_ALL_PATHS:
			{
				std::vector<DocTabView*> docTabs;
				if (viewVisible(MAIN_VIEW))
					docTabs.push_back(&_mainDocTab);
				if (viewVisible(SUB_VIEW))
					docTabs.push_back(&_subDocTab);
				std::vector<Buffer*> buffers;
				for (auto&& docTab : docTabs)
				{
					for (size_t i = 0, len = docTab->nbItem(); i < len; ++i)
					{
						BufferID bufID = docTab->getBufferByIndex(i);
						Buffer* buf = MainFileManager.getBufferByID(bufID);
						// Don't add duplicates because a buffer might be cloned in other view.
						if (docTabs.size() < 2 || std::find(buffers.begin(), buffers.end(), buf) == buffers.end())
							buffers.push_back(buf);
					}
				}
				buf2Clipboard({ buffers.begin(), buffers.end() }, id == IDM_EDIT_COPY_ALL_PATHS, _pPublicInterface->getHSelf());
			}
			break;

		case IDM_SEARCH_FIND :
		case IDM_SEARCH_REPLACE :
		case IDM_SEARCH_MARK :
		{
			const int strSize = FINDREPLACE_MAXLENGTH;
			wchar_t str[strSize] = { '\0' };

			const NppGUI& nppGui = (NppParameters::getInstance()).getNppGUI();
			if (nppGui._fillFindFieldWithSelected)
			{
				_pEditView->getGenericSelectedText(str, strSize, nppGui._fillFindFieldSelectCaret);
			}

			bool isFirstTime = !_findReplaceDlg.isCreated();

			DIALOG_TYPE dlgID = FIND_DLG;
			if (id == IDM_SEARCH_REPLACE)
				dlgID = REPLACE_DLG;
			else if (id == IDM_SEARCH_MARK)
				dlgID = MARK_DLG;
			_findReplaceDlg.doDialog(dlgID, _nativeLangSpeaker.isRTL());

			if (nppGui._fillFindFieldWithSelected)
			{
				if (lstrlen(str) <= FINDREPLACE_INSELECTION_THRESHOLD_DEFAULT)
				{
					_findReplaceDlg.setSearchText(str);
				}
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
			wchar_t str[strSize] = { '\0' };

			static bool isFirstTime = true;
			if (isFirstTime)
			{
				_nativeLangSpeaker.changeDlgLang(_incrementFindDlg.getHSelf(), "IncrementalFind");
				isFirstTime = false;
			}

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
				const NppParameters& nppParams = NppParameters::getInstance();
				if ((id == IDM_SEARCH_FINDPREV) && (op._searchType == FindRegex) && !nppParams.regexBackward4PowerUser())
				{
					// regex upward search is disabled
					// make this a no-action command

					_findReplaceDlg.regexBackwardMsgBox();
				}
				else
				{
					op._whichDirection = (id == IDM_SEARCH_FINDNEXT ? DIR_DOWN : DIR_UP);
					wstring s = _findReplaceDlg.getText2search();
					FindStatus status = FSNoMessage;
					_findReplaceDlg.processFindNext(s.c_str(), &op, &status);
					if (status == FSEndReached)
					{
						wstring msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-end-reached", FIND_STATUS_END_REACHED_TEXT);
						_findReplaceDlg.setStatusbarMessage(msg, FSEndReached);
					}
					else if (status == FSTopReached)
					{
						wstring msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-top-reached", FIND_STATUS_TOP_REACHED_TEXT);
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
			wchar_t str[strSize] = { '\0' };
			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str);
			_findReplaceDlg._env->_str2Search = str;
			setFindReplaceFolderFilter(NULL, NULL);
			if (isFirstTime)
				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);

			FindOption op = _findReplaceDlg.getCurrentOptions();
			op._searchType = FindNormal;
			op._whichDirection = (id == IDM_SEARCH_SETANDFINDNEXT?DIR_DOWN:DIR_UP);

			FindStatus status = FSNoMessage;
			_findReplaceDlg.processFindNext(str, &op, &status);
			if (status == FSEndReached)
			{
				wstring msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-end-reached", FIND_STATUS_END_REACHED_TEXT);
				_findReplaceDlg.setStatusbarMessage(msg, FSEndReached);
			}
			else if (status == FSTopReached)
			{
				wstring msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-top-reached", FIND_STATUS_TOP_REACHED_TEXT);
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
			const int strSize = FINDREPLACE_MAXLENGTH;
			wchar_t str[strSize] = { '\0' };
			_pEditView->getGenericSelectedText(str, strSize);

			FindOption op;
			op._isMatchCase = false;
			op._isWholeWord = false;
			op._isWrapAround = true;
			op._searchType = FindNormal;
			op._whichDirection = (id == IDM_SEARCH_VOLATILE_FINDNEXT ? DIR_DOWN : DIR_UP);

			FindStatus status = FSNoMessage;
			_findReplaceDlg.processFindNext(str, &op, &status);
			if (status == FSEndReached)
			{
				wstring msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-end-reached", FIND_STATUS_END_REACHED_TEXT);
				_findReplaceDlg.setStatusbarMessage(msg, FSEndReached);
			}
			else if (status == FSTopReached)
			{
				wstring msg = _nativeLangSpeaker.getLocalizedStrFromID("find-status-top-reached", FIND_STATUS_TOP_REACHED_TEXT);
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
			wchar_t selectedText[strSize] = { '\0' };
			wchar_t wordOnCaret[strSize] = { '\0' };

			_pEditView->getGenericSelectedText(selectedText, strSize, false);
			_pEditView->getGenericWordOnCaretPos(wordOnCaret, strSize);

			if (selectedText[0] == '\0')
			{
				if (lstrlen(wordOnCaret) > 0)
				{
					_findReplaceDlg.markAll(wordOnCaret, styleID);
				}
			}
			else
			{
				_findReplaceDlg.markAll(selectedText, styleID);
			}
		}
		break;

		case IDM_SEARCH_MARKONEEXT1:
		case IDM_SEARCH_MARKONEEXT2:
		case IDM_SEARCH_MARKONEEXT3:
		case IDM_SEARCH_MARKONEEXT4:
		case IDM_SEARCH_MARKONEEXT5:
		{
			int styleID;
			if (id == IDM_SEARCH_MARKONEEXT1)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT1;
			else if (id == IDM_SEARCH_MARKONEEXT2)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT2;
			else if (id == IDM_SEARCH_MARKONEEXT3)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT3;
			else if (id == IDM_SEARCH_MARKONEEXT4)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT4;
			else // (id == IDM_SEARCH_MARKONEEXT5)
				styleID = SCE_UNIVERSAL_FOUND_STYLE_EXT5;

			Sci_CharacterRangeFull range = _pEditView->getSelection();
			if (range.cpMin == range.cpMax)
			{
				auto caretPos = _pEditView->execute(SCI_GETCURRENTPOS, 0, 0);
				range.cpMin = _pEditView->execute(SCI_WORDSTARTPOSITION, caretPos, true);
				range.cpMax = _pEditView->execute(SCI_WORDENDPOSITION, caretPos, true);
			}
			if (range.cpMax > range.cpMin)
			{
				_pEditView->execute(SCI_SETINDICATORCURRENT, styleID);
				_pEditView->execute(SCI_INDICATORFILLRANGE, range.cpMin, range.cpMax - range.cpMin);
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

		case IDM_SEARCH_STYLE1TOCLIP:
		{
			_pEditView->markedTextToClipboard(SCE_UNIVERSAL_FOUND_STYLE_EXT1);
		}
		break;
		case IDM_SEARCH_STYLE2TOCLIP:
		{
			_pEditView->markedTextToClipboard(SCE_UNIVERSAL_FOUND_STYLE_EXT2);
		}
		break;
		case IDM_SEARCH_STYLE3TOCLIP:
		{
			_pEditView->markedTextToClipboard(SCE_UNIVERSAL_FOUND_STYLE_EXT3);
		}
		break;
		case IDM_SEARCH_STYLE4TOCLIP:
		{
			_pEditView->markedTextToClipboard(SCE_UNIVERSAL_FOUND_STYLE_EXT4);
		}
		break;
		case IDM_SEARCH_STYLE5TOCLIP:
		{
			_pEditView->markedTextToClipboard(SCE_UNIVERSAL_FOUND_STYLE_EXT5);
		}
		break;
		case IDM_SEARCH_ALLSTYLESTOCLIP:
		{
			_pEditView->markedTextToClipboard(-1, true);
		}
		break;
		case IDM_SEARCH_MARKEDTOCLIP:
		{
			_pEditView->markedTextToClipboard(SCE_UNIVERSAL_FOUND_STYLE);
		}
		break;

		case IDM_SEARCH_GOTOLINE:
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
					L"There are 3 ways to switch to column-select mode:\r\n\r\n"
					L"1. (Keyboard and Mouse)  Hold Alt while left-click dragging\r\n\r\n"
					L"2. (Keyboard only)  Hold Alt+Shift while using arrow keys\r\n\r\n"
					L"3. (Keyboard or Mouse)\r\n"
					L"      Put caret at desired start of column block position, then\r\n"
					L"       execute \"Begin/End Select in Column Mode\" command;\r\n"
					L"      Move caret to desired end of column block position, then\r\n"
					L"       execute \"Begin/End Select in Column Mode\" command again\r\n",
					L"Column Mode Tip",
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
			intptr_t braceAtCaret = -1;
			intptr_t braceOpposite = -1;
			findMatchingBracePos(braceAtCaret, braceOpposite);

			if (braceOpposite != -1)
			{
				if (id == IDM_SEARCH_GOTOMATCHINGBRACE)
					_pEditView->execute(SCI_GOTOPOS, braceOpposite);
				else
					_pEditView->execute(SCI_SETSEL, std::min<intptr_t>(braceAtCaret, braceOpposite), std::max<intptr_t>(braceAtCaret, braceOpposite) + 1); // + 1 so we always include the ending brace in the selection.

				// Update Scintilla's knowledge about what column the caret is in, so that if user
				// does up/down arrow as first navigation after the brace-match operation,
				// the caret doesn't jump to an unexpected column
				_pEditView->execute(SCI_CHOOSECARETX);
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

	    case IDM_SEARCH_CHANGED_PREV:
	    case IDM_SEARCH_CHANGED_NEXT:
			changedHistoryGoTo(id);
		    break;
			
	    case IDM_SEARCH_CLEAR_CHANGE_HISTORY:
			clearChangesHistory();
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
					_mainWindowStatus &= static_cast<UCHAR>(~WindowUserActive);
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
						int splitterSizeDyn = NppParameters::getInstance()._dpiManager.scaleX(splitterSize);
                        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), splitterSizeDyn, SplitterMode::RIGHT_FIX, 45);
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
		case IDM_EDIT_RMV_TAB:
		{
			bool forwards = id == IDM_EDIT_INS_TAB;
			size_t selStartPos = _pEditView->execute(SCI_GETSELECTIONSTART);
			size_t lineNumber = _pEditView->execute(SCI_LINEFROMPOSITION, selStartPos);
			size_t nbSelections = _pEditView->execute(SCI_GETSELECTIONS);
			size_t selEndPos = _pEditView->execute(SCI_GETSELECTIONEND);
			size_t selEndLineNumber = _pEditView->execute(SCI_LINEFROMPOSITION, selEndPos);
			if ((nbSelections > 1) || (lineNumber != selEndLineNumber))
			{
				// multiple-selection or multi-line selection; use Scintilla SCI_TAB / SCI_BACKTAB behavior
				_pEditView->execute(forwards ? SCI_TAB : SCI_BACKTAB);
			}
			else
			{
				// zero-length selection (simple single caret) or selected text is all on single line
				// depart from Scintilla behavior and do it our way
				size_t currentIndent = _pEditView->execute(SCI_GETLINEINDENTATION, lineNumber);
				intptr_t indentDelta = _pEditView->execute(SCI_GETTABWIDTH);
				if (!forwards) indentDelta = -indentDelta;
				_pEditView->setLineIndent(lineNumber, static_cast<intptr_t>(currentIndent) + indentDelta);
			}
		}
		break;

		case IDM_EDIT_DUP_LINE:
			_pEditView->execute(SCI_LINEDUPLICATE);
			break;

		case IDM_EDIT_REMOVE_CONSECUTIVE_DUP_LINES:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			removeDuplicateLines();
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_REMOVE_ANY_DUP_LINES:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			_pEditView->removeAnyDuplicateLines();
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_SPLIT_LINES:
		{
			if (_pEditView->execute(SCI_GETSELECTIONS) == 1)
			{
				pair<size_t, size_t> lineRange = _pEditView->getSelectionLinesRange();
				auto anchorPos = _pEditView->execute(SCI_POSITIONFROMLINE, lineRange.first);
				auto caretPos = _pEditView->execute(SCI_GETLINEENDPOSITION, lineRange.second);
				_pEditView->execute(SCI_SETSELECTION, caretPos, anchorPos);
				_pEditView->execute(SCI_TARGETFROMSELECTION);
				size_t edgeMode = _pEditView->execute(SCI_GETEDGEMODE);
				if (edgeMode == EDGE_NONE)
				{
					_pEditView->execute(SCI_LINESSPLIT, 0);
				}
				else
				{
					auto textWidth = _pEditView->execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P"));
					auto edgeCol = _pEditView->execute(SCI_GETEDGECOLUMN); // will work for edgeMode == EDGE_BACKGROUND
					if (edgeMode == EDGE_MULTILINE)
					{
						NppParameters& nppParam = NppParameters::getInstance();
						ScintillaViewParams& svp = const_cast<ScintillaViewParams&>(nppParam.getSVP());
						edgeCol = svp._edgeMultiColumnPos.back();  // the LAST edge column specified by the user
					}
					++edgeCol;  // compensate for zero-based column number
					_pEditView->execute(SCI_LINESSPLIT, textWidth * edgeCol);
				}
			}
		}
		break;

		case IDM_EDIT_JOIN_LINES:
		{
			const pair<size_t, size_t> lineRange = _pEditView->getSelectionLinesRange();
			if (lineRange.first != lineRange.second)
			{
				auto anchorPos = _pEditView->execute(SCI_POSITIONFROMLINE, lineRange.first);
				auto caretPos = _pEditView->execute(SCI_GETLINEENDPOSITION, lineRange.second);
				_pEditView->execute(SCI_SETSELECTION, caretPos, anchorPos);
				_pEditView->execute(SCI_TARGETFROMSELECTION);
				_pEditView->execute(SCI_LINESJOIN);
			}
		}
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
			_pEditView->convertSelectedTextToNewerCase(PROPERCASE_FORCE);
			break;

		case IDM_EDIT_PROPERCASE_BLEND:
			_pEditView->convertSelectedTextToNewerCase(PROPERCASE_BLEND);
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
			doTrim(lineBoth);
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;
		}

		case IDM_EDIT_EOL2WS:
			_pEditView->execute(SCI_BEGINUNDOACTION);
			eol2ws();
			_pEditView->execute(SCI_ENDUNDOACTION);
			break;

		case IDM_EDIT_TRIMALL:
		{
			std::lock_guard<std::mutex> lock(command_mutex);

			_pEditView->execute(SCI_BEGINUNDOACTION);
			bool isEntireDoc = _pEditView->execute(SCI_GETANCHOR) == _pEditView->execute(SCI_GETCURRENTPOS);
			doTrim(lineBoth);
			if (isEntireDoc || _pEditView->execute(SCI_GETANCHOR) != _pEditView->execute(SCI_GETCURRENTPOS))
				eol2ws();
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
			removeReadOnlyFlagFromFileAttributes(buf->getFullPathName());
			buf->setFileReadOnly(false);
		}
		break;

		case IDM_EDIT_MULTISELECTALL:
		case IDM_EDIT_MULTISELECTALLMATCHCASE:
		case IDM_EDIT_MULTISELECTALLWHOLEWORD:
		case IDM_EDIT_MULTISELECTALLMATCHCASEWHOLEWORD:
		{
			_multiSelectFlag = id == IDM_EDIT_MULTISELECTALL ? 0 :
				(id == IDM_EDIT_MULTISELECTALLMATCHCASE ? SCFIND_MATCHCASE :
					(id == IDM_EDIT_MULTISELECTALLWHOLEWORD ? SCFIND_WHOLEWORD: SCFIND_MATCHCASE| SCFIND_WHOLEWORD));

			// Don't use _pEditView->hasSelection() because when multi-selection is active and main selection has no selection,
			// it will cause an infinite loop on SCI_MULTIPLESELECTADDEACH. See:
			// https://github.com/notepad-plus-plus/notepad-plus-plus/pull/14330#issuecomment-1797080251
			bool hasSelection = (_pEditView->execute(SCI_GETSELECTIONSTART) != _pEditView->execute(SCI_GETSELECTIONEND));
			if (!hasSelection)
				_pEditView->expandWordSelection();

			_pEditView->execute(SCI_TARGETWHOLEDOCUMENT);

			// Firstly do a selection of the word on which the cursor is
			_pEditView->execute(SCI_SETSEARCHFLAGS, _multiSelectFlag);
			_pEditView->execute(SCI_MULTIPLESELECTADDEACH);
		}
		break;

		case IDM_EDIT_MULTISELECTNEXT:
		case IDM_EDIT_MULTISELECTNEXTMATCHCASE:
		case IDM_EDIT_MULTISELECTNEXTWHOLEWORD:
		case IDM_EDIT_MULTISELECTNEXTMATCHCASEWHOLEWORD:
		{
			_multiSelectFlag = id == IDM_EDIT_MULTISELECTNEXT ? 0 :
				(id == IDM_EDIT_MULTISELECTNEXTMATCHCASE ? SCFIND_MATCHCASE :
					(id == IDM_EDIT_MULTISELECTNEXTWHOLEWORD ? SCFIND_WHOLEWORD : SCFIND_MATCHCASE | SCFIND_WHOLEWORD));

			_pEditView->execute(SCI_TARGETWHOLEDOCUMENT);
			_pEditView->execute(SCI_SETSEARCHFLAGS, _multiSelectFlag);
			_pEditView->execute(SCI_MULTIPLESELECTADDNEXT);
		}
		break;

		case IDM_EDIT_MULTISELECTUNDO:
		{
			LRESULT n = _pEditView->execute(SCI_GETSELECTIONS);
			if (n > 0)
				_pEditView->execute(SCI_DROPSELECTIONN, n - 1);
		}
		break;

		case IDM_EDIT_MULTISELECTSSKIP:
		{
			_pEditView->execute(SCI_TARGETWHOLEDOCUMENT);
			_pEditView->execute(SCI_SETSEARCHFLAGS, _multiSelectFlag); // use the last used flag to select the next one
			_pEditView->execute(SCI_MULTIPLESELECTADDNEXT);

			LRESULT n = _pEditView->execute(SCI_GETSELECTIONS);
			if (n > 1)
				_pEditView->execute(SCI_DROPSELECTIONN, n - 2);
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

	    case IDM_VIEW_ALWAYSONTOP:
		{
			int check = (::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED)?MF_UNCHECKED:MF_CHECKED;
			::CheckMenuItem(_mainMenuHandle, id, MF_BYCOMMAND | check);
			SetWindowPos(_pPublicInterface->getHSelf(), check == MF_CHECKED?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		}
		break;


		case IDM_VIEW_FOLD_CURRENT:
		case IDM_VIEW_UNFOLD_CURRENT:
		{
			bool isToggleEnabled = NppParameters::getInstance().getNppGUI()._enableFoldCmdToggable;
			bool mode = id == IDM_VIEW_FOLD_CURRENT ? fold_collapse : fold_uncollapse;

			if (isToggleEnabled)
			{
				bool isFolded = _pEditView->isCurrentLineFolded();
				mode = isFolded ? fold_uncollapse : fold_collapse;
			}

			_pEditView->foldCurrentPos(mode);
		}
		break;

		case IDM_VIEW_FOLDALL:
		case IDM_VIEW_UNFOLDALL:
		{
			_isFolding = true; // So we can ignore events while folding is taking place
			bool doCollapse = (id==IDM_VIEW_FOLDALL)?fold_collapse:fold_uncollapse;
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

		case IDM_VIEW_FULLSCREENTOGGLE:
		{
			if (!_beforeSpecialView._isDistractionFree)
				fullScreenToggle();
		}
		break;

		case IDM_VIEW_POSTIT :
		{
			if (!_beforeSpecialView._isDistractionFree)
				postItToggle();
		}
		break;

		case IDM_VIEW_DISTRACTIONFREE:
		{
			if ((_beforeSpecialView._isDistractionFree && _beforeSpecialView._isFullScreen && _beforeSpecialView._isPostIt) ||
				(!_beforeSpecialView._isDistractionFree && !_beforeSpecialView._isFullScreen && !_beforeSpecialView._isPostIt))
				distractionFreeToggle();
		}
		break;

		case IDM_VIEW_IN_FIREFOX:
		case IDM_VIEW_IN_CHROME:
		case IDM_VIEW_IN_EDGE:
		case IDM_VIEW_IN_IE:
		{
			auto currentBuf = _pEditView->getCurrentBuffer();
			if (!currentBuf->isUntitled())
			{
				wstring appName;

				if (id == IDM_VIEW_IN_FIREFOX)
				{
					appName = L"firefox.exe";
				}
				else if (id == IDM_VIEW_IN_CHROME)
				{
					appName = L"chrome.exe";
				}
				else if (id == IDM_VIEW_IN_EDGE)
				{
					appName = L"msedge.exe";
				}
				else // if (id == IDM_VIEW_IN_IE)
				{
					appName = L"IEXPLORE.EXE";
				}

				wchar_t valData[MAX_PATH] = {'\0'};
				DWORD valDataLen = MAX_PATH * sizeof(wchar_t);
				DWORD valType = 0;
				HKEY hKey2Check = nullptr;
				wstring appEntry = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\";
				appEntry += appName;
				::RegOpenKeyEx(HKEY_LOCAL_MACHINE, appEntry.c_str(), 0, KEY_READ, &hKey2Check);
				::RegQueryValueEx(hKey2Check, L"", nullptr, &valType, reinterpret_cast<LPBYTE>(valData), &valDataLen);


				wstring fullCurrentPath = L"\"";
				fullCurrentPath += currentBuf->getFullPathName();
				fullCurrentPath += L"\"";

				if (hKey2Check && valData[0] != '\0')
				{
					::ShellExecute(NULL, L"open", valData, fullCurrentPath.c_str(), NULL, SW_SHOWNORMAL);
				}
				else if (id == IDM_VIEW_IN_EDGE)
				{
					// Try the Legacy version

					// Don't put the quots for Edge, otherwise it doesn't work
					//fullCurrentPath = L"\"";
					fullCurrentPath = currentBuf->getFullPathName();
					//fullCurrentPath += L"\"";

					::ShellExecute(NULL, L"open", L"shell:Appsfolder\\Microsoft.MicrosoftEdge_8wekyb3d8bbwe!MicrosoftEdge", fullCurrentPath.c_str(), NULL, SW_SHOW);
				} 
				else 
				{
					_nativeLangSpeaker.messageBox("ViewInBrowser",
						_pPublicInterface->getHSelf(),
						L"Application cannot be found in your system.",
						L"View Current File in Browser",
						MB_OK);
				}
				::RegCloseKey(hKey2Check);
			}
		}
		break;

		case IDM_VIEW_TAB_SPACE:
		{
			const bool isChecked = !(::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED);
			checkMenuItem(id, isChecked);

			_mainEditView.showWSAndTab(isChecked);
			_subEditView.showWSAndTab(isChecked);

			auto& svp1 = const_cast<ScintillaViewParams&>(NppParameters::getInstance().getSVP());
			svp1._whiteSpaceShow = isChecked;

			const bool allChecked = svp1._whiteSpaceShow && svp1._eolShow && svp1._npcShow && svp1._ccUniEolShow;

			checkMenuItem(IDM_VIEW_ALL_CHARACTERS, allChecked);
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, allChecked);

			break;
		}

		case IDM_VIEW_EOL:
		{
			const bool isChecked = !(::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED);
			checkMenuItem(id, isChecked);

			_mainEditView.showEOL(isChecked);
			_subEditView.showEOL(isChecked);

			auto& svp1 = const_cast<ScintillaViewParams&>(NppParameters::getInstance().getSVP());
			svp1._eolShow = isChecked;

			const bool allChecked = svp1._whiteSpaceShow && svp1._eolShow && svp1._npcShow && svp1._ccUniEolShow;

			checkMenuItem(IDM_VIEW_ALL_CHARACTERS, allChecked);
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, allChecked);

			break;
		}

		case IDM_VIEW_NPC:
		{
			const bool isChecked = !(::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED);
			checkMenuItem(id, isChecked);

			auto& svp1 = const_cast<ScintillaViewParams&>(NppParameters::getInstance().getSVP());
			svp1._npcShow = isChecked;

			// setNpcAndCcUniEOL() in showNpc() uses svp1._npcShow
			_mainEditView.showNpc(isChecked);
			_subEditView.showNpc(isChecked);

			const bool allChecked = svp1._whiteSpaceShow && svp1._eolShow && svp1._npcShow && svp1._ccUniEolShow;

			checkMenuItem(IDM_VIEW_ALL_CHARACTERS, allChecked);
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, allChecked);

			_findReplaceDlg.updateFinderScintillaForNpc();

			break;
		}

		case IDM_VIEW_NPC_CCUNIEOL:
		{
			const bool isChecked = !(::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED);
			checkMenuItem(id, isChecked);

			auto& svp1 = const_cast<ScintillaViewParams&>(NppParameters::getInstance().getSVP());
			svp1._ccUniEolShow = isChecked;

			// setNpcAndCcUniEOL() in showCcUniEol() uses svp1._ccUniEolShow
			_mainEditView.showCcUniEol(isChecked);
			_subEditView.showCcUniEol(isChecked);

			const bool allChecked = svp1._whiteSpaceShow && svp1._eolShow && svp1._npcShow && svp1._ccUniEolShow;

			checkMenuItem(IDM_VIEW_ALL_CHARACTERS, allChecked);
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, allChecked);

			break;
		}

		case IDM_VIEW_ALL_CHARACTERS:
		{
			const bool isChecked = !(::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED);
			checkMenuItem(id, isChecked);
			checkMenuItem(IDM_VIEW_TAB_SPACE, isChecked);
			checkMenuItem(IDM_VIEW_EOL, isChecked);
			checkMenuItem(IDM_VIEW_NPC, isChecked);
			checkMenuItem(IDM_VIEW_NPC_CCUNIEOL, isChecked);
			_toolBar.setCheck(id, isChecked);

			auto& svp1 = const_cast<ScintillaViewParams&>(NppParameters::getInstance().getSVP());

			svp1._whiteSpaceShow = isChecked;
			svp1._eolShow = isChecked;
			svp1._npcShow = isChecked;
			svp1._ccUniEolShow = isChecked;

			_mainEditView.showInvisibleChars(isChecked);
			_subEditView.showInvisibleChars(isChecked);

			_findReplaceDlg.updateFinderScintillaForNpc();

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
				intptr_t mainCurrentLine = _mainEditView.execute(SCI_GETFIRSTVISIBLELINE);
				intptr_t subCurrentLine = _subEditView.execute(SCI_GETFIRSTVISIBLELINE);
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
				intptr_t mxoffset = _mainEditView.execute(SCI_GETXOFFSET);
				intptr_t pixel = _mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P"));
				intptr_t mainColumn = mxoffset/pixel;

				intptr_t sxoffset = _subEditView.execute(SCI_GETXOFFSET);
				pixel = _subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P"));
				intptr_t subColumn = sxoffset/pixel;
				_syncInfo._column = mainColumn - subColumn;
			}
		}
		break;

		case IDM_VIEW_SUMMARY:
		{
			Buffer * curBuf = _pEditView->getCurrentBuffer();
			int64_t fileLen = curBuf->getFileLength();

			// localization for summary date
			NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
			if (pNativeSpeaker)
			{
				wstring characterNumber = L"";

				if (fileLen != -1)
				{
					wstring filePathLabel = pNativeSpeaker->getLocalizedStrFromID("summary-filepath", L"Full file path: ");
					wstring fileCreateTimeLabel = pNativeSpeaker->getLocalizedStrFromID("summary-filecreatetime", L"Created: ");
					wstring fileModifyTimeLabel = pNativeSpeaker->getLocalizedStrFromID("summary-filemodifytime", L"Modified: ");

					characterNumber += filePathLabel;
					characterNumber += curBuf->getFullPathName();
					characterNumber += L"\r";

					characterNumber += fileCreateTimeLabel;
					characterNumber += curBuf->getFileTime(Buffer::ft_created);
					characterNumber += L"\r";

					characterNumber += fileModifyTimeLabel;
					characterNumber += curBuf->getFileTime(Buffer::ft_modified);
					characterNumber += L"\r";
				}
				wstring nbCharLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbchar", L"Characters (without line endings): ");
				wstring nbWordLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbword", L"Words: ");
				wstring nbLineLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbline", L"Lines: ");
				wstring nbByteLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbbyte", L"Document length: ");
				wstring nbSelLabel1 = pNativeSpeaker->getLocalizedStrFromID("summary-nbsel1", L" selected characters (");
				wstring nbSelLabel2 = pNativeSpeaker->getLocalizedStrFromID("summary-nbsel2", L" bytes) in ");
				wstring nbRangeLabel = pNativeSpeaker->getLocalizedStrFromID("summary-nbrange", L" ranges");

				UniMode um = _pEditView->getCurrentBuffer()->getUnicodeMode();
				size_t nbChar = getCurrentDocCharCount(um);
				int nbWord = wordCount();
				size_t nbLine = _pEditView->execute(SCI_GETLINECOUNT);
				size_t nbByte = _pEditView->execute(SCI_GETLENGTH);
				size_t nbSel = getSelectedCharNumber(um);
				size_t nbSelByte = getSelectedBytes();
				size_t nbRange = getSelectedAreas();

				characterNumber += nbCharLabel;
				characterNumber += commafyInt(nbChar).c_str();
				characterNumber += L"\r";

				characterNumber += nbWordLabel;
				characterNumber += commafyInt(nbWord).c_str();
				characterNumber += L"\r";

				characterNumber += nbLineLabel;
				characterNumber += commafyInt(nbLine).c_str();
				characterNumber += L"\r";

				characterNumber += nbByteLabel;
				characterNumber += commafyInt(nbByte).c_str();
				characterNumber += L"\r";

				characterNumber += commafyInt(nbSel).c_str();
				characterNumber += nbSelLabel1;
				characterNumber += commafyInt(nbSelByte).c_str();
				characterNumber += nbSelLabel2;
				characterNumber += commafyInt(nbRange).c_str();
				characterNumber += nbRangeLabel;
				characterNumber += L"\r";

				wstring summaryLabel = pNativeSpeaker->getLocalizedStrFromID("summary", L"Summary");

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
				const wchar_t *longFileName = curBuf->getFullPathName();
				if (doesFileExist(longFileName))
				{
					if (curBuf->isDirty())
					{
						_nativeLangSpeaker.messageBox("DocTooDirtyToMonitor",
							_pPublicInterface->getHSelf(),
							L"The document is dirty. Please save the modification before monitoring it.",
							L"Monitoring problem",
							MB_OK);
					}
					else
					{
						// Monitoring firstly for making monitoring icon
						monitoringStartOrStopAndUpdateUI(curBuf, true);
						createMonitoringThread(curBuf);
					}
				}
				else
				{
					_nativeLangSpeaker.messageBox("DocNoExistToMonitor",
						_pPublicInterface->getHSelf(),
						L"The file should exist to be monitored.",
						L"Monitoring problem",
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

			if (!buf->isReadOnly())
			{
				std::lock_guard<std::mutex> lock(command_mutex);
				buf->setEolFormat(newFormat);
				_pEditView->execute(SCI_CONVERTEOLS, static_cast<int>(buf->getEolFormat()));
			}

			break;
		}

		case IDM_FORMAT_ANSI :
		case IDM_FORMAT_UTF_8 :
		case IDM_FORMAT_UTF_16BE :
		case IDM_FORMAT_UTF_16LE :
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

				case IDM_FORMAT_UTF_16BE:
					um = uni16BE;
					break;

				case IDM_FORMAT_UTF_16LE:
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
						L"You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?",
						L"Save Current Modification",
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
					int answer = _nativeLangSpeaker.messageBox("LoseUndoAbilityWarning",
						_pPublicInterface->getHSelf(),
						L"You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?",
						L"Lose Undo Ability Waning",
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

			const EncodingMapper& em = EncodingMapper::getInstance();
			int encoding = em.getEncodingFromIndex(index);
			if (encoding == -1)
			{
				//printStr(L"Encoding problem. Command is not added in encoding_table?");
				return;
			}

            Buffer* buf = _pEditView->getCurrentBuffer();
            if (buf->isDirty())
            {
				int answer = _nativeLangSpeaker.messageBox("SaveCurrentModifWarning",
					_pPublicInterface->getHSelf(),
					L"You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?",
					L"Save Current Modification",
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
				int answer = _nativeLangSpeaker.messageBox("LoseUndoAbilityWarning",
					_pPublicInterface->getHSelf(),
					L"You should save the current modification.\rAll the saved modifications can not be undone.\r\rContinue?",
					L"Lose Undo Ability Waning",
					MB_YESNO);

                if (answer != IDYES)
                    return;
            }

            if (!buf->isDirty())
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
		case IDM_FORMAT_CONV2_UTF_16BE:
		case IDM_FORMAT_CONV2_UTF_16LE:
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

				case IDM_FORMAT_CONV2_UTF_16BE:
				{
                    if (encoding != -1)
                    {
                        buf->setDirty(true);
                        buf->setUnicodeMode(uni16BE);
                        buf->setEncoding(-1);
                        return;
                    }

					idEncoding = IDM_FORMAT_UTF_16BE;
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

				case IDM_FORMAT_CONV2_UTF_16LE:
				{
                    if (encoding != -1)
                    {
                        buf->setDirty(true);
                        buf->setUnicodeMode(uni16LE);
                        buf->setEncoding(-1);
                        return;
                    }

					idEncoding = IDM_FORMAT_UTF_16LE;
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
				// try to save the current clipboard CF_TEXT content 1st
				HGLOBAL hglbClipboardCopy = NULL;
				if (::OpenClipboard(_pPublicInterface->getHSelf()))
				{
					HANDLE hClipboardData = ::GetClipboardData(CF_TEXT);
					if (hClipboardData) // NULL if there is no previous CF_TEXT data in
					{
						LPVOID pClipboardData = ::GlobalLock(hClipboardData);
						if (pClipboardData)
						{
							size_t clipboardDataSize = ::GlobalSize(pClipboardData);
							hglbClipboardCopy = ::GlobalAlloc(GMEM_MOVEABLE, clipboardDataSize);
							if (hglbClipboardCopy)
							{
								LPVOID pClipboardCopy = ::GlobalLock(hglbClipboardCopy);
								if (pClipboardCopy)
								{
									::memcpy(pClipboardCopy, pClipboardData, clipboardDataSize);
									::GlobalUnlock(hglbClipboardCopy);
								}
								else
								{
									::GlobalFree(hglbClipboardCopy);
									hglbClipboardCopy = NULL;
								}
							}
							::GlobalUnlock(hClipboardData);
						}
					}
					::CloseClipboard();
				}

				_pEditView->saveCurrentPos();

				bool bPreviousCHPanelTrackingState = true;
				if (_pClipboardHistoryPanel)
					bPreviousCHPanelTrackingState = _pClipboardHistoryPanel->trackClipboardOps(false); // we do not want to track & show the next Clipboard op

				// Cut all text
				size_t docLen = _pEditView->getCurrentDocLen();
				_pEditView->execute(SCI_COPYRANGE, 0, docLen);
				_pEditView->execute(SCI_CLEARALL);

				if (_pClipboardHistoryPanel)
					_pClipboardHistoryPanel->trackClipboardOps(bPreviousCHPanelTrackingState); // restore

				// Change to the proper buffer, save buffer status

				::SendMessage(_pPublicInterface->getHSelf(), WM_COMMAND, idEncoding, 0);

				// Paste the text, restore buffer status
				_pEditView->execute(SCI_PASTE);
				_pEditView->restoreCurrentPosPreStep();

				// Restore the previous Clipboard data if any
				if (hglbClipboardCopy)
				{
					bool bAllOk = false;
					if (::OpenClipboard(_pPublicInterface->getHSelf()))
					{
						LPVOID pClipboardCopy = ::GlobalLock(hglbClipboardCopy);
						if (pClipboardCopy)
						{
							if (::EmptyClipboard())
							{
								if (::SetClipboardData(CF_TEXT, pClipboardCopy))
									bAllOk = true;
							}
							::GlobalUnlock(hglbClipboardCopy);
						}
						::CloseClipboard();
					}
					if (!bAllOk)
					{
						// when we failed to pass the data back to the Clipboard,
						// we have to free our copy here otherwise there will be memory leak
						::GlobalFree(hglbClipboardCopy);
						hglbClipboardCopy = NULL;
					}
				}
				else
				{
					// no previous Clipboard data, clear the ones used by the Scintilla's conversion
					if (::OpenClipboard(_pPublicInterface->getHSelf()))
					{
						::EmptyClipboard();
						::CloseClipboard();
					}
				}

				//Do not free anything, EmptyClipboard does that
				_pEditView->execute(SCI_EMPTYUNDOBUFFER);

				// The "save" point is on dirty state, so let's memorize it
				buf->setSavePointDirty(true);
			}
			break;
		}

		case IDM_SETTING_IMPORTPLUGIN :
        {
			// Copy plugins to Plugins Home
            const wchar_t *extFilterName = L"Notepad++ plugin";
            const wchar_t *extFilter = L".dll";
            vector<wstring> copiedFiles = addNppPlugins(extFilterName, extFilter);

            // Tell users to restart Notepad++ to load plugin
			if (copiedFiles.size())
			{
				NativeLangSpeaker *pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
				pNativeSpeaker->messageBox("NeedToRestartToLoadPlugins",
					_pPublicInterface->getHSelf(),
					L"You have to restart Notepad++ to load plugins you installed.",
					L"Notepad++ needs to be relaunched",
					MB_OK | MB_APPLMODAL);
			}
            break;
        }

        case IDM_SETTING_IMPORTSTYLETHEMES :
        {
            // get plugin source path
            const wchar_t *extFilterName = L"Notepad++ style theme";
            const wchar_t *extFilter = L".xml";
            const wchar_t *destDir = L"themes";

            // load styler
            NppParameters& nppParams = NppParameters::getInstance();
            ThemeSwitcher & themeSwitcher = nppParams.getThemeSwitcher();

            vector<wstring> copiedFiles = addNppComponents(destDir, extFilterName, extFilter);
            for (size_t i = 0, len = copiedFiles.size(); i < len ; ++i)
            {
                wstring themeName(themeSwitcher.getThemeFromXmlFileName(copiedFiles[i].c_str()));
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
				_pluginsAdminDlg.updateList();
			}
			break;
		}

		case IDM_SETTING_OPENPLUGINSDIR:
		{
			const wchar_t* pluginHomePath = NppParameters::getInstance().getPluginRootDir();
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
				L"Editing contextMenu.xml allows you to modify your Notepad++ popup context menu on edit zone.\rYou have to restart your Notepad++ to take effect after modifying contextMenu.xml.",
				L"Editing contextMenu",
				MB_OK|MB_APPLMODAL);

            const NppParameters& nppParams = NppParameters::getInstance();
            BufferID bufID = doOpen((nppParams.getContextMenuPath()));
			switchToFile(bufID);
            break;
        }

        case IDM_VIEW_GOTO_START:
			_pDocTab->tabToStart();
			break;

        case IDM_VIEW_GOTO_END:
			_pDocTab->tabToEnd();
			break;

        case IDM_VIEW_GOTO_ANOTHER_VIEW:
            docGotoAnotherEditView(TransferMove);
			checkSyncState();
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
            break;

        case IDM_VIEW_CLONE_TO_ANOTHER_VIEW:
            docGotoAnotherEditView(TransferClone);
			checkSyncState();
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
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

				intptr_t strLen = selectionEnd - selectionStart;
				if (strLen)
				{
					intptr_t strSize = strLen + 1;
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

		case IDM_TOOL_SHA1_GENERATE:
		{
			bool isFirstTime = !_sha1FromTextDlg.isCreated();
			_sha1FromTextDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_sha1FromTextDlg.getHSelf(), "SHA1FromTextDlg");
		}
		break;

		case IDM_TOOL_SHA1_GENERATEFROMFILE:
		{
			bool isFirstTime = !_sha1FromFilesDlg.isCreated();
			_sha1FromFilesDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_sha1FromFilesDlg.getHSelf(), "SHA1FromFilesDlg");
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

		case IDM_TOOL_SHA512_GENERATE:
		{
			bool isFirstTime = !_sha512FromTextDlg.isCreated();
			_sha512FromTextDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_sha512FromTextDlg.getHSelf(), "SHA512FromTextDlg");
		}
		break;

		case IDM_TOOL_SHA512_GENERATEFROMFILE:
		{
			bool isFirstTime = !_sha512FromFilesDlg.isCreated();
			_sha512FromFilesDlg.doDialog(_nativeLangSpeaker.isRTL());
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_sha512FromFilesDlg.getHSelf(), "SHA512FromFilesDlg");
		}
		break;

		case IDM_TOOL_SHA1_GENERATEINTOCLIPBOARD:
		case IDM_TOOL_SHA256_GENERATEINTOCLIPBOARD:
		case IDM_TOOL_SHA512_GENERATEINTOCLIPBOARD:
		{
			if (_pEditView->execute(SCI_GETSELECTIONS) == 1)
			{
				size_t selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
				size_t selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);

				intptr_t strLen = selectionEnd - selectionStart;
				if (strLen)
				{
					intptr_t strSize = strLen + 1;
					char *selectedStr = new char[strSize];
					_pEditView->execute(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(selectedStr));

					uint8_t hash[HASH_MAX_LENGTH] {};
					wchar_t hashStr[HASH_STR_MAX_LENGTH] {};
					int hashLen = 0;

					switch (id)
					{
						case IDM_TOOL_SHA1_GENERATEINTOCLIPBOARD:
						{
							calc_sha1(hash, reinterpret_cast<const uint8_t*>(selectedStr), strlen(selectedStr));
							hashLen = hash_sha1;
						}
						break;

						case IDM_TOOL_SHA256_GENERATEINTOCLIPBOARD:
						{
							calc_sha_256(hash, reinterpret_cast<const uint8_t*>(selectedStr), strlen(selectedStr));
							hashLen = hash_sha256;
						}
						break;
						
						case IDM_TOOL_SHA512_GENERATEINTOCLIPBOARD:
						{
							calc_sha_512(hash, reinterpret_cast<const uint8_t*>(selectedStr), strlen(selectedStr));
							hashLen = hash_sha512;
						}
						break;

						default:
							return;
					}
					for (int i = 0; i < hashLen; i++)
						wsprintf(hashStr + i * 2, L"%02x", hash[i]);

					str2Clipboard(hashStr, _pPublicInterface->getHSelf());

					delete[] selectedStr;
				}
			}
		}
		break;

		case IDM_DEBUGINFO:
		{
			const bool isFirstTime = !_debugInfoDlg.isCreated();
			_debugInfoDlg.doDialog();
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_debugInfoDlg.getHSelf(), "DebugInfo");
			break;
		}

        case IDM_ABOUT:
		{
			bool doAboutDlg = false;
			const int maxSelLen = 32;
			auto textLen = _pEditView->execute(SCI_GETSELTEXT, 0, 0);
			if (textLen <= 0)
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
					wstring noEasterEggsPath((NppParameters::getInstance()).getNppPath());
					noEasterEggsPath.append(L"\\noEasterEggs.xml");
					if (!doesFileExist(noEasterEggsPath.c_str()))
						showAllQuotes();
					return;
				}
				if (iQuote != -1)
				{
					wstring noEasterEggsPath((NppParameters::getInstance()).getNppPath());
					noEasterEggsPath.append(L"\\noEasterEggs.xml");
					if (!doesFileExist(noEasterEggsPath.c_str()))
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

		case IDM_HOMESWEETHOME :
		{
			::ShellExecute(NULL, L"open", L"https://notepad-plus-plus.org/", NULL, NULL, SW_SHOWNORMAL);
			break;
		}
		case IDM_PROJECTPAGE :
		{
			::ShellExecute(NULL, L"open", L"https://github.com/notepad-plus-plus/notepad-plus-plus/", NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_ONLINEDOCUMENT:
		{
			::ShellExecute(NULL, L"open", L"https://npp-user-manual.org/", NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_CMDLINEARGUMENTS:
		{
			// No translattable
			::MessageBox(_pPublicInterface->getHSelf(), COMMAND_ARG_HELP, L"Notepad++ Command Argument Help", MB_OK | MB_APPLMODAL);
			break;
		}

		case IDM_FORUM:
		{
			::ShellExecute(NULL, L"open", L"https://community.notepad-plus-plus.org/", NULL, NULL, SW_SHOWNORMAL);
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
					L"Notepad++ updater is not compatible with XP due to the obsolete security layer under XP.\rDo you want to go to Notepad++ page to download the latest version?",
					L"Notepad++ Updater",
					MB_YESNO);

				if (res == IDYES)
				{
					::ShellExecute(NULL, L"open", L"https://notepad-plus-plus.org/downloads/", NULL, NULL, SW_SHOWNORMAL);
				}
			}
			else
			{
				wstring updaterDir = (NppParameters::getInstance()).getNppPath();
				pathAppend(updaterDir, L"updater");

				wstring updaterFullPath = updaterDir;
				pathAppend(updaterFullPath, L"gup.exe");


#ifdef DEBUG // if not debug, then it's release
				bool isCertifVerified = true;
#else //RELEASE
				// check the signature on updater
				SecurityGuard securityGuard;
				bool isCertifVerified = securityGuard.checkModule(updaterFullPath, nm_gup);
#endif
				if (isCertifVerified)
				{
					wstring param;
					if (id == IDM_CONFUPDATERPROXY)
					{
						if (!_isAdministrator)
						{
							_nativeLangSpeaker.messageBox("GUpProxyConfNeedAdminMode",
								_pPublicInterface->getHSelf(),
								L"Please relaunch Notepad++ in Admin mode to configure proxy.",
								L"Proxy Settings",
								MB_OK | MB_APPLMODAL);
							return;
						}
						param = L"-options";
					}
					else
					{
						param = L"-verbose -v";
						param += VERSION_INTERNAL_VALUE;
						int archType = NppParameters::getInstance().archType();
						if (archType == IMAGE_FILE_MACHINE_AMD64)
						{
							param += L" -px64";
						}
						else if (archType == IMAGE_FILE_MACHINE_ARM64)
						{
							param += L" -parm64";
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

		case IDM_EDIT_FUNCCALLTIP_PREVIOUS :
			showFunctionNextHint(false);
			break;

		case IDM_EDIT_FUNCCALLTIP_NEXT :
			showFunctionNextHint();
			break;

        case IDM_LANGSTYLE_CONFIG_DLG :
		{
			if (!(NppParameters::getInstance()).isStylerDocLoaded())
			{
				// do nothing
			}
			else
			{
				bool isFirstTime = !_configStyleDlg.isCreated();
				_configStyleDlg.doDialog(_nativeLangSpeaker.isRTL());
				if (isFirstTime)
					_nativeLangSpeaker.changeConfigLang(_configStyleDlg.getHSelf());
			}
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
		case IDM_LANG_JSON5 :
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
        case IDM_LANG_MSSQL :
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
        case IDM_LANG_NIM :
        case IDM_LANG_NNCRONTAB :
        case IDM_LANG_OSCRIPT :
        case IDM_LANG_REBOL :
        case IDM_LANG_REGISTRY :
        case IDM_LANG_RUST :
        case IDM_LANG_SPICE :
        case IDM_LANG_TXT2TAGS :
        case IDM_LANG_VISUALPROLOG:
		case IDM_LANG_TYPESCRIPT:
		case IDM_LANG_GDSCRIPT:
		case IDM_LANG_HOLLYWOOD:
		case IDM_LANG_GOLANG:
		case IDM_LANG_RAKU:
		case IDM_LANG_TOML:
		case IDM_LANG_USER :
		{
			LangType lang = menuID2LangType(id);
			setLanguage(lang);

			// Manually set language, don't change language even file extension changes.
			Buffer *buffer = _pEditView->getCurrentBuffer();
			buffer->langHasBeenSetFromMenu();
			buffer->setLastLangType(static_cast<int>(lang));

			if (_pDocMap)
			{
				_pDocMap->setSyntaxHiliting();
			}
		}
        break;
		
		case IDM_LANG_OPENUDLDIR:
		{
			wstring userDefineLangFolderPath = NppParameters::getInstance().getUserDefineLangFolderPath();
			::ShellExecute(_pPublicInterface->getHSelf(), L"open", userDefineLangFolderPath.c_str(), NULL, NULL, SW_SHOW);
			break;
		}

		case IDM_LANG_UDLCOLLECTION_PROJECT_SITE:
		{
			::ShellExecute(NULL, L"open", L"https://github.com/notepad-plus-plus/userDefinedLanguages", NULL, NULL, SW_SHOWNORMAL);
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
						const int tabIconSet = NppDarkMode::getTabIconSet(NppDarkMode::isEnabled());
						HIMAGELIST hImgLst = _mainDocTab.getImgLst(tabIconSet);

						tld.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), hImgLst, direction);
						tld.doDialog(_nativeLangSpeaker.isRTL());
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
			bool toRTL = id == IDM_EDIT_RTL;

			bool isRTL = _pEditView->isTextDirectionRTL();

			if ((toRTL && isRTL) || (!toRTL && !isRTL))
			{
				if (! ((NppParameters::getInstance()).getNppGUI())._muteSounds)
					::MessageBeep(MB_OK);

				return;
			}

			_pEditView->changeTextDirection(toRTL);

			// Wrap then !wrap to fix problem of mirror characters
			bool isWraped = _pEditView->isWrap();
			_pEditView->wrap(!isWraped);
			_pEditView->wrap(isWraped);

			if (_pDocMap)
			{
				_pDocMap->changeTextDirection(toRTL);
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

		case IDM_WINDOW_SORT_FN_ASC :
		{
			WindowsDlg windowsDlg;
			windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			windowsDlg.sortFileNameASC();
			windowsDlg.doSort();
		}
		break;

		case IDM_WINDOW_SORT_FN_DSC :
		{
			WindowsDlg windowsDlg;
			windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			windowsDlg.sortFileNameDSC();
			windowsDlg.doSort();
		}
		break;

		case IDM_WINDOW_SORT_FP_ASC :
		{
			WindowsDlg windowsDlg;
			windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			windowsDlg.sortFilePathASC();
			windowsDlg.doSort();
		}
		break;

		case IDM_WINDOW_SORT_FP_DSC :
		{
			WindowsDlg windowsDlg;
			windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			windowsDlg.sortFilePathDSC();
			windowsDlg.doSort();
		}
		break;

		case IDM_WINDOW_SORT_FT_ASC :
		{
			WindowsDlg windowsDlg;
			windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			windowsDlg.sortFileTypeASC();
			windowsDlg.doSort();
		}
		break;

		case IDM_WINDOW_SORT_FT_DSC :
		{
			WindowsDlg windowsDlg;
			windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			windowsDlg.sortFileTypeDSC();
			windowsDlg.doSort();
		}
		break;

		case IDM_WINDOW_SORT_FS_ASC :
		{
			WindowsDlg windowsDlg;
			windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			windowsDlg.sortFileSizeASC();
			windowsDlg.doSort();
		}
		break;

		case IDM_WINDOW_SORT_FS_DSC :
		{
			WindowsDlg windowsDlg;
			windowsDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), _pDocTab);
			windowsDlg.sortFileSizeDSC();
			windowsDlg.doSort();
		}
		break;

		case IDM_SYSTRAYPOPUP_NEWDOC:
		{
			NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			_dockingManager.showFloatingContainers(true);
			restoreMinimizeDialogs();
			fileNew();
		}
		break;

		case IDM_SYSTRAYPOPUP_ACTIVATE :
		{
			NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			_dockingManager.showFloatingContainers(true);
			restoreMinimizeDialogs();

			// Send sizing info to make window fit (specially to show tool bar. Fixed issue #2600)
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
		}
		break;

		case IDM_SYSTRAYPOPUP_NEW_AND_PASTE:
		{
			NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			_dockingManager.showFloatingContainers(true);
			restoreMinimizeDialogs();

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
			NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
			::ShowWindow(_pPublicInterface->getHSelf(), nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
			_dockingManager.showFloatingContainers(true);
			restoreMinimizeDialogs();

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
			wstring lastOpenedFullPath = _lastRecentFileList.getFirstItem();
			if (!lastOpenedFullPath.empty())
			{
				BufferID lastOpened = doOpen(lastOpenedFullPath);
				if (lastOpened != BUFFER_INVALID)
					switchToFile(lastOpened);
			}
		}
		break;

		case IDM_PINTAB:
		{
			TBHDR nmhdr{};
			nmhdr._hdr.hwndFrom = _pDocTab->getHSelf();
			nmhdr._hdr.code = TCN_TABPINNED;
			nmhdr._hdr.idFrom = reinterpret_cast<UINT_PTR>(this);
			nmhdr._tabOrigin = _pDocTab->getCurrentTabIndex();
			::SendMessage(_pPublicInterface->getHSelf(), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmhdr));
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
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
				wchar_t langName[menuItemStrLenMax];
				::GetMenuString(_mainMenuHandle, id, langName, menuItemStrLenMax, MF_BYCOMMAND);
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
				const vector<UserCommand> & theUserCommands = (NppParameters::getInstance()).getUserCommandList();
				UserCommand ucmd = theUserCommands[i];

				Command cmd(string2wstring(ucmd.getCmd(), CP_UTF8));
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
			else if ((id >= IDM_WINDOW_MRU_FIRST) && (id <= IDM_WINDOW_MRU_LIMIT))
			{
				activateDoc(id - IDM_WINDOW_MRU_FIRST);
			}
			else if ((id >= IDM_DROPLIST_MRU_FIRST) && (id < (IDM_DROPLIST_MRU_FIRST + static_cast<int32_t>(_pDocTab->nbItem()))))
			{
				activateDoc(id - IDM_DROPLIST_MRU_FIRST);
			}
	}

	if (_recordingMacro)
		switch (id)
		{
			case IDM_FILE_NEW :
			case IDM_FILE_CLOSE :
			case IDM_FILE_CLOSEALL :
			case IDM_FILE_CLOSEALL_BUT_CURRENT :
			case IDM_FILE_CLOSEALL_BUT_PINNED :
			case IDM_FILE_CLOSEALL_TOLEFT :
			case IDM_FILE_CLOSEALL_TORIGHT :
			case IDM_FILE_CLOSEALL_UNCHANGED:
			case IDM_FILE_SAVE :
			case IDM_FILE_SAVEALL :
			case IDM_FILE_RELOAD:
			case IDM_EDIT_UNDO:
			case IDM_EDIT_REDO:
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
			case IDM_EDIT_REMOVE_CONSECUTIVE_DUP_LINES:
			case IDM_EDIT_REMOVE_ANY_DUP_LINES:
			case IDM_EDIT_TRANSPOSE_LINE:
			case IDM_EDIT_SPLIT_LINES:
			case IDM_EDIT_JOIN_LINES:
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
			case IDM_EDIT_BEGINENDSELECT_COLUMNMODE:
			case IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING:
			case IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING:
			case IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_ASCENDING:
			case IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_DESCENDING:
			case IDM_EDIT_SORTLINES_INTEGER_ASCENDING:
			case IDM_EDIT_SORTLINES_INTEGER_DESCENDING:
			case IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING:
			case IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING:
			case IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING:
			case IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING:
			case IDM_EDIT_SORTLINES_REVERSE_ORDER:
			case IDM_EDIT_SORTLINES_RANDOMLY:
			case IDM_EDIT_BLANKLINEABOVECURRENT:
			case IDM_EDIT_BLANKLINEBELOWCURRENT:
			case IDM_VIEW_FULLSCREENTOGGLE :
			case IDM_VIEW_ALWAYSONTOP :
			case IDM_VIEW_WRAP :
			case IDM_VIEW_FOLD_CURRENT :
			case IDM_VIEW_UNFOLD_CURRENT :
			case IDM_VIEW_FOLDALL:
			case IDM_VIEW_UNFOLDALL:
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
			case IDM_VIEW_GOTO_START:
			case IDM_VIEW_GOTO_END:
			case IDM_VIEW_GOTO_ANOTHER_VIEW:
			case IDM_VIEW_CLONE_TO_ANOTHER_VIEW:
			case IDM_VIEW_GOTO_NEW_INSTANCE:
			case IDM_VIEW_LOAD_IN_NEW_INSTANCE:
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
			case IDM_SEARCH_STYLE1TOCLIP:
			case IDM_SEARCH_STYLE2TOCLIP:
			case IDM_SEARCH_STYLE3TOCLIP:
			case IDM_SEARCH_STYLE4TOCLIP:
			case IDM_SEARCH_STYLE5TOCLIP:
			case IDM_SEARCH_ALLSTYLESTOCLIP:
			case IDM_SEARCH_MARKEDTOCLIP:
			case IDM_SEARCH_VOLATILE_FINDNEXT:
			case IDM_SEARCH_VOLATILE_FINDPREV:
			case IDM_SEARCH_CUTMARKEDLINES   :
			case IDM_SEARCH_COPYMARKEDLINES     :
			case IDM_SEARCH_PASTEMARKEDLINES    :
			case IDM_SEARCH_DELETEMARKEDLINES   :
			case IDM_SEARCH_DELETEUNMARKEDLINES :
			case IDM_SEARCH_MARKALLEXT1      :
			case IDM_SEARCH_MARKALLEXT2      :
			case IDM_SEARCH_MARKALLEXT3      :
			case IDM_SEARCH_MARKALLEXT4      :
			case IDM_SEARCH_MARKALLEXT5      :
			case IDM_SEARCH_MARKONEEXT1      :
			case IDM_SEARCH_MARKONEEXT2      :
			case IDM_SEARCH_MARKONEEXT3      :
			case IDM_SEARCH_MARKONEEXT4      :
			case IDM_SEARCH_MARKONEEXT5      :
			case IDM_SEARCH_UNMARKALLEXT1    :
			case IDM_SEARCH_UNMARKALLEXT2    :
			case IDM_SEARCH_UNMARKALLEXT3    :
			case IDM_SEARCH_UNMARKALLEXT4    :
			case IDM_SEARCH_UNMARKALLEXT5    :
			case IDM_SEARCH_CLEARALLMARKS    :
			case IDM_FORMAT_TODOS  :
			case IDM_FORMAT_TOUNIX :
			case IDM_FORMAT_TOMAC  :
			case IDM_VIEW_IN_FIREFOX :
			case IDM_VIEW_IN_CHROME  :
			case IDM_VIEW_IN_EDGE    :
			case IDM_VIEW_IN_IE      :
			case IDM_EDIT_COPY_ALL_NAMES:
			case IDM_EDIT_COPY_ALL_PATHS:
			case IDM_EDIT_MULTISELECTALLWHOLEWORD:
			case IDM_EDIT_MULTISELECTALLMATCHCASEWHOLEWORD:
			case IDM_EDIT_MULTISELECTNEXT:
			case IDM_EDIT_MULTISELECTNEXTMATCHCASE:
			case IDM_EDIT_MULTISELECTNEXTWHOLEWORD:
			case IDM_EDIT_MULTISELECTNEXTMATCHCASEWHOLEWORD:
			case IDM_EDIT_MULTISELECTUNDO:
			case IDM_EDIT_MULTISELECTSSKIP:
				_macro.push_back(recordedMacroStep(id));
				break;

			// No need to record the following commands: all they do is execute Scintilla commands, which are recorded instead.
			case IDM_EDIT_CUT:
			case IDM_EDIT_COPY:
			case IDM_EDIT_PASTE:
			case IDM_EDIT_LINE_UP:
			case IDM_EDIT_LINE_DOWN:
				break;

			// The following 3 commands will insert date time string during the recording:
			// SCI_REPLACESEL will be recorded firstly, then SCI_ADDTEXT (for adding date time string)
			// So we erase these 2 unwanted commanded for recording these 3 following commands.
			case IDM_EDIT_INSERT_DATETIME_SHORT:
			case IDM_EDIT_INSERT_DATETIME_LONG:
			case IDM_EDIT_INSERT_DATETIME_CUSTOMIZED:
			{
				size_t lastIndex = _macro.size();
				if (lastIndex >= 2)
				{
					--lastIndex;
					if (_macro[lastIndex]._message == SCI_ADDTEXT && _macro[lastIndex - 1]._message == SCI_REPLACESEL)
					{
						_macro.erase(_macro.begin() + lastIndex);
						--lastIndex;
						_macro.erase(_macro.begin() + lastIndex);
					}
				}
				_macro.push_back(recordedMacroStep(id));
			}
			break;
		}
}
