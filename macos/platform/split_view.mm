// split_view.mm — Split view management
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "split_view.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "document_manager.h"
#include "lexer_styles.h"
#include "appearance.h"
#include "scintilla_config.h"
#include "scintilla_bridge.h"
#include "handle_registry.h"
#include "brace_match.h"
#include "smart_highlight.h"
#include "incremental_search.h"
#include "auto_indent.h"
#include "auto_close.h"
#include "sync_scroll.h"
#include "document_map.h"
#include "function_list_panel.h"
#include "scintilla_notify.h"
#include "windows.h"
#include "commctrl.h"

void updateSplitMenuState()
{
	HMENU hMenu = GetMenu(ctx().mainHwnd);
	if (!hMenu)
		return;

	const bool split = ctx().isSplit;
	EnableMenuItem(hMenu, IDM_VIEW_SPLIT, MF_BYCOMMAND | (split ? MF_GRAYED : MF_ENABLED));
	EnableMenuItem(hMenu, IDM_VIEW_UNSPLIT, MF_BYCOMMAND | (split ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_VIEW_MOVETOOTHER, MF_BYCOMMAND | (split ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_VIEW_CLONETOOTHER, MF_BYCOMMAND | (split ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, IDM_VIEW_SYNCHRONIZE_SCROLLING, MF_BYCOMMAND | (split ? MF_ENABLED : MF_GRAYED));
	if (!split)
		CheckMenuItem(hMenu, IDM_VIEW_SYNCHRONIZE_SCROLLING, MF_BYCOMMAND | MF_UNCHECKED);
}

void layoutSplitTopTabBars()
{
	if (!ctx().mainWindow || !ctx().tabHwnd)
		return;

	NSView* contentView = ctx().mainWindow.contentView;
	if (!contentView)
		return;

	const CGFloat tabHeight = NPP_TAB_BAR_HEIGHT;
	const CGFloat topY = contentView.bounds.size.height - tabHeight;

	auto* leftTabInfo = HandleRegistry::getWindowInfo(ctx().tabHwnd);
	if (leftTabInfo && leftTabInfo->nativeView)
	{
		NSView* leftTabView = (__bridge NSView*)leftTabInfo->nativeView;
		if (ctx().isSplit && ctx().editorContainer)
		{
			leftTabView.frame = NSMakeRect(ctx().editorContainer.frame.origin.x, topY,
			                               ctx().editorContainer.frame.size.width, tabHeight);
		}
		else
		{
			NSRect editorFrame = ctx().editorContainer ? ctx().editorContainer.frame :
				NSMakeRect(0, 0, contentView.bounds.size.width, 0);
			leftTabView.frame = NSMakeRect(editorFrame.origin.x, topY, editorFrame.size.width, tabHeight);
		}
	}

	if (ctx().isSplit && ctx().tabHwnd2 && ctx().editorContainer2)
	{
		auto* rightTabInfo = HandleRegistry::getWindowInfo(ctx().tabHwnd2);
		if (rightTabInfo && rightTabInfo->nativeView)
		{
			NSView* rightTabView = (__bridge NSView*)rightTabInfo->nativeView;
			rightTabView.frame = NSMakeRect(ctx().editorContainer2.frame.origin.x, topY,
			                                ctx().editorContainer2.frame.size.width, tabHeight);
		}
	}
}

void doSplit()
{
	if (ctx().isSplit || !ctx().mainWindow || !ctx().editorContainer) return;

	NSView* contentView = ctx().mainWindow.contentView;

	ctx().splitView = [[NSSplitView alloc] initWithFrame:ctx().editorContainer.frame];
	ctx().splitView.vertical = YES;
	ctx().splitView.dividerStyle = NSSplitViewDividerStyleThin;
	ctx().splitView.autoresizingMask = ctx().editorContainer.autoresizingMask;

	NSRect leftFrame = NSMakeRect(0, 0, ctx().editorContainer.frame.size.width / 2, ctx().editorContainer.frame.size.height);
	ctx().editorContainer.frame = leftFrame;
	[ctx().editorContainer removeFromSuperview];
	[ctx().splitView addArrangedSubview:ctx().editorContainer];

	NSRect rightFrame = NSMakeRect(0, 0, ctx().splitView.frame.size.width / 2, ctx().splitView.frame.size.height);
	ctx().editorContainer2 = [[NSView alloc] initWithFrame:rightFrame];
	ctx().editorContainer2.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	[ctx().splitView addArrangedSubview:ctx().editorContainer2];

	CGFloat containerHeight = rightFrame.size.height;
	CGFloat containerWidth = rightFrame.size.width;

	ctx().tabHwnd2 = CreateWindowExW(
		0, L"SysTabControl32", L"",
		WS_CHILD | WS_VISIBLE | TCS_FOCUSNEVER,
		0, 0,
		static_cast<int>(containerWidth), 28,
		ctx().mainHwnd,
		nullptr, nullptr, nullptr
	);

	ctx().sciContainer2 = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, containerWidth, containerHeight)];
	ctx().sciContainer2.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	[ctx().editorContainer2 addSubview:ctx().sciContainer2];

	ctx().scintillaView2 = ScintillaBridge_createView((__bridge void*)ctx().sciContainer2, 0, 0, 0, 0);
	if (ctx().scintillaView2)
	{
		configureScintilla(ctx().scintillaView2);
		ScintillaBridge_setNotifyCallback(ctx().scintillaView2, 1,
			[](intptr_t windowid, unsigned int iMessage, uintptr_t wParam, uintptr_t lParam) {
				if (iMessage == 1002 && lParam)
				{
					auto* scn = reinterpret_cast<const SciNotification*>(lParam);
					if (scn->nmhdr.code == SCN_FOCUSIN)
					{
						// Clear smart highlights from the other view on focus switch
						if (ctx().activeView != 1 && ctx().scintillaView)
							clearSmartHighlight(ctx().scintillaView);
						ctx().activeView = 1;
						bindDocumentMapToActiveView();
						bindFunctionListToActiveView();
					}
					else if (scn->nmhdr.code == SCN_SAVEPOINTLEFT)
					{
						if (!ctx().suppressSavePointNotifications)
						{
							int tabIdx = ctx().activeTab2;
							if (tabIdx >= 0 && tabIdx < static_cast<int>(ctx().documents2.size()))
							{
								ctx().documents2[tabIdx].modified = true;
								updateTabModifiedIndicator(1, tabIdx);
								updateWindowDocumentEdited();
							}
						}
					}
					else if (scn->nmhdr.code == SCN_SAVEPOINTREACHED)
					{
						if (!ctx().suppressSavePointNotifications)
						{
							int tabIdx = ctx().activeTab2;
							if (tabIdx >= 0 && tabIdx < static_cast<int>(ctx().documents2.size())
							    && ctx().documents2[tabIdx].savePointValid)
							{
								ctx().documents2[tabIdx].modified = false;
								updateTabModifiedIndicator(1, tabIdx);
								updateWindowDocumentEdited();
							}
						}
					}
					else if (scn->nmhdr.code == 2010 && scn->margin == 1 && ctx().scintillaView2)
					{
						intptr_t line = ScintillaBridge_sendMessage(ctx().scintillaView2, SCI_LINEFROMPOSITION, scn->position, 0);
						intptr_t markers = ScintillaBridge_sendMessage(ctx().scintillaView2, SCI_MARKERGET, line, 0);
						if (markers & BOOKMARK_MASK)
							ScintillaBridge_sendMessage(ctx().scintillaView2, SCI_MARKERDELETE, line, BOOKMARK_MARKER);
						else
							ScintillaBridge_sendMessage(ctx().scintillaView2, SCI_MARKERADD, line, BOOKMARK_MARKER);
					}
					else if (scn->nmhdr.code == SCN_UPDATEUI)
					{
						if (ctx().scintillaView2)
						{
							doBraceMatch(ctx().scintillaView2);
							if (scn->updated & SC_UPDATE_SELECTION)
								scheduleSmartHighlight(ctx().scintillaView2);
							handleSyncScrollUpdate(ctx().scintillaView2, scn->updated);
							handleDocumentMapUpdateUI(ctx().scintillaView2, scn->updated);
						}
					}
					else if (scn->nmhdr.code == SCN_CHARADDED)
					{
						if (ctx().scintillaView2)
						{
							int langIdx = -1;
							if (ctx().activeTab2 >= 0 && ctx().activeTab2 < static_cast<int>(ctx().documents2.size()))
								langIdx = ctx().documents2[ctx().activeTab2].languageIndex;
							if (ctx().autoCloseBrackets)
								handleAutoCloseCharAdded(ctx().scintillaView2, scn->ch, langIdx);
							if (ctx().autoIndent)
								performAutoIndent(ctx().scintillaView2, scn->ch, langIdx);
						}
					}
					else if (scn->nmhdr.code == SCN_MODIFIED)
					{
						if (scn->linesAdded != 0)
							refreshLineNumberMargin(ctx().scintillaView2);

						if (ctx().scintillaView2 && ctx().autoCloseBrackets)
						{
							int langIdx = -1;
							if (ctx().activeTab2 >= 0 && ctx().activeTab2 < static_cast<int>(ctx().documents2.size()))
								langIdx = ctx().documents2[ctx().activeTab2].languageIndex;
							handleAutoCloseModified(ctx().scintillaView2, scn, langIdx);
						}

						if (scn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
						{
							int tabIdx = ctx().activeTab2;
							if (!ctx().suppressSavePointNotifications
							    && tabIdx >= 0 && tabIdx < static_cast<int>(ctx().documents2.size()))
							{
								++ctx().documents2[tabIdx].functionListRevision;
								scheduleFunctionListRefresh();
							}
						}
					}
				}
			});
	}

	saveScintillaState();
	ctx().documents2.clear();
	int srcIdx = ctx().activeTab >= 0 ? ctx().activeTab : 0;
	if (srcIdx >= 0 && srcIdx < static_cast<int>(ctx().documents.size()))
	{
		ctx().documents2.push_back(ctx().documents[srcIdx]);
		ctx().documents2[0].functionListDocumentId = allocateFunctionListDocumentId();
		ctx().activeTab2 = 0;

		if (ctx().tabHwnd2)
		{
			TCITEMW tcItem = {};
			tcItem.mask = TCIF_TEXT;
			wchar_t titleBuf[256];
			wcsncpy(titleBuf, ctx().documents[srcIdx].title.c_str(), 255);
			titleBuf[255] = L'\0';
			tcItem.pszText = titleBuf;
			SendMessageW(ctx().tabHwnd2, TCM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&tcItem));
			SendMessageW(ctx().tabHwnd2, TCM_SETCURSEL, 0, 0);
		}

		if (ctx().scintillaView2)
		{
			restoreViewToScintilla(ctx().scintillaView2, ctx().documents2, 0);
			applyLanguageToView(ctx().scintillaView2, ctx().documents2[0].languageIndex);
		}
	}

	[contentView addSubview:ctx().splitView];
	ctx().splitView.delegate = (id<NSSplitViewDelegate>)ctx().mainWindow.delegate;
	ctx().isSplit = true;
	layoutSplitTopTabBars();
	updateSplitMenuState();

	ScintillaBridge_resizeToFit(ctx().scintillaView);
	if (ctx().scintillaView2)
		ScintillaBridge_resizeToFit(ctx().scintillaView2);

	applyAppearance();

	// Sync word wrap state from main view to new split view
	if (ctx().scintillaView && ctx().scintillaView2)
	{
		intptr_t wrapMode = ScintillaBridge_sendMessage(ctx().scintillaView, SCI_GETWRAPMODE, 0, 0);
		ScintillaBridge_sendMessage(ctx().scintillaView2, SCI_SETWRAPMODE, wrapMode, 0);
	}

	relayoutFunctionListPanel();
	bindDocumentMapToActiveView();
	updateDocumentMapViewport();
	bindFunctionListToActiveView();
	refreshSyncScrollAnchor();
}

void doUnsplit()
{
	if (!ctx().isSplit || !ctx().splitView) return;

	NSView* contentView = ctx().mainWindow.contentView;

	if (ctx().scintillaView2)
		saveViewState(ctx().scintillaView2, ctx().documents2, ctx().activeTab2);

	// If incremental search bar is in the closing view, remove it first
	if (isIncrementalSearchVisible() && ctx().incrementalSearchBar)
	{
		NSView* bar = ctx().incrementalSearchBar;
		if ([bar superview] == ctx().editorContainer2 || [bar superview] == ctx().sciContainer2)
		{
			[bar removeFromSuperview];
		}
	}

	if (ctx().scintillaView2)
	{
		cancelPendingSmartHighlight();
		ScintillaBridge_clearNotifyCallback(ctx().scintillaView2);
		autoCloseOnViewDestroyed(ctx().scintillaView2);
		ScintillaBridge_destroyView(ctx().scintillaView2);
		ctx().scintillaView2 = nullptr;
	}

	if (ctx().tabHwnd2)
	{
		DestroyWindow(ctx().tabHwnd2);
		ctx().tabHwnd2 = nullptr;
	}

	ctx().sciContainer2 = nil;

	[ctx().editorContainer removeFromSuperview];
	if (ctx().editorContainer2)
	{
		[ctx().editorContainer2 removeFromSuperview];
		ctx().editorContainer2 = nil;
	}

	[ctx().splitView removeFromSuperview];
	ctx().splitView = nil;

	CGFloat tabHeight = NPP_TAB_BAR_HEIGHT;
	CGFloat statusHeight = NPP_STATUS_BAR_HEIGHT;
	CGFloat editorHeight = contentView.bounds.size.height - tabHeight - statusHeight;
	ctx().editorContainer.frame = NSMakeRect(0, statusHeight, contentView.bounds.size.width, editorHeight);
	ctx().editorContainer.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	[contentView addSubview:ctx().editorContainer];
	ScintillaBridge_resizeToFit(ctx().scintillaView);

	ctx().documents2.clear();
	ctx().activeTab2 = -1;
	ctx().activeView = 0;
	ctx().isSplit = false;
	ctx().syncScrollReentrant = false;
	refreshSyncScrollAnchor();

	if (isIncrementalSearchVisible())
		updateIncrementalSearchTarget();

	layoutSplitTopTabBars();
	updateSplitMenuState();
	relayoutFunctionListPanel();
	bindDocumentMapToActiveView();
	updateDocumentMapViewport();
	bindFunctionListToActiveView();
}

void doMoveToOtherView()
{
	if (!ctx().isSplit || !ctx().scintillaView || !ctx().scintillaView2) return;

	int srcView = ctx().activeView;
	int dstView = 1 - srcView;
	auto& srcDocs = (srcView == 0) ? ctx().documents : ctx().documents2;
	int srcTab = (srcView == 0) ? ctx().activeTab : ctx().activeTab2;

	if (srcTab < 0 || srcTab >= static_cast<int>(srcDocs.size())) return;
	if (srcDocs.size() <= 1) return;

	void* srcSci = (srcView == 0) ? ctx().scintillaView : ctx().scintillaView2;
	saveViewState(srcSci, srcDocs, srcTab);

	DocumentData docCopy = srcDocs[srcTab];
	addNewTabToView(dstView, docCopy.title, docCopy.content, docCopy.filePath, docCopy.languageIndex);

	auto& dstDocs = (dstView == 0) ? ctx().documents : ctx().documents2;
	int dstIdx = static_cast<int>(dstDocs.size()) - 1;
	if (dstIdx >= 0)
	{
		dstDocs[dstIdx].encoding = docCopy.encoding;
		dstDocs[dstIdx].eolMode = docCopy.eolMode;
		dstDocs[dstIdx].cursorPos = docCopy.cursorPos;
		dstDocs[dstIdx].anchorPos = docCopy.anchorPos;
		dstDocs[dstIdx].firstVisibleLine = docCopy.firstVisibleLine;
		dstDocs[dstIdx].bookmarkedLines = docCopy.bookmarkedLines;
	}

	closeTabFromView(srcView, srcTab);
}

void doCloneToOtherView()
{
	if (!ctx().isSplit || !ctx().scintillaView || !ctx().scintillaView2) return;

	int srcView = ctx().activeView;
	int dstView = 1 - srcView;
	auto& srcDocs = (srcView == 0) ? ctx().documents : ctx().documents2;
	int srcTab = (srcView == 0) ? ctx().activeTab : ctx().activeTab2;

	if (srcTab < 0 || srcTab >= static_cast<int>(srcDocs.size())) return;

	void* srcSci = (srcView == 0) ? ctx().scintillaView : ctx().scintillaView2;
	saveViewState(srcSci, srcDocs, srcTab);

	DocumentData docCopy = srcDocs[srcTab];
	addNewTabToView(dstView, docCopy.title, docCopy.content, docCopy.filePath, docCopy.languageIndex);

	auto& dstDocs = (dstView == 0) ? ctx().documents : ctx().documents2;
	int dstIdx = static_cast<int>(dstDocs.size()) - 1;
	if (dstIdx >= 0)
	{
		dstDocs[dstIdx].encoding = docCopy.encoding;
		dstDocs[dstIdx].eolMode = docCopy.eolMode;
		dstDocs[dstIdx].cursorPos = docCopy.cursorPos;
		dstDocs[dstIdx].anchorPos = docCopy.anchorPos;
		dstDocs[dstIdx].firstVisibleLine = docCopy.firstVisibleLine;
		dstDocs[dstIdx].bookmarkedLines = docCopy.bookmarkedLines;
	}
}
