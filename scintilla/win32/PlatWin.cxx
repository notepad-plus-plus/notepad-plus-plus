// Scintilla source code edit control
/** @file PlatWin.cxx
 ** Implementation of platform facilities on Windows.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0500
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <windowsx.h>

#include "Platform.h"
#include "PlatformRes.h"
#include "UniConversion.h"
#include "XPM.h"
#include "FontQuality.h"

// We want to use multi monitor functions, but via LoadLibrary etc
// Luckily microsoft has done the heavy lifting for us, so we'll just use their stub functions!
#if (defined(_MSC_VER) && (MSC_VER > 1200)) || defined(__BORLANDC__)
#define COMPILE_MULTIMON_STUBS
#include "MultiMon.h"
#endif

#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)
#endif

// Take care of 32/64 bit pointers
#ifdef GetWindowLongPtr
static void *PointerFromWindow(HWND hWnd) {
	return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, 0));
}
static void SetWindowPointer(HWND hWnd, void *ptr) {
	::SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(ptr));
}
#else
static void *PointerFromWindow(HWND hWnd) {
	return reinterpret_cast<void *>(::GetWindowLong(hWnd, 0));
}
static void SetWindowPointer(HWND hWnd, void *ptr) {
	::SetWindowLong(hWnd, 0, reinterpret_cast<LONG>(ptr));
}

#ifndef GWLP_USERDATA
#define GWLP_USERDATA GWL_USERDATA
#endif

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC GWL_WNDPROC
#endif

#ifndef LONG_PTR
#define LONG_PTR LONG
#endif

static LONG_PTR SetWindowLongPtr(HWND hWnd, int nIndex, LONG_PTR dwNewLong) {
	return ::SetWindowLong(hWnd, nIndex, dwNewLong);
}

static LONG_PTR GetWindowLongPtr(HWND hWnd, int nIndex) {
	return ::GetWindowLong(hWnd, nIndex);
}
#endif

typedef BOOL (WINAPI *AlphaBlendSig)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);

static CRITICAL_SECTION crPlatformLock;
static HINSTANCE hinstPlatformRes = 0;
static bool onNT = false;
static HMODULE hDLLImage = 0;
static AlphaBlendSig AlphaBlendFn = 0;


bool IsNT() {
	return onNT;
}

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

Point Point::FromLong(long lpoint) {
	return Point(static_cast<short>(LOWORD(lpoint)), static_cast<short>(HIWORD(lpoint)));
}

static RECT RectFromPRectangle(PRectangle prc) {
	RECT rc = {prc.left, prc.top, prc.right, prc.bottom};
	return rc;
}

Palette::Palette() {
	used = 0;
	allowRealization = false;
	hpal = 0;
	size = 100;
	entries = new ColourPair[size];
}

Palette::~Palette() {
	Release();
	delete []entries;
	entries = 0;
}

void Palette::Release() {
	used = 0;
	if (hpal)
		::DeleteObject(hpal);
	hpal = 0;
	delete []entries;
	size = 100;
	entries = new ColourPair[size];
}

/**
 * This method either adds a colour to the list of wanted colours (want==true)
 * or retrieves the allocated colour back to the ColourPair.
 * This is one method to make it easier to keep the code for wanting and retrieving in sync.
 */
void Palette::WantFind(ColourPair &cp, bool want) {
	if (want) {
		for (int i=0; i < used; i++) {
			if (entries[i].desired == cp.desired)
				return;
		}

		if (used >= size) {
			int sizeNew = size * 2;
			ColourPair *entriesNew = new ColourPair[sizeNew];
			for (int j=0; j<size; j++) {
				entriesNew[j] = entries[j];
			}
			delete []entries;
			entries = entriesNew;
			size = sizeNew;
		}

		entries[used].desired = cp.desired;
		entries[used].allocated.Set(cp.desired.AsLong());
		used++;
	} else {
		for (int i=0; i < used; i++) {
			if (entries[i].desired == cp.desired) {
				cp.allocated = entries[i].allocated;
				return;
			}
		}
		cp.allocated.Set(cp.desired.AsLong());
	}
}

void Palette::Allocate(Window &) {
	if (hpal)
		::DeleteObject(hpal);
	hpal = 0;

	if (allowRealization) {
		char *pal = new char[sizeof(LOGPALETTE) + (used-1) * sizeof(PALETTEENTRY)];
		LOGPALETTE *logpal = reinterpret_cast<LOGPALETTE *>(pal);
		logpal->palVersion = 0x300;
		logpal->palNumEntries = static_cast<WORD>(used);
		for (int iPal=0;iPal<used;iPal++) {
			ColourDesired desired = entries[iPal].desired;
			logpal->palPalEntry[iPal].peRed   = static_cast<BYTE>(desired.GetRed());
			logpal->palPalEntry[iPal].peGreen = static_cast<BYTE>(desired.GetGreen());
			logpal->palPalEntry[iPal].peBlue  = static_cast<BYTE>(desired.GetBlue());
			entries[iPal].allocated.Set(
				PALETTERGB(desired.GetRed(), desired.GetGreen(), desired.GetBlue()));
			// PC_NOCOLLAPSE means exact colours allocated even when in background this means other windows
			// are less likely to get their colours and also flashes more when switching windows
			logpal->palPalEntry[iPal].peFlags = PC_NOCOLLAPSE;
			// 0 allows approximate colours when in background, yielding moe colours to other windows
			//logpal->palPalEntry[iPal].peFlags = 0;
		}
		hpal = ::CreatePalette(logpal);
		delete []pal;
	}
}

#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif

static BYTE Win32MapFontQuality(int extraFontFlag) {
	switch (extraFontFlag & SC_EFF_QUALITY_MASK) {

		case SC_EFF_QUALITY_NON_ANTIALIASED:
			return NONANTIALIASED_QUALITY;

		case SC_EFF_QUALITY_ANTIALIASED:
			return ANTIALIASED_QUALITY;

		case SC_EFF_QUALITY_LCD_OPTIMIZED:
			return CLEARTYPE_QUALITY;

		default:
			return SC_EFF_QUALITY_DEFAULT;
	}
}

static void SetLogFont(LOGFONTA &lf, const char *faceName, int characterSet, int size, bool bold, bool italic, int extraFontFlag) {
	memset(&lf, 0, sizeof(lf));
	// The negative is to allow for leading
	lf.lfHeight = -(abs(size));
	lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = static_cast<BYTE>(italic ? 1 : 0);
	lf.lfCharSet = static_cast<BYTE>(characterSet);
	lf.lfQuality = Win32MapFontQuality(extraFontFlag);
	strncpy(lf.lfFaceName, faceName, sizeof(lf.lfFaceName));
}

/**
 * Create a hash from the parameters for a font to allow easy checking for identity.
 * If one font is the same as another, its hash will be the same, but if the hash is the
 * same then they may still be different.
 */
static int HashFont(const char *faceName, int characterSet, int size, bool bold, bool italic, int extraFontFlag) {
	return
		size ^
		(characterSet << 10) ^
		((extraFontFlag & SC_EFF_QUALITY_MASK) << 9) ^
		(bold ? 0x10000000 : 0) ^
		(italic ? 0x20000000 : 0) ^
		faceName[0];
}

class FontCached : Font {
	FontCached *next;
	int usage;
	LOGFONTA lf;
	int hash;
	FontCached(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_, int extraFontFlag_);
	~FontCached() {}
	bool SameAs(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_, int extraFontFlag_);
	virtual void Release();

	static FontCached *first;
public:
	static FontID FindOrCreate(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_, int extraFontFlag_);
	static void ReleaseId(FontID fid_);
};

FontCached *FontCached::first = 0;

FontCached::FontCached(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_, int extraFontFlag_) :
	next(0), usage(0), hash(0) {
	SetLogFont(lf, faceName_, characterSet_, size_, bold_, italic_, extraFontFlag_);
	hash = HashFont(faceName_, characterSet_, size_, bold_, italic_, extraFontFlag_);
	fid = ::CreateFontIndirectA(&lf);
	usage = 1;
}

bool FontCached::SameAs(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_, int extraFontFlag_) {
	return
		(lf.lfHeight == -(abs(size_))) &&
		(lf.lfWeight == (bold_ ? FW_BOLD : FW_NORMAL)) &&
		(lf.lfItalic == static_cast<BYTE>(italic_ ? 1 : 0)) &&
		(lf.lfCharSet == characterSet_) &&
		(lf.lfQuality == Win32MapFontQuality(extraFontFlag_)) &&
		0 == strcmp(lf.lfFaceName,faceName_);
}

void FontCached::Release() {
	if (fid)
		::DeleteObject(fid);
	fid = 0;
}

FontID FontCached::FindOrCreate(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_, int extraFontFlag_) {
	FontID ret = 0;
	::EnterCriticalSection(&crPlatformLock);
	int hashFind = HashFont(faceName_, characterSet_, size_, bold_, italic_, extraFontFlag_);
	for (FontCached *cur=first; cur; cur=cur->next) {
		if ((cur->hash == hashFind) &&
			cur->SameAs(faceName_, characterSet_, size_, bold_, italic_, extraFontFlag_)) {
			cur->usage++;
			ret = cur->fid;
		}
	}
	if (ret == 0) {
		FontCached *fc = new FontCached(faceName_, characterSet_, size_, bold_, italic_, extraFontFlag_);
		if (fc) {
			fc->next = first;
			first = fc;
			ret = fc->fid;
		}
	}
	::LeaveCriticalSection(&crPlatformLock);
	return ret;
}

