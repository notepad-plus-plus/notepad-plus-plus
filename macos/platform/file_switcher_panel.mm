// file_switcher_panel.mm — Compact open-documents list panel
// Shows all open documents in a table view, supports split view separator,
// single-click switching, and context menu (Close, Save, Copy Path, Reveal).

#import <Cocoa/Cocoa.h>
#include <string>
#include <vector>

#include "file_switcher_panel.h"
#include "panel_layout.h"
#include "app_state.h"
#include "string_utils.h"
#include "document_manager.h"
#include "file_operations.h"
#include "npp_constants.h"

// ---------------------------------------------------------------------------
// File-scoped statics (following clipboard_history_panel.mm pattern)
// ---------------------------------------------------------------------------
static NSView* sContainer = nil;
static NSScrollView* sScrollView = nil;
static NSTableView* sTableView = nil;

@class FileSwitcherController;
static FileSwitcherController* sController = nil;

// ---------------------------------------------------------------------------
// Helper: convert wstring to NSString (uses existing WideToNSString pattern)
// ---------------------------------------------------------------------------
static NSString* wstringToNSString(const std::wstring& ws)
{
	return WideToNSString(ws.c_str());
}

// ---------------------------------------------------------------------------
// Helper: derive display path from a document's filePath
// ---------------------------------------------------------------------------
static NSString* displayPathForDocument(const DocumentData& doc)
{
	if (doc.filePath.empty())
		return @"(unsaved)";

	NSString* fullPath = wstringToNSString(doc.filePath);
	if (!fullPath || fullPath.length == 0)
		return @"(unsaved)";

	// If fileBrowserRootPath is set and the path starts with it, show relative path
	const std::string& rootPath = ctx().fileBrowserRootPath;
	if (!rootPath.empty())
	{
		NSString* root = [NSString stringWithUTF8String:rootPath.c_str()];
		if (root && [fullPath hasPrefix:root])
		{
			NSString* relative = [fullPath substringFromIndex:root.length];
			if ([relative hasPrefix:@"/"])
				relative = [relative substringFromIndex:1];
			return relative;
		}
	}

	// Otherwise show parent directory name
	NSString* parent = [fullPath stringByDeletingLastPathComponent];
	return [parent lastPathComponent];
}

// ---------------------------------------------------------------------------
// Custom NSTableView subclass for Enter-key activation
// (Matches ClipboardHistoryTableView pattern)
// ---------------------------------------------------------------------------
@interface FileSwitcherTableView : NSTableView
@end

