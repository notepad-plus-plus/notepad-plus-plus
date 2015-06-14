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


#ifndef PARAMETERS_H
#define PARAMETERS_H

#ifndef TINYXMLA_INCLUDED
#include "tinyxmlA.h"
#endif //TINYXMLA_INCLUDED

#ifndef TINYXML_INCLUDED
#include "tinyxml.h"
#endif //TINYXML_INCLUDED

#ifndef SCINTILLA_H
#include "Scintilla.h"
#endif //SCINTILLA_H

#ifndef SCINTILLA_REF_H
#include "ScintillaRef.h"
#endif //SCINTILLA_REF_H

#ifndef TOOL_BAR_H
#include "ToolBar.h"
#endif //TOOL_BAR_H

#ifndef USER_DEFINE_LANG_REFERENCE_H
#include "UserDefineLangReference.h"
#endif //USER_DEFINE_LANG_REFERENCE_H

#ifndef COLORS_H
#include "colors.h"
#endif //COLORS_H

#ifndef SHORTCUTS_H
#include "shortcut.h"
#endif //SHORTCUTS_H

#ifndef CONTEXTMENU_H
#include "ContextMenu.h"
#endif //CONTEXTMENU_H

#ifndef DPIMANAGER_H
#include "dpiManager.h"
#endif //DPIMANAGER_H

#include <assert.h>
#include <tchar.h>

class NativeLangSpeaker;

const bool POS_VERTICAL = true;
const bool POS_HORIZOTAL = false;

const int UDD_SHOW   = 1; // 0000 0001 
const int UDD_DOCKED = 2; // 0000 0010

// 0 : 0000 0000 hide & undocked
// 1 : 0000 0001 show & undocked
// 2 : 0000 0010 hide & docked
// 3 : 0000 0011 show & docked

const int TAB_DRAWTOPBAR = 1;      //  0000 0001
const int TAB_DRAWINACTIVETAB = 2; //  0000 0010
const int TAB_DRAGNDROP = 4;       //  0000 0100
const int TAB_REDUCE = 8;	       //  0000 1000
const int TAB_CLOSEBUTTON = 16;    //  0001 0000
const int TAB_DBCLK2CLOSE = 32;    //  0010 0000
const int TAB_VERTICAL = 64;       //  0100 0000
const int TAB_MULTILINE = 128;     //  1000 0000
const int TAB_HIDE = 256;          //1 0000 0000

enum formatType {WIN_FORMAT, MAC_FORMAT, UNIX_FORMAT};
enum UniMode {uni8Bit=0, uniUTF8=1, uni16BE=2, uni16LE=3, uniCookie=4, uni7Bit=5, uni16BE_NoBOM=6, uni16LE_NoBOM=7, uniEnd};
enum ChangeDetect {cdDisabled=0, cdEnabled=1, cdAutoUpdate=2, cdGo2end=3, cdAutoUpdateGo2end=4};
enum BackupFeature {bak_none = 0, bak_simple = 1, bak_verbose = 2};
enum OpenSaveDirSetting {dir_followCurrent = 0, dir_last = 1, dir_userDef = 2};
enum MultiInstSetting {monoInst = 0, multiInstOnSession = 1, multiInst = 2};
enum CloudChoice {noCloud = 0, dropbox = 1, oneDrive = 2, googleDrive = 3};

const int LANG_INDEX_INSTR = 0;
const int LANG_INDEX_INSTR2 = 1;
const int LANG_INDEX_TYPE = 2;
const int LANG_INDEX_TYPE2 = 3;
const int LANG_INDEX_TYPE3 = 4;
const int LANG_INDEX_TYPE4 = 5;
const int LANG_INDEX_TYPE5 = 6;

const int COPYDATA_PARAMS = 0;
const int COPYDATA_FILENAMESA = 1;
const int COPYDATA_FILENAMESW = 2;

#define PURE_LC_NONE    0
#define PURE_LC_BOL     1
#define PURE_LC_WSP     2

#define DECSEP_DOT      0
#define DECSEP_COMMA    1
#define DECSEP_BOTH     2


#define DROPBOX_AVAILABLE 1
#define ONEDRIVE_AVAILABLE 2
#define GOOGLEDRIVE_AVAILABLE 4

const TCHAR fontSizeStrs[][3] = {TEXT(""), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("11"), TEXT("12"), TEXT("14"), TEXT("16"), TEXT("18"), TEXT("20"), TEXT("22"), TEXT("24"), TEXT("26"), TEXT("28")};

const TCHAR localConfFile[] = TEXT("doLocalConf.xml");
const TCHAR allowAppDataPluginsFile[] = TEXT("allowAppDataPlugins.xml");
const TCHAR notepadStyleFile[] = TEXT("asNotepad.xml");

void cutString(const TCHAR *str2cut, std::vector<generic_string> & patternVect);


struct Position
{ 
	int _firstVisibleLine;
	int _startPos;
	int _endPos;
	int _xOffset;
	int _selMode;
	int _scrollWidth;
	Position() : _firstVisibleLine(0), _startPos(0), _endPos(0), _xOffset(0), _scrollWidth(1), _selMode(0) {};
};

struct sessionFileInfo : public Position {
	sessionFileInfo(const TCHAR *fn, const TCHAR *ln, int encoding, Position pos, const TCHAR *backupFilePath, int originalFileLastModifTimestamp) : 
		_encoding(encoding), Position(pos), _originalFileLastModifTimestamp(originalFileLastModifTimestamp) {
		if (fn) _fileName = fn;
		if (ln)	_langName = ln;
		if (backupFilePath) _backupFilePath = backupFilePath;
	};

	sessionFileInfo(generic_string fn) : _fileName(fn), _encoding(-1){};
	
	generic_string _fileName;
	generic_string	_langName;
	std::vector<size_t> _marks;
	std::vector<size_t> _foldStates;
	int	_encoding;

	generic_string _backupFilePath;
	time_t _originalFileLastModifTimestamp;
};

struct Session {
	size_t nbMainFiles() const {return _mainViewFiles.size();};
	size_t nbSubFiles() const {return _subViewFiles.size();};
	size_t _activeView;
	size_t _activeMainIndex;
	size_t _activeSubIndex;
	std::vector<sessionFileInfo> _mainViewFiles;
	std::vector<sessionFileInfo> _subViewFiles;
};

struct CmdLineParams {
	bool _isNoPlugin;
	bool _isReadOnly;
	bool _isNoSession;
	bool _isNoTab;
	bool _isPreLaunch;
	bool _showLoadingTime;
	bool _alwaysOnTop;
	int _line2go;
    int _column2go;

    POINT _point;
	bool _isPointXValid;
	bool _isPointYValid;
	bool isPointValid() {
		return _isPointXValid && _isPointYValid;
	};
	bool _isSessionFile;
	bool _isRecursive;

	LangType _langType;
	generic_string _localizationPath;
	generic_string _easterEggName;
	unsigned char _quoteType;

	CmdLineParams() : _isNoPlugin(false), _isReadOnly(false), _isNoSession(false), _isNoTab(false),_showLoadingTime(false),\
        _isPreLaunch(false), _line2go(-1), _column2go(-1), _langType(L_EXTERNAL), _isPointXValid(false), _isPointYValid(false),\
		_alwaysOnTop(false), _localizationPath(TEXT("")), _easterEggName(TEXT("")), _quoteType(0)
    {
        _point.x = 0;
        _point.y = 0;
    }
};

