// search_results_panel.mm — Search results panel implementation
#import <Cocoa/Cocoa.h>
#include "search_results_panel.h"
#include "find_in_files.h"
#include "file_operations.h"
#include "app_state.h"
#include "string_utils.h"
#include "scintilla_bridge.h"
#include "npp_constants.h"

// ---------------------------------------------------------------------------
// Data model classes
// ---------------------------------------------------------------------------

// Match item: one result line
@interface SearchMatchItem : NSObject
@property (copy) NSString* filePath;      // full path for navigation
@property (copy) NSString* displayText;   // "Line 42:   int count = ..."
@property int lineNumber;                  // 1-based
@property int column;                      // 0-based byte offset
@property int matchLength;                 // bytes
@end

@implementation SearchMatchItem
@end

// File item: represents results within one file
@interface SearchFileItem : NSObject
@property (copy) NSString* filePath;
@property (copy) NSString* displayName;       // "filename.cpp (3 hits)"
@property (strong) NSMutableArray* matchItems; // array of SearchMatchItem
@end

@implementation SearchFileItem
@end

// Root item: represents one search operation
@interface SearchRootItem : NSObject
@property (copy) NSString* summary;           // "Search 'term' (N hits in M files)"
@property (strong) NSMutableArray* fileItems;  // array of SearchFileItem
@end

@implementation SearchRootItem
@end

// ---------------------------------------------------------------------------
// Custom NSOutlineView subclass — handles Enter key
// ---------------------------------------------------------------------------

@interface SearchResultsOutlineView : NSOutlineView
@end

@implementation SearchResultsOutlineView

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
// Outline view data source / delegate
// ---------------------------------------------------------------------------

@interface SearchResultsController : NSObject <NSOutlineViewDataSource, NSOutlineViewDelegate>
@property (strong) NSMutableArray* rootItems;  // array of SearchRootItem
@property (weak) NSOutlineView* outlineView;
- (void)resultDoubleClicked:(id)sender;
@end

@implementation SearchResultsController

- (instancetype)init
{
	self = [super init];
	if (self)
	{
		_rootItems = [[NSMutableArray alloc] init];
	}
	return self;
}

// MARK: NSOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView*)outlineView numberOfChildrenOfItem:(id)item
{
	if (item == nil)
	{
		return static_cast<NSInteger>([_rootItems count]);
	}
	if ([item isKindOfClass:[SearchRootItem class]])
	{
		return static_cast<NSInteger>([((SearchRootItem*)item).fileItems count]);
	}
	if ([item isKindOfClass:[SearchFileItem class]])
	{
		return static_cast<NSInteger>([((SearchFileItem*)item).matchItems count]);
	}
	return 0;
}

- (id)outlineView:(NSOutlineView*)outlineView child:(NSInteger)index ofItem:(id)item
{
	if (item == nil)
	{
		return _rootItems[index];
	}
	if ([item isKindOfClass:[SearchRootItem class]])
	{
		return ((SearchRootItem*)item).fileItems[index];
	}
	if ([item isKindOfClass:[SearchFileItem class]])
	{
		return ((SearchFileItem*)item).matchItems[index];
	}
	return nil;
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item
{
	if ([item isKindOfClass:[SearchRootItem class]])
	{
		return [((SearchRootItem*)item).fileItems count] > 0;
	}
	if ([item isKindOfClass:[SearchFileItem class]])
	{
		return [((SearchFileItem*)item).matchItems count] > 0;
	}
	return NO;
}

// MARK: NSOutlineViewDelegate

- (NSView*)outlineView:(NSOutlineView*)outlineView
	viewForTableColumn:(NSTableColumn*)tableColumn
	              item:(id)item
{
	NSTableCellView* cellView = [outlineView makeViewWithIdentifier:@"SearchResultCell"
	                                                         owner:self];
	if (cellView == nil)
	{
		cellView = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 400, 20)];
		NSTextField* textField = [NSTextField labelWithString:@""];
		textField.translatesAutoresizingMaskIntoConstraints = NO;
		textField.lineBreakMode = NSLineBreakByTruncatingTail;
		[cellView addSubview:textField];
		cellView.textField = textField;
		cellView.identifier = @"SearchResultCell";

		[NSLayoutConstraint activateConstraints:@[
			[textField.leadingAnchor constraintEqualToAnchor:cellView.leadingAnchor constant:4],
			[textField.trailingAnchor constraintEqualToAnchor:cellView.trailingAnchor constant:-4],
			[textField.centerYAnchor constraintEqualToAnchor:cellView.centerYAnchor]
		]];
	}

	NSTextField* textField = cellView.textField;

	if ([item isKindOfClass:[SearchRootItem class]])
	{
		SearchRootItem* rootItem = (SearchRootItem*)item;
		textField.stringValue = rootItem.summary;
		textField.font = [NSFont boldSystemFontOfSize:[NSFont systemFontSize]];
	}
	else if ([item isKindOfClass:[SearchFileItem class]])
	{
		SearchFileItem* fileItem = (SearchFileItem*)item;
		textField.stringValue = fileItem.displayName;
		textField.font = [NSFont boldSystemFontOfSize:[NSFont systemFontSize]];
	}
	else if ([item isKindOfClass:[SearchMatchItem class]])
	{
		SearchMatchItem* matchItem = (SearchMatchItem*)item;
		textField.stringValue = matchItem.displayText;
		textField.font = [NSFont monospacedSystemFontOfSize:[NSFont smallSystemFontSize]
		                                            weight:NSFontWeightRegular];
	}

	return cellView;
}

