#pragma once

void initializeClipboardHistoryPanel();
void destroyClipboardHistoryPanel();
void setClipboardHistoryEnabled(bool enabled);
bool isClipboardHistoryEnabled();

// Directly capture the current clipboard content into history.
// Called from in-app copy/cut handlers to avoid missing rapid operations
// that the 500ms polling timer would drop.
void captureClipboardEntry();

// Returns opaque pointer to the panel's container view (NSView*),
// or nullptr if the panel has not been initialized.
// Used by relayoutPanels() for geometry positioning.
void* clipboardHistoryContainerView();
