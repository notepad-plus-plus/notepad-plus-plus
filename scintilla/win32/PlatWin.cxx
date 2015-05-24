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
#include <limits.h>
#include <math.h>

#include <vector>
#include <map>

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#undef WINVER
#define WINVER 0x0500
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <windowsx.h>

#if defined(NTDDI_WIN7) && !defined(DISABLE_D2D)
#define USE_D2D 1
#endif

#if defined(USE_D2D)
#include <d2d1.h>
#include <dwrite.h>
#endif

#include "Platform.h"
#include "UniConversion.h"
#include "XPM.h"
#include "FontQuality.h"

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

extern UINT CodePageFromCharSet(DWORD characterSet, UINT documentCodePage);

// Declarations needed for functions dynamically loaded as not available on all Windows versions.
typedef BOOL (WINAPI *AlphaBlendSig)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
typedef HMONITOR (WINAPI *MonitorFromPointSig)(POINT, DWORD);
typedef HMONITOR (WINAPI *MonitorFromRectSig)(LPCRECT, DWORD);
typedef BOOL (WINAPI *GetMonitorInfoSig)(HMONITOR, LPMONITORINFO);

static CRITICAL_SECTION crPlatformLock;
static HINSTANCE hinstPlatformRes = 0;
static bool onNT = false;

static HMODULE hDLLImage = 0;
static AlphaBlendSig AlphaBlendFn = 0;

static HMODULE hDLLUser32 = 0;
static HMONITOR (WINAPI *MonitorFromPointFn)(POINT, DWORD) = 0;
static HMONITOR (WINAPI *MonitorFromRectFn)(LPCRECT, DWORD) = 0;
static BOOL (WINAPI *GetMonitorInfoFn)(HMONITOR, LPMONITORINFO) = 0;

static HCURSOR reverseArrowCursor = NULL;

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
	RECT rc = {static_cast<LONG>(prc.left), static_cast<LONG>(prc.top),
		static_cast<LONG>(prc.right), static_cast<LONG>(prc.bottom)};
	return rc;
}

#if defined(USE_D2D)
IDWriteFactory *pIDWriteFactory = 0;
ID2D1Factory *pD2DFactory = 0;

bool LoadD2D() {
	static bool triedLoadingD2D = false;
	if (!triedLoadingD2D) {
		typedef HRESULT (WINAPI *D2D1CFSig)(D2D1_FACTORY_TYPE factoryType, REFIID riid,
			CONST D2D1_FACTORY_OPTIONS *pFactoryOptions, IUnknown **factory);
		typedef HRESULT (WINAPI *DWriteCFSig)(DWRITE_FACTORY_TYPE factoryType, REFIID iid,
			IUnknown **factory);

		HMODULE hDLLD2D = ::LoadLibraryEx(TEXT("D2D1.DLL"), 0, 0x00000800 /*LOAD_LIBRARY_SEARCH_SYSTEM32*/);
		if (hDLLD2D) {
			D2D1CFSig fnD2DCF = (D2D1CFSig)::GetProcAddress(hDLLD2D, "D2D1CreateFactory");
			if (fnD2DCF) {
				// A single threaded factory as Scintilla always draw on the GUI thread
				fnD2DCF(D2D1_FACTORY_TYPE_SINGLE_THREADED,
					__uuidof(ID2D1Factory),
					0,
					reinterpret_cast<IUnknown**>(&pD2DFactory));
			}
		}
		HMODULE hDLLDWrite = ::LoadLibraryEx(TEXT("DWRITE.DLL"), 0, 0x00000800 /*LOAD_LIBRARY_SEARCH_SYSTEM32*/);
		if (hDLLDWrite) {
			DWriteCFSig fnDWCF = (DWriteCFSig)::GetProcAddress(hDLLDWrite, "DWriteCreateFactory");
			if (fnDWCF) {
				fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
					__uuidof(IDWriteFactory),
					reinterpret_cast<IUnknown**>(&pIDWriteFactory));
			}
		}
	}
	triedLoadingD2D = true;
	return pIDWriteFactory && pD2DFactory;
}
#endif

struct FormatAndMetrics {
	int technology;
	HFONT hfont;
#if defined(USE_D2D)
	IDWriteTextFormat *pTextFormat;
#endif
	int extraFontFlag;
	int characterSet;
	FLOAT yAscent;
	FLOAT yDescent;
	FLOAT yInternalLeading;
	FormatAndMetrics(HFONT hfont_, int extraFontFlag_, int characterSet_) :
		technology(SCWIN_TECH_GDI), hfont(hfont_),
#if defined(USE_D2D)
		pTextFormat(0),
#endif
		extraFontFlag(extraFontFlag_), characterSet(characterSet_), yAscent(2), yDescent(1), yInternalLeading(0) {
	}
#if defined(USE_D2D)
	FormatAndMetrics(IDWriteTextFormat *pTextFormat_,
	        int extraFontFlag_,
	        int characterSet_,
	        FLOAT yAscent_,
	        FLOAT yDescent_,
	        FLOAT yInternalLeading_) :
		technology(SCWIN_TECH_DIRECTWRITE),
		hfont(0),
		pTextFormat(pTextFormat_),
		extraFontFlag(extraFontFlag_),
		characterSet(characterSet_),
		yAscent(yAscent_),
		yDescent(yDescent_),
		yInternalLeading(yInternalLeading_) {
	}
#endif
	~FormatAndMetrics() {
		if (hfont)
			::DeleteObject(hfont);
#if defined(USE_D2D)
		if (pTextFormat)
			pTextFormat->Release();
		pTextFormat = 0;
#endif
		extraFontFlag = 0;
		characterSet = 0;
		yAscent = 2;
		yDescent = 1;
		yInternalLeading = 0;
	}
	HFONT HFont();
};

