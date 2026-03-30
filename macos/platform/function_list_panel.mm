#import <Cocoa/Cocoa.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "clipboard_history_panel.h"

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
#include "panel_layout.h"
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

static intptr_t functionListTargetPosition(void* sci, const FunctionListItem* item)
{
	if (!sci || !item)
		return -1;

	if (item.line > 0)
	{
		return ScintillaBridge_sendMessage(sci, SCI_POSITIONFROMLINE,
			static_cast<uintptr_t>(item.line - 1), 0);
	}

	if (item.position >= 0)
		return item.position;

	return -1;
}

static void focusFunctionListEditor(void* sci)
{
	if (!sci)
		return;

	ScintillaBridge_focus(sci);

	NSView* sciView = (__bridge NSView*)sci;
	dispatch_async(dispatch_get_main_queue(), ^{
		ScintillaBridge_focus((__bridge void*)sciView);
	});
}

static void activateFunctionListItem(FunctionListItem* item)
{
	if (!item)
		return;

	void* sci = ctx().activeScintillaView();
	if (!sci)
		return;

	intptr_t targetPos = functionListTargetPosition(sci, item);
	if (targetPos < 0)
		return;

	ScintillaBridge_sendMessage(sci, SCI_GOTOPOS, static_cast<uintptr_t>(targetPos), 0);
	ScintillaBridge_sendMessage(sci, SCI_SETSEL,
		static_cast<uintptr_t>(targetPos), targetPos);
	ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);
	focusFunctionListEditor(sci);
}

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
	static const NSInteger kLineNumberTag = 100;

	NSTableCellView* cell = [outlineView makeViewWithIdentifier:@"FunctionListCell" owner:self];
	if (!cell)
	{
		cell = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 160, 20)];

		NSTextField* text = [NSTextField labelWithString:@""];
		text.translatesAutoresizingMaskIntoConstraints = NO;
		text.lineBreakMode = NSLineBreakByTruncatingTail;
		[text setContentHuggingPriority:NSLayoutPriorityDefaultLow
		                 forOrientation:NSLayoutConstraintOrientationHorizontal];
		[text setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow
		                              forOrientation:NSLayoutConstraintOrientationHorizontal];
		[cell addSubview:text];
		cell.textField = text;

		NSTextField* lineLabel = [NSTextField labelWithString:@""];
		lineLabel.translatesAutoresizingMaskIntoConstraints = NO;
		lineLabel.alignment = NSTextAlignmentRight;
		lineLabel.textColor = NSColor.secondaryLabelColor;
		lineLabel.lineBreakMode = NSLineBreakByClipping;
		lineLabel.tag = kLineNumberTag;
		[lineLabel setContentHuggingPriority:NSLayoutPriorityRequired
		                      forOrientation:NSLayoutConstraintOrientationHorizontal];
		[lineLabel setContentCompressionResistancePriority:NSLayoutPriorityRequired
		                                   forOrientation:NSLayoutConstraintOrientationHorizontal];
		[cell addSubview:lineLabel];

		cell.identifier = @"FunctionListCell";
		[NSLayoutConstraint activateConstraints:@[
			[text.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor constant:4],
			[text.trailingAnchor constraintEqualToAnchor:lineLabel.leadingAnchor constant:-4],
			[text.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor],
			[lineLabel.trailingAnchor constraintEqualToAnchor:cell.trailingAnchor constant:-4],
			[lineLabel.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor],
			[lineLabel.widthAnchor constraintGreaterThanOrEqualToConstant:56]
		]];
	}

	FunctionListItem* flItem = (FunctionListItem*)item;
	cell.textField.stringValue = flItem.title ?: @"";
	if (flItem.container)
		cell.textField.font = [NSFont boldSystemFontOfSize:[NSFont systemFontSize]];
	else
		cell.textField.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];

	NSTextField* lineLabel = [cell viewWithTag:kLineNumberTag];
	CGFloat fontSize = flItem.container ? [NSFont systemFontSize] : [NSFont smallSystemFontSize];
	lineLabel.font = [NSFont monospacedDigitSystemFontOfSize:fontSize weight:NSFontWeightRegular];
	lineLabel.stringValue = flItem.line > 0
		? [NSString stringWithFormat:@"%d", flItem.line]
		: @"";
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

	activateFunctionListItem(item);
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

