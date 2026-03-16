// find_replace.mm — Find/Replace dialog and search logic
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "find_replace.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "scintilla_bridge.h"
#include "handle_registry.h"
#include "windows.h"

void updateFindStatus(const wchar_t* msg)
{
	if (ctx().findDlgHwnd)
		SetDlgItemTextW(ctx().findDlgHwnd, IDC_FIND_STATUS, msg);
}

int buildSearchFlags()
{
	int flags = 0;
	if (ctx().matchCase) flags |= SCFIND_MATCHCASE;
	if (ctx().wholeWord) flags |= SCFIND_WHOLEWORD;
	if (ctx().useRegex)  flags |= SCFIND_REGEXP | SCFIND_CXX11REGEX;
	return flags;
}

bool doFindNext(bool forward)
{
	void* sci = ctx().activeScintillaView();
	if (!sci || ctx().findText.empty()) return false;

	NSString* nsFind = WideToNSString(ctx().findText.c_str());
	const char* utf8Find = [nsFind UTF8String];

	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, buildSearchFlags(), 0);

	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);

	if (forward)
	{
		intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selEnd, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, docLen, 0);
	}
	else
	{
		intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, 0, 0);
	}

	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
	                                            strlen(utf8Find), (intptr_t)utf8Find);
	if (pos >= 0)
	{
		intptr_t targetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);
		if (targetEnd == pos && pos < docLen)
			targetEnd = pos + 1;
		ScintillaBridge_sendMessage(sci, SCI_SETSEL, pos, targetEnd);
		ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);
		updateFindStatus(L"Match found");
		return true;
	}
	else
	{
		if (forward)
		{
			ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, 0, 0);
			intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
			ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
		}
		else
		{
			ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, docLen, 0);
			intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
			ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selStart, 0);
		}

		pos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
		                                  strlen(utf8Find), (intptr_t)utf8Find);
		if (pos >= 0)
		{
			intptr_t targetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);
			if (targetEnd == pos && pos < docLen)
				targetEnd = pos + 1;
			ScintillaBridge_sendMessage(sci, SCI_SETSEL, pos, targetEnd);
			ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);
			updateFindStatus(L"Wrapped around");
			return true;
		}
	}

	if (ctx().useRegex)
		updateFindStatus(L"Not found (check regex syntax)");
	else
		updateFindStatus(L"Not found");
	return false;
}

int doCount()
{
	void* sci = ctx().activeScintillaView();
	if (!sci || ctx().findText.empty()) return 0;

	NSString* nsFind = WideToNSString(ctx().findText.c_str());
	const char* utf8Find = [nsFind UTF8String];

	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, buildSearchFlags(), 0);

	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	int count = 0;
	intptr_t searchStart = 0;
	size_t findLen = strlen(utf8Find);

	while (searchStart < docLen)
	{
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, searchStart, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, docLen, 0);
		intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
		                                            findLen, (intptr_t)utf8Find);
		if (pos < 0) break;
		++count;
		intptr_t targetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);
		if (targetEnd <= pos)
		{
			if (pos < docLen)
			{
				searchStart = pos + 1;
				continue;
			}
			break;
		}
		searchStart = targetEnd;
	}

	wchar_t buf[64];
	swprintf(buf, 64, L"%d matches found", count);
	updateFindStatus(buf);
	return count;
}

void doReplaceOne()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);

	if (selStart == selEnd)
	{
		doFindNext(true);
		return;
	}

	NSString* nsReplace = WideToNSString(ctx().replaceText.c_str());
	const char* utf8Replace = [nsReplace UTF8String];

	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET,
	                            strlen(utf8Replace), (intptr_t)utf8Replace);

	doFindNext(true);
	updateFindStatus(L"Replaced 1 occurrence");
}