HFONT FormatAndMetrics::HFont() {
	LOGFONTW lf;
	memset(&lf, 0, sizeof(lf));
#if defined(USE_D2D)
	if (technology == SCWIN_TECH_GDI) {
		if (0 == ::GetObjectW(hfont, sizeof(lf), &lf)) {
			return 0;
		}
	} else {
		HRESULT hr = pTextFormat->GetFontFamilyName(lf.lfFaceName, LF_FACESIZE);
		if (!SUCCEEDED(hr)) {
			return 0;
		}
		lf.lfWeight = pTextFormat->GetFontWeight();
		lf.lfItalic = pTextFormat->GetFontStyle() == DWRITE_FONT_STYLE_ITALIC;
		lf.lfHeight = -static_cast<int>(pTextFormat->GetFontSize());
	}
#else
	if (0 == ::GetObjectW(hfont, sizeof(lf), &lf)) {
		return 0;
	}
#endif
	return ::CreateFontIndirectW(&lf);
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

#if defined(USE_D2D)
static D2D1_TEXT_ANTIALIAS_MODE DWriteMapFontQuality(int extraFontFlag) {
	switch (extraFontFlag & SC_EFF_QUALITY_MASK) {

		case SC_EFF_QUALITY_NON_ANTIALIASED:
			return D2D1_TEXT_ANTIALIAS_MODE_ALIASED;

		case SC_EFF_QUALITY_ANTIALIASED:
			return D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE;

		case SC_EFF_QUALITY_LCD_OPTIMIZED:
			return D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;

		default:
			return D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
	}
}
#endif

static void SetLogFont(LOGFONTA &lf, const char *faceName, int characterSet, float size, int weight, bool italic, int extraFontFlag) {
	memset(&lf, 0, sizeof(lf));
	// The negative is to allow for leading
	lf.lfHeight = -(abs(static_cast<int>(size + 0.5)));
	lf.lfWeight = weight;
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
static int HashFont(const FontParameters &fp) {
	return
		static_cast<int>(fp.size) ^
		(fp.characterSet << 10) ^
		((fp.extraFontFlag & SC_EFF_QUALITY_MASK) << 9) ^
		((fp.weight/100) << 12) ^
		(fp.italic ? 0x20000000 : 0) ^
		(fp.technology << 15) ^
		fp.faceName[0];
}

class FontCached : Font {
	FontCached *next;
	int usage;
	float size;
	LOGFONTA lf;
	int technology;
	int hash;
	FontCached(const FontParameters &fp);
	~FontCached() {}
	bool SameAs(const FontParameters &fp);
	virtual void Release();

	static FontCached *first;
public:
	static FontID FindOrCreate(const FontParameters &fp);
	static void ReleaseId(FontID fid_);
};

FontCached *FontCached::first = 0;

FontCached::FontCached(const FontParameters &fp) :
	next(0), usage(0), size(1.0), hash(0) {
	SetLogFont(lf, fp.faceName, fp.characterSet, fp.size, fp.weight, fp.italic, fp.extraFontFlag);
	technology = fp.technology;
	hash = HashFont(fp);
	fid = 0;
	if (technology == SCWIN_TECH_GDI) {
		HFONT hfont = ::CreateFontIndirectA(&lf);
		fid = reinterpret_cast<void *>(new FormatAndMetrics(hfont, fp.extraFontFlag, fp.characterSet));
	} else {
#if defined(USE_D2D)
		IDWriteTextFormat *pTextFormat;
		const int faceSize = 200;
		WCHAR wszFace[faceSize];
		UTF16FromUTF8(fp.faceName, static_cast<unsigned int>(strlen(fp.faceName))+1, wszFace, faceSize);
		FLOAT fHeight = fp.size;
		DWRITE_FONT_STYLE style = fp.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
		HRESULT hr = pIDWriteFactory->CreateTextFormat(wszFace, NULL,
			static_cast<DWRITE_FONT_WEIGHT>(fp.weight),
			style,
			DWRITE_FONT_STRETCH_NORMAL, fHeight, L"en-us", &pTextFormat);
		if (SUCCEEDED(hr)) {
			pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			const int maxLines = 2;
			DWRITE_LINE_METRICS lineMetrics[maxLines];
			UINT32 lineCount = 0;
			FLOAT yAscent = 1.0f;
			FLOAT yDescent = 1.0f;
			FLOAT yInternalLeading = 0.0f;
			IDWriteTextLayout *pTextLayout = 0;
			hr = pIDWriteFactory->CreateTextLayout(L"X", 1, pTextFormat,
					100.0f, 100.0f, &pTextLayout);
			if (SUCCEEDED(hr)) {
				hr = pTextLayout->GetLineMetrics(lineMetrics, maxLines, &lineCount);
				if (SUCCEEDED(hr)) {
					yAscent = lineMetrics[0].baseline;
					yDescent = lineMetrics[0].height - lineMetrics[0].baseline;

					FLOAT emHeight;
					hr = pTextLayout->GetFontSize(0, &emHeight);
					if (SUCCEEDED(hr)) {
						yInternalLeading = lineMetrics[0].height - emHeight;
					}
				}
				pTextLayout->Release();
				pTextFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, lineMetrics[0].height, lineMetrics[0].baseline);
			}
			fid = reinterpret_cast<void *>(new FormatAndMetrics(pTextFormat, fp.extraFontFlag, fp.characterSet, yAscent, yDescent, yInternalLeading));
		}
#endif
	}
	usage = 1;
}

bool FontCached::SameAs(const FontParameters &fp) {
	return
		(size == fp.size) &&
		(lf.lfWeight == fp.weight) &&
		(lf.lfItalic == static_cast<BYTE>(fp.italic ? 1 : 0)) &&
		(lf.lfCharSet == fp.characterSet) &&
		(lf.lfQuality == Win32MapFontQuality(fp.extraFontFlag)) &&
		(technology == fp.technology) &&
		0 == strcmp(lf.lfFaceName,fp.faceName);
}

void FontCached::Release() {
	delete reinterpret_cast<FormatAndMetrics *>(fid);
	fid = 0;
}

FontID FontCached::FindOrCreate(const FontParameters &fp) {
	FontID ret = 0;
	::EnterCriticalSection(&crPlatformLock);
	int hashFind = HashFont(fp);
	for (FontCached *cur=first; cur; cur=cur->next) {
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

void Font::Create(const FontParameters &fp) {
	Release();
	if (fp.faceName)
		fid = FontCached::FindOrCreate(fp);
}

void Font::Release() {
	if (fid)
		FontCached::ReleaseId(fid);
	fid = 0;
}

// Buffer to hold strings and string position arrays without always allocating on heap.
// May sometimes have string too long to allocate on stack. So use a fixed stack-allocated buffer
// when less than safe size otherwise allocate on heap and free automatically.
template<typename T, int lengthStandard>
class VarBuffer {
	T bufferStandard[lengthStandard];
	// Private so VarBuffer objects can not be copied
	VarBuffer(const VarBuffer &);
	VarBuffer &operator=(const VarBuffer &);
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
typedef VarBuffer<XYPOSITION, stackBufferLength> TextPositions;

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class SurfaceGDI : public Surface {
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
	int maxWidthMeasure;
	int maxLenText;

	int codePage;
	// If 9x OS and current code page is same as ANSI code page.
	bool win9xACPSame;

	void BrushColor(ColourDesired back);
	void SetFont(Font &font_);

	// Private so SurfaceGDI objects can not be copied
	SurfaceGDI(const SurfaceGDI &);
	SurfaceGDI &operator=(const SurfaceGDI &);
public:
	SurfaceGDI();
	virtual ~SurfaceGDI();

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

	void DrawTextCommon(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, UINT fuOptions);
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
	void SetDBCSMode(int codePage_);
};

#ifdef SCI_NAMESPACE
} //namespace Scintilla
#endif

SurfaceGDI::SurfaceGDI() :
	unicodeMode(false),
	hdc(0), 	hdcOwned(false),
	pen(0), 	penOld(0),
	brush(0), brushOld(0),
	font(0), 	fontOld(0),
	bitmap(0), bitmapOld(0) {
	// Windows 9x has only a 16 bit coordinate system so break after 30000 pixels
	maxWidthMeasure = IsNT() ? INT_MAX : 30000;
	// There appears to be a 16 bit string length limit in GDI on NT and a limit of
	// 8192 characters on Windows 95.
	maxLenText = IsNT() ? 65535 : 8192;

	codePage = 0;
	win9xACPSame = false;
}

SurfaceGDI::~SurfaceGDI() {
	Release();
}

void SurfaceGDI::Release() {
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
	if (hdcOwned) {
		::DeleteDC(reinterpret_cast<HDC>(hdc));
		hdc = 0;
		hdcOwned = false;
	}
}

bool SurfaceGDI::Initialised() {
	return hdc != 0;
}

void SurfaceGDI::Init(WindowID) {
	Release();
	hdc = ::CreateCompatibleDC(NULL);
	hdcOwned = true;
	::SetTextAlign(reinterpret_cast<HDC>(hdc), TA_BASELINE);
}

void SurfaceGDI::Init(SurfaceID sid, WindowID) {
	Release();
	hdc = reinterpret_cast<HDC>(sid);
	::SetTextAlign(reinterpret_cast<HDC>(hdc), TA_BASELINE);
}

void SurfaceGDI::InitPixMap(int width, int height, Surface *surface_, WindowID) {
	Release();
	hdc = ::CreateCompatibleDC(static_cast<SurfaceGDI *>(surface_)->hdc);
	hdcOwned = true;
	bitmap = ::CreateCompatibleBitmap(static_cast<SurfaceGDI *>(surface_)->hdc, width, height);
	bitmapOld = static_cast<HBITMAP>(::SelectObject(hdc, bitmap));
	::SetTextAlign(reinterpret_cast<HDC>(hdc), TA_BASELINE);
}

void SurfaceGDI::PenColour(ColourDesired fore) {
	if (pen) {
		::SelectObject(hdc, penOld);
		::DeleteObject(pen);
		pen = 0;
		penOld = 0;
	}
	pen = ::CreatePen(0,1,fore.AsLong());
	penOld = static_cast<HPEN>(::SelectObject(reinterpret_cast<HDC>(hdc), pen));
}

void SurfaceGDI::BrushColor(ColourDesired back) {
	if (brush) {
		::SelectObject(hdc, brushOld);
		::DeleteObject(brush);
		brush = 0;
		brushOld = 0;
	}
	// Only ever want pure, non-dithered brushes
	ColourDesired colourNearest = ::GetNearestColor(hdc, back.AsLong());
	brush = ::CreateSolidBrush(colourNearest.AsLong());
	brushOld = static_cast<HBRUSH>(::SelectObject(hdc, brush));
}

void SurfaceGDI::SetFont(Font &font_) {
	if (font_.GetID() != font) {
		FormatAndMetrics *pfm = reinterpret_cast<FormatAndMetrics *>(font_.GetID());
		PLATFORM_ASSERT(pfm->technology == SCWIN_TECH_GDI);
		if (fontOld) {
			::SelectObject(hdc, pfm->hfont);
		} else {
			fontOld = static_cast<HFONT>(::SelectObject(hdc, pfm->hfont));
		}
		font = reinterpret_cast<HFONT>(pfm->hfont);
	}
}

int SurfaceGDI::LogPixelsY() {
	return ::GetDeviceCaps(hdc, LOGPIXELSY);
}

int SurfaceGDI::DeviceHeightFont(int points) {
	return ::MulDiv(points, LogPixelsY(), 72);
}

void SurfaceGDI::MoveTo(int x_, int y_) {
	::MoveToEx(hdc, x_, y_, 0);
}

void SurfaceGDI::LineTo(int x_, int y_) {
	::LineTo(hdc, x_, y_);
}

void SurfaceGDI::Polygon(Point *pts, int npts, ColourDesired fore, ColourDesired back) {
	PenColour(fore);
	BrushColor(back);
	std::vector<POINT> outline;
	for (int i=0;i<npts;i++) {
		POINT pt = {static_cast<LONG>(pts[i].x), static_cast<LONG>(pts[i].y)};
		outline.push_back(pt);
	}
	::Polygon(hdc, &outline[0], npts);
}

void SurfaceGDI::RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) {
	PenColour(fore);
	BrushColor(back);
	::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
}

void SurfaceGDI::FillRectangle(PRectangle rc, ColourDesired back) {
	// Using ExtTextOut rather than a FillRect ensures that no dithering occurs.
	// There is no need to allocate a brush either.
	RECT rcw = RectFromPRectangle(rc);
	::SetBkColor(hdc, back.AsLong());
	::ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE, &rcw, TEXT(""), 0, NULL);
}

void SurfaceGDI::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	HBRUSH br;
	if (static_cast<SurfaceGDI &>(surfacePattern).bitmap)
		br = ::CreatePatternBrush(static_cast<SurfaceGDI &>(surfacePattern).bitmap);
	else	// Something is wrong so display in red
		br = ::CreateSolidBrush(RGB(0xff, 0, 0));
	RECT rcw = RectFromPRectangle(rc);
	::FillRect(hdc, &rcw, br);
	::DeleteObject(br);
}

