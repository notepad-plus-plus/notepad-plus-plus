// Scintilla source code edit control
/** @file ScintillaWin.cxx
 ** Windows specific subclass of ScintillaBase.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <climits>

#include <stdexcept>
#include <new>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <memory>
#include <chrono>
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
#include <zmouse.h>
#include <ole2.h>

#if !defined(DISABLE_D2D)
#define USE_D2D 1
#endif

#if defined(USE_D2D)
#include <d2d1.h>
#include <dwrite.h>
#endif

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "CharacterCategoryMap.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "CaseConvert.h"
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "ElapsedPeriod.h"

#include "AutoComplete.h"
#include "ScintillaBase.h"

#include "WinTypes.h"
#include "PlatWin.h"
#include "HanjaDic.h"
#include "ScintillaWin.h"
#include "BoostRegexSearch.h"

#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES   104
#endif

#ifndef WM_UNICHAR
#define WM_UNICHAR                      0x0109
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

#ifndef UNICODE_NOCHAR
#define UNICODE_NOCHAR                  0xFFFF
#endif

#ifndef IS_HIGH_SURROGATE
#define IS_HIGH_SURROGATE(x)            ((x) >= SURROGATE_LEAD_FIRST && (x) <= SURROGATE_LEAD_LAST)
#endif

#ifndef IS_LOW_SURROGATE
#define IS_LOW_SURROGATE(x)             ((x) >= SURROGATE_TRAIL_FIRST && (x) <= SURROGATE_TRAIL_LAST)
#endif

#ifndef MK_ALT
#define MK_ALT 32
#endif

namespace {

// Two idle messages SC_WIN_IDLE and SC_WORK_IDLE.

// SC_WIN_IDLE is low priority so should occur after the next WM_PAINT
// It is for lengthy actions like wrapping and background styling
constexpr UINT SC_WIN_IDLE = 5001;
// SC_WORK_IDLE is high priority and should occur before the next WM_PAINT
// It is for shorter actions like restyling the text just inserted
// and delivering SCN_UPDATEUI
constexpr UINT SC_WORK_IDLE = 5002;

constexpr int IndicatorInput = static_cast<int>(Scintilla::IndicatorNumbers::Ime);
constexpr int IndicatorTarget = IndicatorInput + 1;
constexpr int IndicatorConverted = IndicatorInput + 2;
constexpr int IndicatorUnknown = IndicatorInput + 3;

typedef UINT_PTR (WINAPI *SetCoalescableTimerSig)(HWND hwnd, UINT_PTR nIDEvent,
	UINT uElapse, TIMERPROC lpTimerFunc, ULONG uToleranceDelay);

}

// GCC has trouble with the standard COM ABI so do it the old C way with explicit vtables.

using namespace Scintilla;
using namespace Scintilla::Internal;

namespace {

const TCHAR callClassName[] = TEXT("CallTip");

void SetWindowID(HWND hWnd, int identifier) noexcept {
	::SetWindowLongPtr(hWnd, GWLP_ID, identifier);
}

Point PointFromLParam(sptr_t lpoint) noexcept {
	return Point::FromInts(GET_X_LPARAM(lpoint), GET_Y_LPARAM(lpoint));
}

bool KeyboardIsKeyDown(int key) noexcept {
	return (::GetKeyState(key) & 0x80000000) != 0;
}

constexpr bool KeyboardIsNumericKeypadFunction(uptr_t wParam, sptr_t lParam) {
	// Bit 24 is the extended keyboard flag and the numeric keypad is non-extended
	if ((lParam & (1 << 24)) != 0) {
		// Not from the numeric keypad
		return false;
	}

	switch (wParam) {
	case VK_INSERT:	// 0
	case VK_END:	// 1
	case VK_DOWN:	// 2
	case VK_NEXT:	// 3
	case VK_LEFT:	// 4
	case VK_CLEAR:	// 5
	case VK_RIGHT:	// 6
	case VK_HOME:	// 7
	case VK_UP:		// 8
	case VK_PRIOR:	// 9
		return true;
	default:
		return false;
	}
}

typedef void VFunction(void);

/**
 */
class FormatEnumerator {
public:
	VFunction **vtbl;
	ULONG ref;
	ULONG pos;
	std::vector<CLIPFORMAT> formats;
	FormatEnumerator(ULONG pos_, const CLIPFORMAT formats_[], size_t formatsLen_);
};

/**
 */
class DropSource {
public:
	VFunction **vtbl;
	ScintillaWin *sci;
	DropSource() noexcept;
};

/**
 */
class DataObject {
public:
	VFunction **vtbl;
	ScintillaWin *sci;
	DataObject() noexcept;
};

/**
 */
class DropTarget {
public:
	VFunction **vtbl;
	ScintillaWin *sci;
	DropTarget() noexcept;
};

class IMContext {
	HWND hwnd;
public:
	HIMC hIMC;
	IMContext(HWND hwnd_) noexcept :
		hwnd(hwnd_), hIMC(::ImmGetContext(hwnd_)) {
	}
	// Deleted so IMContext objects can not be copied.
	IMContext(const IMContext &) = delete;
	IMContext(IMContext &&) = delete;
	IMContext &operator=(const IMContext &) = delete;
	IMContext &operator=(IMContext &&) = delete;
	~IMContext() {
		if (hIMC)
			::ImmReleaseContext(hwnd, hIMC);
	}

	unsigned int GetImeCaretPos() const noexcept {
		return ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, nullptr, 0);
	}

	std::vector<BYTE> GetImeAttributes() {
		const int attrLen = ::ImmGetCompositionStringW(hIMC, GCS_COMPATTR, nullptr, 0);
		std::vector<BYTE> attr(attrLen, 0);
		::ImmGetCompositionStringW(hIMC, GCS_COMPATTR, &attr[0], static_cast<DWORD>(attr.size()));
		return attr;
	}

	std::wstring GetCompositionString(DWORD dwIndex) {
		const LONG byteLen = ::ImmGetCompositionStringW(hIMC, dwIndex, nullptr, 0);
		std::wstring wcs(byteLen / 2, 0);
		::ImmGetCompositionStringW(hIMC, dwIndex, &wcs[0], byteLen);
		return wcs;
	}
};

class GlobalMemory;

class ReverseArrowCursor {
	UINT dpi = USER_DEFAULT_SCREEN_DPI;
	HCURSOR cursor {};

public:
	ReverseArrowCursor() noexcept {}
	// Deleted so ReverseArrowCursor objects can not be copied.
	ReverseArrowCursor(const ReverseArrowCursor &) = delete;
	ReverseArrowCursor(ReverseArrowCursor &&) = delete;
	ReverseArrowCursor &operator=(const ReverseArrowCursor &) = delete;
	ReverseArrowCursor &operator=(ReverseArrowCursor &&) = delete;
	~ReverseArrowCursor() {
		if (cursor) {
			::DestroyCursor(cursor);
		}
	}

	HCURSOR Load(UINT dpi_) noexcept {
		if (cursor)	 {
			if (dpi == dpi_) {
				return cursor;
			}
			::DestroyCursor(cursor);
		}

		dpi = dpi_;
		cursor = LoadReverseArrowCursor(dpi_);
		return cursor ? cursor : ::LoadCursor({}, IDC_ARROW);
	}
};

}

namespace Scintilla::Internal {

/**
 */
class ScintillaWin :
	public ScintillaBase {

	bool lastKeyDownConsumed;
	wchar_t lastHighSurrogateChar;

	bool capturedMouse;
	bool trackedMouseLeave;
	SetCoalescableTimerSig SetCoalescableTimerFn;

	unsigned int linesPerScroll;	///< Intellimouse support
	int wheelDelta; ///< Wheel delta from roll

	UINT dpi = USER_DEFAULT_SCREEN_DPI;
	ReverseArrowCursor reverseArrowCursor;

	PRectangle rectangleClient;
	HRGN hRgnUpdate;

	bool hasOKText;

	CLIPFORMAT cfColumnSelect;
	CLIPFORMAT cfBorlandIDEBlockType;
	CLIPFORMAT cfLineSelect;
	CLIPFORMAT cfVSLineTag;

	HRESULT hrOle;
	DropSource ds;
	DataObject dob;
	DropTarget dt;

	static HINSTANCE hInstance;
	static ATOM scintillaClassAtom;
	static ATOM callClassAtom;

#if defined(USE_D2D)
	ID2D1RenderTarget *pRenderTarget;
	bool renderTargetValid;
	// rendering parameters for current monitor
	HMONITOR hCurrentMonitor;
	std::shared_ptr<RenderingParams> renderingParams;
#endif

	explicit ScintillaWin(HWND hwnd);
	// Deleted so ScintillaWin objects can not be copied.
	ScintillaWin(const ScintillaWin &) = delete;
	ScintillaWin(ScintillaWin &&) = delete;
	ScintillaWin &operator=(const ScintillaWin &) = delete;
	ScintillaWin &operator=(ScintillaWin &&) = delete;
	// ~ScintillaWin() in public section

	void Finalise() override;
#if defined(USE_D2D)
	bool UpdateRenderingParams(bool force) noexcept;
	void EnsureRenderTarget(HDC hdc);
#endif
	void DropRenderTarget() noexcept;
	HWND MainHWND() const noexcept;

	static sptr_t DirectFunction(
		    sptr_t ptr, UINT iMessage, uptr_t wParam, sptr_t lParam);
	static sptr_t DirectStatusFunction(
		    sptr_t ptr, UINT iMessage, uptr_t wParam, sptr_t lParam, int *pStatus);
	static LRESULT PASCAL SWndProc(
		    HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	static LRESULT PASCAL CTWndProc(
		    HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	enum : UINT_PTR { invalidTimerID, standardTimerID, idleTimerID, fineTimerStart };

	void DisplayCursor(Window::Cursor c) override;
	bool DragThreshold(Point ptStart, Point ptNow) override;
	void StartDrag() override;
	static KeyMod MouseModifiers(uptr_t wParam) noexcept;

	Sci::Position TargetAsUTF8(char *text) const;
	Sci::Position EncodedFromUTF8(const char *utf8, char *encoded) const;

	void SetRenderingParams(Surface *psurf) const;

	bool PaintDC(HDC hdc);
	sptr_t WndPaint();

	// DBCS
	void ImeStartComposition();
	void ImeEndComposition();
	LRESULT ImeOnReconvert(LPARAM lParam);
	sptr_t HandleCompositionWindowed(uptr_t wParam, sptr_t lParam);
	sptr_t HandleCompositionInline(uptr_t wParam, sptr_t lParam);
	static bool KoreanIME() noexcept;
	void MoveImeCarets(Sci::Position offset) noexcept;
	void DrawImeIndicator(int indicator, Sci::Position len);
	void SetCandidateWindowPos();
	void SelectionToHangul();
	void EscapeHanja();
	void ToggleHanja();
	void AddWString(std::wstring_view wsv, CharacterSource charSource);

	UINT CodePageOfDocument() const noexcept;
	bool ValidCodePage(int codePage) const override;
	std::string UTF8FromEncoded(std::string_view encoded) const override;
	std::string EncodedFromUTF8(std::string_view utf8) const override;

	std::string EncodeWString(std::wstring_view wsv);
	sptr_t DefWndProc(Message iMessage, uptr_t wParam, sptr_t lParam) override;
	void IdleWork() override;
	void QueueIdleWork(WorkItems items, Sci::Position upTo) override;
	bool SetIdle(bool on) override;
	UINT_PTR timers[static_cast<int>(TickReason::dwell)+1] {};
	bool FineTickerRunning(TickReason reason) override;
	void FineTickerStart(TickReason reason, int millis, int tolerance) override;
	void FineTickerCancel(TickReason reason) override;
	void SetMouseCapture(bool on) override;
	bool HaveMouseCapture() override;
	void SetTrackMouseLeaveEvent(bool on) noexcept;
	void UpdateBaseElements() override;
	bool PaintContains(PRectangle rc) override;
	void ScrollText(Sci::Line linesToMove) override;
	void NotifyCaretMove() override;
	void UpdateSystemCaret() override;
	void SetVerticalScrollPos() override;
	void SetHorizontalScrollPos() override;
	bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) override;
	void NotifyChange() override;
	void NotifyFocus(bool focus) override;
	void SetCtrlID(int identifier) override;
	int GetCtrlID() override;
	void NotifyParent(NotificationData scn) override;
	void NotifyDoubleClick(Point pt, KeyMod modifiers) override;
	std::unique_ptr<CaseFolder> CaseFolderForEncoding() override;
	std::string CaseMapString(const std::string &s, CaseMapping caseMapping) override;
	void Copy() override;
	bool CanPaste() override;
	void Paste() override;
	void CreateCallTipWindow(PRectangle rc) override;
	void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;
	void ClaimSelection() override;

	void GetIntelliMouseParameters() noexcept;
	void CopyToGlobal(GlobalMemory &gmUnicode, const SelectionText &selectedText);
	void CopyToClipboard(const SelectionText &selectedText) override;
	void ScrollMessage(WPARAM wParam);
	void HorizontalScrollMessage(WPARAM wParam);
	void FullPaint();
	void FullPaintDC(HDC hdc);
	bool IsCompatibleDC(HDC hOtherDC) noexcept;
	DWORD EffectFromState(DWORD grfKeyState) const noexcept;

	bool IsVisible() const noexcept;
	int SetScrollInfo(int nBar, LPCSCROLLINFO lpsi, BOOL bRedraw) noexcept;
	bool GetScrollInfo(int nBar, LPSCROLLINFO lpsi) noexcept;
	bool ChangeScrollRange(int nBar, int nMin, int nMax, UINT nPage) noexcept;
	void ChangeScrollPos(int barType, Sci::Position pos);
	sptr_t GetTextLength();
	sptr_t GetText(uptr_t wParam, sptr_t lParam);
	Window::Cursor ContextCursor(Point pt);
	sptr_t ShowContextMenu(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	PRectangle GetClientRectangle() const override;
	void SizeWindow();
	sptr_t MouseMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t KeyMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t FocusMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t IMEMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t EditMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t IdleMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t SciMessage(Message iMessage, uptr_t wParam, sptr_t lParam);

public:
	~ScintillaWin() override;

	// Public for benefit of Scintilla_DirectFunction
	sptr_t WndProc(Message iMessage, uptr_t wParam, sptr_t lParam) override;

	/// Implement IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppv);
	STDMETHODIMP_(ULONG)AddRef();
	STDMETHODIMP_(ULONG)Release();

	/// Implement IDropTarget
	STDMETHODIMP DragEnter(LPDATAOBJECT pIDataSource, DWORD grfKeyState,
	                       POINTL pt, PDWORD pdwEffect);
	STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, PDWORD pdwEffect);
	STDMETHODIMP DragLeave();
	STDMETHODIMP Drop(LPDATAOBJECT pIDataSource, DWORD grfKeyState,
	                  POINTL pt, PDWORD pdwEffect);

	/// Implement important part of IDataObject
	STDMETHODIMP GetData(FORMATETC *pFEIn, STGMEDIUM *pSTM);

	static void Prepare() noexcept;
	static bool Register(HINSTANCE hInstance_) noexcept;
	static bool Unregister() noexcept;

	bool DragIsRectangularOK(CLIPFORMAT fmt) const noexcept {
		return drag.rectangular && (fmt == cfColumnSelect);
	}

private:
	// For use in creating a system caret
	bool HasCaretSizeChanged() const noexcept;
	BOOL CreateSystemCaret();
	BOOL DestroySystemCaret() noexcept;
	HBITMAP sysCaretBitmap;
	int sysCaretWidth;
	int sysCaretHeight;
	bool styleIdleInQueue;
};

HINSTANCE ScintillaWin::hInstance {};
ATOM ScintillaWin::scintillaClassAtom = 0;
ATOM ScintillaWin::callClassAtom = 0;

ScintillaWin::ScintillaWin(HWND hwnd) {

	lastKeyDownConsumed = false;
	lastHighSurrogateChar = 0;

	capturedMouse = false;
	trackedMouseLeave = false;
	SetCoalescableTimerFn = nullptr;

	linesPerScroll = 0;
	wheelDelta = 0;   // Wheel delta from roll

	dpi = DpiForWindow(hwnd);

	hRgnUpdate = {};

	hasOKText = false;

	// There does not seem to be a real standard for indicating that the clipboard
	// contains a rectangular selection, so copy Developer Studio and Borland Delphi.
	cfColumnSelect = static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(TEXT("MSDEVColumnSelect")));
	cfBorlandIDEBlockType = static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(TEXT("Borland IDE Block Type")));

	// Likewise for line-copy (copies a full line when no text is selected)
	cfLineSelect = static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(TEXT("MSDEVLineSelect")));
	cfVSLineTag = static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(TEXT("VisualStudioEditorOperationsLineCutCopyClipboardTag")));
	hrOle = E_FAIL;

	wMain = hwnd;

	dob.sci = this;
	ds.sci = this;
	dt.sci = this;

	sysCaretBitmap = {};
	sysCaretWidth = 0;
	sysCaretHeight = 0;

	styleIdleInQueue = false;

