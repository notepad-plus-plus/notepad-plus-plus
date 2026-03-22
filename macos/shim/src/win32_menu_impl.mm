// Win32 Menu shim: NSMenu-backed menu API, accelerator stubs
// Extracted from win32_menu.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commdlg.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"

#include <vector>

// ============================================================
// Menu text helpers
// ============================================================

// Clean Win32 menu text: strip '&' accelerator markers and '\t' shortcut suffix
static NSString* cleanMenuText(const wchar_t* wstr)
{
	if (!wstr) return @"";
	NSString* raw = WideToNS(wstr);
	NSMutableString* result = [NSMutableString string];
	for (NSUInteger i = 0; i < raw.length; ++i)
	{
		unichar c = [raw characterAtIndex:i];
		if (c == '&')
		{
			if (i + 1 < raw.length && [raw characterAtIndex:i + 1] == '&')
			{
				[result appendString:@"&"];
				++i;
			}
			// else skip single &
		}
		else if (c == '\t')
		{
			break; // Keyboard shortcut text follows -- stop here
		}
		else
		{
			[result appendFormat:@"%C", c];
		}
	}
	return result;
}

// Extract keyboard shortcut text after '\t' in menu text
static NSString* extractShortcutText(const wchar_t* wstr)
{
	if (!wstr) return nil;
	NSString* raw = WideToNS(wstr);
	NSRange tabRange = [raw rangeOfString:@"\t"];
	if (tabRange.location != NSNotFound && tabRange.location + 1 < raw.length)
		return [raw substringFromIndex:tabRange.location + 1];
	return nil;
}

