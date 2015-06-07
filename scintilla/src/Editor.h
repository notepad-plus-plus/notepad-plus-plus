// Scintilla source code edit control
/** @file Editor.h
 ** Defines the main editor class.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITOR_H
#define EDITOR_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

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
 * When platform has a way to generate an event before painting,
 * accumulate needed styling range and other work items in 
 * WorkNeeded to avoid unnecessary work inside paint handler
 */
class WorkNeeded {
public:
	enum workItems {
		workNone=0,
		workStyle=1,
		workUpdateUI=2
	};
	bool active;
	enum workItems items;
	Position upTo;

	WorkNeeded() : active(false), items(workNone), upTo(0) {}
	void Reset() {
		active = false;
		items = workNone;
		upTo = 0;
	}
	void Need(workItems items_, Position pos) {
		if ((items_ & workStyle) && (upTo < pos))
			upTo = pos;
		items = static_cast<workItems>(items | items_);
	}
};

/**
 * Hold a piece of text selected for copying or dragging, along with encoding and selection format information.
 */
class SelectionText {
	std::string s;
public:
	bool rectangular;
	bool lineCopy;
	int codePage;
	int characterSet;
	SelectionText() : rectangular(false), lineCopy(false), codePage(0), characterSet(0) {}
	~SelectionText() {
	}
	void Clear() {
		s.clear();
		rectangular = false;
		lineCopy = false;
		codePage = 0;
		characterSet = 0;
	}
	void Copy(const std::string &s_, int codePage_, int characterSet_, bool rectangular_, bool lineCopy_) {
		s = s_;
		codePage = codePage_;
		characterSet = characterSet_;
		rectangular = rectangular_;
		lineCopy = lineCopy_;
		FixSelectionForClipboard();
	}
	void Copy(const SelectionText &other) {
		Copy(other.s, other.codePage, other.characterSet, other.rectangular, other.lineCopy);
	}
	const char *Data() const {
		return s.c_str();
	}
	size_t Length() const {
		return s.length();
	}
	size_t LengthWithTerminator() const {
		return s.length() + 1;
	}
	bool Empty() const {
		return s.empty();
	}
private:
	void FixSelectionForClipboard() {
		// To avoid truncating the contents of the clipboard when pasted where the
		// clipboard contains NUL characters, replace NUL characters by spaces.
		std::replace(s.begin(), s.end(), '\0', ' ');
	}
};

struct WrapPending {
	// The range of lines that need to be wrapped
	enum { lineLarge = 0x7ffffff };
	int start;	// When there are wraps pending, will be in document range
	int end;	// May be lineLarge to indicate all of document after start
	WrapPending() {
		start = lineLarge;
		end = lineLarge;
	}
	void Reset() {
		start = lineLarge;
		end = lineLarge;
	}
	void Wrapped(int line) {
		if (start == line)
			start++;
	}
	bool NeedsWrap() const {
		return start < end;
	}
	bool AddRange(int lineStart, int lineEnd) {
		const bool neededWrap = NeedsWrap();
		bool changed = false;
		if (start > lineStart) {
			start = lineStart;
			changed = true;
		}
		if ((end < lineEnd) || !neededWrap) {
			end = lineEnd;
			changed = true;
		}
		return changed;
	}
};

/**
 */
class Editor : public EditModel, public DocWatcher {
	// Private so Editor objects can not be copied
	explicit Editor(const Editor &);
	Editor &operator=(const Editor &);

protected:	// ScintillaBase subclass needs access to much of Editor

	/** On GTK+, Scintilla is a container widget holding two scroll bars
	 * whereas on Windows there is just one window with both scroll bars turned on. */
	Window wMain;	///< The Scintilla parent window
	Window wMargin;	///< May be separate when using a scroll view for wMain

	/** Style resources may be expensive to allocate so are cached between uses.
	 * When a style attribute is changed, this cache is flushed. */
	bool stylesValid;
	ViewStyle vs;
	int technology;
	Point sizeRGBAImage;
	float scaleRGBAImage;

	MarginView marginView;
	EditView view;

	int cursorMode;

	bool hasFocus;
	bool mouseDownCaptures;