#if defined(USE_D2D)
	pRenderTarget = nullptr;
	renderTargetValid = true;
	hCurrentMonitor = {};
#endif

	caret.period = ::GetCaretBlinkTime();
	if (caret.period < 0)
		caret.period = 0;

	// Initialize COM.  If the app has already done this it will have
	// no effect.  If the app hasn't, we really shouldn't ask them to call
	// it just so this internal feature works.
	hrOle = ::OleInitialize(nullptr);

	// Find SetCoalescableTimer which is only available from Windows 8+
	HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
	SetCoalescableTimerFn = DLLFunction<SetCoalescableTimerSig>(user32, "SetCoalescableTimer");

	vs.indicators[IndicatorUnknown] = Indicator(IndicatorStyle::Hidden, ColourRGBA(0, 0, 0xff));
	vs.indicators[IndicatorInput] = Indicator(IndicatorStyle::Dots, ColourRGBA(0, 0, 0xff));
	vs.indicators[IndicatorConverted] = Indicator(IndicatorStyle::CompositionThick, ColourRGBA(0, 0, 0xff));
	vs.indicators[IndicatorTarget] = Indicator(IndicatorStyle::StraightBox, ColourRGBA(0, 0, 0xff));
}

ScintillaWin::~ScintillaWin() {
	if (sysCaretBitmap) {
		::DeleteObject(sysCaretBitmap);
		sysCaretBitmap = {};
	}
}

void ScintillaWin::Finalise() {
	ScintillaBase::Finalise();
	for (TickReason tr = TickReason::caret; tr <= TickReason::dwell;
		tr = static_cast<TickReason>(static_cast<int>(tr) + 1)) {
		FineTickerCancel(tr);
	}
	SetIdle(false);
	DropRenderTarget();
	::RevokeDragDrop(MainHWND());
	if (SUCCEEDED(hrOle)) {
		::OleUninitialize();
	}
}

#if defined(USE_D2D)

bool ScintillaWin::UpdateRenderingParams(bool force) noexcept {
	if (!renderingParams) {
		renderingParams = std::make_shared<RenderingParams>();
	}
	HMONITOR monitor = ::MonitorFromWindow(MainHWND(), MONITOR_DEFAULTTONEAREST);
	if (!force && monitor == hCurrentMonitor && renderingParams->defaultRenderingParams) {
		return false;
	}

	IDWriteRenderingParams *monitorRenderingParams = nullptr;
	IDWriteRenderingParams *customClearTypeRenderingParams = nullptr;
	const HRESULT hr = pIDWriteFactory->CreateMonitorRenderingParams(monitor, &monitorRenderingParams);
	UINT clearTypeContrast = 0;
	if (SUCCEEDED(hr) && ::SystemParametersInfo(SPI_GETFONTSMOOTHINGCONTRAST, 0, &clearTypeContrast, 0) != 0) {
		if (clearTypeContrast >= 1000 && clearTypeContrast <= 2200) {
			const FLOAT gamma = static_cast<FLOAT>(clearTypeContrast) / 1000.0f;
			pIDWriteFactory->CreateCustomRenderingParams(gamma,
				monitorRenderingParams->GetEnhancedContrast(),
				monitorRenderingParams->GetClearTypeLevel(),
				monitorRenderingParams->GetPixelGeometry(),
				monitorRenderingParams->GetRenderingMode(),
				&customClearTypeRenderingParams);
		}
	}

	hCurrentMonitor = monitor;
	renderingParams->defaultRenderingParams.reset(monitorRenderingParams);
	renderingParams->customRenderingParams.reset(customClearTypeRenderingParams);
	return true;
}


void ScintillaWin::EnsureRenderTarget(HDC hdc) {
	if (!renderTargetValid) {
		DropRenderTarget();
		renderTargetValid = true;
	}
	if (!pRenderTarget) {
		HWND hw = MainHWND();
		RECT rc;
		::GetClientRect(hw, &rc);

		const D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		// Create a Direct2D render target.
		D2D1_RENDER_TARGET_PROPERTIES drtp {};
		drtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
		drtp.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
		drtp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
		drtp.dpiX = 96.0;
		drtp.dpiY = 96.0;
		drtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;
		drtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

		if (technology == Technology::DirectWriteDC) {
			// Explicit pixel format needed.
			drtp.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
				D2D1_ALPHA_MODE_IGNORE);

			ID2D1DCRenderTarget *pDCRT = nullptr;
			const HRESULT hr = pD2DFactory->CreateDCRenderTarget(&drtp, &pDCRT);
			if (SUCCEEDED(hr)) {
				pRenderTarget = pDCRT;
			} else {
				Platform::DebugPrintf("Failed CreateDCRenderTarget 0x%lx\n", hr);
				pRenderTarget = nullptr;
			}

		} else {
			D2D1_HWND_RENDER_TARGET_PROPERTIES dhrtp {};
			dhrtp.hwnd = hw;
			dhrtp.pixelSize = size;
			dhrtp.presentOptions = (technology == Technology::DirectWriteRetain) ?
			D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS : D2D1_PRESENT_OPTIONS_NONE;

			ID2D1HwndRenderTarget *pHwndRenderTarget = nullptr;
			const HRESULT hr = pD2DFactory->CreateHwndRenderTarget(drtp, dhrtp, &pHwndRenderTarget);
			if (SUCCEEDED(hr)) {
				pRenderTarget = pHwndRenderTarget;
			} else {
				Platform::DebugPrintf("Failed CreateHwndRenderTarget 0x%lx\n", hr);
				pRenderTarget = nullptr;
			}
		}
		// Pixmaps were created to be compatible with previous render target so
		// need to be recreated.
		DropGraphics();
	}

	if ((technology == Technology::DirectWriteDC) && pRenderTarget) {
		RECT rcWindow;
		::GetClientRect(MainHWND(), &rcWindow);
		const HRESULT hr = static_cast<ID2D1DCRenderTarget*>(pRenderTarget)->BindDC(hdc, &rcWindow);
		if (FAILED(hr)) {
			Platform::DebugPrintf("BindDC failed 0x%lx\n", hr);
			DropRenderTarget();
		}
	}
}
#endif

void ScintillaWin::DropRenderTarget() noexcept {
#if defined(USE_D2D)
	ReleaseUnknown(pRenderTarget);
#endif
}

HWND ScintillaWin::MainHWND() const noexcept {
	return HwndFromWindow(wMain);
}

void ScintillaWin::DisplayCursor(Window::Cursor c) {
	if (cursorMode != CursorShape::Normal) {
		c = static_cast<Window::Cursor>(cursorMode);
	}
	if (c == Window::Cursor::reverseArrow) {
		::SetCursor(reverseArrowCursor.Load(dpi));
	} else {
		wMain.SetCursor(c);
	}
}

bool ScintillaWin::DragThreshold(Point ptStart, Point ptNow) {
	const Point ptDifference = ptStart - ptNow;
	const XYPOSITION xMove = std::trunc(std::abs(ptDifference.x));
	const XYPOSITION yMove = std::trunc(std::abs(ptDifference.y));
	return (xMove > SystemMetricsForDpi(SM_CXDRAG, dpi)) ||
		(yMove > SystemMetricsForDpi(SM_CYDRAG, dpi));
}

void ScintillaWin::StartDrag() {
	inDragDrop = DragDrop::dragging;
	DWORD dwEffect = 0;
	dropWentOutside = true;
	IDataObject *pDataObject = reinterpret_cast<IDataObject *>(&dob);
	IDropSource *pDropSource = reinterpret_cast<IDropSource *>(&ds);
	//Platform::DebugPrintf("About to DoDragDrop %x %x\n", pDataObject, pDropSource);
	const HRESULT hr = ::DoDragDrop(
	                 pDataObject,
	                 pDropSource,
	                 DROPEFFECT_COPY | DROPEFFECT_MOVE, &dwEffect);
	//Platform::DebugPrintf("DoDragDrop = %x\n", hr);
	if (SUCCEEDED(hr)) {
		if ((hr == DRAGDROP_S_DROP) && (dwEffect == DROPEFFECT_MOVE) && dropWentOutside) {
			// Remove dragged out text
			ClearSelection();
		}
	}
	inDragDrop = DragDrop::none;
	SetDragPosition(SelectionPosition(Sci::invalidPosition));
}

KeyMod ScintillaWin::MouseModifiers(uptr_t wParam) noexcept {
	return ModifierFlags(
		(wParam & MK_SHIFT) != 0,
		(wParam & MK_CONTROL) != 0,
		KeyboardIsKeyDown(VK_MENU));
}

}

namespace {

int InputCodePage() noexcept {
	HKL inputLocale = ::GetKeyboardLayout(0);
	const LANGID inputLang = LOWORD(inputLocale);
	char sCodePage[10];
	const int res = ::GetLocaleInfoA(MAKELCID(inputLang, SORT_DEFAULT),
	  LOCALE_IDEFAULTANSICODEPAGE, sCodePage, sizeof(sCodePage));
	if (!res)
		return 0;
	return atoi(sCodePage);
}

/** Map the key codes to their equivalent Keys:: form. */
Keys KeyTranslate(uptr_t keyIn) noexcept {
	switch (keyIn) {
	case VK_DOWN:		return Keys::Down;
		case VK_UP:		return Keys::Up;
		case VK_LEFT:		return Keys::Left;
		case VK_RIGHT:		return Keys::Right;
		case VK_HOME:		return Keys::Home;
		case VK_END:		return Keys::End;
		case VK_PRIOR:		return Keys::Prior;
		case VK_NEXT:		return Keys::Next;
		case VK_DELETE:	return Keys::Delete;
		case VK_INSERT:		return Keys::Insert;
		case VK_ESCAPE:	return Keys::Escape;
		case VK_BACK:		return Keys::Back;
		case VK_TAB:		return Keys::Tab;
		case VK_RETURN:	return Keys::Return;
		case VK_ADD:		return Keys::Add;
		case VK_SUBTRACT:	return Keys::Subtract;
		case VK_DIVIDE:		return Keys::Divide;
		case VK_LWIN:		return Keys::Win;
		case VK_RWIN:		return Keys::RWin;
		case VK_APPS:		return Keys::Menu;
		case VK_OEM_2:		return static_cast<Keys>('/');
		case VK_OEM_3:		return static_cast<Keys>('`');
		case VK_OEM_4:		return static_cast<Keys>('[');
		case VK_OEM_5:		return static_cast<Keys>('\\');
		case VK_OEM_6:		return static_cast<Keys>(']');
		default:			return static_cast<Keys>(keyIn);
	}
}

bool BoundsContains(PRectangle rcBounds, HRGN hRgnBounds, PRectangle rcCheck) noexcept {
	bool contains = true;
	if (!rcCheck.Empty()) {
		if (!rcBounds.Contains(rcCheck)) {
			contains = false;
		} else if (hRgnBounds) {
			// In bounding rectangle so check more accurately using region
			const RECT rcw = RectFromPRectangle(rcCheck);
			HRGN hRgnCheck = ::CreateRectRgnIndirect(&rcw);
			if (hRgnCheck) {
				HRGN hRgnDifference = ::CreateRectRgn(0, 0, 0, 0);
				if (hRgnDifference) {
					const int combination = ::CombineRgn(hRgnDifference, hRgnCheck, hRgnBounds, RGN_DIFF);
					if (combination != NULLREGION) {
						contains = false;
					}
					::DeleteRgn(hRgnDifference);
				}
				::DeleteRgn(hRgnCheck);
			}
		}
	}
	return contains;
}

// Simplify calling WideCharToMultiByte and MultiByteToWideChar by providing default parameters and using string view.

int MultiByteFromWideChar(UINT codePage, std::wstring_view wsv, LPSTR lpMultiByteStr, ptrdiff_t cbMultiByte) noexcept {
	return ::WideCharToMultiByte(codePage, 0, wsv.data(), static_cast<int>(wsv.length()), lpMultiByteStr, static_cast<int>(cbMultiByte), nullptr, nullptr);
}

int MultiByteLenFromWideChar(UINT codePage, std::wstring_view wsv) noexcept {
	return MultiByteFromWideChar(codePage, wsv, nullptr, 0);
}

int WideCharFromMultiByte(UINT codePage, std::string_view sv, LPWSTR lpWideCharStr, ptrdiff_t cchWideChar) noexcept {
	return ::MultiByteToWideChar(codePage, 0, sv.data(), static_cast<int>(sv.length()), lpWideCharStr, static_cast<int>(cchWideChar));
}

int WideCharLenFromMultiByte(UINT codePage, std::string_view sv) noexcept {
	return WideCharFromMultiByte(codePage, sv, nullptr, 0);
}

std::string StringEncode(std::wstring_view wsv, int codePage) {
	const int cchMulti = wsv.length() ? MultiByteLenFromWideChar(codePage, wsv) : 0;
	std::string sMulti(cchMulti, 0);
	if (cchMulti) {
		MultiByteFromWideChar(codePage, wsv, sMulti.data(), cchMulti);
	}
	return sMulti;
}

std::wstring StringDecode(std::string_view sv, int codePage) {
	const int cchWide = sv.length() ? WideCharLenFromMultiByte(codePage, sv) : 0;
	std::wstring sWide(cchWide, 0);
	if (cchWide) {
		WideCharFromMultiByte(codePage, sv, sWide.data(), cchWide);
	}
	return sWide;
}

std::wstring StringMapCase(std::wstring_view wsv, DWORD mapFlags) {
	const int charsConverted = ::LCMapStringW(LOCALE_SYSTEM_DEFAULT, mapFlags,
		wsv.data(), static_cast<int>(wsv.length()), nullptr, 0);
	std::wstring wsConverted(charsConverted, 0);
	if (charsConverted) {
		::LCMapStringW(LOCALE_SYSTEM_DEFAULT, mapFlags,
			wsv.data(), static_cast<int>(wsv.length()), wsConverted.data(), charsConverted);
	}
	return wsConverted;
}

}

// Returns the target converted to UTF8.
// Return the length in bytes.
Sci::Position ScintillaWin::TargetAsUTF8(char *text) const {
	const Sci::Position targetLength = targetRange.Length();
	if (IsUnicodeMode()) {
		if (text) {
			pdoc->GetCharRange(text, targetRange.start.Position(), targetLength);
		}
	} else {
		// Need to convert
		const std::string s = RangeText(targetRange.start.Position(), targetRange.end.Position());
		const std::wstring characters = StringDecode(s, CodePageOfDocument());
		const int utf8Len = MultiByteLenFromWideChar(CpUtf8, characters);
		if (text) {
			MultiByteFromWideChar(CpUtf8, characters, text, utf8Len);
			text[utf8Len] = '\0';
		}
		return utf8Len;
	}
	return targetLength;
}

// Translates a nul terminated UTF8 string into the document encoding.
// Return the length of the result in bytes.
Sci::Position ScintillaWin::EncodedFromUTF8(const char *utf8, char *encoded) const {
	const Sci::Position inputLength = (lengthForEncode >= 0) ? lengthForEncode : strlen(utf8);
	if (IsUnicodeMode()) {
		if (encoded) {
			memcpy(encoded, utf8, inputLength);
		}
		return inputLength;
	} else {
		// Need to convert
		const std::string_view utf8Input(utf8, inputLength);
		const int charsLen = WideCharLenFromMultiByte(CpUtf8, utf8Input);
		std::wstring characters(charsLen, L'\0');
		WideCharFromMultiByte(CpUtf8, utf8Input, &characters[0], charsLen);

		const int encodedLen = MultiByteLenFromWideChar(CodePageOfDocument(), characters);
		if (encoded) {
			MultiByteFromWideChar(CodePageOfDocument(), characters, encoded, encodedLen);
			encoded[encodedLen] = '\0';
		}
		return encodedLen;
	}
}

void ScintillaWin::SetRenderingParams([[maybe_unused]] Surface *psurf) const {
#if defined(USE_D2D)
	if (psurf) {
		ISetRenderingParams *setDrawingParams = dynamic_cast<ISetRenderingParams *>(psurf);
		if (setDrawingParams) {
			setDrawingParams->SetRenderingParams(renderingParams);
		}
	}
#endif
}

bool ScintillaWin::PaintDC(HDC hdc) {
	if (technology == Technology::Default) {
		AutoSurface surfaceWindow(hdc, this);
		if (surfaceWindow) {
			Paint(surfaceWindow, rcPaint);
			surfaceWindow->Release();
		}
	} else {
#if defined(USE_D2D)
		EnsureRenderTarget(hdc);
		if (pRenderTarget) {
			AutoSurface surfaceWindow(pRenderTarget, this);
			if (surfaceWindow) {
				SetRenderingParams(surfaceWindow);
				pRenderTarget->BeginDraw();
				Paint(surfaceWindow, rcPaint);
				surfaceWindow->Release();
				const HRESULT hr = pRenderTarget->EndDraw();
				if (hr == static_cast<HRESULT>(D2DERR_RECREATE_TARGET)) {
					DropRenderTarget();
					return false;
				}
			}
		}
#endif
	}

	return true;
}

