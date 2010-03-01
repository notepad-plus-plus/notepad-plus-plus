#include <windows.h>
#include <uxtheme.h>

typedef DWORD ARGB;

void InitBitmapInfo(BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp);
HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, void **ppvBits, HBITMAP* phBmp);
HRESULT ConvertToPARGB32(HDC hdc, ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow);
bool HasAlpha(ARGB *pargb, SIZE& sizImage, int cxRow);
HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon);
