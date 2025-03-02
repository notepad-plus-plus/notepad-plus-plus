// Scintilla source code edit control
/** @file PlatWin.cxx
 ** Implementation of platform facilities on Windows.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cmath>
#include <climits>

#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>

// Want to use std::min and std::max so don't want Windows.h version of min and max
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#undef WINVER
#define WINVER 0x0A00
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <windowsx.h>
#include <shellscalingapi.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#if !defined(DISABLE_D2D)
#define USE_D2D 1
#endif

#if defined(USE_D2D)
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite_1.h>
#endif

#include "ScintillaTypes.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"
#include "XPM.h"
#include "UniConversion.h"
#include "DBCS.h"

#include "WinTypes.h"
#include "PlatWin.h"

// __uuidof is a Microsoft extension but makes COM code neater, so disable warning
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

using namespace Scintilla;

namespace Scintilla::Internal {

UINT CodePageFromCharSet(CharacterSet characterSet, UINT documentCodePage) noexcept;

#if defined(USE_D2D)
IDWriteFactory1 *pIDWriteFactory = nullptr;
ID2D1Factory1 *pD2DFactory = nullptr;
D2D1_DRAW_TEXT_OPTIONS d2dDrawTextOptions = D2D1_DRAW_TEXT_OPTIONS_NONE;

namespace {

HMODULE hDLLD2D{};
HMODULE hDLLD3D{};
HMODULE hDLLDWrite{};

PFN_D3D11_CREATE_DEVICE fnDCD {};

void LoadD2DOnce() noexcept {
	DWORD loadLibraryFlags = 0;
	HMODULE kernel32 = ::GetModuleHandleW(L"kernel32.dll");
	if (kernel32) {
		if (::GetProcAddress(kernel32, "SetDefaultDllDirectories")) {
			// Availability of SetDefaultDllDirectories implies Windows 8+ or
			// that KB2533623 has been installed so LoadLibraryEx can be called
			// with LOAD_LIBRARY_SEARCH_SYSTEM32.
			loadLibraryFlags = LOAD_LIBRARY_SEARCH_SYSTEM32;
		}
	}

	typedef HRESULT (WINAPI *D2D1CFSig)(D2D1_FACTORY_TYPE factoryType, REFIID riid,
		CONST D2D1_FACTORY_OPTIONS *pFactoryOptions, IUnknown **factory);
	typedef HRESULT (WINAPI *DWriteCFSig)(DWRITE_FACTORY_TYPE factoryType, REFIID iid,
		IUnknown **factory);

	hDLLD2D = ::LoadLibraryEx(TEXT("D2D1.DLL"), {}, loadLibraryFlags);
	D2D1CFSig fnD2DCF = DLLFunction<D2D1CFSig>(hDLLD2D, "D2D1CreateFactory");
	if (fnD2DCF) {
		const D2D1_FACTORY_OPTIONS options {};
		// A multi threaded factory in case Scintilla is used with multiple GUI threads
		fnD2DCF(D2D1_FACTORY_TYPE_MULTI_THREADED,
			__uuidof(ID2D1Factory1),
			&options,
			reinterpret_cast<IUnknown**>(&pD2DFactory));
	}
	hDLLDWrite = ::LoadLibraryEx(TEXT("DWRITE.DLL"), {}, loadLibraryFlags);
	DWriteCFSig fnDWCF = DLLFunction<DWriteCFSig>(hDLLDWrite, "DWriteCreateFactory");
	if (fnDWCF) {
		const GUID IID_IDWriteFactory2 = // 0439fc60-ca44-4994-8dee-3a9af7b732ec
		{ 0x0439fc60, 0xca44, 0x4994, { 0x8d, 0xee, 0x3a, 0x9a, 0xf7, 0xb7, 0x32, 0xec } };

		const HRESULT hr = fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
			IID_IDWriteFactory2,
			reinterpret_cast<IUnknown**>(&pIDWriteFactory));
		if (SUCCEEDED(hr)) {
			// D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
			d2dDrawTextOptions = static_cast<D2D1_DRAW_TEXT_OPTIONS>(0x00000004);
		} else {
			fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory1),
				reinterpret_cast<IUnknown**>(&pIDWriteFactory));
		}
	}

	hDLLD3D = ::LoadLibraryEx(TEXT("D3D11.DLL"), {}, loadLibraryFlags);
	if (!hDLLD3D) {
		Platform::DebugPrintf("Direct3D not loaded\n");
	}
	fnDCD = DLLFunction<PFN_D3D11_CREATE_DEVICE>(hDLLD3D, "D3D11CreateDevice");
	if (!fnDCD) {
		Platform::DebugPrintf("Direct3D does not have D3D11CreateDevice\n");
	}
}

}

HRESULT CreateD3D(D3D11Device &device) noexcept {
	device = nullptr;
	if (!fnDCD) {
		return E_FAIL;
	}

	const D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Create device.
	// Try for a hardware device but, if that fails, fall back to the Warp software rasterizer.
	ComPtr<ID3D11Device> upDevice;
	HRESULT hr = S_OK;
	const D3D_DRIVER_TYPE typesToTry[] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP };
	for (const D3D_DRIVER_TYPE type : typesToTry) {
		hr = fnDCD(nullptr,
			type,
			{},
			D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			upDevice.GetAddressOf(),
			nullptr,
			nullptr);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr)) {
		Platform::DebugPrintf("Failed to create D3D11 device 0x%lx\n", hr);
		return hr;
	}

	// Convert from D3D11 to D3D11.1
	hr = upDevice.As(&device);
	if (FAILED(hr)) {
		Platform::DebugPrintf("Failed to create D3D11.1 device 0x%lx\n", hr);
	}
	return hr;
}

bool LoadD2D() noexcept {
	static std::once_flag once;
	try {
		std::call_once(once, LoadD2DOnce);
	} catch (...) {
		// ignore
	}
	return pIDWriteFactory && pD2DFactory;
}

HRESULT CreateDCRenderTarget(const D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, DCRenderTarget &dcRT) noexcept {
	return pD2DFactory->CreateDCRenderTarget(renderTargetProperties, dcRT.ReleaseAndGetAddressOf());
}

namespace {

constexpr D2D_COLOR_F ColorFromColourAlpha(ColourRGBA colour) noexcept {
	return D2D_COLOR_F{
		colour.GetRedComponent(),
		colour.GetGreenComponent(),
		colour.GetBlueComponent(),
		colour.GetAlphaComponent()
	};
}

using BrushSolid = ComPtr<ID2D1SolidColorBrush>;

BrushSolid BrushSolidCreate(ID2D1RenderTarget *pTarget, COLORREF colour) noexcept {
	BrushSolid brush;
	const D2D_COLOR_F col = ColorFromColourAlpha(ColourRGBA::FromRGB(colour));
	if (FAILED(pTarget->CreateSolidColorBrush(col, brush.GetAddressOf()))) {
		return {};
	}
	return brush;
}

using Geometry = ComPtr<ID2D1PathGeometry>;

Geometry GeometryCreate() noexcept {
	Geometry geometry;
	if (FAILED(pD2DFactory->CreatePathGeometry(geometry.GetAddressOf()))) {
		return {};
	}
	return geometry;
}

using GeometrySink = ComPtr<ID2D1GeometrySink>;

GeometrySink GeometrySinkCreate(ID2D1PathGeometry *geometry) noexcept {
	GeometrySink sink;
	if (FAILED(geometry->Open(sink.GetAddressOf()))) {
		return {};
	}
	return sink;
}

using StrokeStyle = ComPtr<ID2D1StrokeStyle>;

StrokeStyle StrokeStyleCreate(const D2D1_STROKE_STYLE_PROPERTIES &strokeStyleProperties) noexcept {
	StrokeStyle strokeStyle;
	const HRESULT hr = pD2DFactory->CreateStrokeStyle(
		strokeStyleProperties, nullptr, 0, strokeStyle.GetAddressOf());
	if (FAILED(hr)) {
		return {};
	}
	return strokeStyle;
}

using TextLayout = ComPtr<IDWriteTextLayout>;

TextLayout LayoutCreate(std::wstring_view wsv, IDWriteTextFormat *pTextFormat, FLOAT maxWidth=10000.0F, FLOAT maxHeight=1000.0F) noexcept {
	TextLayout layout;
	const HRESULT hr = pIDWriteFactory->CreateTextLayout(wsv.data(), static_cast<UINT32>(wsv.length()),
		pTextFormat, maxWidth, maxHeight, layout.GetAddressOf());
	if (FAILED(hr)) {
		return {};
	}
	return layout;
}

}

#endif

void *PointerFromWindow(HWND hWnd) noexcept {
	return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, 0));
}

void SetWindowPointer(HWND hWnd, void *ptr) noexcept {
	::SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(ptr));
}

namespace {

// system DPI, same for all monitor.
UINT uSystemDPI = USER_DEFAULT_SCREEN_DPI;

using GetDpiForWindowSig = UINT(WINAPI *)(HWND hwnd);
GetDpiForWindowSig fnGetDpiForWindow = nullptr;

HMODULE hDLLShcore {};
using GetDpiForMonitorSig = HRESULT (WINAPI *)(HMONITOR hmonitor, /*MONITOR_DPI_TYPE*/int dpiType, UINT *dpiX, UINT *dpiY);
GetDpiForMonitorSig fnGetDpiForMonitor = nullptr;

using GetSystemMetricsForDpiSig = int(WINAPI *)(int nIndex, UINT dpi);
GetSystemMetricsForDpiSig fnGetSystemMetricsForDpi = nullptr;

using AdjustWindowRectExForDpiSig = BOOL(WINAPI *)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
AdjustWindowRectExForDpiSig fnAdjustWindowRectExForDpi = nullptr;

using AreDpiAwarenessContextsEqualSig = BOOL(WINAPI *)(DPI_AWARENESS_CONTEXT, DPI_AWARENESS_CONTEXT);
AreDpiAwarenessContextsEqualSig fnAreDpiAwarenessContextsEqual = nullptr;

using GetWindowDpiAwarenessContextSig = DPI_AWARENESS_CONTEXT(WINAPI *)(HWND);
GetWindowDpiAwarenessContextSig fnGetWindowDpiAwarenessContext = nullptr;

using GetScaleFactorForMonitorSig = HRESULT(WINAPI *)(HMONITOR, DEVICE_SCALE_FACTOR *);
GetScaleFactorForMonitorSig fnGetScaleFactorForMonitor = nullptr;

using SetThreadDpiAwarenessContextSig = DPI_AWARENESS_CONTEXT(WINAPI *)(DPI_AWARENESS_CONTEXT);
SetThreadDpiAwarenessContextSig fnSetThreadDpiAwarenessContext = nullptr;

void LoadDpiForWindow() noexcept {
	HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
	fnGetDpiForWindow = DLLFunction<GetDpiForWindowSig>(user32, "GetDpiForWindow");
	fnGetSystemMetricsForDpi = DLLFunction<GetSystemMetricsForDpiSig>(user32, "GetSystemMetricsForDpi");
	fnAdjustWindowRectExForDpi = DLLFunction<AdjustWindowRectExForDpiSig>(user32, "AdjustWindowRectExForDpi");
	fnSetThreadDpiAwarenessContext = DLLFunction<SetThreadDpiAwarenessContextSig>(user32, "SetThreadDpiAwarenessContext");

	using GetDpiForSystemSig = UINT(WINAPI *)(void);
	GetDpiForSystemSig fnGetDpiForSystem = DLLFunction<GetDpiForSystemSig>(user32, "GetDpiForSystem");
	if (fnGetDpiForSystem) {
		uSystemDPI = fnGetDpiForSystem();
	} else {
		HDC hdcMeasure = ::CreateCompatibleDC({});
		uSystemDPI = ::GetDeviceCaps(hdcMeasure, LOGPIXELSY);
		::DeleteDC(hdcMeasure);
	}

	fnGetWindowDpiAwarenessContext = DLLFunction<GetWindowDpiAwarenessContextSig>(user32, "GetWindowDpiAwarenessContext");
	fnAreDpiAwarenessContextsEqual = DLLFunction<AreDpiAwarenessContextsEqualSig>(user32, "AreDpiAwarenessContextsEqual");

	hDLLShcore = ::LoadLibraryExW(L"shcore.dll", {}, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hDLLShcore) {
		fnGetScaleFactorForMonitor = DLLFunction<GetScaleFactorForMonitorSig>(hDLLShcore, "GetScaleFactorForMonitor");
		fnGetDpiForMonitor = DLLFunction<GetDpiForMonitorSig>(hDLLShcore, "GetDpiForMonitor");
	}
}

HINSTANCE hinstPlatformRes {};

constexpr Supports SupportsGDI[] = {
	Supports::PixelModification,
};

constexpr BYTE Win32MapFontQuality(FontQuality extraFontFlag) noexcept {
	switch (extraFontFlag & FontQuality::QualityMask) {

		case FontQuality::QualityNonAntialiased:
			return NONANTIALIASED_QUALITY;

		case FontQuality::QualityAntialiased:
			return ANTIALIASED_QUALITY;

		case FontQuality::QualityLcdOptimized:
			return CLEARTYPE_QUALITY;

		default:
			return DEFAULT_QUALITY;
	}
}

#if defined(USE_D2D)
constexpr D2D1_TEXT_ANTIALIAS_MODE DWriteMapFontQuality(FontQuality extraFontFlag) noexcept {
	switch (extraFontFlag & FontQuality::QualityMask) {

		case FontQuality::QualityNonAntialiased:
			return D2D1_TEXT_ANTIALIAS_MODE_ALIASED;

		case FontQuality::QualityAntialiased:
			return D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE;

		case FontQuality::QualityLcdOptimized:
			return D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;

		default:
			return D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
	}
}
#endif

// Both GDI and DirectWrite can produce a HFONT for use in list boxes
struct FontWin : public Font {
	[[nodiscard]] virtual HFONT HFont() const noexcept = 0;
};

void SetLogFont(LOGFONTW &lf, const char *faceName, CharacterSet characterSet, XYPOSITION size, FontWeight weight, bool italic, FontQuality extraFontFlag) {
	lf = LOGFONTW();
	// The negative is to allow for leading
	lf.lfHeight = -(std::abs(std::lround(size)));
	lf.lfWeight = static_cast<LONG>(weight);
	lf.lfItalic = italic ? 1 : 0;
	lf.lfCharSet = static_cast<BYTE>(characterSet);
	lf.lfQuality = Win32MapFontQuality(extraFontFlag);
	UTF16FromUTF8(faceName, lf.lfFaceName, LF_FACESIZE);
}

struct FontGDI : public FontWin {
	HFONT hfont = {};
	explicit FontGDI(const FontParameters &fp) {
		LOGFONTW lf;
		SetLogFont(lf, fp.faceName, fp.characterSet, fp.size, fp.weight, fp.italic, fp.extraFontFlag);
		hfont = ::CreateFontIndirectW(&lf);
	}
	// Deleted so FontGDI objects can not be copied.
	FontGDI(const FontGDI &) = delete;
	FontGDI(FontGDI &&) = delete;
	FontGDI &operator=(const FontGDI &) = delete;
	FontGDI &operator=(FontGDI &&) = delete;
	~FontGDI() noexcept override {
		if (hfont)
			::DeleteObject(hfont);
	}
	[[nodiscard]] HFONT HFont() const noexcept override {
		// Duplicating hfont
		LOGFONTW lf = {};
		if (0 == ::GetObjectW(hfont, sizeof(lf), &lf)) {
			return {};
		}
		return ::CreateFontIndirectW(&lf);
	}
};

#if defined(USE_D2D)
struct FontDirectWrite : public FontWin {
	ComPtr<IDWriteTextFormat> pTextFormat;
	FontQuality extraFontFlag = FontQuality::QualityDefault;
	CharacterSet characterSet = CharacterSet::Ansi;
	FLOAT yAscent = 2.0f;
	FLOAT yDescent = 1.0f;
	FLOAT yInternalLeading = 0.0f;

	explicit FontDirectWrite(const FontParameters &fp) :
		extraFontFlag(fp.extraFontFlag),
		characterSet(fp.characterSet) {
		const std::wstring wsFace = WStringFromUTF8(fp.faceName);
		const std::wstring wsLocale = WStringFromUTF8(fp.localeName);
		const FLOAT fHeight = static_cast<FLOAT>(fp.size);
		const DWRITE_FONT_STYLE style = fp.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
		HRESULT hr = pIDWriteFactory->CreateTextFormat(wsFace.c_str(), nullptr,
			static_cast<DWRITE_FONT_WEIGHT>(fp.weight),
			style,
			static_cast<DWRITE_FONT_STRETCH>(fp.stretch),
				fHeight, wsLocale.c_str(), pTextFormat.GetAddressOf());
		if (hr == E_INVALIDARG) {
			// Possibly a bad locale name like "/" so try "en-us".
			hr = pIDWriteFactory->CreateTextFormat(wsFace.c_str(), nullptr,
				static_cast<DWRITE_FONT_WEIGHT>(fp.weight),
				style,
				static_cast<DWRITE_FONT_STRETCH>(fp.stretch),
				fHeight, L"en-us", pTextFormat.ReleaseAndGetAddressOf());
		}
		if (SUCCEEDED(hr)) {
			pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

			if (TextLayout pTextLayout = LayoutCreate(L"X", pTextFormat.Get())) {
				constexpr int maxLines = 2;
				DWRITE_LINE_METRICS lineMetrics[maxLines]{};
				UINT32 lineCount = 0;
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
				pTextFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, lineMetrics[0].height, lineMetrics[0].baseline);
			}
		}
	}
	// Deleted so FontDirectWrite objects can not be copied.
	FontDirectWrite(const FontDirectWrite &) = delete;
	FontDirectWrite(FontDirectWrite &&) = delete;
	FontDirectWrite &operator=(const FontDirectWrite &) = delete;
	FontDirectWrite &operator=(FontDirectWrite &&) = delete;
	~FontDirectWrite() noexcept override = default;
	[[nodiscard]] HFONT HFont() const noexcept override {
		LOGFONTW lf = {};
		const HRESULT hr = pTextFormat->GetFontFamilyName(lf.lfFaceName, LF_FACESIZE);
		if (!SUCCEEDED(hr)) {
			return {};
		}
		lf.lfWeight = pTextFormat->GetFontWeight();
		lf.lfItalic = pTextFormat->GetFontStyle() == DWRITE_FONT_STYLE_ITALIC;
		lf.lfHeight = -static_cast<int>(pTextFormat->GetFontSize());
		return ::CreateFontIndirectW(&lf);
	}

	[[nodiscard]] int CodePageText(int codePage) const noexcept {
		if (!(codePage == CpUtf8) && (characterSet != CharacterSet::Ansi)) {
			codePage = CodePageFromCharSet(characterSet, codePage);
		}
		return codePage;
	}

	static const FontDirectWrite *Cast(const Font *font_) {
		const FontDirectWrite *pfm = dynamic_cast<const FontDirectWrite *>(font_);
		PLATFORM_ASSERT(pfm);
		if (!pfm) {
			throw std::runtime_error("SurfaceD2D::SetFont: wrong Font type.");
		}
		return pfm;
	}
};
#endif

}