// MARK: Double-click / Enter navigation

- (void)resultDoubleClicked:(id)sender
{
	NSInteger row = [self.outlineView clickedRow];
	// When triggered via Enter key, clickedRow returns -1; use selectedRow instead
	if (row < 0)
	{
		row = [self.outlineView selectedRow];
	}
	if (row < 0)
	{
		return;
	}

	id item = [self.outlineView itemAtRow:row];

	if ([item isKindOfClass:[SearchMatchItem class]])
	{
		SearchMatchItem* matchItem = (SearchMatchItem*)item;

		// Try to switch to the file if it is already open
		std::wstring wpath = NSStringToWide(matchItem.filePath);
		bool fileReady = switchToFileIfOpen(wpath);

		// If not already open, open it
		if (!fileReady)
		{
			fileReady = openFileAtPath(matchItem.filePath);
		}

		if (fileReady)
		{
			// Navigate to the match in the active Scintilla view
			void* sci = ctx().activeScintillaView();

			// Go to the line (0-based)
			ScintillaBridge_sendMessage(sci, SCI_GOTOLINE,
				static_cast<uintptr_t>(matchItem.lineNumber - 1), 0);

			// Get byte position of the line start
			intptr_t lineStart = ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE,
				static_cast<uintptr_t>(matchItem.lineNumber - 1), 0);

			// Select the match text
			ScintillaBridge_sendMessage(sci, SCI_SETSEL,
				static_cast<uintptr_t>(lineStart + matchItem.column),
				static_cast<intptr_t>(lineStart + matchItem.column + matchItem.matchLength));

			// Scroll to show the caret
			ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);
		}
	}
	else if ([item isKindOfClass:[SearchFileItem class]])
	{
		// Toggle expand/collapse for file items
		if ([self.outlineView isItemExpanded:item])
		{
			[self.outlineView collapseItem:item];
		}
		else
		{
			[self.outlineView expandItem:item];
		}
	}
	else if ([item isKindOfClass:[SearchRootItem class]])
	{
		// Toggle expand/collapse for root items
		if ([self.outlineView isItemExpanded:item])
		{
			[self.outlineView collapseItem:item];
		}
		else
		{
			[self.outlineView expandItem:item];
		}
	}
}

@end

// ---------------------------------------------------------------------------
// File-static state
// ---------------------------------------------------------------------------

static NSPanel* sSearchResultsPanel = nil;
static SearchResultsController* sController = nil;
static NSTextField* sResultCountLabel = nil;

// ---------------------------------------------------------------------------
// Helper: convert C++ result to Obj-C model
// ---------------------------------------------------------------------------

