#pragma once

void setSyncScrollingEnabled(bool enabled);
bool isSyncScrollingEnabled();

// sourceSci is the Scintilla view that emitted SCN_UPDATEUI.
void handleSyncScrollUpdate(void* sourceSci, int updatedFlags);

// Perform a single sync from the active view. Use after batch fold operations.
void syncScrollNow();

// Recompute and cache current relative vertical sync baseline.
void refreshSyncScrollAnchor();
