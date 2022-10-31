// Scintilla source code edit control
/** @file ScintillaStructures.h
 ** Structures used to communicate with Scintilla.
 ** The same structures are defined for C in Scintilla.h.
 ** Uses definitions from ScintillaTypes.h.
 **/
// Copyright 2021 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SCINTILLASTRUCTURES_H
#define SCINTILLASTRUCTURES_H

namespace Scintilla {

using PositionCR = long;

struct CharacterRange {
	PositionCR cpMin;
	PositionCR cpMax;
};

struct CharacterRangeFull {
	Position cpMin;
	Position cpMax;
};

struct TextRange {
	CharacterRange chrg;
	char *lpstrText;
};

struct TextRangeFull {
	CharacterRangeFull chrg;
	char *lpstrText;
};

struct TextToFind {
	CharacterRange chrg;
	const char *lpstrText;
	CharacterRange chrgText;
};

struct TextToFindFull {
	CharacterRangeFull chrg;
	const char *lpstrText;
	CharacterRangeFull chrgText;
};

using SurfaceID = void *;

struct Rectangle {
	int left;
	int top;
	int right;
	int bottom;
};

/* This structure is used in printing. Not needed by most client code. */

struct RangeToFormat {
	SurfaceID hdc;
	SurfaceID hdcTarget;
	Rectangle rc;
	Rectangle rcPage;
	CharacterRange chrg;
};

struct RangeToFormatFull {
	SurfaceID hdc;
	SurfaceID hdcTarget;
	Rectangle rc;
	Rectangle rcPage;
	CharacterRangeFull chrg;
};

struct NotifyHeader {
	/* Compatible with Windows NMHDR.
	 * hwndFrom is really an environment specific window handle or pointer
	 * but most clients of Scintilla.h do not have this type visible. */
	void *hwndFrom;
	uptr_t idFrom;
	Notification code;
};

enum class Message;	// Declare in case ScintillaMessages.h not included

struct NotificationData {
	NotifyHeader nmhdr;
	Position position;
	/* SCN_STYLENEEDED, SCN_DOUBLECLICK, SCN_MODIFIED, SCN_MARGINCLICK, */
	/* SCN_NEEDSHOWN, SCN_DWELLSTART, SCN_DWELLEND, SCN_CALLTIPCLICK, */
	/* SCN_HOTSPOTCLICK, SCN_HOTSPOTDOUBLECLICK, SCN_HOTSPOTRELEASECLICK, */
	/* SCN_INDICATORCLICK, SCN_INDICATORRELEASE, */
	/* SCN_USERLISTSELECTION, SCN_AUTOCSELECTION */

	int ch;
	/* SCN_CHARADDED, SCN_KEY, SCN_AUTOCCOMPLETED, SCN_AUTOCSELECTION, */
	/* SCN_USERLISTSELECTION */
	KeyMod modifiers;
	/* SCN_KEY, SCN_DOUBLECLICK, SCN_HOTSPOTCLICK, SCN_HOTSPOTDOUBLECLICK, */
	/* SCN_HOTSPOTRELEASECLICK, SCN_INDICATORCLICK, SCN_INDICATORRELEASE, */

	ModificationFlags modificationType;	/* SCN_MODIFIED */
	const char *text;
	/* SCN_MODIFIED, SCN_USERLISTSELECTION, SCN_AUTOCSELECTION, SCN_URIDROPPED */

	Position length;		/* SCN_MODIFIED */
	Position linesAdded;	/* SCN_MODIFIED */
	Message message;	/* SCN_MACRORECORD */
	uptr_t wParam;	/* SCN_MACRORECORD */
	sptr_t lParam;	/* SCN_MACRORECORD */
	Position line;		/* SCN_MODIFIED */
	FoldLevel foldLevelNow;	/* SCN_MODIFIED */
	FoldLevel foldLevelPrev;	/* SCN_MODIFIED */
	int margin;		/* SCN_MARGINCLICK */
	int listType;	/* SCN_USERLISTSELECTION */
	int x;			/* SCN_DWELLSTART, SCN_DWELLEND */
	int y;		/* SCN_DWELLSTART, SCN_DWELLEND */
	int token;		/* SCN_MODIFIED with SC_MOD_CONTAINER */
	Position annotationLinesAdded;	/* SCN_MODIFIED with SC_MOD_CHANGEANNOTATION */
	Update updated;	/* SCN_UPDATEUI */
	CompletionMethods listCompletionMethod;
	/* SCN_AUTOCSELECTION, SCN_AUTOCCOMPLETED, SCN_USERLISTSELECTION, */
	CharacterSource characterSource;	/* SCN_CHARADDED */
};

}

#endif
