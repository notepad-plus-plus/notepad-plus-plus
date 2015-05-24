// Scintilla source code edit control
/** @file XPM.h
 ** Define a classes to hold image data in the X Pixmap (XPM) and RGBA formats.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef XPM_H
#define XPM_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 * Hold a pixmap in XPM format.
 */
class XPM {
	int height;
	int width;
	int nColours;
	std::vector<unsigned char> pixels;
	ColourDesired colourCodeTable[256];
	char codeTransparent;
	ColourDesired ColourFromCode(int ch) const;
	void FillRun(Surface *surface, int code, int startX, int y, int x);
public:
	XPM(const char *textForm);
	XPM(const char *const *linesForm);
	~XPM();
	void Init(const char *textForm);
	void Init(const char *const *linesForm);
	void Clear();
	/// Decompose image into runs and use FillRectangle for each run
	void Draw(Surface *surface, PRectangle &rc);
	int GetHeight() const { return height; }
	int GetWidth() const { return width; }
	void PixelAt(int x, int y, ColourDesired &colour, bool &transparent) const;
private:
	static std::vector<const char *>LinesFormFromTextForm(const char *textForm);
};

/**
 * A translucent image stored as a sequence of RGBA bytes.
 */
class RGBAImage {
	// Private so RGBAImage objects can not be copied
	RGBAImage(const RGBAImage &);
	RGBAImage &operator=(const RGBAImage &);
	int height;
	int width;
	float scale;
	std::vector<unsigned char> pixelBytes;
public:
	RGBAImage(int width_, int height_, float scale_, const unsigned char *pixels_);
	RGBAImage(const XPM &xpm);
	virtual ~RGBAImage();
	int GetHeight() const { return height; }
	int GetWidth() const { return width; }
	float GetScale() const { return scale; }
	float GetScaledHeight() const { return height / scale; }
	float GetScaledWidth() const { return width / scale; }
	int CountBytes() const;
	const unsigned char *Pixels() const;
	void SetPixel(int x, int y, ColourDesired colour, int alpha=0xff); 
};

/**
 * A collection of RGBAImage pixmaps indexed by integer id.
 */
class RGBAImageSet {
	typedef std::map<int, RGBAImage*> ImageMap;
	ImageMap images;
	mutable int height;	///< Memorize largest height of the set.
	mutable int width;	///< Memorize largest width of the set.
public:
	RGBAImageSet();
	~RGBAImageSet();
	/// Remove all images.
	void Clear();
	/// Add an image.
	void Add(int ident, RGBAImage *image);
	/// Get image by id.
	RGBAImage *Get(int ident);
	/// Give the largest height of the set.
	int GetHeight() const;
	/// Give the largest width of the set.
	int GetWidth() const;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