sptr_t ScintillaWin::WndPaint() {
	//ElapsedPeriod ep;

	// Redirect assertions to debug output and save current state
	const bool assertsPopup = Platform::ShowAssertionPopUps(false);
	paintState = PaintState::painting;
	PAINTSTRUCT ps = {};

	// Removed since this interferes with reporting other assertions as it occurs repeatedly
	//PLATFORM_ASSERT(hRgnUpdate == NULL);
	hRgnUpdate = ::CreateRectRgn(0, 0, 0, 0);
	::GetUpdateRgn(MainHWND(), hRgnUpdate, FALSE);
	::BeginPaint(MainHWND(), &ps);
	rcPaint = PRectangle::FromInts(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
	const PRectangle rcClient = GetClientRectangle();
	paintingAllText = BoundsContains(rcPaint, hRgnUpdate, rcClient);
	if (!PaintDC(ps.hdc)) {
		paintState = PaintState::abandoned;
	}
	if (hRgnUpdate) {
		::DeleteRgn(hRgnUpdate);
		hRgnUpdate = {};
	}

	::EndPaint(MainHWND(), &ps);
	if (paintState == PaintState::abandoned) {
		// Painting area was insufficient to cover new styling or brace highlight positions
		FullPaint();
		::ValidateRect(MainHWND(), nullptr);
	}
	paintState = PaintState::notPainting;

	// Restore debug output state
	Platform::ShowAssertionPopUps(assertsPopup);

	//Platform::DebugPrintf("Paint took %g\n", ep.Duration());
	return 0;
}

sptr_t ScintillaWin::HandleCompositionWindowed(uptr_t wParam, sptr_t lParam) {
	if (lParam & GCS_RESULTSTR) {
		IMContext imc(MainHWND());
		if (imc.hIMC) {
			AddWString(imc.GetCompositionString(GCS_RESULTSTR), CharacterSource::ImeResult);

			// Set new position after converted
			const Point pos = PointMainCaret();
			COMPOSITIONFORM CompForm {};
			CompForm.dwStyle = CFS_POINT;
			CompForm.ptCurrentPos = POINTFromPoint(pos);
			::ImmSetCompositionWindow(imc.hIMC, &CompForm);
		}
		return 0;
	}
	return ::DefWindowProc(MainHWND(), WM_IME_COMPOSITION, wParam, lParam);
}

bool ScintillaWin::KoreanIME() noexcept {
	const int codePage = InputCodePage();
	return codePage == 949 || codePage == 1361;
}

void ScintillaWin::MoveImeCarets(Sci::Position offset) noexcept {
	// Move carets relatively by bytes.
	for (size_t r=0; r<sel.Count(); r++) {
		const Sci::Position positionInsert = sel.Range(r).Start().Position();
		sel.Range(r).caret.SetPosition(positionInsert + offset);
		sel.Range(r).anchor.SetPosition(positionInsert + offset);
	}
}

void ScintillaWin::DrawImeIndicator(int indicator, Sci::Position len) {
	// Emulate the visual style of IME characters with indicators.
	// Draw an indicator on the character before caret by the character bytes of len
	// so it should be called after InsertCharacter().
	// It does not affect caret positions.
	if (indicator < 8 || indicator > IndicatorMax) {
		return;
	}
	pdoc->DecorationSetCurrentIndicator(indicator);
	for (size_t r=0; r<sel.Count(); r++) {
		const Sci::Position positionInsert = sel.Range(r).Start().Position();
		pdoc->DecorationFillRange(positionInsert - len, 1, len);
	}
}

void ScintillaWin::SetCandidateWindowPos() {
	IMContext imc(MainHWND());
	if (imc.hIMC) {
		const Point pos = PointMainCaret();
		const PRectangle rcClient = GetTextRectangle();
		CANDIDATEFORM CandForm{};
		CandForm.dwIndex = 0;
		CandForm.dwStyle = CFS_EXCLUDE;
		CandForm.ptCurrentPos.x = static_cast<int>(pos.x);
		CandForm.ptCurrentPos.y = static_cast<int>(pos.y + std::max(4, vs.lineHeight/4));
		// Exclude the area of the whole caret line
		CandForm.rcArea.top = static_cast<int>(pos.y);
		CandForm.rcArea.bottom = static_cast<int>(pos.y + vs.lineHeight);
		CandForm.rcArea.left = static_cast<int>(rcClient.left);
		CandForm.rcArea.right = static_cast<int>(rcClient.right);
		::ImmSetCandidateWindow(imc.hIMC, &CandForm);
	}
}

void ScintillaWin::SelectionToHangul() {
	// Convert every hanja to hangul within the main range.
	const Sci::Position selStart = sel.RangeMain().Start().Position();
	const Sci::Position documentStrLen = sel.RangeMain().Length();
	const Sci::Position selEnd = selStart + documentStrLen;
	const Sci::Position utf16Len = pdoc->CountUTF16(selStart, selEnd);

	if (utf16Len > 0) {
		std::string documentStr(documentStrLen, '\0');
		pdoc->GetCharRange(&documentStr[0], selStart, documentStrLen);

		std::wstring uniStr = StringDecode(documentStr, CodePageOfDocument());
		const bool converted = HanjaDict::GetHangulOfHanja(uniStr);

		if (converted) {
			documentStr = StringEncode(uniStr, CodePageOfDocument());
			pdoc->BeginUndoAction();
			ClearSelection();
			InsertPaste(&documentStr[0], documentStr.size());
			pdoc->EndUndoAction();
		}
	}
}

void ScintillaWin::EscapeHanja() {
	// The candidate box pops up to user to select a hanja.
	// It comes into WM_IME_COMPOSITION with GCS_RESULTSTR.
	// The existing hangul or hanja is replaced with it.
	if (sel.Count() > 1) {
		return; // Do not allow multi carets.
	}
	const Sci::Position currentPos = CurrentPosition();
	const int oneCharLen = pdoc->LenChar(currentPos);

	if (oneCharLen < 2) {
		return; // No need to handle SBCS.
	}

	// ImmEscapeW() may overwrite uniChar[] with a null terminated string.
	// So enlarge it enough to Maximum 4 as in UTF-8.
	constexpr size_t safeLength = UTF8MaxBytes + 1;
	std::string oneChar(safeLength, '\0');
	pdoc->GetCharRange(&oneChar[0], currentPos, oneCharLen);

	std::wstring uniChar = StringDecode(oneChar, CodePageOfDocument());

	IMContext imc(MainHWND());
	if (imc.hIMC) {
		// Set the candidate box position since IME may show it.
		SetCandidateWindowPos();
		// IME_ESC_HANJA_MODE appears to receive the first character only.
		if (::ImmEscapeW(GetKeyboardLayout(0), imc.hIMC, IME_ESC_HANJA_MODE, &uniChar[0])) {
			SetSelection(currentPos, currentPos + oneCharLen);
		}
	}
}

void ScintillaWin::ToggleHanja() {
	// If selection, convert every hanja to hangul within the main range.
	// If no selection, commit to IME.
	if (sel.Count() > 1) {
		return; // Do not allow multi carets.
	}

	if (sel.Empty()) {
		EscapeHanja();
	} else {
		SelectionToHangul();
	}
}

namespace {

std::vector<int> MapImeIndicators(std::vector<BYTE> inputStyle) {
	std::vector<int> imeIndicator(inputStyle.size(), IndicatorUnknown);
	for (size_t i = 0; i < inputStyle.size(); i++) {
		switch (static_cast<int>(inputStyle.at(i))) {
		case ATTR_INPUT:
			imeIndicator[i] = IndicatorInput;
			break;
		case ATTR_TARGET_NOTCONVERTED:
		case ATTR_TARGET_CONVERTED:
			imeIndicator[i] = IndicatorTarget;
			break;
		case ATTR_CONVERTED:
			imeIndicator[i] = IndicatorConverted;
			break;
		default:
			imeIndicator[i] = IndicatorUnknown;
			break;
		}
	}
	return imeIndicator;
}

}

void ScintillaWin::AddWString(std::wstring_view wsv, CharacterSource charSource) {
	if (wsv.empty())
		return;

	const int codePage = CodePageOfDocument();
	for (size_t i = 0; i < wsv.size(); ) {
		const size_t ucWidth = UTF16CharLength(wsv[i]);
		const std::string docChar = StringEncode(wsv.substr(i, ucWidth), codePage);

		InsertCharacter(docChar, charSource);
		i += ucWidth;
	}
}

sptr_t ScintillaWin::HandleCompositionInline(uptr_t, sptr_t lParam) {
	// Copy & paste by johnsonj with a lot of helps of Neil.
	// Great thanks for my foreruners, jiniya and BLUEnLIVE.

	IMContext imc(MainHWND());
	if (!imc.hIMC)
		return 0;
	if (pdoc->IsReadOnly() || SelectionContainsProtected()) {
		::ImmNotifyIME(imc.hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
		return 0;
	}

	bool initialCompose = false;
	if (pdoc->TentativeActive()) {
		pdoc->TentativeUndo();
	} else {
		// No tentative undo means start of this composition so
		// fill in any virtual spaces.
		initialCompose = true;
	}

	view.imeCaretBlockOverride = false;

	if (lParam & GCS_RESULTSTR) {
		AddWString(imc.GetCompositionString(GCS_RESULTSTR), CharacterSource::ImeResult);
	}

	if (lParam & GCS_COMPSTR) {
		const std::wstring wcs = imc.GetCompositionString(GCS_COMPSTR);
		if (wcs.empty()) {
			ShowCaretAtCurrentPosition();
			return 0;
		}

		if (initialCompose) {
			ClearBeforeTentativeStart();
		}

		// Set candidate window left aligned to beginning of preedit string.
		SetCandidateWindowPos();
		pdoc->TentativeStart(); // TentativeActive from now on.

		std::vector<int> imeIndicator = MapImeIndicators(imc.GetImeAttributes());

		const int codePage = CodePageOfDocument();
		const std::wstring_view wsv = wcs;
		for (size_t i = 0; i < wsv.size(); ) {
			const size_t ucWidth = UTF16CharLength(wsv[i]);
			const std::string docChar = StringEncode(wsv.substr(i, ucWidth), codePage);

			InsertCharacter(docChar, CharacterSource::TentativeInput);

			DrawImeIndicator(imeIndicator[i], docChar.size());
			i += ucWidth;
		}

		// Japanese IME after pressing Tab replaces input string with first candidate item (target string);
		// when selecting other candidate item, previous item will be replaced with current one.
		// After candidate item been added, it's looks like been full selected, it's better to keep caret
		// at end of "selection" (end of input) instead of jump to beginning of input ("selection").
		const bool onlyTarget = std::all_of(imeIndicator.begin(), imeIndicator.end(), [](int i) noexcept {
			return i == IndicatorTarget;
		});
		if (!onlyTarget) {
			// CS_NOMOVECARET: keep caret at beginning of composition string which already moved in InsertCharacter().
			// GCS_CURSORPOS: current caret position is provided by IME.
			Sci::Position imeEndToImeCaretU16 = -static_cast<Sci::Position>(wcs.size());
			if (!(lParam & CS_NOMOVECARET) && (lParam & GCS_CURSORPOS)) {
				imeEndToImeCaretU16 += imc.GetImeCaretPos();
			}
			if (imeEndToImeCaretU16 != 0) {
				// Move back IME caret from current last position to imeCaretPos.
				const Sci::Position currentPos = CurrentPosition();
				const Sci::Position imeCaretPosDoc = pdoc->GetRelativePositionUTF16(currentPos, imeEndToImeCaretU16);

				MoveImeCarets(-currentPos + imeCaretPosDoc);

				if (std::find(imeIndicator.begin(), imeIndicator.end(), IndicatorTarget) != imeIndicator.end()) {
					// set candidate window left aligned to beginning of target string.
					SetCandidateWindowPos();
				}
			}
		}

		if (KoreanIME()) {
			view.imeCaretBlockOverride = true;
		}
	}
	EnsureCaretVisible();
	ShowCaretAtCurrentPosition();
	return 0;
}

namespace {

// Translate message IDs from WM_* and EM_* to Message::* so can partly emulate Windows Edit control
Message SciMessageFromEM(unsigned int iMessage) noexcept {
	switch (iMessage) {
	case EM_CANPASTE: return Message::CanPaste;
	case EM_CANUNDO: return Message::CanUndo;
	case EM_EMPTYUNDOBUFFER: return Message::EmptyUndoBuffer;
	case EM_FINDTEXTEX: return Message::FindText;
	case EM_FORMATRANGE: return Message::FormatRange;
	case EM_GETFIRSTVISIBLELINE: return Message::GetFirstVisibleLine;
	case EM_GETLINECOUNT: return Message::GetLineCount;
	case EM_GETSELTEXT: return Message::GetSelText;
	case EM_GETTEXTRANGE: return Message::GetTextRange;
	case EM_HIDESELECTION: return Message::HideSelection;
	case EM_LINEINDEX: return Message::PositionFromLine;
	case EM_LINESCROLL: return Message::LineScroll;
	case EM_REPLACESEL: return Message::ReplaceSel;
	case EM_SCROLLCARET: return Message::ScrollCaret;
	case EM_SETREADONLY: return Message::SetReadOnly;
	case WM_CLEAR: return Message::Clear;
	case WM_COPY: return Message::Copy;
	case WM_CUT: return Message::Cut;
	case WM_SETTEXT: return Message::SetText;
	case WM_PASTE: return Message::Paste;
	case WM_UNDO: return Message::Undo;
	}
	return static_cast<Message>(iMessage);
}

}

namespace Scintilla::Internal {

UINT CodePageFromCharSet(CharacterSet characterSet, UINT documentCodePage) noexcept {
	if (documentCodePage == CpUtf8) {
		return CpUtf8;
	}
	switch (characterSet) {
	case CharacterSet::Ansi: return 1252;
	
	// Cyrillic / Turkish or other languages cannot be shown in ANSI mode.
	// This fixes such problem. For more information about this fix, check:
	// https://github.com/notepad-plus-plus/notepad-plus-plus/issues/5671
	//	case CharacterSet::Default: return documentCodePage ? documentCodePage : 1252;
	case CharacterSet::Default: return documentCodePage;
	case CharacterSet::Baltic: return 1257;
	case CharacterSet::ChineseBig5: return 950;
	case CharacterSet::EastEurope: return 1250;
	case CharacterSet::GB2312: return 936;
	case CharacterSet::Greek: return 1253;
	case CharacterSet::Hangul: return 949;
	case CharacterSet::Mac: return 10000;
	case CharacterSet::Oem: return 437;
	case CharacterSet::Russian: return 1251;
	case CharacterSet::ShiftJis: return 932;
	case CharacterSet::Turkish: return 1254;
	case CharacterSet::Johab: return 1361;
	case CharacterSet::Hebrew: return 1255;
	case CharacterSet::Arabic: return 1256;
	case CharacterSet::Vietnamese: return 1258;
	case CharacterSet::Thai: return 874;
	case CharacterSet::Iso8859_15: return 28605;
	// Not supported
	case CharacterSet::Cyrillic: return documentCodePage;
	case CharacterSet::Symbol: return documentCodePage;
	default: break;
	}
	return documentCodePage;
}

}

UINT ScintillaWin::CodePageOfDocument() const noexcept {
	return CodePageFromCharSet(vs.styles[StyleDefault].characterSet, pdoc->dbcsCodePage);
}

std::string ScintillaWin::EncodeWString(std::wstring_view wsv) {
	if (IsUnicodeMode()) {
		const size_t len = UTF8Length(wsv);
		std::string putf(len, 0);
		UTF8FromUTF16(wsv, putf.data(), len);
		return putf;
	} else {
		// Not in Unicode mode so convert from Unicode to current Scintilla code page
		return StringEncode(wsv, CodePageOfDocument());
	}
}

sptr_t ScintillaWin::GetTextLength() {
	if (pdoc->dbcsCodePage == 0 || pdoc->dbcsCodePage == CpUtf8) {
		return pdoc->CountUTF16(0, pdoc->Length());
	} else {
		// Count the number of UTF-16 code units line by line
		const UINT cpSrc = CodePageOfDocument();
		const Sci::Line lines = pdoc->LinesTotal();
		Sci::Position codeUnits = 0;
		std::string lineBytes;
		for (Sci::Line line = 0; line < lines; line++) {
			const Sci::Position start = pdoc->LineStart(line);
			const Sci::Position width = pdoc->LineStart(line+1) - start;
			lineBytes.resize(width);
			pdoc->GetCharRange(lineBytes.data(), start, width);
			codeUnits += WideCharLenFromMultiByte(cpSrc, lineBytes);
		}
		return codeUnits;
	}
}

sptr_t ScintillaWin::GetText(uptr_t wParam, sptr_t lParam) {
	if (lParam == 0) {
		return GetTextLength();
	}
	if (wParam == 0) {
		return 0;
	}
	wchar_t *ptr = static_cast<wchar_t *>(PtrFromSPtr(lParam));
	if (pdoc->Length() == 0) {
		*ptr = L'\0';
		return 0;
	}
	const Sci::Position lengthWanted = wParam - 1;
	if (IsUnicodeMode()) {
		Sci::Position sizeRequestedRange = pdoc->GetRelativePositionUTF16(0, lengthWanted);
		if (sizeRequestedRange < 0) {
			// Requested more text than there is in the document.
			sizeRequestedRange = pdoc->Length();
		}
		std::string docBytes(sizeRequestedRange, '\0');
		pdoc->GetCharRange(&docBytes[0], 0, sizeRequestedRange);
		const size_t uLen = UTF16FromUTF8(docBytes, ptr, lengthWanted);
		ptr[uLen] = L'\0';
		return uLen;
	} else {
		// Not Unicode mode
		// Convert to Unicode using the current Scintilla code page
		// Retrieve as UTF-16 line by line
		const UINT cpSrc = CodePageOfDocument();
		const Sci::Line lines = pdoc->LinesTotal();
		Sci::Position codeUnits = 0;
		std::string lineBytes;
		std::wstring lineAsUTF16;
		for (Sci::Line line = 0; line < lines && codeUnits < lengthWanted; line++) {
			const Sci::Position start = pdoc->LineStart(line);
			const Sci::Position width = pdoc->LineStart(line + 1) - start;
			lineBytes.resize(width);
			pdoc->GetCharRange(lineBytes.data(), start, width);
			const Sci::Position codeUnitsLine = WideCharLenFromMultiByte(cpSrc, lineBytes);
			lineAsUTF16.resize(codeUnitsLine);
			const Sci::Position lengthLeft = lengthWanted - codeUnits;
			WideCharFromMultiByte(cpSrc, lineBytes, lineAsUTF16.data(), lineAsUTF16.length());
			const Sci::Position lengthToCopy = std::min(lengthLeft, codeUnitsLine);
			lineAsUTF16.copy(ptr + codeUnits, lengthToCopy);
			codeUnits += lengthToCopy;
		}
		ptr[codeUnits] = L'\0';
		return codeUnits;
	}
}

Window::Cursor ScintillaWin::ContextCursor(Point pt) {
	if (inDragDrop == DragDrop::dragging) {
		return Window::Cursor::up;
	} else {
		// Display regular (drag) cursor over selection
		if (PointInSelMargin(pt)) {
			return GetMarginCursor(pt);
		} else if (!SelectionEmpty() && PointInSelection(pt)) {
			return Window::Cursor::arrow;
		} else if (PointIsHotspot(pt)) {
			return Window::Cursor::hand;
		} else if (hoverIndicatorPos != Sci::invalidPosition) {
			const Sci::Position pos = PositionFromLocation(pt, true, true);
			if (pos != Sci::invalidPosition) {
				return Window::Cursor::hand;
			}
		}
	}
	return Window::Cursor::text;
}

sptr_t ScintillaWin::ShowContextMenu(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	Point pt = PointFromLParam(lParam);
	POINT rpt = POINTFromPoint(pt);
	::ScreenToClient(MainHWND(), &rpt);
	const Point ptClient = PointFromPOINT(rpt);
	if (ShouldDisplayPopup(ptClient)) {
		if ((pt.x == -1) && (pt.y == -1)) {
			// Caused by keyboard so display menu near caret
			pt = PointMainCaret();
			POINT spt = POINTFromPoint(pt);
			::ClientToScreen(MainHWND(), &spt);
			pt = PointFromPOINT(spt);
		}
		ContextMenu(pt);
		return 0;
	}
	return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
}

PRectangle ScintillaWin::GetClientRectangle() const {
	return rectangleClient;
}

void ScintillaWin::SizeWindow() {
#if defined(USE_D2D)
	if (paintState == PaintState::notPainting) {
		DropRenderTarget();
	} else {
		renderTargetValid = false;
	}
#endif
	//Platform::DebugPrintf("Scintilla WM_SIZE %d %d\n", LOWORD(lParam), HIWORD(lParam));
	rectangleClient = wMain.GetClientPosition();
	ChangeSize();
}

sptr_t ScintillaWin::MouseMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {
	case WM_LBUTTONDOWN: {
			// For IME, set the composition string as the result string.
			IMContext imc(MainHWND());
			if (imc.hIMC) {
				::ImmNotifyIME(imc.hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
			}
			//
			//Platform::DebugPrintf("Buttdown %d %x %x %x %x %x\n",iMessage, wParam, lParam,
			//	KeyboardIsKeyDown(VK_SHIFT),
			//	KeyboardIsKeyDown(VK_CONTROL),
			//	KeyboardIsKeyDown(VK_MENU));
			::SetFocus(MainHWND());
			ButtonDownWithModifiers(PointFromLParam(lParam), ::GetMessageTime(),
						MouseModifiers(wParam));
		}
		break;

	case WM_LBUTTONUP:
		ButtonUpWithModifiers(PointFromLParam(lParam),
				      ::GetMessageTime(), MouseModifiers(wParam));
		break;

	case WM_RBUTTONDOWN: {
			::SetFocus(MainHWND());
			const Point pt = PointFromLParam(lParam);
			if (!PointInSelection(pt)) {
				CancelModes();
				SetEmptySelection(PositionFromLocation(PointFromLParam(lParam)));
			}

			RightButtonDownWithModifiers(pt, ::GetMessageTime(), MouseModifiers(wParam));
		}
		break;

	case WM_MBUTTONDOWN:
		::SetFocus(MainHWND());
		break;

	case WM_MOUSEMOVE: {
			const Point pt = PointFromLParam(lParam);

			// Windows might send WM_MOUSEMOVE even though the mouse has not been moved:
			// http://blogs.msdn.com/b/oldnewthing/archive/2003/10/01/55108.aspx
			if (ptMouseLast != pt) {
				SetTrackMouseLeaveEvent(true);
				ButtonMoveWithModifiers(pt, ::GetMessageTime(), MouseModifiers(wParam));
			}
		}
		break;

	case WM_MOUSELEAVE:
		SetTrackMouseLeaveEvent(false);
		MouseLeave();
		return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

	case WM_MOUSEWHEEL:
		if (!mouseWheelCaptures) {
			// if the mouse wheel is not captured, test if the mouse
			// pointer is over the editor window and if not, don't
			// handle the message but pass it on.
			RECT rc;
			GetWindowRect(MainHWND(), &rc);
			const POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			if (!PtInRect(&rc, pt))
				return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
		}
		// if autocomplete list active then send mousewheel message to it
		if (ac.Active()) {
			HWND hWnd = HwndFromWindow(*(ac.lb));
			::SendMessage(hWnd, iMessage, wParam, lParam);
			break;
		}

		// Don't handle datazoom.
		// (A good idea for datazoom would be to "fold" or "unfold" details.
		// i.e. if datazoomed out only class structures are visible, when datazooming in the control
		// structures appear, then eventually the individual statements...)
		if (wParam & MK_SHIFT) {
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
		}
		// Either SCROLL or ZOOM. We handle the wheel steppings calculation
		wheelDelta -= GET_WHEEL_DELTA_WPARAM(wParam);
		if (std::abs(wheelDelta) >= WHEEL_DELTA && linesPerScroll > 0) {
			Sci::Line linesToScroll = linesPerScroll;
			if (linesPerScroll == WHEEL_PAGESCROLL)
				linesToScroll = LinesOnScreen() - 1;
			if (linesToScroll == 0) {
				linesToScroll = 1;
			}
			linesToScroll *= (wheelDelta / WHEEL_DELTA);
			if (wheelDelta >= 0)
				wheelDelta = wheelDelta % WHEEL_DELTA;
			else
				wheelDelta = -(-wheelDelta % WHEEL_DELTA);

			if (wParam & MK_CONTROL) {
				// Zoom! We play with the font sizes in the styles.
				// Number of steps/line is ignored, we just care if sizing up or down
				if (linesToScroll < 0) {
					KeyCommand(Message::ZoomIn);
				} else {
					KeyCommand(Message::ZoomOut);
				}
			} else {
				// Scroll
				ScrollTo(topLine + linesToScroll);
			}
		}
		return 0;
	}
	return 0;
}

sptr_t ScintillaWin::KeyMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN: {
			// Platform::DebugPrintf("Keydown %c %c%c%c%c %x %x\n",
			// iMessage == WM_KEYDOWN ? 'K' : 'S',
			// (lParam & (1 << 24)) ? 'E' : '-',
			// KeyboardIsKeyDown(VK_SHIFT) ? 'S' : '-',
			// KeyboardIsKeyDown(VK_CONTROL) ? 'C' : '-',
			// KeyboardIsKeyDown(VK_MENU) ? 'A' : '-',
			// wParam, lParam);
			lastKeyDownConsumed = false;
			const bool altDown = KeyboardIsKeyDown(VK_MENU);
			if (altDown && KeyboardIsNumericKeypadFunction(wParam, lParam)) {
				// Don't interpret these as they may be characters entered by number.
				return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
			}
			const int ret = KeyDownWithModifiers(
								 KeyTranslate(wParam),
							     ModifierFlags(KeyboardIsKeyDown(VK_SHIFT),
									     KeyboardIsKeyDown(VK_CONTROL),
									     altDown),
							     &lastKeyDownConsumed);
			if (!ret && !lastKeyDownConsumed) {
				return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
			}
			break;
		}

	case WM_KEYUP:
		//Platform::DebugPrintf("S keyup %d %x %x\n",iMessage, wParam, lParam);
		return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

	case WM_CHAR:
		if (((wParam >= 128) || !iscntrl(static_cast<int>(wParam))) || !lastKeyDownConsumed) {
			wchar_t wcs[3] = { static_cast<wchar_t>(wParam), 0 };
			unsigned int wclen = 1;
			if (IS_HIGH_SURROGATE(wcs[0])) {
				// If this is a high surrogate character, we need a second one
				lastHighSurrogateChar = wcs[0];
				return 0;
			} else if (IS_LOW_SURROGATE(wcs[0])) {
				wcs[1] = wcs[0];
				wcs[0] = lastHighSurrogateChar;
				lastHighSurrogateChar = 0;
				wclen = 2;
			}
			AddWString(std::wstring_view(wcs, wclen), CharacterSource::DirectInput);
		}
		return 0;

	case WM_UNICHAR:
		if (wParam == UNICODE_NOCHAR) {
			return TRUE;
		} else if (lastKeyDownConsumed) {
			return 1;
		} else {
			wchar_t wcs[3] = { 0 };
			const size_t wclen = UTF16FromUTF32Character(static_cast<unsigned int>(wParam), wcs);
			AddWString(std::wstring_view(wcs, wclen), CharacterSource::DirectInput);
			return FALSE;
		}
	}

	return 0;
}

