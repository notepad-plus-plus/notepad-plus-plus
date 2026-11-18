
/**
 * Copyright 2009 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 * @file PlatCocoa.h
 */

#ifndef PLATCOCOA_H
#define PLATCOCOA_H

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

#include <Cocoa/Cocoa.h>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "DictionaryForCF.h"
#include "QuartzTextLayout.h"

NSRect PRectangleToNSRect(const Scintilla::Internal::PRectangle &rc);
Scintilla::Internal::PRectangle NSRectToPRectangle(const NSRect &rc);
CFStringEncoding EncodingFromCharacterSet(bool unicode, Scintilla::CharacterSet characterSet);

@interface ScintillaContextMenu : NSMenu {
	Scintilla::Internal::ScintillaCocoa *owner;
}
- (void) handleCommand: (NSMenuItem *) sender;
- (void) setOwner: (Scintilla::Internal::ScintillaCocoa *) newOwner;

@end

namespace Scintilla::Internal {

// A class to do the actual text rendering for us using Quartz 2D.
class SurfaceImpl : public Surface {
private:
	SurfaceMode mode;

	CGContextRef gc;

	/** If the surface is a bitmap context, contains a reference to the bitmap data. */
	std::unique_ptr<uint8_t[]> bitmapData;
	/** If the surface is a bitmap context, stores the dimensions of the bitmap. */
	int bitmapWidth;
	int bitmapHeight;

	/** Set the CGContext's fill colour to the specified desired colour. */
	void FillColour(ColourRGBA fill);

	void PenColourAlpha(ColourRGBA fore);

	void SetFillStroke(FillStroke fillStroke);

	// 24-bit RGB+A bitmap data constants
	static const int BITS_PER_COMPONENT = 8;
	static const int BITS_PER_PIXEL = BITS_PER_COMPONENT * 4;
	static const int BYTES_PER_PIXEL = BITS_PER_PIXEL / 8;

	bool UnicodeMode() const noexcept;
	void Clear();

public:
	SurfaceImpl();
	SurfaceImpl(const SurfaceImpl *surface, int width, int height);
	~SurfaceImpl() override;

	void Init(WindowID wid) override;
	void Init(SurfaceID sid, WindowID wid) override;
	std::unique_ptr<Surface> AllocatePixMap(int width, int height) override;
	std::unique_ptr<SurfaceImpl> AllocatePixMapImplementation(int width, int height);
	CGContextRef GetContext() { return gc; }

	void SetMode(SurfaceMode mode) override;

	void Release() noexcept override;
	int SupportsFeature(Scintilla::Supports feature) noexcept override;
	bool Initialised() override;

	/** Returns a CGImageRef that represents the surface. Returns NULL if this is not possible. */
	CGImageRef CreateImage();
	void CopyImageRectangle(SurfaceImpl *source, PRectangle srcRect, PRectangle dstRect);

	int LogPixelsY() override;
	int PixelDivisions() override;
	int DeviceHeightFont(int points) override;
	void LineDraw(Point start, Point end, Stroke stroke) override;
	void PolyLine(const Point *pts, size_t npts, Stroke stroke) override;
	void Polygon(const Scintilla::Internal::Point *pts, size_t npts, FillStroke fillStroke) override;
	void RectangleDraw(PRectangle rc, FillStroke fillStroke) override;
	void RectangleFrame(PRectangle rc, Stroke stroke) override;
	void FillRectangle(PRectangle rc, Fill fill) override;
	void FillRectangleAligned(PRectangle rc, Fill fill) override;
	void FillRectangle(PRectangle rc, Surface &surfacePattern) override;
	void RoundedRectangle(PRectangle rc, FillStroke fillStroke) override;
	void AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) override;
	void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) override;
	void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) override;
	void Ellipse(PRectangle rc, FillStroke fillStroke) override;
	void Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) override;
	void Copy(PRectangle rc, Scintilla::Internal::Point from, Surface &surfaceSource) override;
	std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) override;

	void DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore,
			    ColourRGBA back) override;
	void DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore,
			     ColourRGBA back) override;
	void DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) override;
	void MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthText(const Font *font_, std::string_view text) override;

	void DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore,
			    ColourRGBA back) override;
	void DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore,
			     ColourRGBA back) override;
	void DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) override;
	void MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthTextUTF8(const Font *font_, std::string_view text) override;

	XYPOSITION Ascent(const Font *font_) override;
	XYPOSITION Descent(const Font *font_) override;
	XYPOSITION InternalLeading(const Font *font_) override;
	XYPOSITION Height(const Font *font_) override;
	XYPOSITION AverageCharWidth(const Font *font_) override;

	void SetClip(PRectangle rc) override;
	void PopClip() override;
	void FlushCachedState() override;
	void FlushDrawing() override;

}; // SurfaceImpl class

} // Scintilla namespace

#endif
