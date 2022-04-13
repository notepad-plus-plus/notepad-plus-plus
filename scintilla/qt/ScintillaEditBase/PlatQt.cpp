// @file PlatQt.cpp
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// Scintilla platform layer for Qt

#include <cstdio>

#include "PlatQt.h"
#include "Scintilla.h"
#include "UniConversion.h"
#include "DBCS.h"

#include <QApplication>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QScreen>
#endif
#include <QFont>
#include <QColor>
#include <QRect>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QMenu>
#include <QAction>
#include <QTime>
#include <QMessageBox>
#include <QTextCodec>
#include <QListWidget>
#include <QVarLengthArray>
#include <QScrollBar>
#include <QTextLayout>
#include <QTextLine>
#include <QLibrary>

using namespace Scintilla;

namespace Scintilla::Internal {

//----------------------------------------------------------------------

// Convert from a Scintilla characterSet value to a Qt codec name.
const char *CharacterSetID(CharacterSet characterSet)
{
	switch (characterSet) {
		//case CharacterSet::Ansi:
		//	return "";
	case CharacterSet::Default:
		return "ISO 8859-1";
	case CharacterSet::Baltic:
		return "ISO 8859-13";
	case CharacterSet::ChineseBig5:
		return "Big5";
	case CharacterSet::EastEurope:
		return "ISO 8859-2";
	case CharacterSet::GB2312:
		return "GB18030-0";
	case CharacterSet::Greek:
		return "ISO 8859-7";
	case CharacterSet::Hangul:
		return "CP949";
	case CharacterSet::Mac:
		return "Apple Roman";
		//case SC_CHARSET_OEM:
		//	return "ASCII";
	case CharacterSet::Russian:
		return "KOI8-R";
	case CharacterSet::Cyrillic:
		return "Windows-1251";
	case CharacterSet::ShiftJis:
		return "Shift-JIS";
		//case SC_CHARSET_SYMBOL:
		//        return "";
	case CharacterSet::Turkish:
		return "ISO 8859-9";
		//case SC_CHARSET_JOHAB:
		//        return "CP1361";
	case CharacterSet::Hebrew:
		return "ISO 8859-8";
	case CharacterSet::Arabic:
		return "ISO 8859-6";
	case CharacterSet::Vietnamese:
		return "Windows-1258";
	case CharacterSet::Thai:
		return "TIS-620";
	case CharacterSet::Iso8859_15:
		return "ISO 8859-15";
	default:
		return "ISO 8859-1";
	}
}

QString UnicodeFromText(QTextCodec *codec, std::string_view text) {
	return codec->toUnicode(text.data(), static_cast<int>(text.length()));
}

static QFont::StyleStrategy ChooseStrategy(FontQuality eff)
{
	switch (eff) {
		case FontQuality::QualityDefault:         return QFont::PreferDefault;
		case FontQuality::QualityNonAntialiased: return QFont::NoAntialias;
		case FontQuality::QualityAntialiased:     return QFont::PreferAntialias;
		case FontQuality::QualityLcdOptimized:   return QFont::PreferAntialias;
		default:                             return QFont::PreferDefault;
	}
}

class FontAndCharacterSet : public Font {
public:
	CharacterSet characterSet = CharacterSet::Ansi;
	std::unique_ptr<QFont> pfont;
	explicit FontAndCharacterSet(const FontParameters &fp) : characterSet(fp.characterSet) {
		pfont = std::make_unique<QFont>();
		pfont->setStyleStrategy(ChooseStrategy(fp.extraFontFlag));
		pfont->setFamily(QString::fromUtf8(fp.faceName));
		pfont->setPointSizeF(fp.size);
		pfont->setBold(static_cast<int>(fp.weight) > 500);
		pfont->setItalic(fp.italic);
	}
};

namespace {

const Supports SupportsQt[] = {
	Supports::LineDrawsFinal,
	Supports::FractionalStrokeWidth,
	Supports::TranslucentStroke,
	Supports::PixelModification,
};

const FontAndCharacterSet *AsFontAndCharacterSet(const Font *f) {
	return dynamic_cast<const FontAndCharacterSet *>(f);
}

QFont *FontPointer(const Font *f)
{
	return AsFontAndCharacterSet(f)->pfont.get();
}

}

std::shared_ptr<Font> Font::Allocate(const FontParameters &fp)
{
	return std::make_shared<FontAndCharacterSet>(fp);
}

SurfaceImpl::SurfaceImpl() = default;

SurfaceImpl::SurfaceImpl(int width, int height, SurfaceMode mode_)
{
	if (width < 1) width = 1;
	if (height < 1) height = 1;
	deviceOwned = true;
	device = new QPixmap(width, height);
	mode = mode_;
}

SurfaceImpl::~SurfaceImpl()
{
	Clear();
}

void SurfaceImpl::Clear()
{
	if (painterOwned && painter) {
		delete painter;
	}

	if (deviceOwned && device) {
		delete device;
	}
	device = nullptr;
	painter = nullptr;
	deviceOwned = false;
	painterOwned = false;
}

void SurfaceImpl::Init(WindowID wid)
{
	Release();
	device = static_cast<QWidget *>(wid);
}

void SurfaceImpl::Init(SurfaceID sid, WindowID /*wid*/)
{
	Release();
	device = static_cast<QPaintDevice *>(sid);
}

std::unique_ptr<Surface> SurfaceImpl::AllocatePixMap(int width, int height)
{
	return std::make_unique<SurfaceImpl>(width, height, mode);
}

void SurfaceImpl::SetMode(SurfaceMode mode_)
{
	mode = mode_;
}

void SurfaceImpl::Release() noexcept
{
	Clear();
}

int SurfaceImpl::SupportsFeature(Supports feature) noexcept
{
	for (const Supports f : SupportsQt) {
		if (f == feature)
			return 1;
	}
	return 0;
}

bool SurfaceImpl::Initialised()
{
	return device != nullptr;
}

void SurfaceImpl::PenColour(ColourRGBA fore)
{
	QPen penOutline(QColorFromColourRGBA(fore));
	penOutline.setCapStyle(Qt::FlatCap);
	GetPainter()->setPen(penOutline);
}

void SurfaceImpl::PenColourWidth(ColourRGBA fore, XYPOSITION strokeWidth) {
	QPen penOutline(QColorFromColourRGBA(fore));
	penOutline.setCapStyle(Qt::FlatCap);
	penOutline.setJoinStyle(Qt::MiterJoin);
	penOutline.setWidthF(strokeWidth);
	GetPainter()->setPen(penOutline);
}

void SurfaceImpl::BrushColour(ColourRGBA back)
{
	GetPainter()->setBrush(QBrush(QColorFromColourRGBA(back)));
}

void SurfaceImpl::SetCodec(const Font *font)
{
	const FontAndCharacterSet *pfacs = AsFontAndCharacterSet(font);
	if (pfacs && pfacs->pfont) {
		const char *csid = "UTF-8";
		if (!(mode.codePage == SC_CP_UTF8))
			csid = CharacterSetID(pfacs->characterSet);
		if (csid != codecName) {
			codecName = csid;
			codec = QTextCodec::codecForName(csid);
		}
	}
}

void SurfaceImpl::SetFont(const Font *font)
{
	const FontAndCharacterSet *pfacs = AsFontAndCharacterSet(font);
	if (pfacs && pfacs->pfont) {
		GetPainter()->setFont(*(pfacs->pfont));
		SetCodec(font);
	}
}

int SurfaceImpl::LogPixelsY()
{
	return device->logicalDpiY();
}

int SurfaceImpl::PixelDivisions()
{
	// Qt uses device pixels.
	return 1;
}

int SurfaceImpl::DeviceHeightFont(int points)
{
	return points;
}

void SurfaceImpl::LineDraw(Point start, Point end, Stroke stroke)
{
	PenColourWidth(stroke.colour, stroke.width);
	QLineF line(start.x, start.y, end.x, end.y);
	GetPainter()->drawLine(line);
}

void SurfaceImpl::PolyLine(const Point *pts, size_t npts, Stroke stroke)
{
	// TODO: set line joins and caps
	PenColourWidth(stroke.colour, stroke.width);
	std::vector<QPointF> qpts;
	std::transform(pts, pts + npts, std::back_inserter(qpts), QPointFFromPoint);
	GetPainter()->drawPolyline(&qpts[0], static_cast<int>(npts));
}

void SurfaceImpl::Polygon(const Point *pts, size_t npts, FillStroke fillStroke)
{
	PenColourWidth(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);

	std::vector<QPointF> qpts;
	std::transform(pts, pts + npts, std::back_inserter(qpts), QPointFFromPoint);

	GetPainter()->drawPolygon(&qpts[0], static_cast<int>(npts));
}

void SurfaceImpl::RectangleDraw(PRectangle rc, FillStroke fillStroke)
{
	PenColourWidth(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);
	const QRectF rect = QRectFFromPRect(rc.Inset(fillStroke.stroke.width / 2));
	GetPainter()->drawRect(rect);
}

void SurfaceImpl::RectangleFrame(PRectangle rc, Stroke stroke) {
	PenColourWidth(stroke.colour, stroke.width);
	// Default QBrush is Qt::NoBrush so does not fill
	GetPainter()->setBrush(QBrush());
	const QRectF rect = QRectFFromPRect(rc.Inset(stroke.width / 2));
	GetPainter()->drawRect(rect);
}

void SurfaceImpl::FillRectangle(PRectangle rc, Fill fill)
{
	GetPainter()->fillRect(QRectFFromPRect(rc), QColorFromColourRGBA(fill.colour));
}

void SurfaceImpl::FillRectangleAligned(PRectangle rc, Fill fill)
{
	FillRectangle(PixelAlign(rc, 1), fill);
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern)
{
	// Tile pattern over rectangle
	SurfaceImpl *surface = dynamic_cast<SurfaceImpl *>(&surfacePattern);
	const QPixmap *pixmap = static_cast<QPixmap *>(surface->GetPaintDevice());
	GetPainter()->drawTiledPixmap(QRectFromPRect(rc), *pixmap);
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, FillStroke fillStroke)
{
	PenColourWidth(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);
	GetPainter()->drawRoundedRect(QRectFFromPRect(rc), 3.0f, 3.0f);
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke)
{
	QColor qFill = QColorFromColourRGBA(fillStroke.fill.colour);
	QBrush brushFill(qFill);
	GetPainter()->setBrush(brushFill);
	if (fillStroke.fill.colour == fillStroke.stroke.colour) {
		painter->setPen(Qt::NoPen);
		QRectF rect = QRectFFromPRect(rc);
		if (cornerSize > 0.0f) {
			// A radius of 1 shows no curve so add 1
			qreal radius = cornerSize+1;
			GetPainter()->drawRoundedRect(rect, radius, radius);
		} else {
			GetPainter()->fillRect(rect, brushFill);
		}
	} else {
		QColor qOutline = QColorFromColourRGBA(fillStroke.stroke.colour);
		QPen penOutline(qOutline);
		penOutline.setWidthF(fillStroke.stroke.width);
		GetPainter()->setPen(penOutline);

		QRectF rect = QRectFFromPRect(rc.Inset(fillStroke.stroke.width / 2));
		if (cornerSize > 0.0f) {
			// A radius of 1 shows no curve so add 1
			qreal radius = cornerSize+1;
			GetPainter()->drawRoundedRect(rect, radius, radius);
		} else {
			GetPainter()->drawRect(rect);
		}
	}
}

void SurfaceImpl::GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) {
	QRectF rect = QRectFFromPRect(rc);
	QLinearGradient linearGradient;
	switch (options) {
	case GradientOptions::leftToRight:
		linearGradient = QLinearGradient(rc.left, rc.top, rc.right, rc.top);
		break;
	case GradientOptions::topToBottom:
	default:
		linearGradient = QLinearGradient(rc.left, rc.top, rc.left, rc.bottom);
		break;
	}
	linearGradient.setSpread(QGradient::RepeatSpread);
	for (const ColourStop &stop : stops) {
		linearGradient.setColorAt(stop.position, QColorFromColourRGBA(stop.colour));
	}
	QBrush brush = QBrush(linearGradient);
	GetPainter()->fillRect(rect, brush);
}

