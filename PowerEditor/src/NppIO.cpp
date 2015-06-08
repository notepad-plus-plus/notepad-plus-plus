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


#include <time.h>
#include <shlwapi.h>
#include "Notepad_plus_Window.h"
#include "FileDialog.h"
#include "EncodingMapper.h"
#include "VerticalFileSwitcher.h"
#include "functionListPanel.h"
#include <TCHAR.h>

using namespace std;

BufferID Notepad_plus::doOpen(const TCHAR *fileName, bool isRecursive, bool isReadOnly, int encoding, const TCHAR *backupFileName, time_t fileNameTimestamp)
{
 
	const rsize_t longFileNameBufferSize = MAX_PATH;

	//If [GetFullPathName] succeeds, the return value is the length, in TCHARs, of the string copied to lpBuffer, not including the terminating null character.
	//If the lpBuffer buffer is too small to contain the path, the return value [of GetFullPathName] is the size, in TCHARs, of the buffer that is required to hold the path and the terminating null character.
	//If [GetFullPathName] fails for any other reason, the return value is zero.

    NppParameters *pNppParam = NppParameters::getInstance();
    TCHAR longFileName[longFileNameBufferSize];

    const DWORD getFullPathNameResult = ::GetFullPathName(fileName, longFileNameBufferSize, longFileName, NULL);
	if ( getFullPathNameResult == 0 )
	{
		return BUFFER_INVALID;
	}
	if ( getFullPathNameResult > longFileNameBufferSize )
	{
		return BUFFER_INVALID;
	}
	assert( _tcslen( longFileName ) == getFullPathNameResult );
	
	// ignore the returned value of function due to win64 redirection system
	::GetLongPathName(longFileName, longFileName, longFileNameBufferSize);

	bool isSnapshotMode = backupFileName != NULL && PathFileExists(backupFileName);
	if (isSnapshotMode && !PathFileExists(longFileName)) // UNTITLED
	{
		lstrcpy(longFileName, fileName);
	}


    _lastRecentFileList.remove(longFileName);

    const TCHAR * fileName2Find;
    generic_string gs_fileName = fileName;
    size_t res = gs_fileName.find_first_of(UNTITLED_STR);
     
    if (res != string::npos && res == 0)
    {
        fileName2Find = fileName;
    }
    else
    {
        fileName2Find = longFileName;
    }

    BufferID test = MainFileManager->getBufferFromName(fileName2Find);
    if (test != BUFFER_INVALID && !isSnapshotMode)
    {
        //switchToFile(test);
        //Dont switch, not responsibility of doOpen, but of caller
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
        return test;
    }

    if (isFileSession(longFileName) && PathFileExists(longFileName))
    {
        fileLoadSession(longFileName);
        return BUFFER_INVALID;
    }

    bool isWow64Off = false;
    if (!PathFileExists(longFileName))
    {
        pNppParam->safeWow64EnableWow64FsRedirection(FALSE);
        isWow64Off = true;
    }

    bool globbing = wcsrchr(longFileName, TCHAR('*')) || wcsrchr(longFileName, TCHAR('?'));
	bool isOpenningNewEmptyFile = false;

	if (!isSnapshotMode) // if not backup mode, or backupfile path is invalid
	{
		if (!PathFileExists(longFileName) && !globbing)
		{
			TCHAR str2display[MAX_PATH*2];
			generic_string longFileDir(longFileName);
			PathRemoveFileSpec(longFileDir);

			bool isCreateFileSuccessful = false;
			if (PathFileExists(longFileDir.c_str()))
			{
				wsprintf(str2display, TEXT("%s doesn't exist. Create it?"), longFileName);
				if (::MessageBox(_pPublicInterface->getHSelf(), str2display, TEXT("Create new file"), MB_YESNO) == IDYES)
				{
					bool res = MainFileManager->createEmptyFile(longFileName);
					if (res)
					{
						isCreateFileSuccessful = true;
						isOpenningNewEmptyFile = true;
					}
					else
					{
						wsprintf(str2display, TEXT("Cannot create the file \"%s\""), longFileName);
						::MessageBox(_pPublicInterface->getHSelf(), str2display, TEXT("Create new file"), MB_OK);
					}
				}
			}

			if (!isCreateFileSuccessful)
			{
				if (isWow64Off)
				{
					pNppParam->safeWow64EnableWow64FsRedirection(TRUE);
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
    scnN.nmhdr.idFrom = NULL;
    _pluginsManager.notify(&scnN);

    if (encoding == -1)
    {
		encoding = getHtmlXmlEncoding(longFileName);
    }
   
	BufferID buffer;
	if (isSnapshotMode)
	{
		buffer = MainFileManager->loadFile(longFileName, NULL, encoding, backupFileName, fileNameTimestamp);

		if (buffer != BUFFER_INVALID)
		{
			bool isSnapshotMode = backupFileName != NULL && PathFileExists(backupFileName);
			if (isSnapshotMode)
			{
				// To notify plugins that a snapshot dirty file is loaded on startup
				SCNotification scnN;
				scnN.nmhdr.hwndFrom = 0;
				scnN.nmhdr.idFrom = (uptr_t)buffer;
				scnN.nmhdr.code = NPPN_SNAPSHOTDIRTYFILELOADED;
				_pluginsManager.notify(&scnN);

				buffer->setLoadedDirty(true);
			}
		}
	}
	else
	{
		buffer = MainFileManager->loadFile(longFileName, NULL, encoding);
	}

    if (buffer != BUFFER_INVALID)
    {
        _isFileOpening = true;

        Buffer * buf = MainFileManager->getBufferByID(buffer);

        // if file is read only, we set the view read only
        if (isReadOnly)
            buf->setUserReadOnly(true);

		// if it's a new created file, then use new file default settings
		if (isOpenningNewEmptyFile)
		{
			const NewDocDefaultSettings & ndds = (NppParameters::getInstance()->getNppGUI()).getNewDocDefaultSettings();
			buf->setEncoding(ndds._codepage);
			buf->setFormat(ndds._format);
			buf->setUnicodeMode(ndds._unicodeMode);
			buf->setLangType(ndds._lang);
		}

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
        if (_pFileSwitcherPanel)
            _pFileSwitcherPanel->newItem((int)buf, currentView());
    }
    else
    {
        if (globbing || ::PathIsDirectory(fileName))
        {
            vector<generic_string> fileNames;
            vector<generic_string> patterns;
            if (globbing)
            {
                const TCHAR * substring = wcsrchr(fileName, TCHAR('\\'));
                size_t pos = substring - fileName;

                patterns.push_back(substring + 1);
                generic_string dir(fileName, pos + 1);
                getMatchedFileNames(dir.c_str(), patterns, fileNames, isRecursive, false);
            }
            else
            {
                generic_string fileNameStr = fileName;
                if (fileName[lstrlen(fileName) - 1] != '\\')
                    fileNameStr += TEXT("\\");

                patterns.push_back(TEXT("*"));
                getMatchedFileNames(fileNameStr.c_str(), patterns, fileNames, true, false);
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
                                                nbFiles2Open);
            }

            if (ok2Open)
            {
                for (size_t i = 0 ; i < nbFiles2Open ; ++i)
                {
                    doOpen(fileNames[i].c_str());
                }
            }
        }
        else
        {
            generic_string msg = TEXT("Can not open file \"");
            msg += longFileName;
            msg += TEXT("\".");
            ::MessageBox(_pPublicInterface->getHSelf(), msg.c_str(), TEXT("ERROR"), MB_OK);
            _isFileOpening = false;

            scnN.nmhdr.code = NPPN_FILELOADFAILED;
            _pluginsManager.notify(&scnN);
        }
    }

    if (isWow64Off)
    {
        pNppParam->safeWow64EnableWow64FsRedirection(TRUE);
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
	if (mainVisisble) {
		_mainEditView.saveCurrentPos();
		_mainEditView.execute(SCI_SETDOCPOINTER, 0, 0);
	}
	if (subVisisble) {
		_subEditView.saveCurrentPos();
		_subEditView.execute(SCI_SETDOCPOINTER, 0, 0);
	}

	if (!mainVisisble && !subVisisble) {
		return MainFileManager->reloadBufferDeferred(id);
	}

	bool res = MainFileManager->reloadBuffer(id);
	Buffer * pBuf = MainFileManager->getBufferByID(id);
	if (mainVisisble) {
		_mainEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
		_mainEditView.restoreCurrentPos();
	}
	if (subVisisble) {
		_subEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
		_subEditView.restoreCurrentPos();
	}
	return res;
}

bool Notepad_plus::doSave(BufferID id, const TCHAR * filename, bool isCopy)
{
	SCNotification scnN;
	// Notify plugins that current file is about to be saved
	if (!isCopy)
	{
		
		scnN.nmhdr.code = NPPN_FILEBEFORESAVE;
		scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
		scnN.nmhdr.idFrom = (uptr_t)id;
		_pluginsManager.notify(&scnN);
	}

	generic_string error_msg;
	bool res = MainFileManager->saveBuffer(id, filename, isCopy, &error_msg);

	if (!isCopy)
	{
		scnN.nmhdr.code = NPPN_FILESAVED;
		_pluginsManager.notify(&scnN);
	}

	if (!res)
	{
		// try to open Notepad++ in admin mode
		if (!_isAdministrator)
		{
			bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
			if (isSnapshotMode) // if both rememberSession && backup mode are enabled
			{                   // Open the 2nd Notepad++ instance in Admin mode, then close the 1st instance.
				int openInAdminModeRes = _nativeLangSpeaker.messageBox("OpenInAdminMode",
				_pPublicInterface->getHSelf(),
				TEXT("The file cannot be saved and it may be protected.\rDo you want to launch Notepad++ in Administrator mode?"),
				TEXT("Save failed"),
				MB_YESNO);

				if (openInAdminModeRes == IDYES)
				{
					TCHAR nppFullPath[MAX_PATH];
					::GetModuleFileName(NULL, nppFullPath, MAX_PATH);

					generic_string args = TEXT("-multiInst");
					size_t res = (size_t)::ShellExecute(_pPublicInterface->getHSelf(), TEXT("runas"), nppFullPath, args.c_str(), TEXT("."), SW_SHOW);

					// If the function succeeds, it returns a value greater than 32. If the function fails,
					// it returns an error value that indicates the cause of the failure.
					// https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx

					if (res < 32)
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

					BufferID bufferID = bufferID = _pEditView->getCurrentBufferID();
					Buffer * buf = MainFileManager->getBufferByID(bufferID);

					//process the fileNamePath into LRF
					generic_string fileNamePath = buf->getFullPathName();

					generic_string args = TEXT("-multiInst -nosession ");
					args += TEXT("\"");
					args += fileNamePath;
					args += TEXT("\"");
					size_t res = (size_t)::ShellExecute(_pPublicInterface->getHSelf(), TEXT("runas"), nppFullPath, args.c_str(), TEXT("."), SW_SHOW);

					// If the function succeeds, it returns a value greater than 32. If the function fails,
					// it returns an error value that indicates the cause of the failure.
					// https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx

					if (res < 32)
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
		else
		{

			if (error_msg.empty())
			{
				_nativeLangSpeaker.messageBox("FileLockedWarning",
					_pPublicInterface->getHSelf(),
					TEXT("Please check if this file is opened in another program."),
					TEXT("Save failed"),
					MB_OK);
			}
			else
			{
				::MessageBox(_pPublicInterface->getHSelf(), error_msg.c_str(), TEXT("Save failed"), MB_OK);
			}
		}
	}

	if (res && _pFuncList && (!_pFuncList->isClosed()) && _pFuncList->isVisible())
	{
		_pFuncList->reload();
	}
	return res;
}

void Notepad_plus::doClose(BufferID id, int whichOne, bool doDeleteBackup)
{
	DocTabView *tabToClose = (whichOne == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
	int i = tabToClose->getIndexByBuffer(id);
	if (i == -1)
		return;

	if (doDeleteBackup)
		MainFileManager->deleteCurrentBufferBackup();

	Buffer * buf = MainFileManager->getBufferByID(id);

	// Notify plugins that current file is about to be closed
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_FILEBEFORECLOSE;
	scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
	scnN.nmhdr.idFrom = (uptr_t)id;
	_pluginsManager.notify(&scnN);

	//add to recent files if its an existing file
	if (!buf->isUntitled())
	{
		// if the file doesn't exist, it could be redirected
		// So we turn Wow64 off
		bool isWow64Off = false;
		NppParameters *pNppParam = NppParameters::getInstance();
		const TCHAR *fn = buf->getFullPathName();
		if (!PathFileExists(fn))
		{
			pNppParam->safeWow64EnableWow64FsRedirection(FALSE);
			isWow64Off = true;
		}

		if (PathFileExists(buf->getFullPathName()))
			_lastRecentFileList.add(buf->getFullPathName());

		// We enable Wow64 system, if it was disabled
		if (isWow64Off)
		{
			pNppParam->safeWow64EnableWow64FsRedirection(TRUE);
			//isWow64Off = false;
		}
	}

	int nrDocs = whichOne==MAIN_VIEW?(_mainDocTab.nbItem()):(_subDocTab.nbItem());

	//Do all the works
	bool isBufRemoved = removeBufferFromView(id, whichOne);
	int hiddenBufferID = -1;
	if (nrDocs == 1 && canHideView(whichOne))
	{	//close the view if both visible
		hideView(whichOne);

		// if the current activated buffer is in this view, 
		// then get buffer ID to remove the entry from File Switcher Pannel
		hiddenBufferID = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_GETBUFFERIDFROMPOS, 0, whichOne);
	}

	// Notify plugins that current file is closed
	if (isBufRemoved)
	{
		scnN.nmhdr.code = NPPN_FILECLOSED;
		_pluginsManager.notify(&scnN);

		// The document could be clonned.
		// if the same buffer ID is not found then remove the entry from File Switcher Pannel
		if (_pFileSwitcherPanel)
		{
			//int posInfo = ::SendMessage(_pPublicInterface->getHSelf(), NPPM_GETPOSFROMBUFFERID, (WPARAM)id ,0);
				
			_pFileSwitcherPanel->closeItem((int)id, whichOne);

			if (hiddenBufferID != -1)
				_pFileSwitcherPanel->closeItem((int)hiddenBufferID, whichOne);
		}
	}
	command(IDM_VIEW_REFRESHTABAR);
	return;
}

generic_string Notepad_plus::exts2Filters(generic_string exts) const
{
	const TCHAR *extStr = exts.c_str();
	TCHAR aExt[MAX_PATH];
	generic_string filters(TEXT(""));

	int j = 0;
	bool stop = false;
	for (size_t i = 0, len = exts.length(); i < len ; ++i)
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

int Notepad_plus::setFileOpenSaveDlgFilters(FileDialog & fDlg, int langType)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	int i = 0;
	Lang *l = NppParameters::getInstance()->getLangFromIndex(i++);

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

			LexerStylerArray &lsa = (NppParameters::getInstance())->getLStylerArray();
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
			
			generic_string stringFilters = exts2Filters(list);
			const TCHAR *filters = stringFilters.c_str();
			if (filters[0])
			{
				fDlg.setExtsFilter(getLangDesc(lid, false).c_str(), filters);

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
		l = (NppParameters::getInstance())->getLangFromIndex(i++);
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
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

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
	//first check amount of documents, we dont want the view to hide if we closed a secondary doc with primary being empty
	//int nrDocs = _pDocTab->nbItem();
	bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
	doClose(bufferID, viewToClose, isSnapshotMode);
	return true;
}

bool Notepad_plus::fileCloseAll(bool doDeleteBackup, bool isSnapshotMode)
{
	//closes all documents, makes the current view the only one visible

	//first check if we need to save any file
	for(int i = 0; i < _mainDocTab.nbItem(); ++i)
	{
		BufferID id = _mainDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(id);
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
					if(!activateBuffer(id, SUB_VIEW))
						switchEditViewTo(MAIN_VIEW);
	
					TCHAR pattern[140] = TEXT("Your backup file cannot be found (deleted from outside).\rSave it otherwise your data will be lost\rDo you want to save file \"%s\" ?");
					TCHAR phrase[512];
					wsprintf(phrase, pattern, buf->getFullPathName());
					int res = doActionOrNot(TEXT("Save"), phrase, MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
					//int res = doSaveOrNot(buf->getFullPathName());
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
				if(!activateBuffer(id, SUB_VIEW))
					switchEditViewTo(MAIN_VIEW);

				int res = doSaveOrNot(buf->getFullPathName());
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
	}
	for(int i = 0; i < _subDocTab.nbItem(); ++i) 
	{
		BufferID id = _subDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(id);
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
	
					TCHAR pattern[140] = TEXT("Your backup file cannot be found (deleted from outside).\rSave it otherwise your data will be lost\rDo you want to save file \"%s\" ?");
					TCHAR phrase[512];
					wsprintf(phrase, pattern, buf->getFullPathName());
					int res = doActionOrNot(TEXT("Save"), phrase, MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL);
					//int res = doSaveOrNot(buf->getFullPathName());
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

				int res = doSaveOrNot(buf->getFullPathName());
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
			}
		}
	}

	//Then start closing, inactive view first so the active is left open
    if (bothActive())
    {	//first close all docs in non-current view, which gets closed automatically
		//Set active tab to the last one closed.
		activateBuffer(_pNonDocTab->getBufferByIndex(0), otherView());
		for(int i = _pNonDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
			doClose(_pNonDocTab->getBufferByIndex(i), otherView(), doDeleteBackup);
		}
		//hideView(otherView());
    }

	activateBuffer(_pDocTab->getBufferByIndex(0), currentView());
	for(int i = _pDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
		doClose(_pDocTab->getBufferByIndex(i), currentView(), doDeleteBackup);
	}
	return true;
}

bool Notepad_plus::fileCloseAllGiven(const std::vector<int> &krvecBufferIndexes)
{
	// First check if we need to save any file.

	std::vector<int>::const_iterator itIndexesEnd = krvecBufferIndexes.end();

	for(std::vector<int>::const_iterator itIndex = krvecBufferIndexes.begin(); itIndex != itIndexesEnd; ++itIndex) {
		BufferID id = _pDocTab->getBufferByIndex(*itIndex);
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (buf->isUntitled() && buf->docLength() == 0)
		{
			// Do nothing.
		}
		else if (buf->isDirty()) 
		{
			if(_activeView == MAIN_VIEW)
			{
				activateBuffer(id, MAIN_VIEW);
				if(!activateBuffer(id, SUB_VIEW))
					switchEditViewTo(MAIN_VIEW);
			}
			else
			{
				activateBuffer(id, SUB_VIEW);
				switchEditViewTo(SUB_VIEW);
			}

			int res = doSaveOrNot(buf->getFullPathName());
			if (res == IDYES) 
			{
				if (!fileSave(id))
					return false;	// Abort entire procedure.
			} 
			else if (res == IDCANCEL)
			{
					return false;
			}
		}
	}

	// Now we close.
	bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
	for(std::vector<int>::const_iterator itIndex = krvecBufferIndexes.begin(); itIndex != itIndexesEnd; ++itIndex) {
		doClose(_pDocTab->getBufferByIndex(*itIndex), currentView(), isSnapshotMode);
	}

	return true;
}

bool Notepad_plus::fileCloseAllToLeft()
{
	// Indexes must go from high to low to deal with the fact that when one index is closed, any remaining
	// indexes (smaller than the one just closed) will point to the wrong tab.
	std::vector<int> vecIndexesToClose;
	for(int i = _pDocTab->getCurrentTabIndex() - 1; i >= 0; i--) {
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
	for(int i = _pDocTab->nbItem() - 1; i > kiActive; i--) {
		vecIndexesToClose.push_back(i);
	}
	return fileCloseAllGiven(vecIndexesToClose);
}

bool Notepad_plus::fileCloseAllButCurrent()
{
	BufferID current = _pEditView->getCurrentBufferID();
	int active = _pDocTab->getCurrentTabIndex();
	//closes all documents, makes the current view the only one visible

	//first check if we need to save any file
	for(int i = 0; i < _mainDocTab.nbItem(); ++i) {
		BufferID id = _mainDocTab.getBufferByIndex(i);
		if (id == current)
			continue;
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (buf->isUntitled() && buf->docLength() == 0)
		{
			// Do nothing
		}
		else if (buf->isDirty()) 
		{
			activateBuffer(id, MAIN_VIEW);
			if(!activateBuffer(id, SUB_VIEW))
				switchEditViewTo(MAIN_VIEW);

			int res = doSaveOrNot(buf->getFullPathName());
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
	for(int i = 0; i < _subDocTab.nbItem(); ++i) 
	{
		BufferID id = _subDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(id);
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

			int res = doSaveOrNot(buf->getFullPathName());
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

	bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
	//Then start closing, inactive view first so the active is left open
    if (bothActive())
    {	//first close all docs in non-current view, which gets closed automatically
		//Set active tab to the last one closed.
		activateBuffer(_pNonDocTab->getBufferByIndex(0), otherView());
		
		for(int i = _pNonDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
			doClose(_pNonDocTab->getBufferByIndex(i), otherView(), isSnapshotMode);
		}
		//hideView(otherView());
    }

	activateBuffer(_pDocTab->getBufferByIndex(0), currentView());
	for(int i = _pDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
		if (i == active) {	//dont close active index
			continue;
		}
		doClose(_pDocTab->getBufferByIndex(i), currentView(), isSnapshotMode);
	}
	return true;
}

bool Notepad_plus::fileSave(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

	if (!buf->getFileReadOnly() && buf->isDirty())	//cannot save if readonly
	{
		const TCHAR *fn = buf->getFullPathName();
		if (buf->isUntitled())
		{
			return fileSaveAs(bufferID);
		}
		else
		{
			const NppGUI & nppgui = (NppParameters::getInstance())->getNppGUI();
			BackupFeature backup = nppgui._backup;
			TCHAR *name = ::PathFindFileName(fn);

			if (backup == bak_simple)
			{
				//copy fn to fn.backup
				generic_string fn_bak(fn);
				if ((nppgui._useDir) && (nppgui._backupDir != TEXT("")))
				{
					fn_bak = nppgui._backupDir;
					fn_bak += TEXT("\\");
					fn_bak += name;
				}
				else
				{
					fn_bak = fn;
				}
				fn_bak += TEXT(".bak");
				::CopyFile(fn, fn_bak.c_str(), FALSE);
			}
			else if (backup == bak_verbose)
			{
				generic_string fn_dateTime_bak(TEXT(""));
				
				if ((nppgui._useDir) && (nppgui._backupDir != TEXT("")))
				{
					fn_dateTime_bak = nppgui._backupDir;
					fn_dateTime_bak += TEXT("\\");
				}
				else
				{
					const TCHAR *bakDir = TEXT("nppBackup");

					// std::string path should be a temp throwable variable 
					generic_string path = fn;
					::PathRemoveFileSpec(path);
					fn_dateTime_bak = path.c_str();
					

					fn_dateTime_bak += TEXT("\\");
					fn_dateTime_bak += bakDir;
					fn_dateTime_bak += TEXT("\\");

					if (!::PathFileExists(fn_dateTime_bak.c_str()))
					{
						::CreateDirectory(fn_dateTime_bak.c_str(), NULL);
					}
				}

				fn_dateTime_bak += name;

				const int temBufLen = 32;
				TCHAR tmpbuf[temBufLen];
				time_t ltime = time(0);
				struct tm *today;

				today = localtime(&ltime);
				generic_strftime(tmpbuf, temBufLen, TEXT("%Y-%m-%d_%H%M%S"), today);

				fn_dateTime_bak += TEXT(".");
				fn_dateTime_bak += tmpbuf;
				fn_dateTime_bak += TEXT(".bak");

				::CopyFile(fn, fn_dateTime_bak.c_str(), FALSE);
			}
			return doSave(bufferID, buf->getFullPathName(), false);
		}
	}
	return false;
}

bool Notepad_plus::fileSaveAll() {
	if (viewVisible(MAIN_VIEW)) {
		for(int i = 0; i < _mainDocTab.nbItem(); ++i) {
			BufferID idToSave = _mainDocTab.getBufferByIndex(i);
			fileSave(idToSave);
		}
	}

	if (viewVisible(SUB_VIEW)) {
		for(int i = 0; i < _subDocTab.nbItem(); ++i) {
			BufferID idToSave = _subDocTab.getBufferByIndex(i);
			fileSave(idToSave);
		}
	}
	checkDocState();
	return true;
}

bool Notepad_plus::fileSaveAs(BufferID id, bool isSaveCopy)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

	FileDialog fDlg(_pPublicInterface->getHSelf(), _pPublicInterface->getHinst());

    fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	int langTypeIndex = setFileOpenSaveDlgFilters(fDlg, buf->getLangType());
	fDlg.setDefFileName(buf->getFileName());
	
    fDlg.setExtIndex(langTypeIndex+1); // +1 for "All types"

	// Disable file autodetection before opening save dialog to prevent use-after-delete bug.
	NppParameters *pNppParam = NppParameters::getInstance();
	ChangeDetect cdBefore = ((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection;
	((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection = cdDisabled;

	TCHAR *pfn = fDlg.doSaveDlg();

	// Enable file autodetection again.
	((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection = cdBefore;

	if (pfn)
	{
		BufferID other = _pNonDocTab->findBufferByName(pfn);
		if (other == BUFFER_INVALID)	//can save, other view doesnt contain buffer
		{
			bool res = doSave(bufferID, pfn, isSaveCopy);
			//buf->setNeedsLexing(true);	//commented to fix wrapping being removed after save as (due to SCI_CLEARSTYLE or something, seems to be Scintilla bug)
			//Changing lexer after save seems to work properly
			return res;
		}
		else		//cannot save, other view has buffer already open, activate it
		{
			_nativeLangSpeaker.messageBox("FileAlreadyOpenedInNpp",
				_pPublicInterface->getHSelf(),
				TEXT("The file is already opened in the Notepad++."),
				TEXT("ERROR"),
				MB_OK | MB_ICONSTOP);
			switchToFile(other);
			return false;
		}
	}
	else // cancel button is pressed
    {
        checkModifiedDocument();
		return false;
    }
}

bool Notepad_plus::fileRename(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

	SCNotification scnN;
	scnN.nmhdr.code = NPPN_FILEBEFORERENAME;
	scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
	scnN.nmhdr.idFrom = (uptr_t)bufferID;
	_pluginsManager.notify(&scnN);

	FileDialog fDlg(_pPublicInterface->getHSelf(), _pPublicInterface->getHinst());

    fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	setFileOpenSaveDlgFilters(fDlg);
			
	fDlg.setDefFileName(buf->getFileName());
	TCHAR *pfn = fDlg.doSaveDlg();

	bool success = false;
	if (pfn)
		success = MainFileManager->moveFile(bufferID, pfn);

	scnN.nmhdr.code = success ? NPPN_FILERENAMED : NPPN_FILERENAMECANCEL;
	_pluginsManager.notify(&scnN);

	return success;
}


bool Notepad_plus::fileDelete(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	
	Buffer * buf = MainFileManager->getBufferByID(bufferID);
	const TCHAR *fileNamePath = buf->getFullPathName();

	winVer winVersion = (NppParameters::getInstance())->getWinVersion();
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

		if (!MainFileManager->deleteFile(bufferID))
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
		bool isSnapshotMode = NppParameters::getInstance()->getNppGUI().isSnapshotMode();
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
    FileDialog fDlg(_pPublicInterface->getHSelf(), _pPublicInterface->getHinst());
	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	
	setFileOpenSaveDlgFilters(fDlg);

	BufferID lastOpened = BUFFER_INVALID;
	if (stringVector *pfns = fDlg.doOpenMultiFilesDlg())
	{
		size_t sz = pfns->size();
		for (size_t i = 0 ; i < sz ; ++i) {
			BufferID test = doOpen(pfns->at(i).c_str(), fDlg.isReadOnly());
			if (test != BUFFER_INVALID)
				lastOpened = test;
		}
	}
	if (lastOpened != BUFFER_INVALID) {
		switchToFile(lastOpened);
	}
}

void Notepad_plus::fileNew()
{
    BufferID newBufID = MainFileManager->newEmptyDocument();
	
    loadBufferIntoView(newBufID, currentView(), true);	//true, because we want multiple new files if possible
    activateBuffer(newBufID, currentView());
}

bool Notepad_plus::isFileSession(const TCHAR * filename) {
	// if file2open matches the ext of user defined session file ext, then it'll be opened as a session
	const TCHAR *definedSessionExt = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();
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

void Notepad_plus::loadLastSession()
{
	NppParameters *nppParams = NppParameters::getInstance();
	const NppGUI & nppGui = nppParams->getNppGUI();
	Session lastSession = nppParams->getSession();
	bool isSnapshotMode = nppGui.isSnapshotMode();
    loadSession(lastSession, isSnapshotMode);
}

bool Notepad_plus::loadSession(Session & session, bool isSnapshotMode)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	bool allSessionFilesLoaded = true;
	BufferID lastOpened = BUFFER_INVALID;
	//size_t i = 0;
	showView(MAIN_VIEW);
	switchEditViewTo(MAIN_VIEW);	//open files in main

	int mainIndex2Update = -1;

	for (size_t i = 0; i < session.nbMainFiles() ; )
	{
		const TCHAR *pFn = session._mainViewFiles[i]._fileName.c_str();

		if (isFileSession(pFn))
		{
			vector<sessionFileInfo>::iterator posIt = session._mainViewFiles.begin() + i;
			session._mainViewFiles.erase(posIt);
			continue;	//skip session files, not supporting recursive sessions
		}

		bool isWow64Off = false;
		if (!PathFileExists(pFn))
		{
			pNppParam->safeWow64EnableWow64FsRedirection(FALSE);
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
			pNppParam->safeWow64EnableWow64FsRedirection(TRUE);
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

			Buffer *buf = MainFileManager->getBufferByID(lastOpened);

			if (session._mainViewFiles[i]._foldStates.size() > 0)
			{
				if (buf == _mainEditView.getCurrentBuffer()) // current document
					// Set floding state in the current doccument
					mainIndex2Update = i;
				else
					// Set fold states in the buffer
					buf->setHeaderLineState(session._mainViewFiles[i]._foldStates, &_mainEditView);
			}

			buf->setPosition(session._mainViewFiles[i], &_mainEditView);
			buf->setLangType(typeToSet, pLn);
			if (session._mainViewFiles[i]._encoding != -1)
				buf->setEncoding(session._mainViewFiles[i]._encoding);

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
		_mainEditView.syncFoldStateWith(session._mainViewFiles[mainIndex2Update]._foldStates);


	showView(SUB_VIEW);
	switchEditViewTo(SUB_VIEW);	//open files in sub
	int subIndex2Update = -1;

	for (size_t k = 0 ; k < session.nbSubFiles() ; )
	{
		const TCHAR *pFn = session._subViewFiles[k]._fileName.c_str();

		if (isFileSession(pFn)) {
			vector<sessionFileInfo>::iterator posIt = session._subViewFiles.begin() + k;
			session._subViewFiles.erase(posIt);
			continue;	//skip session files, not supporting recursive sessions
		}

		bool isWow64Off = false;
		if (!PathFileExists(pFn))
		{
			pNppParam->safeWow64EnableWow64FsRedirection(FALSE);
			isWow64Off = true;
		}
		if (PathFileExists(pFn)) 
		{
			if (isSnapshotMode && session._subViewFiles[k]._backupFilePath != TEXT(""))
				lastOpened = doOpen(pFn, false, false, session._subViewFiles[k]._encoding, session._subViewFiles[k]._backupFilePath.c_str(), session._subViewFiles[k]._originalFileLastModifTimestamp);
			else
				lastOpened = doOpen(pFn, false, false, session._subViewFiles[k]._encoding);

			//check if already open in main. If so, clone
			if (_mainDocTab.getIndexByBuffer(lastOpened) != -1) {
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
			pNppParam->safeWow64EnableWow64FsRedirection(TRUE);
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

			Buffer * buf = MainFileManager->getBufferByID(lastOpened);

			// Set fold states
			if (session._subViewFiles[k]._foldStates.size() > 0)
			{
				if (buf == _subEditView.getCurrentBuffer()) // current document
					// Set floding state in the current doccument
					subIndex2Update = k;
				else
					// Set fold states in the buffer
					buf->setHeaderLineState(session._subViewFiles[k]._foldStates, &_subEditView);
			}

			buf->setPosition(session._subViewFiles[k], &_subEditView);
			if (typeToSet == L_USER) {
				if (!lstrcmp(pLn, TEXT("User Defined"))) {
					pLn = TEXT("");	//default user defined
				}
			}
			buf->setLangType(typeToSet, pLn);
			buf->setEncoding(session._subViewFiles[k]._encoding);

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
		_subEditView.syncFoldStateWith(session._subViewFiles[subIndex2Update]._foldStates);

	_mainEditView.restoreCurrentPos();
	_subEditView.restoreCurrentPos();

	if (session._activeMainIndex < (size_t)_mainDocTab.nbItem())//session.nbMainFiles())
		activateBuffer(_mainDocTab.getBufferByIndex(session._activeMainIndex), MAIN_VIEW);

	if (session._activeSubIndex < (size_t)_subDocTab.nbItem())//session.nbSubFiles())
		activateBuffer(_subDocTab.getBufferByIndex(session._activeSubIndex), SUB_VIEW);

	if ((session.nbSubFiles() > 0) && (session._activeView == MAIN_VIEW || session._activeView == SUB_VIEW))
		switchEditViewTo(session._activeView);
	else
		switchEditViewTo(MAIN_VIEW);

	if (canHideView(otherView()))
		hideView(otherView());
	else if (canHideView(currentView()))
		hideView(currentView());

	if (_pFileSwitcherPanel)
		_pFileSwitcherPanel->reload();

	return allSessionFilesLoaded;
}

bool Notepad_plus::fileLoadSession(const TCHAR *fn)
{
	bool result = false;
	const TCHAR *sessionFileName = NULL;
	if (fn == NULL)
	{
		FileDialog fDlg(_pPublicInterface->getHSelf(), _pPublicInterface->getHinst());
		fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
		const TCHAR *ext = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();
		generic_string sessionExt = TEXT("");
		if (*ext != '\0')
		{
			if (*ext != '.') 
				sessionExt += TEXT(".");
			sessionExt += ext;
			fDlg.setExtFilter(TEXT("Session file"), sessionExt.c_str(), NULL);
		}
		sessionFileName = fDlg.doOpenSingleFileDlg();
	}
	else
	{
		if (PathFileExists(fn))
			sessionFileName = fn;
	}
	

	NppParameters *pNppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = pNppParam->getNppGUI();
	if (sessionFileName)
	{
		bool isEmptyNpp = false;
		if (_mainDocTab.nbItem() == 1 && _subDocTab.nbItem() == 1)
		{
			Buffer * buf1 = MainFileManager->getBufferByID(_mainDocTab.getBufferByIndex(0));
			Buffer * buf2 = MainFileManager->getBufferByID(_subDocTab.getBufferByIndex(0));
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
		}
		else
		{
			bool isAllSuccessful = true;
			Session session2Load;

			if ((NppParameters::getInstance())->loadSession(session2Load, sessionFileName))
			{
				isAllSuccessful = loadSession(session2Load);
				result = true;
			}
			if (!isAllSuccessful)
				(NppParameters::getInstance())->writeSession(session2Load, sessionFileName);
		}
	}
	return result;
}

const TCHAR * Notepad_plus::fileSaveSession(size_t nbFile, TCHAR ** fileNames, const TCHAR *sessionFile2save)
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

		(NppParameters::getInstance())->writeSession(currentSession, sessionFile2save);
		return sessionFile2save;
	}
	return NULL;
}

const TCHAR * Notepad_plus::fileSaveSession(size_t nbFile, TCHAR ** fileNames)
{
	const TCHAR *sessionFileName = NULL;
	
	FileDialog fDlg(_pPublicInterface->getHSelf(), _pPublicInterface->getHinst());
	const TCHAR *ext = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();

	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	generic_string sessionExt = TEXT("");
	if (*ext != '\0')
	{
		if (*ext != '.') 
			sessionExt += TEXT(".");
		sessionExt += ext;
		fDlg.setExtFilter(TEXT("Session file"), sessionExt.c_str(), NULL);
	}
	sessionFileName = fDlg.doSaveDlg();

	return fileSaveSession(nbFile, fileNames, sessionFileName);
}


void Notepad_plus::saveSession(const Session & session)
{
	(NppParameters::getInstance())->writeSession(session);
}


void Notepad_plus::saveCurrentSession()
{
	::PostMessage(_pPublicInterface->getHSelf(), NPPM_INTERNAL_SAVECURRENTSESSION, 0, 0);
}
