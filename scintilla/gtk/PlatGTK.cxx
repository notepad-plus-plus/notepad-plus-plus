// Scintilla source code edit control
// PlatGTK.cxx - implementation of platform facilities on GTK+/Linux
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>

#include <string>
#include <vector>
#include <map>
#include <sstream>

#include <glib.h>
#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "Platform.h"

#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "StringCopy.h"
#include "XPM.h"
#include "UniConversion.h"

#if defined(__clang__)
// Clang 3.0 incorrectly displays  sentinel warnings. Fixed by clang 3.1.
#pragma GCC diagnostic ignored "-Wsentinel"
#endif

/* GLIB must be compiled with thread support, otherwise we
   will bail on trying to use locks, and that could lead to
   problems for someone.  `glib-config --libs gthread` needs
   to be used to get the glib libraries for linking, otherwise
   g_thread_init will fail */
#define USE_LOCK defined(G_THREADS_ENABLED) && !defined(G_THREADS_IMPL_NONE)

#include "Converter.h"

static const double kPi = 3.14159265358979323846;

// The Pango version guard for pango_units_from_double and pango_units_to_double
// is more complex than simply implementing these here.

static int pangoUnitsFromDouble(double d) {
	return static_cast<int>(d * PANGO_SCALE + 0.5);
}

static double doubleFromPangoUnits(int pu) {
	return static_cast<double>(pu) / PANGO_SCALE;
}

static cairo_surface_t *CreateSimilarSurface(GdkWindow *window, cairo_content_t content, int width, int height) {
#if GTK_CHECK_VERSION(2,22,0)
	return gdk_window_create_similar_surface(window, content, width, height);
#else
	cairo_surface_t *window_surface, *surface;

	g_return_val_if_fail(GDK_IS_WINDOW(window), NULL);

	window_surface = GDK_DRAWABLE_GET_CLASS(window)->ref_cairo_surface(window);

	surface = cairo_surface_create_similar(window_surface, content, width, height);

	cairo_surface_destroy(window_surface);

	return surface;
#endif
}

static GdkWindow *WindowFromWidget(GtkWidget *w) {
	return gtk_widget_get_window(w);
}

#ifdef _MSC_VER
// Ignore unreferenced local functions in GTK+ headers
#pragma warning(disable: 4505)
#endif

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

enum encodingType { singleByte, UTF8, dbcs};

struct LOGFONT {
	int size;
	int weight;
	bool italic;
	int characterSet;
	char faceName[300];
};

#if USE_LOCK
static GMutex *fontMutex = NULL;

static void InitializeGLIBThreads() {
#if !GLIB_CHECK_VERSION(2,31,0)
	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}
#endif
}
#endif

static void FontMutexAllocate() {
#if USE_LOCK
	if (!fontMutex) {
		InitializeGLIBThreads();
#if GLIB_CHECK_VERSION(2,31,0)
		fontMutex = g_new(GMutex, 1);
		g_mutex_init(fontMutex);
#else
		fontMutex = g_mutex_new();
#endif
	}
#endif
}

static void FontMutexFree() {
#if USE_LOCK
	if (fontMutex) {
#if GLIB_CHECK_VERSION(2,31,0)
		g_mutex_clear(fontMutex);
		g_free(fontMutex);
#else
		g_mutex_free(fontMutex);
#endif
		fontMutex = NULL;
	}
#endif
}

static void FontMutexLock() {
#if USE_LOCK
	g_mutex_lock(fontMutex);
#endif
}

static void FontMutexUnlock() {
#if USE_LOCK
	if (fontMutex) {
		g_mutex_unlock(fontMutex);
	}
#endif
}

// Holds a PangoFontDescription*.
class FontHandle {
	XYPOSITION width[128];
	encodingType et;
public:
	int ascent;
	PangoFontDescription *pfd;
	int characterSet;
	FontHandle() : et(singleByte), ascent(0), pfd(0), characterSet(-1) {
		ResetWidths(et);
	}
	FontHandle(PangoFontDescription *pfd_, int characterSet_) {
		et = singleByte;
		ascent = 0;
		pfd = pfd_;
		characterSet = characterSet_;
		ResetWidths(et);
	}
	~FontHandle() {
		if (pfd)
			pango_font_description_free(pfd);
		pfd = 0;
	}
	void ResetWidths(encodingType et_) {
		et = et_;
		for (int i=0; i<=127; i++) {
			width[i] = 0;
		}
	}
	XYPOSITION CharWidth(unsigned char ch, encodingType et_) const {
		XYPOSITION w = 0;
		FontMutexLock();
		if ((ch <= 127) && (et == et_)) {
			w = width[ch];
		}
		FontMutexUnlock();
		return w;
	}
	void SetCharWidth(unsigned char ch, XYPOSITION w, encodingType et_) {
		if (ch <= 127) {
			FontMutexLock();
			if (et != et_) {
				ResetWidths(et_);
			}
			width[ch] = w;
			FontMutexUnlock();
		}
	}
};

// X has a 16 bit coordinate space, so stop drawing here to avoid wrapping
static const int maxCoordinate = 32000;

static FontHandle *PFont(Font &f) {
	return static_cast<FontHandle *>(f.GetID());
}

static GtkWidget *PWidget(WindowID wid) {
	return static_cast<GtkWidget *>(wid);
}

Point Point::FromLong(long lpoint) {
	return Point(
	           Platform::LowShortFromLong(lpoint),
	           Platform::HighShortFromLong(lpoint));
}

static void SetLogFont(LOGFONT &lf, const char *faceName, int characterSet, float size, int weight, bool italic) {
	lf = LOGFONT();
	lf.size = size;
	lf.weight = weight;
	lf.italic = italic;
	lf.characterSet = characterSet;
	StringCopy(lf.faceName, faceName);
}

/**
 * Create a hash from the parameters for a font to allow easy checking for identity.
 * If one font is the same as another, its hash will be the same, but if the hash is the
 * same then they may still be different.
 */
static int HashFont(const FontParameters &fp) {
	return
	    static_cast<int>(fp.size+0.5) ^
	    (fp.characterSet << 10) ^
	    ((fp.weight / 100) << 12) ^
	    (fp.italic ? 0x20000000 : 0) ^
	    fp.faceName[0];
}

class FontCached : Font {
	FontCached *next;
	int usage;
	LOGFONT lf;
	int hash;
	explicit FontCached(const FontParameters &fp);
	~FontCached() {}
	bool SameAs(const FontParameters &fp);
	virtual void Release();
	static FontID CreateNewFont(const FontParameters &fp);
	static FontCached *first;
public:
	static FontID FindOrCreate(const FontParameters &fp);
	static void ReleaseId(FontID fid_);
	static void ReleaseAll();
};

FontCached *FontCached::first = 0;

FontCached::FontCached(const FontParameters &fp) :
next(0), usage(0), hash(0) {
	::SetLogFont(lf, fp.faceName, fp.characterSet, fp.size, fp.weight, fp.italic);
	hash = HashFont(fp);
	fid = CreateNewFont(fp);
	usage = 1;
}

bool FontCached::SameAs(const FontParameters &fp) {
	return
	    lf.size == fp.size &&
	    lf.weight == fp.weight &&
	    lf.italic == fp.italic &&
	    lf.characterSet == fp.characterSet &&
	    0 == strcmp(lf.faceName, fp.faceName);
}

void FontCached::Release() {
	if (fid)
		delete PFont(*this);
	fid = 0;
}

FontID FontCached::FindOrCreate(const FontParameters &fp) {
	FontID ret = 0;
	FontMutexLock();
	int hashFind = HashFont(fp);
	for (FontCached *cur = first; cur; cur = cur->next) {
		if ((cur->hash == hashFind) &&
		        cur->SameAs(fp)) {
			cur->usage++;
			ret = cur->fid;
		}
	}
	if (ret == 0) {
		FontCached *fc = new FontCached(fp);
		fc->next = first;
		first = fc;
		ret = fc->fid;
	}
	FontMutexUnlock();
	return ret;
}

void FontCached::ReleaseId(FontID fid_) {
	FontMutexLock();
	FontCached **pcur = &first;
	for (FontCached *cur = first; cur; cur = cur->next) {
		if (cur->fid == fid_) {
			cur->usage--;
			if (cur->usage == 0) {
				*pcur = cur->next;
				cur->Release();
				cur->next = 0;
				delete cur;
			}
			break;
		}
		pcur = &cur->next;
	}
	FontMutexUnlock();
}

void FontCached::ReleaseAll() {
	while (first) {
		ReleaseId(first->GetID());
	}
}

