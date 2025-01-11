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
#include "Common.h"

using namespace std;

// https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
// Reserved characters:  < > : " / \ | ? * tab  
//  ("tab" is not in the official list, but it is good to avoid it)
const std::wstring filenameReservedChars = L"<>:\"/\\|\?*\t";

DWORD WINAPI Notepad_plus::monitorFileOnChange(void * params)
{
	MonitorInfo *monitorInfo = static_cast<MonitorInfo *>(params);
	Buffer *buf = monitorInfo->_buffer;
	HWND h = monitorInfo->_nppHandle;

	const wchar_t *fullFileName = (const wchar_t *)buf->getFullPathName();

	//The folder to watch :
	wchar_t folderToMonitor[MAX_PATH]{};
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
				bool bDoneOnce = false;
				DWORD dwAction = 0;
				wstring fn;
				// Process all available changes, ignore User actions
				while (dirChanges.Pop(dwAction, fn))
				{
					// Fix monitoring files which are under root problem
					size_t pos = fn.find(L"\\\\");
					if (pos == 2)
						fn.replace(pos, 2, L"\\");

					if (wcscmp(fullFileName, fn.c_str()) == 0)
					{
						if (dwAction == FILE_ACTION_MODIFIED)
						{
							if (!bDoneOnce)
							{
								::PostMessage(h, NPPM_INTERNAL_RELOADSCROLLTOEND, reinterpret_cast<WPARAM>(buf), 0);
								bDoneOnce = true;
								Sleep(250);	// Limit refresh rate
							}
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
	return ERROR_SUCCESS;
}

bool resolveLinkFile(std::wstring& linkFilePath)
{
	// upperize for the following comparison because the ends_with is case sensitive unlike the Windows OS filesystem
	std::wstring linkFilePathUp = linkFilePath;
	std::transform(linkFilePathUp.begin(), linkFilePathUp.end(), linkFilePathUp.begin(), ::towupper);
	if (!linkFilePathUp.ends_with(L".LNK"))
		return false; // we will not check the renamed shortcuts like "file.lnk.txt"

	bool isResolved = false;

	IShellLink* psl = nullptr;
	wchar_t targetFilePath[MAX_PATH]{};
	WIN32_FIND_DATA wfd{};

	HRESULT hres = CoInitialize(NULL);
	if (SUCCEEDED(hres))
	{
		hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
		if (SUCCEEDED(hres))
		{
			IPersistFile* ppf = nullptr;
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
							isResolved = true;
						}
					}
				}
				ppf->Release();
			}
			psl->Release();
		}
		CoUninitialize();
	}

	return isResolved;
}

