// Scintilla source code edit control
/** @file Editor.h
 ** Defines the main editor class.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITOR_H
#define EDITOR_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 */
class Caret {
public:
	bool active;
	bool on;
	int period;

	Caret();
};

/**
 */
class Timer {
public:
	bool ticking;
	int ticksToWait;
	enum {tickSize = 100};
	TickerID tickerID;

	Timer();
};

/**
 */
class Idler {
public:
	bool state;
	IdlerID idlerID;

	Idler();
};

/**
 * Hold a piece of text selected for copying or dragging.
 * The text is expected to hold a terminating '\0' and this is counted in len.
 */
class SelectionText {
public:
	char *s;
	int len;
	bool rectangular;
	bool lineCopy;
	int codePage;
	int characterSet;
	SelectionText() : s(0), len(0), rectangular(false), lineCopy(false), codePage(0), characterSet(0) {}
	~SelectionText() {
		Free();
	}
	void Free() {
		Set(0, 0, 0, 0, false, false);
	}
	void Set(char *s_, int len_, int codePage_, int characterSet_, bool rectangular_, bool lineCopy_) {
		delete []s;
		s = s_;
		if (s)
			len = len_;
		else
			len = 0;
		codePage = codePage_;
		characterSet = characterSet_;
		rectangular = rectangular_;
		lineCopy = lineCopy_;
	}
	void Copy(const char *s_, int len_, int codePage_, int characterSet_, bool rectangular_, bool lineCopy_) {
		delete []s;
		s = new char[len_];
		if (s) {
			len = len_;
			for (int i = 0; i < len_; i++) {
				s[i] = s_[i];
			}
		} else {
			len = 0;
		}
		codePage = codePage_;
		characterSet = characterSet_;
		rectangular = rectangular_;
		lineCopy = lineCopy_;
	}
	void Copy(const SelectionText &other) {
		Copy(other.s, other.len, other.codePage, other.characterSet, other.rectangular, other.lineCopy);
	}
};

/**
 */
class Editor : public DocWatcher {
	// Private so Editor objects can not be copied
	Editor(const Editor &) : DocWatcher() {}
	Editor &operator=(const Editor &) { return *this; }

protected:	// ScintillaBase subclass needs access to much of Editor

	/** On GTK+, Scintilla is a container widget holding two scroll bars
	 * whereas on Windows there is just one window with both scroll bars turned on. */
	Window wMain;	///< The Scintilla parent window

	/** Style resources may be expensive to allocate so are cached between uses.
	 * When a style attribute is changed, this cache is flushed. */
	bool stylesValid;
	ViewStyle vs;
	Palette palette;

	int printMagnification;
	int printColourMode;
	int printWrapState;
	int cursorMode;
	int controlCharSymbol;

	bool hasFocus;
	bool hideSelection;
	bool inOverstrike;
	int errorStatus;
	bool mouseDownCaptures;

	/** In bufferedDraw mode, graphics operations are drawn to a pixmap and then copied to
	 * the screen. This avoids flashing but is about 30% slower. */
	bool bufferedDraw;
	/** In twoPhaseDraw mode, drawing is performed in two phases, first the background
	* and then the foreground. This avoids chopping off characters that overlap the next run. */
	bool twoPhaseDraw;

	int xOffset;		///< Horizontal scrolled amount in pixels
	int xCaretMargin;	///< Ensure this many pixels visible on both sides of caret
	bool horizontalScrollBarVisible;
	int scrollWidth;
	bool trackLineWidth;
	int lineWidthMaxSeen;
	bool verticalScrollBarVisible;
	bool endAtLastLine;
	bool caretSticky;

	Surface *pixmapLine;
	Surface *pixmapSelMargin;
	Surface *pixmapSelPattern;
	Surface *pixmapIndentGuide;
	Surface *pixmapIndentGuideHighlight;

	LineLayoutCache llc;
	PositionCache posCache;

	KeyMap kmap;

	Caret caret;
	Timer timer;
	Timer autoScrollTimer;
	enum { autoScrollDelay = 200 };

	Idler idler;

	Point lastClick;
	unsigned int lastClickTime;
	int dwellDelay;
	int ticksToDwell;
	bool dwelling;
	enum { selChar, selWord, selLine } selectionType;
	Point ptMouseLast;
	enum { ddNone, ddInitial, ddDragging } inDragDrop;
	bool dropWentOutside;
	int posDrag;
	int posDrop;
	int lastXChosen;
	int lineAnchor;
	int originalAnchorPos;
	int currentPos;
	int anchor;
	int targetStart;
	int targetEnd;
	int searchFlags;
	int topLine;
	int posTopLine;
	int lengthForEncode;

