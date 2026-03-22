// search_results_panel.h — Search results panel
#pragma once

#include "find_in_files.h"

void showSearchResultsPanel(const FIFSearchResult& result);
void clearSearchResults();
void hideSearchResultsPanel();
bool isSearchResultsPanelVisible();
