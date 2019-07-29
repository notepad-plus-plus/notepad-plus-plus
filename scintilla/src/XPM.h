// Scintilla source code edit control
/** @file XPM.h
 ** Define a classes to hold image data in the X Pixmap (XPM) and RGBA formats.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef XPM_H
#define XPM_H

namespace Scintilla {

/**
 * Hold a pixmap in XPM format.
 */
class XPM {
	int height=1;
	int width=1;
	int nColours=1;
	std::vector<unsigned char> pixels;
	ColourDesired colourCodeTable[256];
	char codeTransparent=' ';
	ColourDesired ColourFromCode(int ch) const;
	void FillRun(Surface *surface, int code, int startX, int y, int x) const;
public:
	explicit XPM(const char *textForm);
	explicit XPM(const char *const *linesForm);
	XPM(const XPM &) = default;
	XPM(XPM &&) noexcept = default;
	XPM &operator=(const XPM &) = default;
	XPM &operator=(XPM &&) noexcept = default;
	~XPM();
	void Init(const char *textForm);
	void Init(const char *const *linesForm);
	/// Decompose image into runs and use FillRectangle for each run
	void Draw(Surface *surface, const PRectangle &rc);
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
	int height;
	int width;
	float scale;
	std::vector<unsigned char> pixelBytes;
public:
	RGBAImage(int width_, int height_, float scale_, const unsigned char *pixels_);
	explicit RGBAImage(const XPM &xpm);
	RGBAImage(const RGBAImage &) = default;
	RGBAImage(RGBAImage &&) noexcept = default;
	RGBAImage &operator=(const RGBAImage &) = default;
	RGBAImage &operator=(RGBAImage &&) noexcept = default;
	virtual ~RGBAImage();
	int GetHeight() const { return height; }
	int GetWidth() const { return width; }
	float GetScale() const { return scale; }
	float GetScaledHeight() const { return height / scale; }
	float GetScaledWidth() const { return width / scale; }
	int CountBytes() const;
	const unsigned char *Pixels() const;
	void SetPixel(int x, int y, ColourDesired colour, int alpha);
};

/**
 * A collection of RGBAImage pixmaps indexed by integer id.
 */
class RGBAImageSet {
	typedef std::map<int, std::unique_ptr<RGBAImage>> ImageMap;
	ImageMap images;
	mutable int height;	///< Memorize largest height of the set.
	mutable int width;	///< Memorize largest width of the set.
public:
	RGBAImageSet();
	// Deleted so RGBAImageSet objects can not be copied.
	RGBAImageSet(const RGBAImageSet &) = delete;
	RGBAImageSet(RGBAImageSet &&) = delete;
	RGBAImageSet &operator=(const RGBAImageSet &) = delete;
	RGBAImageSet &operator=(RGBAImageSet &&) = delete;
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

}

#endif
