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


#include <time.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "Notepad_plus_Window.h"
#include "CustomFileDialog.h"
#include "EncodingMapper.h"
#include "VerticalFileSwitcher.h"
#include "functionListPanel.h"
#include "ReadDirectoryChanges.h"
#include "ReadFileChanges.h"
#include "fileBrowser.h"
#include <tchar.h>
#include <unordered_set>

using namespace std;

DWORD WINAPI Notepad_plus::monitorFileOnChange(void * params)
{
	MonitorInfo *monitorInfo = static_cast<MonitorInfo *>(params);
	Buffer *buf = monitorInfo->_buffer;
	HWND h = monitorInfo->_nppHandle;

	const TCHAR *fullFileName = (const TCHAR *)buf->getFullPathName();

	//The folder to watch :
	WCHAR folderToMonitor[MAX_PATH];
	wcscpy_s(folderToMonitor, fullFileName);

	::PathRemoveFileSpecW(folderToMonitor);
	
	const DWORD dwNotificationFlags = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE;

	// Create the monitor and add directory to watch.
	CReadDirectoryChanges dirChanges;
	dirChanges.AddDirectory(folderToMonitor, true, dwNotificationFlags);

	CReadFileChanges fileChanges;
	fileChanges.AddFile(fullFileName, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE);

	HANDLE changeHandles[] = { buf->getMonitoringEvent(), dirChanges.GetWaitHandle() };

	bool toBeContinued = true;

	while (toBeContinued)
	{
		DWORD waitStatus = ::WaitForMultipleObjects(_countof(changeHandles), changeHandles, FALSE, 250);
		switch (waitStatus)
		{
			case WAIT_OBJECT_0 + 0:
			// Mutex was signaled. User removes this folder or file browser is closed
			{
				toBeContinued = false;
			}
			break;

			case WAIT_OBJECT_0 + 1:
			// We've received a notification in the queue.
			{
				DWORD dwAction;
				generic_string fn;
				// Process all available changes, ignore User actions
				while (dirChanges.Pop(dwAction, fn))
				{
					// Fix monitoring files which are under root problem
					size_t pos = fn.find(TEXT("\\\\"));
					if (pos == 2)
						fn.replace(pos, 2, TEXT("\\"));

					if (OrdinalIgnoreCaseCompareStrings(fullFileName, fn.c_str()) == 0)
					{
						if (dwAction == FILE_ACTION_MODIFIED)
						{
							::PostMessage(h, NPPM_INTERNAL_RELOADSCROLLTOEND, reinterpret_cast<WPARAM>(buf), 0);
						}
						else if ((dwAction == FILE_ACTION_REMOVED) || (dwAction == FILE_ACTION_RENAMED_OLD_NAME))
						{
							// File is deleted or renamed - quit monitoring thread and close file
							::PostMessage(h, NPPM_INTERNAL_STOPMONITORING, reinterpret_cast<WPARAM>(buf), 0);
						}
					}
				}
			}
			break;

			case WAIT_TIMEOUT:
			{
				if (fileChanges.DetectChanges())
					::PostMessage(h, NPPM_INTERNAL_RELOADSCROLLTOEND, reinterpret_cast<WPARAM>(buf), 0);
			}
			break;

			case WAIT_IO_COMPLETION:
				// Nothing to do.
			break;
		}
	}

	// Just for sample purposes. The destructor will
	// call Terminate() automatically.
	dirChanges.Terminate();
	fileChanges.Terminate();
	delete monitorInfo;
	return EXIT_SUCCESS;
}

void resolveLinkFile(generic_string& linkFilePath)
{
	IShellLink* psl;
	WCHAR targetFilePath[MAX_PATH];
	WIN32_FIND_DATA wfd = {};

	HRESULT hres = CoInitialize(NULL);
	if (SUCCEEDED(hres))
	{
		hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
		if (SUCCEEDED(hres))
		{
			IPersistFile* ppf;
			hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
			if (SUCCEEDED(hres))
			{
				// Load the shortcut. 
				hres = ppf->Load(linkFilePath.c_str(), STGM_READ);
				if (SUCCEEDED(hres) && hres != S_FALSE)
				{
					// Resolve the link. 
					hres = psl->Resolve(NULL, 0);
					if (SUCCEEDED(hres) && hres != S_FALSE)
					{
						// Get the path to the link target. 
						hres = psl->GetPath(targetFilePath, MAX_PATH, (WIN32_FIND_DATA*)&wfd, SLGP_SHORTPATH);
						if (SUCCEEDED(hres) && hres != S_FALSE)
						{
							linkFilePath = targetFilePath;
						}
					}
				}
				ppf->Release();
			}
			psl->Release();
		}
		CoUninitialize();
	}
}

