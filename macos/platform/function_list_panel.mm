#import <Cocoa/Cocoa.h>
#include <algorithm>
#include <atomic>
#include <vector>

#include "function_list_panel.h"
#include "function_list_parser.h"
#include "app_state.h"
#include "document_manager.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"
#include "split_view.h"
#include "string_utils.h"

@interface FunctionListItem : NSObject
@property (copy) NSString* title;
@property int line;
@property int position;
@property BOOL container;
@property (strong) NSMutableArray<FunctionListItem*>* children;
@end

@implementation FunctionListItem
@end

@interface FunctionListOutlineView : NSOutlineView
@end

@implementation FunctionListOutlineView
- (void)keyDown:(NSEvent*)event
{
	if (event.keyCode == 36)
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

@interface FunctionListController : NSObject <NSOutlineViewDataSource, NSOutlineViewDelegate>
@property (strong) NSMutableArray<FunctionListItem*>* roots;
@property (weak) NSOutlineView* outlineView;
@property (weak) NSTextField* emptyLabel;
- (void)outlineActivated:(id)sender;
@end

@implementation FunctionListController
- (instancetype)init
{
	self = [super init];
	if (self)
		_roots = [NSMutableArray array];
	return self;
}

- (NSInteger)outlineView:(NSOutlineView*)outlineView numberOfChildrenOfItem:(id)item
{
	if (!item) return _roots.count;
	return ((FunctionListItem*)item).children.count;
}

- (id)outlineView:(NSOutlineView*)outlineView child:(NSInteger)index ofItem:(id)item
{
	if (!item) return _roots[index];
	return ((FunctionListItem*)item).children[index];
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item
{
	return ((FunctionListItem*)item).children.count > 0;
}

- (NSView*)outlineView:(NSOutlineView*)outlineView
	viewForTableColumn:(NSTableColumn*)tableColumn
	              item:(id)item
{
	NSTableCellView* cell = [outlineView makeViewWithIdentifier:@"FunctionListCell" owner:self];
	if (!cell)
	{
		cell = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 160, 20)];
		NSTextField* text = [NSTextField labelWithString:@""];
		text.translatesAutoresizingMaskIntoConstraints = NO;
		text.lineBreakMode = NSLineBreakByTruncatingTail;
		[cell addSubview:text];
		cell.textField = text;
		cell.identifier = @"FunctionListCell";
		[NSLayoutConstraint activateConstraints:@[
			[text.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor constant:4],
			[text.trailingAnchor constraintEqualToAnchor:cell.trailingAnchor constant:-4],
			[text.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor]
		]];
	}

	FunctionListItem* flItem = (FunctionListItem*)item;
	cell.textField.stringValue = flItem.title ?: @"";
	if (flItem.container)
		cell.textField.font = [NSFont boldSystemFontOfSize:[NSFont systemFontSize]];
	else
		cell.textField.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
	return cell;
}

- (void)outlineActivated:(id)sender
{
	NSInteger row = [self.outlineView clickedRow];
	if (row < 0)
		row = self.outlineView.selectedRow;
	if (row < 0)
		return;

	FunctionListItem* item = (FunctionListItem*)[self.outlineView itemAtRow:row];
	if (!item)
		return;

	if (item.container)
	{
		if ([self.outlineView isItemExpanded:item])
			[self.outlineView collapseItem:item];
		else
			[self.outlineView expandItem:item];
		return;
	}

	void* sci = ctx().activeScintillaView();
	if (!sci || item.position < 0)
		return;

	ScintillaBridge_sendMessage(sci, SCI_GOTOPOS, static_cast<uintptr_t>(item.position), 0);
	ScintillaBridge_sendMessage(sci, SCI_SETSEL, static_cast<uintptr_t>(item.position), static_cast<intptr_t>(item.position));
	ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);
}
@end

static FunctionListController* sController = nil;
static NSView* sContainer = nil;
static NSTextField* sEmptyLabel = nil;
static NSScrollView* sScroll = nil;
static std::atomic<int> sRefreshGeneration{0};

static constexpr size_t kMaxParseSize = 16 * 1024 * 1024; // 16 MB
static constexpr size_t kAsyncParseThreshold = 2 * 1024 * 1024; // 2 MB

static dispatch_queue_t parseQueue()
{
	static dispatch_queue_t q = dispatch_queue_create("com.notepadpp.functionlist.parse", DISPATCH_QUEUE_SERIAL);
	return q;
}

static FunctionListItem* toItem(const FunctionListNode& node)
{
	FunctionListItem* item = [[FunctionListItem alloc] init];
	item.title = [NSString stringWithUTF8String:node.title.c_str()];
	item.line = node.line;
	item.position = node.position;
	item.container = node.isContainer ? YES : NO;
	item.children = [NSMutableArray array];
	for (const auto& child : node.children)
		[item.children addObject:toItem(child)];
	return item;
}