FontID FontCached::CreateNewFont(const FontParameters &fp) {
	PangoFontDescription *pfd = pango_font_description_new();
	if (pfd) {
		pango_font_description_set_family(pfd,
			(fp.faceName[0] == '!') ? fp.faceName+1 : fp.faceName);
		pango_font_description_set_size(pfd, pangoUnitsFromDouble(fp.size));
		pango_font_description_set_weight(pfd, static_cast<PangoWeight>(fp.weight));
		pango_font_description_set_style(pfd, fp.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
		return new FontHandle(pfd, fp.characterSet);
	}

	return new FontHandle();
}

Font::Font() : fid(0) {}

Font::~Font() {}

void Font::Create(const FontParameters &fp) {
	Release();
	fid = FontCached::FindOrCreate(fp);
}

void Font::Release() {
	if (fid)
		FontCached::ReleaseId(fid);
	fid = 0;
}

// Required on OS X
#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

// SurfaceID is a cairo_t*
class SurfaceImpl : public Surface {
	encodingType et;
	cairo_t *context;
	cairo_surface_t *psurf;
	int x;
	int y;
	bool inited;
	bool createdGC;
	PangoContext *pcontext;
	PangoLayout *layout;
	Converter conv;
	int characterSet;
	void SetConverter(int characterSet_);
public:
	SurfaceImpl();
	virtual ~SurfaceImpl();

	void Init(WindowID wid);
	void Init(SurfaceID sid, WindowID wid);
	void InitPixMap(int width, int height, Surface *surface_, WindowID wid);

	void Release();
	bool Initialised();
	void PenColour(ColourDesired fore);
	int LogPixelsY();
	int DeviceHeightFont(int points);
	void MoveTo(int x_, int y_);
	void LineTo(int x_, int y_);
	void Polygon(Point *pts, int npts, ColourDesired fore, ColourDesired back);
	void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back);
	void FillRectangle(PRectangle rc, ColourDesired back);
	void FillRectangle(PRectangle rc, Surface &surfacePattern);
	void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back);
	void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int flags);
	void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage);
	void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back);
	void Copy(PRectangle rc, Point from, Surface &surfaceSource);

	void DrawTextBase(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore);
	void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back);
	void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back);
	void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore);
	void MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions);
	XYPOSITION WidthText(Font &font_, const char *s, int len);
	XYPOSITION WidthChar(Font &font_, char ch);
	XYPOSITION Ascent(Font &font_);
	XYPOSITION Descent(Font &font_);
	XYPOSITION InternalLeading(Font &font_);
	XYPOSITION ExternalLeading(Font &font_);
	XYPOSITION Height(Font &font_);
	XYPOSITION AverageCharWidth(Font &font_);

	void SetClip(PRectangle rc);
	void FlushCachedState();

	void SetUnicodeMode(bool unicodeMode_);
	void SetDBCSMode(int codePage);
};
#ifdef SCI_NAMESPACE
}
#endif

const char *CharacterSetID(int characterSet) {
	switch (characterSet) {
	case SC_CHARSET_ANSI:
		return "";
	case SC_CHARSET_DEFAULT:
		return "ISO-8859-1";
	case SC_CHARSET_BALTIC:
		return "ISO-8859-13";
	case SC_CHARSET_CHINESEBIG5:
		return "BIG-5";
	case SC_CHARSET_EASTEUROPE:
		return "ISO-8859-2";
	case SC_CHARSET_GB2312:
		return "CP936";
	case SC_CHARSET_GREEK:
		return "ISO-8859-7";
	case SC_CHARSET_HANGUL:
		return "CP949";
	case SC_CHARSET_MAC:
		return "MACINTOSH";
	case SC_CHARSET_OEM:
		return "ASCII";
	case SC_CHARSET_RUSSIAN:
		return "KOI8-R";
	case SC_CHARSET_OEM866:
		return "CP866";
	case SC_CHARSET_CYRILLIC:
		return "CP1251";
	case SC_CHARSET_SHIFTJIS:
		return "SHIFT-JIS";
	case SC_CHARSET_SYMBOL:
		return "";
	case SC_CHARSET_TURKISH:
		return "ISO-8859-9";
	case SC_CHARSET_JOHAB:
		return "CP1361";
	case SC_CHARSET_HEBREW:
		return "ISO-8859-8";
	case SC_CHARSET_ARABIC:
		return "ISO-8859-6";
	case SC_CHARSET_VIETNAMESE:
		return "";
	case SC_CHARSET_THAI:
		return "ISO-8859-11";
	case SC_CHARSET_8859_15:
		return "ISO-8859-15";
	default:
		return "";
	}
}

void SurfaceImpl::SetConverter(int characterSet_) {
	if (characterSet != characterSet_) {
		characterSet = characterSet_;
		conv.Open("UTF-8", CharacterSetID(characterSet), false);
	}
}

SurfaceImpl::SurfaceImpl() : et(singleByte),
context(0),
psurf(0),
x(0), y(0), inited(false), createdGC(false)
, pcontext(0), layout(0), characterSet(-1) {
}

SurfaceImpl::~SurfaceImpl() {
	Release();
}

void SurfaceImpl::Release() {
	et = singleByte;
	if (createdGC) {
		createdGC = false;
		cairo_destroy(context);
	}
	context = 0;
	if (psurf)
		cairo_surface_destroy(psurf);
	psurf = 0;
	if (layout)
		g_object_unref(layout);
	layout = 0;
	if (pcontext)
		g_object_unref(pcontext);
	pcontext = 0;
	conv.Close();
	characterSet = -1;
	x = 0;
	y = 0;
	inited = false;
	createdGC = false;
}

bool SurfaceImpl::Initialised() {
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 8, 0)
	if (inited && context) {
		if (cairo_status(context) == CAIRO_STATUS_SUCCESS) {
			// Even when status is success, the target surface may have been
			// finished whch may cause an assertion to fail crashing the application.
			// The cairo_surface_has_show_text_glyphs call checks the finished flag
			// and when set, sets the status to CAIRO_STATUS_SURFACE_FINISHED
			// which leads to warning messages instead of crashes.
			// Performing the check in this method as it is called rarely and has no
			// other side effects.
			cairo_surface_t *psurfContext = cairo_get_target(context);
			if (psurfContext) {
				cairo_surface_has_show_text_glyphs(psurfContext);
			}
		}
		return cairo_status(context) == CAIRO_STATUS_SUCCESS;
	}
#endif
	return inited;
}

void SurfaceImpl::Init(WindowID wid) {
	Release();
	PLATFORM_ASSERT(wid);
	// if we are only created from a window ID, we can't perform drawing
	psurf = 0;
	context = 0;
	createdGC = false;
	pcontext = gtk_widget_create_pango_context(PWidget(wid));
	PLATFORM_ASSERT(pcontext);
	layout = pango_layout_new(pcontext);
	PLATFORM_ASSERT(layout);
	inited = true;
}

void SurfaceImpl::Init(SurfaceID sid, WindowID wid) {
	PLATFORM_ASSERT(sid);
	Release();
	PLATFORM_ASSERT(wid);
	context = cairo_reference(static_cast<cairo_t *>(sid));
	pcontext = gtk_widget_create_pango_context(PWidget(wid));
	// update the Pango context in case sid isn't the widget's surface
	pango_cairo_update_context(context, pcontext);
	layout = pango_layout_new(pcontext);
	cairo_set_line_width(context, 1);
	createdGC = true;
	inited = true;
}

void SurfaceImpl::InitPixMap(int width, int height, Surface *surface_, WindowID wid) {
	PLATFORM_ASSERT(surface_);
	Release();
	SurfaceImpl *surfImpl = static_cast<SurfaceImpl *>(surface_);
	PLATFORM_ASSERT(wid);
	context = cairo_reference(surfImpl->context);
	pcontext = gtk_widget_create_pango_context(PWidget(wid));
	// update the Pango context in case surface_ isn't the widget's surface
	pango_cairo_update_context(context, pcontext);
	PLATFORM_ASSERT(pcontext);
	layout = pango_layout_new(pcontext);
	PLATFORM_ASSERT(layout);
	if (height > 0 && width > 0)
		psurf = CreateSimilarSurface(
			WindowFromWidget(PWidget(wid)),
			CAIRO_CONTENT_COLOR_ALPHA, width, height);
	cairo_destroy(context);
	context = cairo_create(psurf);
	cairo_rectangle(context, 0, 0, width, height);
	cairo_set_source_rgb(context, 1.0, 0, 0);
	cairo_fill(context);
	// This produces sharp drawing more similar to GDK:
	//cairo_set_antialias(context, CAIRO_ANTIALIAS_NONE);
	cairo_set_line_width(context, 1);
	createdGC = true;
	inited = true;
	et = surfImpl->et;
}

void SurfaceImpl::PenColour(ColourDesired fore) {
	if (context) {
		ColourDesired cdFore(fore.AsLong());
		cairo_set_source_rgb(context,
			cdFore.GetRed() / 255.0,
			cdFore.GetGreen() / 255.0,
			cdFore.GetBlue() / 255.0);
	}
}

int SurfaceImpl::LogPixelsY() {
	return 72;
}

int SurfaceImpl::DeviceHeightFont(int points) {
	int logPix = LogPixelsY();
	return (points * logPix + logPix / 2) / 72;
}

void SurfaceImpl::MoveTo(int x_, int y_) {
	x = x_;
	y = y_;
}

static int Delta(int difference) {
	if (difference < 0)
		return -1;
	else if (difference > 0)
		return 1;
	else
		return 0;
}