HMONITOR MonitorFromWindowHandleScaling(HWND hWnd) noexcept {
	constexpr DWORD monitorFlags = MONITOR_DEFAULTTONEAREST;

	if (!fnSetThreadDpiAwarenessContext) {
		return ::MonitorFromWindow(hWnd, monitorFlags);
	}

	// Temporarily switching to PerMonitorV2 to retrieve correct monitor via MonitorFromRect() in case of active GDI scaling.
	const DPI_AWARENESS_CONTEXT oldContext = fnSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	PLATFORM_ASSERT(oldContext != nullptr);

	RECT rect;
	::GetWindowRect(hWnd, &rect);
	const HMONITOR monitor = ::MonitorFromRect(&rect, monitorFlags);

	fnSetThreadDpiAwarenessContext(oldContext);
	return monitor;
}

float GetDeviceScaleFactorWhenGdiScalingActive(HWND hWnd) noexcept {
	if (fnAreDpiAwarenessContextsEqual) {
		PLATFORM_ASSERT(fnGetWindowDpiAwarenessContext && fnGetScaleFactorForMonitor);
		if (fnAreDpiAwarenessContextsEqual(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED, fnGetWindowDpiAwarenessContext(hWnd))) {
			const HWND hRootWnd = ::GetAncestor(hWnd, GA_ROOT); // Scale factor applies to entire (root) window.
			const HMONITOR hMonitor = MonitorFromWindowHandleScaling(hRootWnd);
			DEVICE_SCALE_FACTOR deviceScaleFactor;
			if (S_OK == fnGetScaleFactorForMonitor(hMonitor, &deviceScaleFactor))
				return static_cast<int>(deviceScaleFactor) / 100.f;
		}
	}
	return 1.f;
}

std::shared_ptr<Font> Font::Allocate(const FontParameters &fp) {
#if defined(USE_D2D)
	if (fp.technology != Technology::Default) {
		return std::make_shared<FontDirectWrite>(fp);
	}
#endif
	return std::make_shared<FontGDI>(fp);
}

// Buffer to hold strings and string position arrays without always allocating on heap.
// May sometimes have string too long to allocate on stack. So use a fixed stack-allocated buffer
// when less than safe size otherwise allocate on heap and free automatically.
template<typename T, int lengthStandard>
class VarBuffer {
	T bufferStandard[lengthStandard];
public:
	T *buffer;
	explicit VarBuffer(size_t length) : buffer(nullptr) {
		if (length > lengthStandard) {
			buffer = new T[length];
		} else {
			buffer = bufferStandard;
		}
	}
	// Deleted so VarBuffer objects can not be copied.
	VarBuffer(const VarBuffer &) = delete;
	VarBuffer(VarBuffer &&) = delete;
	VarBuffer &operator=(const VarBuffer &) = delete;
	VarBuffer &operator=(VarBuffer &&) = delete;

	~VarBuffer() noexcept {
		if (buffer != bufferStandard) {
			delete []buffer;
			buffer = nullptr;
		}
	}
};

constexpr int stackBufferLength = 400;
class TextWide : public VarBuffer<wchar_t, stackBufferLength> {
public:
	int tlen;	// Using int instead of size_t as most Win32 APIs take int.
	TextWide(std::string_view text, int codePage) :
		VarBuffer<wchar_t, stackBufferLength>(text.length()) {
		if (codePage == CpUtf8) {
			tlen = static_cast<int>(UTF16FromUTF8(text, buffer, text.length()));
		} else {
			// Support Asian string display in 9x English
			tlen = ::MultiByteToWideChar(codePage, 0, text.data(), static_cast<int>(text.length()),
				buffer, static_cast<int>(text.length()));
		}
	}
	[[nodiscard]] std::wstring_view AsView() const noexcept {
		return std::wstring_view(buffer, tlen);
	}
};
using TextPositions = VarBuffer<XYPOSITION, stackBufferLength>;

UINT DpiForWindow(WindowID wid) noexcept {
	if (fnGetDpiForWindow) {
		return fnGetDpiForWindow(HwndFromWindowID(wid));
	}
	if (fnGetDpiForMonitor) {
		HMONITOR hMonitor = ::MonitorFromWindow(HwndFromWindowID(wid), MONITOR_DEFAULTTONEAREST);
		UINT dpiX = 0;
		UINT dpiY = 0;
		if (fnGetDpiForMonitor(hMonitor, 0 /*MDT_EFFECTIVE_DPI*/, &dpiX, &dpiY) == S_OK) {
			return dpiY;
		}
	}
	return uSystemDPI;
}

int SystemMetricsForDpi(int nIndex, UINT dpi) noexcept {
	if (fnGetSystemMetricsForDpi) {
		return fnGetSystemMetricsForDpi(nIndex, dpi);
	}

	int value = ::GetSystemMetrics(nIndex);
	value = (dpi == uSystemDPI) ? value : ::MulDiv(value, dpi, uSystemDPI);
	return value;
}

class SurfaceGDI : public Surface {
	SurfaceMode mode;
	HDC hdc{};
	bool hdcOwned=false;
	HPEN pen{};
	HPEN penOld{};
	HBRUSH brush{};
	HBRUSH brushOld{};
	HFONT fontOld{};
	HBITMAP bitmap{};
	HBITMAP bitmapOld{};

	int logPixelsY = USER_DEFAULT_SCREEN_DPI;

	static constexpr int maxWidthMeasure = INT_MAX;
	// There appears to be a 16 bit string length limit in GDI on NT.
	static constexpr int maxLenText = 65535;

	void PenColour(ColourRGBA fore, XYPOSITION widthStroke) noexcept;

	void BrushColour(ColourRGBA back) noexcept;
	void SetFont(const Font *font_);
	void Clear() noexcept;

public:
	SurfaceGDI() noexcept = default;
	SurfaceGDI(HDC hdcCompatible, int width, int height, SurfaceMode mode_, int logPixelsY_) noexcept;
	// Deleted so SurfaceGDI objects can not be copied.
	SurfaceGDI(const SurfaceGDI &) = delete;
	SurfaceGDI(SurfaceGDI &&) = delete;
	SurfaceGDI &operator=(const SurfaceGDI &) = delete;
	SurfaceGDI &operator=(SurfaceGDI &&) = delete;

	~SurfaceGDI() noexcept override;

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

	void DrawTextCommon(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, UINT fuOptions);
	void DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) override;
	void MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthText(const Font *font_, std::string_view text) override;

	void DrawTextCommonUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, UINT fuOptions);
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

SurfaceGDI::SurfaceGDI(HDC hdcCompatible, int width, int height, SurfaceMode mode_, int logPixelsY_) noexcept {
	hdc = ::CreateCompatibleDC(hdcCompatible);
	hdcOwned = true;
	bitmap = ::CreateCompatibleBitmap(hdcCompatible, width, height);
	bitmapOld = SelectBitmap(hdc, bitmap);
	::SetTextAlign(hdc, TA_BASELINE);
	mode = mode_;
	logPixelsY = logPixelsY_;
}

SurfaceGDI::~SurfaceGDI() noexcept {
	Clear();
}

void SurfaceGDI::Clear() noexcept {
	if (penOld) {
		::SelectObject(hdc, penOld);
		::DeleteObject(pen);
		penOld = {};
	}
	pen = {};
	if (brushOld) {
		::SelectObject(hdc, brushOld);
		::DeleteObject(brush);
		brushOld = {};
	}
	brush = {};
	if (fontOld) {
		// Fonts are not deleted as they are owned by a Font object
		::SelectObject(hdc, fontOld);
		fontOld = {};
	}
	if (bitmapOld) {
		::SelectObject(hdc, bitmapOld);
		::DeleteObject(bitmap);
		bitmapOld = {};
	}
	bitmap = {};
	if (hdcOwned) {
		::DeleteDC(hdc);
		hdc = {};
		hdcOwned = false;
	}
}

void SurfaceGDI::Release() noexcept {
	Clear();
}

int SurfaceGDI::SupportsFeature(Supports feature) noexcept {
	for (const Supports f : SupportsGDI) {
		if (f == feature)
			return 1;
	}
	return 0;
}

bool SurfaceGDI::Initialised() {
	return hdc != 0;
}

void SurfaceGDI::Init(WindowID wid) {
	Release();
	hdc = ::CreateCompatibleDC({});
	hdcOwned = true;
	::SetTextAlign(hdc, TA_BASELINE);
	logPixelsY = DpiForWindow(wid);
}

void SurfaceGDI::Init(SurfaceID sid, WindowID wid) {
	Release();
	hdc = static_cast<HDC>(sid);
	::SetTextAlign(hdc, TA_BASELINE);
	// Windows on screen are scaled but printers are not.
	const bool printing = ::GetDeviceCaps(hdc, TECHNOLOGY) != DT_RASDISPLAY;
	logPixelsY = printing ? ::GetDeviceCaps(hdc, LOGPIXELSY) : DpiForWindow(wid);
}

std::unique_ptr<Surface> SurfaceGDI::AllocatePixMap(int width, int height) {
	return std::make_unique<SurfaceGDI>(hdc, width, height, mode, logPixelsY);
}

void SurfaceGDI::SetMode(SurfaceMode mode_) {
	mode = mode_;
}

void SurfaceGDI::PenColour(ColourRGBA fore, XYPOSITION widthStroke) noexcept {
	if (pen) {
		::SelectObject(hdc, penOld);
		::DeleteObject(pen);
		pen = {};
		penOld = {};
	}
	const DWORD penWidth = std::lround(widthStroke);
	const COLORREF penColour = fore.OpaqueRGB();
	if (widthStroke > 1) {
		const LOGBRUSH brushParameters{ BS_SOLID, penColour, 0 };
		pen = ::ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_ROUND | PS_JOIN_MITER,
			penWidth,
			&brushParameters,
			0,
			nullptr);
	} else {
		pen = ::CreatePen(PS_INSIDEFRAME, penWidth, penColour);
	}
	penOld = SelectPen(hdc, pen);
}

void SurfaceGDI::BrushColour(ColourRGBA back) noexcept {
	if (brush) {
		::SelectObject(hdc, brushOld);
		::DeleteObject(brush);
		brush = {};
		brushOld = {};
	}
	brush = ::CreateSolidBrush(back.OpaqueRGB());
	brushOld = SelectBrush(hdc, brush);
}

void SurfaceGDI::SetFont(const Font *font_) {
	const FontGDI *pfm = dynamic_cast<const FontGDI *>(font_);
	PLATFORM_ASSERT(pfm);
	if (!pfm) {
		throw std::runtime_error("SurfaceGDI::SetFont: wrong Font type.");
	}
	if (fontOld) {
		SelectFont(hdc, pfm->hfont);
	} else {
		fontOld = SelectFont(hdc, pfm->hfont);
	}
}

int SurfaceGDI::LogPixelsY() {
	return logPixelsY;
}

int SurfaceGDI::PixelDivisions() {
	// Win32 uses device pixels.
	return 1;
}

int SurfaceGDI::DeviceHeightFont(int points) {
	return ::MulDiv(points, LogPixelsY(), 72);
}

void SurfaceGDI::LineDraw(Point start, Point end, Stroke stroke) {
	PenColour(stroke.colour, stroke.width);
	::MoveToEx(hdc, std::lround(std::floor(start.x)), std::lround(std::floor(start.y)), nullptr);
	::LineTo(hdc, std::lround(std::floor(end.x)), std::lround(std::floor(end.y)));
}

void SurfaceGDI::PolyLine(const Point *pts, size_t npts, Stroke stroke) {
	PLATFORM_ASSERT(npts > 1);
	if (npts <= 1) {
		return;
	}
	PenColour(stroke.colour, stroke.width);
	std::vector<POINT> outline;
	std::transform(pts, pts + npts, std::back_inserter(outline), POINTFromPoint);
	::Polyline(hdc, outline.data(), static_cast<int>(npts));
}

void SurfaceGDI::Polygon(const Point *pts, size_t npts, FillStroke fillStroke) {
	PenColour(fillStroke.stroke.colour.WithoutAlpha(), fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour.WithoutAlpha());
	std::vector<POINT> outline;
	std::transform(pts, pts + npts, std::back_inserter(outline), POINTFromPoint);
	::Polygon(hdc, outline.data(), static_cast<int>(npts));
}

void SurfaceGDI::RectangleDraw(PRectangle rc, FillStroke fillStroke) {
	RectangleFrame(rc, fillStroke.stroke);
	FillRectangle(rc.Inset(fillStroke.stroke.width), fillStroke.fill.colour);
}

void SurfaceGDI::RectangleFrame(PRectangle rc, Stroke stroke) {
	BrushColour(stroke.colour);
	const RECT rcw = RectFromPRectangle(rc);
	::FrameRect(hdc, &rcw, brush);
}

void SurfaceGDI::FillRectangle(PRectangle rc, Fill fill) {
	if (fill.colour.IsOpaque()) {
		// Using ExtTextOut rather than a FillRect ensures that no dithering occurs.
		// There is no need to allocate a brush either.
		const RECT rcw = RectFromPRectangle(rc);
		::SetBkColor(hdc, fill.colour.OpaqueRGB());
		::ExtTextOut(hdc, rcw.left, rcw.top, ETO_OPAQUE, &rcw, TEXT(""), 0, nullptr);
	} else {
		AlphaRectangle(rc, 0, FillStroke(fill.colour));
	}
}

void SurfaceGDI::FillRectangleAligned(PRectangle rc, Fill fill) {
	FillRectangle(PixelAlign(rc, 1), fill);
}

void SurfaceGDI::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	HBRUSH br{};
	if (SurfaceGDI *psgdi = dynamic_cast<SurfaceGDI *>(&surfacePattern); psgdi && psgdi->bitmap) {
		br = ::CreatePatternBrush(psgdi->bitmap);
	} else {	// Something is wrong so display in red
		br = ::CreateSolidBrush(RGB(0xff, 0, 0));
	}
	const RECT rcw = RectFromPRectangle(rc);
	::FillRect(hdc, &rcw, br);
	::DeleteObject(br);
}

void SurfaceGDI::RoundedRectangle(PRectangle rc, FillStroke fillStroke) {
	PenColour(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);
	const RECT rcw = RectFromPRectangle(rc);
	::RoundRect(hdc,
		rcw.left + 1, rcw.top,
		rcw.right - 1, rcw.bottom,
		8, 8);
}

namespace {

constexpr DWORD dwordFromBGRA(byte b, byte g, byte r, byte a) noexcept {
	return (a << 24) | (r << 16) | (g << 8) | b;
}

constexpr byte AlphaScaled(unsigned char component, unsigned int alpha) noexcept {
	return static_cast<byte>(component * alpha / 255);
}

constexpr DWORD dwordMultiplied(ColourRGBA colour) noexcept {
	return dwordFromBGRA(
		AlphaScaled(colour.GetBlue(), colour.GetAlpha()),
		AlphaScaled(colour.GetGreen(), colour.GetAlpha()),
		AlphaScaled(colour.GetRed(), colour.GetAlpha()),
		colour.GetAlpha());
}

class DIBSection {
	HDC hMemDC {};
	HBITMAP hbmMem {};
	HBITMAP hbmOld {};
	SIZE size {};
	DWORD *pixels = nullptr;
public:
	DIBSection(HDC hdc, SIZE size_) noexcept;
	// Deleted so DIBSection objects can not be copied.
	DIBSection(const DIBSection&) = delete;
	DIBSection(DIBSection&&) = delete;
	DIBSection &operator=(const DIBSection&) = delete;
	DIBSection &operator=(DIBSection&&) = delete;
	~DIBSection() noexcept;
	operator bool() const noexcept {
		return hMemDC && hbmMem && pixels;
	}
	[[nodiscard]] DWORD *Pixels() const noexcept {
		return pixels;
	}
	[[nodiscard]] unsigned char *Bytes() const noexcept {
		return reinterpret_cast<unsigned char *>(pixels);
	}
	[[nodiscard]] HDC DC() const noexcept {
		return hMemDC;
	}
	void SetPixel(LONG x, LONG y, DWORD value) noexcept {
		PLATFORM_ASSERT(x >= 0);
		PLATFORM_ASSERT(y >= 0);
		PLATFORM_ASSERT(x < size.cx);
		PLATFORM_ASSERT(y < size.cy);
		pixels[y * size.cx + x] = value;
	}
	void SetSymmetric(LONG x, LONG y, DWORD value) noexcept;
};

DIBSection::DIBSection(HDC hdc, SIZE size_) noexcept {
	hMemDC = ::CreateCompatibleDC(hdc);
	if (!hMemDC) {
		return;
	}

	size = size_;

	// -size.y makes bitmap start from top
	const BITMAPINFO bpih = { {sizeof(BITMAPINFOHEADER), size.cx, -size.cy, 1, 32, BI_RGB, 0, 0, 0, 0, 0},
		{{0, 0, 0, 0}} };
	void *image = nullptr;
	hbmMem = CreateDIBSection(hMemDC, &bpih, DIB_RGB_COLORS, &image, {}, 0);
	if (!hbmMem || !image) {
		return;
	}
	pixels = static_cast<DWORD *>(image);
	hbmOld = SelectBitmap(hMemDC, hbmMem);
}

DIBSection::~DIBSection() noexcept {
	if (hbmOld) {
		SelectBitmap(hMemDC, hbmOld);
		hbmOld = {};
	}
	if (hbmMem) {
		::DeleteObject(hbmMem);
		hbmMem = {};
	}
	if (hMemDC) {
		::DeleteDC(hMemDC);
		hMemDC = {};
	}
}

void DIBSection::SetSymmetric(LONG x, LONG y, DWORD value) noexcept {
	// Plot a point symmetrically to all 4 quadrants
	const LONG xSymmetric = size.cx - 1 - x;
	const LONG ySymmetric = size.cy - 1 - y;
	SetPixel(x, y, value);
	SetPixel(xSymmetric, y, value);
	SetPixel(x, ySymmetric, value);
	SetPixel(xSymmetric, ySymmetric, value);
}

ColourRGBA GradientValue(const std::vector<ColourStop> &stops, XYPOSITION proportion) noexcept {
	for (size_t stop = 0; stop < stops.size() - 1; stop++) {
		// Loop through each pair of stops
		const XYPOSITION positionStart = stops[stop].position;
		const XYPOSITION positionEnd = stops[stop + 1].position;
		if ((proportion >= positionStart) && (proportion <= positionEnd)) {
			const XYPOSITION proportionInPair = (proportion - positionStart) /
				(positionEnd - positionStart);
			return stops[stop].colour.MixedWith(stops[stop + 1].colour, proportionInPair);
		}
	}
	// Loop should always find a value
	return ColourRGBA();
}

constexpr BLENDFUNCTION mergeAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

}

