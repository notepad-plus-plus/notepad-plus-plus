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

#include <deque>
#include <time.h>
#include <sys/stat.h>
#include "Buffer.h"
#include "Scintilla.h"
#include "Parameters.h"
#include "Notepad_plus.h"
#include "ScintillaEditView.h"
#include "EncodingMapper.h"
#include "uchardet.h"
#include "LongRunningOperation.h"

FileManager * FileManager::_pSelf = new FileManager();

static const int blockSize = 128 * 1024 + 4;
static const int CR = 0x0D;
static const int LF = 0x0A;


Buffer::Buffer(FileManager * pManager, BufferID id, Document doc, DocFileStatus type, const TCHAR *fileName)	//type must be either DOC_REGULAR or DOC_UNNAMED
	: _pManager(pManager), _id(id), _isDirty(false), _doc(doc), _isFileReadOnly(false), _isUserReadOnly(false), _recentTag(-1), _references(0),
	_canNotify(false), _timeStamp(0), _needReloading(false), _encoding(-1), _backupFileName(TEXT("")), _isModified(false), _isLoadedDirty(false), _lang(L_TEXT)
{
	NppParameters *pNppParamInst = NppParameters::getInstance();
	const NewDocDefaultSettings & ndds = (pNppParamInst->getNppGUI()).getNewDocDefaultSettings();
	_format = ndds._format;
	_unicodeMode = ndds._unicodeMode;
	_encoding = ndds._codepage;
	if (_encoding != -1)
		_unicodeMode = uniCookie;

	_userLangExt = TEXT("");
	_fullPathName = TEXT("");
	_fileName = NULL;
	_currentStatus = type;

	setFileName(fileName, ndds._lang);
	updateTimeStamp();
	checkFileState();
	_isDirty = false;

	_needLexer = false;	//new buffers do not need lexing, Scintilla takes care of that
	_canNotify = true;
}


void Buffer::setLangType(LangType lang, const TCHAR * userLangName)
{
	if (lang == _lang && lang != L_USER)
		return;
	_lang = lang;
	if (_lang == L_USER) 
	{
		_userLangExt = userLangName;
	}
	_needLexer = true;	//change of lang means lexern needs updating
	doNotify(BufferChangeLanguage|BufferChangeLexing);
}

long Buffer::_recentTagCtr = 0;

void Buffer::updateTimeStamp() {
	struct _stat buf;
	time_t timeStamp = (generic_stat(_fullPathName.c_str(), &buf)==0)?buf.st_mtime:0;

	if (timeStamp != _timeStamp) {
		_timeStamp = timeStamp;
		doNotify(BufferChangeTimestamp);
	}
};

// Set full path file name in buffer object,
// and determinate its language by its extension.
// If the ext is not in the list, the defaultLang passed as argument will be set.
void Buffer::setFileName(const TCHAR *fn, LangType defaultLang) 
{
	NppParameters *pNppParamInst = NppParameters::getInstance();
	if (_fullPathName == fn) 
	{
		updateTimeStamp();
		doNotify(BufferChangeTimestamp);
		return;
	}
	_fullPathName = fn;
	_fileName = PathFindFileName(_fullPathName.c_str());

	// for _lang
	LangType newLang = defaultLang;
	TCHAR *ext = PathFindExtension(_fullPathName.c_str());
	if (*ext == '.') {	//extension found
		ext += 1;

		// Define User Lang firstly
		const TCHAR *langName = pNppParamInst->getUserDefinedLangNameFromExt(ext, _fileName);
		if (langName)
		{
			newLang = L_USER;
			_userLangExt = langName;
		}
		else // if it's not user lang, then check if it's supported lang
		{
			_userLangExt = TEXT("");
			newLang = pNppParamInst->getLangFromExt(ext);
		}	
	}

	if (newLang == defaultLang || newLang == L_TEXT)	//language can probably be refined
	{
		if ((!generic_stricmp(_fileName, TEXT("makefile"))) || (!generic_stricmp(_fileName, TEXT("GNUmakefile"))))
			newLang = L_MAKEFILE;
		else if (!generic_stricmp(_fileName, TEXT("CmakeLists.txt")))
			newLang = L_CMAKE;
		else if ((!generic_stricmp(_fileName, TEXT("SConstruct"))) || (!generic_stricmp(_fileName, TEXT("SConscript"))) || (!generic_stricmp(_fileName, TEXT("wscript"))))
			newLang = L_PYTHON;
		else if (!generic_stricmp(_fileName, TEXT("Rakefile")))
			newLang = L_RUBY;
	}

	updateTimeStamp();
	if (newLang != _lang || _lang == L_USER) {
		_lang = newLang;
		doNotify(BufferChangeFilename | BufferChangeLanguage | BufferChangeTimestamp);
		return;
	}

	doNotify(BufferChangeFilename | BufferChangeTimestamp);
}

