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

#include "tinyxmlA.h"
#include "tinyxml.h"
#include "Scintilla.h"
#include "ScintillaRef.h"
#include "ToolBar.h"
#include "UserDefineLangReference.h"
#include "colors.h"
#include "shortcut.h"
#include "ContextMenu.h"
#include "dpiManager.h"
#include "NppDarkMode.h"
#include <assert.h>
#include <tchar.h>
#include <map>
#include <array>
#include <shlwapi.h>
#include "ILexer.h"
#include "Lexilla.h"
#include "DockingCont.h"

#ifdef _WIN64

#ifdef _M_ARM64
#define ARCH_TYPE IMAGE_FILE_MACHINE_ARM64
#else
#define ARCH_TYPE IMAGE_FILE_MACHINE_AMD64
#endif

#else
#define ARCH_TYPE IMAGE_FILE_MACHINE_I386

#endif

#define CMD_INTERPRETER L"%COMSPEC%"

class NativeLangSpeaker;

const bool POS_VERTICAL = true;
const bool POS_HORIZOTAL = false;

const int UDD_SHOW   = 1; // 0000 0001
const int UDD_DOCKED = 2; // 0000 0010

// 0 : 0000 0000 hide & undocked
// 1 : 0000 0001 show & undocked
// 2 : 0000 0010 hide & docked
// 3 : 0000 0011 show & docked

const int TAB_DRAWTOPBAR            =    1;    // 0000 0000 0000 0001
const int TAB_DRAWINACTIVETAB       =    2;    // 0000 0000 0000 0010
const int TAB_DRAGNDROP             =    4;    // 0000 0000 0000 0100
const int TAB_REDUCE                =    8;    // 0000 0000 0000 1000
const int TAB_CLOSEBUTTON           =   16;    // 0000 0000 0001 0000
const int TAB_DBCLK2CLOSE           =   32;    // 0000 0000 0010 0000
const int TAB_VERTICAL              =   64;    // 0000 0000 0100 0000
const int TAB_MULTILINE             =  128;    // 0000 0000 1000 0000
const int TAB_HIDE                  =  256;    // 0000 0001 0000 0000
const int TAB_QUITONEMPTY           =  512;    // 0000 0010 0000 0000
const int TAB_ALTICONS              = 1024;    // 0000 0100 0000 0000
const int TAB_PINBUTTON             = 2048;    // 0000 1000 0000 0000
const int TAB_INACTIVETABSHOWBUTTON = 4096;    // 0001 0000 0000 0000

const bool activeText = true;
const bool activeNumeric = false;

enum class EolType: std::uint8_t
{
	windows,
	macos,
	unix,

	// special values
	unknown, // can not be the first value for legacy code
	osdefault = windows,
};

/*!
** \brief Convert an int into a FormatType
** \param value An arbitrary int
** \param defvalue The default value to use if an invalid value is provided
*/
EolType convertIntToFormatType(int value, EolType defvalue = EolType::osdefault);




enum UniMode {
	uni8Bit       = 0,  // ANSI
	uniUTF8       = 1,  // UTF-8 with BOM
	uni16BE       = 2,  // UTF-16 Big Ending with BOM
	uni16LE       = 3,  // UTF-16 Little Ending with BOM
	uniCookie     = 4,  // UTF-8 without BOM
	uni7Bit       = 5,  // 
	uni16BE_NoBOM = 6,  // UTF-16 Big Ending without BOM
	uni16LE_NoBOM = 7,  // UTF-16 Little Ending without BOM
	uniEnd};

enum ChangeDetect { cdDisabled = 0x0, cdEnabledOld = 0x01, cdEnabledNew = 0x02, cdAutoUpdate = 0x04, cdGo2end = 0x08 };
enum BackupFeature {bak_none = 0, bak_simple = 1, bak_verbose = 2};
enum OpenSaveDirSetting {dir_followCurrent = 0, dir_last = 1, dir_userDef = 2};
enum MultiInstSetting {monoInst = 0, multiInstOnSession = 1, multiInst = 2};
enum writeTechnologyEngine {defaultTechnology = 0, directWriteTechnology = 1};
enum urlMode {urlDisable = 0, urlNoUnderLineFg, urlUnderLineFg, urlNoUnderLineBg, urlUnderLineBg,
              urlMin = urlDisable,
              urlMax = urlUnderLineBg};

enum AutoIndentMode { autoIndent_none = 0, autoIndent_advanced = 1, autoIndent_basic = 2 };
enum SysTrayAction { sta_none = 0, sta_minimize = 1, sta_close = 2, sta_minimize_close = 3 };

const int LANG_INDEX_INSTR = 0;
const int LANG_INDEX_INSTR2 = 1;
const int LANG_INDEX_TYPE = 2;
const int LANG_INDEX_TYPE2 = 3;
const int LANG_INDEX_TYPE3 = 4;
const int LANG_INDEX_TYPE4 = 5;
const int LANG_INDEX_TYPE5 = 6;
const int LANG_INDEX_TYPE6 = 7;
const int LANG_INDEX_TYPE7 = 8;
const int LANG_INDEX_SUBSTYLE1 = 9;
const int LANG_INDEX_SUBSTYLE2 = 10;
const int LANG_INDEX_SUBSTYLE3 = 11;
const int LANG_INDEX_SUBSTYLE4 = 12;
const int LANG_INDEX_SUBSTYLE5 = 13;
const int LANG_INDEX_SUBSTYLE6 = 14;
const int LANG_INDEX_SUBSTYLE7 = 15;
const int LANG_INDEX_SUBSTYLE8 = 16;

const int COPYDATA_PARAMS = 0;
//const int COPYDATA_FILENAMESA = 1; // obsolete, no more useful
const int COPYDATA_FILENAMESW = 2;
const int COPYDATA_FULL_CMDLINE = 3;

#define PURE_LC_NONE	0
#define PURE_LC_BOL	 1
#define PURE_LC_WSP	 2

#define DECSEP_DOT	  0
#define DECSEP_COMMA	1
#define DECSEP_BOTH	 2


#define DROPBOX_AVAILABLE 1
#define ONEDRIVE_AVAILABLE 2
#define GOOGLEDRIVE_AVAILABLE 4

#define NPP_STYLING_FILESIZE_LIMIT_DEFAULT (200 * 1024 * 1024) // 200MB+ file won't be styled

const int FINDREPLACE_INSELECTION_THRESHOLD_DEFAULT = 1024;

const wchar_t fontSizeStrs[][3] = {L"", L"5", L"6", L"7", L"8", L"9", L"10", L"11", L"12", L"14", L"16", L"18", L"20", L"22", L"24", L"26", L"28"};

const wchar_t localConfFile[] = L"doLocalConf.xml";
const wchar_t notepadStyleFile[] = L"asNotepad.xml";

// issue xml/log file name
const wchar_t nppLogNetworkDriveIssue[] = L"nppLogNetworkDriveIssue";
const wchar_t nppLogNulContentCorruptionIssue[] = L"nppLogNulContentCorruptionIssue";

void cutString(const wchar_t *str2cut, std::vector<std::wstring> & patternVect);
void cutStringBy(const wchar_t *str2cut, std::vector<std::wstring> & patternVect, char byChar, bool allowEmptyStr);

// style names
const wchar_t g_npcStyleName[] = L"Non-printing characters custom color";

struct Position
{
	intptr_t _firstVisibleLine = 0;
	intptr_t _startPos = 0;
	intptr_t _endPos = 0;
	intptr_t _xOffset = 0;
	intptr_t _selMode = 0;
	intptr_t _scrollWidth = 1;
	intptr_t _offset = 0;
	intptr_t _wrapCount = 0;
};


struct MapPosition
{
private:
	intptr_t _maxPeekLenInKB = 512; // 512 KB
public:
	intptr_t _firstVisibleDisplayLine = -1;

	intptr_t _firstVisibleDocLine = -1; // map
	intptr_t _lastVisibleDocLine = -1;  // map
	intptr_t _nbLine = -1;              // map
	intptr_t _higherPos = -1;           // map
	intptr_t _width = -1;
	intptr_t _height = -1;
	intptr_t _wrapIndentMode = -1;

	intptr_t _KByteInDoc = _maxPeekLenInKB;

	bool _isWrap = false;
	bool isValid() const { return (_firstVisibleDisplayLine != -1); };
	bool canScroll() const { return (_KByteInDoc < _maxPeekLenInKB); }; // _nbCharInDoc < _maxPeekLen : Don't scroll the document for the performance issue
};


struct sessionFileInfo : public Position
{
	sessionFileInfo(const wchar_t* fn, const wchar_t *ln, int encoding, bool userReadOnly,bool isPinned, const Position& pos, const wchar_t *backupFilePath, FILETIME originalFileLastModifTimestamp, const MapPosition & mapPos) :
		Position(pos), _encoding(encoding), _isUserReadOnly(userReadOnly), _isPinned(isPinned), _originalFileLastModifTimestamp(originalFileLastModifTimestamp), _mapPos(mapPos)
	{
		if (fn) _fileName = fn;
		if (ln)	_langName = ln;
		if (backupFilePath) _backupFilePath = backupFilePath;
	}

	sessionFileInfo(const std::wstring& fn) : _fileName(fn) {}

	std::wstring _fileName;
	std::wstring _langName;
	std::vector<size_t> _marks;
	std::vector<size_t> _foldStates;
	int	_encoding = -1;
	bool _isUserReadOnly = false;
	bool _isMonitoring = false;
	int _individualTabColour = -1;
	bool _isRTL = false;
	bool _isPinned = false;
	std::wstring _backupFilePath;
	FILETIME _originalFileLastModifTimestamp {};

	MapPosition _mapPos;
};


struct Session
{
	size_t nbMainFiles() const {return _mainViewFiles.size();};
	size_t nbSubFiles() const {return _subViewFiles.size();};
	size_t _activeView = 0;
	size_t _activeMainIndex = 0;
	size_t _activeSubIndex = 0;
	bool _includeFileBrowser = false;
	std::wstring _fileBrowserSelectedItem;
	std::vector<sessionFileInfo> _mainViewFiles;
	std::vector<sessionFileInfo> _subViewFiles;
	std::vector<std::wstring> _fileBrowserRoots;
};