void SurfaceGDI::AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) {
	// TODO: Implement strokeWidth
	const RECT rcw = RectFromPRectangle(rc);
	const SIZE size = SizeOfRect(rcw);

	if (size.cx > 0) {

		DIBSection section(hdc, size);

		if (section) {

			// Ensure not distorted too much by corners when small
			const LONG corner = std::min(static_cast<LONG>(cornerSize), (std::min(size.cx, size.cy) / 2) - 2);

			constexpr DWORD valEmpty = dwordFromBGRA(0,0,0,0);
			const DWORD valFill = dwordMultiplied(fillStroke.fill.colour);
			const DWORD valOutline = dwordMultiplied(fillStroke.stroke.colour);

			// Draw a framed rectangle
			for (int y=0; y<size.cy; y++) {
				for (int x=0; x<size.cx; x++) {
					if ((x==0) || (x==size.cx-1) || (y == 0) || (y == size.cy -1)) {
						section.SetPixel(x, y, valOutline);
					} else {
						section.SetPixel(x, y, valFill);
					}
				}
			}

			// Make the corners transparent
			for (LONG c=0; c<corner; c++) {
				for (LONG x=0; x<c+1; x++) {
					section.SetSymmetric(x, c - x, valEmpty);
				}
			}

			// Draw the corner frame pieces
			for (LONG x=1; x<corner; x++) {
				section.SetSymmetric(x, corner - x, valOutline);
			}

			GdiAlphaBlend(hdc, rcw.left, rcw.top, size.cx, size.cy, section.DC(), 0, 0, size.cx, size.cy, mergeAlpha);
		}
	} else {
		BrushColour(fillStroke.stroke.colour);
		FrameRect(hdc, &rcw, brush);
	}
}

void SurfaceGDI::GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) {

	const RECT rcw = RectFromPRectangle(rc);
	const SIZE size = SizeOfRect(rcw);

	DIBSection section(hdc, size);

	if (section) {

		if (options == GradientOptions::topToBottom) {
			for (LONG y = 0; y < size.cy; y++) {
				// Find y/height proportional colour
				const XYPOSITION proportion = y / (rc.Height() - 1.0f);
				const ColourRGBA mixed = GradientValue(stops, proportion);
				const DWORD valFill = dwordMultiplied(mixed);
				for (LONG x = 0; x < size.cx; x++) {
					section.SetPixel(x, y, valFill);
				}
			}
		} else {
			for (LONG x = 0; x < size.cx; x++) {
				// Find x/width proportional colour
				const XYPOSITION proportion = x / (rc.Width() - 1.0f);
				const ColourRGBA mixed = GradientValue(stops, proportion);
				const DWORD valFill = dwordMultiplied(mixed);
				for (LONG y = 0; y < size.cy; y++) {
					section.SetPixel(x, y, valFill);
				}
			}
		}

		GdiAlphaBlend(hdc, rcw.left, rcw.top, size.cx, size.cy, section.DC(), 0, 0, size.cx, size.cy, mergeAlpha);
	}
}

void SurfaceGDI::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	if (rc.Width() > 0) {
		if (rc.Width() > width)
			rc.left += std::floor((rc.Width() - width) / 2);
		rc.right = rc.left + width;
		if (rc.Height() > height)
			rc.top += std::floor((rc.Height() - height) / 2);
		rc.bottom = rc.top + height;

		const SIZE size { width, height };
		DIBSection section(hdc, size);
		if (section) {
			RGBAImage::BGRAFromRGBA(section.Bytes(), pixelsImage, static_cast<size_t>(width) * height);
			GdiAlphaBlend(hdc, static_cast<int>(rc.left), static_cast<int>(rc.top),
				static_cast<int>(rc.Width()), static_cast<int>(rc.Height()), section.DC(),
				0, 0, width, height, mergeAlpha);
		}
	}
}

void SurfaceGDI::Ellipse(PRectangle rc, FillStroke fillStroke) {
	PenColour(fillStroke.stroke.colour, fillStroke.stroke.width);
	BrushColour(fillStroke.fill.colour);
	const RECT rcw = RectFromPRectangle(rc);
	::Ellipse(hdc, rcw.left, rcw.top, rcw.right, rcw.bottom);
}

void SurfaceGDI::Stadium(PRectangle rc, FillStroke fillStroke, [[maybe_unused]] Ends ends) {
	// TODO: Implement properly - the rectangle is just a placeholder
	RectangleDraw(rc, fillStroke);
}

void SurfaceGDI::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	::BitBlt(hdc,
		static_cast<int>(rc.left), static_cast<int>(rc.top),
		static_cast<int>(rc.Width()), static_cast<int>(rc.Height()),
		dynamic_cast<SurfaceGDI &>(surfaceSource).hdc,
		static_cast<int>(from.x), static_cast<int>(from.y), SRCCOPY);
}

std::unique_ptr<IScreenLineLayout> SurfaceGDI::Layout(const IScreenLine *) {
	return {};
}

using TextPositionsI = VarBuffer<int, stackBufferLength>;

void SurfaceGDI::DrawTextCommon(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, UINT fuOptions) {
	SetFont(font_);
	const RECT rcw = RectFromPRectangle(rc);
	const int x = static_cast<int>(rc.left);
	const int yBaseInt = static_cast<int>(ybase);

	if (mode.codePage == CpUtf8) {
		const TextWide tbuf(text, mode.codePage);
		::ExtTextOutW(hdc, x, yBaseInt, fuOptions, &rcw, tbuf.buffer, tbuf.tlen, nullptr);
	} else {
		::ExtTextOutA(hdc, x, yBaseInt, fuOptions, &rcw, text.data(), static_cast<UINT>(text.length()), nullptr);
	}
}

void SurfaceGDI::DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	::SetTextColor(hdc, fore.OpaqueRGB());
	::SetBkColor(hdc, back.OpaqueRGB());
	DrawTextCommon(rc, font_, ybase, text, ETO_OPAQUE);
}

void SurfaceGDI::DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	::SetTextColor(hdc, fore.OpaqueRGB());
	::SetBkColor(hdc, back.OpaqueRGB());
	DrawTextCommon(rc, font_, ybase, text, ETO_OPAQUE | ETO_CLIPPED);
}

void SurfaceGDI::DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore) {
	// Avoid drawing spaces in transparent mode
	for (const char ch : text) {
		if (ch != ' ') {
			::SetTextColor(hdc, fore.OpaqueRGB());
			::SetBkMode(hdc, TRANSPARENT);
			DrawTextCommon(rc, font_, ybase, text, 0);
			::SetBkMode(hdc, OPAQUE);
			return;
		}
	}
}

void SurfaceGDI::MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) {
	// Zero positions to avoid random behaviour on failure.
	std::fill(positions, positions + text.length(), 0.0f);
	SetFont(font_);
	SIZE sz = { 0,0 };
	int fit = 0;
	int i = 0;
	const int len = static_cast<int>(text.length());
	if (mode.codePage == CpUtf8) {
		const TextWide tbuf(text, mode.codePage);
		TextPositionsI poses(tbuf.tlen);
		if (!::GetTextExtentExPointW(hdc, tbuf.buffer, tbuf.tlen, maxWidthMeasure, &fit, poses.buffer, &sz)) {
			// Failure
			return;
		}
		// Map the widths given for UTF-16 characters back onto the UTF-8 input string
		for (int ui = 0; ui < fit; ui++) {
			const unsigned char uch = text[i];
			const unsigned int byteCount = UTF8BytesOfLead[uch];
			if (byteCount == 4) {	// Non-BMP
				ui++;
			}
			for (unsigned int bytePos = 0; (bytePos < byteCount) && (i < len); bytePos++) {
				positions[i++] = static_cast<XYPOSITION>(poses.buffer[ui]);
			}
		}
	} else {
		TextPositionsI poses(len);
		if (!::GetTextExtentExPointA(hdc, text.data(), len, maxWidthMeasure, &fit, poses.buffer, &sz)) {
			// Eeek - a NULL DC or other foolishness could cause this.
			return;
		}
		while (i < fit) {
			positions[i] = static_cast<XYPOSITION>(poses.buffer[i]);
			i++;
		}
	}
	// If any positions not filled in then use the last position for them
	const XYPOSITION lastPos = (fit > 0) ? positions[fit - 1] : 0.0f;
	std::fill(positions + i, positions + text.length(), lastPos);
}

XYPOSITION SurfaceGDI::WidthText(const Font *font_, std::string_view text) {
	SetFont(font_);
	SIZE sz = { 0,0 };
	if (!(mode.codePage == CpUtf8)) {
		::GetTextExtentPoint32A(hdc, text.data(), std::min(static_cast<int>(text.length()), maxLenText), &sz);
	} else {
		const TextWide tbuf(text, mode.codePage);
		::GetTextExtentPoint32W(hdc, tbuf.buffer, tbuf.tlen, &sz);
	}
	return static_cast<XYPOSITION>(sz.cx);
}

void SurfaceGDI::DrawTextCommonUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, UINT fuOptions) {
	SetFont(font_);
	const RECT rcw = RectFromPRectangle(rc);
	const int x = static_cast<int>(rc.left);
	const int yBaseInt = static_cast<int>(ybase);

	const TextWide tbuf(text, CpUtf8);
	::ExtTextOutW(hdc, x, yBaseInt, fuOptions, &rcw, tbuf.buffer, tbuf.tlen, nullptr);
}

void SurfaceGDI::DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	::SetTextColor(hdc, fore.OpaqueRGB());
	::SetBkColor(hdc, back.OpaqueRGB());
	DrawTextCommonUTF8(rc, font_, ybase, text, ETO_OPAQUE);
}

void SurfaceGDI::DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	::SetTextColor(hdc, fore.OpaqueRGB());
	::SetBkColor(hdc, back.OpaqueRGB());
	DrawTextCommonUTF8(rc, font_, ybase, text, ETO_OPAQUE | ETO_CLIPPED);
}

void SurfaceGDI::DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore) {
	// Avoid drawing spaces in transparent mode
	for (const char ch : text) {
		if (ch != ' ') {
			::SetTextColor(hdc, fore.OpaqueRGB());
			::SetBkMode(hdc, TRANSPARENT);
			DrawTextCommonUTF8(rc, font_, ybase, text, 0);
			::SetBkMode(hdc, OPAQUE);
			return;
		}
	}
}

void SurfaceGDI::MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) {
	// Zero positions to avoid random behaviour on failure.
	std::fill(positions, positions + text.length(), 0.0f);
	SetFont(font_);
	SIZE sz = { 0,0 };
	int fit = 0;
	int i = 0;
	const int len = static_cast<int>(text.length());
	const TextWide tbuf(text, CpUtf8);
	TextPositionsI poses(tbuf.tlen);
	if (!::GetTextExtentExPointW(hdc, tbuf.buffer, tbuf.tlen, maxWidthMeasure, &fit, poses.buffer, &sz)) {
		// Failure
		return;
	}
	// Map the widths given for UTF-16 characters back onto the UTF-8 input string
	for (int ui = 0; ui < fit; ui++) {
		const unsigned char uch = text[i];
		const unsigned int byteCount = UTF8BytesOfLead[uch];
		if (byteCount == 4) {	// Non-BMP
			ui++;
		}
		for (unsigned int bytePos = 0; (bytePos < byteCount) && (i < len); bytePos++) {
			positions[i++] = static_cast<XYPOSITION>(poses.buffer[ui]);
		}
	}
	// If any positions not filled in then use the last position for them
	const XYPOSITION lastPos = (fit > 0) ? positions[fit - 1] : 0.0f;
	std::fill(positions + i, positions + text.length(), lastPos);
}

XYPOSITION SurfaceGDI::WidthTextUTF8(const Font *font_, std::string_view text) {
	SetFont(font_);
	SIZE sz = { 0,0 };
	const TextWide tbuf(text, CpUtf8);
	::GetTextExtentPoint32W(hdc, tbuf.buffer, tbuf.tlen, &sz);
	return static_cast<XYPOSITION>(sz.cx);
}

XYPOSITION SurfaceGDI::Ascent(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmAscent);
}

XYPOSITION SurfaceGDI::Descent(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmDescent);
}

XYPOSITION SurfaceGDI::InternalLeading(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmInternalLeading);
}

XYPOSITION SurfaceGDI::Height(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmHeight);
}

XYPOSITION SurfaceGDI::AverageCharWidth(const Font *font_) {
	SetFont(font_);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	return static_cast<XYPOSITION>(tm.tmAveCharWidth);
}

void SurfaceGDI::SetClip(PRectangle rc) {
	::SaveDC(hdc);
	::IntersectClipRect(hdc, static_cast<int>(rc.left), static_cast<int>(rc.top),
		static_cast<int>(rc.right), static_cast<int>(rc.bottom));
}

void SurfaceGDI::PopClip() {
	::RestoreDC(hdc, -1);
}

void SurfaceGDI::FlushCachedState() {
	pen = {};
	brush = {};
}

void SurfaceGDI::FlushDrawing() {
}

#if defined(USE_D2D)

namespace {

constexpr D2D1_RECT_F RectangleFromPRectangle(PRectangle rc) noexcept {
	return {
		static_cast<FLOAT>(rc.left),
		static_cast<FLOAT>(rc.top),
		static_cast<FLOAT>(rc.right),
		static_cast<FLOAT>(rc.bottom)
	};
}

constexpr D2D1_POINT_2F DPointFromPoint(Point point) noexcept {
	return { static_cast<FLOAT>(point.x), static_cast<FLOAT>(point.y) };
}

constexpr Supports SupportsD2D[] = {
	Supports::LineDrawsFinal,
	Supports::FractionalStrokeWidth,
	Supports::TranslucentStroke,
	Supports::PixelModification,
	Supports::ThreadSafeMeasureWidths,
};

constexpr D2D1_RECT_F RectangleInset(D2D1_RECT_F rect, FLOAT inset) noexcept {
	return D2D1_RECT_F{
		rect.left + inset,
		rect.top + inset,
		rect.right - inset,
		rect.bottom - inset };
}

}

class BlobInline;

class SurfaceD2D : public Surface, public ISetRenderingParams {
	SurfaceMode mode;

	// Text measuring surface: both pRenderTarget and pBitmapRenderTarget are null.
	// Window surface: pRenderTarget is valid but not pBitmapRenderTarget.
	// Bitmap drawing surface: both pRenderTarget and pBitmapRenderTarget are valid and the same.
	ComPtr<ID2D1RenderTarget> pRenderTarget;
	ComPtr<ID2D1BitmapRenderTarget> pBitmapRenderTarget;
	int clipsActive = 0;

	BrushSolid pBrush = nullptr;

	static constexpr FontQuality invalidFontQuality = FontQuality::QualityMask;
	FontQuality fontQuality = invalidFontQuality;
	int logPixelsY = USER_DEFAULT_SCREEN_DPI;
	int deviceScaleFactor = 1;
	std::shared_ptr<RenderingParams> renderingParams;

	void Clear() noexcept;
	void SetFontQuality(FontQuality extraFontFlag);
	HRESULT GetBitmap(ID2D1Bitmap **ppBitmap);
	void SetDeviceScaleFactor(const ID2D1RenderTarget *const pD2D1RenderTarget) noexcept;

public:
	SurfaceD2D() noexcept = default;
	SurfaceD2D(ID2D1RenderTarget *pRenderTargetCompatible, int width, int height, SurfaceMode mode_, int logPixelsY_) noexcept;
	// Deleted so SurfaceD2D objects can not be copied.
	SurfaceD2D(const SurfaceD2D &) = delete;
	SurfaceD2D(SurfaceD2D &&) = delete;
	SurfaceD2D &operator=(const SurfaceD2D &) = delete;
	SurfaceD2D &operator=(SurfaceD2D &&) = delete;
	~SurfaceD2D() noexcept override;

	void SetScale(WindowID wid) noexcept;
	void Init(WindowID wid) override;
	void Init(SurfaceID sid, WindowID wid) override;
	std::unique_ptr<Surface> AllocatePixMap(int width, int height) override;

	void SetMode(SurfaceMode mode_) override;

	void Release() noexcept override;
	int SupportsFeature(Supports feature) noexcept override;
	bool Initialised() override;

	void D2DPenColourAlpha(ColourRGBA fore) noexcept;
	int LogPixelsY() override;
	int PixelDivisions() override;
	int DeviceHeightFont(int points) override;
	void LineDraw(Point start, Point end, Stroke stroke) override;
	static Geometry GeometricFigure(const Point *pts, size_t npts, D2D1_FIGURE_BEGIN figureBegin) noexcept;
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

	void DrawTextCommon(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, int codePageOverride, UINT fuOptions);

	void DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore, ColourRGBA back) override;
	void DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, ColourRGBA fore) override;
	void MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) override;
	XYPOSITION WidthText(const Font *font_, std::string_view text) override;

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

	void SetRenderingParams(std::shared_ptr<RenderingParams> renderingParams_) override;
};