bool Buffer::checkFileState() {	//returns true if the status has been changed (it can change into DOC_REGULAR too). false otherwise
	struct _stat buf;

 	if (_currentStatus == DOC_UNNAMED)	//unsaved document cannot change by environment
		return false;

	bool isWow64Off = false;
	NppParameters *pNppParam = NppParameters::getInstance();
	if (!PathFileExists(_fullPathName.c_str()))
	{
		pNppParam->safeWow64EnableWow64FsRedirection(FALSE);
		isWow64Off = true;
	}

	bool isOK = false;
	if (_currentStatus != DOC_DELETED && !PathFileExists(_fullPathName.c_str()))	//document has been deleted
	{
		_currentStatus = DOC_DELETED;
		_isFileReadOnly = false;
		_isDirty = true;	//dirty sicne no match with filesystem
		_timeStamp = 0;
		doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
		isOK = true;
	} 
	else if (_currentStatus == DOC_DELETED && PathFileExists(_fullPathName.c_str())) 
	{	//document has returned from its grave
		if (!generic_stat(_fullPathName.c_str(), &buf))
		{
			_isFileReadOnly = (bool)(!(buf.st_mode & _S_IWRITE));

			_currentStatus = DOC_MODIFIED;
			_timeStamp = buf.st_mtime;
			doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
			isOK = true;
		}
	}
	else if (!generic_stat(_fullPathName.c_str(), &buf))
	{
		int mask = 0;	//status always 'changes', even if from modified to modified
		bool isFileReadOnly = (bool)(!(buf.st_mode & _S_IWRITE));
		if (isFileReadOnly != _isFileReadOnly)
		{
			_isFileReadOnly = isFileReadOnly;
			mask |= BufferChangeReadonly;
		}
		if (_timeStamp != buf.st_mtime)
		{
			_timeStamp = buf.st_mtime;
			mask |= BufferChangeTimestamp;
			_currentStatus = DOC_MODIFIED;
			mask |= BufferChangeStatus;	//status always 'changes', even if from modified to modified
		}

		if (mask != 0) 
		{
			doNotify(mask);
			isOK = true;
		}
		isOK = false;
	}
	
	if (isWow64Off)
	{
		pNppParam->safeWow64EnableWow64FsRedirection(TRUE);
		//isWow64Off = false;
	}
	return isOK;
}

int Buffer::getFileLength()
{
	if (_currentStatus == DOC_UNNAMED)
		return -1;

	struct _stat buf;

	if (PathFileExists(_fullPathName.c_str())) 
	{
		if (!generic_stat(_fullPathName.c_str(), &buf))
		{
			return buf.st_size;
		}
	}
	return -1;
}

generic_string Buffer::getFileTime(fileTimeType ftt)
{
	if (_currentStatus == DOC_UNNAMED)
		return TEXT("");

	struct _stat buf;

	if (PathFileExists(_fullPathName.c_str())) 
	{
		if (!generic_stat(_fullPathName.c_str(), &buf))
		{
			time_t rawtime = ftt==ft_created?buf.st_ctime:ftt==ft_modified?buf.st_mtime:buf.st_atime;
			tm *timeinfo = localtime(&rawtime);
			const int temBufLen = 64;
			TCHAR tmpbuf[temBufLen];

			generic_strftime(tmpbuf, temBufLen, TEXT("%Y-%m-%d %H:%M:%S"), timeinfo);
			return tmpbuf;
		}
	}
	return TEXT("");
}


void Buffer::setPosition(const Position & pos, ScintillaEditView * identifier) {
	int index = indexOfReference(identifier);
	if (index == -1)
		return;
	_positions[index] = pos;
}

Position & Buffer::getPosition(ScintillaEditView * identifier) {
	int index = indexOfReference(identifier);
	return _positions.at(index);
}

void Buffer::setHeaderLineState(const std::vector<size_t> & folds, ScintillaEditView * identifier) {
	int index = indexOfReference(identifier);
	if (index == -1)
		return;
	//deep copy
	std::vector<size_t> & local = _foldStates[index];
	local.clear();
	size_t size = folds.size();
	for(size_t i = 0; i < size; ++i) {
		local.push_back(folds[i]);
	}
}

const std::vector<size_t> & Buffer::getHeaderLineState(const ScintillaEditView * identifier) const {
	int index = indexOfReference(identifier);
	return _foldStates.at(index);
}

Lang * Buffer::getCurrentLang() const {
	NppParameters *pNppParam = NppParameters::getInstance();
	int i = 0;
	Lang *l = pNppParam->getLangFromIndex(i);
	++i;
	while (l)
	{
		if (l->_langID == _lang)
			return l;

		l = pNppParam->getLangFromIndex(i);
		++i;
	}
	return NULL;
};

int Buffer::indexOfReference(const ScintillaEditView * identifier) const {
	int size = (int)_referees.size();
	for(int i = 0; i < size; ++i) {
		if (_referees[i] == identifier)
			return i;
	}
	return -1;	//not found
}

int Buffer::addReference(ScintillaEditView * identifier) {
	if (indexOfReference(identifier) != -1)
		return _references;
	_referees.push_back(identifier);
	_positions.push_back(Position());
	_foldStates.push_back(std::vector<size_t>());
	++_references;
	return _references;
}

int Buffer::removeReference(ScintillaEditView * identifier) {
	int indexToPop = indexOfReference(identifier);
	if (indexToPop == -1)
		return _references;
	_referees.erase(_referees.begin() + indexToPop);
	_positions.erase(_positions.begin() + indexToPop);
	_foldStates.erase(_foldStates.begin() + indexToPop);
	_references--;
	return _references;
}

void Buffer::setHideLineChanged(bool isHide, int location) {
	//First run through all docs without removing markers
	for(int i = 0; i < _references; ++i) {
		_referees.at(i)->notifyMarkers(this, isHide, location, false);//(i == _references-1));
	}

	if (!isHide) {	//no deleting if hiding lines
		//Then all docs to remove markers.
		for(int i = 0; i < _references; ++i) {
			_referees.at(i)->notifyMarkers(this, isHide, location, true);
		}
	}
}
void Buffer::setDeferredReload() {	//triggers a reload on the next Document access
	_isDirty = false;	//when reloading, just set to false, since it sohuld be marked as clean
	_needReloading = true;
	doNotify(BufferChangeDirty);
}