static bool shouldShowFunctionList()
{
	return ctx().functionListEnabled && sContainer != nil;
}

static void updateEmptyState()
{
	if (!sEmptyLabel || !sController)
		return;
	sEmptyLabel.hidden = (sController.roots.count != 0);
}

static void clearPanelData()
{
	if (!sController || !sController.outlineView)
		return;
	[sController.roots removeAllObjects];
	[sController.outlineView reloadData];
	updateEmptyState();
}

static bool fetchActiveDocumentText(std::string& outText, int& outLangIndex)
{
	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	if (tabIdx < 0 || tabIdx >= static_cast<int>(docs.size()))
		return false;

	outText = docs[tabIdx].content;
	outLangIndex = docs[tabIdx].languageIndex;
	void* sci = ctx().activeScintillaView();
	if (sci)
	{
		intptr_t len = ScintillaBridge_sendMessage(sci, SCI_GETTEXTLENGTH, 0, 0);
		if (len > 0 && static_cast<size_t>(len) < kMaxParseSize)
		{
			std::vector<char> buf(static_cast<size_t>(len + 1), '\0');
			ScintillaBridge_sendMessage(sci, SCI_GETTEXT, len + 1, reinterpret_cast<intptr_t>(buf.data()));
			outText.assign(buf.data(), static_cast<size_t>(len));
		}
	}

	return !outText.empty() && outText.size() < kMaxParseSize;
}

static void applyParsedNodes(const std::vector<FunctionListNode>& nodes)
{
	if (!sController || !sController.outlineView)
		return;
	[sController.roots removeAllObjects];
	for (const auto& n : nodes)
		[sController.roots addObject:toItem(n)];
	[sController.outlineView reloadData];
	for (FunctionListItem* item in sController.roots)
	{
		if (item.container)
			[sController.outlineView expandItem:item];
	}
	updateEmptyState();
}

void initializeFunctionListPanel()
{
	if (!ctx().mainWindow || !ctx().editorContainer || sContainer)
		return;

	NSView* contentView = ctx().mainWindow.contentView;
	if (!contentView)
		return;

	sController = [[FunctionListController alloc] init];

	CGFloat width = static_cast<CGFloat>(ctx().functionListWidth);
	if (width < 120) width = 120;
	if (width > 360) width = 360;
	ctx().functionListWidth = static_cast<int>(width);

	sContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, 100)];
	sContainer.autoresizingMask = NSViewMinXMargin | NSViewHeightSizable;
	[contentView addSubview:sContainer];

	sEmptyLabel = [NSTextField labelWithString:@"No functions"];
	sEmptyLabel.translatesAutoresizingMaskIntoConstraints = NO;
	sEmptyLabel.alignment = NSTextAlignmentCenter;
	sEmptyLabel.textColor = NSColor.secondaryLabelColor;
	[sContainer addSubview:sEmptyLabel];

	sScroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
	sScroll.translatesAutoresizingMaskIntoConstraints = NO;
	sScroll.hasVerticalScroller = YES;
	sScroll.hasHorizontalScroller = NO;
	sScroll.autohidesScrollers = YES;
	sScroll.borderType = NSBezelBorder;
	[sContainer addSubview:sScroll];

	FunctionListOutlineView* outline = [[FunctionListOutlineView alloc] initWithFrame:NSZeroRect];
	outline.headerView = nil;
	outline.usesAlternatingRowBackgroundColors = YES;
	NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"FunctionListColumn"];
	column.resizingMask = NSTableColumnAutoresizingMask;
	[outline addTableColumn:column];
	outline.outlineTableColumn = column;
	outline.dataSource = sController;
	outline.delegate = sController;
	outline.target = sController;
	outline.action = @selector(outlineActivated:);
	outline.doubleAction = @selector(outlineActivated:);
	sController.outlineView = outline;
	sController.emptyLabel = sEmptyLabel;
	sScroll.documentView = outline;

	[NSLayoutConstraint activateConstraints:@[
		[sScroll.topAnchor constraintEqualToAnchor:sContainer.topAnchor constant:6],
		[sScroll.leadingAnchor constraintEqualToAnchor:sContainer.leadingAnchor constant:6],
		[sScroll.trailingAnchor constraintEqualToAnchor:sContainer.trailingAnchor constant:-6],
		[sScroll.bottomAnchor constraintEqualToAnchor:sContainer.bottomAnchor constant:-6],
		[sEmptyLabel.centerXAnchor constraintEqualToAnchor:sContainer.centerXAnchor],
		[sEmptyLabel.centerYAnchor constraintEqualToAnchor:sContainer.centerYAnchor]
	]];
}

void destroyFunctionListPanel()
{
	invalidateFunctionListPendingRefresh();
	if (sScroll)
	{
		[sScroll removeFromSuperview];
		sScroll = nil;
	}
	if (sEmptyLabel)
	{
		[sEmptyLabel removeFromSuperview];
		sEmptyLabel = nil;
	}
	if (sContainer)
	{
		[sContainer removeFromSuperview];
		sContainer = nil;
	}
	sController = nil;
}