BufferID Notepad_plus::doOpen(const generic_string& fileName, bool isRecursive, bool isReadOnly, int encoding, const TCHAR *backupFileName, FILETIME fileNameTimestamp)
{
	const rsize_t longFileNameBufferSize = MAX_PATH; // TODO stop using fixed-size buffer
	if (fileName.size() >= longFileNameBufferSize - 1) // issue with all other sub-routines
		return BUFFER_INVALID;

	generic_string targetFileName = fileName;
	resolveLinkFile(targetFileName);

	//If [GetFullPathName] succeeds, the return value is the length, in TCHARs, of the string copied to lpBuffer, not including the terminating null character.
	//If the lpBuffer buffer is too small to contain the path, the return value [of GetFullPathName] is the size, in TCHARs, of the buffer that is required to hold the path and the terminating null character.
	//If [GetFullPathName] fails for any other reason, the return value is zero.

	NppParameters& nppParam = NppParameters::getInstance();
	TCHAR longFileName[longFileNameBufferSize];

	const DWORD getFullPathNameResult = ::GetFullPathName(targetFileName.c_str(), longFileNameBufferSize, longFileName, NULL);
	if (getFullPathNameResult == 0)
	{
		return BUFFER_INVALID;
	}
	if (getFullPathNameResult > longFileNameBufferSize)
	{
		return BUFFER_INVALID;
	}
	assert(_tcslen(longFileName) == getFullPathNameResult);

	if (_tcschr(longFileName, '~'))
	{
		// ignore the returned value of function due to win64 redirection system
		::GetLongPathName(longFileName, longFileName, longFileNameBufferSize);
	}

	bool isSnapshotMode = backupFileName != NULL && PathFileExists(backupFileName);
	if (isSnapshotMode && !PathFileExists(longFileName)) // UNTITLED
	{
		wcscpy_s(longFileName, targetFileName.c_str());
	}
    _lastRecentFileList.remove(longFileName);

	generic_string fileName2Find;
	generic_string gs_fileName{ targetFileName };


	// "fileName" could be:
	// 1. full file path to open or create
	// 2. "new N" or whatever renamed from "new N" to switch to (if user double-clicks the found entries of untitled documents, which is the results of running commands "Find in current doc" & "Find in opened docs")
	// 3. a file name with relative path to open or create

	// Search case 1 & 2 firstly
	BufferID foundBufID = MainFileManager.getBufferFromName(targetFileName.c_str());

	if (foundBufID == BUFFER_INVALID)
		fileName2Find = longFileName;

	// if case 1 & 2 not found, search case 3
	if (foundBufID == BUFFER_INVALID)
		foundBufID = MainFileManager.getBufferFromName(fileName2Find.c_str());

	// If we found the document, then we don't open the existing doc. We return the found buffer ID instead.
    if (foundBufID != BUFFER_INVALID && !isSnapshotMode)
    {
        if (_pTrayIco)
        {
            if (_pTrayIco->isInTray())
            {
                ::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
                if (!_pPublicInterface->isPrelaunch())
                    _pTrayIco->doTrayIcon(REMOVE);
                ::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
            }
        }
        return foundBufID;
    }

    if (isFileSession(longFileName) && PathFileExists(longFileName))
    {
        fileLoadSession(longFileName);
        return BUFFER_INVALID;
    }

	if (isFileWorkspace(longFileName) && PathFileExists(longFileName))
	{
		nppParam.setWorkSpaceFilePath(0, longFileName);
		// This line switches to Project Panel 1 while starting up Npp
		// and after dragging a workspace file to Npp:
		launchProjectPanel(IDM_VIEW_PROJECT_PANEL_1, &_pProjectPanel_1, 0);
		return BUFFER_INVALID;
	}

    bool isWow64Off = false;
    if (!PathFileExists(longFileName))
    {
        nppParam.safeWow64EnableWow64FsRedirection(FALSE);
        isWow64Off = true;
    }

    bool globbing = wcsrchr(longFileName, TCHAR('*')) || wcsrchr(longFileName, TCHAR('?'));

	if (!isSnapshotMode) // if not backup mode, or backupfile path is invalid
	{
		if (!PathFileExists(longFileName) && !globbing)
		{
			generic_string longFileDir(longFileName);
			PathRemoveFileSpec(longFileDir);

			bool isCreateFileSuccessful = false;
			if (PathFileExists(longFileDir.c_str()))
			{
				int res = _nativeLangSpeaker.messageBox("CreateNewFileOrNot",
					_pPublicInterface->getHSelf(),
					TEXT("\"$STR_REPLACE$\" doesn't exist. Create it?"),
					TEXT("Create new file"),
					MB_YESNO,
					0,
					longFileName);

				if (res == IDYES)
				{
					bool isOK = MainFileManager.createEmptyFile(longFileName);
					if (isOK)
					{
						isCreateFileSuccessful = true;
					}
					else
					{
						_nativeLangSpeaker.messageBox("CreateNewFileError",
							_pPublicInterface->getHSelf(),
							TEXT("Cannot create the file \"$STR_REPLACE$\"."),
							TEXT("Create new file"),
							MB_OK,
							0,
							longFileName);
					}
				}
			}
			else
			{
				generic_string str2display = TEXT("\"");
				str2display += longFileName;
				str2display += TEXT("\" cannot be opened:\nFolder \"");
				str2display += longFileDir;
				str2display += TEXT("\" doesn't exist.");
				::MessageBox(_pPublicInterface->getHSelf(), str2display.c_str(), TEXT("Cannot open file"), MB_OK);
			}

			if (!isCreateFileSuccessful)
			{
				if (isWow64Off)
				{
					nppParam.safeWow64EnableWow64FsRedirection(TRUE);
					isWow64Off = false;
				}
				return BUFFER_INVALID;
			}
		}
	}

    // Notify plugins that current file is about to load
    // Plugins can should use this notification to filter SCN_MODIFIED
    SCNotification scnN;
    scnN.nmhdr.code = NPPN_FILEBEFORELOAD;
    scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
    scnN.nmhdr.idFrom = 0;
    _pluginsManager.notify(&scnN);

    if (encoding == -1)
    {
		encoding = getHtmlXmlEncoding(longFileName);
    }

	BufferID buffer;
	if (isSnapshotMode)
	{
		buffer = MainFileManager.loadFile(longFileName, NULL, encoding, backupFileName, fileNameTimestamp);

		if (buffer != BUFFER_INVALID)
		{
			isSnapshotMode = (backupFileName != NULL && ::PathFileExists(backupFileName));
			if (isSnapshotMode)
			{
				// To notify plugins that a snapshot dirty file is loaded on startup
				SCNotification scnN2;
				scnN2.nmhdr.hwndFrom = 0;
				scnN2.nmhdr.idFrom = (uptr_t)buffer;
				scnN2.nmhdr.code = NPPN_SNAPSHOTDIRTYFILELOADED;
				_pluginsManager.notify(&scnN2);

				buffer->setLoadedDirty(true);
			}
		}
	}
	else
	{
		buffer = MainFileManager.loadFile(longFileName, NULL, encoding);
	}

    if (buffer != BUFFER_INVALID)
    {
        _isFileOpening = true;

        Buffer * buf = MainFileManager.getBufferByID(buffer);

        // if file is read only, we set the view read only
        if (isReadOnly)
            buf->setUserReadOnly(true);

        // Notify plugins that current file is about to open
        scnN.nmhdr.code = NPPN_FILEBEFOREOPEN;
        scnN.nmhdr.idFrom = (uptr_t)buffer;
        _pluginsManager.notify(&scnN);


        loadBufferIntoView(buffer, currentView());

        if (_pTrayIco)
        {
            if (_pTrayIco->isInTray())
            {
                ::ShowWindow(_pPublicInterface->getHSelf(), SW_SHOW);
                if (!_pPublicInterface->isPrelaunch())
                    _pTrayIco->doTrayIcon(REMOVE);
                ::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
            }
        }
        PathRemoveFileSpec(longFileName);
        _linkTriggered = true;
        _isFileOpening = false;

        // Notify plugins that current file is just opened
        scnN.nmhdr.code = NPPN_FILEOPENED;
        _pluginsManager.notify(&scnN);
        if (_pDocumentListPanel)
            _pDocumentListPanel->newItem(buf, currentView());
    }
    else
    {
        if (globbing || ::PathIsDirectory(targetFileName.c_str()))
        {
            vector<generic_string> fileNames;
            vector<generic_string> patterns;
            if (globbing)
            {
                const TCHAR * substring = wcsrchr(targetFileName.c_str(), TCHAR('\\'));
				if (substring)
				{
					size_t pos = substring - targetFileName.c_str();

					patterns.push_back(substring + 1);
					generic_string dir(targetFileName.c_str(), pos + 1); // use char * to evoke:
																   // string (const char* s, size_t n);
																   // and avoid to call (if pass string) :
																   // string (const string& str, size_t pos, size_t len = npos);

					getMatchedFileNames(dir.c_str(), 0, patterns, fileNames, isRecursive, false);
				}
            }
            else
            {
                generic_string fileNameStr = targetFileName;
                if (targetFileName[targetFileName.size() - 1] != '\\')
                    fileNameStr += TEXT("\\");

                patterns.push_back(TEXT("*"));
                getMatchedFileNames(fileNameStr.c_str(), 0, patterns, fileNames, true, false);
            }

            bool ok2Open = true;
            size_t nbFiles2Open = fileNames.size();

            if (nbFiles2Open > 200)
            {
                ok2Open = IDYES == _nativeLangSpeaker.messageBox("NbFileToOpenImportantWarning",
					_pPublicInterface->getHSelf(),
                    TEXT("$INT_REPLACE$ files are about to be opened.\rAre you sure to open them?"),
                    TEXT("Amount of files to open is too large"),
                    MB_YESNO|MB_APPLMODAL,
					static_cast<int32_t>(nbFiles2Open));
            }

            if (ok2Open)
            {
                for (size_t i = 0 ; i < nbFiles2Open ; ++i)
                    doOpen(fileNames[i]);
            }
        }
        else
        {
			_nativeLangSpeaker.messageBox("OpenFileError",
				_pPublicInterface->getHSelf(),
				TEXT("Can not open file \"$STR_REPLACE$\"."),
				TEXT("ERROR"),
				MB_OK,
				0,
				longFileName);

            _isFileOpening = false;

            scnN.nmhdr.code = NPPN_FILELOADFAILED;
            _pluginsManager.notify(&scnN);
        }
    }

    if (isWow64Off)
    {
        nppParam.safeWow64EnableWow64FsRedirection(TRUE);
        //isWow64Off = false;
    }
    return buffer;
}


bool Notepad_plus::doReload(BufferID id, bool alert)
{
	if (alert)
	{
		int answer = _nativeLangSpeaker.messageBox("DocReloadWarning",
			_pPublicInterface->getHSelf(),
			TEXT("Are you sure you want to reload the current file and lose the changes made in Notepad++?"),
			TEXT("Reload"),
			MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL);
		if (answer != IDYES)
			return false;
	}

	//In order to prevent Scintilla from restyling the entire document,
	//an empty Document is inserted during reload if needed.
	bool mainVisisble = (_mainEditView.getCurrentBufferID() == id);
	bool subVisisble = (_subEditView.getCurrentBufferID() == id);
	if (mainVisisble)
	{
		_mainEditView.saveCurrentPos();
		_mainEditView.execute(SCI_SETDOCPOINTER, 0, 0);
	}

	if (subVisisble)
	{
		_subEditView.saveCurrentPos();
		_subEditView.execute(SCI_SETDOCPOINTER, 0, 0);
	}

	if (!mainVisisble && !subVisisble)
	{
		return MainFileManager.reloadBufferDeferred(id);
	}

	bool res = MainFileManager.reloadBuffer(id);
	Buffer * pBuf = MainFileManager.getBufferByID(id);
	if (mainVisisble)
	{
		_mainEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
		_mainEditView.restoreCurrentPosPreStep();
	}

	if (subVisisble)
	{
		_subEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
		_subEditView.restoreCurrentPosPreStep();
	}

	// Once reload is complete, activate buffer which will take care of
	// many settings such as update status bar, clickable link etc.
	activateBuffer(id, currentView(), true);
	return res;
}

bool Notepad_plus::doSave(BufferID id, const TCHAR * filename, bool isCopy)
{
	const int index = MainFileManager.getBufferIndexByID(id);
	if (index == -1)
	{
		_nativeLangSpeaker.messageBox("BufferInvalidWarning",
			_pPublicInterface->getHSelf(),
			TEXT("Cannot save: Buffer is invalid."),
			TEXT("Save failed"),
			MB_OK | MB_ICONWARNING);

		return false;
	}

	SCNotification scnN;
	// Notify plugins that current file is about to be saved
	if (!isCopy)
	{

		scnN.nmhdr.code = NPPN_FILEBEFORESAVE;
		scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
		scnN.nmhdr.idFrom = (uptr_t)id;
		_pluginsManager.notify(&scnN);
	}

	SavingStatus res = MainFileManager.saveBuffer(id, filename, isCopy);

	if (!isCopy)
	{
		scnN.nmhdr.code = NPPN_FILESAVED;
		_pluginsManager.notify(&scnN);
	}

	if (res == SavingStatus::SaveWritingFailed)
	{
		_nativeLangSpeaker.messageBox("NotEnoughRoom4Saving",
			_pPublicInterface->getHSelf(),
			TEXT("Failed to save file.\nIt seems there's not enough space on disk to save file."),
			TEXT("Save failed"),
			MB_OK);
	}
	else if (res == SavingStatus::SaveOpenFailed)
	{
		if (_isAdministrator)
		{
			// Already in admin mode? File is probably locked.
			_nativeLangSpeaker.messageBox("FileLockedWarning",
				_pPublicInterface->getHSelf(),
				TEXT("Please check whether if this file is opened in another program"),
				TEXT("Save failed"),
				MB_OK | MB_ICONWARNING);
		}
		else
		{
			// try to open Notepad++ in admin mode
			bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
			if (isSnapshotMode) // if both rememberSession && backup mode are enabled
			{                   // Open the 2nd Notepad++ instance in Admin mode, then close the 1st instance.
				int openInAdminModeRes = _nativeLangSpeaker.messageBox("OpenInAdminMode",
				_pPublicInterface->getHSelf(),
				TEXT("This file cannot be saved and it may be protected.\rDo you want to launch Notepad++ in Administrator mode?"),
				TEXT("Save failed"),
				MB_YESNO);

				if (openInAdminModeRes == IDYES)
				{
					TCHAR nppFullPath[MAX_PATH];
					::GetModuleFileName(NULL, nppFullPath, MAX_PATH);

					generic_string args = TEXT("-multiInst");
					size_t shellExecRes = (size_t)::ShellExecute(_pPublicInterface->getHSelf(), TEXT("runas"), nppFullPath, args.c_str(), TEXT("."), SW_SHOW);

					// If the function succeeds, it returns a value greater than 32. If the function fails,
					// it returns an error value that indicates the cause of the failure.
					// https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx

					if (shellExecRes <= 32)
					{
						_nativeLangSpeaker.messageBox("OpenInAdminModeFailed",
							_pPublicInterface->getHSelf(),
							TEXT("Notepad++ cannot be opened in Administrator mode."),
							TEXT("Open in Administrator mode failed"),
							MB_OK);
					}
					else
					{
						::SendMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
					}

				}
			}
			else // rememberSession && backup mode are not both enabled
			{    // open only the file to save in Notepad++ of Administrator mode by keeping the current instance.
				int openInAdminModeRes = _nativeLangSpeaker.messageBox("OpenInAdminModeWithoutCloseCurrent",
				_pPublicInterface->getHSelf(),
				TEXT("The file cannot be saved and it may be protected.\rDo you want to launch Notepad++ in Administrator mode?"),
				TEXT("Save failed"),
				MB_YESNO);

				if (openInAdminModeRes == IDYES)
				{
					TCHAR nppFullPath[MAX_PATH];
					::GetModuleFileName(NULL, nppFullPath, MAX_PATH);

					BufferID bufferID = _pEditView->getCurrentBufferID();
					Buffer * buf = MainFileManager.getBufferByID(bufferID);

					//process the fileNamePath into LRF
					generic_string fileNamePath = buf->getFullPathName();

					generic_string args = TEXT("-multiInst -nosession ");
					args += TEXT("\"");
					args += fileNamePath;
					args += TEXT("\"");
					size_t shellExecRes = (size_t)::ShellExecute(_pPublicInterface->getHSelf(), TEXT("runas"), nppFullPath, args.c_str(), TEXT("."), SW_SHOW);

					// If the function succeeds, it returns a value greater than 32. If the function fails,
					// it returns an error value that indicates the cause of the failure.
					// https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx

					if (shellExecRes <= 32)
					{
						_nativeLangSpeaker.messageBox("OpenInAdminModeFailed",
							_pPublicInterface->getHSelf(),
							TEXT("Notepad++ cannot be opened in Administrator mode."),
							TEXT("Open in Administrator mode failed"),
							MB_OK);
					}
				}
			}

		}
	}

	if (res == SavingStatus::SaveOK && _pFuncList && (!_pFuncList->isClosed()) && _pFuncList->isVisible())
	{
		_pFuncList->reload();
	}

	return res == SavingStatus::SaveOK;
}

