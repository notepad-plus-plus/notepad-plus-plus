// save_prompt.mm — Save-before-close prompts
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "save_prompt.h"
#include "app_state.h"
#include "string_utils.h"
#include "document_manager.h"
#include "file_operations.h"
#include "scintilla_bridge.h"
#include "npp_constants.h"

// Re-entry guard to prevent double prompting
static bool s_promptInProgress = false;

SavePromptResult promptSaveSingleDocument(const std::wstring& title)
{
	@autoreleasepool {
		NSAlert* alert = [[NSAlert alloc] init];
		NSString* nsTitle = WideToNSString(title.c_str());
		[alert setMessageText:[NSString stringWithFormat:@"Do you want to save changes to \"%@\"?", nsTitle]];
		[alert setInformativeText:@"Your changes will be lost if you don't save them."];
		[alert addButtonWithTitle:@"Save"];
		[alert addButtonWithTitle:@"Don't Save"];
		[alert addButtonWithTitle:@"Cancel"];
		[alert setAlertStyle:NSAlertStyleWarning];

		NSModalResponse response = [alert runModal];
		if (response == NSAlertFirstButtonReturn)
			return SavePromptResult::Save;
		if (response == NSAlertSecondButtonReturn)
			return SavePromptResult::DontSave;
		return SavePromptResult::Cancel;
	}
}

bool isDocumentDirty(int viewIndex, int tabIndex)
{
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;
	void* sci = (viewIndex == 0) ? ctx().scintillaView : ctx().scintillaView2;
	int activeTab = (viewIndex == 0) ? ctx().activeTab : ctx().activeTab2;

	if (tabIndex < 0 || tabIndex >= static_cast<int>(docs.size()))
		return false;

	// If this is the currently active tab, sync state from Scintilla
	if (tabIndex == activeTab && sci && docs[tabIndex].savePointValid)
		docs[tabIndex].modified = ScintillaBridge_sendMessage(sci, SCI_GETMODIFY, 0, 0) != 0;

	// Untitled buffers with content are dirty even if not "modified"
	if (docs[tabIndex].filePath.empty() && !docs[tabIndex].content.empty() && !docs[tabIndex].modified)
	{
		// Check if there's actually content in Scintilla
		if (tabIndex == activeTab && sci)
		{
			intptr_t len = ScintillaBridge_sendMessage(sci, SCI_GETTEXTLENGTH, 0, 0);
			if (len > 0)
				return true;
		}
	}

	return docs[tabIndex].modified;
}

bool saveDocumentAt(int viewIndex, int tabIndex)
{
	int origView = ctx().activeView;
	int origTab = ctx().activeTabIndex();

	// Switch to the target view/tab if not already active
	bool needSwitch = (viewIndex != origView || tabIndex != ctx().activeTabIndex());
	if (needSwitch)
	{
		ctx().activeView = viewIndex;
		if (viewIndex == 0)
		{
			if (tabIndex != ctx().activeTab)
				switchToTabInView(0, tabIndex);
		}
		else
		{
			if (tabIndex != ctx().activeTab2)
				switchToTabInView(1, tabIndex);
		}
	}

	auto& docs = ctx().activeDocuments();
	std::wstring pathBefore = docs[ctx().activeTabIndex()].filePath;

	saveCurrentFile();

	auto& docsAfter = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	bool saved = false;
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docsAfter.size()))
	{
		// Save succeeded if: file has a path AND is no longer modified
		void* sci = ctx().activeScintillaView();
		saved = !docsAfter[tabIdx].filePath.empty() &&
		        (sci ? ScintillaBridge_sendMessage(sci, SCI_GETMODIFY, 0, 0) == 0 : !docsAfter[tabIdx].modified);
	}

	// Restore original context if we switched
	if (needSwitch)
	{
		ctx().activeView = origView;
	}

	return saved;
}

bool promptAndHandleClose(int viewIndex, int tabIndex)
{
	if (!isDocumentDirty(viewIndex, tabIndex))
		return true;

	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;
	SavePromptResult result = promptSaveSingleDocument(docs[tabIndex].title);

	switch (result)
	{
		case SavePromptResult::Save:
			return saveDocumentAt(viewIndex, tabIndex);
		case SavePromptResult::DontSave:
			return true;
		case SavePromptResult::Cancel:
			return false;
	}
	return false;
}

bool promptAndHandleCloseAll(int viewIndex)
{
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;

	// Snapshot dirty tab indices (iterate in reverse to avoid index shifting issues)
	std::vector<int> dirtyTabs;
	for (int i = 0; i < static_cast<int>(docs.size()); ++i)
	{
		if (isDocumentDirty(viewIndex, i))
			dirtyTabs.push_back(i);
	}

	for (int idx : dirtyTabs)
	{
		// Re-validate index (docs may have changed)
		if (idx >= static_cast<int>(docs.size()))
			continue;

		SavePromptResult result = promptSaveSingleDocument(docs[idx].title);
		switch (result)
		{
			case SavePromptResult::Save:
				if (!saveDocumentAt(viewIndex, idx))
					return false; // Save As cancelled — abort entire close-all
				break;
			case SavePromptResult::DontSave:
				break;
			case SavePromptResult::Cancel:
				return false;
		}
	}
	return true;
}

bool promptAndHandleQuit()
{
	if (s_promptInProgress)
		return false;
	s_promptInProgress = true;

	// Collect all dirty docs across both views
	struct DirtyDoc {
		int viewIndex;
		int tabIndex;
		std::wstring title;
	};
	std::vector<DirtyDoc> dirtyDocs;

	for (int i = 0; i < static_cast<int>(ctx().documents.size()); ++i)
	{
		if (isDocumentDirty(0, i))
			dirtyDocs.push_back({0, i, ctx().documents[i].title});
	}
	if (ctx().isSplit)
	{
		for (int i = 0; i < static_cast<int>(ctx().documents2.size()); ++i)
		{
			if (isDocumentDirty(1, i))
			{
				// Dedupe by file path — if same file open in both views, only prompt once
				bool isDupe = false;
				if (!ctx().documents2[i].filePath.empty())
				{
					for (const auto& dd : dirtyDocs)
					{
						auto& ddDocs = (dd.viewIndex == 0) ? ctx().documents : ctx().documents2;
						if (dd.tabIndex < static_cast<int>(ddDocs.size()) &&
						    ddDocs[dd.tabIndex].filePath == ctx().documents2[i].filePath)
						{
							isDupe = true;
							break;
						}
					}
				}
				if (!isDupe)
					dirtyDocs.push_back({1, i, ctx().documents2[i].title});
			}
		}
	}

	if (dirtyDocs.empty())
	{
		s_promptInProgress = false;
		return true;
	}

	// Prompt for each dirty document
	for (const auto& dd : dirtyDocs)
	{
		auto& docs = (dd.viewIndex == 0) ? ctx().documents : ctx().documents2;
		if (dd.tabIndex >= static_cast<int>(docs.size()))
			continue;

		SavePromptResult result = promptSaveSingleDocument(dd.title);
		switch (result)
		{
			case SavePromptResult::Save:
				if (!saveDocumentAt(dd.viewIndex, dd.tabIndex))
				{
					s_promptInProgress = false;
					return false;
				}
				break;
			case SavePromptResult::DontSave:
				break;
			case SavePromptResult::Cancel:
				s_promptInProgress = false;
				return false;
		}
	}

	s_promptInProgress = false;
	return true;
}