sptr_t ScintillaWin::FocusMessage(unsigned int iMessage, uptr_t wParam, sptr_t) {
	switch (iMessage) {
	case WM_KILLFOCUS: {
		HWND wOther = reinterpret_cast<HWND>(wParam);
		HWND wThis = MainHWND();
		const HWND wCT = HwndFromWindow(ct.wCallTip);
		if (!wParam ||
			!(::IsChild(wThis, wOther) || (wOther == wCT))) {
			SetFocusState(false);
			DestroySystemCaret();
		}
		// Explicitly complete any IME composition
		IMContext imc(MainHWND());
		if (imc.hIMC) {
			::ImmNotifyIME(imc.hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
		}
		break;
	}

	case WM_SETFOCUS:
		SetFocusState(true);
		DestroySystemCaret();
		CreateSystemCaret();
		break;
	}
	return 0;
}

sptr_t ScintillaWin::IMEMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {

	case WM_INPUTLANGCHANGE:
		return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

	case WM_INPUTLANGCHANGEREQUEST:
		return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

	case WM_IME_KEYDOWN: {
			if (wParam == VK_HANJA) {
				ToggleHanja();
			}
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
		}

	case WM_IME_REQUEST: {
			if (wParam == IMR_RECONVERTSTRING) {
				return ImeOnReconvert(lParam);
			}
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
		}

	case WM_IME_STARTCOMPOSITION:
		if (KoreanIME() || imeInteraction == IMEInteraction::Inline) {
			return 0;
		} else {
			ImeStartComposition();
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
		}

	case WM_IME_ENDCOMPOSITION:
		ImeEndComposition();
		return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

	case WM_IME_COMPOSITION:
		if (KoreanIME() || imeInteraction == IMEInteraction::Inline) {
			return HandleCompositionInline(wParam, lParam);
		} else {
			return HandleCompositionWindowed(wParam, lParam);
		}

	case WM_IME_SETCONTEXT:
		if (KoreanIME() || imeInteraction == IMEInteraction::Inline) {
			if (wParam) {
				LPARAM NoImeWin = lParam;
				NoImeWin = NoImeWin & (~ISC_SHOWUICOMPOSITIONWINDOW);
				return ::DefWindowProc(MainHWND(), iMessage, wParam, NoImeWin);
			}
		}
		return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

	case WM_IME_NOTIFY:
		return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

	}
	return 0;
}

sptr_t ScintillaWin::EditMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {

	case EM_LINEFROMCHAR:
		if (PositionFromUPtr(wParam) < 0) {
			wParam = SelectionStart().Position();
		}
		return pdoc->LineFromPosition(wParam);

	case EM_EXLINEFROMCHAR:
		return pdoc->LineFromPosition(lParam);

	case EM_GETSEL:
		if (wParam) {
			*reinterpret_cast<DWORD *>(wParam) = static_cast<DWORD>(SelectionStart().Position());
		}
		if (lParam) {
			*reinterpret_cast<DWORD *>(lParam) = static_cast<DWORD>(SelectionEnd().Position());
		}
		return MAKELRESULT(SelectionStart().Position(), SelectionEnd().Position());

	case EM_EXGETSEL: {
			if (lParam == 0) {
				return 0;
			}
			CHARRANGE *pCR = reinterpret_cast<CHARRANGE *>(lParam);
			pCR->cpMin = static_cast<LONG>(SelectionStart().Position());
			pCR->cpMax = static_cast<LONG>(SelectionEnd().Position());
		}
		break;

	case EM_SETSEL: {
			Sci::Position nStart = wParam;
			Sci::Position nEnd = lParam;
			if (nStart == 0 && nEnd == -1) {
				nEnd = pdoc->Length();
			}
			if (nStart == -1) {
				nStart = nEnd;	// Remove selection
			}
			SetSelection(nEnd, nStart);
			EnsureCaretVisible();
		}
		break;

	case EM_EXSETSEL: {
			if (lParam == 0) {
				return 0;
			}
			const CHARRANGE *pCR = reinterpret_cast<const CHARRANGE *>(lParam);
			sel.selType = Selection::SelTypes::stream;
			if (pCR->cpMin == 0 && pCR->cpMax == -1) {
				SetSelection(pCR->cpMin, pdoc->Length());
			} else {
				SetSelection(pCR->cpMin, pCR->cpMax);
			}
			EnsureCaretVisible();
			return pdoc->LineFromPosition(SelectionStart().Position());
		}
	}
	return 0;
}

sptr_t ScintillaWin::IdleMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {
	case SC_WIN_IDLE:
		// wParam=dwTickCountInitial, or 0 to initialize.  lParam=bSkipUserInputTest
		if (idler.state) {
			if (lParam || (WAIT_TIMEOUT == MsgWaitForMultipleObjects(0, nullptr, 0, 0, QS_INPUT | QS_HOTKEY))) {
				if (Idle()) {
					// User input was given priority above, but all events do get a turn.  Other
					// messages, notifications, etc. will get interleaved with the idle messages.

					// However, some things like WM_PAINT are a lower priority, and will not fire
					// when there's a message posted.  So, several times a second, we stop and let
					// the low priority events have a turn (after which the timer will fire again).

					// Suppress a warning from Code Analysis that the GetTickCount function
					// wraps after 49 days. The WM_TIMER will kick off another SC_WIN_IDLE
					// after the wrap.
#ifdef _MSC_VER
#pragma warning(suppress: 28159)
#endif
					const DWORD dwCurrent = GetTickCount();
					const DWORD dwStart = wParam ? static_cast<DWORD>(wParam) : dwCurrent;
					constexpr DWORD maxWorkTime = 50;

					if (dwCurrent >= dwStart && dwCurrent > maxWorkTime &&dwCurrent - maxWorkTime < dwStart)
						PostMessage(MainHWND(), SC_WIN_IDLE, dwStart, 0);
				} else {
					SetIdle(false);
				}
			}
		}
		break;

	case SC_WORK_IDLE:
		IdleWork();
		break;
	}
	return 0;
}