	int xCaretMargin;	///< Ensure this many pixels visible on both sides of caret
	bool horizontalScrollBarVisible;
	int scrollWidth;
	bool verticalScrollBarVisible;
	bool endAtLastLine;
	int caretSticky;
	int marginOptions;
	bool mouseSelectionRectangularSwitch;
	bool multipleSelection;
	bool additionalSelectionTyping;
	int multiPasteMode;

	int virtualSpaceOptions;

	KeyMap kmap;

	Timer timer;
	Timer autoScrollTimer;
	enum { autoScrollDelay = 200 };

	Idler idler;

	Point lastClick;
	unsigned int lastClickTime;
	Point doubleClickCloseThreshold;
	int dwellDelay;
	int ticksToDwell;
	bool dwelling;
	enum { selChar, selWord, selSubLine, selWholeLine } selectionType;
	Point ptMouseLast;
	enum { ddNone, ddInitial, ddDragging } inDragDrop;
	bool dropWentOutside;
	SelectionPosition posDrop;
	int hotSpotClickPos;
	int lastXChosen;
	int lineAnchorPos;
	int originalAnchorPos;
	int wordSelectAnchorStartPos;
	int wordSelectAnchorEndPos;
	int wordSelectInitialCaretPos;
	int targetStart;
	int targetEnd;
	int searchFlags;
	int topLine;
	int posTopLine;
	int lengthForEncode;

	int needUpdateUI;

	enum { notPainting, painting, paintAbandoned } paintState;
	bool paintAbandonedByStyling;
	PRectangle rcPaint;
	bool paintingAllText;
	bool willRedrawAll;
	WorkNeeded workNeeded;

	int modEventMask;

	SelectionText drag;

	int caretXPolicy;
	int caretXSlop;	///< Ensure this many pixels visible on both sides of caret

	int caretYPolicy;
	int caretYSlop;	///< Ensure this many lines visible on both sides of caret

	int visiblePolicy;
	int visibleSlop;

	int searchAnchor;

	bool recordingMacro;

	int foldAutomatic;

	// Wrapping support
	WrapPending wrapPending;

	bool convertPastes;

	Editor();
	virtual ~Editor();
	virtual void Initialise() = 0;
	virtual void Finalise();

	void InvalidateStyleData();
	void InvalidateStyleRedraw();
	void RefreshStyleData();
	void SetRepresentations();
	void DropGraphics(bool freeObjects);
	void AllocateGraphics();

	// The top left visible point in main window coordinates. Will be 0,0 except for
	// scroll views where it will be equivalent to the current scroll position.
	virtual Point GetVisibleOriginInMain() const;
	Point DocumentPointFromView(Point ptView) const;  // Convert a point from view space to document
	int TopLineOfMain() const;   // Return the line at Main's y coordinate 0
	virtual PRectangle GetClientRectangle() const;
	virtual PRectangle GetClientDrawingRectangle();
	PRectangle GetTextRectangle() const;

	virtual int LinesOnScreen() const;
	int LinesToScroll() const;
	int MaxScrollPos() const;
	SelectionPosition ClampPositionIntoDocument(SelectionPosition sp) const;
	Point LocationFromPosition(SelectionPosition pos);
	Point LocationFromPosition(int pos);
	int XFromPosition(int pos);
	int XFromPosition(SelectionPosition sp);
	SelectionPosition SPositionFromLocation(Point pt, bool canReturnInvalid=false, bool charPosition=false, bool virtualSpace=true);
	int PositionFromLocation(Point pt, bool canReturnInvalid = false, bool charPosition = false);
	SelectionPosition SPositionFromLineX(int lineDoc, int x);
	int PositionFromLineX(int line, int x);
	int LineFromLocation(Point pt) const;
	void SetTopLine(int topLineNew);

	virtual bool AbandonPaint();
	virtual void RedrawRect(PRectangle rc);
	virtual void DiscardOverdraw();
	virtual void Redraw();
	void RedrawSelMargin(int line=-1, bool allAfter=false);
	PRectangle RectangleFromRange(Range r, int overlap);
	void InvalidateRange(int start, int end);

