// scintilla_config.mm — Scintilla editor configuration
// Part of the Notepad++ macOS port modular refactor.

#include "scintilla_config.h"
#include "npp_constants.h"
#include "app_state.h"
#include "scintilla_bridge.h"
#include "keyboard_shortcuts.h"

void configureScintilla(void* sci)
{
	if (!sci) return;

	ScintillaBridge_sendMessage(sci, SCI_SETCODEPAGE, 65001, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETWRAPMODE, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTABWIDTH, ctx().tabWidth, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETUSETABS, 0, 0);

	ScintillaBridge_sendMessage(sci, SCI_SETMARGINTYPEN, 0, 1);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 0, 50);

	ScintillaBridge_sendMessage(sci, SCI_SETMARGINTYPEN, 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 1, 16);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINMASKN, 1, BOOKMARK_MASK);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINSENSITIVEN, 1, 1);

	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, BOOKMARK_MARKER, SC_MARK_BOOKMARK);
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETFORE, BOOKMARK_MARKER, 0xFFFFFF);
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETBACK, BOOKMARK_MARKER, 0xFF8000);

	ScintillaBridge_sendMessage(sci, SCI_SETMARGINTYPEN, 2, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 2, 16);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINSENSITIVEN, 2, 1);

	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN,    SC_MARK_BOXMINUS);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDER,        SC_MARK_BOXPLUS);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB,     SC_MARK_VLINE);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL,    SC_MARK_LCORNER);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND,     SC_MARK_BOXPLUSCONNECTED);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

	ScintillaBridge_sendMessage(sci, SCI_SETFOLDFLAGS, 16, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_CLICK, 0);

	ScintillaBridge_sendMessage(sci, SCI_SETCARETLINEVISIBLE, ctx().showCaretLine ? 1 : 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETCARETLINEBACK, 0xF0F0F0, 0);

	if (ctx().zoomLevel != 0)
		ScintillaBridge_sendMessage(sci, SCI_SETZOOM, ctx().zoomLevel, 0);

	configureKeyboardShortcuts(sci);
}