void doReplaceAll()
{
	void* sci = ctx().activeScintillaView();
	if (!sci || ctx().findText.empty()) return;

	NSString* nsFind = WideToNSString(ctx().findText.c_str());
	NSString* nsReplace = WideToNSString(ctx().replaceText.c_str());
	const char* utf8Find = [nsFind UTF8String];
	const char* utf8Replace = [nsReplace UTF8String];

	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, buildSearchFlags(), 0);

	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	int count = 0;
	intptr_t searchStart = 0;
	size_t findLen = strlen(utf8Find);
	size_t replaceLen = strlen(utf8Replace);

	while (searchStart < docLen)
	{
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, searchStart, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, docLen, 0);
		intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
		                                            findLen, (intptr_t)utf8Find);
		if (pos < 0) break;

		ScintillaBridge_sendMessage(sci, SCI_REPLACETARGET,
		                            replaceLen, (intptr_t)utf8Replace);
		++count;

		searchStart = pos + static_cast<intptr_t>(replaceLen);
		docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
		if (searchStart <= pos)
		{
			if (searchStart < docLen)
				++searchStart;
			else
				break;
		}
	}

	wchar_t buf[64];
	swprintf(buf, 64, L"Replaced %d occurrences", count);
	updateFindStatus(buf);
}

void readFindDlgState()
{
	if (!ctx().findDlgHwnd) return;

	wchar_t buf[1024];
	GetDlgItemTextW(ctx().findDlgHwnd, IDC_FIND_EDIT, buf, 1024);
	ctx().findText = buf;

	GetDlgItemTextW(ctx().findDlgHwnd, IDC_REPLACE_EDIT, buf, 1024);
	ctx().replaceText = buf;

	ctx().matchCase = (IsDlgButtonChecked(ctx().findDlgHwnd, IDC_MATCH_CASE) == BST_CHECKED);
	ctx().wholeWord = (IsDlgButtonChecked(ctx().findDlgHwnd, IDC_WHOLE_WORD) == BST_CHECKED);
	ctx().useRegex  = (IsDlgButtonChecked(ctx().findDlgHwnd, IDC_USE_REGEX) == BST_CHECKED);
}