static std::vector<unsigned char> ImageByteSwapped(int width, int height, const unsigned char *pixelsImage)
{
	// Input is RGBA, but Format_ARGB32 is BGRA, so swap the red bytes and blue bytes
	size_t bytes = width * height * 4;
	std::vector<unsigned char> imageBytes(pixelsImage, pixelsImage+bytes);
	for (size_t i=0; i<bytes; i+=4)
		std::swap(imageBytes[i], imageBytes[i+2]);
	return imageBytes;
}

void SurfaceImpl::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage)
{
	std::vector<unsigned char> imageBytes = ImageByteSwapped(width, height, pixelsImage);
	QImage image(&imageBytes[0], width, height, QImage::Format_ARGB32);
	QPoint pt(rc.left, rc.top);
	GetPainter()->drawImage(pt, image);
}

void SurfaceImpl::Ellipse(PRectangle rc, FillStroke fillStroke)
{
	PenColourWidth(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);
	const QRectF rect = QRectFFromPRect(rc.Inset(fillStroke.stroke.width / 2));
	GetPainter()->drawEllipse(rect);
}

void SurfaceImpl::Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) {
	const XYPOSITION halfStroke = fillStroke.stroke.width / 2.0f;
	const XYPOSITION radius = rc.Height() / 2.0f - halfStroke;
	PRectangle rcInner = rc;
	rcInner.left += radius;
	rcInner.right -= radius;
	const XYPOSITION arcHeight = rc.Height() - fillStroke.stroke.width;

	PenColourWidth(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);

	QPainterPath path;

	const Ends leftSide = static_cast<Ends>(static_cast<unsigned int>(ends) & 0xfu);
	const Ends rightSide = static_cast<Ends>(static_cast<unsigned int>(ends) & 0xf0u);
	switch (leftSide) {
		case Ends::leftFlat:
			path.moveTo(rc.left + halfStroke, rc.top + halfStroke);
			path.lineTo(rc.left + halfStroke, rc.bottom - halfStroke);
			break;
		case Ends::leftAngle:
			path.moveTo(rcInner.left + halfStroke, rc.top + halfStroke);
			path.lineTo(rc.left + halfStroke, rc.Centre().y);
			path.lineTo(rcInner.left + halfStroke, rc.bottom - halfStroke);
			break;
		case Ends::semiCircles:
		default:
			path.moveTo(rcInner.left + halfStroke, rc.top + halfStroke);
			QRectF rectangleArc(rc.left + halfStroke, rc.top + halfStroke,
					    arcHeight, arcHeight);
			path.arcTo(rectangleArc, 90, 180);
			break;
	}

	switch (rightSide) {
		case Ends::rightFlat:
			path.lineTo(rc.right - halfStroke, rc.bottom - halfStroke);
			path.lineTo(rc.right - halfStroke, rc.top + halfStroke);
			break;
		case Ends::rightAngle:
			path.lineTo(rcInner.right - halfStroke, rc.bottom - halfStroke);
			path.lineTo(rc.right - halfStroke, rc.Centre().y);
			path.lineTo(rcInner.right - halfStroke, rc.top + halfStroke);
			break;
		case Ends::semiCircles:
		default:
			path.lineTo(rcInner.right - halfStroke, rc.bottom - halfStroke);
			QRectF rectangleArc(rc.right - arcHeight - halfStroke, rc.top + halfStroke,
					    arcHeight, arcHeight);
			path.arcTo(rectangleArc, 270, 180);
			break;
	}

	// Close the path to enclose it for stroking and for filling, then draw it
	path.closeSubpath();
	GetPainter()->drawPath(path);
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource)
{
	SurfaceImpl *source = dynamic_cast<SurfaceImpl *>(&surfaceSource);
	QPixmap *pixmap = static_cast<QPixmap *>(source->GetPaintDevice());

	GetPainter()->drawPixmap(rc.left, rc.top, *pixmap, from.x, from.y, -1, -1);
}

