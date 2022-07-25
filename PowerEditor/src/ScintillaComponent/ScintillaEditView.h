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


#include "Scintilla.h"
#include "ScintillaRef.h"
#include "SciLexer.h"
#include "Buffer.h"
#include "colors.h"
#include "UserDefineDialog.h"
#include "rgba_icons.h"


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif //WM_MOUSEWHEEL

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif //WM_MOUSEHWHEEL

#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND                   0x0319
#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define FAPPCOMMAND_MASK  0xF000
#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#endif //WM_APPCOMMAND

class NppParameters;

#define NB_WORD_LIST 4
#define WORD_LIST_LEN 256

typedef sptr_t(*SCINTILLA_FUNC) (void *, unsigned int, uptr_t, sptr_t);
typedef void * SCINTILLA_PTR;

#define WM_DOCK_USERDEFINE_DLG      (SCINTILLA_USER + 1)
#define WM_UNDOCK_USERDEFINE_DLG    (SCINTILLA_USER + 2)
#define WM_CLOSE_USERDEFINE_DLG     (SCINTILLA_USER + 3)
#define WM_REMOVE_USERLANG          (SCINTILLA_USER + 4)
#define WM_RENAME_USERLANG          (SCINTILLA_USER + 5)
#define WM_REPLACEALL_INOPENEDDOC   (SCINTILLA_USER + 6)
#define WM_FINDALL_INOPENEDDOC      (SCINTILLA_USER + 7)
#define WM_DOOPEN                   (SCINTILLA_USER + 8)
#define WM_FINDINFILES              (SCINTILLA_USER + 9)
#define WM_REPLACEINFILES           (SCINTILLA_USER + 10)
#define WM_FINDALL_INCURRENTDOC     (SCINTILLA_USER + 11)
#define WM_FRSAVE_INT               (SCINTILLA_USER + 12)
#define WM_FRSAVE_STR               (SCINTILLA_USER + 13)
#define WM_FINDALL_INCURRENTFINDER  (SCINTILLA_USER + 14)
#define WM_FINDINPROJECTS           (SCINTILLA_USER + 15)
#define WM_REPLACEINPROJECTS        (SCINTILLA_USER + 16)

const int NB_FOLDER_STATE = 7;

// Codepage
const int CP_CHINESE_TRADITIONAL = 950;
const int CP_CHINESE_SIMPLIFIED = 936;
const int CP_JAPANESE = 932;
const int CP_KOREAN = 949;
const int CP_GREEK = 1253;

//wordList
#define LIST_NONE 0
#define LIST_0 1
#define LIST_1 2
#define LIST_2 4
#define LIST_3 8
#define LIST_4 16
#define LIST_5 32
#define LIST_6 64
#define LIST_7 128
#define LIST_8 256

const bool fold_uncollapse = true;
const bool fold_collapse = false;
#define MAX_FOLD_COLLAPSE_LEVEL	8

enum TextCase : UCHAR
{
	UPPERCASE,
	LOWERCASE,
	TITLECASE_FORCE,
	TITLECASE_BLEND,
	SENTENCECASE_FORCE,
	SENTENCECASE_BLEND,
	INVERTCASE,
	RANDOMCASE
};

const UCHAR MASK_FORMAT = 0x03;
const UCHAR MASK_ZERO_LEADING = 0x04;
const UCHAR BASE_10 = 0x00; // Dec
const UCHAR BASE_16 = 0x01; // Hex
const UCHAR BASE_08 = 0x02; // Oct
const UCHAR BASE_02 = 0x03; // Bin


const int MARK_BOOKMARK = 24;
const int MARK_HIDELINESBEGIN = 23;
const int MARK_HIDELINESEND = 22;
const int MARK_HIDELINESUNDERLINE = 21;
//const int MARK_LINEMODIFIEDUNSAVED = 20;
//const int MARK_LINEMODIFIEDSAVED = 19;
// 24 - 16 reserved for Notepad++ internal used
// 15 - 0  are free to use for plugins


int getNbDigits(int aNum, int base);
HMODULE loadSciLexerDll();

TCHAR * int2str(TCHAR *str, int strLen, int number, int base, int nbChiffre, bool isZeroLeading);

typedef LRESULT (WINAPI *CallWindowProcFunc) (WNDPROC,HWND,UINT,WPARAM,LPARAM);

const bool L2R = true;
const bool R2L = false;

