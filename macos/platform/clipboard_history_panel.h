#pragma once

void initializeClipboardHistoryPanel();
void destroyClipboardHistoryPanel();
void setClipboardHistoryEnabled(bool enabled);
bool isClipboardHistoryEnabled();

// Returns opaque pointer to the panel's container view (NSView*),
// or nullptr if the panel has not been initialized.
// Used by relayoutFunctionListPanel() for geometry positioning.
void* clipboardHistoryContainerView();
