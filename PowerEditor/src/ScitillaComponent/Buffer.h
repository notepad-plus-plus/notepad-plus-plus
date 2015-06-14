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


#ifndef BUFFER_H
#define BUFFER_H

#include "Utf8_16.h"

class Buffer;
typedef Buffer * BufferID;	//each buffer has unique ID by which it can be retrieved
#define BUFFER_INVALID	(BufferID)0

typedef sptr_t Document;

enum DocFileStatus{
	DOC_REGULAR = 0x01,	//should not be combined with anything
	DOC_UNNAMED = 0x02,	//not saved (new ##)
	DOC_DELETED = 0x04, //doesn't exist in environment anymore, but not DOC_UNNAMED
	DOC_MODIFIED = 0x08	//File in environment has changed
};

enum BufferStatusInfo {
	BufferChangeLanguage	= 0x001,	//Language was altered
	BufferChangeDirty		= 0x002,	//Buffer has changed dirty state
	BufferChangeFormat		= 0x004,	//EOL type was changed
	BufferChangeUnicode		= 0x008,	//Unicode type was changed
	BufferChangeReadonly	= 0x010,	//Readonly state was changed, can be both file and user
	BufferChangeStatus		= 0x020,	//Filesystem Status has changed
	BufferChangeTimestamp	= 0x040,	//Timestamp was changed
	BufferChangeFilename	= 0x080,	//Filename was changed
	BufferChangeRecentTag	= 0x100,	//Recent tag has changed
	BufferChangeLexing		= 0x200,	//Document needs lexing
	BufferChangeMask		= 0x3FF		//Mask: covers all changes
};

//const int userLangNameMax = 16;
const TCHAR UNTITLED_STR[] = TEXT("new ");

//File manager class maintains all buffers
class FileManager {
public:
	void init(Notepad_plus * pNotepadPlus, ScintillaEditView * pscratchTilla);

	//void activateBuffer(int index);	
	void checkFilesystemChanges();

	int getNrBuffers() { return _nrBufs; };
	int getBufferIndexByID(BufferID id);
	Buffer * getBufferByIndex(int index);	//generates exception if index is invalid
	Buffer * getBufferByID(BufferID id) {return (Buffer*)id;}

	void beNotifiedOfBufferChange(Buffer * theBuf, int mask);

	void closeBuffer(BufferID, ScintillaEditView * identifer);		//called by Notepad++

	void addBufferReference(BufferID id, ScintillaEditView * identifer);	//called by Scintilla etc indirectly

	BufferID loadFile(const TCHAR * filename, Document doc = NULL, int encoding = -1, const TCHAR *backupFileName = NULL, time_t fileNameTimestamp = 0);	//ID == BUFFER_INVALID on failure. If Doc == NULL, a new file is created, otherwise data is loaded in given document
	BufferID newEmptyDocument();
	//create Buffer from existing Scintilla, used from new Scintillas. If dontIncrease = true, then the new document number isnt increased afterwards.
	//usefull for temporary but neccesary docs
	//If dontRef = false, then no extra reference is added for the doc. Its the responsibility of the caller to do so
	BufferID bufferFromDocument(Document doc,  bool dontIncrease = false, bool dontRef = false);

	BufferID getBufferFromName(const TCHAR * name);
	BufferID getBufferFromDocument(Document doc);

	bool reloadBuffer(BufferID id);
	bool reloadBufferDeferred(BufferID id);
	bool saveBuffer(BufferID id, const TCHAR * filename, bool isCopy = false, generic_string * error_msg = NULL);
	bool backupCurrentBuffer();
	bool deleteCurrentBufferBackup();
	bool deleteFile(BufferID id);
	bool moveFile(BufferID id, const TCHAR * newFilename);
	bool createEmptyFile(const TCHAR * path);
	static FileManager * getInstance() {return _pSelf;};
	void destroyInstance() { delete _pSelf; };
	int getFileNameFromBuffer(BufferID id, TCHAR * fn2copy);
	int docLength(Buffer * buffer) const;
	int getEOLFormatForm(const char* const data, size_t length) const;
	size_t nextUntitledNewNumber() const;

private:
	FileManager() : _nextBufferID(0), _pNotepadPlus(NULL), _nrBufs(0), _pscratchTilla(NULL){};
	~FileManager();
	static FileManager *_pSelf;

