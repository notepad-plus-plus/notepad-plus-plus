#pragma once

#include <cstdint>

void initializeFunctionListPanel();
void destroyFunctionListPanel();
void relayoutFunctionListPanel();
void setFunctionListEnabled(bool enabled);
bool isFunctionListEnabled();

void bindFunctionListToActiveView();
void updateFunctionListNow();
void scheduleFunctionListRefresh();
void invalidateFunctionListPendingRefresh();
void invalidateFunctionListCacheForDocument(uint64_t documentId);
bool isFunctionListShuttingDown();
void setFunctionListParseProgress(int percent);