static void applyParsedNodes(const std::vector<FunctionListNode>& nodes);

struct FunctionListCacheEntry
{
	uint64_t revision = 0;
	int languageIndex = -1;
	std::vector<FunctionListNode> nodes;
};

static std::unordered_map<uint64_t, FunctionListCacheEntry> sFunctionListCache;

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

static bool fetchActiveDocumentMetadata(uint64_t& outDocumentId, uint64_t& outRevision, int& outLangIndex)
{
	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	FLLOG("fetchMeta: tabIdx=%d docCount=%d", tabIdx, static_cast<int>(docs.size()));
	if (tabIdx < 0 || tabIdx >= static_cast<int>(docs.size()))
	{
		FLLOG("fetchMeta: BAIL — tabIdx out of range");
		return false;
	}

	auto& doc = docs[tabIdx];
	if (doc.functionListDocumentId == 0)
		doc.functionListDocumentId = allocateFunctionListDocumentId();

	outDocumentId = doc.functionListDocumentId;
	outRevision = doc.functionListRevision;
	outLangIndex = doc.languageIndex;
	FLLOG("fetchMeta: docId=%llu rev=%llu lang=%d",
	      static_cast<unsigned long long>(outDocumentId),
	      static_cast<unsigned long long>(outRevision),
	      outLangIndex);
	return true;
}

static bool applyCachedNodesIfFresh(uint64_t documentId, uint64_t revision, int langIndex)
{
	const auto it = sFunctionListCache.find(documentId);
	if (it == sFunctionListCache.end())
		return false;

	const auto& entry = it->second;
	if (entry.revision != revision || entry.languageIndex != langIndex)
	{
		FLLOG("cache: STALE docId=%llu rev=%llu/%llu lang=%d/%d",
		      static_cast<unsigned long long>(documentId),
		      static_cast<unsigned long long>(revision),
		      static_cast<unsigned long long>(entry.revision),
		      langIndex, entry.languageIndex);
		return false;
	}

	FLLOG("cache: HIT docId=%llu rev=%llu lang=%d nodes=%zu",
	      static_cast<unsigned long long>(documentId),
	      static_cast<unsigned long long>(revision),
	      langIndex, entry.nodes.size());
	applyParsedNodes(entry.nodes);
	return true;
}

static bool applyCachedNodesForActiveDocument()
{
	uint64_t documentId = 0;
	uint64_t revision = 0;
	int langIndex = 0;
	if (!fetchActiveDocumentMetadata(documentId, revision, langIndex))
		return false;

	return applyCachedNodesIfFresh(documentId, revision, langIndex);
}

static void cacheParsedNodes(uint64_t documentId, uint64_t revision, int langIndex,
                             std::vector<FunctionListNode> nodes)
{
	if (documentId == 0)
		return;

	FunctionListCacheEntry entry;
	entry.revision = revision;
	entry.languageIndex = langIndex;
	entry.nodes = std::move(nodes);
	sFunctionListCache[documentId] = std::move(entry);
	FLLOG("cache: STORE docId=%llu rev=%llu lang=%d",
	      static_cast<unsigned long long>(documentId),
	      static_cast<unsigned long long>(revision),
	      langIndex);
}