struct CmdLineParams
{
	bool _isNoPlugin = false;
	bool _isReadOnly = false;
	bool _isNoSession = false;
	bool _isNoTab = false;
	bool _isPreLaunch = false;
	bool _showLoadingTime = false;
	bool _alwaysOnTop = false;
	intptr_t _line2go   = -1;
	intptr_t _column2go = -1;
	intptr_t _pos2go = -1;

	POINT _point = {};
	bool _isPointXValid = false;
	bool _isPointYValid = false;

	bool _isSessionFile = false;
	bool _isRecursive = false;
	bool _openFoldersAsWorkspace = false;
	bool _monitorFiles = false;

	LangType _langType = L_EXTERNAL;
	std::wstring _localizationPath;
	std::wstring _udlName;
	std::wstring _pluginMessage;

	std::wstring _easterEggName;
	unsigned char _quoteType = 0;
	int _ghostTypingSpeed = -1; // -1: initial value  1: slow  2: fast  3: speed of light

	CmdLineParams()
	{
		_point.x = 0;
		_point.y = 0;
	}

	bool isPointValid() const
	{
		return _isPointXValid && _isPointYValid;
	}
};

// A POD class to send CmdLineParams through WM_COPYDATA and to Notepad_plus::loadCommandlineParams
struct CmdLineParamsDTO
{
	bool _isReadOnly = false;
	bool _isNoSession = false;
	bool _isSessionFile = false;
	bool _isRecursive = false;
	bool _openFoldersAsWorkspace = false;
	bool _monitorFiles = false;

	intptr_t _line2go = 0;
	intptr_t _column2go = 0;
	intptr_t _pos2go = 0;

	LangType _langType = L_EXTERNAL;
	wchar_t _udlName[MAX_PATH] = {'\0'};
	wchar_t _pluginMessage[MAX_PATH] = {'\0'};

	static CmdLineParamsDTO FromCmdLineParams(const CmdLineParams& params)
	{
		CmdLineParamsDTO dto;
		dto._isReadOnly = params._isReadOnly;
		dto._isNoSession = params._isNoSession;
		dto._isSessionFile = params._isSessionFile;
		dto._isRecursive = params._isRecursive;
		dto._openFoldersAsWorkspace = params._openFoldersAsWorkspace;
		dto._monitorFiles = params._monitorFiles;

		dto._line2go = params._line2go;
		dto._column2go = params._column2go;
		dto._pos2go = params._pos2go;

		dto._langType = params._langType;
		wcsncpy(dto._udlName, params._udlName.c_str(), MAX_PATH);
		wcsncpy(dto._pluginMessage, params._pluginMessage.c_str(), MAX_PATH);
		return dto;
	}
};

#define FWI_PANEL_WH_DEFAULT 100
struct FloatingWindowInfo
{
	int _cont = 0;
	RECT _pos = { 0, 0, FWI_PANEL_WH_DEFAULT, FWI_PANEL_WH_DEFAULT };

	FloatingWindowInfo(int cont, int x, int y, int w, int h)
		: _cont(cont)
	{
		_pos.left	= x;
		_pos.top	= y;
		_pos.right	= w;
		_pos.bottom = h;
	}
};


struct PluginDlgDockingInfo final
{
	std::wstring _name;
	int _internalID = -1;

	int _currContainer = -1;
	int _prevContainer = -1;
	bool _isVisible = false;

	PluginDlgDockingInfo(const wchar_t* pluginName, int id, int curr, int prev, bool isVis)
		: _name(pluginName), _internalID(id), _currContainer(curr), _prevContainer(prev), _isVisible(isVis)
	{}

	bool operator == (const PluginDlgDockingInfo& rhs) const
	{
		return _internalID == rhs._internalID and _name == rhs._name;
	}
};


struct ContainerTabInfo final
{
	int _cont = 0;
	int _activeTab = 0;

	ContainerTabInfo(int cont, int activeTab) : _cont(cont), _activeTab(activeTab) {};
};


#define DMD_PANEL_WH_DEFAULT 200
struct DockingManagerData final
{
	int _leftWidth = DMD_PANEL_WH_DEFAULT;
	int _rightWidth = DMD_PANEL_WH_DEFAULT;
	int _topHeight = DMD_PANEL_WH_DEFAULT;
	int _bottomHeight = DMD_PANEL_WH_DEFAULT;

	// will be updated at runtime (Notepad_plus::init & DockingManager::runProc DMM_MOVE_SPLITTER)
	LONG _minDockedPanelVisibility = HIGH_CAPTION; 
	SIZE _minFloatingPanelSize = { (HIGH_CAPTION) * 6, HIGH_CAPTION };

	std::vector<FloatingWindowInfo> _floatingWindowInfo;
	std::vector<PluginDlgDockingInfo> _pluginDockInfo;
	std::vector<ContainerTabInfo> _containerTabInfo;

	bool getFloatingRCFrom(int floatCont, RECT& rc) const
	{
		for (size_t i = 0, fwiLen = _floatingWindowInfo.size(); i < fwiLen; ++i)
		{
			if (_floatingWindowInfo[i]._cont == floatCont)
			{
				rc.left   = _floatingWindowInfo[i]._pos.left;
				rc.top	= _floatingWindowInfo[i]._pos.top;
				rc.right  = _floatingWindowInfo[i]._pos.right;
				rc.bottom = _floatingWindowInfo[i]._pos.bottom;
				return true;
			}
		}
		return false;
	}
};



const int FONTSTYLE_NONE = 0;
const int FONTSTYLE_BOLD = 1;
const int FONTSTYLE_ITALIC = 2;
const int FONTSTYLE_UNDERLINE = 4;

const int STYLE_NOT_USED = -1;

const int COLORSTYLE_FOREGROUND = 0x01;
const int COLORSTYLE_BACKGROUND = 0x02;
const int COLORSTYLE_ALL = COLORSTYLE_FOREGROUND|COLORSTYLE_BACKGROUND;



struct Style final
{
	int _styleID = STYLE_NOT_USED;
	std::wstring _styleDesc;

	COLORREF _fgColor = COLORREF(STYLE_NOT_USED);
	COLORREF _bgColor = COLORREF(STYLE_NOT_USED);
	int _colorStyle = COLORSTYLE_ALL;

	bool _isFontEnabled = false;
	std::wstring _fontName;
	int _fontStyle = STYLE_NOT_USED;
	int _fontSize = STYLE_NOT_USED;

	int _nesting = FONTSTYLE_NONE;

	int _keywordClass = STYLE_NOT_USED;
	std::wstring _keywords;
};


struct GlobalOverride final
{
	bool isEnable() const {return (enableFg || enableBg || enableFont || enableFontSize || enableBold || enableItalic || enableUnderLine);}
	bool enableFg = false;
	bool enableBg = false;
	bool enableFont = false;
	bool enableFontSize = false;
	bool enableBold = false;
	bool enableItalic = false;
	bool enableUnderLine = false;
};

struct StyleArray
{
	auto begin() { return _styleVect.begin(); };
	auto end() { return _styleVect.end(); };
	void clear() { _styleVect.clear(); };

	Style& getStyler(size_t index) {
		if (index >=  _styleVect.size())
			throw std::out_of_range("Styler index out of range");
		return _styleVect[index];
	};

	void addStyler(int styleID, TiXmlNode *styleNode);

	void addStyler(int styleID, const std::wstring& styleName) {
		_styleVect.emplace_back();
		Style& s = _styleVect.back();
		s._styleID = styleID;
		s._styleDesc = styleName;
		s._fgColor = black;
		s._bgColor = white;
	};

	Style* findByID(int id) {
		for (size_t i = 0; i < _styleVect.size(); ++i)
		{
			if (_styleVect[i]._styleID == id)
				return &(_styleVect[i]);
		}
		return nullptr;
	};

	Style* findByName(const std::wstring& name) {
		for (size_t i = 0; i < _styleVect.size(); ++i)
		{
			if (_styleVect[i]._styleDesc == name)
				return &(_styleVect[i]);
		}
		return nullptr;
	};

protected:
	std::vector<Style> _styleVect;
};



struct LexerStyler : public StyleArray
{
public:
	LexerStyler & operator=(const LexerStyler & ls)
	{
		if (this != &ls)
		{
			*(static_cast<StyleArray *>(this)) = ls;
			this->_lexerName = ls._lexerName;
			this->_lexerDesc = ls._lexerDesc;
			this->_lexerUserExt = ls._lexerUserExt;
		}
		return *this;
	}

	void setLexerName(const wchar_t *lexerName)
	{
		_lexerName = lexerName;
	}

	void setLexerDesc(const wchar_t *lexerDesc)
	{
		_lexerDesc = lexerDesc;
	}

	void setLexerUserExt(const wchar_t *lexerUserExt) {
		_lexerUserExt = lexerUserExt;
	};

	const wchar_t * getLexerName() const {return _lexerName.c_str();};
	const wchar_t * getLexerDesc() const {return _lexerDesc.c_str();};
	const wchar_t * getLexerUserExt() const {return _lexerUserExt.c_str();};

private :
	std::wstring _lexerName;
	std::wstring _lexerDesc;
	std::wstring _lexerUserExt;
};

struct SortLexersInAlphabeticalOrder {
	bool operator() (const LexerStyler& l, const LexerStyler& r) {
		if (!lstrcmp(l.getLexerDesc(), L"Search result"))
			return false;
		if (!lstrcmp(r.getLexerDesc(), L"Search result"))
			return true;
		return lstrcmp(l.getLexerDesc(), r.getLexerDesc()) < 0;
	}
};

struct LexerStylerArray
{
	size_t getNbLexer() const { return _lexerStylerVect.size(); }
	void clear() { _lexerStylerVect.clear(); }