	Notepad_plus * _pNotepadPlus;
	ScintillaEditView * _pscratchTilla;
	Document _scratchDocDefault;
	std::vector<Buffer *> _buffers;
	BufferID _nextBufferID;
	size_t _nrBufs;
	int detectCodepage(char* buf, size_t len);

	bool loadFileData(Document doc, const TCHAR * filename, char* buffer, Utf8_16_Read * UnicodeConvertor, LangType language, int & encoding, formatType *pFormat = NULL);
};

#define MainFileManager FileManager::getInstance()

class Buffer
{
friend class FileManager;
public :
	//Loading a document: 
	//constructor with ID.
	//Set a reference (pointer to a container mostly, like DocTabView or ScintillaEditView)
	//Set the position manually if needed
	//Load the document into Scintilla/add to TabBar
	//The entire lifetime if the buffer, the Document has reference count of _atleast_ one
	//Destructor makes sure its purged
	Buffer(FileManager * pManager, BufferID id, Document doc, DocFileStatus type, const TCHAR *fileName);

	// this method 1. copies the file name
	//             2. determinates the language from the ext of file name
	//             3. gets the last modified time
	void setFileName(const TCHAR *fn, LangType defaultLang = L_TEXT);

	const TCHAR * getFullPathName() const {
		return _fullPathName.c_str();
	};

	const TCHAR * getFileName() const { return _fileName; };

	BufferID getID() const {
		return _id;
	};

	void increaseRecentTag() {
		_recentTag = ++_recentTagCtr;
		doNotify(BufferChangeRecentTag);
	};

	long getRecentTag() const {
		return _recentTag;
	};

	bool checkFileState();

    bool isDirty() const {
        return _isDirty;
    };

    bool isReadOnly() const {
        return (_isUserReadOnly || _isFileReadOnly);
    };

	bool isUntitled() const {
		return (_currentStatus == DOC_UNNAMED);
	};

	bool getFileReadOnly() const {
        return _isFileReadOnly;
    };

	void setFileReadOnly(bool ro) {
		_isFileReadOnly = ro;
		doNotify(BufferChangeReadonly);
	};

	bool getUserReadOnly() const {
        return _isUserReadOnly;
    };

	void setUserReadOnly(bool ro) {
		_isUserReadOnly = ro;
		doNotify(BufferChangeReadonly);
    };

	formatType getFormat() const {
		return _format;
	};

	void setFormat(formatType format) {
		_format = format;
		doNotify(BufferChangeFormat);
	};

	LangType getLangType() const {
		return _lang;
	};

	void setLangType(LangType lang, const TCHAR * userLangName = TEXT(""));

	UniMode getUnicodeMode() const {
		return _unicodeMode;
	};

	void setUnicodeMode(UniMode mode) {
		_unicodeMode = mode;
		doNotify(BufferChangeUnicode | BufferChangeDirty);
	};

	int getEncoding() const {
		return _encoding;
	};

	void setEncoding(int encoding) {
		_encoding = encoding;
        doNotify(BufferChangeUnicode | BufferChangeDirty);
	};

	DocFileStatus getStatus() const {
		return _currentStatus;
	};

	Document getDocument() {
		return _doc;
	};

	void setDirty(bool dirty) {
		_isDirty = dirty;
		doNotify(BufferChangeDirty);
	};

    void setPosition(const Position & pos, ScintillaEditView * identifier);
	Position & getPosition(ScintillaEditView * identifier);

	void setHeaderLineState(const std::vector<size_t> & folds, ScintillaEditView * identifier);
	const std::vector<size_t> & getHeaderLineState(const ScintillaEditView * identifier) const;

