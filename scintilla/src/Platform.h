// Scintilla source code edit control
/** @file Platform.h
 ** Interface to platform facilities. Also includes some basic utilities.
 ** Implemented in PlatGTK.cxx for GTK+/Linux, PlatWin.cxx for Windows, and PlatWX.cxx for wxWindows.
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PLATFORM_H
#define PLATFORM_H

// PLAT_GTK = GTK+ on Linux or Win32
// PLAT_GTK_WIN32 is defined additionally when running PLAT_GTK under Win32
// PLAT_WIN = Win32 API on Win32 OS
// PLAT_WX is wxWindows on any supported platform
// PLAT_TK = Tcl/TK on Linux or Win32

#define PLAT_GTK 0
#define PLAT_GTK_WIN32 0
#define PLAT_GTK_MACOSX 0
#define PLAT_MACOSX 0
#define PLAT_WIN 0
#define PLAT_WX  0
#define PLAT_QT 0
#define PLAT_QT_QML 0
#define PLAT_FOX 0
#define PLAT_CURSES 0
#define PLAT_TK 0
#define PLAT_HAIKU 0

#if defined(FOX)
#undef PLAT_FOX
#define PLAT_FOX 1

#elif defined(__WX__)
#undef PLAT_WX
#define PLAT_WX  1

#elif defined(CURSES)
#undef PLAT_CURSES
#define PLAT_CURSES 1

#elif defined(__HAIKU__)
#undef PLAT_HAIKU
#define PLAT_HAIKU 1

#elif defined(SCINTILLA_QT)
#undef PLAT_QT
#define PLAT_QT 1

#elif defined(SCINTILLA_QT_QML)
#undef PLAT_QT_QML
#define PLAT_QT_QML 1

#elif defined(TK)
#undef PLAT_TK
#define PLAT_TK 1

#elif defined(GTK)
#undef PLAT_GTK
#define PLAT_GTK 1

#if defined(__WIN32__) || defined(_MSC_VER)
#undef PLAT_GTK_WIN32
#define PLAT_GTK_WIN32 1
#endif

#if defined(__APPLE__)
#undef PLAT_GTK_MACOSX
#define PLAT_GTK_MACOSX 1
#endif

#elif defined(__APPLE__)

#undef PLAT_MACOSX
#define PLAT_MACOSX 1

#else
#undef PLAT_WIN
#define PLAT_WIN 1

#endif

namespace Scintilla::Internal {

// Underlying the implementation of the platform classes are platform specific types.
// Sometimes these need to be passed around by client code so they are defined here

typedef void *SurfaceID;
typedef void *WindowID;
typedef void *MenuID;
typedef void *TickerID;
typedef void *Function;
typedef void *IdlerID;

/**
 * Font management.
 */

constexpr const char *localeNameDefault = "en-us";

struct FontParameters {
	const char *faceName;
	XYPOSITION size;
	Scintilla::FontWeight weight;
	bool italic;
	Scintilla::FontQuality extraFontFlag;
	Scintilla::Technology technology;
	Scintilla::CharacterSet characterSet;
	const char *localeName;

	constexpr FontParameters(
		const char *faceName_,
		XYPOSITION size_=10,
		Scintilla::FontWeight weight_= Scintilla::FontWeight::Normal,
		bool italic_=false,
		Scintilla::FontQuality extraFontFlag_= Scintilla::FontQuality::QualityDefault,
		Scintilla::Technology technology_= Scintilla::Technology::Default,
		Scintilla::CharacterSet characterSet_= Scintilla::CharacterSet::Ansi,
		const char *localeName_=localeNameDefault) noexcept :

		faceName(faceName_),
		size(size_),
		weight(weight_),
		italic(italic_),
		extraFontFlag(extraFontFlag_),
		technology(technology_),
		characterSet(characterSet_),
		localeName(localeName_)
	{
	}

};

class Font {
public:
	Font() noexcept = default;
	// Deleted so Font objects can not be copied
	Font(const Font &) = delete;
	Font(Font &&) = delete;
	Font &operator=(const Font &) = delete;
	Font &operator=(Font &&) = delete;
	virtual ~Font() noexcept = default;

	static std::shared_ptr<Font> Allocate(const FontParameters &fp);
};

