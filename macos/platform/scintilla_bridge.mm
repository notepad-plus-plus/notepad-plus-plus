// Scintilla Cocoa Bridge — Implementation
// This file is compiled WITHOUT the Win32 shim force-include to avoid
// conflicts between ScintillaView.h's WM_COMMAND/WM_NOTIFY values
// and our shim's values.

#import <Cocoa/Cocoa.h>
#import "ScintillaView.h"
#include "scintilla_bridge.h"

// SCI_GETDIRECTFUNCTION and SCI_GETDIRECTPOINTER message IDs
#define SCI_GETDIRECTFUNCTION 2184
#define SCI_GETDIRECTPOINTER  2185

void* ScintillaBridge_createView(void* parentView, int x, int y, int width, int height)
{
	NSView* parent = (__bridge NSView*)parentView;
	if (!parent)
		return nullptr;

	NSRect frame;
	if (width <= 0 || height <= 0)
	{
		// Fill parent bounds
		frame = parent.bounds;
	}
	else
	{
		// Cocoa uses bottom-left origin; flip y relative to parent
		CGFloat parentHeight = parent.bounds.size.height;
		frame = NSMakeRect(x, parentHeight - y - height, width, height);
	}

	ScintillaView* sciView = [[ScintillaView alloc] initWithFrame:frame];
	sciView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	sciView.frame = parent.bounds;

	[parent addSubview:sciView];

	// Return as void* — the view is retained by its superview
	return (__bridge void*)sciView;
}

void ScintillaBridge_destroyView(void* scintillaView)
{
	if (!scintillaView)
		return;
	ScintillaView* view = (__bridge ScintillaView*)scintillaView;
	[view removeFromSuperview];
}

intptr_t ScintillaBridge_sendMessage(void* scintillaView, unsigned int message,
                                     uintptr_t wParam, intptr_t lParam)
{
	if (!scintillaView)
		return 0;
	ScintillaView* view = (__bridge ScintillaView*)scintillaView;
	return [view message:message wParam:(uptr_t)wParam lParam:(sptr_t)lParam];
}

void* ScintillaBridge_getDirectFunction(void* scintillaView)
{
	if (!scintillaView)
		return nullptr;
	ScintillaView* view = (__bridge ScintillaView*)scintillaView;
	sptr_t func = [view message:SCI_GETDIRECTFUNCTION wParam:0 lParam:0];
	return (void*)func;
}

void* ScintillaBridge_getDirectPointer(void* scintillaView)
{
	if (!scintillaView)
		return nullptr;
	ScintillaView* view = (__bridge ScintillaView*)scintillaView;
	sptr_t ptr = [view message:SCI_GETDIRECTPOINTER wParam:0 lParam:0];
	return (void*)ptr;
}

void ScintillaBridge_resizeToFit(void* scintillaView)
{
	if (!scintillaView)
		return;
	ScintillaView* view = (__bridge ScintillaView*)scintillaView;
	NSView* parent = view.superview;
	if (parent)
		view.frame = parent.bounds;
}

void ScintillaBridge_setNotifyCallback(void* scintillaView, intptr_t windowid,
                                        ScintillaBridgeNotifyFunc callback)
{
	if (!scintillaView)
		return;
	ScintillaView* view = (__bridge ScintillaView*)scintillaView;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	[view registerNotifyCallback:windowid value:(SciNotifyFunc)callback];
#pragma clang diagnostic pop
}