struct FloatingWindowInfo {
	int _cont;
	RECT _pos;
	FloatingWindowInfo(int cont, int x, int y, int w, int h) : _cont(cont) {
		_pos.left	= x;
		_pos.top	= y;
		_pos.right	= w;
		_pos.bottom = h;
	};
};

struct PluginDlgDockingInfo {
	generic_string _name;
	int _internalID;

	int _currContainer;
	int _prevContainer;
	bool _isVisible;

	PluginDlgDockingInfo(const TCHAR *pluginName, int id, int curr, int prev, bool isVis) : _internalID(id), _currContainer(curr), _prevContainer(prev), _isVisible(isVis), _name(pluginName){};

	friend inline const bool operator==(const PluginDlgDockingInfo & a, const PluginDlgDockingInfo & b) {
		if ((a._name == b._name) && (a._internalID == b._internalID))
			return true;
		else
			return false;
	};
};

struct ContainerTabInfo {
	int _cont;
	int _activeTab;

	ContainerTabInfo(int cont, int activeTab) : _cont(cont), _activeTab(activeTab) {};
};

struct DockingManagerData {
	int _leftWidth;
	int _rightWidth;
	int _topHeight;
	int _bottomHight;

	DockingManagerData() : _leftWidth(200), _rightWidth(200), _topHeight(200), _bottomHight(200) {};

	std::vector<FloatingWindowInfo>		_flaotingWindowInfo;
	std::vector<PluginDlgDockingInfo>	_pluginDockInfo;
	std::vector<ContainerTabInfo>		_containerTabInfo;

