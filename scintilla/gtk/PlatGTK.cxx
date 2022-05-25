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
#include <optional>
#include <algorithm>
#include <memory>
#include <sstream>

#include <glib.h>
#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdkwayland.h>
#endif

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "XPM.h"
#include "UniConversion.h"

#include "Wrappers.h"
#include "Converter.h"

#ifdef _MSC_VER
// Ignore unreferenced local functions in GTK+ headers
#pragma warning(disable: 4505)
#endif

using namespace Scintilla;
using namespace Scintilla::Internal;

namespace {

constexpr double kPi = 3.14159265358979323846;

constexpr double degrees = kPi / 180.0;

struct IntegerRectangle {
	int left;
	int top;
	int right;
	int bottom;

	explicit IntegerRectangle(PRectangle rc) noexcept :
		left(static_cast<int>(rc.left)), top(static_cast<int>(rc.top)),
		right(static_cast<int>(rc.right)), bottom(static_cast<int>(rc.bottom)) {
	}
	int Width() const noexcept { return right - left; }
	int Height() const noexcept { return bottom - top; }
};

GtkWidget *PWidget(WindowID wid) noexcept {
	return static_cast<GtkWidget *>(wid);
}

void SetFractionalPositions([[maybe_unused]] PangoContext *pcontext) noexcept {
#if PANGO_VERSION_CHECK(1,44,3)
	pango_context_set_round_glyph_positions(pcontext, FALSE);
#endif
}

void LayoutSetText(PangoLayout *layout, std::string_view text) noexcept {
	pango_layout_set_text(layout, text.data(), static_cast<int>(text.length()));
}

enum class EncodingType { singleByte, utf8, dbcs };

// Holds a PangoFontDescription*.
class FontHandle : public Font {
public:
	UniquePangoFontDescription fd;
	CharacterSet characterSet;
	explicit FontHandle(const FontParameters &fp) :
		fd(pango_font_description_new()), characterSet(fp.characterSet) {
		if (fd) {
			pango_font_description_set_family(fd.get(),
				(fp.faceName[0] == '!') ? fp.faceName + 1 : fp.faceName);
			pango_font_description_set_size(fd.get(), pango_units_from_double(fp.size));
			pango_font_description_set_weight(fd.get(), static_cast<PangoWeight>(fp.weight));
			pango_font_description_set_style(fd.get(), fp.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
		}
	}
	~FontHandle() override = default;
};

// X has a 16 bit coordinate space, so stop drawing here to avoid wrapping
constexpr int maxCoordinate = 32000;

const FontHandle *PFont(const Font *f) noexcept {
	return dynamic_cast<const FontHandle *>(f);
}

}

std::shared_ptr<Font> Font::Allocate(const FontParameters &fp) {
	return std::make_shared<FontHandle>(fp);
}

namespace Scintilla {

// SurfaceID is a cairo_t*
class SurfaceImpl : public Surface {
	SurfaceMode mode;
	EncodingType et= EncodingType::singleByte;
	WindowID widSave = nullptr;
	cairo_t *context = nullptr;
	UniqueCairo cairoOwned;
	UniqueCairoSurface surf;
	bool inited = false;
	UniquePangoContext pcontext;
	double resolution = 1.0;
	PangoDirection direction = PANGO_DIRECTION_LTR;
	const cairo_font_options_t *fontOptions = nullptr;
	PangoLanguage *language = nullptr;
	UniquePangoLayout layout;
	Converter conv;
	CharacterSet characterSet = static_cast<CharacterSet>(-1);

	void PenColourAlpha(ColourRGBA fore) noexcept;
	void SetConverter(CharacterSet characterSet_);
	void CairoRectangle(PRectangle rc) noexcept;
public:
	SurfaceImpl() noexcept;
	SurfaceImpl(cairo_t *context_, int width, int height, SurfaceMode mode_, WindowID wid) noexcept;
	// Deleted so SurfaceImpl objects can not be copied.
	SurfaceImpl(const SurfaceImpl&) = delete;
	SurfaceImpl(SurfaceImpl&&) = delete;
	SurfaceImpl&operator=(const SurfaceImpl&) = delete;
	SurfaceImpl&operator=(SurfaceImpl&&) = delete;
	~SurfaceImpl() override = default;

	void GetContextState() noexcept;
	UniquePangoContext MeasuringContext();

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

	void DrawTextBase(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore);
	void DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) override;
	void MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthText(const Font *font_, std::string_view text) override;

	void DrawTextBaseUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore);
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

const Supports SupportsGTK[] = {
	Supports::LineDrawsFinal,
	Supports::FractionalStrokeWidth,
	Supports::TranslucentStroke,
	Supports::PixelModification,
	Supports::ThreadSafeMeasureWidths,
};

}

