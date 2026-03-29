// clipboard_history_panel.mm — In-memory clipboard history panel
// Tracks recent clipboard copy/cut operations and allows paste-back
// via double-click or Enter key.

#import <Cocoa/Cocoa.h>
#include <deque>
#include <string>

#include "clipboard_history_panel.h"
#include "function_list_panel.h"
#include "app_state.h"
#include "scintilla_bridge.h"
#include "npp_constants.h"
#include "Scintilla.h"

static constexpr size_t kMaxHistoryEntries = 20;
static constexpr size_t kMaxEntryBytes = 1024 * 1024; // 1 MB
static constexpr size_t kPreviewChars = 80;

// ---------------------------------------------------------------------------
// File-scoped statics (following function_list_panel.mm pattern)
// ---------------------------------------------------------------------------
static NSView* sContainer = nil;
static NSScrollView* sScrollView = nil;
static NSTableView* sTableView = nil;
static std::deque<std::string> sHistory;
static NSInteger sLastChangeCount = 0;
static NSTimer* sMonitorTimer = nil;
static bool sMonitoringActive = false;

// Forward declarations for ObjC classes
@class ClipboardHistoryController;
static ClipboardHistoryController* sController = nil;

// ---------------------------------------------------------------------------
// Custom NSTableView subclass for Enter-key activation
// (Matches FunctionListOutlineView pattern in function_list_panel.mm:90-110)
// ---------------------------------------------------------------------------
@interface ClipboardHistoryTableView : NSTableView
@end

@implementation ClipboardHistoryTableView
- (void)keyDown:(NSEvent*)event
{
	if (event.keyCode == 36) // Return key
	{
		id target = self.target;
		SEL action = self.doubleAction;
		if (target && action)
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
			[target performSelector:action withObject:self];
#pragma clang diagnostic pop
			return;
		}
	}
	[super keyDown:event];
}
@end

// ---------------------------------------------------------------------------
// Helper: format preview string for display
// ---------------------------------------------------------------------------
static NSString* previewForEntry(const std::string& entry)
{
	NSString* full = [NSString stringWithUTF8String:entry.c_str()];
	if (!full)
		return @"(binary)";

	// Replace newlines with visible symbol
	full = [full stringByReplacingOccurrencesOfString:@"\n" withString:@"\u23CE"];
	full = [full stringByReplacingOccurrencesOfString:@"\r" withString:@""];
	full = [full stringByReplacingOccurrencesOfString:@"\t" withString:@"\u21E5"];

	if (full.length > kPreviewChars)
		full = [[full substringToIndex:kPreviewChars] stringByAppendingString:@"\u2026"];

	return full;
}

// ---------------------------------------------------------------------------
// Helper: focus back to editor after paste
// (Matches focusFunctionListEditor pattern in function_list_panel.mm:57-68)
// ---------------------------------------------------------------------------
static void focusActiveEditor()
{
	void* sci = ctx().activeScintillaView();
	if (!sci)
		return;

	ScintillaBridge_focus(sci);

	NSView* sciView = (__bridge NSView*)sci;
	dispatch_async(dispatch_get_main_queue(), ^{
		ScintillaBridge_focus((__bridge void*)sciView);
	});
}

// ---------------------------------------------------------------------------
// ClipboardHistoryController — NSTableView data source + delegate
// ---------------------------------------------------------------------------
@interface ClipboardHistoryController : NSObject <NSTableViewDataSource, NSTableViewDelegate>
- (void)pasteSelectedEntry:(id)sender;
- (void)clearHistory:(id)sender;
@end

@implementation ClipboardHistoryController

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView
{
	return static_cast<NSInteger>(sHistory.size());
}

- (id)tableView:(NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)column row:(NSInteger)row
{
	if (row < 0 || row >= static_cast<NSInteger>(sHistory.size()))
		return @"";
	return previewForEntry(sHistory[static_cast<size_t>(row)]);
}

- (BOOL)tableView:(NSTableView*)tableView shouldSelectRow:(NSInteger)row
{
	return YES;
}

- (void)pasteSelectedEntry:(id)sender
{
	NSInteger row = sTableView ? sTableView.selectedRow : -1;
	if (row < 0 || row >= static_cast<NSInteger>(sHistory.size()))
		return;

	void* sci = ctx().activeScintillaView();
	if (!sci)
		return;

	const std::string& text = sHistory[static_cast<size_t>(row)];

	ScintillaBridge_sendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACESEL, 0, reinterpret_cast<intptr_t>(text.c_str()));
	ScintillaBridge_sendMessage(sci, SCI_ENDUNDOACTION, 0, 0);

	focusActiveEditor();
}

- (void)clearHistory:(id)sender
{
	sHistory.clear();
	if (sTableView)
		[sTableView reloadData];
}

@end

