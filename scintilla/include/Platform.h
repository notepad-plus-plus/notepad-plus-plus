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

namespace Scintilla {

typedef float XYPOSITION;
typedef double XYACCUMULATOR;

// Underlying the implementation of the platform classes are platform specific types.
// Sometimes these need to be passed around by client code so they are defined here

typedef void *FontID;
typedef void *SurfaceID;
typedef void *WindowID;
typedef void *MenuID;
typedef void *TickerID;
typedef void *Function;
typedef void *IdlerID;

/**
 * A geometric point class.
 * Point is similar to the Win32 POINT and GTK+ GdkPoint types.
 */
class Point {
public:
	XYPOSITION x;
	XYPOSITION y;

	constexpr explicit Point(XYPOSITION x_=0, XYPOSITION y_=0) noexcept : x(x_), y(y_) {
	}

	static Point FromInts(int x_, int y_) noexcept {
		return Point(static_cast<XYPOSITION>(x_), static_cast<XYPOSITION>(y_));
	}

	bool operator!=(Point other) const noexcept {
		return (x != other.x) || (y != other.y);
	}

	Point operator+(Point other) const noexcept {
		return Point(x + other.x, y + other.y);
	}

	Point operator-(Point other) const noexcept {
		return Point(x - other.x, y - other.y);
	}

	// Other automatically defined methods (assignment, copy constructor, destructor) are fine
};

/**
 * A geometric rectangle class.
 * PRectangle is similar to Win32 RECT.
 * PRectangles contain their top and left sides, but not their right and bottom sides.
 */
class PRectangle {
public:
	XYPOSITION left;
	XYPOSITION top;
	XYPOSITION right;
	XYPOSITION bottom;

	constexpr explicit PRectangle(XYPOSITION left_=0, XYPOSITION top_=0, XYPOSITION right_=0, XYPOSITION bottom_ = 0) noexcept :
		left(left_), top(top_), right(right_), bottom(bottom_) {
	}

	static PRectangle FromInts(int left_, int top_, int right_, int bottom_) noexcept {
		return PRectangle(static_cast<XYPOSITION>(left_), static_cast<XYPOSITION>(top_),
			static_cast<XYPOSITION>(right_), static_cast<XYPOSITION>(bottom_));
	}

	// Other automatically defined methods (assignment, copy constructor, destructor) are fine

	bool operator==(const PRectangle &rc) const noexcept {
		return (rc.left == left) && (rc.right == right) &&
			(rc.top == top) && (rc.bottom == bottom);
	}
	bool Contains(Point pt) const noexcept {
		return (pt.x >= left) && (pt.x <= right) &&
			(pt.y >= top) && (pt.y <= bottom);
	}
	bool ContainsWholePixel(Point pt) const noexcept {
		// Does the rectangle contain all of the pixel to left/below the point
		return (pt.x >= left) && ((pt.x+1) <= right) &&
			(pt.y >= top) && ((pt.y+1) <= bottom);
	}
	bool Contains(PRectangle rc) const noexcept {
		return (rc.left >= left) && (rc.right <= right) &&
			(rc.top >= top) && (rc.bottom <= bottom);
	}
	bool Intersects(PRectangle other) const noexcept {
		return (right > other.left) && (left < other.right) &&
			(bottom > other.top) && (top < other.bottom);
	}
	void Move(XYPOSITION xDelta, XYPOSITION yDelta) noexcept {
		left += xDelta;
		top += yDelta;
		right += xDelta;
		bottom += yDelta;
	}
	XYPOSITION Width() const noexcept { return right - left; }
	XYPOSITION Height() const noexcept { return bottom - top; }
	bool Empty() const noexcept {
		return (Height() <= 0) || (Width() <= 0);
	}
};

/**
 * Holds an RGB colour with 8 bits for each component.
 */
constexpr const float componentMaximum = 255.0f;
class ColourDesired {
	int co;
public:
	explicit ColourDesired(int co_=0) noexcept : co(co_) {
	}

	ColourDesired(unsigned int red, unsigned int green, unsigned int blue) noexcept :
		co(red | (green << 8) | (blue << 16)) {
	}

	bool operator==(const ColourDesired &other) const noexcept {
		return co == other.co;
	}

	int AsInteger() const noexcept {
		return co;
	}

	// Red, green and blue values as bytes 0..255
	unsigned char GetRed() const noexcept {
		return co & 0xff;
	}
	unsigned char GetGreen() const noexcept {
		return (co >> 8) & 0xff;
	}
	unsigned char GetBlue() const noexcept {
		return (co >> 16) & 0xff;
	}

	// Red, green and blue values as float 0..1.0
	float GetRedComponent() const noexcept {
		return GetRed() / componentMaximum;
	}
	float GetGreenComponent() const noexcept {
		return GetGreen() / componentMaximum;
	}
	float GetBlueComponent() const noexcept {
		return GetBlue() / componentMaximum;
	}
};

/**
* Holds an RGBA colour.
*/
class ColourAlpha : public ColourDesired {
public:
	explicit ColourAlpha(int co_ = 0) noexcept : ColourDesired(co_) {
	}

