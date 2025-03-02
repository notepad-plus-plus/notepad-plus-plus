// Scintilla source code edit control
/** @file Editor.h
 ** Defines the main editor class.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITOR_H
#define EDITOR_H

namespace Scintilla::Internal {

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

enum class WorkItems {
	none = 0,
	style = 1,
	updateUI = 2
};

class WorkNeeded {
public:
	enum WorkItems items;
	Sci::Position upTo;

	WorkNeeded() noexcept : items(WorkItems::none), upTo(0) {}
	void Reset() noexcept {
		items = WorkItems::none;
		upTo = 0;
	}
	void Need(WorkItems items_, Sci::Position pos) noexcept {
		if (Scintilla::FlagSet(items_, WorkItems::style) && (upTo < pos))
			upTo = pos;
		items = static_cast<WorkItems>(static_cast<int>(items) | static_cast<int>(items_));
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
	Scintilla::CharacterSet characterSet;
	SelectionText() noexcept : rectangular(false), lineCopy(false), codePage(0), characterSet(Scintilla::CharacterSet::Ansi) {}
	void Clear() noexcept {
		s.clear();
		rectangular = false;
		lineCopy = false;
		codePage = 0;
		characterSet = Scintilla::CharacterSet::Ansi;
	}
	void Copy(const std::string &s_, int codePage_, Scintilla::CharacterSet characterSet_, bool rectangular_, bool lineCopy_) {
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

struct CaretPolicySlop {
	Scintilla::CaretPolicy policy;	// Combination from CaretPolicy::Slop, CaretPolicy::Strict, CaretPolicy::Jumps, CaretPolicy::Even
	int slop;	// Pixels for X, lines for Y
	CaretPolicySlop(Scintilla::CaretPolicy policy_, intptr_t slop_) noexcept :
		policy(policy_), slop(static_cast<int>(slop_)) {}
	CaretPolicySlop(uintptr_t policy_=0, intptr_t slop_=0) noexcept :
		policy(static_cast<Scintilla::CaretPolicy>(policy_)), slop(static_cast<int>(slop_)) {}
};

struct CaretPolicies {
	CaretPolicySlop x;
	CaretPolicySlop y;
};

struct VisiblePolicySlop {
	Scintilla::VisiblePolicy policy;	// Combination from VisiblePolicy::Slop, VisiblePolicy::Strict
	int slop;	// Pixels for X, lines for Y
	VisiblePolicySlop(uintptr_t policy_ = 0, intptr_t slop_ = 0) noexcept :
		policy(static_cast<Scintilla::VisiblePolicy>(policy_)), slop(static_cast<int>(slop_)) {}
};

enum class XYScrollOptions {
	none = 0x0,
	useMargin = 0x1,
	vertical = 0x2,
	horizontal = 0x4,
	all = useMargin | vertical | horizontal
};

constexpr XYScrollOptions operator|(XYScrollOptions a, XYScrollOptions b) noexcept {
	return static_cast<XYScrollOptions>(static_cast<int>(a) | static_cast<int>(b));
}

/**
 */
class Editor : public EditModel, public DocWatcher {
protected:	// ScintillaBase subclass needs access to much of Editor

	/** On GTK+, Scintilla is a container widget holding two scroll bars
	 * whereas on Windows there is just one window with both scroll bars turned on. */
	Window wMain;	///< The Scintilla parent window
	Window wMargin;	///< May be separate when using a scroll view for wMain

	// Optimization that avoids superfluous invalidations
	bool redrawPendingText = false;
	bool redrawPendingMargin = false;

	/** Style resources may be expensive to allocate so are cached between uses.
	 * When a style attribute is changed, this cache is flushed. */
	bool stylesValid;
	ViewStyle vs;
	Scintilla::Technology technology;
	Point sizeRGBAImage;
	float scaleRGBAImage;

	MarginView marginView;
	EditView view;

	Scintilla::CursorShape cursorMode;

	bool mouseDownCaptures;
	bool mouseWheelCaptures;

	int xCaretMargin;	///< Ensure this many pixels visible on both sides of caret
	bool horizontalScrollBarVisible;
	int scrollWidth;
	bool verticalScrollBarVisible;
	bool endAtLastLine;
	Scintilla::CaretSticky caretSticky;
	Scintilla::MarginOption marginOptions;
	bool mouseSelectionRectangularSwitch;
	bool multipleSelection;
	bool additionalSelectionTyping;
	Scintilla::MultiPaste multiPasteMode;

