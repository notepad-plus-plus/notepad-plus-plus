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

// "Smooth scrolling (by pixels)" (Preferences > Editing 1). ScintillaEditView::ScintillaProc passes the mouse wheel
// messages of the editors, after the original code has chosen the view to scroll ("advanced scrolling" or not).
//
// Scintilla scrolls by whole lines (its first visible line is a line number) and is not changed. The part of a
// line is shown by moving the editor window:
// - the "offset" is the number of pixels of the first visible line hidden above the visible area: the window is
//   moved up by the offset and made taller by the same amount (its bottom, with the horizontal scrollbar, does
//   not move), and a window region clips it to its normal place;
// - when the offset reaches a line height, Scintilla scrolls by one line and the offset starts again from zero;
// - the place given to the editor by the Notepad++ layout is the "base" (WM_WINDOWPOSCHANGING in a subclass of
//   the editor): the offset is applied on top of it, so a layout change keeps the scroll position;
// - two windows of ours stay in the normal place, above the editor (which gets WS_CLIPSIBLINGS, so it does not
//   paint over them): its top border, painted like the original one, and its vertical scrollbar, which is the
//   window scrollbar of a window as narrow as the bar (same theme and dark mode as the original). The scrollbar
//   of Scintilla moves with the window, below ours: it is never seen;
// - the frames follow the screen refresh: a worker thread waits for each composition of the screen (DwmFlush)
//   and posts a message to the editors being animated, which repaint at once;
// - mouse wheel notches and scrollbar clicks are animated over the duration of the option; touchpads (deltas
//   smaller than a notch) are followed at the next frame, their movement is already smooth; the thumb of the
//   scrollbar moves the view at once;
// - anything else that scrolls the view (keyboard, search, another document, zoom) brings the view back to
//   whole lines: it is checked before and after each paint;
// - horizontally Scintilla already scrolls by pixels (SCI_SETXOFFSET): the horizontal wheel, Shift + wheel and the
//   arrows and track of the horizontal scrollbar are animated the same way, with the native distances.
// Only the two editor views: the other Scintilla windows (search results...) keep the native wheel.
#include "SmoothScrolling.h"

#include <commctrl.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cwchar>
#include <initializer_list>
#include <iterator>
#include <map>

#include "NppConstants.h"
#include "NppDarkMode.h"
#include "Parameters.h"
#include "Scintilla.h"
#include "resource.h"

namespace
{
	constexpr wchar_t overlayClassName[] = L"NppSmoothScrolling"; // our windows above the editors
	constexpr UINT_PTR subclassID = 1;
	constexpr UINT msgFrame = WM_APP + 0x632; // posted to an editor by the frame thread

	struct Editor
	{
		RECT base{};                 // place given by the Notepad++ layout (parent client coordinates)
		int offset = 0;              // pixels of the first visible line hidden above the visible area
		int lineHeight = 1;
		bool ourMove = false;        // our own SetWindowPos in progress
		bool animating = false;
		double from = 0;             // animation: positions in pixels (first visible line * line height + offset)
		double target = 0;
		double start = 0;            // ms
		double duration = 0;         // ms; 0: the target is shown at the next frame
		double exact = 0;            // position last shown, before rounding: the fractions of small deltas add up
		intptr_t line = 0;           // first visible line and document after our last change: other values mean
		intptr_t document = 0;       // that something else has scrolled the view
		HWND edge = nullptr;         // our top border and vertical scrollbar, in the normal place
		HWND scrollbar = nullptr;
		bool addedClipSiblings = false;
		SCROLLINFO shown{};          // last values given to our scrollbar

		// horizontal: Scintilla already scrolls by pixels (SCI_SETXOFFSET), only the steps are animated
		bool xAnimating = false;
		double xFrom = 0;
		double xTarget = 0;
		double xStart = 0;
		double xDuration = 0;
		double xExact = 0;
		int xShown = 0;              // horizontal offset after our last change: another value means something else scrolled
	};

	std::map<HWND, Editor> g_editors;

