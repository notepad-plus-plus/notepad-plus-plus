// Scintilla source code edit control
/** @file CallTip.h
 ** Interface to the call tip control.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CALLTIP_H
#define CALLTIP_H

namespace Scintilla::Internal {

struct Chunk {
	size_t start;
	size_t end;
	constexpr Chunk(size_t start_=0, size_t end_=0) noexcept : start(start_), end(end_) {
		assert(start <= end);
	}
	size_t Length() const noexcept;
};

/**
 */
class CallTip {
	Chunk highlight;    // character offset to start and end of highlighted text
	std::string val;
	std::shared_ptr<Font> font;
	PRectangle rectUp;      // rectangle of last up angle in the tip
	PRectangle rectDown;    // rectangle of last down arrow in the tip
	int lineHeight;         // vertical line spacing
	int offsetMain;         // The alignment point of the call tip
	int tabSize;            // Tab size in pixels, <=0 no TAB expand
	bool useStyleCallTip;   // if true, StyleCallTip should be used
	bool above;		// if true, display calltip above text

	int DrawChunk(Surface *surface, int x, std::string_view sv,
		int ytext, PRectangle rcClient, bool asHighlight, bool draw);
	int PaintContents(Surface *surfaceWindow, bool draw);
	bool IsTabCharacter(char ch) const noexcept;
	int NextTabPos(int x) const noexcept;

public:
	Window wCallTip;
	Window wDraw;
	bool inCallTipMode;
	Sci::Position posStartCallTip;
	ColourRGBA colourBG;
	ColourRGBA colourUnSel;
	ColourRGBA colourSel;
	ColourRGBA colourShade;
	ColourRGBA colourLight;
	int codePage;
	int clickPlace;

	int insetX; // text inset in x from calltip border
	int widthArrow;
	int borderHeight;
	int verticalOffset; // pixel offset up or down of the calltip with respect to the line

	CallTip() noexcept;
	// Deleted so CallTip objects can not be copied.
	CallTip(const CallTip &) = delete;
	CallTip(CallTip &&) = delete;
	CallTip &operator=(const CallTip &) = delete;
	CallTip &operator=(CallTip &&) = delete;
	~CallTip();

	void PaintCT(Surface *surfaceWindow);

	void MouseClick(Point pt) noexcept;

	/// Setup the calltip and return a rectangle of the area required.
	PRectangle CallTipStart(Sci::Position pos, Point pt, int textHeight, const char *defn,
		int codePage_, Surface *surfaceMeasure, std::shared_ptr<Font> font_);

	void CallTipCancel() noexcept;

	/// Set a range of characters to be displayed in a highlight style.
	/// Commonly used to highlight the current parameter.
	void SetHighlight(size_t start, size_t end);

	/// Set the tab size in pixels for the call tip. 0 or -ve means no tab expand.
	void SetTabSize(int tabSz) noexcept;

	/// Set calltip position.
	void SetPosition(bool aboveText) noexcept;

	/// Used to determine which STYLE_xxxx to use for call tip information
	bool UseStyleCallTip() const noexcept;

	// Modify foreground and background colours
	void SetForeBack(ColourRGBA fore, ColourRGBA back) noexcept;
};

}

#endif