// Parse shortcut text like "Ctrl+O", "Ctrl+Shift+S", "F5" and set on NSMenuItem
static void setKeyEquivalentFromText(NSMenuItem* item, NSString* shortcutText)
{
	if (!shortcutText || shortcutText.length == 0) return;

	NSUInteger modifiers = 0;
	NSString* key = @"";

	NSArray<NSString*>* parts = [shortcutText componentsSeparatedByString:@"+"];
	for (NSString* part in parts)
	{
		NSString* p = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		if ([p caseInsensitiveCompare:@"Ctrl"] == NSOrderedSame)
			modifiers |= NSEventModifierFlagCommand; // Ctrl -> Cmd on macOS
		else if ([p caseInsensitiveCompare:@"Cmd"] == NSOrderedSame)
			modifiers |= NSEventModifierFlagControl; // Cmd -> Control on macOS
		else if ([p caseInsensitiveCompare:@"Alt"] == NSOrderedSame)
			modifiers |= NSEventModifierFlagOption;
		else if ([p caseInsensitiveCompare:@"Shift"] == NSOrderedSame)
			modifiers |= NSEventModifierFlagShift;
		else
		{
			// Map function keys and special keys
			if ([p caseInsensitiveCompare:@"F1"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF1FunctionKey];
			else if ([p caseInsensitiveCompare:@"F2"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF2FunctionKey];
			else if ([p caseInsensitiveCompare:@"F3"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF3FunctionKey];
			else if ([p caseInsensitiveCompare:@"F4"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF4FunctionKey];
			else if ([p caseInsensitiveCompare:@"F5"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF5FunctionKey];
			else if ([p caseInsensitiveCompare:@"F6"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF6FunctionKey];
			else if ([p caseInsensitiveCompare:@"F7"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF7FunctionKey];
			else if ([p caseInsensitiveCompare:@"F8"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF8FunctionKey];
			else if ([p caseInsensitiveCompare:@"F9"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF9FunctionKey];
			else if ([p caseInsensitiveCompare:@"F10"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF10FunctionKey];
			else if ([p caseInsensitiveCompare:@"F11"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF11FunctionKey];
			else if ([p caseInsensitiveCompare:@"F12"] == NSOrderedSame) key = [NSString stringWithFormat:@"%C", (unichar)NSF12FunctionKey];
			else if ([p caseInsensitiveCompare:@"Del"] == NSOrderedSame || [p caseInsensitiveCompare:@"Delete"] == NSOrderedSame)
				key = [NSString stringWithFormat:@"%C", (unichar)NSDeleteFunctionKey];
			else if ([p caseInsensitiveCompare:@"Enter"] == NSOrderedSame || [p caseInsensitiveCompare:@"Return"] == NSOrderedSame)
				key = @"\r";
			else if ([p caseInsensitiveCompare:@"Esc"] == NSOrderedSame || [p caseInsensitiveCompare:@"Escape"] == NSOrderedSame)
				key = [NSString stringWithFormat:@"%C", (unichar)27];
			else if ([p caseInsensitiveCompare:@"Tab"] == NSOrderedSame)
				key = @"\t";
			else if ([p caseInsensitiveCompare:@"Home"] == NSOrderedSame)
				key = [NSString stringWithFormat:@"%C", (unichar)NSHomeFunctionKey];
			else if ([p caseInsensitiveCompare:@"End"] == NSOrderedSame)
				key = [NSString stringWithFormat:@"%C", (unichar)NSEndFunctionKey];
			else if ([p caseInsensitiveCompare:@"PageUp"] == NSOrderedSame)
				key = [NSString stringWithFormat:@"%C", (unichar)NSPageUpFunctionKey];
			else if ([p caseInsensitiveCompare:@"PageDown"] == NSOrderedSame)
				key = [NSString stringWithFormat:@"%C", (unichar)NSPageDownFunctionKey];
			else if ([p caseInsensitiveCompare:@"Space"] == NSOrderedSame)
				key = @" ";
			else
				key = [p lowercaseString];
		}
	}

	if (key.length > 0)
	{
		[item setKeyEquivalent:key];
		[item setKeyEquivalentModifierMask:modifiers];
	}
}

// ============================================================
// Menu Target: dispatches NSMenuItem clicks as WM_COMMAND
// ============================================================

// Forward declaration
static HMENU findHmenuForNSMenu(NSMenu* menu);

@interface Win32MenuDelegate : NSObject <NSMenuDelegate>
+ (instancetype)shared;
@end

@implementation Win32MenuDelegate

+ (instancetype)shared
{
	static Win32MenuDelegate* instance = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		instance = [[Win32MenuDelegate alloc] init];
	});
	return instance;
}

- (void)menuWillOpen:(NSMenu*)menu
{
	HMENU hMenu = findHmenuForNSMenu(menu);
	if (!hMenu)
		return;

	HWND mainWnd = HandleRegistry::getMainWindow();
	if (!mainWnd)
		return;

	auto* info = HandleRegistry::getWindowInfo(mainWnd);
	if (info && info->wndProc)
		info->wndProc(mainWnd, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(hMenu), 0);
	else
		SendMessageW(mainWnd, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(hMenu), 0);
}

@end

@interface Win32MenuTarget : NSObject
+ (instancetype)shared;
- (void)menuItemClicked:(NSMenuItem*)sender;
@end

@implementation Win32MenuTarget

+ (instancetype)shared
{
	static Win32MenuTarget* instance = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		instance = [[Win32MenuTarget alloc] init];
	});
	return instance;
}

- (void)menuItemClicked:(NSMenuItem*)sender
{
	UINT cmdId = static_cast<UINT>([sender tag]);
	if (cmdId == 0) return;

	// Dispatch WM_COMMAND to the main window
	HWND mainWnd = HandleRegistry::getMainWindow();
	if (mainWnd)
	{
		auto* info = HandleRegistry::getWindowInfo(mainWnd);
		if (info && info->wndProc)
		{
			info->wndProc(mainWnd, WM_COMMAND, MAKEWPARAM(cmdId, 0), 0);
			return;
		}
	}

	// Fallback: try SendMessage
	if (mainWnd)
		SendMessageW(mainWnd, WM_COMMAND, MAKEWPARAM(cmdId, 0), 0);
}

@end

// ============================================================
// Menu Registry: HMENU -> NSMenu* mapping
// ============================================================

static NSMutableDictionary<NSNumber*, NSMenu*>* s_menuMap = nil;
static uintptr_t s_nextMenuHandle = 0x20000;

// Per-window menu mapping: HWND -> HMENU
static NSMutableDictionary<NSNumber*, NSNumber*>* s_windowMenuMap = nil;

static void ensureMenuMaps()
{
	if (!s_menuMap)
		s_menuMap = [NSMutableDictionary dictionary];
	if (!s_windowMenuMap)
		s_windowMenuMap = [NSMutableDictionary dictionary];
}

static HMENU allocHmenu(NSMenu* menu)
{
	ensureMenuMaps();
	HMENU h = reinterpret_cast<HMENU>(s_nextMenuHandle++);
	s_menuMap[@(reinterpret_cast<uintptr_t>(h))] = menu;
	return h;
}

static NSMenu* resolveMenu(HMENU h)
{
	ensureMenuMaps();
	if (!h) return nil;
	return s_menuMap[@(reinterpret_cast<uintptr_t>(h))];
}

static HMENU findHmenuForNSMenu(NSMenu* menu)
{
	ensureMenuMaps();
	if (!menu) return nullptr;
	for (NSNumber* key in s_menuMap)
	{
		if (s_menuMap[key] == menu)
			return reinterpret_cast<HMENU>([key unsignedLongValue]);
	}
	return nullptr;
}

// ============================================================
// Menu item helper: find item by command ID or position
// ============================================================

static NSMenuItem* findMenuItem(NSMenu* menu, UINT uItem, UINT uFlags)
{
	if (!menu) return nil;
	if (uFlags & MF_BYPOSITION)
	{
		if (static_cast<int>(uItem) < [menu numberOfItems])
			return [menu itemAtIndex:uItem];
		return nil;
	}
	// By command ID
	for (NSInteger i = 0; i < [menu numberOfItems]; ++i)
	{
		NSMenuItem* item = [menu itemAtIndex:i];
		if (static_cast<UINT>([item tag]) == uItem)
			return item;
	}
	return nil;
}

// Recursively find item by command ID in menu and all submenus
static NSMenuItem* findMenuItemRecursive(NSMenu* menu, UINT cmdId)
{
	if (!menu) return nil;
	for (NSInteger i = 0; i < [menu numberOfItems]; ++i)
	{
		NSMenuItem* item = [menu itemAtIndex:i];
		if (static_cast<UINT>([item tag]) == cmdId)
			return item;
		if ([item hasSubmenu])
		{
			NSMenuItem* found = findMenuItemRecursive([item submenu], cmdId);
			if (found) return found;
		}
	}
	return nil;
}

// ============================================================
// Menu API implementations
// ============================================================

HMENU CreateMenu()
{
	@autoreleasepool {
		NSMenu* menu = [[NSMenu alloc] init];
		[menu setAutoenablesItems:NO];
		menu.delegate = [Win32MenuDelegate shared];
		return allocHmenu(menu);
	}
}

HMENU CreatePopupMenu()
{
	return CreateMenu();
}

BOOL DestroyMenu(HMENU hMenu)
{
	ensureMenuMaps();
	NSNumber* key = @(reinterpret_cast<uintptr_t>(hMenu));
	if (s_menuMap[key])
	{
		[s_menuMap removeObjectForKey:key];
		return TRUE;
	}
	return TRUE;
}

HMENU LoadMenuW(HINSTANCE hInstance, LPCWSTR lpMenuName)
{
	// Can't load from resources on macOS. Return a valid empty menu
	// so the N++ code can populate it dynamically.
	return CreateMenu();
}

HMENU GetMenu(HWND hWnd)
{
	ensureMenuMaps();
	NSNumber* key = @(reinterpret_cast<uintptr_t>(hWnd));
	NSNumber* hmenuVal = s_windowMenuMap[key];
	if (hmenuVal)
		return reinterpret_cast<HMENU>([hmenuVal unsignedLongValue]);
	return nullptr;
}

BOOL SetMenu(HWND hWnd, HMENU hMenu)
{
	ensureMenuMaps();
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	// Store the window -> menu mapping
	s_windowMenuMap[@(reinterpret_cast<uintptr_t>(hWnd))] = @(reinterpret_cast<uintptr_t>(hMenu));

	// On macOS, we need to wrap the menu items under a main menu with an app menu
	NSMenu* mainMenu = [[NSMenu alloc] init];
	[mainMenu setAutoenablesItems:NO];
	mainMenu.delegate = [Win32MenuDelegate shared];

	// Add application menu (About, Quit)
	NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
	NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"Notepad++"];
	appMenu.delegate = [Win32MenuDelegate shared];
	[appMenu addItemWithTitle:@"About Notepad++" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
	[appMenu addItem:[NSMenuItem separatorItem]];
	NSMenuItem* quitItem = [appMenu addItemWithTitle:@"Quit Notepad++" action:@selector(terminate:) keyEquivalent:@"q"];
	(void)quitItem;
	appMenuItem.submenu = appMenu;
	[mainMenu addItem:appMenuItem];

	// Add all items from the Win32 menu
	// Each top-level item in hMenu becomes a top-level menu item
	while ([menu numberOfItems] > 0)
	{
		NSMenuItem* item = [menu itemAtIndex:0];
		[menu removeItemAtIndex:0];
		[mainMenu addItem:item];
	}

	[NSApp setMainMenu:mainMenu];

	// Update the menu registry so hMenu now resolves to mainMenu
	// (the original NSMenu is empty after moving items)
	s_menuMap[@(reinterpret_cast<uintptr_t>(hMenu))] = mainMenu;

	return TRUE;
}

HMENU GetSubMenu(HMENU hMenu, int nPos)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu || nPos < 0 || nPos >= [menu numberOfItems])
		return nullptr;

	NSMenuItem* item = [menu itemAtIndex:nPos];
	if ([item hasSubmenu])
	{
		NSMenu* submenu = [item submenu];
		// Find or create HMENU for this submenu
		HMENU h = findHmenuForNSMenu(submenu);
		if (!h)
			h = allocHmenu(submenu);
		return h;
	}
	return nullptr;
}

HMENU GetSystemMenu(HWND hWnd, BOOL bRevert)
{
	return nullptr;
}

// ============================================================
// Menu item insertion
// ============================================================

static NSMenuItem* createMenuItemFromFlags(UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	@autoreleasepool {
		if (uFlags & MF_SEPARATOR)
			return [NSMenuItem separatorItem];

		NSString* title = cleanMenuText(lpNewItem);
		NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
		                                             action:@selector(menuItemClicked:)
		                                      keyEquivalent:@""];
		item.target = [Win32MenuTarget shared];

		if (uFlags & MF_POPUP)
		{
			// uIDNewItem is HMENU of submenu
			HMENU subHmenu = reinterpret_cast<HMENU>(uIDNewItem);
			NSMenu* submenu = resolveMenu(subHmenu);
			if (submenu)
			{
				[submenu setTitle:title];
				item.submenu = submenu;
			}
			item.action = nil;
			item.target = nil;
			item.tag = 0;
		}
		else
		{
			item.tag = static_cast<NSInteger>(uIDNewItem);
		}

		// Parse keyboard shortcut from menu text
		NSString* shortcut = extractShortcutText(lpNewItem);
		if (shortcut)
			setKeyEquivalentFromText(item, shortcut);

		if (uFlags & MF_CHECKED)
			item.state = NSControlStateValueOn;

		if (uFlags & (MF_DISABLED | MF_GRAYED))
			item.enabled = NO;

		return item;
	}
}

BOOL AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	NSMenuItem* item = createMenuItemFromFlags(uFlags, uIDNewItem, lpNewItem);
	if (!item) return FALSE;

	[menu addItem:item];
	return TRUE;
}

BOOL InsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	NSMenuItem* item = createMenuItemFromFlags(uFlags, uIDNewItem, lpNewItem);
	if (!item) return FALSE;

	if (uFlags & MF_BYPOSITION)
	{
		NSInteger pos = static_cast<NSInteger>(uPosition);
		if (pos > [menu numberOfItems]) pos = [menu numberOfItems];
		[menu insertItem:item atIndex:pos];
	}
	else
	{
		// By command: insert before the item with this command ID
		NSInteger idx = -1;
		for (NSInteger i = 0; i < [menu numberOfItems]; ++i)
		{
			if (static_cast<UINT>([[menu itemAtIndex:i] tag]) == uPosition)
			{
				idx = i;
				break;
			}
		}
		if (idx >= 0)
			[menu insertItem:item atIndex:idx];
		else
			[menu addItem:item]; // fallback: append
	}
	return TRUE;
}

BOOL InsertMenuItemW(HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmi)
{
	if (!lpmi) return FALSE;
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	@autoreleasepool {
		NSMenuItem* menuItem = nil;

		// Determine type
		bool isSeparator = false;
		if (lpmi->fMask & MIIM_FTYPE)
			isSeparator = (lpmi->fType & MFT_SEPARATOR) != 0;
		else if (lpmi->fMask & MIIM_TYPE)
			isSeparator = (lpmi->fType & MFT_SEPARATOR) != 0;

		if (isSeparator)
		{
			menuItem = [NSMenuItem separatorItem];
		}
		else
		{
			NSString* title = @"";
			if ((lpmi->fMask & MIIM_STRING) || (lpmi->fMask & MIIM_TYPE))
			{
				if (lpmi->dwTypeData)
					title = cleanMenuText(lpmi->dwTypeData);
			}

			menuItem = [[NSMenuItem alloc] initWithTitle:title
			                                      action:@selector(menuItemClicked:)
			                               keyEquivalent:@""];
			menuItem.target = [Win32MenuTarget shared];

			// Command ID
			if (lpmi->fMask & MIIM_ID)
				menuItem.tag = static_cast<NSInteger>(lpmi->wID);

			// Submenu
			if (lpmi->fMask & MIIM_SUBMENU)
			{
				NSMenu* submenu = resolveMenu(lpmi->hSubMenu);
				if (submenu)
				{
					[submenu setTitle:title];
					menuItem.submenu = submenu;
					menuItem.action = nil;
					menuItem.target = nil;
				}
			}

			// State
			if (lpmi->fMask & MIIM_STATE)
			{
				if (lpmi->fState & MFS_CHECKED)
					menuItem.state = NSControlStateValueOn;
				if (lpmi->fState & MFS_DISABLED)
					menuItem.enabled = NO;
			}

			// Parse shortcut from text
			if (lpmi->dwTypeData)
			{
				NSString* shortcut = extractShortcutText(lpmi->dwTypeData);
				if (shortcut)
					setKeyEquivalentFromText(menuItem, shortcut);
			}

			// Item data
			if (lpmi->fMask & MIIM_DATA)
				menuItem.representedObject = @(lpmi->dwItemData);
		}

		// Insert at position
		if (fByPosition)
		{
			NSInteger pos = static_cast<NSInteger>(item);
			if (pos > [menu numberOfItems]) pos = [menu numberOfItems];
			[menu insertItem:menuItem atIndex:pos];
		}
		else
		{
			// Insert before the item with this command ID
			NSInteger idx = -1;
			for (NSInteger i = 0; i < [menu numberOfItems]; ++i)
			{
				if (static_cast<UINT>([[menu itemAtIndex:i] tag]) == item)
				{
					idx = i;
					break;
				}
			}
			if (idx >= 0)
				[menu insertItem:menuItem atIndex:idx];
			else
				[menu addItem:menuItem];
		}

		return TRUE;
	}
}

BOOL SetMenuItemInfoW(HMENU hMenu, UINT uItem, BOOL fByPosition, LPCMENUITEMINFOW lpmii)
{
	if (!lpmii) return FALSE;
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	NSMenuItem* menuItem = findMenuItem(menu, uItem, fByPosition ? MF_BYPOSITION : MF_BYCOMMAND);
	if (!menuItem) return FALSE;

	@autoreleasepool {
		if ((lpmii->fMask & MIIM_STRING) || (lpmii->fMask & MIIM_TYPE))
		{
			if (lpmii->dwTypeData)
			{
				menuItem.title = cleanMenuText(lpmii->dwTypeData);
				NSString* shortcut = extractShortcutText(lpmii->dwTypeData);
				if (shortcut)
					setKeyEquivalentFromText(menuItem, shortcut);
			}
		}

		if (lpmii->fMask & MIIM_ID)
			menuItem.tag = static_cast<NSInteger>(lpmii->wID);

		if (lpmii->fMask & MIIM_STATE)
		{
			menuItem.state = (lpmii->fState & MFS_CHECKED) ? NSControlStateValueOn : NSControlStateValueOff;
			menuItem.enabled = !(lpmii->fState & MFS_DISABLED);
		}

		if (lpmii->fMask & MIIM_SUBMENU)
		{
			NSMenu* submenu = resolveMenu(lpmii->hSubMenu);
			menuItem.submenu = submenu;
			if (submenu)
			{
				menuItem.action = nil;
				menuItem.target = nil;
			}
		}

		if (lpmii->fMask & MIIM_DATA)
			menuItem.representedObject = @(lpmii->dwItemData);

		return TRUE;
	}
}

BOOL GetMenuItemInfoW(HMENU hMenu, UINT uItem, BOOL fByPosition, LPMENUITEMINFOW lpmii)
{
	if (!lpmii) return FALSE;
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	NSMenuItem* menuItem = findMenuItem(menu, uItem, fByPosition ? MF_BYPOSITION : MF_BYCOMMAND);
	if (!menuItem) return FALSE;

	if (lpmii->fMask & MIIM_ID)
		lpmii->wID = static_cast<UINT>([menuItem tag]);

	if (lpmii->fMask & MIIM_STATE)
	{
		lpmii->fState = 0;
		if (menuItem.state == NSControlStateValueOn)
			lpmii->fState |= MFS_CHECKED;
		if (!menuItem.enabled)
			lpmii->fState |= MFS_DISABLED;
	}

	if (lpmii->fMask & MIIM_SUBMENU)
	{
		if ([menuItem hasSubmenu])
			lpmii->hSubMenu = findHmenuForNSMenu([menuItem submenu]);
		else
			lpmii->hSubMenu = nullptr;
	}

	if ((lpmii->fMask & MIIM_STRING) || (lpmii->fMask & MIIM_TYPE))
	{
		if (lpmii->dwTypeData && lpmii->cch > 0)
		{
			NSStringToWide(menuItem.title, lpmii->dwTypeData, lpmii->cch);
			lpmii->cch = static_cast<UINT>(menuItem.title.length);
		}
		else
		{
			lpmii->cch = static_cast<UINT>(menuItem.title.length);
		}
	}

	if (lpmii->fMask & MIIM_FTYPE)
	{
		lpmii->fType = menuItem.isSeparatorItem ? MFT_SEPARATOR : MFT_STRING;
	}

	if (lpmii->fMask & MIIM_DATA)
	{
		if ([menuItem.representedObject isKindOfClass:[NSNumber class]])
			lpmii->dwItemData = [(NSNumber*)menuItem.representedObject unsignedLongValue];
		else
			lpmii->dwItemData = 0;
	}

	return TRUE;
}

BOOL RemoveMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	NSMenuItem* item = findMenuItem(menu, uPosition, uFlags);
	if (!item) return FALSE;

	[menu removeItem:item];
	return TRUE;
}

BOOL DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
	// DeleteMenu is like RemoveMenu but also destroys submenus
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	NSMenuItem* item = findMenuItem(menu, uPosition, uFlags);
	if (!item) return FALSE;

	if ([item hasSubmenu])
	{
		HMENU subH = findHmenuForNSMenu([item submenu]);
		if (subH) DestroyMenu(subH);
	}

	[menu removeItem:item];
	return TRUE;
}

