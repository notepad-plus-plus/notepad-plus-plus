//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// Scintilla platform layer for Qt

#include "PlatQt.h"
#include "Scintilla.h"
#include "FontQuality.h"

#include <QApplication>
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
#include <QElapsedTimer>
#include <cstdio>

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

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

class FontAndCharacterSet {
public:
	int characterSet;
	QFont *pfont;
	FontAndCharacterSet(int characterSet_, QFont *pfont):
		characterSet(characterSet_), pfont(pfont) {
	}
	~FontAndCharacterSet() {
		delete pfont;
		pfont = 0;
	}
};

static int FontCharacterSet(Font &f)
{
	return reinterpret_cast<FontAndCharacterSet *>(f.GetID())->characterSet;
}

static QFont *FontPointer(Font &f)
{
	return reinterpret_cast<FontAndCharacterSet *>(f.GetID())->pfont;
}

Font::Font() : fid(0) {}

Font::~Font()
{
	delete reinterpret_cast<FontAndCharacterSet *>(fid);
	fid = 0;
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
	font->setPointSize(fp.size);
	font->setBold(fp.weight > 500);
	font->setItalic(fp.italic);

	fid = new FontAndCharacterSet(fp.characterSet, font);
}

void Font::Release()
{
	if (fid)
		delete reinterpret_cast<FontAndCharacterSet *>(fid);

	fid = 0;
}


SurfaceImpl::SurfaceImpl()
: device(0), painter(0), deviceOwned(false), painterOwned(false), x(0), y(0),
	  unicodeMode(false), codePage(0), codecName(0), codec(0)
{}

SurfaceImpl::~SurfaceImpl()
{
	Release();
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
	SurfaceImpl *psurfOther = static_cast<SurfaceImpl *>(surface);
	SetUnicodeMode(psurfOther->unicodeMode);
	SetDBCSMode(psurfOther->codePage);
}

void SurfaceImpl::Release()
{
	if (painterOwned && painter) {
		delete painter;
	}

	if (deviceOwned && device) {
		delete device;
	}

	device = 0;
	painter = 0;
	deviceOwned = false;
	painterOwned = false;
}

bool SurfaceImpl::Initialised()
{
	return device != 0;
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

void SurfaceImpl::SetCodec(Font &font)
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

void SurfaceImpl::SetFont(Font &font)
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
                          int npts,
                          ColourDesired fore,
                          ColourDesired back)
{
	PenColour(fore);
	BrushColour(back);

	QPoint *qpts = new QPoint[npts];
	for (int i = 0; i < npts; i++) {
		qpts[i] = QPoint(pts[i].x, pts[i].y);
	}

	GetPainter()->drawPolygon(qpts, npts);
	delete [] qpts;
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
	SurfaceImpl *surface = static_cast<SurfaceImpl *>(&surfacePattern);
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
	GetPainter()->drawRoundRect(QRectFFromPRect(rc));
}

void SurfaceImpl::AlphaRectangle(PRectangle rc,
                                 int cornerSize,
                                 ColourDesired fill,
                                 int alphaFill,
                                 ColourDesired outline,
                                 int alphaOutline,
                                 int /*flags*/)
{
	QColor qOutline = QColorFromCA(outline);
	qOutline.setAlpha(alphaOutline);
	GetPainter()->setPen(QPen(qOutline));

	QColor qFill = QColorFromCA(fill);
	qFill.setAlpha(alphaFill);
	GetPainter()->setBrush(QBrush(qFill));

	// A radius of 1 shows no curve so add 1
	qreal radius = cornerSize+1;
	QRectF rect(rc.left, rc.top, rc.Width() - 1, rc.Height() - 1);
	GetPainter()->drawRoundedRect(rect, radius, radius);
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
	SurfaceImpl *source = static_cast<SurfaceImpl *>(&surfaceSource);
	QPixmap *pixmap = static_cast<QPixmap *>(source->GetPaintDevice());

	GetPainter()->drawPixmap(rc.left, rc.top, *pixmap, from.x, from.y, -1, -1);
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc,
                                 Font &font,
                                 XYPOSITION ybase,
                                 const char *s,
                                 int len,
                                 ColourDesired fore,
                                 ColourDesired back)
{
	SetFont(font);
	PenColour(fore);

	GetPainter()->setBackground(QColorFromCA(back));
	GetPainter()->setBackgroundMode(Qt::OpaqueMode);
	QString su = codec->toUnicode(s, len);
	GetPainter()->drawText(QPointF(rc.left, ybase), su);
}

