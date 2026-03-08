#pragma once
// Win32 Shim: GDI type definitions for macOS
// GDI drawing will map to CoreGraphics in implementation

#include "windef.h"

// ============================================================
// Stock objects
// ============================================================
#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define HOLLOW_BRUSH        NULL_BRUSH
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16
#define DEFAULT_GUI_FONT    17

// ============================================================
// Pen styles
// ============================================================
#define PS_SOLID       0
#define PS_DASH        1
#define PS_DOT         2
#define PS_DASHDOT     3
#define PS_DASHDOTDOT  4
#define PS_NULL        5
#define PS_INSIDEFRAME 6

// ============================================================
// Brush styles
// ============================================================
#define BS_SOLID         0
#define BS_NULL          1
#define BS_HOLLOW        BS_NULL
#define BS_HATCHED       2
#define BS_PATTERN       3
#define BS_INDEXED       4
#define BS_DIBPATTERN    5
#define BS_DIBPATTERNPT  6
#define BS_PATTERN8X8    7
#define BS_DIBPATTERN8X8 8
#define BS_MONOPATTERN   9

// ============================================================
// Hatch styles
// ============================================================
#define HS_HORIZONTAL 0
#define HS_VERTICAL   1
#define HS_FDIAGONAL  2
#define HS_BDIAGONAL  3
#define HS_CROSS      4
#define HS_DIAGCROSS  5

// ============================================================
// ROP2 codes
// ============================================================
#define R2_BLACK       1
#define R2_NOTMERGEPEN 2
#define R2_MASKNOTPEN  3
#define R2_NOTCOPYPEN  4
#define R2_MASKPENNOT  5
#define R2_NOT         6
#define R2_XORPEN      7
#define R2_NOTMASKPEN  8
#define R2_MASKPEN     9
#define R2_NOTXORPEN   10
#define R2_NOP         11
#define R2_MERGENOTPEN 12
#define R2_COPYPEN     13
#define R2_MERGEPENNOT 14
#define R2_MERGEPEN    15
#define R2_WHITE       16

// ============================================================
// Ternary raster operations
// ============================================================
#define SRCCOPY     0x00CC0020
#define SRCPAINT    0x00EE0086
#define SRCAND      0x008800C6
#define SRCINVERT   0x00660046
#define SRCERASE    0x00440328
#define NOTSRCCOPY  0x00330008
#define NOTSRCERASE 0x001100A6
#define MERGECOPY   0x00C000CA
#define MERGEPAINT  0x00BB0226
#define PATCOPY     0x00F00021
#define PATPAINT    0x00FB0A09
#define PATINVERT   0x005A0049
#define DSTINVERT   0x00550009
#define BLACKNESS   0x00000042
#define WHITENESS   0x00FF0062

// ============================================================
// Background modes
// ============================================================
#define TRANSPARENT 1
#define OPAQUE      2

// ============================================================
// Text alignment
// ============================================================
#define TA_NOUPDATECP 0
#define TA_UPDATECP   1
#define TA_LEFT       0
#define TA_RIGHT      2
#define TA_CENTER     6
#define TA_TOP        0
#define TA_BOTTOM     8
#define TA_BASELINE   24
#define TA_RTLREADING 256

// ============================================================
// Font weight
// ============================================================
#define FW_DONTCARE   0
#define FW_THIN       100
#define FW_EXTRALIGHT 200
#define FW_ULTRALIGHT 200
#define FW_LIGHT      300
#define FW_NORMAL     400
#define FW_REGULAR    400
#define FW_MEDIUM     500
#define FW_SEMIBOLD   600
#define FW_DEMIBOLD   600
#define FW_BOLD       700
#define FW_EXTRABOLD  800
#define FW_ULTRABOLD  800
#define FW_HEAVY      900
#define FW_BLACK      900

