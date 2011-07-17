// Scintilla source code edit control
/** @file ScintillaWin.cxx
 ** Windows specific subclass of ScintillaBase.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <new>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>

#include <string>
#include <vector>

#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0500
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <windowsx.h>

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#ifdef SCI_LEXER
#include "SciLexer.h"
#include "LexerModule.h"
#endif
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "AutoComplete.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "Editor.h"
#include "ScintillaBase.h"
#include "UniConversion.h"

#ifdef SCI_LEXER
#include "ExternalLexer.h"
#endif

#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES   104
#endif

#ifndef WM_UNICHAR
#define WM_UNICHAR                      0x0109
#endif

#ifndef UNICODE_NOCHAR
#define UNICODE_NOCHAR                  0xFFFF
#endif

#ifndef WM_IME_STARTCOMPOSITION
#include <imm.h>
#endif

#include <commctrl.h>
#ifndef __DMC__
#include <zmouse.h>
#endif
#include <ole2.h>

#ifndef MK_ALT
#define MK_ALT 32
#endif

#define SC_WIN_IDLE 5001

// Functions imported from PlatWin
extern bool IsNT();
extern void Platform_Initialise(void *hInstance);
extern void Platform_Finalise();

typedef BOOL (WINAPI *TrackMouseEventSig)(LPTRACKMOUSEEVENT);

// GCC has trouble with the standard COM ABI so do it the old C way with explicit vtables.

const TCHAR scintillaClassName[] = TEXT("Scintilla");
const TCHAR callClassName[] = TEXT("CallTip");

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

// Take care of 32/64 bit pointers
#ifdef GetWindowLongPtr
static void *PointerFromWindow(HWND hWnd) {
	return reinterpret_cast<void *>(::GetWindowLongPtr(hWnd, 0));
}
static void SetWindowPointer(HWND hWnd, void *ptr) {
	::SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(ptr));
}
static void SetWindowID(HWND hWnd, int identifier) {
	::SetWindowLongPtr(hWnd, GWLP_ID, identifier);
}
#else
static void *PointerFromWindow(HWND hWnd) {
	return reinterpret_cast<void *>(::GetWindowLong(hWnd, 0));
}
static void SetWindowPointer(HWND hWnd, void *ptr) {
	::SetWindowLong(hWnd, 0, reinterpret_cast<LONG>(ptr));
}
static void SetWindowID(HWND hWnd, int identifier) {
	::SetWindowLong(hWnd, GWL_ID, identifier);
}
#endif

class ScintillaWin; 	// Forward declaration for COM interface subobjects

typedef void VFunction(void);

/**
 */
class FormatEnumerator {
public:
	VFunction **vtbl;
	int ref;
	int pos;
	CLIPFORMAT formats[2];
	int formatsLen;
	FormatEnumerator(int pos_, CLIPFORMAT formats_[], int formatsLen_);
};

/**
 */
class DropSource {
public:
	VFunction **vtbl;
	ScintillaWin *sci;
	DropSource();
};

/**
 */
class DataObject {
public:
	VFunction **vtbl;
	ScintillaWin *sci;
	DataObject();
};

/**
 */
class DropTarget {
public:
	VFunction **vtbl;
	ScintillaWin *sci;
	DropTarget();
};

/**
 */
class ScintillaWin :
	public ScintillaBase {

	bool lastKeyDownConsumed;

	bool capturedMouse;
	bool trackedMouseLeave;
	TrackMouseEventSig TrackMouseEventFn;

	unsigned int linesPerScroll;	///< Intellimouse support
	int wheelDelta; ///< Wheel delta from roll

	HRGN hRgnUpdate;

	bool hasOKText;

	CLIPFORMAT cfColumnSelect;
	CLIPFORMAT cfLineSelect;

	HRESULT hrOle;
	DropSource ds;
	DataObject dob;
	DropTarget dt;

	static HINSTANCE hInstance;

	ScintillaWin(HWND hwnd);
	ScintillaWin(const ScintillaWin &);
	virtual ~ScintillaWin();
	ScintillaWin &operator=(const ScintillaWin &);

	virtual void Initialise();
	virtual void Finalise();
	HWND MainHWND();

	static sptr_t DirectFunction(
		    ScintillaWin *sci, UINT iMessage, uptr_t wParam, sptr_t lParam);
	static sptr_t PASCAL SWndProc(
		    HWND hWnd, UINT iMessage, WPARAM wParam, sptr_t lParam);
	static sptr_t PASCAL CTWndProc(
		    HWND hWnd, UINT iMessage, WPARAM wParam, sptr_t lParam);

	enum { invalidTimerID, standardTimerID, idleTimerID };

	virtual bool DragThreshold(Point ptStart, Point ptNow);
	virtual void StartDrag();
	sptr_t WndPaint(uptr_t wParam);
	sptr_t HandleComposition(uptr_t wParam, sptr_t lParam);
	UINT CodePageOfDocument();
	virtual bool ValidCodePage(int codePage) const;
	virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	virtual bool SetIdle(bool on);
	virtual void SetTicking(bool on);
	virtual void SetMouseCapture(bool on);
	virtual bool HaveMouseCapture();
	virtual void SetTrackMouseLeaveEvent(bool on);
	virtual bool PaintContains(PRectangle rc);
	virtual void ScrollText(int linesToMove);
	virtual void UpdateSystemCaret();
	virtual void SetVerticalScrollPos();
	virtual void SetHorizontalScrollPos();
	virtual bool ModifyScrollBars(int nMax, int nPage);
	virtual void NotifyChange();
	virtual void NotifyFocus(bool focus);
	virtual void SetCtrlID(int identifier);
	virtual int GetCtrlID();
	virtual void NotifyParent(SCNotification scn);
	virtual void NotifyDoubleClick(Point pt, bool shift, bool ctrl, bool alt);
	virtual CaseFolder *CaseFolderForEncoding();
	virtual std::string CaseMapString(const std::string &s, int caseMapping);
	virtual void Copy();
	virtual void CopyAllowLine();
	virtual bool CanPaste();
	virtual void Paste();
	virtual void CreateCallTipWindow(PRectangle rc);
	virtual void AddToPopUp(const char *label, int cmd = 0, bool enabled = true);
	virtual void ClaimSelection();

	// DBCS
	void ImeStartComposition();
	void ImeEndComposition();

	void AddCharBytes(char b0, char b1);

	void GetIntelliMouseParameters();
	virtual void CopyToClipboard(const SelectionText &selectedText);
	void ScrollMessage(WPARAM wParam);
	void HorizontalScrollMessage(WPARAM wParam);
	void RealizeWindowPalette(bool inBackGround);
	void FullPaint();
	void FullPaintDC(HDC dc);
	bool IsCompatibleDC(HDC dc);
	DWORD EffectFromState(DWORD grfKeyState);

	virtual int SetScrollInfo(int nBar, LPCSCROLLINFO lpsi, BOOL bRedraw);
	virtual bool GetScrollInfo(int nBar, LPSCROLLINFO lpsi);
	void ChangeScrollPos(int barType, int pos);

	void InsertPasteText(const char *text, int len, SelectionPosition selStart, bool isRectangular, bool isLine);

public:
	// Public for benefit of Scintilla_DirectFunction
	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

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

	static bool Register(HINSTANCE hInstance_);
	static bool Unregister();

	friend class DropSource;
	friend class DataObject;
	friend class DropTarget;
	bool DragIsRectangularOK(CLIPFORMAT fmt) {
		return drag.rectangular && (fmt == cfColumnSelect);
	}

private:
	// For use in creating a system caret
	bool HasCaretSizeChanged();
	BOOL CreateSystemCaret();
	BOOL DestroySystemCaret();
	HBITMAP sysCaretBitmap;
	int sysCaretWidth;
	int sysCaretHeight;
	bool keysAlwaysUnicode;
};

HINSTANCE ScintillaWin::hInstance = 0;

ScintillaWin::ScintillaWin(HWND hwnd) {

	lastKeyDownConsumed = false;

	capturedMouse = false;
	trackedMouseLeave = false;
	TrackMouseEventFn = 0;

	linesPerScroll = 0;
	wheelDelta = 0;   // Wheel delta from roll

	hRgnUpdate = 0;

	hasOKText = false;

	// There does not seem to be a real standard for indicating that the clipboard
	// contains a rectangular selection, so copy Developer Studio.
	cfColumnSelect = static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(TEXT("MSDEVColumnSelect")));

	// Likewise for line-copy (copies a full line when no text is selected)
	cfLineSelect = static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(TEXT("MSDEVLineSelect")));

	hrOle = E_FAIL;

	wMain = hwnd;

	dob.sci = this;
	ds.sci = this;
	dt.sci = this;

	sysCaretBitmap = 0;
	sysCaretWidth = 0;
	sysCaretHeight = 0;

	keysAlwaysUnicode = false;

	caret.period = ::GetCaretBlinkTime();
	if (caret.period < 0)
		caret.period = 0;

	Initialise();
}

ScintillaWin::~ScintillaWin() {}