	Scintilla::VirtualSpace virtualSpaceOptions;

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
	enum class DragDrop { none, initial, dragging } inDragDrop;
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
	Scintilla::FindOption searchFlags;
	Sci::Line topLine;
	Sci::Position posTopLine;
	Sci::Position lengthForEncode;

	Scintilla::Update needUpdateUI;

	enum class PaintState { notPainting, painting, abandoned } paintState;
	bool paintAbandonedByStyling;
	PRectangle rcPaint;
	bool paintingAllText;
	bool willRedrawAll;
	WorkNeeded workNeeded;
	Scintilla::IdleStyling idleStyling;
	bool needIdleStyling;

	Scintilla::ModificationFlags modEventMask;
	bool commandEvents;

	SelectionText drag;

	CaretPolicies caretPolicies;

	VisiblePolicySlop visiblePolicy;

	Sci::Position searchAnchor;

	bool recordingMacro;

	Scintilla::AutomaticFold foldAutomatic;

	// Wrapping support
	WrapPending wrapPending;
	ActionDuration durationWrapOneByte;

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

	void InvalidateStyleData() noexcept;
	void InvalidateStyleRedraw();
	void RefreshStyleData();
	void SetRepresentations();
	void DropGraphics() noexcept;

	bool HasMarginWindow() const noexcept;
	// The top left visible point in main window coordinates. Will be 0,0 except for
	// scroll views where it will be equivalent to the current scroll position.
	Point GetVisibleOriginInMain() const override;
	PointDocument DocumentPointFromView(Point ptView) const;  // Convert a point from view space to document
	Sci::Line TopLineOfMain() const noexcept final;   // Return the line at Main's y coordinate 0
	virtual Point ClientSize() const;
	virtual PRectangle GetClientRectangle() const;
	virtual PRectangle GetClientDrawingRectangle();
	PRectangle GetTextRectangle() const;

	Sci::Line LinesOnScreen() const override;
	Sci::Line LinesToScroll() const;
	Sci::Line MaxScrollPos() const;
	SelectionPosition ClampPositionIntoDocument(SelectionPosition sp) const;
	Point LocationFromPosition(SelectionPosition pos, PointEnd pe=PointEnd::start);
	Point LocationFromPosition(Sci::Position pos, PointEnd pe=PointEnd::start);
	int XFromPosition(SelectionPosition sp);
	SelectionPosition SPositionFromLocation(Point pt, bool canReturnInvalid=false, bool charPosition=false, bool virtualSpace=true);
	Sci::Position PositionFromLocation(Point pt, bool canReturnInvalid = false, bool charPosition = false);
	SelectionPosition SPositionFromLineX(Sci::Line lineDoc, int x);
	Sci::Position PositionFromLineX(Sci::Line lineDoc, int x);
	Sci::Line LineFromLocation(Point pt) const noexcept;
	void SetTopLine(Sci::Line topLineNew);

	virtual bool AbandonPaint();
	virtual void RedrawRect(PRectangle rc);
	virtual void DiscardOverdraw();
	virtual void Redraw();
	void RedrawSelMargin(Sci::Line line=-1, bool allAfter=false);
	PRectangle RectangleFromRange(Range r, int overlap);
	void InvalidateRange(Sci::Position start, Sci::Position end);