// ============================================================
// Font charset
// ============================================================
#define ANSI_CHARSET        0
#define DEFAULT_CHARSET     1
#define SYMBOL_CHARSET      2
#define SHIFTJIS_CHARSET    128
#define HANGEUL_CHARSET     129
#define HANGUL_CHARSET      129
#define GB2312_CHARSET      134
#define CHINESEBIG5_CHARSET 136
#define OEM_CHARSET         255
#define JOHAB_CHARSET       130
#define HEBREW_CHARSET      177
#define ARABIC_CHARSET      178
#define GREEK_CHARSET       161
#define TURKISH_CHARSET     162
#define VIETNAMESE_CHARSET  163
#define THAI_CHARSET        222
#define EASTEUROPE_CHARSET  238
#define RUSSIAN_CHARSET     204
#define MAC_CHARSET         77
#define BALTIC_CHARSET      186

// ============================================================
// Font output precision
// ============================================================
#define OUT_DEFAULT_PRECIS   0
#define OUT_STRING_PRECIS    1
#define OUT_CHARACTER_PRECIS 2
#define OUT_STROKE_PRECIS    3
#define OUT_TT_PRECIS        4
#define OUT_DEVICE_PRECIS    5
#define OUT_RASTER_PRECIS    6
#define OUT_TT_ONLY_PRECIS   7
#define OUT_OUTLINE_PRECIS   8
#define OUT_PS_ONLY_PRECIS   10

// ============================================================
// Font clip precision
// ============================================================
#define CLIP_DEFAULT_PRECIS   0
#define CLIP_CHARACTER_PRECIS 1
#define CLIP_STROKE_PRECIS    2
#define CLIP_MASK             0x0F

// ============================================================
// Font quality
// ============================================================
#define DEFAULT_QUALITY        0
#define DRAFT_QUALITY          1
#define PROOF_QUALITY          2
#define NONANTIALIASED_QUALITY 3
#define ANTIALIASED_QUALITY    4
#define CLEARTYPE_QUALITY      5
#define CLEARTYPE_NATURAL_QUALITY 6

// ============================================================
// Font pitch and family
// ============================================================
#define DEFAULT_PITCH  0
#define FIXED_PITCH    1
#define VARIABLE_PITCH 2
#define MONO_FONT      8

#define FF_DONTCARE   (0 << 4)
#define FF_ROMAN      (1 << 4)
#define FF_SWISS      (2 << 4)
#define FF_MODERN     (3 << 4)
#define FF_SCRIPT     (4 << 4)
#define FF_DECORATIVE (5 << 4)

// ============================================================
// LOGFONT
// ============================================================
#define LF_FACESIZE 32

typedef struct tagLOGFONTW {
	LONG  lfHeight;
	LONG  lfWidth;
	LONG  lfEscapement;
	LONG  lfOrientation;
	LONG  lfWeight;
	BYTE  lfItalic;
	BYTE  lfUnderline;
	BYTE  lfStrikeOut;
	BYTE  lfCharSet;
	BYTE  lfOutPrecision;
	BYTE  lfClipPrecision;
	BYTE  lfQuality;
	BYTE  lfPitchAndFamily;
	WCHAR lfFaceName[LF_FACESIZE];
} LOGFONTW, *LPLOGFONTW;
typedef LOGFONTW LOGFONT;
typedef LPLOGFONTW LPLOGFONT;
typedef const LOGFONTW* LPCLOGFONTW;

// ============================================================
// TEXTMETRIC
// ============================================================
typedef struct tagTEXTMETRICW {
	LONG  tmHeight;
	LONG  tmAscent;
	LONG  tmDescent;
	LONG  tmInternalLeading;
	LONG  tmExternalLeading;
	LONG  tmAveCharWidth;
	LONG  tmMaxCharWidth;
	LONG  tmWeight;
	LONG  tmOverhang;
	LONG  tmDigitizedAspectX;
	LONG  tmDigitizedAspectY;
	WCHAR tmFirstChar;
	WCHAR tmLastChar;
	WCHAR tmDefaultChar;
	WCHAR tmBreakChar;
	BYTE  tmItalic;
	BYTE  tmUnderlined;
	BYTE  tmStruckOut;
	BYTE  tmPitchAndFamily;
	BYTE  tmCharSet;
} TEXTMETRICW, *LPTEXTMETRICW;
typedef TEXTMETRICW TEXTMETRIC;
typedef LPTEXTMETRICW LPTEXTMETRIC;

