// Scintilla source code edit control
/** @file KeyMap.cxx
 ** Defines a mapping between keystrokes and commands.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>

#include <vector>

#include "Platform.h"

#include "Scintilla.h"

#include "KeyMap.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

KeyMap::KeyMap() {
	for (int i = 0; MapDefault[i].key; i++) {
		AssignCmdKey(MapDefault[i].key,
			MapDefault[i].modifiers,
			MapDefault[i].msg);
	}
}

KeyMap::~KeyMap() {
	Clear();
}

void KeyMap::Clear() {
	kmap.clear();
}

void KeyMap::AssignCmdKey(int key, int modifiers, unsigned int msg) {
	for (size_t keyIndex = 0; keyIndex < kmap.size(); keyIndex++) {
		if ((key == kmap[keyIndex].key) && (modifiers == kmap[keyIndex].modifiers)) {
			kmap[keyIndex].msg = msg;
			return;
		}
	}
	KeyToCommand ktc;
	ktc.key = key;
	ktc.modifiers = modifiers;
	ktc.msg = msg;
	kmap.push_back(ktc);
}

unsigned int KeyMap::Find(int key, int modifiers) const {
	for (size_t i = 0; i < kmap.size(); i++) {
		if ((key == kmap[i].key) && (modifiers == kmap[i].modifiers)) {
			return kmap[i].msg;
		}
	}
	return 0;
}

#if PLAT_GTK_MACOSX
#define OS_X_KEYS 1
#else
#define OS_X_KEYS 0
#endif

// Define a modifier that is exactly Ctrl key on all platforms
// Most uses of Ctrl map to Cmd on OS X but some can't so use SCI_[S]CTRL_META
#if OS_X_KEYS
#define SCI_CTRL_META SCI_META
#define SCI_SCTRL_META (SCI_META | SCI_SHIFT)
#else
#define SCI_CTRL_META SCI_CTRL
#define SCI_SCTRL_META (SCI_CTRL | SCI_SHIFT)
#endif

const KeyToCommand KeyMap::MapDefault[] = {

#if OS_X_KEYS
    {SCK_DOWN,		SCI_CTRL,	SCI_DOCUMENTEND},
    {SCK_DOWN,		SCI_CSHIFT,	SCI_DOCUMENTENDEXTEND},
    {SCK_UP,		SCI_CTRL,	SCI_DOCUMENTSTART},
    {SCK_UP,		SCI_CSHIFT,	SCI_DOCUMENTSTARTEXTEND},
    {SCK_LEFT,		SCI_CTRL,	SCI_VCHOME},
    {SCK_LEFT,		SCI_CSHIFT,	SCI_VCHOMEEXTEND},
    {SCK_RIGHT,		SCI_CTRL,	SCI_LINEEND},
    {SCK_RIGHT,		SCI_CSHIFT,	SCI_LINEENDEXTEND},
#endif

    {SCK_DOWN,		SCI_NORM,	SCI_LINEDOWN},
    {SCK_DOWN,		SCI_SHIFT,	SCI_LINEDOWNEXTEND},
    {SCK_DOWN,		SCI_CTRL_META,	SCI_LINESCROLLDOWN},
    {SCK_DOWN,		SCI_ASHIFT,	SCI_LINEDOWNRECTEXTEND},
    {SCK_UP,		SCI_NORM,	SCI_LINEUP},
    {SCK_UP,			SCI_SHIFT,	SCI_LINEUPEXTEND},
    {SCK_UP,			SCI_CTRL_META,	SCI_LINESCROLLUP},
    {SCK_UP,		SCI_ASHIFT,	SCI_LINEUPRECTEXTEND},
    {'[',			SCI_CTRL,		SCI_PARAUP},
    {'[',			SCI_CSHIFT,	SCI_PARAUPEXTEND},
    {']',			SCI_CTRL,		SCI_PARADOWN},
    {']',			SCI_CSHIFT,	SCI_PARADOWNEXTEND},
    {SCK_LEFT,		SCI_NORM,	SCI_CHARLEFT},
    {SCK_LEFT,		SCI_SHIFT,	SCI_CHARLEFTEXTEND},
    {SCK_LEFT,		SCI_CTRL_META,	SCI_WORDLEFT},
    {SCK_LEFT,		SCI_SCTRL_META,	SCI_WORDLEFTEXTEND},
    {SCK_LEFT,		SCI_ASHIFT,	SCI_CHARLEFTRECTEXTEND},
    {SCK_RIGHT,		SCI_NORM,	SCI_CHARRIGHT},
    {SCK_RIGHT,		SCI_SHIFT,	SCI_CHARRIGHTEXTEND},
    {SCK_RIGHT,		SCI_CTRL_META,	SCI_WORDRIGHT},
    {SCK_RIGHT,		SCI_SCTRL_META,	SCI_WORDRIGHTEXTEND},
    {SCK_RIGHT,		SCI_ASHIFT,	SCI_CHARRIGHTRECTEXTEND},
    {'/',		SCI_CTRL,		SCI_WORDPARTLEFT},
    {'/',		SCI_CSHIFT,	SCI_WORDPARTLEFTEXTEND},
    {'\\',		SCI_CTRL,		SCI_WORDPARTRIGHT},
    {'\\',		SCI_CSHIFT,	SCI_WORDPARTRIGHTEXTEND},
    {SCK_HOME,		SCI_NORM,	SCI_VCHOME},
    {SCK_HOME, 		SCI_SHIFT, 	SCI_VCHOMEEXTEND},
    {SCK_HOME, 		SCI_CTRL, 	SCI_DOCUMENTSTART},
    {SCK_HOME, 		SCI_CSHIFT, 	SCI_DOCUMENTSTARTEXTEND},
    {SCK_HOME, 		SCI_ALT, 	SCI_HOMEDISPLAY},
    {SCK_HOME,		SCI_ASHIFT,	SCI_VCHOMERECTEXTEND},
    {SCK_END,	 	SCI_NORM,	SCI_LINEEND},
    {SCK_END,	 	SCI_SHIFT, 	SCI_LINEENDEXTEND},
    {SCK_END, 		SCI_CTRL, 	SCI_DOCUMENTEND},
    {SCK_END, 		SCI_CSHIFT, 	SCI_DOCUMENTENDEXTEND},
    {SCK_END, 		SCI_ALT, 	SCI_LINEENDDISPLAY},
    {SCK_END,		SCI_ASHIFT,	SCI_LINEENDRECTEXTEND},
    {SCK_PRIOR,		SCI_NORM,	SCI_PAGEUP},
    {SCK_PRIOR,		SCI_SHIFT, 	SCI_PAGEUPEXTEND},
    {SCK_PRIOR,		SCI_ASHIFT,	SCI_PAGEUPRECTEXTEND},
    {SCK_NEXT, 		SCI_NORM, 	SCI_PAGEDOWN},
    {SCK_NEXT, 		SCI_SHIFT, 	SCI_PAGEDOWNEXTEND},
    {SCK_NEXT,		SCI_ASHIFT,	SCI_PAGEDOWNRECTEXTEND},
    {SCK_DELETE, 	SCI_NORM,	SCI_CLEAR},
    {SCK_DELETE, 	SCI_SHIFT,	SCI_CUT},
    {SCK_DELETE, 	SCI_CTRL,	SCI_DELWORDRIGHT},
    {SCK_DELETE,	SCI_CSHIFT,	SCI_DELLINERIGHT},
    {SCK_INSERT, 		SCI_NORM,	SCI_EDITTOGGLEOVERTYPE},
    {SCK_INSERT, 		SCI_SHIFT,	SCI_PASTE},
    {SCK_INSERT, 		SCI_CTRL,	SCI_COPY},
    {SCK_ESCAPE,  	SCI_NORM,	SCI_CANCEL},
    {SCK_BACK,		SCI_NORM, 	SCI_DELETEBACK},
    {SCK_BACK,		SCI_SHIFT, 	SCI_DELETEBACK},
    {SCK_BACK,		SCI_CTRL, 	SCI_DELWORDLEFT},
    {SCK_BACK, 		SCI_ALT,	SCI_UNDO},
    {SCK_BACK,		SCI_CSHIFT,	SCI_DELLINELEFT},
    {'Z', 			SCI_CTRL,	SCI_UNDO},
#if OS_X_KEYS
    {'Z', 			SCI_CSHIFT,	SCI_REDO},
#else
    {'Y', 			SCI_CTRL,	SCI_REDO},
#endif
    {'X', 			SCI_CTRL,	SCI_CUT},
    {'C', 			SCI_CTRL,	SCI_COPY},
    {'V', 			SCI_CTRL,	SCI_PASTE},
    {'A', 			SCI_CTRL,	SCI_SELECTALL},
    {SCK_TAB,		SCI_NORM,	SCI_TAB},
    {SCK_TAB,		SCI_SHIFT,	SCI_BACKTAB},
    {SCK_RETURN, 	SCI_NORM,	SCI_NEWLINE},
    {SCK_RETURN, 	SCI_SHIFT,	SCI_NEWLINE},
    {SCK_ADD, 		SCI_CTRL,	SCI_ZOOMIN},
    {SCK_SUBTRACT,	SCI_CTRL,	SCI_ZOOMOUT},
    {SCK_DIVIDE,	SCI_CTRL,	SCI_SETZOOM},
    {'L', 			SCI_CTRL,	SCI_LINECUT},
    {'L', 			SCI_CSHIFT,	SCI_LINEDELETE},
    {'T', 			SCI_CSHIFT,	SCI_LINECOPY},
    {'T', 			SCI_CTRL,	SCI_LINETRANSPOSE},
    {'D', 			SCI_CTRL,	SCI_SELECTIONDUPLICATE},
    {'U', 			SCI_CTRL,	SCI_LOWERCASE},
    {'U', 			SCI_CSHIFT,	SCI_UPPERCASE},
    {0,0,0},
};

