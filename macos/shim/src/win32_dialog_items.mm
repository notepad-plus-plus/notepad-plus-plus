// Win32 Common Dialog stubs: ChooseColor, ChooseFont, PrintDlg, etc.
// Extracted from win32_menu.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commdlg.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"

// ============================================================
// Color picker
// ============================================================

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

// ============================================================
// Common dialog stubs
// ============================================================

BOOL ChooseFontW(LPCHOOSEFONTW lpcf) { return FALSE; }
BOOL PrintDlgW(LPPRINTDLGW lppd) { return FALSE; }
HWND FindTextW(LPFINDREPLACEW lpfr) { return nullptr; }
HWND ReplaceTextW(LPFINDREPLACEW lpfr) { return nullptr; }
DWORD CommDlgExtendedError() { return 0; }
