// Scintilla source code edit control
/** @file Editor.cxx
 ** Main code for the edit control.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "Platform.h"

#ifndef PLAT_QT
#define INCLUDE_DEPRECATED_FEATURES
#endif
#include "Scintilla.h"

#include "ContractionState.h"
#include "SVector.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "CellBuffer.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Document.h"
#include "Editor.h"

/*
	return whether this modification represents an operation that
	may reasonably be deferred (not done now OR [possibly] at all)
*/
static bool CanDeferToLastStep(const DocModification& mh) {
	if (mh.modificationType & (SC_MOD_BEFOREINSERT|SC_MOD_BEFOREDELETE))
		return true;	// CAN skip
	if (!(mh.modificationType & (SC_PERFORMED_UNDO|SC_PERFORMED_REDO)))
		return false;	// MUST do
	if (mh.modificationType & SC_MULTISTEPUNDOREDO)
		return true;	// CAN skip
	return false;		// PRESUMABLY must do
}

static bool CanEliminate(const DocModification& mh) {
	return
		(mh.modificationType & (SC_MOD_BEFOREINSERT|SC_MOD_BEFOREDELETE)) != 0;
}

/*
	return whether this modification represents the FINAL step
	in a [possibly lengthy] multi-step Undo/Redo sequence
*/
static bool IsLastStep(const DocModification& mh) {
	return
		(mh.modificationType & (SC_PERFORMED_UNDO|SC_PERFORMED_REDO)) != 0
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

LineLayout::LineLayout(int maxLineLength_) :
	lineStarts(0),
	lenLineStarts(0),
	lineNumber(-1),
	inCache(false),
	maxLineLength(-1),
	numCharsInLine(0),
	validity(llInvalid),
	xHighlightGuide(0),
	highlightColumn(0),
	selStart(0),
	selEnd(0),
	containsCaret(false),
	edgeColumn(0),
	chars(0),
	styles(0),
	styleBitsSet(0),
	indicators(0),
	positions(0),
	hsStart(0),
	hsEnd(0),
	widthLine(wrapWidthInfinite),
	lines(1) {
	Resize(maxLineLength_);
}

LineLayout::~LineLayout() {
	Free();
}

void LineLayout::Resize(int maxLineLength_) {
	if (maxLineLength_ > maxLineLength) {
		Free();
		chars = new char[maxLineLength_ + 1];
		styles = new unsigned char[maxLineLength_ + 1];
		indicators = new char[maxLineLength_ + 1];
		// Extra position allocated as sometimes the Windows
		// GetTextExtentExPoint API writes an extra element.
		positions = new int[maxLineLength_ + 1 + 1];
		maxLineLength = maxLineLength_;
	}
}

void LineLayout::Free() {
	delete []chars;
	chars = 0;
	delete []styles;
	styles = 0;
	delete []indicators;
	indicators = 0;
	delete []positions;
	positions = 0;
	delete []lineStarts;
	lineStarts = 0;
}

void LineLayout::Invalidate(validLevel validity_) {
	if (validity > validity_)
		validity = validity_;
}

void LineLayout::SetLineStart(int line, int start) {
	if ((line >= lenLineStarts) && (line != 0)) {
		int newMaxLines = line + 20;
		int *newLineStarts = new int[newMaxLines];
		if (!newLineStarts)
			return;
		for (int i = 0; i < newMaxLines; i++) {
			if (i < lenLineStarts)
				newLineStarts[i] = lineStarts[i];
			else
				newLineStarts[i] = 0;
		}
		delete []lineStarts;
		lineStarts = newLineStarts;
		lenLineStarts = newMaxLines;
	}
	lineStarts[line] = start;
}

void LineLayout::SetBracesHighlight(Range rangeLine, Position braces[],
                                    char bracesMatchStyle, int xHighlight) {
	if (rangeLine.ContainsCharacter(braces[0])) {
		int braceOffset = braces[0] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			bracePreviousStyles[0] = styles[braceOffset];
			styles[braceOffset] = bracesMatchStyle;
		}
	}
	if (rangeLine.ContainsCharacter(braces[1])) {
		int braceOffset = braces[1] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			bracePreviousStyles[1] = styles[braceOffset];
			styles[braceOffset] = bracesMatchStyle;
		}
	}
	if ((braces[0] >= rangeLine.start && braces[1] <= rangeLine.end) ||
	        (braces[1] >= rangeLine.start && braces[0] <= rangeLine.end)) {
		xHighlightGuide = xHighlight;
	}
}

void LineLayout::RestoreBracesHighlight(Range rangeLine, Position braces[]) {
	if (rangeLine.ContainsCharacter(braces[0])) {
		int braceOffset = braces[0] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			styles[braceOffset] = bracePreviousStyles[0];
		}
	}
	if (rangeLine.ContainsCharacter(braces[1])) {
		int braceOffset = braces[1] - rangeLine.start;
		if (braceOffset < numCharsInLine) {
			styles[braceOffset] = bracePreviousStyles[1];
		}
	}
	xHighlightGuide = 0;
}

LineLayoutCache::LineLayoutCache() :
	level(0), length(0), size(0), cache(0),
	allInvalidated(false), styleClock(-1), useCount(0) {
	Allocate(0);
}

LineLayoutCache::~LineLayoutCache() {
	Deallocate();
}

void LineLayoutCache::Allocate(int length_) {
	PLATFORM_ASSERT(cache == NULL);
	allInvalidated = false;
	length = length_;
	size = length;
	if (size > 1) {
		size = (size / 16 + 1) * 16;
	}
	if (size > 0) {
		cache = new LineLayout * [size];
	}
	for (int i = 0; i < size; i++)
		cache[i] = 0;
}

void LineLayoutCache::AllocateForLevel(int linesOnScreen, int linesInDoc) {
	PLATFORM_ASSERT(useCount == 0);
	int lengthForLevel = 0;
	if (level == llcCaret) {
		lengthForLevel = 1;
	} else if (level == llcPage) {
		lengthForLevel = linesOnScreen + 1;
	} else if (level == llcDocument) {
		lengthForLevel = linesInDoc;
	}
	if (lengthForLevel > size) {
		Deallocate();
		Allocate(lengthForLevel);
	} else {
		if (lengthForLevel < length) {
			for (int i = lengthForLevel; i < length; i++) {
				delete cache[i];
				cache[i] = 0;
			}
		}
		length = lengthForLevel;
	}
	PLATFORM_ASSERT(length == lengthForLevel);
	PLATFORM_ASSERT(cache != NULL || length == 0);
}

void LineLayoutCache::Deallocate() {
	PLATFORM_ASSERT(useCount == 0);
	for (int i = 0; i < length; i++)
		delete cache[i];
	delete []cache;
	cache = 0;
	length = 0;
	size = 0;
}

void LineLayoutCache::Invalidate(LineLayout::validLevel validity_) {
	if (cache && !allInvalidated) {
		for (int i = 0; i < length; i++) {
			if (cache[i]) {
				cache[i]->Invalidate(validity_);
			}
		}
		if (validity_ == LineLayout::llInvalid) {
			allInvalidated = true;
		}
	}
}

void LineLayoutCache::SetLevel(int level_) {
	allInvalidated = false;
	if ((level_ != -1) && (level != level_)) {
		level = level_;
		Deallocate();
	}
}

LineLayout *LineLayoutCache::Retrieve(int lineNumber, int lineCaret, int maxChars, int styleClock_,
                                      int linesOnScreen, int linesInDoc) {
	AllocateForLevel(linesOnScreen, linesInDoc);
	if (styleClock != styleClock_) {
		Invalidate(LineLayout::llCheckTextAndStyle);
		styleClock = styleClock_;
	}
	allInvalidated = false;
	int pos = -1;
	LineLayout *ret = 0;
	if (level == llcCaret) {
		pos = 0;
	} else if (level == llcPage) {
		if (lineNumber == lineCaret) {
			pos = 0;
		} else if (length > 1) {
			pos = 1 + (lineNumber % (length - 1));
		}
	} else if (level == llcDocument) {
		pos = lineNumber;
	}
	if (pos >= 0) {
		PLATFORM_ASSERT(useCount == 0);
		if (cache && (pos < length)) {
			if (cache[pos]) {
				if ((cache[pos]->lineNumber != lineNumber) ||
				        (cache[pos]->maxLineLength < maxChars)) {
					delete cache[pos];
					cache[pos] = 0;
				}
			}
			if (!cache[pos]) {
				cache[pos] = new LineLayout(maxChars);
			}
			if (cache[pos]) {
				cache[pos]->lineNumber = lineNumber;
				cache[pos]->inCache = true;
				ret = cache[pos];
				useCount++;
			}
		}
	}

	if (!ret) {
		ret = new LineLayout(maxChars);
		ret->lineNumber = lineNumber;
	}

	return ret;
}

void LineLayoutCache::Dispose(LineLayout *ll) {
	allInvalidated = false;
	if (ll) {
		if (!ll->inCache) {
			delete ll;
		} else {
			useCount--;
		}
	}
}

Editor::Editor() {
	ctrlID = 0;

	stylesValid = false;

	printMagnification = 0;
	printColourMode = SC_PRINT_NORMAL;
	printWrapState = eWrapWord;
	cursorMode = SC_CURSORNORMAL;
	controlCharSymbol = 0;	/* Draw the control characters */

	hasFocus = false;
	hideSelection = false;
	inOverstrike = false;
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
	inDragDrop = false;
	dropWentOutside = false;
	posDrag = invalidPosition;
	posDrop = invalidPosition;
	selectionType = selChar;

	lastXChosen = 0;
	lineAnchor = 0;
	originalAnchorPos = 0;

	selType = selStream;
	moveExtendsSelection = false;
	xStartSelect = 0;
	xEndSelect = 0;
	primarySelection = true;

	caretXPolicy = CARET_SLOP | CARET_EVEN;
	caretXSlop = 50;

	caretYPolicy = CARET_EVEN;
	caretYSlop = 0;

	searchAnchor = 0;

	xOffset = 0;
	xCaretMargin = 50;
	horizontalScrollBarVisible = true;
	scrollWidth = 2000;
	verticalScrollBarVisible = true;
	endAtLastLine = true;
	caretSticky = false;

	pixmapLine = Surface::Allocate();
	pixmapSelMargin = Surface::Allocate();
	pixmapSelPattern = Surface::Allocate();
	pixmapIndentGuide = Surface::Allocate();
	pixmapIndentGuideHighlight = Surface::Allocate();

	currentPos = 0;
	anchor = 0;

	targetStart = 0;
	targetEnd = 0;
	searchFlags = 0;

	topLine = 0;
	posTopLine = 0;

	lengthForEncode = -1;

	needUpdateUI = true;
	braces[0] = invalidPosition;
	braces[1] = invalidPosition;
	bracesMatchStyle = STYLE_BRACEBAD;
	highlightGuideColumn = 0;

	theEdge = 0;

	paintState = notPainting;

	modEventMask = SC_MODEVENTMASKALL;

	pdoc = new Document();
	pdoc->AddRef();
	pdoc->AddWatcher(this, 0);

	recordingMacro = false;
	foldFlags = 0;

	wrapState = eWrapNone;
	wrapWidth = LineLayout::wrapWidthInfinite;
	wrapStart = wrapLineLarge;
	wrapEnd = wrapLineLarge;
	wrapVisualFlags = 0;
	wrapVisualFlagsLocation = 0;
	wrapVisualStartIndent = 0;
	actualWrapVisualStartIndent = 0;

	convertPastes = true;

	hsStart = -1;
	hsEnd = -1;

	llc.SetLevel(LineLayoutCache::llcCaret);
}

Editor::~Editor() {
	pdoc->RemoveWatcher(this, 0);
	pdoc->Release();
	pdoc = 0;
	DropGraphics();
	delete pixmapLine;
	delete pixmapSelMargin;
	delete pixmapSelPattern;
	delete pixmapIndentGuide;
	delete pixmapIndentGuideHighlight;
}

void Editor::Finalise() {
	SetIdle(false);
	CancelModes();
}

void Editor::DropGraphics() {
	pixmapLine->Release();
	pixmapSelMargin->Release();
	pixmapSelPattern->Release();
	pixmapIndentGuide->Release();
	pixmapIndentGuideHighlight->Release();
}

void Editor::InvalidateStyleData() {
	stylesValid = false;
	palette.Release();
	DropGraphics();
	llc.Invalidate(LineLayout::llInvalid);
	if (selType == selRectangle) {
		xStartSelect = XFromPosition(anchor);
		xEndSelect = XFromPosition(currentPos);
	}
}

void Editor::InvalidateStyleRedraw() {
	NeedWrapping();
	InvalidateStyleData();
	Redraw();
}

void Editor::RefreshColourPalette(Palette &pal, bool want) {
	vs.RefreshColourPalette(pal, want);
}

void Editor::RefreshStyleData() {
	if (!stylesValid) {
		stylesValid = true;
		AutoSurface surface(this);
		if (surface) {
			vs.Refresh(*surface);
			RefreshColourPalette(palette, true);
			palette.Allocate(wMain);
			RefreshColourPalette(palette, false);
		}
		SetScrollBars();
	}
}

PRectangle Editor::GetClientRectangle() {
	return wMain.GetClientPosition();
}

PRectangle Editor::GetTextRectangle() {
	PRectangle rc = GetClientRectangle();
	rc.left += vs.fixedColumnWidth;
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

static inline bool IsControlCharacter(int ch) {
	// iscntrl returns true for lots of chars > 127 which are displayable
	return ch >= 0 && ch < ' ';
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
	AutoLineLayout &operator=(const AutoLineLayout &) { return * this; }
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

/**
 * Allows to iterate through the lines of a selection.
 * Althought it can be called for a stream selection, in most cases
 * it is inefficient and it should be used only for
 * a rectangular or a line selection.
 */
class SelectionLineIterator {
private:
	Editor *ed;
	int line;	///< Current line within the iteration.
	bool forward;	///< True if iterating by increasing line number, false otherwise.
	int selStart, selEnd;	///< Positions of the start and end of the selection relative to the start of the document.
	int minX, maxX;	///< Left and right of selection rectangle.

public:
	int lineStart, lineEnd;	///< Line numbers, first and last lines of the selection.
	int startPos, endPos;	///< Positions of the beginning and end of the selection on the current line.

	void Reset() {
		if (forward) {
			line = lineStart;
		} else {
			line = lineEnd;
		}
	}

	SelectionLineIterator(Editor *ed_, bool forward_ = true) : line(0), startPos(0), endPos(0) {
		ed = ed_;
		forward = forward_;
		selStart = ed->SelectionStart();
		selEnd = ed->SelectionEnd();
		lineStart = ed->pdoc->LineFromPosition(selStart);
		lineEnd = ed->pdoc->LineFromPosition(selEnd);
		// Left of rectangle
		minX = Platform::Minimum(ed->xStartSelect, ed->xEndSelect);
		// Right of rectangle
		maxX = Platform::Maximum(ed->xStartSelect, ed->xEndSelect);
		Reset();
	}
	~SelectionLineIterator() {}

	void SetAt(int line) {
		if (line < lineStart || line > lineEnd) {
			startPos = endPos = INVALID_POSITION;
		} else {
			if (ed->selType == ed->selRectangle) {
				// Measure line and return character closest to minX
				startPos = ed->PositionFromLineX(line, minX);
				// Measure line and return character closest to maxX
				endPos = ed->PositionFromLineX(line, maxX);
			} else if (ed->selType == ed->selLines) {
				startPos = ed->pdoc->LineStart(line);
				endPos = ed->pdoc->LineStart(line + 1);
			} else {	// Stream selection, here only for completion
				if (line == lineStart) {
					startPos = selStart;
				} else {
					startPos = ed->pdoc->LineStart(line);
				}
				if (line == lineEnd) {
					endPos = selEnd;
				} else {
					endPos = ed->pdoc->LineStart(line + 1);
				}
			}
		}
	}
	bool Iterate() {
		SetAt(line);
		if (forward) {
			line++;
		} else {
			line--;
		}
		return startPos != INVALID_POSITION;
	}
};

Point Editor::LocationFromPosition(int pos) {
	Point pt;
	RefreshStyleData();
	if (pos == INVALID_POSITION)
		return pt;
	int line = pdoc->LineFromPosition(pos);
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
		int posInLine = pos - posLineStart;
		// In case of very long line put x at arbitrary large position
		if (posInLine > ll->maxLineLength) {
			pt.x = ll->positions[ll->maxLineLength] - ll->positions[ll->LineStart(ll->lines)];
		}

		for (int subLine = 0; subLine < ll->lines; subLine++) {
			if ((posInLine >= ll->LineStart(subLine)) && (posInLine <= ll->LineStart(subLine + 1))) {
				pt.x = ll->positions[posInLine] - ll->positions[ll->LineStart(subLine)];
				if (actualWrapVisualStartIndent != 0) {
					int lineStart = ll->LineStart(subLine);
					if (lineStart != 0)	// Wrapped
						pt.x += actualWrapVisualStartIndent * vs.aveCharWidth;
				}
			}
			if (posInLine >= ll->LineStart(subLine)) {
				pt.y += vs.lineHeight;
			}
		}
		pt.x += vs.fixedColumnWidth - xOffset;
	}
	return pt;
}

int Editor::XFromPosition(int pos) {
	Point pt = LocationFromPosition(pos);
	return pt.x - vs.fixedColumnWidth + xOffset;
}

int Editor::LineFromLocation(Point pt) {
	return cs.DocFromDisplay(pt.y / vs.lineHeight + topLine);
}

void Editor::SetTopLine(int topLineNew) {
	topLine = topLineNew;
	posTopLine = pdoc->LineStart(cs.DocFromDisplay(topLine));
}

static inline bool IsEOLChar(char ch) {
	return (ch == '\r') || (ch == '\n');
}

int Editor::PositionFromLocation(Point pt) {
	RefreshStyleData();
	pt.x = pt.x - vs.fixedColumnWidth + xOffset;
	int visibleLine = pt.y / vs.lineHeight + topLine;
	if (pt.y < 0) {	// Division rounds towards 0
		visibleLine = (pt.y - (vs.lineHeight - 1)) / vs.lineHeight + topLine;
	}
	if (visibleLine < 0)
		visibleLine = 0;
	int lineDoc = cs.DocFromDisplay(visibleLine);
	if (lineDoc >= pdoc->LinesTotal())
		return pdoc->Length();
	unsigned int posLineStart = pdoc->LineStart(lineDoc);
	int retVal = posLineStart;
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc));
	if (surface && ll) {
		LayoutLine(lineDoc, surface, vs, ll, wrapWidth);
		int lineStartSet = cs.DisplayFromDoc(lineDoc);
		int subLine = visibleLine - lineStartSet;
		if (subLine < ll->lines) {
			int lineStart = ll->LineStart(subLine);
			int lineEnd = ll->LineStart(subLine + 1);
			int subLineStart = ll->positions[lineStart];

			if (actualWrapVisualStartIndent != 0) {
				if (lineStart != 0)	// Wrapped
					pt.x -= actualWrapVisualStartIndent * vs.aveCharWidth;
			}
			for (int i = lineStart; i < lineEnd; i++) {
				if (pt.x < (((ll->positions[i] + ll->positions[i + 1]) / 2) - subLineStart) ||
				        IsEOLChar(ll->chars[i])) {
					return pdoc->MovePositionOutsideChar(i + posLineStart, 1);
				}
			}
			return lineEnd + posLineStart;
		}
		retVal = ll->numCharsInLine + posLineStart;
	}
	return retVal;
}

// Like PositionFromLocation but INVALID_POSITION returned when not near any text.
int Editor::PositionFromLocationClose(Point pt) {
	RefreshStyleData();
	PRectangle rcClient = GetTextRectangle();
	if (!rcClient.Contains(pt))
		return INVALID_POSITION;
	if (pt.x < vs.fixedColumnWidth)
		return INVALID_POSITION;
	if (pt.y < 0)
		return INVALID_POSITION;
	pt.x = pt.x - vs.fixedColumnWidth + xOffset;
	int visibleLine = pt.y / vs.lineHeight + topLine;
	if (pt.y < 0) {	// Division rounds towards 0
		visibleLine = (pt.y - (vs.lineHeight - 1)) / vs.lineHeight + topLine;
	}
	int lineDoc = cs.DocFromDisplay(visibleLine);
	if (lineDoc < 0)
		return INVALID_POSITION;
	if (lineDoc >= pdoc->LinesTotal())
		return INVALID_POSITION;
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc));
	if (surface && ll) {
		LayoutLine(lineDoc, surface, vs, ll, wrapWidth);
		unsigned int posLineStart = pdoc->LineStart(lineDoc);
		int lineStartSet = cs.DisplayFromDoc(lineDoc);
		int subLine = visibleLine - lineStartSet;
		if (subLine < ll->lines) {
			int lineStart = ll->LineStart(subLine);
			int lineEnd = ll->LineStart(subLine + 1);
			int subLineStart = ll->positions[lineStart];

			if (actualWrapVisualStartIndent != 0) {
				if (lineStart != 0)	// Wrapped
					pt.x -= actualWrapVisualStartIndent * vs.aveCharWidth;
			}
			for (int i = lineStart; i < lineEnd; i++) {
				if (pt.x < (((ll->positions[i] + ll->positions[i + 1]) / 2) - subLineStart) ||
				        IsEOLChar(ll->chars[i])) {
					return pdoc->MovePositionOutsideChar(i + posLineStart, 1);
				}
			}
			if (pt.x < (ll->positions[lineEnd] - subLineStart)) {
				return pdoc->MovePositionOutsideChar(lineEnd + posLineStart, 1);
			}
		}
	}

	return INVALID_POSITION;
}