struct ColumnModeInfo {
	intptr_t _selLpos = 0;
	intptr_t _selRpos = 0;
	intptr_t _order = -1; // 0 based index
	bool _direction = L2R; // L2R or R2L
	intptr_t _nbVirtualCaretSpc = 0;
	intptr_t _nbVirtualAnchorSpc = 0;

	ColumnModeInfo(intptr_t lPos, intptr_t rPos, intptr_t order, bool dir = L2R, intptr_t vAnchorNbSpc = 0, intptr_t vCaretNbSpc = 0)
		: _selLpos(lPos), _selRpos(rPos), _order(order), _direction(dir), _nbVirtualAnchorSpc(vAnchorNbSpc), _nbVirtualCaretSpc(vCaretNbSpc){};

	bool isValid() const {
		return (_order >= 0 && _selLpos >= 0 && _selRpos >= 0 && _selLpos <= _selRpos);
	};
};

//
// SortClass for vector<ColumnModeInfo>
// sort in _order : increased order
struct SortInSelectOrder {
	bool operator() (ColumnModeInfo & l, ColumnModeInfo & r) {
		return (l._order < r._order);
	}
};

//
// SortClass for vector<ColumnModeInfo>
// sort in _selLpos : increased order
struct SortInPositionOrder {
	bool operator() (ColumnModeInfo & l, ColumnModeInfo & r) {
		return (l._selLpos < r._selLpos);
	}
};

typedef std::vector<ColumnModeInfo> ColumnModeInfos;

struct LanguageNameInfo {
	const TCHAR* _langName = nullptr;
	const TCHAR* _shortName = nullptr;
	const TCHAR* _longName = nullptr;
	LangType _langID = L_TEXT;
	const char* _lexerID = nullptr;
};

#define URL_INDIC 8
class ISorter;

class ScintillaEditView : public Window
{
friend class Finder;
public:
	ScintillaEditView(): Window() {
		++_refCount;
	};

	virtual ~ScintillaEditView()
	{
		--_refCount;

		if ((!_refCount)&&(_SciInit))
		{
			Scintilla_ReleaseResources();

			for (BufferStyleMap::iterator it(_hotspotStyles.begin()); it != _hotspotStyles.end(); ++it )
			{
				delete it->second;
			}
		}
	};

	virtual void destroy()
	{
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
		_pScintillaFunc = NULL;
	};

	virtual void init(HINSTANCE hInst, HWND hPere);

	LRESULT execute(UINT Msg, WPARAM wParam=0, LPARAM lParam=0) const {
		try {
			return (_pScintillaFunc) ? _pScintillaFunc(_pScintillaPtr, Msg, wParam, lParam) : -1;
		}
		catch (...)
		{
			return -1;
		}
	};

	void activateBuffer(BufferID buffer, bool force = false);

	void getCurrentFoldStates(std::vector<size_t> & lineStateVector);
	void syncFoldStateWith(const std::vector<size_t> & lineStateVectorNew);

	void getText(char *dest, size_t start, size_t end) const;
	void getGenericText(TCHAR *dest, size_t destlen, size_t start, size_t end) const;
	void getGenericText(TCHAR *dest, size_t deslen, size_t start, size_t end, intptr_t* mstart, intptr_t* mend) const;
	generic_string getGenericTextAsString(size_t start, size_t end) const;
	void insertGenericTextFrom(size_t position, const TCHAR *text2insert) const;
	void replaceSelWith(const char * replaceText);

	intptr_t getSelectedTextCount() {
		Sci_CharacterRange range = getSelection();
		return (range.cpMax - range.cpMin);
	};

	void getVisibleStartAndEndPosition(intptr_t* startPos, intptr_t* endPos);
    char * getWordFromRange(char * txt, size_t size, size_t pos1, size_t pos2);
	char * getSelectedText(char * txt, size_t size, bool expand = true);
    char * getWordOnCaretPos(char * txt, size_t size);
    TCHAR * getGenericWordOnCaretPos(TCHAR * txt, int size);
	TCHAR * getGenericSelectedText(TCHAR * txt, int size, bool expand = true);
	intptr_t searchInTarget(const TCHAR * Text2Find, size_t lenOfText2Find, size_t fromPos, size_t toPos) const;
	void appandGenericText(const TCHAR * text2Append) const;
	void addGenericText(const TCHAR * text2Append) const;
	void addGenericText(const TCHAR * text2Append, intptr_t* mstart, intptr_t* mend) const;
	intptr_t replaceTarget(const TCHAR * str2replace, intptr_t fromTargetPos = -1, intptr_t toTargetPos = -1) const;
	intptr_t replaceTargetRegExMode(const TCHAR * re, intptr_t fromTargetPos = -1, intptr_t toTargetPos = -1) const;
	void showAutoComletion(size_t lenEntered, const TCHAR * list);
	void showCallTip(size_t startPos, const TCHAR * def);
	generic_string getLine(size_t lineNumber);
	void getLine(size_t lineNumber, TCHAR * line, size_t lineBufferLen);
	void addText(size_t length, const char *buf);