void ScintillaWin::Initialise() {
	// Initialize COM.  If the app has already done this it will have
	// no effect.  If the app hasnt, we really shouldnt ask them to call
	// it just so this internal feature works.
	hrOle = ::OleInitialize(NULL);

	// Find TrackMouseEvent which is available on Windows > 95
	HMODULE user32 = ::GetModuleHandle(TEXT("user32.dll"));
	TrackMouseEventFn = (TrackMouseEventSig)::GetProcAddress(user32, "TrackMouseEvent");
	if (TrackMouseEventFn == NULL) {
		// Windows 95 has an emulation in comctl32.dll:_TrackMouseEvent
		HMODULE commctrl32 = ::LoadLibrary(TEXT("comctl32.dll"));
		if (commctrl32 != NULL) {
			TrackMouseEventFn = (TrackMouseEventSig)
				::GetProcAddress(commctrl32, "_TrackMouseEvent");
		}
	}
}

void ScintillaWin::Finalise() {
	ScintillaBase::Finalise();
	SetTicking(false);
	SetIdle(false);
	::RevokeDragDrop(MainHWND());
	if (SUCCEEDED(hrOle)) {
		::OleUninitialize();
	}
}

HWND ScintillaWin::MainHWND() {
	return reinterpret_cast<HWND>(wMain.GetID());
}

