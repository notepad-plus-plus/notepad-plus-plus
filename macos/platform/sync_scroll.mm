// sync_scroll.mm — Synchronize split-view scrolling.

#include "sync_scroll.h"
#include "app_state.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"

void setSyncScrollingEnabled(bool enabled)
{
	ctx().syncScrolling = enabled;
}

bool isSyncScrollingEnabled()
{
	return ctx().syncScrolling;
}

static bool canSync(void* sourceSci, void*& targetSci)
{
	targetSci = nullptr;
	if (!ctx().isSplit || !ctx().syncScrolling || ctx().syncScrollReentrant)
		return false;
	if (!sourceSci || !ctx().scintillaView || !ctx().scintillaView2)
		return false;

	if (sourceSci == ctx().scintillaView)
		targetSci = ctx().scintillaView2;
	else if (sourceSci == ctx().scintillaView2)
		targetSci = ctx().scintillaView;
	else
		return false;

	return targetSci != nullptr;
}

void handleSyncScrollUpdate(void* sourceSci, int updatedFlags)
{
	void* targetSci = nullptr;
	if (!canSync(sourceSci, targetSci))
		return;

	ctx().syncScrollReentrant = true;

	if (updatedFlags & SC_UPDATE_V_SCROLL)
	{
		intptr_t firstVisible = ScintillaBridge_sendMessage(sourceSci, SCI_GETFIRSTVISIBLELINE, 0, 0);
		ScintillaBridge_sendMessage(targetSci, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
	}

	if (updatedFlags & SC_UPDATE_H_SCROLL)
	{
		// Horizontal sync is noisy with wrap mismatch. Sync only when wrap modes match.
		intptr_t sourceWrap = ScintillaBridge_sendMessage(sourceSci, SCI_GETWRAPMODE, 0, 0);
		intptr_t targetWrap = ScintillaBridge_sendMessage(targetSci, SCI_GETWRAPMODE, 0, 0);
		if (sourceWrap == targetWrap)
		{
			intptr_t xOffset = ScintillaBridge_sendMessage(sourceSci, SCI_GETXOFFSET, 0, 0);
			ScintillaBridge_sendMessage(targetSci, SCI_SETXOFFSET, xOffset, 0);
		}
	}

	ctx().syncScrollReentrant = false;
}