	LexerStyler & getLexerFromIndex(size_t index)
	{
		assert(index < _lexerStylerVect.size());
		return _lexerStylerVect[index];
	};

	const wchar_t * getLexerNameFromIndex(size_t index) const { return _lexerStylerVect[index].getLexerName(); }
	const wchar_t * getLexerDescFromIndex(size_t index) const { return _lexerStylerVect[index].getLexerDesc(); }

	LexerStyler * getLexerStylerByName(const wchar_t *lexerName) {
		if (!lexerName) return nullptr;
		for (size_t i = 0 ; i < _lexerStylerVect.size() ; ++i)
		{
			if (!lstrcmp(_lexerStylerVect[i].getLexerName(), lexerName))
				return &(_lexerStylerVect[i]);
		}
		return nullptr;
	};

	void addLexerStyler(const wchar_t *lexerName, const wchar_t *lexerDesc, const wchar_t *lexerUserExt, TiXmlNode *lexerNode);

	void sort() {
		std::sort(_lexerStylerVect.begin(), _lexerStylerVect.end(), SortLexersInAlphabeticalOrder());
	};

private :
	std::vector<LexerStyler> _lexerStylerVect;
};


struct NewDocDefaultSettings final
{
	EolType _format = EolType::osdefault;
	UniMode _unicodeMode = uniCookie;
	bool _openAnsiAsUtf8 = true;
	LangType _lang = L_TEXT;
	int _codepage = -1; // -1 when not using
	bool _addNewDocumentOnStartup = false;
};


struct LangMenuItem final
{
	LangType _langType = L_TEXT;
	int	_cmdID = -1;
	std::wstring _langName;

	LangMenuItem(LangType lt, int cmdID = 0, const std::wstring& langName = L""):
	_langType(lt), _cmdID(cmdID), _langName(langName){};

	bool operator<(const LangMenuItem& rhs) const
	{
		std::wstring lhs_lang(this->_langName.length(), ' '), rhs_lang(rhs._langName.length(), ' ');
		std::transform(this->_langName.begin(), this->_langName.end(), lhs_lang.begin(), towlower);
		std::transform(rhs._langName.begin(), rhs._langName.end(), rhs_lang.begin(), towlower);
		return lhs_lang < rhs_lang;
	}
};

struct PrintSettings final {
	bool _printLineNumber = true;
	int _printOption = SC_PRINT_COLOURONWHITE;

	std::wstring _headerLeft;
	std::wstring _headerMiddle;
	std::wstring _headerRight;
	std::wstring _headerFontName;
	int _headerFontStyle = 0;
	int _headerFontSize = 0;

	std::wstring _footerLeft;
	std::wstring _footerMiddle;
	std::wstring _footerRight;
	std::wstring _footerFontName;
	int _footerFontStyle = 0;
	int _footerFontSize = 0;

	RECT _marge = {};

	PrintSettings() {
		_marge.left = 0; _marge.top = 0; _marge.right = 0; _marge.bottom = 0;
	};

	bool isHeaderPresent() const {
		return (!_headerLeft.empty() || !_headerMiddle.empty() || !_headerRight.empty());
	};

	bool isFooterPresent() const {
		return (!_footerLeft.empty() || !_footerMiddle.empty() || !_footerRight.empty());
	};

	bool isUserMargePresent() const {
		return ((_marge.left != 0) || (_marge.top != 0) || (_marge.right != 0) || (_marge.bottom != 0));
	};
};


class Date final
{
public:
	Date() = default;
	Date(unsigned long year, unsigned long month, unsigned long day)
		: _year(year)
		, _month(month)
		, _day(day)
	{
		assert(year > 0 && year <= 9999); // I don't think Notepad++ will last till AD 10000 :)
		assert(month > 0 && month <= 12);
		assert(day > 0 && day <= 31);
		assert(!(month == 2 && day > 29) &&
			   !(month == 4 && day > 30) &&
			   !(month == 6 && day > 30) &&
			   !(month == 9 && day > 30) &&
			   !(month == 11 && day > 30));
	}

	explicit Date(const wchar_t *dateStr);

	// The constructor which makes the date of number of days from now
	// nbDaysFromNow could be negative if user want to make a date in the past
	// if the value of nbDaysFromNow is 0 then the date will be now
	Date(int nbDaysFromNow);

	void now();

	std::wstring toString() const // Return Notepad++ date format : YYYYMMDD
	{
		wchar_t dateStr[16];
		wsprintf(dateStr, L"%04u%02u%02u", _year, _month, _day);
		return dateStr;
	}

	bool operator < (const Date & compare) const
	{
		if (this->_year != compare._year)
			return (this->_year < compare._year);
		if (this->_month != compare._month)
			return (this->_month < compare._month);
		return (this->_day < compare._day);
	}

	bool operator > (const Date & compare) const
	{
		if (this->_year != compare._year)
			return (this->_year > compare._year);
		if (this->_month != compare._month)
			return (this->_month > compare._month);
		return (this->_day > compare._day);
	}

	bool operator == (const Date & compare) const
	{
		if (this->_year != compare._year)
			return false;
		if (this->_month != compare._month)
			return false;
		return (this->_day == compare._day);
	}

	bool operator != (const Date & compare) const
	{
		if (this->_year != compare._year)
			return true;
		if (this->_month != compare._month)
			return true;
		return (this->_day != compare._day);
	}

private:
	unsigned long _year  = 2008;
	unsigned long _month = 4;
	unsigned long _day   = 26;
};


class MatchedPairConf final
{
public:
	bool hasUserDefinedPairs() const { return _matchedPairs.size() != 0; }
	bool hasDefaultPairs() const { return _doParentheses||_doBrackets||_doCurlyBrackets||_doQuotes||_doDoubleQuotes||_doHtmlXmlTag; }
	bool hasAnyPairsPair() const { return hasUserDefinedPairs() || hasDefaultPairs(); }

public:
	std::vector<std::pair<char, char>> _matchedPairs;
	bool _doHtmlXmlTag = false;
	bool _doParentheses = false;
	bool _doBrackets = false;
	bool _doCurlyBrackets = false;
	bool _doQuotes = false;
	bool _doDoubleQuotes = false;
};

struct DarkModeConf final
{
	bool _isEnabled = false;
	bool _isEnabledPlugin = true;
	NppDarkMode::ColorTone _colorTone = NppDarkMode::blackTone;
	NppDarkMode::Colors _customColors = NppDarkMode::getDarkModeDefaultColors();
	NppDarkMode::AdvancedOptions _advOptions{};
};


struct LargeFileRestriction final
{
	int64_t _largeFileSizeDefInByte = NPP_STYLING_FILESIZE_LIMIT_DEFAULT;
	bool _isEnabled = true;

	bool _deactivateWordWrap = true;

	bool _allowBraceMatch = false;
	bool _allowAutoCompletion = false;
	bool _allowSmartHilite = false;
	bool _allowClickableLink = false;
	
	bool _suppress2GBWarning = false;
};

struct NppGUI final
{
	toolBarStatusType _toolBarStatus = TB_STANDARD;
	bool _toolbarShow = true;
	bool _statusBarShow = true;
	bool _menuBarShow = true;

	int _tabStatus = (TAB_DRAWTOPBAR | TAB_DRAWINACTIVETAB | TAB_DRAGNDROP | TAB_REDUCE | TAB_CLOSEBUTTON | TAB_PINBUTTON);

	bool _splitterPos = POS_VERTICAL;
	int _userDefineDlgStatus = UDD_DOCKED;

	int _tabSize = 4;
	bool _tabReplacedBySpace = false;
	bool _backspaceUnindent = false;

	bool _finderLinesAreCurrentlyWrapped = false;
	bool _finderPurgeBeforeEverySearch = false;
	bool _finderShowOnlyOneEntryPerFoundLine = true;

	int _fileAutoDetection = cdEnabledNew;

	bool _checkHistoryFiles = false;

	RECT _appPos {0, 0, 1024, 700};

	RECT _findWindowPos {};
	bool _findWindowLessMode = false;

	bool _isMaximized = false;
	int _isMinimizedToTray = sta_none;
	bool _rememberLastSession = true; // remember next session boolean will be written in the settings
	bool _keepSessionAbsentFileEntries = false;
	bool _isCmdlineNosessionActivated = false; // used for if -nosession is indicated on the launch time
	bool _detectEncoding = true;
	bool _saveAllConfirm = true;
	bool _setSaveDlgExtFiltToAllTypes = false;
	bool _doTaskList = true;
	AutoIndentMode _maintainIndent = autoIndent_advanced;
	bool _enableSmartHilite = true;

	bool _smartHiliteCaseSensitive = false;
	bool _smartHiliteWordOnly = true;
	bool _smartHiliteUseFindSettings = false;
	bool _smartHiliteOnAnotherView = false;

	bool _markAllCaseSensitive = false;
	bool _markAllWordOnly = true;

	bool _disableSmartHiliteTmp = false;
	bool _enableTagsMatchHilite = true;
	bool _enableTagAttrsHilite = true;
	bool _enableHiliteNonHTMLZone = false;
	bool _styleMRU = true;
	char _leftmostDelimiter = '(';
	char _rightmostDelimiter = ')';
	bool _delimiterSelectionOnEntireDocument = false;
	bool _backSlashIsEscapeCharacterForSql = true;
	bool _fillFindFieldWithSelected = true;
	bool _fillFindFieldSelectCaret = true;
	bool _monospacedFontFindDlg = false;
	bool _findDlgAlwaysVisible = false;
	bool _confirmReplaceInAllOpenDocs = true;
	bool _replaceStopsWithoutFindingNext = false;
	int _inSelectionAutocheckThreshold = FINDREPLACE_INSELECTION_THRESHOLD_DEFAULT;
	bool _fillDirFieldFromActiveDoc = false;
	bool _muteSounds = false;
	bool _enableFoldCmdToggable = false;
	bool _hideMenuRightShortcuts = false;
	writeTechnologyEngine _writeTechnologyEngine = directWriteTechnology;
	bool _isWordCharDefault = true;
	std::string _customWordChars;
	urlMode _styleURL = urlUnderLineFg;
	std::wstring _uriSchemes = L"svn:// cvs:// git:// imap:// irc:// irc6:// ircs:// ldap:// ldaps:// news: telnet:// gopher:// ssh:// sftp:// smb:// skype: snmp:// spotify: steam:// sms: slack:// chrome:// bitcoin:";
	NewDocDefaultSettings _newDocDefaultSettings;

