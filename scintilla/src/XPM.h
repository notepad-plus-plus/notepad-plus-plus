// Scintilla source code edit control
/** @file XPM.h
 ** Define a class that holds data in the X Pixmap (XPM) format.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef XPM_H
#define XPM_H

/**
 * Hold a pixmap in XPM format.
 */
class XPM {
	int id;		// Assigned by container
	int height;
	int width;
	int nColours;
	char *data;
	char codeTransparent;
	char *codes;
	ColourPair *colours;
	ColourAllocated ColourFromCode(int ch);
	void FillRun(Surface *surface, int code, int startX, int y, int x);
	char **lines;
	ColourPair *colourCodeTable[256];
public:
	XPM(const char *textForm);
	XPM(const char * const *linesForm);
	~XPM();
	void Init(const char *textForm);
	void Init(const char * const *linesForm);
	void Clear();
	/// Similar to same named method in ViewStyle:
	void RefreshColourPalette(Palette &pal, bool want);
	/// No palette used, so just copy the desired colours to the allocated colours
	void CopyDesiredColours();
	/// Decompose image into runs and use FillRectangle for each run
	void Draw(Surface *surface, PRectangle &rc);
	char **InLinesForm() { return lines; }
	void SetId(int id_) { id = id_; }
	int GetId() { return id; }
	int GetHeight() { return height; }
	int GetWidth() { return width; }
	static const char **LinesFormFromTextForm(const char *textForm);
};

/**
 * A collection of pixmaps indexed by integer id.
 */
class XPMSet {
	XPM **set;	///< The stored XPMs.
	int len;	///< Current number of XPMs.
	int maximum;	///< Current maximum number of XPMs, increased by steps if reached.
	int height;	///< Memorize largest height of the set.
	int width;	///< Memorize largest width of the set.
public:
	XPMSet();
	~XPMSet();
	/// Remove all XPMs.
	void Clear();
	/// Add a XPM.
	void Add(int id, const char *textForm);
	/// Get XPM by id.
	XPM *Get(int id);
	/// Give the largest height of the set.
	int GetHeight();
	/// Give the largest width of the set.
	int GetWidth();
};

#endif
