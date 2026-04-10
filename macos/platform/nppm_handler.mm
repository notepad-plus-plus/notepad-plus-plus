// nppm_handler.mm — NPPM_* and RUNCOMMAND_USER message handlers

#import <Cocoa/Cocoa.h>
#include "nppm_handler.h"
#include "app_state.h"
#include "plugin_manager.h"
#include "scintilla_bridge.h"
#include "language_defs.h"
#include "lexer_styles.h"
#include "Notepad_plus_msgs.h"
#include "Scintilla.h"

#include <cwchar>
#include <filesystem>

// ============================================================
// Helper: copy wide string to plugin buffer with two-call contract
// ============================================================
static LRESULT copyWideToBuffer(const std::wstring& src, WPARAM maxChars, LPARAM lParam)
{
	if (!lParam)
	{
		// First call: return number of wchar_t needed (not including null)
		return static_cast<LRESULT>(src.size());
	}

	// Fail without writing when the buffer is too small (matches upstream behavior).
	// Also reject maxChars==0 with a non-null buffer to prevent overflow.
	if (maxChars == 0 || src.size() >= static_cast<size_t>(maxChars))
		return 0;

	wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
	const size_t copyLen = src.size();
	wcsncpy(buf, src.c_str(), copyLen);
	buf[copyLen] = L'\0';
	return TRUE;
}

