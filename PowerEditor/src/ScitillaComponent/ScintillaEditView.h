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

#ifndef SCINTILLA_EDIT_VIEW_H
#define SCINTILLA_EDIT_VIEW_H

#include <vector>
#include "Window.h"
#include "Scintilla.h"
#include "ScintillaRef.h"
#include "SciLexer.h"
#include "Buffer.h"
#include "colors.h"
#include "SysMsg.h"
#include "UserDefineDialog.h"
#include "resource.h"

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

typedef int (* SCINTILLA_FUNC) (void*, int, int, int);
typedef void * SCINTILLA_PTR;
typedef std::vector<Buffer> buf_vec_t;

#define WM_DOCK_USERDEFINE_DLG      (SCINTILLA_USER + 1)
#define WM_UNDOCK_USERDEFINE_DLG    (SCINTILLA_USER + 2)
#define WM_CLOSE_USERDEFINE_DLG		(SCINTILLA_USER + 3)
#define WM_REMOVE_USERLANG		    (SCINTILLA_USER + 4)
#define WM_RENAME_USERLANG			(SCINTILLA_USER + 5)
#define WM_REPLACEALL_INOPENEDDOC	(SCINTILLA_USER + 6)
#define WM_FINDALL_INOPENEDDOC  	(SCINTILLA_USER + 7)
#define WM_DOOPEN				  	(SCINTILLA_USER + 8)
#define WM_FINDINFILES			  	(SCINTILLA_USER + 9)

#define LINEDRAW_FONT  "LINEDRAW.TTF"

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

const bool fold_uncollapse = true;
const bool fold_collapse = false;

const bool UPPERCASE = true;
const bool LOWERCASE = false;

typedef vector<pair<int, int> > ColumnModeInfo;
const unsigned char MASK_FORMAT = 0x03;
const unsigned char MASK_ZERO_LEADING = 0x04;
const unsigned char BASE_10 = 0x00; // Dec
const unsigned char BASE_16 = 0x01; // Hex
const unsigned char BASE_08 = 0x02; // Oct
const unsigned char BASE_02 = 0x03; // Bin

static int getNbChiffre(int aNum, int base)
{
	int nbChiffre = 1;
	int diviseur = base;
	
	for (;;)
	{
		int result = aNum / diviseur;
		if (!result)
			break;
		else
		{
			diviseur *= base;
			nbChiffre++;
		}
	}
	if ((base == 16) && (nbChiffre % 2 != 0))
		nbChiffre += 1;

	return nbChiffre;
};

char * int2str(char *str, int strLen, int number, int base, int nbChiffre, bool isZeroLeading);

class ScintillaEditView : public Window
{
	friend class Notepad_plus;
	friend class Finder;
public:
	ScintillaEditView()
		: Window(), _pScintillaFunc(NULL),_pScintillaPtr(NULL),
		  _currentIndex(0), _folderStyle(FOLDER_STYLE_BOX), _maxNbDigit(_MARGE_LINENUMBER_NB_CHIFFRE)
	{
		++_refCount;
	};

	virtual ~ScintillaEditView()
	{
		--_refCount;

		if ((!_refCount)&&(_hLib))
		{
			::FreeLibrary(_hLib);
		}
	};
	virtual void destroy()
	{
		removeAllUnusedDocs();
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
	};

	virtual void init(HINSTANCE hInst, HWND hPere);

	LRESULT execute(UINT Msg, WPARAM wParam=0, LPARAM lParam=0) const {
		return _pScintillaFunc(_pScintillaPtr, static_cast<int>(Msg), static_cast<int>(wParam), static_cast<int>(lParam));
	};
	
	void defineDocType(LangType typeDoc);

    bool setCurrentDocType(LangType typeDoc) {
        if ((_buffers[_currentIndex]._lang == typeDoc) && (typeDoc != L_USER))
            return false;
		if (typeDoc == L_USER)
			_buffers[_currentIndex]._userLangExt[0] = '\0';

        _buffers[_currentIndex]._lang = typeDoc;
        defineDocType(typeDoc);
		return true;
    };

	void setCurrentDocUserType(const char *userLangName) {
		strcpy(_buffers[_currentIndex]._userLangExt, userLangName);
        _buffers[_currentIndex]._lang = L_USER;
        defineDocType(L_USER);
    };

	char * attatchDefaultDoc(int nb);

