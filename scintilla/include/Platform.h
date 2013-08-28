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

#if defined(FOX)
#undef PLAT_FOX
#define PLAT_FOX 1

#elif defined(__WX__)
#undef PLAT_WX
#define PLAT_WX  1

#elif defined(CURSES)
#undef PLAT_CURSES
#define PLAT_CURSES 1

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

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

typedef float XYPOSITION;
typedef double XYACCUMULATOR;
//#define XYPOSITION int

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
 * Point is exactly the same as the Win32 POINT and GTK+ GdkPoint so can be used interchangeably.
 */
class Point {
public:
	XYPOSITION x;
	XYPOSITION y;

	explicit Point(XYPOSITION x_=0, XYPOSITION y_=0) : x(x_), y(y_) {
	}

	// Other automatically defined methods (assignment, copy constructor, destructor) are fine

	static Point FromLong(long lpoint);
};

/**
 * A geometric rectangle class.
 * PRectangle is exactly the same as the Win32 RECT so can be used interchangeably.
 * PRectangles contain their top and left sides, but not their right and bottom sides.
 */
class PRectangle {
public:
	XYPOSITION left;
	XYPOSITION top;
	XYPOSITION right;
	XYPOSITION bottom;

	PRectangle(XYPOSITION left_=0, XYPOSITION top_=0, XYPOSITION right_=0, XYPOSITION bottom_ = 0) :
		left(left_), top(top_), right(right_), bottom(bottom_) {
	}

	// Other automatically defined methods (assignment, copy constructor, destructor) are fine

	bool operator==(PRectangle &rc) const {
		return (rc.left == left) && (rc.right == right) &&
			(rc.top == top) && (rc.bottom == bottom);
	}
	bool Contains(Point pt) const {
		return (pt.x >= left) && (pt.x <= right) &&
			(pt.y >= top) && (pt.y <= bottom);
	}
	bool Contains(PRectangle rc) const {
		return (rc.left >= left) && (rc.right <= right) &&
			(rc.top >= top) && (rc.bottom <= bottom);
	}
	bool Intersects(PRectangle other) const {
		return (right > other.left) && (left < other.right) &&
			(bottom > other.top) && (top < other.bottom);
	}
	void Move(XYPOSITION xDelta, XYPOSITION yDelta) {
		left += xDelta;
		top += yDelta;
		right += xDelta;
		bottom += yDelta;
	}
	XYPOSITION Width() const { return right - left; }
	XYPOSITION Height() const { return bottom - top; }
	bool Empty() const {
		return (Height() <= 0) || (Width() <= 0);
	}
};

/**
 * Holds a desired RGB colour.
 */
class ColourDesired {
	long co;
public:
	ColourDesired(long lcol=0) {
		co = lcol;
	}

	ColourDesired(unsigned int red, unsigned int green, unsigned int blue) {
		Set(red, green, blue);
	}

	bool operator==(const ColourDesired &other) const {
		return co == other.co;
	}

	void Set(long lcol) {
		co = lcol;
	}

	void Set(unsigned int red, unsigned int green, unsigned int blue) {
		co = red | (green << 8) | (blue << 16);
	}

	static inline unsigned int ValueOfHex(const char ch) {
		if (ch >= '0' && ch <= '9')
			return ch - '0';
		else if (ch >= 'A' && ch <= 'F')
			return ch - 'A' + 10;
		else if (ch >= 'a' && ch <= 'f')
			return ch - 'a' + 10;
		else
			return 0;
	}

	void Set(const char *val) {
		if (*val == '#') {
			val++;
		}
		unsigned int r = ValueOfHex(val[0]) * 16 + ValueOfHex(val[1]);
		unsigned int g = ValueOfHex(val[2]) * 16 + ValueOfHex(val[3]);
		unsigned int b = ValueOfHex(val[4]) * 16 + ValueOfHex(val[5]);
		Set(r, g, b);
	}

	long AsLong() const {
		return co;
	}

	unsigned int GetRed() const {
		return co & 0xff;
	}