BufferID Notepad_plus::doOpen(const wstring& fileName, bool isRecursive, bool isReadOnly, int encoding, const wchar_t *backupFileName, FILETIME fileNameTimestamp)
{
	const rsize_t longFileNameBufferSize = MAX_PATH; // TODO stop using fixed-size buffer
	if (fileName.size() >= longFileNameBufferSize - 1) // issue with all other sub-routines
		return BUFFER_INVALID;

	wstring targetFileName = fileName;
	bool isResolvedLinkFileName = resolveLinkFile(targetFileName);

	bool isRawFileName;
	if (isResolvedLinkFileName)
		isRawFileName = false;
	else
		isRawFileName = isWin32NamespacePrefixedFileName(fileName);

	if (isUnsupportedFileName(isResolvedLinkFileName ? targetFileName : fileName))
	{
		// TODO:
		// for the raw filenames we can allow even the usually unsupported filenames in the future,
		// but not now as it is not fully supported by the Notepad++ COM IFileDialog based Open/SaveAs dialogs
		//if (isRawFileName)
		//{
		//	int answer = _nativeLangSpeaker.messageBox("OpenNonconformingWin32FileName",
		//		_pPublicInterface->getHSelf(),
		//		L"You are about to open a file with unusual filename:\n\"$STR_REPLACE$\"",
		//		L"Open Nonconforming Win32-Filename",
		//		MB_OKCANCEL | MB_ICONWARNING | MB_APPLMODAL,
		//		0,
		//		isResolvedLinkFileName ? targetFileName.c_str() : fileName.c_str());
		//	if (answer != IDOK)
		//		return BUFFER_INVALID; // aborted by user
		//}
		//else
		//{
			// unsupported, use the existing Notepad++ file dialog to report
			_nativeLangSpeaker.messageBox("OpenFileError",
				_pPublicInterface->getHSelf(),
				L"Cannot open file \"$STR_REPLACE$\".",
				L"ERROR",
				MB_OK,
				0,
				isResolvedLinkFileName ? targetFileName.c_str() : fileName.c_str());
			return BUFFER_INVALID;
		//}
	}

	//If [GetFullPathName] succeeds, the return value is the length, in TCHARs, of the string copied to lpBuffer, not including the terminating null character.
	//If the lpBuffer buffer is too small to contain the path, the return value [of GetFullPathName] is the size, in TCHARs, of the buffer that is required to hold the path and the terminating null character.
	//If [GetFullPathName] fails for any other reason, the return value is zero.

	NppParameters& nppParam = NppParameters::getInstance();
	wchar_t longFileName[longFileNameBufferSize] = { 0 };

	if (isRawFileName)
	{
		// use directly the raw file name, skip the GetFullPathName WINAPI and alike...)
		wcsncpy_s(longFileName, _countof(longFileName), fileName.c_str(), _TRUNCATE);
	}
	else
	{
		const DWORD getFullPathNameResult = ::GetFullPathName(targetFileName.c_str(), longFileNameBufferSize, longFileName, NULL);
		if (getFullPathNameResult == 0)
		{
			return BUFFER_INVALID;
		}
		if (getFullPathNameResult > longFileNameBufferSize)
		{
			return BUFFER_INVALID;
		}
		assert(wcslen(longFileName) == getFullPathNameResult);

		if (wcschr(longFileName, '~'))
		{
			// ignore the returned value of function due to win64 redirection system
			::GetLongPathName(longFileName, longFileName, longFileNameBufferSize);
		}
	}

	bool isSnapshotMode = (backupFileName != NULL) && doesFileExist(backupFileName);
	bool longFileNameExists = doesFileExist(longFileName);
	if (isSnapshotMode && !longFileNameExists) // UNTITLED
	{
		wcscpy_s(longFileName, targetFileName.c_str());
	}
    _lastRecentFileList.remove(longFileName);


	// "fileName" could be:
	// 1. full file path to open or create
	// 2. "new N" or whatever renamed from "new N" to switch to (if user double-clicks the found entries of untitled documents, which is the results of running commands "Find in current doc" & "Find in opened docs")
	// 3. a file name with relative path to open or create

	// Search case 1 & 2 firstly
	BufferID foundBufID = MainFileManager.getBufferFromName(targetFileName.c_str());

	// if case 1 & 2 not found, search case 3
	if (foundBufID == BUFFER_INVALID)
	{
		wstring fileName2Find;
		fileName2Find = longFileName;
		foundBufID = MainFileManager.getBufferFromName(fileName2Find.c_str());
	}

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

    if (isFileSession(longFileName) && longFileNameExists)
    {
        fileLoadSession(longFileName);
        return BUFFER_INVALID;
    }

	if (isFileWorkspace(longFileName) && longFileNameExists)
	{
		nppParam.setWorkSpaceFilePath(0, longFileName);
		// This line switches to Project Panel 1 while starting up Npp
		// and after dragging a workspace file to Npp:
		launchProjectPanel(IDM_VIEW_PROJECT_PANEL_1, &_pProjectPanel_1, 0);
		return BUFFER_INVALID;
	}

#ifndef	_WIN64
    bool isWow64Off = false;
    if (!longFileNameExists)
    {
        nppParam.safeWow64EnableWow64FsRedirection(FALSE);
        isWow64Off = true;
    }
#endif

	bool globbing;
	if (isRawFileName)
		globbing = (wcsrchr(longFileName, wchar_t('*')) || (abs(longFileName - wcsrchr(longFileName, wchar_t('?'))) > 3));
	else
		globbing = (wcsrchr(longFileName, wchar_t('*')) || wcsrchr(longFileName, wchar_t('?')));

	if (!isSnapshotMode) // if not backup mode, or backupfile path is invalid
	{
		if (!doesPathExist(longFileName) && !globbing)
		{
			wstring longFileDir(longFileName);
			pathRemoveFileSpec(longFileDir);

			bool isCreateFileSuccessful = false;
			if (doesDirectoryExist(longFileDir.c_str()))
			{
				int res = _nativeLangSpeaker.messageBox("CreateNewFileOrNot",
					_pPublicInterface->getHSelf(),
					L"\"$STR_REPLACE$\" doesn't exist. Create it?",
					L"Create new file",
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
							L"Cannot create the file \"$STR_REPLACE$\".",
							L"Create new file",
							MB_OK,
							0,
							longFileName);
					}
				}
			}
			else
			{
				wstring msg, title;
				if (!_nativeLangSpeaker.getMsgBoxLang("OpenFileNoFolderError", title, msg))
				{
					title = L"Cannot open file";
					msg = L"\"";
					msg += longFileName;
					msg += L"\" cannot be opened:\nFolder \"";
					msg += longFileDir;
					msg += L"\" doesn't exist.";
				}
				else
				{
					msg = stringReplace(msg, L"$STR_REPLACE1$", longFileName);
					msg = stringReplace(msg, L"$STR_REPLACE2$", longFileDir);
				}
				::MessageBox(_pPublicInterface->getHSelf(), msg.c_str(), title.c_str(), MB_OK);
			}

			if (!isCreateFileSuccessful)
			{
#ifndef	_WIN64
				if (isWow64Off)
				{
					nppParam.safeWow64EnableWow64FsRedirection(TRUE);
					isWow64Off = false;
				}
#endif
				return BUFFER_INVALID;
			}
		}
	}

    // Notify plugins that current file is about to load
    // Plugins can should use this notification to filter SCN_MODIFIED
	SCNotification scnN{};
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
		buffer = MainFileManager.loadFile(longFileName, static_cast<Document>(NULL), encoding, backupFileName, fileNameTimestamp);

		if (buffer != BUFFER_INVALID)
		{
			// To notify plugins that a snapshot dirty file is loaded on startup
			SCNotification scnN2{};
			scnN2.nmhdr.hwndFrom = 0;
			scnN2.nmhdr.idFrom = (uptr_t)buffer;
			scnN2.nmhdr.code = NPPN_SNAPSHOTDIRTYFILELOADED;
			_pluginsManager.notify(&scnN2);

			buffer->setLoadedDirty(true);
		}
	}
	else
	{
		buffer = MainFileManager.loadFile(longFileName, static_cast<Document>(NULL), encoding);
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
        if (globbing || doesDirectoryExist(targetFileName.c_str()))
        {
            vector<wstring> fileNames;
            vector<wstring> patterns;
            if (globbing)
            {
                const wchar_t * substring = wcsrchr(targetFileName.c_str(), wchar_t('\\'));
				if (substring)
				{
					size_t pos = substring - targetFileName.c_str();

					patterns.push_back(substring + 1);
					wstring dir(targetFileName.c_str(), pos + 1); // use char * to evoke:
																   // string (const char* s, size_t n);
																   // and avoid to call (if pass string) :
																   // string (const string& str, size_t pos, size_t len = npos);

					getMatchedFileNames(dir.c_str(), 0, patterns, fileNames, isRecursive, false);
				}
            }
            else
            {
                wstring fileNameStr = targetFileName;
                if (targetFileName[targetFileName.size() - 1] != '\\')
                    fileNameStr += L"\\";

                patterns.push_back(L"*");
                getMatchedFileNames(fileNameStr.c_str(), 0, patterns, fileNames, true, false);
            }

            bool ok2Open = true;
            size_t nbFiles2Open = fileNames.size();

            if (nbFiles2Open > 200)
            {
                ok2Open = IDYES == _nativeLangSpeaker.messageBox("NbFileToOpenImportantWarning",
					_pPublicInterface->getHSelf(),
                    L"$INT_REPLACE$ files are about to be opened.\rAre you sure to open them?",
                    L"Amount of files to open is too large",
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
				L"Can not open file \"$STR_REPLACE$\".",
				L"ERROR",
				MB_OK,
				0,
				longFileName);

            _isFileOpening = false;

            scnN.nmhdr.code = NPPN_FILELOADFAILED;
            _pluginsManager.notify(&scnN);
        }
    }
#ifndef	_WIN64
    if (isWow64Off)
    {
        nppParam.safeWow64EnableWow64FsRedirection(TRUE);
        //isWow64Off = false;
    }
#endif
    return buffer;
}


