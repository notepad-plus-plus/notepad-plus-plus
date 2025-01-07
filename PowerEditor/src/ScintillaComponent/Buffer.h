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

#pragma once

#include <mutex>
#include "Utf8_16.h"


class Notepad_plus;
class Buffer;
typedef Buffer* BufferID;	//each buffer has unique ID by which it can be retrieved
#define BUFFER_INVALID	reinterpret_cast<BufferID>(0)

typedef sptr_t Document;

enum DocFileStatus {
	DOC_REGULAR      = 0x01, // should not be combined with anything
	DOC_UNNAMED      = 0x02, // not saved (new ##)
	DOC_DELETED      = 0x04, // doesn't exist in environment anymore, but not DOC_UNNAMED
	DOC_MODIFIED     = 0x08, // File in environment has changed
	DOC_NEEDRELOAD   = 0x10, // File is modified & needed to be reload (by log monitoring)
	DOC_INACCESSIBLE = 0x20  // File is absent on its load; this status is temporay for setting file not dirty & readonly; and it will be replaced to DOC_DELETED
};

enum BufferStatusInfo {
	BufferChangeNone		= 0x000,  // Nothing to change
	BufferChangeLanguage	= 0x001,  // Language was altered
	BufferChangeDirty		= 0x002,  // Buffer has changed dirty state
	BufferChangeFormat		= 0x004,  // EOL type was changed
	BufferChangeUnicode		= 0x008,  // Unicode type was changed
	BufferChangeReadonly	= 0x010,  // Readonly state was changed, can be both file and user
	BufferChangeStatus		= 0x020,  // Filesystem Status has changed
	BufferChangeTimestamp	= 0x040,  // Timestamp was changed
	BufferChangeFilename	= 0x080,  // Filename was changed
	BufferChangeRecentTag	= 0x100,  // Recent tag has changed
	BufferChangeLexing		= 0x200,  // Document needs lexing
	BufferChangeMask		= 0x3FF   // Mask: covers all changes
};

enum SavingStatus {
	SaveOK             = 0,
	SaveOpenFailed     = 1,
	SaveWritingFailed  = 2,
	NotEnoughRoom      = 3
};

struct BufferViewInfo {
	BufferID _bufID = 0;
	int _iView = 0;

	BufferViewInfo() = delete;
	BufferViewInfo(BufferID buf, int view) : _bufID(buf), _iView(view) {};
};

const wchar_t UNTITLED_STR[] = L"new ";

//File manager class maintains all buffers
class FileManager final {
public:
	void init(Notepad_plus* pNotepadPlus, ScintillaEditView* pscratchTilla);

	void checkFilesystemChanges(bool bCheckOnlyCurrentBuffer);

	size_t getNbBuffers() const { return _nbBufs; };
	size_t getNbDirtyBuffers() const;
	int getBufferIndexByID(BufferID id);
	Buffer * getBufferByIndex(size_t index);
	Buffer * getBufferByID(BufferID id) {return id;}

	void beNotifiedOfBufferChange(Buffer * theBuf, int mask);

	void closeBuffer(BufferID, const ScintillaEditView* identifer);		//called by Notepad++

	void addBufferReference(BufferID id, ScintillaEditView * identifer);	//called by Scintilla etc indirectly

	BufferID loadFile(const wchar_t * filename, Document doc = static_cast<Document>(NULL), int encoding = -1, const wchar_t *backupFileName = nullptr, FILETIME fileNameTimestamp = {});	//ID == BUFFER_INVALID on failure. If Doc == NULL, a new file is created, otherwise data is loaded in given document
	BufferID newEmptyDocument();
	// create an empty placeholder for a missing file when loading session
	BufferID newPlaceholderDocument(const wchar_t * missingFilename, int whichOne, const wchar_t* userCreatedSessionName);

	//create Buffer from existing Scintilla, used from new Scintillas.
	BufferID bufferFromDocument(Document doc, bool isMainEditZone);

	BufferID getBufferFromName(const wchar_t * name);
	BufferID getBufferFromDocument(Document doc);