sptr_t ScintillaWin::SciMessage(Message iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {
	case Message::GetDirectFunction:
		return reinterpret_cast<sptr_t>(DirectFunction);

	case Message::GetDirectStatusFunction:
		return reinterpret_cast<sptr_t>(DirectStatusFunction);

	case Message::GetDirectPointer:
		return reinterpret_cast<sptr_t>(this);

#ifdef SCI_OWNREGEX
	case Message::GetBoostRegexErrmsg:
	{
		// copies behavior of SCI_GETTEXT
		if (lParam == 0)
			return g_exceptionMessage.length() + 1;
		if (wParam == 0)
			return 0;
		char *ptr = CharPtrFromSPtr(lParam);
		const Sci_Position len = std::min<Sci_Position>(wParam - 1, g_exceptionMessage.length());
		strncpy (ptr, g_exceptionMessage.c_str(), len);
		ptr [len] = '\0';
		return len;
	}
#endif

	case Message::GrabFocus:
		::SetFocus(MainHWND());
		break;

#ifdef INCLUDE_DEPRECATED_FEATURES
	case Message::SETKEYSUNICODE:
		break;

	case Message::GETKEYSUNICODE:
		return true;
#endif

	case Message::SetTechnology:
		if (const Technology technologyNew = static_cast<Technology>(wParam);
			(technologyNew == Technology::Default) ||
			(technologyNew == Technology::DirectWriteRetain) ||
			(technologyNew == Technology::DirectWriteDC) ||
			(technologyNew == Technology::DirectWrite)) {
			if (technology != technologyNew) {
				if (technologyNew > Technology::Default) {
#if defined(USE_D2D)
					if (!LoadD2D()) {
						// Failed to load Direct2D or DirectWrite so no effect
						return 0;
					}
					UpdateRenderingParams(true);
#else
					return 0;
#endif
				} else {
					bidirectional = Bidirectional::Disabled;
				}
				DropRenderTarget();
				technology = technologyNew;
				view.bufferedDraw = technologyNew == Technology::Default;
				// Invalidate all cached information including layout.
				InvalidateStyleRedraw();
			}
		}
		break;

	case Message::SetBidirectional:
		if (technology == Technology::Default) {
			bidirectional = Bidirectional::Disabled;
		} else if (static_cast<Bidirectional>(wParam) <= Bidirectional::R2L) {
			bidirectional = static_cast<Bidirectional>(wParam);
		}
		// Invalidate all cached information including layout.
		InvalidateStyleRedraw();
		break;

	case Message::TargetAsUTF8:
		return TargetAsUTF8(CharPtrFromSPtr(lParam));

	case Message::EncodedFromUTF8:
		return EncodedFromUTF8(ConstCharPtrFromUPtr(wParam),
			CharPtrFromSPtr(lParam));

	default:
		break;

	}
	return 0;
}

sptr_t ScintillaWin::WndProc(Message iMessage, uptr_t wParam, sptr_t lParam) {
	try {
		//Platform::DebugPrintf("S M:%x WP:%x L:%x\n", iMessage, wParam, lParam);
		const unsigned int msg = static_cast<unsigned int>(iMessage);
		switch (msg) {

		case WM_CREATE:
			ctrlID = ::GetDlgCtrlID(HwndFromWindow(wMain));
			UpdateBaseElements();
			// Get Intellimouse scroll line parameters
			GetIntelliMouseParameters();
			::RegisterDragDrop(MainHWND(), reinterpret_cast<IDropTarget *>(&dt));
			break;

		case WM_COMMAND:
			Command(LOWORD(wParam));
			break;

		case WM_PAINT:
			return WndPaint();

		case WM_PRINTCLIENT: {
				HDC hdc = reinterpret_cast<HDC>(wParam);
				if (!IsCompatibleDC(hdc)) {
					return ::DefWindowProc(MainHWND(), msg, wParam, lParam);
				}
				FullPaintDC(hdc);
			}
			break;

		case WM_VSCROLL:
			ScrollMessage(wParam);
			break;

		case WM_HSCROLL:
			HorizontalScrollMessage(wParam);
			break;

		case WM_SIZE:
			SizeWindow();
			break;

		case WM_TIMER:
			if (wParam == idleTimerID && idler.state) {
				SendMessage(MainHWND(), SC_WIN_IDLE, 0, 1);
			} else {
				TickFor(static_cast<TickReason>(wParam - fineTimerStart));
			}
			break;

		case SC_WIN_IDLE:
		case SC_WORK_IDLE:
			return IdleMessage(msg, wParam, lParam);

		case WM_GETMINMAXINFO:
			return ::DefWindowProc(MainHWND(), msg, wParam, lParam);

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_MOUSEMOVE:
		case WM_MOUSELEAVE:
		case WM_MOUSEWHEEL:
			return MouseMessage(msg, wParam, lParam);

		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT) {
				POINT pt;
				if (::GetCursorPos(&pt)) {
					::ScreenToClient(MainHWND(), &pt);
					DisplayCursor(ContextCursor(PointFromPOINT(pt)));
				}
				return TRUE;
			} else {
				return ::DefWindowProc(MainHWND(), msg, wParam, lParam);
			}

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
		case WM_UNICHAR:
			return KeyMessage(msg, wParam, lParam);

		case WM_SETTINGCHANGE:
			//Platform::DebugPrintf("Setting Changed\n");
#if defined(USE_D2D)
			if (technology != Technology::Default) {
				UpdateRenderingParams(true);
			}
#endif
			UpdateBaseElements();
			// Get Intellimouse scroll line parameters
			GetIntelliMouseParameters();
			InvalidateStyleRedraw();
			break;

		case WM_GETDLGCODE:
			return DLGC_HASSETSEL | DLGC_WANTALLKEYS;

		case WM_KILLFOCUS:
		case WM_SETFOCUS:
			return FocusMessage(msg, wParam, lParam);

		case WM_SYSCOLORCHANGE:
			//Platform::DebugPrintf("Setting Changed\n");
			UpdateBaseElements();
			InvalidateStyleData();
			break;

		case WM_DPICHANGED:
			dpi = HIWORD(wParam);
			InvalidateStyleRedraw();
			break;

		case WM_DPICHANGED_AFTERPARENT: {
				const UINT dpiNow = DpiForWindow(wMain.GetID());
				if (dpi != dpiNow) {
					dpi = dpiNow;
					InvalidateStyleRedraw();
				}
			}
			break;

		case WM_CONTEXTMENU:
			return ShowContextMenu(msg, wParam, lParam);

		case WM_ERASEBKGND:
			return 1;   // Avoid any background erasure as whole window painted.

		case WM_SETREDRAW:
			::DefWindowProc(MainHWND(), msg, wParam, lParam);
			if (wParam) {
				SetScrollBars();
				SetVerticalScrollPos();
				SetHorizontalScrollPos();
			}
			return 0;

		case WM_CAPTURECHANGED:
			capturedMouse = false;
			return 0;

		// These are not handled in Scintilla and its faster to dispatch them here.
		// Also moves time out to here so profile doesn't count lots of empty message calls.

		case WM_MOVE:
		case WM_MOUSEACTIVATE:
		case WM_NCHITTEST:
		case WM_NCCALCSIZE:
		case WM_NCPAINT:
		case WM_NCMOUSEMOVE:
		case WM_NCLBUTTONDOWN:
		case WM_SYSCOMMAND:
		case WM_WINDOWPOSCHANGING:
			return ::DefWindowProc(MainHWND(), msg, wParam, lParam);

		case WM_WINDOWPOSCHANGED:
#if defined(USE_D2D)
			if (technology != Technology::Default) {
				if (UpdateRenderingParams(false)) {
					DropGraphics();
					Redraw();
				}
			}
#endif
			return ::DefWindowProc(MainHWND(), msg, wParam, lParam);

		case WM_GETTEXTLENGTH:
			return GetTextLength();

		case WM_GETTEXT:
			return GetText(wParam, lParam);

		case WM_INPUTLANGCHANGE:
		case WM_INPUTLANGCHANGEREQUEST:
		case WM_IME_KEYDOWN:
		case WM_IME_REQUEST:
		case WM_IME_STARTCOMPOSITION:
		case WM_IME_ENDCOMPOSITION:
		case WM_IME_COMPOSITION:
		case WM_IME_SETCONTEXT:
		case WM_IME_NOTIFY:
			return IMEMessage(msg, wParam, lParam);

		case EM_LINEFROMCHAR:
		case EM_EXLINEFROMCHAR:
		case EM_GETSEL:
		case EM_EXGETSEL:
		case EM_SETSEL:
		case EM_EXSETSEL:
			return EditMessage(msg, wParam, lParam);
		}

		iMessage = SciMessageFromEM(msg);
		switch (iMessage) {
		case Message::GetDirectFunction:
		case Message::GetDirectStatusFunction:
		case Message::GetDirectPointer:
#ifdef SCI_OWNREGEX
		case Message::GetBoostRegexErrmsg:
#endif
		case Message::GrabFocus:
		case Message::SetTechnology:
		case Message::SetBidirectional:
		case Message::TargetAsUTF8:
		case Message::EncodedFromUTF8:
			return SciMessage(iMessage, wParam, lParam);

		default:
			return ScintillaBase::WndProc(iMessage, wParam, lParam);
		}
	} catch (std::bad_alloc &) {
		errorStatus = Status::BadAlloc;
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return 0;
}

bool ScintillaWin::ValidCodePage(int codePage) const {
	return codePage == 0 || codePage == CpUtf8 ||
	       codePage == 932 || codePage == 936 || codePage == 949 ||
	       codePage == 950 || codePage == 1361;
}

std::string ScintillaWin::UTF8FromEncoded(std::string_view encoded) const {
	if (IsUnicodeMode()) {
		return std::string(encoded);
	} else {
		// Pivot through wide string
		std::wstring ws = StringDecode(encoded, CodePageOfDocument());
		return StringEncode(ws, CpUtf8);
	}
}

std::string ScintillaWin::EncodedFromUTF8(std::string_view utf8) const {
	if (IsUnicodeMode()) {
		return std::string(utf8);
	} else {
		// Pivot through wide string
		std::wstring ws = StringDecode(utf8, CpUtf8);
		return StringEncode(ws, CodePageOfDocument());
	}
}

sptr_t ScintillaWin::DefWndProc(Message iMessage, uptr_t wParam, sptr_t lParam) {
	return ::DefWindowProc(MainHWND(), static_cast<unsigned int>(iMessage), wParam, lParam);
}

bool ScintillaWin::FineTickerRunning(TickReason reason) {
	return timers[static_cast<size_t>(reason)] != 0;
}

void ScintillaWin::FineTickerStart(TickReason reason, int millis, int tolerance) {
	FineTickerCancel(reason);
	const UINT_PTR reasonIndex = static_cast<UINT_PTR>(reason);
	const UINT_PTR eventID = static_cast<UINT_PTR>(fineTimerStart) + reasonIndex;
	if (SetCoalescableTimerFn && tolerance) {
		timers[reasonIndex] = SetCoalescableTimerFn(MainHWND(), eventID, millis, nullptr, tolerance);
	} else {
		timers[reasonIndex] = ::SetTimer(MainHWND(), eventID, millis, nullptr);
	}
}

void ScintillaWin::FineTickerCancel(TickReason reason) {
	const UINT_PTR reasonIndex = static_cast<UINT_PTR>(reason);
	if (timers[reasonIndex]) {
		::KillTimer(MainHWND(), timers[reasonIndex]);
		timers[reasonIndex] = 0;
	}
}


bool ScintillaWin::SetIdle(bool on) {
	// On Win32 the Idler is implemented as a Timer on the Scintilla window.  This
	// takes advantage of the fact that WM_TIMER messages are very low priority,
	// and are only posted when the message queue is empty, i.e. during idle time.
	if (idler.state != on) {
		if (on) {
			idler.idlerID = ::SetTimer(MainHWND(), idleTimerID, 10, nullptr)
				? reinterpret_cast<IdlerID>(idleTimerID) : 0;
		} else {
			::KillTimer(MainHWND(), reinterpret_cast<uptr_t>(idler.idlerID));
			idler.idlerID = 0;
		}
		idler.state = idler.idlerID != 0;
	}
	return idler.state;
}

void ScintillaWin::IdleWork() {
	styleIdleInQueue = false;
	Editor::IdleWork();
}

void ScintillaWin::QueueIdleWork(WorkItems items, Sci::Position upTo) {
	Editor::QueueIdleWork(items, upTo);
	if (!styleIdleInQueue) {
		if (PostMessage(MainHWND(), SC_WORK_IDLE, 0, 0)) {
			styleIdleInQueue = true;
		}
	}
}

void ScintillaWin::SetMouseCapture(bool on) {
	if (mouseDownCaptures) {
		if (on) {
			::SetCapture(MainHWND());
		} else {
			::ReleaseCapture();
		}
	}
	capturedMouse = on;
}

bool ScintillaWin::HaveMouseCapture() {
	// Cannot just see if GetCapture is this window as the scroll bar also sets capture for the window
	return capturedMouse;
	//return capturedMouse && (::GetCapture() == MainHWND());
}

void ScintillaWin::SetTrackMouseLeaveEvent(bool on) noexcept {
	if (on && !trackedMouseLeave) {
		TRACKMOUSEEVENT tme {};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = MainHWND();
		tme.dwHoverTime = HOVER_DEFAULT;	// Unused but triggers Dr. Memory if not initialized
		TrackMouseEvent(&tme);
	}
	trackedMouseLeave = on;
}

void ScintillaWin::UpdateBaseElements() {
	struct ElementToIndex { Element element; int nIndex; };
	ElementToIndex eti[] = {
		{ Element::List, COLOR_WINDOWTEXT },
		{ Element::ListBack, COLOR_WINDOW },
		{ Element::ListSelected, COLOR_HIGHLIGHTTEXT },
		{ Element::ListSelectedBack, COLOR_HIGHLIGHT },
	};
	bool changed = false;
	for (const ElementToIndex &ei : eti) {
		changed = vs.SetElementBase(ei.element, ColourRGBA::FromRGB(static_cast<int>(::GetSysColor(ei.nIndex)))) || changed;
	}
	if (changed) {
		Redraw();
	}
}

bool ScintillaWin::PaintContains(PRectangle rc) {
	if (paintState == PaintState::painting) {
		return BoundsContains(rcPaint, hRgnUpdate, rc);
	}
	return true;
}

void ScintillaWin::ScrollText(Sci::Line /* linesToMove */) {
	//Platform::DebugPrintf("ScintillaWin::ScrollText %d\n", linesToMove);
	//::ScrollWindow(MainHWND(), 0,
	//	vs.lineHeight * linesToMove, 0, 0);
	//::UpdateWindow(MainHWND());
	Redraw();
	UpdateSystemCaret();
}

void ScintillaWin::NotifyCaretMove() {
	NotifyWinEvent(EVENT_OBJECT_LOCATIONCHANGE, MainHWND(), OBJID_CARET, CHILDID_SELF);
}

void ScintillaWin::UpdateSystemCaret() {
	if (hasFocus) {
		if (pdoc->TentativeActive()) {
			// ongoing inline mode IME composition, don't inform IME of system caret position.
			// fix candidate window for Google Japanese IME moved on typing on Win7.
			return;
		}
		if (HasCaretSizeChanged()) {
			DestroySystemCaret();
			CreateSystemCaret();
		}
		const Point pos = PointMainCaret();
		::SetCaretPos(static_cast<int>(pos.x), static_cast<int>(pos.y));
	}
}

bool ScintillaWin::IsVisible() const noexcept {
	return GetWindowStyle(MainHWND()) & WS_VISIBLE;
}

int ScintillaWin::SetScrollInfo(int nBar, LPCSCROLLINFO lpsi, BOOL bRedraw) noexcept {
	return ::SetScrollInfo(MainHWND(), nBar, lpsi, bRedraw);
}

bool ScintillaWin::GetScrollInfo(int nBar, LPSCROLLINFO lpsi) noexcept {
	return ::GetScrollInfo(MainHWND(), nBar, lpsi) ? true : false;
}

// Change the scroll position but avoid repaint if changing to same value
void ScintillaWin::ChangeScrollPos(int barType, Sci::Position pos) {
	if (!IsVisible()) {
		return;
	}

	SCROLLINFO sci = {
		sizeof(sci), 0, 0, 0, 0, 0, 0
	};
	sci.fMask = SIF_POS;
	GetScrollInfo(barType, &sci);
	if (sci.nPos != pos) {
		DwellEnd(true);
		sci.nPos = static_cast<int>(pos);
		SetScrollInfo(barType, &sci, TRUE);
	}
}

void ScintillaWin::SetVerticalScrollPos() {
	ChangeScrollPos(SB_VERT, topLine);
}

void ScintillaWin::SetHorizontalScrollPos() {
	ChangeScrollPos(SB_HORZ, xOffset);
}

bool ScintillaWin::ChangeScrollRange(int nBar, int nMin, int nMax, UINT nPage) noexcept {
	SCROLLINFO sci = { sizeof(sci), SIF_PAGE | SIF_RANGE, 0, 0, 0, 0, 0 };
	GetScrollInfo(nBar, &sci);
	if ((sci.nMin != nMin) || (sci.nMax != nMax) ||	(sci.nPage != nPage)) {
		sci.nMin = nMin;
		sci.nMax = nMax;
		sci.nPage = nPage;
		SetScrollInfo(nBar, &sci, TRUE);
		return true;
	}
	return false;
}

bool ScintillaWin::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) {
	if (!IsVisible()) {
		return false;
	}

	bool modified = false;
	const Sci::Line vertEndPreferred = nMax;
	if (!verticalScrollBarVisible)
		nPage = vertEndPreferred + 1;
	if (ChangeScrollRange(SB_VERT, 0, static_cast<int>(vertEndPreferred), static_cast<unsigned int>(nPage))) {
		modified = true;
	}

	const PRectangle rcText = GetTextRectangle();
	int pageWidth = static_cast<int>(rcText.Width());
	const int horizEndPreferred = std::max({scrollWidth, pageWidth - 1, 0});
	if (!horizontalScrollBarVisible || Wrapping())
		pageWidth = horizEndPreferred + 1;
	if (ChangeScrollRange(SB_HORZ, 0, horizEndPreferred, pageWidth)) {
		modified = true;
		if (scrollWidth < pageWidth) {
			HorizontalScrollTo(0);
		}
	}

	return modified;
}

