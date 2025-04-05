// Scintilla source code edit control
/** @file PlatWin.h
 ** Implementation of platform facilities on Windows.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PLATWIN_H
#define PLATWIN_H

namespace Scintilla::Internal {

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI		96
#endif
constexpr FLOAT dpiDefault = USER_DEFAULT_SCREEN_DPI;

// Used for defining font size with LOGFONT
constexpr int pointsPerInch = 72;

extern void Platform_Initialise(void *hInstance) noexcept;

extern void Platform_Finalise(bool fromDllMain) noexcept;

constexpr RECT RectFromPRectangle(PRectangle prc) noexcept {
	const RECT rc = { static_cast<LONG>(prc.left), static_cast<LONG>(prc.top),
		static_cast<LONG>(prc.right), static_cast<LONG>(prc.bottom) };
	return rc;
}

constexpr PRectangle PRectangleFromRECT(RECT rc) noexcept {
	return PRectangle::FromInts(rc.left, rc.top, rc.right, rc.bottom);
}

constexpr POINT POINTFromPoint(Point pt) noexcept {
	return POINT{ static_cast<LONG>(pt.x), static_cast<LONG>(pt.y) };
}

constexpr Point PointFromPOINT(POINT pt) noexcept {
	return Point::FromInts(pt.x, pt.y);
}

constexpr SIZE SizeOfRect(RECT rc) noexcept {
	return { rc.right - rc.left, rc.bottom - rc.top };
}

ColourRGBA ColourFromSys(int nIndex) noexcept;

constexpr HWND HwndFromWindowID(WindowID wid) noexcept {
	return static_cast<HWND>(wid);
}

inline HWND HwndFromWindow(const Window &w) noexcept {
	return HwndFromWindowID(w.GetID());
}

extern HINSTANCE hinstPlatformRes;

UINT CodePageFromCharSet(CharacterSet characterSet, UINT documentCodePage) noexcept;

void *PointerFromWindow(HWND hWnd) noexcept;
void SetWindowPointer(HWND hWnd, void *ptr) noexcept;

HMONITOR MonitorFromWindowHandleScaling(HWND hWnd) noexcept;

UINT DpiForWindow(WindowID wid) noexcept;
float GetDeviceScaleFactorWhenGdiScalingActive(HWND hWnd) noexcept;

int SystemMetricsForDpi(int nIndex, UINT dpi) noexcept;

void AdjustWindowRectForDpi(LPRECT lpRect, DWORD dwStyle, UINT dpi) noexcept;

HCURSOR LoadReverseArrowCursor(UINT dpi) noexcept;

// Encapsulate WM_PAINT handling so that EndPaint is always called even with unexpected returns or exceptions.
struct Painter {
	HWND hWnd{};
	PAINTSTRUCT ps{};
	explicit Painter(HWND hWnd_) noexcept : hWnd(hWnd_) {
		::BeginPaint(hWnd, &ps);
	}
	~Painter() {
		::EndPaint(hWnd, &ps);
	}
};

class MouseWheelDelta {
	int wheelDelta = 0;
public:
	bool Accumulate(WPARAM wParam) noexcept {
		wheelDelta -= GET_WHEEL_DELTA_WPARAM(wParam);
		return std::abs(wheelDelta) >= WHEEL_DELTA;
	}
	int Actions() noexcept {
		const int actions = wheelDelta / WHEEL_DELTA;
		wheelDelta = wheelDelta % WHEEL_DELTA;
		return actions;
	}
};

// Both GDI and DirectWrite can produce a HFONT for use in list boxes
struct FontWin : public Font {
	[[nodiscard]] virtual HFONT HFont() const noexcept = 0;
	[[nodiscard]] virtual std::unique_ptr<FontWin> Duplicate() const = 0;
	[[nodiscard]] virtual CharacterSet GetCharacterSet() const noexcept = 0;
};

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
			delete[]buffer;
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

// Manage the lifetime of a memory HBITMAP and its HDC so there are no leaks.
class GDIBitMap {
	HDC hdc{};
	HBITMAP hbm{};
	HBITMAP hbmOriginal{};

public:
	GDIBitMap() noexcept = default;
	// Deleted so GDIBitMap objects can not be copied.
	GDIBitMap(const GDIBitMap &) = delete;
	GDIBitMap(GDIBitMap &&) = delete;
	// Move would be OK but not needed yet
	GDIBitMap &operator=(const GDIBitMap &) = delete;
	GDIBitMap &operator=(GDIBitMap &&) = delete;
	~GDIBitMap() noexcept;

	void Create(HDC hdcBase, int width, int height, DWORD **pixels) noexcept;
	void Release() noexcept;
	HBITMAP Extract() noexcept;

	[[nodiscard]] HDC DC() const noexcept {
		return hdc;
	}
	[[nodiscard]] explicit operator bool() const noexcept {
		return hdc && hbm;
	}
};

}

#endif