	void setLoadedBufferEncodingAndEol(Buffer* buf, const Utf8_16_Read& UnicodeConvertor, int encoding, EolType bkformat);
	bool reloadBuffer(BufferID id);
	bool reloadBufferDeferred(BufferID id);
	SavingStatus saveBuffer(BufferID id, const wchar_t* filename, bool isCopy = false);
	bool backupCurrentBuffer();
	bool deleteBufferBackup(BufferID id);
	bool deleteFile(BufferID id);
	bool moveFile(BufferID id, const wchar_t * newFilename);
	bool createEmptyFile(const wchar_t * path);
	static FileManager& getInstance() {
		static FileManager instance;
		return instance;
	};
	int getFileNameFromBuffer(BufferID id, wchar_t * fn2copy);
	size_t docLength(Buffer * buffer) const;
	void removeHotSpot(Buffer * buffer) const;
	size_t nextUntitledNewNumber() const;

private:
	struct LoadedFileFormat {
		LoadedFileFormat() = default;
		LangType _language = L_TEXT;
		int _encoding = 0;
		EolType _eolFormat = EolType::osdefault;
	};

	FileManager() = default;
	~FileManager();

	// No copy ctor and assignment
	FileManager(const FileManager&) = delete;
	FileManager& operator=(const FileManager&) = delete;

	// No move ctor and assignment
	FileManager(FileManager&&) = delete;
	FileManager& operator=(FileManager&&) = delete;

	int detectCodepage(char* buf, size_t len);
	bool loadFileData(Document doc, int64_t fileSize, const wchar_t* filename, char* buffer, Utf8_16_Read* UnicodeConvertor, LoadedFileFormat& fileFormat);
	LangType detectLanguageFromTextBegining(const unsigned char *data, size_t dataLen);

	Notepad_plus* _pNotepadPlus = nullptr;
	ScintillaEditView* _pscratchTilla = nullptr;
	Document _scratchDocDefault = 0;
	std::vector<Buffer*> _buffers;
	BufferID _nextBufferID = 0;
	size_t _nbBufs = 0;
};

#define MainFileManager FileManager::getInstance()

class Buffer final {
	friend class FileManager;
public:
	//Loading a document:
	//constructor with ID.
	//Set a reference (pointer to a container mostly, like DocTabView or ScintillaEditView)
	//Set the position manually if needed
	//Load the document into Scintilla/add to TabBar
	//The entire lifetime if the buffer, the Document has reference count of _atleast_ one
	//Destructor makes sure its purged
	Buffer(FileManager * pManager, BufferID id, Document doc, DocFileStatus type, const wchar_t *fileName, bool isLargeFile);

	// this method 1. copies the file name
	//             2. determinates the language from the ext of file name
	//             3. gets the last modified time
	void setFileName(const wchar_t *fn);

	const wchar_t * getFullPathName() const { return _fullPathName.c_str(); }

	const wchar_t * getFileName() const { return _fileName; }

	BufferID getID() const { return _id; }

	void increaseRecentTag() {
		_recentTag = ++_recentTagCtr;
		doNotify(BufferChangeRecentTag);
	}

	long getRecentTag() const { return _recentTag; }

	bool checkFileState();

	bool isDirty() const { return _isDirty; }

	bool isReadOnly() const { return (_isUserReadOnly || _isFileReadOnly); }

	bool isUntitled() const { return ((_currentStatus & DOC_UNNAMED) == DOC_UNNAMED); }

	bool isFromNetwork() const { return _isFromNetwork; }

	bool isInaccessible() const { return _isInaccessible; }
	void setInaccessibility(bool val) { _isInaccessible = val; }

	bool getFileReadOnly() const { return _isFileReadOnly; }

	void setFileReadOnly(bool ro) {
		_isFileReadOnly = ro;
		doNotify(BufferChangeReadonly);
	}

	bool getUserReadOnly() const { return _isUserReadOnly; }

	void setUserReadOnly(bool ro) {
		_isUserReadOnly = ro;
		doNotify(BufferChangeReadonly);
	}

	EolType getEolFormat() const { return _eolFormat; }

	void setEolFormat(EolType format) {
		_eolFormat = format;
		doNotify(BufferChangeFormat);
	}