void FontCached::ReleaseId(FontID fid_) {
	::EnterCriticalSection(&crPlatformLock);
	FontCached **pcur=&first;
	for (FontCached *cur=first; cur; cur=cur->next) {
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
		pcur=&cur->next;
	}
	::LeaveCriticalSection(&crPlatformLock);
}

Font::Font() {
	fid = 0;
}

Font::~Font() {
}

#define FONTS_CACHED

void Font::Create(const char *faceName, int characterSet, int size,
	bool bold, bool italic, int extraFontFlag) {
	Release();
#ifndef FONTS_CACHED
	LOGFONT lf;
	SetLogFont(lf, faceName, characterSet, size, bold, italic, extraFontFlag);
	fid = ::CreateFontIndirect(&lf);
#else
	if (faceName)
		fid = FontCached::FindOrCreate(faceName, characterSet, size, bold, italic, extraFontFlag);
#endif
}

void Font::Release() {
#ifndef FONTS_CACHED
	if (fid)
		::DeleteObject(fid);
#else
	if (fid)
		FontCached::ReleaseId(fid);
#endif
	fid = 0;
}

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class SurfaceImpl : public Surface {
	bool unicodeMode;
	HDC hdc;
	bool hdcOwned;
	HPEN pen;
	HPEN penOld;
	HBRUSH brush;
	HBRUSH brushOld;
	HFONT font;
	HFONT fontOld;
	HBITMAP bitmap;
	HBITMAP bitmapOld;
	HPALETTE paletteOld;
	int maxWidthMeasure;
	int maxLenText;

	int codePage;
	// If 9x OS and current code page is same as ANSI code page.
	bool win9xACPSame;

	void BrushColor(ColourAllocated back);
	void SetFont(Font &font_);

	// Private so SurfaceImpl objects can not be copied
	SurfaceImpl(const SurfaceImpl &);
	SurfaceImpl &operator=(const SurfaceImpl &);
public:
	SurfaceImpl();
	virtual ~SurfaceImpl();

	void Init(WindowID wid);
	void Init(SurfaceID sid, WindowID wid);
	void InitPixMap(int width, int height, Surface *surface_, WindowID wid);

	void Release();
	bool Initialised();
	void PenColour(ColourAllocated fore);
	int LogPixelsY();
	int DeviceHeightFont(int points);
	void MoveTo(int x_, int y_);
	void LineTo(int x_, int y_);
	void Polygon(Point *pts, int npts, ColourAllocated fore, ColourAllocated back);
	void RectangleDraw(PRectangle rc, ColourAllocated fore, ColourAllocated back);
	void FillRectangle(PRectangle rc, ColourAllocated back);
	void FillRectangle(PRectangle rc, Surface &surfacePattern);
	void RoundedRectangle(PRectangle rc, ColourAllocated fore, ColourAllocated back);
	void AlphaRectangle(PRectangle rc, int cornerSize, ColourAllocated fill, int alphaFill,
		ColourAllocated outline, int alphaOutline, int flags);
	void Ellipse(PRectangle rc, ColourAllocated fore, ColourAllocated back);
	void Copy(PRectangle rc, Point from, Surface &surfaceSource);

	void DrawTextCommon(PRectangle rc, Font &font_, int ybase, const char *s, int len, UINT fuOptions);
	void DrawTextNoClip(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore, ColourAllocated back);
	void DrawTextClipped(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore, ColourAllocated back);
	void DrawTextTransparent(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore);
	void MeasureWidths(Font &font_, const char *s, int len, int *positions);
	int WidthText(Font &font_, const char *s, int len);
	int WidthChar(Font &font_, char ch);
	int Ascent(Font &font_);
	int Descent(Font &font_);
	int InternalLeading(Font &font_);
	int ExternalLeading(Font &font_);
	int Height(Font &font_);
	int AverageCharWidth(Font &font_);

	int SetPalette(Palette *pal, bool inBackGround);
	void SetClip(PRectangle rc);
	void FlushCachedState();

	void SetUnicodeMode(bool unicodeMode_);
	void SetDBCSMode(int codePage_);
};

#ifdef SCI_NAMESPACE
} //namespace Scintilla
#endif

SurfaceImpl::SurfaceImpl() :
	unicodeMode(false),
	hdc(0), 	hdcOwned(false),
	pen(0), 	penOld(0),
	brush(0), brushOld(0),
	font(0), 	fontOld(0),
	bitmap(0), bitmapOld(0),
	paletteOld(0) {
	// Windows 9x has only a 16 bit coordinate system so break after 30000 pixels
	maxWidthMeasure = IsNT() ? 1000000 : 30000;
	// There appears to be a 16 bit string length limit in GDI on NT and a limit of
	// 8192 characters on Windows 95.
	maxLenText = IsNT() ? 65535 : 8192;

	codePage = 0;
	win9xACPSame = false;
}

SurfaceImpl::~SurfaceImpl() {
	Release();
}

void SurfaceImpl::Release() {
	if (penOld) {
		::SelectObject(reinterpret_cast<HDC>(hdc), penOld);
		::DeleteObject(pen);
		penOld = 0;
	}
	pen = 0;
	if (brushOld) {
		::SelectObject(reinterpret_cast<HDC>(hdc), brushOld);
		::DeleteObject(brush);
		brushOld = 0;
	}
	brush = 0;
	if (fontOld) {
		// Fonts are not deleted as they are owned by a Font object
		::SelectObject(reinterpret_cast<HDC>(hdc), fontOld);
		fontOld = 0;
	}
	font = 0;
	if (bitmapOld) {
		::SelectObject(reinterpret_cast<HDC>(hdc), bitmapOld);
		::DeleteObject(bitmap);
		bitmapOld = 0;
	}
	bitmap = 0;
	if (paletteOld) {
		// Palettes are not deleted as they are owned by a Palette object
		::SelectPalette(reinterpret_cast<HDC>(hdc),
			reinterpret_cast<HPALETTE>(paletteOld), TRUE);
		paletteOld = 0;
	}
	if (hdcOwned) {
		::DeleteDC(reinterpret_cast<HDC>(hdc));
		hdc = 0;
		hdcOwned = false;
	}
}

bool SurfaceImpl::Initialised() {
	return hdc != 0;
}

void SurfaceImpl::Init(WindowID) {
	Release();
	hdc = ::CreateCompatibleDC(NULL);
	hdcOwned = true;
	::SetTextAlign(reinterpret_cast<HDC>(hdc), TA_BASELINE);
}

void SurfaceImpl::Init(SurfaceID sid, WindowID) {
	Release();
	hdc = reinterpret_cast<HDC>(sid);
	::SetTextAlign(reinterpret_cast<HDC>(hdc), TA_BASELINE);
}

void SurfaceImpl::InitPixMap(int width, int height, Surface *surface_, WindowID) {
	Release();
	hdc = ::CreateCompatibleDC(static_cast<SurfaceImpl *>(surface_)->hdc);
	hdcOwned = true;
	bitmap = ::CreateCompatibleBitmap(static_cast<SurfaceImpl *>(surface_)->hdc, width, height);
	bitmapOld = static_cast<HBITMAP>(::SelectObject(hdc, bitmap));
	::SetTextAlign(reinterpret_cast<HDC>(hdc), TA_BASELINE);
}

void SurfaceImpl::PenColour(ColourAllocated fore) {
	if (pen) {
		::SelectObject(hdc, penOld);
		::DeleteObject(pen);
		pen = 0;
		penOld = 0;
	}
	pen = ::CreatePen(0,1,fore.AsLong());
	penOld = static_cast<HPEN>(::SelectObject(reinterpret_cast<HDC>(hdc), pen));
}

void SurfaceImpl::BrushColor(ColourAllocated back) {
	if (brush) {
		::SelectObject(hdc, brushOld);
		::DeleteObject(brush);
		brush = 0;
		brushOld = 0;
	}
	// Only ever want pure, non-dithered brushes
	ColourAllocated colourNearest = ::GetNearestColor(hdc, back.AsLong());
	brush = ::CreateSolidBrush(colourNearest.AsLong());
	brushOld = static_cast<HBRUSH>(::SelectObject(hdc, brush));
}

void SurfaceImpl::SetFont(Font &font_) {
	if (font_.GetID() != font) {
		if (fontOld) {
			::SelectObject(hdc, font_.GetID());
		} else {
			fontOld = static_cast<HFONT>(::SelectObject(hdc, font_.GetID()));
		}
		font = reinterpret_cast<HFONT>(font_.GetID());
	}
}

int SurfaceImpl::LogPixelsY() {
	return ::GetDeviceCaps(hdc, LOGPIXELSY);
}

int SurfaceImpl::DeviceHeightFont(int points) {
	return ::MulDiv(points, LogPixelsY(), 72);
}

void SurfaceImpl::MoveTo(int x_, int y_) {
	::MoveToEx(hdc, x_, y_, 0);
}

void SurfaceImpl::LineTo(int x_, int y_) {
	::LineTo(hdc, x_, y_);
}

void SurfaceImpl::Polygon(Point *pts, int npts, ColourAllocated fore, ColourAllocated back) {
	PenColour(fore);
	BrushColor(back);
	::Polygon(hdc, reinterpret_cast<POINT *>(pts), npts);
}

void SurfaceImpl::RectangleDraw(PRectangle rc, ColourAllocated fore, ColourAllocated back) {
	PenColour(fore);
	BrushColor(back);
	::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
}