void Notepad_plus::doClose(BufferID id, int whichOne, bool doDeleteBackup)
{
	DocTabView *tabToClose = (whichOne == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
	int i = tabToClose->getIndexByBuffer(id);
	if (i == -1)
		return;

	size_t numInitialOpenBuffers =
		((_mainWindowStatus & WindowMainActive) == WindowMainActive ? _mainDocTab.nbItem() : 0) +
		((_mainWindowStatus & WindowSubActive) == WindowSubActive ? _subDocTab.nbItem() : 0);

	if (doDeleteBackup)
		MainFileManager.deleteBufferBackup(id);

	Buffer * buf = MainFileManager.getBufferByID(id);

	// Notify plugins that current file is about to be closed
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_FILEBEFORECLOSE;
	scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
	scnN.nmhdr.idFrom = (uptr_t)id;
	_pluginsManager.notify(&scnN);

	// Add to recent file history only if file is removed from all the views
	// There might be cases when file is cloned/moved to view.
	// Don't add to recent list unless file is removed from all the views
	generic_string fileFullPath;
	if (!buf->isUntitled())
	{
		// if the file doesn't exist, it could be redirected
		// So we turn Wow64 off
		bool isWow64Off = false;
		NppParameters& nppParam = NppParameters::getInstance();
		const TCHAR *fn = buf->getFullPathName();
		if (!PathFileExists(fn))
		{
			nppParam.safeWow64EnableWow64FsRedirection(FALSE);
			isWow64Off = true;
		}

		if (PathFileExists(buf->getFullPathName()))
			fileFullPath = buf->getFullPathName();

		// We enable Wow64 system, if it was disabled
		if (isWow64Off)
		{
			nppParam.safeWow64EnableWow64FsRedirection(TRUE);
			//isWow64Off = false;
		}
	}

	size_t nbDocs = whichOne==MAIN_VIEW?(_mainDocTab.nbItem()):(_subDocTab.nbItem());

	if (buf->isMonitoringOn())
	{
		// turn off monitoring
		monitoringStartOrStopAndUpdateUI(buf, false);
	}

	//Do all the works
	bool isBufRemoved = removeBufferFromView(id, whichOne);
	BufferID hiddenBufferID = BUFFER_INVALID;
	if (nbDocs == 1 && canHideView(whichOne))
	{	//close the view if both visible
		hideView(whichOne);

		// if the current activated buffer is in this view,
		// then get buffer ID to remove the entry from File Switcher Pannel
		hiddenBufferID = reinterpret_cast<BufferID>(::SendMessage(_pPublicInterface->getHSelf(), NPPM_GETBUFFERIDFROMPOS, 0, whichOne));
		if (!isBufRemoved && hiddenBufferID != BUFFER_INVALID && _pDocumentListPanel)
			_pDocumentListPanel->closeItem(hiddenBufferID, whichOne);
	}

	// Notify plugins that current file is closed
	if (isBufRemoved)
	{
		scnN.nmhdr.code = NPPN_FILECLOSED;
		_pluginsManager.notify(&scnN);

		// The document could be clonned.
		// if the same buffer ID is not found then remove the entry from File Switcher Panel
		if (_pDocumentListPanel)
		{
			_pDocumentListPanel->closeItem(id, whichOne);

			if (hiddenBufferID != BUFFER_INVALID)
				_pDocumentListPanel->closeItem(hiddenBufferID, whichOne);
		}

		// Add to recent file only if file is removed and does not exist in any of the views
		BufferID buffID = MainFileManager.getBufferFromName(fileFullPath.c_str());
		if (buffID == BUFFER_INVALID && fileFullPath.length() > 0)
			_lastRecentFileList.add(fileFullPath.c_str());
	}
	command(IDM_VIEW_REFRESHTABAR);

	if (NppParameters::getInstance().getNppGUI()._tabStatus & TAB_QUITONEMPTY)
	{
		// the user closed the last open tab
		if (numInitialOpenBuffers == 1 && isEmpty() && !_isAttemptingCloseOnQuit)
		{
			command(IDM_FILE_EXIT);
		}
	}

	return;
}

generic_string Notepad_plus::exts2Filters(const generic_string& exts, int maxExtsLen) const
{
	const TCHAR *extStr = exts.c_str();
	TCHAR aExt[MAX_PATH];
	generic_string filters(TEXT(""));

	int j = 0;
	bool stop = false;

	for (size_t i = 0, len = exts.length(); i < len && j < MAX_PATH - 1; ++i)
	{
		if (extStr[i] == ' ')
		{
			if (!stop)
			{
				aExt[j] = '\0';
				stop = true;

				if (aExt[0])
				{
					filters += TEXT("*.");
					filters += aExt;
					filters += TEXT(";");
				}
				j = 0;

				if (maxExtsLen != -1 && i >= static_cast<size_t>(maxExtsLen))
				{
					filters += TEXT(" ... ");
					break;
				}
			}
		}
		else
		{
			aExt[j] = extStr[i];
			stop = false;
			++j;
		}
	}

	if (j > 0)
	{
		aExt[j] = '\0';
		if (aExt[0])
		{
			filters += TEXT("*.");
			filters += aExt;
			filters += TEXT(";");
		}
	}

	// remove the last ';'
    filters = filters.substr(0, filters.length()-1);
	return filters;
}

int Notepad_plus::setFileOpenSaveDlgFilters(CustomFileDialog & fDlg, bool showAllExt, int langType)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )nppParam.getNppGUI();

	int i = 0;
	Lang *l = NppParameters::getInstance().getLangFromIndex(i++);

    int ltIndex = 0;
    bool ltFound = false;
	while (l)
	{
		LangType lid = l->getLangID();

		bool inExcludedList = false;

		for (size_t j = 0, len = nppGUI._excludedLangList.size() ; j < len ; ++j)
		{
			if (lid == nppGUI._excludedLangList[j]._langType)
			{
				inExcludedList = true;
				break;
			}
		}

		if (!inExcludedList)
		{
			const TCHAR *defList = l->getDefaultExtList();
			const TCHAR *userList = NULL;

			LexerStylerArray &lsa = (NppParameters::getInstance()).getLStylerArray();
			const TCHAR *lName = l->getLangName();
			LexerStyler *pLS = lsa.getLexerStylerByName(lName);

			if (pLS)
				userList = pLS->getLexerUserExt();

			generic_string list(TEXT(""));
			if (defList)
				list += defList;
			if (userList)
			{
				list += TEXT(" ");
				list += userList;
			}

			generic_string stringFilters = exts2Filters(list, showAllExt ? -1 : 40);
			const TCHAR *filters = stringFilters.c_str();
			if (filters[0])
			{
				fDlg.setExtFilter(getLangDesc(lid, false).c_str(), filters);

                //
                // Get index of lang type to find
                //
                if (langType != -1 && !ltFound)
                {
                    ltFound = langType == lid;
                }

                if (langType != -1 && !ltFound)
                {
                    ++ltIndex;
                }
			}
		}
		l = (NppParameters::getInstance()).getLangFromIndex(i++);
	}

    if (!ltFound)
        return -1;
    return ltIndex;
}


bool Notepad_plus::fileClose(BufferID id, int curView)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager.getBufferByID(bufferID);

	int res;

	//process the fileNamePath into LRF
	const TCHAR *fileNamePath = buf->getFullPathName();

	if (buf->isUntitled() && buf->docLength() == 0)
	{
		// Do nothing
	}
	else if (buf->isDirty())
	{
		res = doSaveOrNot(fileNamePath);
		if (res == IDYES)
		{
			if (!fileSave(id)) // the cancel button of savedialog is pressed, aborts closing
				return false;
		}
		else if (res == IDCANCEL)
		{
			return false;	//cancel aborts closing
		}
		else
		{
			// else IDNO we continue
		}
	}

	int viewToClose = currentView();
	if (curView != -1)
		viewToClose = curView;

	bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
	doClose(bufferID, viewToClose, isSnapshotMode);
	return true;
}

