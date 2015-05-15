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

#include "Platform.h"

#include <QPaintDevice>
#include <QPainter>
#include <QHash>

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

const char *CharacterSetID(int characterSet);

inline QColor QColorFromCA(ColourDesired ca)
{
	long c = ca.AsLong();
	return QColor(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
}

inline QRect QRectFromPRect(PRectangle pr)
{
	return QRect(pr.left, pr.top, pr.Width(), pr.Height());
}

inline PRectangle PRectFromQRect(QRect qr)
{
	return PRectangle(qr.x(), qr.y(), qr.x() + qr.width(), qr.y() + qr.height());
}

inline Point PointFromQPoint(QPoint qp)
{
	return Point(qp.x(), qp.y());
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

public:
	SurfaceImpl();
	virtual ~SurfaceImpl();

	virtual void Init(WindowID wid);
	virtual void Init(SurfaceID sid, WindowID wid);
	virtual void InitPixMap(int width, int height,
		Surface *surface, WindowID wid);

	virtual void Release();
	virtual bool Initialised();
	virtual void PenColour(ColourDesired fore);
	virtual int LogPixelsY();
	virtual int DeviceHeightFont(int points);
	virtual void MoveTo(int x, int y);
	virtual void LineTo(int x, int y);
	virtual void Polygon(Point *pts, int npts, ColourDesired fore,
		ColourDesired back);
	virtual void RectangleDraw(PRectangle rc, ColourDesired fore,
		ColourDesired back);
	virtual void FillRectangle(PRectangle rc, ColourDesired back);
	virtual void FillRectangle(PRectangle rc, Surface &surfacePattern);
	virtual void RoundedRectangle(PRectangle rc, ColourDesired fore,
		ColourDesired back);
	virtual void AlphaRectangle(PRectangle rc, int corner, ColourDesired fill,
		int alphaFill, ColourDesired outline, int alphaOutline, int flags);
	virtual void DrawRGBAImage(PRectangle rc, int width, int height,
		const unsigned char *pixelsImage);
	virtual void Ellipse(PRectangle rc, ColourDesired fore,
		ColourDesired back);
	virtual void Copy(PRectangle rc, Point from, Surface &surfaceSource);

	virtual void DrawTextNoClip(PRectangle rc, Font &font, XYPOSITION ybase,
		const char *s, int len, ColourDesired fore, ColourDesired back);
	virtual void DrawTextClipped(PRectangle rc, Font &font, XYPOSITION ybase,
		const char *s, int len, ColourDesired fore, ColourDesired back);
	virtual void DrawTextTransparent(PRectangle rc, Font &font, XYPOSITION ybase,
		const char *s, int len, ColourDesired fore);
	virtual void MeasureWidths(Font &font, const char *s, int len,
		XYPOSITION *positions);
	virtual XYPOSITION WidthText(Font &font, const char *s, int len);
	virtual XYPOSITION WidthChar(Font &font, char ch);
	virtual XYPOSITION Ascent(Font &font);
	virtual XYPOSITION Descent(Font &font);
	virtual XYPOSITION InternalLeading(Font &font);
	virtual XYPOSITION ExternalLeading(Font &font);
	virtual XYPOSITION Height(Font &font);
	virtual XYPOSITION AverageCharWidth(Font &font);

	virtual void SetClip(PRectangle rc);
	virtual void FlushCachedState();

	virtual void SetUnicodeMode(bool unicodeMode);
	virtual void SetDBCSMode(int codePage);

	void BrushColour(ColourDesired back);
	void SetCodec(Font &font);
	void SetFont(Font &font);

	QPaintDevice *GetPaintDevice();
	void SetPainter(QPainter *painter);
	QPainter *GetPainter();
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