std::unique_ptr<IScreenLineLayout> SurfaceImpl::Layout(const IScreenLine *)
{
	return {};
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc,
				 const Font *font,
                                 XYPOSITION ybase,
				 std::string_view text,
				 ColourRGBA fore,
				 ColourRGBA back)
{
	SetFont(font);
	PenColour(fore);

	GetPainter()->setBackground(QColorFromColourRGBA(back));
	GetPainter()->setBackgroundMode(Qt::OpaqueMode);
	QString su = UnicodeFromText(codec, text);
	GetPainter()->drawText(QPointF(rc.left, ybase), su);
}

void SurfaceImpl::DrawTextClipped(PRectangle rc,
				  const Font *font,
                                  XYPOSITION ybase,
				  std::string_view text,
				  ColourRGBA fore,
				  ColourRGBA back)
{
	SetClip(rc);
	DrawTextNoClip(rc, font, ybase, text, fore, back);
	PopClip();
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc,
				      const Font *font,
                                      XYPOSITION ybase,
				      std::string_view text,
	ColourRGBA fore)
{
	SetFont(font);
	PenColour(fore);

	GetPainter()->setBackgroundMode(Qt::TransparentMode);
	QString su = UnicodeFromText(codec, text);
	GetPainter()->drawText(QPointF(rc.left, ybase), su);
}