void SurfaceImpl::DrawTextClipped(PRectangle rc,
                                  Font &font,
                                  XYPOSITION ybase,
                                  const char *s,
                                  int len,
                                  ColourDesired fore,
                                  ColourDesired back)
{
	SetClip(rc);
	DrawTextNoClip(rc, font, ybase, s, len, fore, back);
	GetPainter()->setClipping(false);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc,
                                      Font &font,
                                      XYPOSITION ybase,
                                      const char *s,
                                      int len,
        ColourDesired fore)
{
	SetFont(font);
	PenColour(fore);

	GetPainter()->setBackgroundMode(Qt::TransparentMode);
	QString su = codec->toUnicode(s, len);
	GetPainter()->drawText(QPointF(rc.left, ybase), su);
}

void SurfaceImpl::SetClip(PRectangle rc)
{
	GetPainter()->setClipRect(QRectFFromPRect(rc));
}

static size_t utf8LengthFromLead(unsigned char uch)
{
	if (uch >= (0x80 + 0x40 + 0x20 + 0x10)) {
		return 4;
	} else if (uch >= (0x80 + 0x40 + 0x20)) {
		return 3;
	} else if (uch >= (0x80)) {
		return 2;
	} else {
		return 1;
	}
}

void SurfaceImpl::MeasureWidths(Font &font,
                                const char *s,
                                int len,
                                XYPOSITION *positions)
{
	if (!font.GetID())
		return;
	SetCodec(font);
	QString su = codec->toUnicode(s, len);
	QTextLayout tlay(su, *FontPointer(font), GetPaintDevice());
	tlay.beginLayout();
	QTextLine tl = tlay.createLine();
	tlay.endLayout();
	if (unicodeMode) {
		int fit = su.size();
		int ui=0;
		const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
		int i=0;
		while (ui<fit) {
			size_t lenChar = utf8LengthFromLead(us[i]);
			int codeUnits = (lenChar < 4) ? 1 : 2;
			qreal xPosition = tl.cursorToX(ui+codeUnits);
			for (unsigned int bytePos=0; (bytePos<lenChar) && (i<len); bytePos++) {
				positions[i++] = xPosition;
			}
			ui += codeUnits;
		}
		XYPOSITION lastPos = 0;
		if (i > 0)
			lastPos = positions[i-1];
		while (i<len) {
			positions[i++] = lastPos;
		}
	} else if (codePage) {
		// DBCS
		int ui = 0;
		for (int i=0; i<len;) {
			size_t lenChar = Platform::IsDBCSLeadByte(codePage, s[i]) ? 2 : 1;
			qreal xPosition = tl.cursorToX(ui+1);
			for (unsigned int bytePos=0; (bytePos<lenChar) && (i<len); bytePos++) {
				positions[i++] = xPosition;
			}
			ui++;
		}
	} else {
		// Single byte encoding
		for (int i=0; i<len; i++) {
			positions[i] = tl.cursorToX(i+1);
		}
	}
}

XYPOSITION SurfaceImpl::WidthText(Font &font, const char *s, int len)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	SetCodec(font);
	QString string = codec->toUnicode(s, len);
	return metrics.width(string);
}

XYPOSITION SurfaceImpl::WidthChar(Font &font, char ch)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	return metrics.width(ch);
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