void createFindReplaceDlg(bool replaceMode)
{
	ctx().findMode = !replaceMode;

	if (ctx().findDlgHwnd)
	{
		auto* info = HandleRegistry::getWindowInfo(ctx().findDlgHwnd);
		if (info && info->nativeWindow)
		{
			NSWindow* win = (__bridge NSWindow*)info->nativeWindow;

			int dlgHeight = replaceMode ? 270 : 210;
			NSRect contentRect = [win contentRectForFrameRect:win.frame];
			CGFloat heightDiff = dlgHeight - contentRect.size.height;
			NSRect newFrame = win.frame;
			newFrame.origin.y -= heightDiff;
			newFrame.size.height += heightDiff;
			[win setFrame:newFrame display:YES];

			int dlgWidth = 450;
			int cbY = replaceMode ? 75 : 45;
			int regexY = replaceMode ? 100 : 70;
			int btnY = replaceMode ? 135 : 100;
			int rBtnY = 170;
			int statusY = replaceMode ? 240 : 150;
			int btnW = 90;
			int btnH = 28;
			int btnGap = 8;

			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_FIND_LABEL), 15, 15, 80, 20, TRUE);
			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_FIND_EDIT), 100, 12, 230, 24, TRUE);

			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_REPLACE_LABEL), 15, 45, 80, 20, TRUE);
			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_REPLACE_EDIT), 100, 42, 230, 24, TRUE);

			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_MATCH_CASE), 15, cbY, 120, 20, TRUE);
			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_WHOLE_WORD), 145, cbY, 120, 20, TRUE);
			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_USE_REGEX), 15, regexY, 160, 20, TRUE);

			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_FIND_NEXT), 15, btnY, btnW, btnH, TRUE);
			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_FIND_PREV), 15 + btnW + btnGap, btnY, btnW, btnH, TRUE);
			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_FIND_COUNT), 15 + 2 * (btnW + btnGap), btnY, 70, btnH, TRUE);
			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_FIND_CLOSE), dlgWidth - 85, btnY, 70, btnH, TRUE);

			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_REPLACE_ONE), 15, rBtnY, btnW, btnH, TRUE);
			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_REPLACE_ALL), 15 + btnW + btnGap, rBtnY, btnW + 10, btnH, TRUE);

			MoveWindow(GetDlgItem(ctx().findDlgHwnd, IDC_FIND_STATUS), 15, statusY, dlgWidth - 30, 20, TRUE);

			ShowWindow(GetDlgItem(ctx().findDlgHwnd, IDC_REPLACE_EDIT), replaceMode ? SW_SHOW : SW_HIDE);
			ShowWindow(GetDlgItem(ctx().findDlgHwnd, IDC_REPLACE_LABEL), replaceMode ? SW_SHOW : SW_HIDE);
			ShowWindow(GetDlgItem(ctx().findDlgHwnd, IDC_REPLACE_ONE), replaceMode ? SW_SHOW : SW_HIDE);
			ShowWindow(GetDlgItem(ctx().findDlgHwnd, IDC_REPLACE_ALL), replaceMode ? SW_SHOW : SW_HIDE);

			[win setTitle:replaceMode ? @"Replace" : @"Find"];
			[win makeKeyAndOrderFront:nil];
		}
		return;
	}

	int dlgWidth = 450;
	int dlgHeight = replaceMode ? 270 : 210;

	NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
	NSRect contentRect = NSMakeRect(0, 0, dlgWidth, dlgHeight);
	NSPanel* panel = [[NSPanel alloc] initWithContentRect:contentRect
	                                    styleMask:styleMask
	                                    backing:NSBackingStoreBuffered
	                                    defer:NO];
	[panel setTitle:replaceMode ? @"Replace" : @"Find"];
	[panel setReleasedWhenClosed:NO];
	[panel setFloatingPanel:YES];
	[panel setBecomesKeyOnlyIfNeeded:YES];

	if (ctx().mainWindow)
	{
		NSRect mainFrame = ctx().mainWindow.frame;
		CGFloat x = mainFrame.origin.x + (mainFrame.size.width - dlgWidth) / 2;
		CGFloat y = mainFrame.origin.y + (mainFrame.size.height - dlgHeight) / 2;
		[panel setFrameOrigin:NSMakePoint(x, y)];
	}
	else
	{
		[panel center];
	}

	HandleRegistry::WindowInfo dlgInfo;
	dlgInfo.className = L"#32770";
	dlgInfo.windowName = replaceMode ? L"Replace" : L"Find";
	dlgInfo.style = WS_POPUP | WS_CAPTION;
	dlgInfo.parent = ctx().mainHwnd;
	dlgInfo.nativeWindow = (__bridge void*)panel;
	dlgInfo.nativeView = (__bridge void*)[panel contentView];

	ctx().findDlgHwnd = HandleRegistry::createWindow(dlgInfo);

	CreateWindowExW(0, L"Static", L"Find:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		15, 15, 80, 20,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_FIND_LABEL), nullptr, nullptr);

	CreateWindowExW(0, L"Edit", L"",
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		100, 12, 230, 24,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_FIND_EDIT), nullptr, nullptr);

	CreateWindowExW(0, L"Static", L"Replace:",
		WS_CHILD | (replaceMode ? WS_VISIBLE : 0) | SS_LEFT,
		15, 45, 80, 20,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_REPLACE_LABEL), nullptr, nullptr);

	CreateWindowExW(0, L"Edit", L"",
		WS_CHILD | (replaceMode ? WS_VISIBLE : 0) | ES_AUTOHSCROLL,
		100, 42, 230, 24,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_REPLACE_EDIT), nullptr, nullptr);

	int cbY = replaceMode ? 75 : 45;
	CreateWindowExW(0, L"Button", L"Match case",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		15, cbY, 120, 20,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_MATCH_CASE), nullptr, nullptr);

	CreateWindowExW(0, L"Button", L"Whole word",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		145, cbY, 120, 20,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_WHOLE_WORD), nullptr, nullptr);

	int regexY = replaceMode ? 100 : 70;
	CreateWindowExW(0, L"Button", L"Regular expression",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		15, regexY, 160, 20,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_USE_REGEX), nullptr, nullptr);

	int btnY = replaceMode ? 135 : 100;
	int btnW = 90;
	int btnH = 28;
	int btnGap = 8;

	CreateWindowExW(0, L"Button", L"Find Next",
		WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
		15, btnY, btnW, btnH,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_FIND_NEXT), nullptr, nullptr);

	CreateWindowExW(0, L"Button", L"Find Prev",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		15 + btnW + btnGap, btnY, btnW, btnH,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_FIND_PREV), nullptr, nullptr);

	CreateWindowExW(0, L"Button", L"Count",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		15 + 2 * (btnW + btnGap), btnY, 70, btnH,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_FIND_COUNT), nullptr, nullptr);

	CreateWindowExW(0, L"Button", L"Close",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		dlgWidth - 85, btnY, 70, btnH,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_FIND_CLOSE), nullptr, nullptr);

	int rBtnY = 170;
	CreateWindowExW(0, L"Button", L"Replace",
		WS_CHILD | (replaceMode ? WS_VISIBLE : 0) | BS_PUSHBUTTON,
		15, rBtnY, btnW, btnH,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_REPLACE_ONE), nullptr, nullptr);

	CreateWindowExW(0, L"Button", L"Replace All",
		WS_CHILD | (replaceMode ? WS_VISIBLE : 0) | BS_PUSHBUTTON,
		15 + btnW + btnGap, rBtnY, btnW + 10, btnH,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_REPLACE_ALL), nullptr, nullptr);

	int statusY = replaceMode ? 240 : 150;
	CreateWindowExW(0, L"Static", L"",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		15, statusY, dlgWidth - 30, 20,
		ctx().findDlgHwnd, reinterpret_cast<HMENU>(IDC_FIND_STATUS), nullptr, nullptr);

	auto* findDlgInfo = HandleRegistry::getWindowInfo(ctx().findDlgHwnd);
	if (findDlgInfo)
	{
		findDlgInfo->wndProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
			if (msg == WM_COMMAND)
			{
				UINT cmdId = LOWORD(wParam);
				UINT notif = HIWORD(wParam);

				if (notif == BN_CLICKED)
				{
					switch (cmdId)
					{
						case IDC_FIND_NEXT:
							readFindDlgState();
							doFindNext(true);
							return 0;
						case IDC_FIND_PREV:
							readFindDlgState();
							doFindNext(false);
							return 0;
						case IDC_FIND_COUNT:
							readFindDlgState();
							doCount();
							return 0;
						case IDC_REPLACE_ONE:
							readFindDlgState();
							doReplaceOne();
							return 0;
						case IDC_REPLACE_ALL:
							readFindDlgState();
							doReplaceAll();
							return 0;
						case IDC_FIND_CLOSE:
						{
							auto* info = HandleRegistry::getWindowInfo(hWnd);
							if (info && info->nativeWindow)
							{
								NSWindow* win = (__bridge NSWindow*)info->nativeWindow;
								[win orderOut:nil];
							}
							return 0;
						}
					}
				}
			}
			return DefWindowProcW(hWnd, msg, wParam, lParam);
		};
	}

	// Pre-fill find text from current selection
	{
		void* sci = ctx().activeScintillaView();
		if (sci)
		{
			intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
			intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
			if (selEnd > selStart && (selEnd - selStart) < 256)
			{
				char utf8Buf[512] = {};
				ScintillaBridge_sendMessage(sci, SCI_GETSELTEXT, 0, (intptr_t)utf8Buf);
				NSString* sel = [NSString stringWithUTF8String:utf8Buf];
				if (sel.length > 0)
				{
					NSData* data = [sel dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
					if (data && data.length > 0)
					{
						std::wstring wsel(reinterpret_cast<const wchar_t*>(data.bytes),
						                  data.length / sizeof(wchar_t));
						SetDlgItemTextW(ctx().findDlgHwnd, IDC_FIND_EDIT, wsel.c_str());
					}
				}
			}
		}
	}

	[panel makeKeyAndOrderFront:nil];
}

void showGoToLineDlg()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
	intptr_t curPos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	intptr_t curLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, curPos, 0);

	@autoreleasepool {
		NSAlert* alert = [[NSAlert alloc] init];
		[alert setMessageText:@"Go To Line"];
		[alert setInformativeText:[NSString stringWithFormat:
			@"Enter line number (1-%ld). Current line: %ld",
			(long)lineCount, (long)(curLine + 1)]];
		[alert addButtonWithTitle:@"Go"];
		[alert addButtonWithTitle:@"Cancel"];

		NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
		input.stringValue = [NSString stringWithFormat:@"%ld", (long)(curLine + 1)];
		[alert setAccessoryView:input];
		[alert.window setInitialFirstResponder:input];

		NSModalResponse response = [alert runModal];
		if (response == NSAlertFirstButtonReturn)
		{
			int lineNum = input.intValue;
			if (lineNum > 0 && lineNum <= static_cast<int>(lineCount))
			{
				ScintillaBridge_sendMessage(sci, SCI_GOTOLINE, lineNum - 1, 0);
				ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);
			}
		}
	}
}