/**
 * Find the document position corresponding to an x coordinate on a particular document line.
 * Ensure is between whole characters when document is in multi-byte or UTF-8 mode.
 */
int Editor::PositionFromLineX(int lineDoc, int x) {
	RefreshStyleData();
	if (lineDoc >= pdoc->LinesTotal())
		return pdoc->Length();
	//Platform::DebugPrintf("Position of (%d,%d) line = %d top=%d\n", pt.x, pt.y, line, topLine);
	AutoSurface surface(this);
	AutoLineLayout ll(llc, RetrieveLineLayout(lineDoc));
	int retVal = 0;
	if (surface && ll) {
		unsigned int posLineStart = pdoc->LineStart(lineDoc);
		LayoutLine(lineDoc, surface, vs, ll, wrapWidth);
		retVal = ll->numCharsInLine + posLineStart;
		int subLine = 0;
		int lineStart = ll->LineStart(subLine);
		int lineEnd = ll->LineStart(subLine + 1);
		int subLineStart = ll->positions[lineStart];

		if (actualWrapVisualStartIndent != 0) {
			if (lineStart != 0)	// Wrapped
				x -= actualWrapVisualStartIndent * vs.aveCharWidth;
		}
		for (int i = lineStart; i < lineEnd; i++) {
			if (x < (((ll->positions[i] + ll->positions[i + 1]) / 2) - subLineStart) ||
			        IsEOLChar(ll->chars[i])) {
				retVal = pdoc->MovePositionOutsideChar(i + posLineStart, 1);
				break;
			}
		}
	}
	return retVal;
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
	//wMain.InvalidateAll();
}