	std::wstring _dateTimeFormat = L"yyyy-MM-dd HH:mm:ss";
	bool _dateTimeReverseDefaultOrder = false;

	void setTabReplacedBySpace(bool b) {_tabReplacedBySpace = b;};
	const NewDocDefaultSettings & getNewDocDefaultSettings() const {return _newDocDefaultSettings;};
	std::vector<LangMenuItem> _excludedLangList;
	bool _isLangMenuCompact = true;

	PrintSettings _printSettings;
	BackupFeature _backup = bak_none;
	bool _useDir = false;
	std::wstring _backupDir;
	DockingManagerData _dockingData;
	GlobalOverride _globalOverride;
	enum AutocStatus{autoc_none, autoc_func, autoc_word, autoc_both};
	AutocStatus _autocStatus = autoc_both;
	UINT  _autocFromLen = 1;
	bool _autocIgnoreNumbers = true;
	bool _autocInsertSelectedUseENTER = true;
	bool _autocInsertSelectedUseTAB = true;
	bool _autocBrief = false;
	bool _funcParams = true;
	MatchedPairConf _matchedPairConf;

	std::wstring _definedSessionExt;
	std::wstring _definedWorkspaceExt;

	// items with no Notepad++ GUI to set
	std::wstring _commandLineInterpreter = CMD_INTERPRETER;

	struct AutoUpdateOptions
	{
		bool _doAutoUpdate = true;
		int _intervalDays = 15;
		Date _nextUpdateDate;
		AutoUpdateOptions(): _nextUpdateDate(Date()) {};
	}
	_autoUpdateOpt;

	bool _doesExistUpdater = false;
	int _caretBlinkRate = 600;
	int _caretWidth = 1;

	bool _shortTitlebar = false;

	OpenSaveDirSetting _openSaveDir = dir_followCurrent;

	wchar_t _defaultDir[MAX_PATH]{};
	wchar_t _defaultDirExp[MAX_PATH]{};	//expanded environment variables
	wchar_t _lastUsedDir[MAX_PATH]{};
	
	std::wstring _themeName;
	MultiInstSetting _multiInstSetting = monoInst;
	bool _clipboardHistoryPanelKeepState = false;
	bool _docListKeepState = false;
	bool _charPanelKeepState = false;
	bool _fileBrowserKeepState = false;
	bool _projectPanelKeepState = false;
	bool _docMapKeepState = false;
	bool _funcListKeepState = false;
	bool _pluginPanelKeepState = false;
	bool _fileSwitcherWithoutExtColumn = false;
	int _fileSwitcherExtWidth = 50;
	bool _fileSwitcherWithoutPathColumn = true;
	int _fileSwitcherPathWidth = 50;
	bool _fileSwitcherDisableListViewGroups = false;
	bool isSnapshotMode() const {return _isSnapshotMode && _rememberLastSession && !_isCmdlineNosessionActivated;};
	bool _isSnapshotMode = true;
	size_t _snapshotBackupTiming = 7000;
	std::wstring _cloudPath; // this option will never be read/written from/to config.xml
	unsigned char _availableClouds = '\0'; // this option will never be read/written from/to config.xml

	enum SearchEngineChoice{ se_custom = 0, se_duckDuckGo = 1, se_google = 2, se_bing = 3, se_yahoo = 4, se_stackoverflow = 5 };
	SearchEngineChoice _searchEngineChoice = se_google;
	std::wstring _searchEngineCustom;

	bool _isFolderDroppedOpenFiles = false;

	bool _isDocPeekOnTab = false;
	bool _isDocPeekOnMap = false;

	// function list should be sorted by default on new file open
	bool _shouldSortFunctionList = false;

	DarkModeConf _darkmode;

	LargeFileRestriction _largeFileRestriction;
};


struct ScintillaViewParams
{
	bool _lineNumberMarginShow = true;
	bool _lineNumberMarginDynamicWidth = true;
	bool _bookMarkMarginShow = true;
	
	bool _isChangeHistoryMarginEnabled = true;
	bool _isChangeHistoryIndicatorEnabled = false;
	changeHistoryState _isChangeHistoryEnabled4NextSession = changeHistoryState::margin; // no -> 0 (disable), yes -> 1 (margin), yes ->2 (indicator), yes-> 3 (margin + indicator)

	folderStyle  _folderStyle = FOLDER_STYLE_BOX; //"simple", "arrow", "circle", "box" and "none"
	lineWrapMethod _lineWrapMethod = LINEWRAP_ALIGNED;
	bool _foldMarginShow = true;
	bool _indentGuideLineShow = true;
	lineHiliteMode _currentLineHiliteMode = LINEHILITE_HILITE;
	unsigned char _currentLineFrameWidth = 1; // 1-6 pixel
	bool _wrapSymbolShow = false;
	bool _doWrap = false;
	bool _isEdgeBgMode = false;

	std::vector<size_t> _edgeMultiColumnPos;
	intptr_t _zoom = 0;
	intptr_t _zoom2 = 0;
	bool _whiteSpaceShow = false;
	bool _eolShow = false;
	enum crlfMode {plainText = 0, roundedRectangleText = 1, plainTextCustomColor = 2, roundedRectangleTextCustomColor = 3};
	crlfMode _eolMode = roundedRectangleText;
	bool _npcShow = false;
	enum npcMode { identity = 0, abbreviation = 1, codepoint = 2 };
	npcMode _npcMode = abbreviation;
	bool _npcCustomColor = false;
	bool _npcIncludeCcUniEol = false;
	bool _ccUniEolShow = true;
	bool _npcNoInputC0 = true;

	int _borderWidth = 2;
	bool _virtualSpace = false;
	bool _scrollBeyondLastLine = true;
	bool _rightClickKeepsSelection = false;
	bool _disableAdvancedScrolling = false;
	bool _doSmoothFont = false;
	bool _showBorderEdge = true;

	unsigned char _paddingLeft = 0;  // 0-9 pixel
	unsigned char _paddingRight = 0; // 0-9 pixel

	// distractionFreeDivPart is used for divising the fullscreen pixel width.
	// the result of division will be the left & right padding in Distraction Free mode
	unsigned char _distractionFreeDivPart = 4;     // 3-9 parts

	int getDistractionFreePadding(int editViewWidth) const {
		const int defaultDiviser = 4;
		int diviser = _distractionFreeDivPart > 2 ? _distractionFreeDivPart : defaultDiviser;
		int paddingLen = editViewWidth / diviser;
		if (paddingLen <= 0)
			paddingLen = editViewWidth / defaultDiviser;
		return paddingLen;
	};

	bool _lineCopyCutWithoutSelection = true;

	bool _multiSelection = true;      // if _multiSelection is false
	bool _columnSel2MultiEdit = true; // _columnSel2MultiEdit must be false
};

const int NB_LIST = 20;
const int NB_MAX_LRF_FILE = 30;
const int NB_MAX_USER_LANG = 30;
const int NB_MAX_EXTERNAL_LANG = 30;
const int NB_MAX_IMPORTED_UDL = 50;

constexpr int NB_DEFAULT_LRF_CUSTOMLENGTH = 100;
constexpr int NB_MAX_LRF_CUSTOMLENGTH = MAX_PATH - 1;

const int NB_MAX_FINDHISTORY_FIND	= 30;
const int NB_MAX_FINDHISTORY_REPLACE = 30;
const int NB_MAX_FINDHISTORY_PATH	= 30;
const int NB_MAX_FINDHISTORY_FILTER  = 20;


const int MASK_ReplaceBySpc = 0x80;
const int MASK_TabSize = 0x7F;




struct Lang final
{
	LangType _langID = L_TEXT;
	std::wstring _langName;
	const wchar_t* _defaultExtList = nullptr;
	const wchar_t* _langKeyWordList[NB_LIST];
	const wchar_t* _pCommentLineSymbol = nullptr;
	const wchar_t* _pCommentStart = nullptr;
	const wchar_t* _pCommentEnd = nullptr;

	bool _isTabReplacedBySpace = false;
	int _tabSize = -1;
	bool _isBackspaceUnindent = false;

	Lang()
	{
		for (int i = 0 ; i < NB_LIST ; _langKeyWordList[i] = NULL, ++i);
	}

	Lang(LangType langID, const wchar_t *name) : _langID(langID), _langName(name ? name : L"")
	{
		for (int i = 0 ; i < NB_LIST ; _langKeyWordList[i] = NULL, ++i);
	}

	~Lang() = default;

	void setDefaultExtList(const wchar_t *extLst){
		_defaultExtList = extLst;
	}

	void setCommentLineSymbol(const wchar_t *commentLine){
		_pCommentLineSymbol = commentLine;
	}

	void setCommentStart(const wchar_t *commentStart){
		_pCommentStart = commentStart;
	}

	void setCommentEnd(const wchar_t *commentEnd){
		_pCommentEnd = commentEnd;
	}

	void setTabInfo(int tabInfo, bool isBackspaceUnindent)
	{
		if (tabInfo != -1 && tabInfo & MASK_TabSize)
		{
			_isTabReplacedBySpace = (tabInfo & MASK_ReplaceBySpc) != 0;
			_tabSize = tabInfo & MASK_TabSize;
		}

		_isBackspaceUnindent = isBackspaceUnindent;
	}

	const wchar_t * getDefaultExtList() const {
		return _defaultExtList;
	}

	void setWords(const wchar_t *words, int index) {
		_langKeyWordList[index] = words;
	}

