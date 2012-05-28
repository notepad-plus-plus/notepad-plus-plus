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


#include "precompiledHeaders.h"
#include "Buffer.h"
#include "Scintilla.h"
#include "Parameters.h"
#include "Notepad_plus.h"
#include "ScintillaEditView.h"
#include "EncodingMapper.h"

FileManager * FileManager::_pSelf = new FileManager();

const int blockSize = 128 * 1024 + 4;

// Ordre important!! Ne le changes pas!
//SC_EOL_CRLF (0), SC_EOL_CR (1), or SC_EOL_LF (2).

const int CR = 0x0D;
const int LF = 0x0A;

Buffer::Buffer(FileManager * pManager, BufferID id, Document doc, DocFileStatus type, const TCHAR *fileName)	//type must be either DOC_REGULAR or DOC_UNNAMED
	: _pManager(pManager), _id(id), _isDirty(false), _doc(doc), _isFileReadOnly(false), _isUserReadOnly(false), _recentTag(-1), _references(0),
	_canNotify(false), _timeStamp(0), _needReloading(false), _encoding(-1)
{
	NppParameters *pNppParamInst = NppParameters::getInstance();
	const NewDocDefaultSettings & ndds = (pNppParamInst->getNppGUI()).getNewDocDefaultSettings();
	_format = ndds._format;
	_unicodeMode = ndds._encoding;
	_encoding = ndds._codepage;
	if (_encoding != -1)
		_unicodeMode = uniCookie;

	_userLangExt = TEXT("");
	_fullPathName = TEXT("");
	_fileName = NULL;
	setFileName(fileName, ndds._lang);
	updateTimeStamp();
	checkFileState();
	_currentStatus = type;
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
	_needLexer = true;	//change of lang means lexern eeds updating
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
		const TCHAR *langName = pNppParamInst->getUserDefinedLangNameFromExt(ext);
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
		else if ((!generic_stricmp(_fileName, TEXT("SConstruct"))) || (!generic_stricmp(_fileName, TEXT("SConscript"))))
			newLang = L_PYTHON;
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
		isWow64Off = false;
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

void Buffer::setHeaderLineState(const std::vector<HeaderLineState> & folds, ScintillaEditView * identifier) {
	int index = indexOfReference(identifier);
	if (index == -1)
		return;
	//deep copy
	std::vector<HeaderLineState> & local = _foldStates[index];
	local.clear();
	size_t size = folds.size();
	for(size_t i = 0; i < size; i++) {
		local.push_back(folds[i]);
	}
}

const std::vector<HeaderLineState> & Buffer::getHeaderLineState(const ScintillaEditView * identifier) const {
	int index = indexOfReference(identifier);
	return _foldStates.at(index);
}

Lang * Buffer::getCurrentLang() const {
	NppParameters *pNppParam = NppParameters::getInstance();
	int i = 0;
	Lang *l = pNppParam->getLangFromIndex(i++);
	while (l)
	{
		if (l->_langID == _lang)
			return l;

		l = pNppParam->getLangFromIndex(i++);
	}
	return NULL;
};

int Buffer::indexOfReference(const ScintillaEditView * identifier) const {
	int size = (int)_referees.size();
	for(int i = 0; i < size; i++) {
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
	_foldStates.push_back(std::vector<HeaderLineState>());
	_references++;
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
	for(int i = 0; i < _references; i++) {
		_referees.at(i)->notifyMarkers(this, isHide, location, false);//(i == _references-1));
	}

	if (!isHide) {	//no deleting if hiding lines
		//Then all docs to remove markers.
		for(int i = 0; i < _references; i++) {
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
	for(size_t i = 0; i < _nrBufs; i++) {
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
};

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

BufferID FileManager::loadFile(const TCHAR * filename, Document doc, int encoding)
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
	Utf8_16_Read UnicodeConvertor;	//declare here so we can get information after loading is done

	formatType format;
	bool res = loadFileData(doc, fullpath, &UnicodeConvertor, L_TEXT, encoding, &format);
	if (res) 
	{
		Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_REGULAR, fullpath);
		BufferID id = (BufferID) newBuf;
		newBuf->_id = id;
		_buffers.push_back(newBuf);
		_nrBufs++;
		Buffer * buf = _buffers.at(_nrBufs - 1);

		// restore the encoding (ANSI based) while opening the existing file
		NppParameters *pNppParamInst = NppParameters::getInstance();
		const NewDocDefaultSettings & ndds = (pNppParamInst->getNppGUI()).getNewDocDefaultSettings();
		buf->setUnicodeMode(ndds._encoding);
		buf->setEncoding(-1);

		if (encoding == -1)
		{
			// 3 formats : WIN_FORMAT, UNIX_FORMAT and MAC_FORMAT
			if (UnicodeConvertor.getNewBuf()) 
			{
				int format = getEOLFormatForm(UnicodeConvertor.getNewBuf());
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
		_nextBufferID++;
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
	formatType format;
	bool res = loadFileData(doc, buf->getFullPathName(), &UnicodeConvertor, buf->getLangType(), encoding, &format);
	buf->_canNotify = true;
	if (res) 
	{
		if (encoding == -1)
		{
			if (UnicodeConvertor.getNewBuf()) 
			{
				int format = getEOLFormatForm(UnicodeConvertor.getNewBuf());
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
	const TCHAR *fileNamePath = buf->getFullPathName();
	if (!PathFileExists(fileNamePath))
		return false;
	return ::DeleteFile(fileNamePath) != 0;
}

bool FileManager::moveFile(BufferID id, const TCHAR * newFileName)
{
	Buffer * buf = getBufferByID(id);
	const TCHAR *fileNamePath = buf->getFullPathName();
	if (!PathFileExists(fileNamePath))
		return false;

	if (::MoveFile(fileNamePath, newFileName) == 0)
		return false;

	buf->setFileName(newFileName);
	return true;
}

bool FileManager::saveBuffer(BufferID id, const TCHAR * filename, bool isCopy) {
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
		if (encoding == -1) //no special encoding; can be handled directly by Utf8_16_Write
		{
			UnicodeConvertor.fwrite(buf, lengthDoc);
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
				UnicodeConvertor.fwrite(newData, newDataLen);
			}
		}
		UnicodeConvertor.fclose();

		if (isHidden)
			::SetFileAttributes(fullpath, attrib | FILE_ATTRIBUTE_HIDDEN);

		if (isSys)
			::SetFileAttributes(fullpath, attrib | FILE_ATTRIBUTE_SYSTEM);

		if (isCopy) {
			_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
			return true;	//all done
		}

		buffer->setFileName(fullpath);
		buffer->setDirty(false);
		buffer->setStatus(DOC_REGULAR);
		buffer->checkFileState();
		_pscratchTilla->execute(SCI_SETSAVEPOINT);
		//_pscratchTilla->markSavedLines();
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);

		return true;
	}
	return false;
}

BufferID FileManager::newEmptyDocument() 
{
	generic_string newTitle = UNTITLED_STR;
	TCHAR nb[10];
	wsprintf(nb, TEXT(" %d"), _nextNewNumber);
	_nextNewNumber++;
	newTitle += nb;

	Document doc = (Document)_pscratchTilla->execute(SCI_CREATEDOCUMENT);	//this already sets a reference for filemanager
	Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle.c_str());
	BufferID id = (BufferID)newBuf;
	newBuf->_id = id;
	_buffers.push_back(newBuf);
	_nrBufs++;
	_nextBufferID++;
	return id;
}

BufferID FileManager::bufferFromDocument(Document doc, bool dontIncrease, bool dontRef)  
{
	generic_string newTitle = UNTITLED_STR;
	TCHAR nb[10];
	wsprintf(nb, TEXT(" %d"), _nextNewNumber);
	newTitle += nb;

	if (!dontRef)
		_pscratchTilla->execute(SCI_ADDREFDOCUMENT, 0, doc);	//set reference for FileManager
	Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle.c_str());
	BufferID id = (BufferID)newBuf;
	newBuf->_id = id;
	_buffers.push_back(newBuf);
	_nrBufs++;

	if (!dontIncrease)
		_nextBufferID++;
	return id;
}

bool FileManager::loadFileData(Document doc, const TCHAR * filename, Utf8_16_Read * UnicodeConvertor, LangType language, int & encoding, formatType *pFormat)
{
	const int blockSize = 128 * 1024;	//128 kB
	char data[blockSize+8];
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
#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif
	if (language < L_EXTERNAL)
	{
		_pscratchTilla->execute(SCI_SETLEXER, ScintillaEditView::langNames[language].lexerID);
	} 
	else
	{
		int id = language - L_EXTERNAL;
		TCHAR * name = NppParameters::getInstance()->getELCFromIndex(id)._name;
#ifdef UNICODE
		const char *pName = wmc->wchar2char(name, CP_ACP);
#else
		const char *pName = name;
#endif
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

            // check if file contain any BOM
            if (isFirstTime) 
            {
                if (Utf8_16_Read::determineEncoding((unsigned char *)data, lenFile) != uni8Bit)
                {
                    // if file contains any BOM, then encoding will be erased,
                    // and the document will be interpreted as UTF 
                    encoding = -1;
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
					format = getEOLFormatForm(data);
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

BufferID FileManager::getBufferFromName(const TCHAR * name) {
	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(name, MAX_PATH, fullpath, NULL);
	::GetLongPathName(fullpath, fullpath, MAX_PATH);
	for(size_t i = 0; i < _buffers.size(); i++) {
		if (!lstrcmpi(name, _buffers.at(i)->getFullPathName()))
			return _buffers.at(i)->getID();
	}
	return BUFFER_INVALID;
}

BufferID FileManager::getBufferFromDocument(Document doc) {
	for(size_t i = 0; i < _nrBufs; i++) {
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

int FileManager::getEOLFormatForm(const char *data) const
{
	size_t len = strlen(data);
	for (size_t i = 0 ; i < len ; i++)
	{
		if (data[i] == CR)
		{
			if (i+1 < len &&  data[i+1] == LF)
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