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

#define WM_DOCK_USERDEFINE_DLG      (SCINTILLA_USER + 1)
#define WM_UNDOCK_USERDEFINE_DLG    (SCINTILLA_USER + 2)
#define WM_CLOSE_USERDEFINE_DLG		(SCINTILLA_USER + 3)
#define WM_REMOVE_USERLANG		    (SCINTILLA_USER + 4)
#define WM_RENAME_USERLANG			(SCINTILLA_USER + 5)
#define WM_REPLACEALL_INOPENEDDOC	(SCINTILLA_USER + 6)
#define WM_FINDALL_INOPENEDDOC  	(SCINTILLA_USER + 7)
#define WM_DOOPEN				  	(SCINTILLA_USER + 8)
#define WM_FINDINFILES			  	(SCINTILLA_USER + 9)
#define WM_REPLACEINFILES		  	(SCINTILLA_USER + 10)

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
const UCHAR MASK_FORMAT = 0x03;
const UCHAR MASK_ZERO_LEADING = 0x04;
const UCHAR BASE_10 = 0x00; // Dec
const UCHAR BASE_16 = 0x01; // Hex
const UCHAR BASE_08 = 0x02; // Oct
const UCHAR BASE_02 = 0x03; // Bin

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

TCHAR * int2str(TCHAR *str, int strLen, int number, int base, int nbChiffre, bool isZeroLeading);

typedef LRESULT (WINAPI *CallWindowProcFunc) (WNDPROC,HWND,UINT,WPARAM,LPARAM);

struct LanguageName {
	const TCHAR * lexerName;
	const TCHAR * shortName;
	const TCHAR * longName;
	LangType LangID;
	int lexerID;
};