	bool isUserDefineLangExt() const {
		return (_userLangExt[0] != '\0');
	};

	const TCHAR * getUserDefineLangName() const {
		return _userLangExt.c_str();
	};

	const TCHAR * getCommentLineSymbol() const {
		Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentLineSymbol;

	};

	const TCHAR * getCommentStart() const {
		Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentStart;
	};

    const TCHAR * getCommentEnd() const {
		Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentEnd;
	};

	bool getNeedsLexing() const {
		return _needLexer;
	};

	void setNeedsLexing(bool lex) {
		_needLexer = lex;
		doNotify(BufferChangeLexing);
	};

	//these two return reference count after operation
	int addReference(ScintillaEditView * identifier);		//if ID not registered, creates a new Position for that ID and new foldstate
	int removeReference(ScintillaEditView * identifier);		//reduces reference. If zero, Document is purged

	void setHideLineChanged(bool isHide, int location);

	void setDeferredReload();

	bool getNeedReload() {
		return _needReloading;
	}

	void setNeedReload(bool reload) {
		_needReloading = reload;
	}

	/*
	pair<size_t, bool> getLineUndoState(size_t currentLine) const;
	void setLineUndoState(size_t currentLine, size_t undoLevel, bool isSaved = false);
	*/

	int docLength() const {
		return _pManager->docLength(_id);
	};

	int getFileLength(); // return file length. -1 if file is not existing.

	enum fileTimeType {ft_created, ft_modified, ft_accessed};
	generic_string getFileTime(fileTimeType ftt);

    Lang * getCurrentLang() const;
	
	bool isModified() const {return _isModified;};
	void setModifiedStatus(bool isModified) {_isModified = isModified;};
	generic_string getBackupFileName() const {return _backupFileName;};
	void setBackupFileName(generic_string fileName) {_backupFileName = fileName;};
	time_t getLastModifiedTimestamp() const {return _timeStamp;};
	bool isLoadedDirty() const {
		return _isLoadedDirty;
	};

	void setLoadedDirty(bool val) {
		_isLoadedDirty = val;
	};

private :
	FileManager * _pManager;
	bool _canNotify;
	int _references;							//if no references file inaccessible, can be closed
	BufferID _id;

	//document properties
	Document _doc;	//invariable
	LangType _lang;
	generic_string _userLangExt; // it's useful if only (_lang == L_USER)
	bool _isDirty;
	formatType _format;
	UniMode _unicodeMode;
	int _encoding;
	bool _isUserReadOnly;
	bool _needLexer;	//initially true
	//these properties have to be duplicated because of multiple references
	//All the vectors must have the same size at all times
	std::vector< ScintillaEditView * > _referees;
	std::vector< Position > _positions;
	std::vector< std::vector<size_t> > _foldStates;

	//vector< pair<size_t, pair<size_t, bool> > > _linesUndoState;

	//Environment properties
	DocFileStatus _currentStatus;
	time_t _timeStamp; // 0 if it's a new doc
	
	bool _isFileReadOnly;
	generic_string _fullPathName;
	TCHAR * _fileName;	//points to filename part in _fullPathName
	bool _needReloading;	//True if Buffer needs to be reloaded on activation

	long _recentTag;
	static long _recentTagCtr;

	// For backup system
	generic_string _backupFileName; // default: ""
	bool _isModified; // default: false
	bool _isLoadedDirty; // default: false

	void updateTimeStamp();

	int indexOfReference(const ScintillaEditView * identifier) const;

	void setStatus(DocFileStatus status) {
		_currentStatus = status;
		doNotify(BufferChangeStatus);
	}

	void doNotify(int mask) {
		if (_canNotify)
			_pManager->beNotifiedOfBufferChange(this, mask); 
	};

	Buffer(const Buffer&) { assert(false);  }
	Buffer& operator = (const Buffer&) { assert(false);  return *this; }
};

#endif //BUFFER_H
