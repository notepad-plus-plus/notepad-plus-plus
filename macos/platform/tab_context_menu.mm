// tab_context_menu.mm — Tab bar context menu (right-click)
// Part of the Notepad++ macOS port.

#import <Cocoa/Cocoa.h>
#include "tab_context_menu.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "document_manager.h"
#include "save_prompt.h"
#include "file_path_ops.h"
#include "handle_registry.h"

// Action context stored per menu invocation
struct TabActionContext
{
	int viewIndex;
	int tabIndex;
};

static TabActionContext s_tabActionCtx;

// ============================================================
// ObjC target for tab context menu actions
// ============================================================

@interface NppTabContextTarget : NSObject
- (void)menuAction:(NSMenuItem*)sender;
@end

@implementation NppTabContextTarget

- (void)menuAction:(NSMenuItem*)sender
{
	int viewIndex = s_tabActionCtx.viewIndex;
	int tabIndex = s_tabActionCtx.tabIndex;
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;

	switch (sender.tag)
	{
		case IDM_TAB_CLOSE:
		{
			if (promptAndHandleClose(viewIndex, tabIndex))
				closeTabFromView(viewIndex, tabIndex);
			break;
		}
		case IDM_TAB_CLOSE_OTHERS:
		{
			// Close all tabs except the target one
			// Iterate from end to avoid index shifting
			for (int i = static_cast<int>(docs.size()) - 1; i >= 0; --i)
			{
				if (i == tabIndex)
					continue;
				if (promptAndHandleClose(viewIndex, i))
				{
					closeTabFromView(viewIndex, i);
					// Adjust tabIndex if a tab before it was removed
					if (i < tabIndex)
						--tabIndex;
				}
				else
				{
					break; // Cancel stops the entire operation
				}
			}
			break;
		}
		case IDM_TAB_CLOSE_ALL:
		{
			if (!promptAndHandleCloseAll(viewIndex))
				break;
			while (docs.size() > 1)
				closeTabFromView(viewIndex, static_cast<int>(docs.size()) - 1);
			closeTabFromView(viewIndex, 0);
			break;
		}
		case IDM_TAB_CLOSE_TO_RIGHT:
		{
			for (int i = static_cast<int>(docs.size()) - 1; i > tabIndex; --i)
			{
				if (promptAndHandleClose(viewIndex, i))
					closeTabFromView(viewIndex, i);
				else
					break;
			}
			break;
		}
		case IDM_TAB_COPY_FULL_PATH:
		{
			if (tabIndex >= 0 && tabIndex < static_cast<int>(docs.size()) && !docs[tabIndex].filePath.empty())
			{
				NSString* path = WideToNSString(docs[tabIndex].filePath.c_str());
				NSPasteboard* pb = [NSPasteboard generalPasteboard];
				[pb clearContents];
				[pb setString:path forType:NSPasteboardTypeString];
			}
			break;
		}
		case IDM_TAB_COPY_FILENAME:
		{
			if (tabIndex >= 0 && tabIndex < static_cast<int>(docs.size()) && !docs[tabIndex].filePath.empty())
			{
				NSString* path = WideToNSString(docs[tabIndex].filePath.c_str());
				NSPasteboard* pb = [NSPasteboard generalPasteboard];
				[pb clearContents];
				[pb setString:[path lastPathComponent] forType:NSPasteboardTypeString];
			}
			break;
		}
		case IDM_TAB_COPY_DIR_PATH:
		{
			if (tabIndex >= 0 && tabIndex < static_cast<int>(docs.size()) && !docs[tabIndex].filePath.empty())
			{
				NSString* path = WideToNSString(docs[tabIndex].filePath.c_str());
				NSPasteboard* pb = [NSPasteboard generalPasteboard];
				[pb clearContents];
				[pb setString:[path stringByDeletingLastPathComponent] forType:NSPasteboardTypeString];
			}
			break;
		}
		case IDM_TAB_REVEAL_FINDER:
		{
			if (tabIndex >= 0 && tabIndex < static_cast<int>(docs.size()) && !docs[tabIndex].filePath.empty())
			{
				NSString* path = WideToNSString(docs[tabIndex].filePath.c_str());
				[[NSWorkspace sharedWorkspace] selectFile:path inFileViewerRootedAtPath:@""];
			}
			break;
		}
	}
}

@end

// Keep the target alive
static NppTabContextTarget* s_tabContextTarget = nil;

