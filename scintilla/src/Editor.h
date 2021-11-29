// Scintilla source code edit control
/** @file Editor.h
 ** Defines the main editor class.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITOR_H
#define EDITOR_H

namespace Scintilla {

/**
 */
class Timer {
public:
	bool ticking;
	int ticksToWait;
	enum {tickSize = 100};
	TickerID tickerID;

	Timer() noexcept;
};

/**
 */
class Idler {
public:
	bool state;
	IdlerID idlerID;

	Idler() noexcept;
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
	enum workItems items;
	Sci::Position upTo;

	WorkNeeded() noexcept : items(workNone), upTo(0) {}
	void Reset() noexcept {
		items = workNone;
		upTo = 0;
	}
	void Need(workItems items_, Sci::Position pos) noexcept {
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
	SelectionText() noexcept : rectangular(false), lineCopy(false), codePage(0), characterSet(0) {}
	void Clear() noexcept {
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
	const char *Data() const noexcept {
		return s.c_str();
	}
	size_t Length() const noexcept {
		return s.length();
	}
	size_t LengthWithTerminator() const noexcept {
		return s.length() + 1;
	}
	bool Empty() const noexcept {
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
	Sci::Line start;	// When there are wraps pending, will be in document range
	Sci::Line end;	// May be lineLarge to indicate all of document after start
	WrapPending() noexcept {
		start = lineLarge;
		end = lineLarge;
	}
	void Reset() noexcept {
		start = lineLarge;
		end = lineLarge;
	}
	void Wrapped(Sci::Line line) noexcept {
		if (start == line)
			start++;
	}
	bool NeedsWrap() const noexcept {
		return start < end;
	}
	bool AddRange(Sci::Line lineStart, Sci::Line lineEnd) noexcept {
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

struct CaretPolicy {
	int policy;	// Combination from CARET_SLOP, CARET_STRICT, CARET_JUMPS, CARET_EVEN
	int slop;	// Pixels for X, lines for Y
	CaretPolicy(uptr_t policy_=0, sptr_t slop_=0) noexcept :
		policy(static_cast<int>(policy_)), slop(static_cast<int>(slop_)) {}
};

struct CaretPolicies {
	CaretPolicy x;
	CaretPolicy y;
};

/**
 */
class Editor : public EditModel, public DocWatcher {
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
	bool mouseWheelCaptures;

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
	enum class TextUnit { character, word, subLine, wholeLine } selectionUnit;
	Point ptMouseLast;
	enum { ddNone, ddInitial, ddDragging } inDragDrop;
	bool dropWentOutside;
	SelectionPosition posDrop;
	Sci::Position hotSpotClickPos;
	int lastXChosen;
	Sci::Position lineAnchorPos;
	Sci::Position originalAnchorPos;
	Sci::Position wordSelectAnchorStartPos;
	Sci::Position wordSelectAnchorEndPos;
	Sci::Position wordSelectInitialCaretPos;
	SelectionSegment targetRange;
	int searchFlags;
	Sci::Line topLine;
	Sci::Position posTopLine;
	Sci::Position lengthForEncode;

	int needUpdateUI;

	enum { notPainting, painting, paintAbandoned } paintState;
	bool paintAbandonedByStyling;
	PRectangle rcPaint;
	bool paintingAllText;
	bool willRedrawAll;
	WorkNeeded workNeeded;
	int idleStyling;
	bool needIdleStyling;

	int modEventMask;
	bool commandEvents;

	SelectionText drag;

	CaretPolicies caretPolicies;

	CaretPolicy visiblePolicy;

	Sci::Position searchAnchor;

	bool recordingMacro;

	int foldAutomatic;

	// Wrapping support
	WrapPending wrapPending;
	ActionDuration durationWrapOneLine;

	bool convertPastes;

	Editor();
	// Deleted so Editor objects can not be copied.
	Editor(const Editor &) = delete;
	Editor(Editor &&) = delete;
	Editor &operator=(const Editor &) = delete;
	Editor &operator=(Editor &&) = delete;
	// ~Editor() in public section
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
	Point GetVisibleOriginInMain() const override;
	PointDocument DocumentPointFromView(Point ptView) const;  // Convert a point from view space to document
	Sci::Line TopLineOfMain() const override;   // Return the line at Main's y coordinate 0
	virtual PRectangle GetClientRectangle() const;
	virtual PRectangle GetClientDrawingRectangle();
	PRectangle GetTextRectangle() const;

	Sci::Line LinesOnScreen() const override;
	Sci::Line LinesToScroll() const;
	Sci::Line MaxScrollPos() const;
	SelectionPosition ClampPositionIntoDocument(SelectionPosition sp) const;
	Point LocationFromPosition(SelectionPosition pos, PointEnd pe=peDefault);
	Point LocationFromPosition(Sci::Position pos, PointEnd pe=peDefault);
	int XFromPosition(SelectionPosition sp);
	SelectionPosition SPositionFromLocation(Point pt, bool canReturnInvalid=false, bool charPosition=false, bool virtualSpace=true);
	Sci::Position PositionFromLocation(Point pt, bool canReturnInvalid = false, bool charPosition = false);
	SelectionPosition SPositionFromLineX(Sci::Line lineDoc, int x);
	Sci::Position PositionFromLineX(Sci::Line lineDoc, int x);
	Sci::Line LineFromLocation(Point pt) const;
	void SetTopLine(Sci::Line topLineNew);

	virtual bool AbandonPaint();
	virtual void RedrawRect(PRectangle rc);
	virtual void DiscardOverdraw();
	virtual void Redraw();
	void RedrawSelMargin(Sci::Line line=-1, bool allAfter=false);
	PRectangle RectangleFromRange(Range r, int overlap);
	void InvalidateRange(Sci::Position start, Sci::Position end);

	bool UserVirtualSpace() const noexcept {
		return ((virtualSpaceOptions & SCVS_USERACCESSIBLE) != 0);
	}
	Sci::Position CurrentPosition() const;
	bool SelectionEmpty() const noexcept;
	SelectionPosition SelectionStart();
	SelectionPosition SelectionEnd();
	void SetRectangularRange();
	void ThinRectangularRange();
	void InvalidateSelection(SelectionRange newMain, bool invalidateWholeSelection=false);
	void InvalidateWholeSelection();
	SelectionRange LineSelectionRange(SelectionPosition currentPos_, SelectionPosition anchor_) const;
	void SetSelection(SelectionPosition currentPos_, SelectionPosition anchor_);
	void SetSelection(Sci::Position currentPos_, Sci::Position anchor_);
	void SetSelection(SelectionPosition currentPos_);
	void SetSelection(int currentPos_);
	void SetEmptySelection(SelectionPosition currentPos_);
	void SetEmptySelection(Sci::Position currentPos_);
	enum class AddNumber { one, each };
	void MultipleSelectAdd(AddNumber addNumber);
	bool RangeContainsProtected(Sci::Position start, Sci::Position end) const noexcept;
	bool SelectionContainsProtected() const;
	Sci::Position MovePositionOutsideChar(Sci::Position pos, Sci::Position moveDir, bool checkLineEnd=true) const;
	SelectionPosition MovePositionOutsideChar(SelectionPosition pos, Sci::Position moveDir, bool checkLineEnd=true) const;
	void MovedCaret(SelectionPosition newPos, SelectionPosition previousPos,
		bool ensureVisible, CaretPolicies policies);
	void MovePositionTo(SelectionPosition newPos, Selection::selTypes selt=Selection::noSel, bool ensureVisible=true);
	void MovePositionTo(Sci::Position newPos, Selection::selTypes selt=Selection::noSel, bool ensureVisible=true);
	SelectionPosition MovePositionSoVisible(SelectionPosition pos, int moveDir);
	SelectionPosition MovePositionSoVisible(Sci::Position pos, int moveDir);
	Point PointMainCaret();
	void SetLastXChosen();

	void ScrollTo(Sci::Line line, bool moveThumb=true);
	virtual void ScrollText(Sci::Line linesToMove);
	void HorizontalScrollTo(int xPos);
	void VerticalCentreCaret();
	void MoveSelectedLines(int lineDelta);
	void MoveSelectedLinesUp();
	void MoveSelectedLinesDown();
	void MoveCaretInsideView(bool ensureVisible=true);
	Sci::Line DisplayFromPosition(Sci::Position pos);

	struct XYScrollPosition {
		int xOffset;
		Sci::Line topLine;
		XYScrollPosition(int xOffset_, Sci::Line topLine_) noexcept : xOffset(xOffset_), topLine(topLine_) {}
		bool operator==(const XYScrollPosition &other) const noexcept {
			return (xOffset == other.xOffset) && (topLine == other.topLine);
		}
	};
	enum XYScrollOptions {
		xysUseMargin=0x1,
		xysVertical=0x2,
		xysHorizontal=0x4,
		xysDefault=xysUseMargin|xysVertical|xysHorizontal};
	XYScrollPosition XYScrollToMakeVisible(const SelectionRange &range,
		const XYScrollOptions options, CaretPolicies policies);
	void SetXYScroll(XYScrollPosition newXY);
	void EnsureCaretVisible(bool useMargin=true, bool vert=true, bool horiz=true);
	void ScrollRange(SelectionRange range);
	void ShowCaretAtCurrentPosition();
	void DropCaret();
	void CaretSetPeriod(int period);
	void InvalidateCaret();
	virtual void NotifyCaretMove();
	virtual void UpdateSystemCaret();

	bool Wrapping() const noexcept;
	void NeedWrapping(Sci::Line docLineStart=0, Sci::Line docLineEnd=WrapPending::lineLarge);
	bool WrapOneLine(Surface *surface, Sci::Line lineToWrap);
	enum class WrapScope {wsAll, wsVisible, wsIdle};
	bool WrapLines(WrapScope ws);
	void LinesJoin();
	void LinesSplit(int pixelWidth);

	void PaintSelMargin(Surface *surfaceWindow, const PRectangle &rc);
	void RefreshPixMaps(Surface *surfaceWindow);
	void Paint(Surface *surfaceWindow, PRectangle rcArea);
	Sci::Position FormatRange(bool draw, const Sci_RangeToFormat *pfr);
	long TextWidth(uptr_t style, const char *text);

	virtual void SetVerticalScrollPos() = 0;
	virtual void SetHorizontalScrollPos() = 0;
	virtual bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) = 0;
	virtual void ReconfigureScrollBars();
	void SetScrollBars();
	void ChangeSize();

	void FilterSelections();
	Sci::Position RealizeVirtualSpace(Sci::Position position, Sci::Position virtualSpace);
	SelectionPosition RealizeVirtualSpace(const SelectionPosition &position);
	void AddChar(char ch);
	virtual void InsertCharacter(std::string_view sv, CharacterSource charSource);
	void ClearBeforeTentativeStart();
	void InsertPaste(const char *text, Sci::Position len);
	enum PasteShape { pasteStream=0, pasteRectangular = 1, pasteLine = 2 };
	void InsertPasteShape(const char *text, Sci::Position len, PasteShape shape);
	void ClearSelection(bool retainMultipleSelections = false);
	void ClearAll();
	void ClearDocumentStyle();
	virtual void Cut();
	void PasteRectangular(SelectionPosition pos, const char *ptr, Sci::Position len);
	virtual void Copy() = 0;
	void CopyAllowLine();
	virtual bool CanPaste();
	virtual void Paste() = 0;
	void Clear();
	virtual void SelectAll();
	virtual void Undo();
	virtual void Redo();
	void DelCharBack(bool allowLineStartDeletion);
	virtual void ClaimSelection() = 0;

	static int ModifierFlags(bool shift, bool ctrl, bool alt, bool meta=false, bool super=false) noexcept;
	virtual void NotifyChange() = 0;
	virtual void NotifyFocus(bool focus);
	virtual void SetCtrlID(int identifier);
	virtual int GetCtrlID() { return ctrlID; }
	virtual void NotifyParent(SCNotification scn) = 0;
	virtual void NotifyStyleToNeeded(Sci::Position endStyleNeeded);
	void NotifyChar(int ch, CharacterSource charSource);
	void NotifySavePoint(bool isSavePoint);
	void NotifyModifyAttempt();
	virtual void NotifyDoubleClick(Point pt, int modifiers);
	void NotifyHotSpotClicked(Sci::Position position, int modifiers);
	void NotifyHotSpotDoubleClicked(Sci::Position position, int modifiers);
	void NotifyHotSpotReleaseClick(Sci::Position position, int modifiers);
	bool NotifyUpdateUI();
	void NotifyPainted();
	void NotifyIndicatorClick(bool click, Sci::Position position, int modifiers);
	bool NotifyMarginClick(Point pt, int modifiers);
	bool NotifyMarginRightClick(Point pt, int modifiers);
	void NotifyNeedShown(Sci::Position pos, Sci::Position len);
	void NotifyDwelling(Point pt, bool state);
	void NotifyZoom();

	void NotifyModifyAttempt(Document *document, void *userData) override;
	void NotifySavePoint(Document *document, void *userData, bool atSavePoint) override;
	void CheckModificationForWrap(DocModification mh);
	void NotifyModified(Document *document, DocModification mh, void *userData) override;
	void NotifyDeleted(Document *document, void *userData) noexcept override;
	void NotifyStyleNeeded(Document *doc, void *userData, Sci::Position endStyleNeeded) override;
	void NotifyLexerChanged(Document *doc, void *userData) override;
	void NotifyErrorOccurred(Document *doc, void *userData, int status) override;
	void NotifyMacroRecord(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

	void ContainerNeedsUpdate(int flags) noexcept;
	void PageMove(int direction, Selection::selTypes selt=Selection::noSel, bool stuttered = false);
	enum { cmSame, cmUpper, cmLower };
	virtual std::string CaseMapString(const std::string &s, int caseMapping);
	void ChangeCaseOfSelection(int caseMapping);
	void LineTranspose();
	void LineReverse();
	void Duplicate(bool forLine);
	virtual void CancelModes();
	void NewLine();
	SelectionPosition PositionUpOrDown(SelectionPosition spStart, int direction, int lastX);
	void CursorUpOrDown(int direction, Selection::selTypes selt);
	void ParaUpOrDown(int direction, Selection::selTypes selt);
	Range RangeDisplayLine(Sci::Line lineVisible);
	Sci::Position StartEndDisplayLine(Sci::Position pos, bool start);
	Sci::Position VCHomeDisplayPosition(Sci::Position position);
	Sci::Position VCHomeWrapPosition(Sci::Position position);
	Sci::Position LineEndWrapPosition(Sci::Position position);
	int HorizontalMove(unsigned int iMessage);
	int DelWordOrLine(unsigned int iMessage);
	virtual int KeyCommand(unsigned int iMessage);
	virtual int KeyDefault(int /* key */, int /*modifiers*/);
	int KeyDownWithModifiers(int key, int modifiers, bool *consumed);

	void Indent(bool forwards);

	virtual CaseFolder *CaseFolderForEncoding();
	Sci::Position FindText(uptr_t wParam, sptr_t lParam);
	void SearchAnchor();
	Sci::Position SearchText(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	Sci::Position SearchInTarget(const char *text, Sci::Position length);
	void GoToLine(Sci::Line lineNo);

	virtual void CopyToClipboard(const SelectionText &selectedText) = 0;
	std::string RangeText(Sci::Position start, Sci::Position end) const;
	void CopySelectionRange(SelectionText *ss, bool allowLineCopy=false);
	void CopyRangeToClipboard(Sci::Position start, Sci::Position end);
	void CopyText(size_t length, const char *text);
	void SetDragPosition(SelectionPosition newPos);
	virtual void DisplayCursor(Window::Cursor c);
	virtual bool DragThreshold(Point ptStart, Point ptNow);
	virtual void StartDrag();
	void DropAt(SelectionPosition position, const char *value, size_t lengthValue, bool moving, bool rectangular);
	void DropAt(SelectionPosition position, const char *value, bool moving, bool rectangular);
	/** PositionInSelection returns true if position in selection. */
	bool PositionInSelection(Sci::Position pos);
	bool PointInSelection(Point pt);
	bool PointInSelMargin(Point pt) const;
	Window::Cursor GetMarginCursor(Point pt) const noexcept;
	void TrimAndSetSelection(Sci::Position currentPos_, Sci::Position anchor_);
	void LineSelection(Sci::Position lineCurrentPos_, Sci::Position lineAnchorPos_, bool wholeLine);
	void WordSelection(Sci::Position pos);
	void DwellEnd(bool mouseMoved);
	void MouseLeave();
	virtual void ButtonDownWithModifiers(Point pt, unsigned int curTime, int modifiers);
	virtual void RightButtonDownWithModifiers(Point pt, unsigned int curTime, int modifiers);
	void ButtonMoveWithModifiers(Point pt, unsigned int curTime, int modifiers);
	void ButtonUpWithModifiers(Point pt, unsigned int curTime, int modifiers);

	bool Idle();
	enum TickReason { tickCaret, tickScroll, tickWiden, tickDwell, tickPlatform };
	virtual void TickFor(TickReason reason);
	virtual bool FineTickerRunning(TickReason reason);
	virtual void FineTickerStart(TickReason reason, int millis, int tolerance);
	virtual void FineTickerCancel(TickReason reason);
	virtual bool SetIdle(bool) { return false; }
	virtual void SetMouseCapture(bool on) = 0;
	virtual bool HaveMouseCapture() = 0;
	void SetFocusState(bool focusState);

	Sci::Position PositionAfterArea(PRectangle rcArea) const;
	void StyleToPositionInView(Sci::Position pos);
	Sci::Position PositionAfterMaxStyling(Sci::Position posMax, bool scrolling) const;
	void StartIdleStyling(bool truncatedLastStyling);
	void StyleAreaBounded(PRectangle rcArea, bool scrolling);
	constexpr bool SynchronousStylingToVisible() const noexcept {
		return (idleStyling == SC_IDLESTYLING_NONE) || (idleStyling == SC_IDLESTYLING_AFTERVISIBLE);
	}
	void IdleStyling();
	virtual void IdleWork();
	virtual void QueueIdleWork(WorkNeeded::workItems items, Sci::Position upTo=0);

	virtual bool PaintContains(PRectangle rc);
	bool PaintContainsMargin();
	void CheckForChangeOutsidePaint(Range r);
	void SetBraceHighlight(Sci::Position pos0, Sci::Position pos1, int matchStyle);

	void SetAnnotationHeights(Sci::Line start, Sci::Line end);
	virtual void SetDocPointer(Document *document);

	void SetAnnotationVisible(int visible);
	void SetEOLAnnotationVisible(int visible);

	Sci::Line ExpandLine(Sci::Line line);
	void SetFoldExpanded(Sci::Line lineDoc, bool expanded);
	void FoldLine(Sci::Line line, int action);
	void FoldExpand(Sci::Line line, int action, int level);
	Sci::Line ContractedFoldNext(Sci::Line lineStart) const;
	void EnsureLineVisible(Sci::Line lineDoc, bool enforcePolicy);
	void FoldChanged(Sci::Line line, int levelNow, int levelPrev);
	void NeedShown(Sci::Position pos, Sci::Position len);
	void FoldAll(int action);

	Sci::Position GetTag(char *tagValue, int tagNumber);
	Sci::Position ReplaceTarget(bool replacePatterns, const char *text, Sci::Position length=-1);

	bool PositionIsHotspot(Sci::Position position) const;
	bool PointIsHotspot(Point pt);
	void SetHotSpotRange(const Point *pt);
	Range GetHotSpotRange() const noexcept override;
	void SetHoverIndicatorPosition(Sci::Position position);
	void SetHoverIndicatorPoint(Point pt);

	int CodePage() const noexcept;
	virtual bool ValidCodePage(int /* codePage */) const { return true; }
	Sci::Line WrapCount(Sci::Line line);
	void AddStyledText(const char *buffer, Sci::Position appendLength);

	virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) = 0;
	bool ValidMargin(uptr_t wParam) const noexcept;
	void StyleSetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t StyleGetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	void SetSelectionNMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

	static const char *StringFromEOLMode(int eolMode) noexcept;

	// Coercion functions for transforming WndProc parameters into pointers
	static void *PtrFromSPtr(sptr_t lParam) noexcept {
		return reinterpret_cast<void *>(lParam);
	}
	static const char *ConstCharPtrFromSPtr(sptr_t lParam) noexcept {
		return static_cast<const char *>(PtrFromSPtr(lParam));
	}
	static const unsigned char *ConstUCharPtrFromSPtr(sptr_t lParam) noexcept {
		return static_cast<const unsigned char *>(PtrFromSPtr(lParam));
	}
	static char *CharPtrFromSPtr(sptr_t lParam) noexcept {
		return static_cast<char *>(PtrFromSPtr(lParam));
	}
	static unsigned char *UCharPtrFromSPtr(sptr_t lParam) noexcept {
		return static_cast<unsigned char *>(PtrFromSPtr(lParam));
	}
	static void *PtrFromUPtr(uptr_t wParam) noexcept {
		return reinterpret_cast<void *>(wParam);
	}
	static const char *ConstCharPtrFromUPtr(uptr_t wParam) noexcept {
		return static_cast<const char *>(PtrFromUPtr(wParam));
	}

	static sptr_t StringResult(sptr_t lParam, const char *val) noexcept;
	static sptr_t BytesResult(sptr_t lParam, const unsigned char *val, size_t len) noexcept;

	// Set a variable controlling appearance to a value and invalidates the display
	// if a change was made. Avoids extra text and the possibility of mistyping.
	template <typename T>
	bool SetAppearance(T &variable, T value) {
		// Using ! and == as more types have == defined than !=.
		const bool changed = !(variable == value);
		if (changed) {
			variable = value;
			InvalidateStyleRedraw();
		}
		return changed;
	}

public:
	~Editor() override;

	// Public so the COM thunks can access it.
	bool IsUnicodeMode() const noexcept;
	// Public so scintilla_send_message can use it.
	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	// Public so scintilla_set_id can use it.
	int ctrlID;
	// Public so COM methods for drag and drop can set it.
	int errorStatus;
	friend class AutoSurface;
};

/**
 * A smart pointer class to ensure Surfaces are set up and deleted correctly.
 */
class AutoSurface {
private:
	std::unique_ptr<Surface> surf;
public:
	AutoSurface(const Editor *ed, int technology = -1) {
		if (ed->wMain.GetID()) {
			surf.reset(Surface::Allocate(technology != -1 ? technology : ed->technology));
			surf->Init(ed->wMain.GetID());
			surf->SetUnicodeMode(SC_CP_UTF8 == ed->CodePage());
			surf->SetDBCSMode(ed->CodePage());
			surf->SetBidiR2L(ed->BidirectionalR2L());
		}
	}
	AutoSurface(SurfaceID sid, Editor *ed, int technology = -1) {
		if (ed->wMain.GetID()) {
			surf.reset(Surface::Allocate(technology != -1 ? technology : ed->technology));
			surf->Init(sid, ed->wMain.GetID());
			surf->SetUnicodeMode(SC_CP_UTF8 == ed->CodePage());
			surf->SetDBCSMode(ed->CodePage());
			surf->SetBidiR2L(ed->BidirectionalR2L());
		}
	}
	// Deleted so AutoSurface objects can not be copied.
	AutoSurface(const AutoSurface &) = delete;
	AutoSurface(AutoSurface &&) = delete;
	void operator=(const AutoSurface &) = delete;
	void operator=(AutoSurface &&) = delete;
	~AutoSurface() {
	}
	Surface *operator->() const noexcept {
		return surf.get();
	}
	operator Surface *() const noexcept {
		return surf.get();
	}
};

}

#endif