	intptr_t sci(HWND hwnd, UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
	{
		return static_cast<intptr_t>(::SendMessage(hwnd, message, wParam, lParam));
	}

	int lineHeightOf(HWND hwnd)
	{
		return std::max(1, static_cast<int>(sci(hwnd, SCI_TEXTHEIGHT, 0)));
	}

	double nowMs()
	{
		static const double ticksPerMs = []
		{
			LARGE_INTEGER frequency{};
			::QueryPerformanceFrequency(&frequency);
			return static_cast<double>(frequency.QuadPart) / 1000.0;
		}();
		LARGE_INTEGER counter{};
		::QueryPerformanceCounter(&counter);
		return static_cast<double>(counter.QuadPart) / ticksPerMs;
	}

	// 0..1 -> 0..1: fast at first, slow at the end
	double easeOut(double t)
	{
		const double u = 1.0 - t;
		return 1.0 - u * u * u;
	}

	bool isActive()
	{
		return NppParameters::getInstance().getSVP()._smoothScrolling;
	}

	// The two editor views: the Scintilla windows that are direct children of the Notepad++ main window
	bool isEditorWindow(HWND hwnd)
	{
		wchar_t className[16]{};
		if (!hwnd || !::GetClassName(hwnd, className, _countof(className)) || wcscmp(className, L"Scintilla") != 0)
			return false;

		const HWND parent = ::GetParent(hwnd);
		if (!parent || parent != ::GetAncestor(hwnd, GA_ROOT))
			return false;

		return ::GetClassName(parent, className, _countof(className)) && wcscmp(className, L"Notepad++") == 0;
	}

	// --- frame clock ---
	// A worker thread waits for each composition of the screen (DwmFlush) and posts msgFrame to the editors being
	// animated, one message at a time per editor. Plain globals and a slim lock: nothing is destroyed at exit
	// while the thread may still be running.

	SRWLOCK g_frameLock = SRWLOCK_INIT;
	HWND g_frameWindows[4]{};
	bool g_framePending[4]{};
	bool g_frameThreadRunning = false;

	using DwmFlushFn = HRESULT(WINAPI*)();

	DWORD WINAPI frameThread(LPVOID /*parameter*/)
	{
		static const DwmFlushFn dwmFlush = []
		{
			HMODULE dwmapi = ::LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			return dwmapi ? reinterpret_cast<DwmFlushFn>(::GetProcAddress(dwmapi, "DwmFlush")) : nullptr;
		}();

		for (;;)
		{
			if (!dwmFlush || FAILED(dwmFlush()))
				::Sleep(10);

			bool animating = false;
			::AcquireSRWLockExclusive(&g_frameLock);
			for (size_t i = 0; i < std::size(g_frameWindows); ++i)
			{
				if (!g_frameWindows[i])
					continue;
				animating = true;
				if (!g_framePending[i])
					g_framePending[i] = ::PostMessage(g_frameWindows[i], msgFrame, 0, 0) != FALSE;
			}
			if (!animating)
				g_frameThreadRunning = false;
			::ReleaseSRWLockExclusive(&g_frameLock);

			if (!animating)
				return 0;
		}
	}

	void stopFrames(HWND hwnd)
	{
		::AcquireSRWLockExclusive(&g_frameLock);
		for (size_t i = 0; i < std::size(g_frameWindows); ++i)
		{
			if (g_frameWindows[i] == hwnd)
			{
				g_frameWindows[i] = nullptr;
				g_framePending[i] = false;
			}
		}
		::ReleaseSRWLockExclusive(&g_frameLock);
	}

	// Returns false when the frames cannot start (the caller then shows the target at once)
	bool startFrames(HWND hwnd)
	{
		bool added = false;
		bool startThread = false;
		::AcquireSRWLockExclusive(&g_frameLock);
		for (size_t i = 0; i < std::size(g_frameWindows) && !added; ++i)
			added = g_frameWindows[i] == hwnd;
		for (size_t i = 0; i < std::size(g_frameWindows) && !added; ++i)
		{
			if (!g_frameWindows[i])
			{
				g_frameWindows[i] = hwnd;
				g_framePending[i] = false;
				added = true;
			}
		}
		if (added && !g_frameThreadRunning)
		{
			g_frameThreadRunning = true;
			startThread = true;
		}
		::ReleaseSRWLockExclusive(&g_frameLock);

		if (startThread)
		{
			if (HANDLE thread = ::CreateThread(nullptr, 0, frameThread, nullptr, 0, nullptr))
			{
				::CloseHandle(thread);
			}
			else
			{
				stopFrames(hwnd);
				::AcquireSRWLockExclusive(&g_frameLock);
				g_frameThreadRunning = false;
				::ReleaseSRWLockExclusive(&g_frameLock);
				added = false;
			}
		}
		return added;
	}

	void frameHandled(HWND hwnd)
	{
		::AcquireSRWLockExclusive(&g_frameLock);
		for (size_t i = 0; i < std::size(g_frameWindows); ++i)
		{
			if (g_frameWindows[i] == hwnd)
				g_framePending[i] = false;
		}
		::ReleaseSRWLockExclusive(&g_frameLock);
	}

	// --- geometry ---

	RECT rectInParent(HWND hwnd)
	{
		RECT rc{};
		::GetWindowRect(hwnd, &rc);
		::MapWindowPoints(HWND_DESKTOP, ::GetParent(hwnd), reinterpret_cast<POINT*>(&rc), 2);
		return rc;
	}

	// The window is moved up by the offset and made taller by the same amount (its bottom does not move), and a
	// window region keeps it in its place. Nothing is repainted here: the caller repaints.
	void setOffset(HWND hwnd, Editor& ed, int offset)
	{
		const int width = ed.base.right - ed.base.left;
		const int height = ed.base.bottom - ed.base.top;
		ed.offset = offset;
		ed.ourMove = true;
		::SetWindowRgn(hwnd, offset ? ::CreateRectRgn(0, offset, width, offset + height) : nullptr, FALSE);
		::SetWindowPos(hwnd, nullptr, ed.base.left, ed.base.top - offset, width, height + offset,
			SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW);
		ed.ourMove = false;
	}

	void invalidateAll(HWND hwnd)
	{
		::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);
	}

