// app_delegate.mm — Application delegate, lifecycle, drag-and-drop
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "app_delegate.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "document_manager.h"
#include "file_operations.h"
#include "menu_builder.h"
#include "wndproc.h"
#include "scintilla_config.h"
#include "appearance.h"
#include "recent_files.h"
#include "session_manager.h"
#include "split_view.h"
#include "status_bar.h"
#include "scintilla_bridge.h"
#include "handle_registry.h"
#include "settings_manager.h"
#include "file_monitor_mac.h"
#include "windows.h"
#include "commctrl.h"

// ============================================================
// Drag-and-drop support view
// ============================================================

@implementation NppDropTargetView

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
	NSPasteboard* pb = [sender draggingPasteboard];
	if ([pb canReadObjectForClasses:@[[NSURL class]] options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}])
		return NSDragOperationCopy;
	return NSDragOperationNone;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
	NSPasteboard* pb = [sender draggingPasteboard];
	NSArray<NSURL*>* urls = [pb readObjectsForClasses:@[[NSURL class]]
	                                          options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
	if (!urls || urls.count == 0) return NO;

	for (NSURL* url in urls)
	{
		if (url.isFileURL)
			openFileAtPath(url.path);
	}
	return YES;
}

- (NSView*)hitTest:(NSPoint)point
{
	return nil;
}

@end

// ============================================================
// Dock icon
// ============================================================