	void insertNewLineAboveCurrentLine();
	void insertNewLineBelowCurrentLine();

	void saveCurrentPos();
	void restoreCurrentPosPreStep();
	void restoreCurrentPosPostStep();

	void beginOrEndSelect();
	bool beginEndSelectedIsStarted() const {
		return _beginSelectPosition != -1;
	};

	size_t getCurrentDocLen() const {
		return size_t(execute(SCI_GETLENGTH));
	};

	Sci_CharacterRange getSelection() const {
		Sci_CharacterRange crange;
		crange.cpMin = static_cast<Sci_PositionCR>(execute(SCI_GETSELECTIONSTART));
		crange.cpMax = static_cast<Sci_PositionCR>(execute(SCI_GETSELECTIONEND));
		return crange;
	};

	void getWordToCurrentPos(TCHAR * str, intptr_t strLen) const {
		auto caretPos = execute(SCI_GETCURRENTPOS);
		auto startPos = execute(SCI_WORDSTARTPOSITION, caretPos, true);

		str[0] = '\0';
		if ((caretPos - startPos) < strLen)
			getGenericText(str, strLen, startPos, caretPos);
	};

    void doUserDefineDlg(bool willBeShown = true, bool isRTL = false) {
        _userDefineDlg.doDialog(willBeShown, isRTL);
    };

    static UserDefineDialog * getUserDefineDlg() {return &_userDefineDlg;};

    void setCaretColorWidth(int color, int width = 1) const {
        execute(SCI_SETCARETFORE, color);
        execute(SCI_SETCARETWIDTH, width);
    };

	void beSwitched() {
		_userDefineDlg.setScintilla(this);
	};

    //Marge member and method
    static const int _SC_MARGE_LINENUMBER;
    static const int _SC_MARGE_SYBOLE;
    static const int _SC_MARGE_FOLDER;
	//static const int _SC_MARGE_MODIFMARKER;

    void showMargin(int whichMarge, bool willBeShowed = true);

    bool hasMarginShowed(int witchMarge) {
		return (execute(SCI_GETMARGINWIDTHN, witchMarge, 0) != 0);
    };

    void updateBeginEndSelectPosition(bool is_insert, size_t position, size_t length);
    void marginClick(Sci_Position position, int modifiers);

    void setMakerStyle(folderStyle style) {
		bool display;
		if (style == FOLDER_STYLE_NONE)
		{
			style = FOLDER_STYLE_BOX;
			display = false;
		}
		else
		{
			display = true;
		}

		COLORREF foldfgColor = white, foldbgColor = grey, activeFoldFgColor = red;
		getFoldColor(foldfgColor, foldbgColor, activeFoldFgColor);

		for (int i = 0 ; i < NB_FOLDER_STATE ; ++i)
			defineMarker(_markersArray[FOLDER_TYPE][i], _markersArray[style][i], foldfgColor, foldbgColor, activeFoldFgColor);
		showMargin(ScintillaEditView::_SC_MARGE_FOLDER, display);
    };


	void setWrapMode(lineWrapMethod meth) {
		int mode = (meth == LINEWRAP_ALIGNED)?SC_WRAPINDENT_SAME:\
				(meth == LINEWRAP_INDENT)?SC_WRAPINDENT_INDENT:SC_WRAPINDENT_FIXED;
		execute(SCI_SETWRAPINDENTMODE, mode);
	};


	void showWSAndTab(bool willBeShowed = true) {
		execute(SCI_SETVIEWWS, willBeShowed?SCWS_VISIBLEALWAYS:SCWS_INVISIBLE);
		execute(SCI_SETWHITESPACESIZE, 2, 0);
	};