	bool UserVirtualSpace() const {
		return ((virtualSpaceOptions & SCVS_USERACCESSIBLE) != 0);
	}
	int CurrentPosition() const;
	bool SelectionEmpty() const;
	SelectionPosition SelectionStart();
	SelectionPosition SelectionEnd();
	void SetRectangularRange();
	void ThinRectangularRange();
	void InvalidateSelection(SelectionRange newMain, bool invalidateWholeSelection=false);
	void SetSelection(SelectionPosition currentPos_, SelectionPosition anchor_);
	void SetSelection(int currentPos_, int anchor_);
	void SetSelection(SelectionPosition currentPos_);
	void SetSelection(int currentPos_);
	void SetEmptySelection(SelectionPosition currentPos_);
	void SetEmptySelection(int currentPos_);
	bool RangeContainsProtected(int start, int end) const;
	bool SelectionContainsProtected();
	int MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd=true) const;
	SelectionPosition MovePositionOutsideChar(SelectionPosition pos, int moveDir, bool checkLineEnd=true) const;
	int MovePositionTo(SelectionPosition newPos, Selection::selTypes selt=Selection::noSel, bool ensureVisible=true);
	int MovePositionTo(int newPos, Selection::selTypes selt=Selection::noSel, bool ensureVisible=true);
	SelectionPosition MovePositionSoVisible(SelectionPosition pos, int moveDir);
	SelectionPosition MovePositionSoVisible(int pos, int moveDir);
	Point PointMainCaret();
	void SetLastXChosen();

	void ScrollTo(int line, bool moveThumb=true);
	virtual void ScrollText(int linesToMove);
	void HorizontalScrollTo(int xPos);
	void VerticalCentreCaret();
	void MoveSelectedLines(int lineDelta);
	void MoveSelectedLinesUp();
	void MoveSelectedLinesDown();
	void MoveCaretInsideView(bool ensureVisible=true);
	int DisplayFromPosition(int pos);

	struct XYScrollPosition {
		int xOffset;
		int topLine;
		XYScrollPosition(int xOffset_, int topLine_) : xOffset(xOffset_), topLine(topLine_) {}
		bool operator==(const XYScrollPosition &other) const {
			return (xOffset == other.xOffset) && (topLine == other.topLine);
		}
	};
	enum XYScrollOptions {
		xysUseMargin=0x1,
		xysVertical=0x2,
		xysHorizontal=0x4,
		xysDefault=xysUseMargin|xysVertical|xysHorizontal};
	XYScrollPosition XYScrollToMakeVisible(const SelectionRange &range, const XYScrollOptions options);
	void SetXYScroll(XYScrollPosition newXY);
	void EnsureCaretVisible(bool useMargin=true, bool vert=true, bool horiz=true);
	void ScrollRange(SelectionRange range);
	void ShowCaretAtCurrentPosition();
	void DropCaret();
	void CaretSetPeriod(int period);
	void InvalidateCaret();
	virtual void UpdateSystemCaret();

	bool Wrapping() const;
	void NeedWrapping(int docLineStart=0, int docLineEnd=WrapPending::lineLarge);
	bool WrapOneLine(Surface *surface, int lineToWrap);
	enum wrapScope {wsAll, wsVisible, wsIdle};
	bool WrapLines(enum wrapScope ws);
	void LinesJoin();
	void LinesSplit(int pixelWidth);

	void PaintSelMargin(Surface *surface, PRectangle &rc);
	void RefreshPixMaps(Surface *surfaceWindow);
	void Paint(Surface *surfaceWindow, PRectangle rcArea);
	long FormatRange(bool draw, Sci_RangeToFormat *pfr);
	int TextWidth(int style, const char *text);

	virtual void SetVerticalScrollPos() = 0;
	virtual void SetHorizontalScrollPos() = 0;
	virtual bool ModifyScrollBars(int nMax, int nPage) = 0;
	virtual void ReconfigureScrollBars();
	void SetScrollBars();
	void ChangeSize();

	void FilterSelections();
	int InsertSpace(int position, unsigned int spaces);
	void AddChar(char ch);
	virtual void AddCharUTF(const char *s, unsigned int len, bool treatAsDBCS=false);
	void FillVirtualSpace();
	void InsertPaste(const char *text, int len);
	enum PasteShape { pasteStream=0, pasteRectangular = 1, pasteLine = 2 };
	void InsertPasteShape(const char *text, int len, PasteShape shape);
	void ClearSelection(bool retainMultipleSelections = false);
	void ClearAll();
	void ClearDocumentStyle();
	void Cut();
	void PasteRectangular(SelectionPosition pos, const char *ptr, int len);
	virtual void Copy() = 0;
	virtual void CopyAllowLine();
	virtual bool CanPaste();
	virtual void Paste() = 0;
	void Clear();
	void SelectAll();
	void Undo();
	void Redo();
	void DelCharBack(bool allowLineStartDeletion);
	virtual void ClaimSelection() = 0;

	static int ModifierFlags(bool shift, bool ctrl, bool alt, bool meta=false);
	virtual void NotifyChange() = 0;
	virtual void NotifyFocus(bool focus);
	virtual void SetCtrlID(int identifier);
	virtual int GetCtrlID() { return ctrlID; }
	virtual void NotifyParent(SCNotification scn) = 0;
	virtual void NotifyStyleToNeeded(int endStyleNeeded);
	void NotifyChar(int ch);
	void NotifySavePoint(bool isSavePoint);
	void NotifyModifyAttempt();
	virtual void NotifyDoubleClick(Point pt, int modifiers);
	virtual void NotifyDoubleClick(Point pt, bool shift, bool ctrl, bool alt);
	void NotifyHotSpotClicked(int position, int modifiers);
	void NotifyHotSpotClicked(int position, bool shift, bool ctrl, bool alt);
	void NotifyHotSpotDoubleClicked(int position, int modifiers);
	void NotifyHotSpotDoubleClicked(int position, bool shift, bool ctrl, bool alt);
	void NotifyHotSpotReleaseClick(int position, int modifiers);
	void NotifyHotSpotReleaseClick(int position, bool shift, bool ctrl, bool alt);
	bool NotifyUpdateUI();
	void NotifyPainted();
	void NotifyScrolled();
	void NotifyIndicatorClick(bool click, int position, int modifiers);
	void NotifyIndicatorClick(bool click, int position, bool shift, bool ctrl, bool alt);
	bool NotifyMarginClick(Point pt, int modifiers);
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
	void NotifyLexerChanged(Document *doc, void *userData);
	void NotifyErrorOccurred(Document *doc, void *userData, int status);
	void NotifyMacroRecord(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

	void ContainerNeedsUpdate(int flags);
	void PageMove(int direction, Selection::selTypes selt=Selection::noSel, bool stuttered = false);
	enum { cmSame, cmUpper, cmLower };
	virtual std::string CaseMapString(const std::string &s, int caseMapping);
	void ChangeCaseOfSelection(int caseMapping);
	void LineTranspose();
	void Duplicate(bool forLine);
	virtual void CancelModes();
	void NewLine();
	void CursorUpOrDown(int direction, Selection::selTypes selt=Selection::noSel);
	void ParaUpOrDown(int direction, Selection::selTypes selt=Selection::noSel);
	int StartEndDisplayLine(int pos, bool start);
	virtual int KeyCommand(unsigned int iMessage);
	virtual int KeyDefault(int /* key */, int /*modifiers*/);
	int KeyDownWithModifiers(int key, int modifiers, bool *consumed);
	int KeyDown(int key, bool shift, bool ctrl, bool alt, bool *consumed=0);

	void Indent(bool forwards);

	virtual CaseFolder *CaseFolderForEncoding();
	long FindText(uptr_t wParam, sptr_t lParam);
	void SearchAnchor();
	long SearchText(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	long SearchInTarget(const char *text, int length);
	void GoToLine(int lineNo);

	virtual void CopyToClipboard(const SelectionText &selectedText) = 0;
	std::string RangeText(int start, int end) const;
	void CopySelectionRange(SelectionText *ss, bool allowLineCopy=false);
	void CopyRangeToClipboard(int start, int end);
	void CopyText(int length, const char *text);
	void SetDragPosition(SelectionPosition newPos);
	virtual void DisplayCursor(Window::Cursor c);
	virtual bool DragThreshold(Point ptStart, Point ptNow);
	virtual void StartDrag();
	void DropAt(SelectionPosition position, const char *value, size_t lengthValue, bool moving, bool rectangular);
	void DropAt(SelectionPosition position, const char *value, bool moving, bool rectangular);
	/** PositionInSelection returns true if position in selection. */
	bool PositionInSelection(int pos);
	bool PointInSelection(Point pt);
	bool PointInSelMargin(Point pt) const;
	Window::Cursor GetMarginCursor(Point pt) const;
	void TrimAndSetSelection(int currentPos_, int anchor_);
	void LineSelection(int lineCurrentPos_, int lineAnchorPos_, bool wholeLine);
	void WordSelection(int pos);
	void DwellEnd(bool mouseMoved);
	void MouseLeave();
	virtual void ButtonDownWithModifiers(Point pt, unsigned int curTime, int modifiers);
	virtual void ButtonDown(Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt);
	void ButtonMoveWithModifiers(Point pt, int modifiers);
	void ButtonMove(Point pt);
	void ButtonUp(Point pt, unsigned int curTime, bool ctrl);

	void Tick();
	bool Idle();
	virtual void SetTicking(bool on);
	enum TickReason { tickCaret, tickScroll, tickWiden, tickDwell, tickPlatform };
	virtual void TickFor(TickReason reason);
	virtual bool FineTickerAvailable();
	virtual bool FineTickerRunning(TickReason reason);
	virtual void FineTickerStart(TickReason reason, int millis, int tolerance);
	virtual void FineTickerCancel(TickReason reason);
	virtual bool SetIdle(bool) { return false; }
	virtual void SetMouseCapture(bool on) = 0;
	virtual bool HaveMouseCapture() = 0;
	void SetFocusState(bool focusState);

	int PositionAfterArea(PRectangle rcArea) const;
	void StyleToPositionInView(Position pos);
	virtual void IdleWork();
	virtual void QueueIdleWork(WorkNeeded::workItems items, int upTo=0);

	virtual bool PaintContains(PRectangle rc);
	bool PaintContainsMargin();
	void CheckForChangeOutsidePaint(Range r);
	void SetBraceHighlight(Position pos0, Position pos1, int matchStyle);

	void SetAnnotationHeights(int start, int end);
	virtual void SetDocPointer(Document *document);

	void SetAnnotationVisible(int visible);

	int ExpandLine(int line);
	void SetFoldExpanded(int lineDoc, bool expanded);
	void FoldLine(int line, int action);
	void FoldExpand(int line, int action, int level);
	int ContractedFoldNext(int lineStart) const;
	void EnsureLineVisible(int lineDoc, bool enforcePolicy);
	void FoldChanged(int line, int levelNow, int levelPrev);
	void NeedShown(int pos, int len);
	void FoldAll(int action);

	int GetTag(char *tagValue, int tagNumber);
	int ReplaceTarget(bool replacePatterns, const char *text, int length=-1);

	bool PositionIsHotspot(int position) const;
	bool PointIsHotspot(Point pt);
	void SetHotSpotRange(Point *pt);
	Range GetHotSpotRange() const;
	void SetHoverIndicatorPosition(int position);
	void SetHoverIndicatorPoint(Point pt);

	int CodePage() const;
	virtual bool ValidCodePage(int /* codePage */) const { return true; }
	int WrapCount(int line);
	void AddStyledText(char *buffer, int appendLength);

	virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) = 0;
	void StyleSetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t StyleGetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

	static const char *StringFromEOLMode(int eolMode);

	static sptr_t StringResult(sptr_t lParam, const char *val);
	static sptr_t BytesResult(sptr_t lParam, const unsigned char *val, size_t len);

public:
	// Public so the COM thunks can access it.
	bool IsUnicodeMode() const;
	// Public so scintilla_send_message can use it.
	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	// Public so scintilla_set_id can use it.
	int ctrlID;
	// Public so COM methods for drag and drop can set it.
	int errorStatus;
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
	AutoSurface(Editor *ed, int technology = -1) : surf(0) {
		if (ed->wMain.GetID()) {
			surf = Surface::Allocate(technology != -1 ? technology : ed->technology);
			if (surf) {
				surf->Init(ed->wMain.GetID());
				surf->SetUnicodeMode(SC_CP_UTF8 == ed->CodePage());
				surf->SetDBCSMode(ed->CodePage());
			}
		}
	}
	AutoSurface(SurfaceID sid, Editor *ed, int technology = -1) : surf(0) {
		if (ed->wMain.GetID()) {
			surf = Surface::Allocate(technology != -1 ? technology : ed->technology);
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