bool Notepad_plus::fileCloseAll(bool doDeleteBackup, bool isSnapshotMode)
{
	bool noSaveToAll = false;
	bool saveToAll = false;

	//closes all documents, makes the current view the only one visible

	//first check if we need to save any file
	//check in the both view
	std::unordered_set<BufferID> uniqueBuffers;

	for (size_t i = 0; i < _mainDocTab.nbItem() && !noSaveToAll; ++i)
	{
		BufferID id = _mainDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager.getBufferByID(id);

		// Put all the BufferID from main vaiew to hash table
		// hash table is used for fast searching
		uniqueBuffers.insert(id);

		if (buf->isUntitled() && buf->docLength() == 0)
		{
			// Do nothing
		}
		else if (buf->isDirty())
		{
			if (isSnapshotMode)
			{
				if (buf->getBackupFileName() == TEXT("") || !::PathFileExists(buf->getBackupFileName().c_str())) //backup file has been deleted from outside
				{
					// warning user and save it if user want it.
					activateBuffer(id, MAIN_VIEW);
					if (!activateBuffer(id, SUB_VIEW))
						switchEditViewTo(MAIN_VIEW);

					int res = _nativeLangSpeaker.messageBox("NoBackupDoSaveFile",
						_pPublicInterface->getHSelf(),
						TEXT("Your backup file cannot be found (deleted from outside).\rSave it otherwise your data will be lost\rDo you want to save file \"$STR_REPLACE$\" ?"),
						TEXT("Save"),
						MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL,
						0, // not used
						buf->getFullPathName());

					if (res == IDYES)
					{
						if (!fileSave(id))
							return false;	//abort entire procedure
					}
					else if (res == IDCANCEL)
					{
						return false;
					}
				}
			}
			else
			{
				activateBuffer(id, MAIN_VIEW);
				if (!activateBuffer(id, SUB_VIEW))
					switchEditViewTo(MAIN_VIEW);

				int res = -1;
				if (saveToAll)
				{
					res = IDYES;
				}
				else
				{
					size_t nbDirtyFiles = MainFileManager.getNbDirtyBuffers();
					res = doSaveOrNot(buf->getFullPathName(), nbDirtyFiles > 1);
				}

				if (res == IDYES)
				{
					if (!fileSave(id))
						return false;	//abort entire procedure
				}
				else if (res == IDCANCEL)
				{
					return false;
				}
				else if (res == IDIGNORE)
				{
					noSaveToAll = true;
				}
				else if (res == IDRETRY)
				{
					if (!fileSave(id))
						return false;	// Abort entire procedure.

					saveToAll = true;
				}
			}
		}
	}

	for (size_t i = 0; i < _subDocTab.nbItem() && !noSaveToAll; ++i)
	{
		BufferID id = _subDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager.getBufferByID(id);

		// Is this buffer already included?
		if (uniqueBuffers.find(id) != uniqueBuffers.end())
			continue;

		if (buf->isUntitled() && buf->docLength() == 0)
		{
			// Do nothing
		}
		else if (buf->isDirty())
		{
			if (isSnapshotMode)
			{
				if (buf->getBackupFileName() == TEXT("") || !::PathFileExists(buf->getBackupFileName().c_str())) //backup file has been deleted from outside
				{
					// warning user and save it if user want it.
					activateBuffer(id, SUB_VIEW);
					switchEditViewTo(SUB_VIEW);

					int res = _nativeLangSpeaker.messageBox("NoBackupDoSaveFile",
						_pPublicInterface->getHSelf(),
						TEXT("Your backup file cannot be found (deleted from outside).\rSave it otherwise your data will be lost\rDo you want to save file \"$STR_REPLACE$\" ?"),
						TEXT("Save"),
						MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL,
						0, // not used
						buf->getFullPathName());

					if (res == IDYES)
					{
						if (!fileSave(id))
							return false;	//abort entire procedure
					}
					else if (res == IDCANCEL)
					{
						return false;
					}
				}
			}
			else
			{
				activateBuffer(id, SUB_VIEW);
				switchEditViewTo(SUB_VIEW);

				int res = -1;
				if (saveToAll)
				{
					res = IDYES;
				}
				else
				{
					size_t nbDirtyFiles = MainFileManager.getNbDirtyBuffers();
					res = doSaveOrNot(buf->getFullPathName(), nbDirtyFiles > 1);
				}

				if (res == IDYES)
				{
					if (!fileSave(id))
						return false;	//abort entire procedure
				}
				else if (res == IDCANCEL)
				{
					return false;
					//otherwise continue (IDNO)
				}
				else if (res == IDIGNORE)
				{
					noSaveToAll = true;
				}
				else if (res == IDRETRY)
				{
					if (!fileSave(id))
						return false;	// Abort entire procedure.

					saveToAll = true;
				}
			}
		}
	}

	//Then start closing, inactive view first so the active is left open
    if (bothActive())
    {
		//first close all docs in non-current view, which gets closed automatically
		//Set active tab to the last one closed.
		activateBuffer(_pNonDocTab->getBufferByIndex(0), otherView());
		for (int32_t i = static_cast<int32_t>(_pNonDocTab->nbItem()) - 1; i >= 0; i--) //close all from right to left
		{
			doClose(_pNonDocTab->getBufferByIndex(i), otherView(), doDeleteBackup);
		}
    }

	activateBuffer(_pDocTab->getBufferByIndex(0), currentView());
	for (int32_t i = static_cast<int32_t>(_pDocTab->nbItem()) - 1; i >= 0; i--)
	{	//close all from right to left
		doClose(_pDocTab->getBufferByIndex(i), currentView(), doDeleteBackup);
	}
	return true;
}

bool Notepad_plus::fileCloseAllGiven(const std::vector<int>& krvecBufferIndexes)
{
	// First check if we need to save any file.

	bool noSaveToAll = false;
	bool saveToAll = false;
	std::vector<int> bufferIndexesToClose;

	// Count the number of dirty file
	size_t nbDirtyFiles = 0;
	for (const auto& index : krvecBufferIndexes)
	{
		BufferID id = _pDocTab->getBufferByIndex(index);
		Buffer* buf = MainFileManager.getBufferByID(id);

		if (buf->isDirty())
			++nbDirtyFiles;
	}

	for (const auto& index : krvecBufferIndexes)
	{
		BufferID id = _pDocTab->getBufferByIndex(index);
		Buffer* buf = MainFileManager.getBufferByID(id);

		if ((buf->isUntitled() && buf->docLength() == 0) || noSaveToAll || !buf->isDirty())
		{
			// Do nothing, these documents are already ready to close
			bufferIndexesToClose.push_back(index);
		}
		else if (buf->isDirty() && !noSaveToAll)
		{
			if (_activeView == MAIN_VIEW)
			{
				activateBuffer(id, MAIN_VIEW);
				if (!activateBuffer(id, SUB_VIEW))
					switchEditViewTo(MAIN_VIEW);
			}
			else
			{
				activateBuffer(id, SUB_VIEW);
				switchEditViewTo(SUB_VIEW);
			}

			/* Do you want to save
			*	IDYES		: Yes
			*	IDRETRY		: Yes to All
			*	IDNO		: No
			*	IDIGNORE	: No To All
			*	IDCANCEL	: Cancel Opration
			*/			

			int res = saveToAll ? IDYES : doSaveOrNot(buf->getFullPathName(), nbDirtyFiles > 1);

			if (res == IDYES || res == IDRETRY)
			{
				if (!fileSave(id))
					break;	// Abort entire procedure, but close whatever processed so far

				bufferIndexesToClose.push_back(index);

				if (res == IDRETRY)
					saveToAll = true;
			}
			else if (res == IDNO || res == IDIGNORE)
			{
				bufferIndexesToClose.push_back(index);

				if (res == IDIGNORE)
					noSaveToAll = true;
			}
			else if (res == IDCANCEL)
			{
				break;
			}
		}
	}

	// Now we close.
	bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
	for (const auto& index : bufferIndexesToClose)
	{
		doClose(_pDocTab->getBufferByIndex(index), currentView(), isSnapshotMode);
	}

	return true;
}

bool Notepad_plus::fileCloseAllToLeft()
{
	// Indexes must go from high to low to deal with the fact that when one index is closed, any remaining
	// indexes (smaller than the one just closed) will point to the wrong tab.
	std::vector<int> vecIndexesToClose;
	for (int i = _pDocTab->getCurrentTabIndex() - 1; i >= 0; i--)
	{
		vecIndexesToClose.push_back(i);
	}
	return fileCloseAllGiven(vecIndexesToClose);
}

bool Notepad_plus::fileCloseAllToRight()
{
	// Indexes must go from high to low to deal with the fact that when one index is closed, any remaining
	// indexes (smaller than the one just closed) will point to the wrong tab.
	const int kiActive = _pDocTab->getCurrentTabIndex();
	std::vector<int> vecIndexesToClose;
	for (int i = int(_pDocTab->nbItem()) - 1; i > kiActive; i--)
	{
		vecIndexesToClose.push_back(i);
	}
	return fileCloseAllGiven(vecIndexesToClose);
}

bool Notepad_plus::fileCloseAllUnchanged()
{
	// Indexes must go from high to low to deal with the fact that when one index is closed, any remaining
	// indexes (smaller than the one just closed) will point to the wrong tab.
	std::vector<int> vecIndexesToClose;

	for (int i = int(_pDocTab->nbItem()) - 1; i >= 0; i--)
	{
		BufferID id = _pDocTab->getBufferByIndex(i);
		Buffer* buf = MainFileManager.getBufferByID(id);
		if ((buf->isUntitled() && buf->docLength() == 0) || !buf->isDirty())
		{
			vecIndexesToClose.push_back(i);
		}
	}

	return fileCloseAllGiven(vecIndexesToClose);
}