@implementation FileSwitcherTableView
- (void)keyDown:(NSEvent*)event
{
	if (event.keyCode == 36) // Return key
	{
		id target = self.target;
		SEL action = self.action;
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
// resolveRow — maps table row to (viewIndex, docIndex)
// Returns false for separator rows.
// ---------------------------------------------------------------------------
struct ResolvedRow
{
	int viewIndex;
	int docIndex;
};

static bool resolveRow(NSInteger row, ResolvedRow& out)
{
	auto& docs = ctx().documents;
	auto& docs2 = ctx().documents2;
	int count1 = static_cast<int>(docs.size());

	if (row < 0)
		return false;

	if (row < count1)
	{
		out.viewIndex = 0;
		out.docIndex = static_cast<int>(row);
		return true;
	}

	if (!ctx().isSplit)
		return false;

	// Separator row
	if (row == count1)
		return false;

	int idx2 = static_cast<int>(row) - count1 - 1;
	if (idx2 < 0 || idx2 >= static_cast<int>(docs2.size()))
		return false;

	out.viewIndex = 1;
	out.docIndex = idx2;
	return true;
}

// ---------------------------------------------------------------------------
// switchToDocumentAtRow — activates the document at the given table row
// ---------------------------------------------------------------------------
static void switchToDocumentAtRow(NSInteger row)
{
	ResolvedRow resolved;
	if (!resolveRow(row, resolved))
		return;

	switchToTabInView(resolved.viewIndex, resolved.docIndex);
}

// ---------------------------------------------------------------------------
// FileSwitcherController — NSTableView data source + delegate
// ---------------------------------------------------------------------------
@interface FileSwitcherController : NSObject <NSTableViewDataSource, NSTableViewDelegate, NSMenuDelegate>
- (void)tableViewClicked:(id)sender;
- (void)closeDocument:(id)sender;
- (void)saveDocument:(id)sender;
- (void)copyPath:(id)sender;
- (void)revealInFinder:(id)sender;
@end

@implementation FileSwitcherController

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView
{
	NSInteger count = static_cast<NSInteger>(ctx().documents.size());
	if (ctx().isSplit)
		count += 1 + static_cast<NSInteger>(ctx().documents2.size());
	return count;
}

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row
{
	auto& docs = ctx().documents;
	int count1 = static_cast<int>(docs.size());

	// Check if this is a separator row
	if (ctx().isSplit && row == count1)
	{
		NSTextField* label = [NSTextField labelWithString:@"\u2014 Split View \u2014"];
		label.alignment = NSTextAlignmentCenter;
		label.font = [NSFont systemFontOfSize:10 weight:NSFontWeightMedium];
		label.textColor = NSColor.tertiaryLabelColor;

		NSTableCellView* cell = [[NSTableCellView alloc] initWithFrame:NSZeroRect];
		label.translatesAutoresizingMaskIntoConstraints = NO;
		[cell addSubview:label];
		[NSLayoutConstraint activateConstraints:@[
			[label.centerXAnchor constraintEqualToAnchor:cell.centerXAnchor],
			[label.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor]
		]];
		return cell;
	}

	// Resolve to document
	ResolvedRow resolved;
	if (!resolveRow(row, resolved))
		return nil;

	auto& docList = (resolved.viewIndex == 0) ? ctx().documents : ctx().documents2;
	if (resolved.docIndex < 0 || resolved.docIndex >= static_cast<int>(docList.size()))
		return nil;

	const auto& doc = docList[resolved.docIndex];

	// Determine if this is the active document in its view
	int activeIdx = (resolved.viewIndex == 0) ? ctx().activeTab : ctx().activeTab2;
	bool isActive = (resolved.docIndex == activeIdx) && (resolved.viewIndex == ctx().activeView);

	// Build two-line cell
	NSTableCellView* cell = [[NSTableCellView alloc] initWithFrame:NSZeroRect];

	// Modified indicator dot (6px orange circle)
	NSView* modDot = [[NSView alloc] initWithFrame:NSZeroRect];
	modDot.translatesAutoresizingMaskIntoConstraints = NO;
	modDot.wantsLayer = YES;
	modDot.layer.cornerRadius = 3.0;
	modDot.layer.backgroundColor = doc.modified
		? [NSColor orangeColor].CGColor
		: [NSColor clearColor].CGColor;
	[cell addSubview:modDot];

	// Filename label
	NSString* titleStr = wstringToNSString(doc.title);
	NSTextField* titleLabel = [NSTextField labelWithString:titleStr ?: @"Untitled"];
	titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
	titleLabel.font = isActive
		? [NSFont systemFontOfSize:12 weight:NSFontWeightSemibold]
		: [NSFont systemFontOfSize:12 weight:NSFontWeightRegular];
	titleLabel.textColor = NSColor.labelColor;
	titleLabel.lineBreakMode = NSLineBreakByTruncatingMiddle;
	[cell addSubview:titleLabel];

	// Path label (dimmer, smaller)
	NSString* pathStr = displayPathForDocument(doc);
	NSTextField* pathLabel = [NSTextField labelWithString:pathStr];
	pathLabel.translatesAutoresizingMaskIntoConstraints = NO;
	pathLabel.font = [NSFont systemFontOfSize:10 weight:NSFontWeightRegular];
	pathLabel.textColor = NSColor.secondaryLabelColor;
	pathLabel.lineBreakMode = NSLineBreakByTruncatingMiddle;
	[cell addSubview:pathLabel];

	// Layout constraints
	[NSLayoutConstraint activateConstraints:@[
		// Modified dot: 6x6, vertically centered, left edge
		[modDot.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor constant:4],
		[modDot.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor],
		[modDot.widthAnchor constraintEqualToConstant:6],
		[modDot.heightAnchor constraintEqualToConstant:6],

		// Title: to the right of the dot
		[titleLabel.leadingAnchor constraintEqualToAnchor:modDot.trailingAnchor constant:4],
		[titleLabel.trailingAnchor constraintLessThanOrEqualToAnchor:cell.trailingAnchor constant:-4],
		[titleLabel.topAnchor constraintEqualToAnchor:cell.topAnchor constant:2],

		// Path: below title
		[pathLabel.leadingAnchor constraintEqualToAnchor:titleLabel.leadingAnchor],
		[pathLabel.trailingAnchor constraintLessThanOrEqualToAnchor:cell.trailingAnchor constant:-4],
		[pathLabel.topAnchor constraintEqualToAnchor:titleLabel.bottomAnchor constant:0],
		[pathLabel.bottomAnchor constraintLessThanOrEqualToAnchor:cell.bottomAnchor constant:-2]
	]];

	return cell;
}

- (CGFloat)tableView:(NSTableView*)tableView heightOfRow:(NSInteger)row
{
	auto& docs = ctx().documents;
	int count1 = static_cast<int>(docs.size());

	// Separator row is shorter
	if (ctx().isSplit && row == count1)
		return 20.0;

	return 34.0; // Two lines (filename + path)
}

- (BOOL)tableView:(NSTableView*)tableView shouldSelectRow:(NSInteger)row
{
	// Don't allow selecting the separator
	auto& docs = ctx().documents;
	int count1 = static_cast<int>(docs.size());
	if (ctx().isSplit && row == count1)
		return NO;
	return YES;
}

- (BOOL)tableView:(NSTableView*)tableView isGroupRow:(NSInteger)row
{
	auto& docs = ctx().documents;
	int count1 = static_cast<int>(docs.size());
	return ctx().isSplit && row == count1;
}

// ---------------------------------------------------------------------------
// Single-click action
// ---------------------------------------------------------------------------
- (void)tableViewClicked:(id)sender
{
	NSInteger row = sTableView ? sTableView.clickedRow : -1;
	if (row < 0)
		return;
	switchToDocumentAtRow(row);
}

// ---------------------------------------------------------------------------
// Context menu actions
// ---------------------------------------------------------------------------
- (void)closeDocument:(id)sender
{
	NSInteger row = sTableView ? sTableView.clickedRow : -1;
	ResolvedRow resolved;
	if (!resolveRow(row, resolved))
		return;

	closeTabFromView(resolved.viewIndex, resolved.docIndex);
	reloadFileSwitcherData();
}

- (void)saveDocument:(id)sender
{
	NSInteger row = sTableView ? sTableView.clickedRow : -1;
	ResolvedRow resolved;
	if (!resolveRow(row, resolved))
		return;

	// Switch to the document first, then trigger save
	switchToTabInView(resolved.viewIndex, resolved.docIndex);
	saveCurrentFile();
}

- (void)copyPath:(id)sender
{
	NSInteger row = sTableView ? sTableView.clickedRow : -1;
	ResolvedRow resolved;
	if (!resolveRow(row, resolved))
		return;

	auto& docList = (resolved.viewIndex == 0) ? ctx().documents : ctx().documents2;
	if (resolved.docIndex < 0 || resolved.docIndex >= static_cast<int>(docList.size()))
		return;

	const auto& doc = docList[resolved.docIndex];
	if (doc.filePath.empty())
		return;

	NSString* path = wstringToNSString(doc.filePath);
	if (path)
	{
		NSPasteboard* pb = [NSPasteboard generalPasteboard];
		[pb clearContents];
		[pb setString:path forType:NSPasteboardTypeString];
	}
}

- (void)revealInFinder:(id)sender
{
	NSInteger row = sTableView ? sTableView.clickedRow : -1;
	ResolvedRow resolved;
	if (!resolveRow(row, resolved))
		return;

	auto& docList = (resolved.viewIndex == 0) ? ctx().documents : ctx().documents2;
	if (resolved.docIndex < 0 || resolved.docIndex >= static_cast<int>(docList.size()))
		return;

	const auto& doc = docList[resolved.docIndex];
	if (doc.filePath.empty())
		return;

	NSString* path = wstringToNSString(doc.filePath);
	if (path)
		[[NSWorkspace sharedWorkspace] selectFile:path inFileViewerRootedAtPath:@""];
}

// ---------------------------------------------------------------------------
// Context menu delegate — build menu for right-clicked row
// ---------------------------------------------------------------------------
- (void)menuNeedsUpdate:(NSMenu*)menu
{
	[menu removeAllItems];

	NSInteger row = sTableView ? sTableView.clickedRow : -1;
	ResolvedRow resolved;
	if (!resolveRow(row, resolved))
		return;

	auto& docList = (resolved.viewIndex == 0) ? ctx().documents : ctx().documents2;
	if (resolved.docIndex < 0 || resolved.docIndex >= static_cast<int>(docList.size()))
		return;

	const auto& doc = docList[resolved.docIndex];

	[menu addItemWithTitle:@"Close" action:@selector(closeDocument:) keyEquivalent:@""];
	[menu addItemWithTitle:@"Save" action:@selector(saveDocument:) keyEquivalent:@""];

	if (!doc.filePath.empty())
	{
		[menu addItem:[NSMenuItem separatorItem]];
		[menu addItemWithTitle:@"Copy Path" action:@selector(copyPath:) keyEquivalent:@""];
		[menu addItemWithTitle:@"Reveal in Finder" action:@selector(revealInFinder:) keyEquivalent:@""];
	}

	for (NSMenuItem* item in menu.itemArray)
	{
		if (item.action)
			item.target = self;
	}
}

@end

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void initializeFileSwitcherPanel()
{
	if (!ctx().mainWindow || !ctx().editorContainer || sContainer)
		return;

	NSView* contentView = ctx().mainWindow.contentView;
	if (!contentView)
		return;

	sController = [[FileSwitcherController alloc] init];

	CGFloat width = static_cast<CGFloat>(ctx().leftPanelWidth);
	if (width < 120) width = 120;
	if (width > 360) width = 360;
	ctx().leftPanelWidth = static_cast<int>(width);

	sContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, 100)];
	sContainer.autoresizingMask = NSViewWidthSizable;
	[contentView addSubview:sContainer];

	// Title label
	NSTextField* titleLabel = [NSTextField labelWithString:@"Open Files"];
	titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
	titleLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightSemibold];
	titleLabel.textColor = NSColor.secondaryLabelColor;
	[sContainer addSubview:titleLabel];

	// Scroll view
	sScrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
	sScrollView.translatesAutoresizingMaskIntoConstraints = NO;
	sScrollView.hasVerticalScroller = YES;
	sScrollView.hasHorizontalScroller = NO;
	sScrollView.autohidesScrollers = YES;
	sScrollView.borderType = NSBezelBorder;
	[sContainer addSubview:sScrollView];

	// Table view
	FileSwitcherTableView* table = [[FileSwitcherTableView alloc] initWithFrame:NSZeroRect];
	table.headerView = nil;
	table.usesAlternatingRowBackgroundColors = YES;
	NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"FileSwitcherColumn"];
	column.resizingMask = NSTableColumnAutoresizingMask;
	[table addTableColumn:column];
	table.dataSource = sController;
	table.delegate = sController;
	table.target = sController;
	table.action = @selector(tableViewClicked:);
	sTableView = table;
	sScrollView.documentView = table;

	// Context menu
	NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@""];
	contextMenu.delegate = sController;
	table.menu = contextMenu;

	// Layout constraints
	[NSLayoutConstraint activateConstraints:@[
		[titleLabel.topAnchor constraintEqualToAnchor:sContainer.topAnchor constant:6],
		[titleLabel.leadingAnchor constraintEqualToAnchor:sContainer.leadingAnchor constant:6],
		[sScrollView.topAnchor constraintEqualToAnchor:titleLabel.bottomAnchor constant:4],
		[sScrollView.leadingAnchor constraintEqualToAnchor:sContainer.leadingAnchor constant:6],
		[sScrollView.trailingAnchor constraintEqualToAnchor:sContainer.trailingAnchor constant:-6],
		[sScrollView.bottomAnchor constraintEqualToAnchor:sContainer.bottomAnchor constant:-6]
	]];
}

