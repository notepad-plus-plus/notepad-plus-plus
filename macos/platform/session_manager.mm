// session_manager.mm — Session save/restore
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "session_manager.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "document_manager.h"
#include "file_operations.h"
#include "split_view.h"
#include "sync_scroll.h"
#include "scintilla_bridge.h"
#include "windows.h"
#include "commctrl.h"

std::string sessionPath()
{
	NSString* home = NSHomeDirectory();
	return std::string([home UTF8String]) + "/.npp-macos/session.json";
}

void saveSession()
{
	NSString* dir = [NSString stringWithFormat:@"%@/.npp-macos", NSHomeDirectory()];
	[[NSFileManager defaultManager] createDirectoryAtPath:dir
	                          withIntermediateDirectories:YES attributes:nil error:nil];

	saveScintillaState();
	if (ctx().isSplit && ctx().scintillaView2)
		saveViewState(ctx().scintillaView2, ctx().documents2, ctx().activeTab2);

	NSMutableArray* tabs = [NSMutableArray array];
	for (const auto& doc : ctx().documents)
	{
		if (doc.filePath.empty()) continue;
		NSString* fp = WideToNSString(doc.filePath.c_str());
		if (!fp) continue;
		[tabs addObject:@{
			@"filePath": fp,
			@"cursorPos": @((long long)doc.cursorPos),
			@"anchorPos": @((long long)doc.anchorPos),
			@"firstVisibleLine": @((long long)doc.firstVisibleLine),
			@"languageIndex": @(doc.languageIndex),
			@"encoding": @(doc.encoding),
			@"eolMode": @(doc.eolMode),
			@"zoomLevel": @(doc.zoomLevel),
		}];
	}

	NSMutableArray* tabs2 = [NSMutableArray array];
	if (ctx().isSplit)
	{
		for (const auto& doc : ctx().documents2)
		{
			if (doc.filePath.empty()) continue;
			NSString* fp = WideToNSString(doc.filePath.c_str());
			if (!fp) continue;
			[tabs2 addObject:@{
				@"filePath": fp,
				@"cursorPos": @((long long)doc.cursorPos),
				@"anchorPos": @((long long)doc.anchorPos),
				@"firstVisibleLine": @((long long)doc.firstVisibleLine),
				@"languageIndex": @(doc.languageIndex),
				@"encoding": @(doc.encoding),
				@"eolMode": @(doc.eolMode),
				@"zoomLevel": @(doc.zoomLevel),
			}];
		}
	}

	NSDictionary* session = @{
		@"version": @1,
		@"tabs": tabs,
		@"tabs2": tabs2,
		@"activeTab": @(ctx().activeTab),
		@"activeTab2": @(ctx().activeTab2),
		@"isSplit": @(ctx().isSplit),
	};

	NSData* data = [NSJSONSerialization dataWithJSONObject:session
	                                              options:NSJSONWritingPrettyPrinted error:nil];
	if (data)
	{
		NSString* path = [NSString stringWithUTF8String:sessionPath().c_str()];
		[data writeToFile:path atomically:YES];
	}
}