/*
pair<size_t, bool> Buffer::getLineUndoState(size_t currentLine) const
{
	for (size_t i = 0 ; i < _linesUndoState.size() ; i++)
	{
		if (_linesUndoState[i].first == currentLine)
			return _linesUndoState[i].second;
	}
	return pair<size_t, bool>(0, false);
}

void Buffer::setLineUndoState(size_t currentLine, size_t undoLevel, bool isSaved)
{
	bool found = false;
	for (size_t i = 0 ; i < _linesUndoState.size() ; i++)
	{
		if (_linesUndoState[i].first == currentLine)
		{
			_linesUndoState[i].second.first = undoLevel;
			_linesUndoState[i].second.second = isSaved;
		}
	}
	if (!found)
	{
		_linesUndoState.push_back(pair<size_t, pair<size_t, bool> >(currentLine, pair<size_t, bool>(undoLevel, false)));
	}
}
*/

//filemanager

FileManager::~FileManager()
{
	for (std::vector<Buffer *>::iterator it = _buffers.begin(), end = _buffers.end(); it != end; ++it)
	{
		delete *it;
	}
}

void FileManager::init(Notepad_plus * pNotepadPlus, ScintillaEditView * pscratchTilla)
{
	_pNotepadPlus = pNotepadPlus;
	_pscratchTilla = pscratchTilla;
	_pscratchTilla->execute(SCI_SETUNDOCOLLECTION, false);	//dont store any undo information
	_scratchDocDefault = (Document)_pscratchTilla->execute(SCI_GETDOCPOINTER);
	_pscratchTilla->execute(SCI_ADDREFDOCUMENT, 0, _scratchDocDefault);
}

void FileManager::checkFilesystemChanges() {
	for(int i = int(_nrBufs -1) ; i >= 0 ; i--)
    {
        if (i >= int(_nrBufs))
        {
            if (_nrBufs == 0)
                return;

            i = _nrBufs - 1;
        }
        _buffers[i]->checkFileState();	//something has changed. Triggers update automatically
	}
    
}

int FileManager::getBufferIndexByID(BufferID id) {
	for(size_t i = 0; i < _nrBufs; ++i) {
		if (_buffers[i]->_id == id)
			return (int)i;
	}
	return -1;
}

Buffer * FileManager::getBufferByIndex(int index) {
	return _buffers.at(index);
}

void FileManager::beNotifiedOfBufferChange(Buffer * theBuf, int mask) {
	_pNotepadPlus->notifyBufferChanged(theBuf, mask);
}

void FileManager::addBufferReference(BufferID buffer, ScintillaEditView * identifier) {
	Buffer * buf = getBufferByID(buffer);
	buf->addReference(identifier);
}

void FileManager::closeBuffer(BufferID id, ScintillaEditView * identifier) {
	int index = getBufferIndexByID(id);
	Buffer * buf = getBufferByIndex(index);

	int refs = buf->removeReference(identifier);

	if (!refs) {	//buffer can be deallocated
		_pscratchTilla->execute(SCI_RELEASEDOCUMENT, 0, buf->_doc);	//release for FileManager, Document is now gone
		_buffers.erase(_buffers.begin() + index);
		delete buf;
		_nrBufs--;
	}
}

// backupFileName is sentinel of backup mode: if it's not NULL, then we use it (load it). Otherwise we use filename
BufferID FileManager::loadFile(const TCHAR * filename, Document doc, int encoding, const TCHAR *backupFileName, time_t fileNameTimestamp)
{
	bool ownDoc = false;
	if (doc == NULL) 
	{
		doc = (Document)_pscratchTilla->execute(SCI_CREATEDOCUMENT);
		ownDoc = true;
	}

	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(filename, MAX_PATH, fullpath, NULL);
	::GetLongPathName(fullpath, fullpath, MAX_PATH);

	bool isSnapshotMode = backupFileName != NULL && PathFileExists(backupFileName);
	if (isSnapshotMode && !PathFileExists(fullpath)) // if backup mode and fullpath doesn't exist, we guess is UNTITLED
	{
		lstrcpy(fullpath, filename); // we restore fullpath with filename, in our case is "new  #"
	}

	Utf8_16_Read UnicodeConvertor;	//declare here so we can get information after loading is done

	char data[blockSize + 8]; // +8 for incomplete multibyte char
	formatType format;
	bool res = loadFileData(doc, backupFileName?backupFileName:fullpath, data, &UnicodeConvertor, L_TEXT, encoding, &format);
	if (res) 
	{
		Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_REGULAR, fullpath);
		BufferID id = (BufferID) newBuf;
		newBuf->_id = id;
		if (backupFileName != NULL)
		{
			newBuf->_backupFileName = backupFileName;
			if (!PathFileExists(fullpath))
				newBuf->_currentStatus = DOC_UNNAMED;
		}
		if (fileNameTimestamp != 0)
			newBuf->_timeStamp = fileNameTimestamp;
		_buffers.push_back(newBuf);
		++_nrBufs;
		Buffer * buf = _buffers.at(_nrBufs - 1);

		// restore the encoding (ANSI based) while opening the existing file
		NppParameters *pNppParamInst = NppParameters::getInstance();
		const NewDocDefaultSettings & ndds = (pNppParamInst->getNppGUI()).getNewDocDefaultSettings();
		buf->setUnicodeMode(ndds._unicodeMode);
		buf->setEncoding(-1);

		if (encoding == -1)
		{
			// 3 formats : WIN_FORMAT, UNIX_FORMAT and MAC_FORMAT
			if (nullptr != UnicodeConvertor.getNewBuf()) 
			{
				int format = getEOLFormatForm(UnicodeConvertor.getNewBuf(), UnicodeConvertor.getNewSize());
				buf->setFormat(format == -1?WIN_FORMAT:(formatType)format);
			}
			else
			{
				buf->setFormat(WIN_FORMAT);
			}

			UniMode um = UnicodeConvertor.getEncoding();
			if (um == uni7Bit)
			{
				if (ndds._openAnsiAsUtf8)
				{
					um = uniCookie;
				}
				else
				{
					um = uni8Bit;
				}
			}
			buf->setUnicodeMode(um);
		}
		else // encoding != -1
		{
            // Test if encoding is set to UTF8 w/o BOM (usually for utf8 indicator of xml or html)
            buf->setEncoding((encoding == SC_CP_UTF8)?-1:encoding);
            buf->setUnicodeMode(uniCookie);
			buf->setFormat(format);
		}

		//determine buffer properties
		++_nextBufferID;
		return id;
	}
	else //failed loading, release document
	{	
		if (ownDoc)
			_pscratchTilla->execute(SCI_RELEASEDOCUMENT, 0, doc);	//Failure, so release document
		return BUFFER_INVALID;
	}
}