	LangType getLangType() const { return _lang; }

	void setLangType(LangType lang, const wchar_t * userLangName = L"");

	int getLastLangType() const { return _lastLangType; }

	void setLastLangType(int val) { _lastLangType = val; }

	UniMode getUnicodeMode() const { return _unicodeMode; }

	void setUnicodeMode(UniMode mode);

	int getEncoding() const { return _encoding; }

	void setEncoding(int encoding);

	DocFileStatus getStatus() const { return _currentStatus; }

	Document getDocument() { return _doc; }

	void setDirty(bool dirty);

	void setPosition(const Position & pos, const ScintillaEditView * identifier);
	Position & getPosition(const ScintillaEditView * identifier);

	void setHeaderLineState(const std::vector<size_t> & folds, ScintillaEditView * identifier);
	const std::vector<size_t> & getHeaderLineState(const ScintillaEditView * identifier) const;

	bool isUserDefineLangExt() const { return (_userLangExt[0] != '\0'); }

	const wchar_t * getUserDefineLangName() const	{ return _userLangExt.c_str(); }

	const wchar_t * getCommentLineSymbol() const {
		const Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentLineSymbol;
	}

	const wchar_t * getCommentStart() const {
		const Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentStart;
	}

	const wchar_t * getCommentEnd() const {
		const Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentEnd;
	}

	bool getNeedsLexing() const { return _needLexer; }

	void setNeedsLexing(bool lex) {
		_needLexer = lex;
		doNotify(BufferChangeLexing);
	}

	//these two return reference count after operation
	int addReference(ScintillaEditView * identifier);		//if ID not registered, creates a new Position for that ID and new foldstate
	int removeReference(const ScintillaEditView * identifier);		//reduces reference. If zero, Document is purged

	void setHideLineChanged(bool isHide, size_t location);

	void setDeferredReload();

	bool getNeedReload() const { return _needReloading; }
	void setNeedReload(bool reload) { _needReloading = reload; }

	std::wstring tabCreatedTimeString() const { return _tabCreatedTimeString; }
	void setTabCreatedTimeStringFromBakFile() {
		if (!_isFromNetwork && _currentStatus == DOC_UNNAMED)
			_tabCreatedTimeString = getFileTime(Buffer::ft_created); // while DOC_UNNAMED, getFileTime will retrieve time from backup file
	}
	void setTabCreatedTimeStringWithCurrentTime() {
		if (_currentStatus == DOC_UNNAMED)
		{
			FILETIME now{};
			GetSystemTimeAsFileTime(&now);
			_tabCreatedTimeString = getTimeString(now);
		}
	}

	size_t docLength() const {
		assert(_pManager != nullptr);
		return _pManager->docLength(_id);
	}

	int64_t getFileLength() const; // return file length. -1 if file is not existing.

	enum fileTimeType { ft_created, ft_modified, ft_accessed };
	std::wstring getFileTime(fileTimeType ftt) const;
	std::wstring getTimeString(FILETIME rawtime) const;

	Lang * getCurrentLang() const;

	bool isModified() const { return _isModified; }
	void setModifiedStatus(bool isModified) { _isModified = isModified; }

	std::wstring getBackupFileName() const { return _backupFileName; }
	void setBackupFileName(const std::wstring& fileName) { _backupFileName = fileName; }

	FILETIME getLastModifiedTimestamp() const { return _timeStamp; }

	bool isLoadedDirty() const { return _isLoadedDirty; }
	void setLoadedDirty(bool val) {	_isLoadedDirty = val; }

	bool isUnsync() const { return _isUnsync; }
	void setUnsync(bool val) { _isUnsync = val; }

	bool isSavePointDirty() const { return _isSavePointDirty; }
	void setSavePointDirty(bool val) { _isSavePointDirty = val; }

	bool isLargeFile() const { return _isLargeFile; }

	void startMonitoring() {
		_isMonitoringOn = true;
		_eventHandle = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	};

	HANDLE getMonitoringEvent() const { return _eventHandle; };

	void stopMonitoring() {
		_isMonitoringOn = false;
		::SetEvent(_eventHandle);
		::CloseHandle(_eventHandle);
	};