void ScintillaWin::NotifyChange() {
	::SendMessage(::GetParent(MainHWND()), WM_COMMAND,
	        MAKEWPARAM(GetCtrlID(), FocusChange::Change),
		reinterpret_cast<LPARAM>(MainHWND()));
}

void ScintillaWin::NotifyFocus(bool focus) {
	if (commandEvents) {
		::SendMessage(::GetParent(MainHWND()), WM_COMMAND,
			MAKEWPARAM(GetCtrlID(), focus ? FocusChange::Setfocus : FocusChange::Killfocus),
			reinterpret_cast<LPARAM>(MainHWND()));
	}
	Editor::NotifyFocus(focus);
}

void ScintillaWin::SetCtrlID(int identifier) {
	::SetWindowID(HwndFromWindow(wMain), identifier);
}

int ScintillaWin::GetCtrlID() {
	return ::GetDlgCtrlID(HwndFromWindow(wMain));
}

void ScintillaWin::NotifyParent(NotificationData scn) {
	scn.nmhdr.hwndFrom = MainHWND();
	scn.nmhdr.idFrom = GetCtrlID();
	::SendMessage(::GetParent(MainHWND()), WM_NOTIFY,
	              GetCtrlID(), reinterpret_cast<LPARAM>(&scn));
}

void ScintillaWin::NotifyDoubleClick(Point pt, KeyMod modifiers) {
	//Platform::DebugPrintf("ScintillaWin Double click 0\n");
	ScintillaBase::NotifyDoubleClick(pt, modifiers);
	// Send myself a WM_LBUTTONDBLCLK, so the container can handle it too.
	::SendMessage(MainHWND(),
			  WM_LBUTTONDBLCLK,
			  FlagSet(modifiers, KeyMod::Shift) ? MK_SHIFT : 0,
			  MAKELPARAM(pt.x, pt.y));
}

namespace {

class CaseFolderDBCS : public CaseFolderTable {
	// Allocate the expandable storage here so that it does not need to be reallocated
	// for each call to Fold.
	std::vector<wchar_t> utf16Mixed;
	std::vector<wchar_t> utf16Folded;
	UINT cp;
public:
	explicit CaseFolderDBCS(UINT cp_) : cp(cp_) {
	}
	size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) override {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else {
			if (lenMixed > utf16Mixed.size()) {
				utf16Mixed.resize(lenMixed + 8);
			}
			const size_t nUtf16Mixed = WideCharFromMultiByte(cp,
				std::string_view(mixed, lenMixed),
				&utf16Mixed[0],
				utf16Mixed.size());

			if (nUtf16Mixed == 0) {
				// Failed to convert -> bad input
				folded[0] = '\0';
				return 1;
			}

			size_t lenFlat = 0;
			for (size_t mixIndex=0; mixIndex < nUtf16Mixed; mixIndex++) {
				if ((lenFlat + 20) > utf16Folded.size())
					utf16Folded.resize(lenFlat + 60);
				const char *foldedUTF8 = CaseConvert(utf16Mixed[mixIndex], CaseConversion::fold);
				if (foldedUTF8) {
					// Maximum length of a case conversion is 6 bytes, 3 characters
					wchar_t wFolded[20];
					const size_t charsConverted = UTF16FromUTF8(std::string_view(foldedUTF8),
							wFolded, std::size(wFolded));
					for (size_t j=0; j<charsConverted; j++)
						utf16Folded[lenFlat++] = wFolded[j];
				} else {
					utf16Folded[lenFlat++] = utf16Mixed[mixIndex];
				}
			}

			const std::wstring_view wsvFolded(&utf16Folded[0], lenFlat);
			const size_t lenOut = MultiByteLenFromWideChar(cp, wsvFolded);

			if (lenOut < sizeFolded) {
				MultiByteFromWideChar(cp, wsvFolded, folded, lenOut);
				return lenOut;
			} else {
				return 0;
			}
		}
	}
};

}

std::unique_ptr<CaseFolder> ScintillaWin::CaseFolderForEncoding() {
	const UINT cpDest = CodePageOfDocument();
	if (cpDest == CpUtf8) {
		return std::make_unique<CaseFolderUnicode>();
	} else {
		if (pdoc->dbcsCodePage == 0) {
			std::unique_ptr<CaseFolderTable> pcf = std::make_unique<CaseFolderTable>();
			// Only for single byte encodings
			for (int i=0x80; i<0x100; i++) {
				char sCharacter[2] = "A";
				sCharacter[0] = static_cast<char>(i);
				wchar_t wCharacter[20];
				const unsigned int lengthUTF16 = WideCharFromMultiByte(cpDest, sCharacter,
					wCharacter, std::size(wCharacter));
				if (lengthUTF16 == 1) {
					const char *caseFolded = CaseConvert(wCharacter[0], CaseConversion::fold);
					if (caseFolded) {
						wchar_t wLower[20];
						const size_t charsConverted = UTF16FromUTF8(std::string_view(caseFolded),
							wLower, std::size(wLower));
						if (charsConverted == 1) {
							char sCharacterLowered[20];
							const unsigned int lengthConverted = MultiByteFromWideChar(cpDest,
								std::wstring_view(wLower, charsConverted),
								sCharacterLowered, std::size(sCharacterLowered));
							if ((lengthConverted == 1) && (sCharacter[0] != sCharacterLowered[0])) {
								pcf->SetTranslation(sCharacter[0], sCharacterLowered[0]);
							}
						}
					}
				}
			}
			return pcf;
		} else {
			return std::make_unique<CaseFolderDBCS>(cpDest);
		}
	}
}

std::string ScintillaWin::CaseMapString(const std::string &s, CaseMapping caseMapping) {
	if ((s.size() == 0) || (caseMapping == CaseMapping::same))
		return s;

	const UINT cpDoc = CodePageOfDocument();
	if (cpDoc == CpUtf8) {
		return CaseConvertString(s, (caseMapping == CaseMapping::upper) ? CaseConversion::upper : CaseConversion::lower);
	}

	// Change text to UTF-16
	const std::wstring wsText = StringDecode(s, cpDoc);

	const DWORD mapFlags = LCMAP_LINGUISTIC_CASING |
		((caseMapping == CaseMapping::upper) ? LCMAP_UPPERCASE : LCMAP_LOWERCASE);

	// Change case
	const std::wstring wsConverted = StringMapCase(wsText, mapFlags);

	// Change back to document encoding
	std::string sConverted = StringEncode(wsConverted, cpDoc);

	return sConverted;
}

void ScintillaWin::Copy() {
	//Platform::DebugPrintf("Copy\n");
	if (!sel.Empty()) {
		SelectionText selectedText;
		CopySelectionRange(&selectedText);
		CopyToClipboard(selectedText);
	}
}

bool ScintillaWin::CanPaste() {
	if (!Editor::CanPaste())
		return false;
	return ::IsClipboardFormatAvailable(CF_UNICODETEXT) != FALSE;
}

namespace {

class GlobalMemory {
	HGLOBAL hand {};
public:
	void *ptr {};
	GlobalMemory() noexcept {
	}
	explicit GlobalMemory(HGLOBAL hand_) noexcept : hand(hand_) {
		if (hand) {
			ptr = ::GlobalLock(hand);
		}
	}
	// Deleted so GlobalMemory objects can not be copied.
	GlobalMemory(const GlobalMemory &) = delete;
	GlobalMemory(GlobalMemory &&) = delete;
	GlobalMemory &operator=(const GlobalMemory &) = delete;
	GlobalMemory &operator=(GlobalMemory &&) = delete;
	~GlobalMemory() {
		assert(!ptr);
		assert(!hand);
	}
	void Allocate(size_t bytes) noexcept {
		assert(!hand);
		hand = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes);
		if (hand) {
			ptr = ::GlobalLock(hand);
		}
	}
	HGLOBAL Unlock() noexcept {
		assert(ptr);
		HGLOBAL handCopy = hand;
		::GlobalUnlock(hand);
		ptr = nullptr;
		hand = {};
		return handCopy;
	}
	void SetClip(UINT uFormat) noexcept {
		::SetClipboardData(uFormat, Unlock());
	}
	operator bool() const noexcept {
		return ptr != nullptr;
	}
	SIZE_T Size() const noexcept {
		return ::GlobalSize(hand);
	}
};

// OpenClipboard may fail if another application has opened the clipboard.
// Try up to 8 times, with an initial delay of 1 ms and an exponential back off
// for a maximum total delay of 127 ms (1+2+4+8+16+32+64).
bool OpenClipboardRetry(HWND hwnd) noexcept {
	for (int attempt=0; attempt<8; attempt++) {
		if (attempt > 0) {
			::Sleep(1 << (attempt-1));
		}
		if (::OpenClipboard(hwnd)) {
			return true;
		}
	}
	return false;
}

bool IsValidFormatEtc(const FORMATETC *pFE) noexcept {
	return pFE->ptd == nullptr &&
		(pFE->dwAspect & DVASPECT_CONTENT) != 0 &&
		pFE->lindex == -1 &&
		(pFE->tymed & TYMED_HGLOBAL) != 0;
}

bool SupportedFormat(const FORMATETC *pFE) noexcept {
	return pFE->cfFormat == CF_UNICODETEXT &&
		IsValidFormatEtc(pFE);
}

}

void ScintillaWin::Paste() {
	if (!::OpenClipboardRetry(MainHWND())) {
		return;
	}
	UndoGroup ug(pdoc);
	const bool isLine = SelectionEmpty() &&
		(::IsClipboardFormatAvailable(cfLineSelect) || ::IsClipboardFormatAvailable(cfVSLineTag));
	ClearSelection(multiPasteMode == MultiPaste::Each);
	bool isRectangular = (::IsClipboardFormatAvailable(cfColumnSelect) != 0);

	if (!isRectangular) {
		// Evaluate "Borland IDE Block Type" explicitly
		GlobalMemory memBorlandSelection(::GetClipboardData(cfBorlandIDEBlockType));
		if (memBorlandSelection) {
			isRectangular = (memBorlandSelection.Size() == 1) && (static_cast<BYTE *>(memBorlandSelection.ptr)[0] == 0x02);
			memBorlandSelection.Unlock();
		}
	}
	const PasteShape pasteShape = isRectangular ? PasteShape::rectangular : (isLine ? PasteShape::line : PasteShape::stream);

	// Use CF_UNICODETEXT if available
	GlobalMemory memUSelection(::GetClipboardData(CF_UNICODETEXT));
	if (const wchar_t *uptr = static_cast<const wchar_t *>(memUSelection.ptr)) {
		const std::string putf = EncodeWString(uptr);
		InsertPasteShape(putf.c_str(), putf.length(), pasteShape);
		memUSelection.Unlock();
	}
	::CloseClipboard();
	Redraw();
}

void ScintillaWin::CreateCallTipWindow(PRectangle) {
	if (!ct.wCallTip.Created()) {
		HWND wnd = ::CreateWindow(callClassName, TEXT("ACallTip"),
					     WS_POPUP, 100, 100, 150, 20,
					     MainHWND(), 0,
					     GetWindowInstance(MainHWND()),
					     this);
		ct.wCallTip = wnd;
		ct.wDraw = wnd;
	}
}

void ScintillaWin::AddToPopUp(const char *label, int cmd, bool enabled) {
	HMENU hmenuPopup = static_cast<HMENU>(popup.GetID());
	if (!label[0])
		::AppendMenuA(hmenuPopup, MF_SEPARATOR, 0, "");
	else if (enabled)
		::AppendMenuA(hmenuPopup, MF_STRING, cmd, label);
	else
		::AppendMenuA(hmenuPopup, MF_STRING | MF_DISABLED | MF_GRAYED, cmd, label);
}

void ScintillaWin::ClaimSelection() {
	// Windows does not have a primary selection
}

/// Implement IUnknown

STDMETHODIMP_(ULONG)FormatEnumerator_AddRef(FormatEnumerator *fe);
STDMETHODIMP FormatEnumerator_QueryInterface(FormatEnumerator *fe, REFIID riid, PVOID *ppv) {
	//Platform::DebugPrintf("EFE QI");
	*ppv = nullptr;
	if (riid == IID_IUnknown)
		*ppv = reinterpret_cast<IEnumFORMATETC *>(fe);
	if (riid == IID_IEnumFORMATETC)
		*ppv = reinterpret_cast<IEnumFORMATETC *>(fe);
	if (!*ppv)
		return E_NOINTERFACE;
	FormatEnumerator_AddRef(fe);
	return S_OK;
}
STDMETHODIMP_(ULONG)FormatEnumerator_AddRef(FormatEnumerator *fe) {
	return ++fe->ref;
}
STDMETHODIMP_(ULONG)FormatEnumerator_Release(FormatEnumerator *fe) {
	fe->ref--;
	if (fe->ref > 0)
		return fe->ref;
	delete fe;
	return 0;
}
/// Implement IEnumFORMATETC
STDMETHODIMP FormatEnumerator_Next(FormatEnumerator *fe, ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched) {
	if (!rgelt) return E_POINTER;
	ULONG putPos = 0;
	while ((fe->pos < fe->formats.size()) && (putPos < celt)) {
		rgelt->cfFormat = fe->formats[fe->pos];
		rgelt->ptd = nullptr;
		rgelt->dwAspect = DVASPECT_CONTENT;
		rgelt->lindex = -1;
		rgelt->tymed = TYMED_HGLOBAL;
		rgelt++;
		fe->pos++;
		putPos++;
	}
	if (pceltFetched)
		*pceltFetched = putPos;
	return putPos ? S_OK : S_FALSE;
}
STDMETHODIMP FormatEnumerator_Skip(FormatEnumerator *fe, ULONG celt) {
	fe->pos += celt;
	return S_OK;
}
STDMETHODIMP FormatEnumerator_Reset(FormatEnumerator *fe) {
	fe->pos = 0;
	return S_OK;
}
STDMETHODIMP FormatEnumerator_Clone(FormatEnumerator *fe, IEnumFORMATETC **ppenum) {
	FormatEnumerator *pfe;
	try {
		pfe = new FormatEnumerator(fe->pos, &fe->formats[0], fe->formats.size());
	} catch (...) {
		return E_OUTOFMEMORY;
	}
	return FormatEnumerator_QueryInterface(pfe, IID_IEnumFORMATETC,
	                                       reinterpret_cast<void **>(ppenum));
}

static VFunction *vtFormatEnumerator[] = {
	(VFunction *)(FormatEnumerator_QueryInterface),
	(VFunction *)(FormatEnumerator_AddRef),
	(VFunction *)(FormatEnumerator_Release),
	(VFunction *)(FormatEnumerator_Next),
	(VFunction *)(FormatEnumerator_Skip),
	(VFunction *)(FormatEnumerator_Reset),
	(VFunction *)(FormatEnumerator_Clone)
};

FormatEnumerator::FormatEnumerator(ULONG pos_, const CLIPFORMAT formats_[], size_t formatsLen_) {
	vtbl = vtFormatEnumerator;
	ref = 0;   // First QI adds first reference...
	pos = pos_;
	formats.insert(formats.begin(), formats_, formats_+formatsLen_);
}

/// Implement IUnknown
STDMETHODIMP DropSource_QueryInterface(DropSource *ds, REFIID riid, PVOID *ppv) {
	return ds->sci->QueryInterface(riid, ppv);
}
STDMETHODIMP_(ULONG)DropSource_AddRef(DropSource *ds) {
	return ds->sci->AddRef();
}
STDMETHODIMP_(ULONG)DropSource_Release(DropSource *ds) {
	return ds->sci->Release();
}

/// Implement IDropSource
STDMETHODIMP DropSource_QueryContinueDrag(DropSource *, BOOL fEsc, DWORD grfKeyState) {
	if (fEsc)
		return DRAGDROP_S_CANCEL;
	if (!(grfKeyState & MK_LBUTTON))
		return DRAGDROP_S_DROP;
	return S_OK;
}