void SurfaceImpl::SetClip(PRectangle rc)
{
	GetPainter()->save();
	GetPainter()->setClipRect(QRectFFromPRect(rc));
}

void SurfaceImpl::PopClip()
{
	GetPainter()->restore();
}

void SurfaceImpl::MeasureWidths(const Font *font,
				std::string_view text,
                                XYPOSITION *positions)
{
	if (!font)
		return;
	SetCodec(font);
	QString su = UnicodeFromText(codec, text);
	QTextLayout tlay(su, *FontPointer(font), GetPaintDevice());
	tlay.beginLayout();
	QTextLine tl = tlay.createLine();
	tlay.endLayout();
	if (mode.codePage == SC_CP_UTF8) {
		int fit = su.size();
		int ui=0;
		size_t i=0;
		while (ui<fit) {
			const unsigned char uch = text[i];
			const unsigned int byteCount = UTF8BytesOfLead[uch];
			const int codeUnits = UTF16LengthFromUTF8ByteCount(byteCount);
			qreal xPosition = tl.cursorToX(ui+codeUnits);
			for (size_t bytePos=0; (bytePos<byteCount) && (i<text.length()); bytePos++) {
				positions[i++] = xPosition;
			}
			ui += codeUnits;
		}
		XYPOSITION lastPos = 0;
		if (i > 0)
			lastPos = positions[i-1];
		while (i<text.length()) {
			positions[i++] = lastPos;
		}
	} else if (mode.codePage) {
		// DBCS
		int ui = 0;
		for (size_t i=0; i<text.length();) {
			size_t lenChar = DBCSIsLeadByte(mode.codePage, text[i]) ? 2 : 1;
			qreal xPosition = tl.cursorToX(ui+1);
			for (unsigned int bytePos=0; (bytePos<lenChar) && (i<text.length()); bytePos++) {
				positions[i++] = xPosition;
			}
			ui++;
		}
	} else {
		// Single byte encoding
		for (int i=0; i<static_cast<int>(text.length()); i++) {
			positions[i] = tl.cursorToX(i+1);
		}
	}
}

XYPOSITION SurfaceImpl::WidthText(const Font *font, std::string_view text)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	SetCodec(font);
	QString su = UnicodeFromText(codec, text);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return metrics.horizontalAdvance(su);
#else
	return metrics.width(su);
#endif
}

void SurfaceImpl::DrawTextNoClipUTF8(PRectangle rc,
				 const Font *font,
				 XYPOSITION ybase,
				 std::string_view text,
				 ColourRGBA fore,
				 ColourRGBA back)
{
	SetFont(font);
	PenColour(fore);

	GetPainter()->setBackground(QColorFromColourRGBA(back));
	GetPainter()->setBackgroundMode(Qt::OpaqueMode);
	QString su = QString::fromUtf8(text.data(), static_cast<int>(text.length()));
	GetPainter()->drawText(QPointF(rc.left, ybase), su);
}

void SurfaceImpl::DrawTextClippedUTF8(PRectangle rc,
				  const Font *font,
				  XYPOSITION ybase,
				  std::string_view text,
				  ColourRGBA fore,
				  ColourRGBA back)
{
	SetClip(rc);
	DrawTextNoClip(rc, font, ybase, text, fore, back);
	PopClip();
}

