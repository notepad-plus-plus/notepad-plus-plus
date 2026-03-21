// Win32 Toolbar, ReBar, and ImageList shim (data-only, no visual rendering)
// Extracted from win32_controls.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"
#include "win32_toolbar_rebar_impl.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>

// ============================================================
// Toolbar data (data-only, no visual rendering)
// ============================================================

struct ToolbarButtonData
{
	int iBitmap = 0;
	int idCommand = 0;
	BYTE fsState = TBSTATE_ENABLED;
	BYTE fsStyle = TBSTYLE_BUTTON;
	DWORD_PTR dwData = 0;
	INT_PTR iString = 0;
};

struct ToolbarData
{
	HWND hwnd = nullptr;
	std::vector<ToolbarButtonData> buttons;
	int buttonSizeCx = 24;
	int buttonSizeCy = 22;
};

static std::unordered_map<uintptr_t, ToolbarData> s_toolBars;

// ============================================================
// ReBar data (data-only)
// ============================================================

struct ReBarBandData
{
	UINT fMask = 0;
	UINT fStyle = 0;
	HWND hwndChild = nullptr;
	UINT cxMinChild = 0;
	UINT cyMinChild = 0;
	UINT cx = 0;
	UINT wID = 0;
	std::wstring text;
};

struct ReBarData
{
	HWND hwnd = nullptr;
	std::vector<ReBarBandData> bands;
};

static std::unordered_map<uintptr_t, ReBarData> s_reBars;

// ============================================================
// Toolbar init/destroy
// ============================================================

void Win32Toolbar_Init(void* hwndVoid)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);
	ToolbarData data;
	data.hwnd = hwnd;
	s_toolBars[key] = data;
}

void Win32Toolbar_Destroy(void* hwndVoid)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);
	s_toolBars.erase(key);
}

// ============================================================
// ReBar init/destroy
// ============================================================

void Win32ReBar_Init(void* hwndVoid)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);
	ReBarData data;
	data.hwnd = hwnd;
	s_reBars[key] = data;
}

void Win32ReBar_Destroy(void* hwndVoid)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);
	s_reBars.erase(key);
}

// ============================================================
// Toolbar message handling (data-only)
// ============================================================

