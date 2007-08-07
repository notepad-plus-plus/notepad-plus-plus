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

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <string>
#include "tinyxml.h"
//#include "ScintillaEditView.h"
#include "Scintilla.h"
#include "ScintillaRef.h"
#include "ToolBar.h"
#include "UserDefineLangReference.h"
#include "colors.h"
#include "shortcut.h"
#include "ContextMenu.h"
//#include "Buffer.h"

using namespace std;

const bool POS_VERTICAL = true;
const bool POS_HORIZOTAL = false;

const int UDD_SHOW   = 1; // 0000 0001 
const int UDD_DOCKED = 2; // 0000 0010

// 0 : 0000 0000 hide & undocked
// 1 : 0000 0001 show & undocked
// 2 : 0000 0010 hide & docked
// 3 : 0000 0011 show & docked

const int TAB_DRAWTOPBAR = 1;      // 0000 0001
const int TAB_DRAWINACTIVETAB = 2; // 0000 0010
const int TAB_DRAGNDROP = 4;       // 0000 0100
const int TAB_REDUCE = 8;	       // 0000 1000
const int TAB_CLOSEBUTTON = 16;    // 0001 0000
const int TAB_DBCLK2CLOSE = 32;    // 0010 0000

enum formatType {WIN_FORMAT, MAC_FORMAT, UNIX_FORMAT};

enum UniMode {uni8Bit=0, uniUTF8=1, uni16BE=2, uni16LE=3, uniCookie=4, uniEnd};

enum ChangeDetect {cdDisabled=0, cdEnabled=1, cdAutoUpdate=2};

enum BackupFeature {bak_none = 0, bak_simple = 1, bak_verbose = 2};

const int LANG_INDEX_INSTR = 0;
const int LANG_INDEX_INSTR2 = 1;
const int LANG_INDEX_TYPE = 2;
const int LANG_INDEX_TYPE2 = 3;
const int LANG_INDEX_TYPE3 = 4;
const int LANG_INDEX_TYPE4 = 5;
const int LANG_INDEX_TYPE5 = 6;

const bool SCIV_PRIMARY = false;
const bool SCIV_SECOND = true;