void SurfaceGDI::RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) {
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

void SurfaceGDI::AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int /* flags*/ ) {
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

		if (hbmMem) {
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
		}
		::DeleteDC(hMemDC);
	} else {
		BrushColor(outline);
		RECT rcw = RectFromPRectangle(rc);
		FrameRect(hdc, &rcw, brush);
	}
}

void SurfaceGDI::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	if (AlphaBlendFn && rc.Width() > 0) {
		HDC hMemDC = ::CreateCompatibleDC(reinterpret_cast<HDC>(hdc));
		if (rc.Width() > width)
			rc.left += static_cast<int>((rc.Width() - width) / 2);
		rc.right = rc.left + width;
		if (rc.Height() > height)
			rc.top += static_cast<int>((rc.Height() - height) / 2);
		rc.bottom = rc.top + height;

		BITMAPINFO bpih = {sizeof(BITMAPINFOHEADER), width, height, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
		unsigned char *image = 0;
		HBITMAP hbmMem = CreateDIBSection(reinterpret_cast<HDC>(hMemDC), &bpih,
			DIB_RGB_COLORS, reinterpret_cast<void **>(&image), NULL, 0);
		if (hbmMem) {
			HBITMAP hbmOld = SelectBitmap(hMemDC, hbmMem);

			for (int y=height-1; y>=0; y--) {
				for (int x=0; x<width; x++) {
					unsigned char *pixel = image + (y*width+x) * 4;
					unsigned char alpha = pixelsImage[3];
					// Input is RGBA, output is BGRA with premultiplied alpha
					pixel[2] = static_cast<unsigned char>((*pixelsImage++) * alpha / 255);
					pixel[1] = static_cast<unsigned char>((*pixelsImage++) * alpha / 255);
					pixel[0] = static_cast<unsigned char>((*pixelsImage++) * alpha / 255);
					pixel[3] = static_cast<unsigned char>(*pixelsImage++);
				}
			}

			BLENDFUNCTION merge = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

			AlphaBlendFn(reinterpret_cast<HDC>(hdc), rc.left, rc.top, rc.Width(), rc.Height(), hMemDC, 0, 0, width, height, merge);

			SelectBitmap(hMemDC, hbmOld);
			::DeleteObject(hbmMem);
		}
		::DeleteDC(hMemDC);

	}
}

void SurfaceGDI::Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) {
	PenColour(fore);
	BrushColor(back);
	::Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
}

void SurfaceGDI::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	::BitBlt(hdc,
		rc.left, rc.top, rc.Width(), rc.Height(),
		static_cast<SurfaceGDI &>(surfaceSource).hdc, from.x, from.y, SRCCOPY);
}

typedef VarBuffer<int, stackBufferLength> TextPositionsI;

void SurfaceGDI::DrawTextCommon(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, UINT fuOptions) {
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

void SurfaceGDI::DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore, ColourDesired back) {
	::SetTextColor(hdc, fore.AsLong());
	::SetBkColor(hdc, back.AsLong());
	DrawTextCommon(rc, font_, ybase, s, len, ETO_OPAQUE);
}

void SurfaceGDI::DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore, ColourDesired back) {
	::SetTextColor(hdc, fore.AsLong());
	::SetBkColor(hdc, back.AsLong());
	DrawTextCommon(rc, font_, ybase, s, len, ETO_OPAQUE | ETO_CLIPPED);
}