	const wchar_t * getWords(int index) const {
		return _langKeyWordList[index];
	}

	LangType getLangID() const {return _langID;};
	const wchar_t * getLangName() const {return _langName.c_str();};

	int getTabInfo() const
	{
		if (_tabSize == -1) return -1;
		return (_isTabReplacedBySpace?0x80:0x00) | _tabSize;
	}
};



class UserLangContainer final
{
public:
	UserLangContainer() :_name(L"new user define"), _ext(L""), _udlVersion(L"") {
		for (int i = 0; i < SCE_USER_KWLIST_TOTAL; ++i) *_keywordLists[i] = '\0';
	}

	UserLangContainer(const wchar_t *name, const wchar_t *ext, bool isDarkModeTheme, const wchar_t *udlVer):
		_name(name), _ext(ext), _isDarkModeTheme(isDarkModeTheme), _udlVersion(udlVer) {
		for (int i = 0; i < SCE_USER_KWLIST_TOTAL; ++i) *_keywordLists[i] = '\0';
	}

	UserLangContainer & operator = (const UserLangContainer & ulc)
	{
		if (this != &ulc)
		{
			this->_name = ulc._name;
			this->_ext = ulc._ext;
			this->_isDarkModeTheme = ulc._isDarkModeTheme;
			this->_udlVersion = ulc._udlVersion;
			this->_isCaseIgnored = ulc._isCaseIgnored;
			this->_styles = ulc._styles;
			this->_allowFoldOfComments = ulc._allowFoldOfComments;
			this->_forcePureLC = ulc._forcePureLC;
			this->_decimalSeparator = ulc._decimalSeparator;
			this->_foldCompact = ulc._foldCompact;
			for (Style & st : this->_styles)
			{
				if (st._bgColor == COLORREF(-1))
					st._bgColor = white;
				if (st._fgColor == COLORREF(-1))
					st._fgColor = black;
			}

			for (int i = 0 ; i < SCE_USER_KWLIST_TOTAL ; ++i)
				wcscpy_s(this->_keywordLists[i], ulc._keywordLists[i]);

			for (int i = 0 ; i < SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
				_isPrefix[i] = ulc._isPrefix[i];
		}
		return *this;
	}

	const wchar_t * getName() {return _name.c_str();};
	const wchar_t * getExtention() {return _ext.c_str();};
	const wchar_t * getUdlVersion() {return _udlVersion.c_str();};

private:
	StyleArray _styles;
	std::wstring _name;
	std::wstring _ext;
	bool _isDarkModeTheme = false;
	std::wstring _udlVersion;

	wchar_t _keywordLists[SCE_USER_KWLIST_TOTAL][max_char];
	bool _isPrefix[SCE_USER_TOTAL_KEYWORD_GROUPS] = {false};

	bool _isCaseIgnored = false;
	bool _allowFoldOfComments = false;
	int  _forcePureLC = PURE_LC_NONE;
	int _decimalSeparator = DECSEP_DOT;
	bool _foldCompact = false;

	// nakama zone
	friend class Notepad_plus;
	friend class ScintillaEditView;
	friend class NppParameters;

	friend class SharedParametersDialog;
	friend class FolderStyleDialog;
	friend class KeyWordsStyleDialog;
	friend class CommentStyleDialog;
	friend class SymbolsStyleDialog;
	friend class UserDefineDialog;
	friend class StylerDlg;
};

#define MAX_EXTERNAL_LEXER_NAME_LEN 128



class ExternalLangContainer final
{
public:
	// Mandatory for Lexilla
	std::string _name;
	Lexilla::CreateLexerFn fnCL = nullptr;
	//Lexilla::GetLibraryPropertyNamesFn fnGLPN = nullptr;
	//Lexilla::SetLibraryPropertyFn fnSLP = nullptr;

	// For Notepad++
	ExternalLexerAutoIndentMode _autoIndentMode = ExternalLexerAutoIndentMode::Standard;
};


struct FindHistory final
{
	enum searchMode{normal, extended, regExpr};
	enum transparencyMode{none, onLossingFocus, persistant};

	bool _isSearch2ButtonsMode = false;

	int _nbMaxFindHistoryPath    = 10;
	int _nbMaxFindHistoryFilter  = 10;
	int _nbMaxFindHistoryFind    = 10;
	int _nbMaxFindHistoryReplace = 10;

	std::vector<std::wstring> _findHistoryPaths;
	std::vector<std::wstring> _findHistoryFilters;
	std::vector<std::wstring> _findHistoryFinds;
	std::vector<std::wstring> _findHistoryReplaces;

	bool _isMatchWord = false;
	bool _isMatchCase = false;
	bool _isWrap = true;
	bool _isDirectionDown = true;
	bool _dotMatchesNewline = false;

	bool _isFifRecuisive = true;
	bool _isFifInHiddenFolder = false;
    bool _isFifProjectPanel_1 = false;
    bool _isFifProjectPanel_2 = false;
    bool _isFifProjectPanel_3 = false;

	searchMode _searchMode = normal;
	transparencyMode _transparencyMode = onLossingFocus;
	int _transparency = 150;

	bool _isFilterFollowDoc = false;

	bool _isBookmarkLine = false;
	bool _isPurge = false;

	// Allow regExpr backward search: this option is not present in UI, only to modify in config.xml
	bool _regexBackward4PowerUser = false;
};

struct ColumnEditorParam final
{
	enum leadingChoice : UCHAR { noneLeading, zeroLeading, spaceLeading };

	bool _mainChoice = activeNumeric;

	std::wstring _insertedTextContent;

	int _initialNum = -1;
	int _increaseNum = -1;
	int _repeatNum = -1;
	int _formatChoice = 0; // 0:Dec 1:Hex 2:Oct 3:Bin
	leadingChoice _leadingChoice = noneLeading;
};

class LocalizationSwitcher final
{
friend class NppParameters;
public:
	struct LocalizationDefinition
	{
		const wchar_t *_langName = nullptr;
		const wchar_t *_xmlFileName = nullptr;
	};

	bool addLanguageFromXml(const std::wstring& xmlFullPath);
	std::wstring getLangFromXmlFileName(const wchar_t *fn) const;

	std::wstring getXmlFilePathFromLangName(const wchar_t *langName) const;
	bool switchToLang(const wchar_t *lang2switch) const;

	size_t size() const
	{
		return _localizationList.size();
	}

	std::pair<std::wstring, std::wstring> getElementFromIndex(size_t index) const
	{
		if (index >= _localizationList.size())
			return std::pair<std::wstring, std::wstring>(std::wstring(), std::wstring());
		return _localizationList[index];
	}

	void setFileName(const char *fn)
	{
		if (fn)
			_fileName = fn;
	}

	std::string getFileName() const
	{
		return _fileName;
	}

private:
	std::vector< std::pair< std::wstring, std::wstring > > _localizationList;
	std::wstring _nativeLangPath;
	std::string _fileName;
};


class ThemeSwitcher final
{
friend class NppParameters;

public:
	void addThemeFromXml(const std::wstring& xmlFullPath) {
		_themeList.push_back(std::pair<std::wstring, std::wstring>(getThemeFromXmlFileName(xmlFullPath.c_str()), xmlFullPath));
	}

	void addDefaultThemeFromXml(const std::wstring& xmlFullPath) {
		_themeList.push_back(std::pair<std::wstring, std::wstring>(_defaultThemeLabel, xmlFullPath));
	}

	std::wstring getThemeFromXmlFileName(const wchar_t *fn) const;

	std::wstring getXmlFilePathFromThemeName(const wchar_t *themeName) const {
		if (!themeName || themeName[0])
			return std::wstring();
		std::wstring themePath = _stylesXmlPath;
		return themePath;
	}

	bool themeNameExists(const wchar_t *themeName) {
		for (size_t i = 0; i < _themeList.size(); ++i )
		{
			auto& themeNameOnList = getElementFromIndex(i).first;
			if (lstrcmp(themeName, themeNameOnList.c_str()) == 0)
				return true;
		}
		return false;
	}

	size_t size() const { return _themeList.size(); }


	std::pair<std::wstring, std::wstring> & getElementFromIndex(size_t index)
	{
		assert(index < _themeList.size());
		return _themeList[index];
	}

	void setThemeDirPath(const std::wstring& themeDirPath) { _themeDirPath = themeDirPath; }
	std::wstring getThemeDirPath() const { return _themeDirPath; }

	std::wstring getDefaultThemeLabel() const { return _defaultThemeLabel; }

	std::wstring getSavePathFrom(const std::wstring& path) const {
		const auto iter = _themeStylerSavePath.find(path);
		if (iter == _themeStylerSavePath.end())
		{
			return L"";
		}
		else
		{
			return iter->second;
		}
	};

	void addThemeStylerSavePath(const std::wstring& key, const std::wstring& val) {
		_themeStylerSavePath[key] = val;
	};

private:
	std::vector<std::pair<std::wstring, std::wstring>> _themeList;
	std::map<std::wstring, std::wstring> _themeStylerSavePath;
	std::wstring _themeDirPath;
	const std::wstring _defaultThemeLabel = L"Default (stylers.xml)";
	std::wstring _stylesXmlPath;
};

struct HLSColour
{
	WORD _hue = 0;
	WORD _lightness = 0;
	WORD _saturation = 0;

	HLSColour() = default;
	HLSColour(WORD hue, WORD lightness, WORD saturation): _hue(hue), _lightness(lightness), _saturation(saturation) {}
	HLSColour(COLORREF rgb) { ColorRGBToHLS(rgb, &_hue, &_lightness, &_saturation); }

	void loadFromRGB(COLORREF rgb) { ColorRGBToHLS(rgb, &_hue, &_lightness, &_saturation); }
	COLORREF toRGB() const { return ColorHLSToRGB(_hue, _lightness, _saturation); }

