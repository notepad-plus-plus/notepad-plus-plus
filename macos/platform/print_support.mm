// print_support.mm — Printing via macOS print system (Phase 1 MVP)
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "print_support.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "scintilla_bridge.h"
#include <vector>

void showPrintDialog(NSWindow* parentWindow)
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t len = ScintillaBridge_sendMessage(sci, SCI_GETTEXTLENGTH, 0, 0);
	if (len <= 0) return;

	std::vector<char> buf(len + 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_GETTEXT, len + 1, reinterpret_cast<intptr_t>(buf.data()));
	NSString* text = [[NSString alloc] initWithBytes:buf.data()
	                                          length:static_cast<NSUInteger>(len)
	                                        encoding:NSUTF8StringEncoding];
	if (!text || text.length == 0) return;

	// Use the editor's current font
	NSFont* font = [NSFont fontWithName:[NSString stringWithUTF8String:ctx().fontName.c_str()]
	                               size:ctx().fontSize];
	if (!font)
		font = [NSFont monospacedSystemFontOfSize:ctx().fontSize weight:NSFontWeightRegular];

	// Size the print view from the printable area to match selected paper/margins
	NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
	NSRect printableRect = [printInfo imageablePageBounds];
	NSTextView* printView = [[NSTextView alloc] initWithFrame:printableRect];
	[printView setFont:font];
	[printView setString:text];

	NSPrintOperation* op = [NSPrintOperation printOperationWithView:printView printInfo:printInfo];
	[op setShowsPrintPanel:YES];
	[op setShowsProgressPanel:YES];

	if (parentWindow)
	{
		[op runOperationModalForWindow:parentWindow
		                      delegate:nil
		                didRunSelector:nil
		                   contextInfo:nil];
	}
	else
	{
		[op runOperation];
	}
}