// ============================================================
// NPPMSG range (WM_USER + 1000)
// ============================================================
LRESULT handleNppmMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();

	switch (msg)
	{
		case NPPM_GETCURRENTSCINTILLA:
		{
			if (lParam)
				*reinterpret_cast<int*>(lParam) = ctx().activeView;
			return TRUE;
		}

		case NPPM_GETCURRENTLANGTYPE:
		{
			if (lParam && tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
				*reinterpret_cast<int*>(lParam) = docs[tabIdx].languageIndex;
			return TRUE;
		}

		case NPPM_SETCURRENTLANGTYPE:
		{
			int langType = static_cast<int>(lParam);
			if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
			{
				docs[tabIdx].languageIndex = langType;
				applyLanguage(langType);

				// Notify plugins that the language changed
				SCNotification notif{};
				notif.nmhdr.hwndFrom = hWnd;
				notif.nmhdr.code = NPPN_LANGCHANGED;
				notif.nmhdr.idFrom = static_cast<uintptr_t>(docs[tabIdx].bufferId);
				pluginManager().notify(&notif);
			}
			return TRUE;
		}

		case NPPM_GETNBOPENFILES:
		{
			int viewType = static_cast<int>(lParam);
			if (viewType == 1) // PRIMARY_VIEW
				return static_cast<LRESULT>(ctx().documents.size());
			else if (viewType == 2) // SECOND_VIEW
				return static_cast<LRESULT>(ctx().documents2.size());
			else // ALL_OPEN_FILES — deduplicate split-view mirrors
			{
				size_t count = ctx().documents.size();
				for (const auto& doc2 : ctx().documents2)
				{
					bool duplicate = false;
					if (!doc2.filePath.empty())
					{
						for (const auto& doc1 : ctx().documents)
						{
							if (doc1.filePath == doc2.filePath)
							{
								duplicate = true;
								break;
							}
						}
					}
					if (!duplicate)
						++count;
				}
				return static_cast<LRESULT>(count);
			}
		}

		case NPPM_SETMENUITEMCHECK:
		{
			HMENU hMenu = nullptr; // Search across all menus
			(void)hMenu;
			::CheckMenuItem(GetMenu(hWnd), static_cast<UINT>(wParam),
			                lParam ? MF_CHECKED : MF_UNCHECKED);
			return TRUE;
		}

		case NPPM_GETPLUGINSCONFIGDIR:
		{
			@autoreleasepool {
				NSString* configDir = [@"~/Library/Application Support/MacNote++/plugins/Config"
					stringByExpandingTildeInPath];
				[[NSFileManager defaultManager] createDirectoryAtPath:configDir
					withIntermediateDirectories:YES attributes:nil error:nil];
				std::wstring wConfigDir;
				NSData* data = [configDir dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
				if (data)
					wConfigDir = std::wstring(reinterpret_cast<const wchar_t*>(data.bytes),
					                          data.length / sizeof(wchar_t));
				return copyWideToBuffer(wConfigDir, wParam, lParam);
			}
		}

		case NPPM_MENUCOMMAND:
		{
			::SendMessageW(hWnd, WM_COMMAND, static_cast<WPARAM>(lParam), 0);
			return TRUE;
		}

		case NPPM_GETNPPVERSION:
		{
			// Return version as MAKELONG(minor, major)
			// Major = 1, Minor = 0 for MacNote++ 1.0
			int major = 1;
			int minor = 0;
			if (wParam) // ADD_ZERO_PADDING
				minor = 0; // Already zero-padded
			return MAKELONG(minor, major);
		}

		case NPPM_ALLOCATECMDID:
		{
			int* startNumber = reinterpret_cast<int*>(lParam);
			if (!startNumber)
				return FALSE;
			return pluginManager().allocateCmdID(static_cast<int>(wParam), startNumber) ? TRUE : FALSE;
		}

		case NPPM_ALLOCATEMARKER:
		{
			int* startNumber = reinterpret_cast<int*>(lParam);
			if (!startNumber)
				return FALSE;
			return pluginManager().allocateMarker(static_cast<int>(wParam), startNumber) ? TRUE : FALSE;
		}

		case NPPM_GETCURRENTVIEW:
			return static_cast<LRESULT>(ctx().activeView);

		case NPPM_ALLOCATEINDICATOR:
		{
			int* startNumber = reinterpret_cast<int*>(lParam);
			if (!startNumber)
				return FALSE;
			return pluginManager().allocateIndicator(static_cast<int>(wParam), startNumber) ? TRUE : FALSE;
		}

		default:
			NSLog(@"Unhandled NPPM message: 0x%X (offset +%d)", msg, msg - NPPMSG);
			return 0;
	}
}

// ============================================================
// RUNCOMMAND_USER range (WM_USER + 3000)
// ============================================================
LRESULT handleRunCommandMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	std::wstring filePath;

	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
		filePath = docs[tabIdx].filePath;

	switch (msg)
	{
		case NPPM_GETFULLCURRENTPATH:
			return copyWideToBuffer(filePath, wParam, lParam);

		case NPPM_GETCURRENTDIRECTORY:
		{
			if (filePath.empty())
				return copyWideToBuffer(L"", wParam, lParam);
			namespace fs = std::filesystem;
			fs::path p(filePath);
			return copyWideToBuffer(p.parent_path().wstring(), wParam, lParam);
		}

		case NPPM_GETFILENAME:
		{
			if (filePath.empty())
				return copyWideToBuffer(L"", wParam, lParam);
			namespace fs = std::filesystem;
			fs::path p(filePath);
			return copyWideToBuffer(p.filename().wstring(), wParam, lParam);
		}

		case NPPM_GETNAMEPART:
		{
			if (filePath.empty())
				return copyWideToBuffer(L"", wParam, lParam);
			namespace fs = std::filesystem;
			fs::path p(filePath);
			return copyWideToBuffer(p.stem().wstring(), wParam, lParam);
		}

		case NPPM_GETEXTPART:
		{
			if (filePath.empty())
				return copyWideToBuffer(L"", wParam, lParam);
			namespace fs = std::filesystem;
			fs::path p(filePath);
			return copyWideToBuffer(p.extension().wstring(), wParam, lParam);
		}

		case NPPM_GETCURRENTWORD:
		{
			void* sci = ctx().activeScintillaView();
			if (!sci)
				return copyWideToBuffer(L"", wParam, lParam);

			// If there is a selection, return the selected text (matches upstream behavior).
			// Only fall back to word-boundary logic when there is no selection.
			intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
			intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);

			intptr_t rangeStart, rangeEnd;
			if (selEnd > selStart)
			{
				rangeStart = selStart;
				rangeEnd = selEnd;
			}
			else
			{
				intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
				rangeStart = ScintillaBridge_sendMessage(sci, SCI_WORDSTARTPOSITION, pos, 1);
				rangeEnd = ScintillaBridge_sendMessage(sci, SCI_WORDENDPOSITION, pos, 1);
			}

			if (rangeEnd <= rangeStart)
				return copyWideToBuffer(L"", wParam, lParam);

			intptr_t len = rangeEnd - rangeStart;
			std::string utf8(static_cast<size_t>(len) + 1, '\0');
			Sci_TextRangeFull tr{};
			tr.chrg.cpMin = rangeStart;
			tr.chrg.cpMax = rangeEnd;
			tr.lpstrText = utf8.data();
			ScintillaBridge_sendMessage(sci, SCI_GETTEXTRANGEFULL, 0, reinterpret_cast<intptr_t>(&tr));

			// Convert UTF-8 to wide
			@autoreleasepool {
				NSString* str = [NSString stringWithUTF8String:utf8.c_str()];
				if (!str) return copyWideToBuffer(L"", wParam, lParam);
				NSData* data = [str dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
				if (!data) return copyWideToBuffer(L"", wParam, lParam);
				std::wstring wWord(reinterpret_cast<const wchar_t*>(data.bytes),
				                   data.length / sizeof(wchar_t));
				return copyWideToBuffer(wWord, wParam, lParam);
			}
		}

		case NPPM_GETCURRENTLINE:
		{
			void* sci = ctx().activeScintillaView();
			if (!sci) return 0;
			intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
			return static_cast<LRESULT>(ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, pos, 0));
		}

		case NPPM_GETCURRENTCOLUMN:
		{
			void* sci = ctx().activeScintillaView();
			if (!sci) return 0;
			intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
			return static_cast<LRESULT>(ScintillaBridge_sendMessage(sci, SCI_GETCOLUMN, pos, 0));
		}

		default:
			NSLog(@"Unhandled RUNCOMMAND_USER message: 0x%X (offset +%d)", msg, msg - RUNCOMMAND_USER);
			return 0;
	}
}
