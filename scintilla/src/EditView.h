// Scintilla source code edit control
/** @file EditView.h
 ** Defines the appearance of the main text area of the editor window.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITVIEW_H
#define EDITVIEW_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

struct PrintParameters {
	int magnification;
	int colourMode;
	WrapMode wrapState;
	PrintParameters();
};

/**
* The view may be drawn in separate phases.
*/
enum DrawPhase {
	drawBack = 0x1,
	drawIndicatorsBack = 0x2,
	drawText = 0x4,
	drawIndentationGuides = 0x8,
	drawIndicatorsFore = 0x10,
	drawSelectionTranslucent = 0x20,
	drawLineTranslucent = 0x40,
	drawFoldLines = 0x80,
	drawCarets = 0x100,
	drawAll = 0x1FF
};

bool ValidStyledText(const ViewStyle &vs, size_t styleOffset, const StyledText &st);
int WidestLineWidth(Surface *surface, const ViewStyle &vs, int styleOffset, const StyledText &st);
void DrawTextNoClipPhase(Surface *surface, PRectangle rc, const Style &style, XYPOSITION ybase,
	const char *s, int len, DrawPhase phase);
void DrawStyledText(Surface *surface, const ViewStyle &vs, int styleOffset, PRectangle rcText,
	const StyledText &st, size_t start, size_t length, DrawPhase phase);

typedef void (*DrawTabArrowFn)(Surface *surface, PRectangle rcTab, int ymid);

/**
* EditView draws the main text area.
*/
class EditView {
public:
	PrintParameters printParameters;
	PerLine *ldTabstops;
	int tabWidthMinimumPixels;

	bool hideSelection;
	bool drawOverstrikeCaret;

	/** In bufferedDraw mode, graphics operations are drawn to a pixmap and then copied to
	* the screen. This avoids flashing but is about 30% slower. */
	bool bufferedDraw;
	/** In phasesTwo mode, drawing is performed in two phases, first the background
	* and then the foreground. This avoids chopping off characters that overlap the next run.
	* In multiPhaseDraw mode, drawing is performed in multiple phases with each phase drawing
	* one feature over the whole drawing area, instead of within one line. This allows text to
	* overlap from one line to the next. */
	enum PhasesDraw { phasesOne, phasesTwo, phasesMultiple };
	PhasesDraw phasesDraw;

	int lineWidthMaxSeen;

	bool additionalCaretsBlink;
	bool additionalCaretsVisible;

	bool imeCaretBlockOverride;

	Surface *pixmapLine;
	Surface *pixmapIndentGuide;
	Surface *pixmapIndentGuideHighlight;

	LineLayoutCache llc;
	PositionCache posCache;

	int tabArrowHeight; // draw arrow heads this many pixels above/below line midpoint
	/** Some platforms, notably PLAT_CURSES, do not support Scintilla's native
	 * DrawTabArrow function for drawing tab characters. Allow those platforms to
	 * override it instead of creating a new method in the Surface class that
	 * existing platforms must implement as empty. */
	DrawTabArrowFn customDrawTabArrow;
	DrawWrapMarkerFn customDrawWrapMarker;

	EditView();
	virtual ~EditView();

	bool SetTwoPhaseDraw(bool twoPhaseDraw);
	bool SetPhasesDraw(int phases);
	bool LinesOverlap() const;

	void ClearAllTabstops();
	XYPOSITION NextTabstopPos(int line, XYPOSITION x, XYPOSITION tabWidth) const;
	bool ClearTabstops(int line);
	bool AddTabstop(int line, int x);
	int GetNextTabstop(int line, int x) const;
	void LinesAddedOrRemoved(int lineOfPos, int linesAdded);

	void DropGraphics(bool freeObjects);
	void AllocateGraphics(const ViewStyle &vsDraw);
	void RefreshPixMaps(Surface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw);

	LineLayout *RetrieveLineLayout(int lineNumber, const EditModel &model);
	void LayoutLine(const EditModel &model, int line, Surface *surface, const ViewStyle &vstyle,
		LineLayout *ll, int width = LineLayout::wrapWidthInfinite);

	Point LocationFromPosition(Surface *surface, const EditModel &model, SelectionPosition pos, int topLine, const ViewStyle &vs);
	SelectionPosition SPositionFromLocation(Surface *surface, const EditModel &model, Point pt, bool canReturnInvalid,
		bool charPosition, bool virtualSpace, const ViewStyle &vs);
	SelectionPosition SPositionFromLineX(Surface *surface, const EditModel &model, int lineDoc, int x, const ViewStyle &vs);
	int DisplayFromPosition(Surface *surface, const EditModel &model, int pos, const ViewStyle &vs);
	int StartEndDisplayLine(Surface *surface, const EditModel &model, int pos, bool start, const ViewStyle &vs);

	void DrawIndentGuide(Surface *surface, int lineVisible, int lineHeight, int start, PRectangle rcSegment, bool highlight);
	void DrawEOL(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
		int line, int lineEnd, int xStart, int subLine, XYACCUMULATOR subLineStart,
		ColourOptional background);
	void DrawAnnotation(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		int line, int xStart, PRectangle rcLine, int subLine, DrawPhase phase);
	void DrawCarets(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int line,
		int xStart, PRectangle rcLine, int subLine) const;
	void DrawBackground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
		Range lineRange, int posLineStart, int xStart,
		int subLine, ColourOptional background) const;
	void DrawForeground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int lineVisible,
		PRectangle rcLine, Range lineRange, int posLineStart, int xStart,
		int subLine, ColourOptional background);
	void DrawIndentGuidesOverEmpty(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		int line, int lineVisible, PRectangle rcLine, int xStart, int subLine);
	void DrawLine(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int line,
		int lineVisible, int xStart, PRectangle rcLine, int subLine, DrawPhase phase);
	void PaintText(Surface *surfaceWindow, const EditModel &model, PRectangle rcArea, PRectangle rcClient,
		const ViewStyle &vsDraw);
	long FormatRange(bool draw, Sci_RangeToFormat *pfr, Surface *surface, Surface *surfaceMeasure,
		const EditModel &model, const ViewStyle &vs);
};

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

#ifdef SCI_NAMESPACE
}
#endif

#endif
