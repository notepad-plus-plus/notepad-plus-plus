// Win32 Tab Control shim: TabItemData, TabControlData, NSSegmentedControl backing
// Extracted from win32_controls.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"
#include "win32_tab_control_impl.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

// ============================================================
// Tab Control data
// ============================================================

struct TabItemData
{
	std::wstring text;
	int image = -1;
	LPARAM lParam = 0;
};

struct TabControlData
{
	std::vector<TabItemData> items;
	int currentSel = -1;
	HWND hwnd = nullptr;
	HWND parent = nullptr;
};

static std::unordered_map<uintptr_t, TabControlData> s_tabControls;

// ============================================================
// ObjC helper: Routes NSSegmentedControl selection -> WM_NOTIFY/TCN_SELCHANGE
// ============================================================

@interface Win32TabTarget : NSObject
@property (assign) HWND tabHwnd;
- (void)tabSelectionChanged:(id)sender;
@end

@implementation Win32TabTarget
- (void)tabSelectionChanged:(id)sender
{
	NSSegmentedControl* seg = (NSSegmentedControl*)sender;
	uintptr_t key = reinterpret_cast<uintptr_t>(self.tabHwnd);
	auto it = s_tabControls.find(key);
	if (it == s_tabControls.end()) return;

	int newSel = static_cast<int>([seg selectedSegment]);
	int oldSel = it->second.currentSel;
	it->second.currentSel = newSel;

	// Send WM_NOTIFY with TCN_SELCHANGE to the parent
	HWND parentHwnd = it->second.parent;
	if (parentHwnd)
	{
		auto* parentInfo = HandleRegistry::getWindowInfo(parentHwnd);
		auto* tabInfo = HandleRegistry::getWindowInfo(self.tabHwnd);
		if (parentInfo && parentInfo->wndProc && tabInfo)
		{
			NMHDR nmhdr;
			nmhdr.hwndFrom = self.tabHwnd;
			nmhdr.idFrom = static_cast<UINT_PTR>(tabInfo->controlId);
			nmhdr.code = TCN_SELCHANGE;
			parentInfo->wndProc(parentHwnd, WM_NOTIFY, nmhdr.idFrom,
			                    reinterpret_cast<LPARAM>(&nmhdr));
		}
	}
}
@end

// Global map of tab targets (prevent deallocation under ARC)
static NSMutableDictionary<NSNumber*, Win32TabTarget*>* s_tabTargets = nil;

// ============================================================
// Tab control init/destroy
// ============================================================

void Win32TabControl_Init(void* hwndVoid, void* parentVoid)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	HWND parent = reinterpret_cast<HWND>(parentVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);

	TabControlData data;
	data.hwnd = hwnd;
	data.parent = parent;
	s_tabControls[key] = data;

	// Set up action target for the NSSegmentedControl
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (info && info->nativeView)
	{
		if (!s_tabTargets)
			s_tabTargets = [NSMutableDictionary dictionary];

		Win32TabTarget* target = [[Win32TabTarget alloc] init];
		target.tabHwnd = hwnd;

		NSSegmentedControl* seg = (__bridge NSSegmentedControl*)info->nativeView;
		seg.target = target;
		seg.action = @selector(tabSelectionChanged:);

		s_tabTargets[@(key)] = target;
	}
}

void Win32TabControl_Destroy(void* hwndVoid)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);
	s_tabControls.erase(key);
	NSNumber* num = @(key);
	[s_tabTargets removeObjectForKey:num];
}

// ============================================================
// Tab control message handling
// ============================================================

