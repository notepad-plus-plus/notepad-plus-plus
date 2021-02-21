//
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
#include "FontQuality.h"

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
#include <QMenu>
#include <QAction>
#include <QTime>
#include <QMessageBox>
#include <QTextCodec>
#include <QListWidget>
#include <QVarLengthArray>
#include <QScrollBar>
#include <QDesktopWidget>
#include <QTextLayout>
#include <QTextLine>
#include <QLibrary>

namespace Scintilla {

//----------------------------------------------------------------------

// Convert from a Scintilla characterSet value to a Qt codec name.
const char *CharacterSetID(int characterSet)
{
	switch (characterSet) {
		//case SC_CHARSET_ANSI:
		//	return "";
	case SC_CHARSET_DEFAULT:
		return "ISO 8859-1";
	case SC_CHARSET_BALTIC:
		return "ISO 8859-13";
	case SC_CHARSET_CHINESEBIG5:
		return "Big5";
	case SC_CHARSET_EASTEUROPE:
		return "ISO 8859-2";
	case SC_CHARSET_GB2312:
		return "GB18030-0";
	case SC_CHARSET_GREEK:
		return "ISO 8859-7";
	case SC_CHARSET_HANGUL:
		return "CP949";
	case SC_CHARSET_MAC:
		return "Apple Roman";
		//case SC_CHARSET_OEM:
		//	return "ASCII";
	case SC_CHARSET_RUSSIAN:
		return "KOI8-R";
	case SC_CHARSET_CYRILLIC:
		return "Windows-1251";
	case SC_CHARSET_SHIFTJIS:
		return "Shift-JIS";
		//case SC_CHARSET_SYMBOL:
		//        return "";
	case SC_CHARSET_TURKISH:
		return "ISO 8859-9";
		//case SC_CHARSET_JOHAB:
		//        return "CP1361";
	case SC_CHARSET_HEBREW:
		return "ISO 8859-8";
	case SC_CHARSET_ARABIC:
		return "ISO 8859-6";
	case SC_CHARSET_VIETNAMESE:
		return "Windows-1258";
	case SC_CHARSET_THAI:
		return "TIS-620";
	case SC_CHARSET_8859_15:
		return "ISO 8859-15";
	default:
		return "ISO 8859-1";
	}
}

QString UnicodeFromText(QTextCodec *codec, std::string_view text) {
	return codec->toUnicode(text.data(), static_cast<int>(text.length()));
}

class FontAndCharacterSet {
public:
	int characterSet;
	QFont *pfont;
	FontAndCharacterSet(int characterSet_, QFont *pfont):
		characterSet(characterSet_), pfont(pfont) {
	}
	~FontAndCharacterSet() {
		delete pfont;
		pfont = nullptr;
	}
};

namespace {

FontAndCharacterSet *AsFontAndCharacterSet(const Font &f) {
	return reinterpret_cast<FontAndCharacterSet *>(f.GetID());
}

int FontCharacterSet(const Font &f)
{
	return AsFontAndCharacterSet(f)->characterSet;
}

QFont *FontPointer(const Font &f)
{
	return AsFontAndCharacterSet(f)->pfont;
}

}

Font::Font() noexcept : fid(nullptr) {}
Font::~Font()
{
	delete reinterpret_cast<FontAndCharacterSet *>(fid);
	fid = nullptr;
}
static QFont::StyleStrategy ChooseStrategy(int eff)
{
	switch (eff) {
		case SC_EFF_QUALITY_DEFAULT:         return QFont::PreferDefault;
		case SC_EFF_QUALITY_NON_ANTIALIASED: return QFont::NoAntialias;
		case SC_EFF_QUALITY_ANTIALIASED:     return QFont::PreferAntialias;
		case SC_EFF_QUALITY_LCD_OPTIMIZED:   return QFont::PreferAntialias;
		default:                             return QFont::PreferDefault;
	}
}

void Font::Create(const FontParameters &fp)
{
	Release();

	QFont *font = new QFont;
	font->setStyleStrategy(ChooseStrategy(fp.extraFontFlag));
	font->setFamily(QString::fromUtf8(fp.faceName));
	font->setPointSizeF(fp.size);
	font->setBold(fp.weight > 500);
	font->setItalic(fp.italic);

	fid = new FontAndCharacterSet(fp.characterSet, font);
}

void Font::Release()
{
	if (fid)
		delete reinterpret_cast<FontAndCharacterSet *>(fid);
	fid = nullptr;
}
SurfaceImpl::SurfaceImpl()
: device(nullptr), painter(nullptr), deviceOwned(false), painterOwned(false), x(0), y(0),
	  unicodeMode(false), codePage(0), codecName(nullptr), codec(nullptr)
{}
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

void SurfaceImpl::InitPixMap(int width,
        int height,
        Surface *surface,
        WindowID /*wid*/)
{
	Release();
	if (width < 1) width = 1;
	if (height < 1) height = 1;
	deviceOwned = true;
	device = new QPixmap(width, height);
	SurfaceImpl *psurfOther = dynamic_cast<SurfaceImpl *>(surface);
	SetUnicodeMode(psurfOther->unicodeMode);
	SetDBCSMode(psurfOther->codePage);
}

void SurfaceImpl::Release()
{
	Clear();
}

bool SurfaceImpl::Initialised()
{
	return device != nullptr;
}

void SurfaceImpl::PenColour(ColourDesired fore)
{
	QPen penOutline(QColorFromCA(fore));
	penOutline.setCapStyle(Qt::FlatCap);
	GetPainter()->setPen(penOutline);
}

void SurfaceImpl::BrushColour(ColourDesired back)
{
	GetPainter()->setBrush(QBrush(QColorFromCA(back)));
}

void SurfaceImpl::SetCodec(const Font &font)
{
	if (font.GetID()) {
		const char *csid = "UTF-8";
		if (!unicodeMode)
			csid = CharacterSetID(FontCharacterSet(font));
		if (csid != codecName) {
			codecName = csid;
			codec = QTextCodec::codecForName(csid);
		}
	}
}

void SurfaceImpl::SetFont(const Font &font)
{
	if (font.GetID()) {
		GetPainter()->setFont(*FontPointer(font));
		SetCodec(font);
	}
}

int SurfaceImpl::LogPixelsY()
{
	return device->logicalDpiY();
}

int SurfaceImpl::DeviceHeightFont(int points)
{
	return points;
}

void SurfaceImpl::MoveTo(int x_, int y_)
{
	x = x_;
	y = y_;
}

void SurfaceImpl::LineTo(int x_, int y_)
{
	QLineF line(x, y, x_, y_);
	GetPainter()->drawLine(line);
	x = x_;
	y = y_;
}

void SurfaceImpl::Polygon(Point *pts,
			  size_t npts,
                          ColourDesired fore,
                          ColourDesired back)
{
	PenColour(fore);
	BrushColour(back);

	std::vector<QPoint> qpts(npts);
	for (size_t i = 0; i < npts; i++) {
		qpts[i] = QPoint(pts[i].x, pts[i].y);
	}

	GetPainter()->drawPolygon(&qpts[0], static_cast<int>(npts));
}

void SurfaceImpl::RectangleDraw(PRectangle rc,
                                ColourDesired fore,
                                ColourDesired back)
{
	PenColour(fore);
	BrushColour(back);
	QRectF rect(rc.left, rc.top, rc.Width() - 1, rc.Height() - 1);
	GetPainter()->drawRect(rect);
}

void SurfaceImpl::FillRectangle(PRectangle rc, ColourDesired back)
{
	GetPainter()->fillRect(QRectFFromPRect(rc), QColorFromCA(back));
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern)
{
	// Tile pattern over rectangle
	SurfaceImpl *surface = dynamic_cast<SurfaceImpl *>(&surfacePattern);
	// Currently assumes 8x8 pattern
	int widthPat = 8;
	int heightPat = 8;
	for (int xTile = rc.left; xTile < rc.right; xTile += widthPat) {
		int widthx = (xTile + widthPat > rc.right) ? rc.right - xTile : widthPat;
		for (int yTile = rc.top; yTile < rc.bottom; yTile += heightPat) {
			int heighty = (yTile + heightPat > rc.bottom) ? rc.bottom - yTile : heightPat;
			QRect source(0, 0, widthx, heighty);
			QRect target(xTile, yTile, widthx, heighty);
			QPixmap *pixmap = static_cast<QPixmap *>(surface->GetPaintDevice());
			GetPainter()->drawPixmap(target, *pixmap, source);
		}
	}
}

void SurfaceImpl::RoundedRectangle(PRectangle rc,
                                   ColourDesired fore,
                                   ColourDesired back)
{
	PenColour(fore);
	BrushColour(back);
	GetPainter()->drawRoundedRect(QRectFFromPRect(RectangleInset(rc, 0.5f)), 3.0f, 3.0f);
}

void SurfaceImpl::AlphaRectangle(PRectangle rc,
                                 int cornerSize,
                                 ColourDesired fill,
                                 int alphaFill,
                                 ColourDesired outline,
                                 int alphaOutline,
                                 int /*flags*/)
{
	const QColor qFill = QColorFromColourAlpha(ColourAlpha(fill, alphaFill));
	const QBrush brushFill(qFill);
	GetPainter()->setBrush(brushFill);

	if ((fill == outline) && (alphaFill == alphaOutline)) {
		painter->setPen(Qt::NoPen);
		const QRectF rect = QRectFFromPRect(rc);
		if (cornerSize > 0.0f) {
			// A radius of 1 shows no curve so add 1
			qreal radius = cornerSize+1;
			GetPainter()->drawRoundedRect(rect, radius, radius);
		} else {
			GetPainter()->fillRect(rect, brushFill);
		}
	} else {
		const QColor qOutline = QColorFromColourAlpha(ColourAlpha(outline, alphaOutline));
		const QPen penOutline(qOutline);
		GetPainter()->setPen(penOutline);

		const QRectF rect(rc.left, rc.top, rc.Width() - 1.5, rc.Height() - 1.5);
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
		linearGradient.setColorAt(stop.position, QColorFromColourAlpha(stop.colour));
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

void SurfaceImpl::Ellipse(PRectangle rc,
                          ColourDesired fore,
                          ColourDesired back)
{
	PenColour(fore);
	BrushColour(back);
	GetPainter()->drawEllipse(QRectFFromPRect(rc));
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
                                 Font &font,
                                 XYPOSITION ybase,
				 std::string_view text,
                                 ColourDesired fore,
                                 ColourDesired back)
{
	SetFont(font);
	PenColour(fore);

	GetPainter()->setBackground(QColorFromCA(back));
	GetPainter()->setBackgroundMode(Qt::OpaqueMode);
	QString su = UnicodeFromText(codec, text);
	GetPainter()->drawText(QPointF(rc.left, ybase), su);
}

void SurfaceImpl::DrawTextClipped(PRectangle rc,
                                  Font &font,
                                  XYPOSITION ybase,
				  std::string_view text,
                                  ColourDesired fore,
                                  ColourDesired back)
{
	SetClip(rc);
	DrawTextNoClip(rc, font, ybase, text, fore, back);
	GetPainter()->setClipping(false);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc,
                                      Font &font,
                                      XYPOSITION ybase,
				      std::string_view text,
        ColourDesired fore)
{
	SetFont(font);
	PenColour(fore);

	GetPainter()->setBackgroundMode(Qt::TransparentMode);
	QString su = UnicodeFromText(codec, text);
	GetPainter()->drawText(QPointF(rc.left, ybase), su);
}

void SurfaceImpl::SetClip(PRectangle rc)
{
	GetPainter()->setClipRect(QRectFFromPRect(rc));
}

void SurfaceImpl::MeasureWidths(Font &font,
				std::string_view text,
                                XYPOSITION *positions)
{
	if (!font.GetID())
		return;
	SetCodec(font);
	QString su = UnicodeFromText(codec, text);
	QTextLayout tlay(su, *FontPointer(font), GetPaintDevice());
	tlay.beginLayout();
	QTextLine tl = tlay.createLine();
	tlay.endLayout();
	if (unicodeMode) {
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
	} else if (codePage) {
		// DBCS
		int ui = 0;
		for (size_t i=0; i<text.length();) {
			size_t lenChar = DBCSIsLeadByte(codePage, text[i]) ? 2 : 1;
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

XYPOSITION SurfaceImpl::WidthText(Font &font, std::string_view text)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	SetCodec(font);
	QString su = UnicodeFromText(codec, text);
	return metrics.width(su);
}

XYPOSITION SurfaceImpl::Ascent(Font &font)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	return metrics.ascent();
}

XYPOSITION SurfaceImpl::Descent(Font &font)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	// Qt returns 1 less than true descent
	// See: QFontEngineWin::descent which says:
	// ### we subtract 1 to even out the historical +1 in QFontMetrics's
	// ### height=asc+desc+1 equation. Fix in Qt5.
	return metrics.descent() + 1;
}

XYPOSITION SurfaceImpl::InternalLeading(Font & /* font */)
{
	return 0;
}

XYPOSITION SurfaceImpl::Height(Font &font)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	return metrics.height();
}

XYPOSITION SurfaceImpl::AverageCharWidth(Font &font)
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

void SurfaceImpl::SetUnicodeMode(bool unicodeMode_)
{
	unicodeMode=unicodeMode_;
}

void SurfaceImpl::SetDBCSMode(int codePage_)
{
	codePage = codePage_;
}

void SurfaceImpl::SetBidiR2L(bool)
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
	}