void SurfaceGDI::DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore) {
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

XYPOSITION SurfaceGDI::WidthText(Font &font_, const char *s, int len) {
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

void SurfaceGDI::MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions) {
	SetFont(font_);
	SIZE sz={0,0};
	int fit = 0;
	if (unicodeMode) {
		const TextWide tbuf(s, len, unicodeMode, codePage);
		TextPositionsI poses(tbuf.tlen);
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
			TextPositionsI poses(len);
			if (!::GetTextExtentExPointA(hdc, s, lenBlock, maxWidthMeasure, &fit, poses.buffer, &sz)) {
				// Eeek - a NULL DC or other foolishness could cause this.
				return;
			} else if (fit < lenBlock) {
				// For some reason, such as an incomplete DBCS character
				// Not all the positions are filled in so make them equal to end.
				if (fit == 0)
					poses.buffer[fit++] = 0;
				for (int i = fit;i<lenBlock;i++)
					poses.buffer[i] = poses.buffer[fit-1];
			}
			for (int i=0;i<lenBlock;i++)
				positions[i] = poses.buffer[i] + startOffset;
			startOffset = poses.buffer[lenBlock-1];
			len -= lenBlock;
			positions += lenBlock;
			s += lenBlock;
		}
	} else {
		// Support Asian string display in 9x English
		const TextWide tbuf(s, len, unicodeMode, codePage);
		TextPositionsI poses(tbuf.tlen);
		for (int widthSS=0; widthSS<tbuf.tlen; widthSS++) {
			::GetTextExtentPoint32W(hdc, tbuf.buffer, widthSS+1, &sz);
			poses.buffer[widthSS] = sz.cx;
		}

		int ui = 0;
		for (int i=0;i<len;) {
			if (Platform::IsDBCSLeadByte(codePage, s[i])) {
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

XYPOSITION SurfaceGDI::WidthChar(Font &font_, char ch) {
	SetFont(font_);
	SIZE sz;
	::GetTextExtentPoint32A(hdc, &ch, 1, &sz);
	return sz.cx;
}

XYPOSITION SurfaceGDI::Ascent(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmAscent;
}

XYPOSITION SurfaceGDI::Descent(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmDescent;
}

XYPOSITION SurfaceGDI::InternalLeading(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmInternalLeading;
}

XYPOSITION SurfaceGDI::ExternalLeading(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmExternalLeading;
}

XYPOSITION SurfaceGDI::Height(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmHeight;
}

XYPOSITION SurfaceGDI::AverageCharWidth(Font &font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return tm.tmAveCharWidth;
}

void SurfaceGDI::SetClip(PRectangle rc) {
	::IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
}

void SurfaceGDI::FlushCachedState() {
	pen = 0;
	brush = 0;
	font = 0;
}

void SurfaceGDI::SetUnicodeMode(bool unicodeMode_) {
	unicodeMode=unicodeMode_;
}

void SurfaceGDI::SetDBCSMode(int codePage_) {
	// No action on window as automatically handled by system.
	codePage = codePage_;
	win9xACPSame = !IsNT() && ((unsigned int)codePage == ::GetACP());
}

#if defined(USE_D2D)

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class SurfaceD2D : public Surface {
	bool unicodeMode;
	int x, y;

	int codePage;
	int codePageText;

	ID2D1RenderTarget *pRenderTarget;
	bool ownRenderTarget;
	int clipsActive;

	IDWriteTextFormat *pTextFormat;
	FLOAT yAscent;
	FLOAT yDescent;
	FLOAT yInternalLeading;

	ID2D1SolidColorBrush *pBrush;

	int logPixelsY;
	float dpiScaleX;
	float dpiScaleY;

	void SetFont(Font &font_);

	// Private so SurfaceD2D objects can not be copied
	SurfaceD2D(const SurfaceD2D &);
	SurfaceD2D &operator=(const SurfaceD2D &);
public:
	SurfaceD2D();
	virtual ~SurfaceD2D();

	void SetScale();
	void Init(WindowID wid);
	void Init(SurfaceID sid, WindowID wid);
	void InitPixMap(int width, int height, Surface *surface_, WindowID wid);

	void Release();
	bool Initialised();

	HRESULT FlushDrawing();

	void PenColour(ColourDesired fore);
	void D2DPenColour(ColourDesired fore, int alpha=255);
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

	void DrawTextCommon(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, UINT fuOptions);
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
	void SetDBCSMode(int codePage_);
};

#ifdef SCI_NAMESPACE
} //namespace Scintilla
#endif

SurfaceD2D::SurfaceD2D() :
	unicodeMode(false),
	x(0), y(0) {

	codePage = 0;
	codePageText = 0;

	pRenderTarget = NULL;
	ownRenderTarget = false;
	clipsActive = 0;

	// From selected font
	pTextFormat = NULL;
	yAscent = 2;
	yDescent = 1;
	yInternalLeading = 0;

	pBrush = NULL;

	logPixelsY = 72;
	dpiScaleX = 1.0;
	dpiScaleY = 1.0;
}

SurfaceD2D::~SurfaceD2D() {
	Release();
}

void SurfaceD2D::Release() {
	if (pBrush) {
		pBrush->Release();
		pBrush = 0;
	}
	if (pRenderTarget) {
		while (clipsActive) {
			pRenderTarget->PopAxisAlignedClip();
			clipsActive--;
		}
		if (ownRenderTarget) {
			pRenderTarget->Release();
		}
		pRenderTarget = 0;
	}
}

void SurfaceD2D::SetScale() {
	HDC hdcMeasure = ::CreateCompatibleDC(NULL);
	logPixelsY = ::GetDeviceCaps(hdcMeasure, LOGPIXELSY);
	dpiScaleX = ::GetDeviceCaps(hdcMeasure, LOGPIXELSX) / 96.0f;
	dpiScaleY = logPixelsY / 96.0f;
	::DeleteDC(hdcMeasure);
}

bool SurfaceD2D::Initialised() {
	return pRenderTarget != 0;
}

HRESULT SurfaceD2D::FlushDrawing() {
	return pRenderTarget->Flush();
}

void SurfaceD2D::Init(WindowID /* wid */) {
	Release();
	SetScale();
}

void SurfaceD2D::Init(SurfaceID sid, WindowID) {
	Release();
	SetScale();
	pRenderTarget = reinterpret_cast<ID2D1RenderTarget *>(sid);
}

void SurfaceD2D::InitPixMap(int width, int height, Surface *surface_, WindowID) {
	Release();
	SetScale();
	SurfaceD2D *psurfOther = static_cast<SurfaceD2D *>(surface_);
	ID2D1BitmapRenderTarget *pCompatibleRenderTarget = NULL;
	D2D1_SIZE_F desiredSize = D2D1::SizeF(width, height);
	D2D1_PIXEL_FORMAT desiredFormat = psurfOther->pRenderTarget->GetPixelFormat();
	desiredFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
	HRESULT hr = psurfOther->pRenderTarget->CreateCompatibleRenderTarget(
		&desiredSize, NULL, &desiredFormat, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &pCompatibleRenderTarget);
	if (SUCCEEDED(hr)) {
		pRenderTarget = pCompatibleRenderTarget;
		pRenderTarget->BeginDraw();
		ownRenderTarget = true;
	}
}

void SurfaceD2D::PenColour(ColourDesired fore) {
	D2DPenColour(fore);
}

void SurfaceD2D::D2DPenColour(ColourDesired fore, int alpha) {
	if (pRenderTarget) {
		D2D_COLOR_F col;
		col.r = (fore.AsLong() & 0xff) / 255.0;
		col.g = ((fore.AsLong() & 0xff00) >> 8) / 255.0;
		col.b = (fore.AsLong() >> 16) / 255.0;
		col.a = alpha / 255.0;
		if (pBrush) {
			pBrush->SetColor(col);
		} else {
			HRESULT hr = pRenderTarget->CreateSolidColorBrush(col, &pBrush);
			if (!SUCCEEDED(hr) && pBrush) {
				pBrush->Release();
				pBrush = 0;
			}
		}
	}
}

void SurfaceD2D::SetFont(Font &font_) {
	FormatAndMetrics *pfm = reinterpret_cast<FormatAndMetrics *>(font_.GetID());
	PLATFORM_ASSERT(pfm->technology == SCWIN_TECH_DIRECTWRITE);
	pTextFormat = pfm->pTextFormat;
	yAscent = pfm->yAscent;
	yDescent = pfm->yDescent;
	yInternalLeading = pfm->yInternalLeading;
	codePageText = codePage;
	if (pfm->characterSet) {
		codePageText = CodePageFromCharSet(pfm->characterSet, codePage);
	}
	if (pRenderTarget) {
		pRenderTarget->SetTextAntialiasMode(DWriteMapFontQuality(pfm->extraFontFlag));
	}
}

int SurfaceD2D::LogPixelsY() {
	return logPixelsY;
}

int SurfaceD2D::DeviceHeightFont(int points) {
	return ::MulDiv(points, LogPixelsY(), 72);
}

void SurfaceD2D::MoveTo(int x_, int y_) {
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

static int RoundFloat(float f) {
	return int(f+0.5);
}

void SurfaceD2D::LineTo(int x_, int y_) {
	if (pRenderTarget) {
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
			D2D1_RECT_F rectangle1 = D2D1::RectF(left, top, left+width, top+height);
			pRenderTarget->FillRectangle(&rectangle1, pBrush);
		} else if ((abs(xDiff) == abs(yDiff))) {
			// 45 degree slope
			pRenderTarget->DrawLine(D2D1::Point2F(x + 0.5, y + 0.5),
				D2D1::Point2F(x_ + 0.5 - xDelta, y_ + 0.5 - yDelta), pBrush);
		} else {
			// Line has a different slope so difficult to avoid last pixel
			pRenderTarget->DrawLine(D2D1::Point2F(x + 0.5, y + 0.5),
				D2D1::Point2F(x_ + 0.5, y_ + 0.5), pBrush);
		}
		x = x_;
		y = y_;
	}
}

void SurfaceD2D::Polygon(Point *pts, int npts, ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		ID2D1Factory *pFactory = 0;
		pRenderTarget->GetFactory(&pFactory);
		ID2D1PathGeometry *geometry=0;
		HRESULT hr = pFactory->CreatePathGeometry(&geometry);
		if (SUCCEEDED(hr)) {
			ID2D1GeometrySink *sink = 0;
			hr = geometry->Open(&sink);
			if (SUCCEEDED(hr)) {
				sink->BeginFigure(D2D1::Point2F(pts[0].x + 0.5f, pts[0].y + 0.5f), D2D1_FIGURE_BEGIN_FILLED);
				for (size_t i=1; i<static_cast<size_t>(npts); i++) {
					sink->AddLine(D2D1::Point2F(pts[i].x + 0.5f, pts[i].y + 0.5f));
				}
				sink->EndFigure(D2D1_FIGURE_END_CLOSED);
				sink->Close();
				sink->Release();

				D2DPenColour(back);
				pRenderTarget->FillGeometry(geometry,pBrush);
				D2DPenColour(fore);
				pRenderTarget->DrawGeometry(geometry,pBrush);
			}

			geometry->Release();
		}
	}
}

void SurfaceD2D::RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		D2D1_RECT_F rectangle1 = D2D1::RectF(RoundFloat(rc.left) + 0.5, rc.top+0.5, RoundFloat(rc.right) - 0.5, rc.bottom-0.5);
		D2DPenColour(back);
		pRenderTarget->FillRectangle(&rectangle1, pBrush);
		D2DPenColour(fore);
		pRenderTarget->DrawRectangle(&rectangle1, pBrush);
	}
}