	ColourAlpha(unsigned int red, unsigned int green, unsigned int blue) noexcept :
		ColourDesired(red | (green << 8) | (blue << 16)) {
	}

	ColourAlpha(unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha) noexcept :
		ColourDesired(red | (green << 8) | (blue << 16) | (alpha << 24)) {
	}

	ColourAlpha(ColourDesired cd, unsigned int alpha) noexcept :
		ColourDesired(cd.AsInteger() | (alpha << 24)) {
	}

	ColourDesired GetColour() const noexcept {
		return ColourDesired(AsInteger() & 0xffffff);
	}

	unsigned char GetAlpha() const noexcept {
		return (AsInteger() >> 24) & 0xff;
	}

	float GetAlphaComponent() const noexcept {
		return GetAlpha() / componentMaximum;
	}

	ColourAlpha MixedWith(ColourAlpha other) const noexcept {
		const unsigned int red = (GetRed() + other.GetRed()) / 2;
		const unsigned int green = (GetGreen() + other.GetGreen()) / 2;
		const unsigned int blue = (GetBlue() + other.GetBlue()) / 2;
		const unsigned int alpha = (GetAlpha() + other.GetAlpha()) / 2;
		return ColourAlpha(red, green, blue, alpha);
	}
};

/**
* Holds an element of a gradient with an RGBA colour and a relative position.
*/
class ColourStop {
public:
	float position;
	ColourAlpha colour;
	ColourStop(float position_, ColourAlpha colour_) noexcept :
		position(position_), colour(colour_) {
	}
};

/**
 * Font management.
 */

struct FontParameters {
	const char *faceName;
	float size;
	int weight;
	bool italic;
	int extraFontFlag;
	int technology;
	int characterSet;

	FontParameters(
		const char *faceName_,
		float size_=10,
		int weight_=400,
		bool italic_=false,
		int extraFontFlag_=0,
		int technology_=0,
		int characterSet_=0) noexcept :

		faceName(faceName_),
		size(size_),
		weight(weight_),
		italic(italic_),
		extraFontFlag(extraFontFlag_),
		technology(technology_),
		characterSet(characterSet_)
	{
	}

};

class Font {
protected:
	FontID fid;
public:
	Font() noexcept;
	// Deleted so Font objects can not be copied
	Font(const Font &) = delete;
	Font(Font &&) = delete;
	Font &operator=(const Font &) = delete;
	Font &operator=(Font &&) = delete;
	virtual ~Font();

	virtual void Create(const FontParameters &fp);
	virtual void Release();

	FontID GetID() const noexcept { return fid; }
	// Alias another font - caller guarantees not to Release
	void SetID(FontID fid_) noexcept { fid = fid_; }
	friend class Surface;
	friend class SurfaceImpl;
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

struct Interval {
	XYPOSITION left;
	XYPOSITION right;
};

class IScreenLineLayout {
public:
	virtual ~IScreenLineLayout() = default;
	virtual size_t PositionFromX(XYPOSITION xDistance, bool charPosition) = 0;
	virtual XYPOSITION XFromPosition(size_t caretPosition) = 0;
	virtual std::vector<Interval> FindRangeIntervals(size_t start, size_t end) = 0;
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
	virtual ~Surface() {}
	static Surface *Allocate(int technology);

	virtual void Init(WindowID wid)=0;
	virtual void Init(SurfaceID sid, WindowID wid)=0;
	virtual void InitPixMap(int width, int height, Surface *surface_, WindowID wid)=0;

	virtual void Release()=0;
	virtual bool Initialised()=0;
	virtual void PenColour(ColourDesired fore)=0;
	virtual int LogPixelsY()=0;
	virtual int DeviceHeightFont(int points)=0;
	virtual void MoveTo(int x_, int y_)=0;
	virtual void LineTo(int x_, int y_)=0;
	virtual void Polygon(Point *pts, size_t npts, ColourDesired fore, ColourDesired back)=0;
	virtual void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back)=0;
	virtual void FillRectangle(PRectangle rc, ColourDesired back)=0;
	virtual void FillRectangle(PRectangle rc, Surface &surfacePattern)=0;
	virtual void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back)=0;
	virtual void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int flags)=0;
	enum class GradientOptions { leftToRight, topToBottom };
	virtual void GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options)=0;
	virtual void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) = 0;
	virtual void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back)=0;
	virtual void Copy(PRectangle rc, Point from, Surface &surfaceSource)=0;

	virtual std::unique_ptr<IScreenLineLayout> Layout(const IScreenLine *screenLine) = 0;

	virtual void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back) = 0;
	virtual void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore, ColourDesired back) = 0;
	virtual void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, std::string_view text, ColourDesired fore) = 0;
	virtual void MeasureWidths(Font &font_, std::string_view text, XYPOSITION *positions) = 0;
	virtual XYPOSITION WidthText(Font &font_, std::string_view text) = 0;
	virtual XYPOSITION Ascent(Font &font_)=0;
	virtual XYPOSITION Descent(Font &font_)=0;
	virtual XYPOSITION InternalLeading(Font &font_)=0;
	virtual XYPOSITION Height(Font &font_)=0;
	virtual XYPOSITION AverageCharWidth(Font &font_)=0;

	virtual void SetClip(PRectangle rc)=0;
	virtual void FlushCachedState()=0;

	virtual void SetUnicodeMode(bool unicodeMode_)=0;
	virtual void SetDBCSMode(int codePage)=0;
	virtual void SetBidiR2L(bool bidiR2L_)=0;
};