bool ScintillaWin::DragThreshold(Point ptStart, Point ptNow) {
	int xMove = abs(ptStart.x - ptNow.x);
	int yMove = abs(ptStart.y - ptNow.y);
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
	HRESULT hr = ::DoDragDrop(
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
	SetDragPosition(SelectionPosition(invalidPosition));
}

// Avoid warnings everywhere for old style casts by concentrating them here
static WORD LoWord(DWORD l) {
	return LOWORD(l);
}

static WORD HiWord(DWORD l) {
	return HIWORD(l);
}

static int InputCodePage() {
	HKL inputLocale = ::GetKeyboardLayout(0);
	LANGID inputLang = LOWORD(inputLocale);
	char sCodePage[10];
	int res = ::GetLocaleInfoA(MAKELCID(inputLang, SORT_DEFAULT),
	  LOCALE_IDEFAULTANSICODEPAGE, sCodePage, sizeof(sCodePage));
	if (!res)
		return 0;
	return atoi(sCodePage);
}

#ifndef VK_OEM_2
static const int VK_OEM_2=0xbf;
static const int VK_OEM_3=0xc0;
static const int VK_OEM_4=0xdb;
static const int VK_OEM_5=0xdc;
static const int VK_OEM_6=0xdd;
#endif

/** Map the key codes to their equivalent SCK_ form. */
static int KeyTranslate(int keyIn) {
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

LRESULT ScintillaWin::WndPaint(uptr_t wParam) {
	//ElapsedTime et;

	// Redirect assertions to debug output and save current state
	bool assertsPopup = Platform::ShowAssertionPopUps(false);
	paintState = painting;
	PAINTSTRUCT ps;
	PAINTSTRUCT *pps;

	bool IsOcxCtrl = (wParam != 0); // if wParam != 0, it contains
								   // a PAINSTRUCT* from the OCX
	// Removed since this interferes with reporting other assertions as it occurs repeatedly
	//PLATFORM_ASSERT(hRgnUpdate == NULL);
	hRgnUpdate = ::CreateRectRgn(0, 0, 0, 0);
	if (IsOcxCtrl) {
		pps = reinterpret_cast<PAINTSTRUCT*>(wParam);
	} else {
		::GetUpdateRgn(MainHWND(), hRgnUpdate, FALSE);
		pps = &ps;
		::BeginPaint(MainHWND(), pps);
	}
	AutoSurface surfaceWindow(pps->hdc, this);
	if (surfaceWindow) {
		rcPaint = PRectangle(pps->rcPaint.left, pps->rcPaint.top, pps->rcPaint.right, pps->rcPaint.bottom);
		PRectangle rcClient = GetClientRectangle();
		paintingAllText = rcPaint.Contains(rcClient);
		if (paintingAllText) {
			//Platform::DebugPrintf("Performing full text paint\n");
		} else {
			//Platform::DebugPrintf("Performing partial paint %d .. %d\n", rcPaint.top, rcPaint.bottom);
		}
		Paint(surfaceWindow, rcPaint);
		surfaceWindow->Release();
	}
	if (hRgnUpdate) {
		::DeleteRgn(hRgnUpdate);
		hRgnUpdate = 0;
	}

	if (!IsOcxCtrl)
		::EndPaint(MainHWND(), pps);
	if (paintState == paintAbandoned) {
		// Painting area was insufficient to cover new styling or brace highlight positions
		FullPaint();
	}
	paintState = notPainting;

	// Restore debug output state
	Platform::ShowAssertionPopUps(assertsPopup);

	//Platform::DebugPrintf("Paint took %g\n", et.Duration());
	return 0l;
}

sptr_t ScintillaWin::HandleComposition(uptr_t wParam, sptr_t lParam) {
#ifdef __DMC__
	// Digital Mars compiler does not include Imm library
	return 0;
#else
	if (lParam & GCS_RESULTSTR) {
		HIMC hIMC = ::ImmGetContext(MainHWND());
		if (hIMC) {
			const int maxLenInputIME = 200;
			wchar_t wcs[maxLenInputIME];
			LONG bytes = ::ImmGetCompositionStringW(hIMC,
				GCS_RESULTSTR, wcs, (maxLenInputIME-1)*2);
			int wides = bytes / 2;
			if (IsUnicodeMode()) {
				char utfval[maxLenInputIME * 3];
				unsigned int len = UTF8Length(wcs, wides);
				UTF8FromUTF16(wcs, wides, utfval, len);
				utfval[len] = '\0';
				AddCharUTF(utfval, len);
			} else {
				char dbcsval[maxLenInputIME * 2];
				int size = ::WideCharToMultiByte(InputCodePage(),
					0, wcs, wides, dbcsval, sizeof(dbcsval) - 1, 0, 0);
				for (int i=0; i<size; i++) {
					AddChar(dbcsval[i]);
				}
			}
			// Set new position after converted
			Point pos = PointMainCaret();
			COMPOSITIONFORM CompForm;
			CompForm.dwStyle = CFS_POINT;
			CompForm.ptCurrentPos.x = pos.x;
			CompForm.ptCurrentPos.y = pos.y;
			::ImmSetCompositionWindow(hIMC, &CompForm);
			::ImmReleaseContext(MainHWND(), hIMC);
		}
		return 0;
	}
	return ::DefWindowProc(MainHWND(), WM_IME_COMPOSITION, wParam, lParam);
#endif
}

// Translate message IDs from WM_* and EM_* to SCI_* so can partly emulate Windows Edit control
static unsigned int SciMessageFromEM(unsigned int iMessage) {
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
	case WM_GETTEXT: return SCI_GETTEXT;
	case WM_SETTEXT: return SCI_SETTEXT;
	case WM_GETTEXTLENGTH: return SCI_GETTEXTLENGTH;
	case WM_PASTE: return SCI_PASTE;
	case WM_UNDO: return SCI_UNDO;
	}
	return iMessage;
}

static UINT CodePageFromCharSet(DWORD characterSet, UINT documentCodePage) {
	if (documentCodePage == SC_CP_UTF8) {
		// The system calls here are a little slow so avoid if known case.
		return SC_CP_UTF8;
	}
	CHARSETINFO ci = { 0, 0, { { 0, 0, 0, 0 }, { 0, 0 } } };
	BOOL bci = ::TranslateCharsetInfo((DWORD*)characterSet,
		&ci, TCI_SRCCHARSET);

	UINT cp;
	if (bci)
		cp = ci.ciACP;
	else
		cp = documentCodePage;

	CPINFO cpi;
	if (!IsValidCodePage(cp) && !GetCPInfo(cp, &cpi))
		cp = CP_ACP;

	return cp;
}

UINT ScintillaWin::CodePageOfDocument() {
	return CodePageFromCharSet(vs.styles[STYLE_DEFAULT].characterSet, pdoc->dbcsCodePage);
}

sptr_t ScintillaWin::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	try {
		//Platform::DebugPrintf("S M:%x WP:%x L:%x\n", iMessage, wParam, lParam);
		iMessage = SciMessageFromEM(iMessage);
		switch (iMessage) {

		case WM_CREATE:
			ctrlID = ::GetDlgCtrlID(reinterpret_cast<HWND>(wMain.GetID()));
			// Get Intellimouse scroll line parameters
			GetIntelliMouseParameters();
			::RegisterDragDrop(MainHWND(), reinterpret_cast<IDropTarget *>(&dt));
			break;

		case WM_COMMAND:
			Command(LoWord(wParam));
			break;

		case WM_PAINT:
			return WndPaint(wParam);

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
				//Platform::DebugPrintf("Scintilla WM_SIZE %d %d\n", LoWord(lParam), HiWord(lParam));
				ChangeSize();
			}
			break;

		case WM_MOUSEWHEEL:
			// Don't handle datazoom.
			// (A good idea for datazoom would be to "fold" or "unfold" details.
			// i.e. if datazoomed out only class structures are visible, when datazooming in the control
			// structures appear, then eventually the individual statements...)
			if (wParam & MK_SHIFT) {
				return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
			}

			// Either SCROLL or ZOOM. We handle the wheel steppings calculation
			wheelDelta -= static_cast<short>(HiWord(wParam));
			if (abs(wheelDelta) >= WHEEL_DELTA && linesPerScroll > 0) {
				int linesToScroll = linesPerScroll;
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
			if (wParam == standardTimerID && timer.ticking) {
				Tick();
			} else if (wParam == idleTimerID && idler.state) {
				SendMessage(MainHWND(), SC_WIN_IDLE, 0, 1);
			} else {
				return 1;
			}
			break;

		case SC_WIN_IDLE:
			// wParam=dwTickCountInitial, or 0 to initialize.  lParam=bSkipUserInputTest
			if (idler.state) {
				if (lParam || (WAIT_TIMEOUT == MsgWaitForMultipleObjects(0, 0, 0, 0, QS_INPUT|QS_HOTKEY))) {
					if (Idle()) {
						// User input was given priority above, but all events do get a turn.  Other
						// messages, notifications, etc. will get interleaved with the idle messages.

						// However, some things like WM_PAINT are a lower priority, and will not fire
						// when there's a message posted.  So, several times a second, we stop and let
						// the low priority events have a turn (after which the timer will fire again).

						DWORD dwCurrent = GetTickCount();
						DWORD dwStart = wParam ? wParam : dwCurrent;

						if (dwCurrent >= dwStart && dwCurrent > 200 && dwCurrent - 200 < dwStart)
							PostMessage(MainHWND(), SC_WIN_IDLE, dwStart, 0);
					} else {
						SetIdle(false);
					}
				}
			}
			break;

		case WM_GETMINMAXINFO:
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_LBUTTONDOWN: {
#ifndef __DMC__
			// Digital Mars compiler does not include Imm library
			// For IME, set the composition string as the result string.
			HIMC hIMC = ::ImmGetContext(MainHWND());
			::ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
			::ImmReleaseContext(MainHWND(), hIMC);
#endif
			//
			//Platform::DebugPrintf("Buttdown %d %x %x %x %x %x\n",iMessage, wParam, lParam,
			//	Platform::IsKeyDown(VK_SHIFT),
			//	Platform::IsKeyDown(VK_CONTROL),
			//	Platform::IsKeyDown(VK_MENU));
			::SetFocus(MainHWND());
			ButtonDown(Point::FromLong(lParam), ::GetMessageTime(),
				(wParam & MK_SHIFT) != 0,
				(wParam & MK_CONTROL) != 0,
				Platform::IsKeyDown(VK_MENU));
			}
			break;
			
		case WM_MBUTTONDOWN:
            ::SetFocus(MainHWND());
            break;
			
		case WM_MOUSEMOVE:
			SetTrackMouseLeaveEvent(true);
			ButtonMove(Point::FromLong(lParam));
			break;

		case WM_MOUSELEAVE:
			SetTrackMouseLeaveEvent(false);
			MouseLeave();
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_LBUTTONUP:
			ButtonUp(Point::FromLong(lParam),
				::GetMessageTime(),
				(wParam & MK_CONTROL) != 0);
			break;

		case WM_RBUTTONDOWN:
			::SetFocus(MainHWND());
			if (!PointInSelection(Point::FromLong(lParam))) {
				CancelModes();
				SetEmptySelection(PositionFromLocation(Point::FromLong(lParam)));
			}
			break;

		case WM_SETCURSOR:
			if (LoWord(lParam) == HTCLIENT) {
				if (inDragDrop == ddDragging) {
					DisplayCursor(Window::cursorUp);
				} else {
					// Display regular (drag) cursor over selection
					POINT pt;
					::GetCursorPos(&pt);
					::ScreenToClient(MainHWND(), &pt);
					if (PointInSelMargin(Point(pt.x, pt.y))) {
						DisplayCursor(GetMarginCursor(Point(pt.x, pt.y)));
					} else if (PointInSelection(Point(pt.x, pt.y)) && !SelectionEmpty()) {
						DisplayCursor(Window::cursorArrow);
					} else if (PointIsHotspot(Point(pt.x, pt.y))) {
						DisplayCursor(Window::cursorHand);
					} else {
						DisplayCursor(Window::cursorText);
					}
				}
				return TRUE;
			} else {
				return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
			}

		case WM_CHAR:
			if (((wParam >= 128) || !iscntrl(wParam)) || !lastKeyDownConsumed) {
				if (::IsWindowUnicode(MainHWND()) || keysAlwaysUnicode) {
					wchar_t wcs[2] = {wParam, 0};
					if (IsUnicodeMode()) {
						// For a wide character version of the window:
						char utfval[4];
						unsigned int len = UTF8Length(wcs, 1);
						UTF8FromUTF16(wcs, 1, utfval, len);
						AddCharUTF(utfval, len);
					} else {
						UINT cpDest = CodePageOfDocument();
						char inBufferCP[20];
						int size = ::WideCharToMultiByte(cpDest,
							0, wcs, 1, inBufferCP, sizeof(inBufferCP) - 1, 0, 0);
						inBufferCP[size] = '\0';
						AddCharUTF(inBufferCP, size);
					}
				} else {
					if (IsUnicodeMode()) {
						AddCharBytes('\0', LOBYTE(wParam));
					} else {
						AddChar(LOBYTE(wParam));
					}
				}
			}
			return 0;

		case WM_UNICHAR:
			if (wParam == UNICODE_NOCHAR) {
				return IsUnicodeMode() ? 1 : 0;
			} else if (lastKeyDownConsumed) {
				return 1;
			} else {
				if (IsUnicodeMode()) {
					char utfval[4];
					wchar_t wcs[2] = {static_cast<wchar_t>(wParam), 0};
					unsigned int len = UTF8Length(wcs, 1);
					UTF8FromUTF16(wcs, 1, utfval, len);
					AddCharUTF(utfval, len);
					return 1;
				} else {
					return 0;
				}
			}

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
			//Platform::DebugPrintf("S keydown %d %x %x %x %x\n",iMessage, wParam, lParam, ::IsKeyDown(VK_SHIFT), ::IsKeyDown(VK_CONTROL));
				lastKeyDownConsumed = false;
				int ret = KeyDown(KeyTranslate(wParam),
					Platform::IsKeyDown(VK_SHIFT),
					Platform::IsKeyDown(VK_CONTROL),
					Platform::IsKeyDown(VK_MENU),
					&lastKeyDownConsumed);
				if (!ret && !lastKeyDownConsumed) {
					return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
				}
				break;
			}

		case WM_IME_KEYDOWN:
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

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
				HWND wCT = reinterpret_cast<HWND>(ct.wCallTip.GetID());
				if (!wParam ||
					!(::IsChild(wThis, wOther) || (wOther == wCT))) {
					SetFocusState(false);
					DestroySystemCaret();
				}
			}
			//RealizeWindowPalette(true);
			break;

		case WM_SETFOCUS:
			SetFocusState(true);
			RealizeWindowPalette(false);
			DestroySystemCaret();
			CreateSystemCaret();
			break;

		case WM_SYSCOLORCHANGE:
			//Platform::DebugPrintf("Setting Changed\n");
			InvalidateStyleData();
			break;

		case WM_PALETTECHANGED:
			if (wParam != reinterpret_cast<uptr_t>(MainHWND())) {
				//Platform::DebugPrintf("** Palette Changed\n");
				RealizeWindowPalette(true);
			}
			break;

		case WM_QUERYNEWPALETTE:
			//Platform::DebugPrintf("** Query palette\n");
			RealizeWindowPalette(false);
			break;

		case WM_IME_STARTCOMPOSITION: 	// dbcs
			ImeStartComposition();
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_IME_ENDCOMPOSITION: 	// dbcs
			ImeEndComposition();
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_IME_COMPOSITION:
			return HandleComposition(wParam, lParam);

		case WM_IME_CHAR: {
				AddCharBytes(HIBYTE(wParam), LOBYTE(wParam));
				return 0;
			}

		case WM_CONTEXTMENU:
			if (displayPopupMenu) {
				Point pt = Point::FromLong(lParam);
				if ((pt.x == -1) && (pt.y == -1)) {
					// Caused by keyboard so display menu near caret
					pt = PointMainCaret();
					POINT spt = {pt.x, pt.y};
					::ClientToScreen(MainHWND(), &spt);
					pt = Point(spt.x, spt.y);
				}
				ContextMenu(pt);
				return 0;
			}
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_INPUTLANGCHANGE:
			//::SetThreadLocale(LOWORD(lParam));
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_INPUTLANGCHANGEREQUEST:
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case WM_ERASEBKGND:
			return 1;   // Avoid any background erasure as whole window painted.

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
		case WM_IME_SETCONTEXT:
		case WM_IME_NOTIFY:
		case WM_SYSCOMMAND:
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case EM_LINEFROMCHAR:
			if (static_cast<int>(wParam) < 0) {
				wParam = SelectionStart().Position();
			}
			return pdoc->LineFromPosition(wParam);

		case EM_EXLINEFROMCHAR:
			return pdoc->LineFromPosition(lParam);

		case EM_GETSEL:
			if (wParam) {
				*reinterpret_cast<int *>(wParam) = SelectionStart().Position();
			}
			if (lParam) {
				*reinterpret_cast<int *>(lParam) = SelectionEnd().Position();
			}
			return MAKELONG(SelectionStart().Position(), SelectionEnd().Position());

		case EM_EXGETSEL: {
				if (lParam == 0) {
					return 0;
				}
				Sci_CharacterRange *pCR = reinterpret_cast<Sci_CharacterRange *>(lParam);
				pCR->cpMin = SelectionStart().Position();
				pCR->cpMax = SelectionEnd().Position();
			}
			break;

		case EM_SETSEL: {
				int nStart = static_cast<int>(wParam);
				int nEnd = static_cast<int>(lParam);
				if (nStart == 0 && nEnd == -1) {
					nEnd = pdoc->Length();
				}
				if (nStart == -1) {
					nStart = nEnd;	// Remove selection
				}
				if (nStart > nEnd) {
					SetSelection(nEnd, nStart);
				} else {
					SetSelection(nStart, nEnd);
				}
				EnsureCaretVisible();
			}
			break;

		case EM_EXSETSEL: {
				if (lParam == 0) {
					return 0;
				}
				Sci_CharacterRange *pCR = reinterpret_cast<Sci_CharacterRange *>(lParam);
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

		case SCI_SETKEYSUNICODE:
			keysAlwaysUnicode = wParam != 0;
			break;

		case SCI_GETKEYSUNICODE:
			return keysAlwaysUnicode;

#ifdef SCI_LEXER
		case SCI_LOADLEXERLIBRARY:
			LexerManager::GetInstance()->Load(reinterpret_cast<const char *>(lParam));
			break;
#endif

		default:
			return ScintillaBase::WndProc(iMessage, wParam, lParam);
		}
	} catch (std::bad_alloc &) {
		errorStatus = SC_STATUS_BADALLOC;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return 0l;
}

