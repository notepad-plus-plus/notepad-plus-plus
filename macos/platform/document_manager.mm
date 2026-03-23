// document_manager.mm — Tab/document management, state save/restore
// Part of the Notepad++ macOS port modular refactor.

#include "document_manager.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "lexer_styles.h"
#include "scintilla_bridge.h"
#include "smart_highlight.h"
#include "incremental_search.h"
#include "document_map.h"
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "tab_bar_view.h"
#include "win32_tab_control_impl.h"

void saveViewState(void* sci, std::vector<DocumentData>& docs, int tabIdx)
{
	if (tabIdx < 0 || tabIdx >= static_cast<int>(docs.size()))
		return;
	if (!sci) return;

	auto& doc = docs[tabIdx];
	intptr_t len = ScintillaBridge_sendMessage(sci, SCI_GETTEXTLENGTH, 0, 0);
	if (len >= 0)
	{
		doc.content.resize(len + 1);
		ScintillaBridge_sendMessage(sci, SCI_GETTEXT, len + 1,
		                            (intptr_t)doc.content.data());
		doc.content.resize(len);
	}
	doc.cursorPos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	doc.anchorPos = ScintillaBridge_sendMessage(sci, SCI_GETANCHOR, 0, 0);
	doc.firstVisibleLine = ScintillaBridge_sendMessage(sci, SCI_GETFIRSTVISIBLELINE, 0, 0);
	if (doc.savePointValid)
		doc.modified = ScintillaBridge_sendMessage(sci, SCI_GETMODIFY, 0, 0) != 0;

	doc.bookmarkedLines.clear();
	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
	intptr_t line = 0;
	while (line < lineCount)
	{
		line = ScintillaBridge_sendMessage(sci, SCI_MARKERNEXT, line, BOOKMARK_MASK);
		if (line < 0) break;
		doc.bookmarkedLines.push_back(static_cast<int>(line));
		++line;
	}
}

void saveScintillaState()
{
	saveViewState(ctx().scintillaView, ctx().documents, ctx().activeTab);
}

void restoreViewToScintilla(void* sci, std::vector<DocumentData>& docs, int tabIndex)
{
	if (tabIndex < 0 || tabIndex >= static_cast<int>(docs.size()))
		return;
	if (!sci) return;

	auto& doc = docs[tabIndex];
	ctx().suppressSavePointNotifications = true;
	ScintillaBridge_sendMessage(sci, SCI_SETTEXT, 0, (intptr_t)doc.content.c_str());
	if (!doc.modified)
		ScintillaBridge_sendMessage(sci, SCI_SETSAVEPOINT, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETFIRSTVISIBLELINE, doc.firstVisibleLine, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETSEL, doc.anchorPos, doc.cursorPos);
	ScintillaBridge_sendMessage(sci, SCI_EMPTYUNDOBUFFER, 0, 0);
	// SCI_EMPTYUNDOBUFFER resets Scintilla's save point to current position.
	// For modified documents, this makes Scintilla think it's clean when it isn't.
	// Mark the save point as invalid so SCN_SAVEPOINTREACHED won't clear modified.
	doc.savePointValid = !doc.modified;
	ctx().suppressSavePointNotifications = false;

	for (int bkLine : doc.bookmarkedLines)
		ScintillaBridge_sendMessage(sci, SCI_MARKERADD, bkLine, BOOKMARK_MARKER);
}

void restoreScintillaState(int tabIndex)
{
	restoreViewToScintilla(ctx().scintillaView, ctx().documents, tabIndex);
}

void switchToTabInView(int viewIndex, int tabIndex)
{
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;
	auto& activeTab = (viewIndex == 0) ? ctx().activeTab : ctx().activeTab2;
	void* sci = (viewIndex == 0) ? ctx().scintillaView : ctx().scintillaView2;
	HWND tabHwnd = (viewIndex == 0) ? ctx().tabHwnd : ctx().tabHwnd2;

	if (tabIndex < 0 || tabIndex >= static_cast<int>(docs.size()))
		return;
	if (tabIndex == activeTab)
		return;
	if (!sci) return;

	clearSmartHighlight(sci);
	// Clear incremental search highlights
	{
		intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
		if (docLen > 0) {
			ScintillaBridge_sendMessage(sci, SCI_SETINDICATORCURRENT, INDIC_INCREMENTAL_SEARCH, 0);
			ScintillaBridge_sendMessage(sci, SCI_INDICATORCLEARRANGE, 0, docLen);
		}
	}
	saveViewState(sci, docs, activeTab);
	activeTab = tabIndex;
	if (tabHwnd)
		SendMessageW(tabHwnd, TCM_SETCURSEL, tabIndex, 0);
	restoreViewToScintilla(sci, docs, tabIndex);
	applyLanguageToView(sci, docs[tabIndex].languageIndex);
	bindDocumentMapToActiveView();
	updateDocumentMapViewport();

	if (isIncrementalSearchVisible())
		updateIncrementalSearchTarget();

	const auto& doc = docs[tabIndex];
	NSString* title = WideToNSString(doc.title.c_str());
	[ctx().mainWindow setTitle:[NSString stringWithFormat:@"Notepad++ — %@", title]];
	updateWindowDocumentEdited();
}