static void setDockIconFromLogo()
{
	NSString* executablePath = [[NSBundle mainBundle] executablePath];
	if (!executablePath)
		return;

	NSString* logoPath = [[executablePath stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"logo.png"];
	NSImage* dockIcon = [[NSImage alloc] initWithContentsOfFile:logoPath];
	if (dockIcon)
		[NSApp setApplicationIconImage:dockIcon];
}

// ============================================================
// Application Delegate
// ============================================================

@implementation NppAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
	SettingsManager::instance().load();
	auto& s = SettingsManager::instance().settings;
	ctx().fontName = s.fontName;
	ctx().fontSize = s.fontSize;
	ctx().tabWidth = s.tabWidth;
	ctx().showLineNumbers = s.showLineNumbers;
	setDockIconFromLogo();

	ctx().recentFiles.clear();
	for (const auto& f : s.recentFiles)
	{
		NSString* nsf = [NSString stringWithUTF8String:f.c_str()];
		if (nsf)
			ctx().recentFiles.push_back(NSStringToWide(nsf));
	}

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = MainWndProc;
	wc.lpszClassName = L"Notepad++Phase7";
	RegisterClassExW(&wc);

	HMENU hMenuBar = buildMenuBar();

	ctx().mainHwnd = CreateWindowExW(
		0, L"Notepad++Phase7", L"Notepad++ (macOS) — Phase 7",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		static_cast<int>(s.windowWidth), static_cast<int>(s.windowHeight),
		nullptr, hMenuBar, nullptr, nullptr
	);

	if (!ctx().mainHwnd)
	{
		NSLog(@"ERROR: Failed to create main window!");
		return;
	}

	auto* mainInfo = HandleRegistry::getWindowInfo(ctx().mainHwnd);
	if (mainInfo && mainInfo->nativeWindow)
	{
		ctx().mainWindow = (__bridge NSWindow*)mainInfo->nativeWindow;
		ctx().mainWindow.delegate = self;
		[ctx().mainWindow setMinSize:NSMakeSize(500, 400)];

		if (s.windowX != 100 || s.windowY != 100)
		{
			NSRect frame = ctx().mainWindow.frame;
			frame.origin.x = s.windowX;
			frame.origin.y = s.windowY;
			[ctx().mainWindow setFrame:frame display:YES];
		}
	}

	SetMenu(ctx().mainHwnd, hMenuBar);

	NSView* contentView = ctx().mainWindow.contentView;

	ctx().tabHwnd = CreateWindowExW(
		0, L"SysTabControl32", L"",
		WS_CHILD | WS_VISIBLE | TCS_FOCUSNEVER,
		0, 0,
		static_cast<int>(contentView.bounds.size.width), 28,
		ctx().mainHwnd,
		reinterpret_cast<HMENU>(IDC_TABBAR),
		nullptr, nullptr
	);

	ctx().statusBarHwnd = CreateWindowExW(
		0, L"msctls_statusbar32", L"",
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0,
		ctx().mainHwnd,
		reinterpret_cast<HMENU>(IDC_STATUSBAR),
		nullptr, nullptr
	);

	int parts[] = {180, 360, 480, 570, 630, -1};
	SendMessageW(ctx().statusBarHwnd, SB_SETPARTS, 6, reinterpret_cast<LPARAM>(parts));
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Ln 1, Col 1"));
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(L"0 lines"));
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"C++"));
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(L"UTF-8"));
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 4, reinterpret_cast<LPARAM>(L"LF"));
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 5, reinterpret_cast<LPARAM>(L"Ready"));

	CGFloat tabHeight = 28;
	CGFloat statusHeight = 22;
	CGFloat editorHeight = contentView.bounds.size.height - tabHeight - statusHeight;

	if (ctx().statusBarHwnd)
	{
		auto* sbInfo = HandleRegistry::getWindowInfo(ctx().statusBarHwnd);
		if (sbInfo && sbInfo->nativeView)
		{
			NSView* sbView = (__bridge NSView*)sbInfo->nativeView;
			sbView.frame = NSMakeRect(0, 0, contentView.bounds.size.width, statusHeight);
			sbView.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
		}
	}

	NSRect editorFrame = NSMakeRect(0, statusHeight, contentView.bounds.size.width, editorHeight);
	ctx().editorContainer = [[NSView alloc] initWithFrame:editorFrame];
	ctx().editorContainer.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	[contentView addSubview:ctx().editorContainer];

	ctx().scintillaView = ScintillaBridge_createView((__bridge void*)ctx().editorContainer, 0, 0, 0, 0);
	if (!ctx().scintillaView)
	{
		NSLog(@"ERROR: Failed to create ScintillaView!");
		return;
	}

	configureScintilla(ctx().scintillaView);
	applyAppearance();

	// Scintilla notification callback for main view
	ScintillaBridge_setNotifyCallback(ctx().scintillaView, (intptr_t)ctx().mainHwnd,
		[](intptr_t windowid, unsigned int iMessage, uintptr_t wParam, uintptr_t lParam) {
			if (iMessage == 1002 && lParam)
			{
				struct SciNotifyHeader {
					void* hwndFrom;
					uintptr_t idFrom;
					unsigned int code;
				};
				struct SciNotify {
					SciNotifyHeader nmhdr;
					intptr_t position;
					int ch;
					int modifiers;
					int modificationType;
					const char* text;
					intptr_t length;
					intptr_t linesAdded;
					int message;
					uintptr_t wParam;
					intptr_t sLParam;
					intptr_t line;
					int foldLevelNow;
					int foldLevelPrev;
					int margin;
				};
				auto* scn = reinterpret_cast<const SciNotify*>(lParam);

				if (scn->nmhdr.code == 2028) // SCN_FOCUSIN
					ctx().activeView = 0;
				else if (scn->nmhdr.code == 2010) // SCN_MARGINCLICK
				{
					if (scn->margin == 1 && ctx().scintillaView)
					{
						intptr_t line = ScintillaBridge_sendMessage(ctx().scintillaView,
							SCI_LINEFROMPOSITION, scn->position, 0);
						intptr_t markers = ScintillaBridge_sendMessage(ctx().scintillaView,
							SCI_MARKERGET, line, 0);
						if (markers & BOOKMARK_MASK)
							ScintillaBridge_sendMessage(ctx().scintillaView, SCI_MARKERDELETE, line, BOOKMARK_MARKER);
						else
							ScintillaBridge_sendMessage(ctx().scintillaView, SCI_MARKERADD, line, BOOKMARK_MARKER);
					}
				}
			}
		});

	const char* welcomeText =
		"// Welcome to Notepad++ on macOS \xe2\x80\x94 Phase 7!\n"
		"//\n"
		"// What's new in Phase 7:\n"
		"//   - Settings persistence (window pos, font, recent files)\n"
		"//   - Session management (restores tabs on relaunch)\n"
		"//   - Split view (View > Split View)\n"
		"//   - Edit commands (case conversion, line ops, comments, sort)\n"
		"//   - Encoding support (UTF-8, UTF-16 LE/BE, ANSI, BOM)\n"
		"//   - EOL conversion (LF, CRLF, CR) via Format menu\n"
		"//   - Drag and drop files from Finder\n"
		"//\n"
		"// Includes all Phase 1-6 features:\n"
		"//   Syntax highlighting, code folding, regex search, bookmarks,\n"
		"//   auto-completion, recent files, preferences, language selection\n"
		"\n"
		"#include <iostream>\n"
		"#include <string>\n"
		"#include <vector>\n"
		"#include <memory>\n"
		"\n"
		"namespace demo {\n"
		"\n"
		"// A template class to demonstrate syntax highlighting\n"
		"template <typename T>\n"
		"class Container {\n"
		"public:\n"
		"    explicit Container(size_t capacity)\n"
		"        : _data(std::make_unique<T[]>(capacity))\n"
		"        , _capacity(capacity)\n"
		"        , _size(0) {}\n"
		"\n"
		"    void push_back(const T& value) {\n"
		"        if (_size < _capacity) {\n"
		"            _data[_size++] = value;\n"
		"        }\n"
		"    }\n"
		"\n"
		"    T& operator[](size_t index) {\n"
		"        return _data[index];\n"
		"    }\n"
		"\n"
		"    size_t size() const { return _size; }\n"
		"\n"
		"private:\n"
		"    std::unique_ptr<T[]> _data;\n"
		"    size_t _capacity;\n"
		"    size_t _size;\n"
		"};\n"
		"\n"
		"} // namespace demo\n"
		"\n"
		"/* Multi-line comment:\n"
		"   Demonstrates comment folding and styling */\n"
		"\n"
		"int main() {\n"
		"    demo::Container<std::string> names(10);\n"
		"    names.push_back(\"Hello\");\n"
		"    names.push_back(\"World\");\n"
		"\n"
		"    for (size_t i = 0; i < names.size(); ++i) {\n"
		"        std::cout << \"Item \" << i << \": \" << names[i] << std::endl;\n"
		"    }\n"
		"\n"
		"    constexpr double pi = 3.14159;\n"
		"    int count = 42;\n"
		"    bool found = true;\n"
		"\n"
		"    #ifdef DEBUG\n"
		"    std::cout << \"Debug mode\" << std::endl;\n"
		"    #endif\n"
		"\n"
		"    return 0;\n"
		"}\n";

	addNewTab(L"Welcome", std::string(welcomeText));

	if (s.wordWrap && ctx().scintillaView)
		ScintillaBridge_sendMessage(ctx().scintillaView, SCI_SETWRAPMODE, 1, 0);
	if (!ctx().showLineNumbers && ctx().scintillaView)
		ScintillaBridge_sendMessage(ctx().scintillaView, SCI_SETMARGINWIDTHN, 0, 0);

	rebuildRecentMenu();
	SetTimer(ctx().mainHwnd, IDT_STATUSBAR, 500, nullptr);

	ctx().fileMonitor = new FileMonitorMac();

	ShowWindow(ctx().mainHwnd, SW_SHOW);
	[NSApp activateIgnoringOtherApps:YES];

	// Drag and drop
	{
		NppDropTargetView* dropView = [[NppDropTargetView alloc] initWithFrame:contentView.bounds];
		dropView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		[dropView registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
		[contentView addSubview:dropView positioned:NSWindowAbove relativeTo:nil];
	}

	[NSDistributedNotificationCenter.defaultCenter
		addObserver:self
		   selector:@selector(appearanceChanged:)
		       name:@"AppleInterfaceThemeChangedNotification"
		     object:nil];

	restoreSession();

	NSLog(@"=== Notepad++ macOS Port — Phase 7 ===");
	NSLog(@"Settings, split view, edit commands, encoding, session, drag-and-drop!");
}