bool FileManager::reloadBuffer(BufferID id)
{
	Buffer * buf = getBufferByID(id);
	Document doc = buf->getDocument();
	Utf8_16_Read UnicodeConvertor;
	buf->_canNotify = false;	//disable notify during file load, we dont want dirty to be triggered
	int encoding = buf->getEncoding();
	char data[blockSize + 8]; // +8 for incomplete multibyte char
	formatType format;
	bool res = loadFileData(doc, buf->getFullPathName(), data, &UnicodeConvertor, buf->getLangType(), encoding, &format);
	buf->_canNotify = true;
	if (res) 
	{
		if (encoding == -1)
		{
			if (nullptr != UnicodeConvertor.getNewBuf()) 
			{
				int format = getEOLFormatForm(UnicodeConvertor.getNewBuf(), UnicodeConvertor.getNewSize());
				buf->setFormat(format == -1?WIN_FORMAT:(formatType)format);
			}
			else
			{
				buf->setFormat(WIN_FORMAT);
			}
			buf->setUnicodeMode(UnicodeConvertor.getEncoding());
		}
		else
		{
			buf->setEncoding(encoding);
			buf->setFormat(format);
			buf->setUnicodeMode(uniCookie);
		}
	}
	return res;
}

bool FileManager::reloadBufferDeferred(BufferID id)
{
	Buffer * buf = getBufferByID(id);
	buf->setDeferredReload();
	return true;
}

bool FileManager::deleteFile(BufferID id)
{
	Buffer * buf = getBufferByID(id);
	generic_string fileNamePath = buf->getFullPathName();

	// Make sure to form a string with double '\0' terminator.
	fileNamePath.append(1, '\0');

	if (!PathFileExists(fileNamePath.c_str()))
		return false;
	//return ::DeleteFile(fileNamePath) != 0;

	SHFILEOPSTRUCT fileOpStruct = {0};
	fileOpStruct.hwnd = NULL;
	fileOpStruct.pFrom = fileNamePath.c_str();
	fileOpStruct.pTo = NULL;
	fileOpStruct.wFunc = FO_DELETE;
	fileOpStruct.fFlags = FOF_ALLOWUNDO;
	fileOpStruct.fAnyOperationsAborted = false;
	fileOpStruct.hNameMappings         = NULL;
	fileOpStruct.lpszProgressTitle     = NULL;

	return SHFileOperation(&fileOpStruct) == 0;
}

bool FileManager::moveFile(BufferID id, const TCHAR * newFileName)
{
	Buffer * buf = getBufferByID(id);
	const TCHAR *fileNamePath = buf->getFullPathName();
	if (::MoveFileEx(fileNamePath, newFileName, MOVEFILE_REPLACE_EXISTING) == 0)
		return false;

	buf->setFileName(newFileName);
	return true;
}


