// Scintilla source code edit control
/** @file ScintillaWin.cxx
 ** Windows specific subclass of ScintillaBase.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
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
#include <algorithm>
#include <memory>
#include <chrono>

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#undef WINVER
#define WINVER 0x0500
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

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"

#ifdef SCI_LEXER
#include "SciLexer.h"
#endif
#include "CharacterCategory.h"
#ifdef SCI_LEXER
#include "LexerModule.h"
#endif
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

#ifdef SCI_LEXER
#include "ExternalLexer.h"
#endif

#include "PlatWin.h"
#include "HanjaDic.h"
#include "ScintillaWin.h"

#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES   104
#endif

#ifndef WM_UNICHAR
#define WM_UNICHAR                      0x0109
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

// Two idle messages SC_WIN_IDLE and SC_WORK_IDLE.

// SC_WIN_IDLE is low priority so should occur after the next WM_PAINT
// It is for lengthy actions like wrapping and background styling
constexpr UINT SC_WIN_IDLE = 5001;
// SC_WORK_IDLE is high priority and should occur before the next WM_PAINT
// It is for shorter actions like restyling the text just inserted
// and delivering SCN_UPDATEUI
constexpr UINT SC_WORK_IDLE = 5002;

#define SC_INDICATOR_INPUT INDICATOR_IME
#define SC_INDICATOR_TARGET INDICATOR_IME+1
#define SC_INDICATOR_CONVERTED INDICATOR_IME+2
#define SC_INDICATOR_UNKNOWN INDICATOR_IME_MAX

#ifndef SCS_CAP_SETRECONVERTSTRING
#define SCS_CAP_SETRECONVERTSTRING 0x00000004
#define SCS_QUERYRECONVERTSTRING 0x00020000
#define SCS_SETRECONVERTSTRING 0x00010000
#endif

typedef UINT_PTR (WINAPI *SetCoalescableTimerSig)(HWND hwnd, UINT_PTR nIDEvent,
	UINT uElapse, TIMERPROC lpTimerFunc, ULONG uToleranceDelay);

// GCC has trouble with the standard COM ABI so do it the old C way with explicit vtables.

using namespace Scintilla;

namespace {

const TCHAR callClassName[] = TEXT("CallTip");

void *PointerFromWindow(HWND hWnd) noexcept {
	return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, 0));
}

void SetWindowPointer(HWND hWnd, void *ptr) noexcept {
	::SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(ptr));
}

void SetWindowID(HWND hWnd, int identifier) noexcept {
	::SetWindowLongPtr(hWnd, GWLP_ID, identifier);
}

Point PointFromPOINT(POINT pt) noexcept {
	return Point::FromInts(pt.x, pt.y);
}
Point PointFromLParam(sptr_t lpoint) noexcept {
	return Point::FromInts(GET_X_LPARAM(lpoint), GET_Y_LPARAM(lpoint));
}
constexpr POINT POINTFromPoint(Point pt) noexcept {
	return POINT{ static_cast<LONG>(pt.x), static_cast<LONG>(pt.y) };
}
bool KeyboardIsKeyDown(int key) noexcept {
	return (::GetKeyState(key) & 0x80000000) != 0;
}

}

class ScintillaWin; 	// Forward declaration for COM interface subobjects

typedef void VFunction(void);


/**
 */
class FormatEnumerator {
public:
	VFunction **vtbl;
	int ref;
	unsigned int pos;
	std::vector<CLIPFORMAT> formats;
	FormatEnumerator(int pos_, CLIPFORMAT formats_[], size_t formatsLen_);
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

namespace {

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

}

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
#endif

	explicit ScintillaWin(HWND hwnd);
	// Deleted so ScintillaWin objects can not be copied.
	ScintillaWin(const ScintillaWin &) = delete;
	ScintillaWin(ScintillaWin &&) = delete;
	ScintillaWin &operator=(const ScintillaWin &) = delete;
	ScintillaWin &operator=(ScintillaWin &&) = delete;
	// ~ScintillaWin() in public section

	void Init();
	void Finalise() override;
#if defined(USE_D2D)
	void EnsureRenderTarget(HDC hdc);
	void DropRenderTarget();
#endif
	HWND MainHWND() const noexcept;