bool Win32TabControl_HandleMessage(void* hwndVoid, unsigned int msg,
                                    uintptr_t wParam, intptr_t lParam,
                                    intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);
	auto it = s_tabControls.find(key);
	if (it == s_tabControls.end()) return false;

	auto& tab = it->second;
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	NSSegmentedControl* seg = info ? (__bridge NSSegmentedControl*)info->nativeView : nil;

	switch (msg)
	{
		case TCM_INSERTITEMW:
		{
			int index = static_cast<int>(wParam);
			const TCITEMW* pItem = reinterpret_cast<const TCITEMW*>(lParam);
			if (!pItem) { result = -1; return true; }

			TabItemData item;
			if (pItem->mask & TCIF_TEXT && pItem->pszText)
				item.text = pItem->pszText;
			if (pItem->mask & TCIF_IMAGE)
				item.image = pItem->iImage;
			if (pItem->mask & TCIF_PARAM)
				item.lParam = pItem->lParam;

			if (index < 0 || index > static_cast<int>(tab.items.size()))
				index = static_cast<int>(tab.items.size());

			tab.items.insert(tab.items.begin() + index, item);

			// Update NSSegmentedControl
			if (seg)
			{
				seg.segmentCount = static_cast<NSInteger>(tab.items.size());
				for (int i = 0; i < static_cast<int>(tab.items.size()); ++i)
				{
					[seg setLabel:WideToNS(tab.items[i].text.c_str()) forSegment:i];
					[seg setWidth:0 forSegment:i]; // auto-size
				}

				if (tab.currentSel < 0 && !tab.items.empty())
				{
					tab.currentSel = 0;
					seg.selectedSegment = 0;
				}
			}

			result = index;
			return true;
		}

		case TCM_DELETEITEM:
		{
			int index = static_cast<int>(wParam);
			if (index < 0 || index >= static_cast<int>(tab.items.size()))
			{
				result = FALSE;
				return true;
			}

			tab.items.erase(tab.items.begin() + index);

			if (seg)
			{
				seg.segmentCount = static_cast<NSInteger>(tab.items.size());
				for (int i = 0; i < static_cast<int>(tab.items.size()); ++i)
					[seg setLabel:WideToNS(tab.items[i].text.c_str()) forSegment:i];
			}

			if (tab.currentSel >= static_cast<int>(tab.items.size()))
				tab.currentSel = tab.items.empty() ? -1 : static_cast<int>(tab.items.size()) - 1;

			if (seg && tab.currentSel >= 0)
				seg.selectedSegment = tab.currentSel;

			result = TRUE;
			return true;
		}

		case TCM_DELETEALLITEMS:
		{
			tab.items.clear();
			tab.currentSel = -1;
			if (seg) seg.segmentCount = 0;
			result = TRUE;
			return true;
		}

		case TCM_GETCURSEL:
			result = tab.currentSel;
			return true;

		case TCM_SETCURSEL:
		{
			int index = static_cast<int>(wParam);
			int oldSel = tab.currentSel;

			if (index >= 0 && index < static_cast<int>(tab.items.size()))
			{
				tab.currentSel = index;
				if (seg) seg.selectedSegment = index;
			}

			result = oldSel;
			return true;
		}

		case TCM_GETITEMCOUNT:
			result = static_cast<intptr_t>(tab.items.size());
			return true;

		case TCM_GETITEMW:
		{
			int index = static_cast<int>(wParam);
			TCITEMW* pItem = reinterpret_cast<TCITEMW*>(lParam);
			if (!pItem || index < 0 || index >= static_cast<int>(tab.items.size()))
			{
				result = FALSE;
				return true;
			}

			const auto& item = tab.items[index];
			if (pItem->mask & TCIF_TEXT && pItem->pszText && pItem->cchTextMax > 0)
			{
				int maxCopy = pItem->cchTextMax - 1;
				int len = (std::min)(maxCopy, static_cast<int>(item.text.size()));
				wcsncpy(pItem->pszText, item.text.c_str(), len);
				pItem->pszText[len] = L'\0';
			}
			if (pItem->mask & TCIF_IMAGE)
				pItem->iImage = item.image;
			if (pItem->mask & TCIF_PARAM)
				pItem->lParam = item.lParam;

			result = TRUE;
			return true;
		}

		case TCM_SETITEMW:
		{
			int index = static_cast<int>(wParam);
			const TCITEMW* pItem = reinterpret_cast<const TCITEMW*>(lParam);
			if (!pItem || index < 0 || index >= static_cast<int>(tab.items.size()))
			{
				result = FALSE;
				return true;
			}

			auto& item = tab.items[index];
			if (pItem->mask & TCIF_TEXT && pItem->pszText)
			{
				item.text = pItem->pszText;
				if (seg)
					[seg setLabel:WideToNS(item.text.c_str()) forSegment:index];
			}
			if (pItem->mask & TCIF_IMAGE)
				item.image = pItem->iImage;
			if (pItem->mask & TCIF_PARAM)
				item.lParam = pItem->lParam;

			result = TRUE;
			return true;
		}

		case TCM_GETITEMRECT:
		{
			int index = static_cast<int>(wParam);
			RECT* pRect = reinterpret_cast<RECT*>(lParam);
			if (!pRect || !seg || index < 0 || index >= static_cast<int>(tab.items.size()))
			{
				result = FALSE;
				return true;
			}

			// Approximate: divide evenly
			CGFloat segWidth = seg.frame.size.width / (std::max)(1, (int)tab.items.size());
			pRect->left = static_cast<LONG>(index * segWidth);
			pRect->top = 0;
			pRect->right = static_cast<LONG>((index + 1) * segWidth);
			pRect->bottom = static_cast<LONG>(seg.frame.size.height);
			result = TRUE;
			return true;
		}

		case TCM_ADJUSTRECT:
		{
			// wParam TRUE  = given display rect, return window rect (larger)
			// wParam FALSE = given window rect, return display rect (smaller)
			// For simplicity, just shrink/expand by tab bar height
			RECT* pRect = reinterpret_cast<RECT*>(lParam);
			if (pRect)
			{
				if (wParam) // larger: expand upward to include tab bar
					pRect->top -= 28; // tab bar height
				else // smaller: shrink to exclude tab bar
					pRect->top += 28;
			}
			result = 0;
			return true;
		}

		case TCM_SETITEMSIZE:
		case TCM_SETPADDING:
		case TCM_SETIMAGELIST:
		case TCM_GETIMAGELIST:
		case TCM_SETMINTABWIDTH:
		case TCM_SETEXTENDEDSTYLE:
		case TCM_GETEXTENDEDSTYLE:
		case TCM_HIGHLIGHTITEM:
		case TCM_GETTOOLTIPS:
		case TCM_SETTOOLTIPS:
		case TCM_GETROWCOUNT:
		case TCM_GETCURFOCUS:
		case TCM_SETCURFOCUS:
			result = 0;
			return true;

		case TCM_HITTEST:
		{
			TCHITTESTINFO* pHitTest = reinterpret_cast<TCHITTESTINFO*>(lParam);
			if (!pHitTest || !seg)
			{
				result = -1;
				return true;
			}
			// Simple hit test: check which segment the point falls in
			CGFloat segWidth = seg.frame.size.width / (std::max)(1, (int)tab.items.size());
			int hitIndex = static_cast<int>(pHitTest->pt.x / segWidth);
			if (hitIndex >= 0 && hitIndex < static_cast<int>(tab.items.size()))
			{
				pHitTest->flags = TCHT_ONITEMLABEL;
				result = hitIndex;
			}
			else
			{
				pHitTest->flags = TCHT_NOWHERE;
				result = -1;
			}
			return true;
		}
	}

	return false;
}