void Editor::RedrawSelMargin(int line) {
	if (!AbandonPaint()) {
		if (vs.maskInLine) {
			Redraw();
		} else {
			PRectangle rcSelMargin = GetClientRectangle();
			rcSelMargin.right = vs.fixedColumnWidth;
			if (line != -1) {
				int position = pdoc->LineStart(line);
				PRectangle rcLine = RectangleFromRange(position, position);
				rcSelMargin.top = rcLine.top;
				rcSelMargin.bottom = rcLine.bottom;
			}
			wMain.InvalidateRectangle(rcSelMargin);
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
	rc.left = vs.fixedColumnWidth;
	rc.top = (minLine - topLine) * vs.lineHeight;
	if (rc.top < 0)
		rc.top = 0;
	rc.right = rcClient.right;
	rc.bottom = (maxLine - topLine + 1) * vs.lineHeight;
	// Ensure PRectangle is within 16 bit space
	rc.top = Platform::Clamp(rc.top, -32000, 32000);
	rc.bottom = Platform::Clamp(rc.bottom, -32000, 32000);

	return rc;
}

void Editor::InvalidateRange(int start, int end) {
	RedrawRect(RectangleFromRange(start, end));
}

int Editor::CurrentPosition() {
	return currentPos;
}

bool Editor::SelectionEmpty() {
	return anchor == currentPos;
}

int Editor::SelectionStart() {
	return Platform::Minimum(currentPos, anchor);
}

int Editor::SelectionEnd() {
	return Platform::Maximum(currentPos, anchor);
}

void Editor::SetRectangularRange() {
	if (selType == selRectangle) {
		xStartSelect = XFromPosition(anchor);
		xEndSelect = XFromPosition(currentPos);
	}
}

void Editor::InvalidateSelection(int currentPos_, int anchor_) {
	int firstAffected = anchor;
	if (firstAffected > currentPos)
		firstAffected = currentPos;
	if (firstAffected > anchor_)
		firstAffected = anchor_;
	if (firstAffected > currentPos_)
		firstAffected = currentPos_;
	int lastAffected = anchor;
	if (lastAffected < currentPos)
		lastAffected = currentPos;
	if (lastAffected < anchor_)
		lastAffected = anchor_;
	if (lastAffected < (currentPos_ + 1))	// +1 ensures caret repainted
		lastAffected = (currentPos_ + 1);
	needUpdateUI = true;
	InvalidateRange(firstAffected, lastAffected);
}

void Editor::SetSelection(int currentPos_, int anchor_) {
	currentPos_ = pdoc->ClampPositionIntoDocument(currentPos_);
	anchor_ = pdoc->ClampPositionIntoDocument(anchor_);
	if ((currentPos != currentPos_) || (anchor != anchor_)) {
		InvalidateSelection(currentPos_, anchor_);
		currentPos = currentPos_;
		anchor = anchor_;
	}
	SetRectangularRange();
	ClaimSelection();
}

void Editor::SetSelection(int currentPos_) {
	currentPos_ = pdoc->ClampPositionIntoDocument(currentPos_);
	if (currentPos != currentPos_) {
		InvalidateSelection(currentPos_, currentPos_);
		currentPos = currentPos_;
	}
	SetRectangularRange();
	ClaimSelection();
}

void Editor::SetEmptySelection(int currentPos_) {
	selType = selStream;
	moveExtendsSelection = false;
	SetSelection(currentPos_, currentPos_);
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
	// DONE, but untested...: make support rectangular selection
	bool scp = false;
	if (selType == selStream) {
		scp = RangeContainsProtected(anchor, currentPos);
	} else {
		SelectionLineIterator lineIterator(this);
		while (lineIterator.Iterate()) {
			if (RangeContainsProtected(lineIterator.startPos, lineIterator.endPos)) {
				scp = true;
				break;
			}
		}
	}
	return scp;
}

/**
 * Asks document to find a good position and then moves out of any invisible positions.
 */
int Editor::MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd) {
	pos = pdoc->MovePositionOutsideChar(pos, moveDir, checkLineEnd);
	if (vs.ProtectionActive()) {
		int mask = pdoc->stylingBitsMask;
		if (moveDir > 0) {
			if ((pos > 0) && vs.styles[pdoc->StyleAt(pos - 1) & mask].IsProtected()) {
				while ((pos < pdoc->Length()) &&
				        (vs.styles[pdoc->StyleAt(pos) & mask].IsProtected()))
					pos++;
			}
		} else if (moveDir < 0) {
			if (vs.styles[pdoc->StyleAt(pos) & mask].IsProtected()) {
				while ((pos > 0) &&
				        (vs.styles[pdoc->StyleAt(pos - 1) & mask].IsProtected()))
					pos--;
			}
		}
	}
	return pos;
}

int Editor::MovePositionTo(int newPos, selTypes sel, bool ensureVisible) {
	int delta = newPos - currentPos;
	newPos = pdoc->ClampPositionIntoDocument(newPos);
	newPos = MovePositionOutsideChar(newPos, delta);
	if (sel != noSel) {
		selType = sel;
	}
	if (sel != noSel || moveExtendsSelection) {
		SetSelection(newPos);
	} else {
		SetEmptySelection(newPos);
	}
	ShowCaretAtCurrentPosition();
	if (ensureVisible) {
		EnsureCaretVisible();
	}
	NotifyMove(newPos);
	return 0;
}

int Editor::MovePositionSoVisible(int pos, int moveDir) {
	pos = pdoc->ClampPositionIntoDocument(pos);
	pos = MovePositionOutsideChar(pos, moveDir);
	int lineDoc = pdoc->LineFromPosition(pos);
	if (cs.GetVisible(lineDoc)) {
		return pos;
	} else {
		int lineDisplay = cs.DisplayFromDoc(lineDoc);
		if (moveDir > 0) {
			// lineDisplay is already line before fold as lines in fold use display line of line after fold
			lineDisplay = Platform::Clamp(lineDisplay, 0, cs.LinesDisplayed());
			return pdoc->LineStart(cs.DocFromDisplay(lineDisplay));
		} else {
			lineDisplay = Platform::Clamp(lineDisplay - 1, 0, cs.LinesDisplayed());
			return pdoc->LineEnd(cs.DocFromDisplay(lineDisplay));
		}
	}
}

/**
 * Choose the x position that the caret will try to stick to
 * as it moves up and down.
 */
void Editor::SetLastXChosen() {
	Point pt = LocationFromPosition(currentPos);
	lastXChosen = pt.x;
}

void Editor::ScrollTo(int line, bool moveThumb) {
	int topLineNew = Platform::Clamp(line, 0, MaxScrollPos());
	if (topLineNew != topLine) {
		// Try to optimise small scrolls
		int linesToMove = topLine - topLineNew;
		SetTopLine(topLineNew);
		ShowCaretAtCurrentPosition();
		// Perform redraw rather than scroll if many lines would be redrawn anyway.
#ifndef UNDER_CE
		if ((abs(linesToMove) <= 10) && (paintState == notPainting)) {
			ScrollText(linesToMove);
		} else {
			Redraw();
		}
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
		SetHorizontalScrollPos();
		RedrawRect(GetClientRectangle());
	}
}

void Editor::MoveCaretInsideView(bool ensureVisible) {
	PRectangle rcClient = GetTextRectangle();
	Point pt = LocationFromPosition(currentPos);
	if (pt.y < rcClient.top) {
		MovePositionTo(PositionFromLocation(
		                   Point(lastXChosen, rcClient.top)),
		               noSel, ensureVisible);
	} else if ((pt.y + vs.lineHeight - 1) > rcClient.bottom) {
		int yOfLastLineFullyDisplayed = rcClient.top + (LinesOnScreen() - 1) * vs.lineHeight;
		MovePositionTo(PositionFromLocation(
		                   Point(lastXChosen, rcClient.top + yOfLastLineFullyDisplayed)),
		               noSel, ensureVisible);
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
slop | strict | jumps | even | Caret can go to the margin                 | When reaching limitÝ(caret going out of
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
void Editor::EnsureCaretVisible(bool useMargin, bool vert, bool horiz) {
	//Platform::DebugPrintf("EnsureCaretVisible %d %s\n", xOffset, useMargin ? " margin" : " ");
	PRectangle rcClient = GetTextRectangle();
	//int rcClientFullWidth = rcClient.Width();
	int posCaret = currentPos;
	if (posDrag >= 0) {
		posCaret = posDrag;
	}
	Point pt = LocationFromPosition(posCaret);
	Point ptBottomCaret = pt;
	ptBottomCaret.y += vs.lineHeight - 1;
	int lineCaret = DisplayFromPosition(posCaret);
	bool bSlop, bStrict, bJump, bEven;

	// Vertical positioning
	if (vert && (pt.y < rcClient.top || ptBottomCaret.y > rcClient.bottom || (caretYPolicy & CARET_STRICT) != 0)) {
		int linesOnScreen = LinesOnScreen();
		int halfScreen = Platform::Maximum(linesOnScreen - 1, 2) / 2;
		int newTopLine = topLine;
		bSlop = (caretYPolicy & CARET_SLOP) != 0;
		bStrict = (caretYPolicy & CARET_STRICT) != 0;
		bJump = (caretYPolicy & CARET_JUMPS) != 0;
		bEven = (caretYPolicy & CARET_EVEN) != 0;

		// It should be possible to scroll the window to show the caret,
		// but this fails to remove the caret on GTK+
		if (bSlop) {	// A margin is defined
			int yMoveT, yMoveB;
			if (bStrict) {
				int yMarginT, yMarginB;
				if (!useMargin) {
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
					newTopLine = lineCaret - yMoveT;
				} else if (lineCaret > topLine + linesOnScreen - 1 - yMarginB) {
					// Caret goes too low
					newTopLine = lineCaret - linesOnScreen + 1 + yMoveB;
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
					newTopLine = lineCaret - yMoveT;
				} else if (lineCaret > topLine + linesOnScreen - 1) {
					// Caret goes too low
					newTopLine = lineCaret - linesOnScreen + 1 + yMoveB;
				}
			}
		} else {	// No slop
			if (!bStrict && !bJump) {
				// Minimal move
				if (lineCaret < topLine) {
					// Caret goes too high
					newTopLine = lineCaret;
				} else if (lineCaret > topLine + linesOnScreen - 1) {
					// Caret goes too low
					if (bEven) {
						newTopLine = lineCaret - linesOnScreen + 1;
					} else {
						newTopLine = lineCaret;
					}
				}
			} else {	// Strict or going out of display
				if (bEven) {
					// Always center caret
					newTopLine = lineCaret - halfScreen;
				} else {
					// Always put caret on top of display
					newTopLine = lineCaret;
				}
			}
		}
		newTopLine = Platform::Clamp(newTopLine, 0, MaxScrollPos());
		if (newTopLine != topLine) {
			Redraw();
			SetTopLine(newTopLine);
			SetVerticalScrollPos();
		}
	}

	// Horizontal positioning
	if (horiz && (wrapState == eWrapNone)) {
		int halfScreen = Platform::Maximum(rcClient.Width() - 4, 4) / 2;
		int xOffsetNew = xOffset;
		bSlop = (caretXPolicy & CARET_SLOP) != 0;
		bStrict = (caretXPolicy & CARET_STRICT) != 0;
		bJump = (caretXPolicy & CARET_JUMPS) != 0;
		bEven = (caretXPolicy & CARET_EVEN) != 0;

		if (bSlop) {	// A margin is defined
			int xMoveL, xMoveR;
			if (bStrict) {
				int xMarginL, xMarginR;
				if (!useMargin) {
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
						xOffsetNew -= xMoveL;
					} else {
						// Move just enough to allow to display the caret
						xOffsetNew -= (rcClient.left + xMarginL) - pt.x;
					}
				} else if (pt.x >= rcClient.right - xMarginR) {
					// Caret is on the right of the display
					if (bJump && bEven) {
						xOffsetNew += xMoveR;
					} else {
						// Move just enough to allow to display the caret
						xOffsetNew += pt.x - (rcClient.right - xMarginR) + 1;
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
					xOffsetNew -= xMoveL;
				} else if (pt.x >= rcClient.right) {
					// Caret is on the right of the display
					xOffsetNew += xMoveR;
				}
			}
		} else {	// No slop
			if (bStrict ||
			        (bJump && (pt.x < rcClient.left || pt.x >= rcClient.right))) {
				// Strict or going out of display
				if (bEven) {
					// Center caret
					xOffsetNew += pt.x - rcClient.left - halfScreen;
				} else {
					// Put caret on right
					xOffsetNew += pt.x - rcClient.right + 1;
				}
			} else {
				// Move just enough to allow to display the caret
				if (pt.x < rcClient.left) {
					// Caret is on the left of the display
					if (bEven) {
						xOffsetNew -= rcClient.left - pt.x;
					} else {
						xOffsetNew += pt.x - rcClient.right + 1;
					}
				} else if (pt.x >= rcClient.right) {
					// Caret is on the right of the display
					xOffsetNew += pt.x - rcClient.right + 1;
				}
			}
		}
		// In case of a jump (find result) largely out of display, adjust the offset to display the caret
		if (pt.x + xOffset < rcClient.left + xOffsetNew) {
			xOffsetNew = pt.x + xOffset - rcClient.left;
		} else if (pt.x + xOffset >= rcClient.right + xOffsetNew) {
			xOffsetNew = pt.x + xOffset - rcClient.right + 1;
		}
		if (xOffsetNew < 0) {
			xOffsetNew = 0;
		}
		if (xOffset != xOffsetNew) {
			xOffset = xOffsetNew;
			if (xOffsetNew > 0) {
				PRectangle rcText = GetTextRectangle();
				if (horizontalScrollBarVisible == true &&
				        rcText.Width() + xOffset > scrollWidth) {
					scrollWidth = xOffset + rcText.Width();
					SetScrollBars();
				}
			}
			SetHorizontalScrollPos();
			Redraw();
		}
	}
	UpdateSystemCaret();
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
	if (posDrag >= 0)
		InvalidateRange(posDrag, posDrag + 1);
	else
		InvalidateRange(currentPos, currentPos + 1);
	UpdateSystemCaret();
}

void Editor::UpdateSystemCaret() {
}

void Editor::NeedWrapping(int docLineStart, int docLineEnd) {
	docLineStart = Platform::Clamp(docLineStart, 0, pdoc->LinesTotal());
	if (wrapStart > docLineStart) {
		wrapStart = docLineStart;
		llc.Invalidate(LineLayout::llPositions);
	}
	if (wrapEnd < docLineEnd) {
		wrapEnd = docLineEnd;
	}
	wrapEnd = Platform::Clamp(wrapEnd, 0, pdoc->LinesTotal());
	// Wrap lines during idle.
	if ((wrapState != eWrapNone) && (wrapEnd != wrapStart)) {
		SetIdle(true);
	}
}

// Check if wrapping needed and perform any needed wrapping.
// fullwrap: if true, all lines which need wrapping will be done,
//           in this single call.
// priorityWrapLineStart: If greater than zero, all lines starting from
//           here to 1 page + 100 lines past will be wrapped (even if there are
//           more lines under wrapping process in idle).
// If it is neither fullwrap, nor priorityWrap, then 1 page + 100 lines will be
// wrapped, if there are any wrapping going on in idle. (Generally this
// condition is called only from idler).
// Return true if wrapping occurred.
bool Editor::WrapLines(bool fullWrap, int priorityWrapLineStart) {
	// If there are any pending wraps, do them during idle if possible.
	int linesInOneCall = LinesOnScreen() + 100;
	if (wrapState != eWrapNone) {
		if (wrapStart < wrapEnd) {
			if (!SetIdle(true)) {
				// Idle processing not supported so full wrap required.
				fullWrap = true;
			}
		}
		if (!fullWrap && priorityWrapLineStart >= 0 &&
			// .. and if the paint window is outside pending wraps
			(((priorityWrapLineStart + linesInOneCall) < wrapStart) ||
			 (priorityWrapLineStart > wrapEnd))) {
			// No priority wrap pending
			return false;
		}
	}
	int goodTopLine = topLine;
	bool wrapOccurred = false;
	if (wrapStart <= pdoc->LinesTotal()) {
		if (wrapState == eWrapNone) {
			if (wrapWidth != LineLayout::wrapWidthInfinite) {
				wrapWidth = LineLayout::wrapWidthInfinite;
				for (int lineDoc = 0; lineDoc < pdoc->LinesTotal(); lineDoc++) {
					cs.SetHeight(lineDoc, 1);
				}
				wrapOccurred = true;
			}
			wrapStart = wrapLineLarge;
			wrapEnd = wrapLineLarge;
		} else {
			if (wrapEnd >= pdoc->LinesTotal())
				wrapEnd = pdoc->LinesTotal();
			//ElapsedTime et;
			int lineDocTop = cs.DocFromDisplay(topLine);
			int subLineTop = topLine - cs.DisplayFromDoc(lineDocTop);
			PRectangle rcTextArea = GetClientRectangle();
			rcTextArea.left = vs.fixedColumnWidth;
			rcTextArea.right -= vs.rightMarginWidth;
			wrapWidth = rcTextArea.Width();
			// Ensure all of the document is styled.
			pdoc->EnsureStyledTo(pdoc->Length());
			RefreshStyleData();
			AutoSurface surface(this);
			if (surface) {
				bool priorityWrap = false;
				int lastLineToWrap = wrapEnd;
				int lineToWrap = wrapStart;
				if (!fullWrap) {
					if (priorityWrapLineStart >= 0) {
						// This is a priority wrap.
						lineToWrap = priorityWrapLineStart;
						lastLineToWrap = priorityWrapLineStart + linesInOneCall;
						priorityWrap = true;
					} else {
						// This is idle wrap.
						lastLineToWrap = wrapStart + linesInOneCall;
					}
					if (lastLineToWrap >= wrapEnd)
						lastLineToWrap = wrapEnd;
				} // else do a fullWrap.

				// Platform::DebugPrintf("Wraplines: full = %d, priorityStart = %d (wrapping: %d to %d)\n", fullWrap, priorityWrapLineStart, lineToWrap, lastLineToWrap);
				// Platform::DebugPrintf("Pending wraps: %d to %d\n", wrapStart, wrapEnd);
				while (lineToWrap < lastLineToWrap) {
					AutoLineLayout ll(llc, RetrieveLineLayout(lineToWrap));
					int linesWrapped = 1;
					if (ll) {
						LayoutLine(lineToWrap, surface, vs, ll, wrapWidth);
						linesWrapped = ll->lines;
					}
					if (cs.SetHeight(lineToWrap, linesWrapped)) {
						wrapOccurred = true;
					}
					lineToWrap++;
				}
				if (!priorityWrap)
					wrapStart = lineToWrap;
				// If wrapping is done, bring it to resting position
				if (wrapStart >= wrapEnd) {
					wrapStart = wrapLineLarge;
					wrapEnd = wrapLineLarge;
				}
			}
			goodTopLine = cs.DisplayFromDoc(lineDocTop);
			if (subLineTop < cs.GetHeight(lineDocTop))
				goodTopLine += subLineTop;
			else
				goodTopLine += cs.GetHeight(lineDocTop);
			//double durWrap = et.Duration(true);
			//Platform::DebugPrintf("Wrap:%9.6g \n", durWrap);
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
		pdoc->BeginUndoAction();
		bool prevNonWS = true;
		for (int pos = targetStart; pos < targetEnd; pos++) {
			if (IsEOLChar(pdoc->CharAt(pos))) {
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
		pdoc->EndUndoAction();
	}
}

const char *StringFromEOLMode(int eolMode) {
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
		pdoc->BeginUndoAction();
		for (int line = lineStart; line <= lineEnd; line++) {
			AutoSurface surface(this);
			AutoLineLayout ll(llc, RetrieveLineLayout(line));
			if (surface && ll) {
				unsigned int posLineStart = pdoc->LineStart(line);
				LayoutLine(line, surface, vs, ll, pixelWidth);
				for (int subLine = 1; subLine < ll->lines; subLine++) {
					pdoc->InsertCString(posLineStart + (subLine - 1) * strlen(eol) +
						ll->LineStart(subLine), eol);
					targetEnd += static_cast<int>(strlen(eol));
				}
			}
			lineEnd = pdoc->LineFromPosition(targetEnd);
		}
		pdoc->EndUndoAction();
	}
}

int Editor::SubstituteMarkerIfEmpty(int markerCheck, int markerDefault) {
	if (vs.markers[markerCheck].markType == SC_MARK_EMPTY)
		return markerDefault;
	return markerCheck;
}

// Avoid 64 bit compiler warnings.
// Scintilla does not support text buffers larger than 2**31
static int istrlen(const char *s) {
	return static_cast<int>(strlen(s));
}

void Editor::PaintSelMargin(Surface *surfWindow, PRectangle &rc) {
	if (vs.fixedColumnWidth == 0)
		return;

	PRectangle rcMargin = GetClientRectangle();
	rcMargin.right = vs.fixedColumnWidth;

	if (!rc.Intersects(rcMargin))
		return;

	Surface *surface;
	if (bufferedDraw) {
		surface = pixmapSelMargin;
	} else {
		surface = surfWindow;
	}

	PRectangle rcSelMargin = rcMargin;
	rcSelMargin.right = rcMargin.left;

	for (int margin = 0; margin < vs.margins; margin++) {
		if (vs.ms[margin].width > 0) {

			rcSelMargin.left = rcSelMargin.right;
			rcSelMargin.right = rcSelMargin.left + vs.ms[margin].width;

			if (vs.ms[margin].style != SC_MARGIN_NUMBER) {
				/* alternate scheme:
				if (vs.ms[margin].mask & SC_MASK_FOLDERS)
					surface->FillRectangle(rcSelMargin, vs.styles[STYLE_DEFAULT].back.allocated);
				else
					// Required because of special way brush is created for selection margin
					surface->FillRectangle(rcSelMargin, pixmapSelPattern);
				*/
				if (vs.ms[margin].mask & SC_MASK_FOLDERS)
					// Required because of special way brush is created for selection margin
					surface->FillRectangle(rcSelMargin, *pixmapSelPattern);
				else {
					ColourAllocated colour;
					switch (vs.ms[margin].style) {
					case SC_MARGIN_BACK:
						colour = vs.styles[STYLE_DEFAULT].back.allocated;
						break;
					case SC_MARGIN_FORE:
						colour = vs.styles[STYLE_DEFAULT].fore.allocated;
						break;
					default:
						colour = vs.styles[STYLE_LINENUMBER].back.allocated;
						break;
					}
					surface->FillRectangle(rcSelMargin, colour);
				}
			} else {
				surface->FillRectangle(rcSelMargin, vs.styles[STYLE_LINENUMBER].back.allocated);
			}

			int visibleLine = topLine;
			int yposScreen = 0;

			// Work out whether the top line is whitespace located after a
			// lessening of fold level which implies a 'fold tail' but which should not
			// be displayed until the last of a sequence of whitespace.
			bool needWhiteClosure = false;
			int level = pdoc->GetLevel(cs.DocFromDisplay(topLine));
			if (level & SC_FOLDLEVELWHITEFLAG) {
				int lineBack = cs.DocFromDisplay(topLine);
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

			// Old code does not know about new markers needed to distinguish all cases
			int folderOpenMid = SubstituteMarkerIfEmpty(SC_MARKNUM_FOLDEROPENMID,
			                    SC_MARKNUM_FOLDEROPEN);
			int folderEnd = SubstituteMarkerIfEmpty(SC_MARKNUM_FOLDEREND,
			                                        SC_MARKNUM_FOLDER);

			while ((visibleLine < cs.LinesDisplayed()) && yposScreen < rcMargin.bottom) {

				PLATFORM_ASSERT(visibleLine < cs.LinesDisplayed());

				int lineDoc = cs.DocFromDisplay(visibleLine);
				PLATFORM_ASSERT(cs.GetVisible(lineDoc));
				bool firstSubLine = visibleLine == cs.DisplayFromDoc(lineDoc);

				// Decide which fold indicator should be displayed
				level = pdoc->GetLevel(lineDoc);
				int levelNext = pdoc->GetLevel(lineDoc + 1);
				int marks = pdoc->GetMark(lineDoc);
				if (!firstSubLine)
					marks = 0;
				int levelNum = level & SC_FOLDLEVELNUMBERMASK;
				int levelNextNum = levelNext & SC_FOLDLEVELNUMBERMASK;
				if (level & SC_FOLDLEVELHEADERFLAG) {
					if (firstSubLine) {
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
					} else {
						marks |= 1 << SC_MARKNUM_FOLDERSUB;
					}
					needWhiteClosure = false;
				} else if (level & SC_FOLDLEVELWHITEFLAG) {
					if (needWhiteClosure) {
						if (levelNext & SC_FOLDLEVELWHITEFLAG) {
							marks |= 1 << SC_MARKNUM_FOLDERSUB;
						} else if (levelNum > SC_FOLDLEVELBASE) {
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
						} else if (levelNextNum > SC_FOLDLEVELBASE) {
							marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
						} else {
							marks |= 1 << SC_MARKNUM_FOLDERTAIL;
						}
					} else {
						marks |= 1 << SC_MARKNUM_FOLDERSUB;
					}
				}

				marks &= vs.ms[margin].mask;
				PRectangle rcMarker = rcSelMargin;
				rcMarker.top = yposScreen;
				rcMarker.bottom = yposScreen + vs.lineHeight;
				if (vs.ms[margin].style == SC_MARGIN_NUMBER) {
					char number[100];
					number[0] = '\0';
					if (firstSubLine)
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
					int width = surface->WidthText(vs.styles[STYLE_LINENUMBER].font, number, istrlen(number));
					int xpos = rcNumber.right - width - 3;
					rcNumber.left = xpos;
					surface->DrawTextNoClip(rcNumber, vs.styles[STYLE_LINENUMBER].font,
					                        rcNumber.top + vs.maxAscent, number, istrlen(number),
					                        vs.styles[STYLE_LINENUMBER].fore.allocated,
					                        vs.styles[STYLE_LINENUMBER].back.allocated);
				}

				if (marks) {
					for (int markBit = 0; (markBit < 32) && marks; markBit++) {
						if (marks & 1) {
							vs.markers[markBit].Draw(surface, rcMarker, vs.styles[STYLE_LINENUMBER].font);
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
	surface->FillRectangle(rcBlankMargin, vs.styles[STYLE_DEFAULT].back.allocated);

	if (bufferedDraw) {
		surfWindow->Copy(rcMargin, Point(), *pixmapSelMargin);
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

static bool IsSpaceOrTab(char ch) {
	return ch == ' ' || ch == '\t';
}

LineLayout *Editor::RetrieveLineLayout(int lineNumber) {
	int posLineStart = pdoc->LineStart(lineNumber);
	int posLineEnd = pdoc->LineStart(lineNumber + 1);
	PLATFORM_ASSERT(posLineEnd >= posLineStart);
	int lineCaret = pdoc->LineFromPosition(currentPos);
	return llc.Retrieve(lineNumber, lineCaret,
	                    posLineEnd - posLineStart, pdoc->GetStyleClock(),
	                    LinesOnScreen() + 1, pdoc->LinesTotal());
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
			int cid = posLineEnd - 1;
			while ((cid > posLineStart) && IsEOLChar(pdoc->CharAt(cid))) {
				cid--;
				lineLength--;
			}
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
		int numCharsInLine = 0;
		if (vstyle.edgeState == EDGE_BACKGROUND) {
			ll->edgeColumn = pdoc->FindColumn(line, theEdge);
			if (ll->edgeColumn >= posLineStart) {
				ll->edgeColumn -= posLineStart;
			}
		} else {
			ll->edgeColumn = -1;
		}

		char styleByte = 0;
		int styleMask = pdoc->stylingBitsMask;
		ll->styleBitsSet = 0;
		// Fill base line layout
		for (int charInDoc = posLineStart; charInDoc < posLineEnd; charInDoc++) {
			char chDoc = pdoc->CharAt(charInDoc);
			styleByte = pdoc->StyleAt(charInDoc);
			ll->styleBitsSet |= styleByte;
			if (vstyle.viewEOL || (!IsEOLChar(chDoc))) {
				ll->chars[numCharsInLine] = chDoc;
				ll->styles[numCharsInLine] = static_cast<char>(styleByte & styleMask);
				ll->indicators[numCharsInLine] = static_cast<char>(styleByte & ~styleMask);
				if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseUpper)
					ll->chars[numCharsInLine] = static_cast<char>(toupper(chDoc));
				else if (vstyle.styles[ll->styles[numCharsInLine]].caseForce == Style::caseLower)
					ll->chars[numCharsInLine] = static_cast<char>(tolower(chDoc));
				numCharsInLine++;
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
		int startsegx = 0;	// Start of the current segment, in pixels
		ll->positions[0] = 0;
		unsigned int tabWidth = vstyle.spaceWidth * pdoc->tabInChars;
		bool lastSegItalics = false;
		Font &ctrlCharsFont = vstyle.styles[STYLE_CONTROLCHAR].font;

		int ctrlCharWidth[32] = {0};
		bool isControlNext = IsControlCharacter(ll->chars[0]);
		for (int charInLine = 0; charInLine < numCharsInLine; charInLine++) {
			bool isControl = isControlNext;
			isControlNext = IsControlCharacter(ll->chars[charInLine + 1]);
			if ((ll->styles[charInLine] != ll->styles[charInLine + 1]) ||
			        isControl || isControlNext) {
				ll->positions[startseg] = 0;
				if (vstyle.styles[ll->styles[charInLine]].visible) {
					if (isControl) {
						if (ll->chars[charInLine] == '\t') {
							ll->positions[charInLine + 1] = ((((startsegx + 2) /
							                                   tabWidth) + 1) * tabWidth) - startsegx;
						} else if (controlCharSymbol < 32) {
							if (ctrlCharWidth[ll->chars[charInLine]] == 0) {
								const char *ctrlChar = ControlCharacterString(ll->chars[charInLine]);
								// +3 For a blank on front and rounded edge each side:
								ctrlCharWidth[ll->chars[charInLine]] =
									surface->WidthText(ctrlCharsFont, ctrlChar, istrlen(ctrlChar)) + 3;
							}
							ll->positions[charInLine + 1] = ctrlCharWidth[ll->chars[charInLine]];
						} else {
							char cc[2] = { static_cast<char>(controlCharSymbol), '\0' };
							surface->MeasureWidths(ctrlCharsFont, cc, 1,
							                       ll->positions + startseg + 1);
						}
						lastSegItalics = false;
					} else {	// Regular character
						int lenSeg = charInLine - startseg + 1;
						if ((lenSeg == 1) && (' ' == ll->chars[startseg])) {
							lastSegItalics = false;
							// Over half the segments are single characters and of these about half are space characters.
							ll->positions[charInLine + 1] = vstyle.styles[ll->styles[charInLine]].spaceWidth;
						} else {
							lastSegItalics = vstyle.styles[ll->styles[charInLine]].italic;
							surface->MeasureWidths(vstyle.styles[ll->styles[charInLine]].font, ll->chars + startseg,
							                       lenSeg, ll->positions + startseg + 1);
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
			ll->positions[startseg] += 2;
		}
		ll->numCharsInLine = numCharsInLine;
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
				width -= vstyle.aveCharWidth; // take into account the space for end wrap mark
			}
			ll->lines = 0;
			// Calculate line start positions based upon width.
			// For now this is simplistic - wraps on byte rather than character and
			// in the middle of words. Should search for spaces or style changes.
			int lastGoodBreak = 0;
			int lastLineStart = 0;
			int startOffset = 0;
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
					startOffset -= actualWrapVisualStartIndent * vstyle.aveCharWidth;
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

ColourAllocated Editor::SelectionBackground(ViewStyle &vsDraw) {
	return primarySelection ? vsDraw.selbackground.allocated : vsDraw.selbackground2.allocated;
}

ColourAllocated Editor::TextBackground(ViewStyle &vsDraw, bool overrideBackground,
                                       ColourAllocated background, bool inSelection, bool inHotspot, int styleMain, int i, LineLayout *ll) {
	if (inSelection) {
		if (vsDraw.selbackset && (vsDraw.selAlpha == SC_ALPHA_NOALPHA)) {
			return SelectionBackground(vsDraw);
		}
	} else {
		if ((vsDraw.edgeState == EDGE_BACKGROUND) &&
		        (i >= ll->edgeColumn) &&
		        !IsEOLChar(ll->chars[i]))
			return vsDraw.edgecolour.allocated;
		if (inHotspot && vsDraw.hotspotBackgroundSet)
			return vsDraw.hotspotBackground.allocated;
		if (overrideBackground)
			return background;
	}
	return vsDraw.styles[styleMain].back.allocated;
}

void Editor::DrawIndentGuide(Surface *surface, int lineVisible, int lineHeight, int start, PRectangle rcSegment, bool highlight) {
	Point from(0, ((lineVisible & 1) && (lineHeight & 1)) ? 1 : 0);
	PRectangle rcCopyArea(start + 1, rcSegment.top, start + 2, rcSegment.bottom);
	surface->Copy(rcCopyArea, from,
	              highlight ? *pixmapIndentGuideHighlight : *pixmapIndentGuide);
}

void Editor::DrawWrapMarker(Surface *surface, PRectangle rcPlace,
                            bool isEndMarker, ColourAllocated wrapColour) {
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

static void SimpleAlphaRectangle(Surface *surface, PRectangle rc, ColourAllocated fill, int alpha) {
	if (alpha != SC_ALPHA_NOALPHA) {
		surface->AlphaRectangle(rc, 0, fill, alpha, fill, alpha, 0);
	}
}

void Editor::DrawEOL(Surface *surface, ViewStyle &vsDraw, PRectangle rcLine, LineLayout *ll,
                     int line, int lineEnd, int xStart, int subLine, int subLineStart,
                     bool overrideBackground, ColourAllocated background,
                     bool drawWrapMarkEnd, ColourAllocated wrapColour) {

	int styleMask = pdoc->stylingBitsMask;
	PRectangle rcSegment = rcLine;

	// Fill in a PRectangle representing the end of line characters
	int xEol = ll->positions[lineEnd] - subLineStart;
	rcSegment.left = xEol + xStart;
	rcSegment.right = xEol + vsDraw.aveCharWidth + xStart;
	int posLineEnd = pdoc->LineStart(line + 1);
	bool eolInSelection = (subLine == (ll->lines - 1)) &&
	                      (posLineEnd > ll->selStart) && (posLineEnd <= ll->selEnd) && (ll->selStart != ll->selEnd);

	if (eolInSelection && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (vsDraw.selAlpha == SC_ALPHA_NOALPHA)) {
		surface->FillRectangle(rcSegment, SelectionBackground(vsDraw));
	} else {
		if (overrideBackground) {
			surface->FillRectangle(rcSegment, background);
		} else {
			surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].back.allocated);
		}
		if (eolInSelection && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (vsDraw.selAlpha != SC_ALPHA_NOALPHA)) {
			SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw), vsDraw.selAlpha);
		}
	}

	rcSegment.left = xEol + vsDraw.aveCharWidth + xStart;
	rcSegment.right = rcLine.right;
	if (overrideBackground) {
		surface->FillRectangle(rcSegment, background);
	} else if (vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].eolFilled) {
		surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].back.allocated);
	} else {
		surface->FillRectangle(rcSegment, vsDraw.styles[STYLE_DEFAULT].back.allocated);
	}

	if (vsDraw.selEOLFilled && eolInSelection && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (vsDraw.selAlpha == SC_ALPHA_NOALPHA)) {
		surface->FillRectangle(rcSegment, SelectionBackground(vsDraw));
	} else {
		if (overrideBackground) {
			surface->FillRectangle(rcSegment, background);
		} else if (vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].eolFilled) {
			surface->FillRectangle(rcSegment, vsDraw.styles[ll->styles[ll->numCharsInLine] & styleMask].back.allocated);
		} else {
			surface->FillRectangle(rcSegment, vsDraw.styles[STYLE_DEFAULT].back.allocated);
		}
		if (vsDraw.selEOLFilled && eolInSelection && vsDraw.selbackset && (line < pdoc->LinesTotal() - 1) && (vsDraw.selAlpha != SC_ALPHA_NOALPHA)) {
			SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw), vsDraw.selAlpha);
		}
 	}

	if (drawWrapMarkEnd) {
		PRectangle rcPlace = rcSegment;

		if (wrapVisualFlagsLocation & SC_WRAPVISUALFLAGLOC_END_BY_TEXT) {
			rcPlace.left = xEol + xStart;
			rcPlace.right = rcPlace.left + vsDraw.aveCharWidth;
		} else {
			// draw left of the right text margin, to avoid clipping by the current clip rect
			rcPlace.right = rcLine.right - vs.rightMarginWidth;
			rcPlace.left = rcPlace.right - vsDraw.aveCharWidth;
		}
		DrawWrapMarker(surface, rcPlace, true, wrapColour);
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
	ColourAllocated background;
	if (caret.active && vsDraw.showCaretLineBackground && (vsDraw.caretLineAlpha == SC_ALPHA_NOALPHA) && ll->containsCaret) {
		overrideBackground = true;
		background = vsDraw.caretLineBackground.allocated;
	}
	if (!overrideBackground) {
		int marks = pdoc->GetMark(line);
		for (int markBit = 0; (markBit < 32) && marks; markBit++) {
			if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_BACKGROUND) &&
				(vsDraw.markers[markBit].alpha == SC_ALPHA_NOALPHA)) {
				background = vsDraw.markers[markBit].back.allocated;
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
						background = vsDraw.markers[markBit].back.allocated;
					}
					marksMasked >>= 1;
				}
			}
		}
	}

	bool drawWhitespaceBackground = (vsDraw.viewWhitespace != wsInvisible) &&
	                                (!overrideBackground) && (vsDraw.whitespaceBackgroundSet);

	bool inIndentation = subLine == 0;	// Do not handle indentation except on first subline.
	int indentWidth = pdoc->IndentSize() * vsDraw.spaceWidth;

	int posLineStart = pdoc->LineStart(line);

	int startseg = ll->LineStart(subLine);
	int subLineStart = ll->positions[startseg];
	int lineStart = 0;
	int lineEnd = 0;
	if (subLine < ll->lines) {
		lineStart = ll->LineStart(subLine);
		lineEnd = ll->LineStart(subLine + 1);
	}

	ColourAllocated wrapColour = vsDraw.styles[STYLE_DEFAULT].fore.allocated;
	if (vsDraw.whitespaceForegroundSet)
		wrapColour = vsDraw.whitespaceForeground.allocated;

	bool drawWrapMarkEnd = false;

	if (wrapVisualFlags & SC_WRAPVISUALFLAG_END) {
		if (subLine + 1 < ll->lines) {
			drawWrapMarkEnd = ll->LineStart(subLine + 1) != 0;
		}
	}

	if (actualWrapVisualStartIndent != 0) {

		bool continuedWrapLine = false;
		if (subLine < ll->lines) {
			continuedWrapLine = ll->LineStart(subLine) != 0;
		}

		if (continuedWrapLine) {
			// draw continuation rect
			PRectangle rcPlace = rcSegment;

			rcPlace.left = ll->positions[startseg] + xStart - subLineStart;
			rcPlace.right = rcPlace.left + actualWrapVisualStartIndent * vsDraw.aveCharWidth;

			// default bgnd here..
			surface->FillRectangle(rcSegment, overrideBackground ? background :
				vsDraw.styles[STYLE_DEFAULT].back.allocated);

			// main line style would be below but this would be inconsistent with end markers
			// also would possibly not be the style at wrap point
			//int styleMain = ll->styles[lineStart];
			//surface->FillRectangle(rcPlace, vsDraw.styles[styleMain].back.allocated);

			if (wrapVisualFlags & SC_WRAPVISUALFLAG_START) {

				if (wrapVisualFlagsLocation & SC_WRAPVISUALFLAGLOC_START_BY_TEXT)
					rcPlace.left = rcPlace.right - vsDraw.aveCharWidth;
				else
					rcPlace.right = rcPlace.left + vsDraw.aveCharWidth;

				DrawWrapMarker(surface, rcPlace, false, wrapColour);
			}

			xStart += actualWrapVisualStartIndent * vsDraw.aveCharWidth;
		}
	}

	int i;

	// Background drawing loop
	for (i = lineStart; twoPhaseDraw && (i < lineEnd); i++) {

		int iDoc = i + posLineStart;
		// If there is the end of a style run for any reason
		if ((ll->styles[i] != ll->styles[i + 1]) ||
		        i == (lineEnd - 1) ||
		        IsControlCharacter(ll->chars[i]) || IsControlCharacter(ll->chars[i + 1]) ||
		        ((ll->selStart != ll->selEnd) && ((iDoc + 1 == ll->selStart) || (iDoc + 1 == ll->selEnd))) ||
		        (i == (ll->edgeColumn - 1))) {
			rcSegment.left = ll->positions[startseg] + xStart - subLineStart;
			rcSegment.right = ll->positions[i + 1] + xStart - subLineStart;
			// Only try to draw if really visible - enhances performance by not calling environment to
			// draw strings that are completely past the right side of the window.
			if ((rcSegment.left <= rcLine.right) && (rcSegment.right >= rcLine.left)) {
				int styleMain = ll->styles[i];
				bool inSelection = (iDoc >= ll->selStart) && (iDoc < ll->selEnd) && (ll->selStart != ll->selEnd);
				bool inHotspot = (ll->hsStart != -1) && (iDoc >= ll->hsStart) && (iDoc < ll->hsEnd);
				ColourAllocated textBack = TextBackground(vsDraw, overrideBackground, background, inSelection, inHotspot, styleMain, i, ll);
				if (ll->chars[i] == '\t') {
					// Tab display
					if (drawWhitespaceBackground &&
					        (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways))
						textBack = vsDraw.whitespaceBackground.allocated;
					surface->FillRectangle(rcSegment, textBack);
				} else if (IsControlCharacter(ll->chars[i])) {
					// Control character display
					inIndentation = false;
					surface->FillRectangle(rcSegment, textBack);
				} else {
					// Normal text display
					surface->FillRectangle(rcSegment, textBack);
					if (vsDraw.viewWhitespace != wsInvisible ||
					        (inIndentation && vsDraw.viewIndentationGuides)) {
						for (int cpos = 0; cpos <= i - startseg; cpos++) {
							if (ll->chars[cpos + startseg] == ' ') {
								if (drawWhitespaceBackground &&
								        (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways)) {
									PRectangle rcSpace(ll->positions[cpos + startseg] + xStart, rcSegment.top,
									                   ll->positions[cpos + startseg + 1] + xStart, rcSegment.bottom);
									surface->FillRectangle(rcSpace, vsDraw.whitespaceBackground.allocated);
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
			startseg = i + 1;
		}
	}

	if (twoPhaseDraw) {
		DrawEOL(surface, vsDraw, rcLine, ll, line, lineEnd,
		        xStart, subLine, subLineStart, overrideBackground, background,
		        drawWrapMarkEnd, wrapColour);
	}

	if (vsDraw.edgeState == EDGE_LINE) {
		int edgeX = theEdge * vsDraw.spaceWidth;
		rcSegment.left = edgeX + xStart;
		rcSegment.right = rcSegment.left + 1;
		surface->FillRectangle(rcSegment, vsDraw.edgecolour.allocated);
	}

	inIndentation = subLine == 0;	// Do not handle indentation except on first subline.
	startseg = ll->LineStart(subLine);
	// Foreground drawing loop
	for (i = lineStart; i < lineEnd; i++) {

		int iDoc = i + posLineStart;
		// If there is the end of a style run for any reason
		if ((ll->styles[i] != ll->styles[i + 1]) ||
		        i == (lineEnd - 1) ||
		        IsControlCharacter(ll->chars[i]) || IsControlCharacter(ll->chars[i + 1]) ||
		        ((ll->selStart != ll->selEnd) && ((iDoc + 1 == ll->selStart) || (iDoc + 1 == ll->selEnd))) ||
		        (i == (ll->edgeColumn - 1))) {
			rcSegment.left = ll->positions[startseg] + xStart - subLineStart;
			rcSegment.right = ll->positions[i + 1] + xStart - subLineStart;
			// Only try to draw if really visible - enhances performance by not calling environment to
			// draw strings that are completely past the right side of the window.
			if ((rcSegment.left <= rcLine.right) && (rcSegment.right >= rcLine.left)) {
				int styleMain = ll->styles[i];
				ColourAllocated textFore = vsDraw.styles[styleMain].fore.allocated;
				Font &textFont = vsDraw.styles[styleMain].font;
				//hotspot foreground
				if (ll->hsStart != -1 && iDoc >= ll->hsStart && iDoc < hsEnd) {
					if (vsDraw.hotspotForegroundSet)
						textFore = vsDraw.hotspotForeground.allocated;
				}
				bool inSelection = (iDoc >= ll->selStart) && (iDoc < ll->selEnd) && (ll->selStart != ll->selEnd);
				if (inSelection && (vsDraw.selforeset)) {
					textFore = vsDraw.selforeground.allocated;
				}
				bool inHotspot = (ll->hsStart != -1) && (iDoc >= ll->hsStart) && (iDoc < ll->hsEnd);
				ColourAllocated textBack = TextBackground(vsDraw, overrideBackground, background, inSelection, inHotspot, styleMain, i, ll);
				if (ll->chars[i] == '\t') {
					// Tab display
					if (!twoPhaseDraw) {
						if (drawWhitespaceBackground &&
						        (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways))
							textBack = vsDraw.whitespaceBackground.allocated;
						surface->FillRectangle(rcSegment, textBack);
					}
					if ((vsDraw.viewWhitespace != wsInvisible) || ((inIndentation && vsDraw.viewIndentationGuides))) {
						if (vsDraw.whitespaceForegroundSet)
							textFore = vsDraw.whitespaceForeground.allocated;
						surface->PenColour(textFore);
					}
					if (inIndentation && vsDraw.viewIndentationGuides) {
						for (int xIG = ll->positions[i] / indentWidth * indentWidth; xIG < ll->positions[i + 1]; xIG += indentWidth) {
							if (xIG >= ll->positions[i] && xIG > 0) {
								DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, xIG + xStart, rcSegment,
								                (ll->xHighlightGuide == xIG));
							}
						}
					}
					if (vsDraw.viewWhitespace != wsInvisible) {
						if (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways) {
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
						if (!twoPhaseDraw) {
							surface->FillRectangle(rcSegment, textBack);
						}
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
						                         rcSegment.top + vsDraw.maxAscent, ctrlChar, istrlen(ctrlChar),
						                         textBack, textFore);
					} else {
						char cc[2] = { static_cast<char>(controlCharSymbol), '\0' };
						surface->DrawTextNoClip(rcSegment, ctrlCharsFont,
						                        rcSegment.top + vsDraw.maxAscent,
						                        cc, 1, textBack, textFore);
					}
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
					        (inIndentation && vsDraw.viewIndentationGuides)) {
						for (int cpos = 0; cpos <= i - startseg; cpos++) {
							if (ll->chars[cpos + startseg] == ' ') {
								if (vsDraw.viewWhitespace != wsInvisible) {
									if (vsDraw.whitespaceForegroundSet)
										textFore = vsDraw.whitespaceForeground.allocated;
									if (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways) {
										int xmid = (ll->positions[cpos + startseg] + ll->positions[cpos + startseg + 1]) / 2;
										if (!twoPhaseDraw && drawWhitespaceBackground &&
										        (!inIndentation || vsDraw.viewWhitespace == wsVisibleAlways)) {
											textBack = vsDraw.whitespaceBackground.allocated;
											PRectangle rcSpace(ll->positions[cpos + startseg] + xStart, rcSegment.top, ll->positions[cpos + startseg + 1] + xStart, rcSegment.bottom);
											surface->FillRectangle(rcSpace, textBack);
										}
										PRectangle rcDot(xmid + xStart - subLineStart, rcSegment.top + vsDraw.lineHeight / 2, 0, 0);
										rcDot.right = rcDot.left + 1;
										rcDot.bottom = rcDot.top + 1;
										surface->FillRectangle(rcDot, textFore);
									}
								}
								if (inIndentation && vsDraw.viewIndentationGuides) {
									int startSpace = ll->positions[cpos + startseg];
									if (startSpace > 0 && (startSpace % indentWidth == 0)) {
										DrawIndentGuide(surface, lineVisible, vsDraw.lineHeight, startSpace + xStart, rcSegment,
										                (ll->xHighlightGuide == ll->positions[cpos + startseg]));
									}
								}
							} else {
								inIndentation = false;
							}
						}
					}
				}
				if (ll->hsStart != -1 && vsDraw.hotspotUnderline && iDoc >= ll->hsStart && iDoc < ll->hsEnd ) {
					PRectangle rcUL = rcSegment;
					rcUL.top = rcUL.top + vsDraw.maxAscent + 1;
					rcUL.bottom = rcUL.top + 1;
					if (vsDraw.hotspotForegroundSet)
						surface->FillRectangle(rcUL, vsDraw.hotspotForeground.allocated);
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
			startseg = i + 1;
		}
	}

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
					PRectangle rcIndic(
						ll->positions[startPos] + xStart - subLineStart,
						rcLine.top + vsDraw.maxAscent,
						ll->positions[indicPos] + xStart - subLineStart,
						rcLine.top + vsDraw.maxAscent + 3);
					vsDraw.indicators[indicnum].Draw(surface, rcIndic, rcLine);
					// RESET control var
					startPos = -1;
				}
			}
		}
		mask <<= 1;
	}
	// End of the drawing of the current line
	if (!twoPhaseDraw) {
		DrawEOL(surface, vsDraw, rcLine, ll, line, lineEnd,
		        xStart, subLine, subLineStart, overrideBackground, background,
		        drawWrapMarkEnd, wrapColour);
	}
	if ((vsDraw.selAlpha != SC_ALPHA_NOALPHA) && (ll->selStart >= 0) && (ll->selEnd >= 0)) {
		int startPosSel = (ll->selStart < posLineStart) ? posLineStart : ll->selStart;
		int endPosSel = (ll->selEnd < (lineEnd + posLineStart)) ? ll->selEnd : (lineEnd + posLineStart);
		if (startPosSel < endPosSel) {
			rcSegment.left = xStart + ll->positions[startPosSel - posLineStart] - subLineStart;
			rcSegment.right = xStart + ll->positions[endPosSel - posLineStart] - subLineStart;
			SimpleAlphaRectangle(surface, rcSegment, SelectionBackground(vsDraw), vsDraw.selAlpha);
		}
	}

	// Draw any translucent whole line states
	rcSegment.left = xStart;
	rcSegment.right = rcLine.right - 1;
	if (caret.active && vsDraw.showCaretLineBackground && ll->containsCaret) {
		SimpleAlphaRectangle(surface, rcSegment, vsDraw.caretLineBackground.allocated, vsDraw.caretLineAlpha);
	}
	int marks = pdoc->GetMark(line);
	for (int markBit = 0; (markBit < 32) && marks; markBit++) {
		if ((marks & 1) && (vsDraw.markers[markBit].markType == SC_MARK_BACKGROUND)) {
			SimpleAlphaRectangle(surface, rcSegment, vsDraw.markers[markBit].back.allocated, vsDraw.markers[markBit].alpha);
		}
		marks >>= 1;
	}
	if (vsDraw.maskInLine) {
		int marksMasked = pdoc->GetMark(line) & vsDraw.maskInLine;
		if (marksMasked) {
			for (int markBit = 0; (markBit < 32) && marksMasked; markBit++) {
				if ((marksMasked & 1) && (vsDraw.markers[markBit].markType != SC_MARK_EMPTY)) {
					SimpleAlphaRectangle(surface, rcSegment, vsDraw.markers[markBit].back.allocated, vsDraw.markers[markBit].alpha);
				}
				marksMasked >>= 1;
			}
		}
	}
}

void Editor::RefreshPixMaps(Surface *surfaceWindow) {
	if (!pixmapSelPattern->Initialised()) {
		const int patternSize = 8;
		pixmapSelPattern->InitPixMap(patternSize, patternSize, surfaceWindow, wMain.GetID());
		// This complex procedure is to reproduce the checkerboard dithered pattern used by windows
		// for scroll bars and Visual Studio for its selection margin. The colour of this pattern is half
		// way between the chrome colour and the chrome highlight colour making a nice transition
		// between the window chrome and the content area. And it works in low colour depths.
		PRectangle rcPattern(0, 0, patternSize, patternSize);

		// Initialize default colours based on the chrome colour scheme.  Typically the highlight is white.
		ColourAllocated colourFMFill = vs.selbar.allocated;
		ColourAllocated colourFMStripes = vs.selbarlight.allocated;

		if (!(vs.selbarlight.desired == ColourDesired(0xff, 0xff, 0xff))) {
			// User has chosen an unusual chrome colour scheme so just use the highlight edge colour.
			// (Typically, the highlight colour is white.)
			colourFMFill = vs.selbarlight.allocated;
		}

		if (vs.foldmarginColourSet) {
			// override default fold margin colour
			colourFMFill = vs.foldmarginColour.allocated;
		}
		if (vs.foldmarginHighlightColourSet) {
			// override default fold margin highlight colour
			colourFMStripes = vs.foldmarginHighlightColour.allocated;
		}

		pixmapSelPattern->FillRectangle(rcPattern, colourFMFill);
		pixmapSelPattern->PenColour(colourFMStripes);
		for (int stripe = 0; stripe < patternSize; stripe++) {
			// Alternating 1 pixel stripes is same as checkerboard.
			pixmapSelPattern->MoveTo(0, stripe * 2);
			pixmapSelPattern->LineTo(patternSize, stripe * 2 - patternSize);
		}
	}

	if (!pixmapIndentGuide->Initialised()) {
		// 1 extra pixel in height so can handle odd/even positions and so produce a continuous line
		pixmapIndentGuide->InitPixMap(1, vs.lineHeight + 1, surfaceWindow, wMain.GetID());
		pixmapIndentGuideHighlight->InitPixMap(1, vs.lineHeight + 1, surfaceWindow, wMain.GetID());
		PRectangle rcIG(0, 0, 1, vs.lineHeight);
		pixmapIndentGuide->FillRectangle(rcIG, vs.styles[STYLE_INDENTGUIDE].back.allocated);
		pixmapIndentGuide->PenColour(vs.styles[STYLE_INDENTGUIDE].fore.allocated);
		pixmapIndentGuideHighlight->FillRectangle(rcIG, vs.styles[STYLE_BRACELIGHT].back.allocated);
		pixmapIndentGuideHighlight->PenColour(vs.styles[STYLE_BRACELIGHT].fore.allocated);
		for (int stripe = 1; stripe < vs.lineHeight + 1; stripe += 2) {
			pixmapIndentGuide->MoveTo(0, stripe);
			pixmapIndentGuide->LineTo(2, stripe);
			pixmapIndentGuideHighlight->MoveTo(0, stripe);
			pixmapIndentGuideHighlight->LineTo(2, stripe);
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

void Editor::Paint(Surface *surfaceWindow, PRectangle rcArea) {
	//Platform::DebugPrintf("Paint:%1d (%3d,%3d) ... (%3d,%3d)\n",
	//	paintingAllText, rcArea.left, rcArea.top, rcArea.right, rcArea.bottom);

	RefreshStyleData();
	RefreshPixMaps(surfaceWindow);

	PRectangle rcClient = GetClientRectangle();
	//Platform::DebugPrintf("Client: (%3d,%3d) ... (%3d,%3d)   %d\n",
	//	rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

	surfaceWindow->SetPalette(&palette, true);
	pixmapLine->SetPalette(&palette, !hasFocus);

	int screenLinePaintFirst = rcArea.top / vs.lineHeight;
	// The area to be painted plus one extra line is styled.
	// The extra line is to determine when a style change, such as starting a comment flows on to other lines.
	int lineStyleLast = topLine + (rcArea.bottom - 1) / vs.lineHeight + 1;
	//Platform::DebugPrintf("Paint lines = %d .. %d\n", topLine + screenLinePaintFirst, lineStyleLast);
	int endPosPaint = pdoc->Length();
	if (lineStyleLast < cs.LinesDisplayed())
		endPosPaint = pdoc->LineStart(cs.DocFromDisplay(lineStyleLast + 1));

	int xStart = vs.fixedColumnWidth - xOffset;
	int ypos = 0;
	if (!bufferedDraw)
		ypos += screenLinePaintFirst * vs.lineHeight;
	int yposScreen = screenLinePaintFirst * vs.lineHeight;

	// Ensure we are styled as far as we are painting.
	pdoc->EnsureStyledTo(endPosPaint);
	bool paintAbandonedByStyling = paintState == paintAbandoned;
	if (needUpdateUI) {
		NotifyUpdateUI();
		needUpdateUI = false;
		RefreshStyleData();
		RefreshPixMaps(surfaceWindow);
	}

	// Call priority lines wrap on a window of lines which are likely
	// to rendered with the following paint (that is wrap the visible
	// 	lines first).
	int startLineToWrap = cs.DocFromDisplay(topLine) - 5;
	if (startLineToWrap < 0)
		startLineToWrap = -1;
	if (WrapLines(false, startLineToWrap)) {
		// The wrapping process has changed the height of some lines so
		// abandon this paint for a complete repaint.
		if (AbandonPaint()) {
			return;
		}
		RefreshPixMaps(surfaceWindow);	// In case pixmaps invalidated by scrollbar change
	}
	PLATFORM_ASSERT(pixmapSelPattern->Initialised());

	PaintSelMargin(surfaceWindow, rcArea);

	PRectangle rcRightMargin = rcClient;
	rcRightMargin.left = rcRightMargin.right - vs.rightMarginWidth;
	if (rcArea.Intersects(rcRightMargin)) {
		surfaceWindow->FillRectangle(rcRightMargin, vs.styles[STYLE_DEFAULT].back.allocated);
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

	// Do the painting
	if (rcArea.right > vs.fixedColumnWidth) {

		Surface *surface = surfaceWindow;
		if (bufferedDraw) {
			surface = pixmapLine;
			PLATFORM_ASSERT(pixmapLine->Initialised());
		}
		surface->SetUnicodeMode(IsUnicodeMode());
		surface->SetDBCSMode(CodePage());

		int visibleLine = topLine + screenLinePaintFirst;

		int posCaret = currentPos;
		if (posDrag >= 0)
			posCaret = posDrag;
		int lineCaret = pdoc->LineFromPosition(posCaret);

		// Remove selection margin from drawing area so text will not be drawn
		// on it in unbuffered mode.
		PRectangle rcTextArea = rcClient;
		rcTextArea.left = vs.fixedColumnWidth;
		rcTextArea.right -= vs.rightMarginWidth;
		surfaceWindow->SetClip(rcTextArea);

		// Loop on visible lines
		//double durLayout = 0.0;
		//double durPaint = 0.0;
		//double durCopy = 0.0;
		//ElapsedTime etWhole;
		int lineDocPrevious = -1;	// Used to avoid laying out one document line multiple times
		AutoLineLayout ll(llc, 0);
		SelectionLineIterator lineIterator(this);
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
				// For rectangular selection this accesses the layout cache so should be after layout returned.
				lineIterator.SetAt(lineDoc);
				ll.Set(RetrieveLineLayout(lineDoc));
				LayoutLine(lineDoc, surface, vs, ll, wrapWidth);
				lineDocPrevious = lineDoc;
			}
			//durLayout += et.Duration(true);

			if (ll) {
				if (selType == selStream) {
					ll->selStart = SelectionStart();
					ll->selEnd = SelectionEnd();
				} else {
					ll->selStart = lineIterator.startPos;
					ll->selEnd = lineIterator.endPos;
				}
				ll->containsCaret = lineDoc == lineCaret;
				if (hideSelection) {
					ll->selStart = -1;
					ll->selEnd = -1;
					ll->containsCaret = false;
				}

				GetHotSpotRange(ll->hsStart, ll->hsEnd);

				PRectangle rcLine = rcClient;
				rcLine.top = ypos;
				rcLine.bottom = ypos + vs.lineHeight;

				Range rangeLine(pdoc->LineStart(lineDoc), pdoc->LineStart(lineDoc + 1));
				// Highlight the current braces if any
				ll->SetBracesHighlight(rangeLine, braces, static_cast<char>(bracesMatchStyle),
				                       highlightGuideColumn * vs.spaceWidth);

				// Draw the line
				DrawLine(surface, vs, lineDoc, visibleLine, xStart, rcLine, ll, subLine);
				//durPaint += et.Duration(true);

				// Restore the previous styles for the brace highlights in case layout is in cache.
				ll->RestoreBracesHighlight(rangeLine, braces);

				bool expanded = cs.GetExpanded(lineDoc);
				if ((foldFlags & SC_FOLDFLAG_BOX) == 0) {
					// Paint the line above the fold
					if ((expanded && (foldFlags & SC_FOLDFLAG_LINEBEFORE_EXPANDED))
					        ||
					        (!expanded && (foldFlags & SC_FOLDFLAG_LINEBEFORE_CONTRACTED))) {
						if (pdoc->GetLevel(lineDoc) & SC_FOLDLEVELHEADERFLAG) {
							PRectangle rcFoldLine = rcLine;
							rcFoldLine.bottom = rcFoldLine.top + 1;
							surface->FillRectangle(rcFoldLine, vs.styles[STYLE_DEFAULT].fore.allocated);
						}
					}
					// Paint the line below the fold
					if ((expanded && (foldFlags & SC_FOLDFLAG_LINEAFTER_EXPANDED))
					        ||
					        (!expanded && (foldFlags & SC_FOLDFLAG_LINEAFTER_CONTRACTED))) {
						if (pdoc->GetLevel(lineDoc) & SC_FOLDLEVELHEADERFLAG) {
							PRectangle rcFoldLine = rcLine;
							rcFoldLine.top = rcFoldLine.bottom - 1;
							surface->FillRectangle(rcFoldLine, vs.styles[STYLE_DEFAULT].fore.allocated);
						}
					}
				} else {
					int FoldLevelCurr = (pdoc->GetLevel(lineDoc) & SC_FOLDLEVELNUMBERMASK) - SC_FOLDLEVELBASE;
					int FoldLevelPrev = (pdoc->GetLevel(lineDoc - 1) & SC_FOLDLEVELNUMBERMASK) - SC_FOLDLEVELBASE;
					int FoldLevelFlags = (pdoc->GetLevel(lineDoc) & ~SC_FOLDLEVELNUMBERMASK) & ~(0xFFF0000);
					int indentationStep = pdoc->IndentSize();
					// Draw line above fold
					if ((FoldLevelPrev < FoldLevelCurr)
					        ||
					        (FoldLevelFlags & SC_FOLDLEVELBOXHEADERFLAG
					         &&
					         (pdoc->GetLevel(lineDoc - 1) & SC_FOLDLEVELBOXFOOTERFLAG) == 0)) {
						PRectangle rcFoldLine = rcLine;
						rcFoldLine.bottom = rcFoldLine.top + 1;
						rcFoldLine.left += xStart + FoldLevelCurr * vs.spaceWidth * indentationStep - 1;
						surface->FillRectangle(rcFoldLine, vs.styles[STYLE_DEFAULT].fore.allocated);
					}

					// Line below the fold (or below a contracted fold)
					if (FoldLevelFlags & SC_FOLDLEVELBOXFOOTERFLAG
					        ||
					        (!expanded && (foldFlags & SC_FOLDFLAG_LINEAFTER_CONTRACTED))) {
						PRectangle rcFoldLine = rcLine;
						rcFoldLine.top = rcFoldLine.bottom - 1;
						rcFoldLine.left += xStart + (FoldLevelCurr) * vs.spaceWidth * indentationStep - 1;
						surface->FillRectangle(rcFoldLine, vs.styles[STYLE_DEFAULT].fore.allocated);
					}

					PRectangle rcBoxLine = rcLine;
					// Draw vertical line for every fold level
					for (int i = 0; i <= FoldLevelCurr; i++) {
						rcBoxLine.left = xStart + i * vs.spaceWidth * indentationStep - 1;
						rcBoxLine.right = rcBoxLine.left + 1;
						surface->FillRectangle(rcBoxLine, vs.styles[STYLE_DEFAULT].fore.allocated);
					}
				}

				// Draw the Caret
				if (lineDoc == lineCaret) {
					int offset = Platform::Minimum(posCaret - rangeLine.start, ll->maxLineLength);
					if ((offset >= ll->LineStart(subLine)) &&
					        ((offset < ll->LineStart(subLine + 1)) || offset == ll->numCharsInLine)) {
						int xposCaret = ll->positions[offset] - ll->positions[ll->LineStart(subLine)] + xStart;

						if (actualWrapVisualStartIndent != 0) {
							int lineStart = ll->LineStart(subLine);
							if (lineStart != 0)	// Wrapped
								xposCaret += actualWrapVisualStartIndent * vs.aveCharWidth;
						}
						int widthOverstrikeCaret;
						if (posCaret == pdoc->Length())	{   // At end of document
							widthOverstrikeCaret = vs.aveCharWidth;
						} else if ((posCaret - rangeLine.start) >= ll->numCharsInLine) {	// At end of line
							widthOverstrikeCaret = vs.aveCharWidth;
						} else {
							widthOverstrikeCaret = ll->positions[offset + 1] - ll->positions[offset];
						}
						if (widthOverstrikeCaret < 3)	// Make sure its visible
							widthOverstrikeCaret = 3;
						if (((caret.active && caret.on) || (posDrag >= 0)) && xposCaret >= 0) {
							PRectangle rcCaret = rcLine;
							int caretWidthOffset = 0;
							if ((offset > 0) && (vs.caretWidth > 1))
								caretWidthOffset = 1;	// Move back so overlaps both character cells.
							if (posDrag >= 0) {
								rcCaret.left = xposCaret - caretWidthOffset;
								rcCaret.right = rcCaret.left + vs.caretWidth;
							} else {
								if (inOverstrike) {
									rcCaret.top = rcCaret.bottom - 2;
									rcCaret.left = xposCaret + 1;
									rcCaret.right = rcCaret.left + widthOverstrikeCaret - 1;
								} else {
									rcCaret.left = xposCaret - caretWidthOffset;
									rcCaret.right = rcCaret.left + vs.caretWidth;
								}
							}
							surface->FillRectangle(rcCaret, vs.caretcolour.allocated);
						}
					}
				}

				if (bufferedDraw) {
					Point from(vs.fixedColumnWidth, 0);
					PRectangle rcCopyArea(vs.fixedColumnWidth, yposScreen,
					                      rcClient.right, yposScreen + vs.lineHeight);
					surfaceWindow->Copy(rcCopyArea, from, *pixmapLine);
				}
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
		PRectangle rcBeyondEOF = rcClient;
		rcBeyondEOF.left = vs.fixedColumnWidth;
		rcBeyondEOF.right = rcBeyondEOF.right;
		rcBeyondEOF.top = (cs.LinesDisplayed() - topLine) * vs.lineHeight;
		if (rcBeyondEOF.top < rcBeyondEOF.bottom) {
			surfaceWindow->FillRectangle(rcBeyondEOF, vs.styles[STYLE_DEFAULT].back.allocated);
			if (vs.edgeState == EDGE_LINE) {
				int edgeX = theEdge * vs.spaceWidth;
				rcBeyondEOF.left = edgeX + xStart;
				rcBeyondEOF.right = rcBeyondEOF.left + 1;
				surfaceWindow->FillRectangle(rcBeyondEOF, vs.edgecolour.allocated);
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
long Editor::FormatRange(bool draw, RangeToFormat *pfr) {
	if (!pfr)
		return 0;

	AutoSurface surface(pfr->hdc, this);
	if (!surface)
		return 0;
	AutoSurface surfaceMeasure(pfr->hdcTarget, this);
	if (!surfaceMeasure) {
		return 0;
	}

	ViewStyle vsPrint(vs);

	// Modify the view style for printing as do not normally want any of the transient features to be printed
	// Printing supports only the line number margin.
	int lineNumberIndex = -1;
	for (int margin = 0; margin < ViewStyle::margins; margin++) {
		if ((vsPrint.ms[margin].style == SC_MARGIN_NUMBER) && (vsPrint.ms[margin].width > 0)) {
			lineNumberIndex = margin;
		} else {
			vsPrint.ms[margin].width = 0;
		}
	}
	vsPrint.showMarkedLines = false;
	vsPrint.fixedColumnWidth = 0;
	vsPrint.zoomLevel = printMagnification;
	vsPrint.viewIndentationGuides = false;
	// Don't show the selection when printing
	vsPrint.selbackset = false;
	vsPrint.selforeset = false;
	vsPrint.selAlpha = SC_ALPHA_NOALPHA;
	vsPrint.whitespaceBackgroundSet = false;
	vsPrint.whitespaceForegroundSet = false;
	vsPrint.showCaretLineBackground = false;

	// Set colours for printing according to users settings
	for (int sty = 0;sty <= STYLE_MAX;sty++) {
		if (printColourMode == SC_PRINT_INVERTLIGHT) {
			vsPrint.styles[sty].fore.desired = InvertedLight(vsPrint.styles[sty].fore.desired);
			vsPrint.styles[sty].back.desired = InvertedLight(vsPrint.styles[sty].back.desired);
		} else if (printColourMode == SC_PRINT_BLACKONWHITE) {
			vsPrint.styles[sty].fore.desired = ColourDesired(0, 0, 0);
			vsPrint.styles[sty].back.desired = ColourDesired(0xff, 0xff, 0xff);
		} else if (printColourMode == SC_PRINT_COLOURONWHITE) {
			vsPrint.styles[sty].back.desired = ColourDesired(0xff, 0xff, 0xff);
		} else if (printColourMode == SC_PRINT_COLOURONWHITEDEFAULTBG) {
			if (sty <= STYLE_DEFAULT) {
				vsPrint.styles[sty].back.desired = ColourDesired(0xff, 0xff, 0xff);
			}
		}
	}
	// White background for the line numbers
	vsPrint.styles[STYLE_LINENUMBER].back.desired = ColourDesired(0xff, 0xff, 0xff);

	vsPrint.Refresh(*surfaceMeasure);
	// Determining width must hapen after fonts have been realised in Refresh
	int lineNumberWidth = 0;
	if (lineNumberIndex >= 0) {
		lineNumberWidth = surfaceMeasure->WidthText(vsPrint.styles[STYLE_LINENUMBER].font,
		                  "99999" lineNumberPrintSpace, 5 + istrlen(lineNumberPrintSpace));
		vsPrint.ms[lineNumberIndex].width = lineNumberWidth;
		vsPrint.Refresh(*surfaceMeasure);	// Recalculate fixedColumnWidth
	}
	// Ensure colours are set up
	vsPrint.RefreshColourPalette(palette, true);
	vsPrint.RefreshColourPalette(palette, false);

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
	int widthPrint = pfr->rc.Width() - vsPrint.fixedColumnWidth;
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
		LineLayout ll(8000);
		LayoutLine(lineDoc, surfaceMeasure, vsPrint, &ll, widthPrint);

		ll.selStart = -1;
		ll.selEnd = -1;
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
			                        vsPrint.styles[STYLE_LINENUMBER].fore.allocated,
			                        vsPrint.styles[STYLE_LINENUMBER].back.allocated);
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
	DropGraphics();
	SetScrollBars();
	if (wrapState != eWrapNone) {
		PRectangle rcTextArea = GetClientRectangle();
		rcTextArea.left = vs.fixedColumnWidth;
		rcTextArea.right -= vs.rightMarginWidth;
		if (wrapWidth != rcTextArea.Width()) {
			NeedWrapping();
			Redraw();
		}
	}
}

void Editor::AddChar(char ch) {
	char s[2];
	s[0] = ch;
	s[1] = '\0';
	AddCharUTF(s, 1);
}

void Editor::AddCharUTF(char *s, unsigned int len, bool treatAsDBCS) {
	bool wasSelection = currentPos != anchor;
	ClearSelection();
	bool charReplaceAction = false;
	if (inOverstrike && !wasSelection && !RangeContainsProtected(currentPos, currentPos + 1)) {
		if (currentPos < (pdoc->Length())) {
			if (!IsEOLChar(pdoc->CharAt(currentPos))) {
				charReplaceAction = true;
				pdoc->BeginUndoAction();
				pdoc->DelChar(currentPos);
			}
		}
	}
	if (pdoc->InsertString(currentPos, s, len)) {
		SetEmptySelection(currentPos + len);
	}
	if (charReplaceAction) {
		pdoc->EndUndoAction();
	}
	EnsureCaretVisible();
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
	if (!caretSticky) {
		SetLastXChosen();
	}

	if (treatAsDBCS) {
		NotifyChar((static_cast<unsigned char>(s[0]) << 8) |
		           static_cast<unsigned char>(s[1]));
	} else {
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
}

void Editor::ClearSelection() {
	if (!SelectionContainsProtected()) {
		int startPos = SelectionStart();
		if (selType == selStream) {
			unsigned int chars = SelectionEnd() - startPos;
			if (0 != chars) {
				pdoc->BeginUndoAction();
				pdoc->DeleteChars(startPos, chars);
				pdoc->EndUndoAction();
			}
		} else {
			pdoc->BeginUndoAction();
			SelectionLineIterator lineIterator(this, false);
			while (lineIterator.Iterate()) {
				startPos = lineIterator.startPos;
				unsigned int chars = lineIterator.endPos - startPos;
				if (0 != chars) {
					pdoc->DeleteChars(startPos, chars);
				}
			}
			pdoc->EndUndoAction();
			selType = selStream;
		}
		SetEmptySelection(startPos);
	}
}

void Editor::ClearAll() {
	pdoc->BeginUndoAction();
	if (0 != pdoc->Length()) {
		pdoc->DeleteChars(0, pdoc->Length());
	}
	if (!pdoc->IsReadOnly()) {
		cs.Clear();
	}
	pdoc->EndUndoAction();
	anchor = 0;
	currentPos = 0;
	SetTopLine(0);
	SetVerticalScrollPos();
	InvalidateStyleRedraw();
}

void Editor::ClearDocumentStyle() {
	pdoc->StartStyling(0, '\377');
	pdoc->SetStyleFor(pdoc->Length(), 0);
	cs.ShowAll();
	pdoc->ClearLevels();
}

void Editor::Cut() {
	if (!pdoc->IsReadOnly() && !SelectionContainsProtected()) {
		Copy();
		ClearSelection();
	}
}

void Editor::PasteRectangular(int pos, const char *ptr, int len) {
	if (pdoc->IsReadOnly() || SelectionContainsProtected()) {
		return;
	}
	currentPos = pos;
	int xInsert = XFromPosition(currentPos);
	int line = pdoc->LineFromPosition(currentPos);
	bool prevCr = false;
	pdoc->BeginUndoAction();
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
			currentPos = PositionFromLineX(line, xInsert);
			if ((XFromPosition(currentPos) < xInsert) && (i + 1 < len)) {
				for (int i = 0; i < xInsert - XFromPosition(currentPos); i++) {
					pdoc->InsertChar(currentPos, ' ');
					currentPos++;
				}
			}
			prevCr = ptr[i] == '\r';
		} else {
			pdoc->InsertString(currentPos, ptr + i, 1);
			currentPos++;
			prevCr = false;
		}
	}
	pdoc->EndUndoAction();
	SetEmptySelection(pos);
}

bool Editor::CanPaste() {
	return !pdoc->IsReadOnly() && !SelectionContainsProtected();
}

void Editor::Clear() {
	if (currentPos == anchor) {
		if (!RangeContainsProtected(currentPos, currentPos + 1)) {
			DelChar();
		}
	} else {
		ClearSelection();
	}
	SetEmptySelection(currentPos);
}

void Editor::SelectAll() {
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
	if (!RangeContainsProtected(currentPos, currentPos + 1)) {
		pdoc->DelChar(currentPos);
	}
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

void Editor::DelCharBack(bool allowLineStartDeletion) {
	if (currentPos == anchor) {
		if (!RangeContainsProtected(currentPos - 1, currentPos)) {
			int lineCurrentPos = pdoc->LineFromPosition(currentPos);
			if (allowLineStartDeletion || (pdoc->LineStart(lineCurrentPos) != currentPos)) {
				if (pdoc->GetColumn(currentPos) <= pdoc->GetLineIndentation(lineCurrentPos) &&
				        pdoc->GetColumn(currentPos) > 0 && pdoc->backspaceUnindents) {
					pdoc->BeginUndoAction();
					int indentation = pdoc->GetLineIndentation(lineCurrentPos);
					int indentationStep = pdoc->IndentSize();
					if (indentation % indentationStep == 0) {
						pdoc->SetLineIndentation(lineCurrentPos, indentation - indentationStep);
					} else {
						pdoc->SetLineIndentation(lineCurrentPos, indentation - (indentation % indentationStep));
					}
					SetEmptySelection(pdoc->GetLineIndentPosition(lineCurrentPos));
					pdoc->EndUndoAction();
				} else {
					pdoc->DelCharBack(currentPos);
				}
			}
		}
	} else {
		ClearSelection();
		SetEmptySelection(currentPos);
	}
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

void Editor::NotifyFocus(bool) {}

void Editor::NotifyStyleToNeeded(int endStyleNeeded) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_STYLENEEDED;
	scn.position = endStyleNeeded;
	NotifyParent(scn);
}

void Editor::NotifyStyleNeeded(Document*, void *, int endStyleNeeded) {
	NotifyStyleToNeeded(endStyleNeeded);
}

void Editor::NotifyChar(int ch) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_CHARADDED;
	scn.ch = ch;
	NotifyParent(scn);
	if (recordingMacro) {
		char txt[2];
		txt[0] = static_cast<char>(ch);
		txt[1] = '\0';
		NotifyMacroRecord(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(txt));
	}
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
	scn.position = PositionFromLocationClose(pt);
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

void Editor::NotifyUpdateUI() {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_UPDATEUI;
	NotifyParent(scn);
}

void Editor::NotifyPainted() {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_PAINTED;
	NotifyParent(scn);
}

bool Editor::NotifyMarginClick(Point pt, bool shift, bool ctrl, bool alt) {
	int marginClicked = -1;
	int x = 0;
	for (int margin = 0; margin < ViewStyle::margins; margin++) {
		if ((pt.x > x) && (pt.x < x + vs.ms[margin].width))
			marginClicked = margin;
		x += vs.ms[margin].width;
	}
	if ((marginClicked >= 0) && vs.ms[marginClicked].sensitive) {
		SCNotification scn = {0};
		scn.nmhdr.code = SCN_MARGINCLICK;
		scn.modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) |
		                (alt ? SCI_ALT : 0);
		scn.position = pdoc->LineStart(LineFromLocation(pt));
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
	scn.position = PositionFromLocationClose(pt);
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
void Editor::NotifyModifyAttempt(Document*, void *) {
	//Platform::DebugPrintf("** Modify Attempt\n");
	NotifyModifyAttempt();
}

void Editor::NotifyMove(int position) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_POSCHANGED;
	scn.position = position;
	NotifyParent(scn);
}

void Editor::NotifySavePoint(Document*, void *, bool atSavePoint) {
	//Platform::DebugPrintf("** Save Point %s\n", atSavePoint ? "On" : "Off");
	NotifySavePoint(atSavePoint);
}

void Editor::CheckModificationForWrap(DocModification mh) {
	if (mh.modificationType & (SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT)) {
		llc.Invalidate(LineLayout::llCheckTextAndStyle);
		if (wrapState != eWrapNone) {
			int lineDoc = pdoc->LineFromPosition(mh.position);
			int lines = Platform::Maximum(0, mh.linesAdded);
			NeedWrapping(lineDoc, lineDoc + lines + 1);
		}
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

void Editor::NotifyModified(Document*, DocModification mh, void *) {
	needUpdateUI = true;
	if (paintState == painting) {
		CheckForChangeOutsidePaint(Range(mh.position, mh.position + mh.length));
	}
	if (mh.modificationType & SC_MOD_CHANGESTYLE) {
		pdoc->IncrementStyleClock();
		if (paintState == notPainting) {
			if (mh.position < pdoc->LineStart(topLine)) {
				// Styling performed before this view
				Redraw();
			} else {
				InvalidateRange(mh.position, mh.position + mh.length);
			}
		}
		llc.Invalidate(LineLayout::llCheckTextAndStyle);
	} else {
		// Move selection and brace highlights
		if (mh.modificationType & SC_MOD_INSERTTEXT) {
			currentPos = MovePositionForInsertion(currentPos, mh.position, mh.length);
			anchor = MovePositionForInsertion(anchor, mh.position, mh.length);
			braces[0] = MovePositionForInsertion(braces[0], mh.position, mh.length);
			braces[1] = MovePositionForInsertion(braces[1], mh.position, mh.length);
		} else if (mh.modificationType & SC_MOD_DELETETEXT) {
			currentPos = MovePositionForDeletion(currentPos, mh.position, mh.length);
			anchor = MovePositionForDeletion(anchor, mh.position, mh.length);
			braces[0] = MovePositionForDeletion(braces[0], mh.position, mh.length);
			braces[1] = MovePositionForDeletion(braces[1], mh.position, mh.length);
		}
		if (cs.LinesDisplayed() < cs.LinesInDoc()) {
			// Some lines are hidden so may need shown.
			// TODO: check if the modified area is hidden.
			if (mh.modificationType & SC_MOD_BEFOREINSERT) {
				NotifyNeedShown(mh.position, 0);
			} else if (mh.modificationType & SC_MOD_BEFOREDELETE) {
				NotifyNeedShown(mh.position, mh.length);
			}
		}
		if (mh.linesAdded != 0) {
			// Update contraction state for inserted and removed lines
			// lineOfPos should be calculated in context of state before modification, shouldn't it
			int lineOfPos = pdoc->LineFromPosition(mh.position);
			if (mh.linesAdded > 0) {
				cs.InsertLines(lineOfPos, mh.linesAdded);
			} else {
				cs.DeleteLines(lineOfPos, -mh.linesAdded);
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

			//Platform::DebugPrintf("** %x Doc Changed\n", this);
			// TODO: could invalidate from mh.startModification to end of screen
			//InvalidateRange(mh.position, mh.position + mh.length);
			if (paintState == notPainting && !CanDeferToLastStep(mh)) {
				Redraw();
			}
		} else {
			//Platform::DebugPrintf("** %x Line Changed %d .. %d\n", this,
			//	mh.position, mh.position + mh.length);
			if (paintState == notPainting && mh.length && !CanEliminate(mh)) {
				InvalidateRange(mh.position, mh.position + mh.length);
			}
		}
	}

	if (mh.linesAdded != 0 && !CanDeferToLastStep(mh)) {
		SetScrollBars();
	}

	if (mh.modificationType & SC_MOD_CHANGEMARKER) {
		if ((paintState == notPainting) || !PaintContainsMargin()) {
			if (mh.modificationType & SC_MOD_CHANGEFOLD) {
				// Fold changes can affect the drawing of following lines so redraw whole margin
				RedrawSelMargin();
			} else {
				RedrawSelMargin(mh.line);
			}
		}
	}

	// NOW pay the piper WRT "deferred" visual updates
	if (IsLastStep(mh)) {
		SetScrollBars();
		Redraw();
	}

	// If client wants to see this modification
	if (mh.modificationType & modEventMask) {
		if ((mh.modificationType & SC_MOD_CHANGESTYLE) == 0) {
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
	case SCI_DELWORDLEFT:
	case SCI_DELWORDRIGHT:
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

/**
 * Force scroll and keep position relative to top of window.
 *
 * If stuttered = true and not already at first/last row, move to first/last row of window.
 * If stuttered = true and already at first/last row, scroll as normal.
 */
void Editor::PageMove(int direction, selTypes sel, bool stuttered) {
	int topLineNew, newPos;

	// I consider only the caretYSlop, and ignore the caretYPolicy-- is that a problem?
	int currentLine = pdoc->LineFromPosition(currentPos);
	int topStutterLine = topLine + caretYSlop;
	int bottomStutterLine =
		pdoc->LineFromPosition(PositionFromLocation(
		Point(lastXChosen, direction * vs.lineHeight * LinesToScroll())))
		- caretYSlop - 1;

	if (stuttered && (direction < 0 && currentLine > topStutterLine)) {
		topLineNew = topLine;
		newPos = PositionFromLocation(Point(lastXChosen, vs.lineHeight * caretYSlop));

	} else if (stuttered && (direction > 0 && currentLine < bottomStutterLine)) {
		topLineNew = topLine;
		newPos = PositionFromLocation(Point(lastXChosen, vs.lineHeight * (LinesToScroll() - caretYSlop)));

	} else {
		Point pt = LocationFromPosition(currentPos);

		topLineNew = Platform::Clamp(
	                     topLine + direction * LinesToScroll(), 0, MaxScrollPos());
		newPos = PositionFromLocation(
	                 Point(lastXChosen, pt.y + direction * (vs.lineHeight * LinesToScroll())));
	}

	if (topLineNew != topLine) {
		SetTopLine(topLineNew);
		MovePositionTo(newPos, sel);
		Redraw();
		SetVerticalScrollPos();
	} else {
		MovePositionTo(newPos, sel);
	}
}

void Editor::ChangeCaseOfSelection(bool makeUpperCase) {
	pdoc->BeginUndoAction();
	int startCurrent = currentPos;
	int startAnchor = anchor;
	if (selType == selStream) {
		pdoc->ChangeCase(Range(SelectionStart(), SelectionEnd()),
		                 makeUpperCase);
		SetSelection(startCurrent, startAnchor);
	} else {
		SelectionLineIterator lineIterator(this, false);
		while (lineIterator.Iterate()) {
			pdoc->ChangeCase(
			    Range(lineIterator.startPos, lineIterator.endPos),
			    makeUpperCase);
		}
		// Would be nicer to keep the rectangular selection but this is complex
		SetEmptySelection(startCurrent);
	}
	pdoc->EndUndoAction();
}

void Editor::LineTranspose() {
	int line = pdoc->LineFromPosition(currentPos);
	if (line > 0) {
		pdoc->BeginUndoAction();
		int startPrev = pdoc->LineStart(line - 1);
		int endPrev = pdoc->LineEnd(line - 1);
		int start = pdoc->LineStart(line);
		int end = pdoc->LineEnd(line);
		char *line1 = CopyRange(startPrev, endPrev);
		int len1 = endPrev - startPrev;
		char *line2 = CopyRange(start, end);
		int len2 = end - start;
		if (pdoc->DeleteChars(start, len2)) {
			pdoc->DeleteChars(startPrev, len1);
			pdoc->InsertString(startPrev, line2, len2);
			pdoc->InsertString(start - len1 + len2, line1, len1);
			MovePositionTo(start - len1 + len2);
		}
		delete []line1;
		delete []line2;
		pdoc->EndUndoAction();
	}
}

void Editor::Duplicate(bool forLine) {
	int start = SelectionStart();
	int end = SelectionEnd();
	if (start == end) {
		forLine = true;
	}
	if (forLine) {
		int line = pdoc->LineFromPosition(currentPos);
		start = pdoc->LineStart(line);
		end = pdoc->LineEnd(line);
	}
	char *text = CopyRange(start, end);
	if (forLine) {
		const char *eol = StringFromEOLMode(pdoc->eolMode);
		pdoc->InsertCString(end, eol);
		pdoc->InsertString(end + istrlen(eol), text, end - start);
	} else {
		pdoc->InsertString(end, text, end - start);
	}
	delete []text;
}

void Editor::CancelModes() {
	moveExtendsSelection = false;
}

void Editor::NewLine() {
	ClearSelection();
	const char *eol = "\n";
	if (pdoc->eolMode == SC_EOL_CRLF) {
		eol = "\r\n";
	} else if (pdoc->eolMode == SC_EOL_CR) {
		eol = "\r";
	} // else SC_EOL_LF -> "\n" already set
	if (pdoc->InsertCString(currentPos, eol)) {
		SetEmptySelection(currentPos + istrlen(eol));
		while (*eol) {
			NotifyChar(*eol);
			eol++;
		}
	}
	SetLastXChosen();
	EnsureCaretVisible();
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

void Editor::CursorUpOrDown(int direction, selTypes sel) {
	Point pt = LocationFromPosition(currentPos);
	int posNew = PositionFromLocation(
	                 Point(lastXChosen, pt.y + direction * vs.lineHeight));
	if (direction < 0) {
		// Line wrapping may lead to a location on the same line, so
		// seek back if that is the case.
		// There is an equivalent case when moving down which skips
		// over a line but as that does not trap the user it is fine.
		Point ptNew = LocationFromPosition(posNew);
		while ((posNew > 0) && (pt.y == ptNew.y)) {
			posNew--;
			ptNew = LocationFromPosition(posNew);
		}
	}
	MovePositionTo(posNew, sel);
}

void Editor::ParaUpOrDown(int direction, selTypes sel) {
	int lineDoc, savedPos = currentPos;
	do {
		MovePositionTo(direction > 0 ? pdoc->ParaDown(currentPos) : pdoc->ParaUp(currentPos), sel);
		lineDoc = pdoc->LineFromPosition(currentPos);
		if (direction > 0) {
			if (currentPos >= pdoc->Length() && !cs.GetVisible(lineDoc)) {
				if (sel == noSel) {
					MovePositionTo(pdoc->LineEndPosition(savedPos));
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
		CursorUpOrDown(1, selStream);
		break;
	case SCI_LINEDOWNRECTEXTEND:
		CursorUpOrDown(1, selRectangle);
		break;
	case SCI_PARADOWN:
		ParaUpOrDown(1);
		break;
	case SCI_PARADOWNEXTEND:
		ParaUpOrDown(1, selStream);
		break;
	case SCI_LINESCROLLDOWN:
		ScrollTo(topLine + 1);
		MoveCaretInsideView(false);
		break;
	case SCI_LINEUP:
		CursorUpOrDown(-1);
		break;
	case SCI_LINEUPEXTEND:
		CursorUpOrDown(-1, selStream);
		break;
	case SCI_LINEUPRECTEXTEND:
		CursorUpOrDown(-1, selRectangle);
		break;
	case SCI_PARAUP:
		ParaUpOrDown(-1);
		break;
	case SCI_PARAUPEXTEND:
		ParaUpOrDown(-1, selStream);
		break;
	case SCI_LINESCROLLUP:
		ScrollTo(topLine - 1);
		MoveCaretInsideView(false);
		break;
	case SCI_CHARLEFT:
		if (SelectionEmpty() || moveExtendsSelection) {
			MovePositionTo(MovePositionSoVisible(currentPos - 1, -1));
		} else {
			MovePositionTo(SelectionStart());
		}
		SetLastXChosen();
		break;
	case SCI_CHARLEFTEXTEND:
		MovePositionTo(MovePositionSoVisible(currentPos - 1, -1), selStream);
		SetLastXChosen();
		break;
	case SCI_CHARLEFTRECTEXTEND:
		MovePositionTo(MovePositionSoVisible(currentPos - 1, -1), selRectangle);
		SetLastXChosen();
		break;
	case SCI_CHARRIGHT:
		if (SelectionEmpty() || moveExtendsSelection) {
			MovePositionTo(MovePositionSoVisible(currentPos + 1, 1));
		} else {
			MovePositionTo(SelectionEnd());
		}
		SetLastXChosen();
		break;
	case SCI_CHARRIGHTEXTEND:
		MovePositionTo(MovePositionSoVisible(currentPos + 1, 1), selStream);
		SetLastXChosen();
		break;
	case SCI_CHARRIGHTRECTEXTEND:
		MovePositionTo(MovePositionSoVisible(currentPos + 1, 1), selRectangle);
		SetLastXChosen();
		break;
	case SCI_WORDLEFT:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(currentPos, -1), -1));
		SetLastXChosen();
		break;
	case SCI_WORDLEFTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(currentPos, -1), -1), selStream);
		SetLastXChosen();
		break;
	case SCI_WORDRIGHT:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(currentPos, 1), 1));
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(currentPos, 1), 1), selStream);
		SetLastXChosen();
		break;

	case SCI_WORDLEFTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(currentPos, -1), -1));
		SetLastXChosen();
		break;
	case SCI_WORDLEFTENDEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(currentPos, -1), -1), selStream);
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(currentPos, 1), 1));
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTENDEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(currentPos, 1), 1), selStream);
		SetLastXChosen();
		break;

	case SCI_HOME:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(currentPos)));
		SetLastXChosen();
		break;
	case SCI_HOMEEXTEND:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(currentPos)), selStream);
		SetLastXChosen();
		break;
	case SCI_HOMERECTEXTEND:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(currentPos)), selRectangle);
		SetLastXChosen();
		break;
	case SCI_LINEEND:
		MovePositionTo(pdoc->LineEndPosition(currentPos));
		SetLastXChosen();
		break;
	case SCI_LINEENDEXTEND:
		MovePositionTo(pdoc->LineEndPosition(currentPos), selStream);
		SetLastXChosen();
		break;
	case SCI_LINEENDRECTEXTEND:
		MovePositionTo(pdoc->LineEndPosition(currentPos), selRectangle);
		SetLastXChosen();
		break;
	case SCI_HOMEWRAP: {
			int homePos = MovePositionSoVisible(StartEndDisplayLine(currentPos, true), -1);
			if (currentPos <= homePos)
				homePos = pdoc->LineStart(pdoc->LineFromPosition(currentPos));
			MovePositionTo(homePos);
			SetLastXChosen();
		}
		break;
	case SCI_HOMEWRAPEXTEND: {
			int homePos = MovePositionSoVisible(StartEndDisplayLine(currentPos, true), -1);
			if (currentPos <= homePos)
				homePos = pdoc->LineStart(pdoc->LineFromPosition(currentPos));
			MovePositionTo(homePos, selStream);
			SetLastXChosen();
		}
		break;
	case SCI_LINEENDWRAP: {
			int endPos = MovePositionSoVisible(StartEndDisplayLine(currentPos, false), 1);
			int realEndPos = pdoc->LineEndPosition(currentPos);
			if (endPos > realEndPos      // if moved past visible EOLs
				|| currentPos >= endPos) // if at end of display line already
				endPos = realEndPos;
			MovePositionTo(endPos);
			SetLastXChosen();
		}
		break;
	case SCI_LINEENDWRAPEXTEND: {
			int endPos = MovePositionSoVisible(StartEndDisplayLine(currentPos, false), 1);
			int realEndPos = pdoc->LineEndPosition(currentPos);
			if (endPos > realEndPos      // if moved past visible EOLs
				|| currentPos >= endPos) // if at end of display line already
				endPos = realEndPos;
			MovePositionTo(endPos, selStream);
			SetLastXChosen();
		}
		break;
	case SCI_DOCUMENTSTART:
		MovePositionTo(0);
		SetLastXChosen();
		break;
	case SCI_DOCUMENTSTARTEXTEND:
		MovePositionTo(0, selStream);
		SetLastXChosen();
		break;
	case SCI_DOCUMENTEND:
		MovePositionTo(pdoc->Length());
		SetLastXChosen();
		break;
	case SCI_DOCUMENTENDEXTEND:
		MovePositionTo(pdoc->Length(), selStream);
		SetLastXChosen();
		break;
	case SCI_STUTTEREDPAGEUP:
		PageMove(-1, noSel, true);
		break;
	case SCI_STUTTEREDPAGEUPEXTEND:
		PageMove(-1, selStream, true);
		break;
	case SCI_STUTTEREDPAGEDOWN:
		PageMove(1, noSel, true);
		break;
	case SCI_STUTTEREDPAGEDOWNEXTEND:
		PageMove(1, selStream, true);
		break;
	case SCI_PAGEUP:
		PageMove(-1);
		break;
	case SCI_PAGEUPEXTEND:
		PageMove(-1, selStream);
		break;
	case SCI_PAGEUPRECTEXTEND:
		PageMove(-1, selRectangle);
		break;
	case SCI_PAGEDOWN:
		PageMove(1);
		break;
	case SCI_PAGEDOWNEXTEND:
		PageMove(1, selStream);
		break;
	case SCI_PAGEDOWNRECTEXTEND:
		PageMove(1, selRectangle);
		break;
	case SCI_EDITTOGGLEOVERTYPE:
		inOverstrike = !inOverstrike;
		DropCaret();
		ShowCaretAtCurrentPosition();
		NotifyUpdateUI();
		break;
	case SCI_CANCEL:            	// Cancel any modes - handled in subclass
		// Also unselect text
		CancelModes();
		break;
	case SCI_DELETEBACK:
		DelCharBack(true);
		if (!caretSticky) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case SCI_DELETEBACKNOTLINE:
		DelCharBack(false);
		if (!caretSticky) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case SCI_TAB:
		Indent(true);
		if (!caretSticky) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case SCI_BACKTAB:
		Indent(false);
		if (!caretSticky) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case SCI_NEWLINE:
		NewLine();
		break;
	case SCI_FORMFEED:
		AddChar('\f');
		break;
	case SCI_VCHOME:
		MovePositionTo(pdoc->VCHomePosition(currentPos));
		SetLastXChosen();
		break;
	case SCI_VCHOMEEXTEND:
		MovePositionTo(pdoc->VCHomePosition(currentPos), selStream);
		SetLastXChosen();
		break;
	case SCI_VCHOMERECTEXTEND:
		MovePositionTo(pdoc->VCHomePosition(currentPos), selRectangle);
		SetLastXChosen();
		break;
	case SCI_VCHOMEWRAP: {
			int homePos = pdoc->VCHomePosition(currentPos);
			int viewLineStart = MovePositionSoVisible(StartEndDisplayLine(currentPos, true), -1);
			if ((viewLineStart < currentPos) && (viewLineStart > homePos))
				homePos = viewLineStart;

			MovePositionTo(homePos);
			SetLastXChosen();
		}
		break;
	case SCI_VCHOMEWRAPEXTEND: {
			int homePos = pdoc->VCHomePosition(currentPos);
			int viewLineStart = MovePositionSoVisible(StartEndDisplayLine(currentPos, true), -1);
			if ((viewLineStart < currentPos) && (viewLineStart > homePos))
				homePos = viewLineStart;

			MovePositionTo(homePos, selStream);
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
			int startWord = pdoc->NextWordStart(currentPos, -1);
			pdoc->DeleteChars(startWord, currentPos - startWord);
			SetLastXChosen();
		}
		break;
	case SCI_DELWORDRIGHT: {
			int endWord = pdoc->NextWordStart(currentPos, 1);
			pdoc->DeleteChars(currentPos, endWord - currentPos);
		}
		break;
	case SCI_DELLINELEFT: {
			int line = pdoc->LineFromPosition(currentPos);
			int start = pdoc->LineStart(line);
			pdoc->DeleteChars(start, currentPos - start);
			SetLastXChosen();
		}
		break;
	case SCI_DELLINERIGHT: {
			int line = pdoc->LineFromPosition(currentPos);
			int end = pdoc->LineEnd(line);
			pdoc->DeleteChars(currentPos, end - currentPos);
		}
		break;
	case SCI_LINECOPY: {
			int lineStart = pdoc->LineFromPosition(SelectionStart());
			int lineEnd = pdoc->LineFromPosition(SelectionEnd());
			CopyRangeToClipboard(pdoc->LineStart(lineStart),
				pdoc->LineStart(lineEnd + 1));
		}
		break;
	case SCI_LINECUT: {
			int lineStart = pdoc->LineFromPosition(SelectionStart());
			int lineEnd = pdoc->LineFromPosition(SelectionEnd());
			int start = pdoc->LineStart(lineStart);
			int end = pdoc->LineStart(lineEnd + 1);
			SetSelection(start, end);
			Cut();
			SetLastXChosen();
		}
		break;
	case SCI_LINEDELETE: {
			int line = pdoc->LineFromPosition(currentPos);
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
		ChangeCaseOfSelection(false);
		break;
	case SCI_UPPERCASE:
		ChangeCaseOfSelection(true);
		break;
	case SCI_WORDPARTLEFT:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartLeft(currentPos), -1));
		SetLastXChosen();
		break;
	case SCI_WORDPARTLEFTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartLeft(currentPos), -1), selStream);
		SetLastXChosen();
		break;
	case SCI_WORDPARTRIGHT:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartRight(currentPos), 1));
		SetLastXChosen();
		break;
	case SCI_WORDPARTRIGHTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartRight(currentPos), 1), selStream);
		SetLastXChosen();
		break;
	case SCI_HOMEDISPLAY:
		MovePositionTo(MovePositionSoVisible(
		                   StartEndDisplayLine(currentPos, true), -1));
		SetLastXChosen();
		break;
	case SCI_HOMEDISPLAYEXTEND:
		MovePositionTo(MovePositionSoVisible(
		                   StartEndDisplayLine(currentPos, true), -1), selStream);
		SetLastXChosen();
		break;
	case SCI_LINEENDDISPLAY:
		MovePositionTo(MovePositionSoVisible(
		                   StartEndDisplayLine(currentPos, false), 1));
		SetLastXChosen();
		break;
	case SCI_LINEENDDISPLAYEXTEND:
		MovePositionTo(MovePositionSoVisible(
		                   StartEndDisplayLine(currentPos, false), 1), selStream);
		SetLastXChosen();
		break;
	}
	return 0;
}

