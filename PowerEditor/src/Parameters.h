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
#include "ILexer.h"
#include "Lexilla.h"

#ifdef _WIN64

#ifdef _M_ARM64
#define ARCH_TYPE IMAGE_FILE_MACHINE_ARM64
#else
#define ARCH_TYPE IMAGE_FILE_MACHINE_AMD64
#endif

#else
#define ARCH_TYPE IMAGE_FILE_MACHINE_I386

#endif

#define CMD_INTERPRETER TEXT("%COMSPEC%")

class NativeLangSpeaker;

const bool POS_VERTICAL = true;
const bool POS_HORIZOTAL = false;

const int UDD_SHOW   = 1; // 0000 0001
const int UDD_DOCKED = 2; // 0000 0010

// 0 : 0000 0000 hide & undocked
// 1 : 0000 0001 show & undocked
// 2 : 0000 0010 hide & docked
// 3 : 0000 0011 show & docked

const int TAB_DRAWTOPBAR = 1;      //0000 0000 0001
const int TAB_DRAWINACTIVETAB = 2; //0000 0000 0010
const int TAB_DRAGNDROP = 4;       //0000 0000 0100
const int TAB_REDUCE = 8;          //0000 0000 1000
const int TAB_CLOSEBUTTON = 16;    //0000 0001 0000
const int TAB_DBCLK2CLOSE = 32;    //0000 0010 0000
const int TAB_VERTICAL = 64;       //0000 0100 0000
const int TAB_MULTILINE = 128;     //0000 1000 0000
const int TAB_HIDE = 256;          //0001 0000 0000
const int TAB_QUITONEMPTY = 512;   //0010 0000 0000
const int TAB_ALTICONS = 1024;     //0100 0000 0000


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




enum UniMode {uni8Bit=0, uniUTF8=1, uni16BE=2, uni16LE=3, uniCookie=4, uni7Bit=5, uni16BE_NoBOM=6, uni16LE_NoBOM=7, uniEnd};
enum ChangeDetect { cdDisabled = 0x0, cdEnabledOld = 0x01, cdEnabledNew = 0x02, cdAutoUpdate = 0x04, cdGo2end = 0x08 };
enum BackupFeature {bak_none = 0, bak_simple = 1, bak_verbose = 2};
enum OpenSaveDirSetting {dir_followCurrent = 0, dir_last = 1, dir_userDef = 2};
enum MultiInstSetting {monoInst = 0, multiInstOnSession = 1, multiInst = 2};
enum writeTechnologyEngine {defaultTechnology = 0, directWriteTechnology = 1};
enum urlMode {urlDisable = 0, urlNoUnderLineFg, urlUnderLineFg, urlNoUnderLineBg, urlUnderLineBg,
              urlMin = urlDisable,
              urlMax = urlUnderLineBg};

const int LANG_INDEX_INSTR = 0;
const int LANG_INDEX_INSTR2 = 1;
const int LANG_INDEX_TYPE = 2;
const int LANG_INDEX_TYPE2 = 3;
const int LANG_INDEX_TYPE3 = 4;
const int LANG_INDEX_TYPE4 = 5;
const int LANG_INDEX_TYPE5 = 6;
const int LANG_INDEX_TYPE6 = 7;
const int LANG_INDEX_TYPE7 = 8;

const int COPYDATA_PARAMS = 0;
const int COPYDATA_FILENAMESA = 1;
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

const TCHAR fontSizeStrs[][3] = {TEXT(""), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("11"), TEXT("12"), TEXT("14"), TEXT("16"), TEXT("18"), TEXT("20"), TEXT("22"), TEXT("24"), TEXT("26"), TEXT("28")};

const TCHAR localConfFile[] = TEXT("doLocalConf.xml");
const TCHAR notepadStyleFile[] = TEXT("asNotepad.xml");

// issue xml/log file name
const TCHAR nppLogNetworkDriveIssue[] = TEXT("nppLogNetworkDriveIssue");
const TCHAR nppLogNulContentCorruptionIssue[] = TEXT("nppLogNulContentCorruptionIssue");

void cutString(const TCHAR *str2cut, std::vector<generic_string> & patternVect);
void cutStringBy(const TCHAR *str2cut, std::vector<generic_string> & patternVect, char byChar, bool allowEmptyStr);


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
	sessionFileInfo(const TCHAR *fn, const TCHAR *ln, int encoding, bool userReadOnly, const Position& pos, const TCHAR *backupFilePath, FILETIME originalFileLastModifTimestamp, const MapPosition & mapPos) :
		_isUserReadOnly(userReadOnly), _encoding(encoding), Position(pos), _originalFileLastModifTimestamp(originalFileLastModifTimestamp), _mapPos(mapPos)
	{
		if (fn) _fileName = fn;
		if (ln)	_langName = ln;
		if (backupFilePath) _backupFilePath = backupFilePath;
	}

	sessionFileInfo(generic_string fn) : _fileName(fn) {}

	generic_string _fileName;
	generic_string	_langName;
	std::vector<size_t> _marks;
	std::vector<size_t> _foldStates;
	int	_encoding = -1;
	bool _isUserReadOnly = false;
	bool _isMonitoring = false;

	generic_string _backupFilePath;
	FILETIME _originalFileLastModifTimestamp = {};

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
	generic_string _fileBrowserSelectedItem;
	std::vector<sessionFileInfo> _mainViewFiles;
	std::vector<sessionFileInfo> _subViewFiles;
	std::vector<generic_string> _fileBrowserRoots;
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
	generic_string _localizationPath;
	generic_string _udlName;
	generic_string _pluginMessage;

	generic_string _easterEggName;
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