void SurfaceImpl::FillRectangle(PRectangle rc, ColourAllocated back) {
	// Using ExtTextOut rather than a FillRect ensures that no dithering occurs.
	// There is no need to allocate a brush either.
	RECT rcw = RectFromPRectangle(rc);
	::SetBkColor(hdc, back.AsLong());
	::ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE, &rcw, TEXT(""), 0, NULL);
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	HBRUSH br;
	if (static_cast<SurfaceImpl &>(surfacePattern).bitmap)
		br = ::CreatePatternBrush(static_cast<SurfaceImpl &>(surfacePattern).bitmap);
	else	// Something is wrong so display in red
		br = ::CreateSolidBrush(RGB(0xff, 0, 0));
	RECT rcw = RectFromPRectangle(rc);
	::FillRect(hdc, &rcw, br);
	::DeleteObject(br);
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, ColourAllocated fore, ColourAllocated back) {
	PenColour(fore);
	BrushColor(back);
	::RoundRect(hdc,
		rc.left + 1, rc.top,
		rc.right - 1, rc.bottom,
		8, 8);
}

// Plot a point into a DWORD buffer symetrically to all 4 qudrants
static void AllFour(DWORD *pixels, int width, int height, int x, int y, DWORD val) {
	pixels[y*width+x] = val;
	pixels[y*width+width-1-x] = val;
	pixels[(height-1-y)*width+x] = val;
	pixels[(height-1-y)*width+width-1-x] = val;
}

#ifndef AC_SRC_OVER
#define AC_SRC_OVER                 0x00
#endif
#ifndef AC_SRC_ALPHA
#define AC_SRC_ALPHA		0x01
#endif

static DWORD dwordFromBGRA(byte b, byte g, byte r, byte a) {
	union {
		byte pixVal[4];
		DWORD val;
	} converter;
	converter.pixVal[0] = b;
	converter.pixVal[1] = g;
	converter.pixVal[2] = r;
	converter.pixVal[3] = a;
	return converter.val;
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, int cornerSize, ColourAllocated fill, int alphaFill,
		ColourAllocated outline, int alphaOutline, int /* flags*/ ) {
	if (AlphaBlendFn && rc.Width() > 0) {
		HDC hMemDC = ::CreateCompatibleDC(reinterpret_cast<HDC>(hdc));
		int width = rc.Width();
		int height = rc.Height();
		// Ensure not distorted too much by corners when small
		cornerSize = Platform::Minimum(cornerSize, (Platform::Minimum(width, height) / 2) - 2);
		BITMAPINFO bpih = {sizeof(BITMAPINFOHEADER), width, height, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
		void *image = 0;
		HBITMAP hbmMem = CreateDIBSection(reinterpret_cast<HDC>(hMemDC), &bpih,
			DIB_RGB_COLORS, &image, NULL, 0);

		HBITMAP hbmOld = SelectBitmap(hMemDC, hbmMem);

		DWORD valEmpty = dwordFromBGRA(0,0,0,0);
		DWORD valFill = dwordFromBGRA(
			static_cast<byte>(GetBValue(fill.AsLong()) * alphaFill / 255),
			static_cast<byte>(GetGValue(fill.AsLong()) * alphaFill / 255),
			static_cast<byte>(GetRValue(fill.AsLong()) * alphaFill / 255),
			static_cast<byte>(alphaFill));
		DWORD valOutline = dwordFromBGRA(
			static_cast<byte>(GetBValue(outline.AsLong()) * alphaOutline / 255),
			static_cast<byte>(GetGValue(outline.AsLong()) * alphaOutline / 255),
			static_cast<byte>(GetRValue(outline.AsLong()) * alphaOutline / 255),
			static_cast<byte>(alphaOutline));
		DWORD *pixels = reinterpret_cast<DWORD *>(image);
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				if ((x==0) || (x==width-1) || (y == 0) || (y == height-1)) {
					pixels[y*width+x] = valOutline;
				} else {
					pixels[y*width+x] = valFill;
				}
			}
		}
		for (int c=0;c<cornerSize; c++) {
			for (int x=0;x<c+1; x++) {
				AllFour(pixels, width, height, x, c-x, valEmpty);
			}
		}
		for (int x=1;x<cornerSize; x++) {
			AllFour(pixels, width, height, x, cornerSize-x, valOutline);
		}

		BLENDFUNCTION merge = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

		AlphaBlendFn(reinterpret_cast<HDC>(hdc), rc.left, rc.top, width, height, hMemDC, 0, 0, width, height, merge);

		SelectBitmap(hMemDC, hbmOld);
		::DeleteObject(hbmMem);
		::DeleteDC(hMemDC);
	} else {
		BrushColor(outline);
		RECT rcw = RectFromPRectangle(rc);
		FrameRect(hdc, &rcw, brush);
	}
}

void SurfaceImpl::Ellipse(PRectangle rc, ColourAllocated fore, ColourAllocated back) {
	PenColour(fore);
	BrushColor(back);
	::Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	::BitBlt(hdc,
		rc.left, rc.top, rc.Width(), rc.Height(),
		static_cast<SurfaceImpl &>(surfaceSource).hdc, from.x, from.y, SRCCOPY);
}

// Buffer to hold strings and string position arrays without always allocating on heap.
// May sometimes have string too long to allocate on stack. So use a fixed stack-allocated buffer
// when less than safe size otherwise allocate on heap and free automatically.
template<typename T, int lengthStandard>
class VarBuffer {
	T bufferStandard[lengthStandard];
public:
	T *buffer;
	VarBuffer(size_t length) : buffer(0) {
		if (length > lengthStandard) {
			buffer = new T[length];
		} else {
			buffer = bufferStandard;
		}
	}
	~VarBuffer() {
		if (buffer != bufferStandard) {
			delete []buffer;
			buffer = 0;
		}
	}
};

const int stackBufferLength = 10000;
class TextWide : public VarBuffer<wchar_t, stackBufferLength> {
public:
	int tlen;
	TextWide(const char *s, int len, bool unicodeMode, int codePage=0) :
		VarBuffer<wchar_t, stackBufferLength>(len) {
		if (unicodeMode) {
			tlen = UTF16FromUTF8(s, len, buffer, len);
		} else {
			// Support Asian string display in 9x English
			tlen = ::MultiByteToWideChar(codePage, 0, s, len, buffer, len);
		}
	}
};
typedef VarBuffer<int, stackBufferLength> TextPositions;

void SurfaceImpl::DrawTextCommon(PRectangle rc, Font &font_, int ybase, const char *s, int len, UINT fuOptions) {
	SetFont(font_);
	RECT rcw = RectFromPRectangle(rc);
	SIZE sz={0,0};
	int pos = 0;
	int x = rc.left;

	// Text drawing may fail if the text is too big.
	// If it does fail, slice up into segments and draw each segment.
	const int maxSegmentLength = 0x200;

	if ((!unicodeMode) && (IsNT() || (codePage==0) || win9xACPSame)) {
		// Use ANSI calls
		int lenDraw = Platform::Minimum(len, maxLenText);
		if (!::ExtTextOutA(hdc, x, ybase, fuOptions, &rcw, s, lenDraw, NULL)) {
			while (lenDraw > pos) {
				int seglen = Platform::Minimum(maxSegmentLength, lenDraw - pos);
				if (!::ExtTextOutA(hdc, x, ybase, fuOptions, &rcw, s+pos, seglen, NULL)) {
					PLATFORM_ASSERT(false);
					return;
				}
				::GetTextExtentPoint32A(hdc, s+pos, seglen, &sz);
				x += sz.cx;
				pos += seglen;
			}
		}
	} else {
		// Use Unicode calls
		const TextWide tbuf(s, len, unicodeMode, codePage);
		if (!::ExtTextOutW(hdc, x, ybase, fuOptions, &rcw, tbuf.buffer, tbuf.tlen, NULL)) {
			while (tbuf.tlen > pos) {
				int seglen = Platform::Minimum(maxSegmentLength, tbuf.tlen - pos);
				if (!::ExtTextOutW(hdc, x, ybase, fuOptions, &rcw, tbuf.buffer+pos, seglen, NULL)) {
					PLATFORM_ASSERT(false);
					return;
				}
				::GetTextExtentPoint32W(hdc, tbuf.buffer+pos, seglen, &sz);
				x += sz.cx;
				pos += seglen;
			}
		}
	}
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc, Font &font_, int ybase, const char *s, int len,
	ColourAllocated fore, ColourAllocated back) {
	::SetTextColor(hdc, fore.AsLong());
	::SetBkColor(hdc, back.AsLong());
	DrawTextCommon(rc, font_, ybase, s, len, ETO_OPAQUE);
}

void SurfaceImpl::DrawTextClipped(PRectangle rc, Font &font_, int ybase, const char *s, int len,
	ColourAllocated fore, ColourAllocated back) {
	::SetTextColor(hdc, fore.AsLong());
	::SetBkColor(hdc, back.AsLong());
	DrawTextCommon(rc, font_, ybase, s, len, ETO_OPAQUE | ETO_CLIPPED);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, Font &font_, int ybase, const char *s, int len,
	ColourAllocated fore) {
	// Avoid drawing spaces in transparent mode
	for (int i=0;i<len;i++) {
		if (s[i] != ' ') {
			::SetTextColor(hdc, fore.AsLong());
			::SetBkMode(hdc, TRANSPARENT);
			DrawTextCommon(rc, font_, ybase, s, len, 0);
			::SetBkMode(hdc, OPAQUE);
			return;
		}
	}
}

