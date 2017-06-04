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
	int _selLpos;
	int _selRpos;
	int _order; // 0 based index
	bool _direction; // L2R or R2L
	int _nbVirtualCaretSpc;
	int _nbVirtualAnchorSpc;

	ColumnModeInfo() : _selLpos(0), _selRpos(0), _order(-1), _direction(L2R), _nbVirtualAnchorSpc(0), _nbVirtualCaretSpc(0){};
	ColumnModeInfo(int lPos, int rPos, int order, bool dir = L2R, int vAnchorNbSpc = 0, int vCaretNbSpc = 0)
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

struct LanguageName {
	const TCHAR * lexerName;
	const TCHAR * shortName;
	const TCHAR * longName;
	LangType LangID;
	int lexerID;
};

class ISorter;

#define SCI_UNUSED 0

typedef struct {
	unsigned char ch;
	unsigned char style;
} Cell;

typedef COLORREF Colour;

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

		if ((!_refCount)&&(_hLib))
		{
			::FreeLibrary(_hLib);

			for (BufferStyleMap::iterator it(_hotspotStyles.begin()); it != _hotspotStyles.end(); ++it )
			{
				for (StyleMap::iterator it2(it->second->begin()) ; it2 != it->second->end() ; ++it2)
				{
					if (it2->second._fontName != NULL)
						delete [] it2->second._fontName;
				}
				delete it->second;
			}
		}
	};

	virtual void destroy()
	{
		::DestroyWindow(_hSelf);
		_hSelf = NULL;
	};

	virtual void init(HINSTANCE hInst, HWND hPere);

	template<typename T = uptr_t, typename U = sptr_t>
	inline sptr_t execute(unsigned int Msg, T wParam = 0, U lParam = 0) const {
		return _pScintillaFunc(_pScintillaPtr, Msg, (uptr_t)wParam, (sptr_t)lParam);
	};

	void activateBuffer(BufferID buffer);

	void getCurrentFoldStates(std::vector<size_t> & lineStateVector);
	void syncFoldStateWith(const std::vector<size_t> & lineStateVectorNew);

	void getText(char *dest, size_t start, size_t end) const;
	void getGenericText(TCHAR *dest, size_t destlen, size_t start, size_t end) const;
	void getGenericText(TCHAR *dest, size_t deslen, int start, int end, int *mstart, int *mend) const;
	generic_string getGenericTextAsString(size_t start, size_t end) const;
	void insertGenericTextFrom(size_t position, const TCHAR *text2insert) const;
	void replaceSelWith(const char * replaceText);

	int getSelectedTextCount() {
		CharacterRange range = getSelection();
		return (range.cpMax - range.cpMin);
	};

	void getVisibleStartAndEndPosition(int * startPos, int * endPos);
    char * getWordFromRange(char * txt, int size, int pos1, int pos2);
	char * getSelectedText(char * txt, int size, bool expand = true);
    char * getWordOnCaretPos(char * txt, int size);
    TCHAR * getGenericWordOnCaretPos(TCHAR * txt, int size);
	TCHAR * getGenericSelectedText(TCHAR * txt, int size, bool expand = true);
	int searchInTarget(const TCHAR * Text2Find, size_t lenOfText2Find, size_t fromPos, size_t toPos) const;
	void appandGenericText(const TCHAR * text2Append) const;
	void addGenericText(const TCHAR * text2Append) const;
	void addGenericText(const TCHAR * text2Append, long *mstart, long *mend) const;
	int replaceTarget(const TCHAR * str2replace, int fromTargetPos = -1, int toTargetPos = -1) const;
	int replaceTargetRegExMode(const TCHAR * re, int fromTargetPos = -1, int toTargetPos = -1) const;
	void showAutoComletion(size_t lenEntered, const TCHAR * list);
	void showCallTip(int startPos, const TCHAR * def);
	generic_string getLine(size_t lineNumber);
	void getLine(size_t lineNumber, TCHAR * line, int lineBufferLen);
	void addText(size_t length, const char *buf);

	void insertNewLineAboveCurrentLine();
	void insertNewLineBelowCurrentLine();

	void saveCurrentPos();
	void restoreCurrentPos();

	void beginOrEndSelect();
	bool beginEndSelectedIsStarted() const {
		return _beginSelectPosition != -1;
	};

	CharacterRange getSelection() const {
		CharacterRange crange;
		crange.cpMin = long(GetSelectionStart());
		crange.cpMax = long(GetSelectionEnd());
		return crange;
	};

	void getWordToCurrentPos(TCHAR * str, int strLen) const {
		auto caretPos = GetCurrentPos();
		auto startPos = WordStartPosition(caretPos, true);

		str[0] = '\0';
		if ((caretPos - startPos) < strLen)
			getGenericText(str, strLen, startPos, caretPos);
	};

    void doUserDefineDlg(bool willBeShown = true, bool isRTL = false) {
        _userDefineDlg.doDialog(willBeShown, isRTL);
    };

    static UserDefineDialog * getUserDefineDlg() {return &_userDefineDlg;};

	void setCaretColorWidth(int color, int width = 1) const {
		SetCaretFore(color);
		SetCaretWidth(width);
	};

	void beSwitched() {
		_userDefineDlg.setScintilla(this);
	};

    //Marge member and method
    static const int _SC_MARGE_LINENUMBER;
    static const int _SC_MARGE_SYBOLE;
    static const int _SC_MARGE_FOLDER;
	//static const int _SC_MARGE_MODIFMARKER;

    void showMargin(int whichMarge, bool willBeShowed = true) {
        if (whichMarge == _SC_MARGE_LINENUMBER)
			showLineNumbersMargin(willBeShowed);
        else
		{
			int width = 3;
			if (whichMarge == _SC_MARGE_SYBOLE)
				width = NppParameters::getInstance()->_dpiManager.scaleX(100) >= 150 ? 20 : 16;
			else if (whichMarge == _SC_MARGE_FOLDER)
				width = NppParameters::getInstance()->_dpiManager.scaleX(100) >= 150 ? 18 : 14;
			SetMarginWidthN(whichMarge, willBeShowed ? width : 0);
		}
    };

	bool hasMarginShowed(int witchMarge) {
		return (GetMarginWidthN(witchMarge) != 0);
	};

    void updateBeginEndSelectPosition(const bool is_insert, const int position, const int length);
    void marginClick(int position, int modifiers);

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
		SetWrapIndentMode(mode);
	};


	void showWSAndTab(bool willBeShowed = true) {
		SetViewWS(willBeShowed ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE);
		SetWhitespaceSize(2);
	};

	bool isEolVisible() {
		return (GetViewEOL() != 0);
	};

	void showInvisibleChars(bool willBeShowed = true) {
		showWSAndTab(willBeShowed);
		SetViewEOL(willBeShowed);
	};

	bool isInvisibleCharsShown() {
		return (GetViewWS() != 0);
	};

	void showIndentGuideLine(bool willBeShowed = true) {
		SetIndentationGuides(willBeShowed ? SC_IV_LOOKBOTH : SC_IV_NONE);
	};

	bool isShownIndentGuide() const {
		return (GetIndentationGuides() != 0);
	};

	void wrap(bool willBeWrapped = true) {
		SetWrapMode(willBeWrapped ? SC_WRAP_WORD : SC_WRAP_NONE);
	};

	bool isWrap() const {
		return (GetWrapMode() == SC_WRAP_WORD);
	};

	bool isWrapSymbolVisible() const {
		return (GetWrapVisualFlags() != SC_WRAPVISUALFLAG_NONE);
	};

	void showWrapSymbol(bool willBeShown = true) {
		SetWrapVisualFlagsLocation(SC_WRAPVISUALFLAGLOC_DEFAULT);
		SetWrapVisualFlags(willBeShown ? SC_WRAPVISUALFLAG_END : SC_WRAPVISUALFLAG_NONE);
	};

	size_t getCurrentLineNumber()const {
		return static_cast<size_t>(LineFromPosition(GetCurrentPos()));
	};

	int32_t lastZeroBasedLineNumber() const {
		auto endPos = GetLength();
		return static_cast<int32_t>(LineFromPosition(endPos));
	};

	int getCurrentPointX() const {
		return PointXFromPosition(GetCurrentPos());
	};

	int getCurrentPointY() const {
		return PointYFromPosition(GetCurrentPos());
	};

	int getCurrentColumnNumber() const {
		return GetColumn(GetCurrentPos());
	};

	bool getSelectedCount(int & selByte, int & selLine) const {
		// return false if it's multi-selection or rectangle selection
		if ((GetSelections() > 1) || SelectionIsRectangle())
			return false;
		int pStart = GetSelectionStart();
		int pEnd = GetSelectionEnd();
		selByte = pEnd - pStart;

		int lStart = LineFromPosition(pStart);
		int lEnd = LineFromPosition(pEnd);
		selLine = lEnd - lStart;
		if (selLine || selByte)
			++selLine;

		return true;
	};

	long getUnicodeSelectedLength() const
	{
		// return -1 if it's multi-selection or rectangle selection
		if ((GetSelections() > 1) || SelectionIsRectangle())
			return -1;

		auto text = GetSelText();

		const char *c = text.c_str();
		long length = 0;
		while(*c != '\0')
		{
			if( (*c & 0xC0) != 0x80)
				++length;
			++c;
		}
		return length;
	}


	int getLineLength(int line) const {
		return GetLineEndPosition(line) - PositionFromLine(line);
	};

	void setLineIndent(int line, int indent) const;

	void showLineNumbersMargin(bool show)
	{
		if (show == _lineNumbersShown) return;
		_lineNumbersShown = show;
		if (show)
		{
			updateLineNumberWidth();
		}
		else
		{
			SetMarginWidthN(_SC_MARGE_LINENUMBER, 0);
		}
	}

	void updateLineNumberWidth();

	void setCurrentLineHiLiting(bool isHiliting, COLORREF bgColor) const {
		SetCaretLineVisible(isHiliting);
		if (!isHiliting)
			return;
		SetCaretLineBack(bgColor);
	};

	bool isCurrentLineHiLiting() const {
		return (GetCaretLineVisible() != 0);
	};

	void performGlobalStyles();

	void expand(int &line, bool doExpand, bool force = false, int visLevels = 0, int level = -1);

	std::pair<int, int> getSelectionLinesRange() const;
    void currentLinesUp() const;
    void currentLinesDown() const;

	void changeCase(__inout wchar_t * const strWToConvert, const int & nbChars, const TextCase & caseToConvert) const;
	void convertSelectedTextTo(const TextCase & caseToConvert);
	void setMultiSelections(const ColumnModeInfos & cmi);

    void convertSelectedTextToLowerCase() {
		// if system is w2k or xp
		if ((NppParameters::getInstance())->isTransparentAvailable())
			convertSelectedTextTo(LOWERCASE);
		else
			LowerCase();
	};

    void convertSelectedTextToUpperCase() {
		// if system is w2k or xp
		if ((NppParameters::getInstance())->isTransparentAvailable())
			convertSelectedTextTo(UPPERCASE);
		else
			UpperCase();
	};

	void convertSelectedTextToNewerCase(const TextCase & caseToConvert) {
		// if system is w2k or xp
		if ((NppParameters::getInstance())->isTransparentAvailable())
			convertSelectedTextTo(caseToConvert);
		else
			::MessageBox(_hSelf, TEXT("This function needs a newer OS version."), TEXT("Change Case Error"), MB_OK | MB_ICONHAND);
	};

	void collapse(int level2Collapse, bool mode);
	void foldAll(bool mode);
	void fold(size_t line, bool mode);
	bool isFolded(int line){
		return (GetFoldExpanded(line) != 0);
	};
	void foldCurrentPos(bool mode);
	int getCodepage() const {return _codepage;};

	NppParameters * getParameter() {
		return _pParameter;
	};

	ColumnModeInfos getColumnModeSelectInfo();

	void columnReplace(ColumnModeInfos & cmi, const TCHAR *str);
	void columnReplace(ColumnModeInfos & cmi, int initial, int incr, int repeat, UCHAR format);

	void foldChanged(int line, int levelNow, int levelPrev);
	void clearIndicator(int indicatorNumber) {
		int docStart = 0;
		int docEnd = GetLength();
		SetIndicatorCurrent(indicatorNumber);
		IndicatorClearRange(docStart, docEnd - docStart);
	};

	static LanguageName langNames[L_EXTERNAL+1];

	void bufferUpdated(Buffer * buffer, int mask);
	BufferID getCurrentBufferID() { return _currentBufferID; };
	Buffer * getCurrentBuffer() { return _currentBuffer; };
	void setCurrentBuffer(Buffer *buf2set) { _currentBuffer = buf2set; };
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

	void defineDocType(LangType typeDoc);	//setup stylers for active document

	void addCustomWordChars();
	void restoreDefaultWordChars();
	void setWordChars();

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

	void AddText(int length, const char* text) const
	{
		execute(SCI_ADDTEXT, length, text);
	};

	void AddText(const std::string& text) const
	{
		execute(SCI_ADDTEXT, text.length(), text.c_str());
	};

	void AddStyledText(int length, const Cell* c) const
	{
		execute(SCI_ADDSTYLEDTEXT, length, c);
	};

	void InsertText(int pos, const char* text) const
	{
		execute(SCI_INSERTTEXT, pos, text);
	};

	void InsertText(int pos, const std::string& text) const
	{
		execute(SCI_INSERTTEXT, pos, text.c_str());
	};

	void ChangeInsertion(int length, const char* text) const
	{
		execute(SCI_CHANGEINSERTION, length, text);
	};

	void ChangeInsertion(const std::string& text) const
	{
		execute(SCI_CHANGEINSERTION, text.length(), text.c_str());
	};

	void ClearAll() const
	{
		execute(SCI_CLEARALL, SCI_UNUSED, SCI_UNUSED);
	};

	void DeleteRange(int pos, int deleteLength) const
	{
		execute(SCI_DELETERANGE, pos, deleteLength);
	};

	void ClearDocumentStyle() const
	{
		execute(SCI_CLEARDOCUMENTSTYLE, SCI_UNUSED, SCI_UNUSED);
	};

	int GetLength() const
	{
		sptr_t res = execute(SCI_GETLENGTH, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetCharAt(int pos) const
	{
		sptr_t res = execute(SCI_GETCHARAT, pos, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetCurrentPos() const
	{
		sptr_t res = execute(SCI_GETCURRENTPOS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetAnchor() const
	{
		sptr_t res = execute(SCI_GETANCHOR, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetStyleAt(int pos) const
	{
		sptr_t res = execute(SCI_GETSTYLEAT, pos, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void Redo() const
	{
		execute(SCI_REDO, SCI_UNUSED, SCI_UNUSED);
	};

	void SetUndoCollection(bool collectUndo) const
	{
		execute(SCI_SETUNDOCOLLECTION, collectUndo, SCI_UNUSED);
	};

	void SelectAll() const
	{
		execute(SCI_SELECTALL, SCI_UNUSED, SCI_UNUSED);
	};

	void SetSavePoint() const
	{
		execute(SCI_SETSAVEPOINT, SCI_UNUSED, SCI_UNUSED);
	};

	int GetStyledText(Sci_TextRange* tr) const
	{
		sptr_t res = execute(SCI_GETSTYLEDTEXT, SCI_UNUSED, tr);
		return static_cast<int>(res);
	};

	bool CanRedo() const
	{
		sptr_t res = execute(SCI_CANREDO, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	int MarkerLineFromHandle(int handle) const
	{
		sptr_t res = execute(SCI_MARKERLINEFROMHANDLE, handle, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void MarkerDeleteHandle(int handle) const
	{
		execute(SCI_MARKERDELETEHANDLE, handle, SCI_UNUSED);
	};

	bool GetUndoCollection() const
	{
		sptr_t res = execute(SCI_GETUNDOCOLLECTION, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	int GetViewWS() const
	{
		sptr_t res = execute(SCI_GETVIEWWS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetViewWS(int viewWS) const
	{
		execute(SCI_SETVIEWWS, viewWS, SCI_UNUSED);
	};

	int PositionFromPoint(int x, int y) const
	{
		sptr_t res = execute(SCI_POSITIONFROMPOINT, x, y);
		return static_cast<int>(res);
	};

	int PositionFromPointClose(int x, int y) const
	{
		sptr_t res = execute(SCI_POSITIONFROMPOINTCLOSE, x, y);
		return static_cast<int>(res);
	};

	void GotoLine(int line) const
	{
		execute(SCI_GOTOLINE, line, SCI_UNUSED);
	};

	void GotoPos(int pos) const
	{
		execute(SCI_GOTOPOS, pos, SCI_UNUSED);
	};

	void SetAnchor(int posAnchor) const
	{
		execute(SCI_SETANCHOR, posAnchor, SCI_UNUSED);
	};

	int GetCurLine(int length, char* text) const
	{
		sptr_t res = execute(SCI_GETCURLINE, length, text);
		return static_cast<int>(res);
	};

	std::string GetCurLine() const
	{
		auto size = execute(SCI_GETCURLINE, SCI_UNUSED, NULL);
		std::string text(size + 1, '\0');
		execute(SCI_GETCURLINE, text.length(), &text[0]);
		trim(text);
		return text;
	};

	int GetEndStyled() const
	{
		sptr_t res = execute(SCI_GETENDSTYLED, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void ConvertEOLs(int eolMode) const
	{
		execute(SCI_CONVERTEOLS, eolMode, SCI_UNUSED);
	};

	int GetEOLMode() const
	{
		sptr_t res = execute(SCI_GETEOLMODE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetEOLMode(int eolMode) const
	{
		execute(SCI_SETEOLMODE, eolMode, SCI_UNUSED);
	};

	void StartStyling(int pos, int mask) const
	{
		execute(SCI_STARTSTYLING, pos, mask);
	};

	void SetStyling(int length, int style) const
	{
		execute(SCI_SETSTYLING, length, style);
	};

	bool GetBufferedDraw() const
	{
		sptr_t res = execute(SCI_GETBUFFEREDDRAW, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetBufferedDraw(bool buffered) const
	{
		execute(SCI_SETBUFFEREDDRAW, buffered, SCI_UNUSED);
	};

	void SetTabWidth(int tabWidth) const
	{
		execute(SCI_SETTABWIDTH, tabWidth, SCI_UNUSED);
	};

	int GetTabWidth() const
	{
		sptr_t res = execute(SCI_GETTABWIDTH, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void ClearTabStops(int line) const
	{
		execute(SCI_CLEARTABSTOPS, line, SCI_UNUSED);
	};

	void AddTabStop(int line, int x) const
	{
		execute(SCI_ADDTABSTOP, line, x);
	};

	int GetNextTabStop(int line, int x) const
	{
		sptr_t res = execute(SCI_GETNEXTTABSTOP, line, x);
		return static_cast<int>(res);
	};

	void SetCodePage(int codePage) const
	{
		execute(SCI_SETCODEPAGE, codePage, SCI_UNUSED);
	};

	int GetIMEInteraction() const
	{
		sptr_t res = execute(SCI_GETIMEINTERACTION, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetIMEInteraction(int imeInteraction) const
	{
		execute(SCI_SETIMEINTERACTION, imeInteraction, SCI_UNUSED);
	};

	void MarkerDefine(int markerNumber, int markerSymbol) const
	{
		execute(SCI_MARKERDEFINE, markerNumber, markerSymbol);
	};

	void MarkerSetFore(int markerNumber, Colour fore) const
	{
		execute(SCI_MARKERSETFORE, markerNumber, fore);
	};

	void MarkerSetBack(int markerNumber, Colour back) const
	{
		execute(SCI_MARKERSETBACK, markerNumber, back);
	};

	void MarkerSetBackSelected(int markerNumber, Colour back) const
	{
		execute(SCI_MARKERSETBACKSELECTED, markerNumber, back);
	};

	void MarkerEnableHighlight(bool enabled) const
	{
		execute(SCI_MARKERENABLEHIGHLIGHT, enabled, SCI_UNUSED);
	};

	int MarkerAdd(int line, int markerNumber) const
	{
		sptr_t res = execute(SCI_MARKERADD, line, markerNumber);
		return static_cast<int>(res);
	};

	void MarkerDelete(int line, int markerNumber) const
	{
		execute(SCI_MARKERDELETE, line, markerNumber);
	};

	void MarkerDeleteAll(int markerNumber) const
	{
		execute(SCI_MARKERDELETEALL, markerNumber, SCI_UNUSED);
	};

	int MarkerGet(int line) const
	{
		sptr_t res = execute(SCI_MARKERGET, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int MarkerNext(int lineStart, int markerMask) const
	{
		sptr_t res = execute(SCI_MARKERNEXT, lineStart, markerMask);
		return static_cast<int>(res);
	};

	int MarkerPrevious(int lineStart, int markerMask) const
	{
		sptr_t res = execute(SCI_MARKERPREVIOUS, lineStart, markerMask);
		return static_cast<int>(res);
	};

	void MarkerDefinePixmap(int markerNumber, const char* pixmap) const
	{
		execute(SCI_MARKERDEFINEPIXMAP, markerNumber, pixmap);
	};

	void MarkerDefinePixmap(int markerNumber, const std::string& pixmap) const
	{
		execute(SCI_MARKERDEFINEPIXMAP, markerNumber, pixmap.c_str());
	};

	void MarkerAddSet(int line, int set) const
	{
		execute(SCI_MARKERADDSET, line, set);
	};

	void MarkerSetAlpha(int markerNumber, int alpha) const
	{
		execute(SCI_MARKERSETALPHA, markerNumber, alpha);
	};

	void SetMarginTypeN(int margin, int marginType) const
	{
		execute(SCI_SETMARGINTYPEN, margin, marginType);
	};

	int GetMarginTypeN(int margin) const
	{
		sptr_t res = execute(SCI_GETMARGINTYPEN, margin, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetMarginWidthN(int margin, int pixelWidth) const
	{
		execute(SCI_SETMARGINWIDTHN, margin, pixelWidth);
	};

	int GetMarginWidthN(int margin) const
	{
		sptr_t res = execute(SCI_GETMARGINWIDTHN, margin, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetMarginMaskN(int margin, int mask) const
	{
		execute(SCI_SETMARGINMASKN, margin, mask);
	};

	int GetMarginMaskN(int margin) const
	{
		sptr_t res = execute(SCI_GETMARGINMASKN, margin, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetMarginSensitiveN(int margin, bool sensitive) const
	{
		execute(SCI_SETMARGINSENSITIVEN, margin, sensitive);
	};

	bool GetMarginSensitiveN(int margin) const
	{
		sptr_t res = execute(SCI_GETMARGINSENSITIVEN, margin, SCI_UNUSED);
		return res != 0;
	};

	void SetMarginCursorN(int margin, int cursor) const
	{
		execute(SCI_SETMARGINCURSORN, margin, cursor);
	};

	int GetMarginCursorN(int margin) const
	{
		sptr_t res = execute(SCI_GETMARGINCURSORN, margin, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void StyleClearAll() const
	{
		execute(SCI_STYLECLEARALL, SCI_UNUSED, SCI_UNUSED);
	};

	void StyleSetFore(int style, Colour fore) const
	{
		execute(SCI_STYLESETFORE, style, fore);
	};

	void StyleSetBack(int style, Colour back) const
	{
		execute(SCI_STYLESETBACK, style, back);
	};

	void StyleSetBold(int style, bool bold) const
	{
		execute(SCI_STYLESETBOLD, style, bold);
	};

	void StyleSetItalic(int style, bool italic) const
	{
		execute(SCI_STYLESETITALIC, style, italic);
	};

	void StyleSetSize(int style, int sizePoints) const
	{
		execute(SCI_STYLESETSIZE, style, sizePoints);
	};

	void StyleSetFont(int style, const char* fontName) const
	{
		execute(SCI_STYLESETFONT, style, fontName);
	};

	void StyleSetFont(int style, const std::string& fontName) const
	{
		execute(SCI_STYLESETFONT, style, fontName.c_str());
	};

	void StyleSetEOLFilled(int style, bool filled) const
	{
		execute(SCI_STYLESETEOLFILLED, style, filled);
	};

	void StyleResetDefault() const
	{
		execute(SCI_STYLERESETDEFAULT, SCI_UNUSED, SCI_UNUSED);
	};

	void StyleSetUnderline(int style, bool underline) const
	{
		execute(SCI_STYLESETUNDERLINE, style, underline);
	};

	Colour StyleGetFore(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETFORE, style, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	Colour StyleGetBack(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETBACK, style, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	bool StyleGetBold(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETBOLD, style, SCI_UNUSED);
		return res != 0;
	};

	bool StyleGetItalic(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETITALIC, style, SCI_UNUSED);
		return res != 0;
	};

	int StyleGetSize(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETSIZE, style, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int StyleGetFont(int style, char* fontName) const
	{
		sptr_t res = execute(SCI_STYLEGETFONT, style, fontName);
		return static_cast<int>(res);
	};

	std::string StyleGetFont(int style) const
	{
		auto size = execute(SCI_STYLEGETFONT, style, NULL);
		std::string fontName(size + 1, '\0');
		execute(SCI_STYLEGETFONT, style, &fontName[0]);
		trim(fontName);
		return fontName;
	};

	bool StyleGetEOLFilled(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETEOLFILLED, style, SCI_UNUSED);
		return res != 0;
	};

	bool StyleGetUnderline(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETUNDERLINE, style, SCI_UNUSED);
		return res != 0;
	};

	int StyleGetCase(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETCASE, style, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int StyleGetCharacterSet(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETCHARACTERSET, style, SCI_UNUSED);
		return static_cast<int>(res);
	};

	bool StyleGetVisible(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETVISIBLE, style, SCI_UNUSED);
		return res != 0;
	};

	bool StyleGetChangeable(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETCHANGEABLE, style, SCI_UNUSED);
		return res != 0;
	};

	bool StyleGetHotSpot(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETHOTSPOT, style, SCI_UNUSED);
		return res != 0;
	};

	void StyleSetCase(int style, int caseForce) const
	{
		execute(SCI_STYLESETCASE, style, caseForce);
	};

	void StyleSetSizeFractional(int style, int caseForce) const
	{
		execute(SCI_STYLESETSIZEFRACTIONAL, style, caseForce);
	};

	int StyleGetSizeFractional(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETSIZEFRACTIONAL, style, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void StyleSetWeight(int style, int weight) const
	{
		execute(SCI_STYLESETWEIGHT, style, weight);
	};

	int StyleGetWeight(int style) const
	{
		sptr_t res = execute(SCI_STYLEGETWEIGHT, style, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void StyleSetCharacterSet(int style, int characterSet) const
	{
		execute(SCI_STYLESETCHARACTERSET, style, characterSet);
	};

	void StyleSetHotSpot(int style, bool hotspot) const
	{
		execute(SCI_STYLESETHOTSPOT, style, hotspot);
	};

	void SetSelFore(bool useSetting, Colour fore) const
	{
		execute(SCI_SETSELFORE, useSetting, fore);
	};

	void SetSelBack(bool useSetting, Colour back) const
	{
		execute(SCI_SETSELBACK, useSetting, back);
	};

	int GetSelAlpha() const
	{
		sptr_t res = execute(SCI_GETSELALPHA, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetSelAlpha(int alpha) const
	{
		execute(SCI_SETSELALPHA, alpha, SCI_UNUSED);
	};

	bool GetSelEOLFilled() const
	{
		sptr_t res = execute(SCI_GETSELEOLFILLED, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetSelEOLFilled(bool filled) const
	{
		execute(SCI_SETSELEOLFILLED, filled, SCI_UNUSED);
	};

	void SetCaretFore(Colour fore) const
	{
		execute(SCI_SETCARETFORE, fore, SCI_UNUSED);
	};

	void AssignCmdKey(int km, int msg) const
	{
		execute(SCI_ASSIGNCMDKEY, km, msg);
	};

	void ClearCmdKey(int km) const
	{
		execute(SCI_CLEARCMDKEY, km, SCI_UNUSED);
	};

	void ClearAllCmdKeys() const
	{
		execute(SCI_CLEARALLCMDKEYS, SCI_UNUSED, SCI_UNUSED);
	};

	void SetStylingEx(int length, const char* styles) const
	{
		execute(SCI_SETSTYLINGEX, length, styles);
	};

	void SetStylingEx(const std::string& styles) const
	{
		execute(SCI_SETSTYLINGEX, styles.length(), styles.c_str());
	};

	void StyleSetVisible(int style, bool visible) const
	{
		execute(SCI_STYLESETVISIBLE, style, visible);
	};

	int GetCaretPeriod() const
	{
		sptr_t res = execute(SCI_GETCARETPERIOD, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetCaretPeriod(int periodMilliseconds) const
	{
		execute(SCI_SETCARETPERIOD, periodMilliseconds, SCI_UNUSED);
	};

	void SetWordChars(const char* characters) const
	{
		execute(SCI_SETWORDCHARS, SCI_UNUSED, characters);
	};

	void SetWordChars(const std::string& characters) const
	{
		execute(SCI_SETWORDCHARS, SCI_UNUSED, characters.c_str());
	};

	int GetWordChars(char* characters) const
	{
		sptr_t res = execute(SCI_GETWORDCHARS, SCI_UNUSED, characters);
		return static_cast<int>(res);
	};

	std::string GetWordChars() const
	{
		auto size = execute(SCI_GETWORDCHARS, SCI_UNUSED, NULL);
		std::string characters(size + 1, '\0');
		execute(SCI_GETWORDCHARS, SCI_UNUSED, &characters[0]);
		trim(characters);
		return characters;
	};

	void BeginUndoAction() const
	{
		execute(SCI_BEGINUNDOACTION, SCI_UNUSED, SCI_UNUSED);
	};

	void EndUndoAction() const
	{
		execute(SCI_ENDUNDOACTION, SCI_UNUSED, SCI_UNUSED);
	};

	void IndicSetStyle(int indic, int style) const
	{
		execute(SCI_INDICSETSTYLE, indic, style);
	};

	int IndicGetStyle(int indic) const
	{
		sptr_t res = execute(SCI_INDICGETSTYLE, indic, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void IndicSetFore(int indic, Colour fore) const
	{
		execute(SCI_INDICSETFORE, indic, fore);
	};

	Colour IndicGetFore(int indic) const
	{
		sptr_t res = execute(SCI_INDICGETFORE, indic, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	void IndicSetUnder(int indic, bool under) const
	{
		execute(SCI_INDICSETUNDER, indic, under);
	};

	bool IndicGetUnder(int indic) const
	{
		sptr_t res = execute(SCI_INDICGETUNDER, indic, SCI_UNUSED);
		return res != 0;
	};

	void IndicSetHoverStyle(int indic, int style) const
	{
		execute(SCI_INDICSETHOVERSTYLE, indic, style);
	};

	int IndicGetHoverStyle(int indic) const
	{
		sptr_t res = execute(SCI_INDICGETHOVERSTYLE, indic, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void IndicSetHoverFore(int indic, Colour fore) const
	{
		execute(SCI_INDICSETHOVERFORE, indic, fore);
	};

	Colour IndicGetHoverFore(int indic) const
	{
		sptr_t res = execute(SCI_INDICGETHOVERFORE, indic, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	void IndicSetFlags(int indic, int flags) const
	{
		execute(SCI_INDICSETFLAGS, indic, flags);
	};

	int IndicGetFlags(int indic) const
	{
		sptr_t res = execute(SCI_INDICGETFLAGS, indic, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetWhitespaceFore(bool useSetting, Colour fore) const
	{
		execute(SCI_SETWHITESPACEFORE, useSetting, fore);
	};

	void SetWhitespaceBack(bool useSetting, Colour back) const
	{
		execute(SCI_SETWHITESPACEBACK, useSetting, back);
	};

	void SetWhitespaceSize(int size) const
	{
		execute(SCI_SETWHITESPACESIZE, size, SCI_UNUSED);
	};

	int GetWhitespaceSize() const
	{
		sptr_t res = execute(SCI_GETWHITESPACESIZE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetStyleBits(int bits) const
	{
		execute(SCI_SETSTYLEBITS, bits, SCI_UNUSED);
	};

	int GetStyleBits() const
	{
		sptr_t res = execute(SCI_GETSTYLEBITS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetLineState(int line, int state) const
	{
		execute(SCI_SETLINESTATE, line, state);
	};

	int GetLineState(int line) const
	{
		sptr_t res = execute(SCI_GETLINESTATE, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetMaxLineState() const
	{
		sptr_t res = execute(SCI_GETMAXLINESTATE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	bool GetCaretLineVisible() const
	{
		sptr_t res = execute(SCI_GETCARETLINEVISIBLE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetCaretLineVisible(bool show) const
	{
		execute(SCI_SETCARETLINEVISIBLE, show, SCI_UNUSED);
	};

	Colour GetCaretLineBack() const
	{
		sptr_t res = execute(SCI_GETCARETLINEBACK, SCI_UNUSED, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	void SetCaretLineBack(Colour back) const
	{
		execute(SCI_SETCARETLINEBACK, back, SCI_UNUSED);
	};

	void StyleSetChangeable(int style, bool changeable) const
	{
		execute(SCI_STYLESETCHANGEABLE, style, changeable);
	};

	void AutoCShow(int lenEntered, const char* itemList) const
	{
		execute(SCI_AUTOCSHOW, lenEntered, itemList);
	};

	void AutoCShow(int lenEntered, const std::string& itemList) const
	{
		execute(SCI_AUTOCSHOW, lenEntered, itemList.c_str());
	};

	void AutoCCancel() const
	{
		execute(SCI_AUTOCCANCEL, SCI_UNUSED, SCI_UNUSED);
	};

	bool AutoCActive() const
	{
		sptr_t res = execute(SCI_AUTOCACTIVE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	int AutoCPosStart() const
	{
		sptr_t res = execute(SCI_AUTOCPOSSTART, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AutoCComplete() const
	{
		execute(SCI_AUTOCCOMPLETE, SCI_UNUSED, SCI_UNUSED);
	};

	void AutoCStops(const char* characterSet) const
	{
		execute(SCI_AUTOCSTOPS, SCI_UNUSED, characterSet);
	};

	void AutoCStops(const std::string& characterSet) const
	{
		execute(SCI_AUTOCSTOPS, SCI_UNUSED, characterSet.c_str());
	};

	void AutoCSetSeparator(int separatorCharacter) const
	{
		execute(SCI_AUTOCSETSEPARATOR, separatorCharacter, SCI_UNUSED);
	};

	int AutoCGetSeparator() const
	{
		sptr_t res = execute(SCI_AUTOCGETSEPARATOR, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AutoCSelect(const char* text) const
	{
		execute(SCI_AUTOCSELECT, SCI_UNUSED, text);
	};

	void AutoCSelect(const std::string& text) const
	{
		execute(SCI_AUTOCSELECT, SCI_UNUSED, text.c_str());
	};

	void AutoCSetCancelAtStart(bool cancel) const
	{
		execute(SCI_AUTOCSETCANCELATSTART, cancel, SCI_UNUSED);
	};

	bool AutoCGetCancelAtStart() const
	{
		sptr_t res = execute(SCI_AUTOCGETCANCELATSTART, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void AutoCSetFillUps(const char* characterSet) const
	{
		execute(SCI_AUTOCSETFILLUPS, SCI_UNUSED, characterSet);
	};

	void AutoCSetFillUps(const std::string& characterSet) const
	{
		execute(SCI_AUTOCSETFILLUPS, SCI_UNUSED, characterSet.c_str());
	};

	void AutoCSetChooseSingle(bool chooseSingle) const
	{
		execute(SCI_AUTOCSETCHOOSESINGLE, chooseSingle, SCI_UNUSED);
	};

	bool AutoCGetChooseSingle() const
	{
		sptr_t res = execute(SCI_AUTOCGETCHOOSESINGLE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void AutoCSetIgnoreCase(bool ignoreCase) const
	{
		execute(SCI_AUTOCSETIGNORECASE, ignoreCase, SCI_UNUSED);
	};

	bool AutoCGetIgnoreCase() const
	{
		sptr_t res = execute(SCI_AUTOCGETIGNORECASE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void UserListShow(int listType, const char* itemList) const
	{
		execute(SCI_USERLISTSHOW, listType, itemList);
	};

	void UserListShow(int listType, const std::string& itemList) const
	{
		execute(SCI_USERLISTSHOW, listType, itemList.c_str());
	};

	void AutoCSetAutoHide(bool autoHide) const
	{
		execute(SCI_AUTOCSETAUTOHIDE, autoHide, SCI_UNUSED);
	};

	bool AutoCGetAutoHide() const
	{
		sptr_t res = execute(SCI_AUTOCGETAUTOHIDE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void AutoCSetDropRestOfWord(bool dropRestOfWord) const
	{
		execute(SCI_AUTOCSETDROPRESTOFWORD, dropRestOfWord, SCI_UNUSED);
	};

	bool AutoCGetDropRestOfWord() const
	{
		sptr_t res = execute(SCI_AUTOCGETDROPRESTOFWORD, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void RegisterImage(int type, const char* xpmData) const
	{
		execute(SCI_REGISTERIMAGE, type, xpmData);
	};

	void RegisterImage(int type, const std::string& xpmData) const
	{
		execute(SCI_REGISTERIMAGE, type, xpmData.c_str());
	};

	void ClearRegisteredImages() const
	{
		execute(SCI_CLEARREGISTEREDIMAGES, SCI_UNUSED, SCI_UNUSED);
	};

	int AutoCGetTypeSeparator() const
	{
		sptr_t res = execute(SCI_AUTOCGETTYPESEPARATOR, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AutoCSetTypeSeparator(int separatorCharacter) const
	{
		execute(SCI_AUTOCSETTYPESEPARATOR, separatorCharacter, SCI_UNUSED);
	};

	void AutoCSetMaxWidth(int characterCount) const
	{
		execute(SCI_AUTOCSETMAXWIDTH, characterCount, SCI_UNUSED);
	};

	int AutoCGetMaxWidth() const
	{
		sptr_t res = execute(SCI_AUTOCGETMAXWIDTH, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AutoCSetMaxHeight(int rowCount) const
	{
		execute(SCI_AUTOCSETMAXHEIGHT, rowCount, SCI_UNUSED);
	};

	int AutoCGetMaxHeight() const
	{
		sptr_t res = execute(SCI_AUTOCGETMAXHEIGHT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetIndent(int indentSize) const
	{
		execute(SCI_SETINDENT, indentSize, SCI_UNUSED);
	};

	int GetIndent() const
	{
		sptr_t res = execute(SCI_GETINDENT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetUseTabs(bool useTabs) const
	{
		execute(SCI_SETUSETABS, useTabs, SCI_UNUSED);
	};

	bool GetUseTabs() const
	{
		sptr_t res = execute(SCI_GETUSETABS, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetLineIndentation(int line, int indentSize) const
	{
		execute(SCI_SETLINEINDENTATION, line, indentSize);
	};

	int GetLineIndentation(int line) const
	{
		sptr_t res = execute(SCI_GETLINEINDENTATION, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetLineIndentPosition(int line) const
	{
		sptr_t res = execute(SCI_GETLINEINDENTPOSITION, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetColumn(int pos) const
	{
		sptr_t res = execute(SCI_GETCOLUMN, pos, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int CountCharacters(int startPos, int endPos) const
	{
		sptr_t res = execute(SCI_COUNTCHARACTERS, startPos, endPos);
		return static_cast<int>(res);
	};

	void SetHScrollBar(bool show) const
	{
		execute(SCI_SETHSCROLLBAR, show, SCI_UNUSED);
	};

	bool GetHScrollBar() const
	{
		sptr_t res = execute(SCI_GETHSCROLLBAR, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetIndentationGuides(int indentView) const
	{
		execute(SCI_SETINDENTATIONGUIDES, indentView, SCI_UNUSED);
	};

	int GetIndentationGuides() const
	{
		sptr_t res = execute(SCI_GETINDENTATIONGUIDES, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetHighlightGuide(int column) const
	{
		execute(SCI_SETHIGHLIGHTGUIDE, column, SCI_UNUSED);
	};

	int GetHighlightGuide() const
	{
		sptr_t res = execute(SCI_GETHIGHLIGHTGUIDE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetLineEndPosition(int line) const
	{
		sptr_t res = execute(SCI_GETLINEENDPOSITION, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetCodePage() const
	{
		sptr_t res = execute(SCI_GETCODEPAGE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	Colour GetCaretFore() const
	{
		sptr_t res = execute(SCI_GETCARETFORE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	bool GetReadOnly() const
	{
		sptr_t res = execute(SCI_GETREADONLY, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetCurrentPos(int pos) const
	{
		execute(SCI_SETCURRENTPOS, pos, SCI_UNUSED);
	};

	void SetSelectionStart(int pos) const
	{
		execute(SCI_SETSELECTIONSTART, pos, SCI_UNUSED);
	};

	int GetSelectionStart() const
	{
		sptr_t res = execute(SCI_GETSELECTIONSTART, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetSelectionEnd(int pos) const
	{
		execute(SCI_SETSELECTIONEND, pos, SCI_UNUSED);
	};

	int GetSelectionEnd() const
	{
		sptr_t res = execute(SCI_GETSELECTIONEND, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetEmptySelection(int pos) const
	{
		execute(SCI_SETEMPTYSELECTION, pos, SCI_UNUSED);
	};

	void SetPrintMagnification(int magnification) const
	{
		execute(SCI_SETPRINTMAGNIFICATION, magnification, SCI_UNUSED);
	};

	int GetPrintMagnification() const
	{
		sptr_t res = execute(SCI_GETPRINTMAGNIFICATION, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetPrintColourMode(int mode) const
	{
		execute(SCI_SETPRINTCOLOURMODE, mode, SCI_UNUSED);
	};

	int GetPrintColourMode() const
	{
		sptr_t res = execute(SCI_GETPRINTCOLOURMODE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int FindText(int flags, Sci_TextToFind* ft) const
	{
		sptr_t res = execute(SCI_FINDTEXT, flags, ft);
		return static_cast<int>(res);
	};

	int FormatRange(bool draw, Sci_RangeToFormat* fr) const
	{
		sptr_t res = execute(SCI_FORMATRANGE, draw, fr);
		return static_cast<int>(res);
	};

	int GetFirstVisibleLine() const
	{
		sptr_t res = execute(SCI_GETFIRSTVISIBLELINE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetLine(int line, char* text) const
	{
		sptr_t res = execute(SCI_GETLINE, line, text);
		return static_cast<int>(res);
	};

	std::string GetLine(int line) const
	{
		auto size = execute(SCI_GETLINE, line, NULL);
		std::string text(size + 1, '\0');
		execute(SCI_GETLINE, line, &text[0]);
		trim(text);
		return text;
	};

	int GetLineCount() const
	{
		sptr_t res = execute(SCI_GETLINECOUNT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetMarginLeft(int pixelWidth) const
	{
		execute(SCI_SETMARGINLEFT, SCI_UNUSED, pixelWidth);
	};

	int GetMarginLeft() const
	{
		sptr_t res = execute(SCI_GETMARGINLEFT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetMarginRight(int pixelWidth) const
	{
		execute(SCI_SETMARGINRIGHT, SCI_UNUSED, pixelWidth);
	};

	int GetMarginRight() const
	{
		sptr_t res = execute(SCI_GETMARGINRIGHT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	bool GetModify() const
	{
		sptr_t res = execute(SCI_GETMODIFY, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetSel(int start, int end) const
	{
		execute(SCI_SETSEL, start, end);
	};

	int GetSelText(char* text) const
	{
		sptr_t res = execute(SCI_GETSELTEXT, SCI_UNUSED, text);
		return static_cast<int>(res);
	};

	std::string GetSelText() const
	{
		auto size = execute(SCI_GETSELTEXT, SCI_UNUSED, NULL);
		std::string text(size + 1, '\0');
		execute(SCI_GETSELTEXT, SCI_UNUSED, &text[0]);
		trim(text);
		return text;
	};

	int GetTextRange(Sci_TextRange* tr) const
	{
		sptr_t res = execute(SCI_GETTEXTRANGE, SCI_UNUSED, tr);
		return static_cast<int>(res);
	};

	void HideSelection(bool normal) const
	{
		execute(SCI_HIDESELECTION, normal, SCI_UNUSED);
	};

	int PointXFromPosition(int pos) const
	{
		sptr_t res = execute(SCI_POINTXFROMPOSITION, SCI_UNUSED, pos);
		return static_cast<int>(res);
	};

	int PointYFromPosition(int pos) const
	{
		sptr_t res = execute(SCI_POINTYFROMPOSITION, SCI_UNUSED, pos);
		return static_cast<int>(res);
	};

	int LineFromPosition(int pos) const
	{
		sptr_t res = execute(SCI_LINEFROMPOSITION, pos, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int PositionFromLine(int line) const
	{
		sptr_t res = execute(SCI_POSITIONFROMLINE, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void LineScroll(int columns, int lines) const
	{
		execute(SCI_LINESCROLL, columns, lines);
	};

	void ScrollCaret() const
	{
		execute(SCI_SCROLLCARET, SCI_UNUSED, SCI_UNUSED);
	};

	void ScrollRange(int secondary, int primary) const
	{
		execute(SCI_SCROLLRANGE, secondary, primary);
	};

	void ReplaceSel(const char* text) const
	{
		execute(SCI_REPLACESEL, SCI_UNUSED, text);
	};

	void ReplaceSel(const std::string& text) const
	{
		execute(SCI_REPLACESEL, SCI_UNUSED, text.c_str());
	};

	void SetReadOnly(bool readOnly) const
	{
		execute(SCI_SETREADONLY, readOnly, SCI_UNUSED);
	};

	void Null() const
	{
		execute(SCI_NULL, SCI_UNUSED, SCI_UNUSED);
	};

	bool CanPaste() const
	{
		sptr_t res = execute(SCI_CANPASTE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	bool CanUndo() const
	{
		sptr_t res = execute(SCI_CANUNDO, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void EmptyUndoBuffer() const
	{
		execute(SCI_EMPTYUNDOBUFFER, SCI_UNUSED, SCI_UNUSED);
	};

	void Undo() const
	{
		execute(SCI_UNDO, SCI_UNUSED, SCI_UNUSED);
	};

	void Cut() const
	{
		execute(SCI_CUT, SCI_UNUSED, SCI_UNUSED);
	};

	void Copy() const
	{
		execute(SCI_COPY, SCI_UNUSED, SCI_UNUSED);
	};

	void Paste() const
	{
		execute(SCI_PASTE, SCI_UNUSED, SCI_UNUSED);
	};

	void Clear() const
	{
		execute(SCI_CLEAR, SCI_UNUSED, SCI_UNUSED);
	};

	void SetText(const char* text) const
	{
		execute(SCI_SETTEXT, SCI_UNUSED, text);
	};

	void SetText(const std::string& text) const
	{
		execute(SCI_SETTEXT, SCI_UNUSED, text.c_str());
	};

	int GetText(int length, char* text) const
	{
		sptr_t res = execute(SCI_GETTEXT, length, text);
		return static_cast<int>(res);
	};

	std::string GetText() const
	{
		auto size = execute(SCI_GETTEXT, SCI_UNUSED, NULL);
		std::string text(size + 1, '\0');
		execute(SCI_GETTEXT, text.length(), &text[0]);
		trim(text);
		return text;
	};

	int GetTextLength() const
	{
		sptr_t res = execute(SCI_GETTEXTLENGTH, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	sptr_t GetDirectFunction() const
	{
		return execute(SCI_GETDIRECTFUNCTION, SCI_UNUSED, SCI_UNUSED);
	};

	sptr_t GetDirectPointer() const
	{
		return execute(SCI_GETDIRECTPOINTER, SCI_UNUSED, SCI_UNUSED);
	};

	void SetOvertype(bool overtype) const
	{
		execute(SCI_SETOVERTYPE, overtype, SCI_UNUSED);
	};

	bool GetOvertype() const
	{
		sptr_t res = execute(SCI_GETOVERTYPE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetCaretWidth(int pixelWidth) const
	{
		execute(SCI_SETCARETWIDTH, pixelWidth, SCI_UNUSED);
	};

	int GetCaretWidth() const
	{
		sptr_t res = execute(SCI_GETCARETWIDTH, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetTargetStart(int pos) const
	{
		execute(SCI_SETTARGETSTART, pos, SCI_UNUSED);
	};

	int GetTargetStart() const
	{
		sptr_t res = execute(SCI_GETTARGETSTART, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetTargetEnd(int pos) const
	{
		execute(SCI_SETTARGETEND, pos, SCI_UNUSED);
	};

	int GetTargetEnd() const
	{
		sptr_t res = execute(SCI_GETTARGETEND, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetTargetRange(int start, int end) const
	{
		execute(SCI_SETTARGETRANGE, start, end);
	};

	int GetTargetText(char* characters) const
	{
		sptr_t res = execute(SCI_GETTARGETTEXT, SCI_UNUSED, characters);
		return static_cast<int>(res);
	};

	std::string GetTargetText() const
	{
		auto size = execute(SCI_GETTARGETTEXT, SCI_UNUSED, NULL);
		std::string characters(size + 1, '\0');
		execute(SCI_GETTARGETTEXT, SCI_UNUSED, &characters[0]);
		trim(characters);
		return characters;
	};

	int ReplaceTarget(int length, const char* text) const
	{
		sptr_t res = execute(SCI_REPLACETARGET, length, text);
		return static_cast<int>(res);
	};

	int ReplaceTarget(const std::string& text) const
	{
		sptr_t res = execute(SCI_REPLACETARGET, text.length(), text.c_str());
		return static_cast<int>(res);
	};

	int ReplaceTargetRE(int length, const char* text) const
	{
		sptr_t res = execute(SCI_REPLACETARGETRE, length, text);
		return static_cast<int>(res);
	};

	int ReplaceTargetRE(const std::string& text) const
	{
		sptr_t res = execute(SCI_REPLACETARGETRE, text.length(), text.c_str());
		return static_cast<int>(res);
	};

	int SearchInTarget(int length, const char* text) const
	{
		sptr_t res = execute(SCI_SEARCHINTARGET, length, text);
		return static_cast<int>(res);
	};

	int SearchInTarget(const std::string& text) const
	{
		sptr_t res = execute(SCI_SEARCHINTARGET, text.length(), text.c_str());
		return static_cast<int>(res);
	};

	void SetSearchFlags(int flags) const
	{
		execute(SCI_SETSEARCHFLAGS, flags, SCI_UNUSED);
	};

	int GetSearchFlags() const
	{
		sptr_t res = execute(SCI_GETSEARCHFLAGS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void CallTipShow(int pos, const char* definition) const
	{
		execute(SCI_CALLTIPSHOW, pos, definition);
	};

	void CallTipShow(int pos, const std::string& definition) const
	{
		execute(SCI_CALLTIPSHOW, pos, definition.c_str());
	};

	void CallTipCancel() const
	{
		execute(SCI_CALLTIPCANCEL, SCI_UNUSED, SCI_UNUSED);
	};

	bool CallTipActive() const
	{
		sptr_t res = execute(SCI_CALLTIPACTIVE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	int CallTipPosStart() const
	{
		sptr_t res = execute(SCI_CALLTIPPOSSTART, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void CallTipSetPosStart(int posStart) const
	{
		execute(SCI_CALLTIPSETPOSSTART, posStart, SCI_UNUSED);
	};

	void CallTipSetHlt(int start, int end) const
	{
		execute(SCI_CALLTIPSETHLT, start, end);
	};

	void CallTipSetBack(Colour back) const
	{
		execute(SCI_CALLTIPSETBACK, back, SCI_UNUSED);
	};

	void CallTipSetFore(Colour fore) const
	{
		execute(SCI_CALLTIPSETFORE, fore, SCI_UNUSED);
	};

	void CallTipSetForeHlt(Colour fore) const
	{
		execute(SCI_CALLTIPSETFOREHLT, fore, SCI_UNUSED);
	};

	void CallTipUseStyle(int tabSize) const
	{
		execute(SCI_CALLTIPUSESTYLE, tabSize, SCI_UNUSED);
	};

	void CallTipSetPosition(bool above) const
	{
		execute(SCI_CALLTIPSETPOSITION, above, SCI_UNUSED);
	};

	int VisibleFromDocLine(int line) const
	{
		sptr_t res = execute(SCI_VISIBLEFROMDOCLINE, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int DocLineFromVisible(int lineDisplay) const
	{
		sptr_t res = execute(SCI_DOCLINEFROMVISIBLE, lineDisplay, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int WrapCount(int line) const
	{
		sptr_t res = execute(SCI_WRAPCOUNT, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetFoldLevel(int line, int level) const
	{
		execute(SCI_SETFOLDLEVEL, line, level);
	};

	int GetFoldLevel(int line) const
	{
		sptr_t res = execute(SCI_GETFOLDLEVEL, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetLastChild(int line, int level) const
	{
		sptr_t res = execute(SCI_GETLASTCHILD, line, level);
		return static_cast<int>(res);
	};

	int GetFoldParent(int line) const
	{
		sptr_t res = execute(SCI_GETFOLDPARENT, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void ShowLines(int lineStart, int lineEnd) const
	{
		execute(SCI_SHOWLINES, lineStart, lineEnd);
	};

	void HideLines(int lineStart, int lineEnd) const
	{
		execute(SCI_HIDELINES, lineStart, lineEnd);
	};

	bool GetLineVisible(int line) const
	{
		sptr_t res = execute(SCI_GETLINEVISIBLE, line, SCI_UNUSED);
		return res != 0;
	};

	bool GetAllLinesVisible() const
	{
		sptr_t res = execute(SCI_GETALLLINESVISIBLE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetFoldExpanded(int line, bool expanded) const
	{
		execute(SCI_SETFOLDEXPANDED, line, expanded);
	};

	bool GetFoldExpanded(int line) const
	{
		sptr_t res = execute(SCI_GETFOLDEXPANDED, line, SCI_UNUSED);
		return res != 0;
	};

	void ToggleFold(int line) const
	{
		execute(SCI_TOGGLEFOLD, line, SCI_UNUSED);
	};

	void FoldLine(int line, int action) const
	{
		execute(SCI_FOLDLINE, line, action);
	};

	void FoldChildren(int line, int action) const
	{
		execute(SCI_FOLDCHILDREN, line, action);
	};

	void ExpandChildren(int line, int level) const
	{
		execute(SCI_EXPANDCHILDREN, line, level);
	};

	void FoldAll(int action) const
	{
		execute(SCI_FOLDALL, action, SCI_UNUSED);
	};

	void EnsureVisible(int line) const
	{
		execute(SCI_ENSUREVISIBLE, line, SCI_UNUSED);
	};

	void SetAutomaticFold(int automaticFold) const
	{
		execute(SCI_SETAUTOMATICFOLD, automaticFold, SCI_UNUSED);
	};

	int GetAutomaticFold() const
	{
		sptr_t res = execute(SCI_GETAUTOMATICFOLD, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetFoldFlags(int flags) const
	{
		execute(SCI_SETFOLDFLAGS, flags, SCI_UNUSED);
	};

	void EnsureVisibleEnforcePolicy(int line) const
	{
		execute(SCI_ENSUREVISIBLEENFORCEPOLICY, line, SCI_UNUSED);
	};

	void SetTabIndents(bool tabIndents) const
	{
		execute(SCI_SETTABINDENTS, tabIndents, SCI_UNUSED);
	};

	bool GetTabIndents() const
	{
		sptr_t res = execute(SCI_GETTABINDENTS, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetBackSpaceUnIndents(bool bsUnIndents) const
	{
		execute(SCI_SETBACKSPACEUNINDENTS, bsUnIndents, SCI_UNUSED);
	};

	bool GetBackSpaceUnIndents() const
	{
		sptr_t res = execute(SCI_GETBACKSPACEUNINDENTS, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetMouseDwellTime(int periodMilliseconds) const
	{
		execute(SCI_SETMOUSEDWELLTIME, periodMilliseconds, SCI_UNUSED);
	};

	int GetMouseDwellTime() const
	{
		sptr_t res = execute(SCI_GETMOUSEDWELLTIME, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int WordStartPosition(int pos, bool onlyWordCharacters) const
	{
		sptr_t res = execute(SCI_WORDSTARTPOSITION, pos, onlyWordCharacters);
		return static_cast<int>(res);
	};

	int WordEndPosition(int pos, bool onlyWordCharacters) const
	{
		sptr_t res = execute(SCI_WORDENDPOSITION, pos, onlyWordCharacters);
		return static_cast<int>(res);
	};

	void SetWrapMode(int mode) const
	{
		execute(SCI_SETWRAPMODE, mode, SCI_UNUSED);
	};

	int GetWrapMode() const
	{
		sptr_t res = execute(SCI_GETWRAPMODE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetWrapVisualFlags(int wrapVisualFlags) const
	{
		execute(SCI_SETWRAPVISUALFLAGS, wrapVisualFlags, SCI_UNUSED);
	};

	int GetWrapVisualFlags() const
	{
		sptr_t res = execute(SCI_GETWRAPVISUALFLAGS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetWrapVisualFlagsLocation(int wrapVisualFlagsLocation) const
	{
		execute(SCI_SETWRAPVISUALFLAGSLOCATION, wrapVisualFlagsLocation, SCI_UNUSED);
	};

	int GetWrapVisualFlagsLocation() const
	{
		sptr_t res = execute(SCI_GETWRAPVISUALFLAGSLOCATION, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetWrapStartIndent(int indent) const
	{
		execute(SCI_SETWRAPSTARTINDENT, indent, SCI_UNUSED);
	};

	int GetWrapStartIndent() const
	{
		sptr_t res = execute(SCI_GETWRAPSTARTINDENT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetWrapIndentMode(int mode) const
	{
		execute(SCI_SETWRAPINDENTMODE, mode, SCI_UNUSED);
	};

	int GetWrapIndentMode() const
	{
		sptr_t res = execute(SCI_GETWRAPINDENTMODE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetLayoutCache(int mode) const
	{
		execute(SCI_SETLAYOUTCACHE, mode, SCI_UNUSED);
	};

	int GetLayoutCache() const
	{
		sptr_t res = execute(SCI_GETLAYOUTCACHE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetScrollWidth(int pixelWidth) const
	{
		execute(SCI_SETSCROLLWIDTH, pixelWidth, SCI_UNUSED);
	};

	int GetScrollWidth() const
	{
		sptr_t res = execute(SCI_GETSCROLLWIDTH, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetScrollWidthTracking(bool tracking) const
	{
		execute(SCI_SETSCROLLWIDTHTRACKING, tracking, SCI_UNUSED);
	};

	bool GetScrollWidthTracking() const
	{
		sptr_t res = execute(SCI_GETSCROLLWIDTHTRACKING, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	int TextWidth(int style, const char* text) const
	{
		sptr_t res = execute(SCI_TEXTWIDTH, style, text);
		return static_cast<int>(res);
	};

	int TextWidth(int style, const std::string& text) const
	{
		sptr_t res = execute(SCI_TEXTWIDTH, style, text.c_str());
		return static_cast<int>(res);
	};

	void SetEndAtLastLine(bool endAtLastLine) const
	{
		execute(SCI_SETENDATLASTLINE, endAtLastLine, SCI_UNUSED);
	};

	bool GetEndAtLastLine() const
	{
		sptr_t res = execute(SCI_GETENDATLASTLINE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	int TextHeight(int line) const
	{
		sptr_t res = execute(SCI_TEXTHEIGHT, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetVScrollBar(bool show) const
	{
		execute(SCI_SETVSCROLLBAR, show, SCI_UNUSED);
	};

	bool GetVScrollBar() const
	{
		sptr_t res = execute(SCI_GETVSCROLLBAR, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void AppendText(int length, const char* text) const
	{
		execute(SCI_APPENDTEXT, length, text);
	};

	void AppendText(const std::string& text) const
	{
		execute(SCI_APPENDTEXT, text.length(), text.c_str());
	};

	bool GetTwoPhaseDraw() const
	{
		sptr_t res = execute(SCI_GETTWOPHASEDRAW, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetTwoPhaseDraw(bool twoPhase) const
	{
		execute(SCI_SETTWOPHASEDRAW, twoPhase, SCI_UNUSED);
	};

	int GetPhasesDraw() const
	{
		sptr_t res = execute(SCI_GETPHASESDRAW, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetPhasesDraw(int phases) const
	{
		execute(SCI_SETPHASESDRAW, phases, SCI_UNUSED);
	};

	void SetFontQuality(int fontQuality) const
	{
		execute(SCI_SETFONTQUALITY, fontQuality, SCI_UNUSED);
	};

	int GetFontQuality() const
	{
		sptr_t res = execute(SCI_GETFONTQUALITY, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetFirstVisibleLine(int lineDisplay) const
	{
		execute(SCI_SETFIRSTVISIBLELINE, lineDisplay, SCI_UNUSED);
	};

	void SetMultiPaste(int multiPaste) const
	{
		execute(SCI_SETMULTIPASTE, multiPaste, SCI_UNUSED);
	};

	int GetMultiPaste() const
	{
		sptr_t res = execute(SCI_GETMULTIPASTE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetTag(int tagNumber, char* tagValue) const
	{
		sptr_t res = execute(SCI_GETTAG, tagNumber, tagValue);
		return static_cast<int>(res);
	};

	std::string GetTag(int tagNumber) const
	{
		auto size = execute(SCI_GETTAG, tagNumber, NULL);
		std::string tagValue(size + 1, '\0');
		execute(SCI_GETTAG, tagNumber, &tagValue[0]);
		trim(tagValue);
		return tagValue;
	};

	void TargetFromSelection() const
	{
		execute(SCI_TARGETFROMSELECTION, SCI_UNUSED, SCI_UNUSED);
	};

	void LinesJoin() const
	{
		execute(SCI_LINESJOIN, SCI_UNUSED, SCI_UNUSED);
	};

	void LinesSplit(int pixelWidth) const
	{
		execute(SCI_LINESSPLIT, pixelWidth, SCI_UNUSED);
	};

	void SetFoldMarginColour(bool useSetting, Colour back) const
	{
		execute(SCI_SETFOLDMARGINCOLOUR, useSetting, back);
	};

	void SetFoldMarginHiColour(bool useSetting, Colour fore) const
	{
		execute(SCI_SETFOLDMARGINHICOLOUR, useSetting, fore);
	};

	void LineDown() const
	{
		execute(SCI_LINEDOWN, SCI_UNUSED, SCI_UNUSED);
	};

	void LineDownExtend() const
	{
		execute(SCI_LINEDOWNEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineUp() const
	{
		execute(SCI_LINEUP, SCI_UNUSED, SCI_UNUSED);
	};

	void LineUpExtend() const
	{
		execute(SCI_LINEUPEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void CharLeft() const
	{
		execute(SCI_CHARLEFT, SCI_UNUSED, SCI_UNUSED);
	};

	void CharLeftExtend() const
	{
		execute(SCI_CHARLEFTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void CharRight() const
	{
		execute(SCI_CHARRIGHT, SCI_UNUSED, SCI_UNUSED);
	};

	void CharRightExtend() const
	{
		execute(SCI_CHARRIGHTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void WordLeft() const
	{
		execute(SCI_WORDLEFT, SCI_UNUSED, SCI_UNUSED);
	};

	void WordLeftExtend() const
	{
		execute(SCI_WORDLEFTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void WordRight() const
	{
		execute(SCI_WORDRIGHT, SCI_UNUSED, SCI_UNUSED);
	};

	void WordRightExtend() const
	{
		execute(SCI_WORDRIGHTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void Home() const
	{
		execute(SCI_HOME, SCI_UNUSED, SCI_UNUSED);
	};

	void HomeExtend() const
	{
		execute(SCI_HOMEEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineEnd() const
	{
		execute(SCI_LINEEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineEndExtend() const
	{
		execute(SCI_LINEENDEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void DocumentStart() const
	{
		execute(SCI_DOCUMENTSTART, SCI_UNUSED, SCI_UNUSED);
	};

	void DocumentStartExtend() const
	{
		execute(SCI_DOCUMENTSTARTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void DocumentEnd() const
	{
		execute(SCI_DOCUMENTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void DocumentEndExtend() const
	{
		execute(SCI_DOCUMENTENDEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void PageUp() const
	{
		execute(SCI_PAGEUP, SCI_UNUSED, SCI_UNUSED);
	};

	void PageUpExtend() const
	{
		execute(SCI_PAGEUPEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void PageDown() const
	{
		execute(SCI_PAGEDOWN, SCI_UNUSED, SCI_UNUSED);
	};

	void PageDownExtend() const
	{
		execute(SCI_PAGEDOWNEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void EditToggleOvertype() const
	{
		execute(SCI_EDITTOGGLEOVERTYPE, SCI_UNUSED, SCI_UNUSED);
	};

	void Cancel() const
	{
		execute(SCI_CANCEL, SCI_UNUSED, SCI_UNUSED);
	};

	void DeleteBack() const
	{
		execute(SCI_DELETEBACK, SCI_UNUSED, SCI_UNUSED);
	};

	void Tab() const
	{
		execute(SCI_TAB, SCI_UNUSED, SCI_UNUSED);
	};

	void BackTab() const
	{
		execute(SCI_BACKTAB, SCI_UNUSED, SCI_UNUSED);
	};

	void NewLine() const
	{
		execute(SCI_NEWLINE, SCI_UNUSED, SCI_UNUSED);
	};

	void FormFeed() const
	{
		execute(SCI_FORMFEED, SCI_UNUSED, SCI_UNUSED);
	};

	void VCHome() const
	{
		execute(SCI_VCHOME, SCI_UNUSED, SCI_UNUSED);
	};

	void VCHomeExtend() const
	{
		execute(SCI_VCHOMEEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void ZoomIn() const
	{
		execute(SCI_ZOOMIN, SCI_UNUSED, SCI_UNUSED);
	};

	void ZoomOut() const
	{
		execute(SCI_ZOOMOUT, SCI_UNUSED, SCI_UNUSED);
	};

	void DelWordLeft() const
	{
		execute(SCI_DELWORDLEFT, SCI_UNUSED, SCI_UNUSED);
	};

	void DelWordRight() const
	{
		execute(SCI_DELWORDRIGHT, SCI_UNUSED, SCI_UNUSED);
	};

	void DelWordRightEnd() const
	{
		execute(SCI_DELWORDRIGHTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineCut() const
	{
		execute(SCI_LINECUT, SCI_UNUSED, SCI_UNUSED);
	};

	void LineDelete() const
	{
		execute(SCI_LINEDELETE, SCI_UNUSED, SCI_UNUSED);
	};

	void LineTranspose() const
	{
		execute(SCI_LINETRANSPOSE, SCI_UNUSED, SCI_UNUSED);
	};

	void LineDuplicate() const
	{
		execute(SCI_LINEDUPLICATE, SCI_UNUSED, SCI_UNUSED);
	};

	void LowerCase() const
	{
		execute(SCI_LOWERCASE, SCI_UNUSED, SCI_UNUSED);
	};

	void UpperCase() const
	{
		execute(SCI_UPPERCASE, SCI_UNUSED, SCI_UNUSED);
	};

	void LineScrollDown() const
	{
		execute(SCI_LINESCROLLDOWN, SCI_UNUSED, SCI_UNUSED);
	};

	void LineScrollUp() const
	{
		execute(SCI_LINESCROLLUP, SCI_UNUSED, SCI_UNUSED);
	};

	void DeleteBackNotLine() const
	{
		execute(SCI_DELETEBACKNOTLINE, SCI_UNUSED, SCI_UNUSED);
	};

	void HomeDisplay() const
	{
		execute(SCI_HOMEDISPLAY, SCI_UNUSED, SCI_UNUSED);
	};

	void HomeDisplayExtend() const
	{
		execute(SCI_HOMEDISPLAYEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineEndDisplay() const
	{
		execute(SCI_LINEENDDISPLAY, SCI_UNUSED, SCI_UNUSED);
	};

	void LineEndDisplayExtend() const
	{
		execute(SCI_LINEENDDISPLAYEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void HomeWrap() const
	{
		execute(SCI_HOMEWRAP, SCI_UNUSED, SCI_UNUSED);
	};

	void HomeWrapExtend() const
	{
		execute(SCI_HOMEWRAPEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineEndWrap() const
	{
		execute(SCI_LINEENDWRAP, SCI_UNUSED, SCI_UNUSED);
	};

	void LineEndWrapExtend() const
	{
		execute(SCI_LINEENDWRAPEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void VCHomeWrap() const
	{
		execute(SCI_VCHOMEWRAP, SCI_UNUSED, SCI_UNUSED);
	};

	void VCHomeWrapExtend() const
	{
		execute(SCI_VCHOMEWRAPEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineCopy() const
	{
		execute(SCI_LINECOPY, SCI_UNUSED, SCI_UNUSED);
	};

	void MoveCaretInsideView() const
	{
		execute(SCI_MOVECARETINSIDEVIEW, SCI_UNUSED, SCI_UNUSED);
	};

	int LineLength(int line) const
	{
		sptr_t res = execute(SCI_LINELENGTH, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void BraceHighlight(int pos1, int pos2) const
	{
		execute(SCI_BRACEHIGHLIGHT, pos1, pos2);
	};

	void BraceHighlightIndicator(bool useBraceHighlightIndicator, int indicator) const
	{
		execute(SCI_BRACEHIGHLIGHTINDICATOR, useBraceHighlightIndicator, indicator);
	};

	void BraceBadLight(int pos) const
	{
		execute(SCI_BRACEBADLIGHT, pos, SCI_UNUSED);
	};

	void BraceBadLightIndicator(bool useBraceBadLightIndicator, int indicator) const
	{
		execute(SCI_BRACEBADLIGHTINDICATOR, useBraceBadLightIndicator, indicator);
	};

	int BraceMatch(int pos) const
	{
		sptr_t res = execute(SCI_BRACEMATCH, pos, SCI_UNUSED);
		return static_cast<int>(res);
	};

	bool GetViewEOL() const
	{
		sptr_t res = execute(SCI_GETVIEWEOL, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetViewEOL(bool visible) const
	{
		execute(SCI_SETVIEWEOL, visible, SCI_UNUSED);
	};

	sptr_t GetDocPointer() const
	{
		return execute(SCI_GETDOCPOINTER, SCI_UNUSED, SCI_UNUSED);
	};

	void SetDocPointer(sptr_t pointer) const
	{
		execute(SCI_SETDOCPOINTER, SCI_UNUSED, pointer);
	};

	void SetModEventMask(int mask) const
	{
		execute(SCI_SETMODEVENTMASK, mask, SCI_UNUSED);
	};

	int GetEdgeColumn() const
	{
		sptr_t res = execute(SCI_GETEDGECOLUMN, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetEdgeColumn(int column) const
	{
		execute(SCI_SETEDGECOLUMN, column, SCI_UNUSED);
	};

	int GetEdgeMode() const
	{
		sptr_t res = execute(SCI_GETEDGEMODE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetEdgeMode(int mode) const
	{
		execute(SCI_SETEDGEMODE, mode, SCI_UNUSED);
	};

	Colour GetEdgeColour() const
	{
		sptr_t res = execute(SCI_GETEDGECOLOUR, SCI_UNUSED, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	void SetEdgeColour(Colour edgeColour) const
	{
		execute(SCI_SETEDGECOLOUR, edgeColour, SCI_UNUSED);
	};

	void SearchAnchor() const
	{
		execute(SCI_SEARCHANCHOR, SCI_UNUSED, SCI_UNUSED);
	};

	int SearchNext(int flags, const char* text) const
	{
		sptr_t res = execute(SCI_SEARCHNEXT, flags, text);
		return static_cast<int>(res);
	};

	int SearchNext(int flags, const std::string& text) const
	{
		sptr_t res = execute(SCI_SEARCHNEXT, flags, text.c_str());
		return static_cast<int>(res);
	};

	int SearchPrev(int flags, const char* text) const
	{
		sptr_t res = execute(SCI_SEARCHPREV, flags, text);
		return static_cast<int>(res);
	};

	int SearchPrev(int flags, const std::string& text) const
	{
		sptr_t res = execute(SCI_SEARCHPREV, flags, text.c_str());
		return static_cast<int>(res);
	};

	int LinesOnScreen() const
	{
		sptr_t res = execute(SCI_LINESONSCREEN, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void UsePopUp(bool allowPopUp) const
	{
		execute(SCI_USEPOPUP, allowPopUp, SCI_UNUSED);
	};

	bool SelectionIsRectangle() const
	{
		sptr_t res = execute(SCI_SELECTIONISRECTANGLE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetZoom(int zoom) const
	{
		execute(SCI_SETZOOM, zoom, SCI_UNUSED);
	};

	int GetZoom() const
	{
		sptr_t res = execute(SCI_GETZOOM, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	sptr_t CreateDocument() const
	{
		return execute(SCI_CREATEDOCUMENT, SCI_UNUSED, SCI_UNUSED);
	};

	void AddRefDocument(sptr_t doc) const
	{
		execute(SCI_ADDREFDOCUMENT, SCI_UNUSED, doc);
	};

	void ReleaseDocument(sptr_t doc) const
	{
		execute(SCI_RELEASEDOCUMENT, SCI_UNUSED, doc);
	};

	int GetModEventMask() const
	{
		sptr_t res = execute(SCI_GETMODEVENTMASK, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetFocus(bool focus) const
	{
		execute(SCI_SETFOCUS, focus, SCI_UNUSED);
	};

	bool GetFocus() const
	{
		sptr_t res = execute(SCI_GETFOCUS, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetStatus(int statusCode) const
	{
		execute(SCI_SETSTATUS, statusCode, SCI_UNUSED);
	};

	int GetStatus() const
	{
		sptr_t res = execute(SCI_GETSTATUS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetMouseDownCaptures(bool captures) const
	{
		execute(SCI_SETMOUSEDOWNCAPTURES, captures, SCI_UNUSED);
	};

	bool GetMouseDownCaptures() const
	{
		sptr_t res = execute(SCI_GETMOUSEDOWNCAPTURES, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetCursor(int cursorType) const
	{
		execute(SCI_SETCURSOR, cursorType, SCI_UNUSED);
	};

	int GetCursor() const
	{
		sptr_t res = execute(SCI_GETCURSOR, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetControlCharSymbol(int symbol) const
	{
		execute(SCI_SETCONTROLCHARSYMBOL, symbol, SCI_UNUSED);
	};

	int GetControlCharSymbol() const
	{
		sptr_t res = execute(SCI_GETCONTROLCHARSYMBOL, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void WordPartLeft() const
	{
		execute(SCI_WORDPARTLEFT, SCI_UNUSED, SCI_UNUSED);
	};

	void WordPartLeftExtend() const
	{
		execute(SCI_WORDPARTLEFTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void WordPartRight() const
	{
		execute(SCI_WORDPARTRIGHT, SCI_UNUSED, SCI_UNUSED);
	};

	void WordPartRightExtend() const
	{
		execute(SCI_WORDPARTRIGHTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void SetVisiblePolicy(int visiblePolicy, int visibleSlop) const
	{
		execute(SCI_SETVISIBLEPOLICY, visiblePolicy, visibleSlop);
	};

	void DelLineLeft() const
	{
		execute(SCI_DELLINELEFT, SCI_UNUSED, SCI_UNUSED);
	};

	void DelLineRight() const
	{
		execute(SCI_DELLINERIGHT, SCI_UNUSED, SCI_UNUSED);
	};

	void SetXOffset(int newOffset) const
	{
		execute(SCI_SETXOFFSET, newOffset, SCI_UNUSED);
	};

	int GetXOffset() const
	{
		sptr_t res = execute(SCI_GETXOFFSET, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void ChooseCaretX() const
	{
		execute(SCI_CHOOSECARETX, SCI_UNUSED, SCI_UNUSED);
	};

	void GrabFocus() const
	{
		execute(SCI_GRABFOCUS, SCI_UNUSED, SCI_UNUSED);
	};

	void SetXCaretPolicy(int caretPolicy, int caretSlop) const
	{
		execute(SCI_SETXCARETPOLICY, caretPolicy, caretSlop);
	};

	void SetYCaretPolicy(int caretPolicy, int caretSlop) const
	{
		execute(SCI_SETYCARETPOLICY, caretPolicy, caretSlop);
	};

	void SetPrintWrapMode(int mode) const
	{
		execute(SCI_SETPRINTWRAPMODE, mode, SCI_UNUSED);
	};

	int GetPrintWrapMode() const
	{
		sptr_t res = execute(SCI_GETPRINTWRAPMODE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetHotspotActiveFore(bool useSetting, Colour fore) const
	{
		execute(SCI_SETHOTSPOTACTIVEFORE, useSetting, fore);
	};

	Colour GetHotspotActiveFore() const
	{
		sptr_t res = execute(SCI_GETHOTSPOTACTIVEFORE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	void SetHotspotActiveBack(bool useSetting, Colour back) const
	{
		execute(SCI_SETHOTSPOTACTIVEBACK, useSetting, back);
	};

	Colour GetHotspotActiveBack() const
	{
		sptr_t res = execute(SCI_GETHOTSPOTACTIVEBACK, SCI_UNUSED, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	void SetHotspotActiveUnderline(bool underline) const
	{
		execute(SCI_SETHOTSPOTACTIVEUNDERLINE, underline, SCI_UNUSED);
	};

	bool GetHotspotActiveUnderline() const
	{
		sptr_t res = execute(SCI_GETHOTSPOTACTIVEUNDERLINE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetHotspotSingleLine(bool singleLine) const
	{
		execute(SCI_SETHOTSPOTSINGLELINE, singleLine, SCI_UNUSED);
	};

	bool GetHotspotSingleLine() const
	{
		sptr_t res = execute(SCI_GETHOTSPOTSINGLELINE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void ParaDown() const
	{
		execute(SCI_PARADOWN, SCI_UNUSED, SCI_UNUSED);
	};

	void ParaDownExtend() const
	{
		execute(SCI_PARADOWNEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void ParaUp() const
	{
		execute(SCI_PARAUP, SCI_UNUSED, SCI_UNUSED);
	};

	void ParaUpExtend() const
	{
		execute(SCI_PARAUPEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	int PositionBefore(int pos) const
	{
		sptr_t res = execute(SCI_POSITIONBEFORE, pos, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int PositionAfter(int pos) const
	{
		sptr_t res = execute(SCI_POSITIONAFTER, pos, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int PositionRelative(int pos, int relative) const
	{
		sptr_t res = execute(SCI_POSITIONRELATIVE, pos, relative);
		return static_cast<int>(res);
	};

	void CopyRange(int start, int end) const
	{
		execute(SCI_COPYRANGE, start, end);
	};

	void CopyText(int length, const char* text) const
	{
		execute(SCI_COPYTEXT, length, text);
	};

	void CopyText(const std::string& text) const
	{
		execute(SCI_COPYTEXT, text.length(), text.c_str());
	};

	void SetSelectionMode(int mode) const
	{
		execute(SCI_SETSELECTIONMODE, mode, SCI_UNUSED);
	};

	int GetSelectionMode() const
	{
		sptr_t res = execute(SCI_GETSELECTIONMODE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetLineSelStartPosition(int line) const
	{
		sptr_t res = execute(SCI_GETLINESELSTARTPOSITION, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetLineSelEndPosition(int line) const
	{
		sptr_t res = execute(SCI_GETLINESELENDPOSITION, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void LineDownRectExtend() const
	{
		execute(SCI_LINEDOWNRECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineUpRectExtend() const
	{
		execute(SCI_LINEUPRECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void CharLeftRectExtend() const
	{
		execute(SCI_CHARLEFTRECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void CharRightRectExtend() const
	{
		execute(SCI_CHARRIGHTRECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void HomeRectExtend() const
	{
		execute(SCI_HOMERECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void VCHomeRectExtend() const
	{
		execute(SCI_VCHOMERECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void LineEndRectExtend() const
	{
		execute(SCI_LINEENDRECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void PageUpRectExtend() const
	{
		execute(SCI_PAGEUPRECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void PageDownRectExtend() const
	{
		execute(SCI_PAGEDOWNRECTEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void StutteredPageUp() const
	{
		execute(SCI_STUTTEREDPAGEUP, SCI_UNUSED, SCI_UNUSED);
	};

	void StutteredPageUpExtend() const
	{
		execute(SCI_STUTTEREDPAGEUPEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void StutteredPageDown() const
	{
		execute(SCI_STUTTEREDPAGEDOWN, SCI_UNUSED, SCI_UNUSED);
	};

	void StutteredPageDownExtend() const
	{
		execute(SCI_STUTTEREDPAGEDOWNEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void WordLeftEnd() const
	{
		execute(SCI_WORDLEFTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void WordLeftEndExtend() const
	{
		execute(SCI_WORDLEFTENDEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void WordRightEnd() const
	{
		execute(SCI_WORDRIGHTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void WordRightEndExtend() const
	{
		execute(SCI_WORDRIGHTENDEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	void SetWhitespaceChars(const char* characters) const
	{
		execute(SCI_SETWHITESPACECHARS, SCI_UNUSED, characters);
	};

	void SetWhitespaceChars(const std::string& characters) const
	{
		execute(SCI_SETWHITESPACECHARS, SCI_UNUSED, characters.c_str());
	};

	int GetWhitespaceChars(char* characters) const
	{
		sptr_t res = execute(SCI_GETWHITESPACECHARS, SCI_UNUSED, characters);
		return static_cast<int>(res);
	};

	std::string GetWhitespaceChars() const
	{
		auto size = execute(SCI_GETWHITESPACECHARS, SCI_UNUSED, NULL);
		std::string characters(size + 1, '\0');
		execute(SCI_GETWHITESPACECHARS, SCI_UNUSED, &characters[0]);
		trim(characters);
		return characters;
	};

	void SetPunctuationChars(const char* characters) const
	{
		execute(SCI_SETPUNCTUATIONCHARS, SCI_UNUSED, characters);
	};

	void SetPunctuationChars(const std::string& characters) const
	{
		execute(SCI_SETPUNCTUATIONCHARS, SCI_UNUSED, characters.c_str());
	};

	int GetPunctuationChars(char* characters) const
	{
		sptr_t res = execute(SCI_GETPUNCTUATIONCHARS, SCI_UNUSED, characters);
		return static_cast<int>(res);
	};

	std::string GetPunctuationChars() const
	{
		auto size = execute(SCI_GETPUNCTUATIONCHARS, SCI_UNUSED, NULL);
		std::string characters(size + 1, '\0');
		execute(SCI_GETPUNCTUATIONCHARS, SCI_UNUSED, &characters[0]);
		trim(characters);
		return characters;
	};

	void SetCharsDefault() const
	{
		execute(SCI_SETCHARSDEFAULT, SCI_UNUSED, SCI_UNUSED);
	};

	int AutoCGetCurrent() const
	{
		sptr_t res = execute(SCI_AUTOCGETCURRENT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int AutoCGetCurrentText(char* s) const
	{
		sptr_t res = execute(SCI_AUTOCGETCURRENTTEXT, SCI_UNUSED, s);
		return static_cast<int>(res);
	};

	std::string AutoCGetCurrentText() const
	{
		auto size = execute(SCI_AUTOCGETCURRENTTEXT, SCI_UNUSED, NULL);
		std::string s(size + 1, '\0');
		execute(SCI_AUTOCGETCURRENTTEXT, SCI_UNUSED, &s[0]);
		trim(s);
		return s;
	};

	void AutoCSetCaseInsensitiveBehaviour(int behaviour) const
	{
		execute(SCI_AUTOCSETCASEINSENSITIVEBEHAVIOUR, behaviour, SCI_UNUSED);
	};

	int AutoCGetCaseInsensitiveBehaviour() const
	{
		sptr_t res = execute(SCI_AUTOCGETCASEINSENSITIVEBEHAVIOUR, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AutoCSetMulti(int multi) const
	{
		execute(SCI_AUTOCSETMULTI, multi, SCI_UNUSED);
	};

	int AutoCGetMulti() const
	{
		sptr_t res = execute(SCI_AUTOCGETMULTI, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AutoCSetOrder(int order) const
	{
		execute(SCI_AUTOCSETORDER, order, SCI_UNUSED);
	};

	int AutoCGetOrder() const
	{
		sptr_t res = execute(SCI_AUTOCGETORDER, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void Allocate(int bytes) const
	{
		execute(SCI_ALLOCATE, bytes, SCI_UNUSED);
	};

	int TargetAsUTF8(char* s) const
	{
		sptr_t res = execute(SCI_TARGETASUTF8, SCI_UNUSED, s);
		return static_cast<int>(res);
	};

	std::string TargetAsUTF8() const
	{
		auto size = execute(SCI_TARGETASUTF8, SCI_UNUSED, NULL);
		std::string s(size + 1, '\0');
		execute(SCI_TARGETASUTF8, SCI_UNUSED, &s[0]);
		trim(s);
		return s;
	};

	void SetLengthForEncode(int bytes) const
	{
		execute(SCI_SETLENGTHFORENCODE, bytes, SCI_UNUSED);
	};

	int EncodedFromUTF8(const char* utf8, char* encoded) const
	{
		sptr_t res = execute(SCI_ENCODEDFROMUTF8, utf8, encoded);
		return static_cast<int>(res);
	};

	std::string EncodedFromUTF8(const std::string& utf8) const
	{
		auto size = execute(SCI_ENCODEDFROMUTF8, utf8.c_str(), NULL);
		std::string encoded(size + 1, '\0');
		execute(SCI_ENCODEDFROMUTF8, utf8.c_str(), &encoded[0]);
		trim(encoded);
		return encoded;
	};

	int FindColumn(int line, int column) const
	{
		sptr_t res = execute(SCI_FINDCOLUMN, line, column);
		return static_cast<int>(res);
	};

	int GetCaretSticky() const
	{
		sptr_t res = execute(SCI_GETCARETSTICKY, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetCaretSticky(int useCaretStickyBehaviour) const
	{
		execute(SCI_SETCARETSTICKY, useCaretStickyBehaviour, SCI_UNUSED);
	};

	void ToggleCaretSticky() const
	{
		execute(SCI_TOGGLECARETSTICKY, SCI_UNUSED, SCI_UNUSED);
	};

	void SetPasteConvertEndings(bool convert) const
	{
		execute(SCI_SETPASTECONVERTENDINGS, convert, SCI_UNUSED);
	};

	bool GetPasteConvertEndings() const
	{
		sptr_t res = execute(SCI_GETPASTECONVERTENDINGS, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SelectionDuplicate() const
	{
		execute(SCI_SELECTIONDUPLICATE, SCI_UNUSED, SCI_UNUSED);
	};

	void SetCaretLineBackAlpha(int alpha) const
	{
		execute(SCI_SETCARETLINEBACKALPHA, alpha, SCI_UNUSED);
	};

	int GetCaretLineBackAlpha() const
	{
		sptr_t res = execute(SCI_GETCARETLINEBACKALPHA, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetCaretStyle(int caretStyle) const
	{
		execute(SCI_SETCARETSTYLE, caretStyle, SCI_UNUSED);
	};

	int GetCaretStyle() const
	{
		sptr_t res = execute(SCI_GETCARETSTYLE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetIndicatorCurrent(int indicator) const
	{
		execute(SCI_SETINDICATORCURRENT, indicator, SCI_UNUSED);
	};

	int GetIndicatorCurrent() const
	{
		sptr_t res = execute(SCI_GETINDICATORCURRENT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetIndicatorValue(int value) const
	{
		execute(SCI_SETINDICATORVALUE, value, SCI_UNUSED);
	};

	int GetIndicatorValue() const
	{
		sptr_t res = execute(SCI_GETINDICATORVALUE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void IndicatorFillRange(int position, int fillLength) const
	{
		execute(SCI_INDICATORFILLRANGE, position, fillLength);
	};

	void IndicatorClearRange(int position, int clearLength) const
	{
		execute(SCI_INDICATORCLEARRANGE, position, clearLength);
	};

	int IndicatorAllOnFor(int position) const
	{
		sptr_t res = execute(SCI_INDICATORALLONFOR, position, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int IndicatorValueAt(int indicator, int position) const
	{
		sptr_t res = execute(SCI_INDICATORVALUEAT, indicator, position);
		return static_cast<int>(res);
	};

	int IndicatorStart(int indicator, int position) const
	{
		sptr_t res = execute(SCI_INDICATORSTART, indicator, position);
		return static_cast<int>(res);
	};

	int IndicatorEnd(int indicator, int position) const
	{
		sptr_t res = execute(SCI_INDICATOREND, indicator, position);
		return static_cast<int>(res);
	};

	void SetPositionCache(int size) const
	{
		execute(SCI_SETPOSITIONCACHE, size, SCI_UNUSED);
	};

	int GetPositionCache() const
	{
		sptr_t res = execute(SCI_GETPOSITIONCACHE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void CopyAllowLine() const
	{
		execute(SCI_COPYALLOWLINE, SCI_UNUSED, SCI_UNUSED);
	};

	const char* GetCharacterPointer() const
	{
		sptr_t res = execute(SCI_GETCHARACTERPOINTER, SCI_UNUSED, SCI_UNUSED);
		return reinterpret_cast<const char*>(res);
	};

	const char* GetRangePointer(int position, int rangeLength) const
	{
		sptr_t res = execute(SCI_GETRANGEPOINTER, position, rangeLength);
		return reinterpret_cast<const char*>(res);
	};

	int GetGapPosition() const
	{
		sptr_t res = execute(SCI_GETGAPPOSITION, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void IndicSetAlpha(int indicator, int alpha) const
	{
		execute(SCI_INDICSETALPHA, indicator, alpha);
	};

	int IndicGetAlpha(int indicator) const
	{
		sptr_t res = execute(SCI_INDICGETALPHA, indicator, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void IndicSetOutlineAlpha(int indicator, int alpha) const
	{
		execute(SCI_INDICSETOUTLINEALPHA, indicator, alpha);
	};

	int IndicGetOutlineAlpha(int indicator) const
	{
		sptr_t res = execute(SCI_INDICGETOUTLINEALPHA, indicator, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetExtraAscent(int extraAscent) const
	{
		execute(SCI_SETEXTRAASCENT, extraAscent, SCI_UNUSED);
	};

	int GetExtraAscent() const
	{
		sptr_t res = execute(SCI_GETEXTRAASCENT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetExtraDescent(int extraDescent) const
	{
		execute(SCI_SETEXTRADESCENT, extraDescent, SCI_UNUSED);
	};

	int GetExtraDescent() const
	{
		sptr_t res = execute(SCI_GETEXTRADESCENT, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int MarkerSymbolDefined(int markerNumber) const
	{
		sptr_t res = execute(SCI_MARKERSYMBOLDEFINED, markerNumber, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void MarginSetText(int line, const char* text) const
	{
		execute(SCI_MARGINSETTEXT, line, text);
	};

	void MarginSetText(int line, const std::string& text) const
	{
		execute(SCI_MARGINSETTEXT, line, text.c_str());
	};

	int MarginGetText(int line, char* text) const
	{
		sptr_t res = execute(SCI_MARGINGETTEXT, line, text);
		return static_cast<int>(res);
	};

	std::string MarginGetText(int line) const
	{
		auto size = execute(SCI_MARGINGETTEXT, line, NULL);
		std::string text(size + 1, '\0');
		execute(SCI_MARGINGETTEXT, line, &text[0]);
		trim(text);
		return text;
	};

	void MarginSetStyle(int line, int style) const
	{
		execute(SCI_MARGINSETSTYLE, line, style);
	};

	int MarginGetStyle(int line) const
	{
		sptr_t res = execute(SCI_MARGINGETSTYLE, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void MarginSetStyles(int line, const char* styles) const
	{
		execute(SCI_MARGINSETSTYLES, line, styles);
	};

	void MarginSetStyles(int line, const std::string& styles) const
	{
		execute(SCI_MARGINSETSTYLES, line, styles.c_str());
	};

	int MarginGetStyles(int line, char* styles) const
	{
		sptr_t res = execute(SCI_MARGINGETSTYLES, line, styles);
		return static_cast<int>(res);
	};

	std::string MarginGetStyles(int line) const
	{
		auto size = execute(SCI_MARGINGETSTYLES, line, NULL);
		std::string styles(size + 1, '\0');
		execute(SCI_MARGINGETSTYLES, line, &styles[0]);
		trim(styles);
		return styles;
	};

	void MarginTextClearAll() const
	{
		execute(SCI_MARGINTEXTCLEARALL, SCI_UNUSED, SCI_UNUSED);
	};

	void MarginSetStyleOffset(int style) const
	{
		execute(SCI_MARGINSETSTYLEOFFSET, style, SCI_UNUSED);
	};

	int MarginGetStyleOffset() const
	{
		sptr_t res = execute(SCI_MARGINGETSTYLEOFFSET, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetMarginOptions(int marginOptions) const
	{
		execute(SCI_SETMARGINOPTIONS, marginOptions, SCI_UNUSED);
	};

	int GetMarginOptions() const
	{
		sptr_t res = execute(SCI_GETMARGINOPTIONS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AnnotationSetText(int line, const char* text) const
	{
		execute(SCI_ANNOTATIONSETTEXT, line, text);
	};

	void AnnotationSetText(int line, const std::string& text) const
	{
		execute(SCI_ANNOTATIONSETTEXT, line, text.c_str());
	};

	int AnnotationGetText(int line, char* text) const
	{
		sptr_t res = execute(SCI_ANNOTATIONGETTEXT, line, text);
		return static_cast<int>(res);
	};

	std::string AnnotationGetText(int line) const
	{
		auto size = execute(SCI_ANNOTATIONGETTEXT, line, NULL);
		std::string text(size + 1, '\0');
		execute(SCI_ANNOTATIONGETTEXT, line, &text[0]);
		trim(text);
		return text;
	};

	void AnnotationSetStyle(int line, int style) const
	{
		execute(SCI_ANNOTATIONSETSTYLE, line, style);
	};

	int AnnotationGetStyle(int line) const
	{
		sptr_t res = execute(SCI_ANNOTATIONGETSTYLE, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AnnotationSetStyles(int line, const char* styles) const
	{
		execute(SCI_ANNOTATIONSETSTYLES, line, styles);
	};

	void AnnotationSetStyles(int line, const std::string& styles) const
	{
		execute(SCI_ANNOTATIONSETSTYLES, line, styles.c_str());
	};

	int AnnotationGetStyles(int line, char* styles) const
	{
		sptr_t res = execute(SCI_ANNOTATIONGETSTYLES, line, styles);
		return static_cast<int>(res);
	};

	std::string AnnotationGetStyles(int line) const
	{
		auto size = execute(SCI_ANNOTATIONGETSTYLES, line, NULL);
		std::string styles(size + 1, '\0');
		execute(SCI_ANNOTATIONGETSTYLES, line, &styles[0]);
		trim(styles);
		return styles;
	};

	int AnnotationGetLines(int line) const
	{
		sptr_t res = execute(SCI_ANNOTATIONGETLINES, line, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AnnotationClearAll() const
	{
		execute(SCI_ANNOTATIONCLEARALL, SCI_UNUSED, SCI_UNUSED);
	};

	void AnnotationSetVisible(int visible) const
	{
		execute(SCI_ANNOTATIONSETVISIBLE, visible, SCI_UNUSED);
	};

	int AnnotationGetVisible() const
	{
		sptr_t res = execute(SCI_ANNOTATIONGETVISIBLE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AnnotationSetStyleOffset(int style) const
	{
		execute(SCI_ANNOTATIONSETSTYLEOFFSET, style, SCI_UNUSED);
	};

	int AnnotationGetStyleOffset() const
	{
		sptr_t res = execute(SCI_ANNOTATIONGETSTYLEOFFSET, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void ReleaseAllExtendedStyles() const
	{
		execute(SCI_RELEASEALLEXTENDEDSTYLES, SCI_UNUSED, SCI_UNUSED);
	};

	int AllocateExtendedStyles(int numberStyles) const
	{
		sptr_t res = execute(SCI_ALLOCATEEXTENDEDSTYLES, numberStyles, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void AddUndoAction(int token, int flags) const
	{
		execute(SCI_ADDUNDOACTION, token, flags);
	};

	int CharPositionFromPoint(int x, int y) const
	{
		sptr_t res = execute(SCI_CHARPOSITIONFROMPOINT, x, y);
		return static_cast<int>(res);
	};

	int CharPositionFromPointClose(int x, int y) const
	{
		sptr_t res = execute(SCI_CHARPOSITIONFROMPOINTCLOSE, x, y);
		return static_cast<int>(res);
	};

	void SetMouseSelectionRectangularSwitch(bool mouseSelectionRectangularSwitch) const
	{
		execute(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, mouseSelectionRectangularSwitch, SCI_UNUSED);
	};

	bool GetMouseSelectionRectangularSwitch() const
	{
		sptr_t res = execute(SCI_GETMOUSESELECTIONRECTANGULARSWITCH, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetMultipleSelection(bool multipleSelection) const
	{
		execute(SCI_SETMULTIPLESELECTION, multipleSelection, SCI_UNUSED);
	};

	bool GetMultipleSelection() const
	{
		sptr_t res = execute(SCI_GETMULTIPLESELECTION, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetAdditionalSelectionTyping(bool additionalSelectionTyping) const
	{
		execute(SCI_SETADDITIONALSELECTIONTYPING, additionalSelectionTyping, SCI_UNUSED);
	};

	bool GetAdditionalSelectionTyping() const
	{
		sptr_t res = execute(SCI_GETADDITIONALSELECTIONTYPING, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetAdditionalCaretsBlink(bool additionalCaretsBlink) const
	{
		execute(SCI_SETADDITIONALCARETSBLINK, additionalCaretsBlink, SCI_UNUSED);
	};

	bool GetAdditionalCaretsBlink() const
	{
		sptr_t res = execute(SCI_GETADDITIONALCARETSBLINK, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetAdditionalCaretsVisible(bool additionalCaretsBlink) const
	{
		execute(SCI_SETADDITIONALCARETSVISIBLE, additionalCaretsBlink, SCI_UNUSED);
	};

	bool GetAdditionalCaretsVisible() const
	{
		sptr_t res = execute(SCI_GETADDITIONALCARETSVISIBLE, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	int GetSelections() const
	{
		sptr_t res = execute(SCI_GETSELECTIONS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	bool GetSelectionEmpty() const
	{
		sptr_t res = execute(SCI_GETSELECTIONEMPTY, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void ClearSelections() const
	{
		execute(SCI_CLEARSELECTIONS, SCI_UNUSED, SCI_UNUSED);
	};

	int SetSelection(int caret, int anchor) const
	{
		sptr_t res = execute(SCI_SETSELECTION, caret, anchor);
		return static_cast<int>(res);
	};

	int AddSelection(int caret, int anchor) const
	{
		sptr_t res = execute(SCI_ADDSELECTION, caret, anchor);
		return static_cast<int>(res);
	};

	void DropSelectionN(int selection) const
	{
		execute(SCI_DROPSELECTIONN, selection, SCI_UNUSED);
	};

	void SetMainSelection(int selection) const
	{
		execute(SCI_SETMAINSELECTION, selection, SCI_UNUSED);
	};

	int GetMainSelection() const
	{
		sptr_t res = execute(SCI_GETMAINSELECTION, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetSelectionNCaret(int selection, int pos) const
	{
		execute(SCI_SETSELECTIONNCARET, selection, pos);
	};

	int GetSelectionNCaret(int selection) const
	{
		sptr_t res = execute(SCI_GETSELECTIONNCARET, selection, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetSelectionNAnchor(int selection, int posAnchor) const
	{
		execute(SCI_SETSELECTIONNANCHOR, selection, posAnchor);
	};

	int GetSelectionNAnchor(int selection) const
	{
		sptr_t res = execute(SCI_GETSELECTIONNANCHOR, selection, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetSelectionNCaretVirtualSpace(int selection, int space) const
	{
		execute(SCI_SETSELECTIONNCARETVIRTUALSPACE, selection, space);
	};

	int GetSelectionNCaretVirtualSpace(int selection) const
	{
		sptr_t res = execute(SCI_GETSELECTIONNCARETVIRTUALSPACE, selection, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetSelectionNAnchorVirtualSpace(int selection, int space) const
	{
		execute(SCI_SETSELECTIONNANCHORVIRTUALSPACE, selection, space);
	};

	int GetSelectionNAnchorVirtualSpace(int selection) const
	{
		sptr_t res = execute(SCI_GETSELECTIONNANCHORVIRTUALSPACE, selection, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetSelectionNStart(int selection, int pos) const
	{
		execute(SCI_SETSELECTIONNSTART, selection, pos);
	};

	int GetSelectionNStart(int selection) const
	{
		sptr_t res = execute(SCI_GETSELECTIONNSTART, selection, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetSelectionNEnd(int selection, int pos) const
	{
		execute(SCI_SETSELECTIONNEND, selection, pos);
	};

	int GetSelectionNEnd(int selection) const
	{
		sptr_t res = execute(SCI_GETSELECTIONNEND, selection, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetRectangularSelectionCaret(int pos) const
	{
		execute(SCI_SETRECTANGULARSELECTIONCARET, pos, SCI_UNUSED);
	};

	int GetRectangularSelectionCaret() const
	{
		sptr_t res = execute(SCI_GETRECTANGULARSELECTIONCARET, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetRectangularSelectionAnchor(int posAnchor) const
	{
		execute(SCI_SETRECTANGULARSELECTIONANCHOR, posAnchor, SCI_UNUSED);
	};

	int GetRectangularSelectionAnchor() const
	{
		sptr_t res = execute(SCI_GETRECTANGULARSELECTIONANCHOR, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetRectangularSelectionCaretVirtualSpace(int space) const
	{
		execute(SCI_SETRECTANGULARSELECTIONCARETVIRTUALSPACE, space, SCI_UNUSED);
	};

	int GetRectangularSelectionCaretVirtualSpace() const
	{
		sptr_t res = execute(SCI_GETRECTANGULARSELECTIONCARETVIRTUALSPACE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetRectangularSelectionAnchorVirtualSpace(int space) const
	{
		execute(SCI_SETRECTANGULARSELECTIONANCHORVIRTUALSPACE, space, SCI_UNUSED);
	};

	int GetRectangularSelectionAnchorVirtualSpace() const
	{
		sptr_t res = execute(SCI_GETRECTANGULARSELECTIONANCHORVIRTUALSPACE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetVirtualSpaceOptions(int virtualSpaceOptions) const
	{
		execute(SCI_SETVIRTUALSPACEOPTIONS, virtualSpaceOptions, SCI_UNUSED);
	};

	int GetVirtualSpaceOptions() const
	{
		sptr_t res = execute(SCI_GETVIRTUALSPACEOPTIONS, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetRectangularSelectionModifier(int modifier) const
	{
		execute(SCI_SETRECTANGULARSELECTIONMODIFIER, modifier, SCI_UNUSED);
	};

	int GetRectangularSelectionModifier() const
	{
		sptr_t res = execute(SCI_GETRECTANGULARSELECTIONMODIFIER, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetAdditionalSelFore(Colour fore) const
	{
		execute(SCI_SETADDITIONALSELFORE, fore, SCI_UNUSED);
	};

	void SetAdditionalSelBack(Colour back) const
	{
		execute(SCI_SETADDITIONALSELBACK, back, SCI_UNUSED);
	};

	void SetAdditionalSelAlpha(int alpha) const
	{
		execute(SCI_SETADDITIONALSELALPHA, alpha, SCI_UNUSED);
	};

	int GetAdditionalSelAlpha() const
	{
		sptr_t res = execute(SCI_GETADDITIONALSELALPHA, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetAdditionalCaretFore(Colour fore) const
	{
		execute(SCI_SETADDITIONALCARETFORE, fore, SCI_UNUSED);
	};

	Colour GetAdditionalCaretFore() const
	{
		sptr_t res = execute(SCI_GETADDITIONALCARETFORE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<Colour>(res);
	};

	void RotateSelection() const
	{
		execute(SCI_ROTATESELECTION, SCI_UNUSED, SCI_UNUSED);
	};

	void SwapMainAnchorCaret() const
	{
		execute(SCI_SWAPMAINANCHORCARET, SCI_UNUSED, SCI_UNUSED);
	};

	int ChangeLexerState(int start, int end) const
	{
		sptr_t res = execute(SCI_CHANGELEXERSTATE, start, end);
		return static_cast<int>(res);
	};

	int ContractedFoldNext(int lineStart) const
	{
		sptr_t res = execute(SCI_CONTRACTEDFOLDNEXT, lineStart, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void VerticalCentreCaret() const
	{
		execute(SCI_VERTICALCENTRECARET, SCI_UNUSED, SCI_UNUSED);
	};

	void MoveSelectedLinesUp() const
	{
		execute(SCI_MOVESELECTEDLINESUP, SCI_UNUSED, SCI_UNUSED);
	};

	void MoveSelectedLinesDown() const
	{
		execute(SCI_MOVESELECTEDLINESDOWN, SCI_UNUSED, SCI_UNUSED);
	};

	void SetIdentifier(int identifier) const
	{
		execute(SCI_SETIDENTIFIER, identifier, SCI_UNUSED);
	};

	int GetIdentifier() const
	{
		sptr_t res = execute(SCI_GETIDENTIFIER, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void RGBAImageSetWidth(int width) const
	{
		execute(SCI_RGBAIMAGESETWIDTH, width, SCI_UNUSED);
	};

	void RGBAImageSetHeight(int height) const
	{
		execute(SCI_RGBAIMAGESETHEIGHT, height, SCI_UNUSED);
	};

	void RGBAImageSetScale(int scalePercent) const
	{
		execute(SCI_RGBAIMAGESETSCALE, scalePercent, SCI_UNUSED);
	};

	void MarkerDefineRGBAImage(int markerNumber, const char* pixels) const
	{
		execute(SCI_MARKERDEFINERGBAIMAGE, markerNumber, pixels);
	};

	void MarkerDefineRGBAImage(int markerNumber, const std::string& pixels) const
	{
		execute(SCI_MARKERDEFINERGBAIMAGE, markerNumber, pixels.c_str());
	};

	void RegisterRGBAImage(int type, const char* pixels) const
	{
		execute(SCI_REGISTERRGBAIMAGE, type, pixels);
	};

	void RegisterRGBAImage(int type, const std::string& pixels) const
	{
		execute(SCI_REGISTERRGBAIMAGE, type, pixels.c_str());
	};

	void ScrollToStart() const
	{
		execute(SCI_SCROLLTOSTART, SCI_UNUSED, SCI_UNUSED);
	};

	void ScrollToEnd() const
	{
		execute(SCI_SCROLLTOEND, SCI_UNUSED, SCI_UNUSED);
	};

	void SetTechnology(int technology) const
	{
		execute(SCI_SETTECHNOLOGY, technology, SCI_UNUSED);
	};

	int GetTechnology() const
	{
		sptr_t res = execute(SCI_GETTECHNOLOGY, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int CreateLoader(int bytes) const
	{
		sptr_t res = execute(SCI_CREATELOADER, bytes, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void FindIndicatorShow(int start, int end) const
	{
		execute(SCI_FINDINDICATORSHOW, start, end);
	};

	void FindIndicatorFlash(int start, int end) const
	{
		execute(SCI_FINDINDICATORFLASH, start, end);
	};

	void FindIndicatorHide() const
	{
		execute(SCI_FINDINDICATORHIDE, SCI_UNUSED, SCI_UNUSED);
	};

	void VCHomeDisplay() const
	{
		execute(SCI_VCHOMEDISPLAY, SCI_UNUSED, SCI_UNUSED);
	};

	void VCHomeDisplayExtend() const
	{
		execute(SCI_VCHOMEDISPLAYEXTEND, SCI_UNUSED, SCI_UNUSED);
	};

	bool GetCaretLineVisibleAlways() const
	{
		sptr_t res = execute(SCI_GETCARETLINEVISIBLEALWAYS, SCI_UNUSED, SCI_UNUSED);
		return res != 0;
	};

	void SetCaretLineVisibleAlways(bool alwaysVisible) const
	{
		execute(SCI_SETCARETLINEVISIBLEALWAYS, alwaysVisible, SCI_UNUSED);
	};

	void SetLineEndTypesAllowed(int lineEndBitSet) const
	{
		execute(SCI_SETLINEENDTYPESALLOWED, lineEndBitSet, SCI_UNUSED);
	};

	int GetLineEndTypesAllowed() const
	{
		sptr_t res = execute(SCI_GETLINEENDTYPESALLOWED, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetLineEndTypesActive() const
	{
		sptr_t res = execute(SCI_GETLINEENDTYPESACTIVE, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void SetRepresentation(const char* encodedCharacter, const char* representation) const
	{
		execute(SCI_SETREPRESENTATION, encodedCharacter, representation);
	};

	void SetRepresentation(const std::string& encodedCharacter, const std::string& representation) const
	{
		execute(SCI_SETREPRESENTATION, encodedCharacter.c_str(), representation.c_str());
	};

	int GetRepresentation(const char* encodedCharacter, char* representation) const
	{
		sptr_t res = execute(SCI_GETREPRESENTATION, encodedCharacter, representation);
		return static_cast<int>(res);
	};

	std::string GetRepresentation(const std::string& encodedCharacter) const
	{
		auto size = execute(SCI_GETREPRESENTATION, encodedCharacter.c_str(), NULL);
		std::string representation(size + 1, '\0');
		execute(SCI_GETREPRESENTATION, encodedCharacter.c_str(), &representation[0]);
		trim(representation);
		return representation;
	};

	void ClearRepresentation(const char* encodedCharacter) const
	{
		execute(SCI_CLEARREPRESENTATION, encodedCharacter, SCI_UNUSED);
	};

	void ClearRepresentation(const std::string& encodedCharacter) const
	{
		execute(SCI_CLEARREPRESENTATION, encodedCharacter.c_str(), SCI_UNUSED);
	};

	void StartRecord() const
	{
		execute(SCI_STARTRECORD, SCI_UNUSED, SCI_UNUSED);
	};

	void StopRecord() const
	{
		execute(SCI_STOPRECORD, SCI_UNUSED, SCI_UNUSED);
	};

	void SetLexer(int lexer) const
	{
		execute(SCI_SETLEXER, lexer, SCI_UNUSED);
	};

	int GetLexer() const
	{
		sptr_t res = execute(SCI_GETLEXER, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void Colourise(int start, int end) const
	{
		execute(SCI_COLOURISE, start, end);
	};

	void SetProperty(const char* key, const char* value) const
	{
		execute(SCI_SETPROPERTY, key, value);
	};

	void SetProperty(const std::string& key, const std::string& value) const
	{
		execute(SCI_SETPROPERTY, key.c_str(), value.c_str());
	};

	void SetKeyWords(int keywordSet, const char* keyWords) const
	{
		execute(SCI_SETKEYWORDS, keywordSet, keyWords);
	};

	void SetKeyWords(int keywordSet, const std::string& keyWords) const
	{
		execute(SCI_SETKEYWORDS, keywordSet, keyWords.c_str());
	};

	void SetLexerLanguage(const char* language) const
	{
		execute(SCI_SETLEXERLANGUAGE, SCI_UNUSED, language);
	};

	void SetLexerLanguage(const std::string& language) const
	{
		execute(SCI_SETLEXERLANGUAGE, SCI_UNUSED, language.c_str());
	};

	void LoadLexerLibrary(const char* path) const
	{
		execute(SCI_LOADLEXERLIBRARY, SCI_UNUSED, path);
	};

	void LoadLexerLibrary(const std::string& path) const
	{
		execute(SCI_LOADLEXERLIBRARY, SCI_UNUSED, path.c_str());
	};

	int GetProperty(const char* key, char* buf) const
	{
		sptr_t res = execute(SCI_GETPROPERTY, key, buf);
		return static_cast<int>(res);
	};

	std::string GetProperty(const std::string& key) const
	{
		auto size = execute(SCI_GETPROPERTY, key.c_str(), NULL);
		std::string buf(size + 1, '\0');
		execute(SCI_GETPROPERTY, key.c_str(), &buf[0]);
		trim(buf);
		return buf;
	};

	int GetPropertyExpanded(const char* key, char* buf) const
	{
		sptr_t res = execute(SCI_GETPROPERTYEXPANDED, key, buf);
		return static_cast<int>(res);
	};

	std::string GetPropertyExpanded(const std::string& key) const
	{
		auto size = execute(SCI_GETPROPERTYEXPANDED, key.c_str(), NULL);
		std::string buf(size + 1, '\0');
		execute(SCI_GETPROPERTYEXPANDED, key.c_str(), &buf[0]);
		trim(buf);
		return buf;
	};

	int GetPropertyInt(const char* key) const
	{
		sptr_t res = execute(SCI_GETPROPERTYINT, key, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetPropertyInt(const std::string& key) const
	{
		sptr_t res = execute(SCI_GETPROPERTYINT, key.c_str(), SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetStyleBitsNeeded() const
	{
		sptr_t res = execute(SCI_GETSTYLEBITSNEEDED, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetLexerLanguage(char* text) const
	{
		sptr_t res = execute(SCI_GETLEXERLANGUAGE, SCI_UNUSED, text);
		return static_cast<int>(res);
	};

	std::string GetLexerLanguage() const
	{
		auto size = execute(SCI_GETLEXERLANGUAGE, SCI_UNUSED, NULL);
		std::string text(size + 1, '\0');
		execute(SCI_GETLEXERLANGUAGE, SCI_UNUSED, &text[0]);
		trim(text);
		return text;
	};

	int PrivateLexerexecute(int operation, sptr_t pointer) const
	{
		sptr_t res = execute(SCI_PRIVATELEXERCALL, operation, pointer);
		return static_cast<int>(res);
	};

	int PropertyNames(char* names) const
	{
		sptr_t res = execute(SCI_PROPERTYNAMES, SCI_UNUSED, names);
		return static_cast<int>(res);
	};

	std::string PropertyNames() const
	{
		auto size = execute(SCI_PROPERTYNAMES, SCI_UNUSED, NULL);
		std::string names(size + 1, '\0');
		execute(SCI_PROPERTYNAMES, SCI_UNUSED, &names[0]);
		trim(names);
		return names;
	};

	int PropertyType(const char* name) const
	{
		sptr_t res = execute(SCI_PROPERTYTYPE, name, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int PropertyType(const std::string& name) const
	{
		sptr_t res = execute(SCI_PROPERTYTYPE, name.c_str(), SCI_UNUSED);
		return static_cast<int>(res);
	};

	int DescribeProperty(const char* name, char* description) const
	{
		sptr_t res = execute(SCI_DESCRIBEPROPERTY, name, description);
		return static_cast<int>(res);
	};

	std::string DescribeProperty(const std::string& name) const
	{
		auto size = execute(SCI_DESCRIBEPROPERTY, name.c_str(), NULL);
		std::string description(size + 1, '\0');
		execute(SCI_DESCRIBEPROPERTY, name.c_str(), &description[0]);
		trim(description);
		return description;
	};

	int DescribeKeyWordSets(char* descriptions) const
	{
		sptr_t res = execute(SCI_DESCRIBEKEYWORDSETS, SCI_UNUSED, descriptions);
		return static_cast<int>(res);
	};

	std::string DescribeKeyWordSets() const
	{
		auto size = execute(SCI_DESCRIBEKEYWORDSETS, SCI_UNUSED, NULL);
		std::string descriptions(size + 1, '\0');
		execute(SCI_DESCRIBEKEYWORDSETS, SCI_UNUSED, &descriptions[0]);
		trim(descriptions);
		return descriptions;
	};

	int GetLineEndTypesSupported() const
	{
		sptr_t res = execute(SCI_GETLINEENDTYPESSUPPORTED, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int AllocateSubStyles(int styleBase, int numberStyles) const
	{
		sptr_t res = execute(SCI_ALLOCATESUBSTYLES, styleBase, numberStyles);
		return static_cast<int>(res);
	};

	int GetSubStylesStart(int styleBase) const
	{
		sptr_t res = execute(SCI_GETSUBSTYLESSTART, styleBase, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetSubStylesLength(int styleBase) const
	{
		sptr_t res = execute(SCI_GETSUBSTYLESLENGTH, styleBase, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetStyleFromSubStyle(int subStyle) const
	{
		sptr_t res = execute(SCI_GETSTYLEFROMSUBSTYLE, subStyle, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetPrimaryStyleFromStyle(int style) const
	{
		sptr_t res = execute(SCI_GETPRIMARYSTYLEFROMSTYLE, style, SCI_UNUSED);
		return static_cast<int>(res);
	};

	void FreeSubStyles() const
	{
		execute(SCI_FREESUBSTYLES, SCI_UNUSED, SCI_UNUSED);
	};

	void SetIdentifiers(int style, const char* identifiers) const
	{
		execute(SCI_SETIDENTIFIERS, style, identifiers);
	};

	void SetIdentifiers(int style, const std::string& identifiers) const
	{
		execute(SCI_SETIDENTIFIERS, style, identifiers.c_str());
	};

	int DistanceToSecondaryStyles() const
	{
		sptr_t res = execute(SCI_DISTANCETOSECONDARYSTYLES, SCI_UNUSED, SCI_UNUSED);
		return static_cast<int>(res);
	};

	int GetSubStyleBases(char* styles) const
	{
		sptr_t res = execute(SCI_GETSUBSTYLEBASES, SCI_UNUSED, styles);
		return static_cast<int>(res);
	};

	std::string GetSubStyleBases() const
	{
		auto size = execute(SCI_GETSUBSTYLEBASES, SCI_UNUSED, NULL);
		std::string styles(size + 1, '\0');
		execute(SCI_GETSUBSTYLEBASES, SCI_UNUSED, &styles[0]);
		trim(styles);
		return styles;
	};

protected:
	static HINSTANCE _hLib;
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

    NppParameters *_pParameter = nullptr;
	int _codepage = CP_ACP;
	bool _lineNumbersShown = false;
	bool _wrapRestoreNeeded = false;

	typedef std::unordered_map<int, Style> StyleMap;
	typedef std::unordered_map<BufferID, StyleMap*> BufferStyleMap;
	BufferStyleMap _hotspotStyles;

	int _beginSelectPosition = -1;

	static std::string _defaultCharList;

//Lexers and Styling
	void restyleBuffer();
	const char * getCompleteKeywordList(std::basic_string<char> & kwl, LangType langType, int keywordIndex);
	void setKeywords(LangType langType, const char *keywords, int index);
	void setLexer(int lexerID, LangType langType, int whichList);
	inline void makeStyle(LangType langType, const TCHAR **keywordArray = NULL);
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
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));
	};


	void setSqlLexer() {
		const bool kbBackSlash = NppParameters::getInstance()->getNppGUI()._backSlashIsEscapeCharacterForSql;
		setLexer(SCLEX_SQL, L_SQL, LIST_0);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("sql.backslash.escapes"), reinterpret_cast<LPARAM>(kbBackSlash ? "1" : "0"));
	};

	void setBashLexer() {
		setLexer(SCLEX_BASH, L_BASH, LIST_0);
	};

	void setVBLexer() {
		setLexer(SCLEX_VB, L_VB, LIST_0);
	};

	void setPascalLexer() {
		setLexer(SCLEX_PASCAL, L_PASCAL, LIST_0);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setPerlLexer() {
		setLexer(SCLEX_PERL, L_PERL, LIST_0);
	};

	void setPythonLexer() {
		setLexer(SCLEX_PYTHON, L_PYTHON, LIST_0 | LIST_1);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.quotes.python"), reinterpret_cast<LPARAM>("1"));
	};

	void setBatchLexer() {
		setLexer(SCLEX_BATCH, L_BATCH, LIST_0);
	};

	void setTeXLexer() {
		for (int i = 0 ; i < 4 ; ++i)
			execute(SCI_SETKEYWORDS, i, reinterpret_cast<LPARAM>(TEXT("")));
		setLexer(SCLEX_TEX, L_TEX, 0);
	};

	void setNsisLexer() {
		setLexer(SCLEX_NSIS, L_NSIS, LIST_0 | LIST_1 | LIST_2 | LIST_3);
	};

	void setFortranLexer() {
		setLexer(SCLEX_FORTRAN, L_FORTRAN, LIST_0 | LIST_1 | LIST_2);
	};

	void setFortran77Lexer() {
		setLexer(SCLEX_F77, L_FORTRAN_77, LIST_0 | LIST_1 | LIST_2);
	};

	void setLispLexer(){
		setLexer(SCLEX_LISP, L_LISP, LIST_0 | LIST_1);
	};

	void setSchemeLexer(){
		setLexer(SCLEX_LISP, L_SCHEME, LIST_0 | LIST_1);
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
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setCamlLexer(){
		setLexer(SCLEX_CAML, L_CAML, LIST_0 | LIST_1 | LIST_2);
	};

	void setAdaLexer(){
		setLexer(SCLEX_ADA, L_ADA, LIST_0);
	};

	void setVerilogLexer(){
		setLexer(SCLEX_VERILOG, L_VERILOG, LIST_0 | LIST_1);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<LPARAM>("1"));
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

    //--------------------

    void setCobolLexer() {
		setLexer(SCLEX_COBOL, L_COBOL, LIST_0 | LIST_1 | LIST_2);
	};
    void setGui4CliLexer() {
		setLexer(SCLEX_GUI4CLI, L_GUI4CLI, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4);
	};
    void setDLexer() {
		setLexer(SCLEX_D, L_D, LIST_0 | LIST_1 | LIST_2 | LIST_3 | LIST_4 | LIST_5 | LIST_6);
	};
    void setPowerShellLexer() {
		setLexer(SCLEX_POWERSHELL, L_POWERSHELL, LIST_0 | LIST_1 | LIST_2 | LIST_5);
	};
    void setRLexer() {
		setLexer(SCLEX_R, L_R, LIST_0 | LIST_1 | LIST_2);
	};

    void setCoffeeScriptLexer() {
		setLexer(SCLEX_COFFEESCRIPT, L_COFFEESCRIPT, LIST_0 | LIST_1 | LIST_2  | LIST_3);
	};

	void setBaanCLexer() {
		setLexer(SCLEX_BAAN, L_BAANC, LIST_0 | LIST_1);
		execute(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("styling.within.preprocessor"), reinterpret_cast<LPARAM>("1"));
	};

	void setSrecLexer() {
		setLexer(SCLEX_SREC, L_SREC, LIST_NONE);
	};

	void setIHexLexer() {
		setLexer(SCLEX_IHEX, L_IHEX, LIST_NONE);
	};

	void setTEHexLexer() {
		setLexer(SCLEX_TEHEX, L_TEHEX, LIST_NONE);
	};

    //--------------------

	void setSearchResultLexer() {
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_FILE_HEADER, true);
		execute(SCI_STYLESETEOLFILLED, SCE_SEARCHRESULT_SEARCH_HEADER, true);
		setLexer(SCLEX_SEARCHRESULT, L_SEARCHRESULT, 0);
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

	std::pair<int, int> getWordRange();
	bool expandWordSelection();
	void getFoldColor(COLORREF& fgColor, COLORREF& bgColor, COLORREF& activeFgColor);

	static inline void trim(std::string &s) {
		while (s.length() > 0 && s.back() == '\0')
			s.pop_back();
	}
};

