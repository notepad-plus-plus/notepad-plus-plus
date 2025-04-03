// Scintilla source code edit control
/** @file SurfaceGDI.cxx
 ** Implementation of drawing to GDI on Windows.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cmath>
#include <climits>

#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>

// Want to use std::min and std::max so don't want Windows.h version of min and max
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#undef WINVER
#define WINVER 0x0A00
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <windowsx.h>
#include <shellscalingapi.h>

#include "ScintillaTypes.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"
#include "XPM.h"
#include "UniConversion.h"
#include "DBCS.h"

#include "WinTypes.h"
#include "PlatWin.h"
#include "SurfaceGDI.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

// All file hidden in unnamed namespace except for FontGDI_Allocate and SurfaceGDI_Allocate
namespace {

constexpr Supports SupportsGDI[] = {
	Supports::PixelModification,
};

constexpr BYTE Win32MapFontQuality(FontQuality extraFontFlag) noexcept {
	switch (extraFontFlag & FontQuality::QualityMask) {

	case FontQuality::QualityNonAntialiased:
		return NONANTIALIASED_QUALITY;

	case FontQuality::QualityAntialiased:
		return ANTIALIASED_QUALITY;

	case FontQuality::QualityLcdOptimized:
		return CLEARTYPE_QUALITY;

	default:
		return DEFAULT_QUALITY;
	}
}

void SetLogFont(LOGFONTW &lf, const char *faceName, CharacterSet characterSet, XYPOSITION size, FontWeight weight, bool italic, FontQuality extraFontFlag) {
	lf = LOGFONTW();
	// The negative is to allow for leading
	lf.lfHeight = -(std::abs(std::lround(size)));
	lf.lfWeight = static_cast<LONG>(weight);
	lf.lfItalic = italic ? 1 : 0;
	lf.lfCharSet = static_cast<BYTE>(characterSet);
	lf.lfQuality = Win32MapFontQuality(extraFontFlag);
	UTF16FromUTF8(faceName, lf.lfFaceName, LF_FACESIZE);
}

struct FontGDI : public FontWin {
	HFONT hfont = {};
	CharacterSet characterSet = CharacterSet::Ansi;
	FontGDI(HFONT hfont_, CharacterSet characterSet_) noexcept : hfont(hfont_), characterSet(characterSet_) {
		// Takes ownership and deletes the font
	}
	explicit FontGDI(const FontParameters &fp) : characterSet(fp.characterSet) {
		LOGFONTW lf;
		SetLogFont(lf, fp.faceName, fp.characterSet, fp.size, fp.weight, fp.italic, fp.extraFontFlag);
		hfont = ::CreateFontIndirectW(&lf);
	}
	// Deleted so FontGDI objects can not be copied.
	FontGDI(const FontGDI &) = delete;
	FontGDI(FontGDI &&) = delete;
	FontGDI &operator=(const FontGDI &) = delete;
	FontGDI &operator=(FontGDI &&) = delete;
	~FontGDI() noexcept override {
		if (hfont)
			::DeleteObject(hfont);
	}
	[[nodiscard]] HFONT HFont() const noexcept override {
		// Duplicating hfont
		LOGFONTW lf = {};
		if (0 == ::GetObjectW(hfont, sizeof(lf), &lf)) {
			return {};
		}
		return ::CreateFontIndirectW(&lf);
	}
	[[nodiscard]] std::unique_ptr<FontWin> Duplicate() const override {
		HFONT hfontCopy = HFont();
		return std::make_unique<FontGDI>(hfontCopy, characterSet);
	}
	[[nodiscard]] CharacterSet GetCharacterSet() const noexcept override {
		return characterSet;
	}
};

class SurfaceGDI : public Surface {
	SurfaceMode mode;
	HDC hdc{};
	bool hdcOwned = false;
	HPEN pen{};
	HPEN penOld{};
	HBRUSH brush{};
	HBRUSH brushOld{};
	HFONT fontOld{};
	HBITMAP bitmap{};
	HBITMAP bitmapOld{};

	int logPixelsY = USER_DEFAULT_SCREEN_DPI;

	static constexpr int maxWidthMeasure = INT_MAX;
	// There appears to be a 16 bit string length limit in GDI on NT.
	static constexpr int maxLenText = 65535;

	void PenColour(ColourRGBA fore, XYPOSITION widthStroke) noexcept;

	void BrushColour(ColourRGBA back) noexcept;
	void SetFont(const Font *font_);
	void Clear() noexcept;

public:
	SurfaceGDI() noexcept = default;
	SurfaceGDI(HDC hdcCompatible, int width, int height, SurfaceMode mode_, int logPixelsY_) noexcept;
	// Deleted so SurfaceGDI objects can not be copied.
	SurfaceGDI(const SurfaceGDI &) = delete;
	SurfaceGDI(SurfaceGDI &&) = delete;
	SurfaceGDI &operator=(const SurfaceGDI &) = delete;
	SurfaceGDI &operator=(SurfaceGDI &&) = delete;

	~SurfaceGDI() noexcept override;

	void Init(WindowID wid) override;
	void Init(SurfaceID sid, WindowID wid) override;
	std::unique_ptr<Surface> AllocatePixMap(int width, int height) override;

	void SetMode(SurfaceMode mode_) override;

	void Release() noexcept override;
	int SupportsFeature(Supports feature) noexcept override;
	bool Initialised() override;
	int LogPixelsY() override;
	int PixelDivisions() override;
	int DeviceHeightFont(int points) override;
	void LineDraw(Point start, Point end, Stroke stroke) override;
	void PolyLine(const Point *pts, size_t npts, Stroke stroke) override;
	void Polygon(const Point *pts, size_t npts, FillStroke fillStroke) override;
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
	void Copy(PRectangle rc, Point from, Surface &surfaceSource) override;

	std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) override;

	void DrawTextCommon(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, UINT fuOptions);
	void DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) override;
	void MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthText(const Font *font_, std::string_view text) override;

	void DrawTextCommonUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, UINT fuOptions);
	void DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
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
};

SurfaceGDI::SurfaceGDI(HDC hdcCompatible, int width, int height, SurfaceMode mode_, int logPixelsY_) noexcept {
	hdc = ::CreateCompatibleDC(hdcCompatible);
	hdcOwned = true;
	bitmap = ::CreateCompatibleBitmap(hdcCompatible, width, height);
	bitmapOld = SelectBitmap(hdc, bitmap);
	::SetTextAlign(hdc, TA_BASELINE);
	mode = mode_;
	logPixelsY = logPixelsY_;
}

SurfaceGDI::~SurfaceGDI() noexcept {
	Clear();
}

void SurfaceGDI::Clear() noexcept {
	if (penOld) {
		::SelectObject(hdc, penOld);
		::DeleteObject(pen);
		penOld = {};
	}
	pen = {};
	if (brushOld) {
		::SelectObject(hdc, brushOld);
		::DeleteObject(brush);
		brushOld = {};
	}
	brush = {};
	if (fontOld) {
		// Fonts are not deleted as they are owned by a Font object
		::SelectObject(hdc, fontOld);
		fontOld = {};
	}
	if (bitmapOld) {
		::SelectObject(hdc, bitmapOld);
		::DeleteObject(bitmap);
		bitmapOld = {};
	}
	bitmap = {};
	if (hdcOwned) {
		::DeleteDC(hdc);
		hdc = {};
		hdcOwned = false;
	}
}

void SurfaceGDI::Release() noexcept {
	Clear();
}

int SurfaceGDI::SupportsFeature(Supports feature) noexcept {
	for (const Supports f : SupportsGDI) {
		if (f == feature)
			return 1;
	}
	return 0;
}

bool SurfaceGDI::Initialised() {
	return hdc;
}

void SurfaceGDI::Init(WindowID wid) {
	Release();
	hdc = ::CreateCompatibleDC({});
	hdcOwned = true;
	::SetTextAlign(hdc, TA_BASELINE);
	logPixelsY = DpiForWindow(wid);
}

void SurfaceGDI::Init(SurfaceID sid, WindowID wid) {
	Release();
	hdc = static_cast<HDC>(sid);
	::SetTextAlign(hdc, TA_BASELINE);
	// Windows on screen are scaled but printers are not.
	const bool printing = ::GetDeviceCaps(hdc, TECHNOLOGY) != DT_RASDISPLAY;
	logPixelsY = printing ? ::GetDeviceCaps(hdc, LOGPIXELSY) : DpiForWindow(wid);
}

std::unique_ptr<Surface> SurfaceGDI::AllocatePixMap(int width, int height) {
	return std::make_unique<SurfaceGDI>(hdc, width, height, mode, logPixelsY);
}

void SurfaceGDI::SetMode(SurfaceMode mode_) {
	mode = mode_;
}

void SurfaceGDI::PenColour(ColourRGBA fore, XYPOSITION widthStroke) noexcept {
	if (pen) {
		::SelectObject(hdc, penOld);
		::DeleteObject(pen);
		pen = {};
		penOld = {};
	}
	const DWORD penWidth = std::lround(widthStroke);
	const COLORREF penColour = fore.OpaqueRGB();
	if (widthStroke > 1) {
		const LOGBRUSH brushParameters{ BS_SOLID, penColour, 0 };
		pen = ::ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_ROUND | PS_JOIN_MITER,
			penWidth,
			&brushParameters,
			0,
			nullptr);
	} else {
		pen = ::CreatePen(PS_INSIDEFRAME, penWidth, penColour);
	}
	penOld = SelectPen(hdc, pen);
}

void SurfaceGDI::BrushColour(ColourRGBA back) noexcept {
	if (brush) {
		::SelectObject(hdc, brushOld);
		::DeleteObject(brush);
		brush = {};
		brushOld = {};
	}
	brush = ::CreateSolidBrush(back.OpaqueRGB());
	brushOld = SelectBrush(hdc, brush);
}

void SurfaceGDI::SetFont(const Font *font_) {
	const FontGDI *pfm = dynamic_cast<const FontGDI *>(font_);
	PLATFORM_ASSERT(pfm);
	if (!pfm) {
		throw std::runtime_error("SurfaceGDI::SetFont: wrong Font type.");
	}
	if (fontOld) {
		SelectFont(hdc, pfm->hfont);
	} else {
		fontOld = SelectFont(hdc, pfm->hfont);
	}
}

int SurfaceGDI::LogPixelsY() {
	return logPixelsY;
}

int SurfaceGDI::PixelDivisions() {
	// Win32 uses device pixels.
	return 1;
}

int SurfaceGDI::DeviceHeightFont(int points) {
	return ::MulDiv(points, LogPixelsY(), pointsPerInch);
}

void SurfaceGDI::LineDraw(Point start, Point end, Stroke stroke) {
	PenColour(stroke.colour, stroke.width);
	::MoveToEx(hdc, std::lround(std::floor(start.x)), std::lround(std::floor(start.y)), nullptr);
	::LineTo(hdc, std::lround(std::floor(end.x)), std::lround(std::floor(end.y)));
}

void SurfaceGDI::PolyLine(const Point *pts, size_t npts, Stroke stroke) {
	PLATFORM_ASSERT(npts > 1);
	if (npts <= 1) {
		return;
	}
	PenColour(stroke.colour, stroke.width);
	std::vector<POINT> outline;
	std::transform(pts, pts + npts, std::back_inserter(outline), POINTFromPoint);
	::Polyline(hdc, outline.data(), static_cast<int>(npts));
}

void SurfaceGDI::Polygon(const Point *pts, size_t npts, FillStroke fillStroke) {
	PenColour(fillStroke.stroke.colour.WithoutAlpha(), fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour.WithoutAlpha());
	std::vector<POINT> outline;
	std::transform(pts, pts + npts, std::back_inserter(outline), POINTFromPoint);
	::Polygon(hdc, outline.data(), static_cast<int>(npts));
}

void SurfaceGDI::RectangleDraw(PRectangle rc, FillStroke fillStroke) {
	RectangleFrame(rc, fillStroke.stroke);
	FillRectangle(rc.Inset(fillStroke.stroke.width), fillStroke.fill.colour);
}

void SurfaceGDI::RectangleFrame(PRectangle rc, Stroke stroke) {
	BrushColour(stroke.colour);
	const RECT rcw = RectFromPRectangle(rc);
	::FrameRect(hdc, &rcw, brush);
}

void SurfaceGDI::FillRectangle(PRectangle rc, Fill fill) {
	if (fill.colour.IsOpaque()) {
		// Using ExtTextOut rather than a FillRect ensures that no dithering occurs.
		// There is no need to allocate a brush either.
		const RECT rcw = RectFromPRectangle(rc);
		::SetBkColor(hdc, fill.colour.OpaqueRGB());
		::ExtTextOut(hdc, rcw.left, rcw.top, ETO_OPAQUE, &rcw, TEXT(""), 0, nullptr);
	} else {
		AlphaRectangle(rc, 0, FillStroke(fill.colour));
	}
}

void SurfaceGDI::FillRectangleAligned(PRectangle rc, Fill fill) {
	FillRectangle(PixelAlign(rc, 1), fill);
}

void SurfaceGDI::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	HBRUSH br{};
	if (SurfaceGDI *psgdi = dynamic_cast<SurfaceGDI *>(&surfacePattern); psgdi && psgdi->bitmap) {
		br = ::CreatePatternBrush(psgdi->bitmap);
	} else {	// Something is wrong so display in red
		br = ::CreateSolidBrush(RGB(0xff, 0, 0));
	}
	const RECT rcw = RectFromPRectangle(rc);
	::FillRect(hdc, &rcw, br);
	::DeleteObject(br);
}

void SurfaceGDI::RoundedRectangle(PRectangle rc, FillStroke fillStroke) {
	PenColour(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);
	const RECT rcw = RectFromPRectangle(rc);
	constexpr int cornerSize = 8;
	::RoundRect(hdc,
		rcw.left + 1, rcw.top,
		rcw.right - 1, rcw.bottom,
		cornerSize, cornerSize);
}

// DIBSection is bitmap with some drawing operations used by SurfaceGDI.
class DIBSection {
	GDIBitMap bm;
	SIZE size{};
	DWORD *pixels = nullptr;
public:
	DIBSection(HDC hdc, SIZE size_) noexcept;
	explicit operator bool() const noexcept {
		return bm && pixels;
	}
	[[nodiscard]] DWORD *Pixels() const noexcept {
		return pixels;
	}
	[[nodiscard]] unsigned char *Bytes() const noexcept {
		return reinterpret_cast<unsigned char *>(pixels);
	}
	[[nodiscard]] HDC DC() const noexcept {
		return bm.DC();
	}
	void SetPixel(LONG x, LONG y, DWORD value) noexcept {
		PLATFORM_ASSERT(x >= 0);
		PLATFORM_ASSERT(y >= 0);
		PLATFORM_ASSERT(x < size.cx);
		PLATFORM_ASSERT(y < size.cy);
		pixels[(y * size.cx) + x] = value;
	}
	void SetSymmetric(LONG x, LONG y, DWORD value) noexcept;
};

DIBSection::DIBSection(HDC hdc, SIZE size_) noexcept : size(size_) {
	// -size.y makes bitmap start from top
	bm.Create(hdc, size.cx, -size.cy, &pixels);
}

void DIBSection::SetSymmetric(LONG x, LONG y, DWORD value) noexcept {
	// Plot a point symmetrically to all 4 quadrants
	const LONG xSymmetric = size.cx - 1 - x;
	const LONG ySymmetric = size.cy - 1 - y;
	SetPixel(x, y, value);
	SetPixel(xSymmetric, y, value);
	SetPixel(x, ySymmetric, value);
	SetPixel(xSymmetric, ySymmetric, value);
}

ColourRGBA GradientValue(const std::vector<ColourStop> &stops, XYPOSITION proportion) noexcept {
	for (size_t stop = 0; stop < stops.size() - 1; stop++) {
		// Loop through each pair of stops
		const XYPOSITION positionStart = stops[stop].position;
		const XYPOSITION positionEnd = stops[stop + 1].position;
		if ((proportion >= positionStart) && (proportion <= positionEnd)) {
			const XYPOSITION proportionInPair = (proportion - positionStart) /
				(positionEnd - positionStart);
			return stops[stop].colour.MixedWith(stops[stop + 1].colour, proportionInPair);
		}
	}
	// Loop should always find a value
	return ColourRGBA();
}

constexpr DWORD dwordFromBGRA(byte b, byte g, byte r, byte a) noexcept {
	constexpr int aShift = 24;
	constexpr int rShift = 16;
	constexpr int gShift = 8;
	return (a << aShift) | (r << rShift) | (g << gShift) | b;
}

constexpr byte AlphaScaled(unsigned char component, unsigned int alpha) noexcept {
	constexpr byte maxByte = 0xFFU;
	return (component * alpha / maxByte) & maxByte;
}

constexpr DWORD dwordMultiplied(ColourRGBA colour) noexcept {
	return dwordFromBGRA(
		AlphaScaled(colour.GetBlue(), colour.GetAlpha()),
		AlphaScaled(colour.GetGreen(), colour.GetAlpha()),
		AlphaScaled(colour.GetRed(), colour.GetAlpha()),
		colour.GetAlpha());
}

constexpr BLENDFUNCTION mergeAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

void SurfaceGDI::AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) {
	// TODO: Implement strokeWidth
	const RECT rcw = RectFromPRectangle(rc);
	const SIZE size = SizeOfRect(rcw);

	if (size.cx > 0) {

		DIBSection section(hdc, size);

		if (section) {

			// Ensure not distorted too much by corners when small
			const LONG corner = std::min(static_cast<LONG>(cornerSize), (std::min(size.cx, size.cy) / 2) - 2);

			constexpr DWORD valEmpty = dwordFromBGRA(0, 0, 0, 0);
			const DWORD valFill = dwordMultiplied(fillStroke.fill.colour);
			const DWORD valOutline = dwordMultiplied(fillStroke.stroke.colour);

			// Draw a framed rectangle
			for (int y = 0; y < size.cy; y++) {
				for (int x = 0; x < size.cx; x++) {
					if ((x == 0) || (x == size.cx - 1) || (y == 0) || (y == size.cy - 1)) {
						section.SetPixel(x, y, valOutline);
					} else {
						section.SetPixel(x, y, valFill);
					}
				}
			}

			// Make the corners transparent
			for (LONG c = 0; c < corner; c++) {
				for (LONG x = 0; x < c + 1; x++) {
					section.SetSymmetric(x, c - x, valEmpty);
				}
			}

			// Draw the corner frame pieces
			for (LONG x = 1; x < corner; x++) {
				section.SetSymmetric(x, corner - x, valOutline);
			}

			GdiAlphaBlend(hdc, rcw.left, rcw.top, size.cx, size.cy, section.DC(), 0, 0, size.cx, size.cy, mergeAlpha);
		}
	} else {
		BrushColour(fillStroke.stroke.colour);
		FrameRect(hdc, &rcw, brush);
	}
}

void SurfaceGDI::GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) {

	const RECT rcw = RectFromPRectangle(rc);
	const SIZE size = SizeOfRect(rcw);

	DIBSection section(hdc, size);

	if (section) {

		if (options == GradientOptions::topToBottom) {
			for (LONG y = 0; y < size.cy; y++) {
				// Find y/height proportional colour
				const XYPOSITION proportion = y / (rc.Height() - 1.0f);
				const ColourRGBA mixed = GradientValue(stops, proportion);
				const DWORD valFill = dwordMultiplied(mixed);
				for (LONG x = 0; x < size.cx; x++) {
					section.SetPixel(x, y, valFill);
				}
			}
		} else {
			for (LONG x = 0; x < size.cx; x++) {
				// Find x/width proportional colour
				const XYPOSITION proportion = x / (rc.Width() - 1.0f);
				const ColourRGBA mixed = GradientValue(stops, proportion);
				const DWORD valFill = dwordMultiplied(mixed);
				for (LONG y = 0; y < size.cy; y++) {
					section.SetPixel(x, y, valFill);
				}
			}
		}

		GdiAlphaBlend(hdc, rcw.left, rcw.top, size.cx, size.cy, section.DC(), 0, 0, size.cx, size.cy, mergeAlpha);
	}
}

void SurfaceGDI::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	if (rc.Width() > 0) {
		if (rc.Width() > width)
			rc.left += std::floor((rc.Width() - width) / 2);
		rc.right = rc.left + width;
		if (rc.Height() > height)
			rc.top += std::floor((rc.Height() - height) / 2);
		rc.bottom = rc.top + height;

		const SIZE size{ width, height };
		DIBSection section(hdc, size);
		if (section) {
			RGBAImage::BGRAFromRGBA(section.Bytes(), pixelsImage, static_cast<size_t>(width) * height);
			GdiAlphaBlend(hdc, static_cast<int>(rc.left), static_cast<int>(rc.top),
				static_cast<int>(rc.Width()), static_cast<int>(rc.Height()), section.DC(),
				0, 0, width, height, mergeAlpha);
		}
	}
}

void SurfaceGDI::Ellipse(PRectangle rc, FillStroke fillStroke) {
	PenColour(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);
	const RECT rcw = RectFromPRectangle(rc);
	::Ellipse(hdc, rcw.left, rcw.top, rcw.right, rcw.bottom);
}

void SurfaceGDI::Stadium(PRectangle rc, FillStroke fillStroke, [[maybe_unused]] Ends ends) {
	// TODO: Implement properly - the rectangle is just a placeholder
	RectangleDraw(rc, fillStroke);
}

void SurfaceGDI::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	::BitBlt(hdc,
		static_cast<int>(rc.left), static_cast<int>(rc.top),
		static_cast<int>(rc.Width()), static_cast<int>(rc.Height()),
		dynamic_cast<SurfaceGDI &>(surfaceSource).hdc,
		static_cast<int>(from.x), static_cast<int>(from.y), SRCCOPY);
}

std::unique_ptr<IScreenLineLayout> SurfaceGDI::Layout(const IScreenLine *) {
	return {};
}

using TextPositionsI = VarBuffer<int, stackBufferLength>;

void SurfaceGDI::DrawTextCommon(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, UINT fuOptions) {
	SetFont(font_);
	const RECT rcw = RectFromPRectangle(rc);
	const int x = static_cast<int>(rc.left);
	const int yBaseInt = static_cast<int>(ybase);

	if (mode.codePage == CpUtf8) {
		const TextWide tbuf(text, mode.codePage);
		::ExtTextOutW(hdc, x, yBaseInt, fuOptions, &rcw, tbuf.buffer, tbuf.tlen, nullptr);
	} else {
		::ExtTextOutA(hdc, x, yBaseInt, fuOptions, &rcw, text.data(), static_cast<UINT>(text.length()), nullptr);
	}
}

void SurfaceGDI::DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	::SetTextColor(hdc, fore.OpaqueRGB());
	::SetBkColor(hdc, back.OpaqueRGB());
	DrawTextCommon(rc, font_, ybase, text, ETO_OPAQUE);
}

void SurfaceGDI::DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	::SetTextColor(hdc, fore.OpaqueRGB());
	::SetBkColor(hdc, back.OpaqueRGB());
	DrawTextCommon(rc, font_, ybase, text, ETO_OPAQUE | ETO_CLIPPED);
}

void SurfaceGDI::DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore) {
	// Avoid drawing spaces in transparent mode
	for (const char ch : text) {
		if (ch != ' ') {
			::SetTextColor(hdc, fore.OpaqueRGB());
			::SetBkMode(hdc, TRANSPARENT);
			DrawTextCommon(rc, font_, ybase, text, 0);
			::SetBkMode(hdc, OPAQUE);
			return;
		}
	}
}

void SurfaceGDI::MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) {
	// Zero positions to avoid random behaviour on failure.
	std::fill(positions, positions + text.length(), 0.0f);
	SetFont(font_);
	SIZE sz = { 0,0 };
	int fit = 0;
	int i = 0;
	const int len = static_cast<int>(text.length());
	if (mode.codePage == CpUtf8) {
		const TextWide tbuf(text, mode.codePage);
		TextPositionsI poses(tbuf.tlen);
		if (!::GetTextExtentExPointW(hdc, tbuf.buffer, tbuf.tlen, maxWidthMeasure, &fit, poses.buffer, &sz)) {
			// Failure
			return;
		}
		// Map the widths given for UTF-16 characters back onto the UTF-8 input string
		for (int ui = 0; ui < fit; ui++) {
			const unsigned char uch = text[i];
			const unsigned int byteCount = UTF8BytesOfLead[uch];
			if (byteCount == 4) {	// Non-BMP
				ui++;
			}
			for (unsigned int bytePos = 0; (bytePos < byteCount) && (i < len); bytePos++) {
				positions[i++] = static_cast<XYPOSITION>(poses.buffer[ui]);
			}
		}
	} else {
		TextPositionsI poses(len);
		if (!::GetTextExtentExPointA(hdc, text.data(), len, maxWidthMeasure, &fit, poses.buffer, &sz)) {
			// Eeek - a NULL DC or other foolishness could cause this.
			return;
		}
		while (i < fit) {
			positions[i] = static_cast<XYPOSITION>(poses.buffer[i]);
			i++;
		}
	}
	// If any positions not filled in then use the last position for them
	const XYPOSITION lastPos = (fit > 0) ? positions[fit - 1] : 0.0f;
	std::fill(positions + i, positions + text.length(), lastPos);
}

XYPOSITION SurfaceGDI::WidthText(const Font *font_, std::string_view text) {
	SetFont(font_);
	SIZE sz = { 0,0 };
	if (!(mode.codePage == CpUtf8)) {
		::GetTextExtentPoint32A(hdc, text.data(), std::min(static_cast<int>(text.length()), maxLenText), &sz);
	} else {
		const TextWide tbuf(text, mode.codePage);
		::GetTextExtentPoint32W(hdc, tbuf.buffer, tbuf.tlen, &sz);
	}
	return static_cast<XYPOSITION>(sz.cx);
}

void SurfaceGDI::DrawTextCommonUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, UINT fuOptions) {
	SetFont(font_);
	const RECT rcw = RectFromPRectangle(rc);
	const int x = static_cast<int>(rc.left);
	const int yBaseInt = static_cast<int>(ybase);

	const TextWide tbuf(text, CpUtf8);
	::ExtTextOutW(hdc, x, yBaseInt, fuOptions, &rcw, tbuf.buffer, tbuf.tlen, nullptr);
}

void SurfaceGDI::DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	::SetTextColor(hdc, fore.OpaqueRGB());
	::SetBkColor(hdc, back.OpaqueRGB());
	DrawTextCommonUTF8(rc, font_, ybase, text, ETO_OPAQUE);
}

void SurfaceGDI::DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	::SetTextColor(hdc, fore.OpaqueRGB());
	::SetBkColor(hdc, back.OpaqueRGB());
	DrawTextCommonUTF8(rc, font_, ybase, text, ETO_OPAQUE | ETO_CLIPPED);
}

void SurfaceGDI::DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore) {
	// Avoid drawing spaces in transparent mode
	for (const char ch : text) {
		if (ch != ' ') {
			::SetTextColor(hdc, fore.OpaqueRGB());
			::SetBkMode(hdc, TRANSPARENT);
			DrawTextCommonUTF8(rc, font_, ybase, text, 0);
			::SetBkMode(hdc, OPAQUE);
			return;
		}
	}
}

void SurfaceGDI::MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) {
	// Zero positions to avoid random behaviour on failure.
	std::fill(positions, positions + text.length(), 0.0f);
	SetFont(font_);
	SIZE sz = { 0,0 };
	int fit = 0;
	int i = 0;
	const int len = static_cast<int>(text.length());
	const TextWide tbuf(text, CpUtf8);
	TextPositionsI poses(tbuf.tlen);
	if (!::GetTextExtentExPointW(hdc, tbuf.buffer, tbuf.tlen, maxWidthMeasure, &fit, poses.buffer, &sz)) {
		// Failure
		return;
	}
	// Map the widths given for UTF-16 characters back onto the UTF-8 input string
	for (int ui = 0; ui < fit; ui++) {
		const unsigned char uch = text[i];
		const unsigned int byteCount = UTF8BytesOfLead[uch];
		if (byteCount == 4) {	// Non-BMP
			ui++;
		}
		for (unsigned int bytePos = 0; (bytePos < byteCount) && (i < len); bytePos++) {
			positions[i++] = static_cast<XYPOSITION>(poses.buffer[ui]);
		}
	}
	// If any positions not filled in then use the last position for them
	const XYPOSITION lastPos = (fit > 0) ? positions[fit - 1] : 0.0f;
	std::fill(positions + i, positions + text.length(), lastPos);
}

XYPOSITION SurfaceGDI::WidthTextUTF8(const Font *font_, std::string_view text) {
	SetFont(font_);
	SIZE sz = { 0,0 };
	const TextWide tbuf(text, CpUtf8);
	::GetTextExtentPoint32W(hdc, tbuf.buffer, tbuf.tlen, &sz);
	return static_cast<XYPOSITION>(sz.cx);
}

XYPOSITION SurfaceGDI::Ascent(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmAscent);
}

XYPOSITION SurfaceGDI::Descent(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmDescent);
}

XYPOSITION SurfaceGDI::InternalLeading(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmInternalLeading);
}

XYPOSITION SurfaceGDI::Height(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmHeight);
}

XYPOSITION SurfaceGDI::AverageCharWidth(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmAveCharWidth);
}

void SurfaceGDI::SetClip(PRectangle rc) {
	::SaveDC(hdc);
	::IntersectClipRect(hdc, static_cast<int>(rc.left), static_cast<int>(rc.top),
		static_cast<int>(rc.right), static_cast<int>(rc.bottom));
}

void SurfaceGDI::PopClip() {
	::RestoreDC(hdc, -1);
}

void SurfaceGDI::FlushCachedState() {
	pen = {};
	brush = {};
}

void SurfaceGDI::FlushDrawing() {
}

}

namespace Scintilla::Internal {

std::shared_ptr<Font> FontGDI_Allocate(const FontParameters &fp) {
	return std::make_shared<FontGDI>(fp);
}

std::unique_ptr<Surface> SurfaceGDI_Allocate() {
	return std::make_unique<SurfaceGDI>();
}

}