	bool UserVirtualSpace() const noexcept {
		return (FlagSet(virtualSpaceOptions, Scintilla::VirtualSpace::UserAccessible));
	}
	Sci::Position CurrentPosition() const noexcept;
	bool SelectionEmpty() const noexcept;
	SelectionPosition SelectionStart() noexcept;
	SelectionPosition SelectionEnd() noexcept;
	void SetRectangularRange();
	void ThinRectangularRange();
	void InvalidateSelection(SelectionRange newMain, bool invalidateWholeSelection=false);
	void InvalidateWholeSelection();
	SelectionRange LineSelectionRange(SelectionPosition currentPos_, SelectionPosition anchor_) const;
	void SetSelection(SelectionPosition currentPos_, SelectionPosition anchor_);
	void SetSelection(Sci::Position currentPos_, Sci::Position anchor_);
	void SetSelection(SelectionPosition currentPos_);
	void SetEmptySelection(SelectionPosition currentPos_);
	void SetEmptySelection(Sci::Position currentPos_);
	void SetSelectionFromSerialized(const char *serialized);
	enum class AddNumber { one, each };
	void MultipleSelectAdd(AddNumber addNumber);
	bool RangeContainsProtected(Sci::Position start, Sci::Position end) const noexcept;
	bool RangeContainsProtected(const SelectionRange &range) const noexcept;
	bool SelectionContainsProtected() const noexcept;
	Sci::Position MovePositionOutsideChar(Sci::Position pos, Sci::Position moveDir, bool checkLineEnd=true) const;
	SelectionPosition MovePositionOutsideChar(SelectionPosition pos, Sci::Position moveDir, bool checkLineEnd=true) const;
	void MovedCaret(SelectionPosition newPos, SelectionPosition previousPos,
		bool ensureVisible, CaretPolicies policies);
	void MovePositionTo(SelectionPosition newPos, Selection::SelTypes selt=Selection::SelTypes::none, bool ensureVisible=true);
	void MovePositionTo(Sci::Position newPos, Selection::SelTypes selt=Selection::SelTypes::none, bool ensureVisible=true);
	SelectionPosition MovePositionSoVisible(SelectionPosition pos, int moveDir);
	SelectionPosition MovePositionSoVisible(Sci::Position pos, int moveDir);
	Point PointMainCaret();
	void SetLastXChosen();
	void RememberSelectionForUndo(int index);
	void RememberSelectionOntoStack(int index);
	void RememberCurrentSelectionForRedoOntoStack();

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
	bool WrapBlock(Surface *surface, Sci::Line lineToWrap, Sci::Line lineToWrapEnd);
	enum class WrapScope {wsAll, wsVisible, wsIdle};
	bool WrapLines(WrapScope ws);
	void LinesJoin();
	void LinesSplit(int pixelWidth);

	void PaintSelMargin(Surface *surfaceWindow, const PRectangle &rc);
	void RefreshPixMaps(Surface *surfaceWindow);
	void Paint(Surface *surfaceWindow, PRectangle rcArea);
	Sci::Position FormatRange(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	long TextWidth(Scintilla::uptr_t style, const char *text);

	virtual void SetVerticalScrollPos() = 0;
	virtual void SetHorizontalScrollPos() = 0;
	virtual bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) = 0;
	virtual void ReconfigureScrollBars();
	void ChangeScrollBars();
	virtual void SetScrollBars();
	void ChangeSize();

	void FilterSelections();
	Sci::Position RealizeVirtualSpace(Sci::Position position, Sci::Position virtualSpace);
	SelectionPosition RealizeVirtualSpace(const SelectionPosition &position);
	void AddChar(char ch);
	virtual void InsertCharacter(std::string_view sv, Scintilla::CharacterSource charSource);
	void ClearSelectionRange(SelectionRange &range);
	void ClearBeforeTentativeStart();
	void InsertPaste(const char *text, Sci::Position len);
	enum class PasteShape { stream=0, rectangular = 1, line = 2 };
	void InsertPasteShape(const char *text, Sci::Position len, PasteShape shape);
	void ClearSelection(bool retainMultipleSelections = false);
	void ClearAll();
	void ClearDocumentStyle();
	virtual void Cut();
	void PasteRectangular(SelectionPosition pos, const char *ptr, Sci::Position len);
	virtual void Copy() = 0;
	void CopyAllowLine();
	void CutAllowLine();
	virtual bool CanPaste();
	virtual void Paste() = 0;
	void Clear();
	virtual void SelectAll();
	void RestoreSelection(Sci::Position newPos, UndoRedo history);
	virtual void Undo();
	virtual void Redo();
	void DelCharBack(bool allowLineStartDeletion);
	virtual void ClaimSelection() = 0;