	int findDocIndexByName(const char *fn) const;
	char * activateDocAt(int index);
	char * createNewDoc(const char *fn);
	char * createNewDoc(int nbNew);
	int getCurrentDocIndex() const {return _currentIndex;};
	const char * getCurrentTitle() const {return _buffers[_currentIndex]._fullPathName;};
	int setCurrentTitle(const char *fn) {
		_buffers[_currentIndex].setFileName(fn);
		defineDocType(_buffers[_currentIndex]._lang);
		return _currentIndex;
	};
	int closeCurrentDoc(int & i2Activate);
    void closeDocAt(int i2Close);

	void removeAllUnusedDocs();

	void getText(char *dest, int start, int end) const;

	void setCurrentDocState(bool isDirty) {
		_buffers[_currentIndex]._isDirty = isDirty;
	};
	
	bool isCurrentDocDirty() const {
		return _buffers[_currentIndex]._isDirty;
	};

    void setCurrentDocReadOnly(bool isReadOnly) {
        _buffers[_currentIndex]._isReadOnly = isReadOnly;
		execute(SCI_SETREADONLY, isReadOnly);
    };

	bool setCurrentDocReadOnlyByUser(bool ro) {
		execute(SCI_SETREADONLY, ro);
		return _buffers[_currentIndex].setReadOnly(ro);
	};

	void updateCurrentDocSysReadOnlyStat() {
		_buffers[_currentIndex].checkIfReadOnlyFile();
	};

    bool isCurrentBufReadOnly() const {
		return _buffers[_currentIndex].isReadOnly();
	};

	bool isCurrentBufSysReadOnly() const {
		return _buffers[_currentIndex].isSystemReadOnly();
	};

	bool isCurrentBufUserReadOnly() const {
		return _buffers[_currentIndex].isUserReadOnly();
	};

	bool isAllDocsClean() const {
		for (int i = 0 ; i < static_cast<int>(_buffers.size()) ; i++)
			if (_buffers[i]._isDirty)
				return false;
		return true;
	};

	size_t getNbDoc() const {
		return _buffers.size();
	};

	void saveCurrentPos();
	void restoreCurrentPos();
	bool needRestoreFromWrap() {
		return _wrapRestoreNeeded;
	}
	void restoreFromWrap();


	Buffer & getBufferAt(size_t index) {
		if (index >= _buffers.size())
			throw int(index);
		return _buffers[index];
	};

	void updateCurrentBufTimeStamp() {
		_buffers[_currentIndex].updatTimeStamp();
	};

	int getCurrentDocLen() const {
		return int(execute(SCI_GETLENGTH));
	};

	CharacterRange getSelection() const {
		CharacterRange crange;
		crange.cpMin = long(execute(SCI_GETSELECTIONSTART));
		crange.cpMax = long(execute(SCI_GETSELECTIONEND));
		return crange;
	};

	void getWordToCurrentPos(char *str, int strLen) const {
		int caretPos = execute(SCI_GETCURRENTPOS);
		int startPos = static_cast<int>(execute(SCI_WORDSTARTPOSITION, caretPos, true));
		
		str[0] = '\0';
		if ((caretPos - startPos) < strLen)
			getText(str, startPos, caretPos);
	};

    LangType getCurrentDocType() const {
        return _buffers[_currentIndex]._lang;
    };

    void doUserDefineDlg(bool willBeShown = true, bool isRTL = false) {
        _userDefineDlg.doDialog(willBeShown, isRTL);
    };

    static UserDefineDialog * getUserDefineDlg() {return &_userDefineDlg;};

    void setCaretColorWidth(int color, int width = 1) const {
        execute(SCI_SETCARETFORE, color);
        execute(SCI_SETCARETWIDTH, width);
    };

	// if we use this method, it must be via the 
	// gotoAnotherView or cloneToAnotherEditView
	// So the ref counter of document should increase
    int addBuffer(Buffer & buffer) {
        _buffers.push_back(buffer);
		execute(SCI_ADDREFDOCUMENT, 0, buffer._doc);
        return (int(_buffers.size()) - 1);
    };

    Buffer & getCurrentBuffer() {
        return getBufferAt(_currentIndex);
    };

	void beSwitched() {
		_userDefineDlg.setScintilla(this);
	};

    //Marge memeber and method
    static const int _SC_MARGE_LINENUMBER;
    static const int _SC_MARGE_SYBOLE;
    static const int _SC_MARGE_FOLDER;

    static const int _MARGE_LINENUMBER_NB_CHIFFRE;

