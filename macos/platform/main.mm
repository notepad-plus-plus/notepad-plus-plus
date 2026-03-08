// Notepad++ macOS Port — Phase 1 Entry Point
// Creates an NSApplication with a window containing a ScintillaView editor.
// This demonstrates: window creation, Scintilla Cocoa integration,
// and the Win32 shim bridge (HandleRegistry + ScintillaBridge).

#import <Cocoa/Cocoa.h>
#include "scintilla_bridge.h"

// ============================================================
// Application Delegate
// ============================================================

@interface NppAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (strong) NSWindow* mainWindow;
@property (assign) void* scintillaView;  // ScintillaView* as void*
@end

@implementation NppAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
	[self createMainWindow];
	[self createScintillaEditor];
	[self setupMenuBar];

	[self.mainWindow makeKeyAndOrderFront:nil];
	[NSApp activateIgnoringOtherApps:YES];

	NSLog(@"=== Notepad++ macOS Port — Phase 1 ===");
	NSLog(@"Window created, Scintilla editor active. Type away!");
}

- (void)createMainWindow
{
	NSRect frame = NSMakeRect(100, 100, 900, 700);
	NSUInteger styleMask = NSWindowStyleMaskTitled |
	                       NSWindowStyleMaskClosable |
	                       NSWindowStyleMaskMiniaturizable |
	                       NSWindowStyleMaskResizable;

	self.mainWindow = [[NSWindow alloc] initWithContentRect:frame
	                                    styleMask:styleMask
	                                    backing:NSBackingStoreBuffered
	                                    defer:NO];

	[self.mainWindow setTitle:@"Notepad++ (macOS) — Phase 1"];
	[self.mainWindow setMinSize:NSMakeSize(400, 300)];
	[self.mainWindow setReleasedWhenClosed:NO];
	self.mainWindow.delegate = self;
}

- (void)createScintillaEditor
{
	NSView* contentView = self.mainWindow.contentView;

	// Create ScintillaView via the bridge (fills the content area)
	self.scintillaView = ScintillaBridge_createView((__bridge void*)contentView, 0, 0, 0, 0);

	if (!self.scintillaView)
	{
		NSLog(@"ERROR: Failed to create ScintillaView!");
		return;
	}

	// Configure the editor with some sensible defaults
	[self configureScintilla];

	// Set welcome text
	const char* welcomeText =
		"// Welcome to Notepad++ on macOS!\n"
		"//\n"
		"// Phase 1: Window + Scintilla integration working.\n"
		"// You can type, select, copy/paste, undo/redo.\n"
		"//\n"
		"// Key milestones achieved:\n"
		"//   - Win32 shim layer compiles ~90 N++ source files\n"
		"//   - HandleRegistry maps HWND to NSView/NSWindow\n"
		"//   - ScintillaView (Cocoa) bridge functional\n"
		"//   - Real CreateWindowEx / SendMessage dispatch\n"
		"//\n"
		"// Next: File I/O, menus, tabs, full N++ init.\n"
		"\n"
		"#include <iostream>\n"
		"\n"
		"int main() {\n"
		"    std::cout << \"Hello from Notepad++ macOS!\" << std::endl;\n"
		"    return 0;\n"
		"}\n";

	// SCI_SETTEXT = 2181
	ScintillaBridge_sendMessage(self.scintillaView, 2181, 0, (intptr_t)welcomeText);

	// Go to start
	// SCI_GOTOPOS = 2025
	ScintillaBridge_sendMessage(self.scintillaView, 2025, 0, 0);

	// Clear undo buffer so welcome text isn't undoable
	// SCI_EMPTYUNDOBUFFER = 2175
	ScintillaBridge_sendMessage(self.scintillaView, 2175, 0, 0);

	// Mark as not modified
	// SCI_SETSAVEPOINT = 2014
	ScintillaBridge_sendMessage(self.scintillaView, 2014, 0, 0);
}