void restoreSession()
{
	NSString* path = [NSString stringWithUTF8String:sessionPath().c_str()];
	NSData* data = [NSData dataWithContentsOfFile:path];
	if (!data) return;

	NSDictionary* session = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
	if (!session) return;

	NSArray* tabs = session[@"tabs"];
	if (![tabs isKindOfClass:[NSArray class]] || tabs.count == 0) return;

	bool anyOpened = false;
	for (NSDictionary* tab in tabs)
	{
		NSString* fp = tab[@"filePath"];
		if (!fp || ![[NSFileManager defaultManager] fileExistsAtPath:fp]) continue;
		if (openFileAtPath(fp))
		{
			anyOpened = true;
			int idx = static_cast<int>(ctx().documents.size()) - 1;
			if (idx >= 0)
			{
				if (tab[@"cursorPos"])
					ctx().documents[idx].cursorPos = [tab[@"cursorPos"] longLongValue];
				if (tab[@"anchorPos"])
					ctx().documents[idx].anchorPos = [tab[@"anchorPos"] longLongValue];
				if (tab[@"firstVisibleLine"])
					ctx().documents[idx].firstVisibleLine = [tab[@"firstVisibleLine"] longLongValue];
				if (tab[@"languageIndex"])
					ctx().documents[idx].languageIndex = [tab[@"languageIndex"] intValue];
				if (tab[@"encoding"])
					ctx().documents[idx].encoding = [tab[@"encoding"] intValue];
				if (tab[@"eolMode"])
					ctx().documents[idx].eolMode = [tab[@"eolMode"] intValue];
				if (tab[@"zoomLevel"])
					ctx().documents[idx].zoomLevel = [tab[@"zoomLevel"] intValue];
			}
		}
	}

	if (anyOpened)
	{
		if (ctx().documents.size() > 1 && ctx().documents[0].filePath.empty() && ctx().documents[0].title == L"Welcome")
			closeTabFromView(0, 0);

		int savedActive = [session[@"activeTab"] intValue];
		if (savedActive >= 0 && savedActive < static_cast<int>(ctx().documents.size()))
		{
			switchToTabInView(0, savedActive);
			const auto& doc = ctx().documents[savedActive];
			ScintillaBridge_sendMessage(ctx().scintillaView, SCI_SETFIRSTVISIBLELINE, doc.firstVisibleLine, 0);
			ScintillaBridge_sendMessage(ctx().scintillaView, SCI_SETSEL, doc.anchorPos, doc.cursorPos);
		}

		bool wasSplit = [session[@"isSplit"] boolValue];
		NSArray* tabs2 = session[@"tabs2"];
		if (wasSplit && [tabs2 isKindOfClass:[NSArray class]] && tabs2.count > 0)
		{
			doSplit();
			if (ctx().isSplit && ctx().scintillaView2)
			{
				ctx().documents2.clear();
				while (SendMessageW(ctx().tabHwnd2, TCM_GETITEMCOUNT, 0, 0) > 0)
					SendMessageW(ctx().tabHwnd2, TCM_DELETEITEM, 0, 0);

				ctx().activeView = 1;
				for (NSDictionary* tab in tabs2)
				{
					NSString* fp = tab[@"filePath"];
					if (!fp || ![[NSFileManager defaultManager] fileExistsAtPath:fp]) continue;
					openFileAtPath(fp);
					int idx = static_cast<int>(ctx().documents2.size()) - 1;
					if (idx >= 0)
					{
						if (tab[@"cursorPos"])
							ctx().documents2[idx].cursorPos = [tab[@"cursorPos"] longLongValue];
						if (tab[@"anchorPos"])
							ctx().documents2[idx].anchorPos = [tab[@"anchorPos"] longLongValue];
						if (tab[@"firstVisibleLine"])
							ctx().documents2[idx].firstVisibleLine = [tab[@"firstVisibleLine"] longLongValue];
						if (tab[@"languageIndex"])
							ctx().documents2[idx].languageIndex = [tab[@"languageIndex"] intValue];
						if (tab[@"encoding"])
							ctx().documents2[idx].encoding = [tab[@"encoding"] intValue];
						if (tab[@"eolMode"])
							ctx().documents2[idx].eolMode = [tab[@"eolMode"] intValue];
						if (tab[@"zoomLevel"])
							ctx().documents2[idx].zoomLevel = [tab[@"zoomLevel"] intValue];
					}
				}

				int savedActive2 = [session[@"activeTab2"] intValue];
				if (savedActive2 >= 0 && savedActive2 < static_cast<int>(ctx().documents2.size()))
				{
					switchToTabInView(1, savedActive2);
					const auto& doc2 = ctx().documents2[savedActive2];
					ScintillaBridge_sendMessage(ctx().scintillaView2, SCI_SETFIRSTVISIBLELINE, doc2.firstVisibleLine, 0);
					ScintillaBridge_sendMessage(ctx().scintillaView2, SCI_SETSEL, doc2.anchorPos, doc2.cursorPos);
				}

				ctx().activeView = 0;
				refreshSyncScrollAnchor();
			}
		}
		else if (wasSplit && ctx().documents.size() > 0)
		{
			doSplit();
			refreshSyncScrollAnchor();
		}
	}
}