void SurfaceImpl::LineTo(int x_, int y_) {
	// cairo_line_to draws the end position, unlike Win32 or GDK with GDK_CAP_NOT_LAST.
	// For simple cases, move back one pixel from end.
	if (context) {
		int xDiff = x_ - x;
		int xDelta = Delta(xDiff);
		int yDiff = y_ - y;
		int yDelta = Delta(yDiff);
		if ((xDiff == 0) || (yDiff == 0)) {
			// Horizontal or vertical lines can be more precisely drawn as a filled rectangle
			int xEnd = x_ - xDelta;
			int left = Platform::Minimum(x, xEnd);
			int width = abs(x - xEnd) + 1;
			int yEnd = y_ - yDelta;
			int top = Platform::Minimum(y, yEnd);
			int height = abs(y - yEnd) + 1;
			cairo_rectangle(context, left, top, width, height);
			cairo_fill(context);
		} else if ((abs(xDiff) == abs(yDiff))) {
			// 45 degree slope
			cairo_move_to(context, x + 0.5, y + 0.5);
			cairo_line_to(context, x_ + 0.5 - xDelta, y_ + 0.5 - yDelta);
		} else {
			// Line has a different slope so difficult to avoid last pixel
			cairo_move_to(context, x + 0.5, y + 0.5);
			cairo_line_to(context, x_ + 0.5, y_ + 0.5);
		}
		cairo_stroke(context);
	}
	x = x_;
	y = y_;
}

void SurfaceImpl::Polygon(Point *pts, int npts, ColourDesired fore,
                          ColourDesired back) {
	PLATFORM_ASSERT(context);
	PenColour(back);
	cairo_move_to(context, pts[0].x + 0.5, pts[0].y + 0.5);
	for (int i = 1; i < npts; i++) {
		cairo_line_to(context, pts[i].x + 0.5, pts[i].y + 0.5);
	}
	cairo_close_path(context);
	cairo_fill_preserve(context);
	PenColour(fore);
	cairo_stroke(context);
}

void SurfaceImpl::RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) {
	if (context) {
		cairo_rectangle(context, rc.left + 0.5, rc.top + 0.5,
	                     rc.right - rc.left - 1, rc.bottom - rc.top - 1);
		PenColour(back);
		cairo_fill_preserve(context);
		PenColour(fore);
		cairo_stroke(context);
	}
}

void SurfaceImpl::FillRectangle(PRectangle rc, ColourDesired back) {
	PenColour(back);
	if (context && (rc.left < maxCoordinate)) {	// Protect against out of range
		rc.left = lround(rc.left);
		rc.right = lround(rc.right);
		cairo_rectangle(context, rc.left, rc.top,
	                     rc.right - rc.left, rc.bottom - rc.top);
		cairo_fill(context);
	}
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceImpl &surfi = static_cast<SurfaceImpl &>(surfacePattern);
	bool canDraw = surfi.psurf != NULL;
	if (canDraw) {
		PLATFORM_ASSERT(context);
		// Tile pattern over rectangle
		// Currently assumes 8x8 pattern
		int widthPat = 8;
		int heightPat = 8;
		for (int xTile = rc.left; xTile < rc.right; xTile += widthPat) {
			int widthx = (xTile + widthPat > rc.right) ? rc.right - xTile : widthPat;
			for (int yTile = rc.top; yTile < rc.bottom; yTile += heightPat) {
				int heighty = (yTile + heightPat > rc.bottom) ? rc.bottom - yTile : heightPat;
				cairo_set_source_surface(context, surfi.psurf, xTile, yTile);
				cairo_rectangle(context, xTile, yTile, widthx, heighty);
				cairo_fill(context);
			}
		}
	} else {
		// Something is wrong so try to show anyway
		// Shows up black because colour not allocated
		FillRectangle(rc, ColourDesired(0));
	}
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) {
	if (((rc.right - rc.left) > 4) && ((rc.bottom - rc.top) > 4)) {
		// Approximate a round rect with some cut off corners
		Point pts[] = {
		                  Point(rc.left + 2, rc.top),
		                  Point(rc.right - 2, rc.top),
		                  Point(rc.right, rc.top + 2),
		                  Point(rc.right, rc.bottom - 2),
		                  Point(rc.right - 2, rc.bottom),
		                  Point(rc.left + 2, rc.bottom),
		                  Point(rc.left, rc.bottom - 2),
		                  Point(rc.left, rc.top + 2),
		              };
		Polygon(pts, ELEMENTS(pts), fore, back);
	} else {
		RectangleDraw(rc, fore, back);
	}
}

static void PathRoundRectangle(cairo_t *context, double left, double top, double width, double height, int radius) {
	double degrees = kPi / 180.0;

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 2, 0)
	cairo_new_sub_path(context);
#else
	// First arc is in the top-right corner and starts from a point on the top line
	cairo_move_to(context, left + width - radius, top);