	unsigned int GetGreen() const {
		return (co >> 8) & 0xff;
	}

	unsigned int GetBlue() const {
		return (co >> 16) & 0xff;
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
		int characterSet_=0) :

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
#if PLAT_WX
	int ascent;
#endif
	// Private so Font objects can not be copied
	Font(const Font &);
	Font &operator=(const Font &);
public:
	Font();
	virtual ~Font();

	virtual void Create(const FontParameters &fp);
	virtual void Release();

	FontID GetID() { return fid; }
	// Alias another font - caller guarantees not to Release
	void SetID(FontID fid_) { fid = fid_; }
#if PLAT_WX
	void SetAscent(int ascent_) { ascent = ascent_; }
#endif
	friend class Surface;
	friend class SurfaceImpl;
};

/**
 * A surface abstracts a place to draw.
 */
class Surface {
private:
	// Private so Surface objects can not be copied
	Surface(const Surface &) {}
	Surface &operator=(const Surface &) { return *this; }
public:
	Surface() {}
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
	virtual void Polygon(Point *pts, int npts, ColourDesired fore, ColourDesired back)=0;
	virtual void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back)=0;
	virtual void FillRectangle(PRectangle rc, ColourDesired back)=0;
	virtual void FillRectangle(PRectangle rc, Surface &surfacePattern)=0;
	virtual void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back)=0;
	virtual void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int flags)=0;
	virtual void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) = 0;
	virtual void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back)=0;
	virtual void Copy(PRectangle rc, Point from, Surface &surfaceSource)=0;

	virtual void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back)=0;
	virtual void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back)=0;
	virtual void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore)=0;
	virtual void MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions)=0;
	virtual XYPOSITION WidthText(Font &font_, const char *s, int len)=0;
	virtual XYPOSITION WidthChar(Font &font_, char ch)=0;
	virtual XYPOSITION Ascent(Font &font_)=0;
	virtual XYPOSITION Descent(Font &font_)=0;
	virtual XYPOSITION InternalLeading(Font &font_)=0;
	virtual XYPOSITION ExternalLeading(Font &font_)=0;
	virtual XYPOSITION Height(Font &font_)=0;
	virtual XYPOSITION AverageCharWidth(Font &font_)=0;

	virtual void SetClip(PRectangle rc)=0;
	virtual void FlushCachedState()=0;

	virtual void SetUnicodeMode(bool unicodeMode_)=0;
	virtual void SetDBCSMode(int codePage)=0;
};

/**
 * A simple callback action passing one piece of untyped user data.
 */
typedef void (*CallBackAction)(void*);

/**
 * Class to hide the details of window manipulation.
 * Does not own the window which will normally have a longer life than this object.
 */
class Window {
protected:
	WindowID wid;
#if PLAT_MACOSX
	void *windowRef;
	void *control;
#endif
public:
	Window() : wid(0), cursorLast(cursorInvalid) {
#if PLAT_MACOSX
	  windowRef = 0;
	  control = 0;
#endif
	}
	Window(const Window &source) : wid(source.wid), cursorLast(cursorInvalid) {
#if PLAT_MACOSX
	  windowRef = 0;
	  control = 0;
#endif
	}
	virtual ~Window();
	Window &operator=(WindowID wid_) {
		wid = wid_;
		return *this;
	}
	WindowID GetID() const { return wid; }
	bool Created() const { return wid != 0; }
	void Destroy();
	bool HasFocus();
	PRectangle GetPosition();
	void SetPosition(PRectangle rc);
	void SetPositionRelative(PRectangle rc, Window relativeTo);
	PRectangle GetClientPosition();
	void Show(bool show=true);
	void InvalidateAll();
	void InvalidateRectangle(PRectangle rc);
	virtual void SetFont(Font &font);
	enum Cursor { cursorInvalid, cursorText, cursorArrow, cursorUp, cursorWait, cursorHoriz, cursorVert, cursorReverseArrow, cursorHand };
	void SetCursor(Cursor curs);
	void SetTitle(const char *s);
	PRectangle GetMonitorRect(Point pt);
#if PLAT_MACOSX
	void SetWindow(void *ref) { windowRef = ref; }
	void SetControl(void *_control) { control = _control; }
#endif
private:
	Cursor cursorLast;
};

