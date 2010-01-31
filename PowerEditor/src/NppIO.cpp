//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "precompiledHeaders.h"
#include "Notepad_plus.h"
#include "FileDialog.h"


BufferID Notepad_plus::doOpen(const TCHAR *fileName, bool isReadOnly, int encoding)
{	
	TCHAR longFileName[MAX_PATH];

	::GetFullPathName(fileName, MAX_PATH, longFileName, NULL);
	::GetLongPathName(longFileName, longFileName, MAX_PATH);

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
	if (test != BUFFER_INVALID)
	{
		//switchToFile(test);
		//Dont switch, not responsibility of doOpen, but of caller
		if (_pTrayIco)
		{
			if (_pTrayIco->isInTray())
			{
				::ShowWindow(_hSelf, SW_SHOW);
				if (!_isPrelaunch)
					_pTrayIco->doTrayIcon(REMOVE);
				::SendMessage(_hSelf, WM_SIZE, 0, 0);
			}
		}
		return test;
	}

	if (isFileSession(longFileName) && PathFileExists(longFileName)) 
	{
		fileLoadSession(longFileName);
		return BUFFER_INVALID;
	}


	
	if (!PathFileExists(longFileName))
	{
		TCHAR str2display[MAX_PATH*2];
		generic_string longFileDir(longFileName);
		PathRemoveFileSpec(longFileDir);

		if (PathFileExists(longFileDir.c_str()))
		{
			wsprintf(str2display, TEXT("%s doesn't exist. Create it?"), longFileName);

			if (::MessageBox(_hSelf, str2display, TEXT("Create new file"), MB_YESNO) == IDYES)
			{
				bool res = MainFileManager->createEmptyFile(longFileName);
				if (!res) 
				{
					wsprintf(str2display, TEXT("Cannot create the file \"%s\""), longFileName);
					::MessageBox(_hSelf, str2display, TEXT("Create new file"), MB_OK);
					return BUFFER_INVALID;
				}
			}
			else
			{
				return BUFFER_INVALID;
			}
		}
		else
		{
			return BUFFER_INVALID;
		}
	}

	// Notify plugins that current file is about to load
	// Plugins can should use this notification to filter SCN_MODIFIED
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_FILEBEFORELOAD;
	scnN.nmhdr.hwndFrom = _hSelf;
	scnN.nmhdr.idFrom = NULL;
	_pluginsManager.notify(&scnN);

	if (encoding == -1)
	{
		encoding = getHtmlXmlEncoding(longFileName);
	}
	
	BufferID buffer = MainFileManager->loadFile(longFileName, NULL, encoding);
	if (buffer != BUFFER_INVALID)
	{
		_isFileOpening = true;

		Buffer * buf = MainFileManager->getBufferByID(buffer);
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
				::ShowWindow(_hSelf, SW_SHOW);
				if (!_isPrelaunch)
					_pTrayIco->doTrayIcon(REMOVE);
				::SendMessage(_hSelf, WM_SIZE, 0, 0);
			}
		}
		PathRemoveFileSpec(longFileName);
		_linkTriggered = true;
		_isDocModifing = false;
		
		_isFileOpening = false;

		// Notify plugins that current file is just opened
		scnN.nmhdr.code = NPPN_FILEOPENED;
		_pluginsManager.notify(&scnN);

		return buffer;
	}
	else
	{
		if (::PathIsDirectory(fileName))
		{
			//::MessageBox(_hSelf, fileName, TEXT("Dir"), MB_OK);
			vector<generic_string> fileNames;
			vector<generic_string> patterns;
			patterns.push_back(TEXT("*.*"));

			generic_string fileNameStr = fileName;
			if (fileName[lstrlen(fileName) - 1] != '\\')
				fileNameStr += TEXT("\\");

			getMatchedFileNames(fileNameStr.c_str(), patterns, fileNames, true, false);
			for (size_t i = 0 ; i < fileNames.size() ; i++)
			{
				//::MessageBox(_hSelf, fileNames[i].c_str(), TEXT("Dir"), MB_OK);
				doOpen(fileNames[i].c_str());
			}
		}
		else
		{
			generic_string msg = TEXT("Can not open file \"");
			msg += longFileName;
			msg += TEXT("\".");
			::MessageBox(_hSelf, msg.c_str(), TEXT("ERR"), MB_OK);
			_isFileOpening = false;

			scnN.nmhdr.code = NPPN_FILELOADFAILED;
			_pluginsManager.notify(&scnN);
		}
		return BUFFER_INVALID;
	}
}