- (void)appearanceChanged:(NSNotification*)notification
{
	dispatch_async(dispatch_get_main_queue(), ^{
		applyAppearance();
		if (ctx().scintillaView)
			ScintillaBridge_sendMessage(ctx().scintillaView, SCI_COLOURISE, 0, -1);
		if (ctx().isSplit && ctx().scintillaView2)
			ScintillaBridge_sendMessage(ctx().scintillaView2, SCI_COLOURISE, 0, -1);
	});
}

- (void)performContextAction:(NSMenuItem*)sender
{
	if (ctx().mainHwnd)
		SendMessageW(ctx().mainHwnd, WM_COMMAND, MAKEWPARAM(static_cast<WORD>(sender.tag), 0), 0);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
	return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
	saveSession();

	auto& s = SettingsManager::instance().settings;
	if (ctx().mainWindow)
	{
		NSRect frame = ctx().mainWindow.frame;
		s.windowX = frame.origin.x;
		s.windowY = frame.origin.y;
		s.windowWidth = frame.size.width;
		s.windowHeight = frame.size.height;
	}
	s.fontName = ctx().fontName;
	s.fontSize = ctx().fontSize;
	s.tabWidth = ctx().tabWidth;
	s.showLineNumbers = ctx().showLineNumbers;
	s.wordWrap = ctx().scintillaView ?
		(ScintillaBridge_sendMessage(ctx().scintillaView, SCI_GETWRAPMODE, 0, 0) != 0) : false;

	s.recentFiles.clear();
	for (const auto& rf : ctx().recentFiles)
	{
		NSString* ns = WideToNSString(rf.c_str());
		if (ns)
			s.recentFiles.push_back([ns UTF8String]);
	}
	SettingsManager::instance().save();

	if (ctx().fileMonitor)
	{
		ctx().fileMonitor->terminate();
		delete ctx().fileMonitor;
		ctx().fileMonitor = nullptr;
	}
}

- (void)windowDidResize:(NSNotification*)notification
{
	if (ctx().scintillaView)
		ScintillaBridge_resizeToFit(ctx().scintillaView);
	if (ctx().isSplit && ctx().scintillaView2)
		ScintillaBridge_resizeToFit(ctx().scintillaView2);

	layoutSplitTopTabBars();
}

- (void)splitViewDidResizeSubviews:(NSNotification*)notification
{
	layoutSplitTopTabBars();
}

@end
