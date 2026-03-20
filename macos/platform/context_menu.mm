// context_menu.mm — Context menu
// Part of the Notepad++ macOS port modular refactor.

#include "context_menu.h"
#include "npp_constants.h"
#include "app_state.h"
#include "file_path_ops.h"

void showContextMenu(NSPoint screenPoint)
{
	NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@"Context"];

	NSMenuItem* undoItem = [[NSMenuItem alloc] initWithTitle:@"Undo" action:@selector(performContextAction:) keyEquivalent:@""];
	undoItem.tag = IDM_EDIT_UNDO;

	NSMenuItem* redoItem = [[NSMenuItem alloc] initWithTitle:@"Redo" action:@selector(performContextAction:) keyEquivalent:@""];
	redoItem.tag = IDM_EDIT_REDO;

	NSMenuItem* cutItem = [[NSMenuItem alloc] initWithTitle:@"Cut" action:@selector(performContextAction:) keyEquivalent:@""];
	cutItem.tag = IDM_EDIT_CUT;

	NSMenuItem* copyItem = [[NSMenuItem alloc] initWithTitle:@"Copy" action:@selector(performContextAction:) keyEquivalent:@""];
	copyItem.tag = IDM_EDIT_COPY;

	NSMenuItem* pasteItem = [[NSMenuItem alloc] initWithTitle:@"Paste" action:@selector(performContextAction:) keyEquivalent:@""];
	pasteItem.tag = IDM_EDIT_PASTE;

	NSMenuItem* selectAllItem = [[NSMenuItem alloc] initWithTitle:@"Select All" action:@selector(performContextAction:) keyEquivalent:@""];
	selectAllItem.tag = IDM_EDIT_SELECTALL;

	NSMenuItem* bookmarkItem = [[NSMenuItem alloc] initWithTitle:@"Toggle Bookmark" action:@selector(performContextAction:) keyEquivalent:@""];
	bookmarkItem.tag = IDM_SEARCH_BOOKMARK_TOGGLE;

	[contextMenu addItem:undoItem];
	[contextMenu addItem:redoItem];
	[contextMenu addItem:[NSMenuItem separatorItem]];
	[contextMenu addItem:cutItem];
	[contextMenu addItem:copyItem];
	[contextMenu addItem:pasteItem];
	[contextMenu addItem:[NSMenuItem separatorItem]];
	[contextMenu addItem:selectAllItem];
	[contextMenu addItem:[NSMenuItem separatorItem]];
	[contextMenu addItem:bookmarkItem];

	if (hasActiveFilePath())
	{
		[contextMenu addItem:[NSMenuItem separatorItem]];

		NSMenuItem* revealItem = [[NSMenuItem alloc] initWithTitle:@"Reveal in Finder" action:@selector(performContextAction:) keyEquivalent:@""];
		revealItem.tag = IDM_FILE_REVEAL_FINDER;
		[contextMenu addItem:revealItem];

		NSMenuItem* copyPathItem = [[NSMenuItem alloc] initWithTitle:@"Copy Full Path" action:@selector(performContextAction:) keyEquivalent:@""];
		copyPathItem.tag = IDM_FILE_COPY_FULL_PATH;
		[contextMenu addItem:copyPathItem];
	}

	[NSMenu popUpContextMenu:contextMenu withEvent:[NSApp currentEvent] forView:ctx().mainWindow.contentView];
}
