#include "stdafx.h"
#include "plugin.h"

#include <cstring>

#include <PluginInterface.h>
#include <SciLexer.h>

#include "lexer.h"
#include "version.h"

///// Helper Functions /////

HWND getCurrentScintilla() {
	int which;
	SendMessage(reinterpret_cast<HWND>(nppHandle), NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&which));
	return which == 0? scintillaHandle1: scintillaHandle2;
}

bool isTlDatLexerActive() {
	int id;
	SendMessage(nppHandle, NPPM_GETCURRENTLANGTYPE, 0, reinterpret_cast<LPARAM>(&id));
	std::size_t nameLen = SendMessage(nppHandle, NPPM_GETLANGUAGENAME, id, reinterpret_cast<LPARAM>(nullptr));
	if(nameLen != TlDatLexer::nameLen) return false;

	wchar_t name[TlDatLexer::nameLen + 1];
	SendMessage(nppHandle, NPPM_GETLANGUAGENAME, id, reinterpret_cast<LPARAM>(name));
	return std::wcscmp(name, TlDatLexer::wname) == 0;
}

sptr_t getDirectScintillaPtr(void* handle) {
	return SendMessage(static_cast<HWND>(handle), SCI_GETDIRECTPOINTER, 0, 0);
}

SciFnDirect getDirectScintillaFunc(void* handle) {
	return reinterpret_cast<SciFnDirect>(SendMessage(static_cast<HWND>(handle), SCI_GETDIRECTFUNCTION, 0, 0));
}

///// Notepad++ exports /////

namespace {

void __cdecl about() {
	MessageBox(
		nppHandle,
		L"Author: Jason Newcomb.\n"
		L"Source: http://bitbucket.org/Jarcho/tldatlexer\n\n"
		L"This is free software; it comes with ABSOLUTELY NO WARRANTY.\n",
		L"TLDatLexer (v" TEXT(TLDATLEXER_VERSION_STRING) L")",
		MB_OK
	);
}

}

FuncItem* getFuncsArray(int* count) {
	static FuncItem funcs[] = { { L"About", &about, 0, false, nullptr } };
	
	*count = sizeof(funcs) / sizeof(FuncItem);
	return funcs;
}

BOOL isUnicode() { return TRUE; }
const wchar_t* getName() { return name; }
LRESULT messageProc(UINT, WPARAM, LPARAM) { return TRUE; }

void setInfo(NppData data) {
	nppHandle = data._nppHandle;
	scintillaHandle1 = data._scintillaMainHandle;
	scintillaHandle2 = data._scintillaSecondHandle;
}

void beNotified(SCNotification* notification) {
	// Notepad++ handles this notification last and overwrites tag matching and brace highlighting.
	// This is used to know when to react to the tag mathcing indicator being removed.
	static bool doTagMatch = false;

	switch(notification->nmhdr.code) {
	case SCN_UPDATEUI: {
		if(!isTlDatLexerActive()) break;
		if(SendMessage(static_cast<HWND>(notification->nmhdr.hwndFrom), SCI_GETLENGTH, 0, 0) == 0) break;

		// Ensure the indicator is set, otherwise the SCN_MODIFIED message won't be set.
		SendMessage(static_cast<HWND>(notification->nmhdr.hwndFrom), SCI_SETINDICATORCURRENT, SCE_UNIVERSAL_TAGMATCH, 0);
		SendMessage(static_cast<HWND>(notification->nmhdr.hwndFrom), SCI_INDICATORFILLRANGE, 0, 1);
		doTagMatch = true;
	} break;
	case SCN_MODIFIED: {
		if((notification->modificationType & SC_MOD_CHANGEINDICATOR) == 0) break;
		if(!doTagMatch) break;

		doTagMatch = false;
		auto scintilla = getDirectScintillaPtr(notification->nmhdr.hwndFrom);
		auto message = getDirectScintillaFunc(notification->nmhdr.hwndFrom);
		unsigned int pos = message(scintilla, SCI_GETCURRENTPOS, 0, 0);
		matchTags(pos, scintilla, message);
	} break;
	case NPPN_FILEOPENED:
	case NPPN_LANGCHANGED: {
		if(!isTlDatLexerActive()) break;

		HWND scintilla = getCurrentScintilla();
		SendMessage(scintilla, SCI_INDICSETSTYLE, IndicatorStyle::error, INDIC_SQUIGGLE);
		SendMessage(scintilla, SCI_INDICSETFORE, IndicatorStyle::error, 0x0000D0);
	} break;
	}
}

///// Scintilla Exports /////

int SCI_METHOD GetLexerCount() { return 1; }

void SCI_METHOD GetLexerName(unsigned int index, char* name, int len) {
	if(index == 0) {
		std::strncpy(name, TlDatLexer::name, len);
		name[len - 1] = '\0';
	}
}

void SCI_METHOD GetLexerStatusText(unsigned int index, wchar_t* desc, int len) {
	if(index == 0) {
		std::wcsncpy(desc, TlDatLexer::statusText, len);
		desc[len - 1] = L'\0';
	}
}

LexerFactoryFunction SCI_METHOD GetLexerFactory(unsigned int index) {
	return index == 0? TlDatLexer::factory: nullptr;
}