- (void)configureScintilla
{
	void* sci = self.scintillaView;
	if (!sci) return;

	// SCI_SETCODEPAGE = 2037, SC_CP_UTF8 = 65001
	ScintillaBridge_sendMessage(sci, 2037, 65001, 0);

	// SCI_SETWRAPMODE = 2268, SC_WRAP_NONE = 0
	ScintillaBridge_sendMessage(sci, 2268, 0, 0);

	// SCI_SETTABWIDTH = 2036
	ScintillaBridge_sendMessage(sci, 2036, 4, 0);

	// SCI_SETUSETABS = 2124, 0 = use spaces
	ScintillaBridge_sendMessage(sci, 2124, 0, 0);

	// Show line numbers
	// SCI_SETMARGINTYPEN = 2240, SC_MARGIN_NUMBER = 0
	ScintillaBridge_sendMessage(sci, 2240, 0, 0);  // margin 0 = line numbers

	// SCI_SETMARGINWIDTHN = 2242, margin 0, width 50
	ScintillaBridge_sendMessage(sci, 2242, 0, 50);

	// Set C++ lexer
	// SCI_SETLEXERLANGUAGE = 4006
	ScintillaBridge_sendMessage(sci, 4006, 0, (intptr_t)"cpp");

	// Configure some C++ keywords
	// SCI_SETKEYWORDS = 4005
	const char* keywords = "int char float double void bool true false "
	                        "if else for while do switch case break continue return "
	                        "class struct enum namespace using typedef "
	                        "const static virtual override public private protected "
	                        "new delete nullptr sizeof typeof "
	                        "try catch throw include define ifdef ifndef endif";
	ScintillaBridge_sendMessage(sci, 4005, 0, (intptr_t)keywords);

	// Default style: Menlo font, 13pt
	// SCI_STYLESETFONT = 2056
	ScintillaBridge_sendMessage(sci, 2056, 32, (intptr_t)"Menlo");  // STYLE_DEFAULT = 32

	// SCI_STYLESETSIZE = 2055
	ScintillaBridge_sendMessage(sci, 2055, 32, 13);

	// SCI_STYLECLEARALL = 2050 (apply default to all styles)
	ScintillaBridge_sendMessage(sci, 2050, 0, 0);

	// Set some syntax highlighting colors
	// SCI_STYLESETFORE = 2051
	// SCE_C_COMMENT = 1, SCE_C_COMMENTLINE = 2, SCE_C_NUMBER = 4,
	// SCE_C_WORD = 5, SCE_C_STRING = 6, SCE_C_PREPROCESSOR = 9
	ScintillaBridge_sendMessage(sci, 2051, 1, 0x008000);   // Comments: green
	ScintillaBridge_sendMessage(sci, 2051, 2, 0x008000);   // Line comments: green
	ScintillaBridge_sendMessage(sci, 2051, 4, 0xFF8000);   // Numbers: orange
	ScintillaBridge_sendMessage(sci, 2051, 5, 0x0000FF);   // Keywords: blue
	ScintillaBridge_sendMessage(sci, 2051, 6, 0x800080);   // Strings: purple
	ScintillaBridge_sendMessage(sci, 2051, 9, 0x808080);   // Preprocessor: gray

	// Bold keywords
	// SCI_STYLESETBOLD = 2053
	ScintillaBridge_sendMessage(sci, 2053, 5, 1);

	// Enable code folding
	// SCI_SETPROPERTY = 4004
	ScintillaBridge_sendMessage(sci, 4004, (uintptr_t)"fold", (intptr_t)"1");
	ScintillaBridge_sendMessage(sci, 4004, (uintptr_t)"fold.compact", (intptr_t)"0");

	// SCI_SETMARGINTYPEN = 2240 (margin 2 = folder)
	ScintillaBridge_sendMessage(sci, 2240, 2, 4);  // SC_MARGIN_SYMBOL = 1... actually type 4 for folding
	// Hmm, let's use margin type SC_MARGIN_SYMBOL=1 with fold mask
	// SCI_SETMARGINMASKN = 2244, SC_MASK_FOLDERS = 0xFE000000
	ScintillaBridge_sendMessage(sci, 2244, 2, 0xFE000000);
	ScintillaBridge_sendMessage(sci, 2242, 2, 16);  // Folder margin width

	// SCI_SETMARGINSENSITIVEN = 2246
	ScintillaBridge_sendMessage(sci, 2246, 2, 1);  // Make folder margin clickable

	// Current line highlight
	// SCI_SETCARETLINEVISIBLE = 2096
	ScintillaBridge_sendMessage(sci, 2096, 1, 0);
	// SCI_SETCARETLINEBACK = 2098
	ScintillaBridge_sendMessage(sci, 2098, 0xF0F0F0, 0);
}