	bool isMonitoringOn() const { return _isMonitoringOn; };
	void updateTimeStamp();
	void reload();
	void setMapPosition(const MapPosition & mapPosition) { _mapPosition = mapPosition; };
	MapPosition getMapPosition() const { return _mapPosition; };

	void langHasBeenSetFromMenu() { _hasLangBeenSetFromMenu = true; };

	bool allowBraceMach() const;
	bool allowAutoCompletion() const;
	bool allowSmartHilite() const;
	bool allowClickableLink() const;

	void setDocColorId(int idx) {
		_docColorId = idx;
	};

	int getDocColorId() {
		return _docColorId;
	};

	bool isRTL() const { return _isRTL; };
	void setRTL(bool isRTL) { _isRTL = isRTL; };

	bool isPinned() const { return _isPinned; };
	void setPinned(bool isPinned) { _isPinned = isPinned; };

private:
	int indexOfReference(const ScintillaEditView * identifier) const;

	void setStatus(DocFileStatus status) {
		_currentStatus = status;
		doNotify(BufferChangeStatus);
	}

	void doNotify(int mask);

	Buffer(const Buffer&) = delete;
	Buffer& operator = (const Buffer&) = delete;


private:
	FileManager * _pManager = nullptr;
	bool _canNotify = false; // All the notification should be disabled at the beginning 
	int _references = 0; // if no references file inaccessible, can be closed
	BufferID _id = nullptr;

	//document properties
	Document _doc;	//invariable
	LangType _lang = L_TEXT;
	int _lastLangType = -1;
	std::wstring _userLangExt; // it's useful if only (_lang == L_USER)
	bool _isDirty = false;
	EolType _eolFormat = EolType::osdefault;
	UniMode _unicodeMode = uniUTF8;
	int _encoding = -1;
	bool _isUserReadOnly = false;
	bool _isFromNetwork = false;
	bool _needLexer = false; // new buffers do not need lexing, Scintilla takes care of that
	//these properties have to be duplicated because of multiple references

	//All the vectors must have the same size at all times
	std::vector<ScintillaEditView *> _referees; // Instances of ScintillaEditView which contain this buffer
	std::vector<Position> _positions;
	std::vector<std::vector<size_t>> _foldStates;

	//Environment properties
	DocFileStatus _currentStatus = DOC_REGULAR;
	FILETIME _timeStamp = {}; // 0 if it's a new doc

	bool _isFileReadOnly = false;
	std::wstring _fullPathName;
	wchar_t * _fileName = nullptr; // points to filename part in _fullPathName
	bool _needReloading = false; // True if Buffer needs to be reloaded on activation

	std::wstring _tabCreatedTimeString;

	long _recentTag = -1;
	static long _recentTagCtr;

	int _docColorId = -1;

	// For backup system
	std::wstring _backupFileName;
	bool _isModified = false;
	bool _isLoadedDirty = false; // it's the indicator for finding buffer's initial state

	bool _isUnsync = false; // Buffer should be always dirty (with any undo/redo operation) if the editing buffer is unsyncronized with file on disk.
	                        // By "unsyncronized" it means :
	                        // 1. the file is deleted outside but the buffer in Notepad++ is kept.
	                        // 2. the file is modified by another app but the buffer is not reloaded in Notepad++.
	                        // Note that if the buffer is untitled, there's no correspondent file on the disk so the buffer is considered as independent therefore synchronized.

	bool _isLargeFile = false; // The loading of huge files will disable automatically 1. auto-completion 2. snapshot periode backup 3. backup on save 4. word wrap

	bool _isSavePointDirty = false; // After converting document to another ecoding, the document becomes dirty, and all the undo states are emptied.
	                                // This variable member keeps this situation in memory and when the undo state back to the save_point_reached, it'll still be dirty (its original state) 

	// For the monitoring
	HANDLE _eventHandle = nullptr;
	bool _isMonitoringOn = false;

	bool _hasLangBeenSetFromMenu = false;

	MapPosition _mapPosition;

	std::mutex _reloadFromDiskRequestGuard;

	bool _isInaccessible = false;

	bool _isRTL = false;
	bool _isPinned = false;
};
