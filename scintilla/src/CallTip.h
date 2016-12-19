// Scintilla source code edit control
/** @file CallTip.h
 ** Interface to the call tip control.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CALLTIP_H
#define CALLTIP_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 */
class CallTip {
	int startHighlight;    // character offset to start and...
	int endHighlight;      // ...end of highlighted text
	std::string val;
	Font font;
	PRectangle rectUp;      // rectangle of last up angle in the tip
	PRectangle rectDown;    // rectangle of last down arrow in the tip
	int lineHeight;         // vertical line spacing
	int offsetMain;         // The alignment point of the call tip
	int tabSize;            // Tab size in pixels, <=0 no TAB expand
	bool useStyleCallTip;   // if true, STYLE_CALLTIP should be used
	bool above;		// if true, display calltip above text

	// Private so CallTip objects can not be copied
	CallTip(const CallTip &);
	CallTip &operator=(const CallTip &);
	void DrawChunk(Surface *surface, int &x, const char *s,
		int posStart, int posEnd, int ytext, PRectangle rcClient,
		bool highlight, bool draw);
	int PaintContents(Surface *surfaceWindow, bool draw);
	bool IsTabCharacter(char c) const;
	int NextTabPos(int x) const;

public:
	Window wCallTip;
	Window wDraw;
	bool inCallTipMode;
	int posStartCallTip;
	ColourDesired colourBG;
	ColourDesired colourUnSel;
	ColourDesired colourSel;
	ColourDesired colourShade;
	ColourDesired colourLight;
	int codePage;
	int clickPlace;

	int insetX; // text inset in x from calltip border
	int widthArrow;
	int borderHeight;
	int verticalOffset; // pixel offset up or down of the calltip with respect to the line

	CallTip();
	~CallTip();

	void PaintCT(Surface *surfaceWindow);

	void MouseClick(Point pt);

	/// Setup the calltip and return a rectangle of the area required.
	PRectangle CallTipStart(int pos, Point pt, int textHeight, const char *defn,
		const char *faceName, int size, int codePage_,
		int characterSet, int technology, Window &wParent);

	void CallTipCancel();

	/// Set a range of characters to be displayed in a highlight style.
	/// Commonly used to highlight the current parameter.
	void SetHighlight(int start, int end);

	/// Set the tab size in pixels for the call tip. 0 or -ve means no tab expand.
	void SetTabSize(int tabSz);

	/// Set calltip position.
	void SetPosition(bool aboveText);

	/// Used to determine which STYLE_xxxx to use for call tip information
	bool UseStyleCallTip() const { return useStyleCallTip;}

	// Modify foreground and background colours
	void SetForeBack(const ColourDesired &fore, const ColourDesired &back);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