#endif
	cairo_arc(context, left + width - radius, top + radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(context, left + width - radius, top + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(context, left + radius, top + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(context, left + radius, top + radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(context);
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int flags) {
	if (context && rc.Width() > 0) {
		ColourDesired cdFill(fill.AsLong());
		cairo_set_source_rgba(context,
			cdFill.GetRed() / 255.0,
			cdFill.GetGreen() / 255.0,
			cdFill.GetBlue() / 255.0,
			alphaFill / 255.0);
		if (cornerSize > 0)
			PathRoundRectangle(context, rc.left + 1.0, rc.top + 1.0, rc.right - rc.left - 2.0, rc.bottom - rc.top - 2.0, cornerSize);
		else
			cairo_rectangle(context, rc.left + 1.0, rc.top + 1.0, rc.right - rc.left - 2.0, rc.bottom - rc.top - 2.0);
		cairo_fill(context);

		ColourDesired cdOutline(outline.AsLong());
		cairo_set_source_rgba(context,
			cdOutline.GetRed() / 255.0,
			cdOutline.GetGreen() / 255.0,
			cdOutline.GetBlue() / 255.0,
			alphaOutline / 255.0);
		if (cornerSize > 0)
			PathRoundRectangle(context, rc.left + 0.5, rc.top + 0.5, rc.right - rc.left - 1, rc.bottom - rc.top - 1, cornerSize);
		else
			cairo_rectangle(context, rc.left + 0.5, rc.top + 0.5, rc.right - rc.left - 1, rc.bottom - rc.top - 1);
		cairo_stroke(context);
	}
}

void SurfaceImpl::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	PLATFORM_ASSERT(context);
	if (rc.Width() > width)
		rc.left += (rc.Width() - width) / 2;
	rc.right = rc.left + width;
	if (rc.Height() > height)
		rc.top += (rc.Height() - height) / 2;
	rc.bottom = rc.top + height;

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1,6,0)
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
#else
	int stride = width * 4;
#endif
	int ucs = stride * height;
	std::vector<unsigned char> image(ucs);
	for (int iy=0; iy<height; iy++) {
		for (int ix=0; ix<width; ix++) {
			unsigned char *pixel = &image[0] + iy*stride + ix * 4;
			unsigned char alpha = pixelsImage[3];
			pixel[2] = (*pixelsImage++) * alpha / 255;
			pixel[1] = (*pixelsImage++) * alpha / 255;
			pixel[0] = (*pixelsImage++) * alpha / 255;
			pixel[3] = *pixelsImage++;
		}
	}

	cairo_surface_t *psurfImage = cairo_image_surface_create_for_data(&image[0], CAIRO_FORMAT_ARGB32, width, height, stride);
	cairo_set_source_surface(context, psurfImage, rc.left, rc.top);
	cairo_rectangle(context, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
	cairo_fill(context);

	cairo_surface_destroy(psurfImage);
}

void SurfaceImpl::Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) {
	PLATFORM_ASSERT(context);
	PenColour(back);
	cairo_arc(context, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2,
		Platform::Minimum(rc.Width(), rc.Height()) / 2, 0, 2*kPi);
	cairo_fill_preserve(context);
	PenColour(fore);
	cairo_stroke(context);
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceImpl &surfi = static_cast<SurfaceImpl &>(surfaceSource);
	bool canDraw = surfi.psurf != NULL;
	if (canDraw) {
		PLATFORM_ASSERT(context);
		cairo_set_source_surface(context, surfi.psurf,
			rc.left - from.x, rc.top - from.y);
		cairo_rectangle(context, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
		cairo_fill(context);
	}
}

std::string UTF8FromLatin1(const char *s, int len) {
	std::string utfForm(len*2 + 1, '\0');
	size_t lenU = 0;
	for (int i=0; i<len; i++) {
		unsigned int uch = static_cast<unsigned char>(s[i]);
		if (uch < 0x80) {
			utfForm[lenU++] = uch;
		} else {
			utfForm[lenU++] = static_cast<char>(0xC0 | (uch >> 6));
			utfForm[lenU++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
	}
	utfForm.resize(lenU);
	return utfForm;
}

static std::string UTF8FromIconv(const Converter &conv, const char *s, int len) {
	if (conv) {
		std::string utfForm(len*3+1, '\0');
		char *pin = const_cast<char *>(s);
		size_t inLeft = len;
		char *putf = &utfForm[0];
		char *pout = putf;
		size_t outLeft = len*3+1;
		size_t conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions != ((size_t)(-1))) {
			*pout = '\0';
			utfForm.resize(pout - putf);
			return utfForm;
		}
	}
	return std::string();
}

// Work out how many bytes are in a character by trying to convert using iconv,
// returning the first length that succeeds.
static size_t MultiByteLenFromIconv(const Converter &conv, const char *s, size_t len) {
	for (size_t lenMB=1; (lenMB<4) && (lenMB <= len); lenMB++) {
		char wcForm[2];
		char *pin = const_cast<char *>(s);
		size_t inLeft = lenMB;
		char *pout = wcForm;
		size_t outLeft = 2;
		size_t conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions != ((size_t)(-1))) {
			return lenMB;
		}
	}
	return 1;
}

void SurfaceImpl::DrawTextBase(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
                                 ColourDesired fore) {
	PenColour(fore);
	if (context) {
		XYPOSITION xText = rc.left;
		if (PFont(font_)->pfd) {
			std::string utfForm;
			if (et == UTF8) {
				pango_layout_set_text(layout, s, len);
			} else {
				SetConverter(PFont(font_)->characterSet);
				utfForm = UTF8FromIconv(conv, s, len);
				if (utfForm.empty()) {	// iconv failed so treat as Latin1
					utfForm = UTF8FromLatin1(s, len);
				}
				pango_layout_set_text(layout, utfForm.c_str(), utfForm.length());
			}
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
			pango_cairo_update_layout(context, layout);
#ifdef PANGO_VERSION
			PangoLayoutLine *pll = pango_layout_get_line_readonly(layout,0);
#else
			PangoLayoutLine *pll = pango_layout_get_line(layout,0);
#endif
			cairo_move_to(context, xText, ybase);
			pango_cairo_show_layout_line(context, pll);
		}
	}
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
                                 ColourDesired fore, ColourDesired back) {
	FillRectangle(rc, back);
	DrawTextBase(rc, font_, ybase, s, len, fore);
}

// On GTK+, exactly same as DrawTextNoClip
void SurfaceImpl::DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
                                  ColourDesired fore, ColourDesired back) {
	FillRectangle(rc, back);
	DrawTextBase(rc, font_, ybase, s, len, fore);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
                                  ColourDesired fore) {
	// Avoid drawing spaces in transparent mode
	for (int i=0; i<len; i++) {
		if (s[i] != ' ') {
			DrawTextBase(rc, font_, ybase, s, len, fore);
			return;
		}
	}
}

class ClusterIterator {
	PangoLayoutIter *iter;
	PangoRectangle pos;
	int lenPositions;
public:
	bool finished;
	XYPOSITION positionStart;
	XYPOSITION position;
	XYPOSITION distance;
	int curIndex;
	ClusterIterator(PangoLayout *layout, int len) : lenPositions(len), finished(false),
		positionStart(0), position(0), distance(0), curIndex(0) {
		iter = pango_layout_get_iter(layout);
		pango_layout_iter_get_cluster_extents(iter, NULL, &pos);
	}
	~ClusterIterator() {
		pango_layout_iter_free(iter);
	}

	void Next() {
		positionStart = position;
		if (pango_layout_iter_next_cluster(iter)) {
			pango_layout_iter_get_cluster_extents(iter, NULL, &pos);
			position = doubleFromPangoUnits(pos.x);
			curIndex = pango_layout_iter_get_index(iter);
		} else {
			finished = true;
			position = doubleFromPangoUnits(pos.x + pos.width);
			curIndex = lenPositions;
		}
		distance = position - positionStart;
	}
};

void SurfaceImpl::MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions) {
	if (font_.GetID()) {
		const int lenPositions = len;
		if (PFont(font_)->pfd) {
			if (len == 1) {
				int width = PFont(font_)->CharWidth(*s, et);
				if (width) {
					positions[0] = width;
					return;
				}
			}
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
			if (et == UTF8) {
				// Simple and direct as UTF-8 is native Pango encoding
				int i = 0;
				pango_layout_set_text(layout, s, len);
				ClusterIterator iti(layout, lenPositions);
				while (!iti.finished) {
					iti.Next();
					int places = iti.curIndex - i;
					while (i < iti.curIndex) {
						// Evenly distribute space among bytes of this cluster.
						// Would be better to find number of characters and then
						// divide evenly between characters with each byte of a character
						// being at the same position.
						positions[i] = iti.position - (iti.curIndex - 1 - i) * iti.distance / places;
						i++;
					}
				}
				PLATFORM_ASSERT(i == lenPositions);
			} else {
				int positionsCalculated = 0;
				if (et == dbcs) {
					SetConverter(PFont(font_)->characterSet);
					std::string utfForm = UTF8FromIconv(conv, s, len);
					if (!utfForm.empty()) {
						// Convert to UTF-8 so can ask Pango for widths, then
						// Loop through UTF-8 and DBCS forms, taking account of different
						// character byte lengths.
						Converter convMeasure("UCS-2", CharacterSetID(characterSet), false);
						pango_layout_set_text(layout, utfForm.c_str(), strlen(utfForm.c_str()));
						int i = 0;
						int clusterStart = 0;
						ClusterIterator iti(layout, strlen(utfForm.c_str()));
						while (!iti.finished) {
							iti.Next();
							int clusterEnd = iti.curIndex;
							int places = g_utf8_strlen(utfForm.c_str() + clusterStart, clusterEnd - clusterStart);
							int place = 1;
							while (clusterStart < clusterEnd) {
								size_t lenChar = MultiByteLenFromIconv(convMeasure, s+i, len-i);
								while (lenChar--) {
									positions[i++] = iti.position - (places - place) * iti.distance / places;
									positionsCalculated++;
								}
								clusterStart += UTF8CharLength(static_cast<unsigned char>(utfForm.c_str()[clusterStart]));
								place++;
							}
						}
						PLATFORM_ASSERT(i == lenPositions);
					}
				}
				if (positionsCalculated < 1 ) {
					// Either 8-bit or DBCS conversion failed so treat as 8-bit.
					SetConverter(PFont(font_)->characterSet);
					const bool rtlCheck = PFont(font_)->characterSet == SC_CHARSET_HEBREW ||
						PFont(font_)->characterSet == SC_CHARSET_ARABIC;
					std::string utfForm = UTF8FromIconv(conv, s, len);
					if (utfForm.empty()) {
						utfForm = UTF8FromLatin1(s, len);
					}
					pango_layout_set_text(layout, utfForm.c_str(), utfForm.length());
					int i = 0;
					int clusterStart = 0;
					// Each 8-bit input character may take 1 or 2 bytes in UTF-8
					// and groups of up to 3 may be represented as ligatures.
					ClusterIterator iti(layout, utfForm.length());
					while (!iti.finished) {
						iti.Next();
						int clusterEnd = iti.curIndex;
						int ligatureLength = g_utf8_strlen(utfForm.c_str() + clusterStart, clusterEnd - clusterStart);
						if (rtlCheck && ((clusterEnd <= clusterStart) || (ligatureLength == 0) || (ligatureLength > 3))) {
							// Something has gone wrong: exit quickly but pretend all the characters are equally spaced:
							int widthLayout = 0;
							pango_layout_get_size(layout, &widthLayout, NULL);
							XYPOSITION widthTotal = doubleFromPangoUnits(widthLayout);
							for (int bytePos=0; bytePos<lenPositions; bytePos++) {
								positions[bytePos] = widthTotal / lenPositions * (bytePos + 1);
							}
							return;
						}
						PLATFORM_ASSERT(ligatureLength > 0 && ligatureLength <= 3);
						for (int charInLig=0; charInLig<ligatureLength; charInLig++) {
							positions[i++] = iti.position - (ligatureLength - 1 - charInLig) * iti.distance / ligatureLength;
						}
						clusterStart = clusterEnd;
					}
					while (i < lenPositions) {
						// If something failed, fill in rest of the positions
						positions[i++] = clusterStart;
					}
					PLATFORM_ASSERT(i == lenPositions);
				}
			}
			if (len == 1) {
				PFont(font_)->SetCharWidth(*s, positions[0], et);
			}
			return;
		}
	} else {
		// No font so return an ascending range of values
		for (int i = 0; i < len; i++) {
			positions[i] = i + 1;
		}
	}
}

XYPOSITION SurfaceImpl::WidthText(Font &font_, const char *s, int len) {
	if (font_.GetID()) {
		if (PFont(font_)->pfd) {
			std::string utfForm;
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
			PangoRectangle pos;
			if (et == UTF8) {
				pango_layout_set_text(layout, s, len);
			} else {
				SetConverter(PFont(font_)->characterSet);
				utfForm = UTF8FromIconv(conv, s, len);
				if (utfForm.empty()) {	// iconv failed so treat as Latin1
					utfForm = UTF8FromLatin1(s, len);
				}
				pango_layout_set_text(layout, utfForm.c_str(), utfForm.length());
			}
#ifdef PANGO_VERSION
			PangoLayoutLine *pangoLine = pango_layout_get_line_readonly(layout,0);
#else
			PangoLayoutLine *pangoLine = pango_layout_get_line(layout,0);
#endif
			pango_layout_line_get_extents(pangoLine, NULL, &pos);
			return doubleFromPangoUnits(pos.width);
		}
		return 1;
	} else {
		return 1;
	}
}

