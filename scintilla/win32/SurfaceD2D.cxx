// Scintilla source code edit control
/** @file SurfaceD2D.cxx
 ** Implementation of drawing to Direct2D on Windows.
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
#include "SurfaceGDI.h"
#if defined(USE_D2D)
#include "SurfaceD2D.h"
#endif

// __uuidof is a Microsoft extension but makes COM code neater, so disable warning
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

using namespace Scintilla;
using namespace Scintilla::Internal;

namespace {

#if defined(USE_D2D)

D2D1_DRAW_TEXT_OPTIONS d2dDrawTextOptions = D2D1_DRAW_TEXT_OPTIONS_NONE;

HMODULE hDLLD2D{};
HMODULE hDLLD3D{};
HMODULE hDLLDWrite{};

PFN_D3D11_CREATE_DEVICE fnDCD{};

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

	using D2D1CFSig = HRESULT(WINAPI *)(D2D1_FACTORY_TYPE factoryType, REFIID riid,
		CONST D2D1_FACTORY_OPTIONS *pFactoryOptions, IUnknown **factory);
	using DWriteCFSig = HRESULT(WINAPI *)(DWRITE_FACTORY_TYPE factoryType, REFIID iid,
		IUnknown **factory);

	hDLLD2D = ::LoadLibraryEx(TEXT("D2D1.DLL"), {}, loadLibraryFlags);
	D2D1CFSig fnD2DCF = DLLFunction<D2D1CFSig>(hDLLD2D, "D2D1CreateFactory");
	if (fnD2DCF) {
		const D2D1_FACTORY_OPTIONS options{};
		// A multi threaded factory in case Scintilla is used with multiple GUI threads
		fnD2DCF(D2D1_FACTORY_TYPE_MULTI_THREADED,
			__uuidof(ID2D1Factory1),
			&options,
			reinterpret_cast<IUnknown **>(&pD2DFactory));
	}
	hDLLDWrite = ::LoadLibraryEx(TEXT("DWRITE.DLL"), {}, loadLibraryFlags);
	DWriteCFSig fnDWCF = DLLFunction<DWriteCFSig>(hDLLDWrite, "DWriteCreateFactory");
	if (fnDWCF) {
		const GUID IID_IDWriteFactory2 = // 0439fc60-ca44-4994-8dee-3a9af7b732ec
		{ 0x0439fc60, 0xca44, 0x4994, { 0x8d, 0xee, 0x3a, 0x9a, 0xf7, 0xb7, 0x32, 0xec } };

		const HRESULT hr = fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
			IID_IDWriteFactory2,
			reinterpret_cast<IUnknown **>(&pIDWriteFactory));
		if (SUCCEEDED(hr)) {
			// D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
			d2dDrawTextOptions = static_cast<D2D1_DRAW_TEXT_OPTIONS>(0x00000004);
		} else {
			fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory1),
				reinterpret_cast<IUnknown **>(&pIDWriteFactory));
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

struct FontDirectWrite : public FontWin {
	ComPtr<IDWriteTextFormat> pTextFormat;
	FontQuality extraFontFlag = FontQuality::QualityDefault;
	CharacterSet characterSet = CharacterSet::Ansi;
	static constexpr FLOAT minimalAscent = 2.0f;
	FLOAT yAscent = minimalAscent;
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
	// Allow copy constructor. Has to explicitly copy each field since can't use =default as deleted in Font.
	FontDirectWrite(const FontDirectWrite &other) noexcept {
		pTextFormat = other.pTextFormat;
		extraFontFlag = other.extraFontFlag;
		characterSet = other.characterSet;
		yAscent = other.yAscent;
		yDescent = other.yDescent;
		yInternalLeading = other.yInternalLeading;
	}
	// Deleted so FontDirectWrite objects can not be copied.
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

	[[nodiscard]] std::unique_ptr<FontWin> Duplicate() const override {
		return std::make_unique<FontDirectWrite>(*this);
	}

	[[nodiscard]] CharacterSet GetCharacterSet() const noexcept override {
		return characterSet;
	}
};

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

constexpr D2D_COLOR_F ColorFromColourAlpha(ColourRGBA colour) noexcept {
	return D2D_COLOR_F{
		colour.GetRedComponent(),
		colour.GetGreenComponent(),
		colour.GetBlueComponent(),
		colour.GetAlphaComponent()
	};
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
	deviceScaleFactor = static_cast<int>(dpiX / dpiDefault);
}

int SurfaceD2D::PixelDivisions() {
	return deviceScaleFactor;
}

int SurfaceD2D::DeviceHeightFont(int points) {
	return ::MulDiv(points, LogPixelsY(), pointsPerInch);
}

constexpr FLOAT mitreLimit = 4.0f;

void SurfaceD2D::LineDraw(Point start, Point end, Stroke stroke) {
	D2DPenColourAlpha(stroke.colour);

	D2D1_STROKE_STYLE_PROPERTIES strokeProps {};
	strokeProps.startCap = D2D1_CAP_STYLE_SQUARE;
	strokeProps.endCap = D2D1_CAP_STYLE_SQUARE;
	strokeProps.dashCap = D2D1_CAP_STYLE_FLAT;
	strokeProps.lineJoin = D2D1_LINE_JOIN_MITER;
	strokeProps.miterLimit = mitreLimit;
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
	strokeProps.miterLimit = mitreLimit;
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

	const FLOAT radiusFill = static_cast<FLOAT>((rc.Width() / 2.0f) - fillStroke.stroke.width);
	const D2D1_ELLIPSE ellipseFill = { centre, radiusFill, radiusFill };

	D2DPenColourAlpha(fillStroke.fill.colour);
	pRenderTarget->FillEllipse(ellipseFill, pBrush.Get());

	const FLOAT radiusOutline = static_cast<FLOAT>((rc.Width() / 2.0f) - (fillStroke.stroke.width / 2.0f));
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
	std::string text;
	std::wstring buffer;
	std::vector<BlobInline> blobs;
	// textLayout depends on blobs so must be declared after blobs so it is destroyed before blobs.
	TextLayout textLayout;
	static void FillTextLayoutFormats(const IScreenLine *screenLine, IDWriteTextLayout *textLayout, std::vector<BlobInline> &blobs);
	static std::wstring ReplaceRepresentation(std::string_view text);
	static UINT32 GetPositionInLayout(std::string_view text, size_t position);
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
			blobs.emplace_back(representationWidth);
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

UINT32 ScreenLineLayout::GetPositionInLayout(std::string_view text, size_t position) {
	const std::string_view textUptoPosition = text.substr(0, position);
	return static_cast<UINT32>(UTF16Length(textUptoPosition));
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
	const UINT32 position = GetPositionInLayout(text, caretPosition);

	// Translate text character offset to point x,y.
	DWRITE_HIT_TEST_METRICS caretMetrics {};
	D2D1_POINT_2F pt {};

	textLayout->HitTestTextPosition(
		position,
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
	const UINT32 startPos = GetPositionInLayout(text, start);
	const UINT32 endPos = GetPositionInLayout(text, end);

	// Find selection range length
	const UINT32 rangeLength = (endPos > startPos) ? (endPos - startPos) : (startPos - endPos);

	// Determine actual number of hit-test ranges
	UINT32 hitTestCount = 2;	// Simple selection often produces just 2 hits

	// First try with 2 elements and if more needed, allocate.
	std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(hitTestCount);
	textLayout->HitTestTextRange(
		startPos,
		rangeLength,
		0, // x
		0, // y
		hitTestMetrics.data(),
		hitTestCount,
		&hitTestCount
	);

	if (hitTestCount == 0) {
		return ret;
	}

	if (hitTestMetrics.size() < hitTestCount) {
		// Allocate enough room to return all hit-test metrics.
		hitTestMetrics.resize(hitTestCount);
		textLayout->HitTestTextRange(
			startPos,
			rangeLength,
			0, // x
			0, // y
			hitTestMetrics.data(),
			hitTestCount,
			&hitTestCount
		);
	}

	// Get the selection ranges behind the text.

	for (size_t i = 0; i < hitTestCount; ++i) {
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

}

namespace Scintilla::Internal {

#if defined(USE_D2D)
IDWriteFactory1 *pIDWriteFactory = nullptr;
ID2D1Factory1 *pD2DFactory = nullptr;

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
	}
	catch (...) {
		// ignore
	}
	return pIDWriteFactory && pD2DFactory;
}

void ReleaseD2D() noexcept {
	ReleaseUnknown(pIDWriteFactory);
	ReleaseUnknown(pD2DFactory);
	ReleaseLibrary(hDLLDWrite);
	ReleaseLibrary(hDLLD3D);
	ReleaseLibrary(hDLLD2D);
}

HRESULT CreateDCRenderTarget(const D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, DCRenderTarget &dcRT) noexcept {
	return pD2DFactory->CreateDCRenderTarget(renderTargetProperties, dcRT.ReleaseAndGetAddressOf());
}

BrushSolid BrushSolidCreate(ID2D1RenderTarget *pTarget, COLORREF colour) noexcept {
	BrushSolid brush;
	const D2D_COLOR_F col = ColorFromColourAlpha(ColourRGBA::FromRGB(colour));
	if (FAILED(pTarget->CreateSolidColorBrush(col, brush.GetAddressOf()))) {
		return {};
	}
	return brush;
}

Geometry GeometryCreate() noexcept {
	Geometry geometry;
	if (FAILED(pD2DFactory->CreatePathGeometry(geometry.GetAddressOf()))) {
		return {};
	}
	return geometry;
}

GeometrySink GeometrySinkCreate(ID2D1PathGeometry *geometry) noexcept {
	GeometrySink sink;
	if (FAILED(geometry->Open(sink.GetAddressOf()))) {
		return {};
	}
	return sink;
}

StrokeStyle StrokeStyleCreate(const D2D1_STROKE_STYLE_PROPERTIES &strokeStyleProperties) noexcept {
	StrokeStyle strokeStyle;
	const HRESULT hr = pD2DFactory->CreateStrokeStyle(
		strokeStyleProperties, nullptr, 0, strokeStyle.GetAddressOf());
	if (FAILED(hr)) {
		return {};
	}
	return strokeStyle;
}

TextLayout LayoutCreate(std::wstring_view wsv, IDWriteTextFormat *pTextFormat, FLOAT maxWidth, FLOAT maxHeight) noexcept {
	TextLayout layout;
	const HRESULT hr = pIDWriteFactory->CreateTextLayout(wsv.data(), static_cast<UINT32>(wsv.length()),
		pTextFormat, maxWidth, maxHeight, layout.GetAddressOf());
	if (FAILED(hr)) {
		return {};
	}
	return layout;
}

#endif

std::shared_ptr<Font> Font::Allocate(const FontParameters &fp) {
#if defined(USE_D2D)
	if (fp.technology != Technology::Default) {
		return std::make_shared<FontDirectWrite>(fp);
	}
#endif
	return FontGDI_Allocate(fp);
}

std::unique_ptr<Surface> Surface::Allocate([[maybe_unused]] Technology technology) {
#if defined(USE_D2D)
	if (technology != Technology::Default) {
		return std::make_unique<SurfaceD2D>();
	}
#endif
	return SurfaceGDI_Allocate();
}

}