	// Scroll position in pixels shown now
	double shownPosition(const Editor& ed)
	{
		return static_cast<double>(ed.line) * ed.lineHeight + ed.offset;
	}

	// Height of the text area that is seen (the hidden part of the window excluded)
	int visibleHeight(HWND hwnd, const Editor& ed)
	{
		RECT client{};
		::GetClientRect(hwnd, &client);
		return std::max(1, static_cast<int>(client.bottom) - ed.offset);
	}

	// Highest scroll position: the end of the last line at the bottom of the visible area, or the last line at the
	// top with "scrolling beyond the last line"
	double maxPosition(HWND hwnd, const Editor& ed)
	{
		const auto displayLines = static_cast<double>(sci(hwnd, SCI_VISIBLEFROMDOCLINE, sci(hwnd, SCI_GETLINECOUNT)));
		const double lineHeight = ed.lineHeight;
		const double last = sci(hwnd, SCI_GETENDATLASTLINE) ? displayLines * lineHeight - visibleHeight(hwnd, ed) : (displayLines - 1) * lineHeight;
		return std::max(0.0, last);
	}

	// Something else has scrolled the view (keyboard, scrollbar, search, another document, zoom)
	bool scrolledByOthers(HWND hwnd, const Editor& ed)
	{
		return sci(hwnd, SCI_GETFIRSTVISIBLELINE) != ed.line || sci(hwnd, SCI_GETDOCPOINTER) != ed.document
			|| lineHeightOf(hwnd) != ed.lineHeight;
	}

	// Back to whole lines: the animation stops and the window goes back to its place (painted by the next WM_PAINT).
	// The frames stop by themselves when nothing is animated any more.
	void resync(HWND hwnd, Editor& ed)
	{
		ed.animating = false;
		if (ed.offset)
		{
			setOffset(hwnd, ed, 0);
			invalidateAll(hwnd);
		}
		ed.lineHeight = lineHeightOf(hwnd);
		ed.line = sci(hwnd, SCI_GETFIRSTVISIBLELINE);
		ed.document = sci(hwnd, SCI_GETDOCPOINTER);
		ed.exact = shownPosition(ed);
	}

	// Shows a scroll position (first visible line of Scintilla + offset of the window) and paints it at once.
	// Returns false when Scintilla has not scrolled as far as asked (top or end of the document).
	bool showPosition(HWND hwnd, Editor& ed, double position)
	{
		position = std::max(0.0, position);
		const auto pixels = static_cast<intptr_t>(std::llround(position));
		const intptr_t line = pixels / ed.lineHeight;
		const int offset = static_cast<int>(pixels % ed.lineHeight);
		ed.exact = position;
		if (line == ed.line && offset == ed.offset)
			return true;

		if (offset != ed.offset)
			setOffset(hwnd, ed, offset); // before the line: Scintilla checks the line against its new height
		if (line != ed.line)
			sci(hwnd, SCI_SETFIRSTVISIBLELINE, static_cast<WPARAM>(line));
		ed.line = sci(hwnd, SCI_GETFIRSTVISIBLELINE);
		::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
		if (ed.line == line)
			return true;
		ed.exact = shownPosition(ed); // top or end of the document
		return false;
	}

	// --- animation ---

	// Animates the view to a position (duration 0: shown at the next frame)
	void animateTo(HWND hwnd, Editor& ed, double target, double duration)
	{
		const double shown = shownPosition(ed);
		ed.from = shown;
		ed.target = std::clamp(target, 0.0, std::max(maxPosition(hwnd, ed), shown));
		ed.start = nowMs();
		ed.duration = duration;
		if (!ed.animating && ed.target != shown)
		{
			if (startFrames(hwnd))
				ed.animating = true;
			else
				showPosition(hwnd, ed, ed.target);
		}
	}

	// Moves the target of the animation; a step in the opposite direction drops what was left
	void scrollBy(HWND hwnd, Editor& ed, double pixels, double duration)
	{
		const double shown = shownPosition(ed);
		const double current = std::llround(ed.exact) == std::llround(shown) ? ed.exact : shown;
		double target = ed.animating ? ed.target : current;
		if ((target - current) * pixels < 0)
			target = current;
		animateTo(hwnd, ed, target + pixels, duration);
	}

	// The view follows the thumb of our scrollbar at once
	void jumpTo(HWND hwnd, Editor& ed, double position)
	{
		ed.animating = false;
		showPosition(hwnd, ed, std::clamp(position, 0.0, std::max(maxPosition(hwnd, ed), shownPosition(ed))));
	}

