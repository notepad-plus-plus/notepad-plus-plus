     1|     1|// This file is part of npminmin project
     2|     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|     3|
     4|     4|// This program is free software: you can redistribute it and/or modify
     5|     5|// it under the terms of the GNU General Public License as published by
     6|     6|// the Free Software Foundation, either version 3 of the License, or
     7|     7|// at your option any later version.
     8|     8|//
     9|     9|// This program is distributed in the hope that it will be useful,
    10|    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|    12|// GNU General Public License for more details.
    13|    13|//
    14|    14|// You should have received a copy of the GNU General Public License
    15|    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|    16|
    17|    17|
    18|    18|#include <algorithm>
    19|    19|#include <memory>
    20|    20|#include <string>
    21|    21|#include <vector>
    22|    22|#include <shlwapi.h>
    23|    23|#include <shlobj.h>
    24|    24|#include <filesystem>
    25|    25|#include "Notepad_plus_Window.h"
    26|    26|#include "EncodingMapper.h"
    27|    27|#include "ShortcutMapper.h"
    28|    28|#include "TaskListDlg.h"
    29|    29|#include "clipboardFormats.h"
    30|    30|#include "VerticalFileSwitcher.h"
    31|    31|#include "documentMap.h"
    32|    32|#include "functionListPanel.h"
    33|    33|#include "ProjectPanel.h"
    34|    34|#include "fileBrowser.h"
    35|    35|#include "clipboardHistoryPanel.h"
    36|    36|#include "ansiCharPanel.h"
    37|    37|#include "Sorters.h"
    38|    38|#include "verifySignedfile.h"
    39|    39|#include "md5.h"
    40|    40|#include "sha-256.h"
    41|    41|#include "calc_sha1.h"
    42|    42|#include "sha512.h"
    43|    43|#include "SortLocale.h"
    44|    44|#include "dpiManagerV2.h"
    45|    45|
    46|    46|#include "NppConstants.h"
    47|    47|
    48|    48|using namespace std;
    49|    49|
    50|    50|std::mutex command_mutex;
    51|    51|
    52|    52|void Notepad_plus::macroPlayback(Macro macro, std::vector<Document>* pDocs4EndUAIn)
    53|    53|{
    54|    54|	_playingBackMacro = true;
    55|    55|
    56|    56|	std::vector<Document>* pDocs4EndUA = nullptr;
    57|    57|	if (pDocs4EndUAIn)
    58|    58|	{
    59|    59|		// continue with the passed param doc list
    60|    60|		pDocs4EndUA = pDocs4EndUAIn;
    61|    61|	}
    62|    62|	else
    63|    63|	{
    64|    64|		// use local doc list
    65|    65|		pDocs4EndUA = new std::vector<Document>;
    66|    66|		if (!pDocs4EndUA)
    67|    67|			return;
    68|    68|	}
    69|    69|
    70|    70|	Document prevSciDoc = 0;
    71|    71|	for (Macro::iterator step = macro.begin(); step != macro.end(); ++step)
    72|    72|	{
    73|    73|		Document curSciDoc = _pEditView->getCurrentBuffer()->getDocument();
    74|    74|		if (curSciDoc != prevSciDoc)
    75|    75|		{
    76|    76|			// macro step is going to work with different Scintilla Document object
    77|    77|			// (for which the undo actions are bound)
    78|    78|
    79|    79|			if (std::find(pDocs4EndUA->begin(), pDocs4EndUA->end(), curSciDoc) == pDocs4EndUA->end())
    80|    80|			{
    81|    81|				// not in the list of the macro affected docs so far
    82|    82|				_pEditView->execute(SCI_BEGINUNDOACTION); // the macro step will touch another doc, open another undo action
    83|    83|				pDocs4EndUA->push_back(curSciDoc); // store for possible ending undo action later
    84|    84|			}
    85|    85|
    86|    86|			prevSciDoc = curSciDoc; // remember the doc switch
    87|    87|		}
    88|    88|
    89|    89|		if (step->isScintillaMacro())
    90|    90|			step->PlayBack(_pPublicInterface, _pEditView);
    91|    91|		else
    92|    92|			_findReplaceDlg.execSavedCommand(step->_message, step->_lParameter, string2wstring(step->_sParameter, CP_UTF8));
    93|    93|	}
    94|    94|
    95|    95|	if (!pDocs4EndUAIn)
    96|    96|	{
    97|    97|		// handle all the affected docs undo actions closing (for local-only doc list)
    98|    98|
    99|    99|		Document invisSciDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER); // store the view's original doc
   100|   100|		while (!pDocs4EndUA->empty())
   101|   101|		{
   102|   102|			Document doc = pDocs4EndUA->back();
   103|   103|			if (MainFileManager.getBufferFromDocument(doc) == BUFFER_INVALID)
   104|   104|			{
   105|   105|				// affected doc no longer exists (a macro step closed its associated npminmin tab/buffer),
   106|   106|				// the ending undo action is not needed (until npminmin supports tab/buffer closing undo)
   107|   107|			}
   108|   108|			else
   109|   109|			{
   110|   110|				// complete the open undo action for existing doc object
   111|   111|				_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, doc);
   112|   112|				_invisibleEditView.execute(SCI_ENDUNDOACTION);
   113|   113|			}
   114|   114|			pDocs4EndUA->pop_back();
   115|   115|		}
   116|   116|		_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, invisSciDoc); // restore
   117|   117|
   118|   118|		delete pDocs4EndUA;
   119|   119|		pDocs4EndUA = nullptr;
   120|   120|	}
   121|   121|
   122|   122|	_playingBackMacro = false;
   123|   123|}
   124|   124|
   125|   125|
   126|   126|
   127|   127|void Notepad_plus::command(int id)
   128|   128|{
   129|   129|	switch (id)
   130|   130|	{
   131|   131|		case IDM_FILE_NEW:
   132|   132|		{
   133|   133|			fileNew();
   134|   134|		}
   135|   135|		break;
   136|   136|
   137|   137|		case IDM_EDIT_INSERT_DATETIME_SHORT:
   138|   138|		case IDM_EDIT_INSERT_DATETIME_LONG:
   139|   139|		{
   140|   140|			SYSTEMTIME currentTime = {};
   141|   141|			::GetLocalTime(&currentTime);
   142|   142|
   143|   143|			wchar_t dateStr[128] = { '\0' };
   144|   144|			wchar_t timeStr[128] = { '\0' };
   145|   145|
   146|   146|			int dateFlag = (id == IDM_EDIT_INSERT_DATETIME_SHORT) ? DATE_SHORTDATE : DATE_LONGDATE;
   147|   147|			GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, dateFlag, &currentTime, NULL, dateStr, sizeof(dateStr) / sizeof(dateStr[0]), NULL);
   148|   148|			GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, &currentTime, NULL, timeStr, sizeof(timeStr) / sizeof(timeStr[0]));
   149|   149|
   150|   150|			wstring dateTimeStr;
   151|   151|			if (NppParameters::getInstance().getNppGUI()._dateTimeReverseDefaultOrder)
   152|   152|			{
   153|   153|				// reverse default order: DATE + TIME
   154|   154|				dateTimeStr = dateStr;
   155|   155|				dateTimeStr += L" ";
   156|   156|				dateTimeStr += timeStr;
   157|   157|			}
   158|   158|			else
   159|   159|			{
   160|   160|				// default: TIME + DATE (Microsoft Notepad behaviour)
   161|   161|				dateTimeStr = timeStr;
   162|   162|				dateTimeStr += L" ";
   163|   163|				dateTimeStr += dateStr;
   164|   164|			}
   165|   165|			_pEditView->execute(SCI_BEGINUNDOACTION);
   166|   166|
   167|   167|			_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
   168|   168|			_pEditView->addGenericText(dateTimeStr.c_str());
   169|   169|
   170|   170|			_pEditView->execute(SCI_ENDUNDOACTION);
   171|   171|		}
   172|   172|		break;
   173|   173|
   174|   174|		case IDM_EDIT_INSERT_DATETIME_CUSTOMIZED:
   175|   175|		{
   176|   176|			SYSTEMTIME currentTime = {};
   177|   177|			::GetLocalTime(&currentTime);
   178|   178|
   179|   179|			NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
   180|   180|			wstring dateTimeStr = getDateTimeStrFrom(nppGUI._dateTimeFormat, currentTime);
   181|   181|
   182|   182|			_pEditView->execute(SCI_BEGINUNDOACTION);
   183|   183|
   184|   184|			_pEditView->execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
   185|   185|			_pEditView->addGenericText(dateTimeStr.c_str());
   186|   186|
   187|   187|			_pEditView->execute(SCI_ENDUNDOACTION);
   188|   188|		}
   189|   189|		break;
   190|   190|
   191|   191|		case IDM_FILE_OPEN:
   192|   192|		{
   193|   193|			fileOpen();
   194|   194|		}
   195|   195|		break;
   196|   196|
   197|   197|		case IDM_FILE_OPEN_FOLDER:
   198|   198|		{
   199|   199|			HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
   200|   200|
   201|   201|			ScopedCOMInit com;
   202|   202|			if (com.isInitialized())
   203|   203|			{
   204|   204|				ITEMIDLIST* pidl = nullptr;
   205|   205|				hr = ::SHParseDisplayName(_pEditView->getCurrentBuffer()->getFullPathName(), nullptr, &pidl, 0, nullptr);
   206|   206|				if (SUCCEEDED(hr))
   207|   207|				{
   208|   208|					hr = ::SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
   209|   209|					::CoTaskMemFree(pidl);
   210|   210|				}
   211|   211|			}
   212|   212|
   213|   213|			if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
   214|   214|			{
   215|   215|				// fallback (but without selecting the current file)
   216|   216|				// - either the COM cannot be used or the above shell APIs mysteriously fail on some systems
   217|   217|				//   with the "file not found" even though the file is there
   218|   218|				// - do not use this fallback for any other possible error (like E_INVALIDARG, etc.)
   219|   219|				::ShellExecuteW(_pPublicInterface->getHSelf(), L"explore",
   220|   220|					std::filesystem::path(_pEditView->getCurrentBuffer()->getFullPathName()).parent_path().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
   221|   221|			}
   222|   222|
   223|   223|			break;
   224|   224|		}
   225|   225|
   226|   226|		case IDM_FILE_OPEN_CMD:
   227|   227|		{
   228|   228|			Command cmd(NppParameters::getInstance().getNppGUI()._commandLineInterpreter.c_str());
   229|   229|			cmd.run(_pPublicInterface->getHSelf(), L"$(CURRENT_DIRECTORY)");
   230|   230|		}
   231|   231|		break;
   232|   232|
   233|   233|		case IDM_FILE_CONTAININGFOLDERASWORKSPACE:
   234|   234|		{
   235|   235|			const int strSize = CURRENTWORD_MAXLENGTH;
   236|   236|			auto currentFile = std::make_unique<wchar_t[]>(strSize);
   237|   237|			std::fill_n(currentFile.get(), strSize, L'\0');
   238|   238|
   239|   239|			auto currentDir = std::make_unique<wchar_t[]>(strSize);
   240|   240|			std::fill_n(currentDir.get(), strSize, L'\0');
   241|   241|
   242|   242|			::SendMessage(_pPublicInterface->getHSelf(), NPPM_GETFULLCURRENTPATH, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(currentFile.get()));
   243|   243|			::SendMessage(_pPublicInterface->getHSelf(), NPPM_GETCURRENTDIRECTORY, CURRENTWORD_MAXLENGTH, reinterpret_cast<LPARAM>(currentDir.get()));
   244|   244|	
   245|   245|			if (!_pFileBrowser)
   246|   246|			{
   247|   247|				command(IDM_VIEW_FILEBROWSER);
   248|   248|			}
   249|   249|
   250|   250|			vector<wstring> folders;
   251|   251|			folders.push_back(currentDir.get());
   252|   252|			
   253|   253|			launchFileBrowser(folders, currentFile.get());
   254|   254|		}
   255|   255|		break;
   256|   256|
   257|   257|		case IDM_FILE_OPEN_DEFAULT_VIEWER:
   258|   258|		{
   259|   259|			// Opens file in its default viewer. 
   260|   260|            // Has the same effect as double–clicking this file in Windows Explorer.
   261|   261|            BufferID buf = _pEditView->getCurrentBufferID();
   262|   262|			HINSTANCE res = ::ShellExecute(NULL, L"open", buf->getFullPathName(), NULL, NULL, SW_SHOW);
   263|   263|
   264|   264|			// As per MSDN (https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153(v=vs.85).aspx)
   265|   265|			// If the function succeeds, it returns a value greater than 32.
   266|   266|			// If the function fails, it returns an error value that indicates the cause of the failure.
   267|   267|			int retResult = static_cast<int>(reinterpret_cast<intptr_t>(res));
   268|   268|			if (retResult <= 32)
   269|   269|			{
   270|   270|				wstring errorMsg;
   271|   271|				errorMsg += GetLastErrorAsString(retResult);
   272|   272|				errorMsg += L"An attempt was made to execute the below command.";
   273|   273|				errorMsg += L"\n----------------------------------------------------------";
   274|   274|				errorMsg += L"\nCommand: ";
   275|   275|				errorMsg += buf->getFullPathName();
   276|   276|				errorMsg += L"\nError Code: ";
   277|   277|				errorMsg += intToString(retResult);
   278|   278|				errorMsg += L"\n----------------------------------------------------------";
   279|   279|				
   280|   280|				::MessageBox(_pPublicInterface->getHSelf(), errorMsg.c_str(), L"ShellExecute - ERROR", MB_ICONINFORMATION | MB_APPLMODAL);
   281|   281|			}
   282|   282|		}
   283|   283|		break;
   284|   284|
   285|   285|		case IDM_FILE_OPENFOLDERASWORKSPACE:
   286|   286|		{
   287|   287|			const NativeLangSpeaker* pNativeSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
   288|   288|			wstring openWorkspaceStr = pNativeSpeaker->getAttrNameStr(L"Select a folder to add in Folder as Workspace panel",
   289|   289|				FOLDERASWORKSPACE_NODE, "SelectFolderFromBrowserString");
   290|   290|			wstring folderPath = folderBrowser(_pPublicInterface->getHSelf(), openWorkspaceStr);
   291|   291|			if (!folderPath.empty())
   292|   292|			{
   293|   293|				if (_pFileBrowser == nullptr) // first launch, check in params to open folders
   294|   294|				{
   295|   295|					vector<wstring> dummy;
   296|   296|					wstring emptyStr;
   297|   297|					launchFileBrowser(dummy, emptyStr);
   298|   298|					if (_pFileBrowser != nullptr)
   299|   299|					{
   300|   300|						checkMenuItem(IDM_VIEW_FILEBROWSER, true);
   301|   301|						_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
   302|   302|						_pFileBrowser->setClosed(false);
   303|   303|					}
   304|   304|					else // problem
   305|   305|						return;
   306|   306|				}
   307|   307|				else
   308|   308|				{
   309|   309|					if (_pFileBrowser->isClosed())
   310|   310|					{
   311|   311|						_pFileBrowser->display();
   312|   312|						checkMenuItem(IDM_VIEW_FILEBROWSER, true);
   313|   313|						_toolBar.setCheck(IDM_VIEW_FILEBROWSER, true);
   314|   314|						_pFileBrowser->setClosed(false);
   315|   315|					}
   316|   316|				}
   317|   317|				_pFileBrowser->addRootFolder(folderPath);
   318|   318|			}
   319|   319|		}
   320|   320|		break;
   321|   321|
   322|   322|		case IDM_FILE_RELOAD:
   323|   323|			fileReload();
   324|   324|			break;
   325|   325|
   326|   326|		case IDM_DOCLIST_FILESCLOSE:
   327|   327|		case IDM_DOCLIST_FILESCLOSEOTHERS:
   328|   328|			if (_pDocumentListPanel)
   329|   329|			{
   330|   330|				vector<BufferViewInfo> bufs2Close = _pDocumentListPanel->getSelectedFiles(id == IDM_DOCLIST_FILESCLOSEOTHERS);
   331|   331|
   332|   332|				fileCloseAllGiven(bufs2Close);
   333|   333|
   334|   334|				if (id == IDM_DOCLIST_FILESCLOSEOTHERS)
   335|   335|				{
   336|   336|					// Get current buffer and its view
   337|   337|					_pDocumentListPanel->activateItem(_pEditView->getCurrentBufferID(), currentView());
   338|   338|				}
   339|   339|			}
   340|   340|			break;
   341|   341|
   342|   342|		case IDM_DOCLIST_COPYNAMES:
   343|   343|		case IDM_DOCLIST_COPYPATHS:
   344|   344|			if (_pDocumentListPanel)
   345|   345|			{
   346|   346|				std::vector<Buffer*> buffers;
   347|   347|				auto files = _pDocumentListPanel->getSelectedFiles(false);
   348|   348|				for (auto&& sel : files)
   349|   349|					buffers.push_back(MainFileManager.getBufferByID(sel._bufID));
   350|   350|				buf2Clipboard(buffers, id == IDM_DOCLIST_COPYPATHS, _pDocumentListPanel->getHSelf());
   351|   351|			}
   352|   352|			break;
   353|   353|
   354|   354|		case IDM_FILE_CLOSE:
   355|   355|			if (fileClose())
   356|   356|                checkDocState();
   357|   357|			break;
   358|   358|
   359|   359|		case IDM_FILE_DELETE:
   360|   360|			if (fileDelete())
   361|   361|                checkDocState();
   362|   362|			break;
   363|   363|
   364|   364|		case IDM_FILE_RENAME:
   365|   365|			fileRename();
   366|   366|			break;
   367|   367|
   368|   368|		case IDM_FILE_CLOSEALL:
   369|   369|		{
   370|   370|			bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
   371|   371|			fileCloseAll(isSnapshotMode, false);
   372|   372|			checkDocState();
   373|   373|			break;
   374|   374|		}
   375|   375|
   376|   376|		case IDM_FILE_CLOSEALL_BUT_CURRENT :
   377|   377|			fileCloseAllButCurrent();
   378|   378|			checkDocState();
   379|   379|			break;
   380|   380|
   381|   381|		case IDM_FILE_CLOSEALL_BUT_PINNED :
   382|   382|			fileCloseAllButPinned();
   383|   383|			checkDocState();
   384|   384|			break;
   385|   385|
   386|   386|		case IDM_FILE_CLOSEALL_TOLEFT :
   387|   387|			fileCloseAllToLeft();
   388|   388|			checkDocState();
   389|   389|			break;
   390|   390|
   391|   391|		case IDM_FILE_CLOSEALL_TORIGHT :
   392|   392|			fileCloseAllToRight();
   393|   393|			checkDocState();
   394|   394|			break;
   395|   395|
   396|   396|		case IDM_FILE_CLOSEALL_UNCHANGED:
   397|   397|			fileCloseAllUnchanged();
   398|   398|			checkDocState();
   399|   399|			break;
   400|   400|
   401|   401|		case IDM_FILE_SAVE :
   402|   402|			fileSave();
   403|   403|			break;
   404|   404|
   405|   405|		case IDM_FILE_SAVEALL :
   406|   406|			fileSaveAll();
   407|   407|			break;
   408|   408|
   409|   409|		case IDM_FILE_SAVEAS :
   410|   410|			fileSaveAs();
   411|   411|			break;
   412|   412|
   413|   413|		case IDM_FILE_SAVECOPYAS :
   414|   414|			fileSaveAs(BUFFER_INVALID, true);
   415|   415|			break;
   416|   416|
   417|   417|		case IDM_FILE_LOADSESSION:
   418|   418|			fileLoadSession();
   419|   419|			break;
   420|   420|
   421|   421|		case IDM_FILE_SAVESESSION:
   422|   422|			fileSaveSession();
   423|   423|			break;
   424|   424|
   425|   425|		case IDM_FILE_PRINTNOW :
   426|   426|			filePrint(false);
   427|   427|			break;
   428|   428|
   429|   429|		case IDM_FILE_PRINT :
   430|   430|			filePrint(true);
   431|   431|			break;
   432|   432|
   433|   433|		case IDM_FILE_EXIT:
   434|   434|			::PostMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
   435|   435|			break;
   436|   436|
   437|   437|		case IDM_EDIT_UNDO:
   438|   438|		{
   439|   439|			std::lock_guard<std::mutex> lock(command_mutex);
   440|   440|			_pEditView->execute(WM_UNDO);
   441|   441|			checkClipboard();
   442|   442|			checkUndoState();
   443|   443|			break;
   444|   444|		}
   445|   445|
   446|   446|		case IDM_EDIT_REDO:
   447|   447|		{
   448|   448|			std::lock_guard<std::mutex> lock(command_mutex);
   449|   449|			_pEditView->execute(SCI_REDO);
   450|   450|			checkClipboard();
   451|   451|			checkUndoState();
   452|   452|			break;
   453|   453|		}
   454|   454|		
   455|   455|		case IDM_EDIT_CUT:
   456|   456|		{
   457|   457|			HWND focusedHwnd = ::GetFocus();
   458|   458|			if (focusedHwnd == _pEditView->getHSelf())
   459|   459|			{
   460|   460|				if (_pEditView->hasSelection()) // Cut normally
   461|   461|				{
   462|   462|					_pEditView->execute(WM_CUT);
   463|   463|				}
   464|   464|				else if (NppParameters::getInstance().getSVP()._lineCopyCutWithoutSelection) // Cut the entire line with EOL
   465|   465|				{
   466|   466|					_pEditView->execute(SCI_COPYALLOWLINE);
   467|   467|					_pEditView->execute(SCI_LINEDELETE);
   468|   468|				}
   469|   469|			}
   470|   470|			else
   471|   471|			{
   472|   472|				::SendMessage(focusedHwnd, WM_CUT, 0, 0);
   473|   473|			}
   474|   474|			checkClipboard(); // for enabling possible Paste command
   475|   475|			break;
   476|   476|		}
   477|   477|
   478|   478|		case IDM_EDIT_COPY:
   479|   479|		{
   480|   480|			HWND focusedHwnd = ::GetFocus();
   481|   481|			if (focusedHwnd == _pEditView->getHSelf())
   482|   482|			{
   483|   483|				if (_pEditView->hasSelection())
   484|   484|				{
   485|   485|					_pEditView->execute(WM_COPY);
   486|   486|				}
   487|   487|				else if (NppParameters::getInstance().getSVP()._lineCopyCutWithoutSelection)
   488|   488|				{
   489|   489|					_pEditView->execute(SCI_COPYALLOWLINE); // Copy without selected text, it will copy the whole line with EOL, for pasting before any line where the caret is.
   490|   490|				}
   491|   491|			}
   492|   492|			else
   493|   493|			{
   494|   494|				Finder* finder = _findReplaceDlg.getFinderFrom(focusedHwnd);
   495|   495|				if (finder)  // Search result
   496|   496|					finder->scintillaExecute(WM_COPY);
   497|   497|				else
   498|   498|					::SendMessage(focusedHwnd, WM_COPY, 0, 0);
   499|   499|			}
   500|   500|			checkClipboard(); // for enabling possible Paste command
   501|