const char fontSizeStrs[][3] = {"", "8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "26", "28"};

#define LINEDRAW_FONT  "LINEDRAW.TTF"
const char localConfFile[] = "doLocalConf.xml";

void cutString(const char *str2cut, vector<string> & patternVect);
/*
struct HeaderLineState {
	HeaderLineState() : _headerLineNumber(0), _isCollapsed(false){};
	HeaderLineState(int lineNumber, bool isFoldUp) : _headerLineNumber(lineNumber), _isCollapsed(isFoldUp){};
	int _headerLineNumber;
	bool _isCollapsed;
};
*/
struct Position
{ 
	int _firstVisibleLine;
	int _startPos;
	int _endPos;
	int _xOffset;
};

struct sessionFileInfo : public Position {
	string _fileName;
	sessionFileInfo(const char *fn) {
		if (fn) _fileName = fn;
	};
	sessionFileInfo(const char *fn, Position pos) : Position(pos) {
		if (fn) _fileName = fn;
	};
	sessionFileInfo(string fn) : _fileName(fn){};
	sessionFileInfo(string fn, Position pos) : Position(pos), _fileName(fn){};
	vector<size_t> marks;
};

struct Session {
	int nbFiles() const {return _files.size();};
	size_t _actifIndex;
	vector<sessionFileInfo> _files;
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

struct PlugingDlgDockingInfo {
	char _name[MAX_PATH];
	int _internalID;

	int _currContainer;
	int _prevContainer;
	bool _isVisible;

	PlugingDlgDockingInfo(const char *pluginName, int id, int curr, int prev, bool isVis) : _internalID(id), _currContainer(curr), _prevContainer(prev), _isVisible(isVis){
		strcpy(_name, pluginName);
	};

	friend inline const bool operator==(const PlugingDlgDockingInfo & a, const PlugingDlgDockingInfo & b) {
		if ((strcmp(a._name, b._name) == 0) && (a._internalID == b._internalID))
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

	vector<FloatingWindowInfo>		_flaotingWindowInfo;
	vector<PlugingDlgDockingInfo>	_pluginDockInfo;
	vector<ContainerTabInfo>		_containerTabInfo;

	RECT * getFloatingRCFrom(int floatCont) {
		for (size_t i = 0 ; i < _flaotingWindowInfo.size() ; i++)
		{
			if (_flaotingWindowInfo[i]._cont == floatCont)
				return &(_flaotingWindowInfo[i]._pos);
		}
		return NULL;
	}
};

static int strVal(const char *str, int base) {
	if (!str) return -1;
	if (!str[0]) return 0;

    char *finStr;
    int result = strtol(str, &finStr, base);
    if (*finStr != '\0')
        return -1;
    return result;
};

static int decStrVal(const char *str) {
    return strVal(str, 10);
};

static int hexStrVal(const char *str) {
    return strVal(str, 16);
};


static int getKwClassFromName(const char *str) {
	if (!strcmp("instre1", str)) return LANG_INDEX_INSTR;
	if (!strcmp("instre2", str)) return LANG_INDEX_INSTR2;
	if (!strcmp("type1", str)) return LANG_INDEX_TYPE;
	if (!strcmp("type2", str)) return LANG_INDEX_TYPE2;
	if (!strcmp("type3", str)) return LANG_INDEX_TYPE3;
	if (!strcmp("type4", str)) return LANG_INDEX_TYPE4;
	if (!strcmp("type5", str)) return LANG_INDEX_TYPE5;
	return -1;
};

const int FONTSTYLE_BOLD = 1;
const int FONTSTYLE_ITALIC = 2;
const int FONTSTYLE_UNDERLINE = 4;

struct Style
{
	int _styleID;
    const char *_styleDesc;

	COLORREF _fgColor;
	COLORREF _bgColor;
	const char *_fontName;
	int _fontStyle;
	int _fontSize;

	int _keywordClass;
	string *_keywords;

	Style():_styleID(-1), _fgColor(COLORREF(-1)), _bgColor(COLORREF(-1)), _fontName(NULL), _fontStyle(-1), _fontSize(-1), _keywordClass(-1), _keywords(NULL){};

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
		_fontName = style._fontName;
		_fontSize = style._fontSize;
		_fontStyle = style._fontStyle;
		_keywordClass = style._keywordClass;
		if (style._keywords)
			_keywords = new string(*(style._keywords));
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
			this->_fontName = style._fontName;
			this->_fontSize = style._fontSize;
			this->_fontStyle = style._fontStyle;
			this->_keywordClass = style._keywordClass;

			if (!(this->_keywords) && style._keywords)
				this->_keywords = new string(*(style._keywords));
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

	void setKeywords(const char *str) {
		if (!_keywords)
			_keywords = new string(str);
		else
			*_keywords = str;
	};
};

const int MAX_STYLE = 30;

struct StyleArray
{
public:
    StyleArray() : _nbStyler(0){};

    StyleArray & operator=(const StyleArray & sa)
    {
        if (this != &sa)
        {
            this->_nbStyler = sa._nbStyler;
            for (int i = 0 ; i < _nbStyler ; i++)
            {
                this->_styleArray[i] = sa._styleArray[i];
            }
        }
        return *this;
    }

    int getNbStyler() const {return _nbStyler;};
	void setNbStyler(int nb) {_nbStyler = nb;};

    Style & getStyler(int index) {return _styleArray[index];};

    bool hasEnoughSpace() {return (_nbStyler < MAX_STYLE);};
    void addStyler(int styleID, TiXmlNode *styleNode);

	void addStyler(int styleID, char *styleName) {
		_styleArray[_nbStyler]._styleID = styleID;
		_styleArray[_nbStyler]._styleDesc = styleName;
		_nbStyler++;
	};

    int getStylerIndexByID(int id) {
        for (int i = 0 ; i < _nbStyler ; i++)
            if (_styleArray[i]._styleID == id)
                return i;
        return -1;
    };

    int getStylerIndexByName(const char *name) const {
		if (!name)
			return -1;
        for (int i = 0 ; i < _nbStyler ; i++)
			if (!strcmp(_styleArray[i]._styleDesc, name))
                return i;
        return -1;
    };

protected:
	Style _styleArray[MAX_STYLE];
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
            strcpy(this->_lexerName, ls._lexerName);
			strcpy(this->_lexerDesc, ls._lexerDesc);
			strcpy(this->_lexerUserExt, ls._lexerUserExt);
        }
        return *this;
    }

    void setLexerName(const char *lexerName) {
        strcpy(_lexerName, lexerName);
    };
	
	void setLexerDesc(const char *lexerDesc) {
        strcpy(_lexerDesc, lexerDesc);
    };

	void setLexerUserExt(const char *lexerUserExt) {
        strcpy(_lexerUserExt, lexerUserExt);
    };

    const char * getLexerName() const {return _lexerName;};
	const char * getLexerDesc() const {return _lexerDesc;};
    const char * getLexerUserExt() const {return _lexerUserExt;};

private :
	char _lexerName[16];
	char _lexerDesc[32];
	char _lexerUserExt[256];
};

const int MAX_LEXER_STYLE = 50;

struct LexerStylerArray
{
public :
	LexerStylerArray() : _nbLexerStyler(0){};

    LexerStylerArray & operator=(const LexerStylerArray & lsa)
    {
        if (this != &lsa)
        {
            this->_nbLexerStyler = lsa._nbLexerStyler;
            for (int i = 0 ; i < this->_nbLexerStyler ; i++)
                this->_lexerStylerArray[i] = lsa._lexerStylerArray[i];
        }
        return *this;
    }

    int getNbLexer() const {return _nbLexerStyler;};

    LexerStyler & getLexerFromIndex(int index)
    {
        return _lexerStylerArray[index];
    };

    const char * getLexerNameFromIndex(int index) const {return _lexerStylerArray[index].getLexerName();}
	const char * getLexerDescFromIndex(int index) const {return _lexerStylerArray[index].getLexerDesc();}

    LexerStyler * getLexerStylerByName(const char *lexerName) {
		if (!lexerName) return NULL;
        for (int i = 0 ; i < _nbLexerStyler ; i++)
        {
            if (!strcmp(_lexerStylerArray[i].getLexerName(), lexerName))
                return &(_lexerStylerArray[i]);
        }
        return NULL;
    };
    bool hasEnoughSpace() {return (_nbLexerStyler < MAX_LEXER_STYLE);};
    void addLexerStyler(const char *lexerName, const char *lexerDesc, const char *lexerUserExt, TiXmlNode *lexerNode);

private :
	LexerStyler _lexerStylerArray[MAX_LEXER_STYLE];
	int _nbLexerStyler;
};

struct NewDocDefaultSettings 
{
	formatType _format;
	UniMode _encoding;
	LangType _lang;
	NewDocDefaultSettings():_format(WIN_FORMAT), _encoding(uni8Bit), _lang(L_TXT){};
};

struct LangMenuItem {
	LangType _langType;
	int	_cmdID;
	string _langName;

	LangMenuItem(LangType lt, int cmdID = 0, string langName = ""):
	_langType(lt), _cmdID(cmdID), _langName(langName){};
};

struct PrintSettings {
	bool _printLineNumber;
	int _printOption;
	
	string _headerLeft;
	string _headerMiddle;
	string _headerRight;
	string _headerFontName;
	int _headerFontStyle;
	int _headerFontSize;
	
	string _footerLeft;
	string _footerMiddle;
	string _footerRight;
	string _footerFontName;
	int _footerFontStyle;
	int _footerFontSize;

	RECT _marge;

	PrintSettings() : _printLineNumber(true), _printOption(SC_PRINT_NORMAL), _headerLeft(""), _headerMiddle(""), _headerRight(""),\
		_headerFontName(""), _headerFontStyle(0), _headerFontSize(0),  _footerLeft(""), _footerMiddle(""), _footerRight(""),\
		_footerFontName(""), _footerFontStyle(0), _footerFontSize(0) {
			_marge.left = 0; _marge.top = 0; _marge.right = 0; _marge.bottom = 0;
		};

	bool isHeaderPresent() const {
		return ((_headerLeft != "") || (_headerMiddle != "") || (_headerRight != ""));
	};

	bool isFooterPresent() const {
		return ((_footerLeft != "") || (_footerMiddle != "") || (_footerRight != ""));
	};

	bool isUserMargePresent() const {
		return ((_marge.left != 0) || (_marge.top != 0) || (_marge.right != 0) || (_marge.bottom != 0));
	};


};

struct NppGUI
{
	NppGUI() : _toolBarStatus(TB_LARGE), _statusBarShow(true), \
		       _tabStatus(TAB_DRAWTOPBAR | TAB_DRAWINACTIVETAB | TAB_DRAGNDROP),\
	           _splitterPos(POS_HORIZOTAL), _userDefineDlgStatus(UDD_DOCKED), _tabSize(8),\
			   _tabReplacedBySpace(false), _fileAutoDetection(cdEnabled), _checkHistoryFiles(true),\
			   _isMaximized(false), _isMinimizedToTray(false), _rememberLastSession(true), _backup(bak_none), _useDir(false),\
			   _doTaskList(true), _maitainIndent(true), _saveOpenKeepInSameDir(false), _styleMRU(true), _styleURL(0) {
		_appPos.left = 0;
		_appPos.top = 0;
		_appPos.right = 700;
		_appPos.bottom = 500;

		_backupDir[0] = '\0';
	};
	toolBarStatusType _toolBarStatus;		// small, large ou hide
	bool _statusBarShow;		// show ou hide

	// 1er  bit : draw top bar; 
	// 2nd  bit : draw inactive tabs
	// 3eme bit : enable drag&drop
	// 4eme bit : reduce the height
	// 0:don't draw; 1:draw top bar 2:draw inactive tabs 3:draw both 7:draw both+drag&drop
	int _tabStatus;

	bool _splitterPos;			// horizontal ou vertical
	int _userDefineDlgStatus;	// (hide||show) && (docked||undocked)

	int _tabSize;
	bool _tabReplacedBySpace;

	ChangeDetect _fileAutoDetection;
	bool _checkHistoryFiles;

	RECT _appPos;

	bool _isMaximized;
	bool _isMinimizedToTray;
	bool _rememberLastSession;
	bool _doTaskList;
	bool _maitainIndent;
	bool _saveOpenKeepInSameDir;
	bool _styleMRU;

	// 0 : do nothing
	// 1 : don't draw underline
	// 2 : draw underline
	int _styleURL;

	NewDocDefaultSettings _newDocDefaultSettings;
	void setTabReplacedBySpace(bool b) {_tabReplacedBySpace = b;};
	const NewDocDefaultSettings & getNewDocDefaultSettings() const {return _newDocDefaultSettings;};
	vector<LangMenuItem> _excludedLangList;
	PrintSettings _printSettings;
	BackupFeature _backup;
	bool _useDir;
	char _backupDir[MAX_PATH];
	DockingManagerData _dockingData;
};

struct ScintillaViewParams
{
	ScintillaViewParams() : _lineNumberMarginShow(true), _bookMarkMarginShow(true), \
		                    _folderStyle(FOLDER_STYLE_BOX), _indentGuideLineShow(true),\
	                        _currentLineHilitingShow(true), _wrapSymbolShow(false),  _doWrap(false),\
							_zoom(0), _whiteSpaceShow(false), _eolShow(false){};
	bool _lineNumberMarginShow;
	bool _bookMarkMarginShow;
	folderStyle  _folderStyle; //"simple", "arrow", "circle" and "box"
	bool _indentGuideLineShow;
	bool _currentLineHilitingShow;
	bool _wrapSymbolShow;
	bool _doWrap;
	int _edgeMode;
	int _edgeNbColumn;
	int _zoom;
	bool _whiteSpaceShow;
	bool _eolShow;
};

const int NB_LIST = 20;
const int NB_MAX_LRF_FILE = 30;
const int NB_MAX_USER_LANG = 30;
const int LANG_NAME_LEN = 32;

struct Lang
{
	LangType _langID;
	char _langName[LANG_NAME_LEN];
	const char *_defaultExtList;
	const char *_langKeyWordList[NB_LIST];
	const char *_pCommentLineSymbol;
	const char *_pCommentStart;
	const char *_pCommentEnd;

	Lang() {for (int i = 0 ; i < NB_LIST ; _langKeyWordList[i] = NULL ,i++);};
	Lang(LangType langID, const char *name) : _langID(langID){
		_langName[0] = '\0';
		if (name)
			strcpy(_langName, name);
		for (int i = 0 ; i < NB_LIST ; _langKeyWordList[i] = NULL ,i++);
	};
	~Lang() {};
	void setDefaultExtList(const char *extLst){
		_defaultExtList = extLst;
	};
	
	void setCommentLineSymbol(const char *commentLine){
		_pCommentLineSymbol = commentLine;
	};
	
	void setCommentStart(const char *commentStart){
		_pCommentStart = commentStart;
	};

	void setCommentEnd(const char *commentEnd){
		_pCommentEnd = commentEnd;
	};

	const char * getDefaultExtList() const {
		return _defaultExtList;
	};
	
	void setWords(const char *words, int index) {
		_langKeyWordList[index] = words;
	};

	const char * getWords(int index) const {
		return _langKeyWordList[index];
	};

	LangType getLangID() const {return _langID;};
	const char * getLangName() const {return _langName;};
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

public :
	UserLangContainer(){
		strcpy(_name, "new user define");
		_ext[0] = '\0';

		// Keywords list of Delimiters (index 0)
		strcpy(_keywordLists[0], "000000");
		for (int i = 1 ; i < nbKeywodList ; i++)
			*_keywordLists[i] = '\0';
	};
	UserLangContainer(const char *name, const char *ext){
		//si le nom est trop long, on le tranche!
		int minSize = ((sizeof(_name) - 1) < strlen(name))?(sizeof(_name) - 1):strlen(name);
		int i = 0;
		for ( ; i < minSize ; i++)
			_name[i] = name[i];
		_name[i] = '\0';

		strcpy(_ext, ext);

		// Keywords list of Delimiters (index 0)
		strcpy(_keywordLists[0], "000000");
		for (int j = 1 ; j < nbKeywodList ; j++)
			*_keywordLists[j] = '\0';
	};

	UserLangContainer & operator=(const UserLangContainer & ulc) {
		if (this != &ulc)
        {
			strcpy(this->_name, ulc._name);
			strcpy(this->_ext, ulc._ext);
			this->_isCaseIgnored = ulc._isCaseIgnored;
			this->_styleArray = ulc._styleArray;
			int nbStyler = this->_styleArray.getNbStyler();
			for (int i = 0 ; i < nbStyler ; i++)
			{
				Style & st = this->_styleArray.getStyler(i);
				if (st._bgColor == COLORREF(-1))
					st._bgColor = white;
				if (st._fgColor == COLORREF(-1))
					st._fgColor = black;
			}
			for (int i = 0 ; i < nbKeywodList ; i++)
				strcpy(this->_keywordLists[i], ulc._keywordLists[i]);
		}
		return *this;
	};

	int getNbKeywordList() {return nbKeywodList;};
	char * getName() {return _name;};

private:
	char _name[langNameLenMax];
	char _ext[extsLenMax];

	StyleArray _styleArray;
	char _keywordLists[nbKeywodList][max_char];

	bool _isCaseIgnored;
	bool _isCommentLineSymbol;
	bool _isCommentSymbol;
	bool _isPrefix[nbPrefixListAllowed];
};

const int NB_LANG = 50;

const bool DUP = true;
const bool FREE = false;

class NppParameters 
{
public:
    static NppParameters * getInstance() {return _pSelf;};
	static LangType getLangIDFromStr(const char *langName);
	bool load(/*bool noUserPath = false*/);
    void destroyInstance();

	bool _isTaskListRBUTTONUP_Active;

	const NppGUI & getNppGUI() const {
        return _nppGUI;
    };

    const char * getWordList(LangType langID, int typeIndex) const {
    	Lang *pLang = getLangFromID(langID);
	    if (!pLang) return NULL;

        return pLang->getWords(typeIndex);
    };

	Lang * getLangFromID(LangType langID) const {
		for (int i = 0 ; i < _nbLang ; i++)
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

	const char * getLangExtFromName(const char *langName) const {
		for (int i = 0 ; i < _nbLang ; i++)
		{
			if (!strcmp(_langList[i]->_langName, langName))
				return _langList[i]->_defaultExtList;
		}
		return NULL;
	};

	const char * getLangExtFromLangType(LangType langType) const {
		for (int i = 0 ; i < _nbLang ; i++)
		{
			if (_langList[i]->_langID == langType)
				return _langList[i]->_defaultExtList;
		}
		return NULL;
	};

	int getNbLRFile() const {return _nbFile;};

	string *getLRFile(int index) const {
		return _LRFileList[index];
	};

	void setNbMaxFile(int nb) {
		_nbMaxFile = nb;
	};

	int getNbMaxFile() const {return _nbMaxFile;};

    const ScintillaViewParams & getSVP(bool whichOne) const {
        return _svp[whichOne];
    };

	void writeNbHistoryFile(int nb) {
		if (!_pXmlUserDoc) return;

		TiXmlNode *historyNode = (_pXmlUserDoc->FirstChild("NotepadPlus"))->FirstChildElement("History");
		if (historyNode)
			(historyNode->ToElement())->SetAttribute("nbMaxFile", nb);
	};

	void writeHistory(const char *fullpath);

	TiXmlNode * getChildElementByAttribut(TiXmlNode *pere, const char *childName,\
										  const char *attributName, const char *attributVal) const;

	void writeScintillaParams(const ScintillaViewParams & svp, bool whichOne);

	void writeGUIParams();

	void writeStyles(LexerStylerArray & lexersStylers, StyleArray & globalStylers);

    LexerStylerArray & getLStylerArray() {return _lexerStylerArray;};
    StyleArray & getGlobalStylers() {return _widgetStyleArray;};

    StyleArray & getMiscStylerArray() {return _widgetStyleArray;};

    COLORREF getCurLineHilitingColour() {
		int i = _widgetStyleArray.getStylerIndexByName("Current line background colour");
        if (i == -1) return i;
        Style & style = _widgetStyleArray.getStyler(i);
        return style._bgColor;
    };
    void setCurLineHilitingColour(COLORREF colour2Set) {
        int i = _widgetStyleArray.getStylerIndexByName("Current line background colour");
        if (i == -1) return;
        Style & style = _widgetStyleArray.getStyler(i);
        style._bgColor = colour2Set;
    };

	void setFontList(HWND hWnd);
	const vector<string> & getFontList() const {return _fontlist;};
	
	int getNbUserLang() const {return _nbUserLang;};
	UserLangContainer & getULCFromIndex(int i) {return *_userLangArray[i];};
	UserLangContainer & getULCFromName(const char *userLangName) {
		for (int i = 0 ; i < _nbUserLang ; i++)
			if (!strcmp(userLangName, _userLangArray[i]->_name))
				return *_userLangArray[i];
		//qui doit etre jamais passer
		return *_userLangArray[0];
	};

	void writeUserDefinedLang();
	void writeShortcuts(bool rewriteCmdSc, bool rewriteMacrosSc, bool rewriteUserCmdSc, bool rewriteScintillaKey, bool rewritePluginCmdSc);
	void writeSession(const Session & session, const char *fileName = NULL);


	bool isExistingUserLangName(const char *newName) const {
		if ((!newName) || (!newName[0]))
			return true;

		for (int i = 0 ; i < _nbUserLang ; i++)
		{
			if (!strcmp(_userLangArray[i]->_name, newName))
				return true;
		}
		return false;
	};

	const char * getLangNameFromExt(char *ext) {
		if ((!ext) || (!ext[0]))
			return NULL;

		for (int i = 0 ; i < _nbUserLang ; i++)
		{
			vector<string> extVect;
			cutString(_userLangArray[i]->_ext, extVect);
			for (size_t j = 0 ; j < extVect.size() ; j++)
				if (!strcmp(extVect[j].c_str(), ext))
					return _userLangArray[i]->_name;
		}
		return NULL;
	};

	int addUserLangToEnd(const UserLangContainer & userLang, const char *newName);
	void removeUserLang(int index);
	TiXmlDocument * getNativeLang() const {return _pXmlNativeLangDoc;};
	TiXmlDocument * getToolIcons() const {return _pXmlToolIconsDoc;};

	bool isTransparentAvailable() const {
		return (_transparentFuncAddr != NULL);
	};

	void SetTransparent(HWND hwnd, int percent) {
		//WNDPROC transparentFunc = (NppParameters::getInstance())->getTransparentFunc();
		if (!_transparentFuncAddr) return;
		::SetWindowLong(hwnd, GWL_EXSTYLE, ::GetWindowLong(hwnd, GWL_EXSTYLE) | /*WS_EX_LAYERED*/0x00080000);
				
		_transparentFuncAddr(hwnd, 0, percent, 0x00000002); 
	};

	void removeTransparent(HWND hwnd) {
		::SetWindowLong(hwnd, GWL_EXSTYLE,  ::GetWindowLong(hwnd, GWL_EXSTYLE) & ~/*WS_EX_LAYERED*/0x00080000);
	};
	
	void setDefLang(LangType langType) {_defLangType = langType;};
	LangType getDefLang() const {return _defLangType;};

	void setLineNumber2go(int ln) {_lineNumber2Go = ln;};
	int getLineNumber2go() const {return _lineNumber2Go;};

	bool isRemappingShortcut() const {return _shortcuts.size() != 0;};

	vector<CommandShortcut> & getUserShortcuts() {return _shortcuts;};
	vector<MacroShortcut> & getMacroList() {return _macros;};
	vector<UserCommand> & getUserCommandList() {return _userCommands;};
	vector<PluginCmdShortcut> & getPluginCommandList() {return _pluginCommands;};
	vector<PluginCmdShortcut> & getPluginCustomizedCmds() {return _pluginCustomizedCmds;};

	vector<ScintillaKeyMap> & getScintillaKeyList() {return _scintillaKeyCommands;};
	vector<ScintillaKeyMap> & getScintillaModifiedKeys() {return _scintillaModifiedKeys;};

	//vector<string> & getNoMenuCmdNames() {return _noMenuCmdNames;};

	vector<MenuItemUnit> & getContextMenuItems() {return _contextMenuItems;};
	const Session & getSession() const {return _session;};
	bool hasCustomContextMenu() const {return !_contextMenuItems.empty();};

	void setAccelerator(Accelerator *pAccel) {_pAccelerator = pAccel;};
	Accelerator * getAccelerator() {return _pAccelerator;};
	const char * getNppPath() const {return _nppPath;};
	const char * getAppDataNppDir() const {return _appdataNppDir;};

	bool loadSession(Session & session, const char *sessionFileName);
	int langTypeToCommandID(LangType lt) const;
	WNDPROC getEnableThemeDlgTexture() const {return _enableThemeDialogTextureFuncAddr;};
		
	struct FindDlgTabTitiles {
		string _find;
		string _replace;
		string _findInFiles;
		FindDlgTabTitiles() : _find(""), _replace(""), _findInFiles("") {};
		bool isWellFilled() {
			return (strcmp(_find.c_str(), "") != 0 && strcmp(_replace.c_str(), "") && strcmp(_findInFiles.c_str(), ""));
		};
	};

	FindDlgTabTitiles & getFindDlgTabTitiles() { return _findDlgTabTitiles;};

private:
    NppParameters();
	~NppParameters() {
		for (int i = 0 ; i < _nbLang ; i++)
			delete _langList[i];
		for (int i = 0 ; i < _nbFile ; i++)
			delete _LRFileList[i];
		for (int i = 0 ; i < _nbUserLang ; i++)
			delete _userLangArray[i];
		if (_hUser32)
			FreeLibrary(_hUser32);
		if (_hUXTheme)
			FreeLibrary(_hUXTheme);
		//::RemoveFontResource(LINEDRAW_FONT);
	};
    static NppParameters *_pSelf;

	TiXmlDocument *_pXmlDoc, *_pXmlUserDoc, *_pXmlUserStylerDoc, *_pXmlUserLangDoc, *_pXmlNativeLangDoc,\
		*_pXmlToolIconsDoc, *_pXmlShortcutDoc, *_pXmlContextMenuDoc, *_pXmlSessionDoc;

	NppGUI _nppGUI;
	ScintillaViewParams _svp[2];
	Lang *_langList[NB_LANG];
	int _nbLang;

	string *_LRFileList[NB_MAX_LRF_FILE];
	int _nbFile;
	int _nbMaxFile;

	UserLangContainer *_userLangArray[NB_MAX_USER_LANG];
	int _nbUserLang;
	char _userDefineLangPath[MAX_PATH];

	LangType _defLangType;
	int _lineNumber2Go;

    // All Styles (colours & fonts)
	LexerStylerArray _lexerStylerArray;
    StyleArray _widgetStyleArray;

	vector<string> _fontlist;

	HMODULE _hUser32;
	HMODULE _hUXTheme;

	WNDPROC _transparentFuncAddr;
	WNDPROC _enableThemeDialogTextureFuncAddr;


	vector<CommandShortcut> _shortcuts;
	vector<MacroShortcut> _macros;
	vector<UserCommand> _userCommands;
	vector<PluginCmdShortcut> _pluginCommands;
	vector<PluginCmdShortcut> _pluginCustomizedCmds;

	vector<ScintillaKeyMap> _scintillaKeyCommands;
	vector<ScintillaKeyMap> _scintillaModifiedKeys;

	//vector<string> _noMenuCmdNames;
	vector<MenuItemUnit> _contextMenuItems;
	Session _session;

	char _shortcutsPath[MAX_PATH];
	char _contextMenuPath[MAX_PATH];
	char _sessionPath[MAX_PATH];
	char _nppPath[MAX_PATH];
	char _appdataNppDir[MAX_PATH]; // sentinel of the absence of "doLocalConf.xml" : (_appdataNppDir == "")?"doLocalConf.xml present":"doLocalConf.xml absent"

	Accelerator *_pAccelerator;

	FindDlgTabTitiles _findDlgTabTitiles;
	
	static int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType, LPARAM lParam) {
		vector<string> *pStrVect = (vector<string> *)lParam;
        size_t vectSize = pStrVect->size();
        if (vectSize == 0)
			pStrVect->push_back((char *)lpelfe->elfFullName);
        else
        {
            const char *lastFontName = pStrVect->at(vectSize - 1).c_str();
            if (strcmp(lastFontName, (const char *)lpelfe->elfFullName))
				pStrVect->push_back((char *)lpelfe->elfFullName);
        } 
		return 1; // I want to get all fonts
	};

	void getLangKeywordsFromXmlTree();
	bool getUserParametersFromXmlTree();
	bool getUserStylersFromXmlTree();
	bool getUserDefineLangsFromXmlTree();
	bool getShortcutsFromXmlTree();
	void initScintillaKeys();

	bool getMacrosFromXmlTree();
	bool getUserCmdsFromXmlTree();
	bool getPluginCmdsFromXmlTree();
	bool getScintKeysFromXmlTree();
	bool getContextMenuFromXmlTree();
	bool getSessionFromXmlTree(TiXmlDocument *pSessionDoc = NULL, Session *session = NULL);

	void feedGUIParameters(TiXmlNode *node);
	void feedKeyWordsParameters(TiXmlNode *node);
	void feedFileListParameters(TiXmlNode *node);
    void feedScintillaParam(bool whichOne, TiXmlNode *node);
	void feedDockingManager(TiXmlNode *node);
    
	bool feedStylerArray(TiXmlNode *node);
    void getAllWordStyles(char *lexerName, TiXmlNode *lexerNode);

	void feedUserLang(TiXmlNode *node);
	int getIndexFromKeywordListName(const char *name);
	void feedUserStyles(TiXmlNode *node);
	void feedUserKeywordList(TiXmlNode *node);
	void feedUserSettings(TiXmlNode *node);

	void feedShortcut(TiXmlNode *node);
	void feedMacros(TiXmlNode *node);
	void feedUserCmds(TiXmlNode *node);
	void feedPluginCustomizedCmds(TiXmlNode *node);
	void feedScintKeys(TiXmlNode *node);

	void getActions(TiXmlNode *node, MacroShortcut & macroShortcut);
	bool getShortcuts(TiXmlNode *node, Shortcut & sc);
	bool getScintKey(TiXmlNode *node, ScintillaKeyMap & skm);
	
	int getIndexFromStr(const char *indexName) const;
    void writeStyle2Element(Style & style2Wite, Style & style2Sync, TiXmlElement *element);
	void insertUserLang2Tree(TiXmlNode *node, UserLangContainer *userLang);
	void insertCmd(TiXmlNode *cmdRoot, const CommandShortcut & cmd);
	void insertMacro(TiXmlNode *macrosRoot, const MacroShortcut & macro);
	void insertUserCmd(TiXmlNode *userCmdRoot, const UserCommand & userCmd);
	void insertScintKey(TiXmlNode *scintKeyRoot, const ScintillaKeyMap & scintKeyMap);
	void insertPluginCmd(TiXmlNode *pluginCmdRoot, const PluginCmdShortcut & pluginCmd);
	void stylerStrOp(bool op);
	void insertGUIConfigBoolNode(TiXmlNode *r2w, const char *name, bool bVal);
	void insertDockingParamNode(TiXmlNode *GUIRoot);
	void writeExcludedLangList(TiXmlElement *element);
	void writePrintSetting(TiXmlElement *element);
};

#endif //PARAMETERS_H