void SurfaceD2D::FillRectangle(PRectangle rc, ColourDesired back) {
	if (pRenderTarget) {
		D2DPenColour(back);
        D2D1_RECT_F rectangle1 = D2D1::RectF(RoundFloat(rc.left), rc.top, RoundFloat(rc.right), rc.bottom);
        pRenderTarget->FillRectangle(&rectangle1, pBrush);
	}
}

void SurfaceD2D::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceD2D &surfOther = static_cast<SurfaceD2D &>(surfacePattern);
	surfOther.FlushDrawing();
	ID2D1Bitmap *pBitmap = NULL;
	ID2D1BitmapRenderTarget *pCompatibleRenderTarget = reinterpret_cast<ID2D1BitmapRenderTarget *>(
		surfOther.pRenderTarget);
	HRESULT hr = pCompatibleRenderTarget->GetBitmap(&pBitmap);
	if (SUCCEEDED(hr)) {
		ID2D1BitmapBrush *pBitmapBrush = NULL;
		D2D1_BITMAP_BRUSH_PROPERTIES brushProperties =
	        D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
		// Create the bitmap brush.
		hr = pRenderTarget->CreateBitmapBrush(pBitmap, brushProperties, &pBitmapBrush);
		pBitmap->Release();
		if (SUCCEEDED(hr)) {
			pRenderTarget->FillRectangle(
				D2D1::RectF(rc.left, rc.top, rc.right, rc.bottom),
				pBitmapBrush);
			pBitmapBrush->Release();
		}
	}
}

void SurfaceD2D::RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		D2D1_ROUNDED_RECT roundedRectFill = {
			D2D1::RectF(rc.left+1.0, rc.top+1.0, rc.right-1.0, rc.bottom-1.0),
			8, 8};
		D2DPenColour(back);
		pRenderTarget->FillRoundedRectangle(roundedRectFill, pBrush);

		D2D1_ROUNDED_RECT roundedRect = {
			D2D1::RectF(rc.left + 0.5, rc.top+0.5, rc.right - 0.5, rc.bottom-0.5),
			8, 8};
		D2DPenColour(fore);
		pRenderTarget->DrawRoundedRectangle(roundedRect, pBrush);
	}
}

void SurfaceD2D::AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int /* flags*/ ) {
	if (pRenderTarget) {
		if (cornerSize == 0) {
			// When corner size is zero, draw square rectangle to prevent blurry pixels at corners
			D2D1_RECT_F rectFill = D2D1::RectF(RoundFloat(rc.left) + 1.0, rc.top + 1.0, RoundFloat(rc.right) - 1.0, rc.bottom - 1.0);
			D2DPenColour(fill, alphaFill);
			pRenderTarget->FillRectangle(rectFill, pBrush);

			D2D1_RECT_F rectOutline = D2D1::RectF(RoundFloat(rc.left) + 0.5, rc.top + 0.5, RoundFloat(rc.right) - 0.5, rc.bottom - 0.5);
			D2DPenColour(outline, alphaOutline);
			pRenderTarget->DrawRectangle(rectOutline, pBrush);
		} else {
			const float cornerSizeF = static_cast<float>(cornerSize);
			D2D1_ROUNDED_RECT roundedRectFill = {
				D2D1::RectF(RoundFloat(rc.left) + 1.0, rc.top + 1.0, RoundFloat(rc.right) - 1.0, rc.bottom - 1.0),
				cornerSizeF, cornerSizeF};
			D2DPenColour(fill, alphaFill);
			pRenderTarget->FillRoundedRectangle(roundedRectFill, pBrush);

			D2D1_ROUNDED_RECT roundedRect = {
				D2D1::RectF(RoundFloat(rc.left) + 0.5, rc.top + 0.5, RoundFloat(rc.right) - 0.5, rc.bottom - 0.5),
				cornerSizeF, cornerSizeF};
			D2DPenColour(outline, alphaOutline);
			pRenderTarget->DrawRoundedRectangle(roundedRect, pBrush);
		}
	}
}

void SurfaceD2D::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	if (pRenderTarget) {
		if (rc.Width() > width)
			rc.left += static_cast<int>((rc.Width() - width) / 2);
		rc.right = rc.left + width;
		if (rc.Height() > height)
			rc.top += static_cast<int>((rc.Height() - height) / 2);
		rc.bottom = rc.top + height;

		std::vector<unsigned char> image(height * width * 4);
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				unsigned char *pixel = &image[0] + (y*width+x) * 4;
				unsigned char alpha = pixelsImage[3];
				// Input is RGBA, output is BGRA with premultiplied alpha
				pixel[2] = (*pixelsImage++) * alpha / 255;
				pixel[1] = (*pixelsImage++) * alpha / 255;
				pixel[0] = (*pixelsImage++) * alpha / 255;
				pixel[3] = *pixelsImage++;
			}
		}

		ID2D1Bitmap *bitmap = 0;
		D2D1_SIZE_U size = D2D1::SizeU(width, height);
		D2D1_BITMAP_PROPERTIES props = {{DXGI_FORMAT_B8G8R8A8_UNORM,
		    D2D1_ALPHA_MODE_PREMULTIPLIED}, 72.0, 72.0};
		HRESULT hr = pRenderTarget->CreateBitmap(size, &image[0],
                  width * 4, &props, &bitmap);
		if (SUCCEEDED(hr)) {
			D2D1_RECT_F rcDestination = {rc.left, rc.top, rc.right, rc.bottom};
			pRenderTarget->DrawBitmap(bitmap, rcDestination);
			bitmap->Release();
		}
	}
}

void SurfaceD2D::Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		FLOAT radius = rc.Width() / 2.0f - 1.0f;
		D2D1_ELLIPSE ellipse = {
			D2D1::Point2F((rc.left + rc.right) / 2.0f, (rc.top + rc.bottom) / 2.0f),
			radius,radius};

		PenColour(back);
		pRenderTarget->FillEllipse(ellipse, pBrush);
		PenColour(fore);
		pRenderTarget->DrawEllipse(ellipse, pBrush);
	}
}

