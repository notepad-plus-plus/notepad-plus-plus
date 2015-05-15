// Scintilla source code edit control
/** @file Editor.cxx
 ** Main code for the edit control.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "Editor.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

/*
	return whether this modification represents an operation that
	may reasonably be deferred (not done now OR [possibly] at all)
*/
static bool CanDeferToLastStep(const DocModification &mh) {
	if (mh.modificationType & (SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE))
		return true;	// CAN skip
	if (!(mh.modificationType & (SC_PERFORMED_UNDO | SC_PERFORMED_REDO)))
		return false;	// MUST do
	if (mh.modificationType & SC_MULTISTEPUNDOREDO)
		return true;	// CAN skip
	return false;		// PRESUMABLY must do
}

static bool CanEliminate(const DocModification &mh) {
	return
	    (mh.modificationType & (SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE)) != 0;
}

/*
	return whether this modification represents the FINAL step
	in a [possibly lengthy] multi-step Undo/Redo sequence
*/
static bool IsLastStep(const DocModification &mh) {
	return
	    (mh.modificationType & (SC_PERFORMED_UNDO | SC_PERFORMED_REDO)) != 0
	    && (mh.modificationType & SC_MULTISTEPUNDOREDO) != 0
	    && (mh.modificationType & SC_LASTSTEPINUNDOREDO) != 0
	    && (mh.modificationType & SC_MULTILINEUNDOREDO) != 0;
}

Caret::Caret() :
		active(false), on(false), period(500) {}

Timer::Timer() :
		ticking(false), ticksToWait(0), tickerID(0) {}

Idler::Idler() :
		state(false), idlerID(0) {}

static inline bool IsControlCharacter(int ch) {
	// iscntrl returns true for lots of chars > 127 which are displayable
	return ch >= 0 && ch < ' ';
}

static inline bool IsAllSpacesOrTabs(char *s, unsigned int len) {
	for (unsigned int i = 0; i < len; i++) {
		// This is safe because IsSpaceOrTab() will return false for null terminators
		if (!IsSpaceOrTab(s[i]))
			return false;
	}
	return true;
}

Editor::Editor() {
	ctrlID = 0;

	stylesValid = false;
	technology = SC_TECHNOLOGY_DEFAULT;
	scaleRGBAImage = 100;

	printMagnification = 0;
	printColourMode = SC_PRINT_NORMAL;
	printWrapState = eWrapWord;
	cursorMode = SC_CURSORNORMAL;
	controlCharSymbol = 0;	/* Draw the control characters */

	hasFocus = false;
	hideSelection = false;
	inOverstrike = false;
	drawOverstrikeCaret = true;
	errorStatus = 0;
	mouseDownCaptures = true;

	bufferedDraw = true;
	twoPhaseDraw = true;

	lastClickTime = 0;
	dwellDelay = SC_TIME_FOREVER;
	ticksToDwell = SC_TIME_FOREVER;
	dwelling = false;
	ptMouseLast.x = 0;
	ptMouseLast.y = 0;
	inDragDrop = ddNone;
	dropWentOutside = false;
	posDrag = SelectionPosition(invalidPosition);
	posDrop = SelectionPosition(invalidPosition);
	hotSpotClickPos = INVALID_POSITION;
	selectionType = selChar;

	lastXChosen = 0;
	lineAnchorPos = 0;
	originalAnchorPos = 0;
	wordSelectAnchorStartPos = 0;
	wordSelectAnchorEndPos = 0;
	wordSelectInitialCaretPos = -1;

	primarySelection = true;

	caretXPolicy = CARET_SLOP | CARET_EVEN;
	caretXSlop = 50;

	caretYPolicy = CARET_EVEN;
	caretYSlop = 0;

	visiblePolicy = 0;
	visibleSlop = 0;

	searchAnchor = 0;

	xOffset = 0;
	xCaretMargin = 50;
	horizontalScrollBarVisible = true;
	scrollWidth = 2000;
	trackLineWidth = false;
	lineWidthMaxSeen = 0;
	verticalScrollBarVisible = true;
	endAtLastLine = true;
	caretSticky = SC_CARETSTICKY_OFF;
	marginOptions = SC_MARGINOPTION_NONE;
	multipleSelection = false;
	additionalSelectionTyping = false;
	multiPasteMode = SC_MULTIPASTE_ONCE;
	additionalCaretsBlink = true;
	additionalCaretsVisible = true;
	virtualSpaceOptions = SCVS_NONE;

	pixmapLine = 0;
	pixmapSelMargin = 0;
	pixmapSelPattern = 0;
	pixmapSelPatternOffset1 = 0;
	pixmapIndentGuide = 0;
	pixmapIndentGuideHighlight = 0;

	targetStart = 0;
	targetEnd = 0;
	searchFlags = 0;

	topLine = 0;
	posTopLine = 0;

	lengthForEncode = -1;

	needUpdateUI = 0;
	ContainerNeedsUpdate(SC_UPDATE_CONTENT);
	braces[0] = invalidPosition;
	braces[1] = invalidPosition;
	bracesMatchStyle = STYLE_BRACEBAD;
	highlightGuideColumn = 0;

	theEdge = 0;

	paintState = notPainting;
	willRedrawAll = false;

	modEventMask = SC_MODEVENTMASKALL;

	pdoc = new Document();
	pdoc->AddRef();
	pdoc->AddWatcher(this, 0);

	recordingMacro = false;
	foldFlags = 0;
	foldAutomatic = 0;

	wrapState = eWrapNone;
	wrapWidth = LineLayout::wrapWidthInfinite;
	wrapVisualFlags = 0;
	wrapVisualFlagsLocation = 0;
	wrapVisualStartIndent = 0;
	wrapIndentMode = SC_WRAPINDENT_FIXED;

	convertPastes = true;

	marginNumberPadding = 3;
	ctrlCharPadding = 3; // +3 For a blank on front and rounded edge each side
	lastSegItalicsOffset = 2;

	hsStart = -1;
	hsEnd = -1;

	llc.SetLevel(LineLayoutCache::llcCaret);
	posCache.SetSize(0x400);
}

Editor::~Editor() {
	pdoc->RemoveWatcher(this, 0);
	pdoc->Release();
	pdoc = 0;
	DropGraphics(true);
}

void Editor::Finalise() {
	SetIdle(false);
	CancelModes();
}

void Editor::DropGraphics(bool freeObjects) {
	if (freeObjects) {
		delete pixmapLine;
		pixmapLine = 0;
		delete pixmapSelMargin;
		pixmapSelMargin = 0;
		delete pixmapSelPattern;
		pixmapSelPattern = 0;
		delete pixmapSelPatternOffset1;
		pixmapSelPatternOffset1 = 0;
		delete pixmapIndentGuide;
		pixmapIndentGuide = 0;
		delete pixmapIndentGuideHighlight;
		pixmapIndentGuideHighlight = 0;
	} else {
		if (pixmapLine)
			pixmapLine->Release();
		if (pixmapSelMargin)
			pixmapSelMargin->Release();
		if (pixmapSelPattern)
			pixmapSelPattern->Release();
		if (pixmapSelPatternOffset1)
			pixmapSelPatternOffset1->Release();
		if (pixmapIndentGuide)
			pixmapIndentGuide->Release();
		if (pixmapIndentGuideHighlight)
			pixmapIndentGuideHighlight->Release();
	}
}

void Editor::AllocateGraphics() {
	if (!pixmapLine)
		pixmapLine = Surface::Allocate(technology);
	if (!pixmapSelMargin)
		pixmapSelMargin = Surface::Allocate(technology);
	if (!pixmapSelPattern)
		pixmapSelPattern = Surface::Allocate(technology);
	if (!pixmapSelPatternOffset1)
		pixmapSelPatternOffset1 = Surface::Allocate(technology);
	if (!pixmapIndentGuide)
		pixmapIndentGuide = Surface::Allocate(technology);
	if (!pixmapIndentGuideHighlight)
		pixmapIndentGuideHighlight = Surface::Allocate(technology);
}

void Editor::InvalidateStyleData() {
	stylesValid = false;
	vs.technology = technology;
	DropGraphics(false);
	AllocateGraphics();
	llc.Invalidate(LineLayout::llInvalid);
	posCache.Clear();
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
			vs.Refresh(*surface);
		}
		SetScrollBars();
		SetRectangularRange();
	}
}

Point Editor::GetVisibleOriginInMain() {
	return Point(0,0);
}

Point Editor::DocumentPointFromView(Point ptView) {
	Point ptDocument = ptView;
	if (wMargin.GetID()) {
		Point ptOrigin = GetVisibleOriginInMain();
		ptDocument.x += ptOrigin.x;
		ptDocument.y += ptOrigin.y;
	} else {
		ptDocument.x += xOffset;
		ptDocument.y += topLine * vs.lineHeight;
	}
	return ptDocument;
}

int Editor::TopLineOfMain() const {
	if (wMargin.GetID())
		return 0;
	else
		return topLine;
}

PRectangle Editor::GetClientRectangle() {
	return wMain.GetClientPosition();
}

PRectangle Editor::GetTextRectangle() {
	PRectangle rc = GetClientRectangle();
	rc.left += vs.textStart;
	rc.right -= vs.rightMarginWidth;
	return rc;
}

int Editor::LinesOnScreen() {
	PRectangle rcClient = GetClientRectangle();
	int htClient = rcClient.bottom - rcClient.top;
	//Platform::DebugPrintf("lines on screen = %d\n", htClient / lineHeight + 1);
	return htClient / vs.lineHeight;
}

int Editor::LinesToScroll() {
	int retVal = LinesOnScreen() - 1;
	if (retVal < 1)
		return 1;
	else
		return retVal;
}

int Editor::MaxScrollPos() {
	//Platform::DebugPrintf("Lines %d screen = %d maxScroll = %d\n",
	//LinesTotal(), LinesOnScreen(), LinesTotal() - LinesOnScreen() + 1);
	int retVal = cs.LinesDisplayed();
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

const char *ControlCharacterString(unsigned char ch) {
	const char *reps[] = {
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		"BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US"
	};
	if (ch < (sizeof(reps) / sizeof(reps[0]))) {
		return reps[ch];
	} else {
		return "BAD";
	}
}

/**
 * Convenience class to ensure LineLayout objects are always disposed.
 */
class AutoLineLayout {
	LineLayoutCache &llc;
	LineLayout *ll;
	AutoLineLayout &operator=(const AutoLineLayout &);
public:
	AutoLineLayout(LineLayoutCache &llc_, LineLayout *ll_) : llc(llc_), ll(ll_) {}
	~AutoLineLayout() {
		llc.Dispose(ll);
		ll = 0;
	}
	LineLayout *operator->() const {
		return ll;
	}
	operator LineLayout *() const {
		return ll;
	}
	void Set(LineLayout *ll_) {
		llc.Dispose(ll);
		ll = ll_;
	}
};

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

Point Editor::LocationFromPosition(SelectionPosition pos) {
	Point pt;
	RefreshStyleData();
	if (pos.Position() == INVALID_POSITION)
		return pt;
	int line = pdoc->LineFromPosition(pos.Position());
	int lineVisible = cs.DisplayFromDoc(line);
	//Platform::DebugPrintf("line=%d\n", line);
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(line));
	if (surface && ll) {
		// -1 because of adding in for visible lines in following loop.
		pt.y = (lineVisible - topLine - 1) * vs.lineHeight;
		pt.x = 0;
		unsigned int posLineStart = pdoc->LineStart(line);
		LayoutLine(line, surface, vs, ll, wrapWidth);
		int posInLine = pos.Position() - posLineStart;
		// In case of very long line put x at arbitrary large position
		if (posInLine > ll->maxLineLength) {
			pt.x = ll->positions[ll->maxLineLength] - ll->positions[ll->LineStart(ll->lines)];
		}

		for (int subLine = 0; subLine < ll->lines; subLine++) {
			if ((posInLine >= ll->LineStart(subLine)) && (posInLine <= ll->LineStart(subLine + 1))) {
				pt.x = ll->positions[posInLine] - ll->positions[ll->LineStart(subLine)];
				if (ll->wrapIndent != 0) {
					int lineStart = ll->LineStart(subLine);
					if (lineStart != 0)	// Wrapped
						pt.x += ll->wrapIndent;
				}
			}
			if (posInLine >= ll->LineStart(subLine)) {
				pt.y += vs.lineHeight;
			}
		}
		pt.x += vs.textStart - xOffset;
	}
	pt.x += pos.VirtualSpace() * vs.styles[ll->EndLineStyle()].spaceWidth;
	return pt;
}

Point Editor::LocationFromPosition(int pos) {
	return LocationFromPosition(SelectionPosition(pos));
}

int Editor::XFromPosition(int pos) {
	Point pt = LocationFromPosition(pos);
	return pt.x - vs.textStart + xOffset;
}

int Editor::XFromPosition(SelectionPosition sp) {
	Point pt = LocationFromPosition(sp);
	return pt.x - vs.textStart + xOffset;
}

int Editor::LineFromLocation(Point pt) const {
	return cs.DocFromDisplay(pt.y / vs.lineHeight + topLine);
}

void Editor::SetTopLine(int topLineNew) {
	if ((topLine != topLineNew) && (topLineNew >= 0)) {
		topLine = topLineNew;
		ContainerNeedsUpdate(SC_UPDATE_V_SCROLL);
	}
	posTopLine = pdoc->LineStart(cs.DocFromDisplay(topLine));
}

SelectionPosition Editor::SPositionFromLocation(Point pt, bool canReturnInvalid, bool charPosition, bool virtualSpace) {
	RefreshStyleData();
	if (canReturnInvalid) {
		PRectangle rcClient = GetTextRectangle();
		if (!rcClient.Contains(pt))
			return SelectionPosition(INVALID_POSITION);
		if (pt.x < vs.textStart)
			return SelectionPosition(INVALID_POSITION);
		if (pt.y < 0)
			return SelectionPosition(INVALID_POSITION);
	}
	pt = DocumentPointFromView(pt);
	pt.x = pt.x - vs.textStart;
	int visibleLine = floor(pt.y / vs.lineHeight);
	if (!canReturnInvalid && (visibleLine < 0))
		visibleLine = 0;
	int lineDoc = cs.DocFromDisplay(visibleLine);
	if (canReturnInvalid && (lineDoc < 0))
		return SelectionPosition(INVALID_POSITION);
	if (lineDoc >= pdoc->LinesTotal())
		return SelectionPosition(canReturnInvalid ? INVALID_POSITION : pdoc->Length());
	unsigned int posLineStart = pdoc->LineStart(lineDoc);
	SelectionPosition retVal(canReturnInvalid ? INVALID_POSITION : static_cast<int>(posLineStart));
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc));
	if (surface && ll) {
		LayoutLine(lineDoc, surface, vs, ll, wrapWidth);
		int lineStartSet = cs.DisplayFromDoc(lineDoc);
		int subLine = visibleLine - lineStartSet;
		if (subLine < ll->lines) {
			int lineStart = ll->LineStart(subLine);
			int lineEnd = ll->LineLastVisible(subLine);
			XYPOSITION subLineStart = ll->positions[lineStart];

			if (ll->wrapIndent != 0) {
				if (lineStart != 0)	// Wrapped
					pt.x -= ll->wrapIndent;
			}
			int i = ll->FindBefore(pt.x + subLineStart, lineStart, lineEnd);
			while (i < lineEnd) {
				if (charPosition) {
					if ((pt.x + subLineStart) < (ll->positions[i + 1])) {
						return SelectionPosition(pdoc->MovePositionOutsideChar(i + posLineStart, 1));
					}
				} else {
					if ((pt.x + subLineStart) < ((ll->positions[i] + ll->positions[i + 1]) / 2)) {
						return SelectionPosition(pdoc->MovePositionOutsideChar(i + posLineStart, 1));
					}
				}
				i++;
			}
			if (virtualSpace) {
				const XYPOSITION spaceWidth = vs.styles[ll->EndLineStyle()].spaceWidth;
				int spaceOffset = (pt.x + subLineStart - ll->positions[lineEnd] + spaceWidth / 2) /
					spaceWidth;
				return SelectionPosition(lineEnd + posLineStart, spaceOffset);
			} else if (canReturnInvalid) {
				if (pt.x < (ll->positions[lineEnd] - subLineStart)) {
					return SelectionPosition(pdoc->MovePositionOutsideChar(lineEnd + posLineStart, 1));
				}
			} else {
				return SelectionPosition(lineEnd + posLineStart);
			}
		}
		if (!canReturnInvalid)
			return SelectionPosition(ll->numCharsInLine + posLineStart);
	}
	return retVal;
}

int Editor::PositionFromLocation(Point pt, bool canReturnInvalid, bool charPosition) {
	return SPositionFromLocation(pt, canReturnInvalid, charPosition, false).Position();
}

/**
 * Find the document position corresponding to an x coordinate on a particular document line.
 * Ensure is between whole characters when document is in multi-byte or UTF-8 mode.
 */
SelectionPosition Editor::SPositionFromLineX(int lineDoc, int x) {
	RefreshStyleData();
	if (lineDoc >= pdoc->LinesTotal())
		return SelectionPosition(pdoc->Length());
	//Platform::DebugPrintf("Position of (%d,%d) line = %d top=%d\n", pt.x, pt.y, line, topLine);
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc));
	int retVal = 0;
	if (surface && ll) {
		unsigned int posLineStart = pdoc->LineStart(lineDoc);
		LayoutLine(lineDoc, surface, vs, ll, wrapWidth);
		int subLine = 0;
		int lineStart = ll->LineStart(subLine);
		int lineEnd = ll->LineLastVisible(subLine);
		XYPOSITION subLineStart = ll->positions[lineStart];
		XYPOSITION newX = x;

		if (ll->wrapIndent != 0) {
			if (lineStart != 0)	// Wrapped
				newX -= ll->wrapIndent;
		}
		int i = ll->FindBefore(newX + subLineStart, lineStart, lineEnd);
		while (i < lineEnd) {
			if ((newX + subLineStart) < ((ll->positions[i] + ll->positions[i + 1]) / 2)) {
				retVal = pdoc->MovePositionOutsideChar(i + posLineStart, 1);
				return SelectionPosition(retVal);
			}
			i++;
		}
		const XYPOSITION spaceWidth = vs.styles[ll->EndLineStyle()].spaceWidth;
		int spaceOffset = (newX + subLineStart - ll->positions[lineEnd] + spaceWidth / 2) / spaceWidth;
		return SelectionPosition(lineEnd + posLineStart, spaceOffset);
	}
	return SelectionPosition(retVal);
}

int Editor::PositionFromLineX(int lineDoc, int x) {
	return SPositionFromLineX(lineDoc, x).Position();
}

/**
 * If painting then abandon the painting because a wider redraw is needed.
 * @return true if calling code should stop drawing.
 */
bool Editor::AbandonPaint() {
	if ((paintState == painting) && !paintingAllText) {
		paintState = paintAbandoned;
	}
	return paintState == paintAbandoned;
}

void Editor::RedrawRect(PRectangle rc) {
	//Platform::DebugPrintf("Redraw %0d,%0d - %0d,%0d\n", rc.left, rc.top, rc.right, rc.bottom);

	// Clip the redraw rectangle into the client area
	PRectangle rcClient = GetClientRectangle();
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

void Editor::Redraw() {
	//Platform::DebugPrintf("Redraw all\n");
	PRectangle rcClient = GetClientRectangle();
	wMain.InvalidateRectangle(rcClient);
	if (wMargin.GetID())
		wMargin.InvalidateAll();
	//wMain.InvalidateAll();
}

void Editor::RedrawSelMargin(int line, bool allAfter) {
	if (!AbandonPaint()) {
		if (vs.maskInLine) {
			Redraw();
		} else {
			PRectangle rcSelMargin = GetClientRectangle();
			rcSelMargin.right = rcSelMargin.left + vs.fixedColumnWidth;
			if (line != -1) {
				int position = pdoc->LineStart(line);
				PRectangle rcLine = RectangleFromRange(position, position);

				// Inflate line rectangle if there are image markers with height larger than line height
				if (vs.largestMarkerHeight > vs.lineHeight) {
					int delta = (vs.largestMarkerHeight - vs.lineHeight + 1) / 2;
					rcLine.top -= delta;
					rcLine.bottom += delta;
					if (rcLine.top < rcSelMargin.top)
						rcLine.top = rcSelMargin.top;
					if (rcLine.bottom > rcSelMargin.bottom)
						rcLine.bottom = rcSelMargin.bottom;
				}

				rcSelMargin.top = rcLine.top;
				if (!allAfter)
					rcSelMargin.bottom = rcLine.bottom;
			}
			if (wMargin.GetID()) {
				Point ptOrigin = GetVisibleOriginInMain();
				rcSelMargin.Move(-ptOrigin.x, -ptOrigin.y);
				wMargin.InvalidateRectangle(rcSelMargin);
			} else {
				wMain.InvalidateRectangle(rcSelMargin);
			}
		}
	}
}

PRectangle Editor::RectangleFromRange(int start, int end) {
	int minPos = start;
	if (minPos > end)
		minPos = end;
	int maxPos = start;
	if (maxPos < end)
		maxPos = end;
	int minLine = cs.DisplayFromDoc(pdoc->LineFromPosition(minPos));
	int lineDocMax = pdoc->LineFromPosition(maxPos);
	int maxLine = cs.DisplayFromDoc(lineDocMax) + cs.GetHeight(lineDocMax) - 1;
	PRectangle rcClient = GetTextRectangle();
	PRectangle rc;
	const int leftTextOverlap = ((xOffset == 0) && (vs.leftMarginWidth > 0)) ? 1 : 0;
	rc.left = vs.textStart - leftTextOverlap;
	rc.top = (minLine - TopLineOfMain()) * vs.lineHeight;
	if (rc.top < rcClient.top)
		rc.top = rcClient.top;
	rc.right = rcClient.right;
	rc.bottom = (maxLine - TopLineOfMain() + 1) * vs.lineHeight;

	return rc;
}

void Editor::InvalidateRange(int start, int end) {
	RedrawRect(RectangleFromRange(start, end));
}

int Editor::CurrentPosition() const {
	return sel.MainCaret();
}

bool Editor::SelectionEmpty() const {
	return sel.Empty();
}

SelectionPosition Editor::SelectionStart() {
	return sel.RangeMain().Start();
}

SelectionPosition Editor::SelectionEnd() {
	return sel.RangeMain().End();
}

void Editor::SetRectangularRange() {
	if (sel.IsRectangular()) {
		int xAnchor = XFromPosition(sel.Rectangular().anchor);
		int xCaret = XFromPosition(sel.Rectangular().caret);
		if (sel.selType == Selection::selThin) {
			xCaret = xAnchor;
		}
		int lineAnchorRect = pdoc->LineFromPosition(sel.Rectangular().anchor.Position());
		int lineCaret = pdoc->LineFromPosition(sel.Rectangular().caret.Position());
		int increment = (lineCaret > lineAnchorRect) ? 1 : -1;
		for (int line=lineAnchorRect; line != lineCaret+increment; line += increment) {
			SelectionRange range(SPositionFromLineX(line, xCaret), SPositionFromLineX(line, xAnchor));
			if ((virtualSpaceOptions & SCVS_RECTANGULARSELECTION) == 0)
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
		sel.selType = Selection::selThin;
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
	int firstAffected = Platform::Minimum(sel.RangeMain().Start().Position(), newMain.Start().Position());
	// +1 for lastAffected ensures caret repainted
	int lastAffected = Platform::Maximum(newMain.caret.Position()+1, newMain.anchor.Position());
	lastAffected = Platform::Maximum(lastAffected, sel.RangeMain().End().Position());
	if (invalidateWholeSelection) {
		for (size_t r=0; r<sel.Count(); r++) {
			firstAffected = Platform::Minimum(firstAffected, sel.Range(r).caret.Position());
			firstAffected = Platform::Minimum(firstAffected, sel.Range(r).anchor.Position());
			lastAffected = Platform::Maximum(lastAffected, sel.Range(r).caret.Position()+1);
			lastAffected = Platform::Maximum(lastAffected, sel.Range(r).anchor.Position());
		}
	}
	ContainerNeedsUpdate(SC_UPDATE_SELECTION);
	InvalidateRange(firstAffected, lastAffected);
}

void Editor::SetSelection(SelectionPosition currentPos_, SelectionPosition anchor_) {
	currentPos_ = ClampPositionIntoDocument(currentPos_);
	anchor_ = ClampPositionIntoDocument(anchor_);
	int currentLine = pdoc->LineFromPosition(currentPos_.Position());
	/* For Line selection - ensure the anchor and caret are always
	   at the beginning and end of the region lines. */
	if (sel.selType == Selection::selLines) {
		if (currentPos_ > anchor_) {
			anchor_ = SelectionPosition(pdoc->LineStart(pdoc->LineFromPosition(anchor_.Position())));
			currentPos_ = SelectionPosition(pdoc->LineEnd(pdoc->LineFromPosition(currentPos_.Position())));
		} else {
			currentPos_ = SelectionPosition(pdoc->LineStart(pdoc->LineFromPosition(currentPos_.Position())));
			anchor_ = SelectionPosition(pdoc->LineEnd(pdoc->LineFromPosition(anchor_.Position())));
		}
	}
	SelectionRange rangeNew(currentPos_, anchor_);
	if (sel.Count() > 1 || !(sel.RangeMain() == rangeNew)) {
		InvalidateSelection(rangeNew);
	}
	sel.RangeMain() = rangeNew;
	SetRectangularRange();
	ClaimSelection();

	if (highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkNeeded::workUpdateUI);
}

void Editor::SetSelection(int currentPos_, int anchor_) {
	SetSelection(SelectionPosition(currentPos_), SelectionPosition(anchor_));
}

// Just move the caret on the main selection
void Editor::SetSelection(SelectionPosition currentPos_) {
	currentPos_ = ClampPositionIntoDocument(currentPos_);
	int currentLine = pdoc->LineFromPosition(currentPos_.Position());
	if (sel.Count() > 1 || !(sel.RangeMain().caret == currentPos_)) {
		InvalidateSelection(SelectionRange(currentPos_));
	}
	if (sel.IsRectangular()) {
		sel.Rectangular() =
			SelectionRange(SelectionPosition(currentPos_), sel.Rectangular().anchor);
		SetRectangularRange();
	} else {
		sel.RangeMain() =
			SelectionRange(SelectionPosition(currentPos_), sel.RangeMain().anchor);
	}
	ClaimSelection();

	if (highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkNeeded::workUpdateUI);
}

void Editor::SetSelection(int currentPos_) {
	SetSelection(SelectionPosition(currentPos_));
}

void Editor::SetEmptySelection(SelectionPosition currentPos_) {
	int currentLine = pdoc->LineFromPosition(currentPos_.Position());
	SelectionRange rangeNew(ClampPositionIntoDocument(currentPos_));
	if (sel.Count() > 1 || !(sel.RangeMain() == rangeNew)) {
		InvalidateSelection(rangeNew);
	}
	sel.Clear();
	sel.RangeMain() = rangeNew;
	SetRectangularRange();
	ClaimSelection();

	if (highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkNeeded::workUpdateUI);
}

void Editor::SetEmptySelection(int currentPos_) {
	SetEmptySelection(SelectionPosition(currentPos_));
}

bool Editor::RangeContainsProtected(int start, int end) const {
	if (vs.ProtectionActive()) {
		if (start > end) {
			int t = start;
			start = end;
			end = t;
		}
		int mask = pdoc->stylingBitsMask;
		for (int pos = start; pos < end; pos++) {
			if (vs.styles[pdoc->StyleAt(pos) & mask].IsProtected())
				return true;
		}
	}
	return false;
}

bool Editor::SelectionContainsProtected() {
	for (size_t r=0; r<sel.Count(); r++) {
		if (RangeContainsProtected(sel.Range(r).Start().Position(),
			sel.Range(r).End().Position())) {
			return true;
		}
	}
	return false;
}

/**
 * Asks document to find a good position and then moves out of any invisible positions.
 */
int Editor::MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd) const {
	return MovePositionOutsideChar(SelectionPosition(pos), moveDir, checkLineEnd).Position();
}

SelectionPosition Editor::MovePositionOutsideChar(SelectionPosition pos, int moveDir, bool checkLineEnd) const {
	int posMoved = pdoc->MovePositionOutsideChar(pos.Position(), moveDir, checkLineEnd);
	if (posMoved != pos.Position())
		pos.SetPosition(posMoved);
	if (vs.ProtectionActive()) {
		int mask = pdoc->stylingBitsMask;
		if (moveDir > 0) {
			if ((pos.Position() > 0) && vs.styles[pdoc->StyleAt(pos.Position() - 1) & mask].IsProtected()) {
				while ((pos.Position() < pdoc->Length()) &&
				        (vs.styles[pdoc->StyleAt(pos.Position()) & mask].IsProtected()))
					pos.Add(1);
			}
		} else if (moveDir < 0) {
			if (vs.styles[pdoc->StyleAt(pos.Position()) & mask].IsProtected()) {
				while ((pos.Position() > 0) &&
				        (vs.styles[pdoc->StyleAt(pos.Position() - 1) & mask].IsProtected()))
					pos.Add(-1);
			}
		}
	}
	return pos;
}

int Editor::MovePositionTo(SelectionPosition newPos, Selection::selTypes selt, bool ensureVisible) {
	bool simpleCaret = (sel.Count() == 1) && sel.Empty();
	SelectionPosition spCaret = sel.Last();

	int delta = newPos.Position() - sel.MainCaret();
	newPos = ClampPositionIntoDocument(newPos);
	newPos = MovePositionOutsideChar(newPos, delta);
	if (!multipleSelection && sel.IsRectangular() && (selt == Selection::selStream)) {
		// Can't turn into multiple selection so clear additional selections
		InvalidateSelection(SelectionRange(newPos), true);
		SelectionRange rangeMain = sel.RangeMain();
		sel.SetSelection(rangeMain);
	}
	if (!sel.IsRectangular() && (selt == Selection::selRectangle)) {
		// Switching to rectangular
		SelectionRange rangeMain = sel.RangeMain();
		sel.Clear();
		sel.Rectangular() = rangeMain;
	}
	if (selt != Selection::noSel) {
		sel.selType = selt;
	}
	if (selt != Selection::noSel || sel.MoveExtends()) {
		SetSelection(newPos);
	} else {
		SetEmptySelection(newPos);
	}
	ShowCaretAtCurrentPosition();

	int currentLine = pdoc->LineFromPosition(newPos.Position());
	if (ensureVisible) {
		// In case in need of wrapping to ensure DisplayFromDoc works.
		if (currentLine >= wrapPending.start)
			WrapLines(wsAll);
		XYScrollPosition newXY = XYScrollToMakeVisible(
			SelectionRange(posDrag.IsValid() ? posDrag : sel.RangeMain().caret), xysDefault);
		if (simpleCaret && (newXY.xOffset == xOffset)) {
			// simple vertical scroll then invalidate
			ScrollTo(newXY.topLine);
			InvalidateSelection(SelectionRange(spCaret), true);
		} else {
			SetXYScroll(newXY);
		}
	}

	if (highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	return 0;
}

int Editor::MovePositionTo(int newPos, Selection::selTypes selt, bool ensureVisible) {
	return MovePositionTo(SelectionPosition(newPos), selt, ensureVisible);
}

SelectionPosition Editor::MovePositionSoVisible(SelectionPosition pos, int moveDir) {
	pos = ClampPositionIntoDocument(pos);
	pos = MovePositionOutsideChar(pos, moveDir);
	int lineDoc = pdoc->LineFromPosition(pos.Position());
	if (cs.GetVisible(lineDoc)) {
		return pos;
	} else {
		int lineDisplay = cs.DisplayFromDoc(lineDoc);
		if (moveDir > 0) {
			// lineDisplay is already line before fold as lines in fold use display line of line after fold
			lineDisplay = Platform::Clamp(lineDisplay, 0, cs.LinesDisplayed());
			return SelectionPosition(pdoc->LineStart(cs.DocFromDisplay(lineDisplay)));
		} else {
			lineDisplay = Platform::Clamp(lineDisplay - 1, 0, cs.LinesDisplayed());
			return SelectionPosition(pdoc->LineEnd(cs.DocFromDisplay(lineDisplay)));
		}
	}
}

SelectionPosition Editor::MovePositionSoVisible(int pos, int moveDir) {
	return MovePositionSoVisible(SelectionPosition(pos), moveDir);
}

Point Editor::PointMainCaret() {
	return LocationFromPosition(sel.Range(sel.Main()).caret);
}

/**
 * Choose the x position that the caret will try to stick to
 * as it moves up and down.
 */
void Editor::SetLastXChosen() {
	Point pt = PointMainCaret();
	lastXChosen = pt.x + xOffset;
}

void Editor::ScrollTo(int line, bool moveThumb) {
	int topLineNew = Platform::Clamp(line, 0, MaxScrollPos());
	if (topLineNew != topLine) {
		// Try to optimise small scrolls
#ifndef UNDER_CE
		int linesToMove = topLine - topLineNew;
		bool performBlit = (abs(linesToMove) <= 10) && (paintState == notPainting);
		willRedrawAll = !performBlit;
#endif
		SetTopLine(topLineNew);
		// Optimize by styling the view as this will invalidate any needed area
		// which could abort the initial paint if discovered later.
		StyleToPositionInView(PositionAfterArea(GetClientRectangle()));
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

void Editor::ScrollText(int /* linesToMove */) {
	//Platform::DebugPrintf("Editor::ScrollText %d\n", linesToMove);
	Redraw();
}

void Editor::HorizontalScrollTo(int xPos) {
	//Platform::DebugPrintf("HorizontalScroll %d\n", xPos);
	if (xPos < 0)
		xPos = 0;
	if ((wrapState == eWrapNone) && (xOffset != xPos)) {
		xOffset = xPos;
		ContainerNeedsUpdate(SC_UPDATE_H_SCROLL);
		SetHorizontalScrollPos();
		RedrawRect(GetClientRectangle());
	}
}

void Editor::VerticalCentreCaret() {
	int lineDoc = pdoc->LineFromPosition(sel.IsRectangular() ? sel.Rectangular().caret.Position() : sel.MainCaret());
	int lineDisplay = cs.DisplayFromDoc(lineDoc);
	int newTop = lineDisplay - (LinesOnScreen() / 2);
	if (topLine != newTop) {
		SetTopLine(newTop > 0 ? newTop : 0);
		RedrawRect(GetClientRectangle());
	}
}

// Avoid 64 bit compiler warnings.
// Scintilla does not support text buffers larger than 2**31
static int istrlen(const char *s) {
	return static_cast<int>(strlen(s));
}

void Editor::MoveSelectedLines(int lineDelta) {

	// if selection doesn't start at the beginning of the line, set the new start
	int selectionStart = SelectionStart().Position();
	int startLine = pdoc->LineFromPosition(selectionStart);
	int beginningOfStartLine = pdoc->LineStart(startLine);
	selectionStart = beginningOfStartLine;

	// if selection doesn't end at the beginning of a line greater than that of the start,
	// then set it at the beginning of the next one
	int selectionEnd = SelectionEnd().Position();
	int endLine = pdoc->LineFromPosition(selectionEnd);
	int beginningOfEndLine = pdoc->LineStart(endLine);
	bool appendEol = false;
	if (selectionEnd > beginningOfEndLine
		|| selectionStart == selectionEnd) {
		selectionEnd = pdoc->LineStart(endLine + 1);
		appendEol = (selectionEnd == pdoc->Length() && pdoc->LineFromPosition(selectionEnd) == endLine);
	}

	// if there's nowhere for the selection to move
	// (i.e. at the beginning going up or at the end going down),
	// stop it right there!
	if ((selectionStart == 0 && lineDelta < 0)
		|| (selectionEnd == pdoc->Length() && lineDelta > 0)
	        || selectionStart == selectionEnd) {
		return;
	}

	UndoGroup ug(pdoc);

	if (lineDelta > 0 && selectionEnd == pdoc->LineStart(pdoc->LinesTotal() - 1)) {
		SetSelection(pdoc->MovePositionOutsideChar(selectionEnd - 1, -1), selectionEnd);
		ClearSelection();
		selectionEnd = CurrentPosition();
	}
	SetSelection(selectionStart, selectionEnd);

	SelectionText selectedText;
	CopySelectionRange(&selectedText);

	int selectionLength = SelectionRange(selectionStart, selectionEnd).Length();
	Point currentLocation = LocationFromPosition(CurrentPosition());
	int currentLine = LineFromLocation(currentLocation);

	if (appendEol)
		SetSelection(pdoc->MovePositionOutsideChar(selectionStart - 1, -1), selectionEnd);
	ClearSelection();

	const char *eol = StringFromEOLMode(pdoc->eolMode);
	if (currentLine + lineDelta >= pdoc->LinesTotal())
		pdoc->InsertCString(pdoc->Length(), eol);
	GoToLine(currentLine + lineDelta);

	pdoc->InsertCString(CurrentPosition(), selectedText.Data());
	if (appendEol) {
		pdoc->InsertCString(CurrentPosition() + selectionLength, eol);
		selectionLength += istrlen(eol);
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
	PRectangle rcClient = GetTextRectangle();
	Point pt = PointMainCaret();
	if (pt.y < rcClient.top) {
		MovePositionTo(SPositionFromLocation(
		            Point(lastXChosen - xOffset, rcClient.top),
					false, false, UserVirtualSpace()),
					Selection::noSel, ensureVisible);
	} else if ((pt.y + vs.lineHeight - 1) > rcClient.bottom) {
		int yOfLastLineFullyDisplayed = rcClient.top + (LinesOnScreen() - 1) * vs.lineHeight;
		MovePositionTo(SPositionFromLocation(
		            Point(lastXChosen - xOffset, rcClient.top + yOfLastLineFullyDisplayed),
					false, false, UserVirtualSpace()),
		        Selection::noSel, ensureVisible);
	}
}

int Editor::DisplayFromPosition(int pos) {
	int lineDoc = pdoc->LineFromPosition(pos);
	int lineDisplay = cs.DisplayFromDoc(lineDoc);
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc));
	if (surface && ll) {
		LayoutLine(lineDoc, surface, vs, ll, wrapWidth);
		unsigned int posLineStart = pdoc->LineStart(lineDoc);
		int posInLine = pos - posLineStart;
		lineDisplay--; // To make up for first increment ahead.
		for (int subLine = 0; subLine < ll->lines; subLine++) {
			if (posInLine >= ll->LineStart(subLine)) {
				lineDisplay++;
			}
		}
	}
	return lineDisplay;
}

/**
 * Ensure the caret is reasonably visible in context.
 *
Caret policy in SciTE

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
This way, we favour the displaying of useful information: the begining of lines,
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

Editor::XYScrollPosition Editor::XYScrollToMakeVisible(const SelectionRange range, const XYScrollOptions options) {
	PRectangle rcClient = GetTextRectangle();
	Point pt = LocationFromPosition(range.caret);
	Point ptAnchor = LocationFromPosition(range.anchor);
	const Point ptOrigin = GetVisibleOriginInMain();
	pt.x += ptOrigin.x;
	pt.y += ptOrigin.y;
	ptAnchor.x += ptOrigin.x;
	ptAnchor.y += ptOrigin.y;
	const Point ptBottomCaret(pt.x, pt.y + vs.lineHeight - 1);

	XYScrollPosition newXY(xOffset, topLine);
	if (rcClient.Empty()) {
		return newXY;
	}

	// Vertical positioning
	if ((options & xysVertical) && (pt.y < rcClient.top || ptBottomCaret.y >= rcClient.bottom || (caretYPolicy & CARET_STRICT) != 0)) {
		const int lineCaret = DisplayFromPosition(range.caret.Position());
		const int linesOnScreen = LinesOnScreen();
		const int halfScreen = Platform::Maximum(linesOnScreen - 1, 2) / 2;
		const bool bSlop = (caretYPolicy & CARET_SLOP) != 0;
		const bool bStrict = (caretYPolicy & CARET_STRICT) != 0;
		const bool bJump = (caretYPolicy & CARET_JUMPS) != 0;
		const bool bEven = (caretYPolicy & CARET_EVEN) != 0;

		// It should be possible to scroll the window to show the caret,
		// but this fails to remove the caret on GTK+
		if (bSlop) {	// A margin is defined
			int yMoveT, yMoveB;
			if (bStrict) {
				int yMarginT, yMarginB;
				if (!(options & xysUseMargin)) {
					// In drag mode, avoid moves
					// otherwise, a double click will select several lines.
					yMarginT = yMarginB = 0;
				} else {
					// yMarginT must equal to caretYSlop, with a minimum of 1 and
					// a maximum of slightly less than half the heigth of the text area.
					yMarginT = Platform::Clamp(caretYSlop, 1, halfScreen);
					if (bEven) {
						yMarginB = yMarginT;
					} else {
						yMarginB = linesOnScreen - yMarginT - 1;
					}
				}
				yMoveT = yMarginT;
				if (bEven) {
					if (bJump) {
						yMoveT = Platform::Clamp(caretYSlop * 3, 1, halfScreen);
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
				yMoveT = bJump ? caretYSlop * 3 : caretYSlop;
				yMoveT = Platform::Clamp(yMoveT, 1, halfScreen);
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
					// Always center caret
					newXY.topLine = lineCaret - halfScreen;
				} else {
					// Always put caret on top of display
					newXY.topLine = lineCaret;
				}
			}
		}
		if (!(range.caret == range.anchor)) {
			const int lineAnchor = DisplayFromPosition(range.anchor.Position());
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
		newXY.topLine = Platform::Clamp(newXY.topLine, 0, MaxScrollPos());
	}

	// Horizontal positioning
	if ((options & xysHorizontal) && (wrapState == eWrapNone)) {
		const int halfScreen = Platform::Maximum(rcClient.Width() - 4, 4) / 2;
		const bool bSlop = (caretXPolicy & CARET_SLOP) != 0;
		const bool bStrict = (caretXPolicy & CARET_STRICT) != 0;
		const bool bJump = (caretXPolicy & CARET_JUMPS) != 0;
		const bool bEven = (caretXPolicy & CARET_EVEN) != 0;

		if (bSlop) {	// A margin is defined
			int xMoveL, xMoveR;
			if (bStrict) {
				int xMarginL, xMarginR;
				if (!(options & xysUseMargin)) {
					// In drag mode, avoid moves unless very near of the margin
					// otherwise, a simple click will select text.
					xMarginL = xMarginR = 2;
				} else {
					// xMargin must equal to caretXSlop, with a minimum of 2 and
					// a maximum of slightly less than half the width of the text area.
					xMarginR = Platform::Clamp(caretXSlop, 2, halfScreen);
					if (bEven) {
						xMarginL = xMarginR;
					} else {
						xMarginL = rcClient.Width() - xMarginR - 4;
					}
				}
				if (bJump && bEven) {
					// Jump is used only in even mode
					xMoveL = xMoveR = Platform::Clamp(caretXSlop * 3, 1, halfScreen);
				} else {
					xMoveL = xMoveR = 0;	// Not used, avoid a warning
				}
				if (pt.x < rcClient.left + xMarginL) {
					// Caret is on the left of the display
					if (bJump && bEven) {
						newXY.xOffset -= xMoveL;
					} else {
						// Move just enough to allow to display the caret
						newXY.xOffset -= (rcClient.left + xMarginL) - pt.x;
					}
				} else if (pt.x >= rcClient.right - xMarginR) {
					// Caret is on the right of the display
					if (bJump && bEven) {
						newXY.xOffset += xMoveR;
					} else {
						// Move just enough to allow to display the caret
						newXY.xOffset += pt.x - (rcClient.right - xMarginR) + 1;
					}
				}
			} else {	// Not strict
				xMoveR = bJump ? caretXSlop * 3 : caretXSlop;
				xMoveR = Platform::Clamp(xMoveR, 1, halfScreen);
				if (bEven) {
					xMoveL = xMoveR;
				} else {
					xMoveL = rcClient.Width() - xMoveR - 4;
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
					// Center caret
					newXY.xOffset += pt.x - rcClient.left - halfScreen;
				} else {
					// Put caret on right
					newXY.xOffset += pt.x - rcClient.right + 1;
				}
			} else {
				// Move just enough to allow to display the caret
				if (pt.x < rcClient.left) {
					// Caret is on the left of the display
					if (bEven) {
						newXY.xOffset -= rcClient.left - pt.x;
					} else {
						newXY.xOffset += pt.x - rcClient.right + 1;
					}
				} else if (pt.x >= rcClient.right) {
					// Caret is on the right of the display
					newXY.xOffset += pt.x - rcClient.right + 1;
				}
			}
		}
		// In case of a jump (find result) largely out of display, adjust the offset to display the caret
		if (pt.x + xOffset < rcClient.left + newXY.xOffset) {
			newXY.xOffset = pt.x + xOffset - rcClient.left;
		} else if (pt.x + xOffset >= rcClient.right + newXY.xOffset) {
			newXY.xOffset = pt.x + xOffset - rcClient.right + 1;
			if (vs.caretStyle == CARETSTYLE_BLOCK) {
				// Ensure we can see a good portion of the block caret
				newXY.xOffset += static_cast<int>(vs.aveCharWidth);
			}
		}
		if (!(range.caret == range.anchor)) {
			if (ptAnchor.x < pt.x) {
				// Shift to left to show anchor or as much of range as possible
				int maxOffset = ptAnchor.x + xOffset - rcClient.left - 1;
				int minOffset = pt.x + xOffset - rcClient.right + 1;
				newXY.xOffset = std::min(newXY.xOffset, maxOffset);
				newXY.xOffset = std::max(newXY.xOffset, minOffset);
			} else {
				// Shift to right to show anchor or as much of range as possible
				int minOffset = ptAnchor.x + xOffset - rcClient.right + 1;
				int maxOffset = pt.x + xOffset - rcClient.left - 1;
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
			ContainerNeedsUpdate(SC_UPDATE_H_SCROLL);
			if (newXY.xOffset > 0) {
				PRectangle rcText = GetTextRectangle();
				if (horizontalScrollBarVisible &&
					rcText.Width() + xOffset > scrollWidth) {
					scrollWidth = xOffset + rcText.Width();
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
	SetXYScroll(XYScrollToMakeVisible(range, xysDefault));
}

void Editor::EnsureCaretVisible(bool useMargin, bool vert, bool horiz) {
	SetXYScroll(XYScrollToMakeVisible(SelectionRange(posDrag.IsValid() ? posDrag : sel.RangeMain().caret),
		static_cast<XYScrollOptions>((useMargin?xysUseMargin:0)|(vert?xysVertical:0)|(horiz?xysHorizontal:0))));
}

void Editor::ShowCaretAtCurrentPosition() {
	if (hasFocus) {
		caret.active = true;
		caret.on = true;
		SetTicking(true);
	} else {
		caret.active = false;
		caret.on = false;
	}
	InvalidateCaret();
}

void Editor::DropCaret() {
	caret.active = false;
	InvalidateCaret();
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

void Editor::UpdateSystemCaret() {
}

void Editor::NeedWrapping(int docLineStart, int docLineEnd) {
//Platform::DebugPrintf("\nNeedWrapping: %0d..%0d\n", docLineStart, docLineEnd);
	if (wrapPending.AddRange(docLineStart, docLineEnd)) {
		llc.Invalidate(LineLayout::llPositions);
	}
	// Wrap lines during idle.
	if ((wrapState != eWrapNone) && wrapPending.NeedsWrap()) {
		SetIdle(true);
	}
}

bool Editor::WrapOneLine(Surface *surface, int lineToWrap) {
	AutoLineLayout ll(llc, RetrieveLineLayout(lineToWrap));
	int linesWrapped = 1;
	if (ll) {
		LayoutLine(lineToWrap, surface, vs, ll, wrapWidth);
		linesWrapped = ll->lines;
	}
	return cs.SetHeight(lineToWrap, linesWrapped +
		(vs.annotationVisible ? pdoc->AnnotationLines(lineToWrap) : 0));
}

// Perform  wrapping for a subset of the lines needing wrapping.
// wsAll: wrap all lines which need wrapping in this single call
// wsVisible: wrap currently visible lines
// wsIdle: wrap one page + 100 lines
// Return true if wrapping occurred.
bool Editor::WrapLines(enum wrapScope ws) {
	int goodTopLine = topLine;
	bool wrapOccurred = false;
	if (wrapState == eWrapNone) {
		if (wrapWidth != LineLayout::wrapWidthInfinite) {
			wrapWidth = LineLayout::wrapWidthInfinite;
			for (int lineDoc = 0; lineDoc < pdoc->LinesTotal(); lineDoc++) {
				cs.SetHeight(lineDoc, 1 +
					(vs.annotationVisible ? pdoc->AnnotationLines(lineDoc) : 0));
			}
			wrapOccurred = true;
		}
		wrapPending.Reset();

	} else if (wrapPending.NeedsWrap()) {
		wrapPending.start = std::min(wrapPending.start, pdoc->LinesTotal());
		if (!SetIdle(true)) {
			// Idle processing not supported so full wrap required.
			ws = wsAll;
		}
		// Decide where to start wrapping
		int lineToWrap = wrapPending.start;
		int lineToWrapEnd = std::min(wrapPending.end, pdoc->LinesTotal());
		const int lineDocTop = cs.DocFromDisplay(topLine);
		const int subLineTop = topLine - cs.DisplayFromDoc(lineDocTop);
		if (ws == wsVisible) {
			lineToWrap = Platform::Clamp(lineDocTop-5, wrapPending.start, pdoc->LinesTotal());
			// Priority wrap to just after visible area.
			// Since wrapping could reduce display lines, treat each
			// as taking only one display line.
			lineToWrapEnd = lineDocTop;
			int lines = LinesOnScreen() + 1;
			while ((lineToWrapEnd < cs.LinesInDoc()) && (lines>0)) {
				if (cs.GetVisible(lineToWrapEnd))
					lines--;
				lineToWrapEnd++;
			}
			// .. and if the paint window is outside pending wraps
			if ((lineToWrap > wrapPending.end) || (lineToWrapEnd < wrapPending.start)) {
				// Currently visible text does not need wrapping
				return false;
			}
		} else if (ws == wsIdle) {
			lineToWrapEnd = lineToWrap + LinesOnScreen() + 100;
		}
		const int lineEndNeedWrap = std::min(wrapPending.end, pdoc->LinesTotal());
		lineToWrapEnd = std::min(lineToWrapEnd, lineEndNeedWrap);

		// Ensure all lines being wrapped are styled.
		pdoc->EnsureStyledTo(pdoc->LineStart(lineToWrapEnd));

		if (lineToWrap < lineToWrapEnd) {

			PRectangle rcTextArea = GetClientRectangle();
			rcTextArea.left = vs.textStart;
			rcTextArea.right -= vs.rightMarginWidth;
			wrapWidth = rcTextArea.Width();
			RefreshStyleData();
			AutoSurface surface(this);
			if (surface) {
//Platform::DebugPrintf("Wraplines: scope=%0d need=%0d..%0d perform=%0d..%0d\n", ws, wrapPending.start, wrapPending.end, lineToWrap, lineToWrapEnd);

				while (lineToWrap < lineToWrapEnd) {
					if (WrapOneLine(surface, lineToWrap)) {
						wrapOccurred = true;
					}
					wrapPending.Wrapped(lineToWrap);
					lineToWrap++;
				}

				goodTopLine = cs.DisplayFromDoc(lineDocTop) + std::min(subLineTop, cs.GetHeight(lineDocTop)-1);
			}
		}

		// If wrapping is done, bring it to resting position
		if (wrapPending.start >= lineEndNeedWrap) {
			wrapPending.Reset();
		}
	}

	if (wrapOccurred) {
		SetScrollBars();
		SetTopLine(Platform::Clamp(goodTopLine, 0, MaxScrollPos()));
		SetVerticalScrollPos();
	}

	return wrapOccurred;
}

void Editor::LinesJoin() {
	if (!RangeContainsProtected(targetStart, targetEnd)) {
		UndoGroup ug(pdoc);
		bool prevNonWS = true;
		for (int pos = targetStart; pos < targetEnd; pos++) {
			if (pdoc->IsPositionInLineEnd(pos)) {
				targetEnd -= pdoc->LenChar(pos);
				pdoc->DelChar(pos);
				if (prevNonWS) {
					// Ensure at least one space separating previous lines
					pdoc->InsertChar(pos, ' ');
					targetEnd++;
				}
			} else {
				prevNonWS = pdoc->CharAt(pos) != ' ';
			}
		}
	}
}

const char *Editor::StringFromEOLMode(int eolMode) {
	if (eolMode == SC_EOL_CRLF) {
		return "\r\n";
	} else if (eolMode == SC_EOL_CR) {
		return "\r";
	} else {
		return "\n";
	}
}

void Editor::LinesSplit(int pixelWidth) {
	if (!RangeContainsProtected(targetStart, targetEnd)) {
		if (pixelWidth == 0) {
			PRectangle rcText = GetTextRectangle();
			pixelWidth = rcText.Width();
		}
		int lineStart = pdoc->LineFromPosition(targetStart);
		int lineEnd = pdoc->LineFromPosition(targetEnd);
		const char *eol = StringFromEOLMode(pdoc->eolMode);
		UndoGroup ug(pdoc);
		for (int line = lineStart; line <= lineEnd; line++) {
			AutoSurface surface(this);
			AutoLineLayout ll(llc, RetrieveLineLayout(line));
			if (surface && ll) {
				unsigned int posLineStart = pdoc->LineStart(line);
				LayoutLine(line, surface, vs, ll, pixelWidth);
				for (int subLine = 1; subLine < ll->lines; subLine++) {
					pdoc->InsertCString(
						static_cast<int>(posLineStart + (subLine - 1) * strlen(eol) +
							ll->LineStart(subLine)),
						eol);
					targetEnd += static_cast<int>(strlen(eol));
				}
			}
			lineEnd = pdoc->LineFromPosition(targetEnd);
		}
	}
}

int Editor::SubstituteMarkerIfEmpty(int markerCheck, int markerDefault) const {
	if (vs.markers[markerCheck].markType == SC_MARK_EMPTY)
		return markerDefault;
	return markerCheck;
}

bool ValidStyledText(ViewStyle &vs, size_t styleOffset, const StyledText &st) {
	if (st.multipleStyles) {
		for (size_t iStyle=0; iStyle<st.length; iStyle++) {
			if (!vs.ValidStyle(styleOffset + st.styles[iStyle]))
				return false;
		}
	} else {
		if (!vs.ValidStyle(styleOffset + st.style))
			return false;
	}
	return true;
}

static int WidthStyledText(Surface *surface, ViewStyle &vs, int styleOffset,
	const char *text, const unsigned char *styles, size_t len) {
	int width = 0;
	size_t start = 0;
	while (start < len) {
		size_t style = styles[start];
		size_t endSegment = start;
		while ((endSegment+1 < len) && (static_cast<size_t>(styles[endSegment+1]) == style))
			endSegment++;
		width += surface->WidthText(vs.styles[style+styleOffset].font, text + start,
			static_cast<int>(endSegment - start + 1));
		start = endSegment + 1;
	}
	return width;
}

static int WidestLineWidth(Surface *surface, ViewStyle &vs, int styleOffset, const StyledText &st) {
	int widthMax = 0;
	size_t start = 0;
	while (start < st.length) {
		size_t lenLine = st.LineLength(start);
		int widthSubLine;
		if (st.multipleStyles) {
			widthSubLine = WidthStyledText(surface, vs, styleOffset, st.text + start, st.styles + start, lenLine);
		} else {
			widthSubLine = surface->WidthText(vs.styles[styleOffset + st.style].font,
				st.text + start, static_cast<int>(lenLine));
		}
		if (widthSubLine > widthMax)
			widthMax = widthSubLine;
		start += lenLine + 1;
	}
	return widthMax;
}

void DrawStyledText(Surface *surface, ViewStyle &vs, int styleOffset, PRectangle rcText, int ascent,
	const StyledText &st, size_t start, size_t length) {

	if (st.multipleStyles) {
		int x = rcText.left;
		size_t i = 0;
		while (i < length) {
			size_t end = i;
			int style = st.styles[i + start];
			while (end < length-1 && st.styles[start+end+1] == style)
				end++;
			style += styleOffset;
			int width = surface->WidthText(vs.styles[style].font,
				st.text + start + i, static_cast<int>(end - i + 1));
			PRectangle rcSegment = rcText;
			rcSegment.left = x;
			rcSegment.right = x + width + 1;
			surface->DrawTextNoClip(rcSegment, vs.styles[style].font,
					ascent, st.text + start + i,
					static_cast<int>(end - i + 1),
					vs.styles[style].fore,
					vs.styles[style].back);
			x += width;
			i = end + 1;
		}
	} else {
		size_t style = st.style + styleOffset;
		surface->DrawTextNoClip(rcText, vs.styles[style].font,
				rcText.top + vs.maxAscent, st.text + start,
				static_cast<int>(length),
				vs.styles[style].fore,
				vs.styles[style].back);
	}
}

void Editor::PaintSelMargin(Surface *surfWindow, PRectangle &rc) {
	if (vs.fixedColumnWidth == 0)
		return;

	AllocateGraphics();
	RefreshStyleData();
	RefreshPixMaps(surfWindow);

	PRectangle rcMargin = GetClientRectangle();
	Point ptOrigin = GetVisibleOriginInMain();
	rcMargin.Move(0, -ptOrigin.y);
	rcMargin.left = 0;
	rcMargin.right = vs.fixedColumnWidth;

	if (!rc.Intersects(rcMargin))
		return;

	Surface *surface;
	if (bufferedDraw) {
		surface = pixmapSelMargin;
	} else {
		surface = surfWindow;
	}

	// Clip vertically to paint area to avoid drawing line numbers
	if (rcMargin.bottom > rc.bottom)
		rcMargin.bottom = rc.bottom;
	if (rcMargin.top < rc.top)
		rcMargin.top = rc.top;

	PRectangle rcSelMargin = rcMargin;
	rcSelMargin.right = rcMargin.left;
	if (rcSelMargin.bottom < rc.bottom)
		rcSelMargin.bottom = rc.bottom;

	for (int margin = 0; margin <= SC_MAX_MARGIN; margin++) {
		if (vs.ms[margin].width > 0) {

			rcSelMargin.left = rcSelMargin.right;
			rcSelMargin.right = rcSelMargin.left + vs.ms[margin].width;

			if (vs.ms[margin].style != SC_MARGIN_NUMBER) {
				if (vs.ms[margin].mask & SC_MASK_FOLDERS) {
					// Required because of special way brush is created for selection margin
					// Ensure patterns line up when scrolling with separate margin view
					// by choosing correctly aligned variant.
					bool invertPhase = static_cast<int>(ptOrigin.y) & 1;
					surface->FillRectangle(rcSelMargin,
					  invertPhase ? *pixmapSelPattern : *pixmapSelPatternOffset1);
				} else {
					ColourDesired colour;
					switch (vs.ms[margin].style) {
					case SC_MARGIN_BACK:
						colour = vs.styles[STYLE_DEFAULT].back;
						break;
					case SC_MARGIN_FORE:
						colour = vs.styles[STYLE_DEFAULT].fore;
						break;
					default:
						colour = vs.styles[STYLE_LINENUMBER].back;
						break;
					}
					surface->FillRectangle(rcSelMargin, colour);
				}
			} else {
				surface->FillRectangle(rcSelMargin, vs.styles[STYLE_LINENUMBER].back);
			}

			const int lineStartPaint = (rcMargin.top + ptOrigin.y) / vs.lineHeight;
			int visibleLine = TopLineOfMain() + lineStartPaint;
			int yposScreen = lineStartPaint * vs.lineHeight - ptOrigin.y;
			// Work out whether the top line is whitespace located after a
			// lessening of fold level which implies a 'fold tail' but which should not
			// be displayed until the last of a sequence of whitespace.
			bool needWhiteClosure = false;
			if (vs.ms[margin].mask & SC_MASK_FOLDERS) {
				int level = pdoc->GetLevel(cs.DocFromDisplay(visibleLine));
				if (level & SC_FOLDLEVELWHITEFLAG) {
					int lineBack = cs.DocFromDisplay(visibleLine);
					int levelPrev = level;
					while ((lineBack > 0) && (levelPrev & SC_FOLDLEVELWHITEFLAG)) {
						lineBack--;
						levelPrev = pdoc->GetLevel(lineBack);
					}
					if (!(levelPrev & SC_FOLDLEVELHEADERFLAG)) {
						if ((level & SC_FOLDLEVELNUMBERMASK) < (levelPrev & SC_FOLDLEVELNUMBERMASK))
							needWhiteClosure = true;
					}
				}
				if (highlightDelimiter.isEnabled) {
					int lastLine = cs.DocFromDisplay(topLine + LinesOnScreen()) + 1;
					pdoc->GetHighlightDelimiters(highlightDelimiter, pdoc->LineFromPosition(CurrentPosition()), lastLine);
				}
			}

			// Old code does not know about new markers needed to distinguish all cases
			int folderOpenMid = SubstituteMarkerIfEmpty(SC_MARKNUM_FOLDEROPENMID,
			        SC_MARKNUM_FOLDEROPEN);
			int folderEnd = SubstituteMarkerIfEmpty(SC_MARKNUM_FOLDEREND,
			        SC_MARKNUM_FOLDER);

			while ((visibleLine < cs.LinesDisplayed()) && yposScreen < rc.bottom) {

				PLATFORM_ASSERT(visibleLine < cs.LinesDisplayed());
				int lineDoc = cs.DocFromDisplay(visibleLine);
				PLATFORM_ASSERT(cs.GetVisible(lineDoc));
				bool firstSubLine = visibleLine == cs.DisplayFromDoc(lineDoc);
				bool lastSubLine = visibleLine == (cs.DisplayFromDoc(lineDoc + 1) - 1);

				int marks = pdoc->GetMark(lineDoc);
				if (!firstSubLine)
					marks = 0;

				bool headWithTail = false;

				if (vs.ms[margin].mask & SC_MASK_FOLDERS) {
					// Decide which fold indicator should be displayed
					int level = pdoc->GetLevel(lineDoc);
					int levelNext = pdoc->GetLevel(lineDoc + 1);
					int levelNum = level & SC_FOLDLEVELNUMBERMASK;
					int levelNextNum = levelNext & SC_FOLDLEVELNUMBERMASK;
					if (level & SC_FOLDLEVELHEADERFLAG) {
						if (firstSubLine) {
							if (levelNum < levelNextNum) {
								if (cs.GetExpanded(lineDoc)) {
									if (levelNum == SC_FOLDLEVELBASE)
										marks |= 1 << SC_MARKNUM_FOLDEROPEN;
									else
										marks |= 1 << folderOpenMid;
								} else {
									if (levelNum == SC_FOLDLEVELBASE)
										marks |= 1 << SC_MARKNUM_FOLDER;
									else
										marks |= 1 << folderEnd;
								}
							} else if (levelNum > SC_FOLDLEVELBASE) {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
 							}
						} else {
							if (levelNum < levelNextNum) {
								if (cs.GetExpanded(lineDoc)) {
									marks |= 1 << SC_MARKNUM_FOLDERSUB;
								} else if (levelNum > SC_FOLDLEVELBASE) {
									marks |= 1 << SC_MARKNUM_FOLDERSUB;
								}
							} else if (levelNum > SC_FOLDLEVELBASE) {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
 							}
						}
						needWhiteClosure = false;
						int firstFollowupLine = cs.DocFromDisplay(cs.DisplayFromDoc(lineDoc + 1));
						int firstFollowupLineLevel = pdoc->GetLevel(firstFollowupLine);
						int secondFollowupLineLevelNum = pdoc->GetLevel(firstFollowupLine + 1) & SC_FOLDLEVELNUMBERMASK;
						if (!cs.GetExpanded(lineDoc)) {
							if ((firstFollowupLineLevel & SC_FOLDLEVELWHITEFLAG) &&
								(levelNum > secondFollowupLineLevelNum))
								needWhiteClosure = true;

							if (highlightDelimiter.IsFoldBlockHighlighted(firstFollowupLine))
								headWithTail = true;
						}
					} else if (level & SC_FOLDLEVELWHITEFLAG) {
						if (needWhiteClosure) {
							if (levelNext & SC_FOLDLEVELWHITEFLAG) {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
							} else if (levelNextNum > SC_FOLDLEVELBASE) {
								marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
								needWhiteClosure = false;
							} else {
								marks |= 1 << SC_MARKNUM_FOLDERTAIL;
								needWhiteClosure = false;
							}
						} else if (levelNum > SC_FOLDLEVELBASE) {
							if (levelNextNum < levelNum) {
								if (levelNextNum > SC_FOLDLEVELBASE) {
									marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
								} else {
									marks |= 1 << SC_MARKNUM_FOLDERTAIL;
								}
							} else {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
							}
						}
					} else if (levelNum > SC_FOLDLEVELBASE) {
						if (levelNextNum < levelNum) {
							needWhiteClosure = false;
							if (levelNext & SC_FOLDLEVELWHITEFLAG) {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
								needWhiteClosure = true;
							} else if (lastSubLine) {
								if (levelNextNum > SC_FOLDLEVELBASE) {
									marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
								} else {
									marks |= 1 << SC_MARKNUM_FOLDERTAIL;
								}
							} else {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
							}
						} else {
							marks |= 1 << SC_MARKNUM_FOLDERSUB;
						}
					}
				}

				marks &= vs.ms[margin].mask;

				PRectangle rcMarker = rcSelMargin;
				rcMarker.top = yposScreen;
				rcMarker.bottom = yposScreen + vs.lineHeight;
				if (vs.ms[margin].style == SC_MARGIN_NUMBER) {
					if (firstSubLine) {
						char number[100] = "";
						if (lineDoc >= 0)
							sprintf(number, "%d", lineDoc + 1);
						if (foldFlags & SC_FOLDFLAG_LEVELNUMBERS) {
							int lev = pdoc->GetLevel(lineDoc);
							sprintf(number, "%c%c %03X %03X",
									(lev & SC_FOLDLEVELHEADERFLAG) ? 'H' : '_',
									(lev & SC_FOLDLEVELWHITEFLAG) ? 'W' : '_',
									lev & SC_FOLDLEVELNUMBERMASK,
									lev >> 16
								   );
						}
						PRectangle rcNumber = rcMarker;
						// Right justify
						XYPOSITION width = surface->WidthText(vs.styles[STYLE_LINENUMBER].font, number, istrlen(number));
						XYPOSITION xpos = rcNumber.right - width - marginNumberPadding;
						rcNumber.left = xpos;
						surface->DrawTextNoClip(rcNumber, vs.styles[STYLE_LINENUMBER].font,
								rcNumber.top + vs.maxAscent, number, istrlen(number),
								vs.styles[STYLE_LINENUMBER].fore,
								vs.styles[STYLE_LINENUMBER].back);
					} else if (wrapVisualFlags & SC_WRAPVISUALFLAG_MARGIN) {
						PRectangle rcWrapMarker = rcMarker;
						rcWrapMarker.right -= 3;
						rcWrapMarker.left = rcWrapMarker.right - vs.styles[STYLE_LINENUMBER].aveCharWidth;
						DrawWrapMarker(surface, rcWrapMarker, false, vs.styles[STYLE_LINENUMBER].fore);
					}
				} else if (vs.ms[margin].style == SC_MARGIN_TEXT || vs.ms[margin].style == SC_MARGIN_RTEXT) {
					if (firstSubLine) {
						const StyledText stMargin  = pdoc->MarginStyledText(lineDoc);
						if (stMargin.text && ValidStyledText(vs, vs.marginStyleOffset, stMargin)) {
							surface->FillRectangle(rcMarker,
								vs.styles[stMargin.StyleAt(0)+vs.marginStyleOffset].back);
							if (vs.ms[margin].style == SC_MARGIN_RTEXT) {
								int width = WidestLineWidth(surface, vs, vs.marginStyleOffset, stMargin);
								rcMarker.left = rcMarker.right - width - 3;
							}
							DrawStyledText(surface, vs, vs.marginStyleOffset, rcMarker, rcMarker.top + vs.maxAscent,
								stMargin, 0, stMargin.length);
						}
					}
				}

				if (marks) {
					for (int markBit = 0; (markBit < 32) && marks; markBit++) {
						if (marks & 1) {
							LineMarker::typeOfFold tFold = LineMarker::undefined;
							if ((vs.ms[margin].mask & SC_MASK_FOLDERS) && highlightDelimiter.IsFoldBlockHighlighted(lineDoc)) {
								if (highlightDelimiter.IsBodyOfFoldBlock(lineDoc)) {
									tFold = LineMarker::body;
								} else if (highlightDelimiter.IsHeadOfFoldBlock(lineDoc)) {
									if (firstSubLine) {
										tFold = headWithTail ? LineMarker::headWithTail : LineMarker::head;
									} else {
										if (cs.GetExpanded(lineDoc) || headWithTail) {
											tFold = LineMarker::body;
										} else {
											tFold = LineMarker::undefined;
										}
									}
								} else if (highlightDelimiter.IsTailOfFoldBlock(lineDoc)) {
									tFold = LineMarker::tail;
								}
							}
							vs.markers[markBit].Draw(surface, rcMarker, vs.styles[STYLE_LINENUMBER].font, tFold, vs.ms[margin].style);
						}
						marks >>= 1;
					}
				}

				visibleLine++;
				yposScreen += vs.lineHeight;
			}
		}
	}

	PRectangle rcBlankMargin = rcMargin;
	rcBlankMargin.left = rcSelMargin.right;
	surface->FillRectangle(rcBlankMargin, vs.styles[STYLE_DEFAULT].back);

	if (bufferedDraw) {
		surfWindow->Copy(rcMargin, Point(rcMargin.left, rcMargin.top), *pixmapSelMargin);
	}
}

void DrawTabArrow(Surface *surface, PRectangle rcTab, int ymid) {
	int ydiff = (rcTab.bottom - rcTab.top) / 2;
	int xhead = rcTab.right - 1 - ydiff;
	if (xhead <= rcTab.left) {
		ydiff -= rcTab.left - xhead - 1;
		xhead = rcTab.left - 1;
	}
	if ((rcTab.left + 2) < (rcTab.right - 1))
		surface->MoveTo(rcTab.left + 2, ymid);
	else
		surface->MoveTo(rcTab.right - 1, ymid);
	surface->LineTo(rcTab.right - 1, ymid);
	surface->LineTo(xhead, ymid - ydiff);
	surface->MoveTo(rcTab.right - 1, ymid);
	surface->LineTo(xhead, ymid + ydiff);
}

LineLayout *Editor::RetrieveLineLayout(int lineNumber) {
	int posLineStart = pdoc->LineStart(lineNumber);
	int posLineEnd = pdoc->LineStart(lineNumber + 1);
	PLATFORM_ASSERT(posLineEnd >= posLineStart);
	int lineCaret = pdoc->LineFromPosition(sel.MainCaret());
	return llc.Retrieve(lineNumber, lineCaret,
	        posLineEnd - posLineStart, pdoc->GetStyleClock(),
	        LinesOnScreen() + 1, pdoc->LinesTotal());
}

bool BadUTF(const char *s, int len, int &trailBytes) {
	// For the rules: http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
	if (trailBytes) {
		trailBytes--;
		return false;
	}
	int utf8status = UTF8Classify(reinterpret_cast<const unsigned char *>(s), len);
	if (utf8status & UTF8MaskInvalid) {
		return true;
	} else {
		trailBytes = (utf8status & UTF8MaskWidth) - 1;
		return false;
	}
}

/**
 * Fill in the LineLayout data for the given line.
 * Copy the given @a line and its styles from the document into local arrays.
 * Also determine the x position at which each character starts.
 */
void Editor::LayoutLine(int line, Surface *surface, ViewStyle &vstyle, LineLayout *ll, int width) {
	if (!ll)
		return;

	PLATFORM_ASSERT(line < pdoc->LinesTotal());
	PLATFORM_ASSERT(ll->chars != NULL);
	int posLineStart = pdoc->LineStart(line);
	int posLineEnd = pdoc->LineStart(line + 1);
	// If the line is very long, limit the treatment to a length that should fit in the viewport
	if (posLineEnd > (posLineStart + ll->maxLineLength)) {
		posLineEnd = posLineStart + ll->maxLineLength;
	}
	if (ll->validity == LineLayout::llCheckTextAndStyle) {
		int lineLength = posLineEnd - posLineStart;
		if (!vstyle.viewEOL) {
			lineLength = pdoc->LineEnd(line) - posLineStart;
		}
		if (lineLength == ll->numCharsInLine) {
			// See if chars, styles, indicators, are all the same
			bool allSame = true;
			const int styleMask = pdoc->stylingBitsMask;
			// Check base line layout
			char styleByte = 0;
			int numCharsInLine = 0;
			while (numCharsInLine < lineLength) {
				int charInDoc = numCharsInLine + posLineStart;
				char chDoc = pdoc->CharAt(charInDoc);
				styleByte = pdoc->StyleAt(charInDoc);
				allSame = allSame &&
				        (ll->styles[numCharsInLine] == static_cast<unsigned char>(styleByte & styleMask));
				allSame = allSame &&
				        (ll->indicators[numCharsInLine] == static_cast<char>(styleByte & ~styleMask));
				if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseMixed)
					allSame = allSame &&
					        (ll->chars[numCharsInLine] == chDoc);
				else if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseLower)
					allSame = allSame &&
					        (ll->chars[numCharsInLine] == static_cast<char>(tolower(chDoc)));
				else	// Style::caseUpper
					allSame = allSame &&
					        (ll->chars[numCharsInLine] == static_cast<char>(toupper(chDoc)));
				numCharsInLine++;
			}
			allSame = allSame && (ll->styles[numCharsInLine] == styleByte);	// For eolFilled
			if (allSame) {
				ll->validity = LineLayout::llPositions;
			} else {
				ll->validity = LineLayout::llInvalid;
			}
		} else {
			ll->validity = LineLayout::llInvalid;
		}
	}
	if (ll->validity == LineLayout::llInvalid) {
		ll->widthLine = LineLayout::wrapWidthInfinite;
		ll->lines = 1;
		if (vstyle.edgeState == EDGE_BACKGROUND) {
			ll->edgeColumn = pdoc->FindColumn(line, theEdge);
			if (ll->edgeColumn >= posLineStart) {
				ll->edgeColumn -= posLineStart;
			}
		} else {
			ll->edgeColumn = -1;
		}

		char styleByte;
		const int styleMask = pdoc->stylingBitsMask;
		ll->styleBitsSet = 0;
		// Fill base line layout
		const int lineLength = posLineEnd - posLineStart;
		pdoc->GetCharRange(ll->chars, posLineStart, lineLength);
		pdoc->GetStyleRange(ll->styles, posLineStart, lineLength);
		int numCharsBeforeEOL = pdoc->LineEnd(line) - posLineStart;
		const int numCharsInLine = (vstyle.viewEOL) ? lineLength : numCharsBeforeEOL;
		for (int styleInLine = 0; styleInLine < numCharsInLine; styleInLine++) {
			styleByte = ll->styles[styleInLine];
			ll->styleBitsSet |= styleByte;
			ll->styles[styleInLine] = static_cast<char>(styleByte & styleMask);
			ll->indicators[styleInLine] = static_cast<char>(styleByte & ~styleMask);
		}
		styleByte = static_cast<char>(((lineLength > 0) ? ll->styles[lineLength-1] : 0) & styleMask);
		if (vstyle.someStylesForceCase) {
			for (int charInLine = 0; charInLine<lineLength; charInLine++) {
				char chDoc = ll->chars[charInLine];
				if (vstyle.styles[ll->styles[charInLine]].caseForce == Style::caseUpper)
					ll->chars[charInLine] = static_cast<char>(toupper(chDoc));
				else if (vstyle.styles[ll->styles[charInLine]].caseForce == Style::caseLower)
					ll->chars[charInLine] = static_cast<char>(tolower(chDoc));
			}
		}
		ll->xHighlightGuide = 0;
		// Extra element at the end of the line to hold end x position and act as
		ll->chars[numCharsInLine] = 0;   // Also triggers processing in the loops as this is a control character
		ll->styles[numCharsInLine] = styleByte;	// For eolFilled
		ll->indicators[numCharsInLine] = 0;

		// Layout the line, determining the position of each character,
		// with an extra element at the end for the end of the line.
		int startseg = 0;	// Start of the current segment, in char. number
		XYACCUMULATOR startsegx = 0;	// Start of the current segment, in pixels
		ll->positions[0] = 0;
		XYPOSITION tabWidth = vstyle.spaceWidth * pdoc->tabInChars;
		bool lastSegItalics = false;
		Font &ctrlCharsFont = vstyle.styles[STYLE_CONTROLCHAR].font;

		XYPOSITION ctrlCharWidth[32] = {0};
		bool isControlNext = IsControlCharacter(ll->chars[0]);
		int trailBytes = 0;
		bool isBadUTFNext = IsUnicodeMode() && BadUTF(ll->chars, numCharsInLine, trailBytes);
		for (int charInLine = 0; charInLine < numCharsInLine; charInLine++) {
			bool isControl = isControlNext;
			isControlNext = IsControlCharacter(ll->chars[charInLine + 1]);
			bool isBadUTF = isBadUTFNext;
			isBadUTFNext = IsUnicodeMode() && BadUTF(ll->chars + charInLine + 1, numCharsInLine - charInLine - 1, trailBytes);
			if ((ll->styles[charInLine] != ll->styles[charInLine + 1]) ||
			        isControl || isControlNext || isBadUTF || isBadUTFNext || ((charInLine+1) >= numCharsBeforeEOL)) {
				ll->positions[startseg] = 0;
				if (vstyle.styles[ll->styles[charInLine]].visible) {
					if (isControl) {
						if (ll->chars[charInLine] == '\t') {
							ll->positions[charInLine + 1] =
								((static_cast<int>((startsegx + 2) / tabWidth) + 1) * tabWidth) - startsegx;
						} else if (controlCharSymbol < 32) {
							if (ctrlCharWidth[ll->chars[charInLine]] == 0) {
								const char *ctrlChar = ControlCharacterString(ll->chars[charInLine]);
								ctrlCharWidth[ll->chars[charInLine]] =
								    surface->WidthText(ctrlCharsFont, ctrlChar, istrlen(ctrlChar)) + ctrlCharPadding;
							}
							ll->positions[charInLine + 1] = ctrlCharWidth[ll->chars[charInLine]];
						} else {
							char cc[2] = { static_cast<char>(controlCharSymbol), '\0' };
							surface->MeasureWidths(ctrlCharsFont, cc, 1,
							        ll->positions + startseg + 1);
						}
						lastSegItalics = false;
					} else if ((isBadUTF) || (charInLine >= numCharsBeforeEOL)) {
						char hexits[4];
						sprintf(hexits, "x%2X", ll->chars[charInLine] & 0xff);
						ll->positions[charInLine + 1] =
						    surface->WidthText(ctrlCharsFont, hexits, istrlen(hexits)) + 3;
					} else {	// Regular character
						int lenSeg = charInLine - startseg + 1;
						if ((lenSeg == 1) && (' ' == ll->chars[startseg])) {
							lastSegItalics = false;
							// Over half the segments are single characters and of these about half are space characters.
							ll->positions[charInLine + 1] = vstyle.styles[ll->styles[charInLine]].spaceWidth;
						} else {
							lastSegItalics = vstyle.styles[ll->styles[charInLine]].italic;
							posCache.MeasureWidths(surface, vstyle, ll->styles[charInLine], ll->chars + startseg,
							        lenSeg, ll->positions + startseg + 1, pdoc);
						}
					}
				} else {    // invisible
					for (int posToZero = startseg; posToZero <= (charInLine + 1); posToZero++) {
						ll->positions[posToZero] = 0;
					}
				}
				for (int posToIncrease = startseg; posToIncrease <= (charInLine + 1); posToIncrease++) {
					ll->positions[posToIncrease] += startsegx;
				}
				startsegx = ll->positions[charInLine + 1];
				startseg = charInLine + 1;
			}
		}
		// Small hack to make lines that end with italics not cut off the edge of the last character
		if ((startseg > 0) && lastSegItalics) {
			ll->positions[startseg] += lastSegItalicsOffset;
		}
		ll->numCharsInLine = numCharsInLine;
		ll->numCharsBeforeEOL = numCharsBeforeEOL;
		ll->validity = LineLayout::llPositions;
	}
	// Hard to cope when too narrow, so just assume there is space
	if (width < 20) {
		width = 20;
	}
	if ((ll->validity == LineLayout::llPositions) || (ll->widthLine != width)) {
		ll->widthLine = width;
		if (width == LineLayout::wrapWidthInfinite) {
			ll->lines = 1;
		} else if (width > ll->positions[ll->numCharsInLine]) {
			// Simple common case where line does not need wrapping.
			ll->lines = 1;
		} else {
			if (wrapVisualFlags & SC_WRAPVISUALFLAG_END) {
				width -= static_cast<int>(vstyle.aveCharWidth); // take into account the space for end wrap mark
			}
			XYPOSITION wrapAddIndent = 0; // This will be added to initial indent of line
			if (wrapIndentMode == SC_WRAPINDENT_INDENT) {
				wrapAddIndent = pdoc->IndentSize() * vstyle.spaceWidth;
			} else if (wrapIndentMode == SC_WRAPINDENT_FIXED) {
				wrapAddIndent = wrapVisualStartIndent * vstyle.aveCharWidth;
			}
			ll->wrapIndent = wrapAddIndent;
			if (wrapIndentMode != SC_WRAPINDENT_FIXED)
				for (int i = 0; i < ll->numCharsInLine; i++) {
					if (!IsSpaceOrTab(ll->chars[i])) {
						ll->wrapIndent += ll->positions[i]; // Add line indent
						break;
					}
				}
			// Check for text width minimum
			if (ll->wrapIndent > width - static_cast<int>(vstyle.aveCharWidth) * 15)
				ll->wrapIndent = wrapAddIndent;
			// Check for wrapIndent minimum
			if ((wrapVisualFlags & SC_WRAPVISUALFLAG_START) && (ll->wrapIndent < vstyle.aveCharWidth))
				ll->wrapIndent = vstyle.aveCharWidth; // Indent to show start visual
			ll->lines = 0;
			// Calculate line start positions based upon width.
			int lastGoodBreak = 0;
			int lastLineStart = 0;
			XYACCUMULATOR startOffset = 0;
			int p = 0;
			while (p < ll->numCharsInLine) {
				if ((ll->positions[p + 1] - startOffset) >= width) {
					if (lastGoodBreak == lastLineStart) {
						// Try moving to start of last character
						if (p > 0) {
							lastGoodBreak = pdoc->MovePositionOutsideChar(p + posLineStart, -1)
							        - posLineStart;
						}
						if (lastGoodBreak == lastLineStart) {
							// Ensure at least one character on line.
							lastGoodBreak = pdoc->MovePositionOutsideChar(lastGoodBreak + posLineStart + 1, 1)
							        - posLineStart;
						}
					}
					lastLineStart = lastGoodBreak;
					ll->lines++;
					ll->SetLineStart(ll->lines, lastGoodBreak);
					startOffset = ll->positions[lastGoodBreak];
					// take into account the space for start wrap mark and indent
					startOffset -= ll->wrapIndent;
					p = lastGoodBreak + 1;
					continue;
				}
				if (p > 0) {
					if (wrapState == eWrapChar) {
						lastGoodBreak = pdoc->MovePositionOutsideChar(p + posLineStart, -1)
						        - posLineStart;
						p = pdoc->MovePositionOutsideChar(p + 1 + posLineStart, 1) - posLineStart;
						continue;
					} else if (ll->styles[p] != ll->styles[p - 1]) {
						lastGoodBreak = p;
					} else if (IsSpaceOrTab(ll->chars[p - 1]) && !IsSpaceOrTab(ll->chars[p])) {
						lastGoodBreak = p;
					}
				}
				p++;
			}
			ll->lines++;
		}
		ll->validity = LineLayout::llLines;
	}
}

ColourDesired Editor::SelectionBackground(ViewStyle &vsDraw, bool main) const {
	return main ?
		(primarySelection ? vsDraw.selbackground : vsDraw.selbackground2) :
		vsDraw.selAdditionalBackground;
}

ColourDesired Editor::TextBackground(ViewStyle &vsDraw, bool overrideBackground,
        ColourDesired background, int inSelection, bool inHotspot, int styleMain, int i, LineLayout *ll) const {
	if (inSelection == 1) {
		if (vsDraw.selbackset && (vsDraw.selAlpha == SC_ALPHA_NOALPHA)) {
			return SelectionBackground(vsDraw, true);
		}
	} else if (inSelection == 2) {
		if (vsDraw.selbackset && (vsDraw.selAdditionalAlpha == SC_ALPHA_NOALPHA)) {
			return SelectionBackground(vsDraw, false);
		}
	} else {
		if ((vsDraw.edgeState == EDGE_BACKGROUND) &&
		        (i >= ll->edgeColumn) &&
		        (i < ll->numCharsBeforeEOL))
			return vsDraw.edgecolour;
		if (inHotspot && vsDraw.hotspotBackgroundSet)
			return vsDraw.hotspotBackground;
	}
	if (overrideBackground && (styleMain != STYLE_BRACELIGHT) && (styleMain != STYLE_BRACEBAD)) {
		return background;
	} else {
		return vsDraw.styles[styleMain].back;
	}
}

void Editor::DrawIndentGuide(Surface *surface, int lineVisible, int lineHeight, int start, PRectangle rcSegment, bool highlight) {
	Point from(0, ((lineVisible & 1) && (lineHeight & 1)) ? 1 : 0);
	PRectangle rcCopyArea(start + 1, rcSegment.top, start + 2, rcSegment.bottom);
	surface->Copy(rcCopyArea, from,
	        highlight ? *pixmapIndentGuideHighlight : *pixmapIndentGuide);
}

void Editor::DrawWrapMarker(Surface *surface, PRectangle rcPlace,
        bool isEndMarker, ColourDesired wrapColour) {
	surface->PenColour(wrapColour);

	enum { xa = 1 }; // gap before start
	int w = rcPlace.right - rcPlace.left - xa - 1;

	bool xStraight = isEndMarker;  // x-mirrored symbol for start marker
	bool yStraight = true;
	//bool yStraight= isEndMarker; // comment in for start marker y-mirrowed

	int x0 = xStraight ? rcPlace.left : rcPlace.right - 1;
	int y0 = yStraight ? rcPlace.top : rcPlace.bottom - 1;

	int dy = (rcPlace.bottom - rcPlace.top) / 5;
	int y = (rcPlace.bottom - rcPlace.top) / 2 + dy;

	struct Relative {
		Surface *surface;
		int xBase;
		int xDir;
		int yBase;
		int yDir;
		void MoveTo(int xRelative, int yRelative) {
			surface->MoveTo(xBase + xDir * xRelative, yBase + yDir * yRelative);
		}
		void LineTo(int xRelative, int yRelative) {
			surface->LineTo(xBase + xDir * xRelative, yBase + yDir * yRelative);
		}
	};
	Relative rel = {surface, x0, xStraight ? 1 : -1, y0, yStraight ? 1 : -1};

	// arrow head
	rel.MoveTo(xa, y);
	rel.LineTo(xa + 2*w / 3, y - dy);
	rel.MoveTo(xa, y);
	rel.LineTo(xa + 2*w / 3, y + dy);

	// arrow body
	rel.MoveTo(xa, y);
	rel.LineTo(xa + w, y);
	rel.LineTo(xa + w, y - 2 * dy);
	rel.LineTo(xa - 1,   // on windows lineto is exclusive endpoint, perhaps GTK not...
	        y - 2 * dy);
}

static void SimpleAlphaRectangle(Surface *surface, PRectangle rc, ColourDesired fill, int alpha) {
	if (alpha != SC_ALPHA_NOALPHA) {
		surface->AlphaRectangle(rc, 0, fill, alpha, fill, alpha, 0);
	}
}

void DrawTextBlob(Surface *surface, ViewStyle &vsDraw, PRectangle rcSegment,
				  const char *s, ColourDesired textBack, ColourDesired textFore, bool twoPhaseDraw) {
	if (!twoPhaseDraw) {
		surface->FillRectangle(rcSegment, textBack);
	}
	Font &ctrlCharsFont = vsDraw.styles[STYLE_CONTROLCHAR].font;
	int normalCharHeight = surface->Ascent(ctrlCharsFont) -
	        surface->InternalLeading(ctrlCharsFont);
	PRectangle rcCChar = rcSegment;
	rcCChar.left = rcCChar.left + 1;
	rcCChar.top = rcSegment.top + vsDraw.maxAscent - normalCharHeight;
	rcCChar.bottom = rcSegment.top + vsDraw.maxAscent + 1;
	PRectangle rcCentral = rcCChar;
	rcCentral.top++;
	rcCentral.bottom--;
	surface->FillRectangle(rcCentral, textFore);
	PRectangle rcChar = rcCChar;
	rcChar.left++;
	rcChar.right--;
	surface->DrawTextClipped(rcChar, ctrlCharsFont,
	        rcSegment.top + vsDraw.maxAscent, s, istrlen(s),
	        textBack, textFore);
}

void Editor::DrawEOL(Surface *surface, ViewStyle &vsDraw, PRectangle rcLine, LineLayout *ll,
        int line, int lineEnd, int xStart, int subLine, XYACCUMULATOR subLineStart,
        bool overrideBackground, ColourDesired background,
        bool drawWrapMarkEnd, ColourDesired wrapColour) {

	const int posLineStart = pdoc->LineStart(line);
	const int styleMask = pdoc->stylingBitsMask;
	PRectangle rcSegment = rcLine;

	const bool lastSubLine = subLine == (ll->lines - 1);
	XYPOSITION virtualSpace = 0;
	if (lastSubLine) {
		const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
		virtualSpace = sel.VirtualSpaceFor(pdoc->LineEnd(line)) * spaceWidth;
	}
	XYPOSITION xEol = ll->positions[lineEnd] - subLineStart;

	// Fill the virtual space and show selections within it
	if (virtualSpace) {
		rcSegment.left = xEol + xStart;
		rcSegment.right = xEol + xStart + virtualSpace;
		surface->FillRectangle(rcSegment, overrideBackground ? background : vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].back);
		if (!hideSelection && ((vsDraw.selAlpha == SC_ALPHA_NOALPHA) || (vsDraw.selAdditionalAlpha == SC_ALPHA_NOALPHA))) {
			SelectionSegment virtualSpaceRange(SelectionPosition(pdoc->LineEnd(line)), SelectionPosition(pdoc->LineEnd(line), sel.VirtualSpaceFor(pdoc->LineEnd(line))));
			for (size_t r=0; r<sel.Count(); r++) {
				int alpha = (r == sel.Main()) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
				if (alpha == SC_ALPHA_NOALPHA) {
					SelectionSegment portion = sel.Range(r).Intersect(virtualSpaceRange);
					if (!portion.Empty()) {
						const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
						rcSegment.left = xStart + ll->positions[portion.start.Position() - posLineStart] - subLineStart + portion.start.VirtualSpace() * spaceWidth;
						rcSegment.right = xStart + ll->positions[portion.end.Position() - posLineStart] - subLineStart + portion.end.VirtualSpace() * spaceWidth;
						rcSegment.left = (rcSegment.left > rcLine.left) ? rcSegment.left : rcLine.left;
						rcSegment.right = (rcSegment.right < rcLine.right) ? rcSegment.right : rcLine.right;
						surface->FillRectangle(rcSegment, SelectionBackground(vsDraw, r == sel.Main()));
					}
				}
			}
		}
	}

	int eolInSelection = 0;
	int alpha = SC_ALPHA_NOALPHA;
	if (!hideSelection) {
		int posAfterLineEnd = pdoc->LineStart(line + 1);
		eolInSelection = (subLine == (ll->lines - 1)) ? sel.InSelectionForEOL(posAfterLineEnd) : 0;
		alpha = (eolInSelection == 1) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
	}

	// Draw the [CR], [LF], or [CR][LF] blobs if visible line ends are on
	XYPOSITION blobsWidth = 0;
	if (lastSubLine) {
		for (int eolPos=ll->numCharsBeforeEOL; eolPos<ll->numCharsInLine; eolPos++) {
			rcSegment.left = xStart + ll->positions[eolPos] - subLineStart + virtualSpace;
			rcSegment.right = xStart + ll->positions[eolPos+1] - subLineStart + virtualSpace;
			blobsWidth += rcSegment.Width();
			char hexits[4];
			const char *ctrlChar;
			unsigned char chEOL = ll->chars[eolPos];
			if (UTF8IsAscii(chEOL)) {
				ctrlChar = ControlCharacterString(chEOL);
			} else {
				sprintf(hexits, "x%2X", chEOL);
				ctrlChar = hexits;
			}
			int styleMain = ll->styles[eolPos];
			ColourDesired textBack = TextBackground(vsDraw, overrideBackground, background, eolInSelection, false, styleMain, eolPos, ll);
			ColourDesired textFore = vsDraw.styles[styleMain].fore;
			if (eolInSelection && vsDraw.selforeset) {
				textFore = (eolInSelection == 1) ? vsDraw.selforeground : vsDraw.selAdditionalForeground;
			}
			if (eolInSelection && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1)) {
				if (alpha == SC_ALPHA_NOALPHA) {
					surface->FillRectangle(rcSegment, SelectionBackground(vsDraw, eolInSelection == 1));
				} else {
					surface->FillRectangle(rcSegment, textBack);
				}
			} else {
				surface->FillRectangle(rcSegment, textBack);
			}
			DrawTextBlob(surface, vsDraw, rcSegment, ctrlChar, textBack, textFore, twoPhaseDraw);
			if (eolInSelection && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (alpha != SC_ALPHA_NOALPHA)) {
				SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, eolInSelection == 1), alpha);
			}
		}
	}

	// Draw the eol-is-selected rectangle
	rcSegment.left = xEol + xStart + virtualSpace + blobsWidth;
	rcSegment.right = rcSegment.left + vsDraw.aveCharWidth;

	if (eolInSelection && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (alpha == SC_ALPHA_NOALPHA)) {
		surface->FillRectangle(rcSegment, SelectionBackground(vsDraw, eolInSelection == 1));
	} else {
		if (overrideBackground) {
			surface->FillRectangle(rcSegment, background);
		} else if (line < pdoc->LinesTotal() - 1) {
			surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].back);
		} else if (vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].eolFilled) {
			surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].back);
		} else {
			surface->FillRectangle(rcSegment, vsDraw.styles[STYLE_DEFAULT].back);
		}
		if (eolInSelection && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (alpha != SC_ALPHA_NOALPHA)) {
			SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, eolInSelection == 1), alpha);
		}
	}

	// Fill the remainder of the line
	rcSegment.left = rcSegment.right;
	if (rcSegment.left < rcLine.left)
		rcSegment.left = rcLine.left;
	rcSegment.right = rcLine.right;

	if (eolInSelection && vsDraw.selEOLFilled && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (alpha == SC_ALPHA_NOALPHA)) {
		surface->FillRectangle(rcSegment, SelectionBackground(vsDraw, eolInSelection == 1));
	} else {
		if (overrideBackground) {
			surface->FillRectangle(rcSegment, background);
		} else if (vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].eolFilled) {
			surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].back);
		} else {
			surface->FillRectangle(rcSegment, vsDraw.styles[STYLE_DEFAULT].back);
		}
		if (eolInSelection && vsDraw.selEOLFilled && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (alpha != SC_ALPHA_NOALPHA)) {
			SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, eolInSelection == 1), alpha);
		}
	}

	if (drawWrapMarkEnd) {
		PRectangle rcPlace = rcSegment;

		if (wrapVisualFlagsLocation & SC_WRAPVISUALFLAGLOC_END_BY_TEXT) {
			rcPlace.left = xEol + xStart + virtualSpace;
			rcPlace.right = rcPlace.left + vsDraw.aveCharWidth;
		} else {
			// rcLine is clipped to text area
			rcPlace.right = rcLine.right;
			rcPlace.left = rcPlace.right - vsDraw.aveCharWidth;
		}
		DrawWrapMarker(surface, rcPlace, true, wrapColour);
	}
}

void Editor::DrawIndicator(int indicNum, int startPos, int endPos, Surface *surface, ViewStyle &vsDraw,
		int xStart, PRectangle rcLine, LineLayout *ll, int subLine) {
	const XYPOSITION subLineStart = ll->positions[ll->LineStart(subLine)];
	PRectangle rcIndic(
		ll->positions[startPos] + xStart - subLineStart,
		rcLine.top + vsDraw.maxAscent,
		ll->positions[endPos] + xStart - subLineStart,
		rcLine.top + vsDraw.maxAscent + 3);
	vsDraw.indicators[indicNum].Draw(surface, rcIndic, rcLine);
}

void Editor::DrawIndicators(Surface *surface, ViewStyle &vsDraw, int line, int xStart,
        PRectangle rcLine, LineLayout *ll, int subLine, int lineEnd, bool under) {
	// Draw decorators
	const int posLineStart = pdoc->LineStart(line);
	const int lineStart = ll->LineStart(subLine);
	const int posLineEnd = posLineStart + lineEnd;

	if (!under) {
		// Draw indicators
		// foreach indicator...
		for (int indicnum = 0, mask = 1 << pdoc->stylingBits; mask < 0x100; indicnum++) {
			if (!(mask & ll->styleBitsSet)) {
				mask <<= 1;
				continue;
			}
			int startPos = -1;
			// foreach style pos in line...
			for (int indicPos = lineStart; indicPos <= lineEnd; indicPos++) {
				// look for starts...
				if (startPos < 0) {
					// NOT in indicator run, looking for START
					if (indicPos < lineEnd && (ll->indicators[indicPos] & mask))
						startPos = indicPos;
				}
				// ... or ends
				if (startPos >= 0) {
					// IN indicator run, looking for END
					if (indicPos >= lineEnd || !(ll->indicators[indicPos] & mask)) {
						// AT end of indicator run, DRAW it!
						DrawIndicator(indicnum, startPos, indicPos, surface, vsDraw, xStart, rcLine, ll, subLine);
						// RESET control var
						startPos = -1;
					}
				}
			}
			mask <<= 1;
		}
	}

	for (Decoration *deco = pdoc->decorations.root; deco; deco = deco->next) {
		if (under == vsDraw.indicators[deco->indicator].under) {
			int startPos = posLineStart + lineStart;
			if (!deco->rs.ValueAt(startPos)) {
				startPos = deco->rs.EndRun(startPos);
			}
			while ((startPos < posLineEnd) && (deco->rs.ValueAt(startPos))) {
				int endPos = deco->rs.EndRun(startPos);
				if (endPos > posLineEnd)
					endPos = posLineEnd;
				DrawIndicator(deco->indicator, startPos - posLineStart, endPos - posLineStart,
					surface, vsDraw, xStart, rcLine, ll, subLine);
				startPos = deco->rs.EndRun(endPos);
			}
		}
	}

	// Use indicators to highlight matching braces
	if ((vs.braceHighlightIndicatorSet && (bracesMatchStyle == STYLE_BRACELIGHT)) ||
		(vs.braceBadLightIndicatorSet && (bracesMatchStyle == STYLE_BRACEBAD))) {
		int braceIndicator = (bracesMatchStyle == STYLE_BRACELIGHT) ? vs.braceHighlightIndicator : vs.braceBadLightIndicator;
		if (under == vsDraw.indicators[braceIndicator].under) {
			Range rangeLine(posLineStart + lineStart, posLineEnd);
			if (rangeLine.ContainsCharacter(braces[0])) {
				int braceOffset = braces[0] - posLineStart;
				if (braceOffset < ll->numCharsInLine) {
					DrawIndicator(braceIndicator, braceOffset, braceOffset + 1, surface, vsDraw, xStart, rcLine, ll, subLine);
				}
			}
			if (rangeLine.ContainsCharacter(braces[1])) {
				int braceOffset = braces[1] - posLineStart;
				if (braceOffset < ll->numCharsInLine) {
					DrawIndicator(braceIndicator, braceOffset, braceOffset + 1, surface, vsDraw, xStart, rcLine, ll, subLine);
				}
			}
		}
	}
}

void Editor::DrawAnnotation(Surface *surface, ViewStyle &vsDraw, int line, int xStart,
    PRectangle rcLine, LineLayout *ll, int subLine) {
	int indent = pdoc->GetLineIndentation(line) * vsDraw.spaceWidth;
	PRectangle rcSegment = rcLine;
	int annotationLine = subLine - ll->lines;
	const StyledText stAnnotation  = pdoc->AnnotationStyledText(line);
	if (stAnnotation.text && ValidStyledText(vsDraw, vsDraw.annotationStyleOffset, stAnnotation)) {
		surface->FillRectangle(rcSegment, vsDraw.styles[0].back);
		if (vs.annotationVisible == ANNOTATION_BOXED) {
			// Only care about calculating width if need to draw box
			int widthAnnotation = WidestLineWidth(surface, vsDraw, vsDraw.annotationStyleOffset, stAnnotation);
			widthAnnotation += vsDraw.spaceWidth * 2; // Margins
			rcSegment.left = xStart + indent;
			rcSegment.right = rcSegment.left + widthAnnotation;
		} else {
			rcSegment.left = xStart;
		}
		const int annotationLines = pdoc->AnnotationLines(line);
		size_t start = 0;
		size_t lengthAnnotation = stAnnotation.LineLength(start);
		int lineInAnnotation = 0;
		while ((lineInAnnotation < annotationLine) && (start < stAnnotation.length)) {
			start += lengthAnnotation + 1;
			lengthAnnotation = stAnnotation.LineLength(start);
			lineInAnnotation++;
		}
		PRectangle rcText = rcSegment;
		if (vs.annotationVisible == ANNOTATION_BOXED) {
			surface->FillRectangle(rcText,
				vsDraw.styles[stAnnotation.StyleAt(start) + vsDraw.annotationStyleOffset].back);
			rcText.left += vsDraw.spaceWidth;
		}
		DrawStyledText(surface, vsDraw, vsDraw.annotationStyleOffset, rcText, rcText.top + vsDraw.maxAscent,
			stAnnotation, start, lengthAnnotation);
		if (vs.annotationVisible == ANNOTATION_BOXED) {
			surface->PenColour(vsDraw.styles[vsDraw.annotationStyleOffset].fore);
			surface->MoveTo(rcSegment.left, rcSegment.top);
			surface->LineTo(rcSegment.left, rcSegment.bottom);
			surface->MoveTo(rcSegment.right, rcSegment.top);
			surface->LineTo(rcSegment.right, rcSegment.bottom);
			if (subLine == ll->lines) {
				surface->MoveTo(rcSegment.left, rcSegment.top);
				surface->LineTo(rcSegment.right, rcSegment.top);
			}
			if (subLine == ll->lines+annotationLines-1) {
				surface->MoveTo(rcSegment.left, rcSegment.bottom - 1);
				surface->LineTo(rcSegment.right, rcSegment.bottom - 1);
			}
		}
	}
}

void Editor::DrawLine(Surface *surface, ViewStyle &vsDraw, int line, int lineVisible, int xStart,
        PRectangle rcLine, LineLayout *ll, int subLine) {

	PRectangle rcSegment = rcLine;

	// Using one font for all control characters so it can be controlled independently to ensure
	// the box goes around the characters tightly. Seems to be no way to work out what height
	// is taken by an individual character - internal leading gives varying results.
	Font &ctrlCharsFont = vsDraw.styles[STYLE_CONTROLCHAR].font;

	// See if something overrides the line background color:  Either if caret is on the line
	// and background color is set for that, or if a marker is defined that forces its background
	// color onto the line, or if a marker is defined but has no selection margin in which to
	// display itself (as long as it's not an SC_MARK_EMPTY marker).  These are checked in order
	// with the earlier taking precedence.  When multiple markers cause background override,
	// the color for the highest numbered one is used.
	bool overrideBackground = false;
	ColourDesired background;
	if ((caret.active || vsDraw.alwaysShowCaretLineBackground) && vsDraw.showCaretLineBackground && (vsDraw.caretLineAlpha == SC_ALPHA_NOALPHA) && ll->containsCaret) {
		overrideBackground = true;
		background = vsDraw.caretLineBackground;
	}
	if (!overrideBackground) {
		int marks = pdoc->GetMark(line);
		for (int markBit = 0; (markBit < 32) && marks; markBit++) {
			if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_BACKGROUND) &&
			        (vsDraw.markers[markBit].alpha == SC_ALPHA_NOALPHA)) {
				background = vsDraw.markers[markBit].back;
				overrideBackground = true;
			}
			marks >>= 1;
		}
	}
	if (!overrideBackground) {
		if (vsDraw.maskInLine) {
			int marksMasked = pdoc->GetMark(line) & vsDraw.maskInLine;
			if (marksMasked) {
				for (int markBit = 0; (markBit < 32) && marksMasked; markBit++) {
					if ((marksMasked & 1) && (vsDraw.markers[markBit].markType != SC_MARK_EMPTY) &&
					        (vsDraw.markers[markBit].alpha == SC_ALPHA_NOALPHA)) {
						overrideBackground = true;
						background = vsDraw.markers[markBit].back;
					}
					marksMasked >>= 1;
				}
			}
		}
	}

	bool drawWhitespaceBackground = (vsDraw.viewWhitespace != wsInvisible) &&
	        (!overrideBackground) && (vsDraw.whitespaceBackgroundSet);

	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.
	const XYPOSITION indentWidth = pdoc->IndentSize() * vsDraw.spaceWidth;
	const XYPOSITION epsilon = 0.0001f;	// A small nudge to avoid floating point precision issues

	int posLineStart = pdoc->LineStart(line);

	int startseg = ll->LineStart(subLine);
	XYACCUMULATOR subLineStart = ll->positions[startseg];
	if (subLine >= ll->lines) {
		DrawAnnotation(surface, vsDraw, line, xStart, rcLine, ll, subLine);
		return; // No further drawing
	}
	int lineStart = 0;
	int lineEnd = 0;
	if (subLine < ll->lines) {
		lineStart = ll->LineStart(subLine);
		lineEnd = ll->LineStart(subLine + 1);
		if (subLine == ll->lines - 1) {
			lineEnd = ll->numCharsBeforeEOL;
		}
	}

	ColourDesired wrapColour = vsDraw.styles[STYLE_DEFAULT].fore;
	if (vsDraw.whitespaceForegroundSet)
		wrapColour = vsDraw.whitespaceForeground;

	bool drawWrapMarkEnd = false;

	if (wrapVisualFlags & SC_WRAPVISUALFLAG_END) {
		if (subLine + 1 < ll->lines) {
			drawWrapMarkEnd = ll->LineStart(subLine + 1) != 0;
		}
	}

	if (ll->wrapIndent != 0) {

		bool continuedWrapLine = false;
		if (subLine < ll->lines) {
			continuedWrapLine = ll->LineStart(subLine) != 0;
		}

		if (continuedWrapLine) {
			// draw continuation rect
			PRectangle rcPlace = rcSegment;

			rcPlace.left = ll->positions[startseg] + xStart - subLineStart;
			rcPlace.right = rcPlace.left + ll->wrapIndent;

			// default bgnd here..
			surface->FillRectangle(rcSegment, overrideBackground ? background :
			        vsDraw.styles[STYLE_DEFAULT].back);

			// main line style would be below but this would be inconsistent with end markers
			// also would possibly not be the style at wrap point
			//int styleMain = ll->styles[lineStart];
			//surface->FillRectangle(rcPlace, vsDraw.styles[styleMain].back);

			if (wrapVisualFlags & SC_WRAPVISUALFLAG_START) {

				if (wrapVisualFlagsLocation & SC_WRAPVISUALFLAGLOC_START_BY_TEXT)
					rcPlace.left = rcPlace.right - vsDraw.aveCharWidth;
				else
					rcPlace.right = rcPlace.left + vsDraw.aveCharWidth;

				DrawWrapMarker(surface, rcPlace, false, wrapColour);
			}

			xStart += static_cast<int>(ll->wrapIndent);
		}
	}

	bool selBackDrawn = vsDraw.selbackset &&
		((vsDraw.selAlpha == SC_ALPHA_NOALPHA) || (vsDraw.selAdditionalAlpha == SC_ALPHA_NOALPHA));

	// Does not take margin into account but not significant
	int xStartVisible = static_cast<int>(subLineStart) - xStart;

	ll->psel = &sel;

	BreakFinder bfBack(ll, lineStart, lineEnd, posLineStart, xStartVisible, selBackDrawn, pdoc);
	int next = bfBack.First();

	// Background drawing loop
	while (twoPhaseDraw && (next < lineEnd)) {

		startseg = next;
		next = bfBack.Next();
		int i = next - 1;
		int iDoc = i + posLineStart;

		rcSegment.left = ll->positions[startseg] + xStart - subLineStart;
		rcSegment.right = ll->positions[i + 1] + xStart - subLineStart;
		// Only try to draw if really visible - enhances performance by not calling environment to
		// draw strings that are completely past the right side of the window.
		if ((rcSegment.left <= rcLine.right) && (rcSegment.right >= rcLine.left)) {
			// Clip to line rectangle, since may have a huge position which will not work with some platforms
			if (rcSegment.left < rcLine.left)
				rcSegment.left = rcLine.left;
			if (rcSegment.right > rcLine.right)
				rcSegment.right = rcLine.right;

			int styleMain = ll->styles[i];
			const int inSelection = hideSelection ? 0 : sel.CharacterInSelection(iDoc);
			bool inHotspot = (ll->hsStart != -1) && (iDoc >= ll->hsStart) && (iDoc < ll->hsEnd);
			ColourDesired textBack = TextBackground(vsDraw, overrideBackground, background, inSelection, inHotspot, styleMain, i, ll);
			if (ll->chars[i] == '\t') {
				// Tab display
				if (drawWhitespaceBackground &&
				        (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways))
					textBack = vsDraw.whitespaceBackground;
				surface->FillRectangle(rcSegment, textBack);
			} else if (IsControlCharacter(ll->chars[i])) {
				// Control character display
				inIndentation = false;
				surface->FillRectangle(rcSegment, textBack);
			} else {
				// Normal text display
				surface->FillRectangle(rcSegment, textBack);
				if (vsDraw.viewWhitespace != wsInvisible ||
				        (inIndentation && vsDraw.viewIndentationGuides == ivReal)) {
					for (int cpos = 0; cpos <= i - startseg; cpos++) {
						if (ll->chars[cpos + startseg] == ' ') {
							if (drawWhitespaceBackground &&
							        (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways)) {
								PRectangle rcSpace(ll->positions[cpos + startseg] + xStart - subLineStart,
									rcSegment.top,
									ll->positions[cpos + startseg + 1] + xStart - subLineStart,
									rcSegment.bottom);
								surface->FillRectangle(rcSpace, vsDraw.whitespaceBackground);
							}
						} else {
							inIndentation = false;
						}
					}
				}
			}
		} else if (rcSegment.left > rcLine.right) {
			break;
		}
	}

	if (twoPhaseDraw) {
		DrawEOL(surface, vsDraw, rcLine, ll, line, lineEnd,
		        xStart, subLine, subLineStart, overrideBackground, background,
		        drawWrapMarkEnd, wrapColour);
	}

	DrawIndicators(surface, vsDraw, line, xStart, rcLine, ll, subLine, lineEnd, true);

	if (vsDraw.edgeState == EDGE_LINE) {
		int edgeX = theEdge * vsDraw.spaceWidth;
		rcSegment.left = edgeX + xStart;
		if ((ll->wrapIndent != 0) && (lineStart != 0))
			rcSegment.left -= ll->wrapIndent;
		rcSegment.right = rcSegment.left + 1;
		surface->FillRectangle(rcSegment, vsDraw.edgecolour);
	}

	// Draw underline mark as part of background if not transparent
	int marks = pdoc->GetMark(line);
	int markBit;
	for (markBit = 0; (markBit < 32) && marks; markBit++) {
		if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_UNDERLINE) &&
		    (vsDraw.markers[markBit].alpha == SC_ALPHA_NOALPHA)) {
			PRectangle rcUnderline = rcLine;
			rcUnderline.top = rcUnderline.bottom - 2;
			surface->FillRectangle(rcUnderline, vsDraw.markers[markBit].back);
		}
		marks >>= 1;
	}

	inIndentation = subLine == 0;	// Do not handle indentation except on first subline.
	// Foreground drawing loop
	BreakFinder bfFore(ll, lineStart, lineEnd, posLineStart, xStartVisible,
		((!twoPhaseDraw && selBackDrawn) || vsDraw.selforeset), pdoc);
	next = bfFore.First();

	while (next < lineEnd) {

		startseg = next;
		next = bfFore.Next();
		int i = next - 1;

		int iDoc = i + posLineStart;

		rcSegment.left = ll->positions[startseg] + xStart - subLineStart;
		rcSegment.right = ll->positions[i + 1] + xStart - subLineStart;
		// Only try to draw if really visible - enhances performance by not calling environment to
		// draw strings that are completely past the right side of the window.
		if ((rcSegment.left <= rcLine.right) && (rcSegment.right >= rcLine.left)) {
			int styleMain = ll->styles[i];
			ColourDesired textFore = vsDraw.styles[styleMain].fore;
			Font &textFont = vsDraw.styles[styleMain].font;
			//hotspot foreground
			if (ll->hsStart != -1 && iDoc >= ll->hsStart && iDoc < hsEnd) {
				if (vsDraw.hotspotForegroundSet)
					textFore = vsDraw.hotspotForeground;
			}
			const int inSelection = hideSelection ? 0 : sel.CharacterInSelection(iDoc);
			if (inSelection && (vsDraw.selforeset)) {
				textFore = (inSelection == 1) ? vsDraw.selforeground : vsDraw.selAdditionalForeground;
			}
			bool inHotspot = (ll->hsStart != -1) && (iDoc >= ll->hsStart) && (iDoc < ll->hsEnd);
			ColourDesired textBack = TextBackground(vsDraw, overrideBackground, background, inSelection, inHotspot, styleMain, i, ll);
			if (ll->chars[i] == '\t') {
				// Tab display
				if (!twoPhaseDraw) {
					if (drawWhitespaceBackground &&
					        (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways))
						textBack = vsDraw.whitespaceBackground;
					surface->FillRectangle(rcSegment, textBack);
				}
				if (inIndentation && vsDraw.viewIndentationGuides == ivReal) {
					for (int indentCount = (ll->positions[i] + epsilon) / indentWidth;
						indentCount <= (ll->positions[i + 1] - epsilon) / indentWidth;
						indentCount++) {
						if (indentCount > 0) {
							int xIndent = indentCount * indentWidth;
							DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcSegment,
								    (ll->xHighlightGuide == xIndent));
						}
					}
				}
				if (vsDraw.viewWhitespace != wsInvisible) {
					if (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways) {
						if (vsDraw.whitespaceForegroundSet)
							textFore = vsDraw.whitespaceForeground;
						surface->PenColour(textFore);
						PRectangle rcTab(rcSegment.left + 1, rcSegment.top + 4,
						        rcSegment.right - 1, rcSegment.bottom - vsDraw.maxDescent);
						DrawTabArrow(surface, rcTab, rcSegment.top + vsDraw.lineHeight / 2);
					}
				}
			} else if (IsControlCharacter(ll->chars[i])) {
				// Control character display
				inIndentation = false;
				if (controlCharSymbol < 32) {
					// Draw the character
					const char *ctrlChar = ControlCharacterString(ll->chars[i]);
					DrawTextBlob(surface, vsDraw, rcSegment, ctrlChar, textBack, textFore, twoPhaseDraw);
				} else {
					char cc[2] = { static_cast<char>(controlCharSymbol), '\0' };
					surface->DrawTextNoClip(rcSegment, ctrlCharsFont,
					        rcSegment.top + vsDraw.maxAscent,
					        cc, 1, textBack, textFore);
				}
			} else if ((i == startseg) && (static_cast<unsigned char>(ll->chars[i]) >= 0x80) && IsUnicodeMode()) {
				// A single byte >= 0x80 in UTF-8 is a bad byte and is displayed as its hex value
				char hexits[4];
				sprintf(hexits, "x%2X", ll->chars[i] & 0xff);
				DrawTextBlob(surface, vsDraw, rcSegment, hexits, textBack, textFore, twoPhaseDraw);
			} else {
				// Normal text display
				if (vsDraw.styles[styleMain].visible) {
					if (twoPhaseDraw) {
						surface->DrawTextTransparent(rcSegment, textFont,
						        rcSegment.top + vsDraw.maxAscent, ll->chars + startseg,
						        i - startseg + 1, textFore);
					} else {
						surface->DrawTextNoClip(rcSegment, textFont,
						        rcSegment.top + vsDraw.maxAscent, ll->chars + startseg,
						        i - startseg + 1, textFore, textBack);
					}
				}
				if (vsDraw.viewWhitespace != wsInvisible ||
				        (inIndentation && vsDraw.viewIndentationGuides != ivNone)) {
					for (int cpos = 0; cpos <= i - startseg; cpos++) {
						if (ll->chars[cpos + startseg] == ' ') {
							if (vsDraw.viewWhitespace != wsInvisible) {
								if (vsDraw.whitespaceForegroundSet)
									textFore = vsDraw.whitespaceForeground;
								if (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways) {
									XYPOSITION xmid = (ll->positions[cpos + startseg] + ll->positions[cpos + startseg + 1]) / 2;
									if (!twoPhaseDraw && drawWhitespaceBackground &&
									        (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways)) {
										textBack = vsDraw.whitespaceBackground;
										PRectangle rcSpace(ll->positions[cpos + startseg] + xStart - subLineStart,
											rcSegment.top,
											ll->positions[cpos + startseg + 1] + xStart - subLineStart,
											rcSegment.bottom);
										surface->FillRectangle(rcSpace, textBack);
									}
									PRectangle rcDot(xmid + xStart - subLineStart, rcSegment.top + vsDraw.lineHeight / 2, 0, 0);
									rcDot.right = rcDot.left + vs.whitespaceSize;
									rcDot.bottom = rcDot.top + vs.whitespaceSize;
									surface->FillRectangle(rcDot, textFore);
								}
							}
							if (inIndentation && vsDraw.viewIndentationGuides == ivReal) {
								for (int indentCount = (ll->positions[cpos + startseg] + epsilon) / indentWidth;
									indentCount <= (ll->positions[cpos + startseg + 1] - epsilon) / indentWidth;
									indentCount++) {
									if (indentCount > 0) {
										int xIndent = indentCount * indentWidth;
										DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcSegment,
												(ll->xHighlightGuide == xIndent));
									}
								}
							}
						} else {
							inIndentation = false;
						}
					}
				}
			}
			if (ll->hsStart != -1 && vsDraw.hotspotUnderline && iDoc >= ll->hsStart && iDoc < ll->hsEnd) {
				PRectangle rcUL = rcSegment;
				rcUL.top = rcUL.top + vsDraw.maxAscent + 1;
				rcUL.bottom = rcUL.top + 1;
				if (vsDraw.hotspotForegroundSet)
					surface->FillRectangle(rcUL, vsDraw.hotspotForeground);
				else
					surface->FillRectangle(rcUL, textFore);
			} else if (vsDraw.styles[styleMain].underline) {
				PRectangle rcUL = rcSegment;
				rcUL.top = rcUL.top + vsDraw.maxAscent + 1;
				rcUL.bottom = rcUL.top + 1;
				surface->FillRectangle(rcUL, textFore);
			}
		} else if (rcSegment.left > rcLine.right) {
			break;
		}
	}
	if ((vsDraw.viewIndentationGuides == ivLookForward || vsDraw.viewIndentationGuides == ivLookBoth)
	        && (subLine == 0)) {
		int indentSpace = pdoc->GetLineIndentation(line);
		int xStartText = ll->positions[pdoc->GetLineIndentPosition(line) - posLineStart];

		// Find the most recent line with some text

		int lineLastWithText = line;
		while (lineLastWithText > Platform::Maximum(line-20, 0) && pdoc->IsWhiteLine(lineLastWithText)) {
			lineLastWithText--;
		}
		if (lineLastWithText < line) {
			xStartText = 100000;	// Don't limit to visible indentation on empty line
			// This line is empty, so use indentation of last line with text
			int indentLastWithText = pdoc->GetLineIndentation(lineLastWithText);
			int isFoldHeader = pdoc->GetLevel(lineLastWithText) & SC_FOLDLEVELHEADERFLAG;
			if (isFoldHeader) {
				// Level is one more level than parent
				indentLastWithText += pdoc->IndentSize();
			}
			if (vsDraw.viewIndentationGuides == ivLookForward) {
				// In viLookForward mode, previous line only used if it is a fold header
				if (isFoldHeader) {
					indentSpace = Platform::Maximum(indentSpace, indentLastWithText);
				}
			} else {	// viLookBoth
				indentSpace = Platform::Maximum(indentSpace, indentLastWithText);
			}
		}

		int lineNextWithText = line;
		while (lineNextWithText < Platform::Minimum(line+20, pdoc->LinesTotal()) && pdoc->IsWhiteLine(lineNextWithText)) {
			lineNextWithText++;
		}
		if (lineNextWithText > line) {
			xStartText = 100000;	// Don't limit to visible indentation on empty line
			// This line is empty, so use indentation of first next line with text
			indentSpace = Platform::Maximum(indentSpace,
			        pdoc->GetLineIndentation(lineNextWithText));
		}

		for (int indentPos = pdoc->IndentSize(); indentPos < indentSpace; indentPos += pdoc->IndentSize()) {
			int xIndent = indentPos * vsDraw.spaceWidth;
			if (xIndent < xStartText) {
				DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIndent + xStart, rcSegment,
					(ll->xHighlightGuide == xIndent));
			}
		}
	}

	DrawIndicators(surface, vsDraw, line, xStart, rcLine, ll, subLine, lineEnd, false);

	// End of the drawing of the current line
	if (!twoPhaseDraw) {
		DrawEOL(surface, vsDraw, rcLine, ll, line, lineEnd,
		        xStart, subLine, subLineStart, overrideBackground, background,
		        drawWrapMarkEnd, wrapColour);
	}
	if (!hideSelection && ((vsDraw.selAlpha != SC_ALPHA_NOALPHA) || (vsDraw.selAdditionalAlpha != SC_ALPHA_NOALPHA))) {
		// For each selection draw
		int virtualSpaces = 0;
		if (subLine == (ll->lines - 1)) {
			virtualSpaces = sel.VirtualSpaceFor(pdoc->LineEnd(line));
		}
		SelectionPosition posStart(posLineStart + lineStart);
		SelectionPosition posEnd(posLineStart + lineEnd, virtualSpaces);
		SelectionSegment virtualSpaceRange(posStart, posEnd);
		for (size_t r=0; r<sel.Count(); r++) {
			int alpha = (r == sel.Main()) ? vsDraw.selAlpha : vsDraw.selAdditionalAlpha;
			if (alpha != SC_ALPHA_NOALPHA) {
				SelectionSegment portion = sel.Range(r).Intersect(virtualSpaceRange);
				if (!portion.Empty()) {
					const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
					rcSegment.left = xStart + ll->positions[portion.start.Position() - posLineStart] - subLineStart + portion.start.VirtualSpace() * spaceWidth;
					rcSegment.right = xStart + ll->positions[portion.end.Position() - posLineStart] - subLineStart + portion.end.VirtualSpace() * spaceWidth;
					if ((ll->wrapIndent != 0) && (lineStart != 0)) {
						if ((portion.start.Position() - posLineStart) == lineStart && sel.Range(r).ContainsCharacter(portion.start.Position() - 1))
							rcSegment.left -= static_cast<int>(ll->wrapIndent); // indentation added to xStart was truncated to int, so we do the same here
					}
					rcSegment.left = (rcSegment.left > rcLine.left) ? rcSegment.left : rcLine.left;
					rcSegment.right = (rcSegment.right < rcLine.right) ? rcSegment.right : rcLine.right;
					if (rcSegment.right > rcLine.left)
						SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw, r == sel.Main()), alpha);
				}
			}
		}
	}

	// Draw any translucent whole line states
	rcSegment = rcLine;
	if ((caret.active || vsDraw.alwaysShowCaretLineBackground) && vsDraw.showCaretLineBackground && ll->containsCaret) {
		SimpleAlphaRectangle(surface, rcSegment, vsDraw.caretLineBackground, vsDraw.caretLineAlpha);
	}
	marks = pdoc->GetMark(line);
	for (markBit = 0; (markBit < 32) && marks; markBit++) {
		if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_BACKGROUND)) {
			SimpleAlphaRectangle(surface, rcSegment, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
		} else if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_UNDERLINE)) {
			PRectangle rcUnderline = rcSegment;
			rcUnderline.top = rcUnderline.bottom - 2;
			SimpleAlphaRectangle(surface, rcUnderline, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
		}
		marks >>= 1;
	}
	if (vsDraw.maskInLine) {
		int marksMasked = pdoc->GetMark(line) & vsDraw.maskInLine;
		if (marksMasked) {
			for (markBit = 0; (markBit < 32) && marksMasked; markBit++) {
				if ((marksMasked & 1) && (vsDraw.markers[markBit].markType != SC_MARK_EMPTY)) {
					SimpleAlphaRectangle(surface, rcSegment, vsDraw.markers[markBit].back, vsDraw.markers[markBit].alpha);
				}
				marksMasked >>= 1;
			}
		}
	}
}

void Editor::DrawBlockCaret(Surface *surface, ViewStyle &vsDraw, LineLayout *ll, int subLine,
							int xStart, int offset, int posCaret, PRectangle rcCaret, ColourDesired caretColour) {

	int lineStart = ll->LineStart(subLine);
	int posBefore = posCaret;
	int posAfter = MovePositionOutsideChar(posCaret + 1, 1);
	int numCharsToDraw = posAfter - posCaret;

	// Work out where the starting and ending offsets are. We need to
	// see if the previous character shares horizontal space, such as a
	// glyph / combining character. If so we'll need to draw that too.
	int offsetFirstChar = offset;
	int offsetLastChar = offset + (posAfter - posCaret);
	while ((posBefore > 0) && ((offsetLastChar - numCharsToDraw) >= lineStart)) {
		if ((ll->positions[offsetLastChar] - ll->positions[offsetLastChar - numCharsToDraw]) > 0) {
			// The char does not share horizontal space
			break;
		}
		// Char shares horizontal space, update the numChars to draw
		// Update posBefore to point to the prev char
		posBefore = MovePositionOutsideChar(posBefore - 1, -1);
		numCharsToDraw = posAfter - posBefore;
		offsetFirstChar = offset - (posCaret - posBefore);
	}

	// See if the next character shares horizontal space, if so we'll
	// need to draw that too.
	if (offsetFirstChar < 0)
		offsetFirstChar = 0;
	numCharsToDraw = offsetLastChar - offsetFirstChar;
	while ((offsetLastChar < ll->LineStart(subLine + 1)) && (offsetLastChar <= ll->numCharsInLine)) {
		// Update posAfter to point to the 2nd next char, this is where
		// the next character ends, and 2nd next begins. We'll need
		// to compare these two
		posBefore = posAfter;
		posAfter = MovePositionOutsideChar(posAfter + 1, 1);
		offsetLastChar = offset + (posAfter - posCaret);
		if ((ll->positions[offsetLastChar] - ll->positions[offsetLastChar - (posAfter - posBefore)]) > 0) {
			// The char does not share horizontal space
			break;
		}
		// Char shares horizontal space, update the numChars to draw
		numCharsToDraw = offsetLastChar - offsetFirstChar;
	}

	// We now know what to draw, update the caret drawing rectangle
	rcCaret.left = ll->positions[offsetFirstChar] - ll->positions[lineStart] + xStart;
	rcCaret.right = ll->positions[offsetFirstChar+numCharsToDraw] - ll->positions[lineStart] + xStart;

	// Adjust caret position to take into account any word wrapping symbols.
	if ((ll->wrapIndent != 0) && (lineStart != 0)) {
		XYPOSITION wordWrapCharWidth = ll->wrapIndent;
		rcCaret.left += wordWrapCharWidth;
		rcCaret.right += wordWrapCharWidth;
	}

	// This character is where the caret block is, we override the colours
	// (inversed) for drawing the caret here.
	int styleMain = ll->styles[offsetFirstChar];
	surface->DrawTextClipped(rcCaret, vsDraw.styles[styleMain].font,
	        rcCaret.top + vsDraw.maxAscent, ll->chars + offsetFirstChar,
	        numCharsToDraw, vsDraw.styles[styleMain].back,
	        caretColour);
}

void Editor::RefreshPixMaps(Surface *surfaceWindow) {
	if (!pixmapSelPattern->Initialised()) {
		const int patternSize = 8;
		pixmapSelPattern->InitPixMap(patternSize, patternSize, surfaceWindow, wMain.GetID());
		pixmapSelPatternOffset1->InitPixMap(patternSize, patternSize, surfaceWindow, wMain.GetID());
		// This complex procedure is to reproduce the checkerboard dithered pattern used by windows
		// for scroll bars and Visual Studio for its selection margin. The colour of this pattern is half
		// way between the chrome colour and the chrome highlight colour making a nice transition
		// between the window chrome and the content area. And it works in low colour depths.
		PRectangle rcPattern(0, 0, patternSize, patternSize);

		// Initialize default colours based on the chrome colour scheme.  Typically the highlight is white.
		ColourDesired colourFMFill = vs.selbar;
		ColourDesired colourFMStripes = vs.selbarlight;

		if (!(vs.selbarlight == ColourDesired(0xff, 0xff, 0xff))) {
			// User has chosen an unusual chrome colour scheme so just use the highlight edge colour.
			// (Typically, the highlight colour is white.)
			colourFMFill = vs.selbarlight;
		}

		if (vs.foldmarginColourSet) {
			// override default fold margin colour
			colourFMFill = vs.foldmarginColour;
		}
		if (vs.foldmarginHighlightColourSet) {
			// override default fold margin highlight colour
			colourFMStripes = vs.foldmarginHighlightColour;
		}

		pixmapSelPattern->FillRectangle(rcPattern, colourFMFill);
		pixmapSelPatternOffset1->FillRectangle(rcPattern, colourFMStripes);
		for (int y = 0; y < patternSize; y++) {
			for (int x = y % 2; x < patternSize; x+=2) {
				PRectangle rcPixel(x, y, x+1, y+1);
				pixmapSelPattern->FillRectangle(rcPixel, colourFMStripes);
				pixmapSelPatternOffset1->FillRectangle(rcPixel, colourFMFill);
			}
		}
	}

	if (!pixmapIndentGuide->Initialised()) {
		// 1 extra pixel in height so can handle odd/even positions and so produce a continuous line
		pixmapIndentGuide->InitPixMap(1, vs.lineHeight + 1, surfaceWindow, wMain.GetID());
		pixmapIndentGuideHighlight->InitPixMap(1, vs.lineHeight + 1, surfaceWindow, wMain.GetID());
		PRectangle rcIG(0, 0, 1, vs.lineHeight);
		pixmapIndentGuide->FillRectangle(rcIG, vs.styles[STYLE_INDENTGUIDE].back);
		pixmapIndentGuide->PenColour(vs.styles[STYLE_INDENTGUIDE].fore);
		pixmapIndentGuideHighlight->FillRectangle(rcIG, vs.styles[STYLE_BRACELIGHT].back);
		pixmapIndentGuideHighlight->PenColour(vs.styles[STYLE_BRACELIGHT].fore);
		for (int stripe = 1; stripe < vs.lineHeight + 1; stripe += 2) {
			PRectangle rcPixel(0, stripe, 1, stripe+1);
			pixmapIndentGuide->FillRectangle(rcPixel, vs.styles[STYLE_INDENTGUIDE].fore);
			pixmapIndentGuideHighlight->FillRectangle(rcPixel, vs.styles[STYLE_BRACELIGHT].fore);
		}
	}

	if (bufferedDraw) {
		if (!pixmapLine->Initialised()) {
			PRectangle rcClient = GetClientRectangle();
			pixmapLine->InitPixMap(rcClient.Width(), vs.lineHeight,
			        surfaceWindow, wMain.GetID());
			pixmapSelMargin->InitPixMap(vs.fixedColumnWidth,
			        rcClient.Height(), surfaceWindow, wMain.GetID());
		}
	}
}

void Editor::DrawCarets(Surface *surface, ViewStyle &vsDraw, int lineDoc, int xStart,
        PRectangle rcLine, LineLayout *ll, int subLine) {
	// When drag is active it is the only caret drawn
	bool drawDrag = posDrag.IsValid();
	if (hideSelection && !drawDrag)
		return;
	const int posLineStart = pdoc->LineStart(lineDoc);
	// For each selection draw
	for (size_t r=0; (r<sel.Count()) || drawDrag; r++) {
		const bool mainCaret = r == sel.Main();
		const SelectionPosition posCaret = (drawDrag ? posDrag : sel.Range(r).caret);
		const int offset = posCaret.Position() - posLineStart;
		const XYPOSITION spaceWidth = vsDraw.styles[ll->EndLineStyle()].spaceWidth;
		const XYPOSITION virtualOffset = posCaret.VirtualSpace() * spaceWidth;
		if (ll->InLine(offset, subLine) && offset <= ll->numCharsBeforeEOL) {
			XYPOSITION xposCaret = ll->positions[offset] + virtualOffset - ll->positions[ll->LineStart(subLine)];
			if (ll->wrapIndent != 0) {
				int lineStart = ll->LineStart(subLine);
				if (lineStart != 0)	// Wrapped
					xposCaret += ll->wrapIndent;
			}
			bool caretBlinkState = (caret.active && caret.on) || (!additionalCaretsBlink && !mainCaret);
			bool caretVisibleState = additionalCaretsVisible || mainCaret;
			if ((xposCaret >= 0) && (vsDraw.caretWidth > 0) && (vsDraw.caretStyle != CARETSTYLE_INVISIBLE) &&
			        ((posDrag.IsValid()) || (caretBlinkState && caretVisibleState))) {
				bool caretAtEOF = false;
				bool caretAtEOL = false;
				bool drawBlockCaret = false;
				XYPOSITION widthOverstrikeCaret;
				int caretWidthOffset = 0;
				PRectangle rcCaret = rcLine;

				if (posCaret.Position() == pdoc->Length())	{   // At end of document
					caretAtEOF = true;
					widthOverstrikeCaret = vsDraw.aveCharWidth;
				} else if ((posCaret.Position() - posLineStart) >= ll->numCharsInLine) {	// At end of line
					caretAtEOL = true;
					widthOverstrikeCaret = vsDraw.aveCharWidth;
				} else {
					widthOverstrikeCaret = ll->positions[offset + 1] - ll->positions[offset];
				}
				if (widthOverstrikeCaret < 3)	// Make sure its visible
					widthOverstrikeCaret = 3;

				if (xposCaret > 0)
					caretWidthOffset = 1;	// Move back so overlaps both character cells.
				xposCaret += xStart;
				if (posDrag.IsValid()) {
					/* Dragging text, use a line caret */
					rcCaret.left = xposCaret - caretWidthOffset;
					rcCaret.right = rcCaret.left + vsDraw.caretWidth;
				} else if (inOverstrike && drawOverstrikeCaret) {
					/* Overstrike (insert mode), use a modified bar caret */
					rcCaret.top = rcCaret.bottom - 2;
					rcCaret.left = xposCaret + 1;
					rcCaret.right = rcCaret.left + widthOverstrikeCaret - 1;
				} else if (vsDraw.caretStyle == CARETSTYLE_BLOCK) {
					/* Block caret */
					rcCaret.left = xposCaret;
					if (!caretAtEOL && !caretAtEOF && (ll->chars[offset] != '\t') && !(IsControlCharacter(ll->chars[offset]))) {
						drawBlockCaret = true;
						rcCaret.right = xposCaret + widthOverstrikeCaret;
					} else {
						rcCaret.right = xposCaret + vsDraw.aveCharWidth;
					}
				} else {
					/* Line caret */
					rcCaret.left = xposCaret - caretWidthOffset;
					rcCaret.right = rcCaret.left + vsDraw.caretWidth;
				}
				ColourDesired caretColour = mainCaret ? vsDraw.caretcolour : vsDraw.additionalCaretColour;
				if (drawBlockCaret) {
					DrawBlockCaret(surface, vsDraw, ll, subLine, xStart, offset, posCaret.Position(), rcCaret, caretColour);
				} else {
					surface->FillRectangle(rcCaret, caretColour);
				}
			}
		}
		if (drawDrag)
			break;
	}
}

void Editor::Paint(Surface *surfaceWindow, PRectangle rcArea) {
	//Platform::DebugPrintf("Paint:%1d (%3d,%3d) ... (%3d,%3d)\n",
	//	paintingAllText, rcArea.left, rcArea.top, rcArea.right, rcArea.bottom);
	AllocateGraphics();

	RefreshStyleData();
	if (paintState == paintAbandoned)
		return;	// Scroll bars may have changed so need redraw
	RefreshPixMaps(surfaceWindow);

	paintAbandonedByStyling = false;

	StyleToPositionInView(PositionAfterArea(rcArea));

	PRectangle rcClient = GetClientRectangle();
	Point ptOrigin = GetVisibleOriginInMain();
	//Platform::DebugPrintf("Client: (%3d,%3d) ... (%3d,%3d)   %d\n",
	//	rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

	int screenLinePaintFirst = rcArea.top / vs.lineHeight;

	int xStart = vs.textStart - xOffset + ptOrigin.x;
	int ypos = 0;
	if (!bufferedDraw)
		ypos += screenLinePaintFirst * vs.lineHeight;
	int yposScreen = screenLinePaintFirst * vs.lineHeight;

	if (NotifyUpdateUI()) {
		RefreshStyleData();
		RefreshPixMaps(surfaceWindow);
	}

	// Wrap the visible lines if needed.
	if (WrapLines(wsVisible)) {
		// The wrapping process has changed the height of some lines so
		// abandon this paint for a complete repaint.
		if (AbandonPaint()) {
			return;
		}
		RefreshPixMaps(surfaceWindow);	// In case pixmaps invalidated by scrollbar change
	}
	PLATFORM_ASSERT(pixmapSelPattern->Initialised());

	if (!bufferedDraw)
		surfaceWindow->SetClip(rcArea);

	if (paintState != paintAbandoned) {
		if (vs.marginInside) {
			PaintSelMargin(surfaceWindow, rcArea);
			PRectangle rcRightMargin = rcClient;
			rcRightMargin.left = rcRightMargin.right - vs.rightMarginWidth;
			if (rcArea.Intersects(rcRightMargin)) {
				surfaceWindow->FillRectangle(rcRightMargin, vs.styles[STYLE_DEFAULT].back);
			}
		} else { // Else separate view so separate paint event but leftMargin included to allow overlap
			PRectangle rcLeftMargin = rcArea;
			rcLeftMargin.left = 0;
			rcLeftMargin.right = rcLeftMargin.left + vs.leftMarginWidth;
			if (rcArea.Intersects(rcLeftMargin)) {
				surfaceWindow->FillRectangle(rcLeftMargin, vs.styles[STYLE_DEFAULT].back);
			}
		}
	}

	if (paintState == paintAbandoned) {
		// Either styling or NotifyUpdateUI noticed that painting is needed
		// outside the current painting rectangle
		//Platform::DebugPrintf("Abandoning paint\n");
		if (wrapState != eWrapNone) {
			if (paintAbandonedByStyling) {
				// Styling has spilled over a line end, such as occurs by starting a multiline
				// comment. The width of subsequent text may have changed, so rewrap.
				NeedWrapping(cs.DocFromDisplay(topLine));
			}
		}
		return;
	}
	//Platform::DebugPrintf("start display %d, offset = %d\n", pdoc->Length(), xOffset);

	// Allow text at start of line to overlap 1 pixel into the margin as this displays
	// serifs and italic stems for aliased text.
	const int leftTextOverlap = ((xOffset == 0) && (vs.leftMarginWidth > 0)) ? 1 : 0;

	// Do the painting
	if (rcArea.right > vs.textStart - leftTextOverlap) {

		Surface *surface = surfaceWindow;
		if (bufferedDraw) {
			surface = pixmapLine;
			PLATFORM_ASSERT(pixmapLine->Initialised());
		}
		surface->SetUnicodeMode(IsUnicodeMode());
		surface->SetDBCSMode(CodePage());

		int visibleLine = TopLineOfMain() + screenLinePaintFirst;

		SelectionPosition posCaret = sel.RangeMain().caret;
		if (posDrag.IsValid())
			posCaret = posDrag;
		int lineCaret = pdoc->LineFromPosition(posCaret.Position());

		PRectangle rcTextArea = rcClient;
		if (vs.marginInside) {
			rcTextArea.left += vs.textStart;
			rcTextArea.right -= vs.rightMarginWidth;
		} else {
			rcTextArea = rcArea;
		}

		// Remove selection margin from drawing area so text will not be drawn
		// on it in unbuffered mode.
		if (!bufferedDraw && vs.marginInside) {
			PRectangle rcClipText = rcTextArea;
			rcClipText.left -= leftTextOverlap;
			surfaceWindow->SetClip(rcClipText);
		}

		// Loop on visible lines
		//double durLayout = 0.0;
		//double durPaint = 0.0;
		//double durCopy = 0.0;
		//ElapsedTime etWhole;
		int lineDocPrevious = -1;	// Used to avoid laying out one document line multiple times
		AutoLineLayout ll(llc, 0);
		while (visibleLine < cs.LinesDisplayed() && yposScreen < rcArea.bottom) {

			int lineDoc = cs.DocFromDisplay(visibleLine);
			// Only visible lines should be handled by the code within the loop
			PLATFORM_ASSERT(cs.GetVisible(lineDoc));
			int lineStartSet = cs.DisplayFromDoc(lineDoc);
			int subLine = visibleLine - lineStartSet;

			// Copy this line and its styles from the document into local arrays
			// and determine the x position at which each character starts.
			//ElapsedTime et;
			if (lineDoc != lineDocPrevious) {
				ll.Set(0);
				ll.Set(RetrieveLineLayout(lineDoc));
				LayoutLine(lineDoc, surface, vs, ll, wrapWidth);
				lineDocPrevious = lineDoc;
			}
			//durLayout += et.Duration(true);

			if (ll) {
				ll->containsCaret = lineDoc == lineCaret;
				if (hideSelection) {
					ll->containsCaret = false;
				}

				GetHotSpotRange(ll->hsStart, ll->hsEnd);

				PRectangle rcLine = rcTextArea;
				rcLine.top = ypos;
				rcLine.bottom = ypos + vs.lineHeight;

				bool bracesIgnoreStyle = false;
				if ((vs.braceHighlightIndicatorSet && (bracesMatchStyle == STYLE_BRACELIGHT)) ||
					(vs.braceBadLightIndicatorSet && (bracesMatchStyle == STYLE_BRACEBAD))) {
					bracesIgnoreStyle = true;
				}
				Range rangeLine(pdoc->LineStart(lineDoc), pdoc->LineStart(lineDoc + 1));
				// Highlight the current braces if any
				ll->SetBracesHighlight(rangeLine, braces, static_cast<char>(bracesMatchStyle),
				        highlightGuideColumn * vs.spaceWidth, bracesIgnoreStyle);

				if (leftTextOverlap && bufferedDraw) {
					PRectangle rcSpacer = rcLine;
					rcSpacer.right = rcSpacer.left;
					rcSpacer.left -= 1;
					surface->FillRectangle(rcSpacer, vs.styles[STYLE_DEFAULT].back);
				}

				// Draw the line
				DrawLine(surface, vs, lineDoc, visibleLine, xStart, rcLine, ll, subLine);
				//durPaint += et.Duration(true);

				// Restore the previous styles for the brace highlights in case layout is in cache.
				ll->RestoreBracesHighlight(rangeLine, braces, bracesIgnoreStyle);

				bool expanded = cs.GetExpanded(lineDoc);
				const int level = pdoc->GetLevel(lineDoc);
				const int levelNext = pdoc->GetLevel(lineDoc + 1);
				if ((level & SC_FOLDLEVELHEADERFLAG) &&
					((level & SC_FOLDLEVELNUMBERMASK) < (levelNext & SC_FOLDLEVELNUMBERMASK))) {
					// Paint the line above the fold
					if ((expanded && (foldFlags & SC_FOLDFLAG_LINEBEFORE_EXPANDED))
						||
						(!expanded && (foldFlags & SC_FOLDFLAG_LINEBEFORE_CONTRACTED))) {
						PRectangle rcFoldLine = rcLine;
						rcFoldLine.bottom = rcFoldLine.top + 1;
						surface->FillRectangle(rcFoldLine, vs.styles[STYLE_DEFAULT].fore);
					}
					// Paint the line below the fold
					if ((expanded && (foldFlags & SC_FOLDFLAG_LINEAFTER_EXPANDED))
						||
						(!expanded && (foldFlags & SC_FOLDFLAG_LINEAFTER_CONTRACTED))) {
						PRectangle rcFoldLine = rcLine;
						rcFoldLine.top = rcFoldLine.bottom - 1;
						surface->FillRectangle(rcFoldLine, vs.styles[STYLE_DEFAULT].fore);
					}
				}

				DrawCarets(surface, vs, lineDoc, xStart, rcLine, ll, subLine);

				if (bufferedDraw) {
					Point from(vs.textStart-leftTextOverlap, 0);
					PRectangle rcCopyArea(vs.textStart-leftTextOverlap, yposScreen,
					        rcClient.right - vs.rightMarginWidth, yposScreen + vs.lineHeight);
					surfaceWindow->Copy(rcCopyArea, from, *pixmapLine);
				}

				lineWidthMaxSeen = Platform::Maximum(
					    lineWidthMaxSeen, ll->positions[ll->numCharsInLine]);
				//durCopy += et.Duration(true);
			}

			if (!bufferedDraw) {
				ypos += vs.lineHeight;
			}

			yposScreen += vs.lineHeight;
			visibleLine++;

			//gdk_flush();
		}
		ll.Set(0);
		//if (durPaint < 0.00000001)
		//	durPaint = 0.00000001;

		// Right column limit indicator
		PRectangle rcBeyondEOF = (vs.marginInside) ? rcClient : rcArea;
		rcBeyondEOF.left = vs.textStart;
		rcBeyondEOF.right = rcBeyondEOF.right - ((vs.marginInside) ? vs.rightMarginWidth : 0);
		rcBeyondEOF.top = (cs.LinesDisplayed() - TopLineOfMain()) * vs.lineHeight;
		if (rcBeyondEOF.top < rcBeyondEOF.bottom) {
			surfaceWindow->FillRectangle(rcBeyondEOF, vs.styles[STYLE_DEFAULT].back);
			if (vs.edgeState == EDGE_LINE) {
				int edgeX = theEdge * vs.spaceWidth;
				rcBeyondEOF.left = edgeX + xStart;
				rcBeyondEOF.right = rcBeyondEOF.left + 1;
				surfaceWindow->FillRectangle(rcBeyondEOF, vs.edgecolour);
			}
		}
		//Platform::DebugPrintf(
		//"Layout:%9.6g    Paint:%9.6g    Ratio:%9.6g   Copy:%9.6g   Total:%9.6g\n",
		//durLayout, durPaint, durLayout / durPaint, durCopy, etWhole.Duration());
		NotifyPainted();
	}
}

// Space (3 space characters) between line numbers and text when printing.
#define lineNumberPrintSpace "   "

ColourDesired InvertedLight(ColourDesired orig) {
	unsigned int r = orig.GetRed();
	unsigned int g = orig.GetGreen();
	unsigned int b = orig.GetBlue();
	unsigned int l = (r + g + b) / 3; 	// There is a better calculation for this that matches human eye
	unsigned int il = 0xff - l;
	if (l == 0)
		return ColourDesired(0xff, 0xff, 0xff);
	r = r * il / l;
	g = g * il / l;
	b = b * il / l;
	return ColourDesired(Platform::Minimum(r, 0xff), Platform::Minimum(g, 0xff), Platform::Minimum(b, 0xff));
}

// This is mostly copied from the Paint method but with some things omitted
// such as the margin markers, line numbers, selection and caret
// Should be merged back into a combined Draw method.
long Editor::FormatRange(bool draw, Sci_RangeToFormat *pfr) {
	if (!pfr)
		return 0;

	AutoSurface surface(pfr->hdc, this, SC_TECHNOLOGY_DEFAULT);
	if (!surface)
		return 0;
	AutoSurface surfaceMeasure(pfr->hdcTarget, this, SC_TECHNOLOGY_DEFAULT);
	if (!surfaceMeasure) {
		return 0;
	}

	// Can't use measurements cached for screen
	posCache.Clear();

	ViewStyle vsPrint(vs);
	vsPrint.technology = SC_TECHNOLOGY_DEFAULT;

	// Modify the view style for printing as do not normally want any of the transient features to be printed
	// Printing supports only the line number margin.
	int lineNumberIndex = -1;
	for (int margin = 0; margin <= SC_MAX_MARGIN; margin++) {
		if ((vsPrint.ms[margin].style == SC_MARGIN_NUMBER) && (vsPrint.ms[margin].width > 0)) {
			lineNumberIndex = margin;
		} else {
			vsPrint.ms[margin].width = 0;
		}
	}
	vsPrint.fixedColumnWidth = 0;
	vsPrint.zoomLevel = printMagnification;
	// Don't show indentation guides
	// If this ever gets changed, cached pixmap would need to be recreated if technology != SC_TECHNOLOGY_DEFAULT
	vsPrint.viewIndentationGuides = ivNone;
	// Don't show the selection when printing
	vsPrint.selbackset = false;
	vsPrint.selforeset = false;
	vsPrint.selAlpha = SC_ALPHA_NOALPHA;
	vsPrint.selAdditionalAlpha = SC_ALPHA_NOALPHA;
	vsPrint.whitespaceBackgroundSet = false;
	vsPrint.whitespaceForegroundSet = false;
	vsPrint.showCaretLineBackground = false;
	vsPrint.alwaysShowCaretLineBackground = false;
	// Don't highlight matching braces using indicators
	vsPrint.braceHighlightIndicatorSet = false;
	vsPrint.braceBadLightIndicatorSet = false;

	// Set colours for printing according to users settings
	for (size_t sty = 0; sty < vsPrint.styles.size(); sty++) {
		if (printColourMode == SC_PRINT_INVERTLIGHT) {
			vsPrint.styles[sty].fore = InvertedLight(vsPrint.styles[sty].fore);
			vsPrint.styles[sty].back = InvertedLight(vsPrint.styles[sty].back);
		} else if (printColourMode == SC_PRINT_BLACKONWHITE) {
			vsPrint.styles[sty].fore = ColourDesired(0, 0, 0);
			vsPrint.styles[sty].back = ColourDesired(0xff, 0xff, 0xff);
		} else if (printColourMode == SC_PRINT_COLOURONWHITE) {
			vsPrint.styles[sty].back = ColourDesired(0xff, 0xff, 0xff);
		} else if (printColourMode == SC_PRINT_COLOURONWHITEDEFAULTBG) {
			if (sty <= STYLE_DEFAULT) {
				vsPrint.styles[sty].back = ColourDesired(0xff, 0xff, 0xff);
			}
		}
	}
	// White background for the line numbers
	vsPrint.styles[STYLE_LINENUMBER].back = ColourDesired(0xff, 0xff, 0xff);

	// Printing uses different margins, so reset screen margins
	vsPrint.leftMarginWidth = 0;
	vsPrint.rightMarginWidth = 0;

	vsPrint.Refresh(*surfaceMeasure);
	// Determining width must hapen after fonts have been realised in Refresh
	int lineNumberWidth = 0;
	if (lineNumberIndex >= 0) {
		lineNumberWidth = surfaceMeasure->WidthText(vsPrint.styles[STYLE_LINENUMBER].font,
		        "99999" lineNumberPrintSpace, 5 + istrlen(lineNumberPrintSpace));
		vsPrint.ms[lineNumberIndex].width = lineNumberWidth;
		vsPrint.Refresh(*surfaceMeasure);	// Recalculate fixedColumnWidth
	}

	int linePrintStart = pdoc->LineFromPosition(pfr->chrg.cpMin);
	int linePrintLast = linePrintStart + (pfr->rc.bottom - pfr->rc.top) / vsPrint.lineHeight - 1;
	if (linePrintLast < linePrintStart)
		linePrintLast = linePrintStart;
	int linePrintMax = pdoc->LineFromPosition(pfr->chrg.cpMax);
	if (linePrintLast > linePrintMax)
		linePrintLast = linePrintMax;
	//Platform::DebugPrintf("Formatting lines=[%0d,%0d,%0d] top=%0d bottom=%0d line=%0d %0d\n",
	//      linePrintStart, linePrintLast, linePrintMax, pfr->rc.top, pfr->rc.bottom, vsPrint.lineHeight,
	//      surfaceMeasure->Height(vsPrint.styles[STYLE_LINENUMBER].font));
	int endPosPrint = pdoc->Length();
	if (linePrintLast < pdoc->LinesTotal())
		endPosPrint = pdoc->LineStart(linePrintLast + 1);

	// Ensure we are styled to where we are formatting.
	pdoc->EnsureStyledTo(endPosPrint);

	int xStart = vsPrint.fixedColumnWidth + pfr->rc.left;
	int ypos = pfr->rc.top;

	int lineDoc = linePrintStart;

	int nPrintPos = pfr->chrg.cpMin;
	int visibleLine = 0;
	int widthPrint = pfr->rc.right - pfr->rc.left - vsPrint.fixedColumnWidth;
	if (printWrapState == eWrapNone)
		widthPrint = LineLayout::wrapWidthInfinite;

	while (lineDoc <= linePrintLast && ypos < pfr->rc.bottom) {

		// When printing, the hdc and hdcTarget may be the same, so
		// changing the state of surfaceMeasure may change the underlying
		// state of surface. Therefore, any cached state is discarded before
		// using each surface.
		surfaceMeasure->FlushCachedState();

		// Copy this line and its styles from the document into local arrays
		// and determine the x position at which each character starts.
		LineLayout ll(pdoc->LineStart(lineDoc+1)-pdoc->LineStart(lineDoc)+1);
		LayoutLine(lineDoc, surfaceMeasure, vsPrint, &ll, widthPrint);

		ll.containsCaret = false;

		PRectangle rcLine;
		rcLine.left = pfr->rc.left;
		rcLine.top = ypos;
		rcLine.right = pfr->rc.right - 1;
		rcLine.bottom = ypos + vsPrint.lineHeight;

		// When document line is wrapped over multiple display lines, find where
		// to start printing from to ensure a particular position is on the first
		// line of the page.
		if (visibleLine == 0) {
			int startWithinLine = nPrintPos - pdoc->LineStart(lineDoc);
			for (int iwl = 0; iwl < ll.lines - 1; iwl++) {
				if (ll.LineStart(iwl) <= startWithinLine && ll.LineStart(iwl + 1) >= startWithinLine) {
					visibleLine = -iwl;
				}
			}

			if (ll.lines > 1 && startWithinLine >= ll.LineStart(ll.lines - 1)) {
				visibleLine = -(ll.lines - 1);
			}
		}

		if (draw && lineNumberWidth &&
		        (ypos + vsPrint.lineHeight <= pfr->rc.bottom) &&
		        (visibleLine >= 0)) {
			char number[100];
			sprintf(number, "%d" lineNumberPrintSpace, lineDoc + 1);
			PRectangle rcNumber = rcLine;
			rcNumber.right = rcNumber.left + lineNumberWidth;
			// Right justify
			rcNumber.left = rcNumber.right - surfaceMeasure->WidthText(
			            vsPrint.styles[STYLE_LINENUMBER].font, number, istrlen(number));
			surface->FlushCachedState();
			surface->DrawTextNoClip(rcNumber, vsPrint.styles[STYLE_LINENUMBER].font,
			        ypos + vsPrint.maxAscent, number, istrlen(number),
			        vsPrint.styles[STYLE_LINENUMBER].fore,
			        vsPrint.styles[STYLE_LINENUMBER].back);
		}

		// Draw the line
		surface->FlushCachedState();

		for (int iwl = 0; iwl < ll.lines; iwl++) {
			if (ypos + vsPrint.lineHeight <= pfr->rc.bottom) {
				if (visibleLine >= 0) {
					if (draw) {
						rcLine.top = ypos;
						rcLine.bottom = ypos + vsPrint.lineHeight;
						DrawLine(surface, vsPrint, lineDoc, visibleLine, xStart, rcLine, &ll, iwl);
					}
					ypos += vsPrint.lineHeight;
				}
				visibleLine++;
				if (iwl == ll.lines - 1)
					nPrintPos = pdoc->LineStart(lineDoc + 1);
				else
					nPrintPos += ll.LineStart(iwl + 1) - ll.LineStart(iwl);
			}
		}

		++lineDoc;
	}

	// Clear cache so measurements are not used for screen
	posCache.Clear();

	return nPrintPos;
}

int Editor::TextWidth(int style, const char *text) {
	RefreshStyleData();
	AutoSurface surface(this);
	if (surface) {
		return surface->WidthText(vs.styles[style].font, text, istrlen(text));
	} else {
		return 1;
	}
}

// Empty method is overridden on GTK+ to show / hide scrollbars
void Editor::ReconfigureScrollBars() {}

void Editor::SetScrollBars() {
	RefreshStyleData();

	int nMax = MaxScrollPos();
	int nPage = LinesOnScreen();
	bool modified = ModifyScrollBars(nMax + nPage - 1, nPage);
	if (modified) {
		DwellEnd(true);
	}

	// TODO: ensure always showing as many lines as possible
	// May not be, if, for example, window made larger
	if (topLine > MaxScrollPos()) {
		SetTopLine(Platform::Clamp(topLine, 0, MaxScrollPos()));
		SetVerticalScrollPos();
		Redraw();
	}
	if (modified) {
		if (!AbandonPaint())
			Redraw();
	}
	//Platform::DebugPrintf("end max = %d page = %d\n", nMax, nPage);
}

void Editor::ChangeSize() {
	DropGraphics(false);
	SetScrollBars();
	if (wrapState != eWrapNone) {
		PRectangle rcTextArea = GetClientRectangle();
		rcTextArea.left = vs.textStart;
		rcTextArea.right -= vs.rightMarginWidth;
		if (wrapWidth != rcTextArea.Width()) {
			NeedWrapping();
			Redraw();
		}
	}
}

int Editor::InsertSpace(int position, unsigned int spaces) {
	if (spaces > 0) {
		std::string spaceText(spaces, ' ');
		pdoc->InsertString(position, spaceText.c_str(), spaces);
		position += spaces;
	}
	return position;
}

void Editor::AddChar(char ch) {
	char s[2];
	s[0] = ch;
	s[1] = '\0';
	AddCharUTF(s, 1);
}

void Editor::FilterSelections() {
	if (!additionalSelectionTyping && (sel.Count() > 1)) {
		SelectionRange rangeOnly = sel.RangeMain();
		InvalidateSelection(rangeOnly, true);
		sel.SetSelection(rangeOnly);
	}
}

static bool cmpSelPtrs(const SelectionRange *a, const SelectionRange *b) {
	return *a < *b;
}

// AddCharUTF inserts an array of bytes which may or may not be in UTF-8.
void Editor::AddCharUTF(char *s, unsigned int len, bool treatAsDBCS) {
	FilterSelections();
	{
		UndoGroup ug(pdoc, (sel.Count() > 1) || !sel.Empty() || inOverstrike);

		std::vector<SelectionRange *> selPtrs;
		for (size_t r = 0; r < sel.Count(); r++) {
			selPtrs.push_back(&sel.Range(r));
		}
		std::sort(selPtrs.begin(), selPtrs.end(), cmpSelPtrs);

		for (std::vector<SelectionRange *>::reverse_iterator rit = selPtrs.rbegin();
			rit != selPtrs.rend(); ++rit) {
			SelectionRange *currentSel = *rit;
			if (!RangeContainsProtected(currentSel->Start().Position(),
				currentSel->End().Position())) {
				int positionInsert = currentSel->Start().Position();
				if (!currentSel->Empty()) {
					if (currentSel->Length()) {
						pdoc->DeleteChars(positionInsert, currentSel->Length());
						currentSel->ClearVirtualSpace();
					} else {
						// Range is all virtual so collapse to start of virtual space
						currentSel->MinimizeVirtualSpace();
					}
				} else if (inOverstrike) {
					if (positionInsert < pdoc->Length()) {
						if (!pdoc->IsPositionInLineEnd(positionInsert)) {
							pdoc->DelChar(positionInsert);
							currentSel->ClearVirtualSpace();
						}
					}
				}
				positionInsert = InsertSpace(positionInsert, currentSel->caret.VirtualSpace());
				if (pdoc->InsertString(positionInsert, s, len)) {
					currentSel->caret.SetPosition(positionInsert + len);
					currentSel->anchor.SetPosition(positionInsert + len);
				}
				currentSel->ClearVirtualSpace();
				// If in wrap mode rewrap current line so EnsureCaretVisible has accurate information
				if (wrapState != eWrapNone) {
					AutoSurface surface(this);
					if (surface) {
						if (WrapOneLine(surface, pdoc->LineFromPosition(positionInsert))) {
							SetScrollBars();
							SetVerticalScrollPos();
							Redraw();
						}
					}
				}
			}
		}
	}
	if (wrapState != eWrapNone) {
		SetScrollBars();
	}
	ThinRectangularRange();
	// If in wrap mode rewrap current line so EnsureCaretVisible has accurate information
	EnsureCaretVisible();
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
	if ((caretSticky == SC_CARETSTICKY_OFF) ||
		((caretSticky == SC_CARETSTICKY_WHITESPACE) && !IsAllSpacesOrTabs(s, len))) {
		SetLastXChosen();
	}

	if (treatAsDBCS) {
		NotifyChar((static_cast<unsigned char>(s[0]) << 8) |
		        static_cast<unsigned char>(s[1]));
	} else if (len > 0) {
		int byte = static_cast<unsigned char>(s[0]);
		if ((byte < 0xC0) || (1 == len)) {
			// Handles UTF-8 characters between 0x01 and 0x7F and single byte
			// characters when not in UTF-8 mode.
			// Also treats \0 and naked trail bytes 0x80 to 0xBF as valid
			// characters representing themselves.
		} else {
			// Unroll 1 to 3 byte UTF-8 sequences.  See reference data at:
			// http://www.cl.cam.ac.uk/~mgk25/unicode.html
			// http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
			if (byte < 0xE0) {
				int byte2 = static_cast<unsigned char>(s[1]);
				if ((byte2 & 0xC0) == 0x80) {
					// Two-byte-character lead-byte followed by a trail-byte.
					byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
				}
				// A two-byte-character lead-byte not followed by trail-byte
				// represents itself.
			} else if (byte < 0xF0) {
				int byte2 = static_cast<unsigned char>(s[1]);
				int byte3 = static_cast<unsigned char>(s[2]);
				if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80)) {
					// Three-byte-character lead byte followed by two trail bytes.
					byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) |
					        (byte3 & 0x3F));
				}
				// A three-byte-character lead-byte not followed by two trail-bytes
				// represents itself.
			}
		}
		NotifyChar(byte);
	}

	if (recordingMacro) {
		NotifyMacroRecord(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(s));
	}
}

void Editor::InsertPaste(SelectionPosition selStart, const char *text, int len) {
	if (multiPasteMode == SC_MULTIPASTE_ONCE) {
		selStart = SelectionPosition(InsertSpace(selStart.Position(), selStart.VirtualSpace()));
		if (pdoc->InsertString(selStart.Position(), text, len)) {
			SetEmptySelection(selStart.Position() + len);
		}
	} else {
		// SC_MULTIPASTE_EACH
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r).Start().Position(),
				sel.Range(r).End().Position())) {
				int positionInsert = sel.Range(r).Start().Position();
				if (!sel.Range(r).Empty()) {
					if (sel.Range(r).Length()) {
						pdoc->DeleteChars(positionInsert, sel.Range(r).Length());
						sel.Range(r).ClearVirtualSpace();
					} else {
						// Range is all virtual so collapse to start of virtual space
						sel.Range(r).MinimizeVirtualSpace();
					}
				}
				positionInsert = InsertSpace(positionInsert, sel.Range(r).caret.VirtualSpace());
				if (pdoc->InsertString(positionInsert, text, len)) {
					sel.Range(r).caret.SetPosition(positionInsert + len);
					sel.Range(r).anchor.SetPosition(positionInsert + len);
				}
				sel.Range(r).ClearVirtualSpace();
			}
		}
	}
}

void Editor::ClearSelection(bool retainMultipleSelections) {
	if (!sel.IsRectangular() && !retainMultipleSelections)
		FilterSelections();
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		if (!sel.Range(r).Empty()) {
			if (!RangeContainsProtected(sel.Range(r).Start().Position(),
				sel.Range(r).End().Position())) {
				pdoc->DeleteChars(sel.Range(r).Start().Position(),
					sel.Range(r).Length());
				sel.Range(r) = sel.Range(r).Start();
			}
		}
	}
	ThinRectangularRange();
	sel.RemoveDuplicates();
	ClaimSelection();
}

void Editor::ClearAll() {
	{
		UndoGroup ug(pdoc);
		if (0 != pdoc->Length()) {
			pdoc->DeleteChars(0, pdoc->Length());
		}
		if (!pdoc->IsReadOnly()) {
			cs.Clear();
			pdoc->AnnotationClearAll();
			pdoc->MarginClearAll();
		}
	}
	sel.Clear();
	SetTopLine(0);
	SetVerticalScrollPos();
	InvalidateStyleRedraw();
}

void Editor::ClearDocumentStyle() {
	Decoration *deco = pdoc->decorations.root;
	while (deco) {
		// Save next in case deco deleted
		Decoration *decoNext = deco->next;
		if (deco->indicator < INDIC_CONTAINER) {
			pdoc->decorations.SetCurrentIndicator(deco->indicator);
			pdoc->DecorationFillRange(0, 0, pdoc->Length());
		}
		deco = decoNext;
	}
	pdoc->StartStyling(0, '\377');
	pdoc->SetStyleFor(pdoc->Length(), 0);
	cs.ShowAll();
	pdoc->ClearLevels();
}

void Editor::CopyAllowLine() {
	SelectionText selectedText;
	CopySelectionRange(&selectedText, true);
	CopyToClipboard(selectedText);
}

void Editor::Cut() {
	pdoc->CheckReadOnly();
	if (!pdoc->IsReadOnly() && !SelectionContainsProtected()) {
		Copy();
		ClearSelection();
	}
}

void Editor::PasteRectangular(SelectionPosition pos, const char *ptr, int len) {
	if (pdoc->IsReadOnly() || SelectionContainsProtected()) {
		return;
	}
	sel.Clear();
	sel.RangeMain() = SelectionRange(pos);
	int line = pdoc->LineFromPosition(sel.MainCaret());
	UndoGroup ug(pdoc);
	sel.RangeMain().caret = SelectionPosition(
		InsertSpace(sel.RangeMain().caret.Position(), sel.RangeMain().caret.VirtualSpace()));
	int xInsert = XFromPosition(sel.RangeMain().caret);
	bool prevCr = false;
	while ((len > 0) && IsEOLChar(ptr[len-1]))
		len--;
	for (int i = 0; i < len; i++) {
		if (IsEOLChar(ptr[i])) {
			if ((ptr[i] == '\r') || (!prevCr))
				line++;
			if (line >= pdoc->LinesTotal()) {
				if (pdoc->eolMode != SC_EOL_LF)
					pdoc->InsertChar(pdoc->Length(), '\r');
				if (pdoc->eolMode != SC_EOL_CR)
					pdoc->InsertChar(pdoc->Length(), '\n');
			}
			// Pad the end of lines with spaces if required
			sel.RangeMain().caret.SetPosition(PositionFromLineX(line, xInsert));
			if ((XFromPosition(sel.MainCaret()) < xInsert) && (i + 1 < len)) {
				while (XFromPosition(sel.MainCaret()) < xInsert) {
					pdoc->InsertChar(sel.MainCaret(), ' ');
					sel.RangeMain().caret.Add(1);
				}
			}
			prevCr = ptr[i] == '\r';
		} else {
			pdoc->InsertString(sel.MainCaret(), ptr + i, 1);
			sel.RangeMain().caret.Add(1);
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
						sel.Range(r) = SelectionPosition(InsertSpace(sel.Range(r).anchor.Position(), sel.Range(r).anchor.VirtualSpace()));
					else
						sel.Range(r) = SelectionPosition(InsertSpace(sel.Range(r).caret.Position(), sel.Range(r).caret.VirtualSpace()));
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
}

void Editor::SelectAll() {
	sel.Clear();
	SetSelection(0, pdoc->Length());
	Redraw();
}

void Editor::Undo() {
	if (pdoc->CanUndo()) {
		InvalidateCaret();
		int newPos = pdoc->Undo();
		if (newPos >= 0)
			SetEmptySelection(newPos);
		EnsureCaretVisible();
	}
}

void Editor::Redo() {
	if (pdoc->CanRedo()) {
		int newPos = pdoc->Redo();
		if (newPos >= 0)
			SetEmptySelection(newPos);
		EnsureCaretVisible();
	}
}

void Editor::DelChar() {
	if (!RangeContainsProtected(sel.MainCaret(), sel.MainCaret() + 1)) {
		pdoc->DelChar(sel.MainCaret());
	}
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

void Editor::DelCharBack(bool allowLineStartDeletion) {
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
					int lineCurrentPos = pdoc->LineFromPosition(sel.Range(r).caret.Position());
					if (allowLineStartDeletion || (pdoc->LineStart(lineCurrentPos) != sel.Range(r).caret.Position())) {
						if (pdoc->GetColumn(sel.Range(r).caret.Position()) <= pdoc->GetLineIndentation(lineCurrentPos) &&
								pdoc->GetColumn(sel.Range(r).caret.Position()) > 0 && pdoc->backspaceUnindents) {
							UndoGroup ugInner(pdoc, !ug.Needed());
							int indentation = pdoc->GetLineIndentation(lineCurrentPos);
							int indentationStep = pdoc->IndentSize();
							if (indentation % indentationStep == 0) {
								pdoc->SetLineIndentation(lineCurrentPos, indentation - indentationStep);
							} else {
								pdoc->SetLineIndentation(lineCurrentPos, indentation - (indentation % indentationStep));
							}
							// SetEmptySelection
							sel.Range(r) = SelectionRange(pdoc->GetLineIndentPosition(lineCurrentPos),
								pdoc->GetLineIndentPosition(lineCurrentPos));
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
	ContainerNeedsUpdate(SC_UPDATE_SELECTION);
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

void Editor::NotifyFocus(bool) {}

void Editor::SetCtrlID(int identifier) {
	ctrlID = identifier;
}

void Editor::NotifyStyleToNeeded(int endStyleNeeded) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_STYLENEEDED;
	scn.position = endStyleNeeded;
	NotifyParent(scn);
}

void Editor::NotifyStyleNeeded(Document *, void *, int endStyleNeeded) {
	NotifyStyleToNeeded(endStyleNeeded);
}

void Editor::NotifyLexerChanged(Document *, void *) {
}

void Editor::NotifyErrorOccurred(Document *, void *, int status) {
	errorStatus = status;
}

void Editor::NotifyChar(int ch) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_CHARADDED;
	scn.ch = ch;
	NotifyParent(scn);
}

void Editor::NotifySavePoint(bool isSavePoint) {
	SCNotification scn = {0};
	if (isSavePoint) {
		scn.nmhdr.code = SCN_SAVEPOINTREACHED;
	} else {
		scn.nmhdr.code = SCN_SAVEPOINTLEFT;
	}
	NotifyParent(scn);
}

void Editor::NotifyModifyAttempt() {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_MODIFYATTEMPTRO;
	NotifyParent(scn);
}

void Editor::NotifyDoubleClick(Point pt, bool shift, bool ctrl, bool alt) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_DOUBLECLICK;
	scn.line = LineFromLocation(pt);
	scn.position = PositionFromLocation(pt, true);
	scn.modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) |
	        (alt ? SCI_ALT : 0);
	NotifyParent(scn);
}

void Editor::NotifyHotSpotDoubleClicked(int position, bool shift, bool ctrl, bool alt) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_HOTSPOTDOUBLECLICK;
	scn.position = position;
	scn.modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) |
	        (alt ? SCI_ALT : 0);
	NotifyParent(scn);
}

void Editor::NotifyHotSpotClicked(int position, bool shift, bool ctrl, bool alt) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_HOTSPOTCLICK;
	scn.position = position;
	scn.modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) |
	        (alt ? SCI_ALT : 0);
	NotifyParent(scn);
}

void Editor::NotifyHotSpotReleaseClick(int position, bool shift, bool ctrl, bool alt) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_HOTSPOTRELEASECLICK;
	scn.position = position;
	scn.modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) |
	        (alt ? SCI_ALT : 0);
	NotifyParent(scn);
}

bool Editor::NotifyUpdateUI() {
	if (needUpdateUI) {
		SCNotification scn = {0};
		scn.nmhdr.code = SCN_UPDATEUI;
		scn.updated = needUpdateUI;
		NotifyParent(scn);
		needUpdateUI = 0;
		return true;
	}
	return false;
}

void Editor::NotifyPainted() {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_PAINTED;
	NotifyParent(scn);
}

void Editor::NotifyIndicatorClick(bool click, int position, bool shift, bool ctrl, bool alt) {
	int mask = pdoc->decorations.AllOnFor(position);
	if ((click && mask) || pdoc->decorations.clickNotified) {
		SCNotification scn = {0};
		pdoc->decorations.clickNotified = click;
		scn.nmhdr.code = click ? SCN_INDICATORCLICK : SCN_INDICATORRELEASE;
		scn.modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) | (alt ? SCI_ALT : 0);
		scn.position = position;
		NotifyParent(scn);
	}
}

bool Editor::NotifyMarginClick(Point pt, bool shift, bool ctrl, bool alt) {
	int marginClicked = -1;
	int x = vs.textStart - vs.fixedColumnWidth;
	for (int margin = 0; margin <= SC_MAX_MARGIN; margin++) {
		if ((pt.x >= x) && (pt.x < x + vs.ms[margin].width))
			marginClicked = margin;
		x += vs.ms[margin].width;
	}
	if ((marginClicked >= 0) && vs.ms[marginClicked].sensitive) {
		int position = pdoc->LineStart(LineFromLocation(pt));
		if ((vs.ms[marginClicked].mask & SC_MASK_FOLDERS) && (foldAutomatic & SC_AUTOMATICFOLD_CLICK)) {
			int lineClick = pdoc->LineFromPosition(position);
			if (shift && ctrl) {
				FoldAll(SC_FOLDACTION_TOGGLE);
			} else {
				int levelClick = pdoc->GetLevel(lineClick);
				if (levelClick & SC_FOLDLEVELHEADERFLAG) {
					if (shift) {
						// Ensure all children visible
						FoldExpand(lineClick, SC_FOLDACTION_EXPAND, levelClick);
					} else if (ctrl) {
						FoldExpand(lineClick, SC_FOLDACTION_TOGGLE, levelClick);
					} else {
						// Toggle this line
						FoldLine(lineClick, SC_FOLDACTION_TOGGLE);
					}
				}
			}
			return true;
		}
		SCNotification scn = {0};
		scn.nmhdr.code = SCN_MARGINCLICK;
		scn.modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) |
		        (alt ? SCI_ALT : 0);
		scn.position = position;
		scn.margin = marginClicked;
		NotifyParent(scn);
		return true;
	} else {
		return false;
	}
}

void Editor::NotifyNeedShown(int pos, int len) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_NEEDSHOWN;
	scn.position = pos;
	scn.length = len;
	NotifyParent(scn);
}

void Editor::NotifyDwelling(Point pt, bool state) {
	SCNotification scn = {0};
	scn.nmhdr.code = state ? SCN_DWELLSTART : SCN_DWELLEND;
	scn.position = PositionFromLocation(pt, true);
	scn.x = pt.x;
	scn.y = pt.y;
	NotifyParent(scn);
}

void Editor::NotifyZoom() {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_ZOOM;
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
	if (mh.modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
		llc.Invalidate(LineLayout::llCheckTextAndStyle);
		int lineDoc = pdoc->LineFromPosition(mh.position);
		int lines = Platform::Maximum(0, mh.linesAdded);
		if (wrapState != eWrapNone) {
			NeedWrapping(lineDoc, lineDoc + lines + 1);
		}
		RefreshStyleData();
		// Fix up annotation heights
		SetAnnotationHeights(lineDoc, lineDoc + lines + 2);
	}
}

// Move a position so it is still after the same character as before the insertion.
static inline int MovePositionForInsertion(int position, int startInsertion, int length) {
	if (position > startInsertion) {
		return position + length;
	}
	return position;
}

// Move a position so it is still after the same character as before the deletion if that
// character is still present else after the previous surviving character.
static inline int MovePositionForDeletion(int position, int startDeletion, int length) {
	if (position > startDeletion) {
		int endDeletion = startDeletion + length;
		if (position > endDeletion) {
			return position - length;
		} else {
			return startDeletion;
		}
	} else {
		return position;
	}
}

void Editor::NotifyModified(Document *, DocModification mh, void *) {
	ContainerNeedsUpdate(SC_UPDATE_CONTENT);
	if (paintState == painting) {
		CheckForChangeOutsidePaint(Range(mh.position, mh.position + mh.length));
	}
	if (mh.modificationType & SC_MOD_CHANGELINESTATE) {
		if (paintState == painting) {
			CheckForChangeOutsidePaint(
			    Range(pdoc->LineStart(mh.line), pdoc->LineStart(mh.line + 1)));
		} else {
			// Could check that change is before last visible line.
			Redraw();
		}
	}
	if (mh.modificationType & SC_MOD_LEXERSTATE) {
		if (paintState == painting) {
			CheckForChangeOutsidePaint(
			    Range(mh.position, mh.position + mh.length));
		} else {
			Redraw();
		}
	}
	if (mh.modificationType & (SC_MOD_CHANGESTYLE | SC_MOD_CHANGEINDICATOR)) {
		if (mh.modificationType & SC_MOD_CHANGESTYLE) {
			pdoc->IncrementStyleClock();
		}
		if (paintState == notPainting) {
			if (mh.position < pdoc->LineStart(topLine)) {
				// Styling performed before this view
				Redraw();
			} else {
				InvalidateRange(mh.position, mh.position + mh.length);
			}
		}
		if (mh.modificationType & SC_MOD_CHANGESTYLE) {
			llc.Invalidate(LineLayout::llCheckTextAndStyle);
		}
	} else {
		// Move selection and brace highlights
		if (mh.modificationType & SC_MOD_INSERTTEXT) {
			sel.MovePositions(true, mh.position, mh.length);
			braces[0] = MovePositionForInsertion(braces[0], mh.position, mh.length);
			braces[1] = MovePositionForInsertion(braces[1], mh.position, mh.length);
		} else if (mh.modificationType & SC_MOD_DELETETEXT) {
			sel.MovePositions(false, mh.position, mh.length);
			braces[0] = MovePositionForDeletion(braces[0], mh.position, mh.length);
			braces[1] = MovePositionForDeletion(braces[1], mh.position, mh.length);
		}
		if ((mh.modificationType & (SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE)) && cs.HiddenLines()) {
			// Some lines are hidden so may need shown.
			// TODO: check if the modified area is hidden.
			if (mh.modificationType & SC_MOD_BEFOREINSERT) {
				int lineOfPos = pdoc->LineFromPosition(mh.position);
				bool insertingNewLine = false;
				for (int i=0; i < mh.length; i++) {
					if ((mh.text[i] == '\n') || (mh.text[i] == '\r'))
						insertingNewLine = true;
				}
				if (insertingNewLine && (mh.position != pdoc->LineStart(lineOfPos)))
					NeedShown(mh.position, pdoc->LineStart(lineOfPos+1) - mh.position);
				else
					NeedShown(mh.position, 0);
			} else if (mh.modificationType & SC_MOD_BEFOREDELETE) {
				NeedShown(mh.position, mh.length);
			}
		}
		if (mh.linesAdded != 0) {
			// Update contraction state for inserted and removed lines
			// lineOfPos should be calculated in context of state before modification, shouldn't it
			int lineOfPos = pdoc->LineFromPosition(mh.position);
			if (mh.position > pdoc->LineStart(lineOfPos))
				lineOfPos++;	// Affecting subsequent lines
			if (mh.linesAdded > 0) {
				cs.InsertLines(lineOfPos, mh.linesAdded);
			} else {
				cs.DeleteLines(lineOfPos, -mh.linesAdded);
			}
		}
		if (mh.modificationType & SC_MOD_CHANGEANNOTATION) {
			int lineDoc = pdoc->LineFromPosition(mh.position);
			if (vs.annotationVisible) {
				cs.SetHeight(lineDoc, cs.GetHeight(lineDoc) + mh.annotationLinesAdded);
				Redraw();
			}
		}
		CheckModificationForWrap(mh);
		if (mh.linesAdded != 0) {
			// Avoid scrolling of display if change before current display
			if (mh.position < posTopLine && !CanDeferToLastStep(mh)) {
				int newTop = Platform::Clamp(topLine + mh.linesAdded, 0, MaxScrollPos());
				if (newTop != topLine) {
					SetTopLine(newTop);
					SetVerticalScrollPos();
				}
			}

			if (paintState == notPainting && !CanDeferToLastStep(mh)) {
				QueueIdleWork(WorkNeeded::workStyle, pdoc->Length());
				Redraw();
			}
		} else {
			if (paintState == notPainting && mh.length && !CanEliminate(mh)) {
				QueueIdleWork(WorkNeeded::workStyle, mh.position + mh.length);
				InvalidateRange(mh.position, mh.position + mh.length);
			}
		}
	}

	if (mh.linesAdded != 0 && !CanDeferToLastStep(mh)) {
		SetScrollBars();
	}

	if ((mh.modificationType & SC_MOD_CHANGEMARKER) || (mh.modificationType & SC_MOD_CHANGEMARGIN)) {
		if ((!willRedrawAll) && ((paintState == notPainting) || !PaintContainsMargin())) {
			if (mh.modificationType & SC_MOD_CHANGEFOLD) {
				// Fold changes can affect the drawing of following lines so redraw whole margin
				RedrawSelMargin(highlightDelimiter.isEnabled ? -1 : mh.line-1, true);
			} else {
				RedrawSelMargin(mh.line);
			}
		}
	}
	if ((mh.modificationType & SC_MOD_CHANGEFOLD) && (foldAutomatic & SC_AUTOMATICFOLD_CHANGE)) {
		FoldChanged(mh.line, mh.foldLevelNow, mh.foldLevelPrev);
	}

	// NOW pay the piper WRT "deferred" visual updates
	if (IsLastStep(mh)) {
		SetScrollBars();
		Redraw();
	}

	// If client wants to see this modification
	if (mh.modificationType & modEventMask) {
		if ((mh.modificationType & (SC_MOD_CHANGESTYLE | SC_MOD_CHANGEINDICATOR)) == 0) {
			// Real modification made to text of document.
			NotifyChange();	// Send EN_CHANGE
		}

		SCNotification scn = {0};
		scn.nmhdr.code = SCN_MODIFIED;
		scn.position = mh.position;
		scn.modificationType = mh.modificationType;
		scn.text = mh.text;
		scn.length = mh.length;
		scn.linesAdded = mh.linesAdded;
		scn.line = mh.line;
		scn.foldLevelNow = mh.foldLevelNow;
		scn.foldLevelPrev = mh.foldLevelPrev;
		scn.token = mh.token;
		scn.annotationLinesAdded = mh.annotationLinesAdded;
		NotifyParent(scn);
	}
}

void Editor::NotifyDeleted(Document *, void *) {
	/* Do nothing */
}

void Editor::NotifyMacroRecord(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {

	// Enumerates all macroable messages
	switch (iMessage) {
	case SCI_CUT:
	case SCI_COPY:
	case SCI_PASTE:
	case SCI_CLEAR:
	case SCI_REPLACESEL:
	case SCI_ADDTEXT:
	case SCI_INSERTTEXT:
	case SCI_APPENDTEXT:
	case SCI_CLEARALL:
	case SCI_SELECTALL:
	case SCI_GOTOLINE:
	case SCI_GOTOPOS:
	case SCI_SEARCHANCHOR:
	case SCI_SEARCHNEXT:
	case SCI_SEARCHPREV:
	case SCI_LINEDOWN:
	case SCI_LINEDOWNEXTEND:
	case SCI_PARADOWN:
	case SCI_PARADOWNEXTEND:
	case SCI_LINEUP:
	case SCI_LINEUPEXTEND:
	case SCI_PARAUP:
	case SCI_PARAUPEXTEND:
	case SCI_CHARLEFT:
	case SCI_CHARLEFTEXTEND:
	case SCI_CHARRIGHT:
	case SCI_CHARRIGHTEXTEND:
	case SCI_WORDLEFT:
	case SCI_WORDLEFTEXTEND:
	case SCI_WORDRIGHT:
	case SCI_WORDRIGHTEXTEND:
	case SCI_WORDPARTLEFT:
	case SCI_WORDPARTLEFTEXTEND:
	case SCI_WORDPARTRIGHT:
	case SCI_WORDPARTRIGHTEXTEND:
	case SCI_WORDLEFTEND:
	case SCI_WORDLEFTENDEXTEND:
	case SCI_WORDRIGHTEND:
	case SCI_WORDRIGHTENDEXTEND:
	case SCI_HOME:
	case SCI_HOMEEXTEND:
	case SCI_LINEEND:
	case SCI_LINEENDEXTEND:
	case SCI_HOMEWRAP:
	case SCI_HOMEWRAPEXTEND:
	case SCI_LINEENDWRAP:
	case SCI_LINEENDWRAPEXTEND:
	case SCI_DOCUMENTSTART:
	case SCI_DOCUMENTSTARTEXTEND:
	case SCI_DOCUMENTEND:
	case SCI_DOCUMENTENDEXTEND:
	case SCI_STUTTEREDPAGEUP:
	case SCI_STUTTEREDPAGEUPEXTEND:
	case SCI_STUTTEREDPAGEDOWN:
	case SCI_STUTTEREDPAGEDOWNEXTEND:
	case SCI_PAGEUP:
	case SCI_PAGEUPEXTEND:
	case SCI_PAGEDOWN:
	case SCI_PAGEDOWNEXTEND:
	case SCI_EDITTOGGLEOVERTYPE:
	case SCI_CANCEL:
	case SCI_DELETEBACK:
	case SCI_TAB:
	case SCI_BACKTAB:
	case SCI_FORMFEED:
	case SCI_VCHOME:
	case SCI_VCHOMEEXTEND:
	case SCI_VCHOMEWRAP:
	case SCI_VCHOMEWRAPEXTEND:
	case SCI_VCHOMEDISPLAY:
	case SCI_VCHOMEDISPLAYEXTEND:
	case SCI_DELWORDLEFT:
	case SCI_DELWORDRIGHT:
	case SCI_DELWORDRIGHTEND:
	case SCI_DELLINELEFT:
	case SCI_DELLINERIGHT:
	case SCI_LINECOPY:
	case SCI_LINECUT:
	case SCI_LINEDELETE:
	case SCI_LINETRANSPOSE:
	case SCI_LINEDUPLICATE:
	case SCI_LOWERCASE:
	case SCI_UPPERCASE:
	case SCI_LINESCROLLDOWN:
	case SCI_LINESCROLLUP:
	case SCI_DELETEBACKNOTLINE:
	case SCI_HOMEDISPLAY:
	case SCI_HOMEDISPLAYEXTEND:
	case SCI_LINEENDDISPLAY:
	case SCI_LINEENDDISPLAYEXTEND:
	case SCI_SETSELECTIONMODE:
	case SCI_LINEDOWNRECTEXTEND:
	case SCI_LINEUPRECTEXTEND:
	case SCI_CHARLEFTRECTEXTEND:
	case SCI_CHARRIGHTRECTEXTEND:
	case SCI_HOMERECTEXTEND:
	case SCI_VCHOMERECTEXTEND:
	case SCI_LINEENDRECTEXTEND:
	case SCI_PAGEUPRECTEXTEND:
	case SCI_PAGEDOWNRECTEXTEND:
	case SCI_SELECTIONDUPLICATE:
	case SCI_COPYALLOWLINE:
	case SCI_VERTICALCENTRECARET:
	case SCI_MOVESELECTEDLINESUP:
	case SCI_MOVESELECTEDLINESDOWN:
	case SCI_SCROLLTOSTART:
	case SCI_SCROLLTOEND:
		break;

		// Filter out all others like display changes. Also, newlines are redundant
		// with char insert messages.
	case SCI_NEWLINE:
	default:
		//		printf("Filtered out %ld of macro recording\n", iMessage);
		return ;
	}

	// Send notification
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_MACRORECORD;
	scn.message = iMessage;
	scn.wParam = wParam;
	scn.lParam = lParam;
	NotifyParent(scn);
}

// Something has changed that the container should know about
void Editor::ContainerNeedsUpdate(int flags) {
	needUpdateUI |= flags;
}

/**
 * Force scroll and keep position relative to top of window.
 *
 * If stuttered = true and not already at first/last row, move to first/last row of window.
 * If stuttered = true and already at first/last row, scroll as normal.
 */
void Editor::PageMove(int direction, Selection::selTypes selt, bool stuttered) {
	int topLineNew;
	SelectionPosition newPos;

	int currentLine = pdoc->LineFromPosition(sel.MainCaret());
	int topStutterLine = topLine + caretYSlop;
	int bottomStutterLine =
	    pdoc->LineFromPosition(PositionFromLocation(
	                Point(lastXChosen - xOffset, direction * vs.lineHeight * LinesToScroll())))
	    - caretYSlop - 1;

	if (stuttered && (direction < 0 && currentLine > topStutterLine)) {
		topLineNew = topLine;
		newPos = SPositionFromLocation(Point(lastXChosen - xOffset, vs.lineHeight * caretYSlop),
			false, false, UserVirtualSpace());

	} else if (stuttered && (direction > 0 && currentLine < bottomStutterLine)) {
		topLineNew = topLine;
		newPos = SPositionFromLocation(Point(lastXChosen - xOffset, vs.lineHeight * (LinesToScroll() - caretYSlop)),
			false, false, UserVirtualSpace());

	} else {
		Point pt = LocationFromPosition(sel.MainCaret());

		topLineNew = Platform::Clamp(
		            topLine + direction * LinesToScroll(), 0, MaxScrollPos());
		newPos = SPositionFromLocation(
			Point(lastXChosen - xOffset, pt.y + direction * (vs.lineHeight * LinesToScroll())),
			false, false, UserVirtualSpace());
	}

	if (topLineNew != topLine) {
		SetTopLine(topLineNew);
		MovePositionTo(newPos, selt);
		Redraw();
		SetVerticalScrollPos();
	} else {
		MovePositionTo(newPos, selt);
	}
}

void Editor::ChangeCaseOfSelection(int caseMapping) {
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		SelectionRange current = sel.Range(r);
		SelectionRange currentNoVS = current;
		currentNoVS.ClearVirtualSpace();
		size_t rangeBytes = currentNoVS.Length();
		if (rangeBytes > 0) {
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
				size_t endDifferenceText = sText.size() - 1 - lastDifferenceText;
				pdoc->DeleteChars(
					static_cast<int>(currentNoVS.Start().Position() + firstDifference),
					static_cast<int>(rangeBytes - firstDifference - endDifferenceText));
				pdoc->InsertString(
					static_cast<int>(currentNoVS.Start().Position() + firstDifference),
					sMapped.c_str() + firstDifference,
					static_cast<int>(lastDifferenceMapped - firstDifference + 1));
				// Automatic movement changes selection so reset to exactly the same as it was.
				int diffSizes = static_cast<int>(sMapped.size() - sText.size());
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

void Editor::LineTranspose() {
	int line = pdoc->LineFromPosition(sel.MainCaret());
	if (line > 0) {
		UndoGroup ug(pdoc);
		int startPrev = pdoc->LineStart(line - 1);
		int endPrev = pdoc->LineEnd(line - 1);
		int start = pdoc->LineStart(line);
		int end = pdoc->LineEnd(line);
		std::string line1 = RangeText(startPrev, endPrev);
		int len1 = endPrev - startPrev;
		std::string line2 = RangeText(start, end);
		int len2 = end - start;
		pdoc->DeleteChars(start, len2);
		pdoc->DeleteChars(startPrev, len1);
		pdoc->InsertString(startPrev, line2.c_str(), len2);
		pdoc->InsertString(start - len1 + len2, line1.c_str(), len1);
		MovePositionTo(SelectionPosition(start - len1 + len2));
	}
}

void Editor::Duplicate(bool forLine) {
	if (sel.Empty()) {
		forLine = true;
	}
	UndoGroup ug(pdoc);
	const char *eol = "";
	int eolLen = 0;
	if (forLine) {
		eol = StringFromEOLMode(pdoc->eolMode);
		eolLen = istrlen(eol);
	}
	for (size_t r=0; r<sel.Count(); r++) {
		SelectionPosition start = sel.Range(r).Start();
		SelectionPosition end = sel.Range(r).End();
		if (forLine) {
			int line = pdoc->LineFromPosition(sel.Range(r).caret.Position());
			start = SelectionPosition(pdoc->LineStart(line));
			end = SelectionPosition(pdoc->LineEnd(line));
		}
		std::string text = RangeText(start.Position(), end.Position());
		if (forLine)
			pdoc->InsertString(end.Position(), eol, eolLen);
		pdoc->InsertString(end.Position() + eolLen, text.c_str(), SelectionRange(end, start).Length());
	}
	if (sel.Count() && sel.IsRectangular()) {
		SelectionPosition last = sel.Last();
		if (forLine) {
			int line = pdoc->LineFromPosition(last.Position());
			last = SelectionPosition(last.Position() + pdoc->LineStart(line+1) - pdoc->LineStart(line));
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
	// Remove non-main ranges
	InvalidateSelection(sel.RangeMain(), true);
	sel.SetSelection(sel.RangeMain());

	// Clear main range and insert line end
	bool needGroupUndo = !sel.Empty();
	if (needGroupUndo)
		pdoc->BeginUndoAction();

	if (!sel.Empty())
		ClearSelection();
	const char *eol = "\n";
	if (pdoc->eolMode == SC_EOL_CRLF) {
		eol = "\r\n";
	} else if (pdoc->eolMode == SC_EOL_CR) {
		eol = "\r";
	} // else SC_EOL_LF -> "\n" already set
	bool inserted = pdoc->InsertCString(sel.MainCaret(), eol);
	// Want to end undo group before NotifyChar as applications often modify text here
	if (needGroupUndo)
		pdoc->EndUndoAction();
	if (inserted) {
		SetEmptySelection(sel.MainCaret() + istrlen(eol));
		while (*eol) {
			NotifyChar(*eol);
			if (recordingMacro) {
				char txt[2];
				txt[0] = *eol;
				txt[1] = '\0';
				NotifyMacroRecord(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(txt));
			}
			eol++;
		}
	}
	SetLastXChosen();
	SetScrollBars();
	EnsureCaretVisible();
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

void Editor::CursorUpOrDown(int direction, Selection::selTypes selt) {
	SelectionPosition caretToUse = sel.Range(sel.Main()).caret;
	if (sel.IsRectangular()) {
		if (selt ==  Selection::noSel) {
			caretToUse = (direction > 0) ? sel.Limits().end : sel.Limits().start;
		} else {
			caretToUse = sel.Rectangular().caret;
		}
	}

	Point pt = LocationFromPosition(caretToUse);
	int skipLines = 0;

	if (vs.annotationVisible) {
		int lineDoc = pdoc->LineFromPosition(caretToUse.Position());
		Point ptStartLine = LocationFromPosition(pdoc->LineStart(lineDoc));
		int subLine = (pt.y - ptStartLine.y) / vs.lineHeight;

		if (direction < 0 && subLine == 0) {
			int lineDisplay = cs.DisplayFromDoc(lineDoc);
			if (lineDisplay > 0) {
				skipLines = pdoc->AnnotationLines(cs.DocFromDisplay(lineDisplay - 1));
			}
		} else if (direction > 0 && subLine >= (cs.GetHeight(lineDoc) - 1 - pdoc->AnnotationLines(lineDoc))) {
			skipLines = pdoc->AnnotationLines(lineDoc);
		}
	}

	int newY = pt.y + (1 + skipLines) * direction * vs.lineHeight;
	SelectionPosition posNew = SPositionFromLocation(
	            Point(lastXChosen - xOffset, newY), false, false, UserVirtualSpace());

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
		while ((posNew.Position() > caretToUse.Position()) && (ptNew.y > newY)) {
			posNew.Add(-1);
			posNew.SetVirtualSpace(0);
			ptNew = LocationFromPosition(posNew.Position());
		}
	}

	MovePositionTo(MovePositionSoVisible(posNew, direction), selt);
}

void Editor::ParaUpOrDown(int direction, Selection::selTypes selt) {
	int lineDoc, savedPos = sel.MainCaret();
	do {
		MovePositionTo(SelectionPosition(direction > 0 ? pdoc->ParaDown(sel.MainCaret()) : pdoc->ParaUp(sel.MainCaret())), selt);
		lineDoc = pdoc->LineFromPosition(sel.MainCaret());
		if (direction > 0) {
			if (sel.MainCaret() >= pdoc->Length() && !cs.GetVisible(lineDoc)) {
				if (selt == Selection::noSel) {
					MovePositionTo(SelectionPosition(pdoc->LineEndPosition(savedPos)));
				}
				break;
			}
		}
	} while (!cs.GetVisible(lineDoc));
}

int Editor::StartEndDisplayLine(int pos, bool start) {
	RefreshStyleData();
	int line = pdoc->LineFromPosition(pos);
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(line));
	int posRet = INVALID_POSITION;
	if (surface && ll) {
		unsigned int posLineStart = pdoc->LineStart(line);
		LayoutLine(line, surface, vs, ll, wrapWidth);
		int posInLine = pos - posLineStart;
		if (posInLine <= ll->maxLineLength) {
			for (int subLine = 0; subLine < ll->lines; subLine++) {
				if ((posInLine >= ll->LineStart(subLine)) && (posInLine <= ll->LineStart(subLine + 1))) {
					if (start) {
						posRet = ll->LineStart(subLine) + posLineStart;
					} else {
						if (subLine == ll->lines - 1)
							posRet = ll->LineStart(subLine + 1) + posLineStart;
						else
							posRet = ll->LineStart(subLine + 1) + posLineStart - 1;
					}
				}
			}
		}
	}
	if (posRet == INVALID_POSITION) {
		return pos;
	} else {
		return posRet;
	}
}

int Editor::KeyCommand(unsigned int iMessage) {
	switch (iMessage) {
	case SCI_LINEDOWN:
		CursorUpOrDown(1);
		break;
	case SCI_LINEDOWNEXTEND:
		CursorUpOrDown(1, Selection::selStream);
		break;
	case SCI_LINEDOWNRECTEXTEND:
		CursorUpOrDown(1, Selection::selRectangle);
		break;
	case SCI_PARADOWN:
		ParaUpOrDown(1);
		break;
	case SCI_PARADOWNEXTEND:
		ParaUpOrDown(1, Selection::selStream);
		break;
	case SCI_LINESCROLLDOWN:
		ScrollTo(topLine + 1);
		MoveCaretInsideView(false);
		break;
	case SCI_LINEUP:
		CursorUpOrDown(-1);
		break;
	case SCI_LINEUPEXTEND:
		CursorUpOrDown(-1, Selection::selStream);
		break;
	case SCI_LINEUPRECTEXTEND:
		CursorUpOrDown(-1, Selection::selRectangle);
		break;
	case SCI_PARAUP:
		ParaUpOrDown(-1);
		break;
	case SCI_PARAUPEXTEND:
		ParaUpOrDown(-1, Selection::selStream);
		break;
	case SCI_LINESCROLLUP:
		ScrollTo(topLine - 1);
		MoveCaretInsideView(false);
		break;
	case SCI_CHARLEFT:
		if (SelectionEmpty() || sel.MoveExtends()) {
			if ((sel.Count() == 1) && pdoc->IsLineEndPosition(sel.MainCaret()) && sel.RangeMain().caret.VirtualSpace()) {
				SelectionPosition spCaret = sel.RangeMain().caret;
				spCaret.SetVirtualSpace(spCaret.VirtualSpace() - 1);
				MovePositionTo(spCaret);
			} else if (sel.MoveExtends() && sel.selType == Selection::selStream) {
				MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() - 1), -1));
			} else {
				MovePositionTo(MovePositionSoVisible(
					SelectionPosition((sel.LimitsForRectangularElseMain().start).Position() - 1), -1));
			}
		} else {
			MovePositionTo(sel.LimitsForRectangularElseMain().start);
		}
		SetLastXChosen();
		break;
	case SCI_CHARLEFTEXTEND:
		if (pdoc->IsLineEndPosition(sel.MainCaret()) && sel.RangeMain().caret.VirtualSpace()) {
			SelectionPosition spCaret = sel.RangeMain().caret;
			spCaret.SetVirtualSpace(spCaret.VirtualSpace() - 1);
			MovePositionTo(spCaret, Selection::selStream);
		} else {
			MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() - 1), -1), Selection::selStream);
		}
		SetLastXChosen();
		break;
	case SCI_CHARLEFTRECTEXTEND:
		if (pdoc->IsLineEndPosition(sel.MainCaret()) && sel.RangeMain().caret.VirtualSpace()) {
			SelectionPosition spCaret = sel.RangeMain().caret;
			spCaret.SetVirtualSpace(spCaret.VirtualSpace() - 1);
			MovePositionTo(spCaret, Selection::selRectangle);
		} else {
			MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() - 1), -1), Selection::selRectangle);
		}
		SetLastXChosen();
		break;
	case SCI_CHARRIGHT:
		if (SelectionEmpty() || sel.MoveExtends()) {
			if ((virtualSpaceOptions & SCVS_USERACCESSIBLE) && pdoc->IsLineEndPosition(sel.MainCaret())) {
				SelectionPosition spCaret = sel.RangeMain().caret;
				spCaret.SetVirtualSpace(spCaret.VirtualSpace() + 1);
				MovePositionTo(spCaret);
			} else if (sel.MoveExtends() && sel.selType == Selection::selStream) {
				MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() + 1), 1));
			} else {
				MovePositionTo(MovePositionSoVisible(
					SelectionPosition((sel.LimitsForRectangularElseMain().end).Position() + 1), 1));
			}
		} else {
			MovePositionTo(sel.LimitsForRectangularElseMain().end);
		}
		SetLastXChosen();
		break;
	case SCI_CHARRIGHTEXTEND:
		if ((virtualSpaceOptions & SCVS_USERACCESSIBLE) && pdoc->IsLineEndPosition(sel.MainCaret())) {
			SelectionPosition spCaret = sel.RangeMain().caret;
			spCaret.SetVirtualSpace(spCaret.VirtualSpace() + 1);
			MovePositionTo(spCaret, Selection::selStream);
		} else {
			MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() + 1), 1), Selection::selStream);
		}
		SetLastXChosen();
		break;
	case SCI_CHARRIGHTRECTEXTEND:
		if ((virtualSpaceOptions & SCVS_RECTANGULARSELECTION) && pdoc->IsLineEndPosition(sel.MainCaret())) {
			SelectionPosition spCaret = sel.RangeMain().caret;
			spCaret.SetVirtualSpace(spCaret.VirtualSpace() + 1);
			MovePositionTo(spCaret, Selection::selRectangle);
		} else {
			MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() + 1), 1), Selection::selRectangle);
		}
		SetLastXChosen();
		break;
	case SCI_WORDLEFT:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(sel.MainCaret(), -1), -1));
		SetLastXChosen();
		break;
	case SCI_WORDLEFTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(sel.MainCaret(), -1), -1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_WORDRIGHT:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(sel.MainCaret(), 1), 1));
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(sel.MainCaret(), 1), 1), Selection::selStream);
		SetLastXChosen();
		break;

	case SCI_WORDLEFTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(sel.MainCaret(), -1), -1));
		SetLastXChosen();
		break;
	case SCI_WORDLEFTENDEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(sel.MainCaret(), -1), -1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(sel.MainCaret(), 1), 1));
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTENDEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(sel.MainCaret(), 1), 1), Selection::selStream);
		SetLastXChosen();
		break;

	case SCI_HOME:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())));
		SetLastXChosen();
		break;
	case SCI_HOMEEXTEND:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_HOMERECTEXTEND:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())), Selection::selRectangle);
		SetLastXChosen();
		break;
	case SCI_LINEEND:
		MovePositionTo(pdoc->LineEndPosition(sel.MainCaret()));
		SetLastXChosen();
		break;
	case SCI_LINEENDEXTEND:
		MovePositionTo(pdoc->LineEndPosition(sel.MainCaret()), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_LINEENDRECTEXTEND:
		MovePositionTo(pdoc->LineEndPosition(sel.MainCaret()), Selection::selRectangle);
		SetLastXChosen();
		break;
	case SCI_HOMEWRAP: {
			SelectionPosition homePos = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if (sel.RangeMain().caret <= homePos)
				homePos = SelectionPosition(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())));
			MovePositionTo(homePos);
			SetLastXChosen();
		}
		break;
	case SCI_HOMEWRAPEXTEND: {
			SelectionPosition homePos = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if (sel.RangeMain().caret <= homePos)
				homePos = SelectionPosition(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())));
			MovePositionTo(homePos, Selection::selStream);
			SetLastXChosen();
		}
		break;
	case SCI_LINEENDWRAP: {
			SelectionPosition endPos = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), false), 1);
			SelectionPosition realEndPos = SelectionPosition(pdoc->LineEndPosition(sel.MainCaret()));
			if (endPos > realEndPos      // if moved past visible EOLs
			        || sel.RangeMain().caret >= endPos) // if at end of display line already
				endPos = realEndPos;
			MovePositionTo(endPos);
			SetLastXChosen();
		}
		break;
	case SCI_LINEENDWRAPEXTEND: {
			SelectionPosition endPos = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), false), 1);
			SelectionPosition realEndPos = SelectionPosition(pdoc->LineEndPosition(sel.MainCaret()));
			if (endPos > realEndPos      // if moved past visible EOLs
			        || sel.RangeMain().caret >= endPos) // if at end of display line already
				endPos = realEndPos;
			MovePositionTo(endPos, Selection::selStream);
			SetLastXChosen();
		}
		break;
	case SCI_DOCUMENTSTART:
		MovePositionTo(0);
		SetLastXChosen();
		break;
	case SCI_DOCUMENTSTARTEXTEND:
		MovePositionTo(0, Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_DOCUMENTEND:
		MovePositionTo(pdoc->Length());
		SetLastXChosen();
		break;
	case SCI_DOCUMENTENDEXTEND:
		MovePositionTo(pdoc->Length(), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_STUTTEREDPAGEUP:
		PageMove(-1, Selection::noSel, true);
		break;
	case SCI_STUTTEREDPAGEUPEXTEND:
		PageMove(-1, Selection::selStream, true);
		break;
	case SCI_STUTTEREDPAGEDOWN:
		PageMove(1, Selection::noSel, true);
		break;
	case SCI_STUTTEREDPAGEDOWNEXTEND:
		PageMove(1, Selection::selStream, true);
		break;
	case SCI_PAGEUP:
		PageMove(-1);
		break;
	case SCI_PAGEUPEXTEND:
		PageMove(-1, Selection::selStream);
		break;
	case SCI_PAGEUPRECTEXTEND:
		PageMove(-1, Selection::selRectangle);
		break;
	case SCI_PAGEDOWN:
		PageMove(1);
		break;
	case SCI_PAGEDOWNEXTEND:
		PageMove(1, Selection::selStream);
		break;
	case SCI_PAGEDOWNRECTEXTEND:
		PageMove(1, Selection::selRectangle);
		break;
	case SCI_EDITTOGGLEOVERTYPE:
		inOverstrike = !inOverstrike;
		DropCaret();
		ShowCaretAtCurrentPosition();
		ContainerNeedsUpdate(SC_UPDATE_CONTENT);
		NotifyUpdateUI();
		break;
	case SCI_CANCEL:            	// Cancel any modes - handled in subclass
		// Also unselect text
		CancelModes();
		break;
	case SCI_DELETEBACK:
		DelCharBack(true);
		if ((caretSticky == SC_CARETSTICKY_OFF) || (caretSticky == SC_CARETSTICKY_WHITESPACE)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case SCI_DELETEBACKNOTLINE:
		DelCharBack(false);
		if ((caretSticky == SC_CARETSTICKY_OFF) || (caretSticky == SC_CARETSTICKY_WHITESPACE)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case SCI_TAB:
		Indent(true);
		if (caretSticky == SC_CARETSTICKY_OFF) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		ShowCaretAtCurrentPosition();		// Avoid blinking
		break;
	case SCI_BACKTAB:
		Indent(false);
		if ((caretSticky == SC_CARETSTICKY_OFF) || (caretSticky == SC_CARETSTICKY_WHITESPACE)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		ShowCaretAtCurrentPosition();		// Avoid blinking
		break;
	case SCI_NEWLINE:
		NewLine();
		break;
	case SCI_FORMFEED:
		AddChar('\f');
		break;
	case SCI_VCHOME:
		MovePositionTo(pdoc->VCHomePosition(sel.MainCaret()));
		SetLastXChosen();
		break;
	case SCI_VCHOMEEXTEND:
		MovePositionTo(pdoc->VCHomePosition(sel.MainCaret()), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_VCHOMERECTEXTEND:
		MovePositionTo(pdoc->VCHomePosition(sel.MainCaret()), Selection::selRectangle);
		SetLastXChosen();
		break;
	case SCI_VCHOMEWRAP: {
			SelectionPosition homePos = SelectionPosition(pdoc->VCHomePosition(sel.MainCaret()));
			SelectionPosition viewLineStart = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if ((viewLineStart < sel.RangeMain().caret) && (viewLineStart > homePos))
				homePos = viewLineStart;

			MovePositionTo(homePos);
			SetLastXChosen();
		}
		break;
	case SCI_VCHOMEWRAPEXTEND: {
			SelectionPosition homePos = SelectionPosition(pdoc->VCHomePosition(sel.MainCaret()));
			SelectionPosition viewLineStart = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if ((viewLineStart < sel.RangeMain().caret) && (viewLineStart > homePos))
				homePos = viewLineStart;

			MovePositionTo(homePos, Selection::selStream);
			SetLastXChosen();
		}
		break;
	case SCI_ZOOMIN:
		if (vs.zoomLevel < 20) {
			vs.zoomLevel++;
			InvalidateStyleRedraw();
			NotifyZoom();
		}
		break;
	case SCI_ZOOMOUT:
		if (vs.zoomLevel > -10) {
			vs.zoomLevel--;
			InvalidateStyleRedraw();
			NotifyZoom();
		}
		break;
	case SCI_DELWORDLEFT: {
			int startWord = pdoc->NextWordStart(sel.MainCaret(), -1);
			pdoc->DeleteChars(startWord, sel.MainCaret() - startWord);
			sel.RangeMain().ClearVirtualSpace();
			SetLastXChosen();
		}
		break;
	case SCI_DELWORDRIGHT: {
			UndoGroup ug(pdoc);
			sel.RangeMain().caret = SelectionPosition(
				InsertSpace(sel.RangeMain().caret.Position(), sel.RangeMain().caret.VirtualSpace()));
			sel.RangeMain().anchor = sel.RangeMain().caret;
			int endWord = pdoc->NextWordStart(sel.MainCaret(), 1);
			pdoc->DeleteChars(sel.MainCaret(), endWord - sel.MainCaret());
		}
		break;
	case SCI_DELWORDRIGHTEND: {
			UndoGroup ug(pdoc);
			sel.RangeMain().caret = SelectionPosition(
				InsertSpace(sel.RangeMain().caret.Position(), sel.RangeMain().caret.VirtualSpace()));
			int endWord = pdoc->NextWordEnd(sel.MainCaret(), 1);
			pdoc->DeleteChars(sel.MainCaret(), endWord - sel.MainCaret());
		}
		break;
	case SCI_DELLINELEFT: {
			int line = pdoc->LineFromPosition(sel.MainCaret());
			int start = pdoc->LineStart(line);
			pdoc->DeleteChars(start, sel.MainCaret() - start);
			sel.RangeMain().ClearVirtualSpace();
			SetLastXChosen();
		}
		break;
	case SCI_DELLINERIGHT: {
			int line = pdoc->LineFromPosition(sel.MainCaret());
			int end = pdoc->LineEnd(line);
			pdoc->DeleteChars(sel.MainCaret(), end - sel.MainCaret());
		}
		break;
	case SCI_LINECOPY: {
			int lineStart = pdoc->LineFromPosition(SelectionStart().Position());
			int lineEnd = pdoc->LineFromPosition(SelectionEnd().Position());
			CopyRangeToClipboard(pdoc->LineStart(lineStart),
			        pdoc->LineStart(lineEnd + 1));
		}
		break;
	case SCI_LINECUT: {
			int lineStart = pdoc->LineFromPosition(SelectionStart().Position());
			int lineEnd = pdoc->LineFromPosition(SelectionEnd().Position());
			int start = pdoc->LineStart(lineStart);
			int end = pdoc->LineStart(lineEnd + 1);
			SetSelection(start, end);
			Cut();
			SetLastXChosen();
		}
		break;
	case SCI_LINEDELETE: {
			int line = pdoc->LineFromPosition(sel.MainCaret());
			int start = pdoc->LineStart(line);
			int end = pdoc->LineStart(line + 1);
			pdoc->DeleteChars(start, end - start);
		}
		break;
	case SCI_LINETRANSPOSE:
		LineTranspose();
		break;
	case SCI_LINEDUPLICATE:
		Duplicate(true);
		break;
	case SCI_SELECTIONDUPLICATE:
		Duplicate(false);
		break;
	case SCI_LOWERCASE:
		ChangeCaseOfSelection(cmLower);
		break;
	case SCI_UPPERCASE:
		ChangeCaseOfSelection(cmUpper);
		break;
	case SCI_WORDPARTLEFT:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartLeft(sel.MainCaret()), -1));
		SetLastXChosen();
		break;
	case SCI_WORDPARTLEFTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartLeft(sel.MainCaret()), -1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_WORDPARTRIGHT:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartRight(sel.MainCaret()), 1));
		SetLastXChosen();
		break;
	case SCI_WORDPARTRIGHTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartRight(sel.MainCaret()), 1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_HOMEDISPLAY:
		MovePositionTo(MovePositionSoVisible(
		            StartEndDisplayLine(sel.MainCaret(), true), -1));
		SetLastXChosen();
		break;
	case SCI_VCHOMEDISPLAY: {
			SelectionPosition homePos = SelectionPosition(pdoc->VCHomePosition(sel.MainCaret()));
			SelectionPosition viewLineStart = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if (viewLineStart > homePos)
				homePos = viewLineStart;

			MovePositionTo(homePos);
			SetLastXChosen();
		}
		break;
	case SCI_HOMEDISPLAYEXTEND:
		MovePositionTo(MovePositionSoVisible(
		            StartEndDisplayLine(sel.MainCaret(), true), -1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_VCHOMEDISPLAYEXTEND: {
			SelectionPosition homePos = SelectionPosition(pdoc->VCHomePosition(sel.MainCaret()));
			SelectionPosition viewLineStart = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if (viewLineStart > homePos)
				homePos = viewLineStart;

			MovePositionTo(homePos, Selection::selStream);
			SetLastXChosen();
		}
		break;
	case SCI_LINEENDDISPLAY:
		MovePositionTo(MovePositionSoVisible(
		            StartEndDisplayLine(sel.MainCaret(), false), 1));
		SetLastXChosen();
		break;
	case SCI_LINEENDDISPLAYEXTEND:
		MovePositionTo(MovePositionSoVisible(
		            StartEndDisplayLine(sel.MainCaret(), false), 1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_SCROLLTOSTART:
		ScrollTo(0);
		break;
	case SCI_SCROLLTOEND:
		ScrollTo(MaxScrollPos());
		break;
	}
	return 0;
}

int Editor::KeyDefault(int, int) {
	return 0;
}

int Editor::KeyDownWithModifiers(int key, int modifiers, bool *consumed) {
	DwellEnd(false);
	int msg = kmap.Find(key, modifiers);
	if (msg) {
		if (consumed)
			*consumed = true;
		return WndProc(msg, 0, 0);
	} else {
		if (consumed)
			*consumed = false;
		return KeyDefault(key, modifiers);
	}
}

int Editor::KeyDown(int key, bool shift, bool ctrl, bool alt, bool *consumed) {
	int modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) |
	        (alt ? SCI_ALT : 0);
	return KeyDownWithModifiers(key, modifiers, consumed);
}

void Editor::Indent(bool forwards) {
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		int lineOfAnchor = pdoc->LineFromPosition(sel.Range(r).anchor.Position());
		int caretPosition = sel.Range(r).caret.Position();
		int lineCurrentPos = pdoc->LineFromPosition(caretPosition);
		if (lineOfAnchor == lineCurrentPos) {
			if (forwards) {
				pdoc->DeleteChars(sel.Range(r).Start().Position(), sel.Range(r).Length());
				caretPosition = sel.Range(r).caret.Position();
				if (pdoc->GetColumn(caretPosition) <= pdoc->GetColumn(pdoc->GetLineIndentPosition(lineCurrentPos)) &&
						pdoc->tabIndents) {
					int indentation = pdoc->GetLineIndentation(lineCurrentPos);
					int indentationStep = pdoc->IndentSize();
					pdoc->SetLineIndentation(lineCurrentPos, indentation + indentationStep - indentation % indentationStep);
					sel.Range(r) = SelectionRange(pdoc->GetLineIndentPosition(lineCurrentPos));
				} else {
					if (pdoc->useTabs) {
						pdoc->InsertChar(caretPosition, '\t');
						sel.Range(r) = SelectionRange(caretPosition+1);
					} else {
						int numSpaces = (pdoc->tabInChars) -
								(pdoc->GetColumn(caretPosition) % (pdoc->tabInChars));
						if (numSpaces < 1)
							numSpaces = pdoc->tabInChars;
						for (int i = 0; i < numSpaces; i++) {
							pdoc->InsertChar(caretPosition + i, ' ');
						}
						sel.Range(r) = SelectionRange(caretPosition+numSpaces);
					}
				}
			} else {
				if (pdoc->GetColumn(caretPosition) <= pdoc->GetLineIndentation(lineCurrentPos) &&
						pdoc->tabIndents) {
					int indentation = pdoc->GetLineIndentation(lineCurrentPos);
					int indentationStep = pdoc->IndentSize();
					pdoc->SetLineIndentation(lineCurrentPos, indentation - indentationStep);
					sel.Range(r) = SelectionRange(pdoc->GetLineIndentPosition(lineCurrentPos));
				} else {
					int newColumn = ((pdoc->GetColumn(caretPosition) - 1) / pdoc->tabInChars) *
							pdoc->tabInChars;
					if (newColumn < 0)
						newColumn = 0;
					int newPos = caretPosition;
					while (pdoc->GetColumn(newPos) > newColumn)
						newPos--;
					sel.Range(r) = SelectionRange(newPos);
				}
			}
		} else {	// Multiline
			int anchorPosOnLine = sel.Range(r).anchor.Position() - pdoc->LineStart(lineOfAnchor);
			int currentPosPosOnLine = caretPosition - pdoc->LineStart(lineCurrentPos);
			// Multiple lines selected so indent / dedent
			int lineTopSel = Platform::Minimum(lineOfAnchor, lineCurrentPos);
			int lineBottomSel = Platform::Maximum(lineOfAnchor, lineCurrentPos);
			if (pdoc->LineStart(lineBottomSel) == sel.Range(r).anchor.Position() || pdoc->LineStart(lineBottomSel) == caretPosition)
				lineBottomSel--;  	// If not selecting any characters on a line, do not indent
			pdoc->Indent(forwards, lineBottomSel, lineTopSel);
			if (lineOfAnchor < lineCurrentPos) {
				if (currentPosPosOnLine == 0)
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor));
				else
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos + 1), pdoc->LineStart(lineOfAnchor));
			} else {
				if (anchorPosOnLine == 0)
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor));
				else
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor + 1));
			}
		}
	}
}

class CaseFolderASCII : public CaseFolderTable {
public:
	CaseFolderASCII() {
		StandardASCII();
	}
	~CaseFolderASCII() {
	}
};


CaseFolder *Editor::CaseFolderForEncoding() {
	// Simple default that only maps ASCII upper case to lower case.
	return new CaseFolderASCII();
}

/**
 * Search of a text in the document, in the given range.
 * @return The position of the found text, -1 if not found.
 */
long Editor::FindText(
    uptr_t wParam,		///< Search modes : @c SCFIND_MATCHCASE, @c SCFIND_WHOLEWORD,
    ///< @c SCFIND_WORDSTART, @c SCFIND_REGEXP or @c SCFIND_POSIX.
    sptr_t lParam) {	///< @c TextToFind structure: The text to search for in the given range.

	Sci_TextToFind *ft = reinterpret_cast<Sci_TextToFind *>(lParam);
	int lengthFound = istrlen(ft->lpstrText);
	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	int pos = pdoc->FindText(ft->chrg.cpMin, ft->chrg.cpMax, ft->lpstrText,
	        (wParam & SCFIND_MATCHCASE) != 0,
	        (wParam & SCFIND_WHOLEWORD) != 0,
	        (wParam & SCFIND_WORDSTART) != 0,
	        (wParam & SCFIND_REGEXP) != 0,
	        wParam,
	        &lengthFound);
	if (pos != -1) {
		ft->chrgText.cpMin = pos;
		ft->chrgText.cpMax = pos + lengthFound;
	}
	return pos;
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
void Editor::SearchAnchor() {
	searchAnchor = SelectionStart().Position();
}

/**
 * Find text from current search anchor: Must call @c SearchAnchor first.
 * Used for next text and previous text requests.
 * @return The position of the found text, -1 if not found.
 */
long Editor::SearchText(
    unsigned int iMessage,		///< Accepts both @c SCI_SEARCHNEXT and @c SCI_SEARCHPREV.
    uptr_t wParam,				///< Search modes : @c SCFIND_MATCHCASE, @c SCFIND_WHOLEWORD,
    ///< @c SCFIND_WORDSTART, @c SCFIND_REGEXP or @c SCFIND_POSIX.
    sptr_t lParam) {			///< The text to search for.

	const char *txt = reinterpret_cast<char *>(lParam);
	int pos;
	int lengthFound = istrlen(txt);
	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	if (iMessage == SCI_SEARCHNEXT) {
		pos = pdoc->FindText(searchAnchor, pdoc->Length(), txt,
		        (wParam & SCFIND_MATCHCASE) != 0,
		        (wParam & SCFIND_WHOLEWORD) != 0,
		        (wParam & SCFIND_WORDSTART) != 0,
		        (wParam & SCFIND_REGEXP) != 0,
		        wParam,
		        &lengthFound);
	} else {
		pos = pdoc->FindText(searchAnchor, 0, txt,
		        (wParam & SCFIND_MATCHCASE) != 0,
		        (wParam & SCFIND_WHOLEWORD) != 0,
		        (wParam & SCFIND_WORDSTART) != 0,
		        (wParam & SCFIND_REGEXP) != 0,
		        wParam,
		        &lengthFound);
	}
	if (pos != -1) {
		SetSelection(pos, pos + lengthFound);
	}

	return pos;
}

std::string Editor::CaseMapString(const std::string &s, int caseMapping) {
	std::string ret(s);
	for (size_t i=0; i<ret.size(); i++) {
		switch (caseMapping) {
			case cmUpper:
				if (ret[i] >= 'a' && ret[i] <= 'z')
					ret[i] = static_cast<char>(ret[i] - 'a' + 'A');
				break;
			case cmLower:
				if (ret[i] >= 'A' && ret[i] <= 'Z')
					ret[i] = static_cast<char>(ret[i] - 'A' + 'a');
				break;
		}
	}
	return ret;
}

/**
 * Search for text in the target range of the document.
 * @return The position of the found text, -1 if not found.
 */
long Editor::SearchInTarget(const char *text, int length) {
	int lengthFound = length;

	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	int pos = pdoc->FindText(targetStart, targetEnd, text,
	        (searchFlags & SCFIND_MATCHCASE) != 0,
	        (searchFlags & SCFIND_WHOLEWORD) != 0,
	        (searchFlags & SCFIND_WORDSTART) != 0,
	        (searchFlags & SCFIND_REGEXP) != 0,
	        searchFlags,
	        &lengthFound);
	if (pos != -1) {
		targetStart = pos;
		targetEnd = pos + lengthFound;
	}
	return pos;
}

void Editor::GoToLine(int lineNo) {
	if (lineNo > pdoc->LinesTotal())
		lineNo = pdoc->LinesTotal();
	if (lineNo < 0)
		lineNo = 0;
	SetEmptySelection(pdoc->LineStart(lineNo));
	ShowCaretAtCurrentPosition();
	EnsureCaretVisible();
}

static bool Close(Point pt1, Point pt2) {
	if (abs(pt1.x - pt2.x) > 3)
		return false;
	if (abs(pt1.y - pt2.y) > 3)
		return false;
	return true;
}

std::string Editor::RangeText(int start, int end) const {
	if (start < end) {
		int len = end - start;
		std::string ret(len, '\0');
		for (int i = 0; i < len; i++) {
			ret[i] = pdoc->CharAt(start + i);
		}
		return ret;
	}
	return std::string();
}

void Editor::CopySelectionRange(SelectionText *ss, bool allowLineCopy) {
	if (sel.Empty()) {
		if (allowLineCopy) {
			int currentLine = pdoc->LineFromPosition(sel.MainCaret());
			int start = pdoc->LineStart(currentLine);
			int end = pdoc->LineEnd(currentLine);

			std::string text = RangeText(start, end);
			if (pdoc->eolMode != SC_EOL_LF)
				text.push_back('\r');
			if (pdoc->eolMode != SC_EOL_CR)
				text.push_back('\n');
			ss->Copy(text, pdoc->dbcsCodePage,
				vs.styles[STYLE_DEFAULT].characterSet, false, true);
		}
	} else {
		std::string text;
		std::vector<SelectionRange> rangesInOrder = sel.RangesCopy();
		if (sel.selType == Selection::selRectangle)
			std::sort(rangesInOrder.begin(), rangesInOrder.end());
		for (size_t r=0; r<rangesInOrder.size(); r++) {
			SelectionRange current = rangesInOrder[r];
			text.append(RangeText(current.Start().Position(), current.End().Position()));
			if (sel.selType == Selection::selRectangle) {
				if (pdoc->eolMode != SC_EOL_LF)
					text.push_back('\r');
				if (pdoc->eolMode != SC_EOL_CR)
					text.push_back('\n');
			}
		}
		ss->Copy(text, pdoc->dbcsCodePage,
			vs.styles[STYLE_DEFAULT].characterSet, sel.IsRectangular(), sel.selType == Selection::selLines);
	}
}

void Editor::CopyRangeToClipboard(int start, int end) {
	start = pdoc->ClampPositionIntoDocument(start);
	end = pdoc->ClampPositionIntoDocument(end);
	SelectionText selectedText;
	std::string text = RangeText(start, end);
	selectedText.Copy(text,
		pdoc->dbcsCodePage, vs.styles[STYLE_DEFAULT].characterSet, false, false);
	CopyToClipboard(selectedText);
}

void Editor::CopyText(int length, const char *text) {
	SelectionText selectedText;
	selectedText.Copy(std::string(text, length),
		pdoc->dbcsCodePage, vs.styles[STYLE_DEFAULT].characterSet, false, false);
	CopyToClipboard(selectedText);
}

void Editor::SetDragPosition(SelectionPosition newPos) {
	if (newPos.Position() >= 0) {
		newPos = MovePositionOutsideChar(newPos, 1);
		posDrop = newPos;
	}
	if (!(posDrag == newPos)) {
		caret.on = true;
		SetTicking(true);
		InvalidateCaret();
		posDrag = newPos;
		InvalidateCaret();
	}
}

void Editor::DisplayCursor(Window::Cursor c) {
	if (cursorMode == SC_CURSORNORMAL)
		wMain.SetCursor(c);
	else
		wMain.SetCursor(static_cast<Window::Cursor>(cursorMode));
}

bool Editor::DragThreshold(Point ptStart, Point ptNow) {
	int xMove = ptStart.x - ptNow.x;
	int yMove = ptStart.y - ptNow.y;
	int distanceSquared = xMove * xMove + yMove * yMove;
	return distanceSquared > 16;
}

void Editor::StartDrag() {
	// Always handled by subclasses
	//SetMouseCapture(true);
	//DisplayCursor(Window::cursorArrow);
}

void Editor::DropAt(SelectionPosition position, const char *value, size_t lengthValue, bool moving, bool rectangular) {
	//Platform::DebugPrintf("DropAt %d %d\n", inDragDrop, position);
	if (inDragDrop == ddDragging)
		dropWentOutside = false;

	bool positionWasInSelection = PositionInSelection(position.Position());

	bool positionOnEdgeOfSelection =
	    (position == SelectionStart()) || (position == SelectionEnd());

	if ((inDragDrop != ddDragging) || !(positionWasInSelection) ||
	        (positionOnEdgeOfSelection && !moving)) {

		SelectionPosition selStart = SelectionStart();
		SelectionPosition selEnd = SelectionEnd();

		UndoGroup ug(pdoc);

		SelectionPosition positionAfterDeletion = position;
		if ((inDragDrop == ddDragging) && moving) {
			// Remove dragged out text
			if (rectangular || sel.selType == Selection::selLines) {
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

		if (rectangular) {
			PasteRectangular(position, value, static_cast<int>(lengthValue));
			// Should try to select new rectangle but it may not be a rectangle now so just select the drop position
			SetEmptySelection(position);
		} else {
			position = MovePositionOutsideChar(position, sel.MainCaret() - position.Position());
			position = SelectionPosition(InsertSpace(position.Position(), position.VirtualSpace()));
			if (pdoc->InsertString(position.Position(), value, static_cast<int>(lengthValue))) {
				SelectionPosition posAfterInsertion = position;
				posAfterInsertion.Add(static_cast<int>(lengthValue));
				SetSelection(posAfterInsertion, position);
			}
		}
	} else if (inDragDrop == ddDragging) {
		SetEmptySelection(position);
	}
}

void Editor::DropAt(SelectionPosition position, const char *value, bool moving, bool rectangular) {
	DropAt(position, value, strlen(value), moving, rectangular);
}

/**
 * @return true if given position is inside the selection,
 */
bool Editor::PositionInSelection(int pos) {
	pos = MovePositionOutsideChar(pos, sel.MainCaret() - pos);
	for (size_t r=0; r<sel.Count(); r++) {
		if (sel.Range(r).Contains(pos))
			return true;
	}
	return false;
}

bool Editor::PointInSelection(Point pt) {
	SelectionPosition pos = SPositionFromLocation(pt, false, true);
	Point ptPos = LocationFromPosition(pos);
	for (size_t r=0; r<sel.Count(); r++) {
		SelectionRange range = sel.Range(r);
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

bool Editor::PointInSelMargin(Point pt) {
	// Really means: "Point in a margin"
	if (vs.fixedColumnWidth > 0) {	// There is a margin
		PRectangle rcSelMargin = GetClientRectangle();
		rcSelMargin.right = vs.textStart - vs.leftMarginWidth;
		rcSelMargin.left = vs.textStart - vs.fixedColumnWidth;
		return rcSelMargin.Contains(pt);
	} else {
		return false;
	}
}

Window::Cursor Editor::GetMarginCursor(Point pt) const {
	int x = 0;
	for (int margin = 0; margin <= SC_MAX_MARGIN; margin++) {
		if ((pt.x >= x) && (pt.x < x + vs.ms[margin].width))
			return static_cast<Window::Cursor>(vs.ms[margin].cursor);
		x += vs.ms[margin].width;
	}
	return Window::cursorReverseArrow;
}

void Editor::TrimAndSetSelection(int currentPos_, int anchor_) {
	sel.TrimSelection(SelectionRange(currentPos_, anchor_));
	SetSelection(currentPos_, anchor_);
}

void Editor::LineSelection(int lineCurrentPos_, int lineAnchorPos_, bool wholeLine) {
	int selCurrentPos, selAnchorPos;
	if (wholeLine) {
		int lineCurrent_ = pdoc->LineFromPosition(lineCurrentPos_);
		int lineAnchor_ = pdoc->LineFromPosition(lineAnchorPos_);
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

void Editor::WordSelection(int pos) {
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
		if (pos > pdoc->LineStart(pdoc->LineFromPosition(pos)))
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
		ticksToDwell = SC_TIME_FOREVER;
	if (dwelling && (dwellDelay < SC_TIME_FOREVER)) {
		dwelling = false;
		NotifyDwelling(ptMouseLast, dwelling);
	}
}

void Editor::MouseLeave() {
	SetHotSpotRange(NULL);
	if (!HaveMouseCapture()) {
		ptMouseLast = Point(-1,-1);
		DwellEnd(true);
	}
}

static bool AllowVirtualSpace(int virtualSpaceOptions, bool rectangular) {
	return (!rectangular && ((virtualSpaceOptions & SCVS_USERACCESSIBLE) != 0))
		|| (rectangular && ((virtualSpaceOptions & SCVS_RECTANGULARSELECTION) != 0));
}

void Editor::ButtonDown(Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) {
	//Platform::DebugPrintf("ButtonDown %d %d = %d alt=%d %d\n", curTime, lastClickTime, curTime - lastClickTime, alt, inDragDrop);
	ptMouseLast = pt;
	SelectionPosition newPos = SPositionFromLocation(pt, false, false, AllowVirtualSpace(virtualSpaceOptions, alt));
	newPos = MovePositionOutsideChar(newPos, sel.MainCaret() - newPos.Position());
	inDragDrop = ddNone;
	sel.SetMoveExtends(false);

	if (NotifyMarginClick(pt, shift, ctrl, alt))
		return;

	NotifyIndicatorClick(true, newPos.Position(), shift, ctrl, alt);

	bool inSelMargin = PointInSelMargin(pt);
	// In margin ctrl+(double)click should always select everything
	if (ctrl && inSelMargin) {
		SelectAll();
		lastClickTime = curTime;
		lastClick = pt;
		return;
	}
	if (shift && !inSelMargin) {
		SetSelection(newPos);
	}
	if (((curTime - lastClickTime) < Platform::DoubleClickTime()) && Close(pt, lastClick)) {
		//Platform::DebugPrintf("Double click %d %d = %d\n", curTime, lastClickTime, curTime - lastClickTime);
		SetMouseCapture(true);
		if (!ctrl || !multipleSelection || (selectionType != selChar && selectionType != selWord))
			SetEmptySelection(newPos.Position());
		bool doubleClick = false;
		// Stop mouse button bounce changing selection type
		if (!Platform::MouseButtonBounce() || curTime != lastClickTime) {
			if (inSelMargin) {
				// Inside margin selection type should be either selSubLine or selWholeLine.
				if (selectionType == selSubLine) {
					// If it is selSubLine, we're inside a *double* click and word wrap is enabled,
					// so we switch to selWholeLine in order to select whole line.
					selectionType = selWholeLine;
				} else if (selectionType != selSubLine && selectionType != selWholeLine) {
					// If it is neither, reset selection type to line selection.
					selectionType = ((wrapState != eWrapNone) && (marginOptions & SC_MARGINOPTION_SUBLINESELECT)) ? selSubLine : selWholeLine;
				}
			} else {
				if (selectionType == selChar) {
					selectionType = selWord;
					doubleClick = true;
				} else if (selectionType == selWord) {
					// Since we ended up here, we're inside a *triple* click, which should always select
					// whole line irregardless of word wrap being enabled or not.
					selectionType = selWholeLine;
				} else {
					selectionType = selChar;
					originalAnchorPos = sel.MainCaret();
				}
			}
		}

		if (selectionType == selWord) {
			int charPos = originalAnchorPos;
			if (sel.MainCaret() == originalAnchorPos) {
				charPos = PositionFromLocation(pt, false, true);
				charPos = MovePositionOutsideChar(charPos, -1);
			}

			int startWord, endWord;
			if ((sel.MainCaret() >= originalAnchorPos) && !pdoc->IsLineEndPosition(charPos)) {
				startWord = pdoc->ExtendWordSelect(pdoc->MovePositionOutsideChar(charPos + 1, 1), -1);
				endWord = pdoc->ExtendWordSelect(charPos, 1);
			} else {
				// Selecting backwards, or anchor beyond last character on line. In these cases,
				// we select the word containing the character to the *left* of the anchor.
				if (charPos > pdoc->LineStart(pdoc->LineFromPosition(charPos))) {
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
		} else if (selectionType == selSubLine || selectionType == selWholeLine) {
			lineAnchorPos = newPos.Position();
			LineSelection(lineAnchorPos, lineAnchorPos, selectionType == selWholeLine);
			//Platform::DebugPrintf("Triple click: %d - %d\n", anchor, currentPos);
		} else {
			SetEmptySelection(sel.MainCaret());
		}
		//Platform::DebugPrintf("Double click: %d - %d\n", anchor, currentPos);
		if (doubleClick) {
			NotifyDoubleClick(pt, shift, ctrl, alt);
			if (PositionIsHotspot(newPos.Position()))
				NotifyHotSpotDoubleClicked(newPos.Position(), shift, ctrl, alt);
		}
	} else {	// Single click
		if (inSelMargin) {
			sel.selType = Selection::selStream;
			if (!shift) {
				// Single click in margin: select whole line or only subline if word wrap is enabled
				lineAnchorPos = newPos.Position();
				selectionType = ((wrapState != eWrapNone) && (marginOptions & SC_MARGINOPTION_SUBLINESELECT)) ? selSubLine : selWholeLine;
				LineSelection(lineAnchorPos, lineAnchorPos, selectionType == selWholeLine);
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
				if (sel.Empty() || (selectionType != selSubLine && selectionType != selWholeLine))
					selectionType = ((wrapState != eWrapNone) && (marginOptions & SC_MARGINOPTION_SUBLINESELECT)) ? selSubLine : selWholeLine;
				LineSelection(newPos.Position(), lineAnchorPos, selectionType == selWholeLine);
			}

			SetDragPosition(SelectionPosition(invalidPosition));
			SetMouseCapture(true);
		} else {
			if (PointIsHotspot(pt)) {
				NotifyHotSpotClicked(newPos.Position(), shift, ctrl, alt);
				hotSpotClickPos = PositionFromLocation(pt,true,false);
			}
			if (!shift) {
				if (PointInSelection(pt) && !SelectionEmpty())
					inDragDrop = ddInitial;
				else
					inDragDrop = ddNone;
			}
			SetMouseCapture(true);
			if (inDragDrop != ddInitial) {
				SetDragPosition(SelectionPosition(invalidPosition));
				if (!shift) {
					if (ctrl && multipleSelection) {
						SelectionRange range(newPos);
						sel.TentativeSelection(range);
						InvalidateSelection(range, true);
					} else {
						InvalidateSelection(SelectionRange(newPos), true);
						if (sel.Count() > 1)
							Redraw();
						if ((sel.Count() > 1) || (sel.selType != Selection::selStream))
							sel.Clear();
						sel.selType = alt ? Selection::selRectangle : Selection::selStream;
						SetSelection(newPos, newPos);
					}
				}
				SelectionPosition anchorCurrent = newPos;
				if (shift)
					anchorCurrent = sel.IsRectangular() ?
						sel.Rectangular().anchor : sel.RangeMain().anchor;
				sel.selType = alt ? Selection::selRectangle : Selection::selStream;
				selectionType = selChar;
				originalAnchorPos = sel.MainCaret();
				sel.Rectangular() = SelectionRange(newPos, anchorCurrent);
				SetRectangularRange();
			}
		}
	}
	lastClickTime = curTime;
	lastClick = pt;
	lastXChosen = pt.x + xOffset;
	ShowCaretAtCurrentPosition();
}

bool Editor::PositionIsHotspot(int position) const {
	return vs.styles[pdoc->StyleAt(position) & pdoc->stylingBitsMask].hotspot;
}

bool Editor::PointIsHotspot(Point pt) {
	int pos = PositionFromLocation(pt, true);
	if (pos == INVALID_POSITION)
		return false;
	return PositionIsHotspot(pos);
}

void Editor::SetHotSpotRange(Point *pt) {
	if (pt) {
		int pos = PositionFromLocation(*pt);

		// If we don't limit this to word characters then the
		// range can encompass more than the run range and then
		// the underline will not be drawn properly.
		int hsStart_ = pdoc->ExtendStyleRange(pos, -1, vs.hotspotSingleLine);
		int hsEnd_ = pdoc->ExtendStyleRange(pos, 1, vs.hotspotSingleLine);

		// Only invalidate the range if the hotspot range has changed...
		if (hsStart_ != hsStart || hsEnd_ != hsEnd) {
			if (hsStart != -1) {
				InvalidateRange(hsStart, hsEnd);
			}
			hsStart = hsStart_;
			hsEnd = hsEnd_;
			InvalidateRange(hsStart, hsEnd);
		}
	} else {
		if (hsStart != -1) {
			int hsStart_ = hsStart;
			int hsEnd_ = hsEnd;
			hsStart = -1;
			hsEnd = -1;
			InvalidateRange(hsStart_, hsEnd_);
		} else {
			hsStart = -1;
			hsEnd = -1;
		}
	}
}

void Editor::GetHotSpotRange(int &hsStart_, int &hsEnd_) const {
	hsStart_ = hsStart;
	hsEnd_ = hsEnd;
}

void Editor::ButtonMove(Point pt) {
	if ((ptMouseLast.x != pt.x) || (ptMouseLast.y != pt.y)) {
		DwellEnd(true);
	}

	SelectionPosition movePos = SPositionFromLocation(pt, false, false,
		AllowVirtualSpace(virtualSpaceOptions, sel.IsRectangular()));
	movePos = MovePositionOutsideChar(movePos, sel.MainCaret() - movePos.Position());

	if (inDragDrop == ddInitial) {
		if (DragThreshold(ptMouseLast, pt)) {
			SetMouseCapture(false);
			SetDragPosition(movePos);
			CopySelectionRange(&drag);
			StartDrag();
		}
		return;
	}

	ptMouseLast = pt;
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
			if (selectionType == selChar) {
				if (sel.IsRectangular()) {
					sel.Rectangular() = SelectionRange(movePos, sel.Rectangular().anchor);
					SetSelection(movePos, sel.RangeMain().anchor);
				} else if (sel.Count() > 1) {
					SelectionRange range(movePos, sel.RangeMain().anchor);
					sel.TentativeSelection(range);
					InvalidateSelection(range, true);
				} else {
					SetSelection(movePos, sel.RangeMain().anchor);
				}
			} else if (selectionType == selWord) {
				// Continue selecting by word
				if (movePos.Position() == wordSelectInitialCaretPos) {  // Didn't move
					// No need to do anything. Previously this case was lumped
					// in with "Moved forward", but that can be harmful in this
					// case: a handler for the NotifyDoubleClick re-adjusts
					// the selection for a fancier definition of "word" (for
					// example, in Perl it is useful to include the leading
					// '$', '%' or '@' on variables for word selection). In this
					// the ButtonMove() called via Tick() for auto-scrolling
					// could result in the fancier word selection adjustment
					// being unmade.
				} else {
					wordSelectInitialCaretPos = -1;
					WordSelection(movePos.Position());
				}
			} else {
				// Continue selecting by line
				LineSelection(movePos.Position(), lineAnchorPos, selectionType == selWholeLine);
			}
		}

		// Autoscroll
		PRectangle rcClient = GetClientRectangle();
		Point ptOrigin = GetVisibleOriginInMain();
		rcClient.Move(0, -ptOrigin.y);
		int lineMove = DisplayFromPosition(movePos.Position());
		if (pt.y > rcClient.bottom) {
			ScrollTo(lineMove - LinesOnScreen() + 1);
			Redraw();
		} else if (pt.y < rcClient.top) {
			ScrollTo(lineMove);
			Redraw();
		}
		EnsureCaretVisible(false, false, true);

		if (hsStart != -1 && !PositionIsHotspot(movePos.Position()))
			SetHotSpotRange(NULL);

		if (hotSpotClickPos != INVALID_POSITION && PositionFromLocation(pt,true,false) != hotSpotClickPos) {
			if (inDragDrop == ddNone) {
				DisplayCursor(Window::cursorText);
			}
			hotSpotClickPos = INVALID_POSITION;
		}

	} else {
		if (vs.fixedColumnWidth > 0) {	// There is a margin
			if (PointInSelMargin(pt)) {
				DisplayCursor(GetMarginCursor(pt));
				SetHotSpotRange(NULL);
				return; 	// No need to test for selection
			}
		}
		// Display regular (drag) cursor over selection
		if (PointInSelection(pt) && !SelectionEmpty()) {
			DisplayCursor(Window::cursorArrow);
		} else if (PointIsHotspot(pt)) {
			DisplayCursor(Window::cursorHand);
			SetHotSpotRange(&pt);
		} else {
			DisplayCursor(Window::cursorText);
			SetHotSpotRange(NULL);
		}
	}
}

void Editor::ButtonUp(Point pt, unsigned int curTime, bool ctrl) {
	//Platform::DebugPrintf("ButtonUp %d %d\n", HaveMouseCapture(), inDragDrop);
	SelectionPosition newPos = SPositionFromLocation(pt, false, false,
		AllowVirtualSpace(virtualSpaceOptions, sel.IsRectangular()));
	newPos = MovePositionOutsideChar(newPos, sel.MainCaret() - newPos.Position());
	if (inDragDrop == ddInitial) {
		inDragDrop = ddNone;
		SetEmptySelection(newPos);
		selectionType = selChar;
		originalAnchorPos = sel.MainCaret();
	}
	if (hotSpotClickPos != INVALID_POSITION && PointIsHotspot(pt)) {
		hotSpotClickPos = INVALID_POSITION;
		NotifyHotSpotReleaseClick(newPos.Position(), false, ctrl, false);
	}
	if (HaveMouseCapture()) {
		if (PointInSelMargin(pt)) {
			DisplayCursor(GetMarginCursor(pt));
		} else {
			DisplayCursor(Window::cursorText);
			SetHotSpotRange(NULL);
		}
		ptMouseLast = pt;
		SetMouseCapture(false);
		NotifyIndicatorClick(false, newPos.Position(), false, false, false);
		if (inDragDrop == ddDragging) {
			SelectionPosition selStart = SelectionStart();
			SelectionPosition selEnd = SelectionEnd();
			if (selStart < selEnd) {
				if (drag.Length()) {
					if (ctrl) {
						if (pdoc->InsertString(newPos.Position(), drag.Data(), static_cast<int>(drag.Length()))) {
							SetSelection(newPos.Position(), newPos.Position() + static_cast<int>(drag.Length()));
						}
					} else if (newPos < selStart) {
						pdoc->DeleteChars(selStart.Position(), static_cast<int>(drag.Length()));
						if (pdoc->InsertString(newPos.Position(), drag.Data(), static_cast<int>(drag.Length()))) {
							SetSelection(newPos.Position(), newPos.Position() + static_cast<int>(drag.Length()));
						}
					} else if (newPos > selEnd) {
						pdoc->DeleteChars(selStart.Position(), static_cast<int>(drag.Length()));
						newPos.Add(-static_cast<int>(drag.Length()));
						if (pdoc->InsertString(newPos.Position(), drag.Data(), static_cast<int>(drag.Length()))) {
							SetSelection(newPos.Position(), newPos.Position() + static_cast<int>(drag.Length()));
						}
					} else {
						SetEmptySelection(newPos.Position());
					}
					drag.Clear();
				}
				selectionType = selChar;
			}
		} else {
			if (selectionType == selChar) {
				if (sel.Count() > 1) {
					sel.RangeMain() =
						SelectionRange(newPos, sel.Range(sel.Count() - 1).anchor);
					InvalidateSelection(sel.RangeMain(), true);
				} else {
					SetSelection(newPos, sel.RangeMain().anchor);
				}
			}
			sel.CommitTentative();
		}
		SetRectangularRange();
		lastClickTime = curTime;
		lastClick = pt;
		lastXChosen = pt.x + xOffset;
		if (sel.selType == Selection::selStream) {
			SetLastXChosen();
		}
		inDragDrop = ddNone;
		EnsureCaretVisible(false);
	}
}

// Called frequently to perform background UI including
// caret blinking and automatic scrolling.
void Editor::Tick() {
	if (HaveMouseCapture()) {
		// Auto scroll
		ButtonMove(ptMouseLast);
	}
	if (caret.period > 0) {
		timer.ticksToWait -= timer.tickSize;
		if (timer.ticksToWait <= 0) {
			caret.on = !caret.on;
			timer.ticksToWait = caret.period;
			if (caret.active) {
				InvalidateCaret();
			}
		}
	}
	if (horizontalScrollBarVisible && trackLineWidth && (lineWidthMaxSeen > scrollWidth)) {
		scrollWidth = lineWidthMaxSeen;
		SetScrollBars();
	}
	if ((dwellDelay < SC_TIME_FOREVER) &&
	        (ticksToDwell > 0) &&
	        (!HaveMouseCapture()) &&
	        (ptMouseLast.y >= 0)) {
		ticksToDwell -= timer.tickSize;
		if (ticksToDwell <= 0) {
			dwelling = true;
			NotifyDwelling(ptMouseLast, dwelling);
		}
	}
}

bool Editor::Idle() {

	bool idleDone;

	bool wrappingDone = wrapState == eWrapNone;

	if (!wrappingDone) {
		// Wrap lines during idle.
		WrapLines(wsIdle);
		// No more wrapping
		if (!wrapPending.NeedsWrap())
			wrappingDone = true;
	}

	// Add more idle things to do here, but make sure idleDone is
	// set correctly before the function returns. returning
	// false will stop calling this idle funtion until SetIdle() is
	// called again.

	idleDone = wrappingDone; // && thatDone && theOtherThingDone...

	return !idleDone;
}

void Editor::SetFocusState(bool focusState) {
	hasFocus = focusState;
	NotifyFocus(hasFocus);
	if (hasFocus) {
		ShowCaretAtCurrentPosition();
	} else {
		CancelModes();
		DropCaret();
	}
}

int Editor::PositionAfterArea(PRectangle rcArea) const {
	// The start of the document line after the display line after the area
	// This often means that the line after a modification is restyled which helps
	// detect multiline comment additions and heals single line comments
	int lineAfter = topLine + (rcArea.bottom - 1) / vs.lineHeight + 1;
	if (lineAfter < cs.LinesDisplayed())
		return pdoc->LineStart(cs.DocFromDisplay(lineAfter) + 1);
	else
		return pdoc->Length();
}

// Style to a position within the view. If this causes a change at end of last line then
// affects later lines so style all the viewed text.
void Editor::StyleToPositionInView(Position pos) {
	int endWindow = (vs.marginInside) ? (PositionAfterArea(GetClientRectangle())) : (pdoc->Length());
	if (pos > endWindow)
		pos = endWindow;
	int styleAtEnd = pdoc->StyleAt(pos-1);
	pdoc->EnsureStyledTo(pos);
	if ((endWindow > pos) && (styleAtEnd != pdoc->StyleAt(pos-1))) {
		// Style at end of line changed so is multi-line change like starting a comment
		// so require rest of window to be styled.
		pdoc->EnsureStyledTo(endWindow);
	}
}

void Editor::IdleWork() {
	// Style the line after the modification as this allows modifications that change just the
	// line of the modification to heal instead of propagating to the rest of the window.
	if (workNeeded.items & WorkNeeded::workStyle)
		StyleToPositionInView(pdoc->LineStart(pdoc->LineFromPosition(workNeeded.upTo) + 2));

	NotifyUpdateUI();
	workNeeded.Reset();
}

void Editor::QueueIdleWork(WorkNeeded::workItems items, int upTo) {
	workNeeded.Need(items, upTo);
}

bool Editor::PaintContains(PRectangle rc) {
	if (rc.Empty()) {
		return true;
	} else {
		return rcPaint.Contains(rc);
	}
}

bool Editor::PaintContainsMargin() {
	if (wMargin.GetID()) {
		// With separate margin view, paint of text view
		// never contains margin.
		return false;
	}
	PRectangle rcSelMargin = GetClientRectangle();
	rcSelMargin.right = vs.textStart;
	return PaintContains(rcSelMargin);
}

void Editor::CheckForChangeOutsidePaint(Range r) {
	if (paintState == painting && !paintingAllText) {
		//Platform::DebugPrintf("Checking range in paint %d-%d\n", r.start, r.end);
		if (!r.Valid())
			return;

		PRectangle rcRange = RectangleFromRange(r.start, r.end);
		PRectangle rcText = GetTextRectangle();
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

void Editor::SetBraceHighlight(Position pos0, Position pos1, int matchStyle) {
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
		if (paintState == notPainting) {
			Redraw();
		}
	}
}

void Editor::SetAnnotationHeights(int start, int end) {
	if (vs.annotationVisible) {
		bool changedHeight = false;
		for (int line=start; line<end && line<pdoc->LinesTotal(); line++) {
			int linesWrapped = 1;
			if (wrapState != eWrapNone) {
				AutoSurface surface(this);
				AutoLineLayout ll(llc, RetrieveLineLayout(line));
				if (surface && ll) {
					LayoutLine(line, surface, vs, ll, wrapWidth);
					linesWrapped = ll->lines;
				}
			}
			if (cs.SetHeight(line, pdoc->AnnotationLines(line) + linesWrapped))
				changedHeight = true;
		}
		if (changedHeight) {
			Redraw();
		}
	}
}

void Editor::SetDocPointer(Document *document) {
	//Platform::DebugPrintf("** %x setdoc to %x\n", pdoc, document);
	pdoc->RemoveWatcher(this, 0);
	pdoc->Release();
	if (document == NULL) {
		pdoc = new Document();
	} else {
		pdoc = document;
	}
	pdoc->AddRef();

	// Ensure all positions within document
	sel.Clear();
	targetStart = 0;
	targetEnd = 0;

	braces[0] = invalidPosition;
	braces[1] = invalidPosition;

	vs.ReleaseAllExtendedStyles();

	// Reset the contraction state to fully shown.
	cs.Clear();
	cs.InsertLines(0, pdoc->LinesTotal() - 1);
	SetAnnotationHeights(0, pdoc->LinesTotal());
	llc.Deallocate();
	NeedWrapping();

	pdoc->AddWatcher(this, 0);
	SetScrollBars();
	Redraw();
}

void Editor::SetAnnotationVisible(int visible) {
	if (vs.annotationVisible != visible) {
		bool changedFromOrToHidden = ((vs.annotationVisible != 0) != (visible != 0));
		vs.annotationVisible = visible;
		if (changedFromOrToHidden) {
			int dir = vs.annotationVisible ? 1 : -1;
			for (int line=0; line<pdoc->LinesTotal(); line++) {
				int annotationLines = pdoc->AnnotationLines(line);
				if (annotationLines > 0) {
					cs.SetHeight(line, cs.GetHeight(line) + annotationLines * dir);
				}
			}
		}
		Redraw();
	}
}

/**
 * Recursively expand a fold, making lines visible except where they have an unexpanded parent.
 */
int Editor::ExpandLine(int line) {
	int lineMaxSubord = pdoc->GetLastChild(line);
	line++;
	while (line <= lineMaxSubord) {
		cs.SetVisible(line, line, true);
		int level = pdoc->GetLevel(line);
		if (level & SC_FOLDLEVELHEADERFLAG) {
			if (cs.GetExpanded(line)) {
				line = ExpandLine(line);
			} else {
				line = pdoc->GetLastChild(line);
			}
		}
		line++;
	}
	return lineMaxSubord;
}

void Editor::SetFoldExpanded(int lineDoc, bool expanded) {
	if (cs.SetExpanded(lineDoc, expanded)) {
		RedrawSelMargin();
	}
}

void Editor::FoldLine(int line, int action) {
	if (line >= 0) {
		if (action == SC_FOLDACTION_TOGGLE) {
 			if ((pdoc->GetLevel(line) & SC_FOLDLEVELHEADERFLAG) == 0) {
				line = pdoc->GetFoldParent(line);
				if (line < 0)
					return;
			}
			action = (cs.GetExpanded(line)) ? SC_FOLDACTION_CONTRACT : SC_FOLDACTION_EXPAND;
		}

		if (action == SC_FOLDACTION_CONTRACT) {
			int lineMaxSubord = pdoc->GetLastChild(line);
			if (lineMaxSubord > line) {
				cs.SetExpanded(line, 0);
				cs.SetVisible(line + 1, lineMaxSubord, false);

				int lineCurrent = pdoc->LineFromPosition(sel.MainCaret());
				if (lineCurrent > line && lineCurrent <= lineMaxSubord) {
					// This does not re-expand the fold
					EnsureCaretVisible();
				}
			}

		} else {
			if (!(cs.GetVisible(line))) {
				EnsureLineVisible(line, false);
				GoToLine(line);
			}
			cs.SetExpanded(line, 1);
			ExpandLine(line);
		}

		SetScrollBars();
		Redraw();
	}
}

void Editor::FoldExpand(int line, int action, int level) {
	bool expanding = action == SC_FOLDACTION_EXPAND;
	if (action == SC_FOLDACTION_TOGGLE) {
		expanding = !cs.GetExpanded(line);
	}
	SetFoldExpanded(line, expanding);
	if (expanding && (cs.HiddenLines() == 0))
		// Nothing to do
		return;
	int lineMaxSubord = pdoc->GetLastChild(line, level & SC_FOLDLEVELNUMBERMASK);
	line++;
	cs.SetVisible(line, lineMaxSubord, expanding);
	while (line <= lineMaxSubord) {
		int levelLine = pdoc->GetLevel(line);
		if (levelLine & SC_FOLDLEVELHEADERFLAG) {
			SetFoldExpanded(line, expanding);
		}
		line++;
	}
	SetScrollBars();
	Redraw();
}

int Editor::ContractedFoldNext(int lineStart) const {
	for (int line = lineStart; line<pdoc->LinesTotal();) {
		if (!cs.GetExpanded(line) && (pdoc->GetLevel(line) & SC_FOLDLEVELHEADERFLAG))
			return line;
		line = cs.ContractedNext(line+1);
		if (line < 0)
			return -1;
	}

	return -1;
}

/**
 * Recurse up from this line to find any folds that prevent this line from being visible
 * and unfold them all.
 */
void Editor::EnsureLineVisible(int lineDoc, bool enforcePolicy) {

	// In case in need of wrapping to ensure DisplayFromDoc works.
	if (lineDoc >= wrapPending.start)
		WrapLines(wsAll);

	if (!cs.GetVisible(lineDoc)) {
		// Back up to find a non-blank line
		int lookLine = lineDoc;
		int lookLineLevel = pdoc->GetLevel(lookLine);
		while ((lookLine > 0) && (lookLineLevel & SC_FOLDLEVELWHITEFLAG)) {
			lookLineLevel = pdoc->GetLevel(--lookLine);
		}
		int lineParent = pdoc->GetFoldParent(lookLine);
		if (lineParent < 0) {
			// Backed up to a top level line, so try to find parent of initial line
			lineParent = pdoc->GetFoldParent(lineDoc);
		}
		if (lineParent >= 0) {
			if (lineDoc != lineParent)
				EnsureLineVisible(lineParent, enforcePolicy);
			if (!cs.GetExpanded(lineParent)) {
				cs.SetExpanded(lineParent, 1);
				ExpandLine(lineParent);
			}
		}
		SetScrollBars();
		Redraw();
	}
	if (enforcePolicy) {
		int lineDisplay = cs.DisplayFromDoc(lineDoc);
		if (visiblePolicy & VISIBLE_SLOP) {
			if ((topLine > lineDisplay) || ((visiblePolicy & VISIBLE_STRICT) && (topLine + visibleSlop > lineDisplay))) {
				SetTopLine(Platform::Clamp(lineDisplay - visibleSlop, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			} else if ((lineDisplay > topLine + LinesOnScreen() - 1) ||
			        ((visiblePolicy & VISIBLE_STRICT) && (lineDisplay > topLine + LinesOnScreen() - 1 - visibleSlop))) {
				SetTopLine(Platform::Clamp(lineDisplay - LinesOnScreen() + 1 + visibleSlop, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			}
		} else {
			if ((topLine > lineDisplay) || (lineDisplay > topLine + LinesOnScreen() - 1) || (visiblePolicy & VISIBLE_STRICT)) {
				SetTopLine(Platform::Clamp(lineDisplay - LinesOnScreen() / 2 + 1, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			}
		}
	}
}

void Editor::FoldAll(int action) {
	pdoc->EnsureStyledTo(pdoc->Length());
	int maxLine = pdoc->LinesTotal();
	bool expanding = action == SC_FOLDACTION_EXPAND;
	if (action == SC_FOLDACTION_TOGGLE) {
		// Discover current state
		for (int lineSeek = 0; lineSeek < maxLine; lineSeek++) {
			if (pdoc->GetLevel(lineSeek) & SC_FOLDLEVELHEADERFLAG) {
				expanding = !cs.GetExpanded(lineSeek);
				break;
			}
		}
	}
	if (expanding) {
		cs.SetVisible(0, maxLine-1, true);
		for (int line = 0; line < maxLine; line++) {
			int levelLine = pdoc->GetLevel(line);
			if (levelLine & SC_FOLDLEVELHEADERFLAG) {
				SetFoldExpanded(line, true);
			}
		}
	} else {
		for (int line = 0; line < maxLine; line++) {
			int level = pdoc->GetLevel(line);
			if ((level & SC_FOLDLEVELHEADERFLAG) &&
					(SC_FOLDLEVELBASE == (level & SC_FOLDLEVELNUMBERMASK))) {
				SetFoldExpanded(line, false);
				int lineMaxSubord = pdoc->GetLastChild(line, -1);
				if (lineMaxSubord > line) {
					cs.SetVisible(line + 1, lineMaxSubord, false);
				}
			}
		}
	}
	SetScrollBars();
	Redraw();
}

void Editor::FoldChanged(int line, int levelNow, int levelPrev) {
	if (levelNow & SC_FOLDLEVELHEADERFLAG) {
		if (!(levelPrev & SC_FOLDLEVELHEADERFLAG)) {
			// Adding a fold point.
			if (cs.SetExpanded(line, true)) {
				RedrawSelMargin();
			}
			FoldExpand(line, SC_FOLDACTION_EXPAND, levelPrev);
		}
	} else if (levelPrev & SC_FOLDLEVELHEADERFLAG) {
		if (!cs.GetExpanded(line)) {
			// Removing the fold from one that has been contracted so should expand
			// otherwise lines are left invisible with no way to make them visible
			if (cs.SetExpanded(line, true)) {
				RedrawSelMargin();
			}
			FoldExpand(line, SC_FOLDACTION_EXPAND, levelPrev);
		}
	}
	if (!(levelNow & SC_FOLDLEVELWHITEFLAG) &&
	        ((levelPrev & SC_FOLDLEVELNUMBERMASK) > (levelNow & SC_FOLDLEVELNUMBERMASK))) {
		if (cs.HiddenLines()) {
			// See if should still be hidden
			int parentLine = pdoc->GetFoldParent(line);
			if ((parentLine < 0) || (cs.GetExpanded(parentLine) && cs.GetVisible(parentLine))) {
				cs.SetVisible(line, line, true);
				SetScrollBars();
				Redraw();
			}
		}
	}
}

void Editor::NeedShown(int pos, int len) {
	if (foldAutomatic & SC_AUTOMATICFOLD_SHOW) {
		int lineStart = pdoc->LineFromPosition(pos);
		int lineEnd = pdoc->LineFromPosition(pos+len);
		for (int line = lineStart; line <= lineEnd; line++) {
			EnsureLineVisible(line, false);
		}
	} else {
		NotifyNeedShown(pos, len);
	}
}

int Editor::GetTag(char *tagValue, int tagNumber) {
	const char *text = 0;
	int length = 0;
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

int Editor::ReplaceTarget(bool replacePatterns, const char *text, int length) {
	UndoGroup ug(pdoc);
	if (length == -1)
		length = istrlen(text);
	if (replacePatterns) {
		text = pdoc->SubstituteByPosition(text, &length);
		if (!text) {
			return 0;
		}
	}
	if (targetStart != targetEnd)
		pdoc->DeleteChars(targetStart, targetEnd - targetStart);
	targetEnd = targetStart;
	pdoc->InsertString(targetStart, text, length);
	targetEnd = targetStart + length;
	return length;
}

bool Editor::IsUnicodeMode() const {
	return pdoc && (SC_CP_UTF8 == pdoc->dbcsCodePage);
}

int Editor::CodePage() const {
	if (pdoc)
		return pdoc->dbcsCodePage;
	else
		return 0;
}

int Editor::WrapCount(int line) {
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(line));

	if (surface && ll) {
		LayoutLine(line, surface, vs, ll, wrapWidth);
		return ll->lines;
	} else {
		return 1;
	}
}

void Editor::AddStyledText(char *buffer, int appendLength) {
	// The buffer consists of alternating character bytes and style bytes
	int textLength = appendLength / 2;
	std::string text(textLength, '\0');
	int i;
	for (i = 0; i < textLength; i++) {
		text[i] = buffer[i*2];
	}
	pdoc->InsertString(CurrentPosition(), text.c_str(), textLength);
	for (i = 0; i < textLength; i++) {
		text[i] = buffer[i*2+1];
	}
	pdoc->StartStyling(CurrentPosition(), static_cast<char>(0xff));
	pdoc->SetStyles(textLength, text.c_str());
	SetEmptySelection(sel.MainCaret() + textLength);
}

static bool ValidMargin(unsigned long wParam) {
	return wParam <= SC_MAX_MARGIN;
}

static char *CharPtrFromSPtr(sptr_t lParam) {
	return reinterpret_cast<char *>(lParam);
}

void Editor::StyleSetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	vs.EnsureStyle(wParam);
	switch (iMessage) {
	case SCI_STYLESETFORE:
		vs.styles[wParam].fore = ColourDesired(lParam);
		break;
	case SCI_STYLESETBACK:
		vs.styles[wParam].back = ColourDesired(lParam);
		break;
	case SCI_STYLESETBOLD:
		vs.styles[wParam].weight = lParam != 0 ? SC_WEIGHT_BOLD : SC_WEIGHT_NORMAL;
		break;
	case SCI_STYLESETWEIGHT:
		vs.styles[wParam].weight = lParam;
		break;
	case SCI_STYLESETITALIC:
		vs.styles[wParam].italic = lParam != 0;
		break;
	case SCI_STYLESETEOLFILLED:
		vs.styles[wParam].eolFilled = lParam != 0;
		break;
	case SCI_STYLESETSIZE:
		vs.styles[wParam].size = lParam * SC_FONT_SIZE_MULTIPLIER;
		break;
	case SCI_STYLESETSIZEFRACTIONAL:
		vs.styles[wParam].size = lParam;
		break;
	case SCI_STYLESETFONT:
		if (lParam != 0) {
			vs.SetStyleFontName(wParam, CharPtrFromSPtr(lParam));
		}
		break;
	case SCI_STYLESETUNDERLINE:
		vs.styles[wParam].underline = lParam != 0;
		break;
	case SCI_STYLESETCASE:
		vs.styles[wParam].caseForce = static_cast<Style::ecaseForced>(lParam);
		break;
	case SCI_STYLESETCHARACTERSET:
		vs.styles[wParam].characterSet = lParam;
		pdoc->SetCaseFolder(NULL);
		break;
	case SCI_STYLESETVISIBLE:
		vs.styles[wParam].visible = lParam != 0;
		break;
	case SCI_STYLESETCHANGEABLE:
		vs.styles[wParam].changeable = lParam != 0;
		break;
	case SCI_STYLESETHOTSPOT:
		vs.styles[wParam].hotspot = lParam != 0;
		break;
	}
	InvalidateStyleRedraw();
}

sptr_t Editor::StyleGetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	vs.EnsureStyle(wParam);
	switch (iMessage) {
	case SCI_STYLEGETFORE:
		return vs.styles[wParam].fore.AsLong();
	case SCI_STYLEGETBACK:
		return vs.styles[wParam].back.AsLong();
	case SCI_STYLEGETBOLD:
		return vs.styles[wParam].weight > SC_WEIGHT_NORMAL;
	case SCI_STYLEGETWEIGHT:
		return vs.styles[wParam].weight;
	case SCI_STYLEGETITALIC:
		return vs.styles[wParam].italic ? 1 : 0;
	case SCI_STYLEGETEOLFILLED:
		return vs.styles[wParam].eolFilled ? 1 : 0;
	case SCI_STYLEGETSIZE:
		return vs.styles[wParam].size / SC_FONT_SIZE_MULTIPLIER;
	case SCI_STYLEGETSIZEFRACTIONAL:
		return vs.styles[wParam].size;
	case SCI_STYLEGETFONT:
		if (!vs.styles[wParam].fontName)
			return 0;
		if (lParam != 0)
			strcpy(CharPtrFromSPtr(lParam), vs.styles[wParam].fontName);
		return strlen(vs.styles[wParam].fontName);
	case SCI_STYLEGETUNDERLINE:
		return vs.styles[wParam].underline ? 1 : 0;
	case SCI_STYLEGETCASE:
		return static_cast<int>(vs.styles[wParam].caseForce);
	case SCI_STYLEGETCHARACTERSET:
		return vs.styles[wParam].characterSet;
	case SCI_STYLEGETVISIBLE:
		return vs.styles[wParam].visible ? 1 : 0;
	case SCI_STYLEGETCHANGEABLE:
		return vs.styles[wParam].changeable ? 1 : 0;
	case SCI_STYLEGETHOTSPOT:
		return vs.styles[wParam].hotspot ? 1 : 0;
	}
	return 0;
}

sptr_t Editor::StringResult(sptr_t lParam, const char *val) {
	const size_t n = strlen(val);
	if (lParam != 0) {
		char *ptr = reinterpret_cast<char *>(lParam);
		strcpy(ptr, val);
	}
	return n;	// Not including NUL
}

sptr_t Editor::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	//Platform::DebugPrintf("S start wnd proc %d %d %d\n",iMessage, wParam, lParam);

	// Optional macro recording hook
	if (recordingMacro)
		NotifyMacroRecord(iMessage, wParam, lParam);

	switch (iMessage) {

	case SCI_GETTEXT: {
			if (lParam == 0)
				return pdoc->Length() + 1;
			if (wParam == 0)
				return 0;
			char *ptr = CharPtrFromSPtr(lParam);
			unsigned int iChar = 0;
			for (; iChar < wParam - 1; iChar++)
				ptr[iChar] = pdoc->CharAt(iChar);
			ptr[iChar] = '\0';
			return iChar;
		}

	case SCI_SETTEXT: {
			if (lParam == 0)
				return 0;
			UndoGroup ug(pdoc);
			pdoc->DeleteChars(0, pdoc->Length());
			SetEmptySelection(0);
			pdoc->InsertCString(0, CharPtrFromSPtr(lParam));
			return 1;
		}

	case SCI_GETTEXTLENGTH:
		return pdoc->Length();

	case SCI_CUT:
		Cut();
		SetLastXChosen();
		break;

	case SCI_COPY:
		Copy();
		break;

	case SCI_COPYALLOWLINE:
		CopyAllowLine();
		break;

	case SCI_VERTICALCENTRECARET:
		VerticalCentreCaret();
		break;

	case SCI_MOVESELECTEDLINESUP:
		MoveSelectedLinesUp();
		break;

	case SCI_MOVESELECTEDLINESDOWN:
		MoveSelectedLinesDown();
		break;

	case SCI_COPYRANGE:
		CopyRangeToClipboard(wParam, lParam);
		break;

	case SCI_COPYTEXT:
		CopyText(wParam, CharPtrFromSPtr(lParam));
		break;

	case SCI_PASTE:
		Paste();
		if ((caretSticky == SC_CARETSTICKY_OFF) || (caretSticky == SC_CARETSTICKY_WHITESPACE)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;

	case SCI_CLEAR:
		Clear();
		SetLastXChosen();
		EnsureCaretVisible();
		break;

	case SCI_UNDO:
		Undo();
		SetLastXChosen();
		break;

	case SCI_CANUNDO:
		return (pdoc->CanUndo() && !pdoc->IsReadOnly()) ? 1 : 0;

	case SCI_EMPTYUNDOBUFFER:
		pdoc->DeleteUndoHistory();
		return 0;

	case SCI_GETFIRSTVISIBLELINE:
		return topLine;

	case SCI_SETFIRSTVISIBLELINE:
		ScrollTo(wParam);
		break;

	case SCI_GETLINE: {	// Risk of overwriting the end of the buffer
			int lineStart = pdoc->LineStart(wParam);
			int lineEnd = pdoc->LineStart(wParam + 1);
			if (lParam == 0) {
				return lineEnd - lineStart;
			}
			char *ptr = CharPtrFromSPtr(lParam);
			int iPlace = 0;
			for (int iChar = lineStart; iChar < lineEnd; iChar++) {
				ptr[iPlace++] = pdoc->CharAt(iChar);
			}
			return iPlace;
		}

	case SCI_GETLINECOUNT:
		if (pdoc->LinesTotal() == 0)
			return 1;
		else
			return pdoc->LinesTotal();

	case SCI_GETMODIFY:
		return !pdoc->IsSavePoint();

	case SCI_SETSEL: {
			int nStart = static_cast<int>(wParam);
			int nEnd = static_cast<int>(lParam);
			if (nEnd < 0)
				nEnd = pdoc->Length();
			if (nStart < 0)
				nStart = nEnd; 	// Remove selection
			InvalidateSelection(SelectionRange(nStart, nEnd));
			sel.Clear();
			sel.selType = Selection::selStream;
			SetSelection(nEnd, nStart);
			EnsureCaretVisible();
		}
		break;

	case SCI_GETSELTEXT: {
			SelectionText selectedText;
			CopySelectionRange(&selectedText);
			if (lParam == 0) {
				return selectedText.LengthWithTerminator();
			} else {
				char *ptr = CharPtrFromSPtr(lParam);
				unsigned int iChar = 0;
				if (selectedText.Length()) {
					for (; iChar < selectedText.LengthWithTerminator(); iChar++)
						ptr[iChar] = selectedText.Data()[iChar];
				} else {
					ptr[0] = '\0';
				}
				return iChar;
			}
		}

	case SCI_LINEFROMPOSITION:
		if (static_cast<int>(wParam) < 0)
			return 0;
		return pdoc->LineFromPosition(wParam);

	case SCI_POSITIONFROMLINE:
		if (static_cast<int>(wParam) < 0)
			wParam = pdoc->LineFromPosition(SelectionStart().Position());
		if (wParam == 0)
			return 0; 	// Even if there is no text, there is a first line that starts at 0
		if (static_cast<int>(wParam) > pdoc->LinesTotal())
			return -1;
		//if (wParam > pdoc->LineFromPosition(pdoc->Length()))	// Useful test, anyway...
		//	return -1;
		return pdoc->LineStart(wParam);

		// Replacement of the old Scintilla interpretation of EM_LINELENGTH
	case SCI_LINELENGTH:
		if ((static_cast<int>(wParam) < 0) ||
		        (static_cast<int>(wParam) > pdoc->LineFromPosition(pdoc->Length())))
			return 0;
		return pdoc->LineStart(wParam + 1) - pdoc->LineStart(wParam);

	case SCI_REPLACESEL: {
			if (lParam == 0)
				return 0;
			UndoGroup ug(pdoc);
			ClearSelection();
			char *replacement = CharPtrFromSPtr(lParam);
			pdoc->InsertCString(sel.MainCaret(), replacement);
			SetEmptySelection(sel.MainCaret() + istrlen(replacement));
			EnsureCaretVisible();
		}
		break;

	case SCI_SETTARGETSTART:
		targetStart = wParam;
		break;

	case SCI_GETTARGETSTART:
		return targetStart;

	case SCI_SETTARGETEND:
		targetEnd = wParam;
		break;

	case SCI_GETTARGETEND:
		return targetEnd;

	case SCI_TARGETFROMSELECTION:
		if (sel.MainCaret() < sel.MainAnchor()) {
			targetStart = sel.MainCaret();
			targetEnd = sel.MainAnchor();
		} else {
			targetStart = sel.MainAnchor();
			targetEnd = sel.MainCaret();
		}
		break;

	case SCI_REPLACETARGET:
		PLATFORM_ASSERT(lParam);
		return ReplaceTarget(false, CharPtrFromSPtr(lParam), wParam);

	case SCI_REPLACETARGETRE:
		PLATFORM_ASSERT(lParam);
		return ReplaceTarget(true, CharPtrFromSPtr(lParam), wParam);

	case SCI_SEARCHINTARGET:
		PLATFORM_ASSERT(lParam);
		return SearchInTarget(CharPtrFromSPtr(lParam), wParam);

	case SCI_SETSEARCHFLAGS:
		searchFlags = wParam;
		break;

	case SCI_GETSEARCHFLAGS:
		return searchFlags;

	case SCI_GETTAG:
		return GetTag(CharPtrFromSPtr(lParam), wParam);

	case SCI_POSITIONBEFORE:
		return pdoc->MovePositionOutsideChar(wParam - 1, -1, true);

	case SCI_POSITIONAFTER:
		return pdoc->MovePositionOutsideChar(wParam + 1, 1, true);

	case SCI_LINESCROLL:
		ScrollTo(topLine + lParam);
		HorizontalScrollTo(xOffset + static_cast<int>(wParam) * vs.spaceWidth);
		return 1;

	case SCI_SETXOFFSET:
		xOffset = wParam;
		ContainerNeedsUpdate(SC_UPDATE_H_SCROLL);
		SetHorizontalScrollPos();
		Redraw();
		break;

	case SCI_GETXOFFSET:
		return xOffset;

	case SCI_CHOOSECARETX:
		SetLastXChosen();
		break;

	case SCI_SCROLLCARET:
		EnsureCaretVisible();
		break;

	case SCI_SETREADONLY:
		pdoc->SetReadOnly(wParam != 0);
		return 1;

	case SCI_GETREADONLY:
		return pdoc->IsReadOnly();

	case SCI_CANPASTE:
		return CanPaste();

	case SCI_POINTXFROMPOSITION:
		if (lParam < 0) {
			return 0;
		} else {
			Point pt = LocationFromPosition(lParam);
			// Convert to view-relative
			return pt.x - vs.textStart + vs.fixedColumnWidth;
		}

	case SCI_POINTYFROMPOSITION:
		if (lParam < 0) {
			return 0;
		} else {
			Point pt = LocationFromPosition(lParam);
			return pt.y;
		}

	case SCI_FINDTEXT:
		return FindText(wParam, lParam);

	case SCI_GETTEXTRANGE: {
			if (lParam == 0)
				return 0;
			Sci_TextRange *tr = reinterpret_cast<Sci_TextRange *>(lParam);
			int cpMax = tr->chrg.cpMax;
			if (cpMax == -1)
				cpMax = pdoc->Length();
			PLATFORM_ASSERT(cpMax <= pdoc->Length());
			int len = cpMax - tr->chrg.cpMin; 	// No -1 as cpMin and cpMax are referring to inter character positions
			pdoc->GetCharRange(tr->lpstrText, tr->chrg.cpMin, len);
			// Spec says copied text is terminated with a NUL
			tr->lpstrText[len] = '\0';
			return len; 	// Not including NUL
		}

	case SCI_HIDESELECTION:
		hideSelection = wParam != 0;
		Redraw();
		break;

	case SCI_FORMATRANGE:
		return FormatRange(wParam != 0, reinterpret_cast<Sci_RangeToFormat *>(lParam));

	case SCI_GETMARGINLEFT:
		return vs.leftMarginWidth;

	case SCI_GETMARGINRIGHT:
		return vs.rightMarginWidth;

	case SCI_SETMARGINLEFT:
		vs.leftMarginWidth = lParam;
		InvalidateStyleRedraw();
		break;

	case SCI_SETMARGINRIGHT:
		vs.rightMarginWidth = lParam;
		InvalidateStyleRedraw();
		break;

		// Control specific mesages

	case SCI_ADDTEXT: {
			if (lParam == 0)
				return 0;
			pdoc->InsertString(CurrentPosition(), CharPtrFromSPtr(lParam), wParam);
			SetEmptySelection(sel.MainCaret() + wParam);
			return 0;
		}

	case SCI_ADDSTYLEDTEXT:
		if (lParam)
			AddStyledText(CharPtrFromSPtr(lParam), wParam);
		return 0;

	case SCI_INSERTTEXT: {
			if (lParam == 0)
				return 0;
			int insertPos = wParam;
			if (static_cast<int>(wParam) == -1)
				insertPos = CurrentPosition();
			int newCurrent = CurrentPosition();
			char *sz = CharPtrFromSPtr(lParam);
			pdoc->InsertCString(insertPos, sz);
			if (newCurrent > insertPos)
				newCurrent += istrlen(sz);
			SetEmptySelection(newCurrent);
			return 0;
		}

	case SCI_APPENDTEXT:
		pdoc->InsertString(pdoc->Length(), CharPtrFromSPtr(lParam), wParam);
		return 0;

	case SCI_CLEARALL:
		ClearAll();
		return 0;

	case SCI_DELETERANGE:
		pdoc->DeleteChars(wParam, lParam);
		return 0;

	case SCI_CLEARDOCUMENTSTYLE:
		ClearDocumentStyle();
		return 0;

	case SCI_SETUNDOCOLLECTION:
		pdoc->SetUndoCollection(wParam != 0);
		return 0;

	case SCI_GETUNDOCOLLECTION:
		return pdoc->IsCollectingUndo();

	case SCI_BEGINUNDOACTION:
		pdoc->BeginUndoAction();
		return 0;

	case SCI_ENDUNDOACTION:
		pdoc->EndUndoAction();
		return 0;

	case SCI_GETCARETPERIOD:
		return caret.period;

	case SCI_SETCARETPERIOD:
		caret.period = wParam;
		break;

	case SCI_GETWORDCHARS:
		return pdoc->GetCharsOfClass(CharClassify::ccWord, reinterpret_cast<unsigned char *>(lParam));

	case SCI_SETWORDCHARS: {
			pdoc->SetDefaultCharClasses(false);
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(reinterpret_cast<unsigned char *>(lParam), CharClassify::ccWord);
		}
		break;

	case SCI_GETWHITESPACECHARS:
		return pdoc->GetCharsOfClass(CharClassify::ccSpace, reinterpret_cast<unsigned char *>(lParam));

	case SCI_SETWHITESPACECHARS: {
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(reinterpret_cast<unsigned char *>(lParam), CharClassify::ccSpace);
		}
		break;

	case SCI_GETPUNCTUATIONCHARS:
		return pdoc->GetCharsOfClass(CharClassify::ccPunctuation, reinterpret_cast<unsigned char *>(lParam));

	case SCI_SETPUNCTUATIONCHARS: {
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(reinterpret_cast<unsigned char *>(lParam), CharClassify::ccPunctuation);
		}
		break;

	case SCI_SETCHARSDEFAULT:
		pdoc->SetDefaultCharClasses(true);
		break;

	case SCI_GETLENGTH:
		return pdoc->Length();

	case SCI_ALLOCATE:
		pdoc->Allocate(wParam);
		break;

	case SCI_GETCHARAT:
		return pdoc->CharAt(wParam);

	case SCI_SETCURRENTPOS:
		if (sel.IsRectangular()) {
			sel.Rectangular().caret.SetPosition(wParam);
			SetRectangularRange();
			Redraw();
		} else {
			SetSelection(wParam, sel.MainAnchor());
		}
		break;

	case SCI_GETCURRENTPOS:
		return sel.IsRectangular() ? sel.Rectangular().caret.Position() : sel.MainCaret();

	case SCI_SETANCHOR:
		if (sel.IsRectangular()) {
			sel.Rectangular().anchor.SetPosition(wParam);
			SetRectangularRange();
			Redraw();
		} else {
			SetSelection(sel.MainCaret(), wParam);
		}
		break;

	case SCI_GETANCHOR:
		return sel.IsRectangular() ? sel.Rectangular().anchor.Position() : sel.MainAnchor();

	case SCI_SETSELECTIONSTART:
		SetSelection(Platform::Maximum(sel.MainCaret(), wParam), wParam);
		break;

	case SCI_GETSELECTIONSTART:
		return sel.LimitsForRectangularElseMain().start.Position();

	case SCI_SETSELECTIONEND:
		SetSelection(wParam, Platform::Minimum(sel.MainAnchor(), wParam));
		break;

	case SCI_GETSELECTIONEND:
		return sel.LimitsForRectangularElseMain().end.Position();

	case SCI_SETEMPTYSELECTION:
		SetEmptySelection(wParam);
		break;

	case SCI_SETPRINTMAGNIFICATION:
		printMagnification = wParam;
		break;

	case SCI_GETPRINTMAGNIFICATION:
		return printMagnification;

	case SCI_SETPRINTCOLOURMODE:
		printColourMode = wParam;
		break;

	case SCI_GETPRINTCOLOURMODE:
		return printColourMode;

	case SCI_SETPRINTWRAPMODE:
		printWrapState = (wParam == SC_WRAP_WORD) ? eWrapWord : eWrapNone;
		break;

	case SCI_GETPRINTWRAPMODE:
		return printWrapState;

	case SCI_GETSTYLEAT:
		if (static_cast<int>(wParam) >= pdoc->Length())
			return 0;
		else
			return pdoc->StyleAt(wParam);

	case SCI_REDO:
		Redo();
		break;

	case SCI_SELECTALL:
		SelectAll();
		break;

	case SCI_SETSAVEPOINT:
		pdoc->SetSavePoint();
		break;

	case SCI_GETSTYLEDTEXT: {
			if (lParam == 0)
				return 0;
			Sci_TextRange *tr = reinterpret_cast<Sci_TextRange *>(lParam);
			int iPlace = 0;
			for (int iChar = tr->chrg.cpMin; iChar < tr->chrg.cpMax; iChar++) {
				tr->lpstrText[iPlace++] = pdoc->CharAt(iChar);
				tr->lpstrText[iPlace++] = pdoc->StyleAt(iChar);
			}
			tr->lpstrText[iPlace] = '\0';
			tr->lpstrText[iPlace + 1] = '\0';
			return iPlace;
		}

	case SCI_CANREDO:
		return (pdoc->CanRedo() && !pdoc->IsReadOnly()) ? 1 : 0;

	case SCI_MARKERLINEFROMHANDLE:
		return pdoc->LineFromHandle(wParam);

	case SCI_MARKERDELETEHANDLE:
		pdoc->DeleteMarkFromHandle(wParam);
		break;

	case SCI_GETVIEWWS:
		return vs.viewWhitespace;

	case SCI_SETVIEWWS:
		vs.viewWhitespace = static_cast<WhiteSpaceVisibility>(wParam);
		Redraw();
		break;

	case SCI_GETWHITESPACESIZE:
		return vs.whitespaceSize;

	case SCI_SETWHITESPACESIZE:
		vs.whitespaceSize = static_cast<int>(wParam);
		Redraw();
		break;

	case SCI_POSITIONFROMPOINT:
		return PositionFromLocation(Point(wParam, lParam), false, false);

	case SCI_POSITIONFROMPOINTCLOSE:
		return PositionFromLocation(Point(wParam, lParam), true, false);

	case SCI_CHARPOSITIONFROMPOINT:
		return PositionFromLocation(Point(wParam, lParam), false, true);

	case SCI_CHARPOSITIONFROMPOINTCLOSE:
		return PositionFromLocation(Point(wParam, lParam), true, true);

	case SCI_GOTOLINE:
		GoToLine(wParam);
		break;

	case SCI_GOTOPOS:
		SetEmptySelection(wParam);
		EnsureCaretVisible();
		break;

	case SCI_GETCURLINE: {
			int lineCurrentPos = pdoc->LineFromPosition(sel.MainCaret());
			int lineStart = pdoc->LineStart(lineCurrentPos);
			unsigned int lineEnd = pdoc->LineStart(lineCurrentPos + 1);
			if (lParam == 0) {
				return 1 + lineEnd - lineStart;
			}
			PLATFORM_ASSERT(wParam > 0);
			char *ptr = CharPtrFromSPtr(lParam);
			unsigned int iPlace = 0;
			for (unsigned int iChar = lineStart; iChar < lineEnd && iPlace < wParam - 1; iChar++) {
				ptr[iPlace++] = pdoc->CharAt(iChar);
			}
			ptr[iPlace] = '\0';
			return sel.MainCaret() - lineStart;
		}

	case SCI_GETENDSTYLED:
		return pdoc->GetEndStyled();

	case SCI_GETEOLMODE:
		return pdoc->eolMode;

	case SCI_SETEOLMODE:
		pdoc->eolMode = wParam;
		break;

	case SCI_SETLINEENDTYPESALLOWED:
		if (pdoc->SetLineEndTypesAllowed(wParam)) {
			cs.Clear();
			cs.InsertLines(0, pdoc->LinesTotal() - 1);
			SetAnnotationHeights(0, pdoc->LinesTotal());
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETLINEENDTYPESALLOWED:
		return pdoc->GetLineEndTypesAllowed();

	case SCI_GETLINEENDTYPESACTIVE:
		return pdoc->GetLineEndTypesActive();

	case SCI_STARTSTYLING:
		pdoc->StartStyling(wParam, static_cast<char>(lParam));
		break;

	case SCI_SETSTYLING:
		pdoc->SetStyleFor(wParam, static_cast<char>(lParam));
		break;

	case SCI_SETSTYLINGEX:             // Specify a complete styling buffer
		if (lParam == 0)
			return 0;
		pdoc->SetStyles(wParam, CharPtrFromSPtr(lParam));
		break;

	case SCI_SETBUFFEREDDRAW:
		bufferedDraw = wParam != 0;
		break;

	case SCI_GETBUFFEREDDRAW:
		return bufferedDraw;

	case SCI_GETTWOPHASEDRAW:
		return twoPhaseDraw;

	case SCI_SETTWOPHASEDRAW:
		twoPhaseDraw = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_SETFONTQUALITY:
		vs.extraFontFlag &= ~SC_EFF_QUALITY_MASK;
		vs.extraFontFlag |= (wParam & SC_EFF_QUALITY_MASK);
		InvalidateStyleRedraw();
		break;

	case SCI_GETFONTQUALITY:
		return (vs.extraFontFlag & SC_EFF_QUALITY_MASK);

	case SCI_SETTABWIDTH:
		if (wParam > 0) {
			pdoc->tabInChars = wParam;
			if (pdoc->indentInChars == 0)
				pdoc->actualIndentInChars = pdoc->tabInChars;
		}
		InvalidateStyleRedraw();
		break;

	case SCI_GETTABWIDTH:
		return pdoc->tabInChars;

	case SCI_SETINDENT:
		pdoc->indentInChars = wParam;
		if (pdoc->indentInChars != 0)
			pdoc->actualIndentInChars = pdoc->indentInChars;
		else
			pdoc->actualIndentInChars = pdoc->tabInChars;
		InvalidateStyleRedraw();
		break;

	case SCI_GETINDENT:
		return pdoc->indentInChars;

	case SCI_SETUSETABS:
		pdoc->useTabs = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETUSETABS:
		return pdoc->useTabs;

	case SCI_SETLINEINDENTATION:
		pdoc->SetLineIndentation(wParam, lParam);
		break;

	case SCI_GETLINEINDENTATION:
		return pdoc->GetLineIndentation(wParam);

	case SCI_GETLINEINDENTPOSITION:
		return pdoc->GetLineIndentPosition(wParam);

	case SCI_SETTABINDENTS:
		pdoc->tabIndents = wParam != 0;
		break;

	case SCI_GETTABINDENTS:
		return pdoc->tabIndents;

	case SCI_SETBACKSPACEUNINDENTS:
		pdoc->backspaceUnindents = wParam != 0;
		break;

	case SCI_GETBACKSPACEUNINDENTS:
		return pdoc->backspaceUnindents;

	case SCI_SETMOUSEDWELLTIME:
		dwellDelay = wParam;
		ticksToDwell = dwellDelay;
		break;

	case SCI_GETMOUSEDWELLTIME:
		return dwellDelay;

	case SCI_WORDSTARTPOSITION:
		return pdoc->ExtendWordSelect(wParam, -1, lParam != 0);

	case SCI_WORDENDPOSITION:
		return pdoc->ExtendWordSelect(wParam, 1, lParam != 0);

	case SCI_SETWRAPMODE:
		switch (wParam) {
		case SC_WRAP_WORD:
			wrapState = eWrapWord;
			break;
		case SC_WRAP_CHAR:
			wrapState = eWrapChar;
			break;
		default:
			wrapState = eWrapNone;
			break;
		}
		xOffset = 0;
		ContainerNeedsUpdate(SC_UPDATE_H_SCROLL);
		InvalidateStyleRedraw();
		ReconfigureScrollBars();
		break;

	case SCI_GETWRAPMODE:
		return wrapState;

	case SCI_SETWRAPVISUALFLAGS:
		if (wrapVisualFlags != static_cast<int>(wParam)) {
			wrapVisualFlags = wParam;
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETWRAPVISUALFLAGS:
		return wrapVisualFlags;

	case SCI_SETWRAPVISUALFLAGSLOCATION:
		wrapVisualFlagsLocation = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_GETWRAPVISUALFLAGSLOCATION:
		return wrapVisualFlagsLocation;

	case SCI_SETWRAPSTARTINDENT:
		if (wrapVisualStartIndent != static_cast<int>(wParam)) {
			wrapVisualStartIndent = wParam;
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETWRAPSTARTINDENT:
		return wrapVisualStartIndent;

	case SCI_SETWRAPINDENTMODE:
		if (wrapIndentMode != static_cast<int>(wParam)) {
			wrapIndentMode = wParam;
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETWRAPINDENTMODE:
		return wrapIndentMode;

	case SCI_SETLAYOUTCACHE:
		llc.SetLevel(wParam);
		break;

	case SCI_GETLAYOUTCACHE:
		return llc.GetLevel();

	case SCI_SETPOSITIONCACHE:
		posCache.SetSize(wParam);
		break;

	case SCI_GETPOSITIONCACHE:
		return posCache.GetSize();

	case SCI_SETSCROLLWIDTH:
		PLATFORM_ASSERT(wParam > 0);
		if ((wParam > 0) && (wParam != static_cast<unsigned int >(scrollWidth))) {
			lineWidthMaxSeen = 0;
			scrollWidth = wParam;
			SetScrollBars();
		}
		break;

	case SCI_GETSCROLLWIDTH:
		return scrollWidth;

	case SCI_SETSCROLLWIDTHTRACKING:
		trackLineWidth = wParam != 0;
		break;

	case SCI_GETSCROLLWIDTHTRACKING:
		return trackLineWidth;

	case SCI_LINESJOIN:
		LinesJoin();
		break;

	case SCI_LINESSPLIT:
		LinesSplit(wParam);
		break;

	case SCI_TEXTWIDTH:
		PLATFORM_ASSERT(wParam < vs.styles.size());
		PLATFORM_ASSERT(lParam);
		return TextWidth(wParam, CharPtrFromSPtr(lParam));

	case SCI_TEXTHEIGHT:
		return vs.lineHeight;

	case SCI_SETENDATLASTLINE:
		PLATFORM_ASSERT((wParam == 0) || (wParam == 1));
		if (endAtLastLine != (wParam != 0)) {
			endAtLastLine = wParam != 0;
			SetScrollBars();
		}
		break;

	case SCI_GETENDATLASTLINE:
		return endAtLastLine;

	case SCI_SETCARETSTICKY:
		PLATFORM_ASSERT(wParam <= SC_CARETSTICKY_WHITESPACE);
		if (wParam <= SC_CARETSTICKY_WHITESPACE) {
			caretSticky = wParam;
		}
		break;

	case SCI_GETCARETSTICKY:
		return caretSticky;

	case SCI_TOGGLECARETSTICKY:
		caretSticky = !caretSticky;
		break;

	case SCI_GETCOLUMN:
		return pdoc->GetColumn(wParam);

	case SCI_FINDCOLUMN:
		return pdoc->FindColumn(wParam, lParam);

	case SCI_SETHSCROLLBAR :
		if (horizontalScrollBarVisible != (wParam != 0)) {
			horizontalScrollBarVisible = wParam != 0;
			SetScrollBars();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETHSCROLLBAR:
		return horizontalScrollBarVisible;

	case SCI_SETVSCROLLBAR:
		if (verticalScrollBarVisible != (wParam != 0)) {
			verticalScrollBarVisible = wParam != 0;
			SetScrollBars();
			ReconfigureScrollBars();
			if (verticalScrollBarVisible)
				SetVerticalScrollPos();
		}
		break;

	case SCI_GETVSCROLLBAR:
		return verticalScrollBarVisible;

	case SCI_SETINDENTATIONGUIDES:
		vs.viewIndentationGuides = IndentView(wParam);
		Redraw();
		break;

	case SCI_GETINDENTATIONGUIDES:
		return vs.viewIndentationGuides;

	case SCI_SETHIGHLIGHTGUIDE:
		if ((highlightGuideColumn != static_cast<int>(wParam)) || (wParam > 0)) {
			highlightGuideColumn = wParam;
			Redraw();
		}
		break;

	case SCI_GETHIGHLIGHTGUIDE:
		return highlightGuideColumn;

	case SCI_GETLINEENDPOSITION:
		return pdoc->LineEnd(wParam);

	case SCI_SETCODEPAGE:
		if (ValidCodePage(wParam)) {
			if (pdoc->SetDBCSCodePage(wParam)) {
				cs.Clear();
				cs.InsertLines(0, pdoc->LinesTotal() - 1);
				SetAnnotationHeights(0, pdoc->LinesTotal());
				InvalidateStyleRedraw();
			}
		}
		break;

	case SCI_GETCODEPAGE:
		return pdoc->dbcsCodePage;

#ifdef INCLUDE_DEPRECATED_FEATURES
	case SCI_SETUSEPALETTE:
		InvalidateStyleRedraw();
		break;

	case SCI_GETUSEPALETTE:
		return 0;
#endif

		// Marker definition and setting
	case SCI_MARKERDEFINE:
		if (wParam <= MARKER_MAX) {
			vs.markers[wParam].markType = lParam;
			vs.CalcLargestMarkerHeight();
		}
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case SCI_MARKERSYMBOLDEFINED:
		if (wParam <= MARKER_MAX)
			return vs.markers[wParam].markType;
		else
			return 0;

	case SCI_MARKERSETFORE:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].fore = ColourDesired(lParam);
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case SCI_MARKERSETBACKSELECTED:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].backSelected = ColourDesired(lParam);
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case SCI_MARKERENABLEHIGHLIGHT:
		highlightDelimiter.isEnabled = wParam == 1;
		RedrawSelMargin();
		break;
	case SCI_MARKERSETBACK:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].back = ColourDesired(lParam);
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case SCI_MARKERSETALPHA:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].alpha = lParam;
		InvalidateStyleRedraw();
		break;
	case SCI_MARKERADD: {
			int markerID = pdoc->AddMark(wParam, lParam);
			return markerID;
		}
	case SCI_MARKERADDSET:
		if (lParam != 0)
			pdoc->AddMarkSet(wParam, lParam);
		break;

	case SCI_MARKERDELETE:
		pdoc->DeleteMark(wParam, lParam);
		break;

	case SCI_MARKERDELETEALL:
		pdoc->DeleteAllMarks(static_cast<int>(wParam));
		break;

	case SCI_MARKERGET:
		return pdoc->GetMark(wParam);

	case SCI_MARKERNEXT:
		return pdoc->MarkerNext(wParam, lParam);

	case SCI_MARKERPREVIOUS: {
			for (int iLine = wParam; iLine >= 0; iLine--) {
				if ((pdoc->GetMark(iLine) & lParam) != 0)
					return iLine;
			}
		}
		return -1;

	case SCI_MARKERDEFINEPIXMAP:
		if (wParam <= MARKER_MAX) {
			vs.markers[wParam].SetXPM(CharPtrFromSPtr(lParam));
			vs.CalcLargestMarkerHeight();
		};
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case SCI_RGBAIMAGESETWIDTH:
		sizeRGBAImage.x = wParam;
		break;

	case SCI_RGBAIMAGESETHEIGHT:
		sizeRGBAImage.y = wParam;
		break;

	case SCI_RGBAIMAGESETSCALE:
		scaleRGBAImage = wParam;
		break;

	case SCI_MARKERDEFINERGBAIMAGE:
		if (wParam <= MARKER_MAX) {
			vs.markers[wParam].SetRGBAImage(sizeRGBAImage, scaleRGBAImage / 100.0, reinterpret_cast<unsigned char *>(lParam));
			vs.CalcLargestMarkerHeight();
		};
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case SCI_SETMARGINTYPEN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].style = lParam;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETMARGINTYPEN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].style;
		else
			return 0;

	case SCI_SETMARGINWIDTHN:
		if (ValidMargin(wParam)) {
			// Short-circuit if the width is unchanged, to avoid unnecessary redraw.
			if (vs.ms[wParam].width != lParam) {
				vs.ms[wParam].width = lParam;
				InvalidateStyleRedraw();
			}
		}
		break;

	case SCI_GETMARGINWIDTHN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].width;
		else
			return 0;

	case SCI_SETMARGINMASKN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].mask = lParam;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETMARGINMASKN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].mask;
		else
			return 0;

	case SCI_SETMARGINSENSITIVEN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].sensitive = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETMARGINSENSITIVEN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].sensitive ? 1 : 0;
		else
			return 0;

	case SCI_SETMARGINCURSORN:
		if (ValidMargin(wParam))
			vs.ms[wParam].cursor = lParam;
		break;

	case SCI_GETMARGINCURSORN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].cursor;
		else
			return 0;

	case SCI_STYLECLEARALL:
		vs.ClearStyles();
		InvalidateStyleRedraw();
		break;

	case SCI_STYLESETFORE:
	case SCI_STYLESETBACK:
	case SCI_STYLESETBOLD:
	case SCI_STYLESETWEIGHT:
	case SCI_STYLESETITALIC:
	case SCI_STYLESETEOLFILLED:
	case SCI_STYLESETSIZE:
	case SCI_STYLESETSIZEFRACTIONAL:
	case SCI_STYLESETFONT:
	case SCI_STYLESETUNDERLINE:
	case SCI_STYLESETCASE:
	case SCI_STYLESETCHARACTERSET:
	case SCI_STYLESETVISIBLE:
	case SCI_STYLESETCHANGEABLE:
	case SCI_STYLESETHOTSPOT:
		StyleSetMessage(iMessage, wParam, lParam);
		break;

	case SCI_STYLEGETFORE:
	case SCI_STYLEGETBACK:
	case SCI_STYLEGETBOLD:
	case SCI_STYLEGETWEIGHT:
	case SCI_STYLEGETITALIC:
	case SCI_STYLEGETEOLFILLED:
	case SCI_STYLEGETSIZE:
	case SCI_STYLEGETSIZEFRACTIONAL:
	case SCI_STYLEGETFONT:
	case SCI_STYLEGETUNDERLINE:
	case SCI_STYLEGETCASE:
	case SCI_STYLEGETCHARACTERSET:
	case SCI_STYLEGETVISIBLE:
	case SCI_STYLEGETCHANGEABLE:
	case SCI_STYLEGETHOTSPOT:
		return StyleGetMessage(iMessage, wParam, lParam);

	case SCI_STYLERESETDEFAULT:
		vs.ResetDefaultStyle();
		InvalidateStyleRedraw();
		break;
	case SCI_SETSTYLEBITS:
		vs.EnsureStyle((1 << wParam) - 1);
		pdoc->SetStylingBits(wParam);
		break;

	case SCI_GETSTYLEBITS:
		return pdoc->stylingBits;

	case SCI_SETLINESTATE:
		return pdoc->SetLineState(wParam, lParam);

	case SCI_GETLINESTATE:
		return pdoc->GetLineState(wParam);

	case SCI_GETMAXLINESTATE:
		return pdoc->GetMaxLineState();

	case SCI_GETCARETLINEVISIBLE:
		return vs.showCaretLineBackground;
	case SCI_SETCARETLINEVISIBLE:
		vs.showCaretLineBackground = wParam != 0;
		InvalidateStyleRedraw();
		break;
	case SCI_GETCARETLINEVISIBLEALWAYS:
		return vs.alwaysShowCaretLineBackground;
	case SCI_SETCARETLINEVISIBLEALWAYS:
		vs.alwaysShowCaretLineBackground = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETLINEBACK:
		return vs.caretLineBackground.AsLong();
	case SCI_SETCARETLINEBACK:
		vs.caretLineBackground = wParam;
		InvalidateStyleRedraw();
		break;
	case SCI_GETCARETLINEBACKALPHA:
		return vs.caretLineAlpha;
	case SCI_SETCARETLINEBACKALPHA:
		vs.caretLineAlpha = wParam;
		InvalidateStyleRedraw();
		break;

		// Folding messages

	case SCI_VISIBLEFROMDOCLINE:
		return cs.DisplayFromDoc(wParam);

	case SCI_DOCLINEFROMVISIBLE:
		return cs.DocFromDisplay(wParam);

	case SCI_WRAPCOUNT:
		return WrapCount(wParam);

	case SCI_SETFOLDLEVEL: {
			int prev = pdoc->SetLevel(wParam, lParam);
			if (prev != lParam)
				RedrawSelMargin();
			return prev;
		}

	case SCI_GETFOLDLEVEL:
		return pdoc->GetLevel(wParam);

	case SCI_GETLASTCHILD:
		return pdoc->GetLastChild(wParam, lParam);

	case SCI_GETFOLDPARENT:
		return pdoc->GetFoldParent(wParam);

	case SCI_SHOWLINES:
		cs.SetVisible(wParam, lParam, true);
		SetScrollBars();
		Redraw();
		break;

	case SCI_HIDELINES:
		if (wParam > 0)
			cs.SetVisible(wParam, lParam, false);
		SetScrollBars();
		Redraw();
		break;

	case SCI_GETLINEVISIBLE:
		return cs.GetVisible(wParam);

	case SCI_GETALLLINESVISIBLE:
		return cs.HiddenLines() ? 0 : 1;

	case SCI_SETFOLDEXPANDED:
		SetFoldExpanded(wParam, lParam != 0);
		break;

	case SCI_GETFOLDEXPANDED:
		return cs.GetExpanded(wParam);

	case SCI_SETAUTOMATICFOLD:
		foldAutomatic = wParam;
		break;

	case SCI_GETAUTOMATICFOLD:
		return foldAutomatic;

	case SCI_SETFOLDFLAGS:
		foldFlags = wParam;
		Redraw();
		break;

	case SCI_TOGGLEFOLD:
		FoldLine(wParam, SC_FOLDACTION_TOGGLE);
		break;

	case SCI_FOLDLINE:
		FoldLine(wParam, lParam);
		break;

	case SCI_FOLDCHILDREN:
		FoldExpand(wParam, lParam, pdoc->GetLevel(wParam));
		break;

	case SCI_FOLDALL:
		FoldAll(wParam);
		break;

	case SCI_EXPANDCHILDREN:
		FoldExpand(wParam, SC_FOLDACTION_EXPAND, lParam);
		break;

	case SCI_CONTRACTEDFOLDNEXT:
		return ContractedFoldNext(wParam);

	case SCI_ENSUREVISIBLE:
		EnsureLineVisible(wParam, false);
		break;

	case SCI_ENSUREVISIBLEENFORCEPOLICY:
		EnsureLineVisible(wParam, true);
		break;

	case SCI_SCROLLRANGE:
		ScrollRange(SelectionRange(lParam, wParam));
		break;

	case SCI_SEARCHANCHOR:
		SearchAnchor();
		break;

	case SCI_SEARCHNEXT:
	case SCI_SEARCHPREV:
		return SearchText(iMessage, wParam, lParam);

	case SCI_SETXCARETPOLICY:
		caretXPolicy = wParam;
		caretXSlop = lParam;
		break;

	case SCI_SETYCARETPOLICY:
		caretYPolicy = wParam;
		caretYSlop = lParam;
		break;

	case SCI_SETVISIBLEPOLICY:
		visiblePolicy = wParam;
		visibleSlop = lParam;
		break;

	case SCI_LINESONSCREEN:
		return LinesOnScreen();

	case SCI_SETSELFORE:
		vs.selforeset = wParam != 0;
		vs.selforeground = ColourDesired(lParam);
		vs.selAdditionalForeground = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETSELBACK:
		vs.selbackset = wParam != 0;
		vs.selbackground = ColourDesired(lParam);
		vs.selAdditionalBackground = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETSELALPHA:
		vs.selAlpha = wParam;
		vs.selAdditionalAlpha = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_GETSELALPHA:
		return vs.selAlpha;

	case SCI_GETSELEOLFILLED:
		return vs.selEOLFilled;

	case SCI_SETSELEOLFILLED:
		vs.selEOLFilled = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_SETWHITESPACEFORE:
		vs.whitespaceForegroundSet = wParam != 0;
		vs.whitespaceForeground = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETWHITESPACEBACK:
		vs.whitespaceBackgroundSet = wParam != 0;
		vs.whitespaceBackground = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETCARETFORE:
		vs.caretcolour = ColourDesired(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETFORE:
		return vs.caretcolour.AsLong();

	case SCI_SETCARETSTYLE:
		if (wParam <= CARETSTYLE_BLOCK)
			vs.caretStyle = wParam;
		else
			/* Default to the line caret */
			vs.caretStyle = CARETSTYLE_LINE;
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETSTYLE:
		return vs.caretStyle;

	case SCI_SETCARETWIDTH:
		if (static_cast<int>(wParam) <= 0)
			vs.caretWidth = 0;
		else if (wParam >= 3)
			vs.caretWidth = 3;
		else
			vs.caretWidth = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETWIDTH:
		return vs.caretWidth;

	case SCI_ASSIGNCMDKEY:
		kmap.AssignCmdKey(Platform::LowShortFromLong(wParam),
		        Platform::HighShortFromLong(wParam), lParam);
		break;

	case SCI_CLEARCMDKEY:
		kmap.AssignCmdKey(Platform::LowShortFromLong(wParam),
		        Platform::HighShortFromLong(wParam), SCI_NULL);
		break;

	case SCI_CLEARALLCMDKEYS:
		kmap.Clear();
		break;

	case SCI_INDICSETSTYLE:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].style = lParam;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETSTYLE:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].style : 0;

	case SCI_INDICSETFORE:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].fore = ColourDesired(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETFORE:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].fore.AsLong() : 0;

	case SCI_INDICSETUNDER:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].under = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETUNDER:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].under : 0;

	case SCI_INDICSETALPHA:
		if (wParam <= INDIC_MAX && lParam >=0 && lParam <= 255) {
			vs.indicators[wParam].fillAlpha = lParam;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETALPHA:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].fillAlpha : 0;

	case SCI_INDICSETOUTLINEALPHA:
		if (wParam <= INDIC_MAX && lParam >=0 && lParam <= 255) {
			vs.indicators[wParam].outlineAlpha = lParam;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETOUTLINEALPHA:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].outlineAlpha : 0;

	case SCI_SETINDICATORCURRENT:
		pdoc->decorations.SetCurrentIndicator(wParam);
		break;
	case SCI_GETINDICATORCURRENT:
		return pdoc->decorations.GetCurrentIndicator();
	case SCI_SETINDICATORVALUE:
		pdoc->decorations.SetCurrentValue(wParam);
		break;
	case SCI_GETINDICATORVALUE:
		return pdoc->decorations.GetCurrentValue();

	case SCI_INDICATORFILLRANGE:
		pdoc->DecorationFillRange(wParam, pdoc->decorations.GetCurrentValue(), lParam);
		break;

	case SCI_INDICATORCLEARRANGE:
		pdoc->DecorationFillRange(wParam, 0, lParam);
		break;

	case SCI_INDICATORALLONFOR:
		return pdoc->decorations.AllOnFor(wParam);

	case SCI_INDICATORVALUEAT:
		return pdoc->decorations.ValueAt(wParam, lParam);

	case SCI_INDICATORSTART:
		return pdoc->decorations.Start(wParam, lParam);

	case SCI_INDICATOREND:
		return pdoc->decorations.End(wParam, lParam);

	case SCI_LINEDOWN:
	case SCI_LINEDOWNEXTEND:
	case SCI_PARADOWN:
	case SCI_PARADOWNEXTEND:
	case SCI_LINEUP:
	case SCI_LINEUPEXTEND:
	case SCI_PARAUP:
	case SCI_PARAUPEXTEND:
	case SCI_CHARLEFT:
	case SCI_CHARLEFTEXTEND:
	case SCI_CHARRIGHT:
	case SCI_CHARRIGHTEXTEND:
	case SCI_WORDLEFT:
	case SCI_WORDLEFTEXTEND:
	case SCI_WORDRIGHT:
	case SCI_WORDRIGHTEXTEND:
	case SCI_WORDLEFTEND:
	case SCI_WORDLEFTENDEXTEND:
	case SCI_WORDRIGHTEND:
	case SCI_WORDRIGHTENDEXTEND:
	case SCI_HOME:
	case SCI_HOMEEXTEND:
	case SCI_LINEEND:
	case SCI_LINEENDEXTEND:
	case SCI_HOMEWRAP:
	case SCI_HOMEWRAPEXTEND:
	case SCI_LINEENDWRAP:
	case SCI_LINEENDWRAPEXTEND:
	case SCI_DOCUMENTSTART:
	case SCI_DOCUMENTSTARTEXTEND:
	case SCI_DOCUMENTEND:
	case SCI_DOCUMENTENDEXTEND:
	case SCI_SCROLLTOSTART:
	case SCI_SCROLLTOEND:

	case SCI_STUTTEREDPAGEUP:
	case SCI_STUTTEREDPAGEUPEXTEND:
	case SCI_STUTTEREDPAGEDOWN:
	case SCI_STUTTEREDPAGEDOWNEXTEND:

	case SCI_PAGEUP:
	case SCI_PAGEUPEXTEND:
	case SCI_PAGEDOWN:
	case SCI_PAGEDOWNEXTEND:
	case SCI_EDITTOGGLEOVERTYPE:
	case SCI_CANCEL:
	case SCI_DELETEBACK:
	case SCI_TAB:
	case SCI_BACKTAB:
	case SCI_NEWLINE:
	case SCI_FORMFEED:
	case SCI_VCHOME:
	case SCI_VCHOMEEXTEND:
	case SCI_VCHOMEWRAP:
	case SCI_VCHOMEWRAPEXTEND:
	case SCI_VCHOMEDISPLAY:
	case SCI_VCHOMEDISPLAYEXTEND:
	case SCI_ZOOMIN:
	case SCI_ZOOMOUT:
	case SCI_DELWORDLEFT:
	case SCI_DELWORDRIGHT:
	case SCI_DELWORDRIGHTEND:
	case SCI_DELLINELEFT:
	case SCI_DELLINERIGHT:
	case SCI_LINECOPY:
	case SCI_LINECUT:
	case SCI_LINEDELETE:
	case SCI_LINETRANSPOSE:
	case SCI_LINEDUPLICATE:
	case SCI_LOWERCASE:
	case SCI_UPPERCASE:
	case SCI_LINESCROLLDOWN:
	case SCI_LINESCROLLUP:
	case SCI_WORDPARTLEFT:
	case SCI_WORDPARTLEFTEXTEND:
	case SCI_WORDPARTRIGHT:
	case SCI_WORDPARTRIGHTEXTEND:
	case SCI_DELETEBACKNOTLINE:
	case SCI_HOMEDISPLAY:
	case SCI_HOMEDISPLAYEXTEND:
	case SCI_LINEENDDISPLAY:
	case SCI_LINEENDDISPLAYEXTEND:
	case SCI_LINEDOWNRECTEXTEND:
	case SCI_LINEUPRECTEXTEND:
	case SCI_CHARLEFTRECTEXTEND:
	case SCI_CHARRIGHTRECTEXTEND:
	case SCI_HOMERECTEXTEND:
	case SCI_VCHOMERECTEXTEND:
	case SCI_LINEENDRECTEXTEND:
	case SCI_PAGEUPRECTEXTEND:
	case SCI_PAGEDOWNRECTEXTEND:
	case SCI_SELECTIONDUPLICATE:
		return KeyCommand(iMessage);

	case SCI_BRACEHIGHLIGHT:
		SetBraceHighlight(static_cast<int>(wParam), lParam, STYLE_BRACELIGHT);
		break;

	case SCI_BRACEHIGHLIGHTINDICATOR:
		if (lParam >= 0 && lParam <= INDIC_MAX) {
			vs.braceHighlightIndicatorSet = wParam != 0;
			vs.braceHighlightIndicator = lParam;
		}
		break;

	case SCI_BRACEBADLIGHT:
		SetBraceHighlight(static_cast<int>(wParam), -1, STYLE_BRACEBAD);
		break;

	case SCI_BRACEBADLIGHTINDICATOR:
		if (lParam >= 0 && lParam <= INDIC_MAX) {
			vs.braceBadLightIndicatorSet = wParam != 0;
			vs.braceBadLightIndicator = lParam;
		}
		break;

	case SCI_BRACEMATCH:
		// wParam is position of char to find brace for,
		// lParam is maximum amount of text to restyle to find it
		return pdoc->BraceMatch(wParam, lParam);

	case SCI_GETVIEWEOL:
		return vs.viewEOL;

	case SCI_SETVIEWEOL:
		vs.viewEOL = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_SETZOOM:
		vs.zoomLevel = wParam;
		InvalidateStyleRedraw();
		NotifyZoom();
		break;

	case SCI_GETZOOM:
		return vs.zoomLevel;

	case SCI_GETEDGECOLUMN:
		return theEdge;

	case SCI_SETEDGECOLUMN:
		theEdge = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_GETEDGEMODE:
		return vs.edgeState;

	case SCI_SETEDGEMODE:
		vs.edgeState = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_GETEDGECOLOUR:
		return vs.edgecolour.AsLong();

	case SCI_SETEDGECOLOUR:
		vs.edgecolour = ColourDesired(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETDOCPOINTER:
		return reinterpret_cast<sptr_t>(pdoc);

	case SCI_SETDOCPOINTER:
		CancelModes();
		SetDocPointer(reinterpret_cast<Document *>(lParam));
		return 0;

	case SCI_CREATEDOCUMENT: {
			Document *doc = new Document();
			doc->AddRef();
			return reinterpret_cast<sptr_t>(doc);
		}

	case SCI_ADDREFDOCUMENT:
		(reinterpret_cast<Document *>(lParam))->AddRef();
		break;

	case SCI_RELEASEDOCUMENT:
		(reinterpret_cast<Document *>(lParam))->Release();
		break;

	case SCI_CREATELOADER: {
			Document *doc = new Document();
			doc->AddRef();
			doc->Allocate(wParam);
			doc->SetUndoCollection(false);
			return reinterpret_cast<sptr_t>(static_cast<ILoader *>(doc));
		}

	case SCI_SETMODEVENTMASK:
		modEventMask = wParam;
		return 0;

	case SCI_GETMODEVENTMASK:
		return modEventMask;

	case SCI_CONVERTEOLS:
		pdoc->ConvertLineEnds(wParam);
		SetSelection(sel.MainCaret(), sel.MainAnchor());	// Ensure selection inside document
		return 0;

	case SCI_SETLENGTHFORENCODE:
		lengthForEncode = wParam;
		return 0;

	case SCI_SELECTIONISRECTANGLE:
		return sel.selType == Selection::selRectangle ? 1 : 0;

	case SCI_SETSELECTIONMODE: {
			switch (wParam) {
			case SC_SEL_STREAM:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selStream));
				sel.selType = Selection::selStream;
				break;
			case SC_SEL_RECTANGLE:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selRectangle));
				sel.selType = Selection::selRectangle;
				break;
			case SC_SEL_LINES:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selLines));
				sel.selType = Selection::selLines;
				break;
			case SC_SEL_THIN:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selThin));
				sel.selType = Selection::selThin;
				break;
			default:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selStream));
				sel.selType = Selection::selStream;
			}
			InvalidateSelection(sel.RangeMain(), true);
		}
	case SCI_GETSELECTIONMODE:
		switch (sel.selType) {
		case Selection::selStream:
			return SC_SEL_STREAM;
		case Selection::selRectangle:
			return SC_SEL_RECTANGLE;
		case Selection::selLines:
			return SC_SEL_LINES;
		case Selection::selThin:
			return SC_SEL_THIN;
		default:	// ?!
			return SC_SEL_STREAM;
		}
	case SCI_GETLINESELSTARTPOSITION:
	case SCI_GETLINESELENDPOSITION: {
			SelectionSegment segmentLine(SelectionPosition(pdoc->LineStart(wParam)),
				SelectionPosition(pdoc->LineEnd(wParam)));
			for (size_t r=0; r<sel.Count(); r++) {
				SelectionSegment portion = sel.Range(r).Intersect(segmentLine);
				if (portion.start.IsValid()) {
					return (iMessage == SCI_GETLINESELSTARTPOSITION) ? portion.start.Position() : portion.end.Position();
				}
			}
			return INVALID_POSITION;
		}

	case SCI_SETOVERTYPE:
		inOverstrike = wParam != 0;
		break;

	case SCI_GETOVERTYPE:
		return inOverstrike ? 1 : 0;

	case SCI_SETFOCUS:
		SetFocusState(wParam != 0);
		break;

	case SCI_GETFOCUS:
		return hasFocus;

	case SCI_SETSTATUS:
		errorStatus = wParam;
		break;

	case SCI_GETSTATUS:
		return errorStatus;

	case SCI_SETMOUSEDOWNCAPTURES:
		mouseDownCaptures = wParam != 0;
		break;

	case SCI_GETMOUSEDOWNCAPTURES:
		return mouseDownCaptures;

	case SCI_SETCURSOR:
		cursorMode = wParam;
		DisplayCursor(Window::cursorText);
		break;

	case SCI_GETCURSOR:
		return cursorMode;

	case SCI_SETCONTROLCHARSYMBOL:
		controlCharSymbol = wParam;
		break;

	case SCI_GETCONTROLCHARSYMBOL:
		return controlCharSymbol;

	case SCI_STARTRECORD:
		recordingMacro = true;
		return 0;

	case SCI_STOPRECORD:
		recordingMacro = false;
		return 0;

	case SCI_MOVECARETINSIDEVIEW:
		MoveCaretInsideView();
		break;

	case SCI_SETFOLDMARGINCOLOUR:
		vs.foldmarginColourSet = wParam != 0;
		vs.foldmarginColour = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETFOLDMARGINHICOLOUR:
		vs.foldmarginHighlightColourSet = wParam != 0;
		vs.foldmarginHighlightColour = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETHOTSPOTACTIVEFORE:
		vs.hotspotForegroundSet = wParam != 0;
		vs.hotspotForeground = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTACTIVEFORE:
		return vs.hotspotForeground.AsLong();

	case SCI_SETHOTSPOTACTIVEBACK:
		vs.hotspotBackgroundSet = wParam != 0;
		vs.hotspotBackground = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTACTIVEBACK:
		return vs.hotspotBackground.AsLong();

	case SCI_SETHOTSPOTACTIVEUNDERLINE:
		vs.hotspotUnderline = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTACTIVEUNDERLINE:
		return vs.hotspotUnderline ? 1 : 0;

	case SCI_SETHOTSPOTSINGLELINE:
		vs.hotspotSingleLine = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTSINGLELINE:
		return vs.hotspotSingleLine ? 1 : 0;

	case SCI_SETPASTECONVERTENDINGS:
		convertPastes = wParam != 0;
		break;

	case SCI_GETPASTECONVERTENDINGS:
		return convertPastes ? 1 : 0;

	case SCI_GETCHARACTERPOINTER:
		return reinterpret_cast<sptr_t>(pdoc->BufferPointer());

	case SCI_GETRANGEPOINTER:
		return reinterpret_cast<sptr_t>(pdoc->RangePointer(wParam, lParam));

	case SCI_GETGAPPOSITION:
		return pdoc->GapPosition();

	case SCI_SETEXTRAASCENT:
		vs.extraAscent = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_GETEXTRAASCENT:
		return vs.extraAscent;

	case SCI_SETEXTRADESCENT:
		vs.extraDescent = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_GETEXTRADESCENT:
		return vs.extraDescent;

	case SCI_MARGINSETSTYLEOFFSET:
		vs.marginStyleOffset = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_MARGINGETSTYLEOFFSET:
		return vs.marginStyleOffset;

	case SCI_SETMARGINOPTIONS:
		marginOptions = wParam;
		break;

	case SCI_GETMARGINOPTIONS:
		return marginOptions;

	case SCI_MARGINSETTEXT:
		pdoc->MarginSetText(wParam, CharPtrFromSPtr(lParam));
		break;

	case SCI_MARGINGETTEXT: {
			const StyledText st = pdoc->MarginStyledText(wParam);
			if (lParam) {
				if (st.text)
					memcpy(CharPtrFromSPtr(lParam), st.text, st.length);
				else
					strcpy(CharPtrFromSPtr(lParam), "");
			}
			return st.length;
		}

	case SCI_MARGINSETSTYLE:
		pdoc->MarginSetStyle(wParam, lParam);
		break;

	case SCI_MARGINGETSTYLE: {
			const StyledText st = pdoc->MarginStyledText(wParam);
			return st.style;
		}

	case SCI_MARGINSETSTYLES:
		pdoc->MarginSetStyles(wParam, reinterpret_cast<const unsigned char *>(lParam));
		break;

	case SCI_MARGINGETSTYLES: {
			const StyledText st = pdoc->MarginStyledText(wParam);
			if (lParam) {
				if (st.styles)
					memcpy(CharPtrFromSPtr(lParam), st.styles, st.length);
				else
					strcpy(CharPtrFromSPtr(lParam), "");
			}
			return st.styles ? st.length : 0;
		}

	case SCI_MARGINTEXTCLEARALL:
		pdoc->MarginClearAll();
		break;

	case SCI_ANNOTATIONSETTEXT:
		pdoc->AnnotationSetText(wParam, CharPtrFromSPtr(lParam));
		break;

	case SCI_ANNOTATIONGETTEXT: {
			const StyledText st = pdoc->AnnotationStyledText(wParam);
			if (lParam) {
				if (st.text)
					memcpy(CharPtrFromSPtr(lParam), st.text, st.length);
				else
					strcpy(CharPtrFromSPtr(lParam), "");
			}
			return st.length;
		}

	case SCI_ANNOTATIONGETSTYLE: {
			const StyledText st = pdoc->AnnotationStyledText(wParam);
			return st.style;
		}

	case SCI_ANNOTATIONSETSTYLE:
		pdoc->AnnotationSetStyle(wParam, lParam);
		break;

	case SCI_ANNOTATIONSETSTYLES:
		pdoc->AnnotationSetStyles(wParam, reinterpret_cast<const unsigned char *>(lParam));
		break;

	case SCI_ANNOTATIONGETSTYLES: {
			const StyledText st = pdoc->AnnotationStyledText(wParam);
			if (lParam) {
				if (st.styles)
					memcpy(CharPtrFromSPtr(lParam), st.styles, st.length);
				else
					strcpy(CharPtrFromSPtr(lParam), "");
			}
			return st.styles ? st.length : 0;
		}

	case SCI_ANNOTATIONGETLINES:
		return pdoc->AnnotationLines(wParam);

	case SCI_ANNOTATIONCLEARALL:
		pdoc->AnnotationClearAll();
		break;

	case SCI_ANNOTATIONSETVISIBLE:
		SetAnnotationVisible(wParam);
		break;

	case SCI_ANNOTATIONGETVISIBLE:
		return vs.annotationVisible;

	case SCI_ANNOTATIONSETSTYLEOFFSET:
		vs.annotationStyleOffset = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_ANNOTATIONGETSTYLEOFFSET:
		return vs.annotationStyleOffset;

	case SCI_RELEASEALLEXTENDEDSTYLES:
		vs.ReleaseAllExtendedStyles();
		break;

	case SCI_ALLOCATEEXTENDEDSTYLES:
		return vs.AllocateExtendedStyles(wParam);

	case SCI_ADDUNDOACTION:
		pdoc->AddUndoAction(wParam, lParam & UNDO_MAY_COALESCE);
		break;

	case SCI_SETMULTIPLESELECTION:
		multipleSelection = wParam != 0;
		InvalidateCaret();
		break;

	case SCI_GETMULTIPLESELECTION:
		return multipleSelection;

	case SCI_SETADDITIONALSELECTIONTYPING:
		additionalSelectionTyping = wParam != 0;
		InvalidateCaret();
		break;

	case SCI_GETADDITIONALSELECTIONTYPING:
		return additionalSelectionTyping;

	case SCI_SETMULTIPASTE:
		multiPasteMode = wParam;
		break;

	case SCI_GETMULTIPASTE:
		return multiPasteMode;

	case SCI_SETADDITIONALCARETSBLINK:
		additionalCaretsBlink = wParam != 0;
		InvalidateCaret();
		break;

	case SCI_GETADDITIONALCARETSBLINK:
		return additionalCaretsBlink;

	case SCI_SETADDITIONALCARETSVISIBLE:
		additionalCaretsVisible = wParam != 0;
		InvalidateCaret();
		break;

	case SCI_GETADDITIONALCARETSVISIBLE:
		return additionalCaretsVisible;

	case SCI_GETSELECTIONS:
		return sel.Count();

	case SCI_GETSELECTIONEMPTY:
		return sel.Empty();

	case SCI_CLEARSELECTIONS:
		sel.Clear();
		Redraw();
		break;

	case SCI_SETSELECTION:
		sel.SetSelection(SelectionRange(wParam, lParam));
		Redraw();
		break;

	case SCI_ADDSELECTION:
		sel.AddSelection(SelectionRange(wParam, lParam));
		Redraw();
		break;

	case SCI_SETMAINSELECTION:
		sel.SetMain(wParam);
		Redraw();
		break;

	case SCI_GETMAINSELECTION:
		return sel.Main();

	case SCI_SETSELECTIONNCARET:
		sel.Range(wParam).caret.SetPosition(lParam);
		Redraw();
		break;

	case SCI_GETSELECTIONNCARET:
		return sel.Range(wParam).caret.Position();

	case SCI_SETSELECTIONNANCHOR:
		sel.Range(wParam).anchor.SetPosition(lParam);
		Redraw();
		break;
	case SCI_GETSELECTIONNANCHOR:
		return sel.Range(wParam).anchor.Position();

	case SCI_SETSELECTIONNCARETVIRTUALSPACE:
		sel.Range(wParam).caret.SetVirtualSpace(lParam);
		Redraw();
		break;

	case SCI_GETSELECTIONNCARETVIRTUALSPACE:
		return sel.Range(wParam).caret.VirtualSpace();

	case SCI_SETSELECTIONNANCHORVIRTUALSPACE:
		sel.Range(wParam).anchor.SetVirtualSpace(lParam);
		Redraw();
		break;

	case SCI_GETSELECTIONNANCHORVIRTUALSPACE:
		return sel.Range(wParam).anchor.VirtualSpace();

	case SCI_SETSELECTIONNSTART:
		sel.Range(wParam).anchor.SetPosition(lParam);
		Redraw();
		break;

	case SCI_GETSELECTIONNSTART:
		return sel.Range(wParam).Start().Position();

	case SCI_SETSELECTIONNEND:
		sel.Range(wParam).caret.SetPosition(lParam);
		Redraw();
		break;

	case SCI_GETSELECTIONNEND:
		return sel.Range(wParam).End().Position();

	case SCI_SETRECTANGULARSELECTIONCARET:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::selRectangle;
		sel.Rectangular().caret.SetPosition(wParam);
		SetRectangularRange();
		Redraw();
		break;

	case SCI_GETRECTANGULARSELECTIONCARET:
		return sel.Rectangular().caret.Position();

	case SCI_SETRECTANGULARSELECTIONANCHOR:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::selRectangle;
		sel.Rectangular().anchor.SetPosition(wParam);
		SetRectangularRange();
		Redraw();
		break;

	case SCI_GETRECTANGULARSELECTIONANCHOR:
		return sel.Rectangular().anchor.Position();

	case SCI_SETRECTANGULARSELECTIONCARETVIRTUALSPACE:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::selRectangle;
		sel.Rectangular().caret.SetVirtualSpace(wParam);
		SetRectangularRange();
		Redraw();
		break;

	case SCI_GETRECTANGULARSELECTIONCARETVIRTUALSPACE:
		return sel.Rectangular().caret.VirtualSpace();

	case SCI_SETRECTANGULARSELECTIONANCHORVIRTUALSPACE:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::selRectangle;
		sel.Rectangular().anchor.SetVirtualSpace(wParam);
		SetRectangularRange();
		Redraw();
		break;

	case SCI_GETRECTANGULARSELECTIONANCHORVIRTUALSPACE:
		return sel.Rectangular().anchor.VirtualSpace();

	case SCI_SETVIRTUALSPACEOPTIONS:
		virtualSpaceOptions = wParam;
		break;

	case SCI_GETVIRTUALSPACEOPTIONS:
		return virtualSpaceOptions;

	case SCI_SETADDITIONALSELFORE:
		vs.selAdditionalForeground = ColourDesired(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETADDITIONALSELBACK:
		vs.selAdditionalBackground = ColourDesired(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETADDITIONALSELALPHA:
		vs.selAdditionalAlpha = wParam;
		InvalidateStyleRedraw();
		break;

	case SCI_GETADDITIONALSELALPHA:
		return vs.selAdditionalAlpha;

	case SCI_SETADDITIONALCARETFORE:
		vs.additionalCaretColour = ColourDesired(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETADDITIONALCARETFORE:
		return vs.additionalCaretColour.AsLong();

	case SCI_ROTATESELECTION:
		sel.RotateMain();
		InvalidateSelection(sel.RangeMain(), true);
		break;

	case SCI_SWAPMAINANCHORCARET:
		InvalidateSelection(sel.RangeMain());
		sel.RangeMain() = SelectionRange(sel.RangeMain().anchor, sel.RangeMain().caret);
		break;

	case SCI_CHANGELEXERSTATE:
		pdoc->ChangeLexerState(wParam, lParam);
		break;

	case SCI_SETIDENTIFIER:
		SetCtrlID(wParam);
		break;

	case SCI_GETIDENTIFIER:
		return GetCtrlID();

	case SCI_SETTECHNOLOGY:
		// No action by default
		break;

	case SCI_GETTECHNOLOGY:
		return technology;

	case SCI_COUNTCHARACTERS:
		return pdoc->CountCharacters(wParam, lParam);

	default:
		return DefWndProc(iMessage, wParam, lParam);
	}
	//Platform::DebugPrintf("end wnd proc\n");
	return 0l;
}
