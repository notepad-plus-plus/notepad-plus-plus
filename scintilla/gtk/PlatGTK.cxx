// Scintilla source code edit control
// PlatGTK.cxx - implementation of platform facilities on GTK+/Linux
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <sstream>

#include <glib.h>
#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "Platform.h"

#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "IntegerRectangle.h"
#include "XPM.h"
#include "UniConversion.h"

#include "Converter.h"

#ifdef _MSC_VER
// Ignore unreferenced local functions in GTK+ headers
#pragma warning(disable: 4505)
#endif

using namespace Scintilla;

namespace {

constexpr double kPi = 3.14159265358979323846;

// The Pango version guard for pango_units_from_double and pango_units_to_double
// is more complex than simply implementing these here.

constexpr int pangoUnitsFromDouble(double d) noexcept {
	return static_cast<int>(d * PANGO_SCALE + 0.5);
}

constexpr float floatFromPangoUnits(int pu) noexcept {
	return static_cast<float>(pu) / PANGO_SCALE;
}

cairo_surface_t *CreateSimilarSurface(GdkWindow *window, cairo_content_t content, int width, int height) noexcept {
	return gdk_window_create_similar_surface(window, content, width, height);
}

GdkWindow *WindowFromWidget(GtkWidget *w) noexcept {
	return gtk_widget_get_window(w);
}

GtkWidget *PWidget(WindowID wid) noexcept {
	return static_cast<GtkWidget *>(wid);
}

enum encodingType { singleByte, UTF8, dbcs };

// Holds a PangoFontDescription*.
class FontHandle {
public:
	PangoFontDescription *pfd;
	int characterSet;
	FontHandle() noexcept : pfd(nullptr), characterSet(-1) {
	}
	FontHandle(PangoFontDescription *pfd_, int characterSet_) noexcept {
		pfd = pfd_;
		characterSet = characterSet_;
	}
	// Deleted so FontHandle objects can not be copied.
	FontHandle(const FontHandle &) = delete;
	FontHandle(FontHandle &&) = delete;
	FontHandle &operator=(const FontHandle &) = delete;
	FontHandle &operator=(FontHandle &&) = delete;
	~FontHandle() {
		if (pfd)
			pango_font_description_free(pfd);
		pfd = nullptr;
	}
	static FontHandle *CreateNewFont(const FontParameters &fp);
};

FontHandle *FontHandle::CreateNewFont(const FontParameters &fp) {
	PangoFontDescription *pfd = pango_font_description_new();
	if (pfd) {
		pango_font_description_set_family(pfd,
						  (fp.faceName[0] == '!') ? fp.faceName+1 : fp.faceName);
		pango_font_description_set_size(pfd, pangoUnitsFromDouble(fp.size));
		pango_font_description_set_weight(pfd, static_cast<PangoWeight>(fp.weight));
		pango_font_description_set_style(pfd, fp.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
		return new FontHandle(pfd, fp.characterSet);
	}

	return nullptr;
}

// X has a 16 bit coordinate space, so stop drawing here to avoid wrapping
constexpr int maxCoordinate = 32000;

FontHandle *PFont(const Font &f) noexcept {
	return static_cast<FontHandle *>(f.GetID());
}

}

Font::Font() noexcept : fid(nullptr) {}

Font::~Font() {}

void Font::Create(const FontParameters &fp) {
	Release();
	fid = FontHandle::CreateNewFont(fp);
}

void Font::Release() {
	if (fid)
		delete static_cast<FontHandle *>(fid);
	fid = nullptr;
}

// Required on OS X
namespace Scintilla {

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
	SurfaceImpl() noexcept;
	// Deleted so SurfaceImpl objects can not be copied.
	SurfaceImpl(const SurfaceImpl&) = delete;
	SurfaceImpl(SurfaceImpl&&) = delete;
	SurfaceImpl&operator=(const SurfaceImpl&) = delete;
	SurfaceImpl&operator=(SurfaceImpl&&) = delete;
	~SurfaceImpl() override;

	void Init(WindowID wid) override;
	void Init(SurfaceID sid, WindowID wid) override;
	void InitPixMap(int width, int height, Surface *surface_, WindowID wid) override;

	void Clear() noexcept;
	void Release() override;
	bool Initialised() override;
	void PenColour(ColourDesired fore) override;
	int LogPixelsY() override;
	int DeviceHeightFont(int points) override;
	void MoveTo(int x_, int y_) override;
	void LineTo(int x_, int y_) override;
	void Polygon(Point *pts, size_t npts, ColourDesired fore, ColourDesired back) override;
	void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) override;
	void FillRectangle(PRectangle rc, ColourDesired back) override;
	void FillRectangle(PRectangle rc, Surface &surfacePattern) override;
	void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) override;
	void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
			    ColourDesired outline, int alphaOutline, int flags) override;
	void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) override;
	void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) override;
	void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) override;
	void Copy(PRectangle rc, Point from, Surface &surfaceSource) override;

	std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) override;

	void DrawTextBase(PRectangle rc, const Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore);
	void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back) override;
	void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back) override;
	void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore) override;
	void MeasureWidths(Font &font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthText(Font &font_, std::string_view text) override;
	XYPOSITION Ascent(Font &font_) override;
	XYPOSITION Descent(Font &font_) override;
	XYPOSITION InternalLeading(Font &font_) override;
	XYPOSITION Height(Font &font_) override;
	XYPOSITION AverageCharWidth(Font &font_) override;

	void SetClip(PRectangle rc) override;
	void FlushCachedState() override;

	void SetUnicodeMode(bool unicodeMode_) override;
	void SetDBCSMode(int codePage) override;
	void SetBidiR2L(bool bidiR2L_) override;
};
}