bool Notepad_plus::fileCloseAllButCurrent()
{
	BufferID current = _pEditView->getCurrentBufferID();
	const int activeViewID = currentView();
	int active = _pDocTab->getCurrentTabIndex();
	bool noSaveToAll = false;
	bool saveToAll = false;
	std::vector<unsigned int> mainSaveOpIndex, subSaveOpIndex;

	bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();

	//closes all documents, makes the current view the only one visible

	//first check if we need to save any file
	for (size_t i = 0; i < _mainDocTab.nbItem() && !noSaveToAll; ++i)
	{
		BufferID id = _mainDocTab.getBufferByIndex(i);
		if (id == current)
			continue;

		Buffer * buf = MainFileManager.getBufferByID(id);
		if (buf->isUntitled() && buf->docLength() == 0)
		{
			// Do nothing
		}
		else if (buf->isDirty())
		{
			activateBuffer(id, MAIN_VIEW);
			if (!activateBuffer(id, SUB_VIEW))
				switchEditViewTo(MAIN_VIEW);

			int res = -1;
			if (saveToAll)
			{
				res = IDYES;
			}
			else
			{
				size_t nbDirtyFiles = MainFileManager.getNbDirtyBuffers();

				// if current file is dirty, if should be removed from dirty files to make nbDirtyFiles accurate
				Buffer* currentBuf = MainFileManager.getBufferByID(current);
				nbDirtyFiles -= currentBuf->isDirty() ? 1 : 0;

				res = doSaveOrNot(buf->getFullPathName(), nbDirtyFiles > 1);
			}

			if (res == IDYES)
			{
				bool isSaved = fileSave(id);
				if (isSaved)
					mainSaveOpIndex.push_back((unsigned int)i);
				else
					res = IDCANCEL;	//about to abort entire procedure
			}
			else if (res == IDNO)
			{
				mainSaveOpIndex.push_back((unsigned int)i);
			}
			else if (res == IDIGNORE)
			{
				noSaveToAll = true;
			}
			else if (res == IDRETRY)
			{
				if (!fileSave(id))
					return false;	// Abort entire procedure.

				saveToAll = true;
			}


			if (res == IDCANCEL)
			{
				for (int32_t j = static_cast<int32_t>(mainSaveOpIndex.size()) - 1; j >= 0; j--) 	//close all from right to left
				{
					doClose(_mainDocTab.getBufferByIndex(mainSaveOpIndex[j]), MAIN_VIEW, isSnapshotMode);
				}

				return false;
			}

		}
	}

	for (size_t i = 0; i < _subDocTab.nbItem() && !noSaveToAll; ++i)
	{
		BufferID id = _subDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager.getBufferByID(id);
		if (id == current)
			continue;

		if (buf->isUntitled() && buf->docLength() == 0)
		{
			// Do nothing
		}
		else if (buf->isDirty())
		{
			activateBuffer(id, SUB_VIEW);
			switchEditViewTo(SUB_VIEW);

			int res = -1;
			if (saveToAll)
			{
				res = IDYES;
			}
			else
			{
				size_t nbDirtyFiles = MainFileManager.getNbDirtyBuffers();

				// if current file is dirty, if should be removed from dirty files to make nbDirtyFiles accurate
				Buffer* currentBuf = MainFileManager.getBufferByID(current);
				nbDirtyFiles -= currentBuf->isDirty() ? 1 : 0;

				res = doSaveOrNot(buf->getFullPathName(), nbDirtyFiles > 1);
			}


			if (res == IDYES)
			{
				bool isSaved = fileSave(id);
				if (isSaved)
					subSaveOpIndex.push_back((unsigned int)i);
				else
					res = IDCANCEL;	//about to abort entire procedure
			}
			else if (res == IDNO)
			{
				subSaveOpIndex.push_back((unsigned int)i);
			}
			else if (res == IDIGNORE)
			{
				noSaveToAll = true;
			}
			else if (res == IDRETRY)
			{
				if (!fileSave(id))
					return false;	// Abort entire procedure.

				saveToAll = true;
			}

			
			if (res == IDCANCEL)
			{
				for (int32_t j = static_cast<int32_t>(mainSaveOpIndex.size()) - 1; j >= 0; j--) 	//close all from right to left
				{
					doClose(_mainDocTab.getBufferByIndex(mainSaveOpIndex[j]), MAIN_VIEW, isSnapshotMode);
				}

				for (int32_t j = static_cast<int32_t>(subSaveOpIndex.size()) - 1; j >= 0; j--) 	//close all from right to left
				{
					doClose(_subDocTab.getBufferByIndex(subSaveOpIndex[j]), SUB_VIEW, isSnapshotMode);
				}
				return false;
			}

		}
	}

	// We may have to restore previous view after saving new files
	switchEditViewTo(activeViewID);

	//Then start closing, inactive view first so the active is left open
    if (bothActive())
    {
		//first close all docs in non-current view, which gets closed automatically
		//Set active tab to the last one closed.
		const int viewNo = otherView();
		activateBuffer(_pNonDocTab->getBufferByIndex(0), viewNo);

		for (int32_t i = static_cast<int32_t>(_pNonDocTab->nbItem()) - 1; i >= 0; i--) 	//close all from right to left
		{
			doClose(_pNonDocTab->getBufferByIndex(i), viewNo, isSnapshotMode);
		}
    }

	const int viewNo = currentView();
	size_t nbItems = _pDocTab->nbItem();
	activateBuffer(_pDocTab->getBufferByIndex(0), viewNo);
	
	// After activateBuffer() call, if file is deleteed, user will decide to keep or not the tab
	// So here we check if the 1st tab is closed or not
	size_t newNbItems = _pDocTab->nbItem();

	if (nbItems > newNbItems) // the 1st tab has been removed
	{
		// active tab move 1 position forward
		active -= 1;
	}

	for (int32_t i = static_cast<int32_t>(newNbItems) - 1; i >= 0; i--)	//close all from right to left
	{
		if (i == active)	//dont close active index
		{
			continue;
		}
		doClose(_pDocTab->getBufferByIndex(i), viewNo, isSnapshotMode);
	}
	return true;
}

bool Notepad_plus::fileSave(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager.getBufferByID(bufferID);

	if (!buf->getFileReadOnly() && buf->isDirty())	//cannot save if readonly
	{
		if (buf->isUntitled())
		{
			return fileSaveAs(bufferID);
		}

		const NppGUI & nppgui = (NppParameters::getInstance()).getNppGUI();
		BackupFeature backup = nppgui._backup;

		if (backup != bak_none && !buf->isLargeFile())
		{
			const TCHAR *fn = buf->getFullPathName();
			TCHAR *name = ::PathFindFileName(fn);
			generic_string fn_bak;

			if (nppgui._useDir && !nppgui._backupDir.empty())
			{
				// Get the custom directory, make sure it has a trailing slash
				fn_bak = nppgui._backupDir;
				if (fn_bak.back() != TEXT('\\'))
					fn_bak += TEXT("\\");
			}
			else
			{
				// Get the current file's directory
				generic_string path = fn;
				::PathRemoveFileSpec(path);
				fn_bak = path.c_str();
				fn_bak += TEXT("\\");

				// If verbose, save it in a sub folder
				if (backup == bak_verbose)
				{
					fn_bak += TEXT("nppBackup\\");
				}
			}

			// Expand any environment variables
			TCHAR fn_bak_expanded[MAX_PATH] = { '\0' };
			::ExpandEnvironmentStrings(fn_bak.c_str(), fn_bak_expanded, MAX_PATH);
			fn_bak = fn_bak_expanded;

			// Make sure the directory exists
			if (!::PathFileExists(fn_bak.c_str()))
			{
				SHCreateDirectory(NULL, fn_bak.c_str());
			}

			// Determine what to name the backed-up file
			if (backup == bak_simple)
			{
				fn_bak += name;
				fn_bak += TEXT(".bak");
			}
			else if (backup == bak_verbose)
			{
				const int temBufLen = 32;
				TCHAR tmpbuf[temBufLen];
				time_t ltime = time(0);
				struct tm *today;

				today = localtime(&ltime);
				if (today)
				{
					generic_strftime(tmpbuf, temBufLen, TEXT("%Y-%m-%d_%H%M%S"), today);

					fn_bak += name;
					fn_bak += TEXT(".");
					fn_bak += tmpbuf;
					fn_bak += TEXT(".bak");
				}
			}

			BOOL doCancel = FALSE;
			if (!::CopyFileEx(fn, fn_bak.c_str(), nullptr, nullptr, &doCancel, COPY_FILE_NO_BUFFERING))
			{
				int res = _nativeLangSpeaker.messageBox("FileBackupFailed",
					_pPublicInterface->getHSelf(),
					TEXT("The previous version of the file could not be saved into the backup directory at \"$STR_REPLACE$\".\r\rDo you want to save the current file anyways?"),
					TEXT("File Backup Failed"),
					MB_YESNO | MB_ICONERROR,
					0,
					fn_bak.c_str());

				if (res == IDNO)
				{
					return false;
				}
			}
		}

		return doSave(bufferID, buf->getFullPathName(), false);
	}
	return false;
}

bool Notepad_plus::fileSaveSpecific(const generic_string& fileNameToSave)
{
	BufferID idToSave = _mainDocTab.findBufferByName(fileNameToSave.c_str());
	if (idToSave == BUFFER_INVALID)
	{
		idToSave = _subDocTab.findBufferByName(fileNameToSave.c_str());
	}
    //do not use else syntax, id might be taken from sub doc tab, 
    //in which case fileSave needs to be executed also
	if (idToSave != BUFFER_INVALID)
	{
		fileSave(idToSave);
		checkDocState();
		return true;
	}
	else
	{
		return false;
	}
}

