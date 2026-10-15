// This file is part of Notepad++ project
// Copyright (c) 2026 LucaSorvillo and Notepad++ team

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// How it works:
// - the scrollbars of the editors are standard window scrollbars. comctl32 draws them with the "ScrollBar"
//   theme, through two functions it imports from uxtheme.dll: OpenNcThemeData (a theme handle for each window)
//   and DrawThemeBackground (arrows, tracks, thumb, gripper);
// - like FixDarkScrollBar() does for the dark scrollbars (DarkMode/DarkMode.cpp), these two imports of comctl32
//   are replaced:
//   - OpenNcThemeData: the two editor windows get their own handle of the scrollbar theme, opened for a DPI that
//     nobody else uses (their DPI + 1), so that their drawing can be told apart from the other scrollbars;
//   - DrawThemeBackground: the parts drawn with such a handle are drawn by the original function into a bitmap,
//     then recoloured, so they keep the native look (shape, arrows, rounded ends, thumb growing under the mouse)
//     with the colours of the option. Everything else goes to the original functions;
// - the corner between the two scrollbars is not a theme part: it is painted after WM_NCPAINT, by a subclass of
//   the editor windows;
// - the native behaviour (dragging, page up/down, "Scroll Here" menu, hover states) is not touched;
// - if the imports are not found (another version of Windows), the scrollbars keep their native colours.

#include "EditorScrollbars.h"

#include <commctrl.h>
#include <uxtheme.h>
#include <vsstyle.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include "NppDarkMode.h"
#include "Parameters.h"
#include "dpiManagerV2.h"

namespace
{
	using fnOpenNcThemeData = HTHEME(WINAPI*)(HWND hwnd, LPCWSTR classList);
	using fnOpenThemeDataForDpi = HTHEME(WINAPI*)(HWND hwnd, LPCWSTR classList, UINT dpi);
	using fnDrawThemeBackground = HRESULT(WINAPI*)(HTHEME theme, HDC hdc, int part, int state, LPCRECT rc, LPCRECT clip);

	constexpr WORD ordinalOpenNcThemeData = 49; // exported by uxtheme.dll by ordinal only
	constexpr wchar_t scrollBarClass[] = L"ScrollBar";
	constexpr wchar_t darkScrollBarClass[] = L"DarkMode_Explorer::ScrollBar";
	constexpr UINT_PTR editorSubclassID = 1;

	bool g_installTried = false;
	std::atomic<bool> g_active{ false };

	HMODULE g_comctl32 = nullptr;
	IMAGE_THUNK_DATA* g_openNcSlot = nullptr;
	fnOpenNcThemeData g_realOpenNc = nullptr;
	fnOpenNcThemeData g_nextOpenNc = nullptr;
	fnDrawThemeBackground g_nextDraw = nullptr;
	fnOpenThemeDataForDpi g_openThemeDataForDpi = nullptr;

	std::mutex g_themesMutex;
	std::vector<HTHEME> g_editorThemes; // handles given to the editor windows; a reference to each one is kept

	struct Rgb
	{
		double r = 0;
		double g = 0;
		double b = 0;
	};

	// Native track and thumb colours of an editor theme handle, read once from its own drawing (per orientation)
	struct NativeColours
	{
		Rgb track;
		Rgb thumb;
	};

	std::mutex g_nativeMutex;
	std::map<std::pair<HTHEME, bool>, NativeColours> g_nativeColours;

	// --- delay-load imports of comctl32 (like FindDelayLoadThunkInModule() of DarkMode/IatHook.h) ---

	template <typename T>
	T fromRva(HMODULE module, ULONGLONG rva)
	{
		return reinterpret_cast<T>(reinterpret_cast<BYTE*>(module) + rva);
	}