SurfaceD2D::SurfaceD2D(ID2D1RenderTarget *pRenderTargetCompatible, int width, int height, SurfaceMode mode_, int logPixelsY_) noexcept {
	const D2D1_SIZE_F desiredSize = D2D1::SizeF(static_cast<float>(width), static_cast<float>(height));
	D2D1_PIXEL_FORMAT desiredFormat;
#ifdef __MINGW32__
	desiredFormat.format = DXGI_FORMAT_UNKNOWN;
#else
	desiredFormat = pRenderTargetCompatible->GetPixelFormat();
#endif
	desiredFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
	const HRESULT hr = pRenderTargetCompatible->CreateCompatibleRenderTarget(
		&desiredSize, nullptr, &desiredFormat, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, pBitmapRenderTarget.GetAddressOf());
	if (SUCCEEDED(hr)) {
		pRenderTarget = pBitmapRenderTarget;
		SetDeviceScaleFactor(pRenderTarget.Get());
		pRenderTarget->BeginDraw();
	}
	mode = mode_;
	logPixelsY = logPixelsY_;
}

SurfaceD2D::~SurfaceD2D() noexcept {
	Clear();
}

void SurfaceD2D::Clear() noexcept {
	pBrush = nullptr;
	if (pRenderTarget) {
		while (clipsActive) {
			pRenderTarget->PopAxisAlignedClip();
			clipsActive--;
		}
		if (pBitmapRenderTarget) {
			pRenderTarget->EndDraw();
		}
	}
	pRenderTarget = nullptr;
	pBitmapRenderTarget = nullptr;
}

void SurfaceD2D::Release() noexcept {
	Clear();
}

void SurfaceD2D::SetScale(WindowID wid) noexcept {
	fontQuality = invalidFontQuality;
	logPixelsY = DpiForWindow(wid);
}

int SurfaceD2D::SupportsFeature(Supports feature) noexcept {
	for (const Supports f : SupportsD2D) {
		if (f == feature)
			return 1;
	}
	return 0;
}

bool SurfaceD2D::Initialised() {
	return pRenderTarget;
}

void SurfaceD2D::Init(WindowID wid) {
	Release();
	SetScale(wid);
}

void SurfaceD2D::Init(SurfaceID sid, WindowID wid) {
	Release();
	SetScale(wid);
	pRenderTarget = static_cast<ID2D1RenderTarget *>(sid);
	SetDeviceScaleFactor(pRenderTarget.Get());
}

std::unique_ptr<Surface> SurfaceD2D::AllocatePixMap(int width, int height) {
	std::unique_ptr<SurfaceD2D> surf = std::make_unique<SurfaceD2D>(pRenderTarget.Get(), width, height, mode, logPixelsY);
	surf->SetRenderingParams(renderingParams);
	return surf;
}

void SurfaceD2D::SetMode(SurfaceMode mode_) {
	mode = mode_;
}

HRESULT SurfaceD2D::GetBitmap(ID2D1Bitmap **ppBitmap) {
	PLATFORM_ASSERT(pBitmapRenderTarget);
	return pBitmapRenderTarget->GetBitmap(ppBitmap);
}

void SurfaceD2D::D2DPenColourAlpha(ColourRGBA fore) noexcept {
	if (pRenderTarget) {
		const D2D_COLOR_F col = ColorFromColourAlpha(fore);
		if (pBrush) {
			pBrush->SetColor(col);
		} else {
			const HRESULT hr = pRenderTarget->CreateSolidColorBrush(col, &pBrush);
			if (!SUCCEEDED(hr)) {
				pBrush = nullptr;
			}
		}
	}
}

void SurfaceD2D::SetFontQuality(FontQuality extraFontFlag) {
	if ((fontQuality != extraFontFlag) && renderingParams) {
		fontQuality = extraFontFlag;
		const D2D1_TEXT_ANTIALIAS_MODE aaMode = DWriteMapFontQuality(extraFontFlag);
		if (aaMode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE && renderingParams->customRenderingParams) {
			pRenderTarget->SetTextRenderingParams(renderingParams->customRenderingParams.Get());
		} else if (renderingParams->defaultRenderingParams) {
			pRenderTarget->SetTextRenderingParams(renderingParams->defaultRenderingParams.Get());
		}
		pRenderTarget->SetTextAntialiasMode(aaMode);
	}
}

int SurfaceD2D::LogPixelsY() {
	return logPixelsY;
}

void SurfaceD2D::SetDeviceScaleFactor(const ID2D1RenderTarget *const pD2D1RenderTarget) noexcept {
	FLOAT dpiX = 0.f;
	FLOAT dpiY = 0.f;
	pD2D1RenderTarget->GetDpi(&dpiX, &dpiY);
	deviceScaleFactor = static_cast<int>(dpiX / 96.f);
}

int SurfaceD2D::PixelDivisions() {
	return deviceScaleFactor;
}

int SurfaceD2D::DeviceHeightFont(int points) {
	return ::MulDiv(points, LogPixelsY(), 72);
}

void SurfaceD2D::LineDraw(Point start, Point end, Stroke stroke) {
	D2DPenColourAlpha(stroke.colour);

	D2D1_STROKE_STYLE_PROPERTIES strokeProps {};
	strokeProps.startCap = D2D1_CAP_STYLE_SQUARE;
	strokeProps.endCap = D2D1_CAP_STYLE_SQUARE;
	strokeProps.dashCap = D2D1_CAP_STYLE_FLAT;
	strokeProps.lineJoin = D2D1_LINE_JOIN_MITER;
	strokeProps.miterLimit = 4.0f;
	strokeProps.dashStyle = D2D1_DASH_STYLE_SOLID;
	strokeProps.dashOffset = 0;

	// get the stroke style to apply
	if (const StrokeStyle pStrokeStyle = StrokeStyleCreate(strokeProps)) {
		pRenderTarget->DrawLine(
			DPointFromPoint(start),
			DPointFromPoint(end), pBrush.Get(), stroke.WidthF(), pStrokeStyle.Get());
	}
}

Geometry SurfaceD2D::GeometricFigure(const Point *pts, size_t npts, D2D1_FIGURE_BEGIN figureBegin) noexcept {
	Geometry geometry = GeometryCreate();
	if (geometry) {
		if (const GeometrySink sink = GeometrySinkCreate(geometry.Get())) {
			sink->BeginFigure(DPointFromPoint(pts[0]), figureBegin);
			for (size_t i = 1; i < npts; i++) {
				sink->AddLine(DPointFromPoint(pts[i]));
			}
			sink->EndFigure((figureBegin == D2D1_FIGURE_BEGIN_FILLED) ?
				D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
			sink->Close();
		}
	}
	return geometry;
}

void SurfaceD2D::PolyLine(const Point *pts, size_t npts, Stroke stroke) {
	PLATFORM_ASSERT(pRenderTarget && (npts > 1));
	if (!pRenderTarget || (npts <= 1)) {
		return;
	}

	const Geometry geometry = GeometricFigure(pts, npts, D2D1_FIGURE_BEGIN_HOLLOW);
	PLATFORM_ASSERT(geometry);
	if (!geometry) {
		return;
	}

	D2DPenColourAlpha(stroke.colour);
	D2D1_STROKE_STYLE_PROPERTIES strokeProps {};
	strokeProps.startCap = D2D1_CAP_STYLE_ROUND;
	strokeProps.endCap = D2D1_CAP_STYLE_ROUND;
	strokeProps.dashCap = D2D1_CAP_STYLE_FLAT;
	strokeProps.lineJoin = D2D1_LINE_JOIN_MITER;
	strokeProps.miterLimit = 4.0f;
	strokeProps.dashStyle = D2D1_DASH_STYLE_SOLID;
	strokeProps.dashOffset = 0;

	// get the stroke style to apply
	if (const StrokeStyle pStrokeStyle = StrokeStyleCreate(strokeProps)) {
		pRenderTarget->DrawGeometry(geometry.Get(), pBrush.Get(), stroke.WidthF(), pStrokeStyle.Get());
	}
}

void SurfaceD2D::Polygon(const Point *pts, size_t npts, FillStroke fillStroke) {
	PLATFORM_ASSERT(pRenderTarget && (npts > 2));
	if (pRenderTarget) {
		const Geometry geometry = GeometricFigure(pts, npts, D2D1_FIGURE_BEGIN_FILLED);
		PLATFORM_ASSERT(geometry);
		if (geometry) {
			D2DPenColourAlpha(fillStroke.fill.colour);
			pRenderTarget->FillGeometry(geometry.Get(), pBrush.Get());
			D2DPenColourAlpha(fillStroke.stroke.colour);
			pRenderTarget->DrawGeometry(geometry.Get(), pBrush.Get(), fillStroke.stroke.WidthF());
		}
	}
}

void SurfaceD2D::RectangleDraw(PRectangle rc, FillStroke fillStroke) {
	if (!pRenderTarget)
		return;
	const D2D1_RECT_F rect = RectangleFromPRectangle(rc);
	const D2D1_RECT_F rectFill = RectangleInset(rect, fillStroke.stroke.WidthF());
	const float halfStroke = fillStroke.stroke.WidthF() / 2.0f;
	const D2D1_RECT_F rectOutline = RectangleInset(rect, halfStroke);

	D2DPenColourAlpha(fillStroke.fill.colour);
	pRenderTarget->FillRectangle(&rectFill, pBrush.Get());
	D2DPenColourAlpha(fillStroke.stroke.colour);
	pRenderTarget->DrawRectangle(&rectOutline, pBrush.Get(), fillStroke.stroke.WidthF());
}

void SurfaceD2D::RectangleFrame(PRectangle rc, Stroke stroke) {
	if (pRenderTarget) {
		const XYPOSITION halfStroke = stroke.width / 2.0f;
		const D2D1_RECT_F rectangle1 = RectangleFromPRectangle(rc.Inset(halfStroke));
		D2DPenColourAlpha(stroke.colour);
		pRenderTarget->DrawRectangle(&rectangle1, pBrush.Get(), stroke.WidthF());
	}
}

void SurfaceD2D::FillRectangle(PRectangle rc, Fill fill) {
	if (pRenderTarget) {
		D2DPenColourAlpha(fill.colour);
		const D2D1_RECT_F rectangle = RectangleFromPRectangle(rc);
		pRenderTarget->FillRectangle(&rectangle, pBrush.Get());
	}
}

void SurfaceD2D::FillRectangleAligned(PRectangle rc, Fill fill) {
	FillRectangle(PixelAlign(rc, PixelDivisions()), fill);
}

void SurfaceD2D::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceD2D *psurfOther = dynamic_cast<SurfaceD2D *>(&surfacePattern);
	PLATFORM_ASSERT(psurfOther);
	if (!psurfOther) {
		throw std::runtime_error("SurfaceD2D::FillRectangle: wrong Surface type.");
	}
	ComPtr<ID2D1Bitmap> pBitmap;
	HRESULT hr = psurfOther->GetBitmap(pBitmap.GetAddressOf());
	if (SUCCEEDED(hr) && pBitmap) {
		ComPtr<ID2D1BitmapBrush> pBitmapBrush;
		const D2D1_BITMAP_BRUSH_PROPERTIES brushProperties =
	        D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
		// Create the bitmap brush.
		hr = pRenderTarget->CreateBitmapBrush(pBitmap.Get(), brushProperties, pBitmapBrush.GetAddressOf());
		if (SUCCEEDED(hr) && pBitmapBrush) {
			pRenderTarget->FillRectangle(
				RectangleFromPRectangle(rc),
				pBitmapBrush.Get());
		}
	}
}

void SurfaceD2D::RoundedRectangle(PRectangle rc, FillStroke fillStroke) {
	if (pRenderTarget) {
		const FLOAT minDimension = static_cast<FLOAT>(std::min(rc.Width(), rc.Height())) / 2.0f;
		const FLOAT radius = std::min(4.0f, minDimension);
		if (fillStroke.fill.colour == fillStroke.stroke.colour) {
			const D2D1_ROUNDED_RECT roundedRectFill = {
				RectangleFromPRectangle(rc),
				radius, radius };
			D2DPenColourAlpha(fillStroke.fill.colour);
			pRenderTarget->FillRoundedRectangle(roundedRectFill, pBrush.Get());
		} else {
			const D2D1_ROUNDED_RECT roundedRectFill = {
				RectangleFromPRectangle(rc.Inset(1.0)),
				radius-1, radius-1 };
			D2DPenColourAlpha(fillStroke.fill.colour);
			pRenderTarget->FillRoundedRectangle(roundedRectFill, pBrush.Get());

			const D2D1_ROUNDED_RECT roundedRect = {
				RectangleFromPRectangle(rc.Inset(0.5)),
				radius, radius };
			D2DPenColourAlpha(fillStroke.stroke.colour);
			pRenderTarget->DrawRoundedRectangle(roundedRect, pBrush.Get(), fillStroke.stroke.WidthF());
		}
	}
}

void SurfaceD2D::AlphaRectangle(PRectangle rc, XYPOSITION cornerSize, FillStroke fillStroke) {
	const D2D1_RECT_F rect = RectangleFromPRectangle(rc);
	const D2D1_RECT_F rectFill = RectangleInset(rect, fillStroke.stroke.WidthF());
	const float halfStroke = fillStroke.stroke.WidthF() / 2.0f;
	const D2D1_RECT_F rectOutline = RectangleInset(rect, halfStroke);
	if (pRenderTarget) {
		if (cornerSize == 0) {
			// When corner size is zero, draw square rectangle to prevent blurry pixels at corners
			D2DPenColourAlpha(fillStroke.fill.colour);
			pRenderTarget->FillRectangle(rectFill, pBrush.Get());

			D2DPenColourAlpha(fillStroke.stroke.colour);
			pRenderTarget->DrawRectangle(rectOutline, pBrush.Get(), fillStroke.stroke.WidthF());
		} else {
			const float cornerSizeF = static_cast<float>(cornerSize);
			const D2D1_ROUNDED_RECT roundedRectFill = {
				rectFill, cornerSizeF - 1.0f, cornerSizeF - 1.0f };
			D2DPenColourAlpha(fillStroke.fill.colour);
			pRenderTarget->FillRoundedRectangle(roundedRectFill, pBrush.Get());

			const D2D1_ROUNDED_RECT roundedRect = {
				rectOutline, cornerSizeF, cornerSizeF};
			D2DPenColourAlpha(fillStroke.stroke.colour);
			pRenderTarget->DrawRoundedRectangle(roundedRect, pBrush.Get(), fillStroke.stroke.WidthF());
		}
	}
}

void SurfaceD2D::GradientRectangle(PRectangle rc, const std::vector<ColourStop> &stops, GradientOptions options) {
	if (pRenderTarget) {
		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES lgbp {
			DPointFromPoint(Point(rc.left, rc.top)), {}
		};
		switch (options) {
		case GradientOptions::leftToRight:
			lgbp.endPoint = DPointFromPoint(Point(rc.right, rc.top));
			break;
		case GradientOptions::topToBottom:
		default:
			lgbp.endPoint = DPointFromPoint(Point(rc.left, rc.bottom));
			break;
		}

		std::vector<D2D1_GRADIENT_STOP> gradientStops;
		for (const ColourStop &stop : stops) {
			gradientStops.push_back({ static_cast<FLOAT>(stop.position), ColorFromColourAlpha(stop.colour) });
		}
		ComPtr<ID2D1GradientStopCollection> pGradientStops;
		HRESULT hr = pRenderTarget->CreateGradientStopCollection(
			gradientStops.data(), static_cast<UINT32>(gradientStops.size()), pGradientStops.GetAddressOf());
		if (FAILED(hr) || !pGradientStops) {
			return;
		}
		ComPtr<ID2D1LinearGradientBrush> pBrushLinear;
		hr = pRenderTarget->CreateLinearGradientBrush(
			lgbp, pGradientStops.Get(), pBrushLinear.GetAddressOf());
		if (SUCCEEDED(hr) && pBrushLinear) {
			const D2D1_RECT_F rectangle = RectangleFromPRectangle(PRectangle(
				std::round(rc.left), rc.top, std::round(rc.right), rc.bottom));
			pRenderTarget->FillRectangle(&rectangle, pBrushLinear.Get());
		}
	}
}

void SurfaceD2D::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	if (pRenderTarget) {
		if (rc.Width() > width)
			rc.left += std::floor((rc.Width() - width) / 2);
		rc.right = rc.left + width;
		if (rc.Height() > height)
			rc.top += std::floor((rc.Height() - height) / 2);
		rc.bottom = rc.top + height;

		std::vector<unsigned char> image(RGBAImage::bytesPerPixel * height * width);
		RGBAImage::BGRAFromRGBA(image.data(), pixelsImage, static_cast<ptrdiff_t>(height) * width);

		ComPtr<ID2D1Bitmap> bitmap;
		const D2D1_SIZE_U size = D2D1::SizeU(width, height);
		const D2D1_BITMAP_PROPERTIES props = {{DXGI_FORMAT_B8G8R8A8_UNORM,
		    D2D1_ALPHA_MODE_PREMULTIPLIED}, 72.0, 72.0};
		const HRESULT hr = pRenderTarget->CreateBitmap(size, image.data(),
                  width * 4, &props, bitmap.GetAddressOf());
		if (SUCCEEDED(hr)) {
			const D2D1_RECT_F rcDestination = RectangleFromPRectangle(rc);
			pRenderTarget->DrawBitmap(bitmap.Get(), rcDestination);
		}
	}
}

void SurfaceD2D::Ellipse(PRectangle rc, FillStroke fillStroke) {
	if (!pRenderTarget)
		return;
	const D2D1_POINT_2F centre = DPointFromPoint(rc.Centre());

	const FLOAT radiusFill = static_cast<FLOAT>(rc.Width() / 2.0f - fillStroke.stroke.width);
	const D2D1_ELLIPSE ellipseFill = { centre, radiusFill, radiusFill };

	D2DPenColourAlpha(fillStroke.fill.colour);
	pRenderTarget->FillEllipse(ellipseFill, pBrush.Get());

	const FLOAT radiusOutline = static_cast<FLOAT>(rc.Width() / 2.0f - fillStroke.stroke.width / 2.0f);
	const D2D1_ELLIPSE ellipseOutline = { centre, radiusOutline, radiusOutline };

	D2DPenColourAlpha(fillStroke.stroke.colour);
	pRenderTarget->DrawEllipse(ellipseOutline, pBrush.Get(), fillStroke.stroke.WidthF());
}