bool ScintillaWin::ValidCodePage(int codePage) const {
	return codePage == 0 || codePage == SC_CP_UTF8 ||
	       codePage == 932 || codePage == 936 || codePage == 949 ||
	       codePage == 950 || codePage == 1361;
}

sptr_t ScintillaWin::DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
}

void ScintillaWin::SetTicking(bool on) {
	if (timer.ticking != on) {
		timer.ticking = on;
		if (timer.ticking) {
			timer.tickerID = ::SetTimer(MainHWND(), standardTimerID, timer.tickSize, NULL)
				? reinterpret_cast<TickerID>(standardTimerID) : 0;
		} else {
			::KillTimer(MainHWND(), reinterpret_cast<uptr_t>(timer.tickerID));
			timer.tickerID = 0;
		}
	}
	timer.ticksToWait = caret.period;
}

bool ScintillaWin::SetIdle(bool on) {
	// On Win32 the Idler is implemented as a Timer on the Scintilla window.  This
	// takes advantage of the fact that WM_TIMER messages are very low priority,
	// and are only posted when the message queue is empty, i.e. during idle time.
	if (idler.state != on) {
		if (on) {
			idler.idlerID = ::SetTimer(MainHWND(), idleTimerID, 10, NULL)
				? reinterpret_cast<IdlerID>(idleTimerID) : 0;
		} else {
			::KillTimer(MainHWND(), reinterpret_cast<uptr_t>(idler.idlerID));
			idler.idlerID = 0;
		}
		idler.state = idler.idlerID != 0;
	}
	return idler.state;
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

void ScintillaWin::SetTrackMouseLeaveEvent(bool on) {
	if (on && TrackMouseEventFn && !trackedMouseLeave) {
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = MainHWND();
		TrackMouseEventFn(&tme);
	}
	trackedMouseLeave = on;
}

bool ScintillaWin::PaintContains(PRectangle rc) {
	bool contains = true;
	if ((paintState == painting) && (!rc.Empty())) {
		if (!rcPaint.Contains(rc)) {
			contains = false;
		} else {
			// In bounding rectangle so check more accurately using region
			HRGN hRgnRange = ::CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
			if (hRgnRange) {
				HRGN hRgnDest = ::CreateRectRgn(0, 0, 0, 0);
				if (hRgnDest) {
					int combination = ::CombineRgn(hRgnDest, hRgnRange, hRgnUpdate, RGN_DIFF);
					if (combination != NULLREGION) {
						contains = false;
					}
					::DeleteRgn(hRgnDest);
				}
				::DeleteRgn(hRgnRange);
			}
		}
	}
	return contains;
}

void ScintillaWin::ScrollText(int linesToMove) {
	//Platform::DebugPrintf("ScintillaWin::ScrollText %d\n", linesToMove);
	::ScrollWindow(MainHWND(), 0,
		vs.lineHeight * linesToMove, 0, 0);
	::UpdateWindow(MainHWND());
}

void ScintillaWin::UpdateSystemCaret() {
	if (hasFocus) {
		if (HasCaretSizeChanged()) {
			DestroySystemCaret();
			CreateSystemCaret();
		}
		Point pos = PointMainCaret();
		::SetCaretPos(pos.x, pos.y);
	}
}

int ScintillaWin::SetScrollInfo(int nBar, LPCSCROLLINFO lpsi, BOOL bRedraw) {
	return ::SetScrollInfo(MainHWND(), nBar, lpsi, bRedraw);
}

bool ScintillaWin::GetScrollInfo(int nBar, LPSCROLLINFO lpsi) {
	return ::GetScrollInfo(MainHWND(), nBar, lpsi) ? true : false;
}

// Change the scroll position but avoid repaint if changing to same value
void ScintillaWin::ChangeScrollPos(int barType, int pos) {
	SCROLLINFO sci = {
		sizeof(sci), 0, 0, 0, 0, 0, 0
	};
	sci.fMask = SIF_POS;
	GetScrollInfo(barType, &sci);
	if (sci.nPos != pos) {
		DwellEnd(true);
		sci.nPos = pos;
		SetScrollInfo(barType, &sci, TRUE);
	}
}

void ScintillaWin::SetVerticalScrollPos() {
	ChangeScrollPos(SB_VERT, topLine);
}

void ScintillaWin::SetHorizontalScrollPos() {
	ChangeScrollPos(SB_HORZ, xOffset);
}

bool ScintillaWin::ModifyScrollBars(int nMax, int nPage) {
	bool modified = false;
	SCROLLINFO sci = {
		sizeof(sci), 0, 0, 0, 0, 0, 0
	};
	sci.fMask = SIF_PAGE | SIF_RANGE;
	GetScrollInfo(SB_VERT, &sci);
	int vertEndPreferred = nMax;
	if (!verticalScrollBarVisible)
		vertEndPreferred = 0;
	if ((sci.nMin != 0) ||
		(sci.nMax != vertEndPreferred) ||
	        (sci.nPage != static_cast<unsigned int>(nPage)) ||
	        (sci.nPos != 0)) {
		//Platform::DebugPrintf("Scroll info changed %d %d %d %d %d\n",
		//	sci.nMin, sci.nMax, sci.nPage, sci.nPos, sci.nTrackPos);
		sci.fMask = SIF_PAGE | SIF_RANGE;
		sci.nMin = 0;
		sci.nMax = vertEndPreferred;
		sci.nPage = nPage;
		sci.nPos = 0;
		sci.nTrackPos = 1;
		SetScrollInfo(SB_VERT, &sci, TRUE);
		modified = true;
	}

	PRectangle rcText = GetTextRectangle();
	int horizEndPreferred = scrollWidth;
	if (horizEndPreferred < 0)
		horizEndPreferred = 0;
	if (!horizontalScrollBarVisible || (wrapState != eWrapNone))
		horizEndPreferred = 0;
	unsigned int pageWidth = rcText.Width();
	sci.fMask = SIF_PAGE | SIF_RANGE;
	GetScrollInfo(SB_HORZ, &sci);
	if ((sci.nMin != 0) ||
		(sci.nMax != horizEndPreferred) ||
		(sci.nPage != pageWidth) ||
	        (sci.nPos != 0)) {
		sci.fMask = SIF_PAGE | SIF_RANGE;
		sci.nMin = 0;
		sci.nMax = horizEndPreferred;
		sci.nPage = pageWidth;
		sci.nPos = 0;
		sci.nTrackPos = 1;
		SetScrollInfo(SB_HORZ, &sci, TRUE);
		modified = true;
		if (scrollWidth < static_cast<int>(pageWidth)) {
			HorizontalScrollTo(0);
		}
	}
	return modified;
}

void ScintillaWin::NotifyChange() {
	::SendMessage(::GetParent(MainHWND()), WM_COMMAND,
	        MAKELONG(GetCtrlID(), SCEN_CHANGE),
		reinterpret_cast<LPARAM>(MainHWND()));
}

void ScintillaWin::NotifyFocus(bool focus) {
	::SendMessage(::GetParent(MainHWND()), WM_COMMAND,
	        MAKELONG(GetCtrlID(), focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS),
		reinterpret_cast<LPARAM>(MainHWND()));
}

void ScintillaWin::SetCtrlID(int identifier) {
	::SetWindowID(reinterpret_cast<HWND>(wMain.GetID()), identifier);
}

int ScintillaWin::GetCtrlID() {
	return ::GetDlgCtrlID(reinterpret_cast<HWND>(wMain.GetID()));
}

void ScintillaWin::NotifyParent(SCNotification scn) {
	scn.nmhdr.hwndFrom = MainHWND();
	scn.nmhdr.idFrom = GetCtrlID();
	::SendMessage(::GetParent(MainHWND()), WM_NOTIFY,
	              GetCtrlID(), reinterpret_cast<LPARAM>(&scn));
}

void ScintillaWin::NotifyDoubleClick(Point pt, bool shift, bool ctrl, bool alt) {
	//Platform::DebugPrintf("ScintillaWin Double click 0\n");
	ScintillaBase::NotifyDoubleClick(pt, shift, ctrl, alt);
	// Send myself a WM_LBUTTONDBLCLK, so the container can handle it too.
	::SendMessage(MainHWND(),
			  WM_LBUTTONDBLCLK,
			  shift ? MK_SHIFT : 0,
			  MAKELPARAM(pt.x, pt.y));
}

class CaseFolderUTF8 : public CaseFolderTable {
	// Allocate the expandable storage here so that it does not need to be reallocated
	// for each call to Fold.
	std::vector<wchar_t> utf16Mixed;
	std::vector<wchar_t> utf16Folded;
public:
	CaseFolderUTF8() {
		StandardASCII();
	}
	virtual size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else {
			if (lenMixed > utf16Mixed.size()) {
				utf16Mixed.resize(lenMixed + 8);
			}
			size_t nUtf16Mixed = ::MultiByteToWideChar(65001, 0, mixed, lenMixed,
				&utf16Mixed[0], utf16Mixed.size());

			if (nUtf16Mixed == 0) {
				// Failed to convert -> bad UTF-8
				folded[0] = '\0';
				return 1;
			}

			if (nUtf16Mixed * 4 > utf16Folded.size()) {	// Maximum folding expansion factor of 4
				utf16Folded.resize(nUtf16Mixed * 4 + 8);
			}
			int lenFlat = ::LCMapStringW(LOCALE_SYSTEM_DEFAULT,
				LCMAP_LINGUISTIC_CASING | LCMAP_LOWERCASE,
				&utf16Mixed[0], nUtf16Mixed, &utf16Folded[0], utf16Folded.size());

			size_t lenOut = UTF8Length(&utf16Folded[0], lenFlat);
			if (lenOut < sizeFolded) {
				UTF8FromUTF16(&utf16Folded[0], lenFlat, folded, lenOut);
				return lenOut;
			} else {
				return 0;
			}
		}
	}
};