bool Notepad_plus::doReload(BufferID id, bool alert)
{
	if (alert)
	{
		int answer = _nativeLangSpeaker.messageBox("DocReloadWarning",
			_pPublicInterface->getHSelf(),
			L"Are you sure you want to reload the current file and lose the changes made in Notepad++?",
			L"Reload",
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
		_mainEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
		_mainEditView.execute(SCI_SETDOCPOINTER, 0, 0);
		_mainEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
	}

	if (subVisisble)
	{
		_subEditView.saveCurrentPos();
		_subEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
		_subEditView.execute(SCI_SETDOCPOINTER, 0, 0);
		_subEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
	}

	if (!mainVisisble && !subVisisble)
	{
		return MainFileManager.reloadBufferDeferred(id);
	}

	bool res = MainFileManager.reloadBuffer(id);
	Buffer * pBuf = MainFileManager.getBufferByID(id);
	if (mainVisisble)
	{
		_mainEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
		_mainEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
		_mainEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
		_mainEditView.restoreCurrentPosPreStep();
	}

	if (subVisisble)
	{
		_subEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
		_subEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
		_subEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
		_subEditView.restoreCurrentPosPreStep();
	}

	// Once reload is complete, activate buffer which will take care of
	// many settings such as update status bar, clickable link etc.
	activateBuffer(id, currentView(), true);

	auto svp = NppParameters::getInstance().getSVP();
	if (svp._isChangeHistoryMarginEnabled || svp._isChangeHistoryIndicatorEnabled)
		clearChangesHistory();

	return res;
}

bool Notepad_plus::doSave(BufferID id, const wchar_t * filename, bool isCopy)
{
	const int index = MainFileManager.getBufferIndexByID(id);
	if (index == -1)
	{
		_nativeLangSpeaker.messageBox("BufferInvalidWarning",
			_pPublicInterface->getHSelf(),
			L"Cannot save: Buffer is invalid.",
			L"Save failed",
			MB_OK | MB_ICONWARNING);

		return false;
	}

	SCNotification scnN{};
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

	if (res == SavingStatus::NotEnoughRoom)
	{
		_nativeLangSpeaker.messageBox("NotEnoughRoom4Saving",
			_pPublicInterface->getHSelf(),
			L"Failed to save file.\nIt seems there's not enough space on disk to save file. Your file is not saved.",
			L"Save failed",
			MB_OK);
	}
	else if (res == SavingStatus::SaveWritingFailed)
	{
		wstring errorMessage = GetLastErrorAsString(GetLastError());
		::MessageBox(_pPublicInterface->getHSelf(), errorMessage.c_str(), L"Save failed", MB_OK | MB_ICONWARNING);
	}
	else if (res == SavingStatus::SaveOpenFailed)
	{
		Buffer* buf = MainFileManager.getBufferByID(id);
		if (buf->isFromNetwork())
		{
			_nativeLangSpeaker.messageBox("FileLockedWarning",
				_pPublicInterface->getHSelf(),
				L"Please check whether the network where the file is located is connected.",
				L"Save failed",
				MB_OK | MB_ICONWARNING);
		}
		else
		{
			if (_isAdministrator)
			{
				// Already in admin mode? File is probably locked.
				_nativeLangSpeaker.messageBox("FileLockedWarning",
					_pPublicInterface->getHSelf(),
					L"Please check whether if this file is opened in another program.",
					L"Save failed",
					MB_OK | MB_ICONWARNING);
			}
			else
			{
				// try to open Notepad++ in admin mode
				const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
				bool isSnapshotMode = nppGui.isSnapshotMode();
				bool isAlwaysInMultiInstMode = nppGui._multiInstSetting == multiInst;
				if (isSnapshotMode && !isAlwaysInMultiInstMode) // if both rememberSession && backup mode are enabled and "Always In Multi-Instance Mode" option not activated:
				{                                               // Open the 2nd Notepad++ instance in Admin mode, then close the 1st instance.

					int openInAdminModeRes = _nativeLangSpeaker.messageBox("OpenInAdminMode",
						_pPublicInterface->getHSelf(),
						L"This file cannot be saved and it may be protected.\rDo you want to launch Notepad++ in Administrator mode?",
						L"Save failed",
						MB_YESNO);

					if (openInAdminModeRes == IDYES)
					{
						wchar_t nppFullPath[MAX_PATH]{};
						::GetModuleFileName(NULL, nppFullPath, MAX_PATH);

						wstring args = L"-multiInst";
						size_t shellExecRes = (size_t)::ShellExecute(_pPublicInterface->getHSelf(), L"runas", nppFullPath, args.c_str(), L".", SW_SHOW);

						// If the function succeeds, it returns a value greater than 32. If the function fails,
						// it returns an error value that indicates the cause of the failure.
						// https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx

						if (shellExecRes <= 32)
						{
							_nativeLangSpeaker.messageBox("OpenInAdminModeFailed",
								_pPublicInterface->getHSelf(),
								L"Notepad++ cannot be opened in Administrator mode.",
								L"Open in Administrator mode failed",
								MB_OK);
						}
						else
						{
							::SendMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
						}

					}
				}
				else // rememberSession && backup mode are not both enabled, or "Always In Multi-Instance Mode" option is ON:
				{    // Open only the file to save in Notepad++ of Administrator mode by keeping the current instance.

					int openInAdminModeRes = _nativeLangSpeaker.messageBox("OpenInAdminModeWithoutCloseCurrent",
						_pPublicInterface->getHSelf(),
						L"The file cannot be saved and it may be protected.\rDo you want to launch Notepad++ in Administrator mode?",
						L"Save failed",
						MB_YESNO);

					if (openInAdminModeRes == IDYES)
					{
						wchar_t nppFullPath[MAX_PATH]{};
						::GetModuleFileName(NULL, nppFullPath, MAX_PATH);

						Buffer* buf = MainFileManager.getBufferByID(id);

						//process the fileNamePath into LRF
						wstring fileNamePath = buf->getFullPathName();

						wstring args = L"-multiInst -nosession ";
						args += L"\"";
						args += fileNamePath;
						args += L"\"";
						size_t shellExecRes = (size_t)::ShellExecute(_pPublicInterface->getHSelf(), L"runas", nppFullPath, args.c_str(), L".", SW_SHOW);

						// If the function succeeds, it returns a value greater than 32. If the function fails,
						// it returns an error value that indicates the cause of the failure.
						// https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx

						if (shellExecRes <= 32)
						{
							_nativeLangSpeaker.messageBox("OpenInAdminModeFailed",
								_pPublicInterface->getHSelf(),
								L"Notepad++ cannot be opened in Administrator mode.",
								L"Open in Administrator mode failed",
								MB_OK);
						}
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
	SCNotification scnN{};
	scnN.nmhdr.code = NPPN_FILEBEFORECLOSE;
	scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
	scnN.nmhdr.idFrom = (uptr_t)id;
	_pluginsManager.notify(&scnN);

	// Add to recent file history only if file is removed from all the views
	// There might be cases when file is cloned/moved to view.
	// Don't add to recent list unless file is removed from all the views
	wstring fileFullPath;
	if (!buf->isUntitled())
	{
		const wchar_t *fn = buf->getFullPathName();
		bool fileExists = doesFileExist(fn);

#ifndef	_WIN64
		// For Notepad++ 32 bits, if the file doesn't exist, it could be redirected
		// So we turn Wow64 off
		NppParameters& nppParam = NppParameters::getInstance();
		bool isWow64Off = false;
		if (!fileExists)
		{
			nppParam.safeWow64EnableWow64FsRedirection(FALSE);
			isWow64Off = true;
			fileExists = doesFileExist(fn);
		}
#endif

		if (fileExists)
			fileFullPath = fn;

#ifndef	_WIN64
		// We enable Wow64 system, if it was disabled
		if (isWow64Off)
		{
			nppParam.safeWow64EnableWow64FsRedirection(TRUE);
			//isWow64Off = false;
		}
#endif
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

	checkSyncState();

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
	::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);

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

wstring Notepad_plus::exts2Filters(const wstring& exts, int maxExtsLen) const
{
	const wchar_t *extStr = exts.c_str();
	wchar_t aExt[MAX_PATH] = { '\0' };
	wstring filters(L"");

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
					filters += L"*.";
					filters += aExt;
					filters += L";";
				}
				j = 0;

				if (maxExtsLen != -1 && i >= static_cast<size_t>(maxExtsLen))
				{
					filters += L" ... ";
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
			filters += L"*.";
			filters += aExt;
			filters += L";";
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
			const wchar_t *defList = l->getDefaultExtList();
			const wchar_t *userList = NULL;

			LexerStylerArray &lsa = (NppParameters::getInstance()).getLStylerArray();
			const wchar_t *lName = l->getLangName();
			LexerStyler *pLS = lsa.getLexerStylerByName(lName);

			if (pLS)
				userList = pLS->getLexerUserExt();

			wstring list(L"");
			if (defList)
				list += defList;
			if (userList)
			{
				list += L" ";
				list += userList;
			}

			wstring stringFilters = exts2Filters(list, showAllExt ? -1 : 40);
			const wchar_t *filters = stringFilters.c_str();
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
	
	LangType lt = (LangType)langType;
	wstring fileUdlString(getLangDesc(lt, true));

	for (size_t u=0; u<(size_t)nppParam.getNbUserLang(); u++)
	{
		UserLangContainer& ulc = nppParam.getULCFromIndex(u);
		const wchar_t *extList = ulc.getExtention();
		const wchar_t *lName = ulc.getName();

		wstring list(L"");
		list += extList;

		wstring stringFilters = exts2Filters(list, showAllExt ? -1 : 40);
		const wchar_t *filters = stringFilters.c_str();

		if (filters[0])
		{
			fDlg.setExtFilter(lName, filters);

			if (lt == L_USER)
			{
				wstring loopUdlString(L"udf - ");
				loopUdlString += lName;

				if (!ltFound)
				{
					ltFound = fileUdlString.compare(loopUdlString) == 0;
				}

				if (!ltFound)
				{
					++ltIndex;
				}
			}
		}
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

	int viewToClose = currentView();
	if (curView != -1)
		viewToClose = curView;

	// Determinate if it's a cloned buffer
	DocTabView* nonCurrentTab = (viewToClose == MAIN_VIEW) ? &_subDocTab : &_mainDocTab;
	bool isCloned = nonCurrentTab->getIndexByBuffer(bufferID) != -1;

	if ((buf->isUntitled() && buf->docLength() == 0) || isCloned)
	{
		// Do nothing
	}
	else if (buf->isDirty())
	{
		const wchar_t* fileNamePath = buf->getFullPathName();
		int res = doSaveOrNot(fileNamePath);

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

	bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
	bool doDeleteBackup = isSnapshotMode;
	if (isSnapshotMode && isCloned) // if Buffer is cloned then we don't delete backup file
		doDeleteBackup = false;

	doClose(bufferID, viewToClose, doDeleteBackup);
	return true;
}

void Notepad_plus::unPinnedForAllBuffers()
{
	for (size_t i = 0; i < _mainDocTab.nbItem(); ++i)
	{
		BufferID id = _mainDocTab.getBufferByIndex(i);
		Buffer* buf = MainFileManager.getBufferByID(id);
		buf->setPinned(false);
	}

	for (size_t i = 0; i < _subDocTab.nbItem(); ++i)
	{
		BufferID id = _subDocTab.getBufferByIndex(i);
		Buffer* buf = MainFileManager.getBufferByID(id);
		buf->setPinned(false);
	}
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
				if (buf->getBackupFileName() == L"" || !doesFileExist(buf->getBackupFileName().c_str())) //backup file has been deleted from outside
				{
					// warning user and save it if user want it.
					activateBuffer(id, MAIN_VIEW);
					if (!activateBuffer(id, SUB_VIEW))
						switchEditViewTo(MAIN_VIEW);

					int res = _nativeLangSpeaker.messageBox("NoBackupDoSaveFile",
						_pPublicInterface->getHSelf(),
						L"Your backup file cannot be found (deleted from outside).\rSave it otherwise your data will be lost\rDo you want to save file \"$STR_REPLACE$\" ?",
						L"Save",
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
				if (buf->getBackupFileName() == L"" || !doesFileExist(buf->getBackupFileName().c_str())) //backup file has been deleted from outside
				{
					// warning user and save it if user want it.
					activateBuffer(id, SUB_VIEW);
					switchEditViewTo(SUB_VIEW);

					int res = _nativeLangSpeaker.messageBox("NoBackupDoSaveFile",
						_pPublicInterface->getHSelf(),
						L"Your backup file cannot be found (deleted from outside).\rSave it otherwise your data will be lost\rDo you want to save file \"$STR_REPLACE$\" ?",
						L"Save",
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

bool Notepad_plus::fileCloseAllGiven(const std::vector<BufferViewInfo>& fileInfos)
{
	// First check if we need to save any file.

	bool noSaveToAll = false;
	bool saveToAll = false;
	std::vector<BufferViewInfo> buffersToClose;

	// Count the number of dirty file
	size_t nbDirtyFiles = 0;
	for (const auto& i : fileInfos)
	{
		Buffer* buf = MainFileManager.getBufferByID(i._bufID);

		if (buf->isDirty())
			++nbDirtyFiles;
	}

	for (const auto& i : fileInfos)
	{
		Buffer* buf = MainFileManager.getBufferByID(i._bufID);

		if ((buf->isUntitled() && buf->docLength() == 0) || noSaveToAll || !buf->isDirty())
		{
			// Do nothing, these documents are already ready to close
			buffersToClose.push_back(i);
		}
		else if (buf->isDirty() && !noSaveToAll)
		{
			if (_activeView == MAIN_VIEW)
			{
				activateBuffer(i._bufID, MAIN_VIEW);
				if (!activateBuffer(i._bufID, SUB_VIEW))
					switchEditViewTo(MAIN_VIEW);
			}
			else
			{
				activateBuffer(i._bufID, SUB_VIEW);
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
				if (!fileSave(i._bufID))
					break;	// Abort entire procedure, but close whatever processed so far

				buffersToClose.push_back(i);

				if (res == IDRETRY)
					saveToAll = true;
			}
			else if (res == IDNO || res == IDIGNORE)
			{
				buffersToClose.push_back(i);

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
	for (const auto& i : buffersToClose)
	{
		doClose(i._bufID, i._iView, isSnapshotMode);
	}

	return true;
}

bool Notepad_plus::fileCloseAllToLeft()
{
	// Indexes must go from high to low to deal with the fact that when one index is closed, any remaining
	// indexes (smaller than the one just closed) will point to the wrong tab.
	std::vector<BufferViewInfo> bufsToClose;
	for (int i = _pDocTab->getCurrentTabIndex() - 1; i >= 0; i--)
	{
		bufsToClose.push_back(BufferViewInfo(_pDocTab->getBufferByIndex(i), currentView()));
	}
	return fileCloseAllGiven(bufsToClose);
}

bool Notepad_plus::fileCloseAllToRight()
{
	// Indexes must go from high to low to deal with the fact that when one index is closed, any remaining
	// indexes (smaller than the one just closed) will point to the wrong tab.
	const int iActive = _pDocTab->getCurrentTabIndex();
	std::vector<BufferViewInfo> bufsToClose;
	for (int i = int(_pDocTab->nbItem()) - 1; i > iActive; i--)
	{
		bufsToClose.push_back(BufferViewInfo(_pDocTab->getBufferByIndex(i), currentView()));
	}
	return fileCloseAllGiven(bufsToClose);
}

void Notepad_plus::fileCloseAllButPinned()
{
	std::vector<BufferViewInfo> bufsToClose;

	int iPinned = -1;
	for (int j = 0; j < int(_mainDocTab.nbItem()); ++j)
	{
		if (_mainDocTab.getBufferByIndex(j)->isPinned())
			iPinned++;
		else
			break;
	}
	
	for (int i = int(_mainDocTab.nbItem()) - 1; i > iPinned; i--)
	{
		bufsToClose.push_back(BufferViewInfo(_mainDocTab.getBufferByIndex(i), MAIN_VIEW));
	}


	iPinned = -1;
	for (int j = 0; j < int(_subDocTab.nbItem()); ++j)
	{
		if (_subDocTab.getBufferByIndex(j)->isPinned())
			iPinned++;
		else
			break;
	}
	for (int i = int(_subDocTab.nbItem()) - 1; i > iPinned; i--)
	{
		bufsToClose.push_back(BufferViewInfo(_subDocTab.getBufferByIndex(i), SUB_VIEW));
	}
	
	fileCloseAllGiven(bufsToClose);
}

bool Notepad_plus::fileCloseAllUnchanged()
{
	// Indexes must go from high to low to deal with the fact that when one index is closed, any remaining
	// indexes (smaller than the one just closed) will point to the wrong tab.
	std::vector<BufferViewInfo> bufsToClose;

	for (int i = int(_pDocTab->nbItem()) - 1; i >= 0; i--)
	{
		BufferID id = _pDocTab->getBufferByIndex(i);
		Buffer* buf = MainFileManager.getBufferByID(id);
		if ((buf->isUntitled() && buf->docLength() == 0) || !buf->isDirty())
		{
			bufsToClose.push_back(BufferViewInfo(_pDocTab->getBufferByIndex(i), currentView()));
		}
	}

	return fileCloseAllGiven(bufsToClose);
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
		Buffer* buf = MainFileManager.getBufferByID(id);
		if (id == current)
			continue;

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
			const wchar_t *fn = buf->getFullPathName();
			wchar_t *name = ::PathFindFileName(fn);
			wstring fn_bak;

			if (nppgui._useDir && !nppgui._backupDir.empty())
			{
				// Get the custom directory, make sure it has a trailing slash
				fn_bak = nppgui._backupDir;
				if (fn_bak.back() != L'\\')
					fn_bak += L"\\";
			}
			else
			{
				// Get the current file's directory
				wstring path = fn;
				::pathRemoveFileSpec(path);
				fn_bak = path;
				fn_bak += L"\\";

				// If verbose, save it in a sub folder
				if (backup == bak_verbose)
				{
					fn_bak += L"nppBackup\\";
				}
			}

			// Expand any environment variables
			wchar_t fn_bak_expanded[MAX_PATH] = { '\0' };
			::ExpandEnvironmentStrings(fn_bak.c_str(), fn_bak_expanded, MAX_PATH);
			fn_bak = fn_bak_expanded;

			// Make sure the directory exists
			if (!doesDirectoryExist(fn_bak.c_str()))
			{
				SHCreateDirectory(NULL, fn_bak.c_str());
			}

			// Determine what to name the backed-up file
			if (backup == bak_simple)
			{
				fn_bak += name;
				fn_bak += L".bak";
			}
			else if (backup == bak_verbose)
			{
				time_t ltime = time(0);
				const struct tm* today;

				today = localtime(&ltime);
				if (today)
				{
					constexpr int temBufLen = 32;
					wchar_t tmpbuf[temBufLen]{};
					wcsftime(tmpbuf, temBufLen, L"%Y-%m-%d_%H%M%S", today);

					fn_bak += name;
					fn_bak += L".";
					fn_bak += tmpbuf;
					fn_bak += L".bak";
				}
			}

			BOOL doCancel = FALSE;
			if (!::CopyFileEx(fn, fn_bak.c_str(), nullptr, nullptr, &doCancel, COPY_FILE_NO_BUFFERING))
			{
				int res = _nativeLangSpeaker.messageBox("FileBackupFailed",
					_pPublicInterface->getHSelf(),
					L"The previous version of the file could not be saved into the backup directory at \"$STR_REPLACE$\".\r\rDo you want to save the current file anyways?",
					L"File Backup Failed",
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

bool Notepad_plus::fileSaveSpecific(const wstring& fileNameToSave)
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

	wstring origPathname = buf->getFullPathName();
	bool wasUntitled = buf->isUntitled();

	CustomFileDialog fDlg(_pPublicInterface->getHSelf());

	fDlg.setExtFilter(L"All types", L".*");

	LangType langType = buf->getLangType();

	const bool defaultAllTypes = NppParameters::getInstance().getNppGUI()._setSaveDlgExtFiltToAllTypes;
	const int langTypeIndex = setFileOpenSaveDlgFilters(fDlg, false, langType);
	
	fDlg.setDefFileName(buf->getFileName());

	fDlg.setExtIndex(langTypeIndex + 1); // +1 for "All types"

	fDlg.setSaveAsCopy(isSaveCopy);

	wstring localizedTitle;
	if (isSaveCopy)
	{
		localizedTitle = _nativeLangSpeaker.getNativeLangMenuString(IDM_FILE_SAVECOPYAS, L"Save a Copy As", true);
	}
	else
	{
		localizedTitle = _nativeLangSpeaker.getNativeLangMenuString(IDM_FILE_SAVEAS, L"Save As", true);
	}
	fDlg.setTitle(localizedTitle.c_str());

	const wstring checkboxLabel = _nativeLangSpeaker.getLocalizedStrFromID("file-save-assign-type", L"&Append extension");
	fDlg.enableFileTypeCheckbox(checkboxLabel, !defaultAllTypes);

	// Disable file autodetection before opening save dialog to prevent use-after-delete bug.
	NppParameters& nppParam = NppParameters::getInstance();
	auto cdBefore = nppParam.getNppGUI()._fileAutoDetection;
	(nppParam.getNppGUI())._fileAutoDetection = cdDisabled;

	wstring fn = fDlg.doSaveDlg();

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

			if (res && isSaveCopy && fDlg.getOpenTheCopyAfterSaveAsCopy())
			{
				BufferID bid = doOpen(fn);
				if (bid != BUFFER_INVALID)
				{
					switchToFile(bid);
				}
			}

			return res;
		}
		else		//cannot save, other view has buffer already open, activate it
		{
			_nativeLangSpeaker.messageBox("FileAlreadyOpenedInNpp",
				_pPublicInterface->getHSelf(),
				L"The file is already opened in Notepad++.",
				L"ERROR",
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

	SCNotification scnN{};
	scnN.nmhdr.code = NPPN_FILEBEFORERENAME;
	scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
	scnN.nmhdr.idFrom = (uptr_t)bufferID;

	bool success = false;
	wstring oldFileNamePath = buf->getFullPathName();
	bool isFileExisting = doesFileExist(oldFileNamePath.c_str());
	if (isFileExisting)
	{
		CustomFileDialog fDlg(_pPublicInterface->getHSelf());

		fDlg.setExtFilter(L"All types", L".*");
		setFileOpenSaveDlgFilters(fDlg, false);
		fDlg.setFolder(buf->getFullPathName());
		fDlg.setDefFileName(buf->getFileName());

		wstring localizedRename = _nativeLangSpeaker.getNativeLangMenuString(IDM_FILE_RENAME, L"Rename", true);
		fDlg.setTitle(localizedRename.c_str());

		std::wstring fn = fDlg.doSaveDlg();

		if (!fn.empty())
		{
			_pluginsManager.notify(&scnN);
			success = MainFileManager.moveFile(bufferID, fn.c_str());
			scnN.nmhdr.code = success ? NPPN_FILERENAMED : NPPN_FILERENAMECANCEL;
			_pluginsManager.notify(&scnN);
		}
	}
	else
	{
		// We are just going to rename the tab nothing else
		// So just rename the tab and rename the backup file too if applicable

		wstring staticName = _nativeLangSpeaker.getLocalizedStrFromID("tabrename-newname", L"New name");

		StringDlg strDlg;
		wstring title = _nativeLangSpeaker.getLocalizedStrFromID("tabrename-title", L"Rename Current Tab");
		strDlg.init(_pPublicInterface->getHinst(), _pPublicInterface->getHSelf(), title.c_str(), staticName.c_str(), buf->getFileName(), langNameLenMax - 1, filenameReservedChars.c_str(), true);

		wchar_t *tabNewName = reinterpret_cast<wchar_t *>(strDlg.doDialog());
		if (tabNewName)
		{
			wstring tabNewNameStr = tabNewName;
			trim(tabNewNameStr); // No leading and tailing space allowed

			BufferID sameNamedBufferId = _pDocTab->findBufferByName(tabNewNameStr.c_str());
			if (sameNamedBufferId == BUFFER_INVALID)
			{
				sameNamedBufferId = _pNonDocTab->findBufferByName(tabNewNameStr.c_str());
			}
			
			if (sameNamedBufferId != BUFFER_INVALID)
			{
				_nativeLangSpeaker.messageBox("RenameTabTemporaryNameAlreadyInUse",
					_pPublicInterface->getHSelf(),
					L"The specified name is already in use on another tab.",
					L"Rename failed",
					MB_OK | MB_ICONSTOP);
			}
			else if (tabNewNameStr.empty())
			{
				_nativeLangSpeaker.messageBox("RenameTabTemporaryNameIsEmpty",
					_pPublicInterface->getHSelf(),
					L"The specified name cannot be empty, or it cannot contain only space(s) or TAB(s).",
					L"Rename failed",
					MB_OK | MB_ICONSTOP);
			}
			else
			{
				_pluginsManager.notify(&scnN);
				buf->setFileName(tabNewNameStr.c_str());
				scnN.nmhdr.code = NPPN_FILERENAMED;
				_pluginsManager.notify(&scnN);

				success = true;

				bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
				if (isSnapshotMode)
				{
					wstring oldBackUpFileName = buf->getBackupFileName();
					if (oldBackUpFileName.empty())
						return success;

					wstring newBackUpFileName = oldBackUpFileName;
					newBackUpFileName.replace(newBackUpFileName.rfind(oldFileNamePath), oldFileNamePath.length(), tabNewNameStr);

					if (doesFileExist(newBackUpFileName.c_str()))
						::ReplaceFile(newBackUpFileName.c_str(), oldBackUpFileName.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, 0, 0);
					else
						::MoveFileEx(oldBackUpFileName.c_str(), newBackUpFileName.c_str(), MOVEFILE_REPLACE_EXISTING);

					buf->setBackupFileName(newBackUpFileName);
				}
			}
		}
	}

	return success;
}

bool Notepad_plus::fileRenameUntitledPluginAPI(BufferID id, const wchar_t* tabNewName)
{
	if (tabNewName == nullptr) return false;

	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
	{
		bufferID = _pEditView->getCurrentBufferID();
	}

	int bufferIndex = MainFileManager.getBufferIndexByID(bufferID);
	if (bufferIndex == -1) return false;

	Buffer* buf = MainFileManager.getBufferByIndex(bufferIndex);

	if (buf == nullptr || !buf->isUntitled()) return false;

	// We are just going to rename the tab nothing else
	// So just rename the tab and rename the backup file too if applicable

	std::wstring tabNewNameStr = tabNewName;

	trim(tabNewNameStr); // No leading and tailing space allowed

	if (tabNewNameStr.empty()) return false;

	if (tabNewNameStr.length() > langNameLenMax - 1) return false;

	if (tabNewNameStr.find_first_of(filenameReservedChars) != std::wstring::npos) return false;

	BufferID sameNamedBufferId = _pDocTab->findBufferByName(tabNewNameStr.c_str());
	if (sameNamedBufferId == BUFFER_INVALID)
	{
		sameNamedBufferId = _pNonDocTab->findBufferByName(tabNewNameStr.c_str());
	}

	if (sameNamedBufferId != BUFFER_INVALID) return false;


	SCNotification scnN{};
	scnN.nmhdr.code = NPPN_FILEBEFORERENAME;
	scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
	scnN.nmhdr.idFrom = (uptr_t)bufferID;
	_pluginsManager.notify(&scnN);

	wstring oldName = buf->getFullPathName();
	buf->setFileName(tabNewNameStr.c_str());

	scnN.nmhdr.code = NPPN_FILERENAMED;
	_pluginsManager.notify(&scnN);

	bool isSnapshotMode = NppParameters::getInstance().getNppGUI().isSnapshotMode();
	if (isSnapshotMode)
	{
		wstring oldBackUpFileName = buf->getBackupFileName();
		if (oldBackUpFileName.empty())
		{
			return true;
		}

		wstring newBackUpFileName = oldBackUpFileName;
		newBackUpFileName.replace(newBackUpFileName.rfind(oldName), oldName.length(), tabNewNameStr);

		if (doesFileExist(newBackUpFileName.c_str()))
			::ReplaceFile(newBackUpFileName.c_str(), oldBackUpFileName.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, 0, 0);
		else
			::MoveFileEx(oldBackUpFileName.c_str(), newBackUpFileName.c_str(), MOVEFILE_REPLACE_EXISTING);

		buf->setBackupFileName(newBackUpFileName);
	}

	return true;
}

bool Notepad_plus::fileDelete(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();

	Buffer * buf = MainFileManager.getBufferByID(bufferID);
	const wchar_t *fileNamePath = buf->getFullPathName();

	winVer winVersion = (NppParameters::getInstance()).getWinVersion();
	bool goAhead = true;
	if (winVersion >= WV_WIN8 || winVersion == WV_UNKNOWN)
	{
		// Windows 8 (and version afer?) has no system alert, so we ask user's confirmation
		goAhead = (doDeleteOrNot(fileNamePath) == IDOK);
	}

	if (goAhead)
	{
		SCNotification scnN{};
		scnN.nmhdr.code = NPPN_FILEBEFOREDELETE;
		scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
		scnN.nmhdr.idFrom = (uptr_t)bufferID;
		_pluginsManager.notify(&scnN);

		if (!MainFileManager.deleteFile(bufferID))
		{
			_nativeLangSpeaker.messageBox("DeleteFileFailed",
				_pPublicInterface->getHSelf(),
				L"Delete File failed",
				L"Delete File",
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
	wstring localizedTitle = _nativeLangSpeaker.getNativeLangMenuString(IDM_FILE_OPEN, L"Open", true);
	fDlg.setTitle(localizedTitle.c_str());
	fDlg.setExtFilter(L"All types", L".*");

	setFileOpenSaveDlgFilters(fDlg, true);

	BufferID lastOpened = BUFFER_INVALID;
	const auto& fns = fDlg.doOpenMultiFilesDlg();
	size_t sz = fns.size();
	for (size_t i = 0 ; i < sz ; ++i)
	{
		BufferID test = doOpen(fns.at(i).c_str());
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


bool Notepad_plus::isFileSession(const wchar_t * filename)
{
	// if file2open matches the ext of user defined session file ext, then it'll be opened as a session
	const wchar_t *definedSessionExt = NppParameters::getInstance().getNppGUI()._definedSessionExt.c_str();
	if (*definedSessionExt != '\0')
	{
		wstring fncp = filename;
		wchar_t *pExt = PathFindExtension(fncp.c_str());

		wstring usrSessionExt = L"";
		if (*definedSessionExt != '.')
		{
			usrSessionExt += L".";
		}
		usrSessionExt += definedSessionExt;

		if (!wcsicmp(pExt, usrSessionExt.c_str()))
		{
			return true;
		}
	}
	return false;
}

bool Notepad_plus::isFileWorkspace(const wchar_t * filename)
{
	// if filename matches the ext of user defined workspace file ext, then it'll be opened as a workspace
	const wchar_t *definedWorkspaceExt = NppParameters::getInstance().getNppGUI()._definedWorkspaceExt.c_str();
	if (*definedWorkspaceExt != '\0')
	{
		wstring fncp = filename;
		wchar_t *pExt = PathFindExtension(fncp.c_str());

		wstring usrWorkspaceExt = L"";
		if (*definedWorkspaceExt != '.')
		{
			usrWorkspaceExt += L".";
		}
		usrWorkspaceExt += definedWorkspaceExt;

		if (!wcsicmp(pExt, usrWorkspaceExt.c_str()))
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

bool Notepad_plus::loadSession(Session & session, bool isSnapshotMode, const wchar_t* userCreatedSessionName)
{
	NppParameters& nppParam = NppParameters::getInstance();
	const NppGUI& nppGUI = nppParam.getNppGUI();

	nppParam.setTheWarningHasBeenGiven(false);

	bool allSessionFilesLoaded = true;
	BufferID lastOpened = BUFFER_INVALID;
	//size_t i = 0;
	showView(MAIN_VIEW);
	switchEditViewTo(MAIN_VIEW);	//open files in main

	int mainIndex2Update = -1;

	// no session
	if (!session.nbMainFiles() && !session.nbSubFiles())
	{
		Buffer* buf = getCurrentBuffer();
		if (nppParam.getNativeLangSpeaker()->isRTL() && nppParam.getNativeLangSpeaker()->isEditZoneRTL())
			buf->setRTL(true);

		_mainEditView.changeTextDirection(buf->isRTL());
		return true;
	}

	for (size_t i = 0; i < session.nbMainFiles() ; )
	{
		const wchar_t *pFn = session._mainViewFiles[i]._fileName.c_str();

		if (isFileSession(pFn) || isFileWorkspace(pFn))
		{
			vector<sessionFileInfo>::iterator posIt = session._mainViewFiles.begin() + i;
			session._mainViewFiles.erase(posIt);
			continue;	//skip session files, not supporting recursive sessions or embedded workspace files
		}
#ifndef	_WIN64
		bool isWow64Off = false;
		if (!doesFileExist(pFn))
		{
			nppParam.safeWow64EnableWow64FsRedirection(FALSE);
			isWow64Off = true;
		}
#endif
		if (doesFileExist(pFn))
		{
			if (isSnapshotMode && !session._mainViewFiles[i]._backupFilePath.empty())
				lastOpened = doOpen(pFn, false, false, session._mainViewFiles[i]._encoding, session._mainViewFiles[i]._backupFilePath.c_str(), session._mainViewFiles[i]._originalFileLastModifTimestamp);
			else
				lastOpened = doOpen(pFn, false, false, session._mainViewFiles[i]._encoding);
		}
		else if (isSnapshotMode && doesFileExist(session._mainViewFiles[i]._backupFilePath.c_str()))
		{
			lastOpened = doOpen(pFn, false, false, session._mainViewFiles[i]._encoding, session._mainViewFiles[i]._backupFilePath.c_str(), session._mainViewFiles[i]._originalFileLastModifTimestamp);
		}
		else
		{
			BufferID foundBufID = MainFileManager.getBufferFromName(pFn);
			if (foundBufID == BUFFER_INVALID)
				lastOpened = nppGUI._keepSessionAbsentFileEntries ? MainFileManager.newPlaceholderDocument(pFn, MAIN_VIEW, userCreatedSessionName) : BUFFER_INVALID;
		}
#ifndef	_WIN64
		if (isWow64Off)
		{
			nppParam.safeWow64EnableWow64FsRedirection(TRUE);
			isWow64Off = false;
		}
#endif
		if (lastOpened != BUFFER_INVALID)
		{
			showView(MAIN_VIEW);
			const wchar_t* pLn = nullptr;
			LangType langTypeToSet = L_TEXT;
			Buffer* buf = MainFileManager.getBufferByID(lastOpened);

			if (!buf->isLargeFile())
			{
				pLn = session._mainViewFiles[i]._langName.c_str();

				int id = getLangFromMenuName(pLn);
				
				if (!id) // it could be due to the hidden language from the sub-menu "Languages"
				{
					for (size_t k = 0; k < nppGUI._excludedLangList.size(); ++k) // try to find it in exclude lang list
					{
						if (nppGUI._excludedLangList[k]._langName == pLn)
						{
							langTypeToSet = nppGUI._excludedLangList[k]._langType;
							break;
						}
					}
				}
				else if (id != IDM_LANG_USER)
					langTypeToSet = menuID2LangType(id);

				if (langTypeToSet == L_EXTERNAL)
					langTypeToSet = (LangType)(id - IDM_LANG_EXTERNAL + L_EXTERNAL);
			}
			

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
			buf->setLangType(langTypeToSet, pLn);
			if (session._mainViewFiles[i]._encoding != -1)
				buf->setEncoding(session._mainViewFiles[i]._encoding);

			buf->setUserReadOnly(session._mainViewFiles[i]._isUserReadOnly);
			buf->setPinned(session._mainViewFiles[i]._isPinned);

			if (isSnapshotMode && !session._mainViewFiles[i]._backupFilePath.empty() && doesFileExist(session._mainViewFiles[i]._backupFilePath.c_str()))
				buf->setDirty(true);

			buf->setRTL(session._mainViewFiles[i]._isRTL);
			if (i == 0 && session._activeMainIndex == 0)
				_mainEditView.changeTextDirection(buf->isRTL());

			_mainDocTab.setIndividualTabColour(lastOpened, session._mainViewFiles[i]._individualTabColour);

			//Force in the document so we can add the markers
			//Don't use default methods because of performance
			Document prevDoc = _mainEditView.execute(SCI_GETDOCPOINTER);
			_mainEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
			_mainEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			_mainEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
			for (size_t j = 0, len = session._mainViewFiles[i]._marks.size(); j < len ; ++j)
			{
				_mainEditView.execute(SCI_MARKERADD, session._mainViewFiles[i]._marks[j], MARK_BOOKMARK);
			}
			_mainEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
			_mainEditView.execute(SCI_SETDOCPOINTER, 0, prevDoc);
			_mainEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
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
		const wchar_t *pFn = session._subViewFiles[k]._fileName.c_str();

		if (isFileSession(pFn) || isFileWorkspace(pFn))
		{
			vector<sessionFileInfo>::iterator posIt = session._subViewFiles.begin() + k;
			session._subViewFiles.erase(posIt);
			continue;	//skip session files, not supporting recursive sessions or embedded workspace files
		}
#ifndef	_WIN64
		bool isWow64Off = false;
		if (!doesFileExist(pFn))
		{
			nppParam.safeWow64EnableWow64FsRedirection(FALSE);
			isWow64Off = true;
		}
#endif
		if (doesFileExist(pFn))
		{
			//check if already open in main. If so, clone
			BufferID clonedBuf = _mainDocTab.findBufferByName(pFn);
			if (clonedBuf != BUFFER_INVALID)
			{
				loadBufferIntoView(clonedBuf, SUB_VIEW);
				lastOpened = clonedBuf;
			}
			else
			{
				if (isSnapshotMode && !session._subViewFiles[k]._backupFilePath.empty())
					lastOpened = doOpen(pFn, false, false, session._subViewFiles[k]._encoding, session._subViewFiles[k]._backupFilePath.c_str(), session._subViewFiles[k]._originalFileLastModifTimestamp);
				else
					lastOpened = doOpen(pFn, false, false, session._subViewFiles[k]._encoding);
			}
		}
		else if (isSnapshotMode && doesFileExist(session._subViewFiles[k]._backupFilePath.c_str()))
		{
			lastOpened = doOpen(pFn, false, false, session._subViewFiles[k]._encoding, session._subViewFiles[k]._backupFilePath.c_str(), session._subViewFiles[k]._originalFileLastModifTimestamp);
		}
		else
		{
			BufferID foundBufID = MainFileManager.getBufferFromName(pFn);
			if (foundBufID == BUFFER_INVALID)
				lastOpened = nppGUI._keepSessionAbsentFileEntries ? MainFileManager.newPlaceholderDocument(pFn, SUB_VIEW, userCreatedSessionName) : BUFFER_INVALID;
		}
#ifndef	_WIN64
		if (isWow64Off)
		{
			nppParam.safeWow64EnableWow64FsRedirection(TRUE);
			isWow64Off = false;
		}
#endif

		if (lastOpened != BUFFER_INVALID)
		{
			showView(SUB_VIEW);
			if (canHideView(MAIN_VIEW))
				hideView(MAIN_VIEW);
			const wchar_t *pLn = session._subViewFiles[k]._langName.c_str();
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
				if (!lstrcmp(pLn, L"User Defined"))
				{
					pLn = L"";	//default user defined
				}
			}
			buf->setLangType(typeToSet, pLn);
			buf->setEncoding(session._subViewFiles[k]._encoding);
			buf->setUserReadOnly(session._subViewFiles[k]._isUserReadOnly);
			buf->setPinned(session._subViewFiles[k]._isPinned);

			if (isSnapshotMode && !session._subViewFiles[k]._backupFilePath.empty() && doesFileExist(session._subViewFiles[k]._backupFilePath.c_str()))
				buf->setDirty(true);

			buf->setRTL(session._subViewFiles[k]._isRTL);

			_subDocTab.setIndividualTabColour(lastOpened, session._subViewFiles[k]._individualTabColour);

			//Force in the document so we can add the markers
			//Don't use default methods because of performance
			Document prevDoc = _subEditView.execute(SCI_GETDOCPOINTER);
			_subEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
			_subEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			_subEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
			for (size_t j = 0, len = session._subViewFiles[k]._marks.size(); j < len ; ++j)
			{
				_subEditView.execute(SCI_MARKERADD, session._subViewFiles[k]._marks[j], MARK_BOOKMARK);
			}
			_subEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
			_subEditView.execute(SCI_SETDOCPOINTER, 0, prevDoc);
			_subEditView.execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);

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

	if (session._activeMainIndex < session._mainViewFiles.size())
	{
		const wchar_t* fileName = session._mainViewFiles[session._activeMainIndex]._fileName.c_str();
		BufferID buf = _mainDocTab.findBufferByName(fileName);
		if (buf != BUFFER_INVALID)
			activateBuffer(buf, MAIN_VIEW);
	}

	if (session._activeSubIndex < session._subViewFiles.size())
	{
		const wchar_t* fileName = session._subViewFiles[session._activeSubIndex]._fileName.c_str();
		BufferID buf = _subDocTab.findBufferByName(fileName);
		if (buf != BUFFER_INVALID)
			activateBuffer(buf, SUB_VIEW);
	}

	if ((session.nbSubFiles() > 0) && (session._activeView == MAIN_VIEW || session._activeView == SUB_VIEW))
		switchEditViewTo(static_cast<int32_t>(session._activeView));
	else
		switchEditViewTo(MAIN_VIEW);

	if (canHideView(otherView()))
		hideView(otherView());
	else if (canHideView(currentView()))
		hideView(currentView());

	checkSyncState();

	if (_pDocumentListPanel)
		_pDocumentListPanel->reload();

	if (userCreatedSessionName && !session._fileBrowserRoots.empty())
	{
		// If the session is user's created session but not session.xml, we force to launch Folder as Workspace and add roots
		launchFileBrowser(session._fileBrowserRoots, session._fileBrowserSelectedItem, true);
	}

	// Especially File status auto-detection set on "Enable for all opened files":  nppGUI._fileAutoDetection & cdEnabledOld
	// when "Remember inaccessible files from past session" is enabled:             nppGUI._keepSessionAbsentFileEntries
	// there are some (or 1) absent files:                                          nppParam.theWarningHasBeenGiven()
	// and user want to create the placeholders for these files:                    nppParam.isPlaceHolderEnabled()
	//
	// When above conditions are true, the created placeholders are not read-only, due to the lack of file-detection on them.
	if (nppGUI._keepSessionAbsentFileEntries && nppParam.theWarningHasBeenGiven() && nppParam.isPlaceHolderEnabled() && (nppGUI._fileAutoDetection & cdEnabledOld))
	{
		checkModifiedDocument(false); // so here we launch file-detection for all placeholders manually
	}

	return allSessionFilesLoaded;
}

bool Notepad_plus::fileLoadSession(const wchar_t *fn)
{
	bool result = false;
	wstring sessionFileName;
	if (fn == NULL)
	{
		CustomFileDialog fDlg(_pPublicInterface->getHSelf());
		const wchar_t *ext = NppParameters::getInstance().getNppGUI()._definedSessionExt.c_str();
		if (*ext != '\0')
		{
			wstring sessionExt = L"";
			if (*ext != '.')
				sessionExt += L".";
			sessionExt += ext;
			fDlg.setExtFilter(L"Session file", sessionExt.c_str());
			fDlg.setDefExt(ext);
		}
		fDlg.setExtFilter(L"All types", L".*");
		wstring localizedTitle = _nativeLangSpeaker.getNativeLangMenuString(IDM_FILE_LOADSESSION, L"Load Session", true);
		fDlg.setTitle(localizedTitle.c_str());
		sessionFileName = fDlg.doOpenSingleFileDlg();
	}
	else
	{
		if (doesFileExist(fn))
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
			wchar_t nppFullPath[MAX_PATH]{};
			::GetModuleFileName(NULL, nppFullPath, MAX_PATH);

			wstring args = L"-multiInst -nosession -openSession ";
			args += L"\"";
			args += sessionFileName;
			args += L"\"";
			if (::ShellExecute(_pPublicInterface->getHSelf(), L"open", nppFullPath, args.c_str(), L".", SW_SHOW) > (HINSTANCE)32)
				result = true;
		}
		else
		{
			Session session2Load;

			if (nppParam.loadSession(session2Load, sessionFileName.c_str()))
			{
				const bool isSnapshotMode = false;
				result = loadSession(session2Load, isSnapshotMode, sessionFileName.c_str());

				if (isEmptyNpp && nppGUI._multiInstSetting == multiInstOnSession)
					nppParam.setLoadedSessionFilePath(sessionFileName);
			}
		}
	}

	return result;
}

const wchar_t * Notepad_plus::fileSaveSession(size_t nbFile, wchar_t ** fileNames, const wchar_t *sessionFile2save, bool includeFileBrowser)
{
	if (sessionFile2save && (lstrlen(sessionFile2save) > 0))
	{
		Session currentSession;
		if ((nbFile) && (fileNames))
		{
			for (size_t i = 0 ; i < nbFile ; ++i)
			{
				if (doesFileExist(fileNames[i]))
					currentSession._mainViewFiles.push_back(wstring(fileNames[i]));
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

const wchar_t * Notepad_plus::fileSaveSession(size_t nbFile, wchar_t ** fileNames)
{
	CustomFileDialog fDlg(_pPublicInterface->getHSelf());
	const wchar_t *ext = NppParameters::getInstance().getNppGUI()._definedSessionExt.c_str();

	if (*ext != '\0')
	{
		wstring sessionExt = L"";
		if (*ext != '.')
			sessionExt += L".";
		sessionExt += ext;
		fDlg.setExtFilter(L"Session file", sessionExt.c_str());
		fDlg.setDefExt(ext);
		fDlg.setExtIndex(0);		// 0 index for "custom extension types"
	}
	fDlg.setExtFilter(L"All types", L".*");
	const bool isCheckboxActive = _pFileBrowser && !_pFileBrowser->isClosed();
	const wstring checkboxLabel = _nativeLangSpeaker.getLocalizedStrFromID("session-save-folder-as-workspace", L"Save Folder as Workspace");
	fDlg.setCheckbox(checkboxLabel.c_str(), isCheckboxActive);
	wstring localizedTitle = _nativeLangSpeaker.getNativeLangMenuString(IDM_FILE_SAVESESSION, L"Save Session", true);
	fDlg.setTitle(localizedTitle.c_str());
	wstring sessionFileName = fDlg.doSaveDlg();

	if (!sessionFileName.empty())
		return fileSaveSession(nbFile, fileNames, sessionFileName.c_str(), fDlg.getCheckboxState());

	return NULL;
}


void Notepad_plus::saveSession(const Session & session)
{
	(NppParameters::getInstance()).writeSession(session);
}


void Notepad_plus::saveCurrentSession()
{
	::SendMessage(_pPublicInterface->getHSelf(), NPPM_INTERNAL_SAVECURRENTSESSION, 0, 0);
}