void SurfaceD2D::Stadium(PRectangle rc, FillStroke fillStroke, Ends ends) {
	if (!pRenderTarget)
		return;
	if (rc.Width() < rc.Height()) {
		// Can't draw nice ends so just draw a rectangle
		RectangleDraw(rc, fillStroke);
		return;
	}
	const FLOAT radius = static_cast<FLOAT>(rc.Height() / 2.0);
	const FLOAT radiusFill = radius - fillStroke.stroke.WidthF();
	const FLOAT halfStroke = fillStroke.stroke.WidthF() / 2.0f;
	if (ends == Surface::Ends::semiCircles) {
		const D2D1_RECT_F rect = RectangleFromPRectangle(rc);
		const D2D1_ROUNDED_RECT roundedRectFill = { RectangleInset(rect, fillStroke.stroke.WidthF()),
			radiusFill, radiusFill };
		D2DPenColourAlpha(fillStroke.fill.colour);
		pRenderTarget->FillRoundedRectangle(roundedRectFill, pBrush.Get());

		const D2D1_ROUNDED_RECT roundedRect = { RectangleInset(rect, halfStroke),
			radius, radius };
		D2DPenColourAlpha(fillStroke.stroke.colour);
		pRenderTarget->DrawRoundedRectangle(roundedRect, pBrush.Get(), fillStroke.stroke.WidthF());
	} else {
		const Ends leftSide = static_cast<Ends>(static_cast<int>(ends) & 0xf);
		const Ends rightSide = static_cast<Ends>(static_cast<int>(ends) & 0xf0);
		PRectangle rcInner = rc;
		rcInner.left += radius;
		rcInner.right -= radius;
		const Geometry pathGeometry = GeometryCreate();
		if (!pathGeometry)
			return;
		if (const GeometrySink pSink = GeometrySinkCreate(pathGeometry.Get())) {
			switch (leftSide) {
				case Ends::leftFlat:
					pSink->BeginFigure(DPointFromPoint(Point(rc.left + halfStroke, rc.top + halfStroke)), D2D1_FIGURE_BEGIN_FILLED);
					pSink->AddLine(DPointFromPoint(Point(rc.left + halfStroke, rc.bottom - halfStroke)));
					break;
				case Ends::leftAngle:
					pSink->BeginFigure(DPointFromPoint(Point(rcInner.left + halfStroke, rc.top + halfStroke)), D2D1_FIGURE_BEGIN_FILLED);
					pSink->AddLine(DPointFromPoint(Point(rc.left + halfStroke, rc.Centre().y)));
					pSink->AddLine(DPointFromPoint(Point(rcInner.left + halfStroke, rc.bottom - halfStroke)));
					break;
				case Ends::semiCircles:
				default: {
						pSink->BeginFigure(DPointFromPoint(Point(rcInner.left + halfStroke, rc.top + halfStroke)), D2D1_FIGURE_BEGIN_FILLED);
						D2D1_ARC_SEGMENT segment{};
						segment.point = DPointFromPoint(Point(rcInner.left + halfStroke, rc.bottom - halfStroke));
						segment.size = D2D1::SizeF(radiusFill, radiusFill);
						segment.rotationAngle = 0.0f;
						segment.sweepDirection = D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
						segment.arcSize = D2D1_ARC_SIZE_SMALL;
						pSink->AddArc(segment);
					}
					break;
			}

			switch (rightSide) {
			case Ends::rightFlat:
				pSink->AddLine(DPointFromPoint(Point(rc.right - halfStroke, rc.bottom - halfStroke)));
				pSink->AddLine(DPointFromPoint(Point(rc.right - halfStroke, rc.top + halfStroke)));
				break;
			case Ends::rightAngle:
				pSink->AddLine(DPointFromPoint(Point(rcInner.right - halfStroke, rc.bottom - halfStroke)));
				pSink->AddLine(DPointFromPoint(Point(rc.right - halfStroke, rc.Centre().y)));
				pSink->AddLine(DPointFromPoint(Point(rcInner.right - halfStroke, rc.top + halfStroke)));
				break;
			case Ends::semiCircles:
			default: {
					pSink->AddLine(DPointFromPoint(Point(rcInner.right - halfStroke, rc.bottom - halfStroke)));
					D2D1_ARC_SEGMENT segment{};
					segment.point = DPointFromPoint(Point(rcInner.right - halfStroke, rc.top + halfStroke));
					segment.size = D2D1::SizeF(radiusFill, radiusFill);
					segment.rotationAngle = 0.0f;
					segment.sweepDirection = D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
					segment.arcSize = D2D1_ARC_SIZE_SMALL;
					pSink->AddArc(segment);
				}
				break;
			}

			pSink->EndFigure(D2D1_FIGURE_END_CLOSED);

			pSink->Close();
		}
		D2DPenColourAlpha(fillStroke.fill.colour);
		pRenderTarget->FillGeometry(pathGeometry.Get(), pBrush.Get());
		D2DPenColourAlpha(fillStroke.stroke.colour);
		pRenderTarget->DrawGeometry(pathGeometry.Get(), pBrush.Get(), fillStroke.stroke.WidthF());
	}
}

void SurfaceD2D::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceD2D &surfOther = dynamic_cast<SurfaceD2D &>(surfaceSource);
	ComPtr<ID2D1Bitmap> pBitmap;
	const HRESULT hr = surfOther.GetBitmap(pBitmap.GetAddressOf());
	if (SUCCEEDED(hr) && pBitmap) {
		const D2D1_RECT_F rcDestination = RectangleFromPRectangle(rc);
		const D2D1_RECT_F rcSource = RectangleFromPRectangle(PRectangle(
			from.x, from.y, from.x + rc.Width(), from.y + rc.Height()));
		pRenderTarget->DrawBitmap(pBitmap.Get(), rcDestination, 1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, rcSource);
	}
}

class BlobInline final : public IDWriteInlineObject {
	XYPOSITION width;

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppv) override;
	STDMETHODIMP_(ULONG)AddRef() override;
	STDMETHODIMP_(ULONG)Release() override;

	// IDWriteInlineObject
	COM_DECLSPEC_NOTHROW STDMETHODIMP Draw(
		void *clientDrawingContext,
		IDWriteTextRenderer *renderer,
		FLOAT originX,
		FLOAT originY,
		BOOL isSideways,
		BOOL isRightToLeft,
		IUnknown *clientDrawingEffect
		) override;
	COM_DECLSPEC_NOTHROW STDMETHODIMP GetMetrics(DWRITE_INLINE_OBJECT_METRICS *metrics) override;
	COM_DECLSPEC_NOTHROW STDMETHODIMP GetOverhangMetrics(DWRITE_OVERHANG_METRICS *overhangs) override;
	COM_DECLSPEC_NOTHROW STDMETHODIMP GetBreakConditions(
		DWRITE_BREAK_CONDITION *breakConditionBefore,
		DWRITE_BREAK_CONDITION *breakConditionAfter) override;
public:
	explicit BlobInline(XYPOSITION width_=0.0f) noexcept : width(width_) {
	}
};

/// Implement IUnknown
STDMETHODIMP BlobInline::QueryInterface(REFIID riid, PVOID *ppv) {
	if (!ppv)
		return E_POINTER;
	// Never called so not checked.
	*ppv = nullptr;
	if (riid == IID_IUnknown)
		*ppv = this;
	if (riid == __uuidof(IDWriteInlineObject))
		*ppv = this;
	if (!*ppv)
		return E_NOINTERFACE;
	return S_OK;
}

STDMETHODIMP_(ULONG) BlobInline::AddRef() {
	// Lifetime tied to Platform methods so ignore any reference operations.
	return 1;
}

STDMETHODIMP_(ULONG) BlobInline::Release() {
	// Lifetime tied to Platform methods so ignore any reference operations.
	return 1;
}

/// Implement IDWriteInlineObject
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE BlobInline::Draw(
	void*,
	IDWriteTextRenderer*,
	FLOAT,
	FLOAT,
	BOOL,
	BOOL,
	IUnknown*) {
	// Since not performing drawing, not necessary to implement
	// Could be implemented by calling back into platform-independent code.
	// This would allow more of the drawing to be mediated through DirectWrite.
	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE BlobInline::GetMetrics(
	DWRITE_INLINE_OBJECT_METRICS *metrics
) {
	if (!metrics)
		return E_POINTER;
	metrics->width = static_cast<FLOAT>(width);
	metrics->height = 2;
	metrics->baseline = 1;
	metrics->supportsSideways = FALSE;
	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE BlobInline::GetOverhangMetrics(
	DWRITE_OVERHANG_METRICS *overhangs
) {
	if (!overhangs)
		return E_POINTER;
	overhangs->left = 0;
	overhangs->top = 0;
	overhangs->right = 0;
	overhangs->bottom = 0;
	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE BlobInline::GetBreakConditions(
	DWRITE_BREAK_CONDITION *breakConditionBefore,
	DWRITE_BREAK_CONDITION *breakConditionAfter
) {
	if (!breakConditionBefore || !breakConditionAfter)
		return E_POINTER;
	// Since not performing 2D layout, not necessary to implement
	*breakConditionBefore = DWRITE_BREAK_CONDITION_NEUTRAL;
	*breakConditionAfter = DWRITE_BREAK_CONDITION_NEUTRAL;
	return S_OK;
}

class ScreenLineLayout : public IScreenLineLayout {
	TextLayout textLayout;
	std::string text;
	std::wstring buffer;
	std::vector<BlobInline> blobs;
	static void FillTextLayoutFormats(const IScreenLine *screenLine, IDWriteTextLayout *textLayout, std::vector<BlobInline> &blobs);
	static std::wstring ReplaceRepresentation(std::string_view text);
	static size_t GetPositionInLayout(std::string_view text, size_t position);
public:
	explicit ScreenLineLayout(const IScreenLine *screenLine);
	// Deleted so ScreenLineLayout objects can not be copied
	ScreenLineLayout(const ScreenLineLayout &) = delete;
	ScreenLineLayout(ScreenLineLayout &&) = delete;
	ScreenLineLayout &operator=(const ScreenLineLayout &) = delete;
	ScreenLineLayout &operator=(ScreenLineLayout &&) = delete;
	~ScreenLineLayout() noexcept override = default;
	size_t PositionFromX(XYPOSITION xDistance, bool charPosition) override;
	XYPOSITION XFromPosition(size_t caretPosition) override;
	std::vector<Interval> FindRangeIntervals(size_t start, size_t end) override;
};

// Each char can have its own style, so we fill the textLayout with the textFormat of each char

void ScreenLineLayout::FillTextLayoutFormats(const IScreenLine *screenLine, IDWriteTextLayout *textLayout, std::vector<BlobInline> &blobs) {
	// Reserve enough entries up front so they are not moved and the pointers handed
	// to textLayout remain valid.
	const ptrdiff_t numRepresentations = screenLine->RepresentationCount();
	std::string_view text = screenLine->Text();
	const ptrdiff_t numTabs = std::count(std::begin(text), std::end(text), '\t');
	blobs.reserve(numRepresentations + numTabs);

	UINT32 layoutPosition = 0;

	for (size_t bytePosition = 0; bytePosition < screenLine->Length();) {
		const unsigned char uch = screenLine->Text()[bytePosition];
		const unsigned int byteCount = UTF8BytesOfLead[uch];
		const UINT32 codeUnits = UTF16LengthFromUTF8ByteCount(byteCount);
		const DWRITE_TEXT_RANGE textRange = { layoutPosition, codeUnits };

		XYPOSITION representationWidth = screenLine->RepresentationWidth(bytePosition);
		if ((representationWidth == 0.0f) && (screenLine->Text()[bytePosition] == '\t')) {
			D2D1_POINT_2F realPt {};
			DWRITE_HIT_TEST_METRICS realCaretMetrics {};
			textLayout->HitTestTextPosition(
				layoutPosition,
				false, // trailing if false, else leading edge
				&realPt.x,
				&realPt.y,
				&realCaretMetrics
			);

			const XYPOSITION nextTab = screenLine->TabPositionAfter(realPt.x);
			representationWidth = nextTab - realPt.x;
		}
		if (representationWidth > 0.0f) {
			blobs.push_back(BlobInline(representationWidth));
			textLayout->SetInlineObject(&blobs.back(), textRange);
		};

		const FontDirectWrite *pfm =
			dynamic_cast<const FontDirectWrite *>(screenLine->FontOfPosition(bytePosition));
		if (!pfm) {
			throw std::runtime_error("FillTextLayoutFormats: wrong Font type.");
		}

		const unsigned int fontFamilyNameSize = pfm->pTextFormat->GetFontFamilyNameLength();
		std::wstring fontFamilyName(fontFamilyNameSize, 0);
		const HRESULT hrFamily = pfm->pTextFormat->GetFontFamilyName(fontFamilyName.data(), fontFamilyNameSize + 1);
		if (SUCCEEDED(hrFamily)) {
			textLayout->SetFontFamilyName(fontFamilyName.c_str(), textRange);
		}

		textLayout->SetFontSize(pfm->pTextFormat->GetFontSize(), textRange);
		textLayout->SetFontWeight(pfm->pTextFormat->GetFontWeight(), textRange);
		textLayout->SetFontStyle(pfm->pTextFormat->GetFontStyle(), textRange);

		const unsigned int localeNameSize = pfm->pTextFormat->GetLocaleNameLength();
		std::wstring localeName(localeNameSize, 0);
		const HRESULT hrLocale = pfm->pTextFormat->GetLocaleName(localeName.data(), localeNameSize + 1);
		if (SUCCEEDED(hrLocale)) {
			textLayout->SetLocaleName(localeName.c_str(), textRange);
		}

		textLayout->SetFontStretch(pfm->pTextFormat->GetFontStretch(), textRange);

		IDWriteFontCollection *fontCollection = nullptr;
		if (SUCCEEDED(pfm->pTextFormat->GetFontCollection(&fontCollection))) {
			textLayout->SetFontCollection(fontCollection, textRange);
		}

		bytePosition += byteCount;
		layoutPosition += codeUnits;
	}

}

/* Convert to a wide character string and replace tabs with X to stop DirectWrite tab expansion */

std::wstring ScreenLineLayout::ReplaceRepresentation(std::string_view text) {
	const TextWide wideText(text, CpUtf8);
	std::wstring ws(wideText.buffer, wideText.tlen);
	std::replace(ws.begin(), ws.end(), L'\t', L'X');
	return ws;
}

// Finds the position in the wide character version of the text.

size_t ScreenLineLayout::GetPositionInLayout(std::string_view text, size_t position) {
	const std::string_view textUptoPosition = text.substr(0, position);
	return UTF16Length(textUptoPosition);
}

ScreenLineLayout::ScreenLineLayout(const IScreenLine *screenLine) {
	// If the text is empty, then no need to go through this function
	if (!screenLine || !screenLine->Length())
		return;

	text = screenLine->Text();

	// Get textFormat
	const FontDirectWrite *pfm = FontDirectWrite::Cast(screenLine->FontOfPosition(0));
	if (!pfm->pTextFormat) {
		return;
	}

	// Convert the string to wstring and replace the original control characters with their representative chars.
	buffer = ReplaceRepresentation(screenLine->Text());

	// Create a text layout
	textLayout = LayoutCreate(
		buffer,
		pfm->pTextFormat.Get(),
		static_cast<FLOAT>(screenLine->Width()),
		static_cast<FLOAT>(screenLine->Height()));
	if (!textLayout) {
		return;
	}

	// Fill the textLayout chars with their own formats
	FillTextLayoutFormats(screenLine, textLayout.Get(), blobs);
}

// Get the position from the provided x

size_t ScreenLineLayout::PositionFromX(XYPOSITION xDistance, bool charPosition) {
	if (!textLayout) {
		return 0;
	}

	// Returns the text position corresponding to the mouse x,y.
	// If hitting the trailing side of a cluster, return the
	// leading edge of the following text position.

	BOOL isTrailingHit = FALSE;
	BOOL isInside = FALSE;
	DWRITE_HIT_TEST_METRICS caretMetrics {};

	textLayout->HitTestPoint(
		static_cast<FLOAT>(xDistance),
		0.0f,
		&isTrailingHit,
		&isInside,
		&caretMetrics
	);

	DWRITE_HIT_TEST_METRICS hitTestMetrics {};
	if (isTrailingHit) {
		FLOAT caretX = 0.0f;
		FLOAT caretY = 0.0f;

		// Uses hit-testing to align the current caret position to a whole cluster,
		// rather than residing in the middle of a base character + diacritic,
		// surrogate pair, or character + UVS.

		// Align the caret to the nearest whole cluster.
		textLayout->HitTestTextPosition(
			caretMetrics.textPosition,
			false,
			&caretX,
			&caretY,
			&hitTestMetrics
		);
	}

	size_t pos;
	if (charPosition) {
		pos = isTrailingHit ? hitTestMetrics.textPosition : caretMetrics.textPosition;
	} else {
		pos = isTrailingHit ? static_cast<size_t>(hitTestMetrics.textPosition) + hitTestMetrics.length : caretMetrics.textPosition;
	}

	// Get the character position in original string
	return UTF8PositionFromUTF16Position(text, pos);
}

// Finds the point of the caret position

XYPOSITION ScreenLineLayout::XFromPosition(size_t caretPosition) {
	if (!textLayout) {
		return 0.0;
	}
	// Convert byte positions to wchar_t positions
	const size_t position = GetPositionInLayout(text, caretPosition);

	// Translate text character offset to point x,y.
	DWRITE_HIT_TEST_METRICS caretMetrics {};
	D2D1_POINT_2F pt {};

	textLayout->HitTestTextPosition(
		static_cast<UINT32>(position),
		false, // trailing if false, else leading edge
		&pt.x,
		&pt.y,
		&caretMetrics
	);

	return pt.x;
}

// Find the selection range rectangles

std::vector<Interval> ScreenLineLayout::FindRangeIntervals(size_t start, size_t end) {
	std::vector<Interval> ret;

	if (!textLayout || (start == end)) {
		return ret;
	}

	// Convert byte positions to wchar_t positions
	const size_t startPos = GetPositionInLayout(text, start);
	const size_t endPos = GetPositionInLayout(text, end);

	// Find selection range length
	const size_t rangeLength = (endPos > startPos) ? (endPos - startPos) : (startPos - endPos);

	// Determine actual number of hit-test ranges
	UINT32 actualHitTestCount = 0;

	// First try with 2 elements and if more needed, allocate.
	std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(2);
	textLayout->HitTestTextRange(
		static_cast<UINT32>(startPos),
		static_cast<UINT32>(rangeLength),
		0, // x
		0, // y
		hitTestMetrics.data(),
		static_cast<UINT32>(hitTestMetrics.size()),
		&actualHitTestCount
	);

	if (actualHitTestCount == 0) {
		return ret;
	}

	if (hitTestMetrics.size() < actualHitTestCount) {
		// Allocate enough room to return all hit-test metrics.
		hitTestMetrics.resize(actualHitTestCount);
		textLayout->HitTestTextRange(
			static_cast<UINT32>(startPos),
			static_cast<UINT32>(rangeLength),
			0, // x
			0, // y
			hitTestMetrics.data(),
			static_cast<UINT32>(hitTestMetrics.size()),
			&actualHitTestCount
		);
	}

	// Get the selection ranges behind the text.

	for (size_t i = 0; i < actualHitTestCount; ++i) {
		// Store selection rectangle
		const DWRITE_HIT_TEST_METRICS &htm = hitTestMetrics[i];
		const Interval selectionInterval { htm.left, htm.left + htm.width };
		ret.push_back(selectionInterval);
	}

	return ret;
}

std::unique_ptr<IScreenLineLayout> SurfaceD2D::Layout(const IScreenLine *screenLine) {
	return std::make_unique<ScreenLineLayout>(screenLine);
}

void SurfaceD2D::DrawTextCommon(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text, int codePageOverride, UINT fuOptions) {
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	if (pfm->pTextFormat && pRenderTarget && pBrush) {
		// Use Unicode calls
		const int codePageDraw = codePageOverride ? codePageOverride : pfm->CodePageText(mode.codePage);
		const TextWide tbuf(text, codePageDraw);

		SetFontQuality(pfm->extraFontFlag);
		if (fuOptions & ETO_CLIPPED) {
			const D2D1_RECT_F rcClip = RectangleFromPRectangle(rc);
			pRenderTarget->PushAxisAlignedClip(rcClip, D2D1_ANTIALIAS_MODE_ALIASED);
		}

		// Explicitly creating a text layout appears a little faster
		TextLayout pTextLayout = LayoutCreate(
				tbuf.AsView(),
				pfm->pTextFormat.Get(),
				static_cast<FLOAT>(rc.Width()),
				static_cast<FLOAT>(rc.Height()));
		if (pTextLayout) {
			const D2D1_POINT_2F origin = DPointFromPoint(Point(rc.left, ybase - pfm->yAscent));
			pRenderTarget->DrawTextLayout(origin, pTextLayout.Get(), pBrush.Get(), d2dDrawTextOptions);
		}

		if (fuOptions & ETO_CLIPPED) {
			pRenderTarget->PopAxisAlignedClip();
		}
	}
}

void SurfaceD2D::DrawTextNoClip(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	if (pRenderTarget) {
		FillRectangleAligned(rc, back);
		D2DPenColourAlpha(fore);
		DrawTextCommon(rc, font_, ybase, text, 0, ETO_OPAQUE);
	}
}

void SurfaceD2D::DrawTextClipped(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	if (pRenderTarget) {
		FillRectangleAligned(rc, back);
		D2DPenColourAlpha(fore);
		DrawTextCommon(rc, font_, ybase, text, 0, ETO_OPAQUE | ETO_CLIPPED);
	}
}

void SurfaceD2D::DrawTextTransparent(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore) {
	// Avoid drawing spaces in transparent mode
	for (const char ch : text) {
		if (ch != ' ') {
			if (pRenderTarget) {
				D2DPenColourAlpha(fore);
				DrawTextCommon(rc, font_, ybase, text, 0, 0);
			}
			return;
		}
	}
}

namespace {

HRESULT MeasurePositions(TextPositions &poses, const TextWide &tbuf, IDWriteTextFormat *pTextFormat) {
	if (!pTextFormat) {
		// Unexpected failure like no access to DirectWrite so give up.
		return E_FAIL;
	}

	// Initialize poses for safety.
	std::fill(poses.buffer, poses.buffer + tbuf.tlen, 0.0f);
	// Create a layout
	TextLayout pTextLayout = LayoutCreate(tbuf.AsView(), pTextFormat);
	if (!pTextLayout) {
		return E_FAIL;
	}
	VarBuffer<DWRITE_CLUSTER_METRICS, stackBufferLength> cm(tbuf.tlen);
	UINT32 count = 0;
	const HRESULT hrGetCluster = pTextLayout->GetClusterMetrics(cm.buffer, tbuf.tlen, &count);
	if (!SUCCEEDED(hrGetCluster)) {
		return hrGetCluster;
	}
	const DWRITE_CLUSTER_METRICS * const clusterMetrics = cm.buffer;
	// A cluster may be more than one WCHAR, such as for "ffi" which is a ligature in the Candara font
	XYPOSITION position = 0.0;
	int ti=0;
	for (unsigned int ci=0; ci<count; ci++) {
		for (unsigned int inCluster=0; inCluster<clusterMetrics[ci].length; inCluster++) {
			poses.buffer[ti++] = position + clusterMetrics[ci].width * (inCluster + 1) / clusterMetrics[ci].length;
		}
		position += clusterMetrics[ci].width;
	}
	PLATFORM_ASSERT(ti == tbuf.tlen);
	return S_OK;
}

}

void SurfaceD2D::MeasureWidths(const Font *font_, std::string_view text, XYPOSITION *positions) {
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	const int codePageText = pfm->CodePageText(mode.codePage);
	const TextWide tbuf(text, codePageText);
	TextPositions poses(tbuf.tlen);
	if (FAILED(MeasurePositions(poses, tbuf, pfm->pTextFormat.Get()))) {
		return;
	}
	if (codePageText == CpUtf8) {
		// Map the widths given for UTF-16 characters back onto the UTF-8 input string
		size_t i = 0;
		for (int ui = 0; ui < tbuf.tlen; ui++) {
			const unsigned char uch = text[i];
			const unsigned int byteCount = UTF8BytesOfLead[uch];
			if (byteCount == 4) {	// Non-BMP
				ui++;
			}
			for (unsigned int bytePos=0; (bytePos<byteCount) && (i<text.length()) && (ui<tbuf.tlen); bytePos++) {
				positions[i++] = poses.buffer[ui];
			}
		}
		const XYPOSITION lastPos = (i > 0) ? positions[i - 1] : 0.0;
		while (i<text.length()) {
			positions[i++] = lastPos;
		}
	} else if (!IsDBCSCodePage(codePageText)) {

		// One char per position
		PLATFORM_ASSERT(text.length() == static_cast<size_t>(tbuf.tlen));
		for (int kk=0; kk<tbuf.tlen; kk++) {
			positions[kk] = poses.buffer[kk];
		}

	} else {

		// May be one or two bytes per position
		int ui = 0;
		for (size_t i=0; i<text.length() && ui<tbuf.tlen;) {
			positions[i] = poses.buffer[ui];
			if (DBCSIsLeadByte(codePageText, text[i])) {
				positions[i+1] = poses.buffer[ui];
				i += 2;
			} else {
				i++;
			}

			ui++;
		}
	}
}

XYPOSITION SurfaceD2D::WidthText(const Font *font_, std::string_view text) {
	FLOAT width = 1.0;
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	if (pfm->pTextFormat) {
		const TextWide tbuf(text, pfm->CodePageText(mode.codePage));
		// Create a layout
		if (TextLayout pTextLayout = LayoutCreate(tbuf.AsView(), pfm->pTextFormat.Get())) {
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(pTextLayout->GetMetrics(&textMetrics)))
				width = textMetrics.widthIncludingTrailingWhitespace;
		}
	}
	return width;
}

void SurfaceD2D::DrawTextNoClipUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	if (pRenderTarget) {
		FillRectangleAligned(rc, back);
		D2DPenColourAlpha(fore);
		DrawTextCommon(rc, font_, ybase, text, CpUtf8, ETO_OPAQUE);
	}
}

void SurfaceD2D::DrawTextClippedUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore, ColourRGBA back) {
	if (pRenderTarget) {
		FillRectangleAligned(rc, back);
		D2DPenColourAlpha(fore);
		DrawTextCommon(rc, font_, ybase, text, CpUtf8, ETO_OPAQUE | ETO_CLIPPED);
	}
}

