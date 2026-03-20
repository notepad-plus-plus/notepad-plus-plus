// Win32 Shim: Menu, Dialog, Accelerator, Clipboard, Shell, Theme implementations for macOS
// Phase 2: Real NSMenu backing for menu APIs, NSOpenPanel/NSSavePanel for file dialogs.

#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <vector>
#include "windows.h"
#include "commdlg.h"
#include "shellapi.h"
#include "uxtheme.h"
#include "handle_registry.h"

// ============================================================
// String helper: wchar_t* (UTF-32 on macOS) → NSString*
// ============================================================
static NSString* WideToNSString(const wchar_t* wstr)
{
	if (!wstr) return @"";
	size_t len = wcslen(wstr);
	NSString* str = [[NSString alloc] initWithBytes:wstr
	                                         length:len * sizeof(wchar_t)
	                                       encoding:NSUTF32LittleEndianStringEncoding];
	return str ?: @"";
}

// Convert NSString to wchar_t buffer (UTF-32LE on macOS)
static void NSStringToWide(NSString* str, wchar_t* buffer, size_t maxChars)
{
	if (!buffer || maxChars == 0) return;
	if (!str || str.length == 0)
	{
		buffer[0] = L'\0';
		return;
	}
	NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
	size_t charCount = data.length / sizeof(wchar_t);
	size_t copyLen = (charCount < maxChars - 1) ? charCount : maxChars - 1;
	memcpy(buffer, data.bytes, copyLen * sizeof(wchar_t));
	buffer[copyLen] = L'\0';
}

// Clean Win32 menu text: strip '&' accelerator markers and '\t' shortcut suffix
static NSString* cleanMenuText(const wchar_t* wstr)
{
	if (!wstr) return @"";
	NSString* raw = WideToNSString(wstr);
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
			break; // Keyboard shortcut text follows — stop here
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
	NSString* raw = WideToNSString(wstr);
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
			modifiers |= NSEventModifierFlagCommand; // Ctrl → Cmd on macOS
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
// Menu Registry: HMENU → NSMenu* mapping
// ============================================================

static NSMutableDictionary<NSNumber*, NSMenu*>* s_menuMap = nil;
static uintptr_t s_nextMenuHandle = 0x20000;

// Per-window menu mapping: HWND → HMENU
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

	// Store the window → menu mapping
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

// ============================================================
// Clipboard
// ============================================================

BOOL OpenClipboard(HWND hWndNewOwner) { return TRUE; }
BOOL CloseClipboard() { return TRUE; }

BOOL EmptyClipboard()
{
	@autoreleasepool {
		[[NSPasteboard generalPasteboard] clearContents];
	}
	return TRUE;
}

HANDLE SetClipboardData(UINT uFormat, HANDLE hMem)
{
	if (!hMem) return nullptr;
	@autoreleasepool {
		if (uFormat == CF_UNICODETEXT)
		{
			const wchar_t* wstr = static_cast<const wchar_t*>(hMem);
			NSString* str = WideToNSString(wstr);
			[[NSPasteboard generalPasteboard] clearContents];
			[[NSPasteboard generalPasteboard] setString:str forType:NSPasteboardTypeString];
		}
		else if (uFormat == CF_TEXT)
		{
			const char* cstr = static_cast<const char*>(hMem);
			NSString* str = [NSString stringWithUTF8String:cstr];
			[[NSPasteboard generalPasteboard] clearContents];
			[[NSPasteboard generalPasteboard] setString:str forType:NSPasteboardTypeString];
		}
	}
	return hMem;
}

HANDLE GetClipboardData(UINT uFormat)
{
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		NSString* str = [pb stringForType:NSPasteboardTypeString];
		if (!str) return nullptr;

		if (uFormat == CF_UNICODETEXT)
		{
			// Return a wide string buffer (caller is responsible for GlobalFree)
			NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
			size_t bufSize = data.length + sizeof(wchar_t); // +null terminator
			void* buf = GlobalAlloc(GMEM_FIXED, bufSize);
			memcpy(buf, data.bytes, data.length);
			memset(static_cast<char*>(buf) + data.length, 0, sizeof(wchar_t));
			return buf;
		}
		else if (uFormat == CF_TEXT)
		{
			const char* utf8 = [str UTF8String];
			size_t len = strlen(utf8) + 1;
			void* buf = GlobalAlloc(GMEM_FIXED, len);
			memcpy(buf, utf8, len);
			return buf;
		}
	}
	return nullptr;
}

BOOL IsClipboardFormatAvailable(UINT format)
{
	@autoreleasepool {
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		if (format == CF_UNICODETEXT || format == CF_TEXT)
			return [pb stringForType:NSPasteboardTypeString] != nil;
	}
	return FALSE;
}

