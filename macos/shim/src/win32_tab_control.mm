// Win32 Tab Control shim: TabItemData, TabControlData, NppTabBarView backing
// Extracted from win32_controls.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"
#include "win32_tab_control_impl.h"
#include "tab_bar_view.h"

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
// ObjC bridge: Routes NppTabBarView delegate calls -> WM_NOTIFY
// ============================================================

@interface NppTabBarBridge : NSObject <NppTabBarViewDelegate>
@property (assign) HWND tabHwnd;
@end

@implementation NppTabBarBridge

- (void)tabBarView:(NppTabBarView*)tabBar didSelectTabAtIndex:(NSInteger)index
{
	uintptr_t key = reinterpret_cast<uintptr_t>(self.tabHwnd);
	auto it = s_tabControls.find(key);
	if (it == s_tabControls.end()) return;

	it->second.currentSel = static_cast<int>(index);

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

- (void)tabBarView:(NppTabBarView*)tabBar didCloseTabAtIndex:(NSInteger)index
{
	uintptr_t key = reinterpret_cast<uintptr_t>(self.tabHwnd);
	auto it = s_tabControls.find(key);
	if (it == s_tabControls.end()) return;

	// Fire a WM_NOTIFY with NM_TAB_CLOSE so the platform layer can
	// handle prompting + closing through promptAndHandleClose -> closeTabFromView.
	HWND parentHwnd = it->second.parent;
	if (parentHwnd)
	{
		auto* parentInfo = HandleRegistry::getWindowInfo(parentHwnd);
		if (parentInfo && parentInfo->wndProc)
		{
			NMHDR nmhdr;
			nmhdr.hwndFrom = self.tabHwnd;
			nmhdr.idFrom = 0;
			nmhdr.code = NM_TAB_CLOSE;
			// Pack tab index into a custom notification struct
			struct TabCloseNotify {
				NMHDR hdr;
				int tabIndex;
			};
			TabCloseNotify tcn;
			tcn.hdr = nmhdr;
			tcn.tabIndex = static_cast<int>(index);
			parentInfo->wndProc(parentHwnd, WM_NOTIFY, 0,
			                    reinterpret_cast<LPARAM>(&tcn));
		}
	}
}

- (void)tabBarView:(NppTabBarView*)tabBar didMiddleClickTabAtIndex:(NSInteger)index
{
	// Same as close
	[self tabBarView:tabBar didCloseTabAtIndex:index];
}

- (void)tabBarView:(NppTabBarView*)tabBar didReorderTabFrom:(NSInteger)fromIndex to:(NSInteger)toIndex
{
	uintptr_t key = reinterpret_cast<uintptr_t>(self.tabHwnd);
	auto it = s_tabControls.find(key);
	if (it == s_tabControls.end()) return;

	HWND parentHwnd = it->second.parent;
	if (parentHwnd)
	{
		auto* parentInfo = HandleRegistry::getWindowInfo(parentHwnd);
		if (parentInfo && parentInfo->wndProc)
		{
			struct TabReorderNotify {
				NMHDR hdr;
				int fromIndex;
				int toIndex;
			};
			NMHDR nmhdr;
			nmhdr.hwndFrom = self.tabHwnd;
			nmhdr.idFrom = 0;
			nmhdr.code = NM_TAB_REORDER;
			TabReorderNotify trn;
			trn.hdr = nmhdr;
			trn.fromIndex = static_cast<int>(fromIndex);
			trn.toIndex = static_cast<int>(toIndex);
			parentInfo->wndProc(parentHwnd, WM_NOTIFY, 0,
			                    reinterpret_cast<LPARAM>(&trn));
		}
	}
}

- (void)tabBarView:(NppTabBarView*)tabBar didRightClickTabAtIndex:(NSInteger)index atPoint:(NSPoint)point
{
	uintptr_t key = reinterpret_cast<uintptr_t>(self.tabHwnd);
	auto it = s_tabControls.find(key);
	if (it == s_tabControls.end()) return;

	HWND parentHwnd = it->second.parent;
	if (parentHwnd)
	{
		auto* parentInfo = HandleRegistry::getWindowInfo(parentHwnd);
		if (parentInfo && parentInfo->wndProc)
		{
			struct TabContextNotify {
				NMHDR hdr;
				int tabIndex;
				NSPoint screenPoint;
			};
			NMHDR nmhdr;
			nmhdr.hwndFrom = self.tabHwnd;
			nmhdr.idFrom = 0;
			nmhdr.code = NM_TAB_CONTEXTMENU;
			TabContextNotify tcn;
			tcn.hdr = nmhdr;
			tcn.tabIndex = static_cast<int>(index);
			// Convert window point to screen point
			NSWindow* window = tabBar.window;
			if (window)
				tcn.screenPoint = [window convertPointToScreen:point];
			else
				tcn.screenPoint = point;
			parentInfo->wndProc(parentHwnd, WM_NOTIFY, 0,
			                    reinterpret_cast<LPARAM>(&tcn));
		}
	}
}

@end

// Global map of tab bridges (prevent deallocation under ARC)
static NSMutableDictionary<NSNumber*, NppTabBarBridge*>* s_tabBridges = nil;

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

	// Set up delegate for the NppTabBarView
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (info && info->nativeView)
	{
		if (!s_tabBridges)
			s_tabBridges = [NSMutableDictionary dictionary];

		NppTabBarBridge* bridge = [[NppTabBarBridge alloc] init];
		bridge.tabHwnd = hwnd;

		NppTabBarView* tabBarView = (__bridge NppTabBarView*)info->nativeView;
		tabBarView.delegate = bridge;

		s_tabBridges[@(key)] = bridge;
	}
}