BOOL EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return static_cast<BOOL>(-1);

	// Check both the direct menu and recursively
	NSMenuItem* item = nullptr;
	if (uEnable & MF_BYPOSITION)
		item = findMenuItem(menu, uIDEnableItem, MF_BYPOSITION);
	else
		item = findMenuItemRecursive(menu, uIDEnableItem);

	if (!item) return static_cast<BOOL>(-1);

	BOOL wasEnabled = item.enabled;
	BOOL enable = !((uEnable & MF_DISABLED) || (uEnable & MF_GRAYED));
	item.enabled = enable;

	return wasEnabled ? MF_ENABLED : MF_GRAYED;
}

BOOL CheckMenuItem(HMENU hMenu, UINT uIDCheckItem, UINT uCheck)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return static_cast<BOOL>(-1);

	NSMenuItem* item = nullptr;
	if (uCheck & MF_BYPOSITION)
		item = findMenuItem(menu, uIDCheckItem, MF_BYPOSITION);
	else
		item = findMenuItemRecursive(menu, uIDCheckItem);

	if (!item) return static_cast<BOOL>(-1);

	DWORD prevState = (item.state == NSControlStateValueOn) ? MF_CHECKED : MF_UNCHECKED;
	item.state = (uCheck & MF_CHECKED) ? NSControlStateValueOn : NSControlStateValueOff;
	return prevState;
}

