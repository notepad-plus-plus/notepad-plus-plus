// file_path_ops.mm — Reveal in Finder and copy-path commands
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "file_path_ops.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "windows.h"

NSString* activeDocumentPath()
{
	auto& docs = ctx().activeDocuments();
	int idx = ctx().activeTabIndex();
	if (idx < 0 || idx >= static_cast<int>(docs.size()))
		return nil;
	const std::wstring& fp = docs[idx].filePath;
	if (fp.empty())
		return nil;
	return WideToNSString(fp.c_str());
}

bool hasActiveFilePath()
{
	auto& docs = ctx().activeDocuments();
	int idx = ctx().activeTabIndex();
	if (idx < 0 || idx >= static_cast<int>(docs.size()))
		return false;
	return !docs[idx].filePath.empty();
}

static void copyStringToClipboard(NSString* s)
{
	NSPasteboard* pb = [NSPasteboard generalPasteboard];
	[pb clearContents];
	[pb setString:s forType:NSPasteboardTypeString];
}

void doRevealInFinder()
{
	NSString* path = activeDocumentPath();
	if (!path) return;
	[[NSWorkspace sharedWorkspace] selectFile:path inFileViewerRootedAtPath:@""];
}

void doCopyFullPath()
{
	NSString* path = activeDocumentPath();
	if (!path) return;
	copyStringToClipboard(path);
}

void doCopyFilename()
{
	NSString* path = activeDocumentPath();
	if (!path) return;
	copyStringToClipboard([path lastPathComponent]);
}

void doCopyDirPath()
{
	NSString* path = activeDocumentPath();
	if (!path) return;
	copyStringToClipboard([path stringByDeletingLastPathComponent]);
}

void updateFilePathMenuState()
{
	HMENU hMenu = GetMenu(ctx().mainHwnd);
	if (!hMenu) return;

	UINT flag = hasActiveFilePath() ? MF_ENABLED : MF_GRAYED;
	EnableMenuItem(hMenu, IDM_FILE_REVEAL_FINDER,  MF_BYCOMMAND | flag);
	EnableMenuItem(hMenu, IDM_FILE_COPY_FULL_PATH, MF_BYCOMMAND | flag);
	EnableMenuItem(hMenu, IDM_FILE_COPY_FILENAME,  MF_BYCOMMAND | flag);
	EnableMenuItem(hMenu, IDM_FILE_COPY_DIR_PATH,  MF_BYCOMMAND | flag);
}
