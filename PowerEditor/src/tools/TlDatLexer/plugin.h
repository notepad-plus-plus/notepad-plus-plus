#include <Windows.h>

#include <Scintilla.h>

const wchar_t* name = L"TlDatLexer";

HWND nppHandle;
HWND scintillaHandle1;
HWND scintillaHandle2;

HWND getCurrentScintilla();
bool isTlDatLexerActive();
sptr_t getDirectScintillaPtr(void* handle);
SciFnDirect getDirectScintillaFunc(void* handle);