	static sptr_t DirectFunction(
		    sptr_t ptr, UINT iMessage, uptr_t wParam, sptr_t lParam);
	static LRESULT PASCAL SWndProc(
		    HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	static LRESULT PASCAL CTWndProc(
		    HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	enum { invalidTimerID, standardTimerID, idleTimerID, fineTimerStart };

	bool DragThreshold(Point ptStart, Point ptNow) override;
	void StartDrag() override;
	static int MouseModifiers(uptr_t wParam) noexcept;

	Sci::Position TargetAsUTF8(char *text) const;
	Sci::Position EncodedFromUTF8(const char *utf8, char *encoded) const;
	sptr_t WndPaint();

	sptr_t HandleCompositionWindowed(uptr_t wParam, sptr_t lParam);
	sptr_t HandleCompositionInline(uptr_t wParam, sptr_t lParam);
	static bool KoreanIME() noexcept;
	void MoveImeCarets(Sci::Position offset);
	void DrawImeIndicator(int indicator, int len);
	void SetCandidateWindowPos();
	void SelectionToHangul();
	void EscapeHanja();
	void ToggleHanja();
	void AddWString(std::wstring_view wsv, CharacterSource charSource);

	UINT CodePageOfDocument() const;
	bool ValidCodePage(int codePage) const override;
	sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override;
	void IdleWork() override;
	void QueueIdleWork(WorkNeeded::workItems items, Sci::Position upTo) override;
	bool SetIdle(bool on) override;
	UINT_PTR timers[tickDwell+1] {};
	bool FineTickerRunning(TickReason reason) override;
	void FineTickerStart(TickReason reason, int millis, int tolerance) override;
	void FineTickerCancel(TickReason reason) override;
	void SetMouseCapture(bool on) override;
	bool HaveMouseCapture() override;
	void SetTrackMouseLeaveEvent(bool on) noexcept;
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
	void NotifyParent(SCNotification scn) override;
	void NotifyDoubleClick(Point pt, int modifiers) override;
	CaseFolder *CaseFolderForEncoding() override;
	std::string CaseMapString(const std::string &s, int caseMapping) override;
	void Copy() override;
	void CopyAllowLine() override;
	bool CanPaste() override;
	void Paste() override;
	void CreateCallTipWindow(PRectangle rc) override;
	void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;
	void ClaimSelection() override;

	// DBCS
	void ImeStartComposition();
	void ImeEndComposition();
	LRESULT ImeOnReconvert(LPARAM lParam);

	void GetIntelliMouseParameters() noexcept;
	void CopyToClipboard(const SelectionText &selectedText) override;
	void ScrollMessage(WPARAM wParam);
	void HorizontalScrollMessage(WPARAM wParam);
	void FullPaint();
	void FullPaintDC(HDC hdc);
	bool IsCompatibleDC(HDC hOtherDC) noexcept;
	DWORD EffectFromState(DWORD grfKeyState) const noexcept;

	int SetScrollInfo(int nBar, LPCSCROLLINFO lpsi, BOOL bRedraw) noexcept;
	bool GetScrollInfo(int nBar, LPSCROLLINFO lpsi) noexcept;
	void ChangeScrollPos(int barType, Sci::Position pos);
	sptr_t GetTextLength();
	sptr_t GetText(uptr_t wParam, sptr_t lParam);

public:
	~ScintillaWin() override;

	// Public for benefit of Scintilla_DirectFunction
	sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override;

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

	static bool Register(HINSTANCE hInstance_) noexcept;
	static bool Unregister() noexcept;

	friend class DropSource;
	friend class DataObject;
	friend class DropTarget;
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

	hRgnUpdate = 0;

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

	sysCaretBitmap = 0;
	sysCaretWidth = 0;
	sysCaretHeight = 0;

	styleIdleInQueue = false;

#if defined(USE_D2D)
	pRenderTarget = nullptr;
	renderTargetValid = true;
#endif

	caret.period = ::GetCaretBlinkTime();
	if (caret.period < 0)
		caret.period = 0;

	Init();
}

ScintillaWin::~ScintillaWin() {}

void ScintillaWin::Init() {
	// Initialize COM.  If the app has already done this it will have
	// no effect.  If the app hasn't, we really shouldn't ask them to call
	// it just so this internal feature works.
	hrOle = ::OleInitialize(nullptr);

	// Find SetCoalescableTimer which is only available from Windows 8+
	HMODULE user32 = ::GetModuleHandle(TEXT("user32.dll"));
	if (user32) {
		SetCoalescableTimerFn = reinterpret_cast<SetCoalescableTimerSig>(
			::GetProcAddress(user32, "SetCoalescableTimer"));
	}

	vs.indicators[SC_INDICATOR_UNKNOWN] = Indicator(INDIC_HIDDEN, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_INPUT] = Indicator(INDIC_DOTS, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_CONVERTED] = Indicator(INDIC_COMPOSITIONTHICK, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_TARGET] = Indicator(INDIC_STRAIGHTBOX, ColourDesired(0, 0, 0xff));
}

void ScintillaWin::Finalise() {
	ScintillaBase::Finalise();
	for (TickReason tr = tickCaret; tr <= tickDwell; tr = static_cast<TickReason>(tr + 1)) {
		FineTickerCancel(tr);
	}
	SetIdle(false);
#if defined(USE_D2D)
	DropRenderTarget();
#endif
	::RevokeDragDrop(MainHWND());
	if (SUCCEEDED(hrOle)) {
		::OleUninitialize();
	}
}

#if defined(USE_D2D)

void ScintillaWin::EnsureRenderTarget(HDC hdc) {
	if (!renderTargetValid) {
		DropRenderTarget();
		renderTargetValid = true;
	}
	if (pD2DFactory && !pRenderTarget) {
		HWND hw = MainHWND();
		RECT rc;
		GetClientRect(hw, &rc);

		const D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		// Create a Direct2D render target.
#if 1
		D2D1_RENDER_TARGET_PROPERTIES drtp;
		drtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
		drtp.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
		drtp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
		drtp.dpiX = 96.0;
		drtp.dpiY = 96.0;
		drtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;
		drtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

		if (technology == SC_TECHNOLOGY_DIRECTWRITEDC) {
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
			D2D1_HWND_RENDER_TARGET_PROPERTIES dhrtp;
			dhrtp.hwnd = hw;
			dhrtp.pixelSize = size;
			dhrtp.presentOptions = (technology == SC_TECHNOLOGY_DIRECTWRITERETAIN) ?
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
#else
		pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT ,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				96.0f, 96.0f, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT),
			D2D1::HwndRenderTargetProperties(hw, size),
			&pRenderTarget);
#endif
		// Pixmaps were created to be compatible with previous render target so
		// need to be recreated.
		DropGraphics(false);
	}

	if ((technology == SC_TECHNOLOGY_DIRECTWRITEDC) && pRenderTarget) {
		RECT rcWindow;
		GetClientRect(MainHWND(), &rcWindow);
		const HRESULT hr = static_cast<ID2D1DCRenderTarget*>(pRenderTarget)->BindDC(hdc, &rcWindow);
		if (FAILED(hr)) {
			Platform::DebugPrintf("BindDC failed 0x%lx\n", hr);
			DropRenderTarget();
		}
	}
}

void ScintillaWin::DropRenderTarget() {
	if (pRenderTarget) {
		pRenderTarget->Release();
		pRenderTarget = nullptr;
	}
}

#endif

HWND ScintillaWin::MainHWND() const noexcept {
	return static_cast<HWND>(wMain.GetID());
}

bool ScintillaWin::DragThreshold(Point ptStart, Point ptNow) {
	const Point ptDifference = ptStart - ptNow;
	const XYPOSITION xMove = std::trunc(std::abs(ptDifference.x));
	const XYPOSITION yMove = std::trunc(std::abs(ptDifference.y));
	return (xMove > ::GetSystemMetrics(SM_CXDRAG)) ||
		(yMove > ::GetSystemMetrics(SM_CYDRAG));
}

void ScintillaWin::StartDrag() {
	inDragDrop = ddDragging;
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
	inDragDrop = ddNone;
	SetDragPosition(SelectionPosition(Sci::invalidPosition));
}

int ScintillaWin::MouseModifiers(uptr_t wParam) noexcept {
	return ModifierFlags((wParam & MK_SHIFT) != 0,
		(wParam & MK_CONTROL) != 0,
		KeyboardIsKeyDown(VK_MENU));
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

/** Map the key codes to their equivalent SCK_ form. */
int KeyTranslate(int keyIn) noexcept {
//PLATFORM_ASSERT(!keyIn);
	switch (keyIn) {
		case VK_DOWN:		return SCK_DOWN;
		case VK_UP:		return SCK_UP;
		case VK_LEFT:		return SCK_LEFT;
		case VK_RIGHT:		return SCK_RIGHT;
		case VK_HOME:		return SCK_HOME;
		case VK_END:		return SCK_END;
		case VK_PRIOR:		return SCK_PRIOR;
		case VK_NEXT:		return SCK_NEXT;
		case VK_DELETE:	return SCK_DELETE;
		case VK_INSERT:		return SCK_INSERT;
		case VK_ESCAPE:	return SCK_ESCAPE;
		case VK_BACK:		return SCK_BACK;
		case VK_TAB:		return SCK_TAB;
		case VK_RETURN:	return SCK_RETURN;
		case VK_ADD:		return SCK_ADD;
		case VK_SUBTRACT:	return SCK_SUBTRACT;
		case VK_DIVIDE:		return SCK_DIVIDE;
		case VK_LWIN:		return SCK_WIN;
		case VK_RWIN:		return SCK_RWIN;
		case VK_APPS:		return SCK_MENU;
		case VK_OEM_2:		return '/';
		case VK_OEM_3:		return '`';
		case VK_OEM_4:		return '[';
		case VK_OEM_5:		return '\\';
		case VK_OEM_6:		return ']';
		default:			return keyIn;
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
	const Sci::Position targetLength = targetEnd - targetStart;
	if (IsUnicodeMode()) {
		if (text) {
			pdoc->GetCharRange(text, targetStart, targetLength);
		}
	} else {
		// Need to convert
		const std::string s = RangeText(targetStart, targetEnd);
		const std::wstring characters = StringDecode(s, CodePageOfDocument());
		const int utf8Len = MultiByteLenFromWideChar(CP_UTF8, characters);
		if (text) {
			MultiByteFromWideChar(CP_UTF8, characters, text, utf8Len);
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
		const int charsLen = WideCharLenFromMultiByte(CP_UTF8, utf8Input);
		std::wstring characters(charsLen, L'\0');
		WideCharFromMultiByte(CP_UTF8, utf8Input, &characters[0], charsLen);

		const int encodedLen = MultiByteLenFromWideChar(CodePageOfDocument(), characters);
		if (encoded) {
			MultiByteFromWideChar(CodePageOfDocument(), characters, encoded, encodedLen);
			encoded[encodedLen] = '\0';
		}
		return encodedLen;
	}
}

sptr_t ScintillaWin::WndPaint() {
	//ElapsedPeriod ep;

	// Redirect assertions to debug output and save current state
	const bool assertsPopup = Platform::ShowAssertionPopUps(false);
	paintState = painting;
	PAINTSTRUCT ps = {};

	// Removed since this interferes with reporting other assertions as it occurs repeatedly
	//PLATFORM_ASSERT(hRgnUpdate == NULL);
	hRgnUpdate = ::CreateRectRgn(0, 0, 0, 0);
	::GetUpdateRgn(MainHWND(), hRgnUpdate, FALSE);
	::BeginPaint(MainHWND(), &ps);
	rcPaint = PRectangle::FromInts(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
	const PRectangle rcClient = GetClientRectangle();
	paintingAllText = BoundsContains(rcPaint, hRgnUpdate, rcClient);
	if (technology == SC_TECHNOLOGY_DEFAULT) {
		AutoSurface surfaceWindow(ps.hdc, this);
		if (surfaceWindow) {
			Paint(surfaceWindow, rcPaint);
			surfaceWindow->Release();
		}
	} else {
#if defined(USE_D2D)
		EnsureRenderTarget(ps.hdc);
		AutoSurface surfaceWindow(pRenderTarget, this);
		if (surfaceWindow) {
			pRenderTarget->BeginDraw();
			Paint(surfaceWindow, rcPaint);
			surfaceWindow->Release();
			const HRESULT hr = pRenderTarget->EndDraw();
			if (hr == static_cast<HRESULT>(D2DERR_RECREATE_TARGET)) {
				DropRenderTarget();
				paintState = paintAbandoned;
			}
		}
#endif
	}
	if (hRgnUpdate) {
		::DeleteRgn(hRgnUpdate);
		hRgnUpdate = 0;
	}

	::EndPaint(MainHWND(), &ps);
	if (paintState == paintAbandoned) {
		// Painting area was insufficient to cover new styling or brace highlight positions
		FullPaint();
		::ValidateRect(MainHWND(), nullptr);
	}
	paintState = notPainting;

	// Restore debug output state
	Platform::ShowAssertionPopUps(assertsPopup);

	//Platform::DebugPrintf("Paint took %g\n", ep.Duration());
	return 0;
}

sptr_t ScintillaWin::HandleCompositionWindowed(uptr_t wParam, sptr_t lParam) {
	if (lParam & GCS_RESULTSTR) {
		IMContext imc(MainHWND());
		if (imc.hIMC) {
			AddWString(imc.GetCompositionString(GCS_RESULTSTR), CharacterSource::imeResult);

			// Set new position after converted
			const Point pos = PointMainCaret();
			COMPOSITIONFORM CompForm;
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

void ScintillaWin::MoveImeCarets(Sci::Position offset) {
	// Move carets relatively by bytes.
	for (size_t r=0; r<sel.Count(); r++) {
		const Sci::Position positionInsert = sel.Range(r).Start().Position();
		sel.Range(r).caret.SetPosition(positionInsert + offset);
		sel.Range(r).anchor.SetPosition(positionInsert + offset);
	}
}

void ScintillaWin::DrawImeIndicator(int indicator, int len) {
	// Emulate the visual style of IME characters with indicators.
	// Draw an indicator on the character before caret by the character bytes of len
	// so it should be called after InsertCharacter().
	// It does not affect caret positions.
	if (indicator < 8 || indicator > INDICATOR_MAX) {
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
		CANDIDATEFORM CandForm;
		CandForm.dwIndex = 0;
		CandForm.dwStyle = CFS_CANDIDATEPOS;
		CandForm.ptCurrentPos.x = static_cast<int>(pos.x);
		CandForm.ptCurrentPos.y = static_cast<int>(pos.y + vs.lineHeight);
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
		const int converted = HanjaDict::GetHangulOfHanja(&uniStr[0]);
		documentStr = StringEncode(uniStr, CodePageOfDocument());

		if (converted > 0) {
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
	unsigned int const safeLength = UTF8MaxBytes+1;
	std::string oneChar(safeLength, '\0');
	pdoc->GetCharRange(&oneChar[0], currentPos, oneCharLen);

	std::wstring uniChar = StringDecode(oneChar, CodePageOfDocument());

	IMContext imc(MainHWND());
	if (imc.hIMC) {
		// Set the candidate box position since IME may show it.
		SetCandidateWindowPos();
		// IME_ESC_HANJA_MODE appears to receive the first character only.
		if (ImmEscapeW(GetKeyboardLayout(0), imc.hIMC, IME_ESC_HANJA_MODE, &uniChar[0])) {
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
	std::vector<int> imeIndicator(inputStyle.size(), SC_INDICATOR_UNKNOWN);
	for (size_t i = 0; i < inputStyle.size(); i++) {
		switch (static_cast<int>(inputStyle.at(i))) {
		case ATTR_INPUT:
			imeIndicator[i] = SC_INDICATOR_INPUT;
			break;
		case ATTR_TARGET_NOTCONVERTED:
		case ATTR_TARGET_CONVERTED:
			imeIndicator[i] = SC_INDICATOR_TARGET;
			break;
		case ATTR_CONVERTED:
			imeIndicator[i] = SC_INDICATOR_CONVERTED;
			break;
		default:
			imeIndicator[i] = SC_INDICATOR_UNKNOWN;
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

	if (lParam & GCS_COMPSTR) {
		const std::wstring wcs = imc.GetCompositionString(GCS_COMPSTR);
		if (wcs.empty()) {
			ShowCaretAtCurrentPosition();
			return 0;
		}

		if (initialCompose)
			ClearBeforeTentativeStart();
		pdoc->TentativeStart(); // TentativeActive from now on.

		std::vector<int> imeIndicator = MapImeIndicators(imc.GetImeAttributes());

		const int codePage = CodePageOfDocument();
		const std::wstring_view wsv = wcs;
		for (size_t i = 0; i < wsv.size(); ) {
			const size_t ucWidth = UTF16CharLength(wsv[i]);
			const std::string docChar = StringEncode(wsv.substr(i, ucWidth), codePage);

			InsertCharacter(docChar, CharacterSource::tentativeInput);

			DrawImeIndicator(imeIndicator[i], static_cast<unsigned int>(docChar.size()));
			i += ucWidth;
		}

		// Move IME caret from current last position to imeCaretPos.
		const int imeEndToImeCaretU16 = imc.GetImeCaretPos() - static_cast<unsigned int>(wcs.size());
		const Sci::Position imeCaretPosDoc = pdoc->GetRelativePositionUTF16(CurrentPosition(), imeEndToImeCaretU16);

		MoveImeCarets(- CurrentPosition() + imeCaretPosDoc);

		if (KoreanIME()) {
			view.imeCaretBlockOverride = true;
		}
	} else if (lParam & GCS_RESULTSTR) {
		AddWString(imc.GetCompositionString(GCS_RESULTSTR), CharacterSource::imeResult);
	}
	EnsureCaretVisible();
	SetCandidateWindowPos();
	ShowCaretAtCurrentPosition();
	return 0;
}

namespace {

// Translate message IDs from WM_* and EM_* to SCI_* so can partly emulate Windows Edit control
unsigned int SciMessageFromEM(unsigned int iMessage) noexcept {
	switch (iMessage) {
	case EM_CANPASTE: return SCI_CANPASTE;
	case EM_CANUNDO: return SCI_CANUNDO;
	case EM_EMPTYUNDOBUFFER: return SCI_EMPTYUNDOBUFFER;
	case EM_FINDTEXTEX: return SCI_FINDTEXT;
	case EM_FORMATRANGE: return SCI_FORMATRANGE;
	case EM_GETFIRSTVISIBLELINE: return SCI_GETFIRSTVISIBLELINE;
	case EM_GETLINECOUNT: return SCI_GETLINECOUNT;
	case EM_GETSELTEXT: return SCI_GETSELTEXT;
	case EM_GETTEXTRANGE: return SCI_GETTEXTRANGE;
	case EM_HIDESELECTION: return SCI_HIDESELECTION;
	case EM_LINEINDEX: return SCI_POSITIONFROMLINE;
	case EM_LINESCROLL: return SCI_LINESCROLL;
	case EM_REPLACESEL: return SCI_REPLACESEL;
	case EM_SCROLLCARET: return SCI_SCROLLCARET;
	case EM_SETREADONLY: return SCI_SETREADONLY;
	case WM_CLEAR: return SCI_CLEAR;
	case WM_COPY: return SCI_COPY;
	case WM_CUT: return SCI_CUT;
	case WM_SETTEXT: return SCI_SETTEXT;
	case WM_PASTE: return SCI_PASTE;
	case WM_UNDO: return SCI_UNDO;
	}
	return iMessage;
}

}

namespace Scintilla {

UINT CodePageFromCharSet(DWORD characterSet, UINT documentCodePage) noexcept {
	if (documentCodePage == SC_CP_UTF8) {
		return SC_CP_UTF8;
	}
	switch (characterSet) {
	case SC_CHARSET_ANSI: return 1252;
	
	// Cyrillic / Turkish or other languages cannot be shown in ANSI mode.
	// This fixes such problem. For more information about this fix, check:
	// https://github.com/notepad-plus-plus/notepad-plus-plus/issues/5671
	//case SC_CHARSET_DEFAULT: return documentCodePage ? documentCodePage : 1252;
	case SC_CHARSET_DEFAULT: return documentCodePage;
	
	case SC_CHARSET_BALTIC: return 1257;
	case SC_CHARSET_CHINESEBIG5: return 950;
	case SC_CHARSET_EASTEUROPE: return 1250;
	case SC_CHARSET_GB2312: return 936;
	case SC_CHARSET_GREEK: return 1253;
	case SC_CHARSET_HANGUL: return 949;
	case SC_CHARSET_MAC: return 10000;
	case SC_CHARSET_OEM: return 437;
	case SC_CHARSET_RUSSIAN: return 1251;
	case SC_CHARSET_SHIFTJIS: return 932;
	case SC_CHARSET_TURKISH: return 1254;
	case SC_CHARSET_JOHAB: return 1361;
	case SC_CHARSET_HEBREW: return 1255;
	case SC_CHARSET_ARABIC: return 1256;
	case SC_CHARSET_VIETNAMESE: return 1258;
	case SC_CHARSET_THAI: return 874;
	case SC_CHARSET_8859_15: return 28605;
	// Not supported
	case SC_CHARSET_CYRILLIC: return documentCodePage;
	case SC_CHARSET_SYMBOL: return documentCodePage;
	}
	return documentCodePage;
}

}

UINT ScintillaWin::CodePageOfDocument() const {
	return CodePageFromCharSet(vs.styles[STYLE_DEFAULT].characterSet, pdoc->dbcsCodePage);
}

sptr_t ScintillaWin::GetTextLength() {
	return pdoc->CountUTF16(0, pdoc->Length());
}

sptr_t ScintillaWin::GetText(uptr_t wParam, sptr_t lParam) {
	if (lParam == 0) {
		return pdoc->CountUTF16(0, pdoc->Length());
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
	Sci::Position sizeRequestedRange = pdoc->GetRelativePositionUTF16(0, lengthWanted);
	if (sizeRequestedRange < 0) {
		// Requested more text than there is in the document.
		sizeRequestedRange = pdoc->CountUTF16(0, pdoc->Length());
	}
	std::string docBytes(sizeRequestedRange, '\0');
	pdoc->GetCharRange(&docBytes[0], 0, sizeRequestedRange);
	if (IsUnicodeMode()) {
		const size_t uLen = UTF16FromUTF8(docBytes, ptr, lengthWanted);
		ptr[uLen] = L'\0';
		return uLen;
	} else {
		// Not Unicode mode
		// Convert to Unicode using the current Scintilla code page
		const UINT cpSrc = CodePageOfDocument();
		int lengthUTF16 = WideCharLenFromMultiByte(cpSrc, docBytes);
		if (lengthUTF16 > lengthWanted)
			lengthUTF16 = static_cast<int>(lengthWanted);
		WideCharFromMultiByte(cpSrc, docBytes, ptr, lengthUTF16);
		ptr[lengthUTF16] = L'\0';
		return lengthUTF16;
	}
}

sptr_t ScintillaWin::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	try {
		//Platform::DebugPrintf("S M:%x WP:%x L:%x\n", iMessage, wParam, lParam);
		iMessage = SciMessageFromEM(iMessage);
		switch (iMessage) {

		case WM_CREATE:
			ctrlID = ::GetDlgCtrlID(static_cast<HWND>(wMain.GetID()));
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
					return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
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

		case WM_SIZE: {
#if defined(USE_D2D)
				if (paintState == notPainting) {
					DropRenderTarget();
				} else {
					renderTargetValid = false;
				}
#endif
				//Platform::DebugPrintf("Scintilla WM_SIZE %d %d\n", LOWORD(lParam), HIWORD(lParam));
				ChangeSize();
			}
			break;

		case WM_MOUSEWHEEL:
			if (!mouseWheelCaptures) {
				// if the mouse wheel is not captured, test if the mouse
				// pointer is over the editor window and if not, don't
				// handle the message but pass it on.
				RECT rc;
				GetWindowRect(MainHWND(), &rc);
				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				if (!PtInRect(&rc, pt))
					return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
			}
			// if autocomplete list active then send mousewheel message to it
			if (ac.Active()) {
				HWND hWnd = static_cast<HWND>(ac.lb->GetID());
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
					wheelDelta = - (-wheelDelta % WHEEL_DELTA);

				if (wParam & MK_CONTROL) {
					// Zoom! We play with the font sizes in the styles.
					// Number of steps/line is ignored, we just care if sizing up or down
					if (linesToScroll < 0) {
						KeyCommand(SCI_ZOOMIN);
					} else {
						KeyCommand(SCI_ZOOMOUT);
					}
				} else {
					// Scroll
					ScrollTo(topLine + linesToScroll);
				}
			}
			return 0;

		case WM_TIMER:
			if (wParam == idleTimerID && idler.state) {
				SendMessage(MainHWND(), SC_WIN_IDLE, 0, 1);
			} else {
				TickFor(static_cast<TickReason>(wParam - fineTimerStart));
			}
			break;

		case SC_WIN_IDLE:
			// wParam=dwTickCountInitial, or 0 to initialize.  lParam=bSkipUserInputTest
			if (idler.state) {
				if (lParam || (WAIT_TIMEOUT == MsgWaitForMultipleObjects(0, nullptr, 0, 0, QS_INPUT|QS_HOTKEY))) {
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
						const DWORD maxWorkTime = 50;

						if (dwCurrent >= dwStart && dwCurrent > maxWorkTime && dwCurrent - maxWorkTime < dwStart)
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

		case WM_GETMINMAXINFO:
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_LBUTTONDOWN: {
			// For IME, set the composition string as the result string.
			IMContext imc(MainHWND());
			::ImmNotifyIME(imc.hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
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
		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT) {
				if (inDragDrop == ddDragging) {
					DisplayCursor(Window::cursorUp);
				} else {
					// Display regular (drag) cursor over selection
					POINT pt;
					if (0 != ::GetCursorPos(&pt)) {
						::ScreenToClient(MainHWND(), &pt);
						if (PointInSelMargin(PointFromPOINT(pt))) {
							DisplayCursor(GetMarginCursor(PointFromPOINT(pt)));
						} else if (PointInSelection(PointFromPOINT(pt)) && !SelectionEmpty()) {
							DisplayCursor(Window::cursorArrow);
						} else if (PointIsHotspot(PointFromPOINT(pt))) {
							DisplayCursor(Window::cursorHand);
						} else {
							// Fix the following issue:
							// #8588
							// It should be fixed in Scintilla release superior to v4.4.4 - to verify.
							// ++ added
							Sci::Position pos = PositionFromLocation(PointFromPOINT(pt), true, true);
							if ((pos != INVALID_POSITION) && (hoverIndicatorPos != Sci::invalidPosition))
								DisplayCursor(Window::cursorHand);
							else
							// -- added
								DisplayCursor(Window::cursorText);
						}
					}
				}
				return TRUE;
			} else {
				return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
			}

		case WM_CHAR:
			if (((wParam >= 128) || !iscntrl(static_cast<int>(wParam))) || !lastKeyDownConsumed) {
				wchar_t wcs[3] = {static_cast<wchar_t>(wParam), 0};
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
				AddWString(std::wstring_view(wcs, wclen), CharacterSource::directInput);
			}
			return 0;

		case WM_UNICHAR:
			if (wParam == UNICODE_NOCHAR) {
				return TRUE;
			} else if (lastKeyDownConsumed) {
				return 1;
			} else {
				wchar_t wcs[3] = {0};
				const unsigned int wclen = UTF16FromUTF32Character(static_cast<unsigned int>(wParam), wcs);
				AddWString(std::wstring_view(wcs, wclen), CharacterSource::directInput);
				return FALSE;
			}

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
			//Platform::DebugPrintf("S keydown %d %x %x %x %x\n",iMessage, wParam, lParam, ::IsKeyDown(VK_SHIFT), ::IsKeyDown(VK_CONTROL));
				lastKeyDownConsumed = false;
				const int ret = KeyDownWithModifiers(KeyTranslate(static_cast<int>(wParam)),
					ModifierFlags(KeyboardIsKeyDown(VK_SHIFT),
					KeyboardIsKeyDown(VK_CONTROL),
					KeyboardIsKeyDown(VK_MENU)),
					&lastKeyDownConsumed);
				if (!ret && !lastKeyDownConsumed) {
					return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
				}
				break;
			}

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

		case WM_KEYUP:
			//Platform::DebugPrintf("S keyup %d %x %x\n",iMessage, wParam, lParam);
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_SETTINGCHANGE:
			//Platform::DebugPrintf("Setting Changed\n");
			InvalidateStyleData();
			// Get Intellimouse scroll line parameters
			GetIntelliMouseParameters();
			break;

		case WM_GETDLGCODE:
			return DLGC_HASSETSEL | DLGC_WANTALLKEYS;

		case WM_KILLFOCUS: {
				HWND wOther = reinterpret_cast<HWND>(wParam);
				HWND wThis = MainHWND();
				const HWND wCT = static_cast<HWND>(ct.wCallTip.GetID());
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
			}
			break;

		case WM_SETFOCUS:
			SetFocusState(true);
			DestroySystemCaret();
			CreateSystemCaret();
			break;

		case WM_SYSCOLORCHANGE:
			//Platform::DebugPrintf("Setting Changed\n");
			InvalidateStyleData();
			break;

		case WM_IME_STARTCOMPOSITION: 	// dbcs
			if (KoreanIME() || imeInteraction == imeInline) {
				return 0;
			} else {
				ImeStartComposition();
				return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
			}

		case WM_IME_ENDCOMPOSITION: 	// dbcs
			ImeEndComposition();
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_IME_COMPOSITION:
			if (KoreanIME() || imeInteraction == imeInline) {
				return HandleCompositionInline(wParam, lParam);
			} else {
				return HandleCompositionWindowed(wParam, lParam);
			}

		case WM_CONTEXTMENU: {
				Point pt = PointFromLParam(lParam);
				POINT rpt = {static_cast<int>(pt.x), static_cast<int>(pt.y)};
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
			}
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
		case WM_INPUTLANGCHANGE:
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
		case WM_INPUTLANGCHANGEREQUEST:
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_ERASEBKGND:
			return 1;   // Avoid any background erasure as whole window painted.

		case WM_CAPTURECHANGED:
			capturedMouse = false;
			return 0;

		case WM_IME_SETCONTEXT:
			if (KoreanIME() || imeInteraction == imeInline) {
				if (wParam) {
					LPARAM NoImeWin = lParam;
					NoImeWin = NoImeWin & (~ISC_SHOWUICOMPOSITIONWINDOW);
					return ::DefWindowProc(MainHWND(), iMessage, wParam, NoImeWin);
				}
			}
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		// These are not handled in Scintilla and its faster to dispatch them here.
		// Also moves time out to here so profile doesn't count lots of empty message calls.

		case WM_MOVE:
		case WM_MOUSEACTIVATE:
		case WM_NCHITTEST:
		case WM_NCCALCSIZE:
		case WM_NCPAINT:
		case WM_NCMOUSEMOVE:
		case WM_NCLBUTTONDOWN:
		case WM_IME_NOTIFY:
		case WM_SYSCOMMAND:
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_GETTEXTLENGTH:
			return GetTextLength();

		case WM_GETTEXT:
			return GetText(wParam, lParam);

		case EM_LINEFROMCHAR:
			if (static_cast<int>(wParam) < 0) {
				wParam = SelectionStart().Position();
			}
			return pdoc->LineFromPosition(static_cast<int>(wParam));

		case EM_EXLINEFROMCHAR:
			return pdoc->LineFromPosition(static_cast<int>(lParam));

		case EM_GETSEL:
			if (wParam) {
				*reinterpret_cast<int *>(wParam) = static_cast<int>(SelectionStart().Position());
			}
			if (lParam) {
				*reinterpret_cast<int *>(lParam) = static_cast<int>(SelectionEnd().Position());
			}
			return MAKELRESULT(SelectionStart().Position(), SelectionEnd().Position());

		case EM_EXGETSEL: {
				if (lParam == 0) {
					return 0;
				}
				Sci_CharacterRange *pCR = reinterpret_cast<Sci_CharacterRange *>(lParam);
				pCR->cpMin = static_cast<Sci_PositionCR>(SelectionStart().Position());
				pCR->cpMax = static_cast<Sci_PositionCR>(SelectionEnd().Position());
			}
			break;

		case EM_SETSEL: {
				Sci::Position nStart = static_cast<Sci::Position>(wParam);
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
				const Sci_CharacterRange *pCR = reinterpret_cast<const Sci_CharacterRange *>(lParam);
				sel.selType = Selection::selStream;
				if (pCR->cpMin == 0 && pCR->cpMax == -1) {
					SetSelection(pCR->cpMin, pdoc->Length());
				} else {
					SetSelection(pCR->cpMin, pCR->cpMax);
				}
				EnsureCaretVisible();
				return pdoc->LineFromPosition(SelectionStart().Position());
			}

		case SCI_GETDIRECTFUNCTION:
			return reinterpret_cast<sptr_t>(DirectFunction);

		case SCI_GETDIRECTPOINTER:
			return reinterpret_cast<sptr_t>(this);

		case SCI_GRABFOCUS:
			::SetFocus(MainHWND());
			break;

#ifdef INCLUDE_DEPRECATED_FEATURES
		case SCI_SETKEYSUNICODE:
			break;

		case SCI_GETKEYSUNICODE:
			return true;
#endif

		case SCI_SETTECHNOLOGY:
			if ((wParam == SC_TECHNOLOGY_DEFAULT) ||
				(wParam == SC_TECHNOLOGY_DIRECTWRITERETAIN) ||
				(wParam == SC_TECHNOLOGY_DIRECTWRITEDC) ||
				(wParam == SC_TECHNOLOGY_DIRECTWRITE)) {
				const int technologyNew = static_cast<int>(wParam);
				if (technology != technologyNew) {
					if (technologyNew > SC_TECHNOLOGY_DEFAULT) {
#if defined(USE_D2D)
						if (!LoadD2D())
							// Failed to load Direct2D or DirectWrite so no effect
							return 0;
#else
						return 0;
#endif
					} else {
						bidirectional = EditModel::Bidirectional::bidiDisabled;
					}
#if defined(USE_D2D)
					DropRenderTarget();
#endif
					technology = technologyNew;
					// Invalidate all cached information including layout.
					DropGraphics(true);
					InvalidateStyleRedraw();
				}
			}
			break;

		case SCI_SETBIDIRECTIONAL:
			if (technology == SC_TECHNOLOGY_DEFAULT) {
				bidirectional = EditModel::Bidirectional::bidiDisabled;
			} else if (wParam <= SC_BIDIRECTIONAL_R2L) {
				bidirectional = static_cast<EditModel::Bidirectional>(wParam);
			}
			// Invalidate all cached information including layout.
			DropGraphics(true);
			InvalidateStyleRedraw();
			break;

#ifdef SCI_LEXER
		case SCI_LOADLEXERLIBRARY:
			LexerManager::GetInstance()->Load(ConstCharPtrFromSPtr(lParam));
			break;
#endif

		case SCI_TARGETASUTF8:
			return TargetAsUTF8(CharPtrFromSPtr(lParam));

		case SCI_ENCODEDFROMUTF8:
			return EncodedFromUTF8(ConstCharPtrFromUPtr(wParam),
				CharPtrFromSPtr(lParam));

		default:
			return ScintillaBase::WndProc(iMessage, wParam, lParam);
		}
	} catch (std::bad_alloc &) {
		errorStatus = SC_STATUS_BADALLOC;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return 0;
}

bool ScintillaWin::ValidCodePage(int codePage) const {
	return codePage == 0 || codePage == SC_CP_UTF8 ||
	       codePage == 932 || codePage == 936 || codePage == 949 ||
	       codePage == 950 || codePage == 1361;
}

sptr_t ScintillaWin::DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
}

bool ScintillaWin::FineTickerRunning(TickReason reason) {
	return timers[reason] != 0;
}

void ScintillaWin::FineTickerStart(TickReason reason, int millis, int tolerance) {
	FineTickerCancel(reason);
	if (SetCoalescableTimerFn && tolerance) {
		timers[reason] = SetCoalescableTimerFn(MainHWND(), fineTimerStart + reason, millis, nullptr, tolerance);
	} else {
		timers[reason] = ::SetTimer(MainHWND(), fineTimerStart + reason, millis, nullptr);
	}
}

void ScintillaWin::FineTickerCancel(TickReason reason) {
	if (timers[reason]) {
		::KillTimer(MainHWND(), timers[reason]);
		timers[reason] = 0;
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

void ScintillaWin::QueueIdleWork(WorkNeeded::workItems items, Sci::Position upTo) {
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
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = MainHWND();
		tme.dwHoverTime = HOVER_DEFAULT;	// Unused but triggers Dr. Memory if not initialized
		TrackMouseEvent(&tme);
	}
	trackedMouseLeave = on;
}

bool ScintillaWin::PaintContains(PRectangle rc) {
	if (paintState == painting) {
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
		if (HasCaretSizeChanged()) {
			DestroySystemCaret();
			CreateSystemCaret();
		}
		const Point pos = PointMainCaret();
		::SetCaretPos(static_cast<int>(pos.x), static_cast<int>(pos.y));
	}
}

int ScintillaWin::SetScrollInfo(int nBar, LPCSCROLLINFO lpsi, BOOL bRedraw) noexcept {
	return ::SetScrollInfo(MainHWND(), nBar, lpsi, bRedraw);
}

bool ScintillaWin::GetScrollInfo(int nBar, LPSCROLLINFO lpsi) noexcept {
	return ::GetScrollInfo(MainHWND(), nBar, lpsi) ? true : false;
}

// Change the scroll position but avoid repaint if changing to same value
void ScintillaWin::ChangeScrollPos(int barType, Sci::Position pos) {
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

bool ScintillaWin::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) {
	bool modified = false;
	SCROLLINFO sci = {
		sizeof(sci), 0, 0, 0, 0, 0, 0
	};
	sci.fMask = SIF_PAGE | SIF_RANGE;
	GetScrollInfo(SB_VERT, &sci);
	const Sci::Line vertEndPreferred = nMax;
	if (!verticalScrollBarVisible)
		nPage = vertEndPreferred + 1;
	if ((sci.nMin != 0) ||
		(sci.nMax != vertEndPreferred) ||
	        (sci.nPage != static_cast<unsigned int>(nPage)) ||
	        (sci.nPos != 0)) {
		sci.fMask = SIF_PAGE | SIF_RANGE;
		sci.nMin = 0;
		sci.nMax = static_cast<int>(vertEndPreferred);
		sci.nPage = static_cast<UINT>(nPage);
		sci.nPos = 0;
		sci.nTrackPos = 1;
		SetScrollInfo(SB_VERT, &sci, TRUE);
		modified = true;
	}

	const PRectangle rcText = GetTextRectangle();
	int horizEndPreferred = scrollWidth;
	if (horizEndPreferred < 0)
		horizEndPreferred = 0;
	int pageWidth = static_cast<int>(rcText.Width());
	if (!horizontalScrollBarVisible || Wrapping())
		pageWidth = horizEndPreferred + 1;
	sci.fMask = SIF_PAGE | SIF_RANGE;
	GetScrollInfo(SB_HORZ, &sci);
	if ((sci.nMin != 0) ||
		(sci.nMax != horizEndPreferred) ||
		(sci.nPage != static_cast<unsigned int>(pageWidth)) ||
	        (sci.nPos != 0)) {
		sci.fMask = SIF_PAGE | SIF_RANGE;
		sci.nMin = 0;
		sci.nMax = horizEndPreferred;
		sci.nPage = pageWidth;
		sci.nPos = 0;
		sci.nTrackPos = 1;
		SetScrollInfo(SB_HORZ, &sci, TRUE);
		modified = true;
		if (scrollWidth < pageWidth) {
			HorizontalScrollTo(0);
		}
	}
	return modified;
}

void ScintillaWin::NotifyChange() {
	::SendMessage(::GetParent(MainHWND()), WM_COMMAND,
	        MAKEWPARAM(GetCtrlID(), SCEN_CHANGE),
		reinterpret_cast<LPARAM>(MainHWND()));
}

void ScintillaWin::NotifyFocus(bool focus) {
	if (commandEvents) {
		::SendMessage(::GetParent(MainHWND()), WM_COMMAND,
			MAKEWPARAM(GetCtrlID(), focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS),
			reinterpret_cast<LPARAM>(MainHWND()));
	}
	Editor::NotifyFocus(focus);
}

void ScintillaWin::SetCtrlID(int identifier) {
	::SetWindowID(static_cast<HWND>(wMain.GetID()), identifier);
}

int ScintillaWin::GetCtrlID() {
	return ::GetDlgCtrlID(static_cast<HWND>(wMain.GetID()));
}

void ScintillaWin::NotifyParent(SCNotification scn) {
	scn.nmhdr.hwndFrom = MainHWND();
	scn.nmhdr.idFrom = GetCtrlID();
	::SendMessage(::GetParent(MainHWND()), WM_NOTIFY,
	              GetCtrlID(), reinterpret_cast<LPARAM>(&scn));
}

void ScintillaWin::NotifyDoubleClick(Point pt, int modifiers) {
	//Platform::DebugPrintf("ScintillaWin Double click 0\n");
	ScintillaBase::NotifyDoubleClick(pt, modifiers);
	// Send myself a WM_LBUTTONDBLCLK, so the container can handle it too.
	::SendMessage(MainHWND(),
			  WM_LBUTTONDBLCLK,
			  (modifiers & SCI_SHIFT) ? MK_SHIFT : 0,
			  MAKELPARAM(pt.x, pt.y));
}

class CaseFolderDBCS : public CaseFolderTable {
	// Allocate the expandable storage here so that it does not need to be reallocated
	// for each call to Fold.
	std::vector<wchar_t> utf16Mixed;
	std::vector<wchar_t> utf16Folded;
	UINT cp;
public:
	explicit CaseFolderDBCS(UINT cp_) : cp(cp_) {
		StandardASCII();
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
				const char *foldedUTF8 = CaseConvert(utf16Mixed[mixIndex], CaseConversionFold);
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

CaseFolder *ScintillaWin::CaseFolderForEncoding() {
	const UINT cpDest = CodePageOfDocument();
	if (cpDest == SC_CP_UTF8) {
		return new CaseFolderUnicode();
	} else {
		if (pdoc->dbcsCodePage == 0) {
			CaseFolderTable *pcf = new CaseFolderTable();
			pcf->StandardASCII();
			// Only for single byte encodings
			for (int i=0x80; i<0x100; i++) {
				char sCharacter[2] = "A";
				sCharacter[0] = static_cast<char>(i);
				wchar_t wCharacter[20];
				const unsigned int lengthUTF16 = WideCharFromMultiByte(cpDest, sCharacter,
					wCharacter, std::size(wCharacter));
				if (lengthUTF16 == 1) {
					const char *caseFolded = CaseConvert(wCharacter[0], CaseConversionFold);
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
			return new CaseFolderDBCS(cpDest);
		}
	}
}

std::string ScintillaWin::CaseMapString(const std::string &s, int caseMapping) {
	if ((s.size() == 0) || (caseMapping == cmSame))
		return s;

	const UINT cpDoc = CodePageOfDocument();
	if (cpDoc == SC_CP_UTF8) {
		return CaseConvertString(s, (caseMapping == cmUpper) ? CaseConversionUpper : CaseConversionLower);
	}

	// Change text to UTF-16
	const std::wstring wsText = StringDecode(s, cpDoc);

	const DWORD mapFlags = LCMAP_LINGUISTIC_CASING |
		((caseMapping == cmUpper) ? LCMAP_UPPERCASE : LCMAP_LOWERCASE);

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

void ScintillaWin::CopyAllowLine() {
	SelectionText selectedText;
	CopySelectionRange(&selectedText, true);
	CopyToClipboard(selectedText);
}

bool ScintillaWin::CanPaste() {
	if (!Editor::CanPaste())
		return false;
	if (::IsClipboardFormatAvailable(CF_TEXT))
		return true;
	if (IsUnicodeMode())
		return ::IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
	return false;
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

}

void ScintillaWin::Paste() {
	if (!::OpenClipboardRetry(MainHWND())) {
		return;
	}
	UndoGroup ug(pdoc);
	const bool isLine = SelectionEmpty() &&
		(::IsClipboardFormatAvailable(cfLineSelect) || ::IsClipboardFormatAvailable(cfVSLineTag));
	ClearSelection(multiPasteMode == SC_MULTIPASTE_EACH);
	bool isRectangular = (::IsClipboardFormatAvailable(cfColumnSelect) != 0);

	if (!isRectangular) {
		// Evaluate "Borland IDE Block Type" explicitly
		GlobalMemory memBorlandSelection(::GetClipboardData(cfBorlandIDEBlockType));
		if (memBorlandSelection) {
			isRectangular = (memBorlandSelection.Size() == 1) && (static_cast<BYTE *>(memBorlandSelection.ptr)[0] == 0x02);
			memBorlandSelection.Unlock();
		}
	}
	const PasteShape pasteShape = isRectangular ? pasteRectangular : (isLine ? pasteLine : pasteStream);

	// Always use CF_UNICODETEXT if available
	GlobalMemory memUSelection(::GetClipboardData(CF_UNICODETEXT));
	if (memUSelection) {
		const wchar_t *uptr = static_cast<const wchar_t *>(memUSelection.ptr);
		if (uptr) {
			size_t len;
			std::vector<char> putf;
			// Default Scintilla behaviour in Unicode mode
			if (IsUnicodeMode()) {
				const size_t bytes = memUSelection.Size();
				const std::wstring_view wsv(uptr, bytes / 2);
				len = UTF8Length(wsv);
				putf.resize(len + 1);
				UTF8FromUTF16(wsv, &putf[0], len);
			} else {
				// CF_UNICODETEXT available, but not in Unicode mode
				// Convert from Unicode to current Scintilla code page
				const UINT cpDest = CodePageOfDocument();
				len = MultiByteLenFromWideChar(cpDest, uptr);
				putf.resize(len);
				MultiByteFromWideChar(cpDest, uptr, &putf[0], len);
			}

			InsertPasteShape(&putf[0], len, pasteShape);
		}
		memUSelection.Unlock();
	} else {
		// CF_UNICODETEXT not available, paste ANSI text
		GlobalMemory memSelection(::GetClipboardData(CF_TEXT));
		if (memSelection) {
			const char *ptr = static_cast<const char *>(memSelection.ptr);
			if (ptr) {
				const size_t bytes = memSelection.Size();
				size_t len = bytes;
				for (size_t i = 0; i < bytes; i++) {
					if ((len == bytes) && (0 == ptr[i]))
						len = i;
				}

				// In Unicode mode, convert clipboard text to UTF-8
				if (IsUnicodeMode()) {
					std::vector<wchar_t> uptr(len+1);

					const size_t ulen = WideCharFromMultiByte(CP_ACP,
					                    std::string_view(ptr, len), &uptr[0], len + 1);

					const std::wstring_view wsv(&uptr[0], ulen);
					const size_t mlen = UTF8Length(wsv);
					std::vector<char> putf(mlen+1);
					UTF8FromUTF16(wsv, &putf[0], mlen);

					InsertPasteShape(&putf[0], mlen, pasteShape);
				} else {
					InsertPasteShape(ptr, len, pasteShape);
				}
			}
			memSelection.Unlock();
		}
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
	unsigned int putPos = 0;
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

FormatEnumerator::FormatEnumerator(int pos_, CLIPFORMAT formats_[], size_t formatsLen_) {
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
	if (pd->sci->DragIsRectangularOK(pFE->cfFormat) &&
	    pFE->ptd == nullptr &&
	    (pFE->dwAspect & DVASPECT_CONTENT) != 0 &&
	    pFE->lindex == -1 &&
	    (pFE->tymed & TYMED_HGLOBAL) != 0
	) {
		return S_OK;
	}

	const bool formatOK = (pFE->cfFormat == CF_TEXT) ||
		((pFE->cfFormat == CF_UNICODETEXT) && pd->sci->IsUnicodeMode());
	if (!formatOK ||
	    pFE->ptd != nullptr ||
	    (pFE->dwAspect & DVASPECT_CONTENT) == 0 ||
	    pFE->lindex != -1 ||
	    (pFE->tymed & TYMED_HGLOBAL) == 0
	) {
		//Platform::DebugPrintf("DOB QueryGetData No %x\n",pFE->cfFormat);
		//return DATA_E_FORMATETC;
		return S_FALSE;
	}
	//Platform::DebugPrintf("DOB QueryGetData OK %x\n",pFE->cfFormat);
	return S_OK;
}

STDMETHODIMP DataObject_GetCanonicalFormatEtc(DataObject *pd, FORMATETC *, FORMATETC *pFEOut) {
	//Platform::DebugPrintf("DOB GetCanon\n");
	if (pd->sci->IsUnicodeMode())
		pFEOut->cfFormat = CF_UNICODETEXT;
	else
		pFEOut->cfFormat = CF_TEXT;
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
		FormatEnumerator *pfe;
		if (pd->sci->IsUnicodeMode()) {
			CLIPFORMAT formats[] = {CF_UNICODETEXT, CF_TEXT};
			pfe = new FormatEnumerator(0, formats, std::size(formats));
		} else {
			CLIPFORMAT formats[] = {CF_TEXT};
			pfe = new FormatEnumerator(0, formats, std::size(formats));
		}
		return FormatEnumerator_QueryInterface(pfe, IID_IEnumFORMATETC,
											   reinterpret_cast<void **>(ppEnum));
	} catch (std::bad_alloc &) {
		pd->sci->errorStatus = SC_STATUS_BADALLOC;
		return E_OUTOFMEMORY;
	} catch (...) {
		pd->sci->errorStatus = SC_STATUS_FAILURE;
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
		dt->sci->errorStatus = SC_STATUS_FAILURE;
	}
	return E_FAIL;
}
STDMETHODIMP DropTarget_DragOver(DropTarget *dt, DWORD grfKeyState, POINTL pt, PDWORD pdwEffect) {
	try {
		return dt->sci->DragOver(grfKeyState, pt, pdwEffect);
	} catch (...) {
		dt->sci->errorStatus = SC_STATUS_FAILURE;
	}
	return E_FAIL;
}
STDMETHODIMP DropTarget_DragLeave(DropTarget *dt) {
	try {
		return dt->sci->DragLeave();
	} catch (...) {
		dt->sci->errorStatus = SC_STATUS_FAILURE;
	}
	return E_FAIL;
}
STDMETHODIMP DropTarget_Drop(DropTarget *dt, LPDATAOBJECT pIDataSource, DWORD grfKeyState,
                             POINTL pt, PDWORD pdwEffect) {
	try {
		return dt->sci->Drop(pIDataSource, grfKeyState, pt, pdwEffect);
	} catch (...) {
		dt->sci->errorStatus = SC_STATUS_FAILURE;
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
			int sizeZoomed = vs.styles[styleHere].size + vs.zoomLevel * SC_FONT_SIZE_MULTIPLIER;
			if (sizeZoomed <= 2 * SC_FONT_SIZE_MULTIPLIER)	// Hangs if sizeZoomed <= 1
				sizeZoomed = 2 * SC_FONT_SIZE_MULTIPLIER;
			AutoSurface surface(this);
			int deviceHeight = sizeZoomed;
			if (surface) {
				deviceHeight = (sizeZoomed * surface->LogPixelsY()) / 72;
			}
			// The negative is to allow for leading
			lf.lfHeight = -(std::abs(deviceHeight / SC_FONT_SIZE_MULTIPLIER));
			lf.lfWeight = vs.styles[styleHere].weight;
			lf.lfItalic = static_cast<BYTE>(vs.styles[styleHere].italic ? 1 : 0);
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
	// This retrieves the number of lines per scroll as configured inthe Mouse Properties sheet in Control Panel
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &linesPerScroll, 0);
}

void ScintillaWin::CopyToClipboard(const SelectionText &selectedText) {
	if (!::OpenClipboardRetry(MainHWND())) {
		return;
	}
	::EmptyClipboard();

	GlobalMemory uniText;

	// Default Scintilla behaviour in Unicode mode
	if (IsUnicodeMode()) {
                const std::string_view sv(selectedText.Data(), selectedText.LengthWithTerminator());
		const size_t uchars = UTF16Length(sv);
		uniText.Allocate(2 * uchars);
		if (uniText) {
			UTF16FromUTF8(sv,
				static_cast<wchar_t *>(uniText.ptr), uchars);
		}
	} else {
		// Not Unicode mode
		// Convert to Unicode using the current Scintilla code page
		const UINT cpSrc = CodePageFromCharSet(
					selectedText.characterSet, selectedText.codePage);
		const std::string_view svSelected(selectedText.Data(), selectedText.LengthWithTerminator());
		const size_t uLen = WideCharLenFromMultiByte(cpSrc, svSelected);
		uniText.Allocate(2 * uLen);
		if (uniText) {
			WideCharFromMultiByte(cpSrc, svSelected,
				static_cast<wchar_t *>(uniText.ptr), uLen);
		}
	}

	if (uniText) {
		uniText.SetClip(CF_UNICODETEXT);
	} else {
		// There was a failure - try to copy at least ANSI text
		GlobalMemory ansiText;
		ansiText.Allocate(selectedText.LengthWithTerminator());
		if (ansiText) {
			memcpy(ansiText.ptr, selectedText.Data(), selectedText.LengthWithTerminator());
			ansiText.SetClip(CF_TEXT);
		}
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
			SCROLLINFO si;
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
	if ((technology == SC_TECHNOLOGY_DEFAULT) || (technology == SC_TECHNOLOGY_DIRECTWRITEDC)) {
		HDC hdc = ::GetDC(MainHWND());
		FullPaintDC(hdc);
		::ReleaseDC(MainHWND(), hdc);
	} else {
		FullPaintDC(0);
	}
}

/**
 * Redraw all of text area on the specified DC.
 * This paint will not be abandoned.
 */
void ScintillaWin::FullPaintDC(HDC hdc) {
	paintState = painting;
	rcPaint = GetClientRectangle();
	paintingAllText = true;
	if (technology == SC_TECHNOLOGY_DEFAULT) {
		AutoSurface surfaceWindow(hdc, this);
		if (surfaceWindow) {
			Paint(surfaceWindow, rcPaint);
			surfaceWindow->Release();
		}
	} else {
#if defined(USE_D2D)
		EnsureRenderTarget(hdc);
		AutoSurface surfaceWindow(pRenderTarget, this);
		if (surfaceWindow) {
			pRenderTarget->BeginDraw();
			Paint(surfaceWindow, rcPaint);
			surfaceWindow->Release();
			const HRESULT hr = pRenderTarget->EndDraw();
			if (hr == static_cast<HRESULT>(D2DERR_RECREATE_TARGET)) {
				DropRenderTarget();
			}
		}
#endif
	}
	paintState = notPainting;
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
	if (inDragDrop == ddDragging)	// Internal defaults to move
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
	if (!hasOKText) {
		FORMATETC fmte = {CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		const HRESULT hrHasText = pIDataSource->QueryGetData(&fmte);
		hasOKText = (hrHasText == S_OK);
	}
	if (!hasOKText) {
		*pdwEffect = DROPEFFECT_NONE;
		return S_OK;
	}

	*pdwEffect = EffectFromState(grfKeyState);
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
		errorStatus = SC_STATUS_FAILURE;
	}
	return E_FAIL;
}

STDMETHODIMP ScintillaWin::DragLeave() {
	try {
		SetDragPosition(SelectionPosition(Sci::invalidPosition));
		return S_OK;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
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

		STGMEDIUM medium {};

		std::vector<char> data;	// Includes terminating NUL

		FORMATETC fmtu = {CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		HRESULT hr = pIDataSource->GetData(&fmtu, &medium);
		if (SUCCEEDED(hr) && medium.hGlobal) {
			GlobalMemory memUDrop(medium.hGlobal);
			const wchar_t *udata = static_cast<const wchar_t *>(memUDrop.ptr);
			if (udata) {
				if (IsUnicodeMode()) {
					const size_t tlen = memUDrop.Size();
					// Convert UTF-16 to UTF-8
					const std::wstring_view wsv(udata, tlen / 2);
					const size_t dataLen = UTF8Length(wsv);
					data.resize(dataLen);
					UTF8FromUTF16(wsv, &data[0], dataLen);
				} else {
					// Convert UTF-16 to ANSI
					//
					// Default Scintilla behavior in Unicode mode
					// CF_UNICODETEXT available, but not in Unicode mode
					// Convert from Unicode to current Scintilla code page
					const UINT cpDest = CodePageOfDocument();
					const int tlen = MultiByteLenFromWideChar(cpDest, udata);
					data.resize(tlen);
					MultiByteFromWideChar(cpDest, udata, &data[0], tlen);
				}
			}
			memUDrop.Unlock();
		} else {
			FORMATETC fmte = {CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			hr = pIDataSource->GetData(&fmte, &medium);
			if (SUCCEEDED(hr) && medium.hGlobal) {
				GlobalMemory memDrop(medium.hGlobal);
				const char *cdata = static_cast<const char *>(memDrop.ptr);
				if (cdata)
					data.assign(cdata, cdata+strlen(cdata));
				memDrop.Unlock();
			}
		}

		if (!SUCCEEDED(hr) || data.empty()) {
			//Platform::DebugPrintf("Bad data format: 0x%x\n", hres);
			return hr;
		}

		FORMATETC fmtr = {cfColumnSelect, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		const HRESULT hrRectangular = pIDataSource->QueryGetData(&fmtr);

		POINT rpt = {pt.x, pt.y};
		::ScreenToClient(MainHWND(), &rpt);
		const SelectionPosition movePos = SPositionFromLocation(PointFromPOINT(rpt), false, false, UserVirtualSpace());

		DropAt(movePos, &data[0], data.size(), *pdwEffect == DROPEFFECT_MOVE, hrRectangular == S_OK);

		// Free data
		if (medium.pUnkForRelease)
			medium.pUnkForRelease->Release();
		else
			::GlobalFree(medium.hGlobal);

		return S_OK;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return E_FAIL;
}

/// Implement important part of IDataObject
STDMETHODIMP ScintillaWin::GetData(FORMATETC *pFEIn, STGMEDIUM *pSTM) {
	const bool formatOK = (pFEIn->cfFormat == CF_TEXT) ||
		((pFEIn->cfFormat == CF_UNICODETEXT) && IsUnicodeMode());
	if (!formatOK ||
	    pFEIn->ptd != nullptr ||
	    (pFEIn->dwAspect & DVASPECT_CONTENT) == 0 ||
	    pFEIn->lindex != -1 ||
	    (pFEIn->tymed & TYMED_HGLOBAL) == 0
	) {
		//Platform::DebugPrintf("DOB GetData No %d %x %x fmt=%x\n", lenDrag, pFEIn, pSTM, pFEIn->cfFormat);
		return DATA_E_FORMATETC;
	}
	pSTM->tymed = TYMED_HGLOBAL;
	//Platform::DebugPrintf("DOB GetData OK %d %x %x\n", lenDrag, pFEIn, pSTM);

	GlobalMemory text;
	if (pFEIn->cfFormat == CF_UNICODETEXT) {
                const std::string_view sv(drag.Data(), drag.LengthWithTerminator());
		const size_t uchars = UTF16Length(sv);
		text.Allocate(2 * uchars);
		if (text) {
			UTF16FromUTF8(sv,
				static_cast<wchar_t *>(text.ptr), uchars);
		}
	} else {
		text.Allocate(drag.LengthWithTerminator());
		if (text) {
			memcpy(text.ptr, drag.Data(), drag.LengthWithTerminator());
		}
	}
	pSTM->hGlobal = text ? text.Unlock() : 0;
	pSTM->pUnkForRelease = nullptr;
	return S_OK;
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
	bool result = 0 != scintillaClassAtom;

	if (result) {
		// Register the CallTip class
		WNDCLASSEX wndclassc {};
		wndclassc.cbSize = sizeof(wndclassc);
		wndclassc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
		wndclassc.cbWndExtra = sizeof(ScintillaWin *);
		wndclassc.hInstance = hInstance;
		wndclassc.lpfnWndProc = ScintillaWin::CTWndProc;
		wndclassc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wndclassc.lpszClassName = callClassName;

		callClassAtom = ::RegisterClassEx(&wndclassc);
		result = 0 != callClassAtom;
	}

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
		( (0 != vs.caretWidth) && (sysCaretWidth != vs.caretWidth) )
		|| ((0 != vs.lineHeight) && (sysCaretHeight != vs.lineHeight))
		) {
		return true;
	}
	return false;
}

BOOL ScintillaWin::CreateSystemCaret() {
	sysCaretWidth = vs.caretWidth;
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
	if (technology == SC_TECHNOLOGY_DEFAULT) {
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
		sysCaretBitmap = 0;
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
				::SetWindowLong(hWnd, 0, 0);
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
				// Create a Direct2D render target.
				if (sciThis->technology == SC_TECHNOLOGY_DEFAULT) {
					surfaceWindow->Init(ps.hdc, hWnd);
				} else {
#if defined(USE_D2D)
					D2D1_HWND_RENDER_TARGET_PROPERTIES dhrtp;
					dhrtp.hwnd = hWnd;
					dhrtp.pixelSize = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
					dhrtp.presentOptions = (sciThis->technology == SC_TECHNOLOGY_DIRECTWRITERETAIN) ?
						D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS : D2D1_PRESENT_OPTIONS_NONE;

					D2D1_RENDER_TARGET_PROPERTIES drtp;
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
					surfaceWindow->Init(pCTRenderTarget, hWnd);
					pCTRenderTarget->BeginDraw();
#endif
				}
				surfaceWindow->SetUnicodeMode(SC_CP_UTF8 == sciThis->ct.codePage);
				surfaceWindow->SetDBCSMode(sciThis->ct.codePage);
				surfaceWindow->SetBidiR2L(sciThis->BidirectionalR2L());
				sciThis->ct.PaintCT(surfaceWindow.get());
#if defined(USE_D2D)
				if (pCTRenderTarget)
					pCTRenderTarget->EndDraw();
#endif
				surfaceWindow->Release();
#if defined(USE_D2D)
				if (pCTRenderTarget)
					pCTRenderTarget->Release();
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
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
}

sptr_t ScintillaWin::DirectFunction(
    sptr_t ptr, UINT iMessage, uptr_t wParam, sptr_t lParam) {
	PLATFORM_ASSERT(::GetCurrentThreadId() == ::GetWindowThreadProcessId(reinterpret_cast<ScintillaWin *>(ptr)->MainHWND(), nullptr));
	return reinterpret_cast<ScintillaWin *>(ptr)->WndProc(iMessage, wParam, lParam);
}

namespace Scintilla {

sptr_t DirectFunction(
    ScintillaWin *sci, UINT iMessage, uptr_t wParam, sptr_t lParam) {
	return sci->WndProc(iMessage, wParam, lParam);
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
				// Create C++ object associated with window
				sci = new ScintillaWin(hWnd);
				SetWindowPointer(hWnd, sci);
				return sci->WndProc(iMessage, wParam, lParam);
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
			::SetWindowLong(hWnd, 0, 0);
			return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
		} else {
			return sci->WndProc(iMessage, wParam, lParam);
		}
	}
}

// This function is externally visible so it can be called from container when building statically.
// Must be called once only.
int Scintilla_RegisterClasses(void *hInstance) {
	Platform_Initialise(hInstance);
	const bool result = ScintillaWin::Register(static_cast<HINSTANCE>(hInstance));
#ifdef SCI_LEXER
	Scintilla_LinkLexers();
#endif
	return result;
}

namespace Scintilla {

int ResourcesRelease(bool fromDllMain) {
	const bool result = ScintillaWin::Unregister();
	Platform_Finalise(fromDllMain);
	return result;
}

}

// This function is externally visible so it can be called from container when building statically.
int Scintilla_ReleaseResources() {
	return Scintilla::ResourcesRelease(false);
}