void destroyFileSwitcherPanel()
{
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
}

void setFileSwitcherEnabled(bool enabled)
{
	ctx().fileSwitcherEnabled = enabled;
	if (enabled)
	{
		if (!sContainer)
			initializeFileSwitcherPanel();
		reloadFileSwitcherData();
	}
	relayoutPanels();
}

void bindFileSwitcherToActiveView()
{
	if (!sTableView || !ctx().fileSwitcherEnabled)
		return;

	[sTableView reloadData];

	// Select the row corresponding to the active document in the active view
	auto& docs = ctx().documents;
	int activeTab = ctx().activeTab;
	int activeView = ctx().activeView;

	NSInteger targetRow = -1;

	if (activeView == 0)
	{
		if (activeTab >= 0 && activeTab < static_cast<int>(docs.size()))
			targetRow = activeTab;
	}
	else if (ctx().isSplit)
	{
		int count1 = static_cast<int>(docs.size());
		int activeTab2 = ctx().activeTab2;
		if (activeTab2 >= 0 && activeTab2 < static_cast<int>(ctx().documents2.size()))
			targetRow = count1 + 1 + activeTab2; // +1 for separator row
	}

	if (targetRow >= 0 && targetRow < sTableView.numberOfRows)
	{
		[sTableView selectRowIndexes:[NSIndexSet indexSetWithIndex:targetRow]
		        byExtendingSelection:NO];
		[sTableView scrollRowToVisible:targetRow];
	}
}

void reloadFileSwitcherData()
{
	if (!sTableView || !ctx().fileSwitcherEnabled)
		return;

	[sTableView reloadData];
}

void* fileSwitcherContainerView()
{
	return (__bridge void*)sContainer;
}