void switchToTab(int tabIndex)
{
	switchToTabInView(ctx().activeView, tabIndex);
}

int addNewTabToView(int viewIndex, const std::wstring& title, const std::string& content,
                    const std::wstring& filePath, int langIndex)
{
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;
	auto& activeTab = (viewIndex == 0) ? ctx().activeTab : ctx().activeTab2;
	void* sci = (viewIndex == 0) ? ctx().scintillaView : ctx().scintillaView2;
	HWND tabHwnd = (viewIndex == 0) ? ctx().tabHwnd : ctx().tabHwnd2;

	if (!sci) return -1;

	saveViewState(sci, docs, activeTab);

	DocumentData doc;
	doc.title = title;
	doc.content = content;
	doc.filePath = filePath;
	doc.languageIndex = langIndex;
	docs.push_back(doc);

	int newIndex = static_cast<int>(docs.size()) - 1;

	if (tabHwnd)
	{
		TCITEMW tcItem = {};
		tcItem.mask = TCIF_TEXT;
		wchar_t titleBuf[256];
		wcsncpy(titleBuf, title.c_str(), 255);
		titleBuf[255] = L'\0';
		tcItem.pszText = titleBuf;
		SendMessageW(tabHwnd, TCM_INSERTITEMW, newIndex, reinterpret_cast<LPARAM>(&tcItem));
		SendMessageW(tabHwnd, TCM_SETCURSEL, newIndex, 0);
	}

	activeTab = newIndex;

	ctx().suppressSavePointNotifications = true;
	ScintillaBridge_sendMessage(sci, SCI_SETTEXT, 0, (intptr_t)content.c_str());
	ScintillaBridge_sendMessage(sci, SCI_SETSAVEPOINT, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_GOTOPOS, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_EMPTYUNDOBUFFER, 0, 0);
	ctx().suppressSavePointNotifications = false;

	applyLanguageToView(sci, langIndex);
	if (viewIndex == ctx().activeView)
	{
		bindDocumentMapToActiveView();
		updateDocumentMapViewport();
	}

	NSString* nsTitle = WideToNSString(title.c_str());
	[ctx().mainWindow setTitle:[NSString stringWithFormat:@"Notepad++ — %@", nsTitle]];

	return newIndex;
}

int addNewTab(const std::wstring& title, const std::string& content,
              const std::wstring& filePath, int langIndex)
{
	return addNewTabToView(ctx().activeView, title, content, filePath, langIndex);
}

void closeTabFromView(int viewIndex, int tabIndex)
{
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;
	auto& activeTab = (viewIndex == 0) ? ctx().activeTab : ctx().activeTab2;
	void* sci = (viewIndex == 0) ? ctx().scintillaView : ctx().scintillaView2;
	HWND tabHwnd = (viewIndex == 0) ? ctx().tabHwnd : ctx().tabHwnd2;

	if (tabIndex < 0 || tabIndex >= static_cast<int>(docs.size()))
		return;
	if (!sci) return;

	if (docs.size() <= 1)
	{
		docs[0] = DocumentData();
		ctx().suppressSavePointNotifications = true;
		ScintillaBridge_sendMessage(sci, SCI_CLEARALL, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_EMPTYUNDOBUFFER, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETSAVEPOINT, 0, 0);
		ctx().suppressSavePointNotifications = false;
		if (tabHwnd)
		{
			TCITEMW tcItem = {};
			tcItem.mask = TCIF_TEXT;
			wchar_t untitled[] = L"Untitled";
			tcItem.pszText = untitled;
			SendMessageW(tabHwnd, TCM_SETITEMW, 0, reinterpret_cast<LPARAM>(&tcItem));
		}
		[ctx().mainWindow setTitle:@"Notepad++ — Untitled"];
		updateTabModifiedIndicator(viewIndex, 0);
		updateWindowDocumentEdited();
		return;
	}

	if (tabHwnd)
		SendMessageW(tabHwnd, TCM_DELETEITEM, tabIndex, 0);
	docs.erase(docs.begin() + tabIndex);

	if (tabIndex < activeTab)
		--activeTab;
	else if (tabIndex == activeTab)
	{
		if (activeTab >= static_cast<int>(docs.size()))
			activeTab = static_cast<int>(docs.size()) - 1;
	}

	if (tabHwnd)
		SendMessageW(tabHwnd, TCM_SETCURSEL, activeTab, 0);
	restoreViewToScintilla(sci, docs, activeTab);
	applyLanguageToView(sci, docs[activeTab].languageIndex);
	if (viewIndex == ctx().activeView)
	{
		bindDocumentMapToActiveView();
		updateDocumentMapViewport();
	}

	const auto& doc = docs[activeTab];
	NSString* title = WideToNSString(doc.title.c_str());
	[ctx().mainWindow setTitle:[NSString stringWithFormat:@"Notepad++ — %@", title]];
	updateWindowDocumentEdited();
}