	bool needUpdateUI;
	Position braces[2];
	int bracesMatchStyle;
	int highlightGuideColumn;

	int theEdge;

	enum { notPainting, painting, paintAbandoned } paintState;
	PRectangle rcPaint;
	bool paintingAllText;

	int modEventMask;

	SelectionText drag;
	enum selTypes { noSel, selStream, selRectangle, selLines };
	selTypes selType;
	bool moveExtendsSelection;
	int xStartSelect;	///< x position of start of rectangular selection
	int xEndSelect;		///< x position of end of rectangular selection
	bool primarySelection;

	int caretXPolicy;
	int caretXSlop;	///< Ensure this many pixels visible on both sides of caret

	int caretYPolicy;
	int caretYSlop;	///< Ensure this many lines visible on both sides of caret

	int visiblePolicy;
	int visibleSlop;

	int searchAnchor;

	bool recordingMacro;

	int foldFlags;
	ContractionState cs;

	// Hotspot support
	int hsStart;
	int hsEnd;

	// Wrapping support
	enum { eWrapNone, eWrapWord, eWrapChar } wrapState;
	enum { wrapLineLarge = 0x7ffffff };
	int wrapWidth;
	int wrapStart;
	int wrapEnd;
	int wrapVisualFlags;
	int wrapVisualFlagsLocation;
	int wrapVisualStartIndent;
	int actualWrapVisualStartIndent;

	bool convertPastes;

	Document *pdoc;

	Editor();
	virtual ~Editor();
	virtual void Initialise() = 0;
	virtual void Finalise();

	void InvalidateStyleData();
	void InvalidateStyleRedraw();
	virtual void RefreshColourPalette(Palette &pal, bool want);
	void RefreshStyleData();
	void DropGraphics();

	virtual PRectangle GetClientRectangle();
	PRectangle GetTextRectangle();

	int LinesOnScreen();
	int LinesToScroll();
	int MaxScrollPos();
	Point LocationFromPosition(int pos);
	int XFromPosition(int pos);
	int PositionFromLocation(Point pt);
	int PositionFromLocationClose(Point pt);
	int PositionFromLineX(int line, int x);
	int LineFromLocation(Point pt);
	void SetTopLine(int topLineNew);

	bool AbandonPaint();
	void RedrawRect(PRectangle rc);
	void Redraw();
	void RedrawSelMargin(int line=-1);
	PRectangle RectangleFromRange(int start, int end);
	void InvalidateRange(int start, int end);

