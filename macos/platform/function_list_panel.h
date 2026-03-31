#pragma once

#include <cstdint>

void initializeFunctionListPanel();
void destroyFunctionListPanel();
void setFunctionListEnabled(bool enabled);

// Returns opaque pointer to the panel's container view (NSView*),
// or nullptr if the panel has not been initialized.
void* functionListContainerView();
bool isFunctionListEnabled();

void bindFunctionListToActiveView();
void updateFunctionListNow();
void scheduleFunctionListRefresh();
void invalidateFunctionListPendingRefresh();
void invalidateFunctionListCacheForDocument(uint64_t documentId);
bool isFunctionListShuttingDown();
void setFunctionListParseProgress(int percent);