void SurfaceD2D::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceD2D &surfOther = static_cast<SurfaceD2D &>(surfaceSource);
	surfOther.FlushDrawing();
	ID2D1BitmapRenderTarget *pCompatibleRenderTarget = reinterpret_cast<ID2D1BitmapRenderTarget *>(
		surfOther.pRenderTarget);
	ID2D1Bitmap *pBitmap = NULL;
	HRESULT hr = pCompatibleRenderTarget->GetBitmap(&pBitmap);
	if (SUCCEEDED(hr)) {
		D2D1_RECT_F rcDestination = {rc.left, rc.top, rc.right, rc.bottom};
		D2D1_RECT_F rcSource = {from.x, from.y, from.x + rc.Width(), from.y + rc.Height()};
		pRenderTarget->DrawBitmap(pBitmap, rcDestination, 1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, rcSource);
		pRenderTarget->Flush();
		pBitmap->Release();
	}
}

void SurfaceD2D::DrawTextCommon(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, UINT fuOptions) {
	SetFont(font_);

	// Use Unicode calls
	const TextWide tbuf(s, len, unicodeMode, codePageText);
	if (pRenderTarget && pTextFormat && pBrush) {
		if (fuOptions & ETO_CLIPPED) {
			D2D1_RECT_F rcClip = {rc.left, rc.top, rc.right, rc.bottom};
			pRenderTarget->PushAxisAlignedClip(rcClip, D2D1_ANTIALIAS_MODE_ALIASED);
		}

		// Explicitly creating a text layout appears a little faster
		IDWriteTextLayout *pTextLayout;
		HRESULT hr = pIDWriteFactory->CreateTextLayout(tbuf.buffer, tbuf.tlen, pTextFormat,
				rc.Width(), rc.Height(), &pTextLayout);
		if (SUCCEEDED(hr)) {
			D2D1_POINT_2F origin = {rc.left, ybase-yAscent};
			pRenderTarget->DrawTextLayout(origin, pTextLayout, pBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
			pTextLayout->Release();
		}

		if (fuOptions & ETO_CLIPPED) {
			pRenderTarget->PopAxisAlignedClip();
		}
	}
}

void SurfaceD2D::DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		FillRectangle(rc, back);
		D2DPenColour(fore);
		DrawTextCommon(rc, font_, ybase, s, len, ETO_OPAQUE);
	}
}

void SurfaceD2D::DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore, ColourDesired back) {
	if (pRenderTarget) {
		FillRectangle(rc, back);
		D2DPenColour(fore);
		DrawTextCommon(rc, font_, ybase, s, len, ETO_OPAQUE | ETO_CLIPPED);
	}
}

void SurfaceD2D::DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len,
	ColourDesired fore) {
	// Avoid drawing spaces in transparent mode
	for (int i=0;i<len;i++) {
		if (s[i] != ' ') {
			if (pRenderTarget) {
				D2DPenColour(fore);
				DrawTextCommon(rc, font_, ybase, s, len, 0);
			}
			return;
		}
	}
}

XYPOSITION SurfaceD2D::WidthText(Font &font_, const char *s, int len) {
	FLOAT width = 1.0;
	SetFont(font_);
	const TextWide tbuf(s, len, unicodeMode, codePageText);
	if (pIDWriteFactory && pTextFormat) {
		// Create a layout
		IDWriteTextLayout *pTextLayout = 0;
		HRESULT hr = pIDWriteFactory->CreateTextLayout(tbuf.buffer, tbuf.tlen, pTextFormat, 1000.0, 1000.0, &pTextLayout);
		if (SUCCEEDED(hr)) {
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(pTextLayout->GetMetrics(&textMetrics)))
				width = textMetrics.widthIncludingTrailingWhitespace;
			pTextLayout->Release();
		}
	}
	return width;
}