bool Notepad_plus::doReload(BufferID id, bool alert)
{

	/*
	//No activation when reloading, defer untill document is actually visible
	if (alert) {
		switchToFile(id);
	}
	*/
	if (alert)
	{
		if (::MessageBox(_hSelf, TEXT("Are you sure you want to reload the current file and lose the changes made in Notepad++?"), TEXT("Reload"), MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL) != IDYES)
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
		scnN.nmhdr.hwndFrom = _hSelf;
		scnN.nmhdr.idFrom = (uptr_t)id;
		_pluginsManager.notify(&scnN);
	}

	bool res = MainFileManager->saveBuffer(id, filename, isCopy);

	if (!isCopy)
	{
		scnN.nmhdr.code = NPPN_FILESAVED;
		_pluginsManager.notify(&scnN);
	}

	if (!res)
		::MessageBox(_hSelf, TEXT("Please check whether if this file is opened in another program"), TEXT("Save failed"), MB_OK);
	return res;
}

void Notepad_plus::doClose(BufferID id, int whichOne) {
	Buffer * buf = MainFileManager->getBufferByID(id);

	// Notify plugins that current file is about to be closed
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_FILEBEFORECLOSE;
	scnN.nmhdr.hwndFrom = _hSelf;
	scnN.nmhdr.idFrom = (uptr_t)id;
	_pluginsManager.notify(&scnN);

	//add to recent files if its an existing file
	if (!buf->isUntitled() && PathFileExists(buf->getFullPathName()))
	{
		_lastRecentFileList.add(buf->getFullPathName());
	}

	int nrDocs = whichOne==MAIN_VIEW?(_mainDocTab.nbItem()):(_subDocTab.nbItem());

	//Do all the works
	removeBufferFromView(id, whichOne);
	if (nrDocs == 1 && canHideView(whichOne))
	{	//close the view if both visible
		hideView(whichOne);
	}

	// Notify plugins that current file is closed
	scnN.nmhdr.code = NPPN_FILECLOSED;
	_pluginsManager.notify(&scnN);

	return;
}