void SurfaceD2D::DrawTextTransparentUTF8(PRectangle rc, const Font *font_, XYPOSITION ybase, std::string_view text,
	ColourRGBA fore) {
	// Avoid drawing spaces in transparent mode
	for (const char ch : text) {
		if (ch != ' ') {
			if (pRenderTarget) {
				D2DPenColourAlpha(fore);
				DrawTextCommon(rc, font_, ybase, text, CpUtf8, 0);
			}
			return;
		}
	}
}

void SurfaceD2D::MeasureWidthsUTF8(const Font *font_, std::string_view text, XYPOSITION *positions) {
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	const TextWide tbuf(text, CpUtf8);
	TextPositions poses(tbuf.tlen);
	if (FAILED(MeasurePositions(poses, tbuf, pfm->pTextFormat.Get()))) {
		return;
	}
	// Map the widths given for UTF-16 characters back onto the UTF-8 input string
	size_t i = 0;
	for (int ui = 0; ui < tbuf.tlen; ui++) {
		const unsigned char uch = text[i];
		const unsigned int byteCount = UTF8BytesOfLead[uch];
		if (byteCount == 4) {	// Non-BMP
			ui++;
			PLATFORM_ASSERT(ui < tbuf.tlen);
		}
		for (unsigned int bytePos=0; (bytePos<byteCount) && (i<text.length()) && (ui < tbuf.tlen); bytePos++) {
			positions[i++] = poses.buffer[ui];
		}
	}
	const XYPOSITION lastPos = (i > 0) ? positions[i - 1] : 0.0;
	while (i < text.length()) {
		positions[i++] = lastPos;
	}
}

XYPOSITION SurfaceD2D::WidthTextUTF8(const Font * font_, std::string_view text) {
	FLOAT width = 1.0;
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	if (pfm->pTextFormat) {
		const TextWide tbuf(text, CpUtf8);
		// Create a layout
		if (TextLayout pTextLayout = LayoutCreate(tbuf.AsView(), pfm->pTextFormat.Get())) {
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(pTextLayout->GetMetrics(&textMetrics)))
				width = textMetrics.widthIncludingTrailingWhitespace;
		}
	}
	return width;
}

XYPOSITION SurfaceD2D::Ascent(const Font *font_) {
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	return std::ceil(pfm->yAscent);
}

XYPOSITION SurfaceD2D::Descent(const Font *font_) {
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	return std::ceil(pfm->yDescent);
}

XYPOSITION SurfaceD2D::InternalLeading(const Font *font_) {
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	return std::floor(pfm->yInternalLeading);
}

XYPOSITION SurfaceD2D::Height(const Font *font_) {
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	return std::ceil(pfm->yAscent) + std::ceil(pfm->yDescent);
}

XYPOSITION SurfaceD2D::AverageCharWidth(const Font *font_) {
	FLOAT width = 1.0;
	const FontDirectWrite *pfm = FontDirectWrite::Cast(font_);
	if (pfm->pTextFormat) {
		// Create a layout
		static constexpr std::wstring_view wsvAllAlpha = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		if (TextLayout pTextLayout = LayoutCreate(wsvAllAlpha, pfm->pTextFormat.Get())) {
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(pTextLayout->GetMetrics(&textMetrics)))
				width = textMetrics.width / wsvAllAlpha.length();
		}
	}
	return width;
}

void SurfaceD2D::SetClip(PRectangle rc) {
	if (pRenderTarget) {
		const D2D1_RECT_F rcClip = RectangleFromPRectangle(rc);
		pRenderTarget->PushAxisAlignedClip(rcClip, D2D1_ANTIALIAS_MODE_ALIASED);
		clipsActive++;
	}
}

void SurfaceD2D::PopClip() {
	if (pRenderTarget) {
		PLATFORM_ASSERT(clipsActive > 0);
		pRenderTarget->PopAxisAlignedClip();
		clipsActive--;
	}
}

void SurfaceD2D::FlushCachedState() {
}

void SurfaceD2D::FlushDrawing() {
	if (pRenderTarget) {
		pRenderTarget->Flush();
	}
}

void SurfaceD2D::SetRenderingParams(std::shared_ptr<RenderingParams> renderingParams_) {
	renderingParams = std::move(renderingParams_);
}

#endif

std::unique_ptr<Surface> Surface::Allocate([[maybe_unused]] Technology technology) {
#if defined(USE_D2D)
	if (technology != Technology::Default) {
		return std::make_unique<SurfaceD2D>();
	}
#endif
	return std::make_unique<SurfaceGDI>();
}

Window::~Window() noexcept = default;

void Window::Destroy() noexcept {
	if (wid)
		::DestroyWindow(HwndFromWindowID(wid));
	wid = nullptr;
}