	int CurrentPosition();
	bool SelectionEmpty();
	int SelectionStart();
	int SelectionEnd();
	void SetRectangularRange();
	void InvalidateSelection(int currentPos_, int anchor_, bool invalidateWholeSelection);
	void SetSelection(int currentPos_, int anchor_);
	void SetSelection(int currentPos_);
	void SetEmptySelection(int currentPos_);
	bool RangeContainsProtected(int start, int end) const;
	bool SelectionContainsProtected();
	int MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd=true);
	int MovePositionTo(int newPos, selTypes sel=noSel, bool ensureVisible=true);
	int MovePositionSoVisible(int pos, int moveDir);
	void SetLastXChosen();

	void ScrollTo(int line, bool moveThumb=true);
	virtual void ScrollText(int linesToMove);
	void HorizontalScrollTo(int xPos);
	void MoveCaretInsideView(bool ensureVisible=true);
	int DisplayFromPosition(int pos);
	void EnsureCaretVisible(bool useMargin=true, bool vert=true, bool horiz=true);
	void ShowCaretAtCurrentPosition();
	void DropCaret();
	void InvalidateCaret();
	virtual void UpdateSystemCaret();

	void NeedWrapping(int docLineStart = 0, int docLineEnd = wrapLineLarge);
	bool WrapOneLine(Surface *surface, int lineToWrap);
	bool WrapLines(bool fullWrap, int priorityWrapLineStart);
	void LinesJoin();
	void LinesSplit(int pixelWidth);

	int SubstituteMarkerIfEmpty(int markerCheck, int markerDefault);
	void PaintSelMargin(Surface *surface, PRectangle &rc);
	LineLayout *RetrieveLineLayout(int lineNumber);
	void LayoutLine(int line, Surface *surface, ViewStyle &vstyle, LineLayout *ll,
		int width=LineLayout::wrapWidthInfinite);
	ColourAllocated SelectionBackground(ViewStyle &vsDraw);
	ColourAllocated TextBackground(ViewStyle &vsDraw, bool overrideBackground, ColourAllocated background, bool inSelection, bool inHotspot, int styleMain, int i, LineLayout *ll);
	void DrawIndentGuide(Surface *surface, int lineVisible, int lineHeight, int start, PRectangle rcSegment, bool highlight);
	void DrawWrapMarker(Surface *surface, PRectangle rcPlace, bool isEndMarker, ColourAllocated wrapColour);
	void DrawEOL(Surface *surface, ViewStyle &vsDraw, PRectangle rcLine, LineLayout *ll,
		int line, int lineEnd, int xStart, int subLine, int subLineStart,
		bool overrideBackground, ColourAllocated background,
		bool drawWrapMark, ColourAllocated wrapColour);
	void DrawIndicators(Surface *surface, ViewStyle &vsDraw, int line, int xStart,
		PRectangle rcLine, LineLayout *ll, int subLine, int lineEnd, bool under);
	void DrawLine(Surface *surface, ViewStyle &vsDraw, int line, int lineVisible, int xStart,
		PRectangle rcLine, LineLayout *ll, int subLine=0);
	void DrawBlockCaret(Surface *surface, ViewStyle &vsDraw, LineLayout *ll, int subLine, int xStart, int offset, int posCaret, PRectangle rcCaret);
	void RefreshPixMaps(Surface *surfaceWindow);
	void Paint(Surface *surfaceWindow, PRectangle rcArea);
	long FormatRange(bool draw, RangeToFormat *pfr);
	int TextWidth(int style, const char *text);

	virtual void SetVerticalScrollPos() = 0;
	virtual void SetHorizontalScrollPos() = 0;
	virtual bool ModifyScrollBars(int nMax, int nPage) = 0;
	virtual void ReconfigureScrollBars();
	void SetScrollBars();
	void ChangeSize();

	void AddChar(char ch);
	virtual void AddCharUTF(char *s, unsigned int len, bool treatAsDBCS=false);
	void ClearSelection();
	void ClearAll();
	void ClearDocumentStyle();
	void Cut();
	void PasteRectangular(int pos, const char *ptr, int len);
	virtual void Copy() = 0;
	virtual void CopyAllowLine();
	virtual bool CanPaste();
	virtual void Paste() = 0;
	void Clear();
	void SelectAll();
	void Undo();
	void Redo();
	void DelChar();
	void DelCharBack(bool allowLineStartDeletion);
	virtual void ClaimSelection() = 0;

	virtual void NotifyChange() = 0;
	virtual void NotifyFocus(bool focus);
	virtual int GetCtrlID() { return ctrlID; }
	virtual void NotifyParent(SCNotification scn) = 0;
	virtual void NotifyStyleToNeeded(int endStyleNeeded);
	void NotifyChar(int ch);
	void NotifyMove(int position);
	void NotifySavePoint(bool isSavePoint);
	void NotifyModifyAttempt();
	virtual void NotifyDoubleClick(Point pt, bool shift, bool ctrl, bool alt);
	void NotifyHotSpotClicked(int position, bool shift, bool ctrl, bool alt);
	void NotifyHotSpotDoubleClicked(int position, bool shift, bool ctrl, bool alt);
	void NotifyUpdateUI();
	void NotifyPainted();
	void NotifyIndicatorClick(bool click, int position, bool shift, bool ctrl, bool alt);
	bool NotifyMarginClick(Point pt, bool shift, bool ctrl, bool alt);
	void NotifyNeedShown(int pos, int len);
	void NotifyDwelling(Point pt, bool state);
	void NotifyZoom();

	void NotifyModifyAttempt(Document *document, void *userData);
	void NotifySavePoint(Document *document, void *userData, bool atSavePoint);
	void CheckModificationForWrap(DocModification mh);
	void NotifyModified(Document *document, DocModification mh, void *userData);
	void NotifyDeleted(Document *document, void *userData);
	void NotifyStyleNeeded(Document *doc, void *userData, int endPos);
	void NotifyMacroRecord(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

	void PageMove(int direction, selTypes sel=noSel, bool stuttered = false);
	void ChangeCaseOfSelection(bool makeUpperCase);
	void LineTranspose();
	void Duplicate(bool forLine);
	virtual void CancelModes();
	void NewLine();
	void CursorUpOrDown(int direction, selTypes sel=noSel);
	void ParaUpOrDown(int direction, selTypes sel=noSel);
	int StartEndDisplayLine(int pos, bool start);
	virtual int KeyCommand(unsigned int iMessage);
	virtual int KeyDefault(int /* key */, int /*modifiers*/);
	int KeyDown(int key, bool shift, bool ctrl, bool alt, bool *consumed=0);

	int GetWhitespaceVisible();
	void SetWhitespaceVisible(int view);

	void Indent(bool forwards);

	long FindText(uptr_t wParam, sptr_t lParam);
	void SearchAnchor();
	long SearchText(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	long SearchInTarget(const char *text, int length);
	void GoToLine(int lineNo);

	virtual void CopyToClipboard(const SelectionText &selectedText) = 0;
	char *CopyRange(int start, int end);
	void CopySelectionFromRange(SelectionText *ss, bool allowLineCopy, int start, int end);
	void CopySelectionRange(SelectionText *ss, bool allowLineCopy=false);
	void CopyRangeToClipboard(int start, int end);
	void CopyText(int length, const char *text);
	void SetDragPosition(int newPos);
	virtual void DisplayCursor(Window::Cursor c);
	virtual bool DragThreshold(Point ptStart, Point ptNow);
	virtual void StartDrag();
	void DropAt(int position, const char *value, bool moving, bool rectangular);
	/** PositionInSelection returns 0 if position in selection, -1 if position before selection, and 1 if after.
	 * Before means either before any line of selection or before selection on its line, with a similar meaning to after. */
	int PositionInSelection(int pos);
	bool PointInSelection(Point pt);
	bool PointInSelMargin(Point pt);
	void LineSelection(int lineCurrent_, int lineAnchor_);
	void DwellEnd(bool mouseMoved);
	virtual void ButtonDown(Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt);
	void ButtonMove(Point pt);
	void ButtonUp(Point pt, unsigned int curTime, bool ctrl);

	void Tick();
	bool Idle();
	virtual void SetTicking(bool on) = 0;
	virtual bool SetIdle(bool) { return false; }
	virtual void SetMouseCapture(bool on) = 0;
	virtual bool HaveMouseCapture() = 0;
	void SetFocusState(bool focusState);

	virtual bool PaintContains(PRectangle rc);
	bool PaintContainsMargin();
	void CheckForChangeOutsidePaint(Range r);
	void SetBraceHighlight(Position pos0, Position pos1, int matchStyle);

	void SetDocPointer(Document *document);

	void Expand(int &line, bool doExpand);
	void ToggleContraction(int line);
	void EnsureLineVisible(int lineDoc, bool enforcePolicy);
	int ReplaceTarget(bool replacePatterns, const char *text, int length=-1);

	bool PositionIsHotspot(int position);
	bool PointIsHotspot(Point pt);
	void SetHotSpotRange(Point *pt);
	void GetHotSpotRange(int& hsStart, int& hsEnd);

	int CodePage() const;
	virtual bool ValidCodePage(int /* codePage */) const { return true; }
	int WrapCount(int line);
	void AddStyledText(char *buffer, int appendLength);

	virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) = 0;
	void StyleSetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t StyleGetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

	static const char *StringFromEOLMode(int eolMode);

public:
	// Public so the COM thunks can access it.
	bool IsUnicodeMode() const;
	// Public so scintilla_send_message can use it.
	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	// Public so scintilla_set_id can use it.
	int ctrlID;
	friend class AutoSurface;
	friend class SelectionLineIterator;
};

/**
 * A smart pointer class to ensure Surfaces are set up and deleted correctly.
 */
class AutoSurface {
private:
	Surface *surf;
public:
	AutoSurface(Editor *ed) : surf(0) {
		if (ed->wMain.GetID()) {
			surf = Surface::Allocate();
			if (surf) {
				surf->Init(ed->wMain.GetID());
				surf->SetUnicodeMode(SC_CP_UTF8 == ed->CodePage());
				surf->SetDBCSMode(ed->CodePage());
			}
		}
	}
	AutoSurface(SurfaceID sid, Editor *ed) : surf(0) {
		if (ed->wMain.GetID()) {
			surf = Surface::Allocate();
			if (surf) {
				surf->Init(sid, ed->wMain.GetID());
				surf->SetUnicodeMode(SC_CP_UTF8 == ed->CodePage());
				surf->SetDBCSMode(ed->CodePage());
			}
		}
	}
	~AutoSurface() {
		delete surf;
	}
	Surface *operator->() const {
		return surf;
	}
	operator Surface *() const {
		return surf;
	}
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