int CountClipboardFormats()
{
	@autoreleasepool {
		return static_cast<int>([[[NSPasteboard generalPasteboard] types] count]);
	}
}

UINT EnumClipboardFormats(UINT format) { return 0; }
HWND GetClipboardOwner() { return nullptr; }

// ============================================================
// Shell API
// ============================================================

HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile,
                        LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	@autoreleasepool {
		NSString* filePath = WideToNSString(lpFile);
		if (filePath.length > 0)
		{
			NSURL* url = nil;
			if ([filePath hasPrefix:@"http://"] || [filePath hasPrefix:@"https://"] ||
			    [filePath hasPrefix:@"ftp://"] || [filePath hasPrefix:@"mailto:"])
			{
				url = [NSURL URLWithString:filePath];
			}
			else
			{
				url = [NSURL fileURLWithPath:filePath];
			}
			if (url)
				[[NSWorkspace sharedWorkspace] openURL:url];
		}
	}
	return reinterpret_cast<HINSTANCE>(33); // > 32 means success
}

// Overload for filesystem::path::c_str() which returns const char* on macOS
HINSTANCE ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, const char* lpFile,
                        LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	@autoreleasepool {
		if (lpFile)
		{
			NSString* filePath = [NSString stringWithUTF8String:lpFile];
			NSURL* url = nil;
			if ([filePath hasPrefix:@"http://"] || [filePath hasPrefix:@"https://"])
				url = [NSURL URLWithString:filePath];
			else
				url = [NSURL fileURLWithPath:filePath];
			if (url)
				[[NSWorkspace sharedWorkspace] openURL:url];
		}
	}
	return reinterpret_cast<HINSTANCE>(33);
}

BOOL ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo) { return TRUE; }

UINT DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch) { return 0; }
BOOL DragQueryPoint(HDROP hDrop, LPPOINT lppt) { return FALSE; }
void DragAcceptFiles(HWND hWnd, BOOL fAccept) {}
void DragFinish(HDROP hDrop) {}

BOOL Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW lpData) { return TRUE; }

HICON ExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex) { return nullptr; }

// ============================================================
// Common Dialogs: File Open/Save
// ============================================================

// Parse Win32 filter string: "Desc\0*.ext;*.ext2\0Desc2\0*.ext3\0\0"
static NSArray<NSArray<NSString*>*>* parseFilterPairs(const wchar_t* filter)
{
	NSMutableArray<NSArray<NSString*>*>* pairs = [NSMutableArray array];
	if (!filter) return pairs;

	const wchar_t* p = filter;
	while (*p)
	{
		NSString* desc = WideToNSString(p);
		p += wcslen(p) + 1;
		if (!*p) break;
		NSString* patterns = WideToNSString(p);
		p += wcslen(p) + 1;
		[pairs addObject:@[desc, patterns]];
	}
	return pairs;
}

// Extract file extensions from pattern string like "*.cpp;*.h;*.hpp"
static NSArray<NSString*>* extractExtensions(NSString* patterns)
{
	NSMutableArray<NSString*>* exts = [NSMutableArray array];
	NSArray<NSString*>* parts = [patterns componentsSeparatedByString:@";"];
	for (NSString* part in parts)
	{
		NSString* p = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		if ([p isEqualToString:@"*.*"])
			return @[]; // All files
		if ([p hasPrefix:@"*."])
		{
			NSString* ext = [p substringFromIndex:2];
			if (ext.length > 0)
				[exts addObject:ext];
		}
	}
	return exts;
}