class ScintillaEditView : public Window
{
	friend class Notepad_plus;
	friend class Finder;
public:
	ScintillaEditView()
		: Window(), _pScintillaFunc(NULL),_pScintillaPtr(NULL),
		  _folderStyle(FOLDER_STYLE_BOX), _maxNbDigit(_MARGE_LINENUMBER_NB_CHIFFRE), _wrapRestoreNeeded(false)
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
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
	};

	virtual void init(HINSTANCE hInst, HWND hPere);

	LRESULT execute(UINT Msg, WPARAM wParam=0, LPARAM lParam=0) const {
		return _pScintillaFunc(_pScintillaPtr, static_cast<int>(Msg), static_cast<int>(wParam), static_cast<int>(lParam));
	};
	
	void activateBuffer(BufferID buffer);

	void getText(char *dest, int start, int end) const;
	void getGenericText(TCHAR *dest, int start, int end) const;
	void getGenericText(TCHAR *dest, int start, int end, int *mstart, int *mend) const;
	void insertGenericTextFrom(int position, const TCHAR *text2insert) const;
	void replaceSelWith(const char * replaceText);

	int getSelectedTextCount() {
		CharacterRange range = getSelection();
		return (range.cpMax - range.cpMin);
	};

	char * getSelectedText(char * txt, int size, bool expand = true);
	TCHAR * getGenericSelectedText(TCHAR * txt, int size, bool expand = true);
	int searchInTarget(const TCHAR * Text2Find, int fromPos, int toPos) const;
	void appandGenericText(const TCHAR * text2Append) const;
	void addGenericText(const TCHAR * text2Append) const;
	int replaceTarget(const TCHAR * str2replace, int fromTargetPos = -1, int toTargetPos = -1) const;
	int replaceTargetRegExMode(const TCHAR * re, int fromTargetPos = -1, int toTargetPos = -1) const;
	void showAutoComletion(int lenEntered, const TCHAR * list);
	void showCallTip(int startPos, const TCHAR * def);
	void getLine(int lineNumber, TCHAR * line, int lineBufferLen);
	void addText(int length, const char *buf);

	void saveCurrentPos();
	void restoreCurrentPos();
	void saveCurrentFold();
	void restoreCurrentFold();

	int getCurrentDocLen() const {
		return int(execute(SCI_GETLENGTH));
	};

	CharacterRange getSelection() const {
		CharacterRange crange;
		crange.cpMin = long(execute(SCI_GETSELECTIONSTART));
		crange.cpMax = long(execute(SCI_GETSELECTIONEND));
		return crange;
	};

	void getWordToCurrentPos(TCHAR * str, int strLen) const {
		int caretPos = execute(SCI_GETCURRENTPOS);
		int startPos = static_cast<int>(execute(SCI_WORDSTARTPOSITION, caretPos, true));
		
		str[0] = '\0';
		if ((caretPos - startPos) < strLen)
			getGenericText(str, startPos, caretPos);
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

	long getCurrentLineNumber()const {
		return long(execute(SCI_LINEFROMPOSITION, execute(SCI_GETCURRENTPOS)));
	};

	long lastZeroBasedLineNumber() const {
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

	void setCurrentLineHiLiting(bool isHiliting, COLORREF bgColor) const {
		execute(SCI_SETCARETLINEVISIBLE, isHiliting);
		if (!isHiliting)
			return;
		execute(SCI_SETCARETLINEBACK, bgColor);
	};

	bool isCurrentLineHiLiting() const {
		return (execute(SCI_GETCARETLINEVISIBLE) != 0);
	};

	void performGlobalStyles();

	void expand(int &line, bool doExpand, bool force = false, int visLevels = 0, int level = -1);
		
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
	int getCodepage() const {return _codepage;};

	bool increaseMaxNbDigit(int newValue) {
		if (newValue > _maxNbDigit)
		{
			_maxNbDigit = newValue;
			return true;
		}
		return false;
	};

	NppParameters * getParameter() {
		return _pParameter;
	};
	
	ColumnModeInfo getColumnModeSelectInfo();

	void columnReplace(ColumnModeInfo & cmi, const TCHAR *str);
	void columnReplace(const ColumnModeInfo & cmi, const TCHAR ch);
	void columnReplace(ColumnModeInfo & cmi, int initial, int incr, UCHAR format);

	void foldChanged(int line, int levelNow, int levelPrev);
	void clearIndicator(int indicatorNumber) {
		int docStart = 0;
		int docEnd = getCurrentDocLen();
		execute(SCI_SETINDICATORCURRENT, indicatorNumber);
		execute(SCI_INDICATORCLEARRANGE, docStart, docEnd-docStart);
	};

	static LanguageName ScintillaEditView::langNames[L_EXTERNAL+1];

	void bufferUpdated(Buffer * buffer, int mask);
	BufferID getCurrentBufferID() { return _currentBufferID; };
	Buffer * getCurrentBuffer() { return _currentBuffer; };
	void styleChange();

	void hideLines();

	bool markerMarginClick(int lineNumber);	//true if it did something
	void notifyMarkers(Buffer * buf, bool isHide, int location, bool del);
	void runMarkers(bool doHide, int searchStart, bool endOfDoc, bool doDelete);

	bool isSelecting() const {
		static CharacterRange previousSelRange = getSelection();
		CharacterRange currentSelRange = getSelection();
		
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
	void setHiLiteResultWords(const TCHAR *keywords);

protected:
	static HINSTANCE _hLib;
	static int _refCount;
	
    static UserDefineDialog _userDefineDlg;

    static const int _markersArray[][NB_FOLDER_STATE];

	static LRESULT CALLBACK scintillaStatic_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		ScintillaEditView *pScint = (ScintillaEditView *)(::GetWindowLongPtr(hwnd, GWL_USERDATA));
		//
		if (Message == WM_MOUSEWHEEL || Message == WM_MOUSEHWHEEL)
		{
			POINT pt;
			POINTS pts = MAKEPOINTS(lParam);
			POINTSTOPOINT(pt, pts);
			HWND hwndOnMouse = WindowFromPoint(pt);
			ScintillaEditView *pScintillaOnMouse = (ScintillaEditView *)(::GetWindowLongPtr(hwndOnMouse, GWL_USERDATA));
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

	static WNDPROC _scintillaDefaultProc;
	CallWindowProcFunc _callWindowProc;

	BufferID attachDefaultDoc();

	//Store the current buffer so it can be retrieved later
	BufferID _currentBufferID;
	Buffer * _currentBuffer;

	folderStyle _folderStyle;

    NppParameters *_pParameter;

	int _codepage;
	int _oemCodepage;

	int _maxNbDigit; // For Line Number Marge 

	bool _wrapRestoreNeeded;

//Lexers and Styling
	void defineDocType(LangType typeDoc);	//setup stylers for active document
	void restyleBuffer();
	const char * getCompleteKeywordList(std::basic_string<char> & kwl, LangType langType, int keywordIndex);
	void setKeywords(LangType langType, const char *keywords, int index);
	void setLexer(int lexerID, LangType langType, int whichList);
	inline void makeStyle(LangType langType, const TCHAR **keywordArray = NULL);
	void setStyle(Style styleToSet);			//NOT by reference	(style edited)
	void setSpecialStyle(Style & styleToSet);	//by reference
	void setSpecialIndicator(Style & styleToSet);
	//Complex lexers (same lexer, different language)
	void setXmlLexer(LangType type);
 	void setCppLexer(LangType type);
    void setObjCLexer(LangType type);
	void setUserLexer(const TCHAR *userLangName = NULL);
	void setExternalLexer(LangType typeDoc);
	void setEmbeddedJSLexer();
    void setPhpEmbeddedLexer();
    void setEmbeddedAspLexer();
	//Simple lexers
	void setCssLexer() {
		setLexer(SCLEX_CSS, L_CSS, LIST_0 | LIST_1);
	};

	void setLuaLexer() {
		setLexer(SCLEX_LUA, L_LUA, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setMakefileLexer() {
		execute(SCI_SETLEXER, SCLEX_MAKEFILE);
		makeStyle(L_MAKEFILE);
	};

	void setIniLexer() {
		execute(SCI_SETLEXER, SCLEX_PROPERTIES);
		execute(SCI_STYLESETEOLFILLED, SCE_PROPS_SECTION, true);
		makeStyle(L_INI);
	};


	void setSqlLexer() {
		setLexer(SCLEX_SQL, L_SQL, LIST_0);
	};

	void setBashLexer() {
		setLexer(SCLEX_BASH, L_BASH, LIST_0);
	};

	void setVBLexer() {
		setLexer(SCLEX_VB, L_VB, LIST_0);
	};

	void setPascalLexer() {
		setLexer(SCLEX_PASCAL, L_PASCAL, LIST_0);
	};

	void setPerlLexer() {
		setLexer(SCLEX_PERL, L_PERL, LIST_0);
	};

	void setPythonLexer() {
		setLexer(SCLEX_PYTHON, L_PYTHON, LIST_0);
	};

	void setBatchLexer() {
		setLexer(SCLEX_BATCH, L_BATCH, LIST_0);
	};

	void setTeXLexer() {
		for (int i = 0 ; i < 4 ; i++)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(TEXT("")));
		setLexer(SCLEX_TEX, L_TEX, 0);
	};

	void setNsisLexer() {
		setLexer(SCLEX_NSIS, L_NSIS, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setFortranLexer() {
		setLexer(SCLEX_F77, L_FORTRAN, LIST_0 | LIST_1 | LIST_2);
	};

	void setLispLexer(){
		setLexer(SCLEX_LISP, L_LISP, LIST_0);
	};

	void setSchemeLexer(){
		setLexer(SCLEX_LISP, L_SCHEME, LIST_0);
	};

	void setAsmLexer(){
		setLexer(SCLEX_ASM, L_ASM, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};

	void setDiffLexer(){
		setLexer(SCLEX_DIFF, L_DIFF, LIST_NONE);
	};

	void setPropsLexer(){
		setLexer(SCLEX_PROPERTIES, L_PROPS, LIST_NONE);
	};

	void setPostscriptLexer(){
		setLexer(SCLEX_PS, L_PS, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setRubyLexer(){
		setLexer(SCLEX_RUBY, L_RUBY, LIST_0);
		execute(SCI_STYLESETEOLFILLED, SCE_RB_POD, true);
	};

	void setSmalltalkLexer(){
		setLexer(SCLEX_SMALLTALK, L_SMALLTALK, LIST_0);
	};

	void setVhdlLexer(){
		setLexer(SCLEX_VHDL, L_VHDL, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};

	void setKixLexer(){
		setLexer(SCLEX_KIX, L_KIX, LIST_0 | LIST_1 | LIST_2);
	};

	void setAutoItLexer(){
		setLexer(SCLEX_AU3, L_AU3, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};

	void setCamlLexer(){
		setLexer(SCLEX_CAML, L_CAML, LIST_0 | LIST_1 | LIST_2);
	};

	void setAdaLexer(){
		setLexer(SCLEX_ADA, L_ADA, LIST_0);
	};

	void setVerilogLexer(){
		setLexer(SCLEX_VERILOG, L_VERILOG, LIST_0 | LIST_1);
	};

	void setMatlabLexer(){
		setLexer(SCLEX_MATLAB, L_MATLAB, LIST_0);
	};

	void setHaskellLexer(){
		setLexer(SCLEX_HASKELL, L_HASKELL, LIST_0);
	};

	void setInnoLexer() {
		setLexer(SCLEX_INNOSETUP, L_INNO, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5);
	};

	void setCmakeLexer() {
		setLexer(SCLEX_CMAKE, L_CMAKE, LIST_0 | LIST_1 | LIST_2);
	};

	void setYamlLexer() {
		setLexer(SCLEX_YAML, L_YAML, LIST_0);
	};

	void setSearchResultLexer() {
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_FILE_HEADER, true);
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_SEARCH_HEADER, true);
		setLexer(SCLEX_SEARCHRESULT, L_SEARCHRESULT, 0);
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
//END: Lexers and Styling

    void defineMarker(int marker, int markerType, COLORREF fore, COLORREF back) {
	    execute(SCI_MARKERDEFINE, marker, markerType);
	    execute(SCI_MARKERSETFORE, marker, fore);
	    execute(SCI_MARKERSETBACK, marker, back);
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

	bool expandWordSelection();
};

#endif //SCINTILLA_EDIT_VIEW_H