int SurfaceImpl::WidthText(Font &font_, const char *s, int len) {
	SetFont(font_);
	SIZE sz={0,0};
	if ((!unicodeMode) && (IsNT() || (codePage==0) || win9xACPSame)) {
		::GetTextExtentPoint32A(hdc, s, Platform::Minimum(len, maxLenText), &sz);
	} else {
		const TextWide tbuf(s, len, unicodeMode, codePage);
		::GetTextExtentPoint32W(hdc, tbuf.buffer, tbuf.tlen, &sz);
	}
	return sz.cx;
}

void SurfaceImpl::MeasureWidths(Font &font_, const char *s, int len, int *positions) {
	SetFont(font_);
	SIZE sz={0,0};
	int fit = 0;
	if (unicodeMode) {
		const TextWide tbuf(s, len, unicodeMode, codePage);
		TextPositions poses(tbuf.tlen);
		fit = tbuf.tlen;
		if (!::GetTextExtentExPointW(hdc, tbuf.buffer, tbuf.tlen, maxWidthMeasure, &fit, poses.buffer, &sz)) {
			// Likely to have failed because on Windows 9x where function not available
			// So measure the character widths by measuring each initial substring
			// Turns a linear operation into a qudratic but seems fast enough on test files
			for (int widthSS=0; widthSS < tbuf.tlen; widthSS++) {
				::GetTextExtentPoint32W(hdc, tbuf.buffer, widthSS+1, &sz);
				poses.buffer[widthSS] = sz.cx;
			}
		}
		// Map the widths given for UTF-16 characters back onto the UTF-8 input string
		int ui=0;
		const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
		int i=0;
		while (ui<fit) {
			unsigned char uch = us[i];
			unsigned int lenChar = 1;
			if (uch >= (0x80 + 0x40 + 0x20 + 0x10)) {
				lenChar = 4;
				ui++;
			} else if (uch >= (0x80 + 0x40 + 0x20)) {
				lenChar = 3;
			} else if (uch >= (0x80)) {
				lenChar = 2;
			}
			for (unsigned int bytePos=0; (bytePos<lenChar) && (i<len); bytePos++) {
				positions[i++] = poses.buffer[ui];
			}
			ui++;
		}
		int lastPos = 0;
		if (i > 0)
			lastPos = positions[i-1];
		while (i<len) {
			positions[i++] = lastPos;
		}
	} else if (IsNT() || (codePage==0) || win9xACPSame) {
		// Zero positions to avoid random behaviour on failure.
		memset(positions, 0, len * sizeof(*positions));
		// len may be larger than platform supports so loop over segments small enough for platform
		int startOffset = 0;
		while (len > 0) {
			int lenBlock = Platform::Minimum(len, maxLenText);
			if (!::GetTextExtentExPointA(hdc, s, lenBlock, maxWidthMeasure, &fit, positions, &sz)) {
				// Eeek - a NULL DC or other foolishness could cause this.
				return;
			} else if (fit < lenBlock) {
				// For some reason, such as an incomplete DBCS character
				// Not all the positions are filled in so make them equal to end.
				for (int i=fit;i<lenBlock;i++)
					positions[i] = positions[fit-1];
			} else if (startOffset > 0) {
				for (int i=0;i<lenBlock;i++)
					positions[i] += startOffset;
			}
			startOffset = positions[lenBlock-1];
			len -= lenBlock;
			positions += lenBlock;
			s += lenBlock;
		}
	} else {
		// Support Asian string display in 9x English
		const TextWide tbuf(s, len, unicodeMode, codePage);
		TextPositions poses(tbuf.tlen);
		for (int widthSS=0; widthSS<tbuf.tlen; widthSS++) {
			::GetTextExtentPoint32W(hdc, tbuf.buffer, widthSS+1, &sz);
			poses.buffer[widthSS] = sz.cx;
		}

		int ui = 0;
		for (int i=0;i<len;) {
			if (::IsDBCSLeadByteEx(codePage, s[i])) {
				positions[i] = poses.buffer[ui];
				positions[i+1] = poses.buffer[ui];
				i += 2;
			} else {
				positions[i] = poses.buffer[ui];
				i++;
			}

			ui++;
		}
	}
}

int SurfaceImpl::WidthChar(Font &font_, char ch) {
	SetFont(font_);
	SIZE sz;
	::GetTextExtentPoint32A(hdc, &ch, 1, &sz);
	return sz.cx;
}

int SurfaceImpl::Ascent(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmAscent;
}

int SurfaceImpl::Descent(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmDescent;
}

int SurfaceImpl::InternalLeading(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmInternalLeading;
}

int SurfaceImpl::ExternalLeading(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmExternalLeading;
}

int SurfaceImpl::Height(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmHeight;
}

int SurfaceImpl::AverageCharWidth(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmAveCharWidth;
}

int SurfaceImpl::SetPalette(Palette *pal, bool inBackGround) {
	if (paletteOld) {
		::SelectPalette(hdc, paletteOld, TRUE);
	}
	paletteOld = 0;
	int changes = 0;
	if (pal->allowRealization) {
		paletteOld = ::SelectPalette(hdc,
			reinterpret_cast<HPALETTE>(pal->hpal), inBackGround);
		changes = ::RealizePalette(hdc);
	}
	return changes;
}

void SurfaceImpl::SetClip(PRectangle rc) {
	::IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
}

void SurfaceImpl::FlushCachedState() {
	pen = 0;
	brush = 0;
	font = 0;
}

void SurfaceImpl::SetUnicodeMode(bool unicodeMode_) {
	unicodeMode=unicodeMode_;
}

void SurfaceImpl::SetDBCSMode(int codePage_) {
	// No action on window as automatically handled by system.
	codePage = codePage_;
	win9xACPSame = !IsNT() && ((unsigned int)codePage == ::GetACP());
}

Surface *Surface::Allocate() {
	return new SurfaceImpl;
}

Window::~Window() {
}

void Window::Destroy() {
	if (wid)
		::DestroyWindow(reinterpret_cast<HWND>(wid));
	wid = 0;
}

bool Window::HasFocus() {
	return ::GetFocus() == wid;
}