void SurfaceImpl::DrawTextTransparentUTF8(PRectangle rc,
				      const Font *font,
				      XYPOSITION ybase,
				      std::string_view text,
	ColourRGBA fore)
{
	SetFont(font);
	PenColour(fore);

	GetPainter()->setBackgroundMode(Qt::TransparentMode);
	QString su = QString::fromUtf8(text.data(), static_cast<int>(text.length()));
	GetPainter()->drawText(QPointF(rc.left, ybase), su);
}

void SurfaceImpl::MeasureWidthsUTF8(const Font *font,
				std::string_view text,
				XYPOSITION *positions)
{
	if (!font)
		return;
	QString su = QString::fromUtf8(text.data(), static_cast<int>(text.length()));
	QTextLayout tlay(su, *FontPointer(font), GetPaintDevice());
	tlay.beginLayout();
	QTextLine tl = tlay.createLine();
	tlay.endLayout();
	int fit = su.size();
	int ui=0;
	size_t i=0;
	while (ui<fit) {
		const unsigned char uch = text[i];
		const unsigned int byteCount = UTF8BytesOfLead[uch];
		const int codeUnits = UTF16LengthFromUTF8ByteCount(byteCount);
		qreal xPosition = tl.cursorToX(ui+codeUnits);
		for (size_t bytePos=0; (bytePos<byteCount) && (i<text.length()); bytePos++) {
			positions[i++] = xPosition;
		}
		ui += codeUnits;
	}
	XYPOSITION lastPos = 0;
	if (i > 0)
		lastPos = positions[i-1];
	while (i<text.length()) {
		positions[i++] = lastPos;
	}
}

XYPOSITION SurfaceImpl::WidthTextUTF8(const Font *font, std::string_view text)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	QString su = QString::fromUtf8(text.data(), static_cast<int>(text.length()));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	return metrics.horizontalAdvance(su);
#else
	return metrics.width(su);
#endif
}

XYPOSITION SurfaceImpl::Ascent(const Font *font)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	return metrics.ascent();
}

XYPOSITION SurfaceImpl::Descent(const Font *font)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	// Qt returns 1 less than true descent
	// See: QFontEngineWin::descent which says:
	// ### we subtract 1 to even out the historical +1 in QFontMetrics's
	// ### height=asc+desc+1 equation. Fix in Qt5.
	return metrics.descent() + 1;
}

XYPOSITION SurfaceImpl::InternalLeading(const Font * /* font */)
{
	return 0;
}

XYPOSITION SurfaceImpl::Height(const Font *font)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	return metrics.height();
}

XYPOSITION SurfaceImpl::AverageCharWidth(const Font *font)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	return metrics.averageCharWidth();
}

void SurfaceImpl::FlushCachedState()
{
	if (device->paintingActive()) {
		GetPainter()->setPen(QPen());
		GetPainter()->setBrush(QBrush());
	}
}

void SurfaceImpl::FlushDrawing()
{
}

QPaintDevice *SurfaceImpl::GetPaintDevice()
{
	return device;
}

QPainter *SurfaceImpl::GetPainter()
{
	Q_ASSERT(device);
	if (!painter) {
		if (device->paintingActive()) {
			painter = device->paintEngine()->painter();
		} else {
			painterOwned = true;
			painter = new QPainter(device);
		}

		// Set text antialiasing unconditionally.
		// The font's style strategy will override.
		painter->setRenderHint(QPainter::TextAntialiasing, true);

		painter->setRenderHint(QPainter::Antialiasing, true);
	}

	return painter;
}

std::unique_ptr<Surface> Surface::Allocate(Technology)
{
	return std::make_unique<SurfaceImpl>();
}


//----------------------------------------------------------------------

namespace {

QWidget *window(WindowID wid) noexcept
{
	return static_cast<QWidget *>(wid);
}

QRect ScreenRectangleForPoint(QPoint posGlobal)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	const QScreen *screen = QGuiApplication::screenAt(posGlobal);
	return screen->availableGeometry();
#else
	const QDesktopWidget *desktop = QApplication::desktop();
	return desktop->availableGeometry(posGlobal);
#endif
}

}

Window::~Window() noexcept = default;

void Window::Destroy() noexcept
{
	if (wid)
		delete window(wid);
	wid = nullptr;
}
PRectangle Window::GetPosition() const
{
	// Before any size allocated pretend its 1000 wide so not scrolled
	return wid ? PRectFromQRect(window(wid)->frameGeometry()) : PRectangle(0, 0, 1000, 1000);
}

void Window::SetPosition(PRectangle rc)
{
	if (wid)
		window(wid)->setGeometry(QRectFromPRect(rc));
}