    void showMargin(int witchMarge, bool willBeShowed = true) {
        if (witchMarge == _SC_MARGE_LINENUMBER)
            setLineNumberWidth(willBeShowed);
        else
            execute(SCI_SETMARGINWIDTHN, witchMarge, willBeShowed?14:0);
    };

    bool hasMarginShowed(int witchMarge) {
		return (execute(SCI_GETMARGINWIDTHN, witchMarge, 0) != 0);
    };
    
    void marginClick(int position, int modifiers);

    void setMakerStyle(folderStyle style) {
        if (_folderStyle == style)
            return;
        _folderStyle = style;
        for (int i = 0 ; i < NB_FOLDER_STATE ; i++)
            defineMarker(_markersArray[FOLDER_TYPE][i], _markersArray[style][i], white, grey);
    };

    folderStyle getFolderStyle() {return _folderStyle;};

	void showWSAndTab(bool willBeShowed = true) {
		execute(SCI_SETVIEWWS, willBeShowed?SCWS_VISIBLEALWAYS:SCWS_INVISIBLE);
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

	void showIndentGuideLine(bool willBeShowed = true) {
		execute(SCI_SETINDENTATIONGUIDES, (WPARAM)willBeShowed);
	};

	bool isShownIndentGuide() const {
		return (execute(SCI_GETINDENTATIONGUIDES) != 0);
	};

    void wrap(bool willBeWrapped = true) {
        execute(SCI_SETWRAPMODE, (WPARAM)willBeWrapped);
    };

    bool isWrap() const {
        return (execute(SCI_GETWRAPMODE) == SC_WRAP_WORD);
    };

	bool isWrapSymbolVisible() const {
		return (execute(SCI_GETWRAPVISUALFLAGS) != SC_WRAPVISUALFLAG_NONE);
	};

    void showWrapSymbol(bool willBeShown = true) {
		execute(SCI_SETWRAPVISUALFLAGSLOCATION, SC_WRAPVISUALFLAGLOC_END_BY_TEXT);
		execute(SCI_SETWRAPVISUALFLAGS, willBeShown?SC_WRAPVISUALFLAG_END:SC_WRAPVISUALFLAG_NONE);
    };

    void sortBuffer(int destIndex, int scrIndex) {
		// Do nothing if there's no change of the position
		if (scrIndex == destIndex)
			return;

        Buffer buf2Insert = _buffers.at(scrIndex);
        _buffers.erase(_buffers.begin() + scrIndex);
        _buffers.insert(_buffers.begin() + destIndex, buf2Insert);
    };

	int getSelectedTextCount() {
		CharacterRange range = getSelection();
		return (range.cpMax - range.cpMin);
	};

	char * getSelectedText(char * txt, int size, bool expand=false) {
		CharacterRange range = getSelection();
		if (size <= (range.cpMax - range.cpMin))
			return NULL;
		if (expand && range.cpMax == range.cpMin)
		{
			expandWordSelection();
			range = getSelection();
			if (size <= (range.cpMax - range.cpMin))
				return NULL;
		}
		getText(txt, range.cpMin, range.cpMax);
		return txt;
	};

	long getCurrentLineNumber()const {
		return long(execute(SCI_LINEFROMPOSITION, execute(SCI_GETCURRENTPOS)));
	};

	long getNbLine() const {
		int endPos = execute(SCI_GETLENGTH);
		return execute(SCI_LINEFROMPOSITION, endPos);
	};

	long getCurrentXOffset()const{
		return long(execute(SCI_GETXOFFSET));
	};

	void setCurrentXOffset(long xOffset){
		execute(SCI_SETXOFFSET,xOffset);
	};

	void scroll(int column, int line){
		execute(SCI_LINESCROLL, column, line);
	};

	long getCurrentPointX()const{
		return long (execute(SCI_POINTXFROMPOSITION, 0, execute(SCI_GETCURRENTPOS)));
	};

	long getCurrentPointY()const{
		return long (execute(SCI_POINTYFROMPOSITION, 0, execute(SCI_GETCURRENTPOS)));
	};

	long getTextHeight()const{
		return long(execute(SCI_TEXTHEIGHT));
		};
	
	void gotoLine(int line){
		if (line < execute(SCI_GETLINECOUNT))
			execute(SCI_GOTOLINE,line);
	};

	long getCurrentColumnNumber() const {
        return long(execute(SCI_GETCOLUMN, execute(SCI_GETCURRENTPOS)));
    };

	long getSelectedByteNumber() const {
		long start = long(execute(SCI_GETSELECTIONSTART));
		long end = long(execute(SCI_GETSELECTIONEND));
		return (start < end)?end-start:start-end;
    };

	long getLineLength(int line) const {
		return long(execute(SCI_GETLINEENDPOSITION, line) - execute(SCI_POSITIONFROMLINE, line));
	};

	long getLineIndent(int line) const {
		return long(execute(SCI_GETLINEINDENTATION, line));
	};

	void setLineIndent(int line, int indent) const;

    void setLineNumberWidth(bool willBeShowed = true) {
        // The 4 here allows for spacing: 1 poxel on left and 3 on right.
        int pixelWidth = int((willBeShowed)?(8 + _maxNbDigit * execute(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"8")):0);
        execute(SCI_SETMARGINWIDTHN, 0, pixelWidth);
    };

    void setCurrentIndex(int index2Set) {_currentIndex = index2Set;};

	void setCurrentLineHiLiting(bool isHiliting, COLORREF bgColor) const {
		execute(SCI_SETCARETLINEVISIBLE, isHiliting);
		if (!isHiliting)
			return;
		execute(SCI_SETCARETLINEBACK, bgColor);
	};

	bool isCurrentLineHiLiting() const {
		return (execute(SCI_GETCARETLINEVISIBLE) != 0);
	};

	inline void makeStyle(const char *lexerName, const char **keywordArray = NULL);

	void performGlobalStyles();

	void expand(int &line, bool doExpand, bool force = false, int visLevels = 0, int level = -1);
	void removeUserLang(const char *name) {
		for (int i = 0 ; i < int(_buffers.size()) ; i++)
		{
			if ((_buffers[i]._lang == L_USER) && (!strcmp(name, _buffers[i]._userLangExt)))
			{
				_buffers[i]._userLangExt[0] = '\0';
			}
		}
	};
	void renameUserLang(const char *oldName, const char *newName) {
		for (int i = 0 ; i < int(_buffers.size()) ; i++)
		{
			if ((_buffers[i]._lang == L_USER) && (!strcmp(oldName, _buffers[i]._userLangExt)))
			{
				strcpy(_buffers[i]._userLangExt, newName);
			}
		}
	};

	
		
	void currentLineUp() const {
		int currentLine = getCurrentLineNumber();
		if (currentLine != 0)
		{
			execute(SCI_BEGINUNDOACTION);
			currentLine--;
			execute(SCI_LINETRANSPOSE);
			execute(SCI_GOTOLINE, currentLine);
			execute(SCI_ENDUNDOACTION);
		}
	};

	void currentLineDown() const {
		

		int currentLine = getCurrentLineNumber();
		if (currentLine != (execute(SCI_GETLINECOUNT) - 1))
		{
			execute(SCI_BEGINUNDOACTION);
			currentLine++;
			execute(SCI_GOTOLINE, currentLine);
			execute(SCI_LINETRANSPOSE);
			execute(SCI_ENDUNDOACTION);
		}
	};

	void convertSelectedTextTo(bool Case);

    void convertSelectedTextToLowerCase() {
		// if system is w2k or xp
		if ((NppParameters::getInstance())->isTransparentAvailable())
			convertSelectedTextTo(LOWERCASE);
		else
			execute(SCI_LOWERCASE);
	};

    void convertSelectedTextToUpperCase() {
		// if system is w2k or xp
		if ((NppParameters::getInstance())->isTransparentAvailable())
			convertSelectedTextTo(UPPERCASE);
		else
			execute(SCI_UPPERCASE);
	};
    
	void collapse(int level2Collapse, bool mode);
	void foldAll(bool mode);
	void foldCurrentPos(bool mode);
	int getCodpage() const {return _codepage;};

	//int getMaxNbDigit const () {return _maxNbDigit;};

	bool increaseMaxNbDigit(int newValue) {
		if (newValue > _maxNbDigit)
		{
			_maxNbDigit = newValue;
			return true;
		}
		return false;
	};

	int getNextPriorityIndex(int & weight, int heavest) {
		weight = 0;
		if (_buffers.size() <= 0)
			return -1;
		if (_buffers[0]._recentTag < heavest)
			weight = _buffers[0]._recentTag;

		int maxIndex = 0;

		for (size_t i = 1 ; i < _buffers.size() ; i++)
		{
			if ((_buffers[i]._recentTag < heavest) && (weight < _buffers[i]._recentTag))
			{
				weight = _buffers[i]._recentTag;
				maxIndex = i;
			}
		}
		return maxIndex;
	};

	NppParameters * getParameter() {
		return _pParameter;
	};
	
	ColumnModeInfo getColumnModeSelectInfo();

	void columnReplace(ColumnModeInfo & cmi, const char *str);
	void columnReplace(const ColumnModeInfo & cmi, const char ch);
	void columnReplace(ColumnModeInfo & cmi, int initial, int incr, unsigned char format);

	void recalcHorizontalScrollbar();
	void foldChanged(int line, int levelNow, int levelPrev);

protected:
	static HINSTANCE _hLib;
	static int _refCount;
	
    static UserDefineDialog _userDefineDlg;

    static const int _markersArray[][NB_FOLDER_STATE];

	static LRESULT CALLBACK scintillaStatic_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		ScintillaEditView *pScint = (ScintillaEditView *)(::GetWindowLong(hwnd, GWL_USERDATA));
		//
		if (Message == WM_MOUSEWHEEL || Message == WM_MOUSEHWHEEL)
		{
			POINT pt;
			POINTS pts = MAKEPOINTS(lParam);
			POINTSTOPOINT(pt, pts);
			HWND hwndOnMouse = WindowFromPoint(pt);
			ScintillaEditView *pScintillaOnMouse = (ScintillaEditView *)(::GetWindowLong(hwndOnMouse, GWL_USERDATA));
			if (pScintillaOnMouse != pScint)
				return ::SendMessage(hwndOnMouse, Message, wParam, lParam);
		}
		if (pScint)
			return (pScint->scintillaNew_Proc(hwnd, Message, wParam, lParam));
		else
			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		//
	};

