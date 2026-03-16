// status_bar.mm — Status bar update
// Part of the Notepad++ macOS port modular refactor.

#include "status_bar.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "language_defs.h"
#include "scintilla_bridge.h"
#include "windows.h"
#include "commctrl.h"

void updateStatusBar()
{
	void* sci = ctx().activeScintillaView();
	if (!sci || !ctx().statusBarHwnd) return;

	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();

	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	intptr_t line = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, pos, 0);
	intptr_t col = ScintillaBridge_sendMessage(sci, SCI_GETCOLUMN, pos, 0);
	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);

	wchar_t buf[128];
	swprintf(buf, 128, L"Ln %ld, Col %ld", (long)(line + 1), (long)(col + 1));
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(buf));

	swprintf(buf, 128, L"%ld lines, %ld bytes", (long)lineCount, (long)docLen);
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(buf));

	const char* langName = "Normal Text";
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
	{
		int langIdx = docs[tabIdx].languageIndex;
		if (langIdx >= 0 && langIdx < g_numLanguages)
			langName = g_languages[langIdx].name;
	}
	NSString* nsLang = [NSString stringWithUTF8String:langName];
	std::wstring wLang = NSStringToWide(nsLang);
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(wLang.c_str()));

	const char* encName = "UTF-8";
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
		encName = encodingName(docs[tabIdx].encoding);
	NSString* nsEnc = [NSString stringWithUTF8String:encName];
	std::wstring wEnc = NSStringToWide(nsEnc);
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(wEnc.c_str()));

	const char* eolN = "LF";
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
		eolN = eolName(docs[tabIdx].eolMode);
	NSString* nsEol = [NSString stringWithUTF8String:eolN];
	std::wstring wEol = NSStringToWide(nsEol);
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 4, reinterpret_cast<LPARAM>(wEol.c_str()));

	swprintf(buf, 128, L"Doc %d/%d", tabIdx + 1, (int)docs.size());
	SendMessageW(ctx().statusBarHwnd, SB_SETTEXTW, 5, reinterpret_cast<LPARAM>(buf));
}