int Editor::KeyDefault(int, int) {
	return 0;
}

int Editor::KeyDown(int key, bool shift, bool ctrl, bool alt, bool *consumed) {
	DwellEnd(false);
	int modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) |
	                (alt ? SCI_ALT : 0);
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

void Editor::SetWhitespaceVisible(int view) {
	vs.viewWhitespace = static_cast<WhiteSpaceVisibility>(view);
}

int Editor::GetWhitespaceVisible() {
	return vs.viewWhitespace;
}

void Editor::Indent(bool forwards) {
	//Platform::DebugPrintf("INdent %d\n", forwards);
	int lineOfAnchor = pdoc->LineFromPosition(anchor);
	int lineCurrentPos = pdoc->LineFromPosition(currentPos);
	if (lineOfAnchor == lineCurrentPos) {
		if (forwards) {
			pdoc->BeginUndoAction();
			ClearSelection();
			if (pdoc->GetColumn(currentPos) <= pdoc->GetColumn(pdoc->GetLineIndentPosition(lineCurrentPos)) &&
			        pdoc->tabIndents) {
				int indentation = pdoc->GetLineIndentation(lineCurrentPos);
				int indentationStep = pdoc->IndentSize();
				pdoc->SetLineIndentation(lineCurrentPos, indentation + indentationStep - indentation % indentationStep);
				SetEmptySelection(pdoc->GetLineIndentPosition(lineCurrentPos));
			} else {
				if (pdoc->useTabs) {
					pdoc->InsertChar(currentPos, '\t');
					SetEmptySelection(currentPos + 1);
				} else {
					int numSpaces = (pdoc->tabInChars) -
					                (pdoc->GetColumn(currentPos) % (pdoc->tabInChars));
					if (numSpaces < 1)
						numSpaces = pdoc->tabInChars;
					for (int i = 0; i < numSpaces; i++) {
						pdoc->InsertChar(currentPos + i, ' ');
					}
					SetEmptySelection(currentPos + numSpaces);
				}
			}
			pdoc->EndUndoAction();
		} else {
			if (pdoc->GetColumn(currentPos) <= pdoc->GetLineIndentation(lineCurrentPos) &&
			        pdoc->tabIndents) {
				pdoc->BeginUndoAction();
				int indentation = pdoc->GetLineIndentation(lineCurrentPos);
				int indentationStep = pdoc->IndentSize();
				pdoc->SetLineIndentation(lineCurrentPos, indentation - indentationStep);
				SetEmptySelection(pdoc->GetLineIndentPosition(lineCurrentPos));
				pdoc->EndUndoAction();
			} else {
				int newColumn = ((pdoc->GetColumn(currentPos) - 1) / pdoc->tabInChars) *
				                pdoc->tabInChars;
				if (newColumn < 0)
					newColumn = 0;
				int newPos = currentPos;
				while (pdoc->GetColumn(newPos) > newColumn)
					newPos--;
				SetEmptySelection(newPos);
			}
		}
	} else {
		int anchorPosOnLine = anchor - pdoc->LineStart(lineOfAnchor);
		int currentPosPosOnLine = currentPos - pdoc->LineStart(lineCurrentPos);
		// Multiple lines selected so indent / dedent
		int lineTopSel = Platform::Minimum(lineOfAnchor, lineCurrentPos);
		int lineBottomSel = Platform::Maximum(lineOfAnchor, lineCurrentPos);
		if (pdoc->LineStart(lineBottomSel) == anchor || pdoc->LineStart(lineBottomSel) == currentPos)
			lineBottomSel--;  	// If not selecting any characters on a line, do not indent
		pdoc->BeginUndoAction();
		pdoc->Indent(forwards, lineBottomSel, lineTopSel);
		pdoc->EndUndoAction();
		if (lineOfAnchor < lineCurrentPos) {
			if (currentPosPosOnLine == 0)
				SetSelection(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor));
			else
				SetSelection(pdoc->LineStart(lineCurrentPos + 1), pdoc->LineStart(lineOfAnchor));
		} else {
			if (anchorPosOnLine == 0)
				SetSelection(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor));
			else
				SetSelection(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor + 1));
		}
	}
}

