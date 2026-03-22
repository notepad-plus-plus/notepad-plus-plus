// toolbar.mm — NSToolbar with SF Symbols
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "toolbar.h"
#include "npp_constants.h"
#include "app_state.h"
#include "windows.h"

static NSString* const kToolbarIdentifier = @"MacNotePPToolbar";

static NSString* const kNewItem      = @"ToolbarNew";
static NSString* const kOpenItem     = @"ToolbarOpen";
static NSString* const kSaveItem     = @"ToolbarSave";
static NSString* const kUndoItem     = @"ToolbarUndo";
static NSString* const kRedoItem     = @"ToolbarRedo";
static NSString* const kFindItem     = @"ToolbarFind";
static NSString* const kZoomInItem   = @"ToolbarZoomIn";
static NSString* const kZoomOutItem  = @"ToolbarZoomOut";

struct ToolbarItemDef
{
	NSString* identifier;
	NSString* label;
	NSString* sfSymbol;
	int commandId;
};

static const ToolbarItemDef s_items[] = {
	{kNewItem,     @"New",      @"doc.badge.plus",            IDM_FILE_NEW},
	{kOpenItem,    @"Open",     @"folder",                    IDM_FILE_OPEN},
	{kSaveItem,    @"Save",     @"square.and.arrow.down",     IDM_FILE_SAVE},
	{kUndoItem,    @"Undo",     @"arrow.uturn.backward",      IDM_EDIT_UNDO},
	{kRedoItem,    @"Redo",     @"arrow.uturn.forward",       IDM_EDIT_REDO},
	{kFindItem,    @"Find",     @"magnifyingglass",           IDM_SEARCH_FIND},
	{kZoomInItem,  @"Zoom In",  @"plus.magnifyingglass",      IDM_VIEW_ZOOMIN},
	{kZoomOutItem, @"Zoom Out", @"minus.magnifyingglass",     IDM_VIEW_ZOOMOUT},
};

static const ToolbarItemDef* findItemDef(NSString* identifier)
{
	for (const auto& item : s_items)
	{
		if ([item.identifier isEqualToString:identifier])
			return &item;
	}
	return nullptr;
}

@interface NppToolbarDelegate : NSObject <NSToolbarDelegate>
@end

@implementation NppToolbarDelegate

- (NSToolbarItem*)toolbar:(NSToolbar*)toolbar
    itemForItemIdentifier:(NSToolbarItemIdentifier)itemIdentifier
    willBeInsertedIntoToolbar:(BOOL)flag
{
	const ToolbarItemDef* def = findItemDef(itemIdentifier);
	if (!def)
		return nil;

	NSToolbarItem* item = [[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
	[item setLabel:def->label];
	[item setToolTip:def->label];
	[item setTarget:self];
	[item setAction:@selector(toolbarItemClicked:)];
	[item setTag:def->commandId];

	NSImage* image = nil;
	if (@available(macOS 11.0, *))
		image = [NSImage imageWithSystemSymbolName:def->sfSymbol accessibilityDescription:def->label];
	if (!image)
		image = [NSImage imageNamed:NSImageNameActionTemplate];
	[item setImage:image];

	return item;
}

- (NSArray<NSToolbarItemIdentifier>*)toolbarAllowedItemIdentifiers:(NSToolbar*)toolbar
{
	return @[
		kNewItem, kOpenItem, kSaveItem,
		NSToolbarFlexibleSpaceItemIdentifier,
		kUndoItem, kRedoItem,
		NSToolbarSpaceItemIdentifier,
		kFindItem,
		NSToolbarSpaceItemIdentifier,
		kZoomInItem, kZoomOutItem,
	];
}

- (NSArray<NSToolbarItemIdentifier>*)toolbarDefaultItemIdentifiers:(NSToolbar*)toolbar
{
	return @[
		kNewItem, kOpenItem, kSaveItem,
		NSToolbarFlexibleSpaceItemIdentifier,
		kUndoItem, kRedoItem,
		NSToolbarSpaceItemIdentifier,
		kFindItem,
		NSToolbarFlexibleSpaceItemIdentifier,
		kZoomInItem, kZoomOutItem,
	];
}

- (void)toolbarItemClicked:(NSToolbarItem*)sender
{
	SendMessageW(ctx().mainHwnd, WM_COMMAND, static_cast<WPARAM>(sender.tag), 0);
}

@end

// Prevent delegate from being deallocated
static NppToolbarDelegate* s_toolbarDelegate = nil;

void setupToolbar(NSWindow* window)
{
	if (!window) return;

	s_toolbarDelegate = [[NppToolbarDelegate alloc] init];

	NSToolbar* toolbar = [[NSToolbar alloc] initWithIdentifier:kToolbarIdentifier];
	[toolbar setDelegate:s_toolbarDelegate];
	[toolbar setDisplayMode:NSToolbarDisplayModeIconOnly];
	[toolbar setAllowsUserCustomization:YES];
	[toolbar setAutosavesConfiguration:YES];

	[window setToolbar:toolbar];
}