	// Slot of the delay-load import table of "module" for dllName!funcName (by ordinal when funcName is null)
	IMAGE_THUNK_DATA* findDelayImport(HMODULE module, const char* dllName, const char* funcName, WORD ordinal)
	{
		const auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
		const auto ntHeaders = fromRva<IMAGE_NT_HEADERS*>(module, dosHeader->e_lfanew);
		const IMAGE_DATA_DIRECTORY& dir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
		if (dir.VirtualAddress == 0)
			return nullptr;

		for (auto imp = fromRva<IMAGE_DELAYLOAD_DESCRIPTOR*>(module, dir.VirtualAddress); imp->DllNameRVA; ++imp)
		{
			if (_stricmp(fromRva<const char*>(module, imp->DllNameRVA), dllName) != 0)
				continue;

			auto names = fromRva<IMAGE_THUNK_DATA*>(module, imp->ImportNameTableRVA);
			auto slots = fromRva<IMAGE_THUNK_DATA*>(module, imp->ImportAddressTableRVA);
			for (; names->u1.Ordinal; ++names, ++slots)
			{
				if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
				{
					if (!funcName && IMAGE_ORDINAL(names->u1.Ordinal) == ordinal)
						return slots;
				}
				else if (funcName)
				{
					const auto import = fromRva<IMAGE_IMPORT_BY_NAME*>(module, names->u1.AddressOfData);
					if (strcmp(reinterpret_cast<const char*>(import->Name), funcName) == 0)
						return slots;
				}
			}
		}
		return nullptr;
	}

	// The function a slot calls now, or "fallback" while the slot still points to the delay-load helper of comctl32
	// (import not resolved yet: the helper would overwrite the slot)
	template <typename F>
	F currentTarget(const IMAGE_THUNK_DATA* slot, F fallback)
	{
		const auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(g_comctl32);
		const auto ntHeaders = fromRva<IMAGE_NT_HEADERS*>(g_comctl32, dosHeader->e_lfanew);
		const auto begin = reinterpret_cast<ULONG_PTR>(g_comctl32);
		const ULONG_PTR end = begin + ntHeaders->OptionalHeader.SizeOfImage;
		const auto target = static_cast<ULONG_PTR>(slot->u1.Function);
		return (target >= begin && target < end) ? fallback : reinterpret_cast<F>(target);
	}

