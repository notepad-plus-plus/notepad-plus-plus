#import <Cocoa/Cocoa.h>
#include <algorithm>
#include <atomic>
#include <vector>

#ifdef DEBUG
#include <cstdio>
static FILE* dbgLog()
{
	static FILE* f = fopen("/tmp/MacNotePP.log", "a");
	return f;
}
#define FLLOG(fmt, ...) do { if (FILE* _f = dbgLog()) { fprintf(_f, "[FL] " fmt "\n", ##__VA_ARGS__); fflush(_f); } } while(0)
#else
#define FLLOG(...) do { } while(0)
#endif

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
static std::atomic<bool> sShuttingDown{false};
static std::atomic<int> sParseProgress{-1}; // -1 = idle, 0-100 = parsing
static NSTimer* sProgressTimer = nil;

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

static void stopProgressIndicator()
{
	if (sProgressTimer)
	{
		[sProgressTimer invalidate];
		sProgressTimer = nil;
	}
	sParseProgress.store(-1);
	if (sEmptyLabel)
	{
		sEmptyLabel.stringValue = @"No functions";
		sEmptyLabel.textColor = NSColor.secondaryLabelColor;
	}
}

static void startProgressIndicator()
{
	stopProgressIndicator();
	sParseProgress.store(0);
	if (sEmptyLabel)
	{
		sEmptyLabel.stringValue = @"Parsing\u2026";
		sEmptyLabel.textColor = NSColor.secondaryLabelColor;
		sEmptyLabel.hidden = NO;
	}
	sProgressTimer = [NSTimer scheduledTimerWithTimeInterval:0.3 repeats:YES block:^(NSTimer* timer) {
		int pct = sParseProgress.load();
		if (pct < 0)
		{
			[timer invalidate];
			sProgressTimer = nil;
			return;
		}
		if (sEmptyLabel && !sEmptyLabel.hidden)
			sEmptyLabel.stringValue = [NSString stringWithFormat:@"Parsing\u2026 %d%%", pct];
	}];
}

static void clearPanelData()
{
	stopProgressIndicator();
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
	FLLOG("fetchText: tabIdx=%d docCount=%d", tabIdx, static_cast<int>(docs.size()));
	if (tabIdx < 0 || tabIdx >= static_cast<int>(docs.size()))
	{
		FLLOG("fetchText: BAIL — tabIdx out of range");
		return false;
	}

	outText = docs[tabIdx].content;
	outLangIndex = docs[tabIdx].languageIndex;
	FLLOG("fetchText: langIndex=%d contentLen=%zu", outLangIndex, outText.size());
	void* sci = ctx().activeScintillaView();
	FLLOG("fetchText: scintilla=%p", sci);
	if (sci)
	{
		intptr_t len = ScintillaBridge_sendMessage(sci, SCI_GETTEXTLENGTH, 0, 0);
		FLLOG("fetchText: SCI_GETTEXTLENGTH=%ld maxParse=%zu", static_cast<long>(len), kMaxParseSize);
		if (len > 0 && static_cast<size_t>(len) < kMaxParseSize)
		{
			std::vector<char> buf(static_cast<size_t>(len + 1), '\0');
			ScintillaBridge_sendMessage(sci, SCI_GETTEXT, len + 1, reinterpret_cast<intptr_t>(buf.data()));
			outText.assign(buf.data(), static_cast<size_t>(len));
			FLLOG("fetchText: got %zu bytes from Scintilla", outText.size());
		}
		else
		{
			FLLOG("fetchText: SKIP Scintilla fetch (len=%ld)", static_cast<long>(len));
		}
	}

	bool ok = !outText.empty() && outText.size() < kMaxParseSize;
	FLLOG("fetchText: returning %s (textLen=%zu)", ok ? "true" : "false", outText.size());
	return ok;
}

static void applyParsedNodes(const std::vector<FunctionListNode>& nodes)
{
	stopProgressIndicator();
	FLLOG("applyNodes: %zu nodes, controller=%p outline=%p", nodes.size(), sController, sController ? sController.outlineView : nil);
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
	// Signal background parse to stop and wait for it to drain
	sShuttingDown.store(true);
	stopProgressIndicator();
	invalidateFunctionListPendingRefresh();
	dispatch_sync(parseQueue(), ^{});

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
	{
		clearPanelData();
		invalidateFunctionListPendingRefresh();
	}
}

bool isFunctionListEnabled()
{
	return ctx().functionListEnabled;
}

bool isFunctionListShuttingDown()
{
	return sShuttingDown.load();
}

void setFunctionListParseProgress(int percent)
{
	sParseProgress.store(percent);
}

void bindFunctionListToActiveView()
{
	if (ctx().functionListEnabled)
		scheduleFunctionListRefresh();
}

void updateFunctionListNow()
{
	FLLOG("updateNow: controller=%p outline=%p", sController, sController ? sController.outlineView : nil);
	if (!sController || !sController.outlineView)
	{
		FLLOG("updateNow: BAIL — no controller/outline");
		return;
	}
	if (!shouldShowFunctionList())
	{
		FLLOG("updateNow: BAIL — shouldShow=false (enabled=%d container=%p)", ctx().functionListEnabled, sContainer);
		clearPanelData();
		return;
	}

	std::string text;
	int langIndex = 0;
	if (!fetchActiveDocumentText(text, langIndex))
	{
		FLLOG("updateNow: BAIL — fetchText returned false");
		clearPanelData();
		return;
	}

	FLLOG("updateNow: textLen=%zu langIndex=%d asyncThreshold=%zu", text.size(), langIndex, kAsyncParseThreshold);

	if (text.size() > kAsyncParseThreshold)
	{
		// Parse large files on a background queue to keep the UI responsive
		startProgressIndicator();
		const int generation = sRefreshGeneration.load();
		FLLOG("updateNow: ASYNC parse, generation=%d", generation);
		dispatch_async(parseQueue(), ^{
			int curGen = sRefreshGeneration.load();
			if (generation != curGen)
			{
				FLLOG("asyncParse: STALE (mine=%d current=%d)", generation, curGen);
				return;
			}
			FLLOG("asyncParse: starting parse (textLen=%zu lang=%d)", text.size(), langIndex);
			std::vector<FunctionListNode> nodes = parseFunctionListNodes(langIndex, text);
			FLLOG("asyncParse: done, %zu nodes", nodes.size());
			dispatch_async(dispatch_get_main_queue(), ^{
				int curGen2 = sRefreshGeneration.load();
				if (generation != curGen2)
				{
					FLLOG("asyncApply: STALE (mine=%d current=%d)", generation, curGen2);
					return;
				}
				FLLOG("asyncApply: applying %zu nodes", nodes.size());
				applyParsedNodes(nodes);
			});
		});
	}
	else
	{
		FLLOG("updateNow: SYNC parse");
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