/*
Specs and Algorithm of session snapshot & periodic backup system:
Notepad++ quits without asking for saving unsaved file.
It restores all the unsaved files and document as the states they left. 

For existing file (c:\tmp\foo.h)
	- Open  
	In the next session, Notepad++
	1. load backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) if exist, otherwise load FILENAME (c:\tmp\foo.h).
	2. if backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) is loaded, set it dirty (red).
	3. if backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) is loaded, last modif timestamp of FILENAME (c:\tmp\foo.h), compare with tracked timestamp (in session.xml).
	4. in the case of unequal result, tell user the FILENAME (c:\tmp\foo.h) was modified. ask user if he want to reload FILENAME(c:\tmp\foo.h)

	- Editing
	when a file starts being modified, a file will be created with name: FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776)
	the Buffer object will associate with this FILENAME@CREATION_TIMESTAMP file (backup\foo.h@198776).
	1. sync: (each 3-5 second) backup file will be saved, if buffer is dirty, and modification is present (a bool on modified notificatin).
	2. sync: each save file, or close file, the backup file will be deleted (if buffer is not dirty).
	3. before switch off to another tab (or close files on exit), check 1 & 2 (sync with backup).

	- Close
	In the current session, Notepad++ 
	1. track FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) if exist (in session.xml).
	2. track last modified timestamp of FILENAME (c:\tmp\foo.h) if FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) was tracked  (in session.xml).

For untitled document (new  4)
	- Open
	In the next session, Notepad++
	1. open file UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776)
	2. set label as UNTITLED_NAME (new  4) and disk icon as red.
 
	- Editing
	when a untitled document starts being modified, a backup file will be created with name: UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776)
	the Buffer object will associate with this UNTITLED_NAME@CREATION_TIMESTAMP file (backup\new  4@198776).
	1. sync: (each 3-5 second) backup file will be saved, if buffer is dirty, and modification is present (a bool on modified notificatin).
	2. sync: if untitled document is saved, or closed, the backup file will be deleted.
	3. before switch off to another tab (or close documents on exit), check 1 & 2 (sync with backup).

	- CLOSE
	In the current session, Notepad++ 
	1. track UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776) in session.xml.
*/
bool FileManager::backupCurrentBuffer()
{
	LongRunningOperation op;

	Buffer * buffer = _pNotepadPlus->getCurrentBuffer();
	bool result = false;
	bool hasModifForSession = false;

	if (buffer->isDirty())
	{
		if (buffer->isModified()) // buffer dirty and modified, write the backup file
		{
			// Synchronization
			// This method is called from 2 differents place, so synchronization is important
			HANDLE writeEvent = ::OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("nppWrittingEvent"));
			if (!writeEvent)
			{
				// no thread yet, create a event with non-signaled, to block all threads
				writeEvent = ::CreateEvent(NULL, TRUE, FALSE, TEXT("nppWrittingEvent"));
			}
			else 
			{
				if (::WaitForSingleObject(writeEvent, INFINITE) != WAIT_OBJECT_0)
				{
					// problem!!!
					printStr(TEXT("WaitForSingleObject problem in backupCurrentBuffer()!"));
					return false;
				}

				// unlocled here, set to non-signaled state, to block all threads
				::ResetEvent(writeEvent);
			}

			UniMode mode = buffer->getUnicodeMode();
			if (mode == uniCookie)
				mode = uni8Bit;	//set the mode to ANSI to prevent converter from adding BOM and performing conversions, Scintilla's data can be copied directly

			Utf8_16_Write UnicodeConvertor;
			UnicodeConvertor.setEncoding(mode);
			int encoding = buffer->getEncoding();

			generic_string backupFilePath = buffer->getBackupFileName();
			if (backupFilePath == TEXT(""))
			{
				// Create file
				backupFilePath = NppParameters::getInstance()->getUserPath();
				backupFilePath += TEXT("\\backup\\");

				// if "backup" folder doesn't exist, create it.
				if (!PathFileExists(backupFilePath.c_str()))
				{
					::CreateDirectory(backupFilePath.c_str(), NULL);
				}

				backupFilePath += buffer->getFileName();

				const int temBufLen = 32;
				TCHAR tmpbuf[temBufLen];
				time_t ltime = time(0);
				struct tm *today;

				today = localtime(&ltime);
				generic_strftime(tmpbuf, temBufLen, TEXT("%Y-%m-%d_%H%M%S"), today);

				backupFilePath += TEXT("@");
				backupFilePath += tmpbuf;

				// Set created file name in buffer
				buffer->setBackupFileName(backupFilePath);

				// Session changes, save it
				hasModifForSession = true;
			}

			TCHAR fullpath[MAX_PATH];
			::GetFullPathName(backupFilePath.c_str(), MAX_PATH, fullpath, NULL);
			::GetLongPathName(fullpath, fullpath, MAX_PATH);
			
			// Make sure the backup file is not read only
			DWORD dwFileAttribs = ::GetFileAttributes(fullpath);
			if (dwFileAttribs & FILE_ATTRIBUTE_READONLY) // if file is read only, remove read only attribute
			{
				dwFileAttribs ^= FILE_ATTRIBUTE_READONLY; 
				::SetFileAttributes(fullpath, dwFileAttribs);
			}

			FILE *fp = UnicodeConvertor.fopen(fullpath, TEXT("wb"));
			if (fp)
			{
				int lengthDoc = _pNotepadPlus->_pEditView->getCurrentDocLen();
				char* buf = (char*)_pNotepadPlus->_pEditView->execute(SCI_GETCHARACTERPOINTER);	//to get characters directly from Scintilla buffer
				size_t items_written = 0;
				if (encoding == -1) //no special encoding; can be handled directly by Utf8_16_Write
				{
					items_written = UnicodeConvertor.fwrite(buf, lengthDoc);
					if (lengthDoc == 0)
						items_written = 1;
				}
				else
				{
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					int grabSize;
					for (int i = 0; i < lengthDoc; i += grabSize)
					{
						grabSize = lengthDoc - i;
						if (grabSize > blockSize) 
							grabSize = blockSize;
						
						int newDataLen = 0;
						int incompleteMultibyteChar = 0;
						const char *newData = wmc->encode(SC_CP_UTF8, encoding, buf+i, grabSize, &newDataLen, &incompleteMultibyteChar);
						grabSize -= incompleteMultibyteChar;
						items_written = UnicodeConvertor.fwrite(newData, newDataLen);
					}
					if (lengthDoc == 0)
						items_written = 1;
				}
				UnicodeConvertor.fclose();

				// Note that fwrite() doesn't return the number of bytes written, but rather the number of ITEMS.
				if(items_written == 1) // backup file has been saved
				{
					buffer->setModifiedStatus(false);
					result = true;	//all done
				}
			}
			// set to signaled state
			if (::SetEvent(writeEvent) == NULL)
			{
				printStr(TEXT("oups!"));
			}
			// printStr(TEXT("Event released!"));
			::CloseHandle(writeEvent);
		}
		else // buffer dirty but unmodified
		{
			result = true; 
		}
	}
	else // buffer not dirty, sync: delete the backup file
	{
		generic_string backupFilePath = buffer->getBackupFileName();
		if (backupFilePath != TEXT(""))
		{
			// delete backup file
			generic_string file2Delete = buffer->getBackupFileName();
			buffer->setBackupFileName(TEXT(""));
			result = (::DeleteFile(file2Delete.c_str()) != 0);

			// Session changes, save it
			hasModifForSession = true;
		}
		//printStr(TEXT("backup deleted in backupCurrentBuffer"));
		result = true; // no backup file to delete
	}
	//printStr(TEXT("backup sync"));

	if (result && hasModifForSession)
	{
		//printStr(buffer->getBackupFileName().c_str());
		_pNotepadPlus->saveCurrentSession();
	}
	return result;
}

class EventReset
{
public:
	EventReset(HANDLE h)
	{
		_h = h;
	}