const char *CharacterSetID(int characterSet) noexcept {
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

SurfaceImpl::SurfaceImpl() noexcept : et(singleByte),
	context(nullptr),
	psurf(nullptr),
	x(0), y(0), inited(false), createdGC(false),
	pcontext(nullptr), layout(nullptr), characterSet(-1) {
}

SurfaceImpl::~SurfaceImpl() {
	Clear();
}

void SurfaceImpl::Clear() noexcept {
	et = singleByte;
	if (createdGC) {
		createdGC = false;
		cairo_destroy(context);
	}
	context = nullptr;
	if (psurf)
		cairo_surface_destroy(psurf);
	psurf = nullptr;
	if (layout)
		g_object_unref(layout);
	layout = nullptr;
	if (pcontext)
		g_object_unref(pcontext);
	pcontext = nullptr;
	conv.Close();
	characterSet = -1;
	x = 0;
	y = 0;
	inited = false;
	createdGC = false;
}

void SurfaceImpl::Release() {
	Clear();
}

bool SurfaceImpl::Initialised() {
	if (inited && context) {
		if (cairo_status(context) == CAIRO_STATUS_SUCCESS) {
			// Even when status is success, the target surface may have been
			// finished which may cause an assertion to fail crashing the application.
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
	return inited;
}

void SurfaceImpl::Init(WindowID wid) {
	Release();
	PLATFORM_ASSERT(wid);
	// if we are only created from a window ID, we can't perform drawing
	psurf = nullptr;
	context = nullptr;
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
	SurfaceImpl *surfImpl = dynamic_cast<SurfaceImpl *>(surface_);
	PLATFORM_ASSERT(surfImpl);
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
		const ColourDesired cdFore(fore.AsInteger());
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
	const int logPix = LogPixelsY();
	return (points * logPix + logPix / 2) / 72;
}

void SurfaceImpl::MoveTo(int x_, int y_) {
	x = x_;
	y = y_;
}

static int Delta(int difference) noexcept {
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
		const int xDiff = x_ - x;
		const int xDelta = Delta(xDiff);
		const int yDiff = y_ - y;
		const int yDelta = Delta(yDiff);
		if ((xDiff == 0) || (yDiff == 0)) {
			// Horizontal or vertical lines can be more precisely drawn as a filled rectangle
			const int xEnd = x_ - xDelta;
			const int left = std::min(x, xEnd);
			const int width = std::abs(x - xEnd) + 1;
			const int yEnd = y_ - yDelta;
			const int top = std::min(y, yEnd);
			const int height = std::abs(y - yEnd) + 1;
			cairo_rectangle(context, left, top, width, height);
			cairo_fill(context);
		} else if ((std::abs(xDiff) == std::abs(yDiff))) {
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

void SurfaceImpl::Polygon(Point *pts, size_t npts, ColourDesired fore,
			  ColourDesired back) {
	PLATFORM_ASSERT(context);
	PenColour(back);
	cairo_move_to(context, pts[0].x + 0.5, pts[0].y + 0.5);
	for (size_t i = 1; i < npts; i++) {
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
				rc.Width() - 1, rc.Height() - 1);
		PenColour(back);
		cairo_fill_preserve(context);
		PenColour(fore);
		cairo_stroke(context);
	}
}

void SurfaceImpl::FillRectangle(PRectangle rc, ColourDesired back) {
	PenColour(back);
	if (context && (rc.left < maxCoordinate)) {	// Protect against out of range
		rc.left = std::round(rc.left);
		rc.right = std::round(rc.right);
		cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
		cairo_fill(context);
	}
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceImpl &surfi = dynamic_cast<SurfaceImpl &>(surfacePattern);
	if (context && surfi.psurf) {
		// Tile pattern over rectangle
		cairo_set_source_surface(context, surfi.psurf, rc.left, rc.top);
		cairo_pattern_set_extend(cairo_get_source(context), CAIRO_EXTEND_REPEAT);
		cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
		cairo_fill(context);
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
		Polygon(pts, std::size(pts), fore, back);
	} else {
		RectangleDraw(rc, fore, back);
	}
}

static void PathRoundRectangle(cairo_t *context, double left, double top, double width, double height, double radius) noexcept {
	constexpr double degrees = kPi / 180.0;

	cairo_new_sub_path(context);
	cairo_arc(context, left + width - radius, top + radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(context, left + width - radius, top + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(context, left + radius, top + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(context, left + radius, top + radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(context);
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
				 ColourDesired outline, int alphaOutline, int /*flags*/) {
	if (context && rc.Width() > 0) {
		const ColourDesired cdFill(fill.AsInteger());
		cairo_set_source_rgba(context,
				      cdFill.GetRed() / 255.0,
				      cdFill.GetGreen() / 255.0,
				      cdFill.GetBlue() / 255.0,
				      alphaFill / 255.0);
		if (cornerSize > 0)
			PathRoundRectangle(context, rc.left + 1.0, rc.top + 1.0, rc.Width() - 2.0, rc.Height() - 2.0, cornerSize);
		else
			cairo_rectangle(context, rc.left + 1.0, rc.top + 1.0, rc.Width() - 2.0, rc.Height() - 2.0);
		cairo_fill(context);

		const ColourDesired cdOutline(outline.AsInteger());
		cairo_set_source_rgba(context,
				      cdOutline.GetRed() / 255.0,
				      cdOutline.GetGreen() / 255.0,
				      cdOutline.GetBlue() / 255.0,
				      alphaOutline / 255.0);
		if (cornerSize > 0)
			PathRoundRectangle(context, rc.left + 0.5, rc.top + 0.5, rc.Width() - 1, rc.Height() - 1, cornerSize);
		else
			cairo_rectangle(context, rc.left + 0.5, rc.top + 0.5, rc.Width() - 1, rc.Height() - 1);
		cairo_stroke(context);
	}
}

void SurfaceImpl::GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) {
	if (context) {
		cairo_pattern_t *pattern;
		switch (options) {
		case GradientOptions::leftToRight:
			pattern = cairo_pattern_create_linear(rc.left, rc.top, rc.right, rc.top);
			break;
		case GradientOptions::topToBottom:
		default:
			pattern = cairo_pattern_create_linear(rc.left, rc.top, rc.left, rc.bottom);
			break;
		}
		for (const ColourStop &stop : stops) {
			cairo_pattern_add_color_stop_rgba(pattern, stop.position,
							  stop.colour.GetRedComponent(),
							  stop.colour.GetGreenComponent(),
							  stop.colour.GetBlueComponent(),
							  stop.colour.GetAlphaComponent());
		}
		cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
		cairo_set_source(context, pattern);
		cairo_fill(context);
		cairo_pattern_destroy(pattern);
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

	const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	const int ucs = stride * height;
	std::vector<unsigned char> image(ucs);
	for (ptrdiff_t iy=0; iy<height; iy++) {
		unsigned char *pixel = &image[0] + iy*stride;
		RGBAImage::BGRAFromRGBA(pixel, pixelsImage, width);
		pixelsImage += RGBAImage::bytesPerPixel * width;
	}

	cairo_surface_t *psurfImage = cairo_image_surface_create_for_data(&image[0], CAIRO_FORMAT_ARGB32, width, height, stride);
	cairo_set_source_surface(context, psurfImage, rc.left, rc.top);
	cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
	cairo_fill(context);

	cairo_surface_destroy(psurfImage);
}

void SurfaceImpl::Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) {
	PLATFORM_ASSERT(context);
	PenColour(back);
	cairo_arc(context, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2,
		  std::min(rc.Width(), rc.Height()) / 2, 0, 2*kPi);
	cairo_fill_preserve(context);
	PenColour(fore);
	cairo_stroke(context);
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceImpl &surfi = static_cast<SurfaceImpl &>(surfaceSource);
	const bool canDraw = surfi.psurf != nullptr;
	if (canDraw) {
		PLATFORM_ASSERT(context);
		cairo_set_source_surface(context, surfi.psurf,
					 rc.left - from.x, rc.top - from.y);
		cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
		cairo_fill(context);
	}
}

std::unique_ptr<IScreenLineLayout> SurfaceImpl::Layout(const IScreenLine *) {
	return {};
}

std::string UTF8FromLatin1(std::string_view text) {
	std::string utfForm(text.length()*2 + 1, '\0');
	size_t lenU = 0;
	for (const char ch : text) {
		const unsigned char uch = ch;
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

namespace {

std::string UTF8FromIconv(const Converter &conv, std::string_view text) {
	if (conv) {
		std::string utfForm(text.length()*3+1, '\0');
		char *pin = const_cast<char *>(text.data());
		gsize inLeft = text.length();
		char *putf = &utfForm[0];
		char *pout = putf;
		gsize outLeft = text.length()*3+1;
		const gsize conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions != sizeFailure) {
			*pout = '\0';
			utfForm.resize(pout - putf);
			return utfForm;
		}
	}
	return std::string();
}

// Work out how many bytes are in a character by trying to convert using iconv,
// returning the first length that succeeds.
size_t MultiByteLenFromIconv(const Converter &conv, const char *s, size_t len) noexcept {
	for (size_t lenMB=1; (lenMB<4) && (lenMB <= len); lenMB++) {
		char wcForm[2] {};
		char *pin = const_cast<char *>(s);
		gsize inLeft = lenMB;
		char *pout = wcForm;
		gsize outLeft = 2;
		const gsize conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions != sizeFailure) {
			return lenMB;
		}
	}
	return 1;
}

}

void SurfaceImpl::DrawTextBase(PRectangle rc, const Font &font_, XYPOSITION ybase, std::string_view text,
			       ColourDesired fore) {
	PenColour(fore);
	if (context) {
		const XYPOSITION xText = rc.left;
		if (PFont(font_)->pfd) {
			std::string utfForm;
			if (et == UTF8) {
				pango_layout_set_text(layout, text.data(), text.length());
			} else {
				SetConverter(PFont(font_)->characterSet);
				utfForm = UTF8FromIconv(conv, text);
				if (utfForm.empty()) {	// iconv failed so treat as Latin1
					utfForm = UTF8FromLatin1(text);
				}
				pango_layout_set_text(layout, utfForm.c_str(), utfForm.length());
			}
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
			pango_cairo_update_layout(context, layout);
			PangoLayoutLine *pll = pango_layout_get_line_readonly(layout, 0);
			cairo_move_to(context, xText, ybase);
			pango_cairo_show_layout_line(context, pll);
		}
	}
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text,
				 ColourDesired fore, ColourDesired back) {
	FillRectangle(rc, back);
	DrawTextBase(rc, font_, ybase, text, fore);
}

// On GTK+, exactly same as DrawTextNoClip
void SurfaceImpl::DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text,
				  ColourDesired fore, ColourDesired back) {
	FillRectangle(rc, back);
	DrawTextBase(rc, font_, ybase, text, fore);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text,
				      ColourDesired fore) {
	// Avoid drawing spaces in transparent mode
	for (size_t i=0; i<text.length(); i++) {
		if (text[i] != ' ') {
			DrawTextBase(rc, font_, ybase, text, fore);
			return;
		}
	}
}

class ClusterIterator {
	PangoLayoutIter *iter;
	PangoRectangle pos;
	size_t lenPositions;
public:
	bool finished;
	XYPOSITION positionStart;
	XYPOSITION position;
	XYPOSITION distance;
	int curIndex;
	ClusterIterator(PangoLayout *layout, size_t len) noexcept : lenPositions(len), finished(false),
		positionStart(0), position(0), distance(0), curIndex(0) {
		iter = pango_layout_get_iter(layout);
		pango_layout_iter_get_cluster_extents(iter, nullptr, &pos);
	}
	// Deleted so ClusterIterator objects can not be copied.
	ClusterIterator(const ClusterIterator&) = delete;
	ClusterIterator(ClusterIterator&&) = delete;
	ClusterIterator&operator=(const ClusterIterator&) = delete;
	ClusterIterator&operator=(ClusterIterator&&) = delete;

	~ClusterIterator() {
		pango_layout_iter_free(iter);
	}

	void Next() noexcept {
		positionStart = position;
		if (pango_layout_iter_next_cluster(iter)) {
			pango_layout_iter_get_cluster_extents(iter, nullptr, &pos);
			position = floatFromPangoUnits(pos.x);
			curIndex = pango_layout_iter_get_index(iter);
		} else {
			finished = true;
			position = floatFromPangoUnits(pos.x + pos.width);
			curIndex = lenPositions;
		}
		distance = position - positionStart;
	}
};

void SurfaceImpl::MeasureWidths(Font &font_, std::string_view text, XYPOSITION *positions) {
	if (font_.GetID()) {
		if (PFont(font_)->pfd) {
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
			if (et == UTF8) {
				// Simple and direct as UTF-8 is native Pango encoding
				int i = 0;
				pango_layout_set_text(layout, text.data(), text.length());
				ClusterIterator iti(layout, text.length());
				while (!iti.finished) {
					iti.Next();
					const int places = iti.curIndex - i;
					while (i < iti.curIndex) {
						// Evenly distribute space among bytes of this cluster.
						// Would be better to find number of characters and then
						// divide evenly between characters with each byte of a character
						// being at the same position.
						positions[i] = iti.position - (iti.curIndex - 1 - i) * iti.distance / places;
						i++;
					}
				}
				PLATFORM_ASSERT(static_cast<size_t>(i) == text.length());
			} else {
				int positionsCalculated = 0;
				if (et == dbcs) {
					SetConverter(PFont(font_)->characterSet);
					std::string utfForm = UTF8FromIconv(conv, text);
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
							const int clusterEnd = iti.curIndex;
							const int places = g_utf8_strlen(utfForm.c_str() + clusterStart, clusterEnd - clusterStart);
							int place = 1;
							while (clusterStart < clusterEnd) {
								size_t lenChar = MultiByteLenFromIconv(convMeasure, text.data()+i, text.length()-i);
								while (lenChar--) {
									positions[i++] = iti.position - (places - place) * iti.distance / places;
									positionsCalculated++;
								}
								clusterStart += UTF8BytesOfLead[static_cast<unsigned char>(utfForm[clusterStart])];
								place++;
							}
						}
						PLATFORM_ASSERT(static_cast<size_t>(i) == text.length());
					}
				}
				if (positionsCalculated < 1) {
					const size_t lenPositions = text.length();
					// Either 8-bit or DBCS conversion failed so treat as 8-bit.
					SetConverter(PFont(font_)->characterSet);
					const bool rtlCheck = PFont(font_)->characterSet == SC_CHARSET_HEBREW ||
							      PFont(font_)->characterSet == SC_CHARSET_ARABIC;
					std::string utfForm = UTF8FromIconv(conv, text);
					if (utfForm.empty()) {
						utfForm = UTF8FromLatin1(text);
					}
					pango_layout_set_text(layout, utfForm.c_str(), utfForm.length());
					size_t i = 0;
					int clusterStart = 0;
					// Each 8-bit input character may take 1 or 2 bytes in UTF-8
					// and groups of up to 3 may be represented as ligatures.
					ClusterIterator iti(layout, utfForm.length());
					while (!iti.finished) {
						iti.Next();
						const int clusterEnd = iti.curIndex;
						const int ligatureLength = g_utf8_strlen(utfForm.c_str() + clusterStart, clusterEnd - clusterStart);
						if (rtlCheck && ((clusterEnd <= clusterStart) || (ligatureLength == 0) || (ligatureLength > 3))) {
							// Something has gone wrong: exit quickly but pretend all the characters are equally spaced:
							int widthLayout = 0;
							pango_layout_get_size(layout, &widthLayout, nullptr);
							const XYPOSITION widthTotal = floatFromPangoUnits(widthLayout);
							for (size_t bytePos=0; bytePos<lenPositions; bytePos++) {
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
					PLATFORM_ASSERT(i == text.length());
				}
			}
		}
	} else {
		// No font so return an ascending range of values
		for (size_t i = 0; i < text.length(); i++) {
			positions[i] = i + 1;
		}
	}
}

XYPOSITION SurfaceImpl::WidthText(Font &font_, std::string_view text) {
	if (font_.GetID()) {
		if (PFont(font_)->pfd) {
			std::string utfForm;
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
			if (et == UTF8) {
				pango_layout_set_text(layout, text.data(), text.length());
			} else {
				SetConverter(PFont(font_)->characterSet);
				utfForm = UTF8FromIconv(conv, text);
				if (utfForm.empty()) {	// iconv failed so treat as Latin1
					utfForm = UTF8FromLatin1(text);
				}
				pango_layout_set_text(layout, utfForm.c_str(), utfForm.length());
			}
			PangoLayoutLine *pangoLine = pango_layout_get_line_readonly(layout, 0);
			PangoRectangle pos {};
			pango_layout_line_get_extents(pangoLine, nullptr, &pos);
			return floatFromPangoUnits(pos.width);
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
	XYPOSITION ascent = 0;
	if (PFont(font_)->pfd) {
		PangoFontMetrics *metrics = pango_context_get_metrics(pcontext,
					    PFont(font_)->pfd, pango_context_get_language(pcontext));
		ascent = std::round(floatFromPangoUnits(
					    pango_font_metrics_get_ascent(metrics)));
		pango_font_metrics_unref(metrics);
	}
	if (ascent == 0) {
		ascent = 1;
	}
	return ascent;
}

XYPOSITION SurfaceImpl::Descent(Font &font_) {
	if (!(font_.GetID()))
		return 1;
	if (PFont(font_)->pfd) {
		PangoFontMetrics *metrics = pango_context_get_metrics(pcontext,
					    PFont(font_)->pfd, pango_context_get_language(pcontext));
		const XYPOSITION descent = std::round(floatFromPangoUnits(
				pango_font_metrics_get_descent(metrics)));
		pango_font_metrics_unref(metrics);
		return descent;
	}
	return 0;
}

XYPOSITION SurfaceImpl::InternalLeading(Font &) {
	return 0;
}

XYPOSITION SurfaceImpl::Height(Font &font_) {
	return Ascent(font_) + Descent(font_);
}

XYPOSITION SurfaceImpl::AverageCharWidth(Font &font_) {
	return WidthText(font_, "n");
}

void SurfaceImpl::SetClip(PRectangle rc) {
	PLATFORM_ASSERT(context);
	cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
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

void SurfaceImpl::SetBidiR2L(bool) {
}

Surface *Surface::Allocate(int) {
	return new SurfaceImpl();
}

Window::~Window() {}

void Window::Destroy() {
	if (wid) {
		ListBox *listbox = dynamic_cast<ListBox *>(this);
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
		wid = nullptr;
	}
}

PRectangle Window::GetPosition() const {
	// Before any size allocated pretend its 1000 wide so not scrolled
	PRectangle rc(0, 0, 1000, 1000);
	if (wid) {
		GtkAllocation allocation;
		gtk_widget_get_allocation(PWidget(wid), &allocation);
		rc.left = static_cast<XYPOSITION>(allocation.x);
		rc.top = static_cast<XYPOSITION>(allocation.y);
		if (allocation.width > 20) {
			rc.right = rc.left + allocation.width;
			rc.bottom = rc.top + allocation.height;
		}
	}
	return rc;
}

void Window::SetPosition(PRectangle rc) {
	GtkAllocation alloc;
	alloc.x = static_cast<int>(rc.left);
	alloc.y = static_cast<int>(rc.top);
	alloc.width = static_cast<int>(rc.Width());
	alloc.height = static_cast<int>(rc.Height());
	gtk_widget_size_allocate(PWidget(wid), &alloc);
}

namespace {

GdkRectangle MonitorRectangleForWidget(GtkWidget *wid) noexcept {
	GdkWindow *wnd = WindowFromWidget(wid);
	GdkRectangle rcScreen = GdkRectangle();
#if GTK_CHECK_VERSION(3,22,0)
	GdkDisplay *pdisplay = gtk_widget_get_display(wid);
	GdkMonitor *monitor = gdk_display_get_monitor_at_window(pdisplay, wnd);
	gdk_monitor_get_geometry(monitor, &rcScreen);
#else
	GdkScreen *screen = gtk_widget_get_screen(wid);
	const gint monitor_num = gdk_screen_get_monitor_at_window(screen, wnd);
	gdk_screen_get_monitor_geometry(screen, monitor_num, &rcScreen);
#endif
	return rcScreen;
}

}

void Window::SetPositionRelative(PRectangle rc, const Window *relativeTo) {
	const IntegerRectangle irc(rc);
	int ox = 0;
	int oy = 0;
	GdkWindow *wndRelativeTo = WindowFromWidget(PWidget(relativeTo->wid));
	gdk_window_get_origin(wndRelativeTo, &ox, &oy);
	ox += irc.left;
	oy += irc.top;

	const GdkRectangle rcMonitor = MonitorRectangleForWidget(PWidget(relativeTo->wid));

	/* do some corrections to fit into screen */
	const int sizex = irc.Width();
	const int sizey = irc.Height();
	if (sizex > rcMonitor.width || ox < rcMonitor.x)
		ox = rcMonitor.x; /* the best we can do */
	else if (ox + sizex > rcMonitor.x + rcMonitor.width)
		ox = rcMonitor.x + rcMonitor.width - sizex;
	if (sizey > rcMonitor.height || oy < rcMonitor.y)
		oy = rcMonitor.y;
	else if (oy + sizey > rcMonitor.y + rcMonitor.height)
		oy = rcMonitor.y + rcMonitor.height - sizey;

	gtk_window_move(GTK_WINDOW(PWidget(wid)), ox, oy);

	gtk_window_resize(GTK_WINDOW(wid), sizex, sizey);
}

PRectangle Window::GetClientPosition() const {
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
		const IntegerRectangle irc(rc);
		gtk_widget_queue_draw_area(PWidget(wid),
					   irc.left, irc.top,
					   irc.Width(), irc.Height());
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

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   gdk window coordinates */
PRectangle Window::GetMonitorRect(Point pt) {
	gint x_offset, y_offset;

	gdk_window_get_origin(WindowFromWidget(PWidget(wid)), &x_offset, &y_offset);

	GdkRectangle rect {};

#if GTK_CHECK_VERSION(3,22,0)
	GdkDisplay *pdisplay = gtk_widget_get_display(PWidget(wid));
	GdkMonitor *monitor = gdk_display_get_monitor_at_point(pdisplay,
			      pt.x + x_offset, pt.y + y_offset);
	gdk_monitor_get_geometry(monitor, &rect);
#else
	GdkScreen *screen = gtk_widget_get_screen(PWidget(wid));
	const gint monitor_num = gdk_screen_get_monitor_at_point(screen,
				 pt.x + x_offset, pt.y + y_offset);
	gdk_screen_get_monitor_geometry(screen, monitor_num, &rect);
#endif
	rect.x -= x_offset;
	rect.y -= y_offset;
	return PRectangle::FromInts(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
}

typedef std::map<int, RGBAImage *> ImageMap;

struct ListImage {
	const RGBAImage *rgba_data;
	GdkPixbuf *pixbuf;
};

static void list_image_free(gpointer, gpointer value, gpointer) noexcept {
	ListImage *list_image = static_cast<ListImage *>(value);
	if (list_image->pixbuf)
		g_object_unref(list_image->pixbuf);
	g_free(list_image);
}

ListBox::ListBox() noexcept {
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
	GtkCellRenderer *pixbuf_renderer;
	GtkCellRenderer *renderer;
	RGBAImageSet images;
	int desiredVisibleRows;
	unsigned int maxItemCharacters;
	unsigned int aveCharWidth;
#if GTK_CHECK_VERSION(3,0,0)
	GtkCssProvider *cssProvider;
#endif
public:
	IListBoxDelegate *delegate;

	ListBoxX() noexcept : widCached(nullptr), frame(nullptr), list(nullptr), scroller(nullptr),
		pixhash(nullptr), pixbuf_renderer(nullptr),
		renderer(nullptr),
		desiredVisibleRows(5), maxItemCharacters(0),
		aveCharWidth(1),
#if GTK_CHECK_VERSION(3,0,0)
		cssProvider(nullptr),
#endif
		delegate(nullptr) {
	}
	// Deleted so ListBoxX objects can not be copied.
	ListBoxX(const ListBoxX&) = delete;
	ListBoxX(ListBoxX&&) = delete;
	ListBoxX&operator=(const ListBoxX&) = delete;
	ListBoxX&operator=(ListBoxX&&) = delete;
	~ListBoxX() override {
		if (pixhash) {
			g_hash_table_foreach((GHashTable *) pixhash, list_image_free, nullptr);
			g_hash_table_destroy((GHashTable *) pixhash);
		}
		if (widCached) {
			gtk_widget_destroy(GTK_WIDGET(widCached));
			wid = widCached = nullptr;
		}
#if GTK_CHECK_VERSION(3,0,0)
		if (cssProvider) {
			g_object_unref(cssProvider);
			cssProvider = nullptr;
		}
#endif
	}
	void SetFont(Font &font) override;
	void Create(Window &parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_, int technology_) override;
	void SetAverageCharWidth(int width) override;
	void SetVisibleRows(int rows) override;
	int GetVisibleRows() const override;
	int GetRowHeight();
	PRectangle GetDesiredRect() override;
	int CaretFromEdge() override;
	void Clear() override;
	void Append(char *s, int type = -1) override;
	int Length() override;
	void Select(int n) override;
	int GetSelection() override;
	int Find(const char *prefix) override;
	void GetValue(int n, char *value, int len) override;
	void RegisterRGBA(int type, RGBAImage *image);
	void RegisterImage(int type, const char *xpm_data) override;
	void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) override;
	void ClearRegisteredImages() override;
	void SetDelegate(IListBoxDelegate *lbDelegate) override;
	void SetList(const char *listText, char separator, char typesep) override;
};

ListBox *ListBox::Allocate() {
	ListBoxX *lb = new ListBoxX();
	return lb;
}

static int treeViewGetRowHeight(GtkTreeView *view) {
#if GTK_CHECK_VERSION(3,0,0)
	// This version sometimes reports erroneous results on GTK2, but the GTK2
	// version is inaccurate for GTK 3.14.
	GdkRectangle rect;
	GtkTreePath *path = gtk_tree_path_new_first();
	gtk_tree_view_get_background_area(view, path, nullptr, &rect);
	gtk_tree_path_free(path);
	return rect.height;
#else
	int row_height=0;
	int vertical_separator=0;
	int expander_size=0;
	GtkTreeViewColumn *column = gtk_tree_view_get_column(view, 0);
	gtk_tree_view_column_cell_get_size(column, nullptr, nullptr, nullptr, nullptr, &row_height);
	gtk_widget_style_get(GTK_WIDGET(view),
			     "vertical-separator", &vertical_separator,
			     "expander-size", &expander_size, nullptr);
	row_height += vertical_separator;
	row_height = std::max(row_height, expander_size);
	return row_height;
#endif
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
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(widget));
	if (GTK_IS_TREE_VIEW(child)) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(child));
		int n_rows = gtk_tree_model_iter_n_children(model, nullptr);
		int row_height = treeViewGetRowHeight(GTK_TREE_VIEW(child));

		*min = MAX(1, row_height);
		*nat = MAX(*min, n_rows * row_height);
	} else {
		GTK_WIDGET_CLASS(small_scroller_parent_class)->get_preferred_height(widget, min, nat);
		if (*min > 1)
			*min = 1;
	}
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

static void small_scroller_init(SmallScroller *) {}

static gboolean ButtonPress(GtkWidget *, GdkEventButton *ev, gpointer p) {
	try {
		ListBoxX *lb = static_cast<ListBoxX *>(p);
		if (ev->type == GDK_2BUTTON_PRESS && lb->delegate) {
			ListBoxEvent event(ListBoxEvent::EventType::doubleClick);
			lb->delegate->ListNotify(&event);
			return TRUE;
		}

	} catch (...) {
		// No pointer back to Scintilla to save status
	}
	return FALSE;
}

static gboolean ButtonRelease(GtkWidget *, GdkEventButton *ev, gpointer p) {
	try {
		ListBoxX *lb = static_cast<ListBoxX *>(p);
		if (ev->type != GDK_2BUTTON_PRESS && lb->delegate) {
			ListBoxEvent event(ListBoxEvent::EventType::selectionChange);
			lb->delegate->ListNotify(&event);
			return TRUE;
		}
	} catch (...) {
		// No pointer back to Scintilla to save status
	}
	return FALSE;
}

/* Change the active colour to the selected colour so the listbox uses the colour
scheme that it would use if it had the focus. */
static void StyleSet(GtkWidget *w, GtkStyle *, void *) {

	g_return_if_fail(w != nullptr);

	/* Copy the selected colour to active.  Note that the modify calls will cause
	recursive calls to this function after the value is updated and w->style to
	be set to a new object */

#if GTK_CHECK_VERSION(3,16,0)
	// On recent releases of GTK+, it does not appear necessary to set the list box colours.
	// This may be because of common themes and may be needed with other themes.
	// The *override* calls are deprecated now, so only call them for older versions of GTK+.
#elif GTK_CHECK_VERSION(3,0,0)
	GtkStyleContext *styleContext = gtk_widget_get_style_context(w);
	if (styleContext == nullptr)
		return;

	GdkRGBA colourForeSelected;
	gtk_style_context_get_color(styleContext, GTK_STATE_FLAG_SELECTED, &colourForeSelected);
	GdkRGBA colourForeActive;
	gtk_style_context_get_color(styleContext, GTK_STATE_FLAG_ACTIVE, &colourForeActive);
	if (!gdk_rgba_equal(&colourForeSelected, &colourForeActive))
		gtk_widget_override_color(w, GTK_STATE_FLAG_ACTIVE, &colourForeSelected);

	styleContext = gtk_widget_get_style_context(w);
	if (styleContext == nullptr)
		return;

	GdkRGBA colourBaseSelected;
	gtk_style_context_get_background_color(styleContext, GTK_STATE_FLAG_SELECTED, &colourBaseSelected);
	GdkRGBA colourBaseActive;
	gtk_style_context_get_background_color(styleContext, GTK_STATE_FLAG_ACTIVE, &colourBaseActive);
	if (!gdk_rgba_equal(&colourBaseSelected, &colourBaseActive))
		gtk_widget_override_background_color(w, GTK_STATE_FLAG_ACTIVE, &colourBaseSelected);
#else
	GtkStyle *style = gtk_widget_get_style(w);
	if (style == nullptr)
		return;
	if (!gdk_color_equal(&style->base[GTK_STATE_SELECTED], &style->base[GTK_STATE_ACTIVE]))
		gtk_widget_modify_base(w, GTK_STATE_ACTIVE, &style->base[GTK_STATE_SELECTED]);
	style = gtk_widget_get_style(w);
	if (style == nullptr)
		return;
	if (!gdk_color_equal(&style->text[GTK_STATE_SELECTED], &style->text[GTK_STATE_ACTIVE]))
		gtk_widget_modify_text(w, GTK_STATE_ACTIVE, &style->text[GTK_STATE_SELECTED]);
#endif
}

void ListBoxX::Create(Window &parent, int, Point, int, bool, int) {
	if (widCached != nullptr) {
		wid = widCached;
		return;
	}

#if GTK_CHECK_VERSION(3,0,0)
	if (!cssProvider) {
		cssProvider = gtk_css_provider_new();
	}
#endif

	wid = widCached = gtk_window_new(GTK_WINDOW_POPUP);

	frame = gtk_frame_new(nullptr);
	gtk_widget_show(PWidget(frame));
	gtk_container_add(GTK_CONTAINER(GetID()), PWidget(frame));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);

	scroller = g_object_new(small_scroller_get_type(), nullptr);
	gtk_container_set_border_width(GTK_CONTAINER(scroller), 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), PWidget(scroller));
	gtk_widget_show(PWidget(scroller));

	/* Tree and its model */
	GtkListStore *store =
		gtk_list_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_signal_connect(G_OBJECT(list), "style-set", G_CALLBACK(StyleSet), nullptr);

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
		g_object_set(G_OBJECT(list), "fixed-height-mode", TRUE, nullptr);

	GtkWidget *widget = PWidget(list);	// No code inside the G_OBJECT macro
	gtk_container_add(GTK_CONTAINER(PWidget(scroller)), widget);
	gtk_widget_show(widget);
	g_signal_connect(G_OBJECT(widget), "button_press_event",
			 G_CALLBACK(ButtonPress), this);
	g_signal_connect(G_OBJECT(widget), "button_release_event",
			 G_CALLBACK(ButtonRelease), this);

	GtkWidget *top = gtk_widget_get_toplevel(static_cast<GtkWidget *>(parent.GetID()));
	gtk_window_set_transient_for(GTK_WINDOW(static_cast<GtkWidget *>(wid)),
				     GTK_WINDOW(top));
}

void ListBoxX::SetFont(Font &font) {
	// Only do for Pango font as there have been crashes for GDK fonts
	if (Created() && PFont(font)->pfd) {
		// Current font is Pango font
#if GTK_CHECK_VERSION(3,0,0)
		if (cssProvider) {
			PangoFontDescription *pfd = PFont(font)->pfd;
			std::ostringstream ssFontSetting;
			ssFontSetting << "GtkTreeView, treeview { ";
			ssFontSetting << "font-family: " << pango_font_description_get_family(pfd) <<  "; ";
			ssFontSetting << "font-size:";
			ssFontSetting << static_cast<double>(pango_font_description_get_size(pfd)) / PANGO_SCALE;
			// On GTK < 3.21.0 the units are incorrectly parsed, so a font size in points
			// need to use the "px" unit.  Normally we only get fonts in points here, so
			// don't bother to handle the case the font is actually in pixels on < 3.21.0.
			if (gtk_check_version(3, 21, 0) != nullptr || // on < 3.21.0
					pango_font_description_get_size_is_absolute(pfd)) {
				ssFontSetting << "px; ";
			} else {
				ssFontSetting << "pt; ";
			}
			ssFontSetting << "font-weight:"<< pango_font_description_get_weight(pfd) << "; ";
			ssFontSetting << "}";
			gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(cssProvider),
							ssFontSetting.str().c_str(), -1, nullptr);
		}
#else
		gtk_widget_modify_font(PWidget(list), PFont(font)->pfd);
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

int ListBoxX::GetRowHeight() {
	return treeViewGetRowHeight(GTK_TREE_VIEW(list));
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
		gtk_widget_get_preferred_size(GTK_WIDGET(frame), nullptr, &req);
#else
		gtk_widget_size_request(GTK_WIDGET(frame), &req);
#endif
		int height;

		// First calculate height of the clist for our desired visible
		// row count otherwise it tries to expand to the total # of rows
		// Get cell height
		const int row_height = GetRowHeight();
#if GTK_CHECK_VERSION(3,0,0)
		GtkStyleContext *styleContextFrame = gtk_widget_get_style_context(PWidget(frame));
		GtkStateFlags stateFlagsFrame = gtk_style_context_get_state(styleContextFrame);
		GtkBorder padding, border, border_border = { 0, 0, 0, 0 };
		gtk_style_context_get_padding(styleContextFrame, stateFlagsFrame, &padding);
		gtk_style_context_get_border(styleContextFrame, stateFlagsFrame, &border);

#	if GTK_CHECK_VERSION(3,20,0)
		// on GTK 3.20 the frame border is in a sub-node "border".
		// Unfortunately we need to be built against 3.20 to be able to support this, as it requires
		// new API.
		GtkStyleContext *styleContextFrameBorder = gtk_style_context_new();
		GtkWidgetPath *widget_path = gtk_widget_path_copy(gtk_style_context_get_path(styleContextFrame));
		gtk_widget_path_append_type(widget_path, GTK_TYPE_BORDER); // dummy type
		gtk_widget_path_iter_set_object_name(widget_path, -1, "border");
		gtk_style_context_set_path(styleContextFrameBorder, widget_path);
		gtk_widget_path_free(widget_path);
		gtk_style_context_get_border(styleContextFrameBorder, stateFlagsFrame, &border_border);
		g_object_unref(styleContextFrameBorder);
#	else // < 3.20
		if (gtk_check_version(3, 20, 0) == nullptr) {
			// default to 1px all around as it's likely what it is, and so we don't miss 2px height
			// on GTK 3.20 when built against an earlier version.
			border_border.top = border_border.bottom = border_border.left = border_border.right = 1;
		}
#	endif

		height = (rows * row_height
			  + padding.top + padding.bottom
			  + border.top + border.bottom
			  + border_border.top + border_border.bottom
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
				     "horizontal-separator", &horizontal_separator, nullptr);
		rc.right += horizontal_separator;
#if GTK_CHECK_VERSION(3,0,0)
		rc.right += (padding.left + padding.right
			     + border.left + border.right
			     + border_border.left + border_border.right
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
			gtk_widget_get_preferred_size(vscrollbar, nullptr, &req);
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
						 nullptr,
						 nullptr);
	}
}

#define SPACING 5

void ListBoxX::Append(char *s, int type) {
	ListImage *list_image = nullptr;
	if ((type >= 0) && pixhash) {
		list_image = static_cast<ListImage *>(g_hash_table_lookup((GHashTable *) pixhash,
						      GINT_TO_POINTER(type)));
	}
	GtkTreeIter iter {};
	GtkListStore *store =
		GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	if (list_image) {
		if (nullptr == list_image->pixbuf)
			init_pixmap(list_image);
		if (list_image->pixbuf) {
			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
					   PIXBUF_COLUMN, list_image->pixbuf,
					   TEXT_COLUMN, s, -1);

			const gint pixbuf_width = gdk_pixbuf_get_width(list_image->pixbuf);
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
	const size_t len = strlen(s);
	if (maxItemCharacters < len)
		maxItemCharacters = len;
}

int ListBoxX::Length() {
	if (wid)
		return gtk_tree_model_iter_n_children(gtk_tree_view_get_model
						      (GTK_TREE_VIEW(list)), nullptr);
	return 0;
}

void ListBoxX::Select(int n) {
	GtkTreeIter iter {};
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	GtkTreeSelection *selection =
		gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

	if (n < 0) {
		gtk_tree_selection_unselect_all(selection);
		return;
	}

	const bool valid = gtk_tree_model_iter_nth_child(model, &iter, nullptr, n) != FALSE;
	if (valid) {
		gtk_tree_selection_select_iter(selection, &iter);

		// Move the scrollbar to show the selection.
		const int total = Length();
#if GTK_CHECK_VERSION(3,0,0)
		GtkAdjustment *adj =
			gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(list));
#else
		GtkAdjustment *adj =
			gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(list));
#endif
		gfloat value = (static_cast<gfloat>(n) / total) * (gtk_adjustment_get_upper(adj) - gtk_adjustment_get_lower(adj))
			       + gtk_adjustment_get_lower(adj) - gtk_adjustment_get_page_size(adj) / 2;
		// Get cell height
		const int row_height = GetRowHeight();

		int rows = Length();
		if ((rows == 0) || (rows > desiredVisibleRows))
			rows = desiredVisibleRows;
		if (rows & 0x1) {
			// Odd rows to display -- We are now in the middle.
			// Align it so that we don't chop off rows.
			value += static_cast<gfloat>(row_height) / 2.0f;
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

	if (delegate) {
		ListBoxEvent event(ListBoxEvent::EventType::selectionChange);
		delegate->ListNotify(&event);
	}
}

int ListBoxX::GetSelection() {
	int index = -1;
	GtkTreeIter iter {};
	GtkTreeModel *model {};
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		const int *indices = gtk_tree_path_get_indices(path);
		// Don't free indices.
		if (indices)
			index = indices[0];
		gtk_tree_path_free(path);
	}
	return index;
}

int ListBoxX::Find(const char *prefix) {
	GtkTreeIter iter {};
	GtkTreeModel *model =
		gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	bool valid = gtk_tree_model_get_iter_first(model, &iter) != FALSE;
	int i = 0;
	while (valid) {
		gchar *s = nullptr;
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
	char *text = nullptr;
	GtkTreeIter iter {};
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	const bool valid = gtk_tree_model_iter_nth_child(model, &iter, nullptr, n) != FALSE;
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
				GINT_TO_POINTER(type)));
	if (list_image) {
		// Drop icon already registered
		if (list_image->pixbuf)
			g_object_unref(list_image->pixbuf);
		list_image->pixbuf = nullptr;
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

void ListBoxX::SetDelegate(IListBoxDelegate *lbDelegate) {
	delegate = lbDelegate;
}

void ListBoxX::SetList(const char *listText, char separator, char typesep) {
	Clear();
	const size_t count = strlen(listText) + 1;
	std::vector<char> words(listText, listText+count);
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

Menu::Menu() noexcept : mid(nullptr) {}

void Menu::CreatePopUp() {
	Destroy();
	mid = gtk_menu_new();
	g_object_ref_sink(G_OBJECT(mid));
}

void Menu::Destroy() {
	if (mid)
		g_object_unref(G_OBJECT(mid));
	mid = nullptr;
}

#if !GTK_CHECK_VERSION(3,22,0)
static void MenuPositionFunc(GtkMenu *, gint *x, gint *y, gboolean *, gpointer userData) noexcept {
	sptr_t intFromPointer = GPOINTER_TO_INT(userData);
	*x = intFromPointer & 0xffff;
	*y = intFromPointer >> 16;
}
#endif

void Menu::Show(Point pt, Window &w) {
	GtkMenu *widget = static_cast<GtkMenu *>(mid);
	gtk_widget_show_all(GTK_WIDGET(widget));
#if GTK_CHECK_VERSION(3,22,0)
	// Rely on GTK+ to do the right thing with positioning
	gtk_menu_popup_at_pointer(widget, nullptr);
#else
	const GdkRectangle rcMonitor = MonitorRectangleForWidget(PWidget(w.GetID()));
	GtkRequisition requisition;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_get_preferred_size(GTK_WIDGET(widget), nullptr, &requisition);
#else
	gtk_widget_size_request(GTK_WIDGET(widget), &requisition);
#endif
	if ((pt.x + requisition.width) > rcMonitor.x + rcMonitor.width) {
		pt.x = rcMonitor.x + rcMonitor.width - requisition.width;
	}
	if ((pt.y + requisition.height) > rcMonitor.y + rcMonitor.height) {
		pt.y = rcMonitor.y + rcMonitor.height - requisition.height;
	}
	gtk_menu_popup(widget, nullptr, nullptr, MenuPositionFunc,
		       GINT_TO_POINTER((static_cast<int>(pt.y) << 16) | static_cast<int>(pt.x)), 0,
		       gtk_get_current_event_time());
#endif
}

class DynamicLibraryImpl : public DynamicLibrary {
protected:
	GModule *m;
public:
	explicit DynamicLibraryImpl(const char *modulePath) noexcept {
		m = g_module_open(modulePath, G_MODULE_BIND_LAZY);
	}
	// Deleted so DynamicLibraryImpl objects can not be copied.
	DynamicLibraryImpl(const DynamicLibraryImpl&) = delete;
	DynamicLibraryImpl(DynamicLibraryImpl&&) = delete;
	DynamicLibraryImpl&operator=(const DynamicLibraryImpl&) = delete;
	DynamicLibraryImpl&operator=(DynamicLibraryImpl&&) = delete;
	~DynamicLibraryImpl() override {
		if (m != nullptr)
			g_module_close(m);
	}

	// Use g_module_symbol to get a pointer to the relevant function.
	Function FindFunction(const char *name) override {
		if (m != nullptr) {
			gpointer fn_address = nullptr;
			const gboolean status = g_module_symbol(m, name, &fn_address);
			if (status)
				return static_cast<Function>(fn_address);
			else
				return nullptr;
		} else {
			return nullptr;
		}
	}

	bool IsValid() override {
		return m != nullptr;
	}
};

DynamicLibrary *DynamicLibrary::Load(const char *modulePath) {
	return static_cast<DynamicLibrary *>(new DynamicLibraryImpl(modulePath));
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

void Platform::DebugDisplay(const char *s) {
	fprintf(stderr, "%s", s);
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
	const bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) {
	char buffer[2000];
	g_snprintf(buffer, sizeof(buffer), "Assertion [%s] failed at %s %d\r\n", c, file, line);
	Platform::DebugDisplay(buffer);
	abort();
}

void Platform_Initialise() {
}

void Platform_Finalise() {
}