void Win32TabControl_Destroy(void* hwndVoid)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);
	s_tabControls.erase(key);
	NSNumber* num = @(key);
	[s_tabBridges removeObjectForKey:num];
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
	NppTabBarView* tabView = info ? (__bridge NppTabBarView*)info->nativeView : nil;

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

			if (tabView)
			{
				[tabView insertTabWithTitle:WideToNS(item.text.c_str()) atIndex:index];
			}

			// Adjust currentSel when inserting at or before the selected tab
			if (tab.currentSel >= 0 && index <= tab.currentSel)
				++tab.currentSel;

			if (tab.currentSel < 0 && !tab.items.empty())
			{
				tab.currentSel = 0;
				if (tabView)
					[tabView selectTabAtIndex:0];
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

			if (tabView)
				[tabView removeTabAtIndex:index];

			// Adjust currentSel: decrement if deleting before, clamp if at/past end
			if (tab.items.empty())
				tab.currentSel = -1;
			else if (index < tab.currentSel)
				--tab.currentSel;
			else if (index == tab.currentSel || tab.currentSel >= static_cast<int>(tab.items.size()))
				tab.currentSel = (tab.currentSel >= static_cast<int>(tab.items.size()))
					? static_cast<int>(tab.items.size()) - 1
					: tab.currentSel;

			if (tabView && tab.currentSel >= 0)
				[tabView selectTabAtIndex:tab.currentSel];

			result = TRUE;
			return true;
		}

		case TCM_DELETEALLITEMS:
		{
			tab.items.clear();
			tab.currentSel = -1;
			if (tabView)
				[tabView removeAllTabs];
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
				if (tabView)
					[tabView selectTabAtIndex:index];
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
				if (tabView)
					[tabView setTitle:WideToNS(item.text.c_str()) forTabAtIndex:index];
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
			if (!pRect || !tabView || index < 0 || index >= static_cast<int>(tab.items.size()))
			{
				result = FALSE;
				return true;
			}

			NSRect tabRect = [tabView rectForTabAtIndex:index];
			pRect->left = static_cast<LONG>(tabRect.origin.x);
			pRect->top = static_cast<LONG>(tabRect.origin.y);
			pRect->right = static_cast<LONG>(tabRect.origin.x + tabRect.size.width);
			pRect->bottom = static_cast<LONG>(tabRect.origin.y + tabRect.size.height);
			result = TRUE;
			return true;
		}

		case TCM_ADJUSTRECT:
		{
			RECT* pRect = reinterpret_cast<RECT*>(lParam);
			if (pRect)
			{
				if (wParam)
					pRect->top -= 28;
				else
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
			if (!pHitTest || !tabView)
			{
				result = -1;
				return true;
			}
			NSPoint point = NSMakePoint(pHitTest->pt.x, pHitTest->pt.y);
			NSInteger hitIndex = [tabView tabIndexAtPoint:point];
			if (hitIndex >= 0)
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

// ============================================================
// Tab reorder helper
// ============================================================

void Win32TabControl_ReorderItem(void* hwndVoid, int fromIndex, int toIndex)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);
	auto it = s_tabControls.find(key);
	if (it == s_tabControls.end()) return;

	auto& tab = it->second;
	if (fromIndex < 0 || fromIndex >= static_cast<int>(tab.items.size()))
		return;
	if (toIndex < 0 || toIndex >= static_cast<int>(tab.items.size()))
		return;
	if (fromIndex == toIndex)
		return;

	TabItemData movedItem = tab.items[fromIndex];
	tab.items.erase(tab.items.begin() + fromIndex);
	tab.items.insert(tab.items.begin() + toIndex, movedItem);

	// Adjust currentSel to follow the moved tab
	if (tab.currentSel == fromIndex)
	{
		tab.currentSel = toIndex;
	}
	else
	{
		if (fromIndex < tab.currentSel && toIndex >= tab.currentSel)
			--tab.currentSel;
		else if (fromIndex > tab.currentSel && toIndex <= tab.currentSel)
			++tab.currentSel;
	}
}
