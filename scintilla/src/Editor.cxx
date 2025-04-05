// Scintilla source code edit control
/** @file Editor.cxx
 ** Main code for the edit control.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <forward_list>
#include <optional>
#include <algorithm>
#include <iterator>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <future>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "CharacterType.h"
#include "CharacterCategoryMap.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "PerLine.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "UniConversion.h"
#include "DBCS.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "ElapsedPeriod.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

namespace {

/*
	return whether this modification represents an operation that
	may reasonably be deferred (not done now OR [possibly] at all)
*/
constexpr bool CanDeferToLastStep(const DocModification &mh) noexcept {
	if (FlagSet(mh.modificationType, (ModificationFlags::BeforeInsert | ModificationFlags::BeforeDelete)))
		return true;	// CAN skip
	if (!FlagSet(mh.modificationType, (ModificationFlags::Undo | ModificationFlags::Redo)))
		return false;	// MUST do
	if (FlagSet(mh.modificationType, ModificationFlags::MultiStepUndoRedo))
		return true;	// CAN skip
	return false;		// PRESUMABLY must do
}

constexpr bool CanEliminate(const DocModification &mh) noexcept {
	return
		FlagSet(mh.modificationType, (ModificationFlags::BeforeInsert | ModificationFlags::BeforeDelete));
}

/*
	return whether this modification represents the FINAL step
	in a [possibly lengthy] multi-step Undo/Redo sequence
*/
constexpr bool IsLastStep(const DocModification &mh) noexcept {
	constexpr ModificationFlags finalMask = ModificationFlags::MultiStepUndoRedo
		| ModificationFlags::LastStepInUndoRedo
		| ModificationFlags::MultilineUndoRedo;
	return
		FlagSet(mh.modificationType, (ModificationFlags::Undo | ModificationFlags::Redo))
	    && ((mh.modificationType & finalMask) == finalMask);
}

constexpr bool IsAllSpacesOrTabs(std::string_view sv) noexcept {
	for (const char ch : sv) {
		// This is safe because IsSpaceOrTab() will return false for null terminators
		if (!IsSpaceOrTab(ch))
			return false;
	}
	return true;
}

}

Timer::Timer() noexcept :
		ticking(false), ticksToWait(0), tickerID{} {}

Idler::Idler() noexcept :
		state(false), idlerID(nullptr) {}

Editor::Editor() : durationWrapOneByte(0.000001, 0.00000001, 0.00001) {
	ctrlID = 0;

	stylesValid = false;
	technology = Technology::Default;
	scaleRGBAImage = 100.0f;

	cursorMode = CursorShape::Normal;

	errorStatus = Status::Ok;
	mouseDownCaptures = true;
	mouseWheelCaptures = true;

	lastClickTime = 0;
	doubleClickCloseThreshold = Point(3, 3);
	dwellDelay = TimeForever;
	ticksToDwell = TimeForever;
	dwelling = false;
	ptMouseLast.x = 0;
	ptMouseLast.y = 0;
	inDragDrop = DragDrop::none;
	dropWentOutside = false;
	posDrop = SelectionPosition(Sci::invalidPosition);
	hotSpotClickPos = Sci::invalidPosition;
	selectionUnit = TextUnit::character;

	lastXChosen = 0;
	lineAnchorPos = 0;
	originalAnchorPos = 0;
	wordSelectAnchorStartPos = 0;
	wordSelectAnchorEndPos = 0;
	wordSelectInitialCaretPos = -1;

	caretPolicies.x = { CaretPolicy::Slop | CaretPolicy::Even, 50 };
	caretPolicies.y = { CaretPolicy::Even, 0 };

	visiblePolicy = { 0, 0 };

	searchAnchor = 0;

	xCaretMargin = 50;
	horizontalScrollBarVisible = true;
	scrollWidth = 2000;
	verticalScrollBarVisible = true;
	endAtLastLine = true;
	caretSticky = CaretSticky::Off;
	marginOptions = MarginOption::None;
	mouseSelectionRectangularSwitch = false;
	multipleSelection = false;
	additionalSelectionTyping = false;
	multiPasteMode = MultiPaste::Once;
	virtualSpaceOptions = VirtualSpace::None;

	targetRange = SelectionSegment();
	searchFlags = FindOption::None;

	topLine = 0;
	posTopLine = 0;

	lengthForEncode = -1;

	needUpdateUI = Update::None;
	ContainerNeedsUpdate(Update::Content);

	paintState = PaintState::notPainting;
	paintAbandonedByStyling = false;
	paintingAllText = false;
	willRedrawAll = false;
	idleStyling = IdleStyling::None;
	needIdleStyling = false;

	modEventMask = ModificationFlags::EventMaskAll;
	commandEvents = true;

	pdoc->AddWatcher(this, nullptr);

	recordingMacro = false;
	foldAutomatic = AutomaticFold::None;

	convertPastes = true;

	SetRepresentations();
}

Editor::~Editor() {
	pdoc->RemoveWatcher(this, nullptr);
}

void Editor::Finalise() {
	SetIdle(false);
	CancelModes();
}

void Editor::SetRepresentations() {
	reprs->SetDefaultRepresentations(pdoc->dbcsCodePage);
}

void Editor::DropGraphics() noexcept {
	marginView.DropGraphics();
	view.DropGraphics();
}

void Editor::InvalidateStyleData() noexcept {
	stylesValid = false;
	vs.technology = technology;
	DropGraphics();
	view.llc.Invalidate(LineLayout::ValidLevel::invalid);
	view.posCache->Clear();
}

void Editor::InvalidateStyleRedraw() {
	NeedWrapping();
	InvalidateStyleData();
	Redraw();
}

void Editor::RefreshStyleData() {
	if (!stylesValid) {
		stylesValid = true;
		AutoSurface surface(this);
		if (surface) {
			vs.Refresh(*surface, pdoc->tabInChars);
		}
		SetScrollBars();
		SetRectangularRange();
	}
}

bool Editor::HasMarginWindow() const noexcept {
	return wMargin.Created();
}

Point Editor::GetVisibleOriginInMain() const {
	return Point(0, 0);
}

PointDocument Editor::DocumentPointFromView(Point ptView) const {
	PointDocument ptDocument(ptView);
	if (HasMarginWindow()) {
		const Point ptOrigin = GetVisibleOriginInMain();
		ptDocument.x += ptOrigin.x;
		ptDocument.y += ptOrigin.y;
	} else {
		ptDocument.x += xOffset;
		ptDocument.y += topLine * vs.lineHeight;
	}
	return ptDocument;
}

Sci::Line Editor::TopLineOfMain() const noexcept {
	if (HasMarginWindow())
		return 0;
	else
		return topLine;
}

Point Editor::ClientSize() const {
	const PRectangle rcClient = GetClientRectangle();
	return Point(rcClient.Width(), rcClient.Height());
}

PRectangle Editor::GetClientRectangle() const {
	return wMain.GetClientPosition();
}

PRectangle Editor::GetClientDrawingRectangle() {
	return GetClientRectangle();
}

PRectangle Editor::GetTextRectangle() const {
	PRectangle rc = GetClientRectangle();
	rc.left += vs.textStart;
	rc.right -= vs.rightMarginWidth;
	return rc;
}

Sci::Line Editor::LinesOnScreen() const {
	const Point sizeClient = ClientSize();
	const int htClient = static_cast<int>(sizeClient.y);
	//Platform::DebugPrintf("lines on screen = %d\n", htClient / lineHeight + 1);
	return htClient / vs.lineHeight;
}

Sci::Line Editor::LinesToScroll() const {
	const Sci::Line retVal = LinesOnScreen() - 1;
	if (retVal < 1)
		return 1;
	else
		return retVal;
}

Sci::Line Editor::MaxScrollPos() const {
	//Platform::DebugPrintf("Lines %d screen = %d maxScroll = %d\n",
	//LinesTotal(), LinesOnScreen(), LinesTotal() - LinesOnScreen() + 1);
	Sci::Line retVal = pcs->LinesDisplayed();
	if (endAtLastLine) {
		retVal -= LinesOnScreen();
	} else {
		retVal--;
	}
	if (retVal < 0) {
		return 0;
	} else {
		return retVal;
	}
}

SelectionPosition Editor::ClampPositionIntoDocument(SelectionPosition sp) const {
	if (sp.Position() < 0) {
		return SelectionPosition(0);
	} else if (sp.Position() > pdoc->Length()) {
		return SelectionPosition(pdoc->Length());
	} else {
		// If not at end of line then set offset to 0
		if (!pdoc->IsLineEndPosition(sp.Position()))
			sp.SetVirtualSpace(0);
		return sp;
	}
}

Point Editor::LocationFromPosition(SelectionPosition pos, PointEnd pe) {
	const PRectangle rcClient = GetTextRectangle();
	RefreshStyleData();
	AutoSurface surface(this);
	return view.LocationFromPosition(surface, *this, pos, topLine, vs, pe, rcClient);
}

Point Editor::LocationFromPosition(Sci::Position pos, PointEnd pe) {
	return LocationFromPosition(SelectionPosition(pos), pe);
}

int Editor::XFromPosition(SelectionPosition sp) {
	const Point pt = LocationFromPosition(sp);
	return static_cast<int>(pt.x) - vs.textStart + xOffset;
}

SelectionPosition Editor::SPositionFromLocation(Point pt, bool canReturnInvalid, bool charPosition, bool virtualSpace) {
	RefreshStyleData();
	AutoSurface surface(this);

	PRectangle rcClient = GetTextRectangle();
	// May be in scroll view coordinates so translate back to main view
	const Point ptOrigin = GetVisibleOriginInMain();
	rcClient.Move(-ptOrigin.x, -ptOrigin.y);

	if (canReturnInvalid) {
		if (!rcClient.Contains(pt))
			return SelectionPosition(Sci::invalidPosition);
		if (pt.x < vs.textStart)
			return SelectionPosition(Sci::invalidPosition);
		if (pt.y < 0)
			return SelectionPosition(Sci::invalidPosition);
	}
	const PointDocument ptdoc = DocumentPointFromView(pt);
	return view.SPositionFromLocation(surface, *this, ptdoc, canReturnInvalid,
		charPosition, virtualSpace, vs, rcClient);
}

Sci::Position Editor::PositionFromLocation(Point pt, bool canReturnInvalid, bool charPosition) {
	return SPositionFromLocation(pt, canReturnInvalid, charPosition, false).Position();
}

/**
* Find the document position corresponding to an x coordinate on a particular document line.
* Ensure is between whole characters when document is in multi-byte or UTF-8 mode.
* This method is used for rectangular selections and does not work on wrapped lines.
*/
SelectionPosition Editor::SPositionFromLineX(Sci::Line lineDoc, int x) {
	RefreshStyleData();
	if (lineDoc >= pdoc->LinesTotal())
		return SelectionPosition(pdoc->Length());
	//Platform::DebugPrintf("Position of (%d,%d) line = %d top=%d\n", pt.x, pt.y, line, topLine);
	AutoSurface surface(this);
	return view.SPositionFromLineX(surface, *this, lineDoc, x, vs);
}

Sci::Position Editor::PositionFromLineX(Sci::Line lineDoc, int x) {
	return SPositionFromLineX(lineDoc, x).Position();
}

Sci::Line Editor::LineFromLocation(Point pt) const noexcept {
	return pcs->DocFromDisplay(static_cast<int>(pt.y) / vs.lineHeight + topLine);
}

void Editor::SetTopLine(Sci::Line topLineNew) {
	if ((topLine != topLineNew) && (topLineNew >= 0)) {
		topLine = topLineNew;
		ContainerNeedsUpdate(Update::VScroll);
	}
	posTopLine = pdoc->LineStart(pcs->DocFromDisplay(topLine));
}

/**
 * If painting then abandon the painting because a wider redraw is needed.
 * @return true if calling code should stop drawing.
 */
bool Editor::AbandonPaint() {
	if ((paintState == PaintState::painting) && !paintingAllText) {
		paintState = PaintState::abandoned;
	}
	return paintState == PaintState::abandoned;
}

void Editor::RedrawRect(PRectangle rc) {
	//Platform::DebugPrintf("Redraw %0d,%0d - %0d,%0d\n", rc.left, rc.top, rc.right, rc.bottom);

	// Clip the redraw rectangle into the client area
	const PRectangle rcClient = GetClientRectangle();
	if (rc.top < rcClient.top)
		rc.top = rcClient.top;
	if (rc.bottom > rcClient.bottom)
		rc.bottom = rcClient.bottom;
	if (rc.left < rcClient.left)
		rc.left = rcClient.left;
	if (rc.right > rcClient.right)
		rc.right = rcClient.right;

	if ((rc.bottom > rc.top) && (rc.right > rc.left)) {
		wMain.InvalidateRectangle(rc);
	}
}

void Editor::DiscardOverdraw() {
	// Overridden on platforms that may draw outside visible area.
}

void Editor::Redraw() {
	if (redrawPendingText) {
		return;
	}
	//Platform::DebugPrintf("Redraw all\n");
	const PRectangle rcClient = GetClientRectangle();
	wMain.InvalidateRectangle(rcClient);
	if (HasMarginWindow()) {
		wMargin.InvalidateAll();
	} else if (paintState == PaintState::notPainting) {
		redrawPendingText = true;
	}
}

void Editor::RedrawSelMargin(Sci::Line line, bool allAfter) {
	const bool markersInText = vs.maskInLine || vs.maskDrawInText;
	if (!HasMarginWindow() || markersInText) {	// May affect text area so may need to abandon and retry
		if (AbandonPaint()) {
			return;
		}
	}
	if (HasMarginWindow() && markersInText) {
		Redraw();
		return;
	}
	if (redrawPendingMargin) {
		return;
	}
	PRectangle rcMarkers = GetClientRectangle();
	if (!markersInText) {
		// Normal case: just draw the margin
		rcMarkers.right = rcMarkers.left + vs.fixedColumnWidth;
	}
	const PRectangle rcMarkersFull = rcMarkers;
	if (line != -1) {
		PRectangle rcLine = RectangleFromRange(Range(pdoc->LineStart(line)), 0);

		// Inflate line rectangle if there are image markers with height larger than line height
		if (vs.largestMarkerHeight > vs.lineHeight) {
			const int delta = (vs.largestMarkerHeight - vs.lineHeight + 1) / 2;
			rcLine.top -= delta;
			rcLine.bottom += delta;
			if (rcLine.top < rcMarkers.top)
				rcLine.top = rcMarkers.top;
			if (rcLine.bottom > rcMarkers.bottom)
				rcLine.bottom = rcMarkers.bottom;
		}

		rcMarkers.top = rcLine.top;
		if (!allAfter)
			rcMarkers.bottom = rcLine.bottom;
		if (rcMarkers.Empty())
			return;
	}
	if (HasMarginWindow()) {
		const Point ptOrigin = GetVisibleOriginInMain();
		rcMarkers.Move(-ptOrigin.x, -ptOrigin.y);
		wMargin.InvalidateRectangle(rcMarkers);
	} else {
		wMain.InvalidateRectangle(rcMarkers);
		if (rcMarkers == rcMarkersFull) {
			redrawPendingMargin = true;
		}
	}
}

PRectangle Editor::RectangleFromRange(Range r, int overlap) {
	const Sci::Line docLineFirst = pdoc->SciLineFromPosition(r.First());
	const Sci::Line minLine = pcs->DisplayFromDoc(docLineFirst);
	Sci::Line docLineLast = docLineFirst;	// Common case where range is wholly in one document line
	if (r.Last() >= pdoc->LineStart(docLineFirst + 1)) {
		// Range covers multiple lines so need last line
		docLineLast = pdoc->SciLineFromPosition(r.Last());
	}
	const Sci::Line maxLine = pcs->DisplayLastFromDoc(docLineLast);
	const PRectangle rcClientDrawing = GetClientDrawingRectangle();
	PRectangle rc;
	const int leftTextOverlap = ((xOffset == 0) && (vs.leftMarginWidth > 0)) ? 1 : 0;
	rc.left = static_cast<XYPOSITION>(vs.textStart - leftTextOverlap);
	rc.top = static_cast<XYPOSITION>((minLine - TopLineOfMain()) * vs.lineHeight - overlap);
	if (rc.top < rcClientDrawing.top)
		rc.top = rcClientDrawing.top;
	// Extend to right of prepared area if any to prevent artifacts from caret line highlight
	rc.right = rcClientDrawing.right;
	rc.bottom = static_cast<XYPOSITION>((maxLine - TopLineOfMain() + 1) * vs.lineHeight + overlap);

	return rc;
}

void Editor::InvalidateRange(Sci::Position start, Sci::Position end) {
	if (redrawPendingText) {
		return;
	}
	RedrawRect(RectangleFromRange(Range(start, end), view.LinesOverlap() ? vs.lineOverlap : 0));
}

Sci::Position Editor::CurrentPosition() const noexcept {
	return sel.MainCaret();
}

bool Editor::SelectionEmpty() const noexcept {
	return sel.Empty();
}

SelectionPosition Editor::SelectionStart() noexcept {
	return sel.RangeMain().Start();
}

SelectionPosition Editor::SelectionEnd() noexcept {
	return sel.RangeMain().End();
}

void Editor::SetRectangularRange() {
	if (sel.IsRectangular()) {
		const int xAnchor = XFromPosition(sel.Rectangular().anchor);
		int xCaret = XFromPosition(sel.Rectangular().caret);
		if (sel.selType == Selection::SelTypes::thin) {
			xCaret = xAnchor;
		}
		const Sci::Line lineAnchorRect =
			pdoc->SciLineFromPosition(sel.Rectangular().anchor.Position());
		const Sci::Line lineCaret =
			pdoc->SciLineFromPosition(sel.Rectangular().caret.Position());
		const int increment = (lineCaret > lineAnchorRect) ? 1 : -1;
		AutoSurface surface(this);
		for (Sci::Line line=lineAnchorRect; line != lineCaret+increment; line += increment) {
			SelectionRange range(
				view.SPositionFromLineX(surface, *this, line, xCaret, vs),
				view.SPositionFromLineX(surface, *this, line, xAnchor, vs));
			if (!FlagSet(virtualSpaceOptions, VirtualSpace::RectangularSelection))
				range.ClearVirtualSpace();
			if (line == lineAnchorRect)
				sel.SetSelection(range);
			else
				sel.AddSelectionWithoutTrim(range);
		}
	}
}

void Editor::ThinRectangularRange() {
	if (sel.IsRectangular()) {
		sel.selType = Selection::SelTypes::thin;
		if (sel.Rectangular().caret < sel.Rectangular().anchor) {
			sel.Rectangular() = SelectionRange(sel.Range(sel.Count()-1).caret, sel.Range(0).anchor);
		} else {
			sel.Rectangular() = SelectionRange(sel.Range(sel.Count()-1).anchor, sel.Range(0).caret);
		}
		SetRectangularRange();
	}
}

void Editor::InvalidateSelection(SelectionRange newMain, bool invalidateWholeSelection) {
	if (sel.Count() > 1 || !(sel.RangeMain().anchor == newMain.anchor) || sel.IsRectangular()) {
		invalidateWholeSelection = true;
	}
	Sci::Position firstAffected = std::min(sel.RangeMain().Start().Position(), newMain.Start().Position());
	// +1 for lastAffected ensures caret repainted
	Sci::Position lastAffected = std::max(newMain.caret.Position()+1, newMain.anchor.Position());
	lastAffected = std::max(lastAffected, sel.RangeMain().End().Position());
	if (invalidateWholeSelection) {
		for (size_t r=0; r<sel.Count(); r++) {
			firstAffected = std::min(firstAffected, sel.Range(r).caret.Position());
			firstAffected = std::min(firstAffected, sel.Range(r).anchor.Position());
			lastAffected = std::max(lastAffected, sel.Range(r).caret.Position()+1);
			lastAffected = std::max(lastAffected, sel.Range(r).anchor.Position());
		}
	}
	ContainerNeedsUpdate(Update::Selection);
	InvalidateRange(firstAffected, lastAffected);
}

void Editor::InvalidateWholeSelection() {
	InvalidateSelection(sel.RangeMain(), true);
}

/* For Line selection - the anchor and caret are always
   at the beginning and end of the region lines. */
SelectionRange Editor::LineSelectionRange(SelectionPosition currentPos_, SelectionPosition anchor_) const {
	if (currentPos_ > anchor_) {
		anchor_ = SelectionPosition(pdoc->LineStartPosition(anchor_.Position()));
		currentPos_ = SelectionPosition(pdoc->LineEndPosition(currentPos_.Position()));
	} else {
		currentPos_ = SelectionPosition(pdoc->LineStartPosition(currentPos_.Position()));
		anchor_ = SelectionPosition(pdoc->LineEndPosition(anchor_.Position()));
	}
	return SelectionRange(currentPos_, anchor_);
}

void Editor::SetSelection(SelectionPosition currentPos_, SelectionPosition anchor_) {
	currentPos_ = ClampPositionIntoDocument(currentPos_);
	anchor_ = ClampPositionIntoDocument(anchor_);
	const Sci::Line currentLine = pdoc->SciLineFromPosition(currentPos_.Position());
	SelectionRange rangeNew(currentPos_, anchor_);
	if (sel.selType == Selection::SelTypes::lines) {
		rangeNew = LineSelectionRange(currentPos_, anchor_);
	}
	if (sel.Count() > 1 || !(sel.RangeMain() == rangeNew)) {
		InvalidateSelection(rangeNew);
	}
	sel.RangeMain() = rangeNew;
	SetRectangularRange();
	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());

	if (marginView.highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkItems::updateUI);
}

void Editor::SetSelection(Sci::Position currentPos_, Sci::Position anchor_) {
	SetSelection(SelectionPosition(currentPos_), SelectionPosition(anchor_));
}

// Just move the caret on the main selection
void Editor::SetSelection(SelectionPosition currentPos_) {
	currentPos_ = ClampPositionIntoDocument(currentPos_);
	const Sci::Line currentLine = pdoc->SciLineFromPosition(currentPos_.Position());
	if (sel.Count() > 1 || !(sel.RangeMain().caret == currentPos_)) {
		InvalidateSelection(SelectionRange(currentPos_));
	}
	if (sel.IsRectangular()) {
		sel.Rectangular() =
			SelectionRange(SelectionPosition(currentPos_), sel.Rectangular().anchor);
		SetRectangularRange();
	} else if (sel.selType == Selection::SelTypes::lines) {
		sel.RangeMain() = LineSelectionRange(currentPos_, sel.RangeMain().anchor);
	} else {
		sel.RangeMain() =
			SelectionRange(SelectionPosition(currentPos_), sel.RangeMain().anchor);
	}
	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());

	if (marginView.highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkItems::updateUI);
}

void Editor::SetEmptySelection(SelectionPosition currentPos_) {
	const Sci::Line currentLine = pdoc->SciLineFromPosition(currentPos_.Position());
	SelectionRange rangeNew(ClampPositionIntoDocument(currentPos_));
	if (sel.Count() > 1 || !(sel.RangeMain() == rangeNew)) {
		InvalidateSelection(rangeNew);
	}
	sel.Clear();
	sel.RangeMain() = rangeNew;
	SetRectangularRange();
	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());

	if (marginView.highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkItems::updateUI);
}

void Editor::SetEmptySelection(Sci::Position currentPos_) {
	SetEmptySelection(SelectionPosition(currentPos_));
}

void Editor::SetSelectionFromSerialized(const char *serialized) {
	if (serialized) {
		sel = Selection(serialized);
		sel.Truncate(pdoc->Length());
		SetRectangularRange();
		InvalidateStyleRedraw();
	}
}

void Editor::MultipleSelectAdd(AddNumber addNumber) {
	if (SelectionEmpty() || !multipleSelection) {
		// Select word at caret
		const Sci::Position startWord = pdoc->ExtendWordSelect(sel.MainCaret(), -1, true);
		const Sci::Position endWord = pdoc->ExtendWordSelect(startWord, 1, true);
		TrimAndSetSelection(endWord, startWord);

	} else {

		if (!pdoc->HasCaseFolder())
			pdoc->SetCaseFolder(CaseFolderForEncoding());

		const Range rangeMainSelection(sel.RangeMain().Start().Position(), sel.RangeMain().End().Position());
		const std::string selectedText = RangeText(rangeMainSelection.start, rangeMainSelection.end);

		const Range rangeTarget(targetRange.start.Position(), targetRange.end.Position());
		std::vector<Range> searchRanges;
		// Search should be over the target range excluding the current selection so
		// may need to search 2 ranges, after the selection then before the selection.
		if (rangeTarget.Overlaps(rangeMainSelection)) {
			// Common case is that the selection is completely within the target but
			// may also have overlap at start or end.
			if (rangeMainSelection.end < rangeTarget.end)
				searchRanges.push_back(Range(rangeMainSelection.end, rangeTarget.end));
			if (rangeTarget.start < rangeMainSelection.start)
				searchRanges.push_back(Range(rangeTarget.start, rangeMainSelection.start));
		} else {
			// No overlap
			searchRanges.push_back(rangeTarget);
		}

		for (const Range range : searchRanges) {
			Sci::Position searchStart = range.start;
			const Sci::Position searchEnd = range.end;
			for (;;) {
				Sci::Position lengthFound = selectedText.length();
				const Sci::Position pos = pdoc->FindText(searchStart, searchEnd,
					selectedText.c_str(), searchFlags, &lengthFound);
				if (pos >= 0) {
					sel.AddSelection(SelectionRange(pos + lengthFound, pos));
					ContainerNeedsUpdate(Update::Selection);
					ScrollRange(sel.RangeMain());
					Redraw();
					if (addNumber == AddNumber::one)
						return;
					searchStart = pos + lengthFound;
				} else {
					break;
				}
			}
		}
	}
}

bool Editor::RangeContainsProtected(Sci::Position start, Sci::Position end) const noexcept {
	if (vs.ProtectionActive()) {
		if (start > end) {
			std::swap(start, end);
		}
		for (Sci::Position pos = start; pos < end; pos++) {
			if (vs.styles[pdoc->StyleIndexAt(pos)].IsProtected())
				return true;
		}
	}
	return false;
}

bool Editor::RangeContainsProtected(const SelectionRange &range) const noexcept {
	return RangeContainsProtected(range.Start().Position(), range.End().Position());
}

bool Editor::SelectionContainsProtected() const noexcept {
	for (size_t r=0; r<sel.Count(); r++) {
		if (RangeContainsProtected(sel.Range(r))) {
			return true;
		}
	}
	return false;
}

/**
 * Asks document to find a good position and then moves out of any invisible positions.
 */
Sci::Position Editor::MovePositionOutsideChar(Sci::Position pos, Sci::Position moveDir, bool checkLineEnd) const {
	return MovePositionOutsideChar(SelectionPosition(pos), moveDir, checkLineEnd).Position();
}

SelectionPosition Editor::MovePositionOutsideChar(SelectionPosition pos, Sci::Position moveDir, bool checkLineEnd) const {
	const Sci::Position posMoved = pdoc->MovePositionOutsideChar(pos.Position(), moveDir, checkLineEnd);
	if (posMoved != pos.Position())
		pos.SetPosition(posMoved);
	if (vs.ProtectionActive()) {
		if (moveDir > 0) {
			if ((pos.Position() > 0) && vs.styles[pdoc->StyleIndexAt(pos.Position() - 1)].IsProtected()) {
				while ((pos.Position() < pdoc->Length()) &&
				        (vs.styles[pdoc->StyleIndexAt(pos.Position())].IsProtected()))
					pos.Add(1);
			}
		} else if (moveDir < 0) {
			if (vs.styles[pdoc->StyleIndexAt(pos.Position())].IsProtected()) {
				while ((pos.Position() > 0) &&
				        (vs.styles[pdoc->StyleIndexAt(pos.Position() - 1)].IsProtected()))
					pos.Add(-1);
			}
		}
	}
	return pos;
}

void Editor::MovedCaret(SelectionPosition newPos, SelectionPosition previousPos,
	bool ensureVisible, CaretPolicies policies) {
	const Sci::Line currentLine = pdoc->SciLineFromPosition(newPos.Position());
	if (ensureVisible) {
		// In case in need of wrapping to ensure DisplayFromDoc works.
		if (currentLine >= wrapPending.start) {
			if (WrapLines(WrapScope::wsAll)) {
				Redraw();
			}
		}
		const XYScrollPosition newXY = XYScrollToMakeVisible(
			SelectionRange(posDrag.IsValid() ? posDrag : newPos), XYScrollOptions::all, policies);
		if (previousPos.IsValid() && (newXY.xOffset == xOffset)) {
			// simple vertical scroll then invalidate
			ScrollTo(newXY.topLine);
			InvalidateSelection(SelectionRange(previousPos), true);
		} else {
			SetXYScroll(newXY);
		}
	}

	ShowCaretAtCurrentPosition();
	NotifyCaretMove();

	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());
	QueueIdleWork(WorkItems::updateUI);

	if (marginView.highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
}

void Editor::MovePositionTo(SelectionPosition newPos, Selection::SelTypes selt, bool ensureVisible) {
	const SelectionPosition spCaret = ((sel.Count() == 1) && sel.Empty()) ?
		sel.Last() : SelectionPosition(Sci::invalidPosition);

	const Sci::Position delta = newPos.Position() - sel.MainCaret();
	newPos = ClampPositionIntoDocument(newPos);
	newPos = MovePositionOutsideChar(newPos, delta);
	if (!multipleSelection && sel.IsRectangular() && (selt == Selection::SelTypes::stream)) {
		// Can't turn into multiple selection so clear additional selections
		InvalidateSelection(SelectionRange(newPos), true);
		sel.DropAdditionalRanges();
	}
	if (!sel.IsRectangular() && (selt == Selection::SelTypes::rectangle)) {
		// Switching to rectangular
		InvalidateSelection(sel.RangeMain(), false);
		SelectionRange rangeMain = sel.RangeMain();
		sel.Clear();
		sel.Rectangular() = rangeMain;
	}
	if (selt != Selection::SelTypes::none) {
		sel.selType = selt;
	}
	if (selt != Selection::SelTypes::none || sel.MoveExtends()) {
		SetSelection(newPos);
	} else {
		SetEmptySelection(newPos);
	}

	MovedCaret(newPos, spCaret, ensureVisible, caretPolicies);
}

void Editor::MovePositionTo(Sci::Position newPos, Selection::SelTypes selt, bool ensureVisible) {
	MovePositionTo(SelectionPosition(newPos), selt, ensureVisible);
}

SelectionPosition Editor::MovePositionSoVisible(SelectionPosition pos, int moveDir) {
	pos = ClampPositionIntoDocument(pos);
	pos = MovePositionOutsideChar(pos, moveDir);
	const Sci::Line lineDoc = pdoc->SciLineFromPosition(pos.Position());
	if (pcs->GetVisible(lineDoc)) {
		return pos;
	} else {
		Sci::Line lineDisplay = pcs->DisplayFromDoc(lineDoc);
		if (moveDir > 0) {
			// lineDisplay is already line before fold as lines in fold use display line of line after fold
			lineDisplay = std::clamp<Sci::Line>(lineDisplay, 0, pcs->LinesDisplayed());
			return SelectionPosition(
				pdoc->LineStart(pcs->DocFromDisplay(lineDisplay)));
		} else {
			lineDisplay = std::clamp<Sci::Line>(lineDisplay - 1, 0, pcs->LinesDisplayed());
			return SelectionPosition(
				pdoc->LineEnd(pcs->DocFromDisplay(lineDisplay)));
		}
	}
}

SelectionPosition Editor::MovePositionSoVisible(Sci::Position pos, int moveDir) {
	return MovePositionSoVisible(SelectionPosition(pos), moveDir);
}

Point Editor::PointMainCaret() {
	return LocationFromPosition(sel.RangeMain().caret);
}

/**
 * Choose the x position that the caret will try to stick to
 * as it moves up and down.
 */
void Editor::SetLastXChosen() {
	const Point pt = PointMainCaret();
	lastXChosen = static_cast<int>(pt.x) + xOffset;
}

void Editor::RememberSelectionForUndo(int index) {
	EnsureModelState();
	if (modelState) {
		modelState->RememberSelectionForUndo(index, sel);
		needRedoRemembered = true;
		// Remember selection at end of processing current message
	}
}

void Editor::RememberSelectionOntoStack(int index) {
	EnsureModelState();
	if (modelState) {
		// Is undo currently inside a group?
		if (!pdoc->AfterUndoSequenceStart()) {
			// Don't remember selections inside a grouped sequence as can only
			// unto or redo to the start and end of the group.
			modelState->RememberSelectionOntoStack(index, topLine);
		}
	}
}

void Editor::RememberCurrentSelectionForRedoOntoStack() {
	if (needRedoRemembered && (pdoc->UndoSequenceDepth() == 0)) {
		EnsureModelState();
		if (modelState) {
			modelState->RememberSelectionForRedoOntoStack(pdoc->UndoCurrent(), sel, topLine);
			needRedoRemembered = false;
		}
	}
}

void Editor::ScrollTo(Sci::Line line, bool moveThumb) {
	const Sci::Line topLineNew = std::clamp<Sci::Line>(line, 0, MaxScrollPos());
	if (topLineNew != topLine) {
		// Try to optimise small scrolls
#ifndef UNDER_CE
		const Sci::Line linesToMove = topLine - topLineNew;
		const bool performBlit = (std::abs(linesToMove) <= 10) && (paintState == PaintState::notPainting);
		willRedrawAll = !performBlit;
#endif
		SetTopLine(topLineNew);
		// Optimize by styling the view as this will invalidate any needed area
		// which could abort the initial paint if discovered later.
		StyleAreaBounded(GetClientRectangle(), true);
#ifndef UNDER_CE
		// Perform redraw rather than scroll if many lines would be redrawn anyway.
		if (performBlit) {
			ScrollText(linesToMove);
		} else {
			Redraw();
		}
		willRedrawAll = false;
#else
		Redraw();
#endif
		if (moveThumb) {
			SetVerticalScrollPos();
		}
	}
}

void Editor::ScrollText(Sci::Line /* linesToMove */) {
	//Platform::DebugPrintf("Editor::ScrollText %d\n", linesToMove);
	Redraw();
}

void Editor::HorizontalScrollTo(int xPos) {
	//Platform::DebugPrintf("HorizontalScroll %d\n", xPos);
	if (xPos < 0)
		xPos = 0;
	if (!Wrapping() && (xOffset != xPos)) {
		xOffset = xPos;
		ContainerNeedsUpdate(Update::HScroll);
		SetHorizontalScrollPos();
		RedrawRect(GetClientRectangle());
	}
}

void Editor::VerticalCentreCaret() {
	const Sci::Line lineDoc =
		pdoc->SciLineFromPosition(sel.IsRectangular() ? sel.Rectangular().caret.Position() : sel.MainCaret());
	const Sci::Line lineDisplay = pcs->DisplayFromDoc(lineDoc);
	const Sci::Line newTop = lineDisplay - (LinesOnScreen() / 2);
	if (topLine != newTop) {
		SetTopLine(newTop > 0 ? newTop : 0);
		SetVerticalScrollPos();
		RedrawRect(GetClientRectangle());
	}
}

void Editor::MoveSelectedLines(int lineDelta) {

	if (sel.IsRectangular()) {
		// Convert to stream selection
		const SelectionRange rangeRectangular = sel.Rectangular();
		sel.Clear();
		sel.SetSelection(rangeRectangular);
	}

	// if selection doesn't start at the beginning of the line, set the new start
	Sci::Position selectionStart = SelectionStart().Position();
	const Sci::Line startLine = pdoc->SciLineFromPosition(selectionStart);
	const Sci::Position beginningOfStartLine = pdoc->LineStart(startLine);
	selectionStart = beginningOfStartLine;

	// if selection doesn't end at the beginning of a line greater than that of the start,
	// then set it at the beginning of the next one
	Sci::Position selectionEnd = SelectionEnd().Position();
	Sci::Line endLine = pdoc->SciLineFromPosition(selectionEnd);
	const Sci::Position beginningOfEndLine = pdoc->LineStart(endLine);
	bool appendEol = false;
	if (selectionEnd > beginningOfEndLine
		|| selectionStart == selectionEnd) {
		selectionEnd = pdoc->LineStart(endLine + 1);
		appendEol = (selectionEnd == pdoc->Length() && pdoc->SciLineFromPosition(selectionEnd) == endLine);
		endLine = pdoc->SciLineFromPosition(selectionEnd);
	}

	// if there's nowhere for the selection to move
	// (i.e. at the beginning going up or at the end going down),
	// stop it right there!
	const bool docEndLineEmpty = pdoc->LineStart(endLine) == pdoc->Length();
	if ((selectionStart == 0 && lineDelta < 0)
		|| (selectionEnd == pdoc->Length() && lineDelta > 0
			&& !docEndLineEmpty) // allow moving when end line of document is empty
		|| ((selectionStart == selectionEnd)
			&& !(lineDelta < 0 && docEndLineEmpty && selectionEnd == pdoc->Length()))) { // allow moving-up last empty line
		return;
	}

	UndoGroup ug(pdoc);

	if (lineDelta > 0 && selectionEnd == pdoc->LineStart(pdoc->LinesTotal() - 1)) {
		SetSelection(pdoc->MovePositionOutsideChar(selectionEnd - 1, -1), selectionEnd);
		ClearSelection();
		selectionEnd = CurrentPosition();
	}
	SetSelection(selectionStart, selectionEnd);

	const std::string selectedText = RangeText(selectionStart, selectionEnd);

	const Point currentLocation = LocationFromPosition(CurrentPosition());
	const Sci::Line currentLine = LineFromLocation(currentLocation);

	if (appendEol)
		SetSelection(pdoc->MovePositionOutsideChar(selectionStart - 1, -1), selectionEnd);
	ClearSelection();

	const std::string_view eol = pdoc->EOLString();
	if (currentLine + lineDelta >= pdoc->LinesTotal())
		pdoc->InsertString(pdoc->Length(), eol);
	GoToLine(currentLine + lineDelta);

	Sci::Position selectionLength = pdoc->InsertString(CurrentPosition(), selectedText);
	if (appendEol) {
		const Sci::Position lengthInserted = pdoc->InsertString(CurrentPosition() + selectionLength, eol);
		selectionLength += lengthInserted;
	}
	SetSelection(CurrentPosition(), CurrentPosition() + selectionLength);
}

void Editor::MoveSelectedLinesUp() {
	MoveSelectedLines(-1);
}

void Editor::MoveSelectedLinesDown() {
	MoveSelectedLines(1);
}

void Editor::MoveCaretInsideView(bool ensureVisible) {
	const PRectangle rcClient = GetTextRectangle();
	const Point pt = PointMainCaret();
	if (pt.y < rcClient.top) {
		MovePositionTo(SPositionFromLocation(
		            Point::FromInts(lastXChosen - xOffset, static_cast<int>(rcClient.top)),
					false, false, UserVirtualSpace()),
					Selection::SelTypes::none, ensureVisible);
	} else if ((pt.y + vs.lineHeight - 1) > rcClient.bottom) {
		const ptrdiff_t yOfLastLineFullyDisplayed = static_cast<ptrdiff_t>(rcClient.top) + (LinesOnScreen() - 1) * vs.lineHeight;
		MovePositionTo(SPositionFromLocation(
		            Point::FromInts(lastXChosen - xOffset, static_cast<int>(rcClient.top + yOfLastLineFullyDisplayed)),
					false, false, UserVirtualSpace()),
		        Selection::SelTypes::none, ensureVisible);
	}
}

Sci::Line Editor::DisplayFromPosition(Sci::Position pos) {
	AutoSurface surface(this);
	return view.DisplayFromPosition(surface, *this, pos, vs);
}

/**
 * Ensure the caret is reasonably visible in context.
 *
Caret policy in Scintilla

If slop is set, we can define a slop value.
This value defines an unwanted zone (UZ) where the caret is... unwanted.
This zone is defined as a number of pixels near the vertical margins,
and as a number of lines near the horizontal margins.
By keeping the caret away from the edges, it is seen within its context,
so it is likely that the identifier that the caret is on can be completely seen,
and that the current line is seen with some of the lines following it which are
often dependent on that line.

If strict is set, the policy is enforced... strictly.
The caret is centred on the display if slop is not set,
and cannot go in the UZ if slop is set.

If jumps is set, the display is moved more energetically
so the caret can move in the same direction longer before the policy is applied again.
'3UZ' notation is used to indicate three time the size of the UZ as a distance to the margin.

If even is not set, instead of having symmetrical UZs,
the left and bottom UZs are extended up to right and top UZs respectively.
This way, we favour the displaying of useful information: the beginning of lines,
where most code reside, and the lines after the caret, eg. the body of a function.

     |        |       |      |                                            |
slop | strict | jumps | even | Caret can go to the margin                 | When reaching limit (caret going out of
     |        |       |      |                                            | visibility or going into the UZ) display is...
-----+--------+-------+------+--------------------------------------------+--------------------------------------------------------------
  0  |   0    |   0   |   0  | Yes                                        | moved to put caret on top/on right
  0  |   0    |   0   |   1  | Yes                                        | moved by one position
  0  |   0    |   1   |   0  | Yes                                        | moved to put caret on top/on right
  0  |   0    |   1   |   1  | Yes                                        | centred on the caret
  0  |   1    |   -   |   0  | Caret is always on top/on right of display | -
  0  |   1    |   -   |   1  | No, caret is always centred                | -
  1  |   0    |   0   |   0  | Yes                                        | moved to put caret out of the asymmetrical UZ
  1  |   0    |   0   |   1  | Yes                                        | moved to put caret out of the UZ
  1  |   0    |   1   |   0  | Yes                                        | moved to put caret at 3UZ of the top or right margin
  1  |   0    |   1   |   1  | Yes                                        | moved to put caret at 3UZ of the margin
  1  |   1    |   -   |   0  | Caret is always at UZ of top/right margin  | -
  1  |   1    |   0   |   1  | No, kept out of UZ                         | moved by one position
  1  |   1    |   1   |   1  | No, kept out of UZ                         | moved to put caret at 3UZ of the margin
*/

Editor::XYScrollPosition Editor::XYScrollToMakeVisible(const SelectionRange &range,
	const XYScrollOptions options, CaretPolicies policies) {
	const PRectangle rcClient = GetTextRectangle();
	const Point ptOrigin = GetVisibleOriginInMain();
	const Point pt = LocationFromPosition(range.caret) + ptOrigin;
	const Point ptAnchor = LocationFromPosition(range.anchor) + ptOrigin;
	const Point ptBottomCaret(pt.x, pt.y + vs.lineHeight - 1);

	XYScrollPosition newXY(xOffset, topLine);
	if (rcClient.Empty()) {
		return newXY;
	}

	// Vertical positioning
	if (FlagSet(options, XYScrollOptions::vertical) &&
		(pt.y < rcClient.top || ptBottomCaret.y >= rcClient.bottom || FlagSet(policies.y.policy, CaretPolicy::Strict))) {
		const Sci::Line lineCaret = DisplayFromPosition(range.caret.Position());
		const Sci::Line linesOnScreen = LinesOnScreen();
		const Sci::Line halfScreen = std::max(linesOnScreen - 1, static_cast<Sci::Line>(2)) / 2;
		const bool bSlop = FlagSet(policies.y.policy, CaretPolicy::Slop);
		const bool bStrict = FlagSet(policies.y.policy, CaretPolicy::Strict);
		const bool bJump = FlagSet(policies.y.policy, CaretPolicy::Jumps);
		const bool bEven = FlagSet(policies.y.policy, CaretPolicy::Even);

		// It should be possible to scroll the window to show the caret,
		// but this fails to remove the caret on GTK+
		if (bSlop) {	// A margin is defined
			Sci::Line yMoveT = 0;
			Sci::Line yMoveB = 0;
			if (bStrict) {
				Sci::Line yMarginT = 0;
				Sci::Line yMarginB = 0;
				if (!FlagSet(options, XYScrollOptions::useMargin)) {
					// In drag mode, avoid moves
					// otherwise, a double click will select several lines.
					yMarginT = yMarginB = 0;
				} else {
					// yMarginT must equal to caretYSlop, with a minimum of 1 and
					// a maximum of slightly less than half the height of the text area.
					yMarginT = std::clamp<Sci::Line>(policies.y.slop, 1, halfScreen);
					if (bEven) {
						yMarginB = yMarginT;
					} else {
						yMarginB = linesOnScreen - yMarginT - 1;
					}
				}
				yMoveT = yMarginT;
				if (bEven) {
					if (bJump) {
						yMoveT = std::clamp<Sci::Line>(policies.y.slop * 3, 1, halfScreen);
					}
					yMoveB = yMoveT;
				} else {
					yMoveB = linesOnScreen - yMoveT - 1;
				}
				if (lineCaret < topLine + yMarginT) {
					// Caret goes too high
					newXY.topLine = lineCaret - yMoveT;
				} else if (lineCaret > topLine + linesOnScreen - 1 - yMarginB) {
					// Caret goes too low
					newXY.topLine = lineCaret - linesOnScreen + 1 + yMoveB;
				}
			} else {	// Not strict
				yMoveT = bJump ? policies.y.slop * 3 : policies.y.slop;
				yMoveT = std::clamp<Sci::Line>(yMoveT, 1, halfScreen);
				if (bEven) {
					yMoveB = yMoveT;
				} else {
					yMoveB = linesOnScreen - yMoveT - 1;
				}
				if (lineCaret < topLine) {
					// Caret goes too high
					newXY.topLine = lineCaret - yMoveT;
				} else if (lineCaret > topLine + linesOnScreen - 1) {
					// Caret goes too low
					newXY.topLine = lineCaret - linesOnScreen + 1 + yMoveB;
				}
			}
		} else {	// No slop
			if (!bStrict && !bJump) {
				// Minimal move
				if (lineCaret < topLine) {
					// Caret goes too high
					newXY.topLine = lineCaret;
				} else if (lineCaret > topLine + linesOnScreen - 1) {
					// Caret goes too low
					if (bEven) {
						newXY.topLine = lineCaret - linesOnScreen + 1;
					} else {
						newXY.topLine = lineCaret;
					}
				}
			} else {	// Strict or going out of display
				if (bEven) {
					// Always centre caret
					newXY.topLine = lineCaret - halfScreen;
				} else {
					// Always put caret on top of display
					newXY.topLine = lineCaret;
				}
			}
		}
		if (!(range.caret == range.anchor)) {
			const Sci::Line lineAnchor = DisplayFromPosition(range.anchor.Position());
			if (lineAnchor < lineCaret) {
				// Shift up to show anchor or as much of range as possible
				newXY.topLine = std::min(newXY.topLine, lineAnchor);
				newXY.topLine = std::max(newXY.topLine, lineCaret - LinesOnScreen());
			} else {
				// Shift down to show anchor or as much of range as possible
				newXY.topLine = std::max(newXY.topLine, lineAnchor - LinesOnScreen());
				newXY.topLine = std::min(newXY.topLine, lineCaret);
			}
		}
		newXY.topLine = std::clamp<Sci::Line>(newXY.topLine, 0, MaxScrollPos());
	}

	// Horizontal positioning
	if (FlagSet(options, XYScrollOptions::horizontal) && !Wrapping()) {
		const int halfScreen = std::max(static_cast<int>(rcClient.Width()) - 4, 4) / 2;
		const bool bSlop = FlagSet(policies.x.policy, CaretPolicy::Slop);
		const bool bStrict = FlagSet(policies.x.policy, CaretPolicy::Strict);
		const bool bJump = FlagSet(policies.x.policy, CaretPolicy::Jumps);
		const bool bEven = FlagSet(policies.x.policy, CaretPolicy::Even);

		if (bSlop) {	// A margin is defined
			int xMoveL = 0;
			int xMoveR = 0;
			if (bStrict) {
				int xMarginL = 0;
				int xMarginR = 0;
				if (!FlagSet(options, XYScrollOptions::useMargin)) {
					// In drag mode, avoid moves unless very near of the margin
					// otherwise, a simple click will select text.
					xMarginL = xMarginR = 2;
				} else {
					// xMargin must equal to caretXSlop, with a minimum of 2 and
					// a maximum of slightly less than half the width of the text area.
					xMarginR = std::clamp(policies.x.slop, 2, halfScreen);
					if (bEven) {
						xMarginL = xMarginR;
					} else {
						xMarginL = static_cast<int>(rcClient.Width()) - xMarginR - 4;
					}
				}
				if (bJump && bEven) {
					// Jump is used only in even mode
					xMoveL = xMoveR = std::clamp(policies.x.slop * 3, 1, halfScreen);
				} else {
					xMoveL = xMoveR = 0;	// Not used, avoid a warning
				}
				if (pt.x < rcClient.left + xMarginL) {
					// Caret is on the left of the display
					if (bJump && bEven) {
						newXY.xOffset -= xMoveL;
					} else {
						// Move just enough to allow to display the caret
						newXY.xOffset -= static_cast<int>((rcClient.left + xMarginL) - pt.x);
					}
				} else if (pt.x >= rcClient.right - xMarginR) {
					// Caret is on the right of the display
					if (bJump && bEven) {
						newXY.xOffset += xMoveR;
					} else {
						// Move just enough to allow to display the caret
						newXY.xOffset += static_cast<int>(pt.x - (rcClient.right - xMarginR) + 1);
					}
				}
			} else {	// Not strict
				xMoveR = bJump ? policies.x.slop * 3 : policies.x.slop;
				xMoveR = std::clamp(xMoveR, 1, halfScreen);
				if (bEven) {
					xMoveL = xMoveR;
				} else {
					xMoveL = static_cast<int>(rcClient.Width()) - xMoveR - 4;
				}
				if (pt.x < rcClient.left) {
					// Caret is on the left of the display
					newXY.xOffset -= xMoveL;
				} else if (pt.x >= rcClient.right) {
					// Caret is on the right of the display
					newXY.xOffset += xMoveR;
				}
			}
		} else {	// No slop
			if (bStrict ||
			        (bJump && (pt.x < rcClient.left || pt.x >= rcClient.right))) {
				// Strict or going out of display
				if (bEven) {
					// Centre caret
					newXY.xOffset += static_cast<int>(pt.x - rcClient.left - halfScreen);
				} else {
					// Put caret on right
					newXY.xOffset += static_cast<int>(pt.x - rcClient.right + 1);
				}
			} else {
				// Move just enough to allow to display the caret
				if (pt.x < rcClient.left) {
					// Caret is on the left of the display
					if (bEven) {
						newXY.xOffset -= static_cast<int>(rcClient.left - pt.x);
					} else {
						newXY.xOffset += static_cast<int>(pt.x - rcClient.right) + 1;
					}
				} else if (pt.x >= rcClient.right) {
					// Caret is on the right of the display
					newXY.xOffset += static_cast<int>(pt.x - rcClient.right) + 1;
				}
			}
		}
		// In case of a jump (find result) largely out of display, adjust the offset to display the caret
		if (pt.x + xOffset < rcClient.left + newXY.xOffset) {
			newXY.xOffset = static_cast<int>(pt.x + xOffset - rcClient.left) - 2;
		} else if (pt.x + xOffset >= rcClient.right + newXY.xOffset) {
			newXY.xOffset = static_cast<int>(pt.x + xOffset - rcClient.right) + 2;
			if (vs.IsBlockCaretStyle() || view.imeCaretBlockOverride) {
				// Ensure we can see a good portion of the block caret
				newXY.xOffset += static_cast<int>(vs.aveCharWidth);
			}
		}
		if (!(range.caret == range.anchor)) {
			if (ptAnchor.x < pt.x) {
				// Shift to left to show anchor or as much of range as possible
				const int maxOffset = static_cast<int>(ptAnchor.x + xOffset - rcClient.left) - 1;
				const int minOffset = static_cast<int>(pt.x + xOffset - rcClient.right) + 1;
				newXY.xOffset = std::min(newXY.xOffset, maxOffset);
				newXY.xOffset = std::max(newXY.xOffset, minOffset);
			} else {
				// Shift to right to show anchor or as much of range as possible
				const int minOffset = static_cast<int>(ptAnchor.x + xOffset - rcClient.right) + 1;
				const int maxOffset = static_cast<int>(pt.x + xOffset - rcClient.left) - 1;
				newXY.xOffset = std::max(newXY.xOffset, minOffset);
				newXY.xOffset = std::min(newXY.xOffset, maxOffset);
			}
		}
		if (newXY.xOffset < 0) {
			newXY.xOffset = 0;
		}
	}

	return newXY;
}

void Editor::SetXYScroll(XYScrollPosition newXY) {
	if ((newXY.topLine != topLine) || (newXY.xOffset != xOffset)) {
		if (newXY.topLine != topLine) {
			SetTopLine(newXY.topLine);
			SetVerticalScrollPos();
		}
		if (newXY.xOffset != xOffset) {
			xOffset = newXY.xOffset;
			ContainerNeedsUpdate(Update::HScroll);
			if (newXY.xOffset > 0) {
				const PRectangle rcText = GetTextRectangle();
				if (horizontalScrollBarVisible &&
					rcText.Width() + xOffset > scrollWidth) {
					scrollWidth = xOffset + static_cast<int>(rcText.Width());
					SetScrollBars();
				}
			}
			SetHorizontalScrollPos();
		}
		Redraw();
		UpdateSystemCaret();
	}
}

void Editor::ScrollRange(SelectionRange range) {
	SetXYScroll(XYScrollToMakeVisible(range, XYScrollOptions::all, caretPolicies));
}

void Editor::EnsureCaretVisible(bool useMargin, bool vert, bool horiz) {
	SetXYScroll(XYScrollToMakeVisible(SelectionRange(posDrag.IsValid() ? posDrag : sel.RangeMain().caret),
		(useMargin?XYScrollOptions::useMargin:XYScrollOptions::none)|
		(vert?XYScrollOptions::vertical:XYScrollOptions::none)|
		(horiz?XYScrollOptions::horizontal:XYScrollOptions::none),
		caretPolicies));
}

void Editor::ShowCaretAtCurrentPosition() {
	if (hasFocus) {
		caret.active = true;
		caret.on = true;
		FineTickerCancel(TickReason::caret);
		if (caret.period > 0)
			FineTickerStart(TickReason::caret, caret.period, caret.period/10);
	} else {
		caret.active = false;
		caret.on = false;
		FineTickerCancel(TickReason::caret);
	}
	InvalidateCaret();
}

void Editor::DropCaret() {
	caret.active = false;
	FineTickerCancel(TickReason::caret);
	InvalidateCaret();
}

void Editor::CaretSetPeriod(int period) {
	if (caret.period != period) {
		caret.period = period;
		caret.on = true;
		FineTickerCancel(TickReason::caret);
		if ((caret.active) && (caret.period > 0))
			FineTickerStart(TickReason::caret, caret.period, caret.period/10);
		InvalidateCaret();
	}
}

void Editor::InvalidateCaret() {
	if (posDrag.IsValid()) {
		InvalidateRange(posDrag.Position(), posDrag.Position() + 1);
	} else {
		for (size_t r=0; r<sel.Count(); r++) {
			InvalidateRange(sel.Range(r).caret.Position(), sel.Range(r).caret.Position() + 1);
		}
	}
	UpdateSystemCaret();
}

void Editor::NotifyCaretMove() {
}

void Editor::UpdateSystemCaret() {
}

bool Editor::Wrapping() const noexcept {
	return vs.wrap.state != Wrap::None;
}

void Editor::NeedWrapping(Sci::Line docLineStart, Sci::Line docLineEnd) {
//Platform::DebugPrintf("\nNeedWrapping: %0d..%0d\n", docLineStart, docLineEnd);
	if (wrapPending.AddRange(docLineStart, docLineEnd)) {
		view.llc.Invalidate(LineLayout::ValidLevel::positions);
	}
	// Wrap lines during idle.
	if (Wrapping() && wrapPending.NeedsWrap()) {
		SetIdle(true);
	}
}

bool Editor::WrapOneLine(Surface *surface, Sci::Line lineToWrap) {
	std::shared_ptr<LineLayout> ll = view.RetrieveLineLayout(lineToWrap, *this);
	int linesWrapped = 1;
	if (ll) {
		view.LayoutLine(*this, surface, vs, ll.get(), wrapWidth);
		linesWrapped = ll->lines;
	}
	if (vs.annotationVisible != AnnotationVisible::Hidden) {
		linesWrapped += pdoc->AnnotationLines(lineToWrap);
	}
	return pcs->SetHeight(lineToWrap, linesWrapped);
}

namespace {

// Lines less than lengthToMultiThread are laid out in blocks in parallel.
// Longer lines are multi-threaded inside LayoutLine.
// This allows faster processing when lines differ greatly in length and thus time to lay out.
constexpr Sci::Position lengthToMultiThread = 4000;

}

bool Editor::WrapBlock(Surface *surface, Sci::Line lineToWrap, Sci::Line lineToWrapEnd) {

	const size_t linesBeingWrapped = static_cast<size_t>(lineToWrapEnd - lineToWrap);

	std::vector<int> linesAfterWrap(linesBeingWrapped);

	size_t threads = std::min<size_t>({ linesBeingWrapped, view.maxLayoutThreads });
	if (!surface->SupportsFeature(Supports::ThreadSafeMeasureWidths)) {
		threads = 1;
	}

	const bool multiThreaded = threads > 1;

	ElapsedPeriod epWrapping;

	// Wrap all the short lines in multiple threads

	// If only 1 thread needed then use the main thread, else spin up multiple
	const std::launch policy = multiThreaded ? std::launch::async : std::launch::deferred;

	std::atomic<size_t> nextIndex = 0;

	// Lines that are less likely to be re-examined should not be read from or written to the cache.
	const SignificantLines significantLines {
		pdoc->SciLineFromPosition(sel.MainCaret()),
		pcs->DocFromDisplay(topLine),
		LinesOnScreen() + 1,
		view.llc.GetLevel(),
	};

	// Protect the line layout cache from being accessed from multiple threads simultaneously
	std::mutex mutexRetrieve;

	std::vector<std::future<void>> futures;
	for (size_t th = 0; th < threads; th++) {
		std::future<void> fut = std::async(policy,
			[=, &surface, &nextIndex, &linesAfterWrap, &mutexRetrieve]() {
			// llTemporary is reused for non-significant lines, avoiding allocation costs.
			std::shared_ptr<LineLayout> llTemporary = std::make_shared<LineLayout>(-1, 200);
			while (true) {
				const size_t i = nextIndex.fetch_add(1, std::memory_order_acq_rel);
				if (i >= linesBeingWrapped) {
					break;
				}
				const Sci::Line lineNumber = lineToWrap + i;
				const Range rangeLine = pdoc->LineRange(lineNumber);
				const Sci::Position lengthLine = rangeLine.Length();
				if (lengthLine < lengthToMultiThread) {
					std::shared_ptr<LineLayout> ll;
					if (significantLines.LineMayCache(lineNumber)) {
						std::lock_guard<std::mutex> guard(mutexRetrieve);
						ll = view.RetrieveLineLayout(lineNumber, *this);
					} else {
						ll = llTemporary;
						ll->ReSet(lineNumber, lengthLine);
					}
					view.LayoutLine(*this, surface, vs, ll.get(), wrapWidth, multiThreaded);
					linesAfterWrap[i] = ll->lines;
				}
			}
		});
		futures.push_back(std::move(fut));
	}
	for (const std::future<void> &f : futures) {
		f.wait();
	}
	// End of multiple threads

	// Multiply duration by number of threads to produce (near) equivalence to duration if single threaded
	const double durationShortLines = epWrapping.Duration(true);
	const double durationShortLinesThreads = durationShortLines * threads;

	// Wrap all the long lines in the main thread.
	// LayoutLine may then multi-thread over segments in each line.

	std::shared_ptr<LineLayout> llLarge = std::make_shared<LineLayout>(-1, 200);
	for (size_t indexLarge = 0; indexLarge < linesBeingWrapped; indexLarge++) {
		const Sci::Line lineNumber = lineToWrap + indexLarge;
		const Range rangeLine = pdoc->LineRange(lineNumber);
		const Sci::Position lengthLine = rangeLine.Length();
		if (lengthLine >= lengthToMultiThread) {
			std::shared_ptr<LineLayout> ll;
			if (significantLines.LineMayCache(lineNumber)) {
				ll = view.RetrieveLineLayout(lineNumber, *this);
			} else {
				ll = llLarge;
				ll->ReSet(lineNumber, lengthLine);
			}
			view.LayoutLine(*this, surface, vs, ll.get(), wrapWidth);
			linesAfterWrap[indexLarge] = ll->lines;
		}
	}

	const double durationLongLines = epWrapping.Duration();
	const size_t bytesBeingWrapped = pdoc->LineStart(lineToWrap + linesBeingWrapped) - pdoc->LineStart(lineToWrap);

	size_t wrapsDone = 0;

	for (size_t i = 0; i < linesBeingWrapped; i++) {
		const Sci::Line lineNumber = lineToWrap + i;
		int linesWrapped = linesAfterWrap[i];
		if (vs.annotationVisible != AnnotationVisible::Hidden) {
			linesWrapped += pdoc->AnnotationLines(lineNumber);
		}
		if (pcs->SetHeight(lineNumber, linesWrapped)) {
			wrapsDone++;
		}
		wrapPending.Wrapped(lineNumber);
	}

	durationWrapOneByte.AddSample(bytesBeingWrapped, durationShortLinesThreads + durationLongLines);

	return wrapsDone > 0;
}

// Perform  wrapping for a subset of the lines needing wrapping.
// wsAll: wrap all lines which need wrapping in this single call
// wsVisible: wrap currently visible lines
// wsIdle: wrap one page + 100 lines
// Return true if wrapping occurred.
bool Editor::WrapLines(WrapScope ws) {
	Sci::Line goodTopLine = topLine;
	bool wrapOccurred = false;
	if (!Wrapping()) {
		if (wrapWidth != LineLayout::wrapWidthInfinite) {
			wrapWidth = LineLayout::wrapWidthInfinite;
			for (Sci::Line lineDoc = 0; lineDoc < pdoc->LinesTotal(); lineDoc++) {
				int linesWrapped = 1;
				if (vs.annotationVisible != AnnotationVisible::Hidden) {
					linesWrapped += pdoc->AnnotationLines(lineDoc);
				}
				pcs->SetHeight(lineDoc, linesWrapped);
			}
			wrapOccurred = true;
		}
		wrapPending.Reset();

	} else if (wrapPending.NeedsWrap()) {
		wrapPending.start = std::min(wrapPending.start, pdoc->LinesTotal());
		if (!SetIdle(true)) {
			// Idle processing not supported so full wrap required.
			ws = WrapScope::wsAll;
		}
		// Decide where to start wrapping
		Sci::Line lineToWrap = wrapPending.start;
		Sci::Line lineToWrapEnd = std::min(wrapPending.end, pdoc->LinesTotal());
		const Sci::Line lineDocTop = pcs->DocFromDisplay(topLine);
		const Sci::Line subLineTop = topLine - pcs->DisplayFromDoc(lineDocTop);
		if (ws == WrapScope::wsVisible) {
			lineToWrap = std::clamp(lineDocTop-5, wrapPending.start, pdoc->LinesTotal());
			// Priority wrap to just after visible area.
			// Since wrapping could reduce display lines, treat each
			// as taking only one display line.
			lineToWrapEnd = lineDocTop;
			Sci::Line lines = LinesOnScreen() + 1;
			constexpr double secondsAllowed = 0.1;
			const size_t actionsInAllowedTime = std::clamp<Sci::Line>(
				durationWrapOneByte.ActionsInAllowedTime(secondsAllowed),
				0x2000, 0x200000);
			const Sci::Line lineLast = pdoc->LineFromPositionAfter(lineToWrap, actionsInAllowedTime);
			const Sci::Line maxLine = std::min(lineLast, pcs->LinesInDoc());
			while ((lineToWrapEnd < maxLine) && (lines>0)) {
				if (pcs->GetVisible(lineToWrapEnd))
					lines--;
				lineToWrapEnd++;
			}
			// .. and if the paint window is outside pending wraps
			if ((lineToWrap > wrapPending.end) || (lineToWrapEnd < wrapPending.start)) {
				// Currently visible text does not need wrapping
				return false;
			}
		} else if (ws == WrapScope::wsIdle) {
			// Try to keep time taken by wrapping reasonable so interaction remains smooth.
			constexpr double secondsAllowed = 0.01;
			const size_t actionsInAllowedTime = std::clamp<Sci::Line>(
				durationWrapOneByte.ActionsInAllowedTime(secondsAllowed),
				0x200, 0x20000);
			lineToWrapEnd = pdoc->LineFromPositionAfter(lineToWrap, actionsInAllowedTime);
		}
		const Sci::Line lineEndNeedWrap = std::min(wrapPending.end, pdoc->LinesTotal());
		lineToWrapEnd = std::min(lineToWrapEnd, lineEndNeedWrap);

		// Ensure all lines being wrapped are styled.
		pdoc->EnsureStyledTo(pdoc->LineStart(lineToWrapEnd));

		if (lineToWrap < lineToWrapEnd) {

			PRectangle rcTextArea = GetClientRectangle();
			rcTextArea.left = static_cast<XYPOSITION>(vs.textStart);
			rcTextArea.right -= vs.rightMarginWidth;
			wrapWidth = static_cast<int>(rcTextArea.Width());
			RefreshStyleData();
			AutoSurface surface(this);
			if (surface) {
//Platform::DebugPrintf("Wraplines: scope=%0d need=%0d..%0d perform=%0d..%0d\n", ws, wrapPending.start, wrapPending.end, lineToWrap, lineToWrapEnd);

				wrapOccurred = WrapBlock(surface, lineToWrap, lineToWrapEnd);

				goodTopLine = pcs->DisplayFromDoc(lineDocTop) + std::min(
					subLineTop, static_cast<Sci::Line>(pcs->GetHeight(lineDocTop)-1));
			}
		}

		// If wrapping is done, bring it to resting position
		if (wrapPending.start >= lineEndNeedWrap) {
			wrapPending.Reset();
		}
	}

	if (wrapOccurred) {
		SetScrollBars();
		SetTopLine(std::clamp<Sci::Line>(goodTopLine, 0, MaxScrollPos()));
		SetVerticalScrollPos();
	}

	return wrapOccurred;
}

void Editor::LinesJoin() {
	if (!RangeContainsProtected(targetRange.start.Position(), targetRange.end.Position())) {
		UndoGroup ug(pdoc);
		const Sci::Line line = pdoc->SciLineFromPosition(targetRange.start.Position());
		for (Sci::Position pos = pdoc->LineEnd(line); pos < targetRange.end.Position(); pos = pdoc->LineEnd(line)) {
			const char chPrev = pdoc->CharAt(pos - 1);
			const Sci::Position widthChar = pdoc->LenChar(pos);
			targetRange.end.Add(-widthChar);
			pdoc->DeleteChars(pos, widthChar);
			if (chPrev != ' ') {
				// Ensure at least one space separating previous lines
				const Sci::Position lengthInserted = pdoc->InsertString(pos, " ", 1);
				targetRange.end.Add(lengthInserted);
			}
		}
	}
}

void Editor::LinesSplit(int pixelWidth) {
	if (!RangeContainsProtected(targetRange.start.Position(), targetRange.end.Position())) {
		if (pixelWidth == 0) {
			const PRectangle rcText = GetTextRectangle();
			pixelWidth = static_cast<int>(rcText.Width());
		}
		const Sci::Line lineStart = pdoc->SciLineFromPosition(targetRange.start.Position());
		Sci::Line lineEnd = pdoc->SciLineFromPosition(targetRange.end.Position());
		const std::string_view eol = pdoc->EOLString();
		UndoGroup ug(pdoc);
		for (Sci::Line line = lineStart; line <= lineEnd; line++) {
			AutoSurface surface(this);
			std::shared_ptr<LineLayout> ll = view.RetrieveLineLayout(line, *this);
			if (surface && ll) {
				const Sci::Position posLineStart = pdoc->LineStart(line);
				view.LayoutLine(*this, surface, vs, ll.get(), pixelWidth);
				Sci::Position lengthInsertedTotal = 0;
				for (int subLine = 1; subLine < ll->lines; subLine++) {
					const Sci::Position lengthInserted = pdoc->InsertString(
						posLineStart + lengthInsertedTotal + ll->LineStart(subLine), eol);
					targetRange.end.Add(lengthInserted);
					lengthInsertedTotal += lengthInserted;
				}
			}
			lineEnd = pdoc->SciLineFromPosition(targetRange.end.Position());
		}
	}
}

void Editor::PaintSelMargin(Surface *surfaceWindow, const PRectangle &rc) {
	if (vs.fixedColumnWidth == 0)
		return;

	RefreshStyleData();
	RefreshPixMaps(surfaceWindow);

	// On GTK+ with Ubuntu overlay scroll bars, the surface may have been finished
	// at this point. The Initialised call checks for this case and sets the status
	// to be bad which avoids crashes in following calls.
	if (!surfaceWindow->Initialised()) {
		return;
	}

	PRectangle rcMargin = GetClientRectangle();
	const Point ptOrigin = GetVisibleOriginInMain();
	rcMargin.Move(0, -ptOrigin.y);
	rcMargin.left = 0;
	rcMargin.right = static_cast<XYPOSITION>(vs.fixedColumnWidth);

	if (!rc.Intersects(rcMargin))
		return;

	Surface *surface;
	if (view.bufferedDraw) {
		surface = marginView.pixmapSelMargin.get();
	} else {
		surface = surfaceWindow;
	}
	surface->SetMode(CurrentSurfaceMode());

	// Clip vertically to paint area to avoid drawing line numbers
	if (rcMargin.bottom > rc.bottom)
		rcMargin.bottom = rc.bottom;
	if (rcMargin.top < rc.top)
		rcMargin.top = rc.top;

	marginView.PaintMargin(surface, topLine, rc, rcMargin, *this, vs);

	if (view.bufferedDraw) {
		marginView.pixmapSelMargin->FlushDrawing();
		surfaceWindow->Copy(rcMargin, Point(rcMargin.left, rcMargin.top), *marginView.pixmapSelMargin);
	}
}

void Editor::RefreshPixMaps(Surface *surfaceWindow) {
	view.RefreshPixMaps(surfaceWindow, vs);
	marginView.RefreshPixMaps(surfaceWindow, vs);
	if (view.bufferedDraw) {
		const PRectangle rcClient = GetClientRectangle();
		if (!view.pixmapLine) {
			view.pixmapLine = surfaceWindow->AllocatePixMap(static_cast<int>(rcClient.Width()), vs.lineHeight);
		}
		if (!marginView.pixmapSelMargin) {
			marginView.pixmapSelMargin = surfaceWindow->AllocatePixMap(vs.fixedColumnWidth,
				static_cast<int>(rcClient.Height()));
		}
	}
}

void Editor::Paint(Surface *surfaceWindow, PRectangle rcArea) {
	redrawPendingText = false;
	redrawPendingMargin = false;

	//Platform::DebugPrintf("Paint:%1d (%3d,%3d) ... (%3d,%3d)\n",
	//	paintingAllText, rcArea.left, rcArea.top, rcArea.right, rcArea.bottom);

	RefreshStyleData();
	if (paintState == PaintState::abandoned)
		return;	// Scroll bars may have changed so need redraw
	RefreshPixMaps(surfaceWindow);

	paintAbandonedByStyling = false;

	StyleAreaBounded(rcArea, false);

	const PRectangle rcClient = GetClientRectangle();
	//Platform::DebugPrintf("Client: (%3d,%3d) ... (%3d,%3d)   %d\n",
	//	rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

	if (NotifyUpdateUI()) {
		RefreshStyleData();
		RefreshPixMaps(surfaceWindow);
	}

	// Wrap the visible lines if needed.
	if (WrapLines(WrapScope::wsVisible)) {
		// The wrapping process has changed the height of some lines so
		// abandon this paint for a complete repaint.
		if (AbandonPaint()) {
			return;
		}
		RefreshPixMaps(surfaceWindow);	// In case pixmaps invalidated by scrollbar change
	}

	if (!marginView.pixmapSelPattern->Initialised()) {
		// When Direct2D is used, pixmap creation may fail with D2DERR_RECREATE_TARGET so
		// abandon this paint to avoid further failures.
		// Main drawing surface and pixmaps should be recreated by next paint.
		return;
	}

	if (!view.bufferedDraw)
		surfaceWindow->SetClip(rcArea);

	if (paintState != PaintState::abandoned) {
		if (vs.marginInside) {
			PaintSelMargin(surfaceWindow, rcArea);
			PRectangle rcRightMargin = rcClient;
			rcRightMargin.left = rcRightMargin.right - vs.rightMarginWidth;
			if (rcArea.Intersects(rcRightMargin)) {
				surfaceWindow->FillRectangle(rcRightMargin, vs.styles[StyleDefault].back);
			}
		} else { // Else separate view so separate paint event but leftMargin included to allow overlap
			PRectangle rcLeftMargin = rcArea;
			rcLeftMargin.left = 0;
			rcLeftMargin.right = rcLeftMargin.left + vs.leftMarginWidth;
			if (rcArea.Intersects(rcLeftMargin)) {
				surfaceWindow->FillRectangle(rcLeftMargin, vs.styles[StyleDefault].back);
			}
		}
	}

	if (paintState == PaintState::abandoned) {
		// Either styling or NotifyUpdateUI noticed that painting is needed
		// outside the current painting rectangle
		//Platform::DebugPrintf("Abandoning paint\n");
		if (Wrapping()) {
			if (paintAbandonedByStyling) {
				// Styling has spilled over a line end, such as occurs by starting a multiline
				// comment. The width of subsequent text may have changed, so rewrap.
				NeedWrapping(pcs->DocFromDisplay(topLine));
			}
		}
		if (!view.bufferedDraw)
			surfaceWindow->PopClip();
		return;
	}

	view.PaintText(surfaceWindow, *this, vs, rcArea, rcClient);

	if (horizontalScrollBarVisible && trackLineWidth && (view.lineWidthMaxSeen > scrollWidth)) {
		scrollWidth = view.lineWidthMaxSeen;
		if (!FineTickerRunning(TickReason::widen)) {
			FineTickerStart(TickReason::widen, 50, 5);
		}
	}

	if (!view.bufferedDraw)
		surfaceWindow->PopClip();

	NotifyPainted();
}

// This is mostly copied from the Paint method but with some things omitted
// such as the margin markers, line numbers, selection and caret
// Should be merged back into a combined Draw method.
Sci::Position Editor::FormatRange(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam) {
	if (!lParam)
		return 0;
	const bool draw = wParam != 0;
	void *ptr = PtrFromSPtr(lParam);
	if (iMessage == Message::FormatRange) {
		RangeToFormat *pfr = static_cast<RangeToFormat *>(ptr);
		const CharacterRangeFull chrg{ pfr->chrg.cpMin, pfr->chrg.cpMax };
		AutoSurface surface(pfr->hdc, this, Technology::Default);
		AutoSurface surfaceMeasure(pfr->hdcTarget, this, Technology::Default);
		if (!surface || !surfaceMeasure) {
			return 0;
		}
		return view.FormatRange(draw, chrg, pfr->rc, surface, surfaceMeasure, *this, vs);
	} else {
		// FormatRangeFull
		RangeToFormatFull *pfr = static_cast<RangeToFormatFull *>(ptr);
		AutoSurface surface(pfr->hdc, this, Technology::Default);
		AutoSurface surfaceMeasure(pfr->hdcTarget, this, Technology::Default);
		if (!surface || !surfaceMeasure) {
			return 0;
		}
		return view.FormatRange(draw, pfr->chrg, pfr->rc, surface, surfaceMeasure, *this, vs);
	}
}

long Editor::TextWidth(uptr_t style, const char *text) {
	RefreshStyleData();
	AutoSurface surface(this);
	if (surface) {
		return std::lround(surface->WidthText(vs.styles[style].font.get(), text));
	} else {
		return 1;
	}
}

// Empty method is overridden on GTK+ to show / hide scrollbars
void Editor::ReconfigureScrollBars() {}

void Editor::ChangeScrollBars() {
	RefreshStyleData();

	const Sci::Line nMax = MaxScrollPos();
	const Sci::Line nPage = LinesOnScreen();
	const bool modified = ModifyScrollBars(nMax + nPage - 1, nPage);
	if (modified) {
		DwellEnd(true);
	}

	// TODO: ensure always showing as many lines as possible
	// May not be, if, for example, window made larger
	if (topLine > MaxScrollPos()) {
		SetTopLine(std::clamp<Sci::Line>(topLine, 0, MaxScrollPos()));
		SetVerticalScrollPos();
		Redraw();
	}
	if (modified) {
		if (!AbandonPaint())
			Redraw();
	}
	//Platform::DebugPrintf("end max = %d page = %d\n", nMax, nPage);
}

void Editor::SetScrollBars() {
	// Overridden on GTK to defer to idle
	ChangeScrollBars();
}

void Editor::ChangeSize() {
	DropGraphics();
	SetScrollBars();
	if (Wrapping()) {
		PRectangle rcTextArea = GetClientRectangle();
		rcTextArea.left = static_cast<XYPOSITION>(vs.textStart);
		rcTextArea.right -= vs.rightMarginWidth;
		if (wrapWidth != rcTextArea.Width()) {
			NeedWrapping();
			Redraw();
		}
	}
}

Sci::Position Editor::RealizeVirtualSpace(Sci::Position position, Sci::Position virtualSpace) {
	if (virtualSpace > 0) {
		const Sci::Line line = pdoc->SciLineFromPosition(position);
		const Sci::Position indent = pdoc->GetLineIndentPosition(line);
		if (indent == position) {
			return pdoc->SetLineIndentation(line, pdoc->GetLineIndentation(line) + virtualSpace);
		} else {
			const std::string spaceText(virtualSpace, ' ');
			const Sci::Position lengthInserted = pdoc->InsertString(position, spaceText);
			position += lengthInserted;
		}
	}
	return position;
}

SelectionPosition Editor::RealizeVirtualSpace(const SelectionPosition &position) {
	// Return the new position with no virtual space
	return SelectionPosition(RealizeVirtualSpace(position.Position(), position.VirtualSpace()));
}

void Editor::AddChar(char ch) {
	const char s[1] {ch};
	InsertCharacter(std::string_view(s, 1), CharacterSource::DirectInput);
}

void Editor::FilterSelections() {
	if (!additionalSelectionTyping && (sel.Count() > 1)) {
		InvalidateWholeSelection();
		sel.DropAdditionalRanges();
	}
}

// InsertCharacter inserts a character encoded in document code page.
void Editor::InsertCharacter(std::string_view sv, CharacterSource charSource) {
	if (sv.empty()) {
		return;
	}
	FilterSelections();
	bool wrapOccurred = false;
	{
		UndoGroup ug(pdoc, (sel.Count() > 1) || !sel.Empty() || inOverstrike);

		// Vector elements point into selection in order to change selection.
		std::vector<SelectionRange *> selPtrs;
		for (size_t r = 0; r < sel.Count(); r++) {
			selPtrs.push_back(&sel.Range(r));
		}
		// Order selections by position in document.
		std::sort(selPtrs.begin(), selPtrs.end(),
			[](const SelectionRange *a, const SelectionRange *b) noexcept {return *a < *b;});

		// Loop in reverse to avoid disturbing positions of selections yet to be processed.
		for (std::vector<SelectionRange *>::reverse_iterator rit = selPtrs.rbegin();
			rit != selPtrs.rend(); ++rit) {
			SelectionRange *currentSel = *rit;
			if (!RangeContainsProtected(*currentSel)) {
				Sci::Position positionInsert = currentSel->Start().Position();
				if (!currentSel->Empty()) {
					ClearSelectionRange(*currentSel);
				} else if (inOverstrike) {
					if (positionInsert < pdoc->Length()) {
						if (!pdoc->IsPositionInLineEnd(positionInsert)) {
							pdoc->DelChar(positionInsert);
							currentSel->ClearVirtualSpace();
						}
					}
				}
				positionInsert = RealizeVirtualSpace(positionInsert, currentSel->caret.VirtualSpace());
				const Sci::Position lengthInserted = pdoc->InsertString(positionInsert, sv);
				if (lengthInserted > 0) {
					*currentSel = SelectionRange(positionInsert + lengthInserted);
				}
				currentSel->ClearVirtualSpace();
				// If in wrap mode rewrap current line so EnsureCaretVisible has accurate information
				if (Wrapping()) {
					AutoSurface surface(this);
					if (surface) {
						if (WrapOneLine(surface, pdoc->SciLineFromPosition(positionInsert))) {
							wrapOccurred = true;
						}
					}
				}
			}
		}
		ThinRectangularRange();
	}
	if (wrapOccurred) {
		SetScrollBars();
		SetVerticalScrollPos();
		Redraw();
	}
	// If in wrap mode rewrap current line so EnsureCaretVisible has accurate information
	EnsureCaretVisible();
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
	if ((caretSticky == CaretSticky::Off) ||
		((caretSticky == CaretSticky::WhiteSpace) && !IsAllSpacesOrTabs(sv))) {
		SetLastXChosen();
	}

	int ch = static_cast<unsigned char>(sv[0]);
	if (pdoc->dbcsCodePage != CpUtf8) {
		if (sv.length() > 1) {
			// DBCS code page or DBCS font character set.
			ch = (ch << 8) | static_cast<unsigned char>(sv[1]);
		}
	} else {
		if ((ch < 0xC0) || (1 == sv.length())) {
			// Handles UTF-8 characters between 0x01 and 0x7F and single byte
			// characters when not in UTF-8 mode.
			// Also treats \0 and naked trail bytes 0x80 to 0xBF as valid
			// characters representing themselves.
		} else {
			unsigned int utf32[1] = { 0 };
			UTF32FromUTF8(sv, utf32, std::size(utf32));
			ch = utf32[0];
		}
	}
	NotifyChar(ch, charSource);

	if (recordingMacro && charSource != CharacterSource::TentativeInput) {
		std::string copy(sv); // ensure NUL-terminated
		NotifyMacroRecord(Message::ReplaceSel, 0, reinterpret_cast<sptr_t>(copy.data()));
	}
}

void Editor::ClearSelectionRange(SelectionRange &range) {
	if (!range.Empty()) {
		if (range.Length()) {
			pdoc->DeleteChars(range.Start().Position(), range.Length());
			range.ClearVirtualSpace();
		} else {
			// Range is all virtual so collapse to start of virtual space
			range.MinimizeVirtualSpace();
		}
	}
}

void Editor::ClearBeforeTentativeStart() {
	// Make positions for the first composition string.
	FilterSelections();
	UndoGroup ug(pdoc, (sel.Count() > 1) || !sel.Empty() || inOverstrike);
	for (size_t r = 0; r<sel.Count(); r++) {
		if (!RangeContainsProtected(sel.Range(r))) {
			ClearSelectionRange(sel.Range(r));
			RealizeVirtualSpace(sel.Range(r).caret.Position(), sel.Range(r).caret.VirtualSpace());
			sel.Range(r).ClearVirtualSpace();
		}
	}
}

void Editor::InsertPaste(const char *text, Sci::Position len) {
	if (multiPasteMode == MultiPaste::Once) {
		SelectionPosition selStart = sel.Start();
		selStart = RealizeVirtualSpace(selStart);
		const Sci::Position lengthInserted = pdoc->InsertString(selStart.Position(), text, len);
		if (lengthInserted > 0) {
			SetEmptySelection(selStart.Position() + lengthInserted);
		}
	} else {
		// MultiPaste::Each
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r))) {
				Sci::Position positionInsert = sel.Range(r).Start().Position();
				ClearSelectionRange(sel.Range(r));
				positionInsert = RealizeVirtualSpace(positionInsert, sel.Range(r).caret.VirtualSpace());
				const Sci::Position lengthInserted = pdoc->InsertString(positionInsert, text, len);
				if (lengthInserted > 0) {
					sel.Range(r) = SelectionRange(positionInsert + lengthInserted);
				}
				sel.Range(r).ClearVirtualSpace();
			}
		}
	}
}

void Editor::InsertPasteShape(const char *text, Sci::Position len, PasteShape shape) {
	std::string convertedText;
	if (convertPastes) {
		// Convert line endings of the paste into our local line-endings mode
		convertedText = Document::TransformLineEnds(text, len, pdoc->eolMode);
		len = convertedText.length();
		text = convertedText.c_str();
	}
	if (shape == PasteShape::rectangular) {
		PasteRectangular(sel.Start(), text, len);
	} else {
		if (shape == PasteShape::line) {
			const Sci::Position insertPos = pdoc->LineStartPosition(sel.MainCaret());
			Sci::Position lengthInserted = pdoc->InsertString(insertPos, text, len);
			// add the newline if necessary
			if ((len > 0) && (text[len - 1] != '\n' && text[len - 1] != '\r')) {
				const std::string_view endline = pdoc->EOLString();
				lengthInserted += pdoc->InsertString(insertPos + lengthInserted, endline);
			}
			if (sel.MainCaret() == insertPos) {
				SetEmptySelection(sel.MainCaret() + lengthInserted);
			}
		} else {
			InsertPaste(text, len);
		}
	}
}

void Editor::ClearSelection(bool retainMultipleSelections) {
	if (!sel.IsRectangular() && !retainMultipleSelections)
		FilterSelections();
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		if (!sel.Range(r).Empty()) {
			if (!RangeContainsProtected(sel.Range(r))) {
				pdoc->DeleteChars(sel.Range(r).Start().Position(),
					sel.Range(r).Length());
				sel.Range(r) = SelectionRange(sel.Range(r).Start());
			}
		}
	}
	ThinRectangularRange();
	sel.RemoveDuplicates();
	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());
}

void Editor::ClearAll() {
	{
		UndoGroup ug(pdoc);
		if (0 != pdoc->Length()) {
			pdoc->DeleteChars(0, pdoc->Length());
		}
		if (!pdoc->IsReadOnly()) {
			pcs->Clear();
			pdoc->AnnotationClearAll();
			pdoc->EOLAnnotationClearAll();
			pdoc->MarginClearAll();
		}
	}

	view.ClearAllTabstops();

	sel.Clear();
	SetTopLine(0);
	SetVerticalScrollPos();
	InvalidateStyleRedraw();
}

void Editor::ClearDocumentStyle() {
	pdoc->decorations->DeleteLexerDecorations();
	pdoc->StartStyling(0);
	pdoc->SetStyleFor(pdoc->Length(), 0);
	pcs->ShowAll();
	SetAnnotationHeights(0, pdoc->LinesTotal());
	pdoc->ClearLevels();
}

void Editor::CopyAllowLine() {
	SelectionText selectedText;
	CopySelectionRange(&selectedText, true);
	CopyToClipboard(selectedText);
}

void Editor::CutAllowLine() {
	if (sel.Empty()) {
		pdoc->CheckReadOnly();
		if (!pdoc->IsReadOnly()) {
			SelectionText selectedText;
			if (CopyLineRange(&selectedText, false)) {
				CopyToClipboard(selectedText);
				LineDelete();
			}
		}
	} else {
		Cut();
	}
}

void Editor::Cut() {
	pdoc->CheckReadOnly();
	if (!pdoc->IsReadOnly() && !SelectionContainsProtected()) {
		Copy();
		ClearSelection();
	}
}

void Editor::PasteRectangular(SelectionPosition pos, const char *ptr, Sci::Position len) {
	if (pdoc->IsReadOnly() || SelectionContainsProtected()) {
		return;
	}
	sel.Clear();
	sel.RangeMain() = SelectionRange(pos);
	Sci::Line line = pdoc->SciLineFromPosition(sel.MainCaret());
	UndoGroup ug(pdoc);
	sel.RangeMain().caret = RealizeVirtualSpace(sel.RangeMain().caret);
	const int xInsert = XFromPosition(sel.RangeMain().caret);
	bool prevCr = false;
	while ((len > 0) && IsEOLCharacter(ptr[len-1]))
		len--;
	for (Sci::Position i = 0; i < len; i++) {
		if (IsEOLCharacter(ptr[i])) {
			if ((ptr[i] == '\r') || (!prevCr))
				line++;
			if (line >= pdoc->LinesTotal()) {
				const std::string_view eol = pdoc->EOLString();
				pdoc->InsertString(pdoc->LengthNoExcept(), eol);
			}
			// Pad the end of lines with spaces if required
			sel.RangeMain().caret.SetPosition(PositionFromLineX(line, xInsert));
			if ((XFromPosition(sel.RangeMain().caret) < xInsert) && (i + 1 < len)) {
				while (XFromPosition(sel.RangeMain().caret) < xInsert) {
					assert(pdoc);
					const Sci::Position lengthInserted = pdoc->InsertString(sel.MainCaret(), " ", 1);
					sel.RangeMain().caret.Add(lengthInserted);
				}
			}
			prevCr = ptr[i] == '\r';
		} else {
			const Sci::Position lengthInserted = pdoc->InsertString(sel.MainCaret(), ptr + i, 1);
			sel.RangeMain().caret.Add(lengthInserted);
			prevCr = false;
		}
	}
	SetEmptySelection(pos);
}

bool Editor::CanPaste() {
	return !pdoc->IsReadOnly() && !SelectionContainsProtected();
}

void Editor::Clear() {
	// If multiple selections, don't delete EOLS
	if (sel.Empty()) {
		bool singleVirtual = false;
		if ((sel.Count() == 1) &&
			!RangeContainsProtected(sel.MainCaret(), sel.MainCaret() + 1) &&
			sel.RangeMain().Start().VirtualSpace()) {
			singleVirtual = true;
		}
		UndoGroup ug(pdoc, (sel.Count() > 1) || singleVirtual);
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r).caret.Position(), sel.Range(r).caret.Position() + 1)) {
				if (sel.Range(r).Start().VirtualSpace()) {
					if (sel.Range(r).anchor < sel.Range(r).caret)
						sel.Range(r) = SelectionRange(RealizeVirtualSpace(sel.Range(r).anchor.Position(), sel.Range(r).anchor.VirtualSpace()));
					else
						sel.Range(r) = SelectionRange(RealizeVirtualSpace(sel.Range(r).caret.Position(), sel.Range(r).caret.VirtualSpace()));
				}
				if ((sel.Count() == 1) || !pdoc->IsPositionInLineEnd(sel.Range(r).caret.Position())) {
					pdoc->DelChar(sel.Range(r).caret.Position());
					sel.Range(r).ClearVirtualSpace();
				}  // else multiple selection so don't eat line ends
			} else {
				sel.Range(r).ClearVirtualSpace();
			}
		}
	} else {
		ClearSelection();
	}
	sel.RemoveDuplicates();
	ShowCaretAtCurrentPosition();		// Avoid blinking
}

void Editor::SelectAll() {
	sel.Clear();
	SetSelection(0, pdoc->Length());
	Redraw();
}

void Editor::RestoreSelection(Sci::Position newPos, UndoRedo history) {
	EnsureModelState();
	if ((undoSelectionHistoryOption == UndoSelectionHistoryOption::Enabled) && modelState) {
		// Undo wants the element after the current as it just undid it
		const int index = pdoc->UndoCurrent() + (history == UndoRedo::undo ? 1 : 0);
		const SelectionWithScroll selAndLine = modelState->SelectionFromStack(index, history);
		if (!selAndLine.selection.empty()) {
			ScrollTo(selAndLine.topLine);
			sel = Selection(selAndLine.selection);
			if (sel.IsRectangular()) {
				const size_t mainForRectangular = sel.Main();
				// Reconstitute ranges from rectangular range
				SetRectangularRange();
				// Restore main if possible.
				if (mainForRectangular < sel.Count()) {
					sel.SetMain(mainForRectangular);
				}
			}
			newPos = -1; // Used selection from stack so don't use position returned from undo/redo.
		}
	}
	if (newPos >= 0)
		SetEmptySelection(newPos);
	EnsureCaretVisible();
}

void Editor::Undo() {
	if (pdoc->CanUndo()) {
		InvalidateCaret();
		const Sci::Position newPos = pdoc->Undo();
		RestoreSelection(newPos, UndoRedo::undo);
	}
}

void Editor::Redo() {
	if (pdoc->CanRedo()) {
		const Sci::Position newPos = pdoc->Redo();
		RestoreSelection(newPos, UndoRedo::redo);
	}
}

void Editor::DelCharBack(bool allowLineStartDeletion) {
	RefreshStyleData();
	if (!sel.IsRectangular())
		FilterSelections();
	if (sel.IsRectangular())
		allowLineStartDeletion = false;
	UndoGroup ug(pdoc, (sel.Count() > 1) || !sel.Empty());
	if (sel.Empty()) {
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r).caret.Position() - 1, sel.Range(r).caret.Position())) {
				if (sel.Range(r).caret.VirtualSpace()) {
					sel.Range(r).caret.SetVirtualSpace(sel.Range(r).caret.VirtualSpace() - 1);
					sel.Range(r).anchor.SetVirtualSpace(sel.Range(r).caret.VirtualSpace());
				} else {
					const Sci::Line lineCurrentPos =
						pdoc->SciLineFromPosition(sel.Range(r).caret.Position());
					if (allowLineStartDeletion || (pdoc->LineStart(lineCurrentPos) != sel.Range(r).caret.Position())) {
						if (pdoc->GetColumn(sel.Range(r).caret.Position()) <= pdoc->GetLineIndentation(lineCurrentPos) &&
								pdoc->GetColumn(sel.Range(r).caret.Position()) > 0 && pdoc->backspaceUnindents) {
							UndoGroup ugInner(pdoc, !ug.Needed());
							const int indentation = pdoc->GetLineIndentation(lineCurrentPos);
							const int indentationStep = pdoc->IndentSize();
							int indentationChange = indentation % indentationStep;
							if (indentationChange == 0)
								indentationChange = indentationStep;
							const Sci::Position posSelect = pdoc->SetLineIndentation(lineCurrentPos, indentation - indentationChange);
							// SetEmptySelection
							sel.Range(r) = SelectionRange(posSelect);
						} else {
							pdoc->DelCharBack(sel.Range(r).caret.Position());
						}
					}
				}
			} else {
				sel.Range(r).ClearVirtualSpace();
			}
		}
		ThinRectangularRange();
	} else {
		ClearSelection();
	}
	sel.RemoveDuplicates();
	ContainerNeedsUpdate(Update::Selection);
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

void Editor::NotifyFocus(bool focus) {
	NotificationData scn = {};
	scn.nmhdr.code = focus ? Notification::FocusIn : Notification::FocusOut;
	NotifyParent(scn);
}

void Editor::SetCtrlID(int identifier) {
	ctrlID = identifier;
}

void Editor::NotifyStyleToNeeded(Sci::Position endStyleNeeded) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::StyleNeeded;
	scn.position = endStyleNeeded;
	NotifyParent(scn);
}

void Editor::NotifyStyleNeeded(Document *, void *, Sci::Position endStyleNeeded) {
	NotifyStyleToNeeded(endStyleNeeded);
}

void Editor::NotifyErrorOccurred(Document *, void *, Status status) {
	errorStatus = status;
}

void Editor::NotifyGroupCompleted(Document *, void *) noexcept {
	// RememberCurrentSelectionForRedoOntoStack may throw (for memory exhaustion)
	// but this method may not as it is called in UndoGroup destructor so ignore
	// exception.
	try {
		RememberCurrentSelectionForRedoOntoStack();
	} catch (...) {
		// Ignore any exception
	}
}

void Editor::NotifyChar(int ch, CharacterSource charSource) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::CharAdded;
	scn.ch = ch;
	scn.characterSource = charSource;
	NotifyParent(scn);
}

void Editor::NotifySavePoint(bool isSavePoint) {
	NotificationData scn = {};
	if (isSavePoint) {
		scn.nmhdr.code = Notification::SavePointReached;
		if (changeHistoryOption != ChangeHistoryOption::Disabled) {
			Redraw();
		}
	} else {
		scn.nmhdr.code = Notification::SavePointLeft;
	}
	NotifyParent(scn);
}

void Editor::NotifyModifyAttempt() {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::ModifyAttemptRO;
	NotifyParent(scn);
}

void Editor::NotifyDoubleClick(Point pt, KeyMod modifiers) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::DoubleClick;
	scn.line = LineFromLocation(pt);
	scn.position = PositionFromLocation(pt, true);
	scn.modifiers = modifiers;
	NotifyParent(scn);
}

void Editor::NotifyHotSpotDoubleClicked(Sci::Position position, KeyMod modifiers) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::HotSpotDoubleClick;
	scn.position = position;
	scn.modifiers = modifiers;
	NotifyParent(scn);
}

void Editor::NotifyHotSpotClicked(Sci::Position position, KeyMod modifiers) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::HotSpotClick;
	scn.position = position;
	scn.modifiers = modifiers;
	NotifyParent(scn);
}

void Editor::NotifyHotSpotReleaseClick(Sci::Position position, KeyMod modifiers) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::HotSpotReleaseClick;
	scn.position = position;
	scn.modifiers = modifiers;
	NotifyParent(scn);
}

bool Editor::NotifyUpdateUI() {
	if (needUpdateUI != Update::None) {
		NotificationData scn = {};
		scn.nmhdr.code = Notification::UpdateUI;
		scn.updated = needUpdateUI;
		NotifyParent(scn);
		needUpdateUI = Update::None;
		return true;
	}
	return false;
}

void Editor::NotifyPainted() {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::Painted;
	NotifyParent(scn);
}

void Editor::NotifyIndicatorClick(bool click, Sci::Position position, KeyMod modifiers) {
	const int mask = pdoc->decorations->AllOnFor(position);
	if ((click && mask) || pdoc->decorations->ClickNotified()) {
		NotificationData scn = {};
		pdoc->decorations->SetClickNotified(click);
		scn.nmhdr.code = click ? Notification::IndicatorClick : Notification::IndicatorRelease;
		scn.modifiers = modifiers;
		scn.position = position;
		NotifyParent(scn);
	}
}

bool Editor::NotifyMarginClick(Point pt, KeyMod modifiers) {
	const int marginClicked = vs.MarginFromLocation(pt);
	if ((marginClicked >= 0) && vs.ms[marginClicked].sensitive) {
		const Sci::Position position = pdoc->LineStart(LineFromLocation(pt));
		if ((vs.ms[marginClicked].mask & MaskFolders) && (FlagSet(foldAutomatic, AutomaticFold::Click))) {
			const bool ctrl = FlagSet(modifiers, KeyMod::Ctrl);
			const bool shift = FlagSet(modifiers, KeyMod::Shift);
			const Sci::Line lineClick = pdoc->SciLineFromPosition(position);
			if (shift && ctrl) {
				FoldAll(FoldAction::Toggle);
			} else {
				const FoldLevel levelClick = pdoc->GetFoldLevel(lineClick);
				if (LevelIsHeader(levelClick)) {
					if (shift) {
						// Ensure all children visible
						FoldExpand(lineClick, FoldAction::Expand, levelClick);
					} else if (ctrl) {
						FoldExpand(lineClick, FoldAction::Toggle, levelClick);
					} else {
						// Toggle this line
						FoldLine(lineClick, FoldAction::Toggle);
					}
				}
			}
			return true;
		}
		NotificationData scn = {};
		scn.nmhdr.code = Notification::MarginClick;
		scn.modifiers = modifiers;
		scn.position = position;
		scn.margin = marginClicked;
		NotifyParent(scn);
		return true;
	} else {
		return false;
	}
}

bool Editor::NotifyMarginRightClick(Point pt, KeyMod modifiers) {
	const int marginRightClicked = vs.MarginFromLocation(pt);
	if ((marginRightClicked >= 0) && vs.ms[marginRightClicked].sensitive) {
		const Sci::Position position = pdoc->LineStart(LineFromLocation(pt));
		NotificationData scn = {};
		scn.nmhdr.code = Notification::MarginRightClick;
		scn.modifiers = modifiers;
		scn.position = position;
		scn.margin = marginRightClicked;
		NotifyParent(scn);
		return true;
	} else {
		return false;
	}
}

void Editor::NotifyNeedShown(Sci::Position pos, Sci::Position len) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::NeedShown;
	scn.position = pos;
	scn.length = len;
	NotifyParent(scn);
}

void Editor::NotifyDwelling(Point pt, bool state) {
	NotificationData scn = {};
	scn.nmhdr.code = state ? Notification::DwellStart : Notification::DwellEnd;
	scn.position = PositionFromLocation(pt, true);
	scn.x = static_cast<int>(pt.x + vs.ExternalMarginWidth());
	scn.y = static_cast<int>(pt.y);
	NotifyParent(scn);
}

void Editor::NotifyZoom() {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::Zoom;
	NotifyParent(scn);
}

// Notifications from document
void Editor::NotifyModifyAttempt(Document *, void *) {
	//Platform::DebugPrintf("** Modify Attempt\n");
	NotifyModifyAttempt();
}

void Editor::NotifySavePoint(Document *, void *, bool atSavePoint) {
	//Platform::DebugPrintf("** Save Point %s\n", atSavePoint ? "On" : "Off");
	NotifySavePoint(atSavePoint);
}

void Editor::CheckModificationForWrap(DocModification mh) {
	if (FlagSet(mh.modificationType, ModificationFlags::InsertText | ModificationFlags::DeleteText)) {
		view.llc.Invalidate(LineLayout::ValidLevel::checkTextAndStyle);
		const Sci::Line lineDoc = pdoc->SciLineFromPosition(mh.position);
		const Sci::Line lines = std::max(static_cast<Sci::Line>(0), mh.linesAdded);
		if (Wrapping()) {
			// Check if this modification crosses any of the wrap points
			if (wrapPending.NeedsWrap()) {
				if (lineDoc < wrapPending.end) { // Inserted/deleted before or inside wrap range
					wrapPending.end += mh.linesAdded;
				}
			}
			NeedWrapping(lineDoc, lineDoc + lines + 1);
		}
		RefreshStyleData();
		// Fix up annotation heights
		SetAnnotationHeights(lineDoc, lineDoc + lines + 2);
	}
}

namespace {

// Move a position so it is still after the same character as before the insertion.
constexpr Sci::Position MovePositionForInsertion(Sci::Position position, Sci::Position startInsertion, Sci::Position length) noexcept {
	if (position > startInsertion) {
		return position + length;
	}
	return position;
}

// Move a position so it is still after the same character as before the deletion if that
// character is still present else after the previous surviving character.
constexpr Sci::Position MovePositionForDeletion(Sci::Position position, Sci::Position startDeletion, Sci::Position length) noexcept {
	if (position > startDeletion) {
		const Sci::Position endDeletion = startDeletion + length;
		if (position > endDeletion) {
			return position - length;
		} else {
			return startDeletion;
		}
	} else {
		return position;
	}
}

}

void Editor::NotifyModified(Document *, DocModification mh, void *) {
	ContainerNeedsUpdate(Update::Content);
	if (paintState == PaintState::painting) {
		CheckForChangeOutsidePaint(Range(mh.position, mh.position + mh.length));
	}
	if (FlagSet(mh.modificationType, ModificationFlags::ChangeLineState)) {
		if (paintState == PaintState::painting) {
			CheckForChangeOutsidePaint(
			    Range(pdoc->LineStart(mh.line),
					pdoc->LineStart(mh.line + 1)));
		} else {
			// Could check that change is before last visible line.
			Redraw();
		}
	}
	if (FlagSet(mh.modificationType, ModificationFlags::ChangeTabStops)) {
		Redraw();
	}
	if (FlagSet(mh.modificationType, ModificationFlags::LexerState)) {
		if (paintState == PaintState::painting) {
			CheckForChangeOutsidePaint(
			    Range(mh.position, mh.position + mh.length));
		} else {
			Redraw();
		}
	}
	if (FlagSet(mh.modificationType, ModificationFlags::ChangeStyle | ModificationFlags::ChangeIndicator)) {
		if (FlagSet(mh.modificationType, ModificationFlags::ChangeStyle)) {
			pdoc->IncrementStyleClock();
		}
		if (paintState == PaintState::notPainting) {
			const Sci::Line lineDocTop = pcs->DocFromDisplay(topLine);
			if (mh.position < pdoc->LineStart(lineDocTop)) {
				// Styling performed before this view
				Redraw();
			} else {
				InvalidateRange(mh.position, mh.position + mh.length);
			}
		}
		if (FlagSet(mh.modificationType, ModificationFlags::ChangeStyle)) {
			view.llc.Invalidate(LineLayout::ValidLevel::checkTextAndStyle);
		}
	} else {
		if ((undoSelectionHistoryOption == UndoSelectionHistoryOption::Enabled) &&
			FlagSet(mh.modificationType, ModificationFlags::User)) {
			if (FlagSet(mh.modificationType, ModificationFlags::BeforeInsert | ModificationFlags::BeforeDelete)) {
				RememberSelectionForUndo(pdoc->UndoCurrent());
			}
			if (FlagSet(mh.modificationType, ModificationFlags::InsertText | ModificationFlags::DeleteText)) {
				RememberSelectionOntoStack(pdoc->UndoCurrent());
			}
		}
		// Move selection and brace highlights
		if (FlagSet(mh.modificationType, ModificationFlags::InsertText)) {
			sel.MovePositions(true, mh.position, mh.length);
			braces[0] = MovePositionForInsertion(braces[0], mh.position, mh.length);
			braces[1] = MovePositionForInsertion(braces[1], mh.position, mh.length);
		} else if (FlagSet(mh.modificationType, ModificationFlags::DeleteText)) {
			sel.MovePositions(false, mh.position, mh.length);
			braces[0] = MovePositionForDeletion(braces[0], mh.position, mh.length);
			braces[1] = MovePositionForDeletion(braces[1], mh.position, mh.length);
		}
		if (FlagSet(mh.modificationType, ModificationFlags::BeforeInsert | ModificationFlags::BeforeDelete) && pcs->HiddenLines()) {
			// Some lines are hidden so may need shown.
			const Sci::Line lineOfPos = pdoc->SciLineFromPosition(mh.position);
			Sci::Position endNeedShown = mh.position;
			if (FlagSet(mh.modificationType, ModificationFlags::BeforeInsert)) {
				if (pdoc->ContainsLineEnd(mh.text, mh.length) && (mh.position != pdoc->LineStart(lineOfPos)))
					endNeedShown = pdoc->LineStart(lineOfPos+1);
			} else {
				// If the deletion includes any EOL then we extend the need shown area.
				endNeedShown = mh.position + mh.length;
				Sci::Line lineLast = pdoc->SciLineFromPosition(mh.position+mh.length);
				for (Sci::Line line = lineOfPos + 1; line <= lineLast; line++) {
					const Sci::Line lineMaxSubord = pdoc->GetLastChild(line, {}, -1);
					if (lineLast < lineMaxSubord) {
						lineLast = lineMaxSubord;
						endNeedShown = pdoc->LineEnd(lineLast);
					}
				}
			}
			NeedShown(mh.position, endNeedShown - mh.position);
		}
		if (mh.linesAdded != 0) {
			// Update contraction state for inserted and removed lines
			// lineOfPos should be calculated in context of state before modification, shouldn't it
			Sci::Line lineOfPos = pdoc->SciLineFromPosition(mh.position);
			if (mh.position > pdoc->LineStart(lineOfPos))
				lineOfPos++;	// Affecting subsequent lines
			if (mh.linesAdded > 0) {
				pcs->InsertLines(lineOfPos, mh.linesAdded);
			} else {
				pcs->DeleteLines(lineOfPos, -mh.linesAdded);
			}
			view.LinesAddedOrRemoved(lineOfPos, mh.linesAdded);
		}
		if (FlagSet(mh.modificationType, ModificationFlags::ChangeAnnotation)) {
			const Sci::Line lineDoc = pdoc->SciLineFromPosition(mh.position);
			if (vs.annotationVisible != AnnotationVisible::Hidden) {
				if (pcs->SetHeight(lineDoc, pcs->GetHeight(lineDoc) + static_cast<int>(mh.annotationLinesAdded))) {
					SetScrollBars();
				}
				Redraw();
			}
		}
		if (FlagSet(mh.modificationType, ModificationFlags::ChangeEOLAnnotation)) {
			if (vs.eolAnnotationVisible != EOLAnnotationVisible::Hidden) {
				Redraw();
			}
		}
		CheckModificationForWrap(mh);
		if (mh.linesAdded != 0) {
			// Avoid scrolling of display if change before current display
			if (mh.position < posTopLine && !CanDeferToLastStep(mh)) {
				const Sci::Line newTop = std::clamp<Sci::Line>(topLine + mh.linesAdded, 0, MaxScrollPos());
				if (newTop != topLine) {
					SetTopLine(newTop);
					SetVerticalScrollPos();
				}
			}

			if (paintState == PaintState::notPainting && !CanDeferToLastStep(mh)) {
				if (SynchronousStylingToVisible()) {
					QueueIdleWork(WorkItems::style, pdoc->Length());
				}
				Redraw();
			}
		} else {
			if (paintState == PaintState::notPainting && mh.length && !CanEliminate(mh)) {
				if (SynchronousStylingToVisible()) {
					QueueIdleWork(WorkItems::style, mh.position + mh.length);
				}
				InvalidateRange(mh.position, mh.position + mh.length);
				if (FlagSet(changeHistoryOption, ChangeHistoryOption::Markers)) {
					RedrawSelMargin(pdoc->SciLineFromPosition(mh.position));
				}
			}
		}
	}

	if (mh.linesAdded != 0 && !CanDeferToLastStep(mh)) {
		SetScrollBars();
	}

	if (FlagSet(mh.modificationType, (ModificationFlags::ChangeMarker | ModificationFlags::ChangeMargin))) {
		if ((!willRedrawAll) && ((paintState == PaintState::notPainting) || !PaintContainsMargin())) {
			if (FlagSet(mh.modificationType, ModificationFlags::ChangeFold)) {
				// Fold changes can affect the drawing of following lines so redraw whole margin
				RedrawSelMargin(marginView.highlightDelimiter.isEnabled ? -1 : mh.line - 1, true);
			} else {
				RedrawSelMargin(mh.line);
			}
		}
	}
	if ((FlagSet(mh.modificationType, ModificationFlags::ChangeFold)) && (FlagSet(foldAutomatic, AutomaticFold::Change))) {
		FoldChanged(mh.line, mh.foldLevelNow, mh.foldLevelPrev);
	}

	// NOW pay the piper WRT "deferred" visual updates
	if (IsLastStep(mh)) {
		SetScrollBars();
		Redraw();
	}

	// If client wants to see this modification
	if (FlagSet(mh.modificationType, modEventMask)) {
		if (commandEvents) {
			if ((mh.modificationType & (ModificationFlags::ChangeStyle | ModificationFlags::ChangeIndicator)) == ModificationFlags::None) {
				// Real modification made to text of document.
				NotifyChange();	// Send EN_CHANGE
			}
		}

		NotificationData scn = {};
		scn.nmhdr.code = Notification::Modified;
		scn.position = mh.position;
		scn.modificationType = mh.modificationType;
		scn.text = mh.text;
		scn.length = mh.length;
		scn.linesAdded = mh.linesAdded;
		scn.line = mh.line;
		scn.foldLevelNow = mh.foldLevelNow;
		scn.foldLevelPrev = mh.foldLevelPrev;
		scn.token = static_cast<int>(mh.token);
		scn.annotationLinesAdded = mh.annotationLinesAdded;
		NotifyParent(scn);
	}
}

void Editor::NotifyDeleted(Document *, void *) noexcept {
	/* Do nothing */
}

void Editor::NotifyMacroRecord(Message iMessage, uptr_t wParam, sptr_t lParam) {

	// Enumerates all macroable messages
	switch (iMessage) {
	case Message::Cut:
	case Message::Copy:
	case Message::Paste:
	case Message::Clear:
	case Message::ReplaceSel:
	case Message::AddText:
	case Message::InsertText:
	case Message::AppendText:
	case Message::ClearAll:
	case Message::SelectAll:
	case Message::GotoLine:
	case Message::GotoPos:
	case Message::SearchAnchor:
	case Message::SearchNext:
	case Message::SearchPrev:
	case Message::LineDown:
	case Message::LineDownExtend:
	case Message::ParaDown:
	case Message::ParaDownExtend:
	case Message::LineUp:
	case Message::LineUpExtend:
	case Message::ParaUp:
	case Message::ParaUpExtend:
	case Message::CharLeft:
	case Message::CharLeftExtend:
	case Message::CharRight:
	case Message::CharRightExtend:
	case Message::WordLeft:
	case Message::WordLeftExtend:
	case Message::WordRight:
	case Message::WordRightExtend:
	case Message::WordPartLeft:
	case Message::WordPartLeftExtend:
	case Message::WordPartRight:
	case Message::WordPartRightExtend:
	case Message::WordLeftEnd:
	case Message::WordLeftEndExtend:
	case Message::WordRightEnd:
	case Message::WordRightEndExtend:
	case Message::Home:
	case Message::HomeExtend:
	case Message::LineEnd:
	case Message::LineEndExtend:
	case Message::HomeWrap:
	case Message::HomeWrapExtend:
	case Message::LineEndWrap:
	case Message::LineEndWrapExtend:
	case Message::DocumentStart:
	case Message::DocumentStartExtend:
	case Message::DocumentEnd:
	case Message::DocumentEndExtend:
	case Message::StutteredPageUp:
	case Message::StutteredPageUpExtend:
	case Message::StutteredPageDown:
	case Message::StutteredPageDownExtend:
	case Message::PageUp:
	case Message::PageUpExtend:
	case Message::PageDown:
	case Message::PageDownExtend:
	case Message::EditToggleOvertype:
	case Message::Cancel:
	case Message::DeleteBack:
	case Message::Tab:
	case Message::LineIndent:
	case Message::BackTab:
	case Message::LineDedent:
	case Message::FormFeed:
	case Message::VCHome:
	case Message::VCHomeExtend:
	case Message::VCHomeWrap:
	case Message::VCHomeWrapExtend:
	case Message::VCHomeDisplay:
	case Message::VCHomeDisplayExtend:
	case Message::DelWordLeft:
	case Message::DelWordRight:
	case Message::DelWordRightEnd:
	case Message::DelLineLeft:
	case Message::DelLineRight:
	case Message::LineCopy:
	case Message::LineCut:
	case Message::LineDelete:
	case Message::LineTranspose:
	case Message::LineReverse:
	case Message::LineDuplicate:
	case Message::LowerCase:
	case Message::UpperCase:
	case Message::LineScrollDown:
	case Message::LineScrollUp:
	case Message::DeleteBackNotLine:
	case Message::HomeDisplay:
	case Message::HomeDisplayExtend:
	case Message::LineEndDisplay:
	case Message::LineEndDisplayExtend:
	case Message::SetSelectionMode:
	case Message::LineDownRectExtend:
	case Message::LineUpRectExtend:
	case Message::CharLeftRectExtend:
	case Message::CharRightRectExtend:
	case Message::HomeRectExtend:
	case Message::VCHomeRectExtend:
	case Message::LineEndRectExtend:
	case Message::PageUpRectExtend:
	case Message::PageDownRectExtend:
	case Message::SelectionDuplicate:
	case Message::CopyAllowLine:
	case Message::CutAllowLine:
	case Message::VerticalCentreCaret:
	case Message::MoveSelectedLinesUp:
	case Message::MoveSelectedLinesDown:
	case Message::ScrollToStart:
	case Message::ScrollToEnd:
		break;

		// Filter out all others like display changes. Also, newlines are redundant
		// with char insert messages.
	case Message::NewLine:
	default:
		//		printf("Filtered out %ld of macro recording\n", iMessage);
		return;
	}

	// Send notification
	NotificationData scn = {};
	scn.nmhdr.code = Notification::MacroRecord;
	scn.message = iMessage;
	scn.wParam = wParam;
	scn.lParam = lParam;
	NotifyParent(scn);
}

// Something has changed that the container should know about
void Editor::ContainerNeedsUpdate(Update flags) noexcept {
	needUpdateUI = needUpdateUI | flags;
}

/**
 * Force scroll and keep position relative to top of window.
 *
 * If stuttered = true and not already at first/last row, move to first/last row of window.
 * If stuttered = true and already at first/last row, scroll as normal.
 */
void Editor::PageMove(int direction, Selection::SelTypes selt, bool stuttered) {
	Sci::Line topLineNew;
	SelectionPosition newPos;

	const Sci::Line currentLine = pdoc->SciLineFromPosition(sel.MainCaret());
	const Sci::Line topStutterLine = topLine + caretPolicies.y.slop;
	const Sci::Line bottomStutterLine =
	    pdoc->SciLineFromPosition(PositionFromLocation(
	                Point::FromInts(lastXChosen - xOffset, direction * vs.lineHeight * static_cast<int>(LinesToScroll()))))
	    - caretPolicies.y.slop - 1;

	if (stuttered && (direction < 0 && currentLine > topStutterLine)) {
		topLineNew = topLine;
		newPos = SPositionFromLocation(Point::FromInts(lastXChosen - xOffset, vs.lineHeight * caretPolicies.y.slop),
			false, false, UserVirtualSpace());

	} else if (stuttered && (direction > 0 && currentLine < bottomStutterLine)) {
		topLineNew = topLine;
		newPos = SPositionFromLocation(Point::FromInts(lastXChosen - xOffset, vs.lineHeight * static_cast<int>(LinesToScroll() - caretPolicies.y.slop)),
			false, false, UserVirtualSpace());

	} else {
		const Point pt = LocationFromPosition(sel.MainCaret());

		topLineNew = std::clamp<Sci::Line>(
		            topLine + direction * LinesToScroll(), 0, MaxScrollPos());
		newPos = SPositionFromLocation(
			Point::FromInts(lastXChosen - xOffset, static_cast<int>(pt.y) +
				direction * (vs.lineHeight * static_cast<int>(LinesToScroll()))),
			false, false, UserVirtualSpace());
	}

	if (topLineNew != topLine) {
		SetTopLine(topLineNew);
		MovePositionTo(newPos, selt);
		SetVerticalScrollPos();
		Redraw();
	} else {
		MovePositionTo(newPos, selt);
	}
}

void Editor::ChangeCaseOfSelection(CaseMapping caseMapping) {
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		SelectionRange current = sel.Range(r);
		SelectionRange currentNoVS = current;
		currentNoVS.ClearVirtualSpace();
		const size_t rangeBytes = currentNoVS.Length();
		if (rangeBytes > 0 && !RangeContainsProtected(currentNoVS)) {
			std::string sText = RangeText(currentNoVS.Start().Position(), currentNoVS.End().Position());

			std::string sMapped = CaseMapString(sText, caseMapping);

			if (sMapped != sText) {
				size_t firstDifference = 0;
				while (sMapped[firstDifference] == sText[firstDifference])
					firstDifference++;
				size_t lastDifferenceText = sText.size() - 1;
				size_t lastDifferenceMapped = sMapped.size() - 1;
				while (sMapped[lastDifferenceMapped] == sText[lastDifferenceText]) {
					lastDifferenceText--;
					lastDifferenceMapped--;
				}
				const size_t endDifferenceText = sText.size() - 1 - lastDifferenceText;
				pdoc->DeleteChars(
					currentNoVS.Start().Position() + firstDifference,
					rangeBytes - firstDifference - endDifferenceText);
				const Sci::Position lengthChange = lastDifferenceMapped - firstDifference + 1;
				const Sci::Position lengthInserted = pdoc->InsertString(
					currentNoVS.Start().Position() + firstDifference,
					sMapped.c_str() + firstDifference,
					lengthChange);
				// Automatic movement changes selection so reset to exactly the same as it was.
				const Sci::Position diffSizes = sMapped.size() - sText.size() + lengthInserted - lengthChange;
				if (diffSizes != 0) {
					if (current.anchor > current.caret)
						current.anchor.Add(diffSizes);
					else
						current.caret.Add(diffSizes);
				}
				sel.Range(r) = current;
			}
		}
	}
}

void Editor::LineDelete() {
	const Sci::Line line = pdoc->SciLineFromPosition(sel.MainCaret());
	const Sci::Position start = pdoc->LineStart(line);
	const Sci::Position end = pdoc->LineStart(line + 1);
	pdoc->DeleteChars(start, end - start);
}

void Editor::LineTranspose() {
	const Sci::Line line = pdoc->SciLineFromPosition(sel.MainCaret());
	if (line > 0) {
		UndoGroup ug(pdoc);

		const Sci::Position startPrevious = pdoc->LineStart(line - 1);
		const std::string linePrevious = RangeText(startPrevious, pdoc->LineEnd(line - 1));

		Sci::Position startCurrent = pdoc->LineStart(line);
		const std::string lineCurrent = RangeText(startCurrent, pdoc->LineEnd(line));

		pdoc->DeleteChars(startCurrent, lineCurrent.length());
		pdoc->DeleteChars(startPrevious, linePrevious.length());
		startCurrent -= linePrevious.length();

		startCurrent += pdoc->InsertString(startPrevious, lineCurrent);
		pdoc->InsertString(startCurrent, linePrevious);
		// Move caret to start of current line
		MovePositionTo(SelectionPosition(startCurrent));
	}
}

void Editor::LineReverse() {
	const Sci::Line lineStart =
		pdoc->SciLineFromPosition(sel.RangeMain().Start().Position());
	const Sci::Line lineEnd =
		pdoc->SciLineFromPosition(sel.RangeMain().End().Position()-1);
	const Sci::Line lineDiff = lineEnd - lineStart;
	if (lineDiff <= 0)
		return;
	UndoGroup ug(pdoc);
	for (Sci::Line i=(lineDiff+1)/2-1; i>=0; --i) {
		const Sci::Line lineNum2 = lineEnd - i;
		const Sci::Line lineNum1 = lineStart + i;
		Sci::Position lineStart2 = pdoc->LineStart(lineNum2);
		const Sci::Position lineStart1 = pdoc->LineStart(lineNum1);
		const std::string line2 = RangeText(lineStart2, pdoc->LineEnd(lineNum2));
		const std::string line1 = RangeText(lineStart1, pdoc->LineEnd(lineNum1));
		const Sci::Position lineLen2 = line2.length();
		const Sci::Position lineLen1 = line1.length();
		pdoc->DeleteChars(lineStart2, lineLen2);
		pdoc->DeleteChars(lineStart1, lineLen1);
		lineStart2 -= lineLen1;
		pdoc->InsertString(lineStart2, line1);
		pdoc->InsertString(lineStart1, line2);
	}
	// Wholly select all affected lines
	sel.RangeMain() = SelectionRange(pdoc->LineStart(lineStart),
		pdoc->LineStart(lineEnd+1));
}

void Editor::Duplicate(bool forLine) {
	if (sel.Empty()) {
		forLine = true;
	}
	UndoGroup ug(pdoc);
	std::string_view eol;
	if (forLine) {
		eol = pdoc->EOLString();
	}
	for (size_t r=0; r<sel.Count(); r++) {
		SelectionPosition start = sel.Range(r).Start();
		SelectionPosition end = sel.Range(r).End();
		if (forLine) {
			const Sci::Line line = pdoc->SciLineFromPosition(sel.Range(r).caret.Position());
			start = SelectionPosition(pdoc->LineStart(line));
			end = SelectionPosition(pdoc->LineEnd(line));
		}
		std::string text = RangeText(start.Position(), end.Position());
		Sci::Position lengthInserted = 0;
		if (forLine)
			lengthInserted = pdoc->InsertString(end.Position(), eol);
		pdoc->InsertString(end.Position() + lengthInserted, text);
	}
	if (sel.Count() && sel.IsRectangular()) {
		SelectionPosition last = sel.Last();
		if (forLine) {
			const Sci::Line line = pdoc->SciLineFromPosition(last.Position());
			last = SelectionPosition(last.Position() +
				pdoc->LineStart(line+1) - pdoc->LineStart(line));
		}
		if (sel.Rectangular().anchor > sel.Rectangular().caret)
			sel.Rectangular().anchor = last;
		else
			sel.Rectangular().caret = last;
		SetRectangularRange();
	}
}

void Editor::CancelModes() {
	sel.SetMoveExtends(false);
}

void Editor::NewLine() {
	InvalidateWholeSelection();
	if (sel.IsRectangular() || !additionalSelectionTyping) {
		// Remove non-main ranges
		sel.DropAdditionalRanges();
	}

	UndoGroup ug(pdoc, !sel.Empty() || (sel.Count() > 1));

	// Clear each range
	if (!sel.Empty()) {
		ClearSelection();
	}

	// Insert each line end
	size_t countInsertions = 0;
	const std::string_view eol = pdoc->EOLString();
	for (size_t r = 0; r < sel.Count(); r++) {
		sel.Range(r).ClearVirtualSpace();
		const Sci::Position positionInsert = sel.Range(r).caret.Position();
		const Sci::Position insertLength = pdoc->InsertString(positionInsert, eol);
		if (insertLength > 0) {
			sel.Range(r) = SelectionRange(positionInsert + insertLength);
			countInsertions++;
		}
	}

	// Perform notifications after all the changes as the application may change the
	// selections in response to the characters.
	for (size_t i = 0; i < countInsertions; i++) {
		for (const char ch : eol) {
			NotifyChar(ch, CharacterSource::DirectInput);
			if (recordingMacro) {
				const char txt[2] = { ch, '\0' };
				NotifyMacroRecord(Message::ReplaceSel, 0, reinterpret_cast<sptr_t>(txt));
			}
		}
	}

	SetLastXChosen();
	SetScrollBars();
	EnsureCaretVisible();
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

SelectionPosition Editor::PositionUpOrDown(SelectionPosition spStart, int direction, int lastX) {
	const Point pt = LocationFromPosition(spStart);
	int skipLines = 0;

	if (vs.annotationVisible != AnnotationVisible::Hidden) {
		const Sci::Line lineDoc = pdoc->SciLineFromPosition(spStart.Position());
		const Point ptStartLine = LocationFromPosition(pdoc->LineStart(lineDoc));
		const int subLine = static_cast<int>(pt.y - ptStartLine.y) / vs.lineHeight;

		if (direction < 0 && subLine == 0) {
			const Sci::Line lineDisplay = pcs->DisplayFromDoc(lineDoc);
			if (lineDisplay > 0) {
				skipLines = pdoc->AnnotationLines(pcs->DocFromDisplay(lineDisplay - 1));
			}
		} else if (direction > 0 && subLine >= (pcs->GetHeight(lineDoc) - 1 - pdoc->AnnotationLines(lineDoc))) {
			skipLines = pdoc->AnnotationLines(lineDoc);
		}
	}

	const Sci::Line newY = static_cast<Sci::Line>(pt.y) + (1 + skipLines) * direction * vs.lineHeight;
	if (lastX < 0) {
		lastX = static_cast<int>(pt.x) + xOffset;
	}
	SelectionPosition posNew = SPositionFromLocation(
		Point::FromInts(lastX - xOffset, static_cast<int>(newY)), false, false, UserVirtualSpace());

	if (direction < 0) {
		// Line wrapping may lead to a location on the same line, so
		// seek back if that is the case.
		Point ptNew = LocationFromPosition(posNew.Position());
		while ((posNew.Position() > 0) && (pt.y == ptNew.y)) {
			posNew.Add(-1);
			posNew.SetVirtualSpace(0);
			ptNew = LocationFromPosition(posNew.Position());
		}
	} else if (direction > 0 && posNew.Position() != pdoc->Length()) {
		// There is an equivalent case when moving down which skips
		// over a line.
		Point ptNew = LocationFromPosition(posNew.Position());
		while ((posNew.Position() > spStart.Position()) && (ptNew.y > newY)) {
			posNew.Add(-1);
			posNew.SetVirtualSpace(0);
			ptNew = LocationFromPosition(posNew.Position());
		}
	}
	return posNew;
}

void Editor::CursorUpOrDown(int direction, Selection::SelTypes selt) {
	if ((selt == Selection::SelTypes::none) && sel.MoveExtends()) {
		selt = !sel.IsRectangular() ? Selection::SelTypes::stream : Selection::SelTypes::rectangle;
	}
	SelectionPosition caretToUse = sel.RangeMain().caret;
	if (sel.IsRectangular()) {
		if (selt ==  Selection::SelTypes::none) {
			caretToUse = (direction > 0) ? sel.Limits().end : sel.Limits().start;
		} else {
			caretToUse = sel.Rectangular().caret;
		}
	}
	if (selt == Selection::SelTypes::rectangle) {
		const SelectionRange rangeBase = sel.IsRectangular() ? sel.Rectangular() : sel.RangeMain();
		if (!sel.IsRectangular()) {
			InvalidateWholeSelection();
			sel.DropAdditionalRanges();
		}
		const SelectionPosition posNew = MovePositionSoVisible(
			PositionUpOrDown(caretToUse, direction, lastXChosen), direction);
		sel.selType = Selection::SelTypes::rectangle;
		sel.Rectangular() = SelectionRange(posNew, rangeBase.anchor);
		SetRectangularRange();
		MovedCaret(posNew, caretToUse, true, caretPolicies);
	} else if (sel.selType == Selection::SelTypes::lines && sel.MoveExtends()) {
		// Calculate new caret position and call SetSelection(), which will ensure whole lines are selected.
		const SelectionPosition posNew = MovePositionSoVisible(
			PositionUpOrDown(caretToUse, direction, -1), direction);
		SetSelection(posNew, sel.RangeMain().anchor);
	} else {
		InvalidateWholeSelection();
		if (!additionalSelectionTyping || (sel.IsRectangular())) {
			sel.DropAdditionalRanges();
		}
		sel.selType = Selection::SelTypes::stream;
		for (size_t r = 0; r < sel.Count(); r++) {
			const int lastX = (r == sel.Main()) ? lastXChosen : -1;
			const SelectionPosition spCaretNow = sel.Range(r).caret;
			const SelectionPosition posNew = MovePositionSoVisible(
				PositionUpOrDown(spCaretNow, direction, lastX), direction);
			sel.Range(r) = selt == Selection::SelTypes::stream ?
				SelectionRange(posNew, sel.Range(r).anchor) : SelectionRange(posNew);
		}
		sel.RemoveDuplicates();
		MovedCaret(sel.RangeMain().caret, caretToUse, true, caretPolicies);
	}
}

void Editor::ParaUpOrDown(int direction, Selection::SelTypes selt) {
	Sci::Line lineDoc;
	const Sci::Position savedPos = sel.MainCaret();
	do {
		MovePositionTo(SelectionPosition(direction > 0 ? pdoc->ParaDown(sel.MainCaret()) : pdoc->ParaUp(sel.MainCaret())), selt);
		lineDoc = pdoc->SciLineFromPosition(sel.MainCaret());
		if (direction > 0) {
			if (sel.MainCaret() >= pdoc->Length() && !pcs->GetVisible(lineDoc)) {
				if (selt == Selection::SelTypes::none) {
					MovePositionTo(SelectionPosition(pdoc->LineEndPosition(savedPos)));
				}
				break;
			}
		}
	} while (!pcs->GetVisible(lineDoc));
}

Range Editor::RangeDisplayLine(Sci::Line lineVisible) {
	RefreshStyleData();
	AutoSurface surface(this);
	return view.RangeDisplayLine(surface, *this, lineVisible, vs);
}

Sci::Position Editor::StartEndDisplayLine(Sci::Position pos, bool start) {
	RefreshStyleData();
	AutoSurface surface(this);
	const Sci::Position posRet = view.StartEndDisplayLine(surface, *this, pos, start, vs);
	if (posRet == Sci::invalidPosition) {
		return pos;
	} else {
		return posRet;
	}
}

namespace {

constexpr short HighShortFromWParam(uptr_t x) {
	return static_cast<short>(x >> 16);
}

constexpr short LowShortFromWParam(uptr_t x) {
	return static_cast<short>(x & 0xffff);
}

constexpr Message WithExtends(Message iMessage) noexcept {
	switch (iMessage) {
	case Message::CharLeft: return Message::CharLeftExtend;
	case Message::CharRight: return Message::CharRightExtend;

	case Message::WordLeft: return Message::WordLeftExtend;
	case Message::WordRight: return Message::WordRightExtend;
	case Message::WordLeftEnd: return Message::WordLeftEndExtend;
	case Message::WordRightEnd: return Message::WordRightEndExtend;
	case Message::WordPartLeft: return Message::WordPartLeftExtend;
	case Message::WordPartRight: return Message::WordPartRightExtend;

	case Message::Home: return Message::HomeExtend;
	case Message::HomeDisplay: return Message::HomeDisplayExtend;
	case Message::HomeWrap: return Message::HomeWrapExtend;
	case Message::VCHome: return Message::VCHomeExtend;
	case Message::VCHomeDisplay: return Message::VCHomeDisplayExtend;
	case Message::VCHomeWrap: return Message::VCHomeWrapExtend;

	case Message::LineEnd: return Message::LineEndExtend;
	case Message::LineEndDisplay: return Message::LineEndDisplayExtend;
	case Message::LineEndWrap: return Message::LineEndWrapExtend;

	default:	return iMessage;
	}
}

constexpr int NaturalDirection(Message iMessage) noexcept {
	switch (iMessage) {
	case Message::CharLeft:
	case Message::CharLeftExtend:
	case Message::CharLeftRectExtend:
	case Message::WordLeft:
	case Message::WordLeftExtend:
	case Message::WordLeftEnd:
	case Message::WordLeftEndExtend:
	case Message::WordPartLeft:
	case Message::WordPartLeftExtend:
	case Message::Home:
	case Message::HomeExtend:
	case Message::HomeDisplay:
	case Message::HomeDisplayExtend:
	case Message::HomeWrap:
	case Message::HomeWrapExtend:
		// VC_HOME* mostly goes back
	case Message::VCHome:
	case Message::VCHomeExtend:
	case Message::VCHomeDisplay:
	case Message::VCHomeDisplayExtend:
	case Message::VCHomeWrap:
	case Message::VCHomeWrapExtend:
		return -1;

	default:
		return 1;
	}
}

constexpr bool IsRectExtend(Message iMessage, bool isRectMoveExtends) noexcept {
	switch (iMessage) {
	case Message::CharLeftRectExtend:
	case Message::CharRightRectExtend:
	case Message::HomeRectExtend:
	case Message::VCHomeRectExtend:
	case Message::LineEndRectExtend:
		return true;
	default:
		if (isRectMoveExtends) {
			// Handle Message::SetSelectionMode(SelectionMode::Rectangle) and subsequent movements.
			switch (iMessage) {
			case Message::CharLeftExtend:
			case Message::CharRightExtend:
			case Message::HomeExtend:
			case Message::VCHomeExtend:
			case Message::LineEndExtend:
				return true;
			default:
				return false;
			}
		}
		return false;
	}
}

}

Sci::Position Editor::HomeWrapPosition(Sci::Position position) {
	const Sci::Position viewLineStart = StartEndDisplayLine(position, true);
	const Sci::Position homePos = MovePositionSoVisible(viewLineStart, -1).Position();
	if (position <= homePos)
		return pdoc->LineStartPosition(position);
	return homePos;
}

Sci::Position Editor::VCHomeDisplayPosition(Sci::Position position) {
	const Sci::Position homePos = pdoc->VCHomePosition(position);
	const Sci::Position viewLineStart = StartEndDisplayLine(position, true);
	if (viewLineStart > homePos)
		return viewLineStart;
	else
		return homePos;
}

Sci::Position Editor::VCHomeWrapPosition(Sci::Position position) {
	const Sci::Position homePos = pdoc->VCHomePosition(position);
	const Sci::Position viewLineStart = StartEndDisplayLine(position, true);
	if ((viewLineStart < position) && (viewLineStart > homePos))
		return viewLineStart;
	else
		return homePos;
}

Sci::Position Editor::LineEndWrapPosition(Sci::Position position) {
	const Sci::Position endPos = StartEndDisplayLine(position, false);
	const Sci::Position realEndPos = pdoc->LineEndPosition(position);
	if (endPos > realEndPos      // if moved past visible EOLs
		|| position >= endPos) // if at end of display line already
		return realEndPos;
	else
		return endPos;
}

SelectionPosition Editor::PositionMove(Message iMessage, SelectionPosition spCaret) {
	switch (iMessage) {
	case Message::CharLeft:
	case Message::CharLeftExtend:
		if (spCaret.VirtualSpace()) {
			spCaret.AddVirtualSpace(-1);
		} else if (!FlagSet(virtualSpaceOptions, VirtualSpace::NoWrapLineStart) || pdoc->GetColumn(spCaret.Position()) > 0) {
			spCaret.Add(-1);
		}
		return spCaret;
	case Message::CharRight:
	case Message::CharRightExtend:
		if (FlagSet(virtualSpaceOptions, VirtualSpace::UserAccessible) && pdoc->IsLineEndPosition(spCaret.Position())) {
			spCaret.AddVirtualSpace(1);
		} else {
			spCaret.Add(1);
		}
		return spCaret;
	case Message::WordLeft:
	case Message::WordLeftExtend:
		return SelectionPosition(pdoc->NextWordStart(spCaret.Position(), -1));
	case Message::WordRight:
	case Message::WordRightExtend:
		return SelectionPosition(pdoc->NextWordStart(spCaret.Position(), 1));
	case Message::WordLeftEnd:
	case Message::WordLeftEndExtend:
		return SelectionPosition(pdoc->NextWordEnd(spCaret.Position(), -1));
	case Message::WordRightEnd:
	case Message::WordRightEndExtend:
		return SelectionPosition(pdoc->NextWordEnd(spCaret.Position(), 1));
	case Message::WordPartLeft:
	case Message::WordPartLeftExtend:
		return SelectionPosition(pdoc->WordPartLeft(spCaret.Position()));
	case Message::WordPartRight:
	case Message::WordPartRightExtend:
		return SelectionPosition(pdoc->WordPartRight(spCaret.Position()));
	case Message::Home:
	case Message::HomeExtend:
		return SelectionPosition(pdoc->LineStartPosition(spCaret.Position()));
	case Message::HomeDisplay:
	case Message::HomeDisplayExtend:
		return SelectionPosition(StartEndDisplayLine(spCaret.Position(), true));
	case Message::HomeWrap:
	case Message::HomeWrapExtend:
		return SelectionPosition(HomeWrapPosition(spCaret.Position()));
	case Message::VCHome:
	case Message::VCHomeExtend:
		// VCHome alternates between beginning of line and beginning of text so may move back or forwards
		return SelectionPosition(pdoc->VCHomePosition(spCaret.Position()));
	case Message::VCHomeDisplay:
	case Message::VCHomeDisplayExtend:
		return SelectionPosition(VCHomeDisplayPosition(spCaret.Position()));
	case Message::VCHomeWrap:
	case Message::VCHomeWrapExtend:
		return SelectionPosition(VCHomeWrapPosition(spCaret.Position()));
	case Message::LineEnd:
	case Message::LineEndExtend:
		return SelectionPosition(pdoc->LineEndPosition(spCaret.Position()));
	case Message::LineEndDisplay:
	case Message::LineEndDisplayExtend:
		return SelectionPosition(StartEndDisplayLine(spCaret.Position(), false));
	case Message::LineEndWrap:
	case Message::LineEndWrapExtend:
		return SelectionPosition(LineEndWrapPosition(spCaret.Position()));

	default:
		break;
	}
	// Above switch should be exhaustive so this will never be reached.
	PLATFORM_ASSERT(false);
	return spCaret;
}

SelectionRange Editor::SelectionMove(Scintilla::Message iMessage, size_t r) {
	const SelectionPosition spCaretStart = sel.Range(r).caret;
	const SelectionPosition spCaretMoved = PositionMove(iMessage, spCaretStart);

	const int directionMove = (spCaretMoved < spCaretStart) ? -1 : 1;
	const SelectionPosition spCaret = MovePositionSoVisible(spCaretMoved, directionMove);

	// Handle move versus extend, and special behaviour for non-empty left/right
	switch (iMessage) {
	case Message::CharLeft:
	case Message::CharRight:
		if (sel.Range(r).Empty()) {
			return SelectionRange(spCaret);
		}
		if (iMessage == Message::CharLeft) {
			return SelectionRange(sel.Range(r).Start());
		}
		return SelectionRange(sel.Range(r).End());

	case Message::WordLeft:
	case Message::WordRight:
	case Message::WordLeftEnd:
	case Message::WordRightEnd:
	case Message::WordPartLeft:
	case Message::WordPartRight:
	case Message::Home:
	case Message::HomeDisplay:
	case Message::HomeWrap:
	case Message::VCHome:
	case Message::VCHomeDisplay:
	case Message::VCHomeWrap:
	case Message::LineEnd:
	case Message::LineEndDisplay:
	case Message::LineEndWrap:
		return SelectionRange(spCaret);

	default:
		break;
	}

	// All remaining cases are *Extend
	const SelectionRange rangeNew = SelectionRange(spCaret, sel.Range(r).anchor);
	sel.TrimOtherSelections(r, rangeNew);
	return rangeNew;
}

int Editor::HorizontalMove(Message iMessage) {
	if (sel.selType == Selection::SelTypes::lines) {
		return 0; // horizontal moves with line selection have no effect
	}
	if (sel.MoveExtends()) {
		iMessage = WithExtends(iMessage);
	}

	if (!multipleSelection && !sel.IsRectangular()) {
		// Simplify selection down to 1
		sel.SetSelection(sel.RangeMain());
	}

	// Invalidate each of the current selections
	InvalidateWholeSelection();

	if (IsRectExtend(iMessage, sel.IsRectangular() && sel.MoveExtends())) {
		const SelectionRange rangeBase = sel.IsRectangular() ? sel.Rectangular() : sel.RangeMain();
		if (!sel.IsRectangular()) {
			sel.DropAdditionalRanges();
		}
		// Will change to rectangular if not currently rectangular
		SelectionPosition spCaret = rangeBase.caret;
		switch (iMessage) {
		case Message::CharLeftRectExtend:
		case Message::CharLeftExtend: // only when sel.IsRectangular() && sel.MoveExtends()
			if (pdoc->IsLineEndPosition(spCaret.Position()) && spCaret.VirtualSpace()) {
				spCaret.SetVirtualSpace(spCaret.VirtualSpace() - 1);
			} else if (!FlagSet(virtualSpaceOptions, VirtualSpace::NoWrapLineStart) || pdoc->GetColumn(spCaret.Position()) > 0) {
				spCaret = SelectionPosition(spCaret.Position() - 1);
			}
			break;
		case Message::CharRightRectExtend:
		case Message::CharRightExtend: // only when sel.IsRectangular() && sel.MoveExtends()
			if (FlagSet(virtualSpaceOptions, VirtualSpace::RectangularSelection) && pdoc->IsLineEndPosition(sel.MainCaret())) {
				spCaret.SetVirtualSpace(spCaret.VirtualSpace() + 1);
			} else {
				spCaret = SelectionPosition(spCaret.Position() + 1);
			}
			break;
		case Message::HomeRectExtend:
		case Message::HomeExtend: // only when sel.IsRectangular() && sel.MoveExtends()
			spCaret = SelectionPosition(pdoc->LineStartPosition(spCaret.Position()));
			break;
		case Message::VCHomeRectExtend:
		case Message::VCHomeExtend: // only when sel.IsRectangular() && sel.MoveExtends()
			spCaret = SelectionPosition(pdoc->VCHomePosition(spCaret.Position()));
			break;
		case Message::LineEndRectExtend:
		case Message::LineEndExtend: // only when sel.IsRectangular() && sel.MoveExtends()
			spCaret = SelectionPosition(pdoc->LineEndPosition(spCaret.Position()));
			break;
		default:
			break;
		}
		const int directionMove = (spCaret < rangeBase.caret) ? -1 : 1;
		spCaret = MovePositionSoVisible(spCaret, directionMove);
		sel.selType = Selection::SelTypes::rectangle;
		sel.Rectangular() = SelectionRange(spCaret, rangeBase.anchor);
		SetRectangularRange();
	} else if (sel.IsRectangular()) {
		// Not a rectangular extension so switch to stream.
		SelectionPosition selAtLimit = (NaturalDirection(iMessage) > 0) ? sel.Limits().end : sel.Limits().start;
		switch (iMessage) {
		case Message::Home:
			selAtLimit = SelectionPosition(pdoc->LineStartPosition(selAtLimit.Position()));
			break;
		case Message::VCHome:
			selAtLimit = SelectionPosition(pdoc->VCHomePosition(selAtLimit.Position()));
			break;
		case Message::LineEnd:
			selAtLimit = SelectionPosition(pdoc->LineEndPosition(selAtLimit.Position()));
			break;
		default:
			break;
		}
		sel.selType = Selection::SelTypes::stream;
		sel.SetSelection(SelectionRange(selAtLimit));
	} else {
		if (!additionalSelectionTyping) {
			InvalidateWholeSelection();
			sel.DropAdditionalRanges();
		}
		for (size_t r = 0; r < sel.Count(); r++) {
			sel.Range(r) = SelectionMove(iMessage, r);
		}
	}

	sel.RemoveDuplicates();

	MovedCaret(sel.RangeMain().caret, SelectionPosition(Sci::invalidPosition), true, caretPolicies);

	// Invalidate the new state of the selection
	InvalidateWholeSelection();

	SetLastXChosen();
	// Need the line moving and so forth from MovePositionTo
	return 0;
}

int Editor::DelWordOrLine(Message iMessage) {
	// Virtual space may be realised for Message::DelWordRight or Message::DelWordRightEnd
	// which means 2 actions so wrap in an undo group.

	// Rightwards and leftwards deletions differ in treatment of virtual space.
	// Clear virtual space for leftwards, realise for rightwards.
	const bool leftwards = (iMessage == Message::DelWordLeft) || (iMessage == Message::DelLineLeft);

	if (!additionalSelectionTyping) {
		InvalidateWholeSelection();
		sel.DropAdditionalRanges();
	}

	UndoGroup ug0(pdoc, (sel.Count() > 1) || !leftwards);

	for (size_t r = 0; r < sel.Count(); r++) {
		if (leftwards) {
			// Delete to the left so first clear the virtual space.
			sel.Range(r).ClearVirtualSpace();
		} else {
			// Delete to the right so first realise the virtual space.
			sel.Range(r) = SelectionRange(
				RealizeVirtualSpace(sel.Range(r).caret));
		}

		Range rangeDelete;
		switch (iMessage) {
		case Message::DelWordLeft:
			rangeDelete = Range(
				pdoc->NextWordStart(sel.Range(r).caret.Position(), -1),
				sel.Range(r).caret.Position());
			break;
		case Message::DelWordRight:
			rangeDelete = Range(
				sel.Range(r).caret.Position(),
				pdoc->NextWordStart(sel.Range(r).caret.Position(), 1));
			break;
		case Message::DelWordRightEnd:
			rangeDelete = Range(
				sel.Range(r).caret.Position(),
				pdoc->NextWordEnd(sel.Range(r).caret.Position(), 1));
			break;
		case Message::DelLineLeft:
			rangeDelete = Range(
				pdoc->LineStartPosition(sel.Range(r).caret.Position()),
				sel.Range(r).caret.Position());
			break;
		case Message::DelLineRight:
			rangeDelete = Range(
				sel.Range(r).caret.Position(),
				pdoc->LineEndPosition(sel.Range(r).caret.Position()));
			break;
		default:
			break;
		}
		if (!RangeContainsProtected(rangeDelete.start, rangeDelete.end)) {
			pdoc->DeleteChars(rangeDelete.start, rangeDelete.end - rangeDelete.start);
		}
	}

	// May need something stronger here: can selections overlap at this point?
	sel.RemoveDuplicates();

	MovedCaret(sel.RangeMain().caret, SelectionPosition(Sci::invalidPosition), true, caretPolicies);

	// Invalidate the new state of the selection
	InvalidateWholeSelection();

	SetLastXChosen();
	return 0;
}

int Editor::KeyCommand(Message iMessage) {
	switch (iMessage) {
	case Message::LineDown:
		CursorUpOrDown(1, Selection::SelTypes::none);
		break;
	case Message::LineDownExtend:
		CursorUpOrDown(1, Selection::SelTypes::stream);
		break;
	case Message::LineDownRectExtend:
		CursorUpOrDown(1, Selection::SelTypes::rectangle);
		break;
	case Message::ParaDown:
		ParaUpOrDown(1, Selection::SelTypes::none);
		break;
	case Message::ParaDownExtend:
		ParaUpOrDown(1, Selection::SelTypes::stream);
		break;
	case Message::LineScrollDown:
		ScrollTo(topLine + 1);
		MoveCaretInsideView(false);
		break;
	case Message::LineUp:
		CursorUpOrDown(-1, Selection::SelTypes::none);
		break;
	case Message::LineUpExtend:
		CursorUpOrDown(-1, Selection::SelTypes::stream);
		break;
	case Message::LineUpRectExtend:
		CursorUpOrDown(-1, Selection::SelTypes::rectangle);
		break;
	case Message::ParaUp:
		ParaUpOrDown(-1, Selection::SelTypes::none);
		break;
	case Message::ParaUpExtend:
		ParaUpOrDown(-1, Selection::SelTypes::stream);
		break;
	case Message::LineScrollUp:
		ScrollTo(topLine - 1);
		MoveCaretInsideView(false);
		break;

	case Message::CharLeft:
	case Message::CharLeftExtend:
	case Message::CharLeftRectExtend:
	case Message::CharRight:
	case Message::CharRightExtend:
	case Message::CharRightRectExtend:
	case Message::WordLeft:
	case Message::WordLeftExtend:
	case Message::WordRight:
	case Message::WordRightExtend:
	case Message::WordLeftEnd:
	case Message::WordLeftEndExtend:
	case Message::WordRightEnd:
	case Message::WordRightEndExtend:
	case Message::WordPartLeft:
	case Message::WordPartLeftExtend:
	case Message::WordPartRight:
	case Message::WordPartRightExtend:
	case Message::Home:
	case Message::HomeExtend:
	case Message::HomeRectExtend:
	case Message::HomeDisplay:
	case Message::HomeDisplayExtend:
	case Message::HomeWrap:
	case Message::HomeWrapExtend:
	case Message::VCHome:
	case Message::VCHomeExtend:
	case Message::VCHomeRectExtend:
	case Message::VCHomeDisplay:
	case Message::VCHomeDisplayExtend:
	case Message::VCHomeWrap:
	case Message::VCHomeWrapExtend:
	case Message::LineEnd:
	case Message::LineEndExtend:
	case Message::LineEndRectExtend:
	case Message::LineEndDisplay:
	case Message::LineEndDisplayExtend:
	case Message::LineEndWrap:
	case Message::LineEndWrapExtend:
		return HorizontalMove(iMessage);

	case Message::DocumentStart:
		MovePositionTo(0);
		SetLastXChosen();
		break;
	case Message::DocumentStartExtend:
		MovePositionTo(0, Selection::SelTypes::stream);
		SetLastXChosen();
		break;
	case Message::DocumentEnd:
		MovePositionTo(pdoc->Length());
		SetLastXChosen();
		break;
	case Message::DocumentEndExtend:
		MovePositionTo(pdoc->Length(), Selection::SelTypes::stream);
		SetLastXChosen();
		break;
	case Message::StutteredPageUp:
		PageMove(-1, Selection::SelTypes::none, true);
		break;
	case Message::StutteredPageUpExtend:
		PageMove(-1, Selection::SelTypes::stream, true);
		break;
	case Message::StutteredPageDown:
		PageMove(1, Selection::SelTypes::none, true);
		break;
	case Message::StutteredPageDownExtend:
		PageMove(1, Selection::SelTypes::stream, true);
		break;
	case Message::PageUp:
		PageMove(-1);
		break;
	case Message::PageUpExtend:
		PageMove(-1, Selection::SelTypes::stream);
		break;
	case Message::PageUpRectExtend:
		PageMove(-1, Selection::SelTypes::rectangle);
		break;
	case Message::PageDown:
		PageMove(1);
		break;
	case Message::PageDownExtend:
		PageMove(1, Selection::SelTypes::stream);
		break;
	case Message::PageDownRectExtend:
		PageMove(1, Selection::SelTypes::rectangle);
		break;
	case Message::EditToggleOvertype:
		inOverstrike = !inOverstrike;
		ContainerNeedsUpdate(Update::Selection);
		ShowCaretAtCurrentPosition();
		SetIdle(true);
		break;
	case Message::Cancel:            	// Cancel any modes - handled in subclass
		// Also unselect text
		CancelModes();
		if ((sel.Count() > 1) && !sel.IsRectangular()) {
			// Drop additional selections
			InvalidateWholeSelection();
			sel.DropAdditionalRanges();
		}
		break;
	case Message::DeleteBack:
		DelCharBack(true);
		if ((caretSticky == CaretSticky::Off) || (caretSticky == CaretSticky::WhiteSpace)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case Message::DeleteBackNotLine:
		DelCharBack(false);
		if ((caretSticky == CaretSticky::Off) || (caretSticky == CaretSticky::WhiteSpace)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case Message::Tab:
	case Message::LineIndent:
		Indent(true, iMessage == Message::LineIndent);
		if (caretSticky == CaretSticky::Off) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		ShowCaretAtCurrentPosition();		// Avoid blinking
		break;
	case Message::BackTab:
	case Message::LineDedent:
		Indent(false, iMessage == Message::LineDedent);
		if ((caretSticky == CaretSticky::Off) || (caretSticky == CaretSticky::WhiteSpace)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		ShowCaretAtCurrentPosition();		// Avoid blinking
		break;
	case Message::NewLine:
		NewLine();
		break;
	case Message::FormFeed:
		AddChar('\f');
		break;
	case Message::ZoomIn:
		if (vs.zoomLevel < 60) {
			vs.zoomLevel++;
			InvalidateStyleRedraw();
			NotifyZoom();
		}
		break;
	case Message::ZoomOut:
		if (vs.zoomLevel > -10) {
			vs.zoomLevel--;
			InvalidateStyleRedraw();
			NotifyZoom();
		}
		break;

	case Message::DelWordLeft:
	case Message::DelWordRight:
	case Message::DelWordRightEnd:
	case Message::DelLineLeft:
	case Message::DelLineRight:
		return DelWordOrLine(iMessage);

	case Message::LineCopy: {
			const Sci::Line lineStart = pdoc->SciLineFromPosition(SelectionStart().Position());
			const Sci::Line lineEnd = pdoc->SciLineFromPosition(SelectionEnd().Position());
			CopyRangeToClipboard(pdoc->LineStart(lineStart),
				pdoc->LineStart(lineEnd + 1));
		}
		break;
	case Message::LineCut: {
			const Sci::Line lineStart = pdoc->SciLineFromPosition(SelectionStart().Position());
			const Sci::Line lineEnd = pdoc->SciLineFromPosition(SelectionEnd().Position());
			const Sci::Position start = pdoc->LineStart(lineStart);
			const Sci::Position end = pdoc->LineStart(lineEnd + 1);
			SetSelection(start, end);
			Cut();
			SetLastXChosen();
		}
		break;
	case Message::LineDelete:
		LineDelete();
		break;
	case Message::LineTranspose:
		LineTranspose();
		break;
	case Message::LineReverse:
		LineReverse();
		break;
	case Message::LineDuplicate:
		Duplicate(true);
		break;
	case Message::SelectionDuplicate:
		Duplicate(false);
		break;
	case Message::LowerCase:
		ChangeCaseOfSelection(CaseMapping::lower);
		break;
	case Message::UpperCase:
		ChangeCaseOfSelection(CaseMapping::upper);
		break;
	case Message::ScrollToStart:
		ScrollTo(0);
		break;
	case Message::ScrollToEnd:
		ScrollTo(MaxScrollPos());
		break;
	default:
		break;
	}
	return 0;
}

int Editor::KeyDefault(Keys, KeyMod) {
	return 0;
}

int Editor::KeyDownWithModifiers(Keys key, KeyMod modifiers, bool *consumed) {
	DwellEnd(false);
	const Message msg = kmap.Find(key, modifiers);
	if (msg != static_cast<Message>(0)) {
		if (consumed)
			*consumed = true;
		return static_cast<int>(WndProc(msg, 0, 0));
	} else {
		if (consumed)
			*consumed = false;
		return KeyDefault(key, modifiers);
	}
}

void Editor::Indent(bool forwards, bool lineIndent) {
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		const Sci::Line lineOfAnchor =
			pdoc->SciLineFromPosition(sel.Range(r).anchor.Position());
		Sci::Position caretPosition = sel.Range(r).caret.Position();
		const Sci::Line lineCurrentPos = pdoc->SciLineFromPosition(caretPosition);
		if (lineOfAnchor == lineCurrentPos && !lineIndent) {
			if (forwards) {
				pdoc->DeleteChars(sel.Range(r).Start().Position(), sel.Range(r).Length());
				caretPosition = sel.Range(r).caret.Position();
				if (pdoc->GetColumn(caretPosition) <= pdoc->GetColumn(pdoc->GetLineIndentPosition(lineCurrentPos)) &&
						pdoc->tabIndents) {
					const int indentation = pdoc->GetLineIndentation(lineCurrentPos);
					const int indentationStep = pdoc->IndentSize();
					const Sci::Position posSelect = pdoc->SetLineIndentation(
						lineCurrentPos, indentation + indentationStep - indentation % indentationStep);
					sel.Range(r) = SelectionRange(posSelect);
				} else {
					if (pdoc->useTabs) {
						const Sci::Position lengthInserted = pdoc->InsertString(caretPosition, "\t", 1);
						sel.Range(r) = SelectionRange(caretPosition + lengthInserted);
					} else {
						int numSpaces = (pdoc->tabInChars) -
								(pdoc->GetColumn(caretPosition) % (pdoc->tabInChars));
						if (numSpaces < 1)
							numSpaces = pdoc->tabInChars;
						const std::string spaceText(numSpaces, ' ');
						const Sci::Position lengthInserted = pdoc->InsertString(caretPosition, spaceText);
						sel.Range(r) = SelectionRange(caretPosition + lengthInserted);
					}
				}
			} else {
				if (pdoc->GetColumn(caretPosition) <= pdoc->GetLineIndentation(lineCurrentPos) &&
						pdoc->tabIndents) {
					const int indentation = pdoc->GetLineIndentation(lineCurrentPos);
					const int indentationStep = pdoc->IndentSize();
					const Sci::Position posSelect = pdoc->SetLineIndentation(lineCurrentPos, indentation - indentationStep);
					sel.Range(r) = SelectionRange(posSelect);
				} else {
					Sci::Position newColumn = ((pdoc->GetColumn(caretPosition) - 1) / pdoc->tabInChars) *
							pdoc->tabInChars;
					if (newColumn < 0)
						newColumn = 0;
					Sci::Position newPos = caretPosition;
					while (pdoc->GetColumn(newPos) > newColumn)
						newPos--;
					sel.Range(r) = SelectionRange(newPos);
				}
			}
		} else {	// Multiline or LineIndent
			const Sci::Position anchorPosOnLine = sel.Range(r).anchor.Position() -
				pdoc->LineStart(lineOfAnchor);
			const Sci::Position currentPosPosOnLine = caretPosition -
				pdoc->LineStart(lineCurrentPos);
			// Multiple lines selected so indent / dedent
			const Sci::Line lineTopSel = std::min(lineOfAnchor, lineCurrentPos);
			Sci::Line lineBottomSel = std::max(lineOfAnchor, lineCurrentPos);
			if (pdoc->LineStart(lineBottomSel) == sel.Range(r).anchor.Position() || pdoc->LineStart(lineBottomSel) == caretPosition)
				lineBottomSel--;  	// If not selecting any characters on a line, do not indent
			pdoc->Indent(forwards, lineBottomSel, lineTopSel);
			if (lineOfAnchor < lineCurrentPos) {
				if (currentPosPosOnLine == 0)
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos),
						pdoc->LineStart(lineOfAnchor));
				else
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos + 1),
						pdoc->LineStart(lineOfAnchor));
			} else {
				if (anchorPosOnLine == 0)
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos),
						pdoc->LineStart(lineOfAnchor));
				else
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos),
						pdoc->LineStart(lineOfAnchor + 1));
			}
		}
	}
	ContainerNeedsUpdate(Update::Selection);
}

std::unique_ptr<CaseFolder> Editor::CaseFolderForEncoding() {
	// Simple default that only maps ASCII upper case to lower case.
	return std::make_unique<CaseFolderTable>();
}

/**
 * Search of a text in the document, in the given range.
 * @return The position of the found text, -1 if not found.
 */
Sci::Position Editor::FindText(
    uptr_t wParam,		///< Search modes : @c FindOption::MatchCase, @c FindOption::WholeWord,
    ///< @c FindOption::WordStart, @c FindOption::RegExp or @c FindOption::Posix.
    sptr_t lParam) {	///< @c Sci_TextToFind structure: The text to search for in the given range.

	TextToFind *ft = static_cast<TextToFind *>(PtrFromSPtr(lParam));
	Sci::Position lengthFound = strlen(ft->lpstrText);
	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	try {
		const Sci::Position pos = pdoc->FindText(
			static_cast<Sci::Position>(ft->chrg.cpMin),
			static_cast<Sci::Position>(ft->chrg.cpMax),
			ft->lpstrText,
			static_cast<FindOption>(wParam),
			&lengthFound);
		if (pos != -1) {
			ft->chrgText.cpMin = static_cast<Sci_PositionCR>(pos);
			ft->chrgText.cpMax = static_cast<Sci_PositionCR>(pos + lengthFound);
		}
		return pos;
	} catch (RegexError &) {
		errorStatus = Status::RegEx;
		return -1;
	}
}

/**
 * Search of a text in the document, in the given range.
 * @return The position of the found text, -1 if not found.
 */
Sci::Position Editor::FindTextFull(
    uptr_t wParam,		///< Search modes : @c FindOption::MatchCase, @c FindOption::WholeWord,
    ///< @c FindOption::WordStart, @c FindOption::RegExp or @c FindOption::Posix.
    sptr_t lParam) {	///< @c Sci_TextToFindFull structure: The text to search for in the given range.

	TextToFindFull *ft = static_cast<TextToFindFull *>(PtrFromSPtr(lParam));
	Sci::Position lengthFound = strlen(ft->lpstrText);
	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	try {
		const Sci::Position pos = pdoc->FindText(
			ft->chrg.cpMin,
			ft->chrg.cpMax,
			ft->lpstrText,
			static_cast<FindOption>(wParam),
			&lengthFound);
		if (pos != -1) {
			ft->chrgText.cpMin = pos;
			ft->chrgText.cpMax = pos + lengthFound;
		}
		return pos;
	} catch (RegexError &) {
		errorStatus = Status::RegEx;
		return -1;
	}
}

/**
 * Relocatable search support : Searches relative to current selection
 * point and sets the selection to the found text range with
 * each search.
 */
/**
 * Anchor following searches at current selection start: This allows
 * multiple incremental interactive searches to be macro recorded
 * while still setting the selection to found text so the find/select
 * operation is self-contained.
 */
void Editor::SearchAnchor() noexcept {
	searchAnchor = SelectionStart().Position();
}

/**
 * Find text from current search anchor: Must call @c SearchAnchor first.
 * Used for next text and previous text requests.
 * @return The position of the found text, -1 if not found.
 */
Sci::Position Editor::SearchText(
    Message iMessage,		///< Accepts both @c Message::SearchNext and @c Message::SearchPrev.
    uptr_t wParam,				///< Search modes : @c FindOption::MatchCase, @c FindOption::WholeWord,
    ///< @c FindOption::WordStart, @c FindOption::RegExp or @c FindOption::Posix.
    sptr_t lParam) {			///< The text to search for.

	const char *txt = ConstCharPtrFromSPtr(lParam);
	Sci::Position pos = Sci::invalidPosition;
	Sci::Position lengthFound = strlen(txt);
	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	try {
		if (iMessage == Message::SearchNext) {
			pos = pdoc->FindText(searchAnchor, pdoc->Length(), txt,
					static_cast<FindOption>(wParam),
					&lengthFound);
		} else {
			pos = pdoc->FindText(searchAnchor, 0, txt,
					static_cast<FindOption>(wParam),
					&lengthFound);
		}
	} catch (RegexError &) {
		errorStatus = Status::RegEx;
		return Sci::invalidPosition;
	}
	if (pos != Sci::invalidPosition) {
		SetSelection(pos, pos + lengthFound);
	}

	return pos;
}

std::string Editor::CaseMapString(const std::string &s, CaseMapping caseMapping) {
	std::string ret(s);
	for (char &ch : ret) {
		switch (caseMapping) {
			case CaseMapping::upper:
				ch = MakeUpperCase(ch);
				break;
			case CaseMapping::lower:
				ch = MakeLowerCase(ch);
				break;
			default:	// no action
				break;
		}
	}
	return ret;
}

/**
 * Search for text in the target range of the document.
 * @return The position of the found text, -1 if not found.
 */
Sci::Position Editor::SearchInTarget(const char *text, Sci::Position length) {
	Sci::Position lengthFound = length;

	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	try {
		const Sci::Position pos = pdoc->FindText(targetRange.start.Position(), targetRange.end.Position(), text,
				searchFlags,
				&lengthFound);
		if (pos != -1) {
			targetRange.start.SetPosition(pos);
			targetRange.end.SetPosition(pos + lengthFound);
		}
		return pos;
	} catch (RegexError &) {
		errorStatus = Status::RegEx;
		return -1;
	}
}

void Editor::GoToLine(Sci::Line lineNo) {
	if (lineNo > pdoc->LinesTotal())
		lineNo = pdoc->LinesTotal();
	if (lineNo < 0)
		lineNo = 0;
	SetEmptySelection(pdoc->LineStart(lineNo));
	ShowCaretAtCurrentPosition();
	EnsureCaretVisible();
}

namespace {

bool Close(Point pt1, Point pt2, Point threshold) noexcept {
	const Point ptDifference = pt2 - pt1;
	if (std::abs(ptDifference.x) > threshold.x)
		return false;
	if (std::abs(ptDifference.y) > threshold.y)
		return false;
	return true;
}

constexpr bool AllowVirtualSpace(VirtualSpace virtualSpaceOptions, bool rectangular) noexcept {
	return FlagSet(virtualSpaceOptions, (rectangular ? VirtualSpace::RectangularSelection : VirtualSpace::UserAccessible));
}

}

std::string Editor::RangeText(Sci::Position start, Sci::Position end) const {
	if (start < end) {
		const Sci::Position len = end - start;
		std::string ret(len, '\0');
		pdoc->GetCharRange(ret.data(), start, len);
		return ret;
	}
	return std::string();
}

bool Editor::CopyLineRange(SelectionText *ss, bool allowProtected) {
	const Sci::Line currentLine = pdoc->SciLineFromPosition(sel.MainCaret());
	const Sci::Position start = pdoc->LineStart(currentLine);
	const Sci::Position end = pdoc->LineEnd(currentLine);

	if (allowProtected || !RangeContainsProtected(start, end)) {
		std::string text = RangeText(start, end);
		text.append(pdoc->EOLString());
		ss->Copy(text, pdoc->dbcsCodePage,
			vs.styles[StyleDefault].characterSet, false, true);
		return true;
	} else {
		return false;
	}
}

void Editor::CopySelectionRange(SelectionText *ss, bool allowLineCopy) {
	if (sel.Empty()) {
		if (allowLineCopy) {
			CopyLineRange(ss);
		}
	} else {
		std::string text;
		std::vector<SelectionRange> rangesInOrder = sel.RangesCopy();
		if (sel.selType == Selection::SelTypes::rectangle)
			std::sort(rangesInOrder.begin(), rangesInOrder.end());
		const std::string_view separator = (rangesInOrder.size() > 1) ? pdoc->EOLString() : copySeparator;
		for (size_t part = 0; part < rangesInOrder.size(); part++) {
			text.append(RangeText(rangesInOrder[part].Start().Position(), rangesInOrder[part].End().Position()));
			if ((rangesInOrder.size() > 1) || (part < rangesInOrder.size() - 1)) {
				// Append unless simple selection or last part of multiple selection
				text.append(separator);
			}
		}
		ss->Copy(text, pdoc->dbcsCodePage,
			vs.styles[StyleDefault].characterSet, rangesInOrder.size() > 1, sel.selType == Selection::SelTypes::lines);
	}
}

void Editor::CopyRangeToClipboard(Sci::Position start, Sci::Position end) {
	start = pdoc->ClampPositionIntoDocument(start);
	end = pdoc->ClampPositionIntoDocument(end);
	SelectionText selectedText;
	std::string text = RangeText(start, end);
	selectedText.Copy(text,
		pdoc->dbcsCodePage, vs.styles[StyleDefault].characterSet, false, false);
	CopyToClipboard(selectedText);
}

void Editor::CopyText(size_t length, const char *text) {
	SelectionText selectedText;
	selectedText.Copy(std::string(text, length),
		pdoc->dbcsCodePage, vs.styles[StyleDefault].characterSet, false, false);
	CopyToClipboard(selectedText);
}

void Editor::SetDragPosition(SelectionPosition newPos) {
	if (newPos.Position() >= 0) {
		newPos = MovePositionOutsideChar(newPos, 1);
		posDrop = newPos;
	}
	if (!(posDrag == newPos)) {
		const CaretPolicies dragCaretPolicies = {
			CaretPolicySlop(CaretPolicy::Slop | CaretPolicy::Strict | CaretPolicy::Even, 50),
			CaretPolicySlop(CaretPolicy::Slop | CaretPolicy::Strict | CaretPolicy::Even, 2)
		};
		MovedCaret(newPos, posDrag, true, dragCaretPolicies);

		caret.on = true;
		FineTickerCancel(TickReason::caret);
		if ((caret.active) && (caret.period > 0) && (newPos.Position() < 0))
			FineTickerStart(TickReason::caret, caret.period, caret.period/10);
		InvalidateCaret();
		posDrag = newPos;
		InvalidateCaret();
	}
}

void Editor::DisplayCursor(Window::Cursor c) {
	if (cursorMode == CursorShape::Normal)
		wMain.SetCursor(c);
	else
		wMain.SetCursor(static_cast<Window::Cursor>(cursorMode));
}

bool Editor::DragThreshold(Point ptStart, Point ptNow) {
	const Point ptDiff = ptStart - ptNow;
	const XYPOSITION distanceSquared = ptDiff.x * ptDiff.x + ptDiff.y * ptDiff.y;
	return distanceSquared > 16.0f;
}

void Editor::StartDrag() {
	// Always handled by subclasses
}

void Editor::DropAt(SelectionPosition position, const char *value, size_t lengthValue, bool moving, bool rectangular) {
	//Platform::DebugPrintf("DropAt %d %d\n", inDragDrop, position);
	if (inDragDrop == DragDrop::dragging)
		dropWentOutside = false;

	const bool positionWasInSelection = PositionInSelection(position.Position());

	const bool positionOnEdgeOfSelection =
	    (position == SelectionStart()) || (position == SelectionEnd());

	if ((inDragDrop != DragDrop::dragging) || !(positionWasInSelection) ||
	        (positionOnEdgeOfSelection && !moving)) {

		const SelectionPosition selStart = SelectionStart();
		const SelectionPosition selEnd = SelectionEnd();

		UndoGroup ug(pdoc);

		SelectionPosition positionAfterDeletion = position;
		if ((inDragDrop == DragDrop::dragging) && moving) {
			// Remove dragged out text
			if (rectangular || sel.selType == Selection::SelTypes::lines) {
				for (size_t r=0; r<sel.Count(); r++) {
					if (position >= sel.Range(r).Start()) {
						if (position > sel.Range(r).End()) {
							positionAfterDeletion.Add(-sel.Range(r).Length());
						} else {
							positionAfterDeletion.Add(-SelectionRange(position, sel.Range(r).Start()).Length());
						}
					}
				}
			} else {
				if (position > selStart) {
					positionAfterDeletion.Add(-SelectionRange(selEnd, selStart).Length());
				}
			}
			ClearSelection();
		}
		position = positionAfterDeletion;

		std::string convertedText = Document::TransformLineEnds(value, lengthValue, pdoc->eolMode);

		if (rectangular) {
			PasteRectangular(position, convertedText.c_str(), convertedText.length());
			// Should try to select new rectangle but it may not be a rectangle now so just select the drop position
			SetEmptySelection(position);
		} else {
			position = MovePositionOutsideChar(position, sel.MainCaret() - position.Position());
			position = RealizeVirtualSpace(position);
			const Sci::Position lengthInserted = pdoc->InsertString(
				position.Position(), convertedText);
			if (lengthInserted > 0) {
				SelectionPosition posAfterInsertion = position;
				posAfterInsertion.Add(lengthInserted);
				SetSelection(posAfterInsertion, position);
			}
		}
	} else if (inDragDrop == DragDrop::dragging) {
		SetEmptySelection(position);
	}
}

void Editor::DropAt(SelectionPosition position, const char *value, bool moving, bool rectangular) {
	DropAt(position, value, strlen(value), moving, rectangular);
}

/**
 * @return true if given position is inside the selection,
 */
bool Editor::PositionInSelection(Sci::Position pos) {
	pos = MovePositionOutsideChar(pos, sel.MainCaret() - pos);
	for (size_t r=0; r<sel.Count(); r++) {
		if (sel.Range(r).Contains(pos))
			return true;
	}
	return false;
}

bool Editor::PointInSelection(Point pt) {
	const SelectionPosition pos = SPositionFromLocation(pt, false, true);
	const Point ptPos = LocationFromPosition(pos);
	for (size_t r=0; r<sel.Count(); r++) {
		const SelectionRange &range = sel.Range(r);
		if (range.Contains(pos)) {
			bool hit = true;
			if (pos == range.Start()) {
				// see if just before selection
				if (pt.x < ptPos.x) {
					hit = false;
				}
			}
			if (pos == range.End()) {
				// see if just after selection
				if (pt.x > ptPos.x) {
					hit = false;
				}
			}
			if (hit)
				return true;
		}
	}
	return false;
}

ptrdiff_t Editor::SelectionFromPoint(Point pt) {
	// Prioritize checking inside non-empty selections since each character will be inside only 1 
	const SelectionPosition posChar = SPositionFromLocation(pt, true, true);
	for (size_t r = 0; r < sel.Count(); r++) {
		if (sel.Range(r).ContainsCharacter(posChar)) {
			return r;
		}
	}

	// Then check if near empty selections as may be near more than 1
	const SelectionPosition pos = SPositionFromLocation(pt, true, false);
	for (size_t r = 0; r < sel.Count(); r++) {
		const SelectionRange &range = sel.Range(r);
		if ((range.Empty()) && (pos == range.caret)) {
			return r;
		}
	}

	// No selection at point
	return -1;
}

bool Editor::PointInSelMargin(Point pt) const {
	// Really means: "Point in a margin"
	if (vs.fixedColumnWidth > 0) {	// There is a margin
		PRectangle rcSelMargin = GetClientRectangle();
		rcSelMargin.right = static_cast<XYPOSITION>(vs.textStart - vs.leftMarginWidth);
		rcSelMargin.left = static_cast<XYPOSITION>(vs.textStart - vs.fixedColumnWidth);
		const Point ptOrigin = GetVisibleOriginInMain();
		rcSelMargin.Move(0, -ptOrigin.y);
		return rcSelMargin.ContainsWholePixel(pt);
	} else {
		return false;
	}
}

Window::Cursor Editor::GetMarginCursor(Point pt) const noexcept {
	int x = 0;
	for (const MarginStyle &m : vs.ms) {
		if ((pt.x >= x) && (pt.x < x + m.width))
			return static_cast<Window::Cursor>(m.cursor);
		x += m.width;
	}
	return Window::Cursor::reverseArrow;
}

void Editor::DropSelection(size_t part) {
	sel.DropSelection(part);
	ContainerNeedsUpdate(Update::Selection);
	Redraw();
}

void Editor::TrimAndSetSelection(Sci::Position currentPos_, Sci::Position anchor_) {
	sel.TrimSelection(SelectionRange(currentPos_, anchor_));
	SetSelection(currentPos_, anchor_);
}

void Editor::LineSelection(Sci::Position lineCurrentPos_, Sci::Position lineAnchorPos_, bool wholeLine) {
	Sci::Position selCurrentPos;
	Sci::Position selAnchorPos;
	if (wholeLine) {
		const Sci::Line lineCurrent_ = pdoc->SciLineFromPosition(lineCurrentPos_);
		const Sci::Line lineAnchor_ = pdoc->SciLineFromPosition(lineAnchorPos_);
		if (lineAnchorPos_ < lineCurrentPos_) {
			selCurrentPos = pdoc->LineStart(lineCurrent_ + 1);
			selAnchorPos = pdoc->LineStart(lineAnchor_);
		} else if (lineAnchorPos_ > lineCurrentPos_) {
			selCurrentPos = pdoc->LineStart(lineCurrent_);
			selAnchorPos = pdoc->LineStart(lineAnchor_ + 1);
		} else { // Same line, select it
			selCurrentPos = pdoc->LineStart(lineAnchor_ + 1);
			selAnchorPos = pdoc->LineStart(lineAnchor_);
		}
	} else {
		if (lineAnchorPos_ < lineCurrentPos_) {
			selCurrentPos = StartEndDisplayLine(lineCurrentPos_, false) + 1;
			selCurrentPos = pdoc->MovePositionOutsideChar(selCurrentPos, 1);
			selAnchorPos = StartEndDisplayLine(lineAnchorPos_, true);
		} else if (lineAnchorPos_ > lineCurrentPos_) {
			selCurrentPos = StartEndDisplayLine(lineCurrentPos_, true);
			selAnchorPos = StartEndDisplayLine(lineAnchorPos_, false) + 1;
			selAnchorPos = pdoc->MovePositionOutsideChar(selAnchorPos, 1);
		} else { // Same line, select it
			selCurrentPos = StartEndDisplayLine(lineAnchorPos_, false) + 1;
			selCurrentPos = pdoc->MovePositionOutsideChar(selCurrentPos, 1);
			selAnchorPos = StartEndDisplayLine(lineAnchorPos_, true);
		}
	}
	TrimAndSetSelection(selCurrentPos, selAnchorPos);
}

void Editor::WordSelection(Sci::Position pos) {
	if (pos < wordSelectAnchorStartPos) {
		// Extend backward to the word containing pos.
		// Skip ExtendWordSelect if the line is empty or if pos is after the last character.
		// This ensures that a series of empty lines isn't counted as a single "word".
		if (!pdoc->IsLineEndPosition(pos))
			pos = pdoc->ExtendWordSelect(pdoc->MovePositionOutsideChar(pos + 1, 1), -1);
		TrimAndSetSelection(pos, wordSelectAnchorEndPos);
	} else if (pos > wordSelectAnchorEndPos) {
		// Extend forward to the word containing the character to the left of pos.
		// Skip ExtendWordSelect if the line is empty or if pos is the first position on the line.
		// This ensures that a series of empty lines isn't counted as a single "word".
		if (pos > pdoc->LineStartPosition(pos))
			pos = pdoc->ExtendWordSelect(pdoc->MovePositionOutsideChar(pos - 1, -1), 1);
		TrimAndSetSelection(pos, wordSelectAnchorStartPos);
	} else {
		// Select only the anchored word
		if (pos >= originalAnchorPos)
			TrimAndSetSelection(wordSelectAnchorEndPos, wordSelectAnchorStartPos);
		else
			TrimAndSetSelection(wordSelectAnchorStartPos, wordSelectAnchorEndPos);
	}
}

void Editor::DwellEnd(bool mouseMoved) {
	if (mouseMoved)
		ticksToDwell = dwellDelay;
	else
		ticksToDwell = TimeForever;
	if (dwelling && (dwellDelay < TimeForever)) {
		dwelling = false;
		NotifyDwelling(ptMouseLast, dwelling);
	}
	FineTickerCancel(TickReason::dwell);
}

void Editor::MouseLeave() {
	SetHotSpotRange(nullptr);
	SetHoverIndicatorPosition(Sci::invalidPosition);
	if (!HaveMouseCapture()) {
		ptMouseLast = Point(-1, -1);
		DwellEnd(true);
	}
}

void Editor::ButtonDownWithModifiers(Point pt, unsigned int curTime, KeyMod modifiers) {
	SetHoverIndicatorPoint(pt);
	//Platform::DebugPrintf("ButtonDown %d %d = %d alt=%d %d\n", curTime, lastClickTime, curTime - lastClickTime, alt, inDragDrop);
	ptMouseLast = pt;
	const bool ctrl = FlagSet(modifiers, KeyMod::Ctrl);
	const bool shift = FlagSet(modifiers, KeyMod::Shift);
	const bool alt = FlagSet(modifiers, KeyMod::Alt);
	const SelectionPosition clickPos = SPositionFromLocation(pt, false, false, AllowVirtualSpace(virtualSpaceOptions, alt));
	const SelectionPosition newPos = MovePositionOutsideChar(clickPos, sel.MainCaret() - clickPos.Position());
	const SelectionPosition newCharPos = MovePositionOutsideChar(
		SPositionFromLocation(pt, false, true, false), -1);
	inDragDrop = DragDrop::none;
	sel.SetMoveExtends(false);

	if (NotifyMarginClick(pt, modifiers))
		return;

	NotifyIndicatorClick(true, newPos.Position(), modifiers);

	const bool multiClick = (curTime < (lastClickTime + Platform::DoubleClickTime())) && Close(pt, lastClick, doubleClickCloseThreshold);
	lastClickTime = curTime;
	lastClick = pt;

	const bool inSelMargin = PointInSelMargin(pt);
	// In margin ctrl+(double)click should always select everything
	if (ctrl && inSelMargin) {
		SelectAll();
		return;
	}
	if (shift && !inSelMargin) {
		SetSelection(newPos);
	}
	if (multiClick) {
		//Platform::DebugPrintf("Double click %d %d = %d\n", curTime, lastClickTime, curTime - lastClickTime);
		ChangeMouseCapture(true);
		if (!ctrl || !multipleSelection || (selectionUnit != TextUnit::character && selectionUnit != TextUnit::word))
			SetEmptySelection(newPos.Position());
		bool doubleClick = false;
		if (inSelMargin) {
			// Inside margin selection type should be either subLine or wholeLine.
			if (selectionUnit == TextUnit::subLine) {
				// If it is subLine, we're inside a *double* click and word wrap is enabled,
				// so we switch to wholeLine in order to select whole line.
				selectionUnit = TextUnit::wholeLine;
			} else if (selectionUnit != TextUnit::subLine && selectionUnit != TextUnit::wholeLine) {
				// If it is neither, reset selection type to line selection.
				selectionUnit = (Wrapping() && (FlagSet(marginOptions, MarginOption::SubLineSelect))) ? TextUnit::subLine : TextUnit::wholeLine;
			}
		} else {
			if (selectionUnit == TextUnit::character) {
				selectionUnit = TextUnit::word;
				doubleClick = true;
			} else if (selectionUnit == TextUnit::word) {
				// Since we ended up here, we're inside a *triple* click, which should always select
				// whole line regardless of word wrap being enabled or not.
				selectionUnit = TextUnit::wholeLine;
			} else {
				selectionUnit = TextUnit::character;
				originalAnchorPos = sel.MainCaret();
			}
		}

		if (selectionUnit == TextUnit::word) {
			Sci::Position charPos = originalAnchorPos;
			if (sel.MainCaret() == originalAnchorPos) {
				charPos = PositionFromLocation(pt, false, true);
				charPos = MovePositionOutsideChar(charPos, -1);
			}

			Sci::Position startWord;
			Sci::Position endWord;
			if ((sel.MainCaret() >= originalAnchorPos) && !pdoc->IsLineEndPosition(charPos)) {
				startWord = pdoc->ExtendWordSelect(pdoc->MovePositionOutsideChar(charPos + 1, 1), -1);
				endWord = pdoc->ExtendWordSelect(charPos, 1);
			} else {
				// Selecting backwards, or anchor beyond last character on line. In these cases,
				// we select the word containing the character to the *left* of the anchor.
				if (charPos > pdoc->LineStartPosition(charPos)) {
					startWord = pdoc->ExtendWordSelect(charPos, -1);
					endWord = pdoc->ExtendWordSelect(startWord, 1);
				} else {
					// Anchor at start of line; select nothing to begin with.
					startWord = charPos;
					endWord = charPos;
				}
			}

			wordSelectAnchorStartPos = startWord;
			wordSelectAnchorEndPos = endWord;
			wordSelectInitialCaretPos = sel.MainCaret();
			WordSelection(wordSelectInitialCaretPos);
		} else if (selectionUnit == TextUnit::subLine || selectionUnit == TextUnit::wholeLine) {
			lineAnchorPos = newPos.Position();
			LineSelection(lineAnchorPos, lineAnchorPos, selectionUnit == TextUnit::wholeLine);
			//Platform::DebugPrintf("Triple click: %d - %d\n", anchor, currentPos);
		} else {
			SetEmptySelection(sel.MainCaret());
		}
		//Platform::DebugPrintf("Double click: %d - %d\n", anchor, currentPos);
		if (doubleClick) {
			NotifyDoubleClick(pt, modifiers);
			if (PositionIsHotspot(newCharPos.Position()))
				NotifyHotSpotDoubleClicked(newCharPos.Position(), modifiers);
		}
	} else {	// Single click
		if (inSelMargin) {
			if (sel.IsRectangular() || (sel.Count() > 1)) {
				InvalidateWholeSelection();
				sel.Clear();
			}
			sel.selType = Selection::SelTypes::stream;
			if (!shift) {
				// Single click in margin: select wholeLine or only subLine if word wrap is enabled
				lineAnchorPos = newPos.Position();
				selectionUnit = (Wrapping() && (FlagSet(marginOptions, MarginOption::SubLineSelect))) ? TextUnit::subLine : TextUnit::wholeLine;
				LineSelection(lineAnchorPos, lineAnchorPos, selectionUnit == TextUnit::wholeLine);
			} else {
				// Single shift+click in margin: select from line anchor to clicked line
				if (sel.MainAnchor() > sel.MainCaret())
					lineAnchorPos = sel.MainAnchor() - 1;
				else
					lineAnchorPos = sel.MainAnchor();
				// Reset selection type if there is an empty selection.
				// This ensures that we don't end up stuck in previous selection mode, which is no longer valid.
				// Otherwise, if there's a non empty selection, reset selection type only if it differs from selSubLine and selWholeLine.
				// This ensures that we continue selecting in the same selection mode.
				if (sel.Empty() || (selectionUnit != TextUnit::subLine && selectionUnit != TextUnit::wholeLine))
					selectionUnit = (Wrapping() && (FlagSet(marginOptions, MarginOption::SubLineSelect))) ? TextUnit::subLine : TextUnit::wholeLine;
				LineSelection(newPos.Position(), lineAnchorPos, selectionUnit == TextUnit::wholeLine);
			}

			SetDragPosition(SelectionPosition(Sci::invalidPosition));
			ChangeMouseCapture(true);
		} else {
			if (PointIsHotspot(pt)) {
				NotifyHotSpotClicked(newCharPos.Position(), modifiers);
				hotSpotClickPos = newCharPos.Position();
			}
			if (!shift) {
				const ptrdiff_t selectionPart = SelectionFromPoint(pt);
				if (selectionPart >= 0) {
					if (multipleSelection && ctrl) {
						// Deselect
						if (sel.Count() > 1) {
							DropSelection(selectionPart);
							// Completed: don't want any more processing of this click
							return;
						} else {
							// Switch to just the click position
							SetSelection(newPos, newPos);
						}
					}
					if (!sel.Range(selectionPart).Empty()) {
						inDragDrop = DragDrop::initial;
					}
				}
			}
			ChangeMouseCapture(true);
			if (inDragDrop != DragDrop::initial) {
				SetDragPosition(SelectionPosition(Sci::invalidPosition));
				if (!shift) {
					if (ctrl && multipleSelection) {
						const SelectionRange range(newPos);
						sel.TentativeSelection(range);
						InvalidateSelection(range, true);
					} else {
						InvalidateSelection(SelectionRange(newPos), true);
						if (sel.Count() > 1)
							Redraw();
						if ((sel.Count() > 1) || (sel.selType != Selection::SelTypes::stream))
							sel.Clear();
						sel.selType = alt ? Selection::SelTypes::rectangle : Selection::SelTypes::stream;
						SetSelection(newPos, newPos);
					}
				}
				SelectionPosition anchorCurrent = newPos;
				if (shift)
					anchorCurrent = sel.IsRectangular() ?
						sel.Rectangular().anchor : sel.RangeMain().anchor;
				sel.selType = alt ? Selection::SelTypes::rectangle : Selection::SelTypes::stream;
				selectionUnit = TextUnit::character;
				originalAnchorPos = sel.MainCaret();
				sel.Rectangular() = SelectionRange(newPos, anchorCurrent);
				SetRectangularRange();
			}
		}
	}
	lastXChosen = static_cast<int>(pt.x) + xOffset;
	ShowCaretAtCurrentPosition();
}

void Editor::RightButtonDownWithModifiers(Point pt, unsigned int, KeyMod modifiers) {
	if (NotifyMarginRightClick(pt, modifiers))
		return;
}

bool Editor::PositionIsHotspot(Sci::Position position) const noexcept {
	return vs.styles[pdoc->StyleIndexAt(position)].hotspot;
}

bool Editor::PointIsHotspot(Point pt) {
	const Sci::Position pos = PositionFromLocation(pt, true, true);
	if (pos == Sci::invalidPosition)
		return false;
	return PositionIsHotspot(pos);
}

void Editor::SetHoverIndicatorPosition(Sci::Position position) {
	const Sci::Position hoverIndicatorPosPrev = hoverIndicatorPos;
	hoverIndicatorPos = Sci::invalidPosition;
	if (!vs.indicatorsDynamic)
		return;
	if (position != Sci::invalidPosition) {
		for (const IDecoration *deco : pdoc->decorations->View()) {
			if (vs.indicators[deco->Indicator()].IsDynamic()) {
				if (pdoc->decorations->ValueAt(deco->Indicator(), position)) {
					hoverIndicatorPos = position;
				}
			}
		}
	}
	if (hoverIndicatorPosPrev != hoverIndicatorPos) {
		Redraw();
	}
}

void Editor::SetHoverIndicatorPoint(Point pt) {
	if (!vs.indicatorsDynamic) {
		SetHoverIndicatorPosition(Sci::invalidPosition);
	} else {
		SetHoverIndicatorPosition(PositionFromLocation(pt, true, true));
	}
}

void Editor::SetHotSpotRange(const Point *pt) {
	if (pt) {
		const Sci::Position pos = PositionFromLocation(*pt, false, true);

		// If we don't limit this to word characters then the
		// range can encompass more than the run range and then
		// the underline will not be drawn properly.
		Range hsNew;
		hsNew.start = pdoc->ExtendStyleRange(pos, -1, hotspotSingleLine);
		hsNew.end = pdoc->ExtendStyleRange(pos, 1, hotspotSingleLine);

		// Only invalidate the range if the hotspot range has changed...
		if (!(hsNew == hotspot)) {
			if (hotspot.Valid()) {
				InvalidateRange(hotspot.start, hotspot.end);
			}
			hotspot = hsNew;
			InvalidateRange(hotspot.start, hotspot.end);
		}
	} else {
		if (hotspot.Valid()) {
			InvalidateRange(hotspot.start, hotspot.end);
		}
		hotspot = Range(Sci::invalidPosition);
	}
}

void Editor::ButtonMoveWithModifiers(Point pt, unsigned int, KeyMod modifiers) {
	if (ptMouseLast != pt) {
		DwellEnd(true);
	}

	SelectionPosition movePos = SPositionFromLocation(pt, false, false,
		AllowVirtualSpace(virtualSpaceOptions, sel.IsRectangular()));
	movePos = MovePositionOutsideChar(movePos, sel.MainCaret() - movePos.Position());

	if (inDragDrop == DragDrop::initial) {
		if (DragThreshold(ptMouseLast, pt)) {
			ChangeMouseCapture(false);
			SetDragPosition(movePos);
			CopySelectionRange(&drag);
			StartDrag();
		}
		return;
	}

	ptMouseLast = pt;
	PRectangle rcClient = GetClientRectangle();
	const Point ptOrigin = GetVisibleOriginInMain();
	rcClient.Move(0, -ptOrigin.y);
	if ((dwellDelay < TimeForever) && rcClient.Contains(pt)) {
		FineTickerStart(TickReason::dwell, dwellDelay, dwellDelay/10);
	}
	//Platform::DebugPrintf("Move %d %d\n", pt.x, pt.y);
	if (HaveMouseCapture()) {

		// Slow down autoscrolling/selection
		autoScrollTimer.ticksToWait -= timer.tickSize;
		if (autoScrollTimer.ticksToWait > 0)
			return;
		autoScrollTimer.ticksToWait = autoScrollDelay;

		// Adjust selection
		if (posDrag.IsValid()) {
			SetDragPosition(movePos);
		} else {
			if (selectionUnit == TextUnit::character) {
				if (sel.selType == Selection::SelTypes::stream && FlagSet(modifiers, KeyMod::Alt) && mouseSelectionRectangularSwitch) {
					sel.selType = Selection::SelTypes::rectangle;
				}
				if (sel.IsRectangular()) {
					sel.Rectangular() = SelectionRange(movePos, sel.Rectangular().anchor);
					SetSelection(movePos, sel.RangeMain().anchor);
				} else if (sel.Count() > 1) {
					InvalidateSelection(sel.RangeMain(), false);
					const SelectionRange range(movePos, sel.RangeMain().anchor);
					sel.TentativeSelection(range);
					InvalidateSelection(range, true);
				} else {
					SetSelection(movePos, sel.RangeMain().anchor);
				}
			} else if (selectionUnit == TextUnit::word) {
				// Continue selecting by word
				if (movePos.Position() == wordSelectInitialCaretPos) {  // Didn't move
					// No need to do anything. Previously this case was lumped
					// in with "Moved forward", but that can be harmful in this
					// case: a handler for the NotifyDoubleClick re-adjusts
					// the selection for a fancier definition of "word" (for
					// example, in Perl it is useful to include the leading
					// '$', '%' or '@' on variables for word selection). In this
					// the ButtonMove() called via TickFor() for auto-scrolling
					// could result in the fancier word selection adjustment
					// being unmade.
				} else {
					wordSelectInitialCaretPos = -1;
					WordSelection(movePos.Position());
				}
			} else {
				// Continue selecting by line
				LineSelection(movePos.Position(), lineAnchorPos, selectionUnit == TextUnit::wholeLine);
			}
		}

		// Autoscroll
		const Sci::Line lineMove = DisplayFromPosition(movePos.Position());
		if (pt.y >= rcClient.bottom) {
			ScrollTo(lineMove - LinesOnScreen() + 1);
			Redraw();
		} else if (pt.y < rcClient.top) {
			ScrollTo(lineMove);
			Redraw();
		}
		EnsureCaretVisible(false, false, true);

		if (hotspot.Valid() && !PointIsHotspot(pt))
			SetHotSpotRange(nullptr);

		if (hotSpotClickPos != Sci::invalidPosition && PositionFromLocation(pt, true, true) != hotSpotClickPos) {
			if (inDragDrop == DragDrop::none) {
				DisplayCursor(Window::Cursor::text);
			}
			hotSpotClickPos = Sci::invalidPosition;
		}

	} else {
		if (vs.fixedColumnWidth > 0) {	// There is a margin
			if (PointInSelMargin(pt)) {
				DisplayCursor(GetMarginCursor(pt));
				SetHotSpotRange(nullptr);
				SetHoverIndicatorPosition(Sci::invalidPosition);
				return; 	// No need to test for selection
			}
		}
		// Display regular (drag) cursor over selection
		if (PointInSelection(pt) && !SelectionEmpty()) {
			DisplayCursor(Window::Cursor::arrow);
			SetHoverIndicatorPosition(Sci::invalidPosition);
		} else {
			SetHoverIndicatorPoint(pt);
			if (PointIsHotspot(pt)) {
				DisplayCursor(Window::Cursor::hand);
				SetHotSpotRange(&pt);
			} else {
				if (hoverIndicatorPos != Sci::invalidPosition)
					DisplayCursor(Window::Cursor::hand);
				else
					DisplayCursor(Window::Cursor::text);
				SetHotSpotRange(nullptr);
			}
		}
	}
}

void Editor::ButtonUpWithModifiers(Point pt, unsigned int curTime, KeyMod modifiers) {
	//Platform::DebugPrintf("ButtonUp %d %d\n", HaveMouseCapture(), inDragDrop);
	SelectionPosition newPos = SPositionFromLocation(pt, false, false,
		AllowVirtualSpace(virtualSpaceOptions, sel.IsRectangular()));
	if (hoverIndicatorPos != Sci::invalidPosition)
		InvalidateRange(newPos.Position(), newPos.Position() + 1);
	newPos = MovePositionOutsideChar(newPos, sel.MainCaret() - newPos.Position());
	if (inDragDrop == DragDrop::initial) {
		inDragDrop = DragDrop::none;
		SetEmptySelection(newPos);
		selectionUnit = TextUnit::character;
		originalAnchorPos = sel.MainCaret();
	}
	if (hotSpotClickPos != Sci::invalidPosition && PointIsHotspot(pt)) {
		hotSpotClickPos = Sci::invalidPosition;
		SelectionPosition newCharPos = SPositionFromLocation(pt, false, true, false);
		newCharPos = MovePositionOutsideChar(newCharPos, -1);
		NotifyHotSpotReleaseClick(newCharPos.Position(), modifiers & KeyMod::Ctrl);
	}
	if (HaveMouseCapture()) {
		if (PointInSelMargin(pt)) {
			DisplayCursor(GetMarginCursor(pt));
		} else {
			DisplayCursor(Window::Cursor::text);
			SetHotSpotRange(nullptr);
		}
		ptMouseLast = pt;
		ChangeMouseCapture(false);
		NotifyIndicatorClick(false, newPos.Position(), modifiers);
		if (inDragDrop == DragDrop::dragging) {
			const SelectionPosition selStart = SelectionStart();
			const SelectionPosition selEnd = SelectionEnd();
			if (selStart < selEnd) {
				if (drag.Length()) {
					const Sci::Position length = drag.Length();
					if (FlagSet(modifiers, KeyMod::Ctrl)) {
						const Sci::Position lengthInserted = pdoc->InsertString(
							newPos.Position(), drag.Data(), length);
						if (lengthInserted > 0) {
							SetSelection(newPos.Position(), newPos.Position() + lengthInserted);
						}
					} else if (newPos < selStart) {
						pdoc->DeleteChars(selStart.Position(), drag.Length());
						const Sci::Position lengthInserted = pdoc->InsertString(
							newPos.Position(), drag.Data(), length);
						if (lengthInserted > 0) {
							SetSelection(newPos.Position(), newPos.Position() + lengthInserted);
						}
					} else if (newPos > selEnd) {
						pdoc->DeleteChars(selStart.Position(), drag.Length());
						newPos.Add(-static_cast<Sci::Position>(drag.Length()));
						const Sci::Position lengthInserted = pdoc->InsertString(
							newPos.Position(), drag.Data(), length);
						if (lengthInserted > 0) {
							SetSelection(newPos.Position(), newPos.Position() + lengthInserted);
						}
					} else {
						SetEmptySelection(newPos.Position());
					}
					drag.Clear();
				}
				selectionUnit = TextUnit::character;
			}
		} else {
			if (selectionUnit == TextUnit::character) {
				if (sel.Count() > 1) {
					sel.RangeMain() =
						SelectionRange(newPos, sel.Range(sel.Count() - 1).anchor);
					InvalidateWholeSelection();
				} else {
					SetSelection(newPos, sel.RangeMain().anchor);
				}
			}
			sel.CommitTentative();
		}
		SetRectangularRange();
		lastClickTime = curTime;
		lastClick = pt;
		lastXChosen = static_cast<int>(pt.x) + xOffset;
		if (sel.selType == Selection::SelTypes::stream) {
			SetLastXChosen();
		}
		inDragDrop = DragDrop::none;
		EnsureCaretVisible(false);
	}
}

bool Editor::Idle() {
	NotifyUpdateUI();

	bool needWrap = Wrapping() && wrapPending.NeedsWrap();

	if (needWrap) {
		// Wrap lines during idle.
		WrapLines(WrapScope::wsIdle);
		// No more wrapping
		needWrap = wrapPending.NeedsWrap();
	} else if (needIdleStyling) {
		IdleStyle();
	}

	// Add more idle things to do here, but make sure idleDone is
	// set correctly before the function returns. returning
	// false will stop calling this idle function until SetIdle() is
	// called again.

	const bool idleDone = !needWrap && !needIdleStyling; // && thatDone && theOtherThingDone...

	return !idleDone;
}

void Editor::TickFor(TickReason reason) {
	switch (reason) {
		case TickReason::caret:
			caret.on = !caret.on;
			if (caret.active) {
				InvalidateCaret();
			}
			break;
		case TickReason::scroll:
			// Auto scroll
			if (HaveMouseCapture()) {
				ButtonMoveWithModifiers(ptMouseLast, 0, KeyMod::Norm);
			} else {
				// Capture cancelled so cancel timer
				FineTickerCancel(TickReason::scroll);
			}
			break;
		case TickReason::widen:
			SetScrollBars();
			FineTickerCancel(TickReason::widen);
			break;
		case TickReason::dwell:
			if ((!HaveMouseCapture()) &&
				(ptMouseLast.y >= 0)) {
				dwelling = true;
				NotifyDwelling(ptMouseLast, dwelling);
			}
			FineTickerCancel(TickReason::dwell);
			break;
		default:
			// tickPlatform handled by subclass
			break;
	}
}

// FineTickerStart is be overridden by subclasses that support fine ticking so
// this method should never be called.
bool Editor::FineTickerRunning(TickReason) {
	assert(false);
	return false;
}

// FineTickerStart is be overridden by subclasses that support fine ticking so
// this method should never be called.
void Editor::FineTickerStart(TickReason, int, int) {
	assert(false);
}

// FineTickerCancel is be overridden by subclasses that support fine ticking so
// this method should never be called.
void Editor::FineTickerCancel(TickReason) {
	assert(false);
}

void Editor::ChangeMouseCapture(bool on) {
	SetMouseCapture(on);
	// While mouse captured want timer to scroll automatically
	if (on) {
		FineTickerStart(TickReason::scroll, 100, 10);
	} else {
		FineTickerCancel(TickReason::scroll);
	}
}

void Editor::SetFocusState(bool focusState) {
	const bool changing = hasFocus != focusState;
	hasFocus = focusState;
	if (changing) {
		Redraw();
	}
	NotifyFocus(hasFocus);
	if (!hasFocus) {
		CancelModes();
	}
	ShowCaretAtCurrentPosition();
}

void Editor::UpdateBaseElements() {
	// Overridden by subclasses
}

Sci::Position Editor::PositionAfterArea(PRectangle rcArea) const {
	// The start of the document line after the display line after the area
	// This often means that the line after a modification is restyled which helps
	// detect multiline comment additions and heals single line comments
	const Sci::Line lineAfter = TopLineOfMain() + static_cast<Sci::Line>(rcArea.bottom - 1) / vs.lineHeight + 1;
	if (lineAfter < pcs->LinesDisplayed())
		return pdoc->LineStart(pcs->DocFromDisplay(lineAfter) + 1);
	else
		return pdoc->Length();
}

// Style to a position within the view. If this causes a change at end of last line then
// affects later lines so style all the viewed text.
void Editor::StyleToPositionInView(Sci::Position pos) {
	Sci::Position endWindow = PositionAfterArea(GetClientDrawingRectangle());
	if (pos > endWindow)
		pos = endWindow;
	const int styleAtEnd = pdoc->StyleIndexAt(pos-1);
	pdoc->EnsureStyledTo(pos);
	if ((endWindow > pos) && (styleAtEnd != pdoc->StyleIndexAt(pos-1))) {
		// Style at end of line changed so is multi-line change like starting a comment
		// so require rest of window to be styled.
		DiscardOverdraw();	// Prepared bitmaps may be invalid
		// DiscardOverdraw may have truncated client drawing area so recalculate endWindow
		endWindow = PositionAfterArea(GetClientDrawingRectangle());
		pdoc->EnsureStyledTo(endWindow);
	}
}

Sci::Position Editor::PositionAfterMaxStyling(Sci::Position posMax, bool scrolling) const {
	if (SynchronousStylingToVisible()) {
		// Both states do not limit styling
		return posMax;
	}

	// Try to keep time taken by styling reasonable so interaction remains smooth.
	// When scrolling, allow less time to ensure responsive
	const double secondsAllowed = scrolling ? 0.005 : 0.02;

	const size_t actionsInAllowedTime = std::clamp<Sci::Line>(
		pdoc->durationStyleOneByte.ActionsInAllowedTime(secondsAllowed),
		0x200, 0x20000);
	const Sci::Line lineLast = pdoc->LineFromPositionAfter(pdoc->SciLineFromPosition(pdoc->GetEndStyled()), actionsInAllowedTime);
	const Sci::Line stylingMaxLine = std::min(lineLast, pdoc->LinesTotal());

	return std::min(pdoc->LineStart(stylingMaxLine), posMax);
}

void Editor::StartIdleStyling(bool truncatedLastStyling) {
	if ((idleStyling == IdleStyling::All) || (idleStyling == IdleStyling::AfterVisible)) {
		if (pdoc->GetEndStyled() < pdoc->Length()) {
			// Style remainder of document in idle time
			needIdleStyling = true;
		}
	} else if (truncatedLastStyling) {
		needIdleStyling = true;
	}

	if (needIdleStyling) {
		SetIdle(true);
	}
}

// Style for an area but bound the amount of styling to remain responsive
void Editor::StyleAreaBounded(PRectangle rcArea, bool scrolling) {
	const Sci::Position posAfterArea = PositionAfterArea(rcArea);
	const Sci::Position posAfterMax = PositionAfterMaxStyling(posAfterArea, scrolling);
	if (posAfterMax < posAfterArea) {
		// Idle styling may be performed before current visible area
		// Style a bit now then style further in idle time
		pdoc->StyleToAdjustingLineDuration(posAfterMax);
	} else {
		// Can style all wanted now.
		StyleToPositionInView(posAfterArea);
	}
	StartIdleStyling(posAfterMax < posAfterArea);
}

void Editor::IdleStyle() {
	const Sci::Position posAfterArea = PositionAfterArea(GetClientRectangle());
	const Sci::Position endGoal = (idleStyling >= IdleStyling::AfterVisible) ?
		pdoc->Length() : posAfterArea;
	const Sci::Position posAfterMax = PositionAfterMaxStyling(endGoal, false);
	pdoc->StyleToAdjustingLineDuration(posAfterMax);
	if (pdoc->GetEndStyled() >= endGoal) {
		needIdleStyling = false;
	}
}

void Editor::IdleWork() {
	// Style the line after the modification as this allows modifications that change just the
	// line of the modification to heal instead of propagating to the rest of the window.
	if (FlagSet(workNeeded.items, WorkItems::style)) {
		StyleToPositionInView(pdoc->LineStart(pdoc->LineFromPosition(workNeeded.upTo) + 2));
	}
	NotifyUpdateUI();
	workNeeded.Reset();
}

void Editor::QueueIdleWork(WorkItems items, Sci::Position upTo) {
	workNeeded.Need(items, upTo);
}

int Editor::SupportsFeature(Supports feature) {
	AutoSurface surface(this);
	return surface->SupportsFeature(feature);
}

bool Editor::PaintContains(PRectangle rc) {
	if (rc.Empty()) {
		return true;
	} else {
		return rcPaint.Contains(rc);
	}
}

bool Editor::PaintContainsMargin() {
	if (HasMarginWindow()) {
		// With separate margin view, paint of text view
		// never contains margin.
		return false;
	}
	PRectangle rcSelMargin = GetClientRectangle();
	rcSelMargin.right = static_cast<XYPOSITION>(vs.textStart);
	return PaintContains(rcSelMargin);
}

void Editor::CheckForChangeOutsidePaint(Range r) {
	if (paintState == PaintState::painting && !paintingAllText) {
		//Platform::DebugPrintf("Checking range in paint %d-%d\n", r.start, r.end);
		if (!r.Valid())
			return;

		PRectangle rcRange = RectangleFromRange(r, 0);
		const PRectangle rcText = GetTextRectangle();
		if (rcRange.top < rcText.top) {
			rcRange.top = rcText.top;
		}
		if (rcRange.bottom > rcText.bottom) {
			rcRange.bottom = rcText.bottom;
		}

		if (!PaintContains(rcRange)) {
			AbandonPaint();
			paintAbandonedByStyling = true;
		}
	}
}

void Editor::SetBraceHighlight(Sci::Position pos0, Sci::Position pos1, int matchStyle) {
	if ((pos0 != braces[0]) || (pos1 != braces[1]) || (matchStyle != bracesMatchStyle)) {
		if ((braces[0] != pos0) || (matchStyle != bracesMatchStyle)) {
			CheckForChangeOutsidePaint(Range(braces[0]));
			CheckForChangeOutsidePaint(Range(pos0));
			braces[0] = pos0;
		}
		if ((braces[1] != pos1) || (matchStyle != bracesMatchStyle)) {
			CheckForChangeOutsidePaint(Range(braces[1]));
			CheckForChangeOutsidePaint(Range(pos1));
			braces[1] = pos1;
		}
		bracesMatchStyle = matchStyle;
		if (paintState == PaintState::notPainting) {
			Redraw();
		}
	}
}

void Editor::SetAnnotationHeights(Sci::Line start, Sci::Line end) {
	if (vs.annotationVisible != AnnotationVisible::Hidden) {
		RefreshStyleData();
		bool changedHeight = false;
		for (Sci::Line line=start; line<end && line<pdoc->LinesTotal(); line++) {
			int linesWrapped = 1;
			if (Wrapping()) {
				AutoSurface surface(this);
				std::shared_ptr<LineLayout> ll = view.RetrieveLineLayout(line, *this);
				if (surface && ll) {
					view.LayoutLine(*this, surface, vs, ll.get(), wrapWidth);
					linesWrapped = ll->lines;
				}
			}
			if (pcs->SetHeight(line, pdoc->AnnotationLines(line) + linesWrapped))
				changedHeight = true;
		}
		if (changedHeight) {
			SetScrollBars();
			SetVerticalScrollPos();
			Redraw();
		}
	}
}

void Editor::SetDocPointer(Document *document) {
	//Platform::DebugPrintf("** %x setdoc to %x\n", pdoc, document);
	pdoc->RemoveWatcher(this, nullptr);
	pdoc->Release();
	if (!document) {
		pdoc = new Document(DocumentOption::Default);
	} else {
		pdoc = document;
	}
	pdoc->AddRef();
	modelState.reset();
	pcs = ContractionStateCreate(pdoc->IsLarge());

	// Ensure all positions within document
	sel.Clear();
	targetRange = SelectionSegment();

	braces[0] = Sci::invalidPosition;
	braces[1] = Sci::invalidPosition;

	vs.ReleaseAllExtendedStyles();

	SetRepresentations();

	// Reset the contraction state to fully shown.
	pcs->Clear();
	pcs->InsertLines(0, pdoc->LinesTotal() - 1);
	SetAnnotationHeights(0, pdoc->LinesTotal());
	view.llc.Deallocate();
	NeedWrapping();

	hotspot = Range(Sci::invalidPosition);
	hoverIndicatorPos = Sci::invalidPosition;

	view.ClearAllTabstops();

	pdoc->AddWatcher(this, nullptr);
	SetScrollBars();
	Redraw();
}

void Editor::SetAnnotationVisible(AnnotationVisible visible) {
	if (vs.annotationVisible != visible) {
		const bool changedFromOrToHidden = ((vs.annotationVisible != AnnotationVisible::Hidden) != (visible != AnnotationVisible::Hidden));
		vs.annotationVisible = visible;
		if (changedFromOrToHidden) {
			const int dir = (vs.annotationVisible!= AnnotationVisible::Hidden) ? 1 : -1;
			for (Sci::Line line=0; line<pdoc->LinesTotal(); line++) {
				const int annotationLines = pdoc->AnnotationLines(line);
				if (annotationLines > 0) {
					pcs->SetHeight(line, pcs->GetHeight(line) + annotationLines * dir);
				}
			}
			SetScrollBars();
		}
		Redraw();
	}
}

void Editor::SetEOLAnnotationVisible(EOLAnnotationVisible visible) {
	if (vs.eolAnnotationVisible != visible) {
		vs.eolAnnotationVisible = visible;
		Redraw();
	}
}

/**
 * Recursively expand a fold, making lines visible except where they have an unexpanded parent.
 */
Sci::Line Editor::ExpandLine(Sci::Line line) {
	const Sci::Line lineMaxSubord = pdoc->GetLastChild(line);
	line++;
	Sci::Line lineStart = line;
	while (line <= lineMaxSubord) {
		const FoldLevel level = pdoc->GetFoldLevel(line);
		if (LevelIsHeader(level)) {
			pcs->SetVisible(lineStart, line, true);
			if (pcs->GetExpanded(line)) {
				line = ExpandLine(line);
			} else {
				line = pdoc->GetLastChild(line);
			}
			lineStart = line + 1;
		}
		line++;
	}
	if (lineStart <= lineMaxSubord) {
		pcs->SetVisible(lineStart, lineMaxSubord, true);
	}
	return lineMaxSubord;
}

void Editor::SetFoldExpanded(Sci::Line lineDoc, bool expanded) {
	if (pcs->SetExpanded(lineDoc, expanded)) {
		RedrawSelMargin();
	}
}

void Editor::FoldLine(Sci::Line line, FoldAction action) {
	if (line >= 0) {
		if (action == FoldAction::Toggle) {
			if (!LevelIsHeader(pdoc->GetFoldLevel(line))) {
				line = pdoc->GetFoldParent(line);
				if (line < 0)
					return;
			}
			action = (pcs->GetExpanded(line)) ? FoldAction::Contract : FoldAction::Expand;
		}

		if (action == FoldAction::Contract) {
			const Sci::Line lineMaxSubord = pdoc->GetLastChild(line);
			if (lineMaxSubord > line) {
				pcs->SetExpanded(line, false);
				pcs->SetVisible(line + 1, lineMaxSubord, false);

				const Sci::Line lineCurrent =
					pdoc->SciLineFromPosition(sel.MainCaret());
				if (lineCurrent > line && lineCurrent <= lineMaxSubord) {
					// This does not re-expand the fold
					EnsureCaretVisible();
				}
			}

		} else {
			if (!(pcs->GetVisible(line))) {
				EnsureLineVisible(line, false);
				GoToLine(line);
			}
			pcs->SetExpanded(line, true);
			ExpandLine(line);
		}

		SetScrollBars();
		Redraw();
	}
}

void Editor::FoldExpand(Sci::Line line, FoldAction action, FoldLevel level) {
	bool expanding = action == FoldAction::Expand;
	if (action == FoldAction::Toggle) {
		expanding = !pcs->GetExpanded(line);
	}
	// Ensure child lines lexed and fold information extracted before
	// flipping the state.
	pdoc->GetLastChild(line, LevelNumberPart(level));
	SetFoldExpanded(line, expanding);
	if (expanding && (pcs->HiddenLines() == 0))
		// Nothing to do
		return;
	const Sci::Line lineMaxSubord = pdoc->GetLastChild(line, LevelNumberPart(level));
	line++;
	pcs->SetVisible(line, lineMaxSubord, expanding);
	while (line <= lineMaxSubord) {
		const FoldLevel levelLine = pdoc->GetFoldLevel(line);
		if (LevelIsHeader(levelLine)) {
			SetFoldExpanded(line, expanding);
		}
		line++;
	}
	SetScrollBars();
	Redraw();
}

Sci::Line Editor::ContractedFoldNext(Sci::Line lineStart) const {
	for (Sci::Line line = lineStart; line<pdoc->LinesTotal();) {
		if (!pcs->GetExpanded(line) && LevelIsHeader(pdoc->GetFoldLevel(line)))
			return line;
		line = pcs->ContractedNext(line+1);
		if (line < 0)
			return -1;
	}

	return -1;
}

/**
 * Recurse up from this line to find any folds that prevent this line from being visible
 * and unfold them all.
 */
void Editor::EnsureLineVisible(Sci::Line lineDoc, bool enforcePolicy) {

	// In case in need of wrapping to ensure DisplayFromDoc works.
	if (lineDoc >= wrapPending.start) {
		if (WrapLines(WrapScope::wsAll)) {
			Redraw();
		}
	}

	if (!pcs->GetVisible(lineDoc)) {
		// Back up to find a non-blank line
		Sci::Line lookLine = lineDoc;
		FoldLevel lookLineLevel = pdoc->GetFoldLevel(lookLine);
		while ((lookLine > 0) && LevelIsWhitespace(lookLineLevel)) {
			lookLineLevel = pdoc->GetFoldLevel(--lookLine);
		}
		Sci::Line lineParent = pdoc->GetFoldParent(lookLine);
		if (lineParent < 0) {
			// Backed up to a top level line, so try to find parent of initial line
			lineParent = pdoc->GetFoldParent(lineDoc);
		}
		if (lineParent >= 0) {
			if (lineDoc != lineParent)
				EnsureLineVisible(lineParent, enforcePolicy);
			if (!pcs->GetExpanded(lineParent)) {
				pcs->SetExpanded(lineParent, true);
				ExpandLine(lineParent);
			}
		}
		SetScrollBars();
		Redraw();
	}
	if (enforcePolicy) {
		const Sci::Line lineDisplay = pcs->DisplayFromDoc(lineDoc);
		if (FlagSet(visiblePolicy.policy, VisiblePolicy::Slop)) {
			if ((topLine > lineDisplay) || ((FlagSet(visiblePolicy.policy, VisiblePolicy::Strict)) && (topLine + visiblePolicy.slop > lineDisplay))) {
				SetTopLine(std::clamp<Sci::Line>(lineDisplay - visiblePolicy.slop, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			} else if ((lineDisplay > topLine + LinesOnScreen() - 1) ||
			        ((FlagSet(visiblePolicy.policy, VisiblePolicy::Strict)) && (lineDisplay > topLine + LinesOnScreen() - 1 - visiblePolicy.slop))) {
				SetTopLine(std::clamp<Sci::Line>(lineDisplay - LinesOnScreen() + 1 + visiblePolicy.slop, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			}
		} else {
			if ((topLine > lineDisplay) || (lineDisplay > topLine + LinesOnScreen() - 1) || (FlagSet(visiblePolicy.policy, VisiblePolicy::Strict))) {
				SetTopLine(std::clamp<Sci::Line>(lineDisplay - LinesOnScreen() / 2 + 1, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			}
		}
	}
}

void Editor::FoldAll(FoldAction action) {
	const Sci::Line maxLine = pdoc->LinesTotal();
	const bool contractAll = FlagSet(action, FoldAction::ContractEveryLevel);
	action = static_cast<FoldAction>(static_cast<int>(action) & ~static_cast<int>(FoldAction::ContractEveryLevel));
	bool expanding = action == FoldAction::Expand;
	if (!expanding) {
		pdoc->EnsureStyledTo(pdoc->Length());
	}
	Sci::Line line = 0;
	if (action == FoldAction::Toggle) {
		// Discover current state
		for (; line < maxLine; line++) {
			if (LevelIsHeader(pdoc->GetFoldLevel(line))) {
				expanding = !pcs->GetExpanded(line);
				break;
			}
		}
	}
	if (expanding) {
		pcs->SetVisible(0, maxLine-1, true);
		pcs->ExpandAll();
	} else {
		for (; line < maxLine; line++) {
			const FoldLevel level = pdoc->GetFoldLevel(line);
			if (LevelIsHeader(level)) {
				if (FoldLevel::Base == LevelNumberPart(level)) {
					SetFoldExpanded(line, false);
					const Sci::Line lineMaxSubord = pdoc->GetLastChild(line);
					if (lineMaxSubord > line) {
						pcs->SetVisible(line + 1, lineMaxSubord, false);
						if (!contractAll) {
							line = lineMaxSubord;
						}
					}
				} else if (contractAll) {
					SetFoldExpanded(line, false);
				}
			}
		}
	}
	SetScrollBars();
	Redraw();
}

void Editor::FoldChanged(Sci::Line line, FoldLevel levelNow, FoldLevel levelPrev) {
	if (LevelIsHeader(levelNow)) {
		if (!LevelIsHeader(levelPrev)) {
			// Adding a fold point.
			if (pcs->SetExpanded(line, true)) {
				RedrawSelMargin();
			}
			FoldExpand(line, FoldAction::Expand, levelPrev);
		}
	} else if (LevelIsHeader(levelPrev)) {
		const Sci::Line prevLine = line - 1;
		const FoldLevel prevLineLevel = pdoc->GetFoldLevel(prevLine);

		// Combining two blocks where the first block is collapsed (e.g. by deleting the line(s) which separate(s) the two blocks)
		if ((LevelNumber(prevLineLevel) == LevelNumber(levelNow)) && !pcs->GetVisible(prevLine))
			FoldLine(pdoc->GetFoldParent(prevLine), FoldAction::Expand);

		if (!pcs->GetExpanded(line)) {
			// Removing the fold from one that has been contracted so should expand
			// otherwise lines are left invisible with no way to make them visible
			if (pcs->SetExpanded(line, true)) {
				RedrawSelMargin();
			}
			// Combining two blocks where the second one is collapsed (e.g. by adding characters in the line which separates the two blocks)
			FoldExpand(line, FoldAction::Expand, levelPrev);
		}
	}
	if (!LevelIsWhitespace(levelNow) &&
	        (LevelNumber(levelPrev) > LevelNumber(levelNow))) {
		if (pcs->HiddenLines()) {
			// See if should still be hidden
			const Sci::Line parentLine = pdoc->GetFoldParent(line);
			if ((parentLine < 0) || (pcs->GetExpanded(parentLine) && pcs->GetVisible(parentLine))) {
				pcs->SetVisible(line, line, true);
				SetScrollBars();
				Redraw();
			}
		}
	}

	// Combining two blocks where the first one is collapsed (e.g. by adding characters in the line which separates the two blocks)
	if (!LevelIsWhitespace(levelNow) && (LevelNumber(levelPrev) < LevelNumber(levelNow))) {
		if (pcs->HiddenLines()) {
			const Sci::Line parentLine = pdoc->GetFoldParent(line);
			if (!pcs->GetExpanded(parentLine) && pcs->GetVisible(line))
				FoldLine(parentLine, FoldAction::Expand);
		}
	}
}

void Editor::NeedShown(Sci::Position pos, Sci::Position len) {
	if (FlagSet(foldAutomatic, AutomaticFold::Show)) {
		const Sci::Line lineStart = pdoc->SciLineFromPosition(pos);
		const Sci::Line lineEnd = pdoc->SciLineFromPosition(pos+len);
		for (Sci::Line line = lineStart; line <= lineEnd; line++) {
			EnsureLineVisible(line, false);
		}
	} else {
		NotifyNeedShown(pos, len);
	}
}

Sci::Position Editor::GetTag(char *tagValue, int tagNumber) {
	const char *text = nullptr;
	Sci::Position length = 0;
	if ((tagNumber >= 1) && (tagNumber <= 9)) {
		char name[3] = "\\?";
		name[1] = static_cast<char>(tagNumber + '0');
		length = 2;
		text = pdoc->SubstituteByPosition(name, &length);
	}
	if (tagValue) {
		if (text)
			memcpy(tagValue, text, length + 1);
		else
			*tagValue = '\0';
	}
	return length;
}

Sci::Position Editor::ReplaceTarget(ReplaceType replaceType, std::string_view text) {
	UndoGroup ug(pdoc);

	std::string substituted;	// Copy in case of re-entrance

	if (replaceType == ReplaceType::patterns) {
		Sci::Position length = text.length();
		const char *p = pdoc->SubstituteByPosition(text.data(), &length);
		if (!p) {
			return 0;
		}
		substituted.assign(p, length);
		text = substituted;
	}

	if (replaceType == ReplaceType::minimal) {
		// Check for prefix and suffix and reduce text and target to match.
		// This is performed with Range which doesn't support virtual space.
		Range range(targetRange.start.Position(), targetRange.end.Position());
		pdoc->TrimReplacement(text, range);
		// Re-apply virtual space to start if start position didn't change.
		// Don't bother with end as its virtual space is not used
		const SelectionPosition start(range.start == targetRange.start.Position() ?
			targetRange.start : SelectionPosition(range.start));
		targetRange = SelectionSegment(start, SelectionPosition(range.end));
	}

	// Make a copy of targetRange in case callbacks use target
	SelectionSegment replaceRange = targetRange;

	// Remove the text inside the range
	if (replaceRange.Length() > 0)
		pdoc->DeleteChars(replaceRange.start.Position(), replaceRange.Length());

	// Realize virtual space of target start
	const Sci::Position startAfterSpaceInsertion = RealizeVirtualSpace(replaceRange.start.Position(), replaceRange.start.VirtualSpace());
	replaceRange.start.SetPosition(startAfterSpaceInsertion);
	replaceRange.end = replaceRange.start;

	// Insert the new text
	const Sci::Position lengthInserted = pdoc->InsertString(replaceRange.start.Position(), text);
	replaceRange.end.SetPosition(replaceRange.start.Position() + lengthInserted);

	// Copy back to targetRange in case application is chaining modifications
	targetRange = replaceRange;

	return text.length();
}

bool Editor::IsUnicodeMode() const noexcept {
	return pdoc && (CpUtf8 == pdoc->dbcsCodePage);
}

int Editor::CodePage() const noexcept {
	if (pdoc)
		return pdoc->dbcsCodePage;
	else
		return 0;
}

std::unique_ptr<Surface> Editor::CreateMeasurementSurface() const {
	if (!wMain.GetID()) {
		return {};
	}
	std::unique_ptr<Surface> surf = Surface::Allocate(technology);
	surf->Init(wMain.GetID());
	surf->SetMode(CurrentSurfaceMode());
	return surf;
}

std::unique_ptr<Surface> Editor::CreateDrawingSurface(SurfaceID sid, std::optional<Scintilla::Technology> technologyOpt) const {
	if (!wMain.GetID()) {
		return {};
	}
	std::unique_ptr<Surface> surf = Surface::Allocate(technologyOpt ? *technologyOpt : technology);
	surf->Init(sid, wMain.GetID());
	surf->SetMode(CurrentSurfaceMode());
	return surf;
}

Sci::Line Editor::WrapCount(Sci::Line line) {
	AutoSurface surface(this);
	std::shared_ptr<LineLayout> ll = view.RetrieveLineLayout(line, *this);

	if (surface && ll) {
		view.LayoutLine(*this, surface, vs, ll.get(), wrapWidth);
		return ll->lines;
	} else {
		return 1;
	}
}

void Editor::AddStyledText(const char *buffer, Sci::Position appendLength) {
	// The buffer consists of alternating character bytes and style bytes
	const Sci::Position textLength = appendLength / 2;
	std::string text(textLength, '\0');
	for (Sci::Position i = 0; i < textLength; i++) {
		text[i] = buffer[i*2];
	}
	const Sci::Position lengthInserted = pdoc->InsertString(CurrentPosition(), text);
	for (Sci::Position i = 0; i < textLength; i++) {
		text[i] = buffer[i*2+1];
	}
	pdoc->StartStyling(CurrentPosition());
	pdoc->SetStyles(textLength, text.c_str());
	SetEmptySelection(sel.MainCaret() + lengthInserted);
}

Sci::Position Editor::GetStyledText(char *buffer, Sci::Position cpMin, Sci::Position cpMax) const noexcept {
	Sci::Position iPlace = 0;
	for (Sci::Position iChar = cpMin; iChar < cpMax; iChar++) {
		buffer[iPlace++] = pdoc->CharAt(iChar);
		buffer[iPlace++] = pdoc->StyleAtNoExcept(iChar);
	}
	buffer[iPlace] = '\0';
	buffer[iPlace + 1] = '\0';
	return iPlace;
}

Sci::Position Editor::GetTextRange(char *buffer, Sci::Position cpMin, Sci::Position cpMax) const {
	const Sci::Position cpEnd = (cpMax == -1) ? pdoc->Length() : cpMax;
	PLATFORM_ASSERT(cpEnd <= pdoc->Length());
	const Sci::Position len = cpEnd - cpMin; 	// No -1 as cpMin and cpMax are referring to inter character positions
	pdoc->GetCharRange(buffer, cpMin, len);
	// Spec says copied text is terminated with a NUL
	buffer[len] = '\0';
	return len; 	// Not including NUL
}

bool Editor::ValidMargin(uptr_t wParam) const noexcept {
	return wParam < vs.ms.size();
}

void Editor::StyleSetMessage(Message iMessage, uptr_t wParam, sptr_t lParam) {
	vs.EnsureStyle(wParam);
	switch (iMessage) {
	case Message::StyleSetFore:
		vs.styles[wParam].fore = ColourRGBA::FromIpRGB(lParam);
		break;
	case Message::StyleSetBack:
		vs.styles[wParam].back = ColourRGBA::FromIpRGB(lParam);
		break;
	case Message::StyleSetBold:
		vs.styles[wParam].weight = lParam != 0 ? FontWeight::Bold : FontWeight::Normal;
		break;
	case Message::StyleSetWeight:
		vs.styles[wParam].weight = static_cast<FontWeight>(lParam);
		break;
	case Message::StyleSetStretch:
		vs.styles[wParam].stretch = static_cast<FontStretch>(lParam);
		break;
	case Message::StyleSetItalic:
		vs.styles[wParam].italic = lParam != 0;
		break;
	case Message::StyleSetEOLFilled:
		vs.styles[wParam].eolFilled = lParam != 0;
		break;
	case Message::StyleSetSize:
		vs.styles[wParam].size = static_cast<int>(lParam * FontSizeMultiplier);
		break;
	case Message::StyleSetSizeFractional:
		vs.styles[wParam].size = static_cast<int>(lParam);
		break;
	case Message::StyleSetFont:
		if (lParam != 0) {
			vs.SetStyleFontName(static_cast<int>(wParam), ConstCharPtrFromSPtr(lParam));
		}
		break;
	case Message::StyleSetUnderline:
		vs.styles[wParam].underline = lParam != 0;
		break;
	case Message::StyleSetCase:
		vs.styles[wParam].caseForce = static_cast<Style::CaseForce>(lParam);
		break;
	case Message::StyleSetCharacterSet:
		vs.styles[wParam].characterSet = static_cast<CharacterSet>(lParam);
		pdoc->SetCaseFolder(nullptr);
		break;
	case Message::StyleSetVisible:
		vs.styles[wParam].visible = lParam != 0;
		break;
	case Message::StyleSetInvisibleRepresentation: {
		const char *utf8 = ConstCharPtrFromSPtr(lParam);
		char *rep = vs.styles[wParam].invisibleRepresentation;
		const int classified = UTF8Classify(utf8);
		if (!(classified & UTF8MaskInvalid)) {
			// valid UTF-8
			const int len = classified & UTF8MaskWidth;
			for (int i=0; i<len && i<UTF8MaxBytes; i++)
				*rep++ = *utf8++;
		}
		*rep = 0;
		break;
	}
	case Message::StyleSetChangeable:
		vs.styles[wParam].changeable = lParam != 0;
		break;
	case Message::StyleSetHotSpot:
		vs.styles[wParam].hotspot = lParam != 0;
		break;
	case Message::StyleSetCheckMonospaced:
		vs.styles[wParam].checkMonospaced = lParam != 0;
		break;
	default:
		break;
	}
	InvalidateStyleRedraw();
}

sptr_t Editor::StyleGetMessage(Message iMessage, uptr_t wParam, sptr_t lParam) {
	vs.EnsureStyle(wParam);
	switch (iMessage) {
	case Message::StyleGetFore:
		return vs.styles[wParam].fore.OpaqueRGB();
	case Message::StyleGetBack:
		return vs.styles[wParam].back.OpaqueRGB();
	case Message::StyleGetBold:
		return vs.styles[wParam].weight > FontWeight::Normal;
	case Message::StyleGetWeight:
		return static_cast<sptr_t>(vs.styles[wParam].weight);
	case Message::StyleGetStretch:
		return static_cast<sptr_t>(vs.styles[wParam].stretch);
	case Message::StyleGetItalic:
		return vs.styles[wParam].italic ? 1 : 0;
	case Message::StyleGetEOLFilled:
		return vs.styles[wParam].eolFilled ? 1 : 0;
	case Message::StyleGetSize:
		return vs.styles[wParam].size / FontSizeMultiplier;
	case Message::StyleGetSizeFractional:
		return vs.styles[wParam].size;
	case Message::StyleGetFont:
		return StringResult(lParam, vs.styles[wParam].fontName);
	case Message::StyleGetUnderline:
		return vs.styles[wParam].underline ? 1 : 0;
	case Message::StyleGetCase:
		return static_cast<int>(vs.styles[wParam].caseForce);
	case Message::StyleGetCharacterSet:
		return static_cast<sptr_t>(vs.styles[wParam].characterSet);
	case Message::StyleGetVisible:
		return vs.styles[wParam].visible ? 1 : 0;
	case Message::StyleGetChangeable:
		return vs.styles[wParam].changeable ? 1 : 0;
	case Message::StyleGetInvisibleRepresentation:
		return StringResult(lParam, vs.styles[wParam].invisibleRepresentation);
	case Message::StyleGetHotSpot:
		return vs.styles[wParam].hotspot ? 1 : 0;
	case Message::StyleGetCheckMonospaced:
		return vs.styles[wParam].checkMonospaced ? 1 : 0;
	default:
		break;
	}
	return 0;
}

void Editor::SetSelectionNMessage(Message iMessage, uptr_t wParam, sptr_t lParam) {
	if (wParam >= sel.Count()) {
		return;
	}
	InvalidateRange(sel.Range(wParam).Start().Position(), sel.Range(wParam).End().Position());

	switch (iMessage) {
	case Message::SetSelectionNCaret:
		sel.Range(wParam).caret.SetPosition(lParam);
		break;

	case Message::SetSelectionNAnchor:
		sel.Range(wParam).anchor.SetPosition(lParam);
		break;

	case Message::SetSelectionNCaretVirtualSpace:
		sel.Range(wParam).caret.SetVirtualSpace(lParam);
		break;

	case Message::SetSelectionNAnchorVirtualSpace:
		sel.Range(wParam).anchor.SetVirtualSpace(lParam);
		break;

	case Message::SetSelectionNStart:
		sel.Range(wParam).anchor.SetPosition(lParam);
		break;

	case Message::SetSelectionNEnd:
		sel.Range(wParam).caret.SetPosition(lParam);
		break;

	default:
		break;

	}

	InvalidateRange(sel.Range(wParam).Start().Position(), sel.Range(wParam).End().Position());
	ContainerNeedsUpdate(Update::Selection);
}

namespace {

constexpr Selection::SelTypes SelTypeFromMode(SelectionMode mode) {
	switch (mode) {
	case SelectionMode::Rectangle:
		return Selection::SelTypes::rectangle;
	case SelectionMode::Lines:
		return Selection::SelTypes::lines;
	case SelectionMode::Thin:
		return Selection::SelTypes::thin;
	case SelectionMode::Stream:
	default:
		return Selection::SelTypes::stream;
	}
}

sptr_t SPtrFromPtr(void *ptr) noexcept {
	return reinterpret_cast<sptr_t>(ptr);
}

}

void Editor::SetSelectionMode(uptr_t wParam, bool setMoveExtends) {
	const Selection::SelTypes newSelType = SelTypeFromMode(static_cast<SelectionMode>(wParam));
	if (setMoveExtends) {
		sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != newSelType));
	}
	sel.selType = newSelType;
	switch (sel.selType) {
	case Selection::SelTypes::rectangle:
		sel.Rectangular() = sel.RangeMain(); // adjust current selection
		break;
	case Selection::SelTypes::lines:
		SetSelection(sel.RangeMain().caret, sel.RangeMain().anchor); // adjust current selection
		break;
	default:
		break;
	}
	InvalidateWholeSelection();
}

sptr_t Editor::StringResult(sptr_t lParam, const char *val) noexcept {
	const size_t len = val ? strlen(val) : 0;
	if (lParam) {
		char *ptr = CharPtrFromSPtr(lParam);
		if (val)
			memcpy(ptr, val, len+1);
		else
			*ptr = 0;
	}
	return len;	// Not including NUL
}

sptr_t Editor::BytesResult(sptr_t lParam, const unsigned char *val, size_t len) noexcept {
	// No NUL termination: len is number of valid/displayed bytes
	if ((lParam) && (len > 0)) {
		char *ptr = CharPtrFromSPtr(lParam);
		if (val)
			memcpy(ptr, val, len);
		else
			*ptr = 0;
	}
	return val ? len : 0;
}

sptr_t Editor::BytesResult(Scintilla::sptr_t lParam, std::string_view sv) noexcept {
	// No NUL termination: sv.length() is number of valid/displayed bytes
	if (lParam && !sv.empty()) {
		char *ptr = CharPtrFromSPtr(lParam);
		memcpy(ptr, sv.data(), sv.length());
	}
	return sv.length();
}

sptr_t Editor::WndProc(Message iMessage, uptr_t wParam, sptr_t lParam) {
	//Platform::DebugPrintf("S start wnd proc %d %d %d\n",iMessage, wParam, lParam);

	// Optional macro recording hook
	if (recordingMacro)
		NotifyMacroRecord(iMessage, wParam, lParam);

	switch (iMessage) {

	case Message::GetText: {
			if (lParam == 0)
				return pdoc->Length();
			char *ptr = CharPtrFromSPtr(lParam);
			const Sci_Position len = std::min<Sci_Position>(wParam, pdoc->Length());
			pdoc->GetCharRange(ptr, 0, len);
			ptr[len] = '\0';
			return len;
		}

	case Message::SetText: {
			if (lParam == 0)
				return 0;
			UndoGroup ug(pdoc);
			pdoc->DeleteChars(0, pdoc->Length());
			SetEmptySelection(0);
			const char *text = ConstCharPtrFromSPtr(lParam);
			pdoc->InsertString(0, text, strlen(text));
			return 1;
		}

	case Message::GetTextLength:
		return pdoc->Length();

	case Message::Cut:
		Cut();
		SetLastXChosen();
		break;

	case Message::Copy:
		Copy();
		break;

	case Message::CopyAllowLine:
		CopyAllowLine();
		break;

	case Message::CutAllowLine:
		CutAllowLine();
		SetLastXChosen();
		break;

	case Message::GetCopySeparator:
		return StringResult(lParam, copySeparator.c_str());

	case Message::SetCopySeparator:
		copySeparator = ConstCharPtrFromSPtr(lParam);
		break;

	case Message::VerticalCentreCaret:
		VerticalCentreCaret();
		break;

	case Message::MoveSelectedLinesUp:
		MoveSelectedLinesUp();
		break;

	case Message::MoveSelectedLinesDown:
		MoveSelectedLinesDown();
		break;

	case Message::CopyRange:
		CopyRangeToClipboard(PositionFromUPtr(wParam), lParam);
		break;

	case Message::CopyText:
		CopyText(wParam, ConstCharPtrFromSPtr(lParam));
		break;

	case Message::Paste:
		Paste();
		if ((caretSticky == CaretSticky::Off) || (caretSticky == CaretSticky::WhiteSpace)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;

	case Message::ReplaceRectangular: {
		UndoGroup ug(pdoc);
		if (!sel.Empty()) {
			ClearSelection(); // want to replace rectangular selection contents
		}
		InsertPasteShape(ConstCharPtrFromSPtr(lParam), PositionFromUPtr(wParam), PasteShape::rectangular);
		break;
	}

	case Message::Clear:
		Clear();
		SetLastXChosen();
		EnsureCaretVisible();
		break;

	case Message::Undo:
		Undo();
		SetLastXChosen();
		break;

	case Message::CanUndo:
		return (pdoc->CanUndo() && !pdoc->IsReadOnly()) ? 1 : 0;

	case Message::EmptyUndoBuffer:
		pdoc->DeleteUndoHistory();
		return 0;

	case Message::GetFirstVisibleLine:
		return topLine;

	case Message::SetFirstVisibleLine:
		ScrollTo(LineFromUPtr(wParam));
		break;

	case Message::GetLine: {	// Risk of overwriting the end of the buffer
			const Sci::Position lineStart =
				pdoc->LineStart(LineFromUPtr(wParam));
			const Sci::Position lineEnd =
				pdoc->LineStart(LineFromUPtr(wParam + 1));
			// not NUL terminated
			const Sci::Position len = lineEnd - lineStart;
			if (lParam == 0) {
				return len;
			}
			char *ptr = CharPtrFromSPtr(lParam);
			pdoc->GetCharRange(ptr, lineStart, len);
			return len;
		}

	case Message::GetLineCount:
		if (pdoc->LinesTotal() == 0)
			return 1;
		else
			return pdoc->LinesTotal();

	case Message::AllocateLines:
		pdoc->AllocateLines(wParam);
		break;

	case Message::GetModify:
		return !pdoc->IsSavePoint();

	case Message::SetSel: {
			Sci::Position nStart = PositionFromUPtr(wParam);
			Sci::Position nEnd = lParam;
			if (nEnd < 0)
				nEnd = pdoc->Length();
			if (nStart < 0)
				nStart = nEnd; 	// Remove selection
			InvalidateSelection(SelectionRange(nStart, nEnd));
			sel.Clear();
			sel.selType = Selection::SelTypes::stream;
			SetSelection(nEnd, nStart);
			EnsureCaretVisible();
		}
		break;

	case Message::GetSelText: {
			SelectionText selectedText;
			CopySelectionRange(&selectedText);
			if (lParam) {
				char *ptr = CharPtrFromSPtr(lParam);
				size_t iChar = selectedText.Length();
				if (iChar) {
					memcpy(ptr, selectedText.Data(), iChar);
				}
				ptr[iChar] = '\0';
			}
			return selectedText.Length();
	}

	case Message::LineFromPosition:
		if (PositionFromUPtr(wParam) < 0)
			return 0;
		return pdoc->LineFromPosition(PositionFromUPtr(wParam));

	case Message::PositionFromLine:
		if (LineFromUPtr(wParam) < 0)
			wParam = pdoc->LineFromPosition(SelectionStart().Position());
		if (wParam == 0)
			return 0; 	// Even if there is no text, there is a first line that starts at 0
		if (LineFromUPtr(wParam) > pdoc->LinesTotal())
			return -1;
		//if (wParam > pdoc->LineFromPosition(pdoc->Length()))	// Useful test, anyway...
		//	return -1;
		return pdoc->LineStart(LineFromUPtr(wParam));

		// Replacement of the old Scintilla interpretation of EM_LINELENGTH
	case Message::LineLength:
		if ((LineFromUPtr(wParam) < 0) ||
		        (LineFromUPtr(wParam) > pdoc->LineFromPosition(pdoc->Length())))
			return 0;
		return pdoc->LineStart(LineFromUPtr(wParam) + 1) - pdoc->LineStart(LineFromUPtr(wParam));

	case Message::ReplaceSel: {
			if (lParam == 0)
				return 0;
			UndoGroup ug(pdoc);
			ClearSelection();
			const char *replacement = ConstCharPtrFromSPtr(lParam);
			const Sci::Position lengthInserted = pdoc->InsertString(
				sel.MainCaret(), replacement, strlen(replacement));
			SetEmptySelection(sel.MainCaret() + lengthInserted);
			SetLastXChosen();
			EnsureCaretVisible();
		}
		break;

	case Message::SetTargetStart:
		targetRange.start.SetPosition(PositionFromUPtr(wParam));
		break;

	case Message::GetTargetStart:
		return targetRange.start.Position();

	case Message::SetTargetStartVirtualSpace:
		targetRange.start.SetVirtualSpace(PositionFromUPtr(wParam));
		break;

	case Message::GetTargetStartVirtualSpace:
		return targetRange.start.VirtualSpace();

	case Message::SetTargetEnd:
		targetRange.end.SetPosition(PositionFromUPtr(wParam));
		break;

	case Message::GetTargetEnd:
		return targetRange.end.Position();

	case Message::SetTargetEndVirtualSpace:
		targetRange.end.SetVirtualSpace(PositionFromUPtr(wParam));
		break;

	case Message::GetTargetEndVirtualSpace:
		return targetRange.end.VirtualSpace();

	case Message::SetTargetRange:
		targetRange.start.SetPosition(PositionFromUPtr(wParam));
		targetRange.end.SetPosition(lParam);
		break;

	case Message::TargetWholeDocument:
		targetRange.start.SetPosition(0);
		targetRange.end.SetPosition(pdoc->Length());
		break;

	case Message::TargetFromSelection:
		targetRange.start = sel.RangeMain().Start();
		targetRange.end = sel.RangeMain().End();
		break;

	case Message::GetTargetText: {
			const std::string text = RangeText(targetRange.start.Position(), targetRange.end.Position());
			return BytesResult(lParam, text);
		}

	case Message::ReplaceTarget:
		PLATFORM_ASSERT(lParam);
		return ReplaceTarget(ReplaceType::basic, ViewFromParams(lParam, wParam));

	case Message::ReplaceTargetRE:
		PLATFORM_ASSERT(lParam);
		return ReplaceTarget(ReplaceType::patterns, ViewFromParams(lParam, wParam));

	case Message::ReplaceTargetMinimal:
		PLATFORM_ASSERT(lParam);
		return ReplaceTarget(ReplaceType::minimal, ViewFromParams(lParam, wParam));

	case Message::SearchInTarget:
		PLATFORM_ASSERT(lParam);
		return SearchInTarget(ConstCharPtrFromSPtr(lParam), PositionFromUPtr(wParam));

	case Message::SetSearchFlags:
		searchFlags = static_cast<FindOption>(wParam);
		break;

	case Message::GetSearchFlags:
		return static_cast<sptr_t>(searchFlags);

	case Message::GetTag:
		return GetTag(CharPtrFromSPtr(lParam), static_cast<int>(wParam));

	case Message::PositionBefore:
		return pdoc->MovePositionOutsideChar(PositionFromUPtr(wParam) - 1, -1, true);

	case Message::PositionAfter:
		return pdoc->MovePositionOutsideChar(PositionFromUPtr(wParam) + 1, 1, true);

	case Message::PositionRelative:
		return std::clamp<Sci::Position>(pdoc->GetRelativePosition(
			PositionFromUPtr(wParam), lParam),
			0, pdoc->Length());

	case Message::PositionRelativeCodeUnits:
		return std::clamp<Sci::Position>(pdoc->GetRelativePositionUTF16(
			PositionFromUPtr(wParam), lParam),
			0, pdoc->Length());

	case Message::LineScroll:
		ScrollTo(topLine + lParam);
		HorizontalScrollTo(xOffset + static_cast<int>(static_cast<int>(wParam) * vs.spaceWidth));
		return 1;

	case Message::SetXOffset:
		xOffset = static_cast<int>(wParam);
		ContainerNeedsUpdate(Update::HScroll);
		SetHorizontalScrollPos();
		Redraw();
		break;

	case Message::GetXOffset:
		return xOffset;

	case Message::ChooseCaretX:
		SetLastXChosen();
		break;

	case Message::ScrollCaret:
		EnsureCaretVisible();
		break;

	case Message::SetReadOnly:
		pdoc->SetReadOnly(wParam != 0);
		return 1;

	case Message::GetReadOnly:
		return pdoc->IsReadOnly();

	case Message::CanPaste:
		return CanPaste();

	case Message::PointXFromPosition:
		if (lParam < 0) {
			return 0;
		} else {
			const Point pt = LocationFromPosition(lParam);
			// Convert to view-relative
			return static_cast<int>(pt.x) - vs.textStart + vs.fixedColumnWidth;
		}

	case Message::PointYFromPosition:
		if (lParam < 0) {
			return 0;
		} else {
			const Point pt = LocationFromPosition(lParam);
			return static_cast<int>(pt.y);
		}

	case Message::FindText:
		return FindText(wParam, lParam);

	case Message::FindTextFull:
		return FindTextFull(wParam, lParam);

	case Message::GetTextRange:
		if (TextRange *tr = static_cast<TextRange *>(PtrFromSPtr(lParam))) {
			return GetTextRange(tr->lpstrText, tr->chrg.cpMin, tr->chrg.cpMax);
		}
		return 0;

	case Message::GetTextRangeFull:
		if (TextRangeFull *tr = static_cast<TextRangeFull *>(PtrFromSPtr(lParam))) {
			return GetTextRange(tr->lpstrText, tr->chrg.cpMin, tr->chrg.cpMax);
		}
		return 0;

	case Message::HideSelection:
		vs.selection.visible = wParam == 0;
		Redraw();
		break;

	case Message::GetSelectionHidden:
		return !vs.selection.visible;
		break;

	case Message::FormatRange:
	case Message::FormatRangeFull:
		return FormatRange(iMessage, wParam, lParam);

	case Message::GetMarginLeft:
		return vs.leftMarginWidth;

	case Message::GetMarginRight:
		return vs.rightMarginWidth;

	case Message::SetMarginLeft:
		lastXChosen += static_cast<int>(lParam) - vs.leftMarginWidth;
		vs.leftMarginWidth = static_cast<int>(lParam);
		InvalidateStyleRedraw();
		break;

	case Message::SetMarginRight:
		vs.rightMarginWidth = static_cast<int>(lParam);
		InvalidateStyleRedraw();
		break;

		// Control specific messages

	case Message::AddText: {
			if (lParam == 0)
				return 0;
			const Sci::Position lengthInserted = pdoc->InsertString(
				CurrentPosition(), ConstCharPtrFromSPtr(lParam), PositionFromUPtr(wParam));
			SetEmptySelection(sel.MainCaret() + lengthInserted);
			return 0;
		}

	case Message::AddStyledText:
		if (lParam)
			AddStyledText(ConstCharPtrFromSPtr(lParam), PositionFromUPtr(wParam));
		return 0;

	case Message::InsertText: {
			if (lParam == 0)
				return 0;
			Sci::Position insertPos = PositionFromUPtr(wParam);
			if (insertPos == -1)
				insertPos = CurrentPosition();
			Sci::Position newCurrent = CurrentPosition();
			const char *sz = ConstCharPtrFromSPtr(lParam);
			const Sci::Position lengthInserted = pdoc->InsertString(insertPos, sz, strlen(sz));
			if (newCurrent > insertPos)
				newCurrent += lengthInserted;
			SetEmptySelection(newCurrent);
			return 0;
		}

	case Message::ChangeInsertion:
		PLATFORM_ASSERT(lParam);
		pdoc->ChangeInsertion(ConstCharPtrFromSPtr(lParam), PositionFromUPtr(wParam));
		return 0;

	case Message::AppendText:
		pdoc->InsertString(pdoc->Length(),
			ConstCharPtrFromSPtr(lParam), PositionFromUPtr(wParam));
		return 0;

	case Message::ClearAll:
		ClearAll();
		return 0;

	case Message::DeleteRange:
		pdoc->DeleteChars(PositionFromUPtr(wParam), lParam);
		return 0;

	case Message::ClearDocumentStyle:
		ClearDocumentStyle();
		return 0;

	case Message::SetUndoCollection:
		pdoc->SetUndoCollection(wParam != 0);
		return 0;

	case Message::GetUndoCollection:
		return pdoc->IsCollectingUndo();

	case Message::BeginUndoAction:
		pdoc->BeginUndoAction();
		return 0;

	case Message::EndUndoAction:
		pdoc->EndUndoAction();
		return 0;

	case Message::GetUndoSequence:
		return pdoc->UndoSequenceDepth();

	case Message::GetUndoActions:
		return pdoc->UndoActions();

	case Message::SetUndoSavePoint:
		pdoc->SetUndoSavePoint(static_cast<int>(wParam));
		break;

	case Message::GetUndoSavePoint:
		return pdoc->UndoSavePoint();

	case Message::SetUndoDetach:
		pdoc->SetUndoDetach(static_cast<int>(wParam));
		break;

	case Message::GetUndoDetach:
		return pdoc->UndoDetach();

	case Message::SetUndoTentative:
		pdoc->SetUndoTentative(static_cast<int>(wParam));
		break;

	case Message::GetUndoTentative:
		return pdoc->UndoTentative();

	case Message::SetUndoCurrent:
		pdoc->SetUndoCurrent(static_cast<int>(wParam));
		break;

	case Message::GetUndoCurrent:
		return pdoc->UndoCurrent();

	case Message::GetUndoActionType:
		return pdoc->UndoActionType(static_cast<int>(wParam));

	case Message::GetUndoActionPosition:
		return pdoc->UndoActionPosition(static_cast<int>(wParam));

	case Message::GetUndoActionText: {
		const std::string_view text = pdoc->UndoActionText(static_cast<int>(wParam));
		return BytesResult(lParam, text);
	}

	case Message::PushUndoActionType:
		pdoc->PushUndoActionType(static_cast<int>(wParam), lParam);
		break;

	case Message::ChangeLastUndoActionText:
		pdoc->ChangeLastUndoActionText(wParam, CharPtrFromSPtr(lParam));
		break;

	case Message::GetCaretPeriod:
		return caret.period;

	case Message::SetCaretPeriod:
		CaretSetPeriod(static_cast<int>(wParam));
		break;

	case Message::GetWordChars:
		return pdoc->GetCharsOfClass(CharacterClass::word, UCharPtrFromSPtr(lParam));

	case Message::SetWordChars: {
			pdoc->SetDefaultCharClasses(false);
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(ConstUCharPtrFromSPtr(lParam), CharacterClass::word);
		}
		break;

	case Message::GetWhitespaceChars:
		return pdoc->GetCharsOfClass(CharacterClass::space, UCharPtrFromSPtr(lParam));

	case Message::SetWhitespaceChars: {
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(ConstUCharPtrFromSPtr(lParam), CharacterClass::space);
		}
		break;

	case Message::GetPunctuationChars:
		return pdoc->GetCharsOfClass(CharacterClass::punctuation, UCharPtrFromSPtr(lParam));

	case Message::SetPunctuationChars: {
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(ConstUCharPtrFromSPtr(lParam), CharacterClass::punctuation);
		}
		break;

	case Message::SetCharsDefault:
		pdoc->SetDefaultCharClasses(true);
		break;

	case Message::SetCharacterCategoryOptimization:
		pdoc->SetCharacterCategoryOptimization(static_cast<int>(wParam));
		break;

	case Message::GetCharacterCategoryOptimization:
		return pdoc->CharacterCategoryOptimization();

	case Message::GetLength:
		return pdoc->Length();

	case Message::Allocate:
		pdoc->Allocate(PositionFromUPtr(wParam));
		break;

	case Message::GetCharAt:
		return pdoc->CharAt(PositionFromUPtr(wParam));

	case Message::SetCurrentPos:
		if (sel.IsRectangular()) {
			sel.Rectangular().caret.SetPosition(PositionFromUPtr(wParam));
			SetRectangularRange();
			Redraw();
		} else {
			SetSelection(PositionFromUPtr(wParam), sel.MainAnchor());
		}
		break;

	case Message::GetCurrentPos:
		return sel.IsRectangular() ? sel.Rectangular().caret.Position() : sel.MainCaret();

	case Message::SetAnchor:
		if (sel.IsRectangular()) {
			sel.Rectangular().anchor.SetPosition(PositionFromUPtr(wParam));
			SetRectangularRange();
			Redraw();
		} else {
			SetSelection(sel.MainCaret(), PositionFromUPtr(wParam));
		}
		break;

	case Message::GetAnchor:
		return sel.IsRectangular() ? sel.Rectangular().anchor.Position() : sel.MainAnchor();

	case Message::SetSelectionStart:
		SetSelection(std::max(sel.MainCaret(), PositionFromUPtr(wParam)), PositionFromUPtr(wParam));
		break;

	case Message::GetSelectionStart:
		return sel.LimitsForRectangularElseMain().start.Position();

	case Message::SetSelectionEnd:
		SetSelection(PositionFromUPtr(wParam), std::min(sel.MainAnchor(), PositionFromUPtr(wParam)));
		break;

	case Message::GetSelectionEnd:
		return sel.LimitsForRectangularElseMain().end.Position();

	case Message::SetEmptySelection:
		SetEmptySelection(PositionFromUPtr(wParam));
		break;

	case Message::SetPrintMagnification:
		view.printParameters.magnification = static_cast<int>(wParam);
		break;

	case Message::GetPrintMagnification:
		return view.printParameters.magnification;

	case Message::SetPrintColourMode:
		view.printParameters.colourMode = static_cast<PrintOption>(wParam);
		break;

	case Message::GetPrintColourMode:
		return static_cast<sptr_t>(view.printParameters.colourMode);

	case Message::SetPrintWrapMode:
		view.printParameters.wrapState = (static_cast<Wrap>(wParam) == Wrap::Word) ? Wrap::Word : Wrap::None;
		break;

	case Message::GetPrintWrapMode:
		return static_cast<sptr_t>(view.printParameters.wrapState);

	case Message::GetStyleAt:
		if (PositionFromUPtr(wParam) >= pdoc->Length())
			return 0;
		else
			return pdoc->StyleAt(PositionFromUPtr(wParam));

	case Message::GetStyleIndexAt:
		if (PositionFromUPtr(wParam) >= pdoc->Length())
			return 0;
		else
			return pdoc->StyleIndexAt(PositionFromUPtr(wParam));

	case Message::Redo:
		Redo();
		break;

	case Message::SelectAll:
		SelectAll();
		break;

	case Message::SetSavePoint:
		pdoc->SetSavePoint();
		break;

	case Message::GetStyledText:
		if (TextRange *tr = static_cast<TextRange *>(PtrFromSPtr(lParam))) {
			return GetStyledText(tr->lpstrText, tr->chrg.cpMin, tr->chrg.cpMax);
		}
		return 0;

	case Message::GetStyledTextFull:
		if (TextRangeFull *tr = static_cast<TextRangeFull *>(PtrFromSPtr(lParam))) {
			return GetStyledText(tr->lpstrText, tr->chrg.cpMin, tr->chrg.cpMax);
		}
		return 0;

	case Message::CanRedo:
		return (pdoc->CanRedo() && !pdoc->IsReadOnly()) ? 1 : 0;

	case Message::MarkerLineFromHandle:
		return pdoc->LineFromHandle(static_cast<int>(wParam));

	case Message::MarkerDeleteHandle:
		pdoc->DeleteMarkFromHandle(static_cast<int>(wParam));
		break;

	case Message::MarkerHandleFromLine:
		return pdoc->MarkerHandleFromLine(LineFromUPtr(wParam), static_cast<int>(lParam));

	case Message::MarkerNumberFromLine:
		return pdoc->MarkerNumberFromLine(LineFromUPtr(wParam), static_cast<int>(lParam));

	case Message::GetViewWS:
		return static_cast<sptr_t>(vs.viewWhitespace);

	case Message::SetViewWS:
		vs.viewWhitespace = static_cast<WhiteSpace>(wParam);
		Redraw();
		break;

	case Message::GetTabDrawMode:
		return static_cast<sptr_t>(vs.tabDrawMode);

	case Message::SetTabDrawMode:
		vs.tabDrawMode = static_cast<TabDrawMode>(wParam);
		Redraw();
		break;

	case Message::GetWhitespaceSize:
		return vs.whitespaceSize;

	case Message::SetWhitespaceSize:
		vs.whitespaceSize = static_cast<int>(wParam);
		Redraw();
		break;

	case Message::PositionFromPoint:
		return PositionFromLocation(PointFromParameters(wParam, lParam), false, false);

	case Message::PositionFromPointClose:
		return PositionFromLocation(PointFromParameters(wParam, lParam), true, false);

	case Message::CharPositionFromPoint:
		return PositionFromLocation(PointFromParameters(wParam, lParam), false, true);

	case Message::CharPositionFromPointClose:
		return PositionFromLocation(PointFromParameters(wParam, lParam), true, true);

	case Message::GotoLine:
		GoToLine(LineFromUPtr(wParam));
		break;

	case Message::GotoPos:
		SetEmptySelection(PositionFromUPtr(wParam));
		EnsureCaretVisible();
		break;

	case Message::GetCurLine: {
			const Sci::Line lineCurrentPos = pdoc->SciLineFromPosition(sel.MainCaret());
			const Sci::Position lineStart = pdoc->LineStart(lineCurrentPos);
			const Sci::Position lineEnd = pdoc->LineStart(lineCurrentPos + 1);
			if (lParam == 0) {
				return lineEnd - lineStart;
			}
			char *ptr = CharPtrFromSPtr(lParam);
			const Sci::Position len = std::min<uptr_t>(lineEnd - lineStart, wParam);
			pdoc->GetCharRange(ptr, lineStart, len);
			ptr[len] = '\0';
			return sel.MainCaret() - lineStart;
		}

	case Message::GetEndStyled:
		return pdoc->GetEndStyled();

	case Message::GetEOLMode:
		return static_cast<sptr_t>(pdoc->eolMode);

	case Message::SetEOLMode:
		pdoc->eolMode = static_cast<EndOfLine>(wParam);
		break;

	case Message::SetLineEndTypesAllowed:
		if (pdoc->SetLineEndTypesAllowed(static_cast<LineEndType>(wParam))) {
			pcs->Clear();
			pcs->InsertLines(0, pdoc->LinesTotal() - 1);
			SetAnnotationHeights(0, pdoc->LinesTotal());
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetLineEndTypesAllowed:
		return static_cast<sptr_t>(pdoc->GetLineEndTypesAllowed());

	case Message::GetLineEndTypesActive:
		return static_cast<sptr_t>(pdoc->GetLineEndTypesActive());

	case Message::StartStyling:
		pdoc->StartStyling(PositionFromUPtr(wParam));
		break;

	case Message::SetStyling:
		if (PositionFromUPtr(wParam) < 0)
			errorStatus = Status::Failure;
		else
			pdoc->SetStyleFor(PositionFromUPtr(wParam), static_cast<char>(lParam));
		break;

	case Message::SetStylingEx:             // Specify a complete styling buffer
		if (lParam == 0)
			return 0;
		pdoc->SetStyles(PositionFromUPtr(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case Message::SetBufferedDraw:
		view.bufferedDraw = wParam != 0;
		break;

	case Message::GetBufferedDraw:
		return view.bufferedDraw;

#ifdef INCLUDE_DEPRECATED_FEATURES
	case SCI_GETTWOPHASEDRAW:
		return view.phasesDraw == EditView::phasesTwo;

	case SCI_SETTWOPHASEDRAW:
		if (view.SetTwoPhaseDraw(wParam != 0))
			InvalidateStyleRedraw();
		break;
#endif

	case Message::GetPhasesDraw:
		return static_cast<sptr_t>(view.phasesDraw);

	case Message::SetPhasesDraw:
		if (view.SetPhasesDraw(static_cast<int>(wParam)))
			InvalidateStyleRedraw();
		break;

	case Message::SetFontQuality:
		vs.extraFontFlag = static_cast<FontQuality>(
			(static_cast<int>(vs.extraFontFlag) & ~static_cast<int>(FontQuality::QualityMask)) |
			(wParam & static_cast<int>(FontQuality::QualityMask)));
		InvalidateStyleRedraw();
		break;

	case Message::GetFontQuality:
		return static_cast<int>(vs.extraFontFlag) & static_cast<int>(FontQuality::QualityMask);

	case Message::SetTabWidth:
		if (wParam > 0) {
			pdoc->tabInChars = static_cast<int>(wParam);
			if (pdoc->indentInChars == 0)
				pdoc->actualIndentInChars = pdoc->tabInChars;
		}
		InvalidateStyleRedraw();
		break;

	case Message::GetTabWidth:
		return pdoc->tabInChars;

	case Message::SetTabMinimumWidth:
		SetAppearance(view.tabWidthMinimumPixels, static_cast<int>(wParam));
		break;

	case Message::GetTabMinimumWidth:
		return view.tabWidthMinimumPixels;

	case Message::ClearTabStops:
		if (view.ClearTabstops(LineFromUPtr(wParam))) {
			const DocModification mh(ModificationFlags::ChangeTabStops, 0, 0, 0, nullptr, LineFromUPtr(wParam));
			NotifyModified(pdoc, mh, nullptr);
		}
		break;

	case Message::AddTabStop:
		if (view.AddTabstop(LineFromUPtr(wParam), static_cast<int>(lParam))) {
			const DocModification mh(ModificationFlags::ChangeTabStops, 0, 0, 0, nullptr, LineFromUPtr(wParam));
			NotifyModified(pdoc, mh, nullptr);
		}
		break;

	case Message::GetNextTabStop:
		return view.GetNextTabstop(LineFromUPtr(wParam), static_cast<int>(lParam));

	case Message::SetIndent:
		pdoc->indentInChars = static_cast<int>(wParam);
		if (pdoc->indentInChars != 0)
			pdoc->actualIndentInChars = pdoc->indentInChars;
		else
			pdoc->actualIndentInChars = pdoc->tabInChars;
		InvalidateStyleRedraw();
		break;

	case Message::GetIndent:
		return pdoc->indentInChars;

	case Message::SetUseTabs:
		pdoc->useTabs = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case Message::GetUseTabs:
		return pdoc->useTabs;

	case Message::SetLineIndentation:
		pdoc->SetLineIndentation(LineFromUPtr(wParam), lParam);
		break;

	case Message::GetLineIndentation:
		return pdoc->GetLineIndentation(LineFromUPtr(wParam));

	case Message::GetLineIndentPosition:
		return pdoc->GetLineIndentPosition(LineFromUPtr(wParam));

	case Message::SetTabIndents:
		pdoc->tabIndents = wParam != 0;
		break;

	case Message::GetTabIndents:
		return pdoc->tabIndents;

	case Message::SetBackSpaceUnIndents:
		pdoc->backspaceUnindents = wParam != 0;
		break;

	case Message::GetBackSpaceUnIndents:
		return pdoc->backspaceUnindents;

	case Message::SetMouseDwellTime:
		dwellDelay = static_cast<int>(wParam);
		ticksToDwell = dwellDelay;
		break;

	case Message::GetMouseDwellTime:
		return dwellDelay;

	case Message::WordStartPosition:
		return pdoc->ExtendWordSelect(PositionFromUPtr(wParam), -1, lParam != 0);

	case Message::WordEndPosition:
		return pdoc->ExtendWordSelect(PositionFromUPtr(wParam), 1, lParam != 0);

	case Message::IsRangeWord:
		return pdoc->IsWordAt(PositionFromUPtr(wParam), lParam);

	case Message::SetIdleStyling:
		idleStyling = static_cast<IdleStyling>(wParam);
		break;

	case Message::GetIdleStyling:
		return static_cast<sptr_t>(idleStyling);

	case Message::SetWrapMode:
		if (vs.SetWrapState(static_cast<Wrap>(wParam))) {
			xOffset = 0;
			ContainerNeedsUpdate(Update::HScroll);
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case Message::GetWrapMode:
		return static_cast<sptr_t>(vs.wrap.state);

	case Message::SetWrapVisualFlags:
		if (vs.SetWrapVisualFlags(static_cast<WrapVisualFlag>(wParam))) {
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case Message::GetWrapVisualFlags:
		return static_cast<sptr_t>(vs.wrap.visualFlags);

	case Message::SetWrapVisualFlagsLocation:
		if (vs.SetWrapVisualFlagsLocation(static_cast<WrapVisualLocation>(wParam))) {
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetWrapVisualFlagsLocation:
		return static_cast<sptr_t>(vs.wrap.visualFlagsLocation);

	case Message::SetWrapStartIndent:
		if (vs.SetWrapVisualStartIndent(static_cast<int>(wParam))) {
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case Message::GetWrapStartIndent:
		return vs.wrap.visualStartIndent;

	case Message::SetWrapIndentMode:
		if (vs.SetWrapIndentMode(static_cast<WrapIndentMode>(wParam))) {
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case Message::GetWrapIndentMode:
		return static_cast<sptr_t>(vs.wrap.indentMode);

	case Message::SetLayoutCache:
		if (static_cast<LineCache>(wParam) <= LineCache::Document) {
			view.llc.SetLevel(static_cast<LineCache>(wParam));
		}
		break;

	case Message::GetLayoutCache:
		return static_cast<sptr_t>(view.llc.GetLevel());

	case Message::SetPositionCache:
		view.posCache->SetSize(wParam);
		break;

	case Message::GetPositionCache:
		return view.posCache->GetSize();

	case Message::SetLayoutThreads:
		view.SetLayoutThreads(static_cast<unsigned int>(wParam));
		break;

	case Message::GetLayoutThreads:
		return view.GetLayoutThreads();

	case Message::SetScrollWidth:
		PLATFORM_ASSERT(wParam > 0);
		if ((wParam > 0) && (wParam != static_cast<unsigned int>(scrollWidth))) {
			view.lineWidthMaxSeen = 0;
			scrollWidth = static_cast<int>(wParam);
			SetScrollBars();
		}
		break;

	case Message::GetScrollWidth:
		return scrollWidth;

	case Message::SetScrollWidthTracking:
		trackLineWidth = wParam != 0;
		break;

	case Message::GetScrollWidthTracking:
		return trackLineWidth;

	case Message::LinesJoin:
		LinesJoin();
		break;

	case Message::LinesSplit:
		LinesSplit(static_cast<int>(wParam));
		break;

	case Message::TextWidth:
		PLATFORM_ASSERT(wParam < vs.styles.size());
		PLATFORM_ASSERT(lParam);
		return TextWidth(wParam, ConstCharPtrFromSPtr(lParam));

	case Message::TextHeight:
		RefreshStyleData();
		return vs.lineHeight;

	case Message::SetEndAtLastLine:
		PLATFORM_ASSERT((wParam == 0) || (wParam == 1));
		if (endAtLastLine != (wParam != 0)) {
			endAtLastLine = wParam != 0;
			SetScrollBars();
		}
		break;

	case Message::GetEndAtLastLine:
		return endAtLastLine;

	case Message::SetCaretSticky:
		PLATFORM_ASSERT(static_cast<CaretSticky>(wParam) <= CaretSticky::WhiteSpace);
		if (static_cast<CaretSticky>(wParam) <= CaretSticky::WhiteSpace) {
			caretSticky = static_cast<CaretSticky>(wParam);
		}
		break;

	case Message::GetCaretSticky:
		return static_cast<sptr_t>(caretSticky);

	case Message::ToggleCaretSticky:
		caretSticky = (caretSticky == CaretSticky::Off) ? CaretSticky::On : CaretSticky::Off;
		break;

	case Message::GetColumn:
		return pdoc->GetColumn(PositionFromUPtr(wParam));

	case Message::FindColumn:
		return pdoc->FindColumn(LineFromUPtr(wParam), lParam);

	case Message::SetHScrollBar :
		if (horizontalScrollBarVisible != (wParam != 0)) {
			horizontalScrollBarVisible = wParam != 0;
			SetScrollBars();
			ReconfigureScrollBars();
		}
		break;

	case Message::GetHScrollBar:
		return horizontalScrollBarVisible;

	case Message::SetVScrollBar:
		if (verticalScrollBarVisible != (wParam != 0)) {
			verticalScrollBarVisible = wParam != 0;
			SetScrollBars();
			ReconfigureScrollBars();
			if (verticalScrollBarVisible)
				SetVerticalScrollPos();
		}
		break;

	case Message::GetVScrollBar:
		return verticalScrollBarVisible;

	case Message::SetIndentationGuides:
		vs.viewIndentationGuides = static_cast<IndentView>(wParam);
		Redraw();
		break;

	case Message::GetIndentationGuides:
		return static_cast<sptr_t>(vs.viewIndentationGuides);

	case Message::SetHighlightGuide:
		if ((highlightGuideColumn != static_cast<int>(wParam)) || (wParam > 0)) {
			highlightGuideColumn = static_cast<int>(wParam);
			Redraw();
		}
		break;

	case Message::GetHighlightGuide:
		return highlightGuideColumn;

	case Message::GetLineEndPosition:
		return pdoc->LineEnd(LineFromUPtr(wParam));

	case Message::SetCodePage:
		if (ValidCodePage(static_cast<int>(wParam))) {
			if (pdoc->SetDBCSCodePage(static_cast<int>(wParam))) {
				pcs->Clear();
				pcs->InsertLines(0, pdoc->LinesTotal() - 1);
				SetAnnotationHeights(0, pdoc->LinesTotal());
				InvalidateStyleRedraw();
				SetRepresentations();
			}
		}
		break;

	case Message::GetCodePage:
		return pdoc->dbcsCodePage;

	case Message::SetIMEInteraction:
		imeInteraction = static_cast<IMEInteraction>(wParam);
		break;

	case Message::GetIMEInteraction:
		return static_cast<sptr_t>(imeInteraction);

	case Message::SetBidirectional:
		// Message::SetBidirectional is implemented on platform subclasses if they support bidirectional text.
		break;

	case Message::GetBidirectional:
		return static_cast<sptr_t>(bidirectional);

	case Message::GetLineCharacterIndex:
		return static_cast<sptr_t>(pdoc->LineCharacterIndex());

	case Message::AllocateLineCharacterIndex:
		pdoc->AllocateLineCharacterIndex(static_cast<LineCharacterIndexType>(wParam));
		break;

	case Message::ReleaseLineCharacterIndex:
		pdoc->ReleaseLineCharacterIndex(static_cast<LineCharacterIndexType>(wParam));
		break;

	case Message::LineFromIndexPosition:
		return pdoc->LineFromPositionIndex(PositionFromUPtr(wParam), static_cast<LineCharacterIndexType>(lParam));

	case Message::IndexPositionFromLine:
		return pdoc->IndexLineStart(LineFromUPtr(wParam), static_cast<LineCharacterIndexType>(lParam));

		// Marker definition and setting
	case Message::MarkerDefine:
		if (wParam <= MarkerMax) {
			vs.markers[wParam].markType = static_cast<MarkerSymbol>(lParam);
			vs.CalcLargestMarkerHeight();
		}
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case Message::MarkerSymbolDefined:
		if (wParam <= MarkerMax)
			return static_cast<sptr_t>(vs.markers[wParam].markType);
		else
			return 0;

	case Message::MarkerSetFore:
		if (wParam <= MarkerMax)
			vs.markers[wParam].fore = ColourRGBA::FromIpRGB(lParam);
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case Message::MarkerSetBack:
		if (wParam <= MarkerMax)
			vs.markers[wParam].back = ColourRGBA::FromIpRGB(lParam);
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case Message::MarkerSetBackSelected:
		if (wParam <= MarkerMax)
			vs.markers[wParam].backSelected = ColourRGBA::FromIpRGB(lParam);
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case Message::MarkerSetForeTranslucent:
		if (wParam <= MarkerMax)
			vs.markers[wParam].fore = ColourRGBA(static_cast<int>(lParam));
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case Message::MarkerSetBackTranslucent:
		if (wParam <= MarkerMax)
			vs.markers[wParam].back = ColourRGBA(static_cast<int>(lParam));
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case Message::MarkerSetBackSelectedTranslucent:
		if (wParam <= MarkerMax)
			vs.markers[wParam].backSelected = ColourRGBA(static_cast<int>(lParam));
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case Message::MarkerSetStrokeWidth:
		if (wParam <= MarkerMax)
			vs.markers[wParam].strokeWidth = lParam / 100.0f;
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case Message::MarkerEnableHighlight:
		marginView.highlightDelimiter.isEnabled = wParam == 1;
		RedrawSelMargin();
		break;
	case Message::MarkerSetAlpha:
		if (wParam <= MarkerMax) {
			if (static_cast<Alpha>(lParam) == Alpha::NoAlpha) {
				SetAppearance(vs.markers[wParam].alpha, Alpha::Opaque);
				SetAppearance(vs.markers[wParam].layer, Layer::Base);
			} else {
				SetAppearance(vs.markers[wParam].alpha, static_cast<Alpha>(lParam));
				SetAppearance(vs.markers[wParam].layer, Layer::OverText);
			}
		}
		break;
	case Message::MarkerSetLayer:
		if (wParam <= MarkerMax) {
			SetAppearance(vs.markers[wParam].layer, static_cast<Layer>(lParam));
		}
		break;
	case Message::MarkerGetLayer:
		if (wParam <= MarkerMax) {
			return static_cast<sptr_t>(vs.markers[wParam].layer);
		}
		return 0;
	case Message::MarkerAdd: {
			const int markerID = pdoc->AddMark(LineFromUPtr(wParam), static_cast<int>(lParam));
			return markerID;
		}
	case Message::MarkerAddSet:
		if (lParam != 0)
			pdoc->AddMarkSet(LineFromUPtr(wParam), static_cast<int>(lParam));
		break;

	case Message::MarkerDelete:
		pdoc->DeleteMark(LineFromUPtr(wParam), static_cast<int>(lParam));
		break;

	case Message::MarkerDeleteAll:
		pdoc->DeleteAllMarks(static_cast<int>(wParam));
		break;

	case Message::MarkerGet:
		return GetMark(LineFromUPtr(wParam));

	case Message::MarkerNext:
		return pdoc->MarkerNext(LineFromUPtr(wParam), static_cast<int>(lParam));

	case Message::MarkerPrevious: {
			for (Sci::Line iLine = LineFromUPtr(wParam); iLine >= 0; iLine--) {
				if ((GetMark(iLine) & lParam) != 0)
					return iLine;
			}
		}
		return -1;

	case Message::MarkerDefinePixmap:
		if (wParam <= MarkerMax) {
			vs.markers[wParam].SetXPM(ConstCharPtrFromSPtr(lParam));
			vs.CalcLargestMarkerHeight();
		}
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case Message::RGBAImageSetWidth:
		sizeRGBAImage.x = static_cast<XYPOSITION>(wParam);
		break;

	case Message::RGBAImageSetHeight:
		sizeRGBAImage.y = static_cast<XYPOSITION>(wParam);
		break;

	case Message::RGBAImageSetScale:
		scaleRGBAImage = static_cast<float>(wParam);
		break;

	case Message::MarkerDefineRGBAImage:
		if (wParam <= MarkerMax) {
			vs.markers[wParam].SetRGBAImage(sizeRGBAImage, scaleRGBAImage / 100.0f, ConstUCharPtrFromSPtr(lParam));
			vs.CalcLargestMarkerHeight();
		}
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case Message::SetMarginTypeN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].style = static_cast<MarginType>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetMarginTypeN:
		if (ValidMargin(wParam))
			return static_cast<sptr_t>(vs.ms[wParam].style);
		else
			return 0;

	case Message::SetMarginWidthN:
		if (ValidMargin(wParam)) {
			// Short-circuit if the width is unchanged, to avoid unnecessary redraw.
			if (vs.ms[wParam].width != lParam) {
				lastXChosen += static_cast<int>(lParam) - vs.ms[wParam].width;
				vs.ms[wParam].width = static_cast<int>(lParam);
				InvalidateStyleRedraw();
			}
		}
		break;

	case Message::GetMarginWidthN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].width;
		else
			return 0;

	case Message::SetMarginMaskN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].mask = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetMarginMaskN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].mask;
		else
			return 0;

	case Message::SetMarginSensitiveN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].sensitive = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetMarginSensitiveN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].sensitive ? 1 : 0;
		else
			return 0;

	case Message::SetMarginCursorN:
		if (ValidMargin(wParam))
			vs.ms[wParam].cursor = static_cast<CursorShape>(lParam);
		break;

	case Message::GetMarginCursorN:
		if (ValidMargin(wParam))
			return static_cast<sptr_t>(vs.ms[wParam].cursor);
		else
			return 0;

	case Message::SetMarginBackN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].back = ColourRGBA::FromIpRGB(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetMarginBackN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].back.OpaqueRGB();
		else
			return 0;

	case Message::SetMargins:
		if (wParam < 1000)
			vs.ms.resize(wParam);
		break;

	case Message::GetMargins:
		return vs.ms.size();

	case Message::StyleClearAll:
		vs.ClearStyles();
		InvalidateStyleRedraw();
		break;

	case Message::StyleSetFore:
	case Message::StyleSetBack:
	case Message::StyleSetBold:
	case Message::StyleSetWeight:
	case Message::StyleSetStretch:
	case Message::StyleSetItalic:
	case Message::StyleSetEOLFilled:
	case Message::StyleSetSize:
	case Message::StyleSetSizeFractional:
	case Message::StyleSetFont:
	case Message::StyleSetUnderline:
	case Message::StyleSetCase:
	case Message::StyleSetCharacterSet:
	case Message::StyleSetVisible:
	case Message::StyleSetChangeable:
	case Message::StyleSetHotSpot:
	case Message::StyleSetCheckMonospaced:
	case Message::StyleSetInvisibleRepresentation:
		StyleSetMessage(iMessage, wParam, lParam);
		break;

	case Message::StyleGetFore:
	case Message::StyleGetBack:
	case Message::StyleGetBold:
	case Message::StyleGetWeight:
	case Message::StyleGetStretch:
	case Message::StyleGetItalic:
	case Message::StyleGetEOLFilled:
	case Message::StyleGetSize:
	case Message::StyleGetSizeFractional:
	case Message::StyleGetFont:
	case Message::StyleGetUnderline:
	case Message::StyleGetCase:
	case Message::StyleGetCharacterSet:
	case Message::StyleGetVisible:
	case Message::StyleGetChangeable:
	case Message::StyleGetHotSpot:
	case Message::StyleGetCheckMonospaced:
	case Message::StyleGetInvisibleRepresentation:
		return StyleGetMessage(iMessage, wParam, lParam);

	case Message::StyleResetDefault:
		vs.ResetDefaultStyle();
		InvalidateStyleRedraw();
		break;

	case Message::SetElementColour:
		if (vs.SetElementColour(static_cast<Element>(wParam), ColourRGBA(static_cast<int>(lParam)))) {
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetElementColour:
		return vs.ElementColour(static_cast<Element>(wParam)).value_or(ColourRGBA()).AsInteger();

	case Message::ResetElementColour:
		if (vs.ResetElement(static_cast<Element>(wParam))) {
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetElementIsSet:
		return vs.ElementColour(static_cast<Element>(wParam)).has_value();

	case Message::GetElementAllowsTranslucent:
		return vs.ElementAllowsTranslucent(static_cast<Element>(wParam));

	case Message::GetElementBaseColour:
		return vs.elementBaseColours[static_cast<Element>(wParam)].value_or(ColourRGBA()).AsInteger();

	case Message::SetFontLocale:
		if (lParam) {
			vs.SetFontLocaleName(ConstCharPtrFromSPtr(lParam));
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetFontLocale:
		return StringResult(lParam, vs.localeName.c_str());

#ifdef INCLUDE_DEPRECATED_FEATURES
	case SCI_SETSTYLEBITS:
		vs.EnsureStyle(0xff);
		break;

	case SCI_GETSTYLEBITS:
		return 8;
#endif

	case Message::SetLineState:
		return pdoc->SetLineState(LineFromUPtr(wParam), static_cast<int>(lParam));

	case Message::GetLineState:
		return pdoc->GetLineState(LineFromUPtr(wParam));

	case Message::GetMaxLineState:
		return pdoc->GetMaxLineState();

	case Message::GetCaretLineVisible:
		return vs.ElementColour(Element::CaretLineBack) ? 1 : 0;
	case Message::SetCaretLineVisible:
		if (wParam) {
			if (!vs.elementColours.count(Element::CaretLineBack)) {
				// Yellow default
				vs.elementColours[Element::CaretLineBack] = ColourRGBA(maximumByte, maximumByte, 0);
				InvalidateStyleRedraw();
			}
		} else {
			if (vs.ResetElement(Element::CaretLineBack)) {
				InvalidateStyleRedraw();
			}
		}
		break;
	case Message::GetCaretLineVisibleAlways:
		return vs.caretLine.alwaysShow;
	case Message::SetCaretLineVisibleAlways:
		vs.caretLine.alwaysShow = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case Message::GetCaretLineHighlightSubLine:
		return vs.caretLine.subLine;
	case Message::SetCaretLineHighlightSubLine:
		vs.caretLine.subLine = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case Message::GetCaretLineFrame:
		return vs.caretLine.frame;
	case Message::SetCaretLineFrame:
		vs.caretLine.frame = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;
	case Message::GetCaretLineBack:
		return vs.ElementColourForced(Element::CaretLineBack).OpaqueRGB();

	case Message::SetCaretLineBack:
		vs.SetElementRGB(Element::CaretLineBack, static_cast<int>(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::GetCaretLineLayer:
		return static_cast<sptr_t>(vs.caretLine.layer);

	case Message::SetCaretLineLayer:
		if (vs.caretLine.layer != static_cast<Layer>(wParam)) {
			vs.caretLine.layer = static_cast<Layer>(wParam);
			UpdateBaseElements();
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetCaretLineBackAlpha:
		if (vs.caretLine.layer == Layer::Base)
			return static_cast<sptr_t>(Alpha::NoAlpha);
		return vs.ElementColour(Element::CaretLineBack).value_or(ColourRGBA()).GetAlpha();

	case Message::SetCaretLineBackAlpha: {
			const Layer layerNew = (static_cast<Alpha>(wParam) == Alpha::NoAlpha) ? Layer::Base : Layer::OverText;
			vs.caretLine.layer = layerNew;
			if (vs.ElementColour(Element::CaretLineBack)) {
				vs.SetElementAlpha(Element::CaretLineBack, static_cast<int>(wParam));
			}
			InvalidateStyleRedraw();
		}
		break;

		// Folding messages

	case Message::VisibleFromDocLine:
		return pcs->DisplayFromDoc(LineFromUPtr(wParam));

	case Message::DocLineFromVisible:
		return pcs->DocFromDisplay(LineFromUPtr(wParam));

	case Message::WrapCount:
		return WrapCount(LineFromUPtr(wParam));

	case Message::SetFoldLevel: {
			const int prev = pdoc->SetLevel(LineFromUPtr(wParam), static_cast<int>(lParam));
			if (prev != static_cast<int>(lParam))
				RedrawSelMargin();
			return prev;
		}

	case Message::GetFoldLevel:
		return pdoc->GetLevel(LineFromUPtr(wParam));

	case Message::GetLastChild:
		return pdoc->GetLastChild(LineFromUPtr(wParam), OptionalFoldLevel(lParam));

	case Message::GetFoldParent:
		return pdoc->GetFoldParent(LineFromUPtr(wParam));

	case Message::ShowLines:
		pcs->SetVisible(LineFromUPtr(wParam), lParam, true);
		SetScrollBars();
		Redraw();
		break;

	case Message::HideLines:
		pcs->SetVisible(LineFromUPtr(wParam), lParam, false);
		SetScrollBars();
		Redraw();
		break;

	case Message::GetLineVisible:
		return pcs->GetVisible(LineFromUPtr(wParam));

	case Message::GetAllLinesVisible:
		return pcs->HiddenLines() ? 0 : 1;

	case Message::SetFoldExpanded:
		SetFoldExpanded(LineFromUPtr(wParam), lParam != 0);
		break;

	case Message::GetFoldExpanded:
		return pcs->GetExpanded(LineFromUPtr(wParam));

	case Message::SetAutomaticFold:
		foldAutomatic = static_cast<AutomaticFold>(wParam);
		break;

	case Message::GetAutomaticFold:
		return static_cast<sptr_t>(foldAutomatic);

	case Message::SetFoldFlags:
		foldFlags = static_cast<FoldFlag>(wParam);
		Redraw();
		break;

	case Message::ToggleFoldShowText:
		pcs->SetFoldDisplayText(LineFromUPtr(wParam), ConstCharPtrFromSPtr(lParam));
		FoldLine(LineFromUPtr(wParam), FoldAction::Toggle);
		break;

	case Message::FoldDisplayTextSetStyle:
		foldDisplayTextStyle = static_cast<FoldDisplayTextStyle>(wParam);
		Redraw();
		break;

	case Message::FoldDisplayTextGetStyle:
		return static_cast<sptr_t>(foldDisplayTextStyle);

	case Message::SetDefaultFoldDisplayText:
		SetDefaultFoldDisplayText(ConstCharPtrFromSPtr(lParam));
		Redraw();
		break;

	case Message::GetDefaultFoldDisplayText:
		return StringResult(lParam, GetDefaultFoldDisplayText());

	case Message::ToggleFold:
		FoldLine(LineFromUPtr(wParam), FoldAction::Toggle);
		break;

	case Message::FoldLine:
		FoldLine(LineFromUPtr(wParam), static_cast<FoldAction>(lParam));
		break;

	case Message::FoldChildren:
		FoldExpand(LineFromUPtr(wParam), static_cast<FoldAction>(lParam), pdoc->GetFoldLevel(LineFromUPtr(wParam)));
		break;

	case Message::FoldAll:
		FoldAll(static_cast<FoldAction>(wParam));
		break;

	case Message::ExpandChildren:
		FoldExpand(LineFromUPtr(wParam), FoldAction::Expand, static_cast<FoldLevel>(lParam));
		break;

	case Message::ContractedFoldNext:
		return ContractedFoldNext(LineFromUPtr(wParam));

	case Message::EnsureVisible:
		EnsureLineVisible(LineFromUPtr(wParam), false);
		break;

	case Message::EnsureVisibleEnforcePolicy:
		EnsureLineVisible(LineFromUPtr(wParam), true);
		break;

	case Message::ScrollRange:
		ScrollRange(SelectionRange(PositionFromUPtr(wParam), lParam));
		break;

	case Message::SearchAnchor:
		SearchAnchor();
		break;

	case Message::SearchNext:
	case Message::SearchPrev:
		return SearchText(iMessage, wParam, lParam);

	case Message::SetXCaretPolicy:
		caretPolicies.x = CaretPolicySlop(wParam, lParam);
		break;

	case Message::SetYCaretPolicy:
		caretPolicies.y = CaretPolicySlop(wParam, lParam);
		break;

	case Message::SetVisiblePolicy:
		visiblePolicy = VisiblePolicySlop(wParam, lParam);
		break;

	case Message::LinesOnScreen:
		return LinesOnScreen();

	case Message::SetSelFore:
		vs.elementColours[Element::SelectionText] = OptionalColour(wParam, lParam);
		vs.elementColours[Element::SelectionAdditionalText] = OptionalColour(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case Message::SetSelBack:
		if (wParam) {
			vs.SetElementRGB(Element::SelectionBack, static_cast<int>(lParam));
			vs.SetElementRGB(Element::SelectionAdditionalBack, static_cast<int>(lParam));
		} else {
			vs.ResetElement(Element::SelectionBack);
			vs.ResetElement(Element::SelectionAdditionalBack);
		}
		InvalidateStyleRedraw();
		break;

	case Message::SetSelAlpha: {
			const Layer layerNew = (static_cast<Alpha>(wParam) == Alpha::NoAlpha) ? Layer::Base : Layer::OverText;
			if (vs.selection.layer != layerNew) {
			    vs.selection.layer = layerNew;
			    UpdateBaseElements();
			}
			const int alpha = static_cast<int>(wParam);
			vs.SetElementAlpha(Element::SelectionBack, alpha);
			vs.SetElementAlpha(Element::SelectionAdditionalBack, alpha);
			vs.SetElementAlpha(Element::SelectionSecondaryBack, alpha);
			vs.SetElementAlpha(Element::SelectionInactiveBack, alpha);
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetSelAlpha:
		if (vs.selection.layer == Layer::Base)
			return static_cast<sptr_t>(Alpha::NoAlpha);
		return vs.ElementColourForced(Element::SelectionBack).GetAlpha();

	case Message::GetSelEOLFilled:
		return vs.selection.eolFilled;

	case Message::SetSelEOLFilled:
		vs.selection.eolFilled = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case Message::SetWhitespaceFore:
		if (vs.SetElementColourOptional(Element::WhiteSpace, wParam, lParam)) {
			InvalidateStyleRedraw();
		}
		break;

	case Message::SetWhitespaceBack:
		if (vs.SetElementColourOptional(Element::WhiteSpaceBack, wParam, lParam)) {
			InvalidateStyleRedraw();
		}
		break;

	case Message::SetSelectionLayer:
		if (vs.selection.layer != static_cast<Layer>(wParam)) {
			vs.selection.layer = static_cast<Layer>(wParam);
			UpdateBaseElements();
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetSelectionLayer:
		return static_cast<sptr_t>(vs.selection.layer);

	case Message::SetCaretFore:
		vs.elementColours[Element::Caret] = ColourRGBA::FromIpRGB(SPtrFromUPtr(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::GetCaretFore:
		return vs.ElementColourForced(Element::Caret).OpaqueRGB();

	case Message::SetCaretStyle:
		if (static_cast<CaretStyle>(wParam) <= (CaretStyle::Block | CaretStyle::OverstrikeBlock | CaretStyle::Curses | CaretStyle::BlockAfter))
			vs.caret.style = static_cast<CaretStyle>(wParam);
		else
			/* Default to the line caret */
			vs.caret.style = CaretStyle::Line;
		InvalidateStyleRedraw();
		break;

	case Message::GetCaretStyle:
		return static_cast<sptr_t>(vs.caret.style);

	case Message::SetCaretWidth:
		vs.caret.width = std::clamp(static_cast<int>(wParam), 0, 20);
		InvalidateStyleRedraw();
		break;

	case Message::GetCaretWidth:
		return vs.caret.width;

	case Message::AssignCmdKey:
		kmap.AssignCmdKey(static_cast<Keys>(LowShortFromWParam(wParam)),
			static_cast<KeyMod>(HighShortFromWParam(wParam)), static_cast<Message>(lParam));
		break;

	case Message::ClearCmdKey:
		kmap.AssignCmdKey(static_cast<Keys>(LowShortFromWParam(wParam)),
			static_cast<KeyMod>(HighShortFromWParam(wParam)), Message::Null);
		break;

	case Message::ClearAllCmdKeys:
		kmap.Clear();
		break;

	case Message::IndicSetStyle:
		if (wParam <= IndicatorMax) {
			vs.indicators[wParam].sacNormal.style = static_cast<IndicatorStyle>(lParam);
			vs.indicators[wParam].sacHover.style = static_cast<IndicatorStyle>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetStyle:
		return (wParam <= IndicatorMax) ?
			static_cast<sptr_t>(vs.indicators[wParam].sacNormal.style) : 0;

	case Message::IndicSetFore:
		if (wParam <= IndicatorMax) {
			vs.indicators[wParam].sacNormal.fore = ColourRGBA::FromIpRGB(lParam);
			vs.indicators[wParam].sacHover.fore = ColourRGBA::FromIpRGB(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetFore:
		return (wParam <= IndicatorMax) ?
			vs.indicators[wParam].sacNormal.fore.OpaqueRGB() : 0;

	case Message::IndicSetHoverStyle:
		if (wParam <= IndicatorMax) {
			vs.indicators[wParam].sacHover.style = static_cast<IndicatorStyle>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetHoverStyle:
		return (wParam <= IndicatorMax) ?
			static_cast<sptr_t>(vs.indicators[wParam].sacHover.style) : 0;

	case Message::IndicSetHoverFore:
		if (wParam <= IndicatorMax) {
			vs.indicators[wParam].sacHover.fore = ColourRGBA::FromIpRGB(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetHoverFore:
		return (wParam <= IndicatorMax) ?
			vs.indicators[wParam].sacHover.fore.OpaqueRGB() : 0;

	case Message::IndicSetFlags:
		if (wParam <= IndicatorMax) {
			vs.indicators[wParam].SetFlags(static_cast<IndicFlag>(lParam));
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetFlags:
		return (wParam <= IndicatorMax) ?
			static_cast<sptr_t>(vs.indicators[wParam].Flags()) : 0;

	case Message::IndicSetUnder:
		if (wParam <= IndicatorMax) {
			vs.indicators[wParam].under = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetUnder:
		return (wParam <= IndicatorMax) ?
			vs.indicators[wParam].under : 0;

	case Message::IndicSetAlpha:
		if (wParam <= IndicatorMax && lParam >=0 && lParam <= 255) {
			vs.indicators[wParam].fillAlpha = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetAlpha:
		return (wParam <= IndicatorMax)
			? vs.indicators[wParam].fillAlpha : 0;

	case Message::IndicSetOutlineAlpha:
		if (wParam <= IndicatorMax && lParam >=0 && lParam <= 255) {
			vs.indicators[wParam].outlineAlpha = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetOutlineAlpha:
		return (wParam <= IndicatorMax) ? vs.indicators[wParam].outlineAlpha : 0;

	case Message::IndicSetStrokeWidth:
		if (wParam <= IndicatorMax && lParam >= 0 && lParam <= 1000) {
			vs.indicators[wParam].strokeWidth = lParam / 100.0f;
			InvalidateStyleRedraw();
		}
		break;

	case Message::IndicGetStrokeWidth:
		if (wParam <= IndicatorMax) {
			return std::lround(vs.indicators[wParam].strokeWidth * 100);
		}
		break;

	case Message::SetIndicatorCurrent:
		pdoc->DecorationSetCurrentIndicator(static_cast<int>(wParam));
		break;
	case Message::GetIndicatorCurrent:
		return pdoc->decorations->GetCurrentIndicator();
	case Message::SetIndicatorValue:
		pdoc->decorations->SetCurrentValue(static_cast<int>(wParam));
		break;
	case Message::GetIndicatorValue:
		return pdoc->decorations->GetCurrentValue();

	case Message::IndicatorFillRange:
		pdoc->DecorationFillRange(PositionFromUPtr(wParam),
			pdoc->decorations->GetCurrentValue(), lParam);
		break;

	case Message::IndicatorClearRange:
		pdoc->DecorationFillRange(PositionFromUPtr(wParam), 0,
			lParam);
		break;

	case Message::IndicatorAllOnFor:
		return pdoc->decorations->AllOnFor(PositionFromUPtr(wParam));

	case Message::IndicatorValueAt:
		return pdoc->decorations->ValueAt(static_cast<int>(wParam), lParam);

	case Message::IndicatorStart:
		return pdoc->decorations->Start(static_cast<int>(wParam), lParam);

	case Message::IndicatorEnd:
		return pdoc->decorations->End(static_cast<int>(wParam), lParam);

	case Message::LineDown:
	case Message::LineDownExtend:
	case Message::ParaDown:
	case Message::ParaDownExtend:
	case Message::LineUp:
	case Message::LineUpExtend:
	case Message::ParaUp:
	case Message::ParaUpExtend:
	case Message::CharLeft:
	case Message::CharLeftExtend:
	case Message::CharRight:
	case Message::CharRightExtend:
	case Message::WordLeft:
	case Message::WordLeftExtend:
	case Message::WordRight:
	case Message::WordRightExtend:
	case Message::WordLeftEnd:
	case Message::WordLeftEndExtend:
	case Message::WordRightEnd:
	case Message::WordRightEndExtend:
	case Message::Home:
	case Message::HomeExtend:
	case Message::LineEnd:
	case Message::LineEndExtend:
	case Message::HomeWrap:
	case Message::HomeWrapExtend:
	case Message::LineEndWrap:
	case Message::LineEndWrapExtend:
	case Message::DocumentStart:
	case Message::DocumentStartExtend:
	case Message::DocumentEnd:
	case Message::DocumentEndExtend:
	case Message::ScrollToStart:
	case Message::ScrollToEnd:

	case Message::StutteredPageUp:
	case Message::StutteredPageUpExtend:
	case Message::StutteredPageDown:
	case Message::StutteredPageDownExtend:

	case Message::PageUp:
	case Message::PageUpExtend:
	case Message::PageDown:
	case Message::PageDownExtend:
	case Message::EditToggleOvertype:
	case Message::Cancel:
	case Message::DeleteBack:
	case Message::Tab:
	case Message::LineIndent:
	case Message::BackTab:
	case Message::LineDedent:
	case Message::NewLine:
	case Message::FormFeed:
	case Message::VCHome:
	case Message::VCHomeExtend:
	case Message::VCHomeWrap:
	case Message::VCHomeWrapExtend:
	case Message::VCHomeDisplay:
	case Message::VCHomeDisplayExtend:
	case Message::ZoomIn:
	case Message::ZoomOut:
	case Message::DelWordLeft:
	case Message::DelWordRight:
	case Message::DelWordRightEnd:
	case Message::DelLineLeft:
	case Message::DelLineRight:
	case Message::LineCopy:
	case Message::LineCut:
	case Message::LineDelete:
	case Message::LineTranspose:
	case Message::LineReverse:
	case Message::LineDuplicate:
	case Message::LowerCase:
	case Message::UpperCase:
	case Message::LineScrollDown:
	case Message::LineScrollUp:
	case Message::WordPartLeft:
	case Message::WordPartLeftExtend:
	case Message::WordPartRight:
	case Message::WordPartRightExtend:
	case Message::DeleteBackNotLine:
	case Message::HomeDisplay:
	case Message::HomeDisplayExtend:
	case Message::LineEndDisplay:
	case Message::LineEndDisplayExtend:
	case Message::LineDownRectExtend:
	case Message::LineUpRectExtend:
	case Message::CharLeftRectExtend:
	case Message::CharRightRectExtend:
	case Message::HomeRectExtend:
	case Message::VCHomeRectExtend:
	case Message::LineEndRectExtend:
	case Message::PageUpRectExtend:
	case Message::PageDownRectExtend:
	case Message::SelectionDuplicate:
		return KeyCommand(iMessage);

	case Message::BraceHighlight:
		SetBraceHighlight(PositionFromUPtr(wParam), lParam, StyleBraceLight);
		break;

	case Message::BraceHighlightIndicator:
		if (lParam >= 0 && static_cast<size_t>(lParam) <= IndicatorMax) {
			vs.braceHighlightIndicatorSet = wParam != 0;
			vs.braceHighlightIndicator = static_cast<int>(lParam);
		}
		break;

	case Message::BraceBadLight:
		SetBraceHighlight(PositionFromUPtr(wParam), -1, StyleBraceBad);
		break;

	case Message::BraceBadLightIndicator:
		if (lParam >= 0 && static_cast<size_t>(lParam) <= IndicatorMax) {
			vs.braceBadLightIndicatorSet = wParam != 0;
			vs.braceBadLightIndicator = static_cast<int>(lParam);
		}
		break;

	case Message::BraceMatch:
		// wParam is position of char to find brace for,
		// lParam is maximum amount of text to restyle to find it
		return pdoc->BraceMatch(PositionFromUPtr(wParam), lParam, 0, false);

	case Message::BraceMatchNext:
		return pdoc->BraceMatch(PositionFromUPtr(wParam), 0, lParam, true);

	case Message::GetViewEOL:
		return vs.viewEOL;

	case Message::SetViewEOL:
		vs.viewEOL = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case Message::SetZoom:
		if (SetAppearance(vs.zoomLevel, static_cast<int>(wParam))) {
			NotifyZoom();
		}
		break;

	case Message::GetZoom:
		return vs.zoomLevel;

	case Message::GetEdgeColumn:
		return vs.theEdge.column;

	case Message::SetEdgeColumn:
		vs.theEdge.column = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::GetEdgeMode:
		return static_cast<sptr_t>(vs.edgeState);

	case Message::SetEdgeMode:
		vs.edgeState = static_cast<EdgeVisualStyle>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::GetEdgeColour:
		return vs.theEdge.colour.OpaqueRGB();

	case Message::SetEdgeColour:
		vs.theEdge.colour = ColourRGBA::FromIpRGB(SPtrFromUPtr(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::MultiEdgeAddLine:
		vs.AddMultiEdge(static_cast<int>(wParam), ColourRGBA::FromIpRGB(lParam));
		InvalidateStyleRedraw();
		break;

	case Message::MultiEdgeClearAll:
		std::vector<EdgeProperties>().swap(vs.theMultiEdge); // Free vector and memory, C++03 compatible
		InvalidateStyleRedraw();
		break;

	case Message::GetMultiEdgeColumn: {
			const size_t which = wParam;
			// size_t is unsigned so this also handles negative inputs.
			if (which >= vs.theMultiEdge.size()) {
				return -1;
			}
			return vs.theMultiEdge[which].column;
		}

	case Message::GetAccessibility:
		return static_cast<sptr_t>(Accessibility::Disabled);

	case Message::SetAccessibility:
		// May be implemented by platform code.
		break;

	case Message::GetDocPointer:
		return SPtrFromPtr(pdoc->AsDocumentEditable());

	case Message::SetDocPointer:
		CancelModes();
		SetDocPointer(static_cast<Document *>(static_cast<IDocumentEditable *>(PtrFromSPtr(lParam))));
		return 0;

	case Message::CreateDocument: {
			Document *doc = new Document(static_cast<DocumentOption>(lParam));
			doc->AddRef();
			doc->Allocate(PositionFromUPtr(wParam));
			pcs = ContractionStateCreate(pdoc->IsLarge());
			return SPtrFromPtr(doc->AsDocumentEditable());
		}

	case Message::AddRefDocument:
		(static_cast<IDocumentEditable *>(PtrFromSPtr(lParam)))->AddRef();
		break;

	case Message::ReleaseDocument:
		(static_cast<IDocumentEditable *>(PtrFromSPtr(lParam)))->Release();
		break;

	case Message::GetDocumentOptions:
		return static_cast<sptr_t>(pdoc->Options());

	case Message::CreateLoader: {
			Document *doc = new Document(static_cast<DocumentOption>(lParam));
			doc->AddRef();
			doc->Allocate(PositionFromUPtr(wParam));
			doc->SetUndoCollection(false);
			pcs = ContractionStateCreate(pdoc->IsLarge());
			return reinterpret_cast<sptr_t>(static_cast<ILoader *>(doc));
		}

	case Message::SetModEventMask:
		modEventMask = static_cast<ModificationFlags>(wParam);
		return 0;

	case Message::GetModEventMask:
		return static_cast<sptr_t>(modEventMask);

	case Message::SetCommandEvents:
		commandEvents = static_cast<bool>(wParam);
		return 0;

	case Message::GetCommandEvents:
		return commandEvents;

	case Message::ConvertEOLs:
		pdoc->ConvertLineEnds(static_cast<EndOfLine>(wParam));
		SetSelection(sel.MainCaret(), sel.MainAnchor());	// Ensure selection inside document
		return 0;

	case Message::SetLengthForEncode:
		lengthForEncode = PositionFromUPtr(wParam);
		return 0;

	case Message::SelectionIsRectangle:
		return sel.selType == Selection::SelTypes::rectangle ? 1 : 0;

	case Message::SetSelectionMode:
		SetSelectionMode(wParam, true);
		break;
	case Message::ChangeSelectionMode:
		SetSelectionMode(wParam, false);
		break;
	case Message::GetSelectionMode:
		switch (sel.selType) {
		case Selection::SelTypes::stream:
			return static_cast<sptr_t>(SelectionMode::Stream);
		case Selection::SelTypes::rectangle:
			return static_cast<sptr_t>(SelectionMode::Rectangle);
		case Selection::SelTypes::lines:
			return static_cast<sptr_t>(SelectionMode::Lines);
		case Selection::SelTypes::thin:
			return static_cast<sptr_t>(SelectionMode::Thin);
		default:	// ?!
			return static_cast<sptr_t>(SelectionMode::Stream);
		}
	case Message::SetMoveExtendsSelection:
		sel.SetMoveExtends(wParam != 0);
		break;
	case Message::GetMoveExtendsSelection:
		return sel.MoveExtends();
	case Message::GetLineSelStartPosition:
	case Message::GetLineSelEndPosition: {
			const SelectionSegment segmentLine(
				pdoc->LineStart(LineFromUPtr(wParam)),
				pdoc->LineEnd(LineFromUPtr(wParam)));
			for (size_t r=0; r<sel.Count(); r++) {
				const SelectionSegment portion = sel.Range(r).Intersect(segmentLine);
				if (portion.start.IsValid()) {
					return (iMessage == Message::GetLineSelStartPosition) ? portion.start.Position() : portion.end.Position();
				}
			}
			return Sci::invalidPosition;
		}

	case Message::SetOvertype:
		if (inOverstrike != (wParam != 0)) {
			inOverstrike = wParam != 0;
			ContainerNeedsUpdate(Update::Selection);
			ShowCaretAtCurrentPosition();
			SetIdle(true);
		}
		break;

	case Message::GetOvertype:
		return inOverstrike ? 1 : 0;

	case Message::SetFocus:
		SetFocusState(wParam != 0);
		break;

	case Message::GetFocus:
		return hasFocus;

	case Message::SetStatus:
		errorStatus = static_cast<Status>(wParam);
		break;

	case Message::GetStatus:
		return static_cast<sptr_t>(errorStatus);

	case Message::SetMouseDownCaptures:
		mouseDownCaptures = wParam != 0;
		break;

	case Message::GetMouseDownCaptures:
		return mouseDownCaptures;

	case Message::SetMouseWheelCaptures:
		mouseWheelCaptures = wParam != 0;
		break;

	case Message::GetMouseWheelCaptures:
		return mouseWheelCaptures;

	case Message::SetCursor:
		cursorMode = static_cast<CursorShape>(wParam);
		DisplayCursor(Window::Cursor::text);
		break;

	case Message::GetCursor:
		return static_cast<sptr_t>(cursorMode);

	case Message::SetControlCharSymbol:
		vs.controlCharSymbol = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::GetControlCharSymbol:
		return vs.controlCharSymbol;

	case Message::SetRepresentation:
		reprs->SetRepresentation(ConstCharPtrFromUPtr(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case Message::GetRepresentation: {
			const Representation *repr = reprs->RepresentationFromCharacter(
				ConstCharPtrFromUPtr(wParam));
			if (repr) {
				return StringResult(lParam, repr->stringRep.c_str());
			}
			return 0;
		}

	case Message::ClearRepresentation:
		reprs->ClearRepresentation(ConstCharPtrFromUPtr(wParam));
		break;

	case Message::ClearAllRepresentations:
		SetRepresentations();
		break;

	case Message::SetRepresentationAppearance:
		reprs->SetRepresentationAppearance(ConstCharPtrFromUPtr(wParam), static_cast<RepresentationAppearance>(lParam));
		break;

	case Message::GetRepresentationAppearance: {
			const Representation *repr = reprs->RepresentationFromCharacter(
				ConstCharPtrFromUPtr(wParam));
			if (repr) {
				return static_cast<sptr_t>(repr->appearance);
			}
			return 0;
		}
	case Message::SetRepresentationColour:
		reprs->SetRepresentationColour(ConstCharPtrFromUPtr(wParam), ColourRGBA(static_cast<int>(lParam)));
		break;

	case Message::GetRepresentationColour: {
			const Representation *repr = reprs->RepresentationFromCharacter(
				ConstCharPtrFromUPtr(wParam));
			if (repr) {
				return repr->colour.AsInteger();
			}
			return 0;
		}

	case Message::StartRecord:
		recordingMacro = true;
		return 0;

	case Message::StopRecord:
		recordingMacro = false;
		return 0;

	case Message::MoveCaretInsideView:
		MoveCaretInsideView();
		break;

	case Message::SetFoldMarginColour:
		vs.foldmarginColour = OptionalColour(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case Message::SetFoldMarginHiColour:
		vs.foldmarginHighlightColour = OptionalColour(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case Message::SetHotspotActiveFore:
		if (vs.SetElementColourOptional(Element::HotSpotActive, wParam, lParam)) {
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetHotspotActiveFore:
		return vs.ElementColour(Element::HotSpotActive).value_or(ColourRGBA()).OpaqueRGB();

	case Message::SetHotspotActiveBack:
		if (vs.SetElementColourOptional(Element::HotSpotActiveBack, wParam, lParam)) {
			InvalidateStyleRedraw();
		}
		break;

	case Message::GetHotspotActiveBack:
		return vs.ElementColour(Element::HotSpotActiveBack).value_or(ColourRGBA()).OpaqueRGB();

	case Message::SetHotspotActiveUnderline:
		vs.hotspotUnderline = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case Message::GetHotspotActiveUnderline:
		return vs.hotspotUnderline ? 1 : 0;

	case Message::SetHotspotSingleLine:
		hotspotSingleLine = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case Message::GetHotspotSingleLine:
		return hotspotSingleLine ? 1 : 0;

	case Message::SetPasteConvertEndings:
		convertPastes = wParam != 0;
		break;

	case Message::GetPasteConvertEndings:
		return convertPastes ? 1 : 0;

	case Message::GetCharacterPointer:
		return reinterpret_cast<sptr_t>(pdoc->BufferPointer());

	case Message::GetRangePointer:
		return reinterpret_cast<sptr_t>(pdoc->RangePointer(
			PositionFromUPtr(wParam), lParam));

	case Message::GetGapPosition:
		return pdoc->GapPosition();

	case Message::SetChangeHistory:
		changeHistoryOption = static_cast<ChangeHistoryOption>(wParam);
		pdoc->ChangeHistorySet(wParam & 1);
		break;

	case Message::GetChangeHistory:
		return static_cast<sptr_t>(changeHistoryOption);

	case Message::SetUndoSelectionHistory:
		ChangeUndoSelectionHistory(static_cast<UndoSelectionHistoryOption>(wParam));
		break;

	case Message::GetUndoSelectionHistory:
		return static_cast<sptr_t>(undoSelectionHistoryOption);

	case Message::SetSelectionSerialized:
		SetSelectionFromSerialized(ConstCharPtrFromSPtr(lParam));
		break;

	case Message::GetSelectionSerialized: {
		const std::string serialized = sel.ToString();
		return BytesResult(lParam, serialized);
	}

	case Message::SetExtraAscent:
		vs.extraAscent = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::GetExtraAscent:
		return vs.extraAscent;

	case Message::SetExtraDescent:
		vs.extraDescent = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::GetExtraDescent:
		return vs.extraDescent;

	case Message::MarginSetStyleOffset:
		vs.marginStyleOffset = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::MarginGetStyleOffset:
		return vs.marginStyleOffset;

	case Message::SetMarginOptions:
		marginOptions = static_cast<MarginOption>(wParam);
		break;

	case Message::GetMarginOptions:
		return static_cast<sptr_t>(marginOptions);

	case Message::MarginSetText:
		pdoc->MarginSetText(LineFromUPtr(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case Message::MarginGetText: {
			const StyledText st = pdoc->MarginStyledText(LineFromUPtr(wParam));
			return BytesResult(lParam, reinterpret_cast<const unsigned char *>(st.text), st.length);
		}

	case Message::MarginSetStyle:
		pdoc->MarginSetStyle(LineFromUPtr(wParam), static_cast<int>(lParam));
		break;

	case Message::MarginGetStyle: {
			const StyledText st = pdoc->MarginStyledText(LineFromUPtr(wParam));
			return st.style;
		}

	case Message::MarginSetStyles:
		pdoc->MarginSetStyles(LineFromUPtr(wParam), ConstUCharPtrFromSPtr(lParam));
		break;

	case Message::MarginGetStyles: {
			const StyledText st = pdoc->MarginStyledText(LineFromUPtr(wParam));
			return BytesResult(lParam, st.styles, st.length);
		}

	case Message::MarginTextClearAll:
		pdoc->MarginClearAll();
		break;

	case Message::AnnotationSetText:
		pdoc->AnnotationSetText(LineFromUPtr(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case Message::AnnotationGetText: {
			const StyledText st = pdoc->AnnotationStyledText(LineFromUPtr(wParam));
			return BytesResult(lParam, reinterpret_cast<const unsigned char *>(st.text), st.length);
		}

	case Message::AnnotationGetStyle: {
			const StyledText st = pdoc->AnnotationStyledText(LineFromUPtr(wParam));
			return st.style;
		}

	case Message::AnnotationSetStyle:
		pdoc->AnnotationSetStyle(LineFromUPtr(wParam), static_cast<int>(lParam));
		break;

	case Message::AnnotationSetStyles:
		pdoc->AnnotationSetStyles(LineFromUPtr(wParam), ConstUCharPtrFromSPtr(lParam));
		break;

	case Message::AnnotationGetStyles: {
			const StyledText st = pdoc->AnnotationStyledText(LineFromUPtr(wParam));
			return BytesResult(lParam, st.styles, st.length);
		}

	case Message::AnnotationGetLines:
		return pdoc->AnnotationLines(LineFromUPtr(wParam));

	case Message::AnnotationClearAll:
		pdoc->AnnotationClearAll();
		break;

	case Message::AnnotationSetVisible:
		SetAnnotationVisible(static_cast<AnnotationVisible>(wParam));
		break;

	case Message::AnnotationGetVisible:
		return static_cast<sptr_t>(vs.annotationVisible);

	case Message::AnnotationSetStyleOffset:
		vs.annotationStyleOffset = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::AnnotationGetStyleOffset:
		return vs.annotationStyleOffset;

	case Message::EOLAnnotationSetText:
		pdoc->EOLAnnotationSetText(LineFromUPtr(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case Message::EOLAnnotationGetText: {
			const StyledText st = pdoc->EOLAnnotationStyledText(LineFromUPtr(wParam));
			return BytesResult(lParam, reinterpret_cast<const unsigned char *>(st.text), st.length);
		}

	case Message::EOLAnnotationGetStyle: {
			const StyledText st = pdoc->EOLAnnotationStyledText(LineFromUPtr(wParam));
			return st.style;
		}

	case Message::EOLAnnotationSetStyle:
		pdoc->EOLAnnotationSetStyle(LineFromUPtr(wParam), static_cast<int>(lParam));
		break;

	case Message::EOLAnnotationClearAll:
		pdoc->EOLAnnotationClearAll();
		break;

	case Message::EOLAnnotationSetVisible:
		SetEOLAnnotationVisible(static_cast<EOLAnnotationVisible>(wParam));
		break;

	case Message::EOLAnnotationGetVisible:
		return static_cast<sptr_t>(vs.eolAnnotationVisible);

	case Message::EOLAnnotationSetStyleOffset:
		vs.eolAnnotationStyleOffset = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::EOLAnnotationGetStyleOffset:
		return vs.eolAnnotationStyleOffset;

	case Message::ReleaseAllExtendedStyles:
		vs.ReleaseAllExtendedStyles();
		break;

	case Message::AllocateExtendedStyles:
		return vs.AllocateExtendedStyles(static_cast<int>(wParam));

	case Message::SupportsFeature:
		return SupportsFeature(static_cast<Supports>(wParam));

	case Message::AddUndoAction:
		pdoc->AddUndoAction(PositionFromUPtr(wParam),
			FlagSet(static_cast<UndoFlags>(lParam), UndoFlags::MayCoalesce));
		break;

	case Message::SetMouseSelectionRectangularSwitch:
		mouseSelectionRectangularSwitch = wParam != 0;
		break;

	case Message::GetMouseSelectionRectangularSwitch:
		return mouseSelectionRectangularSwitch;

	case Message::SetMultipleSelection:
		multipleSelection = wParam != 0;
		InvalidateCaret();
		break;

	case Message::GetMultipleSelection:
		return multipleSelection;

	case Message::SetAdditionalSelectionTyping:
		additionalSelectionTyping = wParam != 0;
		InvalidateCaret();
		break;

	case Message::GetAdditionalSelectionTyping:
		return additionalSelectionTyping;

	case Message::SetMultiPaste:
		multiPasteMode = static_cast<MultiPaste>(wParam);
		break;

	case Message::GetMultiPaste:
		return static_cast<sptr_t>(multiPasteMode);

	case Message::SetAdditionalCaretsBlink:
		view.additionalCaretsBlink = wParam != 0;
		InvalidateCaret();
		break;

	case Message::GetAdditionalCaretsBlink:
		return view.additionalCaretsBlink;

	case Message::SetAdditionalCaretsVisible:
		view.additionalCaretsVisible = wParam != 0;
		InvalidateCaret();
		break;

	case Message::GetAdditionalCaretsVisible:
		return view.additionalCaretsVisible;

	case Message::GetSelections:
		return sel.Count();

	case Message::GetSelectionEmpty:
		return sel.Empty();

	case Message::ClearSelections:
		sel.Clear();
		ContainerNeedsUpdate(Update::Selection);
		Redraw();
		break;

	case Message::SetSelection:
		sel.SetSelection(SelectionRange(PositionFromUPtr(wParam), lParam));
		Redraw();
		break;

	case Message::AddSelection:
		sel.AddSelection(SelectionRange(PositionFromUPtr(wParam), lParam));
		ContainerNeedsUpdate(Update::Selection);
		Redraw();
		break;

	case Message::SelectionFromPoint:
		return SelectionFromPoint(PointFromParameters(wParam, lParam));

	case Message::DropSelectionN:
		DropSelection(wParam);
		break;

	case Message::SetMainSelection:
		sel.SetMain(wParam);
		ContainerNeedsUpdate(Update::Selection);
		Redraw();
		break;

	case Message::GetMainSelection:
		return sel.Main();

	case Message::SetSelectionNCaret:
	case Message::SetSelectionNAnchor:
	case Message::SetSelectionNCaretVirtualSpace:
	case Message::SetSelectionNAnchorVirtualSpace:
	case Message::SetSelectionNStart:
	case Message::SetSelectionNEnd:
		SetSelectionNMessage(iMessage, wParam, lParam);
		break;

	case Message::GetSelectionNCaret:
		return sel.Range(wParam).caret.Position();

	case Message::GetSelectionNAnchor:
		return sel.Range(wParam).anchor.Position();

	case Message::GetSelectionNCaretVirtualSpace:
		return sel.Range(wParam).caret.VirtualSpace();

	case Message::GetSelectionNAnchorVirtualSpace:
		return sel.Range(wParam).anchor.VirtualSpace();

	case Message::GetSelectionNStart:
		return sel.Range(wParam).Start().Position();

	case Message::GetSelectionNStartVirtualSpace:
		return sel.Range(wParam).Start().VirtualSpace();

	case Message::GetSelectionNEnd:
		return sel.Range(wParam).End().Position();

	case Message::GetSelectionNEndVirtualSpace:
		return sel.Range(wParam).End().VirtualSpace();

	case Message::SetRectangularSelectionCaret:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::SelTypes::rectangle;
		sel.Rectangular().caret.SetPosition(PositionFromUPtr(wParam));
		SetRectangularRange();
		Redraw();
		break;

	case Message::GetRectangularSelectionCaret:
		return sel.Rectangular().caret.Position();

	case Message::SetRectangularSelectionAnchor:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::SelTypes::rectangle;
		sel.Rectangular().anchor.SetPosition(PositionFromUPtr(wParam));
		SetRectangularRange();
		Redraw();
		break;

	case Message::GetRectangularSelectionAnchor:
		return sel.Rectangular().anchor.Position();

	case Message::SetRectangularSelectionCaretVirtualSpace:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::SelTypes::rectangle;
		sel.Rectangular().caret.SetVirtualSpace(PositionFromUPtr(wParam));
		SetRectangularRange();
		Redraw();
		break;

	case Message::GetRectangularSelectionCaretVirtualSpace:
		return sel.Rectangular().caret.VirtualSpace();

	case Message::SetRectangularSelectionAnchorVirtualSpace:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::SelTypes::rectangle;
		sel.Rectangular().anchor.SetVirtualSpace(PositionFromUPtr(wParam));
		SetRectangularRange();
		Redraw();
		break;

	case Message::GetRectangularSelectionAnchorVirtualSpace:
		return sel.Rectangular().anchor.VirtualSpace();

	case Message::SetVirtualSpaceOptions:
		virtualSpaceOptions = static_cast<VirtualSpace>(wParam);
		break;

	case Message::GetVirtualSpaceOptions:
		return static_cast<sptr_t>(virtualSpaceOptions);

	case Message::SetAdditionalSelFore:
		vs.elementColours[Element::SelectionAdditionalText] = ColourRGBA::FromIpRGB(SPtrFromUPtr(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::SetAdditionalSelBack:
		vs.SetElementRGB(Element::SelectionAdditionalBack, static_cast<int>(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::SetAdditionalSelAlpha:
		vs.SetElementAlpha(Element::SelectionAdditionalBack, static_cast<int>(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::GetAdditionalSelAlpha:
		if (vs.selection.layer == Layer::Base)
			return static_cast<sptr_t>(Alpha::NoAlpha);
		return vs.ElementColourForced(Element::SelectionAdditionalBack).GetAlpha();

	case Message::SetAdditionalCaretFore:
		vs.elementColours[Element::CaretAdditional] = ColourRGBA::FromIpRGB(SPtrFromUPtr(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::GetAdditionalCaretFore:
		return vs.ElementColourForced(Element::CaretAdditional).OpaqueRGB();

	case Message::RotateSelection:
		sel.RotateMain();
		InvalidateWholeSelection();
		break;

	case Message::SwapMainAnchorCaret:
		InvalidateSelection(sel.RangeMain());
		sel.RangeMain().Swap();
		break;

	case Message::MultipleSelectAddNext:
		MultipleSelectAdd(AddNumber::one);
		break;

	case Message::MultipleSelectAddEach:
		MultipleSelectAdd(AddNumber::each);
		break;

	case Message::ChangeLexerState:
		pdoc->ChangeLexerState(PositionFromUPtr(wParam), lParam);
		break;

	case Message::SetIdentifier:
		SetCtrlID(static_cast<int>(wParam));
		break;

	case Message::GetIdentifier:
		return GetCtrlID();

	case Message::SetTechnology:
		// No action by default
		break;

	case Message::GetTechnology:
		return static_cast<sptr_t>(technology);

	case Message::CountCharacters:
		return pdoc->CountCharacters(PositionFromUPtr(wParam), lParam);

	case Message::CountCodeUnits:
		return pdoc->CountUTF16(PositionFromUPtr(wParam), lParam);

	default:
		return DefWndProc(iMessage, wParam, lParam);
	}

	// If there was a change that needs its selection saved and it wasn't explicity saved
	// then do that here.
	RememberCurrentSelectionForRedoOntoStack();

	//Platform::DebugPrintf("end wnd proc\n");
	return 0;
}
