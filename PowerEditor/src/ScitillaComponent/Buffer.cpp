#include "Buffer.h"

#include <shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Scintilla.h"
#include "Parameters.h"


#include "Notepad_plus.h"
#include "ScintillaEditView.h"

FileManager * FileManager::_pSelf = new FileManager();

const int blockSize = 128 * 1024 + 4;
const TCHAR UNTITLED_STR[] = TEXT("new ");

// Ordre important!! Ne le changes pas!
//SC_EOL_CRLF (0), SC_EOL_CR (1), or SC_EOL_LF (2).

const int CR = 0x0D;
const int LF = 0x0A;

static bool isInList(const TCHAR *token, const TCHAR *list) {
	if ((!token) || (!list))
		return false;
	TCHAR word[64];
	int i = 0;
	int j = 0;
	for (; i <= int(lstrlen(list)) ; i++)
	{
		if ((list[i] == ' ')||(list[i] == '\0'))
		{
			if (j != 0)
			{
				word[j] = '\0';
				j = 0;
				
				if (!generic_stricmp(token, word))
					return true;
			}
		}
		else 
		{
			word[j] = list[i];
			j++;
		}
	}
	return false;
};

void Buffer::determinateFormat(const char *data) {
	_format = WIN_FORMAT;
	size_t len = strlen(data);
	for (size_t i = 0 ; i < len ; i++)
	{
		if (data[i] == CR)
		{
			if (data[i+1] == LF)
			{
				_format = WIN_FORMAT;
				break;
			}
			else
			{
				_format = MAC_FORMAT;
				break;
			}
		}
		if (data[i] == LF)
		{
			_format = UNIX_FORMAT;
			break;
		}
	}
	
	doNotify(BufferChangeFormat);
	return;
};

long Buffer::_recentTagCtr = 0;