	LRESULT scintillaNew_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	SCINTILLA_FUNC _pScintillaFunc;
	SCINTILLA_PTR  _pScintillaPtr;

	// the current active buffer index of _buffers
	int _currentIndex;
	static WNDPROC _scintillaDefaultProc;

	// the list of docs
	buf_vec_t _buffers;

	// For the file nfo
	//int _MSLineDrawFont;

	folderStyle _folderStyle;

    NppParameters *_pParameter;

	int _codepage;
	int _oemCodepage;

	int _maxNbDigit; // For Line Number Marge 

	bool _wrapRestoreNeeded;

	void setStyle(int styleID, COLORREF fgColor, COLORREF bgColor = -1, const char *fontName = NULL, int fontStyle = -1, int fontSize = 0);
	void setSpecialStyle(int styleID, COLORREF fgColor, COLORREF bgColor = -1, const char *fontName = NULL, int fontStyle = -1, int fontSize = 0);
	void setCppLexer(LangType type);
	void setXmlLexer(LangType type);
	void setUserLexer();
	void setUserLexer(const char *userLangName);
	void setExternalLexer(LangType typeDoc);
	void setEmbeddedJSLexer();
    void setPhpEmbeddedLexer();
    void setEmbeddedAspLexer();
	void setCssLexer() {
		setLexer(SCLEX_CSS, L_CSS, "css", LIST_0 | LIST_1);
	};