	~EventReset()
	{
		::SetEvent(_h);
		::CloseHandle(_h);
	}

private:
	HANDLE _h;
};

bool FileManager::deleteCurrentBufferBackup()
{
	HANDLE writeEvent = ::OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("nppWrittingEvent"));
	if (!writeEvent)
	{
		// no thread yet, create a event with non-signaled, to block all threads
		writeEvent = ::CreateEvent(NULL, TRUE, FALSE, TEXT("nppWrittingEvent"));
	}
	else
	{
		if (::WaitForSingleObject(writeEvent, INFINITE) != WAIT_OBJECT_0)
		{
			// problem!!!
			printStr(TEXT("WaitForSingleObject problem in deleteCurrentBufferBackup()!"));
			return false;
		}

		// unlocled here, set to non-signaled state, to block all threads
		::ResetEvent(writeEvent);
	}

	EventReset reset(writeEvent); // Will reset event in destructor.

	Buffer * buffer = _pNotepadPlus->getCurrentBuffer();
	bool result = true;
	generic_string backupFilePath = buffer->getBackupFileName();
	if (backupFilePath != TEXT(""))
	{
		// delete backup file
		generic_string file2Delete = buffer->getBackupFileName();
		buffer->setBackupFileName(TEXT(""));
		result = (::DeleteFile(file2Delete.c_str()) != 0);
	}
	
	// set to signaled state via destructor EventReset.
	return result;
}

bool FileManager::saveBuffer(BufferID id, const TCHAR * filename, bool isCopy, generic_string * error_msg)
{
	HANDLE writeEvent = ::OpenEvent(EVENT_ALL_ACCESS, TRUE, TEXT("nppWrittingEvent"));
	if (!writeEvent)
	{
		// no thread yet, create a event with non-signaled, to block all threads
		writeEvent = ::CreateEvent(NULL, TRUE, FALSE, TEXT("nppWrittingEvent"));
	}
	else 
	{		//printStr(TEXT("Locked. I wait."));
		if (::WaitForSingleObject(writeEvent, INFINITE) != WAIT_OBJECT_0)
		{
			// problem!!!
			printStr(TEXT("WaitForSingleObject problem in saveBuffer()!"));
			return false;
		}

		// unlocled here, set to non-signaled state, to block all threads
		::ResetEvent(writeEvent);
	}

	EventReset reset(writeEvent); // Will reset event in destructor.
	Buffer * buffer = getBufferByID(id);
	bool isHidden = false;
	bool isSys = false;
	DWORD attrib = 0;

	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(filename, MAX_PATH, fullpath, NULL);
	::GetLongPathName(fullpath, fullpath, MAX_PATH);
	if (PathFileExists(fullpath))
	{
		attrib = ::GetFileAttributes(fullpath);

		if (attrib != INVALID_FILE_ATTRIBUTES)
		{
			isHidden = (attrib & FILE_ATTRIBUTE_HIDDEN) != 0;
			if (isHidden)
				::SetFileAttributes(filename, attrib & ~FILE_ATTRIBUTE_HIDDEN);

			isSys = (attrib & FILE_ATTRIBUTE_SYSTEM) != 0;
			if (isSys)
				::SetFileAttributes(filename, attrib & ~FILE_ATTRIBUTE_SYSTEM);
		}
	}

	UniMode mode = buffer->getUnicodeMode();
	if (mode == uniCookie)
		mode = uni8Bit;	//set the mode to ANSI to prevent converter from adding BOM and performing conversions, Scintilla's data can be copied directly

	Utf8_16_Write UnicodeConvertor;
	UnicodeConvertor.setEncoding(mode);

	int encoding = buffer->getEncoding();

	FILE *fp = UnicodeConvertor.fopen(fullpath, TEXT("wb"));
	if (fp)
	{
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, buffer->_doc);	//generate new document

		int lengthDoc = _pscratchTilla->getCurrentDocLen();
		char* buf = (char*)_pscratchTilla->execute(SCI_GETCHARACTERPOINTER);	//to get characters directly from Scintilla buffer
		size_t items_written = 0;
		if (encoding == -1) //no special encoding; can be handled directly by Utf8_16_Write
		{
			items_written = UnicodeConvertor.fwrite(buf, lengthDoc);
			if (lengthDoc == 0)
				items_written = 1;
		}
		else
		{
			WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
			int grabSize;
			for (int i = 0; i < lengthDoc; i += grabSize)
			{
				grabSize = lengthDoc - i;
				if (grabSize > blockSize) 
					grabSize = blockSize;
				
				int newDataLen = 0;
				int incompleteMultibyteChar = 0;
				const char *newData = wmc->encode(SC_CP_UTF8, encoding, buf+i, grabSize, &newDataLen, &incompleteMultibyteChar);
				grabSize -= incompleteMultibyteChar;
				items_written = UnicodeConvertor.fwrite(newData, newDataLen);
			}
			if (lengthDoc == 0)
				items_written = 1;
		}
		UnicodeConvertor.fclose();

		// Error, we didn't write the entire document to disk.
		// Note that fwrite() doesn't return the number of bytes written, but rather the number of ITEMS.
		if(items_written != 1)
		{
			if(error_msg != NULL)
				*error_msg = TEXT("Failed to save file.\nNot enough space on disk to save file?");
		
			// set to signaled state via destructor EventReset.
			return false;
		}

		if (isHidden)
			::SetFileAttributes(fullpath, attrib | FILE_ATTRIBUTE_HIDDEN);

		if (isSys)
			::SetFileAttributes(fullpath, attrib | FILE_ATTRIBUTE_SYSTEM);

		if (isCopy)
		{
			_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);

			/* for saveAs it's not necessary since this action is for the "current" directory, so we let manage in SAVEPOINTREACHED event 
			generic_string backupFilePath = buffer->getBackupFileName();
			if (backupFilePath != TEXT(""))
			{
				// delete backup file
				generic_string file2Delete = buffer->getBackupFileName();
				buffer->setBackupFileName(TEXT(""));
				::DeleteFile(file2Delete.c_str());
			}
			*/

			// set to signaled state via destructor EventReset.
			return true;	//all done
		}

		buffer->setFileName(fullpath);
		buffer->setDirty(false);
		buffer->setStatus(DOC_REGULAR);
		buffer->checkFileState();
		_pscratchTilla->execute(SCI_SETSAVEPOINT);
		//_pscratchTilla->markSavedLines();
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);

		generic_string backupFilePath = buffer->getBackupFileName();
		if (backupFilePath != TEXT(""))
		{
			// delete backup file
			generic_string file2Delete = buffer->getBackupFileName();
			buffer->setBackupFileName(TEXT(""));
			::DeleteFile(file2Delete.c_str());
		}

		// set to signaled state via destructor EventReset.
		return true;
	}
	// set to signaled state via destructor EventReset.
	return false;
}