const char *CharacterSetID(CharacterSet characterSet) noexcept {
	switch (characterSet) {
	case CharacterSet::Ansi:
		return "";
	case CharacterSet::Default:
		return "ISO-8859-1";
	case CharacterSet::Baltic:
		return "ISO-8859-13";
	case CharacterSet::ChineseBig5:
		return "BIG-5";
	case CharacterSet::EastEurope:
		return "ISO-8859-2";
	case CharacterSet::GB2312:
		return "CP936";
	case CharacterSet::Greek:
		return "ISO-8859-7";
	case CharacterSet::Hangul:
		return "CP949";
	case CharacterSet::Mac:
		return "MACINTOSH";
	case CharacterSet::Oem:
		return "ASCII";
	case CharacterSet::Russian:
		return "KOI8-R";
	case CharacterSet::Oem866:
		return "CP866";
	case CharacterSet::Cyrillic:
		return "CP1251";
	case CharacterSet::ShiftJis:
		return "SHIFT-JIS";
	case CharacterSet::Symbol:
		return "";
	case CharacterSet::Turkish:
		return "ISO-8859-9";
	case CharacterSet::Johab:
		return "CP1361";
	case CharacterSet::Hebrew:
		return "ISO-8859-8";
	case CharacterSet::Arabic:
		return "ISO-8859-6";
	case CharacterSet::Vietnamese:
		return "";
	case CharacterSet::Thai:
		return "ISO-8859-11";
	case CharacterSet::Iso8859_15:
		return "ISO-8859-15";
	default:
		return "";
	}
}

void SurfaceImpl::PenColourAlpha(ColourRGBA fore) noexcept {
	if (context) {
		cairo_set_source_rgba(context,
			fore.GetRedComponent(),
			fore.GetGreenComponent(),
			fore.GetBlueComponent(),
			fore.GetAlphaComponent());
	}
}

void SurfaceImpl::SetConverter(CharacterSet characterSet_) {
	if (characterSet != characterSet_) {
		characterSet = characterSet_;
		conv.Open("UTF-8", CharacterSetID(characterSet), false);
	}
}

void SurfaceImpl::CairoRectangle(PRectangle rc) noexcept {
	cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
}

SurfaceImpl::SurfaceImpl() noexcept {
}

SurfaceImpl::SurfaceImpl(cairo_t *context_, int width, int height, SurfaceMode mode_, WindowID wid) noexcept {
	if (height > 0 && width > 0) {
		cairo_surface_t *psurfContext = cairo_get_target(context_);
		surf.reset(cairo_surface_create_similar(
			psurfContext,
			CAIRO_CONTENT_COLOR_ALPHA, width, height));
		cairoOwned.reset(cairo_create(surf.get()));
		context = cairoOwned.get();
		pcontext.reset(gtk_widget_create_pango_context(PWidget(wid)));
		PLATFORM_ASSERT(pcontext);
		SetFractionalPositions(pcontext.get());
		GetContextState();
		layout.reset(pango_layout_new(pcontext.get()));
		PLATFORM_ASSERT(layout);
		cairo_rectangle(context, 0, 0, width, height);
		cairo_set_source_rgb(context, 1.0, 0, 0);
		cairo_fill(context);
		cairo_set_line_width(context, 1);
		inited = true;
		mode = mode_;
	}
}