static SearchRootItem* convertResult(const FIFSearchResult& result)
{
	SearchRootItem* root = [[SearchRootItem alloc] init];
	root.summary = [NSString stringWithFormat:@"Search '%s' (%d hits in %d files of %d searched)",
	                result.searchTerm.c_str(),
	                result.totalMatches,
	                result.filesWithMatches,
	                result.filesSearched];
	root.fileItems = [[NSMutableArray alloc] init];

	for (const auto& fr : result.fileResults)
	{
		SearchFileItem* fileItem = [[SearchFileItem alloc] init];
		fileItem.filePath = [NSString stringWithUTF8String:fr.filePath.c_str()];

		NSString* fullPath = fileItem.filePath;
		NSString* fileName = [fullPath lastPathComponent];
		fileItem.displayName = [NSString stringWithFormat:@"%@ (%lu hits)",
		                        fileName, (unsigned long)fr.matches.size()];
		fileItem.matchItems = [[NSMutableArray alloc] init];

		for (const auto& m : fr.matches)
		{
			SearchMatchItem* matchItem = [[SearchMatchItem alloc] init];
			matchItem.filePath = fileItem.filePath;
			matchItem.lineNumber = m.lineNumber;
			matchItem.column = m.column;
			matchItem.matchLength = m.matchLength;
			matchItem.displayText = [NSString stringWithFormat:@"  Line %d:   %s",
			                         m.lineNumber,
			                         m.lineText.c_str()];
			[fileItem.matchItems addObject:matchItem];
		}

		[root.fileItems addObject:fileItem];
	}

	return root;
}

// ---------------------------------------------------------------------------
// Helper: build the panel UI
// ---------------------------------------------------------------------------

static void createPanelIfNeeded()
{
	if (sSearchResultsPanel != nil)
	{
		return;
	}

	// Create the controller
	sController = [[SearchResultsController alloc] init];

	// Create the floating panel
	NSRect frame = NSMakeRect(200, 200, 600, 400);
	NSUInteger styleMask = NSWindowStyleMaskTitled
	                     | NSWindowStyleMaskClosable
	                     | NSWindowStyleMaskResizable
	                     | NSWindowStyleMaskMiniaturizable;
	sSearchResultsPanel = [[NSPanel alloc] initWithContentRect:frame
	                                                 styleMask:styleMask
	                                                   backing:NSBackingStoreBuffered
	                                                     defer:YES];
	sSearchResultsPanel.title = @"Search Results";
	sSearchResultsPanel.floatingPanel = YES;
	sSearchResultsPanel.becomesKeyOnlyIfNeeded = YES;
	sSearchResultsPanel.releasedWhenClosed = NO;
	[sSearchResultsPanel setMinSize:NSMakeSize(300, 200)];

	// Content view
	NSView* contentView = sSearchResultsPanel.contentView;

	// Toolbar area
	NSView* toolbar = [[NSView alloc] initWithFrame:NSZeroRect];
	toolbar.translatesAutoresizingMaskIntoConstraints = NO;
	[contentView addSubview:toolbar];

	// Clear button
	NSButton* clearButton = [NSButton buttonWithTitle:@"Clear"
	                                           target:nil
	                                           action:@selector(clearSearchResultsAction:)];
	clearButton.translatesAutoresizingMaskIntoConstraints = NO;
	[toolbar addSubview:clearButton];

	// Result count label
	sResultCountLabel = [NSTextField labelWithString:@""];
	sResultCountLabel.translatesAutoresizingMaskIntoConstraints = NO;
	sResultCountLabel.alignment = NSTextAlignmentCenter;
	[sResultCountLabel setContentHuggingPriority:NSLayoutPriorityDefaultLow
	                              forOrientation:NSLayoutConstraintOrientationHorizontal];
	[toolbar addSubview:sResultCountLabel];

	// Close button
	NSButton* closeButton = [NSButton buttonWithTitle:@"Close"
	                                           target:nil
	                                           action:@selector(closeSearchResultsAction:)];
	closeButton.translatesAutoresizingMaskIntoConstraints = NO;
	[toolbar addSubview:closeButton];

	// Create a helper target for button actions
	// We use the controller as the target
	clearButton.target = sController;
	closeButton.target = sController;

	// Scroll view + outline view
	NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
	scrollView.translatesAutoresizingMaskIntoConstraints = NO;
	scrollView.hasVerticalScroller = YES;
	scrollView.hasHorizontalScroller = YES;
	scrollView.autohidesScrollers = YES;
	scrollView.borderType = NSBezelBorder;

	SearchResultsOutlineView* outlineView = [[SearchResultsOutlineView alloc] initWithFrame:NSZeroRect];
	outlineView.headerView = nil; // no header
	outlineView.usesAlternatingRowBackgroundColors = YES;

	// Single column that auto-resizes
	NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"ResultColumn"];
	column.resizingMask = NSTableColumnAutoresizingMask;
	[outlineView addTableColumn:column];
	outlineView.outlineTableColumn = column;

	outlineView.dataSource = sController;
	outlineView.delegate = sController;
	sController.outlineView = outlineView;

	// Wire up double-click to navigate to match
	[outlineView setDoubleAction:@selector(resultDoubleClicked:)];
	[outlineView setTarget:sController];

	scrollView.documentView = outlineView;
	[contentView addSubview:scrollView];

	// Layout constraints
	[NSLayoutConstraint activateConstraints:@[
		// Toolbar at top
		[toolbar.topAnchor constraintEqualToAnchor:contentView.topAnchor constant:8],
		[toolbar.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:8],
		[toolbar.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-8],
		[toolbar.heightAnchor constraintEqualToConstant:30],

		// Clear button on left
		[clearButton.leadingAnchor constraintEqualToAnchor:toolbar.leadingAnchor],
		[clearButton.centerYAnchor constraintEqualToAnchor:toolbar.centerYAnchor],

		// Close button on right
		[closeButton.trailingAnchor constraintEqualToAnchor:toolbar.trailingAnchor],
		[closeButton.centerYAnchor constraintEqualToAnchor:toolbar.centerYAnchor],

		// Result count label in center
		[sResultCountLabel.leadingAnchor constraintEqualToAnchor:clearButton.trailingAnchor constant:8],
		[sResultCountLabel.trailingAnchor constraintEqualToAnchor:closeButton.leadingAnchor constant:-8],
		[sResultCountLabel.centerYAnchor constraintEqualToAnchor:toolbar.centerYAnchor],

		// Scroll view fills remaining space
		[scrollView.topAnchor constraintEqualToAnchor:toolbar.bottomAnchor constant:8],
		[scrollView.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:8],
		[scrollView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-8],
		[scrollView.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor constant:-8]
	]];
}