class CaseFolderDBCS : public CaseFolderTable {
	// Allocate the expandable storage here so that it does not need to be reallocated
	// for each call to Fold.
	std::vector<wchar_t> utf16Mixed;
	std::vector<wchar_t> utf16Folded;
	UINT cp;
public:
	CaseFolderDBCS(UINT cp_) : cp(cp_) {
		StandardASCII();
	}
	virtual size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else {
			if (lenMixed > utf16Mixed.size()) {
				utf16Mixed.resize(lenMixed + 8);
			}
			size_t nUtf16Mixed = ::MultiByteToWideChar(cp, 0, mixed, lenMixed,
				&utf16Mixed[0], utf16Mixed.size());

			if (nUtf16Mixed == 0) {
				// Failed to convert -> bad input
				folded[0] = '\0';
				return 1;
			}

			if (nUtf16Mixed * 4 > utf16Folded.size()) {	// Maximum folding expansion factor of 4
				utf16Folded.resize(nUtf16Mixed * 4 + 8);
			}
			int lenFlat = ::LCMapStringW(LOCALE_SYSTEM_DEFAULT,
				LCMAP_LINGUISTIC_CASING | LCMAP_LOWERCASE,
				&utf16Mixed[0], nUtf16Mixed, &utf16Folded[0], utf16Folded.size());

			size_t lenOut = ::WideCharToMultiByte(cp, 0,
				&utf16Folded[0], lenFlat,
				NULL, 0, NULL, 0);

			if (lenOut < sizeFolded) {
				::WideCharToMultiByte(cp, 0,
					&utf16Folded[0], lenFlat,
					folded, lenOut, NULL, 0);
				return lenOut;
			} else {
				return 0;
			}
		}
	}
};