bool Notepad_plus::fileSaveAllConfirm()
{
	bool confirmed = false;

	if (NppParameters::getInstance().getNppGUI()._saveAllConfirm)
	{
		int answer = doSaveAll();

		if (answer == IDYES)
		{
			confirmed = true;
		}

		if (answer == IDRETRY)
		{
			NppParameters::getInstance().getNppGUI()._saveAllConfirm = false;
			//uncheck the "Enable save all confirm dialog" checkbox in Preference-> MISC settings
			_preference._miscSubDlg.setChecked(IDC_CHECK_SAVEALLCONFIRM, false);
			confirmed = true;
		}
	}
	else
	{
		confirmed = true;
	}

	return confirmed;
}

size_t Notepad_plus::getNbDirtyBuffer(int view)
{
	if (view != MAIN_VIEW && view != SUB_VIEW)
		return 0;
	DocTabView* pDocTabView = &_mainDocTab;
	if (view == SUB_VIEW)
		pDocTabView = &_subDocTab;

	size_t count = 0;
	for (size_t i = 0; i < pDocTabView->nbItem(); ++i)
	{
		BufferID id = pDocTabView->getBufferByIndex(i);
		Buffer* buf = MainFileManager.getBufferByID(id);

		if (buf->isDirty())
		{
			++count;
		}
	}
	return count;
}

bool Notepad_plus::fileSaveAll()
{
	size_t nbDirty = getNbDirtyBuffer(MAIN_VIEW) + getNbDirtyBuffer(SUB_VIEW);

	if (!nbDirty)
		return false;

	Buffer* curBuf = _pEditView->getCurrentBuffer();
	if (nbDirty == 1 && curBuf->isDirty())
	{
		fileSave(curBuf);
	}
	else if (fileSaveAllConfirm())
	{
		if (viewVisible(MAIN_VIEW))
		{
			for (size_t i = 0; i < _mainDocTab.nbItem(); ++i)
			{
				BufferID idToSave = _mainDocTab.getBufferByIndex(i);
				fileSave(idToSave);
			}
		}

		if (viewVisible(SUB_VIEW))
		{
			for (size_t i = 0; i < _subDocTab.nbItem(); ++i)
			{
				BufferID idToSave = _subDocTab.getBufferByIndex(i);
				fileSave(idToSave);
			}
		}
		checkDocState();
	}
	return true;
}

bool Notepad_plus::fileSaveAs(BufferID id, bool isSaveCopy)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager.getBufferByID(bufferID);

	generic_string origPathname = buf->getFullPathName();
	bool wasUntitled = buf->isUntitled();

	CustomFileDialog fDlg(_pPublicInterface->getHSelf());

	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"));

	LangType langType = buf->getLangType();

	const bool defaultAllTypes = NppParameters::getInstance().getNppGUI()._setSaveDlgExtFiltToAllTypes;
	const int langTypeIndex = setFileOpenSaveDlgFilters(fDlg, false, langType);
	
	fDlg.setDefFileName(buf->getFileName());

	fDlg.setExtIndex(langTypeIndex + 1); // +1 for "All types"

	const generic_string checkboxLabel = _nativeLangSpeaker.getLocalizedStrFromID("file-save-assign-type",
		TEXT("&Append extension"));
	fDlg.enableFileTypeCheckbox(checkboxLabel, !defaultAllTypes);

	// Disable file autodetection before opening save dialog to prevent use-after-delete bug.
	NppParameters& nppParam = NppParameters::getInstance();
	auto cdBefore = nppParam.getNppGUI()._fileAutoDetection;
	(nppParam.getNppGUI())._fileAutoDetection = cdDisabled;

	generic_string fn = fDlg.doSaveDlg();

	// Remember the selected state
	(nppParam.getNppGUI())._setSaveDlgExtFiltToAllTypes = !fDlg.getFileTypeCheckboxValue();

	// Enable file autodetection again.
	(nppParam.getNppGUI())._fileAutoDetection = cdBefore;

	if (!fn.empty())
	{
		BufferID other = _pDocTab->findBufferByName(fn.c_str());
		if (other == BUFFER_INVALID)
			other = _pNonDocTab->findBufferByName(fn.c_str());

		if (other == BUFFER_INVALID // both (current and other) views don't contain the same file (path), "Save As" action is allowed.
			|| other->getID() == bufferID->getID()) // or if user saves the file by using "Save As" instead of "Save" (ie. in "Save As" dialog to chooses the same file), then it's still legit.
		{
			bool res = doSave(bufferID, fn.c_str(), isSaveCopy);
			//Changing lexer after save seems to work properly
			if (!wasUntitled && !isSaveCopy)
			{
				_lastRecentFileList.add(origPathname.c_str());
			}

			// If file is replaced then remove it from recent list
			if (res && !isSaveCopy)
			{
				_lastRecentFileList.remove(fn.c_str());
			}

			return res;
		}
		else		//cannot save, other view has buffer already open, activate it
		{
			_nativeLangSpeaker.messageBox("FileAlreadyOpenedInNpp",
				_pPublicInterface->getHSelf(),
				TEXT("The file is already opened in Notepad++."),
				TEXT("ERROR"),
				MB_OK | MB_ICONSTOP);
			switchToFile(other);
			return false;
		}
	}
	else // cancel button is pressed
	{
		checkModifiedDocument(true);
		return false;
	}
}

bool Notepad_plus::fileRename(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager.getBufferByID(bufferID);

	SCNotification scnN;
	scnN.nmhdr.code = NPPN_FILEBEFORERENAME;
	scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
	scnN.nmhdr.idFrom = (uptr_t)bufferID;
	_pluginsManager.notify(&scnN);

	bool success = false;
	bool isFileExisting = PathFileExists(buf->getFullPathName()) != FALSE;
	if (isFileExisting)
	{
		CustomFileDialog fDlg(_pPublicInterface->getHSelf());

		fDlg.setExtFilter(TEXT("All types"), TEXT(".*"));
		setFileOpenSaveDlgFilters(fDlg, false);
		fDlg.setFolder(buf->getFullPathName());
		fDlg.setDefFileName(buf->getFileName());

		generic_string title = _nativeLangSpeaker.getLocalizedStrFromID("file-rename-title", TEXT("Rename"));
		fDlg.setTitle(title.c_str());

		generic_string fn = fDlg.doSaveDlg();

		if (!fn.empty())
			success = MainFileManager.moveFile(bufferID, fn.c_str());
	}
	else
	{
		// We are just going to rename the tab nothing else
		// So just rename the tab and rename the backup file too if applicable

		// https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
		// Reserved characters: < > : " / \ | ? *
		std::wstring reservedChars = TEXT("<>:\"/\\|\?*");

		generic_string staticName = _nativeLangSpeaker.getLocalizedStrFromID("tabrename-newname", TEXT("New Name: "));

		StringDlg strDlg;
		generic_string title = _nativeLangSpeaker.getLocalizedStrFromID("tabrename-title", TEXT("Rename Current Tab"));
		strDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), title.c_str(), staticName.c_str(), buf->getFileName(), langNameLenMax - 1, reservedChars.c_str(), true);

		TCHAR *tabNewName = reinterpret_cast<TCHAR *>(strDlg.doDialog());
		if (tabNewName)
		{
			BufferID sameNamedBufferId = _pDocTab->findBufferByName(tabNewName);
			if (sameNamedBufferId == BUFFER_INVALID)
			{
				sameNamedBufferId = _pNonDocTab->findBufferByName(tabNewName);
			}
			
			if (sameNamedBufferId != BUFFER_INVALID)
			{
				_nativeLangSpeaker.messageBox("RenameTabTemporaryNameAlreadyInUse",
					_pPublicInterface->getHSelf(),
					TEXT("The specified name is already in use on another tab."),
					TEXT("Rename failed"),
					MB_OK | MB_ICONSTOP);
			}
			else
			{
				success = true;
				buf->setFileName(tabNewName);
				bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
				if (isSnapshotMode)
				{
					generic_string oldBackUpFile = buf->getBackupFileName();

					// Change the backup file name and let MainFileManager decide the new filename
					buf->setBackupFileName(TEXT(""));

					// Create new backup
					buf->setModifiedStatus(true);
					bool bRes = MainFileManager.backupCurrentBuffer();

					// Delete old backup
					if (bRes)
						::DeleteFile(oldBackUpFile.c_str());
				}
			}
		}
	}

	scnN.nmhdr.code = success ? NPPN_FILERENAMED : NPPN_FILERENAMECANCEL;
	_pluginsManager.notify(&scnN);

	return success;
}


bool Notepad_plus::fileDelete(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();

	Buffer * buf = MainFileManager.getBufferByID(bufferID);
	const TCHAR *fileNamePath = buf->getFullPathName();

	winVer winVersion = (NppParameters::getInstance()).getWinVersion();
	bool goAhead = true;
	if (winVersion >= WV_WIN8 || winVersion == WV_UNKNOWN)
	{
		// Windows 8 (and version afer?) has no system alert, so we ask user's confirmation
		goAhead = (doDeleteOrNot(fileNamePath) == IDYES);
	}

	if (goAhead)
	{
		SCNotification scnN;
		scnN.nmhdr.code = NPPN_FILEBEFOREDELETE;
		scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
		scnN.nmhdr.idFrom = (uptr_t)bufferID;
		_pluginsManager.notify(&scnN);

		if (!MainFileManager.deleteFile(bufferID))
		{
			_nativeLangSpeaker.messageBox("DeleteFileFailed",
				_pPublicInterface->getHSelf(),
				TEXT("Delete File failed"),
				TEXT("Delete File"),
				MB_OK);

			scnN.nmhdr.code = NPPN_FILEDELETEFAILED;
			_pluginsManager.notify(&scnN);

			return false;
		}
		bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
		doClose(bufferID, MAIN_VIEW, isSnapshotMode);
		doClose(bufferID, SUB_VIEW, isSnapshotMode);

		scnN.nmhdr.code = NPPN_FILEDELETED;
		scnN.nmhdr.idFrom = (uptr_t)-1;
		_pluginsManager.notify(&scnN);

		return true;
	}
	return false;
}

