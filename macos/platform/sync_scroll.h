#pragma once

void setSyncScrollingEnabled(bool enabled);
bool isSyncScrollingEnabled();

// sourceSci is the Scintilla view that emitted SCN_UPDATEUI.
void handleSyncScrollUpdate(void* sourceSci, int updatedFlags);