CaseFolder *ScintillaWin::CaseFolderForEncoding() {
	UINT cpDest = CodePageOfDocument();
	if (cpDest == SC_CP_UTF8) {
		return new CaseFolderUTF8();
	} else {
		if (pdoc->dbcsCodePage == 0) {
			CaseFolderTable *pcf = new CaseFolderTable();
			pcf->StandardASCII();
			// Only for single byte encodings
			UINT cpDoc = CodePageOfDocument();
			for (int i=0x80; i<0x100; i++) {
				char sCharacter[2] = "A";
				sCharacter[0] = static_cast<char>(i);
				wchar_t wCharacter[20];
				unsigned int lengthUTF16 = ::MultiByteToWideChar(cpDoc, 0, sCharacter, 1,
					wCharacter, sizeof(wCharacter)/sizeof(wCharacter[0]));
				if (lengthUTF16 == 1) {
					wchar_t wLower[20];
					int charsConverted = ::LCMapStringW(LOCALE_SYSTEM_DEFAULT,
						LCMAP_LINGUISTIC_CASING | LCMAP_LOWERCASE,
						wCharacter, lengthUTF16, wLower, sizeof(wLower)/sizeof(wLower[0]));
					char sCharacterLowered[20];
					unsigned int lengthConverted = ::WideCharToMultiByte(cpDoc, 0,
						wLower, charsConverted,
						sCharacterLowered, sizeof(sCharacterLowered), NULL, 0);
					if ((lengthConverted == 1) && (sCharacter[0] != sCharacterLowered[0])) {
						pcf->SetTranslation(sCharacter[0], sCharacterLowered[0]);
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
	if (s.size() == 0)
		return std::string();

	if (caseMapping == cmSame)
		return s;

	UINT cpDoc = CodePageOfDocument();

	unsigned int lengthUTF16 = ::MultiByteToWideChar(cpDoc, 0, s.c_str(), s.size(), NULL, 0);
	if (lengthUTF16 == 0)	// Failed to convert
		return s;

	DWORD mapFlags = LCMAP_LINGUISTIC_CASING |
		((caseMapping == cmUpper) ? LCMAP_UPPERCASE : LCMAP_LOWERCASE);

	// Many conversions performed by search function are short so optimize this case.
	enum { shortSize=20 };

	if (s.size() > shortSize) {
		// Use dynamic allocations for long strings

		// Change text to UTF-16
		std::vector<wchar_t> vwcText(lengthUTF16);
		::MultiByteToWideChar(cpDoc, 0, s.c_str(), s.size(), &vwcText[0], lengthUTF16);

		// Change case
		int charsConverted = ::LCMapStringW(LOCALE_SYSTEM_DEFAULT, mapFlags,
			&vwcText[0], lengthUTF16, NULL, 0);
		std::vector<wchar_t> vwcConverted(charsConverted);
		::LCMapStringW(LOCALE_SYSTEM_DEFAULT, mapFlags,
			&vwcText[0], lengthUTF16, &vwcConverted[0], charsConverted);

		// Change back to document encoding
		unsigned int lengthConverted = ::WideCharToMultiByte(cpDoc, 0,
			&vwcConverted[0], vwcConverted.size(),
			NULL, 0, NULL, 0);
		std::vector<char> vcConverted(lengthConverted);
		::WideCharToMultiByte(cpDoc, 0,
			&vwcConverted[0], vwcConverted.size(),
			&vcConverted[0], vcConverted.size(), NULL, 0);

		return std::string(&vcConverted[0], vcConverted.size());

	} else {
		// Use static allocations for short strings as much faster
		// A factor of 15 for single character strings

		// Change text to UTF-16
		wchar_t vwcText[shortSize];
		::MultiByteToWideChar(cpDoc, 0, s.c_str(), s.size(), vwcText, lengthUTF16);

		// Change case
		int charsConverted = ::LCMapStringW(LOCALE_SYSTEM_DEFAULT, mapFlags,
			vwcText, lengthUTF16, NULL, 0);
		// Full mapping may produce up to 3 characters per input character
		wchar_t vwcConverted[shortSize*3];
		::LCMapStringW(LOCALE_SYSTEM_DEFAULT, mapFlags, vwcText, lengthUTF16,
			vwcConverted, charsConverted);

		// Change back to document encoding
		unsigned int lengthConverted = ::WideCharToMultiByte(cpDoc, 0,
			vwcConverted, charsConverted,
			NULL, 0, NULL, 0);
		// Each UTF-16 code unit may need up to 3 bytes in UTF-8
		char vcConverted[shortSize * 3 * 3];
		::WideCharToMultiByte(cpDoc, 0,
			vwcConverted, charsConverted,
			vcConverted, lengthConverted, NULL, 0);

		return std::string(vcConverted, lengthConverted);
	}
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

class GlobalMemory {
	HGLOBAL hand;
public:
	void *ptr;
	GlobalMemory() : hand(0), ptr(0) {
	}
	GlobalMemory(HGLOBAL hand_) : hand(hand_), ptr(0) {
		if (hand) {
			ptr = ::GlobalLock(hand);
		}
	}
	~GlobalMemory() {
		PLATFORM_ASSERT(!ptr);
	}
	void Allocate(size_t bytes) {
		hand = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes);
		if (hand) {
			ptr = ::GlobalLock(hand);
		}
	}
	HGLOBAL Unlock() {
		PLATFORM_ASSERT(ptr);
		HGLOBAL handCopy = hand;
		::GlobalUnlock(hand);
		ptr = 0;
		hand = 0;
		return handCopy;
	}
	void SetClip(UINT uFormat) {
		::SetClipboardData(uFormat, Unlock());
	}
	operator bool() const {
		return ptr != 0;
	}
	SIZE_T Size() {
		return ::GlobalSize(hand);
	}
};

void ScintillaWin::InsertPasteText(const char *text, int len, SelectionPosition selStart, bool isRectangular, bool isLine) {
	if (isRectangular) {
		PasteRectangular(selStart, text, len);
	} else {
		char *convertedText = 0;
		if (convertPastes) {
			// Convert line endings of the paste into our local line-endings mode
			convertedText = Document::TransformLineEnds(&len, text, len, pdoc->eolMode);
			text = convertedText;
		}
		if (isLine) {
			int insertPos = pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret()));
			pdoc->InsertString(insertPos, text, len);
			// add the newline if necessary
			if ((len > 0) && (text[len-1] != '\n' && text[len-1] != '\r')) {
				const char *endline = StringFromEOLMode(pdoc->eolMode);
				pdoc->InsertString(insertPos + len, endline, strlen(endline));
				len += strlen(endline);
			}
			if (sel.MainCaret() == insertPos) {
				SetEmptySelection(sel.MainCaret() + len);
			}
		} else {
			InsertPaste(selStart, text, len);
		}
		delete []convertedText;
	}
}

void ScintillaWin::Paste() {
	if (!::OpenClipboard(MainHWND()))
		return;
	UndoGroup ug(pdoc);
	bool isLine = SelectionEmpty() && (::IsClipboardFormatAvailable(cfLineSelect) != 0);
	ClearSelection(multiPasteMode == SC_MULTIPASTE_EACH);
	SelectionPosition selStart = sel.IsRectangular() ?
		sel.Rectangular().Start() :
		sel.Range(sel.Main()).Start();
	bool isRectangular = ::IsClipboardFormatAvailable(cfColumnSelect) != 0;

	// Always use CF_UNICODETEXT if available
	GlobalMemory memUSelection(::GetClipboardData(CF_UNICODETEXT));
	if (memUSelection) {
		wchar_t *uptr = static_cast<wchar_t *>(memUSelection.ptr);
		if (uptr) {
			unsigned int len;
			char *putf;
			// Default Scintilla behaviour in Unicode mode
			if (IsUnicodeMode()) {
				unsigned int bytes = memUSelection.Size();
				len = UTF8Length(uptr, bytes / 2);
				putf = new char[len + 1];
				UTF8FromUTF16(uptr, bytes / 2, putf, len);
			} else {
				// CF_UNICODETEXT available, but not in Unicode mode
				// Convert from Unicode to current Scintilla code page
				UINT cpDest = CodePageOfDocument();
				len = ::WideCharToMultiByte(cpDest, 0, uptr, -1,
				                            NULL, 0, NULL, NULL) - 1; // subtract 0 terminator
				putf = new char[len + 1];
				::WideCharToMultiByte(cpDest, 0, uptr, -1,
					                      putf, len + 1, NULL, NULL);
			}

			InsertPasteText(putf, len, selStart, isRectangular, isLine);
			delete []putf;
		}
		memUSelection.Unlock();
	} else {
		// CF_UNICODETEXT not available, paste ANSI text
		GlobalMemory memSelection(::GetClipboardData(CF_TEXT));
		if (memSelection) {
			char *ptr = static_cast<char *>(memSelection.ptr);
			if (ptr) {
				unsigned int bytes = memSelection.Size();
				unsigned int len = bytes;
				for (unsigned int i = 0; i < bytes; i++) {
					if ((len == bytes) && (0 == ptr[i]))
						len = i;
				}

				// In Unicode mode, convert clipboard text to UTF-8
				if (IsUnicodeMode()) {
					wchar_t *uptr = new wchar_t[len+1];

					unsigned int ulen = ::MultiByteToWideChar(CP_ACP, 0,
					                    ptr, len, uptr, len+1);

					unsigned int mlen = UTF8Length(uptr, ulen);
					char *putf = new char[mlen + 1];
					if (putf) {
						// CP_UTF8 not available on Windows 95, so use UTF8FromUTF16()
						UTF8FromUTF16(uptr, ulen, putf, mlen);
					}

					delete []uptr;

					if (putf) {
						InsertPasteText(putf, mlen, selStart, isRectangular, isLine);
						delete []putf;
					}
				} else {
					InsertPasteText(ptr, len, selStart, isRectangular, isLine);
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
		ct.wCallTip = ::CreateWindow(callClassName, TEXT("ACallTip"),
					     WS_POPUP, 100, 100, 150, 20,
					     MainHWND(), 0,
					     GetWindowInstance(MainHWND()),
					     this);
		ct.wDraw = ct.wCallTip;
	}
}

void ScintillaWin::AddToPopUp(const char *label, int cmd, bool enabled) {
	HMENU hmenuPopup = reinterpret_cast<HMENU>(popup.GetID());
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
	*ppv = NULL;
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
	//Platform::DebugPrintf("EFE Next %d %d", fe->pos, celt);
	if (rgelt == NULL) return E_POINTER;
	// We only support one format, so this is simple.
	unsigned int putPos = 0;
	while ((fe->pos < fe->formatsLen) && (putPos < celt)) {
		rgelt->cfFormat = fe->formats[fe->pos];
		rgelt->ptd = 0;
		rgelt->dwAspect = DVASPECT_CONTENT;
		rgelt->lindex = -1;
		rgelt->tymed = TYMED_HGLOBAL;
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
		pfe = new FormatEnumerator(fe->pos, fe->formats, fe->formatsLen);
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

FormatEnumerator::FormatEnumerator(int pos_, CLIPFORMAT formats_[], int formatsLen_) {
	vtbl = vtFormatEnumerator;
	ref = 0;   // First QI adds first reference...
	pos = pos_;
	formatsLen = formatsLen_;
	for (int i=0; i<formatsLen; i++)
		formats[i] = formats_[i];
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

DropSource::DropSource() {
	vtbl = vtDropSource;
	sci = 0;
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
	    pFE->ptd == 0 &&
	    (pFE->dwAspect & DVASPECT_CONTENT) != 0 &&
	    pFE->lindex == -1 &&
	    (pFE->tymed & TYMED_HGLOBAL) != 0
	) {
		return S_OK;
	}

	bool formatOK = (pFE->cfFormat == CF_TEXT) ||
		((pFE->cfFormat == CF_UNICODETEXT) && pd->sci->IsUnicodeMode());
	if (!formatOK ||
	    pFE->ptd != 0 ||
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
	pFEOut->ptd = 0;
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
			*ppEnum = 0;
			return E_FAIL;
		}
		FormatEnumerator *pfe;
		if (pd->sci->IsUnicodeMode()) {
			CLIPFORMAT formats[] = {CF_UNICODETEXT, CF_TEXT};
			pfe = new FormatEnumerator(0, formats, 2);
		} else {
			CLIPFORMAT formats[] = {CF_TEXT};
			pfe = new FormatEnumerator(0, formats, 1);
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

DataObject::DataObject() {
	vtbl = vtDataObject;
	sci = 0;
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

DropTarget::DropTarget() {
	vtbl = vtDropTarget;
	sci = 0;
}

/**
 * DBCS: support Input Method Editor (IME).
 * Called when IME Window opened.
 */
void ScintillaWin::ImeStartComposition() {
#ifndef __DMC__
	// Digital Mars compiler does not include Imm library
	if (caret.active) {
		// Move IME Window to current caret position
		HIMC hIMC = ::ImmGetContext(MainHWND());
		Point pos = PointMainCaret();
		COMPOSITIONFORM CompForm;
		CompForm.dwStyle = CFS_POINT;
		CompForm.ptCurrentPos.x = pos.x;
		CompForm.ptCurrentPos.y = pos.y;

		::ImmSetCompositionWindow(hIMC, &CompForm);

		// Set font of IME window to same as surrounded text.
		if (stylesValid) {
			// Since the style creation code has been made platform independent,
			// The logfont for the IME is recreated here.
			int styleHere = (pdoc->StyleAt(sel.MainCaret())) & 31;
			LOGFONTA lf = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""};
			int sizeZoomed = vs.styles[styleHere].size + vs.zoomLevel;
			if (sizeZoomed <= 2)	// Hangs if sizeZoomed <= 1
				sizeZoomed = 2;
			AutoSurface surface(this);
			int deviceHeight = sizeZoomed;
			if (surface) {
				deviceHeight = (sizeZoomed * surface->LogPixelsY()) / 72;
			}
			// The negative is to allow for leading
			lf.lfHeight = -(abs(deviceHeight));
			lf.lfWeight = vs.styles[styleHere].bold ? FW_BOLD : FW_NORMAL;
			lf.lfItalic = static_cast<BYTE>(vs.styles[styleHere].italic ? 1 : 0);
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfFaceName[0] = '\0';
			if (vs.styles[styleHere].fontName)
				strcpy(lf.lfFaceName, vs.styles[styleHere].fontName);

			::ImmSetCompositionFontA(hIMC, &lf);
		}
		::ImmReleaseContext(MainHWND(), hIMC);
		// Caret is displayed in IME window. So, caret in Scintilla is useless.
		DropCaret();
	}
#endif
}

/** Called when IME Window closed. */
void ScintillaWin::ImeEndComposition() {
	ShowCaretAtCurrentPosition();
}

void ScintillaWin::AddCharBytes(char b0, char b1) {

	int inputCodePage = InputCodePage();
	if (inputCodePage && IsUnicodeMode()) {
		char utfval[4] = "\0\0\0";
		char ansiChars[3];
		wchar_t wcs[2];
		if (b0) {	// Two bytes from IME
			ansiChars[0] = b0;
			ansiChars[1] = b1;
			ansiChars[2] = '\0';
			::MultiByteToWideChar(inputCodePage, 0, ansiChars, 2, wcs, 1);
		} else {
			ansiChars[0] = b1;
			ansiChars[1] = '\0';
			::MultiByteToWideChar(inputCodePage, 0, ansiChars, 1, wcs, 1);
		}
		unsigned int len = UTF8Length(wcs, 1);
		UTF8FromUTF16(wcs, 1, utfval, len);
		utfval[len] = '\0';
		AddCharUTF(utfval, len ? len : 1);
	} else if (b0) {
		char dbcsChars[3];
		dbcsChars[0] = b0;
		dbcsChars[1] = b1;
		dbcsChars[2] = '\0';
		AddCharUTF(dbcsChars, 2, true);
	} else {
		AddChar(b1);
	}
}

void ScintillaWin::GetIntelliMouseParameters() {
	// This retrieves the number of lines per scroll as configured inthe Mouse Properties sheet in Control Panel
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &linesPerScroll, 0);
}

void ScintillaWin::CopyToClipboard(const SelectionText &selectedText) {
	if (!::OpenClipboard(MainHWND()))
		return;
	::EmptyClipboard();

	GlobalMemory uniText;

	// Default Scintilla behaviour in Unicode mode
	if (IsUnicodeMode()) {
		int uchars = UTF16Length(selectedText.s, selectedText.len);
		uniText.Allocate(2 * uchars);
		if (uniText) {
			UTF16FromUTF8(selectedText.s, selectedText.len, static_cast<wchar_t *>(uniText.ptr), uchars);
		}
	} else {
		// Not Unicode mode
		// Convert to Unicode using the current Scintilla code page
		UINT cpSrc = CodePageFromCharSet(
					selectedText.characterSet, selectedText.codePage);
		int uLen = ::MultiByteToWideChar(cpSrc, 0, selectedText.s, selectedText.len, 0, 0);
		uniText.Allocate(2 * uLen);
		if (uniText) {
			::MultiByteToWideChar(cpSrc, 0, selectedText.s, selectedText.len,
				static_cast<wchar_t *>(uniText.ptr), uLen);
		}
	}

	if (uniText) {
		if (!IsNT()) {
			// Copy ANSI text to clipboard on Windows 9x
			// Convert from Unicode text, so other ANSI programs can
			// paste the text
			// Windows NT, 2k, XP automatically generates CF_TEXT
			GlobalMemory ansiText;
			ansiText.Allocate(selectedText.len);
			if (ansiText) {
				::WideCharToMultiByte(CP_ACP, 0, static_cast<wchar_t *>(uniText.ptr), -1,
					static_cast<char *>(ansiText.ptr), selectedText.len, NULL, NULL);
				ansiText.SetClip(CF_TEXT);
			}
		}
		uniText.SetClip(CF_UNICODETEXT);
	} else {
		// There was a failure - try to copy at least ANSI text
		GlobalMemory ansiText;
		ansiText.Allocate(selectedText.len);
		if (ansiText) {
			memcpy(static_cast<char *>(ansiText.ptr), selectedText.s, selectedText.len);
			ansiText.SetClip(CF_TEXT);
		}
	}

	if (selectedText.rectangular) {
		::SetClipboardData(cfColumnSelect, 0);
	}

	if (selectedText.lineCopy) {
		::SetClipboardData(cfLineSelect, 0);
	}

	::CloseClipboard();
}

void ScintillaWin::ScrollMessage(WPARAM wParam) {
	//DWORD dwStart = timeGetTime();
	//Platform::DebugPrintf("Scroll %x %d\n", wParam, lParam);

	SCROLLINFO sci;
	memset(&sci, 0, sizeof(sci));
	sci.cbSize = sizeof(sci);
	sci.fMask = SIF_ALL;

	GetScrollInfo(SB_VERT, &sci);

	//Platform::DebugPrintf("ScrollInfo %d mask=%x min=%d max=%d page=%d pos=%d track=%d\n", b,sci.fMask,
	//sci.nMin, sci.nMax, sci.nPage, sci.nPos, sci.nTrackPos);

	int topLineNew = topLine;
	switch (LoWord(wParam)) {
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
	PRectangle rcText = GetTextRectangle();
	int pageWidth = rcText.Width() * 2 / 3;
	switch (LoWord(wParam)) {
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
			xPos = scrollWidth - rcText.Width();
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

void ScintillaWin::RealizeWindowPalette(bool inBackGround) {
	RefreshStyleData();
	HDC hdc = ::GetDC(MainHWND());
	// Select a stock font to prevent warnings from BoundsChecker
	::SelectObject(hdc, GetStockFont(DEFAULT_GUI_FONT));
	AutoSurface surfaceWindow(hdc, this);
	if (surfaceWindow) {
		int changes = surfaceWindow->SetPalette(&palette, inBackGround);
		if (changes > 0)
			Redraw();
		surfaceWindow->Release();
	}
	::ReleaseDC(MainHWND(), hdc);
}

/**
 * Redraw all of text area.
 * This paint will not be abandoned.
 */
void ScintillaWin::FullPaint() {
	HDC hdc = ::GetDC(MainHWND());
	FullPaintDC(hdc);
	::ReleaseDC(MainHWND(), hdc);
}

/**
 * Redraw all of text area on the specified DC.
 * This paint will not be abandoned.
 */
void ScintillaWin::FullPaintDC(HDC hdc) {
	paintState = painting;
	rcPaint = GetClientRectangle();
	paintingAllText = true;
	AutoSurface surfaceWindow(hdc, this);
	if (surfaceWindow) {
		Paint(surfaceWindow, rcPaint);
		surfaceWindow->Release();
	}
	paintState = notPainting;
}

static bool CompareDevCap(HDC hdc, HDC hOtherDC, int nIndex) {
	return ::GetDeviceCaps(hdc, nIndex) == ::GetDeviceCaps(hOtherDC, nIndex);
}

bool ScintillaWin::IsCompatibleDC(HDC hOtherDC) {
	HDC hdc = ::GetDC(MainHWND());
	bool isCompatible =
		CompareDevCap(hdc, hOtherDC, TECHNOLOGY) &&
		CompareDevCap(hdc, hOtherDC, LOGPIXELSY) &&
		CompareDevCap(hdc, hOtherDC, LOGPIXELSX) &&
		CompareDevCap(hdc, hOtherDC, BITSPIXEL) &&
		CompareDevCap(hdc, hOtherDC, PLANES);
	::ReleaseDC(MainHWND(), hdc);
	return isCompatible;
}

DWORD ScintillaWin::EffectFromState(DWORD grfKeyState) {
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
	*ppv = NULL;
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
	if (pIDataSource == NULL)
		return E_POINTER;
	FORMATETC fmtu = {CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	HRESULT hrHasUText = pIDataSource->QueryGetData(&fmtu);
	hasOKText = (hrHasUText == S_OK);
	if (!hasOKText) {
		FORMATETC fmte = {CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		HRESULT hrHasText = pIDataSource->QueryGetData(&fmte);
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
		SetDragPosition(SPositionFromLocation(Point(rpt.x, rpt.y), false, false, UserVirtualSpace()));

		return S_OK;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return E_FAIL;
}

STDMETHODIMP ScintillaWin::DragLeave() {
	try {
		SetDragPosition(SelectionPosition(invalidPosition));
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

		if (pIDataSource == NULL)
			return E_POINTER;

		SetDragPosition(SelectionPosition(invalidPosition));

		STGMEDIUM medium = {0, {0}, 0};

		char *data = 0;
		bool dataAllocated = false;

		FORMATETC fmtu = {CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		HRESULT hr = pIDataSource->GetData(&fmtu, &medium);
		if (SUCCEEDED(hr) && medium.hGlobal) {
			wchar_t *udata = static_cast<wchar_t *>(::GlobalLock(medium.hGlobal));
			if (IsUnicodeMode()) {
				int tlen = ::GlobalSize(medium.hGlobal);
				// Convert UTF-16 to UTF-8
				int dataLen = UTF8Length(udata, tlen/2);
				data = new char[dataLen+1];
				UTF8FromUTF16(udata, tlen/2, data, dataLen);
				dataAllocated = true;
			} else {
				// Convert UTF-16 to ANSI
				//
				// Default Scintilla behavior in Unicode mode
				// CF_UNICODETEXT available, but not in Unicode mode
				// Convert from Unicode to current Scintilla code page
				UINT cpDest = CodePageOfDocument();
				int tlen = ::WideCharToMultiByte(cpDest, 0, udata, -1,
					NULL, 0, NULL, NULL) - 1; // subtract 0 terminator
				data = new char[tlen + 1];
				memset(data, 0, (tlen+1));
				::WideCharToMultiByte(cpDest, 0, udata, -1,
						data, tlen + 1, NULL, NULL);
				dataAllocated = true;
			}
		}

		if (!data) {
			FORMATETC fmte = {CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			hr = pIDataSource->GetData(&fmte, &medium);
			if (SUCCEEDED(hr) && medium.hGlobal) {
				data = static_cast<char *>(::GlobalLock(medium.hGlobal));
			}
		}

		if (data && convertPastes) {
			// Convert line endings of the drop into our local line-endings mode
			int len = strlen(data);
			char *convertedText = Document::TransformLineEnds(&len, data, len, pdoc->eolMode);
			if (dataAllocated)
				delete []data;
			data = convertedText;
			dataAllocated = true;
		}

		if (!data) {
			//Platform::DebugPrintf("Bad data format: 0x%x\n", hres);
			return hr;
		}

		FORMATETC fmtr = {cfColumnSelect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		HRESULT hrRectangular = pIDataSource->QueryGetData(&fmtr);

		POINT rpt = {pt.x, pt.y};
		::ScreenToClient(MainHWND(), &rpt);
		SelectionPosition movePos = SPositionFromLocation(Point(rpt.x, rpt.y), false, false, UserVirtualSpace());

		DropAt(movePos, data, *pdwEffect == DROPEFFECT_MOVE, hrRectangular == S_OK);

		::GlobalUnlock(medium.hGlobal);

		// Free data
		if (medium.pUnkForRelease != NULL)
			medium.pUnkForRelease->Release();
		else
			::GlobalFree(medium.hGlobal);

		if (dataAllocated)
			delete []data;

		return S_OK;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return E_FAIL;
}

/// Implement important part of IDataObject
STDMETHODIMP ScintillaWin::GetData(FORMATETC *pFEIn, STGMEDIUM *pSTM) {
	bool formatOK = (pFEIn->cfFormat == CF_TEXT) ||
		((pFEIn->cfFormat == CF_UNICODETEXT) && IsUnicodeMode());
	if (!formatOK ||
	    pFEIn->ptd != 0 ||
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
		int uchars = UTF16Length(drag.s, drag.len);
		text.Allocate(2 * uchars);
		if (text) {
			UTF16FromUTF8(drag.s, drag.len, static_cast<wchar_t *>(text.ptr), uchars);
		}
	} else {
		text.Allocate(drag.len);
		if (text) {
			memcpy(static_cast<char *>(text.ptr), drag.s, drag.len);
		}
	}
	pSTM->hGlobal = text ? text.Unlock() : 0;
	pSTM->pUnkForRelease = 0;
	return S_OK;
}

bool ScintillaWin::Register(HINSTANCE hInstance_) {

	hInstance = hInstance_;
	bool result;

	// Register the Scintilla class
	if (IsNT()) {

		// Register Scintilla as a wide character window
		WNDCLASSEXW wndclass;
		wndclass.cbSize = sizeof(wndclass);
		wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = ScintillaWin::SWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = sizeof(ScintillaWin *);
		wndclass.hInstance = hInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = L"Scintilla";
		wndclass.hIconSm = 0;
		result = ::RegisterClassExW(&wndclass) != 0;
	} else {

		// Register Scintilla as a normal character window
		WNDCLASSEX wndclass;
		wndclass.cbSize = sizeof(wndclass);
		wndclass.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = ScintillaWin::SWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = sizeof(ScintillaWin *);
		wndclass.hInstance = hInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = scintillaClassName;
		wndclass.hIconSm = 0;
		result = ::RegisterClassEx(&wndclass) != 0;
	}

	if (result) {
		// Register the CallTip class
		WNDCLASSEX wndclassc;
		wndclassc.cbSize = sizeof(wndclassc);
		wndclassc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
		wndclassc.cbClsExtra = 0;
		wndclassc.cbWndExtra = sizeof(ScintillaWin *);
		wndclassc.hInstance = hInstance;
		wndclassc.hIcon = NULL;
		wndclassc.hbrBackground = NULL;
		wndclassc.lpszMenuName = NULL;
		wndclassc.lpfnWndProc = ScintillaWin::CTWndProc;
		wndclassc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wndclassc.lpszClassName = callClassName;
		wndclassc.hIconSm = 0;

		result = ::RegisterClassEx(&wndclassc) != 0;
	}

	return result;
}

bool ScintillaWin::Unregister() {
	bool result = ::UnregisterClass(scintillaClassName, hInstance) != 0;
	if (::UnregisterClass(callClassName, hInstance) == 0)
		result = false;
	return result;
}

bool ScintillaWin::HasCaretSizeChanged() {
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
	int bitmapSize = (((sysCaretWidth + 15) & ~15) >> 3) *
		sysCaretHeight;
	char *bits = new char[bitmapSize];
	memset(bits, 0, bitmapSize);
	sysCaretBitmap = ::CreateBitmap(sysCaretWidth, sysCaretHeight, 1,
		1, reinterpret_cast<BYTE *>(bits));
	delete []bits;
	BOOL retval = ::CreateCaret(
		MainHWND(), sysCaretBitmap,
		sysCaretWidth, sysCaretHeight);
	::ShowCaret(MainHWND());
	return retval;
}

BOOL ScintillaWin::DestroySystemCaret() {
	::HideCaret(MainHWND());
	BOOL retval = ::DestroyCaret();
	if (sysCaretBitmap) {
		::DeleteObject(sysCaretBitmap);
		sysCaretBitmap = 0;
	}
	return retval;
}

sptr_t PASCAL ScintillaWin::CTWndProc(
    HWND hWnd, UINT iMessage, WPARAM wParam, sptr_t lParam) {
	// Find C++ object associated with window.
	ScintillaWin *sciThis = reinterpret_cast<ScintillaWin *>(PointerFromWindow(hWnd));
	try {
		// ctp will be zero if WM_CREATE not seen yet
		if (sciThis == 0) {
			if (iMessage == WM_CREATE) {
				// Associate CallTip object with window
				CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
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
				Surface *surfaceWindow = Surface::Allocate();
				if (surfaceWindow) {
					surfaceWindow->Init(ps.hdc, hWnd);
					surfaceWindow->SetUnicodeMode(SC_CP_UTF8 == sciThis->ct.codePage);
					surfaceWindow->SetDBCSMode(sciThis->ct.codePage);
					sciThis->ct.PaintCT(surfaceWindow);
					surfaceWindow->Release();
					delete surfaceWindow;
				}
				::EndPaint(hWnd, &ps);
				return 0;
			} else if ((iMessage == WM_NCLBUTTONDOWN) || (iMessage == WM_NCLBUTTONDBLCLK)) {
				POINT pt;
				pt.x = static_cast<short>(LOWORD(lParam));
				pt.y = static_cast<short>(HIWORD(lParam));
				ScreenToClient(hWnd, &pt);
				sciThis->ct.MouseClick(Point(pt.x, pt.y));
				sciThis->CallTipClick();
				return 0;
			} else if (iMessage == WM_LBUTTONDOWN) {
				// This does not fire due to the hit test code
				sciThis->ct.MouseClick(Point::FromLong(lParam));
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
    ScintillaWin *sci, UINT iMessage, uptr_t wParam, sptr_t lParam) {
	PLATFORM_ASSERT(::GetCurrentThreadId() == ::GetWindowThreadProcessId(sci->MainHWND(), NULL));
	return sci->WndProc(iMessage, wParam, lParam);
}

extern "C"
#ifndef STATIC_BUILD
__declspec(dllexport)
#endif
sptr_t __stdcall Scintilla_DirectFunction(
    ScintillaWin *sci, UINT iMessage, uptr_t wParam, sptr_t lParam) {
	return sci->WndProc(iMessage, wParam, lParam);
}

sptr_t PASCAL ScintillaWin::SWndProc(
    HWND hWnd, UINT iMessage, WPARAM wParam, sptr_t lParam) {
	//Platform::DebugPrintf("S W:%x M:%x WP:%x L:%x\n", hWnd, iMessage, wParam, lParam);

	// Find C++ object associated with window.
	ScintillaWin *sci = reinterpret_cast<ScintillaWin *>(PointerFromWindow(hWnd));
	// sci will be zero if WM_CREATE not seen yet
	if (sci == 0) {
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
	bool result = ScintillaWin::Register(reinterpret_cast<HINSTANCE>(hInstance));
#ifdef SCI_LEXER
	Scintilla_LinkLexers();
#endif
	return result;
}

// This function is externally visible so it can be called from container when building statically.
int Scintilla_ReleaseResources() {
	bool result = ScintillaWin::Unregister();
	Platform_Finalise();
	return result;
}

#ifndef STATIC_BUILD
extern "C" int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID) {
	//Platform::DebugPrintf("Scintilla::DllMain %d %d\n", hInstance, dwReason);
	if (dwReason == DLL_PROCESS_ATTACH) {
		if (!Scintilla_RegisterClasses(hInstance))
			return FALSE;
	} else if (dwReason == DLL_PROCESS_DETACH) {
		Scintilla_ReleaseResources();
	}
	return TRUE;
}
#endif