BOOL CheckMenuRadioItem(HMENU hMenu, UINT first, UINT last, UINT check, UINT flags)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	for (UINT id = first; id <= last; ++id)
	{
		NSMenuItem* item = findMenuItem(menu, id, flags);
		if (item)
			item.state = (id == check) ? NSControlStateValueOn : NSControlStateValueOff;
	}
	return TRUE;
}

int GetMenuItemCount(HMENU hMenu)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return -1;
	return static_cast<int>([menu numberOfItems]);
}

UINT GetMenuItemID(HMENU hMenu, int nPos)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu || nPos < 0 || nPos >= [menu numberOfItems])
		return static_cast<UINT>(-1);

	NSMenuItem* item = [menu itemAtIndex:nPos];
	if ([item hasSubmenu])
		return static_cast<UINT>(-1);
	return static_cast<UINT>([item tag]);
}

BOOL TrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, const RECT* prcRect)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	@autoreleasepool {
		// Convert screen coordinates to Cocoa coordinates
		NSRect screenFrame = [[NSScreen mainScreen] frame];
		NSPoint point = NSMakePoint(x, screenFrame.size.height - y);

		// Find the NSView for the given HWND
		NSView* view = nil;
		auto* info = HandleRegistry::getWindowInfo(hWnd);
		if (info && info->nativeView)
			view = (__bridge NSView*)info->nativeView;

		if (view)
		{
			// Convert screen point to view coordinates
			NSWindow* window = [view window];
			if (window)
			{
				NSPoint windowPt = [window convertPointFromScreen:point];
				NSPoint viewPt = [view convertPoint:windowPt fromView:nil];
				[menu popUpMenuPositioningItem:nil atLocation:viewPt inView:view];
			}
			else
			{
				[menu popUpMenuPositioningItem:nil atLocation:point inView:nil];
			}
		}
		else
		{
			[menu popUpMenuPositioningItem:nil atLocation:point inView:nil];
		}
	}
	return TRUE;
}

