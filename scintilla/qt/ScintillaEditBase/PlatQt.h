//
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

#include <string_view>
#include <vector>
#include <memory>

#include "Platform.h"

#include <QUrl>
#include <QPaintDevice>
#include <QPainter>
#include <QHash>

namespace Scintilla {

const char *CharacterSetID(int characterSet);

inline QColor QColorFromCA(ColourDesired ca)
{
	long c = ca.AsInteger();
	return QColor(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
}

inline QColor QColorFromColourAlpha(ColourAlpha ca)
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

constexpr PRectangle RectangleInset(PRectangle rc, XYPOSITION delta) noexcept {
	return PRectangle(rc.left + delta, rc.top + delta, rc.right - delta, rc.bottom - delta);
}

class SurfaceImpl : public Surface {
private:
	QPaintDevice *device;
	QPainter *painter;
	bool deviceOwned;
	bool painterOwned;
	float x, y;
	bool unicodeMode;
	int codePage;
	const char *codecName;
	QTextCodec *codec;

	void Clear();

public:
	SurfaceImpl();
	virtual ~SurfaceImpl();

	void Init(WindowID wid) override;
	void Init(SurfaceID sid, WindowID wid) override;
	void InitPixMap(int width, int height,
		Surface *surface, WindowID wid) override;

	void Release() override;
	bool Initialised() override;
	void PenColour(ColourDesired fore) override;
	int LogPixelsY() override;
	int DeviceHeightFont(int points) override;
	void MoveTo(int x_, int y_) override;
	void LineTo(int x_, int y_) override;
	void Polygon(Point *pts, size_t npts, ColourDesired fore,
		ColourDesired back) override;
	void RectangleDraw(PRectangle rc, ColourDesired fore,
		ColourDesired back) override;
	void FillRectangle(PRectangle rc, ColourDesired back) override;
	void FillRectangle(PRectangle rc, Surface &surfacePattern) override;
	void RoundedRectangle(PRectangle rc, ColourDesired fore,
		ColourDesired back) override;
	void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill,
		int alphaFill, ColourDesired outline, int alphaOutline, int flags) override;
	void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) override;
	void DrawRGBAImage(PRectangle rc, int width, int height,
		const unsigned char *pixelsImage) override;
	void Ellipse(PRectangle rc, ColourDesired fore,
		ColourDesired back) override;
	void Copy(PRectangle rc, Point from, Surface &surfaceSource) override;

	std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) override;

	void DrawTextNoClip(PRectangle rc, Font &font, XYPOSITION ybase,
		std::string_view text, ColourDesired fore, ColourDesired back) override;
	void DrawTextClipped(PRectangle rc, Font &font, XYPOSITION ybase,
		std::string_view text, ColourDesired fore, ColourDesired back) override;
	void DrawTextTransparent(PRectangle rc, Font &font, XYPOSITION ybase,
		std::string_view text, ColourDesired fore) override;
	void MeasureWidths(Font &font, std::string_view text,
		XYPOSITION *positions) override;
	XYPOSITION WidthText(Font &font, std::string_view text) override;
	XYPOSITION Ascent(Font &font) override;
	XYPOSITION Descent(Font &font) override;
	XYPOSITION InternalLeading(Font &font) override;
	XYPOSITION Height(Font &font) override;
	XYPOSITION AverageCharWidth(Font &font) override;

	void SetClip(PRectangle rc) override;
	void FlushCachedState() override;

	void SetUnicodeMode(bool unicodeMode_) override;
	void SetDBCSMode(int codePage_) override;
	void SetBidiR2L(bool bidiR2L_) override;

	void BrushColour(ColourDesired back);
	void SetCodec(const Font &font);
	void SetFont(const Font &font);

	QPaintDevice *GetPaintDevice();
	void SetPainter(QPainter *painter);
	QPainter *GetPainter();
};

}

#endif