void Buffer::updateTimeStamp() {
	struct _stat buf;
	time_t timeStamp = (generic_stat(_fullPathName, &buf)==0)?buf.st_mtime:0;

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
	if (!lstrcmpi(fn, _fullPathName)) {
		updateTimeStamp();
		doNotify(BufferChangeTimestamp);
		return;
	}
	lstrcpy(_fullPathName, fn);
	_fileName = PathFindFileName(_fullPathName);

	// for _lang
	LangType newLang = defaultLang;
	TCHAR *ext = PathFindExtension(_fullPathName);
	if (*ext == '.') {	//extension found
		ext += 1;

		// Define User Lang firstly
		const TCHAR *langName = NULL;
		if ((langName = pNppParamInst->getUserDefinedLangNameFromExt(ext)))
		{
			newLang = L_USER;
			lstrcpy(_userLangExt, langName);
		}
		else // if it's not user lang, then check if it's supported lang
		{
			_userLangExt[0] = '\0';
			newLang = getLangFromExt(ext);
		}	
	}

	if (newLang == defaultLang || newLang == L_TXT)	//language can probably be refined
	{
		if ((!generic_stricmp(_fileName, TEXT("makefile"))) || (!generic_stricmp(_fileName, TEXT("GNUmakefile"))))
			newLang = L_MAKEFILE;
		else if (!generic_stricmp(_fileName, TEXT("CmakeLists.txt")))
			newLang = L_CMAKE;
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

    if (_currentStatus != DOC_DELETED && !PathFileExists(_fullPathName))	//document has been deleted
	{
		_currentStatus = DOC_DELETED;
		_isFileReadOnly = false;
		_isDirty = true;	//dirty sicne no match with filesystem
		_timeStamp = 0;
		doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
		return true;
	} 

	if (_currentStatus == DOC_DELETED && PathFileExists(_fullPathName)) 
	{	//document has returned from its grave

		if (!generic_stat(_fullPathName, &buf))
		{
			_isFileReadOnly = (bool)(!(buf.st_mode & _S_IWRITE));

			_currentStatus = DOC_MODIFIED;
			_timeStamp = buf.st_mtime;
			doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
			return true;
		}

	}

	if (!generic_stat(_fullPathName, &buf))
	{
		int mask = 0;	//status always 'changes', even if from modified to modified
		bool isFileReadOnly = (bool)(!(buf.st_mode & _S_IWRITE));
		if (isFileReadOnly != _isFileReadOnly) {
			_isFileReadOnly = isFileReadOnly;
			mask |= BufferChangeReadonly;
		}

		if (_timeStamp != buf.st_mtime) {
			_timeStamp = buf.st_mtime;
			mask |= BufferChangeTimestamp;
			_currentStatus = DOC_MODIFIED;
			mask |= BufferChangeStatus;	//status always 'changes', even if from modified to modified
		}

		if (mask != 0) {
			doNotify(mask);
			return true;
		}

		return false;
	}
	
	return false;
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

std::vector<HeaderLineState> & Buffer::getHeaderLineState(ScintillaEditView * identifier) {
	int index = indexOfReference(identifier);
	return _foldStates.at(index);
}

LangType Buffer::getLangFromExt(const TCHAR *ext)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	int i = pNppParam->getNbLang();
	i--;
	while (i >= 0)
	{
		Lang *l = pNppParam->getLangFromIndex(i--);

		const TCHAR *defList = l->getDefaultExtList();
		const TCHAR *userList = NULL;

		LexerStylerArray &lsa = pNppParam->getLStylerArray();
		const TCHAR *lName = l->getLangName();
		LexerStyler *pLS = lsa.getLexerStylerByName(lName);
		
		if (pLS)
			userList = pLS->getLexerUserExt();

		std::generic_string list(TEXT(""));
		if (defList)
			list += defList;
		if (userList)
		{
			list += TEXT(" ");
			list += userList;
		}
		if (isInList(ext, list.c_str()))
			return l->getLangID();
	}
	return L_TXT;
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

int Buffer::indexOfReference(ScintillaEditView * identifier) const {
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

//filemanager


void FileManager::init(Notepad_plus * pNotepadPlus, ScintillaEditView * pscratchTilla)
{
	_pNotepadPlus = pNotepadPlus;
	_pscratchTilla = pscratchTilla;
	_pscratchTilla->execute(SCI_SETUNDOCOLLECTION, false);	//dont store any undo information
	_scratchDocDefault = (Document)_pscratchTilla->execute(SCI_GETDOCPOINTER);
	_pscratchTilla->execute(SCI_ADDREFDOCUMENT, 0, _scratchDocDefault);
}

void FileManager::checkFilesystemChanges() {
	for(size_t i = 0; i < _nrBufs; i++) {
		if (_buffers[i]->checkFileState()){}	//something has changed. Triggers update automatically
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

	int oldRefs = buf->_references;
	int refs = buf->removeReference(identifier);

	if (!refs) {	//buffer can be deallocated
		_pscratchTilla->execute(SCI_RELEASEDOCUMENT, 0, buf->_doc);	//release for FileManager, Document is now gone
		_buffers.erase(_buffers.begin() + index);
		delete buf;
		_nrBufs--;
	}
}

BufferID FileManager::loadFile(const TCHAR * filename, Document doc) {
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
	bool res = loadFileData(doc, fullpath, &UnicodeConvertor, L_TXT);
	if (res) 
	{
		Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_REGULAR, fullpath);
		BufferID id = (BufferID) newBuf;
		newBuf->_id = id;
		_buffers.push_back(newBuf);
		_nrBufs++;
		Buffer * buf = _buffers.at(_nrBufs - 1);

		// 3 formats : WIN_FORMAT, UNIX_FORMAT and MAC_FORMAT
		if (UnicodeConvertor.getNewBuf()) 
		{
			buf->determinateFormat(UnicodeConvertor.getNewBuf());
		}
		else
		{
			buf->determinateFormat("");
		}

		UniMode encoding = UnicodeConvertor.getEncoding();
		if (encoding == uni7Bit)
		{
			NppParameters *pNppParamInst = NppParameters::getInstance();
			const NewDocDefaultSettings & ndds = (pNppParamInst->getNppGUI()).getNewDocDefaultSettings();
			if (ndds._openAnsiAsUtf8)
			{
				encoding = uniCookie;
			}
			else
			{
				encoding = uni8Bit;
			}
		}
		buf->setUnicodeMode(encoding);

		//determine buffer properties
		BufferID retval = _nextBufferID++;
		return id;
	} else {	//failed loading, release document
		if (ownDoc)
			_pscratchTilla->execute(SCI_RELEASEDOCUMENT, 0, doc);	//Failure, so release document
		return BUFFER_INVALID;
	}
}

bool FileManager::reloadBuffer(BufferID id) {
	Buffer * buf = getBufferByID(id);
	Document doc = buf->getDocument();
	Utf8_16_Read UnicodeConvertor;
	buf->_canNotify = false;	//disable notify during file load, we dont want dirty to be triggered
	bool res = loadFileData(doc, buf->getFullPathName(), &UnicodeConvertor, buf->getLangType());
	buf->_canNotify = true;
	if (res) {
		if (UnicodeConvertor.getNewBuf()) {
			buf->determinateFormat(UnicodeConvertor.getNewBuf());
		} else {
			buf->determinateFormat("");
		}
		buf->setUnicodeMode(UnicodeConvertor.getEncoding());
		//	buf->setNeedsLexing(true);
	}
	return res;
}

bool FileManager::reloadBufferDeferred(BufferID id) {
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
	DWORD attrib;

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

	FILE *fp = UnicodeConvertor.fopen(fullpath, TEXT("wb"));
	if (fp)
	{
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, buffer->_doc);	//generate new document

		char data[blockSize + 1];
		int lengthDoc = _pscratchTilla->getCurrentDocLen();
		for (int i = 0; i < lengthDoc; i += blockSize)
		{
			int grabSize = lengthDoc - i;
			if (grabSize > blockSize) 
				grabSize = blockSize;
			
			_pscratchTilla->getText(data, i, i + grabSize);
			UnicodeConvertor.fwrite(data, grabSize);
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
		_pscratchTilla->execute(SCI_SETSAVEPOINT);
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);

		return true;
	}
	return false;
}

BufferID FileManager::newEmptyDocument() {
	TCHAR newTitle[10];
	lstrcpy(newTitle, UNTITLED_STR);
	wsprintf(newTitle+4, TEXT("%d"), _nextNewNumber);
	_nextNewNumber++;
	Document doc = (Document)_pscratchTilla->execute(SCI_CREATEDOCUMENT);	//this already sets a reference for filemanager
	Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle);
	BufferID id = (BufferID)newBuf;
	newBuf->_id = id;
	_buffers.push_back(newBuf);
	_nrBufs++;
	BufferID retval = _nextBufferID++;
	return id;
}

BufferID FileManager::bufferFromDocument(Document doc, bool dontIncrease, bool dontRef)  {
	TCHAR newTitle[10];
	lstrcpy(newTitle, UNTITLED_STR);
	wsprintf(newTitle+4, TEXT("%d"), _nextNewNumber);
	if (!dontRef)
		_pscratchTilla->execute(SCI_ADDREFDOCUMENT, 0, doc);	//set reference for FileManager
	Buffer * newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle);
	BufferID id = (BufferID)newBuf;
	newBuf->_id = id;
	_buffers.push_back(newBuf);
	_nrBufs++;
	BufferID retval = _nextBufferID;
	if (!dontIncrease)
		_nextBufferID++;
	return id;
}