XYPOSITION SurfaceImpl::WidthChar(Font &font_, char ch) {
	if (font_.GetID()) {
		if (PFont(font_)->pfd) {
			return WidthText(font_, &ch, 1);
		}
		return 1;
	} else {
		return 1;
	}
}

// Ascent and descent determined by Pango font metrics.

XYPOSITION SurfaceImpl::Ascent(Font &font_) {
	if (!(font_.GetID()))
		return 1;
	FontMutexLock();
	int ascent = PFont(font_)->ascent;
	if ((ascent == 0) && (PFont(font_)->pfd)) {
		PangoFontMetrics *metrics = pango_context_get_metrics(pcontext,
			PFont(font_)->pfd, pango_context_get_language(pcontext));
		PFont(font_)->ascent =
			doubleFromPangoUnits(pango_font_metrics_get_ascent(metrics));
		pango_font_metrics_unref(metrics);
		ascent = PFont(font_)->ascent;
	}
	if (ascent == 0) {
		ascent = 1;
	}
	FontMutexUnlock();
	return ascent;
}

XYPOSITION SurfaceImpl::Descent(Font &font_) {
	if (!(font_.GetID()))
		return 1;
	if (PFont(font_)->pfd) {
		PangoFontMetrics *metrics = pango_context_get_metrics(pcontext,
			PFont(font_)->pfd, pango_context_get_language(pcontext));
		int descent = doubleFromPangoUnits(pango_font_metrics_get_descent(metrics));
		pango_font_metrics_unref(metrics);
		return descent;
	}
	return 0;
}

XYPOSITION SurfaceImpl::InternalLeading(Font &) {
	return 0;
}

XYPOSITION SurfaceImpl::ExternalLeading(Font &) {
	return 0;
}

XYPOSITION SurfaceImpl::Height(Font &font_) {
	return Ascent(font_) + Descent(font_);
}

XYPOSITION SurfaceImpl::AverageCharWidth(Font &font_) {
	return WidthChar(font_, 'n');
}

void SurfaceImpl::SetClip(PRectangle rc) {
	PLATFORM_ASSERT(context);
	cairo_rectangle(context, rc.left, rc.top, rc.right, rc.bottom);
	cairo_clip(context);
}

void SurfaceImpl::FlushCachedState() {}

void SurfaceImpl::SetUnicodeMode(bool unicodeMode_) {
	if (unicodeMode_)
		et = UTF8;
}

void SurfaceImpl::SetDBCSMode(int codePage) {
	if (codePage && (codePage != SC_CP_UTF8))
		et = dbcs;
}

Surface *Surface::Allocate(int) {
	return new SurfaceImpl();
}

Window::~Window() {}

void Window::Destroy() {
	if (wid) {
		ListBox *listbox = dynamic_cast<ListBox*>(this);
		if (listbox) {
			gtk_widget_hide(GTK_WIDGET(wid));
			// clear up window content
			listbox->Clear();
			// resize the window to the smallest possible size for it to adapt
			// to future content
			gtk_window_resize(GTK_WINDOW(wid), 1, 1);
		} else {
			gtk_widget_destroy(GTK_WIDGET(wid));
		}
		wid = 0;
	}
}

bool Window::HasFocus() {
	return gtk_widget_has_focus(GTK_WIDGET(wid));
}

PRectangle Window::GetPosition() {
	// Before any size allocated pretend its 1000 wide so not scrolled
	PRectangle rc(0, 0, 1000, 1000);
	if (wid) {
		GtkAllocation allocation;
		gtk_widget_get_allocation(PWidget(wid), &allocation);
		rc.left = allocation.x;
		rc.top = allocation.y;
		if (allocation.width > 20) {
			rc.right = rc.left + allocation.width;
			rc.bottom = rc.top + allocation.height;
		}
	}
	return rc;
}

void Window::SetPosition(PRectangle rc) {
	GtkAllocation alloc;
	alloc.x = rc.left;
	alloc.y = rc.top;
	alloc.width = rc.Width();
	alloc.height = rc.Height();
	gtk_widget_size_allocate(PWidget(wid), &alloc);
}

void Window::SetPositionRelative(PRectangle rc, Window relativeTo) {
	int ox = 0;
	int oy = 0;
	gdk_window_get_origin(WindowFromWidget(PWidget(relativeTo.wid)), &ox, &oy);
	ox += rc.left;
	if (ox < 0)
		ox = 0;
	oy += rc.top;
	if (oy < 0)
		oy = 0;

	/* do some corrections to fit into screen */
	int sizex = rc.right - rc.left;
	int sizey = rc.bottom - rc.top;
	int screenWidth = gdk_screen_width();
	int screenHeight = gdk_screen_height();
	if (sizex > screenWidth)
		ox = 0; /* the best we can do */
	else if (ox + sizex > screenWidth)
		ox = screenWidth - sizex;
	if (oy + sizey > screenHeight)
		oy = screenHeight - sizey;

	gtk_window_move(GTK_WINDOW(PWidget(wid)), ox, oy);

	gtk_window_resize(GTK_WINDOW(wid), sizex, sizey);
}

PRectangle Window::GetClientPosition() {
	// On GTK+, the client position is the window position
	return GetPosition();
}

void Window::Show(bool show) {
	if (show)
		gtk_widget_show(PWidget(wid));
}

void Window::InvalidateAll() {
	if (wid) {
		gtk_widget_queue_draw(PWidget(wid));
	}
}

void Window::InvalidateRectangle(PRectangle rc) {
	if (wid) {
		gtk_widget_queue_draw_area(PWidget(wid),
		                           rc.left, rc.top,
		                           rc.right - rc.left, rc.bottom - rc.top);
	}
}

void Window::SetFont(Font &) {
	// Can not be done generically but only needed for ListBox
}

void Window::SetCursor(Cursor curs) {
	// We don't set the cursor to same value numerous times under gtk because
	// it stores the cursor in the window once it's set
	if (curs == cursorLast)
		return;

	cursorLast = curs;
	GdkDisplay *pdisplay = gtk_widget_get_display(PWidget(wid));

	GdkCursor *gdkCurs;
	switch (curs) {
	case cursorText:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_XTERM);
		break;
	case cursorArrow:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_LEFT_PTR);
		break;
	case cursorUp:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_CENTER_PTR);
		break;
	case cursorWait:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_WATCH);
		break;
	case cursorHand:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_HAND2);
		break;
	case cursorReverseArrow:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_RIGHT_PTR);
		break;
	default:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_LEFT_PTR);
		cursorLast = cursorArrow;
		break;
	}

	if (WindowFromWidget(PWidget(wid)))
		gdk_window_set_cursor(WindowFromWidget(PWidget(wid)), gdkCurs);
#if GTK_CHECK_VERSION(3,0,0)
	g_object_unref(gdkCurs);
#else
	gdk_cursor_unref(gdkCurs);
#endif
}

void Window::SetTitle(const char *s) {
	gtk_window_set_title(GTK_WINDOW(wid), s);
}

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   gdk window coordinates */
PRectangle Window::GetMonitorRect(Point pt) {
	gint x_offset, y_offset;

	gdk_window_get_origin(WindowFromWidget(PWidget(wid)), &x_offset, &y_offset);

	GdkScreen* screen;
	gint monitor_num;
	GdkRectangle rect;

	screen = gtk_widget_get_screen(PWidget(wid));
	monitor_num = gdk_screen_get_monitor_at_point(screen, pt.x + x_offset, pt.y + y_offset);
	gdk_screen_get_monitor_geometry(screen, monitor_num, &rect);
	rect.x -= x_offset;
	rect.y -= y_offset;
	return PRectangle(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
}

typedef std::map<int, RGBAImage*> ImageMap;

struct ListImage {
	const RGBAImage *rgba_data;
	GdkPixbuf *pixbuf;
};

static void list_image_free(gpointer, gpointer value, gpointer) {
	ListImage *list_image = static_cast<ListImage *>(value);
	if (list_image->pixbuf)
		g_object_unref(list_image->pixbuf);
	g_free(list_image);
}

ListBox::ListBox() {
}

ListBox::~ListBox() {
}

enum {
	PIXBUF_COLUMN,
	TEXT_COLUMN,
	N_COLUMNS
};

class ListBoxX : public ListBox {
	WindowID widCached;
	WindowID frame;
	WindowID list;
	WindowID scroller;
	void *pixhash;
	GtkCellRenderer* pixbuf_renderer;
	GtkCellRenderer* renderer;
	RGBAImageSet images;
	int desiredVisibleRows;
	unsigned int maxItemCharacters;
	unsigned int aveCharWidth;
#if GTK_CHECK_VERSION(3,0,0)
	GtkCssProvider *cssProvider;
#endif
public:
	CallBackAction doubleClickAction;
	void *doubleClickActionData;

	ListBoxX() : widCached(0), frame(0), list(0), scroller(0), pixhash(NULL), pixbuf_renderer(0),
		renderer(0),
		desiredVisibleRows(5), maxItemCharacters(0),
		aveCharWidth(1),
#if GTK_CHECK_VERSION(3,0,0)
		cssProvider(NULL),
#endif
		doubleClickAction(NULL), doubleClickActionData(NULL) {
	}
	virtual ~ListBoxX() {
		if (pixhash) {
			g_hash_table_foreach((GHashTable *) pixhash, list_image_free, NULL);
			g_hash_table_destroy((GHashTable *) pixhash);
		}
		if (widCached) {
			gtk_widget_destroy(GTK_WIDGET(widCached));
			wid = widCached = 0;
		}
#if GTK_CHECK_VERSION(3,0,0)
		if (cssProvider) {
			g_object_unref(cssProvider);
			cssProvider = NULL;
		}
#endif
	}
	virtual void SetFont(Font &font);
	virtual void Create(Window &parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_, int technology_);
	virtual void SetAverageCharWidth(int width);
	virtual void SetVisibleRows(int rows);
	virtual int GetVisibleRows() const;
	int GetRowHeight();
	virtual PRectangle GetDesiredRect();
	virtual int CaretFromEdge();
	virtual void Clear();
	virtual void Append(char *s, int type = -1);
	virtual int Length();
	virtual void Select(int n);
	virtual int GetSelection();
	virtual int Find(const char *prefix);
	virtual void GetValue(int n, char *value, int len);
	void RegisterRGBA(int type, RGBAImage *image);
	virtual void RegisterImage(int type, const char *xpm_data);
	virtual void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage);
	virtual void ClearRegisteredImages();
	virtual void SetDoubleClickAction(CallBackAction action, void *data) {
		doubleClickAction = action;
		doubleClickActionData = data;
	}
	virtual void SetList(const char *listText, char separator, char typesep);
};