PRectangle Window::GetPosition() {
	RECT rc;
	::GetWindowRect(reinterpret_cast<HWND>(wid), &rc);
	return PRectangle(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::SetPosition(PRectangle rc) {
	::SetWindowPos(reinterpret_cast<HWND>(wid),
		0, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER|SWP_NOACTIVATE);
}

void Window::SetPositionRelative(PRectangle rc, Window w) {
	LONG style = ::GetWindowLong(reinterpret_cast<HWND>(wid), GWL_STYLE);
	if (style & WS_POPUP) {
		RECT rcOther;
		::GetWindowRect(reinterpret_cast<HWND>(w.GetID()), &rcOther);
		rc.Move(rcOther.left, rcOther.top);

		// This #ifdef is for VC 98 which has problems with MultiMon.h under some conditions.
#ifdef MONITOR_DEFAULTTONULL
		// We're using the stub functionality of MultiMon.h to decay gracefully on machines
		// (ie, pre Win2000, Win95) that do not support the newer functions.
		RECT rcMonitor;
		memcpy(&rcMonitor, &rc, sizeof(rcMonitor));  // RECT and Rectangle are the same really.
		MONITORINFO mi = {0};
		mi.cbSize = sizeof(mi);

		HMONITOR hMonitor = ::MonitorFromRect(&rcMonitor, MONITOR_DEFAULTTONEAREST);
		// If hMonitor is NULL, that's just the main screen anyways.
		::GetMonitorInfo(hMonitor, &mi);

		// Now clamp our desired rectangle to fit inside the work area
		// This way, the menu will fit wholly on one screen. An improvement even
		// if you don't have a second monitor on the left... Menu's appears half on
		// one screen and half on the other are just U.G.L.Y.!
		if (rc.right > mi.rcWork.right)
			rc.Move(mi.rcWork.right - rc.right, 0);
		if (rc.bottom > mi.rcWork.bottom)
			rc.Move(0, mi.rcWork.bottom - rc.bottom);
		if (rc.left < mi.rcWork.left)
			rc.Move(mi.rcWork.left - rc.left, 0);
		if (rc.top < mi.rcWork.top)
			rc.Move(0, mi.rcWork.top - rc.top);
#endif
	}
	SetPosition(rc);
}

PRectangle Window::GetClientPosition() {
	RECT rc={0,0,0,0};
	if (wid)
		::GetClientRect(reinterpret_cast<HWND>(wid), &rc);
	return  PRectangle(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::Show(bool show) {
	if (show)
		::ShowWindow(reinterpret_cast<HWND>(wid), SW_SHOWNOACTIVATE);
	else
		::ShowWindow(reinterpret_cast<HWND>(wid), SW_HIDE);
}

void Window::InvalidateAll() {
	::InvalidateRect(reinterpret_cast<HWND>(wid), NULL, FALSE);
}

void Window::InvalidateRectangle(PRectangle rc) {
	RECT rcw = RectFromPRectangle(rc);
	::InvalidateRect(reinterpret_cast<HWND>(wid), &rcw, FALSE);
}

static LRESULT Window_SendMessage(Window *w, UINT msg, WPARAM wParam=0, LPARAM lParam=0) {
	return ::SendMessage(reinterpret_cast<HWND>(w->GetID()), msg, wParam, lParam);
}

void Window::SetFont(Font &font) {
	Window_SendMessage(this, WM_SETFONT,
		reinterpret_cast<WPARAM>(font.GetID()), 0);
}

void Window::SetCursor(Cursor curs) {
	switch (curs) {
	case cursorText:
		::SetCursor(::LoadCursor(NULL,IDC_IBEAM));
		break;
	case cursorUp:
		::SetCursor(::LoadCursor(NULL,IDC_UPARROW));
		break;
	case cursorWait:
		::SetCursor(::LoadCursor(NULL,IDC_WAIT));
		break;
	case cursorHoriz:
		::SetCursor(::LoadCursor(NULL,IDC_SIZEWE));
		break;
	case cursorVert:
		::SetCursor(::LoadCursor(NULL,IDC_SIZENS));
		break;
	case cursorHand:
		::SetCursor(::LoadCursor(NULL,IDC_HAND));
		break;
	case cursorReverseArrow: {
			if (!hinstPlatformRes)
				hinstPlatformRes = ::GetModuleHandle(TEXT("Scintilla"));
			if (!hinstPlatformRes)
				hinstPlatformRes = ::GetModuleHandle(TEXT("SciLexer"));
			if (!hinstPlatformRes)
				hinstPlatformRes = ::GetModuleHandle(NULL);
			HCURSOR hcursor = ::LoadCursor(hinstPlatformRes, MAKEINTRESOURCE(IDC_MARGIN));
			if (hcursor)
				::SetCursor(hcursor);
			else
				::SetCursor(::LoadCursor(NULL,IDC_ARROW));
		}
		break;
	case cursorArrow:
	case cursorInvalid:	// Should not occur, but just in case.
		::SetCursor(::LoadCursor(NULL,IDC_ARROW));
		break;
	}
}

void Window::SetTitle(const char *s) {
	::SetWindowTextA(reinterpret_cast<HWND>(wid), s);
}

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   coordinates */
PRectangle Window::GetMonitorRect(Point pt) {
#ifdef MONITOR_DEFAULTTONULL
	// MonitorFromPoint and GetMonitorInfo are not available on Windows 95 so are not used.
	// There could be conditional code and dynamic loading in a future version
	// so this would work on those platforms where they are available.
	PRectangle rcPosition = GetPosition();
	POINT ptDesktop = {pt.x + rcPosition.left, pt.y + rcPosition.top};
	HMONITOR hMonitor = ::MonitorFromPoint(ptDesktop, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = {0};
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	if (::GetMonitorInfo(hMonitor, &mi)) {
		PRectangle rcMonitor(
			mi.rcWork.left - rcPosition.left,
			mi.rcWork.top - rcPosition.top,
			mi.rcWork.right - rcPosition.left,
			mi.rcWork.bottom - rcPosition.top);
		return rcMonitor;
	} else {
		return PRectangle();
	}
#else
	return PRectangle();
#endif
}

struct ListItemData {
	const char *text;
	int pixId;
};

#define _ROUND2(n,pow2) \
	( ( (n) + (pow2) - 1) & ~((pow2) - 1) )

class LineToItem {
	char *words;
	int wordsCount;
	int wordsSize;

	ListItemData *data;
	int len;
	int count;

private:
	void FreeWords() {
		delete []words;
		words = NULL;
		wordsCount = 0;
		wordsSize = 0;
	}
	char *AllocWord(const char *word);

public:
	LineToItem() : words(NULL), wordsCount(0), wordsSize(0), data(NULL), len(0), count(0) {
	}
	~LineToItem() {
		Clear();
	}
	void Clear() {
		FreeWords();
		delete []data;
		data = NULL;
		len = 0;
		count = 0;
	}

	ListItemData *Append(const char *text, int value);

	ListItemData Get(int index) const {
		if (index >= 0 && index < count) {
			return data[index];
		} else {
			ListItemData missing = {"", -1};
			return missing;
		}
	}
	int Count() const {
		return count;
	}

	ListItemData *AllocItem();

	void SetWords(char *s) {
		words = s;	// N.B. will be deleted on destruction
	}
};

char *LineToItem::AllocWord(const char *text) {
	int chars = strlen(text) + 1;
	int newCount = wordsCount + chars;
	if (newCount > wordsSize) {
		wordsSize = _ROUND2(newCount * 2, 8192);
		char *wordsNew = new char[wordsSize];
		memcpy(wordsNew, words, wordsCount);
		int offset = wordsNew - words;
		for (int i=0; i<count; i++)
			data[i].text += offset;
		delete []words;
		words = wordsNew;
	}
	char *s = &words[wordsCount];
	wordsCount = newCount;
	strncpy(s, text, chars);
	return s;
}

ListItemData *LineToItem::AllocItem() {
	if (count >= len) {
		int lenNew = _ROUND2((count+1) * 2, 1024);
		ListItemData *dataNew = new ListItemData[lenNew];
		memcpy(dataNew, data, count * sizeof(ListItemData));
		delete []data;
		data = dataNew;
		len = lenNew;
	}
	ListItemData *item = &data[count];
	count++;
	return item;
}

ListItemData *LineToItem::Append(const char *text, int imageIndex) {
	ListItemData *item = AllocItem();
	item->text = AllocWord(text);
	item->pixId = imageIndex;
	return item;
}

const TCHAR ListBoxX_ClassName[] = TEXT("ListBoxX");

ListBox::ListBox() {
}

ListBox::~ListBox() {
}

class ListBoxX : public ListBox {
	int lineHeight;
	FontID fontCopy;
	XPMSet xset;
	LineToItem lti;
	HWND lb;
	bool unicodeMode;
	int desiredVisibleRows;
	unsigned int maxItemCharacters;
	unsigned int aveCharWidth;
	Window *parent;
	int ctrlID;
	CallBackAction doubleClickAction;
	void *doubleClickActionData;
	const char *widestItem;
	unsigned int maxCharWidth;
	int resizeHit;
	PRectangle rcPreSize;
	Point dragOffset;
	Point location;	// Caret location at which the list is opened

	HWND GetHWND() const;
	void AppendListItem(const char *startword, const char *numword);
	void AdjustWindowRect(PRectangle *rc) const;
	int ItemHeight() const;
	int MinClientWidth() const;
	int TextOffset() const;
	Point GetClientExtent() const;
	POINT MinTrackSize() const;
	POINT MaxTrackSize() const;
	void SetRedraw(bool on);
	void OnDoubleClick();
	void ResizeToCursor();
	void StartResize(WPARAM);
	int NcHitTest(WPARAM, LPARAM) const;
	void CentreItem(int);
	void Paint(HDC);
	static LRESULT PASCAL ControlWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	static const Point ItemInset;	// Padding around whole item
	static const Point TextInset;	// Padding around text
	static const Point ImageInset;	// Padding around image

public:
	ListBoxX() : lineHeight(10), fontCopy(0), lb(0), unicodeMode(false),
		desiredVisibleRows(5), maxItemCharacters(0), aveCharWidth(8),
		parent(NULL), ctrlID(0), doubleClickAction(NULL), doubleClickActionData(NULL),
		widestItem(NULL), maxCharWidth(1), resizeHit(0) {
	}
	virtual ~ListBoxX() {
		if (fontCopy) {
			::DeleteObject(fontCopy);
			fontCopy = 0;
		}
	}
	virtual void SetFont(Font &font);
	virtual void Create(Window &parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_);
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
	virtual void RegisterImage(int type, const char *xpm_data);
	virtual void ClearRegisteredImages();
	virtual void SetDoubleClickAction(CallBackAction action, void *data) {
		doubleClickAction = action;
		doubleClickActionData = data;
	}
	virtual void SetList(const char *list, char separator, char typesep);
	void Draw(DRAWITEMSTRUCT *pDrawItem);
	LRESULT WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	static LRESULT PASCAL StaticWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
};

const Point ListBoxX::ItemInset(0, 0);
const Point ListBoxX::TextInset(2, 0);
const Point ListBoxX::ImageInset(1, 0);

ListBox *ListBox::Allocate() {
	ListBoxX *lb = new ListBoxX();
	return lb;
}

void ListBoxX::Create(Window &parent_, int ctrlID_, Point location_, int lineHeight_, bool unicodeMode_) {
	parent = &parent_;
	ctrlID = ctrlID_;
	location = location_;
	lineHeight = lineHeight_;
	unicodeMode = unicodeMode_;
	HWND hwndParent = reinterpret_cast<HWND>(parent->GetID());
	HINSTANCE hinstanceParent = GetWindowInstance(hwndParent);
	// Window created as popup so not clipped within parent client area
	wid = ::CreateWindowEx(
		WS_EX_WINDOWEDGE, ListBoxX_ClassName, TEXT(""),
		WS_POPUP | WS_THICKFRAME,
		100,100, 150,80, hwndParent,
		NULL,
		hinstanceParent,
		this);

	::MapWindowPoints(hwndParent, NULL, reinterpret_cast<POINT*>(&location), 1);
}

void ListBoxX::SetFont(Font &font) {
	LOGFONT lf;
	if (0 != ::GetObject(font.GetID(), sizeof(lf), &lf)) {
		if (fontCopy) {
			::DeleteObject(fontCopy);
			fontCopy = 0;
		}
		fontCopy = ::CreateFontIndirect(&lf);
		::SendMessage(lb, WM_SETFONT, reinterpret_cast<WPARAM>(fontCopy), 0);
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

HWND ListBoxX::GetHWND() const {
	return reinterpret_cast<HWND>(GetID());
}

PRectangle ListBoxX::GetDesiredRect() {
	PRectangle rcDesired = GetPosition();

	int rows = Length();
	if ((rows == 0) || (rows > desiredVisibleRows))
		rows = desiredVisibleRows;
	rcDesired.bottom = rcDesired.top + ItemHeight() * rows;

	int width = MinClientWidth();
	HDC hdc = ::GetDC(lb);
	HFONT oldFont = SelectFont(hdc, fontCopy);
	SIZE textSize = {0, 0};
	int len = widestItem ? strlen(widestItem) : 0;
	if (unicodeMode) {
		const TextWide tbuf(widestItem, len, unicodeMode);
		::GetTextExtentPoint32W(hdc, tbuf.buffer, tbuf.tlen, &textSize);
	} else {
		::GetTextExtentPoint32A(hdc, widestItem, len, &textSize);
	}
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	maxCharWidth = tm.tmMaxCharWidth;
	SelectFont(hdc, oldFont);
	::ReleaseDC(lb, hdc);

	int widthDesired = Platform::Maximum(textSize.cx, (len + 1) * tm.tmAveCharWidth);
	if (width < widthDesired)
		width = widthDesired;

	rcDesired.right = rcDesired.left + TextOffset() + width + (TextInset.x * 2);
	if (Length() > rows)
		rcDesired.right += ::GetSystemMetrics(SM_CXVSCROLL);

	AdjustWindowRect(&rcDesired);
	return rcDesired;
}

int ListBoxX::TextOffset() const {
	int pixWidth = const_cast<XPMSet*>(&xset)->GetWidth();
	return pixWidth == 0 ? ItemInset.x : ItemInset.x + pixWidth + (ImageInset.x * 2);
}

int ListBoxX::CaretFromEdge() {
	PRectangle rc;
	AdjustWindowRect(&rc);
	return TextOffset() + TextInset.x + (0 - rc.left) - 1;
}

void ListBoxX::Clear() {
	::SendMessage(lb, LB_RESETCONTENT, 0, 0);
	maxItemCharacters = 0;
	widestItem = NULL;
	lti.Clear();
}

void ListBoxX::Append(char *s, int type) {
	int index = ::SendMessage(lb, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
	if (index < 0)
		return;
	ListItemData *newItem = lti.Append(s, type);
	unsigned int len = static_cast<unsigned int>(strlen(s));
	if (maxItemCharacters < len) {
		maxItemCharacters = len;
		widestItem = newItem->text;
	}
}

int ListBoxX::Length() {
	return lti.Count();
}

void ListBoxX::Select(int n) {
	// We are going to scroll to centre on the new selection and then select it, so disable
	// redraw to avoid flicker caused by a painting new selection twice in unselected and then
	// selected states
	SetRedraw(false);
	CentreItem(n);
	::SendMessage(lb, LB_SETCURSEL, n, 0);
	SetRedraw(true);
}

int ListBoxX::GetSelection() {
	return ::SendMessage(lb, LB_GETCURSEL, 0, 0);
}

// This is not actually called at present
int ListBoxX::Find(const char *) {
	return LB_ERR;
}

void ListBoxX::GetValue(int n, char *value, int len) {
	ListItemData item = lti.Get(n);
	strncpy(value, item.text, len);
	value[len-1] = '\0';
}

void ListBoxX::RegisterImage(int type, const char *xpm_data) {
	xset.Add(type, xpm_data);
}

void ListBoxX::ClearRegisteredImages() {
	xset.Clear();
}

void ListBoxX::Draw(DRAWITEMSTRUCT *pDrawItem) {
	if ((pDrawItem->itemAction == ODA_SELECT) || (pDrawItem->itemAction == ODA_DRAWENTIRE)) {
		RECT rcBox = pDrawItem->rcItem;
		rcBox.left += TextOffset();
		if (pDrawItem->itemState & ODS_SELECTED) {
			RECT rcImage = pDrawItem->rcItem;
			rcImage.right = rcBox.left;
			// The image is not highlighted
			::FillRect(pDrawItem->hDC, &rcImage, reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
			::FillRect(pDrawItem->hDC, &rcBox, reinterpret_cast<HBRUSH>(COLOR_HIGHLIGHT+1));
			::SetBkColor(pDrawItem->hDC, ::GetSysColor(COLOR_HIGHLIGHT));
			::SetTextColor(pDrawItem->hDC, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
		} else {
			::FillRect(pDrawItem->hDC, &pDrawItem->rcItem, reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
			::SetBkColor(pDrawItem->hDC, ::GetSysColor(COLOR_WINDOW));
			::SetTextColor(pDrawItem->hDC, ::GetSysColor(COLOR_WINDOWTEXT));
		}

		ListItemData item = lti.Get(pDrawItem->itemID);
		int pixId = item.pixId;
		const char *text = item.text;
		int len = strlen(text);

		RECT rcText = rcBox;
		::InsetRect(&rcText, TextInset.x, TextInset.y);

		if (unicodeMode) {
			const TextWide tbuf(text, len, unicodeMode);
			::DrawTextW(pDrawItem->hDC, tbuf.buffer, tbuf.tlen, &rcText, DT_NOPREFIX|DT_END_ELLIPSIS|DT_SINGLELINE|DT_NOCLIP);
		} else {
			::DrawTextA(pDrawItem->hDC, text, len, &rcText, DT_NOPREFIX|DT_END_ELLIPSIS|DT_SINGLELINE|DT_NOCLIP);
		}
		if (pDrawItem->itemState & ODS_SELECTED) {
			::DrawFocusRect(pDrawItem->hDC, &rcBox);
		}

		// Draw the image, if any
		XPM *pxpm = xset.Get(pixId);
		if (pxpm) {
			Surface *surfaceItem = Surface::Allocate();
			if (surfaceItem) {
				surfaceItem->Init(pDrawItem->hDC, pDrawItem->hwndItem);
				//surfaceItem->SetUnicodeMode(unicodeMode);
				//surfaceItem->SetDBCSMode(codePage);
				int left = pDrawItem->rcItem.left + ItemInset.x + ImageInset.x;
				PRectangle rcImage(left, pDrawItem->rcItem.top,
					left + xset.GetWidth(), pDrawItem->rcItem.bottom);
				pxpm->Draw(surfaceItem, rcImage);
				delete surfaceItem;
				::SetTextAlign(pDrawItem->hDC, TA_TOP);
			}
		}
	}
}

void ListBoxX::AppendListItem(const char *startword, const char *numword) {
	ListItemData *item = lti.AllocItem();
	item->text = startword;
	if (numword) {
		int pixId = 0;
		char ch;
		while ((ch = *++numword) != '\0') {
			pixId = 10 * pixId + (ch - '0');
		}
		item->pixId = pixId;
	} else {
		item->pixId = -1;
	}

	unsigned int len = static_cast<unsigned int>(strlen(item->text));
	if (maxItemCharacters < len) {
		maxItemCharacters = len;
		widestItem = item->text;
	}
}

void ListBoxX::SetList(const char *list, char separator, char typesep) {
	// Turn off redraw while populating the list - this has a significant effect, even if
	// the listbox is not visible.
	SetRedraw(false);
	Clear();
	int size = strlen(list) + 1;
	char *words = new char[size];
	lti.SetWords(words);
	memcpy(words, list, size);
	char *startword = words;
	char *numword = NULL;
	int i = 0;
	for (; words[i]; i++) {
		if (words[i] == separator) {
			words[i] = '\0';
			if (numword)
				*numword = '\0';
			AppendListItem(startword, numword);
			startword = words + i + 1;
			numword = NULL;
		} else if (words[i] == typesep) {
			numword = words + i;
		}
	}
	if (startword) {
		if (numword)
			*numword = '\0';
		AppendListItem(startword, numword);
	}

	// Finally populate the listbox itself with the correct number of items
	int count = lti.Count();
	::SendMessage(lb, LB_INITSTORAGE, count, 0);
	for (int j=0; j<count; j++) {
		::SendMessage(lb, LB_ADDSTRING, 0, j+1);
	}
	SetRedraw(true);
}

void ListBoxX::AdjustWindowRect(PRectangle *rc) const {
	::AdjustWindowRectEx(reinterpret_cast<RECT*>(rc), WS_THICKFRAME, false, WS_EX_WINDOWEDGE);
}

int ListBoxX::ItemHeight() const {
	int itemHeight = lineHeight + (TextInset.y * 2);
	int pixHeight = const_cast<XPMSet*>(&xset)->GetHeight() + (ImageInset.y * 2);
	if (itemHeight < pixHeight) {
		itemHeight = pixHeight;
	}
	return itemHeight;
}

int ListBoxX::MinClientWidth() const {
	return 12 * (aveCharWidth+aveCharWidth/3);
}

POINT ListBoxX::MinTrackSize() const {
	PRectangle rc(0, 0, MinClientWidth(), ItemHeight());
	AdjustWindowRect(&rc);
	POINT ret = {rc.Width(), rc.Height()};
	return ret;
}

POINT ListBoxX::MaxTrackSize() const {
	PRectangle rc(0, 0,
		maxCharWidth * maxItemCharacters + TextInset.x * 2 +
		 TextOffset() + ::GetSystemMetrics(SM_CXVSCROLL),
		ItemHeight() * lti.Count());
	AdjustWindowRect(&rc);
	POINT ret = {rc.Width(), rc.Height()};
	return ret;
}

void ListBoxX::SetRedraw(bool on) {
	::SendMessage(lb, WM_SETREDRAW, static_cast<BOOL>(on), 0);
	if (on)
		::InvalidateRect(lb, NULL, TRUE);
}

void ListBoxX::ResizeToCursor() {
	PRectangle rc = GetPosition();
	Point pt;
	::GetCursorPos(reinterpret_cast<POINT*>(&pt));
	pt.x += dragOffset.x;
	pt.y += dragOffset.y;

	switch (resizeHit) {
		case HTLEFT:
			rc.left = pt.x;
			break;
		case HTRIGHT:
			rc.right = pt.x;
			break;
		case HTTOP:
			rc.top = pt.y;
			break;
		case HTTOPLEFT:
			rc.top = pt.y;
			rc.left = pt.x;
			break;
		case HTTOPRIGHT:
			rc.top = pt.y;
			rc.right = pt.x;
			break;
		case HTBOTTOM:
			rc.bottom = pt.y;
			break;
		case HTBOTTOMLEFT:
			rc.bottom = pt.y;
			rc.left = pt.x;
			break;
		case HTBOTTOMRIGHT:
			rc.bottom = pt.y;
			rc.right = pt.x;
			break;
	}

	POINT ptMin = MinTrackSize();
	POINT ptMax = MaxTrackSize();
	// We don't allow the left edge to move at present, but just in case
	rc.left = Platform::Maximum(Platform::Minimum(rc.left, rcPreSize.right - ptMin.x), rcPreSize.right - ptMax.x);
	rc.top = Platform::Maximum(Platform::Minimum(rc.top, rcPreSize.bottom - ptMin.y), rcPreSize.bottom - ptMax.y);
	rc.right = Platform::Maximum(Platform::Minimum(rc.right, rcPreSize.left + ptMax.x), rcPreSize.left + ptMin.x);
	rc.bottom = Platform::Maximum(Platform::Minimum(rc.bottom, rcPreSize.top + ptMax.y), rcPreSize.top + ptMin.y);

	SetPosition(rc);
}

void ListBoxX::StartResize(WPARAM hitCode) {
	rcPreSize = GetPosition();
	POINT cursorPos;
	::GetCursorPos(&cursorPos);

	switch (hitCode) {
		case HTRIGHT:
		case HTBOTTOM:
		case HTBOTTOMRIGHT:
			dragOffset.x = rcPreSize.right - cursorPos.x;
			dragOffset.y = rcPreSize.bottom - cursorPos.y;
			break;

		case HTTOPRIGHT:
			dragOffset.x = rcPreSize.right - cursorPos.x;
			dragOffset.y = rcPreSize.top - cursorPos.y;
			break;

		// Note that the current hit test code prevents the left edge cases ever firing
		// as we don't want the left edge to be moveable
		case HTLEFT:
		case HTTOP:
		case HTTOPLEFT:
			dragOffset.x = rcPreSize.left - cursorPos.x;
			dragOffset.y = rcPreSize.top - cursorPos.y;
			break;
		case HTBOTTOMLEFT:
			dragOffset.x = rcPreSize.left - cursorPos.x;
			dragOffset.y = rcPreSize.bottom - cursorPos.y;
			break;

		default:
			return;
	}

	::SetCapture(GetHWND());
	resizeHit = hitCode;
}

int ListBoxX::NcHitTest(WPARAM wParam, LPARAM lParam) const {
	int hit = ::DefWindowProc(GetHWND(), WM_NCHITTEST, wParam, lParam);
	// There is an apparent bug in the DefWindowProc hit test code whereby it will
	// return HTTOPXXX if the window in question is shorter than the default
	// window caption height + frame, even if one is hovering over the bottom edge of
	// the frame, so workaround that here
	if (hit >= HTTOP && hit <= HTTOPRIGHT) {
		int minHeight = GetSystemMetrics(SM_CYMINTRACK);
		PRectangle rc = const_cast<ListBoxX*>(this)->GetPosition();
		int yPos = GET_Y_LPARAM(lParam);
		if ((rc.Height() < minHeight) && (yPos > ((rc.top + rc.bottom)/2))) {
			hit += HTBOTTOM - HTTOP;
		}
	}

	// Nerver permit resizing that moves the left edge. Allow movement of top or bottom edge
	// depending on whether the list is above or below the caret
	switch (hit) {
		case HTLEFT:
		case HTTOPLEFT:
		case HTBOTTOMLEFT:
			hit = HTERROR;
			break;

		case HTTOP:
		case HTTOPRIGHT: {
				PRectangle rc = const_cast<ListBoxX*>(this)->GetPosition();
				// Valid only if caret below list
				if (location.y < rc.top)
					hit = HTERROR;
			}
			break;

		case HTBOTTOM:
		case HTBOTTOMRIGHT: {
				PRectangle rc = const_cast<ListBoxX*>(this)->GetPosition();
				// Valid only if caret above list
				if (rc.bottom < location.y)
					hit = HTERROR;
			}
			break;
	}

	return hit;
}

void ListBoxX::OnDoubleClick() {

	if (doubleClickAction != NULL) {
		doubleClickAction(doubleClickActionData);
	}
}

Point ListBoxX::GetClientExtent() const {
	PRectangle rc = const_cast<ListBoxX*>(this)->GetClientPosition();
	return Point(rc.Width(), rc.Height());
}

void ListBoxX::CentreItem(int n) {
	// If below mid point, scroll up to centre, but with more items below if uneven
	if (n >= 0) {
		Point extent = GetClientExtent();
		int visible = extent.y/ItemHeight();
		if (visible < Length()) {
			int top = ::SendMessage(lb, LB_GETTOPINDEX, 0, 0);
			int half = (visible - 1) / 2;
			if (n > (top + half))
				::SendMessage(lb, LB_SETTOPINDEX, n - half , 0);
		}
	}
}

// Performs a double-buffered paint operation to avoid flicker
void ListBoxX::Paint(HDC hDC) {
	Point extent = GetClientExtent();
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, extent.x, extent.y);
	HDC bitmapDC = ::CreateCompatibleDC(hDC);
	HBITMAP hBitmapOld = SelectBitmap(bitmapDC, hBitmap);
	// The list background is mainly erased during painting, but can be a small
	// unpainted area when at the end of a non-integrally sized list with a
	// vertical scroll bar
	RECT rc = { 0, 0, extent.x, extent.y };
	::FillRect(bitmapDC, &rc, reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
	// Paint the entire client area and vertical scrollbar
	::SendMessage(lb, WM_PRINT, reinterpret_cast<WPARAM>(bitmapDC), PRF_CLIENT|PRF_NONCLIENT);
	::BitBlt(hDC, 0, 0, extent.x, extent.y, bitmapDC, 0, 0, SRCCOPY);
	// Select a stock brush to prevent warnings from BoundsChecker
	::SelectObject(bitmapDC, GetStockFont(WHITE_BRUSH));
	SelectBitmap(bitmapDC, hBitmapOld);
	::DeleteDC(bitmapDC);
	::DeleteObject(hBitmap);
}

LRESULT PASCAL ListBoxX::ControlWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	try {
		switch (uMsg) {
		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT: {
				PAINTSTRUCT ps;
				HDC hDC = ::BeginPaint(hWnd, &ps);
				ListBoxX *lbx = reinterpret_cast<ListBoxX *>(PointerFromWindow(::GetParent(hWnd)));
				if (lbx)
					lbx->Paint(hDC);
				::EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_MOUSEACTIVATE:
			// This prevents the view activating when the scrollbar is clicked
			return MA_NOACTIVATE;

		case WM_LBUTTONDOWN: {
				// We must take control of selection to prevent the ListBox activating
				// the popup
				LRESULT lResult = ::SendMessage(hWnd, LB_ITEMFROMPOINT, 0, lParam);
				int item = LOWORD(lResult);
				if (HIWORD(lResult) == 0 && item >= 0) {
					::SendMessage(hWnd, LB_SETCURSEL, item, 0);
				}
			}
			return 0;

		case WM_LBUTTONUP:
			return 0;

		case WM_LBUTTONDBLCLK: {
				ListBoxX *lbx = reinterpret_cast<ListBoxX *>(PointerFromWindow(::GetParent(hWnd)));
				if (lbx) {
					lbx->OnDoubleClick();
				}
			}
			return 0;
		}

		WNDPROC prevWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (prevWndProc) {
			return ::CallWindowProc(prevWndProc, hWnd, uMsg, wParam, lParam);
		} else {
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	} catch (...) {
	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT ListBoxX::WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	switch (iMessage) {
	case WM_CREATE: {
			HINSTANCE hinstanceParent = GetWindowInstance(reinterpret_cast<HWND>(parent->GetID()));
			// Note that LBS_NOINTEGRALHEIGHT is specified to fix cosmetic issue when resizing the list
			// but has useful side effect of speeding up list population significantly
			lb = ::CreateWindowEx(
				0, TEXT("listbox"), TEXT(""),
				WS_CHILD | WS_VSCROLL | WS_VISIBLE |
				LBS_OWNERDRAWFIXED | LBS_NODATA | LBS_NOINTEGRALHEIGHT,
				0, 0, 150,80, hWnd,
				reinterpret_cast<HMENU>(ctrlID),
				hinstanceParent,
				0);
			WNDPROC prevWndProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(lb, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ControlWndProc)));
			::SetWindowLongPtr(lb, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(prevWndProc));
		}
		break;

	case WM_SIZE:
		if (lb) {
			SetRedraw(false);
			::SetWindowPos(lb, 0, 0,0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE);
			// Ensure the selection remains visible
			CentreItem(GetSelection());
			SetRedraw(true);
		}
		break;

	case WM_PAINT: {
			PAINTSTRUCT ps;
			::BeginPaint(hWnd, &ps);
			::EndPaint(hWnd, &ps);
		}
		break;

	case WM_COMMAND:
		// This is not actually needed now - the registered double click action is used
		// directly to action a choice from the list.
		::SendMessage(reinterpret_cast<HWND>(parent->GetID()), iMessage, wParam, lParam);
		break;

	case WM_MEASUREITEM: {
			MEASUREITEMSTRUCT *pMeasureItem = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
			pMeasureItem->itemHeight = static_cast<unsigned int>(ItemHeight());
		}
		break;

	case WM_DRAWITEM:
		Draw(reinterpret_cast<DRAWITEMSTRUCT *>(lParam));
		break;

	case WM_DESTROY:
		lb = 0;
		::SetWindowLong(hWnd, 0, 0);
		return ::DefWindowProc(hWnd, iMessage, wParam, lParam);

	case WM_ERASEBKGND:
		// To reduce flicker we can elide background erasure since this window is
		// completely covered by its child.
		return TRUE;

	case WM_GETMINMAXINFO: {
			MINMAXINFO *minMax = reinterpret_cast<MINMAXINFO*>(lParam);
			minMax->ptMaxTrackSize = MaxTrackSize();
			minMax->ptMinTrackSize = MinTrackSize();
		}
		break;

	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE;

	case WM_NCHITTEST:
		return NcHitTest(wParam, lParam);

	case WM_NCLBUTTONDOWN:
		// We have to implement our own window resizing because the DefWindowProc
		// implementation insists on activating the resized window
		StartResize(wParam);
		return 0;

	case WM_MOUSEMOVE: {
			if (resizeHit == 0) {
				return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
			} else {
				ResizeToCursor();
			}
		}
		break;

	case WM_LBUTTONUP:
	case WM_CANCELMODE:
		if (resizeHit != 0) {
			resizeHit = 0;
			::ReleaseCapture();
		}
		return ::DefWindowProc(hWnd, iMessage, wParam, lParam);

	default:
		return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
	}

	return 0;
}

LRESULT PASCAL ListBoxX::StaticWndProc(
    HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	if (iMessage == WM_CREATE) {
		CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
		SetWindowPointer(hWnd, pCreate->lpCreateParams);
	}
	// Find C++ object associated with window.
	ListBoxX *lbx = reinterpret_cast<ListBoxX *>(PointerFromWindow(hWnd));
	if (lbx) {
		return lbx->WndProc(hWnd, iMessage, wParam, lParam);
	} else {
		return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
	}
}

static bool ListBoxX_Register() {
	WNDCLASSEX wndclassc;
	wndclassc.cbSize = sizeof(wndclassc);
	// We need CS_HREDRAW and CS_VREDRAW because of the ellipsis that might be drawn for
	// truncated items in the list and the appearance/disappearance of the vertical scroll bar.
	// The list repaint is double-buffered to avoid the flicker this would otherwise cause.
	wndclassc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wndclassc.cbClsExtra = 0;
	wndclassc.cbWndExtra = sizeof(ListBoxX *);
	wndclassc.hInstance = hinstPlatformRes;
	wndclassc.hIcon = NULL;
	wndclassc.hbrBackground = NULL;
	wndclassc.lpszMenuName = NULL;
	wndclassc.lpfnWndProc = ListBoxX::StaticWndProc;
	wndclassc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wndclassc.lpszClassName = ListBoxX_ClassName;
	wndclassc.hIconSm = 0;

	return ::RegisterClassEx(&wndclassc) != 0;
}

bool ListBoxX_Unregister() {
	return ::UnregisterClass(ListBoxX_ClassName, hinstPlatformRes) != 0;
}

Menu::Menu() : mid(0) {
}

void Menu::CreatePopUp() {
	Destroy();
	mid = ::CreatePopupMenu();
}

void Menu::Destroy() {
	if (mid)
		::DestroyMenu(reinterpret_cast<HMENU>(mid));
	mid = 0;
}

void Menu::Show(Point pt, Window &w) {
	::TrackPopupMenu(reinterpret_cast<HMENU>(mid),
		0, pt.x - 4, pt.y, 0,
		reinterpret_cast<HWND>(w.GetID()), NULL);
	Destroy();
}

static bool initialisedET = false;
static bool usePerformanceCounter = false;
static LARGE_INTEGER frequency;

ElapsedTime::ElapsedTime() {
	if (!initialisedET) {
		usePerformanceCounter = ::QueryPerformanceFrequency(&frequency) != 0;
		initialisedET = true;
	}
	if (usePerformanceCounter) {
		LARGE_INTEGER timeVal;
		::QueryPerformanceCounter(&timeVal);
		bigBit = timeVal.HighPart;
		littleBit = timeVal.LowPart;
	} else {
		bigBit = clock();
	}
}

double ElapsedTime::Duration(bool reset) {
	double result;
	long endBigBit;
	long endLittleBit;

	if (usePerformanceCounter) {
		LARGE_INTEGER lEnd;
		::QueryPerformanceCounter(&lEnd);
		endBigBit = lEnd.HighPart;
		endLittleBit = lEnd.LowPart;
		LARGE_INTEGER lBegin;
		lBegin.HighPart = bigBit;
		lBegin.LowPart = littleBit;
		double elapsed = lEnd.QuadPart - lBegin.QuadPart;
		result = elapsed / static_cast<double>(frequency.QuadPart);
	} else {
		endBigBit = clock();
		endLittleBit = 0;
		double elapsed = endBigBit - bigBit;
		result = elapsed / CLOCKS_PER_SEC;
	}
	if (reset) {
		bigBit = endBigBit;
		littleBit = endLittleBit;
	}
	return result;
}

class DynamicLibraryImpl : public DynamicLibrary {
protected:
	HMODULE h;
public:
	DynamicLibraryImpl(const char *modulePath) {
		h = ::LoadLibraryA(modulePath);
	}

	virtual ~DynamicLibraryImpl() {
		if (h != NULL)
			::FreeLibrary(h);
	}

	// Use GetProcAddress to get a pointer to the relevant function.
	virtual Function FindFunction(const char *name) {
		if (h != NULL) {
			// C++ standard doesn't like casts betwen function pointers and void pointers so use a union
			union {
				FARPROC fp;
				Function f;
			} fnConv;
			fnConv.fp = ::GetProcAddress(h, name);
			return fnConv.f;
		} else
			return NULL;
	}

	virtual bool IsValid() {
		return h != NULL;
	}
};

DynamicLibrary *DynamicLibrary::Load(const char *modulePath) {
	return static_cast<DynamicLibrary *>(new DynamicLibraryImpl(modulePath));
}

ColourDesired Platform::Chrome() {
	return ::GetSysColor(COLOR_3DFACE);
}

ColourDesired Platform::ChromeHighlight() {
	return ::GetSysColor(COLOR_3DHIGHLIGHT);
}

const char *Platform::DefaultFont() {
	return "Verdana";
}

int Platform::DefaultFontSize() {
	return 8;
}

unsigned int Platform::DoubleClickTime() {
	return ::GetDoubleClickTime();
}

bool Platform::MouseButtonBounce() {
	return false;
}

void Platform::DebugDisplay(const char *s) {
	::OutputDebugStringA(s);
}

bool Platform::IsKeyDown(int key) {
	return (::GetKeyState(key) & 0x80000000) != 0;
}

long Platform::SendScintilla(WindowID w, unsigned int msg, unsigned long wParam, long lParam) {
	return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam, lParam);
}

long Platform::SendScintillaPointer(WindowID w, unsigned int msg, unsigned long wParam, void *lParam) {
	return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam,
		reinterpret_cast<LPARAM>(lParam));
}

bool Platform::IsDBCSLeadByte(int codePage, char ch) {
	return ::IsDBCSLeadByteEx(codePage, ch) != 0;
}

int Platform::DBCSCharLength(int codePage, const char *s) {
	return (::IsDBCSLeadByteEx(codePage, s[0]) != 0) ? 2 : 1;
}

int Platform::DBCSCharMaxLength() {
	return 2;
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
	vsprintf(buffer,format,pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}
#else
void Platform::DebugPrintf(const char *, ...) {
}
#endif

static bool assertionPopUps = true;

bool Platform::ShowAssertionPopUps(bool assertionPopUps_) {
	bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) {
	char buffer[2000];
	sprintf(buffer, "Assertion [%s] failed at %s %d", c, file, line);
	if (assertionPopUps) {
		int idButton = ::MessageBoxA(0, buffer, "Assertion failure",
			MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);
		if (idButton == IDRETRY) {
			::DebugBreak();
		} else if (idButton == IDIGNORE) {
			// all OK
		} else {
			abort();
		}
	} else {
		strcat(buffer, "\r\n");
		Platform::DebugDisplay(buffer);
		::DebugBreak();
		abort();
	}
}

int Platform::Clamp(int val, int minVal, int maxVal) {
	if (val > maxVal)
		val = maxVal;
	if (val < minVal)
		val = minVal;
	return val;
}

void Platform_Initialise(void *hInstance) {
	OSVERSIONINFO osv = {sizeof(OSVERSIONINFO),0,0,0,0,TEXT("")};
	::GetVersionEx(&osv);
	onNT = osv.dwPlatformId == VER_PLATFORM_WIN32_NT;
	::InitializeCriticalSection(&crPlatformLock);
	hinstPlatformRes = reinterpret_cast<HINSTANCE>(hInstance);
	// This may be called from DllMain, in which case the call to LoadLibrary
	// is bad because it can upset the DLL load order.
	if (!hDLLImage) {
		hDLLImage = ::LoadLibrary(TEXT("Msimg32"));
	}
	if (hDLLImage) {
		AlphaBlendFn = (AlphaBlendSig)::GetProcAddress(hDLLImage, "AlphaBlend");
	}
	ListBoxX_Register();
}

void Platform_Finalise() {
	ListBoxX_Unregister();
	::DeleteCriticalSection(&crPlatformLock);
}