void Window::SetPositionRelative(PRectangle rc, const Window *relativeTo)
{
	QPoint oPos = window(relativeTo->wid)->mapToGlobal(QPoint(0,0));
	int ox = oPos.x();
	int oy = oPos.y();
	ox += rc.left;
	oy += rc.top;

	const QRect rectDesk = ScreenRectangleForPoint(QPoint(ox, oy));
	/* do some corrections to fit into screen */
	int sizex = rc.right - rc.left;
	int sizey = rc.bottom - rc.top;
	int screenWidth = rectDesk.width();
	if (ox < rectDesk.x())
		ox = rectDesk.x();
	if (sizex > screenWidth)
		ox = rectDesk.x(); /* the best we can do */
	else if (ox + sizex > rectDesk.right())
		ox = rectDesk.right() - sizex;
	if (oy + sizey > rectDesk.bottom())
		oy = rectDesk.bottom() - sizey;

	Q_ASSERT(wid);
	window(wid)->move(ox, oy);
	window(wid)->resize(sizex, sizey);
}

PRectangle Window::GetClientPosition() const
{
	// The client position is the window position
	return GetPosition();
}

void Window::Show(bool show)
{
	if (wid)
		window(wid)->setVisible(show);
}

void Window::InvalidateAll()
{
	if (wid)
		window(wid)->update();
}

void Window::InvalidateRectangle(PRectangle rc)
{
	if (wid)
		window(wid)->update(QRectFromPRect(rc));
}

void Window::SetCursor(Cursor curs)
{
	if (wid) {
		Qt::CursorShape shape;

		switch (curs) {
			case Cursor::text:  shape = Qt::IBeamCursor;        break;
			case Cursor::arrow: shape = Qt::ArrowCursor;        break;
			case Cursor::up:    shape = Qt::UpArrowCursor;      break;
			case Cursor::wait:  shape = Qt::WaitCursor;         break;
			case Cursor::horizontal: shape = Qt::SizeHorCursor; break;
			case Cursor::vertical:  shape = Qt::SizeVerCursor;  break;
			case Cursor::hand:  shape = Qt::PointingHandCursor; break;
			default:            shape = Qt::ArrowCursor;        break;
		}

		QCursor cursor = QCursor(shape);

		if (curs != cursorLast) {
			window(wid)->setCursor(cursor);
			cursorLast = curs;
		}
	}
}

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   window coordinates */
PRectangle Window::GetMonitorRect(Point pt)
{
	const QPoint posGlobal = window(wid)->mapToGlobal(QPoint(pt.x, pt.y));
	const QPoint originGlobal = window(wid)->mapToGlobal(QPoint(0, 0));
	QRect rectScreen = ScreenRectangleForPoint(posGlobal);
	rectScreen.translate(-originGlobal.x(), -originGlobal.y());
	return PRectFromQRect(rectScreen);
}

//----------------------------------------------------------------------
class ListWidget : public QListWidget {
public:
	explicit ListWidget(QWidget *parent);

	void setDelegate(IListBoxDelegate *lbDelegate);

	int currentSelection();

protected:
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	void initViewItemOption(QStyleOptionViewItem *option) const override;
#else
	QStyleOptionViewItem viewOptions() const override;
#endif

private:
	IListBoxDelegate *delegate;
};

class ListBoxImpl : public ListBox {
public:
	ListBoxImpl() noexcept;

	void SetFont(const Font *font) override;
	void Create(Window &parent, int ctrlID, Point location,
						int lineHeight, bool unicodeMode_, Technology technology) override;
	void SetAverageCharWidth(int width) override;
	void SetVisibleRows(int rows) override;
	int GetVisibleRows() const override;
	PRectangle GetDesiredRect() override;
	int CaretFromEdge() override;
	void Clear() noexcept override;
	void Append(char *s, int type) override;
	int Length() override;
	void Select(int n) override;
	int GetSelection() override;
	int Find(const char *prefix) override;
	std::string GetValue(int n) override;
	void RegisterImage(int type, const char *xpmData) override;
	void RegisterRGBAImage(int type, int width, int height,
		const unsigned char *pixelsImage) override;
	virtual void RegisterQPixmapImage(int type, const QPixmap& pm);
	void ClearRegisteredImages() override;
	void SetDelegate(IListBoxDelegate *lbDelegate) override;
	void SetList(const char *list, char separator, char typesep) override;
	void SetOptions(ListOptions options_) override;

	[[nodiscard]] ListWidget *GetWidget() const noexcept;
private:
	bool unicodeMode{false};
	int visibleRows{5};
	QMap<int,QPixmap> images;
};
ListBoxImpl::ListBoxImpl() noexcept = default;

void ListBoxImpl::Create(Window &parent,
                         int /*ctrlID*/,
                         Point location,
                         int /*lineHeight*/,
                         bool unicodeMode_,
			 Technology)
{
	unicodeMode = unicodeMode_;

	QWidget *qparent = static_cast<QWidget *>(parent.GetID());
	ListWidget *list = new ListWidget(qparent);

#if defined(Q_OS_WIN)
	// On Windows, Qt::ToolTip causes a crash when the list is clicked on
	// so Qt::Tool is used.
	list->setParent(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		| Qt::WindowDoesNotAcceptFocus
#endif
	);
#else
	// On macOS, Qt::Tool takes focus so main window loses focus so
	// keyboard stops working. Qt::ToolTip works but its only really
	// documented for tooltips.
	// On Linux / X this setting allows clicking on list items.
	list->setParent(nullptr, static_cast<Qt::WindowFlags>(Qt::ToolTip | Qt::FramelessWindowHint));
#endif
	list->setAttribute(Qt::WA_ShowWithoutActivating);
	list->setFocusPolicy(Qt::NoFocus);
	list->setUniformItemSizes(true);
	list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	list->move(location.x, location.y);

	int maxIconWidth = 0;
	int maxIconHeight = 0;
	foreach (QPixmap im, images) {
		if (maxIconWidth < im.width())
			maxIconWidth = im.width();
		if (maxIconHeight < im.height())
			maxIconHeight = im.height();
	}
	list->setIconSize(QSize(maxIconWidth, maxIconHeight));

	wid = list;
}
void ListBoxImpl::SetFont(const Font *font)
{
	ListWidget *list = GetWidget();
	const FontAndCharacterSet *pfacs = AsFontAndCharacterSet(font);
	if (pfacs && pfacs->pfont) {
		list->setFont(*(pfacs->pfont));
	}
}
void ListBoxImpl::SetAverageCharWidth(int /*width*/) {}