static bool fetchActiveDocumentText(std::string& outText, uint64_t& outDocumentId,
                                    uint64_t& outRevision, int& outLangIndex)
{
	if (!fetchActiveDocumentMetadata(outDocumentId, outRevision, outLangIndex))
		return false;

	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	outText = docs[tabIdx].content;
	FLLOG("fetchText: docId=%llu rev=%llu lang=%d contentLen=%zu",
	      static_cast<unsigned long long>(outDocumentId),
	      static_cast<unsigned long long>(outRevision),
	      outLangIndex, outText.size());
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
	FLLOG("fetchText: returning %s (docId=%llu rev=%llu textLen=%zu)",
	      ok ? "true" : "false",
	      static_cast<unsigned long long>(outDocumentId),
	      static_cast<unsigned long long>(outRevision),
	      outText.size());
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
	sFunctionListCache.clear();

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

void* functionListContainerView()
{
	return (__bridge void*)sContainer;
}

void setFunctionListEnabled(bool enabled)
{
	ctx().functionListEnabled = enabled;
	if (enabled && !sContainer)
		initializeFunctionListPanel();
	relayoutPanels();
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
	if (!ctx().functionListEnabled)
		return;

	// Tab/view switches should restore a fresh cached outline immediately.
	if (applyCachedNodesForActiveDocument())
	{
		invalidateFunctionListPendingRefresh();
		return;
	}

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

	uint64_t documentId = 0;
	uint64_t revision = 0;
	int langIndex = 0;
	if (!fetchActiveDocumentMetadata(documentId, revision, langIndex))
	{
		FLLOG("updateNow: BAIL — fetchMeta returned false");
		clearPanelData();
		return;
	}

	if (applyCachedNodesIfFresh(documentId, revision, langIndex))
		return;

	std::string text;
	if (!fetchActiveDocumentText(text, documentId, revision, langIndex))
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
		const uint64_t parseDocumentId = documentId;
		const uint64_t parseRevision = revision;
		const int parseLangIndex = langIndex;
		FLLOG("updateNow: ASYNC parse, generation=%d docId=%llu rev=%llu",
		      generation,
		      static_cast<unsigned long long>(parseDocumentId),
		      static_cast<unsigned long long>(parseRevision));
		dispatch_async(parseQueue(), ^{
			int curGen = sRefreshGeneration.load();
			if (generation != curGen)
			{
				FLLOG("asyncParse: STALE (mine=%d current=%d)", generation, curGen);
				return;
			}
			FLLOG("asyncParse: starting parse (docId=%llu rev=%llu textLen=%zu lang=%d)",
			      static_cast<unsigned long long>(parseDocumentId),
			      static_cast<unsigned long long>(parseRevision),
			      text.size(), parseLangIndex);
			std::vector<FunctionListNode> nodes = parseFunctionListNodes(parseLangIndex, text);
			FLLOG("asyncParse: done, %zu nodes", nodes.size());
			dispatch_async(dispatch_get_main_queue(), ^{
				int curGen2 = sRefreshGeneration.load();
				if (generation != curGen2)
				{
					FLLOG("asyncApply: STALE (mine=%d current=%d)", generation, curGen2);
					return;
				}

				uint64_t currentDocumentId = 0;
				uint64_t currentRevision = 0;
				int currentLangIndex = 0;
				if (!fetchActiveDocumentMetadata(currentDocumentId, currentRevision, currentLangIndex))
				{
					FLLOG("asyncApply: BAIL — fetchMeta failed");
					return;
				}
				if (currentDocumentId != parseDocumentId
				    || currentRevision != parseRevision
				    || currentLangIndex != parseLangIndex)
				{
					FLLOG("asyncApply: STALE doc state mine(doc=%llu rev=%llu lang=%d) current(doc=%llu rev=%llu lang=%d)",
					      static_cast<unsigned long long>(parseDocumentId),
					      static_cast<unsigned long long>(parseRevision),
					      parseLangIndex,
					      static_cast<unsigned long long>(currentDocumentId),
					      static_cast<unsigned long long>(currentRevision),
					      currentLangIndex);
					return;
				}

				FLLOG("asyncApply: applying %zu nodes for docId=%llu rev=%llu", nodes.size(),
				      static_cast<unsigned long long>(parseDocumentId),
				      static_cast<unsigned long long>(parseRevision));
				applyParsedNodes(nodes);
				cacheParsedNodes(parseDocumentId, parseRevision, parseLangIndex, nodes);
			});
		});
	}
	else
	{
		FLLOG("updateNow: SYNC parse");
		std::vector<FunctionListNode> nodes = parseFunctionListNodes(langIndex, text);
		applyParsedNodes(nodes);
		cacheParsedNodes(documentId, revision, langIndex, std::move(nodes));
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

void invalidateFunctionListCacheForDocument(uint64_t documentId)
{
	if (documentId == 0)
		return;

	sFunctionListCache.erase(documentId);
	FLLOG("cache: INVALIDATE docId=%llu", static_cast<unsigned long long>(documentId));
}