struct FloatingWindowInfo
{
	int _cont = 0;
	RECT _pos = {};

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
	generic_string _name;
	int _internalID = -1;

	int _currContainer = -1;
	int _prevContainer = -1;
	bool _isVisible = false;

	PluginDlgDockingInfo(const TCHAR* pluginName, int id, int curr, int prev, bool isVis)
		: _internalID(id), _currContainer(curr), _prevContainer(prev), _isVisible(isVis), _name(pluginName)
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


struct DockingManagerData final
{
	int _leftWidth = 200;
	int _rightWidth = 200;
	int _topHeight = 200;
	int _bottomHight = 200;

	std::vector<FloatingWindowInfo> _flaotingWindowInfo;
	std::vector<PluginDlgDockingInfo> _pluginDockInfo;
	std::vector<ContainerTabInfo> _containerTabInfo;

	bool getFloatingRCFrom(int floatCont, RECT& rc) const
	{
		for (size_t i = 0, fwiLen = _flaotingWindowInfo.size(); i < fwiLen; ++i)
		{
			if (_flaotingWindowInfo[i]._cont == floatCont)
			{
				rc.left   = _flaotingWindowInfo[i]._pos.left;
				rc.top	= _flaotingWindowInfo[i]._pos.top;
				rc.right  = _flaotingWindowInfo[i]._pos.right;
				rc.bottom = _flaotingWindowInfo[i]._pos.bottom;
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
	generic_string _styleDesc;

	COLORREF _fgColor = COLORREF(STYLE_NOT_USED);
	COLORREF _bgColor = COLORREF(STYLE_NOT_USED);
	int _colorStyle = COLORSTYLE_ALL;

	bool _isFontEnabled = false;
	generic_string _fontName;
	int _fontStyle = FONTSTYLE_NONE;
	int _fontSize = STYLE_NOT_USED;

	int _nesting = FONTSTYLE_NONE;

	int _keywordClass = STYLE_NOT_USED;
	generic_string _keywords;
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
		assert(index < _styleVect.size());
		return _styleVect[index];
	};

	void addStyler(int styleID, TiXmlNode *styleNode);

	void addStyler(int styleID, const generic_string& styleName) {
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

	Style* findByName(const generic_string& name) {
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

	void setLexerName(const TCHAR *lexerName)
	{
		_lexerName = lexerName;
	}

	void setLexerDesc(const TCHAR *lexerDesc)
	{
		_lexerDesc = lexerDesc;
	}

	void setLexerUserExt(const TCHAR *lexerUserExt) {
		_lexerUserExt = lexerUserExt;
	};

	const TCHAR * getLexerName() const {return _lexerName.c_str();};
	const TCHAR * getLexerDesc() const {return _lexerDesc.c_str();};
	const TCHAR * getLexerUserExt() const {return _lexerUserExt.c_str();};

private :
	generic_string _lexerName;
	generic_string _lexerDesc;
	generic_string _lexerUserExt;
};

struct SortLexersInAlphabeticalOrder {
	bool operator() (LexerStyler& l, LexerStyler& r) {
		if (!lstrcmp(l.getLexerDesc(), TEXT("Search result")))
			return false;
		if (!lstrcmp(r.getLexerDesc(), TEXT("Search result")))
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

	const TCHAR * getLexerNameFromIndex(size_t index) const { return _lexerStylerVect[index].getLexerName(); }
	const TCHAR * getLexerDescFromIndex(size_t index) const { return _lexerStylerVect[index].getLexerDesc(); }

	LexerStyler * getLexerStylerByName(const TCHAR *lexerName) {
		if (!lexerName) return nullptr;
		for (size_t i = 0 ; i < _lexerStylerVect.size() ; ++i)
		{
			if (!lstrcmp(_lexerStylerVect[i].getLexerName(), lexerName))
				return &(_lexerStylerVect[i]);
		}
		return nullptr;
	};

	void addLexerStyler(const TCHAR *lexerName, const TCHAR *lexerDesc, const TCHAR *lexerUserExt, TiXmlNode *lexerNode);

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
};


struct LangMenuItem final
{
	LangType _langType = L_TEXT;
	int	_cmdID = -1;
	generic_string _langName;

	LangMenuItem(LangType lt, int cmdID = 0, const generic_string& langName = TEXT("")):
	_langType(lt), _cmdID(cmdID), _langName(langName){};
};

struct PrintSettings final {
	bool _printLineNumber = true;
	int _printOption = SC_PRINT_COLOURONWHITE;

	generic_string _headerLeft;
	generic_string _headerMiddle;
	generic_string _headerRight;
	generic_string _headerFontName;
	int _headerFontStyle = 0;
	int _headerFontSize = 0;

	generic_string _footerLeft;
	generic_string _footerMiddle;
	generic_string _footerRight;
	generic_string _footerFontName;
	int _footerFontStyle = 0;
	int _footerFontSize = 0;

	RECT _marge = {};

	PrintSettings() {
		_marge.left = 0; _marge.top = 0; _marge.right = 0; _marge.bottom = 0;
	};

	bool isHeaderPresent() const {
		return ((_headerLeft != TEXT("")) || (_headerMiddle != TEXT("")) || (_headerRight != TEXT("")));
	};

	bool isFooterPresent() const {
		return ((_footerLeft != TEXT("")) || (_footerMiddle != TEXT("")) || (_footerRight != TEXT("")));
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

	explicit Date(const TCHAR *dateStr);

	// The constructor which makes the date of number of days from now
	// nbDaysFromNow could be negative if user want to make a date in the past
	// if the value of nbDaysFromNow is 0 then the date will be now
	Date(int nbDaysFromNow);

	void now();

	generic_string toString() const // Return Notepad++ date format : YYYYMMDD
	{
		TCHAR dateStr[16];
		wsprintf(dateStr, TEXT("%04u%02u%02u"), _year, _month, _day);
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
	std::vector<std::pair<char, char>> _matchedPairsInit; // used only on init
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
};

struct NppGUI final
{
	NppGUI()
	{
		_appPos.left = 0;
		_appPos.top = 0;
		_appPos.right = 1100;
		_appPos.bottom = 700;

		_findWindowPos.left = 0;
		_findWindowPos.top = 0;
		_findWindowPos.right = 0;
		_findWindowPos.bottom = 0;

		_defaultDir[0] = 0;
		_defaultDirExp[0] = 0;
	}

	toolBarStatusType _toolBarStatus = TB_STANDARD;
	bool _toolbarShow = true;
	bool _statusBarShow = true;
	bool _menuBarShow = true;

	// 1st bit : draw top bar;
	// 2nd bit : draw inactive tabs
	// 3rd bit : enable drag & drop
	// 4th bit : reduce the height
	// 5th bit : enable vertical
	// 6th bit : enable multiline

	// 0:don't draw; 1:draw top bar 2:draw inactive tabs 3:draw both 7:draw both+drag&drop
	int _tabStatus = (TAB_DRAWTOPBAR | TAB_DRAWINACTIVETAB | TAB_DRAGNDROP | TAB_REDUCE | TAB_CLOSEBUTTON);

	bool _splitterPos = POS_VERTICAL;
	int _userDefineDlgStatus = UDD_DOCKED;

	int _tabSize = 4;
	bool _tabReplacedBySpace = false;

	bool _finderLinesAreCurrentlyWrapped = false;
	bool _finderPurgeBeforeEverySearch = false;
	bool _finderShowOnlyOneEntryPerFoundLine = true;

	int _fileAutoDetection = cdEnabledNew;

	bool _checkHistoryFiles = false;

	RECT _appPos = {};

	RECT _findWindowPos = {};
	bool _findWindowLessMode = false;

	bool _isMaximized = false;
	bool _isMinimizedToTray = false;
	bool _rememberLastSession = true; // remember next session boolean will be written in the settings
	bool _isCmdlineNosessionActivated = false; // used for if -nosession is indicated on the launch time
	bool _detectEncoding = true;
	bool _saveAllConfirm = true;
	bool _setSaveDlgExtFiltToAllTypes = false;
	bool _doTaskList = true;
	bool _maitainIndent = true;
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
	bool _stopFillingFindField = false;
	bool _monospacedFontFindDlg = false;
	bool _findDlgAlwaysVisible = false;
	bool _confirmReplaceInAllOpenDocs = true;
	bool _replaceStopsWithoutFindingNext = false;
	bool _muteSounds = false;
	bool _enableFoldCmdToggable = false;
	bool _hideMenuRightShortcuts = false;
	writeTechnologyEngine _writeTechnologyEngine = defaultTechnology;
	bool _isWordCharDefault = true;
	std::string _customWordChars;
	urlMode _styleURL = urlUnderLineFg;
	generic_string _uriSchemes = TEXT("svn:// cvs:// git:// imap:// irc:// irc6:// ircs:// ldap:// ldaps:// news: telnet:// gopher:// ssh:// sftp:// smb:// skype: snmp:// spotify: steam:// sms: slack:// chrome:// bitcoin:");
	NewDocDefaultSettings _newDocDefaultSettings;

	generic_string _dateTimeFormat = TEXT("yyyy-MM-dd HH:mm:ss");
	bool _dateTimeReverseDefaultOrder = false;

	void setTabReplacedBySpace(bool b) {_tabReplacedBySpace = b;};
	const NewDocDefaultSettings & getNewDocDefaultSettings() const {return _newDocDefaultSettings;};
	std::vector<LangMenuItem> _excludedLangList;
	bool _isLangMenuCompact = true;

	PrintSettings _printSettings;
	BackupFeature _backup = bak_none;
	bool _useDir = false;
	generic_string _backupDir;
	DockingManagerData _dockingData;
	GlobalOverride _globalOverride;
	enum AutocStatus{autoc_none, autoc_func, autoc_word, autoc_both};
	AutocStatus _autocStatus = autoc_both;
	size_t  _autocFromLen = 1;
	bool _autocIgnoreNumbers = true;
	bool _autocInsertSelectedUseENTER = true;
	bool _autocInsertSelectedUseTAB = true;
	bool _funcParams = true;
	MatchedPairConf _matchedPairConf;

	generic_string _definedSessionExt;
	generic_string _definedWorkspaceExt;

	generic_string _commandLineInterpreter = CMD_INTERPRETER;

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
	bool _enableMultiSelection = false;

	bool _shortTitlebar = false;

	OpenSaveDirSetting _openSaveDir = dir_followCurrent;

	TCHAR _defaultDir[MAX_PATH];
	TCHAR _defaultDirExp[MAX_PATH];	//expanded environment variables
	generic_string _themeName;
	MultiInstSetting _multiInstSetting = monoInst;
	bool _fileSwitcherWithoutExtColumn = true;
	int _fileSwitcherExtWidth = 50;
	bool _fileSwitcherWithoutPathColumn = true;
	int _fileSwitcherPathWidth = 50;
	bool isSnapshotMode() const {return _isSnapshotMode && _rememberLastSession && !_isCmdlineNosessionActivated;};
	bool _isSnapshotMode = true;
	size_t _snapshotBackupTiming = 7000;
	generic_string _cloudPath; // this option will never be read/written from/to config.xml
	unsigned char _availableClouds = '\0'; // this option will never be read/written from/to config.xml

	enum SearchEngineChoice{ se_custom = 0, se_duckDuckGo = 1, se_google = 2, se_bing = 3, se_yahoo = 4, se_stackoverflow = 5 };
	SearchEngineChoice _searchEngineChoice = se_google;
	generic_string _searchEngineCustom;

	bool _isFolderDroppedOpenFiles = false;

	bool _isDocPeekOnTab = false;
	bool _isDocPeekOnMap = false;

	// function list should be sorted by default on new file open
	bool _shouldSortFunctionList = false;

	DarkModeConf _darkmode;
	DarkModeConf _darkmodeplugins;
};

struct ScintillaViewParams
{
	bool _lineNumberMarginShow = true;
	bool _lineNumberMarginDynamicWidth = true;
	bool _bookMarkMarginShow = true;
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
};

const int NB_LIST = 20;
const int NB_MAX_LRF_FILE = 30;
const int NB_MAX_USER_LANG = 30;
const int NB_MAX_EXTERNAL_LANG = 30;
const int NB_MAX_IMPORTED_UDL = 50;

const int NB_MAX_FINDHISTORY_FIND	= 30;
const int NB_MAX_FINDHISTORY_REPLACE = 30;
const int NB_MAX_FINDHISTORY_PATH	= 30;
const int NB_MAX_FINDHISTORY_FILTER  = 20;


const int MASK_ReplaceBySpc = 0x80;
const int MASK_TabSize = 0x7F;




struct Lang final
{
	LangType _langID = L_TEXT;
	generic_string _langName;
	const TCHAR* _defaultExtList = nullptr;
	const TCHAR* _langKeyWordList[NB_LIST];
	const TCHAR* _pCommentLineSymbol = nullptr;
	const TCHAR* _pCommentStart = nullptr;
	const TCHAR* _pCommentEnd = nullptr;

	bool _isTabReplacedBySpace = false;
	int _tabSize = -1;

	Lang()
	{
		for (int i = 0 ; i < NB_LIST ; _langKeyWordList[i] = NULL, ++i);
	}

	Lang(LangType langID, const TCHAR *name) : _langID(langID), _langName(name ? name : TEXT(""))
	{
		for (int i = 0 ; i < NB_LIST ; _langKeyWordList[i] = NULL, ++i);
	}

	~Lang() = default;

	void setDefaultExtList(const TCHAR *extLst){
		_defaultExtList = extLst;
	}

	void setCommentLineSymbol(const TCHAR *commentLine){
		_pCommentLineSymbol = commentLine;
	}

	void setCommentStart(const TCHAR *commentStart){
		_pCommentStart = commentStart;
	}

	void setCommentEnd(const TCHAR *commentEnd){
		_pCommentEnd = commentEnd;
	}

	void setTabInfo(int tabInfo)
	{
		if (tabInfo != -1 && tabInfo & MASK_TabSize)
		{
			_isTabReplacedBySpace = (tabInfo & MASK_ReplaceBySpc) != 0;
			_tabSize = tabInfo & MASK_TabSize;
		}
	}

	const TCHAR * getDefaultExtList() const {
		return _defaultExtList;
	}

	void setWords(const TCHAR *words, int index) {
		_langKeyWordList[index] = words;
	}

	const TCHAR * getWords(int index) const {
		return _langKeyWordList[index];
	}

	LangType getLangID() const {return _langID;};
	const TCHAR * getLangName() const {return _langName.c_str();};

	int getTabInfo() const
	{
		if (_tabSize == -1) return -1;
		return (_isTabReplacedBySpace?0x80:0x00) | _tabSize;
	}
};



class UserLangContainer final
{
public:
	UserLangContainer() :_name(TEXT("new user define")), _ext(TEXT("")), _udlVersion(TEXT("")) {
		for (int i = 0; i < SCE_USER_KWLIST_TOTAL; ++i) *_keywordLists[i] = '\0';
	}

	UserLangContainer(const TCHAR *name, const TCHAR *ext, bool isDarkModeTheme, const TCHAR *udlVer):
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

	const TCHAR * getName() {return _name.c_str();};
	const TCHAR * getExtention() {return _ext.c_str();};
	const TCHAR * getUdlVersion() {return _udlVersion.c_str();};

private:
	StyleArray _styles;
	generic_string _name;
	generic_string _ext;
	generic_string _udlVersion;
	bool _isDarkModeTheme = false;

	TCHAR _keywordLists[SCE_USER_KWLIST_TOTAL][max_char];
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

	std::vector<generic_string> _findHistoryPaths;
	std::vector<generic_string> _findHistoryFilters;
	std::vector<generic_string> _findHistoryFinds;
	std::vector<generic_string> _findHistoryReplaces;

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
	bool _isFolderFollowDoc = false;

	// Allow regExpr backward search: this option is not present in UI, only to modify in config.xml
	bool _regexBackward4PowerUser = false;
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
	void addThemeFromXml(const generic_string& xmlFullPath) {
		_themeList.push_back(std::pair<generic_string, generic_string>(getThemeFromXmlFileName(xmlFullPath.c_str()), xmlFullPath));
	}

	void addDefaultThemeFromXml(const generic_string& xmlFullPath) {
		_themeList.push_back(std::pair<generic_string, generic_string>(_defaultThemeLabel, xmlFullPath));
	}

	generic_string getThemeFromXmlFileName(const TCHAR *fn) const;

	generic_string getXmlFilePathFromThemeName(const TCHAR *themeName) const {
		if (!themeName || themeName[0])
			return generic_string();
		generic_string themePath = _stylesXmlPath;
		return themePath;
	}

	bool themeNameExists(const TCHAR *themeName) {
		for (size_t i = 0; i < _themeList.size(); ++i )
		{
			auto themeNameOnList = getElementFromIndex(i).first;
			if (lstrcmp(themeName, themeNameOnList.c_str()) == 0)
				return true;
		}
		return false;
	}

	size_t size() const { return _themeList.size(); }


	std::pair<generic_string, generic_string> & getElementFromIndex(size_t index)
	{
		assert(index < _themeList.size());
		return _themeList[index];
	}

	void setThemeDirPath(generic_string themeDirPath) { _themeDirPath = themeDirPath; }
	generic_string getThemeDirPath() const { return _themeDirPath; }

	generic_string getDefaultThemeLabel() const { return _defaultThemeLabel; }

	generic_string getSavePathFrom(const generic_string& path) const {
		const auto iter = _themeStylerSavePath.find(path);
		if (iter == _themeStylerSavePath.end())
		{
			return TEXT("");
		}
		else
		{
			return iter->second;
		}
	};

	void addThemeStylerSavePath(generic_string key, generic_string val) {
		_themeStylerSavePath[key] = val;
	};

private:
	std::vector<std::pair<generic_string, generic_string>> _themeList;
	std::map<generic_string, generic_string> _themeStylerSavePath;
	generic_string _themeDirPath;
	const generic_string _defaultThemeLabel = TEXT("Default (stylers.xml)");
	generic_string _stylesXmlPath;
};


struct UdlXmlFileState final {
	TiXmlDocument* _udlXmlDoc = nullptr;
	bool _isDirty = false;
	std::pair<unsigned char, unsigned char> _indexRange;

	UdlXmlFileState(TiXmlDocument* doc, bool isDirty, std::pair<unsigned char, unsigned char> range) : _udlXmlDoc(doc), _isDirty(isDirty), _indexRange(range) {};
};

const int NB_LANG = 100;

const int RECENTFILES_SHOWFULLPATH = -1;
const int RECENTFILES_SHOWONLYFILENAME = 0;

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

	static LangType getLangIDFromStr(const TCHAR *langName);
	static generic_string getLocPathFromStr(const generic_string & localizationCode);

	bool load();
	bool reloadLang();
	bool reloadStylers(const TCHAR *stylePath = nullptr);
	void destroyInstance();
	generic_string getSettingsFolder();

	bool _isTaskListRBUTTONUP_Active = false;
	int L_END;

	NppGUI & getNppGUI() {
		return _nppGUI;
	}

	const TCHAR * getWordList(LangType langID, int typeIndex) const
	{
		Lang *pLang = getLangFromID(langID);
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

	LangType getLangFromExt(const TCHAR *ext);

	const TCHAR * getLangExtFromName(const TCHAR *langName) const
	{
		for (int i = 0 ; i < _nbLang ; ++i)
		{
			if (_langList[i]->_langName == langName)
				return _langList[i]->_defaultExtList;
		}
		return nullptr;
	}

	const TCHAR * getLangExtFromLangType(LangType langType) const
	{
		for (int i = 0 ; i < _nbLang ; ++i)
		{
			if (_langList[i]->_langID == langType)
				return _langList[i]->_defaultExtList;
		}
		return nullptr;
	}

	int getNbLRFile() const {return _nbRecentFile;};

	generic_string *getLRFile(int index) const {
		return _LRFileList[index];
	};

	void setNbMaxRecentFile(int nb) {
		_nbMaxRecentFile = nb;
	};

	int getNbMaxRecentFile() const {return _nbMaxRecentFile;};

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
	bool writeHistory(const TCHAR *fullpath);

	bool writeProjectPanelsSettings() const;
	bool writeFileBrowserSettings(const std::vector<generic_string> & rootPath, const generic_string & latestSelectedItemPath) const;

	TiXmlNode* getChildElementByAttribut(TiXmlNode *pere, const TCHAR *childName, const TCHAR *attributName, const TCHAR *attributVal) const;

	bool writeScintillaParams();
	void createXmlTreeFromGUIParams();

	generic_string writeStyles(LexerStylerArray & lexersStylers, StyleArray & globalStylers); // return "" if saving file succeeds, otherwise return the new saved file path
	bool insertTabInfo(const TCHAR *langName, int tabInfo);

	LexerStylerArray & getLStylerArray() {return _lexerStylerVect;};
	StyleArray & getGlobalStylers() {return _widgetStyleArray;};

	StyleArray & getMiscStylerArray() {return _widgetStyleArray;};
	GlobalOverride & getGlobalOverrideStyle() {return _nppGUI._globalOverride;};

	COLORREF getCurLineHilitingColour();
	void setCurLineHilitingColour(COLORREF colour2Set);

	void setFontList(HWND hWnd);
	bool isInFontList(const generic_string& fontName2Search) const;
	const std::vector<generic_string>& getFontList() const { return _fontlist; }

	HFONT getDefaultUIFont();

	int getNbUserLang() const {return _nbUserLang;}
	UserLangContainer & getULCFromIndex(size_t i) {return *_userLangArray[i];};
	UserLangContainer * getULCFromName(const TCHAR *userLangName);

	int getNbExternalLang() const {return _nbExternalLang;};
	int getExternalLangIndexFromName(const TCHAR *externalLangName) const;

	ExternalLangContainer & getELCFromIndex(int i) {return *_externalLangArray[i];};

	bool ExternalLangHasRoom() const {return _nbExternalLang < NB_MAX_EXTERNAL_LANG;};

	void getExternalLexerFromXmlTree(TiXmlDocument* externalLexerDoc);
	std::vector<TiXmlDocument *> * getExternalLexerDoc() { return &_pXmlExternalLexerDoc; };

	void writeDefaultUDL();
	void writeNonDefaultUDL();
	void writeNeed2SaveUDL();
	void writeShortcuts();
	void writeSession(const Session & session, const TCHAR *fileName = NULL);
	bool writeFindHistory();

	bool isExistingUserLangName(const TCHAR *newName) const
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

	const TCHAR * getUserDefinedLangNameFromExt(TCHAR *ext, TCHAR *fullName) const;

	int addUserLangToEnd(const UserLangContainer & userLang, const TCHAR *newName);
	void removeUserLang(size_t index);

	bool isExistingExternalLangName(const char* newName) const;

	int addExternalLangToEnd(ExternalLangContainer * externalLang);

	TiXmlDocumentA * getNativeLangA() const {return _pXmlNativeLangDocA;};

	TiXmlDocument * getCustomizedToolIcons() const {return _pXmlToolIconsDoc;};

	bool isTransparentAvailable() const {
		return (_transparentFuncAddr != NULL);
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

	const generic_string& getCmdLineString() const { return _cmdLineString; }
	void setCmdLineString(const generic_string& str) { _cmdLineString = str; }

	void setFileSaveDlgFilterIndex(int ln) {_fileSaveDlgFilterIndex = ln;};
	int getFileSaveDlgFilterIndex() const {return _fileSaveDlgFilterIndex;};

	bool isRemappingShortcut() const {return _shortcuts.size() != 0;};

	std::vector<CommandShortcut> & getUserShortcuts() { return _shortcuts; };
	std::vector<size_t> & getUserModifiedShortcuts() { return _customizedShortcuts; };
	void addUserModifiedIndex(size_t index);

	std::vector<MacroShortcut> & getMacroList() { return _macros; };
	std::vector<UserCommand> & getUserCommandList() { return _userCommands; };
	std::vector<PluginCmdShortcut> & getPluginCommandList() { return _pluginCommands; };
	std::vector<size_t> & getPluginModifiedKeyIndices() { return _pluginCustomizedCmds; };
	void addPluginModifiedIndex(size_t index);

	std::vector<ScintillaKeyMap> & getScintillaKeyList() { return _scintillaKeyCommands; };
	std::vector<int> & getScintillaModifiedKeyIndices() { return _scintillaModifiedKeyIndices; };
	void addScintillaModifiedIndex(int index);

	std::vector<MenuItemUnit> & getContextMenuItems() { return _contextMenuItems; };
	const Session & getSession() const {return _session;};

	bool hasCustomContextMenu() const {return !_contextMenuItems.empty();};

	void setAccelerator(Accelerator *pAccel) {_pAccelerator = pAccel;};
	Accelerator * getAccelerator() {return _pAccelerator;};
	void setScintillaAccelerator(ScintillaAccelerator *pScintAccel) {_pScintAccelerator = pScintAccel;};
	ScintillaAccelerator * getScintillaAccelerator() {return _pScintAccelerator;};

	generic_string getNppPath() const {return _nppPath;};
	generic_string getContextMenuPath() const {return _contextMenuPath;};
	const TCHAR * getAppDataNppDir() const {return _appdataNppDir.c_str();};
	const TCHAR * getPluginRootDir() const { return _pluginRootDir.c_str(); };
	const TCHAR * getPluginConfDir() const { return _pluginConfDir.c_str(); };
	const TCHAR * getUserPluginConfDir() const { return _userPluginConfDir.c_str(); };
	const TCHAR * getWorkingDir() const {return _currentDirectory.c_str();};
	const TCHAR * getWorkSpaceFilePath(int i) const {
		if (i < 0 || i > 2) return nullptr;
		return _workSpaceFilePathes[i].c_str();
	};

	const std::vector<generic_string> getFileBrowserRoots() const { return _fileBrowserRoot; };
	generic_string getFileBrowserSelectedItemPath() const { return _fileBrowserSelectedItemPath; };

	void setWorkSpaceFilePath(int i, const TCHAR *wsFile);

	void setWorkingDir(const TCHAR * newPath);

	void setStartWithLocFileName(const generic_string& locPath) {
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

	bool loadSession(Session & session, const TCHAR *sessionFileName);

	void setLoadedSessionFilePath(const generic_string & loadedSessionFilePath) {
		_loadedSessionFullFilePath = loadedSessionFilePath;
	};

	generic_string getLoadedSessionFilePath() {
		return _loadedSessionFullFilePath;
	};

	int langTypeToCommandID(LangType lt) const;
	WNDPROC getEnableThemeDlgTexture() const {return _enableThemeDialogTextureFuncAddr;};

	struct FindDlgTabTitiles final {
		generic_string _find;
		generic_string _replace;
		generic_string _findInFiles;
		generic_string _findInProjects;
		generic_string _mark;
	};

	FindDlgTabTitiles & getFindDlgTabTitiles() { return _findDlgTabTitiles;};

	bool asNotepadStyle() const {return _asNotepadStyle;};

	bool reloadPluginCmds() {
		return getPluginCmdsFromXmlTree();
	}

	bool getContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu);
	bool reloadContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu);
	winVer getWinVersion() const {return _winVersion;};
	generic_string getWinVersionStr() const;
	generic_string getWinVerBitStr() const;
	FindHistory & getFindHistory() {return _findHistory;};
	bool _isFindReplacing = false; // an on the fly variable for find/replace functions
	void safeWow64EnableWow64FsRedirection(BOOL Wow64FsEnableRedirection);

	LocalizationSwitcher & getLocalizationSwitcher() {
		return _localizationSwitcher;
	}

	ThemeSwitcher & getThemeSwitcher() {
		return _themeSwitcher;
	}

	std::vector<generic_string> & getBlackList() { return _blacklist; };
	bool isInBlackList(TCHAR *fn) const
	{
		for (auto& element: _blacklist)
		{
			if (element == fn)
				return true;
		}
		return false;
	}

	bool importUDLFromFile(const generic_string& sourceFile);
	bool exportUDLToFile(size_t langIndex2export, const generic_string& fileName2save);
	NativeLangSpeaker* getNativeLangSpeaker() {
		return _pNativeLangSpeaker;
	}
	void setNativeLangSpeaker(NativeLangSpeaker *nls) {
		_pNativeLangSpeaker = nls;
	}

	bool isLocal() const {
		return _isLocal;
	};

	void saveConfig_xml();

	generic_string getUserPath() const {
		return _userPath;
	}

	generic_string getUserDefineLangFolderPath() const {
		return _userDefineLangsFolderPath;
	}

	generic_string getUserDefineLangPath() const {
		return _userDefineLangPath;
	}

	bool writeSettingsFilesOnCloudForThe1stTime(const generic_string & cloudSettingsPath);
	void setCloudChoice(const TCHAR *pathChoice);
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

	void setCmdSettingsDir(const generic_string& settingsDir) {
		_cmdSettingsDir = settingsDir;
	};

	void setTitleBarAdd(const generic_string& titleAdd)
	{
		_titleBarAdditional = titleAdd;
	}

	const generic_string& getTitleBarAdd() const
	{
		return _titleBarAdditional;
	}

	DPIManager _dpiManager;

	generic_string static getSpecialFolderLocation(int folderKind);

	void setUdlXmlDirtyFromIndex(size_t i);
	void setUdlXmlDirtyFromXmlDoc(const TiXmlDocument* xmlDoc);
	void removeIndexFromXmlUdls(size_t i);
	bool isStylerDocLoaded() const { return _pXmlUserStylerDoc != nullptr; };

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
	TiXmlDocument *_pXmlShortcutDoc = nullptr; // shortcuts.xml
	TiXmlDocument *_pXmlBlacklistDoc = nullptr; // not implemented

	TiXmlDocumentA *_pXmlNativeLangDocA = nullptr; // nativeLang.xml
	TiXmlDocumentA *_pXmlContextMenuDocA = nullptr; // contextMenu.xml

	std::vector<TiXmlDocument *> _pXmlExternalLexerDoc; // External lexer plugins' XMLs

	NppGUI _nppGUI;
	ScintillaViewParams _svp;
	Lang* _langList[NB_LANG] = { nullptr };
	int _nbLang = 0;

	// Recent File History
	generic_string* _LRFileList[NB_MAX_LRF_FILE] = { nullptr };
	int _nbRecentFile = 0;
	int _nbMaxRecentFile = 10;
	bool _putRecentFileInSubMenu = false;
	int _recentFileCustomLength = RECENTFILES_SHOWFULLPATH;	//	<0: Full File Path Name
															//	=0: Only File Name
															//	>0: Custom Entry Length

	FindHistory _findHistory;

	UserLangContainer* _userLangArray[NB_MAX_USER_LANG] = { nullptr };
	unsigned char _nbUserLang = 0; // won't be exceeded to 255;
	generic_string _userDefineLangsFolderPath;
	generic_string _userDefineLangPath;
	ExternalLangContainer* _externalLangArray[NB_MAX_EXTERNAL_LANG] = { nullptr };
	int _nbExternalLang = 0;

	CmdLineParamsDTO _cmdLineParams;
	generic_string _cmdLineString;

	int _fileSaveDlgFilterIndex = -1;

	// All Styles (colours & fonts)
	LexerStylerArray _lexerStylerVect;
	StyleArray _widgetStyleArray;

	std::vector<generic_string> _fontlist;
	std::vector<generic_string> _blacklist;

	HMODULE _hUXTheme = nullptr;

	WNDPROC _transparentFuncAddr = nullptr;
	WNDPROC _enableThemeDialogTextureFuncAddr = nullptr;
	bool _isLocal = false;
	bool _isx64 = false; // by default 32-bit

	generic_string _cmdSettingsDir;
	generic_string _titleBarAdditional;

	generic_string _loadedSessionFullFilePath;

public:
	void setShortcutDirty() { _isAnyShortcutModified = true; };
	void setAdminMode(bool isAdmin) { _isAdminMode = isAdmin; }
	bool isAdmin() const { return _isAdminMode; }
	bool regexBackward4PowerUser() const { return _findHistory._regexBackward4PowerUser; }
	bool isSelectFgColorEnabled() const { return _isSelectFgColorEnabled; };

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
	generic_string _startWithLocFileName;
	bool _doFunctionListExport = false;
	bool _doPrintAndExit = false;

	ThemeSwitcher _themeSwitcher;

	//vector<generic_string> _noMenuCmdNames;
	std::vector<MenuItemUnit> _contextMenuItems;
	Session _session;

	generic_string _shortcutsPath;
	generic_string _contextMenuPath;
	generic_string _sessionPath;
	generic_string _nppPath;
	generic_string _userPath;
	generic_string _stylerPath;
	generic_string _appdataNppDir; // sentinel of the absence of "doLocalConf.xml" : (_appdataNppDir == TEXT(""))?"doLocalConf.xml present":"doLocalConf.xml absent"
	generic_string _pluginRootDir; // plugins root where all the plugins are installed
	generic_string _pluginConfDir; // plugins config dir where the plugin list is installed
	generic_string _userPluginConfDir; // plugins config dir for per user where the plugin parameters are saved / loaded
	generic_string _currentDirectory;
	generic_string _workSpaceFilePathes[3];

	std::vector<generic_string> _fileBrowserRoot;
	generic_string _fileBrowserSelectedItemPath;

	Accelerator* _pAccelerator = nullptr;
	ScintillaAccelerator* _pScintAccelerator = nullptr;

	FindDlgTabTitiles _findDlgTabTitiles;
	bool _asNotepadStyle = false;

	winVer _winVersion = WV_UNKNOWN;
	Platform _platForm = PF_UNKNOWN;

	NativeLangSpeaker *_pNativeLangSpeaker = nullptr;

	COLORREF _currentDefaultBgColor = RGB(0xFF, 0xFF, 0xFF);
	COLORREF _currentDefaultFgColor = RGB(0x00, 0x00, 0x00);

	generic_string _initialCloudChoice;

	generic_string _wingupFullPath;
	generic_string _wingupParams;
	generic_string _wingupDir;
	bool _isElevationRequired = false;
	bool _isAdminMode = false;

	bool _isSelectFgColorEnabled = false;

	bool _doNppLogNetworkDriveIssue = false;

	bool _doNppLogNulContentCorruptionIssue = false;
	bool _isQueryEndSessionStarted = false;

public:
	generic_string getWingupFullPath() const { return _wingupFullPath; };
	generic_string getWingupParams() const { return _wingupParams; };
	generic_string getWingupDir() const { return _wingupDir; };
	bool shouldDoUAC() const { return _isElevationRequired; };
	void setWingupFullPath(const generic_string& val2set) { _wingupFullPath = val2set; };
	void setWingupParams(const generic_string& val2set) { _wingupParams = val2set; };
	void setWingupDir(const generic_string& val2set) { _wingupDir = val2set; };
	void setElevationRequired(bool val2set) { _isElevationRequired = val2set; };

	bool doNppLogNetworkDriveIssue() { return _doNppLogNetworkDriveIssue; };
	bool doNppLogNulContentCorruptionIssue() { return _doNppLogNulContentCorruptionIssue; };
	void queryEndSessionStart() { _isQueryEndSessionStarted = true; };
	bool isQueryEndSessionStarted() { return _isQueryEndSessionStarted; };

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
	bool getBlackListFromXmlTree();

	void feedGUIParameters(TiXmlNode *node);
	void feedKeyWordsParameters(TiXmlNode *node);
	void feedFileListParameters(TiXmlNode *node);
	void feedScintillaParam(TiXmlNode *node);
	void feedDockingManager(TiXmlNode *node);
	void duplicateDockingManager(TiXmlNode *dockMngNode, TiXmlElement* dockMngElmt2Clone);
	void feedFindHistoryParameters(TiXmlNode *node);
	void feedProjectPanelsParameters(TiXmlNode *node);
	void feedFileBrowserParameters(TiXmlNode *node);
	bool feedStylerArray(TiXmlNode *node);
	std::pair<unsigned char, unsigned char> feedUserLang(TiXmlNode *node);
	void feedUserStyles(TiXmlNode *node);
	void feedUserKeywordList(TiXmlNode *node);
	void feedUserSettings(TiXmlNode *node);
	void feedShortcut(TiXmlNode *node);
	void feedMacros(TiXmlNode *node);
	void feedUserCmds(TiXmlNode *node);
	void feedPluginCustomizedCmds(TiXmlNode *node);
	void feedScintKeys(TiXmlNode *node);
	bool feedBlacklist(TiXmlNode *node);

	void getActions(TiXmlNode *node, Macro & macro);
	bool getShortcuts(TiXmlNode *node, Shortcut & sc);

	void writeStyle2Element(const Style & style2Write, Style & style2Sync, TiXmlElement *element);
	void insertUserLang2Tree(TiXmlNode *node, UserLangContainer *userLang);
	void insertCmd(TiXmlNode *cmdRoot, const CommandShortcut & cmd);
	void insertMacro(TiXmlNode *macrosRoot, const MacroShortcut & macro);
	void insertUserCmd(TiXmlNode *userCmdRoot, const UserCommand & userCmd);
	void insertScintKey(TiXmlNode *scintKeyRoot, const ScintillaKeyMap & scintKeyMap);
	void insertPluginCmd(TiXmlNode *pluginCmdRoot, const PluginCmdShortcut & pluginCmd);
	TiXmlElement * insertGUIConfigBoolNode(TiXmlNode *r2w, const TCHAR *name, bool bVal);
	void insertDockingParamNode(TiXmlNode *GUIRoot);
	void writeExcludedLangList(TiXmlElement *element);
	void writePrintSetting(TiXmlElement *element);
	void initMenuKeys();		//initialise menu keys and scintilla keys. Other keys are initialized on their own
	void initScintillaKeys();	//these functions have to be called first before any modifications are loaded
	int getCmdIdFromMenuEntryItemName(HMENU mainMenuHadle, const generic_string& menuEntryName, const generic_string& menuItemName); // return -1 if not found
	int getPluginCmdIdFromMenuEntryItemName(HMENU pluginsMenu, const generic_string& pluginName, const generic_string& pluginCmdName); // return -1 if not found
	winVer getWindowsVersion();

};