void relayoutFunctionListPanel()
{
	if (!ctx().mainWindow || !ctx().editorContainer)
		return;

	NSView* contentView = ctx().mainWindow.contentView;
	if (!contentView)
		return;

	const CGFloat tabHeight = NPP_TAB_BAR_HEIGHT;
	const CGFloat statusHeight = NPP_STATUS_BAR_HEIGHT;
	NSRect baseEditorFrame = NSMakeRect(0, statusHeight,
		contentView.bounds.size.width,
		contentView.bounds.size.height - tabHeight - statusHeight);

	CGFloat mapWidth = (ctx().documentMapEnabled ? static_cast<CGFloat>(ctx().documentMapWidth) : 0.0);
	CGFloat flWidth = (ctx().functionListEnabled ? static_cast<CGFloat>(ctx().functionListWidth) : 0.0);
	if (flWidth < 0) flWidth = 0;
	if (mapWidth < 0) mapWidth = 0;

	CGFloat maxSide = std::max<CGFloat>(0.0, baseEditorFrame.size.width - 120.0);
	if (mapWidth > maxSide)
	{
		mapWidth = maxSide;
		ctx().documentMapWidth = static_cast<int>(mapWidth);
	}
	if (flWidth + mapWidth > maxSide)
	{
		flWidth = std::max<CGFloat>(0.0, maxSide - mapWidth);
		ctx().functionListWidth = static_cast<int>(flWidth);
	}

	NSRect editorFrame = baseEditorFrame;
	editorFrame.size.width -= (flWidth + mapWidth);
	if (editorFrame.size.width < 120)
		editorFrame.size.width = 120;

	if (ctx().isSplit && ctx().splitView)
	{
		ctx().splitView.frame = editorFrame;
		[ctx().splitView adjustSubviews];
	}
	else
	{
		ctx().editorContainer.frame = editorFrame;
	}

	NSRect flFrame = NSMakeRect(NSMaxX(editorFrame), baseEditorFrame.origin.y, flWidth, baseEditorFrame.size.height);
	if (sContainer)
	{
		sContainer.frame = flFrame;
		sContainer.hidden = !ctx().functionListEnabled;
	}

	if (ctx().documentMapContainer)
	{
		NSRect mapFrame = NSMakeRect(NSMaxX(flFrame), baseEditorFrame.origin.y, mapWidth, baseEditorFrame.size.height);
		ctx().documentMapContainer.frame = mapFrame;
		ctx().documentMapContainer.hidden = !ctx().documentMapEnabled;
	}

	layoutSplitTopTabBars();
	if (ctx().documentMapScintilla)
		ScintillaBridge_resizeToFit(ctx().documentMapScintilla);
	if (ctx().scintillaView)
		ScintillaBridge_resizeToFit(ctx().scintillaView);
	if (ctx().isSplit && ctx().scintillaView2)
		ScintillaBridge_resizeToFit(ctx().scintillaView2);
}

void setFunctionListEnabled(bool enabled)
{
	ctx().functionListEnabled = enabled;
	if (enabled && !sContainer)
		initializeFunctionListPanel();
	relayoutFunctionListPanel();
	if (enabled)
		updateFunctionListNow();
	else
		clearPanelData();
}

bool isFunctionListEnabled()
{
	return ctx().functionListEnabled;
}

void bindFunctionListToActiveView()
{
	if (ctx().functionListEnabled)
		scheduleFunctionListRefresh();
}

void updateFunctionListNow()
{
	if (!sController || !sController.outlineView)
		return;
	if (!shouldShowFunctionList())
	{
		clearPanelData();
		return;
	}

	std::string text;
	int langIndex = 0;
	if (!fetchActiveDocumentText(text, langIndex))
	{
		clearPanelData();
		return;
	}

	if (text.size() > kAsyncParseThreshold)
	{
		// Parse large files on a background queue to keep the UI responsive
		const int generation = sRefreshGeneration.load();
		dispatch_async(parseQueue(), ^{
			if (generation != sRefreshGeneration.load())
				return;
			std::vector<FunctionListNode> nodes = parseFunctionListNodes(langIndex, text);
			dispatch_async(dispatch_get_main_queue(), ^{
				if (generation != sRefreshGeneration.load())
					return;
				applyParsedNodes(nodes);
			});
		});
	}
	else
	{
		applyParsedNodes(parseFunctionListNodes(langIndex, text));
	}
}

void scheduleFunctionListRefresh()
{
	if (!ctx().functionListEnabled)
		return;

	const int generation = ++sRefreshGeneration;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 200 * NSEC_PER_MSEC),
		dispatch_get_main_queue(), ^{
			if (generation != sRefreshGeneration)
				return;
			updateFunctionListNow();
		});
}

void invalidateFunctionListPendingRefresh()
{
	++sRefreshGeneration;
}