	void showEOL(bool willBeShowed = true) {
		execute(SCI_SETVIEWEOL, willBeShowed);
	};

	bool isEolVisible() {
		return (execute(SCI_GETVIEWEOL) != 0);
	};
	void showInvisibleChars(bool willBeShowed = true) {
		showWSAndTab(willBeShowed);
		showEOL(willBeShowed);
	};

	bool isInvisibleCharsShown() {
		return (execute(SCI_GETVIEWWS) != 0);
	};

	void showIndentGuideLine(bool willBeShowed = true);

	bool isShownIndentGuide() const {
		return (execute(SCI_GETINDENTATIONGUIDES) != 0);
	};

    void wrap(bool willBeWrapped = true) {
        execute(SCI_SETWRAPMODE, willBeWrapped);
    };

    bool isWrap() const {
        return (execute(SCI_GETWRAPMODE) == SC_WRAP_WORD);
    };

	bool isWrapSymbolVisible() const {
		return (execute(SCI_GETWRAPVISUALFLAGS) != SC_WRAPVISUALFLAG_NONE);
	};

    void showWrapSymbol(bool willBeShown = true) {
		execute(SCI_SETWRAPVISUALFLAGSLOCATION, SC_WRAPVISUALFLAGLOC_DEFAULT);
		execute(SCI_SETWRAPVISUALFLAGS, willBeShown?SC_WRAPVISUALFLAG_END:SC_WRAPVISUALFLAG_NONE);
    };

	intptr_t getCurrentLineNumber()const {
		return execute(SCI_LINEFROMPOSITION, execute(SCI_GETCURRENTPOS));
	};

	intptr_t lastZeroBasedLineNumber() const {
		auto endPos = execute(SCI_GETLENGTH);
		return execute(SCI_LINEFROMPOSITION, endPos);
	};

	intptr_t getCurrentXOffset()const{
		return execute(SCI_GETXOFFSET);
	};

	void setCurrentXOffset(long xOffset){
		execute(SCI_SETXOFFSET,xOffset);
	};

	void scroll(intptr_t column, intptr_t line){
		execute(SCI_LINESCROLL, column, line);
	};

	intptr_t getCurrentPointX()const{
		return execute(SCI_POINTXFROMPOSITION, 0, execute(SCI_GETCURRENTPOS));
	};

	intptr_t getCurrentPointY()const{
		return execute(SCI_POINTYFROMPOSITION, 0, execute(SCI_GETCURRENTPOS));
	};

	intptr_t getTextHeight()const{
		return execute(SCI_TEXTHEIGHT);
	};

	int getTextZoneWidth() const;

	void gotoLine(intptr_t line){
		if (line < execute(SCI_GETLINECOUNT))
			execute(SCI_GOTOLINE,line);
	};

	intptr_t getCurrentColumnNumber() const {
        return execute(SCI_GETCOLUMN, execute(SCI_GETCURRENTPOS));
    };

	std::pair<size_t, size_t> getSelectedCharsAndLinesCount(long long maxSelectionsForLineCount = -1) const;

	size_t getUnicodeSelectedLength() const;

	intptr_t getLineLength(size_t line) const {
		return execute(SCI_GETLINEENDPOSITION, line) - execute(SCI_POSITIONFROMLINE, line);
	};

	intptr_t getLineIndent(size_t line) const {
		return execute(SCI_GETLINEINDENTATION, line);
	};

	void setLineIndent(size_t line, size_t indent) const;

	void updateLineNumbersMargin(bool forcedToHide) {
		const ScintillaViewParams& svp = NppParameters::getInstance().getSVP();
		if (forcedToHide)
		{
			execute(SCI_SETMARGINWIDTHN, _SC_MARGE_LINENUMBER, 0);
		}
		else if (svp._lineNumberMarginShow)
		{
			updateLineNumberWidth();
		}
		else
		{
			execute(SCI_SETMARGINWIDTHN, _SC_MARGE_LINENUMBER, 0);
		}
	}

	void updateLineNumberWidth();
	void performGlobalStyles();

	void expand(size_t& line, bool doExpand, bool force = false, intptr_t visLevels = 0, intptr_t level = -1);

	std::pair<size_t, size_t> getSelectionLinesRange(intptr_t selectionNumber = -1) const;
    void currentLinesUp() const;
    void currentLinesDown() const;