	bool getFloatingRCFrom(int floatCont, RECT & rc) {
		for (size_t i = 0, fwiLen = _flaotingWindowInfo.size(); i < fwiLen; ++i)
		{
			if (_flaotingWindowInfo[i]._cont == floatCont)
      {
        rc.left = _flaotingWindowInfo[i]._pos.left;
        rc.top = _flaotingWindowInfo[i]._pos.top;
        rc.right = _flaotingWindowInfo[i]._pos.right;
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

struct Style
{
	int _styleID;
    const TCHAR *_styleDesc;

	COLORREF _fgColor;
	COLORREF _bgColor;
	int _colorStyle;
	const TCHAR *_fontName;
	int _fontStyle;
	int _fontSize;
	int _nesting;

	int _keywordClass;
	generic_string *_keywords;

	Style():_styleID(-1), _styleDesc(NULL), _fgColor(COLORREF(STYLE_NOT_USED)), _bgColor(COLORREF(STYLE_NOT_USED)), _colorStyle(COLORSTYLE_ALL),\
        _fontName(NULL), _fontStyle(FONTSTYLE_NONE), _fontSize(STYLE_NOT_USED), _keywordClass(STYLE_NOT_USED), _keywords(NULL), _nesting(FONTSTYLE_NONE){};

	~Style(){
		if (_keywords) 
			delete _keywords;
	};

	Style(const Style & style) 
	{
		_styleID = style._styleID;
		_styleDesc = style._styleDesc;
		_fgColor = style._fgColor;
		_bgColor = style._bgColor;
		_colorStyle = style._colorStyle;
		_fontName = style._fontName;
		_fontSize = style._fontSize;
		_fontStyle = style._fontStyle;
		_keywordClass = style._keywordClass;
		_nesting = style._nesting;
		if (style._keywords)
			_keywords = new generic_string(*(style._keywords));
		else
			_keywords = NULL;
	};

	Style & operator=(const Style & style) {
		if (this != &style)
		{
			this->_styleID = style._styleID;
			this->_styleDesc = style._styleDesc;
			this->_fgColor = style._fgColor;
			this->_bgColor = style._bgColor;
			this->_colorStyle = style._colorStyle;
			this->_fontName = style._fontName;
			this->_fontSize = style._fontSize;
			this->_fontStyle = style._fontStyle;
			this->_keywordClass = style._keywordClass;
			this->_nesting = style._nesting;

			if (!(this->_keywords) && style._keywords)
				this->_keywords = new generic_string(*(style._keywords));
			else if (this->_keywords && style._keywords)
				this->_keywords->assign(*(style._keywords));
			else if (this->_keywords && !(style._keywords))
			{
				delete (this->_keywords);
				this->_keywords = NULL;
			}
		}
		return *this;
	};

	void setKeywords(const TCHAR *str) {
		if (!_keywords)
			_keywords = new generic_string(str);
		else
			*_keywords = str;
	};
};

struct GlobalOverride
{
	bool isEnable() const {return (enableFg || enableBg || enableFont || enableFontSize || enableBold || enableItalic || enableUnderLine);};
	bool enableFg;
	bool enableBg;
	bool enableFont;
	bool enableFontSize;
	bool enableBold;
	bool enableItalic;
	bool enableUnderLine;
	GlobalOverride():enableFg(false), enableBg(false), enableFont(false), enableFontSize(false), enableBold(false), enableItalic(false), enableUnderLine(false) {};
};

struct StyleArray
{
public:
    StyleArray() : _nbStyler(0){};

    StyleArray & operator=(const StyleArray & sa)
    {
        if (this != &sa)
        {
            this->_nbStyler = sa._nbStyler;
            for (int i = 0 ; i < _nbStyler ; ++i)
            {
                this->_styleArray[i] = sa._styleArray[i];
            }
        }
        return *this;
    }

    int getNbStyler() const {return _nbStyler;};
	void setNbStyler(int nb) {_nbStyler = nb;};

    Style & getStyler(int index) {
		assert(index >= 0 && index < SCE_STYLE_ARRAY_SIZE);
		return _styleArray[index];
	};

    bool hasEnoughSpace() {return (_nbStyler < SCE_STYLE_ARRAY_SIZE);};
    void addStyler(int styleID, TiXmlNode *styleNode);

	void addStyler(int styleID, const TCHAR *styleName) {
		_styleArray[styleID]._styleID = styleID;
		_styleArray[styleID]._styleDesc = styleName;
		_styleArray[styleID]._fgColor = black;
		_styleArray[styleID]._bgColor = white;
		++_nbStyler;
	};

    int getStylerIndexByID(int id) {
        for (int i = 0 ; i < _nbStyler ; ++i)
            if (_styleArray[i]._styleID == id)
                return i;
        return -1;
    };

    int getStylerIndexByName(const TCHAR *name) const {
		if (!name)
			return -1;
        for (int i = 0 ; i < _nbStyler ; ++i)
			if (!lstrcmp(_styleArray[i]._styleDesc, name))
                return i;
        return -1;
    };

protected:
	Style _styleArray[SCE_STYLE_ARRAY_SIZE];
	int _nbStyler;
};

struct LexerStyler : public StyleArray
{
public :
    LexerStyler():StyleArray(){};

    LexerStyler & operator=(const LexerStyler & ls)
    {
        if (this != &ls)
        {
            *((StyleArray *)this) = ls;
            this->_lexerName = ls._lexerName;
			this->_lexerDesc = ls._lexerDesc;
			this->_lexerUserExt = ls._lexerUserExt;
        }
        return *this;
    }

    void setLexerName(const TCHAR *lexerName) {
        _lexerName = lexerName;
    };
	
	void setLexerDesc(const TCHAR *lexerDesc) {
        _lexerDesc = lexerDesc;
    };

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

const int MAX_LEXER_STYLE = 80;

struct LexerStylerArray
{
public :
	LexerStylerArray() : _nbLexerStyler(0){};

    LexerStylerArray & operator=(const LexerStylerArray & lsa)
    {
        if (this != &lsa)
        {
            this->_nbLexerStyler = lsa._nbLexerStyler;
            for (int i = 0 ; i < this->_nbLexerStyler ; ++i)
                this->_lexerStylerArray[i] = lsa._lexerStylerArray[i];
        }
        return *this;
    }

    int getNbLexer() const {return _nbLexerStyler;};

    LexerStyler & getLexerFromIndex(int index)
    {
        return _lexerStylerArray[index];
    };

    const TCHAR * getLexerNameFromIndex(int index) const {return _lexerStylerArray[index].getLexerName();}
	const TCHAR * getLexerDescFromIndex(int index) const {return _lexerStylerArray[index].getLexerDesc();}

    LexerStyler * getLexerStylerByName(const TCHAR *lexerName) {
		if (!lexerName) return NULL;
        for (int i = 0 ; i < _nbLexerStyler ; ++i)
        {
            if (!lstrcmp(_lexerStylerArray[i].getLexerName(), lexerName))
                return &(_lexerStylerArray[i]);
        }
        return NULL;
    };
    bool hasEnoughSpace() {return (_nbLexerStyler < MAX_LEXER_STYLE);};
    void addLexerStyler(const TCHAR *lexerName, const TCHAR *lexerDesc, const TCHAR *lexerUserExt, TiXmlNode *lexerNode);
	void eraseAll();
private :
	LexerStyler _lexerStylerArray[MAX_LEXER_STYLE];
	int _nbLexerStyler;
};

struct NewDocDefaultSettings 
{
	formatType _format;
	UniMode _unicodeMode;
	bool _openAnsiAsUtf8;
	LangType _lang;
	int _codepage; // -1 when not using
	NewDocDefaultSettings():_format(WIN_FORMAT), _unicodeMode(uniCookie), _openAnsiAsUtf8(true), _lang(L_TEXT), _codepage(-1){};
};

struct LangMenuItem {
	LangType _langType;
	int	_cmdID;
	generic_string _langName;

	LangMenuItem(LangType lt, int cmdID = 0, generic_string langName = TEXT("")):
	_langType(lt), _cmdID(cmdID), _langName(langName){};
};

struct PrintSettings {
	bool _printLineNumber;
	int _printOption;
	
	generic_string _headerLeft;
	generic_string _headerMiddle;
	generic_string _headerRight;
	generic_string _headerFontName;
	int _headerFontStyle;
	int _headerFontSize;
	
	generic_string _footerLeft;
	generic_string _footerMiddle;
	generic_string _footerRight;
	generic_string _footerFontName;
	int _footerFontStyle;
	int _footerFontSize;

	RECT _marge;

	PrintSettings() : _printLineNumber(true), _printOption(SC_PRINT_NORMAL), _headerLeft(TEXT("")), _headerMiddle(TEXT("")), _headerRight(TEXT("")),\
		_headerFontName(TEXT("")), _headerFontStyle(0), _headerFontSize(0),  _footerLeft(TEXT("")), _footerMiddle(TEXT("")), _footerRight(TEXT("")),\
		_footerFontName(TEXT("")), _footerFontStyle(0), _footerFontSize(0) {
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

class Date {
public:
    Date() : _year(2008), _month(4), _day(26){};
    Date(unsigned long year, unsigned long month, unsigned long day) {
        assert(year > 0 && year <= 9999); // I don't think Notepad++ will last till AD 10000 :)
        assert(month > 0 && month <= 12);
        assert(day > 0 && day <= 31);
        assert(!(month == 2 && day > 29) &&
               !(month == 4 && day > 30) &&
               !(month == 6 && day > 30) &&
               !(month == 9 && day > 30) &&
               !(month == 11 && day > 30));

        _year = year;
        _month = month;
        _day = day;
    };
    
    Date(const TCHAR *dateStr);

    // The constructor which makes the date of number of days from now
    // nbDaysFromNow could be negative if user want to make a date in the past
    // if the value of nbDaysFromNow is 0 then the date will be now
	Date(int nbDaysFromNow);

    void now();

    generic_string toString() { // Return Notepad++ date format : YYYYMMDD
        TCHAR dateStr[8+1];
        wsprintf(dateStr, TEXT("%04u%02u%02u"), _year, _month, _day);
        return dateStr;
    };

    bool operator<(const Date & compare) const {
        if (this->_year != compare._year)
            return (this->_year < compare._year);
        if (this->_month != compare._month)
            return (this->_month < compare._month);
        return (this->_day < compare._day);
    };
    bool operator>(const Date & compare) const {
        if (this->_year != compare._year)
            return (this->_year > compare._year);
        if (this->_month != compare._month)
            return (this->_month > compare._month);
        return (this->_day > compare._day);
    };
    bool operator==(const Date & compare) const {
        if (this->_year != compare._year)
            return false;
        if (this->_month != compare._month)
            return false;
        return (this->_day == compare._day);
    };
    bool operator!=(const Date & compare) const {
        if (this->_year != compare._year)
            return true;
        if (this->_month != compare._month)
            return true;
        return (this->_day != compare._day);
    };

private:
    unsigned long _year;
    unsigned long _month;
    unsigned long _day;
};

struct MatchedPairConf {
	std::vector< std::pair<char, char> > _matchedPairs;
	std::vector< std::pair<char, char> > _matchedPairsInit; // used only on init
	bool _doHtmlXmlTag;
	bool _doParentheses;
	bool _doBrackets;
	bool _doCurlyBrackets;
	bool _doQuotes;
	bool _doDoubleQuotes;

	MatchedPairConf(): _doHtmlXmlTag(false), _doParentheses(false), _doBrackets(false), _doCurlyBrackets(false),\
		_doQuotes(false), _doDoubleQuotes(false) {};

	bool hasUserDefinedPairs() const { return _matchedPairs.size() != 0; };
	bool hasDefaultPairs() const { return _doParentheses||_doBrackets||_doCurlyBrackets||_doQuotes||_doDoubleQuotes||_doHtmlXmlTag; };
	bool hasAnyPairsPair() const { return hasUserDefinedPairs() || hasDefaultPairs(); };
};

struct NppGUI
{
	NppGUI() : _toolBarStatus(TB_LARGE), _toolbarShow(true), _statusBarShow(true), _menuBarShow(true),\
		       _tabStatus(TAB_DRAWTOPBAR | TAB_DRAWINACTIVETAB | TAB_DRAGNDROP), _splitterPos(POS_HORIZOTAL),\
	           _userDefineDlgStatus(UDD_DOCKED), _tabSize(8), _tabReplacedBySpace(false), _fileAutoDetection(cdEnabled), _fileAutoDetectionOriginalValue(_fileAutoDetection),\
			   _checkHistoryFiles(true) ,_enableSmartHilite(true), _disableSmartHiliteTmp(false), _enableTagsMatchHilite(true), _enableTagAttrsHilite(true), _enableHiliteNonHTMLZone(false),\
			   _isMaximized(false), _isMinimizedToTray(false), _rememberLastSession(true), _isCmdlineNosessionActivated(false), _detectEncoding(true), _backup(bak_none), _useDir(false), _backupDir(TEXT("")),\
			   _doTaskList(true), _maitainIndent(true), _openSaveDir(dir_followCurrent), _styleMRU(true), _styleURL(0),\
			   _autocStatus(autoc_both), _autocFromLen(1), _funcParams(false), _definedSessionExt(TEXT("")), _cloudChoice(noCloud), _availableClouds(0),\
			   _doesExistUpdater(false), _caretBlinkRate(250), _caretWidth(1), _enableMultiSelection(false), _shortTitlebar(false), _themeName(TEXT("")), _isLangMenuCompact(false),\
			   _smartHiliteCaseSensitive(false), _leftmostDelimiter('('), _rightmostDelimiter(')'), _delimiterSelectionOnEntireDocument(false), _multiInstSetting(monoInst),\
			   _fileSwitcherWithoutExtColumn(false), _isSnapshotMode(true), _snapshotBackupTiming(7000), _backSlashIsEscapeCharacterForSql(true) {
		_appPos.left = 0;
		_appPos.top = 0;
		_appPos.right = 700;
		_appPos.bottom = 500;

		_defaultDir[0] = 0;
		_defaultDirExp[0] = 0;
	};
	toolBarStatusType _toolBarStatus;		// small, large ou standard
	bool _toolbarShow;
	bool _statusBarShow;		// show ou hide
	bool _menuBarShow;

	// 1st bit : draw top bar; 
	// 2nd bit : draw inactive tabs
	// 3rd bit : enable drag & drop
	// 4th bit : reduce the height
	// 5th bit : enable vertical
	// 6th bit : enable multiline

	// 0:don't draw; 1:draw top bar 2:draw inactive tabs 3:draw both 7:draw both+drag&drop
	int _tabStatus;

	bool _splitterPos;			// horizontal ou vertical
	int _userDefineDlgStatus;	// (hide||show) && (docked||undocked)

	int _tabSize;
	bool _tabReplacedBySpace;

	ChangeDetect _fileAutoDetection;
	ChangeDetect _fileAutoDetectionOriginalValue;
	bool _checkHistoryFiles;

	RECT _appPos;

	bool _isMaximized;
	bool _isMinimizedToTray;
	bool _rememberLastSession;	// remember next session boolean will be written in the settings
	bool _isCmdlineNosessionActivated; // used for if -nosession is indicated on the launch time
	bool _detectEncoding;
	bool _doTaskList;
	bool _maitainIndent;
	bool _enableSmartHilite;
	bool _smartHiliteCaseSensitive;
	bool _disableSmartHiliteTmp;
	bool _enableTagsMatchHilite;
	bool _enableTagAttrsHilite;
	bool _enableHiliteNonHTMLZone;
	bool _styleMRU;
	char _leftmostDelimiter, _rightmostDelimiter;
	bool _delimiterSelectionOnEntireDocument;
	bool _backSlashIsEscapeCharacterForSql;


	// 0 : do nothing
	// 1 : don't draw underline
	// 2 : draw underline
	int _styleURL;

	NewDocDefaultSettings _newDocDefaultSettings;
	void setTabReplacedBySpace(bool b) {_tabReplacedBySpace = b;};
	const NewDocDefaultSettings & getNewDocDefaultSettings() const {return _newDocDefaultSettings;};
	std::vector<LangMenuItem> _excludedLangList;
	bool _isLangMenuCompact;

	PrintSettings _printSettings;
	BackupFeature _backup;
	bool _useDir;
	generic_string _backupDir;
	DockingManagerData _dockingData;
	GlobalOverride _globalOverride;
	enum AutocStatus{autoc_none, autoc_func, autoc_word, autoc_both};
	AutocStatus _autocStatus;
	size_t  _autocFromLen;
	bool _funcParams;
	MatchedPairConf _matchedPairConf;

	generic_string _definedSessionExt;
	

    
    struct AutoUpdateOptions {
        bool _doAutoUpdate;
        int _intervalDays;
        Date _nextUpdateDate;
        AutoUpdateOptions(): _doAutoUpdate(true), _intervalDays(15), _nextUpdateDate(Date()) {};
    } _autoUpdateOpt;

	bool _doesExistUpdater;
	int _caretBlinkRate;
	int _caretWidth;
    bool _enableMultiSelection;

	bool _shortTitlebar;

	OpenSaveDirSetting _openSaveDir;
	
	TCHAR _defaultDir[MAX_PATH];
	TCHAR _defaultDirExp[MAX_PATH];	//expanded environment variables
	generic_string _themeName;
	MultiInstSetting _multiInstSetting;
	bool _fileSwitcherWithoutExtColumn;
	bool isSnapshotMode() const {return _isSnapshotMode && _rememberLastSession && !_isCmdlineNosessionActivated;};
	bool _isSnapshotMode;
	size_t _snapshotBackupTiming;
	CloudChoice _cloudChoice; // this option will never be read/written from/to config.xml
	unsigned char _availableClouds; // this option will never be read/written from/to config.xml
};

struct ScintillaViewParams
{
	ScintillaViewParams() : _lineNumberMarginShow(true), _bookMarkMarginShow(true),_borderWidth(2),\
		                    _folderStyle(FOLDER_STYLE_BOX), _foldMarginShow(true), _indentGuideLineShow(true),\
	                        _currentLineHilitingShow(true), _wrapSymbolShow(false),  _doWrap(false), _edgeNbColumn(80),\
							_zoom(0), _zoom2(0), _whiteSpaceShow(false), _eolShow(false), _lineWrapMethod(LINEWRAP_ALIGNED),\
							_disableAdvancedScrolling(false){};
	bool _lineNumberMarginShow;
	bool _bookMarkMarginShow;
	//bool _docChangeStateMarginShow;
	folderStyle  _folderStyle; //"simple", "arrow", "circle", "box" and "none"
	lineWrapMethod _lineWrapMethod;
	bool _foldMarginShow;
	bool _indentGuideLineShow;
	bool _currentLineHilitingShow;
	bool _wrapSymbolShow;
	bool _doWrap;
	int _edgeMode;
	int _edgeNbColumn;
	int _zoom;
	int _zoom2;
	bool _whiteSpaceShow;
	bool _eolShow;
    int _borderWidth;
	bool _disableAdvancedScrolling;
};

const int NB_LIST = 20;
const int NB_MAX_LRF_FILE = 30;
const int NB_MAX_USER_LANG = 30;
const int NB_MAX_EXTERNAL_LANG = 30;
const int NB_MAX_IMPORTED_UDL = 50;

const int NB_MAX_FINDHISTORY_FIND    = 30;
const int NB_MAX_FINDHISTORY_REPLACE = 30;
const int NB_MAX_FINDHISTORY_PATH    = 30;
const int NB_MAX_FINDHISTORY_FILTER  = 20;


const int MASK_ReplaceBySpc = 0x80;
const int MASK_TabSize = 0x7F;

struct Lang
{
	LangType _langID;
	generic_string _langName;
	const TCHAR *_defaultExtList;
	const TCHAR *_langKeyWordList[NB_LIST];
	const TCHAR *_pCommentLineSymbol;
	const TCHAR *_pCommentStart;
	const TCHAR *_pCommentEnd;

    bool _isTabReplacedBySpace;
    int _tabSize;

    Lang(): _langID(L_TEXT), _langName(TEXT("")), _defaultExtList(NULL), _pCommentLineSymbol(NULL), _pCommentStart(NULL),
            _pCommentEnd(NULL), _isTabReplacedBySpace(false), _tabSize(-1) {
        for (int i = 0 ; i < NB_LIST ; _langKeyWordList[i] = NULL, ++i);
    };
	Lang(LangType langID, const TCHAR *name) : _langID(langID), _langName(name?name:TEXT("")),\
                                               _defaultExtList(NULL), _pCommentLineSymbol(NULL), _pCommentStart(NULL),\
                                               _pCommentEnd(NULL), _isTabReplacedBySpace(false), _tabSize(-1) {
		for (int i = 0 ; i < NB_LIST ; _langKeyWordList[i] = NULL, ++i);
	};
	~Lang() {};
	void setDefaultExtList(const TCHAR *extLst){
		_defaultExtList = extLst;
	};
	
	void setCommentLineSymbol(const TCHAR *commentLine){
		_pCommentLineSymbol = commentLine;
	};
	
	void setCommentStart(const TCHAR *commentStart){
		_pCommentStart = commentStart;
	};

	void setCommentEnd(const TCHAR *commentEnd){
		_pCommentEnd = commentEnd;
	};

    void setTabInfo(int tabInfo) {
        if (tabInfo != -1 && tabInfo & MASK_TabSize)
        {
            _isTabReplacedBySpace = (tabInfo & MASK_ReplaceBySpc) != 0; 
            _tabSize = tabInfo & MASK_TabSize;
        }
    };

	const TCHAR * getDefaultExtList() const {
		return _defaultExtList;
	};
	
	void setWords(const TCHAR *words, int index) {
		_langKeyWordList[index] = words;
	};

	const TCHAR * getWords(int index) const {
		return _langKeyWordList[index];
	};

	LangType getLangID() const {return _langID;};
	const TCHAR * getLangName() const {return _langName.c_str();};

    int getTabInfo() const {
        if (_tabSize == -1) return -1;
        return (_isTabReplacedBySpace?0x80:0x00) | _tabSize;
    };
};

class UserLangContainer
{
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

public :
	UserLangContainer(){
		_name = TEXT("new user define");
		_ext = TEXT("");
		_udlVersion = TEXT("");
        _allowFoldOfComments = false;
		_forcePureLC = PURE_LC_NONE;
        _decimalSeparator = DECSEP_DOT;
		_foldCompact = false;
        _isCaseIgnored = false;

		for (int i = 0 ; i < SCE_USER_KWLIST_TOTAL ; ++i)
			*_keywordLists[i] = '\0';

		for (int i = 0 ; i < SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
            _isPrefix[i] = false;
	};
	UserLangContainer(const TCHAR *name, const TCHAR *ext, const TCHAR *udlVer) : _name(name), _ext(ext), _udlVersion(udlVer) {
        _allowFoldOfComments = false;
		_forcePureLC = PURE_LC_NONE;
        _decimalSeparator = DECSEP_DOT;
		_foldCompact = false;

		for (int i = 0 ; i < SCE_USER_KWLIST_TOTAL ; ++i)
			*_keywordLists[i] = '\0';

		for (int i = 0 ; i < SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
            _isPrefix[i] = false;
	};

	UserLangContainer & operator=(const UserLangContainer & ulc) {
		if (this != &ulc)
        {
			this->_name = ulc._name;
			this->_ext = ulc._ext;
			this->_udlVersion = ulc._udlVersion;
			this->_isCaseIgnored = ulc._isCaseIgnored;
			this->_styleArray = ulc._styleArray;
			this->_allowFoldOfComments = ulc._allowFoldOfComments;
			this->_forcePureLC = ulc._forcePureLC;
			this->_decimalSeparator = ulc._decimalSeparator;
			this->_foldCompact = ulc._foldCompact;
			int nbStyler = this->_styleArray.getNbStyler();
			for (int i = 0 ; i < nbStyler ; ++i)
			{
				Style & st = this->_styleArray.getStyler(i);
				if (st._bgColor == COLORREF(-1))
					st._bgColor = white;
				if (st._fgColor == COLORREF(-1))
					st._fgColor = black;
			}
			for (int i = 0 ; i < SCE_USER_KWLIST_TOTAL ; ++i)
				lstrcpy(this->_keywordLists[i], ulc._keywordLists[i]);

			for (int i = 0 ; i < SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
                _isPrefix[i] = ulc._isPrefix[i];
		}
		return *this;
	};

	// int getNbKeywordList() {return SCE_USER_KWLIST_TOTAL;};
	const TCHAR * getName() {return _name.c_str();};
	const TCHAR * getExtention() {return _ext.c_str();};
	const TCHAR * getUdlVersion() {return _udlVersion.c_str();};

private:
	StyleArray _styleArray;
	generic_string _name;
	generic_string _ext;
	generic_string _udlVersion;

	//TCHAR _keywordLists[nbKeywodList][max_char];
	TCHAR _keywordLists[SCE_USER_KWLIST_TOTAL][max_char];
	bool _isPrefix[SCE_USER_TOTAL_KEYWORD_GROUPS];

	bool _isCaseIgnored;
	bool _allowFoldOfComments;
	int  _forcePureLC;
    int _decimalSeparator;
	bool _foldCompact;
};

#define MAX_EXTERNAL_LEXER_NAME_LEN 16
#define MAX_EXTERNAL_LEXER_DESC_LEN 32

class ExternalLangContainer
{
public:
	TCHAR _name[MAX_EXTERNAL_LEXER_NAME_LEN];
	TCHAR _desc[MAX_EXTERNAL_LEXER_DESC_LEN];

	ExternalLangContainer(const TCHAR *name, const TCHAR *desc) {
		generic_strncpy(_name, name, MAX_EXTERNAL_LEXER_NAME_LEN);
		generic_strncpy(_desc, desc, MAX_EXTERNAL_LEXER_DESC_LEN);
	};
};

struct FindHistory {
	enum searchMode{normal, extended, regExpr};
	enum transparencyMode{none, onLossingFocus, persistant};

	FindHistory() : _nbMaxFindHistoryPath(10), _nbMaxFindHistoryFilter(10), _nbMaxFindHistoryFind(10), _nbMaxFindHistoryReplace(10),\
					_isMatchWord(false), _isMatchCase(false),_isWrap(true),_isDirectionDown(true),\
					_isFifRecuisive(true), _isFifInHiddenFolder(false), _isDlgAlwaysVisible(false),\
					_isFilterFollowDoc(false), _isFolderFollowDoc(false),\
					_searchMode(normal), _transparencyMode(onLossingFocus), _transparency(150),
					_dotMatchesNewline(false)
					
	{};
	int _nbMaxFindHistoryPath;
	int _nbMaxFindHistoryFilter;
	int _nbMaxFindHistoryFind;
	int _nbMaxFindHistoryReplace;

    std::vector<generic_string> _findHistoryPaths;
	std::vector<generic_string> _findHistoryFilters;
	std::vector<generic_string> _findHistoryFinds;
	std::vector<generic_string> _findHistoryReplaces;

	bool _isMatchWord;
	bool _isMatchCase;
	bool _isWrap;
	bool _isDirectionDown;
	bool _dotMatchesNewline;

	bool _isFifRecuisive;
	bool _isFifInHiddenFolder;
	
	searchMode _searchMode;
	transparencyMode _transparencyMode;
	int _transparency;

	bool _isDlgAlwaysVisible;
	bool _isFilterFollowDoc;
	bool _isFolderFollowDoc;
};

class LocalizationSwitcher {
friend class NppParameters;
public :
    LocalizationSwitcher() : _fileName("") {};

	struct LocalizationDefinition {
		wchar_t *_langName;
		wchar_t *_xmlFileName;
	};

	bool addLanguageFromXml(std::wstring xmlFullPath);
	std::wstring getLangFromXmlFileName(const wchar_t *fn) const;

	std::wstring getXmlFilePathFromLangName(const wchar_t *langName) const;
	bool switchToLang(wchar_t *lang2switch) const;

	size_t size() const {
		return _localizationList.size();
	};

	std::pair<std::wstring, std::wstring> getElementFromIndex(size_t index) {
		if (index >= _localizationList.size())
			return std::pair<std::wstring, std::wstring>(TEXT(""), TEXT(""));
		return _localizationList[index];
	};

    void setFileName(const char *fn) {
        if (fn)
            _fileName = fn;
    };

	std::string getFileName() const {
        return _fileName;
    };

private :
	std::vector< std::pair< std::wstring, std::wstring > > _localizationList;
	std::wstring _nativeLangPath;
	std::string _fileName;
};

class ThemeSwitcher {
friend class NppParameters;

public :
	ThemeSwitcher(){};

	void addThemeFromXml(generic_string xmlFullPath) {
		_themeList.push_back(std::pair<generic_string, generic_string>(getThemeFromXmlFileName(xmlFullPath.c_str()), xmlFullPath));
	};

	void addDefaultThemeFromXml(generic_string xmlFullPath) {
		_themeList.push_back(std::pair<generic_string, generic_string>(TEXT("Default (stylers.xml)"), xmlFullPath));
	};

	generic_string getThemeFromXmlFileName(const TCHAR *fn) const;

	generic_string getXmlFilePathFromThemeName(const TCHAR *themeName) const {
		if (!themeName || themeName[0])
			return TEXT("");
		generic_string themePath = _stylesXmlPath;
		return themePath;
	};

	bool themeNameExists(const TCHAR *themeName) {
		for (size_t i = 0; i < _themeList.size(); ++i )
		{
			if (! (getElementFromIndex(i)).first.compare(themeName) ) return true;
		}
		return false;
	}

	size_t size() const {
		return _themeList.size();
	};

	
	std::pair<generic_string, generic_string> & getElementFromIndex(size_t index) {
		//if (index >= _themeList.size())
			//return pair<generic_string, generic_string>(TEXT(""), TEXT(""));
		return _themeList[index];
	};

private :
	std::vector< std::pair< generic_string, generic_string > > _themeList;
	generic_string _stylesXmlPath;
};

class PluginList {
public :
    void add(generic_string fn, bool isInBL){
		_list.push_back(std::pair<generic_string, bool>(fn, isInBL));
    };
private :
	std::vector<std::pair<generic_string, bool>>_list;
};

const int NB_LANG = 80;
const bool DUP = true;
const bool FREE = false;

const int RECENTFILES_SHOWFULLPATH = -1;
const int RECENTFILES_SHOWONLYFILENAME = 0;

class NppParameters 
{
public:
    static NppParameters * getInstance() {return _pSelf;};
	static LangType getLangIDFromStr(const TCHAR *langName);
	static generic_string getLocPathFromStr(const generic_string & localizationCode);
	bool load();
	bool reloadLang();
	bool reloadStylers(TCHAR *stylePath = NULL);
    void destroyInstance();
	generic_string getCloudSettingsPath(CloudChoice cloudChoice);
	generic_string getSettingsFolder();

	bool _isTaskListRBUTTONUP_Active;
	int L_END;

	const NppGUI & getNppGUI() const {
        return _nppGUI;
    };

    const TCHAR * getWordList(LangType langID, int typeIndex) const {
    	Lang *pLang = getLangFromID(langID);
	    if (!pLang) return NULL;

        return pLang->getWords(typeIndex);
    };

	Lang * getLangFromID(LangType langID) const {
		for (int i = 0 ; i < _nbLang ; ++i)
		{
			if ((_langList[i]->_langID == langID) || (!_langList[i]))
				return _langList[i];
		}
		return NULL;
	};

	Lang * getLangFromIndex(int i) const {
		if (i >= _nbLang) return NULL;
		return _langList[i];
	};

	int getNbLang() const {return _nbLang;};
	
	LangType getLangFromExt(const TCHAR *ext);

	const TCHAR * getLangExtFromName(const TCHAR *langName) const {
		for (int i = 0 ; i < _nbLang ; ++i)
		{
			if (_langList[i]->_langName == langName)
				return _langList[i]->_defaultExtList;
		}
		return NULL;
	};

	const TCHAR * getLangExtFromLangType(LangType langType) const {
		for (int i = 0 ; i < _nbLang ; ++i)
		{
			if (_langList[i]->_langID == langType)
				return _langList[i]->_defaultExtList;
		}
		return NULL;
	};

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
	};

	bool putRecentFileInSubMenu() const {return _putRecentFileInSubMenu;};

	void setRecentFileCustomLength(int len) {
		_recentFileCustomLength = len;
	};

	int getRecentFileCustomLength() const {return _recentFileCustomLength;};


    const ScintillaViewParams & getSVP() const {
        return _svp;
    };

	bool writeRecentFileHistorySettings(int nbMaxFile = -1) const;
	bool writeHistory(const TCHAR *fullpath);

	bool writeProjectPanelsSettings() const;

	TiXmlNode * getChildElementByAttribut(TiXmlNode *pere, const TCHAR *childName,\
										  const TCHAR *attributName, const TCHAR *attributVal) const;

	bool writeScintillaParams(const ScintillaViewParams & svp);

	bool writeGUIParams();

	void writeStyles(LexerStylerArray & lexersStylers, StyleArray & globalStylers);
    bool insertTabInfo(const TCHAR *langName, int tabInfo);

    LexerStylerArray & getLStylerArray() {return _lexerStylerArray;};
    StyleArray & getGlobalStylers() {return _widgetStyleArray;};

    StyleArray & getMiscStylerArray() {return _widgetStyleArray;};
	GlobalOverride & getGlobalOverrideStyle() {return _nppGUI._globalOverride;};

    COLORREF getCurLineHilitingColour() {
		int i = _widgetStyleArray.getStylerIndexByName(TEXT("Current line background colour"));
        if (i == -1) return i;
        Style & style = _widgetStyleArray.getStyler(i);
        return style._bgColor;
    };
    void setCurLineHilitingColour(COLORREF colour2Set) {
        int i = _widgetStyleArray.getStylerIndexByName(TEXT("Current line background colour"));
        if (i == -1) return;
        Style & style = _widgetStyleArray.getStyler(i);
        style._bgColor = colour2Set;
    };

	void setFontList(HWND hWnd);
	const std::vector<generic_string> & getFontList() const { return _fontlist; };
	
	int getNbUserLang() const {return _nbUserLang;};
	UserLangContainer & getULCFromIndex(int i) {return *_userLangArray[i];};
	UserLangContainer * getULCFromName(const TCHAR *userLangName) {
		for (int i = 0 ; i < _nbUserLang ; ++i)
			if (!lstrcmp(userLangName, _userLangArray[i]->_name.c_str()))
				return _userLangArray[i];
		//qui doit etre jamais passer
		return NULL;
	};
	
	int getNbExternalLang() const {return _nbExternalLang;};
	int getExternalLangIndexFromName(const TCHAR *externalLangName) const {
		for (int i = 0 ; i < _nbExternalLang ; ++i)
		{
			if (!lstrcmp(externalLangName, _externalLangArray[i]->_name))
				return i;
		}
		return -1;
	};
	ExternalLangContainer & getELCFromIndex(int i) {return *_externalLangArray[i];};

	bool ExternalLangHasRoom() const {return _nbExternalLang < NB_MAX_EXTERNAL_LANG;};

	void getExternalLexerFromXmlTree(TiXmlDocument *doc);
	std::vector<TiXmlDocument *> * getExternalLexerDoc() { return &_pXmlExternalLexerDoc; };

	void writeUserDefinedLang();
	void writeShortcuts();
	void writeSession(const Session & session, const TCHAR *fileName = NULL);
	bool writeFindHistory();

	bool isExistingUserLangName(const TCHAR *newName) const {
		if ((!newName) || (!newName[0]))
			return true;

		for (int i = 0 ; i < _nbUserLang ; ++i)
		{
			if (!lstrcmp(_userLangArray[i]->_name.c_str(), newName))
				return true;
		}
		return false;
	};

	const TCHAR * getUserDefinedLangNameFromExt(TCHAR *ext, TCHAR *fullName) {
		if ((!ext) || (!ext[0]))
			return NULL;

		for (int i = 0 ; i < _nbUserLang ; ++i)
		{
			std::vector<generic_string> extVect;
			cutString(_userLangArray[i]->_ext.c_str(), extVect);
			for (size_t j = 0, len = extVect.size(); j < len; ++j)
				if (!generic_stricmp(extVect[j].c_str(), ext) || (_tcschr(fullName, '.') && !generic_stricmp(extVect[j].c_str(), fullName)))
					return _userLangArray[i]->_name.c_str();
		}
		return NULL;
	};

	int addUserLangToEnd(const UserLangContainer & userLang, const TCHAR *newName);
	void removeUserLang(int index);
	
	bool isExistingExternalLangName(const TCHAR *newName) const {
		if ((!newName) || (!newName[0]))
			return true;

		for (int i = 0 ; i < _nbExternalLang ; ++i)
		{
			if (!lstrcmp(_externalLangArray[i]->_name, newName))
				return true;
		}
		return false;
	};

	int addExternalLangToEnd(ExternalLangContainer * externalLang);

	TiXmlDocumentA * getNativeLangA() const {return _pXmlNativeLangDocA;};

	TiXmlDocument * getToolIcons() const {return _pXmlToolIconsDoc;};

	bool isTransparentAvailable() const {
		return (_transparentFuncAddr != NULL);
	};

	// 0 <= percent < 256
	// if (percent == 255) then opacq
	void SetTransparent(HWND hwnd, int percent) {
		if (!_transparentFuncAddr) return;
		::SetWindowLongPtr(hwnd, GWL_EXSTYLE, ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE) | 0x00080000);
		if (percent > 255)
			percent = 255;
		if (percent < 0)
			percent = 0;
		_transparentFuncAddr(hwnd, 0, percent, 0x00000002); 
	};

	void removeTransparent(HWND hwnd) {
		::SetWindowLongPtr(hwnd, GWL_EXSTYLE,  ::GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~0x00080000);
	};

	void setCmdlineParam(const CmdLineParams & cmdLineParams) {
		_cmdLineParams = cmdLineParams;
	};
	CmdLineParams & getCmdLineParams() {return _cmdLineParams;};

	void setFileSaveDlgFilterIndex(int ln) {_fileSaveDlgFilterIndex = ln;};
	int getFileSaveDlgFilterIndex() const {return _fileSaveDlgFilterIndex;};

	bool isRemappingShortcut() const {return _shortcuts.size() != 0;};

	std::vector<CommandShortcut> & getUserShortcuts() { return _shortcuts; };
	std::vector<int> & getUserModifiedShortcuts() { return _customizedShortcuts; };
	void addUserModifiedIndex(int index);

	std::vector<MacroShortcut> & getMacroList() { return _macros; };
	std::vector<UserCommand> & getUserCommandList() { return _userCommands; };
	std::vector<PluginCmdShortcut> & getPluginCommandList() { return _pluginCommands; };
	std::vector<int> & getPluginModifiedKeyIndices() { return _pluginCustomizedCmds; };
	void addPluginModifiedIndex(int index);

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
	const TCHAR * getWorkingDir() const {return _currentDirectory.c_str();};
	const TCHAR * getworkSpaceFilePath(int i) const {
		if (i < 0 || i > 2) return NULL;
		return _workSpaceFilePathes[i].c_str();
	};

	void setWorkSpaceFilePath(int i, const TCHAR *wsFile) {
		if (i < 0 || i > 2 || !wsFile) return;
		_workSpaceFilePathes[i] = wsFile;
	};

	void setWorkingDir(const TCHAR * newPath);

	void setStartWithLocFileName(generic_string locPath) {
		_startWithLocFileName = locPath;
	}

	bool loadSession(Session & session, const TCHAR *sessionFileName);
	int langTypeToCommandID(LangType lt) const;
	WNDPROC getEnableThemeDlgTexture() const {return _enableThemeDialogTextureFuncAddr;};

	struct FindDlgTabTitiles {
		generic_string _find;
		generic_string _replace;
		generic_string _findInFiles;
		generic_string _mark;
		FindDlgTabTitiles() : _find(TEXT("")), _replace(TEXT("")), _findInFiles(TEXT("")), _mark(TEXT("")) {};
	};

	FindDlgTabTitiles & getFindDlgTabTitiles() { return _findDlgTabTitiles;};

	bool asNotepadStyle() const {return _asNotepadStyle;};

	bool reloadPluginCmds() {
		return getPluginCmdsFromXmlTree();
	}

	bool getContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu);
	bool reloadContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu);
	winVer getWinVersion() { return _winVersion;};
	FindHistory & getFindHistory() {return _findHistory;};
	bool _isFindReplacing; // an on the fly variable for find/replace functions
	void safeWow64EnableWow64FsRedirection(BOOL Wow64FsEnableRedirection);
	
	LocalizationSwitcher & getLocalizationSwitcher() {
		return _localizationSwitcher;
	};

	ThemeSwitcher & getThemeSwitcher() {
		return _themeSwitcher;
	};

	std::vector<generic_string> & getBlackList() { return _blacklist; };
    bool isInBlackList(TCHAR *fn) {
        for (size_t i = 0, len = _blacklist.size(); i < len ; ++i)
            if (_blacklist[i] == fn)
                return true;
        return false;
    };

    PluginList & getPluginList() {return _pluginList;};
    bool importUDLFromFile(generic_string sourceFile);
    bool exportUDLToFile(int langIndex2export, generic_string fileName2save);
	NativeLangSpeaker * getNativeLangSpeaker() {
		return _pNativeLangSpeaker;
	};
	void setNativeLangSpeaker(NativeLangSpeaker *nls) {
		_pNativeLangSpeaker = nls;
	};

	bool isLocal() const {
		return _isLocal;
	};

	void saveConfig_xml() {
		if (_pXmlUserDoc)
			_pXmlUserDoc->SaveFile();
	};

	generic_string getUserPath() const {
		return _userPath;
	};

	void writeSettingsFilesOnCloudForThe1stTime(CloudChoice choice);

	COLORREF getCurrentDefaultBgColor() const {
		return _currentDefaultBgColor;
	};

	COLORREF getCurrentDefaultFgColor() const {
		return _currentDefaultFgColor;
	};

	void setCurrentDefaultBgColor(COLORREF c) {
		_currentDefaultBgColor = c;
	};

	void setCurrentDefaultFgColor(COLORREF c) {
		_currentDefaultFgColor = c;
	};

	DPIManager _dpiManager;

private:
    NppParameters();
	~NppParameters();

    static NppParameters *_pSelf;

	TiXmlDocument *_pXmlDoc, *_pXmlUserDoc, *_pXmlUserStylerDoc, *_pXmlUserLangDoc,\
		*_pXmlToolIconsDoc, *_pXmlShortcutDoc, *_pXmlSessionDoc,\
        *_pXmlBlacklistDoc;

	TiXmlDocument *_importedULD[NB_MAX_IMPORTED_UDL];
	int _nbImportedULD;
	
	TiXmlDocumentA *_pXmlNativeLangDocA, *_pXmlContextMenuDocA;

	std::vector<TiXmlDocument *> _pXmlExternalLexerDoc;

	NppGUI _nppGUI;
	ScintillaViewParams _svp;
	Lang *_langList[NB_LANG];
	int _nbLang;

	// Recent File History
	generic_string *_LRFileList[NB_MAX_LRF_FILE];
	int _nbRecentFile;
	int _nbMaxRecentFile;
	bool _putRecentFileInSubMenu;
	int _recentFileCustomLength;	//	<0: Full File Path Name
									//	=0: Only File Name
									//	>0: Custom Entry Length

	FindHistory _findHistory;

	UserLangContainer *_userLangArray[NB_MAX_USER_LANG];
	int _nbUserLang;
	generic_string _userDefineLangPath;
	ExternalLangContainer *_externalLangArray[NB_MAX_EXTERNAL_LANG];
	int _nbExternalLang;

	CmdLineParams _cmdLineParams;

	int _fileSaveDlgFilterIndex;

    // All Styles (colours & fonts)
	LexerStylerArray _lexerStylerArray;
    StyleArray _widgetStyleArray;

	std::vector<generic_string> _fontlist;
	std::vector<generic_string> _blacklist;
    PluginList _pluginList;

	HMODULE _hUXTheme;

	WNDPROC _transparentFuncAddr;
	WNDPROC _enableThemeDialogTextureFuncAddr;
	bool _isLocal;


	std::vector<CommandShortcut> _shortcuts;			//main menu shortuts. Static size
	std::vector<int> _customizedShortcuts;			//altered main menu shortcuts. Indices static. Needed when saving alterations
	std::vector<MacroShortcut> _macros;				//macro shortcuts, dynamic size, defined on loading macros and adding/deleting them
	std::vector<UserCommand> _userCommands;			//run shortcuts, dynamic size, defined on loading run commands and adding/deleting them
	std::vector<PluginCmdShortcut> _pluginCommands;	//plugin commands, dynamic size, defined on loading plugins
	std::vector<int> _pluginCustomizedCmds;			//plugincommands that have been altered. Indices determined after loading ALL plugins. Needed when saving alterations

	std::vector<ScintillaKeyMap> _scintillaKeyCommands;	//scintilla keycommands. Static size
	std::vector<int> _scintillaModifiedKeyIndices;		//modified scintilla keys. Indices static, determined by searching for commandId. Needed when saving alterations

	LocalizationSwitcher _localizationSwitcher;
	generic_string _startWithLocFileName;

	ThemeSwitcher _themeSwitcher;

	//vector<generic_string> _noMenuCmdNames;
	std::vector<MenuItemUnit> _contextMenuItems;
	Session _session;

	generic_string _shortcutsPath;
	generic_string _contextMenuPath;
	generic_string _sessionPath;
    generic_string _blacklistPath;
	generic_string _nppPath;
	generic_string _userPath;
	generic_string _stylerPath;
	generic_string _appdataNppDir; // sentinel of the absence of "doLocalConf.xml" : (_appdataNppDir == TEXT(""))?"doLocalConf.xml present":"doLocalConf.xml absent"
	generic_string _currentDirectory;
	generic_string _workSpaceFilePathes[3];

	Accelerator *_pAccelerator;
	ScintillaAccelerator * _pScintAccelerator;

	FindDlgTabTitiles _findDlgTabTitiles;
	bool _asNotepadStyle;

	winVer _winVersion;

	NativeLangSpeaker *_pNativeLangSpeaker;

	COLORREF _currentDefaultBgColor;
	COLORREF _currentDefaultFgColor;

	static int CALLBACK EnumFontFamExProc(const LOGFONT* lpelfe, const TEXTMETRIC *, DWORD, LPARAM lParam) {
		std::vector<generic_string>& strVect = *(std::vector<generic_string> *)lParam;
		const size_t vectSize = strVect.size();
		const TCHAR* lfFaceName = ((ENUMLOGFONTEX*)lpelfe)->elfLogFont.lfFaceName;

		//Search through all the fonts, EnumFontFamiliesEx never states anything about order
		//Start at the end though, that's the most likely place to find a duplicate
		for(int i = vectSize - 1 ; i >= 0 ; i--) {
			if ( !lstrcmp(strVect[i].c_str(), lfFaceName) )
				return 1;	//we already have seen this typeface, ignore it
		}
		//We can add the font
		//Add the face name and not the full name, we do not care about any styles
		strVect.push_back(lfFaceName);
		return 1; // I want to get all fonts
	};

	void getLangKeywordsFromXmlTree();
	bool getUserParametersFromXmlTree();
	bool getUserStylersFromXmlTree();
	bool getUserDefineLangsFromXmlTree(TiXmlDocument *tixmldoc);
    bool getUserDefineLangsFromXmlTree() {
        return getUserDefineLangsFromXmlTree(_pXmlUserLangDoc);
    };

	bool getShortcutsFromXmlTree();

	bool getMacrosFromXmlTree();
	bool getUserCmdsFromXmlTree();
	bool getPluginCmdsFromXmlTree();
	bool getScintKeysFromXmlTree();
	bool getSessionFromXmlTree(TiXmlDocument *pSessionDoc = NULL, Session *session = NULL);
    bool getBlackListFromXmlTree();

	void feedGUIParameters(TiXmlNode *node);
	void feedKeyWordsParameters(TiXmlNode *node);
	void feedFileListParameters(TiXmlNode *node);
    void feedScintillaParam(TiXmlNode *node);
	void feedDockingManager(TiXmlNode *node);
	void feedFindHistoryParameters(TiXmlNode *node);
	void feedProjectPanelsParameters(TiXmlNode *node);
    
	bool feedStylerArray(TiXmlNode *node);
    void getAllWordStyles(TCHAR *lexerName, TiXmlNode *lexerNode);

	bool feedUserLang(TiXmlNode *node);

	int getIndexFromKeywordListName(const TCHAR *name);
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
	
    void writeStyle2Element(Style & style2Write, Style & style2Sync, TiXmlElement *element);
	void insertUserLang2Tree(TiXmlNode *node, UserLangContainer *userLang);
	void insertCmd(TiXmlNode *cmdRoot, const CommandShortcut & cmd);
	void insertMacro(TiXmlNode *macrosRoot, const MacroShortcut & macro);
	void insertUserCmd(TiXmlNode *userCmdRoot, const UserCommand & userCmd);
	void insertScintKey(TiXmlNode *scintKeyRoot, const ScintillaKeyMap & scintKeyMap);
	void insertPluginCmd(TiXmlNode *pluginCmdRoot, const PluginCmdShortcut & pluginCmd);
	void stylerStrOp(bool op);
	TiXmlElement * insertGUIConfigBoolNode(TiXmlNode *r2w, const TCHAR *name, bool bVal);
	void insertDockingParamNode(TiXmlNode *GUIRoot);
	void writeExcludedLangList(TiXmlElement *element);
	void writePrintSetting(TiXmlElement *element);
	void initMenuKeys();		//initialise menu keys and scintilla keys. Other keys are initialized on their own
	void initScintillaKeys();	//these functions have to be called first before any modifications are loaded
	int getCmdIdFromMenuEntryItemName(HMENU mainMenuHadle, generic_string menuEntryName, generic_string menuItemName); // return -1 if not found
	int getPluginCmdIdFromMenuEntryItemName(HMENU pluginsMenu, generic_string pluginName, generic_string pluginCmdName); // return -1 if not found
};

#endif //PARAMETERS_H