/**
 * Listbox management.
 */

class ListBox : public Window {
public:
	ListBox();
	virtual ~ListBox();
	static ListBox *Allocate();

	virtual void SetFont(Font &font)=0;
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
	virtual void SetDoubleClickAction(CallBackAction, void *)=0;
	virtual void SetList(const char* list, char separator, char typesep)=0;
};

/**
 * Menu management.
 */
class Menu {
	MenuID mid;
public:
	Menu();
	MenuID GetID() { return mid; }
	void CreatePopUp();
	void Destroy();
	void Show(Point pt, Window &w);
};

class ElapsedTime {
	long bigBit;
	long littleBit;
public:
	ElapsedTime();
	double Duration(bool reset=false);
};

/**
 * Dynamic Library (DLL/SO/...) loading
 */
class DynamicLibrary {
public:
	virtual ~DynamicLibrary() {}

	/// @return Pointer to function "name", or NULL on failure.
	virtual Function FindFunction(const char *name) = 0;

	/// @return true if the library was loaded successfully.
	virtual bool IsValid() = 0;

	/// @return An instance of a DynamicLibrary subclass with "modulePath" loaded.
	static DynamicLibrary *Load(const char *modulePath);
};

/**
 * Platform class used to retrieve system wide parameters such as double click speed
 * and chrome colour. Not a creatable object, more of a module with several functions.
 */
class Platform {
	// Private so Platform objects can not be copied
	Platform(const Platform &) {}
	Platform &operator=(const Platform &) { return *this; }
public:
	// Should be private because no new Platforms are ever created
	// but gcc warns about this
	Platform() {}
	~Platform() {}
	static ColourDesired Chrome();
	static ColourDesired ChromeHighlight();
	static const char *DefaultFont();
	static int DefaultFontSize();
	static unsigned int DoubleClickTime();
	static bool MouseButtonBounce();
	static void DebugDisplay(const char *s);
	static bool IsKeyDown(int key);
	static long SendScintilla(
		WindowID w, unsigned int msg, unsigned long wParam=0, long lParam=0);
	static long SendScintillaPointer(
		WindowID w, unsigned int msg, unsigned long wParam=0, void *lParam=0);
	static bool IsDBCSLeadByte(int codePage, char ch);
	static int DBCSCharLength(int codePage, const char *s);
	static int DBCSCharMaxLength();

	// These are utility functions not really tied to a platform
	static int Minimum(int a, int b);
	static int Maximum(int a, int b);
	// Next three assume 16 bit shorts and 32 bit longs
	static long LongFromTwoShorts(short a,short b) {
		return (a) | ((b) << 16);
	}
	static short HighShortFromLong(long x) {
		return static_cast<short>(x >> 16);
	}
	static short LowShortFromLong(long x) {
		return static_cast<short>(x & 0xffff);
	}
	static void DebugPrintf(const char *format, ...);
	static bool ShowAssertionPopUps(bool assertionPopUps_);
	static void Assert(const char *c, const char *file, int line);
	static int Clamp(int val, int minVal, int maxVal);
};

#ifdef  NDEBUG
#define PLATFORM_ASSERT(c) ((void)0)
#else
#ifdef SCI_NAMESPACE
#define PLATFORM_ASSERT(c) ((c) ? (void)(0) : Scintilla::Platform::Assert(#c, __FILE__, __LINE__))
#else
#define PLATFORM_ASSERT(c) ((c) ? (void)(0) : Platform::Assert(#c, __FILE__, __LINE__))
#endif
#endif

#ifdef SCI_NAMESPACE
}
#endif

// Shut up annoying Visual C++ warnings:
#ifdef _MSC_VER
#pragma warning(disable: 4244 4309 4514 4710)
#endif

#if defined(__GNUC__) && defined(SCINTILLA_QT)
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wchar-subscripts"
#endif

#endif