generic_string Notepad_plus::exts2Filters(generic_string exts) const
{
	const TCHAR *extStr = exts.c_str();
	TCHAR aExt[MAX_PATH];
	generic_string filters(TEXT(""));

	int j = 0;
	bool stop = false;
	for (size_t i = 0 ; i < exts.length() ; i++)
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
			j++;
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
		
		for (size_t j = 0 ; j < nppGUI._excludedLangList.size() ; j++)
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
				fDlg.setExtsFilter(getLangDesc(lid, true).c_str(), filters);

                //
                // Get index of lang type to find
                //
                if (langType != -1 && !ltFound)
                {
                    ltFound = langType == lid;
                }

                if (langType != -1 && !ltFound)
                {
                    ltIndex++;
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
	doClose(bufferID, viewToClose);
	return true;
}

bool Notepad_plus::fileCloseAll()
{
	//closes all documents, makes the current view the only one visible

	//first check if we need to save any file
	for(int i = 0; i < _mainDocTab.nbItem(); i++)
	{
		BufferID id = _mainDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (buf->isUntitled() && buf->docLength() == 0)
		{
			// Do nothing
		}
		else if (buf->isDirty()) 
		{
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
	for(int i = 0; i < _subDocTab.nbItem(); i++) 
	{
		BufferID id = _subDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (buf->isUntitled() && buf->docLength() == 0)
		{
			// Do nothing
		}
		else if (buf->isDirty())
		{
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

	//Then start closing, inactive view first so the active is left open
    if (bothActive())
    {	//first close all docs in non-current view, which gets closed automatically
		//Set active tab to the last one closed.
		activateBuffer(_pNonDocTab->getBufferByIndex(0), otherView());
		for(int i = _pNonDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
			doClose(_pNonDocTab->getBufferByIndex(i), otherView());
		}
		//hideView(otherView());
    }

	activateBuffer(_pDocTab->getBufferByIndex(0), currentView());
	for(int i = _pDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
		doClose(_pDocTab->getBufferByIndex(i), currentView());
	}
	return true;
}

bool Notepad_plus::fileCloseAllButCurrent()
{
	BufferID current = _pEditView->getCurrentBufferID();
	int active = _pDocTab->getCurrentTabIndex();
	//closes all documents, makes the current view the only one visible

	//first check if we need to save any file
	for(int i = 0; i < _mainDocTab.nbItem(); i++) {
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
	for(int i = 0; i < _subDocTab.nbItem(); i++) 
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

	//Then start closing, inactive view first so the active is left open
    if (bothActive())
    {	//first close all docs in non-current view, which gets closed automatically
		//Set active tab to the last one closed.
		activateBuffer(_pNonDocTab->getBufferByIndex(0), otherView());
		for(int i = _pNonDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
			doClose(_pNonDocTab->getBufferByIndex(i), otherView());
		}
		//hideView(otherView());
    }

	activateBuffer(_pDocTab->getBufferByIndex(0), currentView());
	for(int i = _pDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
		if (i == active) {	//dont close active index
			continue;
		}
		doClose(_pDocTab->getBufferByIndex(i), currentView());
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
			return fileSaveAs(id);
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
		for(int i = 0; i < _mainDocTab.nbItem(); i++) {
			BufferID idToSave = _mainDocTab.getBufferByIndex(i);
			fileSave(idToSave);
		}
	}

	if (viewVisible(SUB_VIEW)) {
		for(int i = 0; i < _subDocTab.nbItem(); i++) {
			BufferID idToSave = _subDocTab.getBufferByIndex(i);
			fileSave(idToSave);
		}
	}
	return true;
}

bool Notepad_plus::fileSaveAs(BufferID id, bool isSaveCopy)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

	FileDialog fDlg(_hSelf, _hInst);

    fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	int langTypeIndex = setFileOpenSaveDlgFilters(fDlg, buf->getLangType());
	fDlg.setDefFileName(buf->getFileName());
	
    fDlg.setExtIndex(langTypeIndex+1); // +1 for "All types"
	TCHAR *pfn = fDlg.doSaveDlg();

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
			::MessageBox(_hSelf, TEXT("The file is already opened in the Notepad++."), TEXT("ERROR"), MB_OK | MB_ICONSTOP);
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

	FileDialog fDlg(_hSelf, _hInst);

    fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	setFileOpenSaveDlgFilters(fDlg);
			
	fDlg.setDefFileName(buf->getFileName());
	TCHAR *pfn = fDlg.doSaveDlg();

	if (pfn)
	{
		MainFileManager->moveFile(bufferID, pfn);
	}
	return false;
}


bool Notepad_plus::fileDelete(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	
	Buffer * buf = MainFileManager->getBufferByID(bufferID);
	const TCHAR *fileNamePath = buf->getFullPathName();

	if (doDeleteOrNot(fileNamePath) == IDYES)
	{
		if (!MainFileManager->deleteFile(bufferID))
		{
			::MessageBox(_hSelf, TEXT("Delete File failed"), TEXT("Delete File"), MB_OK);
			return false;
		}
		doClose(bufferID, MAIN_VIEW);
		doClose(bufferID, SUB_VIEW);
		return true;
	}
	return false;
}

void Notepad_plus::fileOpen()
{
    FileDialog fDlg(_hSelf, _hInst);
	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	
	setFileOpenSaveDlgFilters(fDlg);

	BufferID lastOpened = BUFFER_INVALID;
	if (stringVector *pfns = fDlg.doOpenMultiFilesDlg())
	{
		size_t sz = pfns->size();
		for (size_t i = 0 ; i < sz ; i++) {
			BufferID test = doOpen(pfns->at(i).c_str(), fDlg.isReadOnly());
			if (test != BUFFER_INVALID)
				lastOpened = test;
		}
	}
	if (lastOpened != BUFFER_INVALID) {
		switchToFile(lastOpened);
	}
}

bool Notepad_plus::fileLoadSession(const TCHAR *fn)
{
	bool result = false;
	const TCHAR *sessionFileName = NULL;
	if (fn == NULL)
	{
		FileDialog fDlg(_hSelf, _hInst);
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
	
	if (sessionFileName)
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
	return result;
}

const TCHAR * Notepad_plus::fileSaveSession(size_t nbFile, TCHAR ** fileNames, const TCHAR *sessionFile2save)
{
	if (sessionFile2save)
	{
		Session currentSession;
		if ((nbFile) && (!fileNames))
		{
			for (size_t i = 0 ; i < nbFile ; i++)
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
	
	FileDialog fDlg(_hSelf, _hInst);
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


