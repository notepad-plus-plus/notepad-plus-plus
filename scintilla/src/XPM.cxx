// Scintilla source code edit control
/** @file XPM.cxx
 ** Define a class that holds data in the X Pixmap (XPM) format.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>
#include <stdlib.h>

#include <vector>
#include <map>

#include "Platform.h"

#include "XPM.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static const char *NextField(const char *s) {
	// In case there are leading spaces in the string
	while (*s && *s == ' ') {
		s++;
	}
	while (*s && *s != ' ') {
		s++;
	}
	while (*s && *s == ' ') {
		s++;
	}
	return s;
}

// Data lines in XPM can be terminated either with NUL or "
static size_t MeasureLength(const char *s) {
	size_t i = 0;
	while (s[i] && (s[i] != '\"'))
		i++;
	return i;
}

ColourDesired XPM::ColourFromCode(int ch) const {
	return colourCodeTable[ch];
}

void XPM::FillRun(Surface *surface, int code, int startX, int y, int x) {
	if ((code != codeTransparent) && (startX != x)) {
		PRectangle rc(startX, y, x, y+1);
		surface->FillRectangle(rc, ColourFromCode(code));
	}
}

XPM::XPM(const char *textForm) {
	Init(textForm);
}

XPM::XPM(const char *const *linesForm) {
	Init(linesForm);
}

XPM::~XPM() {
	Clear();
}

void XPM::Init(const char *textForm) {
	Clear();
	// Test done is two parts to avoid possibility of overstepping the memory
	// if memcmp implemented strangely. Must be 4 bytes at least at destination.
	if ((0 == memcmp(textForm, "/* X", 4)) && (0 == memcmp(textForm, "/* XPM */", 9))) {
		// Build the lines form out of the text form
		std::vector<const char *> linesForm = LinesFormFromTextForm(textForm);
		if (!linesForm.empty()) {
			Init(&linesForm[0]);
		}
	} else {
		// It is really in line form
		Init(reinterpret_cast<const char * const *>(textForm));
	}
}

void XPM::Init(const char *const *linesForm) {
	Clear();
	height = 1;
	width = 1;
	nColours = 1;
	pixels.clear();
	codeTransparent = ' ';
	if (!linesForm)
		return;

	std::fill(colourCodeTable, colourCodeTable+256, 0);
	const char *line0 = linesForm[0];
	width = atoi(line0);
	line0 = NextField(line0);
	height = atoi(line0);
	pixels.resize(width*height);
	line0 = NextField(line0);
	nColours = atoi(line0);
	line0 = NextField(line0);
	if (atoi(line0) != 1) {
		// Only one char per pixel is supported
		return;
	}

	for (int c=0; c<nColours; c++) {
		const char *colourDef = linesForm[c+1];
		int code = static_cast<unsigned char>(colourDef[0]);
		colourDef += 4;
		ColourDesired colour(0xff, 0xff, 0xff);
		if (*colourDef == '#') {
			colour.Set(colourDef);
		} else {
			codeTransparent = code;
		}
		colourCodeTable[code] = colour;
	}

	for (int y=0; y<height; y++) {
		const char *lform = linesForm[y+nColours+1];
		size_t len = MeasureLength(lform);
		for (size_t x = 0; x<len; x++) 
			pixels[y * width + x] = static_cast<unsigned char>(lform[x]);
	}
}

void XPM::Clear() {
}

void XPM::Draw(Surface *surface, PRectangle &rc) {
	if (pixels.empty()) {
		return;
	}
	// Centre the pixmap
	int startY = rc.top + (rc.Height() - height) / 2;
	int startX = rc.left + (rc.Width() - width) / 2;
	for (int y=0; y<height; y++) {
		int prevCode = 0;
		int xStartRun = 0;
		for (int x=0; x<width; x++) {
			int code = pixels[y * width + x];
			if (code != prevCode) {
				FillRun(surface, prevCode, startX + xStartRun, startY + y, startX + x);
				xStartRun = x;
				prevCode = code;
			}
		}
		FillRun(surface, prevCode, startX + xStartRun, startY + y, startX + width);
	}
}

void XPM::PixelAt(int x, int y, ColourDesired &colour, bool &transparent) const {
	if (pixels.empty() || (x<0) || (x >= width) || (y<0) || (y >= height)) {
		colour = 0;
		transparent = true;
		return;
	}
	int code = pixels[y * width + x];
	transparent = code == codeTransparent;
	if (transparent) {
		colour = 0;
	} else {
		colour = ColourFromCode(code).AsLong();
	}
}