	void stepVertical(HWND hwnd, Editor& ed)
	{
		if (scrolledByOthers(hwnd, ed))
		{
			resync(hwnd, ed);
			return;
		}

		const double t = ed.duration > 0 ? std::min(1.0, (nowMs() - ed.start) / ed.duration) : 1.0;
		const bool reached = showPosition(hwnd, ed, ed.from + (ed.target - ed.from) * easeOut(t));
		if (t >= 1.0 || !reached)
			ed.animating = false;
	}

	// --- horizontal: Scintilla scrolls by pixels already, there is no window to move ---

	// Width of the text area, as Scintilla computes it (margins excluded)
	int textWidth(HWND hwnd)
	{
		RECT client{};
		::GetClientRect(hwnd, &client);
		intptr_t textStart = sci(hwnd, SCI_GETMARGINLEFT);
		const intptr_t margins = sci(hwnd, SCI_GETMARGINS);
		for (intptr_t margin = 0; margin < margins; ++margin)
			textStart += sci(hwnd, SCI_GETMARGINWIDTHN, static_cast<WPARAM>(margin));
		return static_cast<int>(client.right - textStart - sci(hwnd, SCI_GETMARGINRIGHT));
	}

	// Highest horizontal offset, the same as the wheel of Scintilla (HorizontalScrollToClamped)
	double maxXOffset(HWND hwnd)
	{
		const int pageWidth = textWidth(hwnd);
		const int documentWidth = std::max({ static_cast<int>(sci(hwnd, SCI_GETSCROLLWIDTH)), pageWidth - 1, 0 });
		return std::max(0, documentWidth - pageWidth + 1);
	}

	// Something else scrolled horizontally (caret, keyboard, scrollbar thumb, search): the animation stops
	void resyncX(HWND hwnd, Editor& ed)
	{
		ed.xAnimating = false;
		ed.xShown = static_cast<int>(sci(hwnd, SCI_GETXOFFSET));
		ed.xExact = ed.xShown;
	}

	// Sets the horizontal offset; Scintilla repaints at the next WM_PAINT
	void showX(HWND hwnd, Editor& ed, double x)
	{
		ed.xExact = std::max(0.0, x);
		const auto pixels = static_cast<int>(std::lround(ed.xExact));
		if (pixels != ed.xShown)
		{
			sci(hwnd, SCI_SETXOFFSET, static_cast<WPARAM>(pixels));
			ed.xShown = static_cast<int>(sci(hwnd, SCI_GETXOFFSET));
		}
	}

	// Moves the target of the horizontal animation (duration 0: shown at the next frame); a step in the opposite
	// direction drops what was left
	void scrollXBy(HWND hwnd, Editor& ed, double pixels, double duration)
	{
		if (sci(hwnd, SCI_GETXOFFSET) != ed.xShown)
			resyncX(hwnd, ed);

		const double shown = ed.xShown;
		const double current = std::lround(ed.xExact) == ed.xShown ? ed.xExact : shown;
		double target = ed.xAnimating ? ed.xTarget : current;
		if ((target - current) * pixels < 0)
			target = current;

		ed.xFrom = shown;
		ed.xTarget = std::clamp(target + pixels, 0.0, std::max(maxXOffset(hwnd), shown));
		ed.xStart = nowMs();
		ed.xDuration = duration;
		if (!ed.xAnimating && ed.xTarget != shown)
		{
			if (startFrames(hwnd))
			{
				ed.xAnimating = true;
			}
			else
			{
				showX(hwnd, ed, ed.xTarget);
				::UpdateWindow(hwnd);
			}
		}
	}

	void stepHorizontal(HWND hwnd, Editor& ed)
	{
		if (sci(hwnd, SCI_GETXOFFSET) != ed.xShown)
		{
			resyncX(hwnd, ed);
			return;
		}

		const double t = ed.xDuration > 0 ? std::min(1.0, (nowMs() - ed.xStart) / ed.xDuration) : 1.0;
		showX(hwnd, ed, ed.xFrom + (ed.xTarget - ed.xFrom) * easeOut(t));
		if (t >= 1.0)
			ed.xAnimating = false;
	}

	void onFrame(HWND hwnd, Editor& ed)
	{
		frameHandled(hwnd);
		if (ed.xAnimating)
			stepHorizontal(hwnd, ed); // first: the vertical step paints both
		if (ed.animating)
			stepVertical(hwnd, ed);
		::UpdateWindow(hwnd);         // a horizontal step alone
		if (!ed.animating && !ed.xAnimating)
			stopFrames(hwnd);
	}

	// --- our top border and vertical scrollbar ---