class IScreenLine {
public:
	virtual std::string_view Text() const = 0;
	virtual size_t Length() const = 0;
	virtual size_t RepresentationCount() const = 0;
	virtual XYPOSITION Width() const = 0;
	virtual XYPOSITION Height() const = 0;
	virtual XYPOSITION TabWidth() const = 0;
	virtual XYPOSITION TabWidthMinimumPixels() const = 0;
	virtual const Font *FontOfPosition(size_t position) const = 0;
	virtual XYPOSITION RepresentationWidth(size_t position) const = 0;
	virtual XYPOSITION TabPositionAfter(XYPOSITION xPosition) const = 0;
};

class IScreenLineLayout {
public:
	virtual ~IScreenLineLayout() noexcept = default;
	virtual size_t PositionFromX(XYPOSITION xDistance, bool charPosition) = 0;
	virtual XYPOSITION XFromPosition(size_t caretPosition) = 0;
	virtual std::vector<Interval> FindRangeIntervals(size_t start, size_t end) = 0;
};

/**
 * Parameters for surfaces.
 */
struct SurfaceMode {
	int codePage = 0;
	bool bidiR2L = false;
	SurfaceMode() = default;
	explicit SurfaceMode(int codePage_, bool bidiR2L_) noexcept : codePage(codePage_), bidiR2L(bidiR2L_) {
	}
};

/**
 * A surface abstracts a place to draw.
 */
class Surface {
public:
	Surface() noexcept = default;
	Surface(const Surface &) = delete;
	Surface(Surface &&) = delete;
	Surface &operator=(const Surface &) = delete;
	Surface &operator=(Surface &&) = delete;
	virtual ~Surface() noexcept = default;
	static std::unique_ptr<Surface> Allocate(Scintilla::Technology technology);

	virtual void Init(WindowID wid)=0;
	virtual void Init(SurfaceID sid, WindowID wid)=0;
	virtual std::unique_ptr<Surface> AllocatePixMap(int width, int height)=0;

	virtual void SetMode(SurfaceMode mode)=0;

	enum class Ends {
		semiCircles = 0x0,
		leftFlat = 0x1,
		leftAngle = 0x2,
		rightFlat = 0x10,
		rightAngle = 0x20,
	};

	virtual void Release() noexcept=0;
	virtual int SupportsFeature(Scintilla::Supports feature) noexcept=0;
	virtual bool Initialised()=0;
	virtual int LogPixelsY()=0;
	virtual int PixelDivisions()=0;
	virtual int DeviceHeightFont(int points)=0;
	virtual void LineDraw(Point start, Point end, Stroke stroke)=0;
	virtual void PolyLine(const Point *pts, size_t npts, Stroke stroke)=0;
	virtual void Polygon(const Point *pts, size_t npts, FillStroke fillStroke)=0;
	virtual void RectangleDraw(PRectangle rc, FillStroke fillStroke)=0;
	virtual void RectangleFrame(PRectangle rc, Stroke stroke)=0;
	virtual void FillRectangle(PRectangle rc, Fill fill)=0;
	virtual void FillRectangleAligned(PRectangle rc, Fill fill)=0;
	virtual void FillRectangle(PRectangle rc, Surface &surfacePattern)=0;
	virtual void RoundedRectangle(PRectangle rc, FillStroke fillStroke)=0;
	virtual void AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke)=0;
	enum class GradientOptions { leftToRight, topToBottom };
	virtual void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options)=0;
	virtual void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) = 0;
	virtual void Ellipse(PRectangle rc, FillStroke fillStroke)=0;
	virtual void Stadium(PRectangle rc, FillStroke fillStroke, Ends ends)=0;
	virtual void Copy(PRectangle rc, Point from, Surface &surfaceSource)=0;

	virtual std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) = 0;

	virtual void DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) = 0;
	virtual void DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) = 0;
	virtual void DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) = 0;
	virtual void MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) = 0;
	virtual XYPOSITION WidthText(const Font *font_, std::string_view text) = 0;

	virtual void DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) = 0;
	virtual void DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) = 0;
	virtual void DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) = 0;
	virtual void MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) = 0;
	virtual XYPOSITION WidthTextUTF8(const Font *font_, std::string_view text) = 0;

	virtual XYPOSITION Ascent(const Font *font_)=0;
	virtual XYPOSITION Descent(const Font *font_)=0;
	virtual XYPOSITION InternalLeading(const Font *font_)=0;
	virtual XYPOSITION Height(const Font *font_)=0;
	virtual XYPOSITION AverageCharWidth(const Font *font_)=0;

	virtual void SetClip(PRectangle rc)=0;
	virtual void PopClip()=0;
	virtual void FlushCachedState()=0;
	virtual void FlushDrawing()=0;
};