void SurfaceImpl::Release() noexcept {
	et = EncodingType::singleByte;
	cairoOwned.reset();
	context = nullptr;
	surf.reset();
	layout.reset();
	// fontOptions and language are owned by original context and don't need to be freed
	fontOptions = nullptr;
	language = nullptr;
	pcontext.reset();
	conv.Close();
	characterSet = static_cast<CharacterSet>(-1);
	inited = false;
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

void SurfaceImpl::GetContextState() noexcept {
	resolution = pango_cairo_context_get_resolution(pcontext.get());
	direction = pango_context_get_base_dir(pcontext.get());
	fontOptions = pango_cairo_context_get_font_options(pcontext.get());
	language = pango_context_get_language(pcontext.get());
}

UniquePangoContext SurfaceImpl::MeasuringContext() {
	UniquePangoFontMap fmMeasure(pango_cairo_font_map_get_default());
	PLATFORM_ASSERT(fmMeasure);
	UniquePangoContext contextMeasure(pango_font_map_create_context(fmMeasure.release()));
	PLATFORM_ASSERT(contextMeasure);
	SetFractionalPositions(contextMeasure.get());

	pango_cairo_context_set_resolution(contextMeasure.get(), resolution);
	pango_context_set_base_dir(contextMeasure.get(), direction);
	pango_cairo_context_set_font_options(contextMeasure.get(), fontOptions);
	pango_context_set_language(contextMeasure.get(), language);
	
	return contextMeasure;
}

void SurfaceImpl::Init(WindowID wid) {
	widSave = wid;
	Release();
	PLATFORM_ASSERT(wid);
	// if we are only created from a window ID, we can't perform drawing
	context = nullptr;
	pcontext.reset(gtk_widget_create_pango_context(PWidget(wid)));
	PLATFORM_ASSERT(pcontext);
	SetFractionalPositions(pcontext.get());
	GetContextState();
	layout.reset(pango_layout_new(pcontext.get()));
	PLATFORM_ASSERT(layout);
	inited = true;
}

void SurfaceImpl::Init(SurfaceID sid, WindowID wid) {
	widSave = wid;
	PLATFORM_ASSERT(sid);
	Release();
	PLATFORM_ASSERT(wid);
	cairoOwned.reset(cairo_reference(static_cast<cairo_t *>(sid)));
	context = cairoOwned.get();
	pcontext.reset(gtk_widget_create_pango_context(PWidget(wid)));
	SetFractionalPositions(pcontext.get());
	// update the Pango context in case sid isn't the widget's surface
	pango_cairo_update_context(context, pcontext.get());
	GetContextState();
	layout.reset(pango_layout_new(pcontext.get()));
	cairo_set_line_width(context, 1);
	inited = true;
}

std::unique_ptr<Surface> SurfaceImpl::AllocatePixMap(int width, int height) {
	// widSave must be alive now so safe for creating a PangoContext
	return std::make_unique<SurfaceImpl>(context, width, height, mode, widSave);
}

void SurfaceImpl::SetMode(SurfaceMode mode_) {
	mode = mode_;
	if (mode.codePage == SC_CP_UTF8) {
		et = EncodingType::utf8;
	} else if (mode.codePage) {
		et = EncodingType::dbcs;
	} else {
		et = EncodingType::singleByte;
	}
}

int SurfaceImpl::SupportsFeature(Supports feature) noexcept {
	for (const Supports f : SupportsGTK) {
		if (f == feature)
			return 1;
	}
	return 0;
}

int SurfaceImpl::LogPixelsY() {
	return 72;
}

int SurfaceImpl::PixelDivisions() {
	// GTK uses device pixels.
	return 1;
}

int SurfaceImpl::DeviceHeightFont(int points) {
	const int logPix = LogPixelsY();
	return (points * logPix + logPix / 2) / 72;
}

void SurfaceImpl::LineDraw(Point start, Point end, Stroke stroke) {
	PLATFORM_ASSERT(context);
	if (!context)
		return;
	PenColourAlpha(stroke.colour);
	cairo_set_line_width(context, stroke.width);
	cairo_move_to(context, start.x, start.y);
	cairo_line_to(context, end.x, end.y);
	cairo_stroke(context);
}

void SurfaceImpl::PolyLine(const Point *pts, size_t npts, Stroke stroke) {
	// TODO: set line joins and caps
	PLATFORM_ASSERT(context && npts > 1);
	if (!context)
		return;
	PenColourAlpha(stroke.colour);
	cairo_set_line_width(context, stroke.width);
	cairo_move_to(context, pts[0].x, pts[0].y);
	for (size_t i = 1; i < npts; i++) {
		cairo_line_to(context, pts[i].x, pts[i].y);
	}
	cairo_stroke(context);
}

void SurfaceImpl::Polygon(const Point *pts, size_t npts, FillStroke fillStroke) {
	PLATFORM_ASSERT(context);
	PenColourAlpha(fillStroke.fill.colour);
	cairo_move_to(context, pts[0].x, pts[0].y);
	for (size_t i = 1; i < npts; i++) {
		cairo_line_to(context, pts[i].x, pts[i].y);
	}
	cairo_close_path(context);
	cairo_fill_preserve(context);
	PenColourAlpha(fillStroke.stroke.colour);
	cairo_set_line_width(context, fillStroke.stroke.width);
	cairo_stroke(context);
}

void SurfaceImpl::RectangleDraw(PRectangle rc, FillStroke fillStroke) {
	if (context) {
		CairoRectangle(rc.Inset(fillStroke.stroke.width / 2));
		PenColourAlpha(fillStroke.fill.colour);
		cairo_fill_preserve(context);
		PenColourAlpha(fillStroke.stroke.colour);
		cairo_set_line_width(context, fillStroke.stroke.width);
		cairo_stroke(context);
	}
}

void SurfaceImpl::RectangleFrame(PRectangle rc, Stroke stroke) {
	if (context) {
		CairoRectangle(rc.Inset(stroke.width / 2));
		PenColourAlpha(stroke.colour);
		cairo_set_line_width(context, stroke.width);
		cairo_stroke(context);
	}
}

void SurfaceImpl::FillRectangle(PRectangle rc, Fill fill) {
	PenColourAlpha(fill.colour);
	if (context && (rc.left < maxCoordinate)) {	// Protect against out of range
		CairoRectangle(rc);
		cairo_fill(context);
	}
}

void SurfaceImpl::FillRectangleAligned(PRectangle rc, Fill fill) {
	FillRectangle(PixelAlign(rc, 1), fill);
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceImpl &surfi = dynamic_cast<SurfaceImpl &>(surfacePattern);
	if (context && surfi.surf) {
		// Tile pattern over rectangle
		cairo_set_source_surface(context, surfi.surf.get(), rc.left, rc.top);
		cairo_pattern_set_extend(cairo_get_source(context), CAIRO_EXTEND_REPEAT);
		cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
		cairo_fill(context);
	}
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, FillStroke fillStroke) {
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
		Polygon(pts, std::size(pts), fillStroke);
	} else {
		RectangleDraw(rc, fillStroke);
	}
}