	virtual void NotifyChange() = 0;
	virtual void NotifyFocus(bool focus);
	virtual void SetCtrlID(int identifier);
	virtual int GetCtrlID() { return ctrlID; }
	virtual void NotifyParent(Scintilla::NotificationData scn) = 0;
	virtual void NotifyStyleToNeeded(Sci::Position endStyleNeeded);
	void NotifyChar(int ch, Scintilla::CharacterSource charSource);
	void NotifySavePoint(bool isSavePoint);
	void NotifyModifyAttempt();
	virtual void NotifyDoubleClick(Point pt, Scintilla::KeyMod modifiers);
	void NotifyHotSpotClicked(Sci::Position position, Scintilla::KeyMod modifiers);
	void NotifyHotSpotDoubleClicked(Sci::Position position, Scintilla::KeyMod modifiers);
	void NotifyHotSpotReleaseClick(Sci::Position position, Scintilla::KeyMod modifiers);
	bool NotifyUpdateUI();
	void NotifyPainted();
	void NotifyIndicatorClick(bool click, Sci::Position position, Scintilla::KeyMod modifiers);
	bool NotifyMarginClick(Point pt, Scintilla::KeyMod modifiers);
	bool NotifyMarginRightClick(Point pt, Scintilla::KeyMod modifiers);
	void NotifyNeedShown(Sci::Position pos, Sci::Position len);
	void NotifyDwelling(Point pt, bool state);
	void NotifyZoom();

	void NotifyModifyAttempt(Document *document, void *userData) override;
	void NotifySavePoint(Document *document, void *userData, bool atSavePoint) override;
	void CheckModificationForWrap(DocModification mh);
	void NotifyModified(Document *document, DocModification mh, void *userData) override;
	void NotifyDeleted(Document *document, void *userData) noexcept override;
	void NotifyStyleNeeded(Document *doc, void *userData, Sci::Position endStyleNeeded) override;
	void NotifyErrorOccurred(Document *doc, void *userData, Scintilla::Status status) override;
	void NotifyGroupCompleted(Document *, void *) noexcept override;
	void NotifyMacroRecord(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);

	void ContainerNeedsUpdate(Scintilla::Update flags) noexcept;
	void PageMove(int direction, Selection::SelTypes selt=Selection::SelTypes::none, bool stuttered = false);
	enum class CaseMapping { same, upper, lower };
	virtual std::string CaseMapString(const std::string &s, CaseMapping caseMapping);
	void ChangeCaseOfSelection(CaseMapping caseMapping);
	void LineDelete();
	void LineTranspose();
	void LineReverse();
	void Duplicate(bool forLine);
	virtual void CancelModes();
	void NewLine();
	SelectionPosition PositionUpOrDown(SelectionPosition spStart, int direction, int lastX);
	void CursorUpOrDown(int direction, Selection::SelTypes selt);
	void ParaUpOrDown(int direction, Selection::SelTypes selt);
	Range RangeDisplayLine(Sci::Line lineVisible);
	Sci::Position StartEndDisplayLine(Sci::Position pos, bool start);
	Sci::Position HomeWrapPosition(Sci::Position position);
	Sci::Position VCHomeDisplayPosition(Sci::Position position);
	Sci::Position VCHomeWrapPosition(Sci::Position position);
	Sci::Position LineEndWrapPosition(Sci::Position position);
	SelectionPosition PositionMove(Scintilla::Message iMessage, SelectionPosition spCaretNow);
	SelectionRange SelectionMove(Scintilla::Message iMessage, size_t r);
	int HorizontalMove(Scintilla::Message iMessage);
	int DelWordOrLine(Scintilla::Message iMessage);
	virtual int KeyCommand(Scintilla::Message iMessage);
	virtual int KeyDefault(Scintilla::Keys /* key */, Scintilla::KeyMod /*modifiers*/);
	int KeyDownWithModifiers(Scintilla::Keys key, Scintilla::KeyMod modifiers, bool *consumed);

	void Indent(bool forwards, bool lineIndent);