ListBox *ListBox::Allocate() {
	ListBoxX *lb = new ListBoxX();
	return lb;
}

// SmallScroller, a GtkScrolledWindow that can shrink very small, as
// gtk_widget_set_size_request() cannot shrink widgets on GTK3
typedef struct {
	GtkScrolledWindow parent;
	/* Workaround ABI issue with Windows GTK2 bundle and GCC > 3.
	   See http://lists.geany.org/pipermail/devel/2015-April/thread.html#9379

	   GtkScrolledWindow contains a bitfield, and GCC 3.4 and 4.8 don't agree
	   on the size of the structure (regardless of -mms-bitfields):
	   - GCC 3.4 has sizeof(GtkScrolledWindow)=88
	   - GCC 4.8 has sizeof(GtkScrolledWindow)=84
	   As Windows GTK2 bundle is built with GCC 3, it requires types derived
	   from GtkScrolledWindow to be at least 88 bytes, which means we need to
	   add some fake padding to fill in the extra 4 bytes.
	   There is however no other issue with the layout difference as we never
	   access any GtkScrolledWindow fields ourselves. */
	int padding;
} SmallScroller;
typedef GtkScrolledWindowClass SmallScrollerClass;

G_DEFINE_TYPE(SmallScroller, small_scroller, GTK_TYPE_SCROLLED_WINDOW)

#if GTK_CHECK_VERSION(3,0,0)
static void small_scroller_get_preferred_height(GtkWidget *widget, gint *min, gint *nat) {
	GTK_WIDGET_CLASS(small_scroller_parent_class)->get_preferred_height(widget, min, nat);
	if (*min > 1)
		*min = 1;
}
#else
static void small_scroller_size_request(GtkWidget *widget, GtkRequisition *req) {
	GTK_WIDGET_CLASS(small_scroller_parent_class)->size_request(widget, req);
	req->height = 1;
}
#endif

static void small_scroller_class_init(SmallScrollerClass *klass) {
#if GTK_CHECK_VERSION(3,0,0)
	GTK_WIDGET_CLASS(klass)->get_preferred_height = small_scroller_get_preferred_height;
#else
	GTK_WIDGET_CLASS(klass)->size_request = small_scroller_size_request;
#endif
}

static void small_scroller_init(SmallScroller *){}

static gboolean ButtonPress(GtkWidget *, GdkEventButton* ev, gpointer p) {
	try {
		ListBoxX* lb = static_cast<ListBoxX*>(p);
		if (ev->type == GDK_2BUTTON_PRESS && lb->doubleClickAction != NULL) {
			lb->doubleClickAction(lb->doubleClickActionData);
			return TRUE;
		}

	} catch (...) {
		// No pointer back to Scintilla to save status
	}
	return FALSE;
}

/* Change the active color to the selected color so the listbox uses the color
scheme that it would use if it had the focus. */
static void StyleSet(GtkWidget *w, GtkStyle*, void*) {

	g_return_if_fail(w != NULL);

	/* Copy the selected color to active.  Note that the modify calls will cause
	recursive calls to this function after the value is updated and w->style to
	be set to a new object */

#if GTK_CHECK_VERSION(3,16,0)
	// On recent releases of GTK+, it does not appear necessary to set the list box colours.
	// This may be because of common themes and may be needed with other themes.
	// The *override* calls are deprecated now, so only call them for older versions of GTK+.
#elif GTK_CHECK_VERSION(3,0,0)
	GtkStyleContext *styleContext = gtk_widget_get_style_context(w);
	if (styleContext == NULL)
		return;

	GdkRGBA colourForeSelected;
	gtk_style_context_get_color(styleContext, GTK_STATE_FLAG_SELECTED, &colourForeSelected);
	GdkRGBA colourForeActive;
	gtk_style_context_get_color(styleContext, GTK_STATE_FLAG_ACTIVE, &colourForeActive);
	if (!gdk_rgba_equal(&colourForeSelected, &colourForeActive))
		gtk_widget_override_color(w, GTK_STATE_FLAG_ACTIVE, &colourForeSelected);

	styleContext = gtk_widget_get_style_context(w);
	if (styleContext == NULL)
		return;

	GdkRGBA colourBaseSelected;
	gtk_style_context_get_background_color(styleContext, GTK_STATE_FLAG_SELECTED, &colourBaseSelected);
	GdkRGBA colourBaseActive;
	gtk_style_context_get_background_color(styleContext, GTK_STATE_FLAG_ACTIVE, &colourBaseActive);
	if (!gdk_rgba_equal(&colourBaseSelected, &colourBaseActive))
		gtk_widget_override_background_color(w, GTK_STATE_FLAG_ACTIVE, &colourBaseSelected);
#else
	GtkStyle *style = gtk_widget_get_style(w);
	if (style == NULL)
		return;
	if (!gdk_color_equal(&style->base[GTK_STATE_SELECTED], &style->base[GTK_STATE_ACTIVE]))
		gtk_widget_modify_base(w, GTK_STATE_ACTIVE, &style->base[GTK_STATE_SELECTED]);
	style = gtk_widget_get_style(w);
	if (style == NULL)
		return;
	if (!gdk_color_equal(&style->text[GTK_STATE_SELECTED], &style->text[GTK_STATE_ACTIVE]))
		gtk_widget_modify_text(w, GTK_STATE_ACTIVE, &style->text[GTK_STATE_SELECTED]);
#endif
}

void ListBoxX::Create(Window &, int, Point, int, bool, int) {
	if (widCached != 0) {
		wid = widCached;
		return;
	}

#if GTK_CHECK_VERSION(3,0,0)
	if (!cssProvider) {
		cssProvider = gtk_css_provider_new();
	}
#endif
	
	wid = widCached = gtk_window_new(GTK_WINDOW_POPUP);

	frame = gtk_frame_new(NULL);
	gtk_widget_show(PWidget(frame));
	gtk_container_add(GTK_CONTAINER(GetID()), PWidget(frame));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);

	scroller = g_object_new(small_scroller_get_type(), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scroller), 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
	                               GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), PWidget(scroller));
	gtk_widget_show(PWidget(scroller));

	/* Tree and its model */
	GtkListStore *store =
		gtk_list_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_signal_connect(G_OBJECT(list), "style-set", G_CALLBACK(StyleSet), NULL);

#if GTK_CHECK_VERSION(3,0,0)
	GtkStyleContext *styleContext = gtk_widget_get_style_context(GTK_WIDGET(list));
	if (styleContext) {
		gtk_style_context_add_provider(styleContext, GTK_STYLE_PROVIDER(cssProvider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
#endif

	GtkTreeSelection *selection =
		gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(list), FALSE);

	/* Columns */
	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_title(column, "Autocomplete");

	pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_fixed_size(pixbuf_renderer, 0, -1);
	gtk_tree_view_column_pack_start(column, pixbuf_renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, pixbuf_renderer,
										"pixbuf", PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer,
										"text", TEXT_COLUMN);

	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	if (g_object_class_find_property(G_OBJECT_GET_CLASS(list), "fixed-height-mode"))
		g_object_set(G_OBJECT(list), "fixed-height-mode", TRUE, NULL);

	GtkWidget *widget = PWidget(list);	// No code inside the G_OBJECT macro
	gtk_container_add(GTK_CONTAINER(PWidget(scroller)), widget);
	gtk_widget_show(widget);
	g_signal_connect(G_OBJECT(widget), "button_press_event",
	                   G_CALLBACK(ButtonPress), this);
}

void ListBoxX::SetFont(Font &scint_font) {
	// Only do for Pango font as there have been crashes for GDK fonts
	if (Created() && PFont(scint_font)->pfd) {
		// Current font is Pango font
#if GTK_CHECK_VERSION(3,0,0)
		if (cssProvider) {
			PangoFontDescription *pfd = PFont(scint_font)->pfd;
			std::ostringstream ssFontSetting;
			ssFontSetting << "GtkTreeView { ";
			ssFontSetting << "font-family: " << pango_font_description_get_family(pfd) <<  "; ";
			ssFontSetting << "font-size:";
			ssFontSetting << static_cast<double>(pango_font_description_get_size(pfd)) / PANGO_SCALE;
			ssFontSetting << "px; ";
			ssFontSetting << "font-weight:"<< pango_font_description_get_weight(pfd) << "; ";
			ssFontSetting << "}";
			gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(cssProvider),
				ssFontSetting.str().c_str(), -1, NULL);
		}
#else
		gtk_widget_modify_font(PWidget(list), PFont(scint_font)->pfd);
#endif
		gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), -1);
		gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
	}
}