static void PathRoundRectangle(cairo_t *context, double left, double top, double width, double height, double radius) noexcept {
	cairo_new_sub_path(context);
	cairo_arc(context, left + width - radius, top + radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(context, left + width - radius, top + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(context, left + radius, top + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(context, left + radius, top + radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(context);
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) {
	if (context && rc.Width() > 0) {
		const XYPOSITION halfStroke = fillStroke.stroke.width / 2.0;
		const XYPOSITION doubleStroke = fillStroke.stroke.width * 2.0;
		PenColourAlpha(fillStroke.fill.colour);
		if (cornerSize > 0)
			PathRoundRectangle(context, rc.left + fillStroke.stroke.width, rc.top + fillStroke.stroke.width,
				rc.Width() - doubleStroke, rc.Height() - doubleStroke, cornerSize);
		else
			cairo_rectangle(context, rc.left + fillStroke.stroke.width, rc.top + fillStroke.stroke.width,
				rc.Width() - doubleStroke, rc.Height() - doubleStroke);
		cairo_fill(context);

		PenColourAlpha(fillStroke.stroke.colour);
		if (cornerSize > 0)
			PathRoundRectangle(context, rc.left + halfStroke, rc.top + halfStroke,
				rc.Width() - fillStroke.stroke.width, rc.Height() - fillStroke.stroke.width, cornerSize);
		else
			cairo_rectangle(context, rc.left + halfStroke, rc.top + halfStroke,
				rc.Width() - fillStroke.stroke.width, rc.Height() - fillStroke.stroke.width);
		cairo_set_line_width(context, fillStroke.stroke.width);
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

	UniqueCairoSurface surfImage(cairo_image_surface_create_for_data(&image[0], CAIRO_FORMAT_ARGB32, width, height, stride));
	cairo_set_source_surface(context, surfImage.get(), rc.left, rc.top);
	cairo_rectangle(context, rc.left, rc.top, rc.Width(), rc.Height());
	cairo_fill(context);
}

void SurfaceImpl::Ellipse(PRectangle rc, FillStroke fillStroke) {
	PLATFORM_ASSERT(context);
	PenColourAlpha(fillStroke.fill.colour);
	cairo_arc(context, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2,
		  (std::min(rc.Width(), rc.Height()) - fillStroke.stroke.width) / 2, 0, 2*kPi);
	cairo_fill_preserve(context);
	PenColourAlpha(fillStroke.stroke.colour);
	cairo_set_line_width(context, fillStroke.stroke.width);
	cairo_stroke(context);
}

void SurfaceImpl::Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) {
	const XYPOSITION midLine = rc.Centre().y;
	const XYPOSITION halfStroke = fillStroke.stroke.width / 2.0f;
	const XYPOSITION radius = rc.Height() / 2.0f - halfStroke;
	PRectangle rcInner = rc;
	rcInner.left += radius;
	rcInner.right -= radius;

	cairo_new_sub_path(context);

	const Ends leftSide = static_cast<Ends>(static_cast<int>(ends) & 0xf);
	const Ends rightSide = static_cast<Ends>(static_cast<int>(ends) & 0xf0);
	switch (leftSide) {
		case Ends::leftFlat:
			cairo_move_to(context, rc.left + halfStroke, rc.top + halfStroke);
			cairo_line_to(context, rc.left + halfStroke, rc.bottom - halfStroke);
			break;
		case Ends::leftAngle:
			cairo_move_to(context, rcInner.left + halfStroke, rc.top + halfStroke);
			cairo_line_to(context, rc.left + halfStroke, rc.Centre().y);
			cairo_line_to(context, rcInner.left + halfStroke, rc.bottom - halfStroke);
			break;
		case Ends::semiCircles:
		default:
			cairo_move_to(context, rcInner.left + halfStroke, rc.top + halfStroke);
			cairo_arc_negative(context, rcInner.left + halfStroke, midLine, radius,
				270 * degrees, 90 * degrees);
			break;
	}

	switch (rightSide) {
		case Ends::rightFlat:
			cairo_line_to(context, rc.right - halfStroke, rc.bottom - halfStroke);
			cairo_line_to(context, rc.right - halfStroke, rc.top + halfStroke);
			break;
		case Ends::rightAngle:
			cairo_line_to(context, rcInner.right - halfStroke, rc.bottom - halfStroke);
			cairo_line_to(context, rc.right - halfStroke, rc.Centre().y);
			cairo_line_to(context, rcInner.right - halfStroke, rc.top + halfStroke);
			break;
		case Ends::semiCircles:
		default:
			cairo_line_to(context, rcInner.right - halfStroke, rc.bottom - halfStroke);
			cairo_arc_negative(context, rcInner.right - halfStroke, midLine, radius,
				90 * degrees, 270 * degrees);
			break;
	}

	// Close the path to enclose it for stroking and for filling, then draw it
	cairo_close_path(context);
	PenColourAlpha(fillStroke.fill.colour);
	cairo_fill_preserve(context);

	PenColourAlpha(fillStroke.stroke.colour);
	cairo_set_line_width(context, fillStroke.stroke.width);
	cairo_stroke(context);
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceImpl &surfi = static_cast<SurfaceImpl &>(surfaceSource);
	const bool canDraw = surfi.surf != nullptr;
	if (canDraw) {
		PLATFORM_ASSERT(context);
		cairo_set_source_surface(context, surfi.surf.get(),
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

void SurfaceImpl::DrawTextBase(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
			       ColourRGBA fore) {
	if (context) {
		PenColourAlpha(fore);
		const XYPOSITION xText = rc.left;
		if (PFont(font_)->fd) {
			std::string utfForm;
			if (et == EncodingType::utf8) {
				LayoutSetText(layout.get(), text);
			} else {
				SetConverter(PFont(font_)->characterSet);
				utfForm = UTF8FromIconv(conv, text);
				if (utfForm.empty()) {	// iconv failed so treat as Latin1
					utfForm = UTF8FromLatin1(text);
				}
				LayoutSetText(layout.get(), utfForm);
			}
			pango_layout_set_font_description(layout.get(), PFont(font_)->fd.get());
			pango_cairo_update_layout(context, layout.get());
			PangoLayoutLine *pll = pango_layout_get_line_readonly(layout.get(), 0);
			cairo_move_to(context, xText, ybase);
			pango_cairo_show_layout_line(context, pll);
		}
	}
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
				 ColourRGBA fore, ColourRGBA back) {
	FillRectangleAligned(rc, back);
	DrawTextBase(rc, font_, ybase, text, fore);
}

// On GTK+, exactly same as DrawTextNoClip
void SurfaceImpl::DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
				  ColourRGBA fore, ColourRGBA back) {
	FillRectangleAligned(rc, back);
	DrawTextBase(rc, font_, ybase, text, fore);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
				      ColourRGBA fore) {
	// Avoid drawing spaces in transparent mode
	for (size_t i=0; i<text.length(); i++) {
		if (text[i] != ' ') {
			DrawTextBase(rc, font_, ybase, text, fore);
			return;
		}
	}
}

namespace {

class ClusterIterator {
	UniquePangoLayoutIter iter;
	PangoRectangle pos {};
	int lenPositions;
public:
	bool finished = false;
	XYPOSITION positionStart = 0.0;
	XYPOSITION position = 0.0;
	XYPOSITION distance = 0.0;
	int curIndex = 0;
	ClusterIterator(PangoLayout *layout, std::string_view text) noexcept :
		lenPositions(static_cast<int>(text.length())) {
		LayoutSetText(layout, text);
		iter.reset(pango_layout_get_iter(layout));
		curIndex = pango_layout_iter_get_index(iter.get());
		pango_layout_iter_get_cluster_extents(iter.get(), nullptr, &pos);
	}

	void Next() noexcept {
		positionStart = position;
		if (pango_layout_iter_next_cluster(iter.get())) {
			pango_layout_iter_get_cluster_extents(iter.get(), nullptr, &pos);
			position = pango_units_to_double(pos.x);
			curIndex = pango_layout_iter_get_index(iter.get());
		} else {
			finished = true;
			position = pango_units_to_double(pos.x + pos.width);
			curIndex = pango_layout_iter_get_index(iter.get());
		}
		distance = position - positionStart;
	}
};

// Something has gone wrong so set all the characters as equally spaced.
void EquallySpaced(PangoLayout *layout, XYPOSITION *positions, size_t lenPositions) {
	int widthLayout = 0;
	pango_layout_get_size(layout, &widthLayout, nullptr);
	const XYPOSITION widthTotal = pango_units_to_double(widthLayout);
	for (size_t bytePos=0; bytePos<lenPositions; bytePos++) {
		positions[bytePos] = widthTotal / lenPositions * (bytePos + 1);
	}
}

}

void SurfaceImpl::MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) {
	if (PFont(font_)->fd) {
		UniquePangoContext contextMeasure = MeasuringContext();
		UniquePangoLayout layoutMeasure(pango_layout_new(contextMeasure.get()));
		PLATFORM_ASSERT(layoutMeasure);

		pango_layout_set_font_description(layoutMeasure.get(), PFont(font_)->fd.get());
		if (et == EncodingType::utf8) {
			// Simple and direct as UTF-8 is native Pango encoding
			ClusterIterator iti(layoutMeasure.get(), text);
			int i = iti.curIndex;
			if (i != 0) {
				// Unexpected start to iteration, could be bidirectional text
				EquallySpaced(layoutMeasure.get(), positions, text.length());
				return;
			}
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
			if (et == EncodingType::dbcs) {
				SetConverter(PFont(font_)->characterSet);
				std::string utfForm = UTF8FromIconv(conv, text);
				if (!utfForm.empty()) {
					// Convert to UTF-8 so can ask Pango for widths, then
					// Loop through UTF-8 and DBCS forms, taking account of different
					// character byte lengths.
					Converter convMeasure("UCS-2", CharacterSetID(characterSet), false);
					int i = 0;
					ClusterIterator iti(layoutMeasure.get(), utfForm);
					int clusterStart = iti.curIndex;
					if (clusterStart != 0) {
						// Unexpected start to iteration, could be bidirectional text
						EquallySpaced(layoutMeasure.get(), positions, text.length());
						return;
					}
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
				const bool rtlCheck = PFont(font_)->characterSet == CharacterSet::Hebrew ||
							    PFont(font_)->characterSet == CharacterSet::Arabic;
				std::string utfForm = UTF8FromIconv(conv, text);
				if (utfForm.empty()) {
					utfForm = UTF8FromLatin1(text);
				}
				size_t i = 0;
				// Each 8-bit input character may take 1 or 2 bytes in UTF-8
				// and groups of up to 3 may be represented as ligatures.
				ClusterIterator iti(layoutMeasure.get(), utfForm);
				int clusterStart = iti.curIndex;
				if (clusterStart != 0) {
					// Unexpected start to iteration, could be bidirectional text
					EquallySpaced(layoutMeasure.get(), positions, lenPositions);
					return;
				}
				while (!iti.finished) {
					iti.Next();
					const int clusterEnd = iti.curIndex;
					const int ligatureLength = g_utf8_strlen(utfForm.c_str() + clusterStart, clusterEnd - clusterStart);
					if (rtlCheck && ((clusterEnd <= clusterStart) || (ligatureLength == 0) || (ligatureLength > 3))) {
						// Something has gone wrong: exit quickly but pretend all the characters are equally spaced:
						EquallySpaced(layoutMeasure.get(), positions, lenPositions);
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
	} else {
		// No font so return an ascending range of values
		for (size_t i = 0; i < text.length(); i++) {
			positions[i] = i + 1.0;
		}
	}
}

XYPOSITION SurfaceImpl::WidthText(const Font *font_, std::string_view text) {
	if (PFont(font_)->fd) {
		std::string utfForm;
		pango_layout_set_font_description(layout.get(), PFont(font_)->fd.get());
		if (et == EncodingType::utf8) {
			LayoutSetText(layout.get(), text);
		} else {
			SetConverter(PFont(font_)->characterSet);
			utfForm = UTF8FromIconv(conv, text);
			if (utfForm.empty()) {	// iconv failed so treat as Latin1
				utfForm = UTF8FromLatin1(text);
			}
			LayoutSetText(layout.get(), utfForm);
		}
		PangoLayoutLine *pangoLine = pango_layout_get_line_readonly(layout.get(), 0);
		PangoRectangle pos {};
		pango_layout_line_get_extents(pangoLine, nullptr, &pos);
		return pango_units_to_double(pos.width);
	}
	return 1;
}

void SurfaceImpl::DrawTextBaseUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore) {
	if (context) {
		PenColourAlpha(fore);
		const XYPOSITION xText = rc.left;
		if (PFont(font_)->fd) {
			LayoutSetText(layout.get(), text);
			pango_layout_set_font_description(layout.get(), PFont(font_)->fd.get());
			pango_cairo_update_layout(context, layout.get());
			PangoLayoutLine *pll = pango_layout_get_line_readonly(layout.get(), 0);
			cairo_move_to(context, xText, ybase);
			pango_cairo_show_layout_line(context, pll);
		}
	}
}

void SurfaceImpl::DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	FillRectangleAligned(rc, back);
	DrawTextBaseUTF8(rc, font_, ybase, text, fore);
}

// On GTK+, exactly same as DrawTextNoClip
void SurfaceImpl::DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	FillRectangleAligned(rc, back);
	DrawTextBaseUTF8(rc, font_, ybase, text, fore);
}

void SurfaceImpl::DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore) {
	// Avoid drawing spaces in transparent mode
	for (size_t i = 0; i < text.length(); i++) {
		if (text[i] != ' ') {
			DrawTextBaseUTF8(rc, font_, ybase, text, fore);
			return;
		}
	}
}

void SurfaceImpl::MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) {
	if (PFont(font_)->fd) {
		UniquePangoContext contextMeasure = MeasuringContext();
		UniquePangoLayout layoutMeasure(pango_layout_new(contextMeasure.get()));
		PLATFORM_ASSERT(layoutMeasure);

		pango_layout_set_font_description(layoutMeasure.get(), PFont(font_)->fd.get());
		// Simple and direct as UTF-8 is native Pango encoding
		ClusterIterator iti(layoutMeasure.get(), text);
		int i = iti.curIndex;
		if (i != 0) {
			// Unexpected start to iteration, could be bidirectional text
			EquallySpaced(layoutMeasure.get(), positions, text.length());
			return;
		}
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
		// No font so return an ascending range of values
		for (size_t i = 0; i < text.length(); i++) {
			positions[i] = i + 1.0;
		}
	}
}

XYPOSITION SurfaceImpl::WidthTextUTF8(const Font *font_, std::string_view text) {
	if (PFont(font_)->fd) {
		pango_layout_set_font_description(layout.get(), PFont(font_)->fd.get());
		LayoutSetText(layout.get(), text);
		PangoLayoutLine *pangoLine = pango_layout_get_line_readonly(layout.get(), 0);
		PangoRectangle pos{};
		pango_layout_line_get_extents(pangoLine, nullptr, &pos);
		return pango_units_to_double(pos.width);
	}
	return 1;
}

// Ascent and descent determined by Pango font metrics.

XYPOSITION SurfaceImpl::Ascent(const Font *font_) {
	if (!PFont(font_)->fd) {
		return 1.0;
	}
	UniquePangoFontMetrics metrics(pango_context_get_metrics(pcontext.get(),
				    PFont(font_)->fd.get(), language));
	return std::max(1.0, std::ceil(pango_units_to_double(
				    pango_font_metrics_get_ascent(metrics.get()))));
}

XYPOSITION SurfaceImpl::Descent(const Font *font_) {
	if (!PFont(font_)->fd) {
		return 0.0;
	}
	UniquePangoFontMetrics metrics(pango_context_get_metrics(pcontext.get(),
				    PFont(font_)->fd.get(), language));
	return std::ceil(pango_units_to_double(pango_font_metrics_get_descent(metrics.get())));
}

XYPOSITION SurfaceImpl::InternalLeading(const Font *) {
	return 0;
}

XYPOSITION SurfaceImpl::Height(const Font *font_) {
	return Ascent(font_) + Descent(font_);
}

XYPOSITION SurfaceImpl::AverageCharWidth(const Font *font_) {
	return WidthText(font_, "n");
}

void SurfaceImpl::SetClip(PRectangle rc) {
	PLATFORM_ASSERT(context);
	cairo_save(context);
	CairoRectangle(rc);
	cairo_clip(context);
}

void SurfaceImpl::PopClip() {
	PLATFORM_ASSERT(context);
	cairo_restore(context);
}

void SurfaceImpl::FlushCachedState() {}

void SurfaceImpl::FlushDrawing() {
}

std::unique_ptr<Surface> Surface::Allocate(Technology) {
	return std::make_unique<SurfaceImpl>();
}

Window::~Window() noexcept {}

void Window::Destroy() noexcept {
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
	GtkAllocation alloc {};
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
#if defined(GDK_WINDOWING_WAYLAND)
	if (GDK_IS_WAYLAND_DISPLAY(pdisplay)) {
		// The GDK behavior on Wayland is not self-consistent, we must correct the display coordinates to match
		// the coordinate space used in gtk_window_move. See also https://sourceforge.net/p/scintilla/bugs/2296/
		rcScreen.x = 0;
		rcScreen.y = 0;
	}
#endif
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

void Window::SetCursor(Cursor curs) {
	// We don't set the cursor to same value numerous times under gtk because
	// it stores the cursor in the window once it's set
	if (curs == cursorLast)
		return;

	cursorLast = curs;
	GdkDisplay *pdisplay = gtk_widget_get_display(PWidget(wid));

	GdkCursor *gdkCurs;
	switch (curs) {
	case Cursor::text:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_XTERM);
		break;
	case Cursor::arrow:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_LEFT_PTR);
		break;
	case Cursor::up:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_CENTER_PTR);
		break;
	case Cursor::wait:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_WATCH);
		break;
	case Cursor::hand:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_HAND2);
		break;
	case Cursor::reverseArrow:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_RIGHT_PTR);
		break;
	default:
		gdkCurs = gdk_cursor_new_for_display(pdisplay, GDK_LEFT_PTR);
		cursorLast = Cursor::arrow;
		break;
	}

	if (WindowFromWidget(PWidget(wid)))
		gdk_window_set_cursor(WindowFromWidget(PWidget(wid)), gdkCurs);
	UnRefCursor(gdkCurs);
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
				 static_cast<gint>(pt.x) + x_offset, static_cast<gint>(pt.y) + y_offset);
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