	virtual std::unique_ptr<CaseFolder> CaseFolderForEncoding();
	Sci::Position FindText(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	Sci::Position FindTextFull(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	void SearchAnchor() noexcept;
	Sci::Position SearchText(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	Sci::Position SearchInTarget(const char *text, Sci::Position length);
	void GoToLine(Sci::Line lineNo);

	virtual void CopyToClipboard(const SelectionText &selectedText) = 0;
	std::string RangeText(Sci::Position start, Sci::Position end) const;
	bool CopyLineRange(SelectionText *ss, bool allowProtected=true);
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
	ptrdiff_t SelectionFromPoint(Point pt);
	bool PointInSelMargin(Point pt) const;
	Window::Cursor GetMarginCursor(Point pt) const noexcept;
	void DropSelection(size_t part);
	void TrimAndSetSelection(Sci::Position currentPos_, Sci::Position anchor_);
	void LineSelection(Sci::Position lineCurrentPos_, Sci::Position lineAnchorPos_, bool wholeLine);
	void WordSelection(Sci::Position pos);
	void DwellEnd(bool mouseMoved);
	void MouseLeave();
	virtual void ButtonDownWithModifiers(Point pt, unsigned int curTime, Scintilla::KeyMod modifiers);
	virtual void RightButtonDownWithModifiers(Point pt, unsigned int curTime, Scintilla::KeyMod modifiers);
	void ButtonMoveWithModifiers(Point pt, unsigned int curTime, Scintilla::KeyMod modifiers);
	void ButtonUpWithModifiers(Point pt, unsigned int curTime, Scintilla::KeyMod modifiers);

	bool Idle();
	enum class TickReason { caret, scroll, widen, dwell, platform };
	virtual void TickFor(TickReason reason);
	virtual bool FineTickerRunning(TickReason reason);
	virtual void FineTickerStart(TickReason reason, int millis, int tolerance);
	virtual void FineTickerCancel(TickReason reason);
	virtual bool SetIdle(bool) { return false; }
	void ChangeMouseCapture(bool on);
	virtual void SetMouseCapture(bool on) = 0;
	virtual bool HaveMouseCapture() = 0;
	void SetFocusState(bool focusState);
	virtual void UpdateBaseElements();

	Sci::Position PositionAfterArea(PRectangle rcArea) const;
	void StyleToPositionInView(Sci::Position pos);
	Sci::Position PositionAfterMaxStyling(Sci::Position posMax, bool scrolling) const;
	void StartIdleStyling(bool truncatedLastStyling);
	void StyleAreaBounded(PRectangle rcArea, bool scrolling);
	constexpr bool SynchronousStylingToVisible() const noexcept {
		return (idleStyling == Scintilla::IdleStyling::None) || (idleStyling == Scintilla::IdleStyling::AfterVisible);
	}
	void IdleStyle();
	virtual void IdleWork();
	virtual void QueueIdleWork(WorkItems items, Sci::Position upTo=0);

	virtual int SupportsFeature(Scintilla::Supports feature);
	virtual bool PaintContains(PRectangle rc);
	bool PaintContainsMargin();
	void CheckForChangeOutsidePaint(Range r);
	void SetBraceHighlight(Sci::Position pos0, Sci::Position pos1, int matchStyle);

	void SetAnnotationHeights(Sci::Line start, Sci::Line end);
	virtual void SetDocPointer(Document *document);

	void SetAnnotationVisible(Scintilla::AnnotationVisible visible);
	void SetEOLAnnotationVisible(Scintilla::EOLAnnotationVisible visible);

	Sci::Line ExpandLine(Sci::Line line);
	void SetFoldExpanded(Sci::Line lineDoc, bool expanded);
	void FoldLine(Sci::Line line, Scintilla::FoldAction action);
	void FoldExpand(Sci::Line line, Scintilla::FoldAction action, Scintilla::FoldLevel level);
	Sci::Line ContractedFoldNext(Sci::Line lineStart) const;
	void EnsureLineVisible(Sci::Line lineDoc, bool enforcePolicy);
	void FoldChanged(Sci::Line line, Scintilla::FoldLevel levelNow, Scintilla::FoldLevel levelPrev);
	void NeedShown(Sci::Position pos, Sci::Position len);
	void FoldAll(Scintilla::FoldAction action);

	Sci::Position GetTag(char *tagValue, int tagNumber);
	enum class ReplaceType {basic, patterns, minimal};
	Sci::Position ReplaceTarget(ReplaceType replaceType, std::string_view text);

	bool PositionIsHotspot(Sci::Position position) const noexcept;
	bool PointIsHotspot(Point pt);
	void SetHotSpotRange(const Point *pt);
	void SetHoverIndicatorPosition(Sci::Position position);
	void SetHoverIndicatorPoint(Point pt);

	int CodePage() const noexcept;
	virtual bool ValidCodePage(int /* codePage */) const { return true; }
	virtual std::string UTF8FromEncoded(std::string_view encoded) const = 0;
	virtual std::string EncodedFromUTF8(std::string_view utf8) const = 0;
	virtual std::unique_ptr<Surface> CreateMeasurementSurface() const;
	virtual std::unique_ptr<Surface> CreateDrawingSurface(SurfaceID sid, std::optional<Scintilla::Technology> technologyOpt = {}) const;

	Sci::Line WrapCount(Sci::Line line);
	void AddStyledText(const char *buffer, Sci::Position appendLength);
	Sci::Position GetStyledText(char *buffer, Sci::Position cpMin, Sci::Position cpMax) const noexcept;
	Sci::Position GetTextRange(char *buffer, Sci::Position cpMin, Sci::Position cpMax) const;

	virtual Scintilla::sptr_t DefWndProc(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam) = 0;
	bool ValidMargin(Scintilla::uptr_t wParam) const noexcept;
	void StyleSetMessage(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	Scintilla::sptr_t StyleGetMessage(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	void SetSelectionNMessage(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	void SetSelectionMode(uptr_t wParam, bool setMoveExtends);

	// Coercion functions for transforming WndProc parameters into pointers
	static void *PtrFromSPtr(Scintilla::sptr_t lParam) noexcept {
		return reinterpret_cast<void *>(lParam);
	}
	static const char *ConstCharPtrFromSPtr(Scintilla::sptr_t lParam) noexcept {
		return static_cast<const char *>(PtrFromSPtr(lParam));
	}
	static const unsigned char *ConstUCharPtrFromSPtr(Scintilla::sptr_t lParam) noexcept {
		return static_cast<const unsigned char *>(PtrFromSPtr(lParam));
	}
	static char *CharPtrFromSPtr(Scintilla::sptr_t lParam) noexcept {
		return static_cast<char *>(PtrFromSPtr(lParam));
	}
	static unsigned char *UCharPtrFromSPtr(Scintilla::sptr_t lParam) noexcept {
		return static_cast<unsigned char *>(PtrFromSPtr(lParam));
	}
	static std::string_view ViewFromParams(Scintilla::sptr_t lParam, Scintilla::uptr_t wParam) noexcept {
		if (SPtrFromUPtr(wParam) == -1) {
			return std::string_view(CharPtrFromSPtr(lParam));
		}
		return std::string_view(CharPtrFromSPtr(lParam), wParam);
	}
	static void *PtrFromUPtr(Scintilla::uptr_t wParam) noexcept {
		return reinterpret_cast<void *>(wParam);
	}
	static const char *ConstCharPtrFromUPtr(Scintilla::uptr_t wParam) noexcept {
		return static_cast<const char *>(PtrFromUPtr(wParam));
	}

	static constexpr Scintilla::sptr_t SPtrFromUPtr(Scintilla::uptr_t wParam) noexcept {
		return static_cast<Scintilla::sptr_t>(wParam);
	}
	static constexpr Sci::Position PositionFromUPtr(Scintilla::uptr_t wParam) noexcept {
		return SPtrFromUPtr(wParam);
	}
	static constexpr Sci::Line LineFromUPtr(Scintilla::uptr_t wParam) noexcept {
		return SPtrFromUPtr(wParam);
	}
	Point PointFromParameters(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam) const noexcept {
		return Point(static_cast<XYPOSITION>(wParam) - vs.ExternalMarginWidth(), static_cast<XYPOSITION>(lParam));
	}

	static constexpr std::optional<FoldLevel> OptionalFoldLevel(Scintilla::sptr_t lParam) {
		if (lParam >= 0) {
			return static_cast<FoldLevel>(lParam);
		}
		return std::nullopt;
	}

	static Scintilla::sptr_t StringResult(Scintilla::sptr_t lParam, const char *val) noexcept;
	static Scintilla::sptr_t BytesResult(Scintilla::sptr_t lParam, const unsigned char *val, size_t len) noexcept;
	static Scintilla::sptr_t BytesResult(Scintilla::sptr_t lParam, std::string_view sv) noexcept;

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
	virtual Scintilla::sptr_t WndProc(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	// Public so scintilla_set_id can use it.
	int ctrlID;
	// Public so COM methods for drag and drop can set it.
	Scintilla::Status errorStatus;
	friend class AutoSurface;
};

/**
 * A smart pointer class to ensure Surfaces are set up and deleted correctly.
 */
class AutoSurface {
private:
	std::unique_ptr<Surface> surf;
public:
	AutoSurface(const Editor *ed) :
		surf(ed->CreateMeasurementSurface())  {
	}
	AutoSurface(SurfaceID sid, const Editor *ed, std::optional<Scintilla::Technology> technology = {}) :
		surf(ed->CreateDrawingSurface(sid, technology)) {
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
