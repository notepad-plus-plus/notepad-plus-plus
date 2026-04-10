// wndproc.mm — Main window procedure (command dispatch)
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "wndproc.h"
#include "npp_constants.h"
#include "app_state.h"
#include "document_manager.h"
#include "file_operations.h"
#include "find_replace.h"
#include "edit_commands.h"
#include "bookmarks.h"
#include "autocomplete.h"
#include "preferences_dialog.h"
#include "split_view.h"
#include "save_prompt.h"
#include "about_dialog.h"
#include "recent_files.h"
#include "status_bar.h"
#include "file_path_ops.h"
#include "tab_context_menu.h"
#include "incremental_search.h"
#include "find_in_files.h"
#include "brace_match.h"
#include "print_support.h"
#include "lexer_styles.h"
#include "language_defs.h"
#include "scintilla_config.h"
#include "scintilla_bridge.h"
#include "auto_close.h"
#include "sync_scroll.h"
#include "document_map.h"
#include "function_list_panel.h"
#include "clipboard_history_panel.h"
#include "file_browser_panel.h"
#include "file_switcher_panel.h"
#include "change_history.h"
#include "hash_tools.h"
#include "macro_manager.h"
#include "windows.h"
#include "commctrl.h"
#include "plugin_manager.h"
#include "nppm_handler.h"
#include "Notepad_plus_msgs.h"

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
		{
			UINT cmdId = LOWORD(wParam);

			if (cmdId >= IDM_FILE_RECENT_BASE && cmdId < IDM_FILE_RECENT_BASE + ctx().MAX_RECENT_FILES)
			{
				openRecentFile(cmdId - IDM_FILE_RECENT_BASE);
				return 0;
			}

			if (cmdId >= IDM_LANG_BASE && cmdId < IDM_LANG_BASE + g_numLanguages)
			{
				int langIdx = cmdId - IDM_LANG_BASE;
				auto& docs = ctx().activeDocuments();
				int tabIdx = ctx().activeTabIndex();
				if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
				{
					if (docs[tabIdx].languageIndex != langIdx)
					{
						docs[tabIdx].languageIndex = langIdx;
						++docs[tabIdx].functionListRevision;
						scheduleFunctionListRefresh();

						// Notify plugins that the language changed
						SCNotification notif{};
						notif.nmhdr.hwndFrom = ctx().mainHwnd;
						notif.nmhdr.code = NPPN_LANGCHANGED;
						notif.nmhdr.idFrom = 0; // V1: no stable buffer ID yet
						pluginManager().notify(&notif);
					}
				}
				applyLanguage(langIdx);
				return 0;
			}

			// Plugin commands: static FuncItem range
			if (cmdId >= 22000 && cmdId < 23000) // ID_PLUGINS_CMD .. ID_PLUGINS_CMD_LIMIT
			{
				int i = cmdId - 22000;
				pluginManager().runPluginCommand(i);
				return 0;
			}
			// Plugin commands: dynamic range (allocated via NPPM_ALLOCATECMDID)
			if (pluginManager().inDynamicRange(cmdId))
			{
				pluginManager().relayNppMessages(WM_COMMAND, cmdId, 0);
				return 0;
			}

			switch (cmdId)
			{
				case IDM_FILE_NEW:
					addNewTab(L"Untitled", "");
					return 0;
				case IDM_FILE_OPEN:
					openFile();
					return 0;
				case IDM_FILE_SAVE:
					saveCurrentFile();
					return 0;
				case IDM_FILE_SAVEAS:
				{
					auto& docs = ctx().activeDocuments();
					int tabIdx = ctx().activeTabIndex();
					if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
					{
						std::wstring origPath = docs[tabIdx].filePath;
						docs[tabIdx].filePath.clear();
						saveCurrentFile();
						if (docs[tabIdx].filePath.empty())
							docs[tabIdx].filePath = origPath;
					}
					return 0;
				}
				case IDM_FILE_CLOSE:
					if (promptAndHandleClose(ctx().activeView, ctx().activeTabIndex()))
						closeTabFromView(ctx().activeView, ctx().activeTabIndex());
					return 0;
				case IDM_FILE_CLOSEALL:
				{
					if (!promptAndHandleCloseAll(ctx().activeView))
						return 0;
					auto& closeDocs = ctx().activeDocuments();
					while (closeDocs.size() > 1)
						closeTabFromView(ctx().activeView, static_cast<int>(closeDocs.size()) - 1);
					closeTabFromView(ctx().activeView, 0);
					return 0;
				}
				case IDM_FILE_RECENT_CLEAR:
					ctx().recentFiles.clear();
					rebuildRecentMenu();
					return 0;

				case IDM_FILE_PRINT:
					showPrintDialog(ctx().mainWindow);
					return 0;

				case IDM_FILE_REVEAL_FINDER:
					doRevealInFinder();
					return 0;
				case IDM_FILE_COPY_FULL_PATH:
					doCopyFullPath();
					return 0;
				case IDM_FILE_COPY_FILENAME:
					doCopyFilename();
					return 0;
				case IDM_FILE_COPY_DIR_PATH:
					doCopyDirPath();
					return 0;

				case IDM_EDIT_UNDO:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_UNDO, 0, 0);
					return 0;
				}
				case IDM_EDIT_REDO:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_REDO, 0, 0);
					return 0;
				}
				case IDM_EDIT_CUT:
				{
					void* sci = ctx().activeScintillaView();
					if (sci)
					{
						ScintillaBridge_sendMessage(sci, SCI_CUT, 0, 0);
						captureClipboardEntry();
					}
					return 0;
				}
				case IDM_EDIT_COPY:
				{
					void* sci = ctx().activeScintillaView();
					if (sci)
					{
						ScintillaBridge_sendMessage(sci, SCI_COPY, 0, 0);
						captureClipboardEntry();
					}
					return 0;
				}
				case IDM_EDIT_PASTE:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_PASTE, 0, 0);
					return 0;
				}
				case IDM_EDIT_SELECTALL:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_SELECTALL, 0, 0);
					return 0;
				}

				case IDM_VIEW_WORDWRAP:
				{
					void* sci = ctx().activeScintillaView();
					if (sci)
					{
						intptr_t mode = ScintillaBridge_sendMessage(sci, SCI_GETWRAPMODE, 0, 0);
						intptr_t newMode = (mode == SC_WRAP_NONE) ? SC_WRAP_WORD : SC_WRAP_NONE;

						// Apply to both views
						void* views[] = { ctx().scintillaView, ctx().scintillaView2 };
						for (void* v : views)
						{
							if (v)
								ScintillaBridge_sendMessage(v, SCI_SETWRAPMODE, newMode, 0);
						}

						HMENU hMenu = GetMenu(hWnd);
						if (hMenu)
							CheckMenuItem(hMenu, IDM_VIEW_WORDWRAP,
							              MF_BYCOMMAND | (newMode == SC_WRAP_WORD ? MF_CHECKED : MF_UNCHECKED));
					}
					return 0;
				}
				case IDM_VIEW_LINENUMBER:
				{
					ctx().showLineNumbers = !ctx().showLineNumbers;
					refreshLineNumberMargins();
					HMENU hMenu = GetMenu(hWnd);
					if (hMenu)
						CheckMenuItem(hMenu, IDM_VIEW_LINENUMBER,
						              MF_BYCOMMAND | (ctx().showLineNumbers ? MF_CHECKED : MF_UNCHECKED));
					return 0;
				}
				case IDM_VIEW_FOLDALL:
				{
					void* sci = ctx().activeScintillaView();
					if (sci)
					{
						ctx().suppressSyncScroll = true;
						ScintillaBridge_sendMessage(sci, SCI_FOLDALL, SC_FOLDACTION_CONTRACT, 0);
						ctx().suppressSyncScroll = false;
						syncScrollNow();
					}
					return 0;
				}
				case IDM_VIEW_UNFOLDALL:
				{
					void* sci = ctx().activeScintillaView();
					if (sci)
					{
						ctx().suppressSyncScroll = true;
						ScintillaBridge_sendMessage(sci, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);
						ctx().suppressSyncScroll = false;
						syncScrollNow();
					}
					return 0;
				}
				case IDM_VIEW_PREFERENCES:
					showPreferencesDlg();
					return 0;

				case IDM_SEARCH_FIND:
					showIncrementalSearch();
					return 0;
				case IDM_SEARCH_REPLACE:
					createFindReplaceDlg(true);
					return 0;
				case IDM_SEARCH_FINDNEXT:
					if (isIncrementalSearchVisible())
						doIncrSearchNext();
					else if (ctx().findText.empty())
						showIncrementalSearch();
					else
						doFindNext(true);
					return 0;
				case IDM_SEARCH_FINDPREV:
					if (isIncrementalSearchVisible())
						doIncrSearchPrev();
					else if (ctx().findText.empty())
						showIncrementalSearch();
					else
						doFindNext(false);
					return 0;
				case IDM_SEARCH_GOTOLINE:
					showGoToLineDlg();
					return 0;
				case IDM_SEARCH_FINDINFILES:
					showFindInFilesDlg();
					return 0;

				case IDM_SEARCH_BOOKMARK_TOGGLE:
					toggleBookmark();
					return 0;
				case IDM_SEARCH_BOOKMARK_NEXT:
					nextBookmark();
					return 0;
				case IDM_SEARCH_BOOKMARK_PREV:
					prevBookmark();
					return 0;
				case IDM_SEARCH_BOOKMARK_CLEARALL:
					clearAllBookmarks();
					return 0;
				case IDM_SEARCH_GOTOMATCHINGBRACE:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) doGoToMatchingBrace(sci);
					return 0;
				}

				case IDM_EDIT_AUTOCOMPLETE:
					showAutoComplete();
					return 0;
				case IDM_EDIT_AUTOCLOSE_BRACKETS:
				{
					ctx().autoCloseBrackets = !ctx().autoCloseBrackets;
					if (!ctx().autoCloseBrackets)
					{
						// Clear stale per-view state to prevent unexpected behavior on re-enable
						if (ctx().scintillaView)
							autoCloseOnViewDestroyed(ctx().scintillaView);
						if (ctx().scintillaView2)
							autoCloseOnViewDestroyed(ctx().scintillaView2);
					}
					HMENU hMenu = GetMenu(hWnd);
					if (hMenu)
						CheckMenuItem(hMenu, IDM_EDIT_AUTOCLOSE_BRACKETS,
						              MF_BYCOMMAND | (ctx().autoCloseBrackets ? MF_CHECKED : MF_UNCHECKED));
					return 0;
				}

				case IDM_EDIT_UPPERCASE:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_UPPERCASE, 0, 0);
					return 0;
				}
				case IDM_EDIT_LOWERCASE:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_LOWERCASE, 0, 0);
					return 0;
				}
				case IDM_EDIT_TITLECASE:
					doTitleCase();
					return 0;
				case IDM_EDIT_DUP_LINE:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_LINEDUPLICATE, 0, 0);
					return 0;
				}
				case IDM_EDIT_DEL_LINE:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_LINEDELETE, 0, 0);
					return 0;
				}
				case IDM_EDIT_MOVEUP:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_MOVESELECTEDLINESUP, 0, 0);
					return 0;
				}
				case IDM_EDIT_MOVEDOWN:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) ScintillaBridge_sendMessage(sci, SCI_MOVESELECTEDLINESDOWN, 0, 0);
					return 0;
				}
				case IDM_EDIT_TRIMTRAILING:
					doTrimTrailingWhitespace();
					return 0;
				case IDM_EDIT_REMOVEEMPTY:
					doRemoveEmptyLines();
					return 0;
				case IDM_EDIT_TOGGLECOMMENT:
					doToggleLineComment();
					return 0;
				case IDM_EDIT_SORTASC:
					doSortLines(true);
					return 0;
				case IDM_EDIT_SORTDESC:
					doSortLines(false);
					return 0;
				case IDM_EDIT_SENTENCECASE:
					doSentenceCase();
					return 0;
				case IDM_EDIT_INVERTCASE:
					doInvertCase();
					return 0;
				case IDM_EDIT_CAMELCASE:
					doCamelCase();
					return 0;
				case IDM_EDIT_SNAKECASE:
					doSnakeCase();
					return 0;
				case IDM_EDIT_SORT_CASE_INSENSITIVE:
					doSortLinesCaseInsensitive();
					return 0;
				case IDM_EDIT_SORT_REVERSE:
					doSortLinesReverse();
					return 0;
				case IDM_EDIT_REMOVE_DUPLICATES:
					doRemoveDuplicateLines();
					return 0;
				case IDM_EDIT_SORT_NUMERIC:
					doSortLinesNumeric();
					return 0;
				case IDM_EDIT_SORT_RANDOM:
					doSortLinesRandom();
					return 0;
				case IDM_EDIT_JOINLINES:
					doJoinLines();
					return 0;
				case IDM_EDIT_TABS_TO_SPACES:
					doTabsToSpaces();
					return 0;
				case IDM_EDIT_SPACES_TO_TABS:
					doSpacesToTabs();
					return 0;
				case IDM_EDIT_INSERT_DATETIME_SHORT:
					insertDateTimeShort();
					return 0;
				case IDM_EDIT_INSERT_DATETIME_LONG:
					insertDateTimeLong();
					return 0;

				case IDM_VIEW_ZOOMIN:
				{
					void* sci = ctx().activeScintillaView();
					if (sci)
					{
						ScintillaBridge_sendMessage(sci, SCI_ZOOMIN, 0, 0);
						auto& docs = ctx().activeDocuments();
						int tabIdx = ctx().activeTabIndex();
						if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
							docs[tabIdx].zoomLevel = static_cast<int>(ScintillaBridge_sendMessage(sci, SCI_GETZOOM, 0, 0));
					}
					refreshLineNumberMargins();
					return 0;
				}
				case IDM_VIEW_ZOOMOUT:
				{
					void* sci = ctx().activeScintillaView();
					if (sci)
					{
						ScintillaBridge_sendMessage(sci, SCI_ZOOMOUT, 0, 0);
						auto& docs = ctx().activeDocuments();
						int tabIdx = ctx().activeTabIndex();
						if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
							docs[tabIdx].zoomLevel = static_cast<int>(ScintillaBridge_sendMessage(sci, SCI_GETZOOM, 0, 0));
					}
					refreshLineNumberMargins();
					return 0;
				}
				case IDM_VIEW_ZOOMRESTORE:
				{
					void* sci = ctx().activeScintillaView();
					if (sci)
					{
						ScintillaBridge_sendMessage(sci, SCI_SETZOOM, 0, 0);
						auto& docs = ctx().activeDocuments();
						int tabIdx = ctx().activeTabIndex();
						if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
							docs[tabIdx].zoomLevel = 0;
					}
					refreshLineNumberMargins();
					return 0;
				}

				case IDM_VIEW_SHOW_WS:
			{
				ctx().showWhitespace = !ctx().showWhitespace;
				void* views[] = { ctx().scintillaView, ctx().scintillaView2 };
				for (void* v : views)
				{
					if (v)
						ScintillaBridge_sendMessage(v, SCI_SETVIEWWS,
							ctx().showWhitespace ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
				}
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_SHOW_WS,
						MF_BYCOMMAND | (ctx().showWhitespace ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_VIEW_SHOW_EOL:
			{
				ctx().showEol = !ctx().showEol;
				void* views[] = { ctx().scintillaView, ctx().scintillaView2 };
				for (void* v : views)
				{
					if (v)
						ScintillaBridge_sendMessage(v, SCI_SETVIEWEOL, ctx().showEol ? 1 : 0, 0);
				}
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_SHOW_EOL,
						MF_BYCOMMAND | (ctx().showEol ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_VIEW_SHOW_INDENT:
			{
				ctx().showIndentGuides = !ctx().showIndentGuides;
				void* views[] = { ctx().scintillaView, ctx().scintillaView2 };
				for (void* v : views)
				{
					if (v)
						ScintillaBridge_sendMessage(v, SCI_SETINDENTATIONGUIDES,
							ctx().showIndentGuides ? SC_IV_LOOKBOTH : SC_IV_NONE, 0);
				}
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_SHOW_INDENT,
						MF_BYCOMMAND | (ctx().showIndentGuides ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_VIEW_SYNCHRONIZE_SCROLLING:
			{
				if (!ctx().isSplit)
					return 0;
				setSyncScrollingEnabled(!ctx().syncScrolling);
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_SYNCHRONIZE_SCROLLING,
					              MF_BYCOMMAND | (ctx().syncScrolling ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_VIEW_DOCUMENTMAP:
			{
				setDocumentMapEnabled(!ctx().documentMapEnabled);
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_DOCUMENTMAP,
					              MF_BYCOMMAND | (ctx().documentMapEnabled ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_VIEW_FUNCTIONLIST:
			{
				setFunctionListEnabled(!ctx().functionListEnabled);
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_FUNCTIONLIST,
					              MF_BYCOMMAND | (ctx().functionListEnabled ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_VIEW_CLIPBOARD_HISTORY:
			{
				setClipboardHistoryEnabled(!ctx().clipboardHistoryEnabled);
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_CLIPBOARD_HISTORY,
					              MF_BYCOMMAND | (ctx().clipboardHistoryEnabled ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_VIEW_FILEBROWSER:
			{
				setFileBrowserEnabled(!ctx().fileBrowserEnabled);
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_FILEBROWSER,
					              MF_BYCOMMAND | (ctx().fileBrowserEnabled ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_VIEW_FILESWITCHER:
			{
				setFileSwitcherEnabled(!ctx().fileSwitcherEnabled);
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_FILESWITCHER,
					              MF_BYCOMMAND | (ctx().fileSwitcherEnabled ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}
			case IDM_FILE_OPENFOLDER:
			{
				openFolderWithDialog();
				return 0;
			}
			case IDM_VIEW_CHANGE_HISTORY:
			{
				ctx().showChangeHistory = !ctx().showChangeHistory;
				void* views[] = { ctx().scintillaView, ctx().scintillaView2 };
				for (void* v : views)
				{
					if (v)
						setChangeHistoryMarginVisible(v, ctx().showChangeHistory);
				}
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
					CheckMenuItem(hMenu, IDM_VIEW_CHANGE_HISTORY,
					              MF_BYCOMMAND | (ctx().showChangeHistory ? MF_CHECKED : MF_UNCHECKED));
				return 0;
			}

			case IDM_VIEW_FULLSCREEN:
				if (ctx().mainWindow)
					[ctx().mainWindow toggleFullScreen:nil];
				return 0;

			case IDM_VIEW_TOOLBAR:
				if (ctx().mainWindow)
					[ctx().mainWindow toggleToolbarShown:nil];
				return 0;

			case IDM_VIEW_SPLIT:
					doSplit();
					return 0;
				case IDM_VIEW_UNSPLIT:
					doUnsplit();
					return 0;
				case IDM_VIEW_MOVETOOTHER:
					doMoveToOtherView();
					return 0;
				case IDM_VIEW_CLONETOOTHER:
					doCloneToOtherView();
					return 0;

				case IDM_TOOLS_HASH_MD5:
					doHashMD5();
					return 0;
				case IDM_TOOLS_HASH_SHA1:
					doHashSHA1();
					return 0;
				case IDM_TOOLS_HASH_SHA256:
					doHashSHA256();
					return 0;
				case IDM_TOOLS_HASH_SHA512:
					doHashSHA512();
					return 0;

				case IDM_MACRO_START_RECORD:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) MacroManager::instance().startRecording(sci);
					return 0;
				}
				case IDM_MACRO_STOP_RECORD:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) MacroManager::instance().stopRecording(sci);
					return 0;
				}
				case IDM_MACRO_PLAYBACK:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) MacroManager::instance().playback(sci);
					return 0;
				}
				case IDM_MACRO_PLAYBACK_MULTI:
				{
					void* sci = ctx().activeScintillaView();
					if (sci) MacroManager::instance().playbackMultiple(sci, 0);
					return 0;
				}
				case IDM_MACRO_SAVE:
					MacroManager::instance().saveMacro("");
					return 0;
				case IDM_MACRO_LOAD:
					MacroManager::instance().loadMacro("");
					return 0;

				case IDM_HELP_ABOUT:
					showAboutDlg();
					return 0;

				case IDM_FORMAT_EOL_LF:
				case IDM_FORMAT_EOL_CRLF:
				case IDM_FORMAT_EOL_CR:
				{
					void* sci = ctx().activeScintillaView();
					auto& docs = ctx().activeDocuments();
					int tabIdx = ctx().activeTabIndex();
					if (sci && tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
					{
						int eol = SC_EOL_LF;
						if (cmdId == IDM_FORMAT_EOL_CRLF) eol = SC_EOL_CRLF;
						if (cmdId == IDM_FORMAT_EOL_CR)   eol = SC_EOL_CR;
						docs[tabIdx].eolMode = eol;
						ScintillaBridge_sendMessage(sci, SCI_SETEOLMODE, eol, 0);
						ScintillaBridge_sendMessage(sci, SCI_CONVERTEOLS, eol, 0);
						updateStatusBar();
					}
					return 0;
				}

				case IDM_FORMAT_ENC_UTF8:
				case IDM_FORMAT_ENC_UTF8BOM:
				case IDM_FORMAT_ENC_UTF16LE:
				case IDM_FORMAT_ENC_UTF16BE:
				case IDM_FORMAT_ENC_ANSI:
				{
					auto& docs = ctx().activeDocuments();
					int tabIdx = ctx().activeTabIndex();
					if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
					{
						int newEnc = ENC_UTF8;
						if (cmdId == IDM_FORMAT_ENC_UTF8BOM)  newEnc = ENC_UTF8_BOM;
						if (cmdId == IDM_FORMAT_ENC_UTF16LE)  newEnc = ENC_UTF16_LE;
						if (cmdId == IDM_FORMAT_ENC_UTF16BE)  newEnc = ENC_UTF16_BE;
						if (cmdId == IDM_FORMAT_ENC_ANSI)     newEnc = ENC_ANSI;
						docs[tabIdx].encoding = newEnc;
						updateStatusBar();
					}
					return 0;
				}
			}
			break;
		}

		case WM_INITMENUPOPUP:
			updateSplitMenuState();
			{
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu)
				{
					CheckMenuItem(hMenu, IDM_VIEW_DOCUMENTMAP,
					              MF_BYCOMMAND | (ctx().documentMapEnabled ? MF_CHECKED : MF_UNCHECKED));
					CheckMenuItem(hMenu, IDM_VIEW_FUNCTIONLIST,
					              MF_BYCOMMAND | (ctx().functionListEnabled ? MF_CHECKED : MF_UNCHECKED));
					CheckMenuItem(hMenu, IDM_VIEW_CLIPBOARD_HISTORY,
					              MF_BYCOMMAND | (ctx().clipboardHistoryEnabled ? MF_CHECKED : MF_UNCHECKED));
					CheckMenuItem(hMenu, IDM_VIEW_FILEBROWSER,
					              MF_BYCOMMAND | (ctx().fileBrowserEnabled ? MF_CHECKED : MF_UNCHECKED));
					CheckMenuItem(hMenu, IDM_VIEW_FILESWITCHER,
					              MF_BYCOMMAND | (ctx().fileSwitcherEnabled ? MF_CHECKED : MF_UNCHECKED));
					CheckMenuItem(hMenu, IDM_EDIT_AUTOCLOSE_BRACKETS,
					              MF_BYCOMMAND | (ctx().autoCloseBrackets ? MF_CHECKED : MF_UNCHECKED));
					CheckMenuItem(hMenu, IDM_VIEW_SYNCHRONIZE_SCROLLING,
					              MF_BYCOMMAND | (ctx().syncScrolling && ctx().isSplit ? MF_CHECKED : MF_UNCHECKED));
					CheckMenuItem(hMenu, IDM_VIEW_CHANGE_HISTORY,
					              MF_BYCOMMAND | (ctx().showChangeHistory ? MF_CHECKED : MF_UNCHECKED));
				}
			}
			updateFilePathMenuState();
			break;

		case WM_NOTIFY:
		{
			NMHDR* pNmhdr = reinterpret_cast<NMHDR*>(lParam);
			if (!pNmhdr)
				break;

			if (pNmhdr->code == TCN_SELCHANGE)
			{
				if (pNmhdr->hwndFrom == ctx().tabHwnd)
				{
					int newSel = static_cast<int>(SendMessageW(ctx().tabHwnd, TCM_GETCURSEL, 0, 0));
					if (newSel != ctx().activeTab && newSel >= 0)
						switchToTabInView(0, newSel);
				}
				else if (pNmhdr->hwndFrom == ctx().tabHwnd2)
				{
					int newSel = static_cast<int>(SendMessageW(ctx().tabHwnd2, TCM_GETCURSEL, 0, 0));
					if (newSel != ctx().activeTab2 && newSel >= 0)
						switchToTabInView(1, newSel);
				}
				return 0;
			}

			if (pNmhdr->code == NM_TAB_CLOSE ||
				pNmhdr->code == NM_TAB_REORDER ||
				pNmhdr->code == NM_TAB_CONTEXTMENU)
			{
				// Determine view index from hwndFrom
				int viewIndex = -1;
				if (pNmhdr->hwndFrom == ctx().tabHwnd)
					viewIndex = 0;
				else if (pNmhdr->hwndFrom == ctx().tabHwnd2)
					viewIndex = 1;
				if (viewIndex < 0)
					break;

				if (pNmhdr->code == NM_TAB_CLOSE)
				{
					struct TabCloseNotify {
						NMHDR hdr;
						int tabIndex;
					};
					auto* tcn = reinterpret_cast<TabCloseNotify*>(lParam);
					if (promptAndHandleClose(viewIndex, tcn->tabIndex))
						closeTabFromView(viewIndex, tcn->tabIndex);
					return 0;
				}

				if (pNmhdr->code == NM_TAB_REORDER)
				{
					struct TabReorderNotify {
						NMHDR hdr;
						int fromIndex;
						int toIndex;
					};
					auto* trn = reinterpret_cast<TabReorderNotify*>(lParam);
					reorderTabInView(viewIndex, trn->fromIndex, trn->toIndex);
					return 0;
				}

				if (pNmhdr->code == NM_TAB_CONTEXTMENU)
				{
					struct TabContextNotify {
						NMHDR hdr;
						int tabIndex;
						NSPoint screenPoint;
					};
					auto* tcn = reinterpret_cast<TabContextNotify*>(lParam);
					showTabContextMenu(viewIndex, tcn->tabIndex, tcn->screenPoint);
					return 0;
				}
			}

			break;
		}

		case WM_TIMER:
			if (wParam == IDT_STATUSBAR)
			{
				updateStatusBar();
				return 0;
			}
			break;

		case WM_SIZE:
			if (ctx().scintillaView)
				ScintillaBridge_resizeToFit(ctx().scintillaView);
			if (ctx().isSplit && ctx().scintillaView2)
				ScintillaBridge_resizeToFit(ctx().scintillaView2);
			relayoutDocumentMap();
			updateDocumentMapViewport();
			return 0;

		case WM_CLOSE:
			if (!promptAndHandleQuit())
				return 0;
			KillTimer(hWnd, IDT_STATUSBAR);
			PostQuitMessage(0);
			return 0;
		default:
		{
			// Handle NPPM range (WM_USER + 1000)
			if (msg >= NPPMSG && msg < NPPMSG + 200)
			{
				LRESULT result = handleNppmMessage(hWnd, msg, wParam, lParam);
				pluginManager().relayNppMessages(msg, wParam, lParam);
				return result;
			}
			// Handle RUNCOMMAND_USER range (WM_USER + 3000)
			if (msg >= RUNCOMMAND_USER && msg < RUNCOMMAND_USER + 20)
			{
				LRESULT result = handleRunCommandMessage(hWnd, msg, wParam, lParam);
				pluginManager().relayNppMessages(msg, wParam, lParam);
				return result;
			}
			break;
		}
	}

	// Catch-all: relay all messages to plugins' messageProc (matches upstream)
	pluginManager().relayNppMessages(msg, wParam, lParam);

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}
