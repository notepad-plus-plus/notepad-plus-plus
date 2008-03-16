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

#ifndef BUFFER_H
#define BUFFER_H

#include <shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Scintilla.h"
#include "Parameters.h"

const char UNTITLED_STR[] = "new ";
typedef sptr_t Document;

// Ordre important!! Ne le changes pas!
//SC_EOL_CRLF (0), SC_EOL_CR (1), or SC_EOL_LF (2).


const int CR = 0x0D;
const int LF = 0x0A;

enum docFileStaus{NEW_DOC, FILE_DELETED, NO_PROBLEM, MODIFIED_FROM_OUTSIDE};

struct HeaderLineState {
	HeaderLineState() : _headerLineNumber(0), _isCollapsed(false){};
	HeaderLineState(int lineNumber, bool isFoldUp) : _headerLineNumber(lineNumber), _isCollapsed(isFoldUp){};
	int _headerLineNumber;
	bool _isCollapsed;
};
/*
struct Position
{ 
	int _firstVisibleLine;
	int _startPos;
	int _endPos;
	int _xOffset;
};
*/
//#define USER_LANG_CURRENT ""
const int userLangNameMax = 16;

static bool isInList(const char *token, const char *list) {
	if ((!token) || (!list))
		return false;
	char word[64];
	int i = 0;
	int j = 0;
	for (; i <= int(strlen(list)) ; i++)
	{
		if ((list[i] == ' ')||(list[i] == '\0'))
		{
			if (j != 0)
			{
				word[j] = '\0';
				j = 0;
				
				if (!stricmp(token, word))
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


class Buffer
{
friend class ScintillaEditView;
friend class Notepad_plus;
public :
	Buffer(Document doc, const char *fileName)
		: _isDirty(false), _doc(doc), _isReadOnly(false), _isSetReadOnly(false), _recentTag(-1)
	{
		NppParameters *pNppParamInst = NppParameters::getInstance();
		const NewDocDefaultSettings & ndds = (pNppParamInst->getNppGUI()).getNewDocDefaultSettings();
		_format = ndds._format;
		_unicodeMode = ndds._encoding;

		_pos._firstVisibleLine = 0;
		_pos._startPos = 0;
		_pos._endPos = 0;
		_pos._xOffset = 0;
		setFileName(fileName, ndds._lang);
		//_userLangExt[0] = '\0';
	};

    Buffer(const Buffer & buf) : _isDirty(buf._isDirty),  _doc(buf._doc), _lang(buf._lang),
        _timeStamp(buf._timeStamp), _isReadOnly(buf._isReadOnly), _isSetReadOnly(buf._isSetReadOnly), _pos(buf._pos),
		_format(buf._format),_unicodeMode(buf._unicodeMode), _foldState(buf._foldState), _recentTag(buf._recentTag),
		_dontBotherMeAnymore(false), _reloadOnSwitchBack(false)
    {
        strcpy(_fullPathName, buf._fullPathName);
		strcpy(_userLangExt, buf._userLangExt);
    };

    Buffer & operator=(const Buffer & buf)
    {
        if (this != &buf)
        {
            this->_isDirty = buf._isDirty;
            this->_doc = buf._doc;
            this->_lang = buf._lang;
            this->_timeStamp = buf._timeStamp;
            this->_isReadOnly = buf._isReadOnly;
            this->_isSetReadOnly = buf._isSetReadOnly;
            this->_pos = buf._pos;
            this->_format = buf._format;
			this->_unicodeMode = buf._unicodeMode;
			this->_foldState = buf._foldState;
			this->_recentTag = buf._recentTag;

			strcpy(this->_fullPathName, buf._fullPathName);
            strcpy(this->_userLangExt, buf._userLangExt);
        }
        return *this;
    }

	LangType getLangFromExt(const char *ext);

	// this method 1. copies the file name
	//             2. determinates the language from the ext of file name
	//             3. gets the last modified time
	void setFileName(const char *fn, LangType defaultLang = L_TXT);

	const char * getFileName() const {return _fullPathName;};

	void updatTimeStamp() {
		struct _stat buf;
		_timeStamp = (_stat(_fullPathName, &buf)==0)?buf.st_mtime:0;
	};

	void increaseRecentTag() {
		_recentTag = ++_recentTagCtr;
	}

	long getRecentTag() const {
		return _recentTag;
	}

	docFileStaus checkFileState() {
		if (isUntitled(_fullPathName))
		{
			_isReadOnly = false;
			return NEW_DOC;
		}
        if (!PathFileExists(_fullPathName))
		{
			_isReadOnly = false;
			return FILE_DELETED;
		}
		struct _stat buf;
		if (!_stat(_fullPathName, &buf))
		{
			_isReadOnly = (bool)(!(buf.st_mode & _S_IWRITE));

			if (_timeStamp != buf.st_mtime)
				return MODIFIED_FROM_OUTSIDE;
		}
		return NO_PROBLEM;
	};

	// to use this method with open and save
	void checkIfReadOnlyFile() {
		struct _stat buf;
		if (!_stat(_fullPathName, &buf))
		{
			_isReadOnly = (bool)(!(buf.st_mode & _S_IWRITE));
		}
	};

    bool isDirty() const {
        return _isDirty;
    };

    bool isReadOnly() const {
        return (_isReadOnly || _isSetReadOnly);
    };
    
	bool isSystemReadOnly() const {
        return _isReadOnly;
    };

	bool isUserReadOnly() const {
        return _isSetReadOnly;
    };

	bool setReadOnly(bool ro) {
		bool oldVal = _isSetReadOnly;
		_isSetReadOnly = ro;
        return oldVal;
    };
	
    time_t getTimeStamp() const {
        return _timeStamp;
    };

    void synchroniseWith(const Buffer & buf) {
        _isDirty = buf.isDirty();
        _timeStamp = buf.getTimeStamp();
    };

	// that is : the prefix of the string is "new "
	static bool isUntitled(const char *str2Test) {
		return (strncmp(str2Test, UNTITLED_STR, sizeof(UNTITLED_STR)-1) == 0);
	}

	void setFormat(formatType format) {
		_format = format;
	};

	void determinateFormat(char *data) {
		size_t len = strlen(data);
		for (size_t i = 0 ; i < len ; i++)
		{
			if (data[i] == CR)
			{
				if (data[i+1] == LF)
				{
					_format = WIN_FORMAT;
					return;
				}
				else
				{
					_format = MAC_FORMAT;
					return;
				}
			}
			if (data[i] == LF)
			{
				_format = UNIX_FORMAT;
				return;
			}
		}
		_format = WIN_FORMAT;
	};
/*
	void detectBin(char *data) {
		size_t len = strlen(data);
		const size_t lenMax = 2048;

		size_t size2Detect = (len > lenMax)?lenMax:len;
		for (size_t i = 0 ; i < size2Detect ; i++)
		{
			if (isNotPrintableChar(data[i]))
			{
				_isBinary = true;
				break;
			}
		}
	};
*/
	formatType getFormat() const {
		return _format;
	};

	bool isUserDefineLangExt() const {
		return (_userLangExt[0] != '\0');
	};

	const char * getUserDefineLangName() const {return _userLangExt;};

	void setUnicodeMode(UniMode mode) {
		if ((_unicodeMode != mode) && !((_unicodeMode == uni8Bit) && (mode == uniCookie)) && \
			!((_unicodeMode == uniCookie) && (mode == uni8Bit)))
			_isDirty = true;
		_unicodeMode = mode;
	};
	UniMode getUnicodeMode() const {return _unicodeMode;};

	const char * getCommentLineSymbol() const {
		Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentLineSymbol;

	};
	const char * getCommentStart() const {
		Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentStart;
	};
    const char * getCommentEnd() const {
		Lang *l = getCurrentLang();
		if (!l)
			return NULL;
		return l->_pCommentEnd;
	};

    const Position & getPosition() const {
       return _pos;
    };

    LangType getLangType() const {
       return _lang;
    };

    void setPosition(const Position& pos) {
       _pos = pos;
    };

	//bool isBin() const {return _isBinary;};

private :
	bool _isDirty;
	Document _doc;
	LangType _lang;
	char _userLangExt[userLangNameMax]; // it's useful if only (_lang == L_USER)

	time_t _timeStamp; // 0 if it's a new doc
	bool _isReadOnly;
	bool _isSetReadOnly;
	Position _pos;
	char _fullPathName[MAX_PATH];
	formatType _format;
	UniMode _unicodeMode;
	std::vector<HeaderLineState> _foldState;
	long _recentTag;
	static long _recentTagCtr;
	//bool _isBinary;
	bool _dontBotherMeAnymore;
	bool _reloadOnSwitchBack;

	Lang * getCurrentLang() const {
		int i = 0 ;
		Lang *l = NppParameters::getInstance()->getLangFromIndex(i++);
		while (l)
		{
			if (l->_langID == _lang)
				return l;

			l = (NppParameters::getInstance())->getLangFromIndex(i++);
		}
		return NULL;
	};
};

#endif //BUFFER_H