	void setLuaLexer() {
		setLexer(SCLEX_LUA, L_LUA, "lua", LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setMakefileLexer() {
		execute(SCI_SETLEXER, SCLEX_MAKEFILE);
		makeStyle("makefile");
	};

	void setIniLexer() {
		execute(SCI_SETLEXER, SCLEX_PROPERTIES);
		execute(SCI_STYLESETEOLFILLED, SCE_PROPS_SECTION, true);
		makeStyle("ini");
	};

    void setObjCLexer(LangType type);

	void setSqlLexer() {
		setLexer(SCLEX_SQL, L_SQL, "sql", LIST_0);
	};

	void setBashLexer() {
		setLexer(SCLEX_BASH, L_BASH, "bash", LIST_0);
	};

	void setVBLexer() {
		setLexer(SCLEX_VB, L_VB, "vb", LIST_0);
	};

	void setPascalLexer() {
		setLexer(SCLEX_PASCAL, L_PASCAL, "pascal", LIST_0);
	};

	void setPerlLexer() {
		setLexer(SCLEX_PERL, L_PERL, "perl", LIST_0);
	};

	void setPythonLexer() {
		setLexer(SCLEX_PYTHON, L_PYTHON, "python", LIST_0);
	};

	void setBatchLexer() {
		setLexer(SCLEX_BATCH, L_BATCH, "batch", LIST_0);
	};

	void setTeXLexer() {
		for (int i = 0 ; i < 4 ; i++)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(""));
		setLexer(SCLEX_TEX, L_TEX, "tex", 0);
	};

	void setNsisLexer() {
		setLexer(SCLEX_NSIS, L_NSIS, "nsis", LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setFortranLexer() {
		setLexer(SCLEX_F77, L_FORTRAN, "fortran", LIST_0 | LIST_1 | LIST_2);
	};

	void setLispLexer(){
		setLexer(SCLEX_LISP, L_LISP, "lisp", LIST_0);
	};
	
	void setSchemeLexer(){
		setLexer(SCLEX_LISP, L_SCHEME, "lisp", LIST_0);
	};

	void setAsmLexer(){
		setLexer(SCLEX_ASM, L_ASM, "asm", LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};
	
	void setDiffLexer(){
		setLexer(SCLEX_DIFF, L_DIFF, "diff", LIST_NONE);
	};
	
	void setPropsLexer(){
		setLexer(SCLEX_PROPERTIES, L_PROPS, "props", LIST_NONE);
	};
	
	void setPostscriptLexer(){
		setLexer(SCLEX_PS, L_PS, "postscript", LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};
	
	void setRubyLexer(){
		setLexer(SCLEX_RUBY, L_RUBY, "ruby", LIST_0);
		execute(SCI_STYLESETEOLFILLED, SCE_RB_POD, true);
	};
	
	void setSmalltalkLexer(){
		setLexer(SCLEX_SMALLTALK, L_SMALLTALK, "smalltalk", LIST_0);
	};
	
	void setVhdlLexer(){
		setLexer(SCLEX_VHDL, L_VHDL, "vhdl", LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};
    
	void setKixLexer(){
		setLexer(SCLEX_KIX, L_KIX, "kix", LIST_0 | LIST_1 | LIST_2);
	};
	
	void setAutoItLexer(){
		setLexer(SCLEX_AU3, L_AU3, "autoit", LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};

	void setCamlLexer(){
		setLexer(SCLEX_CAML, L_CAML, "caml", LIST_0 | LIST_1 | LIST_2);
	};

	void setAdaLexer(){
		setLexer(SCLEX_ADA, L_ADA, "ada", LIST_0);
	};
	
	void setVerilogLexer(){
		setLexer(SCLEX_VERILOG, L_VERILOG, "verilog", LIST_0 | LIST_1);
	};

	void setMatlabLexer(){
		setLexer(SCLEX_MATLAB, L_MATLAB, "matlab", LIST_0);
	};

	void setHaskellLexer(){
		setLexer(SCLEX_HASKELL, L_HASKELL, "haskell", LIST_0);
	};

	void setInnoLexer() {
		setLexer(SCLEX_INNOSETUP, L_INNO, "inno", LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};
	
	void setCmakeLexer() {
		setLexer(SCLEX_CMAKE, L_CMAKE, "cmake", LIST_0 | LIST_1 | LIST_2);
	};

	void setYamlLexer() {
		setLexer(SCLEX_YAML, L_YAML, "yaml", LIST_0);
	};

	void setSearchResultLexer() {
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_HEARDER, true);
		setLexer(SCLEX_SEARCHRESULT, L_SEARCHRESULT, "searchResult", LIST_1 | LIST_2 | LIST_3);
	};

    void defineMarker(int marker, int markerType, COLORREF fore, COLORREF back) {
	    execute(SCI_MARKERDEFINE, marker, markerType);
	    execute(SCI_MARKERSETFORE, marker, fore);
	    execute(SCI_MARKERSETBACK, marker, back);
    };

	bool isNeededFolderMarge(LangType typeDoc) const {
		switch (typeDoc)
		{
			case L_NFO:
			case L_BATCH:
			case L_TXT:
			case L_MAKEFILE:
            case L_SQL:
			case L_ASM:
			//case L_TEX:
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
	bool isCJK() const {
		return ((_codepage == CP_CHINESE_TRADITIONAL) || (_codepage == CP_CHINESE_SIMPLIFIED) || 
			    (_codepage == CP_JAPANESE) || (_codepage == CP_KOREAN) || (_codepage == CP_GREEK));
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
	
	const char * getCompleteKeywordList(std::string & kwl, LangType langType, int keywordIndex);
	void setKeywords(LangType langType, const char *keywords, int index);
	void setLexer(int lexerID, LangType langType, const char *lexerName, int whichList);

	bool expandWordSelection();
	void arrangeBuffers(UINT nItems, UINT *items);
};

#endif //SCINTILLA_EDIT_VIEW_H