// ---------------------------------------------------------------------------
// Button action methods on SearchResultsController
// ---------------------------------------------------------------------------

@interface SearchResultsController (Actions)
- (void)clearSearchResultsAction:(id)sender;
- (void)closeSearchResultsAction:(id)sender;
@end

@implementation SearchResultsController (Actions)

- (void)clearSearchResultsAction:(id)sender
{
	clearSearchResults();
}

- (void)closeSearchResultsAction:(id)sender
{
	hideSearchResultsPanel();
}

@end

// ---------------------------------------------------------------------------
// Helper: update result count label
// ---------------------------------------------------------------------------

static void updateResultCountLabel()
{
	if (sResultCountLabel == nil || sController == nil)
	{
		return;
	}

	NSInteger totalHits = 0;
	for (SearchRootItem* root in sController.rootItems)
	{
		for (SearchFileItem* file in root.fileItems)
		{
			totalHits += static_cast<NSInteger>([file.matchItems count]);
		}
	}

	if (totalHits == 0)
	{
		sResultCountLabel.stringValue = @"No results";
	}
	else
	{
		sResultCountLabel.stringValue = [NSString stringWithFormat:@"%ld total matches",
		                                 (long)totalHits];
	}
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void showSearchResultsPanel(const FIFSearchResult& result)
{
	createPanelIfNeeded();

	// Convert C++ data to Obj-C model and append
	SearchRootItem* rootItem = convertResult(result);
	[sController.rootItems addObject:rootItem];

	// Reload and expand the new root item
	[sController.outlineView reloadData];
	[sController.outlineView expandItem:rootItem expandChildren:NO];

	updateResultCountLabel();

	// Show the panel
	[sSearchResultsPanel makeKeyAndOrderFront:nil];
}

void clearSearchResults()
{
	if (sController == nil)
	{
		return;
	}

	[sController.rootItems removeAllObjects];
	[sController.outlineView reloadData];
	updateResultCountLabel();
}

void hideSearchResultsPanel()
{
	if (sSearchResultsPanel == nil)
	{
		return;
	}

	[sSearchResultsPanel orderOut:nil];
}

bool isSearchResultsPanelVisible()
{
	if (sSearchResultsPanel == nil)
	{
		return false;
	}

	return [sSearchResultsPanel isVisible] == YES;
}
