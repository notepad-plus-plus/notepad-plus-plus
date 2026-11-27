// Scintilla source code edit control
/** @file XPM.h
 ** Define a classes to hold image data in the X Pixmap (XPM) and RGBA formats.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef XPM_H
#define XPM_H

namespace Scintilla::Internal {

/**
 * Hold a pixmap in XPM format.
 */
class XPM {
	static constexpr size_t colourTableSize = 256;
	int height=1;
	int width=1;
	int nColours=1;
	std::vector<unsigned char> pixels;
	ColourRGBA colourCodeTable[colourTableSize];
	char codeTransparent=' ';
	[[nodiscard]] ColourRGBA ColourFromCode(int ch) const noexcept;
	void FillRun(Surface *surface, int code, int startX, int y, int x) const;
public:
	explicit XPM(const char *textForm);
	explicit XPM(const char *const *linesForm);
	void Init(const char *textForm);
	void Init(const char *const *linesForm);
	/// Decompose image into runs and use FillRectangle for each run
	void Draw(Surface *surface, const PRectangle &rc);
	[[nodiscard]] int GetHeight() const noexcept { return height; }
	[[nodiscard]] int GetWidth() const noexcept { return width; }
	[[nodiscard]] ColourRGBA PixelAt(int x, int y) const noexcept;
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
	static constexpr size_t bytesPerPixel = 4;
	RGBAImage(int width_, int height_, float scale_, const unsigned char *pixels_);
	explicit RGBAImage(const XPM &xpm);
	[[nodiscard]] int GetHeight() const noexcept { return height; }
	[[nodiscard]] int GetWidth() const noexcept { return width; }
	[[nodiscard]] float GetScale() const noexcept { return scale; }
	[[nodiscard]] float GetScaledHeight() const noexcept;
	[[nodiscard]] float GetScaledWidth() const noexcept;
	[[nodiscard]] int CountBytes() const noexcept;
	[[nodiscard]] const unsigned char *Pixels() const noexcept;
	void SetPixel(int x, int y, ColourRGBA colour) noexcept;
	static void BGRAFromRGBA(unsigned char *pixelsBGRA, const unsigned char *pixelsRGBA, size_t count) noexcept;
};

/**
 * A collection of RGBAImage pixmaps indexed by integer id.
 */
class RGBAImageSet {
	using ImageMap = std::map<int, std::unique_ptr<RGBAImage>>;
	ImageMap images;
	mutable int height = -1;	///< Memorize largest height of the set.
	mutable int width = -1;	///< Memorize largest width of the set.
public:
	RGBAImageSet();
	/// Remove all images.
	void Clear() noexcept;
	/// Add an image.
	void AddImage(int ident, std::unique_ptr<RGBAImage> image);
	/// Get image by id.
	RGBAImage *Get(int ident);
	/// Give the largest height of the set.
	[[nodiscard]] int GetHeight() const noexcept;
	/// Give the largest width of the set.
	[[nodiscard]] int GetWidth() const noexcept;
};

}

#endif
