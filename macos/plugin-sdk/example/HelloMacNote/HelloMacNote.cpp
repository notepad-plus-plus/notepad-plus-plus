// HelloMacNote — Sample Notepad++ plugin
// Compiles on both Windows (.dll) and macOS (.dylib) without #ifdef guards.

#include "PluginInterface.h"
#include <ctime>

static NppData nppData;
static const int NB_FUNC = 3;
static FuncItem funcItems[NB_FUNC];

// ============================================================
// Plugin commands
// ============================================================

static void helloWorld()
{
	::MessageBox(nppData._nppHandle, L"Hello from MacNote++ plugin!", L"HelloMacNote", MB_OK);
}

static void insertTimestamp()
{
	// Get the current Scintilla view
	int which = 0;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&which));
	HWND sci = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

	// Format current date/time and insert at cursor
	time_t now = time(nullptr);
	struct tm* lt = localtime(&now);
	char buf[64];
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
	::SendMessage(sci, SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(buf));
}

static void showVersion()
{
	LRESULT ver = ::SendMessage(nppData._nppHandle, NPPM_GETNPPVERSION, TRUE, 0);
	int major = HIWORD(ver);
	int minor = LOWORD(ver);

	wchar_t msg[128];
	swprintf(msg, 128, L"MacNote++ version: %d.%d", major, minor);
	::MessageBox(nppData._nppHandle, msg, L"Version", MB_OK);
}

// ============================================================
// Required plugin exports
// ============================================================

extern "C" __declspec(dllexport) void setInfo(NppData nd)
{
	nppData = nd;

	// Initialize function items
	wcscpy(funcItems[0]._itemName, L"Hello World");
	funcItems[0]._pFunc = helloWorld;

	wcscpy(funcItems[1]._itemName, L"Insert Timestamp");
	funcItems[1]._pFunc = insertTimestamp;

	wcscpy(funcItems[2]._itemName, L"Show Version");
	funcItems[2]._pFunc = showVersion;
}

extern "C" __declspec(dllexport) const wchar_t* getName()
{
	return L"HelloMacNote";
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbItems)
{
	*nbItems = NB_FUNC;
	return funcItems;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* scn)
{
	if (scn->nmhdr.code == NPPN_READY)
	{
		// Plugin is ready
	}
	else if (scn->nmhdr.code == NPPN_SHUTDOWN)
	{
		// Cleanup before shutdown
	}
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