bool Win32Toolbar_HandleMessage(void* hwndVoid, unsigned int msg,
                                 uintptr_t wParam, intptr_t lParam,
                                 intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);
	auto it = s_toolBars.find(key);
	if (it == s_toolBars.end()) return false;

	auto& tb = it->second;

	switch (msg)
	{
		case TB_BUTTONSTRUCTSIZE:
			result = 0;
			return true;

		case TB_ADDBUTTONS:
		{
			int count = static_cast<int>(wParam);
			const TBBUTTON* buttons = reinterpret_cast<const TBBUTTON*>(lParam);
			if (buttons)
			{
				for (int i = 0; i < count; ++i)
				{
					ToolbarButtonData btn;
					btn.iBitmap = buttons[i].iBitmap;
					btn.idCommand = buttons[i].idCommand;
					btn.fsState = buttons[i].fsState;
					btn.fsStyle = buttons[i].fsStyle;
					btn.dwData = buttons[i].dwData;
					btn.iString = buttons[i].iString;
					tb.buttons.push_back(btn);
				}
			}
			result = TRUE;
			return true;
		}

		case TB_INSERTBUTTONW:
		{
			int index = static_cast<int>(wParam);
			const TBBUTTON* pBtn = reinterpret_cast<const TBBUTTON*>(lParam);
			if (pBtn)
			{
				ToolbarButtonData btn;
				btn.iBitmap = pBtn->iBitmap;
				btn.idCommand = pBtn->idCommand;
				btn.fsState = pBtn->fsState;
				btn.fsStyle = pBtn->fsStyle;
				btn.dwData = pBtn->dwData;
				btn.iString = pBtn->iString;

				if (index < 0 || index > static_cast<int>(tb.buttons.size()))
					index = static_cast<int>(tb.buttons.size());
				tb.buttons.insert(tb.buttons.begin() + index, btn);
			}
			result = TRUE;
			return true;
		}

		case TB_DELETEBUTTON:
		{
			int index = static_cast<int>(wParam);
			if (index >= 0 && index < static_cast<int>(tb.buttons.size()))
			{
				tb.buttons.erase(tb.buttons.begin() + index);
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case TB_BUTTONCOUNT:
			result = static_cast<intptr_t>(tb.buttons.size());
			return true;

		case TB_GETBUTTON:
		{
			int index = static_cast<int>(wParam);
			TBBUTTON* pBtn = reinterpret_cast<TBBUTTON*>(lParam);
			if (pBtn && index >= 0 && index < static_cast<int>(tb.buttons.size()))
			{
				const auto& btn = tb.buttons[index];
				pBtn->iBitmap = btn.iBitmap;
				pBtn->idCommand = btn.idCommand;
				pBtn->fsState = btn.fsState;
				pBtn->fsStyle = btn.fsStyle;
				pBtn->dwData = btn.dwData;
				pBtn->iString = btn.iString;
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case TB_COMMANDTOINDEX:
		{
			int cmdId = static_cast<int>(wParam);
			for (int i = 0; i < static_cast<int>(tb.buttons.size()); ++i)
			{
				if (tb.buttons[i].idCommand == cmdId)
				{
					result = i;
					return true;
				}
			}
			result = -1;
			return true;
		}

		case TB_ENABLEBUTTON:
		{
			int cmdId = static_cast<int>(wParam);
			bool enable = lParam != 0;
			for (auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					if (enable)
						btn.fsState |= TBSTATE_ENABLED;
					else
						btn.fsState &= ~TBSTATE_ENABLED;
					break;
				}
			}
			result = TRUE;
			return true;
		}

		case TB_CHECKBUTTON:
		{
			int cmdId = static_cast<int>(wParam);
			bool check = lParam != 0;
			for (auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					if (check)
						btn.fsState |= TBSTATE_CHECKED;
					else
						btn.fsState &= ~TBSTATE_CHECKED;
					break;
				}
			}
			result = TRUE;
			return true;
		}

		case TB_HIDEBUTTON:
		{
			int cmdId = static_cast<int>(wParam);
			bool hide = lParam != 0;
			for (auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					if (hide)
						btn.fsState |= TBSTATE_HIDDEN;
					else
						btn.fsState &= ~TBSTATE_HIDDEN;
					break;
				}
			}
			result = TRUE;
			return true;
		}

		case TB_ISBUTTONENABLED:
		{
			int cmdId = static_cast<int>(wParam);
			for (const auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					result = (btn.fsState & TBSTATE_ENABLED) ? TRUE : FALSE;
					return true;
				}
			}
			result = FALSE;
			return true;
		}

		case TB_ISBUTTONCHECKED:
		{
			int cmdId = static_cast<int>(wParam);
			for (const auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					result = (btn.fsState & TBSTATE_CHECKED) ? TRUE : FALSE;
					return true;
				}
			}
			result = FALSE;
			return true;
		}

		case TB_ISBUTTONHIDDEN:
		{
			int cmdId = static_cast<int>(wParam);
			for (const auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					result = (btn.fsState & TBSTATE_HIDDEN) ? TRUE : FALSE;
					return true;
				}
			}
			result = FALSE;
			return true;
		}

		case TB_GETSTATE:
		{
			int cmdId = static_cast<int>(wParam);
			for (const auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					result = btn.fsState;
					return true;
				}
			}
			result = -1;
			return true;
		}

		case TB_SETSTATE:
		{
			int cmdId = static_cast<int>(wParam);
			BYTE newState = static_cast<BYTE>(LOWORD(lParam));
			for (auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					btn.fsState = newState;
					break;
				}
			}
			result = TRUE;
			return true;
		}

		case TB_SETBUTTONSIZE:
		{
			tb.buttonSizeCx = LOWORD(lParam);
			tb.buttonSizeCy = HIWORD(lParam);
			result = TRUE;
			return true;
		}

		case TB_GETBUTTONSIZE:
			result = MAKELONG(tb.buttonSizeCx, tb.buttonSizeCy);
			return true;

		case TB_SETBITMAPSIZE:
		case TB_SETIMAGELIST:
		case TB_GETIMAGELIST:
		case TB_SETHOTIMAGELIST:
		case TB_SETDISABLEDIMAGELIST:
		case TB_AUTOSIZE:
		case TB_SETMAXTEXTROWS:
		case TB_GETTOOLTIPS:
		case TB_SETTOOLTIPS:
		case TB_SETPARENT:
		case TB_SETEXTENDEDSTYLE:
		case TB_GETEXTENDEDSTYLE:
		case TB_ADDBITMAP:
		case TB_SETDRAWTEXTFLAGS:
		case TB_SETPADDING:
		case TB_GETPADDING:
		case TB_SETCMDID:
		case TB_CUSTOMIZE:
		case TB_SAVERESTORE:
		case TB_PRESSBUTTON:
		case TB_INDETERMINATE:
		case TB_ISBUTTONPRESSED:
		case TB_SETROWS:
		case TB_GETROWS:
		case TB_GETBITMAPFLAGS:
			result = 0;
			return true;

		case TB_GETITEMRECT:
		case TB_GETRECT:
		{
			RECT* pRect = reinterpret_cast<RECT*>(lParam);
			if (pRect)
			{
				int index = -1;
				if (msg == TB_GETITEMRECT)
				{
					// TB_GETITEMRECT: wParam is a 0-based button index
					index = static_cast<int>(wParam);
				}
				else // TB_GETRECT
				{
					// TB_GETRECT: wParam is a command ID; map it to a button index
					LRESULT idx = SendMessageW(hwnd, TB_COMMANDTOINDEX, wParam, 0);
					index = static_cast<int>(idx);
				}

				if (index >= 0)
				{
					pRect->left = index * tb.buttonSizeCx;
					pRect->top = 0;
					pRect->right = pRect->left + tb.buttonSizeCx;
					pRect->bottom = tb.buttonSizeCy;
					result = TRUE;
				}
				else
				{
					memset(pRect, 0, sizeof(RECT));
					result = FALSE;
				}
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case TB_GETBUTTONINFOW:
		case TB_SETBUTTONINFOW:
			result = -1;
			return true;

		case TB_GETITEMDROPDOWNRECT:
		{
			RECT* pRect = reinterpret_cast<RECT*>(lParam);
			if (pRect)
				memset(pRect, 0, sizeof(RECT));
			result = FALSE;
			return true;
		}
	}

	return false;
}