XYPOSITION SurfaceImpl::ExternalLeading(Font &font)
{
	QFontMetricsF metrics(*FontPointer(font), device);
	return metrics.leading();
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

QPaintDevice *SurfaceImpl::GetPaintDevice()
{
	return device;
}

QPainter *SurfaceImpl::GetPainter()
{
	Q_ASSERT(device);

	if (painter == 0) {
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
QWidget *window(WindowID wid)
{
	return static_cast<QWidget *>(wid);
}
}

Window::~Window() {}

void Window::Destroy()
{
	if (wid)
		delete window(wid);

	wid = 0;
}

bool Window::HasFocus()
{
	return wid ? window(wid)->hasFocus() : false;
}

PRectangle Window::GetPosition()
{
	// Before any size allocated pretend its 1000 wide so not scrolled
	return wid ? PRectFromQRect(window(wid)->frameGeometry()) : PRectangle(0, 0, 1000, 1000);
}

void Window::SetPosition(PRectangle rc)
{
	if (wid)
		window(wid)->setGeometry(QRectFromPRect(rc));
}

void Window::SetPositionRelative(PRectangle rc, Window relativeTo)
{
	QPoint oPos = window(relativeTo.wid)->mapToGlobal(QPoint(0,0));
	int ox = oPos.x();
	int oy = oPos.y();
	ox += rc.left;
	oy += rc.top;

	QDesktopWidget *desktop = QApplication::desktop();
	QRect rectDesk = desktop->availableGeometry(QPoint(ox, oy));
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

PRectangle Window::GetClientPosition()
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

void Window::SetTitle(const char *s)
{
	if (wid)
		window(wid)->setWindowTitle(s);
}

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   window coordinates */
PRectangle Window::GetMonitorRect(Point pt)
{
	QPoint originGlobal = window(wid)->mapToGlobal(QPoint(0, 0));
	QPoint posGlobal = window(wid)->mapToGlobal(QPoint(pt.x, pt.y));
	QDesktopWidget *desktop = QApplication::desktop();
	QRect rectScreen = desktop->availableGeometry(posGlobal);
	rectScreen.translate(-originGlobal.x(), -originGlobal.y());
	return PRectangle(rectScreen.left(), rectScreen.top(),
	        rectScreen.right(), rectScreen.bottom());
}


//----------------------------------------------------------------------

class ListBoxImpl : public ListBox {
public:
	ListBoxImpl();
	~ListBoxImpl();

	virtual void SetFont(Font &font);
	virtual void Create(Window &parent, int ctrlID, Point location,
						int lineHeight, bool unicodeMode, int technology);
	virtual void SetAverageCharWidth(int width);
	virtual void SetVisibleRows(int rows);
	virtual int GetVisibleRows() const;
	virtual PRectangle GetDesiredRect();
	virtual int CaretFromEdge();
	virtual void Clear();
	virtual void Append(char *s, int type = -1);
	virtual int Length();
	virtual void Select(int n);
	virtual int GetSelection();
	virtual int Find(const char *prefix);
	virtual void GetValue(int n, char *value, int len);
	virtual void RegisterImage(int type, const char *xpmData);
	virtual void RegisterRGBAImage(int type, int width, int height,
		const unsigned char *pixelsImage);
	virtual void RegisterQPixmapImage(int type, const QPixmap& pm);
	virtual void ClearRegisteredImages();
	virtual void SetDoubleClickAction(CallBackAction action, void *data);
	virtual void SetList(const char *list, char separator, char typesep);
private:
	bool unicodeMode;
	int visibleRows;
	QMap<int,QPixmap> images;
};

class ListWidget : public QListWidget {
public:
	explicit ListWidget(QWidget *parent);
	virtual ~ListWidget();

	void setDoubleClickAction(CallBackAction action, void *data);

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual QStyleOptionViewItem viewOptions() const;

private:
	CallBackAction doubleClickAction;
	void *doubleClickActionData;
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
	list->setParent(0, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
#else
	// On OS X, Qt::Tool takes focus so main window loses focus so
	// keyboard stops working. Qt::ToolTip works but its only really
	// documented for tooltips.
	// On Linux / X this setting allows clicking on list items.
	list->setParent(0, Qt::ToolTip | Qt::FramelessWindowHint);
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
	ListWidget *list = static_cast<ListWidget *>(wid);
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
	ListWidget *list = static_cast<ListWidget *>(wid);

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
	ListWidget *list = static_cast<ListWidget *>(wid);

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
	ListWidget *list = static_cast<ListWidget *>(wid);
	list->clear();
}

void ListBoxImpl::Append(char *s, int type)
{
	ListWidget *list = static_cast<ListWidget *>(wid);

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
	ListWidget *list = static_cast<ListWidget *>(wid);
	return list->count();
}

void ListBoxImpl::Select(int n)
{
	ListWidget *list = static_cast<ListWidget *>(wid);
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
	ListWidget *list = static_cast<ListWidget *>(wid);
	return list->currentRow();
}

int ListBoxImpl::Find(const char *prefix)
{
	ListWidget *list = static_cast<ListWidget *>(wid);
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
	ListWidget *list = static_cast<ListWidget *>(wid);
	QListWidgetItem *item = list->item(n);
	QString str = item->data(Qt::DisplayRole).toString();
	QByteArray bytes = unicodeMode ? str.toUtf8() : str.toLocal8Bit();

	strncpy(value, bytes.constData(), len);
	value[len-1] = '\0';
}

void ListBoxImpl::RegisterQPixmapImage(int type, const QPixmap& pm)
{
	images[type] = pm;

	ListWidget *list = static_cast<ListWidget *>(wid);
	if (list != NULL) {
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
	
	ListWidget *list = static_cast<ListWidget *>(wid);
	if (list != NULL)
		list->setIconSize(QSize(0, 0));
}

void ListBoxImpl::SetDoubleClickAction(CallBackAction action, void *data)
{
	ListWidget *list = static_cast<ListWidget *>(wid);
	list->setDoubleClickAction(action, data);
}

void ListBoxImpl::SetList(const char *list, char separator, char typesep)
{
	// This method is *not* platform dependent.
	// It is borrowed from the GTK implementation.
	Clear();
	size_t count = strlen(list) + 1;
	std::vector<char> words(list, list+count);
	char *startword = &words[0];
	char *numword = NULL;
	int i = 0;
	for (; words[i]; i++) {
		if (words[i] == separator) {
			words[i] = '\0';
			if (numword)
				*numword = '\0';
			Append(startword, numword?atoi(numword + 1):-1);
			startword = &words[0] + i + 1;
			numword = NULL;
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

ListBox::ListBox() {}

ListBox::~ListBox() {}

ListBox *ListBox::Allocate()
{
	return new ListBoxImpl();
}

ListWidget::ListWidget(QWidget *parent)
: QListWidget(parent), doubleClickAction(0), doubleClickActionData(0)
{}

ListWidget::~ListWidget() {}

void ListWidget::setDoubleClickAction(CallBackAction action, void *data)
{
	doubleClickAction = action;
	doubleClickActionData = data;
}

void ListWidget::mouseDoubleClickEvent(QMouseEvent * /* event */)
{
	if (doubleClickAction != 0) {
		doubleClickAction(doubleClickActionData);
	}
}

QStyleOptionViewItem ListWidget::viewOptions() const
{
	QStyleOptionViewItem result = QListWidget::viewOptions();
	result.state |= QStyle::State_Active;
	return result;
}

//----------------------------------------------------------------------

Menu::Menu() : mid(0) {}

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
	mid = 0;
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

	virtual ~DynamicLibraryImpl() {
		if (lib)
			lib->unload();
		lib = 0;
	}

	virtual Function FindFunction(const char *name) {
		if (lib) {
			// C++ standard doesn't like casts between function pointers and void pointers so use a union
			union {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
				QFunctionPointer fp;
#else
				void *fp;
#endif
				Function f;
			} fnConv;
			fnConv.fp = lib->resolve(name);
			return fnConv.f;
		}
		return NULL;
	}

	virtual bool IsValid() {
		return lib != NULL;
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

bool Platform::MouseButtonBounce()
{
	return false;
}

bool Platform::IsKeyDown(int /*key*/)
{
	return false;
}

long Platform::SendScintilla(WindowID /*w*/,
                             unsigned int /*msg*/,
                             unsigned long /*wParam*/,
                             long /*lParam*/)
{
	return 0;
}

long Platform::SendScintillaPointer(WindowID /*w*/,
                                    unsigned int /*msg*/,
                                    unsigned long /*wParam*/,
                                    void * /*lParam*/)
{
	return 0;
}

int Platform::Minimum(int a, int b)
{
	return qMin(a, b);
}

int Platform::Maximum(int a, int b)
{
	return qMax(a, b);
}

int Platform::Clamp(int val, int minVal, int maxVal)
{
	return qBound(minVal, val, maxVal);
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


bool Platform::IsDBCSLeadByte(int codePage, char ch)
{
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	unsigned char uch = static_cast<unsigned char>(ch);
	switch (codePage) {
	case 932:
		// Shift_jis
		return ((uch >= 0x81) && (uch <= 0x9F)) ||
		       ((uch >= 0xE0) && (uch <= 0xEF));
	case 936:
		// GBK
		return (uch >= 0x81) && (uch <= 0xFE);
	case 949:
		// Korean Wansung KS C-5601-1987
		return (uch >= 0x81) && (uch <= 0xFE);
	case 950:
		// Big5
		return (uch >= 0x81) && (uch <= 0xFE);
	case 1361:
		// Korean Johab KS C-5601-1992
		return
		    ((uch >= 0x84) && (uch <= 0xD3)) ||
		    ((uch >= 0xD8) && (uch <= 0xDE)) ||
		    ((uch >= 0xE0) && (uch <= 0xF9));
	}
	return false;
}

int Platform::DBCSCharLength(int codePage, const char *s)
{
	if (codePage == 932 || codePage == 936 || codePage == 949 ||
	        codePage == 950 || codePage == 1361) {
		return IsDBCSLeadByte(codePage, s[0]) ? 2 : 1;
	} else {
		return 1;
	}
}

int Platform::DBCSCharMaxLength()
{
	return 2;
}


//----------------------------------------------------------------------

static QElapsedTimer timer;

ElapsedTime::ElapsedTime() : bigBit(0), littleBit(0)
{
	if (!timer.isValid()) {
		timer.start();
	}
	qint64 ns64Now = timer.nsecsElapsed();
	bigBit = static_cast<unsigned long>(ns64Now >> 32);
	littleBit = static_cast<unsigned long>(ns64Now & 0xFFFFFFFF);
}

double ElapsedTime::Duration(bool reset)
{
	qint64 ns64Now = timer.nsecsElapsed();
	qint64 ns64Start = (static_cast<qint64>(static_cast<unsigned long>(bigBit)) << 32) + static_cast<unsigned long>(littleBit);
	double result = ns64Now - ns64Start;
	if (reset) {
		bigBit = static_cast<unsigned long>(ns64Now >> 32);
		littleBit = static_cast<unsigned long>(ns64Now & 0xFFFFFFFF);
	}
	return result / 1000000000.0;	// 1 billion nanoseconds in a second
}

#ifdef SCI_NAMESPACE
}
#endif