std::vector<const char *> XPM::LinesFormFromTextForm(const char *textForm) {
	// Build the lines form out of the text form
	std::vector<const char *> linesForm;
	int countQuotes = 0;
	int strings=1;
	int j=0;
	for (; countQuotes < (2*strings) && textForm[j] != '\0'; j++) {
		if (textForm[j] == '\"') {
			if (countQuotes == 0) {
				// First field: width, height, number of colors, chars per pixel
				const char *line0 = textForm + j + 1;
				// Skip width
				line0 = NextField(line0);
				// Add 1 line for each pixel of height
				strings += atoi(line0);
				line0 = NextField(line0);
				// Add 1 line for each colour
				strings += atoi(line0);
			}
			if (countQuotes / 2 >= strings) {
				break;	// Bad height or number of colors!
			}
			if ((countQuotes & 1) == 0) {
				linesForm.push_back(textForm + j + 1);
			}
			countQuotes++;
		}
	}
	if (textForm[j] == '\0' || countQuotes / 2 > strings) {
		// Malformed XPM! Height + number of colors too high or too low
		linesForm.clear();
	}
	return linesForm;
}

RGBAImage::RGBAImage(int width_, int height_, float scale_, const unsigned char *pixels_) :
	height(height_), width(width_), scale(scale_) {
	if (pixels_) {
		pixelBytes.assign(pixels_, pixels_ + CountBytes());
	} else {
		pixelBytes.resize(CountBytes());
	}
}

RGBAImage::RGBAImage(const XPM &xpm) {
	height = xpm.GetHeight();
	width = xpm.GetWidth();
	scale = 1;
	pixelBytes.resize(CountBytes());
	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			ColourDesired colour;
			bool transparent = false;
			xpm.PixelAt(x, y, colour, transparent);
			SetPixel(x, y, colour, transparent ? 0 : 255);
		}
	}
}

RGBAImage::~RGBAImage() {
}

int RGBAImage::CountBytes() const {
	return width * height * 4;
}

const unsigned char *RGBAImage::Pixels() const {
	return &pixelBytes[0];
}

void RGBAImage::SetPixel(int x, int y, ColourDesired colour, int alpha) {
	unsigned char *pixel = &pixelBytes[0] + (y*width+x) * 4;
	// RGBA
	pixel[0] = static_cast<unsigned char>(colour.GetRed());
	pixel[1] = static_cast<unsigned char>(colour.GetGreen());
	pixel[2] = static_cast<unsigned char>(colour.GetBlue());
	pixel[3] = static_cast<unsigned char>(alpha);
}

RGBAImageSet::RGBAImageSet() : height(-1), width(-1){
}

RGBAImageSet::~RGBAImageSet() {
	Clear();
}

/// Remove all images.
void RGBAImageSet::Clear() {
	for (ImageMap::iterator it=images.begin(); it != images.end(); ++it) {
		delete it->second;
		it->second = 0;
	}
	images.clear();
	height = -1;
	width = -1;
}

/// Add an image.
void RGBAImageSet::Add(int ident, RGBAImage *image) {
	ImageMap::iterator it=images.find(ident);
	if (it == images.end()) {
		images[ident] = image;
	} else {
		delete it->second;
		it->second = image;
	}
	height = -1;
	width = -1;
}

/// Get image by id.
RGBAImage *RGBAImageSet::Get(int ident) {
	ImageMap::iterator it = images.find(ident);
	if (it != images.end()) {
		return it->second;
	}
	return NULL;
}

/// Give the largest height of the set.
int RGBAImageSet::GetHeight() const {
	if (height < 0) {
		for (ImageMap::const_iterator it=images.begin(); it != images.end(); ++it) {
			if (height < it->second->GetHeight()) {
				height = it->second->GetHeight();
			}
		}
	}
	return (height > 0) ? height : 0;
}

/// Give the largest width of the set.
int RGBAImageSet::GetWidth() const {
	if (width < 0) {
		for (ImageMap::const_iterator it=images.begin(); it != images.end(); ++it) {
			if (width < it->second->GetWidth()) {
				width = it->second->GetWidth();
			}
		}
	}
	return (width > 0) ? width : 0;
}