// ============================================================
// ReBar message handling (data-only)
// ============================================================

bool Win32ReBar_HandleMessage(void* hwndVoid, unsigned int msg,
                                uintptr_t wParam, intptr_t lParam,
                                intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);
	auto it = s_reBars.find(key);
	if (it == s_reBars.end()) return false;

	auto& rb = it->second;

	switch (msg)
	{
		case RB_INSERTBANDW:
		{
			const REBARBANDINFOW* pBand = reinterpret_cast<const REBARBANDINFOW*>(lParam);
			if (pBand)
			{
				ReBarBandData band;
				band.fMask = pBand->fMask;
				band.fStyle = pBand->fStyle;
				if (pBand->fMask & RBBIM_CHILD) band.hwndChild = pBand->hwndChild;
				if (pBand->fMask & RBBIM_CHILDSIZE) { band.cxMinChild = pBand->cxMinChild; band.cyMinChild = pBand->cyMinChild; }
				if (pBand->fMask & RBBIM_SIZE) band.cx = pBand->cx;
				if (pBand->fMask & RBBIM_ID) band.wID = pBand->wID;
				if (pBand->fMask & RBBIM_TEXT && pBand->lpText) band.text = pBand->lpText;

				int index = static_cast<int>(wParam);
				if (index < 0 || index > static_cast<int>(rb.bands.size()))
					index = static_cast<int>(rb.bands.size());
				rb.bands.insert(rb.bands.begin() + index, band);
			}
			result = TRUE;
			return true;
		}

		case RB_DELETEBAND:
		{
			int index = static_cast<int>(wParam);
			if (index >= 0 && index < static_cast<int>(rb.bands.size()))
			{
				rb.bands.erase(rb.bands.begin() + index);
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case RB_GETBANDCOUNT:
			result = static_cast<intptr_t>(rb.bands.size());
			return true;

		case RB_SETBANDINFOW:
		{
			int index = static_cast<int>(wParam);
			const REBARBANDINFOW* pBand = reinterpret_cast<const REBARBANDINFOW*>(lParam);
			if (pBand && index >= 0 && index < static_cast<int>(rb.bands.size()))
			{
				auto& band = rb.bands[index];
				if (pBand->fMask & RBBIM_STYLE) band.fStyle = pBand->fStyle;
				if (pBand->fMask & RBBIM_CHILD) band.hwndChild = pBand->hwndChild;
				if (pBand->fMask & RBBIM_CHILDSIZE) { band.cxMinChild = pBand->cxMinChild; band.cyMinChild = pBand->cyMinChild; }
				if (pBand->fMask & RBBIM_SIZE) band.cx = pBand->cx;
				if (pBand->fMask & RBBIM_ID) band.wID = pBand->wID;
				if (pBand->fMask & RBBIM_TEXT && pBand->lpText) band.text = pBand->lpText;
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case RB_GETBANDINFOW:
		{
			int index = static_cast<int>(wParam);
			REBARBANDINFOW* pBand = reinterpret_cast<REBARBANDINFOW*>(lParam);
			if (pBand && index >= 0 && index < static_cast<int>(rb.bands.size()))
			{
				const auto& band = rb.bands[index];
				if (pBand->fMask & RBBIM_STYLE) pBand->fStyle = band.fStyle;
				if (pBand->fMask & RBBIM_CHILD) pBand->hwndChild = band.hwndChild;
				if (pBand->fMask & RBBIM_CHILDSIZE) { pBand->cxMinChild = band.cxMinChild; pBand->cyMinChild = band.cyMinChild; }
				if (pBand->fMask & RBBIM_SIZE) pBand->cx = band.cx;
				if (pBand->fMask & RBBIM_ID) pBand->wID = band.wID;
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case RB_IDTOINDEX:
		{
			UINT id = static_cast<UINT>(wParam);
			for (int i = 0; i < static_cast<int>(rb.bands.size()); ++i)
			{
				if (rb.bands[i].wID == id)
				{
					result = i;
					return true;
				}
			}
			result = -1;
			return true;
		}

		case RB_SHOWBAND:
		{
			int index = static_cast<int>(wParam);
			if (index >= 0 && index < static_cast<int>(rb.bands.size()))
			{
				if (lParam)
					rb.bands[index].fStyle &= ~RBBS_HIDDEN;
				else
					rb.bands[index].fStyle |= RBBS_HIDDEN;
			}
			result = TRUE;
			return true;
		}

		case RB_SETBARINFO:
		case RB_GETBARINFO:
		case RB_GETROWCOUNT:
		case RB_GETROWHEIGHT:
		case RB_SIZETORECT:
		case RB_SETBKCOLOR:
		case RB_GETBKCOLOR:
		case RB_SETTEXTCOLOR:
		case RB_GETTEXTCOLOR:
		case RB_MOVEBAND:
		case RB_GETBARHEIGHT:
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// ImageList stubs (count-tracking only)
// ============================================================

struct ImageListData
{
	int cx = 0;
	int cy = 0;
	UINT flags = 0;
	int count = 0;
};

static std::unordered_map<uintptr_t, ImageListData> s_imageLists;
static uintptr_t s_nextImageList = 0x30000;

HIMAGELIST ImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow)
{
	HIMAGELIST himl = reinterpret_cast<HIMAGELIST>(s_nextImageList++);
	ImageListData data;
	data.cx = cx;
	data.cy = cy;
	data.flags = flags;
	data.count = 0;
	s_imageLists[reinterpret_cast<uintptr_t>(himl)] = data;
	return himl;
}

BOOL ImageList_Destroy(HIMAGELIST himl)
{
	s_imageLists.erase(reinterpret_cast<uintptr_t>(himl));
	return TRUE;
}

int ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it != s_imageLists.end())
		return it->second.count++;
	return -1;
}

int ImageList_AddMasked(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask)
{
	return ImageList_Add(himl, hbmImage, nullptr);
}

int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it == s_imageLists.end()) return -1;

	if (i == -1) // append
		return it->second.count++;
	return i;
}

BOOL ImageList_Remove(HIMAGELIST himl, int i)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it == s_imageLists.end()) return FALSE;

	if (i == -1) // remove all
		it->second.count = 0;
	else if (it->second.count > 0)
		--it->second.count;

	return TRUE;
}

