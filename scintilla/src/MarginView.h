// Scintilla source code edit control
/** @file MarginView.h
 ** Defines the appearance of the editor margin.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef MARGINVIEW_H
#define MARGINVIEW_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

void DrawWrapMarker(Surface *surface, PRectangle rcPlace, bool isEndMarker, ColourDesired wrapColour);

typedef void (*DrawWrapMarkerFn)(Surface *surface, PRectangle rcPlace, bool isEndMarker, ColourDesired wrapColour);

/**
* MarginView draws the margins.
*/
class MarginView {
public:
	Surface *pixmapSelMargin;
	Surface *pixmapSelPattern;
	Surface *pixmapSelPatternOffset1;
	// Highlight current folding block
	HighlightDelimiter highlightDelimiter;

	int wrapMarkerPaddingRight; // right-most pixel padding of wrap markers
	/** Some platforms, notably PLAT_CURSES, do not support Scintilla's native
	 * DrawWrapMarker function for drawing wrap markers. Allow those platforms to
	 * override it instead of creating a new method in the Surface class that
	 * existing platforms must implement as empty. */
	DrawWrapMarkerFn customDrawWrapMarker;

	MarginView();

	void DropGraphics(bool freeObjects);
	void AllocateGraphics(const ViewStyle &vsDraw);
	void RefreshPixMaps(Surface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw);
	void PaintMargin(Surface *surface, int topLine, PRectangle rc, PRectangle rcMargin,
		const EditModel &model, const ViewStyle &vs);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