void Notepad_plus::fileOpen()
{
	CustomFileDialog fDlg(_pPublicInterface->getHSelf());
	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"));

	setFileOpenSaveDlgFilters(fDlg, true);

	BufferID lastOpened = BUFFER_INVALID;
	const auto& fns = fDlg.doOpenMultiFilesDlg();
	size_t sz = fns.size();
	for (size_t i = 0 ; i < sz ; ++i)
	{
		BufferID test = doOpen(fns.at(i).c_str(), fDlg.isReadOnly());
		if (test != BUFFER_INVALID)
			lastOpened = test;
	}

	if (lastOpened != BUFFER_INVALID)
	{
		switchToFile(lastOpened);
	}
}


void Notepad_plus::fileNew()
{
    BufferID newBufID = MainFileManager.newEmptyDocument();

    loadBufferIntoView(newBufID, currentView(), true);	//true, because we want multiple new files if possible
    switchToFile(newBufID);
}


bool Notepad_plus::fileReload()
{
	assert(_pEditView != nullptr);
	BufferID buf = _pEditView->getCurrentBufferID();
	return doReload(buf, buf->isDirty());
}


bool Notepad_plus::isFileSession(const TCHAR * filename)
{
	// if file2open matches the ext of user defined session file ext, then it'll be opened as a session
	const TCHAR *definedSessionExt = NppParameters::getInstance().getNppGUI()._definedSessionExt.c_str();
	if (*definedSessionExt != '\0')
	{
		generic_string fncp = filename;
		TCHAR *pExt = PathFindExtension(fncp.c_str());

		generic_string usrSessionExt = TEXT("");
		if (*definedSessionExt != '.')
		{
			usrSessionExt += TEXT(".");
		}
		usrSessionExt += definedSessionExt;

		if (!generic_stricmp(pExt, usrSessionExt.c_str()))
		{
			return true;
		}
	}
	return false;
}

bool Notepad_plus::isFileWorkspace(const TCHAR * filename)
{
	// if filename matches the ext of user defined workspace file ext, then it'll be opened as a workspace
	const TCHAR *definedWorkspaceExt = NppParameters::getInstance().getNppGUI()._definedWorkspaceExt.c_str();
	if (*definedWorkspaceExt != '\0')
	{
		generic_string fncp = filename;
		TCHAR *pExt = PathFindExtension(fncp.c_str());

		generic_string usrWorkspaceExt = TEXT("");
		if (*definedWorkspaceExt != '.')
		{
			usrWorkspaceExt += TEXT(".");
		}
		usrWorkspaceExt += definedWorkspaceExt;

		if (!generic_stricmp(pExt, usrWorkspaceExt.c_str()))
		{
			return true;
		}
	}
	return false;
}

void Notepad_plus::loadLastSession()
{
	NppParameters& nppParams = NppParameters::getInstance();
	const NppGUI & nppGui = nppParams.getNppGUI();
	Session lastSession = nppParams.getSession();
	bool isSnapshotMode = nppGui.isSnapshotMode();
	_isFolding = true;
    loadSession(lastSession, isSnapshotMode);
	_isFolding = false;
}

bool Notepad_plus::loadSession(Session & session, bool isSnapshotMode, bool shouldLoadFileBrowser)
{
	NppParameters& nppParam = NppParameters::getInstance();
	bool allSessionFilesLoaded = true;
	BufferID lastOpened = BUFFER_INVALID;
	//size_t i = 0;
	showView(MAIN_VIEW);
	switchEditViewTo(MAIN_VIEW);	//open files in main

	int mainIndex2Update = -1;

	for (size_t i = 0; i < session.nbMainFiles() ; )
	{
		const TCHAR *pFn = session._mainViewFiles[i]._fileName.c_str();

		if (isFileSession(pFn) || isFileWorkspace(pFn))
		{
			vector<sessionFileInfo>::iterator posIt = session._mainViewFiles.begin() + i;
			session._mainViewFiles.erase(posIt);
			continue;	//skip session files, not supporting recursive sessions or embedded workspace files
		}

		bool isWow64Off = false;
		if (!PathFileExists(pFn))
		{
			nppParam.safeWow64EnableWow64FsRedirection(FALSE);
			isWow64Off = true;
		}
		if (PathFileExists(pFn))
		{
			if (isSnapshotMode && session._mainViewFiles[i]._backupFilePath != TEXT(""))
				lastOpened = doOpen(pFn, false, false, session._mainViewFiles[i]._encoding, session._mainViewFiles[i]._backupFilePath.c_str(), session._mainViewFiles[i]._originalFileLastModifTimestamp);
			else
				lastOpened = doOpen(pFn, false, false, session._mainViewFiles[i]._encoding);
		}
		else if (isSnapshotMode && PathFileExists(session._mainViewFiles[i]._backupFilePath.c_str()))
		{
			lastOpened = doOpen(pFn, false, false, session._mainViewFiles[i]._encoding, session._mainViewFiles[i]._backupFilePath.c_str(), session._mainViewFiles[i]._originalFileLastModifTimestamp);
		}
		else
		{
			lastOpened = BUFFER_INVALID;
		}
		if (isWow64Off)
		{
			nppParam.safeWow64EnableWow64FsRedirection(TRUE);
			isWow64Off = false;
		}

		if (lastOpened != BUFFER_INVALID)
		{
			showView(MAIN_VIEW);
			const TCHAR *pLn = session._mainViewFiles[i]._langName.c_str();
			int id = getLangFromMenuName(pLn);
			LangType typeToSet = L_TEXT;
			if (id != 0 && id != IDM_LANG_USER)
				typeToSet = menuID2LangType(id);
			if (typeToSet == L_EXTERNAL )
				typeToSet = (LangType)(id - IDM_LANG_EXTERNAL + L_EXTERNAL);

			Buffer *buf = MainFileManager.getBufferByID(lastOpened);

			if (session._mainViewFiles[i]._foldStates.size() > 0)
			{
				if (buf == _mainEditView.getCurrentBuffer()) // current document
					// Set floding state in the current doccument
					mainIndex2Update = static_cast<int32_t>(i);
				else
					// Set fold states in the buffer
					buf->setHeaderLineState(session._mainViewFiles[i]._foldStates, &_mainEditView);
			}

			buf->setPosition(session._mainViewFiles[i], &_mainEditView);
			buf->setMapPosition(session._mainViewFiles[i]._mapPos);
			buf->setLangType(typeToSet, pLn);
			if (session._mainViewFiles[i]._encoding != -1)
				buf->setEncoding(session._mainViewFiles[i]._encoding);

			buf->setUserReadOnly(session._mainViewFiles[i]._isUserReadOnly);

			if (isSnapshotMode && session._mainViewFiles[i]._backupFilePath != TEXT("") && PathFileExists(session._mainViewFiles[i]._backupFilePath.c_str()))
				buf->setDirty(true);

			//Force in the document so we can add the markers
			//Don't use default methods because of performance
			Document prevDoc = _mainEditView.execute(SCI_GETDOCPOINTER);
			_mainEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			for (size_t j = 0, len = session._mainViewFiles[i]._marks.size(); j < len ; ++j)
			{
				_mainEditView.execute(SCI_MARKERADD, session._mainViewFiles[i]._marks[j], MARK_BOOKMARK);
			}
			_mainEditView.execute(SCI_SETDOCPOINTER, 0, prevDoc);
			++i;
		}
		else
		{
			vector<sessionFileInfo>::iterator posIt = session._mainViewFiles.begin() + i;
			session._mainViewFiles.erase(posIt);
			allSessionFilesLoaded = false;
		}
	}
	if (mainIndex2Update != -1)
	{
		_isFolding = true;
		_mainEditView.syncFoldStateWith(session._mainViewFiles[mainIndex2Update]._foldStates);
		_isFolding = false;
	}


	showView(SUB_VIEW);
	switchEditViewTo(SUB_VIEW);	//open files in sub
	int subIndex2Update = -1;

	for (size_t k = 0 ; k < session.nbSubFiles() ; )
	{
		const TCHAR *pFn = session._subViewFiles[k]._fileName.c_str();

		if (isFileSession(pFn) || isFileWorkspace(pFn))
		{
			vector<sessionFileInfo>::iterator posIt = session._subViewFiles.begin() + k;
			session._subViewFiles.erase(posIt);
			continue;	//skip session files, not supporting recursive sessions or embedded workspace files
		}

		bool isWow64Off = false;
		if (!PathFileExists(pFn))
		{
			nppParam.safeWow64EnableWow64FsRedirection(FALSE);
			isWow64Off = true;
		}
		if (PathFileExists(pFn))
		{
			if (isSnapshotMode && session._subViewFiles[k]._backupFilePath != TEXT(""))
				lastOpened = doOpen(pFn, false, false, session._subViewFiles[k]._encoding, session._subViewFiles[k]._backupFilePath.c_str(), session._subViewFiles[k]._originalFileLastModifTimestamp);
			else
				lastOpened = doOpen(pFn, false, false, session._subViewFiles[k]._encoding);

			//check if already open in main. If so, clone
			if (_mainDocTab.getIndexByBuffer(lastOpened) != -1)
			{
				loadBufferIntoView(lastOpened, SUB_VIEW);
			}
		}
		else if (isSnapshotMode && PathFileExists(session._subViewFiles[k]._backupFilePath.c_str()))
		{
			lastOpened = doOpen(pFn, false, false, session._subViewFiles[k]._encoding, session._subViewFiles[k]._backupFilePath.c_str(), session._subViewFiles[k]._originalFileLastModifTimestamp);
		}
		else
		{
			lastOpened = BUFFER_INVALID;
		}
		if (isWow64Off)
		{
			nppParam.safeWow64EnableWow64FsRedirection(TRUE);
			isWow64Off = false;
		}

		if (lastOpened != BUFFER_INVALID)
		{
			showView(SUB_VIEW);
			if (canHideView(MAIN_VIEW))
				hideView(MAIN_VIEW);
			const TCHAR *pLn = session._subViewFiles[k]._langName.c_str();
			int id = getLangFromMenuName(pLn);
			LangType typeToSet = L_TEXT;

			if (id != 0)
				typeToSet = menuID2LangType(id);
			if (typeToSet == L_EXTERNAL )
				typeToSet = (LangType)(id - IDM_LANG_EXTERNAL + L_EXTERNAL);

			Buffer * buf = MainFileManager.getBufferByID(lastOpened);

			// Set fold states
			if (session._subViewFiles[k]._foldStates.size() > 0)
			{
				if (buf == _subEditView.getCurrentBuffer()) // current document
					// Set floding state in the current doccument
					subIndex2Update = static_cast<int32_t>(k);
				else
					// Set fold states in the buffer
					buf->setHeaderLineState(session._subViewFiles[k]._foldStates, &_subEditView);
			}

			buf->setPosition(session._subViewFiles[k], &_subEditView);
			buf->setMapPosition(session._subViewFiles[k]._mapPos);
			if (typeToSet == L_USER)
			{
				if (!lstrcmp(pLn, TEXT("User Defined")))
				{
					pLn = TEXT("");	//default user defined
				}
			}
			buf->setLangType(typeToSet, pLn);
			buf->setEncoding(session._subViewFiles[k]._encoding);
			buf->setUserReadOnly(session._subViewFiles[k]._isUserReadOnly);

			if (isSnapshotMode && session._subViewFiles[k]._backupFilePath != TEXT("") && PathFileExists(session._subViewFiles[k]._backupFilePath.c_str()))
				buf->setDirty(true);

			//Force in the document so we can add the markers
			//Don't use default methods because of performance
			Document prevDoc = _subEditView.execute(SCI_GETDOCPOINTER);
			_subEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			for (size_t j = 0, len = session._subViewFiles[k]._marks.size(); j < len ; ++j)
			{
				_subEditView.execute(SCI_MARKERADD, session._subViewFiles[k]._marks[j], MARK_BOOKMARK);
			}
			_subEditView.execute(SCI_SETDOCPOINTER, 0, prevDoc);

			++k;
		}
		else
		{
			vector<sessionFileInfo>::iterator posIt = session._subViewFiles.begin() + k;
			session._subViewFiles.erase(posIt);
			allSessionFilesLoaded = false;
		}
	}
	if (subIndex2Update != -1)
	{
		_isFolding = true;
		_subEditView.syncFoldStateWith(session._subViewFiles[subIndex2Update]._foldStates);
		_isFolding = false;
	}

	_mainEditView.restoreCurrentPosPreStep();
	_subEditView.restoreCurrentPosPreStep();

	if (session._activeMainIndex < _mainDocTab.nbItem())//session.nbMainFiles())
		activateBuffer(_mainDocTab.getBufferByIndex(session._activeMainIndex), MAIN_VIEW);

	if (session._activeSubIndex < _subDocTab.nbItem())//session.nbSubFiles())
		activateBuffer(_subDocTab.getBufferByIndex(session._activeSubIndex), SUB_VIEW);

	if ((session.nbSubFiles() > 0) && (session._activeView == MAIN_VIEW || session._activeView == SUB_VIEW))
		switchEditViewTo(static_cast<int32_t>(session._activeView));
	else
		switchEditViewTo(MAIN_VIEW);

	if (canHideView(otherView()))
		hideView(otherView());
	else if (canHideView(currentView()))
		hideView(currentView());

	if (_pDocumentListPanel)
		_pDocumentListPanel->reload();

	if (shouldLoadFileBrowser && !session._fileBrowserRoots.empty())
	{
		// Force launch file browser and add roots
		launchFileBrowser(session._fileBrowserRoots, session._fileBrowserSelectedItem, true);
	}

	return allSessionFilesLoaded;
}