ListBox::~ListBox() noexcept {
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
	std::unique_ptr<GtkCssProvider, GObjectReleaser> cssProvider;
#endif
public:
	IListBoxDelegate *delegate;

	ListBoxX() noexcept : widCached(nullptr), frame(nullptr), list(nullptr), scroller(nullptr),
		pixhash(nullptr), pixbuf_renderer(nullptr),
		renderer(nullptr),
		desiredVisibleRows(5), maxItemCharacters(0),
		aveCharWidth(1),
		delegate(nullptr) {
	}
	// Deleted so ListBoxX objects can not be copied.
	ListBoxX(const ListBoxX&) = delete;
	ListBoxX(ListBoxX&&) = delete;
	ListBoxX&operator=(const ListBoxX&) = delete;
	ListBoxX&operator=(ListBoxX&&) = delete;
	~ListBoxX() noexcept override {
		if (pixhash) {
			g_hash_table_foreach((GHashTable *) pixhash, list_image_free, nullptr);
			g_hash_table_destroy((GHashTable *) pixhash);
		}
		if (widCached) {
			gtk_widget_destroy(GTK_WIDGET(widCached));
			wid = widCached = nullptr;
		}
	}
	void SetFont(const Font *font) override;
	void Create(Window &parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_, Technology technology_) override;
	void SetAverageCharWidth(int width) override;
	void SetVisibleRows(int rows) override;
	int GetVisibleRows() const override;
	int GetRowHeight();
	PRectangle GetDesiredRect() override;
	int CaretFromEdge() override;
	void Clear() noexcept override;
	void Append(char *s, int type = -1) override;
	int Length() override;
	void Select(int n) override;
	int GetSelection() override;
	int Find(const char *prefix) override;
	std::string GetValue(int n) override;
	void RegisterRGBA(int type, std::unique_ptr<RGBAImage> image);
	void RegisterImage(int type, const char *xpm_data) override;
	void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) override;
	void ClearRegisteredImages() override;
	void SetDelegate(IListBoxDelegate *lbDelegate) override;
	void SetList(const char *listText, char separator, char typesep) override;
	void SetOptions(ListOptions options_) override;
};