void ListBoxImpl::SetVisibleRows(int rows)
{
	visibleRows = rows;
}

int ListBoxImpl::GetVisibleRows() const
{
	return visibleRows;
}
PRectangle ListBoxImpl::GetDesiredRect()
{
	ListWidget *list = GetWidget();
	int rows = Length();
	if (rows == 0 || rows > visibleRows) {
		rows = visibleRows;
	}
	int rowHeight = list->sizeHintForRow(0);
	int height = (rows * rowHeight) + (2 * list->frameWidth());

	QStyle *style = QApplication::style();
	int width = list->sizeHintForColumn(0) + (2 * list->frameWidth());
	if (Length() > rows) {
		width += style->pixelMetric(QStyle::PM_ScrollBarExtent);
	}

	return PRectangle(0, 0, width, height);
}
int ListBoxImpl::CaretFromEdge()
{
	ListWidget *list = GetWidget();
	int maxIconWidth = 0;
	foreach (QPixmap im, images) {
		if (maxIconWidth < im.width())
			maxIconWidth = im.width();
	}

	int extra;
	// The 12 is from trial and error on macOS and the 7
	// is from trial and error on Windows - there may be
	// a better programmatic way to find any padding factors.
#ifdef Q_OS_DARWIN
	extra = 12;
#else
	extra = 7;
#endif
	return maxIconWidth + (2 * list->frameWidth()) + extra;
}
void ListBoxImpl::Clear() noexcept
{
	ListWidget *list = GetWidget();
	list->clear();
}
void ListBoxImpl::Append(char *s, int type)
{
	ListWidget *list = GetWidget();
	QString str = unicodeMode ? QString::fromUtf8(s) : QString::fromLocal8Bit(s);
	QIcon icon;
	if (type >= 0) {
		Q_ASSERT(images.contains(type));
		icon = images.value(type);
	}
	new QListWidgetItem(icon, str, list);
}
int ListBoxImpl::Length()
{
	ListWidget *list = GetWidget();
	return list->count();
}
void ListBoxImpl::Select(int n)
{
	ListWidget *list = GetWidget();
	QModelIndex index = list->model()->index(n, 0);
	if (index.isValid()) {
		QRect row_rect = list->visualRect(index);
		if (!list->viewport()->rect().contains(row_rect)) {
			list->scrollTo(index, QAbstractItemView::PositionAtTop);
		}
	}
	list->setCurrentRow(n);
}
int ListBoxImpl::GetSelection()
{
	ListWidget *list = GetWidget();
	return list->currentSelection();
}
int ListBoxImpl::Find(const char *prefix)
{
	ListWidget *list = GetWidget();
	QString sPrefix = unicodeMode ? QString::fromUtf8(prefix) : QString::fromLocal8Bit(prefix);
	QList<QListWidgetItem *> ms = list->findItems(sPrefix, Qt::MatchStartsWith);
	int result = -1;
	if (!ms.isEmpty()) {
		result = list->row(ms.first());
	}

	return result;
}
std::string ListBoxImpl::GetValue(int n)
{
	ListWidget *list = GetWidget();
	QListWidgetItem *item = list->item(n);
	QString str = item->data(Qt::DisplayRole).toString();
	QByteArray bytes = unicodeMode ? str.toUtf8() : str.toLocal8Bit();
	return std::string(bytes.constData());
}

void ListBoxImpl::RegisterQPixmapImage(int type, const QPixmap& pm)
{
	images[type] = pm;
	ListWidget *list = GetWidget();
	if (list) {
		QSize iconSize = list->iconSize();
		if (pm.width() > iconSize.width() || pm.height() > iconSize.height())
			list->setIconSize(QSize(qMax(pm.width(), iconSize.width()),
						 qMax(pm.height(), iconSize.height())));
	}

}

void ListBoxImpl::RegisterImage(int type, const char *xpmData)
{
	RegisterQPixmapImage(type, QPixmap(reinterpret_cast<const char * const *>(xpmData)));
}

void ListBoxImpl::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage)
{
	std::vector<unsigned char> imageBytes = ImageByteSwapped(width, height, pixelsImage);
	QImage image(&imageBytes[0], width, height, QImage::Format_ARGB32);
	RegisterQPixmapImage(type, QPixmap::fromImage(image));
}