PRectangle Window::GetPosition() const {
	RECT rc;
	::GetWindowRect(HwndFromWindowID(wid), &rc);
	return PRectangle::FromInts(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::SetPosition(PRectangle rc) {
	::SetWindowPos(HwndFromWindowID(wid),
		{}, static_cast<int>(rc.left), static_cast<int>(rc.top),
		static_cast<int>(rc.Width()), static_cast<int>(rc.Height()), SWP_NOZORDER | SWP_NOACTIVATE);
}

namespace {

RECT RectFromMonitor(HMONITOR hMonitor) noexcept {
	MONITORINFO mi = {};
	mi.cbSize = sizeof(mi);
	if (GetMonitorInfo(hMonitor, &mi)) {
		return mi.rcWork;
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

}

void Window::SetPositionRelative(PRectangle rc, const Window *relativeTo) {
	const DWORD style = GetWindowStyle(HwndFromWindowID(wid));
	if (style & WS_POPUP) {
		POINT ptOther = {0, 0};
		::ClientToScreen(HwndFromWindow(*relativeTo), &ptOther);
		rc.Move(static_cast<XYPOSITION>(ptOther.x), static_cast<XYPOSITION>(ptOther.y));

		const RECT rcMonitor = RectFromPRectangle(rc);

		HMONITOR hMonitor = MonitorFromRect(&rcMonitor, MONITOR_DEFAULTTONEAREST);
		// If hMonitor is NULL, that's just the main screen anyways.
		const RECT rcWork = RectFromMonitor(hMonitor);

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

PRectangle Window::GetClientPosition() const {
	RECT rc={0,0,0,0};
	if (wid)
		::GetClientRect(HwndFromWindowID(wid), &rc);
	return PRectangle::FromInts(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::Show(bool show) {
	if (show)
		::ShowWindow(HwndFromWindowID(wid), SW_SHOWNOACTIVATE);
	else
		::ShowWindow(HwndFromWindowID(wid), SW_HIDE);
}

void Window::InvalidateAll() {
	::InvalidateRect(HwndFromWindowID(wid), nullptr, FALSE);
}

void Window::InvalidateRectangle(PRectangle rc) {
	const RECT rcw = RectFromPRectangle(rc);
	::InvalidateRect(HwndFromWindowID(wid), &rcw, FALSE);
}

namespace {

std::optional<DWORD> RegGetDWORD(HKEY hKey, LPCWSTR valueName) noexcept {
	DWORD value = 0;
	DWORD type = REG_NONE;
	DWORD size = sizeof(DWORD);
	const LSTATUS status = ::RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size);
	if (status == ERROR_SUCCESS && type == REG_DWORD) {
		return value;
	}
	return {};
}

class CursorHelper {
	HDC hMemDC {};
	HBITMAP hBitmap {};
	HBITMAP hOldBitmap {};
	DWORD *pixels = nullptr;
	const int width;
	const int height;

	static constexpr float arrow[][2] = {
		{ 32.0f - 12.73606f,32.0f - 19.04075f },
		{ 32.0f - 7.80159f, 32.0f - 19.04075f },
		{ 32.0f - 9.82813f, 32.0f - 14.91828f },
		{ 32.0f - 6.88341f, 32.0f - 13.42515f },
		{ 32.0f - 4.62301f, 32.0f - 18.05872f },
		{ 32.0f - 1.26394f, 32.0f - 14.78295f },
		{ 32.0f - 1.26394f, 32.0f - 30.57485f },
	};

public:
	~CursorHelper() {
		if (hOldBitmap) {
			SelectBitmap(hMemDC, hOldBitmap);
		}
		if (hBitmap) {
			::DeleteObject(hBitmap);
		}
		if (hMemDC) {
			::DeleteDC(hMemDC);
		}
	}

	CursorHelper(int width_, int height_) noexcept : width{width_}, height{height_} {
		hMemDC = ::CreateCompatibleDC({});
		if (!hMemDC) {
			return;
		}

		// https://learn.microsoft.com/en-us/windows/win32/menurc/using-cursors#creating-a-cursor
		BITMAPV5HEADER bi {};
		bi.bV5Size = sizeof(BITMAPV5HEADER);
		bi.bV5Width = width;
		bi.bV5Height = height;
		bi.bV5Planes = 1;
		bi.bV5BitCount = 32;
		bi.bV5Compression = BI_BITFIELDS;
		// The following mask specification specifies a supported 32 BPP alpha format for Windows XP.
		bi.bV5RedMask   = 0x00FF0000U;
		bi.bV5GreenMask = 0x0000FF00U;
		bi.bV5BlueMask  = 0x000000FFU;
		bi.bV5AlphaMask = 0xFF000000U;

		// Create the DIB section with an alpha channel.
		hBitmap = CreateDIBSection(hMemDC, reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS, reinterpret_cast<void **>(&pixels), nullptr, 0);
		if (hBitmap) {
			hOldBitmap = SelectBitmap(hMemDC, hBitmap);
		}
	}

	[[nodiscard]] bool HasBitmap() const noexcept {
		return hOldBitmap != nullptr;
	}

	HCURSOR Create() noexcept {
		HCURSOR cursor {};
		// Create an empty mask bitmap.
		HBITMAP hMonoBitmap = ::CreateBitmap(width, height, 1, 1, nullptr);
		if (hMonoBitmap) {
			// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createiconindirect
			// hBitmap should not already be selected into a device context
			SelectBitmap(hMemDC, hOldBitmap);
			hOldBitmap = {};
			ICONINFO info = {false, static_cast<DWORD>(width - 1), 0, hMonoBitmap, hBitmap};
			cursor = ::CreateIconIndirect(&info);
			::DeleteObject(hMonoBitmap);
		}
		return cursor;
	}

#if defined(USE_D2D)

	bool DrawD2D(COLORREF fillColour, COLORREF strokeColour) noexcept {
		if (!LoadD2D()) {
			return false;
		}

		D2D1_RENDER_TARGET_PROPERTIES drtp {};
		drtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
		drtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;
		drtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
		drtp.dpiX = 96.f;
		drtp.dpiY = 96.f;
		drtp.pixelFormat = D2D1::PixelFormat(
			DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE_PREMULTIPLIED
		);

		DCRenderTarget pTarget;
		HRESULT hr = CreateDCRenderTarget(&drtp, pTarget);
		if (FAILED(hr) || !pTarget) {
			return false;
		}

		const RECT rc = {0, 0, width, height};
		hr = pTarget->BindDC(hMemDC, &rc);
		if (FAILED(hr)) {
			return false;
		}

		pTarget->BeginDraw();

		// Draw something on the bitmap section.
		constexpr size_t nPoints = std::size(arrow);
		D2D1_POINT_2F points[nPoints]{};
		const FLOAT scale = width/32.0f;
		for (size_t i = 0; i < nPoints; i++) {
			points[i].x = arrow[i][0] * scale;
			points[i].y = arrow[i][1] * scale;
		}

		const Geometry geometry = GeometryCreate();
		if (!geometry) {
			return false;
		}

		const GeometrySink sink = GeometrySinkCreate(geometry.Get());
		if (!sink) {
			return false;
		}

		sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
		for (size_t i = 1; i < nPoints; i++) {
			sink->AddLine(points[i]);
		}
		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		hr = sink->Close();
		if (FAILED(hr)) {
			return false;
		}

		if (const BrushSolid pBrushFill = BrushSolidCreate(pTarget.Get(), fillColour)) {
			pTarget->FillGeometry(geometry.Get(), pBrushFill.Get());
		}

		if (const BrushSolid pBrushStroke = BrushSolidCreate(pTarget.Get(), strokeColour)) {
			pTarget->DrawGeometry(geometry.Get(), pBrushStroke.Get(), scale);
		}

		hr = pTarget->EndDraw();
		return SUCCEEDED(hr);
	}
#endif

	void Draw(COLORREF fillColour, COLORREF strokeColour) noexcept {
#if defined(USE_D2D)
		if (DrawD2D(fillColour, strokeColour)) {
			return;
		}
#endif

		// Draw something on the DIB section.
		constexpr size_t nPoints = std::size(arrow);
		POINT points[nPoints]{};
		const float scale = width/32.0f;
		for (size_t i = 0; i < nPoints; i++) {
			points[i].x = std::lround(arrow[i][0] * scale);
			points[i].y = std::lround(arrow[i][1] * scale);
		}

		const DWORD penWidth = std::lround(scale);
		HPEN pen{};
		if (penWidth > 1) {
			const LOGBRUSH brushParameters { BS_SOLID, strokeColour, 0 };
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_ROUND | PS_JOIN_MITER,
				penWidth,
				&brushParameters,
				0,
				nullptr);
		} else {
			pen = ::CreatePen(PS_INSIDEFRAME, 1, strokeColour);
		}

		HPEN penOld = SelectPen(hMemDC, pen);
		HBRUSH brush = ::CreateSolidBrush(fillColour);
		HBRUSH brushOld = SelectBrush(hMemDC, brush);
		::Polygon(hMemDC, points, static_cast<int>(nPoints));
		SelectPen(hMemDC, penOld);
		SelectBrush(hMemDC, brushOld);
		::DeleteObject(pen);
		::DeleteObject(brush);

		// Set the alpha values for each pixel in the cursor.
		for (int i = 0; i < width*height; i++) {
			if (*pixels != 0) {
				*pixels |= 0xFF000000U;
			}
			pixels++;
		}
	}
};

void ChooseCursor(LPCTSTR cursor) noexcept {
	::SetCursor(::LoadCursor({}, cursor));
}

void ChooseCursor(Window::Cursor curs) noexcept {
	switch (curs) {
	case Window::Cursor::text:
		ChooseCursor(IDC_IBEAM);
		break;
	case Window::Cursor::up:
		ChooseCursor(IDC_UPARROW);
		break;
	case Window::Cursor::wait:
		ChooseCursor(IDC_WAIT);
		break;
	case Window::Cursor::horizontal:
		ChooseCursor(IDC_SIZEWE);
		break;
	case Window::Cursor::vertical:
		ChooseCursor(IDC_SIZENS);
		break;
	case Window::Cursor::hand:
		ChooseCursor(IDC_HAND);
		break;
	case Window::Cursor::reverseArrow:
	case Window::Cursor::arrow:
	case Window::Cursor::invalid:	// Should not occur, but just in case.
	default:
		ChooseCursor(IDC_ARROW);
		break;
	}
}

}

HCURSOR LoadReverseArrowCursor(UINT dpi) noexcept {
	// https://learn.microsoft.com/en-us/answers/questions/815036/windows-cursor-size
	constexpr DWORD defaultCursorBaseSize = 32;
	constexpr DWORD maxCursorBaseSize = 16*(1 + 15); // 16*(1 + CursorSize)
	DWORD cursorBaseSize = 0;
	HKEY hKey {};
	LSTATUS status = ::RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Cursors", 0, KEY_QUERY_VALUE, &hKey);
	if (status == ERROR_SUCCESS) {
		if (std::optional<DWORD> baseSize = RegGetDWORD(hKey, L"CursorBaseSize")) {
			// CursorBaseSize is multiple of 16
			cursorBaseSize = std::min(*baseSize & ~15, maxCursorBaseSize);
		}
		::RegCloseKey(hKey);
	}

	int width = 0;
	int height = 0;
	if (cursorBaseSize > defaultCursorBaseSize) {
		width = ::MulDiv(cursorBaseSize, dpi, USER_DEFAULT_SCREEN_DPI);
		height = width;
	} else {
		width = SystemMetricsForDpi(SM_CXCURSOR, dpi);
		height = SystemMetricsForDpi(SM_CYCURSOR, dpi);
		PLATFORM_ASSERT(width == height);
	}

	CursorHelper cursorHelper(width, height);
	if (!cursorHelper.HasBitmap()) {
		return {};
	}

	COLORREF fillColour = RGB(0xff, 0xff, 0xfe);
	COLORREF strokeColour = RGB(0, 0, 1);
	status = ::RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Accessibility", 0, KEY_QUERY_VALUE, &hKey);
	if (status == ERROR_SUCCESS) {
		if (std::optional<DWORD> cursorType = RegGetDWORD(hKey, L"CursorType")) {
			switch (*cursorType) {
			case 1: // black
			case 4: // black
				std::swap(fillColour, strokeColour);
				break;
			case 6: // custom
				if (std::optional<DWORD> cursorColor = RegGetDWORD(hKey, L"CursorColor")) {
					fillColour = *cursorColor;
				}
				break;
			default: // 0, 3 white, 2, 5 invert
				break;
			}
		}
		::RegCloseKey(hKey);
	}

	cursorHelper.Draw(fillColour, strokeColour);
	HCURSOR cursor = cursorHelper.Create();
	return cursor;
}

void Window::SetCursor(Cursor curs) {
	ChooseCursor(curs);
}

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   coordinates */
PRectangle Window::GetMonitorRect(Point pt) {
	const PRectangle rcPosition = GetPosition();
	const POINT ptDesktop = {static_cast<LONG>(pt.x + rcPosition.left),
		static_cast<LONG>(pt.y + rcPosition.top)};
	HMONITOR hMonitor = MonitorFromPoint(ptDesktop, MONITOR_DEFAULTTONEAREST);

	const RECT rcWork = RectFromMonitor(hMonitor);
	if (rcWork.left < rcWork.right) {
		PRectangle rcMonitor(
			rcWork.left - rcPosition.left,
			rcWork.top - rcPosition.top,
			rcWork.right - rcPosition.left,
			rcWork.bottom - rcPosition.top);
		return rcMonitor;
	}
	return PRectangle();
}

struct ListItemData {
	const char *text;
	int pixId;
};

class LineToItem {
	std::vector<char> words;

	std::vector<ListItemData> data;

public:
	void Clear() noexcept {
		words.clear();
		data.clear();
	}

	[[nodiscard]] ListItemData Get(size_t index) const noexcept {
		if (index < data.size()) {
			return data[index];
		}
		ListItemData missing = {"", -1};
		return missing;
	}
	[[nodiscard]] int Count() const noexcept {
		return static_cast<int>(data.size());
	}

	void AllocItem(const char *text, int pixId) {
		const ListItemData lid = { text, pixId };
		data.push_back(lid);
	}

	char *SetWords(const char *s) {
		words = std::vector<char>(s, s+strlen(s)+1);
		return &words[0];
	}
};

const TCHAR ListBoxX_ClassName[] = TEXT("ListBoxX");

ListBox::ListBox() noexcept = default;

ListBox::~ListBox() noexcept = default;

class ListBoxX : public ListBox {
	int lineHeight = 10;
	HFONT fontCopy {};
	Technology technology = Technology::Default;
	RGBAImageSet images;
	LineToItem lti;
	HWND lb {};
	bool unicodeMode = false;
	int desiredVisibleRows = 9;
	unsigned int maxItemCharacters = 0;
	unsigned int aveCharWidth = 8;
	Window *parent = nullptr;
	int ctrlID = 0;
	UINT dpi = USER_DEFAULT_SCREEN_DPI;
	IListBoxDelegate *delegate = nullptr;
	const char *widestItem = nullptr;
	unsigned int maxCharWidth = 1;
	WPARAM resizeHit = 0;
	PRectangle rcPreSize;
	Point dragOffset;
	Point location;	// Caret location at which the list is opened
	MouseWheelDelta wheelDelta;
	ListOptions options;
	DWORD frameStyle = WS_THICKFRAME;

	HWND GetHWND() const noexcept;
	void AppendListItem(const char *text, const char *numword);
	void AdjustWindowRect(PRectangle *rc, UINT dpiAdjust) const noexcept;
	int ItemHeight() const noexcept;
	int MinClientWidth() const noexcept;
	int TextOffset() const noexcept;
	POINT GetClientExtent() const noexcept;
	POINT MinTrackSize() const noexcept;
	POINT MaxTrackSize() const noexcept;
	void SetRedraw(bool on) noexcept;
	void OnDoubleClick();
	void OnSelChange();
	void ResizeToCursor();
	void StartResize(WPARAM);
	LRESULT NcHitTest(WPARAM, LPARAM) const;
	void CentreItem(int n);
	void Paint(HDC);
	static LRESULT PASCAL ControlWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	static constexpr POINT ItemInset {0, 0};	// Padding around whole item
	static constexpr POINT TextInset {2, 0};	// Padding around text
	static constexpr POINT ImageInset {1, 0};	// Padding around image

public:
	ListBoxX() = default;
	ListBoxX(const ListBoxX &) = delete;
	ListBoxX(ListBoxX &&) = delete;
	ListBoxX &operator=(const ListBoxX &) = delete;
	ListBoxX &operator=(ListBoxX &&) = delete;
	~ListBoxX() noexcept override {
		if (fontCopy) {
			::DeleteObject(fontCopy);
			fontCopy = {};
		}
	}
	void SetFont(const Font *font) override;
	void Create(Window &parent_, int ctrlID_, Point location_, int lineHeight_, bool unicodeMode_, Technology technology_) override;
	void SetAverageCharWidth(int width) override;
	void SetVisibleRows(int rows) override;
	int GetVisibleRows() const override;
	PRectangle GetDesiredRect() override;
	int CaretFromEdge() override;
	void Clear() noexcept override;
	void Append(char *s, int type) override;
	int Length() override;
	void Select(int n) override;
	int GetSelection() override;
	int Find(const char *prefix) override;
	std::string GetValue(int n) override;
	void RegisterImage(int type, const char *xpm_data) override;
	void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) override;
	void ClearRegisteredImages() override;
	void SetDelegate(IListBoxDelegate *lbDelegate) override;
	void SetList(const char *list, char separator, char typesep) override;
	void SetOptions(ListOptions options_) override;
	void Draw(DRAWITEMSTRUCT *pDrawItem);
	LRESULT WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	static LRESULT PASCAL StaticWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
};

std::unique_ptr<ListBox> ListBox::Allocate() {
	return std::make_unique<ListBoxX>();
}

void ListBoxX::Create(Window &parent_, int ctrlID_, Point location_, int lineHeight_, bool unicodeMode_, Technology technology_) {
	parent = &parent_;
	ctrlID = ctrlID_;
	location = location_;
	lineHeight = lineHeight_;
	unicodeMode = unicodeMode_;
	technology = technology_;
	HWND hwndParent = HwndFromWindow(*parent);
	HINSTANCE hinstanceParent = GetWindowInstance(hwndParent);
	// Window created as popup so not clipped within parent client area
	wid = ::CreateWindowEx(
		WS_EX_WINDOWEDGE, ListBoxX_ClassName, TEXT(""),
		WS_POPUP | frameStyle,
		100,100, 150,80, hwndParent,
		{},
		hinstanceParent,
		this);

	dpi = DpiForWindow(hwndParent);
	POINT locationw = POINTFromPoint(location);
	::MapWindowPoints(hwndParent, {}, &locationw, 1);
	location = PointFromPOINT(locationw);
}

void ListBoxX::SetFont(const Font *font) {
	const FontWin *pfm = dynamic_cast<const FontWin *>(font);
	if (pfm) {
		if (fontCopy) {
			::DeleteObject(fontCopy);
			fontCopy = {};
		}
		fontCopy = pfm->HFont();
		SetWindowFont(lb, fontCopy, 0);
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

HWND ListBoxX::GetHWND() const noexcept {
	return HwndFromWindowID(GetID());
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
	int len = 0;
	if (widestItem) {
		len = static_cast<int>(strlen(widestItem));
		if (unicodeMode) {
			const TextWide tbuf(widestItem, CpUtf8);
			::GetTextExtentPoint32W(hdc, tbuf.buffer, tbuf.tlen, &textSize);
		} else {
			::GetTextExtentPoint32A(hdc, widestItem, len, &textSize);
		}
	}
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	maxCharWidth = tm.tmMaxCharWidth;
	SelectFont(hdc, oldFont);
	::ReleaseDC(lb, hdc);

	const int widthDesired = std::max(textSize.cx, (len + 1) * tm.tmAveCharWidth);
	if (width < widthDesired)
		width = widthDesired;

	rcDesired.right = rcDesired.left + TextOffset() + width + (TextInset.x * 2);
	if (Length() > rows)
		rcDesired.right += SystemMetricsForDpi(SM_CXVSCROLL, dpi);

	AdjustWindowRect(&rcDesired, dpi);
	return rcDesired;
}

int ListBoxX::TextOffset() const noexcept {
	const int pixWidth = images.GetWidth();
	return pixWidth == 0 ? ItemInset.x : ItemInset.x + pixWidth + (ImageInset.x * 2);
}

int ListBoxX::CaretFromEdge() {
	PRectangle rc;
	AdjustWindowRect(&rc, dpi);
	return TextOffset() + static_cast<int>(TextInset.x + (0 - rc.left) - 1);
}

void ListBoxX::Clear() noexcept {
	ListBox_ResetContent(lb);
	maxItemCharacters = 0;
	widestItem = nullptr;
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
	ListBox_SetCurSel(lb, n);
	OnSelChange();
	SetRedraw(true);
}

int ListBoxX::GetSelection() {
	return ListBox_GetCurSel(lb);
}

// This is not actually called at present
int ListBoxX::Find(const char *) {
	return LB_ERR;
}

std::string ListBoxX::GetValue(int n) {
	const ListItemData item = lti.Get(n);
	return item.text;
}

void ListBoxX::RegisterImage(int type, const char *xpm_data) {
	XPM xpmImage(xpm_data);
	images.AddImage(type, std::make_unique<RGBAImage>(xpmImage));
}

void ListBoxX::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage) {
	images.AddImage(type, std::make_unique<RGBAImage>(width, height, 1.0f, pixelsImage));
}

void ListBoxX::ClearRegisteredImages() {
	images.Clear();
}

namespace {

int ColourOfElement(std::optional<ColourRGBA> colour, int nIndex) {
	if (colour.has_value()) {
		return colour.value().OpaqueRGB();
	}
	return ::GetSysColor(nIndex);
}

void FillRectColour(HDC hdc, const RECT *lprc, int colour) noexcept {
	const HBRUSH brush = ::CreateSolidBrush(colour);
	::FillRect(hdc, lprc, brush);
	::DeleteObject(brush);
}

}

void ListBoxX::Draw(DRAWITEMSTRUCT *pDrawItem) {
	if ((pDrawItem->itemAction == ODA_SELECT) || (pDrawItem->itemAction == ODA_DRAWENTIRE)) {
		RECT rcBox = pDrawItem->rcItem;
		rcBox.left += TextOffset();
		if (pDrawItem->itemState & ODS_SELECTED) {
			RECT rcImage = pDrawItem->rcItem;
			rcImage.right = rcBox.left;
			// The image is not highlighted
			FillRectColour(pDrawItem->hDC, &rcImage, ColourOfElement(options.back, COLOR_WINDOW));
			FillRectColour(pDrawItem->hDC, &rcBox, ColourOfElement(options.backSelected, COLOR_HIGHLIGHT));
			::SetBkColor(pDrawItem->hDC, ColourOfElement(options.backSelected, COLOR_HIGHLIGHT));
			::SetTextColor(pDrawItem->hDC, ColourOfElement(options.foreSelected, COLOR_HIGHLIGHTTEXT));
		} else {
			FillRectColour(pDrawItem->hDC, &pDrawItem->rcItem, ColourOfElement(options.back, COLOR_WINDOW));
			::SetBkColor(pDrawItem->hDC, ColourOfElement(options.back, COLOR_WINDOW));
			::SetTextColor(pDrawItem->hDC, ColourOfElement(options.fore, COLOR_WINDOWTEXT));
		}

		const ListItemData item = lti.Get(pDrawItem->itemID);
		const int pixId = item.pixId;
		const char *text = item.text;
		const int len = static_cast<int>(strlen(text));

		RECT rcText = rcBox;
		::InsetRect(&rcText, TextInset.x, TextInset.y);

		if (unicodeMode) {
			const TextWide tbuf(text, CpUtf8);
			::DrawTextW(pDrawItem->hDC, tbuf.buffer, tbuf.tlen, &rcText, DT_NOPREFIX|DT_END_ELLIPSIS|DT_SINGLELINE|DT_NOCLIP);
		} else {
			::DrawTextA(pDrawItem->hDC, text, len, &rcText, DT_NOPREFIX|DT_END_ELLIPSIS|DT_SINGLELINE|DT_NOCLIP);
		}

		// Draw the image, if any
		const RGBAImage *pimage = images.Get(pixId);
		if (pimage) {
			std::unique_ptr<Surface> surfaceItem(Surface::Allocate(technology));
			if (technology == Technology::Default) {
				surfaceItem->Init(pDrawItem->hDC, pDrawItem->hwndItem);
				const long left = pDrawItem->rcItem.left + ItemInset.x + ImageInset.x;
				const PRectangle rcImage = PRectangle::FromInts(left, pDrawItem->rcItem.top,
					left + images.GetWidth(), pDrawItem->rcItem.bottom);
				surfaceItem->DrawRGBAImage(rcImage,
					pimage->GetWidth(), pimage->GetHeight(), pimage->Pixels());
				::SetTextAlign(pDrawItem->hDC, TA_TOP);
			} else {
#if defined(USE_D2D)
				const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
					D2D1_RENDER_TARGET_TYPE_DEFAULT,
					D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM,
						D2D1_ALPHA_MODE_IGNORE),
					0,
					0,
					D2D1_RENDER_TARGET_USAGE_NONE,
					D2D1_FEATURE_LEVEL_DEFAULT
					);
				DCRenderTarget pDCRT;
				HRESULT hr = CreateDCRenderTarget(&props, pDCRT);
				if (SUCCEEDED(hr) && pDCRT) {
					const long left = pDrawItem->rcItem.left + ItemInset.x + ImageInset.x;

					RECT rcItem = pDrawItem->rcItem;
					rcItem.left = left;
					rcItem.right = rcItem.left + images.GetWidth();

					hr = pDCRT->BindDC(pDrawItem->hDC, &rcItem);
					if (SUCCEEDED(hr)) {
						surfaceItem->Init(pDCRT.Get(), pDrawItem->hwndItem);
						pDCRT->BeginDraw();
						const PRectangle rcImage = PRectangle::FromInts(0, 0, images.GetWidth(), rcItem.bottom - rcItem.top);
						surfaceItem->DrawRGBAImage(rcImage,
							pimage->GetWidth(), pimage->GetHeight(), pimage->Pixels());
						pDCRT->EndDraw();
					}
				}
#endif
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
	const unsigned int len = static_cast<unsigned int>(strlen(text));
	if (maxItemCharacters < len) {
		maxItemCharacters = len;
		widestItem = text;
	}
}

void ListBoxX::SetDelegate(IListBoxDelegate *lbDelegate) {
	delegate = lbDelegate;
}

void ListBoxX::SetList(const char *list, char separator, char typesep) {
	// Turn off redraw while populating the list - this has a significant effect, even if
	// the listbox is not visible.
	SetRedraw(false);
	Clear();
	const size_t size = strlen(list);
	char *words = lti.SetWords(list);
	const char *startword = words;
	char *numword = nullptr;
	for (size_t i=0; i < size; i++) {
		if (words[i] == separator) {
			words[i] = '\0';
			if (numword)
				*numword = '\0';
			AppendListItem(startword, numword);
			startword = words + i + 1;
			numword = nullptr;
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
	const int count = lti.Count();
	::SendMessage(lb, LB_INITSTORAGE, count, 0);
	for (intptr_t j=0; j<count; j++) {
		ListBox_AddItemData(lb, j+1);
	}
	SetRedraw(true);
}

void ListBoxX::SetOptions(ListOptions options_) {
	options = options_;
	frameStyle = FlagSet(options.options, AutoCompleteOption::FixedSize) ? WS_BORDER : WS_THICKFRAME;
}

void ListBoxX::AdjustWindowRect(PRectangle *rc, UINT dpiAdjust) const noexcept {
	RECT rcw = RectFromPRectangle(*rc);
	if (fnAdjustWindowRectExForDpi) {
		fnAdjustWindowRectExForDpi(&rcw, frameStyle, false, WS_EX_WINDOWEDGE, dpiAdjust);
	} else {
		::AdjustWindowRectEx(&rcw, frameStyle, false, WS_EX_WINDOWEDGE);
	}
	*rc = PRectangle::FromInts(rcw.left, rcw.top, rcw.right, rcw.bottom);
}

int ListBoxX::ItemHeight() const noexcept {
	int itemHeight = lineHeight + (TextInset.y * 2);
	const int pixHeight = images.GetHeight() + (ImageInset.y * 2);
	if (itemHeight < pixHeight) {
		itemHeight = pixHeight;
	}
	return itemHeight;
}

int ListBoxX::MinClientWidth() const noexcept {
	return 12 * (aveCharWidth+aveCharWidth/3);
}

POINT ListBoxX::MinTrackSize() const noexcept {
	PRectangle rc = PRectangle::FromInts(0, 0, MinClientWidth(), ItemHeight());
	AdjustWindowRect(&rc, dpi);
	POINT ret = {static_cast<LONG>(rc.Width()), static_cast<LONG>(rc.Height())};
	return ret;
}

POINT ListBoxX::MaxTrackSize() const noexcept {
	PRectangle rc = PRectangle::FromInts(0, 0,
		std::max<int>(static_cast<unsigned int>(MinClientWidth()),
		maxCharWidth * maxItemCharacters + TextInset.x * 2 +
		 TextOffset() + SystemMetricsForDpi(SM_CXVSCROLL, dpi)),
		ItemHeight() * lti.Count());
	AdjustWindowRect(&rc, dpi);
	POINT ret = {static_cast<LONG>(rc.Width()), static_cast<LONG>(rc.Height())};
	return ret;
}

void ListBoxX::SetRedraw(bool on) noexcept {
	::SendMessage(lb, WM_SETREDRAW, on, 0);
	if (on)
		::InvalidateRect(lb, nullptr, TRUE);
}

void ListBoxX::ResizeToCursor() {
	PRectangle rc = GetPosition();
	POINT ptw;
	::GetCursorPos(&ptw);
	const Point pt = PointFromPOINT(ptw) + dragOffset;

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
		default:
			break;
	}

	const POINT ptMin = MinTrackSize();
	const POINT ptMax = MaxTrackSize();
	// We don't allow the left edge to move at present, but just in case
	rc.left = std::clamp(rc.left, rcPreSize.right - ptMax.x, rcPreSize.right - ptMin.x);
	rc.top = std::clamp(rc.top, rcPreSize.bottom - ptMax.y, rcPreSize.bottom - ptMin.y);
	rc.right = std::clamp(rc.right, rcPreSize.left + ptMin.x, rcPreSize.left + ptMax.x);
	rc.bottom = std::clamp(rc.bottom, rcPreSize.top + ptMin.y, rcPreSize.top + ptMax.y);

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
		// as we don't want the left edge to be movable
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

LRESULT ListBoxX::NcHitTest(WPARAM wParam, LPARAM lParam) const {
	const PRectangle rc = GetPosition();

	LRESULT hit = ::DefWindowProc(GetHWND(), WM_NCHITTEST, wParam, lParam);
	// There is an apparent bug in the DefWindowProc hit test code whereby it will
	// return HTTOPXXX if the window in question is shorter than the default
	// window caption height + frame, even if one is hovering over the bottom edge of
	// the frame, so workaround that here
	if (hit >= HTTOP && hit <= HTTOPRIGHT) {
		const int minHeight = SystemMetricsForDpi(SM_CYMINTRACK, dpi);
		const int yPos = GET_Y_LPARAM(lParam);
		if ((rc.Height() < minHeight) && (yPos > ((rc.top + rc.bottom)/2))) {
			hit += HTBOTTOM - HTTOP;
		}
	}

	// Never permit resizing that moves the left edge. Allow movement of top or bottom edge
	// depending on whether the list is above or below the caret
	switch (hit) {
		case HTLEFT:
		case HTTOPLEFT:
		case HTBOTTOMLEFT:
			hit = HTERROR;
			break;

		case HTTOP:
		case HTTOPRIGHT: {
				// Valid only if caret below list
				if (location.y < rc.top)
					hit = HTERROR;
			}
			break;

		case HTBOTTOM:
		case HTBOTTOMRIGHT: {
				// Valid only if caret above list
				if (rc.bottom <= location.y)
					hit = HTERROR;
			}
			break;
		default:
			break;
	}

	return hit;
}

void ListBoxX::OnDoubleClick() {
	if (delegate) {
		ListBoxEvent event(ListBoxEvent::EventType::doubleClick);
		delegate->ListNotify(&event);
	}
}

void ListBoxX::OnSelChange() {
	if (delegate) {
		ListBoxEvent event(ListBoxEvent::EventType::selectionChange);
		delegate->ListNotify(&event);
	}
}

POINT ListBoxX::GetClientExtent() const noexcept {
	RECT rc;
	::GetWindowRect(HwndFromWindowID(wid), &rc);
	POINT ret { rc.right - rc.left, rc.bottom - rc.top };
	return ret;
}

void ListBoxX::CentreItem(int n) {
	// If below mid point, scroll up to centre, but with more items below if uneven
	if (n >= 0) {
		const POINT extent = GetClientExtent();
		const int visible = extent.y/ItemHeight();
		if (visible < Length()) {
			const int top = ListBox_GetTopIndex(lb);
			const int half = (visible - 1) / 2;
			if (n > (top + half))
				ListBox_SetTopIndex(lb, n - half);
		}
	}
}

// Performs a double-buffered paint operation to avoid flicker
void ListBoxX::Paint(HDC hDC) {
	const POINT extent = GetClientExtent();
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, extent.x, extent.y);
	HDC bitmapDC = ::CreateCompatibleDC(hDC);
	HBITMAP hBitmapOld = SelectBitmap(bitmapDC, hBitmap);
	// The list background is mainly erased during painting, but can be a small
	// unpainted area when at the end of a non-integrally sized list with a
	// vertical scroll bar
	const RECT rc = { 0, 0, extent.x, extent.y };
	FillRectColour(bitmapDC, &rc, ColourOfElement(options.back, COLOR_WINDOWTEXT));
	// Paint the entire client area and vertical scrollbar
	::SendMessage(lb, WM_PRINT, reinterpret_cast<WPARAM>(bitmapDC), PRF_CLIENT|PRF_NONCLIENT);
	::BitBlt(hDC, 0, 0, extent.x, extent.y, bitmapDC, 0, 0, SRCCOPY);
	// Select a stock brush to prevent warnings from BoundsChecker
	SelectBrush(bitmapDC, GetStockBrush(WHITE_BRUSH));
	SelectBitmap(bitmapDC, hBitmapOld);
	::DeleteDC(bitmapDC);
	::DeleteObject(hBitmap);
}

LRESULT PASCAL ListBoxX::ControlWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	try {
		ListBoxX *lbx = static_cast<ListBoxX *>(PointerFromWindow(::GetParent(hWnd)));
		switch (iMessage) {
		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT: {
				PAINTSTRUCT ps;
				HDC hDC = ::BeginPaint(hWnd, &ps);
				if (lbx) {
					lbx->Paint(hDC);
				}
				::EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_MOUSEACTIVATE:
			// This prevents the view activating when the scrollbar is clicked
			return MA_NOACTIVATE;

		case WM_LBUTTONDOWN: {
				// We must take control of selection to prevent the ListBox activating
				// the popup
				const LRESULT lResult = ::SendMessage(hWnd, LB_ITEMFROMPOINT, 0, lParam);
				if (HIWORD(lResult) == 0) {
					ListBox_SetCurSel(hWnd, LOWORD(lResult));
					if (lbx) {
						lbx->OnSelChange();
					}
				}
			}
			return 0;

		case WM_LBUTTONUP:
			return 0;

		case WM_LBUTTONDBLCLK: {
				if (lbx) {
					lbx->OnDoubleClick();
				}
			}
			return 0;

		case WM_MBUTTONDOWN:
			// disable the scroll wheel button click action
			return 0;

		default:
			break;
		}

		WNDPROC prevWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (prevWndProc) {
			return ::CallWindowProc(prevWndProc, hWnd, iMessage, wParam, lParam);
		}
		return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
	} catch (...) {
	}
	return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
}

LRESULT ListBoxX::WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	switch (iMessage) {
	case WM_CREATE: {
			HINSTANCE hinstanceParent = GetWindowInstance(HwndFromWindow(*parent));
			// Note that LBS_NOINTEGRALHEIGHT is specified to fix cosmetic issue when resizing the list
			// but has useful side effect of speeding up list population significantly
			lb = ::CreateWindowEx(
				0, TEXT("listbox"), TEXT(""),
				WS_CHILD | WS_VSCROLL | WS_VISIBLE |
				LBS_OWNERDRAWFIXED | LBS_NODATA | LBS_NOINTEGRALHEIGHT,
				0, 0, 150,80, hWnd,
				reinterpret_cast<HMENU>(static_cast<ptrdiff_t>(ctrlID)),
				hinstanceParent,
				nullptr);
			WNDPROC prevWndProc = SubclassWindow(lb, ControlWndProc);
			::SetWindowLongPtr(lb, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(prevWndProc));
		}
		break;

	case WM_SIZE:
		if (lb) {
			SetRedraw(false);
			::SetWindowPos(lb, {}, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
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
		::SendMessage(HwndFromWindow(*parent), iMessage, wParam, lParam);
		break;

	case WM_MEASUREITEM: {
			MEASUREITEMSTRUCT *pMeasureItem = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
			pMeasureItem->itemHeight = ItemHeight();
		}
		break;

	case WM_DRAWITEM:
		Draw(reinterpret_cast<DRAWITEMSTRUCT *>(lParam));
		break;

	case WM_DESTROY:
		lb = {};
		SetWindowPointer(hWnd, nullptr);
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
			}
			ResizeToCursor();
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
		if (wheelDelta.Accumulate(wParam)) {
			const int nRows = GetVisibleRows();
			int linesToScroll = std::clamp(nRows - 1, 1, 3);
			linesToScroll *= wheelDelta.Actions();
			int top = ListBox_GetTopIndex(lb) + linesToScroll;
			if (top < 0) {
				top = 0;
			}
			ListBox_SetTopIndex(lb, top);
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
	ListBoxX *lbx = static_cast<ListBoxX *>(PointerFromWindow(hWnd));
	if (lbx) {
		return lbx->WndProc(hWnd, iMessage, wParam, lParam);
	}
	return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
}

namespace {

bool ListBoxX_Register() noexcept {
	WNDCLASSEX wndclassc {};
	wndclassc.cbSize = sizeof(wndclassc);
	// We need CS_HREDRAW and CS_VREDRAW because of the ellipsis that might be drawn for
	// truncated items in the list and the appearance/disappearance of the vertical scroll bar.
	// The list repaint is double-buffered to avoid the flicker this would otherwise cause.
	wndclassc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wndclassc.cbWndExtra = sizeof(ListBoxX *);
	wndclassc.hInstance = hinstPlatformRes;
	wndclassc.lpfnWndProc = ListBoxX::StaticWndProc;
	wndclassc.hCursor = ::LoadCursor({}, IDC_ARROW);
	wndclassc.lpszClassName = ListBoxX_ClassName;

	return ::RegisterClassEx(&wndclassc) != 0;
}

void ListBoxX_Unregister() noexcept {
	if (hinstPlatformRes) {
		::UnregisterClass(ListBoxX_ClassName, hinstPlatformRes);
	}
}

}

Menu::Menu() noexcept : mid{} {
}

void Menu::CreatePopUp() {
	Destroy();
	mid = ::CreatePopupMenu();
}

void Menu::Destroy() noexcept {
	if (mid)
		::DestroyMenu(static_cast<HMENU>(mid));
	mid = {};
}

void Menu::Show(Point pt, const Window &w) {
	::TrackPopupMenu(static_cast<HMENU>(mid),
		TPM_RIGHTBUTTON, static_cast<int>(pt.x - 4), static_cast<int>(pt.y), 0,
		HwndFromWindow(w), nullptr);
	Destroy();
}

ColourRGBA Platform::Chrome() {
	return ColourRGBA::FromRGB(static_cast<int>(::GetSysColor(COLOR_3DFACE)));
}

ColourRGBA Platform::ChromeHighlight() {
	return ColourRGBA::FromRGB(static_cast<int>(::GetSysColor(COLOR_3DHIGHLIGHT)));
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

void Platform::DebugDisplay(const char *s) noexcept {
	::OutputDebugStringA(s);
}

//#define TRACE

#ifdef TRACE
void Platform::DebugPrintf(const char *format, ...) noexcept {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsnprintf(buffer, std::size(buffer), format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}
#else
void Platform::DebugPrintf(const char *, ...) noexcept {
}
#endif

static bool assertionPopUps = true;

bool Platform::ShowAssertionPopUps(bool assertionPopUps_) noexcept {
	const bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) noexcept {
	char buffer[2000] {};
	snprintf(buffer, std::size(buffer), "Assertion [%s] failed at %s %d%s", c, file, line, assertionPopUps ? "" : "\r\n");
	if (assertionPopUps) {
		const int idButton = ::MessageBoxA({}, buffer, "Assertion failure",
			MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);
		if (idButton == IDRETRY) {
			::DebugBreak();
		} else if (idButton == IDIGNORE) {
			// all OK
		} else {
			abort();
		}
	} else {
		Platform::DebugDisplay(buffer);
		::DebugBreak();
		abort();
	}
}

void Platform_Initialise(void *hInstance) noexcept {
	hinstPlatformRes = static_cast<HINSTANCE>(hInstance);
	LoadDpiForWindow();
	ListBoxX_Register();
}

namespace {

void ReleaseLibrary(HMODULE &hLib) noexcept {
	if (hLib) {
		FreeLibrary(hLib);
		hLib = {};
	}
}

}

void Platform_Finalise(bool fromDllMain) noexcept {
	if (!fromDllMain) {
#if defined(USE_D2D)
		ReleaseUnknown(pIDWriteFactory);
		ReleaseUnknown(pD2DFactory);
		ReleaseLibrary(hDLLDWrite);
		ReleaseLibrary(hDLLD3D);
		ReleaseLibrary(hDLLD2D);
#endif
		ReleaseLibrary(hDLLShcore);
	}
	ListBoxX_Unregister();
}

}
