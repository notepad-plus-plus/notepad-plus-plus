// sync_scroll.mm — Synchronize split-view scrolling.

#include "sync_scroll.h"
#include "app_state.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"

static inline intptr_t firstVisibleOf(void* sci)
{
	return ScintillaBridge_sendMessage(sci, SCI_GETFIRSTVISIBLELINE, 0, 0);
}

static inline intptr_t maxFirstVisible(void* sci)
{
	intptr_t lineCount = ScintillaBridge_sendMessage(sci, SCI_GETLINECOUNT, 0, 0);
	intptr_t linesOnScreen = ScintillaBridge_sendMessage(sci, SCI_LINESONSCREEN, 0, 0);
	if (linesOnScreen < 1)
		linesOnScreen = 1;
	intptr_t maxFirst = lineCount - linesOnScreen;
	return maxFirst > 0 ? maxFirst : 0;
}

static inline intptr_t clampFirstVisible(void* sci, intptr_t line)
{
	if (line < 0)
		return 0;
	intptr_t maxFirst = maxFirstVisible(sci);
	if (line > maxFirst)
		return maxFirst;
	return line;
}

static void clearVerticalAnchor()
{
	ctx().syncScrollVDelta = 0;
	ctx().syncScrollVDeltaValid = false;
}

void refreshSyncScrollAnchor()
{
	if (!ctx().isSplit || !ctx().syncScrolling || !ctx().scintillaView || !ctx().scintillaView2)
	{
		clearVerticalAnchor();
		return;
	}

	intptr_t mainFirst = firstVisibleOf(ctx().scintillaView);
	intptr_t subFirst = firstVisibleOf(ctx().scintillaView2);
	ctx().syncScrollVDelta = mainFirst - subFirst;
	ctx().syncScrollVDeltaValid = true;
}

void setSyncScrollingEnabled(bool enabled)
{
	ctx().syncScrolling = enabled;
	if (enabled)
		refreshSyncScrollAnchor();
	else
		clearVerticalAnchor();
}

bool isSyncScrollingEnabled()
{
	return ctx().syncScrolling;
}

static bool canSync(void* sourceSci, void*& targetSci)
{
	targetSci = nullptr;
	if (!ctx().isSplit || !ctx().syncScrolling || ctx().syncScrollReentrant || ctx().suppressSyncScroll)
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

// Sync vertical position from source to target using anchored visible-line delta.
// Returns true if the target was actually scrolled.
static bool syncVertical(void* sourceSci, void* targetSci)
{
	if (!ctx().syncScrollVDeltaValid)
		refreshSyncScrollAnchor();
	if (!ctx().syncScrollVDeltaValid)
		return false;

	intptr_t sourceFirst = firstVisibleOf(sourceSci);
	intptr_t targetCurrent = firstVisibleOf(targetSci);

	intptr_t targetDesired = targetCurrent;
	if (sourceSci == ctx().scintillaView && targetSci == ctx().scintillaView2)
		targetDesired = sourceFirst - ctx().syncScrollVDelta;
	else if (sourceSci == ctx().scintillaView2 && targetSci == ctx().scintillaView)
		targetDesired = sourceFirst + ctx().syncScrollVDelta;
	else
		return false;

	targetDesired = clampFirstVisible(targetSci, targetDesired);

	bool moved = false;
	if (targetDesired != targetCurrent)
	{
		ScintillaBridge_sendMessage(targetSci, SCI_SETFIRSTVISIBLELINE, targetDesired, 0);
		moved = true;
	}

	// Re-anchor after each sync to avoid stale-threshold hysteresis after clamping.
	refreshSyncScrollAnchor();
	return moved;
}

void handleSyncScrollUpdate(void* sourceSci, int updatedFlags)
{
	void* targetSci = nullptr;
	if (!canSync(sourceSci, targetSci))
		return;

	ctx().syncScrollReentrant = true;

	// Always verify vertical sync — not just on SC_UPDATE_V_SCROLL.
	// Scintilla defers SCN_UPDATEUI to paint/idle, and at scroll boundaries
	// the V_SCROLL flag can be absent even when the position has drifted.
	// The position check (targetDesired != targetCurrent) inside syncVertical
	// prevents feedback loops: when the deferred echo fires from the target,
	// the source is already at the correct position, so no reverse scroll occurs.
	syncVertical(sourceSci, targetSci);

	if (updatedFlags & SC_UPDATE_H_SCROLL)
	{
		// Horizontal sync is noisy with wrap mismatch. Sync only when wrap modes match.
		intptr_t sourceWrap = ScintillaBridge_sendMessage(sourceSci, SCI_GETWRAPMODE, 0, 0);
		intptr_t targetWrap = ScintillaBridge_sendMessage(targetSci, SCI_GETWRAPMODE, 0, 0);
		if (sourceWrap == targetWrap)
		{
			intptr_t xOffset = ScintillaBridge_sendMessage(sourceSci, SCI_GETXOFFSET, 0, 0);
			intptr_t targetX = ScintillaBridge_sendMessage(targetSci, SCI_GETXOFFSET, 0, 0);
			if (xOffset != targetX)
				ScintillaBridge_sendMessage(targetSci, SCI_SETXOFFSET, xOffset, 0);
		}
	}

	ctx().syncScrollReentrant = false;
}

void syncScrollNow()
{
	if (!ctx().isSplit || !ctx().syncScrolling)
		return;

	void* source = ctx().activeScintillaView();
	void* target = (source == ctx().scintillaView) ? ctx().scintillaView2 : ctx().scintillaView;
	if (!source || !target)
		return;

	ctx().syncScrollReentrant = true;
	syncVertical(source, target);
	ctx().syncScrollReentrant = false;
	refreshSyncScrollAnchor();
}