	return painter;
}

Surface *Surface::Allocate(int)
{
	return new SurfaceImpl;
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

Window::~Window() {}

void Window::Destroy()
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

void Window::SetFont(Font &font)
{
	if (wid)
		window(wid)->setFont(*FontPointer(font));
}

void Window::SetCursor(Cursor curs)
{
	if (wid) {
		Qt::CursorShape shape;

		switch (curs) {
			case cursorText:  shape = Qt::IBeamCursor;        break;
			case cursorArrow: shape = Qt::ArrowCursor;        break;
			case cursorUp:    shape = Qt::UpArrowCursor;      break;
			case cursorWait:  shape = Qt::WaitCursor;         break;
			case cursorHoriz: shape = Qt::SizeHorCursor;      break;
			case cursorVert:  shape = Qt::SizeVerCursor;      break;
			case cursorHand:  shape = Qt::PointingHandCursor; break;
			default:          shape = Qt::ArrowCursor;        break;
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
	virtual ~ListWidget();

	void setDelegate(IListBoxDelegate *lbDelegate);

	int currentSelection();

protected:
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	QStyleOptionViewItem viewOptions() const override;

private:
	IListBoxDelegate *delegate;
};

class ListBoxImpl : public ListBox {
public:
	ListBoxImpl();
	~ListBoxImpl();

	void SetFont(Font &font) override;
	void Create(Window &parent, int ctrlID, Point location,
						int lineHeight, bool unicodeMode_, int technology) override;
	void SetAverageCharWidth(int width) override;
	void SetVisibleRows(int rows) override;
	int GetVisibleRows() const override;
	PRectangle GetDesiredRect() override;
	int CaretFromEdge() override;
	void Clear() override;
	void Append(char *s, int type = -1) override;
	int Length() override;
	void Select(int n) override;
	int GetSelection() override;
	int Find(const char *prefix) override;
	void GetValue(int n, char *value, int len) override;
	void RegisterImage(int type, const char *xpmData) override;
	void RegisterRGBAImage(int type, int width, int height,
		const unsigned char *pixelsImage) override;
	virtual void RegisterQPixmapImage(int type, const QPixmap& pm);
	void ClearRegisteredImages() override;
	void SetDelegate(IListBoxDelegate *lbDelegate) override;
	void SetList(const char *list, char separator, char typesep) override;

	ListWidget *GetWidget() const noexcept;
private:
	bool unicodeMode;
	int visibleRows;
	QMap<int,QPixmap> images;
};
ListBoxImpl::ListBoxImpl()
: unicodeMode(false), visibleRows(5)
{}

ListBoxImpl::~ListBoxImpl() {}

void ListBoxImpl::Create(Window &parent,
                         int /*ctrlID*/,
                         Point location,
                         int /*lineHeight*/,
                         bool unicodeMode_,
			 int)
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
	// On OS X, Qt::Tool takes focus so main window loses focus so
	// keyboard stops working. Qt::ToolTip works but its only really
	// documented for tooltips.
	// On Linux / X this setting allows clicking on list items.
	list->setParent(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
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
void ListBoxImpl::SetFont(Font &font)
{
	ListWidget *list = GetWidget();
	list->setFont(*FontPointer(font));
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
	// The 12 is from trial and error on OS X and the 7
	// is from trial and error on Windows - there may be
	// a better programmatic way to find any padding factors.
#ifdef Q_OS_DARWIN
	extra = 12;
#else
	extra = 7;
#endif
	return maxIconWidth + (2 * list->frameWidth()) + extra;
}
void ListBoxImpl::Clear()
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
void ListBoxImpl::GetValue(int n, char *value, int len)
{
	ListWidget *list = GetWidget();
	QListWidgetItem *item = list->item(n);
	QString str = item->data(Qt::DisplayRole).toString();
	QByteArray bytes = unicodeMode ? str.toUtf8() : str.toLocal8Bit();

	strncpy(value, bytes.constData(), len);
	value[len-1] = '\0';
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
ListWidget *ListBoxImpl::GetWidget() const noexcept
{
	return static_cast<ListWidget *>(wid);
}

ListBox::ListBox() noexcept {}
ListBox::~ListBox() {}

ListBox *ListBox::Allocate()
{
	return new ListBoxImpl();
}
ListWidget::ListWidget(QWidget *parent)
: QListWidget(parent), delegate(nullptr)
{}
ListWidget::~ListWidget() {}

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

QStyleOptionViewItem ListWidget::viewOptions() const
{
	QStyleOptionViewItem result = QListWidget::viewOptions();
	result.state |= QStyle::State_Active;
	return result;
}
//----------------------------------------------------------------------
Menu::Menu() noexcept : mid(nullptr) {}
void Menu::CreatePopUp()
{
	Destroy();
	mid = new QMenu();
}

void Menu::Destroy()
{
	if (mid) {
		QMenu *menu = static_cast<QMenu *>(mid);
		delete menu;
	}
	mid = nullptr;
}
void Menu::Show(Point pt, Window & /*w*/)
{
	QMenu *menu = static_cast<QMenu *>(mid);
	menu->exec(QPoint(pt.x, pt.y));
	Destroy();
}

//----------------------------------------------------------------------

class DynamicLibraryImpl : public DynamicLibrary {
protected:
	QLibrary *lib;
public:
	explicit DynamicLibraryImpl(const char *modulePath) {
		QString path = QString::fromUtf8(modulePath);
		lib = new QLibrary(path);
	}
	// Deleted so DynamicLibraryImpl objects can not be copied
	DynamicLibraryImpl(const DynamicLibraryImpl &) = delete;
	DynamicLibraryImpl(DynamicLibraryImpl &&) = delete;
	DynamicLibraryImpl &operator=(const DynamicLibraryImpl &) = delete;
	DynamicLibraryImpl &operator=(DynamicLibraryImpl &&) = delete;

	virtual ~DynamicLibraryImpl() {
		if (lib)
			lib->unload();
		lib = nullptr;
	}
	Function FindFunction(const char *name) override {
		if (lib) {
			// Use memcpy as it doesn't invoke undefined or conditionally defined behaviour.
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
			QFunctionPointer fp {};
#else
			void *fp = nullptr;
#endif
			fp = lib->resolve(name);
			Function f = nullptr;
			static_assert(sizeof(f) == sizeof(fp));
			memcpy(&f, &fp, sizeof(f));
			return f;
		}
		return nullptr;
	}
	bool IsValid() override {
		return lib != nullptr;
	}
};
DynamicLibrary *DynamicLibrary::Load(const char *modulePath)
{
	return static_cast<DynamicLibrary *>(new DynamicLibraryImpl(modulePath));
}

ColourDesired Platform::Chrome()
{
	QColor c(Qt::gray);
	return ColourDesired(c.red(), c.green(), c.blue());
}

ColourDesired Platform::ChromeHighlight()
{
	QColor c(Qt::lightGray);
	return ColourDesired(c.red(), c.green(), c.blue());
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

void Platform::DebugDisplay(const char *s)
{
	qWarning("Scintilla: %s", s);
}

void Platform::DebugPrintf(const char *format, ...)
{
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsprintf(buffer, format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}

bool Platform::ShowAssertionPopUps(bool /*assertionPopUps*/)
{
	return false;
}

void Platform::Assert(const char *c, const char *file, int line)
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
