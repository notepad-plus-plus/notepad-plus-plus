// incremental_search.h — Incremental search bar (Cmd+F)
#pragma once

void showIncrementalSearch();
void hideIncrementalSearch();
bool isIncrementalSearchVisible();
void clearIncrementalSearchHighlights();
void updateIncrementalSearchTarget();

// Match navigation (called by wndproc for F3/Shift+F3 when bar is visible)
void doIncrSearchNext();
void doIncrSearchPrev();