// ============================================================
// BITMAPINFOHEADER / BITMAPINFO
// ============================================================
typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;
	LONG  biWidth;
	LONG  biHeight;
	WORD  biPlanes;
	WORD  biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG  biXPelsPerMeter;
	LONG  biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD          bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

typedef struct tagBITMAPFILEHEADER {
	WORD  bfType;
	DWORD bfSize;
	WORD  bfReserved1;
	WORD  bfReserved2;
	DWORD bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER, *LPBITMAPFILEHEADER;

// ============================================================
// BITMAP
// ============================================================
typedef struct tagBITMAP {
	LONG   bmType;
	LONG   bmWidth;
	LONG   bmHeight;
	LONG   bmWidthBytes;
	WORD   bmPlanes;
	WORD   bmBitsPixel;
	LPVOID bmBits;
} BITMAP, *PBITMAP, *LPBITMAP;

// ============================================================
// DIB usage
// ============================================================
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1

// ============================================================
// Bitmap compression
// ============================================================
#define BI_RGB       0L
#define BI_RLE8      1L
#define BI_RLE4      2L
#define BI_BITFIELDS 3L
#define BI_JPEG      4L
#define BI_PNG       5L

// ============================================================
// GDI object types for GetObjectType
// ============================================================
#define OBJ_PEN        1
#define OBJ_BRUSH      2
#define OBJ_DC         3
#define OBJ_METADC     4
#define OBJ_PAL        5
#define OBJ_FONT       6
#define OBJ_BITMAP     7
#define OBJ_REGION     8
#define OBJ_METAFILE   9
#define OBJ_MEMDC      10
#define OBJ_EXTPEN     11
#define OBJ_ENHMETADC  12
#define OBJ_ENHMETAFILE 13

// ============================================================
// Region types
// ============================================================
#define ERROR_REGION    0
#define NULLREGION      1
#define SIMPLEREGION    2
#define COMPLEXREGION   3

#define RGN_AND  1
#define RGN_OR   2
#define RGN_XOR  3
#define RGN_DIFF 4
#define RGN_COPY 5

// ============================================================
// Mapping modes
// ============================================================
#define MM_TEXT        1
#define MM_LOMETRIC   2
#define MM_HIMETRIC   3
#define MM_LOENGLISH  4
#define MM_HIENGLISH  5
#define MM_TWIPS       6
#define MM_ISOTROPIC  7
#define MM_ANISOTROPIC 8

// ============================================================
// StretchBlt mode
// ============================================================
#define BLACKONWHITE 1
#define WHITEONBLACK 2
#define COLORONCOLOR 3
#define HALFTONE     4
#define STRETCH_ANDSCANS    BLACKONWHITE
#define STRETCH_ORSCANS     WHITEONBLACK
#define STRETCH_DELETESCANS COLORONCOLOR
#define STRETCH_HALFTONE    HALFTONE

// ============================================================
// GetDeviceCaps indices
// ============================================================
#define HORZRES       8
#define VERTRES       10
#define BITSPIXEL     12
#define PLANES        14
#define LOGPIXELSX    88
#define LOGPIXELSY    90
#define SIZEPALETTE   104
#define NUMRESERVED      106
#define PHYSICALWIDTH    110
#define PHYSICALHEIGHT   111
#define PHYSICALOFFSETX  112
#define PHYSICALOFFSETY  113

// ============================================================
// XFORM
// ============================================================
typedef struct tagXFORM {
	FLOAT eM11;
	FLOAT eM12;
	FLOAT eM21;
	FLOAT eM22;
	FLOAT eDx;
	FLOAT eDy;
} XFORM, *PXFORM, *LPXFORM;

// ============================================================
// BLENDFUNCTION (for AlphaBlend)
// ============================================================
typedef struct _BLENDFUNCTION {
	BYTE BlendOp;
	BYTE BlendFlags;
	BYTE SourceConstantAlpha;
	BYTE AlphaFormat;
} BLENDFUNCTION, *PBLENDFUNCTION;

#define AC_SRC_OVER  0x00
#define AC_SRC_ALPHA 0x01

// ============================================================
// EnumFonts callback
// ============================================================
typedef int (*FONTENUMPROCW)(const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM);
typedef FONTENUMPROCW FONTENUMPROC;

// Font types for enumeration
#define RASTER_FONTTYPE   0x0001
#define DEVICE_FONTTYPE   0x0002
#define TRUETYPE_FONTTYPE 0x0004

// Extended LOGFONT struct
typedef struct tagENUMLOGFONTEXW {
	LOGFONTW elfLogFont;
	WCHAR    elfFullName[64];
	WCHAR    elfStyle[32];
	WCHAR    elfScript[32];
} ENUMLOGFONTEXW, *LPENUMLOGFONTEXW;
typedef ENUMLOGFONTEXW ENUMLOGFONTEX;

// EnumFontFamiliesEx
int EnumFontFamiliesExW(HDC hdc, LPLOGFONTW lpLogfont, FONTENUMPROCW lpProc, LPARAM lParam, DWORD dwFlags);
#define EnumFontFamiliesEx EnumFontFamiliesExW

// GDI text output
#define ETO_OPAQUE   0x0002
#define ETO_CLIPPED  0x0004

#define TA_LEFT      0
#define TA_RIGHT     2
#define TA_CENTER    6
#define TA_TOP       0
#define TA_BOTTOM    8
#define TA_BASELINE  24

inline BOOL ExtTextOutW(HDC hdc, int x, int y, UINT options, const RECT* lprect, LPCWSTR lpString, UINT c, const INT* lpDx)
{
	(void)hdc; (void)x; (void)y; (void)options; (void)lprect; (void)lpString; (void)c; (void)lpDx;
	return TRUE;
}
#define ExtTextOut ExtTextOutW

inline UINT SetTextAlign(HDC hdc, UINT fMode) { (void)hdc; (void)fMode; return 0; }

// GDI coordinate conversion
inline BOOL DPtoLP(HDC hdc, LPPOINT lppt, int c) { (void)hdc; (void)lppt; (void)c; return TRUE; }
inline BOOL LPtoDP(HDC hdc, LPPOINT lppt, int c) { (void)hdc; (void)lppt; (void)c; return TRUE; }

// Printing (stubs)
typedef struct _DOCINFOW {
	int     cbSize;
	LPCWSTR lpszDocName;
	LPCWSTR lpszOutput;
	LPCWSTR lpszDatatype;
	DWORD   fwType;
} DOCINFOW, *LPDOCINFOW;
typedef DOCINFOW DOCINFO;

inline int StartDocW(HDC hdc, const DOCINFOW* lpdi) { (void)hdc; (void)lpdi; return 1; }
#define StartDoc StartDocW
inline int EndDoc(HDC hdc) { (void)hdc; return 1; }
inline int StartPage(HDC hdc) { (void)hdc; return 1; }
inline int EndPage(HDC hdc) { (void)hdc; return 1; }
inline int AbortDoc(HDC hdc) { (void)hdc; return 1; }

// GDI coordinate functions (stubs)
inline BOOL OffsetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lppt)
{
	(void)hdc; (void)x; (void)y;
	if (lppt) { lppt->x = 0; lppt->y = 0; }
	return TRUE;
}

inline BOOL SetWindowOrgEx(HDC hdc, int x, int y, LPPOINT lppt)
{
	(void)hdc; (void)x; (void)y;
	if (lppt) { lppt->x = 0; lppt->y = 0; }
	return TRUE;
}

inline BOOL SetViewportOrgEx(HDC hdc, int x, int y, LPPOINT lppt)
{
	(void)hdc; (void)x; (void)y;
	if (lppt) { lppt->x = 0; lppt->y = 0; }
	return TRUE;
}
