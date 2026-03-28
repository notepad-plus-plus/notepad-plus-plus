#pragma once

void initializeFunctionListPanel();
void destroyFunctionListPanel();
void relayoutFunctionListPanel();
void setFunctionListEnabled(bool enabled);
bool isFunctionListEnabled();

void bindFunctionListToActiveView();
void updateFunctionListNow();
void scheduleFunctionListRefresh();
void invalidateFunctionListPendingRefresh();
bool isFunctionListShuttingDown();