/**
 * Class to hide the details of window manipulation.
 * Does not own the window which will normally have a longer life than this object.
 */
class Window {
protected:
	WindowID wid;
public:
	Window() noexcept : wid(nullptr), cursorLast(Cursor::invalid) {
	}
	Window(const Window &source) = delete;
	Window(Window &&) = delete;
	Window &operator=(WindowID wid_) noexcept {
		wid = wid_;
		cursorLast = Cursor::invalid;
		return *this;
	}
	Window &operator=(const Window &) = delete;
	Window &operator=(Window &&) = delete;
	virtual ~Window() noexcept;
	WindowID GetID() const noexcept { return wid; }
	bool Created() const noexcept { return wid != nullptr; }
	void Destroy() noexcept;
	PRectangle GetPosition() const;
	void SetPosition(PRectangle rc);
	void SetPositionRelative(PRectangle rc, const Window *relativeTo);
	PRectangle GetClientPosition() const;
	void Show(bool show=true);
	void InvalidateAll();
	void InvalidateRectangle(PRectangle rc);
	enum class Cursor { invalid, text, arrow, up, wait, horizontal, vertical, reverseArrow, hand };
	void SetCursor(Cursor curs);
	PRectangle GetMonitorRect(Point pt);
private:
	Cursor cursorLast;
};

/**
 * Listbox management.
 */

// ScintillaBase implements IListBoxDelegate to receive ListBoxEvents from a ListBox

struct ListBoxEvent {
	enum class EventType { selectionChange, doubleClick } event;
	ListBoxEvent(EventType event_) noexcept : event(event_) {
	}
};

class IListBoxDelegate {
public:
	virtual void ListNotify(ListBoxEvent *plbe)=0;
};

struct ListOptions {
	std::optional<ColourRGBA> fore;
	std::optional<ColourRGBA> back;
	std::optional<ColourRGBA> foreSelected;
	std::optional<ColourRGBA> backSelected;
	AutoCompleteOption options=AutoCompleteOption::Normal;
};

class ListBox : public Window {
public:
	ListBox() noexcept;
	virtual ~ListBox() noexcept override;
	static std::unique_ptr<ListBox> Allocate();

	virtual void SetFont(const Font *font)=0;
	virtual void Create(Window &parent, int ctrlID, Point location, int lineHeight_, bool unicodeMode_, Scintilla::Technology technology_)=0;
	virtual void SetAverageCharWidth(int width)=0;
	virtual void SetVisibleRows(int rows)=0;
	virtual int GetVisibleRows() const=0;
	virtual PRectangle GetDesiredRect()=0;
	virtual int CaretFromEdge()=0;
	virtual void Clear() noexcept=0;
	virtual void Append(char *s, int type = -1)=0;
	virtual int Length()=0;
	virtual void Select(int n)=0;
	virtual int GetSelection()=0;
	virtual int Find(const char *prefix)=0;
	virtual std::string GetValue(int n)=0;
	virtual void RegisterImage(int type, const char *xpm_data)=0;
	virtual void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) = 0;
	virtual void ClearRegisteredImages()=0;
	virtual void SetDelegate(IListBoxDelegate *lbDelegate)=0;
	virtual void SetList(const char* list, char separator, char typesep)=0;
	virtual void SetOptions(ListOptions options_)=0;
};

/**
 * Menu management.
 */
class Menu {
	MenuID mid;
public:
	Menu() noexcept;
	MenuID GetID() const noexcept { return mid; }
	void CreatePopUp();
	void Destroy() noexcept;
	void Show(Point pt, const Window &w);
};

/**
 * Platform namespace used to retrieve system wide parameters such as double click speed
 * and chrome colour.
 */
namespace Platform {

ColourRGBA Chrome();
ColourRGBA ChromeHighlight();
const char *DefaultFont();
int DefaultFontSize();
unsigned int DoubleClickTime();
constexpr long LongFromTwoShorts(short a,short b) noexcept {
	return (a) | ((b) << 16);
}

}

}

#endif