	void syncScrollbar(HWND hwnd, Editor& ed)
	{
		if (!ed.scrollbar)
			return;

		const double shown = shownPosition(ed);
		const int visible = visibleHeight(hwnd, ed);
		const double last = std::max(maxPosition(hwnd, ed), shown);
		SCROLLINFO si{};
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
		si.nMin = 0;
		si.nMax = static_cast<int>(std::min(last + visible - 1, static_cast<double>(INT_MAX)));
		si.nPage = static_cast<UINT>(visible);
		si.nPos = static_cast<int>(std::min<long long>(std::llround(shown), INT_MAX));
		if (si.nMax != ed.shown.nMax || si.nPage != ed.shown.nPage || si.nPos != ed.shown.nPos)
		{
			::SetScrollInfo(ed.scrollbar, SB_VERT, &si, TRUE);
			ed.shown = si;
		}
	}

	// The top border of the editor, as Windows paints it: the top of the frame of a taller rectangle
	void paintEdge(HWND edge, HWND editor, HDC hdc)
	{
		RECT rc{};
		::GetClientRect(edge, &rc);
		rc.bottom += 16;
		if (::GetWindowLongPtr(editor, GWL_EXSTYLE) & WS_EX_CLIENTEDGE)
			::DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT);
		else
			::FrameRect(hdc, &rc, ::GetSysColorBrush(COLOR_WINDOWFRAME)); // WS_BORDER (dark mode)
	}

	// Our windows go where the top border and the scrollbar of the editor are when it is not moved
	void layoutOverlays(HWND hwnd, Editor& ed)
	{
		// shown and hidden with the editor (its own flag: the main window may not be shown yet)
		const bool shown = (::GetWindowLongPtr(hwnd, GWL_STYLE) & WS_VISIBLE) != 0;
		constexpr UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER;

		if (ed.edge)
		{
			RECT window{};
			::GetWindowRect(hwnd, &window);
			POINT clientOrigin{};
			::ClientToScreen(hwnd, &clientOrigin);
			const int height = clientOrigin.y - window.top; // the same with any offset
			if (shown && height > 0)
				::SetWindowPos(ed.edge, HWND_TOP, ed.base.left, ed.base.top, ed.base.right - ed.base.left, height, flags | SWP_SHOWWINDOW);
			else
				::ShowWindow(ed.edge, SW_HIDE);
			::InvalidateRect(ed.edge, nullptr, FALSE);
		}

		if (ed.scrollbar)
		{
			SCROLLBARINFO info{};
			info.cbSize = sizeof(info);
			if (shown && ::GetScrollBarInfo(hwnd, OBJID_VSCROLL, &info) && !(info.rgstate[0] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN)))
			{
				RECT rc = info.rcScrollBar;
				rc.top += ed.offset;
				::MapWindowPoints(HWND_DESKTOP, ::GetParent(hwnd), reinterpret_cast<POINT*>(&rc), 2);
				::SetWindowPos(ed.scrollbar, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, flags | SWP_SHOWWINDOW);
			}
			else
			{
				::ShowWindow(ed.scrollbar, SW_HIDE);
			}
		}
	}

	// Dark mode switched: the theme of our scrollbar is opened again
	void refreshOverlays(HWND hwnd, Editor& ed)
	{
		if (ed.scrollbar)
		{
			NppDarkMode::setDarkScrollBar(ed.scrollbar);
			::RedrawWindow(ed.scrollbar, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);
		}
		layoutOverlays(hwnd, ed); // the border may have changed too (dark mode: WS_BORDER)
	}

	// --- windows ---

	LRESULT CALLBACK overlayProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK editorSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass, DWORD_PTR refData);

	void createOverlays(HWND hwnd, Editor& ed)
	{
		static const bool registered = []
		{
			WNDCLASSEXW wc{};
			wc.cbSize = sizeof(wc);
			wc.lpfnWndProc = overlayProc;
			wc.hInstance = ::GetModuleHandle(nullptr);
			wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			wc.lpszClassName = overlayClassName;
			return ::RegisterClassExW(&wc) != 0;
		}();
		if (!registered)
			return;

		const HWND parent = ::GetParent(hwnd);
		const HINSTANCE instance = ::GetModuleHandle(nullptr);
		ed.edge = ::CreateWindowExW(0, overlayClassName, nullptr, WS_CHILD | WS_CLIPSIBLINGS,
			0, 0, 0, 0, parent, nullptr, instance, nullptr);
		ed.scrollbar = ::CreateWindowExW(0, overlayClassName, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL,
			0, 0, 0, 0, parent, nullptr, instance, nullptr);
		for (HWND overlay : { ed.edge, ed.scrollbar })
		{
			if (overlay)
				::SetWindowLongPtr(overlay, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hwnd));
		}
		if (ed.scrollbar)
			NppDarkMode::setDarkScrollBar(ed.scrollbar);
		ed.shown = {};

		// the editor does not paint over them
		const LONG_PTR style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
		if (!(style & WS_CLIPSIBLINGS))
		{
			::SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS);
			ed.addedClipSiblings = true;
		}
	}

	void destroyOverlays(HWND hwnd, Editor& ed)
	{
		for (HWND* overlay : { &ed.edge, &ed.scrollbar })
		{
			if (*overlay)
				::DestroyWindow(*overlay);
			*overlay = nullptr;
		}
		if (ed.addedClipSiblings)
		{
			::SetWindowLongPtr(hwnd, GWL_STYLE, ::GetWindowLongPtr(hwnd, GWL_STYLE) & ~WS_CLIPSIBLINGS);
			ed.addedClipSiblings = false;
		}
	}

	Editor& attach(HWND hwnd)
	{
		auto [it, inserted] = g_editors.try_emplace(hwnd);
		Editor& ed = it->second;
		if (inserted)
		{
			::SetWindowSubclass(hwnd, editorSubclassProc, subclassID, 0);
			ed.base = rectInParent(hwnd);
			resync(hwnd, ed);
			resyncX(hwnd, ed);
			createOverlays(hwnd, ed);
			::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			layoutOverlays(hwnd, ed);
			syncScrollbar(hwnd, ed);
		}
		return ed;
	}

	// Back to the original window: whole lines, its own scrollbar and border
	void detach(HWND hwnd, Editor& ed)
	{
		ed.animating = false;
		stopFrames(hwnd);
		if (ed.offset)
			setOffset(hwnd, ed, 0);
		destroyOverlays(hwnd, ed);
		::RemoveWindowSubclass(hwnd, editorSubclassProc, subclassID);
		::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		invalidateAll(hwnd);
	}

	bool wheel(HWND hwnd, WPARAM wParam)
	{
		if (sci(hwnd, SCI_AUTOCACTIVE))
			return false; // Scintilla gives the wheel to the autocompletion list

		UINT linesPerNotch = 3;
		::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &linesPerNotch, 0);
		if (linesPerNotch == 0)
			return false; // wheel scrolling switched off in Windows

		Editor& ed = attach(hwnd);
		if (scrolledByOthers(hwnd, ed))
			resync(hwnd, ed);
		if (linesPerNotch == WHEEL_PAGESCROLL)
			linesPerNotch = static_cast<UINT>(std::max<intptr_t>(1, sci(hwnd, SCI_LINESONSCREEN) - 1));

		// the wheel turned towards the user (negative delta) scrolls down
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		const double pixels = -static_cast<double>(delta) * linesPerNotch * ed.lineHeight / WHEEL_DELTA;
		const double duration = (delta % WHEEL_DELTA) ? 0.0 : std::max(1, NppParameters::getInstance().getSVP()._smoothScrollingDuration);
		scrollBy(hwnd, ed, pixels, duration);
		return true;
	}

	// Horizontal wheel (tilt wheel, touchpad) and Shift + vertical wheel, by pixels, with the native distances:
	// characters of the Windows setting like Scintilla for the first, 3 x 20 pixels like Notepad++ for the second
	bool horizontalWheel(HWND hwnd, UINT message, WPARAM wParam)
	{
		if (sci(hwnd, SCI_GETWRAPMODE) != SC_WRAP_NONE || sci(hwnd, SCI_AUTOCACTIVE))
			return false; // native: nothing to scroll with word wrap; the wheel goes to the autocompletion list

		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		double pixels = 0;
		if (message == WM_MOUSEHWHEEL)
		{
			UINT charsPerNotch = 3;
			::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &charsPerNotch, 0);
			if (charsPerNotch == 0)
				return false; // horizontal wheel switched off in Windows
			const auto charWidth = static_cast<double>(std::max<intptr_t>(1, sci(hwnd, SCI_TEXTWIDTH, STYLE_DEFAULT, reinterpret_cast<LPARAM>("P"))));
			pixels = static_cast<double>(delta) * charsPerNotch * charWidth / WHEEL_DELTA; // tilted to the right: scrolls right
		}
		else
		{
			pixels = -static_cast<double>(delta) * 60 / WHEEL_DELTA; // turned towards the user: scrolls right
		}

		Editor& ed = attach(hwnd);
		const double duration = (delta % WHEEL_DELTA) ? 0.0 : std::max(1, NppParameters::getInstance().getSVP()._smoothScrollingDuration);
		scrollXBy(hwnd, ed, pixels, duration);
		return true;
	}

	// Arrows and track of the horizontal scrollbar of Scintilla: animated, with the native distances (20 pixels,
	// two thirds of the text width). The thumb stays native: it already scrolls by pixels.
	bool onHorizontalScrollbar(HWND hwnd, Editor& ed, WORD code)
	{
		if (sci(hwnd, SCI_GETWRAPMODE) != SC_WRAP_NONE)
			return false;

		const double arrow = 20;
		const double page = textWidth(hwnd) * 2 / 3;
		double pixels = 0;
		switch (code)
		{
			case SB_LINELEFT: pixels = -arrow; break;
			case SB_LINERIGHT: pixels = arrow; break;
			case SB_PAGELEFT: pixels = -page; break;
			case SB_PAGERIGHT: pixels = page; break;
			default: return false;
		}
		scrollXBy(hwnd, ed, pixels, std::max(1, NppParameters::getInstance().getSVP()._smoothScrollingDuration));
		return true;
	}

	// Our scrollbar: arrows and track scroll like the wheel (animated), the thumb at once
	void onScrollbar(HWND hwnd, Editor& ed, WORD code)
	{
		if (scrolledByOthers(hwnd, ed))
			resync(hwnd, ed);

		const double duration = std::max(1, NppParameters::getInstance().getSVP()._smoothScrollingDuration);
		const double lineHeight = ed.lineHeight;
		const double page = static_cast<double>(std::max<intptr_t>(1, sci(hwnd, SCI_LINESONSCREEN) - 1)) * lineHeight;
		switch (code)
		{
			case SB_LINEUP:
				scrollBy(hwnd, ed, -lineHeight, duration);
				break;

			case SB_LINEDOWN:
				scrollBy(hwnd, ed, lineHeight, duration);
				break;

			case SB_PAGEUP:
				scrollBy(hwnd, ed, -page, duration);
				break;

			case SB_PAGEDOWN:
				scrollBy(hwnd, ed, page, duration);
				break;

			case SB_TOP:
				animateTo(hwnd, ed, 0, duration);
				break;

			case SB_BOTTOM:
				animateTo(hwnd, ed, maxPosition(hwnd, ed), duration);
				break;

			case SB_THUMBTRACK:
			case SB_THUMBPOSITION:
			{
				SCROLLINFO si{};
				si.cbSize = sizeof(si);
				si.fMask = SIF_TRACKPOS;
				if (::GetScrollInfo(ed.scrollbar, SB_VERT, &si))
					jumpTo(hwnd, ed, si.nTrackPos);
				break;
			}

			default: // SB_ENDSCROLL
				break;
		}
		syncScrollbar(hwnd, ed);
	}

	// The wheel over our scrollbar ("advanced scrolling" sends it to the window under the mouse). Sending it back
	// to the editor would loop, Notepad++ would send it here again: DOCUMENTMAP_MOUSEWHEEL is the message Notepad++
	// itself uses to get the native wheel of Scintilla (zoom with Ctrl...).
	void onOverlayWheel(HWND editor, WPARAM wParam, LPARAM lParam)
	{
		const auto keys = GET_KEYSTATE_WPARAM(wParam);
		bool handled = false;
		if (keys & MK_RBUTTON)
		{
			::SendMessage(::GetParent(editor), WM_MOUSEWHEEL, wParam, lParam); // like the editor: switches the tabs
			handled = true;
		}
		else if (keys & MK_SHIFT)
		{
			handled = horizontalWheel(editor, WM_MOUSEWHEEL, wParam);
		}
		else if (!(keys & MK_CONTROL))
		{
			handled = wheel(editor, wParam);
		}

		if (!handled)
			::SendMessage(editor, DOCUMENTMAP_MOUSEWHEEL, wParam, lParam);
	}

	LRESULT CALLBACK overlayProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		const auto editor = reinterpret_cast<HWND>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		const auto it = g_editors.find(editor);
		Editor* ed = it != g_editors.end() ? &it->second : nullptr;

		switch (message)
		{
			case WM_VSCROLL:
			{
				if (ed)
					onScrollbar(editor, *ed, LOWORD(wParam));
				return 0;
			}

			case WM_MOUSEWHEEL:
			{
				if (ed)
					onOverlayWheel(editor, wParam, lParam);
				return 0;
			}

			case WM_MOUSEHWHEEL:
			{
				if (ed)
					horizontalWheel(editor, message, wParam);
				return TRUE; // like Scintilla (Logitech mice)
			}

			case WM_ERASEBKGND:
			{
				return 1;
			}

			case WM_PAINT:
			{
				PAINTSTRUCT ps{};
				const HDC hdc = ::BeginPaint(hwnd, &ps);
				if (ed && hwnd == ed->edge)
					paintEdge(hwnd, editor, hdc);
				::EndPaint(hwnd, &ps);
				return 0;
			}

			case WM_NCDESTROY:
			{
				if (ed && ed->edge == hwnd)
					ed->edge = nullptr;
				if (ed && ed->scrollbar == hwnd)
					ed->scrollbar = nullptr;
				break;
			}

			default:
				break;
		}
		return ::DefWindowProc(hwnd, message, wParam, lParam);
	}

	LRESULT CALLBACK editorSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass, DWORD_PTR /*refData*/)
	{
		if (message == WM_NCDESTROY)
		{
			stopFrames(hwnd);
			if (auto it = g_editors.find(hwnd); it != g_editors.end())
			{
				destroyOverlays(hwnd, it->second);
				g_editors.erase(it);
			}
			::RemoveWindowSubclass(hwnd, editorSubclassProc, idSubclass);
			return ::DefSubclassProc(hwnd, message, wParam, lParam);
		}

		auto it = g_editors.find(hwnd);
		if (it == g_editors.end())
			return ::DefSubclassProc(hwnd, message, wParam, lParam);
		Editor& ed = it->second;

		switch (message)
		{
			case msgFrame:
			{
				onFrame(hwnd, ed);
				return 0;
			}

			case WM_PAINT:
			{
				if (scrolledByOthers(hwnd, ed))
					resync(hwnd, ed);
				const LRESULT result = ::DefSubclassProc(hwnd, message, wParam, lParam);

				// Notepad++ may also scroll while Scintilla paints (position restored after SCN_PAINTED, e.g. when word
				// wrap is switched off), and Scintilla then paints again without a new WM_PAINT: checked once more
				auto again = g_editors.find(hwnd);
				if (again != g_editors.end())
				{
					if (scrolledByOthers(hwnd, again->second))
						resync(hwnd, again->second); // repainted by the next WM_PAINT
					syncScrollbar(hwnd, again->second);
				}
				return result;
			}

			case WM_WINDOWPOSCHANGING:
			{
				auto* pos = reinterpret_cast<WINDOWPOS*>(lParam);
				if (ed.ourMove || (pos->flags & (SWP_NOMOVE | SWP_NOSIZE)) == (SWP_NOMOVE | SWP_NOSIZE))
					break;

				// placed by the Notepad++ layout: that is the new base, and the offset stays
				const bool moves = !(pos->flags & SWP_NOMOVE);
				const bool sizes = !(pos->flags & SWP_NOSIZE);
				const int x = moves ? pos->x : ed.base.left;
				const int y = moves ? pos->y : ed.base.top;
				const int width = sizes ? pos->cx : ed.base.right - ed.base.left;
				const int height = sizes ? pos->cy : ed.base.bottom - ed.base.top;
				ed.base = { x, y, x + width, y + height };
				if (ed.offset)
				{
					pos->x = x;
					pos->y = y - ed.offset;
					pos->cx = width;
					pos->cy = height + ed.offset;
					pos->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
				}
				break;
			}

			case WM_WINDOWPOSCHANGED:
			{
				const auto* pos = reinterpret_cast<const WINDOWPOS*>(lParam);
				if (!ed.ourMove && ed.offset && !(pos->flags & SWP_NOSIZE)) // the region follows the new size
				{
					const int width = ed.base.right - ed.base.left;
					const int height = ed.base.bottom - ed.base.top;
					::SetWindowRgn(hwnd, ::CreateRectRgn(0, ed.offset, width, ed.offset + height), TRUE);
				}
				const LRESULT result = ::DefSubclassProc(hwnd, message, wParam, lParam); // WM_SIZE, WM_MOVE

				// moved, resized, shown, hidden, or its frame changed (border, scrollbars shown or hidden)
				auto again = g_editors.find(hwnd);
				if (again != g_editors.end() && !again->second.ourMove)
				{
					layoutOverlays(hwnd, again->second);
					syncScrollbar(hwnd, again->second);
				}
				return result;
			}

			case WM_HSCROLL:
			{
				if (!lParam && onHorizontalScrollbar(hwnd, ed, LOWORD(wParam))) // the window scrollbar of Scintilla
					return 0;
				break;
			}

			case NPPM_INTERNAL_REFRESHDARKMODE:
			{
				const LRESULT result = ::DefSubclassProc(hwnd, message, wParam, lParam); // the editor scrollbars first
				auto again = g_editors.find(hwnd);
				if (again != g_editors.end())
					refreshOverlays(hwnd, again->second);
				return result;
			}

			default:
				break;
		}
		return ::DefSubclassProc(hwnd, message, wParam, lParam);
	}
}