	void writeSlot(IMAGE_THUNK_DATA* slot, const void* function)
	{
		DWORD oldProtect = 0;
		if (::VirtualProtect(slot, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
		{
			slot->u1.Function = static_cast<decltype(slot->u1.Function)>(reinterpret_cast<ULONG_PTR>(function));
			::VirtualProtect(slot, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
		}
	}

	// --- editor windows and their theme handles ---

	// The two editors are the Scintilla windows that are direct children of the Notepad++ main window
	bool isEditorWindow(HWND hwnd)
	{
		wchar_t className[32]{};
		if (!hwnd || !::GetClassName(hwnd, className, _countof(className)) || wcscmp(className, L"Scintilla") != 0)
			return false;

		const HWND parent = ::GetParent(hwnd);
		if (!parent || parent != ::GetAncestor(hwnd, GA_ROOT))
			return false;

		return ::GetClassName(parent, className, _countof(className)) && wcscmp(className, L"Notepad++") == 0;
	}

	bool isEditorTheme(HTHEME theme)
	{
		std::lock_guard<std::mutex> lock(g_themesMutex);
		return std::find(g_editorThemes.begin(), g_editorThemes.end(), theme) != g_editorThemes.end();
	}

	// The handle stays listed with a reference of ours, so it cannot be freed and given to another theme
	void keepEditorTheme(HTHEME theme, LPCWSTR classList, UINT dpi)
	{
		std::lock_guard<std::mutex> lock(g_themesMutex);
		if (std::find(g_editorThemes.begin(), g_editorThemes.end(), theme) == g_editorThemes.end())
		{
			g_openThemeDataForDpi(nullptr, classList, dpi);
			g_editorThemes.push_back(theme);
		}
	}

	void releaseEditorThemes()
	{
		std::vector<HTHEME> themes;
		{
			std::lock_guard<std::mutex> lock(g_themesMutex);
			themes.swap(g_editorThemes);
		}
		for (HTHEME theme : themes)
			::CloseThemeData(theme);

		std::lock_guard<std::mutex> lock(g_nativeMutex); // the handles may be given to other themes from now on
		g_nativeColours.clear();
	}

	// --- painting: the drawing of Windows, with the colours of the option ---
	// Each part is drawn by Windows into a bitmap, then recoloured. Every pixel of a native scrollbar part is a mix
	// of the native track colour and of one foreground colour (thumb, arrow), with softened edges between them:
	// it becomes the same mix of the colours of the option.

	enum class Arrow { up, down, left, right }; // same order as the ABS_* state groups of SBP_ARROWBTN

	Rgb fromPixel(DWORD pixel) // a pixel of a 32-bit bitmap: 0x00RRGGBB
	{
		return { static_cast<double>((pixel >> 16) & 0xFF), static_cast<double>((pixel >> 8) & 0xFF), static_cast<double>(pixel & 0xFF) };
	}

	Rgb fromColour(COLORREF colour) // 0x00BBGGRR
	{
		return { static_cast<double>(GetRValue(colour)), static_cast<double>(GetGValue(colour)), static_cast<double>(GetBValue(colour)) };
	}

	Rgb minus(const Rgb& a, const Rgb& b)
	{
		return { a.r - b.r, a.g - b.g, a.b - b.b };
	}

	double dot(const Rgb& a, const Rgb& b)
	{
		return a.r * b.r + a.g * b.g + a.b * b.b;
	}

	// A 32-bit top-down bitmap selected in a memory DC
	class PartBitmap
	{
	public:
		PartBitmap(int width, int height)
		{
			BITMAPINFO info{};
			info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			info.bmiHeader.biWidth = width;
			info.bmiHeader.biHeight = -height;
			info.bmiHeader.biPlanes = 1;
			info.bmiHeader.biBitCount = 32;
			info.bmiHeader.biCompression = BI_RGB;
			void* bits = nullptr;
			_bitmap = ::CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
			_dc = ::CreateCompatibleDC(nullptr);
			if (_bitmap && _dc && bits)
			{
				_previous = ::SelectObject(_dc, _bitmap);
				_pixels = static_cast<DWORD*>(bits);
				_count = static_cast<size_t>(width) * static_cast<size_t>(height);
			}
		}

		~PartBitmap()
		{
			if (_previous)
				::SelectObject(_dc, _previous);
			if (_dc)
				::DeleteDC(_dc);
			if (_bitmap)
				::DeleteObject(_bitmap);
		}

		PartBitmap(const PartBitmap&) = delete;
		PartBitmap& operator=(const PartBitmap&) = delete;

		bool ok() const { return _pixels != nullptr; }
		HDC dc() const { return _dc; }
		DWORD* pixels() const { return _pixels; }
		size_t count() const { return _count; }

	private:
		HBITMAP _bitmap = nullptr;
		HDC _dc = nullptr;
		HGDIOBJ _previous = nullptr;
		DWORD* _pixels = nullptr;
		size_t _count = 0;
	};

	// Colour of the centre of a part drawn by Windows
	bool nativeCentre(HTHEME theme, int part, int state, bool vertical, Rgb& colour)
	{
		constexpr int across = 24;
		constexpr int along = 64;
		const int width = vertical ? across : along;
		const int height = vertical ? along : across;
		PartBitmap bitmap(width, height);
		const RECT rc{ 0, 0, width, height };
		if (!bitmap.ok() || FAILED(g_nextDraw(theme, bitmap.dc(), part, state, &rc, nullptr)))
			return false;
		::GdiFlush();
		colour = fromPixel(bitmap.pixels()[static_cast<size_t>(height / 2) * width + width / 2]);
		return true;
	}

	bool nativeColours(HTHEME theme, bool vertical, NativeColours& colours)
	{
		const auto key = std::make_pair(theme, vertical);
		{
			std::lock_guard<std::mutex> lock(g_nativeMutex);
			if (auto it = g_nativeColours.find(key); it != g_nativeColours.end())
			{
				colours = it->second;
				return true;
			}
		}

		// the thumb under the mouse is the widest: its centre has the thumb colour
		NativeColours read;
		if (!nativeCentre(theme, vertical ? SBP_LOWERTRACKVERT : SBP_LOWERTRACKHORZ, SCRBS_NORMAL, vertical, read.track)
			|| !nativeCentre(theme, vertical ? SBP_THUMBBTNVERT : SBP_THUMBBTNHORZ, SCRBS_HOT, vertical, read.thumb))
			return false;

		const Rgb difference = minus(read.thumb, read.track);
		constexpr double minDifference2 = 64;
		if (dot(difference, difference) < minDifference2)
			return false; // thumb and track look the same: nothing to recolour by

		std::lock_guard<std::mutex> lock(g_nativeMutex);
		g_nativeColours[key] = read;
		colours = read;
		return true;
	}

	// Every pixel keeps its mix of the native track colour and of the foreground colour of the part, with the new
	// colours. The foreground is the pixel farthest from the track towards the thumb colour: the thumb itself, an
	// arrow... Parts without one (tracks, arrows hidden while the mouse is away) only have track pixels. Pixels on
	// the other side of the track (the light edge line of the light theme) keep their difference too.
	void recolour(const PartBitmap& bitmap, const NativeColours& native, COLORREF newTrack, COLORREF newForeground)
	{
		DWORD* const pixels = bitmap.pixels();
		const Rgb towardsThumb = minus(native.thumb, native.track);
		const double thumbLength2 = dot(towardsThumb, towardsThumb);

		Rgb foreground = native.thumb;
		double farthest = 0.25; // less than a quarter of the way to the thumb colour: no foreground
		for (size_t i = 0; i < bitmap.count(); ++i)
		{
			const Rgb pixel = fromPixel(pixels[i]);
			const double towards = dot(minus(pixel, native.track), towardsThumb) / thumbLength2;
			if (towards > farthest)
			{
				farthest = towards;
				foreground = pixel;
			}
		}

		const Rgb direction = minus(foreground, native.track);
		const double length2 = std::max(1.0, dot(direction, direction));
		const Rgb track = fromColour(newTrack);
		const Rgb change = minus(fromColour(newForeground), track);
		const auto channel = [](double value) { return static_cast<DWORD>(std::clamp(std::lround(value), 0L, 255L)); };
		for (size_t i = 0; i < bitmap.count(); ++i)
		{
			const double share = dot(minus(fromPixel(pixels[i]), native.track), direction) / length2;
			pixels[i] = (channel(track.r + share * change.r) << 16) | (channel(track.g + share * change.g) << 8) | channel(track.b + share * change.b);
		}
	}

	void fillRect(HDC hdc, const RECT& rc, COLORREF colour)
	{
		::SetDCBrushColor(hdc, colour);
		::FillRect(hdc, &rc, static_cast<HBRUSH>(::GetStockObject(DC_BRUSH)));
	}

	COLORREF mix(COLORREF a, COLORREF b)
	{
		return RGB((GetRValue(a) + GetRValue(b)) / 2, (GetGValue(a) + GetGValue(b)) / 2, (GetBValue(a) + GetBValue(b)) / 2);
	}

	const ScrollbarColours& currentColours()
	{
		const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
		return NppDarkMode::isEnabled() ? nppGui._editorScrollbarDark : nppGui._editorScrollbarLight;
	}

	// Paints one part of an editor scrollbar. False: not a scrollbar part, or it cannot be recoloured: the original
	// function draws it
	bool paintPart(HTHEME theme, HDC hdc, int part, int state, const RECT& rc, LPCRECT clip)
	{
		bool vertical = true;
		Arrow arrow = Arrow::up;
		enum class Look { normal, hot, pressed, disabled } look = Look::normal;

		switch (part)
		{
			case SBP_ARROWBTN:
			{
				if (state >= ABS_UPHOVER && state <= ABS_RIGHTHOVER) // the scrollbar is hovered, not the arrow
				{
					arrow = static_cast<Arrow>(state - ABS_UPHOVER);
				}
				else if (state >= ABS_UPNORMAL && state <= ABS_RIGHTDISABLED)
				{
					arrow = static_cast<Arrow>((state - ABS_UPNORMAL) / 4);
					look = static_cast<Look>((state - ABS_UPNORMAL) % 4);
				}
				else
				{
					return false;
				}
				vertical = arrow == Arrow::up || arrow == Arrow::down;
				break;
			}

			case SBP_THUMBBTNHORZ:
			case SBP_LOWERTRACKHORZ:
			case SBP_UPPERTRACKHORZ:
			case SBP_GRIPPERHORZ:
				vertical = false;
				break;

			case SBP_THUMBBTNVERT:
			case SBP_LOWERTRACKVERT:
			case SBP_UPPERTRACKVERT:
			case SBP_GRIPPERVERT:
			case SBP_SIZEBOX:
			case SBP_SIZEBOXBKGND:
				break;

			default:
				return false;
		}

		if (part == SBP_GRIPPERHORZ || part == SBP_GRIPPERVERT)
			return true; // drawn over the thumb, it could not be recoloured: none (Windows 11 has none)

		const ScrollbarColours& colours = currentColours();
		const int savedDC = ::SaveDC(hdc);
		if (clip)
			::IntersectClipRect(hdc, clip->left, clip->top, clip->right, clip->bottom);

		bool painted = true;
		if (part == SBP_SIZEBOX || part == SBP_SIZEBOXBKGND)
		{
			fillRect(hdc, rc, colours._track); // like the corner between the scrollbars
		}
		else
		{
			// arrows, thumb and tracks: the drawing of Windows, recoloured
			const bool isThumbUnderMouse = (part == SBP_THUMBBTNVERT || part == SBP_THUMBBTNHORZ) && (state == SCRBS_HOT || state == SCRBS_PRESSED);
			COLORREF foreground = colours._thumb;
			if (look == Look::hot || look == Look::pressed || isThumbUnderMouse)
				foreground = colours._thumbHover;
			else if (look == Look::disabled)
				foreground = mix(colours._thumb, colours._track);

			const int width = rc.right - rc.left;
			const int height = rc.bottom - rc.top;
			NativeColours native;
			if (width > 0 && height > 0 && nativeColours(theme, vertical, native))
			{
				PartBitmap bitmap(width, height);
				const RECT local{ 0, 0, width, height };
				painted = bitmap.ok() && SUCCEEDED(g_nextDraw(theme, bitmap.dc(), part, state, &local, nullptr));
				if (painted)
				{
					::GdiFlush();
					recolour(bitmap, native, colours._track, foreground);
					::BitBlt(hdc, rc.left, rc.top, width, height, bitmap.dc(), 0, 0, SRCCOPY);
				}
			}
			else
			{
				painted = width <= 0 || height <= 0; // empty part: nothing to draw; no native colours: the original
			}
		}

		::RestoreDC(hdc, savedDC);
		return painted;
	}

	// The square between the two scrollbars, in the track colour
	void paintCorner(HWND hwnd)
	{
		SCROLLBARINFO vertical{ sizeof(SCROLLBARINFO) };
		SCROLLBARINFO horizontal{ sizeof(SCROLLBARINFO) };
		if (!::GetScrollBarInfo(hwnd, OBJID_VSCROLL, &vertical) || !::GetScrollBarInfo(hwnd, OBJID_HSCROLL, &horizontal))
			return;
		if ((vertical.rgstate[0] | horizontal.rgstate[0]) & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN))
			return; // the corner exists only when both scrollbars are shown

		RECT window{};
		::GetWindowRect(hwnd, &window);
		RECT corner{ vertical.rcScrollBar.left, horizontal.rcScrollBar.top, vertical.rcScrollBar.right, horizontal.rcScrollBar.bottom };
		if (::GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) // mirrored window DC
			corner = { window.right - corner.right, corner.top - window.top, window.right - corner.left, corner.bottom - window.top };
		else
			::OffsetRect(&corner, -window.left, -window.top);
		if (::IsRectEmpty(&corner))
			return;

		if (HDC hdc = ::GetWindowDC(hwnd))
		{
			fillRect(hdc, corner, currentColours()._track);
			::ReleaseDC(hwnd, hdc);
		}
	}

	LRESULT CALLBACK editorSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass, DWORD_PTR /*refData*/)
	{
		if (message == WM_NCDESTROY)
		{
			::RemoveWindowSubclass(hwnd, editorSubclassProc, idSubclass);
			return ::DefSubclassProc(hwnd, message, wParam, lParam);
		}

		const LRESULT result = ::DefSubclassProc(hwnd, message, wParam, lParam);
		if (message == WM_NCPAINT && g_active)
			paintCorner(hwnd);
		return result;
	}

	// --- hooks ---

	HTHEME WINAPI openNcThemeDataHook(HWND hwnd, LPCWSTR classList)
	{
		if (g_active && classList && _wcsicmp(classList, scrollBarClass) == 0 && isEditorWindow(hwnd))
		{
			::SetWindowSubclass(hwnd, editorSubclassProc, editorSubclassID, 0); // set once: same procedure and ID

			// the theme of the original scrollbar (the dark one in dark mode, like FixDarkScrollBar), whose drawing is
			// recoloured
			const LPCWSTR themeClass = NppDarkMode::isEnabled() ? darkScrollBarClass : scrollBarClass;
			const UINT dpi = DPIManagerV2::getDpiForWindow(hwnd) + 1;
			if (HTHEME theme = g_openThemeDataForDpi(nullptr, themeClass, dpi))
			{
				keepEditorTheme(theme, themeClass, dpi);
				return theme;
			}
		}
		return g_nextOpenNc(hwnd, classList);
	}

	HRESULT WINAPI drawThemeBackgroundHook(HTHEME theme, HDC hdc, int part, int state, LPCRECT rc, LPCRECT clip)
	{
		if (g_active && rc && isEditorTheme(theme) && paintPart(theme, hdc, part, state, *rc, clip))
			return S_OK;
		return g_nextDraw(theme, hdc, part, state, rc, clip);
	}

	// Replaces the two imports of comctl32, once; false if they cannot be found
	bool install()
	{
		if (g_installTried)
			return g_openNcSlot != nullptr;
		g_installTried = true;

		const HMODULE uxtheme = ::GetModuleHandle(L"uxtheme.dll");
		if (!uxtheme)
			return false;

		g_openThemeDataForDpi = reinterpret_cast<fnOpenThemeDataForDpi>(::GetProcAddress(uxtheme, "OpenThemeDataForDpi"));
		g_realOpenNc = reinterpret_cast<fnOpenNcThemeData>(::GetProcAddress(uxtheme, MAKEINTRESOURCEA(ordinalOpenNcThemeData)));
		const auto realDraw = reinterpret_cast<fnDrawThemeBackground>(::GetProcAddress(uxtheme, "DrawThemeBackground"));
		if (!g_openThemeDataForDpi || !g_realOpenNc || !realDraw)
			return false;

		// the same comctl32 as FixDarkScrollBar(): the one that draws the window scrollbars
		g_comctl32 = ::LoadLibraryEx(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!g_comctl32)
			return false;

		IMAGE_THUNK_DATA* openNcSlot = findDelayImport(g_comctl32, "uxtheme.dll", nullptr, ordinalOpenNcThemeData);
		IMAGE_THUNK_DATA* drawSlot = findDelayImport(g_comctl32, "uxtheme.dll", "DrawThemeBackground", 0);
		if (!openNcSlot || !drawSlot)
			return false;

		g_nextDraw = currentTarget(drawSlot, realDraw);
		g_nextOpenNc = currentTarget(openNcSlot, g_realOpenNc); // FixDarkScrollBar's function stays in the chain
		writeSlot(drawSlot, reinterpret_cast<const void*>(&drawThemeBackgroundHook));
		writeSlot(openNcSlot, reinterpret_cast<const void*>(&openNcThemeDataHook));
		g_openNcSlot = openNcSlot;
		return true;
	}
}

namespace EditorScrollbars
{
	void init()
	{
		if (NppParameters::getInstance().getNppGUI()._editorScrollbarCustomColours && install())
			g_active = true;
	}

	void apply(HWND mainEditor, HWND subEditor)
	{
		const bool isOn = NppParameters::getInstance().getNppGUI()._editorScrollbarCustomColours;
		if (isOn && !install())
			return; // the imports of this version of Windows are not the expected ones: native colours
		if (!g_openNcSlot)
			return; // never switched on: nothing to undo

		const bool isSwitched = g_active.exchange(isOn) != isOn;
		for (HWND editor : { mainEditor, subEditor })
		{
			if (!editor)
				continue;
			if (isSwitched)
				NppDarkMode::setDarkScrollBar(editor); // SetWindowTheme(): comctl32 opens the scrollbar theme again
			::RedrawWindow(editor, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
		}

		if (!isOn)
			releaseEditorThemes();
	}

	void onDarkModeChanging()
	{
		if (!g_openNcSlot)
			return;

		if (reinterpret_cast<const void*>(g_openNcSlot->u1.Function) != reinterpret_cast<const void*>(&openNcThemeDataHook))
		{
			g_nextOpenNc = currentTarget(g_openNcSlot, g_realOpenNc);
			writeSlot(g_openNcSlot, reinterpret_cast<const void*>(&openNcThemeDataHook));
		}
	}
}
