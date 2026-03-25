// change_history.mm — Change history margin markers
// Part of the Notepad++ macOS port modular refactor.

#include "change_history.h"
#include "npp_constants.h"
#include "app_state.h"
#include "scintilla_bridge.h"

static constexpr int kChangeHistoryMarginWidth = 4;

static int changeHistoryMode(bool showMarginMarkers)
{
	return showMarginMarkers ?
		(SC_CHANGE_HISTORY_ENABLED | SC_CHANGE_HISTORY_MARKERS) :
		SC_CHANGE_HISTORY_ENABLED;
}

void configureChangeHistory(void* sci)
{
	if (!sci) return;

	// Enable change tracking + marker display.
	// SC_CHANGE_HISTORY_ENABLED is required for Scintilla to actually track edits.
	ScintillaBridge_sendMessage(sci, SCI_SETCHANGEHISTORY, changeHistoryMode(ctx().showChangeHistory), 0);

	// Set up margin 3 as a narrow symbol margin for change bars
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINTYPEN, CHANGE_HISTORY_MARGIN, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, CHANGE_HISTORY_MARGIN,
		ctx().showChangeHistory ? kChangeHistoryMarginWidth : 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINMASKN, CHANGE_HISTORY_MARGIN, CHANGE_HISTORY_MASK);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINSENSITIVEN, CHANGE_HISTORY_MARGIN, 0);

	// Use SC_MARK_FULLRECT for a solid change bar
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_HISTORY_MODIFIED, SC_MARK_FULLRECT);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_HISTORY_SAVED, SC_MARK_FULLRECT);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN, SC_MARK_FULLRECT);
	ScintillaBridge_sendMessage(sci, SCI_MARKERDEFINE, SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED, SC_MARK_FULLRECT);
}

void applyChangeHistoryColors(void* sci, bool isDark)
{
	if (!sci) return;

	// Modified (unsaved changes) — orange/amber
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_MODIFIED,
		isDark ? 0x00A5FF : 0x0080CC);  // BGR: orange tones
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_MODIFIED,
		isDark ? 0x00A5FF : 0x0080CC);

	// Saved (modified then saved) — green
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_SAVED,
		isDark ? 0x50C850 : 0x008000);  // BGR: green tones
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_SAVED,
		isDark ? 0x50C850 : 0x008000);

	// Reverted to origin — blue
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN,
		isDark ? 0xFF8050 : 0xCC6600);  // BGR: blue tones
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN,
		isDark ? 0xFF8050 : 0xCC6600);

	// Reverted to modified — cyan/teal
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETBACK, SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED,
		isDark ? 0xD0D050 : 0xA0A000);  // BGR: cyan/teal tones
	ScintillaBridge_sendMessage(sci, SCI_MARKERSETFORE, SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED,
		isDark ? 0xD0D050 : 0xA0A000);
}

void setChangeHistoryMarginVisible(void* sci, bool visible)
{
	if (!sci) return;
	// If markers are enabled but not mapped to any visible margin, Scintilla paints
	// them as line background colours in the text area. Keep tracking on, but only
	// enable marker rendering when the change-history margin is visible.
	ScintillaBridge_sendMessage(sci, SCI_SETCHANGEHISTORY, changeHistoryMode(visible), 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, CHANGE_HISTORY_MARGIN,
		visible ? kChangeHistoryMarginWidth : 0);
}