STDMETHODIMP DropSource_GiveFeedback(DropSource *, DWORD) {
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

static VFunction *vtDropSource[] = {
	(VFunction *)(DropSource_QueryInterface),
	(VFunction *)(DropSource_AddRef),
	(VFunction *)(DropSource_Release),
	(VFunction *)(DropSource_QueryContinueDrag),
	(VFunction *)(DropSource_GiveFeedback)
};

DropSource::DropSource() noexcept {
	vtbl = vtDropSource;
	sci = nullptr;
}

/// Implement IUnkown
STDMETHODIMP DataObject_QueryInterface(DataObject *pd, REFIID riid, PVOID *ppv) {
	//Platform::DebugPrintf("DO QI %x\n", pd);
	return pd->sci->QueryInterface(riid, ppv);
}
STDMETHODIMP_(ULONG)DataObject_AddRef(DataObject *pd) {
	return pd->sci->AddRef();
}
STDMETHODIMP_(ULONG)DataObject_Release(DataObject *pd) {
	return pd->sci->Release();
}
/// Implement IDataObject
STDMETHODIMP DataObject_GetData(DataObject *pd, FORMATETC *pFEIn, STGMEDIUM *pSTM) {
	return pd->sci->GetData(pFEIn, pSTM);
}

STDMETHODIMP DataObject_GetDataHere(DataObject *, FORMATETC *, STGMEDIUM *) {
	//Platform::DebugPrintf("DOB GetDataHere\n");
	return E_NOTIMPL;
}

STDMETHODIMP DataObject_QueryGetData(DataObject *pd, FORMATETC *pFE) {
	if (pd->sci->DragIsRectangularOK(pFE->cfFormat) && IsValidFormatEtc(pFE)) {
		return S_OK;
	}

	if (SupportedFormat(pFE)) {
		return S_OK;
	} else {
		return S_FALSE;
	}
}

STDMETHODIMP DataObject_GetCanonicalFormatEtc(DataObject *, FORMATETC *, FORMATETC *pFEOut) {
	//Platform::DebugPrintf("DOB GetCanon\n");
	pFEOut->cfFormat = CF_UNICODETEXT;
	pFEOut->ptd = nullptr;
	pFEOut->dwAspect = DVASPECT_CONTENT;
	pFEOut->lindex = -1;
	pFEOut->tymed = TYMED_HGLOBAL;
	return S_OK;
}

STDMETHODIMP DataObject_SetData(DataObject *, FORMATETC *, STGMEDIUM *, BOOL) {
	//Platform::DebugPrintf("DOB SetData\n");
	return E_FAIL;
}

STDMETHODIMP DataObject_EnumFormatEtc(DataObject *pd, DWORD dwDirection, IEnumFORMATETC **ppEnum) {
	try {
		//Platform::DebugPrintf("DOB EnumFormatEtc %d\n", dwDirection);
		if (dwDirection != DATADIR_GET) {
			*ppEnum = nullptr;
			return E_FAIL;
		}

		const CLIPFORMAT formats[] = {CF_UNICODETEXT};
		FormatEnumerator *pfe = new FormatEnumerator(0, formats, std::size(formats));
		return FormatEnumerator_QueryInterface(pfe, IID_IEnumFORMATETC,
											   reinterpret_cast<void **>(ppEnum));
	} catch (std::bad_alloc &) {
		pd->sci->errorStatus = Status::BadAlloc;
		return E_OUTOFMEMORY;
	} catch (...) {
		pd->sci->errorStatus = Status::Failure;
		return E_FAIL;
	}
}

STDMETHODIMP DataObject_DAdvise(DataObject *, FORMATETC *, DWORD, IAdviseSink *, PDWORD) {
	//Platform::DebugPrintf("DOB DAdvise\n");
	return E_FAIL;
}

STDMETHODIMP DataObject_DUnadvise(DataObject *, DWORD) {
	//Platform::DebugPrintf("DOB DUnadvise\n");
	return E_FAIL;
}

STDMETHODIMP DataObject_EnumDAdvise(DataObject *, IEnumSTATDATA **) {
	//Platform::DebugPrintf("DOB EnumDAdvise\n");
	return E_FAIL;
}

static VFunction *vtDataObject[] = {
	(VFunction *)(DataObject_QueryInterface),
	(VFunction *)(DataObject_AddRef),
	(VFunction *)(DataObject_Release),
	(VFunction *)(DataObject_GetData),
	(VFunction *)(DataObject_GetDataHere),
	(VFunction *)(DataObject_QueryGetData),
	(VFunction *)(DataObject_GetCanonicalFormatEtc),
	(VFunction *)(DataObject_SetData),
	(VFunction *)(DataObject_EnumFormatEtc),
	(VFunction *)(DataObject_DAdvise),
	(VFunction *)(DataObject_DUnadvise),
	(VFunction *)(DataObject_EnumDAdvise)
};

DataObject::DataObject() noexcept {
	vtbl = vtDataObject;
	sci = nullptr;
}

/// Implement IUnknown
STDMETHODIMP DropTarget_QueryInterface(DropTarget *dt, REFIID riid, PVOID *ppv) {
	//Platform::DebugPrintf("DT QI %x\n", dt);
	return dt->sci->QueryInterface(riid, ppv);
}
STDMETHODIMP_(ULONG)DropTarget_AddRef(DropTarget *dt) {
	return dt->sci->AddRef();
}
STDMETHODIMP_(ULONG)DropTarget_Release(DropTarget *dt) {
	return dt->sci->Release();
}

/// Implement IDropTarget by forwarding to Scintilla
STDMETHODIMP DropTarget_DragEnter(DropTarget *dt, LPDATAOBJECT pIDataSource, DWORD grfKeyState,
                                  POINTL pt, PDWORD pdwEffect) {
	try {
		return dt->sci->DragEnter(pIDataSource, grfKeyState, pt, pdwEffect);
	} catch (...) {
		dt->sci->errorStatus = Status::Failure;
	}
	return E_FAIL;
}
STDMETHODIMP DropTarget_DragOver(DropTarget *dt, DWORD grfKeyState, POINTL pt, PDWORD pdwEffect) {
	try {
		return dt->sci->DragOver(grfKeyState, pt, pdwEffect);
	} catch (...) {
		dt->sci->errorStatus = Status::Failure;
	}
	return E_FAIL;
}
STDMETHODIMP DropTarget_DragLeave(DropTarget *dt) {
	try {
		return dt->sci->DragLeave();
	} catch (...) {
		dt->sci->errorStatus = Status::Failure;
	}
	return E_FAIL;
}
STDMETHODIMP DropTarget_Drop(DropTarget *dt, LPDATAOBJECT pIDataSource, DWORD grfKeyState,
                             POINTL pt, PDWORD pdwEffect) {
	try {
		return dt->sci->Drop(pIDataSource, grfKeyState, pt, pdwEffect);
	} catch (...) {
		dt->sci->errorStatus = Status::Failure;
	}
	return E_FAIL;
}

static VFunction *vtDropTarget[] = {
	(VFunction *)(DropTarget_QueryInterface),
	(VFunction *)(DropTarget_AddRef),
	(VFunction *)(DropTarget_Release),
	(VFunction *)(DropTarget_DragEnter),
	(VFunction *)(DropTarget_DragOver),
	(VFunction *)(DropTarget_DragLeave),
	(VFunction *)(DropTarget_Drop)
};

DropTarget::DropTarget() noexcept {
	vtbl = vtDropTarget;
	sci = nullptr;
}

/**
 * DBCS: support Input Method Editor (IME).
 * Called when IME Window opened.
 */
void ScintillaWin::ImeStartComposition() {
	if (caret.active) {
		// Move IME Window to current caret position
		IMContext imc(MainHWND());
		const Point pos = PointMainCaret();
		COMPOSITIONFORM CompForm;
		CompForm.dwStyle = CFS_POINT;
		CompForm.ptCurrentPos = POINTFromPoint(pos);

		::ImmSetCompositionWindow(imc.hIMC, &CompForm);

		// Set font of IME window to same as surrounded text.
		if (stylesValid) {
			// Since the style creation code has been made platform independent,
			// The logfont for the IME is recreated here.
			const int styleHere = pdoc->StyleIndexAt(sel.MainCaret());
			LOGFONTW lf = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L""};
			int sizeZoomed = vs.styles[styleHere].size + vs.zoomLevel * FontSizeMultiplier;
			if (sizeZoomed <= 2 * FontSizeMultiplier)	// Hangs if sizeZoomed <= 1
				sizeZoomed = 2 * FontSizeMultiplier;
			// The negative is to allow for leading
			lf.lfHeight = -::MulDiv(sizeZoomed, dpi, 72*FontSizeMultiplier);
			lf.lfWeight = static_cast<LONG>(vs.styles[styleHere].weight);
			lf.lfItalic = vs.styles[styleHere].italic ? 1 : 0;
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfFaceName[0] = L'\0';
			if (vs.styles[styleHere].fontName) {
				const char* fontName = vs.styles[styleHere].fontName;
				UTF16FromUTF8(std::string_view(fontName), lf.lfFaceName, LF_FACESIZE);
			}

			::ImmSetCompositionFontW(imc.hIMC, &lf);
		}
		// Caret is displayed in IME window. So, caret in Scintilla is useless.
		DropCaret();
	}
}

/** Called when IME Window closed. */
void ScintillaWin::ImeEndComposition() {
	// clear IME composition state.
	view.imeCaretBlockOverride = false;
	pdoc->TentativeUndo();
	ShowCaretAtCurrentPosition();
}

LRESULT ScintillaWin::ImeOnReconvert(LPARAM lParam) {
	// Reconversion on windows limits within one line without eol.
	// Look around:   baseStart  <--  (|mainStart|  -- mainEnd)  --> baseEnd.
	const Sci::Position mainStart = sel.RangeMain().Start().Position();
	const Sci::Position mainEnd = sel.RangeMain().End().Position();
	const Sci::Line curLine = pdoc->SciLineFromPosition(mainStart);
	if (curLine != pdoc->LineFromPosition(mainEnd))
		return 0;
	const Sci::Position baseStart = pdoc->LineStart(curLine);
	const Sci::Position baseEnd = pdoc->LineEnd(curLine);
	if ((baseStart == baseEnd) || (mainEnd > baseEnd))
		return 0;

	const int codePage = CodePageOfDocument();
	const std::wstring rcFeed = StringDecode(RangeText(baseStart, baseEnd), codePage);
	const int rcFeedLen = static_cast<int>(rcFeed.length()) * sizeof(wchar_t);
	const int rcSize = sizeof(RECONVERTSTRING) + rcFeedLen + sizeof(wchar_t);

	RECONVERTSTRING *rc = static_cast<RECONVERTSTRING *>(PtrFromSPtr(lParam));
	if (!rc)
		return rcSize; // Immediately be back with rcSize of memory block.

	wchar_t *rcFeedStart = reinterpret_cast<wchar_t*>(rc + 1);
	memcpy(rcFeedStart, &rcFeed[0], rcFeedLen);

	std::string rcCompString = RangeText(mainStart, mainEnd);
	std::wstring rcCompWstring = StringDecode(rcCompString, codePage);
	std::string rcCompStart = RangeText(baseStart, mainStart);
	std::wstring rcCompWstart = StringDecode(rcCompStart, codePage);

	// Map selection to dwCompStr.
	// No selection assumes current caret as rcCompString without length.
	rc->dwVersion = 0; // It should be absolutely 0.
	rc->dwStrLen = static_cast<DWORD>(rcFeed.length());
	rc->dwStrOffset = sizeof(RECONVERTSTRING);
	rc->dwCompStrLen = static_cast<DWORD>(rcCompWstring.length());
	rc->dwCompStrOffset = static_cast<DWORD>(rcCompWstart.length()) * sizeof(wchar_t);
	rc->dwTargetStrLen = rc->dwCompStrLen;
	rc->dwTargetStrOffset =rc->dwCompStrOffset;

	IMContext imc(MainHWND());
	if (!imc.hIMC)
		return 0;

	if (!::ImmSetCompositionStringW(imc.hIMC, SCS_QUERYRECONVERTSTRING, rc, rcSize, nullptr, 0))
		return 0;

	// No selection asks IME to fill target fields with its own value.
	const int tgWlen = rc->dwTargetStrLen;
	const int tgWstart = rc->dwTargetStrOffset / sizeof(wchar_t);

	std::string tgCompStart = StringEncode(rcFeed.substr(0, tgWstart), codePage);
	std::string tgComp = StringEncode(rcFeed.substr(tgWstart, tgWlen), codePage);

	// No selection needs to adjust reconvert start position for IME set.
	const int adjust = static_cast<int>(tgCompStart.length() - rcCompStart.length());
	const int docCompLen = static_cast<int>(tgComp.length());

	// Make place for next composition string to sit in.
	for (size_t r=0; r<sel.Count(); r++) {
		const Sci::Position rBase = sel.Range(r).Start().Position();
		const Sci::Position docCompStart = rBase + adjust;

		if (inOverstrike) { // the docCompLen of bytes will be overstriked.
			sel.Range(r).caret.SetPosition(docCompStart);
			sel.Range(r).anchor.SetPosition(docCompStart);
		} else {
			// Ensure docCompStart+docCompLen be not beyond lineEnd.
			// since docCompLen by byte might break eol.
			const Sci::Position lineEnd = pdoc->LineEnd(pdoc->LineFromPosition(rBase));
			const Sci::Position overflow = (docCompStart + docCompLen) - lineEnd;
			if (overflow > 0) {
				pdoc->DeleteChars(docCompStart, docCompLen - overflow);
			} else {
				pdoc->DeleteChars(docCompStart, docCompLen);
			}
		}
	}
	// Immediately Target Input or candidate box choice with GCS_COMPSTR.
	return rcSize;
}

void ScintillaWin::GetIntelliMouseParameters() noexcept {
	// This retrieves the number of lines per scroll as configured in the Mouse Properties sheet in Control Panel
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &linesPerScroll, 0);
}

void ScintillaWin::CopyToGlobal(GlobalMemory &gmUnicode, const SelectionText &selectedText) {
	const std::string_view svSelected(selectedText.Data(), selectedText.LengthWithTerminator());
	if (IsUnicodeMode()) {
		const size_t uchars = UTF16Length(svSelected);
		gmUnicode.Allocate(2 * uchars);
		if (gmUnicode) {
			UTF16FromUTF8(svSelected,
				static_cast<wchar_t *>(gmUnicode.ptr), uchars);
		}
	} else {
		// Not Unicode mode
		// Convert to Unicode using the current Scintilla code page
		const UINT cpSrc = CodePageFromCharSet(
			selectedText.characterSet, selectedText.codePage);
		const size_t uLen = WideCharLenFromMultiByte(cpSrc, svSelected);
		gmUnicode.Allocate(2 * uLen);
		if (gmUnicode) {
			WideCharFromMultiByte(cpSrc, svSelected,
				static_cast<wchar_t *>(gmUnicode.ptr), uLen);
		}
	}
}

void ScintillaWin::CopyToClipboard(const SelectionText &selectedText) {
	if (!::OpenClipboardRetry(MainHWND())) {
		return;
	}
	::EmptyClipboard();

	GlobalMemory uniText;
	CopyToGlobal(uniText, selectedText);
	if (uniText) {
		uniText.SetClip(CF_UNICODETEXT);
	}

	if (selectedText.rectangular) {
		::SetClipboardData(cfColumnSelect, 0);

		GlobalMemory borlandSelection;
		borlandSelection.Allocate(1);
		if (borlandSelection) {
			static_cast<BYTE *>(borlandSelection.ptr)[0] = 0x02;
			borlandSelection.SetClip(cfBorlandIDEBlockType);
		}
	}

	if (selectedText.lineCopy) {
		::SetClipboardData(cfLineSelect, 0);
		::SetClipboardData(cfVSLineTag, 0);
	}

	::CloseClipboard();
}

void ScintillaWin::ScrollMessage(WPARAM wParam) {
	//DWORD dwStart = timeGetTime();
	//Platform::DebugPrintf("Scroll %x %d\n", wParam, lParam);

	SCROLLINFO sci = {};
	sci.cbSize = sizeof(sci);
	sci.fMask = SIF_ALL;

	GetScrollInfo(SB_VERT, &sci);

	//Platform::DebugPrintf("ScrollInfo %d mask=%x min=%d max=%d page=%d pos=%d track=%d\n", b,sci.fMask,
	//sci.nMin, sci.nMax, sci.nPage, sci.nPos, sci.nTrackPos);
	Sci::Line topLineNew = topLine;
	switch (LOWORD(wParam)) {
	case SB_LINEUP:
		topLineNew -= 1;
		break;
	case SB_LINEDOWN:
		topLineNew += 1;
		break;
	case SB_PAGEUP:
		topLineNew -= LinesToScroll(); break;
	case SB_PAGEDOWN: topLineNew += LinesToScroll(); break;
	case SB_TOP: topLineNew = 0; break;
	case SB_BOTTOM: topLineNew = MaxScrollPos(); break;
	case SB_THUMBPOSITION: topLineNew = sci.nTrackPos; break;
	case SB_THUMBTRACK: topLineNew = sci.nTrackPos; break;
	}
	ScrollTo(topLineNew);
}

void ScintillaWin::HorizontalScrollMessage(WPARAM wParam) {
	int xPos = xOffset;
	const PRectangle rcText = GetTextRectangle();
	const int pageWidth = static_cast<int>(rcText.Width() * 2 / 3);
	switch (LOWORD(wParam)) {
	case SB_LINEUP:
		xPos -= 20;
		break;
	case SB_LINEDOWN:	// May move past the logical end
		xPos += 20;
		break;
	case SB_PAGEUP:
		xPos -= pageWidth;
		break;
	case SB_PAGEDOWN:
		xPos += pageWidth;
		if (xPos > scrollWidth - rcText.Width()) {	// Hit the end exactly
			xPos = scrollWidth - static_cast<int>(rcText.Width());
		}
		break;
	case SB_TOP:
		xPos = 0;
		break;
	case SB_BOTTOM:
		xPos = scrollWidth;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK: {
			// Do NOT use wParam, its 16 bit and not enough for very long lines. Its still possible to overflow the 32 bit but you have to try harder =]
			SCROLLINFO si {};
			si.cbSize = sizeof(si);
			si.fMask = SIF_TRACKPOS;
			if (GetScrollInfo(SB_HORZ, &si)) {
				xPos = si.nTrackPos;
			}
		}
		break;
	}
	HorizontalScrollTo(xPos);
}