std::unique_ptr<ListBox> ListBox::Allocate() {
	return std::make_unique<ListBoxX>();
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

static gboolean ButtonPress(GtkWidget *, const GdkEventButton *ev, gpointer p) {
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

static gboolean ButtonRelease(GtkWidget *, const GdkEventButton *ev, gpointer p) {
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

void ListBoxX::Create(Window &parent, int, Point, int, bool, Technology) {
	if (widCached != nullptr) {
		wid = widCached;
		return;
	}

#if GTK_CHECK_VERSION(3,0,0)
	if (!cssProvider) {
		cssProvider.reset(gtk_css_provider_new());
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
		gtk_style_context_add_provider(styleContext, GTK_STYLE_PROVIDER(cssProvider.get()),
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

void ListBoxX::SetFont(const Font *font) {
	// Only do for Pango font as there have been crashes for GDK fonts
	if (Created() && PFont(font)->fd) {
		// Current font is Pango font
#if GTK_CHECK_VERSION(3,0,0)
		if (cssProvider) {
			PangoFontDescription *pfd = PFont(font)->fd.get();
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
			gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(cssProvider.get()),
							ssFontSetting.str().c_str(), -1, nullptr);
		}
#else
		gtk_widget_modify_font(PWidget(list), PFont(font)->fd.get());
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

		const unsigned int width = std::max(maxItemCharacters, 12U);
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

void ListBoxX::Clear() noexcept {
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	maxItemCharacters = 0;
}

static void init_pixmap(ListImage *list_image) noexcept {
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
	const unsigned int len = static_cast<unsigned int>(strlen(s));
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
		gdouble value = (static_cast<gdouble>(n) / total) * (gtk_adjustment_get_upper(adj) - gtk_adjustment_get_lower(adj))
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

std::string ListBoxX::GetValue(int n) {
	char *text = nullptr;
	GtkTreeIter iter {};
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	const bool valid = gtk_tree_model_iter_nth_child(model, &iter, nullptr, n) != FALSE;
	if (valid) {
		gtk_tree_model_get(model, &iter, TEXT_COLUMN, &text, -1);
	}
	std::string value;
	if (text) {
		value = text;
	}
	g_free(text);
	return value;
}

// g_return_if_fail causes unnecessary compiler warning in release compile.
#ifdef _MSC_VER
#pragma warning(disable: 4127)
#endif

void ListBoxX::RegisterRGBA(int type, std::unique_ptr<RGBAImage> image) {
	images.AddImage(type, std::move(image));
	const RGBAImage * const observe = images.Get(type);

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
		list_image->rgba_data = observe;
	} else {
		list_image = g_new0(ListImage, 1);
		list_image->rgba_data = observe;
		g_hash_table_insert((GHashTable *) pixhash, GINT_TO_POINTER(type),
				    (gpointer) list_image);
	}
}

void ListBoxX::RegisterImage(int type, const char *xpm_data) {
	g_return_if_fail(xpm_data);
	XPM xpmImage(xpm_data);
	RegisterRGBA(type, std::make_unique<RGBAImage>(xpmImage));
}

void ListBoxX::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) {
	RegisterRGBA(type, std::make_unique<RGBAImage>(width, height, 1.0f, pixelsImage));
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

void ListBoxX::SetOptions(ListOptions) {
}

Menu::Menu() noexcept : mid(nullptr) {}

void Menu::CreatePopUp() {
	Destroy();
	mid = gtk_menu_new();
	g_object_ref_sink(G_OBJECT(mid));
}

void Menu::Destroy() noexcept {
	if (mid)
		g_object_unref(G_OBJECT(mid));
	mid = nullptr;
}

#if !GTK_CHECK_VERSION(3,22,0)
static void MenuPositionFunc(GtkMenu *, gint *x, gint *y, gboolean *, gpointer userData) noexcept {
	const gint intFromPointer = GPOINTER_TO_INT(userData);
	*x = intFromPointer & 0xffff;
	*y = intFromPointer >> 16;
}
#endif

void Menu::Show(Point pt, const Window &w) {
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

ColourRGBA Platform::Chrome() {
	return ColourRGBA(0xe0, 0xe0, 0xe0);
}

ColourRGBA Platform::ChromeHighlight() {
	return ColourRGBA(0xff, 0xff, 0xff);
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

void Platform::DebugDisplay(const char *s) noexcept {
	fprintf(stderr, "%s", s);
}

//#define TRACE

#ifdef TRACE
void Platform::DebugPrintf(const char *format, ...) noexcept {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsprintf(buffer, format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}
#else
void Platform::DebugPrintf(const char *, ...) noexcept {}

#endif

// Not supported for GTK+
static bool assertionPopUps = true;

bool Platform::ShowAssertionPopUps(bool assertionPopUps_) noexcept {
	const bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) noexcept {
	char buffer[2000];
	g_snprintf(buffer, sizeof(buffer), "Assertion [%s] failed at %s %d\r\n", c, file, line);
	Platform::DebugDisplay(buffer);
	abort();
}

void Platform_Initialise() {
}

void Platform_Finalise() {
}
