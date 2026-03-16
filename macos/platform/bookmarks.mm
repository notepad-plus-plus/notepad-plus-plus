// bookmarks.mm — Bookmark toggle/navigation
// Part of the Notepad++ macOS port modular refactor.

#include "bookmarks.h"
#include "npp_constants.h"
#include "app_state.h"
#include "scintilla_bridge.h"

void toggleBookmark()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	intptr_t line = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, pos, 0);

	intptr_t markers = ScintillaBridge_sendMessage(sci, SCI_MARKERGET, line, 0);
	if (markers & BOOKMARK_MASK)
		ScintillaBridge_sendMessage(sci, SCI_MARKERDELETE, line, BOOKMARK_MARKER);
	else
		ScintillaBridge_sendMessage(sci, SCI_MARKERADD, line, BOOKMARK_MARKER);
}

void nextBookmark()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	intptr_t curLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, pos, 0);

	intptr_t nextLine = ScintillaBridge_sendMessage(sci, SCI_MARKERNEXT, curLine + 1, BOOKMARK_MASK);
	if (nextLine < 0)
		nextLine = ScintillaBridge_sendMessage(sci, SCI_MARKERNEXT, 0, BOOKMARK_MASK);

	if (nextLine >= 0)
	{
		ScintillaBridge_sendMessage(sci, SCI_GOTOLINE, nextLine, 0);
		ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);
	}
}

void prevBookmark()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	intptr_t curLine = ScintillaBridge_sendMessage(sci, SCI_LINEFROMPOSITION, pos, 0);

	intptr_t prevLine = ScintillaBridge_sendMessage(sci, SCI_MARKERPREVIOUS, curLine - 1, BOOKMARK_MASK);
	if (prevLine < 0)
	{
		intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
		prevLine = ScintillaBridge_sendMessage(sci, SCI_MARKERPREVIOUS, lineCount - 1, BOOKMARK_MASK);
	}

	if (prevLine >= 0)
	{
		ScintillaBridge_sendMessage(sci, SCI_GOTOLINE, prevLine, 0);
		ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);
	}
}

void clearAllBookmarks()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;
	ScintillaBridge_sendMessage(sci, SCI_MARKERDELETEALL, BOOKMARK_MARKER, 0);
}