namespace SmoothScrolling
{
	bool onMouseWheel(HWND hwnd, WPARAM wParam)
	{
		if (!isActive() || !isEditorWindow(hwnd))
			return false;
		if (GET_KEYSTATE_WPARAM(wParam) & (MK_CONTROL | MK_SHIFT | MK_RBUTTON))
			return false; // zoom and the other wheel actions stay native
		return wheel(hwnd, wParam);
	}

	bool onHorizontalWheel(HWND hwnd, UINT message, WPARAM wParam)
	{
		if (message == WM_MOUSEWHEEL && (GET_KEYSTATE_WPARAM(wParam) & (MK_SHIFT | MK_RBUTTON)) != MK_SHIFT)
			return false; // vertical wheel, or right button + wheel (tab switching): handled further on
		if (!isActive() || !isEditorWindow(hwnd))
			return false;
		return horizontalWheel(hwnd, message, wParam);
	}

	void apply(HWND mainEditor, HWND subEditor)
	{
		if (!isActive())
		{
			for (auto& [hwnd, ed] : g_editors)
				detach(hwnd, ed);
			g_editors.clear();
			return;
		}

		for (HWND editor : { mainEditor, subEditor })
		{
			if (isEditorWindow(editor))
				attach(editor);
		}
	}
}