void closeTab(int tabIndex)
{
	closeTabFromView(ctx().activeView, tabIndex);
}

void reorderTabInView(int viewIndex, int fromIndex, int toIndex)
{
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;
	auto& activeTab = (viewIndex == 0) ? ctx().activeTab : ctx().activeTab2;
	HWND tabHwnd = (viewIndex == 0) ? ctx().tabHwnd : ctx().tabHwnd2;

	if (fromIndex < 0 || fromIndex >= static_cast<int>(docs.size()))
		return;
	if (toIndex < 0 || toIndex >= static_cast<int>(docs.size()))
		return;
	if (fromIndex == toIndex)
		return;

	// 1. Move the document in the documents vector
	DocumentData movedDoc = std::move(docs[fromIndex]);
	docs.erase(docs.begin() + fromIndex);
	docs.insert(docs.begin() + toIndex, std::move(movedDoc));

	// 2. Adjust activeTab to follow the moved tab
	if (activeTab == fromIndex)
	{
		activeTab = toIndex;
	}
	else
	{
		if (fromIndex < activeTab && toIndex >= activeTab)
			--activeTab;
		else if (fromIndex > activeTab && toIndex <= activeTab)
			++activeTab;
	}

	// 3. Reorder the shim's TabControlData items array
	if (tabHwnd)
		Win32TabControl_ReorderItem(reinterpret_cast<void*>(tabHwnd), fromIndex, toIndex);

	// 4. Reorder the NppTabBarView's tabs array
	if (tabHwnd)
	{
		auto* tabInfo = HandleRegistry::getWindowInfo(tabHwnd);
		if (tabInfo && tabInfo->nativeView)
		{
			NppTabBarView* tabView = (__bridge NppTabBarView*)tabInfo->nativeView;
			[tabView moveTabFrom:fromIndex to:toIndex];
		}
	}
}

void updateTabModifiedIndicator(int viewIndex, int tabIndex)
{
	auto& docs = (viewIndex == 0) ? ctx().documents : ctx().documents2;
	HWND tabHwnd = (viewIndex == 0) ? ctx().tabHwnd : ctx().tabHwnd2;

	if (tabIndex < 0 || tabIndex >= static_cast<int>(docs.size()))
		return;
	if (!tabHwnd) return;

	const auto& doc = docs[tabIndex];

	// Set the title (without bullet — NppTabBarView draws its own modified dot)
	TCITEMW tcItem = {};
	tcItem.mask = TCIF_TEXT;
	wchar_t titleBuf[256];
	wcsncpy(titleBuf, doc.title.c_str(), 255);
	titleBuf[255] = L'\0';
	tcItem.pszText = titleBuf;
	SendMessageW(tabHwnd, TCM_SETITEMW, tabIndex, reinterpret_cast<LPARAM>(&tcItem));

	// Set modified state directly on the NppTabBarView
	auto* tabInfo = HandleRegistry::getWindowInfo(tabHwnd);
	if (tabInfo && tabInfo->nativeView)
	{
		NppTabBarView* tabView = (__bridge NppTabBarView*)tabInfo->nativeView;
		[tabView setModified:doc.modified forTabAtIndex:tabIndex];
	}
}

void updateWindowDocumentEdited()
{
	if (!ctx().mainWindow) return;

	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();

	bool isModified = false;
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
		isModified = docs[tabIdx].modified;

	[ctx().mainWindow setDocumentEdited:isModified];
}