	intptr_t caseConvertRange(intptr_t start, intptr_t end, TextCase caseToConvert);
	void changeCase(__inout wchar_t * const strWToConvert, const int & nbChars, const TextCase & caseToConvert) const;
	void convertSelectedTextTo(const TextCase & caseToConvert);
	void setMultiSelections(const ColumnModeInfos & cmi);

    void convertSelectedTextToLowerCase() {
		// if system is w2k or xp
		if ((NppParameters::getInstance()).isTransparentAvailable())
			convertSelectedTextTo(LOWERCASE);
		else
			execute(SCI_LOWERCASE);
	};

    void convertSelectedTextToUpperCase() {
		// if system is w2k or xp
		if ((NppParameters::getInstance()).isTransparentAvailable())
			convertSelectedTextTo(UPPERCASE);
		else
			execute(SCI_UPPERCASE);
	};

	void convertSelectedTextToNewerCase(const TextCase & caseToConvert) {
		// if system is w2k or xp
		if ((NppParameters::getInstance()).isTransparentAvailable())
			convertSelectedTextTo(caseToConvert);
		else
			::MessageBox(_hSelf, TEXT("This function needs a newer OS version."), TEXT("Change Case Error"), MB_OK | MB_ICONHAND);
	};

	bool isFoldIndentationBased() const;
	void collapseFoldIndentationBased(int level2Collapse, bool mode);
	void collapse(int level2Collapse, bool mode);
	void foldAll(bool mode);
	void fold(size_t line, bool mode);
	bool isFolded(size_t line) const {
		return (execute(SCI_GETFOLDEXPANDED, line) != 0);
	};
	bool isCurrentLineFolded() const;
	void foldCurrentPos(bool mode);
	int getCodepage() const {return _codepage;};

	ColumnModeInfos getColumnModeSelectInfo();

	void columnReplace(ColumnModeInfos & cmi, const TCHAR *str);
	void columnReplace(ColumnModeInfos & cmi, int initial, int incr, int repeat, UCHAR format);

	void clearIndicator(int indicatorNumber) {
		size_t docStart = 0;
		size_t docEnd = getCurrentDocLen();
		execute(SCI_SETINDICATORCURRENT, indicatorNumber);
		execute(SCI_INDICATORCLEARRANGE, docStart, docEnd - docStart);
	};

	bool getIndicatorRange(size_t indicatorNumber, size_t* from = NULL, size_t* to = NULL, size_t* cur = NULL);

	static LanguageNameInfo _langNameInfoArray[L_EXTERNAL+1];

	void bufferUpdated(Buffer * buffer, int mask);
	BufferID getCurrentBufferID() { return _currentBufferID; };
	Buffer * getCurrentBuffer() { return _currentBuffer; };
	void setCurrentBuffer(Buffer *buf2set) { _currentBuffer = buf2set; };
	void styleChange();

	void hideLines();

	bool markerMarginClick(size_t lineNumber);	//true if it did something
	void notifyMarkers(Buffer * buf, bool isHide, size_t location, bool del);
	void runMarkers(bool doHide, size_t searchStart, bool endOfDoc, bool doDelete);

	bool isSelecting() const {
		static Sci_CharacterRange previousSelRange = getSelection();
		Sci_CharacterRange currentSelRange = getSelection();

		if (currentSelRange.cpMin == currentSelRange.cpMax)
		{
			previousSelRange = currentSelRange;
			return false;
		}

		if ((previousSelRange.cpMin == currentSelRange.cpMin) || (previousSelRange.cpMax == currentSelRange.cpMax))
		{
			previousSelRange = currentSelRange;
			return true;
		}

		previousSelRange = currentSelRange;
		return false;
	};

	bool isPythonStyleIndentation(LangType typeDoc) const{
		return (typeDoc == L_PYTHON || typeDoc == L_COFFEESCRIPT || typeDoc == L_HASKELL ||\
			typeDoc == L_C || typeDoc == L_CPP || typeDoc == L_OBJC || typeDoc == L_CS || typeDoc == L_JAVA ||\
			typeDoc == L_PHP || typeDoc == L_JS || typeDoc == L_JAVASCRIPT || typeDoc == L_MAKEFILE || typeDoc == L_ASN1);
	};

	void defineDocType(LangType typeDoc);	//setup stylers for active document