	COLORREF toRGB4DarkModeWithTuning(int lightnessMore, int saturationLess) const { 
		return ColorHLSToRGB(_hue, 
			static_cast<WORD>(static_cast<int>(_lightness) + lightnessMore), 
			static_cast<WORD>(static_cast<int>(_saturation) - saturationLess));
	}
	COLORREF toRGB4DarkMod() const { return toRGB4DarkModeWithTuning(50, 20); }
};

struct UdlXmlFileState final {
	TiXmlDocument* _udlXmlDoc = nullptr;
	bool _isDirty = false;
	bool _isInDefaultSharedContainer = false; // contained in "userDefineLang.xml" file
	std::pair<unsigned char, unsigned char> _indexRange;

	UdlXmlFileState(TiXmlDocument* doc, bool isDirty, bool isInDefaultSharedContainer, std::pair<unsigned char, unsigned char> range)
		: _udlXmlDoc(doc), _isDirty(isDirty), _isInDefaultSharedContainer(isInDefaultSharedContainer), _indexRange(range) {};
};

const int NB_LANG = 100;

const int RECENTFILES_SHOWFULLPATH = -1;
const int RECENTFILES_SHOWONLYFILENAME = 0;

class DynamicMenu final
{
public:
	bool attach(HMENU hMenu, unsigned int posBase, int lastCmd, const std::wstring& lastCmdLabel);
	bool createMenu() const;
	bool clearMenu() const;
	int getTopLevelItemNumber() const;
	void push_back(const MenuItemUnit& m) {
		_menuItems.push_back(m);
	};

	MenuItemUnit& getItemFromIndex(size_t i) {
		return _menuItems[i];
	};

	void erase(size_t i) {
		_menuItems.erase(_menuItems.begin() + i);
	}

	unsigned int getPosBase() const { return _posBase; };

	std::wstring getLastCmdLabel() const { return _lastCmdLabel; };

private:
	std::vector<MenuItemUnit> _menuItems;
	HMENU _hMenu = nullptr;
	unsigned int _posBase = 0;
	int _lastCmd = 0;
	std::wstring _lastCmdLabel;
};

class NppParameters final
{
private:
	static NppParameters* getInstancePointer() {
		static NppParameters* instance = new NppParameters;
		return instance;
	};

public:
	static NppParameters& getInstance() {
		return *getInstancePointer();
	};

	static LangType getLangIDFromStr(const wchar_t *langName);
	static std::wstring getLocPathFromStr(const std::wstring & localizationCode);

	bool load();
	bool reloadLang();
	bool reloadStylers(const wchar_t *stylePath = nullptr);
	void destroyInstance();
	std::wstring getSettingsFolder();

	bool _isTaskListRBUTTONUP_Active = false;
	int L_END;

	NppGUI & getNppGUI() {
		return _nppGUI;
	}

	const wchar_t * getWordList(LangType langID, int typeIndex) const
	{
		const Lang* pLang = getLangFromID(langID);
		if (!pLang) return nullptr;

		return pLang->getWords(typeIndex);
	}


	Lang * getLangFromID(LangType langID) const
	{
		for (int i = 0 ; i < _nbLang ; ++i)
		{
			if ( _langList[i] && _langList[i]->_langID == langID )
				return _langList[i];
		}
		return nullptr;
	}

	Lang * getLangFromIndex(size_t i) const {
		return (i < size_t(_nbLang)) ? _langList[i] : nullptr;
	}

	int getNbLang() const {return _nbLang;};

	LangType getLangFromExt(const wchar_t *ext);

	const wchar_t * getLangExtFromName(const wchar_t *langName) const
	{
		for (int i = 0 ; i < _nbLang ; ++i)
		{
			if (_langList[i]->_langName == langName)
				return _langList[i]->_defaultExtList;
		}
		return nullptr;
	}

	const wchar_t * getLangExtFromLangType(LangType langType) const
	{
		for (int i = 0 ; i < _nbLang ; ++i)
		{
			if (_langList[i]->_langID == langType)
				return _langList[i]->_defaultExtList;
		}
		return nullptr;
	}

	int getNbLRFile() const {return _nbRecentFile;};

	std::wstring *getLRFile(int index) const {
		return _LRFileList[index];
	};

	void setNbMaxRecentFile(UINT nb) {
		_nbMaxRecentFile = nb;
	};

	UINT getNbMaxRecentFile() const {return _nbMaxRecentFile;};

	void setPutRecentFileInSubMenu(bool doSubmenu) {
		_putRecentFileInSubMenu = doSubmenu;
	}

	bool putRecentFileInSubMenu() const {return _putRecentFileInSubMenu;};

	void setRecentFileCustomLength(int len) {
		_recentFileCustomLength = len;
	}

	int getRecentFileCustomLength() const {return _recentFileCustomLength;};


	const ScintillaViewParams& getSVP() const {
		return _svp;
	}

	bool writeRecentFileHistorySettings(int nbMaxFile = -1) const;
	bool writeHistory(const wchar_t *fullpath);

	bool writeProjectPanelsSettings() const;
	bool writeColumnEditorSettings() const;
	bool writeFileBrowserSettings(const std::vector<std::wstring> & rootPath, const std::wstring & latestSelectedItemPath) const;

	TiXmlNode* getChildElementByAttribut(TiXmlNode *pere, const wchar_t *childName, const wchar_t *attributName, const wchar_t *attributVal) const;

	bool writeScintillaParams();
	void createXmlTreeFromGUIParams();

	std::wstring writeStyles(LexerStylerArray & lexersStylers, StyleArray & globalStylers); // return "" if saving file succeeds, otherwise return the new saved file path
	bool insertTabInfo(const wchar_t* langName, int tabInfo, bool backspaceUnindent);

	LexerStylerArray & getLStylerArray() {return _lexerStylerVect;};
	StyleArray & getGlobalStylers() {return _widgetStyleArray;};

	StyleArray & getMiscStylerArray() {return _widgetStyleArray;};
	GlobalOverride & getGlobalOverrideStyle() {return _nppGUI._globalOverride;};

	COLORREF getCurLineHilitingColour();
	void setCurLineHilitingColour(COLORREF colour2Set);

	void setFontList(HWND hWnd);
	bool isInFontList(const std::wstring& fontName2Search) const;
	const std::vector<std::wstring>& getFontList() const { return _fontlist; }

	int getNbUserLang() const {return _nbUserLang;}
	UserLangContainer & getULCFromIndex(size_t i) {return *_userLangArray[i];};
	UserLangContainer * getULCFromName(const wchar_t *userLangName);

	int getNbExternalLang() const {return _nbExternalLang;};
	int getExternalLangIndexFromName(const wchar_t *externalLangName) const;

	ExternalLangContainer & getELCFromIndex(int i) {return *_externalLangArray[i];};

	bool ExternalLangHasRoom() const {return _nbExternalLang < NB_MAX_EXTERNAL_LANG;};

	void getExternalLexerFromXmlTree(TiXmlDocument* externalLexerDoc);
	std::vector<TiXmlDocument *> * getExternalLexerDoc() { return &_pXmlExternalLexerDoc; };

	void writeDefaultUDL();
	void writeNonDefaultUDL();
	void writeNeed2SaveUDL();
	void writeShortcuts();
	void writeSession(const Session & session, const wchar_t *fileName = NULL);
	bool writeFindHistory();

	bool isExistingUserLangName(const wchar_t *newName) const
	{
		if ((!newName) || (!newName[0]))
			return true;

		for (int i = 0 ; i < _nbUserLang ; ++i)
		{
			if (!lstrcmp(_userLangArray[i]->_name.c_str(), newName))
				return true;
		}
		return false;
	}

	const wchar_t * getUserDefinedLangNameFromExt(wchar_t *ext, wchar_t *fullName) const;

	int addUserLangToEnd(const UserLangContainer & userLang, const wchar_t *newName);
	void removeUserLang(size_t index);

	bool isExistingExternalLangName(const char* newName) const;

	int addExternalLangToEnd(ExternalLangContainer * externalLang);

	TiXmlDocumentA * getNativeLangA() const {return _pXmlNativeLangDocA;};

	TiXmlDocument * getCustomizedToolIcons() const {return _pXmlToolIconsDoc;};

	bool isTransparentAvailable() const {
		return (_winVersion >= WV_VISTA);
	}

	// 0 <= percent < 256
	// if (percent == 255) then opacq
	void SetTransparent(HWND hwnd, int percent);

	void removeTransparent(HWND hwnd);

	void setCmdlineParam(const CmdLineParamsDTO & cmdLineParams)
	{
		_cmdLineParams = cmdLineParams;
	}

	const CmdLineParamsDTO & getCmdLineParams() const {return _cmdLineParams;};

	const std::wstring& getCmdLineString() const { return _cmdLineString; }
	void setCmdLineString(const std::wstring& str) { _cmdLineString = str; }

	void setFileSaveDlgFilterIndex(int ln) {_fileSaveDlgFilterIndex = ln;};
	int getFileSaveDlgFilterIndex() const {return _fileSaveDlgFilterIndex;};

	bool isRemappingShortcut() const {return _shortcuts.size() != 0;};

	std::vector<CommandShortcut> & getUserShortcuts() { return _shortcuts; };
	void addUserModifiedIndex(size_t index);

	std::vector<MacroShortcut> & getMacroList() { return _macros; };
	std::vector<UserCommand> & getUserCommandList() { return _userCommands; };
	std::vector<PluginCmdShortcut> & getPluginCommandList() { return _pluginCommands; };
	std::vector<size_t> & getPluginModifiedKeyIndices() { return _pluginCustomizedCmds; };
	void addPluginModifiedIndex(size_t index);

	std::vector<ScintillaKeyMap> & getScintillaKeyList() { return _scintillaKeyCommands; };
	std::vector<int> & getScintillaModifiedKeyIndices() { return _scintillaModifiedKeyIndices; };
	void addScintillaModifiedIndex(int index);

	const Session & getSession() const {return _session;};