size_t FileManager::nextUntitledNewNumber() const
{
	std::vector<size_t> usedNumbers; 
	for(size_t i = 0; i < _buffers.size(); i++)
	{
		Buffer *buf = _buffers.at(i);
		if (buf->isUntitled())
		{
			TCHAR *numberStr = buf->_fileName + lstrlen(UNTITLED_STR);
			int usedNumber = generic_atoi(numberStr);
			usedNumbers.push_back(usedNumber);
		}
	}

	size_t newNumber = 1;
	bool numberAvailable = true;
	bool found = false;
	do
	{
		for(size_t j = 0; j < usedNumbers.size(); j++)
		{
			numberAvailable = true;
			found = false;
			if (usedNumbers[j] == newNumber)
			{
				numberAvailable = false;
				found = true;
				break;
			}
		}
		if (!numberAvailable)
			newNumber++;
		
		if (!found)
			break;

	} while (!numberAvailable);

	return newNumber;
}

BufferID FileManager::newEmptyDocument() 
{
	generic_string newTitle = UNTITLED_STR;
	TCHAR nb[10];
	wsprintf(nb, TEXT("%d"), nextUntitledNewNumber());
	newTitle += nb;

	Document doc = (Document)_pscratchTilla->execute(SCI_CREATEDOCUMENT);	//this already sets a reference for filemanager
	Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle.c_str());
	BufferID id = (BufferID)newBuf;
	newBuf->_id = id;
	_buffers.push_back(newBuf);
	++_nrBufs;
	++_nextBufferID;
	return id;
}

BufferID FileManager::bufferFromDocument(Document doc, bool dontIncrease, bool dontRef)  
{
	generic_string newTitle = UNTITLED_STR;
	TCHAR nb[10];
	wsprintf(nb, TEXT(" %d"), 0);
	newTitle += nb;

	if (!dontRef)
		_pscratchTilla->execute(SCI_ADDREFDOCUMENT, 0, doc);	//set reference for FileManager
	Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle.c_str());
	BufferID id = (BufferID)newBuf;
	newBuf->_id = id;
	_buffers.push_back(newBuf);
	++_nrBufs;

	if (!dontIncrease)
		++_nextBufferID;
	return id;
}

int FileManager::detectCodepage(char* buf, size_t len)
{
	uchardet_t ud = uchardet_new();
	uchardet_handle_data(ud, buf, len);
	uchardet_data_end(ud);
	const char* cs = uchardet_get_charset(ud);
	int codepage = EncodingMapper::getInstance()->getEncodingFromString(cs);
	uchardet_delete(ud);
	return codepage;
}