// ---------------------------------------------------------------------------
// Clipboard monitoring (NSTimer on main run loop)
// ---------------------------------------------------------------------------
static void startClipboardMonitoring()
{
	if (sMonitoringActive)
		return;

	sLastChangeCount = [NSPasteboard generalPasteboard].changeCount;
	sMonitorTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
	                                               repeats:YES
	                                                 block:^(NSTimer* timer) {
		NSInteger currentCount = [NSPasteboard generalPasteboard].changeCount;
		if (currentCount == sLastChangeCount)
			return;
		sLastChangeCount = currentCount;

		NSString* text = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
		if (!text || text.length == 0)
			return;

		std::string utf8(text.UTF8String);

		// Skip consecutive duplicates
		if (!sHistory.empty() && sHistory.front() == utf8)
			return;

		// Cap individual entry size
		if (utf8.size() > kMaxEntryBytes)
			utf8.resize(kMaxEntryBytes);

		sHistory.push_front(std::move(utf8));
		if (sHistory.size() > kMaxHistoryEntries)
			sHistory.pop_back();

		if (sTableView)
			[sTableView reloadData];
	}];
	sMonitoringActive = true;
}

static void stopClipboardMonitoring()
{
	[sMonitorTimer invalidate];
	sMonitorTimer = nil;
	sMonitoringActive = false;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void initializeClipboardHistoryPanel()
{
	if (!ctx().mainWindow || !ctx().editorContainer || sContainer)
		return;

	NSView* contentView = ctx().mainWindow.contentView;
	if (!contentView)
		return;

	sController = [[ClipboardHistoryController alloc] init];

	CGFloat width = static_cast<CGFloat>(ctx().clipboardHistoryWidth);
	if (width < 120) width = 120;
	if (width > 360) width = 360;
	ctx().clipboardHistoryWidth = static_cast<int>(width);

	sContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, 100)];
	sContainer.autoresizingMask = NSViewMinXMargin | NSViewHeightSizable;
	[contentView addSubview:sContainer];

	// Scroll view
	sScrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
	sScrollView.translatesAutoresizingMaskIntoConstraints = NO;
	sScrollView.hasVerticalScroller = YES;
	sScrollView.hasHorizontalScroller = NO;
	sScrollView.autohidesScrollers = YES;
	sScrollView.borderType = NSBezelBorder;
	[sContainer addSubview:sScrollView];

	// Table view
	ClipboardHistoryTableView* table = [[ClipboardHistoryTableView alloc] initWithFrame:NSZeroRect];
	table.headerView = nil;
	table.usesAlternatingRowBackgroundColors = YES;
	NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"ClipboardHistoryColumn"];
	column.resizingMask = NSTableColumnAutoresizingMask;
	[table addTableColumn:column];
	table.dataSource = sController;
	table.delegate = sController;
	table.target = sController;
	table.doubleAction = @selector(pasteSelectedEntry:);
	sTableView = table;
	sScrollView.documentView = table;

	// Clear button
	NSButton* clearButton = [NSButton buttonWithTitle:@"Clear" target:sController action:@selector(clearHistory:)];
	clearButton.translatesAutoresizingMaskIntoConstraints = NO;
	clearButton.bezelStyle = NSBezelStyleSmallSquare;
	clearButton.controlSize = NSControlSizeSmall;
	[sContainer addSubview:clearButton];

	// Title label
	NSTextField* titleLabel = [NSTextField labelWithString:@"Clipboard History"];
	titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
	titleLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightSemibold];
	titleLabel.textColor = NSColor.secondaryLabelColor;
	[sContainer addSubview:titleLabel];

	// Layout constraints
	[NSLayoutConstraint activateConstraints:@[
		[titleLabel.topAnchor constraintEqualToAnchor:sContainer.topAnchor constant:6],
		[titleLabel.leadingAnchor constraintEqualToAnchor:sContainer.leadingAnchor constant:6],
		[clearButton.centerYAnchor constraintEqualToAnchor:titleLabel.centerYAnchor],
		[clearButton.trailingAnchor constraintEqualToAnchor:sContainer.trailingAnchor constant:-6],
		[sScrollView.topAnchor constraintEqualToAnchor:titleLabel.bottomAnchor constant:4],
		[sScrollView.leadingAnchor constraintEqualToAnchor:sContainer.leadingAnchor constant:6],
		[sScrollView.trailingAnchor constraintEqualToAnchor:sContainer.trailingAnchor constant:-6],
		[sScrollView.bottomAnchor constraintEqualToAnchor:sContainer.bottomAnchor constant:-6]
	]];

	relayoutFunctionListPanel();
}

void destroyClipboardHistoryPanel()
{
	stopClipboardMonitoring();

	if (sScrollView)
	{
		[sScrollView removeFromSuperview];
		sScrollView = nil;
	}
	sTableView = nil;
	if (sContainer)
	{
		[sContainer removeFromSuperview];
		sContainer = nil;
	}
	sController = nil;
	sHistory.clear();
}

void setClipboardHistoryEnabled(bool enabled)
{
	ctx().clipboardHistoryEnabled = enabled;
	if (enabled && !sContainer)
		initializeClipboardHistoryPanel();
	if (enabled && !sMonitoringActive)
		startClipboardMonitoring();
	relayoutFunctionListPanel();
}

bool isClipboardHistoryEnabled()
{
	return ctx().clipboardHistoryEnabled;
}

void* clipboardHistoryContainerView()
{
	return (__bridge void*)sContainer;
}
