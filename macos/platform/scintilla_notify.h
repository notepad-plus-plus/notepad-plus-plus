#pragma once
#include <cstdint>

// Shared Scintilla notification struct matching SCNotification layout.
// Used by both app_delegate.mm and split_view.mm notification callbacks.

struct SciNotifyHeader
{
	void* hwndFrom;
	uintptr_t idFrom;
	unsigned int code;
};

struct SciNotification
{
	SciNotifyHeader nmhdr;
	intptr_t position;
	int ch;
	int modifiers;
	int modificationType;
	const char* text;
	intptr_t length;
	intptr_t linesAdded;
	int message;
	uintptr_t wParam;
	intptr_t lParam;
	intptr_t line;
	int foldLevelNow;
	int foldLevelPrev;
	int margin;
	int listType;
	int x;
	int y;
	int token;
	intptr_t annotationLinesAdded;
	int updated;
};