	void addCustomWordChars();
	void restoreDefaultWordChars();
	void setWordChars();
	void setCRLF(long color = -1);

	void mouseWheel(WPARAM wParam, LPARAM lParam) {
		scintillaNew_Proc(_hSelf, WM_MOUSEWHEEL, wParam, lParam);
	};

	void setHotspotStyle(Style& styleToSet);
    void setTabSettings(Lang *lang);
	bool isWrapRestoreNeeded() const {return _wrapRestoreNeeded;};
	void setWrapRestoreNeeded(bool isWrapRestoredNeeded) {_wrapRestoreNeeded = isWrapRestoredNeeded;};

	bool isCJK() const {
		return ((_codepage == CP_CHINESE_TRADITIONAL) || (_codepage == CP_CHINESE_SIMPLIFIED) ||
			    (_codepage == CP_JAPANESE) || (_codepage == CP_KOREAN));
	};
	void scrollPosToCenter(size_t pos);
	generic_string getEOLString();
	void setBorderEdge(bool doWithBorderEdge);
	void sortLines(size_t fromLine, size_t toLine, ISorter *pSort);
	void changeTextDirection(bool isRTL);
	bool isTextDirectionRTL() const;
	void setPositionRestoreNeeded(bool val) { _positionRestoreNeeded = val; };
	void markedTextToClipboard(int indiStyle, bool doAll = false);
	void removeAnyDuplicateLines();

protected:
	static bool _SciInit;

	static int _refCount;

    static UserDefineDialog _userDefineDlg;

    static const int _markersArray[][NB_FOLDER_STATE];

	static LRESULT CALLBACK scintillaStatic_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	LRESULT scintillaNew_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	SCINTILLA_FUNC _pScintillaFunc = nullptr;
	SCINTILLA_PTR  _pScintillaPtr = nullptr;
	static WNDPROC _scintillaDefaultProc;
	CallWindowProcFunc _callWindowProc = nullptr;
	BufferID attachDefaultDoc();

	//Store the current buffer so it can be retrieved later
	BufferID _currentBufferID = nullptr;
	Buffer * _currentBuffer = nullptr;

	int _codepage = CP_ACP;
	bool _wrapRestoreNeeded = false;
	bool _positionRestoreNeeded = false;
	uint32_t _restorePositionRetryCount = 0;

	typedef std::unordered_map<int, Style> StyleMap;
	typedef std::unordered_map<BufferID, StyleMap*> BufferStyleMap;
	BufferStyleMap _hotspotStyles;

	intptr_t _beginSelectPosition = -1;

	static std::string _defaultCharList;

//Lexers and Styling
	void restyleBuffer();
	const char * getCompleteKeywordList(std::basic_string<char> & kwl, LangType langType, int keywordIndex);
	void setKeywords(LangType langType, const char *keywords, int index);
	void setLexer(LangType langID, int whichList);
	bool setLexerFromLangID(int langID);
	void makeStyle(LangType langType, const TCHAR **keywordArray = NULL);
	void setStyle(Style styleToSet);			//NOT by reference	(style edited)
	void setSpecialStyle(const Style & styleToSet);	//by reference
	void setSpecialIndicator(const Style & styleToSet) {
		execute(SCI_INDICSETFORE, styleToSet._styleID, styleToSet._bgColor);
	};

	//Complex lexers (same lexer, different language)
	void setXmlLexer(LangType type);
 	void setCppLexer(LangType type);
	void setJsLexer();
	void setTclLexer();
    void setObjCLexer(LangType type);
	void setUserLexer(const TCHAR *userLangName = NULL);
	void setExternalLexer(LangType typeDoc);
	void setEmbeddedJSLexer();
    void setEmbeddedPhpLexer();
    void setEmbeddedAspLexer();
	void setJsonLexer();
	void setTypeScriptLexer();

	//Simple lexers
	void setCssLexer() {
		setLexer(L_CSS, LIST_0 | LIST_1 | LIST_4 | LIST_6);
	};

