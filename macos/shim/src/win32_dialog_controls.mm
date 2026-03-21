// Win32 dialog controls shim: Button, Edit, Static, ComboBox
// Extracted from win32_controls.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"
#include "win32_dialog_controls_impl.h"

#include <string>
#include <algorithm>

// ============================================================
// ObjC helper: Routes NSButton click -> WM_COMMAND/BN_CLICKED to parent
// ============================================================

@interface Win32ButtonTarget : NSObject
@property (assign) HWND buttonHwnd;
- (void)buttonClicked:(id)sender;
@end

@implementation Win32ButtonTarget
- (void)buttonClicked:(id)sender
{
	auto* info = HandleRegistry::getWindowInfo(self.buttonHwnd);
	if (!info) return;

	HWND parentHwnd = info->parent;
	if (parentHwnd)
	{
		auto* parentInfo = HandleRegistry::getWindowInfo(parentHwnd);
		if (parentInfo && parentInfo->wndProc)
		{
			WPARAM wp = MAKEWPARAM(info->controlId, BN_CLICKED);
			parentInfo->wndProc(parentHwnd, WM_COMMAND, wp,
			                    reinterpret_cast<LPARAM>(self.buttonHwnd));
		}
	}
}
@end

static NSMutableDictionary<NSNumber*, Win32ButtonTarget*>* s_buttonTargets = nil;

// ============================================================
// ObjC helper: Routes NSTextField changes -> WM_COMMAND/EN_CHANGE to parent
// ============================================================

@interface Win32EditTarget : NSObject <NSTextFieldDelegate>
@property (assign) HWND editHwnd;
@end

@implementation Win32EditTarget
- (void)controlTextDidChange:(NSNotification*)notification
{
	auto* info = HandleRegistry::getWindowInfo(self.editHwnd);
	if (!info) return;

	// Update the windowName in the registry
	NSTextField* tf = (__bridge NSTextField*)info->nativeView;
	if (tf)
	{
		NSString* text = tf.stringValue;
		NSData* data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
		if (data && data.length >= sizeof(wchar_t))
			info->windowName = std::wstring(reinterpret_cast<const wchar_t*>(data.bytes),
			                                data.length / sizeof(wchar_t));
		else
			info->windowName.clear();
	}

	HWND parentHwnd = info->parent;
	if (parentHwnd)
	{
		auto* parentInfo = HandleRegistry::getWindowInfo(parentHwnd);
		if (parentInfo && parentInfo->wndProc)
		{
			WPARAM wp = MAKEWPARAM(info->controlId, EN_CHANGE);
			parentInfo->wndProc(parentHwnd, WM_COMMAND, wp,
			                    reinterpret_cast<LPARAM>(self.editHwnd));
		}
	}
}
@end

static NSMutableDictionary<NSNumber*, Win32EditTarget*>* s_editTargets = nil;

// ============================================================
// Button init/destroy
// ============================================================

void Win32Button_Init(void* hwndVoid)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);

	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (info && info->nativeView)
	{
		id view = (__bridge id)info->nativeView;
		if ([view isKindOfClass:[NSButton class]])
		{
			if (!s_buttonTargets)
				s_buttonTargets = [NSMutableDictionary dictionary];

			Win32ButtonTarget* target = [[Win32ButtonTarget alloc] init];
			target.buttonHwnd = hwnd;

			NSButton* btn = (NSButton*)view;
			btn.target = target;
			btn.action = @selector(buttonClicked:);

			s_buttonTargets[@(key)] = target;
		}
	}
}

void Win32Button_Destroy(void* hwndVoid)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);
	NSNumber* num = @(key);
	[s_buttonTargets removeObjectForKey:num];
}

// ============================================================
// Edit init/destroy
// ============================================================

void Win32Edit_Init(void* hwndVoid)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);

	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (info && info->nativeView)
	{
		if (!s_editTargets)
			s_editTargets = [NSMutableDictionary dictionary];

		Win32EditTarget* target = [[Win32EditTarget alloc] init];
		target.editHwnd = hwnd;

		NSTextField* field = (__bridge NSTextField*)info->nativeView;
		field.delegate = target;

		s_editTargets[@(key)] = target;
	}
}

