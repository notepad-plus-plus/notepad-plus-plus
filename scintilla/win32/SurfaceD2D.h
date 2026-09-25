// Scintilla source code edit control
/** @file SurfaceD2D.h
 ** Definitions for drawing to Direct2D on Windows.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SURFACED2D_H
#define SURFACED2D_H

namespace Scintilla::Internal {

extern bool LoadD2D() noexcept;
extern void ReleaseD2D() noexcept;
extern ID2D1Factory1 *pD2DFactory;
extern IDWriteFactory1 *pIDWriteFactory;

using DCRenderTarget = ComPtr<ID2D1DCRenderTarget>;

using D3D11Device = ComPtr<ID3D11Device1>;

HRESULT CreateDCRenderTarget(const D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, DCRenderTarget &dcRT) noexcept;
extern HRESULT CreateD3D(D3D11Device &device) noexcept;

using WriteRenderingParams = ComPtr<IDWriteRenderingParams1>;

// N++: text drawn with its own rendering parameters when SCI_SETFONTRENDERINGPARAMETER asks for it,
// combined as bits: light text gets a higher gamma which makes it heavier (dark text gets lighter),
// small text is drawn without vertical antialiasing in the adaptive rendering mode.
constexpr int renderingVariantLight = 1;
constexpr int renderingVariantSmall = 2;
constexpr int renderingVariants = 4;

struct RenderingParams {
	WriteRenderingParams defaultRenderingParams;
	WriteRenderingParams customRenderingParams;
	WriteRenderingParams monitorRenderingParams;	// N++: monitor's parameters without user overrides, for aliased text
	// N++: parameters of the default and custom ones for each variant, empty when unused ([0] is always empty)
	WriteRenderingParams defaultVariants[renderingVariants];
	WriteRenderingParams customVariants[renderingVariants];
};

// N++: FontQuality bits above FontQuality::QualityMask select DirectWrite GDI-compatible
// text measuring so that fonts are realised and cached separately for each measuring mode.
constexpr int fontQualityMeasuringGdiClassic = 0x10;
constexpr int fontQualityMeasuringGdiNatural = 0x20;
constexpr int fontQualityMeasuringMask = 0x30;

struct ISetRenderingParams {
	virtual void SetRenderingParams(std::shared_ptr<RenderingParams> renderingParams_) = 0;
};

using BrushSolid = ComPtr<ID2D1SolidColorBrush>;
using Geometry = ComPtr<ID2D1PathGeometry>;
using GeometrySink = ComPtr<ID2D1GeometrySink>;
using StrokeStyle = ComPtr<ID2D1StrokeStyle>;
using TextLayout = ComPtr<IDWriteTextLayout>;

BrushSolid BrushSolidCreate(ID2D1RenderTarget *pTarget, COLORREF colour) noexcept;
Geometry GeometryCreate() noexcept;
GeometrySink GeometrySinkCreate(ID2D1PathGeometry *geometry) noexcept;
StrokeStyle StrokeStyleCreate(const D2D1_STROKE_STYLE_PROPERTIES &strokeStyleProperties) noexcept;
// N++: measuringMode added, pass the font's mode so text is measured as it is drawn
TextLayout LayoutCreate(std::wstring_view wsv, IDWriteTextFormat *pTextFormat, DWRITE_MEASURING_MODE measuringMode, FLOAT maxWidth=10000.0F, FLOAT maxHeight=1000.0F) noexcept;

}

#endif