/**
 * Redraw all of text area.
 * This paint will not be abandoned.
 */
void ScintillaWin::FullPaint() {
	if ((technology == Technology::Default) || (technology == Technology::DirectWriteDC)) {
		HDC hdc = ::GetDC(MainHWND());
		FullPaintDC(hdc);
		::ReleaseDC(MainHWND(), hdc);
	} else {
		FullPaintDC({});
	}
}

/**
 * Redraw all of text area on the specified DC.
 * This paint will not be abandoned.
 */
void ScintillaWin::FullPaintDC(HDC hdc) {
	paintState = PaintState::painting;
	rcPaint = GetClientRectangle();
	paintingAllText = true;
	PaintDC(hdc);
	paintState = PaintState::notPainting;
}

namespace {

bool CompareDevCap(HDC hdc, HDC hOtherDC, int nIndex) noexcept {
	return ::GetDeviceCaps(hdc, nIndex) == ::GetDeviceCaps(hOtherDC, nIndex);
}

}

bool ScintillaWin::IsCompatibleDC(HDC hOtherDC) noexcept {
	HDC hdc = ::GetDC(MainHWND());
	const bool isCompatible =
		CompareDevCap(hdc, hOtherDC, TECHNOLOGY) &&
		CompareDevCap(hdc, hOtherDC, LOGPIXELSY) &&
		CompareDevCap(hdc, hOtherDC, LOGPIXELSX) &&
		CompareDevCap(hdc, hOtherDC, BITSPIXEL) &&
		CompareDevCap(hdc, hOtherDC, PLANES);
	::ReleaseDC(MainHWND(), hdc);
	return isCompatible;
}

DWORD ScintillaWin::EffectFromState(DWORD grfKeyState) const noexcept {
	// These are the Wordpad semantics.
	DWORD dwEffect;
	if (inDragDrop == DragDrop::dragging)	// Internal defaults to move
		dwEffect = DROPEFFECT_MOVE;
	else
		dwEffect = DROPEFFECT_COPY;
	if (grfKeyState & MK_ALT)
		dwEffect = DROPEFFECT_MOVE;
	if (grfKeyState & MK_CONTROL)
		dwEffect = DROPEFFECT_COPY;
	return dwEffect;
}

/// Implement IUnknown
STDMETHODIMP ScintillaWin::QueryInterface(REFIID riid, PVOID *ppv) {
	*ppv = nullptr;
	if (riid == IID_IUnknown)
		*ppv = reinterpret_cast<IDropTarget *>(&dt);
	if (riid == IID_IDropSource)
		*ppv = reinterpret_cast<IDropSource *>(&ds);
	if (riid == IID_IDropTarget)
		*ppv = reinterpret_cast<IDropTarget *>(&dt);
	if (riid == IID_IDataObject)
		*ppv = reinterpret_cast<IDataObject *>(&dob);
	if (!*ppv)
		return E_NOINTERFACE;
	return S_OK;
}

STDMETHODIMP_(ULONG) ScintillaWin::AddRef() {
	return 1;
}

STDMETHODIMP_(ULONG) ScintillaWin::Release() {
	return 1;
}

/// Implement IDropTarget
STDMETHODIMP ScintillaWin::DragEnter(LPDATAOBJECT pIDataSource, DWORD grfKeyState,
                                     POINTL, PDWORD pdwEffect) {
	if (!pIDataSource )
		return E_POINTER;
	FORMATETC fmtu = {CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	const HRESULT hrHasUText = pIDataSource->QueryGetData(&fmtu);
	hasOKText = (hrHasUText == S_OK);
	if (hasOKText) {
		*pdwEffect = EffectFromState(grfKeyState);
	} else {
		*pdwEffect = DROPEFFECT_NONE;
	}

	return S_OK;
}

STDMETHODIMP ScintillaWin::DragOver(DWORD grfKeyState, POINTL pt, PDWORD pdwEffect) {
	try {
		if (!hasOKText || pdoc->IsReadOnly()) {
			*pdwEffect = DROPEFFECT_NONE;
			return S_OK;
		}

		*pdwEffect = EffectFromState(grfKeyState);

		// Update the cursor.
		POINT rpt = {pt.x, pt.y};
		::ScreenToClient(MainHWND(), &rpt);
		SetDragPosition(SPositionFromLocation(PointFromPOINT(rpt), false, false, UserVirtualSpace()));

		return S_OK;
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return E_FAIL;
}

STDMETHODIMP ScintillaWin::DragLeave() {
	try {
		SetDragPosition(SelectionPosition(Sci::invalidPosition));
		return S_OK;
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return E_FAIL;
}

STDMETHODIMP ScintillaWin::Drop(LPDATAOBJECT pIDataSource, DWORD grfKeyState,
                                POINTL pt, PDWORD pdwEffect) {
	try {
		*pdwEffect = EffectFromState(grfKeyState);

		if (!pIDataSource)
			return E_POINTER;

		SetDragPosition(SelectionPosition(Sci::invalidPosition));

		std::string putf;
		FORMATETC fmtu = {CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		STGMEDIUM medium{};
		const HRESULT hr = pIDataSource->GetData(&fmtu, &medium);
		if (!SUCCEEDED(hr)) {
			return hr;
		}
		if (medium.hGlobal) {
			GlobalMemory memUDrop(medium.hGlobal);
			if (const wchar_t *uptr = static_cast<const wchar_t *>(memUDrop.ptr)) {
				putf = EncodeWString(uptr);
			}
			memUDrop.Unlock();
		}

		if (putf.empty()) {
			return S_OK;
		}

		FORMATETC fmtr = {cfColumnSelect, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		const bool isRectangular = S_OK == pIDataSource->QueryGetData(&fmtr);

		POINT rpt = {pt.x, pt.y};
		::ScreenToClient(MainHWND(), &rpt);
		const SelectionPosition movePos = SPositionFromLocation(PointFromPOINT(rpt), false, false, UserVirtualSpace());

		DropAt(movePos, putf.c_str(), putf.size(), *pdwEffect == DROPEFFECT_MOVE, isRectangular);

		// Free data
		::ReleaseStgMedium(&medium);

		return S_OK;
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return E_FAIL;
}

/// Implement important part of IDataObject
STDMETHODIMP ScintillaWin::GetData(FORMATETC *pFEIn, STGMEDIUM *pSTM) {
	if (!SupportedFormat(pFEIn)) {
		//Platform::DebugPrintf("DOB GetData No %d %x %x fmt=%x\n", lenDrag, pFEIn, pSTM, pFEIn->cfFormat);
		return DATA_E_FORMATETC;
	}

	pSTM->tymed = TYMED_HGLOBAL;
	//Platform::DebugPrintf("DOB GetData OK %d %x %x\n", lenDrag, pFEIn, pSTM);

	GlobalMemory uniText;
	CopyToGlobal(uniText, drag);
	pSTM->hGlobal = uniText ? uniText.Unlock() : 0;
	pSTM->pUnkForRelease = nullptr;
	return S_OK;
}

void ScintillaWin::Prepare() noexcept {
	Platform_Initialise(hInstance);

	// Register the CallTip class
	WNDCLASSEX wndclassc{};
	wndclassc.cbSize = sizeof(wndclassc);
	wndclassc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wndclassc.cbWndExtra = sizeof(ScintillaWin *);
	wndclassc.hInstance = hInstance;
	wndclassc.lpfnWndProc = ScintillaWin::CTWndProc;
	wndclassc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wndclassc.lpszClassName = callClassName;

	callClassAtom = ::RegisterClassEx(&wndclassc);
}

bool ScintillaWin::Register(HINSTANCE hInstance_) noexcept {

	hInstance = hInstance_;

	// Register the Scintilla class
	// Register Scintilla as a wide character window
	WNDCLASSEXW wndclass {};
	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = ScintillaWin::SWndProc;
	wndclass.cbWndExtra = sizeof(ScintillaWin *);
	wndclass.hInstance = hInstance;
	wndclass.lpszClassName = L"Scintilla";
	scintillaClassAtom = ::RegisterClassExW(&wndclass);
	const bool result = 0 != scintillaClassAtom;

	return result;
}

bool ScintillaWin::Unregister() noexcept {
	bool result = true;
	if (0 != scintillaClassAtom) {
		if (::UnregisterClass(MAKEINTATOM(scintillaClassAtom), hInstance) == 0) {
			result = false;
		}
		scintillaClassAtom = 0;
	}
	if (0 != callClassAtom) {
		if (::UnregisterClass(MAKEINTATOM(callClassAtom), hInstance) == 0) {
			result = false;
		}
		callClassAtom = 0;
	}
	return result;
}

bool ScintillaWin::HasCaretSizeChanged() const noexcept {
	if (
		( (0 != vs.caret.width) && (sysCaretWidth != vs.caret.width) )
		|| ((0 != vs.lineHeight) && (sysCaretHeight != vs.lineHeight))
		) {
		return true;
	}
	return false;
}

BOOL ScintillaWin::CreateSystemCaret() {
	sysCaretWidth = vs.caret.width;
	if (0 == sysCaretWidth) {
		sysCaretWidth = 1;
	}
	sysCaretHeight = vs.lineHeight;
	const int bitmapSize = (((sysCaretWidth + 15) & ~15) >> 3) *
		sysCaretHeight;
	std::vector<BYTE> bits(bitmapSize);
	sysCaretBitmap = ::CreateBitmap(sysCaretWidth, sysCaretHeight, 1,
		1, &bits[0]);
	const BOOL retval = ::CreateCaret(
		MainHWND(), sysCaretBitmap,
		sysCaretWidth, sysCaretHeight);
	if (technology == Technology::Default) {
		// System caret interferes with Direct2D drawing so only show it for GDI.
		::ShowCaret(MainHWND());
	}
	return retval;
}

BOOL ScintillaWin::DestroySystemCaret() noexcept {
	::HideCaret(MainHWND());
	const BOOL retval = ::DestroyCaret();
	if (sysCaretBitmap) {
		::DeleteObject(sysCaretBitmap);
		sysCaretBitmap = {};
	}
	return retval;
}

LRESULT PASCAL ScintillaWin::CTWndProc(
	HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	// Find C++ object associated with window.
	ScintillaWin *sciThis = static_cast<ScintillaWin *>(PointerFromWindow(hWnd));
	try {
		// ctp will be zero if WM_CREATE not seen yet
		if (sciThis == nullptr) {
			if (iMessage == WM_CREATE) {
				// Associate CallTip object with window
				CREATESTRUCT *pCreate = static_cast<CREATESTRUCT *>(PtrFromSPtr(lParam));
				SetWindowPointer(hWnd, pCreate->lpCreateParams);
				return 0;
			} else {
				return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
			}
		} else {
			if (iMessage == WM_NCDESTROY) {
				SetWindowPointer(hWnd, nullptr);
				return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
			} else if (iMessage == WM_PAINT) {
				PAINTSTRUCT ps;
				::BeginPaint(hWnd, &ps);
				std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(sciThis->technology));
#if defined(USE_D2D)
				ID2D1HwndRenderTarget *pCTRenderTarget = nullptr;
#endif
				RECT rc;
				GetClientRect(hWnd, &rc);
				if (sciThis->technology == Technology::Default) {
					surfaceWindow->Init(ps.hdc, hWnd);
				} else {
#if defined(USE_D2D)
					// Create a Direct2D render target.
					D2D1_HWND_RENDER_TARGET_PROPERTIES dhrtp {};
					dhrtp.hwnd = hWnd;
					dhrtp.pixelSize = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
					dhrtp.presentOptions = (sciThis->technology == Technology::DirectWriteRetain) ?
						D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS : D2D1_PRESENT_OPTIONS_NONE;

					D2D1_RENDER_TARGET_PROPERTIES drtp {};
					drtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
					drtp.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
					drtp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
					drtp.dpiX = 96.0;
					drtp.dpiY = 96.0;
					drtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;
					drtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

					if (!SUCCEEDED(pD2DFactory->CreateHwndRenderTarget(drtp, dhrtp, &pCTRenderTarget))) {
						surfaceWindow->Release();
						::EndPaint(hWnd, &ps);
						return 0;
					}
					// If above SUCCEEDED, then pCTRenderTarget not nullptr
					assert(pCTRenderTarget);
					if (pCTRenderTarget) {
						surfaceWindow->Init(pCTRenderTarget, hWnd);
						pCTRenderTarget->BeginDraw();
					}
#endif
				}
				surfaceWindow->SetMode(sciThis->CurrentSurfaceMode());
				sciThis->SetRenderingParams(surfaceWindow.get());
				sciThis->ct.PaintCT(surfaceWindow.get());
#if defined(USE_D2D)
				if (pCTRenderTarget)
					pCTRenderTarget->EndDraw();
#endif
				surfaceWindow->Release();
#if defined(USE_D2D)
				ReleaseUnknown(pCTRenderTarget);
#endif
				::EndPaint(hWnd, &ps);
				return 0;
			} else if ((iMessage == WM_NCLBUTTONDOWN) || (iMessage == WM_NCLBUTTONDBLCLK)) {
				POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				ScreenToClient(hWnd, &pt);
				sciThis->ct.MouseClick(PointFromPOINT(pt));
				sciThis->CallTipClick();
				return 0;
			} else if (iMessage == WM_LBUTTONDOWN) {
				// This does not fire due to the hit test code
				sciThis->ct.MouseClick(PointFromLParam(lParam));
				sciThis->CallTipClick();
				return 0;
			} else if (iMessage == WM_SETCURSOR) {
				::SetCursor(::LoadCursor(NULL, IDC_ARROW));
				return 0;
			} else if (iMessage == WM_NCHITTEST) {
				return HTCAPTION;
			} else {
				return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
			}
		}
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
	return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
}

sptr_t ScintillaWin::DirectFunction(
    sptr_t ptr, UINT iMessage, uptr_t wParam, sptr_t lParam) {
	ScintillaWin *sci = reinterpret_cast<ScintillaWin *>(ptr);
	PLATFORM_ASSERT(::GetCurrentThreadId() == ::GetWindowThreadProcessId(sci->MainHWND(), nullptr));
	return sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

sptr_t ScintillaWin::DirectStatusFunction(
    sptr_t ptr, UINT iMessage, uptr_t wParam, sptr_t lParam, int *pStatus) {
	ScintillaWin *sci = reinterpret_cast<ScintillaWin *>(ptr);
	PLATFORM_ASSERT(::GetCurrentThreadId() == ::GetWindowThreadProcessId(sci->MainHWND(), nullptr));
	const sptr_t returnValue = sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
	*pStatus = static_cast<int>(sci->errorStatus);
	return returnValue;
}

namespace Scintilla::Internal {

sptr_t DirectFunction(
    ScintillaWin *sci, UINT iMessage, uptr_t wParam, sptr_t lParam) {
	return sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

}

LRESULT PASCAL ScintillaWin::SWndProc(
	HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	//Platform::DebugPrintf("S W:%x M:%x WP:%x L:%x\n", hWnd, iMessage, wParam, lParam);

	// Find C++ object associated with window.
	ScintillaWin *sci = static_cast<ScintillaWin *>(PointerFromWindow(hWnd));
	// sci will be zero if WM_CREATE not seen yet
	if (sci == nullptr) {
		try {
			if (iMessage == WM_CREATE) {
				static std::once_flag once;
				std::call_once(once, Prepare);
				// Create C++ object associated with window
				sci = new ScintillaWin(hWnd);
				SetWindowPointer(hWnd, sci);
				return sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
			}
		} catch (...) {
		}
		return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
	} else {
		if (iMessage == WM_NCDESTROY) {
			try {
				sci->Finalise();
				delete sci;
			} catch (...) {
			}
			SetWindowPointer(hWnd, nullptr);
			return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
		} else {
			return sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
		}
	}
}

// This function is externally visible so it can be called from container when building statically.
// Must be called once only.
extern "C" int Scintilla_RegisterClasses(void *hInstance) {
	const bool result = ScintillaWin::Register(static_cast<HINSTANCE>(hInstance));
	return result;
}

namespace Scintilla::Internal {

int ResourcesRelease(bool fromDllMain) noexcept {
	const bool result = ScintillaWin::Unregister();
	Platform_Finalise(fromDllMain);
	return result;
}

int RegisterClasses(void *hInstance) noexcept {
	const bool result = ScintillaWin::Register(static_cast<HINSTANCE>(hInstance));
	return result;
}

}

// This function is externally visible so it can be called from container when building statically.
extern "C" int Scintilla_ReleaseResources() {
	return Scintilla::Internal::ResourcesRelease(false);
}
