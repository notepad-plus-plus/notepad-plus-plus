     1|// This file is part of npminmin project
     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|
     4|// This program is free software: you can redistribute it and/or modify
     5|// it under the terms of the GNU General Public License as published by
     6|// the Free Software Foundation, either version 3 of the License, or
     7|// at your option any later version.
     8|//
     9|// This program is distributed in the hope that it will be useful,
    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|// GNU General Public License for more details.
    13|//
    14|// You should have received a copy of the GNU General Public License
    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|
    17|
    18|#include <ctime>
    19|#include <shlwapi.h>
    20|#include <shlobj.h>
    21|#include "Notepad_plus_Window.h"
    22|#include "CustomFileDialog.h"
    23|#include "VerticalFileSwitcher.h"
    24|#include "functionListPanel.h"
    25|#include "ReadDirectoryChanges.h"
    26|#include "ReadFileChanges.h"
    27|#include "fileBrowser.h"
    28|#include <unordered_set>
    29|#include "Common.h"
    30|#include "NppConstants.h"
    31|
    32|using namespace std;
    33|
    34|// https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
    35|// Reserved characters:  < > : " / \ | ? * tab  
    36|//  ("tab" is not in the official list, but it is good to avoid it)
    37|const std::wstring filenameReservedChars = L"<>:\"/\\|\?*\t";
    38|
    39|DWORD WINAPI Notepad_plus::monitorFileOnChange(void * params)
    40|{
    41|	MonitorInfo *monitorInfo = static_cast<MonitorInfo *>(params);
    42|	Buffer *buf = monitorInfo->_buffer;
    43|	HWND h = monitorInfo->_nppHandle;
    44|
    45|	const wchar_t *fullFileName = (const wchar_t *)buf->getFullPathName();
    46|
    47|	//The folder to watch :
    48|	wchar_t folderToMonitor[MAX_PATH]{};
    49|	wcscpy_s(folderToMonitor, fullFileName);
    50|
    51|	::PathRemoveFileSpecW(folderToMonitor);
    52|	
    53|	const DWORD dwNotificationFlags = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE;
    54|
    55|	// Create the monitor and add directory to watch.
    56|	CReadDirectoryChanges dirChanges;
    57|	dirChanges.AddDirectory(folderToMonitor, true, dwNotificationFlags);
    58|
    59|	CReadFileChanges fileChanges;
    60|	fileChanges.AddFile(fullFileName, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE);
    61|
    62|	HANDLE changeHandles[] = { buf->getMonitoringEvent(), dirChanges.GetWaitHandle() };
    63|
    64|	bool toBeContinued = true;
    65|
    66|	while (toBeContinued)
    67|	{
    68|		DWORD waitStatus = ::WaitForMultipleObjects(_countof(changeHandles), changeHandles, FALSE, 250);
    69|		switch (waitStatus)
    70|		{
    71|			case WAIT_OBJECT_0 + 0:
    72|			// Mutex was signaled. User removes this folder or file browser is closed
    73|			{
    74|				toBeContinued = false;
    75|			}
    76|			break;
    77|
    78|			case WAIT_OBJECT_0 + 1:
    79|			// We've received a notification in the queue.
    80|			{
    81|				bool bDoneOnce = false;
    82|				DWORD dwAction = 0;
    83|				wstring fn;
    84|				// Process all available changes, ignore User actions
    85|				while (dirChanges.Pop(dwAction, fn))
    86|				{
    87|					// Fix monitoring files which are under root problem
    88|					size_t pos = fn.find(L"\\\\");
    89|					if (pos == 2)
    90|						fn.replace(pos, 2, L"\\");
    91|
    92|					if (wcscmp(fullFileName, fn.c_str()) == 0)
    93|					{
    94|						if (dwAction == FILE_ACTION_MODIFIED)
    95|						{
    96|							if (!bDoneOnce)
    97|							{
    98|								::PostMessage(h, NPPM_INTERNAL_RELOADSCROLLTOEND, reinterpret_cast<WPARAM>(buf), 0);
    99|								bDoneOnce = true;
   100|								Sleep(250);	// Limit refresh rate
   101|							}
   102|						}
   103|						else if ((dwAction == FILE_ACTION_REMOVED) || (dwAction == FILE_ACTION_RENAMED_OLD_NAME))
   104|						{
   105|							// File is deleted or renamed - quit monitoring thread and close file
   106|							::PostMessage(h, NPPM_INTERNAL_STOPMONITORING, reinterpret_cast<WPARAM>(buf), 0);
   107|						}
   108|					}
   109|				}
   110|			}
   111|			break;
   112|
   113|			case WAIT_TIMEOUT:
   114|			{
   115|				if (fileChanges.DetectChanges())
   116|					::PostMessage(h, NPPM_INTERNAL_RELOADSCROLLTOEND, reinterpret_cast<WPARAM>(buf), 0);
   117|			}
   118|			break;
   119|
   120|			case WAIT_IO_COMPLETION:
   121|				// Nothing to do.
   122|			break;
   123|		}
   124|	}
   125|
   126|	// Just for sample purposes. The destructor will
   127|	// call Terminate() automatically.
   128|	dirChanges.Terminate();
   129|	fileChanges.Terminate();
   130|	delete monitorInfo;
   131|	return ERROR_SUCCESS;
   132|}
   133|
   134|bool resolveLinkFile(std::wstring& linkFilePath)
   135|{
   136|	// upperize for the following comparison because the ends_with is case sensitive unlike the Windows OS filesystem
   137|	std::wstring linkFilePathUp = linkFilePath;
   138|	std::transform(linkFilePathUp.begin(), linkFilePathUp.end(), linkFilePathUp.begin(), ::towupper);
   139|	if (!linkFilePathUp.ends_with(L".LNK"))
   140|		return false; // we will not check the renamed shortcuts like "file.lnk.txt"
   141|
   142|	bool isResolved = false;
   143|
   144|	IShellLink* psl = nullptr;
   145|	wchar_t targetFilePath[MAX_PATH]{};
   146|	WIN32_FIND_DATA wfd{};
   147|
   148|	HRESULT hres = CoInitialize(NULL);
   149|	if (SUCCEEDED(hres))
   150|	{
   151|		hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
   152|		if (SUCCEEDED(hres))
   153|		{
   154|			IPersistFile* ppf = nullptr;
   155|			hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
   156|			if (SUCCEEDED(hres))
   157|			{
   158|				// Load the shortcut. 
   159|				hres = ppf->Load(linkFilePath.c_str(), STGM_READ);
   160|				if (SUCCEEDED(hres) && hres != S_FALSE)
   161|				{
   162|					// Resolve the link. 
   163|					hres = psl->Resolve(NULL, 0);
   164|					if (SUCCEEDED(hres) && hres != S_FALSE)
   165|					{
   166|						// Get the path to the link target. 
   167|						hres = psl->GetPath(targetFilePath, MAX_PATH, (WIN32_FIND_DATA*)&wfd, SLGP_SHORTPATH);
   168|						if (SUCCEEDED(hres) && hres != S_FALSE)
   169|						{
   170|							linkFilePath = targetFilePath;
   171|							isResolved = true;
   172|						}
   173|					}
   174|				}
   175|				ppf->Release();
   176|			}
   177|			psl->Release();
   178|		}
   179|		CoUninitialize();
   180|	}
   181|
   182|	return isResolved;
   183|}
   184|
   185|BufferID Notepad_plus::doOpen(const wstring& fileName, bool isRecursive, bool isReadOnly, int encoding, const wchar_t *backupFileName, FILETIME fileNameTimestamp)
   186|{
   187|	const rsize_t longFileNameBufferSize = MAX_PATH;
   188|	if (fileName.size() >= longFileNameBufferSize - 1)
   189|		return BUFFER_INVALID;
   190|
   191|	wstring targetFileName = fileName;
   192|	bool isResolvedLinkFileName = resolveLinkFile(targetFileName);
   193|
   194|	bool isRawFileName;
   195|	if (isResolvedLinkFileName)
   196|		isRawFileName = false;
   197|	else
   198|		isRawFileName = isWin32NamespacePrefixedFileName(fileName);
   199|
   200|	if (isUnsupportedFileName(isResolvedLinkFileName ? targetFileName : fileName))
   201|	{
   202|		// TODO:
   203|		// for the raw filenames we can allow even the usually unsupported filenames in the future,
   204|		// but not now as it is not fully supported by the npminmin COM IFileDialog based Open/SaveAs dialogs
   205|		//if (isRawFileName)
   206|		//{
   207|		//	int answer = _nativeLangSpeaker.messageBox("OpenNonconformingWin32FileName",
   208|		//		_pPublicInterface->getHSelf(),
   209|		//		L"You are about to open a file with unusual filename:\n\"$STR_REPLACE$\"",
   210|		//		L"Open Nonconforming Win32-Filename",
   211|		//		MB_OKCANCEL | MB_ICONWARNING | MB_APPLMODAL,
   212|		//		0,
   213|		//		isResolvedLinkFileName ? targetFileName.c_str() : fileName.c_str());
   214|		//	if (answer != IDOK)
   215|		//		return BUFFER_INVALID; // aborted by user
   216|		//}
   217|		//else
   218|		//{
   219|			// unsupported, use the existing npminmin file dialog to report
   220|			_nativeLangSpeaker.messageBox("OpenFileError",
   221|				_pPublicInterface->getHSelf(),
   222|				L"Cannot open file \"$STR_REPLACE$\".",
   223|				L"ERROR",
   224|				MB_OK,
   225|				0,
   226|				isResolvedLinkFileName ? targetFileName.c_str() : fileName.c_str());
   227|			return BUFFER_INVALID;
   228|		//}
   229|	}
   230|
   231|	//If [GetFullPathName] succeeds, the return value is the length, in TCHARs, of the string copied to lpBuffer, not including the terminating null character.
   232|	//If the lpBuffer buffer is too small to contain the path, the return value [of GetFullPathName] is the size, in TCHARs, of the buffer that is required to hold the path and the terminating null character.
   233|	//If [GetFullPathName] fails for any other reason, the return value is zero.
   234|
   235|	NppParameters& nppParam = NppParameters::getInstance();
   236|	wchar_t longFileName[longFileNameBufferSize] = { 0 };
   237|
   238|	if (isRawFileName)
   239|	{
   240|		// use directly the raw file name, skip the GetFullPathName WINAPI and alike...)
   241|		wcsncpy_s(longFileName, _countof(longFileName), fileName.c_str(), _TRUNCATE);
   242|	}
   243|	else
   244|	{
   245|		const DWORD getFullPathNameResult = ::GetFullPathName(targetFileName.c_str(), longFileNameBufferSize, longFileName, NULL);
   246|		if (getFullPathNameResult == 0)
   247|		{
   248|			return BUFFER_INVALID;
   249|		}
   250|		if (getFullPathNameResult > longFileNameBufferSize)
   251|		{
   252|			return BUFFER_INVALID;
   253|		}
   254|		assert(wcslen(longFileName) == getFullPathNameResult);
   255|
   256|		if (wcschr(longFileName, '~'))
   257|		{
   258|			// ignore the returned value of function due to win64 redirection system
   259|			::GetLongPathName(longFileName, longFileName, longFileNameBufferSize);
   260|		}
   261|	}
   262|
   263|	bool isSnapshotMode = (backupFileName != NULL) && doesFileExist(backupFileName);
   264|	bool longFileNameExists = doesFileExist(longFileName);
   265|	if (isSnapshotMode && !longFileNameExists) // UNTITLED
   266|	{
   267|		wcscpy_s(longFileName, targetFileName.c_str());
   268|	}
   269|    _lastRecentFileList.remove(longFileName);
   270|
   271|
   272|	// "fileName" could be:
   273|	// 1. full file path to open or create
   274|	// 2. "new N" or whatever renamed from "new N" to switch to (if user double-clicks the found entries of untitled documents, which is the results of running commands "Find in current doc" & "Find in opened docs")
   275|	// 3. a file name with relative path to open or create
   276|
   277|	// Search case 1 & 2 firstly
   278|	BufferID foundBufID = MainFileManager.getBufferFromName(targetFileName.c_str());
   279|
   280|	// if case 1 & 2 not found, search case 3
   281|	if (foundBufID == BUFFER_INVALID)
   282|	{
   283|		wstring fileName2Find;
   284|		fileName2Find = longFileName;
   285|		foundBufID = MainFileManager.getBufferFromName(fileName2Find.c_str());
   286|	}
   287|
   288|	// If we found the document, then we don't open the existing doc. We return the found buffer ID instead.
   289|    if (foundBufID != BUFFER_INVALID && !isSnapshotMode)
   290|    {
   291|        if (_pTrayIco)
   292|        {
   293|            if (_pTrayIco->isInTray())
   294|            {
   295|                ::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
   296|                if (!_pPublicInterface->isPrelaunch())
   297|                    _pTrayIco->doTrayIcon(REMOVE);
   298|                ::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
   299|            }
   300|        }
   301|        return foundBufID;
   302|    }
   303|
   304|    if (isFileSession(longFileName) && longFileNameExists)
   305|    {
   306|        fileLoadSession(longFileName);
   307|        return BUFFER_INVALID;
   308|    }
   309|
   310|	if (isFileWorkspace(longFileName) && longFileNameExists)
   311|	{
   312|		nppParam.setWorkSpaceFilePath(0, longFileName);
   313|		// This line switches to Project Panel 1 while starting up Npp
   314|		// and after dragging a workspace file to Npp:
   315|		launchProjectPanel(IDM_VIEW_PROJECT_PANEL_1, &_pProjectPanel_1, 0);
   316|		return BUFFER_INVALID;
   317|	}
   318|
   319|#ifndef	_WIN64
   320|    bool isWow64Off = false;
   321|    if (!longFileNameExists)
   322|    {
   323|        nppParam.safeWow64EnableWow64FsRedirection(FALSE);
   324|        isWow64Off = true;
   325|    }
   326|#endif
   327|
   328|	bool globbing;
   329|	if (isRawFileName)
   330|		globbing = (wcsrchr(longFileName, wchar_t('*')) || (abs(longFileName - wcsrchr(longFileName, wchar_t('?'))) > 3));
   331|	else
   332|		globbing = (wcsrchr(longFileName, wchar_t('*')) || wcsrchr(longFileName, wchar_t('?')));
   333|
   334|	if (!isSnapshotMode) // if not backup mode, or backupfile path is invalid
   335|	{
   336|		if (!doesPathExist(longFileName) && !globbing)
   337|		{
   338|			wstring longFileDir(longFileName);
   339|			pathRemoveFileSpec(longFileDir);
   340|
   341|			bool isCreateFileSuccessful = false;
   342|			if (doesDirectoryExist(longFileDir.c_str()))
   343|			{
   344|				int res = _nativeLangSpeaker.messageBox("CreateNewFileOrNot",
   345|					_pPublicInterface->getHSelf(),
   346|					L"\"$STR_REPLACE$\" doesn't exist. Create it?",
   347|					L"Create new file",
   348|					MB_YESNO,
   349|					0,
   350|					longFileName);
   351|
   352|				if (res == IDYES)
   353|				{
   354|					bool isOK = MainFileManager.createEmptyFile(longFileName);
   355|					if (isOK)
   356|					{
   357|						isCreateFileSuccessful = true;
   358|					}
   359|					else
   360|					{
   361|						_nativeLangSpeaker.messageBox("CreateNewFileError",
   362|							_pPublicInterface->getHSelf(),
   363|							L"Cannot create the file \"$STR_REPLACE$\".",
   364|							L"Create new file",
   365|							MB_OK,
   366|							0,
   367|							longFileName);
   368|					}
   369|				}
   370|			}
   371|			else
   372|			{
   373|				wstring msg, title;
   374|				if (!_nativeLangSpeaker.getMsgBoxLang("OpenFileNoFolderError", title, msg))
   375|				{
   376|					title = L"Cannot open file";
   377|					msg = L"\"";
   378|					msg += longFileName;
   379|					msg += L"\" cannot be opened:\nFolder \"";
   380|					msg += longFileDir;
   381|					msg += L"\" doesn't exist.";
   382|				}
   383|				else
   384|				{
   385|					msg = stringReplace(msg, L"$STR_REPLACE1$", longFileName);
   386|					msg = stringReplace(msg, L"$STR_REPLACE2$", longFileDir);
   387|				}
   388|				::MessageBox(_pPublicInterface->getHSelf(), msg.c_str(), title.c_str(), MB_OK);
   389|			}
   390|
   391|			if (!isCreateFileSuccessful)
   392|			{
   393|#ifndef	_WIN64
   394|				if (isWow64Off)
   395|				{
   396|					nppParam.safeWow64EnableWow64FsRedirection(TRUE);
   397|					isWow64Off = false;
   398|				}
   399|#endif
   400|				return BUFFER_INVALID;
   401|			}
   402|		}
   403|	}
   404|
   405|    // Notify plugins that current file is about to load
   406|    // Plugins can should use this notification to filter SCN_MODIFIED
   407|	SCNotification scnN{};
   408|    scnN.nmhdr.code = NPPN_FILEBEFORELOAD;
   409|    scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
   410|    scnN.nmhdr.idFrom = 0;
   411|    _pluginsManager.notify(&scnN);
   412|
   413|    if (encoding == -1)
   414|    {
   415|		encoding = getHtmlXmlEncoding(longFileName);
   416|    }
   417|
   418|	BufferID buffer;
   419|	if (isSnapshotMode)
   420|	{
   421|		buffer = MainFileManager.loadFile(longFileName, static_cast<Document>(NULL), encoding, backupFileName, fileNameTimestamp);
   422|
   423|		if (buffer != BUFFER_INVALID)
   424|		{
   425|			// To notify plugins that a snapshot dirty file is loaded on startup
   426|			SCNotification scnN2{};
   427|			scnN2.nmhdr.hwndFrom = 0;
   428|			scnN2.nmhdr.idFrom = (uptr_t)buffer;
   429|			scnN2.nmhdr.code = NPPN_SNAPSHOTDIRTYFILELOADED;
   430|			_pluginsManager.notify(&scnN2);
   431|
   432|			buffer->setLoadedDirty(true);
   433|		}
   434|	}
   435|	else
   436|	{
   437|		buffer = MainFileManager.loadFile(longFileName, static_cast<Document>(NULL), encoding);
   438|	}
   439|
   440|    if (buffer != BUFFER_INVALID)
   441|    {
   442|        _isFileOpening = true;
   443|
   444|        Buffer * buf = MainFileManager.getBufferByID(buffer);
   445|
   446|        // if file is read only, we set the view read only
   447|        if (isReadOnly)
   448|            buf->setUserReadOnly(true);
   449|
   450|        // Notify plugins that current file is about to open
   451|        scnN.nmhdr.code = NPPN_FILEBEFOREOPEN;
   452|        scnN.nmhdr.idFrom = (uptr_t)buffer;
   453|        _pluginsManager.notify(&scnN);
   454|
   455|
   456|        loadBufferIntoView(buffer, currentView());
   457|
   458|        if (_pTrayIco)
   459|        {
   460|            if (_pTrayIco->isInTray())
   461|            {
   462|                ::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
   463|                if (!_pPublicInterface->isPrelaunch())
   464|                    _pTrayIco->doTrayIcon(REMOVE);
   465|                ::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
   466|            }
   467|        }
   468|        PathRemoveFileSpec(longFileName);
   469|        _linkTriggered = true;
   470|        _isFileOpening = false;
   471|
   472|        // Notify plugins that current file is just opened
   473|        scnN.nmhdr.code = NPPN_FILEOPENED;
   474|        _pluginsManager.notify(&scnN);
   475|        if (_pDocumentListPanel)
   476|            _pDocumentListPanel->newItem(buf, currentView());
   477|    }
   478|    else
   479|    {
   480|        if (globbing || doesDirectoryExist(targetFileName.c_str()))
   481|        {
   482|            vector<wstring> fileNames;
   483|            vector<wstring> patterns;
   484|            if (globbing)
   485|            {
   486|                const wchar_t * substring = wcsrchr(targetFileName.c_str(), wchar_t('\\'));
   487|				if (substring)
   488|				{
   489|					size_t pos = substring - targetFileName.c_str();
   490|
   491|					patterns.push_back(substring + 1);
   492|					wstring dir(targetFileName.c_str(), pos + 1); // use char * to evoke:
   493|																   // string (const char* s, size_t n);
   494|																   // and avoid to call (if pass string) :
   495|																   // string (const string& str, size_t pos, size_t len = npos);
   496|
   497|					getMatchedFileNames(dir.c_str(), 0, patterns, fileNames, isRecursive, false);
   498|				}
   499|            }
   500|            else
   501|