	void setLuaLexer() {
		setLexer(L_LUA, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setMakefileLexer() {
		setLexer(L_MAKEFILE, LIST_NONE);
	};

	void setIniLexer() {
		setLexer(L_INI, LIST_NONE);
		execute(SCI_STYLESETEOLFILLED, SCE_PROPS_SECTION, true);
	};


	void setSqlLexer() {
		const bool kbBackSlash = NppParameters::getInstance().getNppGUI()._backSlashIsEscapeCharacterForSql;
		setLexer(L_SQL, LIST_0 | LIST_1 | LIST_4);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("sql.backslash.escapes"), reinterpret_cast<LPARAM>(kbBackSlash ? "1" : "0"));
	};

	void setBashLexer() {
		setLexer(L_BASH, LIST_0);
	};

	void setVBLexer() {
		setLexer(L_VB, LIST_0);
	};

	void setPascalLexer() {
		setLexer(L_PASCAL, LIST_0);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setPerlLexer() {
		setLexer(L_PERL, LIST_0);
	};

	void setPythonLexer() {
		setLexer(L_PYTHON, LIST_0 | LIST_1);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.quotes.python"), reinterpret_cast<LPARAM>("1"));
	};

	void setBatchLexer() {
		setLexer(L_BATCH, LIST_0);
	};

	void setTeXLexer() {
		for (int i = 0 ; i < 4 ; ++i)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(TEXT("")));
		setLexer(L_TEX, LIST_NONE);
	};

	void setNsisLexer() {
		setLexer(L_NSIS, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setFortranLexer() {
		setLexer(L_FORTRAN, LIST_0 | LIST_1 | LIST_2);
	};

	void setFortran77Lexer() {
		setLexer(L_FORTRAN_77, LIST_0 | LIST_1 | LIST_2);
	};

	void setLispLexer(){
		setLexer(L_LISP, LIST_0 | LIST_1);
	};

	void setSchemeLexer(){
		setLexer(L_SCHEME, LIST_0 | LIST_1);
	};

	void setAsmLexer(){
		setLexer(L_ASM, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};

	void setDiffLexer(){
		setLexer(L_DIFF, LIST_NONE);
	};

	void setPropsLexer(){
		setLexer(L_PROPS, LIST_NONE);
	};

	void setPostscriptLexer(){
		setLexer(L_PS, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setRubyLexer(){
		setLexer(L_RUBY, LIST_0);
		execute(SCI_STYLESETEOLFILLED, SCE_RB_POD, true);
	};

	void setSmalltalkLexer(){
		setLexer(L_SMALLTALK, LIST_0);
	};

	void setVhdlLexer(){
		setLexer(L_VHDL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};

	void setKixLexer(){
		setLexer(L_KIX, LIST_0 | LIST_1 | LIST_2);
	};

	void setAutoItLexer(){
		setLexer(L_AU3, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setCamlLexer(){
		setLexer(L_CAML, LIST_0 | LIST_1 | LIST_2);
	};

	void setAdaLexer(){
		setLexer(L_ADA, LIST_0);
	};

	void setVerilogLexer(){
		setLexer(L_VERILOG, LIST_0 | LIST_1);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setMatlabLexer(){
		setLexer(L_MATLAB, LIST_0);
	};

	void setHaskellLexer(){
		setLexer(L_HASKELL, LIST_0);
	};

	void setInnoLexer() {
		setLexer(L_INNO, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};

	void setCmakeLexer() {
		setLexer(L_CMAKE, LIST_0 | LIST_1 | LIST_2);
	};

	void setYamlLexer() {
		setLexer(L_YAML, LIST_0);
	};

    //--------------------

    void setCobolLexer() {
		setLexer(L_COBOL, LIST_0 | LIST_1 | LIST_2);
	};
    void setGui4CliLexer() {
		setLexer(L_GUI4CLI, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4);
	};
    void setDLexer() {
		setLexer(L_D, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};
    void setPowerShellLexer() {
		setLexer(L_POWERSHELL, LIST_0 | LIST_1 | LIST_2 | LIST_5);
	};
    void setRLexer() {
		setLexer(L_R, LIST_0 | LIST_1 | LIST_2);
	};

    void setCoffeeScriptLexer() {
		setLexer(L_COFFEESCRIPT, LIST_0 | LIST_1 | LIST_2  | LIST_3);
	};

	void setBaanCLexer() {
		setLexer(L_BAANC, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6 | LIST_7 | LIST_8);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.baan.styling.within.preprocessor"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$:"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.syntax.based"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.keywords.based"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.sections"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.baan.inner.level"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_STYLESETEOLFILLED, SCE_BAAN_STRINGEOL, true);
	};

	void setSrecLexer() {
		setLexer(L_SREC, LIST_NONE);
	};

	void setIHexLexer() {
		setLexer(L_IHEX, LIST_NONE);
	};

	void setTEHexLexer() {
		setLexer(L_TEHEX, LIST_NONE);
	};

	void setAsn1Lexer() {
		setLexer(L_ASN1, LIST_0 | LIST_1 | LIST_2 | LIST_3); 
	};

	void setAVSLexer() {
		setLexer(L_AVS, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"));
	};

	void setBlitzBasicLexer() {
		setLexer(L_BLITZBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); 
	};

	void setPureBasicLexer() {
		setLexer(L_PUREBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); 
	};

	void setFreeBasicLexer() {
		setLexer(L_FREEBASIC, LIST_0 | LIST_1 | LIST_2 | LIST_3); 
	};

	void setCsoundLexer() {
		setLexer(L_CSOUND, LIST_0 | LIST_1 | LIST_2);
		execute(SCI_STYLESETEOLFILLED, SCE_CSOUND_STRINGEOL, true);
	};

	void setErlangLexer() {
		setLexer(L_ERLANG, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5); 
	};

	void setESCRIPTLexer() {
		setLexer(L_ESCRIPT, LIST_0 | LIST_1 | LIST_2); 
	};

	void setForthLexer() {
		setLexer(L_FORTH, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%-"));
	};

	void setLatexLexer() {
		setLexer(L_LATEX, LIST_NONE); 
	};

	void setMMIXALLexer() {
		setLexer(L_MMIXAL, LIST_0 | LIST_1 | LIST_2); 
	};

	void setNimrodLexer() {
		setLexer(L_NIM, LIST_0);
	};

	void setNncrontabLexer() {
		setLexer(L_NNCRONTAB, LIST_0 | LIST_1 | LIST_2); 
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789%-"));
	};

	void setOScriptLexer() {
		setLexer(L_OSCRIPT, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$"));
	};

	void setREBOLLexer() {
		setLexer(L_REBOL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?!.’+-*&|=_~"));
	};

	void setRegistryLexer() {
		setLexer(L_REGISTRY, LIST_NONE); 
	};

	void setRustLexer() {
		setLexer(L_RUST, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6); 
		execute(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#"));
	};

	void setSpiceLexer() {
		setLexer(L_SPICE, LIST_0 | LIST_1 | LIST_2); 
	};

	void setTxt2tagsLexer() {
		setLexer(L_TXT2TAGS, LIST_NONE); 
	};

	void setVisualPrologLexer() {
		setLexer(L_VISUALPROLOG, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	}

    //--------------------

	void setSearchResultLexer() {
		if (execute(SCI_GETLEXER) == SCLEX_SEARCHRESULT)
		{
			return;
		}
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_FILE_HEADER, true);
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_SEARCH_HEADER, true);
		setLexer(L_SEARCHRESULT, LIST_NONE);
	};

	bool isNeededFolderMarge(LangType typeDoc) const {
		switch (typeDoc)
		{
			case L_ASCII:
			case L_BATCH:
			case L_TEXT:
			case L_MAKEFILE:
			case L_ASM:
			case L_HASKELL:
			case L_PROPS:
			case L_SMALLTALK:
			case L_KIX:
			case L_ADA:
				return false;
			default:
				return true;
		}
	};
//END: Lexers and Styling

    void defineMarker(int marker, int markerType, COLORREF fore, COLORREF back, COLORREF foreActive) {
	    execute(SCI_MARKERDEFINE, marker, markerType);
	    execute(SCI_MARKERSETFORE, marker, fore);
	    execute(SCI_MARKERSETBACK, marker, back);
		execute(SCI_MARKERSETBACKSELECTED, marker, foreActive);
	};

	int codepage2CharSet() const {
		switch (_codepage)
		{
			case CP_CHINESE_TRADITIONAL : return SC_CHARSET_CHINESEBIG5;
			case CP_CHINESE_SIMPLIFIED : return SC_CHARSET_GB2312;
			case CP_KOREAN : return SC_CHARSET_HANGUL;
			case CP_JAPANESE : return SC_CHARSET_SHIFTJIS;
			case CP_GREEK : return SC_CHARSET_GREEK;
			default : return 0;
		}
	};

	std::pair<size_t, size_t> getWordRange();
	bool expandWordSelection();
	void getFoldColor(COLORREF& fgColor, COLORREF& bgColor, COLORREF& activeFgColor);
};