void ListBoxImpl::ClearRegisteredImages()
{
	images.clear();
	ListWidget *list = GetWidget();
	if (list)
		list->setIconSize(QSize(0, 0));
}
void ListBoxImpl::SetDelegate(IListBoxDelegate *lbDelegate)
{
	ListWidget *list = GetWidget();
	list->setDelegate(lbDelegate);
}
void ListBoxImpl::SetList(const char *list, char separator, char typesep)
{
	// This method is *not* platform dependent.
	// It is borrowed from the GTK implementation.
	Clear();
	size_t count = strlen(list) + 1;
	std::vector<char> words(list, list+count);
	char *startword = &words[0];
	char *numword = nullptr;
	int i = 0;
	for (; words[i]; i++) {
		if (words[i] == separator) {
			words[i] = '\0';
			if (numword)
				*numword = '\0';
			Append(startword, numword?atoi(numword + 1):-1);
			startword = &words[0] + i + 1;
			numword = nullptr;
		} else if (words[i] == typesep) {
			numword = &words[0] + i;
		}
	}
	if (startword) {
		if (numword)
			*numword = '\0';
		Append(startword, numword?atoi(numword + 1):-1);
	}
}
void ListBoxImpl::SetOptions(ListOptions)
{
}
ListWidget *ListBoxImpl::GetWidget() const noexcept
{
	return static_cast<ListWidget *>(wid);
}

ListBox::ListBox() noexcept = default;
ListBox::~ListBox() noexcept = default;

std::unique_ptr<ListBox> ListBox::Allocate()
{
	return std::make_unique<ListBoxImpl>();
}
ListWidget::ListWidget(QWidget *parent)
: QListWidget(parent), delegate(nullptr)
{}

void ListWidget::setDelegate(IListBoxDelegate *lbDelegate)
{
	delegate = lbDelegate;
}

void ListWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
	QListWidget::selectionChanged(selected, deselected);
	if (delegate) {
		const int selection = currentSelection();
		if (selection >= 0) {
			ListBoxEvent event(ListBoxEvent::EventType::selectionChange);
			delegate->ListNotify(&event);
		}
	}
}

int ListWidget::currentSelection() {
	const QModelIndexList indices = selectionModel()->selectedRows();
	foreach (const QModelIndex ind, indices) {
		return ind.row();
	}
	return -1;
}

void ListWidget::mouseDoubleClickEvent(QMouseEvent * /* event */)
{
	if (delegate) {
		ListBoxEvent event(ListBoxEvent::EventType::doubleClick);
		delegate->ListNotify(&event);
	}
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ListWidget::initViewItemOption(QStyleOptionViewItem *option) const
{
	QListWidget::initViewItemOption(option);
	option->state |= QStyle::State_Active;
}
#else
QStyleOptionViewItem ListWidget::viewOptions() const
{
	QStyleOptionViewItem result = QListWidget::viewOptions();
	result.state |= QStyle::State_Active;
	return result;
}
#endif
//----------------------------------------------------------------------
Menu::Menu() noexcept : mid(nullptr) {}
void Menu::CreatePopUp()
{
	Destroy();
	mid = new QMenu();
}

void Menu::Destroy() noexcept
{
	if (mid) {
		QMenu *menu = static_cast<QMenu *>(mid);
		delete menu;
	}
	mid = nullptr;
}
void Menu::Show(Point pt, const Window & /*w*/)
{
	QMenu *menu = static_cast<QMenu *>(mid);
	menu->exec(QPoint(pt.x, pt.y));
	Destroy();
}

//----------------------------------------------------------------------

ColourRGBA Platform::Chrome()
{
	QColor c(Qt::gray);
	return ColourRGBA(c.red(), c.green(), c.blue());
}

ColourRGBA Platform::ChromeHighlight()
{
	QColor c(Qt::lightGray);
	return ColourRGBA(c.red(), c.green(), c.blue());
}

const char *Platform::DefaultFont()
{
	static char fontNameDefault[200] = "";
	if (!fontNameDefault[0]) {
		QFont font = QApplication::font();
		strcpy(fontNameDefault, font.family().toUtf8());
	}
	return fontNameDefault;
}

int Platform::DefaultFontSize()
{
	QFont font = QApplication::font();
	return font.pointSize();
}

unsigned int Platform::DoubleClickTime()
{
	return QApplication::doubleClickInterval();
}

void Platform::DebugDisplay(const char *s) noexcept
{
	qWarning("Scintilla: %s", s);
}

void Platform::DebugPrintf(const char *format, ...) noexcept
{
	char buffer[2000];
	va_list pArguments{};
	va_start(pArguments, format);
	vsprintf(buffer, format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}

bool Platform::ShowAssertionPopUps(bool /*assertionPopUps*/) noexcept
{
	return false;
}

void Platform::Assert(const char *c, const char *file, int line) noexcept
{
	char buffer[2000];
	sprintf(buffer, "Assertion [%s] failed at %s %d", c, file, line);
	if (Platform::ShowAssertionPopUps(false)) {
		QMessageBox mb("Assertion Failure", buffer, QMessageBox::NoIcon,
			QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
		mb.exec();
	} else {
		strcat(buffer, "\n");
		Platform::DebugDisplay(buffer);
	}
}

}