void ListBoxX::SetAverageCharWidth(int width) {
	aveCharWidth = width;
}

void ListBoxX::SetVisibleRows(int rows) {
	desiredVisibleRows = rows;
}

int ListBoxX::GetVisibleRows() const {
	return desiredVisibleRows;
}

int ListBoxX::GetRowHeight()
{
#if GTK_CHECK_VERSION(3,0,0)
	// This version sometimes reports erroneous results on GTK2, but the GTK2
	// version is inaccurate for GTK 3.14.
	GdkRectangle rect;
	GtkTreePath *path = gtk_tree_path_new_first();
	gtk_tree_view_get_background_area(GTK_TREE_VIEW(list), path, NULL, &rect);
	return rect.height;
#else
	int row_height=0;
	int vertical_separator=0;
	int expander_size=0;
	GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
	gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &row_height);
	gtk_widget_style_get(PWidget(list),
		"vertical-separator", &vertical_separator,
		"expander-size", &expander_size, NULL);
	row_height += vertical_separator;
	row_height = Platform::Maximum(row_height, expander_size);
	return row_height;
#endif
}

PRectangle ListBoxX::GetDesiredRect() {
	// Before any size allocated pretend its 100 wide so not scrolled
	PRectangle rc(0, 0, 100, 100);
	if (wid) {
		int rows = Length();
		if ((rows == 0) || (rows > desiredVisibleRows))
			rows = desiredVisibleRows;

		GtkRequisition req;
		// This, apparently unnecessary call, ensures gtk_tree_view_column_cell_get_size
		// returns reasonable values.
#if GTK_CHECK_VERSION(3,0,0)
		gtk_widget_get_preferred_size(GTK_WIDGET(frame), NULL, &req);
#else
		gtk_widget_size_request(GTK_WIDGET(frame), &req);
#endif
		int height;

		// First calculate height of the clist for our desired visible
		// row count otherwise it tries to expand to the total # of rows
		// Get cell height
		int row_height = GetRowHeight();
#if GTK_CHECK_VERSION(3,0,0)
		GtkStyleContext *styleContextFrame = gtk_widget_get_style_context(PWidget(frame));
		GtkBorder padding, border;
		gtk_style_context_get_padding(styleContextFrame, GTK_STATE_FLAG_NORMAL, &padding);
		gtk_style_context_get_border(styleContextFrame, GTK_STATE_FLAG_NORMAL, &border);
		height = (rows * row_height
		          + padding.top + padding.bottom
		          + border.top + border.bottom
		          + 2 * gtk_container_get_border_width(GTK_CONTAINER(PWidget(list))));
#else
		height = (rows * row_height
		          + 2 * (PWidget(frame)->style->ythickness
		                 + GTK_CONTAINER(PWidget(list))->border_width));
#endif
		rc.bottom = height;

		int width = maxItemCharacters;
		if (width < 12)
			width = 12;
		rc.right = width * (aveCharWidth + aveCharWidth / 3);
		// Add horizontal padding and borders
		int horizontal_separator=0;
		gtk_widget_style_get(PWidget(list),
			"horizontal-separator", &horizontal_separator, NULL);
		rc.right += horizontal_separator;
#if GTK_CHECK_VERSION(3,0,0)
		rc.right += (padding.left + padding.right
		             + border.left + border.right
		             + 2 * gtk_container_get_border_width(GTK_CONTAINER(PWidget(list))));
#else
		rc.right += 2 * (PWidget(frame)->style->xthickness
		                 + GTK_CONTAINER(PWidget(list))->border_width);
#endif
		if (Length() > rows) {
			// Add the width of the scrollbar
			GtkWidget *vscrollbar =
				gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(scroller));
#if GTK_CHECK_VERSION(3,0,0)
			gtk_widget_get_preferred_size(vscrollbar, NULL, &req);
#else
			gtk_widget_size_request(vscrollbar, &req);
#endif
			rc.right += req.width;
		}
	}
	return rc;
}

int ListBoxX::CaretFromEdge() {
	gint renderer_width, renderer_height;
	gtk_cell_renderer_get_fixed_size(pixbuf_renderer, &renderer_width,
						&renderer_height);
	return 4 + renderer_width;
}

void ListBoxX::Clear() {
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	maxItemCharacters = 0;
}

static void init_pixmap(ListImage *list_image) {
	if (list_image->rgba_data) {
		// Drop any existing pixmap/bitmap as data may have changed
		if (list_image->pixbuf)
			g_object_unref(list_image->pixbuf);
		list_image->pixbuf =
			gdk_pixbuf_new_from_data(list_image->rgba_data->Pixels(),
                                                         GDK_COLORSPACE_RGB,
                                                         TRUE,
                                                         8,
                                                         list_image->rgba_data->GetWidth(),
                                                         list_image->rgba_data->GetHeight(),
                                                         list_image->rgba_data->GetWidth() * 4,
                                                         NULL,
                                                         NULL);
	}
}

#define SPACING 5

void ListBoxX::Append(char *s, int type) {
	ListImage *list_image = NULL;
	if ((type >= 0) && pixhash) {
		list_image = static_cast<ListImage *>(g_hash_table_lookup((GHashTable *) pixhash
		             , (gconstpointer) GINT_TO_POINTER(type)));
	}
	GtkTreeIter iter;
	GtkListStore *store =
		GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	if (list_image) {
		if (NULL == list_image->pixbuf)
			init_pixmap(list_image);
		if (list_image->pixbuf) {
			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
								PIXBUF_COLUMN, list_image->pixbuf,
								TEXT_COLUMN, s, -1);

			gint pixbuf_width = gdk_pixbuf_get_width(list_image->pixbuf);
			gint renderer_height, renderer_width;
			gtk_cell_renderer_get_fixed_size(pixbuf_renderer,
								&renderer_width, &renderer_height);
			if (pixbuf_width > renderer_width)
				gtk_cell_renderer_set_fixed_size(pixbuf_renderer,
								pixbuf_width, -1);
		} else {
			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
								TEXT_COLUMN, s, -1);
		}
	} else {
			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
								TEXT_COLUMN, s, -1);
	}
	size_t len = strlen(s);
	if (maxItemCharacters < len)
		maxItemCharacters = len;
}

int ListBoxX::Length() {
	if (wid)
		return gtk_tree_model_iter_n_children(gtk_tree_view_get_model
											   (GTK_TREE_VIEW(list)), NULL);
	return 0;
}

void ListBoxX::Select(int n) {
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	GtkTreeSelection *selection =
		gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

	if (n < 0) {
		gtk_tree_selection_unselect_all(selection);
		return;
	}

	bool valid = gtk_tree_model_iter_nth_child(model, &iter, NULL, n) != FALSE;
	if (valid) {
		gtk_tree_selection_select_iter(selection, &iter);

		// Move the scrollbar to show the selection.
		int total = Length();
#if GTK_CHECK_VERSION(3,0,0)
		GtkAdjustment *adj =
			gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(list));
#else
		GtkAdjustment *adj =
			gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(list));
#endif
		gfloat value = ((gfloat)n / total) * (gtk_adjustment_get_upper(adj) - gtk_adjustment_get_lower(adj))
							+ gtk_adjustment_get_lower(adj) - gtk_adjustment_get_page_size(adj) / 2;
		// Get cell height
		int row_height = GetRowHeight();

		int rows = Length();
		if ((rows == 0) || (rows > desiredVisibleRows))
			rows = desiredVisibleRows;
		if (rows & 0x1) {
			// Odd rows to display -- We are now in the middle.
			// Align it so that we don't chop off rows.
			value += (gfloat)row_height / 2.0;
		}
		// Clamp it.
		value = (value < 0)? 0 : value;
		value = (value > (gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj)))?
					(gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj)) : value;

		// Set it.
		gtk_adjustment_set_value(adj, value);
	} else {
		gtk_tree_selection_unselect_all(selection);
	}
}

int ListBoxX::GetSelection() {
	int index = -1;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		int *indices = gtk_tree_path_get_indices(path);
		// Don't free indices.
		if (indices)
			index = indices[0];
		gtk_tree_path_free(path);
	}
	return index;
}