	std::vector<MenuItemUnit>& getContextMenuItems() { return _contextMenuItems; };
	std::vector<MenuItemUnit>& getTabContextMenuItems() { return _tabContextMenuItems; };
	DynamicMenu& getMacroMenuItems() { return _macroMenuItems; };
	DynamicMenu& getRunMenuItems() { return _runMenuItems; };
	bool hasCustomContextMenu() const {return !_contextMenuItems.empty();};
	bool hasCustomTabContextMenu() const {return !_tabContextMenuItems.empty();};

	void setAccelerator(Accelerator *pAccel) {_pAccelerator = pAccel;};
	Accelerator * getAccelerator() {return _pAccelerator;};
	void setScintillaAccelerator(ScintillaAccelerator *pScintAccel) {_pScintAccelerator = pScintAccel;};
	ScintillaAccelerator * getScintillaAccelerator() {return _pScintAccelerator;};

	std::wstring getNppPath() const {return _nppPath;};
	std::wstring getContextMenuPath() const {return _contextMenuPath;};
	const wchar_t * getAppDataNppDir() const {return _appdataNppDir.c_str();};
	const wchar_t * getPluginRootDir() const { return _pluginRootDir.c_str(); };
	const wchar_t * getPluginConfDir() const { return _pluginConfDir.c_str(); };
	const wchar_t * getUserPluginConfDir() const { return _userPluginConfDir.c_str(); };
	const wchar_t * getWorkingDir() const {return _currentDirectory.c_str();};
	const wchar_t * getWorkSpaceFilePath(int i) const {
		if (i < 0 || i > 2) return nullptr;
		return _workSpaceFilePathes[i].c_str();
	};

	const std::vector<std::wstring> getFileBrowserRoots() const { return _fileBrowserRoot; };
	std::wstring getFileBrowserSelectedItemPath() const { return _fileBrowserSelectedItemPath; };

	void setWorkSpaceFilePath(int i, const wchar_t *wsFile);

	void setWorkingDir(const wchar_t * newPath);

	void setStartWithLocFileName(const std::wstring& locPath) {
		_startWithLocFileName = locPath;
	};

	void setFunctionListExportBoolean(bool doIt) {
		_doFunctionListExport = doIt;
	};
	bool doFunctionListExport() const {
		return _doFunctionListExport;
	};

	void setPrintAndExitBoolean(bool doIt) {
		_doPrintAndExit = doIt;
	};
	bool doPrintAndExit() const {
		return _doPrintAndExit;
	};

	bool loadSession(Session& session, const wchar_t* sessionFileName, const bool bSuppressErrorMsg = false);

	void setLoadedSessionFilePath(const std::wstring & loadedSessionFilePath) {
		_loadedSessionFullFilePath = loadedSessionFilePath;
	};

	std::wstring getLoadedSessionFilePath() {
		return _loadedSessionFullFilePath;
	};

	int langTypeToCommandID(LangType lt) const;

	struct FindDlgTabTitiles final {
		std::wstring _find;
		std::wstring _replace;
		std::wstring _findInFiles;
		std::wstring _findInProjects;
		std::wstring _mark;
	};

	FindDlgTabTitiles & getFindDlgTabTitiles() { return _findDlgTabTitiles;};

	bool asNotepadStyle() const {return _asNotepadStyle;};

	bool reloadPluginCmds() {
		return getPluginCmdsFromXmlTree();
	}

	bool getContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu, bool isEditCM = true);
	bool reloadContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu);
	winVer getWinVersion() const {return _winVersion;};
	std::wstring getWinVersionStr() const;
	std::wstring getWinVerBitStr() const;
	FindHistory & getFindHistory() {return _findHistory;};
	bool _isFindReplacing = false; // an on the fly variable for find/replace functions
#ifndef	_WIN64
	void safeWow64EnableWow64FsRedirection(BOOL Wow64FsEnableRedirection);
#endif

	LocalizationSwitcher & getLocalizationSwitcher() {
		return _localizationSwitcher;
	}

	ThemeSwitcher & getThemeSwitcher() {
		return _themeSwitcher;
	}

	std::vector<std::wstring> & getBlackList() { return _blacklist; };
	bool isInBlackList(const wchar_t* fn) const
	{
		for (const auto& element: _blacklist)
		{
			if (element == fn)
				return true;
		}
		return false;
	}

	bool importUDLFromFile(const std::wstring& sourceFile);
	bool exportUDLToFile(size_t langIndex2export, const std::wstring& fileName2save);
	NativeLangSpeaker* getNativeLangSpeaker() {
		return _pNativeLangSpeaker;
	}
	void setNativeLangSpeaker(NativeLangSpeaker *nls) {
		_pNativeLangSpeaker = nls;
	}

	bool isLocal() const {
		return _isLocal;
	}

	bool isCloud() const {
		return _isCloud;
	}

	void saveConfig_xml();

	std::wstring getUserPath() const {
		return _userPath;
	}

	std::wstring getUserDefineLangFolderPath() const {
		return _userDefineLangsFolderPath;
	}

	std::wstring getUserDefineLangPath() const {
		return _userDefineLangPath;
	}

	bool writeSettingsFilesOnCloudForThe1stTime(const std::wstring & cloudSettingsPath);
	void setCloudChoice(const wchar_t *pathChoice);
	void removeCloudChoice();
	bool isCloudPathChanged() const;
	int archType() const { return ARCH_TYPE; };
	COLORREF getCurrentDefaultBgColor() const {
		return _currentDefaultBgColor;
	}

	COLORREF getCurrentDefaultFgColor() const {
		return _currentDefaultFgColor;
	}

	void setCurrentDefaultBgColor(COLORREF c) {
		_currentDefaultBgColor = c;
	}

	void setCurrentDefaultFgColor(COLORREF c) {
		_currentDefaultFgColor = c;
	}

	void setCmdSettingsDir(const std::wstring& settingsDir) {
		_cmdSettingsDir = settingsDir;
	};

	void setTitleBarAdd(const std::wstring& titleAdd) {
		_titleBarAdditional = titleAdd;
	}

	const std::wstring& getTitleBarAdd() const {
		return _titleBarAdditional;
	}

	DPIManager _dpiManager;

	std::wstring static getSpecialFolderLocation(int folderKind);

	void setUdlXmlDirtyFromIndex(size_t i);
	void setUdlXmlDirtyFromXmlDoc(const TiXmlDocument* xmlDoc);
	void removeIndexFromXmlUdls(size_t i);
	bool isStylerDocLoaded() const { return _pXmlUserStylerDoc != nullptr; };

	ColumnEditorParam _columnEditParam;

private:
	NppParameters();
	~NppParameters();

	// No copy ctor and assignment
	NppParameters(const NppParameters&) = delete;
	NppParameters& operator=(const NppParameters&) = delete;

	// No move ctor and assignment
	NppParameters(NppParameters&&) = delete;
	NppParameters& operator=(NppParameters&&) = delete;


	TiXmlDocument *_pXmlDoc = nullptr; // langs.xml
	TiXmlDocument *_pXmlUserDoc = nullptr; // config.xml
	TiXmlDocument *_pXmlUserStylerDoc = nullptr; // stylers.xml
	TiXmlDocument *_pXmlUserLangDoc = nullptr; // userDefineLang.xml
	std::vector<UdlXmlFileState> _pXmlUserLangsDoc; // userDefineLang customized XMLs
	TiXmlDocument *_pXmlToolIconsDoc = nullptr; // toolbarIcons.xml

	TiXmlDocumentA *_pXmlShortcutDocA = nullptr; // shortcuts.xml

	TiXmlDocumentA *_pXmlNativeLangDocA = nullptr; // nativeLang.xml
	TiXmlDocumentA *_pXmlContextMenuDocA = nullptr; // contextMenu.xml
	TiXmlDocumentA *_pXmlTabContextMenuDocA = nullptr; // tabContextMenu.xml

	std::vector<TiXmlDocument *> _pXmlExternalLexerDoc; // External lexer plugins' XMLs

	NppGUI _nppGUI;
	ScintillaViewParams _svp;
	Lang* _langList[NB_LANG] = { nullptr };
	int _nbLang = 0;

	// Recent File History
	std::wstring* _LRFileList[NB_MAX_LRF_FILE] = { nullptr };
	int _nbRecentFile = 0;
	UINT _nbMaxRecentFile = 10;
	bool _putRecentFileInSubMenu = false;
	int _recentFileCustomLength = RECENTFILES_SHOWFULLPATH;	//	<0: Full File Path Name
															//	=0: Only File Name
															//	>0: Custom Entry Length

	FindHistory _findHistory;

	UserLangContainer* _userLangArray[NB_MAX_USER_LANG] = { nullptr };
	unsigned char _nbUserLang = 0; // won't be exceeded to 255;
	std::wstring _userDefineLangsFolderPath;
	std::wstring _userDefineLangPath;
	ExternalLangContainer* _externalLangArray[NB_MAX_EXTERNAL_LANG] = { nullptr };
	int _nbExternalLang = 0;

	CmdLineParamsDTO _cmdLineParams;
	std::wstring _cmdLineString;

	int _fileSaveDlgFilterIndex = -1;

	// All Styles (colours & fonts)
	LexerStylerArray _lexerStylerVect;
	StyleArray _widgetStyleArray;

	std::vector<std::wstring> _fontlist;
	std::vector<std::wstring> _blacklist;

	bool _isLocal = false;
	bool _isx64 = false; // by default 32-bit
	bool _isCloud = false;

	std::wstring _cmdSettingsDir;
	std::wstring _titleBarAdditional;

	std::wstring _loadedSessionFullFilePath;

	std::array<HLSColour, 5> individualTabHuesFor_Dark{ { HLSColour{37, 60, 60}, HLSColour{70, 60, 60}, HLSColour{144, 70, 60}, HLSColour{255, 60, 60}, HLSColour{195, 60, 60} } };
	std::array<HLSColour, 5> individualTabHues{ { HLSColour{37, 210, 150}, HLSColour{70, 210, 150}, HLSColour{144, 210, 150}, HLSColour{255, 210, 150}, HLSColour{195, 210, 150}} };

	std::array<COLORREF, 3> findDlgStatusMessageColor{ red, blue, darkGreen};

