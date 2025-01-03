// @file PlatQt.h
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// Scintilla platform layer for Qt

#ifndef PLATQT_H
#define PLATQT_H

#include <cstddef>
#include <cstdint>

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>

#include "Debugging.h"
#include "Geometry.h"
#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "Platform.h"

#include <QUrl>
#include <QPaintDevice>
#include <QPainter>
#include <QHash>
#include <QTextCodec>

namespace Scintilla::Internal {

const char *CharacterSetID(Scintilla::CharacterSet characterSet);

inline QColor QColorFromColourRGBA(ColourRGBA ca)
{
	return QColor(ca.GetRed(), ca.GetGreen(), ca.GetBlue(), ca.GetAlpha());
}

inline QRect QRectFromPRect(PRectangle pr)
{
	return QRect(pr.left, pr.top, pr.Width(), pr.Height());
}

inline QRectF QRectFFromPRect(PRectangle pr)
{
	return QRectF(pr.left, pr.top, pr.Width(), pr.Height());
}

inline PRectangle PRectFromQRect(QRect qr)
{
	return PRectangle(qr.x(), qr.y(), qr.x() + qr.width(), qr.y() + qr.height());
}

inline Point PointFromQPoint(QPoint qp)
{
	return Point(qp.x(), qp.y());
}

inline QPointF QPointFFromPoint(Point qp)
{
	return QPointF(qp.x, qp.y);
}

constexpr PRectangle RectangleInset(PRectangle rc, XYPOSITION delta) noexcept {
	return PRectangle(rc.left + delta, rc.top + delta, rc.right - delta, rc.bottom - delta);
}

class SurfaceImpl : public Surface {
private:
	QPaintDevice *device = nullptr;
	QPainter *painter = nullptr;
	bool deviceOwned = false;
	bool painterOwned = false;
	SurfaceMode mode;
	const char *codecName = nullptr;
	QTextCodec *codec = nullptr;

	void Clear();

public:
	SurfaceImpl();
	SurfaceImpl(int width, int height, SurfaceMode mode_);
	virtual ~SurfaceImpl() override;

	void Init(WindowID wid) override;
	void Init(SurfaceID sid, WindowID wid) override;
	std::unique_ptr<Surface> AllocatePixMap(int width, int height) override;

	void SetMode(SurfaceMode mode) override;

	void Release() noexcept override;
	int SupportsFeature(Scintilla::Supports feature) noexcept override;
	bool Initialised() override;
	void PenColour(ColourRGBA fore);
	void PenColourWidth(ColourRGBA fore, XYPOSITION strokeWidth);
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
	void DrawRGBAImage(PRectangle rc, int width, int height,
		const unsigned char *pixelsImage) override;
	void Ellipse(PRectangle rc, FillStroke fillStroke) override;
	void Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) override;
	void Copy(PRectangle rc, Point from, Surface &surfaceSource) override;

	std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) override;

	void DrawTextNoClip(PRectangle rc, const Font *font, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClipped(PRectangle rc, const Font *font, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextTransparent(PRectangle rc, const Font *font, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore) override;
	void MeasureWidths(const Font *font, std::string_view text,
		XYPOSITION *positions) override;
	XYPOSITION WidthText(const Font *font, std::string_view text) override;

	void DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase,
		std::string_view text, ColourRGBA fore) override;
	void MeasureWidthsUTF8(const Font *font_, std::string_view text,
		XYPOSITION *positions) override;
	XYPOSITION WidthTextUTF8(const Font *font_, std::string_view text) override;

	XYPOSITION Ascent(const Font *font) override;
	XYPOSITION Descent(const Font *font) override;
	XYPOSITION InternalLeading(const Font *font) override;
	XYPOSITION Height(const Font *font) override;
	XYPOSITION AverageCharWidth(const Font *font) override;

	void SetClip(PRectangle rc) override;
	void PopClip() override;
	void FlushCachedState() override;
	void FlushDrawing() override;

	void BrushColour(ColourRGBA back);
	void SetCodec(const Font *font);
	void SetFont(const Font *font);

	QPaintDevice *GetPaintDevice();
	void SetPainter(QPainter *painter);
	QPainter *GetPainter();
};

}

#endif