bool Notepad_plus::fileLoadSession(const TCHAR *fn)
{
	bool result = false;
	generic_string sessionFileName;
	if (fn == NULL)
	{
		CustomFileDialog fDlg(_pPublicInterface->getHSelf());
		const TCHAR *ext = NppParameters::getInstance().getNppGUI()._definedSessionExt.c_str();
		generic_string sessionExt = TEXT("");
		if (*ext != '\0')
		{
			if (*ext != '.')
				sessionExt += TEXT(".");
			sessionExt += ext;
			fDlg.setExtFilter(TEXT("Session file"), sessionExt.c_str());
			fDlg.setDefExt(ext);
		}
		fDlg.setExtFilter(TEXT("All types"), TEXT(".*"));
		sessionFileName = fDlg.doOpenSingleFileDlg();
	}
	else
	{
		if (PathFileExists(fn))
			sessionFileName = fn;
	}


	NppParameters& nppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = nppParam.getNppGUI();
	if (!sessionFileName.empty())
	{
		bool isEmptyNpp = false;
		if (_mainDocTab.nbItem() == 1 && _subDocTab.nbItem() == 1)
		{
			Buffer * buf1 = MainFileManager.getBufferByID(_mainDocTab.getBufferByIndex(0));
			Buffer * buf2 = MainFileManager.getBufferByID(_subDocTab.getBufferByIndex(0));
			isEmptyNpp = (!buf1->isDirty() && buf1->isUntitled() && !buf2->isDirty() && buf2->isUntitled());
		}
		if (!isEmptyNpp && (nppGUI._multiInstSetting == multiInstOnSession || nppGUI._multiInstSetting == multiInst))
		{
			TCHAR nppFullPath[MAX_PATH];
			::GetModuleFileName(NULL, nppFullPath, MAX_PATH);


			generic_string args = TEXT("-multiInst -nosession -openSession ");
			args += TEXT("\"");
			args += sessionFileName;
			args += TEXT("\"");
			::ShellExecute(_pPublicInterface->getHSelf(), TEXT("open"), nppFullPath, args.c_str(), TEXT("."), SW_SHOW);
			result = true;
		}
		else
		{
			bool isAllSuccessful = true;
			Session session2Load;

			if (nppParam.loadSession(session2Load, sessionFileName.c_str()))
			{
				const bool isSnapshotMode = false;
				const bool shouldLoadFileBrowser = true;
				isAllSuccessful = loadSession(session2Load, isSnapshotMode, shouldLoadFileBrowser);
				result = true;
				if (isEmptyNpp && (nppGUI._multiInstSetting == multiInstOnSession || nppGUI._multiInstSetting == multiInst))
					nppParam.setLoadedSessionFilePath(sessionFileName);
			}
			if (!isAllSuccessful)
				nppParam.writeSession(session2Load, sessionFileName.c_str());
		}
		if (result == false)
		{
			_nativeLangSpeaker.messageBox("SessionFileInvalidError",
				NULL,
				TEXT("Session file is either corrupted or not valid."),
				TEXT("Could not Load Session"),
				MB_OK);
		}
	}

	return result;
}

const TCHAR * Notepad_plus::fileSaveSession(size_t nbFile, TCHAR ** fileNames, const TCHAR *sessionFile2save, bool includeFileBrowser)
{
	if (sessionFile2save)
	{
		Session currentSession;
		if ((nbFile) && (fileNames))
		{
			for (size_t i = 0 ; i < nbFile ; ++i)
			{
				if (PathFileExists(fileNames[i]))
					currentSession._mainViewFiles.push_back(generic_string(fileNames[i]));
			}
		}
		else
			getCurrentOpenedFiles(currentSession);

		currentSession._includeFileBrowser = includeFileBrowser;
		if (includeFileBrowser && _pFileBrowser && !_pFileBrowser->isClosed())
		{
			currentSession._fileBrowserSelectedItem = _pFileBrowser->getSelectedItemPath();
			for (auto&& rootFileName : _pFileBrowser->getRoots())
			{
				currentSession._fileBrowserRoots.push_back({ rootFileName });
			}
		}

		(NppParameters::getInstance()).writeSession(currentSession, sessionFile2save);
		return sessionFile2save;
	}
	return NULL;
}

const TCHAR * Notepad_plus::fileSaveSession(size_t nbFile, TCHAR ** fileNames)
{
	CustomFileDialog fDlg(_pPublicInterface->getHSelf());
	const TCHAR *ext = NppParameters::getInstance().getNppGUI()._definedSessionExt.c_str();

	generic_string sessionExt = TEXT("");
	if (*ext != '\0')
	{
		if (*ext != '.')
			sessionExt += TEXT(".");
		sessionExt += ext;
		fDlg.setExtFilter(TEXT("Session file"), sessionExt.c_str());
		fDlg.setDefExt(ext);
		fDlg.setExtIndex(0);		// 0 index for "custom extension types"
	}
	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"));
	const bool isCheckboxActive = _pFileBrowser && !_pFileBrowser->isClosed();
	const generic_string checkboxLabel = _nativeLangSpeaker.getLocalizedStrFromID("session-save-folder-as-workspace",
		TEXT("Save Folder as Workspace"));
	fDlg.setCheckbox(checkboxLabel.c_str(), isCheckboxActive);
	generic_string sessionFileName = fDlg.doSaveDlg();

	return fileSaveSession(nbFile, fileNames, sessionFileName.c_str(), fDlg.getCheckboxState());
}


void Notepad_plus::saveSession(const Session & session)
{
	(NppParameters::getInstance()).writeSession(session);
}


void Notepad_plus::saveCurrentSession()
{
	::SendMessage(_pPublicInterface->getHSelf(), NPPM_INTERNAL_SAVECURRENTSESSION, 0, 0);
}