BOOL TrackPopupMenuEx(HMENU hMenu, UINT fuFlags, int x, int y, HWND hwnd, LPTPMPARAMS lptpm)
{
	return TrackPopupMenu(hMenu, fuFlags, x, y, 0, hwnd, nullptr);
}

BOOL DrawMenuBar(HWND hWnd)
{
	// On macOS, the menu bar auto-updates
	return TRUE;
}

int GetMenuStringW(HMENU hMenu, UINT uIDItem, LPWSTR lpString, int cchMax, UINT flags)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu)
	{
		if (lpString && cchMax > 0) lpString[0] = L'\0';
		return 0;
	}

	NSMenuItem* item = findMenuItem(menu, uIDItem, flags);
	if (!item)
	{
		if (lpString && cchMax > 0) lpString[0] = L'\0';
		return 0;
	}

	if (lpString && cchMax > 0)
	{
		NSStringToWide(item.title, lpString, static_cast<size_t>(cchMax));
		return static_cast<int>(wcslen(lpString));
	}
	return static_cast<int>(item.title.length);
}

UINT GetMenuState(HMENU hMenu, UINT uId, UINT uFlags)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return static_cast<UINT>(-1);

	NSMenuItem* item = findMenuItem(menu, uId, uFlags);
	if (!item) return static_cast<UINT>(-1);

	UINT state = 0;
	if (item.state == NSControlStateValueOn) state |= MF_CHECKED;
	if (!item.enabled) state |= MF_GRAYED;
	if (item.isSeparatorItem) state |= MF_SEPARATOR;
	if ([item hasSubmenu])
		state |= MF_POPUP;
	return state;
}

