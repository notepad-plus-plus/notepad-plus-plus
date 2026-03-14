     1|     1|// This file is part of npminmin project
     2|     2|// Copyright (C)2021 Don HO <don.h@free.fr>
     3|     3|
     4|     4|// This program is free software: you can redistribute it and/or modify
     5|     5|// it under the terms of the GNU General Public License as published by
     6|     6|// the Free Software Foundation, either version 3 of the License, or
     7|     7|// at your option any later version.
     8|     8|//
     9|     9|// This program is distributed in the hope that it will be useful,
    10|    10|// but WITHOUT ANY WARRANTY; without even the implied warranty of
    11|    11|// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    12|    12|// GNU General Public License for more details.
    13|    13|//
    14|    14|// You should have received a copy of the GNU General Public License
    15|    15|// along with this program.  If not, see <https://www.gnu.org/licenses/>.
    16|    16|
    17|    17|
    18|    18|#pragma once
    19|    19|
    20|    20|#include <windows.h>
    21|    21|
    22|    22|#include <shlwapi.h>
    23|    23|
    24|    24|#include <algorithm>
    25|    25|#include <array>
    26|    26|#include <cassert>
    27|    27|#include <cstdio>
    28|    28|#include <cstdint>
    29|    29|#include <cwchar>
    30|    30|#include <locale>
    31|    31|#include <map>
    32|    32|#include <memory>
    33|    33|#include <stdexcept>
    34|    34|#include <string>
    35|    35|#include <utility>
    36|    36|#include <vector>
    37|    37|
    38|    38|#include <ILexer.h>
    39|    39|#include <Lexilla.h>
    40|    40|#include <SciLexer.h>
    41|    41|#include <Scintilla.h>
    42|    42|
    43|    43|#include "ContextMenu.h"
    44|    44|#include "DockingCont.h"
    45|    45|#include "Notepad_plus_msgs.h"
    46|    46|#include "NppConstants.h"
    47|    47|#include "NppDarkMode.h"
    48|    48|#include "NppXml.h"
    49|    49|#include "ToolBar.h"
    50|    50|#include "colors.h"
    51|    51|#include "shortcut.h"
    52|    52|
    53|    53|#ifdef _WIN64
    54|    54|
    55|    55|#ifdef _M_ARM64
    56|    56|#define ARCH_TYPE IMAGE_FILE_MACHINE_ARM64
    57|    57|#else
    58|    58|#define ARCH_TYPE IMAGE_FILE_MACHINE_AMD64
    59|    59|#endif
    60|    60|
    61|    61|#else
    62|    62|#define ARCH_TYPE IMAGE_FILE_MACHINE_I386
    63|    63|
    64|    64|#endif
    65|    65|
    66|    66|#define CMD_INTERPRETER L"%COMSPEC%"
    67|    67|
    68|    68|class NativeLangSpeaker;
    69|    69|
    70|    70|/*!
    71|    71|** \brief Convert an int into a FormatType
    72|    72|** \param value An arbitrary int
    73|    73|** \param defvalue The default value to use if an invalid value is provided
    74|    74|*/
    75|    75|EolType convertIntToFormatType(int value, EolType defvalue = EolType::osdefault);
    76|    76|
    77|    77|#define PURE_LC_NONE	0
    78|    78|#define PURE_LC_BOL	 1
    79|    79|#define PURE_LC_WSP	 2
    80|    80|
    81|    81|void cutString(const wchar_t* str2cut, std::vector<std::wstring>& patternVect);
    82|    82|void cutStringBy(const wchar_t* str2cut, std::vector<std::wstring>& patternVect, wchar_t byChar, bool allowEmptyStr);
    83|    83|
    84|    84|struct Position
    85|    85|{
    86|    86|	intptr_t _firstVisibleLine = 0;
    87|    87|	intptr_t _startPos = 0;
    88|    88|	intptr_t _endPos = 0;
    89|    89|	intptr_t _xOffset = 0;
    90|    90|	intptr_t _selMode = 0;
    91|    91|	intptr_t _scrollWidth = 1;
    92|    92|	intptr_t _offset = 0;
    93|    93|	intptr_t _wrapCount = 0;
    94|    94|};
    95|    95|
    96|    96|
    97|    97|struct MapPosition
    98|    98|{
    99|    99|private:
   100|   100|	static constexpr intptr_t _maxPeekLenInKB = 512; // 512 KB
   101|   101|public:
   102|   102|	intptr_t _firstVisibleDisplayLine = -1;
   103|   103|
   104|   104|	intptr_t _firstVisibleDocLine = -1; // map
   105|   105|	intptr_t _lastVisibleDocLine = -1;  // map
   106|   106|	intptr_t _nbLine = -1;              // map
   107|   107|	intptr_t _higherPos = -1;           // map
   108|   108|	intptr_t _width = -1;
   109|   109|	intptr_t _height = -1;
   110|   110|	intptr_t _wrapIndentMode = -1;
   111|   111|
   112|   112|	intptr_t _KByteInDoc = _maxPeekLenInKB;
   113|   113|
   114|   114|	bool _isWrap = false;
   115|   115|	bool isValid() const { return (_firstVisibleDisplayLine != -1); }
   116|   116|	bool canScroll() const { return (_KByteInDoc < _maxPeekLenInKB); } // _nbCharInDoc < _maxPeekLen : Don't scroll the document for the performance issue
   117|   117|	static constexpr intptr_t getMaxPeekLenInKB() { return _maxPeekLenInKB; }
   118|   118|};
   119|   119|
   120|   120|
   121|   121|struct sessionFileInfo : public Position
   122|   122|{
   123|   123|	sessionFileInfo(const wchar_t* fn, const wchar_t* ln, int encoding, bool userReadOnly, bool isPinned, bool isUntitleTabRenamed, const Position& pos, const wchar_t* backupFilePath, FILETIME originalFileLastModifTimestamp, const MapPosition& mapPos) noexcept
   124|   124|		: Position(pos), _fileName(fn ? fn : L""), _langName(ln ? ln : L"")
   125|   125|		, _encoding(encoding), _isUserReadOnly(userReadOnly), _isPinned(isPinned)
   126|   126|		, _isUntitledTabRenamed(isUntitleTabRenamed), _backupFilePath(backupFilePath ? backupFilePath : L"")
   127|   127|		, _originalFileLastModifTimestamp(originalFileLastModifTimestamp), _mapPos(mapPos)
   128|   128|	{}
   129|   129|
   130|   130|	explicit sessionFileInfo(const std::wstring& fn) noexcept : _fileName(fn) {}
   131|   131|
   132|   132|	std::wstring _fileName;
   133|   133|	std::wstring _langName;
   134|   134|	std::vector<size_t> _marks;
   135|   135|	std::vector<size_t> _foldStates;
   136|   136|	int	_encoding = -1;
   137|   137|	bool _isUserReadOnly = false;
   138|   138|	bool _isMonitoring = false;
   139|   139|	int _individualTabColour = -1;
   140|   140|	bool _isRTL = false;
   141|   141|	bool _isPinned = false;
   142|   142|	bool _isUntitledTabRenamed = false;
   143|   143|	std::wstring _backupFilePath;
   144|   144|	FILETIME _originalFileLastModifTimestamp {};
   145|   145|
   146|   146|	MapPosition _mapPos;
   147|   147|};
   148|   148|
   149|   149|
   150|   150|struct Session
   151|   151|{
   152|   152|	size_t nbMainFiles() const { return _mainViewFiles.size(); }
   153|   153|	size_t nbSubFiles() const { return _subViewFiles.size(); }
   154|   154|	size_t _activeView = 0;
   155|   155|	size_t _activeMainIndex = 0;
   156|   156|	size_t _activeSubIndex = 0;
   157|   157|	bool _includeFileBrowser = false;
   158|   158|	std::wstring _fileBrowserSelectedItem;
   159|   159|	std::vector<sessionFileInfo> _mainViewFiles;
   160|   160|	std::vector<sessionFileInfo> _subViewFiles;
   161|   161|	std::vector<std::wstring> _fileBrowserRoots;
   162|   162|};
   163|   163|
   164|   164|
   165|   165|struct CmdLineParams
   166|   166|{
   167|   167|	bool _isNoPlugin = false;
   168|   168|	bool _isReadOnly = false;
   169|   169|	bool _isFullReadOnly = false;
   170|   170|	bool _isFullReadOnlySavingForbidden = false;
   171|   171|	bool _isNoSession = false;
   172|   172|	bool _isNoTab = false;
   173|   173|	bool _isPreLaunch = false;
   174|   174|	bool _showLoadingTime = false;
   175|   175|	bool _alwaysOnTop = false;
   176|   176|	bool _displayCmdLineArgs = false;
   177|   177|	intptr_t _line2go   = -1;
   178|   178|	intptr_t _column2go = -1;
   179|   179|	intptr_t _pos2go = -1;
   180|   180|
   181|   181|	POINT _point = {};
   182|   182|	bool _isPointXValid = false;
   183|   183|	bool _isPointYValid = false;
   184|   184|
   185|   185|	bool _isSessionFile = false;
   186|   186|	bool _isRecursive = false;
   187|   187|	bool _openFoldersAsWorkspace = false;
   188|   188|	bool _monitorFiles = false;
   189|   189|
   190|   190|	LangType _langType = L_EXTERNAL;
   191|   191|	std::wstring _localizationPath;
   192|   192|	std::wstring _udlName;
   193|   193|	std::wstring _pluginMessage;
   194|   194|
   195|   195|	std::wstring _easterEggName;
   196|   196|	unsigned char _quoteType = 0;
   197|   197|	int _ghostTypingSpeed = -1; // -1: initial value  1: slow  2: fast  3: speed of light
   198|   198|
   199|   199|	CmdLineParams()
   200|   200|	{
   201|   201|		_point.x = 0;
   202|   202|		_point.y = 0;
   203|   203|	}
   204|   204|
   205|   205|	bool isPointValid() const
   206|   206|	{
   207|   207|		return _isPointXValid && _isPointYValid;
   208|   208|	}
   209|   209|};
   210|   210|
   211|   211|// Command Line Parameters Data Transfer Object class:
   212|   212|// A POD (Plain Old Data) class to send CmdLineParams through WM_COPYDATA and to Notepad_plus::loadCommandlineParams
   213|   213|struct CmdLineParamsDTO
   214|   214|{
   215|   215|	bool _isReadOnly = false;
   216|   216|	bool _isNoSession = false;
   217|   217|	bool _isSessionFile = false;
   218|   218|	bool _isRecursive = false;
   219|   219|	bool _openFoldersAsWorkspace = false;
   220|   220|	bool _monitorFiles = false;
   221|   221|
   222|   222|	intptr_t _line2go = 0;
   223|   223|	intptr_t _column2go = 0;
   224|   224|	intptr_t _pos2go = 0;
   225|   225|
   226|   226|	LangType _langType = L_EXTERNAL;
   227|   227|	wchar_t _udlName[MAX_PATH] = {'\0'};
   228|   228|	wchar_t _pluginMessage[2048] = {'\0'};
   229|   229|
   230|   230|	static CmdLineParamsDTO FromCmdLineParams(const CmdLineParams& params)
   231|   231|	{
   232|   232|		CmdLineParamsDTO dto;
   233|   233|		dto._isReadOnly = params._isReadOnly;
   234|   234|		dto._isNoSession = params._isNoSession;
   235|   235|		dto._isSessionFile = params._isSessionFile;
   236|   236|		dto._isRecursive = params._isRecursive;
   237|   237|		dto._openFoldersAsWorkspace = params._openFoldersAsWorkspace;
   238|   238|		dto._monitorFiles = params._monitorFiles;
   239|   239|
   240|   240|		dto._line2go = params._line2go;
   241|   241|		dto._column2go = params._column2go;
   242|   242|		dto._pos2go = params._pos2go;
   243|   243|
   244|   244|		dto._langType = params._langType;
   245|   245|		wcsncpy(dto._udlName, params._udlName.c_str(), MAX_PATH);
   246|   246|		wcsncpy(dto._pluginMessage, params._pluginMessage.c_str(), 2048);
   247|   247|		return dto;
   248|   248|	}
   249|   249|};
   250|   250|
   251|   251|inline constexpr int FWI_PANEL_WH_DEFAULT = 100;
   252|   252|inline constexpr int DMD_PANEL_WH_DEFAULT = 200;
   253|   253|
   254|   254|struct FloatingWindowInfo
   255|   255|{
   256|   256|	int _cont = 0;
   257|   257|	RECT _pos = { 0, 0, FWI_PANEL_WH_DEFAULT, FWI_PANEL_WH_DEFAULT };
   258|   258|
   259|   259|	explicit FloatingWindowInfo(int cont, int x, int y, int w, int h) noexcept
   260|   260|		: _cont(cont), _pos{ x, y, w, h }
   261|   261|	{}
   262|   262|};
   263|   263|
   264|   264|struct PluginDlgDockingInfo final
   265|   265|{
   266|   266|	std::wstring _name;
   267|   267|	int _internalID = -1;
   268|   268|
   269|   269|	int _currContainer = -1;
   270|   270|	int _prevContainer = -1;
   271|   271|	bool _isVisible = false;
   272|   272|
   273|   273|	PluginDlgDockingInfo(const wchar_t* pluginName, int id, int curr, int prev, bool isVis)
   274|   274|		: _name(pluginName), _internalID(id), _currContainer(curr), _prevContainer(prev), _isVisible(isVis)
   275|   275|	{}
   276|   276|
   277|   277|	bool operator == (const PluginDlgDockingInfo& rhs) const
   278|   278|	{
   279|   279|		return _internalID == rhs._internalID && _name == rhs._name;
   280|   280|	}
   281|   281|};
   282|   282|
   283|   283|
   284|   284|struct ContainerTabInfo final
   285|   285|{
   286|   286|	int _cont = 0;
   287|   287|	int _activeTab = 0;
   288|   288|
   289|   289|	ContainerTabInfo(int cont, int activeTab) : _cont(cont), _activeTab(activeTab) {}
   290|   290|};
   291|   291|
   292|   292|struct DockingManagerData final
   293|   293|{
   294|   294|	int _leftWidth = DMD_PANEL_WH_DEFAULT;
   295|   295|	int _rightWidth = DMD_PANEL_WH_DEFAULT;
   296|   296|	int _topHeight = DMD_PANEL_WH_DEFAULT;
   297|   297|	int _bottomHeight = DMD_PANEL_WH_DEFAULT;
   298|   298|
   299|   299|	// will be updated at runtime (Notepad_plus::init & DockingManager::runProc DMM_MOVE_SPLITTER)
   300|   300|	LONG _minDockedPanelVisibility = HIGH_CAPTION; 
   301|   301|	SIZE _minFloatingPanelSize = { (HIGH_CAPTION) * 6, HIGH_CAPTION };
   302|   302|
   303|   303|	std::vector<FloatingWindowInfo> _floatingWindowInfo;
   304|   304|	std::vector<PluginDlgDockingInfo> _pluginDockInfo;
   305|   305|	std::vector<ContainerTabInfo> _containerTabInfo;
   306|   306|
   307|   307|	bool getFloatingRCFrom(int floatCont, RECT& rc) const
   308|   308|	{
   309|   309|		for (size_t i = 0, fwiLen = _floatingWindowInfo.size(); i < fwiLen; ++i)
   310|   310|		{
   311|   311|			if (_floatingWindowInfo[i]._cont == floatCont)
   312|   312|			{
   313|   313|				rc.left   = _floatingWindowInfo[i]._pos.left;
   314|   314|				rc.top	= _floatingWindowInfo[i]._pos.top;
   315|   315|				rc.right  = _floatingWindowInfo[i]._pos.right;
   316|   316|				rc.bottom = _floatingWindowInfo[i]._pos.bottom;
   317|   317|				return true;
   318|   318|			}
   319|   319|		}
   320|   320|		return false;
   321|   321|	}
   322|   322|};
   323|   323|
   324|   324|struct Style final
   325|   325|{
   326|   326|	int _styleID = STYLE_NOT_USED;
   327|   327|	std::wstring _styleDesc;
   328|   328|
   329|   329|	COLORREF _fgColor = static_cast<COLORREF>(STYLE_NOT_USED);
   330|   330|	COLORREF _bgColor = static_cast<COLORREF>(STYLE_NOT_USED);
   331|   331|	int _colorStyle = COLORSTYLE_ALL;
   332|   332|
   333|   333|	bool _isFontEnabled = false;
   334|   334|	std::wstring _fontName;
   335|   335|	int _fontStyle = STYLE_NOT_USED;
   336|   336|	int _fontSize = STYLE_NOT_USED;
   337|   337|
   338|   338|	int _nesting = FONTSTYLE_NONE;
   339|   339|
   340|   340|	int _keywordClass = STYLE_NOT_USED;
   341|   341|	std::string _keywords;
   342|   342|};
   343|   343|
   344|   344|
   345|   345|struct GlobalOverride final
   346|   346|{
   347|   347|	bool isEnable() const {return (enableFg || enableBg || enableFont || enableFontSize || enableBold || enableItalic || enableUnderLine);}
   348|   348|	bool enableFg = false;
   349|   349|	bool enableBg = false;
   350|   350|	bool enableFont = false;
   351|   351|	bool enableFontSize = false;
   352|   352|	bool enableBold = false;
   353|   353|	bool enableItalic = false;
   354|   354|	bool enableUnderLine = false;
   355|   355|};
   356|   356|
   357|   357|struct StyleArray
   358|   358|{
   359|   359|	auto begin() const { return _styleVect.begin(); }
   360|   360|	auto end() const { return _styleVect.end(); }
   361|   361|	auto begin() { return _styleVect.begin(); }
   362|   362|	auto end() { return _styleVect.end(); }
   363|   363|	void clear() { _styleVect.clear(); }
   364|   364|
   365|   365|	Style& getStyler(size_t index) {
   366|   366|		if (index >=  _styleVect.size())
   367|   367|			throw std::out_of_range("Styler index out of range");
   368|   368|		return _styleVect[index];
   369|   369|	}
   370|   370|
   371|   371|	void addStyler(int styleID, const NppXml::Element& styleNode);
   372|   372|
   373|   373|	void addStyler(int styleID, const std::wstring& styleName) {
   374|   374|		_styleVect.emplace_back();
   375|   375|		Style& s = _styleVect.back();
   376|   376|		s._styleID = styleID;
   377|   377|		s._styleDesc = styleName;
   378|   378|		s._fgColor = black;
   379|   379|		s._bgColor = white;
   380|   380|	}
   381|   381|
   382|   382|	Style* findByID(int id) {
   383|   383|		auto it = std::find_if(_styleVect.begin(), _styleVect.end(),
   384|   384|			[&id](const Style& s) { return s._styleID == id; });
   385|   385|
   386|   386|		return (it != _styleVect.end()) ? &(*it) : nullptr;
   387|   387|	}
   388|   388|
   389|   389|	Style* findByName(const std::wstring& name) {
   390|   390|		auto it = std::find_if(_styleVect.begin(), _styleVect.end(),
   391|   391|			[&name](const Style& s) { return s._styleDesc == name; });
   392|   392|
   393|   393|		return (it != _styleVect.end()) ? &(*it) : nullptr;
   394|   394|	}
   395|   395|
   396|   396|protected:
   397|   397|	std::vector<Style> _styleVect;
   398|   398|};
   399|   399|
   400|   400|
   401|   401|
   402|   402|struct LexerStyler : public StyleArray
   403|   403|{
   404|   404|public:
   405|   405|	LexerStyler() noexcept = default;
   406|   406|	LexerStyler(const LexerStyler& ls) noexcept = default;
   407|   407|	LexerStyler& operator=(const LexerStyler& ls)
   408|   408|	{
   409|   409|		if (this != &ls)
   410|   410|		{
   411|   411|			this->_styleVect = ls._styleVect;
   412|   412|			this->_lexerName = ls._lexerName;
   413|   413|			this->_lexerDesc = ls._lexerDesc;
   414|   414|			this->_lexerUserExt = ls._lexerUserExt;
   415|   415|		}
   416|   416|		return *this;
   417|   417|	}
   418|   418|
   419|   419|	void setLexerName(const wchar_t *lexerName)
   420|   420|	{
   421|   421|		_lexerName = lexerName;
   422|   422|	}
   423|   423|
   424|   424|	void setLexerDesc(const wchar_t *lexerDesc)
   425|   425|	{
   426|   426|		_lexerDesc = lexerDesc;
   427|   427|	}
   428|   428|
   429|   429|	void setLexerUserExt(const wchar_t *lexerUserExt) {
   430|   430|		_lexerUserExt = lexerUserExt;
   431|   431|	}
   432|   432|
   433|   433|	const wchar_t* getLexerName() const { return _lexerName.c_str(); }
   434|   434|	const wchar_t* getLexerDesc() const { return _lexerDesc.c_str(); }
   435|   435|	const wchar_t* getLexerUserExt() const { return _lexerUserExt.c_str(); }
   436|   436|
   437|   437|private :
   438|   438|	std::wstring _lexerName;
   439|   439|	std::wstring _lexerDesc;
   440|   440|	std::wstring _lexerUserExt;
   441|   441|};
   442|   442|
   443|   443|struct SortLexersInAlphabeticalOrder {
   444|   444|	bool operator() (const LexerStyler& l, const LexerStyler& r) const {
   445|   445|		if (std::wcscmp(l.getLexerDesc(), L"Search result") == 0)
   446|   446|			return false;
   447|   447|		if (std::wcscmp(r.getLexerDesc(), L"Search result") == 0)
   448|   448|			return true;
   449|   449|		return std::wcscmp(l.getLexerDesc(), r.getLexerDesc()) < 0;
   450|   450|	}
   451|   451|};
   452|   452|
   453|   453|struct LexerStylerArray
   454|   454|{
   455|   455|	size_t getNbLexer() const { return _lexerStylerVect.size(); }
   456|   456|	void clear() { _lexerStylerVect.clear(); }
   457|   457|
   458|   458|	LexerStyler & getLexerFromIndex(size_t index)
   459|   459|	{
   460|   460|		assert(index < _lexerStylerVect.size());
   461|   461|		return _lexerStylerVect[index];
   462|   462|	}
   463|   463|
   464|   464|	const wchar_t * getLexerNameFromIndex(size_t index) const { return _lexerStylerVect[index].getLexerName(); }
   465|   465|	const wchar_t * getLexerDescFromIndex(size_t index) const { return _lexerStylerVect[index].getLexerDesc(); }
   466|   466|
   467|   467|	LexerStyler* getLexerStylerByName(const wchar_t* lexerName) {
   468|   468|		if (!lexerName) return nullptr;
   469|   469|		auto it = std::find_if(_lexerStylerVect.begin(), _lexerStylerVect.end(),
   470|   470|			[&lexerName](const LexerStyler& ls) { return std::wcscmp(ls.getLexerName(), lexerName) == 0; });
   471|   471|
   472|   472|		return (it != _lexerStylerVect.end()) ? &(*it) : nullptr;
   473|   473|	}
   474|   474|
   475|   475|	void addLexerStyler(const char* lexerName, const char* lexerDesc, const char* lexerUserExt, const NppXml::Element& lexerNode);
   476|   476|
   477|   477|	void sort() {
   478|   478|		std::sort(_lexerStylerVect.begin(), _lexerStylerVect.end(), SortLexersInAlphabeticalOrder());
   479|   479|	}
   480|   480|
   481|   481|private :
   482|   482|	std::vector<LexerStyler> _lexerStylerVect;
   483|   483|};
   484|   484|
   485|   485|
   486|   486|struct NewDocDefaultSettings final
   487|   487|{
   488|   488|	EolType _format = EolType::osdefault;
   489|   489|	UniMode _unicodeMode = uniUTF8_NoBOM;
   490|   490|	bool _openAnsiAsUtf8 = true;
   491|   491|	LangType _lang = L_TEXT;
   492|   492|	int _codepage = -1; // -1 when not using
   493|   493|	bool _addNewDocumentOnStartup = false;
   494|   494|	bool _useContentAsTabName = false;
   495|   495|};
   496|   496|
   497|   497|
   498|   498|struct LangMenuItem final
   499|   499|{
   500|   500|	LangType _langType = L_TEXT;
   501|