void SurfaceD2D::MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions) {
	SetFont(font_);
	int fit = 0;
	const TextWide tbuf(s, len, unicodeMode, codePageText);
	TextPositions poses(tbuf.tlen);
	fit = tbuf.tlen;
	const int clusters = 1000;
	DWRITE_CLUSTER_METRICS clusterMetrics[clusters];
	UINT32 count = 0;
	if (pIDWriteFactory && pTextFormat) {
		SetFont(font_);
		// Create a layout
		IDWriteTextLayout *pTextLayout = 0;
		HRESULT hr = pIDWriteFactory->CreateTextLayout(tbuf.buffer, tbuf.tlen, pTextFormat, 10000.0, 1000.0, &pTextLayout);
		if (!SUCCEEDED(hr))
			return;
		// For now, assuming WCHAR == cluster
		if (!SUCCEEDED(pTextLayout->GetClusterMetrics(clusterMetrics, clusters, &count)))
			return;
		FLOAT position = 0.0f;
		size_t ti=0;
		for (size_t ci=0;ci<count;ci++) {
			position += clusterMetrics[ci].width;
			for (size_t inCluster=0; inCluster<clusterMetrics[ci].length; inCluster++) {
				//poses.buffer[ti++] = int(position + 0.5);
				poses.buffer[ti++] = position;
			}
		}
		PLATFORM_ASSERT(ti == static_cast<size_t>(tbuf.tlen));
		pTextLayout->Release();
	}
	if (unicodeMode) {
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
	} else if (codePageText == 0) {

		// One character per position
		PLATFORM_ASSERT(len == tbuf.tlen);
		for (size_t kk=0;kk<static_cast<size_t>(len);kk++) {
			positions[kk] = poses.buffer[kk];
		}

	} else {

		// May be more than one byte per position
		int ui = 0;
		for (int i=0;i<len;) {
			if (Platform::IsDBCSLeadByte(codePageText, s[i])) {
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

XYPOSITION SurfaceD2D::WidthChar(Font &font_, char ch) {
	FLOAT width = 1.0;
	SetFont(font_);
	if (pIDWriteFactory && pTextFormat) {
		// Create a layout
		IDWriteTextLayout *pTextLayout = 0;
		const WCHAR wch = ch;
		HRESULT hr = pIDWriteFactory->CreateTextLayout(&wch, 1, pTextFormat, 1000.0, 1000.0, &pTextLayout);
		if (SUCCEEDED(hr)) {
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(pTextLayout->GetMetrics(&textMetrics)))
				width = textMetrics.widthIncludingTrailingWhitespace;
			pTextLayout->Release();
		}
	}
	return width;
}

XYPOSITION SurfaceD2D::Ascent(Font &font_) {
	SetFont(font_);
	return ceil(yAscent);
}

XYPOSITION SurfaceD2D::Descent(Font &font_) {
	SetFont(font_);
	return ceil(yDescent);
}

XYPOSITION SurfaceD2D::InternalLeading(Font &font_) {
	SetFont(font_);
	return floor(yInternalLeading);
}

XYPOSITION SurfaceD2D::ExternalLeading(Font &) {
	// Not implemented, always return one
	return 1;
}

XYPOSITION SurfaceD2D::Height(Font &font_) {
	return Ascent(font_) + Descent(font_);
}

XYPOSITION SurfaceD2D::AverageCharWidth(Font &font_) {
	FLOAT width = 1.0;
	SetFont(font_);
	if (pIDWriteFactory && pTextFormat) {
		// Create a layout
		IDWriteTextLayout *pTextLayout = 0;
		const WCHAR wszAllAlpha[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		HRESULT hr = pIDWriteFactory->CreateTextLayout(wszAllAlpha, static_cast<UINT32>(wcslen(wszAllAlpha)),
			pTextFormat, 1000.0, 1000.0, &pTextLayout);
		if (SUCCEEDED(hr)) {
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(pTextLayout->GetMetrics(&textMetrics)))
				width = textMetrics.width / wcslen(wszAllAlpha);
			pTextLayout->Release();
		}
	}
	return width;
}

void SurfaceD2D::SetClip(PRectangle rc) {
	if (pRenderTarget) {
		D2D1_RECT_F rcClip = {rc.left, rc.top, rc.right, rc.bottom};
		pRenderTarget->PushAxisAlignedClip(rcClip, D2D1_ANTIALIAS_MODE_ALIASED);
		clipsActive++;
	}
}

void SurfaceD2D::FlushCachedState() {
}

void SurfaceD2D::SetUnicodeMode(bool unicodeMode_) {
	unicodeMode=unicodeMode_;
}

void SurfaceD2D::SetDBCSMode(int codePage_) {
	// No action on window as automatically handled by system.
	codePage = codePage_;
}
#endif

Surface *Surface::Allocate(int technology) {
#if defined(USE_D2D)
	if (technology == SCWIN_TECH_GDI)
		return new SurfaceGDI;
	else
		return new SurfaceD2D;
#else
	return new SurfaceGDI;
#endif
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

static RECT RectFromMonitor(HMONITOR hMonitor) {
	if (GetMonitorInfoFn) {
		MONITORINFO mi = {0};
		mi.cbSize = sizeof(mi);
		if (GetMonitorInfoFn(hMonitor, &mi)) {
			return mi.rcWork;
		}
	}
	RECT rc = {0, 0, 0, 0};
	if (::SystemParametersInfoA(SPI_GETWORKAREA, 0, &rc, 0) == 0) {
		rc.left = 0;
		rc.top = 0;
		rc.right = 0;
		rc.bottom = 0;
	}
	return rc;
}

void Window::SetPositionRelative(PRectangle rc, Window w) {
	LONG style = ::GetWindowLong(reinterpret_cast<HWND>(wid), GWL_STYLE);
	if (style & WS_POPUP) {
		POINT ptOther = {0, 0};
		::ClientToScreen(reinterpret_cast<HWND>(w.GetID()), &ptOther);
		rc.Move(ptOther.x, ptOther.y);

		RECT rcMonitor = RectFromPRectangle(rc);

		HMONITOR hMonitor = NULL;
		if (MonitorFromRectFn)
			hMonitor = MonitorFromRectFn(&rcMonitor, MONITOR_DEFAULTTONEAREST);
		// If hMonitor is NULL, that's just the main screen anyways.
		//::GetMonitorInfo(hMonitor, &mi);
		RECT rcWork = RectFromMonitor(hMonitor);

		if (rcWork.left < rcWork.right) {
			// Now clamp our desired rectangle to fit inside the work area
			// This way, the menu will fit wholly on one screen. An improvement even
			// if you don't have a second monitor on the left... Menu's appears half on
			// one screen and half on the other are just U.G.L.Y.!
			if (rc.right > rcWork.right)
				rc.Move(rcWork.right - rc.right, 0);
			if (rc.bottom > rcWork.bottom)
				rc.Move(0, rcWork.bottom - rc.bottom);
			if (rc.left < rcWork.left)
				rc.Move(rcWork.left - rc.left, 0);
			if (rc.top < rcWork.top)
				rc.Move(0, rcWork.top - rc.top);
		}
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

static void FlipBitmap(HBITMAP bitmap, int width, int height) {
	HDC hdc = ::CreateCompatibleDC(NULL);
	if (hdc != NULL) {
		HGDIOBJ prevBmp = ::SelectObject(hdc, bitmap);
		::StretchBlt(hdc, width - 1, 0, -width, height, hdc, 0, 0, width, height, SRCCOPY);
		::SelectObject(hdc, prevBmp);
		::DeleteDC(hdc);
	}
}

static HCURSOR GetReverseArrowCursor() {
	if (reverseArrowCursor != NULL)
		return reverseArrowCursor;

	::EnterCriticalSection(&crPlatformLock);
	HCURSOR cursor = reverseArrowCursor;
	if (cursor == NULL) {
		cursor = ::LoadCursor(NULL, IDC_ARROW);
		ICONINFO info;
		if (::GetIconInfo(cursor, &info)) {
			BITMAP bmp;
			if (::GetObject(info.hbmMask, sizeof(bmp), &bmp)) {
				FlipBitmap(info.hbmMask, bmp.bmWidth, bmp.bmHeight);
				if (info.hbmColor != NULL)
					FlipBitmap(info.hbmColor, bmp.bmWidth, bmp.bmHeight);
				info.xHotspot = (DWORD)bmp.bmWidth - 1 - info.xHotspot;

				reverseArrowCursor = ::CreateIconIndirect(&info);
				if (reverseArrowCursor != NULL)
					cursor = reverseArrowCursor;
			}

			::DeleteObject(info.hbmMask);
			if (info.hbmColor != NULL)
				::DeleteObject(info.hbmColor);
		}
	}
	::LeaveCriticalSection(&crPlatformLock);
	return cursor;
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
	case cursorReverseArrow:
		::SetCursor(GetReverseArrowCursor());
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
	// MonitorFromPoint and GetMonitorInfo are not available on Windows 95 and NT 4.
	PRectangle rcPosition = GetPosition();
	POINT ptDesktop = {static_cast<LONG>(pt.x + rcPosition.left),
		static_cast<LONG>(pt.y + rcPosition.top)};
	HMONITOR hMonitor = NULL;
	if (MonitorFromPointFn)
		hMonitor = MonitorFromPointFn(ptDesktop, MONITOR_DEFAULTTONEAREST);

	RECT rcWork = RectFromMonitor(hMonitor);
	if (rcWork.left < rcWork.right) {
		PRectangle rcMonitor(
			rcWork.left - rcPosition.left,
			rcWork.top - rcPosition.top,
			rcWork.right - rcPosition.left,
			rcWork.bottom - rcPosition.top);
		return rcMonitor;
	} else {
		return PRectangle();
	}
}

struct ListItemData {
	const char *text;
	int pixId;
};

class LineToItem {
	std::vector<char> words;

	std::vector<ListItemData> data;

public:
	LineToItem() {
	}
	~LineToItem() {
		Clear();
	}
	void Clear() {
		words.clear();
		data.clear();
	}

	ListItemData Get(int index) const {
		if (index >= 0 && index < static_cast<int>(data.size())) {
			return data[index];
		} else {
			ListItemData missing = {"", -1};
			return missing;
		}
	}
	int Count() const {
		return static_cast<int>(data.size());
	}

	void AllocItem(const char *text, int pixId) {
		ListItemData lid = { text, pixId };
		data.push_back(lid);
	}

	char *SetWords(const char *s) {
		words = std::vector<char>(s, s+strlen(s)+1);
		return &words[0];
	}
};

const TCHAR ListBoxX_ClassName[] = TEXT("ListBoxX");

ListBox::ListBox() {
}

ListBox::~ListBox() {
}

class ListBoxX : public ListBox {
	int lineHeight;
	FontID fontCopy;
	int technology;
	RGBAImageSet images;
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
	int wheelDelta; // mouse wheel residue

	HWND GetHWND() const;
	void AppendListItem(const char *text, const char *numword);
	static void AdjustWindowRect(PRectangle *rc);
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
	ListBoxX() : lineHeight(10), fontCopy(0), technology(0), lb(0), unicodeMode(false),
		desiredVisibleRows(5), maxItemCharacters(0), aveCharWidth(8),
		parent(NULL), ctrlID(0), doubleClickAction(NULL), doubleClickActionData(NULL),
		widestItem(NULL), maxCharWidth(1), resizeHit(0), wheelDelta(0) {
	}
	virtual ~ListBoxX() {
		if (fontCopy) {
			::DeleteObject(fontCopy);
			fontCopy = 0;
		}
	}
	virtual void SetFont(Font &font);
	virtual void Create(Window &parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_, int technology_);
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
	virtual void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage);
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

void ListBoxX::Create(Window &parent_, int ctrlID_, Point location_, int lineHeight_, bool unicodeMode_, int technology_) {
	parent = &parent_;
	ctrlID = ctrlID_;
	location = location_;
	lineHeight = lineHeight_;
	unicodeMode = unicodeMode_;
	technology = technology_;
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

	POINT locationw = {static_cast<LONG>(location.x), static_cast<LONG>(location.y)};
	::MapWindowPoints(hwndParent, NULL, &locationw, 1);
	location = Point(locationw.x, locationw.y);
}

void ListBoxX::SetFont(Font &font) {
	if (font.GetID()) {
		if (fontCopy) {
			::DeleteObject(fontCopy);
			fontCopy = 0;
		}
		FormatAndMetrics *pfm = reinterpret_cast<FormatAndMetrics *>(font.GetID());
		fontCopy = pfm->HFont();
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
	int len = static_cast<int>(widestItem ? strlen(widestItem) : 0);
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
	int pixWidth = images.GetWidth();
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

void ListBoxX::Append(char *, int) {
	// This method is no longer called in Scintilla
	PLATFORM_ASSERT(false);
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
	XPM xpmImage(xpm_data);
	images.Add(type, new RGBAImage(xpmImage));
}

void ListBoxX::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) {
	images.Add(type, new RGBAImage(width, height, 1.0, pixelsImage));
}

void ListBoxX::ClearRegisteredImages() {
	images.Clear();
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
		int len = static_cast<int>(strlen(text));

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
		RGBAImage *pimage = images.Get(pixId);
		if (pimage) {
			Surface *surfaceItem = Surface::Allocate(technology);
			if (surfaceItem) {
				if (technology == SCWIN_TECH_GDI) {
					surfaceItem->Init(pDrawItem->hDC, pDrawItem->hwndItem);
					int left = pDrawItem->rcItem.left + ItemInset.x + ImageInset.x;
					PRectangle rcImage(left, pDrawItem->rcItem.top,
						left + images.GetWidth(), pDrawItem->rcItem.bottom);
					surfaceItem->DrawRGBAImage(rcImage,
						pimage->GetWidth(), pimage->GetHeight(), pimage->Pixels());
					delete surfaceItem;
					::SetTextAlign(pDrawItem->hDC, TA_TOP);
				} else {
#if defined(USE_D2D)
					D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
						D2D1_RENDER_TARGET_TYPE_DEFAULT,
						D2D1::PixelFormat(
							DXGI_FORMAT_B8G8R8A8_UNORM,
							D2D1_ALPHA_MODE_IGNORE),
						0,
						0,
						D2D1_RENDER_TARGET_USAGE_NONE,
						D2D1_FEATURE_LEVEL_DEFAULT
						);
					ID2D1DCRenderTarget *pDCRT = 0;
					HRESULT hr = pD2DFactory->CreateDCRenderTarget(&props, &pDCRT);
					if (SUCCEEDED(hr)) {
						RECT rcWindow;
						GetClientRect(pDrawItem->hwndItem, &rcWindow);
						hr = pDCRT->BindDC(pDrawItem->hDC, &rcWindow);
						if (SUCCEEDED(hr)) {
							surfaceItem->Init(pDCRT, pDrawItem->hwndItem);
							pDCRT->BeginDraw();
							int left = pDrawItem->rcItem.left + ItemInset.x + ImageInset.x;
							PRectangle rcImage(left, pDrawItem->rcItem.top,
								left + images.GetWidth(), pDrawItem->rcItem.bottom);
							surfaceItem->DrawRGBAImage(rcImage,
								pimage->GetWidth(), pimage->GetHeight(), pimage->Pixels());
							delete surfaceItem;
							pDCRT->EndDraw();
							pDCRT->Release();
						}
					}
#endif
				}
			}
		}
	}
}

void ListBoxX::AppendListItem(const char *text, const char *numword) {
	int pixId = -1;
	if (numword) {
		pixId = 0;
		char ch;
		while ((ch = *++numword) != '\0') {
			pixId = 10 * pixId + (ch - '0');
		}
	}

	lti.AllocItem(text, pixId);
	unsigned int len = static_cast<unsigned int>(strlen(text));
	if (maxItemCharacters < len) {
		maxItemCharacters = len;
		widestItem = text;
	}
}

void ListBoxX::SetList(const char *list, char separator, char typesep) {
	// Turn off redraw while populating the list - this has a significant effect, even if
	// the listbox is not visible.
	SetRedraw(false);
	Clear();
	size_t size = strlen(list);
	char *words = lti.SetWords(list);
	char *startword = words;
	char *numword = NULL;
	for (size_t i=0; i < size; i++) {
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

void ListBoxX::AdjustWindowRect(PRectangle *rc) {
	RECT rcw = RectFromPRectangle(*rc);
	::AdjustWindowRectEx(&rcw, WS_THICKFRAME, false, WS_EX_WINDOWEDGE);
	*rc = PRectangle(rcw.left, rcw.top, rcw.right, rcw.bottom);
}

int ListBoxX::ItemHeight() const {
	int itemHeight = lineHeight + (TextInset.y * 2);
	int pixHeight = images.GetHeight() + (ImageInset.y * 2);
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
	POINT ret = {static_cast<LONG>(rc.Width()), static_cast<LONG>(rc.Height())};
	return ret;
}

POINT ListBoxX::MaxTrackSize() const {
	PRectangle rc(0, 0,
		maxCharWidth * maxItemCharacters + TextInset.x * 2 +
		 TextOffset() + ::GetSystemMetrics(SM_CXVSCROLL),
		ItemHeight() * lti.Count());
	AdjustWindowRect(&rc);
	POINT ret = {static_cast<LONG>(rc.Width()), static_cast<LONG>(rc.Height())};
	return ret;
}

void ListBoxX::SetRedraw(bool on) {
	::SendMessage(lb, WM_SETREDRAW, static_cast<BOOL>(on), 0);
	if (on)
		::InvalidateRect(lb, NULL, TRUE);
}

void ListBoxX::ResizeToCursor() {
	PRectangle rc = GetPosition();
	POINT ptw;
	::GetCursorPos(&ptw);
	Point pt(ptw.x, ptw.y);
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
	RECT rc = { 0, 0, static_cast<LONG>(extent.x), static_cast<LONG>(extent.y) };
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

		case WM_MBUTTONDOWN:
			// disable the scroll wheel button click action
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

	case WM_MOUSEWHEEL:
		wheelDelta -= static_cast<short>(HIWORD(wParam));
		if (abs(wheelDelta) >= WHEEL_DELTA) {
			int nRows = GetVisibleRows();
			int linesToScroll = 1;
			if (nRows > 1) {
				linesToScroll = nRows - 1;
			}
			if (linesToScroll > 3) {
				linesToScroll = 3;
			}
			linesToScroll *= (wheelDelta / WHEEL_DELTA);
			int top = ::SendMessage(lb, LB_GETTOPINDEX, 0, 0) + linesToScroll;
			if (top < 0) {
				top = 0;
			}
			::SendMessage(lb, LB_SETTOPINDEX, top, 0);
			// update wheel delta residue
			if (wheelDelta >= 0)
				wheelDelta = wheelDelta % WHEEL_DELTA;
			else
				wheelDelta = - (-wheelDelta % WHEEL_DELTA);
		}
		break;

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

int Platform::DBCSCharLength(int codePage, const char *s) {
	if (codePage == 932 || codePage == 936 || codePage == 949 ||
	        codePage == 950 || codePage == 1361) {
		return Platform::IsDBCSLeadByte(codePage, s[0]) ? 2 : 1;
	} else {
		return 1;
	}
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

#ifdef _MSC_VER
// GetVersionEx has been deprecated fro Windows 8.1 but called here to determine if Windows 9x.
// Too dangerous to find alternate check.
#pragma warning(disable: 4996)
#endif

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
	if (!hDLLUser32) {
		hDLLUser32 = ::LoadLibrary(TEXT("User32"));
	}
	if (hDLLUser32) {
		MonitorFromPointFn = (MonitorFromPointSig)::GetProcAddress(hDLLUser32, "MonitorFromPoint");
		MonitorFromRectFn = (MonitorFromRectSig)::GetProcAddress(hDLLUser32, "MonitorFromRect");
		GetMonitorInfoFn = (GetMonitorInfoSig)::GetProcAddress(hDLLUser32, "GetMonitorInfoA");
	}

	ListBoxX_Register();
}

#ifdef _MSC_VER
#pragma warning(default: 4996)
#endif

void Platform_Finalise() {
	if (reverseArrowCursor != NULL)
		::DestroyCursor(reverseArrowCursor);
	ListBoxX_Unregister();
	::DeleteCriticalSection(&crPlatformLock);
}