void Win32Edit_Destroy(void* hwndVoid)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);
	NSNumber* num = @(key);
	[s_editTargets removeObjectForKey:num];
}

// ============================================================
// Button message handling
// ============================================================

bool Win32Button_HandleMessage(void* hwndVoid, unsigned int msg,
                                uintptr_t wParam, intptr_t lParam,
                                intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (!info || !info->nativeView) return false;

	NSView* view = (__bridge NSView*)info->nativeView;

	// Only handle if it's actually an NSButton
	if (![view isKindOfClass:[NSButton class]] && ![view isKindOfClass:[NSBox class]])
		return false;

	// NSBox is used for group boxes - only handle WM_SETTEXT
	if ([view isKindOfClass:[NSBox class]])
	{
		if (msg == WM_SETTEXT)
		{
			NSBox* box = (NSBox*)view;
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			box.title = text ? WideToNS(text) : @"";
			result = TRUE;
			return true;
		}
		return false;
	}

	NSButton* btn = (NSButton*)view;

	switch (msg)
	{
		case BM_GETCHECK:
			result = (btn.state == NSControlStateValueOn) ? BST_CHECKED :
			         (btn.state == NSControlStateValueMixed) ? BST_INDETERMINATE :
			         BST_UNCHECKED;
			return true;

		case BM_SETCHECK:
			if (wParam == BST_CHECKED)
				btn.state = NSControlStateValueOn;
			else if (wParam == BST_INDETERMINATE)
				btn.state = NSControlStateValueMixed;
			else
				btn.state = NSControlStateValueOff;
			result = 0;
			return true;

		case BM_CLICK:
			[btn performClick:nil];
			result = 0;
			return true;

		case BM_GETSTATE:
			result = (btn.state == NSControlStateValueOn) ? BST_CHECKED : 0;
			if (btn.isHighlighted)
				result |= BST_PUSHED;
			return true;

		case BM_SETSTATE:
			btn.highlighted = (wParam != 0);
			result = 0;
			return true;

		case WM_SETTEXT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			btn.title = text ? WideToNS(text) : @"";
			info->windowName = text ? text : L"";
			result = TRUE;
			return true;
		}

		case WM_GETTEXT:
		{
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			int maxLen = static_cast<int>(wParam);
			if (buf && maxLen > 0)
			{
				size_t copyLen = (std::min)(static_cast<size_t>(maxLen - 1), info->windowName.size());
				wcsncpy(buf, info->windowName.c_str(), copyLen);
				buf[copyLen] = L'\0';
				result = static_cast<intptr_t>(copyLen);
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case WM_GETTEXTLENGTH:
			result = static_cast<intptr_t>(info->windowName.size());
			return true;

		case WM_ENABLE:
			btn.enabled = (wParam != 0);
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// Edit control message handling
// ============================================================

bool Win32Edit_HandleMessage(void* hwndVoid, unsigned int msg,
                               uintptr_t wParam, intptr_t lParam,
                               intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (!info || !info->nativeView) return false;

	NSView* view = (__bridge NSView*)info->nativeView;

	// Single-line: NSTextField; multi-line: NSScrollView wrapping NSTextView
	NSTextField* textField = nil;
	if ([view isKindOfClass:[NSTextField class]])
		textField = (NSTextField*)view;

	switch (msg)
	{
		case WM_SETTEXT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			info->windowName = text ? text : L"";
			if (textField)
				textField.stringValue = text ? WideToNS(text) : @"";
			else if ([view isKindOfClass:[NSScrollView class]])
			{
				NSTextView* tv = ((NSScrollView*)view).documentView;
				if ([tv isKindOfClass:[NSTextView class]])
					[tv setString:text ? WideToNS(text) : @""];
			}
			result = TRUE;
			return true;
		}

		case WM_GETTEXT:
		{
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			int maxLen = static_cast<int>(wParam);
			if (buf && maxLen > 0)
			{
				NSString* text = nil;
				if (textField)
					text = textField.stringValue;
				else if ([view isKindOfClass:[NSScrollView class]])
				{
					NSTextView* tv = ((NSScrollView*)view).documentView;
					if ([tv isKindOfClass:[NSTextView class]])
						text = tv.string;
				}

				if (text)
				{
					NSData* data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
					size_t wcharCount = data ? data.length / sizeof(wchar_t) : 0;
					size_t copyLen = (std::min)(static_cast<size_t>(maxLen - 1), wcharCount);
					if (copyLen > 0 && data)
						memcpy(buf, data.bytes, copyLen * sizeof(wchar_t));
					buf[copyLen] = L'\0';
					result = static_cast<intptr_t>(copyLen);
				}
				else
				{
					buf[0] = L'\0';
					result = 0;
				}
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case WM_GETTEXTLENGTH:
		{
			NSString* text = nil;
			if (textField)
				text = textField.stringValue;
			else if ([view isKindOfClass:[NSScrollView class]])
			{
				NSTextView* tv = ((NSScrollView*)view).documentView;
				if ([tv isKindOfClass:[NSTextView class]])
					text = tv.string;
			}
			result = text ? static_cast<intptr_t>(text.length) : 0;
			return true;
		}

		case EM_SETSEL:
		{
			if (textField)
			{
				NSText* editor = [textField.window fieldEditor:YES forObject:textField];
				if (editor)
				{
					int start = static_cast<int>(wParam);
					int end = static_cast<int>(lParam);
					if (start == 0 && end == -1)
						[editor selectAll:nil];
					else
						editor.selectedRange = NSMakeRange(MIN(start, end), abs(end - start));
				}
			}
			result = 0;
			return true;
		}

		case EM_GETSEL:
		{
			DWORD* pStart = reinterpret_cast<DWORD*>(wParam);
			DWORD* pEnd = reinterpret_cast<DWORD*>(lParam);
			if (textField)
			{
				NSText* editor = [textField.window fieldEditor:YES forObject:textField];
				if (editor)
				{
					NSRange sel = editor.selectedRange;
					if (pStart) *pStart = static_cast<DWORD>(sel.location);
					if (pEnd) *pEnd = static_cast<DWORD>(sel.location + sel.length);
					result = MAKELONG(sel.location, sel.location + sel.length);
				}
			}
			else
			{
				if (pStart) *pStart = 0;
				if (pEnd) *pEnd = 0;
				result = 0;
			}
			return true;
		}

		case EM_REPLACESEL:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			if (textField)
			{
				NSText* editor = [textField.window fieldEditor:YES forObject:textField];
				if (editor)
					[editor replaceCharactersInRange:editor.selectedRange withString:nsText];
			}
			result = 0;
			return true;
		}

		case EM_SETREADONLY:
		{
			if (textField)
				textField.editable = (wParam == 0);
			result = TRUE;
			return true;
		}

		case EM_GETLINECOUNT:
		{
			NSString* text = nil;
			if (textField)
				text = textField.stringValue;
			if (text)
			{
				NSUInteger lines = 1;
				for (NSUInteger i = 0; i < text.length; ++i)
					if ([text characterAtIndex:i] == '\n') ++lines;
				result = static_cast<intptr_t>(lines);
			}
			else
			{
				result = 1;
			}
			return true;
		}

		case EM_SETLIMITTEXT:
		{
			// Limit text length (not enforced on macOS, just store)
			result = 0;
			return true;
		}

		case EM_GETLIMITTEXT:
			result = 0x7FFFFFFE; // unlimited
			return true;

		case EM_SETMODIFY:
		case EM_GETMODIFY:
		case EM_CANUNDO:
		case EM_UNDO:
		case EM_EMPTYUNDOBUFFER:
		case EM_SETMARGINS:
		case EM_GETMARGINS:
		case EM_SCROLL:
		case EM_SCROLLCARET:
		case EM_SETPASSWORDCHAR:
		case EM_GETFIRSTVISIBLELINE:
		case EM_LINELENGTH:
		case EM_LINEINDEX:
		case EM_GETLINE:
			result = 0;
			return true;

		case WM_ENABLE:
			if (textField)
				textField.enabled = (wParam != 0);
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// Static control message handling
// ============================================================

bool Win32Static_HandleMessage(void* hwndVoid, unsigned int msg,
                                 uintptr_t wParam, intptr_t lParam,
                                 intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (!info || !info->nativeView) return false;

	NSView* view = (__bridge NSView*)info->nativeView;
	if (![view isKindOfClass:[NSTextField class]])
		return false;

	NSTextField* label = (NSTextField*)view;

	switch (msg)
	{
		case WM_SETTEXT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			info->windowName = text ? text : L"";
			label.stringValue = text ? WideToNS(text) : @"";
			result = TRUE;
			return true;
		}

		case WM_GETTEXT:
		{
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			int maxLen = static_cast<int>(wParam);
			if (buf && maxLen > 0)
			{
				size_t copyLen = (std::min)(static_cast<size_t>(maxLen - 1), info->windowName.size());
				wcsncpy(buf, info->windowName.c_str(), copyLen);
				buf[copyLen] = L'\0';
				result = static_cast<intptr_t>(copyLen);
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case WM_GETTEXTLENGTH:
			result = static_cast<intptr_t>(info->windowName.size());
			return true;

		case STM_SETIMAGE:
		case STM_GETIMAGE:
		case STM_SETICON:
		case STM_GETICON:
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// ComboBox message handling
// ============================================================

bool Win32ComboBox_HandleMessage(void* hwndVoid, unsigned int msg,
                                   uintptr_t wParam, intptr_t lParam,
                                   intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (!info || !info->nativeView) return false;

	NSView* view = (__bridge NSView*)info->nativeView;

	// NSComboBox (editable) or NSPopUpButton (dropdown list)
	NSComboBox* combo = nil;
	NSPopUpButton* popup = nil;
	if ([view isKindOfClass:[NSComboBox class]])
		combo = (NSComboBox*)view;
	else if ([view isKindOfClass:[NSPopUpButton class]])
		popup = (NSPopUpButton*)view;
	else
		return false;

	switch (msg)
	{
		case CB_ADDSTRING:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			if (combo)
			{
				[combo addItemWithObjectValue:nsText];
				result = combo.numberOfItems - 1;
			}
			else if (popup)
			{
				[popup addItemWithTitle:nsText];
				result = popup.numberOfItems - 1;
			}
			return true;
		}

		case CB_INSERTSTRING:
		{
			int index = static_cast<int>(wParam);
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			if (combo)
			{
				[combo insertItemWithObjectValue:nsText atIndex:index];
				result = index;
			}
			else if (popup)
			{
				[popup insertItemWithTitle:nsText atIndex:index];
				result = index;
			}
			return true;
		}

		case CB_DELETESTRING:
		{
			int index = static_cast<int>(wParam);
			if (combo && index >= 0 && index < combo.numberOfItems)
			{
				[combo removeItemAtIndex:index];
				result = combo.numberOfItems;
			}
			else if (popup && index >= 0 && index < popup.numberOfItems)
			{
				[popup removeItemAtIndex:index];
				result = popup.numberOfItems;
			}
			else
			{
				result = CB_ERR;
			}
			return true;
		}

		case CB_RESETCONTENT:
			if (combo) [combo removeAllItems];
			else if (popup) [popup removeAllItems];
			result = 0;
			return true;

		case CB_GETCOUNT:
			result = combo ? combo.numberOfItems : (popup ? popup.numberOfItems : 0);
			return true;

		case CB_GETCURSEL:
			result = combo ? combo.indexOfSelectedItem : (popup ? popup.indexOfSelectedItem : -1);
			return true;

		case CB_SETCURSEL:
		{
			int index = static_cast<int>(wParam);
			if (combo)
			{
				if (index >= 0 && index < combo.numberOfItems)
					[combo selectItemAtIndex:index];
				else
					{
					NSInteger currentIndex = combo.indexOfSelectedItem;
					if (currentIndex >= 0)
						[combo deselectItemAtIndex:currentIndex];
				}
			}
			else if (popup)
			{
				if (index >= 0 && index < popup.numberOfItems)
					[popup selectItemAtIndex:index];
			}
			result = index;
			return true;
		}

		case CB_GETLBTEXTLEN:
		{
			int index = static_cast<int>(wParam);
			NSString* text = nil;
			if (combo && index >= 0 && index < combo.numberOfItems)
				text = [combo itemObjectValueAtIndex:index];
			else if (popup && index >= 0 && index < popup.numberOfItems)
				text = [popup itemTitleAtIndex:index];
			result = text ? static_cast<intptr_t>(text.length) : CB_ERR;
			return true;
		}

		case CB_GETLBTEXT:
		{
			int index = static_cast<int>(wParam);
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			NSString* text = nil;
			if (combo && index >= 0 && index < combo.numberOfItems)
				text = [combo itemObjectValueAtIndex:index];
			else if (popup && index >= 0 && index < popup.numberOfItems)
				text = [popup itemTitleAtIndex:index];
			if (text && buf)
			{
				NSData* data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
				size_t wcharCount = data ? data.length / sizeof(wchar_t) : 0;
				if (wcharCount > 0 && data)
					memcpy(buf, data.bytes, wcharCount * sizeof(wchar_t));
				buf[wcharCount] = L'\0';
				result = static_cast<intptr_t>(wcharCount);
			}
			else
			{
				result = CB_ERR;
			}
			return true;
		}

		case CB_FINDSTRINGEXACT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			if (combo)
				result = [combo indexOfItemWithObjectValue:nsText];
			else if (popup)
				result = [popup indexOfItemWithTitle:nsText];
			if (result == NSNotFound)
				result = CB_ERR;
			return true;
		}

		case CB_FINDSTRING:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			NSInteger count = combo ? combo.numberOfItems : (popup ? popup.numberOfItems : 0);
			result = CB_ERR;
			for (NSInteger i = 0; i < count; ++i)
			{
				NSString* item = combo ? [combo itemObjectValueAtIndex:i] : [popup itemTitleAtIndex:i];
				if ([item hasPrefix:nsText])
				{
					result = i;
					break;
				}
			}
			return true;
		}

		case CB_SETITEMDATA:
		case CB_GETITEMDATA:
		case CB_SETITEMHEIGHT:
		case CB_GETITEMHEIGHT:
		case CB_SETEXTENDEDUI:
		case CB_GETEXTENDEDUI:
		case CB_SHOWDROPDOWN:
		case CB_GETDROPPEDSTATE:
			result = 0;
			return true;

		case WM_SETTEXT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			info->windowName = text ? text : L"";
			if (combo) combo.stringValue = text ? WideToNS(text) : @"";
			result = TRUE;
			return true;
		}

		case WM_GETTEXT:
		{
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			int maxLen = static_cast<int>(wParam);
			if (buf && maxLen > 0)
			{
				NSString* text = combo ? combo.stringValue : [popup titleOfSelectedItem];
				NSData* data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
				size_t wcharCount = data ? data.length / sizeof(wchar_t) : 0;
				size_t copyLen = (std::min)(static_cast<size_t>(maxLen - 1), wcharCount);
				if (copyLen > 0 && data)
					memcpy(buf, data.bytes, copyLen * sizeof(wchar_t));
				buf[copyLen] = L'\0';
				result = static_cast<intptr_t>(copyLen);
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case WM_GETTEXTLENGTH:
		{
			NSString* text = combo ? combo.stringValue : [popup titleOfSelectedItem];
			result = text ? static_cast<intptr_t>(text.length) : 0;
			return true;
		}
	}

	return false;
}