int ListBoxX::Find(const char *prefix) {
	GtkTreeIter iter;
	GtkTreeModel *model =
		gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	bool valid = gtk_tree_model_get_iter_first(model, &iter) != FALSE;
	int i = 0;
	while(valid) {
		gchar *s;
		gtk_tree_model_get(model, &iter, TEXT_COLUMN, &s, -1);
		if (s && (0 == strncmp(prefix, s, strlen(prefix)))) {
			g_free(s);
			return i;
		}
		g_free(s);
		valid = gtk_tree_model_iter_next(model, &iter) != FALSE;
		i++;
	}
	return -1;
}

void ListBoxX::GetValue(int n, char *value, int len) {
	char *text = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	bool valid = gtk_tree_model_iter_nth_child(model, &iter, NULL, n) != FALSE;
	if (valid) {
		gtk_tree_model_get(model, &iter, TEXT_COLUMN, &text, -1);
	}
	if (text && len > 0) {
		g_strlcpy(value, text, len);
	} else {
		value[0] = '\0';
	}
	g_free(text);
}

// g_return_if_fail causes unnecessary compiler warning in release compile.
#ifdef _MSC_VER
#pragma warning(disable: 4127)
#endif

void ListBoxX::RegisterRGBA(int type, RGBAImage *image) {
	images.Add(type, image);

	if (!pixhash) {
		pixhash = g_hash_table_new(g_direct_hash, g_direct_equal);
	}
	ListImage *list_image = static_cast<ListImage *>(g_hash_table_lookup((GHashTable *) pixhash,
		(gconstpointer) GINT_TO_POINTER(type)));
	if (list_image) {
		// Drop icon already registered
		if (list_image->pixbuf)
			g_object_unref(list_image->pixbuf);
		list_image->pixbuf = NULL;
		list_image->rgba_data = image;
	} else {
		list_image = g_new0(ListImage, 1);
		list_image->rgba_data = image;
		g_hash_table_insert((GHashTable *) pixhash, GINT_TO_POINTER(type),
			(gpointer) list_image);
	}
}

void ListBoxX::RegisterImage(int type, const char *xpm_data) {
	g_return_if_fail(xpm_data);
	XPM xpmImage(xpm_data);
	RegisterRGBA(type, new RGBAImage(xpmImage));
}

void ListBoxX::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) {
	RegisterRGBA(type, new RGBAImage(width, height, 1.0, pixelsImage));
}

void ListBoxX::ClearRegisteredImages() {
	images.Clear();
}

void ListBoxX::SetList(const char *listText, char separator, char typesep) {
	Clear();
	int count = strlen(listText) + 1;
	std::vector<char> words(listText, listText+count);
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

Menu::Menu() : mid(0) {}

void Menu::CreatePopUp() {
	Destroy();
	mid = gtk_menu_new();
	g_object_ref_sink(G_OBJECT(mid));
}

void Menu::Destroy() {
	if (mid)
		g_object_unref(G_OBJECT(mid));
	mid = 0;
}

static void MenuPositionFunc(GtkMenu *, gint *x, gint *y, gboolean *, gpointer userData) {
	sptr_t intFromPointer = GPOINTER_TO_INT(userData);
	*x = intFromPointer & 0xffff;
	*y = intFromPointer >> 16;
}

void Menu::Show(Point pt, Window &) {
	int screenHeight = gdk_screen_height();
	int screenWidth = gdk_screen_width();
	GtkMenu *widget = static_cast<GtkMenu *>(mid);
	gtk_widget_show_all(GTK_WIDGET(widget));
	GtkRequisition requisition;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_get_preferred_size(GTK_WIDGET(widget), NULL, &requisition);
#else
	gtk_widget_size_request(GTK_WIDGET(widget), &requisition);
#endif
	if ((pt.x + requisition.width) > screenWidth) {
		pt.x = screenWidth - requisition.width;
	}
	if ((pt.y + requisition.height) > screenHeight) {
		pt.y = screenHeight - requisition.height;
	}
	gtk_menu_popup(widget, NULL, NULL, MenuPositionFunc,
		GINT_TO_POINTER((static_cast<int>(pt.y) << 16) | static_cast<int>(pt.x)), 0,
		gtk_get_current_event_time());
}

ElapsedTime::ElapsedTime() {
	GTimeVal curTime;
	g_get_current_time(&curTime);
	bigBit = curTime.tv_sec;
	littleBit = curTime.tv_usec;
}

class DynamicLibraryImpl : public DynamicLibrary {
protected:
	GModule* m;
public:
	explicit DynamicLibraryImpl(const char *modulePath) {
		m = g_module_open(modulePath, G_MODULE_BIND_LAZY);
	}

	virtual ~DynamicLibraryImpl() {
		if (m != NULL)
			g_module_close(m);
	}

	// Use g_module_symbol to get a pointer to the relevant function.
	virtual Function FindFunction(const char *name) {
		if (m != NULL) {
			gpointer fn_address = NULL;
			gboolean status = g_module_symbol(m, name, &fn_address);
			if (status)
				return static_cast<Function>(fn_address);
			else
				return NULL;
		} else {
			return NULL;
		}
	}

	virtual bool IsValid() {
		return m != NULL;
	}
};

DynamicLibrary *DynamicLibrary::Load(const char *modulePath) {
	return static_cast<DynamicLibrary *>( new DynamicLibraryImpl(modulePath) );
}

double ElapsedTime::Duration(bool reset) {
	GTimeVal curTime;
	g_get_current_time(&curTime);
	long endBigBit = curTime.tv_sec;
	long endLittleBit = curTime.tv_usec;
	double result = 1000000.0 * (endBigBit - bigBit);
	result += endLittleBit - littleBit;
	result /= 1000000.0;
	if (reset) {
		bigBit = endBigBit;
		littleBit = endLittleBit;
	}
	return result;
}

ColourDesired Platform::Chrome() {
	return ColourDesired(0xe0, 0xe0, 0xe0);
}

ColourDesired Platform::ChromeHighlight() {
	return ColourDesired(0xff, 0xff, 0xff);
}

const char *Platform::DefaultFont() {
#ifdef G_OS_WIN32
	return "Lucida Console";
#else
	return "!Sans";
#endif
}

int Platform::DefaultFontSize() {
#ifdef G_OS_WIN32
	return 10;
#else
	return 12;
#endif
}

unsigned int Platform::DoubleClickTime() {
	return 500; 	// Half a second
}

bool Platform::MouseButtonBounce() {
	return true;
}

void Platform::DebugDisplay(const char *s) {
	fprintf(stderr, "%s", s);
}

bool Platform::IsKeyDown(int) {
	// TODO: discover state of keys in GTK+/X
	return false;
}

long Platform::SendScintilla(
    WindowID w, unsigned int msg, unsigned long wParam, long lParam) {
	return scintilla_send_message(SCINTILLA(w), msg, wParam, lParam);
}

long Platform::SendScintillaPointer(
    WindowID w, unsigned int msg, unsigned long wParam, void *lParam) {
	return scintilla_send_message(SCINTILLA(w), msg, wParam,
	                              reinterpret_cast<sptr_t>(lParam));
}

bool Platform::IsDBCSLeadByte(int codePage, char ch) {
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	unsigned char uch = static_cast<unsigned char>(ch);
	switch (codePage) {
		case 932:
			// Shift_jis
			return ((uch >= 0x81) && (uch <= 0x9F)) ||
				((uch >= 0xE0) && (uch <= 0xFC));
				// Lead bytes F0 to FC may be a Microsoft addition.
		case 936:
			// GBK
			return (uch >= 0x81) && (uch <= 0xFE);
		case 950:
			// Big5
			return (uch >= 0x81) && (uch <= 0xFE);
		// Korean EUC-KR may be code page 949.
	}
	return false;
}

int Platform::DBCSCharLength(int codePage, const char *s) {
	if (codePage == 932 || codePage == 936 || codePage == 950) {
		return IsDBCSLeadByte(codePage, s[0]) ? 2 : 1;
	} else {
		int bytes = mblen(s, MB_CUR_MAX);
		if (bytes >= 1)
			return bytes;
		else
			return 1;
	}
}

int Platform::DBCSCharMaxLength() {
	return MB_CUR_MAX;
	//return 2;
}

// These are utility functions not really tied to a platform

int Platform::Minimum(int a, int b) {
	if (a < b)
		return a;
	else
		return b;
}

int Platform::Maximum(int a, int b) {
	if (a > b)
		return a;
	else
		return b;
}

//#define TRACE

#ifdef TRACE
void Platform::DebugPrintf(const char *format, ...) {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsprintf(buffer, format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}
#else
void Platform::DebugPrintf(const char *, ...) {}

#endif

// Not supported for GTK+
static bool assertionPopUps = true;

bool Platform::ShowAssertionPopUps(bool assertionPopUps_) {
	bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) {
	char buffer[2000];
	g_snprintf(buffer, sizeof(buffer), "Assertion [%s] failed at %s %d\r\n", c, file, line);
	Platform::DebugDisplay(buffer);
	abort();
}

int Platform::Clamp(int val, int minVal, int maxVal) {
	if (val > maxVal)
		val = maxVal;
	if (val < minVal)
		val = minVal;
	return val;
}

void Platform_Initialise() {
	FontMutexAllocate();
}

void Platform_Finalise() {
	FontCached::ReleaseAll();
	FontMutexFree();
}
