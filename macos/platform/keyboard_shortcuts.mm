// keyboard_shortcuts.mm — Native macOS keyboard shortcut configuration
// Part of the Notepad++ macOS port modular refactor.
//
// Remaps Scintilla key bindings to match macOS Human Interface Guidelines:
//   Home/End        → line start/end (not document)
//   Cmd+Up/Down     → document start/end
//   Cmd+Left/Right  → line start/end
//   Opt+Left/Right  → word navigation
//   Opt+Backspace   → delete word backward
//   Opt+Delete      → delete word forward
//   Cmd+Backspace   → delete to line start

#include "keyboard_shortcuts.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"

// Pack key + modifiers into wParam for SCI_ASSIGNCMDKEY / SCI_CLEARCMDKEY
static inline int keyMod(int key, int mod) { return key | (mod << 16); }

// Clear then assign a key binding (avoids conflicts with Scintilla defaults)
static void reassign(void* sci, int key, int mod, int sciCmd)
{
	ScintillaBridge_sendMessage(sci, SCI_CLEARCMDKEY, keyMod(key, mod), 0);
	ScintillaBridge_sendMessage(sci, SCI_ASSIGNCMDKEY, keyMod(key, mod), sciCmd);
}

// Bind a Cmd shortcut to both SCMOD_SUPER and SCMOD_META variants.
// Cocoa Scintilla may map the Command key to either modifier depending on build.
static void reassignCmd(void* sci, int key, int extraMod, int sciCmd)
{
	reassign(sci, key, SCMOD_SUPER | extraMod, sciCmd);
	reassign(sci, key, SCMOD_META | extraMod, sciCmd);
}

void configureKeyboardShortcuts(void* sci)
{
	if (!sci) return;

	// --- Home/End: line start/end (not document) ---
	// VCHOME goes to first non-whitespace char (smarter), matching most macOS editors
	reassign(sci, SCK_HOME, 0,           SCI_VCHOME);
	reassign(sci, SCK_HOME, SCMOD_SHIFT, SCI_VCHOMEEXTEND);
	reassign(sci, SCK_END,  0,           SCI_LINEEND);
	reassign(sci, SCK_END,  SCMOD_SHIFT, SCI_LINEENDEXTEND);

	// --- Cmd+Up/Down: document start/end ---
	reassignCmd(sci, SCK_UP,   0,           SCI_DOCUMENTSTART);
	reassignCmd(sci, SCK_UP,   SCMOD_SHIFT, SCI_DOCUMENTSTARTEXTEND);
	reassignCmd(sci, SCK_DOWN, 0,           SCI_DOCUMENTEND);
	reassignCmd(sci, SCK_DOWN, SCMOD_SHIFT, SCI_DOCUMENTENDEXTEND);

	// --- Cmd+Left/Right: line start/end ---
	reassignCmd(sci, SCK_LEFT,  0,           SCI_VCHOME);
	reassignCmd(sci, SCK_LEFT,  SCMOD_SHIFT, SCI_VCHOMEEXTEND);
	reassignCmd(sci, SCK_RIGHT, 0,           SCI_LINEEND);
	reassignCmd(sci, SCK_RIGHT, SCMOD_SHIFT, SCI_LINEENDEXTEND);

	// --- Opt+Left/Right: word navigation ---
	reassign(sci, SCK_LEFT,  SCMOD_ALT,                SCI_WORDLEFT);
	reassign(sci, SCK_LEFT,  SCMOD_ALT | SCMOD_SHIFT,  SCI_WORDLEFTEXTEND);
	reassign(sci, SCK_RIGHT, SCMOD_ALT,                SCI_WORDRIGHT);
	reassign(sci, SCK_RIGHT, SCMOD_ALT | SCMOD_SHIFT,  SCI_WORDRIGHTEXTEND);

	// --- Opt+Backspace/Delete: delete word ---
	reassign(sci, SCK_BACK,   SCMOD_ALT, SCI_DELWORDLEFT);
	reassign(sci, SCK_DELETE, SCMOD_ALT, SCI_DELWORDRIGHT);

	// --- Cmd+Backspace: delete to line start ---
	reassignCmd(sci, SCK_BACK, 0, SCI_DELLINELEFT);
}
