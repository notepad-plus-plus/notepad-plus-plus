#pragma once

// Document map lifecycle and behavior.
void initializeDocumentMap();
void destroyDocumentMap();
void relayoutDocumentMap();
void setDocumentMapEnabled(bool enabled);
bool isDocumentMapEnabled();

// Keeps minimap bound to the active editor's document.
void bindDocumentMapToActiveView();

// Update minimap viewport overlay from current active editor position.
void updateDocumentMapViewport();

// Forward scroll notifications from active editor updates.
void handleDocumentMapUpdateUI(void* sourceSci, int updatedFlags);