inline bool FileManager::loadFileData(Document doc, const TCHAR * filename, char* data, Utf8_16_Read * UnicodeConvertor,
	LangType language, int & encoding, formatType *pFormat)
{
	FILE *fp = generic_fopen(filename, TEXT("rb"));
	if (!fp)
		return false;

	//Get file size
	_fseeki64 (fp , 0 , SEEK_END);
	unsigned __int64 fileSize =_ftelli64(fp);
	rewind(fp);
	// size/6 is the normal room Scintilla keeps for editing, but here we limit it to 1MiB when loading (maybe we want to load big files without editing them too much)
	unsigned __int64 bufferSizeRequested = fileSize + min(1<<20,fileSize/6);
	// As a 32bit application, we cannot allocate 2 buffer of more than INT_MAX size (it takes the whole address space)
	if(bufferSizeRequested > INT_MAX)
	{
		::MessageBox(NULL, TEXT("File is too big to be opened by Notepad++"), TEXT("File open problem"), MB_OK|MB_APPLMODAL);
		/*
		_nativeLangSpeaker.messageBox("NbFileToOpenImportantWarning",
										_pPublicInterface->getHSelf(),
										TEXT("File is too big to be opened by Notepad++"),
										TEXT("File open problem"),
										MB_OK|MB_APPLMODAL);
		*/
		fclose(fp);
		return false;
	}

	//Setup scratchtilla for new filedata
	_pscratchTilla->execute(SCI_SETSTATUS, SC_STATUS_OK); // reset error status
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, doc);
	bool ro = _pscratchTilla->execute(SCI_GETREADONLY) != 0;
	if (ro)
	{
		_pscratchTilla->execute(SCI_SETREADONLY, false);
	}
	_pscratchTilla->execute(SCI_CLEARALL);

	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

	if (language < L_EXTERNAL)
	{
		_pscratchTilla->execute(SCI_SETLEXER, ScintillaEditView::langNames[language].lexerID);
	} 
	else
	{
		int id = language - L_EXTERNAL;
		TCHAR * name = NppParameters::getInstance()->getELCFromIndex(id)._name;
		const char *pName = wmc->wchar2char(name, CP_ACP);
		_pscratchTilla->execute(SCI_SETLEXERLANGUAGE, 0, (LPARAM)pName);
	}

	if (encoding != -1)
	{
		_pscratchTilla->execute(SCI_SETCODEPAGE, SC_CP_UTF8);
	}

	bool success = true;
	int format = -1;
	__try {
		// First allocate enough memory for the whole file (this will reduce memory copy during loading)
		_pscratchTilla->execute(SCI_ALLOCATE, WPARAM(bufferSizeRequested));
		if(_pscratchTilla->execute(SCI_GETSTATUS) != SC_STATUS_OK) throw;

		size_t lenFile = 0;
		size_t lenConvert = 0;	//just in case conversion results in 0, but file not empty
		bool isFirstTime = true;
		int incompleteMultibyteChar = 0;

		do {
			lenFile = fread(data+incompleteMultibyteChar, 1, blockSize-incompleteMultibyteChar, fp) + incompleteMultibyteChar;
			if (lenFile == 0) break;

            // check if file contain any BOM
            if (isFirstTime) 
            {
                if (Utf8_16_Read::determineEncoding((unsigned char *)data, lenFile) != uni8Bit)
                {
                    // if file contains any BOM, then encoding will be erased,
                    // and the document will be interpreted as UTF 
                    encoding = -1;
				}
				else if (encoding == -1)
				{
					if (NppParameters::getInstance()->getNppGUI()._detectEncoding)
						encoding = detectCodepage(data, lenFile);
                }
                isFirstTime = false;
            }

			if (encoding != -1)
			{
				if (encoding == SC_CP_UTF8)
				{
					// Pass through UTF-8 (this does not check validity of characters, thus inserting a multi-byte character in two halfs is working)
					_pscratchTilla->execute(SCI_APPENDTEXT, lenFile, (LPARAM)data);
				}
				else
				{
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					int newDataLen = 0;
					const char *newData = wmc->encode(encoding, SC_CP_UTF8, data, lenFile, &newDataLen, &incompleteMultibyteChar);
					_pscratchTilla->execute(SCI_APPENDTEXT, newDataLen, (LPARAM)newData);
				}

				if (format == -1)
					format = getEOLFormatForm(data, lenFile);
			}
			else
			{
				lenConvert = UnicodeConvertor->convert(data, lenFile);
				_pscratchTilla->execute(SCI_APPENDTEXT, lenConvert, (LPARAM)(UnicodeConvertor->getNewBuf()));
			}
			if(_pscratchTilla->execute(SCI_GETSTATUS) != SC_STATUS_OK) throw;

			if(incompleteMultibyteChar != 0)
			{
				// copy bytes to next buffer
				memcpy(data, data+blockSize-incompleteMultibyteChar, incompleteMultibyteChar);
			}
			
		} while (lenFile > 0);
	} __except(EXCEPTION_EXECUTE_HANDLER) {  //TODO: should filter correctly for other exceptions; the old filter(GetExceptionCode(), GetExceptionInformation()) was only catching access violations
		::MessageBox(NULL, TEXT("File is too big to be opened by Notepad++"), TEXT("File open problem"), MB_OK|MB_APPLMODAL);
		success = false;
	}
	
	fclose(fp);

	if (pFormat != NULL)
	{
		*pFormat = (format == -1)?WIN_FORMAT:(formatType)format;
	}
	_pscratchTilla->execute(SCI_EMPTYUNDOBUFFER);
	_pscratchTilla->execute(SCI_SETSAVEPOINT);
	if (ro) {
		_pscratchTilla->execute(SCI_SETREADONLY, true);
	}
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
	return success;
}

BufferID FileManager::getBufferFromName(const TCHAR * name)
{
	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(name, MAX_PATH, fullpath, NULL);
	::GetLongPathName(fullpath, fullpath, MAX_PATH);
	for(size_t i = 0; i < _buffers.size(); i++)
	{
		if (!lstrcmpi(name, _buffers.at(i)->getFullPathName()))
			return _buffers.at(i)->getID();
	}
	return BUFFER_INVALID;
}

BufferID FileManager::getBufferFromDocument(Document doc) {
	for(size_t i = 0; i < _nrBufs; ++i) {
		if (_buffers[i]->_doc == doc)
			return _buffers[i]->_id;
	}
	return BUFFER_INVALID;
}

bool FileManager::createEmptyFile(const TCHAR * path) {
	FILE * file = generic_fopen(path, TEXT("wb"));
	if (!file)
		return false;
	fclose(file);
	return true;
}

int FileManager::getFileNameFromBuffer(BufferID id, TCHAR * fn2copy) {
	if (getBufferIndexByID(id) == -1)
		return -1;
	Buffer * buf = getBufferByID(id);
	if (fn2copy)
		lstrcpy(fn2copy, buf->getFullPathName());
	return lstrlen(buf->getFullPathName());
}

int FileManager::docLength(Buffer * buffer) const 
{
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, buffer->_doc);
	int docLen = _pscratchTilla->getCurrentDocLen();
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
	return docLen;
}

int FileManager::getEOLFormatForm(const char* const data, size_t length) const
{
	assert(data != nullptr && "invalid buffer for getEOLFormatForm()");

	for (size_t i = 0; i != length; ++i)
	{
		if (data[i] == CR)
		{
			if (i+1 < length && data[i+1] == LF)
			{
				return int(WIN_FORMAT);
			}
			else
			{
				return int(MAC_FORMAT);
			}
		}
		if (data[i] == LF)
		{
			return int(UNIX_FORMAT);
		}
	}
	return -1;
}