void showTabContextMenu(int viewIndex, int tabIndex, NSPoint screenPoint)
{
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;
	if (tabIndex < 0 || tabIndex >= static_cast<int>(docs.size()))
		return;

	s_tabActionCtx = {viewIndex, tabIndex};

	if (!s_tabContextTarget)
		s_tabContextTarget = [[NppTabContextTarget alloc] init];

	NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Tab"];
	menu.autoenablesItems = NO;

	// Close items
	NSMenuItem* closeItem = [[NSMenuItem alloc] initWithTitle:@"Close"
	                                                   action:@selector(menuAction:) keyEquivalent:@""];
	closeItem.target = s_tabContextTarget;
	closeItem.tag = IDM_TAB_CLOSE;
	[menu addItem:closeItem];

	NSMenuItem* closeOthersItem = [[NSMenuItem alloc] initWithTitle:@"Close Others"
	                                                        action:@selector(menuAction:) keyEquivalent:@""];
	closeOthersItem.target = s_tabContextTarget;
	closeOthersItem.tag = IDM_TAB_CLOSE_OTHERS;
	if (docs.size() <= 1)
		closeOthersItem.enabled = NO;
	[menu addItem:closeOthersItem];

	NSMenuItem* closeAllItem = [[NSMenuItem alloc] initWithTitle:@"Close All"
	                                                     action:@selector(menuAction:) keyEquivalent:@""];
	closeAllItem.target = s_tabContextTarget;
	closeAllItem.tag = IDM_TAB_CLOSE_ALL;
	[menu addItem:closeAllItem];

	NSMenuItem* closeRightItem = [[NSMenuItem alloc] initWithTitle:@"Close to the Right"
	                                                       action:@selector(menuAction:) keyEquivalent:@""];
	closeRightItem.target = s_tabContextTarget;
	closeRightItem.tag = IDM_TAB_CLOSE_TO_RIGHT;
	if (tabIndex >= static_cast<int>(docs.size()) - 1)
		closeRightItem.enabled = NO;
	[menu addItem:closeRightItem];

	[menu addItem:[NSMenuItem separatorItem]];

	// File path items
	bool hasPath = !docs[tabIndex].filePath.empty();

	NSMenuItem* copyPathItem = [[NSMenuItem alloc] initWithTitle:@"Copy Full Path"
	                                                     action:@selector(menuAction:) keyEquivalent:@""];
	copyPathItem.target = s_tabContextTarget;
	copyPathItem.tag = IDM_TAB_COPY_FULL_PATH;
	if (!hasPath) copyPathItem.enabled = NO;
	[menu addItem:copyPathItem];

	NSMenuItem* copyFilenameItem = [[NSMenuItem alloc] initWithTitle:@"Copy Filename"
	                                                         action:@selector(menuAction:) keyEquivalent:@""];
	copyFilenameItem.target = s_tabContextTarget;
	copyFilenameItem.tag = IDM_TAB_COPY_FILENAME;
	if (!hasPath) copyFilenameItem.enabled = NO;
	[menu addItem:copyFilenameItem];

	NSMenuItem* copyDirItem = [[NSMenuItem alloc] initWithTitle:@"Copy Directory Path"
	                                                    action:@selector(menuAction:) keyEquivalent:@""];
	copyDirItem.target = s_tabContextTarget;
	copyDirItem.tag = IDM_TAB_COPY_DIR_PATH;
	if (!hasPath) copyDirItem.enabled = NO;
	[menu addItem:copyDirItem];

	[menu addItem:[NSMenuItem separatorItem]];

	NSMenuItem* revealItem = [[NSMenuItem alloc] initWithTitle:@"Reveal in Finder"
	                                                   action:@selector(menuAction:) keyEquivalent:@""];
	revealItem.target = s_tabContextTarget;
	revealItem.tag = IDM_TAB_REVEAL_FINDER;
	if (!hasPath) revealItem.enabled = NO;
	[menu addItem:revealItem];

	// Pop up at the screen point using the tab bar's HWND native view
	HWND tabHwnd = (viewIndex == 0) ? ctx().tabHwnd : ctx().tabHwnd2;
	NSView* anchorView = nil;
	if (tabHwnd)
	{
		auto* tabInfo = HandleRegistry::getWindowInfo(tabHwnd);
		if (tabInfo && tabInfo->nativeView)
			anchorView = (__bridge NSView*)tabInfo->nativeView;
	}

	if (anchorView)
	{
		NSPoint localPoint = [anchorView convertPoint:[anchorView.window convertPointFromScreen:screenPoint] fromView:nil];
		[menu popUpMenuPositioningItem:nil atLocation:localPoint inView:anchorView];
	}
	else
	{
		[NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:ctx().mainWindow.contentView];
	}
}