int ImageList_GetImageCount(HIMAGELIST himl)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	return (it != s_imageLists.end()) ? it->second.count : 0;
}

BOOL ImageList_SetImageCount(HIMAGELIST himl, UINT uNewCount)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it != s_imageLists.end())
	{
		it->second.count = static_cast<int>(uNewCount);
		return TRUE;
	}
	return FALSE;
}

BOOL ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle)
{
	return TRUE; // stub
}

BOOL ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy,
                       COLORREF rgbBk, COLORREF rgbFg, UINT fStyle)
{
	return TRUE; // stub
}

BOOL ImageList_SetIconSize(HIMAGELIST himl, int cx, int cy)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it != s_imageLists.end())
	{
		it->second.cx = cx;
		it->second.cy = cy;
		return TRUE;
	}
	return FALSE;
}

HICON ImageList_GetIcon(HIMAGELIST himl, int i, UINT flags)
{
	return nullptr;
}

BOOL ImageList_GetIconSize(HIMAGELIST himl, int* cx, int* cy)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it != s_imageLists.end())
	{
		if (cx) *cx = it->second.cx;
		if (cy) *cy = it->second.cy;
		return TRUE;
	}
	return FALSE;
}

BOOL ImageList_GetImageInfo(HIMAGELIST himl, int i, IMAGEINFO* pImageInfo)
{
	if (pImageInfo) memset(pImageInfo, 0, sizeof(IMAGEINFO));
	return FALSE;
}

BOOL ImageList_BeginDrag(HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot) { return TRUE; }
BOOL ImageList_DragEnter(HWND hwndLock, int x, int y) { return TRUE; }
BOOL ImageList_DragMove(int x, int y) { return TRUE; }
BOOL ImageList_DragShowNolock(BOOL fShow) { return TRUE; }
BOOL ImageList_DragLeave(HWND hwndLock) { return TRUE; }
void ImageList_EndDrag() {}
HIMAGELIST ImageList_Merge(HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy) { return nullptr; }