/**
 * Class to hide the details of window manipulation.
 * Does not own the window which will normally have a longer life than this object.
 */
class Window {
protected:
	WindowID wid;
public:
	Window() noexcept : wid(nullptr), cursorLast(cursorInvalid) {
	}
	Window(const Window &source) = delete;
	Window(Window &&) = delete;
	Window &operator=(WindowID wid_) noexcept {
		wid = wid_;
		cursorLast = cursorInvalid;
		return *this;
	}
	Window &operator=(const Window &) = delete;
	Window &operator=(Window &&) = delete;
	virtual ~Window();
	WindowID GetID() const noexcept { return wid; }
	bool Created() const noexcept { return wid != nullptr; }
	void Destroy();
	PRectangle GetPosition() const;
	void SetPosition(PRectangle rc);
	void SetPositionRelative(PRectangle rc, const Window *relativeTo);
	PRectangle GetClientPosition() const;
	void Show(bool show=true);
	void InvalidateAll();
	void InvalidateRectangle(PRectangle rc);
	virtual void SetFont(Font &font);
	enum Cursor { cursorInvalid, cursorText, cursorArrow, cursorUp, cursorWait, cursorHoriz, cursorVert, cursorReverseArrow, cursorHand };
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

class ListBox : public Window {
public:
	ListBox() noexcept;
	~ListBox() override;
	static ListBox *Allocate();

	void SetFont(Font &font) override =0;
	virtual void Create(Window &parent, int ctrlID, Point location, int lineHeight_, bool unicodeMode_, int technology_)=0;
	virtual void SetAverageCharWidth(int width)=0;
	virtual void SetVisibleRows(int rows)=0;
	virtual int GetVisibleRows() const=0;
	virtual PRectangle GetDesiredRect()=0;
	virtual int CaretFromEdge()=0;
	virtual void Clear()=0;
	virtual void Append(char *s, int type = -1)=0;
	virtual int Length()=0;
	virtual void Select(int n)=0;
	virtual int GetSelection()=0;
	virtual int Find(const char *prefix)=0;
	virtual void GetValue(int n, char *value, int len)=0;
	virtual void RegisterImage(int type, const char *xpm_data)=0;
	virtual void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) = 0;
	virtual void ClearRegisteredImages()=0;
	virtual void SetDelegate(IListBoxDelegate *lbDelegate)=0;
	virtual void SetList(const char* list, char separator, char typesep)=0;
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
	void Destroy();
	void Show(Point pt, Window &w);
};

/**
 * Dynamic Library (DLL/SO/...) loading
 */
class DynamicLibrary {
public:
	virtual ~DynamicLibrary() = default;

	/// @return Pointer to function "name", or NULL on failure.
	virtual Function FindFunction(const char *name) = 0;

	/// @return true if the library was loaded successfully.
	virtual bool IsValid() = 0;

	/// @return An instance of a DynamicLibrary subclass with "modulePath" loaded.
	static DynamicLibrary *Load(const char *modulePath);
};

#if defined(__clang__)
# if __has_feature(attribute_analyzer_noreturn)
#  define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
# else
#  define CLANG_ANALYZER_NORETURN
# endif
#else
# define CLANG_ANALYZER_NORETURN
#endif

/**
 * Platform class used to retrieve system wide parameters such as double click speed
 * and chrome colour. Not a creatable object, more of a module with several functions.
 */
class Platform {
public:
	Platform() = default;
	Platform(const Platform &) = delete;
	Platform(Platform &&) = delete;
	Platform &operator=(const Platform &) = delete;
	Platform &operator=(Platform &&) = delete;
	~Platform() = default;
	static ColourDesired Chrome();
	static ColourDesired ChromeHighlight();
	static const char *DefaultFont();
	static int DefaultFontSize();
	static unsigned int DoubleClickTime();
	static void DebugDisplay(const char *s);
	static constexpr long LongFromTwoShorts(short a,short b) noexcept {
		return (a) | ((b) << 16);
	}

	static void DebugPrintf(const char *format, ...);
	static bool ShowAssertionPopUps(bool assertionPopUps_);
	static void Assert(const char *c, const char *file, int line) CLANG_ANALYZER_NORETURN;
};

#ifdef  NDEBUG
#define PLATFORM_ASSERT(c) ((void)0)
#else
#define PLATFORM_ASSERT(c) ((c) ? (void)(0) : Scintilla::Platform::Assert(#c, __FILE__, __LINE__))
#endif

}

#endif