BOOL GetOpenFileNameW(LPOPENFILENAMEW lpofn)
{
	if (!lpofn) return FALSE;

	@autoreleasepool {
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		[panel setCanChooseFiles:YES];
		[panel setCanChooseDirectories:NO];
		[panel setAllowsMultipleSelection:(lpofn->Flags & OFN_ALLOWMULTISELECT) != 0];

		// Title
		if (lpofn->lpstrTitle)
			[panel setTitle:WideToNSString(lpofn->lpstrTitle)];

		// Initial directory
		if (lpofn->lpstrInitialDir)
			[panel setDirectoryURL:[NSURL fileURLWithPath:WideToNSString(lpofn->lpstrInitialDir)]];

		// File type filter
		NSArray<NSArray<NSString*>*>* filterPairs = parseFilterPairs(lpofn->lpstrFilter);
		if (filterPairs.count > 0)
		{
			// Use the filter at nFilterIndex (1-based) or first filter
			NSUInteger filterIdx = (lpofn->nFilterIndex > 0) ? lpofn->nFilterIndex - 1 : 0;
			if (filterIdx < filterPairs.count)
			{
				NSArray<NSString*>* exts = extractExtensions(filterPairs[filterIdx][1]);
				if (exts.count > 0)
				{
					NSMutableArray<UTType*>* types = [NSMutableArray array];
					for (NSString* ext in exts)
					{
						UTType* type = [UTType typeWithFilenameExtension:ext];
						if (type) [types addObject:type];
					}
					if (types.count > 0)
						[panel setAllowedContentTypes:types];
				}
			}
		}

		// Default extension
		if (lpofn->lpstrDefExt)
		{
			NSString* defExt = WideToNSString(lpofn->lpstrDefExt);
			if (defExt.length > 0)
			{
				UTType* defType = [UTType typeWithFilenameExtension:defExt];
				if (defType && panel.allowedContentTypes.count == 0)
					[panel setAllowedContentTypes:@[defType]];
			}
		}

		NSModalResponse response = [panel runModal];
		if (response != NSModalResponseOK)
			return FALSE;

		NSArray<NSURL*>* urls = panel.URLs;
		if (!urls || urls.count == 0) return FALSE;

		if (lpofn->lpstrFile && lpofn->nMaxFile > 0)
		{
			if (urls.count == 1)
			{
				// Single file: standard behavior
				NSString* path = [urls[0] path];
				NSStringToWide(path, lpofn->lpstrFile, lpofn->nMaxFile);

				NSString* fileName = [path lastPathComponent];
				NSString* dirPath = [path stringByDeletingLastPathComponent];
				lpofn->nFileOffset = static_cast<WORD>(dirPath.length + 1);
				NSRange dotRange = [fileName rangeOfString:@"." options:NSBackwardsSearch];
				if (dotRange.location != NSNotFound)
					lpofn->nFileExtension = static_cast<WORD>(lpofn->nFileOffset + dotRange.location + 1);
				else
					lpofn->nFileExtension = 0;
			}
			else
			{
				// Multiple files: null-delimited full paths, double-null terminated
				wchar_t* buf = lpofn->lpstrFile;
				DWORD remaining = lpofn->nMaxFile;

				for (NSURL* fileUrl in urls)
				{
					NSString* path = [fileUrl path];
					NSData* data = [path dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
					size_t charCount = data.length / sizeof(wchar_t);

					if (charCount + 2 > remaining)
					{
						NSLog(@"Warning: multi-select buffer overflow, %lu files could not be included",
						      (unsigned long)(urls.count));
						break;
					}

					memcpy(buf, data.bytes, charCount * sizeof(wchar_t));
					buf[charCount] = L'\0';
					buf += charCount + 1;
					remaining -= (charCount + 1);
				}
				*buf = L'\0'; // Double-null terminator

				lpofn->Flags |= OFN_ALLOWMULTISELECT;
				lpofn->nFileOffset = 0;
				lpofn->nFileExtension = 0;
			}
		}

		if (urls.count == 1 && lpofn->lpstrFileTitle && lpofn->nMaxFileTitle > 0)
		{
			NSString* fileName = [[urls[0] path] lastPathComponent];
			NSStringToWide(fileName, lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);
		}

		return TRUE;
	}
}

BOOL GetSaveFileNameW(LPOPENFILENAMEW lpofn)
{
	if (!lpofn) return FALSE;

	@autoreleasepool {
		NSSavePanel* panel = [NSSavePanel savePanel];

		// Title
		if (lpofn->lpstrTitle)
			[panel setTitle:WideToNSString(lpofn->lpstrTitle)];

		// Initial directory
		if (lpofn->lpstrInitialDir)
			[panel setDirectoryURL:[NSURL fileURLWithPath:WideToNSString(lpofn->lpstrInitialDir)]];

		// Default filename
		if (lpofn->lpstrFile && lpofn->lpstrFile[0] != L'\0')
		{
			NSString* path = WideToNSString(lpofn->lpstrFile);
			NSString* fileName = [path lastPathComponent];
			if (fileName.length > 0)
				[panel setNameFieldStringValue:fileName];
		}

		// Overwrite prompt
		if (!(lpofn->Flags & OFN_OVERWRITEPROMPT))
			[panel setCanCreateDirectories:YES];

		// File type filter
		NSArray<NSArray<NSString*>*>* filterPairs = parseFilterPairs(lpofn->lpstrFilter);
		if (filterPairs.count > 0)
		{
			NSUInteger filterIdx = (lpofn->nFilterIndex > 0) ? lpofn->nFilterIndex - 1 : 0;
			if (filterIdx < filterPairs.count)
			{
				NSArray<NSString*>* exts = extractExtensions(filterPairs[filterIdx][1]);
				if (exts.count > 0)
				{
					NSMutableArray<UTType*>* types = [NSMutableArray array];
					for (NSString* ext in exts)
					{
						UTType* type = [UTType typeWithFilenameExtension:ext];
						if (type) [types addObject:type];
					}
					if (types.count > 0)
						[panel setAllowedContentTypes:types];
				}
			}
		}

		// Default extension
		if (lpofn->lpstrDefExt)
		{
			NSString* defExt = WideToNSString(lpofn->lpstrDefExt);
			if (defExt.length > 0)
			{
				UTType* defType = [UTType typeWithFilenameExtension:defExt];
				if (defType && panel.allowedContentTypes.count == 0)
					[panel setAllowedContentTypes:@[defType]];
			}
		}

		NSModalResponse response = [panel runModal];
		if (response != NSModalResponseOK)
			return FALSE;

		NSURL* url = panel.URL;
		if (!url) return FALSE;

		NSString* path = [url path];
		if (lpofn->lpstrFile && lpofn->nMaxFile > 0)
		{
			NSStringToWide(path, lpofn->lpstrFile, lpofn->nMaxFile);

			NSString* fileName = [path lastPathComponent];
			NSString* dirPath = [path stringByDeletingLastPathComponent];
			lpofn->nFileOffset = static_cast<WORD>(dirPath.length + 1);
			NSRange dotRange = [fileName rangeOfString:@"." options:NSBackwardsSearch];
			if (dotRange.location != NSNotFound)
				lpofn->nFileExtension = static_cast<WORD>(lpofn->nFileOffset + dotRange.location + 1);
			else
				lpofn->nFileExtension = 0;
		}

		if (lpofn->lpstrFileTitle && lpofn->nMaxFileTitle > 0)
		{
			NSString* fileName = [path lastPathComponent];
			NSStringToWide(fileName, lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);
		}

		return TRUE;
	}
}

BOOL ChooseColorW(LPCHOOSECOLORW lpcc)
{
	if (!lpcc) return FALSE;

	@autoreleasepool {
		NSColorPanel* panel = [NSColorPanel sharedColorPanel];

		// Set initial color if CC_RGBINIT
		if (lpcc->Flags & CC_RGBINIT)
		{
			COLORREF cr = lpcc->rgbResult;
			CGFloat r = (cr & 0xFF) / 255.0;
			CGFloat g = ((cr >> 8) & 0xFF) / 255.0;
			CGFloat b = ((cr >> 16) & 0xFF) / 255.0;
			[panel setColor:[NSColor colorWithRed:r green:g blue:b alpha:1.0]];
		}

		[panel setIsVisible:YES];
		[panel makeKeyAndOrderFront:nil];

		// Run a modal session to wait for user to close the panel
		// For simplicity, just show the panel non-modally and return current color
		NSColor* color = [[panel color] colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
		if (color)
		{
			int r = static_cast<int>([color redComponent] * 255);
			int g = static_cast<int>([color greenComponent] * 255);
			int b = static_cast<int>([color blueComponent] * 255);
			lpcc->rgbResult = RGB(r, g, b);
		}
		return TRUE;
	}
}

BOOL ChooseFontW(LPCHOOSEFONTW lpcf) { return FALSE; }
BOOL PrintDlgW(LPPRINTDLGW lppd) { return FALSE; }
HWND FindTextW(LPFINDREPLACEW lpfr) { return nullptr; }
HWND ReplaceTextW(LPFINDREPLACEW lpfr) { return nullptr; }
DWORD CommDlgExtendedError() { return 0; }

// ============================================================
// Theme stubs
// ============================================================
HTHEME OpenThemeData(HWND hwnd, LPCWSTR pszClassList) { return nullptr; }
HRESULT CloseThemeData(HTHEME hTheme) { return S_OK; }
HRESULT DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pRect, const RECT* pClipRect) { return E_NOTIMPL; }
HRESULT DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT* pRect) { return E_NOTIMPL; }
HRESULT GetThemeColor(HTHEME hTheme, int iPartId, int iStateId, int iPropId, COLORREF* pColor) { return E_NOTIMPL; }
HRESULT GetThemePartSize(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* prc, int eSize, SIZE* psz) { return E_NOTIMPL; }
HRESULT GetThemeBackgroundContentRect(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pBoundingRect, RECT* pContentRect) { return E_NOTIMPL; }
BOOL IsThemeActive() { return FALSE; }
BOOL IsAppThemed() { return FALSE; }
HRESULT SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList) { return S_OK; }
HRESULT EnableThemeDialogTexture(HWND hwnd, DWORD dwFlags) { return S_OK; }
HRESULT GetThemeMetric(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, int* piVal) { return E_NOTIMPL; }
HRESULT GetThemeFont(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, LOGFONTW* pFont) { return E_NOTIMPL; }

HRESULT DwmSetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute) { return S_OK; }
HRESULT DwmExtendFrameIntoClientArea(HWND hWnd, const void* pMarInset) { return S_OK; }
BOOL DwmDefWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult) { return FALSE; }