public:
	void setShortcutDirty() { _isAnyShortcutModified = true; };
	void setAdminMode(bool isAdmin) { _isAdminMode = isAdmin; }
	bool isAdmin() const { return _isAdminMode; }
	bool regexBackward4PowerUser() const { return _findHistory._regexBackward4PowerUser; }
	bool isSelectFgColorEnabled() const { return _isSelectFgColorEnabled; };
	bool isRegForOSAppRestartDisabled() const { return _isRegForOSAppRestartDisabled; };

private:
	bool _isAnyShortcutModified = false;
	std::vector<CommandShortcut> _shortcuts;			//main menu shortuts. Static size
	std::vector<size_t> _customizedShortcuts;			//altered main menu shortcuts. Indices static. Needed when saving alterations
	std::vector<MacroShortcut> _macros;				//macro shortcuts, dynamic size, defined on loading macros and adding/deleting them
	std::vector<UserCommand> _userCommands;			//run shortcuts, dynamic size, defined on loading run commands and adding/deleting them
	std::vector<PluginCmdShortcut> _pluginCommands;	//plugin commands, dynamic size, defined on loading plugins
	std::vector<size_t> _pluginCustomizedCmds;			//plugincommands that have been altered. Indices determined after loading ALL plugins. Needed when saving alterations

	std::vector<ScintillaKeyMap> _scintillaKeyCommands;	//scintilla keycommands. Static size
	std::vector<int> _scintillaModifiedKeyIndices;		//modified scintilla keys. Indices static, determined by searching for commandId. Needed when saving alterations

	LocalizationSwitcher _localizationSwitcher;
	std::wstring _startWithLocFileName;
	bool _doFunctionListExport = false;
	bool _doPrintAndExit = false;

	ThemeSwitcher _themeSwitcher;

	std::vector<MenuItemUnit> _contextMenuItems;
	std::vector<MenuItemUnit> _tabContextMenuItems;
	DynamicMenu _macroMenuItems;
	DynamicMenu _runMenuItems;
	Session _session;

	std::wstring _shortcutsPath;
	std::wstring _contextMenuPath;
	std::wstring _tabContextMenuPath;
	std::wstring _sessionPath;
	std::wstring _nppPath;
	std::wstring _userPath;
	std::wstring _stylerPath;
	std::wstring _appdataNppDir; // sentinel of the absence of "doLocalConf.xml" : (_appdataNppDir == L""))?"doLocalConf.xml present":"doLocalConf.xml absent"
	std::wstring _pluginRootDir; // plugins root where all the plugins are installed
	std::wstring _pluginConfDir; // plugins config dir where the plugin list is installed
	std::wstring _userPluginConfDir; // plugins config dir for per user where the plugin parameters are saved / loaded
	std::wstring _currentDirectory;
	std::wstring _workSpaceFilePathes[3];

	std::vector<std::wstring> _fileBrowserRoot;
	std::wstring _fileBrowserSelectedItemPath;

	Accelerator* _pAccelerator = nullptr;
	ScintillaAccelerator* _pScintAccelerator = nullptr;

	FindDlgTabTitiles _findDlgTabTitiles;
	bool _asNotepadStyle = false;

	winVer _winVersion = WV_UNKNOWN;
	Platform _platForm = PF_UNKNOWN;

	NativeLangSpeaker *_pNativeLangSpeaker = nullptr;

	COLORREF _currentDefaultBgColor = RGB(0xFF, 0xFF, 0xFF);
	COLORREF _currentDefaultFgColor = RGB(0x00, 0x00, 0x00);

	std::wstring _initialCloudChoice;

	std::wstring _wingupFullPath;
	std::wstring _wingupParams;
	std::wstring _wingupDir;
	bool _isElevationRequired = false;
	bool _isAdminMode = false;

	bool _isSelectFgColorEnabled = false;
	bool _isRegForOSAppRestartDisabled = false;

	bool _doNppLogNetworkDriveIssue = false;
	bool _doNppLogNulContentCorruptionIssue = false;

	bool _isEndSessionStarted = false;
	bool _isEndSessionCritical = false;

	bool _isPlaceHolderEnabled = false;
	bool _theWarningHasBeenGiven = false;

public:
	std::wstring getWingupFullPath() const { return _wingupFullPath; };
	std::wstring getWingupParams() const { return _wingupParams; };
	std::wstring getWingupDir() const { return _wingupDir; };
	bool shouldDoUAC() const { return _isElevationRequired; };
	void setWingupFullPath(const std::wstring& val2set) { _wingupFullPath = val2set; };
	void setWingupParams(const std::wstring& val2set) { _wingupParams = val2set; };
	void setWingupDir(const std::wstring& val2set) { _wingupDir = val2set; };
	void setElevationRequired(bool val2set) { _isElevationRequired = val2set; };

	bool doNppLogNetworkDriveIssue() const { return _doNppLogNetworkDriveIssue; };
	bool doNppLogNulContentCorruptionIssue() const { return _doNppLogNulContentCorruptionIssue; };
	void endSessionStart() { _isEndSessionStarted = true; };
	bool isEndSessionStarted() const { return _isEndSessionStarted; };
	void makeEndSessionCritical() { _isEndSessionCritical = true; };
	bool isEndSessionCritical() const { return _isEndSessionCritical; };

	void setPlaceHolderEnable(bool isEnabled) { _isPlaceHolderEnabled = isEnabled; };
	bool isPlaceHolderEnabled() const { return _isPlaceHolderEnabled; }
	void setTheWarningHasBeenGiven(bool isEnabled) { _theWarningHasBeenGiven = isEnabled; };
	bool theWarningHasBeenGiven() const { return _theWarningHasBeenGiven; }


	void initTabCustomColors();
	void setIndividualTabColor(COLORREF colour2Set, int colourIndex, bool isDarkMode);
	COLORREF getIndividualTabColor(int colourIndex, bool isDarkMode, bool saturated);

	void initFindDlgStatusMsgCustomColors();
	void setFindDlgStatusMsgIndexColor(COLORREF colour2Set, int colourIndex);
	COLORREF getFindDlgStatusMsgColor(int colourIndex);

private:
	void getLangKeywordsFromXmlTree();
	bool getUserParametersFromXmlTree();
	bool getUserStylersFromXmlTree();
	std::pair<unsigned char, unsigned char> addUserDefineLangsFromXmlTree(TiXmlDocument *tixmldoc);

	bool getShortcutsFromXmlTree();

	bool getMacrosFromXmlTree();
	bool getUserCmdsFromXmlTree();
	bool getPluginCmdsFromXmlTree();
	bool getScintKeysFromXmlTree();
	bool getSessionFromXmlTree(TiXmlDocument *pSessionDoc, Session& session);

	void feedGUIParameters(TiXmlNode *node);
	void feedKeyWordsParameters(TiXmlNode *node);
	void feedFileListParameters(TiXmlNode *node);
	void feedScintillaParam(TiXmlNode *node);
	void feedDockingManager(TiXmlNode *node);
	void duplicateDockingManager(TiXmlNode *dockMngNode, TiXmlElement* dockMngElmt2Clone);
	void feedFindHistoryParameters(TiXmlNode *node);
	void feedProjectPanelsParameters(TiXmlNode *node);
	void feedFileBrowserParameters(TiXmlNode *node);
	void feedColumnEditorParameters(TiXmlNode *node);
	bool feedStylerArray(TiXmlNode *node);
	std::pair<unsigned char, unsigned char> feedUserLang(TiXmlNode *node);
	void feedUserStyles(TiXmlNode *node);
	void feedUserKeywordList(TiXmlNode *node);
	void feedUserSettings(TiXmlNode *node);
	void feedShortcut(TiXmlNodeA *node);
	void feedMacros(TiXmlNodeA *node);
	void feedUserCmds(TiXmlNodeA *node);
	void feedPluginCustomizedCmds(TiXmlNodeA *node);
	void feedScintKeys(TiXmlNodeA *node);

	void getActions(TiXmlNodeA *node, Macro & macro);
	bool getShortcuts(TiXmlNodeA *node, Shortcut & sc, std::string* folderName = nullptr);
	bool getInternalCommandShortcuts(TiXmlNodeA* node, CommandShortcut& cs, std::string* folderName = nullptr);

	void writeStyle2Element(const Style & style2Write, Style & style2Sync, TiXmlElement *element);
	void insertUserLang2Tree(TiXmlNode *node, UserLangContainer *userLang);
	void insertCmd(TiXmlNodeA *cmdRoot, const CommandShortcut & cmd);
	void insertMacro(TiXmlNodeA *macrosRoot, const MacroShortcut & macro, const std::string& folderName);
	void insertUserCmd(TiXmlNodeA *userCmdRoot, const UserCommand & userCmd, const std::string& folderName);
	void insertScintKey(TiXmlNodeA *scintKeyRoot, const ScintillaKeyMap & scintKeyMap);
	void insertPluginCmd(TiXmlNodeA *pluginCmdRoot, const PluginCmdShortcut & pluginCmd);
	TiXmlElement * insertGUIConfigBoolNode(TiXmlNode *r2w, const wchar_t *name, bool bVal);
	void insertDockingParamNode(TiXmlNode *GUIRoot);
	void writeExcludedLangList(TiXmlElement *element);
	void writePrintSetting(TiXmlElement *element);
	void initMenuKeys();		//initialise menu keys and scintilla keys. Other keys are initialized on their own
	void initScintillaKeys();	//these functions have to be called first before any modifications are loaded
	int getCmdIdFromMenuEntryItemName(HMENU mainMenuHadle, const std::wstring& menuEntryName, const std::wstring& menuItemName); // return -1 if not found
	int getPluginCmdIdFromMenuEntryItemName(HMENU pluginsMenu, const std::wstring& pluginName, const std::wstring& pluginCmdName); // return -1 if not found
	winVer getWindowsVersion();

};