- (void)setupMenuBar
{
	NSMenu* mainMenu = [[NSMenu alloc] init];

	// Application menu
	NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
	NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"Notepad++"];
	[appMenu addItemWithTitle:@"About Notepad++" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
	[appMenu addItem:[NSMenuItem separatorItem]];
	[appMenu addItemWithTitle:@"Quit Notepad++" action:@selector(terminate:) keyEquivalent:@"q"];
	appMenuItem.submenu = appMenu;
	[mainMenu addItem:appMenuItem];

	// File menu
	NSMenuItem* fileMenuItem = [[NSMenuItem alloc] init];
	NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
	[fileMenu addItemWithTitle:@"New" action:@selector(newDocument:) keyEquivalent:@"n"];
	[fileMenu addItemWithTitle:@"Open..." action:@selector(openDocument:) keyEquivalent:@"o"];
	[fileMenu addItem:[NSMenuItem separatorItem]];
	[fileMenu addItemWithTitle:@"Close" action:@selector(performClose:) keyEquivalent:@"w"];
	fileMenuItem.submenu = fileMenu;
	[mainMenu addItem:fileMenuItem];

	// Edit menu
	NSMenuItem* editMenuItem = [[NSMenuItem alloc] init];
	NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
	[editMenu addItemWithTitle:@"Undo" action:@selector(undo:) keyEquivalent:@"z"];
	[editMenu addItemWithTitle:@"Redo" action:@selector(redo:) keyEquivalent:@"Z"];
	[editMenu addItem:[NSMenuItem separatorItem]];
	[editMenu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
	[editMenu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
	[editMenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
	[editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
	editMenuItem.submenu = editMenu;
	[mainMenu addItem:editMenuItem];

	// View menu
	NSMenuItem* viewMenuItem = [[NSMenuItem alloc] init];
	NSMenu* viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
	[viewMenu addItemWithTitle:@"Toggle Word Wrap" action:@selector(toggleWordWrap:) keyEquivalent:@""];
	viewMenuItem.submenu = viewMenu;
	[mainMenu addItem:viewMenuItem];

	[NSApp setMainMenu:mainMenu];
}

// ============================================================
// Menu action handlers
// ============================================================

- (void)newDocument:(id)sender
{
	if (self.scintillaView)
	{
		// SCI_CLEARALL = 2004
		ScintillaBridge_sendMessage(self.scintillaView, 2004, 0, 0);
		// SCI_EMPTYUNDOBUFFER = 2175
		ScintillaBridge_sendMessage(self.scintillaView, 2175, 0, 0);
		// SCI_SETSAVEPOINT = 2014
		ScintillaBridge_sendMessage(self.scintillaView, 2014, 0, 0);
	}
}

- (void)openDocument:(id)sender
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	panel.allowsMultipleSelection = NO;
	panel.canChooseDirectories = NO;

	if ([panel runModal] == NSModalResponseOK)
	{
		NSURL* url = panel.URL;
		NSError* error = nil;
		NSString* content = [NSString stringWithContentsOfURL:url encoding:NSUTF8StringEncoding error:&error];
		if (content && self.scintillaView)
		{
			const char* utf8 = [content UTF8String];
			// SCI_SETTEXT = 2181
			ScintillaBridge_sendMessage(self.scintillaView, 2181, 0, (intptr_t)utf8);
			// SCI_EMPTYUNDOBUFFER = 2175
			ScintillaBridge_sendMessage(self.scintillaView, 2175, 0, 0);
			// SCI_SETSAVEPOINT = 2014
			ScintillaBridge_sendMessage(self.scintillaView, 2014, 0, 0);

			[self.mainWindow setTitle:[NSString stringWithFormat:@"Notepad++ — %@", url.lastPathComponent]];
		}
		else if (error)
		{
			NSLog(@"Error opening file: %@", error);
		}
	}
}

- (void)toggleWordWrap:(id)sender
{
	if (!self.scintillaView) return;

	// SCI_GETWRAPMODE = 2269
	intptr_t mode = ScintillaBridge_sendMessage(self.scintillaView, 2269, 0, 0);
	// Toggle: 0 = none, 1 = word
	// SCI_SETWRAPMODE = 2268
	ScintillaBridge_sendMessage(self.scintillaView, 2268, mode == 0 ? 1 : 0, 0);
}

// ============================================================
// Window delegate
// ============================================================

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
	return YES;
}

- (void)windowDidResize:(NSNotification*)notification
{
	if (self.scintillaView)
		ScintillaBridge_resizeToFit(self.scintillaView);
}

@end

// ============================================================
// Entry point
// ============================================================

int main(int argc, const char* argv[])
{
	@autoreleasepool
	{
		NSApplication* app = [NSApplication sharedApplication];
		[app setActivationPolicy:NSApplicationActivationPolicyRegular];

		NppAppDelegate* delegate = [[NppAppDelegate alloc] init];
		app.delegate = delegate;

		[app run];
	}
	return 0;
}
