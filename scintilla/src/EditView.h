// Scintilla source code edit control
/** @file EditView.h
 ** Defines the appearance of the main text area of the editor window.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITVIEW_H
#define EDITVIEW_H

namespace Scintilla {

struct PrintParameters {
	int magnification;
	int colourMode;
	WrapMode wrapState;
	PrintParameters() noexcept;
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
	std::string_view text, DrawPhase phase);
void DrawStyledText(Surface *surface, const ViewStyle &vs, int styleOffset, PRectangle rcText,
	const StyledText &st, size_t start, size_t length, DrawPhase phase);

typedef void (*DrawTabArrowFn)(Surface *surface, PRectangle rcTab, int ymid);

class LineTabstops;

/**
* EditView draws the main text area.
*/
class EditView {
public:
	PrintParameters printParameters;
	std::unique_ptr<LineTabstops> ldTabstops;
	int tabWidthMinimumPixels;

	bool hideSelection;
	bool drawOverstrikeCaret; // used by the curses platform

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

	std::unique_ptr<Surface> pixmapLine;
	std::unique_ptr<Surface> pixmapIndentGuide;
	std::unique_ptr<Surface> pixmapIndentGuideHighlight;

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
	// Deleted so EditView objects can not be copied.
	EditView(const EditView &) = delete;
	EditView(EditView &&) = delete;
	void operator=(const EditView &) = delete;
	void operator=(EditView &&) = delete;
	virtual ~EditView();

	bool SetTwoPhaseDraw(bool twoPhaseDraw) noexcept;
	bool SetPhasesDraw(int phases) noexcept;
	bool LinesOverlap() const noexcept;

	void ClearAllTabstops() noexcept;
	XYPOSITION NextTabstopPos(Sci::Line line, XYPOSITION x, XYPOSITION tabWidth) const;
	bool ClearTabstops(Sci::Line line);
	bool AddTabstop(Sci::Line line, int x);
	int GetNextTabstop(Sci::Line line, int x) const;
	void LinesAddedOrRemoved(Sci::Line lineOfPos, Sci::Line linesAdded);

	void DropGraphics(bool freeObjects);
	void AllocateGraphics(const ViewStyle &vsDraw);
	void RefreshPixMaps(Surface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw);

	LineLayout *RetrieveLineLayout(Sci::Line lineNumber, const EditModel &model);
	void LayoutLine(const EditModel &model, Sci::Line line, Surface *surface, const ViewStyle &vstyle,
		LineLayout *ll, int width = LineLayout::wrapWidthInfinite);

	static void UpdateBidiData(const EditModel &model, const ViewStyle &vstyle, LineLayout *ll);

	Point LocationFromPosition(Surface *surface, const EditModel &model, SelectionPosition pos, Sci::Line topLine,
		const ViewStyle &vs, PointEnd pe, const PRectangle rcClient);
	Range RangeDisplayLine(Surface *surface, const EditModel &model, Sci::Line lineVisible, const ViewStyle &vs);
	SelectionPosition SPositionFromLocation(Surface *surface, const EditModel &model, PointDocument pt, bool canReturnInvalid,
		bool charPosition, bool virtualSpace, const ViewStyle &vs, const PRectangle rcClient);
	SelectionPosition SPositionFromLineX(Surface *surface, const EditModel &model, Sci::Line lineDoc, int x, const ViewStyle &vs);
	Sci::Line DisplayFromPosition(Surface *surface, const EditModel &model, Sci::Position pos, const ViewStyle &vs);
	Sci::Position StartEndDisplayLine(Surface *surface, const EditModel &model, Sci::Position pos, bool start, const ViewStyle &vs);

	void DrawIndentGuide(Surface *surface, Sci::Line lineVisible, int lineHeight, XYPOSITION start, PRectangle rcSegment, bool highlight);
	void DrawEOL(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
		Sci::Line line, Sci::Position lineEnd, int xStart, int subLine, XYACCUMULATOR subLineStart,
		ColourOptional background);
	void DrawFoldDisplayText(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		Sci::Line line, int xStart, PRectangle rcLine, int subLine, XYACCUMULATOR subLineStart, DrawPhase phase);
	void DrawAnnotation(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		Sci::Line line, int xStart, PRectangle rcLine, int subLine, DrawPhase phase);
	void DrawCarets(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, Sci::Line lineDoc,
		int xStart, PRectangle rcLine, int subLine) const;
	void DrawBackground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
		Range lineRange, Sci::Position posLineStart, int xStart,
		int subLine, ColourOptional background) const;
	void DrawForeground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, Sci::Line lineVisible,
		PRectangle rcLine, Range lineRange, Sci::Position posLineStart, int xStart,
		int subLine, ColourOptional background);
	void DrawIndentGuidesOverEmpty(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		Sci::Line line, Sci::Line lineVisible, PRectangle rcLine, int xStart, int subLine);
	void DrawLine(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, Sci::Line line,
		Sci::Line lineVisible, int xStart, PRectangle rcLine, int subLine, DrawPhase phase);
	void PaintText(Surface *surfaceWindow, const EditModel &model, PRectangle rcArea, PRectangle rcClient,
		const ViewStyle &vsDraw);
	void FillLineRemainder(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		Sci::Line line, PRectangle rcArea, int subLine) const;
	Sci::Position FormatRange(bool draw, const Sci_RangeToFormat *pfr, Surface *surface, Surface *surfaceMeasure,
		const EditModel &model, const ViewStyle &vs);
};

/**
* Convenience class to ensure LineLayout objects are always disposed.
*/
class AutoLineLayout {
	LineLayoutCache &llc;
	LineLayout *ll;
public:
	AutoLineLayout(LineLayoutCache &llc_, LineLayout *ll_) noexcept : llc(llc_), ll(ll_) {}
	AutoLineLayout(const AutoLineLayout &) = delete;
	AutoLineLayout(AutoLineLayout &&) = delete;
	AutoLineLayout &operator=(const AutoLineLayout &) = delete;
	AutoLineLayout &operator=(AutoLineLayout &&) = delete;
	~AutoLineLayout() noexcept {
		llc.Dispose(ll);
		ll = nullptr;
	}
	LineLayout *operator->() const noexcept {
		return ll;
	}
	operator LineLayout *() const noexcept {
		return ll;
	}
	void Set(LineLayout *ll_) noexcept {
		llc.Dispose(ll);
		ll = ll_;
	}
};

}

#endif