bool FileManager::loadFileData(Document doc, const TCHAR * filename, Utf8_16_Read * UnicodeConvertor, LangType language) {
	const int blockSize = 128 * 1024;	//128 kB
	char data[blockSize];
	FILE *fp = generic_fopen(filename, TEXT("rb"));
	if (!fp)
		return false;

	//Setup scratchtilla for new filedata
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, doc);
	bool ro = _pscratchTilla->execute(SCI_GETREADONLY) != 0;
	if (ro) {
		_pscratchTilla->execute(SCI_SETREADONLY, false);
	}
	_pscratchTilla->execute(SCI_CLEARALL);
	if (language < L_EXTERNAL) {
		_pscratchTilla->execute(SCI_SETLEXER, ScintillaEditView::langNames[language].lexerID);
	} else {
		int id = language - L_EXTERNAL;
		TCHAR * name = NppParameters::getInstance()->getELCFromIndex(id)._name;
#ifdef UNICODE
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		const char *pName = wmc->wchar2char(name, CP_ACP);
#else
		const char *pName = name;
#endif
		_pscratchTilla->execute(SCI_SETLEXERLANGUAGE, 0, (LPARAM)pName);
	}

	bool success = true;
	__try {
		size_t lenFile = 0;
		size_t lenConvert = 0;	//just in case conversion results in 0, but file not empty
		do {
			lenFile = fread(data, 1, blockSize, fp);
			lenConvert = UnicodeConvertor->convert(data, lenFile);
			_pscratchTilla->execute(SCI_APPENDTEXT, lenConvert, (LPARAM)(UnicodeConvertor->getNewBuf()));
		} while (lenFile > 0);
	} __except(filter(GetExceptionCode(), GetExceptionInformation())) {
		printStr(TEXT("File is too big to be opened by Notepad++"));
		success = false;
	}
	
	fclose(fp);

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