/**
 * Search of a text in the document, in the given range.
 * @return The position of the found text, -1 if not found.
 */
long Editor::FindText(
	uptr_t wParam,		///< Search modes : @c SCFIND_MATCHCASE, @c SCFIND_WHOLEWORD,
						///< @c SCFIND_WORDSTART, @c SCFIND_REGEXP or @c SCFIND_POSIX.
	sptr_t lParam) {	///< @c TextToFind structure: The text to search for in the given range.

	TextToFind *ft = reinterpret_cast<TextToFind *>(lParam);
	int lengthFound = istrlen(ft->lpstrText);
	int pos = pdoc->FindText(ft->chrg.cpMin, ft->chrg.cpMax, ft->lpstrText,
	                         (wParam & SCFIND_MATCHCASE) != 0,
	                         (wParam & SCFIND_WHOLEWORD) != 0,
	                         (wParam & SCFIND_WORDSTART) != 0,
	                         (wParam & SCFIND_REGEXP) != 0,
	                         (wParam & SCFIND_POSIX) != 0,
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
	searchAnchor = SelectionStart();
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
	if (iMessage == SCI_SEARCHNEXT) {
		pos = pdoc->FindText(searchAnchor, pdoc->Length(), txt,
		                     (wParam & SCFIND_MATCHCASE) != 0,
		                     (wParam & SCFIND_WHOLEWORD) != 0,
		                     (wParam & SCFIND_WORDSTART) != 0,
		                     (wParam & SCFIND_REGEXP) != 0,
		                     (wParam & SCFIND_POSIX) != 0,
		                     &lengthFound);
	} else {
		pos = pdoc->FindText(searchAnchor, 0, txt,
		                     (wParam & SCFIND_MATCHCASE) != 0,
		                     (wParam & SCFIND_WHOLEWORD) != 0,
		                     (wParam & SCFIND_WORDSTART) != 0,
		                     (wParam & SCFIND_REGEXP) != 0,
		                     (wParam & SCFIND_POSIX) != 0,
		                     &lengthFound);
	}

	if (pos != -1) {
		SetSelection(pos, pos + lengthFound);
	}

	return pos;
}

/**
 * Search for text in the target range of the document.
 * @return The position of the found text, -1 if not found.
 */
long Editor::SearchInTarget(const char *text, int length) {
	int lengthFound = length;
	int pos = pdoc->FindText(targetStart, targetEnd, text,
	                         (searchFlags & SCFIND_MATCHCASE) != 0,
	                         (searchFlags & SCFIND_WHOLEWORD) != 0,
	                         (searchFlags & SCFIND_WORDSTART) != 0,
	                         (searchFlags & SCFIND_REGEXP) != 0,
	                         (searchFlags & SCFIND_POSIX) != 0,
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

char *Editor::CopyRange(int start, int end) {
	char *text = 0;
	if (start < end) {
		int len = end - start;
		text = new char[len + 1];
		if (text) {
			for (int i = 0; i < len; i++) {
				text[i] = pdoc->CharAt(start + i);
			}
			text[len] = '\0';
		}
	}
	return text;
}

void Editor::CopySelectionFromRange(SelectionText *ss, int start, int end) {
	ss->Set(CopyRange(start, end), end - start + 1,
		pdoc->dbcsCodePage, vs.styles[STYLE_DEFAULT].characterSet, false);
}

void Editor::CopySelectionRange(SelectionText *ss) {
	if (selType == selStream) {
		CopySelectionFromRange(ss, SelectionStart(), SelectionEnd());
	} else {
		char *text = 0;
		int size = 0;
		SelectionLineIterator lineIterator(this);
		while (lineIterator.Iterate()) {
			size += lineIterator.endPos - lineIterator.startPos;
			if (selType != selLines) {
				size++;
				if (pdoc->eolMode == SC_EOL_CRLF) {
					size++;
				}
			}
		}
		if (size > 0) {
			text = new char[size + 1];
			if (text) {
				int j = 0;
				lineIterator.Reset();
				while (lineIterator.Iterate()) {
					for (int i = lineIterator.startPos;
						 i < lineIterator.endPos;
						 i++) {
						text[j++] = pdoc->CharAt(i);
					}
					if (selType != selLines) {
						if (pdoc->eolMode != SC_EOL_LF) {
							text[j++] = '\r';
						}
						if (pdoc->eolMode != SC_EOL_CR) {
							text[j++] = '\n';
						}
					}
				}
				text[size] = '\0';
			}
		}
		ss->Set(text, size + 1, pdoc->dbcsCodePage,
			vs.styles[STYLE_DEFAULT].characterSet, selType == selRectangle);
	}
}

void Editor::CopyRangeToClipboard(int start, int end) {
	start = pdoc->ClampPositionIntoDocument(start);
	end = pdoc->ClampPositionIntoDocument(end);
	SelectionText selectedText;
	selectedText.Set(CopyRange(start, end), end - start + 1,
		pdoc->dbcsCodePage, vs.styles[STYLE_DEFAULT].characterSet, false);
	CopyToClipboard(selectedText);
}

void Editor::CopyText(int length, const char *text) {
	SelectionText selectedText;
	selectedText.Copy(text, length + 1,
		pdoc->dbcsCodePage, vs.styles[STYLE_DEFAULT].characterSet, false);
	CopyToClipboard(selectedText);
}

void Editor::SetDragPosition(int newPos) {
	if (newPos >= 0) {
		newPos = MovePositionOutsideChar(newPos, 1);
		posDrop = newPos;
	}
	if (posDrag != newPos) {
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

void Editor::StartDrag() {
	// Always handled by subclasses
	//SetMouseCapture(true);
	//DisplayCursor(Window::cursorArrow);
}

void Editor::DropAt(int position, const char *value, bool moving, bool rectangular) {
	//Platform::DebugPrintf("DropAt %d\n", inDragDrop);
	if (inDragDrop)
		dropWentOutside = false;

	int positionWasInSelection = PositionInSelection(position);

	bool positionOnEdgeOfSelection =
	    (position == SelectionStart()) || (position == SelectionEnd());

	if ((!inDragDrop) || !(0 == positionWasInSelection) ||
	        (positionOnEdgeOfSelection && !moving)) {

		int selStart = SelectionStart();
		int selEnd = SelectionEnd();

		pdoc->BeginUndoAction();

		int positionAfterDeletion = position;
		if (inDragDrop && moving) {
			// Remove dragged out text
			if (rectangular || selType == selLines) {
				SelectionLineIterator lineIterator(this);
				while (lineIterator.Iterate()) {
					if (position >= lineIterator.startPos) {
						if (position > lineIterator.endPos) {
							positionAfterDeletion -= lineIterator.endPos - lineIterator.startPos;
						} else {
							positionAfterDeletion -= position - lineIterator.startPos;
						}
					}
				}
			} else {
				if (position > selStart) {
					positionAfterDeletion -= selEnd - selStart;
				}
			}
			ClearSelection();
		}
		position = positionAfterDeletion;

		if (rectangular) {
			PasteRectangular(position, value, istrlen(value));
			pdoc->EndUndoAction();
			// Should try to select new rectangle but it may not be a rectangle now so just select the drop position
			SetEmptySelection(position);
		} else {
			position = MovePositionOutsideChar(position, currentPos - position);
			if (pdoc->InsertCString(position, value)) {
				SetSelection(position + istrlen(value), position);
			}
			pdoc->EndUndoAction();
		}
	} else if (inDragDrop) {
		SetEmptySelection(position);
	}
}

/**
 * @return -1 if given position is before the selection,
 *          1 if position is after the selection,
 *          0 if position is inside the selection,
 */
int Editor::PositionInSelection(int pos) {
	pos = MovePositionOutsideChar(pos, currentPos - pos);
	if (pos < SelectionStart()) {
		return -1;
	}
	if (pos > SelectionEnd()) {
		return 1;
	}
	if (selType == selStream) {
		return 0;
	} else {
		SelectionLineIterator lineIterator(this);
		lineIterator.SetAt(pdoc->LineFromPosition(pos));
		if (pos < lineIterator.startPos) {
			return -1;
		} else if (pos > lineIterator.endPos) {
			return 1;
		} else {
			return 0;
		}
	}
}

bool Editor::PointInSelection(Point pt) {
	int pos = PositionFromLocation(pt);
	if (0 == PositionInSelection(pos)) {
		// Probably inside, but we must make a finer test
		int selStart, selEnd;
		if (selType == selStream) {
			selStart = SelectionStart();
			selEnd = SelectionEnd();
		} else {
			SelectionLineIterator lineIterator(this);
			lineIterator.SetAt(pdoc->LineFromPosition(pos));
			selStart = lineIterator.startPos;
			selEnd = lineIterator.endPos;
		}
		if (pos == selStart) {
			// see if just before selection
			Point locStart = LocationFromPosition(pos);
			if (pt.x < locStart.x) {
				return false;
			}
		}
		if (pos == selEnd) {
			// see if just after selection
			Point locEnd = LocationFromPosition(pos);
			if (pt.x > locEnd.x) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool Editor::PointInSelMargin(Point pt) {
	// Really means: "Point in a margin"
	if (vs.fixedColumnWidth > 0) {	// There is a margin
		PRectangle rcSelMargin = GetClientRectangle();
		rcSelMargin.right = vs.fixedColumnWidth - vs.leftMarginWidth;
		return rcSelMargin.Contains(pt);
	} else {
		return false;
	}
}

void Editor::LineSelection(int lineCurrent_, int lineAnchor_) {
	if (lineAnchor_ < lineCurrent_) {
		SetSelection(pdoc->LineStart(lineCurrent_ + 1),
		             pdoc->LineStart(lineAnchor_));
	} else if (lineAnchor_ > lineCurrent_) {
		SetSelection(pdoc->LineStart(lineCurrent_),
		             pdoc->LineStart(lineAnchor_ + 1));
	} else { // Same line, select it
		SetSelection(pdoc->LineStart(lineAnchor_ + 1),
		             pdoc->LineStart(lineAnchor_));
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

void Editor::ButtonDown(Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) {
	//Platform::DebugPrintf("Scintilla:ButtonDown %d %d = %d alt=%d\n", curTime, lastClickTime, curTime - lastClickTime, alt);
	ptMouseLast = pt;
	int newPos = PositionFromLocation(pt);
	newPos = MovePositionOutsideChar(newPos, currentPos - newPos);
	inDragDrop = false;
	moveExtendsSelection = false;

	bool processed = NotifyMarginClick(pt, shift, ctrl, alt);
	if (processed)
		return;

	bool inSelMargin = PointInSelMargin(pt);
	if (shift & !inSelMargin) {
		SetSelection(newPos);
	}
	if (((curTime - lastClickTime) < Platform::DoubleClickTime()) && Close(pt, lastClick)) {
		//Platform::DebugPrintf("Double click %d %d = %d\n", curTime, lastClickTime, curTime - lastClickTime);
		SetMouseCapture(true);
		SetEmptySelection(newPos);
		bool doubleClick = false;
		// Stop mouse button bounce changing selection type
		if (!Platform::MouseButtonBounce() || curTime != lastClickTime) {
			if (selectionType == selChar) {
				selectionType = selWord;
				doubleClick = true;
			} else if (selectionType == selWord) {
				selectionType = selLine;
			} else {
				selectionType = selChar;
				originalAnchorPos = currentPos;
			}
		}

		if (selectionType == selWord) {
			if (currentPos >= originalAnchorPos) {	// Moved forward
				SetSelection(pdoc->ExtendWordSelect(currentPos, 1),
				             pdoc->ExtendWordSelect(originalAnchorPos, -1));
			} else {	// Moved backward
				SetSelection(pdoc->ExtendWordSelect(currentPos, -1),
				             pdoc->ExtendWordSelect(originalAnchorPos, 1));
			}
		} else if (selectionType == selLine) {
			lineAnchor = LineFromLocation(pt);
			SetSelection(pdoc->LineStart(lineAnchor + 1), pdoc->LineStart(lineAnchor));
			//Platform::DebugPrintf("Triple click: %d - %d\n", anchor, currentPos);
		} else {
			SetEmptySelection(currentPos);
		}
		//Platform::DebugPrintf("Double click: %d - %d\n", anchor, currentPos);
		if (doubleClick) {
			NotifyDoubleClick(pt, shift, ctrl, alt);
			if (PositionIsHotspot(newPos))
				NotifyHotSpotDoubleClicked(newPos, shift, ctrl, alt);
		}
	} else {	// Single click
		if (inSelMargin) {
			selType = selStream;
			if (ctrl) {
				SelectAll();
				lastClickTime = curTime;
				return;
			}
			if (!shift) {
				lineAnchor = LineFromLocation(pt);
				// Single click in margin: select whole line
				LineSelection(lineAnchor, lineAnchor);
				SetSelection(pdoc->LineStart(lineAnchor + 1),
				             pdoc->LineStart(lineAnchor));
			} else {
				// Single shift+click in margin: select from line anchor to clicked line
				if (anchor > currentPos)
					lineAnchor = pdoc->LineFromPosition(anchor - 1);
				else
					lineAnchor = pdoc->LineFromPosition(anchor);
				int lineStart = LineFromLocation(pt);
				LineSelection(lineStart, lineAnchor);
				//lineAnchor = lineStart; // Keep the same anchor for ButtonMove
			}

			SetDragPosition(invalidPosition);
			SetMouseCapture(true);
			selectionType = selLine;
		} else {
			if (PointIsHotspot(pt)) {
				NotifyHotSpotClicked(newPos, shift, ctrl, alt);
			}
			if (!shift) {
				inDragDrop = PointInSelection(pt) && !SelectionEmpty();
			}
			if (inDragDrop) {
				SetMouseCapture(false);
				SetDragPosition(newPos);
				CopySelectionRange(&drag);
				StartDrag();
			} else {
				SetDragPosition(invalidPosition);
				SetMouseCapture(true);
				if (!shift) {
					SetEmptySelection(newPos);
				}
				selType = alt ? selRectangle : selStream;
				selectionType = selChar;
				originalAnchorPos = currentPos;
				SetRectangularRange();
			}
		}
	}
	lastClickTime = curTime;
	lastXChosen = pt.x;
	ShowCaretAtCurrentPosition();
}

bool Editor::PositionIsHotspot(int position) {
	return vs.styles[pdoc->StyleAt(position) & pdoc->stylingBitsMask].hotspot;
}

bool Editor::PointIsHotspot(Point pt) {
	int pos = PositionFromLocationClose(pt);
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

void Editor::GetHotSpotRange(int& hsStart_, int& hsEnd_) {
	hsStart_ = hsStart;
	hsEnd_ = hsEnd;
}

void Editor::ButtonMove(Point pt) {
	if ((ptMouseLast.x != pt.x) || (ptMouseLast.y != pt.y)) {
		DwellEnd(true);
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
		int movePos = PositionFromLocation(pt);
		movePos = MovePositionOutsideChar(movePos, currentPos - movePos);
		if (posDrag >= 0) {
			SetDragPosition(movePos);
		} else {
			if (selectionType == selChar) {
				SetSelection(movePos);
			} else if (selectionType == selWord) {
				// Continue selecting by word
				if (movePos == originalAnchorPos) {	// Didn't move
					// No need to do anything. Previously this case was lumped
					// in with "Moved forward", but that can be harmful in this
					// case: a handler for the NotifyDoubleClick re-adjusts
					// the selection for a fancier definition of "word" (for
					// example, in Perl it is useful to include the leading
					// '$', '%' or '@' on variables for word selection). In this
					// the ButtonMove() called via Tick() for auto-scrolling
					// could result in the fancier word selection adjustment
					// being unmade.
				} else if (movePos > originalAnchorPos) {	// Moved forward
					SetSelection(pdoc->ExtendWordSelect(movePos, 1),
					             pdoc->ExtendWordSelect(originalAnchorPos, -1));
				} else {	// Moved backward
					SetSelection(pdoc->ExtendWordSelect(movePos, -1),
					             pdoc->ExtendWordSelect(originalAnchorPos, 1));
				}
			} else {
				// Continue selecting by line
				int lineMove = LineFromLocation(pt);
				LineSelection(lineMove, lineAnchor);
			}
		}
		// While dragging to make rectangular selection, we don't want the current
		// position to jump to the end of smaller or empty lines.
		//xEndSelect = pt.x - vs.fixedColumnWidth + xOffset;
		xEndSelect = XFromPosition(movePos);

		// Autoscroll
		PRectangle rcClient = GetClientRectangle();
		if (pt.y > rcClient.bottom) {
			int lineMove = cs.DisplayFromDoc(LineFromLocation(pt));
			if (lineMove < 0) {
				lineMove = cs.DisplayFromDoc(pdoc->LinesTotal() - 1);
			}
			ScrollTo(lineMove - LinesOnScreen() + 5);
			Redraw();
		} else if (pt.y < rcClient.top) {
			int lineMove = cs.DisplayFromDoc(LineFromLocation(pt));
			ScrollTo(lineMove - 5);
			Redraw();
		}
		EnsureCaretVisible(false, false, true);

		if (hsStart != -1 && !PositionIsHotspot(movePos))
			SetHotSpotRange(NULL);

	} else {
		if (vs.fixedColumnWidth > 0) {	// There is a margin
			if (PointInSelMargin(pt)) {
				DisplayCursor(Window::cursorReverseArrow);
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
	//Platform::DebugPrintf("ButtonUp %d\n", HaveMouseCapture());
	if (HaveMouseCapture()) {
		if (PointInSelMargin(pt)) {
			DisplayCursor(Window::cursorReverseArrow);
		} else {
			DisplayCursor(Window::cursorText);
			SetHotSpotRange(NULL);
		}
		ptMouseLast = pt;
		SetMouseCapture(false);
		int newPos = PositionFromLocation(pt);
		newPos = MovePositionOutsideChar(newPos, currentPos - newPos);
		if (inDragDrop) {
			int selStart = SelectionStart();
			int selEnd = SelectionEnd();
			if (selStart < selEnd) {
				if (drag.len) {
					if (ctrl) {
						if (pdoc->InsertString(newPos, drag.s, drag.len)) {
							SetSelection(newPos, newPos + drag.len);
						}
					} else if (newPos < selStart) {
						pdoc->DeleteChars(selStart, drag.len);
						if (pdoc->InsertString(newPos, drag.s, drag.len)) {
							SetSelection(newPos, newPos + drag.len);
						}
					} else if (newPos > selEnd) {
						pdoc->DeleteChars(selStart, drag.len);
						newPos -= drag.len;
						if (pdoc->InsertString(newPos, drag.s, drag.len)) {
							SetSelection(newPos, newPos + drag.len);
						}
					} else {
						SetEmptySelection(newPos);
					}
					drag.Free();
				}
				selectionType = selChar;
			}
		} else {
			if (selectionType == selChar) {
				SetSelection(newPos);
			}
		}
		SetRectangularRange();
		lastClickTime = curTime;
		lastClick = pt;
		lastXChosen = pt.x;
		if (selType == selStream) {
			SetLastXChosen();
		}
		inDragDrop = false;
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
	if ((dwellDelay < SC_TIME_FOREVER) &&
	        (ticksToDwell > 0) &&
	        (!HaveMouseCapture())) {
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
		WrapLines(false, -1);
		// No more wrapping
		if (wrapStart == wrapEnd)
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

bool Editor::PaintContains(PRectangle rc) {
	return rcPaint.Contains(rc);
}

bool Editor::PaintContainsMargin() {
	PRectangle rcSelMargin = GetClientRectangle();
	rcSelMargin.right = vs.fixedColumnWidth;
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
	selType = selStream;
	currentPos = 0;
	anchor = 0;
	targetStart = 0;
	targetEnd = 0;

	braces[0] = invalidPosition;
	braces[1] = invalidPosition;

	// Reset the contraction state to fully shown.
	cs.Clear();
	cs.InsertLines(0, pdoc->LinesTotal() - 1);
	llc.Deallocate();
	NeedWrapping();

	pdoc->AddWatcher(this, 0);
	SetScrollBars();
	Redraw();
}

/**
 * Recursively expand a fold, making lines visible except where they have an unexpanded parent.
 */
void Editor::Expand(int &line, bool doExpand) {
	int lineMaxSubord = pdoc->GetLastChild(line);
	line++;
	while (line <= lineMaxSubord) {
		if (doExpand)
			cs.SetVisible(line, line, true);
		int level = pdoc->GetLevel(line);
		if (level & SC_FOLDLEVELHEADERFLAG) {
			if (doExpand && cs.GetExpanded(line)) {
				Expand(line, true);
			} else {
				Expand(line, false);
			}
		} else {
			line++;
		}
	}
}

void Editor::ToggleContraction(int line) {
	if (line >= 0) {
		if ((pdoc->GetLevel(line) & SC_FOLDLEVELHEADERFLAG) == 0) {
			line = pdoc->GetFoldParent(line);
			if (line < 0)
				return;
		}

		if (cs.GetExpanded(line)) {
			int lineMaxSubord = pdoc->GetLastChild(line);
			cs.SetExpanded(line, 0);
			if (lineMaxSubord > line) {
				cs.SetVisible(line + 1, lineMaxSubord, false);

				int lineCurrent = pdoc->LineFromPosition(currentPos);
				if (lineCurrent > line && lineCurrent <= lineMaxSubord) {
					// This does not re-expand the fold
					EnsureCaretVisible();
				}

				SetScrollBars();
				Redraw();
			}

		} else {
			if (!(cs.GetVisible(line))) {
				EnsureLineVisible(line, false);
				GoToLine(line);
			}
			cs.SetExpanded(line, 1);
			Expand(line, true);
			SetScrollBars();
			Redraw();
		}
	}
}

/**
 * Recurse up from this line to find any folds that prevent this line from being visible
 * and unfold them all.
 */
void Editor::EnsureLineVisible(int lineDoc, bool enforcePolicy) {

	// In case in need of wrapping to ensure DisplayFromDoc works.
	WrapLines(true, -1);

	if (!cs.GetVisible(lineDoc)) {
		int lineParent = pdoc->GetFoldParent(lineDoc);
		if (lineParent >= 0) {
			if (lineDoc != lineParent)
				EnsureLineVisible(lineParent, enforcePolicy);
			if (!cs.GetExpanded(lineParent)) {
				cs.SetExpanded(lineParent, 1);
				Expand(lineParent, true);
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

int Editor::ReplaceTarget(bool replacePatterns, const char *text, int length) {
	pdoc->BeginUndoAction();
	if (length == -1)
		length = istrlen(text);
	if (replacePatterns) {
		text = pdoc->SubstituteByPosition(text, &length);
		if (!text)
			return 0;
	}
	if (targetStart != targetEnd)
		pdoc->DeleteChars(targetStart, targetEnd - targetStart);
	targetEnd = targetStart;
	pdoc->InsertString(targetStart, text, length);
	targetEnd = targetStart + length;
	pdoc->EndUndoAction();
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
	size_t textLength = appendLength / 2;
	char *text = new char[textLength];
	if (text) {
		size_t i;
		for (i=0;i<textLength;i++) {
			text[i] = buffer[i*2];
		}
		pdoc->InsertString(CurrentPosition(), text, textLength);
		for (i=0;i<textLength;i++) {
			text[i] = buffer[i*2+1];
		}
		pdoc->StartStyling(CurrentPosition(), static_cast<char>(0xff));
		pdoc->SetStyles(textLength, text);
		delete []text;
	}
	SetEmptySelection(currentPos + textLength);
}

static bool ValidMargin(unsigned long wParam) {
	return wParam < ViewStyle::margins;
}

static char *CharPtrFromSPtr(sptr_t lParam) {
	return reinterpret_cast<char *>(lParam);
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
			pdoc->BeginUndoAction();
			pdoc->DeleteChars(0, pdoc->Length());
			SetEmptySelection(0);
			pdoc->InsertCString(0, CharPtrFromSPtr(lParam));
			pdoc->EndUndoAction();
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

	case SCI_COPYRANGE:
		CopyRangeToClipboard(wParam, lParam);
		break;

	case SCI_COPYTEXT:
		CopyText(wParam, CharPtrFromSPtr(lParam));
		break;

	case SCI_PASTE:
		Paste();
		if (!caretSticky) {
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
			selType = selStream;
			SetSelection(nEnd, nStart);
			EnsureCaretVisible();
		}
		break;

	case SCI_GETSELTEXT: {
			if (lParam == 0) {
				if (selType == selStream) {
					return 1 + SelectionEnd() - SelectionStart();
				} else {
					// TODO: why is selLines handled the slow way?
					int size = 0;
					int extraCharsPerLine = 0;
					if (selType != selLines)
						extraCharsPerLine = (pdoc->eolMode == SC_EOL_CRLF) ? 2 : 1;
					SelectionLineIterator lineIterator(this);
					while (lineIterator.Iterate()) {
						size += lineIterator.endPos + extraCharsPerLine - lineIterator.startPos;
					}

					return 1 + size;
				}
			}
			SelectionText selectedText;
			CopySelectionRange(&selectedText);
			char *ptr = CharPtrFromSPtr(lParam);
			int iChar = 0;
			if (selectedText.len) {
				for (; iChar < selectedText.len; iChar++)
					ptr[iChar] = selectedText.s[iChar];
			} else {
				ptr[0] = '\0';
			}
			return iChar;
		}

	case SCI_LINEFROMPOSITION:
		if (static_cast<int>(wParam) < 0)
			return 0;
		return pdoc->LineFromPosition(wParam);

	case SCI_POSITIONFROMLINE:
		if (static_cast<int>(wParam) < 0)
			wParam = pdoc->LineFromPosition(SelectionStart());
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
			pdoc->BeginUndoAction();
			ClearSelection();
			char *replacement = CharPtrFromSPtr(lParam);
			pdoc->InsertCString(currentPos, replacement);
			pdoc->EndUndoAction();
			SetEmptySelection(currentPos + istrlen(replacement));
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
		if (currentPos < anchor) {
			targetStart = currentPos;
			targetEnd = anchor;
		} else {
			targetStart = anchor;
			targetEnd = currentPos;
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

	case SCI_POSITIONBEFORE:
		return pdoc->MovePositionOutsideChar(wParam-1, -1, true);

	case SCI_POSITIONAFTER:
		return pdoc->MovePositionOutsideChar(wParam+1, 1, true);

	case SCI_LINESCROLL:
		ScrollTo(topLine + lParam);
		HorizontalScrollTo(xOffset + wParam * vs.spaceWidth);
		return 1;

	case SCI_SETXOFFSET:
		xOffset = wParam;
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
			return pt.x;
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
			TextRange *tr = reinterpret_cast<TextRange *>(lParam);
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
		return FormatRange(wParam != 0, reinterpret_cast<RangeToFormat *>(lParam));

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
			SetEmptySelection(currentPos + wParam);
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

	case SCI_SETWORDCHARS: {
			pdoc->SetDefaultCharClasses(false);
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(reinterpret_cast<unsigned char *>(lParam), CharClassify::ccWord);
		}
		break;

	case SCI_SETWHITESPACECHARS: {
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(reinterpret_cast<unsigned char *>(lParam), CharClassify::ccSpace);
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
		SetSelection(wParam, anchor);
		break;

	case SCI_GETCURRENTPOS:
		return currentPos;

	case SCI_SETANCHOR:
		SetSelection(currentPos, wParam);
		break;

	case SCI_GETANCHOR:
		return anchor;

	case SCI_SETSELECTIONSTART:
		SetSelection(Platform::Maximum(currentPos, wParam), wParam);
		break;

	case SCI_GETSELECTIONSTART:
		return Platform::Minimum(anchor, currentPos);

	case SCI_SETSELECTIONEND:
		SetSelection(wParam, Platform::Minimum(anchor, wParam));
		break;

	case SCI_GETSELECTIONEND:
		return Platform::Maximum(anchor, currentPos);

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
			TextRange *tr = reinterpret_cast<TextRange *>(lParam);
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

	case SCI_POSITIONFROMPOINT:
		return PositionFromLocation(Point(wParam, lParam));

	case SCI_POSITIONFROMPOINTCLOSE:
		return PositionFromLocationClose(Point(wParam, lParam));

	case SCI_GOTOLINE:
		GoToLine(wParam);
		break;

	case SCI_GOTOPOS:
		SetEmptySelection(wParam);
		EnsureCaretVisible();
		Redraw();
		break;

	case SCI_GETCURLINE: {
			int lineCurrentPos = pdoc->LineFromPosition(currentPos);
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
			return currentPos - lineStart;
		}

	case SCI_GETENDSTYLED:
		return pdoc->GetEndStyled();

	case SCI_GETEOLMODE:
		return pdoc->eolMode;

	case SCI_SETEOLMODE:
		pdoc->eolMode = wParam;
		break;

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
		InvalidateStyleRedraw();
		ReconfigureScrollBars();
		break;

	case SCI_GETWRAPMODE:
		return wrapState;

	case SCI_SETWRAPVISUALFLAGS:
		wrapVisualFlags = wParam;
		actualWrapVisualStartIndent = wrapVisualStartIndent;
		if ((wrapVisualFlags & SC_WRAPVISUALFLAG_START) && (actualWrapVisualStartIndent == 0))
			actualWrapVisualStartIndent = 1; // must indent to show start visual
		InvalidateStyleRedraw();
		ReconfigureScrollBars();
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
		wrapVisualStartIndent = wParam;
		actualWrapVisualStartIndent = wrapVisualStartIndent;
		if ((wrapVisualFlags & SC_WRAPVISUALFLAG_START) && (actualWrapVisualStartIndent == 0))
			actualWrapVisualStartIndent = 1; // must indent to show start visual
		InvalidateStyleRedraw();
		ReconfigureScrollBars();
		break;

	case SCI_GETWRAPSTARTINDENT:
		return wrapVisualStartIndent;

	case SCI_SETLAYOUTCACHE:
		llc.SetLevel(wParam);
		break;

	case SCI_GETLAYOUTCACHE:
		return llc.GetLevel();

	case SCI_SETSCROLLWIDTH:
		PLATFORM_ASSERT(wParam > 0);
		if ((wParam > 0) && (wParam != static_cast<unsigned int >(scrollWidth))) {
			scrollWidth = wParam;
			SetScrollBars();
		}
		break;

	case SCI_GETSCROLLWIDTH:
		return scrollWidth;

	case SCI_LINESJOIN:
		LinesJoin();
		break;

	case SCI_LINESSPLIT:
		LinesSplit(wParam);
		break;

	case SCI_TEXTWIDTH:
		PLATFORM_ASSERT(wParam <= STYLE_MAX);
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
		PLATFORM_ASSERT((wParam == 0) || (wParam == 1));
		if (caretSticky != (wParam != 0)) {
			caretSticky = wParam != 0;
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
		}
		break;

	case SCI_GETVSCROLLBAR:
		return verticalScrollBarVisible;

	case SCI_SETINDENTATIONGUIDES:
		vs.viewIndentationGuides = wParam != 0;
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
			pdoc->dbcsCodePage = wParam;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETCODEPAGE:
		return pdoc->dbcsCodePage;

	case SCI_SETUSEPALETTE:
		palette.allowRealization = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETUSEPALETTE:
		return palette.allowRealization;

		// Marker definition and setting
	case SCI_MARKERDEFINE:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].markType = lParam;
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case SCI_MARKERSETFORE:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].fore.desired = ColourDesired(lParam);
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case SCI_MARKERSETBACK:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].back.desired = ColourDesired(lParam);
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

	case SCI_MARKERNEXT: {
			int lt = pdoc->LinesTotal();
			for (int iLine = wParam; iLine < lt; iLine++) {
				if ((pdoc->GetMark(iLine) & lParam) != 0)
					return iLine;
			}
		}
		return -1;

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

	case SCI_STYLECLEARALL:
		vs.ClearStyles();
		InvalidateStyleRedraw();
		break;

	case SCI_STYLESETFORE:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].fore.desired = ColourDesired(lParam);
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETBACK:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].back.desired = ColourDesired(lParam);
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETBOLD:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].bold = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETITALIC:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].italic = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETEOLFILLED:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].eolFilled = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETSIZE:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].size = lParam;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETFONT:
		if (lParam == 0)
			return 0;
		if (wParam <= STYLE_MAX) {
			vs.SetStyleFontName(wParam, CharPtrFromSPtr(lParam));
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETUNDERLINE:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].underline = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETCASE:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].caseForce = static_cast<Style::ecaseForced>(lParam);
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETCHARACTERSET:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].characterSet = lParam;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETVISIBLE:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].visible = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETCHANGEABLE:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].changeable = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLESETHOTSPOT:
		if (wParam <= STYLE_MAX) {
			vs.styles[wParam].hotspot = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;
	case SCI_STYLEGETFORE:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].fore.desired.AsLong();
		else
			return 0; 
	case SCI_STYLEGETBACK:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].back.desired.AsLong();
		else
			return 0;
	case SCI_STYLEGETBOLD:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].bold ? 1 : 0;
		else
			return 0;
	case SCI_STYLEGETITALIC:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].italic ? 1 : 0;
		else
			return 0;
	case SCI_STYLEGETEOLFILLED:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].eolFilled ? 1 : 0;
		else
			return 0;
	case SCI_STYLEGETSIZE:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].size;
		else
			return 0;
	case SCI_STYLEGETFONT:
		if (lParam == 0)
			return strlen(vs.styles[wParam].fontName);

		if (wParam <= STYLE_MAX)
			strcpy(CharPtrFromSPtr(lParam), vs.styles[wParam].fontName);
		break;
	case SCI_STYLEGETUNDERLINE:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].underline ? 1 : 0;
		else
			return 0;
	case SCI_STYLEGETCASE:
		if (wParam <= STYLE_MAX)
			return static_cast<int>(vs.styles[wParam].caseForce);
		else
			return 0;
	case SCI_STYLEGETCHARACTERSET:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].characterSet;
		else
			return 0;
	case SCI_STYLEGETVISIBLE:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].visible ? 1 : 0;
		else
			return 0;
	case SCI_STYLEGETCHANGEABLE:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].changeable ? 1 : 0;
		else
			return 0;
	case SCI_STYLEGETHOTSPOT:
		if (wParam <= STYLE_MAX)
			return vs.styles[wParam].hotspot ? 1 : 0;
		else
			return 0;
	case SCI_STYLERESETDEFAULT:
		vs.ResetDefaultStyle();
		InvalidateStyleRedraw();
		break;
	case SCI_SETSTYLEBITS:
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
	case SCI_GETCARETLINEBACK:
		return vs.caretLineBackground.desired.AsLong();
	case SCI_SETCARETLINEBACK:
		vs.caretLineBackground.desired = wParam;
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
		cs.SetVisible(wParam, lParam, false);
		SetScrollBars();
		Redraw();
		break;

	case SCI_GETLINEVISIBLE:
		return cs.GetVisible(wParam);

	case SCI_SETFOLDEXPANDED:
		if (cs.SetExpanded(wParam, lParam != 0)) {
			RedrawSelMargin();
		}
		break;

	case SCI_GETFOLDEXPANDED:
		return cs.GetExpanded(wParam);

	case SCI_SETFOLDFLAGS:
		foldFlags = wParam;
		Redraw();
		break;

	case SCI_TOGGLEFOLD:
		ToggleContraction(wParam);
		break;

	case SCI_ENSUREVISIBLE:
		EnsureLineVisible(wParam, false);
		break;

	case SCI_ENSUREVISIBLEENFORCEPOLICY:
		EnsureLineVisible(wParam, true);
		break;

	case SCI_SEARCHANCHOR:
		SearchAnchor();
		break;

	case SCI_SEARCHNEXT:
	case SCI_SEARCHPREV:
		return SearchText(iMessage, wParam, lParam);

#ifdef INCLUDE_DEPRECATED_FEATURES
	case SCI_SETCARETPOLICY:  	// Deprecated
		caretXPolicy = caretYPolicy = wParam;
		caretXSlop = caretYSlop = lParam;
		break;
#endif

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
		vs.selforeground.desired = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETSELBACK:
		vs.selbackset = wParam != 0;
		vs.selbackground.desired = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETSELALPHA:
		vs.selAlpha = wParam;
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
		vs.whitespaceForeground.desired = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETWHITESPACEBACK:
		vs.whitespaceBackgroundSet = wParam != 0;
		vs.whitespaceBackground.desired = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETCARETFORE:
		vs.caretcolour.desired = ColourDesired(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETFORE:
		return vs.caretcolour.desired.AsLong();

	case SCI_SETCARETWIDTH:
		if (wParam <= 0)
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
			vs.indicators[wParam].fore.desired = ColourDesired(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETFORE:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].fore.desired.AsLong() : 0;

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
	case SCI_ZOOMIN:
	case SCI_ZOOMOUT:
	case SCI_DELWORDLEFT:
	case SCI_DELWORDRIGHT:
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

	case SCI_BRACEBADLIGHT:
		SetBraceHighlight(static_cast<int>(wParam), -1, STYLE_BRACEBAD);
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
		return vs.edgecolour.desired.AsLong();

	case SCI_SETEDGECOLOUR:
		vs.edgecolour.desired = ColourDesired(wParam);
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
			if (doc) {
				doc->AddRef();
			}
			return reinterpret_cast<sptr_t>(doc);
		}

	case SCI_ADDREFDOCUMENT:
		(reinterpret_cast<Document *>(lParam))->AddRef();
		break;

	case SCI_RELEASEDOCUMENT:
		(reinterpret_cast<Document *>(lParam))->Release();
		break;

	case SCI_SETMODEVENTMASK:
		modEventMask = wParam;
		return 0;

	case SCI_GETMODEVENTMASK:
		return modEventMask;

	case SCI_CONVERTEOLS:
		pdoc->ConvertLineEnds(wParam);
		SetSelection(currentPos, anchor);	// Ensure selection inside document
		return 0;

	case SCI_SETLENGTHFORENCODE:
		lengthForEncode = wParam;
		return 0;

	case SCI_SELECTIONISRECTANGLE:
		return selType == selRectangle ? 1 : 0;

	case SCI_SETSELECTIONMODE: {
			switch (wParam) {
			case SC_SEL_STREAM:
				moveExtendsSelection = !moveExtendsSelection || (selType != selStream);
				selType = selStream;
				break;
			case SC_SEL_RECTANGLE:
				moveExtendsSelection = !moveExtendsSelection || (selType != selRectangle);
				selType = selRectangle;
				break;
			case SC_SEL_LINES:
				moveExtendsSelection = !moveExtendsSelection || (selType != selLines);
				selType = selLines;
				break;
			default:
				moveExtendsSelection = !moveExtendsSelection || (selType != selStream);
				selType = selStream;
			}
			InvalidateSelection(currentPos, anchor);
		}
	case SCI_GETSELECTIONMODE:
		switch (selType) {
		case selStream:
			return SC_SEL_STREAM;
		case selRectangle:
			return SC_SEL_RECTANGLE;
		case selLines:
			return SC_SEL_LINES;
		default:	// ?!
			return SC_SEL_STREAM;
		}
	case SCI_GETLINESELSTARTPOSITION: {
			SelectionLineIterator lineIterator(this);
			lineIterator.SetAt(wParam);
			return lineIterator.startPos;
		}
	case SCI_GETLINESELENDPOSITION: {
			SelectionLineIterator lineIterator(this);
			lineIterator.SetAt(wParam);
			return lineIterator.endPos;
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
		vs.foldmarginColour.desired = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETFOLDMARGINHICOLOUR:
		vs.foldmarginHighlightColourSet = wParam != 0;
		vs.foldmarginHighlightColour.desired = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETHOTSPOTACTIVEFORE:
		vs.hotspotForegroundSet = wParam != 0;
		vs.hotspotForeground.desired = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTACTIVEFORE:
		return vs.hotspotForeground.desired.AsLong();

	case SCI_SETHOTSPOTACTIVEBACK:
		vs.hotspotBackgroundSet = wParam != 0;
		vs.hotspotBackground.desired = ColourDesired(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTACTIVEBACK:
		return vs.hotspotBackground.desired.AsLong();

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

	default:
		return DefWndProc(iMessage, wParam, lParam);
	}
	//Platform::DebugPrintf("end wnd proc\n");
	return 0l;
}