BOOL ModifyMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	NSMenu* menu = resolveMenu(hMenu);
	if (!menu) return FALSE;

	// Find the existing item
	NSMenuItem* oldItem = findMenuItem(menu, uPosition, uFlags);
	if (!oldItem) return FALSE;

	NSInteger idx = [menu indexOfItem:oldItem];
	if (idx == -1) return FALSE;

	// Remove old and insert new at same position
	[menu removeItemAtIndex:idx];

	NSMenuItem* newItem = createMenuItemFromFlags(uFlags, uIDNewItem, lpNewItem);
	if (!newItem) return FALSE;

	[menu insertItem:newItem atIndex:idx];
	return TRUE;
}

// ============================================================
// Accelerators
// ============================================================

struct AccelTableData
{
	std::vector<ACCEL> entries;
};

HACCEL LoadAcceleratorsW(HINSTANCE hInstance, LPCWSTR lpTableName)
{
	// Can't load from resources; return nullptr
	return nullptr;
}

int TranslateAcceleratorW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg)
{
	// On macOS, accelerators are handled by NSMenuItem keyEquivalents
	// and the Cocoa event system. This is a no-op.
	return 0;
}

HACCEL CreateAcceleratorTableW(LPACCEL paccel, int cAccel)
{
	if (!paccel || cAccel <= 0) return nullptr;

	auto* table = new AccelTableData();
	table->entries.assign(paccel, paccel + cAccel);
	return reinterpret_cast<HACCEL>(table);
}

BOOL DestroyAcceleratorTable(HACCEL hAccel)
{
	if (hAccel)
	{
		auto* table = reinterpret_cast<AccelTableData*>(hAccel);
		delete table;
	}
	return TRUE;
}
